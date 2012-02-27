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

/**
 *
 * This file contains functinality related to IMAP IDLE.
 * to interact with Email Engine.
 * @file		em_core-imap-idle.c
 * @author	
 * @version	0.1
 * @brief 		This file contains functionality to provide IMAP IDLE support in email-service. 
 */

#include <em-core-types.h>

#ifdef _FEATURE_IMAP_IDLE
#include <pthread.h>
#include <glib.h>
#include <openssl/ssl.h>
#include "c-client.h"
#include "lnx_inc.h"
#include "em-core-imap-idle.h"
#include "emf-dbglog.h"
#include "em-storage.h" 
#include "em-network.h"
#include "em-core-utils.h"
#include "em-core-mailbox.h"
#include "em-core-event.h"
#include "em-core-account.h"

static int idle_pipe[2] = {0, 0};

static int wait_on_pipe(int *pipe_fd)
{
	fd_set rfds;
	fd_set wfds;
	struct timeval timeout_value;
	int    ret_value;
	
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_SET(pipe_fd[0], &rfds);
	
	EM_DEBUG_LOG("wait...till dnet fills some data in pipe - %d", pipe_fd[0]);

	/* timeout_value.tv_sec = 10; */ /*  10 seconds. */
	timeout_value.tv_sec = 180; /*  160 + a seconds. */
	timeout_value.tv_usec = 0; 
	EM_DEBUG_LOG("wait_on_pipe()  :  timeout_value (%d) sec, (%d) usec", timeout_value.tv_sec, timeout_value.tv_usec);
	
	ret_value = select(pipe_fd[0]+1, &rfds, &wfds, NULL, &timeout_value);

	switch (ret_value){
		case 0:
			EM_DEBUG_LOG("wait_on_pipe()  :  select timer expired!!! ");
			break;
		case -1:
			EM_DEBUG_LOG("wait_on_pipe()  :  There is an error on calling select. ");
			break;
		default:
			EM_DEBUG_LOG("wait_on_pipe()  :  got response from DNET. ");
			break;
	}
	
	return ret_value;
}

static int read_from_pipe(int *pipe_fd)
{
	int activation = 0;
	
	read(pipe_fd[0], &activation, sizeof(int));
	
	return activation;
}

static void write_to_pipe(int *pipe_fd, int result)
{
	write(pipe_fd[1], (char *)&result, sizeof(int)); 
}


static void _idle_thread_cm_evt_cb(const NetEventInfo_t *event)
{
	EM_DEBUG_FUNC_BEGIN();
	
	/* This callback will be called when any event is recd from datacom */
	
	switch (event->Event) {
		/* Response from Datacom for PDP Activation Request */
		case NET_EVENT_OPEN_RSP: 
			if (event->Error == NET_ERR_NONE) {		/* Successful PDP Activation */
				EM_DEBUG_LOG("\t IMAP IDLE Successful PDP Activation");
				
				NetDevInfo_t devInfo;
				memset(&devInfo, 0x00, sizeof(devInfo));
				
				if (net_get_device_info(&devInfo) != NET_ERR_NONE) 
					EM_DEBUG_EXCEPTION("\t net_get_device_info failed - %d", event->Error);
				write_to_pipe(idle_pipe, 1);
			}
			else {		/* PDP Activation failure */
				EM_DEBUG_EXCEPTION("\t PDP Activation Failure %d", event->Error);
				write_to_pipe(idle_pipe, 0);
			}
			break;
		
		/* Response from Datacom for PDP Dectivation Request */
		case NET_EVENT_CLOSE_RSP: 
			if (event->Error == NET_ERR_NONE) {		/* Successful PDP Dectivation */
				EM_DEBUG_LOG("\t Successful PDP DeActivation");
				
				write_to_pipe(idle_pipe, 1);
			}
			else {		/* PDP Dectivation failure */
				EM_DEBUG_EXCEPTION("\t PDP DeActivation Failure %d", event->Error);
				
				if (event->Error == NET_ERR_TRANSPORT) 
                                {
					/*  TODO  :  add a process to deal an error */
					/* NetCMGetTransportError(&err_code); */
				}
				
				write_to_pipe(idle_pipe, 0);
			}
			break;
		
		case NET_EVENT_CLOSE_IND: 
			EM_DEBUG_LOG("NET_EVENT_CLOSE_IND recieved");
			break;
		
		case NET_EVENT_KILL_RSP: 
			break;
		
		case NET_EVENT_SUSPEND_IND: 
			/*  TODO */
			/*  think over what can we do... */
			break;
		
		case NET_EVENT_RESUME_IND: 
			/*  TODO */
			/*  think over what can we do... */
			break;
		
		default: 
			break;
	}
}


