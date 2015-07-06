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



/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ***
 *File :  email-core-mime.c
 *Desc :  MIME Operation
 *
 *Auth :
 *
 *History :
 *   2011.04.14  :  created
 ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ***/
#undef close

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vconf.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <glib.h>
#include "email-internal-types.h"
#include "lnx_inc.h"
#include "email-utilities.h"
#include "email-core-utils.h"
#include "email-core-mail.h"
#include "email-core-mime.h"
#include "email-core-gmime.h"
#include "email-storage.h"
#include "email-core-event.h"
#include "email-core-account.h"
#include "email-core-signal.h"
#include "email-core-mailbox-sync.h"
#include "email-core-container.h"
#include "email-debug-log.h"

#define MIME_MESG_IS_SOCKET

#define MIME_LINE_LEN	1024
#define BOUNDARY_LEN	256

#define TYPE_TEXT            1
#define TYPE_IMAGE           2
#define TYPE_AUDIO           3
#define TYPE_VIDEO           4
#define TYPE_APPLICATION     5
#define TYPE_MULTIPART       6
#define TYPE_MESSAGE         7
#define TYPE_UNKNOWN         8

#define TEXT_STR             "TEXT"
#define IMAGE_STR            "IMAGE"
#define AUDIO_STR            "AUDIO"
#define VIDEO_STR            "VIDEO"
#define APPLICATION_STR      "APPLICATION"
#define MULTIPART_STR        "MULTIPART"
#define MESSAGE_STR          "MESSAGE"

#define CONTENT_TYPE         1
#define CONTENT_SUBTYPE      2
#define CONTENT_ENCODING     3
#define CONTENT_CHARSET      4
#define CONTENT_DISPOSITION  5
#define CONTENT_NAME         6
#define CONTENT_FILENAME     7
#define CONTENT_BOUNDARY     8
#define CONTENT_REPORT_TYPE  9
#define CONTENT_ID           10
#define CONTENT_LOCATION     11
#define CONTENT_SMIME_TYPE   12
#define CONTENT_PROTOCOL     13

#define GRAB_TYPE_TEXT       1	/*  retrieve text and attachment name */
#define GRAB_TYPE_ATTACHMENT 2	/*  retrieve only attachmen */

#define SAVE_TYPE_SIZE       1	/*  only get content siz */
#define SAVE_TYPE_BUFFER     2	/*  save content to buffe */
#define SAVE_TYPE_FILE       3	/*  save content to temporary fil */

#define EML_FOLDER           20 /*  save eml content to temporary folder */


/* ---------------------------------------------------------------------- */
/*  Global variable */
static int  eml_data_count = 0;

/* ---------------------------------------------------------------------- */
/*  External variable */
extern int _pop3_receiving_mail_id;
extern int _pop3_received_body_size;
extern int _pop3_last_notified_body_size;
extern int _pop3_total_body_size;

extern int _imap4_received_body_size;
extern int _imap4_last_notified_body_size;
extern int _imap4_total_body_size;
extern int _imap4_download_noti_interval_value;

extern int multi_part_body_size;
extern bool only_body_download;

/* ---------------------------------------------------------------------- */
__thread BODY **g_inline_list = NULL;
__thread int g_inline_count = 0;

void emcore_mime_free_param(struct _parameter *param);
void emcore_mime_free_part_header(struct _m_part_header *header);
void emcore_mime_free_message_header(struct _m_mesg_header *header);
void emcore_mime_free_rfc822_header(struct _rfc822header *rfc822header);
void emcore_mime_free_part(struct _m_part *part);
void emcore_mime_free_part_body(struct _m_body *body);
void emcore_mime_free_mime(struct _m_mesg *mmsg);

extern long pop3_reply (MAILSTREAM *stream);
/* ------------------------------------------------------------------------------------------------- */

char *em_get_escaped_str(const char *str)
{
	EM_DEBUG_FUNC_BEGIN("str [%s]", str);

	int i = 0, j = 0;
	int len = 0;

	if (!str)
		return NULL;
	len = strlen(str);
	char *ret = calloc(1, 2 * len);
	if (ret == NULL) {
		return NULL;
	}

	int ret_i = 0;
	const char to_be_escaped[10] = {'\\', '\"', '\0',};

	for (i = 0; i < len; i++) {
		for (j = 0; to_be_escaped[j] != '\0'; j++) {
			if (str[i] == to_be_escaped[j]) {
				ret[ret_i++] = '\\';
				break;
			}
		}
		ret[ret_i++] = str[i];
	}
	ret[ret_i++] = '\0';

	return ret;
}

/* Fix for issue junk characters in mail body */
char *em_split_file_path(char *str)
{
	EM_DEBUG_FUNC_BEGIN("str [%s]", str);

	EM_IF_NULL_RETURN_VALUE(str, NULL);

	char *buf = NULL;
	char delims[] = "@";
	char *result = NULL;
	char *temp_str = NULL;
	char *content_id = NULL;
	int buf_length = 0;
	char *temp_cid_data = NULL;
	char *temp_cid = NULL;

	temp_str = EM_SAFE_STRDUP(str);
	buf_length = EM_SAFE_STRLEN(str) + 1024;
	buf = em_malloc(buf_length);
	if (buf == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		return NULL;
	}

	content_id = temp_str;
	temp_cid = strstr(temp_str, "\"");
	if (temp_cid == NULL) {
		EM_DEBUG_EXCEPTION(">>>> File Path Doesnot contain end line for CID ");
		EM_SAFE_FREE(buf);
		return temp_str;
	}

	temp_cid_data = em_malloc((temp_cid-temp_str)+1);
	if (temp_cid_data == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		return temp_str;
	}

	memcpy(temp_cid_data, temp_str, (temp_cid-temp_str));

	if (temp_cid_data && !strstr(temp_cid_data, delims)) {
		EM_DEBUG_EXCEPTION(">>>> File Path Doesnot contain @ ");
		EM_SAFE_FREE(buf);
		EM_SAFE_FREE(temp_cid_data);
		return temp_str;
	} else {
		result = strstr(temp_str, delims);
		if (result != NULL) {
			*result = '\0';
			result++;
			EM_DEBUG_LOG_SEC("content_id is [ %s ]", content_id);

			if (strcasestr(content_id, ".bmp") || strcasestr(content_id, ".jpeg") || strcasestr(content_id, ".png") ||
					strcasestr(content_id, ".jpg") || strcasestr(content_id, ".gif"))
				snprintf(buf+EM_SAFE_STRLEN(buf), buf_length - EM_SAFE_STRLEN(buf), "%s\"", content_id);
			else
				snprintf(buf+EM_SAFE_STRLEN(buf), buf_length - EM_SAFE_STRLEN(buf), "%s%s", content_id, ".jpeg\"");
		}
		else {
			EM_DEBUG_EXCEPTION(">>>> File Path Doesnot contain end line for CID ");
			EM_SAFE_FREE(buf);
			EM_SAFE_FREE(temp_cid_data);
			return temp_str;
		}
		result = strstr(result, "\"");
		if (result != NULL) {
			result++;
			snprintf(buf+EM_SAFE_STRLEN(buf), buf_length - EM_SAFE_STRLEN(buf), "%s", result);
		}
	}

	EM_SAFE_FREE(temp_str);
	EM_SAFE_FREE(temp_cid_data);
	return buf;
}

/*
 * decode body text (quoted-printable, base64)
 * enc_type : encoding type (base64/quotedprintable)
 */
