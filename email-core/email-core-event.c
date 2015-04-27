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
#include <contacts_internal.h>
#include "c-client.h"
#include "email-convert.h"
#include "email-storage.h"
#include "email-network.h"
#include "email-device.h"
#include "email-utilities.h"
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
#include "email-core-signal.h"
#include "email-debug-log.h"


/*-----------------------------------------------------------------------------
 * Receving Event Queue
 *---------------------------------------------------------------------------*/
INTERNAL_FUNC thread_t g_srv_thread;
INTERNAL_FUNC pthread_cond_t  _event_available_signal = PTHREAD_COND_INITIALIZER;
INTERNAL_FUNC pthread_mutex_t *_event_queue_lock = NULL;
INTERNAL_FUNC pthread_mutex_t *_event_handle_map_lock = NULL;

INTERNAL_FUNC GQueue *g_event_que = NULL;
INTERNAL_FUNC int g_event_loop = 1;
INTERNAL_FUNC int handle_map[EVENT_QUEUE_MAX] = {0,};
INTERNAL_FUNC int recv_thread_run = 0;

static void fail_status_notify(email_event_t *event_data, int error);
static int emcore_get_new_handle(void);


/*-----------------------------------------------------------------------------
 * Sending Event Queue
 *---------------------------------------------------------------------------*/
INTERNAL_FUNC thread_t g_send_srv_thread;
INTERNAL_FUNC pthread_cond_t  _send_event_available_signal = PTHREAD_COND_INITIALIZER;
INTERNAL_FUNC pthread_mutex_t *_send_event_queue_lock = NULL;
INTERNAL_FUNC pthread_mutex_t *_send_event_handle_map_lock = NULL;

INTERNAL_FUNC GQueue *g_send_event_que = NULL;
INTERNAL_FUNC int g_send_event_loop = 1;
INTERNAL_FUNC int send_handle_map[EVENT_QUEUE_MAX] = {0,};
INTERNAL_FUNC int send_thread_run = 0;

static int emcore_get_new_send_handle(void);

/*-----------------------------------------------------------------------------
 * Partial Body Event Queue
 *---------------------------------------------------------------------------*/

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

INTERNAL_FUNC thread_t g_partial_body_thd ;								/* Determines if thread is created or not. Non Null means thread is created */
INTERNAL_FUNC pthread_mutex_t _partial_body_thd_event_queue_lock = PTHREAD_MUTEX_INITIALIZER;	/* Mutex to protect event queue */
INTERNAL_FUNC pthread_cond_t  _partial_body_thd_cond = PTHREAD_COND_INITIALIZER;				/* Condition variable on which partial body thread is waiting  */
INTERNAL_FUNC pthread_mutex_t _state_variables_lock;

INTERNAL_FUNC email_event_partial_body_thd g_partial_body_thd_event_que[TOTAL_PARTIAL_BODY_EVENTS];
INTERNAL_FUNC int g_partial_body_thd_next_event_idx = 0;			/* Index of Next Event to be processed in the queue*/
//INTERNAL_FUNC int g_partial_body_thd_loop = 1;						/* Variable to make a continuos while loop */
INTERNAL_FUNC int g_partial_body_thd_queue_empty = true;			/* Variable to determine if event queue is empty.True means empty*/
INTERNAL_FUNC int g_partial_body_thd_queue_full = false;			/* Variable to determine if event queue is full. True means full*/
INTERNAL_FUNC int g_pb_thd_local_activity_continue = true;			/* Variable to control local activity sync */
INTERNAL_FUNC int g_pbd_thd_state = false;								/* false :  thread is sleeping , true :  thread is working */

INTERNAL_FUNC email_event_partial_body_thd g_partial_body_bulk_dwd_que[BULK_PARTIAL_BODY_DOWNLOAD_COUNT];
static int g_partial_body_bulk_dwd_next_event_idx = 0;		/* Index of Next Event to be processed in the queue*/
INTERNAL_FUNC int g_partial_body_bulk_dwd_queue_empty = true;

static int emcore_copy_partial_body_thd_event(email_event_partial_body_thd *src, email_event_partial_body_thd *dest, int *error_code);
static int emcore_clear_bulk_pbd_que(int *err_code);

INTERNAL_FUNC email_event_t *sync_failed_event_data = NULL;

#endif

/*-----------------------------------------------------------------------------
 * Unused
 *---------------------------------------------------------------------------*/
typedef struct EVENT_CALLBACK_ELEM
{
	email_event_callback callback;
	void *event_data;
	struct EVENT_CALLBACK_ELEM *next;
} EVENT_CALLBACK_LIST;

static EVENT_CALLBACK_LIST *_event_callback_table[EMAIL_ACTION_NUM];		/*  array of singly-linked list for event callbacks */
INTERNAL_FUNC pthread_mutex_t *_event_callback_table_lock = NULL;

#ifdef __FEATURE_LOCAL_ACTIVITY__
INTERNAL_FUNC int g_local_activity_run = 0;
INTERNAL_FUNC int g_save_local_activity_run = 0;
static int event_handler_EMAIL_EVENT_LOCAL_ACTIVITY(int account_id, int *error);
#endif

#ifdef ENABLE_IMAP_IDLE_THREAD
extern int g_imap_idle_thread_alive;
extern int imap_idle_thread;
#endif /* ENABLE_IMAP_IDLE_THREAD */

static int is_gdk_lock_needed()
{
	if (g_event_loop)  {
		return (THREAD_SELF() == g_srv_thread);
	}
	return false;
}

