//------------------------------------------------------------------------------
//
//  Copyright (C) 2013 Nexell Co. All Rights Reserved
//  Nexell Co. Proprietary & Confidential
//
//  NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//  Module      :
//  File        :
//  Description :
//  Author      :
//  Export      :
//  History     :
//
//------------------------------------------------------------------------------

#include <string.h>
#include <stdio.h>
#include <stdlib.h>		// malloc
#include <unistd.h>		// write
#include <fcntl.h>		// open
#include <assert.h>		// assert
#include <stdint.h>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/time.h>		//gettimeofday
#include <linux/sched.h> 	/* SCHED_NORMAL, SCHED_FIFO, SCHED_RR, SCHED_BATCH */
#include <errno.h> 		/* error */
#include <sys/resource.h>
#include <sched.h> 			/* schedule */

#include "asv_type.h"
#include "dvfsutil.h"

#define	MAX_FILE_PATH	256
#define	MAX_FILE_DATA	128

#define	MAX_NUM_CPU_CORE	2

// #> echo userspace  > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor	//	1. user mode 설정
// #> echo 1000000 >  /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed	//	2. 원하는 freq설정
#define	CPU_FREQ_FILE_PREFIX	"/sys/devices/system/cpu/cpu"

// DCDC1(ARM) :  /sys/class/regulator/regulator.1 # echo 1100000 > microvolts
// DCDC2(CORE) : /sys/class/regulator/regulator.2
// DCDC3(SYS) :   /sys/class/regulator/regulator.3
#define	PMIC_SYS_FILE_PREFIX	"/sys/class/regulator/regulator"
#define	PMIC_SYS_VOLTS_FILE	"microvolts"

// /sys/devices/platform/pll/pll.2
#define	VPU_3D_FREQ_FILE		"/sys/devices/platform/pll/pll.2"

int task_command(const char *exec, bool syscmd)
{
	FILE *fp;
	char buf[16];
	size_t len;
	int ret;

	if (syscmd) {
		ret = system(exec);
		return (int)((char)WEXITSTATUS(ret));
	}

	fp = popen(exec, "r");
	if (!fp) {
		printf("ERRor\n");
		return errno;
	}

	len = fread((void*)buf, sizeof(char), sizeof(buf), fp);
	if (!len) {
		pclose(fp);
		printf("ERRor\n");
	        return errno;
	}
	pclose(fp);

	return strtol(buf, NULL, 10);
}

int set_new_scheduler(pid_t pid, int policy, int priority)
{
	struct sched_param param;
	int maxpri = 0, minpri = 0;
	int ret;

	switch(policy) {
		case SCHED_NORMAL:
		case SCHED_FIFO:
		case SCHED_RR:
		case SCHED_BATCH:	break;
		default:
			printf("invalid policy %d (0~3)\n", policy);
			return -1;
	}

	if(SCHED_NORMAL == policy) {
		maxpri =  20;	// #define NICE_TO_PRIO(nice)	(MAX_RT_PRIO + (nice) + 20), MAX_RT_PRIO 100
		minpri = -20;	// nice
	} else {
		maxpri = sched_get_priority_max(policy);
		minpri = sched_get_priority_min(policy);
	}

	if((priority > maxpri) || (minpri > priority)) {
		printf("\nFail, invalid priority (%d ~ %d)...\n", minpri, maxpri);
		return -1;
	}

	if(SCHED_NORMAL == policy) {
		param.sched_priority = 0;
		ret = sched_setscheduler(pid, policy, &param);
		if(ret) {
			printf("Fail, sched_setscheduler(ret %d), %s\n\n", ret, strerror(errno));
			return ret;
		}
		ret = setpriority(PRIO_PROCESS, pid, priority);
		if(ret)
			printf("Fail, setpriority(ret %d), %s\n\n", ret, strerror(errno));
	} else {
		param.sched_priority = priority;
		ret = sched_setscheduler(pid, policy, &param);
		if(ret)
			printf("Fail, sched_setscheduler(ret %d), %s\n\n", ret, strerror(errno));
	}
	return ret;
}


