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
 * email-core-task-manager.h
 *
 *  Created on: 2012. 11. 1.
 *      Author: kyuho.jo@samsung.com
 */

#ifndef EMAIL_CORE_TASK_MANAGER_H_
#define EMAIL_CORE_TASK_MANAGER_H_

#include "email-internal-types.h"
#include "email-core-tasks.h"

INTERNAL_FUNC int  emcore_init_task_handler_array();
INTERNAL_FUNC int  emcore_free_task_handler_array();

INTERNAL_FUNC int  emcore_encode_task_parameter(email_task_type_t input_task_type, void *input_task_parameter_struct, char **output_byte_stream, int *output_stream_size);
INTERNAL_FUNC int  emcore_decode_task_parameter(email_task_type_t input_task_type, char *input_byte_stream, int input_stream_size, void **output_task_parameter_struct);

INTERNAL_FUNC int  emcore_add_task_to_task_table(char *multi_user_name, email_task_type_t input_task_type, email_task_priority_t input_task_priority, char *input_task_parameter, int input_task_parameter_length, int *output_task_id);
INTERNAL_FUNC int  emcore_remove_task_from_task_table(char *multi_user_name, int input_task_id);
INTERNAL_FUNC int  emcore_get_active_task_by_thread_id(thread_t input_thread_id, email_active_task_t **output_active_task);

INTERNAL_FUNC void* emcore_default_async_task_handler(void *intput_param);
INTERNAL_FUNC void* emcore_default_sync_task_handler(void *intput_param);

INTERNAL_FUNC int  emcore_start_task_manager_loop();
INTERNAL_FUNC int  emcore_stop_task_manager_loop();

#endif /* EMAIL_CORE_TASK_MANAGER_H_ */
