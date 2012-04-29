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
 * File :  email-core-mailbox.c
 * Desc :  Local Mailbox Management
 *
 * Auth : 
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include "email-types.h"
#include "email-utilities.h"
#include "email-debug-log.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-core-mailbox.h"
#include "email-core-event.h"
#include "email-core-mail.h"
#include "email-core-imap-mailbox.h"   
#include "email-storage.h"
#include "email-core-account.h" 

#ifdef __FEATURE_KEEP_CONNECTION__
static void *g_receiving_thd_stream = NULL;			/* Stores the recv thd stream for next time reuse */
static int prev_acc_id_recv_thd = 0;				/* Stores the account id for which recv thd stream is open */
extern int recv_thread_run;

static void *g_partial_body_thd_stream = NULL;		/* Stores the pb thd stream for next time reuse */
static int prev_acc_id_pb_thd = 0;					/* Stores the account id for which pb thd stream is open */

__thread emf_connection_info_t *g_connection_info_list = NULL;

static pthread_mutex_t _close_stream_lock = PTHREAD_MUTEX_INITIALIZER;	/* Mutex to protect closing stream */
#endif /*  __FEATURE_KEEP_CONNECTION__ */


/*  Binding IMAP mailbox with its function */
static emf_mailbox_type_item_t  g_mailbox_type[MAX_MAILBOX_TYPE] = {
				{EMF_MAILBOX_TYPE_INBOX, "INBOX" }, 
				/*  Naver */
				{EMF_MAILBOX_TYPE_INBOX, "Inbox" }, 
				{EMF_MAILBOX_TYPE_SENTBOX, "Sent Messages"} , 
				{EMF_MAILBOX_TYPE_SPAMBOX, "&wqTTOLpUx3zVaA-"} , 
				{EMF_MAILBOX_TYPE_DRAFT, "Drafts"} , 
				{EMF_MAILBOX_TYPE_TRASH, "Deleted Messages" } , 
				/*  Google English */
				{EMF_MAILBOX_TYPE_SENTBOX, "[Gmail]/Sent Mail"} , 
				{EMF_MAILBOX_TYPE_SPAMBOX, "[Gmail]/Spam"} , 
				{EMF_MAILBOX_TYPE_DRAFT, "[Gmail]/Drafts"} , 
				{EMF_MAILBOX_TYPE_TRASH, "[Gmail]/Trash" } , 
				{EMF_MAILBOX_TYPE_ALL_EMAILS, "[Gmail]/All Mail" } , 
				/*  Google Korean */
				{EMF_MAILBOX_TYPE_SENTBOX, "[Gmail]/&vPSwuNO4ycDVaA-"} , 
				{EMF_MAILBOX_TYPE_SPAMBOX, "[Gmail]/&wqTTONVo-" }, 
				{EMF_MAILBOX_TYPE_DRAFT, "[Gmail]/&x4TC3Lz0rQDVaA-"} , 
				{EMF_MAILBOX_TYPE_TRASH, "[Gmail]/&1zTJwNG1-"} , 
				{EMF_MAILBOX_TYPE_ALL_EMAILS, "[Gmail]/&yATMtLz0rQDVaA-" } , 
 				/*  AOL */
				{EMF_MAILBOX_TYPE_SENTBOX, "Sent"} , 
				{EMF_MAILBOX_TYPE_SPAMBOX, "Spam" }, 
				{EMF_MAILBOX_TYPE_DRAFT, "Drafts"} , 
				{EMF_MAILBOX_TYPE_TRASH, "Trash"}, 
				/* DAUM */
				{EMF_MAILBOX_TYPE_SPAMBOX, "&wqTTONO4ycDVaA-"},
				/* ETC */
				{EMF_MAILBOX_TYPE_SENTBOX, "mail/sent-mail"}, 
				{EMF_MAILBOX_TYPE_SPAMBOX, "mail/spam-mail" }, 
				{EMF_MAILBOX_TYPE_DRAFT, "mail/saved-drafts"} , 
				{EMF_MAILBOX_TYPE_TRASH, "mail/mail-trash"}, 
};

#ifdef __FEATURE_KEEP_CONNECTION__
emf_connection_info_t* emcore_get_connection_info_by_account_id(int account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d]", account_id);
	emf_connection_info_t *connection_info = g_connection_info_list;

	while(connection_info) {
		if(connection_info->account_id == account_id)
			break;
		connection_info = connection_info->next;
	}
	
	EM_DEBUG_FUNC_END("connection_info [%p]", connection_info);
	return connection_info;
}

int emcore_append_connection_info(emf_connection_info_t *new_connection_info)
{
	EM_DEBUG_FUNC_BEGIN("new_connection_info [%p]", new_connection_info);
	emf_connection_info_t *connection_info = g_connection_info_list;

	if(!new_connection_info) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return EMF_ERROR_INVALID_PARAM;
	}

	if(emcore_get_connection_info_by_account_id(new_connection_info->account_id)) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_ALREADY_EXISTS");
		return EMF_ERROR_ALREADY_EXISTS;
	}

	if(connection_info) {
		while(connection_info) {
			if(connection_info->next == NULL) {
				connection_info->next = new_connection_info;
				new_connection_info->next = NULL;
			}
			connection_info = connection_info->next;
		}
	}
	else {
		new_connection_info->next = NULL;
		g_connection_info_list = new_connection_info;
	}
	
	EM_DEBUG_FUNC_END("EMF_ERROR_NONE");
	return EMF_ERROR_NONE;
}

