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
 * File :  em-core-mesg.h
 * Desc :  Mail Operation Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#ifndef __EM_CORE_MESSAGE_H__
#define __EM_CORE_MESSAGE_H__

#include "em-storage.h"
#include <contacts-svc.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if !defined(EXPORT_API)
#define EXPORT_API __attribute__((visibility("default")))
#endif

#define HTML_EXTENSION_STRING ".htm"
#define MAX_PATH_HTML 256

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
	
} emf_uid_range_set;

#endif


struct _m_content_info 
{
	int grab_type;              /*  1 :  download text and get attachment names (no saving attachment) - #define GRAB_TYPE_TEXT       retrieve text and attachment names */
                              /*  2 :  download attachment - #define GRAB_TYPE_ATTACHMENT retrieve only attachment */
	int file_no;                /*  attachment no to be download (min : 1) */
	int report;                 /*  0 : Non 1 : DSN mail 2 : MDN mail 3 : mail to require MDN */

	struct text_data 
	{
		char *plain;            /*  body plain text */
		char *plain_charset;    /*  charset of body text */
		char *html;             /*  body html text */
	} text;

	struct attachment_info 
	{
		int   type;             /*  1 : inline 2 : attachment */
		char *name;             /*  attachment filename */
		int   size;             /*  attachment size */
		char *save;             /*  content saving filename */
		int   drm;              /*  0 : none 1 : object 2 : rights 3 : dcf */
		int   drm2;             /*  0 : none 1 : FL 2 : CD 3 : SSD 4 : SD */
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
 *          attachments must be downloaded in em_core_mail_download_attachment.
 * @return This function returns true on success or false on failure.
 */

EXPORT_API int em_core_mail_download_body_multi_sections_bulk(void *mail_stream, int account_id, int mail_id, int verbose, int with_attach, int limited_size, int event_handle , int *err_code);


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
EXPORT_API int em_core_mail_download_attachment(int acconut_id, int mail_id,  char *nth, int *err_code);

EXPORT_API int em_core_mail_get_info(int mail_id, emf_mail_info_t **info, int *err_code);
EXPORT_API int em_core_mail_get_header(int mail_id, emf_mail_head_t **head, int *err_code);
EXPORT_API int em_core_mail_get_body(int mail_id, emf_mail_body_t **body, int *err_code);
EXPORT_API int em_core_mail_move(int mail_ids[], int num, char *dst_mailbox_name, int noti_param_1, int noti_param_2, int *err_code);
EXPORT_API int em_core_mail_add_attachment(int mail_id, emf_attachment_info_t *attachment, int *err_code);
EXPORT_API int em_core_mail_delete_attachment(int mail_id,  char *attachment_id, int *err_code);
EXPORT_API int em_core_mail_add_attachment_data(int input_mail_id, emf_attachment_data_t *input_attachment_data);


#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
EXPORT_API int em_core_insert_pbd_activity(emf_event_partial_body_thd *local_activity, int *activity_id, int *err_code) ;
EXPORT_API int em_core_delete_pbd_activity(int account_id, int mail_id, int activity_id, int *err_code);
#endif 

EXPORT_API int em_core_mail_get_contact_info(emf_mail_contact_info_t *contact_info, char *full_address, int *err_code);
EXPORT_API int em_core_mail_get_contact_info_with_update(emf_mail_contact_info_t *contact_info, char *full_address, int mail_id, int *err_code);
EXPORT_API int em_core_mail_free_contact_info(emf_mail_contact_info_t *contact_info, int *err_code);
EXPORT_API int em_core_mail_contact_sync(int mail_id, int *err_code);
EXPORT_API GList *em_core_get_recipients_list(GList *old_recipients_list, char *full_address, int *err_code);
EXPORT_API int em_core_mail_get_address_info_list(int mail_id, emf_address_info_list_t **address_info_list, int *err_code);
EXPORT_API int em_core_mail_get_display_name(CTSvalue *contact_name_value, char **contact_display_name);
EXPORT_API int em_core_mail_get_mail(int msgno, emf_mail_t **mail, int *err_code);
EXPORT_API int em_core_get_mail_data(int input_mail_id, emf_mail_data_t **output_mail_data);

EXPORT_API int em_core_mail_update_old(int mail_id,  emf_mail_t *src_mail, emf_meeting_request_t *src_meeting_req,  int *err_code);
EXPORT_API int em_core_update_mail(emf_mail_data_t *input_mail_data, emf_attachment_data_t *input_attachment_data_list, int input_attachment_count, emf_meeting_request_t* input_meeting_request, int sync_server);

EXPORT_API int em_core_fetch_flags(int account_id, int mail_id, emf_mail_flag_t *mail_flag, int *err_code);
EXPORT_API int em_core_mail_delete_from_local(int account_id, int *mail_ids, int num, int noti_param_1, int noti_param_2, int *err_code);
EXPORT_API int em_core_mail_get_msgno_by_uid(emf_account_t *account, emf_mailbox_t *mailbox, char *uid, int *msgno, int *err_code);

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
EXPORT_API int em_core_mail_delete(int account_id, int mail_id[], int num, int from_server, int noti_param_1, int noti_param_2, int *err_code);

/**
 * Delete mails.
 *
 * @param[in] mailbox			Specifies the mailbox. this argument is for account id or mailbox name.
 * @param[in] with_server		Specifies whether mails is also deleted from server.
 * @param[in] callback			Specifies the callback function for delivering status during deleting.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int   em_core_mail_delete_all(emf_mailbox_t *mailbox, int with_server, int *err_code);

EXPORT_API int   em_core_mail_info_free(emf_mail_info_t **mail_info_list, int count, int *err_code);
EXPORT_API int   em_core_mail_body_free(emf_mail_body_t **mail_body_list, int count, int *err_code);
EXPORT_API int   em_core_mail_head_free(emf_mail_head_t **mail_head_list, int count, int *err_code);
EXPORT_API int   em_core_mail_free(emf_mail_t **mail_list, int count, int *err_code);
EXPORT_API int   em_core_free_mail_data(emf_mail_data_t **mail_list, int count, int *err_code);
EXPORT_API void  em_core_mime_free_content_info(struct _m_content_info *cnt_info);
EXPORT_API int   em_core_mail_attachment_info_free(emf_attachment_info_t **atch_info, int *err_code);
EXPORT_API int   em_core_free_attachment_data(emf_attachment_data_t **attachment_data_list, int attachment_data_count, int *err_code);

EXPORT_API int   em_core_mail_move_from_server(int account_id, char *src_mailbox,  int mail_ids[], int num, char *dest_mailbox, int *error_code);
EXPORT_API int   em_core_mail_sync_flag_with_server(int mail_id, int *err_code);
EXPORT_API int   em_core_mail_sync_seen_flag_with_server(int mail_ids[], int num, int *err_code);
EXPORT_API int   em_core_mail_get_attachment(int mail_id, char *attachment_no, emf_attachment_info_t **attachment, int *err_code);
EXPORT_API int   em_core_get_attachment_data_list(int input_mail_id, emf_attachment_data_t **output_attachment_data, int *output_attachment_count);
EXPORT_API int   em_core_mail_modify_extra_flag(int mail_id, emf_extra_flag_t new_flag, int *err_code);
EXPORT_API int   em_core_mail_modify_flag(int mail_id, emf_mail_flag_t new_flag, int sticky_flag, int *err_code);
EXPORT_API int   em_core_mail_get_size(int mail_id, int *mail_size, int *err_code);
EXPORT_API int   em_core_mail_set_flags_field(int account_id, int mail_ids[], int num, emf_flags_field_type field_type, int value, int *err_code);
EXPORT_API char* em_core_convert_mutf7_to_utf8(char *mailbox_name); 
EXPORT_API int   em_core_convert_string_to_structure(const char *encoded_string, void **struct_var, emf_convert_struct_type_e type);

EXPORT_API int   em_core_mail_get_info_from_mail_tbl(emf_mail_info_t **pp_mail_info, emf_mail_tbl_t *mail , int *err_code);
EXPORT_API int   em_core_mail_get_header_from_mail_tbl(emf_mail_head_t **header, emf_mail_tbl_t *mail , int *err_code); 
EXPORT_API int   em_core_mail_get_body_from_mail_tbl(emf_mail_body_t **p_body, emf_mail_tbl_t *mail, int *err_code);


#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__
EXPORT_API int   em_core_mail_sync_flags_field_with_server(int mail_ids[], int num, emf_flags_field_type field_type, int value, int *err_code);
EXPORT_API int   em_core_mail_move_from_server_ex(int account_id, char *src_mailbox,  int mail_ids[], int num, char *dest_mailbox, int *error_code);
#endif

#ifdef __ATTACHMENT_OPTI__
EXPORT_API int em_core_mail_download_attachment_bulk(/*emf_mailbox_t *mailbox, */ int account_id, int mail_id, char *nth,  int *err_code);
#endif
EXPORT_API int   em_core_mail_filter_by_rule(emf_rule_t *filter_info, int *err_code);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* EOF */
