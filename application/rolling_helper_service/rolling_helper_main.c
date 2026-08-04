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
 * @file rolling_helper_main.c
 * @author qifeng.liu@rollingwireless.com (liuqifeng)
 * @brief rolling helper main function
 * @version 1.0
 * @date 2025-11-23
 *
 *
 **/

#include <glib.h>
#include "unistd.h"
//with fork include
#include <sys/types.h>

#include "rolling_helper_common.h"
#include "rolling_helper_basic_func.h"
#include "common_base_func.h"

GMainLoop   *gMainLoop            = NULL;
GMainLoop   *gMainLoopMbim        = NULL;

static gint
rolling_helperm_control_receiver_init()
{
    int      ret              = RET_ERROR;
    GThread  *ctl_rcv_thread  = NULL;

    // step1: init message queue.
    ret = rolling_helper_queue_init();
    if (ret != RET_OK) {
        ROLLING_LOG_ERROR("message_queue_init failed! can't init message seq!\n");
        return RET_ERROR;
    }

    ctl_rcv_thread = g_thread_new ("control_recv", (GThreadFunc)rolling_helperm_control_receiver, NULL);
    if (!ctl_rcv_thread) {
        ROLLING_LOG_ERROR("thread init failed!\n");
        return RET_ERROR;
    }
    return RET_OK;
}

static gint
rolling_helperm_main_receiver_init()
{
    GThread  *main_rcv_thread = NULL;

    main_rcv_thread = g_thread_new ("req_recv", (GThreadFunc)rolling_helperm_main_receiver, NULL);
    if (!main_rcv_thread) {
        ROLLING_LOG_ERROR("thread init failed!\n");
        return RET_ERROR;
    }
    return RET_OK;
}

/* mbim thread func can't be blocked at any time! */
gint helper_mbim_thread(gint argc, char *argv[])
{
    guint   owner_id        = 0;
    gint    ret             = RET_ERROR;

    ROLLING_LOG_OPEN ("helper_mbim_analyzer");

    #if !GLIB_CHECK_VERSION (2,35,0)
    g_type_init ();
    #endif

    log_set(argc, argv);

    ret = rolling_function_table_init(&g_project_func_table);
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_function_table_init failed! exit mainloop!\n");
        return ret;
    }

    ret = rolling_platform_update_function_table(&g_project_func_table);
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_function_table_update failed! exit mainloop!\n");
        return ret;
    }

    ret = rolling_project_update_function_table(&g_project_func_table);
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_function_table_update failed! exit mainloop!\n");
        return ret;
    }

    ret = rolling_oem_update_function_table(&g_project_func_table);
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_function_table_update failed! exit mainloop!\n");
        return ret;
    }

    // step1: init a thread to get control message, aka, mbim init and close.
    ret = rolling_helperm_control_receiver_init();
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_helperm_control_receiver_init failed! exit mainloop!\n");
        return ret;
    }

    // step2: init main receiver to get normal request, but if mbim not ready, should send resp to message seq with error.
    ret = rolling_helperm_main_receiver_init();
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_helperm_main_receiver_init failed! exit mainloop!\n");
        return ret;
    }

    // main loop go cycle.
    gMainLoopMbim = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (gMainLoopMbim);

    g_main_loop_unref (gMainLoopMbim);
    gMainLoop = NULL;

    // rolling_mbim_port_deinit();

    rolling_function_table_deinit(&g_project_func_table);

    ROLLING_LOG_CRITICAL("exiting 'rolling-helper-mbim'...\n");
    ROLLING_LOG_CLOSE;

    return RET_OK;
}

static gint
rolling_helperd_control_receiver_init()
{
    int      ret                   = RET_ERROR;
    GThread  *mbim_ctl_rcv_thread  = NULL;

    ret = rolling_helper_queue_init();
    if (ret != RET_OK) {
        ROLLING_LOG_ERROR("message_queue_init failed! can't init message seq!");
        return RET_OK;
    }

    mbim_ctl_rcv_thread = g_thread_new ("mbim-ctl-rcv", (GThreadFunc)rolling_helperd_control_message_receiver, NULL);
    if (!mbim_ctl_rcv_thread) {
        ROLLING_LOG_ERROR("thread init failed!\n");
        return RET_ERROR;
    }

    return RET_OK;
}

// Global variable: store device check thread handle for resource cleanup on exit
static GThread *g_dev_check_thread = NULL;

// Thread cleanup function: called on program exit to ensure thread resource release
static void rolling_helperd_device_check_thread_cleanup(void)
{
    if (g_dev_check_thread != NULL) {
        ROLLING_LOG_DEBUG("Joining device check thread...\n");
        g_thread_join(g_dev_check_thread);
        g_dev_check_thread = NULL;
        ROLLING_LOG_DEBUG("Device check thread released!\n");
    }
}

