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
#include "emf-dbglog.h"
#include "em-storage.h"
#include "em-core-mailbox.h"
#include "em-core-sound.h"

static MMHandleType email_mmhandle = MM_PLAYER_STATE_NONE;
static alarm_id_t email_alarm_id = 0;
static int email_vibe_handle = 0;

static char *filename;
alarm_entry_t *alarm_info = NULL;

#ifdef __FEATURE_USE_PTHREAD__
static pthread_mutex_t sound_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sound_condition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mmhandle_mutex = PTHREAD_MUTEX_INITIALIZER;
static thread_t g_alert_thread;
#else /*  _USE_PTHREAD */
static GMutex *_sound_event_available_lock = NULL;
static GCond *_sound_event_available_signal = NULL;
static pthread_mutex_t mmhandle_mutex = NULL;
static thread_t g_alert_thread;
#endif /*  _USE_PTHREAD */

int em_core_alert_sound_init()
{
	int ret = MM_ERROR_NONE;
	if ((ret = mm_session_init(MM_SESSION_TYPE_NOTIFY)) != MM_ERROR_NONE) 
		EM_DEBUG_EXCEPTION("mm_session_int failed");

	return ret;
}

int em_core_alert_alarm_init()
{
	int ret = ALARMMGR_RESULT_SUCCESS;
	
	ret = alarmmgr_init("email-service-0");
	if (ret != 0) 
		EM_DEBUG_EXCEPTION("alarmmgr_init failed : [%d]", ret);

	return ret;
}

int em_core_alert_sound_filepath_init()
{
	filename = (char  *)em_core_malloc(MAX_LENGTH);
	if (filename == NULL) {
		EM_DEBUG_EXCEPTION("Memory malloc error");	
		return false;
	}

	memset(filename, 0, MAX_LENGTH);

	return true;
}
int em_core_alert_vibe_init()
{
	email_vibe_handle = device_haptic_open(DEV_IDX_0, 0);	
	if (!email_vibe_handle) {
		EM_DEBUG_EXCEPTION("device_haptic_open failed");
		return false;
	}

	return true;
}

void em_core_noti_key_changed_cb(keynode_t *key_node, void *data)
{
	int ret = 0;
	
	switch (vconf_keynode_get_type(key_node)) {
	case VCONF_TYPE_INT:
		ret = alarmmgr_remove_alarm(email_alarm_id);
		if (ret != ALARMMGR_RESULT_SUCCESS) {
			EM_DEBUG_EXCEPTION("delete of alarm id failed");
		}
		em_core_set_repetition_alarm(vconf_keynode_get_int(key_node));
		break;
	case VCONF_TYPE_STRING:
		filename = vconf_keynode_get_str(key_node);
		break;
	default:
		EM_DEBUG_EXCEPTION("Invalid key type");
		break;
	}
	return;
}

bool em_core_noti_init(void *data)
{
	struct appdata *ap = data;
	
	if (vconf_notify_key_changed(VCONFKEY_SETAPPL_NOTI_EMAIL_ALERT_REP_TYPE_INT, em_core_noti_key_changed_cb, ap) < 0) {
		EM_DEBUG_EXCEPTION("Register failed : alert type");
		return false;
	}

	if (vconf_notify_key_changed(VCONFKEY_SETAPPL_NOTI_EMAIL_RINGTONE_PATH_STR, em_core_noti_key_changed_cb, ap) < 0) {
		EM_DEBUG_EXCEPTION("Register failed : Ringtone path");
		return false;
	}

	return true;
}

int em_core_alert_init()
{
	EM_DEBUG_FUNC_BEGIN();

	int err = 0;
	
	if (!em_core_alert_sound_filepath_init()) {
		EM_DEBUG_EXCEPTION("em_core_alert_sound_filepath_init failed");
		return false;
	}

	if ((err = em_core_alert_sound_init()) != MM_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_core_alert_sound_init failed : [%d]", err);
		return false;
	}

	if ((err = em_core_alert_alarm_init()) != ALARMMGR_RESULT_SUCCESS) {
		EM_DEBUG_EXCEPTION("em_core_alert_alarm_init failed : [%d]", err);
		return false;		
	}

	if (!em_core_alert_vibe_init()) {
		EM_DEBUG_EXCEPTION("em_core_alert_vibe_init failed");
		return false;		
	}

	if (!em_core_noti_init(NULL)) {
		EM_DEBUG_EXCEPTION("em_core_noti_init failed");
		return false;		
	}

	EM_DEBUG_FUNC_END();	
	return true;
}

bool em_core_set_mp_filepath(const char *key)
{
	filename = vconf_get_str(key);
	if (filename == NULL)
		return false;

	return true;
}