#if 1
static int32_t _WriteSysInterface( const char *file, const char *buffer, int32_t bufSize )
{
	int32_t fd;
	if( 0 != access(file, F_OK) ) {
		printf("Cannot access file (%s).\n", file);
		return -1;
	}
	fd = open(file, O_WRONLY);
	if( fd < 0 )
	{
		printf("Cannot open file (%s).\n", file);
		return -1;
	}

	printf("[[[[[ Write %s", buffer);
	if( 0 > write(fd, buffer, bufSize) ) {
		printf("Write failed. (file=%s, data=%s)\n", file, buffer);
		close(fd);
		return -1;
	}
	close(fd);

#if 1
	{
		char readBuf[64];
		fd = open(file, O_RDONLY);
		if( fd < 0 )
		{
			printf("Cannot open file (%s).\n", file);
			return -1;
		}

		if( 0 > read(fd, readBuf, sizeof(readBuf)) ) {
			printf("Read failed. (file=%s, data=%s)\n", file, buffer);
			close(fd);
			return -1;
		}
		printf("[[[[[ Read %s", readBuf);
		close(fd);
	}
#endif
	return 0;
}


//
//	Freqeuncy Control APIs
//
#if 1
int32_t SetCPUFrequency( uint32_t freq )
{
	int32_t i, len;
	char fileName[MAX_FILE_PATH];
	char dataStr[MAX_FILE_DATA];
	int r_hz, max_hz, min_hz;

	 i = 0;
//	for( i=0 ; i<MAX_NUM_CPU_CORE ; i++ )
	{
		//	Set to User Mode
		snprintf(fileName, sizeof(fileName), "%s%d/cpufreq/scaling_governor", CPU_FREQ_FILE_PREFIX, i);
		len = snprintf( dataStr, sizeof(dataStr), "%s", "userspace" );
		if( _WriteSysInterface( fileName, dataStr, len ) < 0 )
		{
			return -1;
		}
		//	Set Frequency
		snprintf(fileName, sizeof(fileName), "%s%d/cpufreq/scaling_setspeed", CPU_FREQ_FILE_PREFIX, i);
		len = snprintf( dataStr, sizeof(dataStr), "%d", freq/1000 );
		if( _WriteSysInterface( fileName, dataStr, len ) < 0 )
		{
			return -1;
		}
	}

	r_hz = task_command("mhz | awk {'print $1}'", 0);
	r_hz *= 1000;
	max_hz =  min_hz = freq / 1000;
	max_hz = max_hz + (max_hz *0.1);
	min_hz = min_hz - (min_hz *0.1);
	printf("Real set Freq : %dNhz\n", r_hz/1000);
	if((r_hz > max_hz) || (r_hz < min_hz)) {
		return -1;

	}
	

	return 0;
}
#else
#define CPU_FREQ_FILE	"/sys/devices/platform/pll/pll.1"
int32_t SetCPUFrequency( uint32_t freq )
{
	char dataStr[MAX_FILE_DATA];
	sprintf(dataStr, "%d", freq/1000);
	return _WriteSysInterface( CPU_FREQ_FILE, dataStr, strlen(dataStr) );
}
#endif

int32_t Set3DFrequency( uint32_t freq )
{
	char dataStr[MAX_FILE_DATA];
	sprintf(dataStr, "%d", freq/500);
	return _WriteSysInterface( VPU_3D_FREQ_FILE, dataStr, strlen(dataStr) );
}

int32_t SetVPUFrequency( uint32_t freq )
{
	char dataStr[MAX_FILE_DATA];
	sprintf(dataStr, "%d", freq/500);
	return _WriteSysInterface( VPU_3D_FREQ_FILE, dataStr, strlen(dataStr) );
}


//
//	Voltage Control APIs
//
int32_t SetCPUVoltage( uint32_t microVolt )			//	CPU
{
	return SetRegulatorVoltage(REGUL_ID_CPU, microVolt);
}

int32_t SetCoreVoltage( uint32_t microVolt )		//	MPEG/3D
{
	return SetRegulatorVoltage(REGUL_ID_CORE, microVolt);
}

int32_t SetSystemVoltage( uint32_t microVolt )		//	System(3.3v)
{
	return SetRegulatorVoltage(REGUL_ID_SYSTEM, microVolt);
}

int32_t SetSystemLDOVoltage( uint32_t microVolt )		//	LDO Number 4
{
	return SetRegulatorVoltage(REGUL_ID_LDO_SYS, microVolt);
}

