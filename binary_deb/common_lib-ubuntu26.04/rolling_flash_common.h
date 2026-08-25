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
 * @file rolling_flash_common.h
 * @author qifeng.liu@rollingwireless.com (liuqifeng)
 * @brief rolling flash common header file
 * @version 1.0
 * @date 2025-11-23
 *
 *
 **/
#ifndef __COMMON_BASE_FUNC_H__
#define __COMMON_BASE_FUNC_H__


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
#include <signal.h>
#include <unwind.h>
#include <dlfcn.h>
#include <syslog.h>
#include <dirent.h>
#include <glib.h>
#include <gio/gio.h>
#include <glib/gtypes.h>
#include <locale.h>
#include <assert.h>
#include <ctype.h>
#include <zlib.h>
#include <getopt.h>

#include <sys/ioctl.h>
#include <sys/un.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/inotify.h>
#include <sys/stat.h>

#include <libmbim-glib/libmbim-glib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <linux/netlink.h>

#include "safe_str_lib.h"
#include "rolling-helper-gdbus-generated.h"
#include "rolling_log.h"
#include "version.h"

#define LIST_NUM 8
#define AT_COMMAND_LEN		(256)

#define UPGRADE_MAX_TIMES	(1)
#define CMD_OUTPUT_LEN      (64)
#define DEV_SUBSYSID_LEN	(64)
#define DEV_IMEI_LEN		(32)
#define MAX_VERSION_LEN		(100)
#define ERROR_RETRY_COUNT	(60)
#define RETRY_COUNT			(20)

#define GDEBUG_INIT_FAILED  -1

#define CONFIG_FILE_PATH	    "/etc/opt/rolling/run/FwFlashSrv"
#define INI_PATH			    "/etc/opt/rolling/config_file/rolling_flash_service/FwUpdate.ini"
#define NEW_PACKAGE_PATH	    "/etc/opt/rolling/rolling_fw_pkg/FwPackage.zip"
#define FWPACKAGE_PATH		    "/etc/opt/rolling/rolling_fw_pkg/FwPackage/"

extern RollingGdbusHelper *proxy;
extern int fast_boot_timer_source;
extern char g_mccmnc[32];
static GMainLoop *gMainloop = NULL;

typedef enum
{
    GET,
    SET,
    UNKNOWN_TYPE,
} e_command_type;
typedef struct g_flags{
    e_command_type type;
    int flag_arry[3];
} g_flags;
/*
 g_full_flags.flag_arry[0]:   9008->reboot -> flag = 1
 g_full_flags.flag_arry[1]:   9008->reboot->reboot_flag =1 > ready_falsh_flag = 1-> flash
 g_full_flags.flag_arry[2]:    modem port state:0 nopoert 1: flashport/fastbootport/normalport
*/
extern g_flags g_full_flags;


typedef struct {
    int package_flag;
    int edl_flag;
    int retry;
    char subSysId[DEV_SUBSYSID_LEN];
    char IMEI[DEV_IMEI_LEN];
    char lvfsFirmVer[MAX_VERSION_LEN];
} flash_info;

typedef struct {
    char *fw_ver;
    char *cust_pack;
    char *oem_pack;
    char *dev_pack;
    char *ap_ver;
    int antirb_md;
    int antirb_ap;
    char fake;
} fw_details;

typedef struct {
    char fw_ver[DEV_SUBSYSID_LEN];
    char cust_pack[DEV_SUBSYSID_LEN];
    char oem_pack[DEV_SUBSYSID_LEN];
    char dev_pack[DEV_SUBSYSID_LEN];
    char ap_ver[DEV_SUBSYSID_LEN];
    char antirb_md_ap[DEV_SUBSYSID_LEN];
    char fake_ver[DEV_SUBSYSID_LEN];
    int antirb_md;
    int antirb_ap;
    char fake;
} mdmver_details;
extern mdmver_details g_curmdm_versions;

// 函数声明
g_flags get_set_reboot_flag(g_flags value);
gboolean sim_status_handler(RollingGdbusHelper *object, const char *value, gpointer userdata);
gboolean fastboot_status_handler(RollingGdbusHelper *object, const char *value, gpointer userdata);
void print_usage(void);

typedef enum {
    FW_UPDATE_FLOW_UNLOCK,
    FW_UPDATE_FLOW_LOCK,
    FW_UPDATE_FLOW_STATE_INIT
} e_flow_state;
typedef enum {
    CMD_GET_MFC,
    CMD_GET_WWANID,
    CMD_GET_PRODUCTNAME,
    CMD_GET_PRODUCTID,
    CMD_GET_SKUID,
    CMD_MAX_LIST
} e_allow_cmd;

typedef enum {
    GET_SECTION = 1,
    GET_KEY,
    INI_FLAG_INIT
} e_ini_flag;

typedef enum
{
    HELPER,
    FWFLASH,       /* FWflash service */
    FWRECOVERY,    /* FWRecovery service */
    MASERVICE,     /* MA service */
    CONFIGSERVICE, /* Config service */
    UNKNOWN_SERVICE /* unknown service */
} e_service_id;

typedef enum
{
    OK = 0,
    ERROR,
    UNKNOWNPROJECT
} e_error_code;

typedef enum
{
    NO_FLASH = 0,
    AUTO,
    FORCE,
    FACTORY_MODE
} e_update_option;

