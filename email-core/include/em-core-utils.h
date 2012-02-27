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
 * File :  em-core-utils.h
 * Desc :  Mail Utils Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#ifndef __EM_CORE_UTILS_H__
#define __EM_CORE_UTILS_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "emf-types.h"
#include "em-storage.h"
#include "em-core-global.h"
#include "em-core-mesg.h"
#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__
#include "em-core-mesg.h"
#endif

#if !defined(EXPORT_API)
#define EXPORT_API __attribute__((visibility("default")))
#endif

/*  This is used for em_core_get_long_encoded_path */
#define	ENCODED_PATH_SMTP	"UHDF_ENCODED_PATH_SMTP_EKJD"

typedef int (*emf_get_unread_email_count_cb)(int unread, int *err_code);

/* parse the Full mailbox Path and get the Alias Name of the Mailbox */
char *em_core_get_alias_of_mailbox(const char *mailbox_path);

/* Parse the Mailbox Path and get the Account Email address */
EXPORT_API emf_option_t *em_core_get_option(int *err_code);
EXPORT_API int em_core_set_option(emf_option_t *opt, int *err_code);
int em_core_upper_path(char *path);
int em_core_upper_string(char *str);
int em_core_get_temp_file_name(char **filename, int *err_code);
int em_core_get_long_encoded_path(int account_id, char *path, int delimiter, char **long_enc_path, int *err_code);
int em_core_get_encoded_mailbox_name(char *name, char **enc_name, int *err_code);
int em_core_get_file_name(char *path, char **filename, int *err_code);
int em_core_get_file_size(char *path, int *size, int *err_code);
int em_core_get_mail_size(emf_mail_t *mail_src, int *error_code);
int em_core_calc_mail_size(emf_mail_data_t *mail_data_src, emf_attachment_data_t *attachment_data_src, int attachment_count, int *error_code);
int em_core_get_actual_mail_size(char *pBodyPlane, char *pBodyHtml, struct attachment_info *pAttachment, int *error_code);
int em_core_get_address_count(char *addr_str, int *to_num, int *err_code);

void  em_core_skip_whitespace(char *addr_str , char **pAddr);
EXPORT_API char *em_core_skip_whitespace_without_strdup(char *source_string);
EXPORT_API char* em_core_replace_string(char *source_string, char *old_string, char *new_string);
EXPORT_API int em_core_set_network_error(int err_code);
EXPORT_API int em_core_check_unread_mail();
int em_core_is_storage_full(int *error);
int em_core_show_popup(int id, emf_action_t action, int error);
int em_core_get_long_encoded_path_with_account_info(emf_account_t *account, char *path, int delimiter, char **long_enc_path, int *err_code);
EXPORT_API int em_storage_get_emf_error_from_em_storage_error(int error);
EXPORT_API int em_core_open_contact_db_library(void);
EXPORT_API void em_core_close_contact_db_library();
EXPORT_API void em_core_flush_memory();
EXPORT_API int em_core_get_server_time(void *mail_stream , int account_id, char *uid, int msgno, time_t *log_time, int *err_code);
EXPORT_API void em_core_fill_address_information_of_mail_tbl(emf_mail_tbl_t *mail_data);
EXPORT_API int em_core_add_transaction_info(int mail_id , int handle  , int *err_code);
EXPORT_API int em_core_get_handle_by_mailId_from_transaction_info(int mail_id , int *pHandle);
EXPORT_API int em_core_delete_transaction_info_by_mailId(int mail_id);
EXPORT_API int em_core_get_preview_text_from_file(const char *input_plain_path, const char *input_html_path, int input_preview_buffer_length, char **output_preview_buffer);
EXPORT_API int em_core_strip_HTML(char *source_string);
EXPORT_API int em_core_send_noti_for_new_mail(int account_id, char *mailbox_name, char *subject, char *from, char *uid, char *datetime);
EXPORT_API int em_core_find_pos_stripped_subject_for_thread_view(char *subject, char *stripped_subject);
EXPORT_API int em_core_find_tag_for_thread_view(char *subject, int *result);
EXPORT_API int em_core_check_send_mail_thread_status();
EXPORT_API int em_core_make_attachment_file_name_with_extension(char *source_file_name, char *sub_type, char *result_file_name, int result_file_name_buffer_length, int *err_code);
EXPORT_API char *em_core_get_extension_from_file_path(char *source_file_path, int *err_code);
EXPORT_API int em_core_get_encoding_type_from_file_path(const char *input_file_path, char **output_encoding_type);
EXPORT_API int em_core_get_content_type(const char *extension_string, int *err_code);
EXPORT_API char* em_core_get_current_time_string(int *err_code);


/* Session Handling */
EXPORT_API int em_core_get_empty_session(emf_session_t **session);
EXPORT_API int em_core_clear_session(emf_session_t *session);
EXPORT_API int em_core_get_current_session(emf_session_t **session);

/* For notification */ 
EXPORT_API int em_core_update_notification_for_unread_mail(int account_id);
EXPORT_API int em_core_clear_all_notifications();
EXPORT_API int em_core_add_notification_for_unread_mail_by_mail_header(int account_id, int mail_id, emf_mail_head_t *mail_header);
EXPORT_API int em_core_add_notification_for_unread_mail(emf_mail_data_t *input_mail_data);
EXPORT_API int em_core_delete_notification_for_read_mail(int mail_id);
EXPORT_API int em_core_delete_notification_by_account(int account_id);
EXPORT_API int em_core_finalize_sync(int account_id, int *error);

