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

#include <malloc.h>

#include "email-ipc.h"
#include "email-ipc-build.h"
#include "email-ipc-api-info.h"
#include "email-ipc-param-list.h"
#include "email-ipc-socket.h"
#include "email-proxy-main.h"

#include "email-debug-log.h"
#include "email-errors.h"

EXPORT_API emipc_param_list *emipc_get_api_parameters(HIPC_API api, EPARAMETER_DIRECTION direction)
{
	EM_DEBUG_FUNC_BEGIN();
	emipc_email_api_info *api_info = (emipc_email_api_info *)api;

	return (emipc_param_list *)emipc_get_parameters_of_api_info(api_info, direction);
}

EXPORT_API HIPC_API emipc_create_email_api(long api_id)
{
	EM_DEBUG_FUNC_BEGIN();

	emipc_email_api_info *api_info = (emipc_email_api_info *)calloc(1, sizeof(emipc_email_api_info));
	if(api_info == NULL) {
		EM_DEBUG_EXCEPTION("Malloc failed");
		return NULL;
	}

	api_info->api_id = api_id;

	return (HIPC_API)api_info;
}

EXPORT_API void emipc_destroy_email_api(HIPC_API api)
{
	EM_DEBUG_FUNC_BEGIN("API = %p", api);
	emipc_email_api_info *api_info = (emipc_email_api_info *)api;
	emipc_free_api_info(api_info);
	EM_SAFE_FREE(api_info);
}

EXPORT_API long emipc_get_api_id(HIPC_API api)
{
	EM_DEBUG_FUNC_BEGIN();
	emipc_email_api_info *api_info = (emipc_email_api_info*)api;
	return api_info->api_id;
}

EXPORT_API long emipc_get_app_id(HIPC_API api)
{
	EM_DEBUG_FUNC_BEGIN();
	emipc_email_api_info *api_info = (emipc_email_api_info *)api;
	return api_info->app_id;
}

/* note: there incurs additional cost (malloc & memcpy). */
/* if data is a dynamic variable, please use emipc_dynamic_parameter instead */
EXPORT_API bool emipc_add_parameter(HIPC_API api, EPARAMETER_DIRECTION direction, void *data, int data_length)
{
	EM_DEBUG_FUNC_BEGIN("data_length[%d]", data_length);

	emipc_param_list *parameters = emipc_get_api_parameters(api, direction);
	if (!parameters) {
		EM_DEBUG_EXCEPTION("emipc_get_api_parameters failed");
		return false;
	}

	return emipc_add_param_to_param_list(parameters, data, data_length);
}

/* caution : data should be a dynamic variable */
/*           please, do not use a static variable */
EXPORT_API bool emipc_add_dynamic_parameter(HIPC_API api, EPARAMETER_DIRECTION direction, void *data, int data_length)
{
	EM_DEBUG_FUNC_BEGIN("data_length[%d]", data_length);
	
	emipc_param_list *parameters = emipc_get_api_parameters(api, direction);
	if (!parameters) {
		EM_DEBUG_EXCEPTION("emipc_get_api_parameters failed");
		return false;
	}

	emipc_add_dynamic_param_to_param_list(parameters, data, data_length);
	return true;
}



EXPORT_API int emipc_get_parameter(HIPC_API input_api_handle, EPARAMETER_DIRECTION input_parameter_direction,
			int input_parameter_index, int input_parameter_buffer_size, void *output_parameter_buffer)
{
	EM_DEBUG_FUNC_BEGIN("parameter_index [%d], parameter_buffer_size [%d]", input_parameter_index, input_parameter_buffer_size);
	emipc_param_list *parameters = NULL;
	void *local_buffer = NULL;

	if (input_parameter_buffer_size == 0 || output_parameter_buffer == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	parameters = emipc_get_api_parameters(input_api_handle, input_parameter_direction);

	if (parameters == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_IPC_PROTOCOL_FAILURE");
		return EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
	}

	local_buffer = emipc_get_param_of_param_list(parameters, input_parameter_index);

	if (local_buffer == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_IPC_PROTOCOL_FAILURE");
		return EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
	}

	if (emipc_get_param_len_of_param_list(parameters, input_parameter_index) != input_parameter_buffer_size) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_IPC_PROTOCOL_FAILURE");
		return EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
	}

	memcpy(output_parameter_buffer, local_buffer, input_parameter_buffer_size);

	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}


EXPORT_API void* emipc_get_nth_parameter_data(HIPC_API api_handle, EPARAMETER_DIRECTION direction, int param_index)
{
	EM_DEBUG_FUNC_BEGIN("nth_parameter_index [%d]", param_index);
	emipc_param_list *parameters = NULL;
	void *buf = NULL;

	parameters = emipc_get_api_parameters(api_handle, direction);

	if (parameters == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_IPC_PROTOCOL_FAILURE");
		return NULL;
	}

	buf = emipc_get_param_of_param_list(parameters, param_index);

	if (!buf) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_IPC_PROTOCOL_FAILURE");
		return NULL;
	}

	EM_DEBUG_FUNC_END();
	return buf;
}


EXPORT_API int emipc_get_parameter_length(HIPC_API api, EPARAMETER_DIRECTION direction, int parameter_index)
{
	EM_DEBUG_FUNC_BEGIN();
	emipc_param_list *parameters = emipc_get_api_parameters(api, direction);
	if (parameters) {
		EM_DEBUG_FUNC_END("Suceeded");
		return emipc_get_param_len_of_param_list(parameters, parameter_index);
	}
	EM_DEBUG_FUNC_END("Failed");
	return -1;
}

EXPORT_API int emipc_get_nth_parameter_length(HIPC_API api, EPARAMETER_DIRECTION direction, int parameter_index)
{
	EM_DEBUG_FUNC_BEGIN();
	emipc_param_list *parameters = emipc_get_api_parameters(api, direction);
	if (parameters) {
		EM_DEBUG_FUNC_END("Suceeded");
		return emipc_get_param_len_of_param_list(parameters, parameter_index);
	}
	EM_DEBUG_FUNC_END("Failed");
	return -1;
}



