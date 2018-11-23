#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asv.h"

static const char * const __asv_cmd_string[] = {
	"ASVC_SET_FREQ",
	"ASVC_SET_VOLT",
	"ASVC_GET_ECID",
	"ASVC_RUN",
	"ASVC_STATUS",
	"ASVC_GET_TMU0",
	"ASVC_GET_TMU1",
	"ASVC_ON", /* PC Application Only */
	"ASVC_OFF", /* PC Application Only */
	"ASVC_GET_IDS",
	"ASVC_GET_HPM",
	"ASVC_MAX",
};

static const char * const __asv_id_string[] = {
	"ASVM_CPU",
	"ASVM_VPU",
	"ASVM_3D",
	"ASVM_LDO_SYS",
	"ASVM_DEVICE",
	"ASVM_MAX",
};

static int asv_parse_cmd(char *str, char cmd[][MAX_CMD_STR])
{
	int i, j;

	/* Reset all arguments */
	for (i = 0; i < MAX_CMD_ARG; i++)
		cmd[i][0] = 0;

	for (i = 0; i < MAX_CMD_ARG; i++) {
		/* Remove space char */
		while (*str == ' ')
			str++;

		/* check end of string. */
		if (*str == 0)
			break;

		j = 0;
		while ((*str != ' ') && (*str != 0)) {
			cmd[i][j] = *str++;
			/* to upper char */
			if (cmd[i][j] >= 'a' && cmd[i][j] <= 'z')
				cmd[i][j] += ('A' - 'a');
			j++;
			if (j > (MAX_CMD_STR-1))
				j = MAX_CMD_STR-1;
		}
		cmd[i][j] = 0;
	}

	return i;
}

static ASV_COMMAND asv_cmd_index(char *str)
{
	ASV_COMMAND i;

	for (i = ASVC_SET_FREQ; i < ASVC_MAX; i++) {
		if (!strncmp(__asv_cmd_string[i],
			     str, strlen(__asv_cmd_string[i])))
			return (ASV_COMMAND)i;
	}

	return ASVC_MAX;
}

const char *asv_get_id_name(ASV_MODULE_ID id)
{
	if ((id >= ASVM_MAX) || (id < ASVM_CPU))
		return NULL;

	return __asv_id_string[id];
}

ASV_MODULE_ID asv_get_name_id(char *str)
{
	ASV_MODULE_ID i;

	for (i = ASVM_CPU; i < ASVM_MAX ; i++) {
		if (!strncmp(__asv_id_string[i], str,
			     strlen(__asv_id_string[i])))
			return (ASV_MODULE_ID)i;
	}

	return ASVM_MAX;
}

ASV_RESULT asv_parse_command(char *buff, int size, ASV_COMMAND *cmd,
			     ASV_MODULE_ID *id, ASV_PARAM *param)
{
	char cmds[MAX_CMD_ARG][MAX_CMD_STR];
	ASV_COMMAND CMD;
	ASV_MODULE_ID ID;
	int cnt;

	memset(cmds, 0, sizeof(cmds));

	cnt = asv_parse_cmd(buff, cmds);
	if (cnt < 2)
		return ASV_RES_ERR;

	CMD = asv_cmd_index(cmds[0]);
	if (CMD == ASVC_MAX)
		return ASV_RES_ERR;

	ID = asv_get_name_id(cmds[1]);
	if (ID == ASVM_MAX)
		return ASV_RES_ERR;

	switch (CMD) {
	case ASVC_SET_FREQ:
		param->u32 = atoi(cmds[2]);
		break;

	case ASVC_SET_VOLT:
		param->f32 = strtof(cmds[2], NULL);
		break;

	case ASVC_GET_ECID:
	case ASVC_GET_IDS:
	case ASVC_GET_HPM:
		break;

	case ASVC_GET_TMU0: /*	Get TMU 0 */
	case ASVC_GET_TMU1: /*	Get TMU 1 */
		break;

	case ASVC_RUN:
		break;

	default:
		return ASV_RES_ERR;
	}

	if (cmd)
		*cmd = CMD;
	if (id)
		*id = ID;

	return ASV_RES_OK;
}


