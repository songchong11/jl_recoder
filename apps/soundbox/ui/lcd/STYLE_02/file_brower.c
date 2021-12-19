#include "ui/ui.h"
#include "app_config.h"
#include "ui/ui_api.h"
#include "system/timer.h"
#include "key_event_deal.h"
#include "audio_config.h"
#include "jiffies.h"
#include "app_power_manage.h"
#include "asm/charge.h"
#include "audio_dec_file.h"
#include "music/music_player.h"
#ifndef CONFIG_MEDIA_NEW_ENABLE
#include "application/audio_eq_drc_apply.h"
#else
#include "audio_eq.h"
#endif




#if 0
#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_SOUNDBOX))

#define STYLE_NAME  JL//必须要

extern int ui_hide_main(int id);
extern int ui_show_main(int id);
extern void key_ui_takeover(u8 on);


//文件浏览部分

#define TEXT_NAME_LEN (128)
#define TEXT_SHORT_NAME_LEN (13)//8.3+1
#define TEXT_PAGE     (4)
struct text_name_t {
    u16 len;
    u8  unicode;
    u8 fname[TEXT_NAME_LEN];
    u8 fsname[TEXT_SHORT_NAME_LEN];
    int index;
};



struct grid_set_info {
    int flist_index;  //文件列表首项所指的索引
    int open_index;  //所打开的文件所指的索引(未)
    int cur_total;
    const char *disk_name[4];
    u8  in_scan;//是否在盘内
    u8  disk_total;
    FILE *file;
    struct vfscan *fs;
    struct text_name_t text_list[TEXT_PAGE];
    struct vfs_attr attr[TEXT_PAGE];
#if (TCFG_LFN_EN)
    u8  lfn_buf[512];
#endif//TCFG_LFN_EN

};

static struct grid_set_info *handler = NULL;
#define __this 	(handler)
#define sizeof_this     (sizeof(struct grid_set_info))

static u32 LAYOUT_FNAME_LIST_ID[] = {
    FILE_LAYOUT_0,
    FILE_LAYOUT_1,
    FILE_LAYOUT_2,
    FILE_LAYOUT_3,
};

static u32 TEXT_FNAME_ID[] = {
    FILE_BROWSE_TEXT_0,
    FILE_BROWSE_TEXT_1,
    FILE_BROWSE_TEXT_2,
    FILE_BROWSE_TEXT_3,
};

static u32 PIC_FNAME_ID[] = {
    FILE_BROWSE_PIC_0,
    FILE_BROWSE_PIC_1,
    FILE_BROWSE_PIC_2,
    FILE_BROWSE_PIC_3,
};




static int __get_ui_max_num()
{
    return sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]);
}

static const u8 MUSIC_SCAN_PARAM[] = "-t"
                                     "MP1MP2MP3"
                                     " -sn -d"
                                     ;


static int file_select_enter(int from_index)
{

    int one_page_num = sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]);
    int i = 0;
    i = from_index % one_page_num;
    if (__this->attr[i].attr == F_ATTR_DIR) { //判断是不是文件夹文件属性
        if (!from_index) {
            __this->fs = fscan_exitdir(__this->fs);
        } else {
            __this->fs = fscan_enterdir(__this->fs, __this->text_list[i].fsname);
        }

        if (!__this->fs) {
            ui_hide(MUSIC_FILE_LAYOUT);
            ui_hide(MUSIC_MENU_LAYOUT);
            return false;
        }


        __this->cur_total = __this->fs->file_number + 1;
        return TRUE;
    } else {
        /* if (!strcmp(cur->name, APP_NAME_MUSIC)) {//当前在目标app内，则只要显示出目标页面即可 */
        app_task_put_key_msg(KEY_MUSIC_PLAYE_BY_DEV_SCLUST, __this->attr[i].sclust);
        /* music_play_file_by_dev_sclust(__this->dev->logo, __this->attr[i].sclust);///this is a demo */
        ui_hide(MUSIC_FILE_LAYOUT);
        ui_hide(MUSIC_MENU_LAYOUT);
        /* } */
    }
    return false;
}

static int file_list_flush(int from_index)
{
    FILE *f = NULL;
    int one_page_num = sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]);
    int i = 0;
    int end_index = from_index + one_page_num;
    char *name_buf = NULL;

    for (i = 0; i < sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]); i++) {
        memset(__this->text_list[i].fname, 0, TEXT_NAME_LEN);
        __this->text_list[i].len = 0;
    }

