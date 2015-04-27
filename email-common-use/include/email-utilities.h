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

/*
 * email-utilities.h
 *
 *  Created on: 2012. 3. 6.
 *      Author: kyuho.jo@samsung.com
 */

#ifndef __EMAIL_UTILITIES_H__
#define __EMAIL_UTILITIES_H__

#include "email-types.h"
#include "email-internal-types.h"
#include "email-debug-log.h"
#include "email-storage.h"

INTERNAL_FUNC void* em_malloc(int len);
INTERNAL_FUNC void* em_memdup(void* src, int len);
INTERNAL_FUNC char* em_trim_left(char *str);
INTERNAL_FUNC char* em_trim_right(char *str);
INTERNAL_FUNC char* em_upper_string(char *str);
INTERNAL_FUNC char* em_lower_string(char *str);
INTERNAL_FUNC int   em_upper_path(char *path);

INTERNAL_FUNC void  em_skip_whitespace(char *addr_str , char **pAddr);
INTERNAL_FUNC char* em_skip_whitespace_without_strdup(char *source_string);
INTERNAL_FUNC void  em_skip_whitespace_without_alias(char *addr_str, char **pAddr);
INTERNAL_FUNC char* em_replace_all_string(char *source_string, char *old_string, char *new_string);
INTERNAL_FUNC char* em_replace_string(char *source_string, char *old_string, char *new_string);
INTERNAL_FUNC int   em_replace_string_ex(char **input_source_string, char *input_old_string, char *input_new_string);
INTERNAL_FUNC void  em_flush_memory();
INTERNAL_FUNC int   em_get_file_name_from_file_path(char *input_source_file_path, char **output_file_name);
INTERNAL_FUNC int   em_get_file_name_and_extension_from_file_path(char *input_source_file_path, char **output_file_name, char **output_extention);
INTERNAL_FUNC char* em_get_extension_from_file_path(char *source_file_path, int *err_code);
INTERNAL_FUNC int   em_get_encoding_type_from_file_path(const char *input_file_path, char **output_encoding_type);
INTERNAL_FUNC int   em_get_content_type_from_extension_string(const char *extension_string, int *err_code);
INTERNAL_FUNC char *em_shrink_filename(char *fname, int size_limit);

INTERNAL_FUNC int   em_verify_email_address(char *address);
INTERNAL_FUNC int   em_verify_email_address_of_mail_data(email_mail_data_t *mail_data);
INTERNAL_FUNC int   em_verify_email_address_of_mail_tbl(emstorage_mail_tbl_t *input_mail_tbl);

INTERNAL_FUNC int   em_find_pos_stripped_subject_for_thread_view(char *subject, char *stripped_subject, int stripped_subject_buffer_size);
INTERNAL_FUNC int   em_find_tag_for_thread_view(char *subject, int *result);

INTERNAL_FUNC int   em_encode_base64(char *src, unsigned long src_len, char **enc, unsigned long* enc_len, int *err_code);
INTERNAL_FUNC int   em_decode_base64(unsigned char *enc_text, unsigned long enc_len, char **dec_text, unsigned long* dec_len, int *err_code);

extern        char* strcasestr(__const char *__haystack, __const char *__needle) __THROW __attribute_pure__ __nonnull ((1, 2));

INTERNAL_FUNC int   em_get_account_server_type_by_account_id(char *multi_user_name, int account_id, email_account_server_t* account_server_type, int flag, int *error);

INTERNAL_FUNC int   em_get_handle_for_activesync(int *handle, int *error);

/* thread handle definition */
typedef struct {
	GQueue *q;
	pthread_mutex_t mu;
	pthread_cond_t cond;
	int running;
	pthread_t tid;
	void *(*thread_exit)(void*);
	void *thread_exit_arg;
} email_thread_handle_t;

INTERNAL_FUNC email_thread_handle_t* em_thread_create(void *(*thread_exit)(void*), void *arg);
INTERNAL_FUNC void em_thread_destroy (email_thread_handle_t* th);
INTERNAL_FUNC void em_thread_run (email_thread_handle_t *th, void *(*thread_func)(void*), void *(*destroy)(void*), void* arg);
INTERNAL_FUNC void em_thread_join (email_thread_handle_t *th);
INTERNAL_FUNC int  em_fopen(const char *filename, const char *mode, FILE **fp);
INTERNAL_FUNC int  em_open(const char *filename, int oflags, mode_t mode, int *handle);

#endif /* __EMAIL_UTILITIES_H__ */
