################################################################################
#
# nx-drm-allocator
#
################################################################################

NX_DRM_ALLOCATOR_VERSION = 0.0.1
NX_DRM_ALLOCATOR_SITE_METHOD = local
NX_DRM_ALLOCATOR_INSTALL_STAGING = YES

NX_DRM_ALLOCATOR_SITE = package/nexell/library/nx-drm-allocator

NX_DRM_ALLOCATOR_DEPENDENCIES = libdrm

# autotools, so we have to do it manually instead of
# setting NX_DRM_ALLOCATOR_AUTORECONF = YES
define NX_DRM_ALLOCATOR_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh;
	cd $(@D) && make distclean;
endef

NX_DRM_ALLOCATOR_PRE_CONFIGURE_HOOKS = NX_DRM_ALLOCATOR_RUN_AUTOGEN

NX_DRM_ALLOCATOR_CONF_OPTS = --prefix=$(STAGING_DIR)/usr

define NX_DRM_ALLOCATOR_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr DESTDIR=$(STAGING_DIR) \
		-C $(@D) install
endef

define NX_DRM_ALLOCATOR_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr DESTDIR=$(TARGET_DIR) \
		-C $(@D) install
endef

$(eval $(autotools-package))
$(eval $(host-autotools-package))
