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


#include <stdlib.h>
#include <pthread.h>

#include "email-debug-log.h"
#include "email-storage.h"
#include "email-core-utils.h"
#include "email-core-mailbox.h"
#include "email-core-sound.h"
#include "email-core-alarm.h"
#include "email-utilities.h"
#include "email-daemon.h"

#define TIMER 30000   // 30 seconds
#define HAPTIC_TEST_ITERATION 1
#define EMAIL_ALARM_REFERENCE_ID_FOR_ALERT_TONE -1

static int emcore_get_alert_type(int vip_mode);
static int emcore_alarm_timeout_cb_for_alert(int timer_id, void *user_parm);
static int emcore_set_alarm_for_alert(int alert_time_in_minute);
static void emcore_set_repetition_alarm(int repetition);
static char * emcore_get_sound_file_path(int vip_mode);


static int emcore_get_alert_type(int vip_mode)
{
	EM_DEBUG_FUNC_BEGIN();
	int global_sound_status = 0;
	int global_vibe_status = 0;
	int email_vibe_status = 0;
	int call_state = 0;
	int alert_type = EMAIL_ALERT_TYPE_MUTE;
	int voicerecoder_state = 0;

	if (vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &global_sound_status) != 0) {
		EM_DEBUG_LOG("vconf_get_bool for VCONFKEY_SETAPPL_SOUND_STATUS_BOOL failed");
		goto FINISH_OFF;
	}

	if (vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, &global_vibe_status) != 0) {
		EM_DEBUG_LOG("vconf_get_bool for VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOLfailed");
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("global_sound_status [%d] global_vibe_status [%d]", global_sound_status, global_vibe_status);

	if (global_sound_status || global_vibe_status) {

		if (vconf_get_int(VCONFKEY_VOICERECORDER_STATE, &voicerecoder_state) != 0) {
			EM_DEBUG_LOG("vconf_get_int for VCONFKEY_VOICERECORDER_STATE failed");
		}
		EM_DEBUG_LOG("voicerecoder_state [%d]", voicerecoder_state);

		if (vconf_get_int(VCONFKEY_CALL_STATE, &call_state) != 0) {
			EM_DEBUG_LOG("vconf_get_int for VCONFKEY_CALL_STATE failed");
		}
		EM_DEBUG_LOG("call_state [%d] ", call_state);

		if (vip_mode) {
			if (vconf_get_bool(VCONF_VIP_NOTI_VIBRATION_STATUS_BOOL, &email_vibe_status) != 0) {
				EM_DEBUG_EXCEPTION("vconf_get_bool failed");
				return EMAIL_ALERT_TYPE_NONE;
			}
		}
		else {
			if (vconf_get_bool(VCONF_EMAIL_NOTI_VIBRATION_STATUS_BOOL, &email_vibe_status) != 0) {
				EM_DEBUG_EXCEPTION("vconf_get_bool failed");
				return EMAIL_ALERT_TYPE_NONE;
			}
		}

		EM_DEBUG_LOG("email_vibe_status [%d] ", email_vibe_status);

		if (voicerecoder_state == VCONFKEY_VOICERECORDER_RECORDING) {
			alert_type = EMAIL_ALERT_TYPE_VIB;
			EM_DEBUG_LOG("voice recorder is on recording...");
		}
		else if (call_state > VCONFKEY_CALL_OFF && call_state < VCONFKEY_CALL_STATE_MAX) {
			EM_DEBUG_LOG("Calling");
#ifdef FEATURE_OPCO_DOCOMO
			alert_type = EMAIL_ALERT_TYPE_NONE;
#else
			alert_type = EMAIL_ALERT_TYPE_VIB;
#endif
		}
		else if (global_sound_status && email_vibe_status) {
			alert_type = EMAIL_ALERT_TYPE_MELODY_AND_VIB;
		}
		else if (global_sound_status) {
			alert_type = EMAIL_ALERT_TYPE_MELODY;
		}
		else if (global_vibe_status) {
			alert_type = EMAIL_ALERT_TYPE_VIB;
		}
		else {
			alert_type = EMAIL_ALERT_TYPE_MUTE;
		}
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END("alert_type [%d]", alert_type);
	return alert_type;
}

static int emcore_alarm_timeout_cb_for_alert(int timer_id, void *user_parm)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;
	int total_unread_count = 0;
	int total_mail_count = 0;
	email_mailbox_t mailbox;

	memset(&mailbox, 0x00, sizeof(email_mailbox_t));

	mailbox.account_id = ALL_ACCOUNT;
	mailbox.mailbox_name = NULL;

	if (!emcore_get_mail_count(&mailbox, &total_mail_count, &total_unread_count, &err)) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_count failed - %d\n", err);
		return false;
	}

	EM_DEBUG_LOG(">>>> total_unread_count : [%d]\n", total_unread_count);
	
	if (total_unread_count) {
		/* emdaemon_start_alert(); */
		/* Alarm repetition disabled */
	}

	EM_DEBUG_FUNC_END();	
	return true;
}

