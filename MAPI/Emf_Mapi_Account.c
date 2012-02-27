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
 * to interact with Email Engine.
 * @file		Emf_Mapi_Account.c
 * @brief		This file contains the data structures and interfaces of Account related Functionality provided by 
 *			Email Engine . 
 */
 
#include <Emf_Mapi.h>
#include "string.h"
#include "Msg_Convert.h"
#include "Emf_Mapi_Account.h"
#include "em-storage.h"
#include "em-core-mesg.h"
#include "em-core-account.h"
#include "em-core-utils.h"
#include "ipc-library.h"

/* API - Adds the Email Account */
EXPORT_API int email_add_account(emf_account_t* account)
{
	EM_DEBUG_FUNC_BEGIN("account[%p]", account);
	char* local_account_stream = NULL;
	int ret = -1;
	int size = 0;
	int account_id;
	int err = EMF_ERROR_NONE;

	if ( !account )  {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return EMF_ERROR_INVALID_PARAM;
	}

	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_ADD_ACCOUNT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	local_account_stream = em_convert_account_to_byte_stream(account, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(local_account_stream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, local_account_stream, size))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");

	EM_DEBUG_LOG("APPID[%d], APIID [%d]", ipcEmailAPI_GetAPPID(hAPI), ipcEmailAPI_GetAPIID(hAPI));

	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPI failed");
		EM_SAFE_FREE(local_account_stream);	
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}
	
	 ret = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);
	 if(ret == 1) {
		account_id = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);
		EM_DEBUG_LOG("APPID[%d], APIID [%d]", ipcEmailAPI_GetAPPID(hAPI), ipcEmailAPI_GetAPIID(hAPI));
		account->account_id = account_id;
	} else {	/*  get error code */
		err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);
	}
	
	ipcEmailAPI_Destroy(hAPI);
	hAPI = NULL;

	EM_SAFE_FREE(local_account_stream);
	EM_DEBUG_FUNC_END("RETURN VALUE [%d], ERROR CODE [%d]", ret, err);
	return err;
}


EXPORT_API int email_free_account(emf_account_t** account_list, int count)
{
	EM_DEBUG_FUNC_BEGIN("account_list[%p], count[%d]", account_list, count);

	/*  default variable */
	int err = EMF_ERROR_NONE;

	if (count > 0)  {
		if (!account_list || !*account_list)  {
			err = EMF_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		emf_account_t* p = *account_list;
		int i;
	
		for (i = 0; i < count; i++)  {
			EM_SAFE_FREE(p[i].account_name);
			EM_SAFE_FREE(p[i].receiving_server_addr);
			EM_SAFE_FREE(p[i].email_addr);
			EM_SAFE_FREE(p[i].user_name);
			EM_SAFE_FREE(p[i].password);
			EM_SAFE_FREE(p[i].sending_server_addr);
			EM_SAFE_FREE(p[i].sending_user);
			EM_SAFE_FREE(p[i].sending_password);
			EM_SAFE_FREE(p[i].display_name);
			EM_SAFE_FREE(p[i].reply_to_addr);
			EM_SAFE_FREE(p[i].return_addr);
			EM_SAFE_FREE(p[i].logo_icon_path);
			EM_SAFE_FREE(p[i].options.display_name_from);
			EM_SAFE_FREE(p[i].options.signature);
		}
		free(p); *account_list = NULL;
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END("ERROR CODE [%d]", err);
	return err;
}
 

/* API - Delete  the Email Account */
EXPORT_API int email_delete_account(int account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d]", account_id);

	int ret = 0;
	int err = EMF_ERROR_NONE;

	if (account_id < 0 || account_id == 0)  {	
		EM_DEBUG_EXCEPTION(" account_id[%d]", account_id);
		err = EMF_ERROR_INVALID_PARAM;
		return err;
	}

	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_DELETE_ACCOUNT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* account_id */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter account_id  failed ");
	
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPI failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	ret = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);
	if(0==ret) {	/*  get error code */
		err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);	
	}
	ipcEmailAPI_Destroy(hAPI);

	hAPI = NULL;
	EM_DEBUG_FUNC_END("RETURN VALUE [%d], ERROR CODE [%d]", ret, err);
	return err;
}

