/*
*  email-service
*
* Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Minsoo Kim <minnsoo.kim@samsung.com>, Kyuho Jo <kyuho.jo@samsung.com>,
* Sunghyun Kwon <sh0701.kwon@samsung.com>
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

#include <glib.h>
#include <gmime/gmime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vconf.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "email-internal-types.h"
#include "email-utilities.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-core-mail.h"
#include "email-core-mime.h"
#include "email-core-gmime.h"
#include "email-storage.h"
#include "email-core-event.h"
#include "email-core-account.h"
#include "email-core-signal.h"
#include "email-core-mailbox-sync.h"
#include "email-debug-log.h"

static int multipart_status = 0;

static void emcore_gmime_pop3_parse_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data);
static void emcore_gmime_eml_parse_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data);
static int emcore_gmime_parse_mime_header(GMimeMessage *message, struct _rfc822header *rfc822_header);


INTERNAL_FUNC void emcore_gmime_init(void)
{
	EM_DEBUG_FUNC_BEGIN();

#if !GLIB_CHECK_VERSION(2, 31, 0)
	g_thread_init(NULL);
#endif
	g_mime_init(0);

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void emcore_gmime_shutdown(void)
{
	EM_DEBUG_FUNC_BEGIN();

	g_mime_shutdown();

	EM_DEBUG_FUNC_END();
}


INTERNAL_FUNC int emcore_gmime_imap_parse_mime_partial(char *rfc822header_str, char *bodytext_str, struct _m_content_info *cnt_info)
{
	EM_DEBUG_FUNC_BEGIN();

	EM_DEBUG_LOG_DEV("RFC822H:%s", rfc822header_str);
	EM_DEBUG_LOG_DEV("BODYTEXT:%s", bodytext_str);

	GMimeStream *stream = NULL;
	GMimeMessage *message = NULL;
	GMimeParser *parser = NULL;
	char *fulltext = NULL;

	fulltext = g_strconcat(rfc822header_str, "\r\n\r\n", bodytext_str, NULL);

	stream = g_mime_stream_mem_new_with_buffer(fulltext, EM_SAFE_STRLEN(fulltext));

	parser = g_mime_parser_new_with_stream(stream);
	if (stream) g_object_unref(stream);

	message = g_mime_parser_construct_message(parser);
	if (parser) g_object_unref(parser);
	if (!message) { /* prevent null check for message */
		EM_DEBUG_EXCEPTION("g_mime_parser_construct_message error");
		EM_SAFE_FREE(fulltext);
		return false;
	}

	EM_DEBUG_LOG_DEV("Sender:%s", g_mime_message_get_sender(message));
	EM_DEBUG_LOG_DEV("Reply To:%s", g_mime_message_get_reply_to(message));
	EM_DEBUG_LOG_DEV("Subject:%s", g_mime_message_get_subject(message));
	EM_DEBUG_LOG_DEV("Date:%s", g_mime_message_get_date_as_string(message));
	EM_DEBUG_LOG_DEV("Message ID:%s", g_mime_message_get_message_id(message));

	g_mime_message_foreach(message, emcore_gmime_imap_parse_foreach_cb, (gpointer)cnt_info);

	EM_SAFE_FREE(fulltext);

	if (message) g_object_unref (message);

	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emcore_gmime_pop3_parse_mime(char *eml_path, struct _m_content_info *cnt_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("cnt_info[%p], err_code[%p]", cnt_info, err_code);
	EM_DEBUG_LOG_SEC("eml_path[%s]", eml_path);

	GMimeStream *stream = NULL;
	GMimeMessage *message = NULL;
	GMimeParser *parser = NULL;
	int fd = 0;
	int err = EMAIL_ERROR_NONE;

	err = em_open(eml_path, O_RDONLY, 0, &fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("holder open failed :  holder is a filename that will be saved.");
		if (err_code)
			*err_code = err;
		return false; /*prevent 34936*/
	}

	stream = g_mime_stream_fs_new(fd);

	parser = g_mime_parser_new_with_stream(stream);
	if (stream) g_object_unref(stream);

	message = g_mime_parser_construct_message(parser);
	if (parser) g_object_unref(parser);
	if (!message) { /* prevent null check for message */
		EM_DEBUG_EXCEPTION("g_mime_parser_construct_message error");
		return false;
	}

	EM_DEBUG_LOG_DEV("Sender:%s", g_mime_message_get_sender(message));
	EM_DEBUG_LOG_DEV("Reply To:%s", g_mime_message_get_reply_to(message));
	EM_DEBUG_LOG_DEV("Subject:%s", g_mime_message_get_subject(message));
	EM_DEBUG_LOG_DEV("Date:%s", g_mime_message_get_date_as_string(message));
	EM_DEBUG_LOG_DEV("Message ID:%s", g_mime_message_get_message_id(message));

	if (g_strrstr(g_mime_message_get_sender(message), "mmsc.plusnet.pl") != NULL ||
		g_strrstr(g_mime_message_get_sender(message), "mms.t-mobile.pl") != NULL) {
		cnt_info->attachment_only = 1;
	}

	g_mime_message_foreach(message, emcore_gmime_pop3_parse_foreach_cb, (gpointer)cnt_info);

	EM_SAFE_CLOSE (fd);

	if (message) g_object_unref (message);

	EM_DEBUG_FUNC_END();
	return true;
}


INTERNAL_FUNC int emcore_gmime_eml_parse_mime(char *eml_path, struct _rfc822header *rfc822_header, struct _m_content_info *cnt_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("cnt_info[%p], err_code[%p]", cnt_info, err_code);
	EM_DEBUG_LOG_SEC("eml_path[%s]", eml_path);

	GMimeStream *stream = NULL;
	GMimeMessage *message = NULL;
	GMimeParser *parser = NULL;
	int fd = 0;
	int err = EMAIL_ERROR_NONE;

	err = em_open(eml_path, O_RDONLY, 0, &fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("holder open failed :  holder is a filename that will be saved.");
		if (err_code)
			*err_code = err;
		return false; /*prevent 34936*/
	}

	multipart_status = 0;

	stream = g_mime_stream_fs_new(fd);

	parser = g_mime_parser_new_with_stream(stream);
	if (stream) g_object_unref(stream);

	message = g_mime_parser_construct_message(parser);
	if (parser) g_object_unref(parser);
	if (!message) { /* prevent null check for message */
		EM_DEBUG_EXCEPTION("g_mime_parser_construct_message error");
		return false;
	}

	EM_DEBUG_LOG_DEV("Sender:%s", g_mime_message_get_sender(message));
	EM_DEBUG_LOG_DEV("Reply To:%s", g_mime_message_get_reply_to(message));
	EM_DEBUG_LOG_DEV("Subject:%s", g_mime_message_get_subject(message));
	EM_DEBUG_LOG_DEV("Date:%s", g_mime_message_get_date_as_string(message));
	EM_DEBUG_LOG_DEV("Message ID:%s", g_mime_message_get_message_id(message));

	emcore_gmime_parse_mime_header(message, rfc822_header);

	g_mime_message_foreach(message, emcore_gmime_eml_parse_foreach_cb, (gpointer)cnt_info);

	EM_SAFE_CLOSE (fd);

	if (message) g_object_unref (message);

	EM_DEBUG_FUNC_END();
	return true;
}


static void emcore_gmime_pop3_parse_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data)
{
	EM_DEBUG_FUNC_BEGIN("parent[%p], part[%p], user_data[%p]", parent, part, user_data);

	int error = EMAIL_ERROR_NONE;
	char *msg_tmp_content_path = NULL;
	char *content_path = NULL;
	struct _m_content_info *cnt_info = (struct _m_content_info *)user_data;

	if (GMIME_IS_MESSAGE_PART(part)) {

		EM_DEBUG_LOG("Message Part");
		GMimeMessage *message = NULL;
		GMimeContentType *msg_ctype = NULL;
		GMimeContentDisposition *msg_disposition = NULL;
		GMimeStream *out_stream;
		char *msg_ctype_type = NULL;
		char *msg_ctype_subtype = NULL;
		char *msg_ctype_name = NULL;
		char *msg_disposition_str = NULL;
		char *msg_disposition_filename = NULL;
		char *msg_content_id = NULL;
		int real_size = 0;
		int msg_fd = 0;

		if (cnt_info->grab_type != (GRAB_TYPE_TEXT|GRAB_TYPE_ATTACHMENT) &&
				cnt_info->grab_type != GRAB_TYPE_ATTACHMENT) {
			goto FINISH_OFF;
		}

		message = g_mime_message_part_get_message((GMimeMessagePart *)part);
		if (!message) {
			EM_DEBUG_EXCEPTION("Message is NULL");
			goto FINISH_OFF;
		}

		/*Content Type*/
		msg_ctype = g_mime_object_get_content_type(part);
		msg_ctype_type = (char *)g_mime_content_type_get_media_type(msg_ctype);
		msg_ctype_subtype = (char *)g_mime_content_type_get_media_subtype(msg_ctype);
		msg_ctype_name = (char *)g_mime_content_type_get_parameter(msg_ctype, "name");
		EM_DEBUG_LOG("Content-Type[%s/%s]", msg_ctype_type, msg_ctype_subtype);
		EM_DEBUG_LOG_SEC("RFC822/Message Content-Type-Name[%s]", msg_ctype_name);
		/*Content Type - END*/

		/*Content Disposition*/
		msg_disposition = g_mime_object_get_content_disposition(part);
		if (msg_disposition) {
			msg_disposition_str = (char *)g_mime_content_disposition_get_disposition(msg_disposition);
			msg_disposition_filename = (char *)g_mime_content_disposition_get_parameter(msg_disposition, "filename");
			if (EM_SAFE_STRLEN(msg_disposition_filename) == 0)
				msg_disposition_filename = NULL;
		}
		EM_DEBUG_LOG("RFC822/Message Disposition[%s]", msg_disposition_str);
		EM_DEBUG_LOG_SEC("RFC822/Message Disposition-Filename[%s]", msg_disposition_filename);
		/*Content Disposition - END*/

		/*Content ID*/
		msg_content_id = (char *)g_mime_object_get_content_id(part);
		EM_DEBUG_LOG("RFC822/Message Content-ID:%s", msg_content_id);

		/*save message content to tmp file*/
		if (!emcore_get_temp_file_name(&msg_tmp_content_path, &error)) {
			EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed [%d]", error);
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG_SEC("Temporary Content Path[%s]", msg_tmp_content_path);

		msg_fd = open(msg_tmp_content_path, O_WRONLY|O_CREAT, 0644);
		if (msg_fd < 0) {
			EM_DEBUG_EXCEPTION("open failed");
			goto FINISH_OFF;
		}

		out_stream = g_mime_stream_fs_new(msg_fd);
		real_size = g_mime_object_write_to_stream(GMIME_OBJECT(message), out_stream);
		if (out_stream) g_object_unref(out_stream);

		if (real_size <= 0) {
			EM_DEBUG_EXCEPTION("g_mime_object_write_to_stream failed");
			goto FINISH_OFF;
		}

		if (msg_disposition_str && g_ascii_strcasecmp(msg_disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) {
			EM_DEBUG_LOG("RFC822/Message is ATTACHMENT");

			struct attachment_info *file = NULL;
			struct attachment_info *temp_file = cnt_info->file;
			char *utf8_text = NULL;
			int err = EMAIL_ERROR_NONE;

			file = em_malloc(sizeof(struct attachment_info));
			if (file == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed...");
				goto FINISH_OFF;
			}

			file->type = ATTACHMENT;
			if (msg_disposition_filename) file->name = g_strdup(msg_disposition_filename);
			else if (msg_ctype_name) file->name = g_strdup(msg_ctype_name);
			else if (msg_content_id) file->name = g_strdup(msg_content_id);
			else file->name = g_strdup("unknown");

			if (msg_content_id) file->content_id = g_strdup(msg_content_id);

			if (!(utf8_text = g_mime_utils_header_decode_text(file->name))) {
				EM_DEBUG_EXCEPTION("g_mime_utils_header_decode_text failed [%d]", err);
			}
			EM_DEBUG_LOG_SEC("utf8_text : [%s]", utf8_text);

			if (utf8_text) {
				EM_SAFE_FREE(file->name);
				file->name = EM_SAFE_STRDUP(utf8_text);
			}
			EM_SAFE_FREE(utf8_text);

			if(msg_ctype_type && msg_ctype_subtype) {
				char mime_type_buffer[128] = {0,};
				snprintf(mime_type_buffer, sizeof(mime_type_buffer), "%s/%s", msg_ctype_type, msg_ctype_subtype);
				file->attachment_mime_type = g_strdup(mime_type_buffer);
			}

			file->save = g_strdup(msg_tmp_content_path);
			file->size = real_size;
			file->save_status = 1;

			while (temp_file && temp_file->next)
				temp_file = temp_file->next;

			if (temp_file == NULL)
				cnt_info->file = file;
			else
				temp_file->next = file;
		}

		//g_mime_message_foreach(message, emcore_gmime_pop3_parse_foreach_cb, user_data);
	} else if (GMIME_IS_MESSAGE_PARTIAL(part)) {
		EM_DEBUG_LOG("Partial Part");
		//TODO
	} else if (GMIME_IS_MULTIPART(part)) {
		EM_DEBUG_LOG("Multi Part");
		GMimeMultipart *multi_part = NULL;
		multi_part = (GMimeMultipart *)part;

		int multi_count = g_mime_multipart_get_count(multi_part);
		EM_DEBUG_LOG("Multi Part Count:%d", multi_count);
		EM_DEBUG_LOG("Boundary:%s\n\n", g_mime_multipart_get_boundary(multi_part));

	} else if (GMIME_IS_PART(part)) {
		EM_DEBUG_LOG("Part");
		int content_size = 0;

		GMimePart *leaf_part = NULL;
		leaf_part = (GMimePart *)part;

		EM_DEBUG_LOG("Content ID:%s", g_mime_part_get_content_id(leaf_part));
		EM_DEBUG_LOG_SEC("Description:%s", g_mime_part_get_content_description(leaf_part));
		EM_DEBUG_LOG_SEC("MD5:%s", g_mime_part_get_content_md5(leaf_part));

		int content_disposition_type = 0;
		char *content_location = (char *)g_mime_part_get_content_location(leaf_part);
		EM_DEBUG_LOG_SEC("Location:%s", content_location);

		GMimeObject *mobject = (GMimeObject *)part;

		/*Content ID*/
		char *content_id = (char *)g_mime_object_get_content_id(mobject);

		/*Content Disposition*/
		GMimeContentDisposition *disposition = NULL;
		char *disposition_str = NULL;
		char *disposition_filename = NULL;
		disposition = g_mime_object_get_content_disposition(mobject);
		if (disposition) {
			disposition_str = (char *)g_mime_content_disposition_get_disposition(disposition);
			disposition_filename = (char *)g_mime_content_disposition_get_parameter(disposition, "filename");
			if (EM_SAFE_STRLEN(disposition_filename) == 0) disposition_filename = NULL;
		}
		EM_DEBUG_LOG("Disposition[%s]", disposition_str);
		EM_DEBUG_LOG_SEC("Disposition-Filename[%s]", disposition_filename);
		/*Content Disposition - END*/

		/*Content Type*/
		GMimeContentType *ctype = NULL;
		char *ctype_type = NULL;
		char *ctype_subtype = NULL;
		char *ctype_charset = NULL;
		char *ctype_name = NULL;

		ctype = g_mime_object_get_content_type(mobject);
		ctype_type = (char *)g_mime_content_type_get_media_type(ctype);
		ctype_subtype = (char *)g_mime_content_type_get_media_subtype(ctype);
		ctype_charset = (char *)g_mime_content_type_get_parameter(ctype, "charset");
		ctype_name = (char *)g_mime_content_type_get_parameter(ctype, "name");
		if (EM_SAFE_STRLEN(ctype_name) == 0) ctype_name = NULL;
		EM_DEBUG_LOG("Content-Type[%s/%s]", ctype_type, ctype_subtype);
		EM_DEBUG_LOG("Content-Type-Charset[%s]", ctype_charset);
		EM_DEBUG_LOG("Content-Type-Name[%s]", ctype_name);
		/*Content Type - END*/

		/*Content*/
		if (!emcore_get_temp_file_name(&content_path, &error)) {
			EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed [%d]", error);
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG_SEC("Temporary Content Path[%s]", content_path);


		GMimeContentEncoding enc = g_mime_part_get_content_encoding(leaf_part);
		switch(enc) {
		case GMIME_CONTENT_ENCODING_DEFAULT:
			EM_DEBUG_LOG("Encoding:ENCODING_DEFAULT");
			break;
		case GMIME_CONTENT_ENCODING_7BIT:
			EM_DEBUG_LOG("Encoding:ENCODING_7BIT");
			break;
		case GMIME_CONTENT_ENCODING_8BIT:
			EM_DEBUG_LOG("Encoding:ENCODING_8BIT");
			break;
		case GMIME_CONTENT_ENCODING_BINARY:
			EM_DEBUG_LOG("Encoding:ENCODING_BINARY");
			break;
		case GMIME_CONTENT_ENCODING_BASE64:
			EM_DEBUG_LOG("Encoding:ENCODING_BASE64");
			break;
		case GMIME_CONTENT_ENCODING_QUOTEDPRINTABLE:
			EM_DEBUG_LOG("Encoding:ENCODING_QUOTEDPRINTABLE");
			break;
		case GMIME_CONTENT_ENCODING_UUENCODE:
			EM_DEBUG_LOG("Encoding:ENCODING_UUENCODE");
			break;

		default:
			break;
		}

		GMimeDataWrapper *data = g_mime_part_get_content_object(leaf_part);
		if (data) {
			int fd;
			fd = open(content_path, O_WRONLY|O_CREAT, 0644);
			if (fd < 0) {
				EM_DEBUG_EXCEPTION("open failed");
				goto FINISH_OFF;
			}

			GMimeStream *out_stream;
			out_stream = g_mime_stream_fs_new(fd);

			//g_mime_data_wrapper_set_stream(data, out_stream);
			//g_mime_data_wrapper_set_encoding(data, enc);
			g_mime_data_wrapper_write_to_stream(data, out_stream);
			if (out_stream) g_object_unref(out_stream);
			emcore_get_file_size(content_path, &content_size, NULL);
		}
		/*Content - END*/

		/*Figure out TEXT or ATTACHMENT(INLINE) ?*/
		int result = false;
		if (disposition_str && g_ascii_strcasecmp(disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) {
			content_disposition_type = ATTACHMENT;
			EM_DEBUG_LOG("ATTACHMENT");
		} else if ((content_id || content_location) && (ctype_name || disposition_filename)) {
			if (cnt_info->attachment_only) {
				content_disposition_type = ATTACHMENT;
				EM_DEBUG_LOG("ATTACHMENT");
			} else {
				content_disposition_type = INLINE_ATTACHMENT;
				EM_DEBUG_LOG("INLINE_ATTACHMENT");
			}
		} else {
			if (content_id &&
					(emcore_search_string_from_file(cnt_info->text.html, content_id, NULL, &result) == EMAIL_ERROR_NONE && result)) {
				content_disposition_type = INLINE_ATTACHMENT;
				EM_DEBUG_LOG("INLINE_ATTACHMENT");
			} else if (content_id || content_location) {
				if (g_ascii_strcasecmp(ctype_type, "text") == 0 &&
						(g_ascii_strcasecmp(ctype_subtype, "plain") == 0 || g_ascii_strcasecmp(ctype_subtype, "html") == 0)) {
					EM_DEBUG_LOG("TEXT");
				} else {
					if (cnt_info->attachment_only) {
						content_disposition_type = ATTACHMENT;
						EM_DEBUG_LOG("ATTACHMENT");
					} else {
						content_disposition_type = INLINE_ATTACHMENT;
						EM_DEBUG_LOG("INLINE_ATTACHMENT");
					}
				}
			} else {
				if (g_ascii_strcasecmp(ctype_type, "text") == 0 &&
						(g_ascii_strcasecmp(ctype_subtype, "plain") == 0 || g_ascii_strcasecmp(ctype_subtype, "html") == 0)) {
					EM_DEBUG_LOG("TEXT");
				} else {
					content_disposition_type = ATTACHMENT;
					EM_DEBUG_LOG("ATTACHMENT");
				}
			}
		}

		if (content_disposition_type != ATTACHMENT && content_disposition_type != INLINE_ATTACHMENT) {
			/*TEXT*/
			EM_DEBUG_LOG("TEXT");
			if (ctype_type && g_ascii_strcasecmp(ctype_type, "text") == 0) {
				if (!ctype_charset || g_ascii_strcasecmp(ctype_charset, "X-UNKNOWN") == 0) {
					ctype_charset = "UTF-8";
				}

				if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, "plain") == 0) {
					EM_DEBUG_LOG("TEXT/PLAIN");

					char *file_content = NULL;
					int content_size = 0;

					if (emcore_get_content_from_file(content_path, &file_content, &content_size) != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("emcore_get_content_from_file failed");
					}

					if (file_content && content_size > 0) {
						char escape = 0x1b;
						char detector[25] = {0,};
						snprintf(detector, sizeof(detector), "%c$B", escape);
						if (g_strrstr(ctype_charset, "UTF-8") && g_strrstr(file_content, detector)) {
							ctype_charset = "ISO-2022-JP";
						}
					}

					EM_SAFE_FREE(file_content);

					cnt_info->text.plain_charset = g_strdup(ctype_charset);
					cnt_info->text.plain = g_strdup(content_path);
				} else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, "html") == 0) {
					EM_DEBUG_LOG("TEXT/HTML");
					cnt_info->text.html_charset = g_strdup(ctype_charset);
					cnt_info->text.html = g_strdup(content_path);
				}
			}
		} else {
			/*ATTACHMENT*/
			struct attachment_info *file = NULL;
			struct attachment_info *temp_file = NULL;
			char *utf8_text = NULL;
			int err = EMAIL_ERROR_NONE;

			if (content_disposition_type == ATTACHMENT) {
				EM_DEBUG_LOG("ATTACHMENT");
				temp_file = cnt_info->file;
			}
			else if (content_disposition_type == INLINE_ATTACHMENT) {
				EM_DEBUG_LOG("INLINE ATTACHMENT");
				temp_file = cnt_info->inline_file;
			}
			else {
				EM_DEBUG_EXCEPTION("Invalid content_disposition_type");
				goto FINISH_OFF;
			}

			file = em_malloc(sizeof(struct attachment_info));
			if (file == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed...");
				goto FINISH_OFF;
			}

			file->type = content_disposition_type;
			if (disposition_filename) file->name = g_strdup(disposition_filename);
			else if (ctype_name) file->name = g_strdup(ctype_name);
			else if (content_id) file->name = g_strdup(content_id);
			else file->name = g_strdup("unknown");
			file->content_id = g_strdup(content_id);

			if (!(utf8_text = g_mime_utils_header_decode_text(file->name))) {
				EM_DEBUG_EXCEPTION("g_mime_utils_header_decode_text failed [%d]", err);
			}
			EM_DEBUG_LOG_SEC("utf8_text : [%s]", utf8_text);

			if (utf8_text) {
				EM_SAFE_FREE(file->name);
				file->name = EM_SAFE_STRDUP(utf8_text);
			}
			EM_SAFE_FREE(utf8_text);

			/* check inline name duplication */
			if (content_disposition_type == INLINE_ATTACHMENT) {
				char *modified_name = NULL;
				if (emcore_gmime_check_filename_duplication(file->name, cnt_info)) {
					modified_name = emcore_gmime_get_modified_filename_in_duplication(file->name);
					EM_SAFE_FREE(file->name);
					file->name = modified_name;
					modified_name = NULL;
				}
				emcore_unescape_from_url(file->name, &modified_name);
				EM_DEBUG_LOG_SEC("file->name[%s] modified_name[%s]", file->name, modified_name);
				EM_SAFE_FREE(file->name);
				file->name = modified_name;
			}

			/*cid replacement for inline attachment*/
			if (content_disposition_type == INLINE_ATTACHMENT) {
				char *file_content = NULL;
				int content_size = 0;

				if (emcore_get_content_from_file(cnt_info->text.html, &file_content, &content_size) != EMAIL_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("emcore_get_content_from_file failed");
				}

				if (file_content && content_size > 0) {
					em_replace_string_ex(&file_content, "cid:", "");
					em_replace_string_ex(&file_content, file->content_id, file->name);

					content_size = EM_SAFE_STRLEN(file_content);
					if (emcore_set_content_to_file(file_content, cnt_info->text.html, content_size) != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("emcore_set_content_to_file failed");
					}
				}
				g_free(file_content); /* prevent 39110 */
			}

			if(ctype_type && ctype_subtype) {
				char mime_type_buffer[128] = {0,};
				snprintf(mime_type_buffer, sizeof(mime_type_buffer), "%s/%s", ctype_type, ctype_subtype);
				file->attachment_mime_type = g_strdup(mime_type_buffer);
			}

			file->save = g_strdup(content_path);
			file->size = content_size;

			if (ctype_type && g_ascii_strcasecmp(ctype_type, "APPLICATION") == 0) {
				if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, MIME_SUBTYPE_DRM_OBJECT) == 0)
					file->drm = EMAIL_ATTACHMENT_DRM_OBJECT;
				else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, MIME_SUBTYPE_DRM_RIGHTS) == 0)
					file->drm = EMAIL_ATTACHMENT_DRM_RIGHTS;
				else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, MIME_SUBTYPE_DRM_DCF) == 0)
					file->drm = EMAIL_ATTACHMENT_DRM_DCF;
			}

			while (temp_file && temp_file->next)
				temp_file = temp_file->next;

			if (temp_file == NULL) {
				if (content_disposition_type == ATTACHMENT) {
					cnt_info->file = file;
				} else {
					cnt_info->inline_file = file;
				}
			}
			else
				temp_file->next = file;
		}

		EM_SAFE_FREE(content_path);
	}

