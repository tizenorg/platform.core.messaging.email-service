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


#ifndef _IPC_SOCKET_H_ 
#define _IPC_SOCKET_H_ 

#include "email-types.h"

#define SOCKET_IP		"127.0.0.1"
#define SOCKET_PORT		9000

#define BUF_SIZE			204800
#define OPT_USEC		100000

#define EM_SOCKET_USER_PATH tzplatform_mkpath(TZ_SYS_RUN, "user")
#define EM_SOCKET_PATH_NAME ".emailfw_socket"

EXPORT_API bool emipc_init_email_socket(int *fd);

EXPORT_API void emipc_close_email_socket(int *fd);

EXPORT_API int emipc_send_email_socket(int fd, unsigned char *buf, int len);

EXPORT_API int emipc_recv_email_socket(int fd, char **buf);

EXPORT_API int emipc_accept_email_socket(int fd);

EXPORT_API int emipc_open_email_socket(int fd, const char *path);

EXPORT_API int emipc_connect_email_socket(int fd);

#endif /*  _IPC_SOCKET_H_		 */



	


