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
 * @file		email_api_mailbox.c
 * @brief 		This file contains the data structures and interfaces of mailbox related Functionality provided by
 *			email-service .
 */

#include "string.h"
#include "email-convert.h"
#include "email-storage.h"
#include "email-core-utils.h"
#include "email-core-gmime.h"
#include "email-core-signal.h"
#include "email-utilities.h"
#include "email-ipc.h"

/* API - Create a mailbox */

EXPORT_API int email_add_mailbox(email_mailbox_t* new_mailbox, int on_server, int *handle)
{
	EM_DEBUG_API_BEGIN();

	int size = 0;
	int err = EMAIL_ERROR_NONE;
	char* local_mailbox_stream = NULL;
    char *multi_user_name = NULL;
	email_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	ASNotiData as_noti_data;

	EM_IF_NULL_RETURN_VALUE(new_mailbox, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	/*  check account bind type and branch off  */
	if (em_get_account_server_type_by_account_id(multi_user_name, new_mailbox->account_id, &account_server_type, false, &err) == false) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if (account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC && on_server) {
		int as_handle;

		if (em_get_handle_for_activesync(&as_handle, &err) == false) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		/*  noti to active sync */
		as_noti_data.add_mailbox.handle          = as_handle;
		as_noti_data.add_mailbox.account_id      = new_mailbox->account_id;
		as_noti_data.add_mailbox.mailbox_alias   = new_mailbox->alias;
		as_noti_data.add_mailbox.mailbox_path    = new_mailbox->mailbox_name;
		as_noti_data.add_mailbox.eas_data        = new_mailbox->eas_data;
		as_noti_data.add_mailbox.eas_data_length = new_mailbox->eas_data_length;
		as_noti_data.add_mailbox.multi_user_name = multi_user_name;

		if (em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_ADD_MAILBOX, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if (handle)
			*handle = as_handle;
	} else {
		hAPI = emipc_create_email_api(_EMAIL_API_ADD_MAILBOX);

		EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

		local_mailbox_stream = em_convert_mailbox_to_byte_stream(new_mailbox, &size);

		EM_PROXY_IF_NULL_RETURN_VALUE(local_mailbox_stream, hAPI, EMAIL_ERROR_NULL_VALUE);

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, local_mailbox_stream, size)) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			EM_SAFE_FREE(local_mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &on_server, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

		if (handle) {
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
			EM_DEBUG_LOG(" >>>>> Handle [%d]", *handle);
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 2, sizeof(int), &(new_mailbox->mailbox_id));
		EM_DEBUG_LOG(" >>>>> new_mailbox->mailbox_id [%d]", new_mailbox->mailbox_id);

		emipc_destroy_email_api(hAPI);
		hAPI = NULL;
	}

FINISH_OFF:
	EM_SAFE_FREE(local_mailbox_stream);
    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_rename_mailbox(int input_mailbox_id, char *input_mailbox_name, char *input_mailbox_alias, int input_on_server, int *output_handle)
{
	EM_DEBUG_API_BEGIN("input_mailbox_id[%d] input_mailbox_name[%p] input_mailbox_alias[%p] input_on_server[%d] output_handle[%p]", input_mailbox_id, input_mailbox_name, input_mailbox_alias, input_on_server, output_handle);

	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	email_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	ASNotiData as_noti_data;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;

	EM_IF_NULL_RETURN_VALUE(input_mailbox_id,    EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_mailbox_name,  EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_mailbox_alias, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &mailbox_tbl)) != EMAIL_ERROR_NONE || !mailbox_tbl) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", err);
		goto FINISH_OFF;
	}

	/*  check account bind type and branch off  */
	if (em_get_account_server_type_by_account_id(multi_user_name, mailbox_tbl->account_id, &account_server_type, false, &err) == false) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if (account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC && input_on_server) {
		int as_handle;

		if (em_get_handle_for_activesync(&as_handle, &err) == false) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		/*  noti to active sync */
		as_noti_data.rename_mailbox.handle          = as_handle;
		as_noti_data.rename_mailbox.account_id      = mailbox_tbl->account_id;
		as_noti_data.rename_mailbox.mailbox_id      = input_mailbox_id;
		as_noti_data.rename_mailbox.mailbox_name    = input_mailbox_name;
		as_noti_data.rename_mailbox.mailbox_alias   = input_mailbox_alias;
		as_noti_data.rename_mailbox.eas_data        = NULL;
		as_noti_data.rename_mailbox.eas_data_length = 0;
		as_noti_data.rename_mailbox.multi_user_name = multi_user_name;

		if (em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_RENAME_MAILBOX, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if (output_handle)
			*output_handle = as_handle;
	} else {
		hAPI = emipc_create_email_api(_EMAIL_API_RENAME_MAILBOX);

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_mailbox_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION(" emipc_add_parameter for input_mailbox_id failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, input_mailbox_name, EM_SAFE_STRLEN(input_mailbox_name)+1)) {
			EM_DEBUG_EXCEPTION(" emipc_add_parameter for input_mailbox_path failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, input_mailbox_alias, EM_SAFE_STRLEN(input_mailbox_alias)+1)) {
			EM_DEBUG_EXCEPTION(" emipc_add_parameter for input_mailbox_alias failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_on_server, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		if (output_handle)
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), output_handle);

		EM_DEBUG_LOG("error VALUE [%d]", err);
		emipc_destroy_email_api(hAPI);
	}
FINISH_OFF:
	if (mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

    EM_SAFE_FREE(multi_user_name);
	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_rename_mailbox_ex(int input_mailbox_id, char *input_mailbox_name, char *input_mailbox_alias, void *input_eas_data, int input_eas_data_length, int input_on_server, int *output_handle)
{
	EM_DEBUG_API_BEGIN("input_mailbox_id[%d] input_mailbox_name[%p] input_mailbox_alias[%p] input_eas_data[%p] input_eas_data_length[%d] input_on_server[%d] output_handle[%p]", input_mailbox_id, input_mailbox_name, input_mailbox_alias, input_eas_data, input_eas_data_length, input_on_server, output_handle);

	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	email_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	ASNotiData as_noti_data;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;

	EM_IF_NULL_RETURN_VALUE(input_mailbox_id,    EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_mailbox_name,  EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_mailbox_alias, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &mailbox_tbl)) != EMAIL_ERROR_NONE || !mailbox_tbl) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", err);
		goto FINISH_OFF;
	}

	/*  check account bind type and branch off  */
	if (em_get_account_server_type_by_account_id(multi_user_name, mailbox_tbl->account_id, &account_server_type, false, &err) == false) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if (account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC && input_on_server) {
		int as_handle;

		if (em_get_handle_for_activesync(&as_handle, &err) == false) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		/*  noti to active sync */
		as_noti_data.rename_mailbox.handle          = as_handle;
		as_noti_data.rename_mailbox.account_id      = mailbox_tbl->account_id;
		as_noti_data.rename_mailbox.mailbox_id      = input_mailbox_id;
		as_noti_data.rename_mailbox.mailbox_name    = input_mailbox_name;
		as_noti_data.rename_mailbox.mailbox_alias   = input_mailbox_alias;
		as_noti_data.rename_mailbox.eas_data        = input_eas_data;
		as_noti_data.rename_mailbox.eas_data_length = input_eas_data_length;
        as_noti_data.rename_mailbox.multi_user_name = multi_user_name;

		if (em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_RENAME_MAILBOX, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if (output_handle)
			*output_handle = as_handle;
	} else {
		hAPI = emipc_create_email_api(_EMAIL_API_RENAME_MAILBOX_EX);

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_mailbox_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION(" emipc_add_parameter for input_mailbox_id failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, input_mailbox_name, EM_SAFE_STRLEN(input_mailbox_name)+1)) {
			EM_DEBUG_EXCEPTION(" emipc_add_parameter for input_mailbox_path failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, input_mailbox_alias, EM_SAFE_STRLEN(input_mailbox_alias)+1)) {
			EM_DEBUG_EXCEPTION(" emipc_add_parameter for input_mailbox_alias failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, input_eas_data, input_eas_data_length)) {
			EM_DEBUG_EXCEPTION(" emipc_add_parameter for input_eas_data failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_on_server, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		if (output_handle)
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), output_handle);

		EM_DEBUG_LOG("error VALUE [%d]", err);
		emipc_destroy_email_api(hAPI);
	}
FINISH_OFF:
	if (mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

    EM_SAFE_FREE(multi_user_name);
	EM_DEBUG_API_END("err[%d]", err);
	return err;
}


/* API - Delete a mailbox */

EXPORT_API int email_delete_mailbox(int input_mailbox_id, int input_on_server, int *output_handle)
{
	EM_DEBUG_API_BEGIN("input_mailbox_id[%d] input_on_server[%d] output_handle[%p]", input_mailbox_id, input_on_server, output_handle);

	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	email_account_server_t account_server_type;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;
	HIPC_API hAPI = NULL;
	ASNotiData as_noti_data;

	EM_IF_NULL_RETURN_VALUE(input_mailbox_id, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &mailbox_tbl)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed[%d]", err);
		goto FINISH_OFF;
	}

	/*  check account bind type and branch off  */
	if (em_get_account_server_type_by_account_id(multi_user_name, mailbox_tbl->account_id, &account_server_type, false, &err) == false) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if (account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC && input_on_server) {
		int as_handle;

		if (em_get_handle_for_activesync(&as_handle, &err) == false) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		/*  noti to active sync */
		as_noti_data.delete_mailbox.handle          = as_handle;
		as_noti_data.delete_mailbox.account_id      = mailbox_tbl->account_id;
		as_noti_data.delete_mailbox.mailbox_id      = input_mailbox_id;
        as_noti_data.delete_mailbox.multi_user_name = multi_user_name;

		if (em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_DELETE_MAILBOX, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if (output_handle)
			*output_handle = as_handle;
	} else {
		hAPI = emipc_create_email_api(_EMAIL_API_DELETE_MAILBOX);

		EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_mailbox_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_on_server, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		EM_DEBUG_LOG("error VALUE [%d]", err);

		if (input_on_server) {
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), output_handle);
			EM_DEBUG_LOG("output_handle [%d]", output_handle);
		}

		emipc_destroy_email_api(hAPI);
	}

FINISH_OFF:
	if (mailbox_tbl) {
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);
	}

    EM_SAFE_FREE(multi_user_name);
	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_delete_mailbox_ex(int input_account_id, int *input_mailbox_id_array,
										int input_mailbox_id_count, int input_on_server,
										int *output_handle)
{
	EM_DEBUG_API_BEGIN("input_account_id[%d] input_mailbox_id_array[%p] "
						"input_mailbox_id_count[%d] input_on_server[%d] "
						"output_handle[%p]", input_mailbox_id_array, input_mailbox_id_array,
						input_mailbox_id_count, input_on_server, output_handle);

	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	email_account_server_t account_server_type;
	task_parameter_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX task_parameter;

	if (input_account_id == 0 || input_mailbox_id_count <= 0 || input_mailbox_id_array == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	/*  check account bind type and branch off  */
	if (em_get_account_server_type_by_account_id(multi_user_name,
													input_account_id,
													&account_server_type,
													false,
													&err) == false) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if (account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC && input_on_server) {
		int as_handle;
		ASNotiData as_noti_data;

		if (em_get_handle_for_activesync(&as_handle, &err) == false) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		/*  noti to active sync */
		as_noti_data.delete_mailbox_ex.handle            = as_handle;
		as_noti_data.delete_mailbox_ex.account_id        = input_account_id;
		as_noti_data.delete_mailbox_ex.mailbox_id_array  = input_mailbox_id_array;
		as_noti_data.delete_mailbox_ex.mailbox_id_count  = input_mailbox_id_count;
		as_noti_data.delete_mailbox_ex.on_server         = input_on_server;
		as_noti_data.delete_mailbox_ex.multi_user_name   = EM_SAFE_STRDUP(multi_user_name);

		if (em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_DELETE_MAILBOX_EX, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if (output_handle)
			*output_handle = as_handle;
	} else {
		task_parameter.account_id        = input_account_id;
		task_parameter.mailbox_id_array  = input_mailbox_id_array;
		task_parameter.mailbox_id_count  = input_mailbox_id_count;
		task_parameter.on_server         = input_on_server;

		if ((err = emipc_execute_proxy_task(EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX,
											&task_parameter)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("execute_proxy_task failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:

	EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_set_mailbox_type(int input_mailbox_id, email_mailbox_type_e input_mailbox_type)
{
	EM_DEBUG_API_BEGIN("input_mailbox_id[%d] input_mailbox_type[%d]", input_mailbox_id, input_mailbox_type);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(input_mailbox_id, EMAIL_ERROR_INVALID_PARAM);

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SET_MAILBOX_TYPE);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_mailbox_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_mailbox_type, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_set_local_mailbox(int input_mailbox_id, int input_is_local_mailbox)
{
	EM_DEBUG_API_BEGIN("input_mailbox_id[%d] input_is_local_mailbox[%d]", input_mailbox_id, input_is_local_mailbox);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(input_mailbox_id, EMAIL_ERROR_INVALID_PARAM);

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SET_LOCAL_MAILBOX);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_mailbox_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_is_local_mailbox, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_get_sync_mailbox_list(int account_id, email_mailbox_t** mailbox_list, int* count)
{
	EM_DEBUG_API_BEGIN();
	int mailbox_count = 0;
	int err = EMAIL_ERROR_NONE ;
	int i = 0;
    char *multi_user_name = NULL;
	emstorage_mailbox_tbl_t* mailbox_tbl_list = NULL;

	EM_IF_NULL_RETURN_VALUE(mailbox_list, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(account_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(count, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (!emstorage_get_mailbox_list(multi_user_name, account_id, 0, EMAIL_MAILBOX_SORT_BY_NAME_ASC, &mailbox_count, &mailbox_tbl_list, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox failed [%d]", err);
		goto FINISH_OFF;
	} else
		err = EMAIL_ERROR_NONE;

	if (mailbox_count > 0)  {
		if (!(*mailbox_list = em_malloc(sizeof(email_mailbox_t) * mailbox_count)))  {
			EM_DEBUG_EXCEPTION("malloc failed...");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		for (i = 0; i < mailbox_count; i++)
			em_convert_mailbox_tbl_to_mailbox(mailbox_tbl_list + i, (*mailbox_list) + i);
	} else
		*mailbox_list = NULL;

	*count = mailbox_count;

FINISH_OFF:

	if (mailbox_tbl_list != NULL)
		emstorage_free_mailbox(&mailbox_tbl_list, mailbox_count, NULL);

    EM_SAFE_FREE(multi_user_name);
	EM_DEBUG_API_END("err[%d]", err);
	return err;
}


EXPORT_API int email_get_mailbox_list(int account_id, int mailbox_sync_type, email_mailbox_t** mailbox_list, int* count)
{
	EM_DEBUG_API_BEGIN();

	int mailbox_count = 0;
	emstorage_mailbox_tbl_t* mailbox_tbl_list = NULL;
	int err = EMAIL_ERROR_NONE;
	int i;
    char *multi_user_name = NULL;

	EM_IF_NULL_RETURN_VALUE(mailbox_list, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(account_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(count, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (!emstorage_get_mailbox_list(multi_user_name, account_id, mailbox_sync_type, EMAIL_MAILBOX_SORT_BY_NAME_ASC, &mailbox_count, &mailbox_tbl_list, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox failed [%d]", err);
		goto FINISH_OFF;
	} else
		err = EMAIL_ERROR_NONE;

	if (mailbox_count > 0)  {
		if (!(*mailbox_list = em_malloc(sizeof(email_mailbox_t) * mailbox_count)))  {
			EM_DEBUG_EXCEPTION("malloc failed for mailbox_list");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		for (i = 0; i < mailbox_count; i++)
			em_convert_mailbox_tbl_to_mailbox(mailbox_tbl_list + i, (*mailbox_list) + i);
	} else
		*mailbox_list = NULL;

	*count = mailbox_count;

FINISH_OFF:

	if (mailbox_tbl_list != NULL)
		emstorage_free_mailbox(&mailbox_tbl_list, mailbox_count, NULL);

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END("err [%d]", err);
	return err;
}

EXPORT_API int email_get_mailbox_list_ex(int account_id, int mailbox_sync_type, int with_count, email_mailbox_t** mailbox_list, int* count)
{
	EM_DEBUG_FUNC_BEGIN();

	int mailbox_count = 0;
	emstorage_mailbox_tbl_t* mailbox_tbl_list = NULL;
	int err = EMAIL_ERROR_NONE;
	int i;
    char *multi_user_name = NULL;

	EM_IF_NULL_RETURN_VALUE(mailbox_list, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(account_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(count, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	emcore_gmime_init();
	if (!emstorage_get_mailbox_list_ex(multi_user_name, account_id, mailbox_sync_type, with_count, &mailbox_count, &mailbox_tbl_list, true, &err))  {
		emcore_gmime_shutdown();
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_list_ex failed [%d]", err);
		goto FINISH_OFF;
	} else
		err = EMAIL_ERROR_NONE;

	emcore_gmime_shutdown();

	if (mailbox_count > 0)  {
		if (!(*mailbox_list = em_malloc(sizeof(email_mailbox_t) * mailbox_count)))  {
			EM_DEBUG_EXCEPTION("malloc failed for mailbox_list");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		for (i = 0; i < mailbox_count; i++)
			em_convert_mailbox_tbl_to_mailbox(mailbox_tbl_list + i, (*mailbox_list) + i);
	} else
		*mailbox_list = NULL;

	if (count)
		*count = mailbox_count;

FINISH_OFF:

	if (mailbox_tbl_list != NULL)
		emstorage_free_mailbox(&mailbox_tbl_list, mailbox_count, NULL);

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

EXPORT_API int email_get_mailbox_list_by_keyword(int account_id, char *keyword, email_mailbox_t** mailbox_list, int* count)
{
	EM_DEBUG_API_BEGIN();

	int mailbox_count = 0;
	emstorage_mailbox_tbl_t* mailbox_tbl_list = NULL;
	int err = EMAIL_ERROR_NONE;
	int i = 0;
    char *multi_user_name = NULL;

	EM_IF_NULL_RETURN_VALUE(mailbox_list, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(count, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (!emstorage_get_mailbox_by_keyword(multi_user_name, account_id, keyword, &mailbox_tbl_list, &mailbox_count, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_keyword failed [%d]", err);
		goto FINISH_OFF;
	}

	if (mailbox_count > 0) {
		if (!(*mailbox_list = em_malloc(sizeof(email_mailbox_t) * mailbox_count))) {
			EM_DEBUG_EXCEPTION("malloc failed for mailbox_list");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		for (i = 0; i < mailbox_count; i++)
			em_convert_mailbox_tbl_to_mailbox(mailbox_tbl_list + i, (*mailbox_list) + i);
	} else
		*mailbox_list = NULL;

	if (count)
		*count = mailbox_count;

FINISH_OFF:

	if (mailbox_tbl_list != NULL)
		emstorage_free_mailbox(&mailbox_tbl_list, mailbox_count, NULL);

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_get_mailbox_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type,  email_mailbox_t** mailbox)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	email_mailbox_t* curr_mailbox = NULL;
	emstorage_mailbox_tbl_t* local_mailbox = NULL;

	EM_IF_NULL_RETURN_VALUE(mailbox, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(account_id, EMAIL_ERROR_INVALID_PARAM)	;

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (mailbox_type < EMAIL_MAILBOX_TYPE_INBOX || mailbox_type > EMAIL_MAILBOX_TYPE_USER_DEFINED) {
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mailbox_by_mailbox_type(multi_user_name, account_id, mailbox_type, &local_mailbox, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
		goto FINISH_OFF;
	} else {
		err = EMAIL_ERROR_NONE;
		curr_mailbox = em_malloc(sizeof(email_mailbox_t));
		if (curr_mailbox == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		memset(curr_mailbox, 0x00, sizeof(email_mailbox_t));
		em_convert_mailbox_tbl_to_mailbox(local_mailbox, curr_mailbox);
	}

	*mailbox = curr_mailbox;

FINISH_OFF:

	if (local_mailbox)
		emstorage_free_mailbox(&local_mailbox, 1, NULL);

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

EXPORT_API int email_get_mailbox_by_mailbox_id(int input_mailbox_id, email_mailbox_t** output_mailbox)
{
	EM_DEBUG_API_BEGIN();

	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	email_mailbox_t* curr_mailbox = NULL;
	emstorage_mailbox_tbl_t* local_mailbox = NULL;

	EM_IF_NULL_RETURN_VALUE(output_mailbox, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
		goto FINISH_OFF;
    }

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &local_mailbox)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err);
		goto FINISH_OFF;
	} else {
		curr_mailbox = em_malloc(sizeof(email_mailbox_t));
		if (curr_mailbox == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		memset(curr_mailbox, 0x00, sizeof(email_mailbox_t));

		em_convert_mailbox_tbl_to_mailbox(local_mailbox, curr_mailbox);
	}

	*output_mailbox = curr_mailbox;

FINISH_OFF:

	if (local_mailbox)
		emstorage_free_mailbox(&local_mailbox, 1, &err);

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_set_mail_slot_size(int account_id, int mailbox_id, int new_slot_size)
{
	EM_DEBUG_API_BEGIN("account_id[%d] mailbox_id[%d] new_slot_size[%d]", account_id, mailbox_id, new_slot_size);

	int err = EMAIL_ERROR_NONE;

	if (new_slot_size < 0) {
		EM_DEBUG_EXCEPTION("new_slot_size should be greater than 0 or should be equal to 0");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SET_MAIL_SLOT_SIZE);
	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	if (hAPI) {
		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &account_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION(" emipc_add_parameter for account_id failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &mailbox_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION(" emipc_add_parameter for account_id failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &new_slot_size, sizeof(int))) {
			EM_DEBUG_EXCEPTION(" emipc_add_parameter for new_slot_size failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
		}

	 	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		EM_DEBUG_LOG("email_set_mail_slot_size error VALUE [%d]", err);
		/*prevent 23139*/
/*		if (handle) {
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
			EM_DEBUG_LOG("email_set_mail_slot_size handle VALUE [%d]", handle);
		} */
		emipc_destroy_email_api(hAPI);
		hAPI = NULL;

	}
	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_stamp_sync_time_of_mailbox(int input_mailbox_id)
{
	EM_DEBUG_API_BEGIN("input_mailbox_id[%d]", input_mailbox_id);

	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(input_mailbox_id, EMAIL_ERROR_INVALID_PARAM);

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_STAMP_SYNC_TIME_OF_MAILBOX);
	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	if (hAPI) {
		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_mailbox_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION(" emipc_add_parameter for account_id failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
		}

	 	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		EM_DEBUG_LOG("email_stamp_sync_time_of_mailbox error VALUE [%d]", err);
		emipc_destroy_email_api(hAPI);
		hAPI = NULL;
	}

	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_free_mailbox(email_mailbox_t** mailbox_list, int count)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_list[%p] count[%d]", mailbox_list, count);
	int err = EMAIL_ERROR_NONE;

	if (count <= 0 || !mailbox_list || !*mailbox_list) {
		EM_DEBUG_LOG("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	email_mailbox_t *p = *mailbox_list;
	int i;

	for (i = 0; i < count; i++)  {
		EM_SAFE_FREE(p[i].mailbox_name);
		EM_SAFE_FREE(p[i].alias);
		EM_SAFE_FREE(p[i].eas_data);
	}

	EM_SAFE_FREE(p);
	*mailbox_list = NULL;

FINISH_OFF:
	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;

}