__find:
    i = from_index % one_page_num;
    if (from_index == 0) {
        sprintf(__this->text_list[i].fname, "%s", "..");
        sprintf(__this->text_list[i].fsname, "%s", "..");
        __this->attr[i].attr = F_ATTR_DIR;
        __this->text_list[i].unicode = 0;
        __this->text_list[i].len = strlen(__this->text_list[i].fname);
    } else {
        f = fselect(__this->fs, FSEL_BY_NUMBER, from_index);
    }

    if (f) {
        name_buf = malloc(TEXT_NAME_LEN);
        __this->text_list[i].len = fget_name(f, name_buf, TEXT_NAME_LEN);
        if (name_buf[0] == '\\' && name_buf[1] == 'U') {
            __this->text_list[i].len   -= 2;
            __this->text_list[i].unicode = 1;
            memcpy(__this->text_list[i].fname, name_buf + 2, TEXT_NAME_LEN - 2);
        } else {
            __this->text_list[i].unicode = 0;
            memcpy(__this->text_list[i].fname, name_buf, TEXT_NAME_LEN);
        }
        free(name_buf);
        name_buf = NULL;
        printf("\n--func=%s, line=%d\n", __FUNCTION__, __LINE__);
        printf("flush [%d]=%s\n", i, __this->text_list[i].fname);
        fget_attrs(f, &__this->attr[i]);
        if (__this->attr[i].attr == F_ATTR_DIR) { //判断是不是文件夹文件属性,文件夹需要获取短文件名
            fget_name(f, __this->text_list[i].fsname, TEXT_SHORT_NAME_LEN);
        }
        fclose(f);
        f = NULL;
    }

    from_index++;
    if (from_index < __this->cur_total && from_index < end_index) {
        goto __find;
    }


    for (i = 0; i < sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]); i++) {
        if (__this->text_list[i].len) {
            if (__this->text_list[i].unicode) {
                ui_text_set_textw_by_id(TEXT_FNAME_ID[i], __this->text_list[i].fname, __this->text_list[i].len, FONT_ENDIAN_SMALL, FONT_DEFAULT | FONT_HIGHLIGHT_SCROLL);
            } else {
                ui_text_set_text_by_id(TEXT_FNAME_ID[i], __this->text_list[i].fname, __this->text_list[i].len, FONT_DEFAULT);
            }
            if (__this->attr[i].attr == F_ATTR_DIR) {
                ui_pic_show_image_by_id(PIC_FNAME_ID[i], 1);
            } else {
                ui_pic_show_image_by_id(PIC_FNAME_ID[i], 0);
            }
            if (ui_get_disp_status_by_id(PIC_FNAME_ID[i]) != true) {
                ui_show(PIC_FNAME_ID[i]);
            }
        } else {
            ui_text_set_text_by_id(TEXT_FNAME_ID[i], __this->text_list[i].fname, __this->text_list[i].len, FONT_DEFAULT);
            if (ui_get_disp_status_by_id(PIC_FNAME_ID[i]) == true) {
                ui_hide(PIC_FNAME_ID[i]);
            }
        }

    }
    return 0;
}


static int file_browse_enter_onchane(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int fnum = 0;

    switch (e) {
    case ON_CHANGE_INIT:
        if (!__this) {
            __this = zalloc(sizeof_this);
        }

        if (!__this->fs) {
            if (!dev_manager_get_total(1)) {
                break;
            }

            void *dev = dev_manager_find_active(1);
            if (!dev) {
                break;
            }
            __this->fs = fscan(dev_manager_get_root_path(dev), MUSIC_SCAN_PARAM, 9);
#if (TCFG_LFN_EN)
            if (__this->fs) {
                fset_lfn_buf(__this->fs, __this->lfn_buf);
            }
#endif
            printf(">>> file number=%d \n", __this->fs->file_number);
            fnum = (__this->fs->file_number + 1 > __get_ui_max_num()) ? __get_ui_max_num() : __this->fs->file_number + 1;
            __this->cur_total = __this->fs->file_number + 1;
        }
        break;
    case ON_CHANGE_RELEASE:
        if (__this->fs) {
            fscan_release(__this->fs);
            __this->fs = NULL;
        }

        if (__this) {
            free(__this);
            __this = NULL;
        }

        break;
    case ON_CHANGE_FIRST_SHOW:
        if (__this->open_index) {
            //从回放返回文件列表时
            ui_set_call(file_list_flush, __this->flist_index);
        } else {
            ui_set_call(file_list_flush, 0);
            /* file_list_flush(0); */
            //刚进去文件列表时
            __this->flist_index = 0;
        }
        break;
    default:
        return false;
    }
    return false;
}

