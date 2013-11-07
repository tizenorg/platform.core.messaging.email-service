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
 * File :  email-core-mail.h
 * Desc :  Mail Operation Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#ifndef __EMAIL_CORE_MESSAGE_H__
#define __EMAIL_CORE_MESSAGE_H__

#include "email-storage.h"
#include <contacts.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__

#define MAX_SUBSET_STRING_SIZE 260	
#define MAX_IMAP_COMMAND_LENGTH 1000
#define MAX_TAG_SIZE 16


typedef struct _emf_uid_range_set
{
	char *uid_range;
	unsigned long lowest_uid;
	unsigned long highest_uid;
	
	struct _emf_uid_range_set *next;
	
} email_uid_range_set;

#endif


struct _m_content_info 
{
	int grab_type;              /*  1 :  download text and get attachment names (no saving attachment) - #define GRAB_TYPE_TEXT       retrieve text and attachment names */
                              /*  2 :  download attachment - #define GRAB_TYPE_ATTACHMENT retrieve only attachment */
	int file_no;                /*  attachment no to be download (min : 1) */
	int report;                 /*  0 : Non 1 : DSN mail 2 : MDN mail 3 : mail to require MDN */

	int multipart;

	struct text_data 
	{
		char *plain;            /*  body plain text */
		char *plain_charset;    /*  charset of body text */
		char *html;             /*  body html text */
		char *html_charset;     /*  charset of html text */
	} text;

	struct attachment_info 
	{
		int   type;                 /*  1 : inline 2 : attachment */
		char *name;                 /*  attachment filename */
		int   size;                 /*  attachment size */
		char *save;                 /*  content saving filename */
		int   drm;                  /*  0 : none 1 : object 2 : rights 3 : dcf */
		int   drm2;                 /*  0 : none 1 : FL 2 : CD 3 : SSD 4 : SD */
		char *attachment_mime_type; /*  attachment mime type */
		char *content_id;           /*  mime content id */
#ifdef __ATTACHMENT_OPTI__
		int   encoding;         /*  encoding  */
		char *section;          /*  section number */
#endif
		struct attachment_info *next;
	} *file;
};

/**
 * Download email body from server.
 *
 * @param[in] mail_stream	Specifies the mail_stream.
 * @param[in] mailbox		Specifies the mailbox to contain account ID.
 * @param[in] mail_id		Specifies the mail ID.
 * @param[in] callback		Specifies the callback function for retrieving download status.
 * @param[in] with_attach	Specifies the flag for downloading attachments.
 * @param[in] limited_size	Specifies the size to be downloaded.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks In POP3 case, body and attachment are downloaded in this function.
 *          In IMAP case, body and attachment list are downloaded and 
 *          attachments must be downloaded in emcore_download_attachment.
 * @return This function returns true on success or false on failure.
 */

INTERNAL_FUNC int emcore_download_body_multi_sections_bulk(void *mail_stream, int account_id, int mail_id, int verbose, int with_attach, int limited_size, int event_handle , int *err_code);


/**
 * Download a email nth-attachment from server.
 *
 * @param[in] mailbox		Specifies the mailbox to contain account ID.
 * @param[in] mail_id			Specifies the mail ID.
 * @param[in] nth				Specifies the buffer that a attachment number been saved. the minimum number is "1".
 * @param[in] callback		Specifies the callback function for retrieving download status.
 * @param[in] handle			Specifies the handle for stopping downloading.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks This function is used for only IMAP mail.
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emcore_download_attachment(int acconut_id, int mail_id, int nth, int *err_code);
INTERNAL_FUNC int emcore_mail_add_attachment(int mail_id, email_attachment_data_t *attachment, int *err_code);        /* TODO : Remove duplicated function */
INTERNAL_FUNC int emcore_mail_add_attachment_data(int input_mail_id, email_attachment_data_t *input_attachment_data); /* TODO : Remove duplicated function */
INTERNAL_FUNC int emcore_delete_mail_attachment(int attachment_id, int *err_code);
INTERNAL_FUNC int emcore_get_attachment_info(int attachment_id, email_attachment_data_t **attachment, int *err_code);
INTERNAL_FUNC int emcore_get_attachment_data_list(int input_mail_id, email_attachment_data_t **output_attachment_data, int *output_attachment_count);
INTERNAL_FUNC int emcore_free_attachment_data(email_attachment_data_t **attachment_data_list, int attachment_data_count, int *err_code);


INTERNAL_FUNC int emcore_move_mail(int mail_ids[], int num, int dst_mailbox_id, int noti_param_1, int noti_param_2, int *err_code);

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
INTERNAL_FUNC int emcore_insert_pbd_activity(email_event_partial_body_thd *local_activity, int *activity_id, int *err_code) ;
INTERNAL_FUNC int emcore_delete_pbd_activity(int account_id, int mail_id, int activity_id, int *err_code);
#endif 

