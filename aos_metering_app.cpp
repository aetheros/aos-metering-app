// vim: sw=4 expandtab
// Copyright (c) Aetheros, Inc.  See COPYRIGHT
#include <aos/AppMain.hpp>
#include <aos/Log.hpp>
#include <m2m/AppEntity.hpp>
#include <xsd/m2m/Subscription.hpp>
#include <xsd/m2m/ContentInstance.hpp>
#include <xsd/m2m/Names.hpp>
#include <xsd/mtrsvc/MeterServicePolicy.hpp>
#include <xsd/mtrsvc/MeterRead.hpp>
#include <xsd/mtrsvc/Names.hpp>
#include <thread>
#include <fstream>
#include <cstdio>

using std::chrono::seconds;
using std::chrono::minutes;

m2m::AppEntity appEntity; // OneM2M Application Entity (AE) object

void notificationCallback(m2m::Notification notification);
bool create_subscription();
bool create_meter_read_pq_policy();
bool create_meter_read_summations_policy();

[[noreturn]] void usage(const char *prog)
{
        fprintf(stderr, "Usage %s [-d] [-p <POA-URI>|-a <IPADDR[:PORT]>] [CSE-URI]\n", prog);
        exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    // scope the application main loop
    aos::AppMain appMain;
    const char *poaUri = nullptr;
    const char *poaAddr = nullptr;

    int opt;
    while ((opt = getopt(argc, argv, "dp:a:")) != -1)
    {
        switch (opt)
        {
        case 'd':
            aos::setLogLevel(aos::LogLevel::LOG_DEBUG);
            break;
        case 'p':
            poaUri = optarg;
            break;
        case 'a':
            poaAddr = optarg;
            break;
        default:
            usage(argv[0]);
            break;
        }
    }

    if (argc-optind > 1)
    {
        usage(argv[0]);
    }

    // initialize the AE object
    appEntity = m2m::AppEntity(notificationCallback);

    if (argc-optind == 1)
    {
        appEntity.setCseUri(argv[optind]);
    }

    if (poaUri)
    {
        appEntity.setPoaUri(poaUri);
    }
    else if (poaAddr)
    {
        appEntity.setPoaAddr(poaAddr);
    }

    while (true)
    {
        seconds backoffSeconds{30};

        // activate the AE (checks registration, registers if necessary, updates poa if necessary)
        logInfo("activating");
        while (!appEntity.activate())
        {
            auto failureReason = appEntity.getActivationFailureReason();
            logError("activation failed: reason: " << failureReason);
            if (failureReason != m2m::ActivationFailureReason::Timeout &&
                    failureReason != m2m::ActivationFailureReason::NotRegistered)
            {
                logInfo("retrying in " << backoffSeconds.count() << " seconds");
                std::this_thread::sleep_for(backoffSeconds);
                if (backoffSeconds < minutes{16})
                    backoffSeconds *= 2;
            }
        }

        logInfo("activated");
        if (!create_subscription())
        {
            logError("subscription creation failed");
            std::this_thread::sleep_for(seconds{30});
            continue;
        }

        if (!create_meter_read_pq_policy())
        {
            logError("power-quality meter read policy creation failed");
            std::this_thread::sleep_for(seconds{30});
            continue;
        }

        if (!create_meter_read_summations_policy())
        {
            logError("summations meter read policy creation failed");
            std::this_thread::sleep_for(seconds{30});
            continue;
        }

        break;
    }

    appEntity.waitForever();
}

bool create_subscription()
{
    xsd::m2m::Subscription subscription = xsd::m2m::Subscription::Create();

    subscription.creator = std::string();

    // set the subscription's name
    subscription.resourceName = "metersv-samp1-sub-01";

    // have all resource attributes be provided in the notifications
    subscription.notificationContentType = xsd::m2m::NotificationContentType::all_attributes;

    // set the notification destination to be this AE.
    subscription.notificationURI = xsd::m2m::ListOfURIs();
    subscription.notificationURI->push_back(appEntity.getResourceId());

    // set eventNotificationCriteria to creation and deletion of child resources
    xsd::m2m::EventNotificationCriteria eventNotificationCriteria;
    eventNotificationCriteria.notificationEventType.assign().push_back(xsd::m2m::NotificationEventType::Create_of_Direct_Child_Resource);
    //eventNotificationCriteria.notificationEventType.push_back(xsd::m2m::NotificationEventType::Delete_of_Direct_Child_Resource);
    subscription.eventNotificationCriteria = std::move(eventNotificationCriteria);

    m2m::Request request = appEntity.newRequest(xsd::m2m::Operation::Create, m2m::To{"./metersvc/reads"});
    request.req->resultContent = xsd::m2m::ResultContent::Nothing;
    request.req->resourceType = xsd::m2m::ResourceType::subscription;
    request.req->primitiveContent = xsd::toAnyNamed(subscription);

    appEntity.sendRequest(request);
    auto response = appEntity.getResponse(request);

    logInfo("subscription: " << toString(response->responseStatusCode));

    return (response->responseStatusCode == xsd::m2m::ResponseStatusCode::CREATED || response->responseStatusCode == xsd::m2m::ResponseStatusCode::CONFLICT);
}

