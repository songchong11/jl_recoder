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

void module_4g_gpio_init(void)
{
	printf("module_4g_gpio_init\r\n");
	gpio_set_pull_up(MODULE_PWR_GPIO, 0);
	gpio_set_pull_down(MODULE_PWR_GPIO, 0);
	gpio_set_die(MODULE_PWR_GPIO, 0);
	gpio_set_direction(MODULE_PWR_GPIO, 0);
	gpio_set_output_value(MODULE_PWR_GPIO, 0);

}

void module_power_on(void)
{
	printf("4g module power on\r\n");
	gpio_set_output_value(MODULE_PWR_GPIO, 1);
	led_green_on();
	delay_2ms(1500);//need more over 2s
	gpio_set_output_value(MODULE_PWR_GPIO, 0);
	led_green_off();
}

void module_power_off(void)
{
	printf("4g module power off\r\n");
	gpio_set_output_value(MODULE_PWR_GPIO, 1);
	led_red_on();
	delay_2ms(2000);//need more over 3.1s
	gpio_set_output_value(MODULE_PWR_GPIO, 0);
	led_red_off();
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
				#if 1
					/*power on 4g module and send file to at*/
					module_power_on();
					gsm_register_init();
					gsm_set_to_access_mode();
				#endif
					file_read_from_sd_card();
	                break;

				case APP_USER_MSG_STOP_SEND_FILE_TO_AT:
					printf("APP_USER_MSG_STOP_SEND_FILE_TO_AT");
					/*power off 4g module */
					clsoe_module();
					module_power_off();
					break;
				case APP_USER_MSG_SEND_FILE_OVER:
					printf("APP_USER_MSG_SEND_FILE_OVER");
					/*power off 4g module */
					clsoe_module();
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
FILE *fp = NULL;
extern void gsm_send_buffer(u8 *buf, int len);


void file_read_and_send(void *priv)
{
	if(!fp) {
		printf("file open error!!!\r\n");
		return;
	}

	int ret = fread(fp, read_buffer, READ_LEN);

	if(ret == READ_LEN)
		gsm_send_buffer(read_buffer, READ_LEN);
	else {
		gsm_send_buffer(read_buffer, ret);
		printf("send over, close file!!!\r\n");
		fclose(fp);
		printf("send a msg to close 4g module\r\n");
		os_taskq_post_msg("at_4g_task", 1, APP_USER_MSG_SEND_FILE_OVER);
	}

}

void file_read_from_sd_card(void)
{

	fp = fopen("storage/sd0/C/20201214192925","rb");

	if (fp)
		printf("open file successd\r\n");
	else
		printf("open file error\r\n");
	//fseek(fp,0,SEEK_END);

	if (file_send_timer == 0) {
		file_send_timer = sys_timer_add(NULL, file_read_and_send, 30);
	}
}






//////////////////////////////////////////////////////////////////////////////////////
static uint8_t MaxMessAdd=50;


//0表示成功，1表示失败

uint8_t gsm_cmd(char *cmd, char *reply,uint32_t waittime )
{
	GSM_DEBUG_FUNC();

    GSM_CLEAN_RX();                 //清空了接收缓冲区数据

    GSM_TX(cmd);                    //发送命令

	GSM_DEBUG("Send cmd:%s",cmd);	

    if(reply == 0)                      //不需要接收数据
    {
        return GSM_TRUE;
    }

    GSM_DELAY(waittime);                 //延时


    return gsm_cmd_check(reply);    //对接收数据进行处理
}


//0表示成功，1表示失败
uint8_t gsm_cmd_check(char *reply)
{
    uint8_t len;
    uint8_t n;
    uint8_t off;
    char *redata;

	GSM_DEBUG_FUNC();

    redata = GSM_RX(len);   //接收数据

	  *(redata+len) = '\0';
	  GSM_DEBUG("Reply:%s",redata);	

//		GSM_DEBUG_ARRAY((uint8_t *)redata,len);

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
uint8_t gsm_register_init(void)
{
	u8 retry;
	char *redata;
	uint8_t len;

	GSM_DEBUG_FUNC();  

	GSM_CLEAN_RX();                 //清空了接收缓冲区数据

	while(gsm_cmd("AT\r","OK", 800) != GSM_TRUE)// AT
	{
		printf("\r\n module not replay AT OK, retry %d\r\n", retry);
		//return GSM_FALSE;
		if(++retry > 200) {
			printf("\r\n模块响应测试不正常！！\r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}

	retry = 0;
	while(gsm_cmd("ATE0\r","OK", 800) != GSM_TRUE)// 关闭回显
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n模块响应测试不正常！！\r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}

	retry = 0;
	while(gsm_cmd("AT+CGTRAT=6,3,2\r","OK", 800) != GSM_TRUE)// 设置搜索网络顺序
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n模块响应测试不正常！！\r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}

	GSM_DELAY(5*1000);//delay 5s

	retry = 0;
	while(gsm_cmd("AT+CPIN?\r","READY", 800) != GSM_TRUE)//查询SIM卡
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n未检测到SIM卡\r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}
	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+CIMI\r","OK", 800) != GSM_TRUE)//确认SIM卡可用
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\nSIM卡 ERROR\r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
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
	while(gsm_cmd("AT+CFUN?\r","OK", 800) != GSM_TRUE)//查询模块射频功能设置
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n AT+CFUN？ ERROR \r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}

	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+CFUN=1\r","OK", 800) != GSM_TRUE)//查询模块射频功能设置
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n AT+CFUN ERROR \r\n");

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

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}
	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+CGREG?\r","OK", 800) != GSM_TRUE)//确认PS数据业务可用
	{
		printf("\r\n replay AT OK, retry %d\r\n", retry);
		if(++retry > 90) {
			printf("\r\n AT+CGREG? ERROR \r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
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

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}

	return GSM_TRUE;

	sms_failure:
		printf("\r\n GSM MODULE init fail... \r\n");
		//AT+CFUN=4  
		//AT+CFUN=1 关开飞行模式一次再尝试查询
		return GSM_FALSE;
}

/*---------------------------------------------------------------------*/
int gsm_set_to_access_mode(void)
{
	u8 retry;
	char *redata;
	uint8_t len;

	GSM_DEBUG_FUNC();  

	GSM_CLEAN_RX();                 //清空了接收缓冲区数据

	while(gsm_cmd("AT+GTSET=\"IPRFMT\",1\r","OK", 800) != GSM_TRUE)// 设置接收到的数据为原始模式
	{
		printf("\r\n AT+GTSET= not replay AT OK, retry %d\r\n", retry);

		if(++retry > 200) {
			printf("\r\n模块响应测试不正常！！\r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}
	
	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+CGDCONT=1,\"IP\",\"CTNET\"\r","OK", 800) != GSM_TRUE)// 设置激活时APN。电信
	{
		printf("\r\n AT+CGDCONT= not replay AT OK, retry %d\r\n", retry);

		if(++retry > 200) {
			printf("\r\n模块响应测试不正常！！\r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}

	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+MIPCALL=1,\"IP\",\"CTNET\"\r","+MIPCALL", 800) != GSM_TRUE)// 设置激活时APN。电信
	{
		printf("\r\n AT+CGDCONT= not replay AT OK, retry %d\r\n", retry);

		if(++retry > 200) {
			printf("\r\n模块响应测试不正常！！\r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}

	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+MIPODM=1,\"47.113.105.118\",9999,0\r","+MIPOMD", 800) != GSM_TRUE)// 设置激活时APN。电信
	{
		printf("\r\n AT+MIPODM= not replay AT OK, retry %d\r\n", retry);

		if(++retry > 200) {
			printf("\r\n模块响应测试不正常！！\r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}

	printf("\r\n enter into access mode!！\r\n");
	return GSM_TRUE;

	sms_failure:
	printf("\r\n GSM access mode fail... \r\n");
	//AT+CFUN=4  
	//AT+CFUN=1 关开飞行模式一次再尝试查询
	return GSM_FALSE;
}

/*---------------------------------------------------------------------*/

int clsoe_module(void)
{
	u8 retry;
	char *redata;
	uint8_t len;

	GSM_DEBUG_FUNC();  

	GSM_CLEAN_RX();                 //清空了接收缓冲区数据

	GSM_DELAY(1000);//delay 1s

	while(gsm_cmd("+++","OK", 800) != GSM_TRUE)// 设置接收到的数据为原始模式
	{
		printf("\r\n +++ not replay AT OK, retry %d\r\n", retry);

		if(++retry > 200) {
			printf("\r\n模块响应测试不正常！！\r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}
	
	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+MIPODM?","OK", 800) != GSM_TRUE)// 设置激活时APN。电信
	{
		printf("\r\n AT+MIPODM?  not replay AT OK, retry %d\r\n", retry);

		if(++retry > 200) {
			printf("\r\n模块响应测试不正常！！\r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}

	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+MIPCLOSE=1\r","OK", 1000) != GSM_TRUE)//关闭会话
	{
		printf("\r\n AT+CGDCONT= not replay AT OK, retry %d\r\n", retry);

		if(++retry > 200) {
			printf("\r\n模块响应测试不正常！！\r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}

	GSM_DELAY(500);
	retry = 0;
	while(gsm_cmd("AT+MIPCALL=0\r","OK", 800) != GSM_TRUE)//释放IP
	{
		printf("\r\n AT+MIPCALL not replay AT OK, retry %d\r\n", retry);

		if(++retry > 200) {
			printf("\r\n模块响应测试不正常！！\r\n");

			led_blink_time(100, 5000);//5s
			//module_power_off();
			goto sms_failure;
		}
	}

	printf("\r\n close moudule!!！\r\n");
	return GSM_TRUE;

	sms_failure:
	printf("\r\n GSM access mode fail... \r\n");
	//AT+CFUN=4  
	//AT+CFUN=1 关开飞行模式一次再尝试查询
	return GSM_FALSE;
}

/*---------------------------------------------------------------------*/


//检测是否有卡
//0表示成功，1表示失败
uint8_t IsInsertCard(void)
{
	GSM_DEBUG_FUNC();
	
	GSM_CLEAN_RX();
	return gsm_cmd("AT+CPIN?\r","OK",200);
   
}


//设置网络优先的类型为4G
//0表示成功，1表示失败
uint8_t set_net_type(void)
{
	GSM_DEBUG_FUNC();
	
	GSM_CLEAN_RX();
	return gsm_cmd("AT+CGTRAT=6,3,2\r","OK",200);
   
}


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



//查询是否接收到新短信
//0:无新短信，非0：新短信地址
uint8_t IsReceiveMS(void)
{
//	char address[3]={0};
	uint8_t addressnum=0;
	char *redata;
	char *redata_off;
    uint8_t len;
	
	GSM_DEBUG_FUNC();  

/*------------- 查询是否有新短信并提取存储位置 ----------------------------*/
    if(gsm_cmd_check("+CMTI:"))
    {
        return 0;
    }
   
    redata = GSM_RX(len);   //接收数据
		
	//偏移至响应的字符串地址
	redata_off = strstr(redata,"+CMTI: \"SM\",");
//	printf("redata_off=%s",redata_off);

	if(redata_off == NULL)
		return 0;
	
	//计算偏移后剩余的长度
	len = len - (redata_off - redata); 
		
//	printf("CMTI:redata:%s,len=%d\n",redata,len);
    if(len == 0)
    {
        return 0;
    }
    
	//分割字符串
	strtok(redata_off,",");		
	
	//strtok分割，第二段作为atoi输入，若转换成功则为短信地址，否则atoi返回0
	addressnum = atoi(strtok(NULL,","));
					
//	printf("\r\naddressnum=%d",addressnum);

	return addressnum;

}	

//读取短信内容
//形参：	messadd：	短信地址
//			num：		保存发件人号码(unicode编码格式的字符串)
//			str：		保存短信内容(unicode编码格式的字符串)
//返回值：	0表示失败
//			1表示成功读取到短信，该短信未读（此处是第一次读，读完后会自动被标准为已读）
//			2表示成功读取到短信，该短信已读
uint8_t readmessage(uint8_t messadd,char *num,char *str)
{
	char *redata,cmd[20]={0};
    uint8_t len;
	char result=0;
	
	GSM_DEBUG_FUNC();  


	GSM_CLEAN_RX();                 //清空了接收缓冲区数据
	if(messadd>MaxMessAdd)return 0;

/*------------- 读取短信内容 ----------------------------*/
	sprintf(cmd,"AT+CMGR=%d\r",messadd);	

	if(gsm_cmd(cmd,"+CMGR:",500) != GSM_TRUE)
	{
		printf("GSM_FALSE");
		return 0;
	}

	redata = GSM_RX(len);   //接收数据

	if(len == 0)
	{
		return 0;
	}
//	printf("CMGR:redata:%s\nlen=%d\n",redata,len);

	if(strstr(redata,"UNREAD")==0)result=2;
	else	result=1;
	//第一个逗号后面的数据为:”发件人号码“
	while(*redata != ',')
	{
		len--;
		if(len==0)
		{
			return 0;
		}
		redata++;
		
	}
	redata+=2;//去掉',"'
	while(*redata != '"')
	{
		*num++ = *redata++;
		len--;
	}
	*num = 0;               //字符串结尾需要清0
	
	while(*redata != '+')
	{
		len--;
		if(len==0)
		{
			return 0;
		}
		redata++;
	}
	redata+=6;//去掉'+32"\r'
	while(*redata != '\r')
	{
		*str++ = *redata++;
	}
	*str = 0;               //字符串结尾需要清0
	
//	printf("CMGR:result:%d\n",result);
	return result;
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

/**
 * @brief  返回从GSM模块接收到的网络数据，打印到串口
 * @retval 失败GSM_FALSE  成功GSM_TRUE
 */
uint8_t PostGPRS(void)
{

	char *redata;
	uint8_t len;

	GSM_DEBUG_FUNC();  

	redata = GSM_RX(len);   //接收数据 

	if(len == 0)
	{
			return GSM_FALSE;
	}
	
	printf("PostTCP:%s\n",redata);
	GSM_CLEAN_RX();
	
	return GSM_TRUE;

}


/*---------------------------------------------------------------------*/

