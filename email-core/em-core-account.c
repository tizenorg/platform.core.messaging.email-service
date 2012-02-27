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
 * File :  em-core-account.c
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

#include "Msg_Convert.h"
#include "emf-types.h"
#include "emflib.h"
#include "emf-dbglog.h"
#include "em-storage.h"
#include "em-network.h"
#include "em-core-utils.h"
#include "em-core-event.h"
#include "em-core-global.h"
#include "em-core-account.h" 
#include "em-core-mailbox.h"
#include "em-core-imap-mailbox.h"

#ifdef __FEATURE_USING_MY_ACCOUNT__
#include "account.h"
#endif /*  FEATURE_USING_MY_ACCOUN */

char *g_default_mbox_alias[MAILBOX_COUNT] = 
{
	EMF_INBOX_DISPLAY_NAME, 
	EMF_DRAFTBOX_DISPLAY_NAME, 
	EMF_OUTBOX_DISPLAY_NAME, 
	EMF_SENTBOX_DISPLAY_NAME, 
	EMF_TRASH_DISPLAY_NAME, 
	EMF_SPAMBOX_DISPLAY_NAME, 
};

char *g_default_mbox_name[MAILBOX_COUNT]  = 
{
	EMF_INBOX_NAME, 
	EMF_DRAFTBOX_NAME, 
	EMF_OUTBOX_NAME, 
	EMF_SENTBOX_NAME, 
	EMF_TRASH_DISPLAY_NAME, 
	EMF_SPAMBOX_NAME, 
};

emf_mailbox_type_e g_default_mbox_type[MAILBOX_COUNT] = 
{
	EMF_MAILBOX_TYPE_INBOX, 
	EMF_MAILBOX_TYPE_DRAFT, 
	EMF_MAILBOX_TYPE_OUTBOX, 
	EMF_MAILBOX_TYPE_SENTBOX, 
	EMF_MAILBOX_TYPE_TRASH, 
	EMF_MAILBOX_TYPE_SPAMBOX, 		
};

EXPORT_API emf_account_t* em_core_get_account_reference(int account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d]", account_id);
	EM_PROFILE_BEGIN(profile_em_core_get_account_reference);
	emf_account_list_t **p;

	if (account_id == NEW_ACCOUNT_ID)
		return em_core_account_get_new_account_ref();

	if (account_id > 0)  {		
		p = &g_account_list;
		while (*p)  {
			if ((*p)->account->account_id == account_id)
				return ((*p)->account);
			p = &(*p)->next;
		}

		/*  refresh and check once agai */
		if (em_core_refresh_account_reference() == true) {
			p = &g_account_list;
			while (*p)  {
				if ((*p)->account->account_id == account_id)
					return ((*p)->account);
				
				p = &(*p)->next;
			}
		}
	}
 
	EM_PROFILE_END(profile_em_core_get_account_reference);
	EM_DEBUG_FUNC_END();
	return NULL;
}