FINISH_OFF:

	EM_SAFE_FREE(msg_tmp_content_path);
	EM_SAFE_FREE(content_path);
	EM_DEBUG_FUNC_END();
}


static int emcore_gmime_parse_mime_header(GMimeMessage *message, struct _rfc822header *rfc822_header)
{
	EM_DEBUG_FUNC_BEGIN("message[%p], rfc822header[%p]", message, rfc822_header);

	int err = EMAIL_ERROR_NONE;

	if (!message || !rfc822_header) {
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	rfc822_header->reply_to     = EM_SAFE_STRDUP(g_mime_message_get_reply_to(message));
	EM_DEBUG_LOG_SEC("reply_to : [%s]", rfc822_header->reply_to);

	rfc822_header->date         = EM_SAFE_STRDUP(g_mime_message_get_date_as_string(message));
	EM_DEBUG_LOG_SEC("date : [%s]", rfc822_header->date);

	rfc822_header->subject      = EM_SAFE_STRDUP(g_mime_message_get_subject(message));
	EM_DEBUG_LOG_SEC("subject : [%s]", rfc822_header->subject);

	rfc822_header->sender       = EM_SAFE_STRDUP(g_mime_message_get_sender(message));
	EM_DEBUG_LOG_SEC("sender : [%s]", rfc822_header->sender);

	rfc822_header->to           = EM_SAFE_STRDUP(internet_address_list_to_string(g_mime_message_get_recipients(message, GMIME_RECIPIENT_TYPE_TO), false));
	EM_DEBUG_LOG_SEC("to : [%s]", rfc822_header->to);

	rfc822_header->cc           = EM_SAFE_STRDUP(internet_address_list_to_string(g_mime_message_get_recipients(message, GMIME_RECIPIENT_TYPE_CC), false));
	EM_DEBUG_LOG_SEC("cc : [%s]", rfc822_header->cc);

	rfc822_header->bcc          = EM_SAFE_STRDUP(internet_address_list_to_string(g_mime_message_get_recipients(message, GMIME_RECIPIENT_TYPE_BCC), false));
	EM_DEBUG_LOG_SEC("bcc : [%s]", rfc822_header->bcc);

	rfc822_header->message_id   = EM_SAFE_STRDUP(g_mime_message_get_message_id(message));
	EM_DEBUG_LOG("message_id : [%s]", rfc822_header->message_id);

	rfc822_header->content_type = EM_SAFE_STRDUP(g_mime_object_get_header((GMimeObject *)message, "content-type"));
	EM_DEBUG_LOG("content_type : [%s]", rfc822_header->content_type);

	rfc822_header->from         = EM_SAFE_STRDUP(g_mime_object_get_header((GMimeObject *)message, "from"));
	EM_DEBUG_LOG_SEC("from : [%s]", rfc822_header->from);

	rfc822_header->received     = EM_SAFE_STRDUP(g_mime_object_get_header((GMimeObject *)message, "received"));
	EM_DEBUG_LOG_SEC("received : [%s]", rfc822_header->received);

	rfc822_header->return_path  = EM_SAFE_STRDUP(g_mime_object_get_header((GMimeObject *)message, "return-path"));
	EM_DEBUG_LOG_SEC("return_path : [%s]", rfc822_header->return_path);

	rfc822_header->priority     = EM_SAFE_STRDUP(g_mime_object_get_header((GMimeObject *)message, "x-priority"));
	EM_DEBUG_LOG("priority : [%s]", rfc822_header->priority);

	rfc822_header->ms_priority  = EM_SAFE_STRDUP(g_mime_object_get_header((GMimeObject *)message, "x-msmail-priority"));
	EM_DEBUG_LOG("ms_priority : [%s]", rfc822_header->ms_priority);

	rfc822_header->dsp_noti_to  = EM_SAFE_STRDUP(g_mime_object_get_header((GMimeObject *)message, "disposition-notification-to"));
	EM_DEBUG_LOG("dsp_noti_to : [%s]", rfc822_header->dsp_noti_to);

	EM_DEBUG_FUNC_END();
	return err;
}

static void emcore_gmime_eml_parse_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data)
{
	EM_DEBUG_FUNC_BEGIN("parent[%p], part[%p], user_data[%p]", parent, part, user_data);

	int error = EMAIL_ERROR_NONE;
	int save_status = EMAIL_PART_DOWNLOAD_STATUS_NONE;
	char *msg_tmp_content_path = NULL;
	char *content_path = NULL;
	struct _m_content_info *cnt_info = (struct _m_content_info *)user_data;

	if (GMIME_IS_MESSAGE_PART(part)) {
		EM_DEBUG_LOG("Message Part");
		GMimeMessage *message = NULL;
		GMimeContentType *msg_ctype = NULL;
		GMimeContentDisposition *msg_disposition = NULL;
		GMimeStream *out_stream;
		char *msg_ctype_type = NULL;
		char *msg_ctype_subtype = NULL;
		char *msg_ctype_name = NULL;
		char *msg_disposition_str = NULL;
		char *msg_disposition_filename = NULL;
		char *msg_content_id = NULL;
		int real_size = 0;
		int msg_fd = 0;

		save_status = EMAIL_PART_DOWNLOAD_STATUS_NONE;

		if (cnt_info->grab_type != (GRAB_TYPE_TEXT|GRAB_TYPE_ATTACHMENT) &&
				cnt_info->grab_type != GRAB_TYPE_ATTACHMENT) {
			goto FINISH_OFF;
		}

		message = g_mime_message_part_get_message((GMimeMessagePart *)part);
		if (!message) {
			EM_DEBUG_EXCEPTION("Message is NULL");
			goto FINISH_OFF;
		}

		/*Content Type*/
		msg_ctype = g_mime_object_get_content_type(part);
		msg_ctype_type = (char *)g_mime_content_type_get_media_type(msg_ctype);
		msg_ctype_subtype = (char *)g_mime_content_type_get_media_subtype(msg_ctype);
		msg_ctype_name = (char *)g_mime_content_type_get_parameter(msg_ctype, "name");
		EM_DEBUG_LOG("Content-Type[%s/%s]", msg_ctype_type, msg_ctype_subtype);
		EM_DEBUG_LOG("RFC822/Message Content-Type-Name[%s]", msg_ctype_name);
		/*Content Type - END*/

		/*Content Disposition*/
		msg_disposition = g_mime_object_get_content_disposition(part);
		if (msg_disposition) {
			msg_disposition_str = (char *)g_mime_content_disposition_get_disposition(msg_disposition);
			msg_disposition_filename = (char *)g_mime_content_disposition_get_parameter(msg_disposition, "filename");
			if (EM_SAFE_STRLEN(msg_disposition_filename) == 0)
				msg_disposition_filename = NULL;
		}
		EM_DEBUG_LOG("RFC822/Message Disposition[%s]", msg_disposition_str);
		EM_DEBUG_LOG_SEC("RFC822/Message Disposition-Filename[%s]", msg_disposition_filename);
		/*Content Disposition - END*/

		/*Content ID*/
		msg_content_id = (char *)g_mime_object_get_content_id(part);
		EM_DEBUG_LOG("RFC822/Message Content-ID:%s", msg_content_id);

		/*save message content to tmp file*/
		if (!emcore_get_temp_file_name(&msg_tmp_content_path, &error)) {
			EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed [%d]", error);
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG_SEC("Temporary Content Path[%s]", msg_tmp_content_path);

		msg_fd = open(msg_tmp_content_path, O_WRONLY|O_CREAT, 0644);
		if (msg_fd < 0) {
			EM_DEBUG_EXCEPTION("open failed");
			goto FINISH_OFF;
		}

		out_stream = g_mime_stream_fs_new(msg_fd);
		real_size = g_mime_object_write_to_stream(GMIME_OBJECT(message), out_stream);
		if (out_stream) g_object_unref(out_stream);

		if (real_size <= 0) {
			EM_DEBUG_EXCEPTION("g_mime_object_write_to_stream failed");
			goto FINISH_OFF;
		}

		/* rfc822/message type is always saving to the attachment */
		EM_DEBUG_LOG("RFC822/Message is ATTACHMENT");

		struct attachment_info *file = NULL;
		struct attachment_info *temp_file = cnt_info->file;
		char *utf8_text = NULL;
		int err = EMAIL_ERROR_NONE;

		file = em_malloc(sizeof(struct attachment_info));
		if (file == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed...");
			goto FINISH_OFF;
		}

		file->type = ATTACHMENT;
		if (msg_disposition_filename) file->name = g_strdup(msg_disposition_filename);
		else if (msg_ctype_name) file->name = g_strdup(msg_ctype_name);
		else if (msg_content_id) file->name = g_strdup(msg_content_id);
		else file->name = g_strdup("unknown");

		if (msg_content_id) file->content_id = g_strdup(msg_content_id);

		if (!(utf8_text = g_mime_utils_header_decode_text(file->name))) {
			EM_DEBUG_EXCEPTION("g_mime_utils_header_decode_text failed [%d]", err);
		}
		EM_DEBUG_LOG_SEC("utf8_text : [%s]", utf8_text);

		if (utf8_text) {
			EM_SAFE_FREE(file->name);
			file->name = EM_SAFE_STRDUP(utf8_text);
		}
		EM_SAFE_FREE(utf8_text);

		if(msg_ctype_type && msg_ctype_subtype) {
			char mime_type_buffer[128] = {0,};
			snprintf(mime_type_buffer, sizeof(mime_type_buffer), "%s/%s", msg_ctype_type, msg_ctype_subtype);
			file->attachment_mime_type = g_strdup(mime_type_buffer);
		}

		file->save = g_strdup(msg_tmp_content_path);
		file->size = real_size;

		while (temp_file && temp_file->next)
			temp_file = temp_file->next;

		if (temp_file == NULL)
			cnt_info->file = file;
		else
			temp_file->next = file;

		/* check the partial status */
		int save_status = EMAIL_BODY_DOWNLOAD_STATUS_NONE;
		struct _m_content_info *temp_cnt_info = NULL;
		struct attachment_info *ai = NULL;

		temp_cnt_info = (struct _m_content_info *)em_malloc(sizeof(struct _m_content_info));
		if (temp_cnt_info == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed...");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		g_mime_message_foreach(message, emcore_gmime_eml_parse_foreach_cb, temp_cnt_info);

		save_status = temp_cnt_info->text.plain_save_status;
		save_status = temp_cnt_info->text.html_save_status;

		for (ai = temp_cnt_info->file; ai; ai = ai->next) {
			save_status = ai->save_status;
		}

		for (ai = temp_cnt_info->inline_file; ai; ai = ai->next) {
			save_status = ai->save_status;
		}

		file->save_status = save_status;
		EM_DEBUG_LOG("save_status : [%d], [%d]", file->save_status, save_status);

		if (temp_cnt_info) {
			emcore_free_content_info(temp_cnt_info);
			EM_SAFE_FREE(temp_cnt_info);
		}

	} else if (GMIME_IS_MESSAGE_PARTIAL(part)) {
		EM_DEBUG_LOG("Partial Part");
		//TODO
	} else if (GMIME_IS_MULTIPART(part)) {
		EM_DEBUG_LOG("Multi Part");
		GMimeMultipart *multi_part = NULL;
		multi_part = (GMimeMultipart *)part;

		multipart_status = 1;

		int multi_count = g_mime_multipart_get_count(multi_part);
		EM_DEBUG_LOG("Multi Part Count:%d", multi_count);
		EM_DEBUG_LOG("Boundary:%s\n\n", g_mime_multipart_get_boundary(multi_part));

	} else if (GMIME_IS_PART(part)) {
		EM_DEBUG_LOG("Part");
		int content_size = 0;

		save_status = EMAIL_PART_DOWNLOAD_STATUS_NONE;

		GMimePart *leaf_part = NULL;
		leaf_part = (GMimePart *)part;

		EM_DEBUG_LOG("Content ID:%s", g_mime_part_get_content_id(leaf_part));
		EM_DEBUG_LOG("Description:%s", g_mime_part_get_content_description(leaf_part));
		EM_DEBUG_LOG("MD5:%s", g_mime_part_get_content_md5(leaf_part));

		int content_disposition_type = 0;
		char *content_location = (char *)g_mime_part_get_content_location(leaf_part);
		EM_DEBUG_LOG_SEC("Location:%s", content_location);

		GMimeObject *mobject = (GMimeObject *)part;

		/*Content ID*/
		char *content_id = (char *)g_mime_object_get_content_id(mobject);

		/*Content Disposition*/
		GMimeContentDisposition *disposition = NULL;
		char *disposition_str = NULL;
		char *disposition_filename = NULL;
		disposition = g_mime_object_get_content_disposition(mobject);
		if (disposition) {
			disposition_str = (char *)g_mime_content_disposition_get_disposition(disposition);
			disposition_filename = (char *)g_mime_content_disposition_get_parameter(disposition, "filename");
			if (EM_SAFE_STRLEN(disposition_filename) == 0) disposition_filename = NULL;
		}
		EM_DEBUG_LOG("Disposition[%s]", disposition_str);
		EM_DEBUG_LOG_SEC("Disposition-Filename[%s]", disposition_filename);
		/*Content Disposition - END*/

		/*Content Type*/
		GMimeContentType *ctype = NULL;
		char *ctype_type = NULL;
		char *ctype_subtype = NULL;
		char *ctype_charset = NULL;
		char *ctype_name = NULL;

		ctype = g_mime_object_get_content_type(mobject);
		ctype_type = (char *)g_mime_content_type_get_media_type(ctype);
		ctype_subtype = (char *)g_mime_content_type_get_media_subtype(ctype);
		ctype_charset = (char *)g_mime_content_type_get_parameter(ctype, "charset");
		ctype_name = (char *)g_mime_content_type_get_parameter(ctype, "name");
		if (EM_SAFE_STRLEN(ctype_name) == 0) ctype_name = NULL;
		EM_DEBUG_LOG("Content-Type[%s/%s]", ctype_type, ctype_subtype);
		EM_DEBUG_LOG("Content-Type-Charset[%s]", ctype_charset);
		EM_DEBUG_LOG("Content-Type-Name[%s]", ctype_name);
		/*Content Type - END*/

		/*Content*/
		if (!emcore_get_temp_file_name(&content_path, &error)) {
			EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed [%d]", error);
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG_SEC("Temporary Content Path[%s]", content_path);


		GMimeContentEncoding enc = g_mime_part_get_content_encoding(leaf_part);
		switch(enc) {
		case GMIME_CONTENT_ENCODING_DEFAULT:
			EM_DEBUG_LOG("Encoding:ENCODING_DEFAULT");
			break;
		case GMIME_CONTENT_ENCODING_7BIT:
			EM_DEBUG_LOG("Encoding:ENCODING_7BIT");
			break;
		case GMIME_CONTENT_ENCODING_8BIT:
			EM_DEBUG_LOG("Encoding:ENCODING_8BIT");
			break;
		case GMIME_CONTENT_ENCODING_BINARY:
			EM_DEBUG_LOG("Encoding:ENCODING_BINARY");
			break;
		case GMIME_CONTENT_ENCODING_BASE64:
			EM_DEBUG_LOG("Encoding:ENCODING_BASE64");
			break;
		case GMIME_CONTENT_ENCODING_QUOTEDPRINTABLE:
			EM_DEBUG_LOG("Encoding:ENCODING_QUOTEDPRINTABLE");
			break;
		case GMIME_CONTENT_ENCODING_UUENCODE:
			EM_DEBUG_LOG("Encoding:ENCODING_UUENCODE");
			break;

		default:
			break;
		}

		GMimeDataWrapper *data = g_mime_part_get_content_object(leaf_part);
		if (data) {
			int fd;
			fd = open(content_path, O_WRONLY|O_CREAT, 0644);
			if (fd < 0) {
				EM_DEBUG_EXCEPTION("open failed");
				goto FINISH_OFF;
			}

			GMimeStream *out_stream;
			out_stream = g_mime_stream_fs_new(fd);

			//g_mime_data_wrapper_set_stream(data, out_stream);
			//g_mime_data_wrapper_set_encoding(data, enc);
			g_mime_data_wrapper_write_to_stream(data, out_stream);
			if (out_stream) g_object_unref(out_stream);
			emcore_get_file_size(content_path, &content_size, NULL);
		} else {
			EM_DEBUG_LOG("Data is NULL");
			goto FINISH_OFF;
		}
		/*Content - END*/

		/* Set the partial body */
		GMimeStream *part_stream = g_mime_data_wrapper_get_stream(data);
		if (part_stream) {
			EM_DEBUG_LOG("part_stream->bound_end : [%lld]", part_stream->bound_end);
			EM_DEBUG_LOG("super_stream->position : [%lld]", part_stream->super_stream->position);
			EM_DEBUG_LOG("multipart_status : [%d]", multipart_status);
			if (multipart_status && part_stream->super_stream->position <= part_stream->bound_end) {
				save_status = EMAIL_PART_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED;
			} else {
				save_status = EMAIL_PART_DOWNLOAD_STATUS_FULLY_DOWNLOADED;
			}

			EM_DEBUG_LOG("save_status : [%d]", save_status);
		}

		/*Figure out TEXT or ATTACHMENT(INLINE) ?*/
		int result = false;
		cnt_info->total_mail_size += content_size;
		if (content_id && (emcore_search_string_from_file(cnt_info->text.html, content_id, NULL, &result) == EMAIL_ERROR_NONE && result)) {
			content_disposition_type = INLINE_ATTACHMENT;
			cnt_info->total_body_size += content_size;
			EM_DEBUG_LOG("INLINE_ATTACHMENT");
		} else if ((disposition_str && g_ascii_strcasecmp(disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) || disposition_filename || ctype_name) {
			content_disposition_type = ATTACHMENT;
			cnt_info->total_attachment_size += content_size;
			EM_DEBUG_LOG("ATTACHMENT");
		} else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, "delivery-status") == 0) {
			content_disposition_type = ATTACHMENT;
			cnt_info->total_attachment_size += content_size;
			EM_DEBUG_LOG("ATTACHMENT");
		} else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, "calendar") == 0) {
			content_disposition_type = ATTACHMENT;
			cnt_info->total_attachment_size += content_size;
			EM_DEBUG_LOG("ATTACHMENT");
		} else {
			cnt_info->total_body_size += content_size;
			EM_DEBUG_LOG("Not INLINE or ATTACHMENT");
		}

		if (content_disposition_type != ATTACHMENT && content_disposition_type != INLINE_ATTACHMENT) {
			/*TEXT*/
			EM_DEBUG_LOG("TEXT");
			if (ctype_type && g_ascii_strcasecmp(ctype_type, "text") == 0) {
				if (!ctype_charset || g_ascii_strcasecmp(ctype_charset, "X-UNKNOWN") == 0) {
					ctype_charset = "UTF-8";
				}

				if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, "plain") == 0) {
					EM_DEBUG_LOG("TEXT/PLAIN");

					char *file_content = NULL;
					int content_size = 0;

					if (emcore_get_content_from_file(content_path, &file_content, &content_size) != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("emcore_get_content_from_file failed");
					}

					if (file_content && content_size > 0) {
						char escape = 0x1b;
						char detector[25] = {0,};
						snprintf(detector, sizeof(detector), "%c$B", escape);
						if (g_strrstr(ctype_charset, "UTF-8") && g_strrstr(file_content, detector)) {
							ctype_charset = "ISO-2022-JP";
						}
					}

					EM_SAFE_FREE(file_content);

					cnt_info->text.plain_charset = g_strdup(ctype_charset);
					cnt_info->text.plain = g_strdup(content_path);
					cnt_info->text.plain_save_status = save_status;
				} else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, "html") == 0) {
					EM_DEBUG_LOG("TEXT/HTML");
					cnt_info->text.html_charset = g_strdup(ctype_charset);
					cnt_info->text.html = g_strdup(content_path);
					cnt_info->text.html_save_status = save_status;
				}
			}
		} else {
			/*ATTACHMENT*/
			struct attachment_info *file = NULL;
			struct attachment_info *temp_file = NULL;
			char *utf8_text = NULL;
			int err = EMAIL_ERROR_NONE;

			if (content_disposition_type == ATTACHMENT) {
				EM_DEBUG_LOG("ATTACHMENT");
				temp_file = cnt_info->file;
			}
			else if (content_disposition_type == INLINE_ATTACHMENT) {
				EM_DEBUG_LOG("INLINE ATTACHMENT");
				temp_file = cnt_info->inline_file;
			}
			else {
				EM_DEBUG_EXCEPTION("Invalid content_disposition_type");
				goto FINISH_OFF;
			}

			file = em_malloc(sizeof(struct attachment_info));
			if (file == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed...");
				goto FINISH_OFF;
			}

			file->type = content_disposition_type;
			if (disposition_filename) file->name = g_strdup(disposition_filename);
			else if (ctype_name) file->name = g_strdup(ctype_name);
			else if (content_id) file->name = g_strdup(content_id);
			else if (content_disposition_type == ATTACHMENT) file->name = g_strdup("unknown");
			else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, "calendar") == 0) file->name = g_strdup("invite.vcs");
			else file->name = g_strdup("delivery-status");
			file->content_id = g_strdup(content_id);

			if (!(utf8_text = g_mime_utils_header_decode_text(file->name))) {
				EM_DEBUG_EXCEPTION("g_mime_utils_header_decode_text failed [%d]", err);
			}
			EM_DEBUG_LOG_SEC("utf8_text : [%s]", utf8_text);

			if (utf8_text) {
				EM_SAFE_FREE(file->name);
				file->name = EM_SAFE_STRDUP(utf8_text);
			}
			EM_SAFE_FREE(utf8_text);

			/* check inline name duplication */
			if (content_disposition_type == INLINE_ATTACHMENT) {
				if (emcore_gmime_check_filename_duplication(file->name, cnt_info)) {
					char *modified_name= NULL;
					modified_name = emcore_gmime_get_modified_filename_in_duplication(file->name);
					EM_SAFE_FREE(file->name);
					file->name = modified_name;
				}
			}

			/*cid replacement for inline attachment*/
			if (content_disposition_type == INLINE_ATTACHMENT) {
				char *file_content = NULL;
				char *encoding_file_name = NULL;
				int html_size = 0;
				iconv_t cd;

				if (emcore_get_content_from_file(cnt_info->text.html, &file_content, &html_size) != EMAIL_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("emcore_get_content_from_file failed");
				}

				EM_DEBUG_LOG_SEC("html_charset : [%s]", cnt_info->text.html_charset);
				if (strcasecmp(cnt_info->text.html_charset, "UTF-8") != 0) {
					cd = g_mime_iconv_open(cnt_info->text.html_charset, "UTF-8");
					if (cd) {
						encoding_file_name = g_mime_iconv_strdup(cd, file->name);
					}

					if (cd)
						g_mime_iconv_close(cd);

				} else {
					encoding_file_name = g_strdup(file->name);
				}

				EM_DEBUG_LOG_SEC("File name : [%s], encoding file name : [%s]", file->name, encoding_file_name);

				if (file_content && html_size > 0) {
					em_replace_string_ex(&file_content, "cid:", "");
					em_replace_string_ex(&file_content, file->content_id, encoding_file_name);

					html_size = EM_SAFE_STRLEN(file_content);
					if (emcore_set_content_to_file(file_content, cnt_info->text.html, html_size) != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("emcore_set_content_to_file failed");
					}
				}

				g_free(file_content); /* prevent 39110 */
				g_free(encoding_file_name);
			}

			if(ctype_type && ctype_subtype) {
				char mime_type_buffer[128] = {0,};
				snprintf(mime_type_buffer, sizeof(mime_type_buffer), "%s/%s", ctype_type, ctype_subtype);
				file->attachment_mime_type = g_strdup(mime_type_buffer);
			}

			file->save_status = save_status;
			file->save        = g_strdup(content_path);
			file->size        = content_size;

			if (ctype_type && g_ascii_strcasecmp(ctype_type, "APPLICATION") == 0) {
				if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, MIME_SUBTYPE_DRM_OBJECT) == 0)
					file->drm = EMAIL_ATTACHMENT_DRM_OBJECT;
				else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, MIME_SUBTYPE_DRM_RIGHTS) == 0)
					file->drm = EMAIL_ATTACHMENT_DRM_RIGHTS;
				else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, MIME_SUBTYPE_DRM_DCF) == 0)
					file->drm = EMAIL_ATTACHMENT_DRM_DCF;
			}

			while (temp_file && temp_file->next)
				temp_file = temp_file->next;

			if (temp_file == NULL) {
				if (content_disposition_type == ATTACHMENT) {
					cnt_info->file = file;
				} else {
					cnt_info->inline_file = file;
				}
			}
			else
				temp_file->next = file;
		}

		EM_SAFE_FREE(content_path);
	}