static int file_browse_onkey(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    int sel_item;
    sel_item = ui_grid_cur_item(grid);
    switch (e->value) {
    case KEY_OK:
        if (file_select_enter(__this->flist_index)) {
            ui_no_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[sel_item]);
            __this->flist_index = 0;
            file_list_flush(__this->flist_index);
            ui_grid_set_item(grid, 0);
            ui_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[0]);
        }


        break;
    case KEY_DOWN:

        sel_item++;

        __this->flist_index += 1;

        if (sel_item >= __this->cur_total || __this->flist_index >= __this->cur_total) {
            //大于文件数
            ui_no_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[sel_item - 1]);
            __this->flist_index = 0;
            file_list_flush(__this->flist_index);
            ui_grid_set_item(grid, 0);
            ui_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[0]);
            ui_vslider_set_persent_by_id(MUSIC_FILE_SLIDER, (__this->flist_index + 1) * 100 / __this->cur_total);
            return TRUE;
        }
        ui_vslider_set_persent_by_id(MUSIC_FILE_SLIDER, (__this->flist_index + 1) * 100 / __this->cur_total);
        if (sel_item >= TEXT_PAGE) {
            ui_no_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[sel_item - 1]);
            file_list_flush(__this->flist_index);
            ui_grid_set_item(grid, 0);
            ui_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[0]);
            return true; //不返回到首项
        }

        return FALSE;

        break;
    case KEY_UP:
        if (sel_item == 0) {
            __this->flist_index = __this->flist_index ? __this->flist_index - 1 : __this->cur_total - 1;
            ui_no_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[sel_item]);
            file_list_flush(__this->flist_index / TEXT_PAGE * TEXT_PAGE);
            ui_grid_set_item(grid, __this->flist_index % TEXT_PAGE);
            ui_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[__this->flist_index % TEXT_PAGE]);
            ui_vslider_set_persent_by_id(MUSIC_FILE_SLIDER, (__this->flist_index + 1) * 100 / __this->cur_total);
            return true; //不跳转到最后一项
        }
        __this->flist_index--;
        ui_vslider_set_persent_by_id(MUSIC_FILE_SLIDER, (__this->flist_index + 1) * 100 / __this->cur_total);
        return FALSE;
        break;

    default:
        return false;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(MUSIC_FILE_BROWSE)
.onchange = file_browse_enter_onchane,
 .onkey = file_browse_onkey,
  .ontouch = NULL,
};
#endif

#endif


#if 1

//文件浏览部分

#define TEXT_NAME_LEN (128)
#define TEXT_SHORT_NAME_LEN (13)//8.3+1
#define TEXT_PAGE     (10)
struct text_name_t {
    u16 len;
    u8  unicode;
    u8 fname[TEXT_NAME_LEN];
    u8 fsname[TEXT_SHORT_NAME_LEN];
    int index;
};



struct grid_set_info {
    int flist_index;  //文件列表首项所指的索引
    int open_index;  //所打开的文件所指的索引(未)
    int cur_total;
    const char *disk_name[4];
    u8  in_scan;//是否在盘内
    u8  disk_total;
    FILE *file;
    struct vfscan *fs;
    struct text_name_t text_list[TEXT_PAGE];
    struct vfs_attr attr[TEXT_PAGE];
#if (TCFG_LFN_EN)
    u8  lfn_buf[512];
#endif//TCFG_LFN_EN

};

static struct grid_set_info *handler = NULL;
#define __this 	(handler)

static struct grid_set_info *file_handler = NULL;
#define __this_file 	(file_handler)

#define sizeof_this     (sizeof(struct grid_set_info))

#define FILE_BROWSE_PIC_0 0X10805AD
#define FILE_BROWSE_PIC_1 0X1088F22
#define FILE_BROWSE_PIC_2 0X10887D4
#define FILE_BROWSE_PIC_3 0X10894DF
#define FILE_BROWSE_TEXT_0 0X10CA2F5
#define FILE_BROWSE_TEXT_1 0X10C0A6E
#define FILE_BROWSE_TEXT_2 0X10C68CB
#define FILE_BROWSE_TEXT_3 0X10C5339
#define FILE_LAYOUT_0 0X1031888
#define FILE_LAYOUT_1 0X1038A2F
#define FILE_LAYOUT_2 0X1035021
#define FILE_LAYOUT_3 0X103855D


static u32 TEXT_FNAME_ID[10] = {
    FILE_BROWSE_TEXT_0,
    FILE_BROWSE_TEXT_1,
    FILE_BROWSE_TEXT_2,
    FILE_BROWSE_TEXT_3,
};



static const u8 MUSIC_SCAN_PARAM[] = "-t"
                                     "WAVPCM"
                                     " -sn -d"
                                     ;

