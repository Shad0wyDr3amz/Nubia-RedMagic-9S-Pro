# This is the script used to build ufsd.ko and jnl.ko driver modules
# Required variables for the script should either be defined in 'build-ufsd.conf' file

if [ -f ./build-ufsd.conf ]; then
	. ./build-ufsd.conf
else
	echo "Cannot proceed without 'build-ufsd.conf' file" >&2
	exit 1
fi

if [ -f ./build-ufsd.static ]; then
	. ./build-ufsd.static
else
	echo "Cannot proceed without 'build-ufsd.static' file" >&2
	exit 1
fi

for varname in KERNEL_SOURCE_PATH KERNEL_BUILD_PATH BUILD_DEBUG_VERSION; do
	eval "if [ -z \"\$$varname\" ]; then echo 'Variable $varname is not set - insert correct value in build-ufsd.conf file' >&2; exit 1; fi"
done

if [ -n "${CROSSCOMPILE}" ] && [ -z "${COMPILER_PATH}" ]; then
	echo "COMPILER_PATH cannot be empty if cross-compilation is presumed in build-ufsd.static file" >&2
	exit 1
fi

if [ "${BUILD_DEBUG_VERSION}" = 1 ]; then
	CONFIGURE_DEBUG="--enable-ufsd-debug"
elif [ ! "${BUILD_DEBUG_VERSION}" = 0 ]; then
	echo "BUILD_DEBUG_VERSION variable shold be set to either 1 or 0" >&2
	exit 1
fi

build_ufsd_module()
{
	echo "==== start of build_ufsd_module"
	echo "==== kernel version:"
	grep -m1 -A3 VERSION "${KERNEL_SOURCE_PATH}/Makefile"
	echo "==== build-ufsd.conf contents:"
	cat build-ufsd.conf
	echo "==== build-ufsd.static contents:"
	cat build-ufsd.static
	echo "==== pwd = $PWD"

	echo  "==== UFSD: make clean"
	make clean 2>/dev/null

	echo "==== UFSD: removing old files"
	echo "rm -f config.log config.status Makefile modules.order Module.symvers .tmp_versions .*.cmd *.mod.c *.o *.ko ifslinux/libufsd.bin ifslinux/*.o ifslinux/.*.o.cmd"
	rm -f config.log config.status Makefile modules.order Module.symvers .tmp_versions .*.cmd *.mod.c *.o *.ko ifslinux/libufsd.bin ifslinux/*.o ifslinux/.*.o.cmd

	echo "==== UFSD: configure"
	echo "./configure --with-ks-dir=${KERNEL_SOURCE_PATH} --with-kb-dir=${KERNEL_BUILD_PATH} --target=${TARGET} ${CONFIGURE_OPTIONS} ${CONFIGURE_HOST} ${CONFIGURE_DEBUG}"
	./configure --with-ks-dir=${KERNEL_SOURCE_PATH} --with-kb-dir=${KERNEL_BUILD_PATH} --target=${TARGET} ${CONFIGURE_OPTIONS} ${CONFIGURE_HOST} ${CONFIGURE_DEBUG} || exit 1

	echo "==== UFSD: make driver"
	make driver || exit 1

	return 0
}

if [ -n "${UFSD_NO_BUILD_LOG}" ]; then
	build_ufsd_module
else
	build_ufsd_module 2>&1 | tee build-ufsd.log
fi

