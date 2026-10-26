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
 * @file rolling_config_main.c
 * @author qifeng.liu@rollingwireless.com (liuqifeng)
 * @brief rolling config main source file
 * @version 1.0
 * @date 2025-11-23
 *
 *
 **/

#include <stdio.h>
#include <ctype.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>
#include <glib.h>
#include <gio/gio.h>
#ifdef G_OS_UNIX
#include <gio/gunixfdlist.h>
#endif
#include <assert.h>
#include <time.h>
#include "version.h"
#include "rolling_log.h"
#include "rolling_static_config.h"
#include "rolling_dynamic_config.h"
#include "rolling_config_helper.h"

struct option long_options[] = {
    {"version", no_argument, NULL, 'v'},
    {"debug", required_argument, NULL, 'd'},
    {"loglevel", optional_argument, NULL, 'l'},
};

static void print_usage(void)
{
    extern const char *__progname;
    fprintf(stderr,
            "%s -v -d -l <Level> ...\n",
            __progname);
    fprintf(stderr,
            " -v                     --force                      print  version\n"
            " -d <debug level:xx>    --debug <debug:num>          LOG Debug\n"
            " -l <id:file path>      --Level <Level:string>       Level for bin\n"
            "\n"
            "Example: \n"
            "./rolling_flash -v -d -l 7\n");
}

void log_set(int argc, char *argv[])
{
    int option = 0;

    do
    {
        option = getopt_long(argc, argv, "vdl:", long_options, NULL);
        switch (option)
        {
        case 'v':
            ROLLING_LOG_ERROR("rolling_flash_service version:%s", CONFIG_VERSION_STRING);
            break;
        case 'd':
            ROLLING_LOG_ERROR("option d,%s\n", optarg);
            g_debug_level = 7;
            break;
        case 'l':
            if (atoi(optarg) >= 0 && atoi(optarg) <= 7)
            {
                g_debug_level = atoi(optarg);
            }
            else
            {
                ROLLING_LOG_ERROR("log level is invalue :%s\n", optarg);
            }
            ROLLING_LOG_ERROR("log level is :%s,g_debug_level: %d\n", optarg, g_debug_level);
            break;
        case '?':
            print_usage();
            break;
        default:
            break;
        }

    } while (option != -1);
}


static guint timeout_id = 0;
static int inhibit_fd = -1;
GDBusProxy *proxy =NULL;
#define TIMEOUT_MS 5000
#define POWER_SUSPEND   0x01
#define POWER_REBOOT    0x02
#define POWER_SHUTDOWN  0x04


/* Example inhibit_done callback function prototype.
 * You must modify this function according to your needs.
 *
 * This callback is invoked when the asynchronous DBus call completes.
 */
static void
inhibit_done (GObject      *source_object,
              GAsyncResult *res,
              gpointer      user_data)
{
    int *inhibit_fd_ptr = (int *)user_data;
    GError *error = NULL;
    GUnixFDList *out_fd_list = NULL;
    GVariant *result = g_dbus_proxy_call_with_unix_fd_list_finish(
        G_DBUS_PROXY(source_object), &out_fd_list, res, &error);
        if (result == NULL) {
        ROLLING_LOG_ERROR("Inhibit call failed: %s\n", error->message);
        g_error_free(error);
        *inhibit_fd_ptr = -1;
        return;
    }
    g_variant_unref(result);
    if (out_fd_list != NULL) {
        int fd = g_unix_fd_list_get(out_fd_list, 0, &error);
        if (error != NULL) {
            ROLLING_LOG_ERROR("Error retrieving file descriptor: %s\n", error->message);
            g_error_free(error);
            *inhibit_fd_ptr = -1;
        } else {
            *inhibit_fd_ptr = fd;
            ROLLING_LOG_INFO("Sleep inhibited successfully, fd: %d\n", *inhibit_fd_ptr);
        }
        g_object_unref(out_fd_list);
    } else {
        ROLLING_LOG_INFO("No file descriptor received.\n");
        *inhibit_fd_ptr = -1;
    }
}

static void
take_inhibitor (GDBusProxy *proxy, int *inhibit_fd_ptr)
{
    /* Ensure we currently have no inhibitor held */
    g_assert(*inhibit_fd_ptr == -1);

    ROLLING_LOG_INFO("Taking systemd sleep inhibitor\n");

    /* Call the Inhibit method with the following arguments:
       - what: "sleep"
       - who: "ModemManager"
       - why: "ModemManager needs to reset devices" (can be translated if needed)
       - mode: "delay"
    */
    g_dbus_proxy_call_with_unix_fd_list(
                             proxy,
                             "Inhibit",
                             g_variant_new("(ssss)",
                                           "sleep",
                                           "config",
                                           "Blocking for config service",
                                           "delay"),
                             0,                /* no flags */
                             G_MAXINT,         /* use the default timeout */
                             NULL,             /* no specific FD list */
                             NULL,             /* no cancellation object */
                             inhibit_done,     /* callback function */
                             inhibit_fd_ptr);  /* pass the pointer as user data */

    /* The rest of the handling will be in the inhibit_done callback */
}