INTERNAL_FUNC int emcore_decode_body_text(char *enc_buf, int enc_len, int enc_type, int *dec_len, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("enc_buf[%p], enc_len[%d], enc_type[%d], dec_len[%p]", enc_buf, enc_len, enc_type, dec_len);
	unsigned char *content = NULL;

	/*  too many called */
	*dec_len = enc_len;

	switch (enc_type)  {
		case ENCQUOTEDPRINTABLE:

			content = rfc822_qprint((unsigned char *)enc_buf, (unsigned long)enc_len, (unsigned long *)dec_len);
			break;

		case ENCBASE64:

			content = rfc822_base64((unsigned char *)enc_buf, (unsigned long)enc_len, (unsigned long *)dec_len);
			break;

		case ENC7BIT:
		case ENC8BIT:
		case ENCBINARY:
		case ENCOTHER:
		default:
			break;
	}

	if (content)  {
		if (enc_len < *dec_len) {
			EM_DEBUG_EXCEPTION("Decoded length is too big to store it");
			EM_SAFE_FREE(content);
			if(err_code) *err_code = EMAIL_ERROR_INVALID_DATA;
			return EMAIL_ERROR_INVALID_DATA;
		}
		memcpy(enc_buf, content, *dec_len);
		enc_buf[*dec_len] = '\0';
		EM_SAFE_FREE(content);
	}
	EM_DEBUG_FUNC_END();

	if(err_code) *err_code = EMAIL_ERROR_NONE;
	return EMAIL_ERROR_NONE;
}

/*  get temporary file name */
char *emcore_mime_get_save_file_name(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	char tempname[512];
	struct timeval tv;

	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);

	memset(tempname, 0x00, sizeof(tempname));

	SNPRINTF(tempname, sizeof(tempname), "%s%s%d", MAILTEMP, DIR_SEPERATOR, rand());
	EM_DEBUG_FUNC_END();
	return EM_SAFE_STRDUP(tempname);
}

/*  get a line from file pointer */
char *emcore_get_line_from_file(void *stream, char *buf, int size, int *err_code)
{
	if (!fgets(buf, size, (FILE *)stream))  {
		if (feof((FILE *)stream)) {
			*err_code = EMAIL_ERROR_NO_MORE_DATA;
			return NULL;
		} else
			return NULL;
	}
	return buf;
}

/*  get a line from POP3 mail stream */
/*  emcore_mail_cmd_read_mail_pop3 must be called before this function */
char *emcore_mime_get_line_from_sock(void *stream, char *buf, int size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], buf[%p]", stream, buf);
	POP3LOCAL *p_pop3local = NULL;

	if (!stream || !buf)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return NULL;
	}

	memset(buf, 0x00, size);

	p_pop3local = (POP3LOCAL *)(((MAILSTREAM *)stream)->local);
	if (!p_pop3local)  {
		EM_DEBUG_EXCEPTION("stream->local[%p]", p_pop3local);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return NULL;
	}

	if (!pop3_reply((MAILSTREAM *)stream))  { /*  if TRUE, check response */
		EM_DEBUG_LOG_DEV("p_pop3local->response 1[%s]", p_pop3local->response);
		if (p_pop3local->response) {
			if (*p_pop3local->response == '.' && EM_SAFE_STRLEN(p_pop3local->response) == 1)  {
				EM_SAFE_FREE (p_pop3local->response);
				if (err_code != NULL)
					*err_code = EMAIL_ERROR_NO_MORE_DATA;
				EM_DEBUG_FUNC_END("end of response");
				return NULL;
			}
			EM_DEBUG_LOG_DEV("Not end of response");
			strncpy(buf, p_pop3local->response, size-1);
			strncat(buf, CRLF_STRING, size-(EM_SAFE_STRLEN(buf) + 1));

			EM_SAFE_FREE (p_pop3local->response);
			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG_DEV("p_pop3local->response 2[%s]", p_pop3local->response);
	if (p_pop3local->response)
	{
		/*  if response isn't NULL, check whether this response start with '+' */
		/*  if the first character is '+', return error because this response is normal data */
		strncpy(buf, p_pop3local->response, size-1);
		strncat(buf, CRLF_STRING, size-(EM_SAFE_STRLEN(buf)+1));
		EM_SAFE_FREE (p_pop3local->response);
		goto FINISH_OFF;
	} else {
		EM_DEBUG_EXCEPTION("p_pop3local->response is null. network error... ");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_RESPONSE;
		EM_DEBUG_FUNC_END();
		return NULL;
	}

FINISH_OFF:
	if (buf) {

		//remove stuffed dot
		if (EM_SAFE_STRLEN(buf) >= 2 && buf[0] == '.' && buf[1] == '.') {
			char *tmp = g_strdup(buf+1);
			memset(buf, 0x00, size);
			strncpy(buf, tmp, size-1);
			EM_SAFE_FREE(tmp);
		}

		int received_percentage, last_notified_percentage;
		_pop3_received_body_size += EM_SAFE_STRLEN(buf);

		last_notified_percentage = (double)_pop3_last_notified_body_size / (double)_pop3_total_body_size *100.0;
		received_percentage      = (double)_pop3_received_body_size / (double)_pop3_total_body_size *100.0;

		EM_DEBUG_LOG_DEV("_pop3_received_body_size = %d, _pop3_total_body_size = %d", _pop3_received_body_size, _pop3_total_body_size);
		EM_DEBUG_LOG_DEV("received_percentage = %d, last_notified_percentage = %d", received_percentage, last_notified_percentage);

		if (received_percentage > last_notified_percentage + 5) {
			if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_START, _pop3_receiving_mail_id, "dummy-file", _pop3_total_body_size, _pop3_received_body_size))
				EM_DEBUG_EXCEPTION(" emcore_notify_network_event [NOTI_DOWNLOAD_BODY_START] Failed >>>> ");
			else
				EM_DEBUG_LOG("NOTI_DOWNLOAD_BODY_START notified (%d / %d)", _pop3_received_body_size, _pop3_total_body_size);
			_pop3_last_notified_body_size = _pop3_received_body_size;
		}
	}
	EM_DEBUG_FUNC_END();
	return buf;
}

void emcore_mime_free_param(struct _parameter *param)
{
	struct _parameter *t, *p = param;
	EM_DEBUG_FUNC_BEGIN();
	while (p) {
		t = p->next;
		EM_SAFE_FREE (p->name);
		EM_SAFE_FREE (p->value);
		EM_SAFE_FREE (p);
		p = t;
	}
	EM_DEBUG_FUNC_END();
}

void emcore_mime_free_part_header(struct _m_part_header *header)
{
	EM_DEBUG_FUNC_BEGIN();
	if (!header) return ;
	EM_SAFE_FREE (header->type);
	if (header->parameter) emcore_mime_free_param(header->parameter);
	EM_SAFE_FREE (header->subtype);
	EM_SAFE_FREE (header->encoding);
	EM_SAFE_FREE (header->desc);
	EM_SAFE_FREE (header->disp_type);
	EM_SAFE_FREE (header->content_id);
	EM_SAFE_FREE (header->content_location);
	EM_SAFE_FREE (header->priority);
	EM_SAFE_FREE (header->ms_priority);
	if (header->disp_parameter) emcore_mime_free_param(header->disp_parameter);
	EM_SAFE_FREE (header);
	EM_DEBUG_FUNC_END();
}

void emcore_mime_free_message_header(struct _m_mesg_header *header)
{
	EM_DEBUG_FUNC_BEGIN();
	if (!header) return ;
	EM_SAFE_FREE (header->version);
	if (header->part_header) emcore_mime_free_part_header(header->part_header);
	EM_SAFE_FREE (header);
	EM_DEBUG_FUNC_END();
}

