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
 * @file		email-api-smime.c
 * @brief 		This file contains the data structures and interfaces of SMIME related Functionality provided by
 *			Email Engine . 
 */

#include "email-api.h"
#include "string.h"
#include "email-convert.h"
#include "email-api-account.h"
#include "email-storage.h"
#include "email-utilities.h"
#include "email-core-mail.h"
#include "email-core-mime.h"
#include "email-core-account.h"
#include "email-core-cert.h"
#include "email-core-smime.h"
#include "email-ipc.h"

EXPORT_API int email_add_certificate(char *certificate_path, char *email_address)
{
	EM_DEBUG_FUNC_BEGIN("Certificate path : [%s]", certificate_path);
	int result_from_ipc = 0;
	int err = EMAIL_ERROR_NONE;
	
	if (!certificate_path) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_ADD_CERTIFICATE);
	if (hAPI == NULL) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, certificate_path, EM_SAFE_STRLEN(certificate_path)+1)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter certificate_path[%s] failed", certificate_path);
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, email_address, EM_SAFE_STRLEN(email_address)+1)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter certificate_path[%s] failed", email_address);
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if (emipc_execute_proxy_api(hAPI) < 0) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
		goto FINISH_OFF;
	}

	result_from_ipc = emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	if (result_from_ipc != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_get_parameter failed");
		err = EMAIL_ERROR_IPC_CRASH;
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_FUNC_END("ERROR CODE [%d]", err);
	return err;
}

EXPORT_API int email_delete_certificate(char *email_address)
{
	EM_DEBUG_FUNC_BEGIN("Eamil_address : [%s]", email_address);
	int result_from_ipc = 0;
	int err = EMAIL_ERROR_NONE;
	
	if (!email_address) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_DELETE_CERTIFICATE);
	if (hAPI == NULL) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, email_address, EM_SAFE_STRLEN(email_address)+1)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter email_address[%s] failed", email_address);
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if (emipc_execute_proxy_api(hAPI) < 0) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
		goto FINISH_OFF;
	}

	result_from_ipc = emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	if (result_from_ipc != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_get_parameter failed");
		err = EMAIL_ERROR_IPC_CRASH;
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_FUNC_END("ERROR CODE [%d]", err);
	return err;
}