EXPORT_API int em_core_verify_email_address(char *address, int without_bracket, int *err_code);
EXPORT_API int em_core_verify_email_address_of_mail_header(emf_mail_head_t *mail_header, int without_bracket, int *err_code);
EXPORT_API int em_core_verify_email_address_of_mail_data(emf_mail_data_t *mail_data, int without_bracket, int *err_code);


#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__

/**
 * @fn em_core_convert_to_uid_range_set(emf_id_set_t *id_set, int id_set_count, emf_uid_range_set **uid_range_set, int range_len, int *err_code)
 * Prepare a linked list of uid ranges with each node having a uid_range and lowest and highest uid in it.
 *
 *@author 					h.gahlaut@samsung.com
 * @param[in] id_set			Specifies the array of mail_id and corresponding server_mail_id sorted by server_mail_ids in ascending order
 * @param[in] id_set_count		Specifies the no. of cells in id_set array i.e. no. of sets of mail_ids and server_mail_ids
 * @param[in] range_len		Specifies the maximum length of string of range allowed. 
 * @param[out] uid_range_set	Returns the uid_ranges formed in the form of a linked list with head stored in uid_range_set pointer
 * @param[out] err_code		Returns the error code.
 * @remarks 					An example of a uid_range formed is 2 : 6, 8, 10, 14 : 15, 89, 
 *							While using it the caller should remove the ending , (comma)					
 * @return This function returns true on success or false on failure.
 */
 
EXPORT_API int em_core_convert_to_uid_range_set(emf_id_set_t *id_set, int id_set_count, emf_uid_range_set **uid_range_set, int range_len, int *err_code);

/**
 * void em_core_free_uid_range_set(emf_uid_range_set **uid_range_head)
 * Frees the linked list of uid ranges 
 *
 * @author 					h.gahlaut@samsung.com
 * @param[in] uid_range_head	Head pointer of linked list of uid ranges		
 * @remarks 									
 * @return This function does not return anything.
 */
EXPORT_API void em_core_free_uid_range_set(emf_uid_range_set **uid_range_set);

/**
 * @fn em_core_append_subset_string_to_uid_range(char *subset_string, emf_uid_range_set **uid_range_set, int range_len, unsigned long luid, unsigned long huid)
 * Appends the subset_string to uid range if the uid range has not exceeded maximum length(range_len), otherwise creates a new node in linked list of uid range set 
 * and stores the subset_string in its uid_range. Also sets the lowest and highest uids for the corresponsing uid_range
 * 
 * @author 					h.gahlaut@samsung.com
 * @param[in] subset_string		Specifies the subset string to be appended. A subset string can be like X : Y or X where X and Y are uids.
 * @param[in] range_len		Specifies the maximum length of range string allowed. 
 * @param[in] luid				Specifies the lowest uid in subset string
 * @param[in] huid				Specifies the highest uid in subset string
 * @param[out] uid_range_set	Returns the uid_ranges formed in the form of a linked list with head stored in uid_range_set pointer
 * @param[out] err_code		Returns the error code.
 * @remarks 										
 * @return This function returns true on success or false on failure.
 */
 
int em_core_append_subset_string_to_uid_range(char *subset_string, emf_uid_range_set **current_node_adr, emf_uid_range_set **uid_range_set, int range_len, unsigned long luid, unsigned long huid);

/**
 * @fn em_core_form_comma_separated_strings(int numbers[], int num_count, int max_string_len, char ***strings, int *string_count, int *err_code)
 * Forms comma separated strings of a give max_string_len from an array of numbers 
 * 
 * @author 					h.gahlaut@samsung.com
 * @param[in] numbers			Specifies the array of numbers to be converted into comma separated strings.
 * @param[in] num_count		Specifies the count of numbers in numbers array. 
 * @param[in] max_string_len	Specifies the maximum length of comma separated strings that are to be formed.
 * @param[out] strings			Returns the base address of a double dimension array which stores the strings.
 * @param[out] string_count		Returns the number of strings formed.
 * @param[out] err_code		Returns the error code.
 * @remarks 					If Input to the function is five numbers like 2755 2754 2748 2749 2750 and a given max_string_len is 20.
 *							Then this function will form two comma separated strings as follows -
 *							"2755, 2754, 2748"
 *							"2749, 2750"
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int em_core_form_comma_separated_strings(int numbers[], int num_count, int max_string_len, char ***strings, int *string_count, int *err_code);

/**
 * @fn em_core_free_comma_separated_strings(char ***string_list, int *string_count)
 * Frees the double dimensional array of strings. 
 *
 * @author 					h.gahlaut@samsung.com
 * @param[in] uid_range_head	Address of base address of double dimensional array of strings.
 * @param[in] string_count		Address of variable holding the count of strings.
 * @remarks 									
 * @return This function does not return anything.
 */

EXPORT_API void em_core_free_comma_separated_strings(char ***string_list, int *string_count);

#endif

#ifdef __LOCAL_ACTIVITY__
/*  Added to get next activity id sequence */
EXPORT_API int em_core_get_next_activity_id(int *activity_id, int *err_code);
EXPORT_API int em_core_activity_add(emf_activity_tbl_t *new_activity, int *err_code);
EXPORT_API int em_core_activity_delete(emf_activity_tbl_t *activity, int *err_code);
#endif /* __LOCAL_ACTIVITY__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __EM_CORE_UTILS_H__ */
