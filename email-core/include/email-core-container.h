/*
*  email-service
*
* Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Kyuho Jo <kyuho.jo@samsung.com>, Sunghyun Kwon <sh0701.kwon@samsung.com>
* 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

/**
 *
 * This file contains functionality related to KNOX
 * to interact with email-service.
 * @file		email-core-container.c
 * @author	
 * @version	0.1
 * @brief 		This file contains functionality to provide KNOX support in email-service. 
 */

#include <glib.h>
#include <email-internal-types.h>
#ifdef __FEATURE_CONTAINER_ENABLE__
#include <vasum.h>
#else
typedef char* vsm_zone_h; 
#endif /* __FEATURE_CONTAINER_ENABLE__ */

INTERNAL_FUNC void emcore_create_container();
INTERNAL_FUNC void emcore_destroy_container();
INTERNAL_FUNC void emcore_bind_vsm_context();
INTERNAL_FUNC int  emcore_get_container_path(char *multi_user_name, char **container_path);
INTERNAL_FUNC int  emcore_get_user_name(int pid, char **multi_user_name);
INTERNAL_FUNC int  emcore_lookup_zone_by_name(char *user_name);
INTERNAL_FUNC void emcore_set_declare_link(const char *file_path);
INTERNAL_FUNC int  emcore_get_zone_name_list(GList **output_name_list);
INTERNAL_FUNC int  emcore_get_canonicalize_path(char *db_path, char **output_path);
INTERNAL_FUNC int  emcore_set_join_zone(char *multi_user_name, vsm_zone_h *join_zone);
INTERNAL_FUNC void emcore_unset_join_zone(vsm_zone_h join_zone);
