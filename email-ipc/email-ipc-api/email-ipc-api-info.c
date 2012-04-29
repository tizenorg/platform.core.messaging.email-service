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


#include <string.h>
#include <unistd.h>
#include <malloc.h>

#include "email-ipc-api-info.h"
#include "email-ipc-param-list.h"
#include "email-ipc-build.h"

#include "email-debug-log.h"

static long emipc_parse_api_id_of_api_info(emipc_email_api_info *api_info, void *stream);
static long emipc_parse_response_id_of_api_info(emipc_email_api_info *api_info, void* stream);
static long emipc_parse_app_id_of_api_info(emipc_email_api_info *api_info, void* stream);

EXPORT_API bool emipc_set_api_id_of_api_info(emipc_email_api_info *api_info, long api_id)
{
	api_info->api_id = api_id;
	return true;
}

EXPORT_API long emipc_get_api_id_of_api_info(emipc_email_api_info *api_info)
{
	return api_info->api_id;
}

EXPORT_API bool emipc_set_app_id_of_api_info(emipc_email_api_info *api_info, long app_id)
{
	api_info->app_id = app_id;
	return true;
}

EXPORT_API long emipc_get_app_id_of_api_info(emipc_email_api_info *api_info)
{
	return api_info->app_id;
}

EXPORT_API bool emipc_set_response_id_of_api_info(emipc_email_api_info *api_info, long response_id)
{
	api_info->response_id = response_id;
	return true;
}

EXPORT_API long emipc_get_response_id_of_api_info(emipc_email_api_info *api_info)
{
	return api_info->response_id;
}

EXPORT_API bool emipc_parse_stream_of_api_info(emipc_email_api_info *api_info, EPARAMETER_DIRECTION direction, void *stream)
{
	emipc_param_list *new_param_list = NULL;

	if (api_info->params[direction] == NULL) {
		new_param_list = (emipc_param_list *)malloc(sizeof(emipc_param_list));
		if (new_param_list == NULL) {
			EM_DEBUG_EXCEPTION("Memory allocation failed.");
			return false;
		}
		memset(new_param_list, 0x00, sizeof(emipc_param_list));
		api_info->params[direction] = new_param_list;
	}

	emipc_parse_api_id_of_api_info(api_info, stream);
	emipc_parse_app_id_of_api_info(api_info, stream);
	emipc_parse_response_id_of_api_info(api_info, stream);
	return emipc_parse_stream_of_param_list(api_info->params[direction], stream);
}

EXPORT_API unsigned char *emipc_get_stream_of_api_info(emipc_email_api_info *api_info, EPARAMETER_DIRECTION direction)
{
	emipc_param_list *new_param_list = NULL;
	unsigned char *stream = NULL;

	if (api_info->params[direction] == NULL) {
		new_param_list = (emipc_param_list *)malloc(sizeof(emipc_param_list));
		if (new_param_list == NULL) {
			EM_DEBUG_EXCEPTION("Memory allocation failed.");
			return false;
		}
		memset(new_param_list, 0x00, sizeof(emipc_param_list));
		api_info->params[direction] = new_param_list;
	}

	stream = emipc_get_stream_of_param_list(api_info->params[direction]);
	if (stream != NULL) {
		memcpy(stream, &(api_info->api_id), sizeof(api_info->api_id));
		memcpy(stream+(sizeof(long)*eSTREAM_RESID), &(api_info->response_id), sizeof(api_info->response_id));
		memcpy(stream+(sizeof(long)*eSTREAM_APPID), &(api_info->app_id), sizeof(api_info->app_id));
	}
	return stream;
}

EXPORT_API int emipc_get_stream_length_of_api_info(emipc_email_api_info *api_info, EPARAMETER_DIRECTION direction)
{
	if (api_info->params[direction] == NULL)
		return 0;

	return emipc_get_stream_length_of_param_list(api_info->params[direction]);
}

EXPORT_API void *emipc_get_parameters_of_api_info(emipc_email_api_info *api_info, EPARAMETER_DIRECTION direction)
{
	emipc_param_list *new_param_list = NULL;

	if (api_info->params[direction] == NULL) {
		new_param_list = (emipc_param_list *)malloc(sizeof(emipc_param_list));
		if (new_param_list == NULL) {
			EM_DEBUG_EXCEPTION("Memory allocation failed.");
			return false;
		}
		memset(new_param_list, 0x00, sizeof(emipc_param_list));
		api_info->params[direction] = new_param_list;
	}
	return api_info->params[direction];
}

static long emipc_parse_api_id_of_api_info(emipc_email_api_info *api_info, void *stream)
{
	api_info->api_id = *((long *)stream + eSTREAM_APIID);
	return api_info->api_id;
}

static long emipc_parse_response_id_of_api_info(emipc_email_api_info *api_info, void* stream)
{
	api_info->response_id = *((long*)stream + eSTREAM_RESID);
	return api_info->response_id;
}

static long emipc_parse_app_id_of_api_info(emipc_email_api_info *api_info, void* stream)
{
	api_info->app_id = *((long*)stream + eSTREAM_APPID);
	return api_info->app_id;

}