/* API - Update  the Email Account */
EXPORT_API int email_update_account(int account_id, emf_account_t* new_account)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], new_account[%p]", account_id, new_account);

	int ret = 0;
	int size = 0;
	int err = EMF_ERROR_NONE;
	int with_validation = false, *result_from_ipc = NULL;
	char* new_account_stream = NULL;

	if ((account_id <= 0) || !new_account || (with_validation != false && with_validation != true))  {
		EM_DEBUG_EXCEPTION("account_id[%d], new_account[%p], with_validation[%d]", account_id, new_account, with_validation);
		err = EMF_ERROR_INVALID_PARAM;
		return err;
	}

	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_UPDATE_ACCOUNT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* account_id */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter account_id failed ");

	/* new_account */
	new_account_stream = em_convert_account_to_byte_stream(new_account, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(new_account_stream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, new_account_stream, size))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter new_account failed ");

	/* with_validation */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&with_validation, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter with_validation failed ");

	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPI failed");
		/* Prevent defect 18624 */
		EM_SAFE_FREE(new_account_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	result_from_ipc = (int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);

	if(!result_from_ipc) {
		EM_DEBUG_EXCEPTION("ipcEmailAPI_GetParameter fail ");
		err = EMF_ERROR_IPC_CRASH;
		goto FINISH_OFF;
	}

	ret = *result_from_ipc;

	if(ret == 0) {	/*  get error code */
		result_from_ipc = (int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);
		if(!result_from_ipc) {
			EM_DEBUG_EXCEPTION("ipcEmailAPI_GetParameter fail ");
			err = EMF_ERROR_IPC_CRASH;
			goto FINISH_OFF;
		}

		err = *result_from_ipc;	
	}

FINISH_OFF:

	ipcEmailAPI_Destroy(hAPI);
	hAPI = NULL;

	EM_SAFE_FREE(new_account_stream);
	EM_DEBUG_FUNC_END("RETURN VALUE [%d], ERROR CODE [%d]", ret, err);
	return err;
}

/* API - Update  the Email Account */
EXPORT_API int email_update_account_with_validation(int account_id, emf_account_t* new_account)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], new_account[%p]", account_id, new_account);

	int ret = 0;
	int size = 0;
	int err = EMF_ERROR_NONE;
	int with_validation = true;
	char* new_account_stream = NULL;

	if ((account_id <= 0) || !new_account || (with_validation != false && with_validation != true))  {
		EM_DEBUG_EXCEPTION(" account_id[%d], new_account[%p], with_validation[%d]", account_id, new_account, with_validation);
		err = EMF_ERROR_INVALID_PARAM;
		return err;
	}

	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_UPDATE_ACCOUNT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* account_id */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int)))
		EM_DEBUG_EXCEPTION("email_update_account--ipcEmailAPI_AddParameter account_id  failed ");

	/* new_account */
	new_account_stream = em_convert_account_to_byte_stream(new_account, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(new_account_stream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, new_account_stream, size))
		EM_DEBUG_EXCEPTION("email_update_account--ipcEmailAPI_AddParameter new_account  failed ");

	/* with_validation */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&with_validation, sizeof(int)))
		EM_DEBUG_EXCEPTION("email_update_account--ipcEmailAPI_AddParameter with_validation  failed ");


	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("email_update_account--ipcEmailProxy_ExecuteAPI failed ");
		/* Prevent defect 18624 */
		EM_SAFE_FREE(new_account_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	ret = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);
	if(ret == 0) {	/*  get error code */
		err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);	
	}

	ipcEmailAPI_Destroy(hAPI);
	hAPI = NULL;

	EM_SAFE_FREE(new_account_stream);
	EM_DEBUG_FUNC_END("RETURN VALUE [%d], ERROR CODE [%d]", ret, err);
	return err;


}


