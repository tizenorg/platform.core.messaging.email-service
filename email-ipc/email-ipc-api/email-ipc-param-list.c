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
#include <stdlib.h>
#include <malloc.h>

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

	new_param_list = (emipc_param_list *)em_malloc(sizeof(emipc_param_list));
	if (new_param_list == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed.");
		return NULL;
	}
	memset(new_param_list, 0x00, sizeof(emipc_param_list));

	return new_param_list;
}

EXPORT_API void emipc_destroy_param_list(emipc_param_list *param_list)
{
	int count = 10;
	int index = 0;

	if (!param_list)
		return;

	for (index = 0; index < count; index++) {
		emipc_free_param(param_list->params[index]);
	}
	EM_SAFE_FREE(param_list->byte_stream);
	EM_SAFE_FREE(param_list);
}

/* making stream into param length and param data */
EXPORT_API bool emipc_parse_stream_of_param_list(void *stream, emipc_param_list *param_list)
{
	EM_DEBUG_FUNC_BEGIN();
	int parameter_count = 0;

	/* Get the parameter count */
	memcpy(&parameter_count, stream + (sizeof(int) * eSTREAM_COUNT), sizeof(int));
	if (parameter_count < 0) {
		EM_DEBUG_LOG("INVALID_PARAM : count %d", parameter_count);
		return false;
	}

	if (parameter_count == 0) {
		EM_DEBUG_LOG("count %d", parameter_count);
		return true;
	}

	int stream_len = malloc_usable_size(stream);
	int remain_len = stream_len - (sizeof(int) * eSTREAM_DATA);
	EM_DEBUG_LOG_DEV("Allocated stream size : %dbyte", stream_len);

	unsigned char* cur = ((unsigned char *)stream) + (sizeof(int) * eSTREAM_DATA);

	int i = 0;
	/* stream is composed of data type which is encoded into length and data field */
	int len = 0;

	for (i = 0; i < parameter_count; i++) {
		if (remain_len < sizeof(int)) {
			EM_DEBUG_EXCEPTION("Not enough remain stream_len[%d]", remain_len);
			return false;
		}

		/* reading length */
		memcpy(&len, cur, sizeof(int));

		/* moving from length field to data field */
		cur += sizeof(int);
		remain_len -= sizeof(int);

		if (remain_len > 0 && len > 0 && remain_len >= len)
			emipc_add_param_to_param_list(param_list, (void *)cur, len);
		else {
			EM_DEBUG_EXCEPTION("data_len[%d] is not in the boundary of remain stream_len", len);
			return false;
		}

		EM_DEBUG_LOG("Parsing stream : element %d is %dbyte integer", i, len);

		/*  move to next parameter	 */
		cur += len;
		remain_len -= len;
	}

	EM_DEBUG_FUNC_END();
	return true;
}

EXPORT_API unsigned char *emipc_serialize_param_list(emipc_param_list *param_list, int *stream_length)
{
	EM_DEBUG_FUNC_BEGIN("param_list [%p] stream_length [%p]", param_list, stream_length);

	if (!param_list) {
		EM_DEBUG_LOG ("no data to be serialized");
		return NULL;
	}

	EM_SAFE_FREE (param_list->byte_stream);

	int stream_len = emipc_sum_param_list_length(param_list);
	if (stream_len <= 0) {
		EM_DEBUG_EXCEPTION("stream_len error %d", stream_len);
		goto FINISH_OFF;
	}

	param_list->byte_stream = (unsigned char*)calloc(1, stream_len);
	if (param_list->byte_stream == NULL) {
		EM_DEBUG_EXCEPTION("calloc failed : [%d]", errno);
		goto FINISH_OFF;
	}

	int pos = sizeof(int) * eSTREAM_COUNT;
	if (pos + (int)sizeof(param_list->param_count) > stream_len ) {
		EM_DEBUG_EXCEPTION("%d > stream_len", pos + sizeof(param_list->param_count));
		EM_SAFE_FREE(param_list->byte_stream);
		goto FINISH_OFF;
	}

	memcpy((param_list->byte_stream + pos), &param_list->param_count, sizeof(param_list->param_count));

	/* Add param count */
	pos += sizeof(int);
	int index = 0, length = 0;

	/* stream format */
	/* | param1 length | (param1 data) | param2 length | (param2 data) | ... |*/
	/* if param is 0 long, the param data is omitted */
	for(index=0; index<param_list->param_count; index++) {
		length = emipc_get_length(param_list->params[index]);
		if (length < 0) {
		 	EM_DEBUG_EXCEPTION("index = %d, length = %d", index, length);
			EM_SAFE_FREE(param_list->byte_stream);
			goto FINISH_OFF;
		}

		if (pos + (int)sizeof(int) > stream_len) {
		 	EM_DEBUG_EXCEPTION("%d > stream_len", pos + sizeof(int));
			EM_SAFE_FREE(param_list->byte_stream);
			goto FINISH_OFF;
		}
		/* write param i length */
		memcpy((param_list->byte_stream+pos), &length, sizeof(int));
		pos += sizeof(int);

		if (pos + length > stream_len) {
			EM_DEBUG_EXCEPTION("%d > stream_len", pos + length);
			EM_SAFE_FREE(param_list->byte_stream);
			goto FINISH_OFF;
		}
		/* write param i data if length is greater than 0 */
		if (length > 0) {
			memcpy((param_list->byte_stream+pos), emipc_get_data(param_list->params[index]), length);
			pos += length;
		}
	}
	*stream_length = stream_len;

FINISH_OFF:

	EM_DEBUG_FUNC_END("param_list->byte_stream [%p]", param_list->byte_stream);
	return param_list->byte_stream;
}

EXPORT_API int emipc_sum_param_list_length(emipc_param_list *param_list)
{
	int length = sizeof(int) * eSTREAM_DATA;
	int index;
	for (index = 0; index < param_list->param_count; index++) {
		length += sizeof(int);
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
//		EM_DEBUG_EXCEPTION("Index value is not valid");
		return NULL;
	}
	return emipc_get_data(param_list->params[index]);
}

EXPORT_API int emipc_get_param_len_of_param_list(emipc_param_list *param_list, int index)
{
	EM_DEBUG_FUNC_BEGIN();
	if (index < 0 || index >= param_list->param_count) {
//		EM_DEBUG_EXCEPTION("Index value is not valid");
		return 0;
	}
	return emipc_get_length(param_list->params[index]);
}


