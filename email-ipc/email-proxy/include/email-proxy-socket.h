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



#ifndef	_IPC_PROXY_SOCKET_H_
#define	_IPC_PROXY_SOCKET_H_

#include "email-types.h"

EXPORT_API int emipc_start_proxy_socket();

EXPORT_API bool emipc_end_proxy_socket();

EXPORT_API bool emipc_end_all_proxy_sockets();

EXPORT_API int emipc_send_proxy_socket(unsigned char *data, int len);

EXPORT_API int emipc_get_proxy_socket_id();

EXPORT_API int emipc_recv_proxy_socket(char **data);

#endif	/* _IPC_PROXY_SOCKET_H_ */


