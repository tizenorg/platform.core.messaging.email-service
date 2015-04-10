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
 * File :  email-core-imap-mailbox.h
 * Desc :  Mail IMAP mailbox Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.01  :  created
 *****************************************************************************/
#ifndef __EMAIL_CORE_IMAP_MAILBOX_H__
#define __EMAIL_CORE_IMAP_MAILBOX_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 * Get mailbox list from imap server.
 *
 * @param[in] account_id	Specifies the account ID.
 * @param[in] mailbox	Specifies the target mailbox. if NULL, get all mailbox.
 * @param[in] callback	Specifies the callback function for retrieving mailbox list.
 * @param[in] handle	Specifies the handle for stopping this job.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emcore_sync_mailbox_list(char *multi_user_name, int account_id, char *mailbox, int event_handle, int *err_code);

/**
 * Download mailbox list from imap server.
 *
 * @param[in] stream	Specifies the internal mail stream.
 * @param[in] mailbox	Specifies the target mailbox. if NULL, get all mailbox.
 * @param[out] mailbox_list	The returned mailbox are saved here.
 * @param[out] count	The count of mailbox is saved here.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emcore_download_mailbox_list(void *mail_stream, char *mailbox, email_internal_mailbox_t **mailbox_list, int *count, int *err_code);
INTERNAL_FUNC int emcore_delete_imap_mailbox(char *multi_user_name, int input_mailbox_id, int *err_code);
INTERNAL_FUNC int emcore_create_imap_mailbox(char *multi_user_name, email_mailbox_t *mailbox, int *err_code);
INTERNAL_FUNC int emcore_rename_mailbox_on_imap_server(char *multi_user_name, int input_account_id, int input_mailbox_id, char *input_old_mailbox_path, char *input_new_mailbox_path, int handle_to_be_published);
INTERNAL_FUNC int emcore_set_mail_slot_size(char *multi_user_name, int account_id, int mailbox_id, int new_slot_size, int *err_code);
INTERNAL_FUNC int emcore_remove_overflowed_mails(char *multi_user_name, emstorage_mailbox_tbl_t *intput_mailbox_tbl, int *err_code);	
INTERNAL_FUNC int emcore_get_default_mail_slot_count(char *multi_user_name, int input_account_id, int *output_count);

#ifdef __FEATURE_IMAP_QUOTA__
INTERNAL_FUNC int emcore_register_quota_callback();
INTERNAL_FUNC int emcore_get_quota_root(int input_mailbox_id, email_quota_resource_t *output_list_of_resource_limits);
INTERNAL_FUNC int emcore_get_quota(int input_mailbox_id, char *input_quota_root, email_quota_resource_t *output_list_of_resource_limits);
INTERNAL_FUNC int emcore_set_quota(int input_mailbox_id, char *input_quota_root, email_quota_resource_t *input_list_of_resource_limits);
#endif /* __FEATURE_IMAP_QUOTA__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