static const u8 RECODER_SCAN_PARAM[] = "-t"
                                     "WAVPCM"
                                     " -sn -d"
                                     ;


static int file_select_enter(int from_index)
{

    int one_page_num = sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]);
    int i = 0;
    i = from_index % one_page_num;
    if (__this->attr[i].attr == F_ATTR_DIR) { //判断是不是文件夹文件属性
        if (!from_index) {
            __this->fs = fscan_exitdir(__this->fs);
        } else {
            __this->fs = fscan_enterdir(__this->fs, __this->text_list[i].fsname);
        }

        if (!__this->fs) {
            printf("this dir have no file\r\n");
            return false;
        }


        __this->cur_total = __this->fs->file_number + 1;
        return TRUE;
    } else {

		printf("is not a dir\r\n");
    }
    return false;
}

//static int file_list_flush(int from_index)
#if 0
int file_list_flush(int from_index)
{
    FILE *f = NULL;
    int one_page_num = sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]);
    int i = 0;
    int end_index = from_index + one_page_num;
    char *name_buf = NULL;

    for (i = 0; i < sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]); i++) {
        memset(__this->text_list[i].fname, 0, TEXT_NAME_LEN);
        __this->text_list[i].len = 0;
    }
	printf("file_list_flush  -------------\r\n");
__find:
    i = from_index % one_page_num;
    if (from_index == 0) {

        sprintf(__this->text_list[from_index].fname, "%s", "..");
        sprintf(__this->text_list[from_index].fsname, "%s", "..");
        __this->attr[from_index].attr = F_ATTR_DIR;
        __this->text_list[from_index].unicode = 0;
        __this->text_list[from_index].len = strlen(__this->text_list[i].fname);
    } else {
        f = fselect(__this->fs, FSEL_BY_NUMBER, from_index);
    }

    if (f) {
        name_buf = malloc(TEXT_NAME_LEN);
        __this->text_list[from_index].len = fget_name(f, name_buf, TEXT_NAME_LEN);
        if (name_buf[0] == '\\' && name_buf[1] == 'U') {
            __this->text_list[i].len   -= 2;
            __this->text_list[i].unicode = 1;
            memcpy(__this->text_list[i].fname, name_buf + 2, TEXT_NAME_LEN - 2);
        } else {
            __this->text_list[i].unicode = 0;
            memcpy(__this->text_list[i].fname, name_buf, TEXT_NAME_LEN);
        }
        free(name_buf);
        name_buf = NULL;
        printf("flush [%d]=%s\n", i, __this->text_list[i].fname);
		printf("__this->flist_index: %x\n", __this->flist_index);
        fget_attrs(f, &__this->attr[i]);
        if (__this->attr[i].attr == F_ATTR_DIR) { //判断是不是文件夹文件属性,文件夹需要获取短文件名
            fget_name(f, __this->text_list[i].fsname, TEXT_SHORT_NAME_LEN);
        }
        fclose(f);
        f = NULL;
    }

    from_index++;
    if (from_index < __this->cur_total) {
        goto __find;
    }

    return 0;
}
#endif

extern char target_bp_dir[20];
extern char target_bp_file[20];
extern bool have_target_dir;
extern bool have_target_file;