INTERNAL_FUNC int emcore_get_mail_contact_info(email_mail_contact_info_t *contact_info, char *full_address, int *err_code);
INTERNAL_FUNC int emcore_get_mail_contact_info_with_update(email_mail_contact_info_t *contact_info, char *full_address, int mail_id, int *err_code);
INTERNAL_FUNC int emcore_free_contact_info(email_mail_contact_info_t *contact_info, int *err_code);
INTERNAL_FUNC int emcore_sync_contact_info(int mail_id, int *err_code);
INTERNAL_FUNC GList *emcore_get_recipients_list(GList *old_recipients_list, char *full_address, int *err_code);
INTERNAL_FUNC int emcore_get_mail_address_info_list(int mail_id, email_address_info_list_t **address_info_list, int *err_code);

INTERNAL_FUNC int emcore_set_sent_contacts_log(emstorage_mail_tbl_t *input_mail_data);
INTERNAL_FUNC int emcore_set_received_contacts_log(emstorage_mail_tbl_t *input_mail_data);
INTERNAL_FUNC int emcore_delete_contacts_log(int input_account_id);

INTERNAL_FUNC int emcore_get_mail_display_name(char *email_address, char **contact_display_name, int *err_code);
INTERNAL_FUNC int emcore_get_mail_data(int input_mail_id, email_mail_data_t **output_mail_data);

INTERNAL_FUNC int emcore_update_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int sync_server);

INTERNAL_FUNC int emcore_delete_mails_from_local_storage(int account_id, int *mail_ids, int num, int noti_param_1, int noti_param_2, int *err_code);
INTERNAL_FUNC int emcore_get_mail_msgno_by_uid(email_account_t *account, email_internal_mailbox_t *mailbox, char *uid, int *msgno, int *err_code);
INTERNAL_FUNC int emcore_expunge_mails_deleted_flagged_from_local_storage(int input_mailbox_id);
INTERNAL_FUNC int emcore_expunge_mails_deleted_flagged_from_remote_server(int input_account_id, int input_mailbox_id);

/**
 * Delete mails.
 *
 * @param[in] account_id      Specifies the account id.
 * @param[in] mail_id         Specifies the array for mail id.
 * @param[in] num             Specifies the number of id.
 * @param[in] from_server     Specifies whether mails is deleted from server.
 * @param[in] callback        Specifies the callback function for delivering status during deleting.
 * @param[in] noti_param_1    Specifies the first parameter for notification.
 * @param[in] noti_param_2    Specifies the second parameter for notification.
 * @param[out] err_code       Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emcore_delete_mail(int account_id, int mail_id[], int num, int from_server, int noti_param_1, int noti_param_2, int *err_code);

/**
 * Delete mails.
 *
 * @param[in] input_mailbox_id	Specifies the id of mailbox.
 * @param[in] input_from_server	Specifies whether mails is also deleted from server.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int   emcore_delete_all_mails_of_acount(int input_account_id);
INTERNAL_FUNC int   emcore_delete_all_mails_of_mailbox(int input_account_id, int input_mailbox_id, int input_from_server, int *err_code);

INTERNAL_FUNC void  emcore_free_mail_data_list(email_mail_data_t **mail_list, int count);
INTERNAL_FUNC void  emcore_free_mail_data(email_mail_data_t *mail);
INTERNAL_FUNC void  emcore_free_content_info(struct _m_content_info *cnt_info);
INTERNAL_FUNC void  emcore_free_attachment_info(struct attachment_info *attchment);

INTERNAL_FUNC int   emcore_move_mail_on_server(int account_id, int src_mailbox_id,  int mail_ids[], int num, char *dest_mailbox, int *error_code);
INTERNAL_FUNC int   emcore_move_mail_to_another_account(int input_mail_id, int input_source_mailbox_id, int input_target_mailbox_id, int input_task_id);
INTERNAL_FUNC int   emcore_sync_flag_with_server(int mail_id, int *err_code);
INTERNAL_FUNC int   emcore_sync_seen_flag_with_server(int mail_ids[], int num, int *err_code);

INTERNAL_FUNC int   emcore_set_flags_field(int account_id, int mail_ids[], int num, email_flags_field_type field_type, int value, int *err_code);
INTERNAL_FUNC char* emcore_convert_mutf7_to_utf8(char *mailbox_name); 
INTERNAL_FUNC int   emcore_convert_string_to_structure(const char *encoded_string, void **struct_var, email_convert_struct_type_e type);
INTERNAL_FUNC int   emcore_save_mail_file(int account_id, int mail_id, int attachment_id, char *src_file_path, char *file_name, char *full_path, int *err_code);

#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__
INTERNAL_FUNC int   emcore_sync_flags_field_with_server(int mail_ids[], int num, email_flags_field_type field_type, int value, int *err_code);
INTERNAL_FUNC int   emcore_move_mail_on_server_ex(int account_id, int src_mailbox_id,  int mail_ids[], int num, int dest_mailbox_id, int *error_code);
#endif

#ifdef __ATTACHMENT_OPTI__
INTERNAL_FUNC int emcore_download_attachment_bulk(/*email_mailbox_t *mailbox, */ int account_id, int mail_id, char *nth,  int *err_code);
#endif
INTERNAL_FUNC int   emcore_mail_filter_by_rule(email_rule_t *filter_info, int *err_code);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* EOF */
