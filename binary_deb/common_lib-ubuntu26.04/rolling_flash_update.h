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
 * @file rolling_flash_update.h   
 * @author qifeng.liu@rollingwireless.com (liuqifeng)
 * @brief rolling flash update header file
 * @version 1.0
 * @date 2025-11-23
 *
 *
 **/
#ifndef __ROLLING_FLASH_UPDATE_H__
#define __ROLLING_FLASH_UPDATE_H__

#include "rolling_flash_common.h"

char g_strType[256] = {0};
#define DEV_MCCMNC_LEN   (32)
#define LOW_POWER_HINT    "Low Battery, Postpone Flashing."

e_flow_state flash_flow_state = FW_UPDATE_FLOW_UNLOCK;

bool compare_version_need_update(mdmver_details *curmdm_ver, fw_details *fw_ver, e_update_option update_option);
void do_flash_fw(char *g_strType);
bool compare_version_dev_need_update(mdmver_details *curmdm_ver, fw_details *fw_ver);

#endif