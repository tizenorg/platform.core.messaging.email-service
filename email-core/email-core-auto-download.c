/*
*  email-service
*
* Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Minsoo Kim <minnsoo.kim@samsung.com>, Kyuho Jo <kyuho.jo@samsung.com>,
* Sunghyun Kwon <sh0701.kwon@samsung.com>
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
#include "email-storage.h"
#include "email-utilities.h"
#include "email-daemon.h"
#include "email-network.h"
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
#include "email-debug-log.h"
#include "email-types.h"
#include "email-internal-types.h"
#include "email-core-auto-download.h"

/*-----------------------------------------------------------------------------
 * Auto Download Event Queue
 *---------------------------------------------------------------------------*/
INTERNAL_FUNC thread_t g_auto_download_thread;
INTERNAL_FUNC pthread_cond_t  _auto_downalod_available_signal = PTHREAD_COND_INITIALIZER;
INTERNAL_FUNC pthread_mutex_t *_auto_download_queue_lock = NULL;

INTERNAL_FUNC GQueue *g_auto_download_que = NULL;
INTERNAL_FUNC int g_auto_download_loop = 1;
INTERNAL_FUNC int auto_download_thread_run = 0;

#define AUTO_DOWNLOAD_QUEUE_MAX 100

static void* worker_auto_download_queue(void *arg);

INTERNAL_FUNC int emcore_start_auto_download_loop(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int thread_error;

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	if (g_auto_download_thread) {
		EM_DEBUG_EXCEPTION("auto downlaod thread is already running...");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_UNKNOWN;
		return true;
	}

	g_auto_download_que = g_queue_new();
	g_queue_init(g_auto_download_que);
	g_auto_download_loop = 1;

	/* initialize lock */
	INITIALIZE_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);

	/* create thread */
	THREAD_CREATE(g_auto_download_thread, worker_auto_download_queue, NULL, thread_error);

	if (thread_error != 0) {
		EM_DEBUG_EXCEPTION("cannot create thread");
		g_auto_download_loop = 0;
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_SYSTEM_FAILURE;
		return false;
	}

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	return true;
}


INTERNAL_FUNC int emcore_auto_download_loop_continue(void)
{
	return g_auto_download_loop;
}


INTERNAL_FUNC int emcore_stop_auto_download_loop(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	if (!g_auto_download_thread) {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_UNKNOWN;
		return false;
	}

	/* stop event_data loop */
	g_auto_download_loop = 0;

	WAKE_CONDITION_VARIABLE(_auto_downalod_available_signal);

	/* wait for thread finished */
	THREAD_JOIN(g_auto_download_thread);

	g_queue_free(g_auto_download_que);

	g_auto_download_thread = 0;

	DELETE_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);
	DELETE_CONDITION_VARIABLE(_auto_downalod_available_signal);

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;
	EM_DEBUG_FUNC_END();
	return true;
}


INTERNAL_FUNC int emcore_insert_auto_download_event(email_event_auto_download *event_data, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("event_data[%p], err_code[%p]", event_data, err_code);

	if (!event_data) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	if (!g_auto_download_thread) {
		EM_DEBUG_EXCEPTION("g_auto_download_thread is not ready");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_LOAD_ENGINE_FAILURE;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int q_length = 0;

	ENTER_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);

	if (g_auto_download_que)
		q_length = g_queue_get_length(g_auto_download_que);
	EM_DEBUG_LOG("Q Length : [%d]", q_length);

	EM_DEBUG_LOG("event_data->status : [%d]", event_data->status);

	if (q_length > AUTO_DOWNLOAD_QUEUE_MAX) {
		EM_DEBUG_EXCEPTION("auto download que is full...");
		error = EMAIL_ERROR_EVENT_QUEUE_FULL;
		ret = false;
	} else if (event_data->status == EMAIL_EVENT_STATUS_DIRECT) {
		event_data->status = EMAIL_EVENT_STATUS_WAIT;
		g_queue_push_head(g_auto_download_que, event_data);
		//WAKE_CONDITION_VARIABLE(_auto_downalod_available_signal);
		ret = true;
	} else {
		event_data->status = EMAIL_EVENT_STATUS_WAIT;
		g_queue_push_tail(g_auto_download_que, event_data);
		//WAKE_CONDITION_VARIABLE(_auto_downalod_available_signal);
		ret = true;
	}

	LEAVE_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);

	if (err_code) {
		EM_DEBUG_LOG("ERR [%d]", error);
		*err_code = error;
	}

	return ret;
}


