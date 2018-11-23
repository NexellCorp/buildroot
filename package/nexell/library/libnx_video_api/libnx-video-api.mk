################################################################################
#
# libnx-video-api
#
################################################################################

LIBNX_VIDEO_API_VERSION = 0.0.1
LIBNX_VIDEO_API_SITE = package/nexell/library/libnx_video_api
LIBNX_VIDEO_API_SITE_METHOD = local
LIBNX_VIDEO_API_DEPENDENCIES = libdrm libnx_video_alloc
LIBNX_VIDEO_API_INSTALL_STAGING = YES

# we're patching configure.in, but package cannot autoreconf with our version of
# autotools, so we have to do it manually instead of setting LIBNX_VIDEO_API_AUTORECONF = YES
define LIBNX_VIDEO_API_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
endef

LIBNX_VIDEO_API_PRE_CONFIGURE_HOOKS = LIBNX_VIDEO_API_RUN_AUTOGEN

LIBNX_VIDEO_API_CFLAGS = $(TARGET_CFLAGS)
LIBNX_VIDEO_API_CFLAGS += -I$(STAGING_DIR)/usr/include/libdrm

define LIBNX_VIDEO_API_CONFIGURE_CMDS
	(cd $(@D); $(TARGET_CONFIGURE_OPTS) ./configure $(LIBNX_VIDEO_API_CONF_OPTS))
endef

LIBNX_VIDEO_API_CONF_OPTS = \
	CFLAGS="$(LIBNX_VIDEO_API_CFLAGS)" \
	--host=$(GNU_TARGET_NAME)

define LIBNX_VIDEO_API_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)
endef

define LIBNX_VIDEO_API_REMOVE_INSTALL_TARGET
	rm -fr $(TARGET_DIR)/usr/include
endef

LIBNX_VIDEO_API_POST_INSTALL_TARGET_HOOKS = LIBNX_VIDEO_API_REMOVE_INSTALL_TARGET

define LIBNX_VIDEO_API_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr -C $(@D) install DESTDIR=$(TARGET_DIR)
endef

define LIBNX_VIDEO_API_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr -C $(@D) install DESTDIR=$(STAGING_DIR)
endef

$(eval $(autotools-package))
$(eval $(host-autotools-package))