/* API - Get the account Information based on the account ID */
EXPORT_API int email_get_account(int account_id, int pulloption, emf_account_t** account)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = 0;
	int err = EMF_ERROR_NONE;
	emf_mail_account_tbl_t *account_tbl = NULL;

	EM_IF_NULL_RETURN_VALUE(account_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(account, EMF_ERROR_INVALID_PARAM);

	if (pulloption == GET_FULL_DATA)
		pulloption = EMF_ACC_GET_OPT_FULL_DATA;

	if (pulloption & EMF_ACC_GET_OPT_PASSWORD) {
		pulloption = pulloption & (~(EMF_ACC_GET_OPT_PASSWORD));	/*  disable password -Can't obtain password by MAPI */
		EM_DEBUG_LOG("change pulloption : disable password");
	}

	if (!em_storage_get_account_by_id(account_id, pulloption, &account_tbl, true, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_get_account_by_id failed - %d", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	*account = em_core_malloc(sizeof(emf_account_t));
	if (!*account) {
		EM_DEBUG_EXCEPTION("allocation failed [%d]", err);
		goto FINISH_OFF;
	}
	memset((void*)*account, 0, sizeof(emf_account_t));
	em_convert_account_tbl_to_account(account_tbl, *account);

	err = em_storage_get_emf_error_from_em_storage_error(err);
	ret = true;

FINISH_OFF:
	if (account_tbl)
		em_storage_free_account(&account_tbl, 1, NULL);

	EM_DEBUG_FUNC_END();
	return err;
}

EXPORT_API int email_get_account_list(emf_account_t** account_list, int* count)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMF_ERROR_NONE, i = 0;
	emf_mail_account_tbl_t *temp_account_tbl = NULL;

	EM_IF_NULL_RETURN_VALUE(account_list, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(count, EMF_ERROR_INVALID_PARAM);

	if (!em_storage_get_account_list(count, &temp_account_tbl , true, false, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_get_account_list failed [%d]", err);

		goto FINISH_OFF;
	}

	if(temp_account_tbl && (*count) > 0) {
		*account_list = em_core_malloc(sizeof(emf_account_t) * (*count));
		if(!*account_list) {
			EM_DEBUG_EXCEPTION("allocation failed [%d]", err);
			goto FINISH_OFF;
		}
		memset((void*)*account_list, 0, sizeof(emf_account_t) * (*count));
		for(i = 0; i < (*count); i++)
			em_convert_account_tbl_to_account(temp_account_tbl + i, (*account_list) + i);
	}

FINISH_OFF:
	err = em_storage_get_emf_error_from_em_storage_error(err);

	if(temp_account_tbl)
		em_storage_free_account(&temp_account_tbl, (*count), NULL);
	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;

}

EXPORT_API int email_validate_account(int account_id, unsigned* handle)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], handle[%p]", account_id, handle);

	int ret = 0;
	int err = EMF_ERROR_NONE;
	if (account_id <= 0)  {	
		EM_DEBUG_EXCEPTION("account_id[%d]", account_id);
		err = EMF_ERROR_INVALID_PARAM;	
		return err;
	}

	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_VALIDATE_ACCOUNT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* account_id */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int)))
		EM_DEBUG_EXCEPTION(" ipcEmailAPI_AddParameter account_id  failed ");
	
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION(" ipcEmailProxy_ExecuteAPI failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	ret = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0); /*  return  */
	err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1); /*  error code */

	if(handle)
		*handle = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 2);

	ipcEmailAPI_Destroy(hAPI);

	hAPI = NULL;
	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;

}


EXPORT_API int email_add_account_with_validation(emf_account_t* account, unsigned* handle)
{
	EM_DEBUG_FUNC_BEGIN("account[%p]", account);
	char* local_account_stream = NULL;
	int ret = -1;
	int size = 0;
	int handle_inst = 0;
	int err = EMF_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(account, false);

	if(em_storage_check_duplicated_account(account, true, &err) == false) {
		EM_DEBUG_EXCEPTION("em_storage_check_duplicated_account failed (%d) ", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		return err;
	}

	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_ADD_ACCOUNT_WITH_VALIDATION);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	EM_DEBUG_LOG("Before em_convert_account_to_byte_stream");
	local_account_stream = em_convert_account_to_byte_stream(account, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(local_account_stream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, local_account_stream, size))
		EM_DEBUG_EXCEPTION(" ipcEmailAPI_AddParameter  failed ");

	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION(" ipcEmailProxy_ExecuteAPI failed ");
		EM_SAFE_FREE(local_account_stream);	
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}
	
	ret = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);
	if(ret == 1) {
		handle_inst = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);
		if(handle)
			*handle = handle_inst;
	} else {	/*  get error code */
		err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);
	}
	
	ipcEmailAPI_Destroy(hAPI);
	hAPI = NULL;

	EM_SAFE_FREE(local_account_stream);
	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}


