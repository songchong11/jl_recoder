#include "includes.h"
#include "system/includes.h"
#include "asm/uart_dev.h"
#include "system/event.h"
#include "common/app_common.h"
#include "lwrb.h"
#include "at_4g_driver.h"
#include "led_driver.h"
#include "stdlib.h"


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
		led_green_on();
		delay_2ms(1500);//need more over 2s
		gpio_set_output_value(MODULE_PWR_GPIO, 0);
		led_green_off();

		delay_2ms(1500);//delay 3s

		module_status = POWER_ON;
	}
}

void module_power_off(void)
{

	if (module_status == POWER_ON) {

		printf("4g module power off\r\n");
		gpio_set_output_value(MODULE_PWR_GPIO, 1);
		led_red_on();
		delay_2ms(2000);//need more over 3.1s
		gpio_set_output_value(MODULE_PWR_GPIO, 0);
		led_red_off();

		module_status = POWER_OFF;
	}

}

u8 tmp_dir_name[10];
u8 tmp_file_name[10];
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

					#if 1
						module_power_on();
						while(gsm_init_to_access_mode() == GSM_FALSE){
							retry++;
							if(retry > 3) {
								module_power_off();
								delay_2ms(3000);
								module_power_on();
							}
						}
					#endif
					}
					printf("gsm enter into access mode success\n");

					os_taskq_post_msg("at_4g_task", 1, APP_USER_MSG_GET_NEXT_FILE);

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
					release_all_fs_source();
					memset(tmp_dir_name, 0x00, sizeof(tmp_dir_name));
					memset(tmp_file_name, 0x00, sizeof(tmp_file_name));

					printf("user stop send file, close 4G module\n");
					/*power off 4g module */
					clsoe_tcp_link();
					module_power_off();
					break;
				case APP_USER_MSG_SEND_FILE_OVER:

					release_all_fs_source();
					memset(tmp_dir_name, 0x00, sizeof(tmp_dir_name));
					memset(tmp_file_name, 0x00, sizeof(tmp_file_name));

					printf("send file over, close 4G module\n");
					/*power off 4g module */
					clsoe_tcp_link();
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

//////////////////////////////////////////////////////////////////////////////////////
uint8_t read_buffer[320];
#define READ_LEN	320
int file_send_timer;
extern void gsm_send_buffer(u8 *buf, int len);
static u32 packet_num = 0;

void file_read_and_send(void *priv)
{
	FILE *read_p = (FILE*)priv;

	if(!read_p) {
		printf("file open error!!!\r\n");
		return;
	}

	if(packet_num % 2)
		led_green_off();
	else
		led_green_on();

	int ret = fread(read_p, read_buffer, READ_LEN);

	if(ret == READ_LEN) {
		packet_num++;
		printf("sending %d\r\n", packet_num);
		gsm_send_buffer(read_buffer, READ_LEN);

	} else {

		sys_timer_del(file_send_timer);
		file_send_timer = 0;

		gsm_send_buffer(read_buffer, ret);//send last packet

		printf("send over, close file!!!\r\n");
		//fclose(read_p);
		led_green_off();
		printf("send a msg to get next file\r\n");

		//ret = write_config_file_when_send_over(tmp_dir_name, tmp_file_name);
		ret = rename_file_when_send_over(read_p, tmp_file_name);
		if (ret) {
			fclose(read_p);
			release_all_fs_source();
			os_taskq_post_msg("at_4g_task", 1, APP_USER_MSG_GET_NEXT_FILE);
		} else {
			fclose(read_p);
			printf("rename fail, stop send\n");
			os_taskq_post_msg("at_4g_task", 1, APP_USER_MSG_SEND_FILE_OVER);
		}


	}

}


static FILE *config_file = NULL;
char target_bp_dir[20] = {0};
char target_bp_file[20] = {0};
bool have_target_dir = false;
bool have_target_file = false;

