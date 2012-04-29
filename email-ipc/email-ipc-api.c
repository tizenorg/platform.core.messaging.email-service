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

	emipc_email_api_info *api_info = NULL;

	api_info = (emipc_email_api_info *)malloc(sizeof(emipc_email_api_info));	
	if(api_info == NULL) {
		EM_DEBUG_EXCEPTION("Malloc failed");
		return NULL;
	}
	memset(api_info, 0x00, sizeof(emipc_email_api_info));

	emipc_set_api_id_of_api_info(api_info, api_id);

	return (HIPC_API)api_info;
}

EXPORT_API void emipc_destroy_email_api(HIPC_API api)
{
	EM_DEBUG_FUNC_BEGIN("API = %p", api);
	emipc_email_api_info *api_info = (emipc_email_api_info *)api;
	EM_SAFE_FREE(api_info);
}

EXPORT_API long emipc_get_api_id(HIPC_API api)
{
	EM_DEBUG_FUNC_BEGIN();
	emipc_email_api_info *api_info = (emipc_email_api_info*)api;
	return emipc_get_api_id_of_api_info(api_info);
}

EXPORT_API long emipc_get_app_id(HIPC_API api)
{
	EM_DEBUG_FUNC_BEGIN();
	emipc_email_api_info *api_info = (emipc_email_api_info *)api;
	return emipc_get_app_id_of_api_info(api_info);
}

EXPORT_API bool emipc_add_parameter(HIPC_API api, EPARAMETER_DIRECTION direction, void *data, int data_length)
{
	EM_DEBUG_FUNC_BEGIN();

	emipc_param_list *parameters = emipc_get_api_parameters(api, direction);
	if (parameters)
		return emipc_add_param_of_param_list(parameters, data, data_length);
	else
		return false;
}

EXPORT_API int emipc_get_parameter_count(HIPC_API api, EPARAMETER_DIRECTION direction)
{
	EM_DEBUG_FUNC_BEGIN();
	
	emipc_param_list *parameters = emipc_get_api_parameters(api, direction);

	if(parameters) 
		return emipc_get_param_count_of_param_list(parameters);
	else
		return -1;
}

EXPORT_API int emipc_get_parameter(HIPC_API input_api_handle, EPARAMETER_DIRECTION input_parameter_direction, int input_parameter_index, int input_parameter_buffer_size, void *output_parameter_buffer)
{
	EM_DEBUG_FUNC_BEGIN("input_api_handle [%d], input_parameter_direction [%d], input_parameter_index [%d], input_parameter_buffer_size [%d], output_parameter_buffer [%p]", input_api_handle , input_parameter_direction , input_parameter_index, input_parameter_buffer_size, output_parameter_buffer);
	emipc_param_list *parameters = NULL;
	void *local_buffer = NULL;

	if (input_parameter_buffer_size == 0 || output_parameter_buffer == NULL) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return EMF_ERROR_INVALID_PARAM;
	}

	parameters = emipc_get_api_parameters(input_api_handle, input_parameter_direction);

	if (parameters == NULL) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_IPC_PROTOCOL_FAILURE");
		return EMF_ERROR_IPC_PROTOCOL_FAILURE;
	}

	local_buffer = emipc_get_param_of_param_list(parameters, input_parameter_index);

	if (local_buffer == NULL) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_IPC_PROTOCOL_FAILURE");
		return EMF_ERROR_IPC_PROTOCOL_FAILURE;
	}

	if (emipc_get_param_len_of_param_list(parameters, input_parameter_index) != input_parameter_buffer_size) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_IPC_PROTOCOL_FAILURE");
		return EMF_ERROR_IPC_PROTOCOL_FAILURE;
	}

	memcpy(output_parameter_buffer, local_buffer, input_parameter_buffer_size);

	EM_DEBUG_FUNC_END("Suceeded");
	return EMF_ERROR_NONE;
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





