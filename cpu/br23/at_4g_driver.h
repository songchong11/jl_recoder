#ifndef _AT_4G_DRIVER_H_
#define _AT_4G_DRIVER_H_

#include "generic/typedef.h"

enum{
    CTNET = 0,//电信
    CMNET,//移动
    TGNET,//联通
};


typedef enum{
    GSM_TRUE,
    GSM_FALSE,
    GSM_CMD_ERROR,
    
}gsm_res_e;


typedef enum{
    NO_SIM_CARD = 0x81,
	NET_ERROR,
    NO_SD_CARD,
    
}error_code;


typedef enum
{
    GSM_NULL                = 0,
    GSM_CMD_SEND            = '\r',         
    GSM_DATA_SEND           = 0x1A,         //发送数据(ctrl + z)
    GSM_DATA_CANCLE         = 0x1B,         //发送数据(Esc)    
}gsm_cmd_end_e;

//                  指令             正常返回
//本机号码          AT+CNUM\r         +CNUM: "","13265002063",129,7,4            //很多SIM卡默认都是没设置本机号码的，解决方法如下 http://www.multisilicon.com/blog/a21234642.html
//SIM营运商         AT+COPS?\r        +COPS: 0,0,"CHN-UNICOM"   OK
//SIM卡状态         AT+CPIN?\r        +CPIN: READY   OK
//SIM卡信号强度     AT+CSQ\r          +CSQ: 8,0   OK

extern  uint8_t     gsm_cmd(char *cmd, char *reply, uint32_t waittime);
extern  uint8_t     gsm_cmd_check   	(char *reply);
void check_config_file(void);
extern int get_recoder_file_path(u8 *tmp_dir, u8 *tmp_file);

uint8_t get_and_set_time(void);
uint8_t get_and_set_time_form_our_server(void);
void UTCToBeijing(struct sys_time* time);

extern void clean_rebuff(void);
extern char *get_rebuff(uint8_t *len);
extern void GSM_USART_printf(char *Data,...);

#define     GSM_CLEAN_RX()              clean_rebuff()
#define     gsm_ate0()                  gsm_cmd("ATE0\r","OK",100)              //关闭回显
#define     GSM_TX(cmd)                	GSM_USART_printf("%s",cmd)
#define     GSM_RX(len)                 ((char *)get_rebuff(&(len)))
#define     GSM_DELAY(time)             delay_2ms((time+1)/2)            //延时

/*************************** 电话 功能 ***************************/
#define  	GSM_HANGON()				GSM_TX("ATA\r");								
#define  	GSM_HANGOFF()				GSM_TX("ATH\r");	//挂断电话	

uint8_t 	gsm_init								(void);															//初始化并检测模块
uint8_t     gsm_cnum            (char *num);                        //获取本机号码
void        gsm_call           	(char *num);                        //发起拨打电话（不管接不接通）
uint8_t 	IsRing					(char *num);						//查询是否来电，并保存来电号码
uint8_t  IsInsertCard(void);

/***************************  短信功能  ****************************/
uint8_t     gsm_sms             (char *num,char *smstext);          //发送短信（支持中英文,中文为GBK码）
void        gsm_gbk2ucs2   		     (char * ucs2,char * gbk);           

uint8_t 	IsReceiveMS				(void);
uint8_t 	readmessage				(uint8_t messadd,char *num,char *str);
uint8_t 	hexuni2gbk				(char *hexuni,char *chgbk);


/*调试用串口*/

#define GSM_DEBUG_ON         	1
#define GSM_DEBUG_ARRAY_ON    1
#define GSM_DEBUG_FUNC_ON   	1
// Log define
#define GSM_INFO(fmt,arg...)           printf("<<-GSM-INFO->> "fmt"\n",##arg)
#define GSM_ERROR(fmt,arg...)          printf("<<-GSM-ERROR->> "fmt"\n",##arg)
#define GSM_DEBUG(fmt,arg...)          do{\
                                         if(GSM_DEBUG_ON)\
                                         printf("<<-GSM-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg);\
																					}while(0)

#define GSM_DEBUG_ARRAY(array, num)    do{\
                                         int32_t i;\
                                         uint8_t* a = array;\
                                         if(GSM_DEBUG_ARRAY_ON)\
                                         {\
                                            printf("<<-GSM-DEBUG-ARRAY->> [%d]\n",__LINE__);\
                                            for (i = 0; i < (num); i++)\
                                            {\
                                                printf("%02x   ", (a)[i]);\
                                                if ((i + 1 ) %10 == 0)\
                                                {\
                                                    printf("\n");\
                                                }\
                                            }\
                                            printf("\n");\
                                        }\
                                       }while(0)

#define GSM_DEBUG_FUNC()               do{\
                                         if(GSM_DEBUG_FUNC_ON)\
                                         printf("<<-GSM-FUNC->> Func:%s@Line:%d\n",__func__,__LINE__);\
                                       }while(0)


void at_4g_thread_init(void);
int clsoe_tcp_link(void);
uint8_t gsm_init_to_access_mode(void);
int gsm_set_to_access_mode(void);
bool file_read_from_sd_card(u8 *dir, u8 *file_name);
void module_4g_gpio_init(void);
extern void module_power_off(void);
int write_config_file_when_send_over(char *dir, char *file);
void send_end_packet(void);
void send_the_start_packet(const char * filename, const char* dir_name, u32 size);
extern uint8_t gsm_sync_time_from_net(void);
extern void set_sys_time(struct sys_time *time);
void check_moudule_whether_is_power_on(void);
int set_baud_and_reinit_at_port(void);

#endif
