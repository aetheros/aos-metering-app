# AOS Device App SDK Sample Application


## Quick Start Guide ##

### 1. Install the SDK.

Download the SDK installer and run it.  It will prompt you for an install location.

```sh
aos SDK installer version v6.0.6
==========================================
Enter target directory for SDK (default: /usr/local/aos-esr1k-x86_64):
```

Here's a complete example of installing the SDK under /opt/tools:

```sh
$ ./aos-esr1k-metersim-toolchain-v6.0.6.sh
aos SDK installer version v6.0.6
=====================================
Enter target directory for SDK (default: /usr/local/aos-esr1k-x86_64): /opt/tools/aos-esr1k-x86_64
You are about to install the SDK to "/opt/tools/aos-esr1k-x86_64". Proceed[Y/n]? <Enter>
Extracting SDK......................done
Setting it up...done
SDK has been successfully set up and is ready to be used.
Each time you wish to use the SDK in a new shell session, you need to source the environment setup script e.g.
$ . /opt/tools/aos-esr1k-x86_64/environment-setup-armv7a-vfp-neon-oe-linux-gnueabi
```

### 2. Clone the sample app from [github](https://github.com/aetheros/aos-metering-app.git).

```sh
git clone https://github.com/aetheros/aos-metering-app.git
cd aos-metering-app
```

### 3. Build the sample app using the SDK toolchain.

To use the SDK, source its environment file. This sets up the current shell environment for using the SDK toolchain.
Then run `make` to build the app.

```sh
. /opt/tools/aos-esr1k-x86_64/environment-setup-armv7a-vfp-neon-oe-linux-gnueabi
make
```

Alternatively, you can use the convenience wrapper scripts provided in the aos-metering-app repo:

- `./make_app` - Build `aos_metering_app`.
- `./aospkg` - Create an unsigned AOS package.
- `./aossignpkg` - Create a signed AOS package.
- `./aosapp` - Install, Uninstall, Activate, and Deactivate the app on a connected development device.

Each one of these scripts simply sources `./aos.sh`, which in turn sources the SDK environment-setup script, and then either runs `make`
(in the case of `make_app`), or runs the identically named tool that comes with the SDK (`aospkg`, `aossignpkg`, `aosapp`).

If you installed the SDK in the default location, or alternatively in `/opt/tools/aos-esr1k-x86_64`, then `aos.sh`
will find it automatically.  Otherwise, you will need to set an **AOS_SDK_ROOT** environment variable to the location of
your SDK installation directory.

Example:
```sh
$ export AOS_SDK_ROOT=/home/bob/aossdk  # set AOS_SDK_ROOT to the non-standard location where you installed the sdk
$ ./make_app clean                      # clean build artifacts from previous build
rm -f aos_metering_app aos_metering_app.o aos_metering_app.d *.aos
$ ./make_app                            # build the app
arm-oe-linux-gnueabi-g++  -march=armv7-a -mfloat-abi=softfp -mfpu=neon --sysroot=/home/bob/aossdk/sysroots/armv7a-vfp-neon-oe-linux-gnueabi  -O2 -fexpensive-optimizations -frename-registers -fomit-frame-pointer -ftree-vectorize   -Wno-error=maybe-uninitialized -finline-functions -finline-limit=64  -fstack-protector-strong -pie -fpie -Wa,--noexecstack  -std=gnu++14 -MMD -MP -Ilib   -c -o aos_metering_app.o aos_metering_app.cpp
arm-oe-linux-gnueabi-g++  -march=armv7-a -mfloat-abi=softfp -mfpu=neon --sysroot=/home/bob/aossdk/sysroots/armv7a-vfp-neon-oe-linux-gnueabi -o aos_metering_app aos_metering_app.o -Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed -Wl,-z,relro,-z,now,-z,noexecstack -Wl,-rpath,/polnet/lib -lsdk_m2m -lsdk_aos -laosgen -lm2mgen -lxsd -lcoap -ldtls -lcommon -lappfw
$
```

### 4. Install the app on a device and activate it.

Connect a device to your host via USB.  Make sure you have the `adb` command installed.  On Ubuntu, it is in the **android-tools-adb** package.

Use the **aosapp** command to install and activate the app.  Run `./aosapp install` to install the app on the connected device.  Use the `-s <serialno>` option if there are multiple devices connected. If the application installs successfully, it can then be activated with `./aosapp activate`.

