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


/**
 * This file defines all APIs of Auto Poll.
 * @file	emf-auto-poll.h
 * @author  
 * @version	0.1
 * @brief	This file is the header file of Auto Poll.
 */
#ifndef __EMF_AUTO_POLL_H__
#define __EMF_AUTO_POLL_H__

#include "em-core-types.h"

#ifdef __FEATURE_AUTO_POLLING__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

 #include "alarm.h"

#if !defined(EXPORT_API)
#define EXPORT_API __attribute__((visibility("default")))
#endif


EXPORT_API int emf_add_polling_alarm(int account_id, int alarm_interval);
EXPORT_API int emf_remove_polling_alarm(int account_id);
EXPORT_API int is_auto_polling_started(int account_id);
EXPORT_API int emf_alarm_polling_cb(alarm_id_t  alarm_id, void* user_param);
EXPORT_API int emf_free_account_alarm_binder_list();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FEATURE_AUTO_POLLING__ */

#endif /* __EMF_AUTO_POLL_H__ */
/* EOF */
