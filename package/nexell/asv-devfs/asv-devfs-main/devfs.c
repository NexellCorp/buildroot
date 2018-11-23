/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 *
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <asm/unistd.h>

#define SYS_DEV_FREQ_GOV	"/sys/class/devfreq/devfreq0/governor"
#define SYS_DEV_FREQ_TARGET	"/sys/class/devfreq/devfreq0/userspace/set_freq"
#define SYS_DEV_FREQ_CURR	"/sys/class/devfreq/devfreq0/cur_freq"
#define	SYS_DEV_AVAIL_FREQ	"/sys/class/devfreq/devfreq0/available_frequencies"
#define	SYS_DEV_AVAIL_VOL	"/sys/class/devfreq/devfreq0/scaling_available_voltages"

#define	SYS_REGULATOR		"/sys/class/regulator"
#define	SYS_ID_RO		"/sys/devices/platform/cpu/ro"
#define	SYS_ID_IDS		"/sys/devices/platform/cpu/ids"
#define	SYS_ID_UUID		"/sys/devices/platform/cpu/uuid"
#define SYS_THERMAL		"/sys/class/thermal"

int sys_read(const char *file, char *buffer, int size)
{
	int fd, ret;

	if (access(file, F_OK) != 0) {
		fprintf(stderr, "Cannot access file: %s\n", file);
		return -errno;
	}

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Cannot open file: %s\n", file);
		return -errno;
	}

	ret = read(fd, buffer, size);
	if (ret < 0) {
		fprintf(stderr,
			"Failed, read (%s, %s)\n",
			file, buffer);
		close(fd);
		return -errno;
	}

	close(fd);

	return ret;
}

int sys_write(const char *file, const char *buffer, int size)
{
	int fd;

	if (access(file, F_OK) != 0) {
		fprintf(stderr, "Cannot access file: %s\n", file);
		return -errno;
	}

	fd = open(file, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Cannot open file: %s\n", file);
		return -errno;
	}

	if (write(fd, buffer, size) < 0) {
		fprintf(stderr,
			"Failed, write (%s, %s)\n",
			file, buffer);
		close(fd);
		return -errno;
	}

	close(fd);

	return 0;
}

int sys_dev_set_freq(long khz)
{
	char data[32] = { 0, };
	int ret;

	/*
	 * change governor to userspace
	 */
	ret = sys_write(SYS_DEV_FREQ_GOV, "userspace", strlen("userspace"));
	if (ret < 0)
		return -errno;

	/*
	 * change cpu frequency
	 */
	sprintf(data, "%ld", khz);
	if (sys_write(SYS_DEV_FREQ_TARGET, data, strlen(data)) < 0)
		return -errno;

	return 0;
}

unsigned long sys_dev_get_freq(void)
{
	char data[128];
	int ret;

	ret = sys_read(SYS_DEV_FREQ_CURR, data, sizeof(data));
	if (ret < 0)
		return ret;

	return strtol(data, NULL, 10);	 /* khz */
}

int sys_dev_avail_freq_num(void)
{
	char data[256] = { 0, };
	int no = 0, ret;
	char *s = data;

	ret = sys_read(SYS_DEV_AVAIL_FREQ, data, sizeof(data));
	if (ret < 0)
		return ret;

	while (s) {
		if (*s != '\n' && *s != ' ')
			no++;
		s = strchr(s, ' ');
		if (!s)
			break;
		s++;
	}

	return no;
}

int sys_dev_avail_freqs(long *khz, int size)
{
	char data[256] = { 0, };
	int i = 0, no = 0, ret;
	char *s = data;

	if (!khz)
		return -EINVAL;

	ret = sys_read(SYS_DEV_AVAIL_FREQ, data, sizeof(data));
	if (ret < 0)
		return ret;

	while (s) {
		if (*s != '\n' && *s != ' ')
			no++;
		s = strchr(s, ' ');
		if (!s)
			break;
		s++;
	}

	s = data;
	for (i = 0; i < size && i < no; i++, s++) {
		khz[i] = strtol(s, NULL, 10);
		s = strchr(s, ' ');
		if (!s)
			break;
	}
	return no;
}

int sys_dev_avail_volts(long *uV, int size)
{
	char data[256] = { 0, };
	int i = 0, no = 0, ret;
	char *s = data;

	if (!uV)
		return -EINVAL;

	ret = sys_read(SYS_DEV_AVAIL_VOL, data, sizeof(data));
	if (ret < 0)
		return ret;

	while (s) {
		if (*s != '\n' && *s != ' ')
			no++;
		s = strchr(s, ' ');
		if (!s)
			break;
		s++;
	}

	s = data;
	for (i = 0; i < size && i < no; i++, s++) {
		uV[i] = strtol(s, NULL, 10);
		s = strchr(s, ' ');
		if (!s)
			break;
	}

	return no;
}

int sys_vdd_set(int id, long uV)
{
	char path[128];
	char data[32];

	sprintf(path, "%s/regulator.%d/%s",
		SYS_REGULATOR, id, "microvolts");
	sprintf(data, "%ld", uV);

	return sys_write(path, data, strlen(data));
}

unsigned long sys_vdd_get(int id)
{
	char path[128];
	char data[128];
	int ret;

	sprintf(path, "%s/regulator.%d/%s",
		SYS_REGULATOR, id, "microvolts");

	ret = sys_read(path, data, sizeof(data));
	if (ret < 0)
		return ret;

	return strtol(data, NULL, 10);
}

int sys_temp_get(int ch, int *temp)
{
	char path[128];
	char data[128];
	int ret;

	sprintf(path, "%s/thermal_zone%d/%s",
		SYS_THERMAL, ch, "temp");

	ret = sys_read(path, data, sizeof(data));
	if (ret < 0)
		return ret;

	if (temp)
		*temp = strtol(data, NULL, 10);

	return 0;
}

int sys_uid_ids(unsigned int *ids, int size)
{
	char data[128];
	char *p, *sp;
	int ret, i;

	ret = sys_read(SYS_ID_IDS, data, sizeof(data));
	if (ret < 0)
		return ret;

	for (p = data, i = 0; ; p = NULL, i++) {
		p = strtok_r(p, ":", &sp);
		if (!p)
			break;

		if (i >= size)
			break;

		ids[i] = strtoul(p, NULL, 16);
	}

	return 0;
}

int sys_uid_hpm(unsigned int *hpm, int size)
{
	char data[128];
	char *p, *sp;
	int ret, i;

	ret = sys_read(SYS_ID_RO, data, sizeof(data));
	if (ret < 0)
		return ret;

	for (p = data, i = 0; ; p = NULL, i++) {
		p = strtok_r(p, ":", &sp);
		if (!p)
			break;

		if (i >= size)
			break;

		hpm[i] = strtoul(p, NULL, 16);
	}

	return 0;
}

int sys_uid_ecid(unsigned int *ecid, int size)
{
	char data[128];
	char *p, *sp;
	int ret, i;

	ret = sys_read(SYS_ID_UUID, data, sizeof(data));
	if (ret < 0)
		return ret;

	for (p = data, i = 0; ; p = NULL, i++) {
		p = strtok_r(p, ":", &sp);
		if (!p)
			break;

		if (i >= size)
			break;

		ecid[i] = strtoul(p, NULL, 16);
	}

	return 0;
}

