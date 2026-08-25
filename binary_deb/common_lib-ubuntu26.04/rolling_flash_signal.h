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
 * @file rolling_flash_signal.h   
 * @author qifeng.liu@rollingwireless.com (liuqifeng)
 * @brief rolling flash signal header file
 * @version 1.0
 * @date 2025-11-23
 *
 *
 **/
#ifndef __ROLLING_FLASH_SIGNAL_H__
#define __ROLLING_FLASH_SIGNAL_H__

#include "rolling_flash_common.h"

#define BUF_LEN (1024 * (sizeof(struct inotify_event) + NAME_MAX + 1))
pthread_mutex_t mutex;
timer_t timer;
struct itimerspec ts;
struct sigevent evp;
struct itimerspec newts;

int reboot_count = 0;
static int flash_flag = 0;


#define FILE_MONITOR_PATH "/etc/opt/rolling/rolling_fw_pkg/"


gboolean regester_interesting_siganl();
static gboolean flash_status_handler(RollingGdbusHelper *object, const char *value, gpointer userdata);
void sighandle(int signum);
static gboolean modem_status_callback(RollingGdbusHelper *object, const char *value, gpointer userdata);
gboolean get_port_state(e_port_state *state);

int check_port_state(char *state);
void dumpport_process();
gboolean init_flash_timer();
void fastbootport_process();

void noport_process();
gboolean start_flash_timer(int time);
void flashport_process();

void normalport_process();
gboolean stop_fastboot_timer();
gboolean stop_flash_timer();

#endif