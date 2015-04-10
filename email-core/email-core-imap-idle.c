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

/**
 *
 * This file contains functionality related to IMAP IDLE.
 * to interact with email-service.
 * @file		em_core-imap-idle.c
 * @author	
 * @version	0.1
 * @brief 		This file contains functionality to provide IMAP IDLE support in email-service. 
 */

#include <email-internal-types.h>

#ifdef __FEATURE_IMAP_IDLE__
#include <glib.h>
#include <openssl/ssl.h>
#include "c-client.h"
#include <sys/epoll.h>
#include "lnx_inc.h"
#include "email-core-imap-idle.h"
#include "email-debug-log.h"
#include "email-storage.h" 
#include "email-network.h"
#include "email-core-utils.h"
#include "email-core-mailbox.h"
#include "email-core-event.h"
#include "email-core-account.h"
#include "email-core-alarm.h"
#include "email-utilities.h"
#include "email-core-container.h"

/*Definitions copied temporarily from ssl_unix.c */
#define SSLBUFLEN 8192
#define MAX_EPOLL_EVENT 100

typedef struct ssl_stream {
  TCPSTREAM *tcpstream; /* TCP stream */
  SSL_CTX *context;     /* SSL context */
  SSL *con;             /* SSL connection */
  int ictr;             /* input counter */
  char *iptr;           /* input pointer */
  char ibuf[SSLBUFLEN]; /* input buffer */
} SSLSTREAM;
/*Definitions copied temporarily from ssl_unix.c - end*/

typedef struct _email_imap_idle_connection_info_t {
	int          account_id;
	int          mailbox_id;
	MAILSTREAM  *mail_stream;
	int          socket_fd;
	char        *multi_user_name;
} email_imap_idle_connection_info_t;

int       imap_idle_pipe_fd[2];
thread_t  imap_idle_thread_id;