EXPORT_API int email_backup_accounts_into_secure_storage(const char *file_name)
{
	EM_DEBUG_FUNC_BEGIN("file_name[%s]", file_name);
	int ret = 0;
	int err = EMF_ERROR_NONE;

	if (!file_name)  {	
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		return err;
	}

	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_BACKUP_ACCOUNTS);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* file_name */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)file_name, strlen(file_name)))
		EM_DEBUG_EXCEPTION(" ipcEmailAPI_AddParameter account_id  failed ");
	
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION(" ipcEmailProxy_ExecuteAPI failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	ret = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);

	if(0 == ret) {	/*  get error code */
		err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);	
	}

	ipcEmailAPI_Destroy(hAPI);

	hAPI = NULL;
	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

EXPORT_API int email_restore_accounts_from_secure_storage(const char * file_name)
{
	EM_DEBUG_FUNC_BEGIN("file_name[%s]", file_name);
	int ret = 0;
	int err = EMF_ERROR_NONE;

	if (!file_name)  {	
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		return err;
	}

	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_RESTORE_ACCOUNTS);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* file_name */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)file_name, strlen(file_name)))
		EM_DEBUG_EXCEPTION(" ipcEmailAPI_AddParameter account_id  failed ");
	
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION(" ipcEmailProxy_ExecuteAPI failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	ret = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);

	if(0==ret) {	/*  get error code */
		err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);	
	}

	ipcEmailAPI_Destroy(hAPI);

	hAPI = NULL;
	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

EXPORT_API int email_get_password_length_of_account(const int account_id, int *password_length)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], password_length[%p]", account_id, password_length);
	int ret = 0;
	int err = EMF_ERROR_NONE;

	if (account_id <= 0 || password_length == NULL)  {	
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;	
		return err;
	}

	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_GET_PASSWORD_LENGTH_OF_ACCOUNT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* account_id */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int)))
		EM_DEBUG_EXCEPTION(" ipcEmailAPI_AddParameter account_id  failed ");
	
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION(" ipcEmailProxy_ExecuteAPI failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	ret = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0); /*  return  */
	err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1); /*  error code */

	*password_length = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 2);

	ipcEmailAPI_Destroy(hAPI);

	hAPI = NULL;
	EM_DEBUG_FUNC_END("*password_length[%d]", *password_length);
	return err;
}

EXPORT_API int email_update_notification_bar(int account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d]", account_id);
	int err = EMF_ERROR_NONE, *return_value = NULL;

	if (account_id == 0)  {	
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;	
		return err;
	}

	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_UPDATE_NOTIFICATION_BAR_FOR_UNREAD_MAIL);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* account_id */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int)))
		EM_DEBUG_EXCEPTION(" ipcEmailAPI_AddParameter account_id  failed");
	
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION(" ipcEmailProxy_ExecuteAPI failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	return_value = (int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0); /*  error code */;
	if(return_value)
		err = *return_value;

	ipcEmailAPI_Destroy(hAPI);

	hAPI = NULL;
	EM_DEBUG_FUNC_END("ret[%d]", err);
	return err;
}


EXPORT_API int email_clear_all_notification_bar()
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = 0;
	int err = EMF_ERROR_NONE;

	ret = em_core_clear_all_notifications();

	EM_DEBUG_FUNC_END("ret[%d]", ret);
	return err;
}

EXPORT_API int email_save_default_account_id(int input_account_id)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d]", input_account_id);
	int ret = 0;
	int err = EMF_ERROR_NONE;

	ret = em_core_save_default_account_id(input_account_id);

	EM_DEBUG_FUNC_END("ret[%d]", ret);
	return err;
}

EXPORT_API int email_load_default_account_id(int *output_account_id)
{
	EM_DEBUG_FUNC_BEGIN("output_account_id [%p]", output_account_id);
	int ret = 0;
	int err = EMF_ERROR_NONE;

	ret = em_core_load_default_account_id(output_account_id);

	EM_DEBUG_FUNC_END("ret[%d]", ret);
	return err;
}