FINISH_OFF:

	EM_SAFE_FREE(msg_tmp_content_path);
	EM_SAFE_FREE(content_path);
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void emcore_gmime_imap_parse_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data)
{
	EM_DEBUG_FUNC_BEGIN("parent[%p], part[%p], user_data[%p]", parent, part, user_data);

	int error = EMAIL_ERROR_NONE;
	struct _m_content_info *cnt_info = (struct _m_content_info *)user_data;
	char *content_path = NULL;

	if (GMIME_IS_MESSAGE_PART(part)) {
		/* message/rfc822 or message/news */
		GMimeMessage *message = NULL;
		GMimeContentType *msg_ctype = NULL;
		GMimeContentDisposition *msg_disposition = NULL;
		char *msg_ctype_type = NULL;
		char *msg_ctype_subtype = NULL;
		char *msg_ctype_name = NULL;
		char *msg_ctype_size = NULL;
		char *msg_disposition_str = NULL;
		char *msg_disposition_filename = NULL;
		char *msg_content_id = NULL;
		unsigned long msg_content_size = 0;
		EM_DEBUG_LOG("Message Part");

		message = g_mime_message_part_get_message((GMimeMessagePart *)part);
		if (!message) {
			EM_DEBUG_EXCEPTION("Message is NULL");
			goto FINISH_OFF;
		}

		/*Content Type*/
		msg_ctype = g_mime_object_get_content_type(part);
		msg_ctype_type = (char *)g_mime_content_type_get_media_type(msg_ctype);
		msg_ctype_subtype = (char *)g_mime_content_type_get_media_subtype(msg_ctype);
		msg_ctype_name = (char *)g_mime_content_type_get_parameter(msg_ctype, "name");
		msg_ctype_size = (char *)g_mime_content_type_get_parameter(msg_ctype, "message_size");
		EM_DEBUG_LOG("Content-Type[%s/%s]", msg_ctype_type, msg_ctype_subtype);
		EM_DEBUG_LOG("RFC822/Message Content-Type-Name[%s]", msg_ctype_name);
		EM_DEBUG_LOG("Part.size.bytes[%s]", msg_ctype_size);

		if (msg_ctype_size) msg_content_size = atol(msg_ctype_size);
		/*Content Type - END*/

		/*Content Disposition*/
		msg_disposition = g_mime_object_get_content_disposition(part);
		if (msg_disposition) {
			msg_disposition_str = (char *)g_mime_content_disposition_get_disposition(msg_disposition);
			msg_disposition_filename = (char *)g_mime_content_disposition_get_parameter(msg_disposition, "filename");
			if (EM_SAFE_STRLEN(msg_disposition_filename) == 0) msg_disposition_filename = NULL;
		}
		EM_DEBUG_LOG("RFC822/Message Disposition[%s]", msg_disposition_str);
		EM_DEBUG_LOG_SEC("RFC822/Message Disposition-Filename[%s]", msg_disposition_filename);
		/*Content Disposition - END*/

		/*Content ID*/
		msg_content_id = (char *)g_mime_object_get_content_id(part);
		EM_DEBUG_LOG("RFC822/Message Content-ID:%s", msg_content_id);

		if (msg_disposition_str && g_ascii_strcasecmp(msg_disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) {
			EM_DEBUG_LOG("RFC822/Message is ATTACHMENT");

			struct attachment_info *file = NULL;
			struct attachment_info *temp_file = cnt_info->file;
			char *utf8_text = NULL;
			int err = EMAIL_ERROR_NONE;

			file = em_malloc(sizeof(struct attachment_info));
			if (file == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed...");
				goto FINISH_OFF;
			}

			file->type = ATTACHMENT;
			if (msg_disposition_filename) file->name = g_strdup(msg_disposition_filename);
			else if (msg_ctype_name) file->name = g_strdup(msg_ctype_name);
			else if (msg_content_id) file->name = g_strdup(msg_content_id);
			else file->name = g_strdup("unknown");

			if (msg_content_id) file->content_id = g_strdup(msg_content_id);

			if (!(utf8_text = g_mime_utils_header_decode_text(file->name))) {
				EM_DEBUG_EXCEPTION("g_mime_utils_header_decode_text failed [%d]", err);
			}
			EM_DEBUG_LOG_SEC("utf8_text : [%s]", utf8_text);

			if (utf8_text) {
				EM_SAFE_FREE(file->name);
				file->name = EM_SAFE_STRDUP(utf8_text);
			}
			EM_SAFE_FREE(utf8_text);

			if(msg_ctype_type && msg_ctype_subtype) {
				char mime_type_buffer[128] = {0,};
				snprintf(mime_type_buffer, sizeof(mime_type_buffer), "%s/%s", msg_ctype_type, msg_ctype_subtype);
				file->attachment_mime_type = g_strdup(mime_type_buffer);
			}

			file->save = NULL;
			file->size = msg_content_size;
			file->save_status = 0;

			while (temp_file && temp_file->next)
				temp_file = temp_file->next;

			if (temp_file == NULL)
				cnt_info->file = file;
			else
				temp_file->next = file;
		}

		//g_mime_message_foreach(message, emcore_gmime_imap_parse_foreach_cb, user_data);

	} else if (GMIME_IS_MESSAGE_PARTIAL(part)) {
		/* message/partial */

		EM_DEBUG_LOG("Partial Part");
		//TODO
	} else if (GMIME_IS_MULTIPART(part)) {
		/* multipart/mixed, multipart/alternative,
		 * multipart/related, multipart/signed,
		 * multipart/encrypted, etc... */
		EM_DEBUG_LOG("Multi Part");
		GMimeMultipart *multi_part = NULL;
		multi_part = (GMimeMultipart *)part;

		int multi_count = g_mime_multipart_get_count(multi_part);
		EM_DEBUG_LOG("Multi Part Count:%d", multi_count);
		EM_DEBUG_LOG("Boundary:%s\n\n", g_mime_multipart_get_boundary(multi_part));

	} else if (GMIME_IS_PART(part)) {
		/* a normal leaf part, could be text/plain or
		 * image/jpeg etc */

		EM_DEBUG_LOG("Part");
		int download_status = 0;
		int content_disposition_type = 0;
		char *content_id = NULL;
		char *content_location = NULL;
		char *disposition_str = NULL;
		char *disposition_filename = NULL;
		int disposition_size = 0;
		char *disposition_size_str = NULL;
		char *ctype_type = NULL;
		char *ctype_subtype = NULL;
		char *ctype_charset = NULL;
		char *ctype_name = NULL;
		char *ctype_size = NULL;
		unsigned long content_size = 0;
		int real_size = 0;

		GMimeContentType *ctype = NULL;
		GMimeContentDisposition *disposition = NULL;
		GMimePart *leaf_part = NULL;
		GMimeObject *mobject = (GMimeObject *)part;
		leaf_part = (GMimePart *)part;

		/*Content Type*/
		ctype = g_mime_object_get_content_type(mobject);
		ctype_type = (char *)g_mime_content_type_get_media_type(ctype);
		ctype_subtype = (char *)g_mime_content_type_get_media_subtype(ctype);
		ctype_charset = (char *)g_mime_content_type_get_parameter(ctype, "charset");
		ctype_name = (char *)g_mime_content_type_get_parameter(ctype, "name");
		ctype_size = (char *)g_mime_content_type_get_parameter(ctype, "part_size");
		if (EM_SAFE_STRLEN(ctype_name) == 0) ctype_name = NULL;
		EM_DEBUG_LOG("Content-Type[%s/%s]", ctype_type, ctype_subtype);
		EM_DEBUG_LOG("Content-Type-Charset[%s]", ctype_charset);
		EM_DEBUG_LOG_SEC("Content-Type-Name[%s]", ctype_name);
		EM_DEBUG_LOG("Part.size.bytes[%s]", ctype_size);

		if (ctype_size) content_size = atol(ctype_size);
		/*Content Type - END*/

		/*Content Disposition*/
		disposition = g_mime_object_get_content_disposition(mobject);
		if (disposition) {
			disposition_str = (char *)g_mime_content_disposition_get_disposition(disposition);
			disposition_filename = (char *)g_mime_content_disposition_get_parameter(disposition, "filename");
			if (EM_SAFE_STRLEN(disposition_filename) == 0) disposition_filename = NULL;
			disposition_size_str = (char *)g_mime_content_disposition_get_parameter(disposition, "size");
			if (disposition_size_str) disposition_size = atoi(disposition_size_str);
		}
		EM_DEBUG_LOG("Disposition[%s]", disposition_str);
		EM_DEBUG_LOG_SEC("Disposition-Filename[%s]", disposition_filename);
		EM_DEBUG_LOG("Disposition size[%d]", disposition_size);
		/*Content Disposition - END*/

		/*Content ID*/
		content_id = (char *)g_mime_object_get_content_id(mobject);
		EM_DEBUG_LOG_SEC("Content-ID:%s", content_id);

		/*Content Location*/
		content_location = (char *)g_mime_part_get_content_location(leaf_part);
		EM_DEBUG_LOG_SEC("Content-Location:%s", content_location);

		/*Figure out TEXT or ATTACHMENT(INLINE) ?*/
		int result = false;
		if (disposition_str && g_ascii_strcasecmp(disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) {
                	if (content_id &&
		        	(emcore_search_string_from_file(cnt_info->text.html, content_id, NULL, &result) == EMAIL_ERROR_NONE && result)) {
				content_disposition_type = INLINE_ATTACHMENT;
				EM_DEBUG_LOG("INLINE_ATTACHMENT");
                        } else {
        			content_disposition_type = ATTACHMENT;
        			EM_DEBUG_LOG("ATTACHMENT");
                        }
		} else if ((content_id || content_location) && (ctype_name || disposition_filename)) {
			if (cnt_info->attachment_only) {
				content_disposition_type = ATTACHMENT;
				EM_DEBUG_LOG("ATTACHMENT");
			} else {
				content_disposition_type = INLINE_ATTACHMENT;
				EM_DEBUG_LOG("INLINE_ATTACHMENT");
			}
		} else {
			if (content_id &&
					(emcore_search_string_from_file(cnt_info->text.html, content_id, NULL, &result) == EMAIL_ERROR_NONE && result)) {
				content_disposition_type = INLINE_ATTACHMENT;
				EM_DEBUG_LOG("INLINE_ATTACHMENT");
			} else if (content_id || content_location) {
				if (g_ascii_strcasecmp(ctype_type, "text") == 0 &&
						(g_ascii_strcasecmp(ctype_subtype, "plain") == 0 || g_ascii_strcasecmp(ctype_subtype, "html") == 0)) {
					EM_DEBUG_LOG("TEXT");
				} else {
					if (cnt_info->attachment_only) {
						content_disposition_type = ATTACHMENT;
						EM_DEBUG_LOG("ATTACHMENT");
					} else {
						content_disposition_type = INLINE_ATTACHMENT;
						EM_DEBUG_LOG("INLINE_ATTACHMENT");
					}
				}
			} else {
				if (g_ascii_strcasecmp(ctype_type, "text") == 0 &&
						(g_ascii_strcasecmp(ctype_subtype, "plain") == 0 || g_ascii_strcasecmp(ctype_subtype, "html") == 0)) {
					EM_DEBUG_LOG("TEXT");
				} else {
					content_disposition_type = ATTACHMENT;
					EM_DEBUG_LOG("ATTACHMENT");
				}
			}
		}

		/*if (content_id && (emcore_search_string_from_file(cnt_info->text.html, content_id, NULL, &result) == EMAIL_ERROR_NONE && result)) {
			content_disposition_type = INLINE_ATTACHMENT;
			EM_DEBUG_LOG("INLINE_ATTACHMENT");
		} else if ((disposition_str && g_ascii_strcasecmp(disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) || disposition_filename || ctype_name) {
			content_disposition_type = ATTACHMENT;
			EM_DEBUG_LOG("ATTACHMENT");
		}*/

		if (!emcore_get_temp_file_name(&content_path, &error))  {
			EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed [%d]", error);
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG_SEC("Temporary Content Path[%s]", content_path);

		if (content_disposition_type != ATTACHMENT) {
			/*Content*/
			GMimeContentEncoding enc = g_mime_part_get_content_encoding(leaf_part);
			switch(enc) {
			case GMIME_CONTENT_ENCODING_DEFAULT:
				EM_DEBUG_LOG("Encoding:ENCODING_DEFAULT");
				break;
			case GMIME_CONTENT_ENCODING_7BIT:
				EM_DEBUG_LOG("Encoding:ENCODING_7BIT");
				break;
			case GMIME_CONTENT_ENCODING_8BIT:
				EM_DEBUG_LOG("Encoding:ENCODING_8BIT");
				break;
			case GMIME_CONTENT_ENCODING_BINARY:
				EM_DEBUG_LOG("Encoding:ENCODING_BINARY");
				break;
			case GMIME_CONTENT_ENCODING_BASE64:
				EM_DEBUG_LOG("Encoding:ENCODING_BASE64");
				break;
			case GMIME_CONTENT_ENCODING_QUOTEDPRINTABLE:
				EM_DEBUG_LOG("Encoding:ENCODING_QUOTEDPRINTABLE");
				break;
			case GMIME_CONTENT_ENCODING_UUENCODE:
				EM_DEBUG_LOG("Encoding:ENCODING_UUENCODE");
				break;

			default:
				break;
			}

			GMimeDataWrapper *data = g_mime_part_get_content_object(leaf_part);
			if (data) {
				EM_DEBUG_LOG_DEV("DataWrapper/ref-cnt[%d]", data->parent_object.ref_count);
				int fd = 0;
				int src_length = 0;
				GMimeStream *out_stream = NULL;
				GMimeStream *src_stream = NULL;

				fd = open(content_path, O_WRONLY|O_CREAT, 0644);
				if (fd < 0) {
					EM_DEBUG_EXCEPTION("holder open failed :  holder is a filename that will be saved.");
					goto FINISH_OFF;
				}

				out_stream = g_mime_stream_fs_new(fd);
				src_stream = g_mime_data_wrapper_get_stream(data);
				if (src_stream) src_length = g_mime_stream_length(src_stream);
				EM_DEBUG_LOG_DEV("Data length [%d]", src_length);

				if (src_length >= content_size) /* fully downloaded */
					download_status = EMAIL_PART_DOWNLOAD_STATUS_FULLY_DOWNLOADED;
				else /* partialy downloaded */
					download_status = EMAIL_PART_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED;

				/*g_mime_data_wrapper_set_stream(data, out_stream);
				g_mime_data_wrapper_set_encoding(data, enc);*/

				g_mime_data_wrapper_write_to_stream(data, out_stream);
				if (out_stream) g_object_unref(out_stream);
				emcore_get_file_size(content_path, &real_size, NULL);
			}
			/*Content - END*/
		}

		if (content_disposition_type != ATTACHMENT && content_disposition_type != INLINE_ATTACHMENT) {
			/*TEXT*/
			EM_DEBUG_LOG("TEXT");

			if (ctype_type && g_ascii_strcasecmp(ctype_type, "text") == 0) {
				if (!ctype_charset || g_ascii_strcasecmp(ctype_charset, "X-UNKNOWN") == 0) {
					ctype_charset = "UTF-8";
				}

				if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, "plain") == 0) {
					EM_DEBUG_LOG("TEXT/PLAIN");

					char *file_content = NULL;
					int content_size = 0;

					if (emcore_get_content_from_file(content_path, &file_content, &content_size) != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("emcore_get_content_from_file failed");
					}

					if (file_content && content_size > 0) {
						char escape = 0x1b;
						char detector[25] = {0,};
						snprintf(detector, sizeof(detector), "%c$B", escape);
						if (g_strrstr(ctype_charset, "UTF-8") && g_strrstr(file_content, detector)) {
							ctype_charset = "ISO-2022-JP";
						}
					}

					EM_SAFE_FREE(file_content);

					cnt_info->text.plain_charset = g_strdup(ctype_charset);
					cnt_info->text.plain = g_strdup(content_path);
				} else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, "html") == 0) {
					EM_DEBUG_LOG("TEXT/HTML");
					cnt_info->text.html_charset = g_strdup(ctype_charset);
					cnt_info->text.html = g_strdup(content_path);
				}
			}
		} else if (content_disposition_type == INLINE_ATTACHMENT) {

			struct attachment_info *file = NULL;
			struct attachment_info *temp_file = cnt_info->inline_file;
			char *utf8_text = NULL;
			char *file_content = NULL;
			int file_size = 0;
			int err = EMAIL_ERROR_NONE;

			file = em_malloc(sizeof(struct attachment_info));
			if (file == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed...");
				goto FINISH_OFF;
			}

			file->type = content_disposition_type;
			if (disposition_filename) file->name = g_strdup(disposition_filename);
			else if (ctype_name) file->name = g_strdup(ctype_name);
			else if (content_id) file->name = g_strdup(content_id);
			file->content_id = g_strdup(content_id);

			if (file->name && g_strrstr(file->name, "/") != NULL) {
				char *tmp_ptr = file->name;
				int tmp_len = EM_SAFE_STRLEN(file->name);
				int tmpi = 0;
				for (tmpi=0; tmpi<tmp_len; tmpi++) {
					if (*(tmp_ptr+tmpi) == '/') {
						*(tmp_ptr+tmpi) = '_';
					}
				}
				EM_DEBUG_LOG_SEC("file->name[%s]", file->name);
			}

			if (!(utf8_text = g_mime_utils_header_decode_text(file->name))) {
				EM_DEBUG_EXCEPTION("g_mime_utils_header_decode_text failed [%d]", err);
			}
			EM_DEBUG_LOG_SEC("utf8_text : [%s]", utf8_text);

			if (utf8_text) {
				EM_SAFE_FREE(file->name);
				file->name = EM_SAFE_STRDUP(utf8_text);
			}
			EM_SAFE_FREE(utf8_text);

			/* check inline name duplication */
			char *modified_name = NULL;
			if (emcore_gmime_check_filename_duplication(file->name, cnt_info)) {
				modified_name = emcore_gmime_get_modified_filename_in_duplication(file->name);
				EM_SAFE_FREE(file->name);
				file->name = modified_name;
				modified_name = NULL;
			}

			emcore_unescape_from_url(file->name, &modified_name);
			EM_DEBUG_LOG_SEC("file->name[%s] modified_name[%s]", file->name, modified_name);
			EM_SAFE_FREE(file->name);
			file->name = modified_name;

			/*cid replacement for inline attachment*/
			if (emcore_get_content_from_file(cnt_info->text.html, &file_content, &file_size) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_get_content_from_file failed");
			}

			if (file_content && file_size > 0) {
				em_replace_string_ex(&file_content, "cid:", "");
				em_replace_string_ex(&file_content, file->content_id, file->name);

				file_size = EM_SAFE_STRLEN(file_content);
				if (emcore_set_content_to_file(file_content, cnt_info->text.html, file_size) != EMAIL_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("emcore_set_content_to_file failed");
				}
			}
			g_free(file_content); /* prevent 39110 */

			if(ctype_type && ctype_subtype) {
				char mime_type_buffer[128] = {0,};
				snprintf(mime_type_buffer, sizeof(mime_type_buffer), "%s/%s", ctype_type, ctype_subtype);
				file->attachment_mime_type = g_strdup(mime_type_buffer);
			}

			file->save = g_strdup(content_path);
			file->size = disposition_size ? disposition_size : content_size;
			if (download_status == EMAIL_PART_DOWNLOAD_STATUS_FULLY_DOWNLOADED) {
				file->save_status = 1;
				file->size = real_size;
			}
			else
				file->save_status = 0;

			while (temp_file && temp_file->next)
				temp_file = temp_file->next;

			if (temp_file == NULL)
				cnt_info->inline_file = file;
			else
				temp_file->next = file;
		} else if (content_disposition_type == ATTACHMENT) {

			struct attachment_info *file = NULL;
			struct attachment_info *temp_file = cnt_info->file;
			char *utf8_text = NULL;
			int err = EMAIL_ERROR_NONE;

			file = em_malloc(sizeof(struct attachment_info));
			if (file == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed...");
				goto FINISH_OFF;
			}

			file->type = content_disposition_type;
			if (disposition_filename) file->name = g_strdup(disposition_filename);
			else if (ctype_name) file->name = g_strdup(ctype_name);
			else if (content_id) file->name = g_strdup(content_id);
			else file->name = g_strdup("unknown-attachment");

			file->content_id = g_strdup(content_id);

			if (!(utf8_text = g_mime_utils_header_decode_text(file->name))) {
				EM_DEBUG_EXCEPTION("g_mime_utils_header_decode_text failed [%d]", err);
			}
			EM_DEBUG_LOG_SEC("utf8_text : [%s]", utf8_text);

			if (utf8_text) {
				EM_SAFE_FREE(file->name);
				file->name = EM_SAFE_STRDUP(utf8_text);
			}
			EM_SAFE_FREE(utf8_text);

			if(ctype_type && ctype_subtype) {
				char mime_type_buffer[128] = {0,};
				snprintf(mime_type_buffer, sizeof(mime_type_buffer), "%s/%s", ctype_type, ctype_subtype);
				file->attachment_mime_type = g_strdup(mime_type_buffer);
			}

			file->save = g_strdup(content_path);
			file->size = disposition_size ? disposition_size : content_size;
			file->save_status = 0;

			if (ctype_type && g_ascii_strcasecmp(ctype_type, "APPLICATION") == 0) {
				if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, MIME_SUBTYPE_DRM_OBJECT) == 0)
					file->drm = EMAIL_ATTACHMENT_DRM_OBJECT;
				else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, MIME_SUBTYPE_DRM_RIGHTS) == 0)
					file->drm = EMAIL_ATTACHMENT_DRM_RIGHTS;
				else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, MIME_SUBTYPE_DRM_DCF) == 0)
					file->drm = EMAIL_ATTACHMENT_DRM_DCF;
			}

			while (temp_file && temp_file->next)
				temp_file = temp_file->next;

			if (temp_file == NULL)
				cnt_info->file = file;
			else
				temp_file->next = file;
		}

		EM_SAFE_FREE(content_path);
	}

