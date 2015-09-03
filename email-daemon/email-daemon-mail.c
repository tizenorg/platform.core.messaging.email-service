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



/******************************************************************************
 * File: email-daemon-mail.c
 * Desc: email-daemon Mail Operation
 *
 * Auth:
 *
 * History:
 *    2006.08.16 : created
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>

#include "email-internal-types.h"
#include "email-daemon.h"
#include "email-core-event.h"
#include "email-daemon-account.h"
#include "email-debug-log.h"
#include "email-storage.h"
#include "email-utilities.h"
#include "email-core-account.h"
#include "email-core-mail.h"
#include "email-core-mailbox.h"
#include "email-core-utils.h"
#include "email-core-smtp.h"
#include "email-core-timer.h"
#include "email-core-signal.h"

#ifdef __FEATURE_LOCAL_ACTIVITY__
extern int g_local_activity_run;
extern int g_save_local_activity_run;
#endif
static int _emdaemon_check_mail_id(char *multi_user_name, int mail_id, int* err_code);

INTERNAL_FUNC int emdaemon_send_mail(char *multi_user_name, int mail_id, int *handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], handle[%p], err_code[%p]", mail_id, handle, err_code);

	int ret = false, err = EMAIL_ERROR_NONE, err_2 = EMAIL_ERROR_NONE;
	int  result_handle = 0, account_id = 0;
	email_event_t *event_data = NULL;
	emstorage_mail_tbl_t* mail_table_data = NULL;
	emstorage_mailbox_tbl_t* local_mailbox = NULL;
	email_account_t* ref_account = NULL;
	int dst_mailbox_id = 0;

	if(mail_id <= 0) {
		EM_DEBUG_EXCEPTION("mail_id is not valid");
		err= EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if(!emstorage_get_mail_by_id(multi_user_name, mail_id, &mail_table_data, true, &err)) {
		EM_DEBUG_EXCEPTION("Failed to get mail by mail_id [%d]", err);
		goto FINISH_OFF;
	}

	if (!mail_table_data || mail_table_data->mailbox_id <= 0 || mail_table_data->account_id <= 0) {
		if (mail_table_data != NULL)
			EM_DEBUG_EXCEPTION(" mail_table_data->mailbox_id[%d], mail_table_data->account_id[%d]", mail_table_data->mailbox_id, mail_table_data->account_id);
		if (err_code)
			*err_code = EMAIL_ERROR_INVALID_MAILBOX;
		if(mail_table_data)
			emstorage_free_mail(&mail_table_data, 1, &err);
		return false;
	}

	account_id = mail_table_data->account_id;

	ref_account = emcore_get_account_reference(multi_user_name, account_id, false);
	if (!ref_account) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

#ifdef __FEATURE_MOVE_TO_OUTBOX_FIRST__
	if (!emstorage_get_mailbox_by_mailbox_type(multi_user_name, account_id, EMAIL_MAILBOX_TYPE_OUTBOX, &local_mailbox, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
		goto FINISH_OFF;
	}
	dst_mailbox_id = local_mailbox->mailbox_id;
	if ( mail_table_data->mailbox_id != dst_mailbox_id ) {
			/*  mail is moved to 'OUTBOX' first of all. */
		if (!emcore_move_mail(multi_user_name, &mail_id, 1, dst_mailbox_id, EMAIL_MOVED_AFTER_SENDING, 0, &err)) {
			EM_DEBUG_EXCEPTION("emcore_mail_move falied [%d]", err);
			goto FINISH_OFF;
		}
	}
