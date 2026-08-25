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
 * @file rolling_helper_basic_func.h
 * @author rick.chen@rollingwireless.com (chenhaotian)
 * @brief rolling helper basic function header file
 * @version 1.0
 * @date 2025-11-17
 *
 *
 **/

#ifndef _ROLLING_HELPER_BASIC_FUNC_H_
#define _ROLLING_HELPER_BASIC_FUNC_H_

#include <glib.h>
#include <gio/gio.h>
#include <libudev.h>
#include "libmbim-glib.h"
#include "rolling_helper_common.h"
#include "common_base_func.h"

void     rolling_helperd_control_message_receiver(void);
gint     rolling_helperm_send_resp_to_helperd(gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str);
void     rolling_mutex_keep_pointer_exist_unlock(void);
void     rolling_mutex_keep_pointer_exist_lock(void);
void     rolling_mutex_modem_info_unlock(void);
void     rolling_mutex_modem_info_lock(void);
void     rolling_mutex_force_sync_unlock(void);
void     rolling_mutex_force_sync_lock(void);
gint     rolling_mutex_init(void);
void     request_receiver(void);
gint     rolling_get_supported_module_number(void);
gint     rolling_get_supported_module_info(void *module_info, gint index);
void     rolling_register_module_event_signal(void);
void     rolling_helperd_device_state_init(void);
gint     rolling_helper_mmevent_register(void);
void     rolling_helperm_main_receiver(void);
void     rolling_helperm_control_receiver(void);
gint     rolling_register_helper_service(void);
gint     rolling_set_linux_app_signals(void);
void     rolling_udev_deinit(void);
void     rolling_mbim_port_deinit(void);
gint     rolling_get_helper_seq_id(gint seq);
gint     rolling_helper_queue_init(void);
gint     rolling_deinit_message_queue(void);

gint     rolling_parse_sw_reboot(gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str, gpointer callback, gchar *req_cmd);
void     rolling_helperm_get_network_mccmnc_ready (MbimDevice *device, GAsyncResult *res, gpointer userdata);
void     rolling_helperm_get_local_mccmnc_ready (MbimDevice *device, GAsyncResult *res, gpointer userdata);
void     rolling_helperm_get_work_slot_id_ready (MbimDevice *device, GAsyncResult *res, gpointer userdata);
gint     rolling_parse_mbim_request(gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str, gpointer callback, char *req_cmd);

void     rolling_resp_error_result_callback(MbimDevice *device, GAsyncResult *res, gpointer serviceid);
gint     rolling_resp_error_result(gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str, gpointer callback, gchar * req_cmd);

gint     rolling_parse_get_fw_info(gint serviceid, gint cid, gint rtcode, gint payloadlen, gchar *payload_str, gpointer callback, gchar *req_cmd);
gint     rolling_parse_get_fcc_status_ready(MbimDevice *device, GAsyncResult *res, gpointer user_data);
gint     rolling_get_port_command_ready (gchar   *resp_str);
gint     rolling_get_subsysid_ready(MbimDevice *device, GAsyncResult *res, gpointer user_data);
gint     rolling_edl_flash_ready(MbimDevice *device, GAsyncResult *res, gpointer userdata);
gpointer rolling_qdl_flash_command(gpointer payload, int *qdl_success_flag);
gpointer edl_flashing_command(void *data);

gpointer rolling_fastboot_flash_command(gpointer payload, int *fastboot_success_flag);
gpointer fastboot_flashing_command(void *data);
gint     rolling_fastboot_flash_ready(MbimDevice *device, GAsyncResult *res, gpointer userdata);
int rolling_hwreset_gpio_init(void);

#endif /* _ROLLING_HELPER_BASIC_FUNC_H_ */