FINISH_OFF:

	EM_SAFE_FREE(content_path);

	EM_DEBUG_FUNC_END();
}


INTERNAL_FUNC void emcore_gmime_imap_parse_full_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data)
{
	EM_DEBUG_FUNC_BEGIN("parent[%p], part[%p], user_data[%p]", parent, part, user_data);

	int error = EMAIL_ERROR_NONE;
	struct _m_content_info *cnt_info = (struct _m_content_info *)user_data;
	char *content_path = NULL;

	if (GMIME_IS_MESSAGE_PART(part)) {
		EM_DEBUG_LOG("Message Part");
		GMimeContentType *msg_ctype = NULL;
		GMimeContentDisposition *msg_disposition = NULL;
		char *msg_ctype_type = NULL;
		char *msg_ctype_subtype = NULL;
		char *msg_ctype_name = NULL;
		char *msg_disposition_str = NULL;
		char *msg_disposition_filename = NULL;
		char *msg_content_id = NULL;
		char *msg_tmp_content_path = NULL;
		int real_size = 0;

		if (cnt_info->grab_type != (GRAB_TYPE_TEXT|GRAB_TYPE_ATTACHMENT) &&
				cnt_info->grab_type != GRAB_TYPE_ATTACHMENT) {
			goto FINISH_OFF;
		}

		/*Content Type*/
		msg_ctype = g_mime_object_get_content_type(part);
		msg_ctype_type = (char *)g_mime_content_type_get_media_type(msg_ctype);
		msg_ctype_subtype = (char *)g_mime_content_type_get_media_subtype(msg_ctype);
		msg_ctype_name = (char *)g_mime_content_type_get_parameter(msg_ctype, "name");
		msg_tmp_content_path = (char *)g_mime_content_type_get_parameter(msg_ctype, "tmp_content_path");
		EM_DEBUG_LOG("Content-Type[%s/%s]", msg_ctype_type, msg_ctype_subtype);
		EM_DEBUG_LOG_SEC("RFC822/Message Content-Type-Name[%s]", msg_ctype_name);
		EM_DEBUG_LOG_SEC("RFC822/Message Content-Type-tmp-path[%s]", msg_tmp_content_path);
		/*Content Type - END*/

		/*Content Disposition*/
		msg_disposition = g_mime_object_get_content_disposition(part);
		if (msg_disposition) {
			msg_disposition_str = (char *)g_mime_content_disposition_get_disposition(msg_disposition);
			msg_disposition_filename = (char *)g_mime_content_disposition_get_parameter(msg_disposition, "filename");
			if (EM_SAFE_STRLEN(msg_disposition_filename) == 0) msg_disposition_filename = NULL;
		}
		EM_DEBUG_LOG("RFC822/Message Disposition[%s]", msg_disposition_str);
		EM_DEBUG_LOG_SEC("RFC822/Message Disposition-Filename[%s]", msg_disposition_filename);
		/*Content Disposition - END*/

		/*Content ID*/
		msg_content_id = (char *)g_mime_object_get_content_id(part);
		EM_DEBUG_LOG("RFC822/Message Content-ID:%s", msg_content_id);

		emcore_get_file_size(msg_tmp_content_path, &real_size, NULL);
		if (real_size <= 0) {
			EM_DEBUG_EXCEPTION("tmp content file is not valid");
			goto FINISH_OFF;
		}

		if (msg_disposition_str && g_ascii_strcasecmp(msg_disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) {
			EM_DEBUG_LOG("RFC822/Message is ATTACHMENT");

			struct attachment_info *file = NULL;
			struct attachment_info *temp_file = cnt_info->file;
			char *utf8_text = NULL;
			int err = EMAIL_ERROR_NONE;

			file = em_malloc(sizeof(struct attachment_info));
			if (file == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed...");
				goto FINISH_OFF;
			}

			file->type = ATTACHMENT;
			if (msg_disposition_filename) file->name = g_strdup(msg_disposition_filename);
			else if (msg_ctype_name) file->name = g_strdup(msg_ctype_name);
			else if (msg_content_id) file->name = g_strdup(msg_content_id);
			else file->name = g_strdup("unknown");

			if (msg_content_id) file->content_id = g_strdup(msg_content_id);

			if (!(utf8_text = g_mime_utils_header_decode_text(file->name))) {
				EM_DEBUG_EXCEPTION("g_mime_utils_header_decode_text failed [%d]", err);
			}
			EM_DEBUG_LOG_SEC("utf8_text : [%s]", utf8_text);

			if (utf8_text) {
				EM_SAFE_FREE(file->name);
				file->name = EM_SAFE_STRDUP(utf8_text);
			}
			EM_SAFE_FREE(utf8_text);

			if(msg_ctype_type && msg_ctype_subtype) {
				char mime_type_buffer[128] = {0,};
				snprintf(mime_type_buffer, sizeof(mime_type_buffer), "%s/%s", msg_ctype_type, msg_ctype_subtype);
				file->attachment_mime_type = g_strdup(mime_type_buffer);
			}

			file->save = g_strdup(msg_tmp_content_path);
			file->size = real_size;
			file->save_status = 1;

			while (temp_file && temp_file->next)
				temp_file = temp_file->next;

			if (temp_file == NULL)
				cnt_info->file = file;
			else
				temp_file->next = file;
		}

		//g_mime_message_foreach(message, emcore_gmime_imap_parse_full_foreach_cb, user_data);
	} else if (GMIME_IS_MESSAGE_PARTIAL(part)) {
		EM_DEBUG_LOG("Partial Part");
	} else if (GMIME_IS_MULTIPART(part)) {
		EM_DEBUG_LOG("Multi Part");
		GMimeMultipart *multi_part = NULL;
		multi_part = (GMimeMultipart *)part;

		int multi_count = g_mime_multipart_get_count(multi_part);
		EM_DEBUG_LOG("Multi Part Count:%d", multi_count);
		EM_DEBUG_LOG("Boundary:%s\n\n", g_mime_multipart_get_boundary(multi_part));
	} else if (GMIME_IS_PART(part)) {
		EM_DEBUG_LOG("Part");
		int content_disposition_type = 0;
		int real_size = 0;
		char *content_id = NULL;
		char *content_location = NULL;
		char *disposition_str = NULL;
		char *disposition_filename = NULL;
		char *ctype_type = NULL;
		char *ctype_subtype = NULL;
		char *ctype_charset = NULL;
		char *ctype_name = NULL;
		char *ctype_size = NULL;
		char *tmp_path = NULL;

		GMimeContentType *ctype = NULL;
		GMimeContentDisposition *disposition = NULL;
		GMimePart *leaf_part = NULL;
		GMimeObject *mobject = (GMimeObject *)part;
		leaf_part = (GMimePart *)part;

		/*Content Type*/
		ctype = g_mime_object_get_content_type(mobject);
		ctype_type = (char *)g_mime_content_type_get_media_type(ctype);
		ctype_subtype = (char *)g_mime_content_type_get_media_subtype(ctype);
		ctype_charset = (char *)g_mime_content_type_get_parameter(ctype, "charset");
		ctype_name = (char *)g_mime_content_type_get_parameter(ctype, "name");
		ctype_size = (char *)g_mime_content_type_get_parameter(ctype, "part_size");
		if (EM_SAFE_STRLEN(ctype_name) == 0) ctype_name = NULL;
		EM_DEBUG_LOG("Content-Type[%s/%s]", ctype_type, ctype_subtype);
		EM_DEBUG_LOG("Content-Type-Charset[%s]", ctype_charset);
		EM_DEBUG_LOG_SEC("Content-Type-Name[%s]", ctype_name);
		/*Content Type - END*/

		/*Content Disposition*/
		disposition = g_mime_object_get_content_disposition(mobject);
		if (disposition) {
			disposition_str = (char *)g_mime_content_disposition_get_disposition(disposition);
			disposition_filename = (char *)g_mime_content_disposition_get_parameter(disposition, "filename");
			if (EM_SAFE_STRLEN(disposition_filename) == 0) disposition_filename = NULL;
		}
		EM_DEBUG_LOG("Disposition[%s]", disposition_str);
		EM_DEBUG_LOG_SEC("Disposition-Filename[%s]", disposition_filename);
		/*Content Disposition - END*/

		/*Content ID*/
		content_id = (char *)g_mime_object_get_content_id(mobject);
		EM_DEBUG_LOG_SEC("Content-ID:%s", content_id);

		/*Content Location*/
		content_location = (char *)g_mime_part_get_content_location(leaf_part);
		EM_DEBUG_LOG_SEC("Content-Location:%s", content_location);


		/*Figure out TEXT or ATTACHMENT(INLINE) ?*/
		int result = false;
		if (disposition_str && g_ascii_strcasecmp(disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) {
			content_disposition_type = ATTACHMENT;
			EM_DEBUG_LOG("ATTACHMENT");
		} else if ((content_id || content_location) && (ctype_name || disposition_filename)) {
			if (cnt_info->attachment_only) {
				content_disposition_type = ATTACHMENT;
				EM_DEBUG_LOG("ATTACHMENT");
			} else {
				content_disposition_type = INLINE_ATTACHMENT;
				EM_DEBUG_LOG("INLINE_ATTACHMENT");
			}
		} else {
			if (content_id &&
					(emcore_search_string_from_file(cnt_info->text.html, content_id, NULL, &result) == EMAIL_ERROR_NONE && result)) {
				content_disposition_type = INLINE_ATTACHMENT;
				EM_DEBUG_LOG("INLINE_ATTACHMENT");
			} else if (content_id || content_location) {
				if (g_ascii_strcasecmp(ctype_type, "text") == 0 &&
						(g_ascii_strcasecmp(ctype_subtype, "plain") == 0 || g_ascii_strcasecmp(ctype_subtype, "html") == 0)) {
					EM_DEBUG_LOG("TEXT");
				} else {
					if (cnt_info->attachment_only) {
						content_disposition_type = ATTACHMENT;
						EM_DEBUG_LOG("ATTACHMENT");
					} else {
						content_disposition_type = INLINE_ATTACHMENT;
						EM_DEBUG_LOG("INLINE_ATTACHMENT");
					}
				}
			} else {
				if (g_ascii_strcasecmp(ctype_type, "text") == 0 &&
						(g_ascii_strcasecmp(ctype_subtype, "plain") == 0 || g_ascii_strcasecmp(ctype_subtype, "html") == 0)) {
					EM_DEBUG_LOG("TEXT");
				} else {
					content_disposition_type = ATTACHMENT;
					EM_DEBUG_LOG("ATTACHMENT");
				}
			}
		}

		/*if (content_id && (emcore_search_string_from_file(cnt_info->text.html, content_id, NULL, &result) == EMAIL_ERROR_NONE && result)) {
			content_disposition_type = INLINE_ATTACHMENT;
			EM_DEBUG_LOG("INLINE_ATTACHMENT");
		} else if ((disposition_str && g_ascii_strcasecmp(disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) || disposition_filename || ctype_name) {
			content_disposition_type = ATTACHMENT;
			EM_DEBUG_LOG("ATTACHMENT");
		}*/

		/*Content*/
		if (!emcore_get_temp_file_name(&content_path, &error)) {
			EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed [%d]", error);
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG_SEC("Temporary Content Path[%s]", content_path);

		GMimeContentEncoding enc = g_mime_part_get_content_encoding(leaf_part);
		switch(enc) {
		case GMIME_CONTENT_ENCODING_DEFAULT:
			EM_DEBUG_LOG("Encoding:ENCODING_DEFAULT");
			break;
		case GMIME_CONTENT_ENCODING_7BIT:
			EM_DEBUG_LOG("Encoding:ENCODING_7BIT");
			break;
		case GMIME_CONTENT_ENCODING_8BIT:
			EM_DEBUG_LOG("Encoding:ENCODING_8BIT");
			break;
		case GMIME_CONTENT_ENCODING_BINARY:
			EM_DEBUG_LOG("Encoding:ENCODING_BINARY");
			break;
		case GMIME_CONTENT_ENCODING_BASE64:
			EM_DEBUG_LOG("Encoding:ENCODING_BASE64");
			break;
		case GMIME_CONTENT_ENCODING_QUOTEDPRINTABLE:
			EM_DEBUG_LOG("Encoding:ENCODING_QUOTEDPRINTABLE");
			break;
		case GMIME_CONTENT_ENCODING_UUENCODE:
			EM_DEBUG_LOG("Encoding:ENCODING_UUENCODE");
			break;

		default:
			break;
		}

		GMimeDataWrapper *data = g_mime_part_get_content_object(leaf_part);
		if (data) {
			int fd = 0;
			GMimeStream *out_stream = NULL;
			GMimeStream *src_stream = NULL;

			fd = open(content_path, O_WRONLY|O_CREAT, 0644);
			if (fd < 0) {
				EM_DEBUG_EXCEPTION("holder open failed :  holder is a filename that will be saved.");
				goto FINISH_OFF;
			}

			out_stream = g_mime_stream_fs_new(fd);
			src_stream = g_mime_data_wrapper_get_stream(data);

			if (src_stream)
				EM_DEBUG_LOG_DEV("Data length [%d]", g_mime_stream_length(src_stream));

			/*g_mime_data_wrapper_set_stream(data, out_stream);
			g_mime_data_wrapper_set_encoding(data, enc);*/

			g_mime_data_wrapper_write_to_stream(data, out_stream);
			if (out_stream) g_object_unref(out_stream);
			emcore_get_file_size(content_path, &real_size, NULL);
		}
		else
		{
			int fd = 0;
			fd = open(content_path, O_WRONLY|O_CREAT, 0644);
			if (fd < 0) {
				EM_DEBUG_EXCEPTION("holder open failed :  holder is a filename that will be saved.");
				goto FINISH_OFF;
			}
			close(fd);
		}
		/*Content - END*/

		ctype = g_mime_object_get_content_type(mobject);
		tmp_path = (char *)g_mime_content_type_get_parameter(ctype, "tmp_content_path");
		if (tmp_path) {
			g_remove(tmp_path);
		}

		if (content_disposition_type != ATTACHMENT && content_disposition_type != INLINE_ATTACHMENT) {
			/*TEXT*/
			EM_DEBUG_LOG("TEXT");

			if (ctype_type && g_ascii_strcasecmp(ctype_type, "text") == 0) {
				if (!ctype_charset || g_ascii_strcasecmp(ctype_charset, "X-UNKNOWN") == 0) {
					ctype_charset = "UTF-8";
				}

				if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, "plain") == 0) {
					EM_DEBUG_LOG("TEXT/PLAIN");

					char *file_content = NULL;
					int content_size = 0;

					if (emcore_get_content_from_file(content_path, &file_content, &content_size) != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("emcore_get_content_from_file failed");
					}

					if (file_content && content_size > 0) {
						char escape = 0x1b;
						char detector[25] = {0,};
						snprintf(detector, sizeof(detector), "%c$B", escape);
						if (g_strrstr(ctype_charset, "UTF-8") && g_strrstr(file_content, detector)) {
							ctype_charset = "ISO-2022-JP";
						}
					}

					EM_SAFE_FREE(file_content);
					EM_SAFE_FREE(cnt_info->text.plain_charset);
					EM_SAFE_FREE(cnt_info->text.plain);

					cnt_info->text.plain_charset = g_strdup(ctype_charset);
					cnt_info->text.plain = g_strdup(content_path);
				} else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, "html") == 0) {
					EM_DEBUG_LOG("TEXT/HTML");

					EM_SAFE_FREE(cnt_info->text.html_charset);
					EM_SAFE_FREE(cnt_info->text.html);

					cnt_info->text.html_charset = g_strdup(ctype_charset);
					cnt_info->text.html = g_strdup(content_path);
				}
			}
		} else if (content_disposition_type == INLINE_ATTACHMENT) {

			struct attachment_info *file = NULL;
			struct attachment_info *temp_file = cnt_info->inline_file;
			char *utf8_text = NULL;
			char *file_content = NULL;
			int file_size = 0;
			int err = EMAIL_ERROR_NONE;

			file = em_malloc(sizeof(struct attachment_info));
			if (file == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed...");
				goto FINISH_OFF;
			}

			file->type = content_disposition_type;
			if (disposition_filename) file->name = g_strdup(disposition_filename);
			else if (ctype_name) file->name = g_strdup(ctype_name);
			else if (content_id) file->name = g_strdup(content_id);
			file->content_id = g_strdup(content_id);

			if (file->name && g_strrstr(file->name, "/") != NULL) {
				char *tmp_ptr = file->name;
				int tmp_len = EM_SAFE_STRLEN(file->name);
				int tmpi = 0;
				for (tmpi=0; tmpi<tmp_len; tmpi++) {
					if (*(tmp_ptr+tmpi) == '/') {
						*(tmp_ptr+tmpi) = '_';
					}
				}
				EM_DEBUG_LOG_SEC("file->name[%s]", file->name);
			}

			if (!(utf8_text = g_mime_utils_header_decode_text(file->name))) {
				EM_DEBUG_EXCEPTION("g_mime_utils_header_decode_text failed [%d]", err);
			}
			EM_DEBUG_LOG_SEC("utf8_text : [%s]", utf8_text);

			if (utf8_text) {
				EM_SAFE_FREE(file->name);
				file->name = EM_SAFE_STRDUP(utf8_text);
			}
			EM_SAFE_FREE(utf8_text);

			/* check inline name duplication */
			char *modified_name= NULL;
			if (emcore_gmime_check_filename_duplication(file->name, cnt_info)) {
				modified_name = emcore_gmime_get_modified_filename_in_duplication(file->name);
				EM_SAFE_FREE(file->name);
				file->name = modified_name;
			}

			char *modified_name2 = NULL;
			emcore_unescape_from_url(file->name, &modified_name2);
			EM_DEBUG_LOG_SEC("file->name[%s] modified_name[%s]", file->name, modified_name2);
			EM_SAFE_FREE(file->name);
			file->name = modified_name2;

			/* cid replacement for inline attachment */
			if (emcore_get_content_from_file(cnt_info->text.html, &file_content, &file_size) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_get_content_from_file failed");
			}

			if (file_content && file_size > 0) {
				EM_DEBUG_LOG_SEC("content_id[%s] name[%s]", file->content_id, file->name);
				em_replace_string_ex(&file_content, "cid:", "");
				em_replace_string_ex(&file_content, file->content_id, file->name);

				file_size = EM_SAFE_STRLEN(file_content);
				if (emcore_set_content_to_file(file_content, cnt_info->text.html, file_size) != EMAIL_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("emcore_set_content_to_file failed");
				}
			}
			g_free(file_content); /* prevent 39110 */

			if(ctype_type && ctype_subtype) {
				char mime_type_buffer[128] = {0,};
				snprintf(mime_type_buffer, sizeof(mime_type_buffer), "%s/%s", ctype_type, ctype_subtype);
				file->attachment_mime_type = g_strdup(mime_type_buffer);
			}

			file->save = g_strdup(content_path);
			file->size = real_size;
			file->save_status = 1;

			while (temp_file && temp_file->next)
				temp_file = temp_file->next;

			if (temp_file == NULL)
				cnt_info->inline_file = file;
			else
				temp_file->next = file;
		} else if (content_disposition_type == ATTACHMENT) {

			struct attachment_info *file = NULL;
			struct attachment_info *temp_file = cnt_info->file;
			char *utf8_text = NULL;
			int err = EMAIL_ERROR_NONE;

			file = em_malloc(sizeof(struct attachment_info));
			if (file == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed...");
				goto FINISH_OFF;
			}

			file->type = content_disposition_type;
			if (disposition_filename) file->name = g_strdup(disposition_filename);
			else if (ctype_name) file->name = g_strdup(ctype_name);
			else if (content_id) file->name = g_strdup(content_id);
			else file->name = g_strdup("unknown-attachment");

			file->content_id = g_strdup(content_id);

			if (!(utf8_text = g_mime_utils_header_decode_text(file->name))) {
				EM_DEBUG_EXCEPTION("g_mime_utils_header_decode_text failed [%d]", err);
			}
			EM_DEBUG_LOG_SEC("utf8_text : [%s]", utf8_text);

			if (utf8_text) {
				EM_SAFE_FREE(file->name);
				file->name = EM_SAFE_STRDUP(utf8_text);
			}
			EM_SAFE_FREE(utf8_text);

			if(ctype_type && ctype_subtype) {
				char mime_type_buffer[128] = {0,};
				snprintf(mime_type_buffer, sizeof(mime_type_buffer), "%s/%s", ctype_type, ctype_subtype);
				file->attachment_mime_type = g_strdup(mime_type_buffer);
			}

			file->save = g_strdup(content_path);
			if (real_size == 0 && ctype_size)
				file->size = atoi(ctype_size);
			else
				file->size = real_size;
			file->save_status = 1;

			if (ctype_type && g_ascii_strcasecmp(ctype_type, "APPLICATION") == 0) {
				if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, MIME_SUBTYPE_DRM_OBJECT) == 0)
					file->drm = EMAIL_ATTACHMENT_DRM_OBJECT;
				else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, MIME_SUBTYPE_DRM_RIGHTS) == 0)
					file->drm = EMAIL_ATTACHMENT_DRM_RIGHTS;
				else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, MIME_SUBTYPE_DRM_DCF) == 0)
					file->drm = EMAIL_ATTACHMENT_DRM_DCF;
			}

			while (temp_file && temp_file->next)
				temp_file = temp_file->next;

			if (temp_file == NULL)
				cnt_info->file = file;
			else
				temp_file->next = file;
		}

		EM_SAFE_FREE(content_path);
	}

