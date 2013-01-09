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


#include <stdlib.h>
#include <pthread.h>

#include "email-debug-log.h"
#include "email-storage.h"
#include "email-core-utils.h"
#include "email-core-mailbox.h"
#include "email-core-sound.h"
#include "email-utilities.h"

#define TIMER 30000   // 30 seconds
#define HAPTIC_TEST_ITERATION 1

static MMHandleType email_mmhandle = 0;
static alarm_id_t email_alarm_id = 0;
static int setting_noti_status = 0;

static char *filename;
alarm_entry_t *alarm_info = NULL;

static pthread_mutex_t sound_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sound_condition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mmhandle_mutex = PTHREAD_MUTEX_INITIALIZER;
static thread_t g_alert_thread;

void  emcore_set_repetition_alarm(int repetition);
int   emcore_sound_mp_player_stop();
bool  emcore_sound_mp_player_destory();
void *start_alert_thread(void *arg);

bool emcore_set_mp_filepath(const char *key)
{
	filename = vconf_get_str(key);
	if (filename == NULL)
		return false;

	/* initialize the ringtone path */
	if (vconf_set_str(VCONF_VIP_NOTI_RINGTONE_PATH, filename) != 0) {
		EM_DEBUG_EXCEPTION("vconf_set_str failed");
		return false;
	}

	return true;
}

int emcore_alert_sound_init()
{
	int ret = MM_ERROR_NONE;
	if ((ret = mm_session_init(MM_SESSION_TYPE_NOTIFY)) != MM_ERROR_NONE) 
		EM_DEBUG_EXCEPTION("mm_session_int failed");

	return ret;
}

int emcore_alert_alarm_init()
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = ALARMMGR_RESULT_SUCCESS;
	
	ret = alarmmgr_init("email-service-0");
	if (ret != ALARMMGR_RESULT_SUCCESS) 
		EM_DEBUG_EXCEPTION("alarmmgr_init failed : [%d]", ret);

	EM_DEBUG_FUNC_END();
	return ret;
}

int emcore_alert_sound_filepath_init()
{
	filename = (char  *)em_malloc(MAX_PATH);
	if (filename == NULL) {
		EM_DEBUG_EXCEPTION("Memory malloc error");	
		return false;
	}

	if (!emcore_set_mp_filepath(VCONFKEY_SETAPPL_NOTI_EMAIL_RINGTONE_PATH_STR)) {
		/* TODO : Add code to set default ringtone path */
		EM_DEBUG_EXCEPTION("emcore_set_mp_filepath failed.");
		return false;
	}

	return true;
}

void emcore_global_noti_key_changed_cb(keynode_t *key_node, void *data)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = 0;

	switch (vconf_keynode_get_type(key_node)) {
	case VCONF_TYPE_INT:
		ret = alarmmgr_remove_alarm(email_alarm_id);
		if (ret != ALARMMGR_RESULT_SUCCESS) {
			EM_DEBUG_EXCEPTION("delete of alarm id failed");
		}
		emcore_set_repetition_alarm(vconf_keynode_get_int(key_node));
		break;
	case VCONF_TYPE_STRING:
		filename = vconf_keynode_get_str(key_node);
		break;
	default:
		EM_DEBUG_EXCEPTION("Invalid key type");
		break;
	}

	EM_DEBUG_FUNC_END();
}

void emcore_email_noti_key_changed_cb(keynode_t *key_node, void *data)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = 0;

	switch (vconf_keynode_get_type(key_node)) {
	case VCONF_TYPE_INT:
		ret = alarmmgr_remove_alarm(email_alarm_id);
		if (ret != ALARMMGR_RESULT_SUCCESS) {
			EM_DEBUG_EXCEPTION("delete of alarm id failed");
		}
		emcore_set_repetition_alarm(vconf_keynode_get_int(key_node));
		break;
	case VCONF_TYPE_STRING:
		filename = vconf_keynode_get_str(key_node);
		break;
	default:
		EM_DEBUG_EXCEPTION("Invalid key type");
		break;
	}
	EM_DEBUG_FUNC_END();
}