int em_core_mp_player_state_cb(int message, void *param, void *user_param)
{
	switch (message)
	{
		case MM_MESSAGE_ERROR:
			EM_DEBUG_LOG("Error is happened.");
			if (email_mmhandle) {
				em_core_sound_mp_player_destory();
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
				em_core_sound_mp_player_stop();
				em_core_sound_mp_player_destory();
			}
			LEAVE_CRITICAL_SECTION(mmhandle_mutex);
			break;
		default: 
			EM_DEBUG_LOG("Message = %d", message);
			break;
	}
	return 1;
}

bool em_core_sound_mp_player_create() 
{	
	EM_DEBUG_FUNC_BEGIN();
	int err = 0;
	
	if ((err = mm_player_create(&email_mmhandle)) != MM_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("mm_player create fail [%d]", err);
		return false;
	}	
	EM_DEBUG_FUNC_END();
	return true;
}

bool em_core_vibration_create() 
{	
	EM_DEBUG_FUNC_BEGIN();

	email_vibe_handle = device_haptic_open(DEV_IDX_0, 0);

	if (email_vibe_handle < 0) {
		EM_DEBUG_EXCEPTION("vibration create failed");
		return false;
	}
	EM_DEBUG_FUNC_END();
	return true;
}

bool em_core_alarm_create() 
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

bool em_core_alarm_destory()
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

bool em_core_alert_create()
{
	EM_DEBUG_FUNC_BEGIN();

	/* Create the alarm handle */
	if (!em_core_alarm_create()) {
		EM_DEBUG_EXCEPTION("em_core_alarm_create failed.");
		return false;
	}
	
	/* Set the music file in alert */
	if (!em_core_set_mp_filepath(VCONFKEY_SETAPPL_NOTI_EMAIL_RINGTONE_PATH_STR))
		if (!em_core_set_mp_filepath(VCONFKEY_SETAPPL_DEFAULT_NOTI_EMAIL_RINGTONE_PATH_STR)) {
			EM_DEBUG_EXCEPTION("em_core_set_mp_filepath failed.");
			return false;
		}
	
	EM_DEBUG_FUNC_END();
	return true;
}

bool em_core_alert_destory()
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = 0;

	/* Destroy the music player handle */
	if (!em_core_sound_mp_player_destory()) {
		EM_DEBUG_EXCEPTION("em_core_sound_mp_player_destory fail");
		return false;
	}			

	/* Destroy the vibration handle */
	if (!em_core_vibration_destory()) {
		EM_DEBUG_EXCEPTION("em_core_vibration_destory fail");
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
	if (email_mmhandle == MM_PLAYER_STATE_PLAYING)
	{			
		em_core_sound_mp_player_stop();
		em_core_sound_mp_player_destory();
	}
	LEAVE_CRITICAL_SECTION(mmhandle_mutex);
	
	EM_DEBUG_FUNC_END();
	return false;
}

gboolean vibration_timeout_cb(void *data)
{
	EM_DEBUG_FUNC_BEGIN();

	em_core_vibration_stop();
	em_core_vibration_destory();
	
	EM_DEBUG_FUNC_END();
	return false;
}

bool em_core_vibration_start(int haptic_level)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = 0;
	int vibPattern = EFFCTVIBE_NOTIFICATION;

	if (haptic_level == 0) {
		EM_DEBUG_LOG("The level of haptic is zero");
		return true;
	}

	ret = device_haptic_play_pattern(email_vibe_handle, vibPattern, HAPTIC_TEST_ITERATION, haptic_level);

	if (ret != 0) {
		EM_DEBUG_EXCEPTION("Fail to play haptic  :  [%d]", ret);
		return false;
	}

	if ((ret = g_timeout_add(TIMER, (GSourceFunc) vibration_timeout_cb, NULL) <= 0))
	{
		EM_DEBUG_EXCEPTION("play_alert - Failed to start timer [%d]", ret);
		return false;		
	}	
	
	EM_DEBUG_FUNC_END();
	return true;
}

int em_core_vibration_stop()
{
	int err = MM_ERROR_NONE;
	if ((err = device_haptic_stop_play(email_vibe_handle)) != 0)
		EM_DEBUG_EXCEPTION("Fail to stop haptic  :  [%d]", err);

	return err;
}

int em_core_vibration_destory()
{
	int err = MM_ERROR_NONE;
	if ((err = device_haptic_close(email_vibe_handle)) != 0)
		EM_DEBUG_EXCEPTION("Fail to close haptic  :  [%d]", err);

	return err;
}
int em_core_sound_mp_player_start(char *filepath)
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
	mm_player_set_message_callback(email_mmhandle, em_core_mp_player_state_cb, (void  *)email_mmhandle);

	EM_DEBUG_LOG("Before mm_player_set_attribute filepath = %s", filepath);
	if ((err = mm_player_set_attribute(email_mmhandle, NULL, "sound_volume_type", MM_SOUND_VOLUME_TYPE_NOTIFICATION, "profile_uri", filepath, strlen(filepath), NULL)) != MM_ERROR_NONE)
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

