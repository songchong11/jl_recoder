#include "system/includes.h"
#include "asm/uart_dev.h"
#include "system/event.h"
#include "common/app_common.h"
#include "public.h"
#include "os/os_api.h"


extern lwrb_t receive_buff;

#ifdef USE_LWRB
/* Declare rb instance & raw data */
lwrb_t receive_buff;
//uint8_t buff_data[320 * 200];
uint8_t buff_data[320 * 1];//debug only

#endif

FILE *test_file = NULL;

static OS_MUTEX  m_r, m_w;
//#define FRAME_LEN (324 * 3)
#define FRAME_LEN (324)

struct sys_time g_time;

const char root_path[] = "storage/sd0/C/";
char file_path[80] = {0};
char file_path_head[80] = {0};
char year_month_day[20] = {0};
char hour_min_sec[20] = {0};

#if WAV_FORMAT
/**********************************WAV*******************************************/
/*****************************************************************/
// WAV format
/*****************************************************************/

typedef struct WAVE_HEADER{
	char	fccID[4];		//内容为"RIFF"
	unsigned int dwSize;   //最后填写，WAVE格式音频的大小
	char	fccType[4]; 	//内容为"WAVE"
}WAVE_HEADER;

typedef struct WAVE_FMT{
	char	fccID[4];		   //内容为"fmt "
	unsigned int  dwSize;	  //内容为WAVE_FMT占的字节数，为16
	short int wFormatTag; //如果为PCM，改值为 1
	short int wChannels;  //通道数，单通道=1，双通道=2
	unsigned int  dwSamplesPerSec;//采样频率
	unsigned int  dwAvgBytesPerSec;/* ==dwSamplesPerSec*wChannels*uiBitsPerSample/8 */
	short int wBlockAlign;//==wChannels*uiBitsPerSample/8
	short int uiBitsPerSample;//每个采样点的bit数，8bits=8, 16bits=16
}WAVE_FMT;

typedef struct WAVE_DATA{
	char	fccID[4];		//内容为"data"
	unsigned int dwSize;   //==NumSamples*wChannels*uiBitsPerSample/8
}WAVE_DATA;



/**
 * Convert PCM raw data to WAVE format
 * @param pcmpath       Input PCM file.
 * @param channels      Channel number of PCM file.
 * @param sample_rate   Sample rate of PCM file.
 * @param wavepath      Output WAVE file.
 */
#if 1
u16 transform_pcm_timer = 0;
FILE *fpout;
void pcm_2_wav(void *priv)
{
	u8 pcm_data[1024];

	int r_ret = fread(test_file, pcm_data, sizeof(pcm_data));

	wdt_clear();
	int w_ret = fwrite(fpout, pcm_data, r_ret);
	if (r_ret != w_ret)
		printf("wav write error\n");
	wdt_clear();

	led_blue_toggle();

	if (r_ret < sizeof(pcm_data)) {
		printf("last packet\n");// TODO:check

		fdelete(test_file);

		//fclose(test_file);
		test_file = NULL;
		fclose(fpout);
		fpout = NULL;

		led_blue_off();

		sys_timer_del(transform_pcm_timer);
		transform_pcm_timer = 0;
	}

}

int transform_pcm_to_wave(void)
{

		WAVE_HEADER pcmHEADER;
		WAVE_FMT	pcmFMT;
		WAVE_DATA	pcmDATA;


		test_file = fopen(file_path, "r");
		if (!test_file) {
			printf("fopen file faild!\n");
		}
		memset(file_path, 0, sizeof(file_path));

		strcat(file_path_head, ".wav");
		fpout = fopen(file_path_head, "wb+");
		if(fpout==NULL)
		{
			printf("Create wav file error.\n");
			return -1;
		}

		/* WAVE_HEADER */
		memcpy(pcmHEADER.fccID, "RIFF", 4);
		memcpy(pcmHEADER.fccType, "WAVE", 4);

		//fseek(fpout, sizeof(WAVE_HEADER), 1);	//1=SEEK_CUR
		/* WAVE_FMT */
		memcpy(pcmFMT.fccID, "fmt ", 4);
		pcmFMT.dwSize = 16;
		pcmFMT.wFormatTag = 0x0001;
		pcmFMT.wChannels = 1;
		pcmFMT.dwSamplesPerSec = 16000;
		pcmFMT.uiBitsPerSample = 16;
		/* ==dwSamplesPerSec*wChannels*uiBitsPerSample/8 */
		pcmFMT.dwAvgBytesPerSec = pcmFMT.dwSamplesPerSec*pcmFMT.wChannels*pcmFMT.uiBitsPerSample/8;
		/* ==wChannels*uiBitsPerSample/8 */
		pcmFMT.wBlockAlign = pcmFMT.wChannels*pcmFMT.uiBitsPerSample/8;

		/* WAVE_DATA */
		memcpy(pcmDATA.fccID, "data", 4);

		pcmDATA.dwSize = flen(test_file);//get pcm file len
		pcmHEADER.dwSize = 36 + pcmDATA.dwSize;
		printf("pcm file len %d.\n", pcmDATA.dwSize);

		fwrite(fpout, &pcmHEADER, sizeof(WAVE_HEADER));
		fwrite(fpout, &pcmFMT, sizeof(WAVE_FMT));
		fwrite(fpout, &pcmDATA, sizeof(WAVE_DATA));

		transform_pcm_timer = sys_timer_add(NULL, pcm_2_wav, 1);

		wdt_clear();

		return 0;
}