void emcore_mime_free_rfc822_header(struct _rfc822header *header)
{
	EM_DEBUG_FUNC_BEGIN();
	if (!header) return ;
	EM_SAFE_FREE (header->return_path);
	EM_SAFE_FREE (header->received);
	EM_SAFE_FREE (header->reply_to);
	EM_SAFE_FREE (header->date);
	EM_SAFE_FREE (header->from);
	EM_SAFE_FREE (header->subject);
	EM_SAFE_FREE (header->sender);
	EM_SAFE_FREE (header->to);
	EM_SAFE_FREE (header->cc);
	EM_SAFE_FREE (header->bcc);
	EM_SAFE_FREE (header);
	EM_DEBUG_FUNC_END();
}

void emcore_mime_free_part_body(struct _m_body *body)
{
	EM_DEBUG_FUNC_BEGIN();
	if (!body) return ;
	if (body->part_header) emcore_mime_free_part_header(body->part_header);
	EM_SAFE_FREE(body->text);
	if (body->nested.body) emcore_mime_free_part_body(body->nested.body);
	if (body->nested.next) emcore_mime_free_part(body->nested.next);
	EM_SAFE_FREE (body);
	EM_DEBUG_FUNC_END();
}

void emcore_mime_free_part(struct _m_part *part)
{
	EM_DEBUG_FUNC_BEGIN();
	if (!part) return ;
	if (part->body) emcore_mime_free_part_body(part->body);
	if (part->next) emcore_mime_free_part(part->next);
	EM_SAFE_FREE (part);
	EM_DEBUG_FUNC_END();
}

void emcore_mime_free_mime(struct _m_mesg *mmsg)
{
	EM_DEBUG_FUNC_BEGIN();

	if (!mmsg) return ;
	if (mmsg->header) emcore_mime_free_message_header(mmsg->header);
	if (mmsg->rfc822header) emcore_mime_free_rfc822_header(mmsg->rfc822header);
	if (mmsg->nested.body) emcore_mime_free_part_body(mmsg->nested.body);
	if (mmsg->nested.next) emcore_mime_free_part(mmsg->nested.next);
	EM_SAFE_FREE(mmsg->text);
	EM_SAFE_FREE (mmsg);
	EM_DEBUG_FUNC_END();
}

/*
static char *em_parse_filename(char *filename)
{
	EM_DEBUG_FUNC_BEGIN("filename [%p] ", filename);
	if (!filename) {
		EM_DEBUG_EXCEPTION("filename is NULL ");
		return NULL;
	}

	char delims[] = "@";
	char *result = NULL;
	static char parsed_filename[512] = {0, };

	memset(parsed_filename, 0x00, 512);

	if (!strstr(filename, delims)) {
		EM_DEBUG_EXCEPTION("FileName does not contain @ ");
		return NULL;
	}

	result = strtok(filename, delims);

	if (strcasestr(result, ".bmp") || strcasestr(result, ".jpeg") || strcasestr(result, ".png") || strcasestr(result, ".jpg"))
		sprintf(parsed_filename + EM_SAFE_STRLEN(parsed_filename), "%s", result);
    else
		sprintf(parsed_filename + EM_SAFE_STRLEN(parsed_filename), "%s%s", result, ".jpeg");

	EM_DEBUG_LOG_SEC(">>> FileName [ %s ] ", result);
	EM_DEBUG_LOG_SEC("parsed_filename [%s] ", parsed_filename);
	EM_DEBUG_FUNC_END();
	return parsed_filename;
}
*/

#define CONTENT_TYPE_STRING_IN_MIME_HEAEDER "Content-Type:"

