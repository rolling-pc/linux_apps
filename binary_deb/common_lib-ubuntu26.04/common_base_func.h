/**
 * Copyright (C) 2025 Rolling Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * @file common_base_func.h
 * @author rick.chen@rollingwireless.com (chenhaotian)
 * @brief common base function header file
 * @version 1.0
 * @date 2025-11-17
 *
 *
 **/

#ifndef _COMMON_BASE_FUNC_H_
#define _COMMON_BASE_FUNC_H_

#include <glib.h>
#include <gio/gio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include "rolling_helper_cid.h"
#include "rolling_log.h"
#include "libmbim-glib.h"

#define MBIM_FUNCTION_SUPPORTED

#define ROLLING_MODULE_NAME_LEN          16  // max size like "RW101R-GL-00-30"
#define ROLLING_MODULE_USBID_LEN         10  // max size like "2cb7:01a2"
#define ROLLING_MODULE_PCIEID_LEN        20  // max size like "aaaa.bbbb.cccc.dddd"
#define ROLLING_MODULE_PCIESUBSYSID_LEN  10  // max size like "0d40:4D75"
#define ROLLING_MODULE_PCIESSVID_LEN     5   // max size like "0d40"
#define ROLLING_MODULE_PCIESSDID_LEN     5   // max size like "4D75"
#define ROLLING_MODULE_MBIMPORT_LEN      17  // max size like "/dev/wwan0mbimxx"
#define ROLLING_MODULE_DLPORT_LEN        17  // max size like "/dev/wwan0mbimxx"
#define ROLLING_MODULE_ATPORT_LEN        15  // max size like "/dev/wwan0atxx"
#define ROLLING_FW_IDENTIFIER_LEN        6   // max size like "19500"

#ifndef AT_COMMAND_LEN
#define AT_COMMAND_LEN                   256
#endif

#define MSG_PAYLOAD_SIZE        (2000)
#define MSG_BUFFER_SIZE         (2048)
#define MSG_MTEXT_MAX_LEN       (MSG_BUFFER_SIZE - sizeof(long))

// Global thread handle: store device check sub-thread reference for resource cleanup
extern GThread *g_dev_check_sub_thread;
// Global atomic exit flag: graceful sub-thread exit switch, ensures thread-safe access
extern gint g_dev_check_sub_exit;

#define MBIM_FUNCTION_SUPPORTED

#define RDONLY                           "r"
#define WRONLY                           "w"

#define DISABLE_SUSPEND "systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target"
#define ENABLE_SUSPEND  "systemctl unmask sleep.target suspend.target hibernate.target hybrid-sleep.target"

typedef enum{
    SUSPEND_DISABLE,
    SUSPEND_ENABLE
}SUSPEND_STATUS;

typedef enum
{
    RET_ERROR        = -1,
    RET_ERR_INTERNAL = RET_ERROR,
    RET_MIN          = RET_ERROR,
    RET_OK           = 0,
    RET_ERR_PROCESS,
    RET_ERR_BUSY,
    RET_ERR_RESOURCE,
    RET_MAX
}rolling_ret_enum_type;

typedef enum
{
    PLATFORM_QUALCOMM,
    PLATFORM_MEDIATEK,
}e_chip_platform;

typedef enum{
    CELLULAR_TYPE_MIN     = 0,
    CELLULAR_TYPE_UNKNOWN = CELLULAR_TYPE_MIN,
    CELLULAR_TYPE_USB     = 1,
    CELLULAR_TYPE_PCIE    = 2,
    CELLULAR_TYPE_MAX
}e_cellular_type;

 typedef enum
 {
     ENUM_MIN      = 0,
     HELPER        = ENUM_MIN, /* Helper     service */
     FWSWITCHSRV   = 1,        /* FWswitch   service */
     FWRECOVSRV    = 2,        /* FWrecovery service */
     MASRV         = 3,        /* MA         service */
     CONFIGSRV     = 4,        /* Config     service */
     ENUM_MAX                  /* unknown    service */
 }e_service;

typedef struct
{
    gint length;
    gchar *buffer;
}rolling_buffer_type;

typedef struct
{
    gint        serviceid;
    gint        cid;
    gint        rtcode;
    gint        payloadlen;
    void        *data;
    gchar       payload_str[0];
}rolling_async_struct_type;

#define ICCID_LENGTH 20 // ICCID length is 20 characters
#define MAX_PROFILE_NUM 20
typedef enum {
    TEST_PROFILE = 0,
    NORMAL,
} PROFILE_TYPE;