#if 1
void check_config_file(void)
{

	char temp[100] = {0};

	config_file =  fopen("storage/sd0/C/config.ini", "r");
	if (!config_file) {
		printf("Have no config files\n");
		config_file =  fopen("storage/sd0/C/config.ini", "w+");
		if (!config_file) {
			printf("fopen config file faild!\n");
		} else {
			printf("fopen config file succed\r\n");
		}
	}

	if (config_file) {
		printf("config_file is exist\n");

		fread(config_file, temp, sizeof(temp));

		#if 1
		for (int i = 0; i < 40; i++) {
			printf("[%d]:%c %x", i, temp[i], temp[i]);
		}
		printf("\n");
		#endif

		memcpy(target_bp_dir, &temp[7], 8 + 1);
		memcpy(target_bp_file, &temp[25], 10 + 1);

		if(target_bp_dir[0] != 0x00 || target_bp_dir[1] != 0x00 || target_bp_dir[2] != 0x00) {
			have_target_dir = true;
			printf("have targe dir :%s\n", target_bp_dir);
		} else {
			have_target_dir = false;
			printf("have no targe dir\n");
		}

		if(target_bp_file[7] == 'p' && target_bp_file[8] == 'c' && target_bp_file[9] == 'm') {
			have_target_file = true;
			printf("have targe file: %s\n", target_bp_file);
		} else {
			have_target_file = false;
			printf("have no targe file\n");

		}

	}

	fclose(config_file);
	config_file = NULL;
}
#endif
//-----------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------

int write_config_file_when_send_over(char *dir, char *file)
{

	char bp_data[40] = {0};
	u8 val;

	config_file =  fopen("storage/sd0/C/config.ini", "w+");

	if (!config_file) {
		printf("fopen config file faild!\n");
		return -1;
	} else {
		printf("fopen config file succed\r\n");

		sprintf(bp_data, "%s%s\r\n%s%s\r\n","bp_dir:", dir, "bp_file:",file);
		printf("bp_data:%s\n", bp_data);

		//fseek(config_file, 0, SEEK_SET);
		int ret = fwrite(config_file, bp_data, 40);
		if(ret != 40) {

			printf("write config file fail %d\n", ret);
			val = -1;
		} else {
			printf("write config file succed\n");
			val = 0;
		}
	}

	fclose(config_file);
	config_file = NULL;

	return val;
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

#if 0
void file_read_from_sd_card(void)
{



	#if 1
	//fp = fopen("storage/sd0/C/20211130/231521.MP3","rb");
	//fp = fopen("storage/sd0/C/MLtest01.pcm","rb");

	if (fp)
		printf("open file successd\r\n");
	else
		printf("open file error\r\n");
	//fseek(fp,0,SEEK_END);

	if (file_send_timer == 0) {
		packet_num = 0;
		file_send_timer = sys_timer_add(NULL, file_read_and_send, 30);
	}

	#endif
}

#endif

static FILE *fp = NULL;
bool file_read_from_sd_card(u8 *dir, u8 *file_name)
{
	u8 tmp_path[40];

	sprintf(tmp_path, "%s%s/%s", "storage/sd0/C/",dir, file_name);	

	printf("read path:%s\n", tmp_path);

	//fp = fopen("storage/sd0/C/20211130/231521.MP3","rb");
	fp = fopen(tmp_path,"rb");

	if (fp)
		printf("open file successd\r\n");
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
#if 0
void send_the_start_packet(void)
{
	u8 start[320];
	u8 year_month_day[14] = {0};
	struct sys_time _time;


	get_sys_time(&_time);20211100 002222

	sprintf(year_month_day, "%d%02d%02d", _time.year, _time.month, _time.day);
	//printf("year_month_day:%s\n", year_month_day);

	sprintf(start, "%s%d%02d%02d%02d%02d%02d%s", \
							"====startdata====", \
			_time.year, _time.month, _time.day, _time.hour, _time.min, _time.sec \
			"====",)

}
#endif
//////////////////////////////////////////////////////////////////////////////////////
static uint8_t MaxMessAdd=50;


//0表示成功，1表示失败

uint8_t gsm_cmd(char *cmd, char *reply, uint32_t waittime)
{
	uint8_t ret;
	//GSM_DEBUG_FUNC();

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
			GSM_DELAY(1000);				 //延时
			ret = gsm_cmd_check(reply);
			if (ret == GSM_TRUE)
				 return ret;
		}
	}

	GSM_DELAY(1000);				 //延时
    return ret = gsm_cmd_check(reply);    //对接收数据进行处理
}


