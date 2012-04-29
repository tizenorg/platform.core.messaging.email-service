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



/******************************************************************************
 * File: emf-etc.c
 * Desc: email-daemon Etc Implementations
 *
 * Auth:
 *
 * History:
 *    2006.08.16 : created
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vconf.h>
#include "email-daemon.h"
#include "email-daemon-account.h"
#include "email-debug-log.h"
#include "email-internal-types.h"
#include "email-core-event.h"
#include "email-core-utils.h"
#include "email-storage.h"


int emdaemon_register_event_callback(emf_action_t action, emf_event_callback callback, void* event_data)
{
	return emcore_register_event_callback(action, callback, event_data);
}

int emdaemon_unregister_event_callback(emf_action_t action, emf_event_callback callback)
{
	return emcore_unregister_event_callback(action, callback);
}

INTERNAL_FUNC void emdaemon_get_event_queue_status(int* on_sending, int* on_receiving)
{
	emcore_get_event_queue_status(on_sending, on_receiving);
}

INTERNAL_FUNC int emdaemon_get_pending_job(emf_action_t action, int account_id, int mail_id, emf_event_status_type_t* status)
{
	EM_DEBUG_FUNC_BEGIN("action[%d], account_id[%d], mail_id[%d]", action, account_id, mail_id);

	return emcore_get_pending_event(action, account_id, mail_id, status);
}

INTERNAL_FUNC int emdaemon_cancel_job(int account_id, int handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], handle[%d], err_code[%p]", account_id, handle, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	
	if (!emcore_cancel_thread(handle, NULL, &err))  {
		EM_DEBUG_EXCEPTION("emcore_cancel_thread failed [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


INTERNAL_FUNC int emdaemon_cancel_sending_mail_job(int account_id, int mail_id, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], err_code[%p]", account_id, mail_id, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;	
	int handle = 0;
	emf_account_t* ref_account = NULL;
	
	if (account_id <= 0)  {
		EM_DEBUG_EXCEPTION("account_id[%d], mail_id[%d]", account_id, mail_id);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

#ifdef __FEATURE_PROGRESS_IN_OUTBOX__

	/*	h.gahlaut@samsung.com: Moved this code from email_cancel_send_mail API to email-service engine
		since this code has update DB operation which is failing in context of email application process 
		with an sqlite error -> sqlite3_step fail:8 */
		
	/* 	which means #define SQLITE_READONLY   8 */  /* Attempt to write a readonly database */ 
	emstorage_mail_tbl_t *mail_tbl_data = NULL;

	if (!emstorage_get_mail_by_id(mail_id, &mail_tbl_data, false, &err))  {
		EM_DEBUG_EXCEPTION("emcore_get_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	if (mail_tbl_data) {
		if (mail_tbl_data->save_status == EMF_MAIL_STATUS_SEND_CANCELED) {
			EM_DEBUG_EXCEPTION(">>>> EMF_MAIL_STATUS_SEND_CANCELED Already set for Mail ID [ %d ]", mail_id);
			goto FINISH_OFF;
		}
		else {			
			mail_tbl_data->save_status = EMF_MAIL_STATUS_SEND_CANCELED;

			if(!emstorage_set_field_of_mails_with_integer_value(mail_tbl_data->account_id, &mail_id, 1, "save_status", EMF_MAIL_STATUS_SEND_CANCELED, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]",err);
				goto FINISH_OFF;
			}
		}
	}

#endif

	if(!emcore_get_handle_by_mailId_from_transaction_info(mail_id , &handle )) {
		EM_DEBUG_EXCEPTION("emcore_get_handle_by_mailId_from_transaction_info failed for mail_id[%d]", mail_id);
		err = EMF_ERROR_HANDLE_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (!(ref_account = emdaemon_get_account_reference(account_id)))  {
		EM_DEBUG_EXCEPTION("emdaemon_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	switch (ref_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE :
			if (!emcore_cancel_send_mail_thread(handle, NULL, &err))  {
				EM_DEBUG_EXCEPTION("emcore_cancel_send_mail_thread failed [%d]", err);
				goto FINISH_OFF;
			}
			break;

		default:
			EM_DEBUG_EXCEPTION("unknown account bind type...");
			err = EMF_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF;
	}	
	
	if(!emcore_delete_transaction_info_by_mailId(mail_id))
		EM_DEBUG_EXCEPTION("emcore_delete_transaction_info_by_mailId failed for mail_id[%d]", mail_id);
	
	ret = true;
	
FINISH_OFF:
	if(err_code != NULL)
		*err_code = err;
	
#ifdef __FEATURE_PROGRESS_IN_OUTBOX__
	if(!emstorage_free_mail(&mail_tbl_data, 1, &err))
		EM_DEBUG_EXCEPTION("emcore_free_mail Failed [%d ]", err);	

#endif
	EM_DEBUG_FUNC_END();
	return ret;
}	

INTERNAL_FUNC int emdaemon_search_mail_on_server(int input_account_id, const char *input_mailbox_name, email_search_filter_t *input_search_filter, int input_search_filter_count, unsigned int *output_handle)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d], input_mailbox_name [%p], input_search_filter [%p], input_search_filter_count [%d], output_handle [%p]", input_account_id, input_mailbox_name, input_search_filter, input_search_filter_count, output_handle);
	int error = EMF_ERROR_NONE;
	int i = 0;

	for(i = 0; i < input_search_filter_count; i++) {
		EM_DEBUG_LOG("search_filter_type [%d]", input_search_filter->search_filter_type);

		switch(input_search_filter->search_filter_type) {
			case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO       :
			case EMAIL_SEARCH_FILTER_TYPE_UID              :
			case EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER      :
			case EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED   :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED    :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT      :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN       :
				EM_DEBUG_LOG("integer_type_key_value [%d]", input_search_filter->search_filter_key_value.integer_type_key_value);
				break;

			case EMAIL_SEARCH_FILTER_TYPE_BCC              :
			case EMAIL_SEARCH_FILTER_TYPE_CC               :
			case EMAIL_SEARCH_FILTER_TYPE_FROM             :
			case EMAIL_SEARCH_FILTER_TYPE_KEYWORD          :
			case EMAIL_SEARCH_FILTER_TYPE_SUBJECT          :
			case EMAIL_SEARCH_FILTER_TYPE_TO               :
			case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID       :
				EM_DEBUG_LOG("string_type_key_value [%s]", input_search_filter->search_filter_key_value.string_type_key_value);
				break;

			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE :
			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON     :
			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE  :
				/* EM_DEBUG_LOG("time_type_key_value [%d]", input_search_filter->search_filter_key_value.time_type_key_value.); */
				break;
			default :
				EM_DEBUG_LOG("not proper search_filter_type [%d]", input_search_filter->search_filter_type);
				return FALSE;
		}
	}


	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int emdaemon_clear_all_mail_data(int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	
	if (emdaemon_initialize(&error)) {
		if (!emstorage_clear_mail_data(true, &error))
			EM_DEBUG_EXCEPTION("emstorage_clear_mail_data failed [%d]", error);
	}
	else {
		EM_DEBUG_EXCEPTION("emdaemon_initialize failed [%d]", error);
		if (err_code)
			*err_code = error;
		return false;
	}

	emcore_check_unread_mail();

	ret = true;

	if (!emstorage_create_table(EMF_CREATE_DB_NORMAL, &error)) 
		EM_DEBUG_EXCEPTION("emstorage_create_table failed [%d]", error);
	
	emdaemon_finalize(&error);
	
	if (err_code)
		*err_code = error;
	EM_DEBUG_FUNC_END();
    return ret;
}

	
/* --------------------------------------------------------------------------------*/
