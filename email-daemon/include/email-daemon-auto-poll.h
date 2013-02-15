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


/**
 * This file defines all APIs of Auto Poll.
 * @file	email-daemon-auto-poll.h
 * @author  
 * @version	0.1
 * @brief	This file is the header file of Auto Poll.
 */
#ifndef __EMAIL_DAEMON_AUTO_POLL_H__
#define __EMAIL_DAEMON_AUTO_POLL_H__

#include "email-internal-types.h"
#include "alarm.h"

#ifdef __FEATURE_AUTO_POLLING__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


INTERNAL_FUNC int emdaemon_add_polling_alarm(int account_id, int alarm_interval);
INTERNAL_FUNC int emdaemon_remove_polling_alarm(int account_id);
INTERNAL_FUNC int emdaemon_check_auto_polling_started(int account_id);
INTERNAL_FUNC int emdaemon_alarm_polling_cb(alarm_id_t  alarm_id, void* user_param);
INTERNAL_FUNC int emdaemon_free_account_alarm_binder_list();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FEATURE_AUTO_POLLING__ */

#endif /* __EMAIL_DAEMON_AUTO_POLL_H__ */
/* EOF */
