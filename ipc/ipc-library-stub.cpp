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


#include "include/ipc-library.h"
#include "stub/include/ipc-stub-main.h"
#include "include/ipc-library-build.h"
#include "emf-dbglog.h"

EXPORT_API bool ipcEmailStub_Initialize(PFN_EXECUTE_API a_pfnAPIMapper)
{
	EM_DEBUG_LOG("[IPCLib] ipcEmailStub_Initialize ");

	return ipcEmailStubMain::Instance()->Initialize(a_pfnAPIMapper);
}

EXPORT_API bool ipcEmailStub_Finalize(void)
{

	ipcEmailStubMain::Instance()->Finalize();
	ipcEmailStubMain::Instance()->FreeInstance();
	return true;
}

EXPORT_API bool ipcEmailStub_ExecuteAPI(HIPC_API a_hAPI)
{
	EM_DEBUG_LOG("ipcEmailStub_ExecuteAPI [%x]", a_hAPI);
	ipcEmailAPIInfo *pApi = static_cast<ipcEmailAPIInfo *>(a_hAPI);
	if(pApi == NULL)
		return false;
	return ipcEmailStubMain::Instance()->ExecuteAPIStubToProxy(pApi);

}    

