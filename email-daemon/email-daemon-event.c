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

#include <stdio.h>
#include <glib.h>
#include <malloc.h>
#include <pthread.h>
#include <vconf.h>
#include <signal.h>
#include <contacts.h>
#include "c-client.h"
#include "email-convert.h"
#include "email-storage.h"
#include "email-network.h"
#include "email-device.h"
#include "email-utilities.h"
#include "email-daemon.h"
#include "email-daemon-auto-poll.h"
#include "email-core-global.h"
#include "email-core-account.h"
#include "email-core-event.h"
#include "email-core-utils.h"
#include "email-core-mailbox.h"
#include "email-core-imap-mailbox.h"
#include "email-core-mail.h"
#include "email-core-mailbox-sync.h"
#include "email-core-smtp.h"
#include "email-core-utils.h"
#include "email-core-sound.h"
#include "email-core-signal.h"
#include "email-debug-log.h"

#include "email-types.h"
#include "email-internal-types.h"

extern thread_t g_srv_thread;
extern pthread_cond_t  _event_available_signal;
extern pthread_mutex_t *_event_queue_lock;
extern pthread_mutex_t *_event_handle_map_lock;
extern pthread_mutex_t *_event_callback_table_lock;
extern GQueue *g_event_que;
extern int g_event_loop;
extern int recv_thread_run;

static void *worker_event_queue(void *arg);
static int event_handler_EMAIL_EVENT_SYNC_HEADER(int input_account_id, int input_mailbox_id, int handle_to_be_published, int *error);
static int event_handler_EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT(email_account_t *account, int handle_to_be_published, int *error);
static int event_handler_EMAIL_EVENT_VALIDATE_ACCOUNT_EX(email_account_t *input_account, int input_handle_to_be_published);
static int event_handler_EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT(int account_id, email_account_t *new_account_info, int handle_to_be_published, int *error);
static int event_handler_EMAIL_EVENT_SET_MAIL_SLOT_SIZE(int account_id, int mailbox_id, int new_slot_size, int handle_to_be_published, int *error);
static int event_handler_EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED(int input_account_id, int input_mailbox_id);
static int event_handler_EMAIL_EVENT_DOWNLOAD_BODY(int account_id, int mail_id, int option, int handle_to_be_published, int *error);
static int event_handler_EMAIL_EVENT_DOWNLOAD_ATTACHMENT(int account_id, int mail_id, int attachment_no, int handle_to_be_published, int *error);
static int event_handler_EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER(int mail_ids[], int num, email_flags_field_type field_type, int value, int *error);
static int event_handler_EMAIL_EVENT_VALIDATE_ACCOUNT(int account_id, int handle_to_be_published, int *error);
static int event_handler_EMAIL_EVENT_UPDATE_MAIL(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas, int handle_to_be_published);
static int event_handler_EMAIL_EVENT_SAVE_MAIL(int input_account_id, int input_mail_id, int input_handle_to_be_published);
static int event_handler_EMAIL_EVENT_MOVE_MAIL(int account_id, int *mail_ids, int mail_id_count, int dest_mailbox_id, int src_mailbox_id, int handle_to_be_published, int *error);
static int event_handler_EMAIL_EVENT_DELETE_MAILBOX(int mailbox_id, int on_server, int recursive, int handle_to_be_published, int *error);
static int event_handler_EMAIL_EVENT_CREATE_MAILBOX(email_mailbox_t *input_new_mailbox, int on_server, int handle_to_be_published, int *error);
static int event_handler_EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER(int mail_id, int *error);
static int event_handler_EMAIL_EVENT_DELETE_MAIL_ALL(int input_account_id, int input_mailbox_id, int input_from_server, int *error);
static int event_handler_EMAIL_EVENT_DELETE_MAIL(int account_id, int *mail_id_list, int mail_id_count, int *error);
static int event_hanlder_EMAIL_EVENT_SYNC_HEADER_OMA(int account_id, char *maibox_name, int handle_to_be_published, int *error);
static int event_handler_EMAIL_EVENT_SEARCH_ON_SERVER(int account_id, int mailbox_id, char *criteria, int handle_to_be_published, int *error);
static int event_handler_EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER(int input_account_id, int input_mailbox_id, char *input_old_mailbox_path, char *input_new_mailbox_path, char *input_new_mailbox_alias, int handle_to_be_published);

static void* worker_send_event_queue(void *arg);

extern thread_t g_send_srv_thread;
extern pthread_cond_t  _send_event_available_signal;
extern pthread_mutex_t *_send_event_queue_lock;
extern pthread_mutex_t *_send_event_handle_map_lock;

extern GQueue *g_send_event_que;
extern int g_send_event_loop;
extern int send_thread_run;

extern thread_t g_partial_body_thd;
extern pthread_cond_t  _partial_body_thd_cond;
extern pthread_mutex_t _partial_body_thd_event_queue_lock;

extern email_event_partial_body_thd g_partial_body_thd_event_que[TOTAL_PARTIAL_BODY_EVENTS];
extern email_event_partial_body_thd g_partial_body_bulk_dwd_que[BULK_PARTIAL_BODY_DOWNLOAD_COUNT];
extern int g_partial_body_thd_next_event_idx;
extern int g_partial_body_thd_loop;
extern int g_partial_body_thd_queue_empty;
extern int g_partial_body_thd_queue_full;
extern int g_partial_body_bulk_dwd_queue_empty;

static gpointer partial_body_download_thread(gpointer data);

/* start api event_data loop */
INTERNAL_FUNC int emdaemon_start_event_loop(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int thread_error;

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	g_event_que = g_queue_new();
	g_queue_init(g_event_que);

	if (g_srv_thread) {
		EM_DEBUG_EXCEPTION("service thread is already running...");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_UNKNOWN;
		return true;
	}

	g_event_loop = 1;

	/* initialize lock */
	INITIALIZE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
	INITIALIZE_RECURSIVE_CRITICAL_SECTION(_event_handle_map_lock);
	INITIALIZE_RECURSIVE_CRITICAL_SECTION(_event_callback_table_lock);

	emcore_initialize_event_callback_table();

	/* create thread */
	THREAD_CREATE(g_srv_thread, worker_event_queue, NULL, thread_error);

	if (thread_error != 0) {
		EM_DEBUG_EXCEPTION("cannot create thread");
		g_event_loop = 0;
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_SYSTEM_FAILURE;
		return FAILURE;
	}

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	return false;
}

/*Send event_data loop*/
INTERNAL_FUNC int emdaemon_start_event_loop_for_sending_mails(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int thread_error = -1;

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	g_send_event_que = g_queue_new();
	g_queue_init(g_send_event_que);

	if (g_send_srv_thread) {
		EM_DEBUG_EXCEPTION("send service thread is already running...");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_UNKNOWN;
		return true;
	}

	g_send_event_loop = 1;

	/* initialize lock */
	INITIALIZE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
	INITIALIZE_RECURSIVE_CRITICAL_SECTION(_send_event_handle_map_lock);
	INITIALIZE_CONDITION_VARIABLE(_send_event_available_signal);

	/* create thread */
	THREAD_CREATE_JOINABLE(g_send_srv_thread, worker_send_event_queue, thread_error);

	if (thread_error != 0) {
		EM_DEBUG_EXCEPTION("cannot make thread...");
		g_send_event_loop = 0;
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_SYSTEM_FAILURE;
		return FAILURE;
	}

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	EM_DEBUG_FUNC_END();
	return SUCCESS;
}

