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



#ifndef	_IPC_STUB_MAIN_H_
#define	_IPC_STUB_MAIN_H_

#include "ipc-library.h"
#include "cm-sys-msg-queue.h"
#include "cm-list.h"
#include "ipc-stub-socket.h"

class ipcEmailAPIInfo;

class ipcEmailStubMain
{
public:
	ipcEmailStubMain();
	virtual ~ipcEmailStubMain();

private:
	static ipcEmailStubMain *s_pThis;
	PFN_EXECUTE_API		       m_pfnAPIMapper;
	ipcEmailStubSocket*		   m_pStubSocket;

	cmSysMsgQueue		        *m_pMsgSender;
	cmList				          *m_pResponseList;
public:
	static ipcEmailStubMain* Instance();
	static void FreeInstance();

	bool Initialize(PFN_EXECUTE_API a_pfnAPIMapper);
	bool Finalize();

	bool ExecuteAPIProxyToStub(ipcEmailAPIInfo *a_pAPI);
	bool ExecuteAPIStubToProxy(ipcEmailAPIInfo *a_pAPI);
	bool SetResponseInfo(long a_nResponseID, long a_nAPIID);
};	

#endif	/* _IPC_STUB_MAIN_H_ */


