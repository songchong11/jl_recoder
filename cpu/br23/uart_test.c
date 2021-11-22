#include "system/includes.h"
#include "asm/uart_dev.h"
#include "system/event.h"
#include "common/app_common.h"
#include "lwrb.h"
#include "led_driver.h"

extern bool recoder_state;
extern lwrb_t receive_buff;

#if 1
/*
    [[  注意!!!  ]]
    * 如果当系统任务较少时使用本demo，需要将低功耗关闭（#define TCFG_LOWPOWER_LOWPOWER_SEL    0//SLEEP_EN ），否则任务被串口接收函数调用信号量pend时会导致cpu休眠，串口中断和DMA接收将遗漏数据或数据不正确
*/

#define UART_DEV_USAGE_TEST_SEL         2       //uart_dev.c api接口使用方法选择
//  选择1  串口中断回调函数推送事件，由事件响应函数接收串口数据
//  选择2  由task接收串口数据

#define UART_DEV_TEST_MULTI_BYTE        1       //uart_dev.c 读写多个字节api / 读写1个字节api 选择

#define UART_DEV_FLOW_CTRL				0

static u8 uart_cbuf[512] __attribute__((aligned(4)));
static u8 uart_rxbuf[512] __attribute__((aligned(4)));

static void my_put_u8hex(u8 dat)
{
    u8 tmp;
    tmp = dat / 16;
    if (tmp < 10) {
        putchar(tmp + '0');
    } else {
        putchar(tmp - 10 + 'A');
    }
    tmp = dat % 16;
    if (tmp < 10) {
        putchar(tmp + '0');
    } else {
        putchar(tmp - 10 + 'A');
    }
    putchar(0x20);
}

//设备事件响应demo
static void uart_event_handler(struct sys_event *e)
{
    const uart_bus_t *uart_bus;
    u32 uart_rxcnt = 0;

    if ((u32)e->arg == DEVICE_EVENT_FROM_UART_RX_OVERFLOW) {
        if (e->u.dev.event == DEVICE_EVENT_CHANGE) {
            /* printf("uart event: DEVICE_EVENT_FROM_UART_RX_OVERFLOW\n"); */
            uart_bus = (const uart_bus_t *)e->u.dev.value;
            uart_rxcnt = uart_bus->read(uart_rxbuf, sizeof(uart_rxbuf), 0);
            if (uart_rxcnt) {
                printf("--get_buffer:\n");
                for (int i = 0; i < uart_rxcnt; i++) {
                    my_put_u8hex(uart_rxbuf[i]);
                    if (i % 16 == 15) {
                        putchar('\n');
                    }
                }
                if (uart_rxcnt % 16) {
                    putchar('\n');
                }
#if (!UART_DEV_FLOW_CTRL)
                uart_bus->write(uart_rxbuf, uart_rxcnt);
#endif
            }
            printf("uart out\n");
        }
    }
    if ((u32)e->arg == DEVICE_EVENT_FROM_UART_RX_OUTTIME) {
        if (e->u.dev.event == DEVICE_EVENT_CHANGE) {
            /* printf("uart event:DEVICE_EVENT_FROM_UART_RX_OUTTIME\n"); */
            uart_bus = (const uart_bus_t *)e->u.dev.value;
            uart_rxcnt = uart_bus->read(uart_rxbuf, sizeof(uart_rxbuf), 0);
            if (uart_rxcnt) {
                printf("++get_buffer:\n");
                for (int i = 0; i < uart_rxcnt; i++) {
                    my_put_u8hex(uart_rxbuf[i]);
                    if (i % 16 == 15) {
                        putchar('\n');
                    }
                }
                if (uart_rxcnt % 16) {
                    putchar('\n');
                }
#if (!UART_DEV_FLOW_CTRL)
                uart_bus->write(uart_rxbuf, uart_rxcnt);
#endif
            }
            printf("uart out\n");
        }
    }
}
SYS_EVENT_HANDLER(SYS_DEVICE_EVENT, uart_event_handler, 0);