static void fail_status_notify(email_event_t *event_data, int error)
{
	EM_DEBUG_FUNC_BEGIN();
	int account_id, mail_id;

	if(!event_data) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return;
	}
	account_id = event_data->account_id;
	mail_id  = event_data->event_param_data_4;

	EM_DEBUG_LOG("account_id[%d], mail_id[%d], error[%d]", account_id, mail_id, error);

	switch (event_data->type)  {
		case EMAIL_EVENT_SEND_MAIL:
			/* case EMAIL_EVENT_SEND_MAIL_SAVED:  */
			/* emcore_execute_event_callback(EMAIL_ACTION_SEND_MAIL, 0, 0, EMAIL_SEND_FAIL, account_id, mail_id, -1, error); */
			emcore_show_user_message(event_data->multi_user_name, mail_id, EMAIL_ACTION_SEND_MAIL, error);
			break;

		case EMAIL_EVENT_SYNC_HEADER:
			emcore_execute_event_callback(EMAIL_ACTION_SYNC_HEADER, 0, 0, EMAIL_LIST_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(event_data->multi_user_name, account_id, EMAIL_ACTION_SYNC_HEADER, error);
			break;

		case EMAIL_EVENT_DOWNLOAD_BODY:
			emcore_execute_event_callback(EMAIL_ACTION_DOWNLOAD_BODY, 0, 0, EMAIL_DOWNLOAD_FAIL, account_id, mail_id, -1, error);
			emcore_show_user_message(event_data->multi_user_name, account_id, EMAIL_ACTION_DOWNLOAD_BODY, error);
			break;

		case EMAIL_EVENT_DOWNLOAD_ATTACHMENT:
			emcore_execute_event_callback(EMAIL_ACTION_DOWNLOAD_ATTACHMENT, 0, 0, EMAIL_DOWNLOAD_FAIL, account_id, mail_id, -1, error);
			emcore_show_user_message(event_data->multi_user_name, account_id, EMAIL_ACTION_DOWNLOAD_ATTACHMENT, error);
			break;

		case EMAIL_EVENT_DELETE_MAIL:
		case EMAIL_EVENT_DELETE_MAIL_ALL:
			emcore_execute_event_callback(EMAIL_ACTION_DELETE_MAIL, 0, 0, EMAIL_DELETE_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(event_data->multi_user_name, account_id, EMAIL_ACTION_DELETE_MAIL, error);
			break;

		case EMAIL_EVENT_VALIDATE_ACCOUNT:
			emcore_execute_event_callback(EMAIL_ACTION_VALIDATE_ACCOUNT, 0, 0, EMAIL_VALIDATE_ACCOUNT_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(event_data->multi_user_name, account_id, EMAIL_ACTION_VALIDATE_ACCOUNT, error);
			break;

		case EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT:
			emcore_execute_event_callback(EMAIL_ACTION_VALIDATE_AND_CREATE_ACCOUNT, 0, 0, EMAIL_VALIDATE_ACCOUNT_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(event_data->multi_user_name, account_id, EMAIL_ACTION_VALIDATE_AND_CREATE_ACCOUNT, error);
			break;

		case EMAIL_EVENT_VALIDATE_ACCOUNT_EX:
			emcore_execute_event_callback(EMAIL_ACTION_VALIDATE_ACCOUNT_EX, 0, 0, EMAIL_VALIDATE_ACCOUNT_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(event_data->multi_user_name, account_id, EMAIL_ACTION_VALIDATE_ACCOUNT_EX, error);
			break;

		case EMAIL_EVENT_CREATE_MAILBOX:
			emcore_execute_event_callback(EMAIL_ACTION_CREATE_MAILBOX, 0, 0, EMAIL_LIST_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(event_data->multi_user_name, account_id, EMAIL_ACTION_CREATE_MAILBOX, error);
			break;

		case EMAIL_EVENT_DELETE_MAILBOX:
			emcore_execute_event_callback(EMAIL_ACTION_DELETE_MAILBOX, 0, 0, EMAIL_LIST_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(event_data->multi_user_name, account_id, EMAIL_ACTION_DELETE_MAILBOX, error);
			break;

		case EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT:
			emcore_execute_event_callback(EMAIL_ACTION_VALIDATE_AND_UPDATE_ACCOUNT, 0, 0, EMAIL_VALIDATE_ACCOUNT_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(event_data->multi_user_name, account_id, EMAIL_ACTION_VALIDATE_AND_UPDATE_ACCOUNT, error);
			break;

		case EMAIL_EVENT_SET_MAIL_SLOT_SIZE:
			emcore_execute_event_callback(EMAIL_ACTION_SET_MAIL_SLOT_SIZE, 0, 0, EMAIL_SET_SLOT_SIZE_FAIL, account_id, 0, -1, EMAIL_ERROR_NONE);
			break;

		case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
			emcore_execute_event_callback(EMAIL_ACTION_MOVE_MAILBOX, 0, 0, EMAIL_MOVE_MAILBOX_ON_IMAP_SERVER_FAIL, account_id, 0, -1, EMAIL_ERROR_NONE);
			emcore_show_user_message(event_data->multi_user_name, account_id, EMAIL_ACTION_SEARCH_ON_SERVER, error);
			break;

		case EMAIL_EVENT_UPDATE_MAIL:
			emcore_execute_event_callback(EMAIL_ACTION_UPDATE_MAIL, 0, 0, EMAIL_UPDATE_MAIL_FAIL, account_id, 0, -1, EMAIL_ERROR_NONE);
			break;

		case EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED:
			emcore_execute_event_callback(EMAIL_ACTION_EXPUNGE_MAILS_DELETED_FLAGGED, 0, 0, EMAIL_EXPUNGE_MAILS_DELETED_FLAGGED_FAIL, account_id, event_data->event_param_data_4, -1, EMAIL_ERROR_NONE);
			break;

		default:
			break;
	}
	EM_DEBUG_FUNC_END();
}


INTERNAL_FUNC void emcore_initialize_event_callback_table()
{
	ENTER_RECURSIVE_CRITICAL_SECTION(_event_callback_table_lock);

	int i;

	for (i = 0; i < EMAIL_ACTION_NUM; i++)
		_event_callback_table[i] = NULL;

	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_callback_table_lock);
}

int emcore_register_event_callback(email_action_t action, email_event_callback callback, void *event_data)
{
	EM_DEBUG_FUNC_BEGIN("action[%d], callback[%p], event_data[%p]", action, callback, event_data);

	if (callback == NULL)
		return false;

	int ret = false;

	ENTER_RECURSIVE_CRITICAL_SECTION(_event_callback_table_lock);

	EVENT_CALLBACK_LIST *node = _event_callback_table[action];

	while (node != NULL) {
		if (node->callback == callback && node->event_data == event_data)		/*  already registered */
			goto EXIT;

		node = node->next;
	}

	/*  not found, so keep going */

	node = em_malloc(sizeof(EVENT_CALLBACK_LIST));

	if (node == NULL)		/*  not enough memory */
		goto EXIT;

	node->callback = callback;
	node->event_data = event_data;
	node->next = _event_callback_table[action];

	_event_callback_table[action] = node;

	ret = true;

EXIT :
	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_callback_table_lock);
	EM_DEBUG_FUNC_END();
	return ret;
}

int emcore_unregister_event_callback(email_action_t action, email_event_callback callback)
{
	EM_DEBUG_FUNC_BEGIN("action[%d], callback[%p]", action, callback);

	if (callback == NULL)
		return false;

	int ret = false;

	ENTER_RECURSIVE_CRITICAL_SECTION(_event_callback_table_lock);

	EVENT_CALLBACK_LIST *prev = NULL;
	EVENT_CALLBACK_LIST *node = _event_callback_table[action];

	while (node != NULL) {
		if (node->callback == callback) {
			if (prev != NULL)
				prev->next = node->next;
			else
				_event_callback_table[action] = node->next;

			EM_SAFE_FREE(node);

			ret = true;
			break;
		}

		prev = node;
		node = node->next;
	}

	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_callback_table_lock);
	EM_DEBUG_FUNC_END();
	return ret;
}

void emcore_execute_event_callback(email_action_t action, int total, int done, int status, int account_id, int mail_id, int handle, int error)
{
	EM_DEBUG_FUNC_BEGIN("action[%d], total[%d], done[%d], status[%d], account_id[%d], mail_id[%d], handle[%d], error[%d]", action, total, done, status, account_id, mail_id, handle, error);

	int lock_needed = 0;
	lock_needed = is_gdk_lock_needed();

	if (lock_needed)  {
		/*  Todo  :  g_thread_yield */
	}

	ENTER_RECURSIVE_CRITICAL_SECTION(_event_callback_table_lock);

	EVENT_CALLBACK_LIST *node = _event_callback_table[action];

	while (node != NULL)  {
		if (node->callback != NULL)
			node->callback(total, done, status, account_id, mail_id, handle, node->event_data, error);
		node = node->next;
	}

	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_callback_table_lock);

	if (lock_needed)  {
	}
	EM_DEBUG_FUNC_END();
}

/* insert a event to event queue */
INTERNAL_FUNC int emcore_insert_event(email_event_t *event_data, int *handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("event_data[%p], handle[%p], err_code[%p]", event_data, handle, err_code);

	if (!event_data) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	if (!g_srv_thread) {
		EM_DEBUG_EXCEPTION("email-service is not ready");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_LOAD_ENGINE_FAILURE;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int q_length = 0;
	int new_handle = 0;

	ENTER_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);

	if (g_event_que)
		q_length = g_queue_get_length(g_event_que);

	EM_DEBUG_LOG("Q Length : [%d]", q_length);

	if (q_length > EVENT_QUEUE_MAX) {
		EM_DEBUG_EXCEPTION("event que is full...");
		error = EMAIL_ERROR_EVENT_QUEUE_FULL;
		ret = false;
	} else {
		new_handle = emcore_get_new_handle();

		if (new_handle) {
			event_data->status = EMAIL_EVENT_STATUS_WAIT;
			event_data->handle = new_handle;
			g_queue_push_tail(g_event_que, event_data);
			WAKE_CONDITION_VARIABLE(_event_available_signal);
			ret = true;
		}
		else {
			EM_DEBUG_EXCEPTION("event queue is full...");
			error = EMAIL_ERROR_EVENT_QUEUE_FULL;
			ret = false;
		}

	}

	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);

	if (handle)
		*handle = new_handle;

	switch (event_data->type)  {
		case EMAIL_EVENT_DOWNLOAD_ATTACHMENT:
		case EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER:
		case EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER:
		case EMAIL_EVENT_VALIDATE_ACCOUNT:
		case EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT:
		case EMAIL_EVENT_VALIDATE_ACCOUNT_EX:
		case EMAIL_EVENT_SAVE_MAIL:
		case EMAIL_EVENT_MOVE_MAIL:
		case EMAIL_EVENT_DELETE_MAIL:
		case EMAIL_EVENT_DELETE_MAIL_ALL:
		case EMAIL_EVENT_CREATE_MAILBOX:
		case EMAIL_EVENT_DELETE_MAILBOX:
		case EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT:
		case EMAIL_EVENT_SET_MAIL_SLOT_SIZE:
		case EMAIL_EVENT_UPDATE_MAIL:
		case EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED:
		case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
			break;

		default:
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
		{
			int is_local_activity_event_inserted = false;
			emcore_partial_body_thd_local_activity_sync (
                                                 event_data->multi_user_name, 
                                                 &is_local_activity_event_inserted, 
                                                 &error);
			if (error != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_partial_body_thd_local_activity_sync failed [%d]", error);
			}
			else {
				if (true == is_local_activity_event_inserted)
					emcore_pb_thd_set_local_activity_continue (false);
			}
		}
#endif
		break;
	}

	if (err_code) {
		EM_DEBUG_LOG("ERR [%d]", error);
		*err_code = error;
	}

	return ret;
}

/* get a event from event_data queue */
INTERNAL_FUNC int emcore_retrieve_event(email_event_t **event_data, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("event_data[%p], err_code[%p]", event_data, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int q_length = 0;
	email_event_t *poped = NULL;
	email_event_t *head_event = NULL;

	if (g_event_que)
		q_length = g_queue_get_length(g_event_que);

	EM_DEBUG_LOG("Q Length : [%d]", q_length);

	if (!q_length) {
		error = EMAIL_ERROR_EVENT_QUEUE_EMPTY;
		EM_DEBUG_LOG("QUEUE is empty");
		goto FINISH_OFF;
	}

	while (1) {
		head_event = (email_event_t *)g_queue_peek_head(g_event_que);
		if (!head_event) {
			error = EMAIL_ERROR_EVENT_QUEUE_EMPTY;
			EM_DEBUG_LOG_DEV ("QUEUE is empty");
			break;
		}
		if (head_event->status != EMAIL_EVENT_STATUS_WAIT) {
			EM_DEBUG_LOG("EVENT STATUS [%d]", head_event->status);
			poped = g_queue_pop_head(g_event_que);
			if (poped) {
				emcore_return_handle(poped->handle);
				emcore_free_event(poped);
				EM_SAFE_FREE(poped);
			} else {
				error = EMAIL_ERROR_EVENT_QUEUE_EMPTY;
				EM_DEBUG_LOG("QUEUE is empty");
				break;
			}
		} else {
			head_event->status = EMAIL_EVENT_STATUS_STARTED;
			*event_data = head_event;
			ret = true;
			break;
		}
	}

FINISH_OFF:

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/* check that event_data loop is continuous */
INTERNAL_FUNC int emcore_event_loop_continue(void)
{
	return g_event_loop;
}

INTERNAL_FUNC int emcore_is_event_queue_empty(void)
{
	int q_length = 0;
	int ret = false;

	ENTER_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
	if (g_event_que)
		q_length = g_queue_get_length(g_event_que);
	EM_DEBUG_LOG("Q Length : [%d]", q_length);
	if (q_length > 0) {
		ret = false;
	}
	else {
		EM_DEBUG_LOG("event que is empty");
		ret = true;
	}
	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);

	return ret;
}


INTERNAL_FUNC int emcore_is_send_event_queue_empty(void)
{
	int q_length = 0;
	int ret = false;

	ENTER_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
	if (g_send_event_que)
		q_length = g_queue_get_length(g_send_event_que);
	EM_DEBUG_LOG("Q Length : [%d]", q_length);
	if (q_length > 0) {
		ret = false;
	}
	else {
		EM_DEBUG_LOG("send event que is empty");
		ret = true;
	}
	LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);

	return ret;
}


INTERNAL_FUNC void emcore_get_sync_fail_event_data(email_event_t **event_data) 
{
	if (!sync_failed_event_data) {
		EM_DEBUG_EXCEPTION("sync_failed_event_data is NULL");
		return;
	}

	email_event_t *new_event = NULL;

	new_event = em_malloc(sizeof(email_event_t));
	if (new_event == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		return;
	}

	new_event->account_id = sync_failed_event_data->account_id;
	new_event->type = sync_failed_event_data->type;
	if (sync_failed_event_data->event_param_data_3)
		new_event->event_param_data_3 = EM_SAFE_STRDUP(sync_failed_event_data->event_param_data_3);
	new_event->event_param_data_4 = sync_failed_event_data->event_param_data_4;
	new_event->event_param_data_5 = sync_failed_event_data->event_param_data_5;

	*event_data = new_event;

	emcore_free_event(sync_failed_event_data);
	EM_SAFE_FREE(sync_failed_event_data);
	sync_failed_event_data = NULL;
}

INTERNAL_FUNC int emcore_insert_event_for_sending_mails(email_event_t *event_data, int *handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("event_data[%p], handle[%p], err_code[%p]", event_data, handle, err_code);

	if (!event_data) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	if (!g_send_srv_thread) {
		EM_DEBUG_EXCEPTION("email-service is not ready");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_LOAD_ENGINE_FAILURE;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int q_length = 0;
	int new_handle = 0;

	ENTER_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);

	if (g_send_event_que)
		q_length = g_queue_get_length(g_send_event_que);

	EM_DEBUG_LOG("SEND Q Length : [%d]", q_length);

	if (q_length > EVENT_QUEUE_MAX) {
		EM_DEBUG_EXCEPTION("send event que is full...");
		error = EMAIL_ERROR_EVENT_QUEUE_FULL;
		ret = false;
	} else {
		new_handle = emcore_get_new_send_handle();

		if (new_handle) {
			event_data->status = EMAIL_EVENT_STATUS_WAIT;
			event_data->handle = new_handle;
			g_queue_push_tail(g_send_event_que, event_data);
			WAKE_CONDITION_VARIABLE(_send_event_available_signal);
			ret = true;
		}
	}

	LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);

	if (handle)
		*handle = new_handle;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_LOG("Finish with [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emcore_retrieve_send_event(email_event_t **event_data, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("event_data[%p], err_code[%p]", event_data, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int q_length = 0;
	email_event_t *poped = NULL;
	email_event_t *head_event = NULL;

	/* g_queue_get_length is aborted when param is null */
	q_length = g_send_event_que? g_queue_get_length (g_send_event_que): 0; /*prevent 35141*/

	EM_DEBUG_LOG("SEND Q Length : [%d]", q_length);

	if (!q_length) {
		error = EMAIL_ERROR_EVENT_QUEUE_EMPTY;
		EM_DEBUG_LOG_DEV("SEND QUEUE is empty"); 
		goto FINISH_OFF;
	}

	while(1) {
		head_event = (email_event_t *)g_queue_peek_head(g_send_event_que);
		if (!head_event) {
			error = EMAIL_ERROR_EVENT_QUEUE_EMPTY;
			EM_DEBUG_LOG("SEND QUEUE is empty");
			break;
		}
		if (head_event->status != EMAIL_EVENT_STATUS_WAIT) {
			EM_DEBUG_LOG("EVENT STATUS [%d]", head_event->status);
			poped = g_queue_pop_head(g_send_event_que);
			if (poped) {
				emcore_return_send_handle(poped->handle);
				emcore_free_event(poped);
				EM_SAFE_FREE(poped);
			} else {
				error = EMAIL_ERROR_EVENT_QUEUE_EMPTY;
				EM_DEBUG_LOG("SEND QUEUE is empty");
				break;
			}
		} else {
			head_event->status = EMAIL_EVENT_STATUS_STARTED;
			*event_data = head_event;
			ret = true;
			break;
		}
	}

FINISH_OFF:

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/* finish api event_data loop */
INTERNAL_FUNC int emcore_send_event_loop_stop(int *err_code)
{
    EM_DEBUG_FUNC_BEGIN();

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	if (!g_send_srv_thread) 	 {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_UNKNOWN;
		return false;
	}

	/* stop event_data loop */
	g_send_event_loop = 0;

	emcore_cancel_all_send_mail_thread(err_code);

	WAKE_CONDITION_VARIABLE(_send_event_available_signal);		/*  MUST BE HERE */

	/* wait for thread finished */
	THREAD_JOIN(g_send_srv_thread);

	g_queue_free(g_send_event_que);

	g_send_srv_thread = 0;

	DELETE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
	DELETE_RECURSIVE_CRITICAL_SECTION(_send_event_handle_map_lock);
	DELETE_CONDITION_VARIABLE(_send_event_available_signal);

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	return true;
}

/* finish api event_data loop */
INTERNAL_FUNC int emcore_stop_event_loop(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	if (!g_srv_thread) {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_UNKNOWN;
		return false;
	}

	/* stop event_data loop */
	g_event_loop = 0;

	/* pthread_kill(g_srv_thread, SIGINT); */
	emcore_cancel_all_thread(err_code);

	WAKE_CONDITION_VARIABLE(_event_available_signal);

	/* wait for thread finished */
	THREAD_JOIN(g_srv_thread);

	g_queue_free(g_event_que);

	g_srv_thread = 0;

	DELETE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
	DELETE_RECURSIVE_CRITICAL_SECTION(_event_handle_map_lock);
	DELETE_RECURSIVE_CRITICAL_SECTION(_event_callback_table_lock);
	DELETE_CONDITION_VARIABLE(_event_available_signal);

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;
	EM_DEBUG_FUNC_END();
	return true;
}

/* check event thread status (worker_event_queue)
* 0 : stop job 1 : continue job
event list handled by event thread :
case EMAIL_EVENT_SYNC_IMAP_MAILBOX:
case EMAIL_EVENT_SYNC_HEADER:
case EMAIL_EVENT_SYNC_HEADER_OMA:
case EMAIL_EVENT_DOWNLOAD_BODY:
case EMAIL_EVENT_DOWNLOAD_ATTACHMENT:
case EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER:
case EMAIL_EVENT_DELETE_MAIL:
case EMAIL_EVENT_DELETE_MAIL_ALL:
case EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER:
case EMAIL_EVENT_CREATE_MAILBOX:
case EMAIL_EVENT_DELETE_MAILBOX:
case EMAIL_EVENT_SAVE_MAIL:
case EMAIL_EVENT_MOVE_MAIL:
case EMAIL_EVENT_VALIDATE_ACCOUNT:
case EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT: {
case EMAIL_EVENT_VALIDATE_ACCOUNT_EX: {
case EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT: {
case EMAIL_EVENT_UPDATE_MAIL:
case EMAIL_EVENT_SET_MAIL_SLOT_SIZE:
case EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED:
case EMAIL_EVENT_LOCAL_ACTIVITY:
case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
case EMAIL_EVENT_QUERY_SMTP_MAIL_SIZE_LIMIT:
*/
INTERNAL_FUNC int emcore_check_event_thread_status(int *event_type, int handle)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int q_length = 0;
	email_event_t *active_event = NULL;

	ENTER_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);

	if (g_event_que)
		q_length = g_queue_get_length(g_event_que);

	if (q_length) {
		active_event = (email_event_t *)g_queue_peek_head(g_event_que);
		if (active_event) {
			if (active_event->handle == handle) {
				if (event_type)
					*event_type = active_event->type;
				if (active_event->status == EMAIL_EVENT_STATUS_STARTED)
					ret = true;
				else
					ret = false;
			} else {
				ret = true;
			}
		}
	} else {
		EM_DEBUG_LOG("Rcv Queue is empty [%d]", q_length);
		ret = true;
	}

	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/* cancel a job  */
INTERNAL_FUNC int emcore_cancel_thread(int handle, void *arg, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("handle[%d], arg[%p], err_code[%p]", handle, arg, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int found = 0;
	int i = 0;
	int q_length = 0;
	email_event_t *found_elm = NULL;
	email_event_t *pop_elm = NULL;

	ENTER_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);

	if (g_event_que)
		q_length = g_queue_get_length(g_event_que);

	for (i = 0; i < q_length; i++) {
		found_elm = (email_event_t *)g_queue_peek_nth(g_event_que, i);
		if (found_elm && found_elm->handle == handle) {
			EM_DEBUG_LOG("Found Queue element[%d] with handle[%d]", i, handle);
			found = 1;
			break;
		}
	}

	if (found) {
		if (found_elm->status == EMAIL_EVENT_STATUS_WAIT) {
			fail_status_notify(found_elm, EMAIL_ERROR_CANCELLED);

			switch (found_elm->type) {

				case EMAIL_EVENT_SEND_MAIL:
				case EMAIL_EVENT_SEND_MAIL_SAVED:
					EM_DEBUG_LOG("EMAIL_EVENT_SEND_MAIL or EMAIL_EVENT_SEND_MAIL_SAVED");
					if (!emcore_notify_network_event(NOTI_SEND_CANCEL, found_elm->account_id, NULL , found_elm->event_param_data_4, err))
						EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_SEND_CANCEL] Failed >>>>");
					break;

				case EMAIL_EVENT_DOWNLOAD_BODY:
					EM_DEBUG_LOG("EMAIL_EVENT_DOWNLOAD_BODY");
					if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_CANCEL, found_elm->account_id, NULL , found_elm->event_param_data_4, err))
						EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_CANCEL] Failed >>>>");
					break;

				case EMAIL_EVENT_DOWNLOAD_ATTACHMENT:
					EM_DEBUG_LOG("EMAIL_EVENT_DOWNLOAD_ATTACHMENT");
					if (!emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_CANCEL, found_elm->event_param_data_4, NULL , found_elm->event_param_data_5, err))
						EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_DOWNLOAD_ATTACH_CANCEL] Failed >>>>");
					break;

				case EMAIL_EVENT_SYNC_HEADER:
				case EMAIL_EVENT_SYNC_HEADER_OMA:
				case EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER:
				case EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER:
					EM_DEBUG_LOG("EMAIL_EVENT_SYNC_HEADER, EMAIL_EVENT_DOWNLOAD_ATTACHMENT");
					break;

				case EMAIL_EVENT_VALIDATE_ACCOUNT:
					EM_DEBUG_LOG("validate account waiting: cancel account id[%d]", found_elm->account_id);
					if (!emcore_notify_network_event(NOTI_VALIDATE_ACCOUNT_CANCEL, found_elm->account_id, NULL , found_elm->event_param_data_4, err))
						EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_CANCEL] Failed >>>>");
					break;

				case EMAIL_EVENT_DELETE_MAIL:
				case EMAIL_EVENT_DELETE_MAIL_ALL:
				case EMAIL_EVENT_SYNC_IMAP_MAILBOX:
				case EMAIL_EVENT_SAVE_MAIL:
				case EMAIL_EVENT_MOVE_MAIL:
				case EMAIL_EVENT_CREATE_MAILBOX:
				case EMAIL_EVENT_DELETE_MAILBOX:
				case EMAIL_EVENT_SET_MAIL_SLOT_SIZE:
				case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
					EM_DEBUG_LOG("EMAIL_EVENT_DELETE_MAIL, EMAIL_EVENT_SYNC_IMAP_MAILBOX");
					break;

				default:
					break;
			}

			pop_elm = (email_event_t *)g_queue_pop_nth(g_event_que, i);
			if (pop_elm) {
				emcore_return_handle(pop_elm->handle);
				emcore_free_event(pop_elm);
				EM_SAFE_FREE(pop_elm);
			} else {
				EM_DEBUG_LOG("Failed to g_queue_pop_nth [%d] element", i);
			}
		}
		else {
			switch (found_elm->type) {

				case EMAIL_EVENT_SYNC_HEADER:
					EM_DEBUG_LOG("EMAIL_EVENT_SYNC_HEADER");
					if (!emcore_notify_network_event(NOTI_DOWNLOAD_CANCEL, found_elm->account_id, NULL , found_elm->event_param_data_4, err))
						EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_DOWNLOAD_CANCEL] Failed >>>>");
					if ((err = emcore_update_sync_status_of_account(found_elm->multi_user_name, found_elm->account_id, SET_TYPE_MINUS, SYNC_STATUS_SYNCING)) != EMAIL_ERROR_NONE)
						EM_DEBUG_EXCEPTION("emcore_update_sync_status_of_account failed [%d]", err);
					break;

				case EMAIL_EVENT_DOWNLOAD_ATTACHMENT:
					EM_DEBUG_LOG("EMAIL_EVENT_DOWNLOAD_ATTACHMENT");
					if (!emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_CANCEL, found_elm->event_param_data_4, NULL , found_elm->event_param_data_5, err))
						EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_DOWNLOAD_ATTACH_CANCEL] Failed >>>>");
					break;

				default:
					break;
			}
			found_elm->status = EMAIL_EVENT_STATUS_CANCELED;
		}

		ret = true;
	}

	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