FINISH_OFF:

	EM_SAFE_FREE(content_path);
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void emcore_gmime_imap_parse_bodystructure_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data)
{
	EM_DEBUG_FUNC_BEGIN("parent[%p], part[%p], user_data[%p]", parent, part, user_data);

	int error = EMAIL_ERROR_NONE;
	struct _m_content_info *cnt_info = (struct _m_content_info *)user_data;

	if (GMIME_IS_MESSAGE_PART(part)) {
		GMimeMessage *message = NULL;
		GMimeContentType *msg_ctype = NULL;
		GMimeContentDisposition *msg_disposition = NULL;
		char *msg_ctype_type = NULL;
		char *msg_ctype_subtype = NULL;
		char *msg_ctype_size = NULL;
		char *msg_disposition_str = NULL;
		unsigned long msg_content_size = 0;
		EM_DEBUG_LOG("Message Part");

		message = g_mime_message_part_get_message((GMimeMessagePart *)part);
		if (!message) {
			EM_DEBUG_EXCEPTION("Message is NULL");
			//goto FINISH_OFF;
		}

		/*Content Type*/
		msg_ctype = g_mime_object_get_content_type(part);
		msg_ctype_type = (char *)g_mime_content_type_get_media_type(msg_ctype);
		msg_ctype_subtype = (char *)g_mime_content_type_get_media_subtype(msg_ctype);
		msg_ctype_size = (char *)g_mime_content_type_get_parameter(msg_ctype, "message_size");
		EM_DEBUG_LOG("Content-Type[%s/%s]", msg_ctype_type, msg_ctype_subtype);
		EM_DEBUG_LOG("Part.size.bytes[%s]", msg_ctype_size);

		if (msg_ctype_size) msg_content_size = atol(msg_ctype_size);
		cnt_info->total_mail_size += msg_content_size;
		/*Content Type - END*/

		/*Content Disposition*/
		msg_disposition = g_mime_object_get_content_disposition(part);
		if (msg_disposition) {
			msg_disposition_str = (char *)g_mime_content_disposition_get_disposition(msg_disposition);
		}
		EM_DEBUG_LOG("RFC822/Message Disposition[%s]", msg_disposition_str);
		/*Content Disposition - END*/

		if (msg_disposition_str && g_ascii_strcasecmp(msg_disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) {
			EM_DEBUG_LOG("RFC822/Message is ATTACHMENT");
			cnt_info->total_attachment_size += msg_content_size;
		}

		//g_mime_message_foreach(message, emcore_gmime_imap_parse_bodystructure_foreach_cb, user_data);
	} else if (GMIME_IS_MESSAGE_PARTIAL(part)) {
		EM_DEBUG_LOG("Partial Part");
		//TODO
	} else if (GMIME_IS_MULTIPART(part)) {
		EM_DEBUG_LOG("Multi Part");
		GMimeMultipart *multi_part = NULL;
		multi_part = (GMimeMultipart *)part;

		int multi_count = g_mime_multipart_get_count(multi_part);
		EM_DEBUG_LOG("Multi Part Count:%d", multi_count);
		EM_DEBUG_LOG("Boundary:%s\n\n", g_mime_multipart_get_boundary(multi_part));

	} else if (GMIME_IS_PART(part)) {
		EM_DEBUG_LOG("Part");
		int content_disposition_type = 0;
		char *content_id = NULL;
		char *content_location = NULL;
		char *disposition_str = NULL;
		char *disposition_filename = NULL;
		char *ctype_type = NULL;
		char *ctype_subtype = NULL;
		char *ctype_charset = NULL;
		char *ctype_name = NULL;
		char *ctype_size = NULL;
		unsigned long content_size = 0;

		GMimeContentType *ctype = NULL;
		GMimeContentDisposition *disposition = NULL;
		GMimePart *leaf_part = NULL;
		GMimeObject *mobject = (GMimeObject *)part;
		leaf_part = (GMimePart *)part;

		/*Content Type*/
		ctype = g_mime_object_get_content_type(mobject);
		ctype_type = (char *)g_mime_content_type_get_media_type(ctype);
		ctype_subtype = (char *)g_mime_content_type_get_media_subtype(ctype);
		ctype_charset = (char *)g_mime_content_type_get_parameter(ctype, "charset");
		ctype_name = (char *)g_mime_content_type_get_parameter(ctype, "name");
		ctype_size = (char *)g_mime_content_type_get_parameter(ctype, "part_size");
		if (EM_SAFE_STRLEN(ctype_name) == 0) ctype_name = NULL;
		EM_DEBUG_LOG("Content-Type[%s/%s]", ctype_type, ctype_subtype);
		EM_DEBUG_LOG("Content-Type-Charset[%s]", ctype_charset);
		EM_DEBUG_LOG("Content-Type-Name[%s]", ctype_name);
		EM_DEBUG_LOG("Part.size.bytes[%s]", ctype_size);

		if (ctype_size) content_size = atol(ctype_size);
		cnt_info->total_mail_size += content_size;
		/*Content Type - END*/

		/*Content Disposition*/
		disposition = g_mime_object_get_content_disposition(mobject);
		if (disposition) {
			disposition_str = (char *)g_mime_content_disposition_get_disposition(disposition);
			disposition_filename = (char *)g_mime_content_disposition_get_parameter(disposition, "filename");
			if (EM_SAFE_STRLEN(disposition_filename) == 0) disposition_filename = NULL;
		}
		EM_DEBUG_LOG("Disposition[%s]", disposition_str);
		EM_DEBUG_LOG_SEC("Disposition-Filename[%s]", disposition_filename);
		/*Content Disposition - END*/

		/*Content ID*/
		content_id = (char *)g_mime_object_get_content_id(mobject);
		EM_DEBUG_LOG_SEC("Content-ID:%s", content_id);

		/*Content Location*/
		content_location = (char *)g_mime_part_get_content_location(leaf_part);
		EM_DEBUG_LOG_SEC("Content-Location:%s", content_location);

		/*Figure out TEXT or ATTACHMENT(INLINE) */
		if (disposition_str && g_ascii_strcasecmp(disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) {
			content_disposition_type = ATTACHMENT;
			cnt_info->total_attachment_size += content_size;
			EM_DEBUG_LOG("ATTACHMENT");
		} else if ((content_id || content_location) && (ctype_name || disposition_filename)) {
			if (cnt_info->attachment_only) {
				content_disposition_type = ATTACHMENT;
				cnt_info->total_attachment_size += content_size;
				EM_DEBUG_LOG("ATTACHMENT");
			} else {
				content_disposition_type = INLINE_ATTACHMENT;
				cnt_info->total_body_size += content_size;
				EM_DEBUG_LOG("INLINE_ATTACHMENT");
			}
		} else {
			if (content_id || content_location) {
				if (g_ascii_strcasecmp(ctype_type, "text") == 0 &&
						(g_ascii_strcasecmp(ctype_subtype, "plain") == 0 || g_ascii_strcasecmp(ctype_subtype, "html") == 0)) {
					cnt_info->total_body_size += content_size;
					EM_DEBUG_LOG("TEXT");
				} else {
					if (cnt_info->attachment_only) {
						content_disposition_type = ATTACHMENT;
						cnt_info->total_attachment_size += content_size;
						EM_DEBUG_LOG("ATTACHMENT");
					} else {
						content_disposition_type = INLINE_ATTACHMENT;
						cnt_info->total_body_size += content_size;
						EM_DEBUG_LOG("INLINE_ATTACHMENT");
					}
				}
			} else {
				if (g_ascii_strcasecmp(ctype_type, "text") == 0 &&
						(g_ascii_strcasecmp(ctype_subtype, "plain") == 0 || g_ascii_strcasecmp(ctype_subtype, "html") == 0)) {
					cnt_info->total_body_size += content_size;
					EM_DEBUG_LOG("TEXT");
				} else {
					content_disposition_type = ATTACHMENT;
					cnt_info->total_attachment_size += content_size;
					EM_DEBUG_LOG("ATTACHMENT");
				}
			}
		}

		if (content_disposition_type != ATTACHMENT && content_disposition_type != INLINE_ATTACHMENT) {
			/*TEXT*/
			EM_DEBUG_LOG("TEXT");

			if (ctype_type && g_ascii_strcasecmp(ctype_type, "text") == 0) {
				char *tmp_file = NULL;

				if (!ctype_charset || g_ascii_strcasecmp(ctype_charset, "X-UNKNOWN") == 0) {
					ctype_charset = "UTF-8";
				}

				if (!emcore_get_temp_file_name(&tmp_file, &error) || !tmp_file) {
					EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed [%d]", error);
					EM_SAFE_FREE(tmp_file);
					goto FINISH_OFF;
				}

				if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, "plain") == 0) {
					EM_DEBUG_LOG("TEXT/PLAIN");
					cnt_info->text.plain_charset = g_strdup(ctype_charset);
					cnt_info->text.plain = tmp_file;
				}
				else if (ctype_subtype && g_ascii_strcasecmp(ctype_subtype, "html") == 0) {
					EM_DEBUG_LOG("TEXT/HTML");
					cnt_info->text.html_charset = g_strdup(ctype_charset);
					cnt_info->text.html = tmp_file;
				}
			}
		}
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END();
}


INTERNAL_FUNC void emcore_gmime_get_body_sections_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data)
{
	EM_DEBUG_FUNC_BEGIN("parent[%p], part[%p], user_data[%p]", parent, part, user_data);

	struct _m_content_info *cnt_info = (struct _m_content_info *)user_data;
	char sections[IMAP_MAX_COMMAND_LENGTH] = {0,};

	if (GMIME_IS_MESSAGE_PART(part)) {
		EM_DEBUG_LOG("Message Part");

		if (cnt_info->grab_type != (GRAB_TYPE_TEXT|GRAB_TYPE_ATTACHMENT)) {
			EM_DEBUG_LOG("grab_type is not GRAB_TYPE_TEXT|GRAB_TYPE_ATTACHMENT");
			goto FINISH_OFF;
		}

		GMimeMessage *message = NULL;
		GMimeContentDisposition *msg_disposition = NULL;
		GMimeContentType *msg_ctype = NULL;
		char *msg_disposition_str = NULL;
		char *msg_ctype_section = NULL;

		message = g_mime_message_part_get_message((GMimeMessagePart *)part);
		if (!message) {
			EM_DEBUG_EXCEPTION("Message is NULL");
			//goto FINISH_OFF;
		}

		/*Content Disposition*/
		msg_disposition = g_mime_object_get_content_disposition(part);
		if (msg_disposition) {
			msg_disposition_str = (char *)g_mime_content_disposition_get_disposition(msg_disposition);
		}
		EM_DEBUG_LOG("RFC822/Message Disposition[%s]", msg_disposition_str);
		/*Content Disposition - END*/

		if (!msg_disposition_str || g_ascii_strcasecmp(msg_disposition_str, GMIME_DISPOSITION_ATTACHMENT) != 0) {
			goto FINISH_OFF;
		}

		msg_ctype = g_mime_object_get_content_type(part);
		msg_ctype_section = (char *)g_mime_content_type_get_parameter(msg_ctype, "section");
		EM_DEBUG_LOG("section[%s]", msg_ctype_section);

		if (!msg_ctype_section) {
			EM_DEBUG_LOG("section is NULL");
			goto FINISH_OFF;
		}

		snprintf(sections, sizeof(sections), "BODY.PEEK[%s]", msg_ctype_section);

		EM_DEBUG_LOG("sections <%s>", sections);

		if (cnt_info->sections) {
			char *tmp_str = NULL;
			tmp_str = g_strconcat(cnt_info->sections, " ", sections, NULL);

			if (tmp_str) {
				EM_SAFE_FREE(cnt_info->sections);
				cnt_info->sections = tmp_str;
			}
		}
		else {
			cnt_info->sections = EM_SAFE_STRDUP(sections);
		}

		EM_DEBUG_LOG("sections <%s>", cnt_info->sections);
	} else if (GMIME_IS_MESSAGE_PARTIAL(part)) {
		EM_DEBUG_LOG("Partial Part");
	} else if (GMIME_IS_MULTIPART(part)) {
		EM_DEBUG_LOG("Multi Part");
	} else if (GMIME_IS_PART(part)) {
		EM_DEBUG_LOG("Part");
		int content_disposition_type = 0;
		char *content_id = NULL;
		char *content_location = NULL;
		char *disposition_str = NULL;
		char *disposition_filename = NULL;
		char *ctype_type = NULL;
		char *ctype_subtype = NULL;
		char *ctype_name = NULL;
		char *ctype_section = NULL;

		GMimeContentType *ctype = NULL;
		GMimeContentDisposition *disposition = NULL;
		GMimePart *leaf_part = NULL;
		GMimeObject *mobject = (GMimeObject *)part;
		leaf_part = (GMimePart *)part;

		/*Content Type*/
		ctype = g_mime_object_get_content_type(mobject);
		ctype_type = (char *)g_mime_content_type_get_media_type(ctype);
		ctype_subtype = (char *)g_mime_content_type_get_media_subtype(ctype);
		ctype_name = (char *)g_mime_content_type_get_parameter(ctype, "name");
		if (EM_SAFE_STRLEN(ctype_name) == 0) ctype_name = NULL;
		ctype_section = (char *)g_mime_content_type_get_parameter(ctype, "section");
		EM_DEBUG_LOG("Content-Type-Name[%s]", ctype_name);
		EM_DEBUG_LOG("Content-Type-Section[%s]", ctype_section);

		if (!ctype_section) {
			EM_DEBUG_LOG("section is NULL");
			goto FINISH_OFF;
		}
		/*Content Type - END*/

		/*Content Disposition*/
		disposition = g_mime_object_get_content_disposition(mobject);
		if (disposition) {
			disposition_str = (char *)g_mime_content_disposition_get_disposition(disposition);
			disposition_filename = (char *)g_mime_content_disposition_get_parameter(disposition, "filename");
			if (EM_SAFE_STRLEN(disposition_filename) == 0) disposition_filename = NULL;
		}
		EM_DEBUG_LOG("Disposition[%s]", disposition_str);
		EM_DEBUG_LOG_SEC("Disposition-Filename[%s]", disposition_filename);
		/*Content Disposition - END*/

		/*Content ID*/
		content_id = (char *)g_mime_object_get_content_id(mobject);
		EM_DEBUG_LOG_SEC("Content-ID:%s", content_id);

		/*Content Location*/
		content_location = (char *)g_mime_part_get_content_location(leaf_part);
		EM_DEBUG_LOG_SEC("Content-Location:%s", content_location);

		/*Figure out TEXT or ATTACHMENT(INLINE) */
		if (disposition_str && g_ascii_strcasecmp(disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) {
			content_disposition_type = ATTACHMENT;
			EM_DEBUG_LOG("ATTACHMENT");
		} else if ((content_id || content_location) && (ctype_name || disposition_filename)) {
			if (cnt_info->attachment_only) {
				content_disposition_type = ATTACHMENT;
				EM_DEBUG_LOG("ATTACHMENT");
			} else {
				content_disposition_type = INLINE_ATTACHMENT;
				EM_DEBUG_LOG("INLINE_ATTACHMENT");
			}
		} else {
			if (content_id || content_location) {
				if (g_ascii_strcasecmp(ctype_type, "text") == 0 &&
						(g_ascii_strcasecmp(ctype_subtype, "plain") == 0 || g_ascii_strcasecmp(ctype_subtype, "html") == 0)) {
					EM_DEBUG_LOG("TEXT");
				} else {
					if (cnt_info->attachment_only) {
						content_disposition_type = ATTACHMENT;
						EM_DEBUG_LOG("ATTACHMENT");
					} else {
						content_disposition_type = INLINE_ATTACHMENT;
						EM_DEBUG_LOG("INLINE_ATTACHMENT");
					}
				}
			} else {
				if (g_ascii_strcasecmp(ctype_type, "text") == 0 &&
						(g_ascii_strcasecmp(ctype_subtype, "plain") == 0 || g_ascii_strcasecmp(ctype_subtype, "html") == 0)) {
					EM_DEBUG_LOG("TEXT");
				} else {
					content_disposition_type = ATTACHMENT;
					EM_DEBUG_LOG("ATTACHMENT");
				}
			}
		}

		if (content_disposition_type == ATTACHMENT) {
			EM_DEBUG_LOG("ATTACHMENT");

			if (cnt_info->grab_type != (GRAB_TYPE_TEXT|GRAB_TYPE_ATTACHMENT))
				goto FINISH_OFF;
		}
		else {
			snprintf(sections, sizeof(sections), "BODY.PEEK[%s.MIME] BODY.PEEK[%s]", ctype_section, ctype_section);

			if (cnt_info->sections) {
				char *tmp_str = NULL;
				tmp_str = g_strconcat(cnt_info->sections, " ", sections, NULL);

				if (tmp_str) {
					EM_SAFE_FREE(cnt_info->sections);
					cnt_info->sections = tmp_str;
				}
			}
			else {
				cnt_info->sections = EM_SAFE_STRDUP(sections);
			}

			EM_DEBUG_LOG("sections <%s>", cnt_info->sections);
		}
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END();
}


INTERNAL_FUNC void emcore_gmime_get_attachment_section_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data)
{
	EM_DEBUG_FUNC_BEGIN("parent[%p], part[%p], user_data[%p]", parent, part, user_data);

	struct _m_content_info *cnt_info = (struct _m_content_info *)user_data;
	char sections[IMAP_MAX_COMMAND_LENGTH] = {0,};

	if (GMIME_IS_MESSAGE_PART(part)) {
		EM_DEBUG_LOG("Message Part");

		if (cnt_info->grab_type != GRAB_TYPE_ATTACHMENT) {
			EM_DEBUG_LOG("grab_type is not GRAB_TYPE_ATTACHMENT");
			goto FINISH_OFF;
		}

		GMimeMessage *message = NULL;
		GMimeContentDisposition *msg_disposition = NULL;
		GMimeContentType *msg_ctype = NULL;
		char *msg_disposition_str = NULL;
		char *msg_ctype_section = NULL;

		message = g_mime_message_part_get_message((GMimeMessagePart *)part);
		if (!message) {
			EM_DEBUG_EXCEPTION("Message is NULL");
			//goto FINISH_OFF;
		}

		/*Content Disposition*/
		msg_disposition = g_mime_object_get_content_disposition(part);
		if (msg_disposition) {
			msg_disposition_str = (char *)g_mime_content_disposition_get_disposition(msg_disposition);
		}
		EM_DEBUG_LOG("RFC822/Message Disposition[%s]", msg_disposition_str);
		/*Content Disposition - END*/

		if (!msg_disposition_str || g_ascii_strcasecmp(msg_disposition_str, GMIME_DISPOSITION_ATTACHMENT) != 0) {
			goto FINISH_OFF;
		}

		if (--cnt_info->file_no != 0)
			goto FINISH_OFF;

		msg_ctype = g_mime_object_get_content_type(part);
		msg_ctype_section = (char *)g_mime_content_type_get_parameter(msg_ctype, "section");
		EM_DEBUG_LOG("section[%s]", msg_ctype_section);

		if (!msg_ctype_section) {
			EM_DEBUG_LOG("section is NULL");
			goto FINISH_OFF;
		}

		snprintf(sections, sizeof(sections), "%s", msg_ctype_section);

		EM_DEBUG_LOG("sections <%s>", sections);

		if (!cnt_info->sections) {
			cnt_info->sections = EM_SAFE_STRDUP(sections);
		}

		EM_DEBUG_LOG("sections <%s>", cnt_info->sections);
	} else if (GMIME_IS_MESSAGE_PARTIAL(part)) {
		EM_DEBUG_LOG("Partial Part");
	} else if (GMIME_IS_MULTIPART(part)) {
		EM_DEBUG_LOG("Multi Part");
	} else if (GMIME_IS_PART(part)) {
		EM_DEBUG_LOG("Part");
		int content_disposition_type = 0;
		char *content_id = NULL;
		char *content_location = NULL;
		char *disposition_str = NULL;
		char *disposition_filename = NULL;
		char *ctype_type = NULL;
		char *ctype_subtype = NULL;
		char *ctype_name = NULL;
		char *ctype_section = NULL;

		GMimeContentType *ctype = NULL;
		GMimeContentDisposition *disposition = NULL;
		GMimePart *leaf_part = NULL;
		GMimeObject *mobject = (GMimeObject *)part;
		leaf_part = (GMimePart *)part;

		if (cnt_info->grab_type != GRAB_TYPE_ATTACHMENT) {
			EM_DEBUG_LOG("grab_type is not GRAB_TYPE_ATTACHMENT");
			goto FINISH_OFF;
		}

		/*Content Type*/
		ctype = g_mime_object_get_content_type(mobject);
		ctype_type = (char *)g_mime_content_type_get_media_type(ctype);
		ctype_subtype = (char *)g_mime_content_type_get_media_subtype(ctype);
		ctype_name = (char *)g_mime_content_type_get_parameter(ctype, "name");
		if (EM_SAFE_STRLEN(ctype_name) == 0) ctype_name = NULL;
		ctype_section = (char *)g_mime_content_type_get_parameter(ctype, "section");
		EM_DEBUG_LOG("Content-Type-Name[%s]", ctype_name);
		EM_DEBUG_LOG("Content-Type-Section[%s]", ctype_section);

		if (!ctype_section) {
			EM_DEBUG_LOG("section is NULL");
			goto FINISH_OFF;
		}
		/*Content Type - END*/

		/*Content Disposition*/
		disposition = g_mime_object_get_content_disposition(mobject);
		if (disposition) {
			disposition_str = (char *)g_mime_content_disposition_get_disposition(disposition);
			disposition_filename = (char *)g_mime_content_disposition_get_parameter(disposition, "filename");
			if (EM_SAFE_STRLEN(disposition_filename) == 0) disposition_filename = NULL;
		}
		EM_DEBUG_LOG("Disposition[%s]", disposition_str);
		EM_DEBUG_LOG("Disposition-Filename[%s]", disposition_filename);
		/*Content Disposition - END*/

		/*Content ID*/
		content_id = (char *)g_mime_object_get_content_id(mobject);
		EM_DEBUG_LOG_SEC("Content-ID:%s", content_id);

		/*Content Location*/
		content_location = (char *)g_mime_part_get_content_location(leaf_part);
		EM_DEBUG_LOG_SEC("Content-Location:%s", content_location);

		/*Figure out TEXT or ATTACHMENT(INLINE) */
		if (disposition_str && g_ascii_strcasecmp(disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) {
			content_disposition_type = ATTACHMENT;
			EM_DEBUG_LOG("ATTACHMENT");
		} else if ((content_id || content_location) && (ctype_name || disposition_filename)) {
			if (cnt_info->attachment_only) {
				content_disposition_type = ATTACHMENT;
				EM_DEBUG_LOG("ATTACHMENT");
			} else {
				content_disposition_type = INLINE_ATTACHMENT;
				EM_DEBUG_LOG("INLINE_ATTACHMENT");
			}
		} else {
			if (content_id || content_location) {
				if (g_ascii_strcasecmp(ctype_type, "text") == 0 &&
						(g_ascii_strcasecmp(ctype_subtype, "plain") == 0 || g_ascii_strcasecmp(ctype_subtype, "html") == 0)) {
					EM_DEBUG_LOG("TEXT");
				} else {
					if (cnt_info->attachment_only) {
						content_disposition_type = ATTACHMENT;
						EM_DEBUG_LOG("ATTACHMENT");
					} else {
						content_disposition_type = INLINE_ATTACHMENT;
						EM_DEBUG_LOG("INLINE_ATTACHMENT");
					}
				}
			} else {
				if (g_ascii_strcasecmp(ctype_type, "text") == 0 &&
						(g_ascii_strcasecmp(ctype_subtype, "plain") == 0 || g_ascii_strcasecmp(ctype_subtype, "html") == 0)) {
					EM_DEBUG_LOG("TEXT");
				} else {
					content_disposition_type = ATTACHMENT;
					EM_DEBUG_LOG("ATTACHMENT");
				}
			}
		}

		if (content_disposition_type == ATTACHMENT) {
			EM_DEBUG_LOG("ATTACHMENT");

			if (--cnt_info->file_no != 0)
				goto FINISH_OFF;

			snprintf(sections, sizeof(sections), "%s", ctype_section);

			if (!cnt_info->sections) {
				cnt_info->sections = EM_SAFE_STRDUP(sections);
			}

			EM_DEBUG_LOG("sections <%s>", cnt_info->sections);
		}
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void emcore_gmime_search_section_foreach_cb(GMimeObject *parent, GMimeObject *part, gpointer user_data)
{
	EM_DEBUG_FUNC_BEGIN();

	search_section *search_info = (search_section *)user_data;
	GMimeContentType *ctype = NULL;
        char buf[255] = {0};
	char *ctype_section = NULL;

	if (!search_info) {
		EM_DEBUG_LOG("search_info is NULL");
		goto FINISH_OFF;
	}

	if (!part) {
		EM_DEBUG_LOG("part is NULL");
		goto FINISH_OFF;
	}

	if (GMIME_IS_MESSAGE_PART(part)) {
		EM_DEBUG_LOG("Message Part");
	} else if (GMIME_IS_MESSAGE_PARTIAL(part)) {
		EM_DEBUG_LOG("Partial Part");
	} else if (GMIME_IS_MULTIPART(part)) {
		EM_DEBUG_LOG("Multi Part");
	} else if (GMIME_IS_PART(part)) {
		EM_DEBUG_LOG("Part");
	}

	ctype = g_mime_object_get_content_type(part);
	ctype_section = (char *)g_mime_content_type_get_parameter(ctype, "section");
	EM_DEBUG_LOG("section[%s]", ctype_section);

	if (!ctype_section) {
		EM_DEBUG_LOG("section is NULL");
		goto FINISH_OFF;
	}

        SNPRINTF(buf, sizeof(buf), "%s.MIME", ctype_section);

	if (g_ascii_strcasecmp(ctype_section, search_info->section) == 0) {
		EM_DEBUG_LOG("found section");
		if (!(search_info->section_object)) search_info->section_object = part;
        } else if (g_ascii_strcasecmp(search_info->section, buf) == 0) {
                EM_DEBUG_LOG("Mime header");
                if (!(search_info->section_object)) search_info->section_object = part;
        }

FINISH_OFF:

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void emcore_gmime_construct_multipart (GMimeMultipart *multipart,
		BODY *body, const char *spec, int *total_mail_size)
{
	EM_DEBUG_FUNC_BEGIN();
	PART *part = NULL;
	GMimeObject *subpart = NULL;
	GMimeMultipart *sub_multipart = NULL;
	GMimeMessagePart *sub_messagepart = NULL;
	GMimeMessage *sub_message = NULL;
	GMimePart *sub_part = NULL;
	char *subspec = NULL;
	char *id = NULL;
	char *section = NULL;
	int i = 1;

	if (!body || !multipart || !spec) {
		EM_DEBUG_EXCEPTION("body[%p], multipart[%p], spec[%p]", body, multipart, spec);
		return;
	}

	subspec = g_alloca (strlen(spec) + 14);
	id = g_stpcpy(subspec, spec);
	*id++ = '.';

	EM_DEBUG_LOG("constructing a %s/%s part (%s)", body_types[body->type],
		body->subtype, spec);

	/* checkout boundary */
	if (body->parameter) {
		PARAMETER *param = body->parameter;
		while(param) {
			EM_DEBUG_LOG("Content-Type Parameter: attribute[%s], value[%s]", param->attribute, param->value);
			param = param->next;
		}
	}

	part = body->nested.part;

	while (part != NULL) {
		sprintf (id, "%d", i++);

		if (EM_SAFE_STRLEN(subspec) > 2)
			section = EM_SAFE_STRDUP(subspec+2);

		EM_DEBUG_LOG("constructing a %s/%s part (%s/%s)", body_types[part->body.type],
			part->body.subtype, subspec, section);

		if (part->body.type == TYPEMULTIPART) {
			/*multipart*/
			char *subtype = g_ascii_strdown(part->body.subtype, -1);
			sub_multipart = g_mime_multipart_new_with_subtype(subtype);
			EM_SAFE_FREE(subtype);
			emcore_gmime_construct_multipart(sub_multipart, &(part->body), subspec, total_mail_size);

			subpart = GMIME_OBJECT(sub_multipart);
		} else if (part->body.type == TYPEMESSAGE) {
			/*message/rfc822 or message/news*/
			BODY *nested_body = NULL;
			MESSAGE *nested_message = NULL;
			char *subtype = g_ascii_strdown(part->body.subtype, -1);
			sub_messagepart = g_mime_message_part_new(subtype);
			EM_SAFE_FREE(subtype);

			subpart = GMIME_OBJECT(sub_messagepart);

			if (part->body.size.bytes > 0) {
				char size_str[sizeof(unsigned long)*8+1] = {0,};
				snprintf(size_str, sizeof(size_str), "%lu", part->body.size.bytes);
				g_mime_object_set_content_type_parameter(subpart, "message_size", size_str);
			}

			nested_message = part->body.nested.msg;
			if (nested_message) nested_body = nested_message->body;

			if (!emcore_gmime_construct_mime_part_with_bodystructure(nested_body, &sub_message, subspec, NULL)) {
				EM_DEBUG_EXCEPTION("emcore_gmime_construct_mime_part_with_bodystructure failed");
			} else {
				g_mime_message_part_set_message(sub_messagepart, sub_message);
				if (sub_message) g_object_unref(sub_message);
			}
		} else {
			/*other parts*/
			char *type = g_ascii_strdown(body_types[part->body.type], -1);
			char *subtype = g_ascii_strdown(part->body.subtype, -1);
			sub_part = g_mime_part_new_with_type(type, subtype);
			EM_SAFE_FREE(type);
			EM_SAFE_FREE(subtype);
			subpart = GMIME_OBJECT(sub_part);

			if (total_mail_size != NULL) {
				*total_mail_size = *total_mail_size + (int)part->body.size.bytes;
				EM_DEBUG_LOG("body.size.bytes [%d]", part->body.size.bytes);
				EM_DEBUG_LOG("total_mail_size [%d]", *total_mail_size);
			}

			/* encoding */
			if (part->body.encoding <= 5) {
				EM_DEBUG_LOG("Content Encoding: %s", body_encodings[part->body.encoding]);
				GMimeContentEncoding encoding = GMIME_CONTENT_ENCODING_DEFAULT;
				switch(part->body.encoding) {
				case ENC7BIT:
					encoding = GMIME_CONTENT_ENCODING_7BIT;
					break;
				case ENC8BIT:
					encoding = GMIME_CONTENT_ENCODING_8BIT;
					break;
				case ENCBINARY:
					encoding = GMIME_CONTENT_ENCODING_BINARY;
					break;
				case ENCBASE64:
					encoding = GMIME_CONTENT_ENCODING_BASE64;
					break;
				case ENCQUOTEDPRINTABLE:
					encoding = GMIME_CONTENT_ENCODING_QUOTEDPRINTABLE;
					break;
				case ENCOTHER:
					encoding = GMIME_CONTENT_ENCODING_DEFAULT;
					break;

				default:
					break;
				}
				g_mime_part_set_content_encoding(sub_part, encoding);
			}

			/* content description */
			if (part->body.description) {
				EM_DEBUG_LOG("Content-Description: %s", part->body.description);
				g_mime_part_set_content_description(sub_part, part->body.description);
			}

			/* content location */
			if (part->body.location) {
				EM_DEBUG_LOG_SEC("Content-Location: %s", part->body.location);
				g_mime_part_set_content_location(sub_part, part->body.location);
			}

			/* md5 */
			if (part->body.md5) {
				EM_DEBUG_LOG("Content-MD5: %s", part->body.md5);
				g_mime_part_set_content_md5(sub_part, part->body.md5);
			}

			if (part->body.size.bytes > 0) {
				char size_str[sizeof(unsigned long)*8+1] = {0,};
				snprintf(size_str, sizeof(size_str), "%lu", part->body.size.bytes);
				g_mime_object_set_content_type_parameter(subpart, "part_size", size_str);
			}
		}

		/* content type */
		if (part->body.parameter) {
			PARAMETER *param = part->body.parameter;
			while(param) {
			    EM_DEBUG_LOG_SEC("Content-Type Parameter: attribute[%s], value[%s]", param->attribute, param->value);
				if (param->attribute || param->value)
					g_mime_object_set_content_type_parameter(subpart, param->attribute, param->value);
				param = param->next;
			}
		}
		g_mime_object_set_content_type_parameter(subpart, "section", section);

		/* content disposition */
		if (part->body.disposition.type) {
			EM_DEBUG_LOG("Content-Disposition: %s", part->body.disposition.type);
			char *disposition_type = g_ascii_strdown(part->body.disposition.type, -1);
			g_mime_object_set_disposition(subpart, disposition_type);
			g_free(disposition_type);
		}

		if (part->body.disposition.parameter) {
			PARAMETER *param = part->body.disposition.parameter;
			while(param) {
			    EM_DEBUG_LOG_SEC("Content-Disposition Parameter: attribute[%s], value[%s]", param->attribute, param->value);
				if (param->attribute || param->value)
					g_mime_object_set_content_disposition_parameter(subpart, param->attribute, param->value);
				param = param->next;
			}
		}

		/* content id */
		if (part->body.id) {
			EM_DEBUG_LOG_SEC("Content-ID: %s", part->body.id);
			int i = 0;
			char *cid = EM_SAFE_STRDUP(part->body.id);
			g_strstrip(cid);

			while (EM_SAFE_STRLEN(cid) > 0 && cid[i] != '\0') {
				if (cid[i] == '<' || cid[i] == '>')
					cid[i] = ' ';
				i++;
			}

			g_strstrip(cid);
			EM_DEBUG_LOG_DEV("Content-ID stripped: %s", cid);
			g_mime_object_set_content_id(subpart, cid);
			EM_SAFE_FREE(cid);
		}

		g_mime_multipart_add (multipart, subpart);

		if (subpart) {
			g_object_unref (subpart);
			subpart = NULL;
		}

		EM_SAFE_FREE(section);

		part = part->next;
	}

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void emcore_gmime_construct_part (GMimePart *part,
		BODY *body, const char *spec, int *total_mail_size)
{
	EM_DEBUG_FUNC_BEGIN();
	GMimeObject *part_object = NULL;

	if (!body || !part || !spec) {
		EM_DEBUG_EXCEPTION("body[%p], part[%p] spec[%p]", body, part, spec);
		return;
	}

	part_object = GMIME_OBJECT(part);

	if (total_mail_size != NULL) {
		*total_mail_size = (int)(body->size.bytes);
		EM_DEBUG_LOG("total_mail_size [%d]", *total_mail_size);
	}

	GMimeContentType *ctype = NULL;
	char *ctype_type = NULL;
	char *ctype_subtype = NULL;
	ctype = g_mime_object_get_content_type(GMIME_OBJECT(part));
	ctype_type = (char *)g_mime_content_type_get_media_type(ctype);
	ctype_subtype = (char *)g_mime_content_type_get_media_subtype(ctype);

	/* Type-Subtype */
	if (g_strcmp0(ctype_type, "text") == 0 && g_strcmp0(ctype_subtype, "plain") == 0 &&
			body->type >= (unsigned int)0 && body->subtype) {
		GMimeContentType *content_type = NULL;
		char *type = g_ascii_strdown(body_types[body->type], -1);
		char *subtype = g_ascii_strdown(body->subtype, -1);
		EM_DEBUG_LOG("Content Type: [%s/%s]", type, subtype);

		content_type = g_mime_content_type_new(type, subtype);
		g_mime_object_set_content_type(GMIME_OBJECT(part), content_type);
		g_object_unref(content_type);

		EM_SAFE_FREE(type);
		EM_SAFE_FREE(subtype);
	}

	/* encoding */
	if (body->encoding <= 5) {
		EM_DEBUG_LOG("Content Encoding: %s", body_encodings[body->encoding]);
		GMimeContentEncoding encoding = GMIME_CONTENT_ENCODING_DEFAULT;
		switch(body->encoding) {
		case ENC7BIT:
			encoding = GMIME_CONTENT_ENCODING_7BIT;
			break;
		case ENC8BIT:
			encoding = GMIME_CONTENT_ENCODING_8BIT;
			break;
		case ENCBINARY:
			encoding = GMIME_CONTENT_ENCODING_BINARY;
			break;
		case ENCBASE64:
			encoding = GMIME_CONTENT_ENCODING_BASE64;
			break;
		case ENCQUOTEDPRINTABLE:
			encoding = GMIME_CONTENT_ENCODING_QUOTEDPRINTABLE;
			break;
		case ENCOTHER:
			encoding = GMIME_CONTENT_ENCODING_DEFAULT;
			break;

		default:
			break;
		}
		g_mime_part_set_content_encoding(part, encoding);
	}

	/* content description */
	if (body->description) {
		EM_DEBUG_LOG("Content-Description: %s", body->description);
		g_mime_part_set_content_description(part, body->description);
	}

	/* content location */
	if (body->location) {
		EM_DEBUG_LOG_SEC("Content-Location: %s", body->location);
		g_mime_part_set_content_location(part, body->location);
	}

	/* md5 */
	if (body->md5) {
		EM_DEBUG_LOG("Content-MD5: %s", body->md5);
		g_mime_part_set_content_md5(part, body->md5);
	}

	/* content type */
	if (body->parameter) {
		PARAMETER *param = body->parameter;
		while(param) {
			EM_DEBUG_LOG("Content-Type Parameter: attribute[%s], value[%s]", param->attribute, param->value);
			if (param->attribute || param->value)
				g_mime_object_set_content_type_parameter(part_object, param->attribute, param->value);
			param = param->next;
		}
	}

	if (body->size.bytes > 0) {
		char size_str[sizeof(unsigned long)*8+1] = {0,};
		snprintf(size_str, sizeof(size_str), "%lu", body->size.bytes);
		g_mime_object_set_content_type_parameter(part_object, "part_size", size_str);
	}

	if (g_strcmp0(spec, "1") == 0)
		g_mime_object_set_content_type_parameter(part_object, "section", spec);
	else {
		if (EM_SAFE_STRLEN(spec) > 2) {
			char *tmp = EM_SAFE_STRDUP(spec+2);
			g_mime_object_set_content_type_parameter(part_object, "section", tmp);
			EM_SAFE_FREE(tmp);
		}
	}

	/* content disposition */
	if (body->disposition.type) {
		EM_DEBUG_LOG("Content-Disposition: %s", body->disposition.type);
		char *disposition_type = g_ascii_strdown(body->disposition.type, -1);
		g_mime_object_set_disposition(part_object, disposition_type);
		g_free(disposition_type);
	}

	if (body->disposition.parameter) {
		PARAMETER *param = body->disposition.parameter;
		while(param) {
			EM_DEBUG_LOG("Content-Disposition Parameter: attribute[%s], value[%s]", param->attribute, param->value);
			if (param->attribute || param->value)
				g_mime_object_set_content_disposition_parameter(part_object, param->attribute, param->value);
			param = param->next;
		}
	}

	/* content id */
	if (body->id) {
		EM_DEBUG_LOG("Content-ID: %s", body->id);
		int i = 0;
		char *cid = EM_SAFE_STRDUP(body->id);
		if (cid) g_strstrip(cid);

		while (strlen(cid) > 0 && cid[i] != '\0') {
			if (cid[i] == '<' || cid[i] == '>')
				cid[i] = ' ';
			i++;
		}

		g_strstrip(cid);
		EM_DEBUG_LOG_DEV("Content-ID stripped: %s", cid);
		g_mime_object_set_content_id(part_object, cid);
		EM_SAFE_FREE(cid);
	}

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int emcore_gmime_construct_mime_part_with_bodystructure(BODY *mbody,
		GMimeMessage **message, const char *spec, int *total_mail_size)
{
	EM_DEBUG_FUNC_BEGIN();

	GMimeMessage *message1 = NULL;
	GMimeObject *mime_part = NULL;
	GMimeMultipart *multipart = NULL;
	GMimePart *singlepart = NULL;
	int total_size = 0;
	int ret = FALSE;

	if (!mbody || !message) {
		EM_DEBUG_EXCEPTION("body[%p], message[%p]", mbody, message);
		return ret;
	}

	message1 = g_mime_message_new(FALSE);

	/* Construct mime_part of GMimeMessage */
	if (g_ascii_strcasecmp(body_types[mbody->type], "multipart") == 0) {
		/* checkout boundary */
		int boundary_ok = 0;
		if (mbody->parameter) {
			PARAMETER *param = mbody->parameter;
			while(param) {
				EM_DEBUG_LOG("Content-Type Parameter: attribute[%s], value[%s]", param->attribute, param->value);
				if (g_ascii_strcasecmp(param->attribute, "boundary") == 0 && param->value)
					boundary_ok = 1;
				param = param->next;
			}
		}

		if (!boundary_ok) {
			EM_DEBUG_EXCEPTION("boundary is not exist in bodystructure");
			goto FINISH_OFF;
		}

		char *subtype = g_ascii_strdown(mbody->subtype, -1);
		multipart = g_mime_multipart_new_with_subtype(subtype);

		/* Fill up mime part of message1 using bodystructure info */
		emcore_gmime_construct_multipart(multipart, mbody, spec, &total_size);

		mime_part = GMIME_OBJECT(multipart);
		EM_SAFE_FREE(subtype);
	}
	else {
		char *type = g_ascii_strdown(body_types[mbody->type], -1);
		char *subtype = g_ascii_strdown(mbody->subtype, -1);
		singlepart = g_mime_part_new_with_type(type, subtype);

		emcore_gmime_construct_part(singlepart, mbody, spec, &total_size);

		mime_part = GMIME_OBJECT(singlepart);
		EM_SAFE_FREE(type);
		EM_SAFE_FREE(subtype);
	}

	g_mime_message_set_mime_part(message1, mime_part);
	if (mime_part) g_object_unref(mime_part);

	ret = TRUE;

FINISH_OFF:

	if (total_mail_size)
		*total_mail_size = total_size;

	if (message)
		*message = message1;

	EM_DEBUG_FUNC_END();

	return ret;
}

INTERNAL_FUNC int emcore_gmime_get_body_sections_from_message(GMimeMessage *message,
		struct _m_content_info *cnt_info, char **sections_to_fetch)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = FALSE;
	GMimePartIter *iter1 = NULL;
	GMimeObject *part_tmp = NULL;
	char *part_path = NULL;
	char sections[IMAP_MAX_COMMAND_LENGTH] = {0,};

	iter1 = g_mime_part_iter_new((GMimeObject *)message);

	if (!g_mime_part_iter_is_valid(iter1)) {
		EM_DEBUG_EXCEPTION("Part iterator is not valid");
		goto FINISH_OFF;
	}

	do {
		part_tmp = g_mime_part_iter_get_current(iter1);
		if (part_tmp && GMIME_IS_PART(part_tmp)) {
			GMimeObject *mobject = (GMimeObject *)part_tmp;
			GMimeContentDisposition *disposition = NULL;
			char *disposition_str = NULL;

			/*Content Disposition*/
			disposition = g_mime_object_get_content_disposition(mobject);
			if (disposition) {
				disposition_str = (char *)g_mime_content_disposition_get_disposition(disposition);
			}
			EM_DEBUG_LOG("Disposition[%s]", disposition_str);
			/*Content Disposition - END*/

			part_path = g_mime_part_iter_get_path(iter1);

			if (cnt_info->grab_type == (GRAB_TYPE_TEXT | GRAB_TYPE_ATTACHMENT)) {
				char t[100] = {0,};
				snprintf(t, sizeof(t), "BODY.PEEK[%s] ", part_path);
				if (EM_SAFE_STRLEN(sections) + EM_SAFE_STRLEN(t) < sizeof(sections) - 1) {
					strcat(sections, t);
				}
				else {
					EM_DEBUG_EXCEPTION("Too many body parts. IMAP command may cross 2000bytes.");
					goto FINISH_OFF;
				}
			}
			else if (cnt_info->grab_type == GRAB_TYPE_TEXT) {
				if (disposition_str && g_ascii_strcasecmp(disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) {
					EM_DEBUG_LOG("ATTACHMENT");
				} else {
					char t[100] = {0,};
					snprintf(t, sizeof(t), "BODY.PEEK[%s] ", part_path);
					if (EM_SAFE_STRLEN(sections) + EM_SAFE_STRLEN(t) < sizeof(sections) - 1) {
						strcat(sections, t);
					}
					else {
						EM_DEBUG_EXCEPTION("Too many body parts. IMAP command may cross 2000bytes.");
						goto FINISH_OFF;
					}
				}
			}

			EM_SAFE_FREE(part_path);
		}
	} while (g_mime_part_iter_next(iter1));

	if (iter1) {
		g_mime_part_iter_free(iter1);
		iter1 = NULL;
	}

	if (EM_SAFE_STRLEN(sections) <= 0 || EM_SAFE_STRLEN(sections) >= IMAP_MAX_COMMAND_LENGTH-1) {
		EM_DEBUG_EXCEPTION("Too many body parts. IMAP command may cross 2000bytes.");
		goto FINISH_OFF;
	}

	if (sections[EM_SAFE_STRLEN(sections)-1] == ' ')
		sections[EM_SAFE_STRLEN(sections)-1] = '\0';

	EM_DEBUG_LOG("sections <%s>", sections);

	if (g_strlcpy(*sections_to_fetch, sections, IMAP_MAX_COMMAND_LENGTH) > 0)
		ret = TRUE;

FINISH_OFF:

	if (iter1) {
		g_mime_part_iter_free(iter1);
		iter1 = NULL;
	}

	EM_SAFE_FREE(part_path);

	EM_DEBUG_FUNC_END();

	return ret;
}

INTERNAL_FUNC int emcore_gmime_get_attachment_section_from_message(GMimeMessage *message,
		struct _m_content_info *cnt_info, int nth, char **section_to_fetch)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = FALSE;
	int count = nth;
	GMimePartIter *iter1 = NULL;
	GMimeObject *part_tmp = NULL;
	char *part_path = NULL;
	char sections[IMAP_MAX_COMMAND_LENGTH] = {0,};

	if (nth <= 0) {
		EM_DEBUG_EXCEPTION("parameter nth is not valid");
		goto FINISH_OFF;
	}

	iter1 = g_mime_part_iter_new((GMimeObject *)message);

	if (!g_mime_part_iter_is_valid(iter1)) {
		EM_DEBUG_EXCEPTION("Part iterator is not valid");
		goto FINISH_OFF;
	}

	do {
		part_tmp = g_mime_part_iter_get_current(iter1);
		if (part_tmp && GMIME_IS_PART(part_tmp)) {
			GMimeObject *mobject = (GMimeObject *)part_tmp;
			GMimeContentDisposition *disposition = NULL;
			char *disposition_str = NULL;

			/*Content Disposition*/
			disposition = g_mime_object_get_content_disposition(mobject);
			if (disposition) {
				disposition_str = (char *)g_mime_content_disposition_get_disposition(disposition);
			}
			EM_DEBUG_LOG("Disposition[%s]", disposition_str);
			/*Content Disposition - END*/

			part_path = g_mime_part_iter_get_path(iter1);

			if (disposition_str && g_ascii_strcasecmp(disposition_str, GMIME_DISPOSITION_ATTACHMENT) == 0) {
				EM_DEBUG_LOG("ATTACHMENT");
				count--;

				if (count == 0) {
					char t[100] = {0,};
					snprintf(t, sizeof(t), "%s", part_path);
					if (EM_SAFE_STRLEN(sections) + EM_SAFE_STRLEN(t) < sizeof(sections) - 1) {
						strcat(sections, t);
					}
					else {
						EM_DEBUG_EXCEPTION("Too many body parts. IMAP command may cross 2000bytes.");
						goto FINISH_OFF;
					}
				}
			}

			EM_SAFE_FREE(part_path);
		}
	} while (g_mime_part_iter_next(iter1));

	if (iter1) {
		g_mime_part_iter_free(iter1);
		iter1 = NULL;
	}

	EM_DEBUG_LOG("sections <%s>", sections);

	if (g_strlcpy(*section_to_fetch, sections, IMAP_MAX_COMMAND_LENGTH) > 0)
		ret = TRUE;

FINISH_OFF:

	if (iter1) {
		g_mime_part_iter_free(iter1);
		iter1 = NULL;
	}

	EM_SAFE_FREE(part_path);

	EM_DEBUG_FUNC_END();

	return ret;
}

static int emcore_gmime_get_section_n_bodysize(char *response, char *section, int *body_size)
{
	char *p = NULL;
	char *s = NULL;
	int size = 0;

	if ((p = strstr(response, "BODY[")) /* || (p = strstr(s + 1, "BODY["))*/) {

		p += strlen("BODY[");
		s = p;

		while (*s != ']')
			s++;

		*s = '\0';
		strcpy(section, p);

		/* if (strcmp(section, p)) {
					err = EMAIL_ERROR_INVALID_RESPONSE;
					goto FINISH_OFF;
		}*/

		p = strstr(s+1, " {");
		if (p) {
			p += strlen(" {");
			s = p;

			while (isdigit(*s))
				s++;

			*s = '\0';

			size = atoi(p);
			*body_size = size;

			/* sending progress noti to application.
			1. mail_id
			2. file_id
			3. bodysize
			*/
		} else {
			return FALSE;
		}
	} else {
		return FALSE;
	}

	return TRUE;
}

INTERNAL_FUNC int emcore_gmime_fetch_imap_body_sections(MAILSTREAM *stream, int msg_uid, int mail_id,
		struct _m_content_info *cnt_info, GMimeMessage *message, int event_handle, int auto_download, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], cnt_info[%p], err_code[%p]",
			stream, msg_uid, cnt_info, err_code);

	int err = EMAIL_ERROR_NONE;
	int ret = FALSE;

	IMAPLOCAL *imaplocal = NULL;

	char tag[16] = {0,};
	char command[IMAP_MAX_COMMAND_LENGTH+100] = {0,};
	char section[16] = {0,};
	char *response = NULL;

	int server_response_yn = 0;
	int body_size = 0;

	char *buf = NULL;

	unsigned char encoded[DOWNLOAD_MAX_BUFFER_SIZE] = {0,};
	unsigned char test_buffer[LOCAL_MAX_BUFFER_SIZE] = {0,};

	int total = 0;
	int flag_first_write = 1;
	char *tag_position = NULL;

	int part_header = 0;
	int download_interval = 0;
	int download_total_size = 0;
	int downloaded_size = 0;
	int download_progress = 0;
	int last_notified_download_size = 0;

	GMimeDataWrapper *content = NULL;
	GMimeStream *content_stream = NULL;
	GMimeObject *mime_object = NULL;
	GMimePart *mime_part = NULL;
	GMimeMessagePart *mime_message_part = NULL;
	//GMimePartIter *mime_iter = NULL;
	search_section *search_info = NULL;

	imaplocal = stream->local;

	if (!imaplocal || !imaplocal->netstream) {
		EM_DEBUG_EXCEPTION("invalid IMAP4 stream detected...");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!cnt_info || !cnt_info->sections) {
		EM_DEBUG_EXCEPTION("invalid parameter detected...");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
	SNPRINTF(command, sizeof(command), "%s UID FETCH %d (%s)\015\012", tag, msg_uid, cnt_info->sections);
	EM_DEBUG_LOG("command <%s>", command);

	/* send command : get msgno/uid for all message */
	if (!net_sout(imaplocal->netstream, command, (int)EM_SAFE_STRLEN(command))) {
		EM_DEBUG_EXCEPTION("net_sout failed...");
		err = EMAIL_ERROR_CONNECTION_BROKEN;
		goto FINISH_OFF;
	}

	while (imaplocal->netstream) {

		/*  receive response */
		if (!(response = net_getline(imaplocal->netstream))) {
			EM_DEBUG_EXCEPTION("net_getline failed...");
			err = EMAIL_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("response :%s", response);

		if (strstr(response, "BODY[")) {

			if (!server_response_yn) { /* start of response */
				if (response[0] != '*') {
					err = EMAIL_ERROR_INVALID_RESPONSE;
					EM_DEBUG_EXCEPTION("Start of response doesn't contain *");
					goto FINISH_OFF;
				}
				server_response_yn = 1;
			}

			part_header = 0;
			flag_first_write = 1;
			total = 0;
			memset(encoded, 0x00, sizeof(encoded));

			if (!emcore_gmime_get_section_n_bodysize(response, section, &body_size)) {
				EM_DEBUG_EXCEPTION("emcore_get_section_body_size failed [%d]", err);
				err = EMAIL_ERROR_INVALID_RESPONSE;
				goto FINISH_OFF;
			}
			EM_DEBUG_LOG("section :%s, body_size :%d", section, body_size);

			/*if (GMIME_IS_MULTIPART (message->mime_part)) {
				mime_iter = g_mime_part_iter_new((GMimeObject *)message);

				if (!g_mime_part_iter_is_valid(mime_iter)) {
					EM_DEBUG_EXCEPTION("Part iterator is not valid");
					goto FINISH_OFF;
				}

				if (g_mime_part_iter_jump_to(mime_iter, section)) {
					EM_DEBUG_LOG("g_mime_part_iter_jump_to: %s", section);
					mime_object = g_mime_part_iter_get_current(mime_iter);
					if (!mime_object) {
						EM_DEBUG_EXCEPTION("g_mime_part_iter_get_current failed");
						goto FINISH_OFF;
					}
					mime_part = GMIME_PART(mime_object);
				} else {
					EM_DEBUG_LOG("g_mime_part_iter_jump_to: failed to jump to %s", section);
					goto FINISH_OFF;
				}

				if (mime_iter) {
					g_mime_part_iter_free(mime_iter);
					mime_iter = NULL;
				}
			} else if (GMIME_IS_PART (message->mime_part)) {
				mime_object = message->mime_part;
				mime_part = GMIME_PART(mime_object);
			}*/

			if (search_info) {
				EM_SAFE_FREE(search_info->section);
				EM_SAFE_FREE(search_info);
			}

			if (!(search_info = em_malloc(sizeof(search_section)))) { /* prevent */
				EM_DEBUG_EXCEPTION("em_malloc failed...");
				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			search_info->section = EM_SAFE_STRDUP(section);
			g_mime_message_foreach(message, emcore_gmime_search_section_foreach_cb, (gpointer)search_info);

			if (!(search_info->section_object)) {
				EM_DEBUG_EXCEPTION("section search failed");
				goto FINISH_OFF;
			}

			mime_message_part = NULL;
			mime_part = NULL;
			mime_object = search_info->section_object;
			if (GMIME_IS_MESSAGE_PART(mime_object)) {
				if (strcasestr(section, "MIME"))
					part_header = 1;

        		mime_message_part = GMIME_MESSAGE_PART(mime_object);
			} else if (GMIME_IS_PART(mime_object)) {
				if (strcasestr(section, "MIME"))
					part_header = 1;

				mime_part = GMIME_PART(mime_object);
			} else {
				EM_DEBUG_EXCEPTION("invalid mime part type");
				goto FINISH_OFF;
			}

			if (!part_header) {
				if (!emcore_get_temp_file_name(&buf, &err) || !buf) {
					EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed [%d]", err);
					goto FINISH_OFF;
				}

				g_mime_object_set_content_type_parameter(mime_object, "tmp_content_path", buf);

				if (event_handle > 0)
					FINISH_OFF_IF_EVENT_CANCELED (err, event_handle);

				if (cnt_info->grab_type == GRAB_TYPE_TEXT) {
					if (cnt_info->total_body_size > body_size) {
						EM_DEBUG_LOG("Multipart body size is [%d]", cnt_info->total_body_size);
						if (!auto_download) {
							if (!emcore_notify_network_event(NOTI_DOWNLOAD_MULTIPART_BODY, mail_id, buf, cnt_info->total_body_size, 0))
								EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>> ");
						}

						download_interval =  cnt_info->total_body_size * DOWNLOAD_NOTI_INTERVAL_PERCENT / 100;
						download_total_size = cnt_info->total_body_size;
					}
					else {
						if (!auto_download) {
							if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_START, mail_id, buf, body_size, 0))
								EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>> ");
						}

						download_interval =  body_size * DOWNLOAD_NOTI_INTERVAL_PERCENT / 100;
						download_total_size = body_size;
					}
				}

				if (cnt_info->grab_type == (GRAB_TYPE_TEXT | GRAB_TYPE_ATTACHMENT)) {
					if (!auto_download) {
						if (!emcore_notify_network_event(NOTI_DOWNLOAD_MULTIPART_BODY, mail_id, buf, cnt_info->total_mail_size, 0))
							EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>> ");
					}

					download_interval =  cnt_info->total_mail_size * DOWNLOAD_NOTI_INTERVAL_PERCENT / 100;
					download_total_size = cnt_info->total_mail_size;
				}

				if (download_interval > DOWNLOAD_NOTI_INTERVAL_SIZE) {
					download_interval = DOWNLOAD_NOTI_INTERVAL_SIZE;
				}

				if (body_size < DOWNLOAD_MAX_BUFFER_SIZE) {
					if (net_getbuffer(imaplocal->netstream, (long)body_size, (char *)encoded) <= 0) {
						EM_DEBUG_EXCEPTION("net_getbuffer failed...");
						err = EMAIL_ERROR_NO_RESPONSE;
						goto FINISH_OFF;
					}

					if (GMIME_IS_PART(mime_object) && mime_part) {
						content_stream = g_mime_stream_mem_new_with_buffer((const char *)encoded, EM_SAFE_STRLEN(encoded));
						//parser = g_mime_parser_new_with_stream(content_stream);
						content = g_mime_data_wrapper_new_with_stream(content_stream, mime_part->encoding);
						if (content_stream) g_object_unref (content_stream);
						g_mime_part_set_content_object(mime_part, content);
						if (content) g_object_unref(content);
					}
					else if (GMIME_IS_MESSAGE_PART(mime_object) && mime_message_part) {
						FILE *fp = NULL;
						int encoded_len = EM_SAFE_STRLEN((char *)encoded);

						err = em_fopen(buf, "wb+", &fp);
						if (err != EMAIL_ERROR_NONE) {
							EM_DEBUG_EXCEPTION_SEC("em_fopen failed - %s", buf);
							goto FINISH_OFF;
						}

						if (encoded_len > 0 && fwrite(encoded, encoded_len, 1, fp) == 0) {
							EM_DEBUG_EXCEPTION("Error Occured while writing. fwrite() failed");
							err = EMAIL_ERROR_SYSTEM_FAILURE;
							fclose(fp);
							goto FINISH_OFF;
						}

						if (fp != NULL)
							fclose(fp);
					}

					downloaded_size += EM_SAFE_STRLEN((char *)encoded);
					EM_DEBUG_LOG("IMAP4 downloaded body size [%d]", downloaded_size);
					EM_DEBUG_LOG("IMAP4 total body size [%d]", download_total_size);
					EM_DEBUG_LOG("IMAP4 download interval [%d]", download_interval);

					/* In some situation, total_encoded_len includes the length of dummy bytes.
					 * So it might be greater than body_size */

					download_progress = 100 * downloaded_size / download_total_size;

					EM_DEBUG_LOG("DOWNLOADING STATUS NOTIFY:Total[%d]/[%d] = %d %% Completed",
							downloaded_size, download_total_size, download_progress);

					if (cnt_info->grab_type == GRAB_TYPE_TEXT && !auto_download) {
						if (cnt_info->total_body_size > body_size) {
							if (!emcore_notify_network_event(NOTI_DOWNLOAD_MULTIPART_BODY, mail_id, buf, download_total_size, downloaded_size))
								EM_DEBUG_EXCEPTION(" emcore_notify_network_event [NOTI_DOWNLOAD_MULTIPART_BODY] Failed >>>>");
						}
						else {
							if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_START, mail_id, buf, download_total_size, downloaded_size))
								EM_DEBUG_EXCEPTION(" emcore_notify_network_event [NOTI_DOWNLOAD_BODY_START] Failed >>>>");
						}
					}
				} else {
					int remain_body_size = body_size;
					int x = 0;
					int nsize = 0;
					int fd = 0;
					total = 0;

					if (mime_part && mime_part->encoding == GMIME_CONTENT_ENCODING_BASE64)
						x = (sizeof(encoded)/78)*78; /* to solve base64 decoding pro */
					else
						x = sizeof(encoded)-1;

					memset(test_buffer, 0x00, sizeof(test_buffer));
					while (remain_body_size && (total <body_size)) {

						memset(test_buffer, 0x00, sizeof(test_buffer));
						while ((total != body_size) && remain_body_size && ((EM_SAFE_STRLEN((char *)test_buffer) + x) < sizeof(test_buffer))) {
							memset(encoded, 0x00, sizeof(encoded));

							if (net_getbuffer (imaplocal->netstream, (long)x, (char *)encoded) <= 0) {
								EM_DEBUG_EXCEPTION("net_getbuffer failed...");
								err = EMAIL_ERROR_NO_RESPONSE;
								goto FINISH_OFF;
							}

							nsize = EM_SAFE_STRLEN((char *)encoded);
							remain_body_size = remain_body_size - nsize;
							strncat((char *)test_buffer, (char *)encoded, nsize);
							total = total + nsize;
							downloaded_size += nsize;

							EM_DEBUG_LOG("total/body_size [%d/%d]", total, body_size);

							if (!(remain_body_size/x) && remain_body_size%x)
								x = remain_body_size%x;

							//EM_DEBUG_LOG("IMAP4  - %d ", _imap4_last_notified_body_size);
							EM_DEBUG_LOG("IMAP4 download interval [%d]", download_interval);
							EM_DEBUG_LOG("IMAP4 downloaded size [%d]", downloaded_size);
							EM_DEBUG_LOG("IMAP4 total body size [%d]", download_total_size);

							if (((last_notified_download_size + download_interval) <= downloaded_size)
								|| (downloaded_size >= download_total_size)) {

								/*  In some situation, total_encoded_len includes the length of dummy bytes.
								 * So it might be greater than body_size */

								if (downloaded_size > download_total_size)
									downloaded_size = download_total_size;

								last_notified_download_size = downloaded_size;

								if (download_total_size != 0) download_progress = 100*downloaded_size/download_total_size;
								EM_DEBUG_LOG("DOWNLOADING STATUS NOTIFY :Total[%d] / [%d] = %d %% Completed.",
										downloaded_size, download_total_size, download_progress);

								if (!auto_download) {
									if (cnt_info->total_body_size > body_size) {
										if (!emcore_notify_network_event(NOTI_DOWNLOAD_MULTIPART_BODY, mail_id, buf, download_total_size, downloaded_size))
											EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_DOWNLOAD_MULTIPART_BODY] Failed >>>>");
									} else {
										if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_START, mail_id, buf, download_total_size, downloaded_size))
											EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_DOWNLOAD_BODY_START] Failed >>>>");
									}
								}
							}
						}

						if (flag_first_write == 1) {
							FILE *fp = NULL;
							int encoded_len = EM_SAFE_STRLEN((char *)test_buffer);

							err = em_fopen(buf, "wb+", &fp);
							if (err != EMAIL_ERROR_NONE) {
								EM_DEBUG_EXCEPTION_SEC("em_fopen failed - %s", buf);
								goto FINISH_OFF;
							}

							if (encoded_len > 0 && fwrite(test_buffer, encoded_len, 1, fp) == 0) {
								EM_DEBUG_EXCEPTION("Error Occured while writing. fwrite(\"%s\") failed", test_buffer);
								err = EMAIL_ERROR_SYSTEM_FAILURE;
								fclose (fp); /* prevent */
								goto FINISH_OFF;
							}

							if (fp != NULL)
								fclose(fp);

							flag_first_write = 0;
						} else {
							FILE *fp = NULL;
							int encoded_len = EM_SAFE_STRLEN((char *)test_buffer);

							err = em_fopen(buf, "ab+", &fp);
							if (err != EMAIL_ERROR_NONE) {
								EM_DEBUG_EXCEPTION_SEC("em_fopen failed - %s", buf);
								goto FINISH_OFF;
							}

							if (encoded_len > 0 && fwrite(test_buffer, encoded_len, 1, fp) == 0) {
								EM_DEBUG_EXCEPTION("Error Occured while writing. fwrite(\"%s\") failed", test_buffer);
								err = EMAIL_ERROR_SYSTEM_FAILURE;
								fclose (fp); /* prevent */
								goto FINISH_OFF;
							}

							if (fp != NULL)
								fclose(fp);
						}
						EM_DEBUG_LOG("%d has been written", EM_SAFE_STRLEN((char *)test_buffer));
					}

					if (GMIME_IS_PART(mime_object) && mime_part) {
						err = em_open(buf, O_RDONLY, 0, &fd);
						if (err != EMAIL_ERROR_NONE) {
							EM_DEBUG_EXCEPTION("holder open failed : holder is a filename that will be saved.");
							goto FINISH_OFF;
						}

						content_stream = g_mime_stream_fs_new(fd);
						//parser = g_mime_parser_new_with_stream(content_stream);
						content = g_mime_data_wrapper_new_with_stream(content_stream, mime_part->encoding);
						if (content_stream) g_object_unref (content_stream);
						g_mime_part_set_content_object(mime_part, content);
						if (content) g_object_unref(content);
					}
				}
			} else {
                                EM_DEBUG_LOG("MIME header");

                                char *file_name = NULL;

                                GMimeObject *object_header = NULL;
                                GMimeParser *parser_header = NULL;
                                GMimeContentType *ctype_header = NULL;
                                GMimeContentDisposition *disposition_header = NULL;

                                if (net_getbuffer(imaplocal->netstream, (long)body_size, (char *)encoded) <= 0) {
                                        EM_DEBUG_EXCEPTION("net_getbuffer failed...");
                                        err = EMAIL_ERROR_NO_RESPONSE;
                                        goto FINISH_OFF;
                                }

                                EM_DEBUG_LOG_DEV("Data : [%s]", encoded);

                                content_stream = g_mime_stream_mem_new_with_buffer((const char *)encoded, EM_SAFE_STRLEN(encoded));

                                parser_header = g_mime_parser_new_with_stream(content_stream);
                                if (content_stream) g_object_unref (content_stream);

                                object_header = g_mime_parser_construct_part(parser_header);
                                if (parser_header) g_object_unref(parser_header);

                                /* Content type */
                                ctype_header = g_mime_object_get_content_type(object_header);
                                file_name = (char *)g_mime_content_type_get_parameter(ctype_header, "name");
                                EM_DEBUG_LOG_DEV("Content name : [%s]", file_name);

                                if (file_name == NULL) {
                                        /* Content Disposition */
                                        disposition_header = g_mime_object_get_content_disposition(object_header);
                                        file_name = (char *)g_mime_content_disposition_get_parameter(disposition_header, "filename");
                                        EM_DEBUG_LOG_DEV("Disposition name : [%s]", file_name);
                                }

                                /* Replace the file name (Becase the server sometimes send the invalid name in bodystructure) */
                                if (mime_part && file_name) g_mime_part_set_filename(mime_part, file_name);

                                if (object_header) g_object_unref(object_header);
                                if (ctype_header) g_object_unref(ctype_header);
                                if (disposition_header) g_object_unref(disposition_header);
                        }

			EM_SAFE_FREE(buf);
		}
		else if ((tag_position = g_strrstr(response, tag))) /*  end of response */ {
			if (!strncmp(tag_position + EM_SAFE_STRLEN(tag) + 1, "OK", 2)) {
				EM_SAFE_FREE(response);
			}
			else {		/* 'NO' or 'BAD */
				err = EMAIL_ERROR_IMAP4_FETCH_UID_FAILURE;
				goto FINISH_OFF;
			}
			break;
		}
		else if (!strcmp(response, ")")) {
		}

		EM_SAFE_FREE (response);
	}

	ret = TRUE;