#if 1
int file_list_flush(int from_index)
{

    FILE *dir = NULL;
    FILE *file = NULL;

    int i = 0;
    char *name_buf = NULL;
	int dir_num = __this->cur_total;

    for (i = 0; i < sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]); i++) {
        memset(__this->text_list[i].fname, 0, TEXT_NAME_LEN);
        __this->text_list[i].len = 0;
    }
	printf("file_list_flush  -------------\r\n");

	name_buf = malloc(TEXT_NAME_LEN);

	for (from_index = 0; from_index <  dir_num; from_index++) {
        dir = fselect(__this->fs, FSEL_BY_NUMBER, from_index);

	    if (dir) {

			__this->text_list[from_index].len = fget_name(dir, name_buf, TEXT_NAME_LEN);

			printf("dir[%d]=%s\n", from_index, name_buf);


			if(have_target_dir) {

				if(!memcmp(target_bp_dir, name_buf, 8)) {

					printf("find target dir %s\n", name_buf);
					__this->fs = fscan_enterdir(__this->fs, name_buf);
					__this->cur_total = __this->fs->file_number + 1;

					printf("%s have %d	files\n", name_buf, __this->cur_total);
					if (have_target_file) {
						printf("have target file\n");
						for (int n = 1; n < __this->cur_total; n++) {
							file = fselect(__this->fs, FSEL_BY_NUMBER, n);
							fget_name(file, name_buf, TEXT_NAME_LEN);

							if(!memcmp(target_bp_file, name_buf, 8)) {
								printf("find target file %s\n", name_buf);
								// TODO:sned file to AT
								#if 0
								if(have_target_addr) {//have target addr
									//send form target addr
								} else {
									//send form 0 addr
								}
								#endif

							} else {
								printf("not target file continue\n");
								continue;
							}

							printf("file[%d]: %s\n", n, name_buf);
							fclose(file);
							file = NULL;
						}

					} else {

						printf("have no target file\n");
						for (int n = 1; n < __this->cur_total; n++) {
							file = fselect(__this->fs, FSEL_BY_NUMBER, n);
							fget_name(file, name_buf, TEXT_NAME_LEN);
							// TODO:sned file to AT
							printf("file[%d]: %s\n", n, name_buf);
							fclose(file);
							file = NULL;
						}

					}


				} else {
					printf("not target dir, continue\n");

					continue;
				}

			} else {//have no target dir

				printf("have no target dir\n");
				__this->fs = fscan_enterdir(__this->fs, name_buf);
				__this->cur_total = __this->fs->file_number + 1;
				printf("%s have %d  files\n", name_buf, __this->cur_total);

				for (int j = 1; j < __this->cur_total; j++) {
					file = fselect(__this->fs, FSEL_BY_NUMBER, j);
					fget_name(file, name_buf, TEXT_NAME_LEN);
					printf("file[%d]: %s\n", j, name_buf);
					// TODO: sned this file
					fclose(file);
		        	file = NULL;
				}

				__this->fs = fscan_exitdir(__this->fs);
				printf("exitdir\n");

				fclose(dir);
				dir = NULL;
			}

	    }
    }

	free(name_buf);
	name_buf = NULL;

    return 0;
}
#endif