static void* worker_event_queue(void *arg)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;
	int is_storage_full = false;
	int noti_id = 0;
	email_event_t *event_data = NULL;
	email_event_t *started_event = NULL;
	emstorage_account_tbl_t *account_tbl = NULL;
	int handle_to_be_published = 0;

	if (!emstorage_open(&err)) {
		EM_DEBUG_EXCEPTION("emstorage_open falied [%d]", err);
		return false;
	}

	/* check that event_data loop is continuous */
	while (emcore_event_loop_continue()) {

		/* get a event_data from event_data queue */
		ENTER_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
		if (!emcore_retrieve_event(&event_data, &err)) {
			/* no event_data pending */
			if (err != EMAIL_ERROR_EVENT_QUEUE_EMPTY) {
				LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
				continue;
			}

			recv_thread_run = 0;
			emdevice_set_sleep_on_off(true, NULL);

			//go to sleep when queue is empty
			SLEEP_CONDITION_VARIABLE(_event_available_signal, *_event_queue_lock);
			EM_DEBUG_LOG("Wake up by _event_available_signal");
			LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
		}
		else {
			LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
			EM_DEBUG_LOG_DEV(">>>>>>>>>>>>>>> Got event_data !!! <<<<<<<<<<<<<<<");

			emdevice_set_sleep_on_off(false, NULL);

			handle_to_be_published = event_data->handle;
			EM_DEBUG_LOG("Handle to be Published  [%d]", handle_to_be_published);
			recv_thread_run = 1;

			/* Handling storage full */
			is_storage_full = false;
			if (event_data->type == EMAIL_EVENT_SYNC_HEADER ||
				event_data->type == EMAIL_EVENT_SYNC_HEADER_OMA ||
				event_data->type == EMAIL_EVENT_DOWNLOAD_BODY ||
				event_data->type == EMAIL_EVENT_DOWNLOAD_ATTACHMENT) {

				if (emcore_is_storage_full(&err)) {
					EM_DEBUG_EXCEPTION("Storage is full");
					switch (event_data->type) {
						case EMAIL_EVENT_SYNC_HEADER:
						case EMAIL_EVENT_SYNC_HEADER_OMA:
							noti_id = NOTI_DOWNLOAD_FAIL;
							break;
						case EMAIL_EVENT_DOWNLOAD_BODY:
							noti_id = NOTI_DOWNLOAD_BODY_FAIL;
							break;
						case EMAIL_EVENT_DOWNLOAD_ATTACHMENT:
							noti_id = NOTI_DOWNLOAD_ATTACH_FAIL;
							break;
						default:
							break;
					}

					if (!emcore_notify_network_event(noti_id, event_data->account_id, NULL,  handle_to_be_published, err))
						EM_DEBUG_EXCEPTION(" emcore_notify_network_event [NOTI_DOWNLOAD_FAIL] Failed >>>> ");
					is_storage_full = true;
				}
			}



			if (event_data->account_id > 0) {
				if (!emstorage_get_account_by_id(event_data->account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account_tbl, false, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_get_account_by_id [%d]", err);
				}
			}

			if (account_tbl)
				EM_DEBUG_LOG("account_id : [%d], sync_disabled : [%d]", event_data->account_id, account_tbl->sync_disabled);

			if (!account_tbl || account_tbl->sync_disabled == 0) {
				switch (event_data->type) {
					case EMAIL_EVENT_SYNC_IMAP_MAILBOX:  /* get imap mailbox list  */
						if (!emcore_sync_mailbox_list(event_data->account_id, event_data->event_param_data_3, handle_to_be_published, &err))
							EM_DEBUG_EXCEPTION("emcore_sync_mailbox_list failed [%d]", err);
						break;

					case EMAIL_EVENT_SYNC_HEADER:  /* synchronize mail header  */
						if (is_storage_full == false)
							event_handler_EMAIL_EVENT_SYNC_HEADER(event_data->account_id, event_data->event_param_data_5, handle_to_be_published,  &err);
						break;

					case EMAIL_EVENT_SYNC_HEADER_OMA:  /*  synchronize mail header for OMA */
						if (is_storage_full == false)
							event_hanlder_EMAIL_EVENT_SYNC_HEADER_OMA(event_data->account_id, event_data->event_param_data_1, handle_to_be_published, &err);
						break;

					case EMAIL_EVENT_DOWNLOAD_BODY:  /*  download mail body  */
						if (is_storage_full == false)
							event_handler_EMAIL_EVENT_DOWNLOAD_BODY(event_data->account_id, event_data->event_param_data_4, event_data->event_param_data_5, handle_to_be_published, &err);
						break;

					case EMAIL_EVENT_DOWNLOAD_ATTACHMENT:  /*  download attachment */
						if (is_storage_full == false)
							event_handler_EMAIL_EVENT_DOWNLOAD_ATTACHMENT(event_data->account_id, (int)event_data->event_param_data_4, event_data->event_param_data_5, handle_to_be_published, &err);
						break;

					case EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER:  /*  Sync flags field */
						event_handler_EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER((int*)event_data->event_param_data_3, event_data->event_param_data_4 , event_data->event_param_data_5, event_data->event_param_data_6, &err);
						break;

					case EMAIL_EVENT_DELETE_MAIL:  /*  delete mails */
						event_handler_EMAIL_EVENT_DELETE_MAIL(event_data->account_id, (int*)event_data->event_param_data_3, event_data->event_param_data_4, &err);
						break;

					case EMAIL_EVENT_DELETE_MAIL_ALL:  /*  delete all mails */
						event_handler_EMAIL_EVENT_DELETE_MAIL_ALL(event_data->account_id, event_data->event_param_data_4, (int)event_data->event_param_data_5, &err);
						break;
#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
					case EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER:
						event_handler_EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER((int)event_data->event_param_data_4, &err);
						break;
#endif

					case EMAIL_EVENT_CREATE_MAILBOX: {
							email_mailbox_t *new_mailbox = (email_mailbox_t*)event_data->event_param_data_1;

							if(new_mailbox) {
								event_handler_EMAIL_EVENT_CREATE_MAILBOX(new_mailbox, event_data->event_param_data_4, handle_to_be_published, &err);
							}
						}
						break;

					case EMAIL_EVENT_DELETE_MAILBOX:
						event_handler_EMAIL_EVENT_DELETE_MAILBOX(event_data->event_param_data_4, event_data->event_param_data_5, event_data->event_param_data_6,handle_to_be_published, &err);
						break;

					case EMAIL_EVENT_SAVE_MAIL:
						err = event_handler_EMAIL_EVENT_SAVE_MAIL(event_data->account_id, event_data->event_param_data_4, handle_to_be_published);
						break;

					case EMAIL_EVENT_MOVE_MAIL:
						event_handler_EMAIL_EVENT_MOVE_MAIL(event_data->account_id, (int  *)event_data->event_param_data_3, event_data->event_param_data_4, event_data->event_param_data_5, event_data->event_param_data_8, handle_to_be_published, &err);
						break;

					case EMAIL_EVENT_VALIDATE_ACCOUNT:
						event_handler_EMAIL_EVENT_VALIDATE_ACCOUNT(event_data->account_id, handle_to_be_published, &err);
						break;

					case EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT: {
							email_account_t *account = (email_account_t *)event_data->event_param_data_1;
							event_handler_EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT(account, handle_to_be_published, &err);
						}
						break;

					case EMAIL_EVENT_VALIDATE_ACCOUNT_EX: {
							email_account_t *account = (email_account_t *)event_data->event_param_data_1;
							err = event_handler_EMAIL_EVENT_VALIDATE_ACCOUNT_EX(account, handle_to_be_published);
						}
						break;

					case EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT: {
							email_account_t *account = (email_account_t *)event_data->event_param_data_1;
							event_handler_EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT(event_data->account_id, account, handle_to_be_published, &err);
						}
						break;

					case EMAIL_EVENT_UPDATE_MAIL:
						event_handler_EMAIL_EVENT_UPDATE_MAIL((email_mail_data_t*)event_data->event_param_data_1, (email_attachment_data_t*)event_data->event_param_data_2, event_data->event_param_data_4, (email_meeting_request_t*)event_data->event_param_data_3, event_data->event_param_data_5, handle_to_be_published);
						break;

					case EMAIL_EVENT_SET_MAIL_SLOT_SIZE:
						event_handler_EMAIL_EVENT_SET_MAIL_SLOT_SIZE(event_data->account_id, event_data->event_param_data_4, event_data->event_param_data_5, handle_to_be_published, &err);
						break;

					case EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED:
						err = event_handler_EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED(event_data->account_id, event_data->event_param_data_4);
						break;

#ifdef __FEATURE_LOCAL_ACTIVITY__
					case EMAIL_EVENT_LOCAL_ACTIVITY:
						event_handler_EMAIL_EVENT_LOCAL_ACTIVITY(event_data->account_id, &err);
						break;
#endif /* __FEATURE_LOCAL_ACTIVITY__*/

					case EMAIL_EVENT_SEARCH_ON_SERVER:
						event_handler_EMAIL_EVENT_SEARCH_ON_SERVER(event_data->account_id, event_data->event_param_data_4, (char *)event_data->event_param_data_1, handle_to_be_published, &err);
						break;

					case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
						err = event_handler_EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER(event_data->account_id, event_data->event_param_data_4, (char*)event_data->event_param_data_1, (char*)event_data->event_param_data_2, (char*)event_data->event_param_data_3, handle_to_be_published);
						break;

					case EMAIL_EVENT_QUERY_SMTP_MAIL_SIZE_LIMIT:
						if (!emcore_query_mail_size_limit(event_data->account_id, handle_to_be_published, &err))
							EM_DEBUG_EXCEPTION("emcore_sync_mailbox_list failed [%d]", err);
						break;

					default:
						break;
				}
			}

			/* free internals in event */
			emcore_free_event(event_data); /*detected by valgrind*/

			if (account_tbl) {
				emstorage_free_account(&account_tbl, 1, NULL);
				account_tbl = NULL;
			}

			if (!emcore_notify_response_to_api(event_data->type, handle_to_be_published, err))
				EM_DEBUG_EXCEPTION("emcore_notify_response_to_api failed");

			/* free event itself */
			ENTER_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
			started_event = g_queue_pop_head(g_event_que);
			LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
			if (!started_event) {
				EM_DEBUG_EXCEPTION("Failed to g_queue_pop_head");
			}
			else {
				emcore_return_handle(started_event->handle);
				EM_SAFE_FREE(started_event);
			}
			em_flush_memory();
		}
	}

	if (!emstorage_close(&err))
		EM_DEBUG_EXCEPTION("emstorage_close falied [%d]", err);

	EM_DEBUG_FUNC_END();
	return SUCCESS;
}

static int event_handler_EMAIL_EVENT_SYNC_HEADER (int input_account_id, int input_mailbox_id, 
                                                                       int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d], input_mailbox_id [%d], handle_to_be_published [%d], error[%p]", 
                                                   input_account_id, input_mailbox_id, handle_to_be_published, error);

	int err = EMAIL_ERROR_NONE, sync_type = 0, ret = false;
	int mailbox_count = 0, account_count = 0;
	int counter, account_index;
	int unread = 0, total_unread = 0;
	int mail_count = 0, total_mail = 0;
	emcore_uid_list *uid_list = NULL;
	emstorage_account_tbl_t *account_tbl_array = NULL;
	emstorage_mailbox_tbl_t *mailbox_tbl_target = NULL, *mailbox_tbl_list = NULL;