#endif



/*****************************************************************************/
#endif


#if 1
/*
    [[  注意!!!  ]]
    * 如果当系统任务较少时使用本demo，需要将低功耗关闭（#define TCFG_LOWPOWER_LOWPOWER_SEL    0//SLEEP_EN ），否则任务被串口接收函数调用信号量pend时会导致cpu休眠，串口中断和DMA接收将遗漏数据或数据不正确
*/

#define UART_DEV_USAGE_TEST_SEL         1       //uart_dev.c api接口使用方法选择
//  选择1  串口中断回调函数推送事件，由事件响应函数接收串口数据
//  选择2  由task接收串口数据

#define UART_DEV_TEST_MULTI_BYTE        1       //uart_dev.c 读写多个字节api / 读写1个字节api 选择

#define UART_DEV_FLOW_CTRL				0

static u8 uart_cbuf[1024] __attribute__((aligned(4)));
static u8 uart_rxbuf[1024] __attribute__((aligned(4)));

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



//extern lwrb_t receive_buff;
u32 rx_total = 0;
u8 led_cnt;


#if 0
static void uart_u_task_handle(void *arg)
{
		const uart_bus_t *uart_bus = arg;
		int ret;
		u32 uart_rxcnt = 0;
		int i;

		printf("uart_at_task start\n");
		while (1) {

			//uart_bus->read()在尚未收到串口数据时会pend信号量，挂起task，直到UART_RX_PND或UART_RX_OT_PND中断发生，post信号量，唤醒task
			uart_rxcnt = uart_bus->read(uart_rxbuf, sizeof(uart_rxbuf), 0);

			if (uart_rxcnt != 324) {
				printf("\r\nerror------------------------------------------ %d\n", uart_rxcnt);
			}

			if (uart_rxcnt && recoder.recoder_state && \
					uart_rxbuf[0] == 0xcd && uart_rxbuf[1] == 0xab &&\
					uart_rxbuf[2] == 0x40 && uart_rxbuf[3] == 0x01) {


				if ((rx_total % 100) == 0)// bilnk green led when recoder
					led_green_toggle();

				rx_total++;
#if 1
#if ENCODER_ENABLE
		if(test_file && recoder.recoder_state) {

			/*320byte input, 320 / 4 = 80 byte*/
			encode(&mg, (s16 *)&uart_rxbuf[4], 160, micEncodebuf);
			ret = fwrite(test_file, micEncodebuf, sizeof(micEncodebuf));
			if (ret != sizeof(micEncodebuf)) {
				printf(" file write buf err %d\n", ret);
				fclose(test_file);
				test_file = NULL;
			}
		}
#else
		led_blue_on();
		if(test_file && recoder.recoder_state) {

			ret = fwrite(test_file, &uart_rxbuf[4], uart_rxcnt - 4);
			if (ret != (uart_rxcnt - 4)) {
				printf(" file write buf err %d\n", ret);
				fclose(test_file);
				test_file = NULL;
			}
		}
		led_blue_off();
#endif
#endif

#if USE_LWRB
				/*push receive data to fifo*/
				ret = lwrb_is_ready(&receive_buff);

				int	n_written = lwrb_write(&receive_buff, &uart_rxbuf[4], uart_rxcnt - 4);
				if (n_written != uart_rxcnt - 4) {
					printf("write lwrb buffer error %d %d\n", n_written, uart_rxcnt);

				} else {
					os_taskq_post_msg("file_write", 1, APP_USER_MSG_BUFFER_HAVE_DATA);
				}
#endif

			}
		}
}

#endif


/////下面函数调用的使用函数都必须放在ram
___interrupt
AT_VOLATILE_RAM_CODE

