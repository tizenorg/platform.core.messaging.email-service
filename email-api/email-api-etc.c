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
 * This file contains the data structures and interfaces needed for application,
 * to interact with email-service.
 * @file		email-api-etc.c
 * @brief		This file contains the data structures and interfaces of functionalities provided by email-service .
 */

#include "email-types.h"
#include "email-ipc.h"
#include "email-debug-log.h"
#include "email-core-utils.h"
#include "email-core-smtp.h"
#include "email-core-mime.h"
#include "email-convert.h"
#include "email-utilities.h"
#include "email-core-gmime.h"

EXPORT_API int email_show_user_message(int id, email_action_t action, int error_code)
{
	EM_DEBUG_API_BEGIN("id[%d] action[%d] error_code[%d]", id, action, error_code);
	int err = EMAIL_ERROR_NONE;

	if (id < 0 || action < 0) {
		EM_DEBUG_LOG("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SHOW_USER_MESSAGE);

	/* id */
	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&id, sizeof(int))) {
		EM_DEBUG_LOG("emipc_add_parameter failed  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	/* action */
	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&action, sizeof(int))) {
		EM_DEBUG_LOG("emipc_add_parameter failed  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	/* error_code */
	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&error_code, sizeof(int))) {
		EM_DEBUG_LOG("emipc_add_parameter failed  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if (!emipc_execute_proxy_api(hAPI))  {
		EM_DEBUG_LOG("ipcProxy_ExecuteAsyncAPI failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_parse_mime_file(char *eml_file_path, email_mail_data_t **output_mail_data,
									email_attachment_data_t **output_attachment_data, int *output_attachment_count)
{
	EM_DEBUG_API_BEGIN("eml_file_path[%p] output_mail_data[%p] output_attachment_data[%p]",
						eml_file_path, output_mail_data, output_attachment_data);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(eml_file_path, EMAIL_ERROR_INVALID_PARAM);

	if (!emcore_parse_mime_file_to_mail(eml_file_path, output_mail_data, output_attachment_data,
										output_attachment_count, &err) || !*output_mail_data)
		EM_DEBUG_EXCEPTION("emcore_parse_mime_file_to_mail failed [%d]", err);

	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_write_mime_file(email_mail_data_t *input_mail_data,
									email_attachment_data_t *input_attachment_data,
									int input_attachment_count, char **output_file_path)
{
	EM_DEBUG_API_BEGIN("input_mail_data[%p] input_attachment_data[%p] input_attachment_count[%d]",
						input_mail_data, input_attachment_data, input_attachment_count);

	int err = EMAIL_ERROR_NONE;
	int ret_from_ipc = EMAIL_ERROR_NONE;
	HIPC_API hAPI = NULL;

	EM_IF_NULL_RETURN_VALUE(input_mail_data, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(output_file_path, EMAIL_ERROR_INVALID_PARAM);

	hAPI = emipc_create_email_api(_EMAIL_API_WRITE_MIME_FILE);
	if (!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if (!emipc_execute_proxy_api(hAPI)) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
		goto FINISH_OFF;
	}


	if ((ret_from_ipc = emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_get_parameter failed:[%d]", ret_from_ipc);
		err = ret_from_ipc;
		goto FINISH_OFF;
	}

	if (!emcore_make_rfc822_file(NULL, input_mail_data, input_attachment_data, input_attachment_count, false,
								output_file_path,  &err)) {
		EM_DEBUG_EXCEPTION("emcore_make_rfc822_file failed : [%d]", err);
	}

FINISH_OFF:

	if (hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_delete_parsed_data(email_mail_data_t *input_mail_data)
{
	EM_DEBUG_API_BEGIN("mail_data[%p]", input_mail_data);
	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;

	EM_IF_NULL_RETURN_VALUE(input_mail_data, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        return err;
    }

	if (!emcore_delete_parsed_data(multi_user_name, input_mail_data, &err))
		EM_DEBUG_EXCEPTION("emcore_delete_parsed_data failed [%d]", err);

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_get_mime_entity(char *mime_path, char **mime_entity)
{
	EM_DEBUG_API_BEGIN("mime_path[%p]", mime_path);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(mime_path, EMAIL_ERROR_INVALID_PARAM);

	if (!emcore_get_mime_entity(mime_path, mime_entity, &err))
		EM_DEBUG_EXCEPTION("emcore_get_mime_entity failed [%d]", err);

	EM_DEBUG_API_END();
	return err;
}

EXPORT_API int email_verify_email_address(char *input_email_address)
{
	EM_DEBUG_API_BEGIN("input_email_address[%p]", input_email_address);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(input_email_address, EMAIL_ERROR_INVALID_PARAM);

	if ((err = em_verify_email_address(input_email_address)) != EMAIL_ERROR_NONE)
		EM_DEBUG_EXCEPTION("em_verify_email_address failed [%d]", err);

	EM_DEBUG_API_END("err [%d]", err);
	return err;
}

EXPORT_API int email_convert_mutf7_to_utf8(const char *mutf7_str, char **utf8_str)
{
	EM_DEBUG_API_BEGIN("mutf7_str[%p]", mutf7_str);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(mutf7_str, EMAIL_ERROR_INVALID_PARAM);

	emcore_gmime_init();

	char *tmp_mutf7_str = g_strdup(mutf7_str);
	*utf8_str = emcore_convert_mutf7_to_utf8(tmp_mutf7_str);
	if (!(*utf8_str) || EM_SAFE_STRLEN(*utf8_str) == 0) {
		EM_DEBUG_EXCEPTION("emcore_convert_mutf7_to_utf8 failed");
		err = EMAIL_ERROR_UNKNOWN;
	}
	EM_SAFE_FREE(tmp_mutf7_str);

	emcore_gmime_shutdown();

	EM_DEBUG_API_END("ret[%d]", err);
	return err;
}
EXPORT_API int email_check_privilege(const char *privilege_name)
{

	EM_DEBUG_API_BEGIN("privilege_name[%s]", privilege_name);


	int ret = 0;
	int err = EMAIL_ERROR_NONE;
	if(!privilege_name){
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}


	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_CHECK_PRIVILEGE);
	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)privilege_name, EM_SAFE_STRLEN(privilege_name)+1)){
		EM_DEBUG_EXCEPTION("emipc_add_parameter_check_privilege failed");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {

		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
		goto FINISH_OFF;

	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
//	if(ret == false) {

//		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &err);
//	}

	EM_DEBUG_LOG("err : %d", err);

FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	EM_DEBUG_API_END("err[%d]", err);
	return err;

}


