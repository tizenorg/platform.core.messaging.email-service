/*
*  email-service
*
* Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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
 * This file contains the data structures and interfaces needed for application,
 * to interact with email-service.
 * @file		email-api-etc.c
 * @brief		This file contains the data structures and interfaces of functionalities provided by email-service .
 */

#include "email-api.h"
#include "email-types.h"
#include "email-ipc.h"
#include "email-debug-log.h"
#include "email-core-utils.h"
#include "email-core-smtp.h"
#include "email-core-mime.h"
#include "email-convert.h"
#include "email-utilities.h"

EXPORT_API int email_show_user_message(int id, email_action_t action, int error_code)
{
	EM_DEBUG_FUNC_BEGIN("id [%d], action [%d], error_code [%d]", id, action, error_code);
	int err = EMAIL_ERROR_NONE;

	if(id < 0 || action < 0) {
		EM_DEBUG_LOG("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SHOW_USER_MESSAGE);

	/* id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&id, sizeof(int))) {
		EM_DEBUG_LOG("emipc_add_parameter failed  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	/* action */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&action, sizeof(int))) {
		EM_DEBUG_LOG("emipc_add_parameter failed  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	/* error_code */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&error_code, sizeof(int))) {
		EM_DEBUG_LOG("emipc_add_parameter failed  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if(!emipc_execute_proxy_api(hAPI))  {
		EM_DEBUG_LOG("ipcProxy_ExecuteAsyncAPI failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_parse_mime_file(char *eml_file_path, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data, int *output_attachment_count)
{
	EM_DEBUG_FUNC_BEGIN("eml_file_path : [%s], output_mail_data : [%p], output_attachment_data : [%p]", eml_file_path, output_mail_data, output_attachment_data);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(eml_file_path, EMAIL_ERROR_INVALID_PARAM);

	if (!emcore_parse_mime_file_to_mail(eml_file_path, output_mail_data, output_attachment_data, output_attachment_count, &err) || !*output_mail_data)
		EM_DEBUG_EXCEPTION("emcore_parse_mime_file_to_mail failed [%d]", err);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_write_mime_file(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data, int input_attachment_count, char **output_file_path)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data : [%p], input_attachment_data : [%p], input_attachment_count : [%d]", input_mail_data, input_attachment_data, input_attachment_count);

	int err = EMAIL_ERROR_NONE;
	int size = 0;
	char *p_output_file_path = NULL;
	char *mail_data_stream = NULL;
	char *attachment_data_list_stream = NULL;
	HIPC_API hAPI = NULL;

	EM_IF_NULL_RETURN_VALUE(input_mail_data, EMAIL_ERROR_INVALID_PARAM);

	hAPI = emipc_create_email_api(_EMAIL_API_WRITE_MIME_FILE);
	if (!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	mail_data_stream = em_convert_mail_data_to_byte_stream(input_mail_data, &size);
	if (!mail_data_stream) {
		EM_DEBUG_EXCEPTION("em_convert_mail_data_to_byte_stream failed");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if (!emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, mail_data_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_dynamic_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	attachment_data_list_stream = em_convert_attachment_data_to_byte_stream(input_attachment_data, input_attachment_count, &size);
	if (!emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, attachment_data_list_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_dynamic_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (*output_file_path && (size = EM_SAFE_STRLEN(*output_file_path)) > 0) {
		EM_DEBUG_LOG("output_file_path : [%s] size : [%d]", *output_file_path, size);
		size = size + 1;
	} else {
		size = 0;
	}

	if (!emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, *output_file_path, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_dynamic_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}	

	if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
		goto FINISH_OFF;
	}

	size = emipc_get_parameter_length(hAPI, ePARAMETER_OUT, 0);
	if (size > 0) {
		p_output_file_path = (char *)em_malloc(size);
		if (p_output_file_path == NULL) {
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		if ((err = emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, size, p_output_file_path)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_get_parameter failed : [%d]", err);
			goto FINISH_OFF;
		}
	}
	
	*output_file_path = p_output_file_path;	

FINISH_OFF:

	if (hAPI)
		emipc_destroy_email_api(hAPI);

	if (err != EMAIL_ERROR_NONE)
		EM_SAFE_FREE(p_output_file_path);

	EM_DEBUG_FUNC_END("err : [%d]", err);
	return err;
}

EXPORT_API int email_delete_parsed_data(email_mail_data_t *input_mail_data)
{
	EM_DEBUG_FUNC_BEGIN("mail_data : [%p]", input_mail_data);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(input_mail_data, EMAIL_ERROR_INVALID_PARAM);

	if (!emcore_delete_parsed_data(input_mail_data, &err))
		EM_DEBUG_EXCEPTION("emcore_delete_parsed_data failed [%d]", err);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_get_mime_entity(char *mime_path, char **mime_entity)
{
	EM_DEBUG_FUNC_BEGIN("mime_path : [%s]", mime_path);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(mime_path, EMAIL_ERROR_INVALID_PARAM);

	if (!emcore_get_mime_entity(mime_path, mime_entity, &err)) 
		EM_DEBUG_EXCEPTION("emcore_get_mime_entity failed [%d]", err);

	EM_DEBUG_FUNC_END();
	return err;
}


