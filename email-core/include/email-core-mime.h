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
 * File :  email-core-mime.h
 * Desc :  MIME Operation Header
 *
 * Auth : 
 *
 * History : 
 *    2011.04.14  :  created
 *****************************************************************************/
#ifndef __EMAIL_CORE_MIME_H__
#define __EMAIL_CORE_MIME_H__

#include "email-internal-types.h"
#include "c-client.h"
#include "email-storage.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

char *emcore_get_line_from_file(void *stream, char *buf, int size, int *err_code);
int   emcore_parse_mime(void *stream, int is_file, struct _m_content_info *cnt_info, int *err_code);
INTERNAL_FUNC int   emcore_get_content_type_from_mime_string(char *input_mime_string, char **output_content_type);
INTERNAL_FUNC int   emcore_get_content_type_from_mail_bodystruct(BODY *input_body, int input_buffer_length, char *output_content_type);
INTERNAL_FUNC int   emcore_get_attribute_value_of_body_part(PARAMETER *input_param, char *atribute_name, char *output_value, int output_buffer_length, int with_rfc2047_text, int *err_code);
INTERNAL_FUNC int   emcore_get_body_part_list_full(MAILSTREAM *stream, int msg_uid, int account_id, int mail_id, BODY *body, struct _m_content_info *cnt_info, int *err_code, PARTLIST * section_list, int event_handle);
INTERNAL_FUNC int   emcore_get_body(MAILSTREAM *stream, int account_id, int mail_id, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code);
INTERNAL_FUNC int   emcore_get_body_structure(MAILSTREAM *stream, int msg_uid, BODY **body, int *err_code);
INTERNAL_FUNC char *emcore_decode_rfc2047_text(char *rfc2047_text, int *err_code);
INTERNAL_FUNC int   emcore_decode_body_text(char *enc_buf, int enc_len, int enc_type, int *dec_len, int *err_code);
INTERNAL_FUNC int   emcore_set_fetch_body_section(BODY *body, int enable_inline_list, int *total_mail_size, int *err_code);
INTERNAL_FUNC int   emcore_parse_mime_file_to_mail(char *eml_file_path, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data, int *output_attachment_count, int *err_code);
INTERNAL_FUNC int   emcore_delete_parsed_data(email_mail_data_t *input_mail_data, int *err_code);
INTERNAL_FUNC int   emcore_get_mime_entity(char *mime_path, char **mime_entity, int *err_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* EOF */