static void uart_u_task_handle(void *arg)
{
		int ret;
		//int msg[Q_USER_DATA_SIZE + 1] = {0, 0, 0, 0, 0, 0, 0, 0, 00, 0};
		int msg[200 + 1] = {0, 0, 0, 0, 0, 0, 0, 0, 00, 0};
		int n_read;
		uint8_t* data;

		while (1) {
			ret = os_task_pend("taskq", msg, ARRAY_SIZE(msg));
			//app_task_get_msg(int * msg, int msg_size, int block)
			if (ret != OS_TASKQ) {
				continue;
			}
			//put_buf(msg, (Q_USER_DATA_SIZE + 1) * 4);
			if (msg[0] == Q_MSG) {
				//printf("use os_taskq_post_msg");
				switch (msg[1]) {
					case APP_USER_MSG_BUFFER_HAVE_DATA:
				#if USE_LWRB
					if(test_file && recoder.recoder_state) {
								//int n_bytes_in_queue = lwrb_get_full(&receive_buff);

								//while(n_bytes_in_queue) {
								os_mutex_pend(&m_r, 0);
								#if 0
								while ((n_read  = lwrb_read(&receive_buff, write_buffer, sizeof(write_buffer))) > 0) {
								//while(lwrb_get_linear_block_read_address(&receive_buff)) {

									//n_read = lwrb_read(&receive_buff, write_buffer, sizeof(write_buffer));
									led_blue_on();
									ret = fwrite(test_file, write_buffer, n_read);
									if (ret != n_read) {
										printf(" file write buf err %d\n", ret);
										fclose(test_file);
										test_file = NULL;
									}
									led_blue_off();
								}
								#endif
								if ((n_read = lwrb_get_linear_block_read_length(&receive_buff)) > 0) {
									/* Get pointer to first element in linear block at read address */
									data = lwrb_get_linear_block_read_address(&receive_buff);
									ret = fwrite(test_file, data, n_read);
									if (ret != n_read) {
										printf(" file write buf err %d\n", ret);
										fclose(test_file);
										test_file = NULL;
									}

									/* Now skip sent bytes from buffer = move read pointer */
									lwrb_skip(&receive_buff, ret);

								}
								os_mutex_post(&m_r);
					}
					#endif
						break;

					case APP_USER_MSG_START_RECODER:
						printf("APP_USER_MSG_START_RECODER");
#if 1

					#if USE_LWRB
						/*lwrb init*/
						ret = lwrb_init(&receive_buff, buff_data, sizeof(buff_data));
						if(!ret) {
							printf("lwrb_init fail!!! \n");
							return;
						}
						ret = lwrb_is_ready(&receive_buff);
						if(!ret) {
							printf("lwrb ready fail!!! \n");
							return;
						}
					#endif
						get_sys_time(&g_time);
						printf("now_time : %d-%d-%d,%d:%d:%d\n", g_time.year, g_time.month, g_time.day, g_time.hour, g_time.min, g_time.sec);

						sprintf(year_month_day, "%d%02d%02d", g_time.year, g_time.month, g_time.day);
						printf("year_month_day:%s\n", year_month_day);

						sprintf(hour_min_sec, "%02d%02d%02d", g_time.hour, g_time.min, g_time.sec);
						printf("hour_min_sec:%s\n", hour_min_sec);

						strcat(file_path, root_path);
						strcat(file_path, year_month_day);
						strcat(file_path, "/");
						strcat(file_path, hour_min_sec);
#if WAV_FORMAT
						memset(file_path_head, 0x00, sizeof(file_path_head));
						memcpy(file_path_head, file_path, sizeof(file_path));
#endif
						strcat(file_path, ".pcm");
						printf("file_path:%s\n", file_path);
						if (!test_file) {
							test_file = fopen(file_path, "w+");

#if WAV_FORMAT

#else
							memset(file_path, 0, sizeof(file_path));
#endif
							if (!test_file) {
								printf("fopen file faild!\n");
							} else {
								printf("open file succed\r\n");
							}
						}
						fseek(test_file, 0, SEEK_SET);

						bes_start_recoder();
#endif
						break;
					case APP_USER_MSG_STOP_RECODER:
							printf("APP_USER_MSG_STOP_RECODER");

							if (test_file) {
#if WAV_FORMAT
								fclose(test_file);
								test_file = NULL;
								transform_pcm_to_wave();
#else
								fclose(test_file);
								test_file = NULL;
#endif

							}

							lwrb_free(&receive_buff);
						break;
					default:
						break;
			   }
			}
	   }

}