/*Definitions copied temperorily from ssl_unix.c */
#define SSLBUFLEN 8192

typedef struct ssl_stream {
  TCPSTREAM *tcpstream;		/* TCP stream */
  SSL_CTX *context;		/* SSL context */
  SSL *con;			/* SSL connection */
  int ictr;			/* input counter */
  char *iptr;			/* input pointer */
  char ibuf[SSLBUFLEN];		/* input buffer */
} SSLSTREAM;
/*Definitions copied temperorily from ssl_unix.c - end*/

thread_t imap_idle_thread = NULL;
int g_imap_idle_thread_alive = 0;


int em_core_imap_idle_thread_create(int accountID, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = false;
	int thread_error = -1;

#ifdef ENABLE_IMAP_IDLE_THREAD		

	g_imap_idle_thread_alive = 1;	
	THREAD_CREATE(imap_idle_thread, em_core_imap_idle_run, NULL, thread_error);
	if (thread_error != 0)
   	{
		EM_DEBUG_EXCEPTION("cannot make IMAP IDLE thread...");
		if (err_code != NULL) 
			*err_code = EMF_ERROR_UNKNOWN;
		g_imap_idle_thread_alive = 0;	
		goto FINISH_OFF;
   	}
	
	ret = true;
FINISH_OFF: 

#else /*  ENABLE_IMAP_IDLE_THREAD */
	EM_DEBUG_LOG(">>>>>>>> DISABLED IMAP IDLE THREAD");
#endif /*  ENABLE_IMAP_IDLE_THREAD */

	return ret;
}

/*
Need to identify various scenarios where thread needs to be killed
1. After the timer set to 10 min expires.
2. When No sim, thread is created and em_core_check_network_status() fails. 
*/
int em_core_imap_idle_thread_kill(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = true;
	int err = EMF_ERROR_NONE;
	
	EM_DEBUG_LOG(">>>>>>>>>>>>killing thread>>>>>>>>>");
	
	/* kill the thread */
	EM_DEBUG_LOG(">>>>>>>>>Before g_thread_exit");
	g_imap_idle_thread_alive = 0;
	EM_DEBUG_LOG(">>>>>>>>>After g_thread_exit");

	EM_DEBUG_LOG(">>>>>>>>>>>>>>>>>> Making imap idle NULL>>>>>>>>>>>>>>>>>");
	imap_idle_thread = NULL;
	EM_DEBUG_LOG(">>>>>>>>>>>>>>>>>> killed IMAP IDLE >>>>>>>>>>>>>>>>>");
	if (err_code)
		*err_code = err;
	return ret;
}



