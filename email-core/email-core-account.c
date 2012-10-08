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
 * File :  email-core-account.c
 * Desc :  Account Management
 *
 * Auth :  Kyuho Jo
 *
 * History :
 *    2010.08.25  :  created
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <vconf.h>

#include "email-convert.h"
#include "email-types.h"
#include "email-daemon.h"
#include "email-debug-log.h"
#include "email-storage.h"
#include "email-network.h"
#include "email-utilities.h"
#include "email-core-utils.h"
#include "email-core-event.h"
#include "email-core-global.h"
#include "email-core-account.h"
#include "email-core-mailbox.h"
#include "email-core-imap-mailbox.h"

#ifdef __FEATURE_USING_ACCOUNT_SVC__
#include "account.h"
#endif /*  __FEATURE_USING_ACCOUNT_SVC__ */

char *g_default_mbox_alias[MAILBOX_COUNT] =
{
	EMAIL_INBOX_DISPLAY_NAME,
	EMAIL_DRAFTBOX_DISPLAY_NAME,
	EMAIL_OUTBOX_DISPLAY_NAME,
	EMAIL_SENTBOX_DISPLAY_NAME,
	EMAIL_TRASH_DISPLAY_NAME,
	EMAIL_SPAMBOX_DISPLAY_NAME,
};

char *g_default_mbox_name[MAILBOX_COUNT]  =
{
	EMAIL_INBOX_NAME,
	EMAIL_DRAFTBOX_NAME,
	EMAIL_OUTBOX_NAME,
	EMAIL_SENTBOX_NAME,
	EMAIL_TRASH_DISPLAY_NAME,
	EMAIL_SPAMBOX_NAME,
};

email_mailbox_type_e g_default_mbox_type[MAILBOX_COUNT] =
{
	EMAIL_MAILBOX_TYPE_INBOX,
	EMAIL_MAILBOX_TYPE_DRAFT,
	EMAIL_MAILBOX_TYPE_OUTBOX,
	EMAIL_MAILBOX_TYPE_SENTBOX,
	EMAIL_MAILBOX_TYPE_TRASH,
	EMAIL_MAILBOX_TYPE_SPAMBOX,
};

INTERNAL_FUNC email_account_t* emcore_get_account_reference(int account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d]", account_id);
	EM_PROFILE_BEGIN(profile_emcore_get_account_reference);
	email_account_list_t **p;

	if (account_id == NEW_ACCOUNT_ID)
		return emcore_get_new_account_reference();

	if (account_id > 0)  {
		p = &g_account_list;
		while (*p)  {
			if ((*p)->account->account_id == account_id)
				return ((*p)->account);
			p = &(*p)->next;
		}

		/*  refresh and check once agai */
		if (emcore_refresh_account_reference() == true) {
			p = &g_account_list;
			while (*p)  {
				if ((*p)->account->account_id == account_id)
					return ((*p)->account);

				p = &(*p)->next;
			}
		}
	}

	EM_PROFILE_END(profile_emcore_get_account_reference);
	EM_DEBUG_FUNC_END();
	return NULL;
}


