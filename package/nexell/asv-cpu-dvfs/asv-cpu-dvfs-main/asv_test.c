//------------------------------------------------------------------------------
//
//	Copyright (C) 2013 Nexell Co. All Rights Reserved
//	Nexell Co. Proprietary & Confidential
//
//	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module		:
//	File		:
//	Description	:
//	Author		:
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <error.h>
#include "asv_type.h"
#include "asv_command.h"

#include "dvfsutil.h"

#define	ENABLE_EMUL			0
#define MAX_STRING_SIZE		128
#define	MAX_TX_STRING		1024

#define	DBG_COMMAND			1

CBOOL ASV_CPUTest(void);
CBOOL	NX_Test_Init( void );


static void send_data( int fd, const char *msg )
{
	int ret;
	if( fd >0 )
	{
		ret = write( fd, msg, strlen(msg) );
		if(ret < 0)
			printf(" write fail");
	}
}

static void write_msg( int fd, const char *fmt, ... )
{
	char str[MAX_TX_STRING];

	va_list ap;

	va_start( ap, fmt );
	vsnprintf( str, MAX_TX_STRING-1, fmt, ap );
	va_end(ap);

#if DBG_COMMAND
	printf( str );
	fflush( stdout );
#endif
	send_data( fd, str );
}

#if  1
void RUN_TEST( int fd, ASV_MODULE_ID module )
{
#if (!ENABLE_EMUL)
	ASV_RESULT res;
	uint32_t count = 800;	//	10msec * 900 ==> 9 sec
	ASV_TEST_MODULE *test;
	uint64_t start, cur;
	
	if( ASVM_CPU == module )
		test = GetCpuTestModule();
//	else if( ASVM_VPU == module )
//		test = GetVpuTestModule();
//	else if( ASVM_3D == module )
//		test = Get3DTestModule();

	start = NX_GetTickCountUs();

	res = test->run();
	if( res < 0 )
	{
		write_msg(fd, "FAIL : ASVC_RUN %s\n", ASVModuleIDToString(module));
		return;
	}

	while( count-->0 )
	{
		usleep(10000);
		res = test->status();
		if( res < 0 )
		{
			write_msg(fd, "\e[31m FAIL : ASVC_RUN %s\e[0m\n", ASVModuleIDToString(module));
			test->stop();
			return;
		}
		else if( res == ASV_RES_OK )
		{
			write_msg(fd, "SUCCESS : ASVC_RUN %s\n", ASVModuleIDToString(module));
			test->stop();
			return;
		}

		cur = NX_GetTickCountUs();

		if( cur - start > 9000000 )
		{
		//printf("\e[32m %ld \e[0m\n", (cur - start));
			break;
		}
		//printf("\e[31m %ld \e[0m\n", (cur - start));
	}
	res = test->stop();
	if( res == ASV_RES_OK )
	{
		write_msg(fd, "SUCCESS : ASVC_RUN %s\n", ASVModuleIDToString(module));
		return;
	}
	write_msg(fd, "FAIL : ASVC_RUN %s\n", ASVModuleIDToString(module));
#else
//	long r = random();
	static long r = 1;
	r ^= 1;
	if( r % 10 == 1 )
	{
		write_msg(fd, "FAIL : ASVC_RUN %s\n", ASVModuleIDToString(module));
	}
	else
	{
		write_msg(fd, "SUCCESS : ASVC_RUN %s\n", ASVModuleIDToString(module));
	}
#endif
}
#endif

extern int task_command(const char *exec, bool syscmd);

int asv_test_main(void)
{
	ASV_COMMAND asvCmd;
	ASV_MODULE_ID asvModuleID;
	ASV_PARAM asvParam;

	char cmd[MAX_STRING_SIZE];
	int fd;
	int nr;

	fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY);

	struct termios	newtio, oldtio;
	tcgetattr(fd, &oldtio);
	memcpy( &newtio, &oldtio, sizeof(newtio) );
	newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD | IXOFF;
	newtio.c_iflag = IGNPAR | ICRNL;

	newtio.c_oflag = 0;
	newtio.c_lflag = ICANON;

	printf("Default VMIN : %d, VTIME : %d\n", newtio.c_cc[VMIN], newtio.c_cc[VTIME]);

	newtio.c_cc[VMIN] = 0;
	newtio.c_cc[VTIME] = 10;

	tcflush( fd, TCIFLUSH );
	tcsetattr( fd, TCSANOW, &newtio );

//	nr = system("nproc");
	nr = task_command("/usr/bin/cpu_num.sh", 0);
	printf("NR %d\n", nr);
	if(nr == 2)
		write_msg( fd, "\nBOOT DONE.\n");
	else 
		write_msg( fd, "\nBOOT FAIL.\n");

#if 0
	uint32_t ids[2];
	uint32_t ecid[4];
	uint32_t ro[8];
	GetIDS( ids );

		write_msg( fd, "SUCCESS : IDS=%02x-%02x\n", ids[0], ids[1] );
	printf("\n\n=============\n\n");
	GetECID( ecid);
		write_msg( fd, "SUCCESS : ECID=%08x-%08x-%08x-%08x\n", ecid[0], ecid[1], ecid[2], ecid[3] );
	printf("\n\n=============\n\n");
	GetRO( ro );
			write_msg( fd, "SUCCESS : RO=%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
					ro[0], ro[1], ro[2], ro[3], ro[4], ro[5], ro[6], ro[7]);
