/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 *
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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


//
//	Added by Ray Park for NXP3220 MMBlock Frequency & Power
//
#define ASV_MM_PDDR1_RATE	"/sys/devices/platform/pddr1_u_consumer/rate"
#define ASV_MM_CODA_RATE	"/sys/devices/platform/coda_u_consumer/rate"

int sys_dev_mm_coda_set_freq(uint32_t hz)
{
	char data[32] = { 0, };
	int ret;
	uint32_t pddr = 0;

	printf("sys_dev_mm_coda_set_freq = %ld Hz\n", hz);
	switch( hz )
	{
		case 250000000:
			pddr = 1000000000;	//	1 GHz / 4
			break;
		case 300000000:
			pddr = 1800000000;	//	1.8 GHz / 6
			break;
		case 350000000:
			pddr = 1400000000;	//	1.4 GHz / 4
			break;
		case 400000000:
			pddr = 1600000000;	//	1.6 GHz / 4
			break;
		default :
			return -1;
	}

	sprintf(data, "%ld", pddr);
	ret = sys_write(ASV_MM_PDDR1_RATE, data, strlen(data));
	if( 0 != ret )
		return ret;

	sprintf(data, "%ld", hz);
	return sys_write(ASV_MM_CODA_RATE, data, strlen(data));
}

#define ASV_MM_PLL1_RATE	"/sys/devices/platform/pll1_u_consumer/rate"
#define ASV_MM_AXI_BUS_RATE	"/sys/devices/platform/mm_u_consumer/rate"
int sys_dev_mm_axi_set_freq(uint32_t hz)
{
	char data[32] = { 0, };
	int ret;
	uint32_t pll1 = 0;

	printf("sys_dev_mm_axi_set_freq = %ld Hz\n", hz);
	switch( hz )
	{
		case 333333334:
			pll1 = 2000000000;
			break;
		case 400000000:
			pll1 = 1600000000;
			break;
		default :
			return -1;
	}

	sprintf(data, "%ld", pll1);
	ret = sys_write(ASV_MM_PLL1_RATE, data, strlen(data));
	if( 0 != ret )
		return ret;

	sprintf(data, "%ld", hz);
	return sys_write(ASV_MM_AXI_BUS_RATE, data, strlen(data));
}

#define ASV_MM_VOLT	"./sys/class/regulator/regulator.1/microvolts"
int sys_dev_mm_set_volt(uint32_t uVolt)
{
	int ret;
	char data[32] = { 0, };
	uint32_t cutOffVolt = (uVolt+99)/100;
	// Cut off 100 micro volt.
	cutOffVolt = cutOffVolt*100;
	sprintf(data, "%ld", cutOffVolt);
	return sys_write(ASV_MM_VOLT, data, strlen(data));
}
//	End of the MM Block Frequency & Power


//
//	System Bus Block Frequency & Power
//
#define ASV_MM_PLL0_RATE	"/sys/devices/platform/pll0_u_consumer/rate"
#define ASV_MM_SYSBUS_RATE	"/sys/devices/platform/sys0_u_consumer/rate"
#define ASV_MM_SYS_HSIFBUS_RATE	"/sys/devices/platform/sys0_h_u_consumer/rate"
int sys_dev_sysbus_set_freq(uint32_t hz)
{
	char data[32] = { 0, };
	int ret;
	uint32_t pll0 = 0;

	printf("sys_dev_sysbus_set_freq = %ld Hz\n", hz);
	switch( hz )
	{
		case 100000000:
		case 200000000:
		case 250000000:
		case 500000000:
			pll0 = 1000000000;
			break;
		case 150000000:
		case 300000000:
			pll0 = 1200000000;
			break;
		case 350000000:
			pll0 = 700000000;
			break;
		case 400000000:
			pll0 = 800000000;
			break;
		case 450000000:
			pll0 = 900000000;
			break;
		default :
			return -1;
	}
	sprintf(data, "%ld", pll0);
	ret = sys_write(ASV_MM_PLL0_RATE, data, strlen(data));
	if( 0 != ret )
		return ret;

	sprintf(data, "%ld", hz);
	sys_write(ASV_MM_SYS_HSIFBUS_RATE, data, strlen(data));
	if( 0 != ret )
		return ret;
	return sys_write(ASV_MM_SYSBUS_RATE, data, strlen(data));
}

#define ASV_SYSBUS_VOLT		"/sys/class/regulator/regulator.2/microvolts"
int sys_dev_sysbus_set_volt(uint32_t uVolt)
{
	int ret;
	char data[32] = { 0, };
	uint32_t cutOffVolt = (uVolt+99)/100;
	// Cut off 100 micro volt.
	cutOffVolt = cutOffVolt*100;
	sprintf(data, "%ld", cutOffVolt);
	return sys_write(ASV_SYSBUS_VOLT, data, strlen(data));
}
//	End of system bus Block Frequency & Power



#define	CPU_HPM_FILE_NAME	"/sys/devices/platform/cpu/cpu_hpm"
#define CPU_HPM_TEST_CNT	50

int32_t GetHPM_CPU(int32_t *hpm )
{
	int i, sum = 0;
	char buffer[128] = {0, };
	FILE *fp;
	for (i=0; i<50; i++) {
		fp = popen("cat /sys/devices/platform/cpu/cpu_hpm","r");
		fread(buffer,1, 8, fp);
		sum += strtoul(buffer, NULL, 16);
		pclose(fp);
	}
	*hpm = sum/50;

	printf(" HPM AVG : %x \n", *hpm);
	return 0;
}

#define	RO_FILE_NAME	"/sys/devices/platform/cpu/ro"
int32_t GetHPM( uint32_t hpm[8] )
{
	char buffer[128] = {0, };
	int32_t fd;
	if( 0 != access(RO_FILE_NAME, F_OK) ) {
		printf("Cannot access file (%s).\n", RO_FILE_NAME);
		return -1;
	}
	fd = open(RO_FILE_NAME, O_RDONLY);
	if( fd < 0 )
	{
		printf("Cannot open file (%s).\n", RO_FILE_NAME);
		return -1;
	}

	if( 0 > read(fd, buffer, sizeof(buffer)) ) {
		printf("read failed. (file=%s, data=%s)\n", RO_FILE_NAME, buffer);
		close(fd);
		return -1;
	}
	close(fd);

	printf(" %s \n", buffer);
	sscanf(buffer, "%x:%x:%x:%x:%x:%x:%x:%x\n", &hpm[0], &hpm[1], &hpm[2], &hpm[3],
			&hpm[4], &hpm[5], &hpm[6], &hpm[7]);
	return 0;
}

