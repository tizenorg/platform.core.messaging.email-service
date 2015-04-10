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
 * email-tasks.c
 *
 *  Created on: 2012. 11. 5.
 *      Author: kyuho.jo@samsung.com
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "email-debug-log.h"
#include "email-utilities.h"
#include "email-internal-types.h"
#include "email-core-tasks.h"
#include "email-core-task-manager.h"
#include "email-core-mail.h"
#include "email-core-smtp.h"
#include "email-core-mailbox-sync.h"
#include "email-core-signal.h"
#include "email-core-utils.h"
#include "tpl.h"

/*-------------------------------------------------------------------------------------------*/
/* to handle EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT */
#define task_parameter_format_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT "iiiB"

INTERNAL_FUNC int email_encode_task_parameter_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT(void *input_task_parameter_struct, char **output_byte_stream, int *output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN("input_task_parameter_struct [%p] output_byte_stream [%p] output_stream_size [%p]", input_task_parameter_struct, output_byte_stream, output_stream_size);

	int err = EMAIL_ERROR_NONE;
	task_parameter_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT *task_parameter = input_task_parameter_struct;
	tpl_bin tb;
	tpl_node *tn = NULL;
	void  *result_data = NULL;
	size_t result_data_length = 0;

	if (task_parameter == NULL || output_byte_stream == NULL || output_stream_size == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	tn = tpl_map(task_parameter_format_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT
			, &task_parameter->source_mailbox_id
			, &task_parameter->target_mailbox_id
			, &task_parameter->mail_id_count
			, &tb);
	tb.sz   = sizeof(int) * task_parameter->mail_id_count;
	tb.addr = task_parameter->mail_id_array;
	tpl_pack(tn, 0);
	tpl_dump(tn, TPL_MEM, &result_data, &result_data_length);
	tpl_free(tn);

	*output_byte_stream = result_data;
	*output_stream_size = result_data_length;

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int email_decode_task_parameter_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT(char *input_byte_stream, int input_stream_size, void **output_task_parameter_struct)
{
	EM_DEBUG_FUNC_BEGIN("input_byte_stream [%p] input_stream_size [%d] output_task_parameter_struct [%p]", input_byte_stream, input_stream_size, output_task_parameter_struct);
	int err = EMAIL_ERROR_NONE;
	tpl_bin tb;
	tpl_node *tn = NULL;
	task_parameter_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT *task_parameter = NULL;

	if (input_byte_stream == NULL || input_stream_size == 0 || output_task_parameter_struct == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	task_parameter = em_malloc(sizeof(task_parameter_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT));

	if(task_parameter == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	tn = tpl_map(task_parameter_format_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT
				, &task_parameter->source_mailbox_id
				, &task_parameter->target_mailbox_id
				, &task_parameter->mail_id_count
				, &tb);
	tpl_load(tn, TPL_MEM, input_byte_stream, input_stream_size);
	tpl_unpack(tn, 0);
	if(task_parameter->mail_id_count <= 0 || tb.addr == NULL) {
		EM_DEBUG_LOG("No mail id list. mail_id_count[%d] addr[%p]", task_parameter->mail_id_count, tb.addr);
	}
	else {
		task_parameter->mail_id_array = tb.addr;
	}

	*output_task_parameter_struct = task_parameter;

FINISH_OFF:

	if(tn)
		tpl_free(tn);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC void* task_handler_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT(void *input_param)
{
	EM_DEBUG_FUNC_BEGIN("input_param [%p]", input_param);
	int err = EMAIL_ERROR_NONE;
	int err_for_signal = EMAIL_ERROR_NONE;
	int i = 0;
	int task_id = THREAD_SELF();
	task_parameter_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT *task_param = input_param;

	for(i = 0; i < task_param->mail_id_count; i++) {
		if((err = emcore_move_mail_to_another_account(task_param->multi_user_name, task_param->mail_id_array[i], task_param->source_mailbox_id, task_param->target_mailbox_id, task_id)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_move_mail_to_another_account failed [%d]", err);
			goto FINISH_OFF;
		}

		/* Send progress signal */
		if((err_for_signal = emcore_send_task_status_signal(EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT, task_id, EMAIL_TASK_STATUS_IN_PROGRESS, i, task_param->mail_id_count)) != EMAIL_ERROR_NONE)
			EM_DEBUG_LOG("emcore_send_task_status_signal failed [%d]", err_for_signal);
	}

FINISH_OFF:
	/* Free task parameter */
	EM_SAFE_FREE(task_param->mail_id_array);
	EM_SAFE_FREE(task_param);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return NULL;
}
/*-------------------------------------------------------------------------------------------*/
/* to handle EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX */
#define task_parameter_format_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX "iiBi"

INTERNAL_FUNC int email_encode_task_parameter_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX(void *input_task_parameter_struct, char **output_byte_stream, int *output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN("input_task_parameter_struct [%p] output_byte_stream [%p] output_stream_size [%p]", input_task_parameter_struct, output_byte_stream, output_stream_size);

	int err = EMAIL_ERROR_NONE;
	task_parameter_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX *task_parameter = input_task_parameter_struct;
	tpl_bin tb;
	tpl_node *tn = NULL;
	void  *result_data = NULL;
	size_t result_data_length = 0;

	if (task_parameter == NULL || output_byte_stream == NULL || output_stream_size == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	tn = tpl_map(task_parameter_format_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX
			, &task_parameter->account_id
			, &task_parameter->mailbox_id_count
			, &tb
			, &task_parameter->on_server);
	tb.sz   = sizeof(int) * task_parameter->mailbox_id_count;
	tb.addr = task_parameter->mailbox_id_array;
	tpl_pack(tn, 0);
	tpl_dump(tn, TPL_MEM, &result_data, &result_data_length);
	tpl_free(tn);

	*output_byte_stream = result_data;
	*output_stream_size = result_data_length;

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int email_decode_task_parameter_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX(char *input_byte_stream, int input_stream_size, void **output_task_parameter_struct)
{
	EM_DEBUG_FUNC_BEGIN("input_byte_stream [%p] input_stream_size [%d] output_task_parameter_struct [%p]", input_byte_stream, input_stream_size, output_task_parameter_struct);
	int err = EMAIL_ERROR_NONE;
	tpl_bin tb;
	tpl_node *tn = NULL;
	task_parameter_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX *task_parameter = NULL;

	if (input_byte_stream == NULL || input_stream_size == 0 || output_task_parameter_struct == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	task_parameter = em_malloc(sizeof(task_parameter_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX));

	if(task_parameter == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	tn = tpl_map(task_parameter_format_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX
				, &task_parameter->account_id
				, &task_parameter->mailbox_id_count
				, &tb
				, &task_parameter->on_server);
	tpl_load(tn, TPL_MEM, input_byte_stream, input_stream_size);
	tpl_unpack(tn, 0);
	if(task_parameter->mailbox_id_count <= 0 || tb.addr == NULL) {
		EM_DEBUG_LOG("No mail id list. mail_id_count[%d] addr[%p]", task_parameter->mailbox_id_count, tb.addr);
	}
	else {
		task_parameter->mailbox_id_array = tb.addr;
	}

	*output_task_parameter_struct = task_parameter;

FINISH_OFF:

	if(tn)
		tpl_free(tn);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC void* task_handler_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX(void *input_param)
{
	return NULL;
}
/*-------------------------------------------------------------------------------------------*/
/* to handle EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL */
#define task_parameter_format_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL "i"

INTERNAL_FUNC int email_encode_task_parameter_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL(void *input_task_parameter_struct, char **output_byte_stream, int *output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN("input_task_parameter_struct [%p] output_byte_stream [%p] output_stream_size [%p]", input_task_parameter_struct, output_byte_stream, output_stream_size);

	int err = EMAIL_ERROR_NONE;
	task_parameter_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL *task_parameter = input_task_parameter_struct;
	tpl_node *tn = NULL;
	void  *result_data = NULL;
	size_t result_data_length = 0;

	if (task_parameter == NULL || output_byte_stream == NULL || output_stream_size == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	tn = tpl_map(task_parameter_format_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL
			, &task_parameter->mail_id);
	tpl_pack(tn, 0);
	tpl_dump(tn, TPL_MEM, &result_data, &result_data_length);
	tpl_free(tn);

	*output_byte_stream = result_data;
	*output_stream_size = result_data_length;

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int email_decode_task_parameter_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL(char *input_byte_stream, int input_stream_size, void **output_task_parameter_struct)
{
	EM_DEBUG_FUNC_BEGIN("input_byte_stream [%p] input_stream_size [%d] output_task_parameter_struct [%p]", input_byte_stream, input_stream_size, output_task_parameter_struct);
	int err = EMAIL_ERROR_NONE;
	tpl_node *tn = NULL;
	task_parameter_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL *task_parameter = NULL;

	if (input_byte_stream == NULL || input_stream_size == 0 || output_task_parameter_struct == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	task_parameter = em_malloc(sizeof(task_parameter_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL));

	if(task_parameter == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	tn = tpl_map(task_parameter_format_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL
				, &task_parameter->mail_id);
	tpl_load(tn, TPL_MEM, input_byte_stream, input_stream_size);
	tpl_unpack(tn, 0);

	*output_task_parameter_struct = task_parameter;

FINISH_OFF:

	if(tn)
		tpl_free(tn);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC void* task_handler_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL(void *input_param)
{
	return NULL;
}
/*-------------------------------------------------------------------------------------------*/
/* to handle EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL */
#define task_parameter_format_EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL "ii"

INTERNAL_FUNC int email_encode_task_parameter_EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL(void *input_task_parameter_struct, char **output_byte_stream, int *output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN("input_task_parameter_struct [%p] output_byte_stream [%p] output_stream_size [%p]", input_task_parameter_struct, output_byte_stream, output_stream_size);

	int err = EMAIL_ERROR_NONE;
	task_parameter_EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL *task_parameter = input_task_parameter_struct;
	tpl_node *tn = NULL;
	void  *result_data = NULL;
	size_t result_data_length = 0;

	if (task_parameter == NULL || output_byte_stream == NULL || output_stream_size == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	tn = tpl_map(task_parameter_format_EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL
			, &task_parameter->mail_id, &task_parameter->scheduled_time);
	tpl_pack(tn, 0);
	tpl_dump(tn, TPL_MEM, &result_data, &result_data_length);
	tpl_free(tn);

	*output_byte_stream = result_data;
	*output_stream_size = result_data_length;

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int email_decode_task_parameter_EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL(char *input_byte_stream, int input_stream_size, void **output_task_parameter_struct)
{
	EM_DEBUG_FUNC_BEGIN("input_byte_stream [%p] input_stream_size [%d] output_task_parameter_struct [%p]", input_byte_stream, input_stream_size, output_task_parameter_struct);
	int err = EMAIL_ERROR_NONE;
	tpl_node *tn = NULL;
	task_parameter_EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL *task_parameter = NULL;

	if (input_byte_stream == NULL || input_stream_size == 0 || output_task_parameter_struct == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	task_parameter = em_malloc(sizeof(task_parameter_EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL));

	if(task_parameter == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	tn = tpl_map(task_parameter_format_EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL
				, &task_parameter->mail_id, &task_parameter->scheduled_time);
	tpl_load(tn, TPL_MEM, input_byte_stream, input_stream_size);
	tpl_unpack(tn, 0);

	*output_task_parameter_struct = task_parameter;

FINISH_OFF:

	if(tn)
		tpl_free(tn);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC void* task_handler_EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL(void *input_param)
{
	EM_DEBUG_FUNC_BEGIN("input_param [%p]", input_param);
	int err = EMAIL_ERROR_NONE;
	task_parameter_EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL *task_param = input_param;

	if((err = emcore_schedule_sending_mail(task_param->multi_user_name, task_param->mail_id, task_param->scheduled_time)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_schedule_sending_mail [%d]", err);
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_SAFE_FREE(task_param);
	emcore_close_smtp_stream_list ();
	EM_DEBUG_FUNC_END("err [%d]", err);
	return (void*)err;
}
/*-------------------------------------------------------------------------------------------*/
/* to handle EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE */
#define task_parameter_format_EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE "iiBiiB"

INTERNAL_FUNC int email_encode_task_parameter_EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE(void *input_task_parameter_struct, char **output_byte_stream, int *output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN("input_task_parameter_struct [%p] output_byte_stream [%p] output_stream_size [%p]", input_task_parameter_struct, output_byte_stream, output_stream_size);

	int err = EMAIL_ERROR_NONE;
	task_parameter_EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE *task_parameter = input_task_parameter_struct;
	email_mail_attribute_value_type attribute_value_type = EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_NONE;
	tpl_bin tb_mail_id_array;
	tpl_bin tb_value;
	tpl_node *tn = NULL;
	void  *result_data = NULL;
	size_t result_data_length = 0;

	if (task_parameter == NULL || output_byte_stream == NULL || output_stream_size == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emcore_get_mail_attribute_value_type(task_parameter->attribute_type, &attribute_value_type)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_attribute_value_type failed [%d]", err);
		goto FINISH_OFF;
	}

	tn = tpl_map(task_parameter_format_EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE
				, &task_parameter->account_id
				, &task_parameter->mail_id_count
				, &tb_mail_id_array
				, &task_parameter->attribute_type
				, &task_parameter->value_length
				, &tb_value);

	tb_mail_id_array.sz   = sizeof(int) * task_parameter->mail_id_count;
	tb_mail_id_array.addr = task_parameter->mail_id_array;

	switch(attribute_value_type) {
		case EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER :
			task_parameter->value_length = 4;
			tb_value.sz                  = task_parameter->value_length;
			tb_value.addr                = &task_parameter->value.integer_type_value;
			break;
		case EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING :
			task_parameter->value_length = EM_SAFE_STRLEN(task_parameter->value.string_type_value);
			tb_value.sz                  = task_parameter->value_length;
			tb_value.addr                = task_parameter->value.string_type_value;
			break;
		case EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_TIME :
			task_parameter->value_length = 4;
			tb_value.sz                  = task_parameter->value_length;
			tb_value.addr                = &task_parameter->value.datetime_type_value;
			break;
		case EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_NONE :
		default :
			EM_DEBUG_EXCEPTION("invalid attribute value type [%d]", attribute_value_type);
			err = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}

	tpl_pack(tn, 0);
	tpl_dump(tn, TPL_MEM, &result_data, &result_data_length);

	*output_byte_stream = result_data;
	*output_stream_size = result_data_length;

FINISH_OFF:

	if(tn)
		tpl_free(tn);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return EMAIL_ERROR_NONE;
}

INTERNAL_FUNC int email_decode_task_parameter_EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE(char *input_byte_stream, int input_stream_size, void **output_task_parameter_struct)
{
	EM_DEBUG_FUNC_BEGIN("input_byte_stream [%p] input_stream_size [%d] output_task_parameter_struct [%p]", input_byte_stream, input_stream_size, output_task_parameter_struct);
	int err = EMAIL_ERROR_NONE;
	tpl_node *tn = NULL;
	tpl_bin tb_mail_id_array;
	tpl_bin tb_value;
	task_parameter_EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE *task_parameter = NULL;
	email_mail_attribute_value_type attribute_value_type = EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_NONE;

	if (input_byte_stream == NULL || input_stream_size == 0 || output_task_parameter_struct == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	task_parameter = em_malloc(sizeof(task_parameter_EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE));

	if(task_parameter == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	tn = tpl_map(task_parameter_format_EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE
			, &task_parameter->account_id
			, &task_parameter->mail_id_count
			, &tb_mail_id_array
			, &task_parameter->attribute_type
			, &task_parameter->value_length
			, &tb_value);

	tpl_load(tn, TPL_MEM, input_byte_stream, input_stream_size);
	tpl_unpack(tn, 0);

	if(task_parameter->mail_id_count <= 0 || tb_mail_id_array.addr == NULL) {
		EM_DEBUG_LOG("No mail id list. mail_id_count[%d] addr[%p]", task_parameter->mail_id_count, tb_mail_id_array.addr);
	}
	else {
		task_parameter->mail_id_array = tb_mail_id_array.addr;
	}

	if(tb_value.addr) {
		if ((err = emcore_get_mail_attribute_value_type(task_parameter->attribute_type, &attribute_value_type)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_mail_attribute_value_type failed [%d]", err);
			goto FINISH_OFF;
		}

		switch(attribute_value_type) {
			case EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER :
				memcpy(&task_parameter->value.integer_type_value, tb_value.addr, tb_value.sz);
				break;
			case EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING :
				task_parameter->value.string_type_value = em_malloc(tb_value.sz + 1);
				if(task_parameter->value.string_type_value == NULL) {
					EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
					err = EMAIL_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}
				memcpy(&task_parameter->value.string_type_value, tb_value.addr, tb_value.sz);
				break;
			case EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_TIME :
				memcpy(&task_parameter->value.datetime_type_value, tb_value.addr, tb_value.sz);
				break;
			case EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_NONE :
			default :
				EM_DEBUG_EXCEPTION("invalid attribute value type [%d]", attribute_value_type);
				err = EMAIL_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
		}
	}

	*output_task_parameter_struct = task_parameter;

FINISH_OFF:

	if(tn)
		tpl_free(tn);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return EMAIL_ERROR_NONE;
}

INTERNAL_FUNC void* task_handler_EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE(void *input_param)
{
	EM_DEBUG_FUNC_BEGIN("input_param [%p]", input_param);
	if(!input_param) { /*prevent 43681*/
		EM_DEBUG_EXCEPTION("NULL parameter");		
		return NULL;
	}

	int err = EMAIL_ERROR_NONE;
	char *field_name = NULL;
	task_parameter_EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE *task_param = input_param;
	email_mail_attribute_value_type attribute_value_type = EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_NONE; /*prevent 43700*/

	if((field_name = emcore_get_mail_field_name_by_attribute_type(task_param->attribute_type)) == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if((err = emcore_get_mail_attribute_value_type(task_param->attribute_type, &attribute_value_type)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_attribute_value_type failed [%d]", err);
		goto FINISH_OFF;
	}

	switch(attribute_value_type) {
		case EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER :
			if(!emstorage_set_field_of_mails_with_integer_value(task_param->multi_user_name, task_param->account_id, task_param->mail_id_array, task_param->mail_id_count, field_name, task_param->value.integer_type_value, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err);
				goto FINISH_OFF;
			}
			break;
		case EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING :
			err = EMAIL_ERROR_NOT_SUPPORTED;
			EM_DEBUG_LOG("EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING is not supported");
			break;
		case EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_TIME :
			if(!emstorage_set_field_of_mails_with_integer_value(task_param->multi_user_name, task_param->account_id, task_param->mail_id_array, task_param->mail_id_count, field_name, task_param->value.datetime_type_value, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err);
				goto FINISH_OFF;
			}
			break;
		case EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_NONE :
		default :
			EM_DEBUG_LOG("EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_NONE or default");
			err = EMAIL_ERROR_INVALID_PARAM;
			break;
	}

FINISH_OFF:

	EM_SAFE_FREE(task_param->multi_user_name);
	EM_SAFE_FREE(task_param->mail_id_array);
	if (attribute_value_type == EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING) {
		EM_SAFE_FREE(task_param->value.string_type_value);
	}

	EM_SAFE_FREE (task_param);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return (void*)err;
}
/*-------------------------------------------------------------------------------------------*/
