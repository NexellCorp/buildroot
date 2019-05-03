################################################################################
#
# nx_vip_test
#
################################################################################

NX_VIP_TEST_VERSION = 0.0.1
NX_VIP_TEST_SITE_METHOD = local
NX_VIP_TEST_TARGET_NAME = vip_test

ifndef BR2_PACKAGE_NX_VIP_TEST_LOCAL_PATH
NX_VIP_TEST_SITE = ../library/nx_video_api_test
else
NX_VIP_TEST_SITE = $(BR2_PACKAGE_NX_VIP_TEST_LOCAL_PATH)
endif

NX_VIP_TEST_DEPENDENCIES = nx-drm-allocator nx-video-api nx-v4l2

# autotools, so we have to do it manually instead of
# setting NX_VIP_TEST_AUTORECONF = YES
define NX_VIP_TEST_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
	cd $(@D) && make distclean;
endef

NX_VIP_TEST_PRE_CONFIGURE_HOOKS = NX_VIP_TEST_RUN_AUTOGEN

NX_VIP_TEST_CONF_OPTS = --prefix=$(STAGING_DIR)/usr

define NX_VIP_TEST_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr DESTDIR=$(STAGING_DIR) \
		-C $(@D) install
endef

define NX_VIP_TEST_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr DESTDIR=$(TARGET_DIR) \
		-C $(@D) install
endef

$(eval $(autotools-package))
