#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#include "generic/typedef.h"
#include "misc_driver.h"
#include "led_driver.h"
#include "lwrb.h"
#include "ima_enc.h"
#include "app_task.h"

#define TARGET_BAUD		1000000

#define ENCODER_ENABLE	0
#define	USE_LWRB		1
#define DEBUG_FILE_SYS	0
#define WAV_FORMAT		1

#define SIM_CARD_TYPE	CTNET//CMNET


#define POWER_ON	1
#define POWER_OFF	2

typedef struct re_state {

	bool recoder_state;
	bool send_pcm_state;
	bool sd_state;
	bool sim_state;
	bool creg_state;
	u32 baud;
	u32 baud_status;
	u8  module_status;

}RECODER;


extern RECODER	recoder;

#define HAD_SET		0xA5
#define HAD_SYNC	0xA5
#define U32MAX		0xffffffff


enum {
    KEY_USER_DEAL_POST = 0,
    KEY_USER_DEAL_POST_MSG,
    KEY_USER_DEAL_POST_EVENT,
    KEY_USER_DEAL_POST_2,
};

#include "system/includes.h"
#include "system/event.h"

///自定义事件推送的线程

#define Q_USER_DEAL   0xAABBCC ///自定义队列类型
#define Q_USER_DATA_SIZE  6///理论Queue受任务声明struct task_info.qsize限制,但不宜过大,建议<=6


extern void get_sys_time(struct sys_time *time);//获取时间


extern void uart_dev_4g_at_task_del(void);
extern void uart_dev_4g_at_init(u32 baud);
extern void check_at_baud_ret(void);
extern void uart_1_dev_reinit(u32 baud);
extern u32 myuart_dev_close(uart_bus_t *ut);
extern void clear_rx_buffer(void);
extern void prepare_start_send_pcm(void);
extern void stop_send_pcm_to_at(void);
extern void get_next_file(void);
extern int rename_file_when_send_over(FILE* fs, char *file_name);
extern void file_read_and_send(FILE *read_p);

#endif