typedef struct 
{
    char profile_id[ICCID_LENGTH + 1]; // ICCID length is 20, plus null terminator
    bool status; // true:active, false:inactive
    PROFILE_TYPE type; // TEST_PROFILE or NORMAL
    int retry_times;
}profile_status;

typedef struct profile_stack_
{
    profile_status profiles[MAX_PROFILE_NUM]; // Array to hold up to 20 profiles
    int profile_top;
    bool (*func)(rolling_async_struct_type *user_data, struct profile_stack_ *profile);
    int ret;
    int channel_id;
}profile_obj;


 /* AT command handler table */
typedef struct
{
    e_command_cid       cid;
    gint                (*func_pointer)(gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str, gpointer callback, gchar *req_cmd);
    gpointer            callback;
    gchar               *at_cmd;
}rolling_message_handler_list_type;

/* cellular info struct */
typedef struct
{
    char                chipset_platform;
    char                module_name[ROLLING_MODULE_NAME_LEN];
    char                fw_identifier[ROLLING_FW_IDENTIFIER_LEN];
    char                module_type;
    char                mbimport_name[ROLLING_MODULE_MBIMPORT_LEN];
    char                dlport_name[ROLLING_MODULE_DLPORT_LEN];
    char                recoveryport_name[ROLLING_MODULE_DLPORT_LEN];
    char                atport_name[ROLLING_MODULE_ATPORT_LEN];
}project_cellular_info_type;

typedef struct
{
    rolling_message_handler_list_type  *msg_handler_list;
    gint                            msg_handler_num;
    gint                            (*send_message)(void *message, guint32 len, guint32 timeout, GAsyncReadyCallback callback, gpointer user_data);  // this func should support pure AT / AT_over_MBIM func, sync and async func.
    gint                            (*get_response_data)(void *device, GAsyncResult *res, gchar **resp_str, guint32 ret_size);          // this func should support pure AT response / AT_over_MBIM rspcb func, sync and async func.
    gint                            (*default_error_func)(gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str, gpointer callback, char *req_cmd);
    gint                            (*default_error_func_cb)(void *device, GAsyncResult *res, gpointer userdata);
}message_func_handler_table;

typedef struct
{
    // because currently every project use their own method to compile and download, these hooks are not used yet.
    gint (*get_all_images_pathname)(void *dummy);
    gint (*burn)(void *dummy);  // this func should achieve fw burn, check and zenity update.
    gint (*result_check)(void *dummy);
    // below funcs will be used when module enter or exit dl mode, module should add dl flag after it enter dl mode, and remove dl flag before it exit.
    // further: modify name like do extra thing before and after dl.
    gint (*optional_add_dl_flag)(gboolean dummy);
    gint (*optional_remove_dl_flag)(void);
}dl_func_handler_table_type;

typedef struct
{
    gint (*get_all_images_pathname)(void *dummy);
    gint (*burn)(void *dummy);  // this func should achieve fw burn, check and zenity update.
    gint (*result_check)(void *dummy);
    // below funcs will be used when module enter or exit recovery mode, module should add dl flag, stop monitor after it enter recovery mode, remove dl flag before it exit, and run monitor after it exit.
    gint (*optional_recovery_monitor_run)(void *dummy);
    gint (*optional_recovery_monitor_stop)(void *dummy);
    gint (*optional_add_dl_flag)(gboolean dummy);
    gint (*optional_remove_dl_flag)(void);
    gint (*optional_add_ready_flag)(gboolean dummy);
    gint (*optional_remove_ready_flag)(void);
}recovery_func_handler_table_type;

typedef struct
{
    gint (*switch_dl_port)(void *dummy);
    gint (*switch_recovery_port)(void *dummy);
    gint (*get_module_state)(gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str, gpointer callback, char *req_cmd);
    gint (*check_cellular_exist)(gint *check_result, gint expect_result);
    gint (*check_atport_exist)(gchar *atport);
    gint (*check_mbimport_exist)(gchar *mbimport);
    gint (*check_recoveryport_exist)(void *dummy);
    gint (*check_dlport_exist)(void *dummy);
    gint (*init_mbim_port)(void);
    gint (*try_set_mbim_port_ready)(void);
    gint (*deinit_mbim_port)(void);
    gint (*re_modprobe)(void);
    gint (*dummy1)(void *dummy);
}port_func_handler_table_type;

typedef struct
{
    gchar subsysid[ROLLING_MODULE_PCIESUBSYSID_LEN];
}pcie_ssvid_sspid_support_list_type;

typedef struct
{
    gchar vidpid[ROLLING_MODULE_USBID_LEN];
}usb_vid_pid_support_list_type;

