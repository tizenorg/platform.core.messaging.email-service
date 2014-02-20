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

/*
 * email-core-signal.h
 *
 *  Created on: 2012. 11. 22.
 *      Author: kyuho.jo@samsung.com
 */

#ifndef EMAIL_CORE_SIGNAL_H_
#define EMAIL_CORE_SIGNAL_H_

#include "email-internal-types.h"
#include "email-core-tasks.h"

INTERNAL_FUNC int emcore_initialize_signal();

INTERNAL_FUNC int emcore_finalize_signal();

INTERNAL_FUNC int em_send_notification_to_active_sync_engine(int subType, ASNotiData *data);

/* emcore_notify_storage_event - Notification for storage related operations */
INTERNAL_FUNC int emcore_notify_storage_event(email_noti_on_storage_event event_type, int data1, int data2 , char *data3, int data4);

/* emcore_notify_network_event - Notification for network related operations */
INTERNAL_FUNC int emcore_notify_network_event(email_noti_on_network_event event_type, int data1, char *data2, int data3, int data4);

/* emcore_notify_response_to_api - Notification for response to API */
INTERNAL_FUNC int emcore_notify_response_to_api(email_event_type_t event_type, int data1, int data2);

INTERNAL_FUNC int emcore_send_task_status_signal(email_task_type_t input_task_type, int input_task_id, email_task_status_type_t input_task_status, int input_param_1, int input_param_2);

#endif /* EMAIL_CORE_SIGNAL_H_ */
