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
 * @file rolling_helper_adapter.h
 * @author rick.chen@rollingwireless.com (chenhaotian)
 * @brief rolling helper adapter header file
 * @version 1.0
 * @date 2025-11-17
 *
 *
 **/


#ifndef _ROLLING_HELPER_ADAPTER_H_
#define _ROLLING_HELPER_ADAPTER_H_

#include <glib.h>
#include <gio/gio.h>
#include <libudev.h>
#include "libmbim-glib.h"
#include "rolling_helper_common.h"

#ifdef MBIM_FUNCTION_SUPPORTED
#include "mbim-fibocom.h"
#endif

typedef enum{
    CELLULAR_STATE_MIN      = 0,
    CELLULAR_STATE_UNKNOWN  = CELLULAR_STATE_MIN,
    CELLULAR_STATE_MISSING  = 1,
    CELLULAR_STATE_EXISTED  = 2,
    CELLULAR_STATE_MAX
}cellular_state_enum_type;

typedef enum
{
    SEQ_INPUT      =  0,
    SEQ_OUTPUT     =  1,
    HELPERM_INPUT  = SEQ_INPUT,
    HELPERM_OUTPUT = SEQ_OUTPUT,
    HELPERD_INPUT  = SEQ_OUTPUT,
    HELPERD_OUTPUT = SEQ_INPUT
}queue_enum_type;

typedef enum
{
    MSG_ALL     =  0,
    MSG_NORMAL  =  1,
    MSG_CONTROL =  2
}seq_message_enum_type;

typedef enum
{
    OP_MIN     =  0,
    OP_READ    =  OP_MIN,
    OP_WRITE   =  1,
    OP_MAX     =  OP_WRITE
}at_operate_enum_type;

typedef struct
{
    cellular_state_enum_type cellular_state;
    e_cellular_type          cellular_type;
    gchar                    work_module_name[ROLLING_MODULE_NAME_LEN];
    gint                     module_info_index;
} rolling_cellular_type;

typedef struct
{
    gchar module_name[ROLLING_MODULE_NAME_LEN];
    gchar module_type;
    gchar usbsubsysid[ROLLING_MODULE_USBID_LEN];
    gchar pciessvid[ROLLING_MODULE_PCIESSVID_LEN];
    gchar pciessdid[ROLLING_MODULE_PCIESSDID_LEN];
    gchar mbimportname[ROLLING_MODULE_MBIMPORT_LEN];
    gchar dlportname[ROLLING_MODULE_DLPORT_LEN];
    gchar atportname[ROLLING_MODULE_ATPORT_LEN];
} Rolling_module_info_type;

typedef struct
{
    gint                   serviceid;
    gint                   cid;
    RollingGdbusHelper     *skeleton;
    GDBusMethodInvocation  *invocation;
}async_user_data_type;

typedef struct
{
    long   mtype;     // should be seq_message_type.
    char   mtext[0];  // should be rolling_async_struct_type.
}helper_message_struct;

void     rolling_adapter_trigger_app_exit(void);
gint     rolling_adapter_alloc_and_send_resp_structure(gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str);
void     rolling_adapter_mutex_sim_insert_flag_operate_lock(void);
void     rolling_adapter_mutex_sim_insert_flag_operate_unlock(void);
void     rolling_adapter_mutex_mbim_flag_operate_unlock(void);
void     rolling_adapter_mutex_mbim_flag_operate_lock(void);
void     rolling_adapter_mutex_keep_pointer_exist_unlock(void);
void     rolling_adapter_mutex_keep_pointer_exist_lock(void);
void     rolling_adapter_mutex_cellular_info_operate_unlock(void);
void     rolling_adapter_mutex_cellular_info_operate_lock(void);
void     rolling_adapter_mutex_force_sync_unlock(void);
void     rolling_adapter_mutex_force_sync_lock(void);
void     rolling_adapter_mutex_unlock_cold_rest();
void     rolling_adapter_mutex_lock_cold_rest();
gint     rolling_adapter_all_mutex_init(void);
gint     rolling_adapter_helperm_send_control_message_to_helperd(int cid, int payloadlen, char *payload_str);
gint     rolling_adapter_helperd_send_control_message_to_helperm(int cid, int payloadlen, char *payload_str);
void     rolling_adapter_helperd_send_resp_to_dbus(RollingGdbusHelper *skeleton, GDBusMethodInvocation *invocation, gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str);
gint     rolling_adapter_get_supported_module_number(void);
gint     rolling_adapter_get_supported_module_info(Rolling_module_info_type *module_info, gint index);
gint     rolling_adapter_get_work_cellular_info(rolling_cellular_type *work_cellular_info);
gint     rolling_adapter_set_work_cellular_info(rolling_cellular_type *work_cellular_info);
gint     rolling_adapter_set_linux_app_signals(void);
gint     rolling_adapter_send_message_async(void *message, guint32 len, guint32 timeout, GAsyncReadyCallback callback, gpointer user_data);
void     rolling_adapter_udev_deinit(void);
gint     rolling_adapter_udev_init(gint *output_fd);
gint     rolling_adapter_check_cellular(gint *check_result);
void     rolling_adapter_mbim_port_deinit(void);
void     rolling_adapter_mbim_port_init(char *mbimportname);
void     rolling_adapter_control_mbim_init(void);
void     rolling_helperd_device_Check(gpointer user_data);

gint     rolling_adapter_helperm_get_normal_msg_from_helperd(void *msgs);
gint     rolling_adapter_helperm_get_control_msg_from_helperd(void *msgs, gint buf_len);
gint     rolling_adapter_helperm_send_msg_to_helperd(void *msgs, int msgsize);
gint     rolling_adapter_helperd_get_control_msg_from_helperm(void *msgs);
gint     rolling_adapter_helperd_get_normal_msg_from_helperm(void *msgs);
gint     rolling_adapter_helperd_send_req_to_helperm(void *msgs, int msgsize);
gint     rolling_adapter_get_helper_seq_id(int seq);
gint     rolling_adapter_helper_queue_init(void);
gint     rolling_adapter_helperd_timer_handle(void);
gint     rolling_adapter_helperd_timer_close(void);
void     rolling_adapter_helperm_control_get_local_mccmnc_ready (MbimDevice *device, GAsyncResult *res, gpointer userdata);
gint     rolling_adapter_helperm_get_local_mccmnc(GAsyncReadyCallback func_pointer, gpointer userdata);
void     rolling_adapter_helperm_control_get_network_mccmnc_ready (MbimDevice *device, GAsyncResult *res, gpointer userdata);
gint     rolling_adapter_helperm_get_network_mccmnc(GAsyncReadyCallback func_pointer, gpointer userdata);
void     rolling_adapter_helperm_deinit_get_subscriber_ready_status_ready (MbimDevice *device, GAsyncResult *res, gpointer userdata);
void     rolling_adapter_helperm_control_get_subscriber_ready_status_ready (MbimDevice *device, GAsyncResult *res, gpointer userdata);
gint     rolling_adapter_helperm_get_subscriber_ready_status(GAsyncReadyCallback func_pointer, gpointer userdata);
gint     rolling_adapter_helperm_get_work_slot_info(GAsyncReadyCallback func_pointer, gpointer userdata);
gint     rolling_adapter_helperm_switch_work_slot(GAsyncReadyCallback func_pointer, gpointer userdata);
int      rolling_adapter_send_at_command(const char *req_cmd, char *rspbuf, const char *mbimportname);

#endif /* _ROLLING_HELPER_ADAPTER_H_ */
