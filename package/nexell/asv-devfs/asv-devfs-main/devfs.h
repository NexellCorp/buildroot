/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 *
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __DEVFS_H__
#define __DEVFS_H__

#ifdef __cplusplus
extern "C"{
#endif

int sys_read(const char *file, char *buffer, int buflen);
int sys_write(const char *file, const char *buffer, int buflen);

/*
 * Voltage APIs (Unit is uV)
 */
int sys_vdd_set(int id, long uV);
unsigned long sys_vdd_get(int id);

/*
 * Frequency APIs (Unit is KHZ)
 */
int sys_dev_set_freq(long khz);
unsigned long sys_dev_get_freq(void);
int sys_dev_avail_freq_num(void);
int sys_dev_avail_freqs(long *khz, int size);
int sys_dev_avail_volts(long *uV, int size);

/*
 * Temperature
 */
int sys_temp_get(int ch, int *temp);

/*
 * Core IDs
 */
int sys_uid_ecid(unsigned int *ecid, int size);
int sys_uid_ids(unsigned int *ids, int size);
int sys_uid_hpm(unsigned int *hpm, int size);

#ifdef __cplusplus
}
#endif

#endif