#if 0
int file_list_flush(int from_index, u8 *dir_name, u8 *file_name)
{

    FILE *dir = NULL;
    FILE *file = NULL;
	int from_index;

    int i = 0;
   /// char *name_buf = NULL;
	int dir_num = __this->cur_total;

    for (i = 0; i < sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]); i++) {
        memset(__this->text_list[i].fname, 0, TEXT_NAME_LEN);
        __this->text_list[i].len = 0;
    }
	printf("file_list_flush  -------------\r\n");

	//name_buf = malloc(TEXT_NAME_LEN);

	for (from_index = 0; from_index <  dir_num; from_index++) {
        dir = fselect(__this->fs, FSEL_BY_NUMBER, from_index);

	    if (dir) {

			__this->text_list[from_index].len = fget_name(dir, dir_name, TEXT_NAME_LEN);

			printf("dir[%d]=%s\n", from_index, dir);


			if(have_target_dir) {

				if(!memcmp(target_bp_dir, dir_name, 8)) {

					printf("find target dir %s\n", dir_name);
					__this->fs = fscan_enterdir(__this->fs, dir_name);
					__this->cur_total = __this->fs->file_number + 1;

					printf("%s have %d	files\n", dir_name, __this->cur_total);
					if (have_target_file) {
						printf("have target file\n");
						for (int n = 1; n < __this->cur_total; n++) {
							file = fselect(__this->fs, FSEL_BY_NUMBER, n);
							fget_name(file, file_name, TEXT_NAME_LEN);

							if(!memcmp(target_bp_file, file_name, 8)) {
								printf("find target file %s\n", file_name);
								// TODO:find   the file to AT


							} else {
								printf("not target file continue\n");
								continue;
							}

							printf("file[%d]: %s\n", n, file_name);
							fclose(file);
							file = NULL;
						}

					} else {

						printf("have no target file\n");
						for (int n = 1; n < __this->cur_total; n++) {
							file = fselect(__this->fs, FSEL_BY_NUMBER, n);
							fget_name(file, file_name, TEXT_NAME_LEN);
							// TODO:sned file to AT
							printf("file[%d]: %s\n", n, file_name);
							fclose(file);
							file = NULL;
						}

					}


				} else {
					printf("not target dir, continue\n");

					continue;
				}

			} else {//have no target dir

				printf("have no target dir\n");
				__this->fs = fscan_enterdir(__this->fs, dir_name);
				__this->cur_total = __this->fs->file_number + 1;
				printf("%s have %d  files\n", dir_name, __this->cur_total);

				for (int j = 1; j < __this->cur_total; j++) {
					file = fselect(__this->fs, FSEL_BY_NUMBER, j);

					if (!file) {
						fget_name(file, file_name, TEXT_NAME_LEN);// get name for send
						printf("file[%d]: %s\n", j, file_name);
						fclose(file);
		        		file = NULL;
						printf("find file %s break\n", file_name);
						break;
					} else {
						fclose(file);
		        		file = NULL;
						printf("no file return");
						break;
					}

				}

			}

	    }

		__this->fs = fscan_exitdir(__this->fs);
		printf("exitdir\n");

		if (dir) {

			fclose(dir);
			dir = NULL;
		}

    }

    return 0;
}
#endif
#if 0
bool file_get_next_file(u8 *dir_name, u8 *file_name)
{
    FILE *dir = NULL;
    FILE *file = NULL;
	int from_index = 0;
	bool find_target_file = false;
    int i = 0;
    char *name_buf = NULL;
	u8 tmp_dir_name[9];
	bool ret = false;

	int dir_num = __this->cur_total;

    for (i = 0; i < sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]); i++) {
        memset(__this->text_list[i].fname, 0, TEXT_NAME_LEN);
        __this->text_list[i].len = 0;
    }
	printf("file_list_flush  -------------\r\n");

	name_buf = malloc(TEXT_NAME_LEN);

	for (from_index = 0; from_index <  dir_num; from_index++) {
        dir = fselect(__this->fs, FSEL_BY_NUMBER, from_index);

	    if (dir) {

			__this->text_list[from_index].len = fget_name(dir, name_buf, TEXT_NAME_LEN);

			memcpy(tmp_dir_name, name_buf, 9);
			printf("dir[%d]=%s\n", from_index, tmp_dir_name);

#if 1
			if(have_target_dir) {

				if(!memcmp(target_bp_dir, name_buf, 8)) {

					printf("find target dir %s\n", name_buf);
					__this->fs = fscan_enterdir(__this->fs, name_buf);
					__this->cur_total = __this->fs->file_number + 1;

					printf("%s have %d	files\n", name_buf, __this->cur_total);
					if (have_target_file) {
						printf("have target file\n");
						for (int n = 1; n < __this->cur_total; n++) {
							file = fselect(__this->fs, FSEL_BY_NUMBER, n);
							fget_name(file, name_buf, TEXT_NAME_LEN);
							if(!memcmp(target_bp_file, name_buf, 8)) {
								printf("find target file %s\n", name_buf);
								find_target_file = true;

							} else {
								printf("not target file continue\n");
								if (find_target_file) {
									memcpy(file_name, name_buf, 10 + 1);
									memcpy(dir_name, tmp_dir_name, 8 + 1);
									printf("-------find the next file is %s/%s\n", dir_name, file_name);
									ret = true;
								}
								find_target_file = false;
								continue;
							}

							if (n == (__this->cur_total - 1)){//target is last file in dir
								find_target_file = false;
								have_target_dir = false;
							}

							printf("file[%d]: %s\n", n, name_buf);
							fclose(file);
							file = NULL;

						}

					}


				} else {
					printf("not target dir, continue\n");

					continue;
				}

			} else {//have no target dir

				printf("have no target dir\n");
				__this->fs = fscan_enterdir(__this->fs, name_buf);
				__this->cur_total = __this->fs->file_number + 1;
				printf("%s have %d  files\n", name_buf, __this->cur_total);

				memcpy(file_name, name_buf, 10 + 1);

				for (int j = 1; j < __this->cur_total; j++) {
					file = fselect(__this->fs, FSEL_BY_NUMBER, j);
					fget_name(file, name_buf, TEXT_NAME_LEN);
					printf("file[%d]: %s\n", j, name_buf);

					if (file && find_target_file == false) {
						memcpy(dir_name, tmp_dir_name, 8 + 1);
						printf("+++++++ find the next file is %s/%s\n", dir_name, file_name);
						find_target_file = true;
						ret = true;
					}


					fclose(file);
		        	file = NULL;
					break;
				}

				__this->fs = fscan_exitdir(__this->fs);
				printf("exitdir\n");

			}
#endif
		fclose(dir);
		dir = NULL;

	    }
    }

	free(name_buf);
	name_buf = NULL;

    return ret;

}
#endif

bool check_dir_is_sended(u8 *dir_name)
{
	bool ret;

	if ((dir_name[0] == 'S' && dir_name[1] == 'S') ||
		(dir_name[0] == 's' && dir_name[1] == 's')) {
		ret = true;
		printf("this dir sended, skip it\n");
	} else {
		ret = false;
		printf("this dir is not sended, enter in it\n");
	}

	return ret;
}

bool check_file_is_sended(u8 *file_name)
{
	bool ret;

	if ((file_name[0] == 's' /*&& file_name[1] == 's'*/)
		||(file_name[0] == 'S' /*&& file_name[1] == 'S'*/)) {
		ret = true;
		printf("this file sended, skip it\n");
	} else {
		ret = false;
		printf("this file is not sended\n");
	}

	return ret;
}


