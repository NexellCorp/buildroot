#ifndef __ASV_COMMAND_H__
#define __ASV_COMMAND_H__

#include <stdint.h>

#define	MAX_CMD_STR		128
#define MAX_CMD_ARG		8

typedef enum{
	ASV_RES_ERR     = -1,
	ASV_RES_OK      = 0,
	ASV_RES_TESTING = 1
} ASV_RESULT;

typedef enum {
	ASVC_SET_FREQ,
	ASVC_SET_VOLT,
	ASVC_GET_ECID,
	ASVC_RUN,
	ASVC_STATUS,
	ASVC_GET_TMU0,		//	Get TMU 0
	ASVC_GET_TMU1,		//	Get TMU 1
	ASVC_ON,			//	PC Application Only
	ASVC_OFF,			//	PC Application Only
	ASVC_GET_IDS,		//	Get IDS
	ASVC_GET_HPM,		//	Get HPM RO
	ASVC_GET_CPUHPM,	//	Get BOOT CPUHPM RO
	ASVC_GET_CORE_HPM,	//	Get Core HPM
	ASVC_SET_AXI_FREQ,	//	Set AXI Frequency : 
						//		It was orignally created to set AXI bus freqeuncy for NXP3220's MM Block.
	ASVC_MAX,
}ASV_COMMAND;

typedef enum {
	ASVM_CPU,
	ASVM_MM,			//	MM BLOCK
	ASVM_USB,			//	MSB BLOCK
	ASVM_HSIF,			//	HSIF BLOCK
	ASVM_SYS,			//	SYS BUS
	ASVM_MAX,
}ASV_MODULE_ID;

typedef union ASV_PARAM{
	int32_t		i32;
	uint32_t	u32;
	float		f32;
	int64_t		i64;
	uint64_t	u64;
	double		f64;
}ASV_PARAM;

#ifdef __cplusplus
extern "C"{
#endif

const char *ASVCommandToString( ASV_COMMAND cmd );
ASV_COMMAND ASVStringToCommand( char *cmdStr );

const char *ASVModuleIDToString(ASV_MODULE_ID id);
const char *ASVModuleIDToStringSimple(ASV_MODULE_ID id);
ASV_MODULE_ID ASVStringModuleID( char *idStr);

ASV_RESULT MakeCommandString( char *outBuf, int32_t outSize, ASV_COMMAND cmd, ASV_MODULE_ID id, ASV_PARAM param );

ASV_RESULT ParseStringToCommand( char *inBuf, int32_t intSize, ASV_COMMAND *cmd, ASV_MODULE_ID *id, ASV_PARAM *param );

int32_t GetArgument( char *pSrc, char arg[][MAX_CMD_STR] );


#ifdef __cplusplus
}
#endif

#endif // __ASV_COMMAND_H__
