#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#include "generic/typedef.h"
#include "misc_driver.h"
#include "led_driver.h"
#include "lwrb.h"
#include "ima_enc.h"

#define TARGET_BAUD		1000000

#define ENCODER_ENABLE	1

typedef struct re_state {

	bool recoder_state;
	bool send_pcm_state;
	bool sd_state;
	bool sim_state;
	bool creg_state;
	u32 baud;
	u32 baud_status;

}RECODER;


extern RECODER	recoder;

#define HAD_SET		0xA5
#define HAD_SYNC	0xA5
#define U32MAX		0xffffffff

extern void uart_dev_4g_at_task_del(void);
extern void uart_dev_4g_at_init(u32 baud);
extern void check_at_baud_ret(void);
extern void uart_1_dev_reinit(u32 baud);
extern u32 myuart_dev_close(uart_bus_t *ut);

#endif
