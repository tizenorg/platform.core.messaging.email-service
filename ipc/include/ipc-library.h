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


#ifndef __IPC_LIBRARY_H
#define __IPC_LIBRARY_H

#ifdef __cplusplus
extern "C" {
#endif

#define EM_PROXY_IF_NULL_RETURN_VALUE(expr1, expr2, val) {					\
	if (!expr1&& expr2) {																				\
		EM_DEBUG_LOG ("EM_PROXY_IF_NULL_RETURN_VALUE : PARAM IS NULL \n");								\
		ipcEmailAPI_Destroy(expr2);											\
		return val;																	\
	}; }

	
/*  ------------------------------------------------------------------------------------------------------------ */
/* 	Type Definitio */
/*  ------------------------------------------------------------------------------------------------------------ */
typedef enum {
	ePARAMETER_IN = 0,
	ePARAMETER_OUT,
} EPARAMETER_DIRECTION;

typedef void* HIPC_API;
typedef void* HIPC_PARAMETER;
typedef void (*PFN_PROXY_CALLBACK)	(HIPC_API a_hAPI, void* pParam1, void* pParam2);
typedef void (*PFN_EXECUTE_API)		(HIPC_API a_hAPI);

/*  ------------------------------------------------------------------------------------------------------------ */
/* 	Proxy AP */
/*  ------------------------------------------------------------------------------------------------------------ */
int ipcEmailProxy_Initialize();
int ipcEmailProxy_Finalize(void);

bool ipcEmailProxy_ExecuteAPI(HIPC_API a_hAPI);

/*  ------------------------------------------------------------------------------------------------------------ */
/* 	Stub AP */
/*  ------------------------------------------------------------------------------------------------------------ */
bool ipcEmailStub_Initialize(PFN_EXECUTE_API a_pfnAPIMapper);
bool ipcEmailStub_Finalize(void);

bool ipcEmailStub_ExecuteAPI(HIPC_API a_hAPI);

/*  ------------------------------------------------------------------------------------------------------------ */
/* 	AP */
/*  ------------------------------------------------------------------------------------------------------------ */
HIPC_API	ipcEmailAPI_Create(long a_nAPIID);
void		ipcEmailAPI_Destroy(HIPC_API a_hAPI);

long			ipcEmailAPI_GetAPIID(HIPC_API a_hAPI);
long			ipcEmailAPI_GetAPPID(HIPC_API a_hAPI);

bool		ipcEmailAPI_AddParameter(HIPC_API a_hAPI, EPARAMETER_DIRECTION a_eDirection, void *a_pData, int a_nDataLength);
int 		ipcEmailAPI_GetParameterCount(HIPC_API a_hAPI, EPARAMETER_DIRECTION a_eDirection);
void*		ipcEmailAPI_GetParameter(HIPC_API a_hAPI, EPARAMETER_DIRECTION a_eDirection, int a_nParameterIndex);
int 		ipcEmailAPI_GetParameterLength(HIPC_API a_hAPI, EPARAMETER_DIRECTION a_eDirection, int a_nParameterIndex);


#ifdef __cplusplus
}
#endif
#endif


