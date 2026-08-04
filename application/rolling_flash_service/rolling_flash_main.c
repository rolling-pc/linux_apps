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
 * @file rolling_flash_main.c
 * @author qifeng.liu@rollingwireless.com (liuqifeng)
 * @brief rolling flash main source file
 * @version 1.0
 * @date 2025-11-23
 *
 *
 **/

#include "rolling_flash_main.h"
#include "rolling_flash_download.h"

extern gboolean sim_status_handler(RollingGdbusHelper *object, const char *value, gpointer userdata);
extern gboolean fastboot_status_handler(RollingGdbusHelper *object, const char *value, gpointer userdata);

pthread_mutex_t flash_mutex;
pthread_cond_t lvfs_extract_cond;

void print_usage(void)
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
void trigger_rules_activation()
{
	FILE *zip_file = NULL;
	int ret = 0;
	char result[8] = {0};
	int download_lvfs = 0;
	FILE *config_file = NULL;

	ROLLING_LOG_INFO("entry");

	ret = get_keyString(INI_PATH, "BASE_CONFIG", "FwDownloadLVFS", result);
	if (ret)
	{
		ROLLING_LOG_ERROR("get ini config FwDownloadLVFS failed");
	}
	else
	{
		download_lvfs = atoi(result);
		ROLLING_LOG_INFO("Fw update option FwDownloadLVFS is %d", download_lvfs);
	}

	zip_file = fopen(NEW_PACKAGE_PATH, "r");
	config_file = fopen(CONFIG_FILE_PATH, "r+");
	if (!config_file)
	{
		ROLLING_LOG_INFO("FwFlashSrv file not exist, flash service start the first time");
	}
	if ((!config_file && download_lvfs) || zip_file != NULL)
	{
		ROLLING_LOG_INFO("first install need to reload rules");

		ret = system("udevadm control --reload-rules");
		if (ret)
		{
			ROLLING_LOG_ERROR("reload rules failed");
		}

		ret = system("udevadm trigger");
		if (ret)
		{
			ROLLING_LOG_ERROR("trigger failed");
		}
	}

	if (zip_file)
	{
		fclose(zip_file);
	}
	if (config_file)
	{
		fclose(config_file);
	}

	return;
}
void rolling_check_imei_run()
{
    pthread_t ptid;
    pthread_create(&ptid, NULL, &check_imei_change, NULL);
    ROLLING_LOG_INFO("[%s] check imei thread create!\n", __func__);
}
void rolling_monitor_package_run()
{
    pthread_t ptid;
    pthread_create(&ptid, NULL, &rolling_monitor_package, NULL);
    ROLLING_LOG_INFO("monitor package create\n");
}
void rolling_firmware_recovery_run()
{
    pthread_t ptid;
    pthread_create(&ptid, NULL, &rolling_recovery_monitor, NULL);
    ROLLING_LOG_INFO("[%s] Recovery thread create!\n", __func__);
}
void rolling_lvfs_run()
{
    pthread_t ptid;
    pthread_create(&ptid, NULL, &lvfs_download_firmware, NULL);
    ROLLING_LOG_INFO("[%s] lvfs thread create!\n", __func__);
}
void open_log(int argc, char *argv[])
{
    log_init();

    ROLLING_LOG_INFO("FW Flash service entry");
    openlog("FWFlashService", LOG_CONS | LOG_PID, LOG_USER);
    ROLLING_LOG_INFO("rolling_flash_service version:%s", FLASH_VERSION_STRING);
    log_set(argc, argv);
}
int main(int argc, char *argv[])
{
    flash_info checkInfo;
    GDBusConnection *conn = NULL;
    GError *connerror = NULL;
    GError *proxyerror = NULL;
    bool new_pkg = FALSE;
    bool need_flash_flag = FALSE;
    bool normal_port = FALSE;
    int ret;
    char port_status[32] = {0};
    e_error_code status = UNKNOWNPROJECT;
    FILE *g_file = NULL;
    int i;

    open_log(argc, argv);
    if (pthread_mutex_init(&flash_mutex, NULL) != 0) {
        ROLLING_LOG_ERROR("pthread_mutex_init flash_mutex failed");
        goto FINISH2;
    }
    if (pthread_cond_init(&lvfs_extract_cond, NULL) != 0) {
        ROLLING_LOG_ERROR("pthread_cond_init lvfs_extract_cond failed");
        goto FINISH1;
    }

    trigger_rules_activation();
    append_config_param();

    config_file_init(g_file, &checkInfo);
    ret = g_debus_init();
    if (ret == GDEBUG_INIT_FAILED)
    {
        goto FINISH;
    }

    /* create monitor package thread */
    rolling_monitor_package_run();
    /* create recovery thread */
    rolling_firmware_recovery_run();
    /* create lvfs download thrread */
    rolling_lvfs_run();
    gMainloop = g_main_loop_new(NULL, FALSE);

    new_pkg = check_new_package();
    if (FALSE == new_pkg)
    {
        if (0 != access(FWPACKAGE_PATH, F_OK))
        {
            ROLLING_LOG_INFO("=================================================================\n");
            ROLLING_LOG_INFO("********Please ensure that fw package has been installed.********\n");
            ROLLING_LOG_INFO("********Obtain the fw package from the corresponding OEM.********\n");
            ROLLING_LOG_INFO("=================================================================\n");
        }
    }

    for (i = 0; i < 15; i++)
    {
        status = check_port_state(port_status);
        if (ERROR == status)
        {
            ROLLING_LOG_ERROR("get port state failed");
            g_usleep(1000 * 1000 * 2);
            continue;
        }
        else
        {
            if (strstr(port_status, "normalport") == NULL)
            {
                ROLLING_LOG_INFO("port state is abnormal, wait...");
                g_usleep(1000 * 1000 * 2);
                continue;
            }
            else
            {
                normal_port = TRUE;
                ROLLING_LOG_INFO("port state is normal, start FW flash flow");
                break;
            }
        }
    }

    if (TRUE == normal_port)
    {
        ROLLING_LOG_INFO("port state is normal, start FW flash flow");

        if (new_pkg == TRUE)
        {
            fw_update();
        }
        else
        {
            need_flash_flag = check_flash_flag();
            if (need_flash_flag == TRUE) {
	            if(try_get_modem_ap_version()) {
                    fw_update();
		        }
		        else
		        {
		            flash_fw_with_all_img();
		        }
		    }
        }
    }

    g_signal_connect(proxy, "simcard-change", G_CALLBACK(sim_status_handler),NULL);
    g_signal_connect(proxy, "cellular-state",G_CALLBACK(modem_status_handler),NULL);
    g_signal_connect(proxy,"fastboot-status",G_CALLBACK(fastboot_status_handler),NULL);

    rolling_check_imei_run();

    g_main_loop_run(gMainloop);
    g_object_unref(proxy);

FINISH:
    pthread_mutex_destroy(&flash_mutex);
FINISH1:
    pthread_cond_destroy(&lvfs_extract_cond);
FINISH2:
    closelog();
    if (g_file != NULL)
    {
        fclose(g_file);
    }

    return 0;
}
