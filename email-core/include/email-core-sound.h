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


#include <vconf-keys.h>
#include <vconf.h>
#include <mm_player.h>
#include <mm_error.h>
#include <mm_session_private.h>
#include <alarm.h>
#include <feedback.h>

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

typedef enum
{
	SETTING_NOTI_STATUS_OFF    = 0,
	SETTING_NOTI_STATUS_GLOBAL = 1, 
	SETTING_NOTI_STATUS_EMAIL  = 2, 
} EMAIL_NOTI_STATUS;

#ifdef Min
#undef Min
#endif

INTERNAL_FUNC int  emcore_start_thread_for_alerting_new_mails(int *err_code);
INTERNAL_FUNC void emcore_start_alert();
