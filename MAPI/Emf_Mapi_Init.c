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



/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with Email Engine.
 * @file		Emf_Mapi_Init.c
 * @brief 		This file contains the data structures and interfaces of Email FW intialization related Functionality provided by 
 *			Email Engine . 
 */
 
#include <Emf_Mapi.h>
#include "string.h"
#include "Msg_Convert.h"
#include "em-storage.h"
#include "ipc-library.h"
#include <sqlite3.h>


EXPORT_API int email_open_db(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int error  = EMF_ERROR_NONE;
	
	if (em_storage_db_open(&error) == NULL)
		EM_DEBUG_EXCEPTION("em_storage_db_open failed [%d]", error);
	error = em_storage_get_emf_error_from_em_storage_error(error);
	
	EM_DEBUG_FUNC_END("error [%d]", error);

	return error;	
}

EXPORT_API int email_close_db(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int error  = EMF_ERROR_NONE;

	if ( !em_storage_db_close(&error)) 
		EM_DEBUG_EXCEPTION("em_storage_db_close failed [%d]", error);
	error = em_storage_get_emf_error_from_em_storage_error(error);
	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;	
}


EXPORT_API int email_service_begin(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = -1;

	signal(SIGPIPE, SIG_IGN); /*  to ignore signal 13(SIGPIPE) */
	
	ret = ipcEmailProxy_Initialize();
	if (ret != EMF_ERROR_NONE)
		EM_DEBUG_FUNC_END("err[%d]", ret);
	
	return ret;
}


EXPORT_API int email_service_end(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = -1;
	
	ret = ipcEmailProxy_Finalize();
	if (ret != EMF_ERROR_NONE)
		EM_DEBUG_FUNC_END("err[%d]", ret);
	
	return ret;
}

/* API - Exposed to External Application- core team.Creates all Email DB tables [ EXTERNAL] */


EXPORT_API int email_init_storage(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int error  = EMF_ERROR_NONE;
	
	if (!em_storage_create_table(EMF_CREATE_DB_CHECK, &error))  {
		EM_DEBUG_EXCEPTION("\t em_storage_create_table failed - %d\n", error);
	}
	error = em_storage_get_emf_error_from_em_storage_error(error);
	EM_DEBUG_FUNC_END("error[%d]", error);
	return error;
}

EXPORT_API int email_ping_service(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int error  = EMF_ERROR_NONE;
	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_PING_SERVICE);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);
		
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION(" email_ping_service -- ipcEmailProxy_ExecuteAPIFail \n ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	error = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0); 
	
	ipcEmailAPI_Destroy(hAPI);

	hAPI = NULL;

	EM_DEBUG_FUNC_END("err[%d]", error);
	return error;
}
