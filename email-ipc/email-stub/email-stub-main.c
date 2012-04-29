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

#include <glib.h>

#include "email-stub-main.h"
#include "email-ipc.h"
#include "email-stub-response-info.h"
#include "email-ipc-param-list.h"
#include "email-ipc-build.h"
#include "email-stub-socket.h"

#include "email-api.h"
#include "email-debug-log.h"

static bool stub_socket = false;
static GList *response_list = NULL;
static PFN_EXECUTE_API this_fn_api_mapper = NULL;

EXPORT_API bool emipc_initialize_stub_main(PFN_EXECUTE_API fn_api_mapper)
{
	EM_DEBUG_FUNC_BEGIN();

	if (!fn_api_mapper) {
		EM_DEBUG_EXCEPTION("Invalid Param");
		return false;
	}

	this_fn_api_mapper = fn_api_mapper;

	/* Initialize the socket */
	stub_socket = emipc_start_stub_socket();
	if (!stub_socket) {
		EM_DEBUG_EXCEPTION("Socket did not create");
		return false;
	}

	EM_DEBUG_FUNC_END();
	return true;
}

EXPORT_API bool emipc_finalize_stub_main()
{
	EM_DEBUG_FUNC_BEGIN();

	if (response_list)
		response_list = NULL;

	if (stub_socket) {
		emipc_end_stub_socket();
		stub_socket = false;
	}
	
	if (this_fn_api_mapper)
		this_fn_api_mapper = NULL;
		
	EM_DEBUG_FUNC_END();
	return true;
}

EXPORT_API bool emipc_execute_api_proxy_to_stub(emipc_email_api_info *api_info)
{
	EM_DEBUG_FUNC_BEGIN();
	if (!api_info) {
		EM_DEBUG_EXCEPTION("Invalid Param");
		return false;
	}
	
	if (this_fn_api_mapper) {
		this_fn_api_mapper(api_info);
	}

	EM_DEBUG_FUNC_END();
	return true;
}

EXPORT_API bool emipc_execute_api_stub_to_proxy(emipc_email_api_info *api_info)
{
	EM_DEBUG_FUNC_BEGIN("api_info [%p]", api_info);
	EM_IF_NULL_RETURN_VALUE(api_info, false);
	EM_DEBUG_LOG("APIID [%s], response Socket ID [%d], APPID [%d]", EM_APIID_TO_STR(emipc_get_api_id_of_api_info(api_info)), emipc_get_response_id_of_api_info(api_info), emipc_get_app_id_of_api_info(api_info));
	
	unsigned char *stream = emipc_get_stream_of_api_info(api_info, ePARAMETER_OUT);
	int stream_length = emipc_get_stream_length_of_api_info(api_info, ePARAMETER_OUT);
	
	EM_DEBUG_LOG("Data : %p, Data length : %dB", stream, stream_length);
	EM_DEBUG_LOG("Stub => Proxy Sending %dB", stream_length);
	emipc_send_stub_socket(emipc_get_response_id_of_api_info(api_info), stream, stream_length);
	
#ifdef IPCLIB_STREAM_TRACE_ON
	int index = 0;
	for (index = 0; index < stream_length; index++)
		EM_DEBUG_LOG("Stream[index] : [%d]", stream[index]);
#endif

	EM_DEBUG_FUNC_END();
	return true;
}

EXPORT_API bool emipc_set_response_info(long app_id, long api_id)
{
	EM_DEBUG_FUNC_BEGIN("Response ID [%d]", app_id);
	
	if (app_id <= 0)
		return true;

	emipc_email_response_info *response_info = NULL;
	response_info = (emipc_email_response_info *)malloc(sizeof(response_info));
	if (response_info == NULL) {
		EM_DEBUG_EXCEPTION("Malloc failed");
		return false;
	}
	memset(response_info, 0x00, sizeof(response_info));

	if (response_info) {
		emipc_set_value_in_response_info(response_info, app_id, api_id);
		response_list = g_list_append(response_list, response_info);
		return true;
	}
	EM_DEBUG_FUNC_END();
	return false;
}
