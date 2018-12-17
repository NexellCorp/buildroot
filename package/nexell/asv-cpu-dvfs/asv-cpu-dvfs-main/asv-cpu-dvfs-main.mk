################################################################################
#
# cpu dvfs main application
#
################################################################################

ASV_CPU_DVFS_MAIN_VERSION = 0.0.1
ASV_CPU_DVFS_MAIN_SITE = package/nexell/asv-cpu-dvfs/asv-cpu-dvfs-main
ASV_CPU_DVFS_MAIN_SITE_METHOD = local
ASV_CPU_DVFS_MAIN_TARGET_NAME = cpufreq
ASV_CPU_DVFS_MAIN_TARGET_INIT = S61cpufreq
ASV_CPU_DVFS_MAIN_TARGET_SHELL = *.sh

define ASV_CPU_DVFS_MAIN_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) CXX=$(TARGET_CXX) -C $(@D)
endef

define ASV_CPU_DVFS_MAIN_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755  -D $(@D)/$(ASV_CPU_DVFS_MAIN_TARGET_NAME) $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755  -D $(@D)/$(ASV_CPU_DVFS_MAIN_TARGET_SHELL) $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755  -D $(@D)/$(ASV_CPU_DVFS_MAIN_TARGET_INIT) $(TARGET_DIR)/etc/init.d/
endef

$(eval $(generic-package))
