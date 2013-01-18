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



/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with email-service.
 * @file		email-api-init.c
 * @brief 		This file contains the data structures and interfaces of Email FW intialization related Functionality provided by 
 *			email-service . 
 */
 
#include "email-api.h"
#include "string.h"
#include "email-convert.h"
#include "email-storage.h"
#include "email-ipc.h"
#include "email-core-task-manager.h"
#include <sqlite3.h>


EXPORT_API int email_open_db(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int error  = EMAIL_ERROR_NONE;
	
	if (emstorage_db_open(&error) == NULL)
		EM_DEBUG_EXCEPTION("emstorage_db_open failed [%d]", error);

	
	EM_DEBUG_FUNC_END("error [%d]", error);

	return error;	
}

EXPORT_API int email_close_db(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int error  = EMAIL_ERROR_NONE;

	if ( !emstorage_db_close(&error)) 
		EM_DEBUG_EXCEPTION("emstorage_db_close failed [%d]", error);

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;	
}


EXPORT_API int email_service_begin(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = -1;

	signal(SIGPIPE, SIG_IGN); /* to ignore signal 13(SIGPIPE) */
	
	ret = emipc_initialize_proxy();

	emcore_init_task_handler_array();

	EM_DEBUG_FUNC_END("err[%d]", ret);
	return ret;
}


EXPORT_API int email_service_end(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = -1;
	
	ret = emipc_finalize_proxy();
	if (ret != EMAIL_ERROR_NONE)
		EM_DEBUG_FUNC_END("err[%d]", ret);
	
	return ret;
}

/* API - Exposed to External Application- core team.Creates all Email DB tables [ EXTERNAL] */


EXPORT_API int email_init_storage(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int error  = EMAIL_ERROR_NONE;
	
	if (!emstorage_create_table(EMAIL_CREATE_DB_CHECK, &error))  {
		EM_DEBUG_EXCEPTION("emstorage_create_table failed [%d]", error);
	}

	EM_DEBUG_FUNC_END("error[%d]", error);
	return error;
}

EXPORT_API int email_ping_service(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int error  = EMAIL_ERROR_NONE;
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_PING_SERVICE);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);
		
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &error);
	
	emipc_destroy_email_api(hAPI);

	hAPI = NULL;

	EM_DEBUG_FUNC_END("err[%d]", error);
	return error;
}
