
# make 编译并下载
# make VERBOSE=1 显示编译详细过程
# make clean 清除编译临时文件

# 工具路径设置
ifeq ($(OS), Windows_NT)
# Windows 下工具链位置
TOOL_DIR := C:/JL/pi32/bin
CC    := clang.exe
CXX   := clang.exe
LD    := pi32v2-lto-wrapper.exe
AR    := pi32v2-lto-ar.exe
MKDIR := mkdir_win -p
RM    := rm -rf

SYS_LIB_DIR := C:/JL/pi32/pi32v2-lib/r3
SYS_INC_DIR := C:/JL/pi32/pi32v2-include

## 后处理脚本
POST_SCRIPT     := cpu\br23\tools\download.bat
RUN_POST_SCRIPT := $(POST_SCRIPT)
else
# Linux 下工具链位置
TOOL_DIR := /opt/jieli/pi32v2/bin
CC    := clang
CXX   := clang++
LD    := lto-wrapper
AR    := lto-ar
MKDIR := mkdir -p
RM    := rm -rf

SYS_LIB_DIR := $(TOOL_DIR)/../lib/r3
SYS_INC_DIR := $(TOOL_DIR)/../include

## 后处理脚本
POST_SCRIPT     := cpu/br23/tools/download.sh
RUN_POST_SCRIPT := bash $(POST_SCRIPT)
endif

CC  := $(TOOL_DIR)/$(CC)
CXX := $(TOOL_DIR)/$(CXX)
LD  := $(TOOL_DIR)/$(LD)
AR  := $(TOOL_DIR)/$(AR)
# 输出文件设置
OUT_ELF   := cpu/br23/tools/sdk.elf
OBJ_FILE  := $(OUT_ELF).objs.txt
# 编译路径设置
BUILD_DIR := objs

# 编译参数设置
CFLAGS := \
	-target pi32v2 \
	-mcpu=r3 \
	-integrated-as \
	-flto \
	-Wuninitialized \
	-Wno-invalid-noreturn \
	-fno-common \
	-integrated-as \
	-Oz \
	-g \
	-flto \
	-fallow-pointer-null \
	-fprefer-gnu-section \
	-Wno-shift-negative-value \
	-Wundef \
	-fms-extensions \


# 宏定义
DEFINES := \
	-DSUPPORT_MS_EXTENSIONS \
	-DCONFIG_CPU_BR23 \
	-DCONFIG_RELEASE_ENABLE \
	-DCONFIG_EQ_SUPPORT_ASYNC \
	-DCONFIG_MIXER_CYCLIC \
	-DCONFIG_FREE_RTOS_ENABLE \
	-DSUPPORT_BLUETOOTH_PROFILE_RELEASE \
	-DCONFIG_SOUNDBOX \
	-DEVENT_HANDLER_NUM_CONFIG=2 \
	-DEVENT_TOUCH_ENABLE_CONFIG=0 \
	-DEVENT_POOL_SIZE_CONFIG=256 \
	-DCONFIG_EVENT_KEY_MAP_ENABLE=0 \
	-DTIMER_POOL_NUM_CONFIG=15 \
	-DAPP_ASYNC_POOL_NUM_CONFIG=0 \
	-DVFS_ENABLE=1 \
	-DUSE_SDFILE_NEW=1 \
	-DSDFILE_STORAGE=1 \
	-DVFS_FILE_POOL_NUM_CONFIG=1 \
	-DFS_VERSION=0x020001 \
	-DFATFS_VERSION=0x020101 \
	-DSDFILE_VERSION=0x020000 \
	-DVM_MAX_SIZE_CONFIG=64*1024 \
	-DVM_ITEM_MAX_NUM=256 \
	-DCONFIG_TWS_ENABLE \
	-DCONFIG_SOUNDBOX_CASE_ENABLE \
	-DAUDIO_REC_POOL_NUM=1 \
	-DAUDIO_DEC_POOL_NUM=3 \
	-DAUDIO_LOCAL_DEC_USE_MALLOC \
	-DCONFIG_TONE_LOCK_BY_BT_TIME \
	-DCONFIG_LMP_CONN_SUSPEND_ENABLE \
	-DCONFIG_BTCTRLER_TASK_DEL_ENABLE \
	-DCONFIG_UPDATA_ENABLE \
	-DCONFIG_OTA_UPDATA_ENABLE \
	-DCONFIG_ITEM_FORMAT_VM \
	-DCONFIG_MMU_ENABLE \
	-DCONFIG_SBC_CODEC_HW \
	-DCONFIG_MSBC_CODEC_HW \
	-DCONFIG_AEC_M=640 \
	-DCONFIG_AUDIO_ONCHIP \
	-DCONFIG_MEDIA_DEVELOP_ENABLE \
	-DCONFIG_MEDIA_EFFECT_DISABLE \
	-D__GCC_PI32V2__ \
	-DCONFIG_NEW_ECC_ENABLE \


