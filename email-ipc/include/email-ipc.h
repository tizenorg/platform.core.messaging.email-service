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


#ifndef __IPC_LIBRARY_H
#define __IPC_LIBRARY_H

#include "email-types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EM_PROXY_IF_NULL_RETURN_VALUE(expr1, expr2, val) {	\
	if (!expr1&& expr2) {	\
		EM_DEBUG_LOG ("EM_PROXY_IF_NULL_RETURN_VALUE : PARAM IS NULL \n");	\
		emipc_destroy_email_api(expr2);	\
		return val;	\
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
typedef void (*PFN_PROXY_CALLBACK)	(HIPC_API input_api_handle, void* pParam1, void* pParam2);
typedef void (*PFN_EXECUTE_API)		(HIPC_API input_api_handle);

/*  ------------------------------------------------------------------------------------------------------------ */
/* 	Proxy API */
/*  ------------------------------------------------------------------------------------------------------------ */
EXPORT_API int emipc_initialize_proxy();
EXPORT_API int emipc_finalize_proxy();
EXPORT_API int emipc_execute_proxy_api(HIPC_API input_api_handle);

/*  ------------------------------------------------------------------------------------------------------------ */
/* 	Stub API */
/*  ------------------------------------------------------------------------------------------------------------ */
EXPORT_API bool emipc_initialize_stub(PFN_EXECUTE_API input_api_mapper);
EXPORT_API bool emipc_finalize_stub();
EXPORT_API bool emipc_execute_stub_api(HIPC_API input_api_handle);

/*  ------------------------------------------------------------------------------------------------------------ */
/* 	API */
/*  ------------------------------------------------------------------------------------------------------------ */
EXPORT_API HIPC_API emipc_create_email_api(long api_id);
EXPORT_API void emipc_destroy_email_api(HIPC_API input_api_handle);

EXPORT_API long emipc_get_api_id(HIPC_API input_api_handle);
EXPORT_API long emipc_get_app_id(HIPC_API input_api_handle);
EXPORT_API long emipc_get_response_id(HIPC_API input_api_handle);

EXPORT_API long emipc_get_permission(HIPC_API input_api_handle);

EXPORT_API bool emipc_add_parameter(HIPC_API api, EPARAMETER_DIRECTION direction, void *data, int data_length);
EXPORT_API bool emipc_add_dynamic_parameter(HIPC_API api, EPARAMETER_DIRECTION direction, void *data, int data_length);
EXPORT_API int emipc_get_parameter(HIPC_API input_api_handle, EPARAMETER_DIRECTION input_parameter_direction, int input_parameter_index, int input_parameter_buffer_size, void *output_parameter);
EXPORT_API void* emipc_get_nth_parameter_data(HIPC_API api_handle, EPARAMETER_DIRECTION direction, int param_index);
EXPORT_API int emipc_get_parameter_length(HIPC_API input_api_handle, EPARAMETER_DIRECTION input_parameter_direction, int input_parameter_index);
EXPORT_API int emipc_get_nth_parameter_length(HIPC_API input_api_handle, EPARAMETER_DIRECTION input_parameter_direction, int input_parameter_index);

EXPORT_API int emipc_execute_proxy_task(email_task_type_t input_task_type, void *input_task_parameter);
#ifdef __cplusplus
}
#endif
#endif


