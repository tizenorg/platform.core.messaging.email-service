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
 * File :  em-core-mime.h
 * Desc :  MIME Operation Header
 *
 * Auth : 
 *
 * History : 
 *    2011.04.14  :  created
 *****************************************************************************/
#ifndef __EM_CORE_MIME_H__
#define __EM_CORE_MIME_H__

#include "em-core-types.h"
#include "c-client.h"
#include "em-storage.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

char *em_core_mime_get_line_from_file(void *stream, char *buf, int size, int *err_code);
int em_core_mime_parse_mime(void *stream, int is_file, struct _m_content_info *cnt_info, int *err_code);
EXPORT_API int   em_core_get_attribute_value_of_body_part(PARAMETER *input_param, char *atribute_name, char *output_value, int output_buffer_length, int with_rfc2047_text, int *err_code);
EXPORT_API int   em_core_get_body_part_list_full(MAILSTREAM *stream, int msg_uid, int account_id, int mail_id, BODY *body, struct _m_content_info *cnt_info, int *err_code, PARTLIST * section_list, int event_handle);
EXPORT_API int   em_core_get_body(MAILSTREAM *stream, int account_id, int mail_id, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code);
EXPORT_API int   em_core_get_body_structure(MAILSTREAM *stream, int msg_uid, BODY **body, int *err_code);
EXPORT_API char *em_core_decode_rfc2047_text(char *rfc2047_text, int *err_code);
EXPORT_API int   em_core_decode_body_text(char *enc_buf, int enc_len, int enc_type, int *dec_len, int *err_code);
EXPORT_API int   em_core_set_fetch_body_section(BODY *body, int enable_inline_list, int *total_mail_size, int *err_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* EOF */
