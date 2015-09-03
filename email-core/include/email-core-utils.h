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
 * File :  email-core-utils.h
 * Desc :  Mail Utils Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#ifndef __EMAIL_CORE_UTILS_H__
#define __EMAIL_CORE_UTILS_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "email-types.h"
#include "email-internal-types.h"
#include "email-storage.h"
#include "email-core-global.h"
#include "email-core-mail.h"

/*  This is used for emcore_get_long_encoded_path */
#define EMAIL_CONNECT_FOR_SENDING	-1
#define	ENCODED_PATH_SMTP	"UHDF_ENCODED_PATH_SMTP_EKJD"

typedef int (*email_get_unread_email_count_cb)(int unread, int *err_code);

/* parse the Full mailbox Path and get the Alias Name of the Mailbox */
char* emcore_get_alias_of_mailbox(const char *mailbox_path);

/* Parse the Mailbox Path and get the Account Email address */
INTERNAL_FUNC int   emcore_get_temp_file_name(char **filename, int *err_code);
int   emcore_get_long_encoded_path(char *multi_user_name, int account_id, char *path, int delimiter, char **long_enc_path, int *err_code);
int   emcore_get_encoded_mailbox_name(char *name, char **enc_name, int *err_code);
int   emcore_get_file_name(char *path, char **filename, int *err_code);
INTERNAL_FUNC int   emcore_get_file_size(char *path, int *size, int *err_code);
int   emcore_get_actual_mail_size(char *pBodyPlane, char *pBodyHtml, struct attachment_info *pAttachment, int *error_code);
int   emcore_calc_mail_size(char *multi_user_name, email_mail_data_t *mail_data_src, email_attachment_data_t *attachment_data_src, int attachment_count, int *error_code);
int   emcore_get_address_count(char *addr_str, int *to_num, int *err_code);
INTERNAL_FUNC int   emcore_is_storage_full();
int   emcore_get_long_encoded_path_with_account_info(char *multi_user_name, email_account_t *account, char *path, int delimiter, char **long_enc_path, int *err_code);
void  emcore_fill_address_information_of_mail_tbl(char *multi_user_name, emstorage_mail_tbl_t *mail_data);

INTERNAL_FUNC char* emcore_get_mail_field_name_by_attribute_type(email_mail_attribute_type input_attribute_type);
INTERNAL_FUNC int emcore_get_attribute_type_by_mail_field_name(char *input_mail_field_name, email_mail_attribute_type *output_mail_attribute_type);
INTERNAL_FUNC int emcore_get_mail_attribute_value_type(email_mail_attribute_type input_attribute_type, email_mail_attribute_value_type *output_value_type);
INTERNAL_FUNC int emcore_get_preview_text_from_file(char *multi_user_name, const char *input_plain_path, const char *input_html_path, int input_preview_buffer_length, char **output_preview_buffer);
#ifdef __FEATURE_BODY_SEARCH__
INTERNAL_FUNC int emcore_strip_mail_body_from_file(char *multi_user_name, emstorage_mail_tbl_t *mail, char **stripped_text, int *err_code);
#endif
INTERNAL_FUNC int emcore_get_first_address(const char *full_address, char **alias, char **address);


INTERNAL_FUNC int reg_replace (char *input_source_text, char *input_old_pattern_string, char *input_new_string);
INTERNAL_FUNC char *reg_replace_new (char *input_source_text, char *input_old_pattern_string, char *input_new_string);
int   emcore_strip_HTML_tag(const char *input_html_file_path, char *encoding_type, char *output_result_buffer, int input_result_buffer_legnth);
int   emcore_send_noti_for_new_mail(int account_id, char *mailbox_name, char *subject, char *from, char *uid, char *datetime);
int   emcore_make_attachment_file_name_with_extension(char *source_file_name, char *sub_type, char *result_file_name, int result_file_name_buffer_length, int *err_code);