/* cancel all job  */
INTERNAL_FUNC int emcore_cancel_all_thread(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("err_code[%p]", err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int q_length = 0;
	int i = 0;
	email_event_t *pop_elm = NULL;

	ENTER_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);

	/* g_queue_get_length is aborted when param is null */
	q_length = g_event_que? g_queue_get_length (g_event_que): 0; /*prevent 35142 */

	for (i = 0; i < q_length; i++) {
		pop_elm = (email_event_t *)g_queue_peek_nth(g_event_que, i);

		if (pop_elm && (pop_elm->status == EMAIL_EVENT_STATUS_WAIT)) {

			fail_status_notify(pop_elm, EMAIL_ERROR_CANCELLED);

			switch (pop_elm->type) {

				case EMAIL_EVENT_SEND_MAIL:
				case EMAIL_EVENT_SEND_MAIL_SAVED:
					EM_DEBUG_LOG("EMAIL_EVENT_SEND_MAIL or EMAIL_EVENT_SEND_MAIL_SAVED");
					if (!emcore_notify_network_event(NOTI_SEND_CANCEL, pop_elm->account_id, NULL , pop_elm->event_param_data_4, err))
						EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_SEND_CANCEL] Failed >>>>");
					break;

				case EMAIL_EVENT_DOWNLOAD_BODY:
					EM_DEBUG_LOG("EMAIL_EVENT_DOWNLOAD_BODY");
					if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_CANCEL, pop_elm->account_id, NULL , pop_elm->event_param_data_4, err))
						EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_SEND_CANCEL] Failed >>>>");
					break;

				case EMAIL_EVENT_SYNC_HEADER:
				case EMAIL_EVENT_SYNC_HEADER_OMA:
				case EMAIL_EVENT_DOWNLOAD_ATTACHMENT:
				case EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER:
				case EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER:
					EM_DEBUG_LOG("EMAIL_EVENT_SYNC_HEADER, EMAIL_EVENT_DOWNLOAD_ATTACHMENT");
					break;

				case EMAIL_EVENT_VALIDATE_ACCOUNT:
					EM_DEBUG_LOG("validate account waiting: cancel account id[%d]", pop_elm->account_id);
					if (!emcore_notify_network_event(NOTI_VALIDATE_ACCOUNT_CANCEL, pop_elm->account_id, NULL , pop_elm->event_param_data_4, err))
						EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_CANCEL] Failed >>>>");
					break;

				case EMAIL_EVENT_DELETE_MAIL:
				case EMAIL_EVENT_DELETE_MAIL_ALL:
				case EMAIL_EVENT_SYNC_IMAP_MAILBOX:
				case EMAIL_EVENT_SAVE_MAIL:
				case EMAIL_EVENT_MOVE_MAIL:
				case EMAIL_EVENT_CREATE_MAILBOX:
				case EMAIL_EVENT_DELETE_MAILBOX:
				case EMAIL_EVENT_SET_MAIL_SLOT_SIZE:
				case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
					EM_DEBUG_LOG("EMAIL_EVENT_DELETE_MAIL, EMAIL_EVENT_SYNC_IMAP_MAILBOX");
					break;

				default:
					break;
			}

			pop_elm = (email_event_t *)g_queue_pop_nth(g_event_que, i);
			if (pop_elm) {
				emcore_return_handle(pop_elm->handle);
				emcore_free_event(pop_elm);
				EM_SAFE_FREE(pop_elm);
				i--;
			}
			if (g_event_que)
				q_length = g_queue_get_length(g_event_que);

		}
		else {
			pop_elm->status = EMAIL_EVENT_STATUS_CANCELED;
		}
	}
	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);

	ret = true;

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

