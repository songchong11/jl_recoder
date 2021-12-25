#include "system/includes.h"
#include "app_config.h"
#include "asm/pwm_led.h"
#include "tone_player.h"
#include "ui_manage.h"
#include "app_main.h"
#include "app_task.h"
#include "asm/charge.h"
#include "app_power_manage.h"
#include "app_charge.h"
#include "user_cfg.h"
#include "power_on.h"
#include "bt.h"
#include "soundcard/peripheral.h"
#include "audio.h"
#include "vm.h"
#include "syscfg_id.h"
#include "public.h"

#define LOG_TAG_CONST       APP
#define LOG_TAG             "[APP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"



APP_VAR app_var;

RECODER	recoder;


void app_entry_idle()
{
    app_task_switch_to(APP_IDLE_TASK);
}

void user_init(void)
{
	recoder.recoder_state = false;
	recoder.send_pcm_state = false;
	recoder.sd_state = false;
	recoder.sim_state = true;// TODO:check
	recoder.creg_state = false;
	recoder.baud = 115200;
	recoder.baud_status = false;
	recoder.module_status = POWER_OFF;
}

void app_task_loop()
{
    while (1) {
        switch (app_curr_task) {
        case APP_POWERON_TASK:
            log_info("APP_POWERON_TASK \n");
            app_poweron_task();
            break;
        case APP_POWEROFF_TASK:
            log_info("APP_POWEROFF_TASK \n");
            app_poweroff_task();
            break;
        case APP_BT_TASK:
            log_info("APP_BT_TASK \n");
            app_bt_task();
            break;
        case APP_MUSIC_TASK:
            log_info("APP_MUSIC_TASK \n");
            app_music_task();
            break;
        case APP_FM_TASK:
            log_info("APP_FM_TASK \n");
            app_fm_task();
            break;
        case APP_RECORD_TASK:
            log_info("APP_RECORD_TASK \n");
            app_record_task();
            break;
        case APP_LINEIN_TASK:
            log_info("APP_LINEIN_TASK \n");
            app_linein_task();
            break;
        case APP_RTC_TASK:
            log_info("APP_RTC_TASK \n");
            app_rtc_task();
            break;
        case APP_PC_TASK:
            log_info("APP_PC_TASK \n");
            app_pc_task();
            break;
        case APP_SPDIF_TASK:
            log_info("APP_SPDIF_TASK \n");
            app_spdif_task();
            break;
        case APP_IDLE_TASK:
            log_info("APP_IDLE_TASK \n");
            app_idle_task();
            break;
        case APP_SLEEP_TASK:
            log_info("APP_SLEEP_TASK \n");
            app_sleep_task();
            break;
        case APP_SMARTBOX_ACTION_TASK:
            log_info("APP_SMARTBOX_ACTION_TASK \n");
            app_smartbox_task();
            break;
        }
        app_task_clear_key_msg();//清理按键消息
        //检查整理VM
        vm_check_all(0);
    }
}

extern void uart_dev_receive_init();
extern void file_write_thread_init(void);
extern void at_4g_thread_init(void);
extern void set_sys_time(struct sys_time *time);

void app_main()
{
    log_info("app_main\n");

    app_var.start_time = timer_get_ms();

    if (get_charge_online_flag()) {

        app_var.poweron_charge = 1;

#if (TCFG_SYS_LVD_EN == 1)
        vbat_check_init();
#endif

#ifndef  PC_POWER_ON_CHARGE
        app_curr_task = APP_IDLE_TASK;
#else
        app_curr_task = APP_POWERON_TASK;
#endif
    } else {
#if SOUNDCARD_ENABLE
        soundcard_peripheral_init();
#endif

#if TCFG_HOST_AUDIO_ENABLE
        void usb_host_audio_init(int (*put_buf)(void *ptr, u32 len), int *(*get_buf)(void *ptr, u32 len));
        usb_host_audio_init(usb_audio_play_put_buf, usb_audio_record_get_buf);
#endif

        /* endless_loop_debug_int(); */
        ui_update_status(STATUS_POWERON);

        //app_curr_task = APP_POWERON_TASK;
        //app_curr_task = APP_RTC_TASK;
        app_curr_task = APP_IDLE_TASK;
    }

#if TCFG_CHARGE_BOX_ENABLE
    app_curr_task = APP_IDLE_TASK;
#endif

	user_init();
	uart_dev_receive_init();//uart 0 receive data from bes
	check_at_baud_ret();
	//at_4g_thread_init(); // 4g driver
    app_task_loop();

}