bool check_the_file_all_sended_indir(u8 *dir_name)
{
	bool ret;
	static u8 temp_file_name[20];
	static FILE* fb = NULL;
	int len;
	int num = 0;

	__this->fs = fscan_enterdir(__this->fs, dir_name);
	__this->cur_total = __this->fs->file_number + 1;

	printf("dir %s have %d	files\n", temp_file_name, __this->cur_total);

	for (int n = 1; n < __this->cur_total; n++) {
		fb = fselect(__this->fs, FSEL_BY_NUMBER, n);
		len = fget_name(fb, temp_file_name, TEXT_NAME_LEN);

		fclose(fb);
		fb = NULL;

		printf("file[%d]: %s\n", n, temp_file_name);
		ret = check_file_is_sended(temp_file_name);

		if(ret) {
			printf("%s sended, num++\n", dir_name);
			num++;
		}
	}
	__this->fs = fscan_exitdir(__this->fs);
	printf("exitdir\n");

	if (num == __this->fs->file_number) {
		printf("this dir all file has been sned, need rename this dir\n");
		ret = true;
	} else
		ret = false;

	return ret;
}

int rename_dir_name(FILE* fs, char *dir_name);

bool file_get_next_file(u8 *dir_name, u8 *file_name)
{
    FILE *dir = NULL;
    FILE *file = NULL;
	int from_index = 0;
    int i = 0;
    char temp_file_name[20] = {0};
	char tmp_dir_name[20];
	bool ret = false;
	int len;
	int rt;
	bool result = false;
	bool need_rename_dir = false;

	int dir_num = __this->cur_total;

    for (i = 0; i < sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]); i++) {
        memset(__this->text_list[i].fname, 0, TEXT_NAME_LEN);
        __this->text_list[i].len = 0;
    }
	printf("file_list_flush  -------------\r\n");


	for (from_index = 0; from_index <  dir_num; from_index++) {
        dir = fselect(__this->fs, FSEL_BY_NUMBER, from_index);

	    if (dir) {

			__this->text_list[from_index].len = fget_name(dir, tmp_dir_name, TEXT_NAME_LEN);

			printf("dir[%d]= %s\n", from_index, tmp_dir_name);

			ret = check_dir_is_sended(tmp_dir_name);

			if (!ret) { //dir not send over
					__this->fs = fscan_enterdir(__this->fs, tmp_dir_name);
					__this->cur_total = __this->fs->file_number + 1;

					printf("%s have %d	files\n", tmp_dir_name, __this->cur_total);

					for (int n = 1; n < __this->cur_total; n++) {
						file = fselect(__this->fs, FSEL_BY_NUMBER, n);
						len = fget_name(file, temp_file_name, TEXT_NAME_LEN);

						fclose(file);
						file = NULL;

						printf("len = %d  file[%d]: %s\n", len, n, temp_file_name);
						ret = check_file_is_sended(temp_file_name);

						if(!ret) { // file not sended
							memcpy(dir_name, tmp_dir_name, 8 + 1);
							memcpy(file_name, temp_file_name, 10 + 1);
							printf("-------find the next file is %s/%s\n", dir_name, file_name);
							result = true;

							break;
						} else { //sended
							printf("file %s is sended , continue\n", temp_file_name);
							//check all file is sended in the dir, if all sended,need rename the dir name
							if (n == (__this->cur_total) - 1) {
								printf("all file sended , rename dir\n");
								need_rename_dir = true;
							}

							continue;
						}
					}

				__this->fs = fscan_exitdir(__this->fs);
				printf("exitdir\n");

				if (result == true)// break out dir loop
					break;


			} else { //dir send over
				printf("dir is sended , continue\n");
			}

			 if (need_rename_dir) {

				need_rename_dir = false;
				rt = rename_dir_name(dir, tmp_dir_name);

				if (rt) {
					printf("rename dir successed\n");
				} else {
					printf("rename dir fail\n");
				}

			}

			fclose(dir);

		}

    }

    return result;

}


int rename_dir_name(FILE* fs, char *dir_name)
{
	char rename[15] = {0};
	int ret;

	sprintf(rename, "%s%s", "ss", &dir_name[2]);

	printf("new dir name is %s\n", rename);

	if (fs) {

		ret = frename(fs, rename);
		if (ret) {
			ret = false;
		} else {
			ret = true;
		}

	}

	return ret;
}




