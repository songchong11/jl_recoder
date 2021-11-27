#include "includes.h"
#include "system/includes.h"
#include "asm/uart_dev.h"
#include "system/event.h"
#include "common/app_common.h"
#include "lwrb.h"
#include "at_4g_driver.h"
#include "led_driver.h"
#include "stdlib.h"
#include "syscfg_id.h"

#define DEBUG_FILE_SYS	1

#define MODULE_PWR_GPIO		IO_PORTB_03
#define SIM_CARD_TYPE	CTNET//CMNET

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

#define POWER_ON	1
#define POWER_OFF	2

int file_send_timer;
extern void gsm_send_buffer(u8 *buf, int len);
uint8_t read_buffer[320];
#define READ_LEN	320

static u8 module_status ;
#if 1

void module_4g_gpio_init(void)
{
	printf("module_4g_gpio_init\r\n");
	gpio_set_pull_up(MODULE_PWR_GPIO, 0);
	gpio_set_pull_down(MODULE_PWR_GPIO, 0);
	gpio_set_die(MODULE_PWR_GPIO, 0);
	gpio_set_direction(MODULE_PWR_GPIO, 0);
	gpio_set_output_value(MODULE_PWR_GPIO, 0);
	module_status = POWER_OFF;
}

void module_power_on(void)
{
	if (module_status == POWER_OFF) {

		printf("4g module power on\r\n");

		gpio_set_output_value(MODULE_PWR_GPIO, 1);
		wdt_clear();
		delay_2ms(1500);//need more over 2s
		gpio_set_output_value(MODULE_PWR_GPIO, 0);

		wdt_clear();
		delay_2ms(1500);//delay 3s
		wdt_clear();

		module_status = POWER_ON;
	}
}

void module_power_off(void)
{

	//if (module_status == POWER_ON) {

		printf("4g module power off\r\n");
		gpio_set_output_value(MODULE_PWR_GPIO, 1);

		wdt_clear();
		delay_2ms(2000);//need more over 3.1s
		wdt_clear();
		gpio_set_output_value(MODULE_PWR_GPIO, 0);

		module_status = POWER_OFF;
	//}

}

u8 tmp_dir_name[10];
u8 tmp_file_name[10];
extern bool send_pcm_state;
u8 gsn_str[30];

extern bool scan_sd_card_before_get_path(void);
extern void release_all_fs_source(void);
int rename_file_when_send_over(FILE* fs, char *file_name);
static void at_4g_task_handle(void *arg)
{
    int ret;
    int result;
	u8 retry;


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
					if (module_status == POWER_OFF) {

					#if DEBUG_FILE_SYS
						module_power_on();
						while(gsm_init_to_access_mode() == GSM_FALSE) {
							retry++;
							module_power_off();
							wdt_clear();
							delay_2ms(2000);
							wdt_clear();
							module_power_on();
							if(retry > 3) {
								ret = false;
								break;
							}
						}

						ret = true;
					#endif
					}

					if (ret) {

						printf("gsm enter into access mode success\n");
						os_taskq_post_msg("at_4g_task", 1, APP_USER_MSG_GET_NEXT_FILE);

					} else {

						printf("gsm init faild\n");
						os_taskq_post_msg("at_4g_task", 1, APP_USER_MSG_GSM_FAIL);
					}

	                break;

				case APP_USER_MSG_GET_NEXT_FILE:
					printf("APP_USER_MSG_GET_NEXT_FILE");

					memset(tmp_dir_name, 0x00, sizeof(tmp_dir_name));
					memset(tmp_file_name, 0x00, sizeof(tmp_file_name));

					ret = scan_sd_card_before_get_path();

					if (ret) {

						ret = get_recoder_file_path(tmp_dir_name, tmp_file_name);

						if (ret) {

							printf("find a file to send %s/%s", tmp_dir_name, tmp_file_name);

							file_read_from_sd_card(tmp_dir_name, tmp_file_name);

						} else {
							printf("no file to send \n");
							os_taskq_post_msg("at_4g_task", 1, APP_USER_MSG_SEND_FILE_OVER);
						}

					} else
						os_taskq_post_msg("at_4g_task", 1, APP_USER_MSG_SEND_FILE_OVER);



					break;

				case APP_USER_MSG_STOP_SEND_FILE_TO_AT:
				case APP_USER_MSG_SEND_FILE_OVER:

					sys_timer_del(file_send_timer);
					file_send_timer = 0;

					gsm_send_buffer(read_buffer, ret);//send last packet

					printf("user active stop send!!!\r\n");

					send_end_packet();

					release_all_fs_source();
					send_pcm_state = false;

					memset(tmp_dir_name, 0x00, sizeof(tmp_dir_name));
					memset(tmp_file_name, 0x00, sizeof(tmp_file_name));

					printf("send file over, close 4G module\n");
#if DEBUG_FILE_SYS
					/*power off 4g module */
					clsoe_tcp_link();
					module_power_off();
#endif
					led_blue_off();
					break;

				case APP_USER_MSG_SYNC_TIME:
					gsm_sync_time_from_net();
					break;

				case APP_USER_MSG_GSM_ERROR:
					 if(msg[2] == NO_SIM_CARD) {
						printf("NO sim card\n");
					 } else if(msg[2] == NET_ERROR) {
						printf("gsm internet is error\n");
					 } else if (msg[2] == NO_SD_CARD){
						 printf("NO sd card\n");

					 }
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
}


