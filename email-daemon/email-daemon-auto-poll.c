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
#include "email-network.h"
#include "email-utilities.h"

#ifdef __FEATURE_DPM__
extern int g_dpm_policy_status;
#endif /* __FEATURE_DPM__ */

static int _emdaemon_get_polling_account_and_timeinterval(email_alarm_data_t *alarm_data, int *account_id, int *timer_interval);
static int _emdaemon_create_alarm_for_auto_polling(char *multi_user_name, int input_account_id);

INTERNAL_FUNC int emdaemon_add_polling_alarm(char *multi_user_name, int input_account_id)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id[%d]", input_account_id);
	int err = EMAIL_ERROR_NONE;

	if (input_account_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emdaemon_check_auto_polling_started(input_account_id)) != EMAIL_ERROR_ALARM_DATA_NOT_FOUND) {
		EM_DEBUG_EXCEPTION("polling alarm is already exist");
		goto FINISH_OFF;
	}

	if ((err = _emdaemon_create_alarm_for_auto_polling(multi_user_name, input_account_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_emdaemon_create_alarm_for_auto_polling failed[%d]", err);
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
	if ((err = emcore_delete_alram_data_by_reference_id(EMAIL_ALARM_CLASS_AUTO_POLLING, account_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_LOG("emcore_delete_alram_data_by_reference_id return [%d]", err);
	}

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

INTERNAL_FUNC int emdaemon_check_auto_polling_started(int account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d]", account_id);
	int err = EMAIL_ERROR_NONE;
	email_alarm_data_t *alarm_data = NULL;

	if ((err = emcore_get_alarm_data_by_reference_id(EMAIL_ALARM_CLASS_AUTO_POLLING, account_id, &alarm_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_LOG("emcore_get_alarm_data_by_reference_id error [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

INTERNAL_FUNC int emdaemon_alarm_polling_cb(email_alarm_data_t *alarm_data, void* user_param)
{
	EM_DEBUG_FUNC_BEGIN("alarm_data [%p] user_param [%d]", alarm_data, user_param);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int handle = 0;
	int account_id = 0;
	int mailbox_id = 0;
	int timer_interval = 0;
	int roaming_status = 0;
	int wifi_status = 0;
	email_account_t *ref_account = NULL;

	if (!_emdaemon_get_polling_account_and_timeinterval(alarm_data, &account_id, &timer_interval)) {
		EM_DEBUG_EXCEPTION("email_get_polling_account failed");
		return false;
	}

	EM_DEBUG_LOG("target account_id [%d]", account_id);
	/* create alarm, for polling */
	if ((err = _emdaemon_create_alarm_for_auto_polling(alarm_data->multi_user_name, account_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_emdaemon_create_alarm_for_auto_polling failed [%d]", err);
		return false;
	}

	#ifdef __FEATURE_DPM__
		if(g_dpm_policy_status == false){
			EM_DEBUG_EXCEPTION("dpm policy not allowed");
			err = EMAIL_ERROR_DPM_RESTRICTED_MODE;
			goto FINISH_OFF;
		}
	#endif /* __FEATURE_DPM__ */



	ref_account = emcore_get_account_reference(alarm_data->multi_user_name, account_id, false);
	if (ref_account == NULL) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed");
		err = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (ref_account->sync_disabled) {
		err = EMAIL_ERROR_ACCOUNT_SYNC_IS_DISALBED;
		EM_DEBUG_LOG("Sync disabled for this account. Do not sync.");
		goto FINISH_OFF;
	}

	if ((err = emnetwork_get_roaming_status(&roaming_status)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emnetwork_get_roaming_status failed [%d]", err);
		goto FINISH_OFF;
	}

	if ((err = emnetwork_get_wifi_status(&wifi_status)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emnetwork_get_roaming_status failed [%d]", err);
		goto FINISH_OFF;
	}

	if (wifi_status == 0 && roaming_status == 1 && ref_account->roaming_option == EMAIL_ROAMING_OPTION_RESTRICTED_BACKGROUND_TASK) {
		EM_DEBUG_LOG("wifi_status[%d], roaming_status[%d], roaming_option[%d]", wifi_status, roaming_status, ref_account->roaming_option);
		goto FINISH_OFF;
	}

	if (!emstorage_get_mailbox_id_by_mailbox_type(alarm_data->multi_user_name, account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox_id, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_name_by_mailbox_type failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emdaemon_sync_header(alarm_data->multi_user_name, account_id, mailbox_id, &handle, &err)) {
		EM_DEBUG_EXCEPTION("emdaemon_sync_header falied [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;
FINISH_OFF:

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	EM_DEBUG_FUNC_END("err[%d]", err);
	return ret;
}


static int _emdaemon_get_polling_account_and_timeinterval(email_alarm_data_t *alarm_data, int *account_id, int *timer_interval)
{
	EM_DEBUG_FUNC_BEGIN("alarm_data [%p] account_id[%p] timer_interval[%p]", alarm_data, account_id, timer_interval);
	int err = EMAIL_ERROR_NONE;
	email_account_t *account = NULL;

	if (!alarm_data || !account_id) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	account = emcore_get_account_reference(alarm_data->multi_user_name, alarm_data->reference_id, false);
	if (account == NULL) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", err);
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



static int _emdaemon_create_alarm_for_auto_polling(char *multi_user_name, int input_account_id)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d]", input_account_id);
	int err = EMAIL_ERROR_NONE;
	time_t trigger_at_time;
	time_t current_time;

	if (input_account_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	time(&current_time);

	if ((err = emcore_calc_next_time_to_sync(multi_user_name, input_account_id, current_time, &trigger_at_time)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_add_alarm failed [%d]", err);
		goto FINISH_OFF;
	}

	if (trigger_at_time == 0) {
		EM_DEBUG_LOG("trigger_at_time is 0. It means auto polling is disabled");
	} else if ((err = emcore_add_alarm(multi_user_name, trigger_at_time, EMAIL_ALARM_CLASS_AUTO_POLLING, input_account_id, emdaemon_alarm_polling_cb, NULL)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_add_alarm failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

#endif