INTERNAL_FUNC int emcore_retrieve_auto_download_event(email_event_auto_download **event_data, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("event_data[%p], err_code[%p]", event_data, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int q_length = 0;
	//email_event_auto_download *poped = NULL;
	email_event_auto_download *head_event = NULL;

	if (g_auto_download_que)
		q_length = g_queue_get_length(g_auto_download_que);
	EM_DEBUG_LOG("Q Length : [%d]", q_length);

	if (!q_length) {
		error = EMAIL_ERROR_EVENT_QUEUE_EMPTY;
		EM_DEBUG_LOG("QUEUE is empty");
		goto FINISH_OFF;
	}

	while (1) {
		head_event = (email_event_auto_download *)g_queue_peek_head(g_auto_download_que);
		if (!head_event) {
			error = EMAIL_ERROR_EVENT_QUEUE_EMPTY;
			EM_DEBUG_LOG_DEV("QUEUE is empty");
			break;
		}

		/*if (head_event->status != EMAIL_EVENT_STATUS_WAIT) {
			EM_DEBUG_LOG("EVENT STATUS [%d]", head_event->status);
			poped = g_queue_pop_head(g_auto_download_que);
			if (poped) {
				EM_SAFE_FREE(poped);
			} else {
				error = EMAIL_ERROR_EVENT_QUEUE_EMPTY;
				EM_DEBUG_LOG("QUEUE is empty");
				break;
			}
		} else*/
		{
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


INTERNAL_FUNC int emcore_is_auto_download_queue_empty(void)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int q_length = 0;

	ENTER_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);

	if (g_auto_download_que)
		q_length = g_queue_get_length(g_auto_download_que);

	EM_DEBUG_LOG("Q Length : [%d]", q_length);

	if (q_length <= 0) {
		EM_DEBUG_LOG("auto downlaod que is empty...");
		ret = true;
	}

	LEAVE_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emcore_is_auto_download_queue_full(void)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int q_length = 0;

	ENTER_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);

	if (g_auto_download_que)
		q_length = g_queue_get_length(g_auto_download_que);

	EM_DEBUG_LOG("Q Length : [%d]", q_length);

	if (q_length > AUTO_DOWNLOAD_QUEUE_MAX) {
		EM_DEBUG_LOG("auto downlaod que is full...");
		ret = true;
	}

	LEAVE_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);
	EM_DEBUG_FUNC_END();
	return ret;
}


INTERNAL_FUNC int emcore_clear_auto_download_queue(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = false;
	int i = 0;
	int q_length = 0;
	email_event_auto_download *pop_elm = NULL;

	ENTER_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);

	q_length = g_auto_download_que ? g_queue_get_length(g_auto_download_que) : 0;

	for (i = 0; i < q_length; i++) {
		pop_elm = (email_event_auto_download *)g_queue_peek_nth(g_auto_download_que, i);

		if (pop_elm) {
			EM_SAFE_FREE(pop_elm);
		}
	}

	g_queue_clear(g_auto_download_que);
	ret = true;

	LEAVE_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);
	EM_DEBUG_FUNC_END();
	return ret;
}