# 头文件搜索路径
INCLUDES := \
	-Iinclude_lib \
	-Iinclude_lib/driver \
	-Iinclude_lib/driver/device \
	-Iinclude_lib/driver/cpu/br23 \
	-Iinclude_lib/system \
	-Iinclude_lib/system/generic \
	-Iinclude_lib/system/device \
	-Iinclude_lib/system/fs \
	-Iinclude_lib/system/ui \
	-Iinclude_lib/btctrler \
	-Iinclude_lib/btctrler/port/br23 \
	-Iinclude_lib/update \
	-Iinclude_lib/agreement \
	-Iinclude_lib/btstack/third_party/common \
	-Iinclude_lib/btstack/third_party/rcsp \
	-Iinclude_lib/media/media_develop \
	-Iinclude_lib/media/media_develop/media \
	-Iinclude_lib/media/media_develop/media/cpu/br23 \
	-Iinclude_lib/media/media_develop/media/cpu/br23/asm \
	-Icpu/br23/audio_mic \
	-Iapps/soundbox/include \
	-Iapps/soundbox/include/task_manager \
	-Iapps/soundbox/include/task_manager/bt \
	-Iapps/soundbox/include/user_api \
	-Iapps/common \
	-Iapps/common/device \
	-Iapps/common/audio \
	-Iapps/common/audio/stream \
	-Iapps/common/power_manage \
	-Iapps/common/charge_box \
	-Iapps/common/third_party_profile/common \
	-Iapps/common/third_party_profile/jieli \
	-Iapps/common/third_party_profile/jieli/trans_data_demo \
	-Iapps/common/third_party_profile/jieli/online_db \
	-Iapps/common/third_party_profile/jieli/JL_rcsp \
	-Iapps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata \
	-Iapps/common/third_party_profile/jieli/JL_rcsp/bt_trans_data \
	-Iapps/common/third_party_profile/jieli/JL_rcsp/adv_rcsp_protocol \
	-Iapps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting \
	-Iapps/common/dev_manager \
	-Iapps/common/file_operate \
	-Iapps/common/music \
	-Iapps/common/include \
	-Iapps/common/config/include \
	-Iapps/soundbox/board/br23 \
	-Icpu/br23 \
	-Icpu/br23/audio_common \
	-Icpu/br23/audio_dec \
	-Icpu/br23/audio_enc \
	-Icpu/br23/audio_effect \
	-Icpu/br23/localtws \
	-Iinclude_lib/btstack/third_party/baidu \
	-Iinclude_lib/btstack \
	-Iapps/common/third_party_profile/tm_gma_protocol \
	-Iapps/common/third_party_profile/dma_deal \
	-Iapps/common/third_party_profile/tuya_protocol \
	-Iapps/common/third_party_profile/tuya_protocol/app/demo \
	-Iapps/common/third_party_profile/tuya_protocol/app/product_test \
	-Iapps/common/third_party_profile/tuya_protocol/app/uart_common \
	-Iapps/common/third_party_profile/tuya_protocol/extern_components/mbedtls \
	-Iapps/common/third_party_profile/tuya_protocol/port \
	-Iapps/common/third_party_profile/tuya_protocol/sdk/include \
	-Iapps/common/third_party_profile/tuya_protocol/sdk/lib \
	-Iapps/common/usb \
	-Iapps/common/usb/device \
	-Iapps/common/usb/host \
	-Iapps/soundbox/smartbox \
	-Iapps/soundbox/smartbox/bt_manage/bt_trans_data \
	-Iapps/soundbox/smartbox/bt_manage \
	-Iapps/soundbox/smartbox/smartbox_setting \
	-Iapps/soundbox/smartbox/smartbox_setting/smartbox_misc_setting \
	-Iapps/soundbox/smartbox/smartbox_setting_opt \
	-Iapps/soundbox/smartbox/smartbox_update \
	-Iapps/soundbox/smartbox/file_transfer \
	-Iapps/soundbox/smartbox/tuya \
	-Iapps/soundbox/include/third_party_profile/gma \
	-Iapps/soundbox/include/third_party_profile/dma \
	-Iapps/soundbox/include/ui/color_led \
	-I$(SYS_INC_DIR) \


