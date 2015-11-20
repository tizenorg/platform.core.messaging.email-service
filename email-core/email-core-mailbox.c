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
#include "email-convert.h"
#include "email-debug-log.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-core-mailbox.h"
#include "email-core-event.h"
#include "email-network.h"
#include "email-core-mail.h"
#include "email-core-imap-mailbox.h"
#include "email-storage.h"
#include "email-core-account.h"

#include "imap4r1.h"

#ifdef __FEATURE_KEEP_CONNECTION__
static void *g_receiving_thd_stream = NULL;			/* Stores the recv thd stream for next time reuse */
static int prev_acc_id_recv_thd = 0;				/* Stores the account id for which recv thd stream is open */
extern int recv_thread_run;

static void *g_partial_body_thd_stream = NULL;		/* Stores the pb thd stream for next time reuse */
static int prev_acc_id_pb_thd = 0;					/* Stores the account id for which pb thd stream is open */

__thread email_connection_info_t *g_connection_info_list = NULL;

static pthread_mutex_t _close_stream_lock = PTHREAD_MUTEX_INITIALIZER;	/* Mutex to protect closing stream */
#endif /*  __FEATURE_KEEP_CONNECTION__ */

int emcore_imap4_send_command(MAILSTREAM *stream, imap4_cmd_t cmd_type, int *err_code);
int emcore_pop3_send_command(MAILSTREAM *stream, pop3_cmd_t cmd_type, int *err_code);

/* thread local variable for stream reuse */
__thread GList* g_recv_stream_list = NULL;
typedef struct {
	int account_id;
	int mailbox_id;
	MAILSTREAM **mail_stream;
} email_recv_stream_list_t;

/*  Binding IMAP mailbox with its function */
static email_mailbox_type_item_t  g_mailbox_type[MAX_MAILBOX_TYPE] = {
				{EMAIL_MAILBOX_TYPE_INBOX,   "INBOX" },
				/*  Naver */
				{EMAIL_MAILBOX_TYPE_INBOX,   "Inbox" },
				{EMAIL_MAILBOX_TYPE_SENTBOX, "Sent Messages"} ,
				{EMAIL_MAILBOX_TYPE_SPAMBOX, "&wqTTOLpUx3zVaA-"} ,
				{EMAIL_MAILBOX_TYPE_DRAFT,   "Drafts"} ,
				{EMAIL_MAILBOX_TYPE_TRASH,   "Deleted Messages" } ,
 				/*  AOL */
				{EMAIL_MAILBOX_TYPE_SENTBOX, "Sent"} ,
				{EMAIL_MAILBOX_TYPE_SPAMBOX, "Spam" },
				{EMAIL_MAILBOX_TYPE_DRAFT,   "Drafts"} ,
				{EMAIL_MAILBOX_TYPE_TRASH,   "Trash"},
				/* DAUM */
				{EMAIL_MAILBOX_TYPE_SPAMBOX, "&wqTTONO4ycDVaA-"},
				/* ETC */
				{EMAIL_MAILBOX_TYPE_SENTBOX, "mail/sent-mail"},
				{EMAIL_MAILBOX_TYPE_SPAMBOX, "mail/spam-mail" },
				{EMAIL_MAILBOX_TYPE_DRAFT,   "mail/saved-drafts"} ,
				{EMAIL_MAILBOX_TYPE_TRASH,   "mail/mail-trash"},
};

#ifdef __FEATURE_KEEP_CONNECTION__
email_connection_info_t* emcore_get_connection_info_by_account_id(int account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d]", account_id);
	email_connection_info_t *connection_info = g_connection_info_list;

	while(connection_info) {
		if(connection_info->account_id == account_id)
			break;
		connection_info = connection_info->next;
	}

	EM_DEBUG_FUNC_END("connection_info [%p]", connection_info);
	return connection_info;
}

int emcore_append_connection_info(email_connection_info_t *new_connection_info)
{
	EM_DEBUG_FUNC_BEGIN("new_connection_info [%p]", new_connection_info);
	email_connection_info_t *connection_info = g_connection_info_list;

	if(!new_connection_info) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if(emcore_get_connection_info_by_account_id(new_connection_info->account_id)) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_ALREADY_EXISTS");
		return EMAIL_ERROR_ALREADY_EXISTS;
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

	EM_DEBUG_FUNC_END("EMAIL_ERROR_NONE");
	return EMAIL_ERROR_NONE;
}

INTERNAL_FUNC int emcore_remove_connection_info(int account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d]", account_id);
	email_connection_info_t *connection_info = g_connection_info_list, *prev_connection_info = NULL;

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
	return EMAIL_ERROR_NONE;
}

#endif /* __FEATURE_KEEP_CONNECTION__ */


