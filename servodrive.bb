DESCRIPTION = "Beagleboard RC servo drive kernel module"
HOMEPAGE = "http://www.github.com/tallakt/servosource"
SECTION = "kernel/modules"
PRIORITY = "optional"
LICENSE = "Dual BSD/GPL"
RDEPENDS = "kernel (${KERNEL_VERSION})"
DEPENDS = "virtual/kernel"
PR = "r0"

SRC_URI = " \
  file://servodrive.c \
  file://Makefile \
  file://mkservodev \
  "

S = "${WORKDIR}"

inherit module

do_compile () {
  unset CFLAGS CPPFLAGS CXXFLAGS LDFLAGS CC LD CPP
  oe_runmake 'MODPATH="${D}${base_libdir}/modules/${KERNEL_VERSION}/kernel/drivers/ecu" ' \
    'KERNEL_SOURCE="${STAGING_KERNEL_DIR}" ' \
    'KDIR="${STAGING_KERNEL_DIR}"' \
    'KERNEL_VERSION="${KERNEL_VERSION}" ' \
    'CC="${KERNEL_CC}" ' \
    'LD="${KERNEL_LD}" '

}

do_install () {
  install -d ${D}${base_libdir}/modules/${KERNEL_VERSION}/kernel/drivers/extra
  install -m 0644 ${S}/servodrive*${KERNEL_OBJECT_SUFFIX} ${D}${base_libdir}/modules/${KERNEL_VERSION}/kernel/drivers/extra
}



