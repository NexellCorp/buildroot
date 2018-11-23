################################################################################
#
# disk test application
#
################################################################################

HWTEST_DISK_VERSION = 0.0.1
HWTEST_DISK_SITE = package/nexell/hwtest/hwtest-disk
HWTEST_DISK_SITE_METHOD = local
HWTEST_DISK_TARGET_NAME = disk_test

define HWTEST_DISK_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)
endef

define HWTEST_DISK_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755  -D $(@D)/$(HWTEST_DISK_TARGET_NAME) $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