void at_4g_task_del(void)
{
    printf("at_4g_task_del\n");

    os_task_del("at_4g_task");
}
#endif

//////////////////////////////////////////////////////////////////////////////////////

static u32 packet_num = 0;

void file_read_and_send(void *priv)
{
	FILE *read_p = (FILE*)priv;

	if(!read_p) {
		printf("file open error!!!\r\n");
		return;
	}

	if((packet_num % 5) == 0)
		led_blue_toggle();

	int ret = fread(read_p, read_buffer, READ_LEN);

	if(ret == READ_LEN) {
		packet_num++;
		printf("s%d", packet_num);
		gsm_send_buffer(read_buffer, READ_LEN);

	} else {

		sys_timer_del(file_send_timer);
		file_send_timer = 0;

		gsm_send_buffer(read_buffer, ret);//send last packet

		printf("send over, close file!!!\r\n");

		send_end_packet();

		printf("send a msg to get next file\r\n");

		delay_2ms(200);
		ret = rename_file_when_send_over(read_p, tmp_file_name);
		if (ret) {
			fclose(read_p);
			release_all_fs_source();
			os_taskq_post_msg("at_4g_task", 1, APP_USER_MSG_GET_NEXT_FILE);
		} else {
			fclose(read_p);
			release_all_fs_source();
			printf("rename fail, stop send\n");
			os_taskq_post_msg("at_4g_task", 1, APP_USER_MSG_SEND_FILE_OVER);
		}


	}

}



int rename_file_when_send_over(FILE* fs, char *file_name)
{
	char rename[15] = {0};
	int ret;
	int result;

	sprintf(rename, "%s%s", "s", file_name);

	printf("new name is %s\n", rename);

	if (fs) {

		printf("open file successd\r\n");

		ret = frename(fs, rename);
		if (ret) {
			printf("rename fail\n");
			ret = false;
		} else {
			printf("rename ok\n");
			ret = true;
		}

	} else {
		printf("open file error\r\n");
		ret = false;
	}

	return ret;
}

//----------------------------------------------------------


static FILE *fp = NULL;
bool file_read_from_sd_card(u8 *dir, u8 *file_name)
{
	u8 tmp_path[40];

	sprintf(tmp_path, "%s%s/%s", "storage/sd0/C/",dir, file_name);

	printf("read path:%s\n", tmp_path);

	//fp = fopen("storage/sd0/C/20211130/231521.MP3","rb");
	fp = fopen(tmp_path,"rb");

	if (fp) {

		printf("open file successd, send the start packet\n");

		u32 file_len =	flen(fp);
		printf("this file len = %x\n", file_len);

		send_the_start_packet(file_name, dir, file_len);
	}

	else {
		printf("open file error\r\n");
		return false;
	}

	if (file_send_timer == 0) {
		packet_num = 0;
		file_send_timer = sys_timer_add(fp, file_read_and_send, 30);
	}

	fp = NULL;

	return true;
}




//====startdata====年月日时分秒===设备号===文件大小===电量===内存总大小===内存使用量===内存剩余量===文件名称===路径
//====enddata====
#if 1 //sn:867366051072616
extern void get_sys_time(struct sys_time *time);
extern u8 get_vbat_percent(void);

void send_the_start_packet(char * filename, char* dir_name, u32 size)
{
	u8 start[320];
	u8 year_month_day[14] = {0};
	struct sys_time _time;
	//u8 imei[16] = "123456789abcdef\0";
	u32 file_size;
	u8 battery;
	u32 mem_size = 32*1024;
	u32 mem_use = 16*1024;
	u32 mem_left = 16*1024;
	u8 path[30];

	file_size = size;

	battery = get_vbat_percent();

	sprintf(path, "%s/%s", dir_name, filename);
	printf("path:%s\n", path);

	get_sys_time(&_time);

	sprintf(year_month_day, "%d%02d%02d", _time.year, _time.month, _time.day);
	printf("year_month_day:%s\n", year_month_day);

	sprintf(start, "%s%d%02d%02d%02d%02d%02d%s%s%s%x%s%x%s%x%s%x%s%x%s%s%s%s%s", \
							"====startdata====", \
			_time.year, _time.month, _time.day, _time.hour, _time.min, _time.sec, \
			"===", gsn_str, "===", file_size,  "===", battery, "===", mem_size, \
			"===", mem_use, "===", mem_left, "===", filename, "===", path, "====enddata====");

	printf("\n");

	printf("%s\n", start);

	int len = strlen(start);
	printf("len = %d \n", len);
	packet_num = 0;
	gsm_send_buffer(start, len);

}

