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
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <pthread.h>

#include "email-ipc-build.h"
#include "email-ipc-api-info.h"
#include "email-ipc-socket.h"
#include "email-stub-task.h"
#include "email-stub-task-manager.h"
#include "email-stub-socket.h"

#include "email-debug-log.h"
#include "email-proxy-socket.h"

#define MAX_EPOLL_EVENT 100

static int stub_socket = 0;
static pthread_t stub_socket_thread = 0;
static bool stop_thread = false;

GList *connected_fd = NULL;

static void *emipc_stub_socket_thread_proc();

EXPORT_API bool emipc_start_stub_socket()
{
	bool ret = true;

	ret = emipc_init_email_socket(&stub_socket);
	if (!ret) {
		EM_DEBUG_EXCEPTION("emipc_init_email_socket failed");
		return ret;
	}

	ret = emipc_start_stub_socket_thread();
	if (!ret) {
		EM_DEBUG_EXCEPTION("emipc_start_stub_socket_thread failed");
		return ret;
	}

	ret = emipc_start_task_thread();
	if (!ret) {
		EM_DEBUG_EXCEPTION("emipc_start_task_thread failed");
		return ret;
	}

	return ret;
}

EXPORT_API bool emipc_start_stub_socket_thread()
{
	EM_DEBUG_LOG("[IPCLib] emipc_email_stub_socket_thread start");
	if (stub_socket_thread)
		return true;
		
	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&stub_socket_thread, &thread_attr, &emipc_stub_socket_thread_proc, NULL) != 0) {
		EM_DEBUG_EXCEPTION("[IPCLib] emipc_start_stub_socket_thread() - fail to create a thread");
		return false;
	}
	return true;
}

EXPORT_API bool emipc_stop_stub_socket_thread()
{
	if (stub_socket_thread)
		stop_thread = true;

	return true;
}

static void *emipc_stub_socket_thread_proc()
{
	emipc_wait_for_ipc_request();
	stub_socket_thread = 0;
	return NULL;
}

