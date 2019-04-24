################################################################################
#
# kernel-headers
#
################################################################################

KERNEL_HEADERS_PATH_METHOD = local
KERNEL_HEADERS_INSTALL_STAGING = YES

ifndef BR2_PACKAGE_KERNEL_HEADERS_LOCAL_PATH
KERNEL_HEADERS_PATH = ../library/kernel-headers
else
KERNEL_HEADERS_PATH = $(BR2_PACKAGE_KERNEL_HEADERS_LOCAL_PATH)
endif 

define KERNEL_HEADERS_BUILD_CMDS                                                
endef

define KERNEL_HEADERS_INSTALL_TARGET_CMDS
        mkdir -p $(STAGING_DIR)/usr/include/nexell;
	$(INSTALL) -m 0644 -D $(KERNEL_HEADERS_PATH)/include/uapi/drm/nexell_drm.h \
		$(STAGING_DIR)/usr/include/nexell;
endef 

$(eval $(generic-package))