void send_end_packet(void)
{
	u8 data[12];
	sprintf(data, "%s", "====end====");

	packet_num = 0;
	printf("%s\n", data);

	gsm_send_buffer(data, sizeof(data));

	led_blue_off();
}
#endif
//////////////////////////////////////////////////////////////////////////////////////
static uint8_t MaxMessAdd=50;


//0表示成功，1表示失败

uint8_t gsm_cmd(char *cmd, char *reply, uint32_t waittime)
{
	uint8_t ret;

    GSM_CLEAN_RX();                 //清空了接收缓冲区数据

    GSM_TX(cmd);                    //发送命令

	GSM_DEBUG("Send cmd:%s",cmd);

    if(reply == 0)                      //不需要接收数据
    {
        return GSM_TRUE;
    }

	if(waittime > 1000) {

		for (int i = 0; i < (waittime /1000); i++)
		{
			wdt_clear();
			GSM_DELAY(1000);				 //延时
			wdt_clear();
			ret = gsm_cmd_check(reply);
			if (ret == GSM_TRUE || ret == GSM_CMD_ERROR)
				 return ret;
		}

	} else {

			wdt_clear();
			GSM_DELAY(waittime);				 //延时
			wdt_clear();
			ret = gsm_cmd_check(reply);	  //对接收数据进行处理
	}

	return ret;
}


//0表示成功，1表示失败
uint8_t gsm_cmd_check(char *reply)
{
    uint8_t len;
    uint8_t n;
    uint8_t off;
    u8 *redata;
	u8 error_code[] = "ERROR";

    redata = GSM_RX(len);   //接收数据

#if 0
	printf(" %p ++++++++++check len = %d\n", redata, len);
	for (int i = 0; i < len; i++) {

		printf("%c", redata[i]);
	}
#endif

	 *(redata+len) = '\0';
	 GSM_DEBUG("Reply:%s",redata);

#if 0
	 if (!memcmp("+GSN",  redata + 2, 4)) {//OD OA

	 	memcpy(gsn_str, redata + 9, 15);
		gsn_str[15] = '\0';
		printf("get GSN : %s\n", gsn_str);
	 }
#endif
    n = 0;
    off = 0;
    while((n + off)<len)
    {
        if(reply[n] == 0)                 //数据为空或者比较完毕
        {
            return GSM_TRUE;
        }

        if(redata[ n + off]== reply[n])
        {
            n++;                //移动到下一个接收数据
        }
        else
        {
            off++;              //进行下一轮匹配
            n=0;                //重来
        }
        //n++;
    }

    if(reply[n]==0)   //刚好匹配完毕
    {
        return GSM_TRUE;
    }

    n = 0;
    off = 0;
    while((n + off)<len) // check weather is ERROR
    {

        if(redata[ n + off]== error_code[n])
        {
            n++;                //移动到下一个接收数据
        }
        else
        {
            off++;              //进行下一轮匹配
            n=0;                //重来
        }
        //n++;
    }

    if(error_code[n]==0)   //刚好匹配完毕
    {
        return GSM_CMD_ERROR;
    }

    return GSM_FALSE;       //跳出循环表示比较完毕后都没有相同的数据，因此跳出
}