static void* worker_auto_download_queue(void *arg)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

	int ai = 0;
	int mi = 0;
	int di = 0;
	int account_count = 0;
	int mailbox_count = 0;
	int activity_count = 0;
	int activity_list_count = 0;
	int *account_list = NULL;
	int *mailbox_list = NULL;
	email_event_auto_download *event_data = NULL;
	email_event_auto_download *started_event = NULL;
	email_event_auto_download *activity_list = NULL;


	if (!emstorage_open(NULL, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_open falied [%d]", err);
		return false;
	}

	/* check that event_data loop is continuous */
	while (emcore_auto_download_loop_continue()) {

		ENTER_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);

		if (!emcore_retrieve_auto_download_event(&event_data, &err)) {
			/* no event_data pending */
			if (err != EMAIL_ERROR_EVENT_QUEUE_EMPTY) {
				LEAVE_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);
				continue;
			}

			activity_count = 0;
			if (!emstorage_get_auto_download_activity_count(NULL, &activity_count, false, &err)) {
				EM_DEBUG_LOG("emstorage_get_auto_download_activity_count failed [%d]", err);
				goto CHECK_CONTINUE;
			}

			if (activity_count <= 0) {
				EM_DEBUG_LOG("auto download activity count is 0");
				goto CHECK_CONTINUE;
			}

			account_count = 0;
			EM_SAFE_FREE(account_list);
			if (!emstorage_get_auto_download_account_list(NULL, &account_list, &account_count, false, &err)) {
				EM_DEBUG_EXCEPTION(" emstorage_get_auto_download_account_list failed.. [%d]", err);
				goto CHECK_CONTINUE;
			}

			if (account_count <= 0 || !account_list) {
				EM_DEBUG_LOG("auto download account count is 0");
				goto CHECK_CONTINUE;
			}

			for (ai = 0; ai < account_count; ai++) {

				email_account_t *account_ref = NULL;
				if (!(account_ref = emcore_get_account_reference(NULL, account_list[ai], false))) {
					EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_list[ai]);
					err = EMAIL_ERROR_INVALID_ACCOUNT;
					continue;
				}

				if (!(account_ref->wifi_auto_download)) {
					if (account_ref) {
						emcore_free_account(account_ref);
						EM_SAFE_FREE(account_ref);
					}

					continue;
				}

				if (account_ref) {
					emcore_free_account(account_ref);
					EM_SAFE_FREE(account_ref);
				}

				mailbox_count = 0;
				EM_SAFE_FREE(mailbox_list);
				if (!emstorage_get_auto_download_mailbox_list(NULL, account_list[ai], &mailbox_list, &mailbox_count, false, &err)) {
					EM_DEBUG_EXCEPTION(" emstorage_get_auto_download_mailbox_list failed.. [%d]", err);
					goto CHECK_CONTINUE;
				}

				if (mailbox_count <= 0 || !mailbox_list) {
					EM_DEBUG_LOG("auto download mailbox count is 0");
					goto CHECK_CONTINUE;
				}

				for (mi = 0; mi < mailbox_count; mi++) {
					emstorage_mailbox_tbl_t *target_mailbox = NULL;

					activity_list_count = 0;
					EM_SAFE_FREE(activity_list);

					if ((err = emstorage_get_mailbox_by_id(NULL, mailbox_list[mi], &target_mailbox)) != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", err);
					}

					if (target_mailbox) {
						if (target_mailbox->mailbox_type != EMAIL_MAILBOX_TYPE_INBOX) {
							EM_DEBUG_LOG("Not INBOX mail, skip to next");
							emstorage_free_mailbox(&target_mailbox, 1, NULL);
							continue;
						}
						emstorage_free_mailbox(&target_mailbox, 1, NULL);
					}

					if (!emstorage_get_auto_download_activity(NULL, account_list[ai], mailbox_list[mi], &activity_list, &activity_list_count, false, &err)) {
						EM_DEBUG_EXCEPTION(" emstorage_get_auto_download_activity failed.. [%d]", err);
						goto CHECK_CONTINUE;
					}

					if (activity_list_count <= 0 || !activity_list) {
						EM_DEBUG_LOG("auto download activity count is 0");
						goto CHECK_CONTINUE;
					}

					for (di = 0; di < activity_list_count; di++) {
						email_event_auto_download *activity = NULL;
						activity = (email_event_auto_download *)em_malloc(sizeof(email_event_auto_download));
						if (!activity) {
							EM_DEBUG_EXCEPTION("Malloc failed");
							err = EMAIL_ERROR_OUT_OF_MEMORY;
							goto CHECK_CONTINUE;
						}

						activity->activity_id = activity_list[di].activity_id;
						activity->status = 0;
						activity->account_id = activity_list[di].account_id;
						activity->mail_id = activity_list[di].mail_id;
						activity->server_mail_id = activity_list[di].server_mail_id;
						activity->mailbox_id = activity_list[di].mailbox_id;

						if (!emcore_insert_auto_download_event(activity, &err)) {
							EM_DEBUG_EXCEPTION("emcore_insert_auto_download_event is failed [%d]", err);
							EM_SAFE_FREE(activity);
							if (err == EMAIL_ERROR_EVENT_QUEUE_FULL) goto CHECK_CONTINUE;
						}
					}
				}
			}

