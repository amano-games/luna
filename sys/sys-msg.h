#pragma once

#include "base/types.h"

#define SYS_MSG_QUEUE_LEN 8
#define SYS_MSG_LEN_MAX   256

struct sys_msg_queue {
	u8 items[SYS_MSG_QUEUE_LEN][SYS_MSG_LEN_MAX];
	u16 sizes[SYS_MSG_QUEUE_LEN];
	i32 count;
};

void sys_msg_backend_ini(void);

void sys_msg_push_internal(str8 msg);

b32 sys_msg_cmd_is(str8 msg, str8 cmd, str8 *out_args);