//0表示成功，1表示失败
uint8_t gsm_cmd_check(char *reply)
{
    uint8_t len;
    uint8_t n;
    uint8_t off;
    char *redata;

	//GSM_DEBUG_FUNC();

    redata = GSM_RX(len);   //接收数据

	  *(redata+len) = '\0';
	  GSM_DEBUG("Reply:%s",redata);

	  //GSM_DEBUG_ARRAY((uint8_t *)redata,len);

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

    return GSM_FALSE;       //跳出循环表示比较完毕后都没有相同的数据，因此跳出
}

char * gsm_waitask(uint8_t waitask_hook(void))      //等待有数据应答
{
    uint8_t len=0;
    char *redata;

	GSM_DEBUG_FUNC();

    do{
        redata = GSM_RX(len);   //接收数据

        if(waitask_hook!=0)
        {
            if(waitask_hook()==GSM_TRUE)           //返回 GSM_TRUE 表示检测到事件，需要退出
            {
                redata = 0;
                return redata;
            }
        }
    }while(len==0);                 //接收数据为0时一直等待

	GSM_DEBUG_ARRAY((uint8_t *)redata,len);


    GSM_DELAY(20);              //延时，确保能接收到全部数据（115200波特率下，每ms能接收11.52个字节）
    return redata;
}


//本机号码
//0表示成功，1表示失败
uint8_t gsm_cnum(char *num)
{
    char *redata;
    uint8_t len;

	GSM_DEBUG_FUNC();

    if(gsm_cmd("AT+CNUM\r","OK", 100) != GSM_TRUE)
    {
        return GSM_FALSE;
    }

    redata = GSM_RX(len);   //接收数据

	GSM_DEBUG_ARRAY((uint8_t *)redata,len);

    if(len == 0)
    {
        return GSM_FALSE;
    }

    //第一个逗号后面的数据为:"本机号码"
    while(*redata != ',')
    {
        len--;
        if(len==0)
        {
            return GSM_FALSE;
        }
        redata++;
    }
    redata+=2;

    while(*redata != '"')
    {
        *num++ = *redata++;
    }
    *num = 0;               //字符串结尾需要清0
    return GSM_TRUE;
}


//初始化并检测模块,自动注册网络
//0表示成功，1表示失败
uint8_t gsm_init_to_access_mode(void)
{
	u8 retry;
	char *redata;
	uint8_t len;

	GSM_DEBUG_FUNC();

	GSM_CLEAN_RX();                 //清空了接收缓冲区数据

	while(gsm_cmd("AT\r","OK", 1000) != GSM_TRUE)// AT
	{
		printf("\r\n module not replay AT OK, retry %d\r\n", retry);

		if(++retry > 90) {
			printf("\r\n AT not response！！\r\n");
			goto sms_failure;
		}
	}

	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("ATE0\r","OK", 1000) != GSM_TRUE)// 关闭回显
	{
		printf("\r\n ATE0 not replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n模块响应测试不正常！！\r\n");
			goto sms_failure;
		}
	}

	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+CFUN=1\r","OK", 1000) != GSM_TRUE)// 设置工作模式是正常工作模式
	{
		printf("\r\n AT+CFUN=1 not ok retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}

	GSM_DELAY(500);

	retry = 0;
	while(gsm_cmd("AT+CPIN?\r","+CPIN: READY", 1000) != GSM_TRUE)//查询SIM卡
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n未检测到SIM卡\r\n");

			goto sms_failure;
		}
	}

	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+CIMI\r","OK", 1000) != GSM_TRUE)//确认SIM卡可用
	{
		printf("\r\n AT+CIMI not replay OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\nSIM卡 ERROR\r\n");

			goto sms_failure;
		}
	}

	GSM_DELAY(500);
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

	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+CSQ?\r","OK", 800) != GSM_TRUE)//查询信号值
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n CSQ ERROR \r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}


	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+COPS?\r","OK", 800) != GSM_TRUE)//查询注册状态
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n AT+COPS? ERROR \r\n");

			goto sms_failure;
		}
	}
	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+CGREG?\r","OK", 800) != GSM_TRUE)//确认PS数据业务可用
	{
		printf("\r\n AT+CGREG? not OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n AT+CGREG? ERROR \r\n");

			goto sms_failure;
		}
	}
	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+CEREG?\r","OK", 800) != GSM_TRUE)//查询4G状态业务是否可用
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
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


	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+MIPODM=1,,\"47.113.105.118\",9999,0\r","+MIPODM", 1000 * 60) != GSM_TRUE)// 链接TCP
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
		led_blink_time(100, 5000);//5s
		return GSM_FALSE;
}


