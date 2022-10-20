
AOS_NATIVE_CONFIGURED=0
if [ -z "${AOS_NATIVE_OUTPUT_DIR}" ]; then
	echo "AOS_NATIVE_OUTPUT_DIR is not defined"
	echo "set AOS_NATIVE_OUTPUT_DIR to the output directory of your native build"
elif [ ! -d "${AOS_NATIVE_OUTPUT_DIR}" ]; then
	echo "AOS_NATIVE_OUTPUT_DIR ${AOS_NATIVE_OUTPUT_DIR} does not exist"
else
	AOS_NATIVE_CONFIGURED=1
	OUTPUT_DIR=${AOS_NATIVE_OUTPUT_DIR}
	export STAGING_DIR="${OUTPUT_DIR}/staging"
	export LDFLAGS="-L${STAGING_DIR}/lib -L${OUTPUT_DIR}/target/lib -Wl,-rpath-link,${OUTPUT_DIR}/target/lib"
	export CPPFLAGS="-I${STAGING_DIR}/usr/include/sdk -I${STAGING_DIR}/usr/include/policynetm2m -DAOS_DEVICE_BUILD=1"
	export LIBS="-lssl -lcrypto"
fi

