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

#include "email-internal-types.h"
#include "email-debug-log.h"

static int reference = 0;

pthread_mutex_t ipc_proxy_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ipc_proxy_cond = PTHREAD_COND_INITIALIZER;


EXPORT_API int emipc_initialize_proxy_main()
{
	EM_DEBUG_FUNC_BEGIN();
	
	if (reference > 0) {
		EM_DEBUG_EXCEPTION("Already Intialized reference[%d]", reference);
		return EMF_ERROR_NONE;
	}

	reference++;
	
	if (!emipc_start_proxy_socket()) {
		EM_DEBUG_EXCEPTION("Socket start failed");
		return EMF_ERROR_IPC_CONNECTION_FAILURE;
	}
	
	EM_DEBUG_LOG("Socket ID : %d", emipc_get_proxy_socket_id());
	EM_DEBUG_FUNC_END();
	return EMF_ERROR_NONE; 
}

EXPORT_API int emipc_finalize_proxy_main()
{
	EM_DEBUG_FUNC_BEGIN();
	
	if (--reference > 0) {
		EM_DEBUG_EXCEPTION("More than one reference[%d]", reference);
		return EMF_ERROR_NONE;
	}

	EM_DEBUG_FUNC_END();
	return EMF_ERROR_NONE;
}

EXPORT_API bool emipc_execute_api_of_proxy_main(emipc_email_api_info *api_info)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret;
	unsigned char *in_stream = emipc_get_stream_of_api_info(api_info, ePARAMETER_IN);
	int length = emipc_get_stream_length_of_api_info(api_info, ePARAMETER_IN);
	bool result = false;
	int sending_bytes;

	if (!api_info) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		return false;
	}

	ENTER_CRITICAL_SECTION(ipc_proxy_mutex);
	sending_bytes = emipc_send_proxy_socket(in_stream, length);
	LEAVE_CRITICAL_SECTION(ipc_proxy_mutex);	

	EM_DEBUG_LOG("Proxy=>stub sending %d byte.", sending_bytes);

	if (sending_bytes > 0) {
#ifdef IPCLIB_STREAM_TRACE_ON
		int index = 0;
		for (index=0;index<length;index++) 
			EM_DEBUG_LOG("in_stream[index] : [%x]", in_stream[index]);
#endif
		char *ipc_buf = NULL;
			
		ENTER_CRITICAL_SECTION(ipc_proxy_mutex);
		ret = emipc_recv_proxy_socket(&ipc_buf);
		LEAVE_CRITICAL_SECTION(ipc_proxy_mutex);

		EM_DEBUG_LOG("Recv length : %d", ret);

		if (ret > 0)
			result = emipc_parse_stream_of_api_info(api_info, ePARAMETER_OUT, ipc_buf);
		else
			result = false;	

		EM_SAFE_FREE(ipc_buf);
	}
	
	EM_DEBUG_FUNC_END();
	return result;		
}
#if 0
EXPORT_API bool emipc_dispatch_proxy_main()
{
	EM_DEBUG_FUNC_BEGIN();
	
	unsigned char stream[IPC_MSGQ_SIZE];
	emipc_email_api_info *api_info = NULL;
	int api_id = 0;
	int stream_length = 0;

	while(!recv_stop_flag) {
		memset(stream, 0x00, sizeof(stream));

		if (emcm_recv_msg(stream, sizeof(stream)) > 0) {
			api_info = (emipc_email_api_info *)malloc(sizeof(emipc_email_api_info));
			if (api_info == NULL) {
				EM_DEBUG_EXCEPTION("Malloc failed.");
				return false;
			}
			memset(api_info, 0x00, sizeof(emipc_email_api_info));

			api_id = *((int *)stream);

			emipc_set_api_id_of_api_info(api_info, api_id);
			emipc_parse_stream_of_api_info(api_info, ePARAMETER_OUT, stream);
			stream_length = emipc_get_stream_length_of_api_info(api_info, ePARAMETER_OUT);
			EM_DEBUG_LOG("Proxy Message Queue Recv [api_id=%x], [Recv_len=%d]", api_id, stream_length);
			
#ifdef IPCLIB_STREAM_TRACE_ON
			int index = 0;
			for (index = 0; index<stream_length; index++)
				EM_DEBUG_LOG("[%x]", stream[index]);
#endif
		}
		usleep(1000);				
	}
	EM_DEBUG_FUNC_END();
	return false;
}
#endif
