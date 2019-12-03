################################################################################
#
# sleep_rtc_alarm
#
################################################################################

SLEEP_RTC_ALARM_TARGET_NAME = sleep_rtc_alarm
SLEEP_RTC_ALARM_VERSION = 0.0.1
SLEEP_RTC_ALARM_SITE_METHOD = local

ifndef BR2_PACKAGE_SLEEP_RTC_ALARM_LOCAL_PATH
SLEEP_RTC_ALARM_SITE = ../apps/sleep_rtc_alarm
else
SLEEP_RTC_ALARM_SITE = $(BR2_PACKAGE_SLEEP_RTC_ALARM_LOCAL_PATH)
endif

# autotools, so we have to do it manually instead of
# setting SLEEP_RTC_ALARM_AUTORECONF = YES
define SLEEP_RTC_ALARM_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
	cd $(@D) && make distclean;
endef

SLEEP_RTC_ALARM_PRE_CONFIGURE_HOOKS = SLEEP_RTC_ALARM_RUN_AUTOGEN

$(eval $(autotools-package))
