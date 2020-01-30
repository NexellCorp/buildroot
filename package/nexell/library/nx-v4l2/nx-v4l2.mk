################################################################################
#
# nx-v4l2
#
################################################################################

NX_V4L2_VERSION = 0.0.1
NX_V4L2_SITE_METHOD = local
NX_V4L2_INSTALL_STAGING = YES

NX_V4L2_SITE = package/nexell/library/nx-v4l2

NX_V4L2_DEPENDENCIES = libdrm

# autotools, so we have to do it manually instead of
# setting NX_V4L2_AUTORECONF = YES
define NX_V4L2_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh;
	cd $(@D) && make distclean;
endef

NX_V4L2_PRE_CONFIGURE_HOOKS = NX_V4L2_RUN_AUTOGEN

NX_V4L2_CONF_OPTS = --prefix=$(STAGING_DIR)/usr

define NX_V4L2_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr DESTDIR=$(STAGING_DIR) \
		-C $(@D) install
endef

define NX_V4L2_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) prefix=/usr DESTDIR=$(TARGET_DIR) \
		-C $(@D) install
endef

$(eval $(autotools-package))
$(eval $(host-autotools-package))