FINISH_OFF:

	/*if (mime_iter) {
		g_mime_part_iter_free(mime_iter);
		mime_iter = NULL;
	}*/

	if (search_info) {
		EM_SAFE_FREE(search_info->section);
		EM_SAFE_FREE(search_info);
	}

	EM_SAFE_FREE(buf);

	EM_SAFE_FREE(response);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

INTERNAL_FUNC int emcore_gmime_fetch_imap_attachment_section(MAILSTREAM *stream,
		int mail_id, int uid, int nth, struct _m_content_info *cnt_info,
		GMimeMessage *message, int auto_download, int event_handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], uid[%d], err_code[%p]", stream, uid, err_code);

	int ret = FALSE;
	int err = EMAIL_ERROR_NONE;

	IMAPLOCAL *imaplocal = NULL;

	char tag[16] = {0,};
	char command[64] = {0,};
	char *response = NULL;
	unsigned char encoded[DOWNLOAD_MAX_BUFFER_SIZE] = {0, };
	unsigned char test_buffer[LOCAL_MAX_BUFFER_SIZE] = {0, };

	int flag_first_write = TRUE;
	int preline_len = 0;
	int nskip = 0;
	char *concat_encoded = NULL;
	char *new_response = NULL;
	char *tag_position = NULL;
	char *tmp_file = NULL;

	int body_size = 0;
	int total = 0;
	int server_response_yn = 0;
	int download_noti_interval = 0;
	int download_total_size = 0;
	int downloaded_size = 0;
	int download_progress = 0;
	int last_notified_download_size = 0;

	GMimeDataWrapper *content = NULL;
	GMimeStream *content_stream = NULL;
	GMimeObject *mime_object = NULL;
	GMimePart *mime_part = NULL;
	//GMimePartIter *mime_iter = NULL;
	GMimeMessagePart *mime_message_part = NULL;
	search_section *search_info = NULL;

	if (!stream || !cnt_info || !message) {
		EM_DEBUG_EXCEPTION_SEC("stream[%p], section[%s], cnt_info[%p], message[%p]",
				stream, cnt_info, message);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!cnt_info->sections) {
		EM_DEBUG_EXCEPTION("section is NULL");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	imaplocal = stream->local;

	if (!imaplocal->netstream) {
		EM_DEBUG_EXCEPTION("invalid IMAP4 stream detected... %p", imaplocal->netstream);
		err = EMAIL_ERROR_INVALID_STREAM;
		goto FINISH_OFF;
	}

	memset(tag, 0x00, sizeof(tag));
	memset(command, 0x00, sizeof(command));

	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
	SNPRINTF(command, sizeof(command), "%s UID FETCH %d BODY.PEEK[%s]\015\012", tag, uid, cnt_info->sections);
	EM_DEBUG_LOG("[IMAP4] >>> [%s]", command);

	/* send command : get msgno/uid for all message */
	if (!net_sout(imaplocal->netstream, command, (int)EM_SAFE_STRLEN(command))) {
		EM_DEBUG_EXCEPTION("net_sout failed...");
		err = EMAIL_ERROR_CONNECTION_BROKEN;
		goto FINISH_OFF;
	}

	char *p_stream = NULL;
	char *p_content = NULL;

	while (imaplocal->netstream) {

		/*don't delete the comment. several threads including event thread call this func
		if (!emcore_check_thread_status()) {
			EM_DEBUG_LOG("Canceled...");
			imaplocal->netstream = NULL;
			err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}*/

		/* receive response */
		if (!(response = net_getline(imaplocal->netstream))) {
			EM_DEBUG_EXCEPTION("net_getline failed...");
			err = EMAIL_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}

#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG("recv[%s]", response);
#endif

		/* start of response */
		if (response[0] == '*' && !server_response_yn) {

			p_stream = strstr(response, "BODY[");

			if (!p_stream) {
				err = EMAIL_ERROR_INVALID_RESPONSE;
				goto FINISH_OFF;
			}

			server_response_yn = 1;
			p_stream += strlen("BODY[");
			p_content = p_stream;

			while (*p_content != ']')
				p_content++;

			*p_content = '\0';

			/* check correct section */
			if (g_strcmp0(cnt_info->sections, p_stream) != 0) {
				EM_DEBUG_LOG("Invalid response of section");
				err = EMAIL_ERROR_INVALID_RESPONSE;
				goto FINISH_OFF;
			}

			/* get body size */
			p_stream = strstr(p_content+1, " {");
			if (p_stream) {
				p_stream += strlen(" {");
				p_content = p_stream;

				while (isdigit(*p_content))
					p_content++;

				*p_content = '\0';

				body_size = atoi(p_stream);
			} else {	/* no body length is replied */
				/* seek the termination of double quot */
				p_stream = strstr(p_content+1, " \"");
				if (p_stream) {
					char *t = NULL;
					p_stream += strlen(" \"");
					t = strstr(p_stream, "\"");
					if (t) {
						body_size = t - p_stream;
						*t = '\0';
						EM_DEBUG_LOG("Body : start[%p] end[%p] : body[%s]", p_stream, t, p_stream);
						/* need to decode */
						EM_SAFE_FREE(response);
						response = EM_SAFE_STRDUP(p_stream);
					} else {
						err = EMAIL_ERROR_INVALID_RESPONSE;
						goto FINISH_OFF;
					}
				} else {
					err = EMAIL_ERROR_INVALID_RESPONSE;
					goto FINISH_OFF;
				}
			}

			/*if (GMIME_IS_MULTIPART (message->mime_part)) {
				mime_iter = g_mime_part_iter_new((GMimeObject *)message);

				if (!g_mime_part_iter_is_valid(mime_iter)) {
					EM_DEBUG_EXCEPTION("Part iterator is not valid");
					goto FINISH_OFF;
				}

				if (g_mime_part_iter_jump_to (mime_iter, section)) {
					EM_DEBUG_LOG("g_mime_part_iter_jump_to: %s", section);
					mime_object = g_mime_part_iter_get_current (mime_iter);
					if (!mime_object) {
						EM_DEBUG_EXCEPTION("g_mime_part_iter_get_current failed");
						goto FINISH_OFF;
					}
					mime_part = GMIME_PART(mime_object);
				} else {
					EM_DEBUG_LOG("g_mime_part_iter_jump_to: failed to jump to %s", section);
					goto FINISH_OFF;
				}

				if (mime_iter) {
					g_mime_part_iter_free(mime_iter);
					mime_iter = NULL;
				}
			} else if (GMIME_IS_PART (message->mime_part)) {
				mime_object = message->mime_part;
				mime_part = GMIME_PART(mime_object);
			}*/

			if (search_info) {
				EM_SAFE_FREE(search_info->section);
				EM_SAFE_FREE(search_info);
			}

			if (!(search_info = em_malloc(sizeof(search_section)))) { /* prevent */
				EM_DEBUG_EXCEPTION("em_malloc failed...");
				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			search_info->section = EM_SAFE_STRDUP(cnt_info->sections);
			g_mime_message_foreach(message, emcore_gmime_search_section_foreach_cb, (gpointer)search_info);

			if (!(search_info->section_object)) {
				EM_DEBUG_EXCEPTION("section search failed");
				goto FINISH_OFF;
			}

			mime_message_part = NULL;
			mime_part = NULL;
			mime_object = search_info->section_object;
			if (GMIME_IS_MESSAGE_PART(mime_object)) {
				mime_message_part = GMIME_MESSAGE_PART(mime_object);
			} else if (GMIME_IS_PART(mime_object)) {
				mime_part = GMIME_PART(mime_object);
			} else {
				EM_DEBUG_EXCEPTION("invalid mime part type");
				goto FINISH_OFF;
			}

			if (!emcore_get_temp_file_name(&tmp_file, &err) || !tmp_file) {
				EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed [%d]", err);
				goto FINISH_OFF;
			}

			g_mime_object_set_content_type_parameter(mime_object, "tmp_content_path", tmp_file);

			EM_DEBUG_LOG("Attachment number [%d]", nth);
			if (!auto_download) {
				if (!emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_START, mail_id, tmp_file, nth, 0))
					EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_DOWNLOAD_ATTACH_START] Failed >>>>");
			}

			download_noti_interval = body_size * DOWNLOAD_NOTI_INTERVAL_PERCENT / 100;
			download_total_size = body_size;

			if (download_noti_interval > DOWNLOAD_NOTI_INTERVAL_SIZE) {
				download_noti_interval = DOWNLOAD_NOTI_INTERVAL_SIZE;
			}

			/* remove new lines */
			do {
				EM_SAFE_FREE(response);
				if (!(response = net_getline(imaplocal->netstream))) {
					EM_DEBUG_EXCEPTION("net_getline failed...");
					err = EMAIL_ERROR_INVALID_RESPONSE;
					goto FINISH_OFF;
				}

				EM_DEBUG_LOG("response:%s", response);
				if (EM_SAFE_STRLEN(response) == 0) {
					EM_DEBUG_LOG("Skip newline !!");
					nskip++;
				} else {
					EM_SAFE_FREE(new_response); /* detected by valgrind */
					new_response = g_strconcat(response, "\r\n", NULL);
					EM_SAFE_FREE(response);
					break;
				}
			} while(1);

			preline_len = EM_SAFE_STRLEN(new_response);
			EM_DEBUG_LOG("preline_len : %d", preline_len);

			if (body_size <= 0 || preline_len <= 0)
				continue;

			if ((body_size - preline_len - nskip*2) <= 0) {
				/* 1 line content */

				if (GMIME_IS_PART(mime_object) && mime_part) {
					char *tmp_str = NULL;
					tmp_str = g_strndup(new_response, body_size);
					if (tmp_str == NULL) tmp_str = g_strdup(new_response);

					content_stream = g_mime_stream_mem_new_with_buffer((const char *)tmp_str, EM_SAFE_STRLEN(tmp_str));
					//parser = g_mime_parser_new_with_stream(content_stream);
					content = g_mime_data_wrapper_new_with_stream(content_stream, mime_part->encoding);
					if (content_stream) g_object_unref (content_stream);
					g_mime_part_set_content_object(mime_part, content);
					if (content) g_object_unref(content);
					EM_SAFE_FREE(tmp_str);
				}
				else if (GMIME_IS_MESSAGE_PART(mime_object) && mime_message_part) {
					FILE *fp = NULL;
					int response_len = 0;
					char *tmp_str = NULL;

					tmp_str = g_strndup(new_response, body_size);
					if (tmp_str == NULL) tmp_str = g_strdup(new_response);

					response_len = EM_SAFE_STRLEN((char *)tmp_str);

					err = em_fopen(tmp_file, "wb+", &fp);
					if (err != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION_SEC("em_fopen failed - %s", tmp_file);
						EM_SAFE_FREE(tmp_str);
						goto FINISH_OFF;
					}

					if (response_len > 0 && fwrite(tmp_str, response_len, 1, fp) == 0) {
						EM_DEBUG_EXCEPTION("Error Occured while writing. fwrite() failed");
						err = EMAIL_ERROR_SYSTEM_FAILURE;
						fclose(fp);
						EM_SAFE_FREE(tmp_str);
						goto FINISH_OFF;
					}

					if (fp != NULL)
						fclose(fp);

					EM_SAFE_FREE(tmp_str);
				}

				EM_SAFE_FREE(new_response);

				total = body_size;
				EM_DEBUG_LOG("current total = %d", total);
				downloaded_size += total;

				EM_DEBUG_LOG("DOWNLOADING STATUS NOTIFY : received[%d] / total_size[%d] = %d %% Completed",
						downloaded_size, download_total_size, (int)((float)downloaded_size / (float)download_total_size * 100.0));

				if (((last_notified_download_size + download_noti_interval) <= downloaded_size) || (downloaded_size >= download_total_size)) {

					if (downloaded_size > download_total_size)
						downloaded_size = download_total_size;

					last_notified_download_size = downloaded_size;

					download_progress = (int)((float)downloaded_size / (float)download_total_size * 100.0);

					if (!auto_download) {
						if (download_total_size && !emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_START, mail_id, tmp_file, nth, download_progress))
							EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_DOWNLOAD_ATTACH_START] Failed >>>>");
					}
				}
			}
			else if ((body_size < DOWNLOAD_MAX_BUFFER_SIZE) && (body_size - preline_len - nskip*2 < DOWNLOAD_MAX_BUFFER_SIZE)) {
				memset(encoded, 0x00, sizeof(encoded));
				if (net_getbuffer(imaplocal->netstream, body_size - preline_len - nskip*2, (char *)encoded) <= 0) {
					EM_DEBUG_EXCEPTION("net_getbuffer failed...");
					err = EMAIL_ERROR_NO_RESPONSE;
					goto FINISH_OFF;
				}

				concat_encoded = g_strconcat(new_response, encoded, NULL);
				memset(encoded, 0x00, sizeof(encoded));
				memcpy(encoded, concat_encoded, EM_SAFE_STRLEN(concat_encoded));
				EM_SAFE_FREE(concat_encoded);
				EM_SAFE_FREE(new_response);

				if (GMIME_IS_PART(mime_object) && mime_part) {
					content_stream = g_mime_stream_mem_new_with_buffer((const char *)encoded, EM_SAFE_STRLEN(encoded));
					//parser = g_mime_parser_new_with_stream(content_stream);
					content = g_mime_data_wrapper_new_with_stream(content_stream, mime_part->encoding);
					if (content_stream) g_object_unref (content_stream);
					g_mime_part_set_content_object(mime_part, content);
					if (content) g_object_unref(content);
				}
				else if (GMIME_IS_MESSAGE_PART(mime_object) && mime_message_part) {
					FILE *fp = NULL;
					int encoded_len = EM_SAFE_STRLEN((char *)encoded);

					err = em_fopen(tmp_file, "wb+", &fp);
					if (err != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION_SEC("em_fopen failed - %s", tmp_file);
						goto FINISH_OFF;
					}

					if (encoded_len > 0 && fwrite(encoded, encoded_len, 1, fp) == 0) {
						EM_DEBUG_EXCEPTION("Error Occured while writing. fwrite() failed");
						err = EMAIL_ERROR_SYSTEM_FAILURE;
						fclose(fp);
						goto FINISH_OFF;
					}

					if (fp != NULL)
						fclose(fp);
				}

				total = EM_SAFE_STRLEN((char *)encoded);
				EM_DEBUG_LOG("total = %d", total);
				downloaded_size += total;

				EM_DEBUG_LOG("DOWNLOADING STATUS NOTIFY : received[%d] / total_size[%d] = %d %% Completed",
						downloaded_size, download_total_size, (int)((float)downloaded_size / (float)download_total_size * 100.0));

				if (((last_notified_download_size + download_noti_interval) <= downloaded_size) || (downloaded_size >= download_total_size)) {

					if (downloaded_size > download_total_size)
						downloaded_size = download_total_size;

					last_notified_download_size = downloaded_size;

					download_progress = (int)((float)downloaded_size / (float)download_total_size * 100.0);

					if (!auto_download) {
						if (download_total_size && !emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_START, mail_id, tmp_file, nth, download_progress))
							EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_DOWNLOAD_ATTACH_START] Failed >>>>");
					}
				}
			}
			else {
				int remain_body_size = body_size - preline_len - nskip*2;
				int x = 0;
				int nsize = 0;
				int fd = 0;
				total += preline_len + nskip*2;
				downloaded_size += preline_len + nskip*2;

				memset(test_buffer, 0x00, sizeof(test_buffer));

				while (remain_body_size > 0 && (total < body_size)) {

					memset(test_buffer, 0x00, sizeof(test_buffer));

                                        if (remain_body_size < DOWNLOAD_MAX_BUFFER_SIZE)
                                                x = remain_body_size;
                                        else if (mime_part && mime_part->encoding == GMIME_CONTENT_ENCODING_BASE64)
						x = (sizeof(encoded)/preline_len)*preline_len; /* to solve base64 decoding pro */
					else
						x = sizeof(encoded)-1;

					if (new_response) {
						strncat((char *)test_buffer, (char *)new_response, preline_len);
						EM_SAFE_FREE(new_response);
					}

					while (remain_body_size > 0 && (total < body_size) && ((EM_SAFE_STRLEN((char *)test_buffer) + x) < sizeof(test_buffer))) {
						if (event_handle > 0)
							FINISH_OFF_IF_EVENT_CANCELED (err, event_handle);

						memset(encoded, 0x00, sizeof(encoded));

						if (net_getbuffer(imaplocal->netstream, (long)x, (char *)encoded) <= 0) {
							EM_DEBUG_EXCEPTION("net_getbuffer failed...");
							err = EMAIL_ERROR_NO_RESPONSE;
							goto FINISH_OFF;
						}

						nsize = EM_SAFE_STRLEN((char *)encoded);
						remain_body_size = remain_body_size - nsize;
						strncat((char *)test_buffer, (char *)encoded, nsize);
						total = total + nsize;
						downloaded_size += nsize;
						EM_DEBUG_LOG("nsize : %d", nsize);
						EM_DEBUG_LOG("remain_body_size : %d", remain_body_size);
						EM_DEBUG_LOG("total : %d", total);
						EM_DEBUG_LOG("imap_received_body_size : %d", downloaded_size);

						if (!(remain_body_size/x) && remain_body_size%x)
							x = remain_body_size%x;

						EM_DEBUG_LOG("DOWNLOADING STATUS NOTIFY : received[%d] / total_size[%d] = %d %% Completed",
								downloaded_size, download_total_size, (int)((float)downloaded_size / (float)download_total_size * 100.0));

						if (((last_notified_download_size + download_noti_interval) <= downloaded_size) || (downloaded_size >= download_total_size)) {

							if (downloaded_size > download_total_size)
								downloaded_size = download_total_size;

							last_notified_download_size = downloaded_size;

							download_progress = (int)((float)downloaded_size / (float)download_total_size * 100.0);

							if (!auto_download) {
								if (download_total_size && !emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_START, mail_id, tmp_file, nth, download_progress))
									EM_DEBUG_EXCEPTION("emcore_notify_network_event [ NOTI_DOWNLOAD_ATTACH_START] Failed >>>>");
							}
						}
					}

					if (flag_first_write == true) {

						FILE *fp = NULL;
						int encoded_len = EM_SAFE_STRLEN((char *)test_buffer);

						err = em_fopen(tmp_file, "wb+", &fp);
						if (err != EMAIL_ERROR_NONE) {
							EM_DEBUG_EXCEPTION_SEC("em_fopen failed - %s", tmp_file);
							goto FINISH_OFF;
						}

						if (encoded_len > 0 && fwrite(test_buffer, encoded_len, 1, fp) == 0) {
							EM_DEBUG_EXCEPTION("Error Occured while writing. fwrite(\"%s\") failed", test_buffer);
							err = EMAIL_ERROR_SYSTEM_FAILURE;
							fclose(fp); /* prevent */
							goto FINISH_OFF;
						}

						if (fp != NULL)
							fclose(fp);

						flag_first_write = false;
					} else {
						FILE *fp = NULL;
						int encoded_len = EM_SAFE_STRLEN((char *)test_buffer);

						err = em_fopen(tmp_file, "ab+", &fp);
						if (err != EMAIL_ERROR_NONE) {
							EM_DEBUG_EXCEPTION_SEC("em_fopen failed - %s", tmp_file);
							goto FINISH_OFF;
						}

						if (encoded_len > 0 && fwrite(test_buffer, encoded_len, 1, fp) == 0) {
							EM_DEBUG_EXCEPTION("Error Occured while writing. fwrite(\"%s\") failed", test_buffer);
							err = EMAIL_ERROR_SYSTEM_FAILURE;
							fclose(fp); /* prevent */
							goto FINISH_OFF;
						}

						if (fp != NULL)
							fclose(fp);
					}
					EM_DEBUG_LOG("%d has been written", EM_SAFE_STRLEN((char *)test_buffer));
				}

				if (GMIME_IS_PART(mime_object) && mime_part) {
					err = em_open(tmp_file, O_RDONLY, 0, &fd);
					if (err != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("holder open failed :  holder is a filename that will be saved.");
						if (fd) close(fd);
						goto FINISH_OFF;
					}

					content_stream = g_mime_stream_fs_new(fd);
					//parser = g_mime_parser_new_with_stream(content_stream);
					content = g_mime_data_wrapper_new_with_stream(content_stream, mime_part->encoding);
					if (content_stream) g_object_unref (content_stream);
					g_mime_part_set_content_object(mime_part, content);
					if (content) g_object_unref(content);
				}
			}

			EM_SAFE_FREE(tmp_file);
		}
		else if ((tag_position = g_strrstr(response, tag))) /*  end of response */ {
			if (!strncmp(tag_position + EM_SAFE_STRLEN(tag) + 1, "OK", 2)) {
				EM_SAFE_FREE(response);
			} else  {		/* 'NO' or 'BAD */
				err = EMAIL_ERROR_IMAP4_FETCH_UID_FAILURE;
				goto FINISH_OFF;
			}
			break;
		}
		else if (!strcmp(response, ")")) {
			/*  The end of response which contains body information */
		}
	}	/*  while (imaplocal->netstream)  */

	ret = TRUE;