# 需要编译的 .c 文件
c_SRC_FILES := \
	apps/common/audio/audio_digital_vol.c \
	apps/common/audio/decode/audio_key_tone.c \
	apps/common/audio/decode/decode.c \
	apps/common/audio/encode/encode_write_file.c \
	apps/common/audio/sine_make.c \
	apps/common/audio/stream/stream_entry.c \
	apps/common/audio/stream/stream_src.c \
	apps/common/audio/stream/stream_sync.c \
	apps/common/audio/uartPcmSender.c \
	apps/common/bt_common/bt_test_api.c \
	apps/common/charge_box/chargeIc_manage.c \
	apps/common/charge_box/chgbox_box.c \
	apps/common/charge_box/chgbox_ctrl.c \
	apps/common/charge_box/chgbox_det.c \
	apps/common/charge_box/chgbox_handshake.c \
	apps/common/charge_box/chgbox_ui.c \
	apps/common/charge_box/chgbox_ui_drv_pwmled.c \
	apps/common/charge_box/chgbox_ui_drv_timer.c \
	apps/common/charge_box/chgbox_wireless.c \
	apps/common/config/app_config.c \
	apps/common/config/bt_profile_config.c \
	apps/common/config/ci_transport_uart.c \
	apps/common/debug/debug.c \
	apps/common/debug/debug_lite.c \
	apps/common/dev_manager/dev_manager.c \
	apps/common/dev_manager/dev_reg.c \
	apps/common/dev_manager/dev_update.c \
	apps/common/device/detection.c \
	apps/common/device/fm/bk1080/Bk1080.c \
	apps/common/device/fm/fm_inside/fm_inside.c \
	apps/common/device/fm/fm_manage.c \
	apps/common/device/fm/qn8035/QN8035.c \
	apps/common/device/fm/rda5807/RDA5807.c \
	apps/common/device/fm_emitter/ac3433/ac3433.c \
	apps/common/device/fm_emitter/fm_emitter_manage.c \
	apps/common/device/fm_emitter/fm_inside/fm_emitter_inside.c \
	apps/common/device/fm_emitter/qn8007/qn8007.c \
	apps/common/device/fm_emitter/qn8027/qn8027.c \
	apps/common/device/gSensor/SC7A20.c \
	apps/common/device/gSensor/da230.c \
	apps/common/device/gSensor/gSensor_manage.c \
	apps/common/device/nandflash/nandflash.c \
	apps/common/device/norflash/norflash.c \
	apps/common/fat_nor/nor_fs.c \
	apps/common/fat_nor/phone_rec_fs.c \
	apps/common/fat_nor/virfat_flash.c \
	apps/common/file_operate/file_api.c \
	apps/common/file_operate/file_bs_deal.c \
	apps/common/file_operate/file_manager.c \
	apps/common/iap/iAP_des.c \
	apps/common/iap/iAP_device.c \
	apps/common/iap/iAP_iic.c \
	apps/common/key/adkey.c \
	apps/common/key/adkey_rtcvdd.c \
	apps/common/key/ctmu_touch_key.c \
	apps/common/key/iokey.c \
	apps/common/key/irkey.c \
	apps/common/key/key_driver.c \
	apps/common/key/rdec_key.c \
	apps/common/key/slidekey.c \
	apps/common/key/touch_key.c \
	apps/common/music/breakpoint.c \
	apps/common/music/general_player.c \
	apps/common/music/music_decrypt.c \
	apps/common/music/music_id3.c \
	apps/common/music/music_player.c \
	apps/common/rec_nor/nor_interface.c \
	apps/common/rec_nor/nor_rec_fs.c \
	apps/common/third_party_profile/common/3th_profile_api.c \
	apps/common/third_party_profile/common/custom_cfg.c \
	apps/common/third_party_profile/common/mic_rec.c \
	apps/common/third_party_profile/dma_deal/dma_deal.c \
	apps/common/third_party_profile/dma_deal/dueros_spp_user.c \
	apps/common/third_party_profile/dma_deal/le_dueros_dma.c \
	apps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting/adv_bt_name_setting.c \
	apps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting/adv_key_setting.c \
	apps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting/adv_led_setting.c \
	apps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting/adv_mic_setting.c \
	apps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting/adv_time_stamp_setting.c \
	apps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting/adv_work_setting.c \
	apps/common/third_party_profile/jieli/JL_rcsp/adv_rcsp_protocol/rcsp_adv_opt.c \
	apps/common/third_party_profile/jieli/JL_rcsp/adv_rcsp_protocol/rcsp_adv_tws_sync.c \
	apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata/rcsp_adv_user_update.c \
	apps/common/third_party_profile/jieli/hid_user.c \
	apps/common/third_party_profile/jieli/le_client_demo.c \
	apps/common/third_party_profile/jieli/multi_demo/le_multi_client.c \
	apps/common/third_party_profile/jieli/multi_demo/le_multi_common.c \
	apps/common/third_party_profile/jieli/multi_demo/le_multi_trans.c \
	apps/common/third_party_profile/jieli/online_db/online_db_deal.c \
	apps/common/third_party_profile/jieli/online_db/spp_online_db.c \
	apps/common/third_party_profile/jieli/trans_data_demo/le_trans_data.c \
	apps/common/third_party_profile/jieli/trans_data_demo/spp_trans_data.c \
	apps/common/third_party_profile/tm_gma_protocol/gma_license.c \
	apps/common/third_party_profile/tm_gma_protocol/gma_spp_user.c \
	apps/common/third_party_profile/tm_gma_protocol/le_gma.c \
	apps/common/third_party_profile/tm_gma_protocol/tm_gma_deal.c \
	apps/common/third_party_profile/tuya_protocol/app/demo/tuya_ble_app_demo.c \
	apps/common/third_party_profile/tuya_protocol/app/demo/tuya_ota.c \
	apps/common/third_party_profile/tuya_protocol/app/product_test/tuya_ble_app_production_test.c \
	apps/common/third_party_profile/tuya_protocol/app/uart_common/tuya_ble_app_uart_common_handler.c \
	apps/common/third_party_profile/tuya_protocol/extern_components/mbedtls/aes.c \
	apps/common/third_party_profile/tuya_protocol/extern_components/mbedtls/ccm.c \
	apps/common/third_party_profile/tuya_protocol/extern_components/mbedtls/hmac.c \
	apps/common/third_party_profile/tuya_protocol/extern_components/mbedtls/md5.c \
	apps/common/third_party_profile/tuya_protocol/extern_components/mbedtls/sha1.c \
	apps/common/third_party_profile/tuya_protocol/extern_components/mbedtls/sha256.c \
	apps/common/third_party_profile/tuya_protocol/port/JL_to_tuya_ble_port_peripheral.c \
	apps/common/third_party_profile/tuya_protocol/port/tuya_ble_port.c \
	apps/common/third_party_profile/tuya_protocol/port/tuya_ble_port_AD697x.c \
	apps/common/third_party_profile/tuya_protocol/port/tuya_ble_port_peripheral.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_api.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_bulk_data.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_data_handler.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_event.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_event_handler.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_event_handler_weak.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_feature_weather.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_gatt_send_queue.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_heap.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_main.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_mem.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_mutli_tsf_protocol.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_queue.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_storage.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_unix_time.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_utils.c \
	apps/common/ui/lcd/lcd_ui_api.c \
	apps/common/ui/lcd_simple/lcd_simple_api.c \
	apps/common/ui/lcd_simple/ui.c \
	apps/common/ui/lcd_simple/ui_mainmenu.c \
	apps/common/ui/led7/led7_ui_api.c \
	apps/common/update/norflash_ufw_update.c \
	apps/common/update/norflash_update.c \
	apps/common/update/testbox_update.c \
	apps/common/update/uart_update.c \
	apps/common/update/uart_update_master.c \
	apps/common/update/update.c \
	apps/common/usb/device/cdc.c \
	apps/common/usb/device/descriptor.c \
	apps/common/usb/device/hid.c \
	apps/common/usb/device/msd.c \
	apps/common/usb/device/msd_upgrade.c \
	apps/common/usb/device/task_pc.c \
	apps/common/usb/device/uac1.c \
	apps/common/usb/device/uac_stream.c \
	apps/common/usb/device/usb_device.c \
	apps/common/usb/device/user_setup.c \
	apps/common/usb/host/adb.c \
	apps/common/usb/host/aoa.c \
	apps/common/usb/host/apple_mfi.c \
	apps/common/usb/host/audio.c \
	apps/common/usb/host/audio_demo.c \
	apps/common/usb/host/hid.c \
	apps/common/usb/host/usb_bulk_transfer.c \
	apps/common/usb/host/usb_ctrl_transfer.c \
	apps/common/usb/host/usb_host.c \
	apps/common/usb/host/usb_storage.c \
	apps/common/usb/usb_config.c \
	apps/common/usb/usb_host_config.c \
	apps/soundbox/aec/br23/audio_aec.c \
	apps/soundbox/app_main.c \
	apps/soundbox/board/br23/board_ac6083a/board_ac6083a.c \
	apps/soundbox/board/br23/board_ac6083a/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac6083a/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac6083a/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac6083a/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac6083a/key_table/touch_key_table.c \
	apps/soundbox/board/br23/board_ac6083a_iap/board_ac6083a_iap.c \
	apps/soundbox/board/br23/board_ac6083a_iap/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac6083a_iap/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac6083a_iap/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac6083a_iap/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac6083a_iap/key_table/touch_key_table.c \
	apps/soundbox/board/br23/board_ac6951g/board_ac6951g.c \
	apps/soundbox/board/br23/board_ac6951g/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac6951g/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac6951g/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac6951g/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac6951g/key_table/touch_key_table.c \
	apps/soundbox/board/br23/board_ac6952e_lighter/board_ac6952e_lighter.c \
	apps/soundbox/board/br23/board_ac6952e_lighter/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac6952e_lighter/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac6952e_lighter/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac6952e_lighter/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac6952e_lighter/key_table/touch_key_table.c \
	apps/soundbox/board/br23/board_ac6954a_demo/board_ac6954a_demo.c \
	apps/soundbox/board/br23/board_ac6954a_demo/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac6954a_demo/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac6954a_demo/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac6954a_demo/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac6954a_demo/key_table/touch_key_table.c \
	apps/soundbox/board/br23/board_ac6955f_headset_mono/board_ac6955f_headset_mono.c \
	apps/soundbox/board/br23/board_ac6955f_headset_mono/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac6955f_headset_mono/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac6955f_headset_mono/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac6955f_headset_mono/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac6955f_headset_mono/key_table/touch_key_table.c \
	apps/soundbox/board/br23/board_ac695x_btemitter/board_ac695x_btemitter.c \
	apps/soundbox/board/br23/board_ac695x_btemitter/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac695x_btemitter/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac695x_btemitter/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac695x_btemitter/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac695x_btemitter/key_table/touch_key_table.c \
	apps/soundbox/board/br23/board_ac695x_charging_bin/board_ac695x_charging_bin.c \
	apps/soundbox/board/br23/board_ac695x_charging_bin/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac695x_charging_bin/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac695x_charging_bin/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac695x_charging_bin/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac695x_charging_bin/key_table/touch_key_table.c \
	apps/soundbox/board/br23/board_ac695x_demo/board_ac695x_demo.c \
	apps/soundbox/board/br23/board_ac695x_demo/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac695x_demo/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac695x_demo/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac695x_demo/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac695x_demo/key_table/touch_key_table.c \
	apps/soundbox/board/br23/board_ac695x_lcd/board_ac695x_lcd.c \
	apps/soundbox/board/br23/board_ac695x_lcd/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac695x_lcd/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac695x_lcd/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac695x_lcd/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac695x_lcd/key_table/touch_key_table.c \
	apps/soundbox/board/br23/board_ac695x_multimedia_charging_bin/board_ac695x_multimedia_charging_bin.c \
	apps/soundbox/board/br23/board_ac695x_multimedia_charging_bin/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac695x_multimedia_charging_bin/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac695x_multimedia_charging_bin/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac695x_multimedia_charging_bin/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac695x_multimedia_charging_bin/key_table/touch_key_table.c \
	apps/soundbox/board/br23/board_ac695x_smartbox/board_ac695x_smartbox.c \
	apps/soundbox/board/br23/board_ac695x_smartbox/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac695x_smartbox/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac695x_smartbox/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac695x_smartbox/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac695x_smartbox/key_table/touch_key_table.c \
	apps/soundbox/board/br23/board_ac695x_soundcard/board_ac695x_soundcard.c \
	apps/soundbox/board/br23/board_ac695x_soundcard/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac695x_soundcard/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac695x_soundcard/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac695x_soundcard/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac695x_soundcard/key_table/touch_key_table.c \
	apps/soundbox/board/br23/board_ac695x_tws/board_ac695x_tws.c \
	apps/soundbox/board/br23/board_ac695x_tws/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac695x_tws/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac695x_tws/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac695x_tws/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac695x_tws/key_table/touch_key_table.c \
	apps/soundbox/board/br23/board_ac695x_tws_box/board_ac695x_tws_box.c \
	apps/soundbox/board/br23/board_ac695x_tws_box/key_table/adkey_table.c \
	apps/soundbox/board/br23/board_ac695x_tws_box/key_table/iokey_table.c \
	apps/soundbox/board/br23/board_ac695x_tws_box/key_table/irkey_table.c \
	apps/soundbox/board/br23/board_ac695x_tws_box/key_table/rdec_key_table.c \
	apps/soundbox/board/br23/board_ac695x_tws_box/key_table/touch_key_table.c \
	apps/soundbox/board/br23/irq_config.c \
	apps/soundbox/common/dev_status.c \
	apps/soundbox/common/init.c \
	apps/soundbox/common/task_table.c \
	apps/soundbox/common/tone_table.c \
	apps/soundbox/common/user_cfg.c \
	apps/soundbox/font/fontinit.c \
	apps/soundbox/log_config/app_config.c \
	apps/soundbox/log_config/lib_btctrler_config.c \
	apps/soundbox/log_config/lib_btstack_config.c \
	apps/soundbox/log_config/lib_driver_config.c \
	apps/soundbox/log_config/lib_media_config.c \
	apps/soundbox/log_config/lib_system_config.c \
	apps/soundbox/log_config/lib_update_config.c \
	apps/soundbox/power_manage/app_charge.c \
	apps/soundbox/power_manage/app_chargestore.c \
	apps/soundbox/power_manage/app_power_manage.c \
	apps/soundbox/smartbox/browser/browser.c \
	apps/soundbox/smartbox/bt_manage/bt_trans_data/le_smartbox_adv.c \
	apps/soundbox/smartbox/bt_manage/bt_trans_data/le_smartbox_module.c \
	apps/soundbox/smartbox/bt_manage/smartbox_bt_manage.c \
	apps/soundbox/smartbox/cmd_data_deal/cmd_recieve.c \
	apps/soundbox/smartbox/cmd_data_deal/cmd_recieve_no_respone.c \
	apps/soundbox/smartbox/cmd_data_deal/cmd_respone.c \
	apps/soundbox/smartbox/cmd_data_deal/cmd_user.c \
	apps/soundbox/smartbox/cmd_data_deal/command.c \
	apps/soundbox/smartbox/cmd_data_deal/data_recieve.c \
	apps/soundbox/smartbox/cmd_data_deal/data_recieve_no_respone.c \
	apps/soundbox/smartbox/cmd_data_deal/data_respone.c \
	apps/soundbox/smartbox/config.c \
	apps/soundbox/smartbox/event.c \
	apps/soundbox/smartbox/feature.c \
	apps/soundbox/smartbox/file_transfer/dev_format.c \
	apps/soundbox/smartbox/file_transfer/file_delete.c \
	apps/soundbox/smartbox/file_transfer/file_transfer.c \
	apps/soundbox/smartbox/func_cmd/bt_func.c \
	apps/soundbox/smartbox/func_cmd/fm_func.c \
	apps/soundbox/smartbox/func_cmd/linein_func.c \
	apps/soundbox/smartbox/func_cmd/music_func.c \
	apps/soundbox/smartbox/func_cmd/rtc_func.c \
	apps/soundbox/smartbox/function.c \
	apps/soundbox/smartbox/smartbox.c \
	apps/soundbox/smartbox/smartbox_rcsp_manage.c \
	apps/soundbox/smartbox/smartbox_setting/adv_bt_name_setting.c \
	apps/soundbox/smartbox/smartbox_setting/adv_key_setting.c \
	apps/soundbox/smartbox/smartbox_setting/adv_led_setting.c \
	apps/soundbox/smartbox/smartbox_setting/adv_mic_setting.c \
	apps/soundbox/smartbox/smartbox_setting/adv_time_stamp_setting.c \
	apps/soundbox/smartbox/smartbox_setting/adv_work_setting.c \
	apps/soundbox/smartbox/smartbox_setting/smartbox_color_led_setting.c \
	apps/soundbox/smartbox/smartbox_setting/smartbox_eq_setting.c \
	apps/soundbox/smartbox/smartbox_setting/smartbox_high_low_vol_setting.c \
	apps/soundbox/smartbox/smartbox_setting/smartbox_karaoke_eq_setting.c \
	apps/soundbox/smartbox/smartbox_setting/smartbox_karaoke_setting.c \
	apps/soundbox/smartbox/smartbox_setting/smartbox_misc_setting/smartbox_misc_drc_setting.c \
	apps/soundbox/smartbox/smartbox_setting/smartbox_misc_setting/smartbox_misc_reverbration_setting.c \
	apps/soundbox/smartbox/smartbox_setting/smartbox_misc_setting/smartbox_misc_setting.c \
	apps/soundbox/smartbox/smartbox_setting/smartbox_music_info_setting.c \
	apps/soundbox/smartbox/smartbox_setting/smartbox_vol_setting.c \
	apps/soundbox/smartbox/smartbox_setting_opt/smartbox_adv_bluetooth.c \
	apps/soundbox/smartbox/smartbox_setting_opt/smartbox_setting_opt.c \
	apps/soundbox/smartbox/smartbox_setting_opt/smartbox_setting_sync.c \
	apps/soundbox/smartbox/smartbox_task.c \
	apps/soundbox/smartbox/smartbox_update/rcsp_ch_loader_download.c \
	apps/soundbox/smartbox/smartbox_update/smartbox_update.c \
	apps/soundbox/smartbox/smartbox_update/smartbox_update_tws.c \
	apps/soundbox/smartbox/switch_device.c \
	apps/soundbox/smartbox/tuya/tuya_demo.c \
	apps/soundbox/soundcard/lamp.c \
	apps/soundbox/soundcard/notice.c \
	apps/soundbox/soundcard/peripheral.c \
	apps/soundbox/soundcard/soundcard.c \
	apps/soundbox/task_manager/app_common.c \
	apps/soundbox/task_manager/app_task_switch.c \
	apps/soundbox/task_manager/bt/bt.c \
	apps/soundbox/task_manager/bt/bt_ble.c \
	apps/soundbox/task_manager/bt/bt_emitter.c \
	apps/soundbox/task_manager/bt/bt_event_fun.c \
	apps/soundbox/task_manager/bt/bt_key_fun.c \
	apps/soundbox/task_manager/bt/bt_product_test.c \
	apps/soundbox/task_manager/bt/bt_switch_fun.c \
	apps/soundbox/task_manager/bt/bt_tws.c \
	apps/soundbox/task_manager/bt/vol_sync.c \
	apps/soundbox/task_manager/fm/fm.c \
	apps/soundbox/task_manager/fm/fm_api.c \
	apps/soundbox/task_manager/fm/fm_rw.c \
	apps/soundbox/task_manager/idle/idle.c \
	apps/soundbox/task_manager/linein/linein.c \
	apps/soundbox/task_manager/linein/linein_api.c \
	apps/soundbox/task_manager/linein/linein_dev.c \
	apps/soundbox/task_manager/music/music.c \
	apps/soundbox/task_manager/pc/pc.c \
	apps/soundbox/task_manager/power_off/power_off.c \
	apps/soundbox/task_manager/power_on/power_on.c \
	apps/soundbox/task_manager/record/record.c \
	apps/soundbox/task_manager/rtc/alarm_api.c \
	apps/soundbox/task_manager/rtc/alarm_user.c \
	apps/soundbox/task_manager/rtc/rtc.c \
	apps/soundbox/task_manager/rtc/virtual_rtc.c \
	apps/soundbox/task_manager/sleep/sleep.c \
	apps/soundbox/task_manager/spdif/hdmi_cec_drv.c \
	apps/soundbox/task_manager/spdif/spdif.c \
	apps/soundbox/task_manager/task_key.c \
	apps/soundbox/third_party_profile/ancs_client_demo/ancs_client_demo.c \
	apps/soundbox/third_party_profile/dma/dma_app_main.c \
	apps/soundbox/third_party_profile/gma/gma_app_main.c \
	apps/soundbox/third_party_profile/trans_data_demo/trans_data_demo.c \
	apps/soundbox/ui/color_led/color_led_app.c \
	apps/soundbox/ui/color_led/color_led_table.c \
	apps/soundbox/ui/color_led/driver/color_led.c \
	apps/soundbox/ui/color_led/driver/color_led_driver.c \
	apps/soundbox/ui/lcd/STYLE_02/bt_action.c \
	apps/soundbox/ui/lcd/STYLE_02/clock_action.c \
	apps/soundbox/ui/lcd/STYLE_02/file_brower.c \
	apps/soundbox/ui/lcd/STYLE_02/fm_action.c \
	apps/soundbox/ui/lcd/STYLE_02/linein_action.c \
	apps/soundbox/ui/lcd/STYLE_02/music_action.c \
	apps/soundbox/ui/lcd/STYLE_02/record_action.c \
	apps/soundbox/ui/lcd/STYLE_02/system_action.c \
	apps/soundbox/ui/lcd/lyrics_api.c \
	apps/soundbox/ui/lcd/ui_sys_param_api.c \
	apps/soundbox/ui/lcd_simple/my_demo.c \
	apps/soundbox/ui/led/pwm_led_api.c \
	apps/soundbox/ui/led/pwm_led_para_table.c \
	apps/soundbox/ui/led7/ui_bt.c \
	apps/soundbox/ui/led7/ui_common.c \
	apps/soundbox/ui/led7/ui_fm.c \
	apps/soundbox/ui/led7/ui_fm_emitter.c \
	apps/soundbox/ui/led7/ui_idle.c \
	apps/soundbox/ui/led7/ui_linein.c \
	apps/soundbox/ui/led7/ui_music.c \
	apps/soundbox/ui/led7/ui_pc.c \
	apps/soundbox/ui/led7/ui_record.c \
	apps/soundbox/ui/led7/ui_rtc.c \
	apps/soundbox/user_api/app_pwmled_api.c \
	apps/soundbox/user_api/app_record_api.c \
	apps/soundbox/user_api/app_special_play_api.c \
	apps/soundbox/user_api/app_status_api.c \
	apps/soundbox/user_api/dev_multiplex_api.c \
	apps/soundbox/user_api/product_info_api.c \
	apps/soundbox/version.c \
	cpu/br23/adc_api.c \
	cpu/br23/app_timer.c \
	cpu/br23/audio_common/app_audio.c \
	cpu/br23/audio_common/audio_fmtx.c \
	cpu/br23/audio_common/audio_iis.c \
	cpu/br23/audio_common/audio_link.c \
	cpu/br23/audio_dec/audio_dec.c \
	cpu/br23/audio_dec/audio_dec_bt.c \
	cpu/br23/audio_dec/audio_dec_file.c \
	cpu/br23/audio_dec/audio_dec_fm.c \
	cpu/br23/audio_dec/audio_dec_linein.c \
	cpu/br23/audio_dec/audio_dec_midi_ctrl.c \
	cpu/br23/audio_dec/audio_dec_midi_file.c \
	cpu/br23/audio_dec/audio_dec_pc.c \
	cpu/br23/audio_dec/audio_dec_record.c \
	cpu/br23/audio_dec/audio_dec_spdif.c \
	cpu/br23/audio_dec/audio_dec_tone.c \
	cpu/br23/audio_dec/audio_spectrum.c \
	cpu/br23/audio_dec/audio_sync.c \
	cpu/br23/audio_dec/audio_usb_mic.c \
	cpu/br23/audio_dec/tone_player.c \
	cpu/br23/audio_effect/eq_config.c \
	cpu/br23/audio_enc/audio_adc_demo.c \
	cpu/br23/audio_enc/audio_enc.c \
	cpu/br23/audio_enc/audio_enc_file.c \
	cpu/br23/audio_enc/audio_enc_recoder.c \
	cpu/br23/audio_enc/audio_mic_codec.c \
	cpu/br23/audio_enc/audio_recorder_mix.c \
	cpu/br23/audio_enc/audio_sbc_codec.c \
	cpu/br23/audio_mic/effect_cfg.c \
	cpu/br23/audio_mic/effect_debug.c \
	cpu/br23/audio_mic/effect_linein.c \
	cpu/br23/audio_mic/effect_reg.c \
	cpu/br23/audio_mic/effect_reg_echo.c \
	cpu/br23/audio_mic/effect_reg_reverb.c \
	cpu/br23/audio_mic/effect_stream.c \
	cpu/br23/audio_mic/effect_tool.c \
	cpu/br23/audio_mic/loud_speaker.c \
	cpu/br23/audio_mic/mic_effect.c \
	cpu/br23/audio_mic/mic_stream.c \
	cpu/br23/charge.c \
	cpu/br23/chargebox_hw.c \
	cpu/br23/chargestore.c \
	cpu/br23/clock_manager.c \
	cpu/br23/ctmu.c \
	cpu/br23/iic_eeprom_test.c \
	cpu/br23/iic_hw.c \
	cpu/br23/iic_slave_test.c \
	cpu/br23/iic_soft.c \
	cpu/br23/irflt.c \
	cpu/br23/localtws/localtws.c \
	cpu/br23/localtws/localtws_dec.c \
	cpu/br23/mcpwm.c \
	cpu/br23/overlay_code.c \
	cpu/br23/plcnt.c \
	cpu/br23/port_wkup.c \
	cpu/br23/pwm_led.c \
	cpu/br23/setup.c \
	cpu/br23/spi.c \
	cpu/br23/spi_test.c \
	cpu/br23/uart_bt_product.c \
	cpu/br23/uart_dev.c \
	cpu/br23/uart_test.c \
	cpu/br23/ui_driver/LED_1888/LED1888.c \
	cpu/br23/ui_driver/interface/ui_platform.c \
	cpu/br23/ui_driver/lcd_seg/lcd_seg3x9_driver.c \
	cpu/br23/ui_driver/lcd_spi/lcd_drive.c \
	cpu/br23/ui_driver/lcd_spi/spi_lcd_st7735s.c \
	cpu/br23/ui_driver/lcd_spi/spi_lcd_st7789v.c \
	cpu/br23/ui_driver/lcd_spi/spi_lcd_st7789vw.c \
	cpu/br23/ui_driver/lcd_spi/spi_oled.c \
	cpu/br23/ui_driver/led7/led7_driver.c \
	cpu/br23/ui_driver/ui_common.c \