bool emcore_update_noti_status()
{
	EM_DEBUG_FUNC_BEGIN();
	int ticker_noti = 0;

	/* Get the priority noti ticker */
	if (vconf_get_bool(VCONF_VIP_NOTI_NOTIFICATION_TICKER, &ticker_noti) != 0) {
		EM_DEBUG_EXCEPTION("vconf_get_bool failed");
		return false;
	}

	EM_DEBUG_LOG("ticker_noti of vip : [%d]", ticker_noti);

	if (ticker_noti <= 0) {
		/* Get the Global noti ticker */
		if (vconf_get_bool(VCONFKEY_SETAPPL_STATE_TICKER_NOTI_EMAIL_BOOL, &ticker_noti) != 0) {
				EM_DEBUG_EXCEPTION("Not display the noti of email");
				return false;
		}

		EM_DEBUG_LOG("ticker_noti of global : [%d]", ticker_noti);

		if (!ticker_noti) {
			EM_DEBUG_LOG("Not use the notification");
			setting_noti_status = SETTING_NOTI_STATUS_OFF;
			return true;
		}

		setting_noti_status = SETTING_NOTI_STATUS_GLOBAL;
	} else {
		setting_noti_status = SETTING_NOTI_STATUS_EMAIL;
	}

	EM_DEBUG_FUNC_END();
	return true;
}

void emcore_noti_status_changed_cb(keynode_t *key_node, void *data)
{
	EM_DEBUG_FUNC_BEGIN();
	if (!emcore_update_noti_status()) {
		EM_DEBUG_EXCEPTION("emcore_update_noti_status failed");
		return;
	}
	EM_DEBUG_FUNC_END();
}

bool emcore_noti_init(void *data)
{
	EM_DEBUG_FUNC_BEGIN();
	struct appdata *ap = data;

	if (!emcore_update_noti_status()) {
		EM_DEBUG_EXCEPTION("emcore_update_noti_status failed");
		return false;
	}

	/* Noti callback registration */
	if (vconf_notify_key_changed(VCONFKEY_SETAPPL_NOTI_EMAIL_ALERT_REP_TYPE_INT, emcore_global_noti_key_changed_cb, ap) < 0) {
		EM_DEBUG_EXCEPTION("Register failed : alert type");
	}

	if (vconf_notify_key_changed(VCONFKEY_SETAPPL_NOTI_EMAIL_RINGTONE_PATH_STR, emcore_global_noti_key_changed_cb, ap) < 0) {
		EM_DEBUG_EXCEPTION("Register failed : Ringtone path");
	}

	if (vconf_notify_key_changed(VCONF_VIP_NOTI_REP_TYPE, emcore_email_noti_key_changed_cb, ap) < 0) {
		EM_DEBUG_EXCEPTION("Register failed : alert type");
	}

	if (vconf_notify_key_changed(VCONF_VIP_NOTI_RINGTONE_PATH, emcore_email_noti_key_changed_cb, ap) < 0) {
		EM_DEBUG_EXCEPTION("Register failed : Ringtone path");
	}

	if (vconf_notify_key_changed(VCONF_VIP_NOTI_NOTIFICATION_TICKER, emcore_noti_status_changed_cb, ap) < 0) {
		EM_DEBUG_EXCEPTION("Register failed : Ringtone path");
	}

	if (vconf_notify_key_changed(VCONFKEY_SETAPPL_STATE_TICKER_NOTI_EMAIL_BOOL, emcore_noti_status_changed_cb, ap) < 0) {
		EM_DEBUG_EXCEPTION("Register failed : Ringtone path");
	}

	EM_DEBUG_FUNC_END();
	return true;
}

