// vim: sw=4 expandtab
#include <aos/AppMain.hpp>
#include <aos/Log.hpp>
#include <m2m/AppEntity.hpp>
#include <m2m/CdtSubscription.hpp>
#include <m2m/CdtContentInstance.hpp>
#include <m2m/RequestPrimitive.hpp>
#include <m2m/ResponsePrimitive.hpp>
#include <m2m/Names.hpp>
#include <aos/CdtMeterReadSchedulePolicy.hpp>
#include <aos/Names.hpp>
#include <thread>
#include <unistd.h>

using std::chrono::seconds;
using std::chrono::minutes;

m2m::AppEntity appEntity;

void notificationCallback(m2m::Notification notification);
bool create_subscription();
bool create_meter_read_policy();

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
    appEntity = m2m::AppEntity("app_config.json", notificationCallback);

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

        if (!create_meter_read_policy())
        {
            logError("meter read policy creation failed");
            std::this_thread::sleep_for(seconds{30});
            continue;
        }

        break;
    }

    appEntity.waitForever();
}

bool create_subscription()
{
    m2m::CdtSubscription subscription = m2m::CdtSubscription::Create();

    subscription.creator = std::string();

    // set the subscription's name
    subscription.resourceName = "metersv-samp1-sub-01";

    // have all resource attributes be provided in the notifications
    subscription.notificationContentType = m2m::NotificationContentType::all_attributes;

    // set the notification destination to be this AE.
    subscription.notificationURI = m2m::CdtListOfURIs();
    subscription.notificationURI->push_back(appEntity.getResourceId());

    // set eventNotificationCriteria to creation and deletion of child resources
    m2m::CdtEventNotificationCriteria eventNotificationCriteria;
    eventNotificationCriteria.notificationEventType.push_back(m2m::NotificationEventType::Create_of_Direct_Child_Resource);
    //eventNotificationCriteria.notificationEventType.push_back(m2m::NotificationEventType::Delete_of_Direct_Child_Resource);
    subscription.eventNotificationCriteria = std::move(eventNotificationCriteria);

    m2m::Request request = appEntity.newRequest(m2m::Operation::Create, m2m::To{"./metersvc/reads"});
    request.req->resultContent = m2m::ResultContent::Nothing;
    request.req->resourceType = m2m::ResourceType::subscription;
    request.req->marshallPC(m2m::sn_subscription, subscription);

    appEntity.sendRequest(request);
    auto response = appEntity.getResponse(request);

    logInfo("subscription: " << toString(response->responseStatusCode));

    return (response->responseStatusCode == m2m::ResponseStatusCode::CREATED || response->responseStatusCode == m2m::ResponseStatusCode::CONFLICT);
}

bool create_meter_read_policy()
{
    aos::CdtScheduleInterval scheduleInterval;
    scheduleInterval.end = nullptr;
    scheduleInterval.start = "2020-06-19T00:00:00";

    aos::CdtTimeSchedule timeSchedule;
    timeSchedule.recurrencePeriod = 120;
    timeSchedule.scheduleInterval = std::move(scheduleInterval);

    aos::CdtMeterReadSchedule meterReadSchedule;
    meterReadSchedule.readingType = "powerQuality";
    meterReadSchedule.timeSchedule = std::move(timeSchedule);

    aos::CdtMeterServicePolicy meterServicePolicy;
    meterServicePolicy.meterReadSchedule = std::move(meterReadSchedule);

    m2m::CdtContentInstance policyInst = m2m::CdtContentInstance::Create();
    policyInst.content = xs::toAnyType(meterServicePolicy);

    policyInst.resourceName = "metersvc-sampl-pol-01";

    m2m::Request request = appEntity.newRequest(m2m::Operation::Create, m2m::To{"./metersvc/policies"});
    request.req->resultContent = m2m::ResultContent::Nothing;
    request.req->resourceType = m2m::ResourceType::contentInstance;
    request.req->marshallPC(m2m::sn_contentInstance, policyInst);

    appEntity.sendRequest(request);
    auto response = appEntity.getResponse(request);

    logInfo("policy creation: " << toString(response->responseStatusCode));

    return (response->responseStatusCode == m2m::ResponseStatusCode::CREATED || response->responseStatusCode == m2m::ResponseStatusCode::CONFLICT);
}

void notificationCallback(m2m::Notification notification)
{
    if (!notification.notificationEvent)
    {
        logWarn("notification has no notificationEvent");
        return;
    }

    logDebug("got notification type " << toString(notification.notificationEvent->notificationEventType));

    if (notification.notificationEvent->notificationEventType != m2m::NotificationEventType::Create_of_Direct_Child_Resource)
    {
        return;
    }

    auto contentInstance = xs::fromAnyTypeElement<m2m::CdtContentInstance>(*notification.notificationEvent->representation, m2m::sn_contentInstance);
    auto meterSvcData = xs::fromAnyTypeElement<aos::CdtMeterSvcData>(*contentInstance.content, aos::sn_meterSvcData);

    logInfo("timestamp: " << meterSvcData.readTimeLocal);
    if (meterSvcData.powerQuality)
    {
        logInfo("powerQuality: " << *meterSvcData.powerQuality);
    }
    if (meterSvcData.summations)
    {
        logInfo("summations: " << *meterSvcData.summations);
    }
}