EXPORT_API int email_get_certificate(char *email_address, email_certificate_t **certificate)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	char temp_email_address[130] = {0, };
	emstorage_certificate_tbl_t *cert = NULL;
	
	EM_IF_NULL_RETURN_VALUE(email_address, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(certificate, EMAIL_ERROR_INVALID_PARAM);
	SNPRINTF(temp_email_address, sizeof(temp_email_address), "<%s>", email_address);
	
	if (!emstorage_get_certificate_by_email_address(temp_email_address, &cert, false, 0, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_certificate_by_index failed - %d", err);
		return err;
	}

	if (!em_convert_certificate_tbl_to_certificate(cert, certificate, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_certificate_tbl_to_certificate failed");
		return err;
	}	
	
	EM_DEBUG_FUNC_END("ERROR CODE [%d]", err);
	return err;
}

EXPORT_API int email_get_decrypt_message(int mail_id, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data, int *output_attachment_count)
{
	EM_DEBUG_FUNC_BEGIN("mail_id : [%d]", mail_id);
	int err = EMAIL_ERROR_NONE;
	int p_output_attachment_count = 0;
	char *decrypt_filepath = NULL;
	email_mail_data_t *p_output_mail_data = NULL;
	email_attachment_data_t *p_output_attachment_data = NULL;
	emstorage_account_tbl_t *p_account_tbl = NULL;

	EM_IF_NULL_RETURN_VALUE(mail_id, EMAIL_ERROR_INVALID_PARAM);

	if (!output_mail_data || !output_attachment_data || !output_attachment_count) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emcore_get_mail_data(mail_id, &p_output_mail_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_data failed");
		goto FINISH_OFF;
	}

	if (!emstorage_get_account_by_id(p_output_mail_data->account_id, EMAIL_ACC_GET_OPT_OPTIONS, &p_account_tbl, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed : [%d]", err);
		goto FINISH_OFF;
	}

	if ((err = emcore_get_attachment_data_list(mail_id, &p_output_attachment_data, &p_output_attachment_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_attachment_data_list failed");
		goto FINISH_OFF;
	}

	if (p_output_attachment_count != 1 || !p_output_attachment_data) {
		EM_DEBUG_EXCEPTION("This is not the encrypted mail");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emcore_smime_set_decrypt_message(p_output_attachment_data->attachment_path, p_account_tbl->certificate_path, &decrypt_filepath, &err)) {
		EM_DEBUG_EXCEPTION("emcore_smime_set_decrypt_message failed");
		goto FINISH_OFF;
	}

	/* Change decrpyt_message to mail_data_t */
	if (!emcore_parse_mime_file_to_mail(decrypt_filepath, output_mail_data, output_attachment_data, output_attachment_count, &err)) {
		EM_DEBUG_EXCEPTION("emcore_parse_mime_file_to_mail failed : [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (p_account_tbl)
		emstorage_free_account(&p_account_tbl, 1, NULL);

	if (p_output_mail_data)
		email_free_mail_data(&p_output_mail_data, 1);

	if (p_output_attachment_data)
		email_free_attachment_data(&p_output_attachment_data, p_output_attachment_count);

	EM_DEBUG_FUNC_END("ERROR CODE [%d]", err);
	return err;
}

EXPORT_API int email_verify_signature(int mail_id, int *verify)
{
	EM_DEBUG_FUNC_BEGIN("mail_id : [%d]", mail_id);
	int result_from_ipc = 0;
	int err = EMAIL_ERROR_NONE;
	int p_verify = 0;

	EM_IF_NULL_RETURN_VALUE(mail_id, EMAIL_ERROR_INVALID_PARAM);	

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_VERIFY_SIGNATURE);
	if (hAPI == NULL) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, &mail_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter pass_phrase[%d] failed", mail_id);
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if (emipc_execute_proxy_api(hAPI) < 0) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
		goto FINISH_OFF;
	}

	result_from_ipc = emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &p_verify);
	if (result_from_ipc != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_get_parameter failed");
		err = EMAIL_ERROR_IPC_CRASH;
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (hAPI)
		emipc_destroy_email_api(hAPI);

	if (verify != NULL)
		*verify = p_verify;

	EM_DEBUG_FUNC_END("ERROR CODE [%d]", err);
	return err;
}

EXPORT_API int email_verify_certificate(char *certificate_path, int *verify)
{
	EM_DEBUG_FUNC_BEGIN("certificate : [%s]", certificate_path);
	int err = EMAIL_ERROR_NONE;
	int result_from_ipc = 0;
	int p_verify = 0;
	
	if (!certificate_path) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_VERIFY_CERTIFICATE);
	if (hAPI == NULL) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, certificate_path, EM_SAFE_STRLEN(certificate_path)+1)) {
		EM_DEBUG_EXCEPTION("emipc_add_paramter failed : [%s]", certificate_path);
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	if (emipc_execute_proxy_api(hAPI) < 0) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
		goto FINISH_OFF;
	}

	result_from_ipc = emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &p_verify);
	if (result_from_ipc != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_get_parameter failed");
		err = EMAIL_ERROR_IPC_CRASH;
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (hAPI)	
		emipc_destroy_email_api(hAPI);

	if (verify != NULL)
		*verify = p_verify;

	EM_DEBUG_FUNC_END("ERROR CODE [%d]", err);
	return err;
}

/*
EXPORT_API int email_check_ocsp_status(char *email_address, char *response_url, unsigned *handle)
{
	EM_DEBUG_FUNC_BEGIN("email_address : [%s], response_url : [%s], handle : [%p]", email_address, response_url, handle);

	EM_IF_NULL_RETURN_VALUE(email_address, EMAIL_ERROR_INVALID_PARAM);

	int err = EMAIL_ERROR_NONE;
	HIPC_API hAPI = NULL;

	hAPI = emipc_create_email_api(_EMAIL_API_CHECK_OCSP_STATUS);
	
	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, email_address, EM_SAFE_STRLEN(email_address)+1)) {
		EM_DEBUG_EXCEPTION("email_check_ocsp_status--ADD Param email_address failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if (!emipc_add_parameter(hAPI, ePARAMETER_IN, response_url, EM_SAFE_STRLEN(response_url)+1)) {
		EM_DEBUG_EXCEPTION("email_check_ocsp_status--ADD Param response_url failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("email_check_oscp_status--emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	emipc_get_paramter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	if (err == EMAIL_ERROR_NONE) {
		if (handle)
			emipc_get_paramter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
	}
}
*/
EXPORT_API int email_validate_certificate(int account_id, char *email_address, unsigned *handle)
{
	EM_DEBUG_FUNC_BEGIN("account_id : [%d], email_address : [%s], handle : [%p]", account_id, email_address, handle);
	
	EM_IF_NULL_RETURN_VALUE(account_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(email_address, EMAIL_ERROR_INVALID_PARAM);

	int err = EMAIL_ERROR_NONE;
	int as_handle = 0;
	email_account_server_t account_server_type;
	ASNotiData as_noti_data;

	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	if (em_get_account_server_type_by_account_id(account_id, &account_server_type, false, &err)	== false) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if (account_server_type != EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
		EM_DEBUG_EXCEPTION("This api is not supported except of active sync");
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if (em_get_handle_for_activesync(&as_handle, &err) == false) {
		EM_DEBUG_EXCEPTION("em_get_handle_for_activesync_failed[%d]", err);
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;		
	}

	as_noti_data.validate_certificate.handle = as_handle;
	as_noti_data.validate_certificate.account_id = account_id;
	as_noti_data.validate_certificate.email_address = strdup(email_address);

	if (em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_VALIDATE_CERTIFICATE, &as_noti_data) == false) {
		EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed");
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if (handle)
		*handle = as_handle;

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_get_resolve_recipients(int account_id, char *email_address, unsigned *handle)
{
	EM_DEBUG_FUNC_BEGIN("account_id : [%d], email_address : [%s], handle : [%p]", account_id, email_address, handle);
	
	EM_IF_NULL_RETURN_VALUE(account_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(email_address, EMAIL_ERROR_INVALID_PARAM);

	int err = EMAIL_ERROR_NONE;
	int as_handle = 0;
	email_account_server_t account_server_type;
	ASNotiData as_noti_data;

	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	if (em_get_account_server_type_by_account_id(account_id, &account_server_type, false, &err)	== false) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if (account_server_type != EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
		EM_DEBUG_EXCEPTION("This api is not supported except of active sync");
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if (em_get_handle_for_activesync(&as_handle, &err) == false) {
		EM_DEBUG_EXCEPTION("em_get_handle_for_activesync_failed[%d]", err);
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;		
	}

	as_noti_data.get_resolve_recipients.handle = as_handle;
	as_noti_data.get_resolve_recipients.account_id = account_id;
	as_noti_data.get_resolve_recipients.email_address = strdup(email_address);

	if (em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_RESOLVE_RECIPIENT, &as_noti_data) == false) {
		EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed");
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if (handle)
		*handle = as_handle;

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_free_certificate(email_certificate_t **certificate, int count)
{
	EM_DEBUG_FUNC_BEGIN("certificate[%p], count[%d]", certificate, count);
	int err = EMAIL_ERROR_NONE;
	emcore_free_certificate(certificate, count, &err);
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