EXPORT_API void emipc_wait_for_ipc_request()
{
	struct epoll_event ev = {0};
	int epfd = 0;
	int event_num = 0;
	struct epoll_event events[MAX_EPOLL_EVENT] = {{0}, };
	char *ipc_socket_path = NULL;
	char uid_string[12] = {0,};

	if (!stub_socket) {
		EM_DEBUG_EXCEPTION("Server Socket is not initialized");
		return;
	}

	SNPRINTF(uid_string, sizeof(uid_string), "%d", getuid());
	ipc_socket_path = g_strconcat(EM_SOCKET_USER_PATH, "/", uid_string, "/", EM_SOCKET_PATH_NAME, NULL);
	EM_DEBUG_LOG("ipc_socket_path : [%s]", ipc_socket_path);

	emipc_open_email_socket(stub_socket, ipc_socket_path);
	
	EM_SAFE_FREE(ipc_socket_path);

	epfd = epoll_create(MAX_EPOLL_EVENT);
	if (epfd < 0) {
		EM_DEBUG_EXCEPTION("epoll_ctl error [%d]", errno);
		EM_DEBUG_CRITICAL_EXCEPTION("epoll_create error [%d]", errno);
		return;
	}

	ev.events = EPOLLIN;
	ev.data.fd = stub_socket;
	
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, stub_socket, &ev) == -1) {
		EM_DEBUG_EXCEPTION("epoll_ctl error [%d]", errno);
		EM_DEBUG_CRITICAL_EXCEPTION("epoll_ctl error [%d]", errno); 	
	}
	while (!stop_thread) {
		int i = 0;

		event_num = epoll_wait(epfd, events, MAX_EPOLL_EVENT, -1);
		
		if (stop_thread) {
			EM_DEBUG_LOG ("IPC hanlder thread is going to be shut down");
			break;
		}

		if (event_num == -1) {
			if (errno != EINTR ) {
				EM_DEBUG_EXCEPTION("epoll_wait error [%d]", errno);
				EM_DEBUG_CRITICAL_EXCEPTION("epoll_wait error [%d]", errno);
			}
		}
		else {
			for (i = 0; i < event_num; i++) {
				int event_fd = events[i].data.fd;

				if (event_fd == stub_socket) { /*  if it is socket connection request */
					int cfd = emipc_accept_email_socket (stub_socket);
					if (cfd < 0) {
						EM_DEBUG_EXCEPTION ("emipc_accept_email_socket error [%d]", cfd);
						/* EM_DEBUG_CRITICAL_EXCEPTION ("accept failed: %s[%d]", EM_STRERROR(errno_buf), errno);*/
						continue;
					}
					ev.events = EPOLLIN;
					ev.data.fd = cfd;
					if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) == -1) {
						EM_DEBUG_EXCEPTION("epoll_ctl error [%d]", errno);
						/*EM_DEBUG_CRITICAL_EXCEPTION("epoll_ctl failed:%s[%d]", EM_STRERROR(errno_buf), errno);*/
						continue;
					}
				} 
				else {
					int recv_len;
					char *sz_buf = NULL;
					
					recv_len = emipc_recv_email_socket(event_fd, &sz_buf);
					
					if(recv_len > 0) {
						EM_DEBUG_LOG("[IPCLib]Stub Socket Recv [Socket ID = %d], [recv_len = %d]", event_fd, recv_len);

						/* IPC request stream is at least 16byte */
						if (recv_len >= sizeof(int) * eSTREAM_DATA) {
							int ret = 0;
							ret = emipc_create_task((unsigned char *)sz_buf, event_fd);
							if (ret == EMAIL_ERROR_PERMISSION_DENIED) {
								if (epoll_ctl(epfd, EPOLL_CTL_DEL, event_fd, events) == -1) {
									EM_DEBUG_EXCEPTION("epoll_ctl error [%d]", errno);
									EM_DEBUG_CRITICAL_EXCEPTION("epoll_ctl error [%d]", errno);
								}
								EM_SAFE_CLOSE (event_fd);
							}
						} 
						else
							EM_DEBUG_LOG("[IPCLib] Stream size is less than default size");
					} 
					else if( recv_len == 0 ) { /* client shut down connection */
						EM_DEBUG_LOG("[IPCLib] Client closed connection [%d]", event_fd);
						if (epoll_ctl(epfd, EPOLL_CTL_DEL, event_fd, events) == -1) {
							EM_DEBUG_EXCEPTION("epoll_ctl error [%d]", errno);
							EM_DEBUG_CRITICAL_EXCEPTION("epoll_ctl error [%d]", errno);
						}
						emipc_close_fd_in_task_queue (event_fd);
						EM_SAFE_CLOSE (event_fd);
					} 
					else { /* read errs */
						EM_DEBUG_EXCEPTION ("[IPCLib] read err[%d] fd[%d]", recv_len, event_fd);
						if (epoll_ctl(epfd, EPOLL_CTL_DEL, event_fd, events) == -1) {
							EM_DEBUG_EXCEPTION("epoll_ctl error [%d]", errno);
							EM_DEBUG_CRITICAL_EXCEPTION("epoll_ctl error [%d]", errno);
						}
						emipc_close_fd_in_task_queue (event_fd);
						EM_SAFE_CLOSE (event_fd);
					}
					EM_SAFE_FREE(sz_buf);
				}
			}
		}
	}	
	emipc_end_all_proxy_sockets ();
	emipc_close_email_socket(&stub_socket);
	EM_DEBUG_LOG ("IPC hanlder thread is shut down");
}

EXPORT_API bool emipc_end_stub_socket()
{
	EM_DEBUG_FUNC_BEGIN ();
	
	/* stop IPC handler thread */
	emipc_stop_stub_socket_thread();
	stub_socket_thread = 0;

	/* stop task thread */
	emipc_stop_task_thread ();

	EM_DEBUG_FUNC_END ();		
	return true;
}

EXPORT_API int emipc_send_stub_socket(int sock_fd, void *data, int len)
{
	EM_DEBUG_FUNC_BEGIN("Stub socket sending %d bytes", len);

	int sending_bytes = 0;
	if (sock_fd >= 0) {
		sending_bytes = emipc_send_email_socket(sock_fd, data, len);
	}

	if (sending_bytes <= 0) {
		EM_DEBUG_EXCEPTION ("emipc_send_email_socket error [%d] fd [%d]", sending_bytes, sock_fd);
	}

	EM_DEBUG_FUNC_END("sending_bytes = %d", sending_bytes);
	return sending_bytes;
}
/* stub socket accpet, recv, send */