CHECK_CONTINUE:

			EM_SAFE_FREE(account_list);
			EM_SAFE_FREE(mailbox_list);
			EM_SAFE_FREE(activity_list);

			if (!emcore_is_auto_download_queue_empty()) {
				LEAVE_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);
				continue;
			}

			auto_download_thread_run = 0;
			//go to sleep when queue is empty
			SLEEP_CONDITION_VARIABLE(_auto_downalod_available_signal, *_auto_download_queue_lock);
			EM_DEBUG_LOG("Wake up by _auto_downalod_available_signal");
			LEAVE_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);
		} else {
			LEAVE_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);
			EM_DEBUG_LOG_DEV(">>>>>>>>>>>>>>> Got auto download event_data !!! <<<<<<<<<<<<<<<");
			int state1 = 0, state2 = 0, state3 = 0, wifi_status = 0;
			char uid_str[50] = {0, };
			emstorage_mail_tbl_t *mail = NULL;
			emstorage_mailbox_tbl_t *target_mailbox = NULL;
			email_account_t *account_ref = NULL;
			auto_download_thread_run = 1;

			if ((err = emnetwork_get_wifi_status(&wifi_status)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emnetwork_get_wifi_status failed [%d]", err);
			}

			EM_DEBUG_LOG("WIFI Status [%d]", wifi_status);

			if ((state1 = emcore_get_pbd_thd_state()) == true ||
					(state2 = emcore_is_event_queue_empty()) == false ||
					(state3 = emcore_is_send_event_queue_empty()) == false ||
					(wifi_status <= 1)) {

				EM_DEBUG_LOG("Auto Download Thread going to SLEEP");

				ENTER_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);
				auto_download_thread_run = 0;
				SLEEP_CONDITION_VARIABLE(_auto_downalod_available_signal, *_auto_download_queue_lock);
				EM_DEBUG_LOG("Wake up by _auto_downalod_available_signal");
				LEAVE_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);

				auto_download_thread_run = 1;
			}

			if (!(account_ref = emcore_get_account_reference(event_data->multi_user_name, event_data->account_id, false))) {
				EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", event_data->account_id);
				err = EMAIL_ERROR_INVALID_ACCOUNT;
				goto POP_HEAD;
			}

			if (!(account_ref->wifi_auto_download)) {
				EM_DEBUG_LOG("ACCOUNT[%d] AUTO DOWNLOAD MODE OFF", event_data->account_id);
				goto POP_HEAD;
			}

			if (account_ref) {
				emcore_free_account(account_ref);
				EM_SAFE_FREE(account_ref);
			}

			if ((err = emstorage_get_mailbox_by_id(event_data->multi_user_name, event_data->mailbox_id, &target_mailbox)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", err);
			}

			if (target_mailbox) {
				if (target_mailbox->mailbox_type != EMAIL_MAILBOX_TYPE_INBOX) {
					EM_DEBUG_LOG("Not INBOX mail, skip to next");
					emstorage_free_mailbox(&target_mailbox, 1, NULL);
					goto DELETE_ACTIVITY;
				}

				emstorage_free_mailbox(&target_mailbox, 1, NULL);
			}

			/* Handling storage full */
			if ((err = emcore_is_storage_full()) == EMAIL_ERROR_MAIL_MEMORY_FULL) {
				EM_DEBUG_EXCEPTION("Storage is full");

				ENTER_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);
				auto_download_thread_run = 0;
				SLEEP_CONDITION_VARIABLE(_auto_downalod_available_signal, *_auto_download_queue_lock);
				EM_DEBUG_LOG("Wake up by _auto_downalod_available_signal");
				LEAVE_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);

				continue;
			}

			snprintf(uid_str, sizeof(uid_str), "%ld", event_data->server_mail_id);
			if (!emstorage_get_maildata_by_servermailid(event_data->multi_user_name,
														uid_str,
														event_data->mailbox_id,
														&mail,
														true,
														&err) || !mail) {
				EM_DEBUG_EXCEPTION("emstorage_get_mail_data_by_servermailid failed : [%d]", err);
			} else {
				if (mail->body_download_status & EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED) {
					EM_DEBUG_LOG("fully downloaded mail");
				} else {
					EM_DEBUG_LOG("#####AUTO DOWNLOAD BODY: ACCOUNT_ID[%d] MAILBOX_ID[%d] MAIL_ID[%d] UID[%d] ACTIVITY[%d]#####",
							event_data->account_id, event_data->mailbox_id,
							event_data->mail_id, event_data->server_mail_id, event_data->activity_id);

					if (!emcore_gmime_download_body_sections(event_data->multi_user_name,
																NULL,
																event_data->account_id,
																event_data->mail_id,
																0,
																NO_LIMITATION,
																-1,
																0,
																1,
																&err))
						EM_DEBUG_EXCEPTION("emcore_gmime_download_body_sections failed - %d", err);
				}

				if (mail->attachment_count > 0) {
					int i = 0;
					int j = 0;
					int attachment_count = 0;
					emstorage_attachment_tbl_t *attachment_list = NULL;

					if ((err = emstorage_get_attachment_list(event_data->multi_user_name,
																event_data->mail_id,
																true,
																&attachment_list,
																&attachment_count)) != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);
					} else {
						for (i = 0; i < attachment_count; i++) {
							if (attachment_list[i].attachment_inline_content_status)
								continue;

							j++;

							if (attachment_list[i].attachment_save_status)
								continue;

							EM_DEBUG_LOG("#####AUTO DOWNLOAD ATTACHMENT[%d]: ACCOUNT_ID[%d] "
										"MAILBOX_ID[%d] MAIL_ID[%d] UID[%d] ACTIVITY[%d]#####",
										j, event_data->account_id, event_data->mailbox_id,
										event_data->mail_id, event_data->server_mail_id, event_data->activity_id);

							if (!emcore_gmime_download_attachment(event_data->multi_user_name,
																	event_data->mail_id,
																	j,
																	0,
																	-1,
																	1,
																	&err)) {
								EM_DEBUG_EXCEPTION("emcore_gmime_download_attachment failed [%d]", err);
								EM_DEBUG_LOG("Retry again");
								if (!emcore_gmime_download_attachment(event_data->multi_user_name,
																		event_data->mail_id,
																		j,
																		0,
																		-1,
																		1,
																		&err))
									EM_DEBUG_EXCEPTION("emcore_gmime_download_attachment failed [%d]", err);
							}
						}

						if (attachment_list)
							emstorage_free_attachment(&attachment_list, attachment_count, NULL);
					}
				}

				if (mail)
					emstorage_free_mail(&mail, 1, NULL);
			}

