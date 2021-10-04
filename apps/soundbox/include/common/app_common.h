
#ifndef __APP_COMMON_H__
#define __APP_COMMON_H__

#include "typedef.h"

extern u8 app_common_key_var_2_event(u32 key_var);

enum {
	APP_USER_MSG_START_STOP_RECODER = 0x10,
	APP_USER_MSG_SEND_FILE_TO_AT,
	APP_USER_MSG_BUFFER_HAVE_DATA,
	APP_USER_MSG_START_RECODER,
	APP_USER_MSG_STOP_RECODER,
};

#endif