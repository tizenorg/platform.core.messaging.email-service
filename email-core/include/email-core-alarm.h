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
 * File :  email-core-alarm.h
 * Desc :  Account Management
 * * Auth :
 * * History :
 *    2013.01.28  :  created
 *****************************************************************************/
#ifndef _EMAIL_CORE_ALARM_H_
#define _EMAIL_CORE_ALARM_H_

#include <email-internal-types.h>
#include <alarm.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define EMAIL_ALARM_DESTINATION "email-service-0"

typedef struct _email_alarm_data_t {
	alarm_id_t                  alarm_id;
	email_alarm_class_t           class_id;
	int                         reference_id;
	time_t                      trigger_at_time;
	int                       (*alarm_callback)(int, void *);
	void                       *user_data;
} email_alarm_data_t;


INTERNAL_FUNC int emcore_add_alarm(time_t input_trigger_at_time, email_alarm_class_t input_class_id, int input_reference_id, int (*input_alarm_callback)(int, void *), void *input_user_data);

INTERNAL_FUNC int emcore_delete_alram_data_from_alarm_data_list(email_alarm_data_t *input_alarm_data);
INTERNAL_FUNC int emcore_delete_alram_data_by_reference_id(email_alarm_class_t input_class_id, int input_reference_id);

INTERNAL_FUNC int emcore_get_alarm_data_by_alarm_id(alarm_id_t input_alarm_id, email_alarm_data_t **output_alarm_data);
INTERNAL_FUNC int emcore_get_alarm_data_by_reference_id(email_alarm_class_t input_class_id, int input_reference_id, email_alarm_data_t **output_alarm_data);
INTERNAL_FUNC int emcore_check_alarm_by_class_id(email_alarm_class_t input_class_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*_EMAIL_CORE_ALARM_H_*/