INTERNAL_FUNC int emcore_get_content_type_from_mime_string(char *input_mime_string, char **output_content_type)
{
	EM_DEBUG_FUNC_BEGIN("input_mime_string [%p], output_content_type [%p]", input_mime_string, output_content_type);

	int   err = EMAIL_ERROR_NONE;
	int   temp_mime_header_string_length = 0;
	char  result_content_type[256] = { 0, };
	char *temp_mime_header_string = NULL;
	char *temp_content_type_start = NULL;
	char *temp_content_type_end = NULL;

	if(input_mime_string == NULL || output_content_type == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	temp_mime_header_string_length = EM_SAFE_STRLEN(input_mime_string);
	temp_mime_header_string        = input_mime_string;

	EM_DEBUG_LOG("temp_mime_header_string [%s]", temp_mime_header_string);

	temp_content_type_start = strcasestr(temp_mime_header_string, CONTENT_TYPE_STRING_IN_MIME_HEAEDER);

	if(temp_content_type_start == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_DATA");
		err = EMAIL_ERROR_INVALID_DATA;
		goto FINISH_OFF;
	}
	else {
		temp_content_type_start += EM_SAFE_STRLEN(CONTENT_TYPE_STRING_IN_MIME_HEAEDER);
		temp_content_type_end = temp_content_type_start;

		while(temp_content_type_end && temp_content_type_end < (temp_mime_header_string + temp_mime_header_string_length) && *temp_content_type_end != ';')
			temp_content_type_end++;

		if(temp_content_type_end && *temp_content_type_end == ';') {
			if(temp_content_type_end - temp_content_type_start < 256) {
				memcpy(result_content_type, temp_content_type_start, temp_content_type_end - temp_content_type_start);
				EM_DEBUG_LOG_SEC("result_content_type [%s]", result_content_type);
				*output_content_type = EM_SAFE_STRDUP(em_trim_left(result_content_type));
			}
			else {
				EM_DEBUG_EXCEPTION("temp_content_type_end - temp_content_type_start [%d]", temp_content_type_end - temp_content_type_start);
				err = EMAIL_ERROR_DATA_TOO_LONG;
				goto FINISH_OFF;
			}
		}
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_get_utf8_address(char **dest, ADDRESS *address, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("dest[%p], address[%p], err_code[%p]", dest, address, err_code);

	if (!dest || !address)  {
		EM_DEBUG_EXCEPTION("dest[%p], address[%p]", dest, address);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	gchar *concatenated = NULL;
	gchar *utf8_address = NULL;
	gchar *temp = NULL;
	char *nickname = NULL;

	while (address)  {
		if (address->personal)  {
			if (!(nickname = emcore_gmime_get_decoding_text(address->personal)))  {
				EM_DEBUG_EXCEPTION("emcore_gmime_get_decoding_text failed - %d", err);
				goto FINISH_OFF;
			}
			EM_DEBUG_LOG_DEV("nickname[%s]", nickname);
			if (*nickname != '\0') {
				char *tmp_nickname = nickname;
				nickname = em_get_escaped_str(tmp_nickname);
				EM_SAFE_FREE(tmp_nickname);
				utf8_address = g_strdup_printf("\"%s\" <%s%s%s>", nickname, address->mailbox ? address->mailbox : "", address->host ? "@": "", address->host ? address->host : "");
			} else {
				utf8_address = g_strdup_printf("<%s%s%s>", address->mailbox ? address->mailbox : "", address->host ? "@" : "", address->host ? address->host : "");
			}

			EM_SAFE_FREE(nickname);
		}
		else {
			if (address->mailbox || address->host)
				utf8_address = g_strdup_printf("<%s%s%s>", address->mailbox ? address->mailbox : "", address->host ? "@" : "", address->host ? address->host : "");
		}

		EM_DEBUG_LOG_DEV("utf8_address[%s]", utf8_address);

		if (utf8_address) {
			if ((concatenated != NULL))  {
				temp = concatenated;
				concatenated = g_strdup_printf("%s; %s", temp, utf8_address);
				g_free(temp);
			}
			else
				concatenated = g_strdup(utf8_address);
		}

		g_free(utf8_address);
		utf8_address = NULL;

		address = address->next;
	}

	*dest = concatenated;

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE(nickname);
	EM_DEBUG_FUNC_END("ret[%d]", ret);
	return ret;
}

#define SUBTYPE_STRING_LENGTH 128

INTERNAL_FUNC int emcore_get_content_type_from_mail_bodystruct(BODY *input_body, int input_buffer_length, char *output_content_type)
{
	EM_DEBUG_FUNC_BEGIN("input_body [%p], input_buffer_length [%d], output_content_type [%p]", input_body, input_buffer_length, output_content_type);
	int   err = EMAIL_ERROR_NONE;
	char  subtype_string[SUBTYPE_STRING_LENGTH] = { 0 , };

	if(input_body == NULL || output_content_type == NULL || input_buffer_length == 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	EM_SAFE_STRNCPY(subtype_string, input_body->subtype, SUBTYPE_STRING_LENGTH-1); /* prevent 21983 */
	em_lower_string(subtype_string);

	switch(input_body->type) {
		case TYPETEXT :
			SNPRINTF(output_content_type, input_buffer_length, "text/%s", subtype_string);
			break;
		case TYPEMULTIPART :
			SNPRINTF(output_content_type, input_buffer_length, "multipart/%s", subtype_string);
			break;
		case TYPEMESSAGE :
			SNPRINTF(output_content_type, input_buffer_length, "message/%s", subtype_string);
			break;
		case TYPEAPPLICATION :
			SNPRINTF(output_content_type, input_buffer_length, "application/%s", subtype_string);
			break;
		case TYPEAUDIO :
			SNPRINTF(output_content_type, input_buffer_length, "audio/%s", subtype_string);
			break;
		case TYPEIMAGE :
			SNPRINTF(output_content_type, input_buffer_length, "image/%s", subtype_string);
			break;
		case TYPEVIDEO :
			SNPRINTF(output_content_type, input_buffer_length, "video/%s", subtype_string);
			break;
		case TYPEMODEL :
			SNPRINTF(output_content_type, input_buffer_length, "model/%s", subtype_string);
			break;
		case TYPEOTHER :
			SNPRINTF(output_content_type, input_buffer_length, "other/%s", subtype_string);
			break;
		default:
			EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
			err = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
			break;
	}

	EM_DEBUG_LOG_SEC("output_content_type [%s]", output_content_type);

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_get_attribute_value_of_body_part(PARAMETER *input_param, char *atribute_name, char *output_value, int output_buffer_length, int with_rfc2047_text, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("input_param [%p], atribute_name [%s], output_buffer_length [%d], with_rfc2047_text [%d]", input_param, atribute_name, output_buffer_length, with_rfc2047_text);
	PARAMETER *temp_param = input_param;
	char *decoded_value = NULL, *result_value = NULL;
	int ret = false, err = EMAIL_ERROR_NONE;

	if(!output_value) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	memset(output_value, 0, output_buffer_length);

	while (temp_param)  {
		if (!strcasecmp(temp_param->attribute, atribute_name))  {
			if (temp_param->value) {
				if (with_rfc2047_text) {
					decoded_value = emcore_gmime_get_decoding_text(temp_param->value);
					result_value = EM_SAFE_STRDUP(decoded_value);
					EM_SAFE_FREE(decoded_value);
				}
				else
					result_value = g_strdup(temp_param->value);
			}
			else {
				EM_DEBUG_EXCEPTION("EMAIL_ERROR_DATA_NOT_FOUND");
				err = EMAIL_ERROR_DATA_NOT_FOUND;
				goto FINISH_OFF;
			}

			if(result_value) {
				if(output_buffer_length > EM_SAFE_STRLEN(result_value)) {
					strncpy(output_value, result_value, output_buffer_length);
					output_value[EM_SAFE_STRLEN(result_value)] = NULL_CHAR;
					ret = true;
				}
				else {
					EM_DEBUG_EXCEPTION("buffer is too short");
					err = EMAIL_ERROR_DATA_TOO_LONG;
					goto FINISH_OFF;
				}
			}

			break;
		}
		temp_param = temp_param->next;
	}

FINISH_OFF:
	EM_SAFE_FREE(decoded_value);
	EM_SAFE_FREE(result_value);

	if(err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/* get body structure */
INTERNAL_FUNC int emcore_get_body_structure(MAILSTREAM *stream, int msg_uid, BODY **body, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], err_code[%p]", stream, msg_uid, body, err_code);

	EM_IF_NULL_RETURN_VALUE(stream, false);
	EM_IF_NULL_RETURN_VALUE(body, false);

#ifdef __FEATURE_HEADER_OPTIMIZATION__
		ENVELOPE *env = mail_fetch_structure(stream, msg_uid, body, FT_UID | FT_PEEK | FT_NOLOOKAHEAD, 1);
#else
		ENVELOPE *env = mail_fetch_structure(stream, msg_uid, body, FT_UID | FT_PEEK | FT_NOLOOKAHEAD);
#endif
	if (!env) {
		if (err_code)
			*err_code = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER;
		EM_DEBUG_EXCEPTION("mail_fetch_structure failed");
		return FAILURE;
	}

#ifdef FEATURE_CORE_DEBUG
	_print_body(*body, true); /* shasikala.p@partner.samsung.com */
#endif
	EM_DEBUG_FUNC_END();
	return SUCCESS;
}

int emcore_set_fetch_part_section(BODY *body, char *section_pfx, int section_subno, int enable_inline_list, int *total_mail_size, int *total_body_size, int *err_code);

/* set body section to be fetched */
INTERNAL_FUNC int emcore_set_fetch_body_section(BODY *body, int enable_inline_list, int *total_mail_size, int *total_body_size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("body[%p], err_code[%p]", body, err_code);

	if (!body)  {
		EM_DEBUG_EXCEPTION("body [%p]", body);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return FAILURE;
	}

//	body->id = cpystr("1"); /*  top level body */
	EM_DEBUG_LOG_SEC("body->id : [%s]", body->id);

	if (enable_inline_list) {
		g_inline_count = 0;
		EM_SAFE_FREE(g_inline_list);
	}

	emcore_set_fetch_part_section(body, (char *)NULL, 0, enable_inline_list, total_mail_size, total_body_size, err_code);

	if (body && body->id)
		EM_DEBUG_LOG_SEC(">>>>> FILE NAME [%s] ", body->id);
	else
		EM_DEBUG_LOG(">>>>> BODY NULL ");

	EM_DEBUG_FUNC_END();
	return SUCCESS;
}

/* set part section of body to be fetched */
int emcore_set_fetch_part_section(BODY *body, char *section_pfx, int section_subno, int enable_inline_list, int *total_mail_size, int *total_body_size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("body[%p], section_pfx[%s], section_subno[%d], enable_inline_list[%d], total_mail_size[%p] err_code[%p]", body, section_pfx, section_subno, enable_inline_list, total_mail_size, err_code);

	PART *part = NULL;
	char section[64] = {0x00, };

	/* multipart doesn't have a row to itself */
	if (body->type == TYPEMULTIPART)  {
		/* if not first time, extend prefix */
		if (section_pfx) {
			SNPRINTF(section, sizeof(section), "%s%d.", section_pfx, ++section_subno);
		}
		else {
			section[0] = '\0';
		}

		for (section_subno = 0, part = body->nested.part; part; part = part->next)
			emcore_set_fetch_part_section(&part->body, section, section_subno++, enable_inline_list, total_mail_size, total_body_size, err_code);
	}
	else  {
		if (!section_pfx) /* dummy prefix if top level */
			section_pfx = "";

		SNPRINTF(section, sizeof(section), "%s%d", section_pfx, ++section_subno);

		if (enable_inline_list && ((body->disposition.type && (body->disposition.type[0] == 'i' || body->disposition.type[0] == 'I')) ||
			(!body->disposition.type && body->id))) {
			BODY **temp = NULL;
			temp =	realloc(g_inline_list, sizeof(BODY *) *(g_inline_count + 1));
			if (NULL != temp) {
				memset(temp+g_inline_count, 0x00, sizeof(BODY *));
				g_inline_list = temp;
				g_inline_list[g_inline_count] = body;
				g_inline_count++;
				temp = NULL;
			}
			else {
				EM_DEBUG_EXCEPTION("realloc fails");
			}

			EM_DEBUG_LOG("Update g_inline_list with inline count [%d]", g_inline_count);
		}

		if (total_mail_size != NULL) {
			*total_mail_size = *total_mail_size + (int)body->size.bytes;
			EM_DEBUG_LOG("body->size.bytes [%d]", body->size.bytes);
			EM_DEBUG_LOG("total_mail_size [%d]", *total_mail_size);
		}

		if ((total_body_size != NULL) && !(body->disposition.type && (body->disposition.type[0] == 'a' || body->disposition.type[0] == 'A'))) {
			*total_body_size = *total_body_size + (int)body->size.bytes;
			EM_DEBUG_LOG("body->size.bytes [%d]", body->size.bytes);
			EM_DEBUG_LOG("total_mail_size [%d]", *total_body_size);
		}

		/* encapsulated message ? */
		if ((body->type == TYPEMESSAGE) && !strcasecmp(body->subtype, "RFC822") && (body = ((MESSAGE *)body->nested.msg)->body))  {
			if (body->type == TYPEMULTIPART) {
				section[0] = '\0';
				emcore_set_fetch_part_section(body, section, section_subno-1, enable_inline_list, total_mail_size, total_body_size, err_code);
			}
			else  {		/*  build encapsulation prefi */
				SNPRINTF(section, sizeof(section), "%s%d.", section_pfx, section_subno);
				emcore_set_fetch_part_section(body, section, 0, enable_inline_list, total_mail_size, total_body_size, err_code);
			}
		}
		else  {
			/* set body section */
			if (body)
				body->sparep = cpystr(section);
		}
	}
	EM_DEBUG_FUNC_END();
	return SUCCESS;
}

static int emcore_make_mail_data_from_m_mesg(email_mail_data_t *dst_mail_data, struct _m_mesg *mmsg)
{
        EM_DEBUG_FUNC_BEGIN();
        int err = EMAIL_ERROR_NONE;

        if (!mmsg || !dst_mail_data) {
                EM_DEBUG_EXCEPTION("Invalid parameter");
                err = EMAIL_ERROR_INVALID_PARAM;
                return err;
        }

	char *encoded_subject = NULL;
	char *first_address = NULL;
	char *first_alias = NULL;
	struct tm temp_time_info;
	ADDRESS *from = NULL;
	ADDRESS *to = NULL;
	ADDRESS *cc = NULL;
	ADDRESS *bcc = NULL;
	ADDRESS *return_path = NULL;
	ADDRESS *reply_to = NULL;
	MESSAGECACHE mail_cache_element = {0, };

 	memset(&mail_cache_element, 0x00, sizeof(MESSAGECACHE));
	memset((void *)&temp_time_info, 0, sizeof(struct tm));

	/* Set the smime_type */
	if (mmsg->rfc822header && mmsg->rfc822header->content_type) {
		if (strcasestr(mmsg->rfc822header->content_type, "pkcs7-mime")) {
			if (strcasestr(mmsg->rfc822header->content_type, "enveloped-data"))
				dst_mail_data->smime_type = EMAIL_SMIME_ENCRYPTED;
			else
				dst_mail_data->smime_type = EMAIL_SMIME_SIGNED_AND_ENCRYPTED;
		} else if (strcasestr(mmsg->rfc822header->content_type, "encrypted")) {
			dst_mail_data->smime_type = EMAIL_PGP_ENCRYPTED;
		} else if (strcasestr(mmsg->rfc822header->content_type, "signed")) {
			if (strcasestr(mmsg->rfc822header->content_type, "pkcs7-signature"))
				dst_mail_data->smime_type = EMAIL_SMIME_SIGNED;
			else
				dst_mail_data->smime_type = EMAIL_PGP_SIGNED;

			dst_mail_data->digest_type = emcore_get_digest_type(mmsg->rfc822header->content_type);
		} else {
			dst_mail_data->smime_type = EMAIL_SMIME_NONE;
		}
	}

	/* Set the priority */
	if (mmsg->rfc822header) {
		if (mmsg->rfc822header->priority) {
			switch(atoi(mmsg->rfc822header->priority)) {
			case 1:
				dst_mail_data->priority = EMAIL_MAIL_PRIORITY_HIGH;
				break;
			case 5:
				dst_mail_data->priority = EMAIL_MAIL_PRIORITY_LOW;
				break;
			case 3:
			default:
				dst_mail_data->priority = EMAIL_MAIL_PRIORITY_NORMAL;
				break;
			}
		} else if (mmsg->rfc822header->ms_priority) {
			if (strcasestr(mmsg->rfc822header->ms_priority, "HIGH"))
				dst_mail_data->priority = EMAIL_MAIL_PRIORITY_HIGH;
			else if (strcasestr(mmsg->rfc822header->ms_priority, "NORMAL"))
				dst_mail_data->priority = EMAIL_MAIL_PRIORITY_NORMAL;
			else if (strcasestr(mmsg->rfc822header->ms_priority, "LOW"))
				dst_mail_data->priority = EMAIL_MAIL_PRIORITY_LOW;
			else
				dst_mail_data->priority = EMAIL_MAIL_PRIORITY_NORMAL;
		}
	} else if (mmsg->nested.body) {
		if (mmsg->nested.body->part_header->priority) {
			switch(atoi(mmsg->nested.body->part_header->priority)) {
			case 1:
				dst_mail_data->priority = EMAIL_MAIL_PRIORITY_HIGH;
				break;
			case 5:
				dst_mail_data->priority = EMAIL_MAIL_PRIORITY_LOW;
				break;
			case 3:
			default:
				dst_mail_data->priority = EMAIL_MAIL_PRIORITY_NORMAL;
				break;
			}
		} else if (mmsg->nested.body->part_header->ms_priority) {
			if (strcasestr(mmsg->nested.body->part_header->ms_priority, "HIGH"))
				dst_mail_data->priority = EMAIL_MAIL_PRIORITY_HIGH;
			else if (strcasestr(mmsg->nested.body->part_header->ms_priority, "NORMAL"))
				dst_mail_data->priority = EMAIL_MAIL_PRIORITY_NORMAL;
			else if (strcasestr(mmsg->nested.body->part_header->ms_priority, "LOW"))
				dst_mail_data->priority = EMAIL_MAIL_PRIORITY_LOW;
			else
				dst_mail_data->priority = EMAIL_MAIL_PRIORITY_NORMAL;
		}
	}

	if (!mmsg->rfc822header) {
                EM_DEBUG_LOG("This mail did not have envelop");
                return err;
        }

	/* Set the date */
        if (mmsg->rfc822header->date) {
                EM_DEBUG_LOG("date : [%s]", mmsg->rfc822header->date);
                mail_parse_date(&mail_cache_element, (unsigned char *)mmsg->rfc822header->date);
        }

        temp_time_info.tm_sec = mail_cache_element.seconds;
        temp_time_info.tm_min = mail_cache_element.minutes - mail_cache_element.zminutes;
        temp_time_info.tm_hour = mail_cache_element.hours - mail_cache_element.zhours;

        if (mail_cache_element.hours - mail_cache_element.zhours < 0) {
                temp_time_info.tm_mday = mail_cache_element.day - 1;
                temp_time_info.tm_hour += 24;
        } else
                temp_time_info.tm_mday = mail_cache_element.day;

        temp_time_info.tm_mon = mail_cache_element.month - 1;
        temp_time_info.tm_year = mail_cache_element.year + 70;

        dst_mail_data->date_time                   = timegm(&temp_time_info);


	/* Set the subject */
        encoded_subject = emcore_gmime_get_decoding_text(mmsg->rfc822header->subject);
        dst_mail_data->subject                     = EM_SAFE_STRDUP(encoded_subject);

	/* Set the email address(from, to, cc, bcc, received ...) */
        dst_mail_data->email_address_recipient     = EM_SAFE_STRDUP(mmsg->rfc822header->received);

        if (mmsg->rfc822header->from) {
                rfc822_parse_adrlist(&from, mmsg->rfc822header->from, NULL);
                if (!emcore_get_utf8_address(&dst_mail_data->full_address_from, from, &err)) {
                        EM_DEBUG_EXCEPTION_SEC("emcore_get_utf8_address failed : [%d], [%s]", err, mmsg->rfc822header->from);
                }
        }

        if (mmsg->rfc822header->to) {
                rfc822_parse_adrlist(&to, mmsg->rfc822header->to, NULL);
                if (!emcore_get_utf8_address(&dst_mail_data->full_address_to, to, &err)) {
                        EM_DEBUG_EXCEPTION_SEC("emcore_get_utf8_address failed : [%d], [%s]", err, mmsg->rfc822header->to);
                }
        }

        if (mmsg->rfc822header->cc) {
                rfc822_parse_adrlist(&cc, mmsg->rfc822header->cc, NULL);
                if (!emcore_get_utf8_address(&dst_mail_data->full_address_cc, cc, &err)) {
                        EM_DEBUG_EXCEPTION_SEC("emcore_get_utf8_address failed : [%d], [%s]", err, mmsg->rfc822header->cc);
                }
        }

        if (mmsg->rfc822header->bcc) {
                rfc822_parse_adrlist(&bcc, mmsg->rfc822header->bcc, NULL);
                if (!emcore_get_utf8_address(&dst_mail_data->full_address_bcc, bcc, &err)) {
                        EM_DEBUG_EXCEPTION_SEC("emcore_get_utf8_address failed : [%d], [%s]", err, mmsg->rfc822header->bcc);
                }
        }

        if (mmsg->rfc822header->return_path) {
                rfc822_parse_adrlist(&return_path, mmsg->rfc822header->return_path, NULL);
                if (!emcore_get_utf8_address(&dst_mail_data->full_address_return, return_path, &err)) {
                        EM_DEBUG_EXCEPTION_SEC("emcore_get_utf8_address failed : [%d], [%s]", err, mmsg->rfc822header->return_path);
                }
        }

        if (mmsg->rfc822header->reply_to) {
                rfc822_parse_adrlist(&reply_to, mmsg->rfc822header->reply_to, NULL);
                if (!emcore_get_utf8_address(&dst_mail_data->full_address_reply, reply_to, &err)) {
                        EM_DEBUG_EXCEPTION_SEC("emcore_get_utf8_address failed : [%d], [%s]", err, mmsg->rfc822header->reply_to);
                }
        }

	if (emcore_get_first_address(dst_mail_data->full_address_from, &first_alias, &first_address) == true) {
		dst_mail_data->alias_sender = EM_SAFE_STRDUP(first_alias);
		dst_mail_data->email_address_sender = EM_SAFE_STRDUP(first_address);
	}


	EM_SAFE_FREE(encoded_subject);
	EM_SAFE_FREE(first_alias);
	EM_SAFE_FREE(first_address);

	if (from)
		mail_free_address(&from);

	if (to)
		mail_free_address(&to);

	if (cc)
		mail_free_address(&cc);

	if (bcc)
		mail_free_address(&bcc);

	if (return_path)
		mail_free_address(&return_path);

	if (reply_to)
		mail_free_address(&reply_to);

        EM_DEBUG_FUNC_END("err : [%d]", err);
        return err;
}

INTERNAL_FUNC int emcore_make_mail_data_from_mime_data(struct _m_mesg *mmsg, 
													struct _m_content_info *cnt_info, 
													email_mail_data_t **output_mail_data, 
													email_attachment_data_t **output_attachment_data, 
													int *output_attachment_count, 
													int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int i = 0;
	int ret = false;
	int eml_mail_id = 0;
	int err = EMAIL_ERROR_NONE;
	int save_status = EMAIL_BODY_DOWNLOAD_STATUS_NONE;
	int attachment_num = 0;
	int local_attachment_count = 0;
	int local_inline_content_count = 0;
	char move_buf[512];
	char path_buf[512];
	char html_body[MAX_PATH] = {0, };
    char *multi_user_name = NULL;
    struct timeval tv;
	struct attachment_info *ai = NULL;
	email_attachment_data_t *attachment = NULL;
	email_mail_data_t *p_mail_data = NULL;

	if (!mmsg || !cnt_info || !output_mail_data || !output_attachment_data) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* Create rand mail id of eml */
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);
	eml_mail_id = rand();

	p_mail_data = (email_mail_data_t *)em_malloc(sizeof(email_mail_data_t));
	if (p_mail_data == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	p_mail_data->mail_id = eml_mail_id;
	p_mail_data->account_id = EML_FOLDER;
	p_mail_data->mail_size = cnt_info->total_body_size;

	if ((err = emcore_make_mail_data_from_m_mesg(p_mail_data, mmsg)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_make_mail_data_from_m_mesg failed : [%d]", err);
	}

	EM_DEBUG_LOG("cnt_info->text.plain [%s], cnt_info->text.html [%s]", cnt_info->text.plain, cnt_info->text.html);

	if (cnt_info->text.plain) {
        memset(move_buf, 0x00, sizeof(move_buf));
        memset(path_buf, 0x00, sizeof(path_buf));

		EM_DEBUG_LOG("cnt_info->text.plain [%s]", cnt_info->text.plain);
		if (!emstorage_create_dir(multi_user_name, EML_FOLDER, eml_mail_id, 0, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_get_save_name(multi_user_name, EML_FOLDER, eml_mail_id, 0, 
									cnt_info->text.plain_charset ? cnt_info->text.plain_charset : UNKNOWN_CHARSET_PLAIN_TEXT_FILE, 
									move_buf, path_buf, sizeof(path_buf), &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(cnt_info->text.plain, move_buf, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}

		p_mail_data->file_path_plain = EM_SAFE_STRDUP(path_buf);
		save_status = cnt_info->text.plain_save_status;
	}

	if (cnt_info->text.html) {
        memset(move_buf, 0x00, sizeof(move_buf));
        memset(path_buf, 0x00, sizeof(path_buf));

		if (!emstorage_create_dir(multi_user_name, EML_FOLDER, eml_mail_id, 0, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (cnt_info->text.html_charset != NULL) {
			SNPRINTF(html_body, MAX_PATH, "%s%s", cnt_info->text.html_charset, HTML_EXTENSION_STRING);
		} else {
			strcpy(html_body, UNKNOWN_CHARSET_HTML_TEXT_FILE);
		}

		if (!emstorage_get_save_name(multi_user_name, EML_FOLDER, eml_mail_id, 
									0, html_body, move_buf, path_buf, 
									sizeof(path_buf), &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(cnt_info->text.html, move_buf, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}

		p_mail_data->file_path_html = EM_SAFE_STRDUP(path_buf);
		save_status = cnt_info->text.html_save_status;
	}

	if (cnt_info->text.mime_entity) {
		memset(move_buf, 0x00, sizeof(move_buf));
		memset(path_buf, 0x00, sizeof(path_buf));

		if (!emstorage_create_dir(multi_user_name, EML_FOLDER, eml_mail_id, 0, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_get_save_name(multi_user_name, EML_FOLDER, eml_mail_id,
									0, "mime_entity", move_buf, path_buf,
									sizeof(path_buf), &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(cnt_info->text.mime_entity, move_buf, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}

		p_mail_data->file_path_mime_entity = EM_SAFE_STRDUP(path_buf);
	}

	if ((err = emcore_get_preview_text_from_file(multi_user_name, 
                                                p_mail_data->file_path_plain, 
                                                p_mail_data->file_path_html, 
                                                MAX_PREVIEW_TEXT_LENGTH, 
                                                &p_mail_data->preview_text)) != EMAIL_ERROR_NONE)
		EM_DEBUG_EXCEPTION("emcore_get_preview_text_from_file error [%d]", err);

	for (ai = cnt_info->file; ai; ai = ai->next, local_attachment_count++) {}
	EM_DEBUG_LOG("local_attachment_count : [%d]", local_attachment_count);

	for (ai = cnt_info->inline_file; ai; ai = ai->next, local_inline_content_count++) {}
	EM_DEBUG_LOG("local_inline_content_count : [%d]", local_inline_content_count);

	attachment_num = local_attachment_count + local_inline_content_count;

	if (attachment_num > 0) {
		attachment = (email_attachment_data_t *)em_malloc(sizeof(email_attachment_data_t) * attachment_num);
		if (attachment == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		for (ai = cnt_info->file; ai; ai = ai->next, i++) {
			attachment[i].attachment_id          = i + 1;
			attachment[i].attachment_size        = ai->size;
			attachment[i].attachment_name        = EM_SAFE_STRDUP(ai->name);
			attachment[i].content_id             = EM_SAFE_STRDUP(ai->content_id);
			attachment[i].drm_status             = ai->drm;
			attachment[i].save_status            = ai->save_status;
			attachment[i].inline_content_status  = 0;
			attachment[i].attachment_mime_type   = EM_SAFE_STRDUP(ai->attachment_mime_type);
#ifdef __ATTACHMENT_OPTI__
			attachment[i].encoding               = ai->encoding;
			attachment[i].section                = ai->section;
#endif
			EM_DEBUG_LOG("attachment[%d].attachment_id[%d]", i, attachment[i].attachment_id);
			EM_DEBUG_LOG("attachment[%d].attachment_size[%d]", i, attachment[i].attachment_size);
			EM_DEBUG_LOG_SEC("attachment[%d].attachment_name[%s]", i, attachment[i].attachment_name);
			EM_DEBUG_LOG("attachment[%d].drm_status[%d]", i, attachment[i].drm_status);
			EM_DEBUG_LOG("attachment[%d].inline_content_status[%d]", i, attachment[i].inline_content_status);
			EM_DEBUG_LOG("attachment[%d].attachment_mime_type : [%s]", i, attachment[i].attachment_mime_type);

			if (ai->save) {
	        memset(move_buf, 0x00, sizeof(move_buf));
            memset(path_buf, 0x00, sizeof(path_buf));

			if (!emstorage_create_dir(multi_user_name, EML_FOLDER, eml_mail_id, i + 1, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
					goto FINISH_OFF;
				}

				if (!emstorage_get_save_name(multi_user_name, EML_FOLDER, eml_mail_id, 
											i + 1, attachment[i].attachment_name, move_buf, 
											path_buf, sizeof(path_buf), &err))  {
					EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
					goto FINISH_OFF;
				}

				if (!emstorage_move_file(ai->save, move_buf, false, &err))  {
					EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);

					/*  delete all created files. */
					if (!emstorage_get_save_name(multi_user_name, EML_FOLDER, eml_mail_id, 
												0, NULL, move_buf, path_buf, 
												sizeof(path_buf), NULL)) {
						EM_DEBUG_EXCEPTION("emstorage_get_save_name failed...");
						/* goto FINISH_OFF; */
					}

					if (!emstorage_delete_dir(move_buf, NULL)) {
						EM_DEBUG_EXCEPTION("emstorage_delete_dir failed...");
						/* goto FINISH_OFF; */
					}


					goto FINISH_OFF;
				}

				attachment[i].attachment_path = EM_SAFE_STRDUP(path_buf);
			}

			EM_DEBUG_LOG_SEC("attachment[%d].attachment_path[%s]", i, attachment[i].attachment_path);
			save_status = ai->save_status;
		}

		for (ai = cnt_info->inline_file; ai; ai = ai->next, i++) {
			attachment[i].attachment_id          = i + 1;
			attachment[i].attachment_size        = ai->size;
			attachment[i].attachment_name        = EM_SAFE_STRDUP(ai->name);
			attachment[i].content_id             = EM_SAFE_STRDUP(ai->content_id);
			attachment[i].drm_status             = ai->drm;
			attachment[i].save_status            = ai->save_status;
			attachment[i].inline_content_status  = INLINE_ATTACHMENT;
			attachment[i].attachment_mime_type   = EM_SAFE_STRDUP(ai->attachment_mime_type);
#ifdef __ATTACHMENT_OPTI__
			attachment[i].encoding               = ai->encoding;
			attachment[i].section                = ai->section;
#endif
			EM_DEBUG_LOG("attachment[%d].attachment_id[%d]", i, attachment[i].attachment_id);
			EM_DEBUG_LOG("attachment[%d].attachment_size[%d]", i, attachment[i].attachment_size);
			EM_DEBUG_LOG_SEC("attachment[%d].attachment_name[%s]", i, attachment[i].attachment_name);
			EM_DEBUG_LOG("attachment[%d].drm_status[%d]", i, attachment[i].drm_status);
			EM_DEBUG_LOG("attachment[%d].inline_content_status[%d]", i, attachment[i].inline_content_status);
			EM_DEBUG_LOG("attachment[%d].attachment_mime_type : [%s]", i, attachment[i].attachment_mime_type);

			if (ai->save) {
                memset(move_buf, 0x00, sizeof(move_buf));
                memset(path_buf, 0x00, sizeof(path_buf));

				if (!emstorage_create_dir(multi_user_name, EML_FOLDER, eml_mail_id, 0, &err))  {
					EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
					goto FINISH_OFF;
				}

				if (!emstorage_get_save_name(multi_user_name, EML_FOLDER, eml_mail_id, 0, 
											attachment[i].attachment_name, move_buf, path_buf, 
											sizeof(path_buf), &err))  {
					EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
					goto FINISH_OFF;
				}

				if (!emstorage_move_file(ai->save, move_buf, false, &err))  {
					EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);

					/*  delete all created files. */
					if (!emstorage_get_save_name(multi_user_name, EML_FOLDER, eml_mail_id, 0, 
												NULL, move_buf, path_buf, sizeof(path_buf), NULL)) {
						EM_DEBUG_EXCEPTION("emstorage_get_save_name failed...");
						/* goto FINISH_OFF; */
					}

					if (!emstorage_delete_dir(move_buf, NULL)) {
						EM_DEBUG_EXCEPTION("emstorage_delete_dir failed...");
						/* goto FINISH_OFF; */
					}


					goto FINISH_OFF;
				}

				attachment[i].attachment_path = EM_SAFE_STRDUP(path_buf);
			}

			EM_DEBUG_LOG_SEC("attachment[%d].attachment_path[%s]", i, attachment[i].attachment_path);
			save_status = ai->save_status;
		}
	}
	EM_DEBUG_LOG("Check #1");
	EM_DEBUG_LOG("save_status : [%d]", save_status);

	p_mail_data->attachment_count = local_attachment_count;
	p_mail_data->inline_content_count = local_inline_content_count;
	p_mail_data->body_download_status = (save_status == EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED) ? 1 : 0;

	eml_data_count += 1;
	ret = true;

FINISH_OFF:

	if (ret) {
		if (output_mail_data)
			*output_mail_data = p_mail_data;

		if (output_attachment_data)
			*output_attachment_data = attachment;

		if (output_attachment_count)
			*output_attachment_count = attachment_num;
	} else {
		if (p_mail_data) {
			emcore_free_mail_data(p_mail_data);
			EM_SAFE_FREE(p_mail_data);
		}

		if (attachment)
			emcore_free_attachment_data(&attachment, attachment_num, NULL);
	}


	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret : [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emcore_parse_mime_file_to_mail(char *eml_file_path, 
												email_mail_data_t **output_mail_data, 
												email_attachment_data_t **output_attachment_data, 
												int *output_attachment_count, 
												int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("eml_file_path : [%s], output_mail_data : [%p]", eml_file_path, output_mail_data);

	int err = EMAIL_ERROR_NONE;
	int ret = false;
	char *mime_entity = NULL;
	struct _m_content_info *cnt_info = NULL;
	struct _m_mesg *mmsg = NULL;


	if (!eml_file_path || !output_mail_data || !output_attachment_data || !output_attachment_count) {
		EM_DEBUG_EXCEPTION("Invalid paramter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	cnt_info = (struct _m_content_info *)em_malloc(sizeof(struct _m_content_info));
	if (cnt_info == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	cnt_info->grab_type = GRAB_TYPE_TEXT | GRAB_TYPE_ATTACHMENT;

	mmsg = (struct _m_mesg *)em_malloc(sizeof(struct _m_mesg));
	if (mmsg == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	mmsg->rfc822header = (struct _rfc822header *)em_malloc(sizeof(struct _rfc822header));
	if (mmsg->rfc822header == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	emcore_gmime_init();
	if (!emcore_gmime_eml_parse_mime(eml_file_path, mmsg->rfc822header, cnt_info, &err)) {
		EM_DEBUG_EXCEPTION("emcore_gmime_parse_mime failed : [%d]", err);
		err = EMAIL_ERROR_INVALID_DATA;
		emcore_gmime_shutdown();
		goto FINISH_OFF;
	}
	emcore_gmime_shutdown();

	if (!emcore_make_mail_data_from_mime_data(mmsg, 
											cnt_info, 
											output_mail_data, 
											output_attachment_data, 
											output_attachment_count, 
											&err)) {
		EM_DEBUG_EXCEPTION("emcore_make_mail_tbl_data_from_mime failed : [%d]", err);
		goto FINISH_OFF;

	}

	ret = true;

FINISH_OFF:

	if (mmsg)
		emcore_mime_free_mime(mmsg);

	if (cnt_info) {
		emcore_free_content_info(cnt_info);
		EM_SAFE_FREE(cnt_info);
	}

	EM_SAFE_FREE(mime_entity);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("err : %d", err);
	return ret;
}

INTERNAL_FUNC int emcore_delete_parsed_data(char *multi_user_name, email_mail_data_t *input_mail_data, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data : [%p]", input_mail_data);
	int err = EMAIL_ERROR_NONE;
	int ret = false;
	char buf[MAX_PATH] = {0};
    char *prefix_path = NULL;

	if (!input_mail_data) {
		EM_DEBUG_EXCEPTION("Invliad parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((input_mail_data->account_id != EML_FOLDER) && (!input_mail_data->mail_id)) {
		EM_DEBUG_EXCEPTION("Invliad parameter: account_id[%d], mail_id[%d]", input_mail_data->account_id, input_mail_data->mail_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

    if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		err = emcore_get_container_path(multi_user_name, &prefix_path);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_container_path failed :[%d]", err);
			goto FINISH_OFF;
		}
	} else {
		prefix_path = strdup("");
	}

	if (!input_mail_data->mail_id) {
		SNPRINTF(buf, sizeof(buf), "%s%s%s%d", prefix_path, MAILHOME, DIR_SEPERATOR, input_mail_data->account_id);
	} else {
		SNPRINTF(buf, sizeof(buf), "%s%s%s%d%s%d", prefix_path, MAILHOME, DIR_SEPERATOR, input_mail_data->account_id, DIR_SEPERATOR, input_mail_data->mail_id);
	}

	if (!emstorage_delete_dir(buf, &err)) {
		EM_DEBUG_LOG("emstorage_delete_dir failed : buf[%s]", buf);
			memset(buf, 0x00, sizeof(buf));
        	SNPRINTF(buf, sizeof(buf), "%s%s%d", MAILHOME, DIR_SEPERATOR, input_mail_data->account_id);

			if (!emstorage_delete_dir(buf, &err)) {
 			   EM_DEBUG_EXCEPTION("emstorage_delete_dir failed : buf[%s]", buf);
		   		goto FINISH_OFF;
			}

		err = EMAIL_ERROR_NONE;
	}

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(prefix_path);

	if (err_code)
		*err_code = err;

	return ret;
}

INTERNAL_FUNC int emcore_get_mime_entity(char *mime_path, char **output_path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mime_path : [%s], output_path : [%p]", mime_path, output_path);
	int err = EMAIL_ERROR_NONE;
	int ret = false;
	int fd = 0;
	char *tmp_path = NULL;

	if (!mime_path) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	err = em_open(mime_path, O_RDONLY, 0, &fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_open failed");
		goto FINISH_OFF;
	}

	emcore_gmime_init();
	tmp_path = emcore_gmime_get_mime_entity(fd);
	emcore_gmime_shutdown();

	if (tmp_path == NULL) {
		EM_DEBUG_EXCEPTION("emcore_gmime_get_mime_entity failed");
		err = EMAIL_ERROR_INVALID_DATA;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	
	EM_SAFE_CLOSE(fd);

	if (err_code)
		*err_code = err;

	if (output_path)
		*output_path = tmp_path;

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_get_digest_type(char *micalg_value)
{
	EM_DEBUG_FUNC_BEGIN();
	int digest_type = DIGEST_TYPE_NONE;

	if (micalg_value == NULL)
		return DIGEST_TYPE_NONE;

	if (strcasestr(micalg_value, "sha1"))
		digest_type = DIGEST_TYPE_SHA1;
	else if (strcasestr(micalg_value, "md5"))
		digest_type = DIGEST_TYPE_MD5;
	else if (strcasestr(micalg_value, "ripemd160"))
		digest_type = DIGEST_TYPE_RIPEMD160;
	else if (strcasestr(micalg_value, "md2"))
		digest_type = DIGEST_TYPE_MD2;
	else if (strcasestr(micalg_value, "tiger192"))
		digest_type = DIGEST_TYPE_TIGER192;
	else if (strcasestr(micalg_value, "haval5160"))
		digest_type = DIGEST_TYPE_HAVAL5160;
	else if (strcasestr(micalg_value, "sha256"))
		digest_type = DIGEST_TYPE_SHA256;
	else if (strcasestr(micalg_value, "sha384"))
		digest_type = DIGEST_TYPE_SHA384;
	else if (strcasestr(micalg_value, "sha512"))
		digest_type = DIGEST_TYPE_SHA512;
	else if (strcasestr(micalg_value, "sha224"))
		digest_type = DIGEST_TYPE_SHA224;
	else if (strcasestr(micalg_value, "md4"))
		digest_type = DIGEST_TYPE_MD4;
	else
		digest_type = DIGEST_TYPE_NONE;

	EM_DEBUG_FUNC_END();
	return digest_type;
}
