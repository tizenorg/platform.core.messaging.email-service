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


#ifndef _IPC_TASK_H_
#define _IPC_TASK_H_

#include "email-ipc-api-info.h"
#include "email-types.h"

typedef struct {
	int response_channel;
	emipc_email_api_info *api_info;
} emipc_email_task;

EXPORT_API void emipc_free_email_task(emipc_email_task *task);

EXPORT_API bool emipc_parse_stream_email_task(emipc_email_task *task, void *stream, int response_id);

EXPORT_API emipc_email_api_info *emipc_get_api_info(emipc_email_task *task);

EXPORT_API int emipc_get_response_channel(emipc_email_task *task);

EXPORT_API bool emipc_run_task(emipc_email_task *task);

#endif	/* _IPC_TASK_H */


