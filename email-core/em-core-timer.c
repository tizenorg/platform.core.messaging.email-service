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
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/signal.h>
/* #include <pthread.h> */
#include <sys/timeb.h>
#include "em-core-timer.h"
#include "emf-dbglog.h"
#include <dbus/dbus-glib.h> 


typedef struct
{
	EMF_TIMER_CALLBACK user_callback_function;
	void *callback_user_data;
	int time_id;
}em_timer_callback_data;


EXPORT_API int
em_core_timer_ex_callback(void *a_pData)
{
	EM_DEBUG_LOG("[em_core_timer_ex_callback] enter\n");
	void *pUserData = NULL;


	g_thread_init(NULL);
	dbus_g_thread_init ();

	em_timer_callback_data *pTimerData = (em_timer_callback_data *)a_pData;
	if (pTimerData != NULL)
		{
		EMF_TIMER_CALLBACK pfn_UserCB = pTimerData->user_callback_function;
			pUserData = pTimerData->callback_user_data;
		if (pUserData)
			EM_DEBUG_LOG("em_core_timer_ex_callback >>> data  :  %s", (char *)pTimerData->callback_user_data);
			EM_SAFE_FREE(pTimerData);
			pfn_UserCB(pUserData);
		}
 
		
	g_thread_init(NULL);
	dbus_g_thread_init ();

	EM_DEBUG_LOG("[em_core_timer_ex_callback] leave\n");

	if (pUserData)
		return 0;
	else
			return 1;
}



EXPORT_API int
em_core_set_timer_ex(long a_nSetTimeValue, EMF_TIMER_CALLBACK a_pCallBack, void *a_pData)
{
	EM_DEBUG_LOG("em_core_set_timer_ex %d", a_nSetTimeValue);
	em_timer_callback_data *pTimerData = NULL;
	pTimerData = malloc(sizeof(em_timer_callback_data));
	char *data = NULL;
	if (!pTimerData)
		return -1;
	memset(pTimerData, 0x00, sizeof(em_timer_callback_data));
	if (a_pData)
		EM_DEBUG_LOG("em_core_set_timer_ex >>> data  :  %s", (char *)a_pData);

	pTimerData->user_callback_function = a_pCallBack;
	if (a_pData) {
		data = (char *) a_pData;
		pTimerData->callback_user_data = EM_SAFE_STRDUP(data);
	}
	pTimerData->time_id = g_timeout_add(a_nSetTimeValue, em_core_timer_ex_callback, pTimerData);
	return pTimerData->time_id;
}

EXPORT_API void
em_core_kill_timer_ex(int a_nTimerID)
{
	EM_DEBUG_LOG("[em_core_kill_timer_ex] a_nTimerID %d", a_nTimerID);
	g_source_remove(a_nTimerID);
}

