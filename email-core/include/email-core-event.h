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


/******************************************************************************
 * File :  email-core-event.h
 * Desc :  Mail Engine Event Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#ifndef __EMAIL_CORE_EVNET_H__
#define __EMAIL_CORE_EVNET_H__

#include "email-types.h"
#include "email-internal-types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

void emcore_execute_event_callback(email_action_t action, int total, int done, int status, int account_id, int mail_id, int handle, int error);
int  emcore_get_active_queue_idx(void);
int  emcore_check_send_mail_thread_status();

INTERNAL_FUNC int          emcore_get_current_thread_type();
INTERNAL_FUNC int          emcore_register_event_callback(email_action_t action, email_event_callback callback, void *event_data);
INTERNAL_FUNC int          emcore_unregister_event_callback(email_action_t action, email_event_callback callback);
INTERNAL_FUNC int          emcore_start_event_loop(int *err_code);
INTERNAL_FUNC int          emcore_stop_event_loop(int *err_code);
INTERNAL_FUNC int          emcore_insert_event(email_event_t *event_data, int *handle, int *err_code);
INTERNAL_FUNC int          emcore_cancel_thread(int handle, void *arg, int *err_code);
INTERNAL_FUNC int          emcore_start_event_loop_for_sending_mails(int *err_code);
INTERNAL_FUNC int          emcore_send_event_loop_stop(int *err_code);
INTERNAL_FUNC int          emcore_cancel_send_mail_thread(int handle, void *arg, int *err_code);
INTERNAL_FUNC int          emcore_check_thread_status(void);
INTERNAL_FUNC void         emcore_get_event_queue_status(int *on_sending, int *on_receiving);
INTERNAL_FUNC int          emcore_insert_event_for_sending_mails(email_event_t *event_data, int *handle, int *err_code);
INTERNAL_FUNC int          emcore_get_receiving_event_queue(email_event_t **event_queue, int *event_count, int *err);
INTERNAL_FUNC int          emcore_cancel_all_threads_of_an_account(int account_id);
INTERNAL_FUNC int          emcore_free_event(email_event_t *event_data);
INTERNAL_FUNC int          emcore_get_task_information(email_task_information_t **output_task_information, int *output_task_information_count);

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
/*  Please contact -> Himanshu [h.gahlaut@samsung.com] for any explanation in code here under this MACRO */
INTERNAL_FUNC int          emcore_insert_partial_body_thread_event(email_event_partial_body_thd *partial_body_thd_event, int *error_code);
INTERNAL_FUNC int          emcore_is_partial_body_thd_que_empty();
INTERNAL_FUNC int          emcore_is_partial_body_thd_que_full();
INTERNAL_FUNC int          emcore_start_thread_for_downloading_partial_body(int *err_code);
INTERNAL_FUNC int          emcore_clear_partial_body_thd_event_que(int *err_code);
INTERNAL_FUNC int          emcore_free_partial_body_thd_event(email_event_partial_body_thd *partial_body_thd_event, int *error_code);
INTERNAL_FUNC unsigned int emcore_get_partial_body_thd_id();
INTERNAL_FUNC int          emcore_get_pbd_thd_state();
unsigned int emcore_get_receiving_thd_id();
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