INTERNAL_FUNC void emcore_close_recv_stream_list (void)
{
	EM_DEBUG_FUNC_BEGIN();
	GList* cur = g_recv_stream_list;
	email_recv_stream_list_t* data = NULL;

	while(cur) {
		data = cur->data;
		if(data) *(data->mail_stream) = mail_close (*(data->mail_stream));
		g_recv_stream_list = g_list_delete_link (g_recv_stream_list, cur);
		g_free (data);
		cur = g_recv_stream_list;
	}

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC MAILSTREAM** emcore_get_recv_stream (char *multi_user_name, int account_id, int mailbox_id, int *error)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d]", account_id);
	GList* cur = g_recv_stream_list;
	email_recv_stream_list_t* data = NULL;
	MAILSTREAM** ret = NULL;
	int err = EMAIL_ERROR_NONE;
	email_account_t *ref_account = NULL;

	if (!(ref_account = emcore_get_account_reference(multi_user_name, account_id, false)))   {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	for ( ; cur; cur = g_list_next(cur)) {
		data = cur->data;
		if (data->account_id == account_id && data->mailbox_id == mailbox_id) {
			if (data->mail_stream == NULL || *(data->mail_stream) == NULL) {
				EM_DEBUG_LOG ("mail_stream was closed before");
				g_recv_stream_list = g_list_delete_link (g_recv_stream_list, cur);
				g_free (data);
				break;
			}

			if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
				/*  send NOOP command */
				if (!emcore_imap4_send_command(*(data->mail_stream), IMAP4_CMD_NOOP, &err)) {
					EM_DEBUG_LOG("imap4_send_command failed [%d]", err);
					EM_DEBUG_LOG ("mail_stream is not reusable");
					*(data->mail_stream) = mail_close (*(data->mail_stream));
					g_recv_stream_list = g_list_delete_link (g_recv_stream_list, cur);
					g_free (data);
					break;
				}
				else {
					EM_DEBUG_LOG ("reusable mail_stream found");
					emcore_free_account(ref_account); /* prevent */
					EM_SAFE_FREE(ref_account);
					return data->mail_stream;
				}
			}
			else if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_POP3) {
				/*  send NOOP command */
				if (!emcore_pop3_send_command(*(data->mail_stream), POP3_CMD_NOOP, &err)) {
					EM_DEBUG_LOG("emcore_pop3_send_command failed [%d]", err);
					EM_DEBUG_LOG ("mail_stream is not reusable");
					*(data->mail_stream) = mail_close (*(data->mail_stream));
					g_recv_stream_list = g_list_delete_link (g_recv_stream_list, cur);
					g_free (data);
					break;
				}
				else {
					EM_DEBUG_LOG ("reusable mail_stream found");
					emcore_free_account(ref_account); /* prevent */
					EM_SAFE_FREE(ref_account);
					return data->mail_stream;
				}
			}
		}
	}

	ret = em_malloc (sizeof(MAILSTREAM*));
	if (!ret) {
		EM_DEBUG_EXCEPTION("em_malloc error");
		goto FINISH_OFF;
	}

	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											account_id,
											mailbox_id,
											true,
											(void **)ret,
											&err)) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		EM_SAFE_FREE(ret);
		goto FINISH_OFF;
	}

	email_recv_stream_list_t *node = em_malloc (sizeof(email_recv_stream_list_t));
	if (!node) {
		EM_DEBUG_EXCEPTION ("em_malloc error");
		*ret = mail_close (*ret);
		EM_SAFE_FREE(ret);
		goto FINISH_OFF;
	}

	node->account_id = account_id;
	node->mailbox_id = mailbox_id;
	node->mail_stream = ret;

	g_recv_stream_list = g_list_prepend (g_recv_stream_list, node);

FINISH_OFF:

	if (error)
		*error = err;

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	EM_DEBUG_FUNC_END();

	return ret;
}


/* description
 *    get local mailbox list
 */
INTERNAL_FUNC int emcore_get_mailbox_list(char *multi_user_name, int account_id, email_mailbox_t **mailbox_list,
										int *p_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_list[%p], p_count[%p], err_code[%p]",
							account_id, mailbox_list, p_count, err_code);

	if (account_id <= 0 || !mailbox_list || !p_count)  {
		EM_DEBUG_EXCEPTION("PARAM Failed account_id[%d], mailbox_list[%p], p_count[%p]",
							account_id, mailbox_list, p_count);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	emstorage_mailbox_tbl_t *local_mailbox_list = NULL;
	email_account_t *ref_account = NULL;
	int i, count = 512;

	/* get mailbox list from mailbox table */
	if (!(ref_account = emcore_get_account_reference(multi_user_name, account_id, false))) {
		EM_DEBUG_EXCEPTION(" emcore_get_account_reference failed - %d", account_id);
		error = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mailbox_list(multi_user_name, ref_account->account_id,
									EMAIL_MAILBOX_ALL, EMAIL_MAILBOX_SORT_BY_NAME_ASC,
									&count, &local_mailbox_list, true, &error))  {
		EM_DEBUG_EXCEPTION(" emstorage_get_mailbox failed - %d", error);
		goto FINISH_OFF;
	}

	if (count > 0)  {
		if (!(*mailbox_list = em_malloc(sizeof(email_mailbox_t) * count)))  {
			EM_DEBUG_EXCEPTION(" mailloc failed...");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		memset(*mailbox_list, 0x00, (sizeof(email_mailbox_t) * count));

		for (i = 0; i < count; i++)  {
			em_convert_mailbox_tbl_to_mailbox(local_mailbox_list + i, (*mailbox_list) + i);
		}
	}
	else
		mailbox_list = NULL;

	* p_count = count;

	ret = true;

FINISH_OFF:
	if (local_mailbox_list != NULL)
		emstorage_free_mailbox(&local_mailbox_list, count, NULL);

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END();
	return ret;
}

/* description
 *    get imap sync mailbox list
 */
int emcore_get_mailbox_list_to_be_sync(char *multi_user_name, int account_id, email_mailbox_t **mailbox_list, int *p_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_list[%p], p_count[%p], err_code[%p]", account_id, mailbox_list, p_count, err_code);

	if (account_id <= 0 || !mailbox_list || !p_count)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_list[%p], p_count[%p]", account_id, mailbox_list, p_count);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	email_mailbox_t *tmp_mailbox_list = NULL;
	emstorage_mailbox_tbl_t *mailbox_tbl_list = NULL;
	email_account_t *ref_account = NULL;
	int i, count = 512;

	/* get mailbox list from mailbox table */
	if (!(ref_account = emcore_get_account_reference(multi_user_name, account_id, false))) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed - %d", account_id);
		error = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mailbox_list(multi_user_name, ref_account->account_id, 0,
									EMAIL_MAILBOX_SORT_BY_TYPE_ASC, &count,
									&mailbox_tbl_list, true, &error))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox failed - %d", error);
		goto FINISH_OFF;
	}

	if (count > 0)  {
		if (!(tmp_mailbox_list = em_malloc(sizeof(email_mailbox_t) * count)))  {
			EM_DEBUG_EXCEPTION("malloc failed...");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		memset(tmp_mailbox_list, 0x00, (sizeof(email_mailbox_t) * count));

		for (i = 0; i < count; i++)  {
			em_convert_mailbox_tbl_to_mailbox(mailbox_tbl_list + i, tmp_mailbox_list + i);
		}
	}
	else
		tmp_mailbox_list = NULL;
	*p_count = count;
	ret = true;

FINISH_OFF:

	*mailbox_list = tmp_mailbox_list;

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (mailbox_tbl_list != NULL)
		emstorage_free_mailbox(&mailbox_tbl_list, count, NULL);

	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("error [%d]", error);
	return ret;
}

