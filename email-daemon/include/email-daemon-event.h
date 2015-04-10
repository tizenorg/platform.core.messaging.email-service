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

#ifndef __EMAIL_DAEMON_EVENT_H__
#define __EMAIL_DAEMON_EVENT_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "email-types.h"
#include "email-internal-types.h"

INTERNAL_FUNC int emdaemon_start_event_loop(int *err_code);
INTERNAL_FUNC int emdaemon_start_event_loop_for_sending_mails(int *err_code);
INTERNAL_FUNC int emdaemon_start_thread_for_downloading_partial_body(int *err_code);

#ifdef __FEATURE_OPEN_SSL_MULTIHREAD_HANDLE__
INTERNAL_FUNC void emdaemon_setup_handler_for_open_ssl_multithread(void);
INTERNAL_FUNC void emdaemon_cleanup_handler_for_open_ssl_multithread(void);
#endif /* __FEATURE_OPEN_SSL_MULTIHREAD_HANDLE__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