int emcore_alert_init()
{
	EM_DEBUG_FUNC_BEGIN();

	int err = 0;
	
	if (!emcore_alert_sound_filepath_init()) {
		EM_DEBUG_EXCEPTION("emcore_alert_sound_filepath_init failed");
		return false;
	}

	if ((err = emcore_alert_sound_init()) != MM_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_alert_sound_init failed : [%d]", err);
		return false;
	}

	if ((err = emcore_alert_alarm_init()) != ALARMMGR_RESULT_SUCCESS) {
		EM_DEBUG_EXCEPTION("emcore_alert_alarm_init failed : [%d]", err);
		return false;		
	}

	if (!emcore_noti_init(NULL)) {
		EM_DEBUG_EXCEPTION("emcore_noti_init failed");
		return false;		
	}

	EM_DEBUG_FUNC_END();	
	return true;
}

int emcore_mp_player_state_cb(int message, void *param, void *user_param)
{
	switch (message)
	{
		case MM_MESSAGE_ERROR:
			EM_DEBUG_LOG("Error is happened.");
			if (email_mmhandle) {
				emcore_sound_mp_player_destory();
			}
			break;
		case MM_MESSAGE_BEGIN_OF_STREAM:
			EM_DEBUG_LOG("Play is started.");
			break;
		case MM_MESSAGE_END_OF_STREAM:
			EM_DEBUG_LOG("End of stream.");
			ENTER_CRITICAL_SECTION(mmhandle_mutex);
			if (email_mmhandle)
			{			
				emcore_sound_mp_player_stop();
				emcore_sound_mp_player_destory();
			}
			LEAVE_CRITICAL_SECTION(mmhandle_mutex);
			break;
		default: 
			EM_DEBUG_LOG("Message = %d", message);
			break;
	}
	return 1;
}

bool emcore_sound_mp_player_create() 
{	
	EM_DEBUG_FUNC_BEGIN();
	int err = 0;
	
	if (email_mmhandle) {
		EM_DEBUG_LOG("already create the handle");
		return false;
	}

	if ((err = mm_player_create(&email_mmhandle)) != MM_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("mm_player create fail [%d]", err);
		return false;
	}	
	EM_DEBUG_FUNC_END();
	return true;
}

bool emcore_alarm_create() 
{	
	EM_DEBUG_FUNC_BEGIN();

	alarm_info = alarmmgr_create_alarm();

	if (alarm_info == NULL) {
		EM_DEBUG_EXCEPTION("alarm create failed");
		return false;
	}		

	EM_DEBUG_FUNC_END();
	return true;
}

bool emcore_alarm_destory()
{
	EM_DEBUG_FUNC_BEGIN();

	int ret;
	ret = alarmmgr_free_alarm(alarm_info);

	if (ret != ALARMMGR_RESULT_SUCCESS) {
		EM_DEBUG_EXCEPTION("alarm free failed");
		return false;
	}		

	EM_DEBUG_FUNC_END();
	return true;
}

bool emcore_alert_create()
{
	EM_DEBUG_FUNC_BEGIN();

	/* Create the alarm handle */
	if (!emcore_alarm_create()) {
		EM_DEBUG_EXCEPTION("emcore_alarm_create failed.");
		return false;
	}
#if 0	
	/* Set the music file in alert */
	if (!emcore_set_mp_filepath(VCONFKEY_SETAPPL_NOTI_EMAIL_RINGTONE_PATH_STR)) {
		/* TODO : Add code to set default ringtone path */
		EM_DEBUG_EXCEPTION("emcore_set_mp_filepath failed.");
		return false;
	}
#endif	
	EM_DEBUG_FUNC_END();
	return true;
}

bool emcore_alert_destory()
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = 0;

	/* Destroy the music player handle */
	if (!emcore_sound_mp_player_destory()) {
		EM_DEBUG_EXCEPTION("emcore_sound_mp_player_destory fail");
		return false;
	}			

	/* Destroy the alarm handle */
	ret = alarmmgr_free_alarm(alarm_info);
	if (ret != ALARMMGR_RESULT_SUCCESS) {
		EM_DEBUG_EXCEPTION("alarmmgr_free_alarm fail");
		return false;
	}			
	
	/* Set the music file in alert */
	EM_SAFE_FREE(filename);
	
	EM_DEBUG_FUNC_END();
	return true;
}

