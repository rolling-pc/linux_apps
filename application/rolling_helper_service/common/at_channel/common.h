/**
 * Copyright (C) 2025 Rolling Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * @file common.h
 * @author qifeng.liu@rollingwireless.com (liuqifeng)
 * @brief common header file
 * @version 1.0
 * @date 2025-11-23
 *
 *
 **/

#include "serial_port.h"

#ifdef __cplusplus
extern "C"
{
#endif
int rolling_adapter_send_at_command(const char *req_cmd, char *rspbuf, const char *mbimportname);
#ifdef __cplusplus
}
#endif
