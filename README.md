# AOS Device App SDK Sample Application


## Quick Start Guide ##

### 1. Install the SDK.

Download the SDK installer and run it.  It will prompt you for an install location.

```sh
	aos-perf SDK installer version v6.0.6
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

### 2. Clone and the sample app from [github](https://github.com/aetheros/aos-metering-app.git).

```sh
	git clone https://github.com/aetheros/aos-metering-app.git
	cd aos-metering-app
```

### 3. Build the sample app using the SDK toolchain.

To use the SDK, you can source its environment file which sets up an environment for building against the SDK.
Then run `make` to build the app.

```sh
	. /opt/tools/aos-esr1k-x86_64/environment-setup-armv7a-vfp-neon-oe-linux-gnueabi
	make
```

Alternatively, you can use the convenience wrapper scripts provided in the aos-metering-app repo:

- `./make_app` - Build `aos_metering_app`.
- `./aospkg` - Create a unsigned AOS package.
- `./aossignpkg` - Create a signed AOS package.
- `./aosapp` - Install, Uninstall, Activate, and Deactivate the app on a connected development device.

Each one of these scripts simply sources **`./aos.sh`**, which in turn sources the SDK environment-setup script, and then either runs `make`
(in the case of `make_app`), or runs the identically named tool that comes with the SDK (`aospkg`, `aossignpkg`, `aosapp`).

If you installed the SDK in the default location, or alternatively in `/opt/tools/aos-esr1k-x86_64`, then `aos.sh`
will find it automatically.  Otherwise, you will need to set an **`AOS_SDK_ROOT`** environment variable to the location of
your SDK installation directory.

Example:
```sh
    $ export AOS_SDK_ROOT=/home/bob/aossdk  # set AOS_SDK_ROOT to the non-standard location where you installed the sdk
    $ ./make_app clean                      # clean build artifacts from previous build
    rm -f aos_metering_app aos_metering_app.o aos_metering_app.d *.aos
    $ ./make_app                            # build the app
arm-oe-linux-gnueabi-g++  -march=armv7-a -mfloat-abi=softfp -mfpu=neon --sysroot=/home/bob/aossdk/sysroots/armv7a-vfp-neon-oe-linux-gnueabi  -O2 -fexpensive-optimizations -frename-registers -fomit-frame-pointer -ftree-vectorize   -Wno-error=maybe-uninitialized -finline-functions -finline-limit=64  -fstack-protector-strong -pie -fpie -Wa,--noexecstack  -std=gnu++14 -std=gnu++14 -MMD -MP -Ilib   -c -o aos_metering_app.o aos_metering_app.cpp
arm-oe-linux-gnueabi-g++  -march=armv7-a -mfloat-abi=softfp -mfpu=neon --sysroot=/home/bob/aossdk/sysroots/armv7a-vfp-neon-oe-linux-gnueabi -o aos_metering_app aos_metering_app.o -Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed -Wl,-z,relro,-z,now,-z,noexecstack -Wl,-rpath,/polnet/lib -lsdk_m2m -lsdk_aos -laosgen -lm2mgen -lxsd -lcoap -ldtls -lcommon -lappfw
    $
```

### 4. Install the app on a device and activate it.

Connect a device to your host via USB.  Make sure you have the `adb` command installed.  On Ubuntu, it is in the **android-tools-adb** package.

Use the **`aosapp`** command to install and activate the app.  Run **`./aosapp install`** to install the app on the connected device.  Use the `-s <serialno>` option if there are multiple devices connected. If the application installs successfully, it can then be activated with **`./aosapp activate`**.

Run **`./aosapp -h`** to see the list of options.