/* check thread status
* 0 : stop job 1 : continue job
*/
INTERNAL_FUNC int emcore_check_send_mail_thread_status(void)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int q_length = 0;
	email_event_t *active_event = NULL;

	ENTER_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);

	if (g_send_event_que)
		q_length = g_queue_get_length(g_send_event_que);

	if (q_length) {
		active_event = (email_event_t *)g_queue_peek_head(g_send_event_que);
		if (active_event) {
			if (active_event->status == EMAIL_EVENT_STATUS_STARTED)
				ret = true;
			else
				ret = false;
		}
	} else {
		EM_DEBUG_LOG("Send Queue is empty [%d]", q_length);
		ret = true;
	}

	LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emcore_cancel_all_threads_of_an_account(char *multi_user_name, int account_id)
{
	EM_DEBUG_FUNC_BEGIN();
	int error_code = EMAIL_ERROR_NONE;
	int i = 0;
	int q_length = 0;
	email_event_t *found_elm = NULL;
	email_event_t *pop_elm = NULL;

	ENTER_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);

	if (g_event_que)
		q_length = g_queue_get_length(g_event_que);

	for (i = 0; i < q_length; i++) {
		found_elm = (email_event_t *)g_queue_peek_nth(g_event_que, i);
		if (found_elm && (found_elm->account_id == account_id || found_elm->account_id == ALL_ACCOUNT) 
                && (!EM_SAFE_STRCASECMP(found_elm->multi_user_name, multi_user_name) || (!found_elm && !multi_user_name))) {
			EM_DEBUG_LOG("Found Queue element[%d]", i);

			if (found_elm->status == EMAIL_EVENT_STATUS_WAIT) {
				fail_status_notify(found_elm, EMAIL_ERROR_CANCELLED);

				switch (found_elm->type) {

					case EMAIL_EVENT_SEND_MAIL:
					case EMAIL_EVENT_SEND_MAIL_SAVED:
						EM_DEBUG_LOG("EMAIL_EVENT_SEND_MAIL or EMAIL_EVENT_SEND_MAIL_SAVED");
						if (!emcore_notify_network_event(NOTI_SEND_CANCEL, found_elm->account_id, NULL , found_elm->event_param_data_4, error_code))
							EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_SEND_CANCEL] Failed >>>>");
						break;

					case EMAIL_EVENT_DOWNLOAD_BODY:
						EM_DEBUG_LOG("EMAIL_EVENT_DOWNLOAD_BODY");
						if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_CANCEL, found_elm->account_id, NULL , found_elm->event_param_data_4, error_code))
							EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_SEND_CANCEL] Failed >>>>");
						break;

					case EMAIL_EVENT_SYNC_HEADER:
					case EMAIL_EVENT_SYNC_HEADER_OMA:
					case EMAIL_EVENT_DOWNLOAD_ATTACHMENT:
					case EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER:
					case EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER:
						EM_DEBUG_LOG("EMAIL_EVENT_SYNC_HEADER, EMAIL_EVENT_DOWNLOAD_ATTACHMENT");
						break;

					case EMAIL_EVENT_VALIDATE_ACCOUNT:
						EM_DEBUG_LOG("validate account waiting: cancel account id[%d]", found_elm->account_id);
						if (!emcore_notify_network_event(NOTI_VALIDATE_ACCOUNT_CANCEL, found_elm->account_id, NULL , found_elm->event_param_data_4, error_code))
							EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_CANCEL] Failed >>>>");
						break;

					case EMAIL_EVENT_DELETE_MAIL:
					case EMAIL_EVENT_DELETE_MAIL_ALL:
					case EMAIL_EVENT_SYNC_IMAP_MAILBOX:
					case EMAIL_EVENT_SAVE_MAIL:
					case EMAIL_EVENT_MOVE_MAIL:
					case EMAIL_EVENT_CREATE_MAILBOX:
					case EMAIL_EVENT_DELETE_MAILBOX:
					case EMAIL_EVENT_SET_MAIL_SLOT_SIZE:
					case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
						EM_DEBUG_LOG("EMAIL_EVENT_DELETE_MAIL, EMAIL_EVENT_SYNC_IMAP_MAILBOX");
						break;

					default:
						break;
				}

				pop_elm = g_queue_pop_nth(g_event_que, i);
				if (pop_elm) {
					emcore_return_handle(pop_elm->handle);
					emcore_free_event(pop_elm);
					EM_SAFE_FREE(pop_elm);
					i--;
				} else {
					EM_DEBUG_LOG("Failed to g_queue_pop_nth [%d] element", i);
				}
				if (g_event_que)
					q_length = g_queue_get_length(g_event_que);

			}
			else {
				found_elm->status = EMAIL_EVENT_STATUS_CANCELED;
			}
		}
	}

	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);

	EM_DEBUG_FUNC_END();
	return error_code;
}