/* Session Handling */
INTERNAL_FUNC int   emcore_get_empty_session(email_session_t **session);
INTERNAL_FUNC int   emcore_clear_session(email_session_t *session);
INTERNAL_FUNC int   emcore_get_current_session(email_session_t **session);

INTERNAL_FUNC int emcore_get_mail_count_by_query(char *multi_user_name, int account_id, int mailbox_type, int priority_sender, int *total_mail, int *unread_mail, int *err_code);

INTERNAL_FUNC int emcore_check_drm_file(char *path, int *err_code);
INTERNAL_FUNC int emcore_check_drm_is_ringtone(char *ringtone_path, int *err_code);

INTERNAL_FUNC void emcore_display_unread_in_badge(void *data);
INTERNAL_FUNC int emcore_display_badge_count(char *multi_user_name, int count);
INTERNAL_FUNC int emcore_set_network_error(int err_code);

INTERNAL_FUNC int emcore_calc_next_time_to_sync(char *multi_user_name, int input_account_id, time_t input_current_time, time_t *output_time);

/* Transaction Handling */
INTERNAL_FUNC int emcore_add_transaction_info(int mail_id , int handle  , int *err_code);
INTERNAL_FUNC int emcore_get_handle_by_mailId_from_transaction_info(int mail_id , int *pHandle);
INTERNAL_FUNC int emcore_delete_transaction_info_by_mailId(int mail_id);

/* For notification bar */
INTERNAL_FUNC int emcore_update_notification_for_unread_mail(int account_id);
INTERNAL_FUNC int emcore_clear_notifications(char *multi_user_name, int account_id);
//INTERNAL_FUNC int emcore_add_notification_for_unread_mail(emstorage_mail_tbl_t *input_mail_tbl_data);
INTERNAL_FUNC int emcore_add_notification(char *multi_user_name, int account_id, int mail_id, int unread_mail_count, int vip_unread_mail_count, int input_play_alert_tone, int sending_error, unsigned long display);
INTERNAL_FUNC int emcore_add_notification_for_send(char *multi_user_name, int account_id, int mail_id, email_action_t action, int sending_error, unsigned long display);
INTERNAL_FUNC void emcore_update_notification_for_send(int account_id, int mail_id, double progress);
INTERNAL_FUNC int emcore_delete_notification_for_read_mail(int mail_id);
INTERNAL_FUNC int emcore_delete_notification_by_account(char *multi_user_name, int account_id, int with_noti_tray);

INTERNAL_FUNC int emcore_show_user_message(char *multi_user_name, int id, email_action_t action, int error);

INTERNAL_FUNC int emcore_connect_contacts_service(char *multi_user_name);
INTERNAL_FUNC int emcore_disconnect_contacts_service(char *multi_user_name);

#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__

/**
 * @fn emcore_convert_to_uid_range_set(email_id_set_t *id_set, int id_set_count, email_uid_range_set **uid_range_set, int range_len, int *err_code)
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
 
INTERNAL_FUNC int emcore_convert_to_uid_range_set(email_id_set_t *id_set, int id_set_count, email_uid_range_set **uid_range_set, int range_len, int *err_code);

/**
 * void emcore_free_uid_range_set(email_uid_range_set **uid_range_head)
 * Frees the linked list of uid ranges 
 *
 * @author 					h.gahlaut@samsung.com
 * @param[in] uid_range_head	Head pointer of linked list of uid ranges		
 * @remarks 									
 * @return This function does not return anything.
 */
INTERNAL_FUNC void emcore_free_uid_range_set(email_uid_range_set **uid_range_set);

