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
 * File :  email-core-mailbox.h
 * Desc :  Local Mailbox Management Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#ifndef __EMAIL_CORE_MAILBOX_H__
#define __EMAIL_CORE_MAILBOX_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "c-client.h"
#include "email-types.h"
#include "email-internal-types.h"

typedef enum      
{
	EM_CORE_STREAM_TYPE_RECV = 0, 	
	EM_CORE_STREAM_TYPE_SEND, 		
} emcore_stream_type_t;

#ifdef __FEATURE_KEEP_CONNECTION__
INTERNAL_FUNC int emcore_remove_connection_info(int account_id);
#endif /* __FEATURE_KEEP_CONNECTION__ */
/*  in SMTP case, path argument must be (ENCODED_PATH_SMTP) */
/*  ex) emcore_connect_to_remote_mailbox(xxx, (char *)ENCODED_PATH_SMTP, xxx, xxx); */
INTERNAL_FUNC int  emcore_connect_to_remote_mailbox_with_account_info(emf_account_t *ref_account, char *mailbox, void **mail_stream, int *err_code);
INTERNAL_FUNC int  emcore_connect_to_remote_mailbox(int account_id, char *mailbox, void **mail_stream, int *err_code);
INTERNAL_FUNC int  emcore_close_mailbox(int account_id, void *mail_stream);
#ifdef __FEATURE_KEEP_CONNECTION__
INTERNAL_FUNC void emcore_close_mailbox_receiving_stream();
INTERNAL_FUNC void emcore_close_mailbox_partial_body_stream();
INTERNAL_FUNC void emcore_reset_streams();
#endif

INTERNAL_FUNC int  emcore_get_list_to_be_sync(int account_id, emf_mailbox_t **mailbox_list, int *p_count, int *err_code);
INTERNAL_FUNC int  emcore_get_list(int account_id, emf_mailbox_t **mailbox_list, int *p_count, int *err_code);
INTERNAL_FUNC int  emcore_free_mailbox(emf_mailbox_t **mailbox_list, int count, int *err_code);
INTERNAL_FUNC int  emcore_get_mail_count(emf_mailbox_t *mailbox, int *total, int *unseen, int *err_code);
INTERNAL_FUNC int  emcore_create_mailbox(emf_mailbox_t *new_mailbox, int on_server, int *err_code);
INTERNAL_FUNC int  emcore_delete_mailbox(emf_mailbox_t *mailbox, int on_server, int *err_code);
INTERNAL_FUNC int  emcore_delete_mailbox_all(emf_mailbox_t *mailbox, int *err_code);
INTERNAL_FUNC int  emcore_update_mailbox(emf_mailbox_t *old_mailbox, emf_mailbox_t *new_mailbox, int *err_code);
INTERNAL_FUNC int  emcore_save_local_activity_sync(int account_id, int *err_code);
INTERNAL_FUNC void emcore_bind_mailbox_type(emf_mailbox_t *mailbox_list);
INTERNAL_FUNC int  emcore_send_mail_event(emf_mailbox_t *mailbox, int mail_id , int *err_code);
INTERNAL_FUNC int  emcore_partial_body_thd_local_activity_sync(int *is_event_inserted, int *err_code);
INTERNAL_FUNC int  emcore_get_mailbox_by_type(int account_id, emf_mailbox_type_e mailbox_type, emf_mailbox_t *spam_mailbox, int *err_code);

#ifdef __FEATURE_LOCAL_ACTIVITY__
INTERNAL_FUNC int  emcore_local_activity_sync(int account_id, int *err_code);
INTERNAL_FUNC int  emcore_save_local_activity_sync(int account_id, int *err_code);
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