/* cancel send mail job  */
INTERNAL_FUNC int emcore_cancel_send_mail_thread(int handle, void *arg, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("handle[%d], arg[%p], err_code[%p]", handle, arg, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int found = 0;
	int i = 0;
	int q_length = 0;
	email_event_t *found_elm = NULL;
	email_event_t *pop_elm = NULL;

	ENTER_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);

	if (g_send_event_que)
		q_length = g_queue_get_length(g_send_event_que);

	for (i = 0; i < q_length; i++) {
		found_elm = (email_event_t *)g_queue_peek_nth(g_send_event_que, i);
		if (found_elm && (found_elm->handle == handle)) {
			EM_DEBUG_LOG("Found Send Queue element[%d] with handle[%d]", i, handle);
			found = 1;
			break;
		}
	}

	if (found) {
		if (found_elm->status == EMAIL_EVENT_STATUS_WAIT) {
			fail_status_notify(found_elm, EMAIL_ERROR_CANCELLED);

			switch (found_elm->type) {
				case EMAIL_EVENT_SEND_MAIL:
				case EMAIL_EVENT_SEND_MAIL_SAVED:
					found_elm->status = EMAIL_EVENT_STATUS_CANCELED;
					if (!emcore_notify_network_event(NOTI_SEND_CANCEL, found_elm->account_id, NULL, found_elm->event_param_data_4, err))
						EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_SEND_CANCEL] Failed >>>>");
					break;
				default:
					break;
			}

			pop_elm = (email_event_t *)g_queue_pop_nth(g_send_event_que, i);
			if (pop_elm) {
				emcore_return_send_handle(pop_elm->handle);
				emcore_free_event(pop_elm);
				EM_SAFE_FREE(pop_elm);
			} else {
				EM_DEBUG_LOG("Failed to g_queue_pop_nth [%d] element", i);
			}
		} else {
			found_elm->status = EMAIL_EVENT_STATUS_CANCELED;
		}

		ret = true;
	}

	LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_cancel_all_send_mail_thread(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("err_code[%p]", err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int q_length = 0;
	int i = 0;
	email_event_t *pop_elm = NULL;

	ENTER_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);

	if (g_send_event_que)
		q_length = g_queue_get_length(g_send_event_que);

	for (i = 0; i < q_length; i++) {
		pop_elm = (email_event_t *)g_queue_peek_nth(g_send_event_que, i);

		if (pop_elm && pop_elm->status == EMAIL_EVENT_STATUS_WAIT) {

			fail_status_notify(pop_elm, EMAIL_ERROR_CANCELLED);

			switch (pop_elm->type) {
				case EMAIL_EVENT_SEND_MAIL:
				case EMAIL_EVENT_SEND_MAIL_SAVED:
					pop_elm->status = EMAIL_EVENT_STATUS_CANCELED;
					if (!emcore_notify_network_event(NOTI_SEND_CANCEL, pop_elm->account_id, NULL, pop_elm->event_param_data_4, err))
						EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_SEND_CANCEL] Failed >>>>");
					break;
				default:
					break;
			}

			pop_elm = (email_event_t *)g_queue_pop_nth(g_send_event_que, i);
			if (pop_elm) {
				emcore_return_send_handle(pop_elm->handle);
				emcore_free_event(pop_elm);
				EM_SAFE_FREE(pop_elm);
				i--;
			}

			if (g_send_event_que)
				q_length = g_queue_get_length(g_send_event_que);

		} else {
			pop_elm->status = EMAIL_EVENT_STATUS_CANCELED;
		}
	}
	LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);

	ret = true;

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_get_task_information(email_task_information_t **output_task_information, int *output_task_information_count)
{
	EM_DEBUG_FUNC_BEGIN("output_task_information[%p] output_task_information_count[%p]", output_task_information, output_task_information_count);
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int index = 0;
	int q_length = 0;
	email_task_information_t *task_information = NULL;
	email_event_t *elm = NULL;

	if (output_task_information == NULL || output_task_information_count == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (g_event_que)
		q_length = g_queue_get_length(g_event_que);

	if (!q_length) {
		err = EMAIL_ERROR_DATA_NOT_FOUND;
		goto FINISH_OFF;
	}

	if ((task_information = em_malloc(sizeof(email_task_information_t) * q_length)) == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < q_length; i++) {
		elm = (email_event_t *)g_queue_peek_nth(g_event_que, i);
		if (elm && (elm->type != EMAIL_EVENT_NONE && elm->status != EMAIL_EVENT_STATUS_CANCELED)) {
			task_information[index].handle     = elm->handle;
			task_information[index].account_id = elm->account_id;
			task_information[index].type       = elm->type;
			task_information[index].status     = elm->status;
			index++;
		}
	}

	*output_task_information_count = q_length;
	*output_task_information       = task_information;

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_free_event(email_event_t *event_data)
{
	EM_DEBUG_FUNC_BEGIN("event_data [%p]", event_data);
	if(!event_data) 
		return EMAIL_ERROR_INVALID_PARAM;

	switch(event_data->type) {
		case EMAIL_EVENT_SYNC_IMAP_MAILBOX:
			EM_SAFE_FREE(event_data->event_param_data_3);
			break;

		case EMAIL_EVENT_SYNC_HEADER:  /* synchronize mail header  */
			break;

		case EMAIL_EVENT_SYNC_HEADER_OMA:  /*  synchronize mail header for OMA */
			EM_SAFE_FREE(event_data->event_param_data_1);
			break;

		case EMAIL_EVENT_DOWNLOAD_BODY:  /*  download mail body  */
			break;

		case EMAIL_EVENT_DOWNLOAD_ATTACHMENT:  /*  download attachment */
			break;

		case EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER:  /*  Sync flags field */
			EM_SAFE_FREE(event_data->event_param_data_3);
			break;

		case EMAIL_EVENT_DELETE_MAIL:  /*  delete mails */
			EM_SAFE_FREE(event_data->event_param_data_3);
			break;

		case EMAIL_EVENT_DELETE_MAIL_ALL:  /*  delete all mails */
			break;

#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
		case EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER:
			break;
#endif

		case EMAIL_EVENT_CREATE_MAILBOX:
			emcore_free_mailbox((email_mailbox_t*)event_data->event_param_data_1);
			EM_SAFE_FREE(event_data->event_param_data_1);
			break;

		case EMAIL_EVENT_DELETE_MAILBOX:
			break;

		case EMAIL_EVENT_SAVE_MAIL:
			break;

		case EMAIL_EVENT_MOVE_MAIL:
			EM_SAFE_FREE(event_data->event_param_data_3);
			break;

		case EMAIL_EVENT_VALIDATE_ACCOUNT:
			break;

		case EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT:
			emcore_free_account((email_account_t *)event_data->event_param_data_1);
			EM_SAFE_FREE(event_data->event_param_data_1);

			break;

		case EMAIL_EVENT_VALIDATE_ACCOUNT_EX:
			emcore_free_account((email_account_t *)event_data->event_param_data_1);
			EM_SAFE_FREE(event_data->event_param_data_1);
			break;

		case EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT:
			emcore_free_account((email_account_t *)event_data->event_param_data_1);
			EM_SAFE_FREE(event_data->event_param_data_1);
			break;

		case EMAIL_EVENT_UPDATE_MAIL: {
				email_mail_data_t* input_mail_data = (email_mail_data_t*) event_data->event_param_data_1;
				email_attachment_data_t* input_attachment_data_list = (email_attachment_data_t*) event_data->event_param_data_2;
				email_meeting_request_t* input_meeting_request = (email_meeting_request_t*) event_data->event_param_data_3;
				int input_attachment_count = event_data->event_param_data_4;
				emcore_free_mail_data_list(&input_mail_data, 1);
				emcore_free_attachment_data(&input_attachment_data_list, input_attachment_count, NULL);
				emstorage_free_meeting_request(input_meeting_request);				
			}
			break;

		case EMAIL_EVENT_SET_MAIL_SLOT_SIZE:
			break;

		case EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED:
			break;

#ifdef __FEATURE_LOCAL_ACTIVITY__
		case EMAIL_EVENT_LOCAL_ACTIVITY:
			break;
#endif /* __FEATURE_LOCAL_ACTIVITY__*/

		case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
			EM_SAFE_FREE(event_data->event_param_data_1);
			EM_SAFE_FREE(event_data->event_param_data_2);
			EM_SAFE_FREE(event_data->event_param_data_3);
			break;


		case EMAIL_EVENT_SEND_MAIL:
			break;

		case EMAIL_EVENT_SEND_MAIL_SAVED:
			EM_SAFE_FREE(event_data->event_param_data_3);
			break;

		case EMAIL_EVENT_QUERY_SMTP_MAIL_SIZE_LIMIT:
			break;

#ifdef __FEATURE_LOCAL_ACTIVITY__
		case EMAIL_EVENT_LOCAL_ACTIVITY: {
		break;
#endif /* __FEATURE_LOCAL_ACTIVITY__ */

		default : /*free function is not implemented*/
			EM_DEBUG_EXCEPTION("event %d is NOT freed, possibly memory leaks", event_data->type);	
	}

	EM_SAFE_FREE(event_data->multi_user_name);
	event_data->event_param_data_1 = event_data->event_param_data_2 = event_data->event_param_data_3 = NULL;

	EM_DEBUG_FUNC_END();
	return true;
}

static int emcore_get_new_handle(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int i = 0;
	int ret_handle = 0;

	ENTER_RECURSIVE_CRITICAL_SECTION(_event_handle_map_lock);
	for (i = 0; i < EVENT_QUEUE_MAX; i++) {
		if (handle_map[i] == 0) {
			handle_map[i] = 1;
			break;
		}
	}
	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_handle_map_lock);

	if (i < EVENT_QUEUE_MAX) {
		ret_handle = i+1;
		EM_DEBUG_LOG("New handle [%d]", ret_handle);
	}
	else {
		ret_handle = 0;
		EM_DEBUG_EXCEPTION ("there is no available handle");
	}

	EM_DEBUG_FUNC_END();

	return ret_handle;
}

static int emcore_get_new_send_handle(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int i = 0;
	int ret_handle = 0;

	ENTER_RECURSIVE_CRITICAL_SECTION(_send_event_handle_map_lock);
	for (i = 0; i < EVENT_QUEUE_MAX; i++) {
		if (send_handle_map[i] == 0) {
			send_handle_map[i] = 1;
			break;
		}
	}
	LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_handle_map_lock);

	if (i < EVENT_QUEUE_MAX) {
		ret_handle = i+1;
		EM_DEBUG_LOG("New send handle [%d]", ret_handle);
	}
	else {
		ret_handle = 0;
		EM_DEBUG_LOG("there is no available send handle");
	}

	EM_DEBUG_FUNC_END();

	return ret_handle;
}

INTERNAL_FUNC int emcore_return_handle(int handle)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = false;

	ENTER_RECURSIVE_CRITICAL_SECTION(_event_handle_map_lock);
	if (handle > 0 && handle <= EVENT_QUEUE_MAX) {
		handle_map[handle-1] = 0;
		ret = true;
		EM_DEBUG_LOG("handle [%d] is returned", handle);
	}
	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_handle_map_lock);

	EM_DEBUG_FUNC_END();

	return ret;
}