/*---------------------------------------------------------------------*/

int clsoe_tcp_link(void)
{
	u8 retry;
	char *redata;
	uint8_t len;

	//GSM_DEBUG_FUNC();

	GSM_CLEAN_RX();                 //清空了接收缓冲区数据

	GSM_DELAY(2000);//delay 2s

	while(gsm_cmd("+++","OK", 800) != GSM_TRUE)// 设置接收到的数据为原始模式
	{
		printf("\r\n +++ not replay AT OK, retry %d\r\n", retry);

		if(++retry > 200) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}

#if 0

	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+MIPODM?","OK", 1000 * 30) != GSM_TRUE)// 设置激活时APN。电信
	{
		printf("\r\n AT+MIPODM?  not replay AT OK, retry %d\r\n", retry);

		if(++retry > 3) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}
#endif
	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+MIPCLOSE=1\r","+MIPCLOSE: 1,0", 1000 * 30) != GSM_TRUE)//关闭会话
	{
		printf("\r\n AT+MIPCLOSE not replay AT OK, retry %d\r\n", retry);

		if(++retry > 3) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}

#if 1
	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+MIPCALL=0\r","+MIPCALL: 0", 1000 * 30) != GSM_TRUE)//释放IP
	{
		printf("\r\n AT+MIPCALL not replay AT OK, retry %d\r\n", retry);

		if(++retry > 2) {
			printf("\r\n模块响应测试不正常！！\r\n");

			goto sms_failure;
		}
	}
#endif
	printf("\r\n close moudule!!！\r\n");
	return GSM_TRUE;

	sms_failure:
	printf("\r\n GSM access mode fail... \r\n");

	return GSM_FALSE;
}

/*---------------------------------------------------------------------*/




/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/**
 * @brief  IsASSIC 判断是否纯ASSIC编码
 * @param  str: 字符串指针
 * @retval 纯ASSIC编码返回TRUE，非纯ASSIC编码返回FALSE
 */
uint8_t IsASSIC(char * str)
{
	GSM_DEBUG_FUNC();

    while(*str)
    {
        if(*str>0x7F)
        {
            return GSM_FALSE;
        }
        str++;
    }

    return GSM_TRUE;
}

/**
 * @brief  gsm_char2hex 把字符转换成16进制字符
 * @param  hex: 16进制字符存储的位置指针，ch：字符
 * @retval 无
 */
void gsm_char2hex(char *hex,char ch)
{
    const char numhex[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

		GSM_DEBUG_FUNC();

		*hex++  = numhex[(ch & 0xF0)>>4];
    *hex    = numhex[ ch & 0x0F];
}




/**
 * @brief  初始化GPRS网络
 * @param  无
 * @retval 失败GSM_FALSE  成功GSM_TRUE
 */
uint8_t gsm_gprs_init(void)
{
	GSM_DEBUG_FUNC();

	GSM_CLEAN_RX();
	if(gsm_cmd("AT\r","OK", 200) != GSM_TRUE) return GSM_FALSE;

	GSM_CLEAN_RX();
	if(gsm_cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"\r","OK", 200) != GSM_TRUE) return GSM_FALSE;

	GSM_CLEAN_RX();
	if(gsm_cmd("AT+CGATT=1\r","OK", 200)!= GSM_TRUE) return GSM_FALSE;

	GSM_CLEAN_RX();
	if(gsm_cmd("AT+CIPCSGP=1,\"CMNET\"\r","OK", 200)!= GSM_TRUE) return GSM_FALSE;

	return GSM_TRUE;
}