int em_core_imap_idle_run(int accountID)
{
	EM_DEBUG_FUNC_BEGIN();
	char *mailbox_list[3]; /* Temperory - To be modified to char ***/
	int mailbox_num = 0;
	emf_mailbox_t *mail_box_list = NULL;
	emf_mailbox_t *curr_mailbox = NULL;
	emf_mailbox_t *prev_mailbox = NULL;
	int counter = 0;
	emf_mailbox_tbl_t *local_mailbox = NULL;
	int err = EMF_ERROR_NONE;
	int flag = true; 
	int num = 0;
	emf_session_t *session = NULL;
	int ret = false;

	if ( !em_core_check_network_status(&err)) {
		EM_DEBUG_EXCEPTION("em_core_check_network_status failed [%d]", err);
		return ret;
	}
	/* Connect to DB */
	if (!em_storage_open(&err)) {
		EM_DEBUG_EXCEPTION("\t em_storage_open falied - %d", err);
		goto FINISH_OFF;
	}

	if (!em_core_get_empty_session(&session))  
			EM_DEBUG_EXCEPTION("\t em_core_get_empty_session failed...");
	
	/* get the list of mailbox name on which IDLE notifications are required. */
	/* currently all INBOXES of all accounts need to be notified */
	/* Dependent on GetMyIdentities for account names */

	/* For testing - mailbox_num and mailbox_list are hardcoded here */
	mailbox_num = 1;
	mailbox_list[0] = EM_SAFE_STRDUP("INBOX");

	/* make a list of mailboxes IDLING */
	for (counter = 0; counter < mailbox_num; counter++){
		EM_DEBUG_EXCEPTION(">>>> em_core_imap_idle_run 4 ");
		if (!em_storage_get_mailbox_by_name(accountID, 0,  mailbox_list[counter], &local_mailbox, true, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_get_mailbox_by_name failed - %d", err);
		}

		else {
			curr_mailbox = em_core_malloc(sizeof(emf_mailbox_t));

			curr_mailbox->account_id = local_mailbox->account_id;
			curr_mailbox->name = EM_SAFE_STRDUP(local_mailbox->mailbox_name);
			curr_mailbox->local = local_mailbox->local_yn;
			if (!em_core_imap_idle_connect_and_idle_on_mailbox(curr_mailbox, &err)) {
				EM_DEBUG_EXCEPTION("em_core_imap_idle_connect_and_idle_on_mailbox failed - %d", err);
				em_core_mailbox_free(&curr_mailbox, 1, NULL);
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
			em_storage_free_mailbox(&local_mailbox, 1, NULL);

	}

	em_core_clear_session(session);

	em_core_imap_idle_loop_start(mail_box_list, num, NULL);

	EM_DEBUG_EXCEPTION("IMAP IDLE ");

	ret = true;
FINISH_OFF: 
	if (!em_storage_close(&err)) {
		EM_DEBUG_EXCEPTION("\t em_storage_close falied - %d", err);
	}

	return ret;
	
}

int em_core_imap_idle_loop_start(emf_mailbox_t *mailbox_list,  int num, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	fd_set readfds;
	int maxfd = 0;
	int err = EMF_ERROR_NONE;
	int counter = 0;
	int select_result = 0;
	int ret = false;
	emf_mailbox_t *temp = NULL;
	struct timeval timeout;

	EM_DEBUG_EXCEPTION(">>>>>>> em_core_imap_idle_loop_start start ");
	if (!mailbox_list || !num) {
		EM_DEBUG_EXCEPTION("\t mailbox_list[%p], num[%d]", mailbox_list, num);
		
		err = EMF_ERROR_INVALID_PARAM;
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
					if (!em_core_imap_idle_parse_response_stream(temp, &err)) {
						EM_DEBUG_EXCEPTION(">>>> em_core_imap_idle_loop_start 6 ");
						em_core_mailbox_close(temp->account_id, temp->mail_stream);
						EM_DEBUG_EXCEPTION(">>>> em_core_imap_idle_loop_start 7 ");
						goto FINISH_OFF;
					}
					break; /* break for now - check if it is possible to get data on two sockets - shasikala.p */
				}
				temp = temp->next;
			}
		}

		else if (select_result == 0) /* Timeout occured */ {
			EM_DEBUG_EXCEPTION(">>>> em_core_imap_idle_loop_start 8 ");
			IMAPLOCAL *imap_local = NULL;
			char cmd[128], tag[32];
			char *p = NULL;
			/* send DONE Command */
			/* free all data here */
			for (counter = 0; counter < num; counter++) {
				EM_DEBUG_LOG(">>>> em_core_imap_idle_loop_start 9 ");
				if (temp && temp->mail_stream) {
					imap_local = ((MAILSTREAM *)temp->mail_stream)->local;

					sprintf(tag, "%08lx", 0xffffffff & (((MAILSTREAM *)temp->mail_stream)->gensym++));
    					sprintf(cmd, "%s DONE\015\012", tag);

					if (!imap_local->netstream || !net_sout(imap_local->netstream, cmd, (int)strlen(cmd)))
				    	{
				      		EM_DEBUG_EXCEPTION("network error - failed to DONE on Mailbox - %s ", temp->name);
				    	}
					else {
						while (imap_local->netstream) {
					        p = net_getline(imap_local->netstream);
					     	EM_DEBUG_EXCEPTION("p =[%s]", p);
							em_core_mailbox_close(temp->account_id, temp->mail_stream);
							break;
					    	}
					}
				}
		
				temp = temp->next;
			}
			
			
			/* kill idle thread */
			em_core_imap_idle_thread_kill(&err);
			break;
		}

		else {
			/*
			might happen that a socket desciptor passed to select got closed
			chack which got closed and make hold_connection 0
			*/
			EM_DEBUG_EXCEPTION(">>>>>> socket desciptor error :  No Data ");
			break;
		}

		select_result = 0;
	}

	ret = true;

	EM_DEBUG_LOG(">>>> em_core_imap_idle_loop_start 17  ");
FINISH_OFF: 
	if (err_code)
		*err_code = err;
	EM_DEBUG_EXCEPTION(">>>>>>> em_core_imap_idle_loop_start End ");
	return ret;
}