//初始化并检测模块,自动注册网络
//0表示成功，1表示失败
uint8_t gsm_init_to_access_mode(void)
{
	u8 retry;
	char *redata;
	uint8_t len;
	uint8_t ret;
	uint8_t error_code;
	u8 sync_ok = 0;

	GSM_DEBUG_FUNC();

	GSM_CLEAN_RX();                 //清空了接收缓冲区数据

	while(gsm_cmd("AT\r","OK", 200) != GSM_TRUE)// AT
	{
		printf("\r\n module not replay AT OK, retry %d\r\n", retry);

		if(++retry > 50) {
			printf("\r\n AT not response！！\r\n");
			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(50);
	retry = 0;

	while(gsm_cmd("ATE0\r","OK", 200) != GSM_TRUE)// 关闭回显
	{
		printf("\r\n ATE0 not replay AT OK, retry %d\r\n", retry);
		if(++retry > 50) {
			printf("\r\n模块响应测试不正常！！\r\n");
			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(50);
	retry = 0;
	while(gsm_cmd("AT+GTSET=\"IPRFMT\",2\r","OK", 200) != GSM_TRUE)//确认PS数据业务可用
	{
		printf("\r\n AT+GTSET? not OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n AT+GTSET? ERROR \r\n");
			error_code = NET_ERROR;
			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(50);
	retry = 0;
	while(gsm_cmd("AT+CFUN=1\r","OK", 200) != GSM_TRUE)// 设置工作模式是正常工作模式
	{
		printf("\r\n AT+CFUN=1 not ok retry %d\r\n", retry);
		if(++retry > 50) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(50);

	retry = 0;
	while(gsm_cmd("AT+CPIN?\r","+CPIN: READY", 1000) != GSM_TRUE)//查询SIM卡
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 50) {
			printf("\r\n未检测到SIM卡\r\n");

			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(1000);
	retry = 0;
	while(gsm_cmd("AT+GSN?\r","+GSN", 200) != GSM_TRUE)//查询SN
	{
		printf("\r\n AT+GSN not replay OK, retry %d\r\n", retry);
		if(++retry > 50) {
			printf("\r\n GET GSN ERROR\r\n");

			goto sms_failure;
		}
	}

	redata = GSM_RX(len);   //接收数据
	printf("GSN %s\n", redata);

	if (!memcmp("+GSN",  redata + 2, 4)) {//OD OA
	
	   memcpy(gsn_str, redata + 9, 15);
	   gsn_str[15] = '\0';
	   printf("get GSN : %s\n", gsn_str);
	}

	wdt_clear();
	GSM_DELAY(50);
	retry = 0;
#if 0
#if  (SIM_CARD_TYPE == CTNET)
		while(gsm_cmd("AT+CGDCONT=1,\"IP\",\"CTNET\"\r","OK", 8000) != GSM_TRUE)// 设置激活时APN。电信
#elif (SIM_CARD_TYPE == CMNET)
		while(gsm_cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"\r","OK", 8000) != GSM_TRUE)// 设置激活时APN。移动
#elif (SIM_CARD_TYPE == TGNET)
		while(gsm_cmd("AT+CGDCONT=1,\"IP\",\"3GNET\"\r","OK", 8000) != GSM_TRUE)// 设置激活时APN。联通
#endif
#endif
	while(gsm_cmd("AT+CGDCONT=1,\"IP\",\"CTNET\"\r","OK", 8000) != GSM_TRUE)// 设置激活时APN。移动

	{
		printf("\r\n AT+CGDCONT= not replay AT OK, retry %d\r\n", retry);

		if(++retry > 2) {
			printf("\r\n模块响应测试不正常！！\r\n");
			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+CSQ?\r","OK", 1000) != GSM_TRUE)//查询信号值
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n CSQ ERROR \r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+COPS?\r","OK", 2000) != GSM_TRUE)//查询注册状态
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n AT+COPS? ERROR \r\n");

			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+CGREG?\r","OK", 3000) != GSM_TRUE)//确认PS数据业务可用
	{
		printf("\r\n AT+CGREG? not OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n AT+CGREG? ERROR \r\n");

			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+CEREG?\r","OK", 3000) != GSM_TRUE)//查询4G状态业务是否可用
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 50) {
			printf("\r\n AT+CEREG? ERROR \r\n");

			goto sms_failure;
		}
	}

	#if 0
	while(gsm_cmd("AT+GTSET=\"IPRFMT\",1\r","OK", 800) != GSM_TRUE)// 设置接收到的数据为原始模式
	{
		printf("\r\n AT+GTSET= not replay AT OK, retry %d\r\n", retry);

		if(++retry > 20) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}
	#endif

	wdt_clear();
	GSM_DELAY(500);
	retry = 0;
#if 0
#if (SIM_CARD_TYPE == CTNET)
	while(gsm_cmd("AT+MIPCALL=1,\"CTNET\"\r","+MIPCALL", 1000 * 150) != GSM_TRUE)// 链接TCP
#elif ((SIM_CARD_TYPE == CMNET))
	while(gsm_cmd("AT+MIPCALL=1,\"CMNET\"\r","+MIPCALL", 1000 * 150) != GSM_TRUE)// 链接TCP

#elif((SIM_CARD_TYPE == TGNET))
	while(gsm_cmd("AT+MIPCALL=1,\"TGNET\"\r","+MIPCALL", 1000 * 150) != GSM_TRUE)// 链接TCP
#endif
#endif
	while(gsm_cmd("AT+MIPCALL=1,\"CTNET\"\r","+MIPCALL", 1000 * 150) != GSM_TRUE)// 链接TCP

	{
		printf("\r\n AT+MIPCALL= not replay AT OK, retry %d\r\n", retry);

		if(++retry > 2) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(50);
	retry = 0;

#if 1
		while(gsm_cmd("AT+MIPOPEN=1,,\"47.113.105.118\",9899,0\r","+MIPOPEN", 1000 * 60) != GSM_TRUE)// 链接TCP
		{
			printf("\r\n AT+MIPOPEN not replay AT OK, retry %d\r\n", retry);
	
			if(++retry > 3) {
				printf("\r\n模块响应测试不正常！！\r\n");
	
				goto sms_failure;
			}
		}
	
		wdt_clear();
		GSM_DELAY(50);
		retry = 0;
	
		while(gsm_cmd("AT+MIPSEND=1,17\r",">", 1000 * 60) != GSM_TRUE)// 链接TCP
		{
			printf("\r\n AT+MIPOPEN not replay AT OK, retry %d\r\n", retry);
	
			if(++retry > 3) {
				printf("\r\n模块响应测试不正常！！\r\n");
	
				goto sms_failure;
			}
		}
	
		wdt_clear();// sync time from ourself server
		if(gsm_cmd("====AT+CCLK?====\r","+MIPRTCP", 1000 * 60) == GSM_TRUE) {
	
			get_and_set_time_form_our_server();
			ret = GSM_TRUE;
			sync_ok = 1;
		}
#endif


	
	#if 0
	while(gsm_cmd("AT+MIPNTP=\"cn.ntp.org.cn\",123\r","+MIPNTP: 1", 1000 * 60) != GSM_TRUE)// 同步时间
	{
		printf("\r\n AT+MIPNTP not replay AT OK, retry %d\r\n", retry);

		if(++retry > 3) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}
#endif
	ret = gsm_cmd("AT+MIPNTP=\"cn.pool.ntp.org\",123\r","+MIPNTP: 1", 1000 * 60);

	if (ret != GSM_TRUE)
		ret = gsm_cmd("AT+MIPNTP=\"cn.ntp.org.cn\",123\r","+MIPNTP: 1", 1000 * 60);

	if (ret != GSM_TRUE)
		ret = gsm_cmd("AT+MIPNTP=\"edu.ntp.org.cn\",123\r","+MIPNTP: 1", 1000 * 60);

	if (ret != GSM_TRUE)
		ret = gsm_cmd("AT+MIPNTP=\"ntp1.aliyun.com\",123\r","+MIPNTP: 1", 1000 * 60);

	if (ret != GSM_TRUE)
		ret = gsm_cmd("AT+MIPNTP=\"ntp2.aliyun.com\",123\r","+MIPNTP: 1", 1000 * 60);

	if (ret != GSM_TRUE) {
		error_code = NET_ERROR;
		goto sms_failure;

	}

	wdt_clear();

	if ((ret == GSM_TRUE) && (sync_ok == 0)) {

		if(gsm_cmd("AT+CCLK?\r","OK", 1000 * 60) == GSM_TRUE) {

			get_and_set_time();
		}

	}


	wdt_clear();
	GSM_DELAY(50);
	retry = 0;
	//while(gsm_cmd("AT+MIPODM=1,,\"47.113.105.118\",9999,0\r","+MIPODM", 1000 * 60) != GSM_TRUE)// 链接TCP
	//while(gsm_cmd("AT+MIPODM=1,,\"47.113.105.118\",9899,0\r","+MIPODM", 1000 * 60) != GSM_TRUE)// 链接TCP
	//while(gsm_cmd("AT+MIPODM=1,,\"record.miclink.net\",9899,0\r","+MIPODM", 1000 * 60) != GSM_TRUE)// 链接TCP
	while(gsm_cmd("AT+MIPODM=1,,\"luyin.heteen.com\",9899,0\r","+MIPODM", 1000 * 60) != GSM_TRUE)// 链接TCP
	{
		printf("\r\n AT+MIPODM= not replay AT OK, retry %d\r\n", retry);

		if(++retry > 3) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}

	printf("\r\n enter into access mode!！\r\n");


	return GSM_TRUE;

	sms_failure:
		printf("\r\n GSM MODULE init fail... \r\n");
		return GSM_FALSE;
}


/*---------------------------------------------------------------------*/

int clsoe_tcp_link(void)
{
	u8 retry;
	char *redata;
	uint8_t len;

	GSM_CLEAN_RX();                 //清空了接收缓冲区数据

	wdt_clear();
	GSM_DELAY(2000);//delay 2s
	wdt_clear();

	while(gsm_cmd("+++","OK", 2000) != GSM_TRUE)//
	{
		printf("\r\n +++ not replay AT OK, retry %d\r\n", retry);

		if(++retry > 50) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(50);
	retry = 0;
	while(gsm_cmd("AT+MIPCLOSE=1\r","+MIPCLOSE: 1,0", 1000 * 30) != GSM_TRUE)//关闭会话
	{
		printf("\r\n AT+MIPCLOSE not replay AT OK, retry %d\r\n", retry);

		if(++retry > 3) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(50);
	retry = 0;
	while(gsm_cmd("AT+MIPCALL=0\r","+MIPCALL: 0", 1000 * 30) != GSM_TRUE)//释放IP
	{
		printf("\r\n AT+MIPCALL not replay AT OK, retry %d\r\n", retry);

		if(++retry > 2) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}

	printf("\r\n close moudule!!！\r\n");
	return GSM_TRUE;

	sms_failure:
	printf("\r\n close tcp link fail... \r\n");

	return GSM_FALSE;
}

/*---------------------------------------------------------------------*/

/**同步时间接口***/
#define HAD_SYNC	0xA5
uint8_t gsm_sync_time_from_net(void)
{
	u8 retry;
	int ret = GSM_FALSE;
	uint8_t error_code;
	u8 sync_time = 0;
	u8 sync_ok = 0;

#if 1
	ret = syscfg_read(CFG_USER_SYNC_TIME, &sync_time, 1);
	if (ret <= 0)
		printf("syscfg_read failed\n");

	else {
		printf("syscfg_read tmp = %x\n", sync_time);

		if (sync_time != HAD_SYNC) {

			printf("sync time first \n");

		} else {

			printf("had sync time\n");

			return 1;
		}
	}
#endif
	module_power_on();

	GSM_CLEAN_RX();                 //清空了接收缓冲区数据

	while(gsm_cmd("AT\r","OK", 200) != GSM_TRUE)// AT
	{
		printf("\r\n module not replay AT OK, retry %d\r\n", retry);

		if(++retry > 50) {
			printf("\r\n AT not response！！\r\n");
			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(50);
	retry = 0;

	while(gsm_cmd("ATE0\r","OK", 200) != GSM_TRUE)// 关闭回显
	{
		printf("\r\n ATE0 not replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n模块响应测试不正常！！\r\n");
			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(50);

	retry = 0;
	while(gsm_cmd("AT+CPIN?\r","+CPIN: READY", 2000) != GSM_TRUE)//查询SIM卡
	{
		printf("\r\n CPIN replay AT OK, retry %d\r\n", retry);
		if(++retry > 10) {
			printf("\r\n未检测到SIM卡\r\n");

			error_code = NO_SIM_CARD;
			goto sms_failure;
		}
	}


	wdt_clear();
	GSM_DELAY(50);
	retry = 0;
	while(gsm_cmd("AT+CSQ?\r","OK", 200) != GSM_TRUE)//查询信号值
	{
		printf("\r\n AT+CSQ replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {

			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(50);
	retry = 0;
	while(gsm_cmd("AT+CGREG?\r","OK", 200) != GSM_TRUE)//确认PS数据业务可用
	{
		printf("\r\n AT+CGREG? not OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n AT+CGREG? ERROR \r\n");
			error_code = NET_ERROR;
			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(50);
	retry = 0;
	while(gsm_cmd("AT+GTSET=\"IPRFMT\",2\r","OK", 200) != GSM_TRUE)//确认PS数据业务可用
	{
		printf("\r\n AT+GTSET? not OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n AT+GTSET? ERROR \r\n");
			error_code = NET_ERROR;
			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(50);
	retry = 0;
#if 0
#if  (SIM_CARD_TYPE == CTNET)
		while(gsm_cmd("AT+CGDCONT=1,\"IP\",\"CTNET\"\r","OK", 8000) != GSM_TRUE)// 设置激活时APN。电信
#elif (SIM_CARD_TYPE == CMNET)
		while(gsm_cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"\r","OK", 8000) != GSM_TRUE)// 设置激活时APN。移动
#elif (SIM_CARD_TYPE == TGNET)
		while(gsm_cmd("AT+CGDCONT=1,\"IP\",\"3GNET\"\r","OK", 8000) != GSM_TRUE)// 设置激活时APN。联通
#endif
#endif
	while(gsm_cmd("AT+CGDCONT=1,\"IP\",\"CTNET\"\r","OK", 8000) != GSM_TRUE)// 设置激活时APN。移动

	{
		printf("\r\n AT+CGDCONT= not replay AT OK, retry %d\r\n", retry);

		if(++retry > 2) {
			printf("\r\n模块响应测试不正常！！\r\n");
			error_code = NET_ERROR;
			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(50);
	retry = 0;
#if 0
#if (SIM_CARD_TYPE == CTNET)
	while(gsm_cmd("AT+MIPCALL=1,\"CTNET\"\r","+MIPCALL", 1000 * 150) != GSM_TRUE)// 链接TCP
#elif ((SIM_CARD_TYPE == CMNET))
	while(gsm_cmd("AT+MIPCALL=1,\"CMNET\"\r","+MIPCALL", 1000 * 150) != GSM_TRUE)// 链接TCP

#elif((SIM_CARD_TYPE == TGNET))
	while(gsm_cmd("AT+MIPCALL=1,\"TGNET\"\r","+MIPCALL", 1000 * 150) != GSM_TRUE)// 链接TCP
#endif
#endif
	while(gsm_cmd("AT+MIPCALL=1,\"CTNET\"\r","+MIPCALL", 1000 * 90) != GSM_TRUE)// 链接TCP

	{
		printf("\r\n AT+MIPCALL= not replay AT OK, retry %d\r\n", retry);

		if(++retry > 2) {
			error_code = NET_ERROR;
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}


	wdt_clear();
	GSM_DELAY(50);
	retry = 0;
#if 1
	while(gsm_cmd("AT+MIPOPEN=1,,\"47.113.105.118\",9899,0\r","+MIPOPEN", 1000 * 60) != GSM_TRUE)// 链接TCP
	{
		printf("\r\n AT+MIPOPEN not replay AT OK, retry %d\r\n", retry);

		if(++retry > 3) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}

	wdt_clear();
	GSM_DELAY(50);
	retry = 0;

	while(gsm_cmd("AT+MIPSEND=1,17\r",">", 1000 * 60) != GSM_TRUE)// 链接TCP
	{
		printf("\r\n AT+MIPOPEN not replay AT OK, retry %d\r\n", retry);

		if(++retry > 3) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}

	wdt_clear();// sync time from ourself server
	if(gsm_cmd("====AT+CCLK?====\r","+MIPRTCP", 1000 * 60) == GSM_TRUE) {

		get_and_set_time_form_our_server();
		ret = GSM_TRUE;
		sync_ok = 1;
	}
#endif

	wdt_clear();
	GSM_DELAY(50);
	retry = 0;
	/*
	最常见、熟知的就是，www.pool.ntp.org/zone/cn
	cn.ntp.org.cn #中国
	edu.ntp.org.cn #中国教育网
	ntp1.aliyun.com #阿里云
	ntp2.aliyun.com #阿里云
	cn.pool.ntp.org #最常用的国内NTP服务器
	*/
#if 0
	//while(gsm_cmd("AT+MIPNTP=\"cn.ntp.org.cn\",123\r","+MIPNTP: 1", 1000 * 60) != GSM_TRUE)// 同步时间
	while(gsm_cmd("AT+MIPNTP=\"cn.pool.ntp.org\",123\r","+MIPNTP: 1", 1000 * 60) != GSM_TRUE)// 同步时间
	{
		printf("\r\n AT+MIPNTP not replay AT OK, retry %d\r\n", retry);

		if(++retry > 3) {
			printf("\r\n模块响应测试不正常！！\r\n");
			error_code = NET_ERROR;
			goto sms_failure;
		}
	}
#endif

	if (ret != GSM_TRUE)
		ret = gsm_cmd("AT+MIPNTP=\"cn.pool.ntp.org\",123\r","+MIPNTP: 1", 1000 * 60);

	if (ret != GSM_TRUE)
		ret = gsm_cmd("AT+MIPNTP=\"cn.ntp.org.cn\",123\r","+MIPNTP: 1", 1000 * 60);

	if (ret != GSM_TRUE)
		ret = gsm_cmd("AT+MIPNTP=\"edu.ntp.org.cn\",123\r","+MIPNTP: 1", 1000 * 60);

	if (ret != GSM_TRUE)
		ret = gsm_cmd("AT+MIPNTP=\"ntp1.aliyun.com\",123\r","+MIPNTP: 1", 1000 * 60);

	if (ret != GSM_TRUE)
		ret = gsm_cmd("AT+MIPNTP=\"ntp2.aliyun.com\",123\r","+MIPNTP: 1", 1000 * 60);

	if (ret != GSM_TRUE) {
		error_code = NET_ERROR;
		goto sms_failure;
	}

	wdt_clear();

	if ((ret == GSM_TRUE) && (sync_ok == 0)) {

		//+CCLK: "21/11/18,16:04:54+00"
		if(gsm_cmd("AT+CCLK?\r","OK", 1000 * 60) == GSM_TRUE) {

			get_and_set_time();
		}

	}


	printf("\r\n sync time successed\r\n");

	wdt_clear();
	retry = 0;
	while(gsm_cmd("AT+MIPCALL=0\r","+MIPCALL: 0", 1000 * 30) != GSM_TRUE)//释放IP
	{
		printf("\r\n AT+MIPCALL not replay AT OK, retry %d\r\n", retry);

		if(++retry > 2) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}

	module_power_off();

	return GSM_TRUE;

	sms_failure:
		printf("\r\n sync time fail... \r\n");
		os_taskq_post_msg("at_4g_task", 2, APP_USER_MSG_GSM_ERROR, error_code);
		module_power_off();
		return GSM_FALSE;
}



/*---------------------------------------------------------------------*/
//判断4G模块是否处于开机状态
void check_moudule_whether_is_power_on(void)
{
	u8 retry = 0;
	int ret;

	GSM_DELAY(1000); 			 //延时

	GSM_CLEAN_RX();                 //清空了接收缓冲区数据

	while(gsm_cmd("+++","OK", 1000) != GSM_TRUE)//
	{
		printf("\r\n +++ not replay AT OK, retry %d\r\n", retry);

		if(++retry > 2) {
			break;
		}
	}

	while(gsm_cmd("AT\r","OK", 2000) != GSM_TRUE)// AT
	{

		if(++retry > 2) {
			printf("\r\n AT not response！！\r\n");
			led_power_on_show_end();
			return;
		}
	}

	led_power_on_show_end();

	printf("module is power on state\n");
	module_power_off();
	wdt_clear();
	GSM_DELAY(5000);
	wdt_clear();
}


uint8_t get_and_set_time(void)
{
	struct sys_time t;
	char *redata;
	uint8_t len;
	int ret;
	u8 sync_time = 0;

	char year_temp[3];
	char month_temp[3];
	char day_temp[3];
	char hour_temp[3];
	char min_temp[3];
	char sec_temp[3];

	redata = GSM_RX(len);   //接收数据
	printf("sync time: %s\n", redata);

	memcpy(year_temp,  redata + 10, 2);
	t.year = 2000 + atoi(year_temp);
	memcpy(month_temp, redata + 13, 2);
	t.month = atoi(month_temp);
	memcpy(day_temp,   redata + 16, 2);
	t.day = atoi(day_temp);
	memcpy(hour_temp,  redata + 19, 2);
	//t.hour = (atoi(hour_temp) + 8) % 24;//add 8 hours
	t.hour = atoi(hour_temp);
	memcpy(min_temp,   redata + 22, 2);
	t.min = atoi(min_temp);
	memcpy(sec_temp,   redata + 25, 2);
	t.sec = atoi(sec_temp);

	UTCToBeijing(&t);

	printf("time: %d-%d-%d:%d:%d:%d\n", t.year, t.month, t.day, t.hour, t.min, t.sec);
	set_sys_time(&t);

	sync_time = HAD_SYNC;
	ret = syscfg_write(CFG_USER_SYNC_TIME, &sync_time, 1);
	if (ret <= 0) {
		printf("syscfg_write failed");
		ret = GSM_FALSE;
	} else
		ret = GSM_TRUE;

	return ret;
}



uint8_t get_and_set_time_form_our_server(void)
{
	struct sys_time t;
	char *redata;
	uint8_t len;
	int ret;
	int offset;
	u8 sync_time = 0;

	char year_temp[3];
	char month_temp[3];
	char day_temp[3];
	char hour_temp[3];
	char min_temp[3];
	char sec_temp[3];
	//====+CCLK:21/11/25,14:39:50+00====
	//====+CCLK:21/11/18,16:04:54+00"
	printf("get_and_set_time_form_our_server\n");

	redata = GSM_RX(len);   //接收数据
	printf("sync time: %s\n", redata);
#if 0
	for (int i = 0; i < len; i++) {
		printf("redata[%d] = %c\n", i, redata[i]);
		printf("redata[%d] = %x\n", i, redata[i]);
	}
#endif
	memcpy(year_temp,  redata + 78, 2);
	t.year = 2000 + atoi(year_temp);
	memcpy(month_temp, redata + 81, 2);
	t.month = atoi(month_temp);
	memcpy(day_temp,   redata + 84, 2);
	t.day = atoi(day_temp);
	memcpy(hour_temp,  redata + 87, 2);
	t.hour = atoi(hour_temp);
	memcpy(min_temp,   redata + 90, 2);
	t.min = atoi(min_temp);
	memcpy(sec_temp,   redata + 93, 2);
	t.sec = atoi(sec_temp);

	printf("time: %d-%d-%d:%d:%d:%d\n", t.year, t.month, t.day, t.hour, t.min, t.sec);
	set_sys_time(&t);

	sync_time = HAD_SYNC;
	ret = syscfg_write(CFG_USER_SYNC_TIME, &sync_time, 1);
	if (ret <= 0) {
		printf("syscfg_write failed");
		ret = GSM_TRUE;
	} else
		ret = GSM_FALSE;

	return ret;
}


void UTCToBeijing(struct sys_time* time)
{
	uint8_t days = 0;
	if (time->month == 1 || time->month == 3 || time->month == 5 || time->month == 7 || time->month == 8 || time->month == 10 || time->month == 12)
	{
		days = 31;
	}
	else if (time->month == 4 || time->month == 6 || time->month == 9 || time->month == 11)
	{
		days = 30;
	}
	else if (time->month == 2)
	{
		if ((time->year % 400 == 0) || ((time->year % 4 == 0) && (time->year % 100 != 0))) /* 判断平年还是闰年 */
		{
			days = 29;
		}
		else
		{
			days = 28;
		}
	}
	time->hour += 8;                 /* 北京时间比格林威治时间快8小时 */
	if (time->hour >= 24)            /* 跨天 */
	{
		time->hour -= 24;
		time->day++;
		if (time->day > days)        /* 跨月 */
		{
			time->day = 1;
			time->month++;
			if (time->month > 12)    /* 跨年 */
			{
				time->year++;
			}
		}
	}
}