#ifndef __FEATURE_KEEP_CONNECTION__
	MAILSTREAM *stream = NULL;
#endif
	char mailbox_id_param_string[10] = {0,};
	char *input_mailbox_id_str = NULL;

	if (input_mailbox_id == 0)
		sync_type = EMAIL_SYNC_ALL_MAILBOX;
	else {
		if (!emstorage_get_mailbox_by_id(input_mailbox_id, &mailbox_tbl_target) || !mailbox_tbl_target) {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err);
			goto FINISH_OFF;
		}
	}
	if(mailbox_tbl_target)
		SNPRINTF(mailbox_id_param_string, 10, "%d", mailbox_tbl_target->mailbox_id);

	input_mailbox_id_str = (input_mailbox_id == 0)? NULL: mailbox_id_param_string;

/*	if (!emcore_notify_network_event(NOTI_DOWNLOAD_START, input_account_id, input_mailbox_id_str, handle_to_be_published, 0))
		EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_DOWNLOAD_START] Failed >>>> ");
*/
	if (!emnetwork_check_network_status(&err)) {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status error [%d]", err);
		if (!emcore_notify_network_event(NOTI_DOWNLOAD_FAIL, input_account_id, input_mailbox_id_str, handle_to_be_published, err))
			EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_FAIL] Failed >>>> ");
		goto FINISH_OFF;
	}

	if (sync_type != EMAIL_SYNC_ALL_MAILBOX) {	/* Sync only particular mailbox */
		EM_DEBUG_LOG_SEC ("sync start: account_id [%d] alias [%s]", input_account_id, mailbox_tbl_target->alias);
		if ((err = emcore_update_sync_status_of_account(input_account_id, SET_TYPE_SET, SYNC_STATUS_SYNCING)) != EMAIL_ERROR_NONE)
			EM_DEBUG_EXCEPTION("emcore_update_sync_status_of_account failed [%d]", err);

		if (!emcore_sync_header (mailbox_tbl_target, (void**) &stream, &uid_list, &mail_count, &unread, &err)) {
			EM_DEBUG_EXCEPTION("emcore_sync_header failed [%d]", err);
			if (!emcore_notify_network_event(NOTI_DOWNLOAD_FAIL, mailbox_tbl_target->account_id, mailbox_id_param_string, handle_to_be_published, err))
				EM_DEBUG_EXCEPTION(" emcore_notify_network_event [NOTI_DOWNLOAD_FAIL] Failed >>>> ");
		}
		else {
			EM_DEBUG_LOG("emcore_sync_header succeeded [%d]", err);
			if (!emcore_notify_network_event(NOTI_DOWNLOAD_FINISH, mailbox_tbl_target->account_id, mailbox_id_param_string, handle_to_be_published, 0))
				EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_DOWNLOAD_FINISH] Failed >>>> ");
		}
		stream = mail_close (stream);
		total_unread += unread;
		total_mail += mail_count;

		if (total_unread > 0 && (err = emcore_update_sync_status_of_account(input_account_id, SET_TYPE_UNION, SYNC_STATUS_HAVE_NEW_MAILS)) != EMAIL_ERROR_NONE)
			EM_DEBUG_EXCEPTION("emcore_update_sync_status_of_account failed [%d]", err);

		if (!emdaemon_finalize_sync(input_account_id, total_mail, total_unread, &err))
			EM_DEBUG_EXCEPTION("emdaemon_finalize_sync failed [%d]", err);
	}
	else /*  All Foder */ {
		EM_DEBUG_LOG ("sync start for all mailbox: account_id [%d]", input_account_id);
		/*  Sync of all mailbox */

		if (input_account_id == ALL_ACCOUNT) {
			if ((err = emcore_update_sync_status_of_account(ALL_ACCOUNT, SET_TYPE_SET, SYNC_STATUS_SYNCING)) != EMAIL_ERROR_NONE)
				EM_DEBUG_EXCEPTION("emcore_update_sync_status_of_account failed [%d]", err);

			if (!emstorage_get_account_list(&account_count, &account_tbl_array , true, false, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_get_account_list failed [ %d ] ", err);
				if (!emcore_notify_network_event(NOTI_DOWNLOAD_FAIL, input_account_id, NULL,  handle_to_be_published, err))
					EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_FAIL] Failed >>>> ");
				goto FINISH_OFF;
			}
		}
		else {
			if ((err = emcore_update_sync_status_of_account(input_account_id, SET_TYPE_SET, SYNC_STATUS_SYNCING)) != EMAIL_ERROR_NONE)
				EM_DEBUG_EXCEPTION("emcore_update_sync_status_of_account failed [%d]", err);

			if (!emstorage_get_account_by_id(input_account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account_tbl_array, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed [ %d ] ", err);
				if (!emcore_notify_network_event(NOTI_DOWNLOAD_FAIL, input_account_id, input_mailbox_id_str, handle_to_be_published, err))
					EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_FAIL] Failed >>>> ");
				goto FINISH_OFF;
			}
			account_count = 1;
		}

		for (account_index = 0 ; account_index < account_count; account_index++) {
			if (account_tbl_array[account_index].incoming_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
				EM_DEBUG_LOG ("account[%d] is for ActiveSync. Skip  ", account_index);
				continue;
			}

			if (!emstorage_get_mailbox_list (account_tbl_array[account_index].account_id, 0, 
                                  EMAIL_MAILBOX_SORT_BY_TYPE_ASC, &mailbox_count, &mailbox_tbl_list, true, &err) || 
                                                                                                  mailbox_count <= 0) {
				EM_DEBUG_EXCEPTION ("emstorage_get_mailbox error [%d]", err);
				if (!emcore_notify_network_event (NOTI_DOWNLOAD_FAIL, account_tbl_array[account_index].account_id, 
                                                                     input_mailbox_id_str, handle_to_be_published, err))
					EM_DEBUG_EXCEPTION ("emcore_notify_network_event [NOTI_DOWNLOAD_FAIL] error >>>> ");
				continue;
			}

			EM_DEBUG_LOG("emcore_get_mailbox_list_to_be_sync returns [%d] mailboxes", mailbox_count);


#ifndef __FEATURE_KEEP_CONNECTION__
			if (account_tbl_array[account_index].incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
				memset(mailbox_id_param_string, 0, 10);
				SNPRINTF(mailbox_id_param_string, 10, "%d", mailbox_tbl_list[0].mailbox_id);
				if (!emcore_connect_to_remote_mailbox (account_tbl_array[account_index].account_id, 
                                                      mailbox_tbl_list[0].mailbox_id, (void **)&stream, &err))  {
					EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox error [%d]", err);
					if (err == EMAIL_ERROR_LOGIN_FAILURE)
						EM_DEBUG_EXCEPTION("EMAIL_ERROR_LOGIN_FAILURE ");
					/* continue; */
					if (!emcore_notify_network_event(NOTI_DOWNLOAD_FAIL, account_tbl_array[account_index].account_id, mailbox_id_param_string,  handle_to_be_published, err))
						EM_DEBUG_EXCEPTION(" emcore_notify_network_event [NOTI_DOWNLOAD_FAIL] Failed >>>> ");
					continue;
				}
				EM_DEBUG_LOG ("emcore_connect_to_remote_mailbox returns [%d]", err);
			}
			else
				stream = mail_close (stream);
#endif

			for (counter = 0; counter < mailbox_count; counter++) {

				EM_DEBUG_LOG_SEC("mailbox_name [%s], mailbox_id [%d], mailbox_type [%d]", mailbox_tbl_list[counter].mailbox_name, mailbox_tbl_list[counter].mailbox_id, mailbox_tbl_list[counter].mailbox_type);
				unread = mail_count = 0;
				if ( mailbox_tbl_list[counter].mailbox_type == EMAIL_MAILBOX_TYPE_ALL_EMAILS
					|| mailbox_tbl_list[counter].mailbox_type == EMAIL_MAILBOX_TYPE_TRASH
					/*|| mailbox_tbl_list[counter].mailbox_type == EMAIL_MAILBOX_TYPE_SPAMBOX */) {
					EM_DEBUG_LOG("Skipped for all emails or trash");
					continue;
				}
				else if (!mailbox_tbl_list[counter].local_yn) {
					EM_DEBUG_LOG_SEC("[%s] Syncing...", mailbox_tbl_list[counter].mailbox_name);
#ifdef __FEATURE_KEEP_CONNECTION__
					if (!emcore_sync_header ((mailbox_tbl_list + counter), (void **)&stream, &uid_list, 
                                                                                           &mail_count, &unread, &err))
#else /*  __FEATURE_KEEP_CONNECTION__ */
					if (!emcore_sync_header ((mailbox_tbl_list + counter), (void **)&stream, &uid_list, 
                                                                                           &mail_count, &unread, &err))
#endif /*  __FEATURE_KEEP_CONNECTION__ */
					{	
						EM_DEBUG_EXCEPTION_SEC ("emcore_sync_header for %s(mailbox_id = %d) failed [%d]", 
							mailbox_tbl_list[counter].mailbox_name, mailbox_tbl_list[counter].mailbox_id, err);

#ifndef __FEATURE_KEEP_CONNECTION__
						if (err == EMAIL_ERROR_CONNECTION_BROKEN || err == EMAIL_ERROR_NO_SUCH_HOST || 
                                                                                 err == EMAIL_ERROR_SOCKET_FAILURE)
							stream = mail_close (stream);    
#endif /*  __FEATURE_KEEP_CONNECTION__ */
						memset(mailbox_id_param_string, 0, 10);
						SNPRINTF(mailbox_id_param_string, 10, "%d", mailbox_tbl_list[counter].mailbox_id);
						if (!emcore_notify_network_event (NOTI_DOWNLOAD_FAIL, 
                                                                           account_tbl_array[account_index].account_id, 
                                                                           mailbox_id_param_string,
                                                                           handle_to_be_published, err))
							EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_FAIL] Failed >>>> ");

						break;
					}
				}
				EM_DEBUG_LOG_SEC("---mailbox %s has unread %d / %d", mailbox_tbl_list[counter].mailbox_name, 
                                                                                                unread, mail_count);
				total_unread  += unread;
				total_mail += mail_count;
			}

			EM_DEBUG_LOG_SEC("Sync for account_id(%d) is completed....! (unread %d/%d)", account_tbl_array[account_index].account_id, total_unread, total_mail);
			if ((err == EMAIL_ERROR_NONE) && !emcore_notify_network_event(NOTI_DOWNLOAD_FINISH, account_tbl_array[account_index].account_id, NULL, handle_to_be_published, 0))
				EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_FINISH] Failed >>>> ");

			if ((total_unread > 0) && (err = emcore_update_sync_status_of_account(account_tbl_array[account_index].account_id, SET_TYPE_UNION, SYNC_STATUS_HAVE_NEW_MAILS)) != EMAIL_ERROR_NONE)
				EM_DEBUG_EXCEPTION("emcore_update_sync_status_of_account failed [%d]", err);

			if (!emdaemon_finalize_sync(account_tbl_array[account_index].account_id, total_mail, total_unread, &err))
				EM_DEBUG_EXCEPTION("emdaemon_finalize_sync failed [%d]", err);
