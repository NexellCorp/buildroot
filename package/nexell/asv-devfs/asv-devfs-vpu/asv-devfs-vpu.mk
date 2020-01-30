################################################################################
#
# devfs vpu application
#
################################################################################

ASV_DEVFS_VPU_VERSION = 0.0.1
ASV_DEVFS_VPU_SITE = package/nexell/asv-devfs/asv-devfs-vpu
ASV_DEVFS_VPU_SITE_METHOD = local
ASV_DEVFS_VPU_DEPENDENCIES = libdrm nx-video-api
ASV_DEVFS_VPU_TARGET_DATA = data

# we're patching configure.in, but package cannot autoreconf with our version of
# autotools, so we have to do it manually instead of setting ASV_DEVFS_VPU_AUTORECONF = YES
define ASV_DEVFS_VPU_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
endef

ASV_DEVFS_VPU_PRE_CONFIGURE_HOOKS = ASV_DEVFS_VPU_RUN_AUTOGEN

ASV_DEVFS_VPU_CFLAGS = $(TARGET_CFLAGS)
ASV_DEVFS_VPU_CFLAGS += -I$(STAGING_DIR)/usr/include/libdrm

define ASV_DEVFS_VPU_CONFIGURE_CMDS
	(cd $(@D); $(TARGET_CONFIGURE_OPTS) ./configure $(ASV_DEVFS_VPU_CONF_OPTS))
endef

ASV_DEVFS_VPU_CONF_OPTS = \
	CFLAGS="$(ASV_DEVFS_VPU_CFLAGS)" \
	--host=$(GNU_TARGET_NAME)

define ASV_DEVFS_VPU_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)
endef

define ASV_DEVFS_VPU_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr -C $(@D) install DESTDIR=$(TARGET_DIR)
	cp -a $(@D)/src/$(ASV_DEVFS_VPU_TARGET_DATA) $(TARGET_DIR)
endef

$(eval $(autotools-package))
$(eval $(host-autotools-package))