static int emcore_get_connection_info_by_socket_fd(GList *input_imap_idle_task_list, int input_socket_fd, email_imap_idle_connection_info_t **output_connection_info)
{
	EM_DEBUG_FUNC_BEGIN("input_imap_idle_task_list[%p] input_socket_fd[%d] output_connection_info[%p]", input_imap_idle_task_list, input_socket_fd, output_connection_info);
	int err = EMAIL_ERROR_NONE;
	email_imap_idle_connection_info_t *connection_info = NULL;
	GList *index = NULL;

	if (input_imap_idle_task_list == NULL || output_connection_info == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	index = g_list_first(input_imap_idle_task_list);

	while(index) {
		connection_info = index->data;
		if(connection_info && connection_info->socket_fd == input_socket_fd) {
			break;
		}
		index = g_list_next(index);
	}

	if(connection_info)
		*output_connection_info = connection_info;
	else
		err = EMAIL_ERROR_DATA_NOT_FOUND;

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static int emcore_clear_old_connections(int input_epoll_fd, GList **input_imap_idle_task_list)
{
	EM_DEBUG_FUNC_BEGIN("input_epoll_fd[%d] input_imap_idle_task_list[%p] ", input_epoll_fd, input_imap_idle_task_list);
	int err = EMAIL_ERROR_NONE;
	email_imap_idle_connection_info_t *connection_info = NULL;
	struct epoll_event ev = {0};
	GList *index = NULL;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (input_imap_idle_task_list == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	index = g_list_first(*input_imap_idle_task_list);

	while(index) {
		connection_info = index->data;

		/* Removes all fd from epoll_fd */
		ev.events  = EPOLLIN;
		ev.data.fd = connection_info->socket_fd;

		if (epoll_ctl(input_epoll_fd, EPOLL_CTL_DEL, connection_info->socket_fd, &ev) == -1) {
			EM_DEBUG_LOG("epoll_ctl failed: %s[%d]", EM_STRERROR(errno_buf), errno);
		}

		/* Close connection */
		connection_info->mail_stream = mail_close(connection_info->mail_stream);
		EM_SAFE_FREE(connection_info->multi_user_name);
		EM_SAFE_FREE(connection_info);

		index = g_list_next(index);
	}

	g_list_free(*input_imap_idle_task_list);
	*input_imap_idle_task_list = NULL;

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static int emcore_imap_idle_insert_sync_event(char *multi_user_name, int input_account_id, int input_mailbox_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int handle;
	email_event_t *event_data = NULL;

	if (input_account_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	event_data                     = em_malloc(sizeof(email_event_t));
	event_data->type               = EMAIL_EVENT_SYNC_HEADER;
	event_data->event_param_data_5 = input_mailbox_id;
	event_data->account_id         = input_account_id;
	event_data->multi_user_name    = EM_SAFE_STRDUP(multi_user_name);

	if (!emcore_insert_event(event_data, &handle, &err)) {
		EM_DEBUG_EXCEPTION("emcore_insert_event failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

static int emcore_parse_imap_idle_response(char *multi_user_name, int input_account_id, int input_mailbox_id, MAILSTREAM *input_mailstream, int input_socket_fd)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id[%d] input_mailbox_id [%d] input_mailstream[%p] input_socket_fd[%d]", input_account_id, input_mailbox_id, input_mailstream, input_socket_fd);
	int err = EMAIL_ERROR_NONE;
	char *p = NULL;
	IMAPLOCAL *imap_local = NULL;

	if (!input_mailstream) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	imap_local = input_mailstream->local;

	if (!imap_local){
		EM_DEBUG_EXCEPTION("imap_local is NULL");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	while (imap_local->netstream){
		p = net_getline(imap_local->netstream);
		if (p && !strncmp(p, "*", 1)) {
			EM_DEBUG_LOG("p is [%s]", p);
			if (p+1 && p+2 && p+3 && p+4 && !strncmp(p+2, "BYE", 3)) {
				EM_DEBUG_LOG("BYE connection from server");
				EM_SAFE_FREE(p);
				err = EMAIL_ERROR_IMAP4_IDLE_FAILURE;
				goto FINISH_OFF;
			}
			else  {
				if (strstr(p, "EXIST") != NULL) {
					if (!emcore_imap_idle_insert_sync_event(multi_user_name, input_account_id, input_mailbox_id, &err))
						EM_DEBUG_EXCEPTION_SEC("Syncing mailbox[%d] failed with err_code [%d]", input_mailbox_id, err);
				}
				else
					EM_DEBUG_LOG("Skipped this message");
				EM_SAFE_FREE(p);
				break;
			}
		}
		else if (p && (!strncmp(p, "+", 1))) {
			/* Bad response from server */
			EM_DEBUG_LOG("p is [%s]", p);
			EM_SAFE_FREE(p);
			break;
		}
		else {
			EM_DEBUG_LOG("In else part");
			break;
		}
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

/* connects to given mailbox. send idle and returns socket id */
static int emcore_connect_and_idle_on_mailbox(char *multi_user_name, GList **input_connection_list, email_account_t *input_account, email_mailbox_t *input_mailbox, MAILSTREAM **output_mailstream, int *output_socket_fd)
{
	EM_DEBUG_FUNC_BEGIN("input_connection_list [%p] input_account [%p] input_mailbox[%p] output_mailstream [%p] output_socket_fd [%p]", input_connection_list, input_account, input_mailbox, output_mailstream, output_socket_fd);
	void            *mail_stream = NULL;
	char             cmd[128] = { 0, };
	char             tag[32] = { 0, };
	char            *p = NULL;
	int              socket_fd = 0;
	int              err = EMAIL_ERROR_NONE;
	email_session_t *session = NULL;
	IMAPLOCAL       *imap_local = NULL;
	NETSTREAM       *net_stream = NULL;
	TCPSTREAM       *tcp_stream = NULL;
	GList           *imap_idle_connection_list = NULL;
	email_imap_idle_connection_info_t *connection_info = NULL;

	if (input_connection_list == NULL || input_account == NULL || input_mailbox == NULL || output_mailstream == NULL || output_socket_fd == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	imap_idle_connection_list = *input_connection_list;

	if (!emcore_get_empty_session(&session))
		EM_DEBUG_EXCEPTION("emcore_get_empty_session failed...");

	/* Open connection to mailbox */
	if (!emcore_connect_to_remote_mailbox(multi_user_name, input_mailbox->account_id, input_mailbox->mailbox_id, (void **)&mail_stream, &err) || !mail_stream) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}

	imap_local = ((MAILSTREAM *)mail_stream)->local;
	net_stream = imap_local->netstream;

	/* check if ssl option is enabled on this account - shasikala.p */

	if (input_account->incoming_server_secure_connection){
		SSLSTREAM *ssl_stream = net_stream->stream;
		tcp_stream = ssl_stream->tcpstream;
	}
	else
		tcp_stream = net_stream->stream;

	/* Get Socket ID */
	socket_fd = ((TCPSTREAM *)tcp_stream)->tcpsi;

	sprintf(tag, "%08lx", 0xffffffff & (((MAILSTREAM *)mail_stream)->gensym++));
	sprintf(cmd, "%s IDLE\015\012", tag);

	/* Send IDLE command */
	if (!imap_local->netstream || !net_sout(imap_local->netstream, cmd, (int)EM_SAFE_STRLEN(cmd))) {
		EM_DEBUG_EXCEPTION_SEC("network error - failed to IDLE on Mailbox - %s ", input_mailbox->mailbox_name);
		err = EMAIL_ERROR_IMAP4_IDLE_FAILURE;
		goto FINISH_OFF;
	}

	/* Get the response for IDLE command */
	while (imap_local->netstream){
		p = net_getline(imap_local->netstream);

		EM_DEBUG_LOG("p =[%s]", p);

		if (p && !strncmp(p, "+", 1)) {
			EM_DEBUG_LOG("OK. Go.");
			break;
		}
		else if (p && !strncmp(p, "*", 1)) {
			EM_SAFE_FREE(p);
			continue;
		}
		else {
			EM_DEBUG_LOG("Unsuspected response.");
			err = EMAIL_ERROR_IMAP4_IDLE_FAILURE;
			break;
		}
    }
	EM_SAFE_FREE(p);

	connection_info = em_malloc(sizeof(email_imap_idle_connection_info_t));
	if (connection_info == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	connection_info->account_id      = input_account->account_id;
	connection_info->mailbox_id      = input_mailbox->mailbox_id;
	connection_info->mail_stream     = mail_stream;
	connection_info->socket_fd       = socket_fd;
	connection_info->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

	imap_idle_connection_list = g_list_append(imap_idle_connection_list, (gpointer)connection_info);

FINISH_OFF:

	if (err == EMAIL_ERROR_NONE) {
		/* IMAP IDLE - SUCCESS */
//		*output_mailstream     = mail_stream;
		*output_socket_fd      = socket_fd;
		*input_connection_list = imap_idle_connection_list;
	}
	else if (mail_stream)
		mail_stream = mail_close (mail_stream);

	emcore_clear_session(session);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static int emcore_imap_idle_cb(email_alarm_data_t *alarm_data, void *user_parameter)
{
	EM_DEBUG_FUNC_BEGIN("alarm_data [%p] user_parameter [%p]", alarm_data, user_parameter);
	int err = EMAIL_ERROR_NONE;

	emcore_refresh_imap_idle_thread();

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static int emcore_refresh_alarm_for_imap_idle(char *multi_user_name)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int account_count = 0;
	int auto_sync_account_count = 0;
	time_t current_time;
	time_t trigger_at_time;
	email_account_t *account_ref_list = NULL;

	/* Check whether the alarm is already existing */
	if(emcore_check_alarm_by_class_id(EMAIL_ALARM_CLASS_IMAP_IDLE) == EMAIL_ERROR_NONE) {
		EM_DEBUG_LOG("Already exist. Remove old one.");
		emcore_delete_alram_data_by_reference_id(EMAIL_ALARM_CLASS_IMAP_IDLE, 0);
	}

	if ((err = emcore_get_account_reference_list(multi_user_name, &account_ref_list, &account_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_LOG("emcore_get_account_reference_list failed [%d]", err);
		goto FINISH_OFF;
	}

	for (i = 0; i < account_count; i++) {
		if (account_ref_list[i].incoming_server_type == EMAIL_SERVER_TYPE_IMAP4 && (account_ref_list[i].check_interval == 0 || account_ref_list[i].peak_interval == 0)) {
			auto_sync_account_count++;
		}
	}

	if(account_ref_list)
		emcore_free_account_list(&account_ref_list, account_count, NULL);

	if (auto_sync_account_count) {
		time(&current_time);
		trigger_at_time = current_time + (29 * 60);

		if ((err = emcore_add_alarm(multi_user_name, trigger_at_time, EMAIL_ALARM_CLASS_IMAP_IDLE, 0, emcore_imap_idle_cb, NULL)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_add_alarm failed [%d]",err);
		}
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

void* emcore_imap_idle_worker(void* thread_user_data)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int i = 0, j = 0;
	int event_num = 0;
	int epoll_fd = 0;
	int socket_fd = 0;
	int signal_from_pipe = 0;
	int refresh_connection_flag = 1;
	int account_count = 0;
	char errno_buf[ERRNO_BUF_SIZE] = {0};
	struct epoll_event ev = {0};
	struct epoll_event events[MAX_EPOLL_EVENT] = {{0}, };
	MAILSTREAM *mail_stream = NULL;
	email_mailbox_t inbox_mailbox = {0};
	email_account_t *account_ref_list = NULL;
	GList *imap_idle_connection_list = NULL;
	GList *zone_name_list = NULL;
	GList *node = NULL;
	email_imap_idle_connection_info_t *connection_info = NULL;

	EM_DEBUG_LOG("emcore_imap_idle_worker start ");

	if (pipe(imap_idle_pipe_fd) == -1) {
		EM_DEBUG_EXCEPTION("pipe failed");
		err = EMAIL_ERROR_UNKNOWN;
		goto FINISH_OFF;
	}

	epoll_fd = epoll_create(MAX_EPOLL_EVENT);

	if (epoll_fd < 0) {
		EM_DEBUG_CRITICAL_EXCEPTION("epoll_create failed: %s[%d]", EM_STRERROR(errno_buf), errno);
		err = EMAIL_ERROR_IMAP4_IDLE_FAILURE;
		goto FINISH_OFF;
	}

	ev.events  = EPOLLIN;
	ev.data.fd = imap_idle_pipe_fd[0];

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, imap_idle_pipe_fd[0], &ev) == -1) {
		EM_DEBUG_EXCEPTION("epoll_ctl failed: %s[%d]", EM_STRERROR(errno_buf), errno);
	}

	EM_DEBUG_LOG("Enter imap idle loop");
	while (1) {
		if (refresh_connection_flag) {

			EM_DEBUG_LOG("Clear old connections");
			emcore_clear_old_connections(epoll_fd, &imap_idle_connection_list);

			EM_DEBUG_LOG("Getting account list");
			if ((err = emcore_get_zone_name_list(&zone_name_list)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_get_zone_name_list failed : err[%d]", err);
			}

			if (err != EMAIL_ERROR_NONE) {
				if ((err = emcore_get_account_reference_list(NULL, &account_ref_list, &account_count)) != EMAIL_ERROR_NONE) 
					EM_DEBUG_LOG("emcore_get_account_reference_list failed [%d]", err);

				for (i = 0; i < account_count; i++) {
					if (account_ref_list[i].incoming_server_type != EMAIL_SERVER_TYPE_IMAP4 || (account_ref_list[i].check_interval != 0 && account_ref_list[i].peak_interval != 0)) {
						EM_DEBUG_LOG("account_id[%d] is not for auto sync", account_ref_list[i].account_id);
						continue;
					}

					/* TODO: peak schedule handling */

					memset(&inbox_mailbox, 0, sizeof(email_mailbox_t));

					if (!emcore_get_mailbox_by_type(NULL, account_ref_list[i].account_id, EMAIL_MAILBOX_TYPE_INBOX, &inbox_mailbox, &err)) {
						EM_DEBUG_EXCEPTION("emcore_get_mailbox_by_type failed [%d]", err);
						continue;
					}

					EM_DEBUG_LOG("Connect to IMAP server");
					err = emcore_connect_and_idle_on_mailbox(NULL, &imap_idle_connection_list, &(account_ref_list[i]), &inbox_mailbox, &mail_stream, &socket_fd);

					emcore_free_mailbox(&inbox_mailbox);

					if (err != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("emcore_connect_and_idle_on_mailbox failed [%d]", err);
						continue;
					}

					ev.events  = EPOLLIN;
					ev.data.fd = socket_fd;

					EM_DEBUG_LOG("Add listener for socket buffer");
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) == -1) {
						EM_DEBUG_EXCEPTION("epoll_ctl failed: %s[%d]", EM_STRERROR(errno_buf), errno);
					}
				}

				if(account_ref_list)
					emcore_free_account_list(&account_ref_list, account_count, NULL);

				EM_DEBUG_LOG(" Delete old an alarm and add an alarm to refresh connections every 29min");
				emcore_refresh_alarm_for_imap_idle(NULL);

				refresh_connection_flag = 0;
			} else {
				node = g_list_first(zone_name_list);
				while (node != NULL) {
					if (!node->data)
						node = g_list_next(node);

					EM_DEBUG_LOG("Data name of node : [%s]", node->data);

					if ((err = emcore_get_account_reference_list(node->data, &account_ref_list, &account_count)) != EMAIL_ERROR_NONE) {
						EM_DEBUG_LOG("emcore_get_account_reference_list failed [%d]", err);
					}

					for (i = 0; i < account_count; i++) {
						if (account_ref_list[i].incoming_server_type != EMAIL_SERVER_TYPE_IMAP4 || (account_ref_list[i].check_interval != 0 && account_ref_list[i].peak_interval != 0)) {
							EM_DEBUG_LOG("account_id[%d] is not for auto sync", account_ref_list[i].account_id);
							continue;
						}

						/* TODO: peak schedule handling */

						memset(&inbox_mailbox, 0, sizeof(email_mailbox_t));

						if (!emcore_get_mailbox_by_type(node->data, account_ref_list[i].account_id, EMAIL_MAILBOX_TYPE_INBOX, &inbox_mailbox, &err)) {
							EM_DEBUG_EXCEPTION("emcore_get_mailbox_by_type failed [%d]", err);
							continue;
						}

						EM_DEBUG_LOG("Connect to IMAP server");
						err = emcore_connect_and_idle_on_mailbox(node->data, &imap_idle_connection_list, &(account_ref_list[i]), &inbox_mailbox, &mail_stream, &socket_fd);

						emcore_free_mailbox(&inbox_mailbox);

						if (err != EMAIL_ERROR_NONE) {
							EM_DEBUG_EXCEPTION("emcore_connect_and_idle_on_mailbox failed [%d]", err);
							continue;
						}

						ev.events  = EPOLLIN;
						ev.data.fd = socket_fd;

						EM_DEBUG_LOG("Add listener for socket buffer");
						if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) == -1) {
							EM_DEBUG_EXCEPTION("epoll_ctl failed: %s[%d]", EM_STRERROR(errno_buf), errno);
						}
					}

					if(account_ref_list)
						emcore_free_account_list(&account_ref_list, account_count, NULL);

					EM_DEBUG_LOG(" Delete old an alarm and add an alarm to refresh connections every 29min");
					emcore_refresh_alarm_for_imap_idle(node->data);

					EM_SAFE_FREE(node->data);
					node = g_list_next(node);
				}
				g_list_free(zone_name_list);

				refresh_connection_flag = 0;
			}
		}

		EM_DEBUG_LOG("Waiting.......");
		event_num = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENT, -1);
		EM_DEBUG_LOG("epoll_wait returns [%d]", event_num);

		if (event_num > 0)  {
			EM_DEBUG_LOG(">>>> Data coming from socket or pipe ");
			for (j = 0; j < event_num; j++) {
				int event_fd = events[j].data.fd;

				EM_DEBUG_LOG("event_fd [%d]", event_fd);

				if (event_fd == imap_idle_pipe_fd[0]) {
					EM_DEBUG_LOG(">>>> Data coming from pipe ");
					if (read(imap_idle_pipe_fd[0], (void*)&signal_from_pipe, sizeof(signal_from_pipe)) != -1) {
						EM_DEBUG_LOG("Refresh IMAP connections");
						refresh_connection_flag = 1;
						continue;
					}
				}
				else {
					if ((err = emcore_get_connection_info_by_socket_fd(imap_idle_connection_list, event_fd, &connection_info)) != EMAIL_ERROR_NONE) {
						EM_DEBUG_LOG("emcore_get_connection_info_by_socket_fd couldn't find proper connection info. [%d]", err);
						continue;
					}

					if ((err = emcore_parse_imap_idle_response(connection_info->multi_user_name, 
															connection_info->account_id, 
															connection_info->mailbox_id, 
															connection_info->mail_stream, 
															connection_info->socket_fd)) != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("emcore_parse_imap_idle_response failed. [%d] ", err);
						refresh_connection_flag = 1;
//						mail_stream = mail_close (mail_stream);
						continue;
					}
				}
			}
		}
	}

	EM_DEBUG_LOG("Exit imap idle loop");

FINISH_OFF:
	emcore_free_mailbox(&inbox_mailbox);

	EM_DEBUG_LOG("Close pipes");

	close(imap_idle_pipe_fd[0]);
	close(imap_idle_pipe_fd[1]);

	imap_idle_thread_id = 0;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return NULL;
}


INTERNAL_FUNC int emcore_create_imap_idle_thread()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int thread_error;

	if (imap_idle_thread_id) {
		EM_DEBUG_EXCEPTION("IMAP IDLE thread already exists.");
		err = EMAIL_ERROR_ALREADY_EXISTS;
		goto FINISH_OFF;

	}

	THREAD_CREATE(imap_idle_thread_id, emcore_imap_idle_worker, NULL, thread_error);

	if (thread_error != 0) {
		EM_DEBUG_EXCEPTION("cannot make IMAP IDLE thread...");
		err = EMAIL_ERROR_UNKNOWN;
		goto FINISH_OFF;
   	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_refresh_imap_idle_thread()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int signal = 1;

	write(imap_idle_pipe_fd[1], (char*) &signal, sizeof (signal));

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
#endif /*  __FEATURE_IMAP_IDLE__ */