gboolean mp_player_timeout_cb(void *data)
{
	EM_DEBUG_FUNC_BEGIN();

	ENTER_CRITICAL_SECTION(mmhandle_mutex);	
	if (email_mmhandle)
	{			
		emcore_sound_mp_player_stop();
		emcore_sound_mp_player_destory();
	}
	LEAVE_CRITICAL_SECTION(mmhandle_mutex);
	
	EM_DEBUG_FUNC_END();
	return false;
}

bool emcore_vibration_start()
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = false;
	int error = FEEDBACK_ERROR_NONE;
	int call_state = 0;

	error = vconf_get_int(VCONFKEY_CALL_STATE, &call_state);
	if (error == -1) {
		EM_DEBUG_EXCEPTION("vconf_get_int failed");
		goto FINISH_OFF;
	}

	error = feedback_initialize();
	if (error != FEEDBACK_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("feedback_initialize failed : [%d]", error);
		goto FINISH_OFF;
	}

	if (call_state > VCONFKEY_CALL_OFF && call_state < VCONFKEY_CALL_STATE_MAX) {	
		error = feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_EMAIL_ON_CALL);
	} else {
		error = feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_EMAIL);
	}

	if (error != FEEDBACK_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("feedback_play failed : [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	error = feedback_deinitialize();
	if (error != FEEDBACK_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("feedback_deinitialize failed : [%d]", error);
	}
	
	EM_DEBUG_FUNC_END();
	return ret;
}

int emcore_sound_mp_player_start(char *filepath)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = MM_ERROR_NONE;

/*	
	int volume = -1;


	if ((err = vconf_get_int(VCONFKEY_SETAPPL_NOTI_SOUND_VOLUME_INT, &volume)) == -1)
	{
		EM_DEBUG_LOG("vconf_get_int failed \n");
		return err;
	}
*/
	mm_player_set_message_callback(email_mmhandle, emcore_mp_player_state_cb, (void  *)email_mmhandle);

	EM_DEBUG_LOG("Before mm_player_set_attribute filepath = %s", filepath);
	if ((err = mm_player_set_attribute(email_mmhandle, NULL, "sound_volume_type", MM_SOUND_VOLUME_TYPE_NOTIFICATION, "profile_uri", filepath, EM_SAFE_STRLEN(filepath), NULL)) != MM_ERROR_NONE)
	{
		EM_DEBUG_EXCEPTION("mm_player_set_attribute faile [ %d ] ", err);
		return err;
	}

	EM_DEBUG_LOG("After mm_player_set_attribute");	

	if ((err = mm_player_realize(email_mmhandle)) != MM_ERROR_NONE)
	{
		EM_DEBUG_EXCEPTION("mm_player_realize fail [%d]", err);
		return err;
	}

	if ((err = mm_player_start(email_mmhandle)) != MM_ERROR_NONE)
	{
		EM_DEBUG_EXCEPTION("mm_player_start fail [%d]", err);
		return err;
	}

	if ((err = g_timeout_add(TIMER, (GSourceFunc)mp_player_timeout_cb, NULL) <= 0))
	{
		EM_DEBUG_EXCEPTION("g_timeout_add - Failed to start timer");
		return err;		
	}
	
	EM_DEBUG_FUNC_END();
	return err;
}

int emcore_sound_mp_player_stop()
{
	EM_DEBUG_FUNC_BEGIN();

	int err = MM_ERROR_NONE;

	if ((err = mm_player_stop(email_mmhandle)) != MM_ERROR_NONE)
	{
		EM_DEBUG_EXCEPTION("mm_player_stop fail [%d]", err);
		return err;
	}

	if ((err = mm_player_unrealize(email_mmhandle)) != MM_ERROR_NONE)
	{
		EM_DEBUG_EXCEPTION("mm_player_unrealize [%d]", err);
		return err;
	}

	EM_DEBUG_FUNC_END();
	return err;
}

bool emcore_sound_mp_player_destory()
{
	EM_DEBUG_FUNC_BEGIN();

	int err = MM_ERROR_NONE;

	if ((err = mm_player_destroy(email_mmhandle)) != MM_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("mm_player_destory [%d]", err);
		return false;
	}

	email_mmhandle = 0;

	EM_DEBUG_FUNC_END();
	return true;
}	

