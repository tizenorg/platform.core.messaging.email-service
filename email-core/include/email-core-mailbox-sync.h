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
 * File :  email-core-mailbox_sync.h
 * Desc :  Mail Header Sync Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#ifndef __EMAIL_CORE_MAILBOX_SYNC_H__
#define __EMAIL_CORE_MAILBOX_SYNC_H__

#include "email-types.h"
#include "email-storage.h"
#include "email-core-mail.h"
#include "c-client.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

#define IMAGE_DISPLAY_PARTIAL_BODY_COUNT 30
typedef struct 
{
	char  *image_file_name;
	char *text_image;
	char *content_id;
	int   dec_len;
	char *mime_type;
	int   fully_downloaded;
} email_image_data;

typedef struct 
{
	char *bodystructure;
	int bodystructure_len;
	char *rfc822header;
	int rfc822header_len;
	char *bodytext;
	int bodytext_len;
	unsigned long uid_no;
} email_partial_buffer;

#endif


int pop3_mail_calc_rfc822_size(MAILSTREAM *stream, int msgno, int *size, int *err_code);
int pop3_mailbox_get_uids(MAILSTREAM *stream, emcore_uid_list** uid_list, int *err_code);

int imap4_mail_calc_rfc822_size(MAILSTREAM *stream, int msgno, int *size, int *err_code);
int imap4_mailbox_get_uids(MAILSTREAM *stream, char *input_target_uid_string, emcore_uid_list** uid_list, int *err_code);

int emcore_check_rule(const char *input_full_address_from, const char *input_subject, emstorage_rule_tbl_t *rule, int rule_len, int *priority_sender, int *blocked, int *err_code);

INTERNAL_FUNC int emcore_make_mail_tbl_data_from_envelope(char *multi_user_name, int account_id, MAILSTREAM *mail_stream, ENVELOPE *input_envelope, emcore_uid_list *input_uid_elem, emstorage_mail_tbl_t **output_mail_tbl_data,  int *err_code);

INTERNAL_FUNC int emcore_add_mail_to_mailbox(char *multi_user_name, emstorage_mailbox_tbl_t *input_maibox_data, emstorage_mail_tbl_t *input_new_mail_tbl_data, int *output_mail_id, int *output_thread_id);

#ifdef __FEATURE_BODY_SEARCH__
int emcore_add_mail_text(char *multi_user_name, emstorage_mailbox_tbl_t *input_maibox_data, emstorage_mail_tbl_t *input_new_mail_tbl_data, char *stripped_text, int *err_code);
#endif

/**
 * Download unread all headers from mail server.
 *
 * @param[in] input_mailbox_tbl	Specifies the mailbox to contain target mailbox name.
 *                      if the mailbox name is NULL, headers are downloaded from all synchronous mailbox.
 *                      the mailbox name is unused in POP3 case.
 * @param[in] stream_recycle Stream to reuse.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emcore_sync_header (char *multi_user_name, 
                                  emstorage_mailbox_tbl_t *input_mailbox_tbl,
                                  void **stream,
                                  emcore_uid_list **input_uid_list,
                                  int *mail_count,
                                  int *unread_mail,
                                  int *vip_mail_count,
                                  int *vip_unread_mail,
                                  int cancellable, /*if event thread calls, set it*/
                                  int event_handle,
                                  int *err_code);

typedef enum
{
	EM_CORE_GET_UIDS_FOR_NO_DELETE = 0, 
	EM_CORE_GET_UIDS_FOR_DELETE, 
} emcore_get_uids_for_delete_t;
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
int emcore_download_uid_all (char *multi_user_name, MAILSTREAM *mail_stream, email_internal_mailbox_t *mailbox, emcore_uid_list **uid_list,
                                int *total, emstorage_read_mail_uid_tbl_t *read_mail_uids, int count, 
					  emcore_get_uids_for_delete_t  for_delete, int *err_code);



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
int emcore_download_imap_msgno(char *multi_user_name, email_internal_mailbox_t *mailbox, char *uid, int *msgno, int *err_code);

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
int emcore_get_msgno(emcore_uid_list *uid_list, char *uid, int *msgno, int *err_code);

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
int emcore_get_uid(emcore_uid_list *uid_list, int msgno, char **uid, int *err_code);

/**
 * free fetch list.
 *
 * @param[in] uid_list Specifies the fetch data.
 * @param[out] err_code	Specifies the error code returned.
 * @return This function returns true on success or false on failure.
 */
int emcore_free_uids(emcore_uid_list *uid_list, int *err_code);
INTERNAL_FUNC int emcore_sync_mail_from_client_to_server(char *multi_user_name, int mail_id);
INTERNAL_FUNC int emcore_update_attachment_except_inline(char *multi_user_name, 
                                                        struct _m_content_info *cnt_info, 
                                                        int account_id, 
                                                        int mail_id, 
                                                        int mailbox_id, 
                                                        int *output_total_attachment_size, 
                                                        int *output_attachment_count, 
                                                        int *output_inline_attachment_count);

INTERNAL_FUNC int emcore_sync_mail_by_message_id(char *multi_user_name, 
													int mail_id, 
													int mailbox_id, 
													char **output_server_uid);
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
INTERNAL_FUNC int emcore_download_bulk_partial_mail_body(MAILSTREAM *stream, 
														email_event_partial_body_thd *pbd_event, 
														int count, 
														int *error);

INTERNAL_FUNC int emcore_initiate_pbd(char *multi_user_name, MAILSTREAM *stream, 
										int account_id, int mail_id, char *uid, 
										int input_mailbox_id, char *input_mailbox_name,	
										int *err_code);
#endif /* __FEATURE_PARTIAL_BODY_DOWNLOAD__ */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
