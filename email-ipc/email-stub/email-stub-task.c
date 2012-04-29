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



#include <sys/socket.h>
#include <malloc.h>

#include "email-ipc.h"
#include "email-ipc-param-list.h"
#include "email-ipc-build.h"
#include "email-stub-task.h"
#include "email-stub-main.h"

#include "email-api.h"
#include "email-debug-log.h"

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
		emipc_parse_stream_of_api_info(task->api_info, ePARAMETER_IN, stream);
		emipc_set_response_id_of_api_info(task->api_info, response_id);
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
	EM_DEBUG_LOG("[IPCLib] starting a new task...");

	int app_id = emipc_get_app_id_of_api_info(task->api_info);
	int api_id = emipc_get_api_id_of_api_info(task->api_info);

	if (app_id > 0) {
		EM_DEBUG_LOG("[IPCLib] This task (%s) is for async. Response ID [%d]", EM_APIID_TO_STR(api_id), api_id);
		if (!emipc_set_response_info(app_id, api_id)) {
			EM_DEBUG_EXCEPTION("emipc_set_response_info failed");
			return false;
		}
	}
	return emipc_execute_api_proxy_to_stub(task->api_info);
}

