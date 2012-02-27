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



#include "include/ipc-library-build.h"
#include "include/ipc-library.h"
#include "proxy/include/ipc-proxy-main.h"
#include "api/include/ipc-api-info.h"
#include "api/include/ipc-param-list.h"
#include "socket/include/ipc-socket.h"
#include "emf-dbglog.h"

EXPORT_API ipcEmailParamList* _ipcAPI_GetParameters(HIPC_API a_hAPI, EPARAMETER_DIRECTION a_eDirection)
{
	EM_DEBUG_FUNC_BEGIN();
	ipcEmailAPIInfo *pAPI = (ipcEmailAPIInfo*)a_hAPI;
	if(pAPI) {
		return (ipcEmailParamList*)pAPI->GetParameters(a_eDirection);
	}
	return 0;
}

EXPORT_API HIPC_API ipcEmailAPI_Create(long a_nAPIID)
{

	EM_DEBUG_FUNC_BEGIN();

	ipcEmailAPIInfo *pApi = new ipcEmailAPIInfo();
	if(pApi == NULL) {
		return NULL;
	}

	pApi->SetAPIID(a_nAPIID);
	return (HIPC_API)pApi;
}

EXPORT_API void ipcEmailAPI_Destroy(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN("a_hAPI = %p", a_hAPI);
	ipcEmailAPIInfo *pAPI = (ipcEmailAPIInfo*)a_hAPI;
	if(pAPI) {
		delete pAPI;
	}
}


EXPORT_API long ipcEmailAPI_GetAPIID(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	ipcEmailAPIInfo *pAPI = (ipcEmailAPIInfo*)a_hAPI;
	if(pAPI) {
		return pAPI->GetAPIID();
	}
	return -1;
}


EXPORT_API long ipcEmailAPI_GetAPPID(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	ipcEmailAPIInfo *pAPI = (ipcEmailAPIInfo*)a_hAPI;
	if(pAPI) {
		return pAPI->GetAPPID();
	}
	return -1;
}

EXPORT_API bool ipcEmailAPI_AddParameter(HIPC_API a_hAPI, EPARAMETER_DIRECTION a_eDirection, void *a_pData, int a_nDataLength)
{
	EM_DEBUG_FUNC_BEGIN();

	ipcEmailParamList *pParameters = _ipcAPI_GetParameters(a_hAPI, a_eDirection);

	if(pParameters) {
		pParameters->AddParam(a_pData, a_nDataLength);
		return true;
	}
	return false;
}

EXPORT_API int ipcEmailAPI_GetParameterCount(HIPC_API a_hAPI, EPARAMETER_DIRECTION a_eDirection)
{
	EM_DEBUG_FUNC_BEGIN();
	
	ipcEmailParamList *pParameters = _ipcAPI_GetParameters(a_hAPI, a_eDirection);
	if(pParameters) {
		return pParameters->GetParamCount();
	}
	return -1;
}

EXPORT_API void* ipcEmailAPI_GetParameter(HIPC_API a_hAPI, EPARAMETER_DIRECTION a_eDirection, int a_nParameterIndex)
{
	EM_DEBUG_FUNC_BEGIN();
	ipcEmailParamList *pParameters = _ipcAPI_GetParameters(a_hAPI, a_eDirection);

	if(pParameters) {
		EM_DEBUG_FUNC_END("Suceeded");
		return pParameters->GetParam(a_nParameterIndex);
	}
	EM_DEBUG_FUNC_END("Failed");
	return 0;
}

EXPORT_API int ipcEmailAPI_GetParameterLength(HIPC_API a_hAPI, EPARAMETER_DIRECTION a_eDirection, int a_nParameterIndex)
{
	EM_DEBUG_FUNC_BEGIN();
	ipcEmailParamList *pParameters = _ipcAPI_GetParameters(a_hAPI, a_eDirection);
	if(pParameters) {
		EM_DEBUG_FUNC_END("Suceeded");
		return pParameters->GetParamLen(a_nParameterIndex);
	}
	EM_DEBUG_FUNC_END("Failed");
	return -1;
}