int get_vconf_data(int key, int *return_value)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = -1, value = 0;
	
	switch (key)
	{
		case EMAIL_SOUND_STATUS:
			err = vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &value);
			EM_DEBUG_LOG("EMAIL_SOUND_STATUS[%d]", value);
			break;
		case EMAIL_VIBE_STATUS:
			err = vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, &value);
			EM_DEBUG_LOG("EMAIL_VIBRATION_STATUS[%d]", value);
			break;
		case EMAIL_ALERT_REP_TYPE:
			err = vconf_get_int(VCONFKEY_SETAPPL_NOTI_EMAIL_ALERT_REP_TYPE_INT, &value);
			EM_DEBUG_LOG("EMAIL_ALERT_REP_TYPE[%d]", value);
			break;
		case EMAIL_ALERT_VOLUME:
			err = vconf_get_int(VCONFKEY_SETAPPL_NOTI_SOUND_VOLUME_INT, &value);
			EM_DEBUG_LOG("EMAIL_ALERT_VOLUME[%d]", value);
			break;
		case EMAIL_ALERT_VIBE_STENGTH:
			err = vconf_get_int(VCONFKEY_SETAPPL_NOTI_VIBRATION_LEVEL_INT, &value);
			EM_DEBUG_LOG("EMAIL_ALERT_VIBE_STENGTH[%d]", value);
			break;
		default: 
		{
			EM_DEBUG_LOG("Uuknown request\n");
			return false;			
		}
	}
	
	if (err == -1)
	{
		EM_DEBUG_LOG("Vconf_get_int failed\n");
		return false;
	}

	*return_value = value;
	EM_DEBUG_FUNC_END();
	return true;
}

int emcore_get_alert_type()
{
	EM_DEBUG_FUNC_BEGIN();
	int sound_status = 0, vibe_status = 0;
	int err;
	int alert_type = -1;

	if (!(err = get_vconf_data(EMAIL_SOUND_STATUS, &sound_status))) {
		EM_DEBUG_EXCEPTION("Don't get sound status");
		return err;
	}

 	if (!(err = get_vconf_data(EMAIL_VIBE_STATUS, &vibe_status)))
	{
		EM_DEBUG_EXCEPTION("Don't get vibration status");
		return err;
	}

	if (sound_status && vibe_status)
		alert_type = EMAIL_ALERT_TYPE_MELODY_AND_VIB;
	else if (sound_status)
		alert_type = EMAIL_ALERT_TYPE_MELODY;
	else if (vibe_status)
		alert_type = EMAIL_ALERT_TYPE_VIB;
	else
		alert_type = EMAIL_ALERT_TYPE_MUTE;

	return alert_type;
}


INTERNAL_FUNC int emcore_start_thread_for_alerting_new_mails(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int thread_error;

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;
	
	if (g_alert_thread)
	{
		EM_DEBUG_EXCEPTION("Alert service is already running...");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_UNKNOWN;
		
		return 1;
	}
	
	THREAD_CREATE(g_alert_thread, start_alert_thread, NULL, thread_error);
	if (thread_error != 0)
	{
		EM_DEBUG_EXCEPTION("Cannot create alert thread");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_SYSTEM_FAILURE;

		return -1;
	}
		
	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	return 0;
}

int emcore_alarm_timeout_cb(int timer_id, void *user_parm)
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
		emcore_start_alert();
	}

	EM_DEBUG_FUNC_END();	
	return true;
}

