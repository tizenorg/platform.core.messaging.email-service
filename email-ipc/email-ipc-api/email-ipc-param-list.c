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
#include <stdlib.h>

#include "email-ipc-build.h"
#include "email-ipc-param-list.h"

#include "email-debug-log.h"

/*  stream */
/*  +----------------------------------------------------------------------------------------------------------+ */
/*  | API ID(4B) | Resp. ID (4B) | Param Count(4B) | Param1 Len | Param1 Data | ... | ParamN Len | ParamN data |  */
/*  +----------------------------------------------------------------------------------------------------------+ */

static long emipc_parse_parameter_count(void *stream);

EXPORT_API bool emipc_parse_stream_of_param_list(emipc_param_list *param_list, void *stream_data)
{
	EM_DEBUG_FUNC_BEGIN();
	long parameter_count = emipc_parse_parameter_count(stream_data);
	if(parameter_count <= 0) {
		EM_DEBUG_EXCEPTION("There is no parameter. ");
		return false;
	}

	unsigned char* stream = (unsigned char*)stream_data;
	long index, param_len, pos = sizeof(long)*eSTREAM_DATA;

	for(index = 0; index < parameter_count; index++) {
		long len =0;
		memcpy(&len, stream+pos, sizeof(long));
		param_len = len;
		EM_DEBUG_LOG("Parameter Length [%d] : %d ", index, param_len);
		pos += sizeof(long);	/*  Move from length position to data position */

		emipc_add_param_of_param_list(param_list, (void*)(stream+pos), param_len);
		pos += param_len;		/*  move to next parameter	 */
	}
	
	return true;
}

EXPORT_API unsigned char *emipc_get_stream_of_param_list(emipc_param_list *param_list)
{
	EM_DEBUG_FUNC_BEGIN();
	if(param_list->byte_stream)
		return param_list->byte_stream;
	
	int stream_len = emipc_get_stream_length_of_param_list(param_list);
	
	if (stream_len > 0) {
		param_list->byte_stream = (unsigned char*)calloc(1, stream_len);
		int pos = sizeof(long)*eSTREAM_COUNT;

		if (pos + (int)sizeof(param_list->param_count) > stream_len ) {
			EM_DEBUG_EXCEPTION("%d > stream_len", pos + sizeof(param_list->param_count));			
			EM_SAFE_FREE(param_list->byte_stream);
			return NULL;
		}
		
		memcpy((param_list->byte_stream + pos), &param_list->param_count, sizeof(param_list->param_count));
		
		pos += sizeof(long);
		int index = 0, length = 0;

		/*   check memory overflow */
		for(index=0; index<param_list->param_count; index++) {
			length = emipc_get_length(param_list->params[index]);
			if (length <= 0) {
			 	EM_DEBUG_EXCEPTION("index = %d, length = %d", index, length);			
				EM_SAFE_FREE(param_list->byte_stream);
				return NULL;
			}

			if (pos + (int)sizeof(long) > stream_len) {
			 	EM_DEBUG_EXCEPTION("%d > stream_len", pos + sizeof(long));			
				EM_SAFE_FREE(param_list->byte_stream);
				return NULL;
			}
			memcpy((param_list->byte_stream+pos), &length, sizeof(long));
			pos += sizeof(long);

			if (pos + length > stream_len) {
				EM_DEBUG_EXCEPTION("%d > stream_len", pos + length);			
				EM_SAFE_FREE(param_list->byte_stream);
				return NULL;
			}
			
			memcpy((param_list->byte_stream+pos), emipc_get_data(param_list->params[index]), length);
			pos += length;
		}
		return param_list->byte_stream;
	}
	
	EM_DEBUG_EXCEPTION("failed.");			
	EM_SAFE_FREE(param_list->byte_stream);

	EM_DEBUG_FUNC_END();
	return NULL;
}

EXPORT_API int emipc_get_stream_length_of_param_list(emipc_param_list *param_list)
{
	int length = sizeof(long) * eSTREAM_DATA;
	int index;
	for (index = 0; index < param_list->param_count; index++) {
		length += sizeof(long);
		length += emipc_get_length(param_list->params[index]);
	}
	return length;
}

EXPORT_API bool emipc_add_param_of_param_list(emipc_param_list *param_list, void *data, int len)
{
	EM_DEBUG_FUNC_BEGIN();
	if (emipc_set_param(&(param_list->params[param_list->param_count]), data, len)) {
		param_list->param_count++;
		EM_SAFE_FREE(param_list->byte_stream);
		return true;
	}
	return false;
}

EXPORT_API void *emipc_get_param_of_param_list(emipc_param_list *param_list, int index)
{
	EM_DEBUG_FUNC_BEGIN("index [%d]", index);
	if (index < 0 || index >= param_list->param_count) {
		EM_DEBUG_EXCEPTION("Index value is not valid");
		return NULL;
	}
	return emipc_get_data(param_list->params[index]);
}

EXPORT_API int emipc_get_param_len_of_param_list(emipc_param_list *param_list, int index)
{
	EM_DEBUG_FUNC_BEGIN();
	if (index < 0 || index >= param_list->param_count) {
		EM_DEBUG_EXCEPTION("Index valud is not valid");
		return 0;
	}
	return emipc_get_length(param_list->params[index]);
}

EXPORT_API int emipc_get_param_count_of_param_list(emipc_param_list *param_list)
{
	EM_DEBUG_FUNC_BEGIN("Parameter count [%d]", param_list->param_count);
	return param_list->param_count;
}

static long emipc_parse_parameter_count(void *stream)
{
	EM_DEBUG_FUNC_BEGIN();	
	long *parameter_count_position = (long *)stream + eSTREAM_COUNT;
	
	return *parameter_count_position;
}