DELETE_ACTIVITY:
			if (!emcore_delete_auto_download_activity(event_data->multi_user_name,
														event_data->account_id,
														event_data->mail_id,
														event_data->activity_id,
														&err)) {
				EM_DEBUG_EXCEPTION("emcore_delete_auto_download_activity failed [%d]", err);
			}


POP_HEAD:
			/* free event itself */
			ENTER_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);
			started_event = g_queue_pop_head(g_auto_download_que);
			LEAVE_RECURSIVE_CRITICAL_SECTION(_auto_download_queue_lock);
			if (!started_event) {
				EM_DEBUG_EXCEPTION("Failed to g_queue_pop_head");
			} else {
				EM_SAFE_FREE(started_event->multi_user_name);
				EM_SAFE_FREE(started_event);
			}

			if (account_ref) {
				emcore_free_account(account_ref);
				EM_SAFE_FREE(account_ref);
			}
		}
	}

	if (!emstorage_close(&err))
		EM_DEBUG_EXCEPTION("emstorage_close falied [%d]", err);

	emcore_close_recv_stream_list();

	EM_DEBUG_FUNC_END();
	return SUCCESS;
}


INTERNAL_FUNC int emcore_insert_auto_download_job(char *multi_user_name,
													int account_id,
													int mailbox_id,
													int mail_id,
													int auto_download_on,
													char *uid,
													int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], maibox_id[%d], mail_id[%d], uid[%p]", account_id, mailbox_id, mail_id, uid);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int event_pushed = 0;
	email_event_auto_download *ad_event = NULL;

	if (account_id < FIRST_ACCOUNT_ID || mailbox_id <= 0 || mail_id < 0 || !uid) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ad_event = (email_event_auto_download *)em_malloc(sizeof(email_event_auto_download));

	if (!ad_event) {
		EM_DEBUG_EXCEPTION("em_mallocfailed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	ad_event->status = 0;
	ad_event->account_id = account_id;
	ad_event->mail_id = mail_id;
	ad_event->server_mail_id = strtoul(uid, NULL, 0);
	ad_event->mailbox_id = mailbox_id;
	ad_event->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

	if (!emcore_insert_auto_download_activity(ad_event, &(ad_event->activity_id), &err)) {
		EM_DEBUG_EXCEPTION("Inserting auto download activity failed with error[%d]", err);
		goto FINISH_OFF;
	} else {
		ret = true;

		if (!auto_download_on) {
			goto FINISH_OFF;
		}

		if (emcore_is_auto_download_queue_full()) {
			EM_DEBUG_LOG("Activity inserted only in DB .. Queue is Full");
		} else {
			ad_event->status = EMAIL_EVENT_STATUS_DIRECT;

			if (!emcore_insert_auto_download_event(ad_event, &err)) {
				EM_DEBUG_EXCEPTION("emcore_insert_auto_download_event is failed [%d]", err);
				goto FINISH_OFF;
			}
			event_pushed = 1;
		}
	}

FINISH_OFF:

	if (!event_pushed) {
		if (ad_event) {
			EM_SAFE_FREE(ad_event->multi_user_name);
			free(ad_event);
		}
	}

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_insert_auto_download_activity(email_event_auto_download *local_activity, int *activity_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("local_activity[%p], activity_id[%p], err_code[%p]", local_activity, activity_id, err_code);

	if (!local_activity || !activity_id) {
		EM_DEBUG_EXCEPTION("local_activity[%p], activity_id[%p] err_code[%p]", local_activity, activity_id, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int before_tr_begin = 0;

	if (!emstorage_begin_transaction(local_activity->multi_user_name, NULL, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_begin_transaction failed [%d]", err);
		before_tr_begin = 1;
		goto FINISH_OFF;
	}

	if (!emstorage_add_auto_download_activity(local_activity->multi_user_name, local_activity, activity_id, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_add_auto_download_activity failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(local_activity->multi_user_name, NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
			ret = false;
		}
	} else {	/*  ROLLBACK TRANSACTION; */
		if (!before_tr_begin && emstorage_rollback_transaction(local_activity->multi_user_name, NULL, NULL, NULL) == false)
			err = EMAIL_ERROR_DB_FAILURE;
	}

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


INTERNAL_FUNC int emcore_delete_auto_download_activity(char *multi_user_name, int account_id, int mail_id, int activity_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], err_code[%p]", account_id, mail_id, err_code);

	if (account_id < FIRST_ACCOUNT_ID || mail_id < 0) {
		EM_DEBUG_EXCEPTION("account_id[%d], mail_id[%d], err_code[%p]", account_id, mail_id, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int before_tr_begin = 0;

	if (!emstorage_begin_transaction(multi_user_name, NULL, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_begin_transaction failed [%d]", err);
		before_tr_begin = 1;
		goto FINISH_OFF;
	}

	if (!emstorage_delete_auto_download_activity(multi_user_name, account_id, mail_id, activity_id, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_delete_auto_download_activity failed [%d]", err);
		goto FINISH_OFF;
	} else if (err == EMAIL_ERROR_DATA_NOT_FOUND)
		err = EMAIL_ERROR_NONE;

	ret = true;

FINISH_OFF:

	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(multi_user_name, NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
			ret = false;
		}
	} else {	/*  ROLLBACK TRANSACTION; */
		if (!before_tr_begin && emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL) == false)
			err = EMAIL_ERROR_DB_FAILURE;
	}

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


