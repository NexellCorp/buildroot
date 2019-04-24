################################################################################
#
# libdirectfb-nexell
#
################################################################################

LIBDIRECTFB_NEXELL_VERSION = 0.0.1
LIBDIRECTFB_NEXELL_SITE_METHOD = local
LIBDIRECTFB_NEXELL_INSTALL_STAGING = YES

ifndef BR2_PACKAGE_LIBDIRECTFB_NEXELL_LOCAL_PATH
LIBDIRECTFB_NEXELL_SITE = ../library/libdirectfb-nexell
else
LIBDIRECTFB_NEXELL_SITE = $(BR2_PACKAGE_LIBDIRECTFB_NEXELL_LOCAL_PATH)
endif

LIBDIRECTFB_NEXELL_DEPENDENCIES = libdrm directfb

# autotools, so we have to do it manually instead of
# setting LIBDIRECTFB_NEXELL_AUTORECONF = YES
define LIBDIRECTFB_NEXELL_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh;
	cd $(@D) && make distclean;
endef

LIBDIRECTFB_NEXELL_PRE_CONFIGURE_HOOKS = LIBDIRECTFB_NEXELL_RUN_AUTOGEN

LIBDIRECTFB_NEXELL_CONF_OPTS = --prefix=$(STAGING_DIR)/usr

define LIBDIRECTFB_NEXELL_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) \
		prefix=/usr MODULEDIR=$(STAGING_DIR)/usr/lib/directfb-1.7-7 \
		-C $(@D) install
endef

define LIBDIRECTFB_NEXELL_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE)  \
		prefix=/usr MODULEDIR=$(TARGET_DIR)/usr/lib/directfb-1.7-7 \
		-C $(@D) install
endef

$(eval $(autotools-package))
$(eval $(host-autotools-package))
