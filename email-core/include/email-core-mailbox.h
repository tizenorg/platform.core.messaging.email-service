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
INTERNAL_FUNC int  emcore_connect_to_remote_mailbox_with_account_info(email_account_t *ref_account, int input_mailbox_id, void **mail_stream, int *err_code);
INTERNAL_FUNC int  emcore_connect_to_remote_mailbox(int account_id, int input_mailbox_id, void **mail_stream, int *err_code);
INTERNAL_FUNC int  emcore_close_mailbox(int account_id, void *mail_stream);
#ifdef __FEATURE_KEEP_CONNECTION__
INTERNAL_FUNC void emcore_close_mailbox_receiving_stream();
INTERNAL_FUNC void emcore_close_mailbox_partial_body_stream();
INTERNAL_FUNC void emcore_reset_streams();
#endif

INTERNAL_FUNC int  emcore_get_mailbox_list_to_be_sync(int account_id, email_mailbox_t **mailbox_list, int *p_count, int *err_code);
INTERNAL_FUNC int  emcore_get_mailbox_list(int account_id, email_mailbox_t **mailbox_list, int *p_count, int *err_code);
INTERNAL_FUNC int  emcore_get_mail_count(email_mailbox_t *mailbox, int *total, int *unseen, int *err_code);
INTERNAL_FUNC int  emcore_create_mailbox(email_mailbox_t *new_mailbox, int on_server, int *err_code);
INTERNAL_FUNC int  emcore_delete_mailbox(int input_mailbox_id, int input_on_server, int input_recursive);
INTERNAL_FUNC int  emcore_delete_mailbox_ex(int input_account_id, int *input_mailbox_id_array, int input_mailbox_id_count, int input_on_server, int input_recursive);
INTERNAL_FUNC int  emcore_delete_mailbox_all(email_mailbox_t *mailbox, int *err_code);
INTERNAL_FUNC int  emcore_rename_mailbox(int input_mailbox_id, char *input_new_mailbox_name, char *input_new_mailbox_alias, void *input_eas_data, int input_eas_data_length, int input_on_server, int input_recursive, int handle_to_be_published);
INTERNAL_FUNC int  emcore_save_local_activity_sync(int account_id, int *err_code);
INTERNAL_FUNC int  emcore_send_mail_event(email_mailbox_t *mailbox, int mail_id , int *err_code);
INTERNAL_FUNC int  emcore_partial_body_thd_local_activity_sync(int *is_event_inserted, int *err_code);
INTERNAL_FUNC int  emcore_get_mailbox_by_type(int account_id, email_mailbox_type_e mailbox_type, email_mailbox_t *spam_mailbox, int *err_code);
INTERNAL_FUNC void emcore_free_mailbox_list(email_mailbox_t **mailbox_list, int count);
INTERNAL_FUNC void emcore_free_mailbox(email_mailbox_t *mailbox);

INTERNAL_FUNC void emcore_bind_mailbox_type(email_internal_mailbox_t *mailbox_list);
INTERNAL_FUNC int  emcore_free_internal_mailbox(email_internal_mailbox_t **mailbox_list, int count, int *err_code);




#ifdef __FEATURE_LOCAL_ACTIVITY__
INTERNAL_FUNC int  emcore_local_activity_sync(int account_id, int *err_code);
INTERNAL_FUNC int  emcore_save_local_activity_sync(int account_id, int *err_code);
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