static gboolean drop_inhibitor(void)
{
    if (inhibit_fd >= 0) {
        ROLLING_LOG_INFO("Dropping systemd sleep inhibitor\n");
        close(inhibit_fd);
        inhibit_fd = -1;
        return TRUE;
    }
    return FALSE;
}


gboolean timeout_callback(gpointer user_data) {
    ROLLING_LOG_INFO("Timeout reached, releasing sleep inhibition");
    
    // 释放休眠抑制
    drop_inhibitor();
    timeout_id = 0;
    return G_SOURCE_REMOVE;
}

// GET_CURRENT_RADIO_STATUS,
// SET_RADIO_STATUS,
static void send_message_to_helper(int messageid, char *senddata, char *receivedata)
{
    mesg_info *response = NULL;
    if (!send_message_get_response(messageid, senddata, strlen(senddata), &response))
    {
        if (NULL != response)
        {
            free(response);
        }
        return;
    }
    ROLLING_LOG_DEBUG("current data:%s", response->payload);
    strncpy(receivedata, response->payload, response->payload_lenth);
    if (NULL != response)
    {
        free(response);
    }
}

static char find_first_special_char(const char *str) {
    while (*str) {
        if (!isspace((unsigned char)*str) && !isalpha((unsigned char)*str)) {
            return *str;
        }
        str++;
    }
    return '\0';
}

static void on_signal_received(GDBusConnection *connection, const gchar *sender_name,
                               const gchar *object_path, const gchar *interface_name,
                               const gchar *signal_name, GVariant *parameters, gpointer user_data) {
    gboolean going_to_sleep;
    char respdata[32] = {0};
    static char prestatus;
    
    g_variant_get(parameters, "(b)", &going_to_sleep);

    if (going_to_sleep) {
        ROLLING_LOG_INFO("System is going to sleep");
        if (timeout_id == 0) {
            timeout_id = g_timeout_add(TIMEOUT_MS, timeout_callback, NULL);
        }
        // send cfun=4
        send_message_to_helper(GET_CURRENT_RADIO_STATUS, "", respdata);
        
        prestatus = find_first_special_char(respdata);
        ROLLING_LOG_INFO("get radio prestatus :%c",prestatus);
        if(NULL == strchr(respdata, '4'))
        {
            
            memset(respdata, 0, sizeof(respdata));
            send_message_to_helper(SET_RADIO_STATUS, "4", respdata);
            ROLLING_LOG_INFO("set radio status :%s", "4");
        }
        if (timeout_id != 0) {
            g_source_remove(timeout_id);
            timeout_id = 0;
        }
        drop_inhibitor();
    } else {
        ROLLING_LOG_INFO("System has woken up");
        inhibit_fd = -1;
        take_inhibitor(proxy, &inhibit_fd);
        
        if('4' != prestatus)
        {
            // send cfun = prestatus
            send_message_to_helper(SET_RADIO_STATUS, &prestatus, respdata);
            ROLLING_LOG_INFO("set radio status :%s", respdata);
        }
    }
}


void *suspend_monitor(void *arg)
{
    GMainLoop *loop;
    GDBusConnection *connection;
    GError *error = NULL;

    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!connection) {
        ROLLING_LOG_INFO("Failed to connect to system bus");
        return NULL;
    }

    proxy = g_dbus_proxy_new_for_bus_sync(
                             G_BUS_TYPE_SYSTEM,
                             G_DBUS_PROXY_FLAGS_NONE,
                             NULL,  /* no GDBusInterfaceInfo */
                             "org.freedesktop.login1",
                             "/org/freedesktop/login1",
                             "org.freedesktop.login1.Manager",
                             NULL,
                             &error);
    if (!proxy) {
        g_print("Failed to create DBus proxy: %s\n", error->message);
        g_error_free(error);
        return NULL;
    }

    /* Initially, no inhibitor is held */
    inhibit_fd = -1;
    take_inhibitor(proxy, &inhibit_fd);
    g_dbus_connection_signal_subscribe(
        connection, "org.freedesktop.login1", "org.freedesktop.login1.Manager",
        "PrepareForSleep", "/org/freedesktop/login1", NULL, G_DBUS_SIGNAL_FLAGS_NONE,
        on_signal_received, NULL, NULL);
    
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    return NULL;
}


