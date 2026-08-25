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
 * @file rolling_config_helper.h
 * @author qifeng.liu@rollingwireless.com (liuqifeng)
 * @brief rolling config helper header file
 * @version 1.0
 * @date 2025-11-23
 *
 *
 **/

#ifndef __ROLLING_CONFIG_HELPER_H__
#define __ROLLING_CONFIG_HELPER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "rolling_helper_cid.h"

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

typedef enum {
    GET_DATA_SUCCESS=0,
    GET_DATA_FAIL,
    SERVICE_BUSY,
    UNKNOW_CODE,
}status_code;

typedef struct Header
{
    e_service service_id;
    e_command_cid command_cid;

}Header;

typedef struct Mesg
{
    Header header;
    status_code rtcode;
	int payload_lenth;
    char payload[0];
}mesg_info;



void rolling_dus_init(void);
gboolean cfg_get_port_state(void);
void set_static_config_flg(bool value);
bool get_static_config_flg(void);
bool rolling_dbus_init_status(void);
bool rolling_dbus_sem_init(void);
void rolling_dbus_sem_destroy(void);
void rolling_dbus_sem_wait(void);
void rolling_dbus_sem_post(void);
bool register_dbus_event_handler(void);
bool dbus_service_is_ready(void);
bool send_message_get_response(e_command_cid cid, char *payload, int len, mesg_info **response);
bool send_message_test(void);
void rolling_get_network_mcc(char *network_mcc);
#endif