typedef enum
{
    INIT = 0,
    NEW_PACKAGE,
    DECOMPRESS_SUCCESS,
    FLASH_PRE,
    FLASH_START,
    FLASH_FAIL,
    FLASH_SUCCESS
} e_pkg_flag;

typedef enum
{
    EDL_INIT = 0,
    EDL_FLASH_PRE,
    EDL_FLASH_FAIL,
    EDL_FLASH_SUCCESS
} e_edl_pkg_flag;

typedef enum
{
    GET_AP_VERSION = 0x1001,
    GET_MD_VERSION,
    GET_OP_VERSION,
    GET_OEM_VERSION,
    GET_DEV_VERSION,
    GET_IMEI,
    GET_MCCMNC,
    GET_SUBSYSID,
    SET_ATTACH_APN,
    FLASH_FW,
    SET_WARNING_BAR,
    GET_ANTIRB,
    GET_FAKE_VERSION,
    /* FWrecovery service command list */
    GET_PORT_STATE         = 0x2001,
    GET_OEM_ID,
    RESET_MODEM_HW,
    FLASH_FW_EDL,
    /* Modify By zhangxu : Check whether the oem and the current module match  */
    CHECK_SUPPORT_LIST,
    UNKNOWN_COMMAND
} e_command_cid;

typedef enum
{
    NO_PORT,
    NORMAL_PORT,
    FLASH_PORT,
    FASTBOOT_PORT,
    DUMP_PORT,
    UNKNOWN_PORT,
} e_port_state;



typedef enum
{
    REBOOTFLAG,
    READYFLASHFLAG,
    PORTSTATEFLAG,
    UNKNOWNFLAG,
} e_flags;
extern pthread_mutex_t flash_mutex;
extern pthread_cond_t lvfs_extract_cond;
void log_init();
void log_set(int argc, char *argv[]);
int find_ini(const char *filename, const char *section, const char *key, int *section_pos, int *key_pos);
void get_subSysID_from_file(char *subSysID);
int get_mccmnc(char *mccmnc);
int get_imei(char *imei);
int get_subSysID(char *subSysid);
void get_skuID(char *skuid);
void get_productID(char *product_id);
void get_product_name(char *product_name);
int get_modem_version_info();
void get_trimmed_bios_string(int cmd, char *output, const char *log_tag);
int get_retry_times();
int get_keyString(const char *filename, const char *section, const char *key, char *result);
void save_update_retry(int retry_times);
void reset_update_retry();
void execute_cmd(int cmd_id, char* result);
bool check_valid_string(char *result);
e_pkg_flag get_package_flag();
e_pkg_flag get_package_flag_unlock();
void set_package_flag(e_pkg_flag flag);
e_edl_pkg_flag get_edl_flag();
e_edl_pkg_flag get_edl_flag_unlock();
void set_edl_flag(e_edl_pkg_flag flag);
int call_helper_method_final(gchar *inarg, gchar *atresp, gint cids);

void *check_imei_change();
void *rolling_monitor_package(void *arg);
void *rolling_recovery_monitor(void *arg);
gboolean modem_status_handler(RollingGdbusHelper *object, const char *value, gpointer userdata);

void append_config_param();
void config_file_init();
int g_debus_init();

bool check_new_package();
void fw_update();
bool check_flash_flag();
void get_wwanconfigID(char *wwanconfigID);
gboolean comparative_oem_version();
gboolean flash_fw_with_recovery(char *ap, char *modem, char *oem);
gboolean flash_fw_with_all_img(void);
void trigger_rules_activation();
gboolean recovery_get_version_of_xml(char **ap, char **modem, char **oem, char **op, char *subsys_id);
bool check_power_status();
void UpdateSubSysid(char* subSysid);
void save_cur_subSysid(char *subSysid);
void save_cur_imei(char *imei);
bool update_need_sim_enable();
int parse_version_info(char* mccmnc_id, char* subsys_id, char* oemver, fw_details *fw_ver);
gboolean reboot_modem(gpointer data);
gboolean fastboot_reboot_callback();
gboolean stop_fastboot_timer();
gboolean init_flash_timer();
gboolean regester_interesting_siganl();
gboolean get_port_state(e_port_state *state);
gboolean start_flash_timer(int time);
bool try_get_modem_ap_version(void);

/* MM recover timing after fastboot->normal (see rolling_flash_common.c) */
#define MM_PORT_POLL_MS                 500
#define MM_PORT_WAIT_MAX_S              60
#define MM_FASTBOOT_GONE_DEBOUNCE_MS    500
#define MM_PORT_STABLE_MS               3000
#define MM_MODEM_POLL_MAX               15   /* 15 * MM_PORT_POLL_MS = 7.5s */
#define MM_MODEM_POLL_AFTER_RESTART_MAX 30   /* 30 * MM_PORT_POLL_MS = 15s */
#define MM_MODEM_STATE_WAIT_MAX_S       30   /* wait for mmcli state enabled/registered */
#define MM_RECOVER_RETRY_MAX            2
#define MM_RECOVER_RETRY_DELAY_MS       3000

void rolling_flash_mm_mark_post_fastboot_transition(void);
void rolling_flash_mm_clear_post_fastboot_transition(void);
gboolean rolling_flash_mm_is_post_fastboot_transition(void);
void rolling_flash_mm_on_fastboot_flash_start(void);
gboolean rolling_flash_mm_stop_for_fastboot(void);
/* Strategy C: fastboot stop -> bounce-settle -> start -> NM */
gboolean rolling_flash_mm_recover_pcie_modem(void);

#endif
