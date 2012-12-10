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


#include <unistd.h>
#include <pthread.h>
#include <malloc.h>

#include "email-ipc.h"
#include "email-ipc-build.h"
#include "email-proxy-main.h"
#include "email-proxy-socket.h"
#include "email-proxy-callback-info.h"

#include "email-debug-log.h"


EXPORT_API int emipc_initialize_proxy_main()
{
	EM_DEBUG_FUNC_BEGIN();
	int sock_fd = 0;

	sock_fd = emipc_get_proxy_socket_id();
	
	if (sock_fd) {
		EM_DEBUG_LOG("Socket already initialized");
		return EMAIL_ERROR_IPC_ALREADY_INITIALIZED;
	}
	
	if (!emipc_start_proxy_socket()) {
		EM_DEBUG_EXCEPTION("Socket start failed");
		return EMAIL_ERROR_IPC_CONNECTION_FAILURE;
	}
	
	EM_DEBUG_LOG("Socket ID : %d", emipc_get_proxy_socket_id());
	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE; 
}

EXPORT_API int emipc_finalize_proxy_main()
{
	EM_DEBUG_FUNC_BEGIN();
	if (!emipc_end_proxy_socket()) {
		EM_DEBUG_EXCEPTION("emipc_finalize_proxy_main failed");
		return EMAIL_ERROR_IPC_SOCKET_FAILURE;
	}

	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

EXPORT_API bool emipc_execute_api_of_proxy_main(emipc_email_api_info *api_info)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret;
	unsigned char *in_stream = NULL;
	int length = 0;
	bool result = false;
	int sending_bytes;

	if (!api_info) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		return false;
	}

	in_stream = emipc_serialize_api_info(api_info, ePARAMETER_IN, &length);
	if( !in_stream ) {
		EM_DEBUG_EXCEPTION("NULL stream");
		return false;
	}

	sending_bytes = emipc_send_proxy_socket(in_stream, length);

	EM_DEBUG_LOG("Proxy=>stub sending %d byte.", sending_bytes);

	if (sending_bytes > 0) {
#ifdef IPCLIB_STREAM_TRACE_ON
		int index = 0;
		for (index=0;index<length;index++) 
			EM_DEBUG_LOG("in_stream[index] : [%x]", in_stream[index]);
#endif
		char *ipc_buf = NULL;

		ret = emipc_recv_proxy_socket(&ipc_buf);
	
		EM_DEBUG_LOG("Recv length : %d", ret);

		if (ret > 0)
			result = emipc_deserialize_api_info(api_info, ePARAMETER_OUT, ipc_buf);
		else
			result = false;	

		EM_SAFE_FREE(ipc_buf);
	}
	
	EM_DEBUG_FUNC_END();
	return result;		
}
