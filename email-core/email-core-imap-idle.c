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
#include "lnx_inc.h"
#include "email-core-imap-idle.h"
#include "email-debug-log.h"
#include "email-storage.h" 
#include "email-network.h"
#include "email-core-utils.h"
#include "email-core-mailbox.h"
#include "email-core-event.h"
#include "email-core-account.h"


/*Definitions copied temporarily from ssl_unix.c */
#define SSLBUFLEN 8192

typedef struct ssl_stream {
  TCPSTREAM *tcpstream; /* TCP stream */
  SSL_CTX *context;     /* SSL context */
  SSL *con;             /* SSL connection */
  int ictr;             /* input counter */
  char *iptr;           /* input pointer */
  char ibuf[SSLBUFLEN]; /* input buffer */
} SSLSTREAM;
/*Definitions copied temporarily from ssl_unix.c - end*/

thread_t imap_idle_thread;
int g_imap_idle_thread_alive = 0;

void* emcore_imap_idle_run(void* thread_user_data);
static int emcore_imap_idle_parse_response_stream(email_mailbox_t *mailbox, int *err_code);
static int emcore_imap_idle_connect_and_idle_on_mailbox(email_mailbox_t *mailbox, int *err_code);

int emcore_create_imap_idle_thread(int account_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], err_code [%p]", account_id, err_code);
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int thread_error;

	g_imap_idle_thread_alive = 1;
	THREAD_CREATE(imap_idle_thread, emcore_imap_idle_run, NULL, thread_error);

	if (thread_error != 0) {
		EM_DEBUG_EXCEPTION("cannot make IMAP IDLE thread...");
		err = EMAIL_ERROR_UNKNOWN;
		g_imap_idle_thread_alive = 0;
		goto FINISH_OFF;
   	}

	ret = true;

