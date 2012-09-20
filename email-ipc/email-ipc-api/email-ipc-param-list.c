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
#include "email-utilities.h"

/*  stream */
/*  +----------------------------------------------------------------------------------------------------------+ */
/*  | API ID(4B) | Resp. ID (4B) | Param Count(4B) | Param1 Len | Param1 Data | ... | ParamN Len | ParamN data |  */
/*  +----------------------------------------------------------------------------------------------------------+ */


EXPORT_API emipc_param_list *emipc_create_param_list()
{
	emipc_param_list *new_param_list = NULL;

	new_param_list = (emipc_param_list *) em_malloc (sizeof(emipc_param_list));
	if (new_param_list == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed.");
		return NULL;
	}
	memset(new_param_list, 0x00, sizeof(emipc_param_list));

	return new_param_list;
}

EXPORT_API bool emipc_destroy_param_list(emipc_param_list *param_list)
{
	int count = 10;
	int index = 0;

	if (!param_list) {
		EM_DEBUG_EXCEPTION("Invalid parameter.");
		return false;
	}

	for (index = 0; index < count; index++) {
		emipc_free_param(param_list->params[index]);
	}
	EM_SAFE_FREE(param_list->byte_stream);
	EM_SAFE_FREE(param_list);
	return true;
}

/* making stream into param length and param data */
EXPORT_API bool emipc_parse_stream_of_param_list(emipc_param_list *param_list, void *stream)
{
	EM_DEBUG_FUNC_BEGIN();
	long parameter_count = *((long *)stream + eSTREAM_COUNT);
	if(parameter_count <= 0) {
		EM_DEBUG_EXCEPTION("INVALID_PARAM : count %d", parameter_count);
		return false;
	}

	unsigned char* cur = ((unsigned char*)stream) + sizeof(int)*eSTREAM_DATA;

	int i = 0;
	/* stream is composed of data type which is encoded into length and data field */
	int len = 0;
	for(i = 0; i < parameter_count; i++) {
		/* reading length */
		memcpy(&len, cur, sizeof(int));

		/* moving from length field to data field */
		cur += sizeof(int);
		emipc_add_param_to_param_list(param_list, (void*)cur, len);

		EM_DEBUG_LOG("Parsing stream : element %d is %dbyte long ", i, len);

		/*  move to next parameter	 */
		cur += len;
	}

	EM_DEBUG_FUNC_END();
	return true;
}

EXPORT_API unsigned char *emipc_serialize_param_list(emipc_param_list *param_list, int *stream_length)
{
	EM_DEBUG_FUNC_BEGIN();
	if(param_list->byte_stream)
		return param_list->byte_stream;

	int stream_len = emipc_sum_param_list_length (param_list);

	if (stream_len <= 0) {
		EM_DEBUG_EXCEPTION("stream_len error %d", stream_len);
		EM_SAFE_FREE(param_list->byte_stream);
		return NULL;
	}

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

	/* stream format */
	/* | param1 length | (param1 data) | param2 length | (param2 data) | ... |*/
	/* if param is 0 long, the param data is omitted */
	for(index=0; index<param_list->param_count; index++) {
		length = emipc_get_length(param_list->params[index]);
		if (length < 0) {
		 	EM_DEBUG_EXCEPTION("index = %d, length = %d", index, length);
			EM_SAFE_FREE(param_list->byte_stream);
			return NULL;
		}

		if (pos + (int)sizeof(long) > stream_len) {
		 	EM_DEBUG_EXCEPTION("%d > stream_len", pos + sizeof(long));
			EM_SAFE_FREE(param_list->byte_stream);
			return NULL;
		}
		/* write param i length */
		memcpy((param_list->byte_stream+pos), &length, sizeof(long));
		pos += sizeof(long);

		if (pos + length > stream_len) {
			EM_DEBUG_EXCEPTION("%d > stream_len", pos + length);
			EM_SAFE_FREE(param_list->byte_stream);
			return NULL;
		}
		/* write param i data if length is greater than 0 */
		if( length > 0 ) {
			memcpy((param_list->byte_stream+pos), emipc_get_data(param_list->params[index]), length);
			pos += length;
		}
	}
	*stream_length = stream_len;

	EM_DEBUG_FUNC_END();
	return param_list->byte_stream;
}

EXPORT_API int emipc_sum_param_list_length(emipc_param_list *param_list)
{
	int length = sizeof(long) * eSTREAM_DATA;
	int index;
	for (index = 0; index < param_list->param_count; index++) {
		length += sizeof(long);
		length += emipc_get_length(param_list->params[index]);
	}
	return length;
}

EXPORT_API bool emipc_add_param_to_param_list(emipc_param_list *param_list, void *data, int len)
{
	EM_DEBUG_FUNC_BEGIN();
	if (emipc_set_param(&(param_list->params[param_list->param_count]), data, len)) {
		param_list->param_count++;
		EM_SAFE_FREE(param_list->byte_stream);
		return true;
	}
	return false;
}

EXPORT_API void emipc_add_dynamic_param_to_param_list(emipc_param_list *param_list, void *data, int len)
{
	EM_DEBUG_FUNC_BEGIN();
	emipc_set_dynamic_param(&(param_list->params[param_list->param_count]), data, len);
	param_list->param_count++;
	EM_SAFE_FREE(param_list->byte_stream);
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