bool create_meter_read_policy(const char *readingType, unsigned recurrencePeriod, const std::string &polId)
{
    xsd::mtrsvc::ScheduleInterval scheduleInterval;
    scheduleInterval.end = nullptr;
    scheduleInterval.start = "2020-06-19T00:00:00";

    xsd::mtrsvc::TimeSchedule timeSchedule;
    timeSchedule.recurrencePeriod = recurrencePeriod;
    timeSchedule.scheduleInterval = std::move(scheduleInterval);

    xsd::mtrsvc::MeterReadSchedule meterReadSchedule;
    meterReadSchedule.readingType = readingType;
    meterReadSchedule.timeSchedule = std::move(timeSchedule);

    xsd::mtrsvc::MeterServicePolicy meterServicePolicy;
    meterServicePolicy = std::move(meterReadSchedule);

    xsd::m2m::ContentInstance policyInst = xsd::m2m::ContentInstance::Create();
    policyInst.content = xsd::toAnyTypeUnnamed(meterServicePolicy);

    policyInst.resourceName = "metersvc-sampl-pol-" + polId;

    m2m::Request request = appEntity.newRequest(xsd::m2m::Operation::Create, m2m::To{"./metersvc/policies"});
    request.req->resultContent = xsd::m2m::ResultContent::Nothing;
    request.req->resourceType = xsd::m2m::ResourceType::contentInstance;
    request.req->primitiveContent = xsd::toAnyNamed(policyInst);

    appEntity.sendRequest(request);
    auto response = appEntity.getResponse(request);

    logInfo(readingType << " policy creation: " << toString(response->responseStatusCode));

    return (response->responseStatusCode == xsd::m2m::ResponseStatusCode::CREATED || response->responseStatusCode == xsd::m2m::ResponseStatusCode::CONFLICT);
}

bool create_meter_read_pq_policy()
{
    return create_meter_read_policy("powerQuality", 120, "01");
}

bool create_meter_read_summations_policy()
{
    return create_meter_read_policy("summations", 240, "02");
}

void notificationCallback(m2m::Notification notification)
{
    if (!notification.notificationEvent.isSet())
    {
        logWarn("notification has no notificationEvent");
        return;
    }

    logDebug("got notification type " << toString(notification.notificationEvent->notificationEventType));

    if (notification.notificationEvent->notificationEventType != xsd::m2m::NotificationEventType::Create_of_Direct_Child_Resource)
    {
        return;
    }

    auto contentInstance = notification.notificationEvent->representation->extractNamed<xsd::m2m::ContentInstance>();
    auto meterRead = contentInstance.content->extractUnnamed<xsd::mtrsvc::MeterRead>();
    auto &meterSvcData = *meterRead.meterSvcData;

    logInfo("timestamp: " << meterSvcData.readTimeLocal);
    if (meterSvcData.powerQuality.isSet())
    {
        std::ofstream output("powerquality_meter_data.txt");
        logInfo("powerQuality: " << *meterSvcData.powerQuality);
        output << "timestamp: " << meterSvcData.readTimeLocal << '\n';
        output << "powerQuality:\n" << *meterSvcData.powerQuality << '\n';
    }
    if (meterSvcData.summations.isSet())
    {
        std::ofstream output("summation_meter_data.txt");
        logInfo("summations: " << *meterSvcData.summations);
        output << "timestamp: " << meterSvcData.readTimeLocal << '\n';
        output << "summations:\n" << *meterSvcData.summations << '\n';
    }
}