# 需要编译的 .S 文件
S_SRC_FILES :=


# 需要编译的 .s 文件
s_SRC_FILES :=


# 需要编译的 .cpp 文件
cpp_SRC_FILES :=


# 链接参数
LFLAGS := \
	--plugin-opt=-pi32v2-always-use-itblock=false \
	--plugin-opt=-enable-ipra=true \
	--plugin-opt=-pi32v2-merge-max-offset=4096 \
	--plugin-opt=-pi32v2-enable-simd=true \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-global-merge-on-const \
	--plugin-opt=-inline-threshold=5 \
	--plugin-opt=-inline-max-allocated-size=32 \
	--plugin-opt=-inline-normal-into-special-section=true \
	--plugin-opt=-dont-used-symbol-list=malloc,free,sprintf,printf,puts,putchar \
	--plugin-opt=save-temps \
	--plugin-opt=-pi32v2-enable-rep-memop \
	--sort-common \
	--dont-complain-call-overflow \
	--plugin-opt=-used-symbol-file=cpu/br23/sdk_used_list.used \
	--gc-sections \
	--start-group \
	include_lib/liba/br23/cpu.a \
	include_lib/liba/br23/system.a \
	include_lib/liba/br23/aec.a \
	include_lib/liba/br23/media.a \
	include_lib/liba/br23/media_app.a \
	include_lib/liba/br23/libAptFilt_pi32v2_OnChip.a \
	include_lib/liba/br23/libEchoSuppress_pi32v2_OnChip.a \
	include_lib/liba/br23/libNoiseSuppress_pi32v2_OnChip.a \
	include_lib/liba/br23/libSplittingFilter_pi32v2_OnChip.a \
	include_lib/liba/br23/libDelayEstimate_pi32v2_OnChip.a \
	include_lib/liba/br23/libSingleMicSystem_pi32v2_OnChip.a \
	include_lib/liba/br23/libSMS_TDE_pi32v2_OnChip.a \
	include_lib/liba/br23/lib_esco_repair.a \
	include_lib/liba/br23/mp3_dec_lib.a \
	include_lib/liba/br23/mp3tsy_dec_lib.a \
	include_lib/liba/br23/wma_dec_lib.a \
	include_lib/liba/br23/wtg_dec_lib.a \
	include_lib/liba/br23/wtgv2_dec_lib.a \
	include_lib/liba/br23/wav_dec_lib.a \
	include_lib/liba/br23/flac_dec_lib.a \
	include_lib/liba/br23/ape_dec_lib.a \
	include_lib/liba/br23/m4a_tws_lib.a \
	include_lib/liba/br23/amr_dec_lib.a \
	include_lib/liba/br23/dts_dec_lib.a \
	include_lib/liba/br23/mp3_decstream_lib.a \
	include_lib/liba/br23/wma_decstream_lib.a \
	include_lib/liba/br23/sbc_eng_lib.a \
	include_lib/liba/br23/compressor.a \
	include_lib/liba/br23/crossover_coff.a \
	include_lib/liba/br23/limiter.a \
	include_lib/liba/br23/lib_mp2_code.a \
	include_lib/liba/br23/lib_adpcm_code.a \
	include_lib/liba/br23/mp3_enc_lib.a \
	include_lib/liba/br23/opus_enc_lib.a \
	include_lib/liba/br23/speex_enc_lib.a \
	include_lib/liba/br23/lib_pitchshifter.a \
	include_lib/liba/br23/lib_pitch_speed.a \
	include_lib/liba/br23/lib_reverb_cal.a \
	include_lib/liba/br23/lib_rmono2stereo.a \
	include_lib/liba/br23/bfilterfun_lib.a \
	include_lib/liba/br23/libFFT_pi32v2_OnChip.a \
	include_lib/liba/br23/libOpcore_maskrom_pi32v2_OnChip.a \
	include_lib/liba/br23/lib_resample_cal.a \
	include_lib/liba/br23/lib_resample_fast_cal.a \
	include_lib/liba/br23/crypto_toolbox_Ospeed.a \
	include_lib/liba/br23/lib_g726_decode.a \
	include_lib/liba/br23/lib_g726_code.a \
	include_lib/liba/br23/limiter_noiseGate.a \
	include_lib/liba/br23/howling.a \
	include_lib/liba/br23/noisegate.a \
	include_lib/liba/br23/shiftPhase.a \
	include_lib/liba/br23/pemafrow.a \
	include_lib/liba/br23/lib_howlings_phf.a \
	include_lib/liba/br23/math.a \
	include_lib/liba/br23/bt_hash_enc.a \
	include_lib/liba/br23/filtparam_cal_lib.a \
	include_lib/liba/br23/agreement.a \
	include_lib/liba/br23/lib_sur_cal.a \
	include_lib/liba/br23/lib_vbass_cal.a \
	include_lib/liba/br23/loudness.a \
	include_lib/liba/br23/rcsp_stack.a \
	include_lib/liba/br23/lib_usbaudio_plc.a \
	include_lib/liba/br23/coeff_calculate.a \
	include_lib/liba/br23/rms.a \
	include_lib/liba/br23/SpectrumShow.a \
	apps/common/third_party_profile/tuya_protocol/sdk/lib/libtuya_lib.a \
	include_lib/liba/br23/lib_midi_dec.a \
	include_lib/liba/br23/btstack.a \
	include_lib/liba/br23/btctrler.a \
	--end-group \
	include_lib/liba/br23/update.a \
	include_lib/liba/br23/res.a \
	include_lib/liba/br23/ui_dot.a \
	include_lib/liba/br23/ui_s.a \
	include_lib/liba/br23/font.a \
	-Tcpu/br23/sdk.ld \
	-M=cpu/br23/tools/sdk.map \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-mattr=+fprev1 \