INTERNAL_FUNC int emcore_return_send_handle(int handle)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = false;

	ENTER_RECURSIVE_CRITICAL_SECTION(_send_event_handle_map_lock);
	if (handle > 0 && handle <= EVENT_QUEUE_MAX) {
		send_handle_map[handle-1] = 0;
		ret = true;
		EM_DEBUG_LOG("send handle [%d] is returned", handle);
	}
	LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_handle_map_lock);

	EM_DEBUG_FUNC_END();

	return ret;
}

#ifdef __FEATURE_KEEP_CONNECTION__
INTERNAL_FUNC unsigned int emcore_get_receiving_thd_id()
{
	return (unsigned int)g_srv_thread;
}
#endif /*  __FEATURE_KEEP_CONNECTION__ */

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

INTERNAL_FUNC int emcore_get_pbd_thd_state()
{
	int pbd_thd_state = false;
	ENTER_CRITICAL_SECTION(_state_variables_lock);
	pbd_thd_state = g_pbd_thd_state;
	LEAVE_CRITICAL_SECTION(_state_variables_lock);
	return pbd_thd_state;
}

INTERNAL_FUNC int emcore_set_pbd_thd_state(int flag)
{
	ENTER_CRITICAL_SECTION(_state_variables_lock);
	g_pbd_thd_state = flag;
	LEAVE_CRITICAL_SECTION(_state_variables_lock);

	return g_pbd_thd_state;
}

INTERNAL_FUNC unsigned int emcore_get_partial_body_thd_id()
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_FUNC_END();
	return (unsigned int)g_partial_body_thd;
}

static int emcore_clear_bulk_pbd_que(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = true;
	int error = EMAIL_ERROR_NONE;
	int i = 0;

	for (i = 0; i < BULK_PARTIAL_BODY_DOWNLOAD_COUNT; ++i) {
		if (g_partial_body_bulk_dwd_que[i].event_type) {
			if (false == emcore_free_partial_body_thd_event(g_partial_body_bulk_dwd_que + i, &error))					 {
				EM_DEBUG_EXCEPTION("emcore_free_partial_body_thd_event_cell failed [%d]", error);
				ret = false;
				break;
			}
		}
	}

	if (NULL != err_code)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC void emcore_pb_thd_set_local_activity_continue(int flag)
{
	EM_DEBUG_FUNC_BEGIN("flag [%d]", flag);

	ENTER_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);

	g_pb_thd_local_activity_continue = flag;

	if (true == flag) {
		WAKE_CONDITION_VARIABLE(_partial_body_thd_cond);
	}

	LEAVE_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int emcore_pb_thd_can_local_activity_continue()
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;

	ENTER_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);

	ret = g_pb_thd_local_activity_continue;

	LEAVE_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);
	EM_DEBUG_FUNC_END();
	return ret;

}