INTERNAL_FUNC int emcore_validate_account_with_account_info(email_account_t *account, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account[%p], err_code[%p], incoming_server_address [%s]", account, err_code, account->incoming_server_address);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_session_t *session = NULL;
	SENDSTREAM *stream = NULL;
	MAILSTREAM *tmp_stream = NULL;

	if (!emcore_check_thread_status())  {
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	if (!emnetwork_check_network_status(&err))  {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("Network available");

	if (!emcore_check_thread_status())  {
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	if (!emcore_get_empty_session(&session))  {
		EM_DEBUG_EXCEPTION("emcore_get_empty_session failed...");
		err = EMAIL_ERROR_SESSION_NOT_FOUND;
		goto FINISH_OFF;
	}

#ifdef _SMTP_ACCOUNT_VALIDATION_
	/* validate connection for smt */
	EM_DEBUG_LOG("Validate connection for SMTP");

	if (!emcore_check_thread_status()) {
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
       if (!emcore_connect_to_remote_mailbox_with_account_info(account, (char *)ENCODED_PATH_SMTP, (void **)&stream, &err) || !stream)  {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed 1 - %d", err);
		if (EMAIL_ERROR_AUTHENTICATE == err || EMAIL_ERROR_LOGIN_FAILURE == err) {	/*  wrong password or etc */
			EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed  :  Login or Authentication fail 1- %d", err);
			goto FINISH_OFF;
		}

		if (account->outgoing_server_secure_connection == 0x01)	/*  0x01 == ss */ {
			/*  retry with tl */
			EM_DEBUG_LOG("Retry with TLS");
			account->outgoing_server_secure_connection = 0x02;	/*  0x02 == tl */
			if (!emcore_check_thread_status())  {
				err = EMAIL_ERROR_CANCELLED;
				goto FINISH_OFF;
			}

		    if (!emcore_connect_to_remote_mailbox_with_account_info(account, (char *)ENCODED_PATH_SMTP, (void **)&stream, &err) || !stream)  {
				EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed 2 - %d", err);
				if (EMAIL_ERROR_AUTHENTICATE == err || EMAIL_ERROR_LOGIN_FAILURE == err) {	/*  wrong password or etc */
					EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed  :  Login or Authentication fail 2 - %d", err);
				}
				else if (EMAIL_ERROR_CONNECTION_FAILURE != err) {
					err = EMAIL_ERROR_VALIDATE_ACCOUNT;
				}
				account->outgoing_server_secure_connection = 0x01;	/*  restore to the previous value */
				goto FINISH_OFF;
		    }

			if (!emcore_check_thread_status())  {
				err = EMAIL_ERROR_CANCELLED;
				goto FINISH_OFF;
			}

			/*  save outgoing_server_secure_connection = 0x02 (tls) to the d */
			if (!emstorage_update_account(account_id, (emstorage_account_tbl_t  *)account, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_update_account failed - %d", err);
				account->outgoing_server_secure_connection = 0x01;	/*  restore to the previous value */
				err = EMAIL_ERROR_VALIDATE_ACCOUNT;
				goto FINISH_OFF;
			}
		}
		else {
			if (EMAIL_ERROR_CONNECTION_FAILURE != err)
				err = EMAIL_ERROR_VALIDATE_ACCOUNT;
			goto FINISH_OFF;
		}
	}
#endif

	/* validate connection for pop3/ima */
	EM_DEBUG_LOG("Validate connection for POP3/IMAP4");
	if (EMAIL_ERROR_NONE == err) {
		if (!emcore_check_thread_status())  {
			err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

		 if (!emcore_connect_to_remote_mailbox_with_account_info(account, 0, (void **)&tmp_stream, &err) || !tmp_stream)
		 {
			EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed - %d", err);
			if (EMAIL_ERROR_AUTHENTICATE == err || EMAIL_ERROR_LOGIN_FAILURE == err) {	/*  wrong password or etc */
				EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed : Login or Authentication failed - %d", err);
			}
			else if (EMAIL_ERROR_CONNECTION_FAILURE != err) {
				/* err = EMAIL_ERROR_VALIDATE_ACCOUNT */
			}
			goto FINISH_OFF;
		}
	}

	if (!emcore_check_thread_status())  {
		if (!emcore_delete_account(account->account_id, NULL))
			EM_DEBUG_EXCEPTION("emdaemon_delete_account failed [%d]", account->account_id);
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (stream)
		smtp_close(stream);

	if (tmp_stream)
		emcore_close_mailbox(0 , tmp_stream);

	if (err_code != NULL)
		*err_code = err;

	emcore_clear_session(session);

	EM_DEBUG_FUNC_END();
	return ret;
}


INTERNAL_FUNC int emcore_validate_account(int account_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], err_code[%p]", account_id, err_code);

	int err = EMAIL_ERROR_NONE, ret = false;
	email_account_t *ref_account = NULL;


	if (account_id <= 0)
    {
		EM_DEBUG_EXCEPTION("account_id[%p]", account_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(account_id);

	if (ref_account && emcore_validate_account_with_account_info(ref_account, &err) == false) {
		EM_DEBUG_EXCEPTION("emcore_validate_account_with_account_info failed (%d)", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

INTERNAL_FUNC int emcore_delete_account(int account_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], err_code[%p]", account_id, err_code);

	/*  default variabl */
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (account_id < FIRST_ACCOUNT_ID)  {
		EM_DEBUG_EXCEPTION("account_id[%d]", account_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
#ifdef __FEATURE_LOCAL_ACTIVITY__
	/* Delete all local activities of previous account */
	emstorage_activity_tbl_t activity;
	memset(&activity, 0x00, sizeof(emstorage_activity_tbl_t));
	activity.account_id = account_id;

	if (!emcore_delete_activity(&activity, &err)) {
		EM_DEBUG_LOG("\t emcore_delete_activity failed - %d", err);

		goto FINISH_OFF;
	}
#endif

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
	if (false == emcore_clear_partial_body_thd_event_que(&err))
		EM_DEBUG_EXCEPTION(" emcore_clear_partial_body_thd_event_que [%d]", err);

	if (false == emstorage_delete_full_pbd_activity_data(account_id, true, &err))
		EM_DEBUG_EXCEPTION("emstorage_delete_full_pbd_activity_data failed [%d]", err);

#endif

#ifdef __FEATURE_USING_ACCOUNT_SVC__
	{
		int error_code;
		email_account_t *account_to_be_deleted;

		account_to_be_deleted = emcore_get_account_reference(account_id);
		if (account_to_be_deleted && account_to_be_deleted->incoming_server_type != EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
			EM_DEBUG_LOG("Calling account_svc_delete with account_svc_id[%d]", account_to_be_deleted->account_svc_id);
			error_code = account_connect();
			EM_DEBUG_LOG("account_connect returns [%d]", error_code);
			error_code = account_delete_from_db_by_id(account_to_be_deleted->account_svc_id);
			EM_DEBUG_LOG("account_delete_from_db_by_id returns [%d]", error_code);
			error_code = account_disconnect();
			EM_DEBUG_LOG("account_disconnect returns [%d]", error_code);
		}
	}
#endif
	if (emcore_cancel_all_threads_of_an_account(account_id) < EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("There are some remaining jobs. I couldn't stop them.");
		err = EMAIL_ERROR_CANNOT_STOP_THREAD;
		goto FINISH_OFF;
	}

	/*  BEGIN TRANSACTION;		 */
	emstorage_begin_transaction(NULL, NULL, NULL);
	
	if (!emstorage_delete_account(account_id, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_delete_account failed [%d]", err);
		goto FINISH_OFF;
	}

#ifdef __FEATURE_KEEP_CONNECTION__
	/* emcore_reset_streams(); */
	emcore_remove_connection_info(account_id);
#endif
	
	if ((err = emcore_delete_all_mails_of_acount(account_id)) != EMAIL_ERROR_NONE)  {
		EM_DEBUG_EXCEPTION("emcore_delete_all_mails_of_acount failed [%d]", err);
		goto FINISH_OFF;
	}

	/*  delete all mailboxe */
	if (!emstorage_delete_mailbox(account_id, -1, 0, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_delete_mailbox failed - %d", err);
		goto FINISH_OFF;
	}

	/*  delete local imap sync mailbox from imap mailbox tabl */
	if (!emstorage_remove_downloaded_mail(account_id, NULL, NULL, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_remove_downloaded_mail failed - %d", err);
		goto FINISH_OFF;
	}

	emcore_check_unread_mail();
	emcore_delete_notification_by_account(account_id);
	emcore_refresh_account_reference();

	ret = true;

FINISH_OFF:
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
			ret = false;
		}
		if (!emstorage_notify_storage_event(NOTI_ACCOUNT_DELETE, account_id, 0, NULL, 0))
			EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event[ NOTI_ACCOUNT_DELETE] : Notification Failed >>> ");

	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (emstorage_rollback_transaction(NULL, NULL, NULL) == false)
			err = EMAIL_ERROR_DB_FAILURE;
		if (!emstorage_notify_storage_event(NOTI_ACCOUNT_DELETE_FAIL, account_id, err, NULL, 0))
			EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event[ NOTI_ACCOUNT_DELETE] : Notification Failed >>> ");
	}

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

INTERNAL_FUNC int emcore_create_account(email_account_t *account, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account[%p], err_code[%p]", account, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int i, count = 0, is_preset_IMAP_account = false;
	email_mailbox_t local_mailbox = {0};
	emstorage_account_tbl_t *temp_account_tbl = NULL;

	if (!account)  {
		EM_DEBUG_EXCEPTION("account[%p]", account);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_get_account_count(&count, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_account_count failed - %d", err);
		goto FINISH_OFF;
	}


	if (count >= EMAIL_ACCOUNT_MAX)  {
		EM_DEBUG_EXCEPTION("too many accounts...");
		err = EMAIL_ERROR_ACCOUNT_MAX_COUNT;
		goto FINISH_OFF;
	}

	account->account_id = 0;

	/* Temporarily code - begin */
	if (account->auto_download_size == 0) {
		account->auto_download_size = PARTIAL_BODY_SIZE_IN_BYTES;
		EM_DEBUG_LOG("account->auto_download_size [%d]", account->auto_download_size);
	}

	if (account->default_mail_slot_size == 0) {
		account->default_mail_slot_size = 50;
		EM_DEBUG_LOG("account->default_mail_slot_size [%d]", account->default_mail_slot_size);
	}
	/* Temporarily code - end */

	/* check for email address validation */
	EM_DEBUG_LOG("account->user_email_address[%s]", account->user_email_address);
	if (account->user_email_address) {
		if (!em_verify_email_address(account->user_email_address, true, &err)) {
			err = EMAIL_ERROR_INVALID_ADDRESS;
			EM_DEBUG_EXCEPTION("Invalid Email Address");
			goto FINISH_OFF;
		}
	}

	temp_account_tbl = em_malloc(sizeof(emstorage_account_tbl_t));
	if (!temp_account_tbl) {
		EM_DEBUG_EXCEPTION("allocation failed [%d]", err);
		goto FINISH_OFF;
	}
	em_convert_account_to_account_tbl(account, temp_account_tbl);

	if (!emstorage_add_account(temp_account_tbl, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_add_account failed - %d", err);
		goto FINISH_OFF;
	}
	account->account_id = temp_account_tbl->account_id;
	is_preset_IMAP_account = ((account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4)) ? true : false;/*  && (account->is_preset_account)) ? true  :  false */

	EM_DEBUG_LOG("is_preset_IMAP_account  :  %d", is_preset_IMAP_account);

	if ((account->incoming_server_type != EMAIL_SERVER_TYPE_ACTIVE_SYNC)	&& (!is_preset_IMAP_account)) {
		/* 1. create default local mailbox
		*    (Inbox, Draft, Outbox, Sentbox) */
		for (i = 0; i < MAILBOX_COUNT; i++) {
	  	EM_DEBUG_LOG("g_default_mbox_name [%d/%d] is [%s]", i, MAILBOX_COUNT, g_default_mbox_name[i]);
			local_mailbox.account_id = temp_account_tbl->account_id;
			local_mailbox.mailbox_name  = g_default_mbox_name[i];
			local_mailbox.mailbox_type	= g_default_mbox_type[i];
			if (local_mailbox.mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) {
				local_mailbox.local = EMAIL_MAILBOX_FROM_SERVER;
			}
			else {
				local_mailbox.local = EMAIL_MAILBOX_FROM_LOCAL;
			}
			local_mailbox.alias = g_default_mbox_alias[i];
			emcore_get_default_mail_slot_count(&local_mailbox.mail_slot_size, NULL);

			if (!emcore_create_mailbox(&local_mailbox, 0, &err))  {
				EM_DEBUG_EXCEPTION("emcore_create failed - %d", err);
				goto FINISH_OFF;
			}
			
		}
	}
	
	
	
	ret = true;
	
FINISH_OFF: 
	if (temp_account_tbl)
		emstorage_free_account(&temp_account_tbl, 1, NULL);
	
	if (ret == false && account != NULL)  {
		if (!emcore_delete_account(account->account_id, NULL))
			EM_DEBUG_EXCEPTION("emdaemon_delete_account Failed [%d]", account->account_id);
	}
	
	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("Return value [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emcore_init_account_reference()
{
	EM_DEBUG_FUNC_BEGIN();
	
	int err = EMAIL_ERROR_NONE;
	
	email_account_list_t *account_list = NULL;
	email_account_list_t **p = NULL;
	email_account_t *account = NULL;
	emstorage_account_tbl_t *account_tbl_array = NULL;
	int count = 0;		
	int i = 0;
	
	if (!g_account_retrieved)  {
		count = 1000;
		if (!emstorage_get_account_list(&count, &account_tbl_array, true, true, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_account_list failed [%d]", err);
			goto FINISH_OFF;
		}
		
		for (p = &account_list, i = 0; i < count; i++)  {
			account = em_malloc(sizeof(email_account_t));
			if (!account)  {	
				EM_DEBUG_EXCEPTION("malloc failed...");
				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			em_convert_account_tbl_to_account(account_tbl_array + i, account);

			/* memcpy(account, accounts + i, sizeof(email_account_t)) */
			/* memset(accounts + i, 0x00, sizeof(email_account_t)) */
			
			*p = (email_account_list_t*) em_malloc(sizeof(email_account_list_t));
			if (!(*p))  {	
				EM_DEBUG_EXCEPTION("malloc failed...");
				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			
			(*p)->account = account;
			
			p = &(*p)->next;
		}
		if (g_account_num)
			emcore_free_account_reference();
		g_account_retrieved = 1;
		g_account_num = count;
		g_account_list = account_list;
	}
	
FINISH_OFF: 
	if (account_tbl_array)
		emstorage_free_account(&account_tbl_array, count, NULL);
	
	if (err != EMAIL_ERROR_NONE)  {
		g_account_list = account_list;
		emcore_free_account_reference();
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_refresh_account_reference()
{
	EM_DEBUG_FUNC_BEGIN();
	
	if (g_account_retrieved && g_account_num)
		emcore_free_account_reference();
	
	g_account_retrieved = 0;
	g_account_num = 0;
	g_account_list = NULL;
	
	if (emcore_init_account_reference() != EMAIL_ERROR_NONE)  {
		EM_DEBUG_EXCEPTION("emcore_init_account_reference failed...");
		return false;
	}
	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emcore_free_account_reference()
{
	EM_DEBUG_FUNC_BEGIN();
	
	email_account_list_t *p = g_account_list;
	email_account_list_t *p_next = NULL;
	while (p)  {
		emcore_free_account(p->account);
		EM_SAFE_FREE(p->account);
		
		p_next = p->next;
		EM_SAFE_FREE(p);
		p = p_next;
	}

	g_account_retrieved = 0;
	g_account_num = 0;
	g_account_list = NULL;
	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emcore_free_account_list(email_account_t **account_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_list[%p], count[%d], err_code[%p]", account_list, count, err_code);
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	
	if (count <= 0 || !account_list || !*account_list)  {
			err = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}
		
	email_account_t *p = *account_list;
	int i;
	for (i = 0; i < count; i++)
		emcore_free_account(p+i);
		
	EM_SAFE_FREE(p);
	*account_list = NULL;
	
	ret = true;
	
FINISH_OFF: 
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC void emcore_free_option(email_option_t *option)
{
	EM_SAFE_FREE(option->display_name_from);
	EM_SAFE_FREE(option->signature);
}


INTERNAL_FUNC void emcore_free_account(email_account_t *account)
{
	if(!account) return;

	EM_SAFE_FREE(account->account_name);
	EM_SAFE_FREE(account->incoming_server_address);
	EM_SAFE_FREE(account->user_email_address);
	EM_SAFE_FREE(account->incoming_server_user_name);
	EM_SAFE_FREE(account->incoming_server_password);
	EM_SAFE_FREE(account->outgoing_server_address);
	EM_SAFE_FREE(account->outgoing_server_user_name);
	EM_SAFE_FREE(account->outgoing_server_password);
	EM_SAFE_FREE(account->user_display_name);
	EM_SAFE_FREE(account->reply_to_address);
	EM_SAFE_FREE(account->return_address);
	EM_SAFE_FREE(account->logo_icon_path);
	EM_SAFE_FREE(account->certificate_path);
	EM_SAFE_FREE(account->user_data);
	account->user_data_length = 0;
	emcore_free_option(&account->options);


	EM_DEBUG_FUNC_END();
}


INTERNAL_FUNC int emcore_get_account_reference_list(email_account_t **account_list, int *count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_list[%p], count[%p], err_code[%p]", account_list, count, err_code);
	int i, countOfAccounts = 0;
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_account_t *accountRef;
	email_account_list_t *p;

	if (!account_list || !count)  {
		EM_DEBUG_EXCEPTION("account_list[%p], count[%p]", account_list, count);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	p = g_account_list;

	while (p) {
		countOfAccounts++;
		p = p->next;
	}
	
	EM_DEBUG_LOG("Result count[%d]", countOfAccounts);
	*count = countOfAccounts;

	if (countOfAccounts > 0) {
		*account_list = malloc(sizeof(email_account_t) * countOfAccounts);
		if (!*account_list)  {		
			EM_DEBUG_LOG("malloc failed...");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
	}

	p = g_account_list;
	for (i = 0; i < countOfAccounts; i++)  {
		accountRef = (*account_list) + i;
		memcpy(accountRef, p->account , sizeof(email_account_t));
		p = p->next;
	}

	for (i = 0; i < countOfAccounts; i++)  {
		accountRef = (*account_list) + i;
		EM_DEBUG_LOG("Result account id[%d], name[%s]", accountRef->account_id, accountRef->account_name);
	}

	ret = true;

FINISH_OFF: 
	if (ret == false) {
		if (account_list) /*  Warn! this is not *account_list. Just account_list */
			EM_SAFE_FREE(*account_list);
	}

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}
	

#ifdef __FEATURE_BACKUP_ACCOUNT__
#include <ss_manager.h>

static int append_data_into_buffer(char **target_buffer, int *target_buffer_lenth, char *input_data, int input_data_length, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN("target_buffer [%p], target_buffer_lenth [%p], input_data [%p], input_data_length[%d]", target_buffer, target_buffer_lenth, input_data, input_data_length);
	int local_error_code = EMAIL_ERROR_NONE, ret_code = false;

	if (!target_buffer || !target_buffer_lenth || !input_data) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		local_error_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (*target_buffer_lenth > 0 && input_data_length) {
		EM_DEBUG_LOG("*target_buffer_lenth [%d]", *target_buffer_lenth);
		*target_buffer = realloc(*target_buffer, (*target_buffer_lenth) + input_data_length);
		if (!*target_buffer) {
			EM_DEBUG_EXCEPTION("realloc failed");
			local_error_code = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		memcpy(*target_buffer + (*target_buffer_lenth), input_data, input_data_length);
		*target_buffer_lenth += input_data_length;
		EM_DEBUG_LOG("*target_buffer_lenth [%d] input_data_length [%d]", *target_buffer_lenth, input_data_length);
	}
	else {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		local_error_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ret_code = true;

FINISH_OFF: 

	if (error_code)
		*error_code = local_error_code;
	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);

	return ret_code;
}


static int emcore_write_account_into_buffer(char **target_buffer, int *target_buffer_lenth, emstorage_account_tbl_t *account_tbl_ptr, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN("target_buffer [%p], target_buffer_lenth [%p], account_tbl_ptr [%p], error_code [%p]", target_buffer, target_buffer_lenth, account_tbl_ptr, error_code);
	int local_error_code = EMAIL_ERROR_NONE, ret_code = false, stream_length = 0;
	email_account_t temp_account = {0};
	char *byte_stream = NULL;

	if (em_convert_account_tbl_to_account(account_tbl_ptr, &temp_account)) {
		byte_stream = em_convert_account_to_byte_stream(&temp_account, &stream_length);
		EM_DEBUG_LOG("stream_length [%d]", stream_length);
		/*  EM_DEBUG_LOG("incoming_server_password [%s]", temp_account->password) */

		if (byte_stream) {
			if (!append_data_into_buffer(target_buffer, target_buffer_lenth, (char *)&stream_length, sizeof(int), &local_error_code)) {
				EM_DEBUG_EXCEPTION("append_data_into_buffer failed");
				goto FINISH_OFF;
			}
			EM_DEBUG_LOG("append_data_into_buffer succeed for stream_length");

			if (!append_data_into_buffer(target_buffer, target_buffer_lenth, byte_stream, stream_length, &local_error_code)) {
				EM_DEBUG_EXCEPTION("append_data_into_buffer failed");
				goto FINISH_OFF;
			}
			EM_DEBUG_LOG("append_data_into_buffer succeed for byte_stream");
		}
	}
	else {
		EM_DEBUG_EXCEPTION("em_convert_account_tbl_to_account failed");
		local_error_code = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	ret_code = true;
FINISH_OFF: 
	emcore_free_account(&temp_account);
	if (error_code)
		*error_code = local_error_code;

	EM_SAFE_FREE(byte_stream);

	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);
	return ret_code;
}

INTERNAL_FUNC int emcore_backup_accounts(const char *file_path, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN("file_path [%s], error_code [%p]", file_path, error_code);
	int local_error_code = EMAIL_ERROR_NONE, local_error_code_2 = EMAIL_ERROR_NONE, ret_code = false;
	int select_num, i, target_buff_length = 0;
	char *target_buffer = NULL;
	emstorage_account_tbl_t *account_list = NULL;

	if (!file_path) {
		local_error_code = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		goto FINISH_OFF;	
	}

	select_num = 1000;
	
	if (!emstorage_get_account_list(&select_num, &account_list, true, true, &local_error_code)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_list failed [%d]", local_error_code);
		goto FINISH_OFF;	
	}
	
	EM_DEBUG_LOG("select_num [%d]", select_num);
	
	if (account_list) {
		target_buffer = em_malloc(sizeof(int));
		if (!target_buffer)  {
			EM_DEBUG_EXCEPTION("malloc failed");
			local_error_code = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		memcpy(target_buffer, (char *)&select_num, sizeof(int));
		target_buff_length = sizeof(int);

		for (i = 0; i < select_num; i++) {
			if (!emcore_write_account_into_buffer(&target_buffer, &target_buff_length, account_list + i, &local_error_code)) {
				EM_DEBUG_EXCEPTION("emcore_write_account_into_buffer failed [%d]", local_error_code);
				goto FINISH_OFF;	
			}
		}

		EM_DEBUG_LOG("target_buff_length [%d]", target_buff_length);

		ssm_delete_file(file_path, SSM_FLAG_SECRET_OPERATION, NULL);
		
		if (ssm_write_buffer(target_buffer, target_buff_length, file_path, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
			EM_DEBUG_EXCEPTION("ssm_write_buffer failed [%d]", local_error_code);
			local_error_code = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;	
		}
		
	}

	ret_code = true;	
FINISH_OFF: 

	EM_SAFE_FREE(target_buffer);
	if (account_list)
		emstorage_free_account(&account_list, select_num, &local_error_code_2);

	if (error_code)
		*error_code = local_error_code;

	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);
	return ret_code;
}

INTERNAL_FUNC int emcore_restore_accounts(const char *file_path, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN("file_path [%s], error_code [%p]", file_path, error_code);
	int local_error_code = EMAIL_ERROR_NONE, ret_code = false, buffer_length = 0, read_length = 0;
	int account_count = 0, i = 0, account_stream_length = 0;
	char *temp_buffer = NULL, *account_stream = NULL, *buffer_ptr = NULL;
	email_account_t temp_account = {0};
	email_account_t *account_list = NULL;

	ssm_file_info_t sfi;

	if (!file_path) {
		local_error_code = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		goto FINISH_OFF;	
	}

	if (emcore_get_account_reference_list(&account_list, &account_count, &ret_code)) {
		for (i = 0; i < account_count; i++) {
			if (account_list[i].incoming_server_type != EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
				if (!emcore_delete_account(account_list[i].account_id, &ret_code)) {
					local_error_code = EMAIL_ERROR_INVALID_ACCOUNT;
					EM_DEBUG_EXCEPTION("emcore_delete_account failed");
					goto FINISH_OFF;
				}
			}
		}
	}

	if (ssm_getinfo(file_path, &sfi, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION("ssm_getinfo() failed.");
		ret_code = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	buffer_length = sfi.originSize;
	EM_DEBUG_LOG("account buffer_length[%d]", buffer_length);
	if ((temp_buffer = (char *)em_malloc(buffer_length + 1)) == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed...");
		ret_code = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (ssm_read(file_path, temp_buffer, buffer_length, (size_t *)&read_length, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION("ssm_read() failed.");
		ret_code = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("read_length[%d]", read_length);

	if (buffer_length == read_length) {
		memcpy((void *)&account_count, temp_buffer, sizeof(int));
		buffer_ptr = temp_buffer + sizeof(int);

		EM_DEBUG_LOG("account_count[%d]", account_count);		

		for (i = 0; i < account_count; i++) {
			memcpy((void *)&account_stream_length, buffer_ptr, sizeof(int));
			buffer_ptr += sizeof(int);
			EM_DEBUG_LOG("account_stream_length [%d]", account_stream_length);
			if (account_stream_length) {
				account_stream = em_malloc(account_stream_length);
				if (!account_stream) {
					EM_DEBUG_EXCEPTION("em_malloc() failed.");
					ret_code = EMAIL_ERROR_OUT_OF_MEMORY ;
					goto FINISH_OFF;
				}
				memcpy(account_stream, buffer_ptr, account_stream_length);

				em_convert_byte_stream_to_account(account_stream, account_stream_length, &temp_account);
				EM_SAFE_FREE(account_stream);
			
				if (!emcore_create_account(&temp_account, &ret_code)) {
					EM_DEBUG_EXCEPTION("emcore_create_account() failed.");
					goto FINISH_OFF;
				}

				emcore_free_account(&temp_account);
			}
			buffer_ptr += account_stream_length;
			account_stream_length = 0;
		}
	} else {
		EM_DEBUG_EXCEPTION("ssm_read() failed.");
		ret_code = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
	ret_code = true;	
FINISH_OFF: 
	emcore_free_account(&temp_account);
	EM_SAFE_FREE(account_stream);
	EM_SAFE_FREE(temp_buffer);

	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);
	return ret_code;
}

#endif /*  __FEATURE_BACKUP_ACCOUNT_ */

INTERNAL_FUNC int emcore_query_server_info(const char* domain_name, email_server_info_t **result_server_info)
{
	EM_DEBUG_FUNC_BEGIN("domain_name [%s], result_server_info [%p]", domain_name, result_server_info);
	int ret_code = EMAIL_ERROR_NONE;
	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);
	return ret_code;
}


INTERNAL_FUNC int emcore_free_server_info(email_server_info_t **target_server_info)
{
	EM_DEBUG_FUNC_BEGIN("result_server_info [%p]", target_server_info);
	int i, ret_code = EMAIL_ERROR_NONE;
	email_server_info_t *server_info = NULL;

	if(target_server_info && *target_server_info) {
		server_info = *target_server_info;
		EM_SAFE_FREE(server_info->service_name);
		for(i = 0; i < server_info->protocol_conf_count; i++) {
			EM_SAFE_FREE(server_info->protocol_config_array[i].server_addr);
		}
		EM_SAFE_FREE(server_info->protocol_config_array);
		EM_SAFE_FREE(server_info);
	}
	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);
	return ret_code;	
}

INTERNAL_FUNC int emcore_save_default_account_id(int input_account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d]", input_account_id);
	int ret_code = EMAIL_ERROR_NONE, result_value = 0;

	result_value = vconf_set_int(VCONF_KEY_DEFAULT_ACCOUNT_ID, input_account_id);
	if (result_value < 0) {
		EM_DEBUG_EXCEPTION("vconf_set_int failed [%d]", result_value);
		ret_code = EMAIL_ERROR_SYSTEM_FAILURE;
	}

	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);
	return ret_code;	
}

static int _recover_from_invalid_default_account_id(int *output_account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%p]", output_account_id);
	int ret_code = EMAIL_ERROR_NONE;
	int account_count = 100;
	emstorage_account_tbl_t *result_account_list = NULL;

	if (output_account_id == NULL) {
		ret_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if(!emstorage_get_account_list(&account_count, &result_account_list, false, false, &ret_code) || !result_account_list) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_list() failed [%d]", ret_code);
		*output_account_id = 0;
		goto FINISH_OFF;
	}

	if (account_count > 0) {
		*output_account_id = result_account_list[0].account_id;
	}

	EM_DEBUG_LOG("output_account_id [%d]", *output_account_id);

FINISH_OFF:

	if (result_account_list)
		emstorage_free_account(&result_account_list, account_count, NULL);

	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);
	return ret_code;
}

INTERNAL_FUNC int emcore_load_default_account_id(int *output_account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%p]", output_account_id);
	int ret_code = EMAIL_ERROR_NONE;
	int result_value = 0;
	emstorage_account_tbl_t *result_account = NULL;
	
	if (output_account_id == NULL) {
		ret_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	result_value = vconf_get_int(VCONF_KEY_DEFAULT_ACCOUNT_ID, output_account_id);

	if (result_value < 0) {
		EM_DEBUG_EXCEPTION("vconf_get_int() failed [%d]", result_value);
		ret_code = EMAIL_ERROR_SYSTEM_FAILURE;
		*output_account_id = 0;
	}

	if (*output_account_id != 0) {
		if (!emstorage_get_account_by_id(*output_account_id, EMAIL_ACC_GET_OPT_DEFAULT, &result_account, false, &ret_code)) {
			EM_DEBUG_EXCEPTION("emstorage_get_account_by_id() failed [%d]", ret_code);
			if(ret_code == EMAIL_ERROR_ACCOUNT_NOT_FOUND)
				*output_account_id = 0;
			else
				goto FINISH_OFF;
		}
	}

	if (*output_account_id == 0) {
		if ( (ret_code = _recover_from_invalid_default_account_id(output_account_id)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("_recover_from_invalid_default_account() failed [%d]", ret_code);
			*output_account_id = 0;
		}
	}

FINISH_OFF:
	if (result_account)
		emstorage_free_account(&result_account, 1, NULL);
	
	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);
	return ret_code;	
}

INTERNAL_FUNC int emcore_recover_from_secured_storage_failure()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int account_count = 50;
	emstorage_account_tbl_t *temp_account_tbl_list = NULL;
	emstorage_account_tbl_t *temp_account_tbl      = NULL;

	if (!emstorage_get_account_list(&account_count, &temp_account_tbl_list, true, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_list failed [%d]", err);
		goto FINISH_OFF;
	}

	for (i = 0; i < account_count; i++) {
		if(!emstorage_get_account_by_id(temp_account_tbl_list[i].account_id, EMAIL_ACC_GET_OPT_DEFAULT | EMAIL_ACC_GET_OPT_PASSWORD, &temp_account_tbl, true, &err)) {
			if(err == EMAIL_ERROR_SECURED_STORAGE_FAILURE) {
				if(!emcore_delete_account(temp_account_tbl_list[i].account_id, &err)) {
					EM_DEBUG_EXCEPTION("emcore_delete_account failed [%d]", err);
					goto FINISH_OFF;
				}
			}
		}
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_update_sync_status_of_account(int input_account_id, email_set_type_t input_set_operator, int input_sync_status)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d], input_set_operator [%d], input_sync_status [%d]", input_account_id, input_set_operator, input_sync_status);
	int err = EMAIL_ERROR_NONE;
	emstorage_account_tbl_t *account_tbl_data = NULL;

	if (!emstorage_update_sync_status_of_account(input_account_id, input_set_operator, input_sync_status, true, &err))
		EM_DEBUG_EXCEPTION("emstorage_update_sync_status_of_account failed [%d]", err);

	if (account_tbl_data)
		emstorage_free_account(&account_tbl_data, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
