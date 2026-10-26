/**
 * Copyright (C) 2025 Rolling Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * @file rolling_private_log.h
 * @author qifeng.liu@rollingwireless.com (liuqifeng)
 * @brief rolling private log header file
 * @version 1.0
 * @date 2025-11-23
 *
 *
 **/

#ifndef _ROLLING_PRIVATE_LOG_H_
#define _ROLLING_PRIVATE_LOG_H_

#include <syslog.h>
#include <glib.h>
#include "stdio.h"

#define ROLLING_LOG_OPEN(module) openlog(module, LOG_CONS | LOG_PID, LOG_USER);

// #define ROLLING_LOG_CRITICAL(log, ...) syslog(LOG_CRIT, "[Critical]: %s:%u: "log, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define ROLLING_LOG_CRITICAL(log, ...) g_print("[Critical]: %s:%u: "log, __FUNCTION__, __LINE__, ##__VA_ARGS__);

#define ROLLING_LOG_ERROR(log, ...) syslog(LOG_ERR, "[Error]: %s:%u: "log, __FUNCTION__, __LINE__, ##__VA_ARGS__);

#define ROLLING_LOG_NOTICE(log, ...) syslog(LOG_NOTICE, "[Notice]: %s:%u: "log, __FUNCTION__, __LINE__, ##__VA_ARGS__);

#define ROLLING_LOG_WARNING(log, ...) syslog(LOG_WARNING, "[Warning]: %s:%u: "log, __FUNCTION__, __LINE__, ##__VA_ARGS__);

#define ROLLING_LOG_INFO(log, ...) syslog(LOG_INFO, "[Info]: %s:%u: "log, __FUNCTION__, __LINE__, ##__VA_ARGS__);

// #define ROLLING_LOG_DEBUG(log, ...) syslog(LOG_DEBUG, "[Debug]: %s:%u: "log, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define ROLLING_LOG_DEBUG(log, ...) g_print("[Debug]: %s:%u: "log, __FUNCTION__, __LINE__, ##__VA_ARGS__);
// #define ROLLING_LOG_DEBUG(log, ...)

#define ROLLING_LOG_CLOSE closelog();

#endif /* _ROLLING_LOG_H_ */

