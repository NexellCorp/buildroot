################################################################################
#
# disk_test
#
################################################################################

DISK_TEST_TARGET_NAME = disk_test
DISK_TEST_VERSION = 0.0.1
DISK_TEST_SITE_METHOD = local

ifndef BR2_PACKAGE_DISK_TEST_LOCAL_PATH
DISK_TEST_SITE = ../apps/disk_test
else
DISK_TEST_SITE = $(BR2_PACKAGE_DISK_TEST_LOCAL_PATH)
endif

# autotools, so we have to do it manually instead of
# setting DISK_TEST_AUTORECONF = YES
define DISK_TEST_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
	cd $(@D) && make distclean;
endef

DISK_TEST_PRE_CONFIGURE_HOOKS = DISK_TEST_RUN_AUTOGEN

$(eval $(autotools-package))
