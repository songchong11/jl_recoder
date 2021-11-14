#include "system/includes.h"
#include "asm/uart_dev.h"
#include "system/event.h"
#include <stdarg.h>

#if 1
//中断缓存串口数据
#define UART_BUFF_SIZE      512



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
static u8 uart_txbuf[52] __attribute__((aligned(4)));

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
static void uart_event_4g_at_handler(struct sys_event *e)
{
    const uart_bus_t *uart_bus;
    u32 uart_rxcnt = 0;

    if ((u32)e->arg == DEVICE_EVENT_FROM_UART_RX_OVERFLOW) {
        if (e->u.dev.event == DEVICE_EVENT_CHANGE) {
            /* printf("uart event: DEVICE_EVENT_FROM_UART_RX_OVERFLOW\n"); */
            uart_bus = (const uart_bus_t *)e->u.dev.value;
            uart_rxcnt = uart_bus->read(uart_rxbuf, sizeof(uart_rxbuf), 0);
            if (uart_rxcnt) {
                printf("get_buffer-------:\n");
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
                printf("get_buffer:\n");
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
SYS_EVENT_HANDLER(SYS_DEVICE_EVENT, uart_event_4g_at_handler, 0);

static FILE *test_file = NULL;
u32 uart_rxcnt = 0;
static u32 received_len = 0;
static u8 received_buffer[320];

static void uart_at_task_handle(void *arg)
{
    const uart_bus_t *uart_bus = arg;
    int ret;

    while (1) {
        //uart_bus->read()在尚未收到串口数据时会pend信号量，挂起task，直到UART_RX_PND或UART_RX_OT_PND中断发生，post信号量，唤醒task
        uart_rxcnt = uart_bus->read(uart_rxbuf, sizeof(uart_rxbuf), 0);

		//printf("----------------------------------uart_rxcnt %d\n", uart_rxcnt);
        if (uart_rxcnt >= 4) { //some time , it maybe received 1 byte data, it no use

			received_len = uart_rxcnt;
			memcpy(received_buffer, uart_rxbuf, received_len);

            for (int i = 0; i < uart_rxcnt; i++)
				printf("%c", uart_rxbuf[i]);
        }

    }
}

static void uart_isr_hook(void *arg, u32 status)
{
    const uart_bus_t *ubus = arg;
    struct sys_event e;

    //当CONFIG_UARTx_ENABLE_TX_DMA（x = 0, 1）为1时，不要在中断里面调用ubus->write()，因为中断不能pend信号量
    if (status == UT_RX) {
       // printf("uart_rx_isr--------\n");
#if (UART_DEV_USAGE_TEST_SEL == 1)
        e.type = SYS_DEVICE_EVENT;
        e.arg = (void *)DEVICE_EVENT_FROM_UART_RX_OVERFLOW;
        e.u.dev.event = DEVICE_EVENT_CHANGE;
        e.u.dev.value = (int)ubus;
        sys_event_notify(&e);
#endif
    }
    if (status == UT_RX_OT) {
       // printf("uart_rx_ot_isr++++\n");
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

static const uart_bus_t *uart_bus;

void uart_dev_4g_at_init()
{
    struct uart_platform_data_t u_arg = {0};
    u_arg.tx_pin = IO_PORTA_00;
    u_arg.rx_pin = IO_PORTA_01;
    u_arg.rx_cbuf = uart_cbuf;
    u_arg.rx_cbuf_size = 512;
    u_arg.frame_length = 32;
    u_arg.rx_timeout = 100;
    u_arg.isr_cbfun = uart_isr_hook;
    u_arg.baud = 115200;
    u_arg.is_9bit = 0;
#if UART_DEV_FLOW_CTRL
    u_arg.tx_pin = IO_PORTA_00;
    u_arg.rx_pin = IO_PORTA_01;
    u_arg.baud = 115200//1000000;
	extern void flow_ctl_hw_init(void);
	flow_ctl_hw_init();
#endif
    uart_bus = uart_dev_open(&u_arg);
    if (uart_bus != NULL) {
        printf("4G AT uart_dev_open() success\n");
#if (UART_DEV_USAGE_TEST_SEL == 2)
        os_task_create(uart_at_task_handle, (void *)uart_bus, 31, 512, 0, "uart_at_task");
#endif
#if UART_DEV_FLOW_CTRL
		os_task_create(uart_flow_ctrl_task, (void *)uart_bus, 31, 128, 0, "flow_ctrl");
#endif
    }
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


/*
 * 函数名：itoa
 * 描述  ：将整形数据转换成字符串
 * 输入  ：-radix =10 表示10进制，其他结果为0
 *         -value 要转换的整形数
 *         -buf 转换后的字符串
 *         -radix = 10
 * 输出  ：无
 * 返回  ：无
 * 调用  ：被GSM_USARTx_printf()调用
 */
static char *itoa(int value, char *string, int radix)
{
    int     i, d;
    int     flag = 0;
    char    *ptr = string;

    /* This implementation only works for decimal numbers. */
    if (radix != 10)
    {
        *ptr = 0;
        return string;
    }

    if (!value)
    {
        *ptr++ = 0x30;
        *ptr = 0;
        return string;
    }

    /* if this is a negative value insert the minus sign. */
    if (value < 0)
    {
        *ptr++ = '-';

        /* Make the value positive. */
        value *= -1;
    }

    for (i = 10000; i > 0; i /= 10)
    {
        d = value / i;

        if (d || flag)
        {
            *ptr++ = (char)(d + 0x30);
            value -= (d * i);
            flag = 1;
        }
    }

    /* Null terminate the string. */
    *ptr = 0;

    return string;

} /* NCL_Itoa */


#if 1


//获取接收到的数据和长度
char *get_rebuff(uint8_t *len)
{
    //*len = uart_rxcnt;
    //return (char *)&uart_rxbuf;

	*len = received_len;

#if 0
	printf("get bufffer %p -------------------\n", received_buffer);

	for (int i = 0; i < uart_rxcnt; i++) {
		printf("%c", uart_rxbuf[i]);
		//my_put_u8hex(uart_rxbuf[i]);
		if (i % 16 == 15) {
			putchar('\n');
		}
	}
	printf("\n");
#endif

    return received_buffer;
}

void clean_rebuff(void)
{
	//printf("clear rebuff\n");
    uart_rxcnt = 0;
	received_len = 0;

	memset(uart_rxbuf, 0x00, sizeof(uart_rxbuf));
	memset(received_buffer, 0x00, sizeof(received_buffer));
}

#endif

/*
 * 函数名：GSM_USARTx_printf
 * 描述  ：格式化输出，类似于C库中的printf，但这里没有用到C库
 * 输入  ：-USARTx 串口通道，这里只用到了串口2，即GSM_USARTx
 *		     -Data   要发送到串口的内容的指针
 *			   -...    其他参数
 * 输出  ：无
 * 返回  ：无 
 * 调用  ：外部调用
 *         典型应用GSM_USARTx_printf( GSM_USARTx, "\r\n this is a demo \r\n" );
 *            		 GSM_USARTx_printf( GSM_USARTx, "\r\n %d \r\n", i );
 *            		 GSM_USARTx_printf( GSM_USARTx, "\r\n %s \r\n", j );
 */
void GSM_USART_printf(char *Data,...)
{
	const char *s;
  int d;   
  char buf[16];

  va_list ap;
  va_start(ap, Data);

	while ( *Data != 0)     // 判断是否到达字符串结束符
	{				                          
		if ( *Data == 0x5c )  //'\'
		{									  
			switch ( *++Data )
			{
				case 'r':							          //回车符
					uart_txbuf[0] = 0x0d;
					uart_bus->write(uart_txbuf, 1);
					//USART_SendData(GSM_USARTx, 0x0d);
					Data ++;
					break;

				case 'n':							          //换行符
					uart_txbuf[0] = 0x0a;
					uart_bus->write(uart_txbuf, 1);
					//USART_SendData(GSM_USARTx, 0x0a);	
					Data ++;
					break;
				
				default:
					Data ++;
				    break;
			}			 
		}
		else if ( *Data == '%')
		{									  //
			switch ( *++Data )
			{				
				case 's':										  //字符串
					s = va_arg(ap, const char *);
          for ( ; *s; s++) 
					{
						//USART_SendData(GSM_USARTx,*s);
						uart_txbuf[0] = *s;
						uart_bus->write(uart_txbuf, 1);
						//while( USART_GetFlagStatus(GSM_USARTx, USART_FLAG_TXE) == RESET );
          }
					Data++;
          break;

        case 'd':										//十进制
          d = va_arg(ap, int);
          itoa(d, buf, 10);
          for (s = buf; *s; s++) 
					{
						//USART_SendData(GSM_USARTx,*s);
						uart_txbuf[0] = *s;
						uart_bus->write(uart_txbuf, 1);
						//while( USART_GetFlagStatus(GSM_USARTx, USART_FLAG_TXE) == RESET );
          }
					Data++;
          break;
				 default:
						Data++;
				    break;
			}		 
		} /* end of else if */
		//else USART_SendData(GSM_USARTx, *Data++);
		else {
			uart_txbuf[0] = *Data++;
			uart_bus->write(uart_txbuf, 1);
		}
		//while( USART_GetFlagStatus(GSM_USARTx, USART_FLAG_TXE) == RESET );
	}
}

void gsm_send_buffer(u8 *buf, int len)
{
	if(uart_bus != NULL)
		uart_bus->write(buf, len);
}

#endif
