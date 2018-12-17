#ifndef __ASV_TYPE_H__
#define __ASV_TYPE_H__


#ifdef WIN32
typedef signed char			int8_t;
typedef signed short		int16_t;
typedef signed int			int32_t;
typedef __int64				int64_t;

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned int		uint32_t;
typedef unsigned __int64	uint64_t;
#else	//	Linux other
#include "stdint.h"
#endif


typedef enum{
	ASV_RES_ERR     = -1,
	ASV_RES_OK      = 0,
	ASV_RES_TESTING = 1
} ASV_RESULT;


//
//  1. run 실행 후 반드시 100 msec 이내에 return 되어야 함.
//	2. stop 이후 반드시 100 msec 이내에 return 되어야 함.
//	3. status 실행 이후 즉시 status가 return 되어야 함.
//
typedef struct ASV_TEST_MODULE
{
	char name[64];					//	Module Name
	ASV_RESULT (*run)(void);	//	Run Test
	ASV_RESULT (*stop)(void);	//	Stop Test
	ASV_RESULT (*status)(void);	//	Current Status (ERR,OK,TESTING)
} ASV_TEST_MODULE;


#ifdef __cplusplus
extern "C"{
#endif
//
//	CPU / 3D / VPU 각각의 Module의 실행파일은 아래와 같은 API 를 extern 하여야 함.
//
extern ASV_TEST_MODULE *GetCpuTestModule(void);
extern ASV_TEST_MODULE *GetVpuTestModule(void);
extern ASV_TEST_MODULE *Get3DTestModule(void);

#ifdef __cplusplus
}
#endif

///
typedef char			S8;					///< 8bit signed integer(s.7) value
typedef short			S16;				///< 16bit signed integer(s.15) value
typedef int				S32;				///< 32bit signed integer(s.31) value
typedef unsigned char	U8;					///< 8bit unsigned integer value
typedef unsigned short	U16;				///< 16bit unsigned integer value
typedef unsigned int	U32;				///< 32bit unsigned integer value

#define S8_MIN			-128				///< signed char min value
#define S8_MAX			127					///< signed char max value
#define S16_MIN			-32768				///< signed short min value
#define S16_MAX			32767				///< signed short max value
#define S32_MIN			-2147483648			///< signed integer min value
#define S32_MAX			2147483647			///< signed integer max value

#define U8_MIN			0					///< unsigned char min value
#define U8_MAX			255					///< unsigned char max value
#define U16_MIN			0					///< unsigned short min value
#define U16_MAX			65535				///< unsigned short max value
#define U32_MIN			0					///< unsigned integer min value
#define U32_MAX			4294967295			///< unsigned integer max value
/// @}

//==============================================================================
/// @name Boolean data type
///
/// C and C++ has difference boolean type. so we use signed integer type \n
/// instead of bool and you must use CTRUE or CFALSE macro for CBOOL type
//
//==============================================================================
/// @{
typedef S32	CBOOL;							///< boolean type is 32bits signed integer
#define CTRUE	1							///< true value is	integer one
#define CFALSE	0

#if 0	//	CPU Test Application Example
// CPU Test Application Example
ASV_TEST_RESULT cpu_test()
{
	ASV_TEST_MODULE *test = GetCpuTestModule();
	ASV_RESULT res;
	uint32_t count = 10;

	res = test->run();
	if( res < 0 )
		return res;

	while(count-->0)
	{
		usleep(1000000);	//	sleep 1sec
		res = test->status();
		if( res < 0 )
		{
			return res;
		}
	}
	res = test->stop();
	return res;
}
#endif


#endif // __ASV_TYPE_H__