Run `./aosapp -h` to see the list of options.

## Common commands ##

### Starting and stopping the app ###

Use **aosapp** to control whether the app is activated on the device, meaning it will run automatically.

- `./aosapp activate` - Enable the app so it is always running.
- `./aosapp deactivate` - Disable the app so it does not run.

To simply stop, start, or restart an app, use **prctl** from a device shell.

- `prctl stop <appname>` - Stop the app
- `prctl start <appname>` - Start the app
- `prctl restart <appname>` - Restart the app
- `prctl status <appname>` - Check the status of the app

```sh
/ $ prctl stop aos_metering_app
aos_metering_app   stopping     [ok]
/ $
/ $ prctl start aos_metering_app
aos_metering_app   running      [ok] (last killed by signal: 9)
/ $
/ $ prctl restart aos_metering_app
aos_metering_app   want-running [ok] (last killed by signal: 9)
/ $
/ $ prctl status aos_metering_app
aos_metering_app   running      [ok] (last killed by signal: 9)
```

When stopping the app, if it does not exit in response to a SIGTERM sent by the supervisor process, it will be terminated after a few seconds with SIGKILL (9).  **prctl** reports this fact in the status as "(last killed by signal: 9)".

### Updating the app ###

Until further notice, the app can only be updated by first uninstalling it, and then installing the new version.

```sh
./aosapp uninstall
./aosapp install
./aosapp activate
```

### Accessing a device shell ###

Use the **adb** command to access a device.  Running `adb shell` will drop you into a shell as the **apps** user.  Common linux commands can be run from this shell.

If multiple devices are attached, use the `-s` option to select a device by its serial number.  `adb devices` will list all the connected devices.

If you specify a command and arguments after `adb shell`, adb will run that command in a shell.
 
Example:
```sh
$ adb shell cat /etc/issue
aos esr1k-lgausmeter 6.0.6g 6.0.6-R-20210506:061638-g55dc5d7
$
```

### Reading the log ###

From a device shell, use the **logread** command to read the system log.  Use **grep** for filtering.

Example:
```sh
$ adb shell logread |grep aos_metering |tail -n 10
May 13 19:56:31 aos daemon.notice aos_metering_app-sv: containerizing /data/polnet/apps/aos_metering_app/aos_metering_app
May 13 19:56:31 aos daemon.notice aos_metering_app-sv: chdir("/home/apps")
May 13 19:56:31 aos daemon.notice aos_metering_app-sv: forking /data/polnet/apps/aos_metering_app/aos_metering_app
May 14 02:56:32 aos user.warn aos_metering_ap: [1] m2m.AppEntity.Impl_.initialize (163) - app_config.json does not exist
May 14 02:56:32 aos user.info aos_metering_ap: [1] main (83) - activating
May 14 02:56:32 aos user.info aos_metering_ap: [1] m2m.AppEntity.Impl_.startRegistration (369) - AE registration request
May 14 02:56:32 aos user.info aos_metering_ap: [3] m2m.AppEntity.Impl_.handleRegisterAEResponse (451) - AE Registration complete.  AE-ID = Cmetersvc-smpl
May 14 02:56:32 aos user.info aos_metering_ap: [1] main (98) - activated
May 14 02:56:32 aos user.info aos_metering_ap: [1] create_subscription (149) - subscription: CREATED
May 14 02:56:32 aos user.info aos_metering_ap: [1] create_meter_read_policy (184) - policy creation: CREATED
$
```

### Accessing the application environment. ###

Running `./aosapp shell` will open a shell in an environment identically to the one the application runs in.

Example:
```sh
 ./aosapp -s 87e76c0 shell
/home/apps # ls -l
ls -l
-rwxr-xr-x    1 aos      root        193456 May 14 03:15 aos_metering_app
-rw-r-----    1 root     root           165 May 14 03:16 app_config.json
-rw-r--r--    1 aos      root           146 May 14 03:15 env
-rw-r--r--    1 aos      root           291 May 14 03:15 manifest.json
-rw-r-----    1 root     root           343 May 14 03:24 meter_data.txt
-rw-r--r--    1 aos      root           138 May 14 03:15 service.conf
/home/apps # 
```