LIBPATHS := \
	-L$(SYS_LIB_DIR) \


LIBS := \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libc.a \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libcompiler-rt.a \



c_OBJS    := $(c_SRC_FILES:%.c=%.c.o)
S_OBJS    := $(S_SRC_FILES:%.S=%.S.o)
s_OBJS    := $(s_SRC_FILES:%.s=%.s.o)
cpp_OBJS  := $(cpp_SRC_FILES:%.cpp=%.cpp.o)

OBJS      := $(c_OBJS) $(S_OBJS) $(s_OBJS) $(cpp_OBJS)
DEP_FILES := $(OBJS:%.o=%.d)


OBJS      := $(addprefix $(BUILD_DIR)/, $(OBJS))
DEP_FILES := $(addprefix $(BUILD_DIR)/, $(DEP_FILES))


VERBOSE ?= 0
ifeq ($(VERBOSE), 1)
QUITE :=
else
QUITE := @
endif

.PHONY: all clean pre_build

all: pre_build $(OUT_ELF)
	@echo +POST-BUILD
	$(QUITE) $(RUN_POST_SCRIPT) sdk

pre_build:
	@echo +PRE-BUILD
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P cpu/br23/sdk_used_list.c -o cpu/br23/sdk_used_list.used
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P cpu/br23/sdk_ld.c -o cpu/br23/sdk.ld
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P cpu/br23/tools/download.c -o $(POST_SCRIPT)

clean:
	$(QUITE) $(RM) $(OUT_ELF)
	$(QUITE) $(RM) $(OBJS)
	$(QUITE) $(RM) $(DEP_FILES)

$(OUT_ELF): $(OBJS)
	@echo +LINK $@
	$(file >$(OBJ_FILE), $(OBJS))
	$(QUITE) $(LD) -o $(OUT_ELF) @$(OBJ_FILE) $(LFLAGS) $(LIBPATHS) $(LIBS)

$(BUILD_DIR)/%.c.o : %.c
	@echo +CC $<
	@$(MKDIR) $(@D)
	@$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT "$@" $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.S.o : %.S
	@echo +AS $<
	@$(MKDIR) $(@D)
	@$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT "$@" $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.s.o : %.s
	@echo +AS $<
	@$(MKDIR) $(@D)
	@$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT "$@" $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cpp.o : %.cpp
	@echo +CXX $<
	@$(MKDIR) $(@D)
	@$(CXX) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT "$@" $< -o $(@:.o=.d)
	$(QUITE) $(CXX) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

-include $(DEP_FILES)
