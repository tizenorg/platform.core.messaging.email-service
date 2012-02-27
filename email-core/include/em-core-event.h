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


/******************************************************************************
 * File :  em-core-event.h
 * Desc :  Mail Engine Event Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#ifndef __EM_CORE_EVNET_H__
#define __EM_CORE_EVNET_H__

#include "emf-types.h"
#include "em-core-types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#if !defined(EXPORT_API)
#define EXPORT_API __attribute__((visibility("default")))
#endif

void em_core_execute_event_callback(emf_action_t action, int total, int done, int status, int account_id, int mail_id, int handle, int error);
int em_core_get_active_queue_idx(void);

EXPORT_API int em_core_get_current_thread_type();
EXPORT_API int em_core_register_event_callback(emf_action_t action, emf_event_callback callback, void *event_data);
EXPORT_API int em_core_unregister_event_callback(emf_action_t action, emf_event_callback callback);
EXPORT_API int em_core_get_pending_event(emf_action_t action, int account_id, int mail_id, emf_event_status_type_t *status);
EXPORT_API int em_core_event_loop_start(int *err_code);
EXPORT_API int em_core_event_loop_stop(int *err_code);
EXPORT_API int em_core_insert_event(emf_event_t *event_data, int *handle, int *err_code);
EXPORT_API int em_core_cancel_thread(int handle, void *arg, int *err_code);
EXPORT_API int em_core_send_event_loop_start(int *err_code);
EXPORT_API int em_core_send_event_loop_stop(int *err_code);
EXPORT_API int em_core_cancel_send_mail_thread(int handle, void *arg, int *err_code);
EXPORT_API int em_core_check_thread_status(void);
EXPORT_API void em_core_get_event_queue_status(int *on_sending, int *on_receiving);
EXPORT_API int em_core_insert_send_event(emf_event_t *event_data, int *handle, int *err_code);
EXPORT_API int em_core_get_receiving_event_queue(emf_event_t **event_queue, int *event_count, int *err);
EXPORT_API int em_core_cancel_all_threads_of_an_account(int account_id);
EXPORT_API int em_core_free_event(emf_event_t *event_data);

#ifdef _CONTACT_SUBSCRIBE_CHANGE_
EXPORT_API int em_core_contact_sync_handler();
EXPORT_API int em_core_init_last_sync_time(void);
EXPORT_API int em_core_set_last_sync_time(int sync_time);
EXPORT_API int em_core_get_last_sync_time(void);
EXPORT_API int em_core_contact_sync_handler();
#endif

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
/*  Please contact -> Himanshu [h.gahlaut@samsung.com] for any explanation in code here under this MACRO */
EXPORT_API int em_core_insert_partial_body_thread_event(emf_event_partial_body_thd *partial_body_thd_event, int *error_code);
EXPORT_API int em_core_is_partial_body_thd_que_empty();
EXPORT_API int em_core_is_partial_body_thd_que_full();
EXPORT_API int em_core_partial_body_thread_loop_start(int *err_code);
EXPORT_API int em_core_clear_partial_body_thd_event_que(int *err_code);
EXPORT_API int em_core_free_partial_body_thd_event(emf_event_partial_body_thd *partial_body_thd_event, int *error_code);
EXPORT_API unsigned int em_core_get_partial_body_thd_id();
EXPORT_API int em_core_get_pbd_thd_state();
unsigned int em_core_get_receiving_thd_id();
#endif



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
