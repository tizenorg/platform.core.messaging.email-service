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
 * File :  email-core-event_data.h
 * Desc :  Mail Engine Event
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
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


#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

/*[h.gahlaut] - All static variable declaration for partial body download thread are done here */

#define TOTAL_PARTIAL_BODY_EVENTS 100

static email_event_partial_body_thd g_partial_body_thd_event_que[TOTAL_PARTIAL_BODY_EVENTS];

static int g_partial_body_thd_next_event_idx = 0;			/* Index of Next Event to be processed in the queue*/
static int g_partial_body_thd_loop = 1;						/* Variable to make a continuos while loop */
static int g_partial_body_thd_queue_empty = true;			/* Variable to determine if event queue is empty.True means empty*/
static int g_partial_body_thd_queue_full = false;			/* Variable to determine if event queue is full. True means full*/	
static int g_pb_thd_local_activity_continue = true;			/* Variable to control local activity sync */
int g_pbd_thd_state = false;								/* false :  thread is sleeping , true :  thread is working */

static pthread_mutex_t _partial_body_thd_event_queue_lock = PTHREAD_MUTEX_INITIALIZER;	/* Mutex to protect event queue */
static pthread_cond_t  _partial_body_thd_cond = PTHREAD_COND_INITIALIZER;				/* Condition variable on which partial body thread is waiting  */
thread_t g_partial_body_thd ;								/* Determines if thread is created or not. Non Null means thread is created */

email_event_partial_body_thd g_partial_body_bulk_dwd_que[BULK_PARTIAL_BODY_DOWNLOAD_COUNT];
static int g_partial_body_bulk_dwd_next_event_idx = 0;		/* Index of Next Event to be processed in the queue*/
static int g_partial_body_bulk_dwd_queue_empty = true;

static pthread_mutex_t _state_variables_lock;

/*[h.gahlaut] - All static function declaration for partial body download thread are done here */

static int emcore_retrieve_partial_body_thread_event(email_event_partial_body_thd *partial_body_thd_event, int *error_code);
static int emcore_copy_partial_body_thd_event(email_event_partial_body_thd *src, email_event_partial_body_thd *dest, int *error_code);
static void emcore_pb_thd_set_local_activity_continue(int flag);
static int emcore_set_pbd_thd_state(int flag);
static int emcore_clear_bulk_pbd_que(int *err_code);
int emcore_mail_partial_body_download(email_event_partial_body_thd *pbd_event, int *error_code);

#endif

#ifdef ENABLE_IMAP_IDLE_THREAD
extern int g_imap_idle_thread_alive;
extern int imap_idle_thread;
#endif /* ENABLE_IMAP_IDLE_THREAD */
INTERNAL_FUNC int g_client_count = 0;
INTERNAL_FUNC int g_client_run = 0 ;

#ifdef __FEATURE_LOCAL_ACTIVITY__
INTERNAL_FUNC int g_local_activity_run = 0;
INTERNAL_FUNC int g_save_local_activity_run = 0;
#endif


#define EVENT_QUEUE_MAX 32

typedef struct EVENT_CALLBACK_ELEM 
{
	email_event_callback callback;
	void *event_data;
	struct EVENT_CALLBACK_ELEM *next;
} EVENT_CALLBACK_LIST;

static pthread_mutex_t _event_available_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  _event_available_signal = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t *_event_callback_table_lock = NULL;
static pthread_mutex_t *_event_queue_lock = NULL;		
static EVENT_CALLBACK_LIST *_event_callback_table[EMAIL_ACTION_NUM];		/*  array of singly-linked list for event callbacks */

void *thread_func_branch_command(void *arg);

static email_event_t g_event_que[EVENT_QUEUE_MAX];

int send_thread_run = 0;
int recv_thread_run = 0;

static pthread_mutex_t _send_event_available_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t *_send_event_queue_lock = NULL;  /*  MUST BE "recursive" */
static pthread_cond_t  _send_event_available_signal = PTHREAD_COND_INITIALIZER;
static thread_t g_send_srv_thread;
static thread_t g_srv_thread;

static email_event_t g_send_event_que[EVENT_QUEUE_MAX];
static int g_send_event_que_idx = 1;
static int g_send_event_loop = 1;
static int g_send_active_que = 0;
static int g_event_que_idx = 1;
static int g_event_loop = 1;
static int g_active_que = 0;
static int g_sending_busy_cnt = 0;
static int g_receiving_busy_cnt = 0;


INTERNAL_FUNC int emcore_get_current_thread_type()
{
	EM_DEBUG_FUNC_BEGIN();
	thread_t thread_id = THREAD_SELF();
	int thread_type = -1;

	if (thread_id == g_srv_thread)
		thread_type = _SERVICE_THREAD_TYPE_RECEIVING;
	else if (thread_id == g_send_srv_thread)
		thread_type = _SERVICE_THREAD_TYPE_SENDING;
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
	else if (thread_id == g_partial_body_thd)
		thread_type = _SERVICE_THREAD_TYPE_PBD;
#endif 
	EM_DEBUG_FUNC_END("thread_type [%d]", thread_type);
	return thread_type;	
}

static int is_gdk_lock_needed()
{
	if (g_event_loop)  {		
		return (THREAD_SELF() == g_srv_thread);
	}
	return false;
}

INTERNAL_FUNC int emcore_get_pending_event(email_action_t action, int account_id, int mail_id, email_event_status_type_t *status)
{
	EM_DEBUG_FUNC_BEGIN("action[%d], account_id[%d], mail_id[%d]", action, account_id, mail_id);
	
	int found = false;
	int i;
	
	ENTER_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
	
	for (i = 1; i < EVENT_QUEUE_MAX; i++)  {
		switch (g_event_que[i].type)  {
			case EMAIL_EVENT_SEND_MAIL: 
			case EMAIL_EVENT_SEND_MAIL_SAVED: 
				if (action == EMAIL_ACTION_SEND_MAIL && account_id == g_event_que[i].account_id && mail_id == g_event_que[i].event_param_data_4) {
					found = true;
					goto EXIT;
				}
				break;
			
			case EMAIL_EVENT_SYNC_HEADER: 
				if (action == EMAIL_ACTION_SYNC_HEADER && account_id == g_event_que[i].account_id) {
					found = true;
					goto EXIT;
				}
				break;
			
			case EMAIL_EVENT_SYNC_HEADER_OMA:
				if (action == EMAIL_ACTION_SYNC_HEADER_OMA && account_id == g_event_que[i].account_id) {
					found = true;
					goto EXIT;
				}
				break;
			
			case EMAIL_EVENT_DOWNLOAD_BODY: 
				if (action == EMAIL_ACTION_DOWNLOAD_BODY && account_id == g_event_que[i].account_id && mail_id == g_event_que[i].event_param_data_4) {
					found = true;
					goto EXIT;
				}
				break;
			case EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER:
				if (action == EMAIL_ACTION_SYNC_MAIL_FLAG_TO_SERVER && account_id == g_event_que[i].account_id && mail_id == g_event_que[i].event_param_data_4) {
					found = true;
					goto EXIT;
				}				
				break;
			case EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER:
				if (action == EMAIL_ACTION_SYNC_FLAGS_FIELD_TO_SERVER && account_id == g_event_que[i].account_id && mail_id == g_event_que[i].event_param_data_4) {
					found = true;
					goto EXIT;
				}				
				break;
			case EMAIL_EVENT_DOWNLOAD_ATTACHMENT: 
				if (action == EMAIL_ACTION_DOWNLOAD_ATTACHMENT && account_id == g_event_que[i].account_id && mail_id == g_event_que[i].event_param_data_4) {
					found = true;
					goto EXIT;
				}
				break;
			case EMAIL_EVENT_DELETE_MAIL: 
			case EMAIL_EVENT_DELETE_MAIL_ALL:
				if (action == EMAIL_ACTION_DELETE_MAIL && account_id == g_event_que[i].account_id) {
					found = true;
					goto EXIT;
				}
				break;

			case EMAIL_EVENT_CREATE_MAILBOX: 
				if (action == EMAIL_ACTION_CREATE_MAILBOX && account_id == g_event_que[i].account_id) {
					found = true;
					goto EXIT;
				}
				break;

			case EMAIL_EVENT_DELETE_MAILBOX: 
				if (action == EMAIL_ACTION_DELETE_MAILBOX && account_id == g_event_que[i].account_id) {
					found = true;
					goto EXIT;
				}
				break;

			case EMAIL_EVENT_MOVE_MAIL: 
				if (action == EMAIL_ACTION_MOVE_MAIL && account_id == g_event_que[i].account_id && mail_id == g_event_que[i].event_param_data_4) {
					found = true;
					goto EXIT;
				}
				break;

			case EMAIL_EVENT_VALIDATE_ACCOUNT: 
				if (action == EMAIL_ACTION_VALIDATE_ACCOUNT && account_id == g_event_que[i].account_id) {
					found = true;
					goto EXIT;
				}
				break;

			case EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT: 
				if (action == EMAIL_ACTION_VALIDATE_AND_UPDATE_ACCOUNT && account_id == 0) {
					found = true;
					goto EXIT;
				}
				break;
			
			case EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT: 
				if (action == EMAIL_ACTION_VALIDATE_AND_CREATE_ACCOUNT && account_id == 0) {
					found = true;
					goto EXIT;
				}
				break;
			
			case EMAIL_EVENT_UPDATE_MAIL:
				if (action == EMAIL_ACTION_UPDATE_MAIL)  {
					found = true;
					goto EXIT;
				}
				break;
				
			case EMAIL_EVENT_SET_MAIL_SLOT_SIZE: 
				if (action == EMAIL_ACTION_SET_MAIL_SLOT_SIZE)  {
					found = true;
					goto EXIT;
				}
				break;

			case EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED:
				if (action == EMAIL_ACTION_EXPUNGE_MAILS_DELETED_FLAGGED)  {
					found = true;
					goto EXIT;
				}
				break;
				
			case EMAIL_EVENT_SEARCH_ON_SERVER:
				if (action == EMAIL_ACTION_SEARCH_ON_SERVER && account_id == g_event_que[i].account_id) {
					found = true;
					goto EXIT;
				}
				break;	

			case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
				if (action == EMAIL_ACTION_MOVE_MAILBOX && account_id == g_event_que[i].account_id) {
					found = true;
					goto EXIT;
				}
				break;

			default: 
				break;
		}
	}
	
EXIT:
	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
	
	if (found) {
		if (status)
			*status = g_event_que[i].status;
		
		return i;
	}
	
	return FAILURE;
}

INTERNAL_FUNC void emcore_get_event_queue_status(int *on_sending, int *on_receiving)
{
	if (on_sending != NULL)
		*on_sending = g_sending_busy_cnt;
	
	if (on_receiving != NULL)
		*on_receiving = g_receiving_busy_cnt;
}

static void _sending_busy_ref(void)
{
	g_sending_busy_cnt++;
}

static void _sending_busy_unref(void)
{
	g_sending_busy_cnt--;
}

static void _receiving_busy_ref(void)
{
	g_receiving_busy_cnt++;
}

static void _receiving_busy_unref(void)
{
	g_receiving_busy_cnt--;
}