bool set_alarm(int repetition_time)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = 0;
	alarm_date_t alarm_date;
	time_t current_time;
	struct tm current_tm;

	time(&current_time);
	localtime_r(&current_time, &current_tm);
	
	alarm_date.year = 0;
	alarm_date.month = 0;
	alarm_date.day = 0;

	EM_DEBUG_LOG("Current time : [%d]-[%d]-[%d]", current_tm.tm_hour, current_tm.tm_min, current_tm.tm_sec);
	
	if (current_tm.tm_min + repetition_time < 60) {
		alarm_date.hour = current_tm.tm_hour;
		alarm_date.min = current_tm.tm_min + repetition_time;
	} else {
		if (current_tm.tm_hour < 12) {
			alarm_date.hour = current_tm.tm_hour + 1;
		} else {
			alarm_date.hour = (current_tm.tm_hour + 1) % 12;
		}

		alarm_date.min = (current_tm.tm_min + repetition_time) % 60;
	}

	alarm_date.sec = current_tm.tm_sec;
	
	alarmmgr_set_time(alarm_info, alarm_date);
	alarmmgr_set_repeat_mode(alarm_info, ALARM_REPEAT_MODE_ONCE, 0);
	alarmmgr_set_type(alarm_info, ALARM_TYPE_VOLATILE);
	alarmmgr_add_alarm_with_localtime(alarm_info, NULL, &email_alarm_id);

	ret = alarmmgr_set_cb(emcore_alarm_timeout_cb, NULL);

	if (ret != ALARMMGR_RESULT_SUCCESS) {
		EM_DEBUG_EXCEPTION("Failed : alarmmgr_set_cb() -> error[%d]", ret);
		return false;
	}

	EM_DEBUG_LOG("Alarm time : [%d]-[%d]-[%d]-[%d]-[%d]-[%d]", alarm_date.year, alarm_date.month, alarm_date.day, alarm_date.hour, alarm_date.min, alarm_date.sec);
	EM_DEBUG_FUNC_END();
	return true;
}

void emcore_set_repetition_alarm(int repetition)
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
		set_alarm(repetition_time);
	} 

	EM_DEBUG_FUNC_END();
}

void *start_alert_thread(void *arg)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int err = 0;
	int repetition = 0;

	if (!emcore_alert_init())
	{
		EM_DEBUG_EXCEPTION("Error : emcore_alert_init failed");
		return 0;
	}

	while (1) {
		if (!emcore_alert_create()) {
			EM_DEBUG_EXCEPTION("Error : emcore_alert_create failed");
			return 0;
		}

		err = get_vconf_data(EMAIL_ALERT_REP_TYPE, &repetition);
		emcore_set_repetition_alarm(repetition);

		ENTER_CRITICAL_SECTION(sound_mutex);
		SLEEP_CONDITION_VARIABLE(sound_condition , sound_mutex);

		switch (emcore_get_alert_type())
		{
			case EMAIL_ALERT_TYPE_MELODY:
				if (!emcore_sound_mp_player_create()) {
					EM_DEBUG_LOG("emcore_sound_mp_player_create failed : [%d]", email_mmhandle);
					break;
				}
				emcore_sound_mp_player_start(filename);
				break;
			case EMAIL_ALERT_TYPE_VIB:
				emcore_vibration_start();
				break;
			case EMAIL_ALERT_TYPE_MELODY_AND_VIB:
				emcore_vibration_start();
				if (!emcore_sound_mp_player_create()) {
					EM_DEBUG_LOG("emcore_sound_mp_player_create failed : [%d]", email_mmhandle);
					break;
				}
				emcore_sound_mp_player_start(filename);
				break;
			case EMAIL_ALERT_TYPE_MUTE:
				EM_DEBUG_LOG("Alert type is mute!!");
				break;
			default: 
				EM_DEBUG_EXCEPTION("alert type is strange");
				emcore_alert_destory();
				break;
		}
		LEAVE_CRITICAL_SECTION(sound_mutex);
		EM_DEBUG_LOG("Start FINISH");
		emcore_alarm_destory();
	}

	EM_DEBUG_FUNC_END();
	return 0;
}	

INTERNAL_FUNC void emcore_start_alert()
{
	EM_DEBUG_FUNC_BEGIN("setting_noti_status : [%d]", setting_noti_status);

	if (setting_noti_status == SETTING_NOTI_STATUS_OFF)
		return;

	ENTER_CRITICAL_SECTION(sound_mutex);
	WAKE_CONDITION_VARIABLE(sound_condition);
	LEAVE_CRITICAL_SECTION(sound_mutex);
}
