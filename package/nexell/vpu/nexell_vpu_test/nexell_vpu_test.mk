################################################################################
#
# nexell_vpu_test
#
################################################################################

NEXELL_VPU_TEST_VERSION = 0.0.1
NEXELL_VPU_TEST_SITE = package/nexell/vpu/nexell_vpu_test
NEXELL_VPU_TEST_SITE_METHOD = local
NEXELL_VPU_TEST_DEPENDENCIES = libdrm libnx_video_alloc libnx_video_api
NEXELL_VPU_TEST_TARGET_DATA = data

# we're patching configure.in, but package cannot autoreconf with our version of
# autotools, so we have to do it manually instead of setting NEXELL_VPU_TEST_AUTORECONF = YES
define NEXELL_VPU_TEST_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
endef

NEXELL_VPU_TEST_PRE_CONFIGURE_HOOKS = NEXELL_VPU_TEST_RUN_AUTOGEN

NEXELL_VPU_TEST_CFLAGS = $(TARGET_CFLAGS)
NEXELL_VPU_TEST_CFLAGS += -I$(STAGING_DIR)/usr/include/libdrm

define NEXELL_VPU_TEST_CONFIGURE_CMDS
	(cd $(@D); $(TARGET_CONFIGURE_OPTS) ./configure $(NEXELL_VPU_TEST_CONF_OPTS))
endef

NEXELL_VPU_TEST_CONF_OPTS = \
	CFLAGS="$(NEXELL_VPU_TEST_CFLAGS)" \
	--host=$(GNU_TARGET_NAME)

define NEXELL_VPU_TEST_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)
endef

define NEXELL_VPU_TEST_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr -C $(@D) install DESTDIR=$(TARGET_DIR)
	cp -a $(@D)/src/$(NEXELL_VPU_TEST_TARGET_DATA) $(TARGET_DIR)
endef

$(eval $(autotools-package))
$(eval $(host-autotools-package))