static void uart_isr_hook(void *arg, u32 status)
{
    const uart_bus_t *uart_bus = arg;
	u32 uart_rxcnt = 0;
	int ret;
	int i;

    //当CONFIG_UARTx_ENABLE_TX_DMA（x = 0, 1）为1时，不要在中断里面调用ubus->write()，因为中断不能pend信号量
    if (status == UT_RX) {
    //    printf("rx_isr\n");

			uart_rxcnt = uart_bus->read(uart_rxbuf, sizeof(uart_rxbuf), 0);

			if (uart_rxcnt != FRAME_LEN) {
				printf("\r\nerror------------------------------------------ %d\n", uart_rxcnt);
				for(i = 0; i < uart_rxcnt; i++)
						 putchar(uart_rxbuf[i]);
			}

			if (uart_rxcnt && recoder.recoder_state && \
					uart_rxbuf[0] == 0xcd && uart_rxbuf[1] == 0xab &&\
					uart_rxbuf[2] == 0x40 && uart_rxbuf[3] == 0x01) {

				if ((rx_total % 100) == 0)// bilnk green led when recoder
					led_blue_toggle();

				rx_total++;

#if USE_LWRB
				os_mutex_pend(&m_w, 0);
				/*push receive data to fifo*/
				int n_written = lwrb_write(&receive_buff, (void *)&uart_rxbuf[4], uart_rxcnt - 4);
				//int n_written = lwrb_write(&receive_buff, uart_rxbuf, uart_rxcnt);
				if (n_written != uart_rxcnt - 4) {
					printf("lwrb---%d\n", lwrb_get_full(&receive_buff));

				} else {
					os_taskq_post_msg("uart_u_task", 1, APP_USER_MSG_BUFFER_HAVE_DATA);
				}
				os_mutex_post(&m_w);
#endif
		}

    } else if(status == UT_RX_OT) {
   	//intf("ot_isr\n");

		uart_rxcnt = uart_bus->read(uart_rxbuf, sizeof(uart_rxbuf), 0);

		if (uart_rxcnt != FRAME_LEN) {
			printf("\r\nerror------------------------------------------ %d\n", uart_rxcnt);
			for(i = 0; i < uart_rxcnt; i++)
				putchar(uart_rxbuf[i]);
		}

		if (uart_rxcnt && recoder.recoder_state && \
				uart_rxbuf[0] == 0xcd && uart_rxbuf[1] == 0xab &&\
				uart_rxbuf[2] == 0x40 && uart_rxbuf[3] == 0x01) {


			if ((rx_total % 100) == 0)// bilnk green led when recoder
				led_blue_toggle();

			rx_total++;

#if USE_LWRB
			os_mutex_pend(&m_w, 0);
			/*push receive data to fifo*/
			int n_written = lwrb_write(&receive_buff, (void *)&uart_rxbuf[4], uart_rxcnt - 4);
			//int n_written = lwrb_write(&receive_buff, uart_rxbuf, uart_rxcnt);
			if (n_written != uart_rxcnt - 4) {
				printf("lwrb---%d\n", lwrb_get_full(&receive_buff));

			} else {
				os_taskq_post_msg("uart_u_task", 1, APP_USER_MSG_BUFFER_HAVE_DATA);
			}

			os_mutex_post(&m_w);

#endif

		}

    }

}


const uart_bus_t *uart_bus;

void uart_dev_receive_init()
{
    struct uart_platform_data_t u_arg = {0};
    u_arg.tx_pin = IO_PORTA_05;
    u_arg.rx_pin = IO_PORTA_06;
    u_arg.rx_cbuf = uart_cbuf;
    u_arg.rx_cbuf_size = 1024;
    u_arg.frame_length = FRAME_LEN;
    u_arg.rx_timeout = 50;
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
	adpcm_init();

    uart_bus = uart_dev_open(&u_arg);
    if (uart_bus != NULL) {
        printf("uart_dev_open() success\n");
#if 0
#if (UART_DEV_USAGE_TEST_SEL == 2)
        os_task_create(uart_u_task_handle, (void *)uart_bus, 4, 512, 512, "uart_u_task");
        //task_create(uart_u_task,NULL, "uart_u_task");
#endif
#endif
		os_mutex_create(&m_w);
		os_mutex_create(&m_r);


		task_create(uart_u_task_handle,NULL, "uart_u_task");
    }
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



void clear_rx_buffer(void)
{
	memset(uart_rxbuf, 0x00, sizeof(uart_rxbuf));
	rx_total = 0;
}
#endif
