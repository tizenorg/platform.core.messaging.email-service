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

#include <email-utilities.h>

#include "email-core-container.h"
#include "email-debug-log.h"

#ifdef _FEATURE_CONTAINER_ENABLE__
int vsm_state_handle = 0;
int vsm_event_handle = 0;
vsm_context_h container;


static int container_state_listener(vsm_zone_h zone, vsm_zone_state_t state, void *user_data)
{
    EM_DEBUG_FUNC_BEGIN();
    EM_DEBUG_LOG("Container name : [%s]", vsm_get_zone_name(zone));
    EM_DEBUG_LOG("Container state : [%d]", state);
    EM_DEBUG_FUNC_END();
    return 0;
}

static int container_event_listener(vsm_zone_h zone, vsm_zone_event_t event, void *user_data)
{
    EM_DEBUG_FUNC_BEGIN();
    EM_DEBUG_LOG("Container name : [%s]", vsm_get_zone_name(zone));
    EM_DEBUG_LOG("Container event : [%d]", event);
    EM_DEBUG_FUNC_END();
    return 0;
}
#endif /* __FEATURE_CONTAINER_ENABLE__ */

INTERNAL_FUNC void emcore_create_container()
{
    EM_DEBUG_FUNC_BEGIN();
#ifdef __FEATURE_CONTAINER_ENABLE__
    container = NULL;

    container = vsm_create_context();
    if (container == NULL) {
        EM_DEBUG_EXCEPTION("vsm_create_context failed");
        return;
    }

    vsm_state_handle = vsm_add_state_changed_callback(container, container_state_listener, NULL);

	vsm_event_handle = vsm_add_event_callback(container, container_event_listener, NULL);
#endif /* __FEATURE_CONTAINER_ENABLE__ */
    EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void emcore_destroy_container()
{
    EM_DEBUG_FUNC_BEGIN();
#ifdef __FEATURE_CONTAINER_ENABLE__

    if (container) {
        vsm_del_state_changed_callback(container, vsm_state_handle);

		vsm_del_event_callback(container, vsm_event_handle);

        vsm_cleanup_context(container);

        container = NULL;
    }
#endif /* __FEATURE_CONTAINER_ENABLE__ */

    EM_DEBUG_FUNC_END();
}

static gboolean mainloop_callback(GIOChannel *channel, GIOCondition condition, void *data) 
{
    EM_DEBUG_FUNC_BEGIN();

#ifdef __FEATURE_CONTAINER_ENABLE__
    vsm_context_h ctx = (vsm_context_h)data;

    EM_DEBUG_LOG("Enter event loop");
    vsm_enter_eventloop(ctx, 0, 0);
    EM_DEBUG_LOG("Finish event loop");
#endif /* __FEATURE_CONTAINER_ENABLE__ */

    EM_DEBUG_FUNC_END();
    return TRUE;
}

INTERNAL_FUNC void emcore_bind_vsm_context()
{
	EM_DEBUG_FUNC_BEGIN();

#ifdef __FEATURE_CONTAINER_ENABLE__
	int fd;

	GIOChannel *domain_channel = NULL;

	fd = vsm_get_poll_fd(container);

	domain_channel = g_io_channel_unix_new(fd);
	if (domain_channel == NULL) {
		EM_DEBUG_EXCEPTION("Channel allocation failed\n");
		return;
	}

	g_io_add_watch(domain_channel, G_IO_IN, mainloop_callback, container);
#endif /* __FEATURE_CONTAINER_ENABLE__ */

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int emcore_get_container_path(char *multi_user, char **container_path)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

	if (multi_user == NULL) {
		EM_DEBUG_LOG("user_name is NULL");
		return EMAIL_ERROR_INVALID_PARAM;
	}

#ifdef __FEATURE_CONTAINER_ENABLE__
	if (container == NULL) {
		EM_DEBUG_LOG("Container not intialize");
		return EMAIL_ERROR_CONTAINER_NOT_INITIALIZATION;
	}

	vsm_zone_h zone = NULL;
	char *p_container_path = NULL;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	EM_DEBUG_LOG("name : [%s], container : [%p]", multi_user, container);
	zone = vsm_lookup_zone_by_name(container, multi_user);
	if (zone == NULL) {
		EM_DEBUG_EXCEPTION("NULL returned : %s, %d", EM_STRERROR(errno_buf), errno);
		err = EMAIL_ERROR_CONTAINER_GET_DOMAIN;
		goto FINISH_OFF;
	}

	p_container_path = (char *)vsm_get_zone_rootpath(zone);

FINISH_OFF:

	if (container_path)
		*container_path = EM_SAFE_STRDUP(p_container_path);

#endif /* __FEATURE_CONTAINER_ENABLE__ */
	EM_DEBUG_FUNC_END();
	return err;
}

INTERNAL_FUNC int emcore_get_user_name(int pid, char **multi_user)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;

	if (pid <= 0) {
			EM_DEBUG_EXCEPTION("Invalid parameter");
			return EMAIL_ERROR_INVALID_PARAM;
	}
#ifdef __FEATURE_CONTAINER_ENABLE__

	char *user_name = NULL;
	vsm_zone_h zone = NULL;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	EM_DEBUG_LOG("pid : [%d], container : [%p]", pid, container);
	zone = vsm_lookup_zone_by_pid(container, pid);
	if (zone == NULL) {
		EM_DEBUG_EXCEPTION("NULL returned : %s, %d", EM_STRERROR(errno_buf), errno);
		err = EMAIL_ERROR_CONTAINER_GET_DOMAIN;
		goto FINISH_OFF;
	}

	user_name = (char *)vsm_get_zone_name(zone);
	EM_DEBUG_LOG("user_name : [%s]", user_name);

FINISH_OFF:

	if (multi_user)
		*multi_user = EM_SAFE_STRDUP(user_name);

#endif /* __FEATURE_CONTAINER_ENABLE__ */
	EM_DEBUG_FUNC_END();
	return err;
}