#ifndef __FEATURE_KEEP_CONNECTION__
			if (stream)  
				stream = mail_close (stream);
#endif
			if (mailbox_tbl_list) {
				emstorage_free_mailbox(&mailbox_tbl_list, mailbox_count, NULL);
				mailbox_tbl_list = NULL;
				mailbox_count = 0;
			}
		}
	}

	ret = true;

FINISH_OFF:

#ifndef __FEATURE_KEEP_CONNECTION__
	if (stream)
		stream = mail_close (stream);
#endif
	if(mailbox_tbl_target)
		emstorage_free_mailbox(&mailbox_tbl_target, 1, NULL);

	if (mailbox_tbl_list)
		emstorage_free_mailbox(&mailbox_tbl_list, mailbox_count, NULL);

	if (account_tbl_array)
		emstorage_free_account(&account_tbl_array, account_count, NULL);

	EM_DEBUG_FUNC_END();
	return ret;
}

static int event_handler_EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT(email_account_t *account, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN("account [%p]", account);
	int err, ret = false;

	if(!account) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_SEC("incoming_server_address  :  %s", account->incoming_server_address);

	if (!emnetwork_check_network_status(&err)) {
		emcore_delete_account_from_unvalidated_account_list(account->account_id);
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
		if (!emcore_notify_network_event(NOTI_VALIDATE_AND_CREATE_ACCOUNT_FAIL, account->account_id, NULL,  handle_to_be_published, err))
			EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_FAIL] Failed >>>> ");
		goto FINISH_OFF;
	}
	else {
		EM_DEBUG_LOG_SEC("incoming_server_address : %s", account->incoming_server_address);

		if (!emcore_validate_account_with_account_info(account, EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT, &err)) {
			emcore_delete_account_from_unvalidated_account_list(account->account_id);
			EM_DEBUG_EXCEPTION("emcore_validate_account_with_account_info failed err :  %d", err);
			if (err == EMAIL_ERROR_CANCELLED) {
				EM_DEBUG_EXCEPTION(" notify  :  NOTI_VALIDATE_AND_CREATE_ACCOUNT_CANCEL ");
				if (!emcore_notify_network_event(NOTI_VALIDATE_AND_CREATE_ACCOUNT_CANCEL, account->account_id, NULL,  handle_to_be_published, err))
					EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_VALIDATE_AND_CREATE_ACCOUNT_CANCEL] Failed");
				goto FINISH_OFF;
			}
			else
				goto FINISH_OFF;
		}
		else {
			emcore_delete_account_from_unvalidated_account_list(account->account_id);

			if (emcore_create_account(account, &err) == false)	 {
				EM_DEBUG_EXCEPTION(" emdaemon_create_account failed - %d", err);
				goto FINISH_OFF;
			}

			emcore_init_account_reference();

			EM_DEBUG_LOG("incoming_server_type [%d]", account->incoming_server_type);

			if ((EMAIL_SERVER_TYPE_IMAP4 == account->incoming_server_type)) {
				if (!emcore_sync_mailbox_list(account->account_id, "", handle_to_be_published, &err))  {
					EM_DEBUG_EXCEPTION("emcore_get_mailbox_list_to_be_sync failed [%d]", err);
					/*  delete account whose mailbox couldn't be obtained from server */
					emcore_delete_account(account->account_id, NULL);
					goto FINISH_OFF;
				}
			}

			EM_DEBUG_LOG("validating and creating an account are succeeded for account id  [%d]  err [%d]", account->account_id, err);
			if (!emcore_notify_network_event(NOTI_VALIDATE_AND_CREATE_ACCOUNT_FINISH, account->account_id, NULL,  handle_to_be_published, err))
				EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_FINISH] Success");
		}
	}

	ret = true;

FINISH_OFF:
	if (ret == false && err != EMAIL_ERROR_CANCELLED && account) {
		if (!emcore_notify_network_event(NOTI_VALIDATE_AND_CREATE_ACCOUNT_FAIL, account->account_id, NULL,  handle_to_be_published, err))
			EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_VALIDATE_AND_CREATE_ACCOUNT_FAIL] Failed");
	}

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

static int event_handler_EMAIL_EVENT_VALIDATE_ACCOUNT_EX(email_account_t *input_account, int input_handle_to_be_published)
{
	EM_DEBUG_FUNC_BEGIN("input_account [%p]", input_account);
	int err = EMAIL_ERROR_NONE;

	if(!input_account) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_SEC("incoming_server_address [%s]", input_account->incoming_server_address);

	if (!emnetwork_check_network_status(&err)) {
		emcore_delete_account_from_unvalidated_account_list(input_account->account_id);
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
		if (!emcore_notify_network_event(NOTI_VALIDATE_AND_CREATE_ACCOUNT_FAIL, input_account->account_id, NULL, input_handle_to_be_published, err))
			EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_FAIL] Failed >>>> ");
		goto FINISH_OFF;
	}
	else {
		EM_DEBUG_LOG_SEC("incoming_server_address : %s", input_account->incoming_server_address);

		if (!emcore_validate_account_with_account_info(input_account, EMAIL_EVENT_VALIDATE_ACCOUNT_EX, &err)) {
			emcore_delete_account_from_unvalidated_account_list(input_account->account_id);

			EM_DEBUG_EXCEPTION("emcore_validate_account_with_account_info failed err :  %d", err);

			if (err == EMAIL_ERROR_CANCELLED) {
				EM_DEBUG_EXCEPTION(" notify  :  NOTI_VALIDATE_ACCOUNT_CANCEL ");
				if (!emcore_notify_network_event(NOTI_VALIDATE_ACCOUNT_CANCEL, input_account->account_id, NULL, input_handle_to_be_published, err))
					EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_CANCEL] Failed");
				goto FINISH_OFF;
			}
			else
				goto FINISH_OFF;
		}
		else {
			emcore_delete_account_from_unvalidated_account_list(input_account->account_id);

			EM_DEBUG_LOG("validating an account are succeeded for account id  [%d]  err [%d]", input_account->account_id, err);
			if (!emcore_notify_network_event(NOTI_VALIDATE_ACCOUNT_FINISH, input_account->account_id, NULL, input_handle_to_be_published, err))
				EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_FINISH] Success");
		}
	}

FINISH_OFF:
	if (err != EMAIL_ERROR_NONE && err != EMAIL_ERROR_CANCELLED && input_account) {
		if (!emcore_notify_network_event(NOTI_VALIDATE_ACCOUNT_FAIL, input_account->account_id, NULL, input_handle_to_be_published, err))
			EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_FAIL] Failed");
	}

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

