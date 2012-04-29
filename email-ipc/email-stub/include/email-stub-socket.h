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



#ifndef	_IPC_STUB_SOCKET_H_
#define	_IPC_STUB_SOCKET_H_

#include "email-types.h"
/*
typedef struct {
	emipc_email_socket sock_fd;
	pthread_t	stub_socket_thread;
	bool		stop_thread;
	emipc_email_task_manager	*task_manager;
} emipc_email_stub_socket;
*/

EXPORT_API bool emipc_start_stub_socket();

EXPORT_API bool emipc_start_stub_socket_thread();

EXPORT_API bool emipc_stop_stub_socket();

EXPORT_API void emipc_wait_for_ipc_request();

EXPORT_API bool emipc_end_stub_socket();

EXPORT_API int emipc_send_stub_socket(int sock_fd, void *data, int len);

#endif	/* _IPC_STUB_SOCKET_H_ */


