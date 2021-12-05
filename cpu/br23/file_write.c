#include "system/includes.h"
#include "asm/uart_dev.h"
#include "system/event.h"
#include "common/app_common.h"
#include "file_bs_deal.h"
#include "public.h"


#if 0



//static FILE *test_file = NULL;
FILE *test_file = NULL;


struct sys_time g_time;

const char root_path[] = "storage/sd0/C/";
char file_path[80] = {0};
char year_month_day[20] = {0};
char hour_min_sec[20] = {0};

extern void test_browser();
extern int file_browse_enter_onchane(void);
void check_config_file(void);
void file_write_task_del(void);
void uart_receive_task_del(void);

static void file_write_task_handle(void *arg)
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
            //printf("use os_taskq_post_msg");
	        switch (msg[1]) {
	            case APP_USER_MSG_BUFFER_HAVE_DATA:
				#if USE_LWRB
				if(test_file && recoder.recoder_state) {
							int n_bytes_in_queue = lwrb_get_full(&receive_buff);
							if (n_bytes_in_queue > 0) {
								int n_read = lwrb_read(&receive_buff, write_buffer, n_bytes_in_queue);

								ret = fwrite(test_file, write_buffer, n_read);
								if (ret != n_read) {
									log_e(" file write buf err %d\n", ret);
									fclose(test_file);
									test_file = NULL;
								}
						}
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
					strcat(file_path, ".pcm");
					printf("file_path:%s\n", file_path);
					if (!test_file) {
						test_file = fopen(file_path, "w+");
						if (!test_file) {
							printf("fopen file faild!\n");
						} else {
							printf("open file succed\r\n");
						}
					}

					memset(file_path, 0, sizeof(file_path));

					bes_start_recoder();
#endif
					break;
				case APP_USER_MSG_STOP_RECODER:
						printf("APP_USER_MSG_STOP_RECODER");
						printf("file write end....\n");

						if (test_file) {
							fclose(test_file);
							test_file = NULL;
						}
						bes_stop_recoder();
					break;
	            default:
	                break;
	       }
        }
   }
}


void file_write_thread_init(void)
{

	//bes_power_on();
    printf("file_write_thread_init\n");

	//os_task_create(file_write_task,NULL, 7, 512, 512, "file_write_task");
	task_create(file_write_task_handle,NULL, "file_write");

}


void file_write_task_del(void)
{
    printf("file_write_task_del\n");

    os_task_del("file_write_task");
}




#endif