#endif

	while(1)
	{
		memset(cmd, 0, sizeof(cmd));
		read( fd, cmd, sizeof(cmd));
#if DBG_COMMAND
		printf( cmd );
		fflush( stdout );
#endif
#if 1
		if( ASV_RES_OK == ParseStringToCommand( cmd, sizeof(cmd), &asvCmd, &asvModuleID, &asvParam ) )
		{
			printf("asvCmd : %d \n", asvCmd);
			switch( asvCmd )
			{
			case ASVC_SET_FREQ:
			{
				if( ASVM_CPU == asvModuleID )
				{
					write_msg(fd, "ASVC_SET_FREQ  CPU \n");
					if( 0 != SetCPUFrequency(asvParam.u32) )
					{
						write_msg(fd, "FAIL : ASVC_SET_FREQ ASVM_CPU %dHz\n", asvParam.u32);
						//printf("\e[31m FAIL : ASVC_SET_FREQ ASVM_CPU %dHz \e[0m\n", asvParam.u32);
					}
					else
					{
						write_msg(fd, "SUCCESS : ASVC_SET_FREQ ASVM_CPU %dHz\n", asvParam.u32);
					}
				}
				else if( ASVM_VPU == asvModuleID )
				{
					write_msg(fd, "ASVC_SET_FREQ VPU \n");
					if( 0 != SetVPUFrequency(asvParam.u32) )
					{
						write_msg(fd, "FAIL : ASVC_SET_FREQ ASVM_VPU %dHz\n", asvParam.u32);
					}
					else
					{
						write_msg(fd, "SUCCESS : ASVC_SET_FREQ ASVM_VPU %dHz\n", asvParam.u32);
					}
				}
				else if( ASVM_3D == asvModuleID )
				{
					write_msg(fd, "ASVC_SET_FREQ 3D \n");
					if( 0 != Set3DFrequency(asvParam.u32) )
					{
						write_msg(fd, "FAIL : ASVC_SET_FREQ ASVM_3D %dHz\n", asvParam.u32);
					}
					else
					{
						write_msg(fd, "SUCCESS : ASVC_SET_FREQ ASVM_3D %dHz\n", asvParam.u32);
					}
				}
				else
				{
					write_msg(fd, "FAIL : Unknown Module(%d)\n", asvModuleID);
				}
				break;
			}
			case ASVC_SET_VOLT:
			{
				uint32_t microVolt = asvParam.f32 * 1000000;
				if( ASVM_CPU == asvModuleID )
				{
					if( 0 == SetCPUVoltage( microVolt ) )
					{
						write_msg(fd, "SUCCESS : ASVC_SET_VOLT ASVM_CPU %fv\n", (float)((microVolt)/1000000.));
					}
					else
					{
						write_msg(fd, "FAIL : ASVC_SET_VOLT ASVM_CPU %fv\n", (float)((microVolt)/1000000.));
					}
				}
				else if( ( ASVM_VPU == asvModuleID ) || ( ASVM_3D == asvModuleID ) )
				{
					if( 0 == SetCoreVoltage( microVolt ) )
					{
						write_msg(fd, "SUCCESS : ASVC_SET_VOLT %s %fv\n", ASVModuleIDToString(asvModuleID), (float)((microVolt)/1000000.));
					}
					else
					{
						write_msg(fd, "FAIL : ASVC_SET_VOLT %s %fv\n", ASVModuleIDToString(asvModuleID), (float)((microVolt)/1000000.));
					}
				}
				else if( ASVM_LDO_SYS == asvModuleID )
				{
					if( 0 == SetSystemLDOVoltage( microVolt ) )
					{
						write_msg(fd, "SUCCESS : ASVC_SET_VOLT %s %fv\n", ASVModuleIDToString(asvModuleID), (float)((microVolt)/1000000.));
					}
					else
					{
						write_msg(fd, "FAIL : ASVC_SET_VOLT %s %fv\n", ASVModuleIDToString(asvModuleID), (float)((microVolt)/1000000.));
					}
				}
				else
				{
					write_msg(fd, "FAIL : Unknown Module(%d)\n", asvModuleID);
				}
				break;
			}

			case ASVC_GET_IDS:
			{
				uint32_t ids[2];
				GetIDS( ids );
				write_msg( fd, "SUCCESS : IDS=%02x-%02x\n", ids[0], ids[1] );
				break;
			}

			case ASVC_GET_HPM:
			{
				uint32_t hpm[8];
				GetHPM( hpm );
				write_msg( fd, "SUCCESS : HPM=%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
					hpm[0], hpm[1], hpm[2], hpm[3], hpm[4], hpm[5], hpm[6], hpm[7]);
				break;
			}

			case ASVC_GET_ECID:
			{
				uint32_t ecid[4];
				GetECID( ecid );
				write_msg( fd, "SUCCESS : ECID=%08x-%08x-%08x-%08x\n", ecid[0], ecid[1], ecid[2], ecid[3] );
				break;
			}

			case ASVC_GET_TMU0:
			{
				int tmutemp;
				GetTMU(0, &tmutemp);
				write_msg( fd, "SUCCESS : TMU=%d\n", tmutemp );
				break;
			}
			case ASVC_GET_TMU1:
			{
				int tmutemp;
				GetTMU(0, &tmutemp);
				write_msg( fd, "SUCCESS : TMU=%d\n",tmutemp );
				break;
			}
			case ASVC_RUN:
			{
				write_msg(fd, "TEST : RUN TEST(%d)\n", asvCmd);
				RUN_TEST(fd, asvModuleID);
				break;
			}
			default:
				write_msg(fd, "FAIL : Unknown Command(%d)\n", asvCmd);
				break;
			}
		}
		else
		{
			printf("Command Parsing Error!!! : %s\n", cmd );
		}
#endif
	}
	return 0;
}
extern int set_cmu(void);
int main ( int argc, char *argv[] )
{
//	if (!SetCPUFrequency(500000000))
//		RUN_TEST( 0, ASVM_CPU );
//	return 0;
	set_cmu();
	asv_test_main();
	return 0;
}
