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

#include <pthread.h>
#include "ipc-library.h"
#include "ipc-socket.h"
#include "ipc-task-manager.h"


class ipcEmailStubSocket
{
public:
	ipcEmailStubSocket();
	virtual ~ipcEmailStubSocket();
	bool Start();
	bool End();
	int Send(int fd, void* pData , int a_nLen);
	void WaitForIPCRequest();

private:
	bool StartStubSocketThread();
	bool StopStubSocketThread();
	static void* StubSocketThreadProc(void *a_pOwner);


private:
	ipcEmailSocket *m_pSocket;
	pthread_t	m_hStubSocketThread;
	bool		m_bStopThread;
		
	ipcEmailTaskManager *m_pTaskManager;	

};

#endif	/* _IPC_STUB_SOCKET_H_ */