int em_core_sound_mp_player_stop()
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

bool em_core_sound_mp_player_destory()
{
	EM_DEBUG_FUNC_BEGIN();

	int err = MM_ERROR_NONE;

	if ((err = mm_player_destroy(email_mmhandle)) != MM_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("mm_player_destory [%d]", err);
		return false;
	}

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

int em_core_get_alert_type()
{
	EM_DEBUG_FUNC_BEGIN();
	int sound_status = 0, vibe_status = 0;
	int err;
	int alert_type = -1;

 	if (!(err = get_vconf_data(EMAIL_SOUND_STATUS, &sound_status)))
	{
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


EXPORT_API int em_core_alert_loop_start(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int thread_error;

	if (err_code != NULL)
		*err_code = EMF_ERROR_NONE;
	
	if (g_alert_thread)
	{
		EM_DEBUG_EXCEPTION("Alert service is already running...");
		if (err_code != NULL)
			*err_code = EMF_ERROR_UNKNOWN;
		
		return 1;
	}
	
	THREAD_CREATE(g_alert_thread, start_alert_thread, NULL, thread_error);
	if (thread_error != 0)
	{
		EM_DEBUG_EXCEPTION("Cannot create alert thread");
		if (err_code != NULL)
			*err_code = EMF_ERROR_SYSTEM_FAILURE;

		return -1;
	}
		
	if (err_code != NULL)
		*err_code = EMF_ERROR_NONE;

	return 0;
}

int em_core_alarm_timeout_cb(int timer_id, void *user_parm)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMF_ERROR_NONE;
	int total_unread_count = 0;
	int total_mail_count = 0;
	emf_mailbox_t mailbox;

	memset(&mailbox, 0x00, sizeof(emf_mailbox_t));

	mailbox.account_id = ALL_ACCOUNT;
	mailbox.name = NULL;

	if (!em_core_mailbox_get_mail_count(&mailbox, &total_mail_count, &total_unread_count, &err)) {
		EM_DEBUG_EXCEPTION("em_core_mailbox_get_mail_count failed - %d\n", err);
		return false;
	}

	EM_DEBUG_LOG(">>>> total_unread_count : [%d]\n", total_unread_count);
	
	if (total_unread_count) {
		start_alert();
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

	ret = alarmmgr_set_cb(em_core_alarm_timeout_cb, NULL);

	if (ret != 0) {
		EM_DEBUG_EXCEPTION("Failed : alarmmgr_set_cb() -> error[%d]", ret);
		return false;
	}

	EM_DEBUG_LOG("Alarm time : [%d]-[%d]-[%d]-[%d]-[%d]-[%d]", alarm_date.year, alarm_date.month, alarm_date.day, alarm_date.hour, alarm_date.min, alarm_date.sec);
	EM_DEBUG_FUNC_END();
	return true;
}

void em_core_set_repetition_alarm(int repetition)
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
	EM_DEBUG_FUNC_END();
	
	int err = 0;
	int level = 0;

	if (!em_core_alert_init())
	{
		EM_DEBUG_EXCEPTION("Error : em_core_alert_init failed");
		return 0;
	}


	while (1) {
		if (!em_core_alert_create()) {
			EM_DEBUG_EXCEPTION("Error : em_core_alert_create failed");
			return 0;
		}

		err = get_vconf_data(EMAIL_ALERT_REP_TYPE, &level);
		em_core_set_repetition_alarm(level);

		ENTER_CRITICAL_SECTION(sound_mutex);
		SLEEP_CONDITION_VARIABLE(sound_condition , sound_mutex);

		err = get_vconf_data(EMAIL_ALERT_VIBE_STENGTH, &level);

		switch (em_core_get_alert_type())
		{
			case EMAIL_ALERT_TYPE_MELODY:
				em_core_sound_mp_player_create();
				em_core_sound_mp_player_start(filename);
				break;
			case EMAIL_ALERT_TYPE_VIB:
				em_core_vibration_create();
				em_core_vibration_start(level);
				break;
			case EMAIL_ALERT_TYPE_MELODY_AND_VIB:
				em_core_vibration_create();
				em_core_vibration_start(level);
				em_core_sound_mp_player_create();
				em_core_sound_mp_player_start(filename);
				break;
			case EMAIL_ALERT_TYPE_MUTE:
				EM_DEBUG_LOG("Alert type is mute!!");
				break;
			default: 
				EM_DEBUG_EXCEPTION("alert type is strange");
				em_core_alert_destory();
				break;
		}
		LEAVE_CRITICAL_SECTION(sound_mutex);

		em_core_alarm_destory();
	}
	return 0;
}	

EXPORT_API void start_alert()
{
	ENTER_CRITICAL_SECTION(sound_mutex);
	WAKE_CONDITION_VARIABLE(sound_condition);
	LEAVE_CRITICAL_SECTION(sound_mutex);
}	
