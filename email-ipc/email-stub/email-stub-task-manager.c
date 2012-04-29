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



#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <glib.h>

#include "email-stub-task-manager.h"
#include "email-stub-task.h"
#include "email-ipc-build.h"

#include "email-debug-log.h"
#include "email-api.h"
#include "email-internal-types.h"

static pthread_t task_thread = 0;
static bool stop_flag = false;
static GQueue *task_queue = NULL;

pthread_mutex_t ipc_task_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ipc_task_cond = PTHREAD_COND_INITIALIZER;

EXPORT_API bool emipc_start_task_thread()
{
	EM_DEBUG_FUNC_BEGIN();
	if (task_thread)
		return true;

	task_queue = g_queue_new();

	if (pthread_create(&task_thread, NULL, &emipc_do_task_thread, NULL) != 0) {
		EM_DEBUG_LOG("Worker thread creation failed: %s", strerror(errno));
		return false;	
	}

	return true;
}

EXPORT_API void emipc_terminate_task_thread()
{
	emipc_stop_task_thread();
	pthread_cancel(task_thread);

	emipc_email_task *task = (emipc_email_task *)g_queue_pop_head(task_queue);
	while (task) {
		EM_SAFE_FREE(task);
		task = (emipc_email_task *)g_queue_pop_head(task_queue);
	}
	g_queue_free(task_queue);
}

EXPORT_API bool emipc_stop_task_thread()
{
	stop_flag = true;
	return true;
}


EXPORT_API void *emipc_do_task_thread()
{
	EM_DEBUG_FUNC_BEGIN();
	
	emipc_email_task *task = NULL;

	while (!stop_flag) {
		ENTER_CRITICAL_SECTION(ipc_task_mutex);
		while (g_queue_is_empty(task_queue)) {
			EM_DEBUG_LOG("Blocked until new task arrivers %p.", &ipc_task_cond);
			SLEEP_CONDITION_VARIABLE(ipc_task_cond, ipc_task_mutex);
		}
		
		task = (emipc_email_task *)g_queue_pop_head(task_queue);
		LEAVE_CRITICAL_SECTION(ipc_task_mutex);

		if (task) {
			emipc_run_task(task);
			EM_SAFE_FREE(task);
		}
	}
	
	return NULL;			
}

/* code for ipc handler */
EXPORT_API bool emipc_create_task(unsigned char *task_stream, int response_channel)
{
	emipc_email_task *task = NULL;
	bool ret = true;

	task = (emipc_email_task *)malloc(sizeof(emipc_email_task));
	if (task == NULL) {
		EM_DEBUG_EXCEPTION("Malloc failed.");
		ret = false;
	} else {
		emipc_parse_stream_email_task(task, task_stream, response_channel);
		
		EM_DEBUG_LOG("[IPCLib] ======================================================");
		EM_DEBUG_LOG("[IPCLib] Register new task : %p", task);
		EM_DEBUG_LOG("[IPCLib] Task API ID : %s", EM_APIID_TO_STR(emipc_get_api_info(task)->api_id));
		EM_DEBUG_LOG("[IPCLib] Task Response ID : %d", EM_APIID_TO_STR(emipc_get_api_info(task)->response_id));
		EM_DEBUG_LOG("[IPCLib] Task APP ID : %d", emipc_get_api_info(task)->app_id);
		EM_DEBUG_LOG("[IPCLib] ======================================================");

		ENTER_CRITICAL_SECTION(ipc_task_mutex);
		g_queue_push_head(task_queue, (void *)task);
		
		WAKE_CONDITION_VARIABLE(ipc_task_cond);
		LEAVE_CRITICAL_SECTION(ipc_task_mutex);
	}
	return ret;
}

