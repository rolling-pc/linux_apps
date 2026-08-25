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
 * @file rolling_config_parse.h
 * @author qifeng.liu@rollingwireless.com (liuqifeng)
 * @brief rolling config parse header file
 * @version 1.0
 * @date 2025-11-23
 *
 *
 **/

#ifndef __ROLLING_CONFIG_PARSE_H__
#define __ROLLING_CONFIG_PARSE_H__
#include <stdio.h>
#include "rolling_list.h"

typedef struct config_parse_s{
    struct list_head list;
    char key[32];
    int keyval;

}config_parse_t;

int rolling_config_parse(char *ini_path, struct list_head *list_data);


#endif