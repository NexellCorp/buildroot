################################################################################
#
# nx-video-api
#
################################################################################

NX_VIDEO_API_VERSION = 0.0.1
NX_VIDEO_API_SITE_METHOD = local
NX_VIDEO_API_INSTALL_STAGING = YES

ifndef BR2_PACKAGE_NX_VIDEO_API_LOCAL_PATH
NX_VIDEO_API_SITE = ../library/nx-video-api
else
NX_VIDEO_API_SITE = $(BR2_PACKAGE_NX_VIDEO_API_LOCAL_PATH)
endif

NX_VIDEO_API_DEPENDENCIES = libdrm

# autotools, so we have to do it manually instead of
# setting NX_VIDEO_API_AUTORECONF = YES
define NX_VIDEO_API_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh;
	cd $(@D) && make distclean;
endef

NX_VIDEO_API_PRE_CONFIGURE_HOOKS = NX_VIDEO_API_RUN_AUTOGEN

NX_VIDEO_API_CONF_OPTS = --prefix=$(STAGING_DIR)/usr

define NX_VIDEO_API_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr DESTDIR=$(STAGING_DIR) \
		-C $(@D) install
endef

define NX_VIDEO_API_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr DESTDIR=$(TARGET_DIR) \
		-C $(@D) install
endef

$(eval $(autotools-package))
$(eval $(host-autotools-package))
