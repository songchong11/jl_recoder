#include "includes.h"
#include "system/includes.h"
#include "asm/uart_dev.h"
#include "system/event.h"
#include "common/app_common.h"
#include "lwrb.h"
#include "at_4g_driver.h"

#define MODULE_PWR_GPIO		IO_PORTB_03

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
#define Q_USER_DATA_SIZE  10///理论Queue受任务声明struct task_info.qsize限制,但不宜过大,建议<=6

#if 1

void module_4g_init(void)
{
	gpio_set_pull_up(MODULE_PWR_GPIO, 1);
	gpio_set_pull_down(MODULE_PWR_GPIO, 1);
	gpio_set_die(MODULE_PWR_GPIO, 1);
	gpio_set_direction(MODULE_PWR_GPIO, 0);
	gpio_set_output_value(MODULE_PWR_GPIO, 1);
}

void module_power_on(void)
{
	gpio_set_output_value(MODULE_PWR_GPIO, 0);
}

void module_power_off(void)
{
	gpio_set_output_value(MODULE_PWR_GPIO, 1);
}


static void at_4g_task_handle(void *arg)
{
    int ret;
    int msg[Q_USER_DATA_SIZE + 1] = {0, 0, 0, 0, 0, 0, 0, 0, 00, 0};

    while (1) {
        ret = os_task_pend("taskq", msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }
        //put_buf(msg, (Q_USER_DATA_SIZE + 1) * 4);
        if (msg[0] == Q_MSG) {
	        switch (msg[1]) {
	            case APP_USER_MSG_START_SEND_FILE_TO_AT:
	                printf("APP_USER_MSG_START_SEND_FILE_TO_AT");
					/*power on 4g module and send file to at*/
					module_4g_init();
					module_power_on();

	                break;

				case APP_USER_MSG_STOP_SEND_FILE_TO_AT:
					printf("APP_USER_MSG_STOP_SEND_FILE_TO_AT");
					/*power off 4g module */
					module_power_off();
					break;

	            default:
	                break;
	       }
        }
   }
}


void at_4g_thread_init(void)
{

    printf("at_4g_thread_init\n");

	os_task_create(at_4g_task_handle, NULL, 7, 512, 512, "at_4g_task");
	//task_create(at_4g_task_handle,NULL, "at_4g_task");

}


void at_4g_task_del(void)
{
    printf("at_4g_task_del\n");

    os_task_del("at_4g_task");
}
#endif



/*****************************************************************************/
