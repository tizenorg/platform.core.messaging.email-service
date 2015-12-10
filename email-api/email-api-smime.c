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
 * to interact with Email Engine.
 * @file		email-api-smime.c
 * @brief 		This file contains the data structures and interfaces of SMIME related Functionality provided by
 *			Email Engine .
 */

#include "string.h"
#include "email-api-mail.h"
#include "email-convert.h"
#include "email-api-account.h"
#include "email-storage.h"
#include "email-utilities.h"
#include "email-core-mail.h"
#include "email-core-mime.h"
#include "email-core-account.h"
#include "email-core-cert.h"
#include "email-core-smime.h"
#include "email-core-pgp.h"
#include "email-core-signal.h"
#include "email-ipc.h"

EXPORT_API int email_get_decrypt_message(int mail_id, email_mail_data_t **output_mail_data,
										email_attachment_data_t **output_attachment_data,
										int *output_attachment_count, int *verify)
{
	EM_DEBUG_API_BEGIN("mail_id[%d]", mail_id);
	int err = EMAIL_ERROR_NONE;
	int p_output_attachment_count = 0;
    int i = 0;
	char *decrypt_filepath = NULL;
    char *search = NULL;
    char *multi_user_name = NULL;
	email_mail_data_t *p_output_mail_data = NULL;
	email_attachment_data_t *p_output_attachment_data = NULL;
	emstorage_account_tbl_t *p_account_tbl = NULL;

	EM_IF_NULL_RETURN_VALUE(mail_id, EMAIL_ERROR_INVALID_PARAM);

	if (!output_mail_data || !output_attachment_data || !output_attachment_count) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]");
        goto FINISH_OFF;
    }

	if ((err = emcore_get_mail_data(multi_user_name, mail_id, &p_output_mail_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_data failed");
		goto FINISH_OFF;
	}

	if (!emstorage_get_account_by_id(multi_user_name, p_output_mail_data->account_id, EMAIL_ACC_GET_OPT_OPTIONS, &p_account_tbl, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed : [%d]", err);
		goto FINISH_OFF;
	}

	if ((err = emcore_get_attachment_data_list(multi_user_name, mail_id, &p_output_attachment_data, &p_output_attachment_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_attachment_data_list failed");
		goto FINISH_OFF;
	}

        for (i = 0; i < p_output_attachment_count; i++) {
                EM_DEBUG_LOG("mime_type : [%s]", p_output_attachment_data[i].attachment_mime_type);
                if (p_output_attachment_data[i].attachment_mime_type && (search = strcasestr(p_output_attachment_data[i].attachment_mime_type, "PKCS7-MIME"))) {
                        EM_DEBUG_LOG("Found the encrypt file");
                        break;
                } else if (p_output_attachment_data[i].attachment_mime_type && (search = strcasestr(p_output_attachment_data[i].attachment_mime_type, "octet-stream"))) {
			EM_DEBUG_LOG("Found the encrypt file");
			break;
		}
        }

        if (!search) {
                EM_DEBUG_EXCEPTION("No have a decrypt file");
                err = EMAIL_ERROR_INVALID_PARAM;
                goto FINISH_OFF;
        }

	if (p_output_mail_data->smime_type == EMAIL_SMIME_ENCRYPTED || p_output_mail_data->smime_type == EMAIL_SMIME_SIGNED_AND_ENCRYPTED) {
                emcore_init_openssl_library();
		if (!emcore_smime_get_decrypt_message(p_output_attachment_data[i].attachment_path, p_account_tbl->certificate_path, &decrypt_filepath, &err)) {
			EM_DEBUG_EXCEPTION("emcore_smime_get_decrypt_message failed");
                        emcore_clean_openssl_library();
			goto FINISH_OFF;
		}
                emcore_clean_openssl_library();
	} else if (p_output_mail_data->smime_type == EMAIL_PGP_ENCRYPTED) {
		if ((err = emcore_pgp_get_decrypted_message(p_output_attachment_data[i].attachment_path, p_output_mail_data->pgp_password, false, &decrypt_filepath, verify)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_pgp_get_decrypted_message failed : [%d]", err);
			goto FINISH_OFF;
		}
	} else if (p_output_mail_data->smime_type == EMAIL_PGP_SIGNED_AND_ENCRYPTED) {
		if ((err = emcore_pgp_get_decrypted_message(p_output_attachment_data[i].attachment_path, p_output_mail_data->pgp_password, true, &decrypt_filepath, verify)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_pgp_get_decrypted_message failed : [%d]", err);
			goto FINISH_OFF;
		}
	} else {
		EM_DEBUG_LOG("Invalid encrypted mail");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* Change decrpyt_message to mail_data_t */
	if (!emcore_parse_mime_file_to_mail(decrypt_filepath, output_mail_data, output_attachment_data, output_attachment_count, &err)) {
		EM_DEBUG_EXCEPTION("emcore_parse_mime_file_to_mail failed : [%d]", err);
		goto FINISH_OFF;
	}

	(*output_mail_data)->subject                 = EM_SAFE_STRDUP(p_output_mail_data->subject);
	(*output_mail_data)->date_time               = p_output_mail_data->date_time;
	(*output_mail_data)->full_address_return     = EM_SAFE_STRDUP(p_output_mail_data->full_address_return);
	(*output_mail_data)->email_address_recipient = EM_SAFE_STRDUP(p_output_mail_data->email_address_recipient);
	(*output_mail_data)->email_address_sender    = EM_SAFE_STRDUP(p_output_mail_data->email_address_sender);
	(*output_mail_data)->full_address_reply      = EM_SAFE_STRDUP(p_output_mail_data->full_address_reply);
	(*output_mail_data)->full_address_from       = EM_SAFE_STRDUP(p_output_mail_data->full_address_from);
	(*output_mail_data)->full_address_to         = EM_SAFE_STRDUP(p_output_mail_data->full_address_to);
	(*output_mail_data)->full_address_cc         = EM_SAFE_STRDUP(p_output_mail_data->full_address_cc);
	(*output_mail_data)->full_address_bcc        = EM_SAFE_STRDUP(p_output_mail_data->full_address_bcc);
	(*output_mail_data)->flags_flagged_field     = p_output_mail_data->flags_flagged_field;

FINISH_OFF:

    EM_SAFE_FREE(decrypt_filepath);

	if (p_account_tbl)
		emstorage_free_account(&p_account_tbl, 1, NULL);

	if (p_output_mail_data)
		email_free_mail_data(&p_output_mail_data, 1);

	if (p_output_attachment_data)
		email_free_attachment_data(&p_output_attachment_data, p_output_attachment_count);

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_get_decrypt_message_ex(email_mail_data_t *input_mail_data,
											email_attachment_data_t *input_attachment_data,
											int input_attachment_count,
                                            email_mail_data_t **output_mail_data,
											email_attachment_data_t **output_attachment_data,
											int *output_attachment_count,
											int *verify)
{
	EM_DEBUG_API_BEGIN();
	int err = EMAIL_ERROR_NONE;
    int i = 0;
	char *decrypt_filepath = NULL;
    char *search = NULL;
    char *multi_user_name = NULL;
	emstorage_account_tbl_t *p_account_tbl = NULL;

	EM_IF_NULL_RETURN_VALUE(input_mail_data, EMAIL_ERROR_INVALID_PARAM);

	if (!output_mail_data || !output_attachment_data || !output_attachment_count) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (!emstorage_get_account_by_id(multi_user_name, input_mail_data->account_id, EMAIL_ACC_GET_OPT_OPTIONS, &p_account_tbl, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed : [%d]", err);
		goto FINISH_OFF;
	}

        for (i = 0; i < input_attachment_count; i++) {
                EM_DEBUG_LOG("mime_type : [%s]", input_attachment_data[i].attachment_mime_type);
                if (input_attachment_data[i].attachment_mime_type && (search = strcasestr(input_attachment_data[i].attachment_mime_type, "PKCS7-MIME"))) {
                        EM_DEBUG_LOG("Found the encrypt file");
                        break;
                } else if (input_attachment_data[i].attachment_mime_type && (search = strcasestr(input_attachment_data[i].attachment_mime_type, "octet-stream"))) {
			EM_DEBUG_LOG("Found the encrypt file");
			break;
		}
        }

        if (!search) {
                EM_DEBUG_EXCEPTION("No have a decrypt file");
                err = EMAIL_ERROR_INVALID_PARAM;
                goto FINISH_OFF;
        }

	if (input_mail_data->smime_type == EMAIL_SMIME_ENCRYPTED || input_mail_data->smime_type == EMAIL_SMIME_SIGNED_AND_ENCRYPTED) {
		emcore_init_openssl_library();
		if (!emcore_smime_get_decrypt_message(input_attachment_data[i].attachment_path, p_account_tbl->certificate_path, &decrypt_filepath, &err)) {
			EM_DEBUG_EXCEPTION("emcore_smime_get_decrypt_message failed");
			emcore_clean_openssl_library();
			goto FINISH_OFF;
		}
		emcore_clean_openssl_library();
	} else if (input_mail_data->smime_type == EMAIL_PGP_ENCRYPTED) {
		if ((err = emcore_pgp_get_decrypted_message(input_attachment_data[i].attachment_path, input_mail_data->pgp_password, false, &decrypt_filepath, verify)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_pgp_get_decrypted_message failed : [%d]", err);
			goto FINISH_OFF;
		}
	} else if (input_mail_data->smime_type == EMAIL_PGP_SIGNED_AND_ENCRYPTED) {
		if ((err = emcore_pgp_get_decrypted_message(input_attachment_data[i].attachment_path, input_mail_data->pgp_password, true, &decrypt_filepath, verify)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_pgp_get_decrypted_message failed : [%d]", err);
			goto FINISH_OFF;
		}
	} else {
		EM_DEBUG_LOG("Invalid encrypted mail");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* Change decrpyt_message to mail_data_t */
	if (!emcore_parse_mime_file_to_mail(decrypt_filepath, output_mail_data, output_attachment_data, output_attachment_count, &err)) {
		EM_DEBUG_EXCEPTION("emcore_parse_mime_file_to_mail failed : [%d]", err);
		goto FINISH_OFF;
	}

	(*output_mail_data)->subject                 = EM_SAFE_STRDUP(input_mail_data->subject);
	(*output_mail_data)->date_time               = input_mail_data->date_time;
	(*output_mail_data)->full_address_return     = EM_SAFE_STRDUP(input_mail_data->full_address_return);
	(*output_mail_data)->email_address_recipient = EM_SAFE_STRDUP(input_mail_data->email_address_recipient);
	(*output_mail_data)->email_address_sender    = EM_SAFE_STRDUP(input_mail_data->email_address_sender);
	(*output_mail_data)->full_address_reply      = EM_SAFE_STRDUP(input_mail_data->full_address_reply);
	(*output_mail_data)->full_address_from       = EM_SAFE_STRDUP(input_mail_data->full_address_from);
	(*output_mail_data)->full_address_to         = EM_SAFE_STRDUP(input_mail_data->full_address_to);
	(*output_mail_data)->full_address_cc         = EM_SAFE_STRDUP(input_mail_data->full_address_cc);
	(*output_mail_data)->full_address_bcc        = EM_SAFE_STRDUP(input_mail_data->full_address_bcc);
	(*output_mail_data)->flags_flagged_field     = input_mail_data->flags_flagged_field;

FINISH_OFF:

    EM_SAFE_FREE(decrypt_filepath);
    EM_SAFE_FREE(multi_user_name);

	if (p_account_tbl)
		emstorage_free_account(&p_account_tbl, 1, NULL);

	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_verify_signature(int mail_id, int *verify)
{
	EM_DEBUG_API_BEGIN("mail_id[%d]", mail_id);

	if (mail_id <= 0) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	int result_from_ipc = 0;
	int p_verify = 0;
	int err = EMAIL_ERROR_NONE;

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

	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_verify_signature_ex(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data, int input_attachment_count, int *verify)
{
	EM_DEBUG_API_BEGIN();

	if (!input_mail_data || !input_attachment_data || input_attachment_count <= 0) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	int count = 0;
	int err = EMAIL_ERROR_NONE;

	for (count = 0; count < input_attachment_count ; count++) {
		if (input_attachment_data[count].attachment_mime_type && strcasestr(input_attachment_data[count].attachment_mime_type, "SIGNATURE"))
			break;
	}

	if (count == input_attachment_count) {
		EM_DEBUG_LOG("No have the signed attachment");
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (input_mail_data->smime_type == EMAIL_SMIME_SIGNED) {
		emcore_init_openssl_library();
		if (!emcore_verify_signature(input_attachment_data[count].attachment_path, input_mail_data->file_path_mime_entity, verify, &err))
			EM_DEBUG_EXCEPTION("emcore_verify_signature failed : [%d]", err);

		emcore_clean_openssl_library();
	} else if (input_mail_data->smime_type == EMAIL_PGP_SIGNED) {
		if ((err = emcore_pgp_get_verify_signature(input_attachment_data[count].attachment_path, input_mail_data->file_path_mime_entity, input_mail_data->digest_type, verify)) != EMAIL_ERROR_NONE)
			EM_DEBUG_EXCEPTION("emcore_pgp_get_verify_siganture failed : [%d]", err);
	} else {
		EM_DEBUG_LOG("Invalid signed mail : mime_type[%d]", input_mail_data->smime_type);
		err = EMAIL_ERROR_INVALID_PARAM;
	}


	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

/*
EXPORT_API int email_check_ocsp_status(char *email_address, char *response_url, unsigned *handle)
{
	EM_DEBUG_FUNC_BEGIN_SEC("email_address : [%s], response_url : [%s], handle : [%p]", email_address, response_url, handle);

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

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	if (err == EMAIL_ERROR_NONE) {
		if (handle)
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
	}
}
*/
EXPORT_API int email_validate_certificate(int account_id, char *email_address, unsigned *handle)
{
	EM_DEBUG_API_BEGIN("account_id[%d]", account_id);
	EM_DEBUG_FUNC_BEGIN_SEC("account_id[%d] email_address[%s] handle[%p]", account_id, email_address, handle);

	EM_IF_NULL_RETURN_VALUE(account_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(email_address, EMAIL_ERROR_INVALID_PARAM);

	int err = EMAIL_ERROR_NONE;
	int as_handle = 0;
    char *multi_user_name = NULL;
	email_account_server_t account_server_type;
	ASNotiData as_noti_data;

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	if (em_get_account_server_type_by_account_id(multi_user_name, account_id, &account_server_type, false, &err) == false) {
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
	as_noti_data.validate_certificate.email_address = email_address;
    as_noti_data.validate_certificate.multi_user_name = multi_user_name;

	if (em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_VALIDATE_CERTIFICATE, &as_noti_data) == false) {
		EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed");
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if (handle)
		*handle = as_handle;

FINISH_OFF:

    EM_SAFE_FREE(multi_user_name);
	EM_DEBUG_API_END("err[%d]", err);
	return err;
}

EXPORT_API int email_get_resolve_recipients(int account_id, char *email_address, unsigned *handle)
{
	EM_DEBUG_API_BEGIN("account_id[%d]", account_id);
	EM_DEBUG_FUNC_BEGIN_SEC("account_id[%d] email_address[%s] handle[%p]", account_id, email_address, handle);

	EM_IF_NULL_RETURN_VALUE(account_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(email_address, EMAIL_ERROR_INVALID_PARAM);

	int err = EMAIL_ERROR_NONE;
	int as_handle = 0;
    char *multi_user_name = NULL;
	email_account_server_t account_server_type;
	ASNotiData as_noti_data;

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	if (em_get_account_server_type_by_account_id(multi_user_name, account_id, &account_server_type, false, &err) == false) {
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

	as_noti_data.get_resolve_recipients.handle          = as_handle;
	as_noti_data.get_resolve_recipients.account_id      = account_id;
	as_noti_data.get_resolve_recipients.email_address   = email_address;
    as_noti_data.get_resolve_recipients.multi_user_name = multi_user_name;

	if (em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_RESOLVE_RECIPIENT, &as_noti_data) == false) {
		EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed");
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if (handle)
		*handle = as_handle;

FINISH_OFF:

    EM_SAFE_FREE(multi_user_name);
	EM_DEBUG_API_END("err[%d]", err);
	return err;
}
