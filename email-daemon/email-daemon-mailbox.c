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
 * File: email-daemon-mailbox.c
 * Desc: email-daemon Mailbox Operation
 *
 * Auth:
 *
 * History:
 *    2006.08.16 : created
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "email-daemon.h"
#include "email-core-event.h"
#include "email-daemon-account.h"
#include "email-debug-log.h"
#include "email-core-mailbox.h"
#include "email-core-account.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-core-signal.h"
#include "email-utilities.h"

#ifdef __FEATURE_LOCAL_ACTIVITY__
extern int g_local_activity_run;
#endif

INTERNAL_FUNC int emdaemon_get_imap_mailbox_list(char *multi_user_name,
													int account_id,
													char* mailbox,
													int *handle,
													int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d] mailbox[%p] err_code[%p]", account_id, mailbox, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_event_t *event_data = NULL;

	if (account_id <= 0 || !mailbox)  {
		EM_DEBUG_EXCEPTION("account_id[%d], mailbox[%p]", account_id, mailbox);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	event_data = em_malloc(sizeof(email_event_t));
	if (event_data == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	event_data->type = EMAIL_EVENT_SYNC_IMAP_MAILBOX;
	event_data->account_id = account_id;
	event_data->event_param_data_3 = EM_SAFE_STRDUP(mailbox);
	event_data->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

	if (!emcore_insert_event(event_data, handle, &err)) {
		EM_DEBUG_EXCEPTION("emcore_insert_event failed [%d]", err);
		goto FINISH_OFF;
	}

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

INTERNAL_FUNC int emdaemon_get_mailbox_list(char *multi_user_name, int account_id, email_mailbox_t** mailbox_list, int* count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_list[%p], count[%p], err_code[%p]", account_id, mailbox_list, count, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_account_t* ref_account = NULL;

	if (account_id <= 0 || !mailbox_list || !count)  {
		EM_DEBUG_EXCEPTION("account_id[%d], mailbox_list[%p], count[%p]", account_id, mailbox_list, count);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, account_id, false);
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!emcore_get_mailbox_list(multi_user_name, account_id, mailbox_list, count, &err))  {
		EM_DEBUG_EXCEPTION("emcore_get_mailbox_list failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


INTERNAL_FUNC int emdaemon_get_mail_count_of_mailbox(char *multi_user_name, email_mailbox_t* mailbox, int* total, int* unseen, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], total[%p], unseen[%p], err_code[%p]", mailbox, total, unseen, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_account_t* ref_account = NULL;

	if (!mailbox || !total || !unseen)  {
		EM_DEBUG_EXCEPTION("mailbox[%p], total[%p], unseen[%p]", mailbox, total, unseen);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, mailbox->account_id, false);
	if (ref_account == NULL)  {
		EM_DEBUG_EXCEPTION(" emcore_get_account_reference failed [%d]", mailbox->account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!emcore_get_mail_count(multi_user_name, mailbox, total, unseen, &err))  {
		EM_DEBUG_EXCEPTION("emcore_get_mail_count failed [%d]", err);
		goto FINISH_OFF;
	}


	ret = true;

FINISH_OFF:

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_add_mailbox(char *multi_user_name, email_mailbox_t* new_mailbox, int on_server, int *handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("new_mailbox[%p], err_code[%p]", new_mailbox, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_account_t *ref_account = NULL;
	email_event_t   *event_data  = NULL;
	email_mailbox_t *mailbox     = NULL;

	if (!new_mailbox || new_mailbox->account_id <= 0 || !new_mailbox->mailbox_name) {
		if (new_mailbox != NULL)
			EM_DEBUG_EXCEPTION("new_mailbox->account_id[%d], new_mailbox->mailbox_name[%p]", new_mailbox->account_id, new_mailbox->mailbox_name);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, new_mailbox->account_id, false);
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", new_mailbox->account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/*  on_server is allowed to be only 0 when server_type is EMAIL_SERVER_TYPE_ACTIVE_SYNC */
	if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC)
		on_server = 0;

	if (on_server) {	/* async */
		event_data = em_malloc(sizeof(email_event_t));
		if (event_data == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		mailbox = em_malloc(sizeof(email_mailbox_t));
		if (mailbox == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		memcpy(mailbox, new_mailbox, sizeof(email_mailbox_t));
		mailbox->mailbox_name  = EM_SAFE_STRDUP(new_mailbox->mailbox_name);
		mailbox->alias         = EM_SAFE_STRDUP(new_mailbox->alias);

		if (new_mailbox->eas_data_length > 0 && new_mailbox->eas_data) {
			mailbox->eas_data = em_malloc(new_mailbox->eas_data_length);
			if (mailbox->eas_data == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed");
				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			memcpy(mailbox->eas_data, new_mailbox->eas_data, new_mailbox->eas_data_length);
		}

		event_data->type               = EMAIL_EVENT_CREATE_MAILBOX;
		event_data->account_id         = new_mailbox->account_id;
		event_data->event_param_data_1 = (char*)mailbox;
		event_data->event_param_data_4 = on_server;
		event_data->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

		if (!emcore_insert_event(event_data, (int*)handle, &err)) {
			EM_DEBUG_EXCEPTION("emcore_insert_event failed [%d]", err);
			goto FINISH_OFF;
		}
	} else {	/* sync */
		if (!emcore_create_mailbox(multi_user_name, new_mailbox, on_server, -1, -1, &err))  {
			EM_DEBUG_EXCEPTION("emcore_create failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	ret = true;

FINISH_OFF:

	if (ret == false) {
		if (event_data) {
			emcore_free_event(event_data);
			EM_SAFE_FREE(event_data);
		}
	}

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_set_mailbox_type(char *multi_user_name, int input_mailbox_id, email_mailbox_type_e input_mailbox_type)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id [%d], input_mailbox_type [%d]", input_mailbox_id, input_mailbox_type);

	/*  default variable */
	int err = EMAIL_ERROR_NONE;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;

	if (!input_mailbox_id)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &mailbox_tbl)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emstorage_update_mailbox_type(multi_user_name, mailbox_tbl->account_id, -1, input_mailbox_id, input_mailbox_type, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_update_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}


FINISH_OFF:

	if (mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emdaemon_set_local_mailbox(char *multi_user_name, int input_mailbox_id, int input_is_local_mailbox)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id [%d], input_mailbox_type [%d]", input_mailbox_id, input_is_local_mailbox);

	/*  default variable */
	int err = EMAIL_ERROR_NONE;

	if (input_mailbox_id <= 0)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emstorage_set_local_mailbox(multi_user_name, input_mailbox_id, input_is_local_mailbox, true)) != EMAIL_ERROR_NONE)  {
		EM_DEBUG_EXCEPTION("emstorage_set_local_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}


FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emdaemon_delete_mailbox(char *multi_user_name, int input_mailbox_id, int on_server, int *handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d], err_code[%p]", input_mailbox_id, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int recursive = 1;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;
	email_account_t *ref_account = NULL;
	email_event_t *event_data = NULL;

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &mailbox_tbl)) != EMAIL_ERROR_NONE || !mailbox_tbl) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", err);
		goto FINISH_OFF;
	}

	if (!input_mailbox_id || mailbox_tbl->account_id <= 0) {
		if (input_mailbox_id != 0)
			EM_DEBUG_EXCEPTION("mailbox_tbl->account_id[%d]", mailbox_tbl->account_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, mailbox_tbl->account_id, false);
	if (!ref_account) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", mailbox_tbl->account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/*  on_server is allowed to be only 0 when server_type is EMAIL_SERVER_TYPE_ACTIVE_SYNC */
	if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
		on_server = 0;
		recursive = 0;
	}

	if (on_server) {	/*  async */
		event_data = em_malloc(sizeof(email_event_t));
		if (event_data == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		event_data->type = EMAIL_EVENT_DELETE_MAILBOX;
		event_data->account_id = mailbox_tbl->account_id;
		event_data->event_param_data_4 = input_mailbox_id;
		event_data->event_param_data_5 = on_server;
		event_data->event_param_data_6 = recursive;
		event_data->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

		if (!emcore_insert_event(event_data, (int*)handle, &err)) {
			EM_DEBUG_EXCEPTION("emcore_insert_event failed [%d]", err);
			goto FINISH_OFF;
		}
	} else {
		if (!emcore_delete_mailbox(multi_user_name, input_mailbox_id, on_server, recursive))  {
			EM_DEBUG_EXCEPTION("emcore_delete failed [%d]", err);
			goto FINISH_OFF;
		}
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

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_delete_mailbox_all(char *multi_user_name, email_mailbox_t* mailbox, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("malibox[%p], err_code[%p]", mailbox, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_account_t* ref_account = NULL;

	if (!mailbox)  {
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (mailbox->account_id <= 0)  {
		EM_DEBUG_EXCEPTION("malibox->account_id[%d]", mailbox->account_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, mailbox->account_id, false);
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", mailbox->account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!emcore_delete_mailbox_all(multi_user_name, mailbox, &err))  {
		EM_DEBUG_EXCEPTION("emcore_delete_all failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


INTERNAL_FUNC int emdaemon_sync_header(char *multi_user_name, int input_account_id, int input_mailbox_id, int *handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id[%d], input_mailbox_id[%d], handle[%p], err_code[%p]", input_account_id, input_mailbox_id, handle, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_event_t *event_data = NULL;

	if (input_mailbox_id < 0 || handle == NULL) {
		EM_DEBUG_EXCEPTION("parameter is invalid");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	event_data = em_malloc(sizeof(email_event_t));
	if (event_data == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (input_account_id == ALL_ACCOUNT) {
		EM_DEBUG_LOG(">>>> emdaemon_sync_header for all account event_data.event_param_data_4 [%d]", event_data->event_param_data_4);
		event_data->type               = EMAIL_EVENT_SYNC_HEADER;
		event_data->account_id         = input_account_id;
		event_data->event_param_data_5 = input_mailbox_id;
		event_data->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

		/* In case of Mailbox NULL, we need to set arg as EMAIL_SYNC_ALL_MAILBOX */
		if (input_mailbox_id == 0)
			event_data->event_param_data_4 = EMAIL_SYNC_ALL_MAILBOX;

		if (!emcore_insert_event(event_data, (int*)handle, &err)) {
			EM_DEBUG_EXCEPTION("emcore_insert_event falied [%d]", err);
			goto FINISH_OFF;
		}
	} else {
		/* Modified the code to sync all mailbox in a Single Event */
		event_data->type               = EMAIL_EVENT_SYNC_HEADER;
		event_data->account_id         = input_account_id;
		event_data->event_param_data_5 = input_mailbox_id;
		event_data->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

		/* In case of Mailbox NULL, we need to set arg as EMAIL_SYNC_ALL_MAILBOX */
		if (input_mailbox_id == 0)
			event_data->event_param_data_4 = EMAIL_SYNC_ALL_MAILBOX;
		EM_DEBUG_LOG(">>>> EVENT ARG [ %d ] ", event_data->event_param_data_4);

		if (!emcore_insert_event(event_data, (int*)handle, &err)) {
			EM_DEBUG_EXCEPTION("emcore_insert_event falied [%d]", err);
			goto FINISH_OFF;
		}
	}

	/* Due to fast response, event noti is moved here from worker_event_queue */
	char input_mailbox_id_str[10] = {0};
	snprintf(input_mailbox_id_str, 10, "%d ", input_mailbox_id);
	if (!emcore_notify_network_event(NOTI_DOWNLOAD_START, input_account_id, ((input_mailbox_id == 0) ? NULL : input_mailbox_id_str), *handle, 0))
		EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_DOWNLOAD_START] Failed >>>> ");

/*	if ((err = emcore_update_sync_status_of_account(multi_user_name, input_account_id, SET_TYPE_SET, SYNC_STATUS_SYNCING)) != EMAIL_ERROR_NONE)
		EM_DEBUG_EXCEPTION("emcore_update_sync_status_of_account failed [%d]", err);
*/


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

INTERNAL_FUNC int emdaemon_set_mail_slot_size_of_mailbox(char *multi_user_name, int account_id, int mailbox_id, int new_slot_size, int *handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mailbox_id[%d], handle[%p], err_code[%p]", account_id, mailbox_id, handle, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_event_t *event_data = NULL;

	if (handle == NULL) {
		EM_DEBUG_EXCEPTION("handle is required");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	event_data = em_malloc(sizeof(email_event_t));
	if (event_data == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	event_data->type = EMAIL_EVENT_SET_MAIL_SLOT_SIZE;
	event_data->account_id = account_id;
	event_data->event_param_data_4 = mailbox_id;
	event_data->event_param_data_5 = new_slot_size;
	event_data->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

	if (!emcore_insert_event(event_data, (int*)handle, &err))   {
		EM_DEBUG_EXCEPTION("emcore_insert_event falied [%d]", err);
		goto FINISH_OFF;
	}

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

INTERNAL_FUNC int emdaemon_rename_mailbox(char *multi_user_name, int input_mailbox_id, char *input_mailbox_path, char *input_mailbox_alias, void *input_eas_data, int input_eas_data_length, int input_on_server, int *output_handle)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d] input_mailbox_path[%p] input_mailbox_alias[%p] input_eas_data[%p] input_eas_data_length[%d] input_on_server[%d] output_handle[%p]", input_mailbox_id, input_mailbox_path, input_mailbox_alias, input_eas_data, input_eas_data_length, input_on_server, output_handle);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	emstorage_account_tbl_t *account_data = NULL;
	emstorage_mailbox_tbl_t *old_mailbox_data = NULL;
	email_event_t *event_data = NULL;

	if (input_mailbox_id <= 0 || output_handle == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (input_on_server) {
		if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &old_mailbox_data)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_get_account_by_id(multi_user_name, old_mailbox_data->account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account_data, true, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed [%d]", err);
			goto FINISH_OFF;
		}

		if (account_data->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
			event_data = em_malloc(sizeof(email_event_t));
			if (event_data == NULL) {
				EM_DEBUG_EXCEPTION("event_data failed");
				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			event_data->type               = EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER;
			event_data->event_param_data_1 = EM_SAFE_STRDUP(old_mailbox_data->mailbox_name);
			event_data->event_param_data_2 = EM_SAFE_STRDUP(input_mailbox_path);
			event_data->event_param_data_3 = EM_SAFE_STRDUP(input_mailbox_alias);
			event_data->event_param_data_4 = input_mailbox_id;
			event_data->account_id         = old_mailbox_data->account_id;
			event_data->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

			if (!emcore_insert_event(event_data, (int*)output_handle, &err)) {
				EM_DEBUG_EXCEPTION("emcore_insert_event falied [%d]", err);
				goto FINISH_OFF;
			}
		}
	} else {
		if ((err = emcore_rename_mailbox(multi_user_name, input_mailbox_id, input_mailbox_path, input_mailbox_alias, input_eas_data, input_eas_data_length, false, true, 0)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_rename_mailbox failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (old_mailbox_data)
		emstorage_free_mailbox(&old_mailbox_data, 1, NULL);

	if (account_data)
		emstorage_free_account(&account_data, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
