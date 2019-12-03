################################################################################
#
# i2c_test
#
################################################################################

I2C_TEST_TARGET_NAME = i2c_test
I2C_TEST_VERSION = 0.0.1
I2C_TEST_SITE_METHOD = local

ifndef BR2_PACKAGE_I2C_TEST_LOCAL_PATH
I2C_TEST_SITE = ../apps/i2c_test
else
I2C_TEST_SITE = $(BR2_PACKAGE_I2C_TEST_LOCAL_PATH)
endif

# autotools, so we have to do it manually instead of
# setting I2C_TEST_AUTORECONF = YES
define I2C_TEST_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
	cd $(@D) && make distclean;
endef

I2C_TEST_PRE_CONFIGURE_HOOKS = I2C_TEST_RUN_AUTOGEN

$(eval $(autotools-package))