FINISH_OFF:

	EM_SAFE_FREE(tmp_file);
	EM_SAFE_FREE(response);
	EM_SAFE_FREE (new_response);

	if (search_info) {
		EM_SAFE_FREE(search_info->section);
		EM_SAFE_FREE(search_info);
	}

	/*if (mime_iter) {
		g_mime_part_iter_free(mime_iter);
		mime_iter = NULL;
	}*/

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

INTERNAL_FUNC int emcore_gmime_check_filename_duplication(char *source_filename, struct _m_content_info *cnt_info)
{
	EM_DEBUG_FUNC_BEGIN("source_file_name [%s], content_info [%p]", source_filename, cnt_info);

	if (!source_filename || !cnt_info || strlen(source_filename) <= 0) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return FALSE;
	}

	struct attachment_info *cur_attachment_info = NULL;
	int ret = FALSE;

	cur_attachment_info = cnt_info->inline_file;

	while (cur_attachment_info) {

		if(g_strcmp0(source_filename, cur_attachment_info->name) == 0) {
			ret = TRUE;
			break;
		}

		cur_attachment_info = cur_attachment_info->next;
	}

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC char *emcore_gmime_get_modified_filename_in_duplication(char *source_filename)
{
	EM_DEBUG_FUNC_BEGIN();

	struct timeval tv;
	char temp_filename[MAX_PATH] = {0,};
	//char *filename = NULL;
	//char *extension = NULL;

	if (!source_filename) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return NULL;
	}

	/*if ((err = em_get_file_name_and_extension_from_file_path(source_filename, &filename, &extension)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_get_file_name_and_extension_from_file_path failed [%d]", err);
		return NULL;
	}*/

	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);

	snprintf(temp_filename, MAX_PATH, "%d_%s", rand(), source_filename);
	EM_DEBUG_LOG_SEC("temp_file_name [%s]", temp_filename);

	EM_DEBUG_FUNC_END();
	return EM_SAFE_STRDUP(temp_filename);
}