/* repetition_time in minutes */
static int emcore_set_alarm_for_alert(int alert_time_in_minute)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;
	time_t current_time;
	time_t trigger_at_time;

	time(&current_time);

	trigger_at_time = current_time + (alert_time_in_minute * 60);

	if ((err = emcore_add_alarm(trigger_at_time, EMAIL_ALARM_CLASS_NEW_MAIL_ALERT, EMAIL_ALARM_REFERENCE_ID_FOR_ALERT_TONE, emcore_alarm_timeout_cb_for_alert, NULL)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_add_alarm failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static void emcore_set_repetition_alarm(int repetition)
{
	EM_DEBUG_FUNC_BEGIN();

	int repetition_time = 0;
	
	switch (repetition) {
	case EMAIL_GCONF_VALUE_REPEAT_NONE:
		repetition_time = 0;
		break;
	case EMAIL_GCONF_VALUE_REPEAT_2MINS:
		repetition_time = 2;
		break;
	case EMAIL_GCONF_VALUE_REPEAT_5MINS:
		repetition_time = 5;
		break;
	case EMAIL_GCONF_VALUE_REPEAT_10MINS:		
		repetition_time = 10;
		break;
	default:
		EM_DEBUG_EXCEPTION("Invalid repetition time");
		return;
	}

	EM_DEBUG_LOG("repetition time is %d", repetition_time);

	if (repetition_time > 0) {
		emcore_set_alarm_for_alert(repetition_time);
	} 

	EM_DEBUG_FUNC_END();
}

static char * emcore_get_sound_file_path(int vip_mode)
{
	EM_DEBUG_FUNC_BEGIN("vip_mode [%d]", vip_mode);
	char *ret = NULL;
	int   use_default_ring_tone = 0;

	if (vip_mode) {
		if (vconf_get_bool(VCONF_VIP_NOTI_USE_DEFAULT_RINGTONE_BOOL, &use_default_ring_tone) != 0)
			EM_DEBUG_LOG("vconf_get_bool for VCONF_VIP_NOTI_USE_DEFAULT_RINGTONE_BOOL failed");

		EM_DEBUG_LOG("use_default_ring_tone [%d]", use_default_ring_tone);

		if (use_default_ring_tone)
			ret = EM_SAFE_STRDUP(vconf_get_str(VCONFKEY_SETAPPL_NOTI_RINGTONE_DEFAULT_PATH_STR));
		else
			ret = EM_SAFE_STRDUP(vconf_get_str(VCONF_VIP_NOTI_RINGTONE_PATH));
	}
	else {
		if (vconf_get_bool(VCONF_EMAIL_NOTI_USE_DEFAULT_RINGTONE_BOOL, &use_default_ring_tone) != 0)
			EM_DEBUG_LOG("vconf_get_bool for VCONF_EMAIL_NOTI_USE_DEFAULT_RINGTONE_BOOL failed");

		EM_DEBUG_LOG("use_default_ring_tone [%d]", use_default_ring_tone);

		if (use_default_ring_tone)
			ret = EM_SAFE_STRDUP(vconf_get_str(VCONFKEY_SETAPPL_NOTI_RINGTONE_DEFAULT_PATH_STR));
		else
			ret = EM_SAFE_STRDUP(vconf_get_str(VCONFKEY_SETAPPL_NOTI_EMAIL_RINGTONE_PATH_STR));
	}

	EM_DEBUG_FUNC_END("ret [%s]", ret);
	return ret;
}


INTERNAL_FUNC int emcore_get_alert_policy(EMAIL_ALERT_TYPE *output_alert_type, char **output_alert_tone_path)
{
	EM_DEBUG_FUNC_BEGIN("output_alert_type [%p] output_alert_tone_path[%p]", output_alert_type, output_alert_tone_path);

	int err = EMAIL_ERROR_NONE;
	int general_noti_status = false;
	int vip_noti_status = false;
	int vip_mail_count = 0;
	int vip_unread_count = 0;
	int vip_notification_mode = 0;
	char *alert_tone_path = NULL;
	EMAIL_ALERT_TYPE alert_type = EMAIL_ALERT_TYPE_NONE;

	if (output_alert_type == NULL || output_alert_tone_path == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* get general noti status */
	if (vconf_get_bool(VCONFKEY_SETAPPL_STATE_TICKER_NOTI_EMAIL_BOOL, &general_noti_status) != 0) {
		EM_DEBUG_EXCEPTION("vconf_get_int failed");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* get vip noti status */
	if (vconf_get_bool(VCONF_VIP_NOTI_NOTIFICATION_TICKER, &vip_noti_status) != 0) {
		EM_DEBUG_EXCEPTION("vconf_get_int failed");
		err = EMAIL_ERROR_DATA_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (!general_noti_status && !vip_noti_status) {
		EM_DEBUG_LOG("both notification disabled");
		goto FINISH_OFF;
	}

	if (vip_noti_status) {
		if (!emcore_get_mail_count_by_query(ALL_ACCOUNT, EMAIL_MAILBOX_TYPE_INBOX, true, &vip_mail_count, &vip_unread_count, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_mail_count_by_query failed");
			err = EMAIL_ERROR_DATA_NOT_FOUND;
		}
	}

	if (general_noti_status && !vip_noti_status) {
		EM_DEBUG_LOG("GERNERAL NOTI MODE");
		vip_notification_mode = 0;
	}
	else if (!general_noti_status && vip_noti_status) {
		EM_DEBUG_LOG("VIP NOTI MODE");
		if (err == EMAIL_ERROR_FILTER_NOT_FOUND) {
			EM_DEBUG_LOG("VIP filter not found");
			err = EMAIL_ERROR_DATA_NOT_FOUND;
			goto FINISH_OFF;
		}
		vip_notification_mode = 1;
	}
	else if (general_noti_status && vip_noti_status) {
		if (err == EMAIL_ERROR_FILTER_NOT_FOUND) {
			EM_DEBUG_LOG("VIP filter not found");
			EM_DEBUG_LOG("GERNERAL NOTI MODE");
			vip_notification_mode = 0;
		} else {
			//confusing..
			if (vip_unread_count > 0) {
				EM_DEBUG_LOG("VIP NOTI MODE");
				vip_notification_mode = 1;
			} else {
				EM_DEBUG_LOG("GERNERAL NOTI MODE");
				vip_notification_mode = 0;
			}
		}
	}

	alert_type = emcore_get_alert_type(vip_notification_mode);
	EM_DEBUG_LOG("alert_type [%d]", alert_type);



	EM_DEBUG_LOG("vip_notification_mode [%d]", vip_notification_mode);

	if (alert_type == EMAIL_ALERT_TYPE_MELODY_AND_VIB || alert_type == EMAIL_ALERT_TYPE_MELODY)
		alert_tone_path = EM_SAFE_STRDUP(emcore_get_sound_file_path(vip_notification_mode));

	*output_alert_type = alert_type;
	if (alert_tone_path)
		*output_alert_tone_path = alert_tone_path;

	EM_DEBUG_LOG("alert_type [%d] alert_tone_path [%s]", alert_type, alert_tone_path);

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);

	return err;
}
