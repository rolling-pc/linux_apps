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
 * @file rolling_flash_parse_xml.h
 * @author qifeng.liu@rollingwireless.com (liuqifeng)
 * @brief rolling flash parse xml header file
 * @version 1.0 
 * @date 2025-11-23
 *
 *
 **/
#ifndef __ROLLING_FLASH_PARSE_XML_H__
#define __ROLLING_FLASH_PARSE_XML_H__

#include "rolling_flash_common.h"

#ifndef EOK
#define EOK (0)
#endif

#define DEV_PKG_PATH     "/etc/opt/rolling/rolling_fw_pkg/FwPackage/DEV_OTA_PACKAGE/"

int get_fwinfo(fw_details *fwinfo);
void find_path_of_file(const char* file, char* directory, char *pathoffile);

void find_switch_table(char *docname, xmlChar *oemver, xmlChar *subsys_id);
static void search_switchtbl_using_oemver(xmlNode *a_node, const xmlChar *oemver, const xmlChar *subsys_id);
void find_carrier_id(char* docname,xmlChar* mccmnc);
void find_fw_version_default(char* docname, xmlChar* carrier_id, xmlChar* subsys_id );
static void search_fw_version_default(xmlNode *a_node, const xmlChar *carrier_id, const xmlChar *subsys_id);
void find_fw_version(char* docname, xmlChar* carrier_id, xmlChar* subsys_id);
static void search_fw_version(xmlNode *a_node, const xmlChar *carrier_id, const xmlChar *subsys_id);
void fw_version_info_init();
void switch_table_info_init();
void dev_version_init();
void carrier_id_init();
static void search_cid(xmlNode *a_node, const xmlChar *mccmnc);
static void search_skuid(xmlNode *a_node, const xmlChar *oemver);
void find_oem_pack_ver_pkg_info(char *docname,xmlChar *oemver);
static void search_oempack_ver(xmlNode *a_node, xmlChar* oemver);
void find_dev_image(char *docname,xmlChar *oemver, xmlChar* wwandevconfid, xmlChar *skuid,
    xmlChar *subsys_id);
static void search_dev_pack(xmlNode *a_node, xmlChar* oemver, xmlChar* wwandevconfid,
        xmlChar* skuid, xmlChar *subsys_id);

#endif
