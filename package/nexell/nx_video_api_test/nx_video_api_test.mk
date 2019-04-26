################################################################################
#
# nx_video_api_test
#
################################################################################

NX_VIDEO_API_TEST_VERSION = 0.0.1
NX_VIDEO_API_TEST_SITE_METHOD = local
NX_VIDEO_API_TEST_TARGET_NAME = video_api_test

ifndef BR2_PACKAGE_NX_VIDEO_API_TEST_LOCAL_PATH
NX_VIDEO_API_TEST_SITE = ../library/nx_video_api_test
else
NX_VIDEO_API_TEST_SITE = $(BR2_PACKAGE_NX_VIDEO_API_TEST_LOCAL_PATH)
endif

NX_VIDEO_API_TEST_DEPENDENCIES = ffmpeg

# autotools, so we have to do it manually instead of
# setting NX_VIDEO_API_TEST_AUTORECONF = YES
define NX_VIDEO_API_TEST_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
	cd $(@D) && make distclean;
endef

NX_VIDEO_API_TEST_PRE_CONFIGURE_HOOKS = NX_VIDEO_API_TEST_RUN_AUTOGEN

NX_VIDEO_API_TEST_CONF_OPTS = --prefix=$(STAGING_DIR)/usr

define NX_VIDEO_API_TEST_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr DESTDIR=$(STAGING_DIR) \
		-C $(@D) install
endef

define NX_VIDEO_API_TEST_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr DESTDIR=$(TARGET_DIR) \
		-C $(@D) install
endef

$(eval $(autotools-package))
