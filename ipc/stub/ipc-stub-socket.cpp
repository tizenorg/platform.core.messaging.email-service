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




#include <unistd.h>
#include "ipc-task.h"
#include "ipc-library-build.h"
#include "emf-dbglog.h"
#include "ipc-stub-socket.h"
#include <errno.h>
#include <sys/epoll.h>

ipcEmailStubSocket::ipcEmailStubSocket()
{
	m_pSocket = NULL;	
	m_hStubSocketThread = 0;
	m_pTaskManager = NULL;
	m_bStopThread = false;
}

ipcEmailStubSocket::~ipcEmailStubSocket()
{
	End();
}

bool ipcEmailStubSocket::Start()
{
	m_pTaskManager = new ipcEmailTaskManager();
	if(m_pTaskManager == NULL)
		return false;

	m_pSocket = new ipcEmailSocket();
	if( m_pSocket == NULL )
		return false;

	if( m_pSocket ) {
		StartStubSocketThread();
	}

	return m_pTaskManager->StartTaskThread();
}

/* stub socket accpet, recv, send */
bool ipcEmailStubSocket::StartStubSocketThread()
{
	EM_DEBUG_LOG("[IPCLib] ipcEmailStubSocket::StartStubSocketThread \n");
	if(m_hStubSocketThread)
		return true;	

	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);	
	if (pthread_create(&m_hStubSocketThread, &thread_attr, &StubSocketThreadProc, (void*)this) != 0)  {
		EM_DEBUG_EXCEPTION("[IPCLib] ipcEmailStubSocket::StartStubSocketThread() - fail to create a thread \n");
		return false; 
	}
	return true;

}

bool ipcEmailStubSocket::StopStubSocketThread()
{
	if(m_hStubSocketThread)
		m_bStopThread = true;

	return true;
}

void* ipcEmailStubSocket::StubSocketThreadProc(void *a_pOwner)
{
	ipcEmailStubSocket *pStubSocket = (ipcEmailStubSocket*)a_pOwner;
	if(pStubSocket) {
		pStubSocket->WaitForIPCRequest();
		pStubSocket->m_hStubSocketThread = 0; /*  prevent 20071030 */
	}	
	return NULL;
}

#define MAX_EPOLL_EVENT 50

void ipcEmailStubSocket::WaitForIPCRequest()
{
	if ( !m_pSocket ) {
		EM_DEBUG_EXCEPTION("server socket is not initialized");
		return;
	}

	m_pSocket->open(EM_SOCKET_PATH);

	struct epoll_event ev = {0};
	struct epoll_event events[MAX_EPOLL_EVENT] = {{0}, };
	int epfd = epoll_create(MAX_EPOLL_EVENT);
	if( epfd < 0 ) {
		EM_DEBUG_EXCEPTION("epoll_create: %s[%d]", EM_STRERROR(errno), errno);
		EM_DEBUG_CRITICAL_EXCEPTION("epoll_create: %s[%d]", EM_STRERROR(errno), errno);
		abort();
	}
	int server_fd = m_pSocket->GetSocketID();
	ev.events = EPOLLIN;
	ev.data.fd = server_fd;
	if( epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) == -1 ) {
		EM_DEBUG_EXCEPTION("epoll_ctl: %s[%d]", EM_STRERROR(errno), errno);
		EM_DEBUG_CRITICAL_EXCEPTION("epoll_ctl: %s[%d]", EM_STRERROR(errno), errno);
	}

	int event_num = 0;
	while(1){

		event_num = epoll_wait(epfd, events, MAX_EPOLL_EVENT, -1);
		
		if( event_num == -1 ) {
			EM_DEBUG_EXCEPTION("epoll_wait: %s[%d]", EM_STRERROR(errno), errno);
			EM_DEBUG_CRITICAL_EXCEPTION("epoll_wait: %s[%d]", EM_STRERROR(errno), errno);
			if (errno == EINTR) continue; /* resume when interrupted system call*/
			else abort();
		}  else
			for (int i=0 ; i < event_num; i++) {
				int event_fd = events[i].data.fd;

				if (event_fd == server_fd) { /*  if it is socket connection request */
					int cfd = m_pSocket->accept();
					if( cfd < 0 ) {
						EM_DEBUG_EXCEPTION("accept error: %s[%d]", EM_STRERROR(errno), errno);
						EM_DEBUG_CRITICAL_EXCEPTION("accept error: %s[%d]", EM_STRERROR(errno), errno);
						/*  abort(); */
					}
					ev.events = EPOLLIN;
					ev.data.fd = cfd;
					epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
				} else {
					int nRecvLen;
					char *szBuff = NULL;
					
					nRecvLen = m_pSocket->Recv(event_fd, &szBuff);
					
					if(nRecvLen > 0) {
						EM_DEBUG_LOG("====================================================================");
						EM_DEBUG_LOG("[IPCLib]Stub Socket Recv [Socket ID = %d], [RecvLen = %d]", event_fd, nRecvLen);
						EM_DEBUG_LOG("====================================================================");
						m_pTaskManager->CreateTask((unsigned char*)szBuff, event_fd);
					}  else {
						EM_DEBUG_LOG("[IPCLib] Socket [%d] removed - [%d] ", event_fd, nRecvLen);
						epoll_ctl(epfd, EPOLL_CTL_DEL, event_fd, events);
						close(event_fd);
					} 
					if(szBuff)
						delete []szBuff;
				}
			}
	}
}


bool ipcEmailStubSocket::End()
{
	EM_DEBUG_FUNC_BEGIN();
	
	if(m_pSocket) {
		m_pSocket->Close();	/*  prevent 20071030 */
		delete m_pSocket;
		m_pSocket = NULL;
	}

	if(m_hStubSocketThread) {
		StopStubSocketThread();
		pthread_cancel(m_hStubSocketThread);
		m_hStubSocketThread = 0;
	}
	if(m_pTaskManager) {
		m_pTaskManager->StopTaskThread();
		delete m_pTaskManager;
		m_pTaskManager = NULL;
	}	
	
	return true;
}

int ipcEmailStubSocket::Send(int fd, void* pData , int a_nLen)
{
	EM_DEBUG_FUNC_END();

	EM_DEBUG_LOG("Stub socket sending %d bytes", a_nLen);
	int sending_bytes = m_pSocket->Send(fd, (char*)pData, a_nLen);

	EM_DEBUG_FUNC_END();
	return sending_bytes;
}

