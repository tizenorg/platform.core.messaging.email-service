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
/*
 * email-core-tasks.h
 *
 *  Created on: 2012. 11. 5.
 *      Author: kyuho.jo@samsung.com
 */

#ifndef EMAIL_CORE_TASKS_H_
#define EMAIL_CORE_TASKS_H_

#include "email-internal-types.h"

typedef struct
{
	email_task_type_t          task_type;
	void*                    (*task_handler_function)(void*);
	int                      (*task_parameter_encoder)(void*, char**, int*);
	int                      (*task_parameter_decoder)(char*, int, void**);
} email_task_handler_t;

typedef struct
{
	int                       task_id;
	email_task_type_t         task_type;
	email_task_status_type_t  task_status;
	email_task_priority_t     task_priority;
	int                       task_parameter_length;
	char                     *task_parameter;

	/* Not be stored in DB*/
	thread_t                  thread_id;
	int                       active_task_id;
} email_task_t;

#define DECLARE_CONVERTER_FOR_TASK_PARAMETER(TASK_NAME) INTERNAL_FUNC int email_encode_task_parameter_##TASK_NAME(void *input_task_parameter_struct, char **output_byte_stream, int *output_stream_size); \
		INTERNAL_FUNC int email_decode_task_parameter_##TASK_NAME(char *input_byte_stream, int input_stream_size, void **output_task_parameter_struct);

/*-------------------------------------------------------------------------------------------*/
/* to handle _EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT */
typedef struct
{
	int  source_mailbox_id;
	int  target_mailbox_id;
	int  mail_id_count;
	int *mail_id_array;
	char *multi_user_name;
} task_parameter_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT;

DECLARE_CONVERTER_FOR_TASK_PARAMETER(EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT);
INTERNAL_FUNC void* task_handler_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT(void *param);

/*-------------------------------------------------------------------------------------------*/
/* to handle EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX */
typedef struct
{
	int  account_id;
	int  mailbox_id_count;
	int *mailbox_id_array;
	int  on_server;
    char *multi_user_name;
} task_parameter_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX;

DECLARE_CONVERTER_FOR_TASK_PARAMETER(EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX);
INTERNAL_FUNC void* task_handler_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX(void *input_param);
/*-------------------------------------------------------------------------------------------*/
/* to handle EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL */
typedef struct
{
	int  mail_id;
    char *multi_user_name;
} task_parameter_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL;

DECLARE_CONVERTER_FOR_TASK_PARAMETER(EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL);
INTERNAL_FUNC void* task_handler_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL(void *input_param);
/*-------------------------------------------------------------------------------------------*/
/* to handle EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL */
typedef struct
{
    char  *multi_user_name;
	int    mail_id;
	time_t scheduled_time;
} task_parameter_EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL;

DECLARE_CONVERTER_FOR_TASK_PARAMETER(EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL);
INTERNAL_FUNC void* task_handler_EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL(void *input_param);
/*-------------------------------------------------------------------------------------------*/
/* to handle EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE */
typedef struct
{
	int  account_id;
	int  mail_id_count;
	int *mail_id_array;
	email_mail_attribute_type attribute_type;
	int  value_length;
	email_mail_attribute_value_t value;
    char *multi_user_name;
} task_parameter_EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE;

DECLARE_CONVERTER_FOR_TASK_PARAMETER(EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE);
INTERNAL_FUNC void* task_handler_EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE(void *input_param);

#endif /* EMAIL_CORE_TASKS_H_ */
