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
 * @file		Emf_Mapi_Rule.c
 * @brief 		This file contains the data structures and interfaces of Rule related Functionality provided by 
 *			Email Engine . 
 */

#include <Emf_Mapi.h>
#include "string.h"
#include "Msg_Convert.h"
#include "em-storage.h"
#include "ipc-library.h"

EXPORT_API int email_get_rule(int filter_id, emf_rule_t** filtering_set)
{
	EM_DEBUG_FUNC_BEGIN("filter_id[%d], filtering_set[%p]", filter_id, filtering_set);

	int err = 0;

	EM_IF_NULL_RETURN_VALUE(filtering_set, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(filter_id, EMF_ERROR_INVALID_PARAM);

	if (!em_storage_get_rule_by_id(0, filter_id, (emf_mail_rule_tbl_t**)filtering_set, true, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_get_rule_by_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	} else
		err = EMF_ERROR_NONE;

FINISH_OFF:
	EM_DEBUG_FUNC_END("error value [%d]", err);
	return err;
}


EXPORT_API int email_get_rule_list(emf_rule_t** filtering_set, int* count)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int err = EMF_ERROR_NONE;
	int is_completed = 0;
	
	EM_IF_NULL_RETURN_VALUE(filtering_set, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(count, EMF_ERROR_INVALID_PARAM);

	*count = 1000;
	
	if (!em_storage_get_rule(0, 0, 0, count, &is_completed, (emf_mail_rule_tbl_t**)filtering_set, true, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_get_rule failed [%d]", err);

		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	} else
		err = EMF_ERROR_NONE;


FINISH_OFF:
	
	return err;

}

EXPORT_API int email_add_rule(emf_rule_t* filtering_set)
{
	EM_DEBUG_FUNC_BEGIN("filtering_set[%p]", filtering_set);
	
	int size = 0;
	int err = EMF_ERROR_NONE;
	char* pRuleStream = NULL;
	
	EM_IF_NULL_RETURN_VALUE(filtering_set, EMF_ERROR_INVALID_PARAM);

	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_ADD_RULE);	

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	pRuleStream = em_convert_rule_to_byte_stream(filtering_set, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(pRuleStream, hAPI, EMF_ERROR_NULL_VALUE);
	
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, pRuleStream, size)) {
		EM_DEBUG_EXCEPTION("Add Param Failed");
		EM_SAFE_FREE(pRuleStream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPI Failed");
		EM_SAFE_FREE(pRuleStream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}
		
	err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);	
	EM_SAFE_FREE(pRuleStream);
	ipcEmailAPI_Destroy(hAPI);
	hAPI = NULL;
	EM_DEBUG_FUNC_END("error value [%d]", err);
	return err;
}



EXPORT_API int email_update_rule(int filter_id, emf_rule_t* new_set)
{
	EM_DEBUG_FUNC_BEGIN("filter_id[%d], new_set[%p]", filter_id, new_set);
	
	int size = 0;
	char* pFilterStream =  NULL;
	int err = EMF_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(filter_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(new_set, EMF_ERROR_INVALID_PARAM);
	
	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_UPDATE_RULE);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* filter_id */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&filter_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("Add Param filter_id Failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	/* new_set */
	pFilterStream = em_convert_rule_to_byte_stream(new_set, &size);

	if(NULL == pFilterStream) {
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, pFilterStream, size)){
		EM_DEBUG_EXCEPTION("Add Param new_set Failed");
		EM_SAFE_FREE(pFilterStream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}
	
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPI Failed");
		EM_SAFE_FREE(pFilterStream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);
	 	
	EM_SAFE_FREE(pFilterStream);
	ipcEmailAPI_Destroy(hAPI);

	hAPI = NULL;
	EM_DEBUG_FUNC_END("error value [%d]", err);
	return err;
}




EXPORT_API int email_delete_rule(int filter_id)
{
	EM_DEBUG_FUNC_BEGIN("filter_id[%d]", filter_id);
	
	int err = EMF_ERROR_NONE;
		
	EM_IF_NULL_RETURN_VALUE(filter_id, EMF_ERROR_INVALID_PARAM);
			
	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_DELETE_RULE);
	
	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);
	
	/* filter_id */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&filter_id, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
	
		
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPI failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}
	
	err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);
	ipcEmailAPI_Destroy(hAPI);

	hAPI = NULL;
	EM_DEBUG_FUNC_END("error value [%d]", err);
	return err;
}

EXPORT_API int email_free_rule (emf_rule_t** filtering_set, int count)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE, i;	

	EM_IF_NULL_RETURN_VALUE(filtering_set, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(count, EMF_ERROR_INVALID_PARAM);
	
	if (count > 0)  {
		emf_rule_t* p = *filtering_set;
		
		for (i = 0; i < count; i++) {
			EM_SAFE_FREE(p[i].value);
			EM_SAFE_FREE(p[i].mailbox);
		}
		
		EM_SAFE_FREE(p); *filtering_set = NULL;
	}
	
	EM_DEBUG_FUNC_END("error value [%d]", err);
	return err;
}


