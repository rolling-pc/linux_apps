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
 * @file rolling_flash_main.h
 * @author qifeng.liu@rollingwireless.com (liuqifeng)
 * @brief rolling flash main header file
 * @version 1.0
 * @date 2025-11-23
 *
 *
 **/

#ifndef __ROLLING_FLASH_MAIN_H__
#define __ROLLING_FLASH_MAIN_H__


#include "version.h"
#include "rolling_flash_common.h"


int g_debug_level = LOG_INFO;
int noport_timer_source = -1;

typedef struct Header
{
    e_service_id service_id;
    e_command_cid command_cid;

} Header;

typedef struct Mesg
{
    Header header;
    int payload_lenth;
    char payload[0];
} Mesg;

typedef struct oem_list
{
    char *oem;
} oem_list;

typedef struct vid_pid_list
{
    char *id;

} vid_pid_list;

typedef struct recovery_list
{
    oem_list oem;
    vid_pid_list id;
    vid_pid_list subsysid;
} recovery_list;

int check_port_state(char *state);
void open_log(int argc, char *argv[]);
void rolling_firmware_recovery_run();
void rolling_monitor_package_run();
void rolling_check_imei_run();

#endif