static gint
rolling_helperd_device_check_thread_init()
{
    int      ret                = RET_ERROR;

    // If thread already exists, return success directly
    if (g_dev_check_thread != NULL) {
        ROLLING_LOG_WARNING("Device check thread already running!\n");
        return RET_OK;
    }

    g_dev_check_thread = g_thread_new ("dev-check", (GThreadFunc)rolling_helperd_device_state_init, NULL);
    if (!g_dev_check_thread) {
        ROLLING_LOG_ERROR("thread init failed!\n");
        return RET_ERROR;
    }

    // Register exit cleanup function
    atexit(rolling_helperd_device_check_thread_cleanup);
    ROLLING_LOG_DEBUG("Device check thread started successfully.\n");

    return RET_OK;
}

/* helper thread func can't be blocked at any time! */
gint helper_thread(gint argc, char *argv[])
{
    gint    ret        = RET_ERROR;

    ROLLING_LOG_OPEN ("helper");
    ROLLING_LOG_INFO("rolling_helper_service version:%s", HELPER_VERSION_STRING);

    log_set(argc, argv);

    // g_type_system will be initialized automatically from glib 2.36. we need to initialized it manually if glib version is before 2.36.
    // this system will be used to support object-faced language's class, object feature.
    #if !GLIB_CHECK_VERSION (2,35,0)
        g_type_init ();
    #endif

    ret = rolling_function_table_init(&g_project_func_table);
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_function_table_init failed! exit mainloop!\n");
        return ret;
    }

    ret = rolling_platform_update_function_table(&g_project_func_table);
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_function_table_update failed! exit mainloop!\n");
        return ret;
    }

    ret = rolling_project_update_function_table(&g_project_func_table);
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_function_table_update failed! exit mainloop!\n");
        return ret;
    }

    ret = rolling_oem_update_function_table(&g_project_func_table);
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_function_table_update failed! exit mainloop!\n");
        return ret;
    }

    // cause other service will call helper to execute command, here must register it to dbus firstly.
    ret = rolling_register_helper_service();
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_register_helper_service failed! exit mainloop!\n");
        return ret;
    }

    ret = rolling_mutex_init();
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_mutex_init failed! exit mainloop!\n");
        return ret;
    }

    // Setup signals
    ret = rolling_set_linux_app_signals();
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_set_linux_app_signals failed! exit mainloop!\n");
        return ret;
    }

    ret = rolling_helperd_control_receiver_init();
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_helperd_control_receiver_init failed! exit mainloop!\n");
        return ret;
    }

    ret = rolling_helperd_device_check_thread_init();
    if (ret != RET_OK) {
        ROLLING_LOG_CRITICAL("rolling_helperd_device_check_thread_init failed! exit mainloop!\n");
        return ret;
    }

    // main loop go cycle.
    gMainLoop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (gMainLoop);

    /* Below funcs are used to unregister all dependent variables and exit main func */

    ROLLING_LOG_DEBUG("Main loop exited, starting cleanup...\n");

    // Set exit flag to notify sub-thread to exit
    if (g_dev_check_sub_thread != NULL) {
        ROLLING_LOG_DEBUG("Setting exit flag for device check sub thread...\n");
        g_atomic_int_set(&g_dev_check_sub_exit, TRUE);

        // Wait for sub-thread to finish
        ROLLING_LOG_DEBUG("Waiting for device check sub thread to exit...\n");
        g_thread_join(g_dev_check_sub_thread);
        g_dev_check_sub_thread = NULL;
        ROLLING_LOG_DEBUG("Device check sub thread exited.\n");
    }

    rolling_mutex_force_sync_unlock();

    // rolling_udev_deinit();

    rolling_deinit_message_queue();

    rolling_function_table_deinit(&g_project_func_table);

    g_main_loop_unref (gMainLoop);
    gMainLoop = NULL;

    ROLLING_LOG_CRITICAL("exiting 'rolling-helper-dbus'...\n");
    ROLLING_LOG_CLOSE;

    return RET_OK;
}


gint main(gint argc, char *argv[])
{
    pid_t pid = 0;

// further: init once and fork.
    pid = fork();
    if(pid == 0)
    {
        helper_mbim_thread(argc, argv);
    }
    else if(pid > 0)
    {
        helper_thread(argc, argv);
    }
    else
    {
        printf("[%s][%s]: Error!!! please check!\n", __func__, __FILE__);
    }

    return RET_OK;
}
