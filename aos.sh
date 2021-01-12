
if [ -f /opt/tools/aos-esr1k-x86_64/environment-setup-armv7a-vfp-neon-oe-linux-gnueabi ]; then
	. /opt/tools/aos-esr1k-x86_64/environment-setup-armv7a-vfp-neon-oe-linux-gnueabi
elif  [ -f /usr/local/aos-esr1k-x86_64/environment-setup-armv7a-vfp-neon-oe-linux-gnueabi ]; then
	. /usr/local/aos-esr1k-x86_64/environment-setup-armv7a-vfp-neon-oe-linux-gnueabi
else
	echo "Can't find the SDK directory"
	exit 1
fi

