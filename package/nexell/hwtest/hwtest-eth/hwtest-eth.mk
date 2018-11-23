################################################################################
#
# ethernet test application
#
################################################################################

HWTEST_ETH_VERSION = 0.0.1
HWTEST_ETH_SITE = package/nexell/hwtest/hwtest-eth
HWTEST_ETH_SITE_METHOD = local
HWTEST_ETH_TARGET_SERVER_NAME = server 
HWTEST_ETH_TARGET_CLIENT_NAME = client

define HWTEST_ETH_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)
endef

define HWTEST_ETH_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755  -D $(@D)/$(HWTEST_ETH_TARGET_SERVER_NAME) $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755  -D $(@D)/$(HWTEST_ETH_TARGET_CLIENT_NAME) $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