INTERNAL_FUNC int emcore_lookup_zone_by_name(char *user_name)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = false;

	if (EM_SAFE_STRLEN(user_name) == 0) {
		EM_DEBUG_LOG("user_name is NULL");
		return ret;
	}

#ifdef __FEATURE_CONTAINER_ENABLE__
	if (container == NULL) {
		EM_DEBUG_LOG("Container not intialize");
		return ret;
	}

	vsm_zone_h zone = NULL;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	EM_DEBUG_LOG("name : [%s], container : [%p]", user_name, container);
	zone = vsm_lookup_zone_by_name(container, user_name);
	if (zone == NULL) {
		EM_DEBUG_EXCEPTION("NULL returned : %s, %d", EM_STRERROR(errno_buf), errno);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

#endif /* __FEATURE_CONTAINER_ENABLE__ */
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC void emcore_set_declare_link(const char *file_path)
{
    EM_DEBUG_FUNC_BEGIN();

#ifdef __FEATURE_CONTAINER_ENABLE__
    if (container == NULL) {
        EM_DEBUG_EXCEPTION("Not initialize");
        return;
    }

    int ret = 0;
    if ((ret = vsm_declare_link(container, file_path /* src path */, file_path /* dst path */)) != 0) {
        EM_DEBUG_EXCEPTION("vsm_declare_link failed : [%s] [%d]", file_path, ret);
    }
#endif /* __FEATURE_CONTAINER_ENABLE__ */

    EM_DEBUG_FUNC_END();
}

void iterate_callback(vsm_zone_h zone, void *user_data)
{
    EM_DEBUG_FUNC_BEGIN();
#ifdef __FEATURE_CONTAINER_ENABLE__
    char *zone_name = NULL;
    GList *zone_name_list = (GList *)user_data;

    zone_name = EM_SAFE_STRDUP(vsm_get_zone_name(zone));
    zone_name_list = g_list_append(zone_name_list, zone_name);
#endif /* __FEATURE_CONTAINER_ENABLE__ */
    
    EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int emcore_get_zone_name_list(GList **output_name_list)
{
    EM_DEBUG_FUNC_BEGIN();
    int err = EMAIL_ERROR_NONE;
    int ret = 0;
    GList *zone_name_list = NULL;
    GList *node = NULL;

    if (output_name_list == NULL) {
        EM_DEBUG_EXCEPTION("Invalid parameter");
        err = EMAIL_ERROR_INVALID_PARAM;
        goto FINISH_OFF;
    }

#ifdef __FEATURE_CONTAINER_ENABLE__
    if (container == NULL) {
        EM_DEBUG_EXCEPTION("Not initialize");
        err = EMAIL_ERROR_CONTAINER_CREATE_FAILED;
        goto FINISH_OFF;
    }

    zone_name_list = g_list_alloc();
    if (zone_name_list == NULL) {
        EM_DEBUG_EXCEPTION("g_list_alloc failed");
        err = EMAIL_ERROR_OUT_OF_MEMORY;
        goto FINISH_OFF;
    }

    if ((ret = vsm_iterate_zone(container, iterate_callback, (void *)zone_name_list)) < 0) {
        EM_DEBUG_EXCEPTION("vsm_iterate_domain failed : [%d]", ret);
        err = EMAIL_ERROR_CONTAINER_ITERATE_DOMAIN;
        goto FINISH_OFF;
    }
#endif /* __FEATURE_CONTAINER_ENABLE__ */

FINISH_OFF:

    if (err != EMAIL_ERROR_NONE) {
        node = g_list_first(zone_name_list);
        while (node != NULL) {
            /* Free the domain name */
            EM_SAFE_FREE(node->data);                   
            node = g_list_next(node);
        }
        g_list_free(zone_name_list);
        zone_name_list = NULL;
    }

    if (output_name_list)
        *output_name_list = zone_name_list;

    EM_DEBUG_FUNC_END();
	return err;
}

INTERNAL_FUNC int emcore_get_canonicalize_path(char *db_path, char **output_path)
{
    EM_DEBUG_FUNC_BEGIN();
    int err = EMAIL_ERROR_NONE;

#ifdef __FEATURE_CONTAINER_ENABLE__
	int ret = 0;
	ret = vsm_canonicalize_path(db_path, output_path);
	if (ret < 0) {
		EM_DEBUG_EXCEPTION("vsm_canonicalize_path failed : [%d]", ret);
		err = EMAIL_ERROR_FILE_NOT_FOUND;
	}
#endif /* __FEATURE_CONTAINER_ENABLE__ */

    EM_DEBUG_FUNC_END();
    return err;
}

INTERNAL_FUNC int emcore_set_join_zone(char *multi_user_name, vsm_zone_h *join_zone)
{
    EM_DEBUG_FUNC_BEGIN();
    int err = EMAIL_ERROR_NONE;
#ifdef __FEATURE_CONTAINER_ENABLE__
	char errno_buf[ERRNO_BUF_SIZE] = {0};
	vsm_zone_h temp_zone = NULL;
	vsm_zone_h p_join_zone = NULL;

	temp_zone = vsm_lookup_zone_by_name(container, multi_user_name);
	if (temp_zone == NULL) {
		EM_DEBUG_EXCEPTION("NULL returned : %s %d", EM_STRERROR(errno_buf), errno);
		err = EMAIL_ERROR_CONTAINER_LOOKUP_ZONE_FAILED;
		goto FINISH_OFF;
	}

	p_join_zone = vsm_join_zone(temp_zone);
	if (p_join_zone == NULL) {
		EM_DEBUG_EXCEPTION("NULL returned : %s %d", EM_STRERROR(errno_buf), errno);
		err = EMAIL_ERROR_CONTAINER_JOIN_ZONE_FAILED;
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (join_zone)
		*join_zone = p_join_zone;

#endif /* __FEATURE_CONTAINER_ENABLE__ */
    EM_DEBUG_FUNC_END();
    return err;
}

INTERNAL_FUNC void emcore_unset_join_zone(vsm_zone_h join_zone)
{
    EM_DEBUG_FUNC_BEGIN();
#ifdef __FEATURE_CONTAINER_ENABLE__

	char errno_buf[ERRNO_BUF_SIZE] = {0};
	vsm_zone_h temp_zone = NULL;

	temp_zone = vsm_join_zone(join_zone);
	if (temp_zone == NULL) {
		EM_DEBUG_EXCEPTION("NULL returned : %s %d", EM_STRERROR(errno_buf), errno);
	}

#endif /* __FEATURE_CONTAINER_ENABLE__ */
    EM_DEBUG_FUNC_END();
}
