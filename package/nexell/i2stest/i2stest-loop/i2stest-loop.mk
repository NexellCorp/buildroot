################################################################################
#
# i2s loop test application
#
################################################################################

I2STEST_LOOP_VERSION = 0.0.1
I2STEST_LOOP_SITE = package/nexell/i2stest/i2stest-loop
I2STEST_LOOP_SITE_METHOD = local
I2STEST_LOOP_TARGET_NAME = i2s_test
I2STEST_LOOP_TARGET_DATA = data

define I2STEST_LOOP_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)
endef

define I2STEST_LOOP_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755  -D $(@D)/$(I2STEST_LOOP_TARGET_NAME) $(TARGET_DIR)/usr/bin/
	cp -a $(@D)/$(I2STEST_LOOP_TARGET_DATA) $(TARGET_DIR)
endef

$(eval $(generic-package))