FINISH_OFF:
	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/*
Need to identify various scenarios where thread needs to be killed
1. After the timer set to 10 min expires.
2. When No SIM, thread is created and emnetwork_check_network_status() fails.
*/
int emcore_kill_imap_idle_thread(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("err_code [%p]", err_code);
	int ret = true;
	int err = EMAIL_ERROR_NONE;

	EM_DEBUG_LOG("killing thread");

	/* kill the thread */
	EM_DEBUG_LOG("Before g_thread_exit");
	g_imap_idle_thread_alive = 0;
	EM_DEBUG_LOG("After g_thread_exit");

	EM_DEBUG_LOG("Making imap idle NULL");
	imap_idle_thread = 0;
	EM_DEBUG_LOG("killed IMAP IDLE");

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

int emcore_imap_idle_loop_start(email_mailbox_t *mailbox_list,  int num, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_list[%p], num[%d]", mailbox_list, num);
	fd_set readfds;
	int maxfd = 0;
	int err = EMAIL_ERROR_NONE;
	int counter = 0;
	int select_result = 0;
	int ret = false;
	email_mailbox_t *temp = NULL;
	struct timeval timeout;

	EM_DEBUG_EXCEPTION(">>>>>>> emcore_imap_idle_loop_start start ");
	if (!mailbox_list || !num) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* set timeout value to 10min */
	timeout.tv_sec = 10 * 60;
	timeout.tv_usec = 0;

	/* IMAP IDLE LOOP */
	while (1){
		EM_DEBUG_EXCEPTION(">>>>>>>>>>>IDLING>>>>>>>>>>>>");
		FD_ZERO(&readfds);
		temp = mailbox_list;
		for (counter = 0; counter < num; counter++) {
			FD_SET(temp->hold_connection, &readfds);
			if (temp->hold_connection > maxfd)
				maxfd = temp->hold_connection; 
			temp = temp->next;
		}
		maxfd++;
		temp = mailbox_list;
		
		select_result = select(maxfd, &readfds, NULL, NULL, &timeout);

		if (select_result > 0) /* Data coming on some socket */ {
			EM_DEBUG_EXCEPTION(">>>> Data Coming from Socket ");
			for (counter = 0; counter < num; counter++) {
				if (temp && FD_ISSET(temp->hold_connection, &readfds)) {
					if (!emcore_imap_idle_parse_response_stream(temp, &err)) {
						EM_DEBUG_EXCEPTION(">>>> emcore_imap_idle_loop_start 6 ");
						emcore_close_mailbox(temp->account_id, temp->mail_stream);
						EM_DEBUG_EXCEPTION(">>>> emcore_imap_idle_loop_start 7 ");
						goto FINISH_OFF;
					}
					break; /* break for now - check if it is possible to get data on two sockets - shasikala.p */
				}
				temp = temp->next;
			}
		}

		else if (select_result == 0) /* Timeout occurred */ {
			EM_DEBUG_EXCEPTION(">>>> emcore_imap_idle_loop_start 8 ");
			IMAPLOCAL *imap_local = NULL;
			char cmd[128], tag[32];
			char *p = NULL;
			/* send DONE Command */
			/* free all data here */
			for (counter = 0; counter < num; counter++) {
				EM_DEBUG_LOG(">>>> emcore_imap_idle_loop_start 9 ");
				if (temp && temp->mail_stream) {
					imap_local = ((MAILSTREAM *)temp->mail_stream)->local;

					sprintf(tag, "%08lx", 0xffffffff & (((MAILSTREAM *)temp->mail_stream)->gensym++));
    					sprintf(cmd, "%s DONE\015\012", tag);

					if (!imap_local->netstream || !net_sout(imap_local->netstream, cmd, (int)EM_SAFE_STRLEN(cmd))) {
						EM_DEBUG_EXCEPTION("network error - failed to DONE on Mailbox - %s ", temp->name);
					}
					else {
						while (imap_local->netstream) {
					        p = net_getline(imap_local->netstream);
					     	EM_DEBUG_EXCEPTION("p =[%s]", p);
							emcore_close_mailbox(temp->account_id, temp->mail_stream);
							break;
					    }
					}
				}
				temp = temp->next;
			}
			
			
			/* kill idle thread */
			emcore_kill_imap_idle_thread(&err);
			break;
		}

		else {
			/*
			might happen that a socket descriptor passed to select got closed
			check which got closed and make hold_connection 0
			*/
			EM_DEBUG_EXCEPTION(">>>>>> socket descriptor error :  No Data ");
			break;
		}

		select_result = 0;
	}

	ret = true;

	EM_DEBUG_LOG(">>>> emcore_imap_idle_loop_start 17  ");

FINISH_OFF: 

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

void* emcore_imap_idle_run(void* thread_user_data)
{
	EM_DEBUG_FUNC_BEGIN("thread_user_data [%p]", thread_user_data);
	char              *mailbox_list[3];
	int                mailbox_num = 0;
	int                err = EMAIL_ERROR_NONE;
	int                flag = true;
	int                num = 0;
	int                counter = 0;
	int                accountID = (int)thread_user_data;
	email_mailbox_t     *mail_box_list = NULL;
	email_mailbox_t     *curr_mailbox = NULL;
	email_mailbox_t     *prev_mailbox = NULL;
	emstorage_mailbox_tbl_t *local_mailbox = NULL;
	email_session_t     *session = NULL;

	if ( !emnetwork_check_network_status(&err)) {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
		goto FINISH_OFF;
	}
	/* Connect to DB */
	if (!emstorage_open(&err)) {
		EM_DEBUG_EXCEPTION("emstorage_open falied [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_get_empty_session(&session))
		EM_DEBUG_EXCEPTION("emcore_get_empty_session failed...");

	/* get the list of mailbox name on which IDLE notifications are required. */
	/* currently all INBOXES of all accounts need to be notified */
	/* Dependent on GetMyIdentities for account names */

	/* For testing - mailbox_num and mailbox_list are hardcoded here */
	mailbox_num     = 1;
	mailbox_list[0] = strdup("INBOX");

	/* make a list of mailboxes IDLING */
	for (counter = 0; counter < mailbox_num; counter++){
		EM_DEBUG_EXCEPTION(">>>> emcore_imap_idle_run 4 ");
		if (!emstorage_get_mailbox_by_name(accountID, 0,  mailbox_list[counter], &local_mailbox, true, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_name failed [%d]", err);
		}
		else {
			curr_mailbox             = em_malloc(sizeof(email_mailbox_t));
			curr_mailbox->account_id = local_mailbox->account_id;
			curr_mailbox->mailbox_name       = EM_SAFE_STRDUP(local_mailbox->mailbox_name);
			curr_mailbox->local      = local_mailbox->local_yn;
			if (!emcore_imap_idle_connect_and_idle_on_mailbox(curr_mailbox, &err)) {
				EM_DEBUG_EXCEPTION("emcore_imap_idle_connect_and_idle_on_mailbox failed [%d]", err);
				emcore_free_mailbox(curr_mailbox);
				EM_SAFE_FREE(curr_mailbox);
			}
			else {
				if (flag) {
					mail_box_list = curr_mailbox;
					prev_mailbox = curr_mailbox;
					flag = false;
					num++;
				}
				else {
					prev_mailbox->next = curr_mailbox;
					prev_mailbox = curr_mailbox;
					num++;
				}
			}
		}
		if (local_mailbox != NULL)
			emstorage_free_mailbox(&local_mailbox, 1, NULL);
	}

	emcore_clear_session(session);
	emcore_imap_idle_loop_start(mail_box_list, num, NULL);

FINISH_OFF:

	if (!emstorage_close(&err)) {
		EM_DEBUG_EXCEPTION("emstorage_close falied [%d]", err);
	}

	EM_DEBUG_FUNC_END();
	return NULL;
}

int emcore_imap_idle_insert_sync_event(email_mailbox_t *mailbox, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int handle;
	
	if (!mailbox || mailbox->account_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	email_event_t event_data = { 0 };

	event_data.type               = EMAIL_EVENT_SYNC_HEADER;
	event_data.event_param_data_1 = mailbox ? EM_SAFE_STRDUP(mailbox->mailbox_name)  :  NULL;
	event_data.event_param_data_3 = NULL;
	event_data.account_id         = mailbox->account_id;
			
	if (!emcore_insert_event(&event_data, &handle, &err)) {
		EM_DEBUG_EXCEPTION("emcore_insert_event failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/* connects to given mailbox. send idle and returns socket id */
static int emcore_imap_idle_connect_and_idle_on_mailbox(email_mailbox_t *mailbox, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox [%p], err_code [%p]", mailbox, err_code);
	void          *mail_stream = NULL;
	char           cmd[128] = { 0, };
	char           tag[32] = { 0, };
	char          *p = NULL;
	int            socket_id = 0;
	int            ret = 0;
	int            err = EMAIL_ERROR_NONE;
	email_account_t *ref_account = NULL;
	IMAPLOCAL     *imap_local = NULL;
	NETSTREAM     *net_stream = NULL;
	TCPSTREAM     *tcp_stream = NULL;

	/* NULL param check */
	if (!mailbox) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ref_account = emcore_get_account_reference(mailbox->account_id);
	if (!ref_account) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed. account_id[%d]", mailbox->account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/* Open connection to mailbox */
	if (!emcore_connect_to_remote_mailbox(mailbox->account_id, mailbox->mailbox_name, (void **)&mail_stream, &err) || !mail_stream) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}

	imap_local = ((MAILSTREAM *)mail_stream)->local;
	net_stream = imap_local->netstream;
	
	/* check if ssl option is enabled on this account - shasikala.p */

	if (ref_account->incoming_server_secure_connection){
		SSLSTREAM *ssl_stream = net_stream->stream;
		tcp_stream = ssl_stream->tcpstream;
	}
	else
		tcp_stream = net_stream->stream;

	/* Get Socket ID */
	socket_id = ((TCPSTREAM *)tcp_stream)->tcpsi;

	sprintf(tag, "%08lx", 0xffffffff & (((MAILSTREAM *)mail_stream)->gensym++));
	sprintf(cmd, "%s IDLE\015\012", tag);

	/* Send IDLE command */
	if (!imap_local->netstream || !net_sout(imap_local->netstream, cmd, (int)EM_SAFE_STRLEN(cmd))) {
		EM_DEBUG_EXCEPTION("network error - failed to IDLE on Mailbox - %s ", mailbox->mailbox_name);
		err = EMAIL_ERROR_IMAP4_IDLE_FAILURE;
		goto FINISH_OFF;
	}

	/* Get the response for IDLE command */
	while (imap_local->netstream){
		p = net_getline(imap_local->netstream);
		EM_DEBUG_LOG("p =[%s]", p);
		if (!strncmp(p, "+", 1)) {
			ret = 1;
			break;
		}
		else if (!strncmp(p, "*", 1)) {
			EM_SAFE_FREE(p);
			continue;
		}
		else {
			ret = 0;
			break;
		}
    }
	EM_SAFE_FREE(p);

FINISH_OFF: 
	
	if (ret) {
		/* IMAP IDLE - SUCCESS */
		mailbox->mail_stream     = mail_stream;
		mailbox->hold_connection = socket_id; /* holds connection continuously on the given socket_id */
	}
	else if (mail_stream)
		emcore_close_mailbox(mailbox->account_id, mail_stream);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

static int emcore_imap_idle_parse_response_stream(email_mailbox_t *mailbox, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox [%p], err_code [%p]", mailbox, err_code);
	int err = EMAIL_ERROR_NONE;
	char *p = NULL;
	int ret = false;
	IMAPLOCAL *imap_local = NULL;
	
	if (!mailbox || !mailbox->mail_stream) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	imap_local = ((MAILSTREAM *)mailbox->mail_stream)->local;

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
				goto FINISH_OFF;
			}
			else  {	
				if (!emcore_imap_idle_insert_sync_event(mailbox, &err))
					EM_DEBUG_EXCEPTION("Syncing mailbox %s failed with err_code [%d]", mailbox->mailbox_name, err);
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

	ret = true;

FINISH_OFF:

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
#endif /*  __FEATURE_IMAP_IDLE__ */