static int event_handler_EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT(int account_id, email_account_t *new_account_info, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], new_account_info [%p]", account_id, new_account_info);
	int err, ret = false;
	emstorage_account_tbl_t *old_account_tbl = NULL, *new_account_tbl = NULL;

	if (!new_account_info) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emnetwork_check_network_status(&err))  {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);

		if (!emcore_notify_network_event(NOTI_VALIDATE_AND_UPDATE_ACCOUNT_FAIL, new_account_info->account_id, NULL,  handle_to_be_published, err))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_VALIDATE_AND_UPDATE_ACCOUNT_FAIL] Failed >>>> ");
		goto FINISH_OFF;
	}
	else  {
		EM_DEBUG_LOG_SEC("incoming_server_address: (%s)", new_account_info->incoming_server_address);

		if (!emcore_validate_account_with_account_info(new_account_info, EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT, &err)) {
			EM_DEBUG_EXCEPTION("emcore_validate_account_with_account_info() failed err :  %d", err);
			if (err == EMAIL_ERROR_CANCELLED) {
				EM_DEBUG_EXCEPTION(" notify  :  NOTI_VALIDATE_AND_CREATE_ACCOUNT_CANCEL ");
				if (!emcore_notify_network_event(NOTI_VALIDATE_AND_UPDATE_ACCOUNT_CANCEL, new_account_info->account_id, NULL,  handle_to_be_published, err))
					EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_VALIDATE_AND_UPDATE_ACCOUNT_CANCEL] Failed");
				goto FINISH_OFF;
			}
			else {
				goto FINISH_OFF;
			}
		}
		else {
			if (!emstorage_get_account_by_id(account_id, WITHOUT_OPTION, &old_account_tbl, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed [%d]", err);
				/* goto FINISH_OFF; */
			}

			new_account_tbl = em_malloc(sizeof(emstorage_account_tbl_t));
			if (!new_account_tbl) {
				EM_DEBUG_EXCEPTION("allocation failed [%d]", err);
				goto FINISH_OFF;
			}

			em_convert_account_to_account_tbl(new_account_info, new_account_tbl);

			if (emstorage_update_account(account_id, new_account_tbl, true, &err)) {
				emcore_init_account_reference();
			}

			EM_DEBUG_LOG("validating and updating an account are succeeded for account id [%d], err [%d]", new_account_info->account_id, err);
			if (!emcore_notify_network_event(NOTI_VALIDATE_AND_UPDATE_ACCOUNT_FINISH, new_account_info->account_id, NULL,  handle_to_be_published, err))
				EM_DEBUG_EXCEPTION(" emcore_notify_network_event [NOTI_VALIDATE_AND_UPDATE_ACCOUNT_FINISH] Success");
		}
	}

	ret = true;

FINISH_OFF:
	if (old_account_tbl)
		emstorage_free_account(&old_account_tbl, 1, NULL);
	if (new_account_tbl)
		emstorage_free_account(&new_account_tbl, 1, NULL);

	if (ret == false && err != EMAIL_ERROR_CANCELLED && new_account_info) {
		if (!emcore_notify_network_event(NOTI_VALIDATE_AND_UPDATE_ACCOUNT_FAIL, new_account_info->account_id, NULL,  handle_to_be_published, err))
			EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_VALIDATE_AND_CREATE_ACCOUNT_FAIL] Failed");
	}

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

static int event_handler_EMAIL_EVENT_SET_MAIL_SLOT_SIZE(int account_id, int mailbox_id, int new_slot_size, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN();

	emcore_set_mail_slot_size(account_id, mailbox_id, new_slot_size, error);

	EM_DEBUG_FUNC_END();
	return true;
}

static int event_handler_EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED(int input_account_id, int input_mailbox_id)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d], input_mailbox_id [%d]", input_account_id, input_mailbox_id);
	int err = EMAIL_ERROR_NONE;

	if ( (err = emcore_expunge_mails_deleted_flagged_from_remote_server(input_account_id, input_mailbox_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_expunge_mails_deleted_flagged_from_remote_server failed [%d]", err);
		goto FINISH_OFF;
	}

	if ( (err = emcore_expunge_mails_deleted_flagged_from_local_storage(input_mailbox_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_expunge_mails_deleted_flagged_from_local_storage failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

#ifdef __FEATURE_LOCAL_ACTIVITY__
static int event_handler_EMAIL_EVENT_LOCAL_ACTIVITY(int account_id, int *error)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;
	email_mailbox_t mailbox;
	emstorage_activity_tbl_t *local_activity = NULL;
	int activity_id_count = 0;
	int activity_chunk_count = 0;
	int *activity_id_list = NULL;
	int i = 0;

	if (!emnetwork_check_network_status(&err))
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
	else {
		if (false == emstorage_get_activity_id_list(account_id, &activity_id_list, &activity_id_count, ACTIVITY_DELETEMAIL, ACTIVITY_COPYMAIL, true, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_activity_id_list failed [%d]", err);
		}
		else {
			for (i = 0; i < activity_id_count; ++i) {
				if ((false == emstorage_get_activity(account_id , activity_id_list[i], &local_activity, &activity_chunk_count, true,  &err)) || (NULL == local_activity) || (0 == activity_chunk_count))
					EM_DEBUG_EXCEPTION(" emstorage_get_activity Failed [ %d] or local_activity is NULL [%p] or activity_chunk_count is 0[%d]", err, local_activity, activity_chunk_count);
				else {
					EM_DEBUG_LOG("Found local activity type - %d", local_activity[0].activity_type);
					switch (local_activity[0].activity_type) {
						case ACTIVITY_MODIFYFLAG:  {
							if (emcore_sync_flag_with_server(local_activity[0].mail_id , &err))  {
								if (!emcore_delete_activity(&local_activity[0], &err))
									EM_DEBUG_EXCEPTION(">>>>>>Local Activity [ACTIVITY_MODIFYFLAG] [%d] ", err);
							}
						}
						break;

						case ACTIVITY_DELETEMAIL:
						case ACTIVITY_MOVEMAIL:
						case ACTIVITY_MODIFYSEENFLAG:
						case ACTIVITY_COPYMAIL:  {

							int j = 0, k = 0;
							int total_mail_ids = activity_chunk_count;

							int *mail_id_list = NULL;

							mail_id_list = (int *)em_malloc(sizeof(int) * total_mail_ids);

							if (NULL == mail_id_list) {
								EM_DEBUG_EXCEPTION("malloc failed... ");
								break;
							}

							do {

								for (j = 0; j < BULK_OPERATION_COUNT && (k < total_mail_ids); ++j, ++k)
									mail_id_list[j] = local_activity[k].mail_id;

								switch (local_activity[k-1].activity_type) {
									case ACTIVITY_DELETEMAIL:  {
										if (!emcore_delete_mail(local_activity[k-1].account_id,
																mail_id_list,
																j,
																EMAIL_DELETE_LOCAL_AND_SERVER,
																EMAIL_DELETED_BY_COMMAND,
																false,
																&err))
											EM_DEBUG_LOG("\t emcore_delete_mail failed - %d", err);
									}
									break;

									case ACTIVITY_MOVEMAIL:  {
										if (!emcore_move_mail_on_server_ex(local_activity[k-1].account_id ,
																		   local_activity[k-1].src_mbox,
																		   mail_id_list,
																		   j,
																		   local_activity[k-1].dest_mbox,
																		   &err))
											EM_DEBUG_LOG("\t emcore_move_mail_on_server_ex failed - %d", err);
									}
									break;
									case ACTIVITY_MODIFYSEENFLAG:  {
										int seen_flag = atoi(local_activity[0].src_mbox);
										if (!emcore_sync_seen_flag_with_server_ex(mail_id_list, j , seen_flag , &err)) /* local_activity[0].src_mbox points to the seen flag */
											EM_DEBUG_EXCEPTION("\t emcore_sync_seen_flag_with_server_ex failed - %d", err);
									}
									break;
								}

							} while (k < total_mail_ids);

							EM_SAFE_FREE(mail_id_list);
						}

						break;

						default:
							EM_DEBUG_LOG(">>>> No such Local Activity Handled by this thread [ %d ] >>> ", local_activity[0].activity_type);
						break;
					}

					emstorage_free_local_activity(&local_activity, activity_chunk_count, NULL);

					if (g_local_activity_run == 1) {
						EM_DEBUG_LOG(" Network event_data found.. Local sync Stopped..! ");
						break;
					}
				}
			}
		}
	}
	if (activity_id_list) {
		if (false == emstorage_free_activity_id_list(activity_id_list, &err))
			EM_DEBUG_EXCEPTION("emstorage_free_activity_id_list failed");
	}

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();

	return true;
}
#endif /* __FEATURE_LOCAL_ACTIVITY__ */

static int event_handler_EMAIL_EVENT_DOWNLOAD_BODY(int account_id, int mail_id, int option, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;
	email_mailbox_t mailbox;

	memset(&mailbox, 0x00, sizeof(mailbox));
	mailbox.account_id = account_id;

	if (!emnetwork_check_network_status(&err))  {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);

		emcore_notify_network_event(NOTI_DOWNLOAD_BODY_FAIL, mail_id, NULL, handle_to_be_published, err);
	} else {
		if (!emcore_download_body_multi_sections_bulk(NULL,
			mailbox.account_id,
			mail_id,
			option >> 1, 		/*  0 :  silent, 1 :  verbose */
			option & 0x01, 		/*  0 :  without attachments, 1 :  with attachments */
			NO_LIMITATION,
			handle_to_be_published,
			&err))
			EM_DEBUG_EXCEPTION("emcore_download_body_multi_sections_bulk failed - %d", err);
	}

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return true;
}

static int event_handler_EMAIL_EVENT_DOWNLOAD_ATTACHMENT(int account_id, int mail_id, int attachment_no, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;

	EM_DEBUG_LOG("attachment_no is %d", attachment_no);

	if (!emnetwork_check_network_status(&err))  {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
		emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_FAIL, mail_id, NULL, attachment_no, err);
	}
	else  {

#ifdef __ATTACHMENT_OPTI__
		if (!emcore_download_attachment_bulk(account_id, mail_id, attachment_no, &err))
			EM_DEBUG_EXCEPTION("\t emcore_download_attachment failed [%d]", err);
#else
		if (!emcore_download_attachment(account_id, mail_id, attachment_no, &err))
			EM_DEBUG_EXCEPTION("\t emcore_download_attachment failed [%d]", err);
#endif
	}

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return true;
}

static int event_handler_EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER(int mail_ids[], int num, email_flags_field_type field_type, int value, int *error)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;

	if (!emnetwork_check_network_status(&err))
		EM_DEBUG_EXCEPTION("dnet_init failed [%d]", err);
	else if (!emcore_sync_flags_field_with_server(mail_ids, num, field_type, value, &err))
		EM_DEBUG_EXCEPTION("emcore_sync_flags_field_with_server failed [%d]", err);

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return true;
}

static int event_handler_EMAIL_EVENT_VALIDATE_ACCOUNT(int account_id, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;

	if (!emnetwork_check_network_status(&err))  {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);

		if (!emcore_notify_network_event(NOTI_VALIDATE_ACCOUNT_FAIL, account_id, NULL,  handle_to_be_published, err))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_FAIL] Failed >>>>");
	}
	else  {

		if (!emcore_validate_account(account_id, &err)) {
			EM_DEBUG_EXCEPTION("emcore_validate_account failed account id  :  %d  err :  %d", account_id, err);

			if (err == EMAIL_ERROR_CANCELLED) {
				EM_DEBUG_EXCEPTION("notify  :  NOTI_VALIDATE_ACCOUNT_CANCEL ");
				if (!emcore_notify_network_event(NOTI_VALIDATE_ACCOUNT_CANCEL, account_id, NULL,  handle_to_be_published, err))
					EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_CANCEL] Failed >>>> ");
			}
			else {
				if (!emcore_notify_network_event(NOTI_VALIDATE_ACCOUNT_FAIL, account_id, NULL,  handle_to_be_published, err))
					EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_FAIL] Failed >>>> ");
			}
		}
		else {
			email_account_t *account_ref = NULL;
			account_ref = emcore_get_account_reference(account_id);

			if (account_ref) {
				EM_DEBUG_LOG("account_ref->incoming_server_type[%d]", account_ref->incoming_server_type);
				if ( EMAIL_SERVER_TYPE_IMAP4 == account_ref->incoming_server_type ) {
					if (!emcore_check_thread_status())
						err = EMAIL_ERROR_CANCELLED;
					else if (!emcore_sync_mailbox_list(account_id, "", handle_to_be_published, &err))
						EM_DEBUG_EXCEPTION("\t emcore_get_mailbox_list_to_be_sync falied - %d", err);
				}

				if (err > 0) {
					EM_DEBUG_EXCEPTION("emcore_validate_account succeeded account id  :  %d  err :  %d", account_id, err);
					if (!emcore_notify_network_event(NOTI_VALIDATE_ACCOUNT_FINISH, account_id, NULL,  handle_to_be_published, err))
						EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_FINISH] Success >>>>");
				}

				emcore_free_account(account_ref);
				EM_SAFE_FREE(account_ref);
			}
		}
	}

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return true;
}

