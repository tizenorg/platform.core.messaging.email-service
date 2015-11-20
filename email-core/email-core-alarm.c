/*
*  email-service
*
* Copyright (c) 2013 Samsung Electronics Co., Ltd. All rights reserved.
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


/******************************************************************************
 * File :  email-core-alarm.c
 * Desc :  Alarm Management
 *
 * Auth :  Kyuho Jo
 *
 * History :
 *    2013.01.28  :  created
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <vconf.h>
#include <glib.h>
#include <alarm.h>

#include "email-convert.h"
#include "email-types.h"
#include "email-daemon.h"
#include "email-debug-log.h"
#include "email-storage.h"
#include "email-network.h"
#include "email-device.h"
#include "email-utilities.h"
#include "email-core-utils.h"
#include "email-core-global.h"
#include "email-core-alarm.h"


INTERNAL_FUNC GList *alarm_data_list = NULL;
static pthread_mutex_t alarm_data_lock = PTHREAD_MUTEX_INITIALIZER;

INTERNAL_FUNC int emcore_get_alarm_data_by_alarm_id(alarm_id_t input_alarm_id, email_alarm_data_t **output_alarm_data)
{
	EM_DEBUG_FUNC_BEGIN("input_alarm_id [%d] output_alarm_data [%p]", input_alarm_id, output_alarm_data);
	int err = EMAIL_ERROR_NONE;
	GList *index = NULL;
	email_alarm_data_t *alarm_data = NULL;

	if (output_alarm_data == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ENTER_CRITICAL_SECTION(alarm_data_lock);
	index = g_list_first(alarm_data_list);
	while(index) {
		alarm_data = index->data;
		if(alarm_data->alarm_id == input_alarm_id) {
			break;
		}
		index = g_list_next(index);
	}
	LEAVE_CRITICAL_SECTION(alarm_data_lock);

	if(alarm_data)
		*output_alarm_data = alarm_data;
	else
		err = EMAIL_ERROR_ALARM_DATA_NOT_FOUND;

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_get_alarm_data_by_reference_id(email_alarm_class_t input_class_id, int input_reference_id, email_alarm_data_t **output_alarm_data)
{
	EM_DEBUG_FUNC_BEGIN("input_class_id [%d] input_reference_id [%d] output_alarm_data [%p]", input_class_id, input_reference_id, output_alarm_data);
	int err = EMAIL_ERROR_NONE;
	GList *index = NULL;
	email_alarm_data_t *alarm_data = NULL;

	if (output_alarm_data == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ENTER_CRITICAL_SECTION(alarm_data_lock);

	index = g_list_first(alarm_data_list);

	while(index) {
		alarm_data = index->data;
		if(alarm_data->class_id == input_class_id && alarm_data->reference_id == input_reference_id) {
			EM_DEBUG_LOG("found");
			break;
		}
		index = g_list_next(index);
	}

	LEAVE_CRITICAL_SECTION(alarm_data_lock);

	if(index)
		*output_alarm_data = alarm_data;
	else
		err = EMAIL_ERROR_ALARM_DATA_NOT_FOUND;

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_check_alarm_by_class_id(email_alarm_class_t input_class_id)
{
	EM_DEBUG_FUNC_BEGIN("input_class_id [%d]", input_class_id);
	int err = EMAIL_ERROR_NONE;
	GList *index = NULL;
	email_alarm_data_t *alarm_data = NULL;

	ENTER_CRITICAL_SECTION(alarm_data_lock);
	index = g_list_first(alarm_data_list);
	while(index) {
		alarm_data = index->data;
		if(alarm_data->class_id == input_class_id) {
			EM_DEBUG_LOG("found");
			break;
		}
		index = g_list_next(index);
	}
	LEAVE_CRITICAL_SECTION(alarm_data_lock);

	if(!index)
		err = EMAIL_ERROR_ALARM_DATA_NOT_FOUND;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static int emcore_add_alarm_data_to_alarm_data_list(char *multi_user_name,
													alarm_id_t input_alarm_id,
													email_alarm_class_t input_class_id,
													int input_reference_id,
													time_t input_trigger_at_time,
													int (*input_alarm_callback)(email_alarm_data_t*, void *),
													void *user_data)
{
	EM_DEBUG_FUNC_BEGIN("input_alarm_id [%d] input_class_id[%d] input_reference_id[%d] input_trigger_at_time[%d] input_alarm_callback [%p] user_data[%p]", input_alarm_id, input_class_id, input_reference_id, input_trigger_at_time, input_alarm_callback, user_data);
	int err = EMAIL_ERROR_NONE;
	email_alarm_data_t *alarm_data = NULL;

	if (input_alarm_id == 0 || input_trigger_at_time == 0 || input_alarm_callback == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	alarm_data = em_malloc(sizeof(email_alarm_data_t));

	if(alarm_data == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	alarm_data->alarm_id        = input_alarm_id;
	alarm_data->class_id        = input_class_id;
	alarm_data->reference_id    = input_reference_id;
	alarm_data->trigger_at_time = input_trigger_at_time;
	alarm_data->alarm_callback  = input_alarm_callback;
	alarm_data->user_data       = user_data;
	alarm_data->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

	ENTER_CRITICAL_SECTION(alarm_data_lock);
	alarm_data_list = g_list_append(alarm_data_list, (gpointer)alarm_data);
	LEAVE_CRITICAL_SECTION(alarm_data_lock);

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static int emcore_remove_alarm(email_alarm_data_t *input_alarm_data)
{
	EM_DEBUG_FUNC_BEGIN("input_alarm_data [%p]", input_alarm_data);

	int ret = ALARMMGR_RESULT_SUCCESS;
	int err = EMAIL_ERROR_NONE;

	ENTER_CRITICAL_SECTION(alarm_data_lock);
	EM_DEBUG_LOG("alarm_id [%d]", input_alarm_data->alarm_id);
	if ((ret = alarmmgr_remove_alarm(input_alarm_data->alarm_id)) != ALARMMGR_RESULT_SUCCESS) {
		EM_DEBUG_EXCEPTION ("alarmmgr_remove_alarm failed [%d]", ret);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
	}
	EM_SAFE_FREE(input_alarm_data->multi_user_name);
	LEAVE_CRITICAL_SECTION(alarm_data_lock);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_delete_alram_data_from_alarm_data_list(email_alarm_data_t *input_alarm_data)
{
	EM_DEBUG_FUNC_BEGIN("input_alarm_data [%p]", input_alarm_data);
	int err = EMAIL_ERROR_NONE;

	if (input_alarm_data == NULL ) {
		EM_DEBUG_EXCEPTION ("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emcore_remove_alarm(input_alarm_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION ("emcore_remove_alarm failed [%d]", err);
	}

	ENTER_CRITICAL_SECTION(alarm_data_lock);
	alarm_data_list = g_list_remove_all(alarm_data_list, input_alarm_data);

	if (g_list_length(alarm_data_list) == 0) {
		g_list_free (alarm_data_list);
		alarm_data_list = NULL;
	}
	LEAVE_CRITICAL_SECTION(alarm_data_lock);

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_delete_alram_data_by_reference_id(email_alarm_class_t input_class_id, int input_reference_id)
{
	EM_DEBUG_FUNC_BEGIN("input_class_id[%d] input_reference_id[%d]", input_class_id, input_reference_id);
	int err = EMAIL_ERROR_NONE;
	email_alarm_data_t *alarm_data = NULL;

	if ((err = emcore_get_alarm_data_by_reference_id(input_class_id, input_reference_id, &alarm_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_LOG ("emcore_get_alarm_data_by_reference_id return [%d]", err);
		goto FINISH_OFF;
	}

	if ((err = emcore_delete_alram_data_from_alarm_data_list(alarm_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_LOG ("emcore_delete_alram_data_from_alarm_data_list return [%d]", err);
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_add_alarm(char *multi_user_name,
									time_t input_trigger_at_time,
									email_alarm_class_t input_class_id,
									int input_reference_id,
									int (*input_alarm_callback)(email_alarm_data_t*, void *),
									void *input_user_data)
{
	EM_DEBUG_FUNC_BEGIN("input_trigger_at_time[%d] input_class_id[%d] input_reference_id[%d] "
						"input_alarm_callback[%p] input_user_data[%p]",
						input_trigger_at_time, input_class_id, input_reference_id,
						input_alarm_callback, input_user_data);

	int err = EMAIL_ERROR_NONE;
	int ret = 0;
	time_t alarm_interval = 0;
	time_t current_time = 0;
	alarm_id_t alarm_id = 0;

	time(&current_time);

	alarm_interval = input_trigger_at_time - current_time;

	EM_DEBUG_ALARM_LOG("alarm_interval [%d]", (int)alarm_interval);

	if ((ret = alarmmgr_add_alarm(ALARM_TYPE_VOLATILE,
									alarm_interval,
									ALARM_REPEAT_MODE_ONCE,
									EMAIL_ALARM_DESTINATION,
									&alarm_id)) != ALARMMGR_RESULT_SUCCESS) {
		EM_DEBUG_EXCEPTION("alarmmgr_add_alarm failed [%d]",ret);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if ((err = emcore_add_alarm_data_to_alarm_data_list(multi_user_name,
														alarm_id,
														input_class_id,
														input_reference_id,
														input_trigger_at_time,
														input_alarm_callback,
														input_user_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_add_alarm_data_to_alarm_data_list failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