#if 1
//static int file_browse_enter_onchane(void *ctr, enum element_change_event e, void *arg)
int file_browse_enter_onchane(void)
{
		int ret;

        if (!__this) {
            __this = zalloc(sizeof_this);
        }

        if (!__this->fs) {
            if (!dev_manager_get_total(1)) {
                return false;
            }

            void *dev = dev_manager_find_active(1);
            if (!dev) {
				printf("no dev \n");
                return false;
            }
            __this->fs = fscan(dev_manager_get_root_path(dev), RECODER_SCAN_PARAM, 9);

            printf(">>> dir number=%d \n", __this->fs->file_number);
            __this->cur_total = __this->fs->file_number + 1;
        }

		printf("dir cur_total: %d\n", __this->cur_total);

		if(__this->fs->file_number != 0) {

			ret = file_list_flush(0);
		} else {
			printf("have no file\n");
			return false;
		}

		if (__this->fs) {
            fscan_release(__this->fs);
            __this->fs = NULL;
        }

        if (__this) {
            free(__this);
            __this = NULL;
        }

    return true;
}
#endif

bool scan_sd_card_before_get_path(void)
{
	bool ret;

	if (!__this) {
		__this = zalloc(sizeof_this);
	}

	if (!__this->fs) {
		if (!dev_manager_get_total(1)) {
			return false;
		}

		void *dev = dev_manager_find_active(1);
		if (!dev) {
			printf("no dev \n");
			return false;
		}
		__this->fs = fscan(dev_manager_get_root_path(dev), RECODER_SCAN_PARAM, 9);

		printf(">>> dir number=%d \n", __this->fs->file_number);
		__this->cur_total = __this->fs->file_number + 1;
	}

	printf("dir cur_total: %d\n", __this->cur_total);

	return true;
}

int get_recoder_file_path(u8 *tmp_dir, u8 *tmp_file)
{

	bool ret = false;

	if(__this->fs->file_number != 0) {

		ret = file_get_next_file(tmp_dir, tmp_file);

		if (ret) {
			printf("find next file ok %s/%s\n", tmp_dir,tmp_file);
		} else {
			printf("not find next file\n");
		}
	} else {
		printf("have no dir\n");
		ret = false;
	}

    return ret;
}


void release_all_fs_source(void)
{

	printf("release_all_fs_source \n");
	if (__this->fs) {
		fscan_release(__this->fs);
		__this->fs = NULL;
	}

	if (__this) {
		free(__this);
		__this = NULL;
	}

}

static int file_browse_onkey(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    int sel_item;
    sel_item = ui_grid_cur_item(grid);
    switch (e->value) {
    case KEY_OK:
        if (file_select_enter(__this->flist_index)) {
           // ui_no_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[sel_item]);
            __this->flist_index = 0;
            file_list_flush(__this->flist_index);
           // ui_grid_set_item(grid, 0);
          //  ui_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[0]);
        }


        break;
    case KEY_DOWN:

        sel_item++;

        __this->flist_index += 1;

        if (sel_item >= __this->cur_total || __this->flist_index >= __this->cur_total) {
            //大于文件数
           // ui_no_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[sel_item - 1]);
            __this->flist_index = 0;
            file_list_flush(__this->flist_index);
           // ui_grid_set_item(grid, 0);
           // ui_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[0]);
           // ui_vslider_set_persent_by_id(MUSIC_FILE_SLIDER, (__this->flist_index + 1) * 100 / __this->cur_total);
            return TRUE;
        }
        //ui_vslider_set_persent_by_id(MUSIC_FILE_SLIDER, (__this->flist_index + 1) * 100 / __this->cur_total);
        if (sel_item >= TEXT_PAGE) {
          //  ui_no_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[sel_item - 1]);
            file_list_flush(__this->flist_index);
         //   ui_grid_set_item(grid, 0);
         //   ui_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[0]);
            return true; //不返回到首项
        }

        return FALSE;

        break;
    case KEY_UP:
        if (sel_item == 0) {
            __this->flist_index = __this->flist_index ? __this->flist_index - 1 : __this->cur_total - 1;
           // ui_no_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[sel_item]);
            file_list_flush(__this->flist_index / TEXT_PAGE * TEXT_PAGE);
           // ui_grid_set_item(grid, __this->flist_index % TEXT_PAGE);
           // ui_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[__this->flist_index % TEXT_PAGE]);
           // ui_vslider_set_persent_by_id(MUSIC_FILE_SLIDER, (__this->flist_index + 1) * 100 / __this->cur_total);
            return true; //不跳转到最后一项
        }
        __this->flist_index--;
      //  ui_vslider_set_persent_by_id(MUSIC_FILE_SLIDER, (__this->flist_index + 1) * 100 / __this->cur_total);
        return FALSE;
        break;

    default:
        return false;
    }
    return false;
}

#endif