static int event_handler_EMAIL_EVENT_UPDATE_MAIL(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas, int handle_to_be_published)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list[%p], input_attachment_count[%d], input_meeting_request[%p], input_from_eas[%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas);
	int err = EMAIL_ERROR_NONE;
/*
	if ( (err = emcore_update_mail(input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas)) != EMAIL_ERROR_NONE)
		EM_DEBUG_EXCEPTION("emcore_update_mail failed [%d]", err);
*/
	EM_DEBUG_FUNC_END("err [%d", err);
	return err;
}
static int event_handler_EMAIL_EVENT_SAVE_MAIL(int input_account_id, int input_mail_id, int input_handle_to_be_published)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d] input_mail_id [%d] input_handle_to_be_published [%d]", input_account_id, input_mail_id, input_handle_to_be_published);
	int err = EMAIL_ERROR_NONE;

	err = emcore_sync_mail_from_client_to_server(input_mail_id);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static int event_handler_EMAIL_EVENT_MOVE_MAIL(int account_id, int *mail_ids, int mail_id_count, int dest_mailbox_id, int src_mailbox_id, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE, ret = false;
	email_account_t *account_ref = NULL;

	if (!(account_ref = emcore_get_account_reference(account_id))) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/* Move mail local */
	/*
	if (!emcore_mail_move(mail_ids, mail_id_count, dest_mailbox.mailbox_name, EMAIL_MOVED_BY_COMMAND, 0, &err)) {
		EM_DEBUG_EXCEPTION("emcore_mail_move failed [%d]", err);
		goto FINISH_OFF;
	}
	*/

	if (account_ref->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
		/* Move mail on server */
		if (!emnetwork_check_network_status(&err))
			EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
		else {
#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__
			if (!emcore_move_mail_on_server_ex(account_id , src_mailbox_id, mail_ids, mail_id_count, dest_mailbox_id, &err))
				EM_DEBUG_EXCEPTION("emcore_move_mail_on_server_ex failed - %d", err);
#else
			if (!emcore_move_mail_on_server(account_id , src_mailbox_id, mail_ids, mail_id_count, dest_mailbox_id, &err))
				EM_DEBUG_EXCEPTION("\t emcore_move_mail_on_server failed - %d", err);
#endif
		}
	}

	ret = true;
FINISH_OFF:

	if (account_ref) {
		emcore_free_account(account_ref);
		EM_SAFE_FREE(account_ref);
	}

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

static int event_handler_EMAIL_EVENT_DELETE_MAILBOX(int mailbox_id, int on_server, int recursive, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_id[%d] on_server[%d] recursive[%d] handle_to_be_published[%d] error[%p]",  mailbox_id, on_server, recursive, handle_to_be_published, error);
	int err = EMAIL_ERROR_NONE;

	if (!emnetwork_check_network_status(&err))
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
	else  {
		if (( err = emcore_delete_mailbox(mailbox_id, on_server, recursive)) != EMAIL_ERROR_NONE)
			EM_DEBUG_EXCEPTION("emcore_delete failed [%d]", err);
	}

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return true;
}

static int event_handler_EMAIL_EVENT_CREATE_MAILBOX(email_mailbox_t *input_new_mailbox, int on_server, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

	if (!emnetwork_check_network_status(&err))  {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
	}
	else  {
		if (!emcore_create_mailbox(input_new_mailbox, on_server, &err))
			EM_DEBUG_EXCEPTION("emcore_create failed - %d", err);
	}

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return true;
}

static int event_handler_EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER(int mail_id, int *error)
{
	EM_DEBUG_FUNC_BEGIN("mail_id [%d], error [%p]", mail_id, error);

	int err = EMAIL_ERROR_NONE;

	if (!emnetwork_check_network_status(&err))
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
	else {
		if (!emcore_sync_flag_with_server(mail_id, &err))
			EM_DEBUG_EXCEPTION("emcore_sync_flag_with_server failed [%d]", err);
#ifdef __FEATURE_LOCAL_ACTIVITY__
		else {
			emstorage_activity_tbl_t new_activity;
			memset(&new_activity, 0x00, sizeof(emstorage_activity_tbl_t));
			new_activity.activity_type = ACTIVITY_MODIFYFLAG;
			new_activity.account_id    = event_data.account_id;
			new_activity.mail_id	   = event_data.event_param_data_4;
			new_activity.dest_mbox	   = NULL;
			new_activity.server_mailid = NULL;
			new_activity.src_mbox	   = NULL;

			if (!emcore_delete_activity(&new_activity, &err))
				EM_DEBUG_EXCEPTION(">>>>>>Local Activity [ACTIVITY_MODIFYFLAG] [%d] ", err);
		}
#endif /*  __FEATURE_LOCAL_ACTIVITY__ */
	}

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return true;
}

static int event_handler_EMAIL_EVENT_DELETE_MAIL_ALL(int input_account_id, int input_mailbox_id, int input_from_server, int *error)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d] input_mailbox_id [%d], input_from_server [%d], error [%p]", input_account_id, input_mailbox_id, input_from_server, error);
	int err = EMAIL_ERROR_NONE;

	if (!emcore_delete_all_mails_of_mailbox(input_account_id, input_mailbox_id, input_from_server, &err))
		EM_DEBUG_EXCEPTION("emcore_delete_all_mails_of_mailbox failed [%d]", err);

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return true;
}