/**
 * @fn emcore_append_subset_string_to_uid_range(char *subset_string, email_uid_range_set **uid_range_set, int range_len, unsigned long luid, unsigned long huid)
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
 
int emcore_append_subset_string_to_uid_range(char *subset_string, email_uid_range_set **current_node_adr, email_uid_range_set **uid_range_set, int range_len, unsigned long luid, unsigned long huid);

/**
 * @fn emcore_form_comma_separated_strings(int numbers[], int num_count, int max_string_len, char ***strings, int *string_count, int *err_code)
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
INTERNAL_FUNC int emcore_form_comma_separated_strings(int numbers[], int num_count, int max_string_len, char ***strings, int *string_count, int *err_code);

/**
 * @fn emcore_free_comma_separated_strings(char ***string_list, int *string_count)
 * Frees the double dimensional array of strings. 
 *
 * @author 					h.gahlaut@samsung.com
 * @param[in] uid_range_head	Address of base address of double dimensional array of strings.
 * @param[in] string_count		Address of variable holding the count of strings.
 * @remarks 									
 * @return This function does not return anything.
 */

INTERNAL_FUNC void emcore_free_comma_separated_strings(char ***string_list, int *string_count);

#endif

#ifdef __FEATURE_LOCAL_ACTIVITY__
/*  Added to get next activity id sequence */
INTERNAL_FUNC int emcore_get_next_activity_id(int *activity_id, int *err_code);
INTERNAL_FUNC int emcore_add_activity(emstorage_activity_tbl_t *new_activity, int *err_code);
INTERNAL_FUNC int emcore_delete_activity(emstorage_activity_tbl_t *activity, int *err_code);
#endif /* __FEATURE_LOCAL_ACTIVITY__ */

INTERNAL_FUNC void emcore_free_rule(email_rule_t* rule);
INTERNAL_FUNC void emcore_free_body_sparep(void **p);

INTERNAL_FUNC int emcore_search_string_from_file(char *file_path, char *search_string, char *new_string, int *result);

INTERNAL_FUNC int emcore_load_query_from_file(char *file_path, char ***query_array, int *array_len);

#ifdef __FEATURE_DRIVING_MODE__
INTERNAL_FUNC int emcore_start_driving_mode(char *multi_user_name, int mail_id);
#endif /* __FEATURE_DRIVING_MODE__ */

#ifdef __FEATURE_BLOCKING_MODE__
INTERNAL_FUNC bool emcore_init_blocking_mode_status();
INTERNAL_FUNC void emcore_set_blocking_mode_of_setting(int input_blocking_mode_of_setting);
INTERNAL_FUNC int emcore_get_blocking_mode_status();
INTERNAL_FUNC void emcore_set_blocking_mode_status(int blocking_mode);
INTERNAL_FUNC int emcore_check_blocking_mode(char *multi_user_name, char *sender_address, int *blocking_status);
INTERNAL_FUNC int emcore_check_blocking_mode_internal (char *multi_user_name, char *sender_address, int *blocking_status);
#endif /* __FEATURE_BLOCKING_MODE__ */

INTERNAL_FUNC char *emcore_set_mime_entity(char *mime_path);
INTERNAL_FUNC int emcore_get_content_from_file(char *filename, char **contents, int *length);
INTERNAL_FUNC int emcore_set_content_to_file(const char *contents, char *dest_path, int length);

#ifdef __FEATURE_UPDATE_DB_TABLE_SCHEMA__
INTERNAL_FUNC int emcore_update_db_table_schema(char *multi_user_name);
#endif /* #ifdef __FEATURE_UPDATE_DB_TABLE_SCHEMA__ */
INTERNAL_FUNC int emcore_unescape_from_url(char *input_url, char **output_url);
INTERNAL_FUNC char *__em_get_month_in_string(int month);
INTERNAL_FUNC int emcore_make_date_string_for_search(time_t input_time, char *output_date_string);
INTERNAL_FUNC int emcore_make_uid_range_string(emcore_uid_list *uid_list, int total, char **output_uid_range_string);

INTERNAL_FUNC char* emcore_convert_mutf7_to_utf8(char *mailbox_name); 
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __EMAIL_CORE_UTILS_H__ */