extern lwrb_t receive_buff;
u32 rx_total = 0;
u8 led_cnt;
static void uart_u_task_handle(void *arg)
{
		const uart_bus_t *uart_bus = arg;
		int ret;
		u32 uart_rxcnt = 0;

		printf("uart_at_task start\n");
		while (1) {

			//uart_bus->read()在尚未收到串口数据时会pend信号量，挂起task，直到UART_RX_PND或UART_RX_OT_PND中断发生，post信号量，唤醒task
			uart_rxcnt = uart_bus->read(uart_rxbuf, sizeof(uart_rxbuf), 0);
			
			//printf("%d, %x %x", uart_rxcnt, uart_rxbuf[0], uart_rxbuf[1]);

			if (uart_rxcnt && recoder_state && \
					uart_rxbuf[0] == 0xcd && uart_rxbuf[1] == 0xab &&\ 
					uart_rxbuf[2] == 0x40 && uart_rxbuf[3] == 0x01) {

				printf("%d", rx_total);

				rx_total++;

				if ((rx_total % 10) == 0)// bilnk green led when recoder
					led_green_toggle();

#if 0
				for (int i = 0; i < uart_rxcnt; i++) {
					my_put_u8hex(uart_rxbuf[i]);
					if (i % 16 == 15) {
						putchar('\n');
					}
				}
				if (uart_rxcnt % 16) {
					putchar('\n');
				}
#endif

				/*push receive data to fifo*/
				ret = lwrb_is_ready(&receive_buff);

				int	n_written = lwrb_write(&receive_buff, &uart_rxbuf[4], uart_rxcnt - 4);
				if (n_written != uart_rxcnt - 4) {
					printf("write lwrb buffer error %d %d\n", n_written, uart_rxcnt);
					
				} else {
					os_taskq_post_msg("file_write", 1, APP_USER_MSG_BUFFER_HAVE_DATA);
				}
#if (!UART_DEV_FLOW_CTRL)
				//uart_bus->write(uart_rxbuf, uart_rxcnt);
#endif
			}
		}

}

static void uart_isr_hook(void *arg, u32 status)
{
    const uart_bus_t *ubus = arg;
    struct sys_event e;

    //当CONFIG_UARTx_ENABLE_TX_DMA（x = 0, 1）为1时，不要在中断里面调用ubus->write()，因为中断不能pend信号量
    if (status == UT_RX) {
        //printf("uart_rx_isr\n");
#if (UART_DEV_USAGE_TEST_SEL == 1)
        e.type = SYS_DEVICE_EVENT;
        e.arg = (void *)DEVICE_EVENT_FROM_UART_RX_OVERFLOW;
        e.u.dev.event = DEVICE_EVENT_CHANGE;
        e.u.dev.value = (int)ubus;
        sys_event_notify(&e);
#endif
    }
    if (status == UT_RX_OT) {
        //printf("uart_rx_ot_isr\n");
#if (UART_DEV_USAGE_TEST_SEL == 1)
        e.type = SYS_DEVICE_EVENT;
        e.arg = (void *)DEVICE_EVENT_FROM_UART_RX_OUTTIME;
        e.u.dev.event = DEVICE_EVENT_CHANGE;
        e.u.dev.value = (int)ubus;
        sys_event_notify(&e);
#endif
    }
}

static void uart_flow_ctrl_task(void *arg)
{
    const uart_bus_t *uart_bus = arg;
	while (1) {
		uart_bus->write("flow control test ", sizeof("flow control test "));
		os_time_dly(100);
	}
}

const uart_bus_t *uart_bus;

void uart_dev_receive_init()
{
    struct uart_platform_data_t u_arg = {0};
    u_arg.tx_pin = IO_PORTA_05;
    u_arg.rx_pin = IO_PORTA_06;
    u_arg.rx_cbuf = uart_cbuf;
    u_arg.rx_cbuf_size = 512;
    u_arg.frame_length = 324;
    u_arg.rx_timeout = 100;
    u_arg.isr_cbfun = uart_isr_hook;
    u_arg.baud = 921600;//460800;
    u_arg.is_9bit = 0;
#if UART_DEV_FLOW_CTRL
    u_arg.tx_pin = IO_PORTA_00;
    u_arg.rx_pin = IO_PORTA_01;
    u_arg.baud = 1000000;
	extern void flow_ctl_hw_init(void);
	flow_ctl_hw_init();
#endif

	rx_total = 0;

    uart_bus = uart_dev_open(&u_arg);
    if (uart_bus != NULL) {
        printf("uart_dev_open() success\n");
#if (UART_DEV_USAGE_TEST_SEL == 2)
        os_task_create(uart_u_task_handle, (void *)uart_bus, 4, 512, 512, "uart_u_task");
        //task_create(uart_u_task,NULL, "uart_u_task");
#endif
#if UART_DEV_FLOW_CTRL
		os_task_create(uart_flow_ctrl_task, (void *)uart_bus, 31, 128, 0, "flow_ctrl");
#endif
    }

	/*lwrb init*/

}

void uart_receive_task_del(void)
{
	printf("uart_receive_task_del\n");

	os_task_del("uart_u_task");
	if (uart_bus != NULL) {
		uart_dev_close(uart_bus);
		printf("uart_dev_close\n");
	}
	uart_bus = NULL;

	rx_total = 0;
}


#if UART_DEV_FLOW_CTRL
void uart_change_rts_state(void)
{
	static u8 rts_state = 1;
	extern void change_rts_state(u8 state);
	change_rts_state(rts_state);
	rts_state = !rts_state;
}
#endif

#endif
