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

INTERNAL_FUNC int emcore_check_sync_imap_mailbox(emf_mailbox_t *mailbox, int *synchronous, int *err_code);
/**
 * Get mailbox list from imap server.
 *
 * @param[in] account_id	Specifies the account ID.
 * @param[in] mailbox	Specifies the target mailbox. if NULL, get all mailbox.
 * @param[in] callback	Specifies the callback function for retrieving mailbox list.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emcore_sync_mailbox_list(int account_id, char *mailbox, int *err_code);

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
INTERNAL_FUNC int emcore_download_mailbox_list(void *mail_stream, char *mailbox, emf_mailbox_t **mailbox_list, int *count, int *err_code);
INTERNAL_FUNC int emcore_delete_imap_mailbox(emf_mailbox_t *mailbox, int *err_code);
INTERNAL_FUNC int emcore_create_imap_mailbox(emf_mailbox_t *mailbox, int *err_code);
INTERNAL_FUNC int emcore_modify_imap_mailbox(emf_mailbox_t *old_mailbox,  emf_mailbox_t *new_mailbox, int *err_code);
INTERNAL_FUNC int emcore_set_sync_imap_mailbox(emf_mailbox_t *mailbox, int synchronous, int *err_code);
INTERNAL_FUNC int emcore_set_mail_slot_size(int account_id, char *mailbox_name, int new_slot_size, int *err_code);
INTERNAL_FUNC int emcore_remove_overflowed_mails(emstorage_mailbox_tbl_t *intput_mailbox_tbl, int *err_code);	
INTERNAL_FUNC int emcore_get_default_mail_slot_count(int *output_count, int *err_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