int em_core_imap_idle_insert_sync_event(emf_mailbox_t *mailbox, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int handle;
	
	if (!mailbox || mailbox->account_id <= 0) {
		EM_DEBUG_LOG("\t mailbox[%p]", mailbox);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	emf_event_t event_data;
	memset(&event_data, 0x00, sizeof(emf_event_t));

	event_data.type = EMF_EVENT_SYNC_HEADER;
	event_data.event_param_data_1 = mailbox ? EM_SAFE_STRDUP(mailbox->name)  :  NULL;
	event_data.event_param_data_3 = NULL;
	event_data.account_id = mailbox->account_id;
			
	if (!em_core_insert_event(&event_data, &handle, &err)) {
		EM_DEBUG_LOG("\t em_core_insert_event falied - %d", err);
				
		/* err = EMF_ERROR_DB_FAILURE; */
		goto FINISH_OFF;
	}

	ret = true;
FINISH_OFF: 
	if (err_code)
		*err_code = err;
	
	return ret;
}

/* connects to given mailbox. send idle and returns socket id */
int em_core_imap_idle_connect_and_idle_on_mailbox(emf_mailbox_t *mailbox, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();	
	void *mail_stream = NULL;
	char cmd[128], tag[32], *p;
	int socket_id = 0;
	int ret = 0;
	int err = EMF_ERROR_NONE;
	emf_account_t *ref_account = NULL;

	/* NULL param check */
	if (!mailbox) {
		EM_DEBUG_EXCEPTION("\t mailbox[%p]", mailbox);
		if (err_code)
			*err_code = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ref_account = em_core_get_account_reference(mailbox->account_id);
	if (!ref_account) {
		EM_DEBUG_EXCEPTION("\t em_core_get_account_reference failed - %d", mailbox->account_id);
		
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/* Open connection to mailbox */
	if (!em_core_mailbox_open(mailbox->account_id, mailbox->name, (void **)&mail_stream, &err) || !mail_stream) {
		EM_DEBUG_EXCEPTION("\t em_core_mailbox_open failed - %d", err);
		if (err_code)
			*err_code = err;
		goto FINISH_OFF;
	}

	IMAPLOCAL *imap_local = ((MAILSTREAM *)mail_stream)->local;
	NETSTREAM *net_stream = imap_local->netstream;
	
	/* check if ssl option is enabled on this account - shasikala.p */
	TCPSTREAM *tcp_stream = NULL;
	if (ref_account->use_security){
	SSLSTREAM *ssl_stream = net_stream->stream;
		tcp_stream = ssl_stream->tcpstream;
	}
	else{
		tcp_stream = net_stream->stream;
	}

	/* Get Socket ID */
	socket_id = ((TCPSTREAM *)tcp_stream)->tcpsi;

	sprintf(tag, "%08lx", 0xffffffff & (((MAILSTREAM *)mail_stream)->gensym++));
    	sprintf(cmd, "%s IDLE\015\012", tag);

	/* Send IDLE command */
	if (!imap_local->netstream || !net_sout(imap_local->netstream, cmd, (int)strlen(cmd)))
    	{
      		EM_DEBUG_EXCEPTION("network error - failed to IDLE on Mailbox - %s ", mailbox->name);
		if (err_code)
			*err_code = EMF_ERROR_IMAP4_IDLE_FAILURE;
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
	EM_SAFE_FREE(p); /* Prevent Defect - 18815 */

FINISH_OFF: 
	
	if (ret)		/* IMAP IDLE - SUCCESS */{
		mailbox->mail_stream = mail_stream;
		mailbox->hold_connection = socket_id; /* holds connection continuosly on the given socket_id */
	}
	else
	 	if (mail_stream) em_core_mailbox_close(mailbox->account_id, mail_stream);
	return ret;
}


int em_core_imap_idle_parse_response_stream(emf_mailbox_t *mailbox, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	char *p = NULL;
	int ret = false;
	
	/* NULL PARAM CHECK */
	if (!mailbox) {
		EM_DEBUG_EXCEPTION("\t mailbox[%p]", mailbox);
		if (err_code)
			*err_code = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	IMAPLOCAL *imap_local = NULL;

	if (!mailbox->mail_stream){
		EM_DEBUG_EXCEPTION("mail_stream is NULL");
		goto FINISH_OFF;
	}		

	imap_local = ((MAILSTREAM *)mailbox->mail_stream)->local;

	if (!imap_local){
		EM_DEBUG_EXCEPTION("imap_local is NULL");
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
			if (!em_core_imap_idle_insert_sync_event(mailbox, &err))
				EM_DEBUG_EXCEPTION("Syncing mailbox %s failed with err_code - %d", mailbox->name, err);
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
	return ret;
}
#endif /*  _FEATURE_IMAP_IDLE */
