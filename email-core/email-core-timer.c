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
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/timeb.h>
#include "email-core-timer.h"
#include "email-debug-log.h"


typedef struct
{
	EMAIL_TIMER_CALLBACK user_callback_function;
	void *callback_user_data;
	int time_id;
}em_timer_callback_data;


INTERNAL_FUNC int emcore_timer_ex_callback(void *a_pData)
{
	EM_DEBUG_LOG("[emcore_timer_ex_callback] enter\n");
	void *pUserData = NULL;

#if !GLIB_CHECK_VERSION(2, 31, 0)
	g_thread_init(NULL);
#endif

	em_timer_callback_data *pTimerData = (em_timer_callback_data *)a_pData;
	if (pTimerData != NULL)
	{
		EMAIL_TIMER_CALLBACK pfn_UserCB = pTimerData->user_callback_function;
		pUserData = pTimerData->callback_user_data;
		if (pUserData)
			EM_DEBUG_LOG("emcore_timer_ex_callback >>> data  :  %s", (char *)pTimerData->callback_user_data);

		EM_SAFE_FREE(pTimerData);
		pfn_UserCB(pUserData);
	}

 #if !GLIB_CHECK_VERSION(2, 31, 0)
	g_thread_init(NULL);
#endif

	EM_DEBUG_LOG("[emcore_timer_ex_callback] leave\n");

	if (pUserData)
		return 0;
	else
        return 1;
}

INTERNAL_FUNC int emcore_set_timer_ex(long a_nSetTimeValue, EMAIL_TIMER_CALLBACK a_pCallBack, void *a_pData)
{
	EM_DEBUG_LOG("emcore_set_timer_ex %d", a_nSetTimeValue);
	em_timer_callback_data *pTimerData = NULL;
	pTimerData = malloc(sizeof(em_timer_callback_data));
	if (!pTimerData)
		return -1;

    memset(pTimerData, 0x00, sizeof(em_timer_callback_data));

	pTimerData->user_callback_function = a_pCallBack;
	if (a_pData) {
		pTimerData->callback_user_data = a_pData;
	}
	pTimerData->time_id = g_timeout_add(a_nSetTimeValue, emcore_timer_ex_callback, pTimerData);
	return pTimerData->time_id;
}

INTERNAL_FUNC void emcore_kill_timer_ex(int a_nTimerID)
{
	EM_DEBUG_LOG("[emcore_kill_timer_ex] a_nTimerID %d", a_nTimerID);
	g_source_remove(a_nTimerID);
}

