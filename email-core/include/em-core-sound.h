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


#include <vconf-keys.h>
#include <vconf.h>
#include <mm_player.h>
#include <mm_error.h>
#include <mm_session_private.h>
#include <devman_haptic.h>
#include <alarm.h>

#define MAX_LENGTH 255
#define TIMER 7000
#define HAPTIC_TEST_ITERATION 1

void em_core_set_repetition_alarm(int repetition);
int em_core_vibration_destory();
int em_core_vibration_stop();
int em_core_sound_mp_player_stop();
bool em_core_sound_mp_player_destory();
void *start_alert_thread(void *arg);
EXPORT_API int em_core_alert_loop_start(int *err_code);
EXPORT_API void start_alert();

#ifdef Min
#undef Min
#endif

typedef enum
{
	EMAIL_SOUND_STATUS, 
	EMAIL_VIBE_STATUS, 
	EMAIL_ALERT_REP_TYPE, 
	EMAIL_ALERT_VOLUME, 
	EMAIL_ALERT_VIBE_STENGTH, 
} EMAIL_SETTING_t;

typedef enum
{
	EMAIL_ALERT_TYPE_MELODY, 
	EMAIL_ALERT_TYPE_VIB, 
	EMAIL_ALERT_TYPE_MELODY_AND_VIB, 
	EMAIL_ALERT_TYPE_MUTE, 
} EMAIL_ALERT_TYPE;

typedef enum
{
	EMAIL_GCONF_VALUE_REPEAT_NONE = 0,
	EMAIL_GCONF_VALUE_REPEAT_2MINS,
	EMAIL_GCONF_VALUE_REPEAT_5MINS,
	EMAIL_GCONF_VALUE_REPEAT_10MINS,
} EMAIL_ALERT_REPEAT_ALARM;
