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
 * File :  email-core-imap_folder.c
 * Desc :  Mail IMAP mailbox
 *
 * Auth :
 *
 * History :
 *    2006.08.01  :  created
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vconf.h>
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-storage.h"
#include "email-utilities.h"
#include "email-network.h"
#include "email-core-event.h"
#include "email-core-mailbox.h"
#include "email-core-imap-mailbox.h"
#include "email-core-imap-idle.h"
#include "email-core-mailbox-sync.h"
#include "email-core-account.h"
#include "email-core-signal.h"

#include "lnx_inc.h"
#include "c-client.h"

#include "email-debug-log.h"

INTERNAL_FUNC int emcore_get_default_mail_slot_count(char *multi_user_name, int input_account_id, int *output_count)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d] output_count[%p]", input_account_id, output_count);

	int err = EMAIL_ERROR_NONE;
	int default_mail_slot_count = 25;
	email_account_t *account_ref = NULL;

	if (output_count == NULL) {
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	account_ref = emcore_get_account_reference(multi_user_name, input_account_id, false);
	if (account_ref)
		default_mail_slot_count = account_ref->default_mail_slot_size;

FINISH_OFF:

	if (account_ref) {
		emcore_free_account(account_ref);
		EM_SAFE_FREE(account_ref);
	}

	if (output_count)
		*output_count = default_mail_slot_count;

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}


