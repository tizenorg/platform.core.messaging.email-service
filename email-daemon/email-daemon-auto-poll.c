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


#include "email-internal-types.h"

#ifdef __FEATURE_AUTO_POLLING__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>	/*  Needed for the definition of va_list */
#include <glib.h>

#include "email-types.h"
#include "email-daemon.h"
#include "email-daemon-auto-poll.h"
#include "email-core-utils.h"
#include "email-debug-log.h"
#include "email-storage.h"
#include "email-utilities.h"

typedef struct _emf_account_alarm_binder
{
           int account_id;
           alarm_id_t  alarm_id;
           int timer_interval;
} email_account_alarm_binder;

/*  global  list */
typedef struct _emf_account_alarm_binder_list_t
{
    email_account_alarm_binder account_alarm_binder;
    struct _emf_account_alarm_binder_list_t  *next;
}email_account_alarm_binder_list_t;

/* sowmya.kr@samsung.com, 23022010, Implementation of auto-polling feature */

email_account_alarm_binder_list_t *g_account_alarm_binder_list  = NULL;

static int _emdaemon_get_polling_alarm_and_timerinterval(int account_id, alarm_id_t *alarm_id, int *timer_interval);
static int _emdaemon_get_polling_account_and_timeinterval(alarm_id_t  alarm_id, int *account_id, int *timer_interval);
static int _emdaemon_create_alarm(int alarm_interval, alarm_id_t *p_alarm_id);
static int _emdaemon_add_to_account_alarm_binder_list(email_account_alarm_binder_list_t *p_account_alarm_binder);
static int _emdaemon_remove_from_account_alarm_binder_list(int account_id);
static int _emdaemon_update_account_alarm_binder_list(int account_id, alarm_id_t  alarm_id);

INTERNAL_FUNC int emdaemon_add_polling_alarm(int account_id, int alarm_interval)
{
	EM_DEBUG_FUNC_BEGIN();

	if(!account_id || (alarm_interval <= 0)) {
		EM_DEBUG_EXCEPTION("Invalid param");
		return false;
	}
	EM_DEBUG_EXCEPTION(" emdaemon_add_polling_alarm : account_id [%d]",account_id);
	if(emdaemon_check_auto_polling_started(account_id)) {
		EM_DEBUG_EXCEPTION("auto polling already started for account : return");
		return true;
	}

	alarm_id_t alarmID = 0;
	email_account_alarm_binder_list_t *p_account_alarm_binder = NULL;

	if(!_emdaemon_create_alarm(alarm_interval, &alarmID)) {
		EM_DEBUG_EXCEPTION("_emdaemon_create_alarm failed");
		return false;
	}
			
	p_account_alarm_binder = (email_account_alarm_binder_list_t*)em_malloc(sizeof(email_account_alarm_binder_list_t));
	if(!p_account_alarm_binder) {
		EM_DEBUG_EXCEPTION("malloc  Failed ");
		return false;
	}

	p_account_alarm_binder->account_alarm_binder.account_id = account_id;
	p_account_alarm_binder->account_alarm_binder.alarm_id = alarmID;
	p_account_alarm_binder->account_alarm_binder.timer_interval = alarm_interval;
	
	_emdaemon_add_to_account_alarm_binder_list(p_account_alarm_binder);

	return  true;	
}


INTERNAL_FUNC int emdaemon_remove_polling_alarm(int account_id)
{
	EM_DEBUG_FUNC_BEGIN();

	if(!account_id) {
		EM_DEBUG_EXCEPTION("Invalid param ");
		return false;
	}

	alarm_id_t  alarm_id = 0;
	int a_nErrorCode = 0, retval =0;

	if(!_emdaemon_get_polling_alarm_and_timerinterval(account_id,&alarm_id,NULL)) {
		EM_DEBUG_EXCEPTION("_emdaemon_get_polling_alarm_and_timerinterval failed");
		return false;
	}

	/* delete alarm */
	if (alarm_id > 0) {
		a_nErrorCode = alarmmgr_remove_alarm(alarm_id);

		EM_DEBUG_LOG("ErrorCode :%d, Return Value:%d ",a_nErrorCode,retval);

		if(!retval)
			return false;		
	}

	/* delete from list */
	if(!_emdaemon_remove_from_account_alarm_binder_list(account_id)) {
		EM_DEBUG_EXCEPTION("_emdaemon_remove_from_account_alarm_binder_list  failed");
		return false;
	}

	return true;
}

INTERNAL_FUNC int emdaemon_check_auto_polling_started(int account_id)
{
	EM_DEBUG_FUNC_BEGIN();

	if(g_account_alarm_binder_list == NULL) {
		EM_DEBUG_EXCEPTION("g_account_alarm_binder_list NULL ");
		return false;
	}

	email_account_alarm_binder_list_t  *p_temp = g_account_alarm_binder_list;
	int match_found = false;

	while(p_temp != NULL) {
		if(p_temp->account_alarm_binder.account_id == account_id) {
			EM_DEBUG_EXCEPTION("account match found : polling already started");			
			match_found = true;
			break;
		}
		p_temp = p_temp->next;
	}

	if(!match_found) {
		EM_DEBUG_EXCEPTION("account match not found : polling not started");
		return false;
	}
	return true;
}