INTERNAL_FUNC char *emcore_gmime_get_encoding_to_utf8(const char *text)
{
    EM_DEBUG_FUNC_BEGIN();

    if (text == NULL) {
        EM_DEBUG_EXCEPTION("Invalid parameter");
        return NULL;
    }

    char *encoded_text = NULL;

    encoded_text = g_mime_utils_header_encode_text(text);
    if (encoded_text == NULL) {
        EM_DEBUG_EXCEPTION("g_mime_utils_header_encode_text failed : [%s]", text);
        return NULL;
    }

    EM_DEBUG_LOG_SEC("encoded_text : [%s]", encoded_text);

    return encoded_text;
}

INTERNAL_FUNC char *emcore_gmime_get_decoding_text(const char *text)
{
    EM_DEBUG_FUNC_BEGIN();

    if (text == NULL) {
        EM_DEBUG_EXCEPTION("Invalid parameter");
        return NULL;
    }

    char *decoded_text = NULL;

    decoded_text = g_mime_utils_header_decode_text(text);
    if (decoded_text == NULL) {
        EM_DEBUG_EXCEPTION("g_mime_utils_header_encode_text failed : [%s]", text);
        return NULL;
    }

    EM_DEBUG_LOG("decoded_text : [%s]", decoded_text);

    return decoded_text;
}