#endif /* __FEATURE_MOVE_TO_OUTBOX_FIRST__ */

	if(!emcore_notify_network_event(NOTI_SEND_START, account_id, NULL, mail_id, 0))
		EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_SEND_START] Failed >>>> ");

	/* set EMAIL_MAIL_STATUS_SEND_WAIT status */
	if(!emstorage_set_field_of_mails_with_integer_value(multi_user_name, account_id, &mail_id, 1, "save_status", EMAIL_MAIL_STATUS_SEND_WAIT, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value [%d]",err);
		goto FINISH_OFF;
	}

	event_data = em_malloc(sizeof(email_event_t));
	if (event_data == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	event_data->type               = EMAIL_EVENT_SEND_MAIL;
	event_data->account_id         = account_id;
	event_data->event_param_data_4 = mail_id;
	event_data->event_param_data_5 = mail_table_data->mailbox_id;
	event_data->multi_user_name    = EM_SAFE_STRDUP(multi_user_name);

	if (!emcore_insert_event_for_sending_mails(event_data, &result_handle, &err)) {
		EM_DEBUG_EXCEPTION(" emcore_insert_event failed [%d]", err);
		goto FINISH_OFF;
	}

#ifdef __FEATURE_LOCAL_ACTIVITY__
			EM_DEBUG_LOG("Setting g_save_local_activity_run ");
			g_save_local_activity_run = 1;
#endif

	if ( handle )
		*handle = result_handle;

	ret = true;

FINISH_OFF:
	if (ret == false) {
		EM_DEBUG_EXCEPTION("emdaemon_send_mail failed [%d]", err);

		if(!emstorage_set_field_of_mails_with_integer_value(multi_user_name, account_id, &mail_id, 1, "save_status", EMAIL_MAIL_STATUS_SAVED, true, &err))
			EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value [%d]",err);

		if (event_data) {
			emcore_free_event(event_data);
			EM_SAFE_FREE(event_data);
		}
	}

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if(!emcore_add_transaction_info(mail_id , result_handle , &err_2))
		EM_DEBUG_EXCEPTION("emcore_add_transaction_info failed [%d]", err_2);

	if(local_mailbox)
		emstorage_free_mailbox(&local_mailbox, 1, NULL);

	if (err_code != NULL)
		*err_code = err;

	if(mail_table_data)
		emstorage_free_mail(&mail_table_data, 1, &err);

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_send_mail_saved(char *multi_user_name, int account_id, int *handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d],handle[%p], err_code[%p]", account_id, handle, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_event_t *event_data = NULL;
	email_account_t *ref_account = NULL;
	char *mailbox_name = NULL;

	if (account_id <= 0) {
		EM_DEBUG_EXCEPTION("account_id = %d", account_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, account_id, false);
	if (!ref_account) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mailbox_name_by_mailbox_type(multi_user_name, account_id,EMAIL_MAILBOX_TYPE_OUTBOX,&mailbox_name, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_name_by_mailbox_type failed [%d]", err);
		goto FINISH_OFF;
	}

	event_data = em_malloc(sizeof(email_event_t));
	if (event_data == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	event_data->type               = EMAIL_EVENT_SEND_MAIL_SAVED;
	event_data->account_id         = account_id;
	event_data->event_param_data_3 = EM_SAFE_STRDUP(mailbox_name);
	event_data->multi_user_name    = EM_SAFE_STRDUP(multi_user_name);

	if (!emcore_insert_event_for_sending_mails(event_data, (int *)handle, &err)) {
		EM_DEBUG_EXCEPTION(" emcore_insert_event failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	EM_SAFE_FREE(mailbox_name);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_add_mail(char *multi_user_name, 
                                        email_mail_data_t *input_mail_data, 
                                        email_attachment_data_t *input_attachment_data_list, 
                                        int input_attachment_count, 
                                        email_meeting_request_t *input_meeting_request, 
                                        int input_from_eas)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list[%p], input_attachment_count [%d], input_meeting_req [%p], input_from_eas[%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int handle = 0;
	email_event_t *event_data = NULL;
	email_account_t *ref_account = NULL;

	if (!input_mail_data || input_mail_data->account_id <= 0 ||
		( ((input_mail_data->report_status & EMAIL_MAIL_REPORT_MDN) != 0) && !input_mail_data->full_address_to))  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, input_mail_data->account_id, false);
	if (!ref_account)  {
		EM_DEBUG_LOG(" emcore_get_account_reference failed [%d]", input_mail_data->account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if ((err = emcore_add_mail(multi_user_name, input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas, false)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_add_mail failed [%d]", err);
		goto FINISH_OFF;
	}

#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
	if ( input_from_eas == 0 && ref_account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
		event_data = em_malloc(sizeof(email_event_t));
		if (event_data == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		event_data->type               = EMAIL_EVENT_SAVE_MAIL;
		event_data->account_id         = input_mail_data->account_id;
		event_data->event_param_data_4 = input_mail_data->mail_id;
		event_data->multi_user_name    = EM_SAFE_STRDUP(multi_user_name);

		if (!emcore_insert_event(event_data, &handle, &err)) {
			EM_DEBUG_EXCEPTION("emcore_insert_event failed [%d]", err);
			goto FINISH_OFF;
		}
	}
#endif

	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	EM_DEBUG_FUNC_END("err [%d]", err);

	return err;
}


INTERNAL_FUNC int emdaemon_add_meeting_request(char *multi_user_name, int account_id, int input_mailbox_id, email_meeting_request_t *meeting_req, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], input_mailbox_id[%d], meeting_req[%p], err_code[%p]", account_id, input_mailbox_id, meeting_req, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if ( account_id <= 0 || !meeting_req || meeting_req->mail_id <= 0 )  {
		if(meeting_req)
			EM_DEBUG_EXCEPTION("mail_id[%d]", meeting_req->mail_id);

		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emcore_add_meeting_request(multi_user_name, account_id, input_mailbox_id, meeting_req, &err))  {
		EM_DEBUG_EXCEPTION(" emcore_save_mail_meeting_request failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_download_body(char *multi_user_name, int account_id, int mail_id, int verbose, int with_attachment,  int *handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], verbose[%d], with_attachment[%d], handle[%p], err_code[%p]", account_id, mail_id, verbose, with_attachment,  handle, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_event_t *event_data = NULL;

	if (!_emdaemon_check_mail_id(multi_user_name, mail_id, &err)) {
		EM_DEBUG_EXCEPTION("_emdaemon_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

	event_data = em_malloc(sizeof(email_event_t));
	if (event_data == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	event_data->type = EMAIL_EVENT_DOWNLOAD_BODY;
	event_data->account_id = account_id;
	event_data->event_param_data_4 = mail_id;
	event_data->event_param_data_5 = (verbose << 1 | with_attachment);
	event_data->multi_user_name    = EM_SAFE_STRDUP(multi_user_name);

	if (!emcore_insert_event(event_data, (int*)handle, &err)) {
		EM_DEBUG_EXCEPTION("emcore_insert_event failed [%d]", err);
		err = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

#ifdef __FEATURE_LOCAL_ACTIVITY__
	EM_DEBUG_LOG("Setting g_local_activity_run ");
	g_local_activity_run = 1;
#endif
	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

int emdaemon_get_attachment(char *multi_user_name, int attachment_id, email_attachment_data_t** attachment, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_id[%d], attachment[%p], err_code[%p]", attachment_id, attachment, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (!attachment_id || !attachment)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], attachment_id[%d], attachment[%p]\n", attachment_id, attachment);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emcore_get_attachment_info(multi_user_name, attachment_id, attachment, &err) || !attachment)  {
		EM_DEBUG_EXCEPTION("emcore_get_attachment_info failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

int emdaemon_add_attachment(char *multi_user_name, int mail_id, email_attachment_data_t* attachment, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], attachment[%p], err_code[%p]", mail_id, attachment, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (!attachment)  {
		EM_DEBUG_EXCEPTION(" mailbox[%p], mail_id[%d], attachment[%p]", mail_id, attachment);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!_emdaemon_check_mail_id(multi_user_name, mail_id, &err))  {
		EM_DEBUG_EXCEPTION(" _emdaemon_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_add_attachment(multi_user_name, mail_id, attachment, &err))  {
		EM_DEBUG_EXCEPTION(" emcore_add_attachment failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

int emdaemon_delete_mail_attachment(char *multi_user_name, int attachment_id, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_id[%d], err_code[%p]", attachment_id, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (!attachment_id)  {
		EM_DEBUG_EXCEPTION("attachment_id[%d]", attachment_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emcore_delete_mail_attachment(multi_user_name, attachment_id, &err))  {
		EM_DEBUG_EXCEPTION(" emcore_delete_mail_attachment failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

INTERNAL_FUNC int emdaemon_download_attachment(char *multi_user_name, int account_id, int mail_id, int nth, int *handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], nth[%d], handle[%p], err_code[%p]", account_id, mail_id, nth, handle, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_event_t *event_data = NULL;

	if (!nth) {
		EM_DEBUG_EXCEPTION("nth[%p] is invalid", nth);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!_emdaemon_check_mail_id(multi_user_name, mail_id, &err)) {
		EM_DEBUG_EXCEPTION("_emdaemon_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

	event_data = em_malloc(sizeof(email_event_t));
	if (event_data == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	event_data->type = EMAIL_EVENT_DOWNLOAD_ATTACHMENT;
	event_data->account_id         = account_id;
	event_data->event_param_data_4 = mail_id;
	event_data->event_param_data_5 = nth;
	event_data->multi_user_name    = EM_SAFE_STRDUP(multi_user_name);

	if (!emcore_insert_event(event_data, (int*)handle, &err))  {
		EM_DEBUG_EXCEPTION("emcore_insert_event failed [%d]", err);
		goto FINISH_OFF;
	}

#ifdef __FEATURE_LOCAL_ACTIVITY__
	EM_DEBUG_LOG("Setting g_local_activity_run ");
	g_local_activity_run = 1;
#endif

	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

INTERNAL_FUNC int emdaemon_free_attachment_data(email_attachment_data_t** atch_data, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	return emcore_free_attachment_data(atch_data, 1, err_code);
}

void* thread_func_to_delete_mail(void *thread_argument)
{
	EM_DEBUG_FUNC_BEGIN();
	int  err = EMAIL_ERROR_NONE;
	int *mail_id_list = NULL;
	int  mail_id_count = 0;
	int  account_id = 0;
	int  from_server = 0;
	int  noti_param_2 = 0;
	int  handle = 0;
	char *multi_user_name = NULL;
	email_event_t *event_data = (email_event_t*)thread_argument;

	account_id         = event_data->account_id;
	mail_id_list       = (int*)event_data->event_param_data_3;
	mail_id_count      = event_data->event_param_data_4;
	from_server        = event_data->event_param_data_5;
	multi_user_name    = event_data->multi_user_name;

	if (!emcore_delete_mail(multi_user_name, account_id, mail_id_list, 
							mail_id_count, EMAIL_DELETE_LOCALLY, 
							EMAIL_DELETED_BY_COMMAND, noti_param_2, &err)) {
		EM_DEBUG_EXCEPTION("emcore_delete_mail failed [%d]", err);
		emcore_free_event(event_data); /* prevent 17922 */
		EM_SAFE_FREE(event_data);
		goto FINISH_OFF;
	}

	if (from_server == EMAIL_DELETE_LOCAL_AND_SERVER || from_server == EMAIL_DELETE_FROM_SERVER) {
		if (!emcore_insert_event(event_data, &handle, &err)) {
			EM_DEBUG_EXCEPTION("emcore_insert_event failed [%d]", err);
/*			if (from_server != EMAIL_DELETE_LOCAL_AND_SERVER && from_server != EMAIL_DELETE_FROM_SERVER) {
				EM_SAFE_FREE(event_data->event_param_data_3);
			} */
			emcore_free_event(event_data); /* prevent 17922 */
			EM_SAFE_FREE(event_data);
			goto FINISH_OFF;
		}
	}
	else {
		emcore_free_event(event_data); /* prevent 17922 */
		EM_SAFE_FREE(event_data);
	}

FINISH_OFF:
	/* Don't free event_data as if the data is destined to be passed to event handler through emcore_insert_event.*/

	EM_DEBUG_FUNC_END();
	return SUCCESS;
}

INTERNAL_FUNC int emdaemon_delete_mail(char *multi_user_name, int mailbox_id, int mail_ids[], int mail_ids_count, int from_server, int *handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_id[%d], mail_ids[%p], mail_ids_count[%d], from_server[%d], handle[%p], err_code[%p]", mailbox_id, mail_ids, mail_ids_count, from_server, handle, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int *p = NULL;
	int thread_error = 0;
	email_account_t *ref_account = NULL;
	email_event_t *event_data = NULL;
	emstorage_mailbox_tbl_t *mailbox_tbl_data = NULL;
	thread_t delete_thread;

	/* mailbox can be NULL for deleting thread mail. */
	if (mail_ids_count <= 0) {
		EM_DEBUG_EXCEPTION("mail_ids_count [%d]", mail_ids_count);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ( (err = emstorage_get_mailbox_by_id(multi_user_name, mailbox_id, &mailbox_tbl_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%err]", err);
		goto FINISH_OFF;
	}

	if ((p = em_malloc(sizeof(int) * mail_ids_count)) == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc for p failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memcpy(p, mail_ids, sizeof(int) * mail_ids_count);

	if ((event_data = em_malloc(sizeof(email_event_t)) ) == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc for event_data failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, mailbox_tbl_data->account_id, false);
	if (!ref_account) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed.");
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
		from_server = EMAIL_DELETE_LOCALLY;
	}

	event_data->type                   = EMAIL_EVENT_DELETE_MAIL;
	event_data->account_id             = mailbox_tbl_data->account_id;
	event_data->event_param_data_3     = (char*)p;
	event_data->event_param_data_4     = mail_ids_count;
	event_data->event_param_data_5     = from_server;
	event_data->multi_user_name        = EM_SAFE_STRDUP(multi_user_name);

	THREAD_CREATE(delete_thread, thread_func_to_delete_mail, (void*)event_data, thread_error);
	THREAD_DETACH(delete_thread); /* free resources used for new thread */
	ret = true;

FINISH_OFF:

	if (mailbox_tbl_data)
		emstorage_free_mailbox(&mailbox_tbl_data, 1, NULL);

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (err != EMAIL_ERROR_NONE || thread_error != 0) {
	    emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
		EM_SAFE_FREE(p);
	}

	if (err_code)
		*err_code = err;

	return ret;
}

int emdaemon_delete_mail_all(char *multi_user_name, int input_mailbox_id, int input_from_server, int *output_handle, int *output_err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d], input_from_server[%d], handle[%p], err_code[%p]", input_mailbox_id, input_from_server, output_handle, output_err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;
	email_event_t *event_data = NULL;

	if (!input_mailbox_id) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &mailbox_tbl) != EMAIL_ERROR_NONE)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if(!emcore_delete_all_mails_of_mailbox(multi_user_name, mailbox_tbl->account_id, input_mailbox_id, EMAIL_DELETE_LOCALLY, &err)) {
		EM_DEBUG_EXCEPTION("emcore_delete_all_mails_of_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}

#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
	if(input_from_server == EMAIL_DELETE_LOCAL_AND_SERVER || input_from_server == EMAIL_DELETE_FROM_SERVER) {
		event_data = em_malloc(sizeof(email_event_t));
		if (event_data == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		event_data->type               = EMAIL_EVENT_DELETE_MAIL_ALL;
		event_data->account_id         = mailbox_tbl->account_id;
		event_data->event_param_data_4 = input_mailbox_id;
		event_data->event_param_data_5 = input_from_server;
		event_data->multi_user_name    = EM_SAFE_STRDUP(multi_user_name);

		if (!emcore_insert_event(event_data, (int*)output_handle, &err)) {
			EM_DEBUG_EXCEPTION("emcore_insert_event falied [%d]", err);
			goto FINISH_OFF;
		}

#ifdef __FEATURE_LOCAL_ACTIVITY__
		int i, total = 0;
		DB_STMT search_handle = 0;
		int *mail_ids = NULL;
		emstorage_activity_tbl_t new_activity;
		int activityid = 0;

		if (false == emcore_get_next_activity_id(&activityid,&err)) {
			EM_DEBUG_EXCEPTION(" emcore_get_next_activity_id Failed - %d ", err);
		}

		if (!emstorage_mail_search_start(multi_user_name, NULL, mailbox->account_id, mailbox->mailbox_name, 0, &search_handle, &total, true, &err)) {
			EM_DEBUG_EXCEPTION(" emstorage_mail_search_start failed [%d]", err);
			goto FINISH_OFF;
		}

		mail_ids = em_malloc(sizeof(int) * total);
		if (mail_ids == NULL)  {
			EM_DEBUG_EXCEPTION(" mailloc failed...");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		for (i = 0; i < total; i++)  {
			if (!emstorage_mail_search_result(search_handle, RETRIEVE_ID, (void**)&mail_ids[i], true, &err))  {
				EM_DEBUG_EXCEPTION(" emstorage_mail_search_result failed [%d]", err);


				EM_SAFE_FREE(mail_ids);
				goto FINISH_OFF;
			}

			new_activity.activity_id	= activityid;
			new_activity.activity_type	= ACTIVITY_DELETEMAIL;
			new_activity.mail_id		= mail_ids[i];
			new_activity.server_mailid	= NULL;
			new_activity.src_mbox		= mailbox->mailbox_name;
			new_activity.dest_mbox		= NULL;
			new_activity.account_id 	= mailbox->account_id;

			if (! emcore_add_activity(&new_activity, &err))
				EM_DEBUG_EXCEPTION(" emcore_add_activity  Failed  - %d ", err);

		}

		EM_SAFE_FREE(mail_ids);

		EM_DEBUG_LOG("Setting g_local_activity_run ");
		g_local_activity_run = 1;
#endif
	}
#endif 	/*  __FEATURE_SYNC_CLIENT_TO_SERVER__ */

	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

	if (output_err_code)
		*output_err_code = err;

	return ret;
}

void* thread_func_to_move_mail(void *thread_argument)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int *mail_ids = NULL, mail_ids_count, noti_param_1, noti_param_2, err;
	int handle = 0;
	email_event_t *event_data = (email_event_t*)thread_argument;
	int dst_mailbox_id = 0;
	char *multi_user_name = NULL;

	if(!event_data) { /*prevent 53096*/
		EM_DEBUG_EXCEPTION("INVALID_PARMAETER");
		return NULL;
	}


/*	dst_mailbox_name   = (char*)event_data->event_param_data_1; */ /*prevent 33693*/
	mail_ids           = (int*)event_data->event_param_data_3;
	mail_ids_count     = event_data->event_param_data_4;
	dst_mailbox_id	   = event_data->event_param_data_5;
	noti_param_1       = event_data->event_param_data_6;
	noti_param_2       = event_data->event_param_data_7;
	multi_user_name    = event_data->multi_user_name;

	if (!emcore_move_mail(multi_user_name, mail_ids, mail_ids_count, dst_mailbox_id, noti_param_1, noti_param_2, &err)) {
		EM_DEBUG_EXCEPTION("emcore_mail_move failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_insert_event(event_data, (int*)&handle, &err)) {
		EM_DEBUG_EXCEPTION("emcore_insert_event failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	EM_DEBUG_FUNC_END();
	return SUCCESS;
}

INTERNAL_FUNC int emdaemon_move_mail_all_mails(char *multi_user_name, int src_mailbox_id, int dst_mailbox_id, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("src_mailbox_id[%d], dst_mailbox_id[%d], err_code[%p]", src_mailbox_id, dst_mailbox_id, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int select_num = 0;
	int *mails = NULL;
	int *p = NULL;
	int i = 0;
	int num = 0;
	int thread_error = 0;
	email_account_t *ref_account = NULL;
	email_mail_list_item_t *mail_list = NULL;
	email_event_t *event_data = NULL;
	thread_t move_thread;
	emstorage_mailbox_tbl_t *dst_mailbox_tbl = NULL;
	emstorage_mailbox_tbl_t *src_mailbox_tbl = NULL;

	if ( dst_mailbox_id <= 0|| src_mailbox_id <= 0)  {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, dst_mailbox_id, &dst_mailbox_tbl)) != EMAIL_ERROR_NONE || !dst_mailbox_tbl) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", err);
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, src_mailbox_id, &src_mailbox_tbl)) != EMAIL_ERROR_NONE || !src_mailbox_tbl) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", err);
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, dst_mailbox_tbl->account_id, false);
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", dst_mailbox_tbl->account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if(!emstorage_get_mail_list(multi_user_name, src_mailbox_tbl->account_id, src_mailbox_id, NULL, EMAIL_LIST_TYPE_NORMAL, -1, -1, 0, NULL, EMAIL_SORT_DATETIME_HIGH, false, &mail_list, &select_num, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_list failed");
		goto FINISH_OFF;
	}

	mails = malloc(sizeof(int) * select_num);

	if( !mails ) {
		EM_DEBUG_EXCEPTION("Malloc failed...!");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(mails, 0x00, sizeof(int) * select_num);

	for(i = 0 ; i < select_num ; i++) {
		if( mail_list[i].save_status != EMAIL_MAIL_STATUS_SENDING ) {
			mails[num] = mail_list[i].mail_id;
			num++;
		}
	}

	if( num <= 0) {
		EM_DEBUG_EXCEPTION("can't find avalable mails. num = %d", num);
		err = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	if ((event_data = em_malloc(sizeof(email_event_t)) ) == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc for event_data failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if ((p = em_malloc(sizeof(int) * num)) == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc for p failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memcpy(p, mails, sizeof(int) * num);

	event_data->account_id         = dst_mailbox_tbl->account_id;
	event_data->type               = EMAIL_EVENT_MOVE_MAIL;
	event_data->event_param_data_3 = (char*)p;
	event_data->event_param_data_4 = num;
	event_data->event_param_data_5 = dst_mailbox_id;
	event_data->event_param_data_8 = src_mailbox_id;
	event_data->multi_user_name    = EM_SAFE_STRDUP(multi_user_name);

#ifdef __FEATURE_LOCAL_ACTIVITY__
	int i = 0, activityid = 0;

	if (false == emcore_get_next_activity_id(&activityid,&err))
		EM_DEBUG_EXCEPTION(" emcore_get_next_activity_id Failed - %d ", err);

	for (i =0; i < event_data.event_param_data_4; i++) {
		emstorage_activity_tbl_t	new_activity;
		new_activity.activity_id = activityid;
		new_activity.activity_type = ACTIVITY_MOVEMAIL;
		new_activity.account_id    = event_data.account_id;
		new_activity.mail_id	   = mail_ids[i];
		new_activity.dest_mbox	   = event_data.event_param_data_1;
		new_activity.server_mailid = NULL;
		new_activity.src_mbox	   = event_data.event_param_data_2;

		if (!emcore_add_activity(&new_activity, &err))
			EM_DEBUG_EXCEPTION(" emcore_add_activity Failed - %d ", err);
	}
#endif /* __FEATURE_LOCAL_ACTIVITY__ */
	THREAD_CREATE(move_thread, thread_func_to_move_mail, (void*)event_data, thread_error);
	THREAD_DETACH(move_thread); /* free resources used for new thread */
	EM_DEBUG_LOG("thread_error [%d]", thread_error);
	ret = true;

FINISH_OFF:

#ifdef __FEATURE_LOCAL_ACTIVITY__
	EM_DEBUG_LOG("Setting g_local_activity_run ");
	g_local_activity_run = 1;
#endif /* __FEATURE_LOCAL_ACTIVITY__ */

	if (err_code)
		*err_code = err;

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (dst_mailbox_tbl)
		emstorage_free_mailbox(&dst_mailbox_tbl, 1, NULL);

	if (src_mailbox_tbl)
		emstorage_free_mailbox(&src_mailbox_tbl, 1, NULL);

	if(mail_list)
		EM_SAFE_FREE(mail_list);

	if(mails != NULL )
		EM_SAFE_FREE(mails);

    if (err != EMAIL_ERROR_NONE || thread_error != 0) {
        emcore_free_event(event_data);
        EM_SAFE_FREE(event_data);
        EM_SAFE_FREE(p);
    }

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emdaemon_move_mail(char *multi_user_name, int mail_ids[], int num, int dst_mailbox_id, int noti_param_1, int noti_param_2, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], num[%d], dst_mailbox_id[%d], err_code[%p]", mail_ids, num, dst_mailbox_id, err_code);

	/*  default variable */
	int mail_id = 0, *p = NULL, thread_error = 0;
	int ret = false, err = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t* mail_table_data = NULL;
	email_account_t* ref_account = NULL;
	email_event_t *event_data = NULL;
	thread_t move_thread;
	emstorage_mailbox_tbl_t *dest_mailbox_tbl = NULL;
	int src_mailbox_id = 0;

	if (num <= 0 || dst_mailbox_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, dst_mailbox_id, &dest_mailbox_tbl)) != EMAIL_ERROR_NONE || !dest_mailbox_tbl) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", err);
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, dest_mailbox_tbl->account_id, false);

	if (!ref_account) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", dest_mailbox_tbl->account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/* Getting source mailbox name */
	mail_id = mail_ids[0];

	if (!emstorage_get_mail_field_by_id(multi_user_name, mail_id, RETRIEVE_SUMMARY, &mail_table_data, true, &err) || !mail_table_data) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_field_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (src_mailbox_id == 0)
		src_mailbox_id = mail_table_data->mailbox_id;

	emstorage_free_mail(&mail_table_data, 1, NULL);

	if ((event_data = em_malloc(sizeof(email_event_t)) ) == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc for event_data failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if ((p = em_malloc(sizeof(int) * num)) == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc for p failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memcpy(p, mail_ids, sizeof(int) * num);

	event_data->account_id        = dest_mailbox_tbl->account_id;
	event_data->type               = EMAIL_EVENT_MOVE_MAIL;
	event_data->event_param_data_3 = (char*)p;
	event_data->event_param_data_4 = num;
	event_data->event_param_data_5 = dst_mailbox_id;
	event_data->event_param_data_8 = src_mailbox_id;
	event_data->event_param_data_6 = noti_param_1;
	event_data->event_param_data_7 = noti_param_2;
	event_data->multi_user_name    = EM_SAFE_STRDUP(multi_user_name);

#ifdef __FEATURE_LOCAL_ACTIVITY__
	int i = 0, activityid = 0;

	if (false == emcore_get_next_activity_id(&activityid,&err))
		EM_DEBUG_EXCEPTION(" emcore_get_next_activity_id Failed - %d ", err);

	for (i =0; i < event_data.event_param_data_4; i++) {
		emstorage_activity_tbl_t	new_activity;
		new_activity.activity_id = activityid;
		new_activity.activity_type = ACTIVITY_MOVEMAIL;
		new_activity.account_id    = event_data.account_id;
		new_activity.mail_id	   = mail_ids[i];
		new_activity.dest_mbox	   = event_data.event_param_data_1;
		new_activity.server_mailid = NULL;
		new_activity.src_mbox	   = event_data.event_param_data_2;

		if (!emcore_add_activity(&new_activity, &err))
			EM_DEBUG_EXCEPTION(" emcore_add_activity Failed - %d ", err);
	}
#endif /* __FEATURE_LOCAL_ACTIVITY__ */
	THREAD_CREATE(move_thread, thread_func_to_move_mail, (void*)event_data, thread_error);
	THREAD_DETACH(move_thread); /* free resources used for new thread */
	EM_DEBUG_LOG("thread_error [%d]", thread_error);
	ret = true;

FINISH_OFF:

#ifdef __FEATURE_LOCAL_ACTIVITY__
	EM_DEBUG_LOG("Setting g_local_activity_run ");
	g_local_activity_run = 1;
#endif /* __FEATURE_LOCAL_ACTIVITY__ */

	if (err_code)
		*err_code = err;

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (dest_mailbox_tbl)
		emstorage_free_mailbox(&dest_mailbox_tbl, 1, NULL);

    if (err != EMAIL_ERROR_NONE || thread_error != 0) {
        emcore_free_event(event_data);
        EM_SAFE_FREE(event_data);
        EM_SAFE_FREE(p);
    }

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emdaemon_set_flags_field(char *multi_user_name, int account_id, int mail_ids[], int num, email_flags_field_type field_type, int value, int onserver, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], num[%d], field_type [%d], value[%d], err_code[%p]", mail_ids, num, field_type, value, err_code); /*prevent 27460*/

	int ret = false, err = EMAIL_ERROR_NONE;
	emstorage_account_tbl_t *account_tbl = NULL;
	int *mail_id_array = NULL;
	email_event_t *event_data = NULL;
	int handle = 0;

	if(account_id <= 0 || !mail_ids || num <= 0) {
		err = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		goto FINISH_OFF;
	}

	if (!emstorage_get_account_by_id(multi_user_name, account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account_tbl, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id falled [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_set_flags_field(multi_user_name, account_id, mail_ids, num, field_type, value, &err))  {
		EM_DEBUG_EXCEPTION("emcore_set_flags_field falled [%d]", err);
		goto FINISH_OFF;
	}

	if( onserver && account_tbl->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4 ) {
		mail_id_array = em_malloc(sizeof(int) * num);

		if (mail_id_array == NULL)  {
			EM_DEBUG_EXCEPTION("em_malloc failed...");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		memcpy(mail_id_array, mail_ids, sizeof(int) * num);

		event_data = em_malloc(sizeof(email_event_t));
		if (event_data == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		event_data->type               = EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER;
		event_data->account_id         = account_id;
		event_data->event_param_data_1 = NULL;
		event_data->event_param_data_3 = (char*)mail_id_array;
		event_data->event_param_data_4 = num;
		event_data->event_param_data_5 = field_type;
		event_data->event_param_data_6 = value;
		event_data->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

		if (!emcore_insert_event(event_data, (int*)&handle, &err))  {
			EM_DEBUG_LOG("emcore_insert_event failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (account_tbl)
		emstorage_free_account(&account_tbl, 1, NULL);

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


INTERNAL_FUNC int emdaemon_update_mail(char *multi_user_name, email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t *input_meeting_request, int input_from_eas)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list[%p], input_attachment_count [%d], input_meeting_req [%p], input_from_eas[%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas);
	int err = EMAIL_ERROR_NONE;
	/*email_event_t *event_data = NULL;*/
	email_account_t *ref_account = NULL;

	if (!input_mail_data || input_mail_data->account_id <= 0 || input_mail_data->mail_id == 0 ||
		(((input_mail_data->report_status & EMAIL_MAIL_REPORT_MDN) != 0) && !input_mail_data->full_address_to))  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, input_mail_data->account_id, false);
	if (!ref_account)  {
		EM_DEBUG_LOG(" emcore_get_account_reference failed [%d]", input_mail_data->account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if ( (err = emcore_update_mail(multi_user_name, input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_update_mail failed [%d]", err);
		goto FINISH_OFF;
	}

#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
/*	if ( input_from_eas == 0) {
		event_data = em_malloc(sizeof(email_event_t));
		if (event_data == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		event_data->type               = EMAIL_EVENT_UPDATE_MAIL;
		event_data->account_id         = input_mail_data->account_id;
		event_data->event_param_data_1 = (char*)input_mail_data; 			// need to be duplicated, it is double freed
		event_data->event_param_data_2 = (char*)input_attachment_data_list;	// need to be duplicated, it is double freed
		event_data->event_param_data_3 = (char*)input_meeting_request;		// need to be duplicated, it is double freed
		event_data->event_param_data_4 = input_attachment_count;
		event_data->event_param_data_5 = input_from_eas;
		event_data->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

		if (!emcore_insert_event(event_data, &handle, &err))  {
			EM_DEBUG_EXCEPTION("emcore_insert_event_for_sending_mails failed [%d]", err);
			err = EMAIL_ERROR_NONE;
			goto FINISH_OFF;
		}
	}
*/
#endif

FINISH_OFF:

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


int _emdaemon_check_mail_id(char *multi_user_name, int mail_id, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], err_code[%p]", mail_id, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	emstorage_mail_tbl_t* mail = NULL;

	if (!emstorage_get_mail_field_by_id(multi_user_name, mail_id, RETRIEVE_SUMMARY, &mail, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_field_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (mail != NULL)
		emstorage_free_mail(&mail, 1, NULL);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

typedef struct {
        int   mail_id;
        char *multi_user_name;
} email_retry_info;

INTERNAL_FUNC int emdaemon_send_mail_retry(char *multi_user_name, int mail_id, int timeout_in_sec, int* err_code)
{
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	long nTimerValue = 0;
	email_retry_info *retry_info = NULL;

	if (!_emdaemon_check_mail_id(multi_user_name, mail_id, &err))  {
		EM_DEBUG_EXCEPTION("_emdaemon_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (timeout_in_sec == 0) {
		if(!emdaemon_send_mail(multi_user_name, mail_id, NULL, &err)) {
			EM_DEBUG_EXCEPTION("emdaemon_send_mail failed [%d]", err);
			goto FINISH_OFF;
		}
	}
	else if (timeout_in_sec > 0) {
		retry_info = em_malloc(sizeof(email_retry_info));
		if (retry_info == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		retry_info->mail_id = mail_id;
		retry_info->multi_user_name = EM_SAFE_STRDUP(multi_user_name);
		nTimerValue = timeout_in_sec * 1000;

		if (emcore_set_timer_ex(nTimerValue, (EMAIL_TIMER_CALLBACK) _OnMailSendRetryTimerCB, (void*)retry_info) <= 0) {
			EM_DEBUG_EXCEPTION("Failed to start timer");
			goto FINISH_OFF;
		}
	}

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;

	return ret;
}

INTERNAL_FUNC void _OnMailSendRetryTimerCB(void* data)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	email_retry_info *retry_info = NULL;

	if (!data) {
		EM_DEBUG_LOG("Invalid param");
		return;
	}

	retry_info = (email_retry_info *)data;

	if (!_emdaemon_check_mail_id(retry_info->multi_user_name, retry_info->mail_id, &err))  {
		EM_DEBUG_EXCEPTION("_emdaemon_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if(!emdaemon_send_mail(retry_info->multi_user_name, retry_info->mail_id, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emdaemon_send_mail failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	free(retry_info->multi_user_name);
	free(retry_info);

	EM_DEBUG_FUNC_END();
	return;
}

INTERNAL_FUNC int emdaemon_move_mail_thread_to_mailbox(char *multi_user_name, int thread_id, int target_mailbox_id, int move_always_flag, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("thread_id [%d], target_mailbox_id [%d], move_always_flag [%d], err_code [%p]", thread_id, target_mailbox_id, move_always_flag, err_code);
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int *mail_id_list = NULL, result_count = 0, i, mailbox_count = 0, account_id;
	email_mail_list_item_t *mail_list = NULL;
	email_mailbox_t *target_mailbox_list = NULL, *target_mailbox = NULL;
	char mailbox_id_param_string[10] = {0,};

	if (target_mailbox_id == 0) {
		EM_DEBUG_EXCEPTION("target_mailbox_id [%d]", target_mailbox_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mail_list(multi_user_name, 0, 0, NULL, thread_id, -1, -1, 0, NULL, EMAIL_SORT_DATETIME_HIGH, true, &mail_list, &result_count, &err) || !mail_list || !result_count) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_list failed [%d]", err);

		goto FINISH_OFF;
	}

	mail_id_list = em_malloc(sizeof(int) * result_count);

	if (mail_id_list == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for(i = 0; i < result_count; i++) {
		mail_id_list[i] = mail_list[i].mail_id;
	}
	account_id = mail_list[0].account_id;


	if (!emcore_get_mailbox_list(multi_user_name, account_id, &target_mailbox_list, &mailbox_count, &err)) {
		EM_DEBUG_EXCEPTION("emcore_get_mailbox_list failed [%d]", err);
		goto FINISH_OFF;
	}

	for(i = 0; i < mailbox_count; i++) {
		EM_DEBUG_LOG_SEC("%s %d", target_mailbox_list[i].mailbox_name, target_mailbox_id);
		if(target_mailbox_list[i].mailbox_id == target_mailbox_id) {
			target_mailbox = (target_mailbox_list + i);
			break;
		}
	}

	if(!target_mailbox) {
		EM_DEBUG_EXCEPTION("couldn't find proper target mailbox.");
		goto FINISH_OFF;
	}

/*
	if (!emdaemon_move_mail(multi_user_name, mail_id_list, result_count, target_mailbox->mailbox_id, EMAIL_MOVED_BY_MOVING_THREAD, move_always_flag, &err)) {
		EM_DEBUG_EXCEPTION("emdaemon_move_mail failed [%d]", err);
		goto FINISH_OFF;
	}
*/

	SNPRINTF(mailbox_id_param_string, 10, "%d", target_mailbox->mailbox_id);
	if (!emcore_notify_storage_event(NOTI_THREAD_MOVE, account_id, thread_id, mailbox_id_param_string, move_always_flag))
		EM_DEBUG_EXCEPTION(" emcore_notify_storage_event Failed [NOTI_MAIL_MOVE] >>>> ");

	ret = true;

FINISH_OFF:
	emcore_free_mailbox_list(&target_mailbox_list, mailbox_count);
	EM_SAFE_FREE(mail_list);
	EM_SAFE_FREE(mail_id_list);

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_delete_mail_thread(char *multi_user_name, int thread_id, int delete_always_flag, int *handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("thread_id [%d], delete_always_flag [%d], err_code [%p]", thread_id, delete_always_flag, err_code);
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int account_id = 0;
	int mailbox_id, *mail_id_list = NULL, result_count = 0, i;
	email_mail_list_item_t *mail_list = NULL;

	if (!emstorage_get_mail_list(multi_user_name, 0, 0, NULL, thread_id, -1, -1, 0, NULL, EMAIL_SORT_MAILBOX_ID_HIGH, true, &mail_list, &result_count, &err) || !mail_list || !result_count) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_list failed [%d]", err);
		goto FINISH_OFF;
	}

	mail_id_list = em_malloc(sizeof(int) * result_count);

	if (mail_id_list == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for(i = 0; i < result_count; i++) {
		mail_id_list[i] = mail_list[i].mail_id;
	}

	account_id = mail_list[0].account_id;
	mailbox_id = mail_list[0].mailbox_id;

	// should remove requiring of mailbox information from this function.
	// email-service should find mailboxes itself by its mail id.
	if (!emdaemon_delete_mail(multi_user_name, mailbox_id, mail_id_list, result_count, false, handle, &err)) {
		EM_DEBUG_EXCEPTION("emdaemon_delete_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_notify_storage_event(NOTI_THREAD_DELETE, account_id, thread_id, NULL, delete_always_flag))
		EM_DEBUG_EXCEPTION(" emcore_notify_storage_event Failed [NOTI_THREAD_DELETE] >>>> ");

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE(mail_list);
	EM_SAFE_FREE(mail_id_list);

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_modify_seen_flag_of_thread(char *multi_user_name, int thread_id, int seen_flag, int on_server, int *handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("thread_id [%d], seen_flag [%d], on_server [%d], handle [%p], err_code [%p]", thread_id, seen_flag, on_server, handle, err_code);
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int account_id, *mail_id_list = NULL, result_count = 0, i;
	email_mail_list_item_t *mail_list = NULL;

	if (!emstorage_get_mail_list(multi_user_name, 0, 0, NULL, thread_id, -1, -1, 0, NULL, EMAIL_SORT_MAILBOX_ID_HIGH, true, &mail_list, &result_count, &err) || !mail_list || !result_count) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_list failed [%d]", err);
		goto FINISH_OFF;
	}

	mail_id_list = em_malloc(sizeof(int) * result_count);

	if (mail_id_list == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for(i = 0; i < result_count; i++) {
		mail_id_list[i] = mail_list[i].mail_id;
	}

	account_id = mail_list[0].account_id;

	if (!emdaemon_set_flags_field(multi_user_name, account_id, mail_id_list, result_count, EMAIL_FLAGS_SEEN_FIELD, seen_flag, on_server, &err)) {
		EM_DEBUG_EXCEPTION("emdaemon_set_flags_field failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_notify_storage_event(NOTI_THREAD_MODIFY_SEEN_FLAG, account_id, thread_id, NULL, seen_flag))
		EM_DEBUG_EXCEPTION(" emcore_notify_storage_event Failed [NOTI_MAIL_MOVE] >>>> ");

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(mail_list);
	EM_SAFE_FREE(mail_id_list);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_expunge_mails_deleted_flagged(char *multi_user_name, int input_mailbox_id, int input_on_server, int *output_handle)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id [%d], input_on_server [%d], output_handle [%p]", input_mailbox_id, input_on_server, output_handle);
	int err = EMAIL_ERROR_NONE;
	int handle = 0;
	int event_insert = false;
	email_event_t *event_data = NULL;
	email_account_t *ref_account = NULL;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;

	if (input_mailbox_id <= 0)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &mailbox_tbl)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, mailbox_tbl->account_id, false);
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", mailbox_tbl->account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
	if(input_on_server) {
		event_data = em_malloc(sizeof(email_event_t));
		if (event_data == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		event_data->type               = EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED;
		event_data->account_id         = mailbox_tbl->account_id;
		event_data->event_param_data_4 = input_mailbox_id;
		event_data->multi_user_name    = EM_SAFE_STRDUP(multi_user_name);

		if (!emcore_insert_event(event_data, &handle, &err)) {
			EM_DEBUG_EXCEPTION("emcore_insert_event failed [%d]", err);
			goto FINISH_OFF;
		}
		event_insert = true;
	}
	else
#endif
		if ( (err = emcore_expunge_mails_deleted_flagged_from_local_storage(multi_user_name, input_mailbox_id)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_expunge_mails_deleted_flagged_from_local_storage failed [%d]", err);
			goto FINISH_OFF;
		}

FINISH_OFF:

	if (event_insert == false && event_data) {
		EM_SAFE_FREE(event_data->multi_user_name);
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if(mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