INTERNAL_FUNC int emdaemon_alarm_polling_cb(alarm_id_t  alarm_id, void* user_param)
{
	EM_DEBUG_FUNC_BEGIN();

	email_mailbox_t mailbox = {0};
	int account_id = 0, err = EMAIL_ERROR_NONE, timer_interval =0, alarmID =0,ret = false;
	char* mailbox_name = NULL;

	time_t ct = time(&ct);
	struct tm* lt = localtime(&ct);
	
	if (lt) {
		EM_DEBUG_LOG( "Current Time :  [%d-%d-%d %d:%d:%d] ",
			lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
			lt->tm_hour, lt->tm_min, lt->tm_sec);				
	}

	if(!_emdaemon_get_polling_account_and_timeinterval(alarm_id,&account_id,&timer_interval)) {
		EM_DEBUG_EXCEPTION("email_get_polling_account failed");
		return false;
	}

	EM_DEBUG_EXCEPTION(" emdaemon_alarm_polling_cb : account_id [%d]",account_id);
	/* create alarm, for polling */	
	if(!_emdaemon_create_alarm(timer_interval,&alarmID)) {
		EM_DEBUG_EXCEPTION("_emdaemon_create_alarm failed");
		return false;
	}

	/*update alarm ID in list */
	/* delete from list */
	if(!_emdaemon_update_account_alarm_binder_list(account_id,alarmID)) {
		EM_DEBUG_EXCEPTION("_emdaemon_update_account_alarm_binder_list  failed");
		return false;
	}

	memset(&mailbox, 0x00, sizeof(email_mailbox_t));
	mailbox.account_id = account_id;

	if (!emstorage_get_mailbox_name_by_mailbox_type(mailbox.account_id,EMAIL_MAILBOX_TYPE_INBOX,&mailbox_name, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_name_by_mailbox_type failed [%d]", err);
		goto FINISH_OFF;
	}
	mailbox.mailbox_name = mailbox_name;

	if (!emdaemon_sync_header(account_id, mailbox.mailbox_id, NULL, &err))  {
		EM_DEBUG_EXCEPTION("emdaemon_sync_header falied [%d]", err);		
		goto FINISH_OFF;
	}

	ret = true;
FINISH_OFF :

	EM_SAFE_FREE(mailbox_name);

	return ret;
}

static int _emdaemon_get_polling_alarm_and_timerinterval(int account_id, alarm_id_t *alarm_id, int *timer_interval)
{
	EM_DEBUG_FUNC_BEGIN();

	if(!account_id || !alarm_id)
		return false;	

	if(g_account_alarm_binder_list == NULL) {
		EM_DEBUG_EXCEPTION("g_account_alarm_binder_list NULL ");
		return false;
	}

	email_account_alarm_binder_list_t  *p_temp = g_account_alarm_binder_list;
	int match_found = false;

	while(p_temp != NULL) {
		if(p_temp->account_alarm_binder.account_id == account_id) {
			EM_DEBUG_EXCEPTION("account match found ");
			*alarm_id =  p_temp->account_alarm_binder.alarm_id;
			
			if(timer_interval)
				*timer_interval = p_temp->account_alarm_binder.timer_interval;
			
			match_found = true;
			break;
		}

		p_temp = p_temp->next;
	}

	if(!match_found) {
		EM_DEBUG_EXCEPTION("account match not found ");
		return false;
	}


	return true;
}

static int _emdaemon_get_polling_account_and_timeinterval(alarm_id_t  alarm_id, int *account_id, int *timer_interval)
{
	EM_DEBUG_FUNC_BEGIN();

	if(!alarm_id || !account_id)
		return false;	

	if(g_account_alarm_binder_list == NULL) {
		EM_DEBUG_EXCEPTION("g_account_alarm_binder_list NULL ");
		return false;
	}

	email_account_alarm_binder_list_t  *p_temp = g_account_alarm_binder_list;
	int match_found = false;

	while(p_temp != NULL) {
		if(p_temp->account_alarm_binder.alarm_id == alarm_id) {
			EM_DEBUG_EXCEPTION("aalrm match found ");
			*account_id =  p_temp->account_alarm_binder.account_id;
			
			if(timer_interval)
				*timer_interval = p_temp->account_alarm_binder.timer_interval;
			
			match_found = true;
			break;
		}

		p_temp = p_temp->next;
	}

	if(!match_found) {
		EM_DEBUG_EXCEPTION("aalrm  match not found ");
		return false;
	}
	
	return true;
}

#define AUTO_POLL_DESTINATION 	"org.tizen.email-service"
static int _emdaemon_create_alarm(int alarm_interval, alarm_id_t *p_alarm_id)
{
	EM_DEBUG_FUNC_BEGIN();

	/* 	tzset(); */
	time_t ct = time(&ct);
	struct tm* lt = localtime(&ct);
	
	if (lt) {
		EM_DEBUG_LOG( "Current Time : [%d-%d-%d %d:%d:%d] ",
			lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
			lt->tm_hour, lt->tm_min, lt->tm_sec);				
	}

	/* 	alarm_info_t alarm_info = {0};	 */
	int a_nErrorCode = 0;
	int retval =0;
		
	if((alarm_interval <= 0) || !p_alarm_id) {
		EM_DEBUG_EXCEPTION("Invalid param ");
		return false;
	}
	
	/* 	time_t current_time = {0}; */
	/* 	struct tm current_tm = {0};	 */
	int error_code = 0;

	/* Fill alarm info */	
	/* 	int			timeFormat = 0; */ /*0 means 12hrs , 1 means 24hrs*/

	/* TO DO, need to findout header for DBG_MID_MSGPORTING_NORMAL and then we have to use this */
	/* error_code = vconf_get_int(DBG_MID_MSGPORTING_NORMAL, &timeFormat); */

	a_nErrorCode = alarmmgr_init(AUTO_POLL_DESTINATION);
	EM_DEBUG_LOG("ErrorCode :%d, Return Value:%d ",a_nErrorCode,retval);

	if(!retval)
		return false;
	
	a_nErrorCode = alarmmgr_set_cb(emdaemon_alarm_polling_cb, NULL);
	EM_DEBUG_LOG("ErrorCode :%d, Return Value:%d ",a_nErrorCode,retval);

	if(!retval)
		return false;	

	error_code = alarmmgr_add_alarm(ALARM_TYPE_VOLATILE, alarm_interval * 60 /*(sec)*/, ALARM_REPEAT_MODE_ONCE, AUTO_POLL_DESTINATION, p_alarm_id);
	
	EM_DEBUG_LOG("ErrorCode :%d,Return Value :%d ",error_code,retval);

	if(!retval)
		return false;	

	return true;
}

INTERNAL_FUNC int emdaemon_free_account_alarm_binder_list()
{
	EM_DEBUG_FUNC_BEGIN();

	email_account_alarm_binder_list_t *p = g_account_alarm_binder_list, *p_next = NULL; 	
	int a_nErrorCode = 0;
	
	/* delete alarm as well */
	while (p) {
		/* delete alarm */
		if (p->account_alarm_binder.alarm_id  > 0) {
			a_nErrorCode = alarmmgr_remove_alarm(p->account_alarm_binder.alarm_id);
		}

		p_next = p->next;
		EM_SAFE_FREE(p);
		p = p_next;
	}

	g_account_alarm_binder_list = NULL;
	
	return true;
}

static int  _emdaemon_add_to_account_alarm_binder_list(email_account_alarm_binder_list_t *p_account_alarm_binder)
{
	email_account_alarm_binder_list_t  *p_temp = NULL;

	if(!p_account_alarm_binder) {
		EM_DEBUG_EXCEPTION("Invalid param ");
		return false;
	}

	if(g_account_alarm_binder_list == NULL) {
		g_account_alarm_binder_list = p_account_alarm_binder;
		p_account_alarm_binder = NULL;
	}
	else {
		p_temp = g_account_alarm_binder_list;
		while(p_temp->next != NULL)
			p_temp = p_temp->next;

		p_temp->next = p_account_alarm_binder;
		p_account_alarm_binder = NULL;
	}

	return true;
}

static int  _emdaemon_remove_from_account_alarm_binder_list(int account_id)
{
	email_account_alarm_binder_list_t  *p_temp = g_account_alarm_binder_list,  *p_prev = NULL;
	int match_found = false;
	
	if(!account_id) {
		EM_DEBUG_EXCEPTION("Invalid param ");
		return false;
	}

	/* first node mattch */
	if(p_temp->account_alarm_binder.account_id == account_id) {
		/* match found */
		match_found = true;
		g_account_alarm_binder_list = p_temp->next;
		EM_SAFE_FREE(p_temp);
	}
	else {
		while(p_temp != NULL) {			
			if(p_temp->account_alarm_binder.account_id == account_id) {
				EM_DEBUG_EXCEPTION("account match found ");

				p_prev->next = p_temp->next;
				EM_SAFE_FREE(p_temp);
			
				match_found = true;
				break;
			}
			
			p_prev =  p_temp;
			p_temp = p_temp->next;
		}		
	}

	if(!match_found)
		return false;

	return true;
}

static int  _emdaemon_update_account_alarm_binder_list(int account_id, alarm_id_t  alarm_id)
{
	email_account_alarm_binder_list_t  *p_temp = g_account_alarm_binder_list;
	int match_found = false;

	if( !account_id || !alarm_id) {
		EM_DEBUG_EXCEPTION("Invalid param ");
		return false;
	}

	while(p_temp != NULL) {
		if(p_temp->account_alarm_binder.account_id == account_id) {
			EM_DEBUG_EXCEPTION("account match found ");
			/* update alarm id */
			p_temp->account_alarm_binder.alarm_id = alarm_id;
			match_found = true;
			break;
		}
		p_temp = p_temp->next;
	}

	if(!match_found) {
		EM_DEBUG_EXCEPTION("account match not found ");
		return false;
	}

	return true;
}
#endif

