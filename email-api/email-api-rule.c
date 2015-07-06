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
 * @file		email-api-rule.c
 * @brief 		This file contains the data structures and interfaces of Rule related Functionality provided by 
 *			email-service . 
 */

#include "string.h"
#include "email-convert.h"
#include "email-storage.h"
#include "email-utilities.h"
#include "email-ipc.h"
#include "email-core-utils.h"

EXPORT_API int email_get_rule(int filter_id, email_rule_t** filtering_set)
{
	EM_DEBUG_API_BEGIN ("filter_id[%d] filtering_set[%p]", filter_id, filtering_set);

	int err = 0;
    char *multi_user_name = NULL;

	EM_IF_NULL_RETURN_VALUE(filtering_set, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(filter_id, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (!emstorage_get_rule_by_id(multi_user_name, filter_id, (emstorage_rule_tbl_t**)filtering_set, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_rule_by_id failed [%d]", err);
		goto FINISH_OFF;
	} else
		err = EMAIL_ERROR_NONE;

FINISH_OFF:

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}


EXPORT_API int email_get_rule_list(email_rule_t** filtering_set, int* count)
{
	EM_DEBUG_API_BEGIN ();
	
	int err = EMAIL_ERROR_NONE;
	int is_completed = 0;
    char *multi_user_name = NULL;
	
	EM_IF_NULL_RETURN_VALUE(filtering_set, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(count, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	*count = 1000;

	if (!emstorage_get_rule(multi_user_name, 0, 0, 0, count, &is_completed, (emstorage_rule_tbl_t**)filtering_set, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_rule failed [%d]", err);
		goto FINISH_OFF;
	} else
		err = EMAIL_ERROR_NONE;

FINISH_OFF:

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;

}

EXPORT_API int email_add_rule(email_rule_t* filtering_set)
{
	EM_DEBUG_API_BEGIN ("filtering_set[%p]", filtering_set);
	
	int size = 0;
	int *ret_nth_param = NULL;
	int err = EMAIL_ERROR_NONE;
	char* stream = NULL;
	
	EM_IF_NULL_RETURN_VALUE(filtering_set, EMAIL_ERROR_INVALID_PARAM);

	if (filtering_set->account_id < 0 || filtering_set->target_mailbox_id < 0) {
		EM_DEBUG_EXCEPTION("Invalid Param : account_id[%d], target_mailbox_id[%d]",
				filtering_set->account_id, filtering_set->target_mailbox_id);
		return EMAIL_ERROR_INVALID_PARAM;
	}

	/* make rule info */
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_ADD_RULE);	
	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	stream = em_convert_rule_to_byte_stream(filtering_set, &size);
	EM_PROXY_IF_NULL_RETURN_VALUE(stream, hAPI, EMAIL_ERROR_NULL_VALUE);
	if(!emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, stream, size)) {
		EM_DEBUG_EXCEPTION("Add Param Failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	/* pass rule info */
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api Failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}
	
	/* get reult form service */
	if ((ret_nth_param = (int*)emipc_get_nth_parameter_data(hAPI, ePARAMETER_OUT, 0))) {
		err = *ret_nth_param;

		if (err == EMAIL_ERROR_NONE) {
			if ((ret_nth_param = (int *)emipc_get_nth_parameter_data(hAPI, ePARAMETER_OUT, 1)))
				filtering_set->filter_id = *ret_nth_param;
			else
				err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
		}
	} else {
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
	}

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_update_rule(int filter_id, email_rule_t* new_set)
{
	EM_DEBUG_API_BEGIN ("filter_id[%d] new_set[%p]", filter_id, new_set);
	
	int size = 0;
	char* stream =  NULL;
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(filter_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(new_set, EMAIL_ERROR_INVALID_PARAM);
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_UPDATE_RULE);
	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	/* make filter info */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&filter_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("Add Param filter_id Failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	stream = em_convert_rule_to_byte_stream(new_set, &size);
	if(NULL == stream) {
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}
	if(!emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, stream, size)){
		EM_DEBUG_EXCEPTION("Add Param new_set Failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}
	
	/* request update rule with filter info */
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api Failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	/* get result */
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	 	
	emipc_destroy_email_api(hAPI);

	hAPI = NULL;
	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}




EXPORT_API int email_delete_rule(int filter_id)
{
	EM_DEBUG_API_BEGIN ("filter_id[%d]", filter_id);
	
	int err = EMAIL_ERROR_NONE;
		
	EM_IF_NULL_RETURN_VALUE(filter_id, EMAIL_ERROR_INVALID_PARAM);
			
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_DELETE_RULE);
	
	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);
	
	/* filter_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&filter_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}
		
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	emipc_destroy_email_api(hAPI);

	hAPI = NULL;
	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_free_rule (email_rule_t** filtering_set, int count)
{
	EM_DEBUG_FUNC_BEGIN ();
	int err = EMAIL_ERROR_NONE, i;	

	EM_IF_NULL_RETURN_VALUE(filtering_set, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(count, EMAIL_ERROR_INVALID_PARAM);
	
	if (count > 0)  {
		email_rule_t* p = *filtering_set;
		
		for (i = 0; i < count; i++)
			emcore_free_rule(p + i);
		
		EM_SAFE_FREE(p);
		*filtering_set = NULL;
	}
	
	EM_DEBUG_FUNC_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_apply_rule(int filter_id)
{
	EM_DEBUG_API_BEGIN ("filter_id[%d]", filter_id);
	
	int err = EMAIL_ERROR_NONE;
		
	EM_IF_NULL_RETURN_VALUE(filter_id, EMAIL_ERROR_INVALID_PARAM);
			
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_APPLY_RULE);
	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);
	
	/* filter_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&filter_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}
		
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	emipc_destroy_email_api(hAPI);

	hAPI = NULL;
	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}