/**
 * @brief  建立TCP连接，最长等待时间20秒，可自行根据需求修改
	* @param  localport: 本地端口
	* @param  serverip: 服务器IP
	* @param  serverport: 服务器端口
 * @retval 失败GSM_FALSE  成功GSM_TRUE
 */
uint8_t gsm_gprs_tcp_link(char *localport,char * serverip,char * serverport)
{
	char cmd_buf[100];
	uint8_t testConnect=0;

	GSM_DEBUG_FUNC();

	sprintf(cmd_buf,"AT+CLPORT=\"TCP\",\"%s\"\r",localport);

	if(gsm_cmd(cmd_buf,"OK", 100)!= GSM_TRUE)
		return GSM_FALSE;

	GSM_CLEAN_RX();

	sprintf(cmd_buf,"AT+CIPSTART=\"TCP\",\"%s\",\"%s\"\r",serverip,serverport);
	gsm_cmd(cmd_buf,0, 100);

	//检测是否建立连接
	while(gsm_cmd_check("CONNECT OK") != GSM_TRUE)
	{
		if(++testConnect >200)//最长等待20秒
		{
			return GSM_FALSE;
		}
		GSM_DELAY(100);
	}
	return GSM_TRUE;
}


/**
 * @brief  使用GPRS发送数据，发送前需要先建立UDP或TCP连接
	* @param  str: 要发送的数据
 * @retval 失败GSM_FALSE  成功GSM_TRUE
 */
uint8_t gsm_gprs_send(const char * str)
{
	char end = 0x1A;
	uint8_t testSend=0;

	GSM_DEBUG_FUNC();

	GSM_CLEAN_RX();

	if(gsm_cmd("AT+CIPSEND\r",">",500) == 0)
	{
		GSM_USART_printf("%s",str);
		GSM_DEBUG("Send String:%s",str);

		GSM_CLEAN_RX();
		gsm_cmd(&end,0,100);

		//检测是否发送完成
		while(gsm_cmd_check("SEND OK") != GSM_TRUE )
		{
			if(++testSend >200)//最长等待20秒
			{
				goto gprs_send_failure;
			}
			GSM_DELAY(100);
		}
		return GSM_TRUE;
	}
	else
	{

gprs_send_failure:

		end = 0x1B;
		gsm_cmd(&end,0,0);	//ESC,取消发送

		return GSM_FALSE;
	}
}

/**
 * @brief  断开网络连接
 * @retval 失败GSM_FALSE  成功GSM_TRUE
 */
uint8_t gsm_gprs_link_close(void)              //IP链接断开
{
	GSM_DEBUG_FUNC();

	GSM_CLEAN_RX();
	if(gsm_cmd("AT+CIPCLOSE=1\r","OK",200) != GSM_TRUE)
    {
        return GSM_FALSE;
    }
	return GSM_TRUE;
}

/**
 * @brief  关闭场景
 * @retval 失败GSM_FALSE  成功GSM_TRUE
 */
uint8_t gsm_gprs_shut_close(void)               //关闭场景
{
	uint8_t  check_time=0;
	GSM_DEBUG_FUNC();

	GSM_CLEAN_RX();
	gsm_cmd("AT+CIPSHUT\r",0,0) ;
	while(	gsm_cmd_check("OK") != GSM_TRUE)
	{
		if(++check_time >50)
			return GSM_FALSE;

		GSM_DELAY(200);
	}

	return GSM_TRUE;
}


/*---------------------------------------------------------------------*/