INTERNAL_FUNC int emcore_remove_connection_info(int account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d]", account_id);
	emf_connection_info_t *connection_info = g_connection_info_list, *prev_connection_info = NULL;

	while(connection_info) {
		if(connection_info->account_id == account_id) {
			if(prev_connection_info) {
				prev_connection_info->next = connection_info->next;
			}
			else {
				g_connection_info_list = connection_info->next;
			}
			EM_SAFE_FREE(connection_info);
			break;
		}
		prev_connection_info = connection_info;
		connection_info = connection_info->next;
	}
	
	EM_DEBUG_FUNC_END("");
	return EMF_ERROR_NONE;
}

#endif /* __FEATURE_KEEP_CONNECTION__ */


/* description
 *    get local mailbox list
 */
INTERNAL_FUNC int emcore_get_list(int account_id, emf_mailbox_t **mailbox_list, 	int *p_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_list[%p], p_count[%p], err_code[%p]", account_id, mailbox_list, p_count, err_code);
	
	if (account_id <= 0 || !mailbox_list || !p_count)  {
		EM_DEBUG_EXCEPTION("PARAM Failed account_id[%d], mailbox_list[%p], p_count[%p]", account_id, mailbox_list, p_count);
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	emstorage_mailbox_tbl_t *local_mailbox_list = NULL;
	emf_account_t *ref_account = NULL;
	int i, count = 512;
	
	/* get mailbox list from mailbox table */
	
	if (!(ref_account = emcore_get_account_reference(account_id)))  {
		EM_DEBUG_EXCEPTION(" emcore_get_account_reference failed - %d", account_id);
		error = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	if (!emstorage_get_mailbox(ref_account->account_id, EMF_MAILBOX_ALL, EMAIL_MAILBOX_SORT_BY_NAME_ASC, &count, &local_mailbox_list, true, &error))  {	
		EM_DEBUG_EXCEPTION(" emstorage_get_mailbox failed - %d", error);
	
		goto FINISH_OFF;
	}
	
	if (count > 0)  {
		if (!(*mailbox_list = em_malloc(sizeof(emf_mailbox_t) * count)))  {
			EM_DEBUG_EXCEPTION(" mailloc failed...");
			error = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		memset(*mailbox_list, 0x00, (sizeof(emf_mailbox_t) * count));
		
		for (i = 0; i < count; i++)  {
			(*mailbox_list)[i].mailbox_id = local_mailbox_list[i].mailbox_id;
			(*mailbox_list)[i].account_id = account_id;
			(*mailbox_list)[i].name = local_mailbox_list[i].mailbox_name; local_mailbox_list[i].mailbox_name = NULL;
			(*mailbox_list)[i].alias = local_mailbox_list[i].alias; local_mailbox_list[i].alias = NULL;
			(*mailbox_list)[i].local = local_mailbox_list[i].local_yn;		
			(*mailbox_list)[i].mailbox_type = local_mailbox_list[i].mailbox_type;	
			(*mailbox_list)[i].synchronous = local_mailbox_list[i].sync_with_server_yn;
			(*mailbox_list)[i].unread_count = local_mailbox_list[i].unread_count;
			(*mailbox_list)[i].total_mail_count_on_local = local_mailbox_list[i].total_mail_count_on_local;
			(*mailbox_list)[i].total_mail_count_on_server = local_mailbox_list[i].total_mail_count_on_server;
			(*mailbox_list)[i].has_archived_mails = local_mailbox_list[i].has_archived_mails;
			(*mailbox_list)[i].mail_slot_size = local_mailbox_list[i].mail_slot_size;
		}
	}
	else
		mailbox_list = NULL;

	* p_count = count;

	ret = true;

FINISH_OFF: 
	if (local_mailbox_list != NULL)
		emstorage_free_mailbox(&local_mailbox_list, count, NULL);
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END();
	return ret;
}

/* description
 *    get imap sync mailbox list
 */
int emcore_get_list_to_be_sync(int account_id, emf_mailbox_t **mailbox_list, int *p_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_list[%p], p_count[%p], err_code[%p]", account_id, mailbox_list, p_count, err_code);
	
	if (account_id <= 0 || !mailbox_list || !p_count)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_list[%p], p_count[%p]", account_id, mailbox_list, p_count);
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMF_ERROR_NONE;
	emf_mailbox_t *tmp_mailbox_list = NULL;
	emstorage_mailbox_tbl_t *mailbox_tbl_list = NULL;
	emf_account_t *ref_account = NULL;
	int i, count = 512;
	
	/* get mailbox list from mailbox table */
	if (!(ref_account = emcore_get_account_reference(account_id)))  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed - %d", account_id);
		error = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	if (!emstorage_get_mailbox(ref_account->account_id, 0, EMAIL_MAILBOX_SORT_BY_TYPE_ASC, &count, &mailbox_tbl_list, true, &error))  {	
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox failed - %d", error);
	
		goto FINISH_OFF;
	}
	
	if (count > 0)  {
		if (!(tmp_mailbox_list = em_malloc(sizeof(emf_mailbox_t) * count)))  {
			EM_DEBUG_EXCEPTION("malloc failed...");
			error = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		memset(tmp_mailbox_list, 0x00, (sizeof(emf_mailbox_t) * count));
		
		for (i = 0; i < count; i++)  {
			tmp_mailbox_list[i].mailbox_id = mailbox_tbl_list[i].mailbox_id;
			tmp_mailbox_list[i].account_id = account_id;
			tmp_mailbox_list[i].name = mailbox_tbl_list[i].mailbox_name; mailbox_tbl_list[i].mailbox_name = NULL;
			tmp_mailbox_list[i].mailbox_type = mailbox_tbl_list[i].mailbox_type; 
			tmp_mailbox_list[i].alias = mailbox_tbl_list[i].alias; mailbox_tbl_list[i].alias = NULL;
			tmp_mailbox_list[i].local = mailbox_tbl_list[i].local_yn;
			tmp_mailbox_list[i].synchronous = mailbox_tbl_list[i].sync_with_server_yn;
			tmp_mailbox_list[i].unread_count = mailbox_tbl_list[i].unread_count;
			tmp_mailbox_list[i].total_mail_count_on_local = mailbox_tbl_list[i].total_mail_count_on_local;
			tmp_mailbox_list[i].total_mail_count_on_server = mailbox_tbl_list[i].total_mail_count_on_server;
			tmp_mailbox_list[i].mail_slot_size = mailbox_tbl_list[i].mail_slot_size;
		}
	}
	else
		tmp_mailbox_list = NULL;
	*p_count = count;
	ret = true;
	
FINISH_OFF: 
	
	*mailbox_list = tmp_mailbox_list;
	
	
	if (mailbox_tbl_list != NULL)
		emstorage_free_mailbox(&mailbox_tbl_list, count, NULL);
	
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("error [%d]", error);
	return ret;
}

INTERNAL_FUNC int emcore_get_mail_count(emf_mailbox_t *mailbox, int *total, int *unseen, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], total[%p], unseen[%p], err_code[%p]", mailbox, total, unseen, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!mailbox)  {
		EM_DEBUG_EXCEPTION(" mailbox[%p], total[%p], unseen[%p]", mailbox, total, unseen);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emstorage_get_mail_count(mailbox->account_id, mailbox->name, total, unseen, true, &err))  {
		EM_DEBUG_EXCEPTION(" emstorage_get_mail_count failed - %d", err);

		goto FINISH_OFF;
	}

	
	ret = true;
	
FINISH_OFF: 
	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}

INTERNAL_FUNC int emcore_create_mailbox(emf_mailbox_t *new_mailbox, int on_server, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("new_mailbox[%p], err_code[%p]", new_mailbox, err_code);
	int ret = false;
	int err = EMF_ERROR_NONE;
	emstorage_mailbox_tbl_t local_mailbox;
	
	if (new_mailbox == NULL || new_mailbox->name == NULL)  {
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if  (on_server) {
		/* Create a mailbox from Sever */
		if (!emcore_create_imap_mailbox(new_mailbox, &err)) {
			EM_DEBUG_EXCEPTION(">>>>> mailbox Creation in Server FAILED >>> ");
	
			goto FINISH_OFF;
		}
		else
			EM_DEBUG_LOG(">>>>> mailbox Creation in Server SUCCESS >>> ");	
	}

	memset(&local_mailbox, 0x00, sizeof(emstorage_mailbox_tbl_t));
	EM_DEBUG_LOG("box name[%s] local yn[%d] mailbox_type[%d]", new_mailbox->name, local_mailbox.local_yn, new_mailbox->mailbox_type);

	/* add local mailbox into local mailbox table */
	local_mailbox.mailbox_id = new_mailbox->mailbox_id;
	local_mailbox.account_id = new_mailbox->account_id;
	local_mailbox.local_yn = new_mailbox->local;
	local_mailbox.mailbox_name = new_mailbox->name;
	local_mailbox.alias = new_mailbox->alias;
	local_mailbox.sync_with_server_yn = new_mailbox->synchronous;
	local_mailbox.mailbox_type = new_mailbox->mailbox_type;
	local_mailbox.unread_count = 0;
	local_mailbox.total_mail_count_on_local = 0;
	local_mailbox.total_mail_count_on_server = 0;
	emcore_get_default_mail_slot_count(&local_mailbox.mail_slot_size, NULL);

	if (strncmp(new_mailbox->name, EMF_INBOX_NAME, strlen(EMF_INBOX_NAME))    == 0 || 
	     strncmp(new_mailbox->name, EMF_DRAFTBOX_NAME, strlen(EMF_DRAFTBOX_NAME)) == 0 || 
		strncmp(new_mailbox->name, EMF_OUTBOX_NAME, strlen(EMF_OUTBOX_NAME)) == 0 || 
		strncmp(new_mailbox->name, EMF_SENTBOX_NAME, strlen(EMF_SENTBOX_NAME))  == 0)
		local_mailbox.modifiable_yn = 0;			/*  can be deleted/modified */
	else
		local_mailbox.modifiable_yn = 1;


	if (!emstorage_add_mailbox(&local_mailbox, true, &err))  {
		EM_DEBUG_EXCEPTION(" emstorage_add_mailbox failed - %d", err);

		goto FINISH_OFF;
	}
	ret = true;
	
FINISH_OFF: 
	if (err_code)
		*err_code = err;
	
	return ret;
}

INTERNAL_FUNC int emcore_delete_mailbox(emf_mailbox_t *mailbox, int on_server, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], err_code[%p]", mailbox, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (mailbox == NULL)  {
		EM_DEBUG_EXCEPTION(" mailbox[%p]", mailbox);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emcore_delete_mail_all(mailbox, on_server, &err))  {
		EM_DEBUG_EXCEPTION(" emcore_delete_mail_all failed - %d", err);
		goto FINISH_OFF;
	}

	if (on_server) {
		EM_DEBUG_LOG(">>  Delete the mailbox in Sever >>> ");
		if  (!emcore_delete_imap_mailbox(mailbox, &err))
			EM_DEBUG_EXCEPTION(">>> Delete the mailbox in Server  :  FAILED >>> ");
		else
			EM_DEBUG_LOG(">>> Delete the mailbox in Server  :  SUCCESS >>> ");
	}

	if (!emstorage_delete_mailbox(mailbox->account_id, -1, mailbox->name, true, &err))  {
		EM_DEBUG_EXCEPTION(" emstorage_delete_mailbox failed - %d", err);

		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF: 
	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}

INTERNAL_FUNC int emcore_delete_mailbox_all(emf_mailbox_t *mailbox, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	EM_DEBUG_LOG(" mailbox[%p], err_code[%p]", mailbox, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;

	if (mailbox == NULL) {
		EM_DEBUG_EXCEPTION(" mailbox[%p]", mailbox);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emcore_delete_mail_all(mailbox, 0, /*NULL, */ &err)) {
		EM_DEBUG_EXCEPTION(" emcore_delete_mail_all failed - %d", err);
		
		goto FINISH_OFF;
	}
	
	if (!emstorage_delete_mailbox(mailbox->account_id, -1, mailbox->name, true, &err)) {
		EM_DEBUG_EXCEPTION(" emstorage_delete_mailbox failed - %d", err);
		

		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF: 
	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}

INTERNAL_FUNC int emcore_update_mailbox(emf_mailbox_t *old_mailbox, emf_mailbox_t *new_mailbox, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("old_mailbox[%p], new_mailbox[%p], err_code[%p]", old_mailbox, new_mailbox, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (old_mailbox == NULL || new_mailbox == NULL)  {
		EM_DEBUG_EXCEPTION("old_mailbox[%p], new_mailbox[%p]", old_mailbox, new_mailbox);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	emstorage_mailbox_tbl_t new_mailbox_tbl;
	memset(&new_mailbox_tbl, 0x00, sizeof(emstorage_mailbox_tbl_t));
	
	/*  Support only updating mailbox_type */
	new_mailbox_tbl.mailbox_type = new_mailbox->mailbox_type;

	if (old_mailbox->mailbox_type != new_mailbox_tbl.mailbox_type) {
		if (!emstorage_update_mailbox_type(old_mailbox->account_id, -1, old_mailbox->name, &new_mailbox_tbl, true, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_update_mailbox failed - %d", err);
	
			goto FINISH_OFF;
		}
	}

	new_mailbox_tbl.mailbox_id 			= old_mailbox->mailbox_id;
	new_mailbox_tbl.account_id 			= old_mailbox->account_id;
	new_mailbox_tbl.mailbox_name 		= new_mailbox->name;
	new_mailbox_tbl.mailbox_type 		= new_mailbox->mailbox_type;
	new_mailbox_tbl.alias 				= new_mailbox->alias;
	new_mailbox_tbl.sync_with_server_yn = new_mailbox->synchronous;
	new_mailbox_tbl.mail_slot_size 		= new_mailbox->mail_slot_size;
	new_mailbox_tbl.total_mail_count_on_server = new_mailbox->total_mail_count_on_server;
	
	if (!emstorage_update_mailbox(old_mailbox->account_id, -1, old_mailbox->name, &new_mailbox_tbl, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_update_mailbox failed - %d", err);

		goto FINISH_OFF;
	}
	
	if (EM_SAFE_STRCMP(old_mailbox->name, new_mailbox_tbl.mailbox_name) != 0) {
		if (!emstorage_rename_mailbox(old_mailbox->account_id, old_mailbox->name, new_mailbox_tbl.mailbox_name, true, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_rename_mailbox failed - %d", err);
	
			goto FINISH_OFF;
		}
	}
	
	ret = true;
	
FINISH_OFF: 
	if (err_code)
		*err_code = err;
	
	return ret;
}

extern int try_auth;
extern int try_auth_smtp;

#ifdef __FEATURE_KEEP_CONNECTION__
extern long smtp_send(SENDSTREAM *stream, char *command, char *args);
#endif /* __FEATURE_KEEP_CONNECTION__ */

INTERNAL_FUNC int emcore_connect_to_remote_mailbox_with_account_info(emf_account_t *account, char *mailbox, void **result_stream, int *err_code)
{
	EM_PROFILE_BEGIN(emCoreMailboxOpen);
	EM_DEBUG_FUNC_BEGIN("account[%p], mailbox[%p], mail_stream[%p], err_code[%p]", account, mailbox, result_stream, err_code);
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	emf_session_t *session = NULL;
	char *mbox_path = NULL;
	void *reusable_stream = NULL;
	int is_connection_for = _SERVICE_THREAD_TYPE_NONE;

	if (account == NULL) {
		EM_DEBUG_EXCEPTION("Invalid Parameter.");
		error = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emcore_get_current_session(&session)) {
		EM_DEBUG_EXCEPTION("emcore_get_current_session failed...");
		error = EMF_ERROR_SESSION_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (mailbox == NULL || (mailbox && strcmp(mailbox, ENCODED_PATH_SMTP) != 0)) 
		is_connection_for = _SERVICE_THREAD_TYPE_RECEIVING;
	else 
		is_connection_for = _SERVICE_THREAD_TYPE_SENDING;

#ifdef __FEATURE_KEEP_CONNECTION__
	emf_connection_info_t *connection_info = emcore_get_connection_info_by_account_id(account->account_id);

	if(connection_info) {
		if (is_connection_for == _SERVICE_THREAD_TYPE_RECEIVING) {
			if(connection_info->receiving_server_stream_status == EMF_STREAM_STATUS_CONNECTED)
				reusable_stream = connection_info->receiving_server_stream;
		}
		else {
			if(connection_info->sending_server_stream_status == EMF_STREAM_STATUS_CONNECTED)
				reusable_stream = connection_info->sending_server_stream;
		}
	}
	
	if (reusable_stream != NULL)
		EM_DEBUG_LOG("Stream reuse desired");
#else
	reusable_stream = *result_stream;
#endif

	session->error = EMF_ERROR_NONE;
	emcore_set_network_error(EMF_ERROR_NONE);		/*  set current network error as EMF_ERROR_NONE before network operation */
	
	if (is_connection_for == _SERVICE_THREAD_TYPE_RECEIVING) {	
		/*  open pop3/imap server */
		MAILSTREAM *mail_stream = NULL;
		
		if (!emcore_get_long_encoded_path_with_account_info(account, mailbox, '/', &mbox_path, &error)) {
			EM_DEBUG_EXCEPTION("emcore_get_long_encoded_path failed - %d", error);
			session->error = error;
			goto FINISH_OFF;
		}
		
		EM_DEBUG_LOG("open mail connection to mbox_path [%s]", mbox_path);
		
		try_auth = 0; 		/*  ref_account->receiving_auth ? 1  :  0  */
		session->auth = 0; /*  ref_account->receiving_auth ? 1  :  0 */
		
		if (!(mail_stream = mail_open(reusable_stream, mbox_path, IMAP_2004_LOG))) {	
			EM_DEBUG_EXCEPTION("mail_open failed. session->error[%d], session->network[%d]", session->error, session->network);
			
			if (session->network != EMF_ERROR_NONE)
				session->error = session->network;
			if ((session->error == EMF_ERROR_UNKNOWN) || (session->error == EMF_ERROR_NONE))
				session->error = EMF_ERROR_CONNECTION_FAILURE;
			
			error = session->error;

#ifdef __FEATURE_KEEP_CONNECTION__
			/* Since mail_open failed Reset the global stream pointer as it is a dangling pointer now */
#endif /*  __FEATURE_KEEP_CONNECTION__ */
			goto FINISH_OFF;
		}
		*result_stream = mail_stream;
	}
	else {	
		/*  open smtp server */
		SENDSTREAM *send_stream = NULL;
		char *host_list[2] = {NULL, NULL};

#ifdef __FEATURE_KEEP_CONNECTION__
		if (reusable_stream != NULL) {
			int send_ret = 0;
			/* Check whether connection is avaiable */
			send_stream	= reusable_stream;
			/*
			send_ret = smtp_send(send_stream, "RSET", 0);
			
			if (send_ret != SMTP_RESPONSE_OK) {
				EM_DEBUG_EXCEPTION("[SMTP] RSET --> [%s]", send_stream->reply);
				send_stream = NULL;
			}
			*/
		}
#endif
		if(!send_stream) {
			if (!emcore_get_long_encoded_path_with_account_info(account, mailbox, 0, &mbox_path, &error)) {
				EM_DEBUG_EXCEPTION(" emcore_get_long_encoded_path failed - %d", error);
				session->error = error;
				goto FINISH_OFF;
			}
				
			EM_DEBUG_LOG("open SMTP connection to mbox_path [%s]", mbox_path);
			
			try_auth_smtp = account->sending_auth ? 1  :  0;
			session->auth = account->sending_auth ? 1  :  0;
			
			host_list[0] = mbox_path;
			
			if (!(send_stream = smtp_open(host_list, 1))) {
				EM_DEBUG_EXCEPTION("smtp_open failed... : current sending_security[%d] session->error[%d] session->network[%d]", account->sending_security, session->error, session->network);
				if (session->network != EMF_ERROR_NONE)
					session->error = session->network;
				if ((session->error == EMF_ERROR_UNKNOWN) || (session->error == EMF_ERROR_NONE))
					session->error = EMF_ERROR_CONNECTION_FAILURE;
				
				error = session->error;
				goto FINISH_OFF;
			}
		}
		*result_stream = send_stream;
	}
	
	ret = true;
	
FINISH_OFF: 

#ifdef __FEATURE_KEEP_CONNECTION__
	if (ret == true) {
		if(!connection_info) {
			connection_info = em_malloc(sizeof(emf_connection_info_t));
			connection_info->account_id = account->account_id;
			if(!connection_info) 
				EM_DEBUG_EXCEPTION("em_malloc for connection_info failed.");
			else
				emcore_append_connection_info(connection_info);
		}

		if(connection_info) {
			/* connection_info->account_id = account->account_id; */
			if (is_connection_for == _SERVICE_THREAD_TYPE_RECEIVING) {
				connection_info->receiving_server_stream      = *result_stream;
				connection_info->receiving_server_stream_status = EMF_STREAM_STATUS_CONNECTED;
			}
			else {
				connection_info->sending_server_stream        = *result_stream;
				connection_info->sending_server_stream_status = EMF_STREAM_STATUS_CONNECTED;
			}
		}
	}
#endif

	EM_SAFE_FREE(mbox_path);
	if (err_code != NULL)
		*err_code = error;
	EM_PROFILE_END(emCoreMailboxOpen);
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

#ifdef __FEATURE_KEEP_CONNECTION__

/*h.gahlaut@samsung.com :  20-oct-2010*/
/*Precaution :  When Reuse Stream feature is enabled then stream should only be closed using emcore_close_mailbox from email-service code.
mail_close should not be used directly from anywhere in code.
emcore_close_mailbox uses mail_close inside it.

mail_close is only used in emcore_connect_to_remote_mailbox and emcore_reset_streams as an exception to above rule*/

INTERNAL_FUNC int emcore_connect_to_remote_mailbox(int account_id, char *mailbox, void **mail_stream, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox[%p], mail_stream[%p], err_code[%p]", account_id, mailbox, mail_stream, err_code);
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	emf_account_t *ref_account = emcore_get_account_reference(account_id);

	if (!ref_account) {		
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed - account id[%d]", account_id);
		error = EMF_ERROR_INVALID_ACCOUNT;		
		goto FINISH_OFF;
	}
	
	ret = emcore_connect_to_remote_mailbox_with_account_info(ref_account, mailbox, mail_stream, &error);

FINISH_OFF: 
	if (err_code)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC void emcore_close_mailbox_receiving_stream()
{
	EM_DEBUG_FUNC_BEGIN("recv_thread_run [%d]", recv_thread_run);
	if (!recv_thread_run) {
		ENTER_CRITICAL_SECTION(_close_stream_lock);
		mail_close(g_receiving_thd_stream);
		g_receiving_thd_stream = NULL;
		prev_acc_id_recv_thd = 0;
		LEAVE_CRITICAL_SECTION(_close_stream_lock);
	}
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void emcore_close_mailbox_partial_body_stream()
{
	EM_DEBUG_FUNC_BEGIN();
	if (false == emcore_get_pbd_thd_state()) {
		EM_DEBUG_LOG("emcore_get_pbd_thd_state returned false");
		mail_close(g_partial_body_thd_stream);
		g_partial_body_thd_stream = NULL;
		prev_acc_id_pb_thd = 0;
	}
	EM_DEBUG_FUNC_END();
}

/* h.gahlaut@samsung.com :  21-10-2010 - 
emcore_reset_stream() function is used to reset globally stored partial body thread and receiving thread streams 
on account deletion and pdp deactivation */

INTERNAL_FUNC void emcore_reset_streams()
{
	EM_DEBUG_FUNC_BEGIN();

	emcore_close_mailbox_receiving_stream();
	emcore_close_mailbox_partial_body_stream();
	
	EM_DEBUG_FUNC_END();
	return;
}

#else /*  __FEATURE_KEEP_CONNECTION__ */

INTERNAL_FUNC int emcore_connect_to_remote_mailbox(int account_id, char *mailbox, void **mail_stream, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox[%p], mail_stream[%p], err_code[%p]", account_id, mailbox, mail_stream, err_code);
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	emf_account_t *ref_account = emcore_get_account_reference(account_id);

	if (!ref_account)  {		
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed - account id[%d]", account_id);
		error = EMF_ERROR_INVALID_ACCOUNT;		
		goto FINISH_OFF;
	}

	ret = emcore_connect_to_remote_mailbox_with_account_info(ref_account, mailbox, mail_stream, &error);

FINISH_OFF: 
	if (err_code)
		*err_code = error;
	
	return ret;
}
#endif  /*  __FEATURE_KEEP_CONNECTION__ */

INTERNAL_FUNC int emcore_close_mailbox(int account_id, void *mail_stream)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_stream[%p]", account_id, mail_stream);
	
	if (!mail_stream)  {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return false;
	}
	
#ifdef __FEATURE_KEEP_CONNECTION__
	thread_t thread_id = THREAD_SELF();

	if (thread_id == (thread_t)emcore_get_receiving_thd_id()) {
		/*  Receiving thread - Dont' Free Reuse feature enabled  */
	}
	else if (thread_id == (thread_t)emcore_get_partial_body_thd_id()) {
		/*  Partial Body Download thread - Dont' Free Reuse feature enabled  */
	}
	else {
		/*  Some other thread so free stream */
		if (g_receiving_thd_stream != mail_stream && g_partial_body_thd_stream != mail_stream)
			mail_close((MAILSTREAM *)mail_stream);
	}
#else
	mail_close((MAILSTREAM *)mail_stream);
#endif	/*  __FEATURE_KEEP_CONNECTION__ */
	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emcore_free_mailbox(emf_mailbox_t **mailbox_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_list[%p], count[%d], err_code[%p]", mailbox_list, count, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (count > 0)  {
		if (!mailbox_list || !*mailbox_list)  {
			EM_DEBUG_EXCEPTION(" mailbox_list[%p], count[%d]", mailbox_list, count);
			
			err = EMF_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		emf_mailbox_t *p = *mailbox_list;
		int i;
		
		/* EM_DEBUG_LOG("before loop"); */
		for (i = 0; i < count; i++)  {
			/* EM_DEBUG_LOG("p[%d].name [%p]", i, p[i].name); */
			/* EM_DEBUG_LOG("p[%d].alias [%p]", i, p[i].alias); */
			EM_SAFE_FREE(p[i].name);
			EM_SAFE_FREE(p[i].alias);
		}
		/* EM_DEBUG_LOG("p [%p]", p); */
		free(p); 
		*mailbox_list = NULL;
	}
	
	ret = true;
	
FINISH_OFF: 
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC void emcore_bind_mailbox_type(emf_mailbox_t *mailbox_list)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_list[%p]", mailbox_list);

	int i = 0;
	int bIsNotUserMailbox = false;
	emf_mailbox_type_item_t   *pMailboxType1 = NULL ;
	
	for (i = 0 ; i < MAX_MAILBOX_TYPE ; i++) {
		pMailboxType1 = g_mailbox_type + i;
		
		if (pMailboxType1->mailbox_name) {
			if (0 == strcmp(pMailboxType1->mailbox_name, mailbox_list->name)) {
				mailbox_list->mailbox_type = pMailboxType1->mailbox_type;
				EM_DEBUG_LOG("mailbox_list->mailbox_type[%d]", mailbox_list->mailbox_type);
				bIsNotUserMailbox = true;
				break;
			}				
		}
	}

	if (false == bIsNotUserMailbox)
		mailbox_list->mailbox_type = EMF_MAILBOX_TYPE_USER_DEFINED;

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int  emcore_send_mail_event(emf_mailbox_t *mailbox, int mail_id , int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int handle; 
	emf_event_t event_data;

	if (!mailbox || mailbox->account_id <= 0) {
		EM_DEBUG_LOG(" mailbox[%p]", mailbox);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	memset(&event_data, 0x00, sizeof(emf_event_t));

	event_data.type = EMF_EVENT_SEND_MAIL;
	event_data.account_id = mailbox->account_id;
	event_data.event_param_data_3 = EM_SAFE_STRDUP(mailbox->name);
	event_data.event_param_data_4 = mail_id;
	event_data.event_param_data_1 = NULL;
			
	if (!emcore_insert_send_event(&event_data, &handle, &err))  {
		EM_DEBUG_LOG(" emcore_insert_event failed - %d", err);
		goto FINISH_OFF;
	}
	emcore_add_transaction_info(mail_id , handle , &err);

	ret = true;
FINISH_OFF: 
	if (err_code)
		*err_code = err;
	
	return ret;
}

INTERNAL_FUNC int emcore_partial_body_thd_local_activity_sync(int *is_event_inserted, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int activity_count = 0;
	int ret = false;
	int error = EMF_ERROR_NONE;

	if (false == emstorage_get_pbd_activity_count(&activity_count, false, &error)) {
		EM_DEBUG_LOG("emstorage_get_pbd_activity_count failed [%d]", error);
		goto FINISH_OFF;
	}

	if (activity_count > 0) {

		emf_event_partial_body_thd pbd_event;

		/* Carefully initialise the event */ 
		memset(&pbd_event, 0x00, sizeof(emf_event_partial_body_thd));

		pbd_event.event_type = EMF_EVENT_LOCAL_ACTIVITY_SYNC_BULK_PBD;
		pbd_event.activity_type = EMF_EVENT_LOCAL_ACTIVITY_SYNC_BULK_PBD;

		if (false == emcore_insert_partial_body_thread_event(&pbd_event, &error)) {
			EM_DEBUG_LOG(" emcore_insert_partial_body_thread_event failed [%d]", error);
			goto FINISH_OFF;
		}
		else {
			/*Not checking for NULL here because is_event_inserted is never NULL. */
			*is_event_inserted = true;
		}
		
	}
	else {
		*is_event_inserted = false;	
	}

	ret = true;
	
	FINISH_OFF: 

	if (NULL != err_code) {
		*err_code = error;
	}

	return ret;
}

INTERNAL_FUNC int emcore_get_mailbox_by_type(int account_id, emf_mailbox_type_e mailbox_type, emf_mailbox_t *result_mailbox, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], result_mailbox [%p], err_code [%p]", account_id, result_mailbox, err_code);
	int ret = false, err = EMF_ERROR_NONE;
	emstorage_mailbox_tbl_t *mail_box_tbl_spam = NULL;

	if (result_mailbox == NULL)	{	
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mailbox_by_mailbox_type(account_id, mailbox_type, &mail_box_tbl_spam, false, &err)) {

		EM_DEBUG_LOG("emstorage_get_mailbox_by_mailbox_type failed - %d", err);
	}
	else {	
		if (mail_box_tbl_spam) {
			result_mailbox->mailbox_type = mail_box_tbl_spam->mailbox_type;
			result_mailbox->name = EM_SAFE_STRDUP(mail_box_tbl_spam->mailbox_name);
			result_mailbox->account_id = mail_box_tbl_spam->account_id;
			result_mailbox->mail_slot_size = mail_box_tbl_spam->mail_slot_size;
			if (!emstorage_free_mailbox(&mail_box_tbl_spam, 1, &err))
				EM_DEBUG_EXCEPTION(" emstorage_free_mailbox Failed [%d]", err);
			ret = true;	
		}
	}

FINISH_OFF: 
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

#ifdef __FEATURE_LOCAL_ACTIVITY__
INTERNAL_FUNC int emcore_local_activity_sync(int account_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	EM_DEBUG_LOG(">> account_id [%d], err_code [%p] ", account_id, err_code);
	
	int *activity_id_list = NULL;
	int activity_count = 0;
	int err	     	= 0;
	int handle = 0;
	int ret	= false;
	emf_event_t event_data;


	memset(&event_data, 0x00, sizeof(emf_event_t));
	

	EM_IF_NULL_RETURN_VALUE(err_code, false);

	if (account_id <= 0) {
		EM_DEBUG_EXCEPTION(" Invalid Account ID [%d] ", account_id);
		return false;
	}
		EM_DEBUG_LOG(">>> emdaemon_sync_local_activity 3 ");

	if (!emstorage_get_activity_id_list(account_id, &activity_id_list, &activity_count, ACTIVITY_DELETEMAIL, ACTIVITY_COPYMAIL, true,  &err)) {
			EM_DEBUG_LOG(">>> emdaemon_sync_local_activity 4 ");
		EM_DEBUG_EXCEPTION(" emstorage_get_activity_id_list failed [ %d] ", err);
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG(">>> emdaemon_sync_local_activity 5 ");

	if (activity_count > 0) {
		event_data.type = EMF_EVENT_LOCAL_ACTIVITY;
		event_data.account_id  = account_id;
		if (!emcore_insert_event(&event_data, &handle, &err))  {
			EM_DEBUG_LOG(" emcore_insert_event failed - %d", err);
			goto FINISH_OFF;
		}
		
		ret = true;
	}

	FINISH_OFF: 
		
	if (activity_id_list)
		emstorage_free_activity_id_list(activity_id_list, &err); 
	
	if (err_code != NULL)
		*err_code = err;
	
	return ret;

}

INTERNAL_FUNC int emcore_save_local_activity_sync(int account_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	EM_DEBUG_LOG(">> account_id [%d], err_code [%p] ", account_id, err_code);
	
	emstorage_activity_tbl_t *local_activity = NULL;
	int *activity_id_list = NULL;
	int activity_count = 0;
	int err	     	= 0;
	int ret	= false;
	int handle = 0;
	emf_event_t event_data;

	memset(&event_data, 0x00, sizeof(emf_event_t));

	EM_IF_NULL_RETURN_VALUE(err_code, false);

	if (account_id <= 0) {
		EM_DEBUG_EXCEPTION(" Invalid Account ID [%d] ", account_id);
		return false;
	}
		EM_DEBUG_LOG(">>> emdaemon_sync_local_activity 3 ");

	if (!emstorage_get_activity_id_list(account_id, &activity_id_list, &activity_count, ACTIVITY_SAVEMAIL, ACTIVITY_DELETEMAIL_SEND, true,  &err)) {
		EM_DEBUG_EXCEPTION(" emstorage_get_activity_id_list [ %d] ", err);
		goto FINISH_OFF;
	}

	
	if (activity_count > 0) {
		event_data.type = EMF_EVENT_LOCAL_ACTIVITY;
		event_data.account_id  = account_id;
		if (!emcore_insert_send_event(&event_data, &handle, &err))  {
			EM_DEBUG_LOG(" emcore_insert_event failed - %d", err);
			goto FINISH_OFF;
		}	
		
		ret = true;
	}


	FINISH_OFF: 

	if (local_activity)
		emstorage_free_local_activity(&local_activity, activity_count, NULL); 

	if (activity_id_list)
		emstorage_free_activity_id_list(activity_id_list, &err);	

	if (err_code != NULL)
		*err_code = err;
	
	return ret;

}

#endif

