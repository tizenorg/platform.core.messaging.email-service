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
#include "email-core-account.h"
#include "email-core-alarm.h"
#include "email-debug-log.h"
#include "email-storage.h"
#include "email-utilities.h"


static int _emdaemon_get_polling_account_and_timeinterval(alarm_id_t  alarm_id, int *account_id, int *timer_interval);
static int _emdaemon_create_alarm(int input_account_id, int input_alarm_interval);

INTERNAL_FUNC int emdaemon_add_polling_alarm(int input_account_id, int input_alarm_interval_in_minutes)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id[%d] input_alarm_interval_in_minutes[%d]", input_account_id, input_alarm_interval_in_minutes);
	int err = EMAIL_ERROR_NONE;

	if(input_alarm_interval_in_minutes <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if((err = emdaemon_check_auto_polling_started(input_account_id)) != EMAIL_ERROR_ALARM_DATA_NOT_FOUND) {
		EM_DEBUG_EXCEPTION("polling alarm is already exist");
		goto FINISH_OFF;
	}

	if((err = _emdaemon_create_alarm(input_account_id, input_alarm_interval_in_minutes * 60)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_emdaemon_create_alarm failed[%d]", err);
		goto FINISH_OFF;
	}
FINISH_OFF:
	EM_DEBUG_FUNC_END("err[%d]", err);
	return  err;
}


INTERNAL_FUNC int emdaemon_remove_polling_alarm(int account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d]", account_id);
	int err = EMAIL_ERROR_NONE;

	/* delete from list */
	if((err = emcore_delete_alram_data_by_reference_id(EMAIL_ALARM_CLASS_AUTO_POLLING, account_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_delete_alram_data_by_reference_id failed[%d]", err);
	}

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

INTERNAL_FUNC int emdaemon_check_auto_polling_started(int account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d]", account_id);
	int err = EMAIL_ERROR_NONE;
	email_alarm_data_t *alarm_data = NULL;

	if((err = emcore_get_alarm_data_by_reference_id(EMAIL_ALARM_CLASS_AUTO_POLLING, account_id, &alarm_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_alarm_data_by_reference_id  failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

INTERNAL_FUNC int emdaemon_alarm_polling_cb(alarm_id_t alarm_id, void* user_param)
{
	EM_DEBUG_FUNC_BEGIN("alarm_id [%d] user_param [%d]", alarm_id, user_param);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int account_id = 0;
	int mailbox_id = 0;
	int timer_interval = 0;

	if(!_emdaemon_get_polling_account_and_timeinterval(alarm_id, &account_id, &timer_interval)) {
		EM_DEBUG_EXCEPTION("email_get_polling_account failed");
		return false;
	}

	/* delete from list */
	if ((err = emdaemon_remove_polling_alarm(account_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("email_get_polling_account failed [%d]", err);
		return false;
	}

	EM_DEBUG_LOG("target account_id [%d]",account_id);

	/* create alarm, for polling */
	if ((err = _emdaemon_create_alarm(account_id, timer_interval * 60)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_emdaemon_create_alarm failed [%d]", err);
		return false;
	}

	if (!emstorage_get_mailbox_id_by_mailbox_type(account_id,EMAIL_MAILBOX_TYPE_INBOX, &mailbox_id, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_name_by_mailbox_type failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emdaemon_sync_header(account_id, mailbox_id, NULL, &err))  {
		EM_DEBUG_EXCEPTION("emdaemon_sync_header falied [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;
FINISH_OFF :

	return ret;
}


static int _emdaemon_get_polling_account_and_timeinterval(alarm_id_t alarm_id, int *account_id, int *timer_interval)
{
	EM_DEBUG_FUNC_BEGIN("alarm_id [%d] account_id[%p] timer_interval[%p]", alarm_id, account_id, timer_interval);
	int err = EMAIL_ERROR_NONE;
	email_alarm_data_t *alarm_data = NULL;
	email_account_t *account = NULL;

	if(!alarm_id || !account_id) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (((err = emcore_get_alarm_data_by_alarm_id(alarm_id, &alarm_data)) != EMAIL_ERROR_NONE) || alarm_data == NULL) {
		EM_DEBUG_EXCEPTION("emcore_add_alarm failed [%d]",err);
		goto FINISH_OFF;
	}

	account = emcore_get_account_reference(alarm_data->reference_id);
	if (account == NULL) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]",err);
		goto FINISH_OFF;
	}

	*account_id     = alarm_data->reference_id;
	*timer_interval = account->check_interval;

FINISH_OFF:

	emcore_free_account(account);
	EM_SAFE_FREE(account); /*prevent 43342*/
	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}


static int _emdaemon_create_alarm(int input_account_id, int input_alarm_interval_in_second)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d] input_alarm_interval_in_second[%d]", input_account_id, input_alarm_interval_in_second);
	int err = EMAIL_ERROR_NONE;
	time_t current_time;
	time_t trigger_at_time;

	if(input_alarm_interval_in_second <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	time(&current_time);

	trigger_at_time = current_time + input_alarm_interval_in_second;

	if ((err = emcore_add_alarm(trigger_at_time, EMAIL_ALARM_CLASS_AUTO_POLLING, input_account_id, emdaemon_alarm_polling_cb, NULL)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_add_alarm failed [%d]",err);
		goto FINISH_OFF;
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

#endif

