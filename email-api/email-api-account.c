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
 * @file		email-api-account.c
 * @brief		This file contains the data structures and interfaces of Account related Functionality provided by
 *			email-service .
 */

#include "string.h"
#include "email-convert.h"
#include "email-api-account.h"
#include "email-storage.h"
#include "email-core-mail.h"
#include "email-core-account.h"
#include "email-core-utils.h"
#include "email-utilities.h"
#include "email-ipc.h"
#include "email-dbus-activation.h"

/* API - Adds the Email Account */
EXPORT_API int email_add_account(email_account_t* account)
{
	EM_DEBUG_API_BEGIN ("account[%p]", account);
	int size = 0;
	int err = EMAIL_ERROR_NONE;
	int ret_from_ipc = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	char* local_account_stream = NULL;
	HIPC_API hAPI = NULL;

	if (account == NULL || account->user_email_address == NULL || account->incoming_server_user_name == NULL || account->incoming_server_address == NULL||
		account->outgoing_server_user_name == NULL || account->outgoing_server_address == NULL)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (!emstorage_check_duplicated_account(multi_user_name, account, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_check_duplicated_account failed (%d) ", err);
		goto FINISH_OFF;
	}

	/* composing account information to be added */
	hAPI = emipc_create_email_api(_EMAIL_API_ADD_ACCOUNT);
	if (hAPI == NULL) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	local_account_stream = em_convert_account_to_byte_stream(account, &size);
	if (local_account_stream == NULL) {
		EM_DEBUG_EXCEPTION("em_convert_account_to_byte_stream failed");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if (!emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, local_account_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("APPID[%d], APIID [%d]", emipc_get_app_id(hAPI), emipc_get_api_id(hAPI));

	/* passing account information to service */
	if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
		goto FINISH_OFF;
	}

	/* get result from service */
	if ((ret_from_ipc = emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_get_parameter failed [%d]", ret_from_ipc);
		err = ret_from_ipc;
		goto FINISH_OFF;
	}

	if (err == EMAIL_ERROR_NONE) {
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &account->account_id);
	}
	else {	/*  get error code */
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &err);
	}

FINISH_OFF:

	if (hAPI)
		emipc_destroy_email_api(hAPI);

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}


EXPORT_API int email_free_account(email_account_t** account_list, int count)
{
	EM_DEBUG_FUNC_BEGIN ("account_list[%p] count[%d]", account_list, count); /* use only debugging */

	/*  default variable */
	int err = EMAIL_ERROR_NONE;

	if (count > 0)  {
		if (!account_list || !*account_list)  {
			err = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		email_account_t* p = *account_list;
		int i;

		for (i = 0; i < count; i++)  {
			EM_SAFE_FREE(p[i].account_name);
			EM_SAFE_FREE(p[i].incoming_server_address);
			EM_SAFE_FREE(p[i].user_email_address);
			EM_SAFE_FREE(p[i].incoming_server_user_name);
			EM_SAFE_FREE(p[i].incoming_server_password);
			EM_SAFE_FREE(p[i].outgoing_server_address);
			EM_SAFE_FREE(p[i].outgoing_server_user_name);
			EM_SAFE_FREE(p[i].outgoing_server_password);
			EM_SAFE_FREE(p[i].user_display_name);
			EM_SAFE_FREE(p[i].reply_to_address);
			EM_SAFE_FREE(p[i].return_address);
			EM_SAFE_FREE(p[i].logo_icon_path);
			EM_SAFE_FREE(p[i].user_data);
			EM_SAFE_FREE(p[i].certificate_path);
			p[i].user_data_length = 0;
			EM_SAFE_FREE(p[i].options.display_name_from);
			EM_SAFE_FREE(p[i].options.signature);
			EM_SAFE_FREE(p[i].options.alert_ringtone_path);
		}
		free(p); *account_list = NULL;
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END ("err[%d]", err);
	return err;
}


/* API - Delete  the Email Account */
EXPORT_API int email_delete_account(int account_id)
{
	EM_DEBUG_API_BEGIN ("account_id[%d]", account_id);

	int ret = 0;
	int err = EMAIL_ERROR_NONE;

	if (account_id < 0 || account_id == 0)  {
		EM_DEBUG_EXCEPTION(" account_id[%d]", account_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_DELETE_ACCOUNT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	/* account_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter account_id  failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &ret);
	if(ret != EMAIL_ERROR_NONE) {	/*  get error code */
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &err);
	}

	emipc_destroy_email_api(hAPI);

	hAPI = NULL;
	EM_DEBUG_API_END ("ret[%d] err[%d]", ret, err);
	return err;
}

/* API - Update  the Email Account */
EXPORT_API int email_update_account(int account_id, email_account_t* new_account)
{
	EM_DEBUG_API_BEGIN ("account_id[%d] new_account[%p]", account_id, new_account);

	int size = 0;
	int err = EMAIL_ERROR_NONE;
	int with_validation = false;
	char* new_account_stream = NULL;

	if ( account_id <= 0 || !new_account )  { /*prevent 23138*/
		EM_DEBUG_EXCEPTION("account_id[%d], new_account[%p], with_validation[%d]", account_id, new_account, with_validation);
		return  EMAIL_ERROR_INVALID_PARAM;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_UPDATE_ACCOUNT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	/* account_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter account_id failed ");
		goto FINISH_OFF;
	}

	/* new_account */
	new_account_stream = em_convert_account_to_byte_stream(new_account, &size);
	EM_PROXY_IF_NULL_RETURN_VALUE(new_account_stream, hAPI, EMAIL_ERROR_NULL_VALUE);
	if(!emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, new_account_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter new_account failed ");
		goto FINISH_OFF;
	}

	/* with_validation */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&with_validation, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter with_validation failed ");
		goto FINISH_OFF;
	}

	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

/* API - Update  the Email Account */
EXPORT_API int email_update_account_with_validation(int account_id, email_account_t* new_account)
{
	EM_DEBUG_API_BEGIN ("account_id[%d] new_account[%p]", account_id, new_account);

	int size = 0;
	int err = EMAIL_ERROR_NONE;
	int with_validation = true;
	char* new_account_stream = NULL;

	if ((account_id <= 0) || !new_account || (with_validation != false && with_validation != true))  {
		EM_DEBUG_EXCEPTION(" account_id[%d], new_account[%p], with_validation[%d]", account_id, new_account, with_validation);
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_UPDATE_ACCOUNT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	/* account_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("email_update_account--emipc_add_parameter account_id  failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	/* new_account */
	new_account_stream = em_convert_account_to_byte_stream(new_account, &size);
	EM_PROXY_IF_NULL_RETURN_VALUE(new_account_stream, hAPI, EMAIL_ERROR_NULL_VALUE);
	if(!emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, new_account_stream, size)) {
		EM_DEBUG_EXCEPTION("email_update_account--emipc_add_parameter new_account  failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	/* with_validation */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&with_validation, sizeof(int))) {
		EM_DEBUG_EXCEPTION("email_update_account--emipc_add_parameter with_validation  failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("email_update_account--emipc_execute_proxy_api failed ");
		/* Prevent defect 18624 */
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}


/* API - Get the account Information based on the account ID */
EXPORT_API int email_get_account(int account_id, int pulloption, email_account_t** account)
{
	EM_DEBUG_FUNC_BEGIN ("account_id[%d] pulloption[%d]", account_id, pulloption);
	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	emstorage_account_tbl_t *account_tbl = NULL;

	EM_IF_NULL_RETURN_VALUE(account_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(account, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (pulloption == GET_FULL_DATA)
		pulloption = EMAIL_ACC_GET_OPT_FULL_DATA;

	if (!emstorage_get_account_by_id(multi_user_name, account_id, pulloption, &account_tbl, true, &err)) {
		if (err != EMAIL_ERROR_SECURED_STORAGE_FAILURE) {
			EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed - %d", err);
			goto FINISH_OFF;
		}

		if (pulloption & EMAIL_ACC_GET_OPT_PASSWORD) {
			pulloption = pulloption & (~(EMAIL_ACC_GET_OPT_PASSWORD));	/*  disable password -Can't obtain password by MAPI */
			EM_DEBUG_LOG("change pulloption : disable password");
		}

		if (!emstorage_get_account_by_id(multi_user_name, account_id, pulloption, &account_tbl, true, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed - %d", err);
			goto FINISH_OFF;
		}
	}

	*account = em_malloc(sizeof(email_account_t));
	if (!*account) {
		EM_DEBUG_EXCEPTION("allocation failed [%d]", err);
		goto FINISH_OFF;
	}
	memset((void*)*account, 0, sizeof(email_account_t));
	em_convert_account_tbl_to_account(account_tbl, *account);

FINISH_OFF:

	if (account_tbl)
		emstorage_free_account(&account_tbl, 1, NULL);

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_FUNC_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_get_account_list(email_account_t** account_list, int* count)
{
	EM_DEBUG_FUNC_BEGIN ();

	int err = EMAIL_ERROR_NONE, i = 0;
    char *multi_user_name = NULL;

	EM_IF_NULL_RETURN_VALUE(account_list, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(count, EMAIL_ERROR_INVALID_PARAM);

	emstorage_account_tbl_t *temp_account_tbl = NULL;

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (!emstorage_get_account_list(multi_user_name, count, &temp_account_tbl , true, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_list failed [%d]", err);
		goto FINISH_OFF;
	}

	if(temp_account_tbl && (*count) > 0) {
		*account_list = em_malloc(sizeof(email_account_t) * (*count));
		if(!*account_list) {
			EM_DEBUG_EXCEPTION("allocation failed [%d]", err);
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		memset((void*)*account_list, 0, sizeof(email_account_t) * (*count));
		for(i = 0; i < (*count); i++)
			em_convert_account_tbl_to_account(temp_account_tbl + i, (*account_list) + i);
	}

FINISH_OFF:

	if(temp_account_tbl)
		emstorage_free_account(&temp_account_tbl, (*count), NULL);

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_FUNC_END ("err[%d]", err);
	return err;

}

EXPORT_API int email_validate_account(int account_id, int *handle)
{
	EM_DEBUG_API_BEGIN ("account_id[%d] handle[%p]", account_id, handle);

	int ret = 0;
	int err = EMAIL_ERROR_NONE;
	if (account_id <= 0)  {
		EM_DEBUG_EXCEPTION("account_id[%d]", account_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_VALIDATE_ACCOUNT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	/* account_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION(" emipc_add_parameter account_id  failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION(" emipc_execute_proxy_api failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &ret); /*  return  */
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &err); /*  error code */

	if(handle)
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 2, sizeof(int), handle);

	emipc_destroy_email_api(hAPI);

	hAPI = NULL;
	EM_DEBUG_API_END ("err[%d]", err);
	return err;

}

EXPORT_API int email_validate_account_ex(email_account_t* account, int *handle)
{
	EM_DEBUG_API_BEGIN ("account[%p]", account);
	char* local_account_stream = NULL;
	int ret = -1;
	int size = 0;
	int err = EMAIL_ERROR_NONE;

	if (account == NULL || account->user_email_address == NULL || 
		account->incoming_server_user_name == NULL || account->incoming_server_address == NULL||
		account->outgoing_server_user_name == NULL || account->outgoing_server_address == NULL)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	/* create account information */
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_VALIDATE_ACCOUNT_EX);
	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	local_account_stream = em_convert_account_to_byte_stream(account, &size);
	EM_PROXY_IF_NULL_RETURN_VALUE(local_account_stream, hAPI, EMAIL_ERROR_NULL_VALUE);
	if(!emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, local_account_stream, size)) {
		EM_DEBUG_EXCEPTION(" emipc_add_parameter  failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION(" emipc_execute_proxy_api failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &ret);
	if(ret == EMAIL_ERROR_NONE) {
		if(handle)
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
	} else {
		/*  get error code */
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &err);
	}

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_add_account_with_validation(email_account_t* account, int *handle)
{
	EM_DEBUG_API_BEGIN ("account[%p]", account);
	char* local_account_stream = NULL;
	int ret = -1;
	int size = 0;
	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	HIPC_API hAPI = NULL;

	if (account == NULL || account->user_email_address == NULL || account->incoming_server_user_name == NULL || account->incoming_server_address == NULL||
		account->outgoing_server_user_name == NULL || account->outgoing_server_address == NULL)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        return err;
    }

	if(emstorage_check_duplicated_account(multi_user_name, account, true, &err) == false) {
		EM_DEBUG_EXCEPTION("emstorage_check_duplicated_account failed (%d) ", err);
		goto FINISH_OFF;
	}

	/* create account information */
	hAPI = emipc_create_email_api(_EMAIL_API_ADD_ACCOUNT_WITH_VALIDATION);
	if (hAPI == NULL) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	local_account_stream = em_convert_account_to_byte_stream(account, &size);
	if (local_account_stream == NULL) {
		EM_DEBUG_EXCEPTION("em_convert_account_to_byte_stream failed");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if(!emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, local_account_stream, size)) {
		EM_DEBUG_EXCEPTION(" emipc_add_parameter  failed ");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION(" emipc_execute_proxy_api failed ");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
		goto FINISH_OFF;
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &ret);
	if(ret == EMAIL_ERROR_NONE) {
		if(handle)
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
	} else {
		/*  get error code */
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &err);
	}

FINISH_OFF:
	
	if (hAPI) {
		emipc_destroy_email_api(hAPI);
		hAPI = NULL;
	}

	EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}


EXPORT_API int email_backup_accounts_into_secure_storage(const char *file_name)
{
	EM_DEBUG_API_BEGIN ("file_name[%p]", file_name);

	int ret = 0;
	int err = EMAIL_ERROR_NONE;

	if (!file_name)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_BACKUP_ACCOUNTS);
	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	/* Checked the existed account */
	int account_count = 0;

	if (!emstorage_get_account_count(NULL, &account_count, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_count failed - %d", err);
		goto FINISH_OFF;
	}

	if (account_count == 0) {
		EM_DEBUG_LOG("Account is empty");
		err = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

	/* file_name */
	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)file_name, EM_SAFE_STRLEN(file_name)+1)) {
		EM_DEBUG_EXCEPTION(" emipc_add_parameter account_id  failed ");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION(" emipc_execute_proxy_api failed ");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
		goto FINISH_OFF;
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &ret);

	if (ret == false) {	/*  get error code */
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &err);
	}

FINISH_OFF:

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_restore_accounts_from_secure_storage(const char * file_name)
{
	EM_DEBUG_API_BEGIN ("file_name[%p]", file_name);
	int ret = 0;
	int err = EMAIL_ERROR_NONE;

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_RESTORE_ACCOUNTS);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	/* file_name */
	if (file_name) {
		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)file_name, EM_SAFE_STRLEN(file_name)+1)) {
			EM_DEBUG_EXCEPTION(" emipc_add_parameter account_id  failed ");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}
	}

	if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION(" emipc_execute_proxy_api failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &ret);

	if (0 == ret) {	/* get error code */
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &err);
	}

	emipc_destroy_email_api(hAPI);

	hAPI = NULL;
	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_get_password_length_of_account(int account_id, email_get_password_length_type password_type, int *password_length)
{
	EM_DEBUG_API_BEGIN ("account_id[%d] password_length[%p]", account_id, password_length);
	int ret = 0;
	int err = EMAIL_ERROR_NONE;

	if (account_id <= 0 || password_length == NULL)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_GET_PASSWORD_LENGTH_OF_ACCOUNT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	/* account_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION(" emipc_add_parameter account_id  failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	/* password type */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&password_type, sizeof(int))) {
		EM_DEBUG_EXCEPTION(" emipc_add_parameter account_id  failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION(" emipc_execute_proxy_api failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &ret);
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &err);
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 2, sizeof(int), password_length);

	emipc_destroy_email_api(hAPI);

	hAPI = NULL;
	EM_DEBUG_API_END ("*password_length[%d]", *password_length);
	return err;
}

EXPORT_API int email_update_notification_bar(int account_id, int total_mail_count, int unread_mail_count, int input_from_eas)
{
	EM_DEBUG_API_BEGIN ("account_id[%d] total_mail_count[%d] unread_mail_count[%d]", account_id, total_mail_count, unread_mail_count);
	int err = EMAIL_ERROR_NONE;

	if (account_id == 0)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_UPDATE_NOTIFICATION_BAR_FOR_UNREAD_MAIL);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	/* account_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION(" emipc_add_parameter account_id  failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	/* total_mail_count */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&total_mail_count, sizeof(int))) {
		EM_DEBUG_EXCEPTION(" emipc_add_parameter account_id  failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	/* unread_mail_count */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&unread_mail_count, sizeof(int))) {
		EM_DEBUG_EXCEPTION(" emipc_add_parameter account_id  failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	/* input_from_eas */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_from_eas, sizeof(int))) {
		EM_DEBUG_EXCEPTION(" emipc_add_parameter account_id  failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}


	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION(" emipc_execute_proxy_api failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	emipc_destroy_email_api(hAPI);

	hAPI = NULL;
	EM_DEBUG_API_END ("ret[%d]", err);
	return err;
}

EXPORT_API int email_clear_all_notification_bar()
{
    EM_DEBUG_API_BEGIN ();
	int err = EMAIL_ERROR_NONE;
	int account_id = ALL_ACCOUNT;

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_CLEAR_NOTIFICATION_BAR);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	/* account_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION(" emipc_add_parameter account_id  failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION(" emipc_execute_proxy_api failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	emipc_destroy_email_api(hAPI);

	hAPI = NULL;
	EM_DEBUG_API_END ("ret[%d]", err);
	return err;
}

EXPORT_API int email_clear_notification_bar(int account_id)
{
	EM_DEBUG_API_BEGIN ();
	int err = EMAIL_ERROR_NONE;

	if (account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION ("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_CLEAR_NOTIFICATION_BAR);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	/* account_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION(" emipc_add_parameter account_id  failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION(" emipc_execute_proxy_api failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	emipc_destroy_email_api(hAPI);

	hAPI = NULL;
	EM_DEBUG_API_END ("ret[%d]", err);
	return err;
}

EXPORT_API int email_save_default_account_id(int input_account_id)
{
	EM_DEBUG_API_BEGIN ("input_account_id [%d]", input_account_id);
	int err = EMAIL_ERROR_NONE;

	if (input_account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION ("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SAVE_DEFAULT_ACCOUNT_ID);
	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	/* Input account ID */
	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, (char *)&input_account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter input_account_id failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	emipc_destroy_email_api(hAPI);

	hAPI = NULL;
	EM_DEBUG_API_END ("ret[%d]", err);
	return err;
}

EXPORT_API int email_load_default_account_id(int *output_account_id)
{
	EM_DEBUG_FUNC_BEGIN ("output_account_id[%p]", output_account_id);

	int err = EMAIL_ERROR_NONE;
	int account_id = 0;

	EM_IF_NULL_RETURN_VALUE(output_account_id, EMAIL_ERROR_INVALID_PARAM);

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_LOAD_DEFAULT_ACCOUNT_ID);
	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	
	if (err == EMAIL_ERROR_NONE) {
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &account_id);
	}
	
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	*output_account_id = account_id;

	EM_DEBUG_FUNC_END ("ret[%d]", err);
	return err;
}
