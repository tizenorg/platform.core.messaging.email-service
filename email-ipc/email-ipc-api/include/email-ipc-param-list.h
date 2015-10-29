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


#ifndef _IPC_PARAMLIST_H_
#define _IPC_PARAMLIST_H_

#include "email-ipc-param.h"
#include "email-types.h"

typedef enum {
	eSTREAM_APIID = 0,
	eSTREAM_RESID,
	eSTREAM_APPID,
	eSTREAM_COUNT,
	eSTREAM_DATA,
	eSTREAM_MAX
}IPC_STREAM_INFO;

typedef struct {
	int param_count;
	emipc_param params[10];
	unsigned char *byte_stream;
} emipc_param_list;

EXPORT_API emipc_param_list *emipc_create_param_list();

EXPORT_API void emipc_destroy_param_list(emipc_param_list *param_list);

EXPORT_API bool emipc_parse_stream_of_param_list(void *stream_data, emipc_param_list *param_list);

EXPORT_API unsigned char *emipc_serialize_param_list(emipc_param_list *param_list, int *stream_length);

EXPORT_API int emipc_sum_param_list_length (emipc_param_list *param_list);

EXPORT_API bool emipc_add_param_to_param_list(emipc_param_list *param_list, void *data, int len);

EXPORT_API void emipc_add_dynamic_param_to_param_list(emipc_param_list *param_list, void *data, int len);

EXPORT_API void *emipc_get_param_of_param_list(emipc_param_list *param_list, int index);

EXPORT_API int emipc_get_param_len_of_param_list(emipc_param_list *param_list, int index);

#endif	/* _IPC_PARAMLIST_H_ */


