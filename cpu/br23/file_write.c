#include "system/includes.h"
#include "asm/uart_dev.h"
#include "system/event.h"
#include "common/app_common.h"
#include "lwrb.h"


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

/* Declare rb instance & raw data */
lwrb_t receive_buff;
uint8_t buff_data[1024 * 2];
uint8_t read_buffer[1024];
extern bool recoder_state;

static FILE *test_file = NULL;

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
	                //printf("APP_USER_MSG_BUFFER_HAVE_DATA");
					if(test_file && recoder_state) {
							int n_bytes_in_queue = lwrb_get_full(&receive_buff);
							if (n_bytes_in_queue > 0) {
								int n_read = lwrb_read(&receive_buff, read_buffer, n_bytes_in_queue);

								ret = fwrite(test_file, read_buffer, n_read);
								if (ret != n_read) {
									log_e(" file write buf err %d\n", ret);
									fclose(test_file);
									test_file = NULL;
								}
						}
					}
	                break;

				case APP_USER_MSG_START_RECODER:
					printf("APP_USER_MSG_START_RECODER");
						/*lwrb init*/
					int ret;
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

					if (!test_file) {
						test_file = fopen("storage/sd0/C/record02.txt", "w+");
						if (!test_file) {
							printf("fopen file faild!\n");
						} else {
							printf("open file succed\r\n");
						}
					}
					break;
				case APP_USER_MSG_STOP_RECODER:
						printf("APP_USER_MSG_STOP_RECODER");
						printf("file write end....\n");
						fclose(test_file);
						test_file = NULL;
					break;
	            default:
	                break;
	       }
        }
   }
}


void file_write_thread_init(void)
{

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