//void DC_SetVolt( U32 powerport, U32 uVoltage ); // micro Volatage
//void NX_CONSOLE_Printf(const char *format, ...);
int32_t SetRegulatorVoltage( REGUL_ID_TYPE id, uint32_t microVolt )
{
	char fileName[MAX_FILE_PATH];
	char dataStr[MAX_FILE_DATA];
	uint32_t cutOffVolt = (microVolt+99)/100;
	// Cut off 100 micro volt.
	cutOffVolt = cutOffVolt*100;

	if( cutOffVolt != microVolt )
	{
		printf("[Regulator %d] : CutOffVolt=%d, microVolt=%d\n", id, cutOffVolt, microVolt);
		microVolt = cutOffVolt;
	}

	sprintf(fileName, "%s.%d/%s", PMIC_SYS_FILE_PREFIX, id, PMIC_SYS_VOLTS_FILE);
	sprintf(dataStr, "%d", microVolt);

//	DC_SetVolt(id, microVolt);
	return _WriteSysInterface( fileName, dataStr, strlen(dataStr) );
}
#endif

#define	IDS_FILE_NAME	"/sys/devices/platform/cpu/ids"
int32_t GetIDS( uint32_t ids[2] )
{
	char buffer[128];
	int32_t fd;
	if( 0 != access(IDS_FILE_NAME, F_OK) ) {
		printf("Cannot access file (%s).\n", IDS_FILE_NAME);
		return -1;
	}
	fd = open(IDS_FILE_NAME, O_RDONLY);
	if( fd < 0 )
	{
		printf("Cannot open file (%s).\n", IDS_FILE_NAME);
		return -1;
	}

	if( 0 > read(fd, buffer, sizeof(buffer)) ) {
		printf("read failed. (file=%s, data=%s)\n", IDS_FILE_NAME, buffer);
		close(fd);
		return -1;
	}
	close(fd);

	printf(" %s \n", buffer);
	sscanf(buffer, "%x:%x\n", &ids[0], &ids[1]);
	printf(" IDS  = %02x-%02x\n", ids[0], ids[1]);
	return 0;
}


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
#define	ECID_FILE_NAME	"/sys/devices/platform/cpu/uuid"
int32_t GetECID( uint32_t ecid[4] )
{
	char buffer[128];
	int32_t fd;
	if( 0 != access(ECID_FILE_NAME, F_OK) ) {
		printf("Cannot access file (%s).\n", ECID_FILE_NAME);
		return -1;
	}
	fd = open(ECID_FILE_NAME, O_RDONLY);
	if( fd < 0 )
	{
		printf("Cannot open file (%s).\n", ECID_FILE_NAME);
		return -1;
	}

	if( 0 > read(fd, buffer, sizeof(buffer)) ) {
		printf("read failed. (file=%s, data=%s)\n", ECID_FILE_NAME, buffer);
		close(fd);
		return -1;
	}
	close(fd);

	printf(" %s \n", buffer);
	sscanf(buffer, "%x:%x:%x:%x\n", &ecid[0], &ecid[1], &ecid[2], &ecid[3]);
	printf("%x:%x:%x:%x\n", ecid[0], ecid[1], ecid[2],ecid[3]);
	return 0;
}
#define	CPU_FREQ_GET_NAME	"sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"
#define TMU0_FILE_NAME		"/sys/class/thermal/thermal_zone0/temp"

int32_t GetTMU( uint32_t channel, int *tmutemp )
{
	char buffer[128];
	int32_t fd;
	if( 0 != access(TMU0_FILE_NAME, F_OK) ) {
		printf("Cannot access file (%s).\n", TMU0_FILE_NAME);
		return -1;
	}
	fd = open(TMU0_FILE_NAME, O_RDONLY);
	if( fd < 0 )
	{
		printf("Cannot open file (%s).\n", TMU0_FILE_NAME);
		return -1;
	}

	if( 0 > read(fd, buffer, sizeof(buffer)) ) {
		printf("read failed. (file=%s, data=%s)\n", TMU0_FILE_NAME, buffer);
		close(fd);
		return -1;
	}
	close(fd);
	
	*tmutemp = atoi(buffer);

	return 0;
}

uint64_t NX_GetTickCountUs( void )
{
	uint64_t ret = 0;
	struct timeval	tv;

	gettimeofday( &tv, NULL );
	ret = ((uint64_t)tv.tv_sec)*1000000 + tv.tv_usec;
	return ret;
}