static int event_handler_EMAIL_EVENT_DELETE_MAIL(int account_id, int *mail_id_list, int mail_id_count, int *error)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int ret = false;
	email_account_t *account_ref = NULL;

	if (!(account_ref = emcore_get_account_reference(account_id))) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!emcore_delete_mail(account_id, mail_id_list, mail_id_count, EMAIL_DELETE_FROM_SERVER, EMAIL_DELETED_BY_COMMAND, false, &err)) {
		EM_DEBUG_EXCEPTION("emcore_delete_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;
FINISH_OFF:

	if (account_ref) {
		emcore_free_account(account_ref);
		EM_SAFE_FREE(account_ref);
	}

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

static int event_hanlder_EMAIL_EVENT_SYNC_HEADER_OMA(int account_id, char *maibox_name, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

	if (!emnetwork_check_network_status(&err))  {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
		if (!emcore_notify_network_event(NOTI_DOWNLOAD_FAIL, account_id, maibox_name,  0, err))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_DOWNLOAD_FAIL] Failed");
	}
	else  {
		EM_DEBUG_LOG("Sync of all mailbox");
		if (!emcore_sync_mailbox_list(account_id, "", handle_to_be_published, &err))
			EM_DEBUG_EXCEPTION("emcore_sync_mailbox_list failed [%d]", err);
	}

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return true;
}

static int event_handler_EMAIL_EVENT_SEARCH_ON_SERVER(int account_id, int mailbox_id, char *criteria, 
                                                                                 int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN_SEC("account_id : [%d], mailbox_id : [%d], criteria : [%s]", account_id, mailbox_id, criteria);

	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int mail_id = 0;
	int thread_id = 0;
	char temp_uid_string[20] = {0,};

	emcore_uid_list uid_elem;
	emstorage_mailbox_tbl_t *search_mailbox = NULL;
	emstorage_mail_tbl_t *new_mail_tbl_data = NULL;

	MAILSTREAM *stream = NULL;
	MESSAGECACHE *mail_cache_element = NULL;
	ENVELOPE *env = NULL;
	emstorage_mailbox_tbl_t* local_mailbox = NULL;
	char mailbox_id_param_string[10] = {0,};

	if (account_id < 0 || mailbox_id == 0) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ( (err = emstorage_get_mailbox_by_id(mailbox_id, &local_mailbox)) != EMAIL_ERROR_NONE || !local_mailbox) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	SNPRINTF(mailbox_id_param_string, 10, "%d", local_mailbox->mailbox_id);

	if (!emcore_notify_network_event(NOTI_SEARCH_ON_SERVER_START, account_id, mailbox_id_param_string, handle_to_be_published, 0))
		EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_SEARCH_ON_SERVER_START] failed >>>>");

	if (!emnetwork_check_network_status(&err)) {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
		if (!emcore_notify_network_event(NOTI_SEARCH_ON_SERVER_FAIL, account_id, mailbox_id_param_string,  0, err))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_DOWNLOAD_FAIL] Failed");
		goto FINISH_OFF;
	}

	if (!emcore_connect_to_remote_mailbox(account_id, mailbox_id, (void **)&stream, &err)) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed");
		if (!emcore_notify_network_event(NOTI_SEARCH_ON_SERVER_FAIL, account_id, mailbox_id_param_string, handle_to_be_published, err))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_SEARCH_ON_SERVER_FAIL] Failed >>>>");
		goto FINISH_OFF;
	}

	if (!mail_search_full(stream, NIL, mail_criteria(criteria), SE_FREE)) {
		EM_DEBUG_EXCEPTION("mail_search failed");
		if (!emcore_notify_network_event(NOTI_SEARCH_ON_SERVER_FAIL, account_id, mailbox_id_param_string, handle_to_be_published, err))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_SEARCH_ON_SERVER_FAIL] Failed >>>>");
		goto FINISH_OFF;
	}

	for (i = 1; i <= stream->nmsgs; ++i) {
		mail_cache_element = mail_elt(stream, i);
		if (mail_cache_element->searched) {
			env = mail_fetchstructure_full(stream, i, NULL, FT_PEEK);

			memset(&uid_elem, 0x00, sizeof(uid_elem));

			uid_elem.msgno = mail_cache_element->msgno;
			SNPRINTF(temp_uid_string, 20, "%4lu", mail_cache_element->private.uid);
			uid_elem.uid = temp_uid_string;
			uid_elem.flag.seen = mail_cache_element->seen;

			if (!emcore_make_mail_tbl_data_from_envelope(account_id, stream, env, &uid_elem, &new_mail_tbl_data, &err) || !new_mail_tbl_data) {
				EM_DEBUG_EXCEPTION("emcore_make_mail_tbl_data_from_envelope failed [%d]", err);
				if (!emcore_notify_network_event(NOTI_SEARCH_ON_SERVER_FAIL, account_id, mailbox_id_param_string, handle_to_be_published, err))
					EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_SEARCH_ON_SERVER_FAIL] Failed >>>>");
				goto FINISH_OFF;
			}

			search_mailbox = em_malloc(sizeof(emstorage_mailbox_tbl_t));
			if (search_mailbox == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed");
				err = EMAIL_ERROR_OUT_OF_MEMORY;
				if (!emcore_notify_network_event(NOTI_SEARCH_ON_SERVER_FAIL, account_id, mailbox_id_param_string, handle_to_be_published, err))
					EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_SEARCH_ON_SERVER_FAIL] Failed >>>>");
				goto FINISH_OFF;
			}

			search_mailbox->account_id = account_id;
			search_mailbox->mailbox_id = mailbox_id;
			search_mailbox->mailbox_name = EM_SAFE_STRDUP(EMAIL_SEARCH_RESULT_MAILBOX_NAME);
			search_mailbox->mailbox_type = EMAIL_MAILBOX_TYPE_SEARCH_RESULT;

			if ((err = emcore_add_mail_to_mailbox(search_mailbox, new_mail_tbl_data, &mail_id, &thread_id)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_add_mail_to_mailbox failed [%d]", err);
				if (!emcore_notify_network_event(NOTI_SEARCH_ON_SERVER_FAIL, account_id, mailbox_id_param_string, handle_to_be_published, err))
					EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_SEARCH_ON_SERVER_FAIL] Failed >>>>");
				goto FINISH_OFF;
			}
			memset(mailbox_id_param_string, 0, 10);
			SNPRINTF(mailbox_id_param_string, 10, "%d", search_mailbox->mailbox_id);
			if (!emcore_notify_storage_event(NOTI_MAIL_ADD, account_id, mail_id, mailbox_id_param_string, thread_id)) {
				EM_DEBUG_EXCEPTION("emcore_notify_storage_event [NOTI_MAIL_ADD] failed");
			}

			if (new_mail_tbl_data) {
				emstorage_free_mail(&new_mail_tbl_data, 1, NULL);
				new_mail_tbl_data = NULL;
			}

			emstorage_free_mailbox(&search_mailbox, 1, NULL); /*prevent 17942*/
		}
	}

	if (err == EMAIL_ERROR_NONE && !emcore_notify_network_event(NOTI_SEARCH_ON_SERVER_FINISH, account_id, NULL, handle_to_be_published, 0))
		EM_DEBUG_EXCEPTION("emcore_notify_network_event[NOTI_SEARCH_ON_SERVER_FINISH] Failed >>>>>");

FINISH_OFF:
	if (stream)
		mail_close (stream);

	if (search_mailbox != NULL)
		emstorage_free_mailbox(&search_mailbox, 1, NULL);

	if (new_mail_tbl_data)
		emstorage_free_mail(&new_mail_tbl_data, 1, NULL);

	if (local_mailbox)
		emstorage_free_mailbox(&local_mailbox, 1, NULL);

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return true;
}

