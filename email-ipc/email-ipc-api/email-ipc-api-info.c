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


#include <string.h>
#include <unistd.h>
#include <malloc.h>

#include "email-ipc-api-info.h"
#include "email-ipc-param-list.h"
#include "email-ipc-build.h"

#include "email-debug-log.h"


/* deserializing data from stream */
EXPORT_API bool emipc_deserialize_api_info(emipc_email_api_info *api_info, EPARAMETER_DIRECTION direction, void *stream)
{
	EM_DEBUG_FUNC_BEGIN("emipc_email_api_info : [%p], direction : [%d]", api_info, direction);
	
	if (!api_info || !stream) {
		EM_DEBUG_EXCEPTION("Invalid parameter.");
		return false;
	}

	if (api_info->params[direction] == NULL) {
		api_info->params[direction] = emipc_create_param_list();
		if (api_info->params[direction] == NULL) {
			EM_DEBUG_EXCEPTION("Malloc failed");
			return false;
		}
	}

	api_info->api_id = *((long *)stream + eSTREAM_APIID);
	api_info->app_id = *((long*)stream + eSTREAM_APPID);
	api_info->response_id = *((long*)stream + eSTREAM_RESID);

	return emipc_parse_stream_of_param_list(api_info->params[direction], stream);
}

EXPORT_API unsigned char *emipc_serialize_api_info(emipc_email_api_info *api_info, EPARAMETER_DIRECTION direction, int *stream_len)
{
	EM_DEBUG_FUNC_BEGIN();
	unsigned char *stream = NULL;
	
	if (!api_info) {
		EM_DEBUG_EXCEPTION("Invalid parameter.");
		return stream;
	}

	if (api_info->params[direction] == NULL) {
		api_info->params[direction] = emipc_create_param_list();
		if (api_info->params[direction] == NULL) {
			EM_DEBUG_EXCEPTION("Malloc failed");
			return NULL;
		}
	}

	stream = emipc_serialize_param_list(api_info->params[direction], stream_len);
	if (stream != NULL) {
		memcpy(stream, &(api_info->api_id), sizeof(api_info->api_id));
		memcpy(stream+(sizeof(long)*eSTREAM_RESID), &(api_info->response_id), sizeof(api_info->response_id));
		memcpy(stream+(sizeof(long)*eSTREAM_APPID), &(api_info->app_id), sizeof(api_info->app_id));
	}
	EM_DEBUG_FUNC_END();
	return stream;
}

EXPORT_API void *emipc_get_parameters_of_api_info(emipc_email_api_info *api_info, EPARAMETER_DIRECTION direction)
{
	EM_DEBUG_FUNC_BEGIN("emipc_email_api_info : [%p], direction : [%d]", api_info, direction);
	
	if (!api_info) {
		EM_DEBUG_EXCEPTION("INVALID_PARAM");
		return NULL;
	}
	
	if (api_info->params[direction] == NULL) {
		api_info->params[direction] = emipc_create_param_list();
		if (api_info->params[direction] == NULL) {
			EM_DEBUG_EXCEPTION("emipc_create_param_list failed");
			return NULL;
		}
	}

	return api_info->params[direction];
}

EXPORT_API void emipc_free_api_info(emipc_email_api_info *api_info)
{
	if (!api_info) 
		return;

	emipc_destroy_param_list (api_info->params[ePARAMETER_IN]);
	emipc_destroy_param_list (api_info->params[ePARAMETER_OUT]);
}