INTERNAL_FUNC int emcore_get_mail_count(char *multi_user_name, email_mailbox_t *mailbox, int *total, int *unseen, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], total[%p], unseen[%p], err_code[%p]", mailbox, total, unseen, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (!mailbox)  {
		EM_DEBUG_EXCEPTION(" mailbox[%p], total[%p], unseen[%p]", mailbox, total, unseen);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mail_count(multi_user_name, mailbox->account_id, mailbox->mailbox_id, total, unseen, true, &err))  {
		EM_DEBUG_EXCEPTION(" emstorage_get_mail_count failed - %d", err);
		goto FINISH_OFF;
	}


	ret = true;

FINISH_OFF:

	if (err_code != NULL)
		*err_code = err;

	return ret;
}

INTERNAL_FUNC int emcore_create_mailbox(char *multi_user_name, email_mailbox_t *new_mailbox, int on_server, int server_type, int slot_size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("new_mailbox[%p], err_code[%p]", new_mailbox, err_code);
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	emstorage_mailbox_tbl_t *local_mailbox = NULL;
	email_account_t *account_ref = NULL;
	char *enc_mailbox_name = NULL;
	int incomming_server_type = 0;
	int mail_slot_size = 25;

	if (new_mailbox == NULL || new_mailbox->mailbox_name == NULL)  {
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (server_type > 0 && slot_size > 0) {
		incomming_server_type = server_type;
		mail_slot_size = slot_size;
	}
	else {
		account_ref = emcore_get_account_reference(multi_user_name, new_mailbox->account_id, false);
		if (!account_ref) {
			EM_DEBUG_EXCEPTION("Invalid account_id [%d]", new_mailbox->account_id);
			err = EMAIL_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF;
		}

		incomming_server_type = account_ref->incoming_server_type;
		mail_slot_size = account_ref->default_mail_slot_size;

		emcore_free_account(account_ref);
		EM_SAFE_FREE(account_ref);
	}

	/* converting UTF-8 to UTF-7 except EAS */
	if (incomming_server_type != EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
		if (!emcore_get_encoded_mailbox_name(new_mailbox->mailbox_name, &enc_mailbox_name, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_encoded_mailbox_name failed [%d]", err);
			goto FINISH_OFF;
		}
		EM_SAFE_FREE(new_mailbox->mailbox_name);
		new_mailbox->mailbox_name = enc_mailbox_name;

		EM_SAFE_FREE(new_mailbox->alias);
		new_mailbox->alias = emcore_get_alias_of_mailbox((const char *)new_mailbox->mailbox_name);
	}

	if (on_server) {
		/* Create a mailbox from Sever */
		if (!emcore_create_imap_mailbox(multi_user_name, new_mailbox, &err)) {
			EM_DEBUG_EXCEPTION("Creating a mailbox on server failed.");
			goto FINISH_OFF;
		}
		else
			EM_DEBUG_LOG("Creating a mailbox on server succeeded.");
	}

	local_mailbox = em_malloc(sizeof(emstorage_mailbox_tbl_t)); /*prevent 60604*/

	if(local_mailbox == NULL) {
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	em_convert_mailbox_to_mailbox_tbl(new_mailbox, local_mailbox);

	local_mailbox->mail_slot_size = mail_slot_size;

	if (!emstorage_add_mailbox(multi_user_name, local_mailbox, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_add_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}

	new_mailbox->mailbox_id = local_mailbox->mailbox_id;
	ret = true;

FINISH_OFF:

	if(local_mailbox)
		emstorage_free_mailbox(&local_mailbox, 1, NULL);

	if (err_code)
		*err_code = err;

	return ret;
}

INTERNAL_FUNC int emcore_delete_mailbox(char *multi_user_name, int input_mailbox_id, int input_on_server, int input_recursive)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d] input_on_server[%d] input_recursive[%d]", input_mailbox_id, input_on_server, input_recursive);

	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int mailbox_count = 0;
	char *old_mailbox_name = NULL;
	emstorage_mailbox_tbl_t *target_mailbox = NULL;
	emstorage_mailbox_tbl_t *target_mailbox_array = NULL;

	if (input_mailbox_id <= 0)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &target_mailbox)) != EMAIL_ERROR_NONE || !target_mailbox) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", err);
		goto FINISH_OFF;
	}

	old_mailbox_name = g_strdup(target_mailbox->mailbox_name);

