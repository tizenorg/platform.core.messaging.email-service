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



#include <sys/socket.h>
#include "ipc-library.h"
#include "ipc-task.h"
#include "ipc-stub-main.h"
#include "ipc-param-list.h"
#include "ipc-library-build.h"

ipcEmailTask::ipcEmailTask()
{
	EM_DEBUG_FUNC_BEGIN();
	m_nResponseChannel = 0;
	m_pAPIInfo = NULL;
	EM_DEBUG_LOG("pcTask::ipcEmailTask()  ");
}

ipcEmailTask::~ipcEmailTask()
{
	EM_DEBUG_FUNC_BEGIN();
	if(m_pAPIInfo) {
		delete m_pAPIInfo;
		m_pAPIInfo = NULL;
	}
}

bool ipcEmailTask::ParseStream(void* a_pStream, int a_nResponseID)
{
	EM_DEBUG_FUNC_BEGIN();
	if(m_pAPIInfo)
		delete m_pAPIInfo;

	m_pAPIInfo = new ipcEmailAPIInfo();

	if(m_pAPIInfo) {
		m_pAPIInfo->ParseStream(ePARAMETER_IN, a_pStream);
		m_pAPIInfo->SetResponseID(a_nResponseID);
		return true;
	}
	return false;
}

ipcEmailAPIInfo *ipcEmailTask::GetAPIInfo()
{
	return m_pAPIInfo;
}

int  ipcEmailTask::GetResponseChannel()
{
	return m_nResponseChannel;
}

bool ipcEmailTask::Run()
{
	EM_DEBUG_LOG("[IPCLib]  Starting a new task...");
	
	int nAPPID =  m_pAPIInfo->GetAPPID();
	if(nAPPID > 0)	/* If the call is for async, APP ID is passed. */ {	/* It means this call is for async, response ID set. */
		EM_DEBUG_LOG("[IPCLib] This task (%s) is for async. Response ID [%d]", EM_APIID_TO_STR(m_pAPIInfo->GetAPIID()), m_pAPIInfo->GetAPIID());
		ipcEmailStubMain::Instance()->SetResponseInfo(nAPPID, m_pAPIInfo->GetAPIID());	
	}
	
	return ipcEmailStubMain::Instance()->ExecuteAPIProxyToStub(m_pAPIInfo);
	
}


