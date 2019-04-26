################################################################################
#
# hwio
#
################################################################################

HWIO_TARGET_NAME = hwio
HWIO_VERSION = 0.0.1
HWIO_SITE_METHOD = local

ifndef BR2_PACKAGE_HWIO_LOCAL_PATH
HWIO_SITE = ../library/hwio
else
HWIO_SITE = $(BR2_PACKAGE_HWIO_LOCAL_PATH)
endif

# autotools, so we have to do it manually instead of
# setting HWIO_AUTORECONF = YES
define HWIO_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
	cd $(@D) && make distclean;
endef

HWIO_PRE_CONFIGURE_HOOKS = HWIO_RUN_AUTOGEN

$(eval $(autotools-package))