#ifdef __FEATURE_DELETE_MAILBOX_RECURSIVELY__
	if(input_recursive) {
		/* Getting children mailbox list */
		if(!emstorage_get_child_mailbox_list(multi_user_name,
											target_mailbox->account_id,
											target_mailbox->mailbox_name,
											&mailbox_count,
											&target_mailbox_array,
											false,
											&err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_child_mailbox_list failed. [%d]", err);
			goto FINISH_OFF;
		}

		if (target_mailbox)
			emstorage_free_mailbox(&target_mailbox, 1, NULL);

		target_mailbox = NULL;
	}
	else
#endif /* __FEATURE_DELETE_MAILBOX_RECURSIVELY__ */
	{
		target_mailbox_array = target_mailbox;
		mailbox_count        = 1;
		target_mailbox       = NULL;
	}

	/* Remove mailboxes */
	for (i = 0; i < mailbox_count; i++) {
		if (strncasecmp(target_mailbox_array[i].mailbox_name, old_mailbox_name, strlen(old_mailbox_name))) {
			if (!strcasestr(target_mailbox_array[i].mailbox_name, "/")) {
				EM_DEBUG_LOG("Not folder");
				continue;
			}
		}

		EM_DEBUG_LOG("Deleting mailbox_id [%d]", target_mailbox_array[i].mailbox_id);
		if (input_on_server) {
			EM_DEBUG_LOG("Delete the mailbox in Sever >>> ");
			if  (!emcore_delete_imap_mailbox(multi_user_name, target_mailbox_array[i].mailbox_id, &err)) {
				EM_DEBUG_EXCEPTION("Delete the mailbox in server : failed [%d]", err);
				goto FINISH_OFF;
			}
			else
				EM_DEBUG_LOG("Delete the mailbox in server : success");
		}

		if (!emcore_delete_all_mails_of_mailbox(multi_user_name,
												target_mailbox_array[i].account_id,
												target_mailbox_array[i].mailbox_id,
												0,
												false,
												&err))  {
			if (err != EMAIL_ERROR_MAIL_NOT_FOUND) {
				EM_DEBUG_EXCEPTION("emcore_delete_all_mails_of_mailbox failed [%d]", err);
				goto FINISH_OFF;
			}
		}

		if (!emstorage_delete_mailbox(multi_user_name, target_mailbox_array[i].account_id, -1, target_mailbox_array[i].mailbox_id, true, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_delete_mailbox failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:

	EM_SAFE_FREE(old_mailbox_name);

	if (target_mailbox)
		emstorage_free_mailbox(&target_mailbox, 1, NULL);

	if (target_mailbox_array)
		emstorage_free_mailbox(&target_mailbox_array, mailbox_count, NULL);

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}


INTERNAL_FUNC int emcore_delete_mailbox_ex(char *multi_user_name, int input_account_id, int *input_mailbox_id_array, int input_mailbox_id_count, int input_on_server, int input_recursive)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d] input_mailbox_id_array[%p] input_mailbox_id_count[%d] input_on_server[%d] input_recursive[%d]", input_mailbox_id_array, input_mailbox_id_array, input_mailbox_id_count, input_on_server, input_recursive);
	int err = EMAIL_ERROR_NONE;
	int i = 0;

	if(input_account_id == 0 || input_mailbox_id_count <= 0 || input_mailbox_id_array == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if((err = emstorage_set_field_of_mailbox_with_integer_value(multi_user_name, input_account_id, input_mailbox_id_array, input_mailbox_id_count, "deleted_flag", 1, true)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_set_field_of_mailbox_with_integer_value failed[%d]", err);
		goto FINISH_OFF;
	}

	for(i = 0; i < input_mailbox_id_count; i++) {
		if((err = emcore_delete_mailbox(multi_user_name, input_mailbox_id_array[i] , input_on_server, input_recursive)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_delete_mailbox failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_delete_mailbox_all(char *multi_user_name, email_mailbox_t *mailbox, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN(" mailbox[%p], err_code[%p]", mailbox, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (mailbox == NULL) {
		EM_DEBUG_EXCEPTION(" mailbox[%p]", mailbox);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emcore_delete_all_mails_of_mailbox(multi_user_name,
											mailbox->account_id,
											mailbox->mailbox_id,
											0,
											0, /*NULL, */
											&err)) {
		if (err != EMAIL_ERROR_MAIL_NOT_FOUND) {
			EM_DEBUG_EXCEPTION(" emcore_delete_all_mails_of_mailbox failed - %d", err);
			goto FINISH_OFF;
		}
	}

	if (!emstorage_delete_mailbox(multi_user_name, mailbox->account_id, -1, mailbox->mailbox_id, true, &err)) {
		EM_DEBUG_EXCEPTION(" emstorage_delete_mailbox failed - %d", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("err[%d]", err);
	return ret;
}

INTERNAL_FUNC int emcore_rename_mailbox(char *multi_user_name,
										int input_mailbox_id,
										char *input_new_mailbox_name,
										char *input_new_mailbox_alias,
										void *input_eas_data,
										int input_eas_data_length,
										int input_on_server,
										int input_recursive,
										int handle_to_be_published)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d] input_new_mailbox_name[%p] input_new_mailbox_alias[%p] input_eas_data[%p] input_eas_data_length[%d] input_on_server[%d] input_recursive[%d] handle_to_be_published[%d]", input_mailbox_id, input_new_mailbox_name, input_new_mailbox_alias, input_eas_data, input_eas_data_length, input_on_server, input_recursive, handle_to_be_published);

	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int mailbox_count = 0;
	email_account_t *account_ref = NULL;
	emstorage_mailbox_tbl_t *target_mailbox = NULL;
	emstorage_mailbox_tbl_t *target_mailbox_array = NULL;
	char *renamed_mailbox_name = NULL;
	char *old_mailbox_name = NULL;
	char *enc_mailbox_name = NULL;
	char *new_mailbox_name = NULL;

	if (input_mailbox_id == 0 || input_new_mailbox_name == NULL || input_new_mailbox_alias == NULL)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &target_mailbox)) != EMAIL_ERROR_NONE || !target_mailbox) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", err);
		goto FINISH_OFF;
	}

	account_ref = emcore_get_account_reference(multi_user_name, target_mailbox->account_id, false);
	if (account_ref == NULL) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed.");
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_SEC("target_mailbox->mailbox_name [%s]", target_mailbox->mailbox_name);
	old_mailbox_name = EM_SAFE_STRDUP(target_mailbox->mailbox_name);
	new_mailbox_name = EM_SAFE_STRDUP(input_new_mailbox_name);

	if(account_ref->incoming_server_type != EMAIL_SERVER_TYPE_ACTIVE_SYNC)
	{
		if (!emcore_get_encoded_mailbox_name(new_mailbox_name, &enc_mailbox_name, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_encoded_mailbox_name failed [%d]", err);
			goto FINISH_OFF;
		}
		EM_SAFE_FREE(new_mailbox_name);
		new_mailbox_name = enc_mailbox_name;
	}

	if (input_on_server) {
		EM_DEBUG_LOG("Rename the mailbox in Sever >>> ");

		if ((err = emcore_rename_mailbox_on_imap_server(multi_user_name,
														target_mailbox->account_id,
														target_mailbox->mailbox_id,
														target_mailbox->mailbox_name,
														new_mailbox_name,
														handle_to_be_published)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_rename_mailbox_on_imap_server failed [%d]", err);
			goto FINISH_OFF;
		}
		else
			EM_DEBUG_LOG("Rename the mailbox on server : success");
	}

#ifdef __FEATURE_RENAME_MAILBOX_RECURSIVELY__
	if(account_ref->incoming_server_type != EMAIL_SERVER_TYPE_ACTIVE_SYNC && input_recursive) {
		/* Getting children mailbox list */
		if(!emstorage_get_child_mailbox_list(multi_user_name,
											target_mailbox->account_id,
											target_mailbox->mailbox_name,
											&mailbox_count,
											&target_mailbox_array,
											false,
											&err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_child_mailbox_list failed. [%d]", err);
			goto FINISH_OFF;
		}

		if (target_mailbox)
			emstorage_free_mailbox(&target_mailbox, 1, NULL);
		target_mailbox = NULL;
	}
	else
#endif /* __FEATURE_RENAME_MAILBOX_RECURSIVELY__ */
	{
		target_mailbox_array = target_mailbox;
		mailbox_count        = 1;
		target_mailbox       = NULL;
	}

	/* Rename mailboxes */
	for(i = 0; i < mailbox_count ; i++) {
		EM_DEBUG_LOG_SEC("Rename mailbox_id [%d], mailbox_name [%s], old_mailbox_name [%s]",
							target_mailbox_array[i].mailbox_id,
							target_mailbox_array[i].mailbox_name,
							old_mailbox_name);

		if (old_mailbox_name && strcasecmp(target_mailbox_array[i].mailbox_name, old_mailbox_name)) {
			if (!strstr(target_mailbox_array[i].mailbox_name, "/")) {
				EM_DEBUG_LOG("Not folder");
				continue;
			}
		}

		if (input_mailbox_id == target_mailbox_array[i].mailbox_id) {
			if ((err = emstorage_rename_mailbox(multi_user_name,
												target_mailbox_array[i].mailbox_id,
												new_mailbox_name,
												input_new_mailbox_alias,
												input_eas_data,
												input_eas_data_length,
												true)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emstorage_rename_mailbox failed [%d]", err);
				goto FINISH_OFF;
			}
		}
		else {
			EM_DEBUG_LOG_SEC("target_mailbox_array[i].mailbox_name[%s] "
							 "old_mailbox_name[%s] "
							 "new_mailbox_name [%s]",
							 target_mailbox_array[i].mailbox_name,
							 old_mailbox_name,
							 new_mailbox_name);
			renamed_mailbox_name = em_replace_string(target_mailbox_array[i].mailbox_name,
													old_mailbox_name,
													new_mailbox_name);
			EM_DEBUG_LOG_SEC("renamed_mailbox_name[%s]", renamed_mailbox_name);

			if ((err = emstorage_rename_mailbox(multi_user_name,
												target_mailbox_array[i].mailbox_id,
												renamed_mailbox_name,
												target_mailbox_array[i].alias,
												NULL,
												0,
												true)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emstorage_rename_mailbox failed [%d]", err);
				goto FINISH_OFF;
			}

			EM_SAFE_FREE(renamed_mailbox_name);
		}
	}

FINISH_OFF:

	EM_SAFE_FREE(renamed_mailbox_name);
	EM_SAFE_FREE(old_mailbox_name);
	EM_SAFE_FREE(new_mailbox_name);

	if (account_ref) {
		emcore_free_account(account_ref);
		EM_SAFE_FREE(account_ref);
	}

	if (target_mailbox)
		emstorage_free_mailbox(&target_mailbox, 1, NULL);

	if (target_mailbox_array)
		emstorage_free_mailbox(&target_mailbox_array, mailbox_count, NULL);

	return err;
}

#ifdef __FEATURE_KEEP_CONNECTION__
extern long smtp_send(SENDSTREAM *stream, char *command, char *args);
#endif /* __FEATURE_KEEP_CONNECTION__ */

INTERNAL_FUNC int emcore_connect_to_remote_mailbox_with_account_info(char *multi_user_name,
																		email_account_t *account,
																		int input_mailbox_id,
																		int reusable,
									/*either MAILSTREAM or SENDSTREAM*/ void **result_stream,
																		int *err_code)
{
	EM_PROFILE_BEGIN(emCoreMailboxOpen);
	EM_DEBUG_FUNC_BEGIN("account[%p], input_mailbox_id[%d], mail_stream[%p], err_code[%p]", account, input_mailbox_id,
                                                                                              result_stream, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	email_session_t *session = NULL;
	char *mbox_path = NULL;
	int is_connection_for = _SERVICE_THREAD_TYPE_NONE;
	int connection_retry_count = 0;
	emstorage_mailbox_tbl_t* mailbox = NULL;
	char *mailbox_name = NULL;

	if (account == NULL) {
		EM_DEBUG_EXCEPTION("Invalid Parameter.");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emcore_get_current_session(&session)) {
		EM_DEBUG_EXCEPTION("emcore_get_current_session failed...");
		error = EMAIL_ERROR_SESSION_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (input_mailbox_id == 0 || input_mailbox_id != EMAIL_CONNECT_FOR_SENDING)
		is_connection_for = _SERVICE_THREAD_TYPE_RECEIVING;
	else
		is_connection_for = _SERVICE_THREAD_TYPE_SENDING;

#ifdef __FEATURE_KEEP_CONNECTION__
	email_connection_info_t *connection_info = emcore_get_connection_info_by_account_id(account->account_id);

	if (connection_info && reusable) {
		if (is_connection_for == _SERVICE_THREAD_TYPE_RECEIVING) {
			if(connection_info->receiving_server_stream_status == EMAIL_STREAM_STATUS_CONNECTED)
				*result_stream = connection_info->receiving_server_stream;
		}
		else {
			if(connection_info->sending_server_stream_status == EMAIL_STREAM_STATUS_CONNECTED)
				*result_stream = connection_info->sending_server_stream;
		}
	}

	if (*result_stream)
		EM_DEBUG_LOG("Stream reuse desired");
#endif

	/* set current network error as EMAIL_ERROR_NONE before network operation */
	session->error = EMAIL_ERROR_NONE;
	emcore_set_network_error(EMAIL_ERROR_NONE);

	if (input_mailbox_id == EMAIL_CONNECT_FOR_SENDING) {
		mailbox_name = EM_SAFE_STRDUP(ENCODED_PATH_SMTP);
	} else if (input_mailbox_id == 0) {
		mailbox_name = NULL;
	} else {
		if ((error = emstorage_get_mailbox_by_id (multi_user_name, input_mailbox_id, &mailbox)) != EMAIL_ERROR_NONE || !mailbox) {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", error);
			goto FINISH_OFF;
		}
		mailbox_name = EM_SAFE_STRDUP (mailbox->mailbox_name);
	}

	if (is_connection_for == _SERVICE_THREAD_TYPE_RECEIVING) {
		/*  open pop3/imap server */
		if (!emcore_get_long_encoded_path_with_account_info(multi_user_name,
															account,
															mailbox_name,
															'/',
															&mbox_path,
															&error)) {
			EM_DEBUG_EXCEPTION("emcore_get_long_encoded_path failed - %d", error);
			session->error = error;
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG_SEC("open mail connection to mbox_path [%s]", mbox_path);

		session->auth = 0; /*  ref_account->receiving_auth ? 1  :  0 */

		if (!(*result_stream = mail_open(*result_stream, mbox_path, IMAP_2004_LOG))) {
			EM_DEBUG_EXCEPTION("mail_open failed. session->error[%d], session->network[%d]",
								session->error, session->network);
			*result_stream = mail_close(*result_stream);

			if (account->account_id > 0 && (session->network == EMAIL_ERROR_XOAUTH_BAD_REQUEST ||
				session->network == EMAIL_ERROR_XOAUTH_INVALID_UNAUTHORIZED)) {
				error = emcore_refresh_xoauth2_access_token (multi_user_name, account->account_id);

				if (error != EMAIL_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("emcore_refresh_xoauth2_access_token failed. [%d]", error);
				} else {
					while (*result_stream == NULL && connection_retry_count < 3) {

						sleep(3); /* wait for updating access token */
						if (!(*result_stream = mail_open(*result_stream, mbox_path, IMAP_2004_LOG))) {
							EM_DEBUG_LOG("mail_open failed. session->error[%d], session->network[%d]",
											session->error, session->network);
						}
						connection_retry_count++;
						EM_DEBUG_LOG ("connection_retry_count [%d]", connection_retry_count);
					}
				}
			} else if ((session->error == EMAIL_ERROR_TLS_SSL_FAILURE ||
						session->error == EMAIL_ERROR_CANNOT_NEGOTIATE_TLS)) {
				char *replaced_mbox_path = NULL;
				replaced_mbox_path = em_replace_string(mbox_path, "}", "/force_tls_v1_0}");

				if (replaced_mbox_path != NULL &&
					!(*result_stream = mail_open(*result_stream, replaced_mbox_path, IMAP_2004_LOG))) {
					EM_DEBUG_EXCEPTION("retry --> mail_open failed. session->error[%d], session->network[%d]",
										session->error, session->network);
					*result_stream = mail_close(*result_stream);
				}

				EM_SAFE_FREE(replaced_mbox_path);
			}

			if (*result_stream == NULL) { /* Finally, connection failed */
				if (session->error == EMAIL_ERROR_UNKNOWN || session->error == EMAIL_ERROR_NONE)
					session->error = EMAIL_ERROR_CONNECTION_FAILURE;
				error = session->error;
				goto FINISH_OFF;
			}
		}
	} else {
		/*  open smtp server */
		char *host_list[2] = {NULL, NULL};

#ifdef __FEATURE_KEEP_CONNECTION__
		if (*result_stream) {
			int send_ret = 0;
			/* Check whether connection is avaiable */
			send_stream = *result_stream;
			/*
			send_ret = smtp_send(send_stream, "RSET", 0);

			if (send_ret != SMTP_RESPONSE_OK) {
				EM_DEBUG_EXCEPTION("[SMTP] RSET --> [%s]", send_stream->reply);
				send_stream = NULL;
			}
			*/
		}
#endif
		if (!*result_stream) {
			if (!emcore_get_long_encoded_path_with_account_info(multi_user_name,
																account,
																mailbox_name,
																0,
																&mbox_path,
																&error)) {
				EM_DEBUG_EXCEPTION(" emcore_get_long_encoded_path failed - %d", error);
				session->error = error;
				goto FINISH_OFF;
			}

			EM_DEBUG_LOG_SEC("open SMTP connection to mbox_path [%s]", mbox_path);

			session->auth = account->outgoing_server_need_authentication ? 1  :  0;

			host_list[0] = mbox_path;

			if (!(*result_stream = smtp_open (host_list, 1))) {
				EM_DEBUG_EXCEPTION_SEC("smtp_open error : outgoing_server_secure_connection[%d] "
									   "session->error[%d] session->network[%d]",
										  account->outgoing_server_secure_connection,
										  session->error, session->network);

				if (account->account_id > 0 && (session->network == EMAIL_ERROR_XOAUTH_BAD_REQUEST ||
					 session->network == EMAIL_ERROR_XOAUTH_INVALID_UNAUTHORIZED)) {
					*result_stream = smtp_close(*result_stream);

					error = emcore_refresh_xoauth2_access_token(multi_user_name, account->account_id);
					if (error != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION ("emcore_refresh_xoauth2_access_token failed. [%d]", error);
						if ((session->error == EMAIL_ERROR_UNKNOWN) || (session->error == EMAIL_ERROR_NONE))
							session->error = EMAIL_ERROR_CONNECTION_FAILURE;
						error = session->error;
						goto FINISH_OFF;
					}

					sleep(2); /* wait for updating access token */
					if (!(*result_stream = smtp_open(host_list, 1))) {
						if (session->network != EMAIL_ERROR_NONE)
							session->error = session->network;
						if ((session->error == EMAIL_ERROR_UNKNOWN) || (session->error == EMAIL_ERROR_NONE))
							session->error = EMAIL_ERROR_CONNECTION_FAILURE;

						error = session->error;
						goto FINISH_OFF;
					}
				} else {
					error = EMAIL_ERROR_CONNECTION_FAILURE;
					goto FINISH_OFF;
				}
			}
		}
	}

	ret = true;

FINISH_OFF:

#ifdef __FEATURE_KEEP_CONNECTION__
	if (ret == true) {
		if(!connection_info) {
			connection_info = em_malloc(sizeof(email_connection_info_t));
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
				connection_info->receiving_server_stream_status = EMAIL_STREAM_STATUS_CONNECTED;
			}
			else {
				connection_info->sending_server_stream        = *result_stream;
				connection_info->sending_server_stream_status = EMAIL_STREAM_STATUS_CONNECTED;
			}
		}
	}
#endif

	EM_SAFE_FREE(mbox_path);
	EM_SAFE_FREE(mailbox_name);

	if (mailbox) {
		emstorage_free_mailbox(&mailbox, 1, NULL);
	}

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

INTERNAL_FUNC int emcore_connect_to_remote_mailbox(char *multi_user_name,
													int account_id,
													char *mailbox,
													int reusable,
													void **mail_stream,
													int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox[%p], reusable[%d], mail_stream[%p], err_code[%p]",
						account_id, mailbox, reusable, mail_stream, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	email_account_t *ref_account = NULL;

	ref_account = emcore_get_account_reference(multi_user_name, account_id, false);
	if (!ref_account) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed - account id[%d]", account_id);
		error = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	ret = emcore_connect_to_remote_mailbox_with_account_info(multi_user_name,
																ref_account,
																mailbox,
																reusable,
																mail_stream,
																&error);

FINISH_OFF:

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

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
		g_receiving_thd_stream = mail_close (g_receiving_thd_stream);
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
		g_partial_body_thd_stream = mail_close (g_partial_body_thd_stream);
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

INTERNAL_FUNC int emcore_connect_to_remote_mailbox(char *multi_user_name,
													int account_id,
													int input_mailbox_id,
													int reusable,
													void **mail_stream,
													int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], input_mailbox_id[%d], reusable[%d], mail_stream[%p], err_code[%p]",
						account_id, input_mailbox_id, reusable, mail_stream, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	email_session_t *session = NULL;
	email_account_t *ref_account = NULL;

	ref_account = emcore_get_account_reference(multi_user_name, account_id, false);
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed - account id[%d]", account_id);
		error = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

/*    Several threads call it, so check event status is disabled
	if (!emcore_check_thread_status()) {
		error = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
*/
	if (!emnetwork_check_network_status(&error)) {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", error);
		goto FINISH_OFF;
	}

	if (!emcore_get_empty_session(&session)) {
		EM_DEBUG_EXCEPTION("emcore_get_empty_session failed...");
/*		error = EMAIL_ERROR_SESSION_NOT_FOUND;
		goto FINISH_OFF; */
	}

	ret = emcore_connect_to_remote_mailbox_with_account_info(multi_user_name,
																ref_account,
																input_mailbox_id,
																reusable,
																mail_stream,
																&error);
	EM_DEBUG_LOG("ret[%d] incoming_server_type[%d] input_mailbox_id[%d]",
					ret, ref_account->incoming_server_type, input_mailbox_id);

	if (ret == EMAIL_ERROR_NONE && input_mailbox_id == EMAIL_CONNECT_FOR_SENDING) {
		SENDSTREAM *send_stream = (SENDSTREAM*)*mail_stream;

		if (send_stream && send_stream->protocol.esmtp.ok) {
			if (send_stream->protocol.esmtp.size.ok && send_stream->protocol.esmtp.size.limit > 0) {
				EM_DEBUG_LOG("Server size limit : %ld", send_stream->protocol.esmtp.size.limit);
				if (send_stream->protocol.esmtp.size.limit != ref_account->outgoing_server_size_limit) {
					emstorage_set_field_of_accounts_with_integer_value(multi_user_name, account_id, "outgoing_server_size_limit", send_stream->protocol.esmtp.size.limit, true);
				}
			}
		}
	}
	else if (ret == EMAIL_ERROR_NONE && ref_account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4
			&& input_mailbox_id != EMAIL_CONNECT_FOR_SENDING) {
		IMAPCAP *capability = NULL;
		/* check capability changes */
		capability = imap_cap((MAILSTREAM*)*mail_stream);
		EM_DEBUG_LOG("capability [%p]", capability);
		if (capability) {
			EM_DEBUG_LOG("idle [%d] retrieval_mode[%d]", capability->idle, ref_account->retrieval_mode);
			if (capability->idle != ((ref_account->retrieval_mode & EMAIL_IMAP4_IDLE_SUPPORTED) == EMAIL_IMAP4_IDLE_SUPPORTED)) {
				if (capability->idle)
					ref_account->retrieval_mode += EMAIL_IMAP4_IDLE_SUPPORTED;
				else
					ref_account->retrieval_mode -= EMAIL_IMAP4_IDLE_SUPPORTED;
				emstorage_set_field_of_accounts_with_integer_value(multi_user_name, account_id, "retrieval_mode", ref_account->retrieval_mode, true);
			}
		}
	}


FINISH_OFF:

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	emcore_clear_session(session);

	if (err_code)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
#endif  /*  __FEATURE_KEEP_CONNECTION__ */

INTERNAL_FUNC int emcore_close_mailbox(int account_id, void *mail_stream)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_stream[%p]", account_id, mail_stream);

	if (!mail_stream)  {
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

INTERNAL_FUNC void emcore_free_mailbox_list(email_mailbox_t **mailbox_list, int count)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_list[%p], count[%d]", mailbox_list, count);

	if (count <= 0 || !mailbox_list || !*mailbox_list)  {
		EM_DEBUG_EXCEPTION("INVALID_PARAM: mailbox_list[%p], count[%d]", mailbox_list, count);
		return;
	}

	email_mailbox_t *p = *mailbox_list;
	int i;

	for (i = 0; i < count; i++)
		emcore_free_mailbox(p+i);

	EM_SAFE_FREE(p);
	*mailbox_list = NULL;

	EM_DEBUG_FUNC_END();
}


INTERNAL_FUNC void emcore_free_mailbox(email_mailbox_t *mailbox)
{
	EM_DEBUG_FUNC_BEGIN();

	if (!mailbox) return;

	EM_SAFE_FREE(mailbox->mailbox_name);
	EM_SAFE_FREE(mailbox->alias);
	EM_SAFE_FREE(mailbox->eas_data);

	EM_DEBUG_FUNC_END();
}


INTERNAL_FUNC int emcore_free_internal_mailbox(email_internal_mailbox_t **mailbox_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_list[%p], count[%d], err_code[%p]", mailbox_list, count, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (count > 0)  {
		if (!mailbox_list || !*mailbox_list)  {
			EM_DEBUG_EXCEPTION(" mailbox_list[%p], count[%d]", mailbox_list, count);

			err = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		email_internal_mailbox_t *p = *mailbox_list;
		int i;

		/* EM_DEBUG_LOG("before loop"); */
		for (i = 0; i < count; i++)  {
			EM_SAFE_FREE(p[i].mailbox_name);
			EM_SAFE_FREE(p[i].alias);
		}
		/* EM_DEBUG_LOG("p [%p]", p); */
		EM_SAFE_FREE (p);
		*mailbox_list = NULL;
	}

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC void emcore_bind_mailbox_type(email_internal_mailbox_t *mailbox_list)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_list[%p]", mailbox_list);

	int i = 0;
	int bIsNotUserMailbox = false;
	email_mailbox_type_item_t   *pMailboxType1 = NULL ;

	for (i = 0 ; i < MAX_MAILBOX_TYPE ; i++) {
		pMailboxType1 = g_mailbox_type + i;
		if (0 == EM_SAFE_STRCMP(pMailboxType1->mailbox_name, mailbox_list->mailbox_name)) { /*prevent 24662*/
			mailbox_list->mailbox_type = pMailboxType1->mailbox_type;
			EM_DEBUG_LOG("mailbox_list->mailbox_type[%d]", mailbox_list->mailbox_type);
			bIsNotUserMailbox = true;
			break;
		}
	}

	if (false == bIsNotUserMailbox)
		mailbox_list->mailbox_type = EMAIL_MAILBOX_TYPE_USER_DEFINED;

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int  emcore_send_mail_event(email_mailbox_t *mailbox, int mail_id , int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int handle;
	email_event_t *event_data = NULL;

	if (!mailbox || mailbox->account_id <= 0) {
		EM_DEBUG_LOG(" mailbox[%p]", mailbox);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	event_data = em_malloc(sizeof(email_event_t));
	if (event_data == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	event_data->type = EMAIL_EVENT_SEND_MAIL;
	event_data->account_id = mailbox->account_id;
	event_data->event_param_data_4 = mail_id;
	event_data->event_param_data_1 = NULL;
	event_data->event_param_data_5 = mailbox->mailbox_id;

	if (!emcore_insert_event_for_sending_mails(event_data, &handle, &err))  {
		EM_DEBUG_LOG(" emcore_insert_event failed - %d", err);
		goto FINISH_OFF;
	}

	emcore_add_transaction_info(mail_id , handle , &err);

	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (err_code)
		*err_code = err;

	return ret;
}

INTERNAL_FUNC int emcore_partial_body_thd_local_activity_sync(char *multi_user_name, int *is_event_inserted, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int activity_count = 0;
	int ret = false;
	int error = EMAIL_ERROR_NONE;

	if (false == emstorage_get_pbd_activity_count(multi_user_name, &activity_count, false, &error)) {
		EM_DEBUG_LOG("emstorage_get_pbd_activity_count failed [%d]", error);
		goto FINISH_OFF;
	}

	if (activity_count > 0) {

		email_event_partial_body_thd pbd_event;

		/* Carefully initialise the event */
		memset(&pbd_event, 0x00, sizeof(email_event_partial_body_thd));

		pbd_event.event_type = EMAIL_EVENT_LOCAL_ACTIVITY_SYNC_BULK_PBD;
		pbd_event.activity_type = EMAIL_EVENT_LOCAL_ACTIVITY_SYNC_BULK_PBD;

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

INTERNAL_FUNC int emcore_get_mailbox_by_type(char *multi_user_name, int account_id, email_mailbox_type_e mailbox_type, email_mailbox_t *result_mailbox, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], result_mailbox [%p], err_code [%p]", account_id, result_mailbox, err_code);
	int ret = false, err = EMAIL_ERROR_NONE;
	emstorage_mailbox_tbl_t *mail_box_tbl_spam = NULL;

	if (result_mailbox == NULL)	{
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mailbox_by_mailbox_type(multi_user_name, account_id, mailbox_type, &mail_box_tbl_spam, false, &err)) {
		EM_DEBUG_LOG("emstorage_get_mailbox_by_mailbox_type failed - %d", err);
	}
	else {
		if (mail_box_tbl_spam) {
			result_mailbox->mailbox_type = mail_box_tbl_spam->mailbox_type;
			result_mailbox->mailbox_name = EM_SAFE_STRDUP(mail_box_tbl_spam->mailbox_name);
			result_mailbox->account_id = mail_box_tbl_spam->account_id;
			result_mailbox->mailbox_id = mail_box_tbl_spam->mailbox_id;
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
	email_event_t *event_data = NULL;

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
		event_data = em_malloc(sizeof(email_event_t));
		event_data->type = EMAIL_EVENT_LOCAL_ACTIVITY;
		event_data->account_id  = account_id;
		if (!emcore_insert_event(event_data, &handle, &err))  {
			EM_DEBUG_LOG(" emcore_insert_event failed - %d", err);
			goto FINISH_OFF;
		}

		ret = true;
	}

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

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
	int err = EMAIL_ERROR_NONE;
	int ret	= false;
	int handle = 0;
	email_event_t *event_data = NULL;

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
		event_data = em_malloc(sizeof(email_event_t));
		if (!event_data) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		event_data->type = EMAIL_EVENT_LOCAL_ACTIVITY;
		event_data->account_id  = account_id;
		if (!emcore_insert_event_for_sending_mails(event_data, &handle, &err)) {
			EM_DEBUG_LOG(" emcore_insert_event failed - %d", err);
			goto FINISH_OFF;
		}

		ret = true;
	}

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (local_activity)
		emstorage_free_local_activity(&local_activity, activity_count, NULL);

	if (activity_id_list)
		emstorage_free_activity_id_list(activity_id_list, &err);

	if (err_code != NULL)
		*err_code = err;

	return ret;
}

#endif

