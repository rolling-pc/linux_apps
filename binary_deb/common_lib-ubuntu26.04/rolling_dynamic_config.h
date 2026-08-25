/**
 * Copyright (C) 2025 Rolling Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * @file rolling_dynamic_config.h
 * @author qifeng.liu@rollingwireless.com (liuqifeng)
 * @brief rolling dynamic config header file
 * @version 1.0
 * @date 2025-11-23
 *
 *
 **/



#ifndef __ROLLING_DYNAMIC_CONFIG_H__
#define __ROLLING_DYNAMIC_CONFIG_H__
#include <sys/ipc.h>
#include <sys/msg.h>

void* dynamic_thread(void* arg);

void* event_from_file_thread(void* arg);
void* event_from_signal_thread(void* arg);
void send_event_by_mcc_change(char *roam_mcc_new);
char *rolling_get_mcc_value(void);
bool msg_init(void);
int get_msg_id(void);
void dynamic_deinit(void);
bool get_ta_sar_enable(void);
bool get_body_sar_enable(void);
#endif