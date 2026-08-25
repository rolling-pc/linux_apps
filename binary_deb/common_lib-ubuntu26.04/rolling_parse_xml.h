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
 * @file rolling_parse_xml.h
 * @author qifeng.liu@rollingwireless.com (liuqifeng)
 * @brief rolling parse xml header file
 * @version 1.0
 * @date 2025-11-23
 *
 *
 **/

#ifndef __ROLLING_XML_PARSE_H__
#define __ROLLING_XML_PARSE_H__
#include <stdio.h>
#include <stdbool.h>
#include "rolling_list.h"

#define ROLLING_SKU_PATH_LEN 256
#define ROLLING_WWANCONFIGID_PATH_LEN 256
#define ROLLING_SELECT_TYPE 32
#define ROLLING_REGION_VER 32
#define ROLLING_MCC_LEN 16
#define ROLLING_REGULATORY_LEN 8
#define ROLLING_COUNTRY_LEN 32
#define ROLLING_COMBINE_LEN 16


#define SAR_MAP_TYPE_1 "MapType_1"
#define SAR_MAP_TYPE_2 "MapType_2"
#define SAR_MAP_TYPE_3 "MapType_3"
#define SAR_MAP_TYPE_4 "MapType_4"
#define SAR_MAP_TYPE_5 "MapType_5"


typedef enum parse_xml_type_t
{
    ROLLING_PARSE_XML_ESIM = 0,
    ROLLING_PARSE_XML_REGION_MAP,
    ROLLING_PARSE_XML_DEVMODE_SELECT_INDEX,
    SAR_PASE_XML_TYPE_UNKNOWN,
} parse_xml_type;

struct type_method
{
    char name[ROLLING_SELECT_TYPE];
    char num;
    char start;
    char end;
};

typedef struct rolling_sku_black_xml_s
{
    struct list_head list;
    char sku[16];
} rolling_sku_black_xml_t;

typedef struct rolling_mcc_black_xml_s
{
    struct list_head list;
    char mcc[16];
} rolling_mcc_black_xml_t;

typedef struct esim_xml_parse_rule_t
{
    bool esim_enable;
    char SystemSKU_path[ROLLING_SKU_PATH_LEN];
    char SelectType[ROLLING_SELECT_TYPE];
    struct type_method selet_method;
} esim_xml_parse_rule_t;

typedef struct esim_xml_parse_s
{
    esim_xml_parse_rule_t xml_parse_rule;
    /* rolling_sku_black_xml_t SKU_black_list;
    rolling_mcc_black_xml_t mccmnc_black_list; */
    struct list_head sku_black_list;
    struct list_head mcc_black_list;

} esim_disable_parse_t;

typedef struct device_to_sku_map_s{
    struct list_head dev_to_sku_list;
}device_to_sku_map_t;

typedef struct device_to_sku_list_s{
    char device_id[32];
    char sku_id[128];
    struct list_head list;
}device_to_sku_list_t;

typedef struct rolling_wwan_project_xml_s
{
    struct list_head list;
    char wwanconfigid[64];
    char projectid[64];
} rolling_wwan_project_xml_t;

typedef struct rolling_wwancfg_disable_xml_s
{
    struct list_head list;
    char wwanconfigid[32];
} rolling_wwancfg_disable_xml_t;

typedef struct rolling_antenna_xml_s
{
    char *wwanconfig_id;
    char device_mode;
    char index;
} rolling_antenna_xml_t;


typedef struct rolling_sar_xml1_s
{
    char *standard;
    char index;
} rolling_sar_xml1_t;

typedef struct rolling_sar_xml2_s
{
    char *standard;
    char device_mode;
    char index;
} rolling_sar_xml2_t;

typedef struct rolling_sar_xml3_s
{
    char *standard;
    char device_mode;
    char sensor1;
    char index;
} rolling_sar_xml3_t;

typedef struct rolling_sar_xml4_s
{
    char *standard;
    char device_mode;
    char sensor1;
    char sensor2;
    char index;
} rolling_sar_xml4_t;

typedef struct rolling_sar_xml5_s
{
    char *standard;
    char device_mode;
    char sensor1;
    char sensor2;
    char sensor3;
    char index;
} rolling_sar_xml5_t;

typedef struct devicemode_index_xml_parse_s
{
    bool select_index_enable;
    char path_number;
    char combinemode[ROLLING_COMBINE_LEN];
    char productname1_path[ROLLING_SKU_PATH_LEN];
    char boardproduct_path[ROLLING_WWANCONFIGID_PATH_LEN];
    char selectType[ROLLING_SELECT_TYPE];
    struct type_method selet_method;
    struct list_head wwan_project_list;
    struct list_head wwancfg_disable_list;

} devicemode_static_xml_parse_t;

typedef struct rolling_select_rule_xml_s
{
    struct list_head list;
    char mcc[ROLLING_MCC_LEN];
    char regulatory[ROLLING_REGULATORY_LEN];
    char country[ROLLING_COUNTRY_LEN];
} rolling_select_rule_xml_t;

typedef struct rolling_sar_custom_s
{
    struct list_head list;
    char regulatory[ROLLING_REGULATORY_LEN];
    char sar_type[1];
    char db_offset_enable[1];
} rolling_sar_custom_t;

typedef struct region_map_xml_parse_s
{
    char region_ver[ROLLING_REGION_VER];
    /* rolling_select_rule_xml_t select_rule;
    rolling_sar_custom_t custom_rule; */
    struct list_head select_rule_list;
    struct list_head sar_custom_list;
} region_map_xml_parse_t;


// int rolling_parse_xml(void);
// int xml_write(void);
// int xml_read(void);

bool rolling_parse_esim_xml_data(char *filename, esim_xml_parse_rule_t *xmldata,
                             struct list_head *list_sku, struct list_head *list_mcc);
bool rolling_parse_region_mapping_data(char *filename, char *parse_ver, char *version, struct list_head *select_rule_list,
                                   struct list_head *sar_custom_list);
bool rolling_parse_devicemode_static_data(char *filename, devicemode_static_xml_parse_t *xmldata);

bool rolling_parse_devicemode_index_data(char *filename, char *wwanconfig_id, char *map_type, void *xmldata);

bool rolling_parse_antenna_dynamic_data(char *filename, rolling_antenna_xml_t *xmldata);

bool rolling_parse_device_to_sku_xml_data(char *filename, struct list_head *list_device_id);

bool rolling_sartype_db_offset_parse_xml(char *filename, char *wwanconfigid, char *region, int *sar_type, int *de_offset);
#endif