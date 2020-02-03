################################################################################
#
# VIDEO_API_TEST
#
################################################################################

VIDEO_API_TEST_VERSION = 0.0.1
VIDEO_API_TEST_SITE_METHOD = local
VIDEO_API_TEST_TARGET_NAME = video-api-test

VIDEO_API_TEST_SITE = package/nexell/apps/video-api-test
VIDEO_API_TEST_DEPENDENCIES = ffmpeg nx-v4l2 nx-video-api nx-drm-allocator

# autotools, so we have to do it manually instead of
# setting VIDEO_API_TEST_AUTORECONF = YES
define VIDEO_API_TEST_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
	cd $(@D) && make distclean;
endef

VIDEO_API_TEST_PRE_CONFIGURE_HOOKS = VIDEO_API_TEST_RUN_AUTOGEN

VIDEO_API_TEST_CONF_OPTS = --prefix=$(STAGING_DIR)/usr

define VIDEO_API_TEST_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr DESTDIR=$(STAGING_DIR) \
		-C $(@D) install
endef

define VIDEO_API_TEST_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr DESTDIR=$(TARGET_DIR) \
		-C $(@D) install
endef

$(eval $(autotools-package))
