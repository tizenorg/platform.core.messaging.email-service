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



#include <sys/socket.h>
#include <malloc.h>

#include "email-ipc.h"
#include "email-ipc-param-list.h"
#include "email-ipc-build.h"
#include "email-stub-task.h"
#include "email-stub-main.h"

#include "email-debug-log.h"

EXPORT_API void emipc_free_email_task(emipc_email_task *task)
{
	EM_DEBUG_FUNC_BEGIN("task [%p]", task);
	
	if (!task) {
		EM_DEBUG_EXCEPTION("Invalid parameter.");
		return;
	}

	emipc_free_api_info(task->api_info);
	EM_SAFE_FREE(task->api_info);
}

EXPORT_API bool emipc_parse_stream_email_task(emipc_email_task *task, void *stream, int response_id)
{
	EM_DEBUG_FUNC_BEGIN();

	task->api_info = (emipc_email_api_info *)malloc(sizeof(emipc_email_api_info));
	if (task->api_info == NULL) {
		EM_DEBUG_EXCEPTION("Malloc failed.");
		return false;
	}
	memset(task->api_info, 0x00, sizeof(emipc_email_api_info));
	
	if (task->api_info) {
		if (!emipc_deserialize_api_info(task->api_info, ePARAMETER_IN, stream)) {
			EM_DEBUG_EXCEPTION("emipc_deserialize_api_info failed");
			return false;
		}
		task->api_info->response_id = response_id;
		return true;
	}
	return false;
}

EXPORT_API emipc_email_api_info *emipc_get_api_info(emipc_email_task *task)
{
	return task->api_info;
}

EXPORT_API int emipc_get_response_channel(emipc_email_task *task)
{
	return task->response_channel;
}

EXPORT_API bool emipc_run_task(emipc_email_task *task)
{
	int api_id = task->api_info->api_id;
	int app_id = task->api_info->app_id;
	int res_id = task->api_info->response_id;

	EM_DEBUG_LOG_SEC("[IPCLib] Processing task: API_ID[%s][0x%x] RES_ID[%d] APP_ID[%d] ", 
                                   EM_APIID_TO_STR (api_id), api_id, res_id, app_id);

	if (!emipc_execute_api_proxy_to_stub(task->api_info)) {
		EM_DEBUG_EXCEPTION("emipc_execute_api_proxy_to_stub failed");
		return false;
	}
	
	return true;
}