static void waiting_status_notify(email_event_t *event_data, int queue_idx)
{
	EM_DEBUG_FUNC_BEGIN("event_data[%p], queue_idx[%d]", event_data, queue_idx);
	
	int account_id = event_data->account_id;
	int mail_id = event_data->event_param_data_4;		/*  NOT ALWAYS */
			
	switch (event_data->type)  {
		case EMAIL_EVENT_SEND_MAIL: 
			emcore_execute_event_callback(EMAIL_ACTION_SEND_MAIL, 0, 0, EMAIL_SEND_WAITING, account_id, mail_id, queue_idx, EMAIL_ERROR_NONE);
			break;
		
		case EMAIL_EVENT_SYNC_HEADER: 
			emcore_execute_event_callback(EMAIL_ACTION_SYNC_HEADER, 0, 0, EMAIL_LIST_WAITING, account_id, 0, queue_idx, EMAIL_ERROR_NONE);
			break;

		case EMAIL_EVENT_SYNC_HEADER_OMA:
			emcore_execute_event_callback(EMAIL_ACTION_SYNC_HEADER_OMA, 0, 0, EMAIL_LIST_WAITING, account_id, 0, queue_idx, EMAIL_ERROR_NONE);
			break;
		
		case EMAIL_EVENT_DOWNLOAD_BODY: 
			emcore_execute_event_callback(EMAIL_ACTION_DOWNLOAD_BODY, 0, 0, EMAIL_DOWNLOAD_WAITING, account_id, mail_id, queue_idx, EMAIL_ERROR_NONE);
			break;

#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
		case EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER:
			emcore_execute_event_callback(EMAIL_ACTION_SYNC_MAIL_FLAG_TO_SERVER, 0, 0, EMAIL_SYNC_WAITING, account_id, mail_id, queue_idx, EMAIL_ERROR_NONE);
			break;         
#endif
			
		case EMAIL_EVENT_DOWNLOAD_ATTACHMENT: 
			emcore_execute_event_callback(EMAIL_ACTION_DOWNLOAD_ATTACHMENT, 0, 0, EMAIL_DOWNLOAD_WAITING, account_id, mail_id, queue_idx, EMAIL_ERROR_NONE);
			break;

		case EMAIL_EVENT_DELETE_MAIL: 
		case EMAIL_EVENT_DELETE_MAIL_ALL:
			emcore_execute_event_callback(EMAIL_ACTION_DELETE_MAIL, 0, 0, EMAIL_DELETE_WAITING, account_id, 0, queue_idx, EMAIL_ERROR_NONE);
			break;

		case EMAIL_EVENT_VALIDATE_ACCOUNT: 
			emcore_execute_event_callback(EMAIL_ACTION_VALIDATE_ACCOUNT, 0, 0, EMAIL_VALIDATE_ACCOUNT_WAITING, account_id, 0, queue_idx, EMAIL_ERROR_NONE);
			break;

		case EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT: 
			emcore_execute_event_callback(EMAIL_ACTION_VALIDATE_AND_CREATE_ACCOUNT, 0, 0, EMAIL_VALIDATE_ACCOUNT_WAITING, account_id, 0, queue_idx, EMAIL_ERROR_NONE);
			break;

		case EMAIL_EVENT_MOVE_MAIL: 
			emcore_execute_event_callback(EMAIL_ACTION_MOVE_MAIL, 0, 0, EMAIL_LIST_WAITING, account_id, 0, queue_idx, EMAIL_ERROR_NONE);
			break;

		case EMAIL_EVENT_CREATE_MAILBOX: 
			emcore_execute_event_callback(EMAIL_ACTION_CREATE_MAILBOX, 0, 0, EMAIL_LIST_WAITING, account_id, 0, queue_idx, EMAIL_ERROR_NONE);
			break;

		case EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT: 
			emcore_execute_event_callback(EMAIL_ACTION_VALIDATE_AND_UPDATE_ACCOUNT, 0, 0, EMAIL_VALIDATE_ACCOUNT_WAITING, account_id, 0, queue_idx, EMAIL_ERROR_NONE);
			break;

		case EMAIL_EVENT_UPDATE_MAIL:
			break;

		case EMAIL_EVENT_SET_MAIL_SLOT_SIZE: 
			emcore_execute_event_callback(EMAIL_ACTION_SET_MAIL_SLOT_SIZE, 0, 0, EMAIL_SET_SLOT_SIZE_WAITING, account_id, 0, queue_idx, EMAIL_ERROR_NONE);
			break;

		case EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED:
			emcore_execute_event_callback(EMAIL_ACTION_EXPUNGE_MAILS_DELETED_FLAGGED, 0, 0, EMAIL_EXPUNGE_MAILS_DELETED_FLAGGED_WAITING, account_id, event_data->event_param_data_4, queue_idx, EMAIL_ERROR_NONE);
			break;

		case EMAIL_EVENT_SEARCH_ON_SERVER:
			emcore_execute_event_callback(EMAIL_ACTION_SEARCH_ON_SERVER, 0, 0, EMAIL_SYNC_WAITING, account_id, 0, queue_idx, EMAIL_ERROR_NONE);
			break;

		case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
			/* emcore_execute_event_callback(EMAIL_ACTION_CREATE_MAILBOX, 0, 0, EMAIL_LIST_WAITING, account_id, 0, queue_idx, EMAIL_ERROR_NONE); */
			break;

		default: 
			break;
	}
	EM_DEBUG_FUNC_END();
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
			emcore_show_user_message(mail_id, EMAIL_ACTION_SEND_MAIL, error);
			break;
		
		case EMAIL_EVENT_SYNC_HEADER: 
			emcore_execute_event_callback(EMAIL_ACTION_SYNC_HEADER, 0, 0, EMAIL_LIST_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(account_id, EMAIL_ACTION_SYNC_HEADER, error);
			break;
		
		case EMAIL_EVENT_DOWNLOAD_BODY: 
			emcore_execute_event_callback(EMAIL_ACTION_DOWNLOAD_BODY, 0, 0, EMAIL_DOWNLOAD_FAIL, account_id, mail_id, -1, error);
			emcore_show_user_message(account_id, EMAIL_ACTION_DOWNLOAD_BODY, error);
			break;
		
		case EMAIL_EVENT_DOWNLOAD_ATTACHMENT: 
			emcore_execute_event_callback(EMAIL_ACTION_DOWNLOAD_ATTACHMENT, 0, 0, EMAIL_DOWNLOAD_FAIL, account_id, mail_id, -1, error);
			emcore_show_user_message(account_id, EMAIL_ACTION_DOWNLOAD_ATTACHMENT, error);
			break;
		
		case EMAIL_EVENT_DELETE_MAIL: 
		case EMAIL_EVENT_DELETE_MAIL_ALL:
			emcore_execute_event_callback(EMAIL_ACTION_DELETE_MAIL, 0, 0, EMAIL_DELETE_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(account_id, EMAIL_ACTION_DELETE_MAIL, error);
			break;

		case EMAIL_EVENT_VALIDATE_ACCOUNT: 
			emcore_execute_event_callback(EMAIL_ACTION_VALIDATE_ACCOUNT, 0, 0, EMAIL_VALIDATE_ACCOUNT_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(account_id, EMAIL_ACTION_VALIDATE_ACCOUNT, error);
			break;

		case EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT: 
			emcore_execute_event_callback(EMAIL_ACTION_VALIDATE_AND_CREATE_ACCOUNT, 0, 0, EMAIL_VALIDATE_ACCOUNT_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(account_id, EMAIL_ACTION_VALIDATE_AND_CREATE_ACCOUNT, error);
			break;

		case EMAIL_EVENT_CREATE_MAILBOX: 
			emcore_execute_event_callback(EMAIL_ACTION_CREATE_MAILBOX, 0, 0, EMAIL_LIST_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(account_id, EMAIL_ACTION_CREATE_MAILBOX, error);
			break;

		case EMAIL_EVENT_DELETE_MAILBOX: 
			emcore_execute_event_callback(EMAIL_ACTION_DELETE_MAILBOX, 0, 0, EMAIL_LIST_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(account_id, EMAIL_ACTION_DELETE_MAILBOX, error);
			break;

		case EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT: 
			emcore_execute_event_callback(EMAIL_ACTION_VALIDATE_AND_UPDATE_ACCOUNT, 0, 0, EMAIL_VALIDATE_ACCOUNT_FAIL, account_id, 0, -1, error);
			emcore_show_user_message(account_id, EMAIL_ACTION_VALIDATE_AND_UPDATE_ACCOUNT, error);
			break;

		case EMAIL_EVENT_SET_MAIL_SLOT_SIZE: 
			emcore_execute_event_callback(EMAIL_ACTION_SET_MAIL_SLOT_SIZE, 0, 0, EMAIL_SET_SLOT_SIZE_FAIL, account_id, 0, -1, EMAIL_ERROR_NONE);
			break;

		case EMAIL_EVENT_SEARCH_ON_SERVER:
			emcore_execute_event_callback(EMAIL_ACTION_SEARCH_ON_SERVER, 0, 0, EMAIL_SEARCH_ON_SERVER_FAIL, account_id, 0, -1, EMAIL_ERROR_NONE);
			emcore_show_user_message(account_id, EMAIL_ACTION_SEARCH_ON_SERVER, error);
			break;

		case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
			emcore_execute_event_callback(EMAIL_ACTION_MOVE_MAILBOX, 0, 0, EMAIL_MOVE_MAILBOX_ON_IMAP_SERVER_FAIL, account_id, 0, -1, EMAIL_ERROR_NONE);
			emcore_show_user_message(account_id, EMAIL_ACTION_SEARCH_ON_SERVER, error);
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


static void emcore_initialize_event_callback_table()
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
			
			free(node);
			
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
			node->callback(total, done, status, account_id, mail_id, (handle == -1 ? emcore_get_active_queue_idx()  :  handle), node->event_data, error);
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
	
	if (!event_data)  {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	if (!g_srv_thread)  {
		EM_DEBUG_EXCEPTION("email-service is not ready");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_LOAD_ENGINE_FAILURE;
		return false;
	}
	
	int ret = true;
	int error = EMAIL_ERROR_NONE;
	
	ENTER_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
	
	if (!g_event_que[g_event_que_idx].type)  {	/*  if current buffer has not event, insert event data to current buffer  */
		EM_DEBUG_LOG("Current buffer has not a event. [%d]", g_event_que_idx);
		memcpy(g_event_que+g_event_que_idx, event_data, sizeof(email_event_t));
		g_event_que[g_event_que_idx].status = EMAIL_EVENT_STATUS_WAIT;
		waiting_status_notify(event_data, g_event_que_idx);
		if (handle) 
			*handle = g_event_que_idx;
	}
	else  {	/*  if current buffer has event, find the empty buffer */
		EM_DEBUG_LOG("Current buffer has a event. [%d]", g_event_que_idx);
		int i, j = g_event_que_idx + 1;	
	
		for (i = 1; i < EVENT_QUEUE_MAX; i++, j++)  {
			if (j >= EVENT_QUEUE_MAX) 
				j = 1;

			if (!g_event_que[j].type) 
					break;
		} 

		if (i < EVENT_QUEUE_MAX)  {
			EM_DEBUG_LOG("I found available buffer. [%d]", g_event_que + j);		
			memcpy(g_event_que+j, event_data, sizeof(email_event_t));
			g_event_que[j].status = EMAIL_EVENT_STATUS_WAIT;
			waiting_status_notify(event_data, j);
			
			if (handle) 
				*handle = j;
		}
		else  {
			EM_DEBUG_EXCEPTION("event que is full...");
			error = EMAIL_ERROR_EVENT_QUEUE_FULL;
			ret = false;
		}
	}
	
	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
	
	if (ret == true)  {
		event_data->event_param_data_1 = NULL;		/*  MUST BE - to prevent double-free */
		
		switch (event_data->type)  {
			case EMAIL_EVENT_SEND_MAIL: 
			case EMAIL_EVENT_SEND_MAIL_SAVED: 
				_sending_busy_ref();
				break;
			
			case EMAIL_EVENT_SYNC_HEADER: 
			case EMAIL_EVENT_DOWNLOAD_BODY: 
			case EMAIL_EVENT_DOWNLOAD_ATTACHMENT: 
			case EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER:
			case EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER:
			case EMAIL_EVENT_ISSUE_IDLE:
			case EMAIL_EVENT_SYNC_IMAP_MAILBOX: 
			case EMAIL_EVENT_VALIDATE_ACCOUNT: 
			case EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT:
			case EMAIL_EVENT_SAVE_MAIL:
			case EMAIL_EVENT_MOVE_MAIL: 
			case EMAIL_EVENT_DELETE_MAIL: 
			case EMAIL_EVENT_DELETE_MAIL_ALL:
			case EMAIL_EVENT_SYNC_HEADER_OMA:
			case EMAIL_EVENT_CREATE_MAILBOX: 	
			case EMAIL_EVENT_DELETE_MAILBOX: 	
			case EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT: 
			case EMAIL_EVENT_SET_MAIL_SLOT_SIZE: 
			case EMAIL_EVENT_UPDATE_MAIL:
			case EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED:
			case EMAIL_EVENT_SEARCH_ON_SERVER:
			case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
				_receiving_busy_ref();
				break;
			default: 
				break;
		}
		
		ENTER_CRITICAL_SECTION(_event_available_lock);
		WAKE_CONDITION_VARIABLE(_event_available_signal);
		LEAVE_CRITICAL_SECTION(_event_available_lock);
	}
	
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__ 
	{
		int is_local_activity_event_inserted = false;

		if (false == emcore_partial_body_thd_local_activity_sync(&is_local_activity_event_inserted, &error))
			EM_DEBUG_EXCEPTION("emcore_partial_body_thd_local_activity_sync failed [%d]", error);
		else {
			if (true == is_local_activity_event_inserted)
				emcore_pb_thd_set_local_activity_continue(false);
		}
	}
#endif
	
	if (err_code != NULL)
		*err_code = error;
	
	EM_DEBUG_LOG("Finish with [%d]", ret);
	return ret;
}

/* get a event from event_data queue */
static int emcore_retrieve_event(email_event_t *event_data, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("event_data[%p], err_code[%p]", event_data, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	ENTER_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
	
	/*  get a event_data if this queue is not empty */
	if (g_event_que[g_event_que_idx].type)  {
		memcpy(event_data, g_event_que+g_event_que_idx, sizeof(email_event_t));
		
		if (event_data->status != EMAIL_EVENT_STATUS_WAIT)  {	/*  EMAIL_EVENT_STATUS_CANCELED */
			memset(g_event_que+g_event_que_idx, 0x00, sizeof(email_event_t));
			g_active_que = 0;
		}
		else  {
			EM_DEBUG_LINE;
			g_event_que[g_event_que_idx].status = EMAIL_EVENT_STATUS_STARTED;
			g_active_que = g_event_que_idx;
			ret = true;
		}
		
		if (++g_event_que_idx >= EVENT_QUEUE_MAX)
			g_event_que_idx = 1;
		
		EM_DEBUG_LOG("g_event_que[%d].type [%d]", g_active_que, g_event_que[g_active_que].type);
	}
	else  {
		g_active_que = 0;
		error = EMAIL_ERROR_EVENT_QUEUE_EMPTY;
	}
	
	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/* check that event_data loop is continuous */
static int emcore_event_loop_continue()
{
    return g_event_loop;
}



INTERNAL_FUNC int emcore_insert_event_for_sending_mails(email_event_t *event_data, int *handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("event_data[%p], handle[%p], err_code[%p]", event_data, handle, err_code);
	
	if (!event_data)  {
		EM_DEBUG_EXCEPTION("\t event_data[%p], handle[%p]", event_data, handle);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = true;
	int error = EMAIL_ERROR_NONE;
	
	ENTER_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
	
	if (!g_send_event_que[g_send_event_que_idx].type)  {	
		/* if current buffer has not event_data, insert event_data data to current buffer */	
		EM_DEBUG_LOG("Current buffer has not a event_data. [%d]", g_send_event_que_idx);
		memcpy(g_send_event_que+g_send_event_que_idx, event_data, sizeof(email_event_t));
		
		g_send_event_que[g_send_event_que_idx].status = EMAIL_EVENT_STATUS_WAIT;
		
		if (handle) 
			*handle = g_send_event_que_idx;
	}
	else  {	
		/* if current buffer has event_data, find the empty buffer */
		EM_DEBUG_LOG("Current buffer has a event_data. [%d]", g_send_event_que_idx);
		int i, j = g_send_event_que_idx + 1;		
	
		for (i = 1; i < EVENT_QUEUE_MAX; i++, j++)  {
			if (j >= EVENT_QUEUE_MAX) 
				j = 1;

			if (!g_send_event_que[j].type) 
					break;
			} 
		
		if (i < EVENT_QUEUE_MAX)  {
			EM_DEBUG_LOG("I found available buffer. [%d]", j);		
			memcpy(g_send_event_que+j, event_data, sizeof(email_event_t));
			g_send_event_que[j].status = EMAIL_EVENT_STATUS_WAIT;
			if (handle) *handle = j;
		}
		else  {
			EM_DEBUG_EXCEPTION("event_data queue is full...");
			error = EMAIL_ERROR_EVENT_QUEUE_FULL;
			ret = false;
		}
	}
	
	LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
	
	if (ret == true)  {
		ENTER_CRITICAL_SECTION(_send_event_available_lock);
		WAKE_CONDITION_VARIABLE(_send_event_available_signal);
		LEAVE_CRITICAL_SECTION(_send_event_available_lock);
	}

	if (handle)
	EM_DEBUG_LOG("emcore_insert_event_for_sending_mails-handle[%d]", *handle);

	if (err_code != NULL)
		*err_code = error;
	
	/* EM_DEBUG_FUNC_BEGIN(); */
	return ret;
}


static int emcore_retrieve_send_event(email_event_t *event_data, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	ENTER_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
/* get a event_data if this queue is not empty */
	if (g_send_event_que[g_send_event_que_idx].type)  {
		memcpy(event_data, g_send_event_que+g_send_event_que_idx, sizeof(email_event_t));

		if (event_data->status != EMAIL_EVENT_STATUS_WAIT) {	
			memset(g_send_event_que+g_send_event_que_idx, 0x00, sizeof(email_event_t));
			g_send_active_que = 0;
		}
		else  {
			g_send_event_que[g_send_event_que_idx].status = EMAIL_EVENT_STATUS_STARTED;
			EM_DEBUG_LOG("g_send_event_que_idx[%d]", g_send_event_que_idx);
			g_send_active_que = g_send_event_que_idx;
		
			ret = true;
		}

		if (++g_send_event_que_idx >= EVENT_QUEUE_MAX)
			g_send_event_que_idx = 1;

			EM_DEBUG_LOG("\t g_send_event_que[%d].type = %d", g_send_active_que, g_send_event_que[g_send_active_que].type);
	}
	else  {
			EM_DEBUG_LOG("\t send event_data queue is empty...");
			g_send_active_que = 0;
			error = EMAIL_ERROR_EVENT_QUEUE_EMPTY;
	}

	LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);

	if (err_code != NULL)
		*err_code = error;
	
	return ret;
}


void* thread_func_branch_command_for_sending_mails(void *arg)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	email_event_t event_data;
	email_session_t *session = NULL;

	if (!emstorage_open(&err))  {
		EM_DEBUG_EXCEPTION("\t emstorage_open falied - %d", err);
		return NULL;
	}

	while (g_send_event_loop) {
		if (!emcore_get_empty_session(&session)) 
			EM_DEBUG_EXCEPTION("\t SEND THREAD emcore_get_empty_session failed...");

		if (!emcore_retrieve_send_event(&event_data, NULL))  {
			EM_DEBUG_LOG(">>>> waiting for send event_data>>>>>>>>>");
#ifdef __FEATURE_LOCAL_ACTIVITY__			
			if (send_thread_run && g_save_local_activity_run) {			
				emstorage_account_tbl_t *account_list = NULL;
				int count = 0, i;
				if (!emstorage_get_account_list(&count, &account_list, true, true, &err)) {
					EM_DEBUG_LOG("\t emstorage_get_account_list failed - %d", err);
				}
				else {
					for (i = 0; i < count; i++) {
						if (emcore_save_local_activity_sync(account_list[i].account_id, &err)) {
							EM_DEBUG_LOG("Found local activity...!");
							EM_DEBUG_LOG("Resetting g_save_local_activity_run ");
							g_save_local_activity_run = 0;
							emcore_clear_session(session);
						}
					}

					emstorage_free_account(&account_list, count, &err);
					
					if (!g_save_local_activity_run) {
						continue;
					}					
				}
			}
#endif					
			send_thread_run = 0;
			
			ENTER_CRITICAL_SECTION(_send_event_available_lock);	
			SLEEP_CONDITION_VARIABLE(_send_event_available_signal, _send_event_available_lock);
			LEAVE_CRITICAL_SECTION(_send_event_available_lock);
		}
		else {
			EM_DEBUG_LOG(">>>>>>>>>>>>>>Got SEND event_data>>>>>>>>>>>>>>>>");
			send_thread_run = 1;
			g_client_run = 1;

			if (!emnetwork_check_network_status( &err))  {
				EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
				
				emcore_show_user_message(event_data.event_param_data_4, EMAIL_ACTION_SEND_MAIL, err);
				if (!emcore_notify_network_event(NOTI_SEND_FAIL, event_data.account_id, NULL , event_data.event_param_data_4, err))
					EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_SEND_FAIL] Failed >>>> ");
				goto FINISH_OFF;				
			}

			switch (event_data.type)  {
				
				case EMAIL_EVENT_SEND_MAIL: 
					emdevice_set_dimming_on_off(false, NULL);
					
					if (!emcore_send_mail(event_data.account_id, event_data.event_param_data_5, event_data.event_param_data_4, &err))
						EM_DEBUG_EXCEPTION("emcore_send_mail failed [%d]", err);
					
					emdevice_set_dimming_on_off(true, NULL);
					break;
					
				case EMAIL_EVENT_SEND_MAIL_SAVED:  /* send mails to been saved in off-line mode */
					emdevice_set_dimming_on_off(false, NULL);
						
					if (!emcore_send_saved_mail(event_data.account_id, event_data.event_param_data_3, &err))
						EM_DEBUG_EXCEPTION("emcore_send_saved_mail failed - %d", err);
										
					emdevice_set_dimming_on_off(true, NULL);
					break;

#ifdef __FEATURE_LOCAL_ACTIVITY__
					
				case EMAIL_EVENT_LOCAL_ACTIVITY: {
					emdevice_set_dimming_on_off(false, NULL);
					emstorage_activity_tbl_t *local_activity = NULL;
					int activity_id_count = 0;
					int activity_chunk_count = 0;
					int *activity_id_list = NULL;
					int i = 0;

					if (false == emstorage_get_activity_id_list(event_data.account_id, &activity_id_list, &activity_id_count, ACTIVITY_SAVEMAIL, ACTIVITY_DELETEMAIL_SEND, true, &err)) {
						EM_DEBUG_EXCEPTION("emstorage_get_activity_id_list failed [%d]", err);
					}
					else {
						for (i = 0; i < activity_id_count; ++i) {
							if ((false == emstorage_get_activity(event_data.account_id, activity_id_list[i], &local_activity, &activity_chunk_count, true,  &err)) || (NULL == local_activity) || (0 == activity_chunk_count)) {
								EM_DEBUG_EXCEPTION(" emstorage_get_activity Failed [ %d] or local_activity is NULL [%p] or activity_chunk_count is 0[%d]", err, local_activity, activity_chunk_count);
							}
							else {
								EM_DEBUG_LOG("Found local activity type - %d", local_activity[0].activity_type);
								switch (local_activity[0].activity_type) {
									case ACTIVITY_SAVEMAIL:  {
										if (!emcore_sync_mail_from_client_to_server(event_data.account_id, local_activity[0].mail_id, &err)) {
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

					emdevice_set_dimming_on_off(true, NULL);
				}
				break;
#endif /* __FEATURE_LOCAL_ACTIVITY__ */					
				default:  
					EM_DEBUG_LOG("Others not supported by Send Thread..! [%d]", event_data.type);
					break;
			}
						
			ENTER_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
			memset(g_send_event_que+g_send_active_que, 0x00, sizeof(email_event_t));
			LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);

FINISH_OFF: 
			;
		}
		emcore_clear_session(session);
	}	

	if (!emstorage_close(&err)) 
		EM_DEBUG_EXCEPTION("emstorage_close falied [%d]", err);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return NULL;
}


int event_handler_EMAIL_EVENT_SYNC_HEADER(int input_account_id, int input_mailbox_id, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d], input_mailbox_id [%d], handle_to_be_published [%d], error[%p]", input_account_id, input_mailbox_id, handle_to_be_published, error);

	int err = EMAIL_ERROR_NONE, sync_type = 0, ret = false;
	int mailbox_count = 0, account_count = 0;
	int counter, account_index;
	int unread = 0, total_unread = 0;
	emcore_uid_list *uid_list = NULL;
	emstorage_account_tbl_t *account_tbl_array = NULL;
	emstorage_mailbox_tbl_t *mailbox_tbl_target = NULL, *mailbox_tbl_spam = NULL, *mailbox_tbl_list = NULL;
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

	if (!emcore_notify_network_event(NOTI_DOWNLOAD_START, input_account_id, input_mailbox_id_str, handle_to_be_published, 0))
		EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_DOWNLOAD_START] Failed >>>> ");
	
	if (!emnetwork_check_network_status(&err)) {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
		
		if (!emcore_notify_network_event(NOTI_DOWNLOAD_FAIL, input_account_id, input_mailbox_id_str, handle_to_be_published, err))
			EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_FAIL] Failed >>>> ");	
	}
	else {
		if (sync_type != EMAIL_SYNC_ALL_MAILBOX) {	/* Sync only particular mailbox */

			if ((err = emcore_update_sync_status_of_account(input_account_id, SET_TYPE_SET, SYNC_STATUS_SYNCING)) != EMAIL_ERROR_NONE)
				EM_DEBUG_EXCEPTION("emcore_update_sync_status_of_account failed [%d]", err);

			if (!emstorage_get_mailbox_by_mailbox_type(input_account_id, EMAIL_MAILBOX_TYPE_SPAMBOX, &mailbox_tbl_spam, false, &err)) {
				EM_DEBUG_LOG("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
			}

			if (!emcore_sync_header(mailbox_tbl_target, mailbox_tbl_spam, NULL, &uid_list, &unread, &err)) {
				EM_DEBUG_EXCEPTION("emcore_sync_header failed [%d]", err);
				if (!emcore_notify_network_event(NOTI_DOWNLOAD_FAIL, mailbox_tbl_target->account_id, mailbox_id_param_string, handle_to_be_published, err))
					EM_DEBUG_EXCEPTION(" emcore_notify_network_event [NOTI_DOWNLOAD_FAIL] Failed >>>> ");	
			}
			else {
				EM_DEBUG_LOG("emcore_sync_header succeeded [%d]", err);
				if (!emcore_notify_network_event(NOTI_DOWNLOAD_FINISH, mailbox_tbl_target->account_id, mailbox_id_param_string, handle_to_be_published, 0))
					EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_DOWNLOAD_FINISH] Failed >>>> ");	
			}

			total_unread += unread;

			if (total_unread > 0 && (err = emcore_update_sync_status_of_account(input_account_id, SET_TYPE_UNION, SYNC_STATUS_HAVE_NEW_MAILS)) != EMAIL_ERROR_NONE)
				EM_DEBUG_EXCEPTION("emcore_update_sync_status_of_account failed [%d]", err);

			if (!emcore_finalize_sync(input_account_id, &err))
				EM_DEBUG_EXCEPTION("emcore_finalize_sync failed [%d]", err);
		}
		else /*  All Foder */ {
			EM_DEBUG_LOG(">>>> SYNC ALL MAILBOX ");
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
				EM_DEBUG_LOG("Sync all mailbox of an account[%d].", input_account_id); 

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
					EM_DEBUG_LOG("account[%d] is for ActiveSync. Skip  ", account_index);
					continue;
				}
		
				if (!emstorage_get_mailbox_list(account_tbl_array[account_index].account_id, 0, EMAIL_MAILBOX_SORT_BY_TYPE_ASC, &mailbox_count, &mailbox_tbl_list, true, &err) || mailbox_count <= 0) {	
					EM_DEBUG_EXCEPTION("emstorage_get_mailbox failed [%d]", err);
			
					if (!emcore_notify_network_event(NOTI_DOWNLOAD_FAIL, account_tbl_array[account_index].account_id, input_mailbox_id_str, handle_to_be_published, err))
						EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_FAIL] Failed >>>> ");	
	
					continue;
				}

				EM_DEBUG_LOG("emcore_get_mailbox_list_to_be_sync returns [%d] mailboxes", mailbox_count);

				if(mailbox_tbl_spam) {
					if (!emstorage_free_mailbox(&mailbox_tbl_spam, 1, &err)) {
						EM_DEBUG_EXCEPTION("emstorage_free_mailbox failed [%d]", err);
					}
					mailbox_tbl_spam = NULL;
				}

				if (!emstorage_get_mailbox_by_mailbox_type(account_tbl_array[account_index].account_id, EMAIL_MAILBOX_TYPE_SPAMBOX, &mailbox_tbl_spam, false, &err)) {
					EM_DEBUG_LOG("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
				}

				if (mailbox_count > 0) {
#ifndef __FEATURE_KEEP_CONNECTION__
					if (account_tbl_array[account_index].incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
						memset(mailbox_id_param_string, 0, 10);
						SNPRINTF(mailbox_id_param_string, 10, "%d", mailbox_tbl_list[0].mailbox_id);
						if (!emcore_connect_to_remote_mailbox(account_tbl_array[account_index].account_id, mailbox_tbl_list[0].mailbox_id, (void **)&stream, &err))  {
							EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
							if (err == EMAIL_ERROR_LOGIN_FAILURE)
								EM_DEBUG_EXCEPTION("EMAIL_ERROR_LOGIN_FAILURE ");
							/* continue; */
							if (!emcore_notify_network_event(NOTI_DOWNLOAD_FAIL, account_tbl_array[account_index].account_id, mailbox_id_param_string,  handle_to_be_published, err))
								EM_DEBUG_EXCEPTION(" emcore_notify_network_event [NOTI_DOWNLOAD_FAIL] Failed >>>> ");	
							continue;
						}
						EM_DEBUG_LOG("emcore_connect_to_remote_mailbox returns [%d] : ", err);
					}
					else 
						stream = NULL;
#endif
				}
				
				for (counter = 0; counter < mailbox_count; counter++) {

					EM_DEBUG_LOG("maiblox_name [%s], mailbox_id [%d], mailbox_type [%d]", mailbox_tbl_list[counter].mailbox_name, mailbox_tbl_list[counter].mailbox_id, mailbox_tbl_list[counter].mailbox_type);

					if ( mailbox_tbl_list[counter].mailbox_type == EMAIL_MAILBOX_TYPE_ALL_EMAILS  
							|| mailbox_tbl_list[counter].mailbox_type == EMAIL_MAILBOX_TYPE_TRASH  
						/*|| mailbox_tbl_list[counter].mailbox_type == EMAIL_MAILBOX_TYPE_SPAMBOX */)
						EM_DEBUG_LOG("Skipped for all emails or trash");
					else if (!mailbox_tbl_list[counter].local_yn) {
						EM_DEBUG_LOG("[%s] Syncing...", mailbox_tbl_list[counter].mailbox_name);
#ifdef __FEATURE_KEEP_CONNECTION__
						if (!emcore_sync_header((mailbox_tbl_list + counter) , mailbox_tbl_spam, NULL, &uid_list, &unread, &err)) {
#else /*  __FEATURE_KEEP_CONNECTION__ */
						if (!emcore_sync_header((mailbox_tbl_list + counter) , mailbox_tbl_spam, (void *)stream, &uid_list, &unread, &err)) {
#endif /*  __FEATURE_KEEP_CONNECTION__ */ 
							EM_DEBUG_EXCEPTION("emcore_sync_header for %s(mailbox_id = %d) failed [%d]", mailbox_tbl_list[counter].mailbox_name, mailbox_tbl_list[counter].mailbox_id, err);

#ifndef __FEATURE_KEEP_CONNECTION__
							if (err == EMAIL_ERROR_CONNECTION_BROKEN || err == EMAIL_ERROR_NO_SUCH_HOST || err == EMAIL_ERROR_SOCKET_FAILURE) 
								stream = NULL;    /*  Don't retry to connect for broken connection. It might cause crash.  */
#endif /*  __FEATURE_KEEP_CONNECTION__ */ 
							memset(mailbox_id_param_string, 0, 10);
							SNPRINTF(mailbox_id_param_string, 10, "%d", mailbox_tbl_list[counter].mailbox_id);
							if (!emcore_notify_network_event(NOTI_DOWNLOAD_FAIL, account_tbl_array[account_index].account_id, mailbox_id_param_string,  handle_to_be_published, err))
								EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_FAIL] Failed >>>> ");

							break;
						}
					}
					total_unread  += unread;
				}
				
				EM_DEBUG_LOG("Sync for account_id(%d) is completed....!", account_tbl_array[account_index].account_id);
				if ((err == EMAIL_ERROR_NONE) && !emcore_notify_network_event(NOTI_DOWNLOAD_FINISH, account_tbl_array[account_index].account_id, NULL, handle_to_be_published, 0))
					EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_FINISH] Failed >>>> ");	

				if ((total_unread > 0) && (err = emcore_update_sync_status_of_account(account_tbl_array[account_index].account_id, SET_TYPE_UNION, SYNC_STATUS_HAVE_NEW_MAILS)) != EMAIL_ERROR_NONE)
					EM_DEBUG_EXCEPTION("emcore_update_sync_status_of_account failed [%d]", err);

				if (!emcore_finalize_sync(account_tbl_array[account_index].account_id, &err))
					EM_DEBUG_EXCEPTION("emcore_finalize_sync failed [%d]", err);
#ifndef __FEATURE_KEEP_CONNECTION__
				if (stream)  {
					emcore_close_mailbox(0, stream);
					stream = NULL;
				}
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
			emcore_close_mailbox(0, stream);
#endif
		if(mailbox_tbl_target)
			emstorage_free_mailbox(&mailbox_tbl_target, 1, NULL);

		if (mailbox_tbl_list)
			emstorage_free_mailbox(&mailbox_tbl_list, mailbox_count, NULL);

		if (account_tbl_array)
			emstorage_free_account(&account_tbl_array, account_count, NULL); 
	}

	EM_DEBUG_FUNC_END();
	return ret;
}

int event_handler_EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT(email_account_t *account, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN("account [%p]", account);
	int err, ret = false;

	if(!account) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("incoming_server_address  :  %s", account->incoming_server_address);
							
	if (!emnetwork_check_network_status(&err))  {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);

		if (!emcore_notify_network_event(NOTI_VALIDATE_AND_CREATE_ACCOUNT_FAIL, account->account_id, NULL,  handle_to_be_published, err))
			EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_FAIL] Failed >>>> ");	
		goto FINISH_OFF;
	}
	else  {
		EM_DEBUG_LOG("incoming_server_address : %s", account->incoming_server_address);

		if (!emcore_validate_account_with_account_info(account, &err)) {
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

			emcore_refresh_account_reference();
			
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

int event_handler_EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT(int account_id, email_account_t *new_account_info, int handle_to_be_published, int *error)
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
		EM_DEBUG_LOG("incoming_server_address: (%s)", new_account_info->incoming_server_address);

		if (!emcore_validate_account_with_account_info(new_account_info, &err)) {
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
				emcore_refresh_account_reference();
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

int event_handler_EMAIL_EVENT_SET_MAIL_SLOT_SIZE(int account_id, int mailbox_id, int new_slot_size, int handle_to_be_published, int *error)
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
int event_handler_EMAIL_EVENT_LOCAL_ACTIVITY(int account_id, int *error)
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

int event_handler_EMAIL_EVENT_DOWNLOAD_BODY(int account_id, int mail_id, int option, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;
	email_mailbox_t mailbox;
				
	memset(&mailbox, 0x00, sizeof(mailbox));
	mailbox.account_id = account_id;
	
	if (!emnetwork_check_network_status(&err))  {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);

		emcore_notify_network_event(NOTI_DOWNLOAD_BODY_FAIL, mail_id, NULL, handle_to_be_published, err);
	}
	else  {
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

int event_handler_EMAIL_EVENT_DOWNLOAD_ATTACHMENT(int account_id, int mail_id, int attachment_no, int handle_to_be_published, int *error)
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

int event_handler_EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER(int mail_ids[], int num, email_flags_field_type field_type, int value, int *error)
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

int event_handler_EMAIL_EVENT_VALIDATE_ACCOUNT(int account_id, int handle_to_be_published, int *error)
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
			email_account_t *account_ref;
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
			}
		}
	}

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return true;
}

int event_handler_EMAIL_EVENT_UPDATE_MAIL(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas, int handle_to_be_published)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list[%p], input_attachment_count[%d], input_meeting_request[%p], input_from_eas[%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas);
	int err = EMAIL_ERROR_NONE;

	if ( (err = emcore_update_mail(input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas)) != EMAIL_ERROR_NONE) 
		EM_DEBUG_EXCEPTION("emcore_update_mail failed [%d]", err);

	if(input_mail_data)
		emcore_free_mail_data_list(&input_mail_data, 1);

	if(input_attachment_data_list)
		emcore_free_attachment_data(&input_attachment_data_list, input_attachment_count, NULL);

	if(input_meeting_request)
		emstorage_free_meeting_request(input_meeting_request);

	EM_DEBUG_FUNC_END("err [%d", err);
	return err;
}
int event_handler_EMAIL_EVENT_SAVE_MAIL(int input_account_id, int input_mail_id, int input_handle_to_be_published)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d] input_mail_id [%d] input_handle_to_be_published [%d]", input_account_id, input_mail_id, input_handle_to_be_published);
	int err = EMAIL_ERROR_NONE;

	err = emcore_sync_mail_from_client_to_server(input_mail_id);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

int event_handler_EMAIL_EVENT_MOVE_MAIL(int account_id, int *mail_ids, int mail_id_count, int dest_mailbox_id, int src_mailbox_id, int handle_to_be_published, int *error)
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

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

int event_handler_EMAIL_EVENT_DELETE_MAILBOX(int mailbox_id, int on_server, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_id [%d], on_server [%d], handle_to_be_published [%d], error [%p]",  mailbox_id, on_server, handle_to_be_published, error);
	int err = EMAIL_ERROR_NONE;
	
	if (!emnetwork_check_network_status(&err)) 
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
	else  {
		if (!emcore_delete_mailbox(mailbox_id, on_server, &err))
			EM_DEBUG_LOG("emcore_delete failed - %d", err);
	}

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return true;
}

int event_handler_EMAIL_EVENT_CREATE_MAILBOX(int account_id, char *mailbox_name, char *mailbox_alias, int mailbox_type, int on_server, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	email_mailbox_t mailbox;

	memset(&mailbox, 0x00, sizeof(mailbox));

	mailbox.account_id = account_id;
	mailbox.mailbox_name = mailbox_name;
	mailbox.alias = mailbox_alias;
	mailbox.mailbox_type = mailbox_type;
	
	if (!emnetwork_check_network_status(&err))  {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
	}
	else  {
		if (!emcore_create_mailbox(&mailbox, on_server, &err)) 
			EM_DEBUG_EXCEPTION("emcore_create failed - %d", err);
	}


	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return true;
}

int event_handler_EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER(int mail_id, int *error)
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

int event_handler_EMAIL_EVENT_DELETE_MAIL_ALL(int input_account_id, int input_mailbox_id, int input_from_server, int *error)
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

int event_handler_EMAIL_EVENT_DELETE_MAIL(int account_id, int *mail_id_list, int mail_id_count, int *error)
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

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

int event_hanlder_EMAIL_EVENT_SYNC_HEADER_OMA(int account_id, char *maibox_name, int handle_to_be_published, int *error)
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

int event_handler_EMAIL_EVENT_SEARCH_ON_SERVER(int account_id, int mailbox_id, char *criteria, int handle_to_be_published, int *error)
{
	EM_DEBUG_FUNC_BEGIN("account_id : [%d], mailbox_id : [%d], criteria : [%s]", account_id, mailbox_id, criteria);

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
			
			if (!emcore_make_mail_tbl_data_from_envelope(stream, env, &uid_elem, &new_mail_tbl_data, &err) || !new_mail_tbl_data) {
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
		}
	}

	if (err == EMAIL_ERROR_NONE && !emcore_notify_network_event(NOTI_SEARCH_ON_SERVER_FINISH, account_id, NULL, handle_to_be_published, 0)) 
		EM_DEBUG_EXCEPTION("emcore_notify_network_event[NOTI_SEARCH_ON_SERVER_FINISH] Failed >>>>>");

FINISH_OFF:

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
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d], input_mailbox_id [%d], input_old_mailbox_path %s], input_new_mailbox_path [%s], input_new_mailbox_alias [%s], handle_to_be_published [%d]", input_account_id, input_mailbox_id, input_old_mailbox_path, input_new_mailbox_path, input_new_mailbox_alias, handle_to_be_published);
	int err = EMAIL_ERROR_NONE;

	if ((err = emcore_move_mailbox_on_imap_server(input_account_id, input_old_mailbox_path, input_new_mailbox_path)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_move_mailbox_on_imap_server failed [%d]", err);
	}

	if (err == EMAIL_ERROR_NONE) {
		if(!emcore_notify_network_event(NOTI_RENAME_MAILBOX_FINISH, input_mailbox_id, input_new_mailbox_path, handle_to_be_published, 0))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event[NOTI_RENAME_MAILBOX_FINISH] failed");

		if ((err = emstorage_rename_mailbox(input_mailbox_id, input_new_mailbox_path, input_new_mailbox_alias, true)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_rename_mailbox failed [%d]", err);
		}
	}
	else if (!emcore_notify_network_event(NOTI_RENAME_MAILBOX_FAIL, input_mailbox_id, input_new_mailbox_path, handle_to_be_published, 0)) {
		EM_DEBUG_EXCEPTION("emcore_notify_network_event[NOTI_RENAME_MAILBOX_FAIL] failed");
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

void* thread_func_branch_command(void *arg)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int err = EMAIL_ERROR_NONE;
	int is_storage_full = false;
	int noti_id = 0;
	email_event_t event_data;
	email_session_t *session = NULL;
	emstorage_account_tbl_t *account_tbl = NULL;
	int handle_to_be_published = 0;

	if (!emstorage_open(&err))  {
		EM_DEBUG_EXCEPTION("emstorage_open falied [%d]", err);
		return false;
	}

	/* check that event_data loop is continuous */
	while (emcore_event_loop_continue())  {
		if (!emcore_get_empty_session(&session)) 
			EM_DEBUG_EXCEPTION("emcore_get_empty_session failed...");
		
		/* get a event_data from event_data queue */
		if (!emcore_retrieve_event(&event_data, NULL))  {	/*  no event_data pending */
			EM_DEBUG_LOG("For handle g_event_que[g_event_que_idx].type [%d], g_event_que_idx [%d]", g_event_que[g_event_que_idx].type, g_event_que_idx);
#ifdef ENABLE_IMAP_IDLE_THREAD	
			if ( !emnetwork_check_network_status(&err))  {
				EM_DEBUG_LOG(">>>> Data Networking ON ");
				if (g_client_run) {
					if (g_imap_idle_thread_alive) {
						/* emcore_kill_imap_idle_thread(NULL); */
						/* emcore_create_imap_idle_thread(NULL); */
					}
					else {
						if (!send_thread_run)
							emcore_create_imap_idle_thread(event_data.account_id, NULL);
					}
				}
			}
#endif /*  ENABLE_IMAP_IDLE_THREAD */
#ifdef __FEATURE_LOCAL_ACTIVITY__ 
			/*  Local activity sync */
			if (g_client_run && g_local_activity_run) {	
				emstorage_account_tbl_t *account_list = NULL;
				int count = 0, i;
				if (!emstorage_get_account_list(&count, &account_list, true, true, &err)) 
					EM_DEBUG_EXCEPTION("emstorage_get_account_list failed [%d]", err);
				else {
					for (i = 0; i < count; i++) {
						if (emcore_local_activity_sync(account_list[i].account_id, &err)) {
							EM_DEBUG_LOG("Found local activity...!");
							EM_DEBUG_LOG("Resetting g_local_activity_run ");
							g_local_activity_run = 0;
							emcore_clear_session(session);
						}
					}
					
					emstorage_free_account(&account_list, count, &err);
					
					if (!g_local_activity_run)
						continue;
				}
			}
#endif

			recv_thread_run = 0;
		
			ENTER_CRITICAL_SECTION(_event_available_lock);
			SLEEP_CONDITION_VARIABLE(_event_available_signal, _event_available_lock);
			EM_DEBUG_LOG("Wake up by _event_available_signal");
			LEAVE_CRITICAL_SECTION(_event_available_lock);
		}
		else  {
			EM_DEBUG_LOG(">>>>>>>>>>>>>>> Got event_data !!! <<<<<<<<<<<<<<<");
			EM_DEBUG_LOG("For handle g_event_que_idx - %d", g_event_que_idx);

			if (g_event_que_idx == 1)
				handle_to_be_published = 31;
			else
				handle_to_be_published = g_event_que_idx - 1 ;
			
			EM_DEBUG_LOG("Handle to be Published  [%d]", handle_to_be_published);
			recv_thread_run = 1;
			g_client_run = 1;

			/*  Handling storage full */
			is_storage_full = false;
			if (event_data.type == EMAIL_EVENT_SYNC_HEADER || event_data.type == EMAIL_EVENT_SYNC_HEADER_OMA || 
				event_data.type == EMAIL_EVENT_DOWNLOAD_BODY || event_data.type == EMAIL_EVENT_DOWNLOAD_ATTACHMENT) {
				if (emcore_is_storage_full(&err) == true) {
					EM_DEBUG_EXCEPTION("Storage is full");
					switch (event_data.type) {
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
					
					if (!emcore_notify_network_event(noti_id, event_data.account_id, NULL,  handle_to_be_published, err))
						EM_DEBUG_EXCEPTION(" emcore_notify_network_event [NOTI_DOWNLOAD_FAIL] Failed >>>> ");	
					is_storage_full = true;
				}
			}

			emdevice_set_dimming_on_off(false, NULL);

			if (event_data.account_id > 0) {
				if (!emstorage_get_account_by_id(event_data.account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account_tbl, false, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_get_account_by_id [%d]", err);
				}
			}

			if (account_tbl)
				EM_DEBUG_LOG("account_id : [%d], sync_disabled : [%d]", event_data.account_id, account_tbl->sync_disabled);

			if (!account_tbl || account_tbl->sync_disabled == 0) {
				switch (event_data.type)  {
					case EMAIL_EVENT_SYNC_IMAP_MAILBOX:  /*  get imap mailbox list  */
						if (!emcore_sync_mailbox_list(event_data.account_id, event_data.event_param_data_3, handle_to_be_published, &err))
							EM_DEBUG_EXCEPTION("emcore_sync_mailbox_list failed [%d]", err);
						EM_SAFE_FREE(event_data.event_param_data_3);
						break;

					case EMAIL_EVENT_SYNC_HEADER:  /*  synchronize mail header  */
						if (is_storage_full == false)
							event_handler_EMAIL_EVENT_SYNC_HEADER(event_data.account_id, event_data.event_param_data_5, handle_to_be_published,  &err);
						break;

					case EMAIL_EVENT_SYNC_HEADER_OMA:  /*  synchronize mail header for OMA */
						if (is_storage_full == false)
							event_hanlder_EMAIL_EVENT_SYNC_HEADER_OMA(event_data.account_id, event_data.event_param_data_1, handle_to_be_published, &err);
						EM_SAFE_FREE(event_data.event_param_data_1);
						break;

					case EMAIL_EVENT_DOWNLOAD_BODY:  /*  download mail body  */
						if (is_storage_full == false)
							event_handler_EMAIL_EVENT_DOWNLOAD_BODY(event_data.account_id, (int)event_data.event_param_data_4, (int)event_data.event_param_data_3, handle_to_be_published, &err);
						event_data.event_param_data_3 = NULL;		/*  MUST BE */
						break;

					case EMAIL_EVENT_DOWNLOAD_ATTACHMENT:  /*  download attachment */
						if (is_storage_full == false)
							event_handler_EMAIL_EVENT_DOWNLOAD_ATTACHMENT(event_data.account_id, (int)event_data.event_param_data_4, event_data.event_param_data_5, handle_to_be_published, &err);
						break;

					case EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER:  /*  Sync flags field */
						event_handler_EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER((int*)event_data.event_param_data_3, event_data.event_param_data_4 , event_data.event_param_data_5, event_data.event_param_data_6, &err);
						EM_SAFE_FREE(event_data.event_param_data_3);
						break;

					case EMAIL_EVENT_DELETE_MAIL:  /*  delete mails */
						event_handler_EMAIL_EVENT_DELETE_MAIL(event_data.account_id, (int*)event_data.event_param_data_3, event_data.event_param_data_4, &err);
						EM_SAFE_FREE(event_data.event_param_data_3);
						break;

					case EMAIL_EVENT_DELETE_MAIL_ALL:  /*  delete all mails */
						event_handler_EMAIL_EVENT_DELETE_MAIL_ALL(event_data.account_id, event_data.event_param_data_4, (int)event_data.event_param_data_5, &err);
						break;
#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
					case EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER:
						event_handler_EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER((int)event_data.event_param_data_4, &err);
						break;
#endif

					case EMAIL_EVENT_CREATE_MAILBOX:
						event_handler_EMAIL_EVENT_CREATE_MAILBOX(event_data.account_id, event_data.event_param_data_1, event_data.event_param_data_2, GPOINTER_TO_INT(event_data.event_param_data_3), event_data.event_param_data_4, handle_to_be_published, &err);
						EM_SAFE_FREE(event_data.event_param_data_1);
						EM_SAFE_FREE(event_data.event_param_data_2);
						break;

					case EMAIL_EVENT_DELETE_MAILBOX:
						event_handler_EMAIL_EVENT_DELETE_MAILBOX(event_data.event_param_data_4, event_data.event_param_data_4, handle_to_be_published, &err);
						break;

					case EMAIL_EVENT_SAVE_MAIL:
						err = event_handler_EMAIL_EVENT_SAVE_MAIL(event_data.account_id, event_data.event_param_data_4, handle_to_be_published);
						break;

					case EMAIL_EVENT_MOVE_MAIL:
						event_handler_EMAIL_EVENT_MOVE_MAIL(event_data.account_id, (int  *)event_data.event_param_data_3, event_data.event_param_data_4, event_data.event_param_data_5, event_data.event_param_data_8, handle_to_be_published, &err);
						EM_SAFE_FREE(event_data.event_param_data_3);
						break;

					case EMAIL_EVENT_VALIDATE_ACCOUNT:
						event_handler_EMAIL_EVENT_VALIDATE_ACCOUNT(event_data.account_id, handle_to_be_published, &err);
						break;

					case EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT: {
/*						event_handler_EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT(emcore_get_account_from_unvalidated_account_list(), handle_to_be_published, &err);*/
							email_account_t *account = (email_account_t *)event_data.event_param_data_1;
							event_handler_EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT(account, handle_to_be_published, &err);
							if(account) {
								emcore_free_account(account);
								EM_SAFE_FREE(account);
							}
						}
						break;

					case EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT:  {
							email_account_t *account = (email_account_t *)event_data.event_param_data_1;
							event_handler_EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT(event_data.account_id, account, handle_to_be_published, &err);
							emcore_free_account(account);
							EM_SAFE_FREE(account);
						}
						break;

					case EMAIL_EVENT_UPDATE_MAIL:
						event_handler_EMAIL_EVENT_UPDATE_MAIL((email_mail_data_t*)event_data.event_param_data_1, (email_attachment_data_t*)event_data.event_param_data_2, event_data.event_param_data_4, (email_meeting_request_t*)event_data.event_param_data_3, event_data.event_param_data_5, handle_to_be_published);

						event_data.event_param_data_1 = NULL;
						event_data.event_param_data_2 = NULL;
						event_data.event_param_data_3 = NULL;
						break;

					case EMAIL_EVENT_SET_MAIL_SLOT_SIZE:
						event_handler_EMAIL_EVENT_SET_MAIL_SLOT_SIZE(event_data.account_id, event_data.event_param_data_4, event_data.event_param_data_5, handle_to_be_published, &err);
						break;

					case EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED:
						err = event_handler_EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED(event_data.account_id, event_data.event_param_data_4);
						break;

	#ifdef __FEATURE_LOCAL_ACTIVITY__
					case EMAIL_EVENT_LOCAL_ACTIVITY:
						event_handler_EMAIL_EVENT_LOCAL_ACTIVITY(event_data.account_id, &err);
						break;
	#endif /* __FEATURE_LOCAL_ACTIVITY__*/

					case EMAIL_EVENT_SEARCH_ON_SERVER:
						event_handler_EMAIL_EVENT_SEARCH_ON_SERVER(event_data.account_id, event_data.event_param_data_4, (char *)event_data.event_param_data_1, handle_to_be_published, &err);
						EM_SAFE_FREE(event_data.event_param_data_1);
						break;

					case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
						err = event_handler_EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER(event_data.account_id, event_data.event_param_data_4, (char*)event_data.event_param_data_1, (char*)event_data.event_param_data_2, (char*)event_data.event_param_data_3, handle_to_be_published);
						EM_SAFE_FREE(event_data.event_param_data_1);
						EM_SAFE_FREE(event_data.event_param_data_2);
						EM_SAFE_FREE(event_data.event_param_data_3);
						break;

					default:
						break;
				}
			}

			if (account_tbl) {
				emstorage_free_account(&account_tbl, 1, NULL);
				account_tbl = NULL;
			}

			if (!emcore_notify_response_to_api(event_data.type, handle_to_be_published, err))
				EM_DEBUG_EXCEPTION("emcore_notify_response_to_api failed");

			emdevice_set_dimming_on_off(true, NULL);
			em_flush_memory();
			
			switch (event_data.type)  {
				case EMAIL_EVENT_SEND_MAIL: 
				case EMAIL_EVENT_SEND_MAIL_SAVED: 
					_sending_busy_unref();
					break;
				
				case EMAIL_EVENT_SYNC_HEADER: 
				case EMAIL_EVENT_SYNC_HEADER_OMA:
				case EMAIL_EVENT_DOWNLOAD_BODY: 
				case EMAIL_EVENT_DOWNLOAD_ATTACHMENT: 
				case EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER:
				case EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER:
				case EMAIL_EVENT_DELETE_MAIL: 
				case EMAIL_EVENT_DELETE_MAIL_ALL:
				case EMAIL_EVENT_VALIDATE_ACCOUNT: 
				case EMAIL_EVENT_SYNC_IMAP_MAILBOX: 
				case EMAIL_EVENT_SAVE_MAIL:
				case EMAIL_EVENT_MOVE_MAIL: 				
				case EMAIL_EVENT_CREATE_MAILBOX: 			
				case EMAIL_EVENT_DELETE_MAILBOX: 			
				case EMAIL_EVENT_SET_MAIL_SLOT_SIZE:
				case EMAIL_EVENT_SEARCH_ON_SERVER: 
				case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
					_receiving_busy_unref();
					break;

				default: 
					break;
			}
			
			event_data.type = 0;
			
			ENTER_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
			memset(g_event_que+g_active_que, 0x00, sizeof(email_event_t));
			LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
		}
		
		emcore_clear_session(session);
	}

	if (!emstorage_close(&err)) 
		EM_DEBUG_EXCEPTION("emstorage_close falied [%d]", err);
	
	EM_DEBUG_FUNC_END();
	return SUCCESS;
}
/*Send event_data loop*/
INTERNAL_FUNC int emcore_start_event_loop_for_sending_mails(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int thread_error = -1;

	if (err_code != NULL) 
		*err_code = EMAIL_ERROR_NONE;

	memset(&g_send_event_que, 0x00, sizeof(g_send_event_que));

	if (g_send_srv_thread)  {
		EM_DEBUG_EXCEPTION("\t send service thread is already running...");
		if (err_code != NULL) 
			*err_code = EMAIL_ERROR_UNKNOWN;
		return true;
	}

	g_send_event_que_idx = 1;
	g_send_event_loop = 1;
	g_send_active_que = 0;

	/* initialize lock */
	/*  INITIALIZE_CRITICAL_SECTION(_send_event_available_lock); */
	INITIALIZE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
	INITIALIZE_CONDITION_VARIABLE(_send_event_available_signal);

	/* create thread */
	THREAD_CREATE_JOINABLE(g_send_srv_thread, thread_func_branch_command_for_sending_mails, thread_error);

	if (thread_error != 0) {
		EM_DEBUG_EXCEPTION("cannot make thread...");
		if (err_code != NULL) 
			*err_code = EMAIL_ERROR_UNKNOWN;
		return FAILURE;
	}

	if (err_code != NULL) 
		*err_code = EMAIL_ERROR_NONE;
	EM_DEBUG_FUNC_END();
	return SUCCESS;
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

	emcore_cancel_send_mail_thread(g_send_active_que, NULL, err_code);
	ENTER_CRITICAL_SECTION(_send_event_available_lock);
	WAKE_CONDITION_VARIABLE(_send_event_available_signal);		/*  MUST BE HERE */
	LEAVE_CRITICAL_SECTION(_send_event_available_lock);
	
	/* wait for thread finished */
	THREAD_JOIN(g_send_srv_thread);
	
	g_send_srv_thread = 0;

	DELETE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
	DELETE_CRITICAL_SECTION(_send_event_available_lock);
	DELETE_CONDITION_VARIABLE(_send_event_available_signal);
	
	g_send_event_que_idx = 1;
	g_send_active_que = 0;

	if (err_code != NULL) 
		*err_code = EMAIL_ERROR_NONE;

    return true;
}

/* start api event_data loop */
INTERNAL_FUNC int emcore_start_event_loop(int *err_code)
{
    EM_DEBUG_FUNC_BEGIN();
	int thread_error;

	if (err_code != NULL) 
		*err_code = EMAIL_ERROR_NONE;

    memset(&g_event_que, 0x00, sizeof(g_event_que));
	
	if (g_srv_thread) {
		EM_DEBUG_EXCEPTION("service thread is already running...");
		if (err_code != NULL) 
			*err_code = EMAIL_ERROR_UNKNOWN;
		return true;
	}

	g_event_que_idx = 1;
	g_event_loop = 1;
	g_active_que = 0;
	
    /* initialize lock */
	INITIALIZE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
	INITIALIZE_RECURSIVE_CRITICAL_SECTION(_event_callback_table_lock);

	emcore_initialize_event_callback_table();
	
    /* create thread */
	THREAD_CREATE(g_srv_thread, thread_func_branch_command, NULL, thread_error);

	if (thread_error != 0) {
        EM_DEBUG_EXCEPTION("cannot create thread");
		if (err_code != NULL) 
			*err_code = EMAIL_ERROR_SYSTEM_FAILURE;
        return FAILURE;
    }

	if (err_code != NULL) 
		*err_code = EMAIL_ERROR_NONE;

    return false;
}

/* finish api event_data loop */
INTERNAL_FUNC int emcore_stop_event_loop(int *err_code)
{
    EM_DEBUG_FUNC_BEGIN();

	if (err_code != NULL) 
		*err_code = EMAIL_ERROR_NONE;

	if (!g_srv_thread)  {
		if (err_code != NULL) 
			*err_code = EMAIL_ERROR_UNKNOWN;
		return false;
	}

    /* stop event_data loop */
    g_event_loop = 0;

	/* 	pthread_kill(g_srv_thread, SIGINT); */
	emcore_cancel_thread(g_active_que, NULL, err_code);
	
	ENTER_CRITICAL_SECTION(_event_available_lock);
	WAKE_CONDITION_VARIABLE(_event_available_signal);
	LEAVE_CRITICAL_SECTION(_event_available_lock);
	
	/* wait for thread finished */
	THREAD_JOIN(g_srv_thread);
	
	g_srv_thread = 0;

	DELETE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
	DELETE_CRITICAL_SECTION(_event_available_lock);
	DELETE_CONDITION_VARIABLE(_event_available_signal);
	DELETE_RECURSIVE_CRITICAL_SECTION(_event_callback_table_lock);

	g_event_que_idx = 1;
	g_active_que = 0;

	if (err_code != NULL) 
		*err_code = EMAIL_ERROR_NONE;
	EM_DEBUG_FUNC_END();
    return true;
}


int emcore_get_active_queue_idx()
{
	return g_send_active_que;
}

/* check thread status
* 0 : stop job 1 : continue job
*/
INTERNAL_FUNC int emcore_check_thread_status()
{
	if (g_active_que <= 0)
		return true;
	
	return (g_event_que[g_active_que].status == EMAIL_EVENT_STATUS_STARTED);
}

/* cancel a job  */
INTERNAL_FUNC int emcore_cancel_thread(int handle, void *arg, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("handle[%d], arg[%p], err_code[%p]", handle, arg, err_code);
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	
	if (handle <= 0 || handle > (EVENT_QUEUE_MAX - 1))  {
		EM_DEBUG_EXCEPTION("handle[%d], arg[%p]", handle, arg);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	ENTER_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
	
	EM_DEBUG_LOG("status[%d], type[%d], handle[%d]", g_event_que[handle].status, g_event_que[handle].type, handle);
	
	if (g_event_que[handle].status == EMAIL_EVENT_STATUS_WAIT)  {
		fail_status_notify(&g_event_que[handle], EMAIL_ERROR_CANCELLED);
		
		switch (g_event_que[handle].type)  {
			case EMAIL_EVENT_SEND_MAIL: 
			case EMAIL_EVENT_SEND_MAIL_SAVED: 
				EM_DEBUG_LOG("EMAIL_EVENT_SEND_MAIL or EMAIL_EVENT_SEND_MAIL_SAVED");
				_sending_busy_unref();
				if (!emcore_notify_network_event(NOTI_SEND_CANCEL, g_event_que[handle].account_id, NULL , g_event_que[handle].event_param_data_4, err))
					EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_SEND_CANCEL] Failed >>>> ");
				break;
			case EMAIL_EVENT_DOWNLOAD_BODY: 
				EM_DEBUG_LOG("EMAIL_EVENT_DOWNLOAD_BODY");
				_receiving_busy_unref();
				if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_CANCEL, g_event_que[handle].account_id, NULL , g_event_que[handle].event_param_data_4, err))
					EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_SEND_CANCEL] Failed >>>> ");
				break;

			case EMAIL_EVENT_SYNC_HEADER: 
			case EMAIL_EVENT_SYNC_HEADER_OMA:
			case EMAIL_EVENT_DOWNLOAD_ATTACHMENT: 
			case EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER:
			case EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER:
				EM_DEBUG_LOG("EMAIL_EVENT_SYNC_HEADER, EMAIL_EVENT_DOWNLOAD_ATTACHMENT");
				_receiving_busy_unref();
				break;
			
			case EMAIL_EVENT_VALIDATE_ACCOUNT: 
				EM_DEBUG_LOG(" validate account waiting  :  cancel acc id  :  %d", g_event_que[handle].account_id);
				_receiving_busy_unref();
				if (!emcore_notify_network_event(NOTI_VALIDATE_ACCOUNT_CANCEL, g_event_que[handle].account_id, NULL , g_event_que[handle].event_param_data_4, err))
					EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_VALIDATE_ACCOUNT_CANCEL] Failed >>>> ");
				break;

			case EMAIL_EVENT_DELETE_MAIL: 
			case EMAIL_EVENT_DELETE_MAIL_ALL:
			case EMAIL_EVENT_SYNC_IMAP_MAILBOX: 
			case EMAIL_EVENT_SAVE_MAIL:
			case EMAIL_EVENT_MOVE_MAIL: 
			case EMAIL_EVENT_CREATE_MAILBOX: 		
			case EMAIL_EVENT_DELETE_MAILBOX: 		
			case EMAIL_EVENT_SET_MAIL_SLOT_SIZE: 
			case EMAIL_EVENT_SEARCH_ON_SERVER:
			case EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER:
				EM_DEBUG_LOG("EMAIL_EVENT_DELETE_MAIL, EMAIL_EVENT_SYNC_IMAP_MAILBOX");
				_receiving_busy_unref();
				break;
 			default: 
				break;
		}
	}
	
	memset(g_event_que+handle, 0x00, sizeof(email_event_t));
	g_event_que[handle].status = EMAIL_EVENT_STATUS_CANCELED; 

	LEAVE_RECURSIVE_CRITICAL_SECTION(_event_queue_lock);
	
	ret = true;
	
FINISH_OFF: 
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

/* check thread status
* 0 : stop job 1 : continue job
*/
int emcore_check_send_mail_thread_status()
{
	EM_DEBUG_FUNC_BEGIN();

	if (g_send_active_que <= 0)
		return true;
	EM_DEBUG_LOG("g_send_event_que[g_send_active_que[%d]].status[%d]", g_send_active_que, g_send_event_que[g_send_active_que].status);
	EM_DEBUG_FUNC_END();
	return (g_send_event_que[g_send_active_que].status == EMAIL_EVENT_STATUS_STARTED);
}

INTERNAL_FUNC int emcore_cancel_all_threads_of_an_account(int account_id)
{
	EM_DEBUG_FUNC_BEGIN();
	int error_code = EMAIL_ERROR_NONE;
	int i, event_count = EVENT_QUEUE_MAX, exit_flag = 0, sleep_count = 0;

	for (i = 0 ; i < event_count; i++) {
		if (g_event_que[i].type && g_event_que[i].status != EMAIL_EVENT_STATUS_UNUSED) {	
			EM_DEBUG_LOG("There is a live thread. %d", i);
			if (g_event_que[i].account_id == account_id || g_event_que[i].account_id == ALL_ACCOUNT) {
				EM_DEBUG_LOG("And it is for account %d", g_event_que[i].account_id);
				emcore_cancel_thread(i, NULL, &error_code);
			}
		}
	}

	while (exit_flag == 0 && sleep_count < 30) {
		EM_DEBUG_LOG("Sleeping...");
		usleep(100000);
		EM_DEBUG_LOG("Wake up!");
		sleep_count++;
		exit_flag = 1;
		for (i = 0 ; i < event_count; i++) {
			if (g_event_que[i].type && g_event_que[i].status != EMAIL_EVENT_STATUS_UNUSED) {
				EM_DEBUG_LOG("There is still a live thread. %d", i);
				if (g_event_que[i].account_id == account_id || g_event_que[i].account_id == ALL_ACCOUNT) {
					EM_DEBUG_LOG("And it is for account %d. So, I should sleep for a while.", g_event_que[i].account_id);
					exit_flag = 0;
				}
			}
		}
	}

	EM_DEBUG_LOG("Sleep count %d", sleep_count);

	if (sleep_count >= 30)
		error_code = EMAIL_ERROR_CANNOT_STOP_THREAD;
	else
		error_code = EMAIL_ERROR_NONE;
	EM_DEBUG_FUNC_END();
	return error_code;
}


/* cancel send mail job  */
INTERNAL_FUNC int emcore_cancel_send_mail_thread(int handle, void *arg, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("handle[%d], arg[%p], err_code[%p]", handle, arg, err_code);
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	
	if (handle <= 0 || handle > (EVENT_QUEUE_MAX - 1))  {
		EM_DEBUG_EXCEPTION("handle[%d], arg[%p]", handle, arg);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	ENTER_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
	
	EM_DEBUG_LOG("event_data.status[%d], handle[%d]", g_send_event_que[handle].status, handle);
	
	if (g_send_event_que[handle].status == EMAIL_EVENT_STATUS_WAIT)  {
		fail_status_notify(&g_send_event_que[handle], EMAIL_ERROR_CANCELLED);
		
		switch (g_send_event_que[handle].type)  {
			case EMAIL_EVENT_SEND_MAIL: 
			case EMAIL_EVENT_SEND_MAIL_SAVED: 
				_sending_busy_unref();
				g_send_event_que[handle].status = EMAIL_EVENT_STATUS_CANCELED;
				if (!emcore_notify_network_event(NOTI_SEND_CANCEL, g_send_event_que[handle].account_id, NULL , g_send_event_que[handle].event_param_data_4, err))
					EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_SEND_CANCEL] Failed >>>> ");
				break;			
			default: 
				break;
		}
	}
	
	EM_DEBUG_LOG("send_mail_cancel");
	memset(g_send_event_que+handle, 0x00, sizeof(email_event_t));
	g_send_event_que[handle].status = EMAIL_EVENT_STATUS_CANCELED;

	EM_DEBUG_LOG("event_data.status[%d], handle[%d]", g_send_event_que[handle].status, handle);

	
	LEAVE_RECURSIVE_CRITICAL_SECTION(_send_event_queue_lock);
	
	ret = true;
	
FINISH_OFF: 
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emcore_get_receiving_event_queue(email_event_t **event_queue, int *event_active_queue, int *err)
{
	if (event_queue == NULL || event_active_queue == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM event_queue[%p] event_active_queue[%p]", event_queue, event_active_queue);

		if (err)	
			*err = EMAIL_ERROR_INVALID_PARAM;
		
		return false;
	}

	*event_queue = g_event_que;
	*event_active_queue = g_active_que;
	
	return true;
}

INTERNAL_FUNC int emcore_free_event(email_event_t *event_data)
{
	EM_DEBUG_FUNC_BEGIN("event_data [%p]", event_data);

	if(event_data) {
		EM_SAFE_FREE(event_data->event_param_data_1);
		EM_SAFE_FREE(event_data->event_param_data_2);
		EM_SAFE_FREE(event_data->event_param_data_3);
	}

	EM_DEBUG_FUNC_END();
	return true;
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

static int emcore_set_pbd_thd_state(int flag)
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

static void emcore_pb_thd_set_local_activity_continue(int flag)
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

static 
int emcore_pb_thd_can_local_activity_continue()
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

	EM_DEBUG_LOG("dest->account_id[%d], dest->mail_id[%d], dest->server_mail_id [%lu]", dest->account_id, dest->mail_id , dest->server_mail_id);

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
	memset(pbd_event, 0x00, sizeof(email_event_partial_body_thd));
	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emcore_insert_partial_body_thread_event(email_event_partial_body_thd *partial_body_thd_event, int *error_code)
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

	for (count = 0, index =  g_partial_body_thd_next_event_idx; count < TOTAL_PARTIAL_BODY_EVENTS;) {
		if  (g_partial_body_thd_event_que[index].event_type) {
			++index;
			++count;

			if (index == TOTAL_PARTIAL_BODY_EVENTS) {
				index = 0;
			}
		}
		else {
			/*Found empty Cell*/	
			
			empty_cell_index =	 index;
			break;
		}
	}

	if (-1 != empty_cell_index) {
		if (false == emcore_copy_partial_body_thd_event(partial_body_thd_event, g_partial_body_thd_event_que+empty_cell_index , &error)) {
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
		EM_DEBUG_LOG(" partial body thread event_data queue is full ");
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

static int emcore_retrieve_partial_body_thread_event(email_event_partial_body_thd *partial_body_thd_event, int *error_code)
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

gpointer partial_body_download_thread(gpointer data)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;
	email_session_t *session = NULL;
	email_event_partial_body_thd partial_body_thd_event;

	EM_DEBUG_LOG(" ************ PB THREAD ID IS ALIVE. ID IS [%d] ********************" , THREAD_SELF());
	
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
						partial_body_thd_event.mailbox_name = EM_SAFE_STRDUP(g_partial_body_bulk_dwd_que[0].mailbox_name);
						
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
								continue;
							}						
						}	
					}
					
					EM_DEBUG_LOG(" Partial Body Thread is going to sleep");

					emcore_set_pbd_thd_state(false);

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

				emdevice_set_dimming_on_off(false, NULL);
				
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
				
				emdevice_set_dimming_on_off(true, NULL);
		       }	

		       emcore_clear_session(session);
	       }	
	}

	/* If something is added to end thread in future for any case then if thread is holding any resources
	define a function emcore_partial_body_thd_loop_stop to release resources and call it
	here to end thread */
	return SUCCESS;
}

INTERNAL_FUNC int emcore_start_thread_for_downloading_partial_body(int *err_code)
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

/*Function to flush the bulk partial body download queue [santosh.br@samsung.com]*/
static int emcore_partial_body_bulk_flush(int *error_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int error = EMAIL_ERROR_NONE;
	int ret = false;
	MAILSTREAM *stream = NULL;
	void *tmp_stream = NULL;

	if (!emcore_connect_to_remote_mailbox(g_partial_body_bulk_dwd_que[0].account_id, g_partial_body_bulk_dwd_que[0].mailbox_id, (void **)&tmp_stream, &error) || (NULL == tmp_stream)) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", error);
		goto FINISH_OFF;
	}
	stream = (MAILSTREAM *)tmp_stream;

	/*  Call bulk download here */
	if (false == emcore_download_bulk_partial_mail_body(stream, g_partial_body_bulk_dwd_que, g_partial_body_bulk_dwd_next_event_idx, &error)) {
		EM_DEBUG_EXCEPTION(" emcore_download_bulk_partial_mail_body failed.. [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;
FINISH_OFF: 	

	emcore_close_mailbox(0, stream);				
	stream = NULL;

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
INTERNAL_FUNC int emcore_mail_partial_body_download(email_event_partial_body_thd *pbd_event, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int error = EMAIL_ERROR_NONE;
	int num_activity = 0;
	int ret = false;
	int count = 0;
	int i = 0, m = 0;
	MAILSTREAM *stream = NULL;
	void *tmp_stream = NULL;
	email_event_partial_body_thd *activity_data_list = NULL;
	int *mailbox_list = NULL;

	if (NULL == pbd_event)
    {
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
				if (false == emcore_partial_body_bulk_flush(&error)) {
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
				if (false == emcore_partial_body_bulk_flush(&error)) {
					EM_DEBUG_EXCEPTION("Partial Body thread emcore_partial_body_bulk_flush failed - %d", error);	
					goto FINISH_OFF;
				}
			}
		}
		/*Add the event_data to the download que array */
		if (false == emcore_copy_partial_body_thd_event(pbd_event, g_partial_body_bulk_dwd_que+(g_partial_body_bulk_dwd_next_event_idx), &error))
			EM_DEBUG_EXCEPTION("\t Partial Body thread emcore_copy_partial_body_thd_event failed - %d", error);		
		else {
			g_partial_body_bulk_dwd_queue_empty = false;
			g_partial_body_bulk_dwd_next_event_idx++;
			EM_DEBUG_LOG("g_partial_body_bulk_dwd_next_event_idx [%d]", g_partial_body_bulk_dwd_next_event_idx);
		}
	}
	else if (pbd_event->activity_type) {		
		int *account_list = NULL;
		int account_count = 0;

		EM_DEBUG_LOG("Event is coming from local activity.");
		/* Get all the accounts for which local activities are pending */
		if (false == emstorage_get_pbd_account_list(&account_list, &account_count, false, &error)) {
				EM_DEBUG_EXCEPTION(" emstorage_get_mailbox_list failed.. [%d]", error);
				error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
				goto FINISH_OFF;
		}

		for (m = 0; m < account_count; ++m) {
			/* Get the mailbox list for the account to start bulk partial body fetch for mails in each mailbox of accounts one by one*/
			if (false == emstorage_get_pbd_mailbox_list(account_list[m], &mailbox_list, &count, false, &error)) {
					EM_DEBUG_EXCEPTION(" emstorage_get_mailbox_list failed.. [%d]", error);
					error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
					goto FINISH_OFF;
			}
			
			for (i = 0; i < count; i++) {
				int k = 0;
				int activity_count = 0;

				if (!emcore_connect_to_remote_mailbox(account_list[m], mailbox_list[i], (void **)&tmp_stream, &error))  {
					EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", error);
					stream = NULL;
					goto FINISH_OFF;
				}

				stream = (MAILSTREAM *)tmp_stream;
			
				if (false == emstorage_get_mailbox_pbd_activity_count(account_list[m], mailbox_list[i], &activity_count, false, &error)) {
					EM_DEBUG_EXCEPTION(" emstorage_get_mailbox_pbd_activity_count failed.. [%d]", error);
					continue;
				}
				
				if (activity_count > 0) {
					int temp_error = EMAIL_ERROR_NONE;
					int j = 0;
					int iter = 0; 
					int remainder = 0;
					int num = BULK_PARTIAL_BODY_DOWNLOAD_COUNT;
					int index = 0;

					if (false == emstorage_get_pbd_activity_data(account_list[j], mailbox_list[i], &activity_data_list, &num_activity,  false, &error))
						EM_DEBUG_EXCEPTION(" emstorage_get_pbd_activity_data failed.. [%d]", error);

					if (NULL == activity_data_list) {
						EM_DEBUG_EXCEPTION(" activity_data_list is null..");
						continue;
					}

					remainder = num_activity%BULK_PARTIAL_BODY_DOWNLOAD_COUNT;
					iter = num_activity/BULK_PARTIAL_BODY_DOWNLOAD_COUNT + ((num_activity%BULK_PARTIAL_BODY_DOWNLOAD_COUNT) ? 1  :  0);
					
					for (j = 0; j < iter; j++) {
	 					if ((iter == (j+1)) && (remainder != 0))
							num = remainder;

						/*Call bulk download here */
						if (false == emcore_download_bulk_partial_mail_body(stream, activity_data_list+index, num, &error)) {
							EM_DEBUG_EXCEPTION(" emcore_download_bulk_partial_mail_body failed.. [%d]", error);
							temp_error = EMAIL_ERROR_NO_RESPONSE;
						}
						
						for (k = 0; k < num; k++) {
							if (activity_data_list[index + k].activity_type)
								emcore_free_partial_body_thd_event(activity_data_list + index + k, &error);
							else
								break;
						}
						index += num;
						
						if (false == emcore_is_partial_body_thd_que_empty()) {
							ret = true;
							goto FINISH_OFF;		/* Stop Local Activity Sync */
						}
						if (EMAIL_ERROR_NO_RESPONSE == temp_error) {
							temp_error = EMAIL_ERROR_NONE;
							break;
						}
					}
				}
			}	
			emcore_close_mailbox(0, stream);				
			stream = NULL;
			tmp_stream = NULL;
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
		if (false == emcore_partial_body_bulk_flush(&error)) {
			EM_DEBUG_EXCEPTION("\t Partial Body thread emcore_partial_body_bulk_flush failed - %d", error);	
			goto FINISH_OFF;
		}
	}

	ret = true;
	
FINISH_OFF: 
	
	EM_SAFE_FREE(mailbox_list);

	if (activity_data_list) {
		for (i = 0; i < num_activity; i++) {
			if (activity_data_list[i].activity_type)
				emcore_free_partial_body_thd_event(activity_data_list + i, &error);
			else
				break;
		}
	}

	emcore_close_mailbox(0, stream);				
	stream = NULL;
	
	if (NULL != error_code)
		*error_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

#endif /*  __FEATURE_PARTIAL_BODY_DOWNLOAD__ */