typedef struct
{
    message_func_handler_table          msg_handler_table;
    dl_func_handler_table_type          dl_handler_table;
    recovery_func_handler_table_type    recovery_handler_table;
    port_func_handler_table_type        port_handler_table;
    gboolean                            pcie_valid;
    pcie_ssvid_sspid_support_list_type  *ssvid_ssdid_support_list;
    gchar                               ssvid_ssdid_support_list_num;
    gboolean                            usb_valid;
    usb_vid_pid_support_list_type       *vid_pid_support_list;
    gchar                               vid_pid_support_list_num;
}project_function_table_type;

/*progress*/
//枚举元素的命名不能随意更改，需要和DISTRIB_ID完全一致
enum CurrentDistibId {
    None = 0,
    Ubuntu,
    Thinpro,
    Fedora,
};
typedef struct Progress Progress;
// 父类结构体
struct Progress {
    int progressWidth; 
    int progressHeight;
    FILE * progressFd;
    FILE * progressCloseFd;

    char progressType[32];
    char progressSchedule[32];
    char progressId[32];
    char progressTitle[64];
    char progressText[256];
    char environmentVariable[256];
    char progressCmd[1024];
    char progressCloseCmd[512];

    // 父类方法指针
    // 获取环境变量
    int (*rolling_get_progress_environment_variable)(Progress *self);
    // 执行进度条命令
    int (*rolling_start_progress)(Progress *self);
    // 更新进度条标题
    int (*rolling_set_progress_title)(Progress *self, const char* title);
    // 初始进度条内容
    int (*rolling_set_progress_init_text)(Progress *self);
    // 更新进度条内容
    int (*rolling_set_progress_text)(Progress *self, const char* text);
    // 更新进度条进度
    int (*rolling_set_progress_schedule)(Progress *self, int schedule);
    // 刷新进度条
    int (*rolling_refresh_progress)(Progress *progress, const char *text, int schedule);
    // 关闭当前进度条
    int (*rolling_close_progress)(Progress *self);
};

/*end of progress*/

extern project_function_table_type *g_project_func_table;
extern project_cellular_info_type  g_cellular_info;

/*------------above are class definitions.------------*/
int rolling_check_kernel_version(void);
gint rolling_helperm_set_warning_bar(gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str, gpointer callback, char * req_cmd);
gint rolling_parse_send_set_atcmd(gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str, gpointer callback, gchar *req_cmd);
int  rolling_parse_send_atcmd_ready(void *device, GAsyncResult *res, gpointer userdata);
int  rolling_parse_send_req_atcmd_with_userdata(rolling_async_struct_type *user_data, gpointer callback, char *req_cmd);
int  rolling_parse_send_req_atcmd(gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str, gpointer callback, char *req_cmd);
int  rolling_helperm_resp_error_result(gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str, gpointer callback, char *req_cmd);
gint rolling_helperm_resp_error_result_callback(void *device, GAsyncResult *res, gpointer userdata);

gint rolling_helperm_get_network_mccmnc(GAsyncReadyCallback func_pointer, gpointer userdata);
gint rolling_helperm_get_local_mccmnc(GAsyncReadyCallback func_pointer, gpointer userdata);
gint rolling_helperm_get_subscriber_ready_status(GAsyncReadyCallback func_pointer, gpointer userdata);

void rolling_helperm_control_get_local_mccmnc_ready(void *device, GAsyncResult *res, gpointer userdata);
void rolling_helperm_control_get_network_mccmnc_ready(void *device, GAsyncResult *res, gpointer userdata);
void rolling_helperm_control_get_subscriber_ready_status_ready(void *device, GAsyncResult *res, gpointer userdata);

gint rolling_get_cellular_info(project_cellular_info_type *cellular_pointer);
gint rolling_function_table_init(project_function_table_type **pointer);
gint rolling_function_table_deinit(project_function_table_type **pointer);
gint rolling_platform_update_function_table(project_function_table_type **pointer);
gint rolling_project_update_function_table(project_function_table_type **pointer);
gint rolling_oem_update_function_table(project_function_table_type **pointer);

gint rolling_delete_test_profile(gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str, gpointer callback, char *req_cmd);
char* itoa(int num,char* str,int radix);
int rolling_get_current_distrib_id(enum CurrentDistibId *hostType);
Progress *CreateProgressImpl(enum CurrentDistibId hostType);
void DestroyProgressImpl(Progress *self);
int set_suspend_to_os(SUSPEND_STATUS flag);
gint rolling_re_modprobe(void);

#endif /* _COMMON_BASE_FUNC_H_ */
