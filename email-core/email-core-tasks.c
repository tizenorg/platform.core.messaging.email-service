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
#include "email-core-mailbox-sync.h"
#include "email-core-signal.h"
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
	return EMAIL_ERROR_NONE;
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
	return EMAIL_ERROR_NONE;
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
		if((err = emcore_move_mail_to_another_account(task_param->mail_id_array[i], task_param->source_mailbox_id, task_param->target_mailbox_id, task_id)) != EMAIL_ERROR_NONE) {
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
	return EMAIL_ERROR_NONE;
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
	return EMAIL_ERROR_NONE;
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
	return EMAIL_ERROR_NONE;
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
	return EMAIL_ERROR_NONE;
}

INTERNAL_FUNC void* task_handler_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL(void *input_param)
{
	return NULL;
}
/*-------------------------------------------------------------------------------------------*/