INTERNAL_FUNC int emcore_remove_overflowed_mails(char *multi_user_name,
													emstorage_mailbox_tbl_t *input_mailbox_tbl,
													int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_tbl[%p], err_code[%p]", input_mailbox_tbl, err_code);

	int ret = false;
	int *mail_id_list = NULL, mail_id_list_count = 0;
	int err = EMAIL_ERROR_NONE;
	email_account_t *account_ref = NULL;

	if (!input_mailbox_tbl || input_mailbox_tbl->account_id < 1) {
		if (input_mailbox_tbl)
		EM_DEBUG_EXCEPTION("Invalid Parameter. input_mailbox_tbl->account_id [%d]", input_mailbox_tbl->account_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	account_ref = emcore_get_account_reference(multi_user_name, input_mailbox_tbl->account_id, false);
	if (account_ref) {
		if (account_ref->incoming_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
			EM_DEBUG_LOG("ActiveSync Account didn't support mail slot");
			err = EMAIL_ERROR_NOT_SUPPORTED;
			goto FINISH_OFF;
		}
	}

	if (!emstorage_get_overflowed_mail_id_list(multi_user_name,
												input_mailbox_tbl->account_id,
												input_mailbox_tbl->mailbox_id,
												input_mailbox_tbl->mail_slot_size,
												&mail_id_list,
												&mail_id_list_count,
												false,
												&err)) {
		if (err == EMAIL_ERROR_MAIL_NOT_FOUND) {
			EM_DEBUG_LOG_SEC("There are enough slot in input_mailbox_tbl [%s]", input_mailbox_tbl->mailbox_name);
			err = EMAIL_ERROR_NONE;
			ret = true;
		} else
			EM_DEBUG_EXCEPTION("emstorage_get_overflowed_mail_id_list failed [%d]", err);

		goto FINISH_OFF;
	}

	if (mail_id_list) {
		if (!emcore_delete_mails_from_local_storage(multi_user_name,
													input_mailbox_tbl->account_id,
													mail_id_list,
													mail_id_list_count,
													EMAIL_DELETE_LOCALLY,
													EMAIL_DELETED_BY_COMMAND,
													&err)) {
			EM_DEBUG_EXCEPTION("emcore_delete_mail failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(mail_id_list);

	if (account_ref) {
		emcore_free_account(account_ref);
		EM_SAFE_FREE(account_ref);
	}

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emcore_set_mail_slot_size(char *multi_user_name, int account_id, int mailbox_id, int new_slot_size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mailbox_id[%d], err_code[%p]", account_id, mailbox_id, err_code);

	int ret = false, err = EMAIL_ERROR_NONE;
	int i = 0;
	int account_count = 100;
	int mailbox_count = 0;
	email_account_t *account_ref = NULL;
	emstorage_account_tbl_t *account_tbl_list = NULL;
	emstorage_mailbox_tbl_t *mailbox_tbl_list = NULL;

	if (account_id > ALL_ACCOUNT) {
		account_ref = emcore_get_account_reference(multi_user_name, account_id, false);
		if (account_ref && account_ref->incoming_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
			EM_DEBUG_LOG("ActiveSync account didn't support mail slot");
			ret = true;
			goto FINISH_OFF;
		} else if (!account_ref) {
			EM_DEBUG_EXCEPTION("emcore_get_account_reference failed");
			goto FINISH_OFF;
		}

		if (mailbox_id == 0) {
			if ((err = emstorage_set_field_of_accounts_with_integer_value(multi_user_name, account_id, "default_mail_slot_size", new_slot_size, true)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emstorage_set_field_of_accounts_with_integer_value failed [%d]", err);
				goto FINISH_OFF;
			}
		}
	} else {
		if (mailbox_id == 0) {
			if (!emstorage_get_account_list(multi_user_name, &account_count, &account_tbl_list, false, false, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_get_account_list failed [%d]", err);
				goto FINISH_OFF;
			}
			for (i = 0; i < account_count; i++) {
				if ((err = emstorage_set_field_of_accounts_with_integer_value(multi_user_name, account_tbl_list[i].account_id, "default_mail_slot_size", new_slot_size, true)) != EMAIL_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("emstorage_set_field_of_accounts_with_integer_value failed [%d]", err);
					goto FINISH_OFF;
				}
			}
		}
	}

	if (!emstorage_set_mail_slot_size(multi_user_name, account_id, mailbox_id, new_slot_size, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_set_mail_slot_size failed [%d]", err);
		goto FINISH_OFF;
	}

	if (mailbox_id) {
		mailbox_count = 1;
		if (new_slot_size > 0) {
			mailbox_tbl_list = em_malloc(sizeof(emstorage_mailbox_tbl_t) * mailbox_count);
			if (!mailbox_tbl_list) {
				EM_DEBUG_EXCEPTION("em_mallocfailed");
				goto FINISH_OFF;
			}
			mailbox_tbl_list->account_id = account_id;
			mailbox_tbl_list->mailbox_id = mailbox_id;
			mailbox_tbl_list->mail_slot_size = new_slot_size;
		} else {	/*  read information from DB */
			if ((err = emstorage_get_mailbox_by_id(multi_user_name, mailbox_id, &mailbox_tbl_list)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err);
				goto FINISH_OFF;
			}

		}
	} else {
		if (!emstorage_get_mailbox_list(multi_user_name, account_id, EMAIL_MAILBOX_ALL, EMAIL_MAILBOX_SORT_BY_NAME_ASC, &mailbox_count, &mailbox_tbl_list, true, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	for (i = 0; i < mailbox_count; i++) {
		if (!emcore_remove_overflowed_mails(multi_user_name, mailbox_tbl_list + i, &err)) {
			if (err == EMAIL_ERROR_MAIL_NOT_FOUND || err == EMAIL_ERROR_NOT_SUPPORTED)
				err = EMAIL_ERROR_NONE;
			else
				EM_DEBUG_EXCEPTION("emcore_remove_overflowed_mails failed [%d]", err);
		}
	}

	ret = true;

FINISH_OFF:

	if (account_ref) {
		emcore_free_account(account_ref);
		EM_SAFE_FREE(account_ref);
	}

	if (account_tbl_list)
		emstorage_free_account(&account_tbl_list, account_count, NULL);

	if (mailbox_tbl_list)
		emstorage_free_mailbox(&mailbox_tbl_list, mailbox_count, NULL);


	if (err_code)
		*err_code = err;
	return ret;
}

static int emcore_get_mailbox_connection_path(char *multi_user_name, int account_id, char *mailbox_name, char **path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("account_id [%d], mailbox_name[%s], err_code[%p]", account_id, mailbox_name, err_code);
	email_account_t *ref_account = NULL;
	size_t path_len = 0;
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	ref_account = emcore_get_account_reference(multi_user_name, account_id, false);
	if (!ref_account)	 {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed");
		goto FINISH_OFF;
	}

	path_len = EM_SAFE_STRLEN(ref_account->incoming_server_address) +
			(mailbox_name ? EM_SAFE_STRLEN(mailbox_name) : 0) + 50;

	*path = em_malloc(path_len);/* EM_SAFE_STRLEN(ref_account->incoming_server_address) + */
								/* (mailbox_name ? EM_SAFE_STRLEN(mailbox_name) : 0) + 20); */
	if (!*path) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(*path, 0x00, path_len);

	/* 1. server address / server type */
	if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_POP3) {
		SNPRINTF(*path + 1, path_len-1, "%s:%d/pop", ref_account->incoming_server_address, ref_account->incoming_server_port_number);
	} else {
		SNPRINTF(*path + 1, path_len-1, "%s:%d/imap", ref_account->incoming_server_address, ref_account->incoming_server_port_number);
	}

	/* 2. set tls option if security connection */
	/*if (ref_account->incoming_server_secure_connection) strncat(*path + 1, "/tls", path_len-(EM_SAFE_STRLEN(*path)-1));*/
	if (ref_account->incoming_server_secure_connection & 0x01) {
		strncat(*path + 1, "/ssl", path_len-(EM_SAFE_STRLEN(*path)-1));
	}
	if (ref_account->incoming_server_secure_connection & 0x02)
		strncat(*path + 1, "/tls/force_tls_v1_0", path_len-(EM_SAFE_STRLEN(*path)-1));
	else
		strncat(*path + 1, "/notls", path_len-(EM_SAFE_STRLEN(*path)-1));

	/*  3. re-format mailbox name (ex:"{mai.test.com:143/imap} or {mai.test.com:143/imap/tls}"} */
	strncat(*path + 1, "}", path_len-EM_SAFE_STRLEN(*path)-1);
	**path = '{';

	if (mailbox_name) strncat(*path, mailbox_name, path_len-EM_SAFE_STRLEN(*path)-1);

	ret = true;

FINISH_OFF:

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (err_code)
		*err_code = err;

	if (!ret)
		return 0;

	return 1;
}

static void emcore_find_mailbox_diff_between_local_and_remote(email_internal_mailbox_t *remote_box_list,
						int remote_box_count, emstorage_mailbox_tbl_t *local_box_list, int local_box_count,
						GList** remote_box_only, GList** local_box_only)
{
	if (!remote_box_only || !local_box_only) {
		return ;
	}
	int i = 0;
	GList *remote_head = NULL;
	GList *local_head  = NULL;
	GList *remote_p    = NULL;
	GList *local_p     = NULL;

	EM_DEBUG_LOG("remote_box_count[%d] local_box_count[%d]", remote_box_count, local_box_count);

	if (local_box_count == 0) {
		for (i = 0; i < remote_box_count; i++) {
			*remote_box_only = g_list_prepend(*remote_box_only, remote_box_list+i);
			*local_box_only = NULL;
		}
		return;
	}

	for (i = 0; i < remote_box_count; i++)
		remote_head = g_list_prepend(remote_head, remote_box_list+i);

	for (i = 0; i < local_box_count; i++)
		local_head = g_list_prepend(local_head, local_box_list+i);

	int matched = false;
	for (remote_p = remote_head; remote_p; remote_p = g_list_next(remote_p)) {
		matched = false ; /* initialized not matched for each iteration */
		email_internal_mailbox_t *remote_box = (email_internal_mailbox_t *)g_list_nth_data(remote_p, 0);
		EM_DEBUG_LOG_DEV("remote [%s]",  remote_box->mailbox_name);
		/* find matching mailbox in local box */
		for (local_p = local_head; local_p ; local_p = g_list_next(local_p)) {
			emstorage_mailbox_tbl_t *local_box = (emstorage_mailbox_tbl_t *)g_list_nth_data(local_p, 0);
			/* if match found */
			EM_DEBUG_LOG_DEV("vs local [%s]", local_box->mailbox_name);
			if (!EM_SAFE_STRCMP(remote_box->mailbox_name, local_box->mailbox_name)) {
				/* It is unnecessary to compare the matched box in the next iteration, so remove it from local_box*/
				local_head = g_list_delete_link(local_head, local_p);
				matched = true;
				break;
			}
		}
		/* if matching not found, add it to remote box */
		if (matched == false) {
			EM_DEBUG_LOG("New box: name[%s] alias[%s]", remote_box->mailbox_name, remote_box->alias);
			*remote_box_only = g_list_prepend(*remote_box_only, remote_box);
		}
	}

	/* local_head contains unmatched local box */
	*local_box_only = local_head;

	if (remote_head) g_list_free(remote_head);
}

INTERNAL_FUNC int emcore_sync_mailbox_list(char *multi_user_name, int account_id, char *mailbox_name, int event_handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%p], handle[%d], err_code[%p]", account_id, mailbox_name, event_handle, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	MAILSTREAM *stream = NULL;
	email_internal_mailbox_t *mailbox_list = NULL;   /* mailbox list from imap server */
	/* mailbox list from DB */
	emstorage_mailbox_tbl_t *local_mailbox_list = NULL;
	int local_mailbox_count = 0;
	GList *remote_box_only = NULL;
	GList *local_box_only  = NULL;
	email_account_t *ref_account = NULL;
	void *tmp_stream = NULL;
	char *mbox_path = NULL;
	char *mailbox_name_for_mailbox_type = NULL;
	int   i = 0, count = 0, counter = 0, mailbox_type_list[EMAIL_MAILBOX_TYPE_ALL_EMAILS + 1] = {-1, -1, -1, -1, -1, -1, -1, -1};
	int   inbox_added = 0;

	if (!emcore_notify_network_event(NOTI_SYNC_IMAP_MAILBOX_LIST_START, account_id, 0, event_handle, 0))
		EM_DEBUG_EXCEPTION("emcore_notify_network_event[ NOTI_SYNC_IMAP_MAILBOX_LIST_START] Failed >>>> ");

	FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	if (!emnetwork_check_network_status(&err)) {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, account_id, false);
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed - %d", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/* if not imap4 mail, return */
	if (ref_account->incoming_server_type != EMAIL_SERVER_TYPE_IMAP4) {
		EM_DEBUG_EXCEPTION("unsupported account...");
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/*  get mail server path */
	/*  mbox_path is not used. the below func might be unnecessary */
	if (!emcore_get_mailbox_connection_path(multi_user_name, account_id, NULL, &mbox_path, &err) || !mbox_path)  {
		EM_DEBUG_EXCEPTION("emcore_get_mailbox_connection_path - %d", err);
		goto FINISH_OFF;
	}


	FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	stream = NULL;
	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											account_id,
											0,
											true,
											(void **)&tmp_stream,
											&err) || !tmp_stream)  {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed - %d", err);

		if (err == EMAIL_ERROR_CONNECTION_BROKEN)
			err = EMAIL_ERROR_CANCELLED;
		else
			err = EMAIL_ERROR_CONNECTION_FAILURE;
		goto FINISH_OFF;
	}

	EM_SAFE_FREE(mbox_path);

	stream = (MAILSTREAM *)tmp_stream;

	FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	/*  download mailbox list */
	if (!emcore_download_mailbox_list(stream, mailbox_name, &mailbox_list, &count, &err))  {
		EM_DEBUG_EXCEPTION("emcore_download_mailbox_list failed [%d]", err);
		goto FINISH_OFF;
	}

	/* get all mailboxes which is previously synced */
	if (!emstorage_get_mailbox_list(multi_user_name, account_id, EMAIL_MAILBOX_FROM_SERVER, EMAIL_MAILBOX_SORT_BY_NAME_ASC,\
							 &local_mailbox_count, &local_mailbox_list, 1, &err)) {
		if (err != EMAIL_ERROR_MAILBOX_NOT_FOUND) {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_list error [%d]", err);
		} else
			EM_DEBUG_LOG("mailbox not found");
	}

	FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	emcore_find_mailbox_diff_between_local_and_remote(mailbox_list, count, local_mailbox_list, local_mailbox_count,
											&remote_box_only, &local_box_only);

	/* for remote_box_only, add new mailbox to DB */
	GList *p = remote_box_only;
	for (; p; p = g_list_next(p)) {
		email_internal_mailbox_t *new_mailbox = (email_internal_mailbox_t *)g_list_nth_data(p, 0);

		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		if (!new_mailbox->mailbox_name) {
			continue;
		}

		/* EM_DEBUG_LOG_SEC("mailbox name [%s]", new_mailbox->mailbox_name); */
		new_mailbox->mail_slot_size = ref_account->default_mail_slot_size;

		if (new_mailbox->mailbox_type == EMAIL_MAILBOX_TYPE_NONE)
			emcore_bind_mailbox_type(new_mailbox);

		if (new_mailbox->mailbox_type <= EMAIL_MAILBOX_TYPE_ALL_EMAILS) {	/* if result mailbox type is duplicated,  */
			if (mailbox_type_list[new_mailbox->mailbox_type] != -1) {
				EM_DEBUG_LOG_SEC("Mailbox type [%d] of [%s] is duplicated", new_mailbox->mailbox_type, new_mailbox->mailbox_name);
				new_mailbox->mailbox_type = EMAIL_MAILBOX_TYPE_USER_DEFINED; /* ignore latest one  */
			} else
				mailbox_type_list[new_mailbox->mailbox_type] = 1;
		}

		/* make box variable to be added in DB */
		emstorage_mailbox_tbl_t mailbox_tbl = {0};
		mailbox_tbl.mailbox_id     = new_mailbox->mailbox_id;
		mailbox_tbl.account_id     = new_mailbox->account_id;
		mailbox_tbl.local_yn       = 0;
		mailbox_tbl.deleted_flag   = 0;
		mailbox_tbl.mailbox_type   = new_mailbox->mailbox_type;
		mailbox_tbl.mailbox_name   = new_mailbox->mailbox_name;
		mailbox_tbl.mail_slot_size = new_mailbox->mail_slot_size;
		mailbox_tbl.no_select      = new_mailbox->no_select;

		/* Get the Alias Name after Parsing the Full mailbox Path */
		if (new_mailbox->alias == NULL)
			new_mailbox->alias = emcore_get_alias_of_mailbox((const char *)new_mailbox->mailbox_name);

		if (new_mailbox->alias) {
			mailbox_tbl.alias = new_mailbox->alias;
			mailbox_tbl.modifiable_yn = 1;
			mailbox_tbl.total_mail_count_on_server = 0;

			if (!emstorage_add_mailbox(multi_user_name, &mailbox_tbl, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_add_mailbox error [%d]", err);
				goto FINISH_OFF;
			}

			if (mailbox_tbl.mailbox_type == EMAIL_MAILBOX_TYPE_INBOX)
				inbox_added = 1;
			EM_DEBUG_LOG_SEC("MAILBOX ADDED: mailbox_name [%s] alias [%s] mailbox_type [%d]", new_mailbox->mailbox_name,\
								new_mailbox->alias, mailbox_tbl.mailbox_type);
		}
	}

	/* delete all local boxes and mails */
	p = local_box_only;
	for (; p; p = g_list_next(p)) {
		emstorage_mailbox_tbl_t *del_box = (emstorage_mailbox_tbl_t *)g_list_nth_data(p, 0);

		if (!emstorage_delete_mail_by_mailbox(multi_user_name, del_box, 1, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_delete_mail_by_mailbox error [%d] account_id [%d] mailbox_name [%s]",\
							err, del_box->account_id, del_box->mailbox_name);
		}

		if (!emstorage_delete_mailbox(multi_user_name, del_box->account_id, EMAIL_MAILBOX_FROM_SERVER, del_box->mailbox_id, 1, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_delete_mailbox error [%d] account_id [%d] mailbox_name [%s]",\
							err, del_box->account_id, del_box->mailbox_name);
		}

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
		if (!emstorage_delete_auto_download_activity_by_mailbox(multi_user_name, del_box->account_id, del_box->mailbox_id, 1, &err))
			EM_DEBUG_EXCEPTION("emstorage_delete_auto_download_activity_by_mailbox failed");
#endif

		EM_DEBUG_LOG_SEC("MAILBOX REMOVED: mailbox_name[%s] mailbox_id[%d]", del_box->mailbox_name, del_box->mailbox_id);
	}

	for (counter = EMAIL_MAILBOX_TYPE_INBOX; counter <= EMAIL_MAILBOX_TYPE_OUTBOX; counter++) {
		if (mailbox_type_list[counter] == -1) {
			int err2 = EMAIL_ERROR_NONE;
			emstorage_mailbox_tbl_t mailbox_tbl;
			emstorage_mailbox_tbl_t *result_mailbox_tbl = NULL;

			if (emstorage_get_mailbox_by_mailbox_type(multi_user_name, account_id, counter, &result_mailbox_tbl, true, &err2)) {
				if (result_mailbox_tbl) {
					emstorage_free_mailbox(&result_mailbox_tbl, 1, NULL);
					continue;
				}
			}

			memset(&mailbox_tbl, 0x00, sizeof(mailbox_tbl));

			mailbox_tbl.account_id = account_id;
			mailbox_tbl.mailbox_id = 0;
			mailbox_tbl.local_yn = 1;
			mailbox_tbl.mailbox_type = counter;
			mailbox_tbl.deleted_flag =  0;
			mailbox_tbl.modifiable_yn = 1;
			mailbox_tbl.total_mail_count_on_server = 0;
			mailbox_tbl.mail_slot_size = ref_account->default_mail_slot_size;

			switch (counter) {
				case EMAIL_MAILBOX_TYPE_SENTBOX:
					mailbox_tbl.mailbox_name = EMAIL_SENTBOX_NAME;
					mailbox_tbl.alias = EMAIL_SENTBOX_DISPLAY_NAME;
					break;

				case EMAIL_MAILBOX_TYPE_TRASH:
					mailbox_tbl.mailbox_name = EMAIL_TRASH_NAME;
					mailbox_tbl.alias = EMAIL_TRASH_DISPLAY_NAME;
					break;

				case EMAIL_MAILBOX_TYPE_DRAFT:
					mailbox_tbl.mailbox_name = EMAIL_DRAFTBOX_NAME;
					mailbox_tbl.alias = EMAIL_DRAFTBOX_DISPLAY_NAME;
					break;

				case EMAIL_MAILBOX_TYPE_SPAMBOX:
					mailbox_tbl.mailbox_name = EMAIL_SPAMBOX_NAME;
					mailbox_tbl.alias = EMAIL_SPAMBOX_DISPLAY_NAME;
					break;

				case EMAIL_MAILBOX_TYPE_OUTBOX:
					mailbox_tbl.mailbox_name = EMAIL_OUTBOX_NAME;
					mailbox_tbl.alias = EMAIL_OUTBOX_DISPLAY_NAME;
					break;

				default:
					mailbox_tbl.mailbox_name = EMAIL_INBOX_NAME;
					mailbox_tbl.alias = EMAIL_INBOX_DISPLAY_NAME;
					break;
			}

			if (!emstorage_add_mailbox(multi_user_name, &mailbox_tbl, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_add_mailbox failed - %d", err);
				goto FINISH_OFF;
			}
		}
		EM_SAFE_FREE(mailbox_name_for_mailbox_type);
	}

	if (!emstorage_set_all_mailbox_modifiable_yn(multi_user_name, account_id, 0, true, &err)) {
		EM_DEBUG_EXCEPTION(" >>>> emstorage_set_all_mailbox_modifiable_yn Failed [ %d ]", err);
		goto FINISH_OFF;
	}

	if (inbox_added)
		emcore_refresh_imap_idle_thread();

	FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	for (i = 0; i < count; i++)
		mailbox_list[i].account_id = account_id;

	ret = true;

FINISH_OFF:

	if (err == EMAIL_ERROR_NONE) {
		if (!emcore_notify_network_event(NOTI_SYNC_IMAP_MAILBOX_LIST_FINISH, account_id, 0, event_handle, err))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event[ NOTI_SYNC_IMAP_MAILBOX_LIST_FINISH] Failed >>>> ");
	} else {
		if (!emcore_notify_network_event(NOTI_SYNC_IMAP_MAILBOX_LIST_FAIL, account_id, 0, event_handle, err))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event[ NOTI_SYNC_IMAP_MAILBOX_LIST_FAIL] Failed >>>> ");
	}
	EM_SAFE_FREE(mailbox_name_for_mailbox_type);
	EM_SAFE_FREE(mbox_path);

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (stream)
		stream = mail_close(stream);

	if (mailbox_list)
		emcore_free_internal_mailbox(&mailbox_list, count, NULL);

	if (local_mailbox_list)
		emstorage_free_mailbox(&local_mailbox_list, local_mailbox_count, NULL);

	if (local_box_only)
		g_list_free(local_box_only);

	if (remote_box_only)
		g_list_free(remote_box_only);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

int emcore_download_mailbox_list(void *mail_stream,
										char *mailbox_name,
										email_internal_mailbox_t **mailbox_list,
										int *count,
										int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_stream [%p], mailbox_name [%p], mailbox_list [%p], count [%p], err_code [%p]", mail_stream, mailbox_name, mailbox_list, count, err_code);

	MAILSTREAM *stream = mail_stream;
	email_callback_holder_t holder;
	char *pattern = NULL;
	char *reference = NULL;
	int   err = EMAIL_ERROR_NONE;
	int   ret = false;

	if (!stream || !mailbox_list || !count) {
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	memset(&holder, 0x00, sizeof(holder));

    /*  reference (ex : "{mail.test.com}", "{mail.test.com}inbox") */
	if (mailbox_name)  {
		char *s = NULL;
		reference = em_malloc(EM_SAFE_STRLEN(stream->original_mailbox) + strlen(mailbox_name) + 1); /*prevent 34352*/
		if (reference) {
			strncpy(reference, stream->original_mailbox, (size_t)EM_SAFE_STRLEN(stream->original_mailbox));
			if ((s = strchr(reference, '}')))
				*(++s) = '\0';
			EM_SAFE_STRNCAT(reference, mailbox_name, (EM_SAFE_STRLEN(stream->original_mailbox) + strlen(mailbox_name) + 1) - EM_SAFE_STRLEN(reference) - 1);
		}
	} else
		reference = EM_SAFE_STRDUP(stream->original_mailbox);

	pattern        = "*";
	stream->sparep = &holder;

	/*  imap command : tag LIST reference * */
	/*  see callback function mm_list */
	mail_list(stream, reference, pattern);

	stream->sparep = NULL;

   	EM_SAFE_FREE(reference);

	*count        = holder.num;
	*mailbox_list = (email_internal_mailbox_t*) holder.data;

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;

	return ret;
}

/* description
 *    create a new imap mailbox
 * arguments
 *    new_mailbox  :  imap mailbox to be created
 * return
 *    succeed  :  1
 *    fail  :  0
 */
INTERNAL_FUNC int emcore_create_imap_mailbox(char *multi_user_name, email_mailbox_t *mailbox, int *err_code)
{
	MAILSTREAM *stream = NULL;
	char *long_enc_path = NULL;
	void *tmp_stream = NULL;
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_session_t *session = NULL;

	EM_DEBUG_FUNC_BEGIN();

	if (!mailbox) {
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emcore_get_empty_session(&session))
		EM_DEBUG_EXCEPTION("emcore_get_empty_session failed...");

	/* connect mail server */
	stream = NULL;
	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											mailbox->account_id,
											0,
											true,
											(void **)&tmp_stream,
											&err)) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}

	stream = (MAILSTREAM *) tmp_stream;

	/* encode mailbox name by UTF7, and rename to full path (ex : {mail.test.com}child_mailbox) */
	if (!emcore_get_long_encoded_path(multi_user_name, mailbox->account_id, mailbox->mailbox_name, '/', &long_enc_path, &err)) {
		EM_DEBUG_EXCEPTION("emcore_get_long_encoded_path failed [%d]", err);
		goto FINISH_OFF;
	}

	/* create mailbox */
	if (!mail_create(stream, long_enc_path)) {
		EM_DEBUG_EXCEPTION("mail_create failed");

		if (!emcore_get_current_session(&session)) {
			EM_DEBUG_EXCEPTION("emcore_get_current_session failed...");
			err = EMAIL_ERROR_SESSION_NOT_FOUND;
			goto FINISH_OFF;
		}

		if (session->error == EMAIL_ERROR_ALREADY_EXISTS)
			err = session->error;
		else
			err = EMAIL_ERROR_IMAP4_CREATE_FAILURE;

		goto FINISH_OFF;
	}

	EM_SAFE_FREE(long_enc_path);

	ret = true;

FINISH_OFF:
	if (stream) {
		stream = mail_close(stream);
	}

	EM_SAFE_FREE(long_enc_path);

	if (mailbox) {
		if (err == EMAIL_ERROR_NONE) {
			if (!emcore_notify_network_event(NOTI_ADD_MAILBOX_FINISH, mailbox->account_id, mailbox->mailbox_name, 0, 0))
				EM_DEBUG_EXCEPTION("emcore_notify_network_event[NOTI_ADD_MAILBOX_FINISH] failed");
		} else if (!emcore_notify_network_event(NOTI_ADD_MAILBOX_FAIL, mailbox->account_id, mailbox->mailbox_name, 0, err))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event[NOTI_ADD_MAILBOX_FAIL] failed");
	}

	emcore_clear_session(session);

	if (err_code)
		*err_code = err;

	return ret;
}


/* description
 *    delete a imap mailbox
 * arguments
 *    input_mailbox_id  :  mailbox ID to be deleted
 * return
 *    succeed  :  1
 *    fail  :  0
 */
INTERNAL_FUNC int emcore_delete_imap_mailbox(char *multi_user_name, int input_mailbox_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	MAILSTREAM *stream = NULL;
	char *long_enc_path = NULL;
	email_account_t *ref_account = NULL;
	void *tmp_stream = NULL;
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;

	if (!emcore_notify_network_event(NOTI_DELETE_MAILBOX_START, input_mailbox_id, 0, 0, 0))
		EM_DEBUG_EXCEPTION("emcore_notify_network_event[NOTI_DELETE_MAILBOX_START] failed");

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &mailbox_tbl)) != EMAIL_ERROR_NONE || !mailbox_tbl) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", err);
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, mailbox_tbl->account_id, false);
	if (!ref_account || ref_account->incoming_server_type != EMAIL_SERVER_TYPE_IMAP4) {
		EM_DEBUG_EXCEPTION("Invalid account information");
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/* connect mail server */
	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											mailbox_tbl->account_id,
											0,
											true,
											(void **)&tmp_stream,
											&err)) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}

	stream = (MAILSTREAM *)tmp_stream;

	/* encode mailbox name by UTF7, and rename to full path (ex : {mail.test.com}child_mailbox) */
	if (!emcore_get_long_encoded_path(multi_user_name, mailbox_tbl->account_id, mailbox_tbl->mailbox_name, '/', &long_enc_path, &err)) {
		EM_DEBUG_EXCEPTION("emcore_get_long_encoded_path failed [%d]", err);
		goto FINISH_OFF;
	}

	/* delete mailbox */
	if (!mail_delete(stream, long_enc_path)) {
		EM_DEBUG_EXCEPTION("mail_delete failed");
		err = EMAIL_ERROR_IMAP4_DELETE_FAILURE;
		goto FINISH_OFF;
	}

	EM_SAFE_FREE(long_enc_path);

	ret = true;

FINISH_OFF:
	if (stream) {
		stream = mail_close(stream);
	}

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	EM_SAFE_FREE(long_enc_path);

	if (mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

	if (err == EMAIL_ERROR_NONE) {
		if (!emcore_notify_network_event(NOTI_DELETE_MAILBOX_FINISH, input_mailbox_id, 0, 0, 0))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event[NOTI_ADD_MAILBOX_FINISH] failed");
	} else if (!emcore_notify_network_event(NOTI_DELETE_MAILBOX_FAIL, input_mailbox_id, 0, 0, err))
		EM_DEBUG_EXCEPTION("emcore_notify_network_event[NOTI_ADD_MAILBOX_FAIL] failed");

	if (err_code)
		*err_code = err;

	return ret;
}


INTERNAL_FUNC int emcore_rename_mailbox_on_imap_server(char *multi_user_name, int input_account_id, int input_mailbox_id, char *input_old_mailbox_path, char *input_new_mailbox_path, int handle_to_be_published)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d], input_mailbox_id [%d], input_old_mailbox_path [%p], input_new_mailbox_path [%p] handle_to_be_published[%d]", input_account_id, input_mailbox_id, input_old_mailbox_path, input_new_mailbox_path, handle_to_be_published);
	MAILSTREAM *stream = NULL;
	char *long_enc_path_old = NULL;
	char *long_enc_path_new = NULL;
	email_account_t *ref_account = NULL;
	void *tmp_stream = NULL;
	int err = EMAIL_ERROR_NONE;

	if (!input_old_mailbox_path || !input_new_mailbox_path) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, input_account_id, false);
	if (!ref_account || ref_account->incoming_server_type != EMAIL_SERVER_TYPE_IMAP4) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_ACCOUNT");
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/* connect mail server */
	stream = NULL;
	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											input_account_id,
											0,
											true,
											(void **)&tmp_stream,
											&err)) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed. [%d]", err);
		goto FINISH_OFF;
	}

	stream = (MAILSTREAM *)tmp_stream;

	/* encode mailbox name by UTF7, and rename to full path (ex : {mail.test.com}child_mailbox) */
	if (!emcore_get_long_encoded_path(multi_user_name, input_account_id, input_old_mailbox_path, '/', &long_enc_path_old, &err)) {
		EM_DEBUG_EXCEPTION("emcore_get_long_encoded_path failed. [%d]", err);
		goto FINISH_OFF;
	}

	/* encode mailbox name by UTF7, and rename to full path (ex : {mail.test.com}child_mailbox) */
	if (!emcore_get_long_encoded_path(multi_user_name, input_account_id, input_new_mailbox_path, '/', &long_enc_path_new, &err)) {
		EM_DEBUG_EXCEPTION("emcore_get_long_encoded_path failed. [%d]", err);
		goto FINISH_OFF;
	}

	/* rename mailbox */
	if (!mail_rename(stream, long_enc_path_old, long_enc_path_new)) {
		err = EMAIL_ERROR_IMAP4_RENAME_FAILURE;
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (err == EMAIL_ERROR_NONE) {
		if (!emcore_notify_network_event(NOTI_RENAME_MAILBOX_FINISH, input_mailbox_id, input_new_mailbox_path, handle_to_be_published, 0))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event[NOTI_RENAME_MAILBOX_FINISH] failed");
	} else {
		if (!emcore_notify_network_event(NOTI_RENAME_MAILBOX_FAIL, input_mailbox_id, input_new_mailbox_path, handle_to_be_published, 0))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event[NOTI_RENAME_MAILBOX_FAIL] failed");
	}
	EM_SAFE_FREE(long_enc_path_old);
	EM_SAFE_FREE(long_enc_path_new);

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (stream) {
		stream = mail_close(stream);
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


#ifdef __FEATURE_IMAP_QUOTA__

quota_t callback_for_get_quota_root(MAILSTREAM *stream, unsigned char *mailbox, STRINGLIST *quota_root_list)
{
	EM_DEBUG_FUNC_BEGIN();
	quota_t ret_quota;
	EM_DEBUG_FUNC_END();
	return ret_quota;
}

quota_t callback_for_get_quota(MAILSTREAM *stream, unsigned char *quota_root, QUOTALIST *quota_list)
{
	EM_DEBUG_FUNC_BEGIN();
	quota_t ret_quota;
	EM_DEBUG_FUNC_END();
	return ret_quota;
}


INTERNAL_FUNC int emcore_register_quota_callback()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

	mail_parameters(NULL, SET_QUOTAROOT, callback_for_get_quota_root); /* set callback function for handling quota root message */
	mail_parameters(NULL, SET_QUOTA,     callback_for_get_quota);      /* set callback function for handling quota message */

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_get_quota_root(int input_mailbox_id, email_quota_resource_t *output_list_of_resource_limits)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d], output_list_of_resource_limits[%p]", input_mailbox_id, output_list_of_resource_limits);
	int err = EMAIL_ERROR_NONE;
	MAILSTREAM *stream = NULL;
	email_account_t *ref_account = NULL;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &mailbox_tbl)) != EMAIL_ERROR_NONE || !mailbox_tbl) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", err);
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, mailbox_tbl->account_id, false);
	if (!ref_account || ref_account->incoming_server_type != EMAIL_SERVER_TYPE_IMAP4) {
		EM_DEBUG_EXCEPTION("Invalid account information");
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/* connect mail server */
	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											mailbox_tbl->account_id,
											0,
											true,
											(void **)&stream,
											&err)) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}

	imap_getquotaroot(stream, mailbox_tbl->mailbox_name);

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_get_quota(int input_mailbox_id, char *input_quota_root, email_quota_resource_t *output_list_of_resource_limits)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d] input_quota_root[%p] output_list_of_resource_limits[%p]", input_mailbox_id, input_quota_root, output_list_of_resource_limits);
	int err = EMAIL_ERROR_NONE;
	MAILSTREAM *stream = NULL;
	email_account_t *ref_account = NULL;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &mailbox_tbl)) != EMAIL_ERROR_NONE || !mailbox_tbl) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", err);
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(multi_user_name, mailbox_tbl->account_id, false);

	if (!ref_account || ref_account->incoming_server_type != EMAIL_SERVER_TYPE_IMAP4) {
		EM_DEBUG_EXCEPTION("Invalid account information");
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/* connect mail server */
	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											mailbox_tbl->account_id,
											0,
											true,
											(void **)&stream,
											&err)) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}

	imap_getquota(stream, input_quota_root);

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_set_quota(int input_mailbox_id, char *input_quota_root, email_quota_resource_t *input_list_of_resource_limits)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d] input_quota_root[%p] output_list_of_resource_limits[%p]", input_mailbox_id, input_quota_root, input_list_of_resource_limits);
	int err = EMAIL_ERROR_NONE;
	/* TODO : set quota using the function 'imap_setquota' */

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

#endif /* __FEATURE_IMAP_QUOTA__ */
