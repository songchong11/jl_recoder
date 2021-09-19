#include "system/includes.h"
#include "app_config.h"


/*任务列表, 注意:stack_size设置为32*n*/
const struct task_info task_info_table[] = {
    {"app_core",            1,     896,  768  },
    {"sys_event",           6,     256,   0    },
    {"btctrler",            4,     512,   384  },
    {"btencry",             1,     512,   128  },
#if TCFG_USER_TWS_ENABLE
    {"tws",                 5,     512,   128  },
#endif
#if (TCFG_USER_TWS_ENABLE || TCFG_USER_BLE_ENABLE || TCFG_USER_EMITTER_ENABLE)
    {"btstack",             3,     768,   256  },
#else
    {"btstack",             3,     512,   256  },
#endif
#if TCFG_DEC2TWS_TASK_ENABLE
    {"local_dec",           3,     768,   128  },
#endif
#if ((TCFG_USER_TWS_ENABLE && TCFG_MIC_EFFECT_ENABLE)||(SOUNDCARD_ENABLE))
    {"audio_dec",           3,     768 + 128 + 128,   128  },
#else
    {"audio_dec",           3,     768 + 32,   128  },
#endif
    {"audio_enc",           3,     512,   128  },
#if TCFG_AUDIO_DEC_OUT_TASK
    {"audio_out",           2,     384,   0},
#endif
    {"aec",					2,	   768,   0	},
    {"aec_dbg",				3,	   128,   128	},
    {"update",				1,	   512,   0		},
    {"dw_update",		 	2,	   256,   128  },
#if(USER_UART_UPDATE_ENABLE)
    {"uart_update",	        1,	   256,   128	},
#endif
    {"systimer",		    6,	   128,   0		},
    {"dev_mg",           	3,     512,   512  },
#ifndef CONFIG_SOUNDBOX_FLASH_256K
    {"usb_msd",           	1,     512,   128   },
    {"usb_audio",           5,     256,   256  },
    {"plnk_dbg",            5,     256,   256  },
    {"adc_linein",          2,     768,   128  },
    /* {"src_write",           1,     768,   256 	}, */
    {"fm_task",             3,     512,   128  },
#endif
    {"enc_write",           1,     768,   0 	},
#if (RCSP_BTMATE_EN || RCSP_ADV_EN)
    {"rcsp_task",			4,	   768,   128	},
#endif
#if (TCFG_SPI_LCD_ENABLE||TCFG_SIMPLE_LCD_ENABLE)
    {"ui",           	    2,     768,   256  },
#else
    {"ui",                  3,     384 - 64,   128  },
#endif

#if(TCFG_CHARGE_BOX_ENABLE)
    {"chgbox_n",            6,     512,   128  },
#endif
#if (SMART_BOX_EN)
    {"smartbox",            4,     768,   128  },
#endif//SMART_BOX_EN
#if (GMA_EN)
    {"tm_gma",              3,     768,   256   },
#endif
#if RCSP_FILE_OPT
    {"file_bs",				1,	   768,	  64  },
#endif
#if TCFG_KEY_TONE_EN
    {"key_tone",            5,     256,   32},
#endif
#if TCFG_MIXER_CYCLIC_TASK_EN
    {"mix_out",             5,     256,   0},
#endif

    {"mic_stream",          5,     768,   128  },
#if(TCFG_HOST_AUDIO_ENABLE)
    {"uac_play",            6,     768,   0    },
    {"uac_record",          6,     768,   32   },
#endif
#if (TUYA_DEMO_EN)
    {"user_deal",           7,     512,   512   },//定义线程 tuya任务调度
#endif
    {0, 0},
};

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

void user_deal_send_ver(void)
{
    //os_taskq_post("user_deal", 1,KEY_USER_DEAL_POST);
    os_taskq_post_msg("user_deal", 1, KEY_USER_DEAL_POST_MSG);
    //os_taskq_post_event("user_deal",1, KEY_USER_DEAL_POST_EVENT);
}

void user_deal_rand_set(u32 rand)
{
    os_taskq_post("user_deal", 2, KEY_USER_DEAL_POST_2, rand);
}

void user_deal_send_array(int *msg, int argc)
{
    if (argc > Q_USER_DATA_SIZE) {
        return;
    }
    os_taskq_post_type("user_deal", Q_USER_DEAL, argc, msg);
}
void user_deal_send_msg(void)
{
    os_taskq_post_event("user_deal", 1, KEY_USER_DEAL_POST_EVENT);
}

void user_deal_send_test(void)///模拟测试函数,可按键触发调用，自行看打印
{
    user_deal_send_ver();
    user_deal_rand_set(0x11223344);
    static u32 data[Q_USER_DATA_SIZE] = {0x11223344, 0x55667788, 0x11223344, 0x55667788, 0x11223344,
                                         0xff223344, 0x556677ee, 0x11223344, 0x556677dd, 0x112233ff,
                                        };
    user_deal_send_array(data, sizeof(data) / sizeof(int));
}

static void user_deal_task_handle(void *p)
{
    int msg[Q_USER_DATA_SIZE + 1] = {0, 0, 0, 0, 0, 0, 0, 0, 00, 0};
    int res = 0;
    while (1) {
        res = os_task_pend("taskq", msg, ARRAY_SIZE(msg));
        if (res != OS_TASKQ) {
            continue;
        }
        r_printf("user_deal_task_handle:0x%x", msg[0]);
        put_buf(msg, (Q_USER_DATA_SIZE + 1) * 4);
        if (msg[0] == Q_MSG) {
            printf("use os_taskq_post_msg");
            switch (msg[1]) {
            case KEY_USER_DEAL_POST_MSG:
                printf("KEY_USER_DEAL_POST_MSG");
                break;
            default:
                break;
            }
        } else if (msg[0] == Q_EVENT) {
            printf("use os_taskq_post_event");
            switch (msg[1]) {
            case KEY_USER_DEAL_POST_EVENT:
                printf("KEY_USER_DEAL_POST_EVENT");
                break;
            default:
                break;
            }
        } else if (msg[0] == Q_CALLBACK) {
        } else if (msg[0] == Q_USER) {
            printf("use os_taskq_post");
            switch (msg[1]) {
            case KEY_USER_DEAL_POST:
                printf("KEY_USER_DEAL_POST");
                break;
            case KEY_USER_DEAL_POST_2:
                printf("KEY_USER_DEAL_POST_2:0x%x", msg[2]);
                break;
            default:
                break;
            }
        } else if (msg[0] == Q_USER_DEAL) {
            printf("use os_taskq_post_type");
            printf("0x%x 0x%x 0x%x 0x%x 0x%x", msg[1], msg[2], msg[3], msg[4], msg[5]);
            printf("0x%x 0x%x 0x%x 0x%x 0x%x", msg[6], msg[7], msg[8], msg[9], msg[10]);
        }
        puts("");
    }
}

void user_deal_init(void)
{
    task_create(user_deal_task_handle, NULL, "user_deal");
}

void user_deal_exit(void)
{
    task_kill("user_deal");
}
