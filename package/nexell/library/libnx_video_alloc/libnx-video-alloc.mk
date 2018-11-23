################################################################################
#
# libnx-video-alloc
#
################################################################################

LIBNX_VIDEO_ALLOC_VERSION = 0.0.1
LIBNX_VIDEO_ALLOC_SITE = package/nexell/library/libnx_video_alloc
LIBNX_VIDEO_ALLOC_SITE_METHOD = local
LIBNX_VIDEO_ALLOC_DEPENDENCIES = libdrm
LIBNX_VIDEO_ALLOC_INSTALL_STAGING = YES

# we're patching configure.in, but package cannot autoreconf with our version of
# autotools, so we have to do it manually instead of setting LIBNX_VIDEO_ALLOC_AUTORECONF = YES
define LIBNX_VIDEO_ALLOC_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
endef

LIBNX_VIDEO_ALLOC_PRE_CONFIGURE_HOOKS = LIBNX_VIDEO_ALLOC_RUN_AUTOGEN

LIBNX_VIDEO_ALLOC_CFLAGS = $(TARGET_CFLAGS)
LIBNX_VIDEO_ALLOC_CFLAGS += -I$(STAGING_DIR)/usr/include/libdrm

define LIBNX_VIDEO_ALLOC_CONFIGURE_CMDS
	(cd $(@D); $(TARGET_CONFIGURE_OPTS) ./configure $(LIBNX_VIDEO_ALLOC_CONF_OPTS))
endef

LIBNX_VIDEO_ALLOC_CONF_OPTS = \
	CFLAGS="$(LIBNX_VIDEO_ALLOC_CFLAGS)" \
	--host=$(GNU_TARGET_NAME)

define LIBNX_VIDEO_ALLOC_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)
endef

define LIBNX_VIDEO_ALLOC_REMOVE_INSTALL_TARGET
	rm -fr $(TARGET_DIR)/usr/include
endef

LIBNX_VIDEO_ALLOC_POST_INSTALL_TARGET_HOOKS = LIBNX_VIDEO_ALLOC_REMOVE_INSTALL_TARGET

define LIBNX_VIDEO_ALLOC_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr -C $(@D) install DESTDIR=$(TARGET_DIR)
endef

define LIBNX_VIDEO_ALLOC_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr -C $(@D) install DESTDIR=$(STAGING_DIR)
endef

$(eval $(autotools-package))
$(eval $(host-autotools-package))
