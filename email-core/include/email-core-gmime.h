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

#ifndef __EMAIL_CORE_GMIME_H__
#define __EMAIL_CORE_GMIME_H__

#include "email-core-mail.h"
#include <gmime/gmime.h>

#include "c-client.h"
#include "lnx_inc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct _search_section {
	GMimeObject *section_object;
	char *section;
} search_section;

INTERNAL_FUNC void emcore_gmime_init(void);
INTERNAL_FUNC void emcore_gmime_shutdown(void);

INTERNAL_FUNC int emcore_gmime_pop3_parse_mime(char *eml_path, struct _m_content_info *cnt_info, int *err_code);
INTERNAL_FUNC int emcore_gmime_imap_parse_mime_partial(char *rfc822header_str, char *bodytext_str, struct _m_content_info *cnt_info);
INTERNAL_FUNC int emcore_gmime_eml_parse_mime(char *eml_path, struct _rfc822header *rfc822_header, struct _m_content_info *cnt_info, int *err_code);

INTERNAL_FUNC void emcore_gmime_imap_parse_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data);
INTERNAL_FUNC void emcore_gmime_imap_parse_full_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data);
INTERNAL_FUNC void emcore_gmime_imap_parse_bodystructure_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data);

INTERNAL_FUNC void emcore_gmime_get_body_sections_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data);
INTERNAL_FUNC void emcore_gmime_get_attachment_section_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data);
INTERNAL_FUNC void emcore_gmime_search_section_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data);

INTERNAL_FUNC void emcore_gmime_construct_multipart (GMimeMultipart *multipart, BODY *body, const char *spec, int *total_mail_size);
INTERNAL_FUNC void emcore_gmime_construct_part (GMimePart *part, BODY *body, const char *spec, int *total_mail_size);
INTERNAL_FUNC int emcore_gmime_construct_mime_part_with_bodystructure(BODY *mbody, GMimeMessage **message, const char *spec, int *total_mail_size);

INTERNAL_FUNC int emcore_gmime_get_body_sections_from_message(GMimeMessage *message, struct _m_content_info *cnt_info, char **sections_to_fetch);
INTERNAL_FUNC int emcore_gmime_get_attachment_section_from_message(GMimeMessage *message,
		struct _m_content_info *cnt_info, int nth, char **section_to_fetch);

INTERNAL_FUNC int emcore_gmime_fetch_imap_body_sections(MAILSTREAM *stream, int msg_uid, int mail_id,
		struct _m_content_info *cnt_info, GMimeMessage *message, int event_handle, int auto_download, int *err_code);

INTERNAL_FUNC int emcore_gmime_fetch_imap_attachment_section(MAILSTREAM *stream,
		int mail_id, int uid, int nth, struct _m_content_info *cnt_info,
		GMimeMessage *message, int auto_download, int event_handle, int *err_code);

INTERNAL_FUNC int emcore_gmime_check_filename_duplication(char *source_filename, struct _m_content_info *cnt_info);
INTERNAL_FUNC char *emcore_gmime_get_modified_filename_in_duplication(char *source_filename);
INTERNAL_FUNC char *emcore_gmime_get_encoding_to_utf8(const char *text);
INTERNAL_FUNC char *emcore_gmime_get_decoding_text(const char *text);
INTERNAL_FUNC void emcore_gmime_get_mime_entity_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data);
INTERNAL_FUNC char *emcore_gmime_get_mime_entity_signed_message(GMimeObject *multipart);
INTERNAL_FUNC char *emcore_gmime_get_mime_entity(int fd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* EOF */
