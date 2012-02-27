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
 * File :  em-core-mailbox_sync.h
 * Desc :  Mail Header Sync Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#ifndef __EM_CORE_MAILBOX_SYNC_H__
#define __EM_CORE_MAILBOX_SYNC_H__

#include "emf-types.h"
#include "em-storage.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#if !defined(EXPORT_API)
#define EXPORT_API __attribute__((visibility("default")))
#endif

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

#define IMAGE_DISPLAY_PARTIAL_BODY_COUNT 30
typedef struct 
{
	char  image_file_name[100];
	char *text_image;
	char *content_id;
	int   dec_len;
} emf_image_data;

typedef struct 
{
	char *buffer;
	int buflen;
} emf_partial_buffer;

#endif

typedef struct em_core_uid_elem {
	int msgno;
	char *uid;
	emf_mail_flag_t flag;
	struct em_core_uid_elem *next;
} em_core_uid_list; 

int pop3_mail_calc_rfc822_size(MAILSTREAM *stream, int msgno, int *size, int *err_code);
int pop3_mailbox_get_uids(MAILSTREAM *stream, em_core_uid_list** uid_list, int *err_code);

int imap4_mail_calc_rfc822_size(MAILSTREAM *stream, int msgno, int *size, int *err_code);
int imap4_mailbox_get_uids(MAILSTREAM *stream, em_core_uid_list** uid_list, int *err_code);

/**
 * Download unread all headers from mail server.
 *
 * @param[in] input_mailbox_tbl	Specifies the mailbox to contain target mailbox name.
 *                      if the mailbox name is NULL, headers are downloaded from all synchronous mailbox.
 *                      the mailbox name is unused in POP3 case.
 * @param[in] input_mailbox_tbl_spam	Mailbox information of Spambox for filtering blocked mails.
 * @param[in] stream_recycle Stream to reuse.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int em_core_mailbox_sync_header(emf_mailbox_tbl_t *input_mailbox_tbl, emf_mailbox_tbl_t *input_mailbox_tbl_spam, void *stream_recycle, em_core_uid_list **input_uid_list, int *unread_mail, int *err_code);

typedef enum
{
	EM_CORE_GET_UIDS_FOR_NO_DELETE = 0, 
	EM_CORE_GET_UIDS_FOR_DELETE, 
} em_core_get_uids_for_delete_t;
/**
 * Download UID list from mail server. 
 *
 * @param[in] mailbox	  Specifies the mailbox to contain target mailbox name.
 *                        the mailbox name is unused in POP3 case.
 * @param[out] uid_list The returned UID list is saved in a memory. this argument points to the list.
 * @param[out] total      Specifies the count of the uid_list.
 * @param[in] read_mail_uids Specifies the array of the uids have the seen flag.
 * @param[in] count		  Specifies the count of read_mail_uids.
 * @param[in] for_delete  Specifies the flag for deleting. (0 = for downloading or retrieving body, 1 = for deleting message)
 * @param[out] err_code	  Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
int em_core_mailbox_download_uid_all(emf_mailbox_t *mailbox, 
							em_core_uid_list		**uid_list,
							int 							*total,
							emf_mail_read_mail_uid_tbl_t *read_mail_uids, 
							int                           count, 
							em_core_get_uids_for_delete_t  for_delete, 
							int 							*err_code);



/**
 * Search a UID from mail server. this function is supported for only IMAP account.
 *
 * @param[in] mailbox	Specifies the mailbox to contain target mailbox name.
 * @param[in] uid		Specifies the mail uid.
 * @param[out] msgno	The message number to be related to uid is saved here.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
int em_core_mailbox_download_imap_msgno(emf_mailbox_t *mailbox, char *uid, int *msgno, int *err_code);

/**
 * Get a message number to be related to uid.
 *
 * @param[in] uid_list Specifies the uid list.
 * @param[in] uid		 Specifies the mail uid.
 * @param[out] msgno	 The message number to be related to uid is saved here.
 * @param[out] err_code	 Specifies the error code returned.
 * @remarks The uid list must be already saved in fpath before this fucntion is called.
 * @return This function returns true on success or false on failure.
 */
int em_core_mailbox_get_msgno(em_core_uid_list *uid_list, char *uid, int *msgno, int *err_code);

/**
 * Get a uid to be related to a message number.
 *
 * @param[in] uid_list Specifies the uid list.
 * @param[in] msgno		 Specifies the message number.
 * @param[out] uid		 The message uid is saved here.
 * @param[out] err_code	 Specifies the error code returned.
 * @remarks The uid list must be already saved in fpath before this fucntion is called.
 * @return This function returns true on success or false on failure.
 */
int em_core_mailbox_get_uid(em_core_uid_list *uid_list, int msgno, char **uid, int *err_code);

/**
 * free fetch list.
 *
 * @param[in] uid_list Specifies the fetch data.
 * @param[out] err_code	Specifies the error code returned.
 * @return This function returns true on success or false on failure.
 */
int em_core_mailbox_free_uids(em_core_uid_list *uid_list, int *err_code);

EXPORT_API int em_core_mail_sync_from_client_to_server(int account_id, int mail_id, int *err_code);
EXPORT_API int em_core_mail_check_rule(emf_mail_head_t *head, emf_mail_rule_tbl_t *rule, int rule_len, int *matched, int *err_code);

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
EXPORT_API int em_core_bulk_partial_mailbody_download(MAILSTREAM *stream, emf_event_partial_body_thd *pbd_event, int count, int *error);
#endif /* __FEATURE_PARTIAL_BODY_DOWNLOAD__ */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