INTERNAL_FUNC int emcore_clear_partial_body_thd_event_que(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = true;
	int error = EMAIL_ERROR_NONE;
	int i = 0;

	ENTER_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);

	if (true == g_partial_body_thd_queue_empty) {
		EM_DEBUG_LOG(" Partial Body Thread Event Queue Already empty ");
	}
	else {
		for (i = 0; i < TOTAL_PARTIAL_BODY_EVENTS; ++i) {
			if (g_partial_body_thd_event_que[i].event_type) {
				if (false == emcore_free_partial_body_thd_event(g_partial_body_thd_event_que + i, &error))					 {
					EM_DEBUG_EXCEPTION("emcore_free_partial_body_thd_event_cell failed [%d]", error);
					ret = false;
					break;
				}
			}
		}

		g_partial_body_thd_queue_empty = true;
		g_partial_body_thd_queue_full = false;
	}
	LEAVE_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);

	if (NULL != err_code)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_is_partial_body_thd_que_empty()
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;

	ENTER_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);

	ret = g_partial_body_thd_queue_empty;

	LEAVE_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
INTERNAL_FUNC int emcore_is_partial_body_thd_que_full()
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;

	ENTER_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);

	ret = g_partial_body_thd_queue_full;

	LEAVE_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);
	EM_DEBUG_FUNC_END();
	return ret;
}

/*
Himanshu[h.gahalut] :  If either src pointer or dest pointer points to a cell of global partial body thread event_data queue,
then emcore_copy_partial_body_thd_event API should only be called from a portion of code which is protected
through _partial_body_thd_event_queue_lock mutex.

No mutex is used inside this API so that we can also use it to copy partial body events which are not a part of global event_data queue

Precautions :

_partial_body_thd_event_queue_lock mutex should never be used inside this API otherwise it will be a deadlock.
Also never call any function from this API which uses _partial_body_thd_event_queue_lock mutex.

*/

static int emcore_copy_partial_body_thd_event(email_event_partial_body_thd *src, email_event_partial_body_thd *dest, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int error = EMAIL_ERROR_NONE;
	int ret = false;

	if (NULL == src || NULL == dest) {
		EM_DEBUG_LOG(" Invalid Parameter src [%p] dest [%p]", src, dest);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	dest->account_id = src->account_id;
	dest->mail_id = src->mail_id;
	dest->server_mail_id = src->server_mail_id;
	dest->activity_id = src->activity_id;
	dest->mailbox_id = src->mailbox_id;
	dest->mailbox_name = EM_SAFE_STRDUP(src->mailbox_name);
	dest->activity_type = src->activity_type;
	dest->event_type = src->event_type;
	dest->multi_user_name = EM_SAFE_STRDUP(src->multi_user_name);
	

	ret = true;

	FINISH_OFF:

	if (NULL != error_code)
		*error_code = error;

	return ret;

}

/*
Himanshu[h.gahalut] :  If emcore_free_partial_body_thd_event_cell API is used to free a cell of partial body thread event_data queue,
it should only be called from a portion of code which is protected through _partial_body_thd_event_queue_lock mutex.

No mutex is used inside this API so that we can also use it to free partial body events which are not a part of global event_data queue

Precautions :

_partial_body_thd_event_queue_lock mutex should never be used inside this API otherwise it will be a deadlock.
Also never call any function from this API which uses _partial_body_thd_event_queue_lock mutex.

*/

INTERNAL_FUNC int emcore_free_partial_body_thd_event(email_event_partial_body_thd *partial_body_thd_event, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN();

	if (NULL == partial_body_thd_event) {
		*error_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	email_event_partial_body_thd *pbd_event = partial_body_thd_event;

	/*Free character pointers in event_data cell */
	EM_SAFE_FREE(pbd_event->mailbox_name);
	EM_SAFE_FREE(pbd_event->multi_user_name);
	memset(pbd_event, 0x00, sizeof(email_event_partial_body_thd));
	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emcore_insert_partial_body_thread_event (
                              email_event_partial_body_thd *partial_body_thd_event, 
                              int                          *error_code)
{
	EM_DEBUG_FUNC_BEGIN();

	if (NULL == partial_body_thd_event)  {
		EM_DEBUG_EXCEPTION("\t partial_body_thd_event [%p] ", partial_body_thd_event);

		if (error_code != NULL) {
			*error_code = EMAIL_ERROR_INVALID_PARAM;
		}
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int empty_cell_index = -1;
	int index = 0;
	int count = 0;

	ENTER_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);

	/* find a cell in queue which is empty */
	index = g_partial_body_thd_next_event_idx;
	for (count = 0; count < TOTAL_PARTIAL_BODY_EVENTS; count++) {
		/*Found empty Cell*/
		if (g_partial_body_thd_event_que[index].event_type == EMAIL_EVENT_NONE) {		
			empty_cell_index = index;
			break;
		}
		index++;
		index = index % TOTAL_PARTIAL_BODY_EVENTS;
	}

	if (empty_cell_index != -1) {
		emcore_copy_partial_body_thd_event (partial_body_thd_event, 
                                 g_partial_body_thd_event_que + empty_cell_index, 
                                 &error);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_LOG("emcore_copy_partial_body_thd_event failed [%d]", error);
		}
		else {
			g_partial_body_thd_queue_empty = false;
			if (count == (TOTAL_PARTIAL_BODY_EVENTS - 1)) {
				/*This is the last event_data inserted in queue after its insertion, queue is full */
				g_partial_body_thd_queue_full = true;
			}
			WAKE_CONDITION_VARIABLE(_partial_body_thd_cond);
			ret = true;
		}
	}
	else {
		EM_DEBUG_LOG("partial body thread event_data queue is full ");
		error = EMAIL_ERROR_EVENT_QUEUE_FULL;

		g_partial_body_thd_queue_full = true;
		g_partial_body_thd_queue_empty = false;
	}

	LEAVE_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);

	if (NULL != error_code) {
		*error_code = error;
	}

	return ret;

}

/* h.gahlaut :  Return true only if event_data is retrieved successfully */

INTERNAL_FUNC int emcore_retrieve_partial_body_thread_event(email_event_partial_body_thd *partial_body_thd_event, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int index = 0;

	/* Lock Mutex to protect event_data queue and associated global variables variables*/

	ENTER_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);

	index = g_partial_body_thd_next_event_idx;

	if (0 == g_partial_body_thd_event_que[index].event_type) {
		error = EMAIL_ERROR_EVENT_QUEUE_EMPTY;
		g_partial_body_thd_queue_empty = true;
		g_partial_body_thd_queue_full = false;
	}
	else {
		/*Copy the event_data from queue to return it and free the event_data in queue */
		if (false == emcore_copy_partial_body_thd_event(g_partial_body_thd_event_que + index, partial_body_thd_event, &error))
			EM_DEBUG_EXCEPTION("emcore_copy_partial_body_thd_event failed [%d]", error);
		else {
			if (false == emcore_free_partial_body_thd_event(g_partial_body_thd_event_que + index, &error))
				EM_DEBUG_EXCEPTION("emcore_free_partial_body_thd_event_cell failed [%d]", error);
			else {
				g_partial_body_thd_queue_full = false;
				g_partial_body_thd_next_event_idx = ++index;

				if (g_partial_body_thd_next_event_idx == TOTAL_PARTIAL_BODY_EVENTS)
					g_partial_body_thd_next_event_idx = 0;

				/* If the event_data retrieved was the only event_data present in queue,
				we need to set g_partial_body_thd_queue_empty to true
				*/

				if (0 == g_partial_body_thd_event_que[g_partial_body_thd_next_event_idx].event_type) {
					g_partial_body_thd_queue_empty = true;
				}

				ret = true;
			}
		}
	}

	/* Unlock Mutex */

	LEAVE_CRITICAL_SECTION(_partial_body_thd_event_queue_lock);

	if (error_code)
		*error_code = error;

	return ret;

}