EXPORT_API int em_core_account_validate_with_account_info(emf_account_t *account, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account[%p], err_code[%p], receiving_server_addr [%s]", account, err_code, account->receiving_server_addr);

	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_session_t *session = NULL;
	SENDSTREAM *stream = NULL;
	MAILSTREAM *tmp_stream = NULL;

	if (!em_core_check_thread_status())  {		
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	if (!em_core_check_network_status(&err))  {
		EM_DEBUG_EXCEPTION("em_core_check_network_status failed [%d]", err);
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("Network available");

	if (!em_core_check_thread_status())  {		
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	if (!em_core_get_empty_session(&session))  {		
		EM_DEBUG_EXCEPTION("em_core_get_empty_session failed...");
		err = EMF_ERROR_SESSION_NOT_FOUND;
		goto FINISH_OFF;
	}

#ifdef _SMTP_ACCOUNT_VALIDATION_	
	/* validate connection for smt */
	EM_DEBUG_LOG("     >>>>>>>>>> Validate connection for SMTP");

	if (!em_core_check_thread_status()) {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
       if (!em_core_mailbox_open_with_account_info(account, (char *)ENCODED_PATH_SMTP, (void **)&stream, &err) || !stream)  {
		EM_DEBUG_EXCEPTION("\t em_core_mailbox_open failed 1 - %d", err);
		if (EMF_ERROR_AUTHENTICATE == err || EMF_ERROR_LOGIN_FAILURE == err) {	/*  wrong password or etc */
			EM_DEBUG_EXCEPTION("\t em_core_mailbox_open failed  :  Login or Authentication fail 1- %d", err);
			goto FINISH_OFF;
		}

		if (account->sending_security == 0x01)	/*  0x01 == ss */ {
			/*  retry with tl */
			EM_DEBUG_LOG("     >>>>>>>>>> Retry with TLS");
			account->sending_security = 0x02;	/*  0x02 == tl */
			if (!em_core_check_thread_status())  {
				err = EMF_ERROR_CANCELLED;
				goto FINISH_OFF;
			}
			
		    if (!em_core_mailbox_open_with_account_info(account, (char *)ENCODED_PATH_SMTP, (void **)&stream, &err) || !stream)  {
				EM_DEBUG_EXCEPTION("\t em_core_mailbox_open failed 2 - %d", err);
				if (EMF_ERROR_AUTHENTICATE == err || EMF_ERROR_LOGIN_FAILURE == err) {	/*  wrong password or etc */
					EM_DEBUG_EXCEPTION("\t em_core_mailbox_open failed  :  Login or Authentication fail 2 - %d", err);
				}				
				else if (EMF_ERROR_CONNECTION_FAILURE != err) {
					err = EMF_ERROR_VALIDATE_ACCOUNT;
				}
				account->sending_security = 0x01;	/*  restore to the previous value */
				goto FINISH_OFF;
		    }

			if (!em_core_check_thread_status())  {
				err = EMF_ERROR_CANCELLED;
				goto FINISH_OFF;
			}

			/*  save sending_security = 0x02 (tls) to the d */
			if (!em_storage_update_account(account_id, (emf_mail_account_tbl_t  *)account, true, &err)) {
				EM_DEBUG_EXCEPTION("\t em_storage_update_account failed - %d", err);
				account->sending_security = 0x01;	/*  restore to the previous value */
				err = EMF_ERROR_VALIDATE_ACCOUNT;
				goto FINISH_OFF;
			}
		}
		else {
			if (EMF_ERROR_CONNECTION_FAILURE != err)
				err = EMF_ERROR_VALIDATE_ACCOUNT;
			goto FINISH_OFF;
		}
	}
#endif

	/* validate connection for pop3/ima */
	EM_DEBUG_LOG("Validate connection for POP3/IMAP4");
	if (EMF_ERROR_NONE == err) {
		if (!em_core_check_thread_status())  {
			err = EMF_ERROR_CANCELLED;
			goto FINISH_OFF;		
		}
		
		 if (!em_core_mailbox_open_with_account_info(account, NULL, (void **)&tmp_stream, &err) || !tmp_stream) 
		 {
			EM_DEBUG_EXCEPTION("em_core_mailbox_open failed - %d", err);
			if (EMF_ERROR_AUTHENTICATE == err || EMF_ERROR_LOGIN_FAILURE == err) {	/*  wrong password or etc */
				EM_DEBUG_EXCEPTION("em_core_mailbox_open failed  :  Login or Authentication fail - %d", err);
			}
			else if (EMF_ERROR_CONNECTION_FAILURE != err) {
				/* err = EMF_ERROR_VALIDATE_ACCOUNT */
			}
			goto FINISH_OFF;
		}			
	}
	
	if (!em_core_check_thread_status())  {
		if (!em_core_account_delete(account->account_id, NULL))
			EM_DEBUG_EXCEPTION("emf_account_delete failed [%d]", account->account_id);
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	ret = true;
	err = EMF_SUCCESS_VALIDATE_ACCOUNT;
	
FINISH_OFF: 
	if (stream)
		 smtp_close(stream);
	   	  
	if (tmp_stream)
		em_core_mailbox_close(0 , tmp_stream);

	if (err_code != NULL)
		*err_code = err;
	em_core_clear_session(session);

	EM_DEBUG_FUNC_END();
	
	return ret;
}


EXPORT_API int em_core_account_validate(int account_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], err_code[%p]", account_id, err_code);

	int err = EMF_ERROR_NONE, ret = false;
	emf_account_t *ref_account = NULL;


	if (account_id <= 0) 
    {
		EM_DEBUG_EXCEPTION("account_id[%p]", account_id);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ref_account = em_core_get_account_reference(account_id);

	if (ref_account && em_core_account_validate_with_account_info(ref_account, &err) == false) {
		EM_DEBUG_EXCEPTION("em_core_account_validate_with_account_info failed (%d)", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF: 

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

EXPORT_API int em_core_account_delete(int account_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], err_code[%p]", account_id, err_code);
	
	/*  default variabl */
	int ret = false;
	int err = EMF_ERROR_NONE;

	if (account_id < FIRST_ACCOUNT_ID)  {	
		EM_DEBUG_EXCEPTION("account_id[%d]", account_id);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
#ifdef __LOCAL_ACTIVITY__ 
	/* Delete all local activities of previous account */
	emf_activity_tbl_t activity;
	memset(&activity, 0x00, sizeof(emf_activity_tbl_t));
	activity.account_id = account_id;
	
	if (!em_core_activity_delete(&activity, &err)) {
		EM_DEBUG_LOG("\t em_core_activity_delete failed - %d", err);
		
		goto FINISH_OFF;
	}
#endif	

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
	if (false == em_core_clear_partial_body_thd_event_que(&err))
		EM_DEBUG_EXCEPTION(" em_core_clear_partial_body_thd_event_que [%d]", err);

	if (false == em_storage_delete_full_pbd_activity_data(account_id, true, &err))
		EM_DEBUG_EXCEPTION("em_storage_delete_full_pbd_activity_data failed [%d]", err);
	
#endif

#ifdef __FEATURE_USING_MY_ACCOUNT__ 
	{
		int error_code;
		emf_account_t *account_to_be_deleted;

		account_to_be_deleted = em_core_get_account_reference(account_id);
		if (account_to_be_deleted && account_to_be_deleted->receiving_server_type != EMF_SERVER_TYPE_ACTIVE_SYNC) {
			EM_DEBUG_LOG("Calling account_svc_delete with my_account_id[%d]", account_to_be_deleted->my_account_id);
			error_code = account_connect();
			EM_DEBUG_LOG("account_connect returns [%d]", error_code);
			account_delete_from_db_by_id(account_to_be_deleted->my_account_id);		
			error_code = account_disconnect();
			EM_DEBUG_LOG("account_disconnect returns [%d]", error_code);
		}
	}
#endif
	if (em_core_cancel_all_threads_of_an_account(account_id) < EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("There are some remaining jobs. I couldn't stop them.");
		err = EMF_ERROR_CANNOT_STOP_THREAD;
		goto FINISH_OFF;
	}
	
	emf_mailbox_t mbox;

	/*  BEGIN TRANSACTION;		 */
	em_storage_begin_transaction(NULL, NULL, NULL);
	
	if (!em_storage_delete_account(account_id, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_delete_account failed - %d", err);
		goto FINISH_OFF;
	}
	
#ifdef __FEATURE_KEEP_CONNECTION__
	/* em_core_reset_streams(); */
	em_core_remove_connection_info(account_id);
#endif
	
	mbox.account_id = account_id;
	mbox.name = NULL;
	
	if (!em_core_mail_delete_all(&mbox, 0, &err))  {
		EM_DEBUG_EXCEPTION("em_core_mail_delete_all failed - %d", err);
		goto FINISH_OFF;
	}
	
	/*  delete all mailboxe */
	if (!em_storage_delete_mailbox(account_id, -1, NULL, false, &err))  {		
		EM_DEBUG_EXCEPTION("em_storage_delete_mailbox failed - %d", err);
		goto FINISH_OFF;
	}
	
	/*  delete local imap sync mailbox from imap mailbox tabl */
	if (!em_storage_remove_downloaded_mail(account_id, NULL, NULL, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_remove_downloaded_mail failed - %d", err);
		goto FINISH_OFF;
	}

	em_core_check_unread_mail();
	em_core_delete_notification_by_account(account_id);
	em_core_refresh_account_reference();
	
	ret = true;
	
FINISH_OFF: 
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (em_storage_commit_transaction(NULL, NULL, NULL) == false) {
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
			ret = false;
		}
		if (!em_storage_notify_storage_event(NOTI_ACCOUNT_DELETE, account_id, 0, NULL, 0))
			EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event[ NOTI_ACCOUNT_DELETE] : Notification Failed >>> ");

	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (em_storage_rollback_transaction(NULL, NULL, NULL) == false)
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
		if (!em_storage_notify_storage_event(NOTI_ACCOUNT_DELETE_FAIL, account_id, err, NULL, 0))
			EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event[ NOTI_ACCOUNT_DELETE] : Notification Failed >>> ");
	}	

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

EXPORT_API int em_core_account_create(emf_account_t *account, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account[%p], err_code[%p]", account, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int i, count = 0, is_preset_IMAP_account = false;
	emf_mailbox_t local_mailbox = {0};
	emf_mail_account_tbl_t *temp_account_tbl = NULL;
	
	if (!account)  {
		EM_DEBUG_EXCEPTION("account[%p]", account);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!em_storage_get_account_count(&count, true, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_get_account_count failed - %d", err);
		goto FINISH_OFF;
	}


	if (count >= EMF_ACCOUNT_MAX)  {
		EM_DEBUG_EXCEPTION("too many accounts...");
		err = EMF_ERROR_ACCOUNT_MAX_COUNT;
		goto FINISH_OFF;
	}

	account->account_id = 0;

	/* check for email address validation */
	EM_DEBUG_LOG("account->email_addr[%s]", account->email_addr);
	if (account->email_addr) {
		if (!em_core_verify_email_address(account->email_addr, true, &err)) {
			err = EMF_ERROR_INVALID_ADDRESS;
			EM_DEBUG_EXCEPTION("Invalid Email Address");
			goto FINISH_OFF;
		}
	}

#ifdef __FEATURE_USING_MY_ACCOUNT__
	if (account->receiving_server_type != EMF_SERVER_TYPE_ACTIVE_SYNC) {
		int account_svc_id = 0;
		int error_code;
		account_h account_handle = NULL;

		error_code = account_connect();
		error_code = account_create(&account_handle);
		
		if(error_code != ACCOUNT_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("account_create failed [%d]", error_code);
			err = error_code;
			goto FINISH_OFF;
		}

		account_set_user_name(account_handle, account->user_name);
		account_set_domain_name(account_handle, account->account_name); 
		account_set_email_address(account_handle,  account->email_addr);
		account_set_source(account_handle, "EMAIL");
		account_set_package_name(account_handle, "email-setting-efl");
		account_set_capability(account_handle , ACCOUNT_CAPABILITY_EMAIL, ACCOUNT_CAPABILITY_ENABLED);
		if (account->logo_icon_path)
			account_set_icon_path(account_handle, account->logo_icon_path);
		error_code = account_insert_to_db(account_handle, &account_svc_id); 

		if (error_code != ACCOUNT_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("account_insert_to_db failed [%d]", error_code);
			err = error_code;
			goto FINISH_OFF;
		}

			account->my_account_id = account_svc_id;    
			
		EM_DEBUG_LOG("account_insert_to_db succeed");
		
		account_disconnect();
	}
#endif  /*  __FEATURE_USING_MY_ACCOUNT__ */

	temp_account_tbl = em_core_malloc(sizeof(emf_mail_account_tbl_t));
	if (!temp_account_tbl) {
		EM_DEBUG_EXCEPTION("allocation failed [%d]", err);
		goto FINISH_OFF;
	}
	em_convert_account_to_account_tbl(account, temp_account_tbl);

	if (!em_storage_add_account(temp_account_tbl, true, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_add_account failed - %d", err);
		goto FINISH_OFF;
	}
	account->account_id = temp_account_tbl->account_id;
	is_preset_IMAP_account = ((account->receiving_server_type == EMF_SERVER_TYPE_IMAP4)) ? true : false;/*  && (account->preset_account)) ? true  :  false */

	EM_DEBUG_LOG("is_preset_IMAP_account  :  %d", is_preset_IMAP_account);

	if ((account->receiving_server_type != EMF_SERVER_TYPE_ACTIVE_SYNC)	&& (!is_preset_IMAP_account)) {
		/* 1. create default local mailbox
		*    (Inbox, Draft, Outbox, Sentbox) */
		for (i = 0; i < MAILBOX_COUNT; i++) {
	  	EM_DEBUG_LOG("g_default_mbox_name [%d/%d] is [%s]", i, MAILBOX_COUNT, g_default_mbox_name[i]);
			local_mailbox.account_id = temp_account_tbl->account_id;
			local_mailbox.name  = g_default_mbox_name[i];
			local_mailbox.mailbox_type	= g_default_mbox_type[i];
			if (local_mailbox.mailbox_type == EMF_MAILBOX_TYPE_INBOX) {
				local_mailbox.local = EMF_MAILBOX_FROM_SERVER;
				local_mailbox.synchronous = 1;
			}
			else {
				local_mailbox.local = EMF_MAILBOX_FROM_LOCAL;
				local_mailbox.synchronous = 0;
			}
			local_mailbox.alias = g_default_mbox_alias[i];
			em_core_mailbox_get_default_mail_slot_count(&local_mailbox.mail_slot_size, NULL);

			if (!em_core_mailbox_create(&local_mailbox, 0, &err))  {
				EM_DEBUG_EXCEPTION("em_core_mailbox_create failed - %d", err);
				goto FINISH_OFF;
			}
			
		}
	}
	
	
	
	ret = true;
	
FINISH_OFF: 
	if (temp_account_tbl)
		em_storage_free_account(&temp_account_tbl, 1, NULL);
	
	if (ret == false && account != NULL)  {
		if (!em_core_account_delete(account->account_id, NULL))
			EM_DEBUG_EXCEPTION("emf_account_delete Failed [%d]", account->account_id);
	}
	
	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("Return value [%d]", ret);
	return ret;
}


EXPORT_API int em_core_init_account_reference()
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	emf_account_list_t *account_list = NULL;
	emf_account_list_t **p = NULL;
	emf_account_t *account = NULL;
	emf_mail_account_tbl_t *account_tbl_array = NULL;
	int count = 0;		
	int i = 0;
	
	if (!g_account_retrieved)  {
		count = 1000;
		if (!em_storage_get_account_list(&count, &account_tbl_array, true, true, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_get_account_list failed - %d", err);
			goto FINISH_OFF;
		}
		
		for (p = &account_list, i = 0; i < count; i++)  {
			account = malloc(sizeof(emf_account_t));
			if (!account)  {	
				EM_DEBUG_EXCEPTION("malloc failed...");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			em_convert_account_tbl_to_account(account_tbl_array + i, account);

			/* memcpy(account, accounts + i, sizeof(emf_account_t)) */
			/* memset(accounts + i, 0x00, sizeof(emf_account_t)) */
			
			(*p) = malloc(sizeof(emf_account_list_t));
			if (!(*p))  {	
				EM_DEBUG_EXCEPTION("malloc failed...");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			memset((*p), 0x00, sizeof(emf_account_list_t));
			
			(*p)->account = account;
			
			p = &(*p)->next;
		}
		if (g_account_num)
			em_core_free_account_reference();
		g_account_retrieved = 1;
		g_account_num = count;
		g_account_list = account_list;
	}
	
	ret = true;
	
FINISH_OFF: 
	if (account_tbl_array != NULL)
		em_storage_free_account(&account_tbl_array, count, NULL);
	
	if (!ret)  {
		g_account_list = account_list;
		em_core_free_account_reference();
	}

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_core_refresh_account_reference()
{
	EM_DEBUG_FUNC_BEGIN();
	
	if (g_account_retrieved && g_account_num)
		em_core_free_account_reference();
	
	g_account_retrieved = 0;
	g_account_num = 0;
	g_account_list = NULL;
	
	if (!em_core_init_account_reference())  {
		EM_DEBUG_EXCEPTION("em_core_init_account_reference failed...");
		return false;
	}
	EM_DEBUG_FUNC_END();
	return true;
}

EXPORT_API int em_core_free_account_reference()
{
	EM_DEBUG_FUNC_BEGIN();
	
	emf_account_list_t *p = g_account_list, *p_next;
	while (p)  {
		em_core_account_free(&p->account, 1, NULL);
		
		p_next = p->next;
		free(p);
		p = p_next;
	}

	g_account_retrieved = 0;
	g_account_num = 0;
	g_account_list = NULL;
	EM_DEBUG_FUNC_END();
	return true;
}

EXPORT_API int em_core_account_free(emf_account_t **account_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_list[%p], count[%d], err_code[%p]", account_list, count, err_code);
	
	/*  default variabl */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (count > 0)  {
		if (!account_list || !*account_list)  {
			err = EMF_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		emf_account_t *p = *account_list;
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
		
		EM_SAFE_FREE(p); *account_list = NULL;
	}
	
	ret = true;
	
FINISH_OFF: 
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


EXPORT_API int em_core_account_get_list_refer(emf_account_t **account_list, int *count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_list[%p], count[%p], err_code[%p]", account_list, count, err_code);
	int i, countOfAccounts = 0;
	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_account_t *accountRef;
	emf_account_list_t *p;

	if (!account_list || !count)  {
		EM_DEBUG_EXCEPTION("account_list[%p], count[%p]", account_list, count);
		err = EMF_ERROR_INVALID_PARAM;
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
		*account_list = malloc(sizeof(emf_account_t) * countOfAccounts);
		if (!*account_list)  {		
			EM_DEBUG_LOG("malloc failed...");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
	}

	p = g_account_list;
	for (i = 0; i < countOfAccounts; i++)  {
		accountRef = (*account_list) + i;
		memcpy(accountRef, p->account , sizeof(emf_account_t));
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
	int local_error_code = EMF_ERROR_NONE, ret_code = false;

	if (!target_buffer || !target_buffer_lenth || !input_data) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		local_error_code = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (*target_buffer_lenth > 0 && input_data_length) {
		EM_DEBUG_LOG("*target_buffer_lenth [%d]", *target_buffer_lenth);
		*target_buffer = realloc(*target_buffer, (*target_buffer_lenth) + input_data_length);
		if (!*target_buffer) {
			EM_DEBUG_EXCEPTION("realloc failed");
			local_error_code = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		memcpy(*target_buffer + (*target_buffer_lenth), input_data, input_data_length);
		*target_buffer_lenth += input_data_length;
		EM_DEBUG_LOG("*target_buffer_lenth [%d] input_data_length [%d]", *target_buffer_lenth, input_data_length);
	}
	else {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		local_error_code = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ret_code = true;

FINISH_OFF: 

	if (error_code)
		*error_code = local_error_code;
	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);

	return ret_code;
}


static int em_core_write_account_into_buffer(char **target_buffer, int *target_buffer_lenth, emf_mail_account_tbl_t *account_tbl_ptr, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN("target_buffer [%p], target_buffer_lenth [%p], account_tbl_ptr [%p], error_code [%p]", target_buffer, target_buffer_lenth, account_tbl_ptr, error_code);
	int local_error_code = EMF_ERROR_NONE, ret_code = false, stream_length = 0;
	emf_account_t *temp_account = NULL;
	char *byte_stream = NULL;

	temp_account = em_core_malloc(sizeof(emf_account_t));
	memset(temp_account, 0, sizeof(emf_account_t));

	if (em_convert_account_tbl_to_account(account_tbl_ptr, temp_account)) {
		byte_stream = em_convert_account_to_byte_stream(temp_account, &stream_length);
		EM_DEBUG_LOG("stream_length [%d]", stream_length);
		/*  EM_DEBUG_LOG("password [%s]", temp_account->password) */

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
		local_error_code = EM_STORAGE_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	ret_code = true;
FINISH_OFF: 
	if (temp_account)
		em_core_account_free(&temp_account, 1, NULL);
	if (error_code)
		*error_code = local_error_code;

	EM_SAFE_FREE(byte_stream);

	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);
	return ret_code;
}

EXPORT_API int em_core_backup_accounts(const char *file_path, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN("file_path [%s], error_code [%p]", file_path, error_code);
	int local_error_code = EMF_ERROR_NONE, local_error_code_2 = EMF_ERROR_NONE, ret_code = false;
	int select_num, i, target_buff_length = 0;
	char *target_buffer = NULL;
	emf_mail_account_tbl_t *account_list = NULL;

	if (!file_path) {
		local_error_code = EMF_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		goto FINISH_OFF;	
	}

	select_num = 1000;
	
	if (!em_storage_get_account_list(&select_num, &account_list, true, true, &local_error_code)) {
		EM_DEBUG_EXCEPTION("em_storage_get_account_list failed [%d]", local_error_code);
		goto FINISH_OFF;	
	}
	
	EM_DEBUG_LOG("select_num [%d]", select_num);
	
	if (account_list) {
		target_buffer = malloc(sizeof(int));
		if (!target_buffer)  {
			EM_DEBUG_EXCEPTION("malloc failed");
			local_error_code = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		memcpy(target_buffer, (char *)&select_num, sizeof(int));
		target_buff_length = sizeof(int);

		for (i = 0; i < select_num; i++) {
			if (!em_core_write_account_into_buffer(&target_buffer, &target_buff_length, account_list + i, &local_error_code)) {
				EM_DEBUG_EXCEPTION("em_core_write_account_into_buffer failed [%d]", local_error_code);
				goto FINISH_OFF;	
			}
		}

		EM_DEBUG_LOG("target_buff_length [%d]", target_buff_length);

		ssm_delete_file(file_path, SSM_FLAG_SECRET_OPERATION, NULL);
		
		if (ssm_write_buffer(target_buffer, target_buff_length, file_path, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
			EM_DEBUG_EXCEPTION("ssm_write_buffer failed [%d]", local_error_code);
			local_error_code = EM_STORAGE_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;	
		}
		
	}

	ret_code = true;	
FINISH_OFF: 

	EM_SAFE_FREE(target_buffer);
	if (account_list)
		em_storage_free_account(&account_list, select_num, &local_error_code_2);

	if (error_code)
		*error_code = local_error_code;

	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);
	return ret_code;
}

EXPORT_API int em_core_restore_accounts(const char *file_path, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN("file_path [%s], error_code [%p]", file_path, error_code);
	int local_error_code = EMF_ERROR_NONE, ret_code = false, buffer_length = 0, read_length = 0;
	int account_count = 0, i = 0, account_stream_length = 0;
	char *temp_buffer = NULL, *account_stream = NULL, *buffer_ptr = NULL;
	emf_account_t *temp_account = NULL, *account_list = NULL;

	ssm_file_info_t sfi;

	if (!file_path) {
		local_error_code = EMF_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		goto FINISH_OFF;	
	}

	if (em_core_account_get_list_refer(&account_list, &account_count, &ret_code)) {
		for (i = 0; i < account_count; i++) {
			if (!em_core_account_delete(account_list[i].account_id, &ret_code)) {
				local_error_code = EMF_ERROR_INVALID_ACCOUNT;
				EM_DEBUG_EXCEPTION("em_core_account_delete failed");
				goto FINISH_OFF;	
			}
		}
	}

	if (ssm_getinfo(file_path, &sfi, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION("ssm_getinfo() failed.");
		ret_code = EM_STORAGE_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	buffer_length = sfi.originSize;
	EM_DEBUG_LOG("account buffer_length[%d]", buffer_length);
	if ((temp_buffer = (char *)em_core_malloc(buffer_length + 1)) == NULL) {
		EM_DEBUG_EXCEPTION("em_core_malloc failed...");
		ret_code = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (ssm_read(file_path, temp_buffer, buffer_length, (size_t *)&read_length, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION("ssm_read() failed.");
		ret_code = EM_STORAGE_ERROR_SYSTEM_FAILURE;
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
				account_stream = em_core_malloc(account_stream_length);
				if (!account_stream) {
					EM_DEBUG_EXCEPTION("em_core_malloc() failed.");
					ret_code = EMF_ERROR_OUT_OF_MEMORY ;
					goto FINISH_OFF;
				}
				memcpy(account_stream, buffer_ptr, account_stream_length);

				temp_account = em_core_malloc(sizeof(emf_account_t));

				if (!temp_account) {
					EM_DEBUG_EXCEPTION("em_core_malloc() failed.");
					ret_code = EMF_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}

				em_convert_byte_stream_to_account(account_stream, temp_account);
				EM_SAFE_FREE(account_stream);
			
				if (!em_core_account_create(temp_account, &ret_code)) {
					EM_DEBUG_EXCEPTION("em_core_account_create() failed.");
					goto FINISH_OFF;
				}

				em_core_account_free(&temp_account, 1,  &ret_code);
				temp_account = NULL;
			}
			buffer_ptr += account_stream_length;
			account_stream_length = 0;
		}
	} else {
		EM_DEBUG_EXCEPTION("ssm_read() failed.");
		ret_code = EMF_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
	ret_code = true;	
FINISH_OFF: 
	if (temp_account)
		em_core_account_free(&temp_account, 1, NULL);
	EM_SAFE_FREE(account_stream);
	EM_SAFE_FREE(temp_buffer);

	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);
	return ret_code;
}

#endif /*  __FEATURE_BACKUP_ACCOUNT_ */

EXPORT_API int em_core_query_server_info(const char* domain_name, emf_server_info_t **result_server_info)
{
	EM_DEBUG_FUNC_BEGIN("domain_name [%s], result_server_info [%p]", domain_name, result_server_info);
	int ret_code = EMF_ERROR_NONE;


	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);
	return ret_code;
}


EXPORT_API int em_core_free_server_info(emf_server_info_t **target_server_info)
{
	EM_DEBUG_FUNC_BEGIN("result_server_info [%p]", target_server_info);
	int i, ret_code = EMF_ERROR_NONE;
	emf_server_info_t *server_info = NULL;

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

EXPORT_API int em_core_save_default_account_id(int input_account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d]", input_account_id);
	int ret_code = EMF_ERROR_NONE, result_value = 0;

	result_value = vconf_set_int(VCONF_KEY_DEFAULT_ACCOUNT_ID, input_account_id);
	if (result_value < 0) {
		EM_DEBUG_EXCEPTION("vconf_set_int failed [%d]", result_value);
		ret_code = EMF_ERROR_SYSTEM_FAILURE;
	}

	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);
	return ret_code;	
}

EXPORT_API int em_core_load_default_account_id(int *output_account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%p]", output_account_id);
	int ret_code = EMF_ERROR_NONE, result_value = 0;
	
	if (output_account_id == NULL) {
		ret_code = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	result_value = vconf_get_int(VCONF_KEY_DEFAULT_ACCOUNT_ID, output_account_id);

	if (result_value < 0) {
		EM_DEBUG_EXCEPTION("vconf_get_int() failed [%d]", result_value);
		ret_code = EMF_ERROR_SYSTEM_FAILURE;
		*output_account_id = 0;
	}

FINISH_OFF: 
	
	EM_DEBUG_FUNC_END("ret_code [%d]", ret_code);
	return ret_code;	
}
