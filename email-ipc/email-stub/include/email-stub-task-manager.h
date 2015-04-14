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


#ifndef _IPC_TASK_MANAGER_H_
#define _IPC_TASK_MANAGER_H_

#include "email-types.h"

#define IPC_TASK_MAX	64

EXPORT_API bool emipc_start_task_thread();

EXPORT_API void emipc_terminate_task_thread();

EXPORT_API bool emipc_stop_task_thread();

EXPORT_API void *emipc_do_task_thread();

EXPORT_API bool emipc_create_task(unsigned char *task_stream, int response_channel, int permission);

#endif	/* _IPC_TASK_MANAGER_H_ */


