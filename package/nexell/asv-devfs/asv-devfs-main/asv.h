#ifndef __ASV_COMMAND_H__
#define __ASV_COMMAND_H__

#include <stdint.h>

#define	MAX_CMD_STR		128
#define MAX_CMD_ARG		8

typedef enum {
	ASVC_SET_FREQ,
	ASVC_SET_VOLT,
	ASVC_GET_ECID,
	ASVC_RUN,
	ASVC_STATUS,
	ASVC_GET_TMU0,	/* Get TMU 0 */
	ASVC_GET_TMU1,	/* Get TMU 1 */
	ASVC_ON,	/* PC Application Only */
	ASVC_OFF,	/* PC Application Only */
	ASVC_GET_IDS,	/* Get IDS */
	ASVC_GET_HPM,	/* Get HPM RO */
	ASVC_MAX,
} ASV_COMMAND;

typedef enum {
	ASVM_CPU,
	ASVM_VPU,
	ASVM_3D,
	ASVM_LDO_SYS,
	ASVM_DEVICE,
	ASVM_MAX,
} ASV_MODULE_ID;

typedef union ASV_PARAM {
	int32_t i32;
	uint32_t u32;
	float f32;
	int64_t i64;
	uint64_t u64;
	double f64;
} ASV_PARAM;

typedef enum {
	ASV_RES_ERR = -1,
	ASV_RES_OK = 0,
	ASV_RES_TESTING = 1
} ASV_RESULT;

#ifdef __cplusplus
extern "C"{
#endif

ASV_MODULE_ID asv_get_name_id(char *str);
const char *asv_get_id_name(ASV_MODULE_ID id);
ASV_RESULT asv_parse_command(char *buff, int size, ASV_COMMAND *cmd,
			     ASV_MODULE_ID *id, ASV_PARAM *param);

#ifdef __cplusplus
}
#endif

#endif