static int event_handler_EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER(int input_account_id, int input_mailbox_id, char *input_old_mailbox_path, char *input_new_mailbox_path, char *input_new_mailbox_alias, int handle_to_be_published)
{
	EM_DEBUG_FUNC_BEGIN_SEC("input_account_id [%d], input_mailbox_id [%d], input_old_mailbox_path %s], input_new_mailbox_path [%s], input_new_mailbox_alias [%s], handle_to_be_published [%d]", input_account_id, input_mailbox_id, input_old_mailbox_path, input_new_mailbox_path, input_new_mailbox_alias, handle_to_be_published);
	int err = EMAIL_ERROR_NONE;


	if (err == EMAIL_ERROR_NONE) {
		if ((err = emcore_rename_mailbox(input_mailbox_id, input_new_mailbox_path, input_new_mailbox_alias, NULL, 0, true, true, handle_to_be_published)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_rename_mailbox failed [%d]", err);
		}
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static void* worker_send_event_queue(void *arg)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	email_event_t *event_data = NULL;
	email_event_t *started_event = NULL;

	if (!emstorage_open(&err)) {
		EM_DEBUG_EXCEPTION("emstorage_open falied [%d]", err);
		return NULL;
	}

	while (g_send_event_loop) {
		/* get a event_data from event_data send queue */
		ENTER_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
		if (!emcore_retrieve_send_event(&event_data, &err)) {
			/* no event_data pending */
			if (err != EMAIL_ERROR_EVENT_QUEUE_EMPTY) {
				LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
				continue;
			}

			send_thread_run = 0;

			emdevice_set_sleep_on_off(true, NULL);

			//go to sleep when queue is empty
			SLEEP_CONDITION_VARIABLE(_send_event_available_signal, *_send_event_queue_lock);
			EM_DEBUG_LOG("Wake up by _send_event_available_signal");
			LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
		} else {
			LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
			EM_DEBUG_LOG(">>>>>>>>>>>>>>Got SEND event_data>>>>>>>>>>>>>>>>");
			emdevice_set_sleep_on_off(false, NULL);
			send_thread_run = 1;

			switch (event_data->type) {

				case EMAIL_EVENT_SEND_MAIL:
					if (!emcore_send_mail(event_data->event_param_data_4, &err))
						EM_DEBUG_EXCEPTION("emcore_send_mail failed [%d]", err);
					break;

				case EMAIL_EVENT_SEND_MAIL_SAVED:
					/* send mails to been saved in off-line mode */
					if (!emcore_send_saved_mail(event_data->account_id, event_data->event_param_data_3, &err))
						EM_DEBUG_EXCEPTION("emcore_send_saved_mail failed - %d", err);
					break;

#ifdef __FEATURE_LOCAL_ACTIVITY__
				case EMAIL_EVENT_LOCAL_ACTIVITY: {
					emdevice_set_sleep_on_off(false, NULL);
					emstorage_activity_tbl_t *local_activity = NULL;
					int activity_id_count = 0;
					int activity_chunk_count = 0;
					int *activity_id_list = NULL;
					int i = 0;

					if (false == emstorage_get_activity_id_list(event_data->account_id, &activity_id_list, &activity_id_count, ACTIVITY_SAVEMAIL, ACTIVITY_DELETEMAIL_SEND, true, &err)) {
						EM_DEBUG_EXCEPTION("emstorage_get_activity_id_list failed [%d]", err);
					}
					else {
						for (i = 0; i < activity_id_count; ++i) {
							if ((false == emstorage_get_activity(event_data->account_id, activity_id_list[i], &local_activity, &activity_chunk_count, true,  &err)) || (NULL == local_activity) || (0 == activity_chunk_count)) {
								EM_DEBUG_EXCEPTION(" emstorage_get_activity Failed [ %d] or local_activity is NULL [%p] or activity_chunk_count is 0[%d]", err, local_activity, activity_chunk_count);
							}
							else {
								EM_DEBUG_LOG("Found local activity type - %d", local_activity[0].activity_type);
								switch (local_activity[0].activity_type) {
									case ACTIVITY_SAVEMAIL:  {
										if (!emcore_sync_mail_from_client_to_server(event_data->account_id, local_activity[0].mail_id, &err)) {
											EM_DEBUG_EXCEPTION("emcore_sync_mail_from_client_to_server failed - %d ", err);
										}
									}
									break;

									case ACTIVITY_DELETEMAIL_SEND: 				/* New Activity Type Added for Race Condition and Crash Fix */ {
										if (!emcore_delete_mail(local_activity[0].account_id,
																&local_activity[0].mail_id,
																EMAIL_DELETE_FOR_SEND_THREAD,
																true,
																EMAIL_DELETED_BY_COMMAND,
																false,
																&err))  {
											EM_DEBUG_LOG("\t emcore_delete_mail failed - %d", err);
										}
									}
									break;

									default:  {
										EM_DEBUG_LOG(">>>> No such Local Activity Handled by this thread [ %d ] >>> ", local_activity[0].activity_type);
									}
									break;
								}

								emstorage_free_local_activity(&local_activity, activity_chunk_count, NULL);

								if (g_save_local_activity_run == 1) {
									EM_DEBUG_LOG(" Network event_data found.. Local sync Stopped..! ");
									break;
								}
							}

						}
						if (false == emstorage_free_activity_id_list(activity_id_list, &err)) {
							EM_DEBUG_LOG("emstorage_free_activity_id_list failed");
						}
					}

					emdevice_set_sleep_on_off(true, NULL);
				}
				break;
#endif /* __FEATURE_LOCAL_ACTIVITY__ */

				default:
					EM_DEBUG_LOG("Others not supported by Send Thread..! [%d]", event_data->type);
					break;
			}

			ENTER_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
			started_event = g_queue_pop_head(g_send_event_que);
			LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
			if (!started_event) {
				EM_DEBUG_EXCEPTION("Failed to g_queue_pop_head");
			} else {
				emcore_return_send_handle(started_event->handle);
				emcore_free_event(started_event);
				EM_SAFE_FREE(started_event);
			}
		}
	}

	if (!emstorage_close(&err))
		EM_DEBUG_EXCEPTION("emstorage_close falied [%d]", err);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return NULL;
}

INTERNAL_FUNC int emdaemon_start_thread_for_downloading_partial_body(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int i = 0, thread_error = -1;

	/* Clear Partial Body Event Queue*/
	memset(&g_partial_body_thd_event_que, 0x00, sizeof(g_partial_body_thd_event_que));

	for (i = 0; i < TOTAL_PARTIAL_BODY_EVENTS; ++i) {
		g_partial_body_thd_event_que[i].mailbox_name = NULL;
		g_partial_body_thd_event_que[i].mailbox_id = 0;
	}

	if (g_partial_body_thd)  {
		EM_DEBUG_EXCEPTION("partial body thread is already running...");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_UNKNOWN;

		return true;
	}

	g_partial_body_thd_next_event_idx = 0;
	g_partial_body_thd_loop = 1;
	g_partial_body_thd_queue_empty = true;
	g_partial_body_thd_queue_full = false;

	INITIALIZE_CONDITION_VARIABLE(_partial_body_thd_cond);

	/* create thread */
	/* THREAD_CREATE_JOINABLE(g_partial_body_thd, partial_body_download_thread, thread_error); */
	THREAD_CREATE(g_partial_body_thd, partial_body_download_thread, NULL, thread_error);

	if (thread_error != 0) {
		EM_DEBUG_EXCEPTION("cannot make thread...");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_UNKNOWN;
		return FAILURE;
	}

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	return false;

}


static gpointer partial_body_download_thread(gpointer data)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;
	email_session_t *session = NULL;
	email_event_partial_body_thd partial_body_thd_event;

	EM_DEBUG_LOG_DEV(" ************ PB THREAD ID IS ALIVE. ID IS [%d] ********************" , THREAD_SELF());

	/* Open connection with DB */

	if (false == emstorage_open(&err))  {
		EM_DEBUG_EXCEPTION("emstorage_open failed [%d]", err);
		return false;
	}

	/* Start the continuous loop */

	while (g_partial_body_thd_loop) {
		/*  Get an empty session  */
		/*  TODO :  Mutex should be used in session APIs */

		if (false == emcore_get_empty_session(&session))
			EM_DEBUG_EXCEPTION("emcore_get_empty_session failed...");
		else {	/* Get and Event from the Partial Body thread Event Queue */
			memset(&partial_body_thd_event, 0x00, sizeof(email_event_partial_body_thd));

			if (false == emcore_retrieve_partial_body_thread_event(&partial_body_thd_event, &err)) {
				if (EMAIL_ERROR_EVENT_QUEUE_EMPTY != err)
					EM_DEBUG_EXCEPTION("emcore_retrieve_partial_body_thread_event failed [%d]", err);
				else {
					EM_DEBUG_LOG(" partial body thread event_data queue is empty.");

					/*  Flush the que before starting local activity sync to clear the events in queue which are less than 10 in count  */
					if (!g_partial_body_bulk_dwd_queue_empty) {
						partial_body_thd_event.event_type = 0;
						partial_body_thd_event.account_id = g_partial_body_bulk_dwd_que[0].account_id;
						partial_body_thd_event.mailbox_id = g_partial_body_bulk_dwd_que[0].mailbox_id;
						partial_body_thd_event.mailbox_name = EM_SAFE_STRDUP(g_partial_body_bulk_dwd_que[0].mailbox_name); /* need to be freed */

						if (false == emcore_mail_partial_body_download(&partial_body_thd_event, &err))
							EM_DEBUG_EXCEPTION("emcore_mail_partial_body_download from event_data queue failed [%d]", err);

						emcore_pb_thd_set_local_activity_continue(true);
					}

					if (true == emcore_pb_thd_can_local_activity_continue()) {
						/*Check for local Activities */
						int is_local_activity_event_inserted = false;

						if (false == emcore_partial_body_thd_local_activity_sync(&is_local_activity_event_inserted, &err)) {
							EM_DEBUG_EXCEPTION("emcore_partial_body_thd_local_activity_sync failed [%d]", err);
						}
						else {
							if (true == is_local_activity_event_inserted) {
								emcore_pb_thd_set_local_activity_continue(false);

								emcore_clear_session(session);

								if (false == emcore_free_partial_body_thd_event(&partial_body_thd_event, &err))
									EM_DEBUG_EXCEPTION("emcore_free_partial_body_thd_event_cell failed [%d]", err);

								continue;
							}
						}
					}

					EM_DEBUG_LOG_DEV(" Partial Body Thread is going to sleep");

					emcore_set_pbd_thd_state(false);
					/*check: refactoring required*/
					ENTER_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);
					SLEEP_CONDITION_VARIABLE(_partial_body_thd_cond, _partial_body_thd_event_queue_lock);
					LEAVE_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);

					EM_DEBUG_LOG(" Partial Body Thread wakes up ");

					emcore_set_pbd_thd_state(true);
				}

			}
			else {
				EM_DEBUG_LOG(" Event Received from Partial Body Event Queue ");

				/* Since all events are network operations dnet init and sleep control is
				done before entering switch block*/

				emdevice_set_sleep_on_off(false, NULL);

				if (!emnetwork_check_network_status( &err))  {
					EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);;
				}
				else {
					/*  Process events  */
					EM_DEBUG_LOG("partial_body_thd_event.account_id[%d]", partial_body_thd_event.account_id);

					switch (partial_body_thd_event.event_type) {
						case EMAIL_EVENT_BULK_PARTIAL_BODY_DOWNLOAD:  {
							if (false == emcore_mail_partial_body_download(&partial_body_thd_event, &err)) {
								EM_DEBUG_EXCEPTION("emcore_mail_partial_body_download from event_data queue failed [%d]", err);
							}
							break;
						}
						case EMAIL_EVENT_LOCAL_ACTIVITY_SYNC_BULK_PBD:  {
							partial_body_thd_event.event_type = 0;

							/* Both the checks below make sure that before starting local activity there is no new/pending event_data in
							*   g_partial_body_thd_event_que and g_partial_body_bulk_dwd_que */
							if (false == emcore_is_partial_body_thd_que_empty())
								break;
							if (!g_partial_body_bulk_dwd_queue_empty)
								break;

							if (false == emcore_mail_partial_body_download(&partial_body_thd_event, &err))
								EM_DEBUG_EXCEPTION("emcore_mail_partial_body_download from activity table failed [%d]", err);
							break;
						}
						default:
							EM_DEBUG_EXCEPTION(" Warning :  Default case entered. This should not happen ");
							break;
					}
				}

				if (false == emcore_free_partial_body_thd_event(&partial_body_thd_event, &err))
					EM_DEBUG_EXCEPTION("emcore_free_partial_body_thd_event_cell failed [%d]", err);

				emdevice_set_sleep_on_off(true, NULL);
			}

			emcore_clear_session(session);
		}
	}

	/* If something is added to end thread in future for any case then if thread is holding any resources
	define a function emcore_partial_body_thd_loop_stop to release resources and call it
	here to end thread */
	return SUCCESS;
}
