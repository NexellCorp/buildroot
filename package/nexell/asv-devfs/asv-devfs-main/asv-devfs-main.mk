################################################################################
#
# devfs main application
#
################################################################################

ASV_DEVFS_MAIN_VERSION = 0.0.1
ASV_DEVFS_MAIN_SITE = package/nexell/asv-devfs/asv-devfs-main
ASV_DEVFS_MAIN_SITE_METHOD = local
ASV_DEVFS_MAIN_TARGET_NAME = devfreq
ASV_DEVFS_MAIN_TARGET_INIT = S60devfreq

ifeq ($(BR2_PACKAGE_NXP3220),y)
ASV_DEVFS_MAIN_CONFIG_NAME = nxp3220_asv_devfreq.js
endif

define ASV_DEVFS_MAIN_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) CXX=$(TARGET_CXX) -C $(@D)
endef

define ASV_DEVFS_MAIN_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755  -D $(@D)/$(ASV_DEVFS_MAIN_TARGET_NAME) $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755  -D $(@D)/$(ASV_DEVFS_MAIN_TARGET_INIT) $(TARGET_DIR)/etc/init.d/
	mkdir -p $(TARGET_DIR)/data
	if [ ! -z "$(ASV_DEVFS_MAIN_CONFIG_NAME)" ]; then \
		$(INSTALL) -m 0644  -D $(@D)/$(ASV_DEVFS_MAIN_CONFIG_NAME) $(TARGET_DIR)/data/; \
	fi
endef

$(eval $(generic-package))
