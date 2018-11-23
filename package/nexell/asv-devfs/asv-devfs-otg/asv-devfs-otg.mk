################################################################################
#
# devfs otg ums
#
################################################################################

ASV_DEVFS_OTG_VERSION = 0.0.1
ASV_DEVFS_OTG_SITE = package/nexell/asv-devfs/asv-devfs-otg
ASV_DEVFS_OTG_SITE_METHOD = local
ASV_DEVFS_OTG_TARGET_UMS = ums.configfs.sh
ASV_DEVFS_OTG_TARGET_ADB = adb.configfs.sh

define ASV_DEVFS_OTG_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755  -D $(@D)/$(ASV_DEVFS_OTG_TARGET_UMS) $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755  -D $(@D)/$(ASV_DEVFS_OTG_TARGET_ADB) $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
