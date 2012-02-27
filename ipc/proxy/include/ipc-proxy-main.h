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



#ifndef	_IPC_PROXY_MAIN_H_
#define	_IPC_PROXY_MAIN_H_

#include "ipc-library.h"
#include <pthread.h>
#include "cm-list.h"
#include "cm-sys-msg-queue.h"
#include "ipc-api-info.h"
#include "ipc-proxy-socket.h"
#include "ipc-callback-info.h"
#include "emf-mutex.h"


class ipcEmailProxyMain
{
public:
	ipcEmailProxyMain();
	virtual ~ipcEmailProxyMain();

private:
	static ipcEmailProxyMain* s_pThis;
	int 			                m_nReference;
	ipcEmailProxySocket		   *m_pSyncSocket;
	bool				              m_bRecvStopFlag;
	cmSysMsgQueue		         *m_pProxyMessageQ;
	int 				              m_nAPPID;
	Mutex	                    mx;

private:
	bool RegisterTask(ipcEmailAPIInfo *a_pApiInfo);

public:
	
	static ipcEmailProxyMain* Instance();
	static void FreeInstance();
	int Initialize();
	int Finalize();
	bool ExecuteAPI(ipcEmailAPIInfo* pApi);
	bool Dispatch();

};	
#endif/* _IPC_PROXY_MAIN_H_ */

