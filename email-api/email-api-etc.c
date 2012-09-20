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
#include "email-core-mime.h"

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

EXPORT_API int email_open_eml_file(char *eml_file_path, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data, int *output_attachment_count)
{
	EM_DEBUG_FUNC_BEGIN("eml_file_path : [%s], output_mail_data : [%p], output_attachment_data : [%p]", eml_file_path, output_mail_data, output_attachment_data);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(eml_file_path, EMAIL_ERROR_INVALID_PARAM);

	if (!emcore_load_eml_file_to_mail(eml_file_path, output_mail_data, output_attachment_data, output_attachment_count, &err) || !*output_mail_data)
		EM_DEBUG_EXCEPTION("emcore_load_eml_file_to_mail_data failed [%d]", err);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_delete_eml_data(email_mail_data_t *input_mail_data)
{
	EM_DEBUG_FUNC_BEGIN("mail_data : [%p]", input_mail_data);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(input_mail_data, EMAIL_ERROR_INVALID_PARAM);

	if (!emcore_delete_eml_data(input_mail_data, &err))
		EM_DEBUG_EXCEPTION("emcore_load_eml_file_to_mail_data failed [%d]", err);

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