extern void file_test_data(void);
int main(int argc, char **argv)
{
    bool result = 0;
    
    
    pthread_t event_tid;
    pthread_t dynamic_tid;
    pthread_t suspend_tid;
    
    void *event_result = NULL;
    void *dynamic_result = NULL;
    void *suspend_result = NULL;
    char network_mcc[8] = {0};
    int cnt = 0;

    ROLLING_LOG_INFO("rolling_config_service version:%s", CONFIG_VERSION_STRING);

    if (!rolling_load_static_ini_cfg())
    {
        ROLLING_LOG_ERROR("ini file parse error");
        return 1;
    }
    rolling_set_debug_level();
    log_set(argc, argv);
    if (0 == rolling_get_start_state())
    {
        ROLLING_LOG_INFO("rolling_config_service config not run");
        return 0;
    }
    // file_test_data();
    // sleep 5 second wait module ready;
    sleep(5);

    if (!msg_init() || !rolling_dbus_sem_init())
    {
        ROLLING_LOG_ERROR("msg_init failed");
        return 1;
    }
    if(!rolling_config_xml_exist())
    {
        pthread_t monitor_tid;
        rolling_cfg_sem_init();

        result = pthread_create(&monitor_tid, NULL, monitor_config_file_thread, NULL);
        if (result)
        {
            ROLLING_LOG_ERROR("create monitor_config_file_thread error");
            return 1;
        }
        ROLLING_LOG_INFO("config file not exist wait file install,wait...");
        rolling_cfg_sem_wait();
        pthread_join(monitor_tid, NULL);

    }
    if (!rolling_static_config_paese())
    {
        ROLLING_LOG_ERROR("static config error");
        return 1;
    }

    bool suspend_status = rolling_get_suspend_radio();
    if (suspend_status == true)
    {
        // SUSPEND_RESUEM_WA
        result = pthread_create(&suspend_tid, NULL, suspend_monitor, NULL);
        if (result)
        {
            ROLLING_LOG_ERROR("create event_from_file_thread error");
            return 1;
        }
    }

    custom_solucion_type sloution_type = rolling_get_customizationsolutiontype();
    if (SLUCTION_TYPE_NO != sloution_type)
    {
        // receive thread should init before receive event message
        result = pthread_create(&dynamic_tid, NULL, dynamic_thread, NULL);
        if (result)
        {
            ROLLING_LOG_ERROR("create event_from_file_thread error");
            return 1;
        }
    }

    rolling_dus_init();
    ROLLING_LOG_INFO("wait dbus connect service...");
    if ((dbus_service_is_ready()) && (cfg_get_port_state()))
    {
        ROLLING_LOG_DEBUG("connect dbus is ready not wait!");
    }
    else
    {
        ROLLING_LOG_INFO("not connect dbus wait...");
        rolling_dbus_sem_wait();
    }

    while (!send_message_test())
    {
        if (cnt > 5)
        {
            ROLLING_LOG_INFO("send message test cnt :%d,port abnormal", cnt);
            break;
        }
        cnt++;
        sleep(5);
    }
    ROLLING_LOG_INFO("dbus and port is ready,step next...");
    cnt = 0;
again:
    if (get_current_config())
    {
        // static config was executed earlier when the received signal was received.
        //  Therefore, the initialization is monitored here. If the initialization is not
        //  successful in the signal callback, the initialization is performed here.
        //  (If the initialization is being performed in the callback,
        //  the subsequent initialization operations need to be performed after the initialization is completed.)
        while (!static_config_set())
        {
            if (cnt > 5)
            {
                ROLLING_LOG_ERROR("[init static config] config set failed");
                break;
            }
            sleep(20);
            cnt++;
        }
    }
    else
    {
        if ((!static_config_set()) && (get_current_config()))
            goto again;
    }

    // first check cueernt mcc
    rolling_get_network_mcc(network_mcc);
    if (network_mcc[0] != '\0' && network_mcc[0] != '0')
    {
        send_event_by_mcc_change(network_mcc);
    }
    if (SLUCTION_TYPE_NO != sloution_type)
    {
        ROLLING_LOG_DEBUG("create event_from_signal_thread start");
        result = pthread_create(&event_tid, NULL, event_from_signal_thread, NULL);
        if (result)
        {
            ROLLING_LOG_ERROR("create event_from_signal_thread error");
            return 1;
        }
    }
    // rolling_dbus_run();
    if (SLUCTION_TYPE_NO != sloution_type)
    {
        pthread_join(event_tid, &event_result);
        pthread_join(dynamic_tid, &dynamic_result);
    }
    if (suspend_status == true)
    {
        pthread_join(suspend_tid, &suspend_result);
    }
    rolling_static_deinit();
    dynamic_deinit();
    return 0;
}