/*Function to flush the bulk partial body download queue [santosh.br@samsung.com]*/
static int emcore_partial_body_bulk_flush(char *multi_user_name, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int error = EMAIL_ERROR_NONE;
	int ret = false;
	MAILSTREAM *stream = NULL;
	void *tmp_stream = NULL;

	if (!emcore_connect_to_remote_mailbox(multi_user_name, g_partial_body_bulk_dwd_que[0].account_id, g_partial_body_bulk_dwd_que[0].mailbox_id, (void **)&tmp_stream, &error) || (NULL == tmp_stream)) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", error);
		goto FINISH_OFF;
	}
	stream = (MAILSTREAM *)tmp_stream;

	/*  Call bulk download here */
	if (false == emcore_download_bulk_partial_mail_body (stream, g_partial_body_bulk_dwd_que, g_partial_body_bulk_dwd_next_event_idx, &error)) {
		EM_DEBUG_EXCEPTION(" emcore_download_bulk_partial_mail_body failed.. [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;
FINISH_OFF:

	stream = mail_close (stream);

	g_partial_body_bulk_dwd_next_event_idx = 0;
	g_partial_body_bulk_dwd_queue_empty = true;

	if (false == emcore_clear_bulk_pbd_que(&error))
		EM_DEBUG_EXCEPTION("emcore_clear_bulk_pbd_que failed [%d]", error);

	if (NULL != error_code)
		*error_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


/* Function to pass UID list and Data for bulk partial body download [santosh.br@samsung.com]/[h.gahlaut@samsung.com] */
INTERNAL_FUNC int emcore_mail_partial_body_download (email_event_partial_body_thd *pbd_event, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int error = EMAIL_ERROR_NONE;
	int ret = false;
	int count = 0;
	int i = 0, m = 0;
	MAILSTREAM *stream = NULL;
	email_event_partial_body_thd *activity_data_list = NULL;
	int *mailbox_list = NULL;
	int *account_list = NULL;

	if (NULL == pbd_event) {
	    EM_DEBUG_EXCEPTION("Invalid Parameter pbd_event [%p] ", pbd_event);

	    error = EMAIL_ERROR_INVALID_PARAM;
	    goto FINISH_OFF;
	}

	/*Check if the event_data is to flush the event_data que array */
	if (EMAIL_EVENT_BULK_PARTIAL_BODY_DOWNLOAD == pbd_event->event_type) {
		EM_DEBUG_LOG("pbd_event->event_type is EMAIL_EVENT_BULK_PARTIAL_BODY_DOWNLOAD");
		/*Check if the mailbox name and account id for this event_data is same as the mailbox name and account id for earlier events saved in download que array
		then append this event_data also to download que array */
		if ((0 != g_partial_body_bulk_dwd_que[0].mailbox_id) && g_partial_body_bulk_dwd_que[0].mailbox_id == pbd_event->mailbox_id && \
			(g_partial_body_bulk_dwd_que[0].account_id == pbd_event->account_id)) {
			EM_DEBUG_LOG("Event is for the same mailbox and same account as the already present events in download que");
			EM_DEBUG_LOG("Check if the download que reached its limit. If yes then first flush the que.");
			if (g_partial_body_bulk_dwd_next_event_idx == BULK_PARTIAL_BODY_DOWNLOAD_COUNT) {
				if (false == emcore_partial_body_bulk_flush(pbd_event->multi_user_name, &error)) {
					EM_DEBUG_EXCEPTION("Partial Body thread emcore_partial_body_bulk_flush failed - %d", error);
					goto FINISH_OFF;
				}
			}
		}
		else  {
			EM_DEBUG_LOG("Event is not for the same mailbox and same account as the already present events in download que");
			EM_DEBUG_LOG("Flush the current que if not empty");
			EM_DEBUG_LOG("g_partial_body_bulk_dwd_queue_empty [%d]", g_partial_body_bulk_dwd_queue_empty);
			if (!g_partial_body_bulk_dwd_queue_empty) {
				if (false == emcore_partial_body_bulk_flush(pbd_event->multi_user_name, &error)) {
					EM_DEBUG_EXCEPTION("Partial Body thread emcore_partial_body_bulk_flush failed - %d", error);
					goto FINISH_OFF;
				}
			}
		}
		/*Add the event_data to the download que array */
		if (false == emcore_copy_partial_body_thd_event (pbd_event, 
                                 g_partial_body_bulk_dwd_que+(g_partial_body_bulk_dwd_next_event_idx), &error))
			EM_DEBUG_EXCEPTION("\t Partial Body thread emcore_copy_partial_body_thd_event failed - %d", error);
		else {
			g_partial_body_bulk_dwd_queue_empty = false;
			g_partial_body_bulk_dwd_next_event_idx++;
			EM_DEBUG_LOG("g_partial_body_bulk_dwd_next_event_idx [%d]", g_partial_body_bulk_dwd_next_event_idx);
		}
	}
	else if (pbd_event->activity_type) {

		int account_count = 0;

		EM_DEBUG_LOG("Event is coming from local activity.");
		/* Get all the accounts for which local activities are pending */
		if (false == emstorage_get_pbd_account_list(pbd_event->multi_user_name, &account_list, &account_count, false, &error)) {
				EM_DEBUG_EXCEPTION(" emstorage_get_mailbox_list failed.. [%d]", error);
				error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
				goto FINISH_OFF;
		}

		for (m = 0; m < account_count; ++m) {
			/* Get the mailbox list for the account to start bulk partial body fetch for mails in each mailbox of accounts one by one*/
			if (false == emstorage_get_pbd_mailbox_list(pbd_event->multi_user_name, account_list[m], &mailbox_list, &count, false, &error)) {
					EM_DEBUG_EXCEPTION(" emstorage_get_mailbox_list failed.. [%d]", error);
					error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
					goto FINISH_OFF;
			}

			for (i = 0; i < count; i++) {
				int k = 0;
				int activity_count = 0;

				if (!emcore_connect_to_remote_mailbox(pbd_event->multi_user_name, account_list[m], mailbox_list[i], (void **)&stream, &error)) {
					EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", error);
					stream = mail_close (stream);
					goto FINISH_OFF;
				}

				if (false == emstorage_get_pbd_activity_data(pbd_event->multi_user_name, account_list[m], mailbox_list[i], &activity_data_list, &activity_count,  false, &error))
					EM_DEBUG_EXCEPTION(" emstorage_get_pbd_activity_data failed.. [%d]", error);

				/* it is duplicated with emstorage_get_pbd_activity_data
				if (false == emstorage_get_mailbox_pbd_activity_count(pbd_evnet->multi_user_name, account_list[m], mailbox_list[i], &activity_count, false, &error)) {
					EM_DEBUG_EXCEPTION(" emstorage_get_mailbox_pbd_activity_count failed.. [%d]", error);
					continue;
				}
				*/
				if (activity_count>0 && activity_data_list) {
					int temp_error = EMAIL_ERROR_NONE;
					int j = 0;
					int iter = 0;
					int remainder = 0;
					int num = BULK_PARTIAL_BODY_DOWNLOAD_COUNT;
					int index = 0;

/*
					if (false == emstorage_get_pbd_activity_data(pbd_event->multi_user_name, account_list[j], mailbox_list[i], &activity_data_list, &num_activity,  false, &error))
						EM_DEBUG_EXCEPTION(" emstorage_get_pbd_activity_data failed.. [%d]", error);

					if (NULL == activity_data_list) {
						EM_DEBUG_EXCEPTION(" activity_data_list is null..");
						continue;
					}
*/
					remainder = activity_count%BULK_PARTIAL_BODY_DOWNLOAD_COUNT;
					iter = activity_count/BULK_PARTIAL_BODY_DOWNLOAD_COUNT + ((activity_count%BULK_PARTIAL_BODY_DOWNLOAD_COUNT) ? 1  :  0);

					for (j = 0; j < iter; j++) {
	 					if ((iter == (j+1)) && (remainder != 0))
							num = remainder;

						/*Call bulk download here */
						if (false == emcore_download_bulk_partial_mail_body(stream, activity_data_list+index, num, &error)) {
							EM_DEBUG_EXCEPTION(" emcore_download_bulk_partial_mail_body failed.. [%d]", error);
							temp_error = EMAIL_ERROR_NO_RESPONSE;
						}

						index += num;

						if (EMAIL_ERROR_NO_RESPONSE == temp_error) {
							temp_error = EMAIL_ERROR_NONE;
							break;
						}
					}

					for (k = 0; k < activity_count; k++)
						emcore_free_partial_body_thd_event(activity_data_list + k, &error);

					EM_SAFE_FREE(activity_data_list);

					/*check: empty check required?*/
					if (false == emcore_is_partial_body_thd_que_empty()) {
						ret = true;
						goto FINISH_OFF;		/* Stop Local Activity Sync */
					}
				}
				stream = mail_close (stream);
			}
		}

		/* After completing one cycle of local activity sync ,
		local activity continue variable should be set to false. */

		emcore_pb_thd_set_local_activity_continue(false);
	}
	else /* Event-type is 0 which means Event is to flush the que when no more events are present in g_partial_body_thd_event_que */ {
		/*Check if events have arrived in g_partial_body_thd_event_que */
		if (false == emcore_is_partial_body_thd_que_empty()) {
			EM_DEBUG_LOG("emcore_is_partial_body_thd_que_empty retured true");
			ret = true;
			goto FINISH_OFF;		/* Stop Local Activity Sync */
		}

		if (false == emcore_partial_body_bulk_flush(pbd_event->multi_user_name, &error)) {
			EM_DEBUG_EXCEPTION("\t Partial Body thread emcore_partial_body_bulk_flush failed - %d", error);
			goto FINISH_OFF;
		}
	}

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(mailbox_list);
	EM_SAFE_FREE(account_list);
	stream = mail_close (stream);

	if (error_code)
		*error_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

#endif /*  __FEATURE_PARTIAL_BODY_DOWNLOAD__ */
