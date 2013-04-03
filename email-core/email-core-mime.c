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
#include "email-internal-types.h"
#include "lnx_inc.h"
#include "email-utilities.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-core-mail.h"
#include "email-core-mime.h"
#include "email-storage.h"
#include "email-core-event.h"
#include "email-core-account.h"
#include "email-core-signal.h"
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

#define GRAB_TYPE_TEXT       1	/*  retrieve text and attachment name */
#define GRAB_TYPE_ATTACHMENT 2	/*  retrieve only attachmen */

#define SAVE_TYPE_SIZE       1	/*  only get content siz */
#define SAVE_TYPE_BUFFER     2	/*  save content to buffe */
#define SAVE_TYPE_FILE       3	/*  save content to temporary fil */

#define EML_FOLDER           20 /*  save eml content to temporary folder */

/*
	MIME Structure Example

	(part 0)	multipart/mixed
	(part 1)		multipart/alternative
	(part 1.1)			text/plain		<- text message
	(part 1.2)			text/html		<- html message
	(part 2)		text/plain			<- text attachment


	(part 0)	multipart/related
	(part 1)		multipart/alternative
	(part 1.1)			text/plain			<- text message
	(part 1.2)			text/html			<- html message
	(part 2)		image/png				<- inline image
	(part 2)		image/png				<- inline image


	(part 0)	multipart/mixed
	(part 1.1)		multipart/related
	(part 2.1)			multipart/alternative
	(part 3.1)				text/plain(body)  <- text message
	(part 3.2)				text/html(body)	<- html message
	(part 2.2)			image/png(related)	<- inline image
	(part 1.2)		image/png(attachment)	<- image attachment
*/

/*  Text and Attachment Holde */
/* struct _m_content_info  */
/* 	int grab_type;	*/		/*  1 :  text and attachment list */
						/*  2 :  attachmen */
/* 	int file_no; */			/*  attachment sequence to be downloaded (min : 1 */
/* 	struct text_data  */
/* 		char *plain;	*/	/*  body plain tex */
/* 		char *plain_charset */ /*  charset of plai */
/* 		char *html; */ /*  body html tex */
/* 	} text */

/* 	struct attachment_info  */
/* 		int   type;	*/		/*  1 : inline 2 : attachmen */
/* 		char *name;	*/		/*  attachment filenam */
/* 		int   size;		*/	/*  attachment siz */
/* 		char *save;	*/		/*  content saving filenam */
/* 		struct attachment_info *next */
/* 	} *file */
/* } */

/* --------------------- MIME Structure --------------------------------- */
/*  MIME Header Parameter (Content-Type, Content-Disposition, ... */
struct _parameter {
	char				*name;			/*  parameter name */
	char				*value;			/*  parameter valu */
	struct _parameter	*next;			/*  next paramete */
};

/*  Content-Dispositio */
struct _disposition {
	char				*type;			/*  "inline" "attachment */
	struct _parameter	*parameter;		/*  "filename", .. */
};

/*  RFC822 Heade */
struct _rfc822header {
	char				*return_path;	/*  error return pat */
	char				*received;
	char				*date;
	char				*from;
	char				*subject;
	char				*sender;
	char				*to;
	char				*cc;
	char				*bcc;
	char				*reply_to;
	char				*priority;
	char				*ms_priority;
	char				*dsp_noti_to;
	char				*others;
};

/*  MIME Part Header */
struct _m_part_header {
	char				*type;			/*  text, image, audio, video, application, multipart, message */
	char				*subtype;		/*  plain, html, jpeg, .. */
	char				*encoding;		/*  encoding typ */
	struct _parameter	*parameter;		/*  content-type parameter  :  "boundary" "charset" .. */
	char				*desc;			/*  descriptio */
	char				*disp_type;		/*  disposition type  :  "inline" "attachment", */
	struct _parameter	*disp_parameter;    /*  disposition parameter  :  "filename", .. */
	char				*content_id;	/*  content id  :  it is inline  */
	char				*content_location;	/*  content location  :  "inline" location  */
};

/*  MIME Message Header */
struct _m_mesg_header {
	char				*version;		/*  MIME Versio */
	struct _m_part_header  *part_header;	/*  MIME Part Heade */
};

/*  MIME Multipart Body linked list */
typedef struct _m_body _m_body_t;
struct _m_part{
	_m_body_t			*body;			/*  part bod */
	struct _m_part		*next;			/*  the next found par */
};

/*  MIME Multipart Body */
struct _m_body {
	struct _m_part_header *part_header;	/*  MIME Part Heade */
	struct _m_part			nested;			/*  nested structure if contain multipar */
	char				*text;			/*  text if not contain multipar */
	int						size;			/*  text size if not contain multipar */
	char				*holdingfile;
};

/*  MIME Message */
struct _m_mesg {
	struct _rfc822header  *rfc822header;	/*  RFC822 Heade */
	struct _m_mesg_header *header;			/*  MIME Message Heade */
	struct _m_part		nested;			/*  nested structure if contain multipar */
	char				*text;			/*  text if not contain multipar */
	int						size;			/*  text size if not contain multipar */
};
/* ---------------------------------------------------------------------- */
/*  Global variable */
static bool next_decode_string = false;
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
static BODY **g_inline_list;
static int g_inline_count;

/*  Function Declaration */

/*  parsing the first header (RFC822 Header and Message Header and Etc... */
static int emcore_mime_parse_header(void *stream, int is_file, struct _rfc822header **rfcheader, struct _m_mesg_header **header, int *err_code);

/*  parsing the first bod */
static int emcore_mime_parse_body(void *stream, int is_file, struct _m_mesg *mmsg, struct _m_content_info *cnt_info, void *callback, int *err_code);

/*  parsing the message part header (CONTENT-... header */
static int emcore_mime_parse_part_header(void *stream, int is_file, struct _m_part_header **header, int *err_code);

/*  parsing the message part by boundar */
static int emcore_mime_parse_part(void *stream, int is_file, struct _m_part_header *parent_header, struct _m_part *nested, struct _m_content_info *cnt_info, int *end_of_parsing, int *err_code);

/*  set RFC822 Header valu */
static int emcore_mime_set_rfc822_header_value(struct _rfc822header **rfc822header, char *name, char *value, int *err_code);

/*  set Message Part Header (Content-... Header */
static int emcore_mime_set_part_header_value(struct _m_part_header **header, char *name, char *value, int *err_code);

static int emcore_mime_get_param_from_str(char *str, struct _parameter **param, int *err_code);

static int emcore_mime_add_param_to_list(struct _parameter **param_list, struct _parameter *param, int *err_code);

char *emcore_mime_get_header_value(struct _m_part_header *header, int type, int *err_code);

void emcore_mime_free_param(struct _parameter *param);
void emcore_mime_free_part_header(struct _m_part_header *header);
void emcore_mime_free_message_header(struct _m_mesg_header *header);
void emcore_mime_free_rfc822_header(struct _rfc822header *rfc822header);
void emcore_mime_free_part(struct _m_part *part);
void emcore_mime_free_part_body(struct _m_body *body);
void emcore_mime_free_mime(struct _m_mesg *mmsg);
char *emcore_mime_get_line_from_sock(void *stream, char *buf, int size, int *err_code);
char *emcore_mime_get_save_file_name(int *err_code);

/*  get content data and save buffer or fil */
/*  mode - 1 :  get the only siz */
/*         2 :  save content to buffer (holder is buffer */
/*         3 :  save content to file (holder is file name */
static int emcore_mime_get_content_data(void *stream,
					int  is_file,
                       int   is_text,
					char *boundary_str,
					char *content_encoding,
					int *end_of_parsing,
					int   mode,
					char **holder,
					int *size,
					void *callback,
					int *err_code);

int emcore_decode_body_text_from_sock(void *stream, char *boundary_str, int encoding, int mode, int is_text, int fd, char **holder, int *end_of_parsing, int *size);
int emcore_decode_body_text_from_file(FILE *stream, char *boundary_str, int encoding, int mode, int is_text, int fd, char **holder, int *end_of_parsing, int *size);
/*  skip content data to boundary_str or end of fil */
int emcore_mime_skip_content_data(void *stream,
							int is_file,
							char *boundary_str,
							int *end_of_parsing,
							int *size,
							void *callback,
							int *err_code);

static int emcore_get_file_pointer(BODY *body, bool input_check_duplicated_file_name, char *buf, struct _m_content_info *cnt_info , int *err);
static PARTLIST *emcore_get_body_full(MAILSTREAM *stream, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code, PARTLIST *section_list);
static int emcore_get_body_part_imap_full(MAILSTREAM *stream, int msg_uid, int account_id, int mail_id, PARTLIST *section_list, struct _m_content_info *cnt_info, int *err_code, int event_handle);
static PARTLIST *emcore_add_node(PARTLIST *section_list, BODY *body);
static void emcore_free_section_list(PARTLIST *section_list);
static int emcore_get_section_body_size(char *response, char *section, int *body_size);
static void parse_file_path_to_filename(char *src_string, char **out_string);
extern long pop3_reply (MAILSTREAM *stream);
/* ------------------------------------------------------------------------------------------------- */


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
	content_id = temp_str;
	temp_cid = strstr(temp_str, "\"");

	if (temp_cid == NULL) {
		EM_DEBUG_EXCEPTION(">>>> File Path Doesnot contain end line for CID ");
 		next_decode_string = true;
		EM_SAFE_FREE(buf);
		return temp_str;
	}
	temp_cid_data = em_malloc((temp_cid-temp_str)+1);
	memcpy(temp_cid_data, temp_str, (temp_cid-temp_str));

	if (!strstr(temp_cid_data, delims)) {
		EM_DEBUG_EXCEPTION(">>>> File Path Doesnot contain @ ");
		next_decode_string = true;
		EM_SAFE_FREE(buf);
		EM_SAFE_FREE(temp_cid_data);
		return temp_str;
	}
	else		{
		result = strstr(temp_str, delims);
		if (result != NULL) {
			next_decode_string = false;
			*result = '\0';
			result++;
			EM_DEBUG_LOG("content_id is [ %s ]", content_id);

			if (strcasestr(content_id, ".bmp") || strcasestr(content_id, ".jpeg") || strcasestr(content_id, ".png") ||
					strcasestr(content_id, ".jpg") || strcasestr(content_id, ".gif"))
				snprintf(buf+EM_SAFE_STRLEN(buf), buf_length - EM_SAFE_STRLEN(buf), "%s\"", content_id);
			else
				snprintf(buf+EM_SAFE_STRLEN(buf), buf_length - EM_SAFE_STRLEN(buf), "%s%s", content_id, ".jpeg\"");
		}
		else {
			EM_DEBUG_EXCEPTION(">>>> File Path Doesnot contain end line for CID ");
			next_decode_string = true;
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


static char *em_replace_string_with_split_file_path(char *source_string, char *old_string, char *new_string)
{
	EM_DEBUG_FUNC_BEGIN();
	char *buffer = NULL;
	char *split_str = NULL;
	char *p = NULL;
	char *q = NULL;
	int   buf_len = 0;

	EM_IF_NULL_RETURN_VALUE(source_string, NULL);
	EM_IF_NULL_RETURN_VALUE(old_string, NULL);
	EM_IF_NULL_RETURN_VALUE(new_string, NULL);

	EM_DEBUG_LOG("source_string [%s] ", source_string);
	EM_DEBUG_LOG("old_string    [%s] ", old_string);
	EM_DEBUG_LOG("new_string    [%s] ", new_string);

	p = strstr(source_string, old_string);

	if (!p) {
		EM_DEBUG_EXCEPTION("Orig not found in source_string");
		return NULL;
	}

	buf_len = EM_SAFE_STRLEN(source_string) + 1024;
	buffer = (char *)em_malloc(buf_len);

	if(p - source_string < EM_SAFE_STRLEN(source_string) + 1024 + 1) {
		strncpy(buffer, source_string, p - source_string);

		EM_DEBUG_LOG("BUFFER [%s]", buffer);

		split_str = em_split_file_path(p);

		if (!split_str) {
			EM_DEBUG_EXCEPTION(">> SPLIT STRING IS NULL  ");
			goto FINISH_OFF;
		}

		q = strstr(split_str, old_string);
		if (q) {
			EM_DEBUG_LOG("Split string [%s]", split_str);
			snprintf(buffer + EM_SAFE_STRLEN(buffer), buf_len - EM_SAFE_STRLEN(buffer), "%s%s", new_string, q+strlen(old_string)); /*prevent 34353*/
			EM_DEBUG_LOG("BUFFER 1 [%s]", buffer);
			EM_SAFE_FREE(split_str);
			EM_DEBUG_FUNC_END("Suceeded");
			return buffer;
		}
	}
	else  {
		EM_DEBUG_EXCEPTION("Buffer is too small.");
		EM_SAFE_FREE(buffer);
		return NULL;
	}

FINISH_OFF:
	EM_SAFE_FREE(split_str);
	EM_SAFE_FREE(buffer);

	EM_DEBUG_FUNC_END("Failed");
	return NULL;

}


int emcore_mime_flush_receiving_buffer(void *stream, int is_file, char *boundary_string, char *boundary_end_string,  int *end_of_parsing, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], is_file[%d], boundary_string[%s], boundary_end_string[%s], end_of_parsing[%p],  err_code[%p]", stream, is_file, boundary_string, boundary_end_string, end_of_parsing, err_code);
	int err = EMAIL_ERROR_NONE;
	int local_end_of_parsing = 0;
	char buf[MIME_LINE_LEN] = {0, };

	if (!stream) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	while (TRUE)  {
		if (!emcore_check_thread_status()) {
			EM_DEBUG_FUNC_END("EMAIL_ERROR_CANCELLED");
			if (err_code)
				*err_code = EMAIL_ERROR_CANCELLED;
			return false;
		}

		if ((is_file == 0 && !emcore_mime_get_line_from_sock(stream, buf, MIME_LINE_LEN, &err)) ||
			(is_file == 1 && !emcore_get_line_from_file(stream, buf, MIME_LINE_LEN, &err))) {
			if (err != EMAIL_ERROR_NO_MORE_DATA) {
				EM_DEBUG_EXCEPTION("file : [%d], emcore_mime_get_line_from_sock or emcore_mime_get_line_from_file failed", is_file);
				local_end_of_parsing = 0;
				break;
			}
	
			local_end_of_parsing = 1;
			break;
		}

		if (boundary_string && boundary_end_string) {
			if (!strcmp(buf, boundary_string))  {
				EM_DEBUG_LOG("found boundary");
				local_end_of_parsing = 0;
				break;
			}
			else if (!strcmp(buf, boundary_end_string))  {
				EM_DEBUG_LOG("found boundary_end");
				local_end_of_parsing = 1;
				break;
			}
		}
	}

	if (end_of_parsing)
		*end_of_parsing = local_end_of_parsing;

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return true;
}

int emcore_parse_mime(void *stream, int is_file, struct _m_content_info *cnt_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], is_file[%d], cnt_info[%p], err_code[%p]", stream, is_file, cnt_info, err_code);

	struct _m_mesg *mmsg = em_malloc(sizeof(struct _m_mesg));

	if (!mmsg) return false;

	memset(mmsg, 0x00, sizeof(struct _m_mesg));

	/*  1. parse the first found header */
	EM_DEBUG_LOG(">>>>>> 1. parse the first found header");
	if (!emcore_mime_parse_header(stream, is_file, &mmsg->rfc822header, &mmsg->header, err_code)) {
		EM_SAFE_FREE(mmsg);
		return false;
	}

	if (!emcore_check_thread_status())  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_CANCELLED;
		emcore_mime_free_mime(mmsg);
		return false;
	}

	/*  2. parse body */
	EM_DEBUG_LOG(">>>>>> 2. parse body");
	if (mmsg && mmsg->header && mmsg->header->part_header && mmsg->header->part_header->parameter)  {
		EM_DEBUG_LOG("name[%s]", mmsg->header->part_header->parameter->name);
		EM_DEBUG_LOG("value[%s]", mmsg->header->part_header->parameter->value);
		EM_DEBUG_LOG("next  :  %p", mmsg->header->part_header->parameter->next);
	}

	if (!emcore_mime_parse_body(stream, is_file, mmsg, cnt_info, NULL, err_code)) {
		EM_SAFE_FREE(mmsg);
		return false;
	}

	/*  3. free memory */
	EM_DEBUG_LOG(">>>>>> 3. free memory");
	if (mmsg && mmsg->header && mmsg->header->part_header && mmsg->header->part_header->parameter)  {
		EM_DEBUG_LOG("name[%s]", mmsg->header->part_header->parameter->name);
		EM_DEBUG_LOG("value[%s]", mmsg->header->part_header->parameter->value);
		EM_DEBUG_LOG("next  :  %p", mmsg->header->part_header->parameter->next);
	}
	emcore_mime_free_mime(mmsg);
	EM_DEBUG_FUNC_END();
	return true;
}

int emcore_mime_parse_header(void *stream, int is_file, struct _rfc822header **rfc822header, struct _m_mesg_header **header, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], is_file[%d], rfc822header[%p], header[%p], err_code[%p]", stream, is_file, rfc822header, header, err_code);

	struct _m_mesg_header *tmp_header = NULL;
	struct _rfc822header *tmp_rfc822header = NULL;
	char buf[MIME_LINE_LEN] = {0, };
	char *name = NULL;
	char *value = NULL;
	char *pTemp = NULL;
	int  is_longheader;

	if (!emcore_check_thread_status())  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_CANCELLED;
			return false;
		}


	if ((is_file == 0 && !emcore_mime_get_line_from_sock(stream, buf, MIME_LINE_LEN, err_code)) ||
		(is_file == 1 && !emcore_get_line_from_file(stream, buf, MIME_LINE_LEN, err_code)))  {
		return false;
	}

	if (!(tmp_header = em_malloc(sizeof(struct _m_mesg_header))))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_OUT_OF_MEMORY;
		return false;
	}

	if (!emcore_check_thread_status())  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_CANCELLED;
			return false;
		}

	while (TRUE)  {
		EM_DEBUG_LOG("buf[%s]", buf);

		if (!strncmp(buf, CRLF_STRING, 2))
			break;

		is_longheader = (buf[0] == ' ' || buf[0] == '\t') ? TRUE  :  FALSE;


		if (!is_longheader)  { /*  Normal header (format :  "Name :  Value" or "Name :  Value; Parameters" */
			if (name)  {
				EM_SAFE_FREE(name);
			}

			/* EM_DEBUG_FUNC_BEGIN() */
			if ((pTemp = strtok(buf, ":")) == NULL)
				break;

			name = EM_SAFE_STRDUP(pTemp);

			value = strtok(NULL, CRLF_STRING);

			em_upper_string(name);
		}
		else  { /*  Long header */
			value = strtok(buf, CRLF_STRING);
			em_trim_left(value);
		}

		/* --> 2007-05-08 by cy */
		if (!name)
			break;

		EM_DEBUG_LOG("> name[%s]", name);
		EM_DEBUG_LOG("> value[%s]", value);

		/*  MIME Part Heade */
		if (memcmp(name, "CONTENT-", 8) == 0 && value)  {
			EM_DEBUG_LINE;
			emcore_mime_set_part_header_value(&tmp_header->part_header, name, value, err_code);

			if (tmp_header->part_header && tmp_header->part_header->parameter)	{
				EM_DEBUG_LOG("name[%s]", tmp_header->part_header->parameter->name);
				EM_DEBUG_LOG("value[%s]", tmp_header->part_header->parameter->value);
				EM_DEBUG_LOG("next  :  %p", tmp_header->part_header->parameter->next);
			}

			/*  MIME Version Heade */
		}
		else if (memcmp(name, "MIME-VERSION", 12) == 0)  {
			/* EM_DEBUG_FUNC_BEGIN() */
			/*  ignored because we need only contents information */
			/*  tmp_header->version = EM_SAFE_STRDUP(value) */

			/*  RFC822 Heade */
		}
		else  {
			/*  in socket stream case, ignored because we need only contents information */
			if (is_file == 1)
				emcore_mime_set_rfc822_header_value(&tmp_rfc822header, name, value, err_code);
		}

		if (!emcore_check_thread_status())  {
			if (err_code != NULL)
				*err_code = EMAIL_ERROR_CANCELLED;
			return false;
		}

		if ((is_file == 0 && !emcore_mime_get_line_from_sock(stream, buf, MIME_LINE_LEN, err_code)) ||
			(is_file == 1 && !emcore_get_line_from_file(stream, buf, MIME_LINE_LEN, err_code)))  {

			if (tmp_rfc822header)
				emcore_mime_free_rfc822_header(tmp_rfc822header);


			if (tmp_header)  {
				emcore_mime_free_part_header(tmp_header->part_header);

				EM_SAFE_FREE(tmp_header->version);
				EM_SAFE_FREE(tmp_header);
			}
			return false;
		}
	}

	*header = tmp_header;
	*rfc822header = tmp_rfc822header;

	EM_SAFE_FREE(name);
	EM_DEBUG_FUNC_END();
	return true;
}

int emcore_mime_parse_part_header(void *stream, int is_file, struct _m_part_header **header, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], is_file[%d], header[%p], err_code[%p]", stream, is_file, header, err_code);

	struct _m_part_header *tmp_header = NULL;
	char buf[MIME_LINE_LEN] = {0x00};
	char *name = NULL;
	char *value = NULL;
	char *p = NULL;
	int is_longheader = false;

	if (!emcore_check_thread_status())  {
			if (err_code != NULL)
				*err_code = EMAIL_ERROR_CANCELLED;
			return false;
	}

	if ((is_file == 0 && !emcore_mime_get_line_from_sock(stream, buf, MIME_LINE_LEN, err_code)) ||
	(is_file == 1 && !emcore_get_line_from_file(stream, buf, MIME_LINE_LEN, err_code)))
		return false;

	tmp_header = em_malloc(sizeof(struct _m_part_header));

	if (!tmp_header)  {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		return false;
	}

	memset(tmp_header, 0, sizeof(struct _m_part_header));

	while (true)  {
		if (!strncmp(buf, CRLF_STRING, EM_SAFE_STRLEN(CRLF_STRING))) break;

		is_longheader = (buf[0] == ' ' || buf[0] == TAB);

		if (!is_longheader)  {   /*  Normal header (format :  "Name :  Value" or "Name :  Value; Parameters" */
			EM_SAFE_FREE(name);
			p = strtok(buf , ":");

			if (p)  {
				name = EM_SAFE_STRDUP(p);
				value = strtok(NULL, CRLF_STRING);
				em_upper_string(name);
			}
		}
		else		/*  Long header */
			value = strtok(buf, CRLF_STRING);

		if (!name)
			break;

		emcore_mime_set_part_header_value(&tmp_header, name, value, err_code);

		if (!emcore_check_thread_status())  {
			if (err_code != NULL)
				*err_code = EMAIL_ERROR_CANCELLED;
			return false;
		}

		if ((is_file == 0 && !emcore_mime_get_line_from_sock(stream, buf, MIME_LINE_LEN, err_code)) ||
		(is_file == 1 && !emcore_get_line_from_file(stream, buf, MIME_LINE_LEN, err_code)))  {
			EM_SAFE_FREE(name);
			EM_SAFE_FREE(tmp_header);

			return false;
		}
	}

	*header = tmp_header;

	EM_SAFE_FREE(name);
	EM_DEBUG_FUNC_END();
	return true;
}


int emcore_mime_parse_body(void *stream, int is_file, struct _m_mesg *mmsg, struct _m_content_info *cnt_info,  void *callback, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], is_file[%d], mmsg[%p], cnt_info[%p], callback[%p], err_code[%p]", stream, is_file, mmsg, cnt_info, callback, err_code);

	if (!mmsg->header) {
		EM_DEBUG_EXCEPTION("Invalid paramter");
		*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	} 
	
	char *content_type = NULL, *content_encoding = NULL, *holder = NULL, *attachment_name, *t = NULL;
	int type = 0, end_of_parsing = 0, size;
	int err = EMAIL_ERROR_NONE;

	if (!emcore_check_thread_status())  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_CANCELLED;
		return false;
	}

	if (mmsg->header)
		content_type = emcore_mime_get_header_value(mmsg->header->part_header, CONTENT_TYPE, err_code);
	if (!content_type)
		content_type = "TEXT/PLAIN";

	if (mmsg->header)
		content_encoding = emcore_mime_get_header_value(mmsg->header->part_header, CONTENT_ENCODING, err_code);
	if (!content_encoding)
		content_encoding = "7BIT";

	if (strstr(content_type, TEXT_STR)) type = TYPE_TEXT;
	else if (strstr(content_type, IMAGE_STR)) type = TYPE_IMAGE;
	else if (strstr(content_type, AUDIO_STR)) type = TYPE_AUDIO;
	else if (strstr(content_type, VIDEO_STR)) type = TYPE_VIDEO;
	else if (strstr(content_type, APPLICATION_STR)) type = TYPE_APPLICATION;
	else if (strstr(content_type, MULTIPART_STR)) type = TYPE_MULTIPART;
	else if (strstr(content_type, MESSAGE_STR)) type = TYPE_MESSAGE;
	else type = TYPE_UNKNOWN;

	switch (type)  {
		case TYPE_MULTIPART:
			if (mmsg->header && !emcore_mime_get_header_value(mmsg->header->part_header, CONTENT_BOUNDARY, &err))  {
				EM_DEBUG_FUNC_END("false");
				if (err_code != NULL)
					*err_code = err;
				return false;
			}

			if (mmsg->header && !emcore_mime_parse_part(stream, is_file, mmsg->header->part_header, &mmsg->nested, cnt_info, &end_of_parsing, &err)) {
				EM_DEBUG_FUNC_END("false");
				if (err_code != NULL)
					*err_code = err;
				return false;
			}

			/*  after finishing body parsing, make stream empty to get next mail. (get line from sock or file until '.' is read */
			if (end_of_parsing == true && err != EMAIL_ERROR_NO_MORE_DATA)
				emcore_mime_flush_receiving_buffer(stream, is_file, NULL, NULL, NULL, err_code);

			break;

		default:
			attachment_name = NULL;

			if (mmsg->header && emcore_mime_get_header_value(mmsg->header->part_header, CONTENT_DISPOSITION, err_code)) {
				attachment_name = emcore_mime_get_header_value(mmsg->header->part_header, CONTENT_FILENAME, err_code);
				/* if (!attachment_name) attachment_name = "unknown" */
				if (attachment_name) EM_DEBUG_LOG(" attachment = [%s]", attachment_name);
			}

			if (strcasestr(content_type, "PKCS7-MIME")) {
				EM_DEBUG_LOG("Encrypted mail do not include the body");

				if (attachment_name == NULL)
					attachment_name = emcore_mime_get_header_value(mmsg->header->part_header, CONTENT_NAME, err_code);

				EM_DEBUG_LOG("attachment_name : [%s]", attachment_name);

				cnt_info->file = em_malloc(sizeof(struct attachment_info));
				if (cnt_info->file) {
					cnt_info->file->type = ATTACHMENT;
					cnt_info->file->name = EM_SAFE_STRDUP(attachment_name);
					if (!emcore_mime_get_content_data(stream, is_file, false, NULL, content_encoding, &end_of_parsing, SAVE_TYPE_FILE, &holder, &size, NULL, err_code)) {
						EM_DEBUG_EXCEPTION("emcore_mime_get_content_data failed : [%d]", err_code);
						return false;
					}
					cnt_info->file->save = holder;
					cnt_info->file->size = size;
					cnt_info->file->next = NULL;
				}
				break;
			}

			if (cnt_info->grab_type & GRAB_TYPE_TEXT)  {
				/* EM_DEBUG_LINE */
				/*  get content data. content data is saved in file */
				if (!emcore_mime_get_content_data(stream, is_file, true, NULL, content_encoding, &end_of_parsing, SAVE_TYPE_FILE, &holder, &size, NULL, err_code)) {
					EM_DEBUG_EXCEPTION("emcore_mime_get_content_data failed : [%d]", err_code);
					return false;
				}

				EM_DEBUG_LOG("After emcore_mime_get_content_data");

				char *charset = mmsg->header? emcore_mime_get_header_value(mmsg->header->part_header, CONTENT_CHARSET, err_code) : NULL; /* prevent 27453 */
				EM_DEBUG_LOG(">>>> charset [%s]", charset);

				if (mmsg->header && mmsg->header->part_header && strstr((t = emcore_mime_get_header_value(mmsg->header->part_header, CONTENT_TYPE, err_code)) ? t  :  "", "HTML")) {
					if (!charset || !strncmp(charset, "X-UNKNOWN", strlen("X-UNKNOWN")))
						cnt_info->text.html_charset = strdup("UTF-8");
					else
						cnt_info->text.html_charset = EM_SAFE_STRDUP(charset);
					EM_DEBUG_LOG(">>>> cnt_info->text.html_charset [%s]", cnt_info->text.html_charset);

					cnt_info->text.html = holder;
					EM_DEBUG_LOG(">>>> cnt_info->text.html [%s]", cnt_info->text.html);
				}
				else if (mmsg->header) {

					if (!charset || !strncmp(charset, "X-UNKNOWN", strlen("X-UNKNOWN")))
						cnt_info->text.plain_charset = strdup("UTF-8");
					else
						cnt_info->text.plain_charset = EM_SAFE_STRDUP(charset);
					EM_DEBUG_LOG(">>>> cnt_info->text.plain_charset [%s]", cnt_info->text.plain_charset);

					cnt_info->text.plain = holder;
					EM_DEBUG_LOG(">>>> cnt_info->text.plain [%s]", cnt_info->text.plain);
				}
			}

			break;
	}
	EM_DEBUG_FUNC_END();
	return true;
}

int emcore_mime_parse_part(void *stream, int is_file, struct _m_part_header *parent_header, struct _m_part *nested, struct _m_content_info *cnt_info, int *eop, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], is_file[%d], parent_header[%p], nested[%p], cnt_info[%p], eop[%p], err_code[%p]", stream, is_file, parent_header, nested, cnt_info, eop, err_code);

	struct _m_body *tmp_body = NULL;
	struct _m_part **p = NULL;
	char buf[MIME_LINE_LEN] = {0x00, };
	char boundary[BOUNDARY_LEN] = {0x00, };
	char boundary_end[BOUNDARY_LEN] = {0x00, };
	char mime_type_buffer[128] = { 0, };
	char *boundary_str = NULL;
	char *content_type = NULL;
	char *content_encoding = NULL;
	char *holder = NULL;
	char *attachment_name = NULL;
	char *content_disposition = NULL;
	char *pTemp = NULL;
	int type = 0;
	int content_disposition_type = 0;
	int end_of_parsing = 0;
	int size = 0, local_err_code = EMAIL_ERROR_NONE;
	int is_skip = false;
	int result = 0;

	boundary_str = emcore_mime_get_header_value(parent_header, CONTENT_BOUNDARY, err_code);

	SNPRINTF(boundary, BOUNDARY_LEN, "--%s%s", boundary_str, CRLF_STRING);
	SNPRINTF(boundary_end, BOUNDARY_LEN, "--%s%s", boundary_str, "--\r\n");

	nested->body = NULL;
	nested->next = NULL;

	/*  goto the first found useful mime dat */
	EM_DEBUG_LOG("Before first loop");
	while (true)  {
		if (!emcore_check_thread_status())  {
			if (err_code != NULL)
				*err_code = EMAIL_ERROR_CANCELLED;
			return false;
		}
		if ((is_file == 0 && !emcore_mime_get_line_from_sock(stream, buf, MIME_LINE_LEN, err_code)) ||
		(is_file == 1 && !emcore_get_line_from_file(stream, buf, MIME_LINE_LEN, err_code))) {
			EM_DEBUG_EXCEPTION("emcore_mime_get_line_from_sock failed.");
			if (eop)
				*eop = true;
			EM_DEBUG_FUNC_END("false");
			return false;
		}

		if (!strcmp(buf, boundary))
			break;
	}

	EM_DEBUG_LOG("Before second loop");
	while (true)  {
		if (!(tmp_body = em_malloc(sizeof(struct _m_body))))  {
			EM_DEBUG_EXCEPTION("em_malloc failed.");
			if (nested->body)
				emcore_mime_free_part_body(nested->body);
			if (nested->next)
				emcore_mime_free_part(nested->next);
			EM_DEBUG_FUNC_END("false");
			return false;
		}

		memset(tmp_body, 0, sizeof(struct _m_body));

		/*  parsing MIME Header */
		if (!emcore_mime_parse_part_header(stream, is_file, &tmp_body->part_header, err_code))  {
			EM_DEBUG_EXCEPTION("emcore_mime_parse_part_header failed.");
			if (nested->body)
				emcore_mime_free_part_body(nested->body);
			if (nested->next)
				emcore_mime_free_part(nested->next);

			emcore_mime_free_part_body(tmp_body);
			return false;
		}

		content_type = emcore_mime_get_header_value(tmp_body->part_header, CONTENT_TYPE, err_code);

		if (!content_type)
			content_type = "TEXT/PLAIN";

		content_encoding = emcore_mime_get_header_value(tmp_body->part_header, CONTENT_ENCODING, err_code);
		if (!content_encoding)
			content_encoding = "7BIT";

		if (strstr(content_type, TEXT_STR)) type = TYPE_TEXT;
		else if (strstr(content_type, IMAGE_STR)) type = TYPE_IMAGE;
		else if (strstr(content_type, AUDIO_STR)) type = TYPE_AUDIO;
		else if (strstr(content_type, VIDEO_STR)) type = TYPE_VIDEO;
		else if (strstr(content_type, APPLICATION_STR)) type = TYPE_APPLICATION;
		else if (strstr(content_type, MULTIPART_STR)) type = TYPE_MULTIPART;
		else if (strstr(content_type, MESSAGE_STR)) type = TYPE_MESSAGE;
		else type = TYPE_UNKNOWN;

		switch (type)  {
			case TYPE_MULTIPART:
				EM_DEBUG_LOG("TYPE_MULTIPART");
				if (!emcore_mime_get_header_value(tmp_body->part_header, CONTENT_BOUNDARY, err_code))  {
					EM_DEBUG_EXCEPTION("emcore_mime_get_header_value failed.");
					emcore_mime_free_part_body(tmp_body);
					EM_DEBUG_FUNC_END("false");
					return false;
				}

				emcore_mime_parse_part(stream, is_file, tmp_body->part_header, &tmp_body->nested, cnt_info, &end_of_parsing, &local_err_code);

				if (!nested->body)
					nested->body = tmp_body;
				else  {
					p = &nested->next;

					while (*p && (*p)->next)
						*p = (*p)->next;

					if (*p)
						p = &(*p)->next;

					if (!(*p = em_malloc(sizeof(struct _m_part))))  {
						EM_DEBUG_EXCEPTION("em_malloc failed");
						if (nested->body) emcore_mime_free_part_body(nested->body);
						if (nested->next) emcore_mime_free_part(nested->next);
						emcore_mime_free_part_body(tmp_body);
						EM_DEBUG_FUNC_END("false");
						return false;
					}

					(*p)->body = tmp_body;
					(*p)->next = NULL;
				}

				if (err_code)
					*err_code = local_err_code;

				if (end_of_parsing && local_err_code != EMAIL_ERROR_NO_MORE_DATA) /*  working with imap */
				/* if (!end_of_parsing) */ /*  working with pop */ {
					EM_DEBUG_LOG("Enter flushing socket buffer.");
					emcore_mime_flush_receiving_buffer(stream, is_file, boundary, boundary_end, &end_of_parsing, err_code);
				}

				break;

			default:
				EM_DEBUG_LOG("default");
				attachment_name = NULL;
				content_disposition = NULL;

				if (type == TYPE_MESSAGE)
					is_skip = true;

				if (is_skip == true)  {
					if (!emcore_mime_skip_content_data(stream, is_file, boundary_str, &end_of_parsing, &size, NULL, err_code))
						EM_DEBUG_EXCEPTION("emcore_mime_skip_content_data failed...");

					emcore_mime_free_part_body(tmp_body);
					EM_DEBUG_LOG_MIME("break");
					break;
				}

				/*  first check inline content */
				/*  if the content id or content location exis */
				content_disposition = emcore_mime_get_header_value(tmp_body->part_header, CONTENT_DISPOSITION, err_code);
				EM_DEBUG_LOG("content_disposition : [%s]", content_disposition);

				attachment_name = emcore_mime_get_header_value(tmp_body->part_header, CONTENT_ID, err_code);
				EM_DEBUG_LOG("content_id : [%s]", attachment_name);

				if (attachment_name) {
					if (emcore_search_string_from_file(cnt_info->text.html, attachment_name, &result) == EMAIL_ERROR_NONE && result) {
						content_disposition_type = INLINE_ATTACHMENT;
					} else if (!strcasecmp(content_disposition ? content_disposition : "", "attachment")) {
						attachment_name = emcore_mime_get_header_value(tmp_body->part_header, CONTENT_NAME, err_code);

						if (!attachment_name)
							attachment_name = emcore_mime_get_header_value(tmp_body->part_header, CONTENT_FILENAME, err_code);

						content_disposition_type = ATTACHMENT;

					} else {
						EM_DEBUG_EXCEPTION("Unknown mime type");
					}
				} else {
					if (!strcasecmp(content_disposition ? content_disposition : "", "attachment")) {
						attachment_name = emcore_mime_get_header_value(tmp_body->part_header, CONTENT_NAME, err_code);

						if (!attachment_name)
							attachment_name = emcore_mime_get_header_value(tmp_body->part_header, CONTENT_FILENAME, err_code);

						content_disposition_type = ATTACHMENT;

					} else if (!strcasecmp(content_disposition ? content_disposition : "", "inline")) {
						attachment_name = emcore_mime_get_header_value(tmp_body->part_header, CONTENT_NAME, err_code);

						if (!attachment_name)
							attachment_name = emcore_mime_get_header_value(tmp_body->part_header, CONTENT_FILENAME, err_code);

						content_disposition_type = INLINE_ATTACHMENT;
					} else if (strstr(content_type ? content_type : "", "PKCS7")) {
						attachment_name = emcore_mime_get_header_value(tmp_body->part_header, CONTENT_NAME, err_code);
						EM_DEBUG_LOG_MIME(">> attachment = [%s]", attachment_name ? attachment_name  :  NIL);

						content_disposition_type = ATTACHMENT;

					} else {
						EM_DEBUG_LOG("No attachment");
					}
				}

				if (!emcore_check_thread_status())  {
					if (err_code != NULL)
						*err_code = EMAIL_ERROR_CANCELLED;
					EM_DEBUG_EXCEPTION("EMAIL_ERROR_CANCELLED");
					emcore_mime_free_part_body(tmp_body);
					EM_DEBUG_FUNC_END("false");
					return false;
				}

				EM_DEBUG_LOG("attachment_name : [%s]", attachment_name);
				/*  get content and content information */
				if (content_disposition_type != ATTACHMENT && content_disposition_type != INLINE_ATTACHMENT)  {	/*  text */
					/*  get content by buffer */
					EM_DEBUG_LOG_MIME("attachment_name is NULL. It's a text message");
					if (!emcore_mime_get_content_data(stream, is_file, true, boundary_str, content_encoding, &end_of_parsing, SAVE_TYPE_FILE, &holder, &size, NULL, err_code)) {
						EM_DEBUG_EXCEPTION("emcore_mime_get_content_data failed [%d]", err_code);
						emcore_mime_free_part_body(tmp_body);
						return false;
					}

					EM_DEBUG_LOG("After emcore_mime_get_content_data");

					if (cnt_info->grab_type & GRAB_TYPE_TEXT) {
						char *charset = emcore_mime_get_header_value(tmp_body->part_header, CONTENT_CHARSET, err_code);
						EM_DEBUG_LOG(" charset [%s]", charset);

						if (tmp_body->part_header && strstr(content_type ? content_type  :  "", "HTML"))  {
							if (!charset || !strncmp(charset, "X-UNKNOWN", strlen("X-UNKNOWN")))
								cnt_info->text.html_charset = strdup("UTF-8");
							else
								cnt_info->text.html_charset = EM_SAFE_STRDUP(charset);
							EM_DEBUG_LOG(" cnt_info->text.html_charset [%s]", cnt_info->text.html_charset);

							cnt_info->text.html = holder;
							EM_DEBUG_LOG(" cnt_info->text.html [%s]", cnt_info->text.html);
						} else {
							if (!charset || !strncmp(charset, "X-UNKNOWN", strlen("X-UNKNOWN")))
								cnt_info->text.plain_charset = strdup("UTF-8");
							else
								cnt_info->text.plain_charset = EM_SAFE_STRDUP(charset);
							EM_DEBUG_LOG(" cnt_info->text.plain_charset [%s]", cnt_info->text.plain_charset);

							cnt_info->text.plain = holder;
							EM_DEBUG_LOG(" cnt_info->text.plain [%s]", cnt_info->text.plain);
						}
					} else {
						if (holder) {
							free(holder);
							holder = NULL;
						}
					}
				} else {		/*  attachment */
					EM_DEBUG_LOG("attachment_name is not NULL. It's a attachment");
					struct attachment_info *file = NULL;
					struct attachment_info *temp_file = cnt_info->file;

					file = em_malloc(sizeof(struct attachment_info));
					if (file == NULL) {
						EM_DEBUG_EXCEPTION("em_malloc failed...");
						emcore_mime_free_part_body(tmp_body);
						EM_DEBUG_FUNC_END("false");
						return false;
					}

					file->type = content_disposition_type;

					EM_DEBUG_LOG("file->type : %d", file->type);

					file->name                 = EM_SAFE_STRDUP(attachment_name);
					file->content_id           = EM_SAFE_STRDUP(tmp_body->part_header->content_id);
					if(tmp_body->part_header->type && tmp_body->part_header->subtype) {
						SNPRINTF(mime_type_buffer, 128, "%s/%s", tmp_body->part_header->type, tmp_body->part_header->subtype);
						file->attachment_mime_type = EM_SAFE_STRDUP(mime_type_buffer);
					}

					/*  check if the current file is target file */
					if ((cnt_info->grab_type & GRAB_TYPE_ATTACHMENT) || file->type == INLINE_ATTACHMENT)  {
						/*  get content by file */
						EM_DEBUG_LOG_MIME("Trying to get content");
						if (!emcore_mime_get_content_data(stream, is_file, false, boundary_str, content_encoding, &end_of_parsing, SAVE_TYPE_FILE, &holder, &size, NULL, err_code)) {
							EM_DEBUG_EXCEPTION("emcore_mime_get_content_data failed [%d]", err_code);
							emcore_mime_free_part_body(tmp_body);
							emcore_free_attachment_info(file);
							EM_DEBUG_FUNC_END("false");
							return false;
						}

						file->save = holder;
					} else {
						/*  only get content size */
						EM_DEBUG_LOG_MIME("Pass downloading");
						if (!emcore_mime_get_content_data(stream, is_file, false, boundary_str, content_encoding, &end_of_parsing, SAVE_TYPE_SIZE, NULL, &size, NULL, err_code)) {
							EM_DEBUG_EXCEPTION("emcore_mime_get_content_data failed [%d]", err_code);
							emcore_mime_free_part_body(tmp_body);
							emcore_free_attachment_info(file);
							EM_DEBUG_FUNC_END("false");
							return false;
						}

						file->save = NULL;
					}

					if (err_code)
						EM_DEBUG_LOG("end_of_parsing [%d], err_code [%d]", end_of_parsing, *err_code);

					file->size = size;

					if (strstr(content_type, APPLICATION_STR))  {
						pTemp = content_type + EM_SAFE_STRLEN(APPLICATION_STR);

						if (strcasecmp(pTemp, MIME_SUBTYPE_DRM_OBJECT) == 0)
							file->drm = EMAIL_ATTACHMENT_DRM_OBJECT;
						else if (strcasecmp(pTemp, MIME_SUBTYPE_DRM_RIGHTS) == 0)
							file->drm = EMAIL_ATTACHMENT_DRM_RIGHTS;
						else if (strcasecmp(pTemp, MIME_SUBTYPE_DRM_DCF) == 0)
							file->drm = EMAIL_ATTACHMENT_DRM_DCF;
					}

					while (temp_file && temp_file->next)
						temp_file = temp_file->next;

					if (temp_file == NULL)
						cnt_info->file = file;
					else
						temp_file->next = file;
				}

				if (!emcore_check_thread_status())  {
					if (err_code != NULL)
						*err_code = EMAIL_ERROR_CANCELLED;
					EM_DEBUG_EXCEPTION("EMAIL_ERROR_CANCELLED");
					emcore_mime_free_part_body(tmp_body);
					EM_DEBUG_FUNC_END("false");
					return false;
				}

				if (!nested->body)
					nested->body = tmp_body;
				else {
					p = &nested->next;

					while (*p && (*p)->next)
						p = &(*p)->next;

					if (*p)
						p = &(*p)->next;

					if (!(*p = em_malloc(sizeof(struct _m_part))))  {
						EM_DEBUG_EXCEPTION("em_malloc failed");
						if (nested->body) emcore_mime_free_part_body(nested->body);
						if (nested->next) emcore_mime_free_part(nested->next);

						emcore_mime_free_part_body(tmp_body);
						EM_DEBUG_FUNC_END("false");
						return false;
					}

					(*p)->body = tmp_body;
					(*p)->next = NULL;
				}

				break;
		}

		/*  End of parsin */
		if (end_of_parsing)
			break;
	}

	if (eop != NULL)
		*eop = end_of_parsing;

	EM_DEBUG_FUNC_END("end_of_parsing [%d]", end_of_parsing);
	return true;
}

/*  set RFC822 Heade */
int emcore_mime_set_rfc822_header_value(struct _rfc822header **header, char *name, char *value, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("header[%p], name[%s], value[%s], err_code[%p]", header, name, value, err_code);

	char **p = NULL;
	char *t = NULL;

	if (!value || !*value || !name ) return false; /*prevent 34354*/

	if (!*header)  {
		*header = em_malloc(sizeof(struct _rfc822header));
		if (!*header)  {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			return false;
		}
	}


	em_upper_string(name);

	if (strncmp(name, "RETURN-PATH", strlen("RETURN-PATH")) == 0)
		p = &(*header)->return_path;/*  Return-Rat */
	else if (strncmp(name, "RECEIVED", strlen("RECEIVED")) == 0)
		p = &(*header)->received;	/*  Receive */
	else if (strncmp(name, "REPLY-TO", strlen("REPLY-TO")) == 0)
		p = &(*header)->reply_to;	/*  Reply-T */
	else if (strncmp(name, "DATE", strlen("DATE")) == 0)
		p = &(*header)->date;		/*  Dat */
	else if (strncmp(name, "FROM", strlen("FROM")) == 0)
		p = &(*header)->from;		/*  Fro */
	else if (strncmp(name, "SUBJECT", strlen("SUBJECT")) == 0)
		p = &(*header)->subject;	/*  Subjec */
	else if (strncmp(name, "SENDER", strlen("SENDER")) == 0)
		p = &(*header)->sender;		/*  Sende */
	else if (strncmp(name, "TO", strlen("TO")) == 0)
		p = &(*header)->to;			/*  T */
	else if (strncmp(name, "CC", strlen("CC")) == 0)
		p = &(*header)->cc;			/*  C */
	else if (strncmp(name, "BCC", strlen("BCC")) == 0)
		p = &(*header)->bcc;		/*  Bc */
	else if (strncmp(name, "X-PRIORITY", strlen("X-PRIORITY")) == 0)
		p = &(*header)->priority;	/*  Prorit */
	else if (strncmp(name, "X-MSMAIL-PRIORITY", strlen("X-MSMAIL-PRIORITY")) == 0)
		p = &(*header)->ms_priority;/*  Prorit */
	else if (strncmp(name, "DISPOSITION-NOTIFICATION-TO", strlen("DISPOSITION-NOTIFICATION-TO")) == 0)
		p = &(*header)->dsp_noti_to;/*  Disposition-Notification-T */
	else {
		return false;
	}

	if (!*p)
		*p = EM_SAFE_STRDUP(value);
	else  { /*  Long Header */
		if (!(t = realloc(*p, strlen(*p) + strlen(value)+1))) /*prevent 34354*/
			return false;

		strncat(t, value, strlen(value)); /*prevent 34354*/
		*p = t;
	}

	return true;
}

/*  set MIME Part Heade */
int emcore_mime_set_part_header_value(struct _m_part_header **header, char *name, char *value, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("header[%p], name[%s], value[%s], err_code[%p]", header, name, value, err_code);

	if (!name || !value) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	struct _parameter *p = NULL;
	char *p_val = NULL;

	if (!*header)  {
		*header = em_malloc(sizeof(struct _m_part_header));
		if (!(*header))	{
			EM_DEBUG_EXCEPTION("em_malloc failed...");
			return false;
		}
	}

	em_upper_string(name);
	em_trim_left(value);
	em_trim_right(value);

	if (!emcore_check_thread_status())  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_CANCELLED;
		return false;
	}

	if (name) {
		if (strncmp(name, "CONTENT-TYPE", strlen("CONTENT-TYPE")) == 0)  {
			p_val = strtok(value, ";");

			if (p_val)  {
				if (!(*header)->type)  {   /*  Content-Type */
					em_upper_string(p_val);
					(*header)->type = EM_SAFE_STRDUP(p_val);
				}
				else  {   /*  Content-Type Parameter (format :  "name =value" */
					if (emcore_mime_get_param_from_str(p_val, &p, err_code))
						emcore_mime_add_param_to_list(&((*header)->parameter), p, err_code);
					else /* name= CRLF value */ {
						struct _parameter *t = (*header)->parameter;
						while (t && t->next) {
							t = t->next;
						}

						EM_DEBUG_LOG("name : [%s]", t->name);
						EM_DEBUG_LOG("value : [%s]", t->value);

						if (t->value == NULL) {
							char *pointer = NULL;

							if (EM_SAFE_STRLEN(p_val) > 0)  {
								if ((pointer = strchr(p_val, '\"')))  {
									p_val = pointer + 1;
									if (!*p_val) return false;
								}
								if ((pointer = strchr(p_val, '\"')))
									*pointer = '\0';

								/*  = ? ENCODING_TYPE ? B(Q) ? ENCODED_STRING ? */
								int err = EMAIL_ERROR_NONE;
								char *utf8_text = NULL;

								if (!(utf8_text = emcore_decode_rfc2047_text(p_val, &err)))
									EM_DEBUG_EXCEPTION("emcore_decode_rfc2047_text failed [%d]", err);
								EM_DEBUG_LOG("utf8_text : [%s]", utf8_text);
								t->value = EM_SAFE_STRDUP(utf8_text);
							}
						}
					}

				}

				/*  repeatedly get paramete */
				while ((p_val = strtok(NULL, ";")))  {
					if (emcore_mime_get_param_from_str(p_val, &p, err_code))
						emcore_mime_add_param_to_list(&((*header)->parameter), p, err_code);
				}
			}
		}
		else if (strncmp(name, "CONTENT-TRANSFER-ENCODING", strlen("CONTENT-TRANSFER-ENCODING")) == 0)  {
			em_upper_string(value);
			(*header)->encoding = EM_SAFE_STRDUP(value);
		}
		else if (strncmp(name, "CONTENT-DESCRPTION", strlen("CONTENT-DESCRPTION")) == 0)  {
			em_upper_string(value);
			(*header)->desc = EM_SAFE_STRDUP(value);
		}
		else if (strncmp(name, "CONTENT-DISPOSITION", strlen("CONTENT-DISPOSITION")) == 0)  {
			p_val = strtok(value, ";");

			if (p_val)  {
				if (!(*header)->disp_type)  {	/*  Content-Dispositio */
					em_upper_string(p_val);
					(*header)->disp_type = EM_SAFE_STRDUP(p_val);
				}
				else  {	/*  Content-Disposition parameter (format :  "name =value" */
					if (emcore_mime_get_param_from_str(p_val, &p, err_code))
						emcore_mime_add_param_to_list(&((*header)->disp_parameter), p, err_code);
				}

				/*  repeatedly get paramete */
				while ((p_val = strtok(NULL, ";")))  {
					if (emcore_mime_get_param_from_str(p_val, &p, err_code))
						emcore_mime_add_param_to_list(&((*header)->disp_parameter), p, err_code);
				}
			}
		}
		else if (strncmp(name, "CONTENT-ID", strlen("CONTENT-ID")) == 0)  {
			size_t len = 0;
			len = EM_SAFE_STRLEN(value);
			/* em_upper_string(value) */

			if ((len) && (value[0] == '<'))  {
				++value;
				--len;
			}

			if ((len > 1) && (value[len-1] == '>'))
				value[len-1] = '\0';

			(*header)->content_id = EM_SAFE_STRDUP(value);
		}
		else if (strncmp(name, "CONTENT-LOCATION", strlen("CONTENT-LOCATION")) == 0)
			(*header)->content_location = EM_SAFE_STRDUP(value);
	}
	EM_DEBUG_FUNC_END();
	return true;
}

/*  get header parameter from string */
int emcore_mime_get_param_from_str(char *str, struct _parameter **param, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("str[%s], param[%p], err_code[%p]", str, param, err_code);

	char *p_name, *p_val, *p;

	*param = NULL;

	/*  Parameter Check */
	if (!(p = strchr(str, '=')))	return false;

	*p = '\0';

	p_name = str;
	p_val = p+1;

	em_trim_left(p_name);
	em_trim_right(p_name);

	if (!*p_name) return false;

	if (!(*param = em_malloc(sizeof(struct _parameter)))) return false;

	(*param)->next = NULL;

	/*  Name set */
	/*  Check string length */
	if (EM_SAFE_STRLEN(p_name) > 0)  {
		em_upper_string(p_name);
		(*param)->name = EM_SAFE_STRDUP(p_name);
	}

	em_trim_left(p_val);
	em_trim_right(p_val);

	if (!*p_val)  {
		EM_DEBUG_LOG("Parameter value is NULL");
		return true;
	}

	if (EM_SAFE_STRLEN(p_val) > 0)  {
		if ((p = strchr(p_val, '\"')))  {
			p_val = p + 1;
			if (!*p_val) return false;
		}
		if ((p = strchr(p_val, '\"')))
			*p = '\0';

		if (strncmp(p_name, "BOUNDARY", strlen("BOUNDARY")) != 0 && !strstr(p_name, "NAME"))
			em_upper_string(p_val);

		/*  = ? ENCODING_TYPE ? B(Q) ? ENCODED_STRING ? */
		int err = EMAIL_ERROR_NONE;
		char *utf8_text = NULL;

		if (!(utf8_text = emcore_decode_rfc2047_text(p_val, &err)))
			EM_DEBUG_EXCEPTION("emcore_decode_rfc2047_text failed [%d]", err);
		(*param)->value = utf8_text;
	}
	EM_DEBUG_FUNC_END();
	return true;
}

/*  add a parameter to parameter lis */
int emcore_mime_add_param_to_list(struct _parameter **param_list, struct _parameter *param, int *err_code)
{
	struct _parameter **t = param_list;

	while (*t && (*t)->next)
		*t = (*t)->next;

	if (*t)
		(*t)->next = param;
	else
		*t = param;

	return true;
}

/*  get header value from MIME Part Heade */
char *emcore_mime_get_header_value(struct _m_part_header *header, int type, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("header[%p], type[%d], err_code[%p]", header, type, err_code);

	struct _parameter *p = NULL;
	char *name = NULL;

	if (!header)  {
		EM_DEBUG_EXCEPTION("header[%p], type[%d]", header, type);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return NULL;
	}

	switch (type)  {
		case CONTENT_TYPE:
			return header->type;

		case CONTENT_SUBTYPE:
			return header->subtype;

		case CONTENT_ENCODING:
			return header->encoding;

		case CONTENT_CHARSET:
			name = "CHARSET";
			p = header->parameter;
			break;

		case CONTENT_DISPOSITION:
			return header->disp_type;

		case CONTENT_NAME:
			name = "NAME";
			p = header->parameter;
			break;

		case CONTENT_FILENAME:
			name = "FILENAME";
			p = header->disp_parameter;
			break;

		case CONTENT_BOUNDARY:
			name = "BOUNDARY";
			p = header->parameter;
			break;

		case CONTENT_REPORT_TYPE:
			name = "REPORT-TYPE";
			p = header->parameter;
			break;

		case CONTENT_ID:
			return header->content_id;

		case CONTENT_LOCATION:
			return header->content_location;

		default:
			return NULL;
	}

	for (; p; p = p->next)  {
		if (strcmp(p->name, name) == 0)
			break;
	}

	if (!p)
		return NULL;
	EM_DEBUG_FUNC_END();
	return p->value;
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
			EM_DEBUG_LOG("ENCQUOTEDPRINTABLE");
			content = rfc822_qprint((unsigned char *)enc_buf, (unsigned long)enc_len, (unsigned long *)dec_len);
			break;

		case ENCBASE64:
			EM_DEBUG_LOG("ENCBASE64");
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
			return -1;
		}
		memcpy(enc_buf, content, *dec_len);
		enc_buf[*dec_len] = '\0';
		EM_SAFE_FREE(content);
	}
	EM_DEBUG_FUNC_END();
	return 0;
}

/*  1. if boundary is NULL, contnent has not multipart */
/*  2. if boundary isn't NULL, content is from current line to the next found boundary */
/*     if next found boundary is the other part boundary ("--boundary"), return and set end_of_parsing to 1 */
/*     if next found boundary is the multipart ending boundary ("--boundary--"), return and set end_of_parsing to 0 */
/*  mode - SAVE_TYPE_SIZE   :  no saving (only hold content size */
/*         SAVE_TYPE_BUFFER :  content is saved to buffer (holder is buffer */
/*         SAVE_TYPE_FILE   :  content is saved to temporary file (holder is file name */
int emcore_mime_get_content_data(void *stream, int is_file, int is_text, char *boundary_str, char *content_encoding, int *end_of_parsing, int mode, char **holder, int *size, void *callback, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], is_file[%d], boundary_str[%s], content_encoding[%s], end_of_parsing[%p], mode[%d], holder[%p], size[%p], callback[%p], err_code[%p]", stream, is_file, boundary_str, content_encoding, end_of_parsing, mode, holder, size, callback, err_code);

	int encoding = ENC7BIT;
	int error = EMAIL_ERROR_NONE, ret = false;
	int fd = 0;
	int sz = 0;

	if ((mode == SAVE_TYPE_FILE || mode == SAVE_TYPE_BUFFER) && !holder)
		return false;

	if (holder)
		*holder = NULL;

	if (size)
		*size = 0;

	EM_DEBUG_LOG("get content");

	if (content_encoding)  {
		switch (content_encoding[0])  {
			case 'Q':
				encoding = ENCQUOTEDPRINTABLE;
				break;	/*  qutoed-printabl */
			case 'B':
				if (content_encoding[1] == 'A')  {
					encoding = ENCBASE64;
					break; /*  base6 */
				}
				if (content_encoding[1] == 'I')  {
					encoding = ENCBINARY;
					break; /*  binar */
				}
				encoding = ENC7BIT;
				break;
			case '7':
				encoding = ENC7BIT;
				break; /*  7bi */
			case '8':
				encoding = ENC8BIT;
				break; /*  8bi */
			default:
				encoding = ENCOTHER;
				break; /*  unknow */
		}
	}

	/*  saving type is file */
	if (mode == SAVE_TYPE_FILE)  {
		*holder = emcore_mime_get_save_file_name(&error);

		EM_DEBUG_LOG("holder[%s]", *holder);

		fd = open(*holder, O_WRONLY|O_CREAT, 0644);
		if (fd < 0)  {
			EM_DEBUG_EXCEPTION("holder open failed :  holder is a filename that will be saved.");
			goto FINISH_OFF;
		}
	}

	if (is_file) {
		EM_DEBUG_LOG("from file");
		error = emcore_decode_body_text_from_file((FILE *)stream, boundary_str, encoding, mode, is_text, fd, holder, end_of_parsing, &sz);
		if (error != EMAIL_ERROR_NONE) {
			if (error != EMAIL_ERROR_NO_MORE_DATA) {
				EM_DEBUG_EXCEPTION("emcore_decode_body_text_from_file failed : [%d]", error);
				goto FINISH_OFF;
			}
		}
	} else {
		EM_DEBUG_LOG("from sock");
		error = emcore_decode_body_text_from_sock(stream, boundary_str, encoding, mode, is_text, fd, holder, end_of_parsing, &sz);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_decode_body_text_from_sock failed : [%d]", error);
			goto FINISH_OFF;
		}
	}

	ret = true;
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

	if(fd>0) close(fd); /*prevent 32736*/

	if (ret) {
		if (size)
			*size = sz;
	}

	EM_DEBUG_FUNC_END("ret [%d], sz [%d]", ret, sz);
	return ret;
}

int emcore_mime_skip_content_data(void *stream,
							int is_file,
							char *boundary_str,
							int *end_of_parsing,
							int *size,
							void *callback,
							int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], is_file[%d], boundary_str[%s], end_of_parsing[%p], size[%p], callback[%p], err_code[%p]", stream, is_file, boundary_str, end_of_parsing, size, callback, err_code);

	char buf[MIME_LINE_LEN] = {0x00}, boundary[BOUNDARY_LEN], boundary_end[BOUNDARY_LEN];
	int sz = 0;

	if (size)
		*size = 0;

	EM_DEBUG_LOG(">>> skip content <<<<<<<<<<<<<");

	if (!boundary_str)  {	/*  if no boundary, this content is from current line to end of all multipart */
		while (TRUE)  {

			if (!emcore_check_thread_status())  {
				if (err_code != NULL)
					*err_code = EMAIL_ERROR_CANCELLED;
				return false;
			}
			if ((is_file == 0 && !emcore_mime_get_line_from_sock(stream, buf, MIME_LINE_LEN, err_code)) ||
			(is_file == 1 && !emcore_get_line_from_file(stream, buf, MIME_LINE_LEN, err_code)))  {
				*end_of_parsing = 1;
				if (size)
					*size = sz;
				return false;
			}
			sz += EM_SAFE_STRLEN(buf);
		}
	}
	else  {		/*  if there boundary, this content is from current line to ending boundary */
		memset(boundary, 0x00, BOUNDARY_LEN);
		memset(boundary_end, 0x00, BOUNDARY_LEN);

		SNPRINTF(boundary,  BOUNDARY_LEN, "--%s%s", boundary_str, CRLF_STRING);
		SNPRINTF(boundary_end, BOUNDARY_LEN, "--%s%s", boundary_str, "--\r\n");

		while (TRUE)  {

			if (!emcore_check_thread_status())  {
				if (err_code != NULL)
					*err_code = EMAIL_ERROR_CANCELLED;
				return false;
			}
			if ((is_file == 0 && !emcore_mime_get_line_from_sock(stream, buf, MIME_LINE_LEN, err_code)) ||
			(is_file == 1 && !emcore_get_line_from_file(stream, buf, MIME_LINE_LEN, err_code)))  {
				/*  end of fil */
				*end_of_parsing = 1;
				if (size)
					*size = sz;
				return true;
			}

			if (!strcmp(buf, boundary))  {		/*  the other part started. the parsing of other part will be started */
				*end_of_parsing = 0;
				if (size)
					*size = sz;
				return true;
			}
			else if (!strcmp(buf, boundary_end))  {		/*  if ending boundary, the parsing of other multipart will be started */
				*end_of_parsing = 1;
				if (size)
					*size = sz;
				return true;
			}

			sz += EM_SAFE_STRLEN(buf);
		}
	}

	if (size)
		*size = sz;
	EM_DEBUG_FUNC_END();
	return true;
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

	if (!pop3_reply((MAILSTREAM *)stream))  { /*  if TRUE, check respons */
		EM_DEBUG_LOG("p_pop3local->response 1[%s]", p_pop3local->response);
		if (p_pop3local->response) {
			if (*p_pop3local->response == '.' && EM_SAFE_STRLEN(p_pop3local->response) == 1)  {
				free(p_pop3local->response);
				p_pop3local->response = NULL;
				if (err_code != NULL)
					*err_code = EMAIL_ERROR_NO_MORE_DATA;
				EM_DEBUG_FUNC_END("end of response");
				return NULL;
			}
			EM_DEBUG_LOG("Not end of response");
			strncpy(buf, p_pop3local->response, size-1);
			strncat(buf, CRLF_STRING, size-(EM_SAFE_STRLEN(buf) + 1));

			free(p_pop3local->response);
			p_pop3local->response = NULL;

			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG("p_pop3local->response 2[%s]", p_pop3local->response);
	if (p_pop3local->response)
       {
		/*  if response isn't NULL, check whether this response start with '+' */
		/*  if the first character is '+', return error because this response is normal data */
		strncpy(buf, p_pop3local->response, size-1);
		strncat(buf, CRLF_STRING, size-(EM_SAFE_STRLEN(buf)+1));
		free(p_pop3local->response); p_pop3local->response = NULL;
		goto FINISH_OFF;
       }
	else  {
		EM_DEBUG_EXCEPTION("p_pop3local->response is null. network error... ");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_RESPONSE;
		EM_DEBUG_FUNC_END();
		return NULL;
	}

FINISH_OFF:
	if (buf) {
		int received_percentage, last_notified_percentage;
		_pop3_received_body_size += EM_SAFE_STRLEN(buf);

		last_notified_percentage = (double)_pop3_last_notified_body_size / (double)_pop3_total_body_size *100.0;
		received_percentage      = (double)_pop3_received_body_size / (double)_pop3_total_body_size *100.0;

		EM_DEBUG_LOG("_pop3_received_body_size = %d, _pop3_total_body_size = %d", _pop3_received_body_size, _pop3_total_body_size);
		EM_DEBUG_LOG("received_percentage = %d, last_notified_percentage = %d", received_percentage, last_notified_percentage);

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
		EM_SAFE_FREE(p->name);
		EM_SAFE_FREE(p->value);
		free(p);p = NULL;
		p = t;
	}
	EM_DEBUG_FUNC_END();
}

void emcore_mime_free_part_header(struct _m_part_header *header)
{
	EM_DEBUG_FUNC_BEGIN();
	if (!header) return ;
	EM_SAFE_FREE(header->type);
	if (header->parameter) emcore_mime_free_param(header->parameter);
	EM_SAFE_FREE(header->subtype);
	EM_SAFE_FREE(header->encoding);
	EM_SAFE_FREE(header->desc);
	EM_SAFE_FREE(header->disp_type);
	if (header->disp_parameter) emcore_mime_free_param(header->disp_parameter);
	free(header); header = NULL;
	EM_DEBUG_FUNC_END();
}

void emcore_mime_free_message_header(struct _m_mesg_header *header)
{
	EM_DEBUG_FUNC_BEGIN();
	if (!header) return ;
	EM_SAFE_FREE(header->version);
	if (header->part_header) emcore_mime_free_part_header(header->part_header);
	free(header); header = NULL;
	EM_DEBUG_FUNC_END();
}

void emcore_mime_free_rfc822_header(struct _rfc822header *header)
{
	EM_DEBUG_FUNC_BEGIN();
	if (!header) return ;
	EM_SAFE_FREE(header->return_path);
	EM_SAFE_FREE(header->received);
	EM_SAFE_FREE(header->reply_to);
	EM_SAFE_FREE(header->date);
	EM_SAFE_FREE(header->from);
	EM_SAFE_FREE(header->subject);
	EM_SAFE_FREE(header->sender);
	EM_SAFE_FREE(header->to);
	EM_SAFE_FREE(header->cc);
	EM_SAFE_FREE(header->bcc);
	free(header); header = NULL;
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
	free(body); body = NULL;
	EM_DEBUG_FUNC_END();
}

void emcore_mime_free_part(struct _m_part *part)
{
	EM_DEBUG_FUNC_BEGIN();
	if (!part) return ;
	if (part->body) emcore_mime_free_part_body(part->body);
	if (part->next) emcore_mime_free_part(part->next);
	free(part);part = NULL;
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
	free(mmsg); mmsg = NULL;
	EM_DEBUG_FUNC_END();
}

void emcore_free_content_info(struct _m_content_info *cnt_info)
{
	EM_DEBUG_FUNC_BEGIN();
	struct attachment_info *p;

	if (!cnt_info) return ;
	EM_SAFE_FREE(cnt_info->text.plain);
	EM_SAFE_FREE(cnt_info->text.plain_charset);
	EM_SAFE_FREE(cnt_info->text.html);
	while (cnt_info->file) {
		p = cnt_info->file->next;
		EM_SAFE_FREE(cnt_info->file->name);
		EM_SAFE_FREE(cnt_info->file->save);
		EM_SAFE_FREE(cnt_info->file->attachment_mime_type);
		EM_SAFE_FREE(cnt_info->file->content_id);
		free(cnt_info->file); cnt_info->file = NULL;
		cnt_info->file = p;
	}
	free(cnt_info);cnt_info = NULL;
	EM_DEBUG_FUNC_END();
}

void emcore_free_attachment_info(struct attachment_info *attchment)
{
	EM_DEBUG_FUNC_BEGIN();
	struct attachment_info *p;

	if (!attchment) return;

	while (attchment) {
		p = attchment->next;
		EM_SAFE_FREE(attchment->name);
		EM_SAFE_FREE(attchment->save);
		EM_SAFE_FREE(attchment->attachment_mime_type);
		EM_SAFE_FREE(attchment->content_id);
		EM_SAFE_FREE(attchment);
		attchment = p;
	}

	EM_DEBUG_FUNC_END();
}

/* get body-part in nested part */
static PARTLIST *emcore_get_allnested_part_full(MAILSTREAM *stream, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code, PARTLIST *section_list)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, body, cnt_info, err_code);

	PART *part_child = body->nested.part;

	while (part_child) {
		section_list = emcore_get_body_full(stream, msg_uid, &part_child->body, cnt_info, err_code, section_list);
		part_child = part_child->next;
	}

	EM_DEBUG_FUNC_END();
	return section_list;
}

/* get body-part in alternative multiple part */
static PARTLIST *emcore_get_alternative_multi_part_full(MAILSTREAM *stream, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code, PARTLIST *section_list)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, body, cnt_info, err_code);

	PART *part_child = body->nested.part;

	/* find the best sub part we can show */
	while (part_child)  {
		section_list = emcore_get_body_full(stream, msg_uid, &part_child->body, cnt_info, err_code, section_list);
		part_child = part_child->next;
	}

	EM_DEBUG_FUNC_END("section_list[%p]", section_list);
	return section_list;
}

/* get body part in signed multiple part */
static PARTLIST *emcore_get_signed_multi_part_full(MAILSTREAM *stream, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code, PARTLIST *section_list)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, body, cnt_info, err_code);

	PART *part_child = body->nested.part;

	/* find the best sub part we can show */
	while (part_child)  {
		section_list = emcore_get_body_full(stream, msg_uid, &part_child->body, cnt_info, err_code, section_list);
		part_child = part_child->next;
	}

	EM_DEBUG_FUNC_END();
	return section_list;
}

/* get body part in encrypted multiple part */
static PARTLIST *emcore_get_encrypted_multi_part_full(MAILSTREAM *stream, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code, PARTLIST *section_list)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, body, cnt_info, err_code);

	/*  "protocol" = "application/pgp-encrypted */
	EM_DEBUG_FUNC_END();
	return section_list;
}

/* get body part in multiple part */
static PARTLIST *emcore_get_multi_part_full(MAILSTREAM *stream, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code, PARTLIST *section_list)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, body, cnt_info, err_code);

	switch (body->subtype[0])  {
		case 'A': 		/*  ALTERNATIV */
			return section_list = emcore_get_alternative_multi_part_full(stream, msg_uid, body, cnt_info, err_code, section_list);

		case 'S': 		/*  SIGNE */
			return section_list = emcore_get_signed_multi_part_full(stream, msg_uid, body, cnt_info, err_code, section_list);

		case 'E': 		/*  ENCRYPTE */
			return section_list = emcore_get_encrypted_multi_part_full(stream, msg_uid, body, cnt_info, err_code, section_list);

		default: 		/*  process all unknown as MIXED (according to the RFC 2047 */
			return section_list = emcore_get_allnested_part_full(stream, msg_uid, body, cnt_info, err_code, section_list);
	}
	EM_DEBUG_FUNC_END();
}


PARTLIST* emcore_get_body_full(MAILSTREAM *stream, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code, PARTLIST *section_list)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, body, cnt_info, err_code);
	char content_type_buffer[512] = { 0, };

	if (!stream || !body || !cnt_info)  {
		EM_DEBUG_EXCEPTION("stream[%p], msg_uid[%d], body[%p], cnt_info[%p]", stream, msg_uid, body, cnt_info);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_FUNC_END();
		return NULL;
	}

	switch (body->type)  {
		case TYPEMULTIPART:
			section_list = emcore_get_multi_part_full(stream, msg_uid, body, cnt_info, err_code, section_list);
			EM_DEBUG_FUNC_END("section_list [%p]", section_list);
			return section_list;

		case TYPEMESSAGE:
			break;

		case TYPETEXT:
		case TYPEAPPLICATION:
		case TYPEAUDIO:
		case TYPEIMAGE:
		case TYPEVIDEO:
		case TYPEMODEL:
		case TYPEOTHER:

			/* Form list of attachment followed by list of inline images */
			if (body->id || body->location || body->disposition.type) {

				char filename[512] = {0, };
				struct attachment_info *current_ai = NULL;
				struct attachment_info *prev_ai = NULL;
				struct attachment_info *ai = NULL;

				if (emcore_get_file_pointer(body, true, filename, cnt_info, (int*)NULL) < 0)
					EM_DEBUG_EXCEPTION("emcore_get_file_pointer failed");
				else {
					/* To form list of attachment info - Attachment list followed by inline attachment list */
					current_ai = cnt_info->file;

					EM_DEBUG_LOG("current_ai - %p", current_ai);

					ai = em_malloc(sizeof(struct attachment_info));
					if (ai == NULL)  {
						EM_DEBUG_EXCEPTION("em_malloc failed...");
						if(err_code)
							*err_code = EMAIL_ERROR_OUT_OF_MEMORY;
						return NULL;
					}

					if ((body->id) || (body->location) || ((body->disposition.type != NULL) && ((body->disposition.type[0] == 'i') || (body->disposition.type[0] == 'I'))))
						ai->type = 1; /* inline contents */
					else
						ai->type = 2; /* attachment */

					ai->name                 = EM_SAFE_STRDUP(filename);
					ai->size                 = body->size.bytes;
					ai->content_id           = EM_SAFE_STRDUP(body->id);
					if (emcore_get_content_type_from_mail_bodystruct(body, 512, content_type_buffer) == EMAIL_ERROR_NONE)
						ai->attachment_mime_type = EM_SAFE_STRDUP(content_type_buffer);

#ifdef __ATTACHMENT_OPTI__
					ai->encoding = body->encoding;
					if (body->sparep)
						ai->section = EM_SAFE_STRDUP(body->sparep);

					EM_DEBUG_LOG("Encoding - %d  Section No - %s ", ai->encoding, ai->section);
#endif

					EM_DEBUG_LOG("Type[%d], Name[%s], Path[%s] ", ai->type, ai->name, ai->save);
					if (body->type == TYPEAPPLICATION)  {
						if (!strcasecmp(body->subtype, MIME_SUBTYPE_DRM_OBJECT))
							ai->drm = EMAIL_ATTACHMENT_DRM_OBJECT;
						else if (!strcasecmp(body->subtype, MIME_SUBTYPE_DRM_RIGHTS))
							ai->drm = EMAIL_ATTACHMENT_DRM_RIGHTS;
						else if (!strcasecmp(body->subtype, MIME_SUBTYPE_DRM_DCF))
							ai->drm = EMAIL_ATTACHMENT_DRM_DCF;
						else if (!strcasecmp(body->subtype, "pkcs7-mime"))
							cnt_info->grab_type = cnt_info->grab_type | GRAB_TYPE_ATTACHMENT;
					}

					if (current_ai == NULL) {
						cnt_info->file = ai;
					} else {
						while(current_ai->next != NULL) {
							if (ai->type == ATTACHMENT) {
								if (current_ai->type == INLINE_ATTACHMENT) 
									break;
							} 

							prev_ai = current_ai;
							current_ai = current_ai->next;
						}

						EM_DEBUG_LOG("current_ai:[%p], prev_ai:[%p]", current_ai, prev_ai);

						if (ai->type == ATTACHMENT) {
							if (prev_ai == NULL) {
								cnt_info->file = ai;
								ai->next = current_ai;
							} else {
								prev_ai->next = ai;
								ai->next = current_ai;
							}
						} else {
							current_ai->next = ai;
						}
					}
				}
			}

			/* if (cnt_info->grab_type == GRAB_TYPE_ATTACHMENT */
			if (cnt_info->grab_type & GRAB_TYPE_ATTACHMENT) {
				if (((body->disposition.type != NULL) && ((body->disposition.type[0] == 'a') || (body->disposition.type[0] == 'A'))) && (cnt_info->file != NULL)) {
					PARAMETER *param = NULL;
					char *fn = NULL;

					param = body->parameter;

					while (param)  {
						if (!strcasecmp(param->attribute, "NAME")) {
							fn = EM_SAFE_STRDUP(param->value);
							break;
						}
						if (!strcasecmp(param->attribute, "FILENAME")) {
							fn = EM_SAFE_STRDUP(param->value);
							break;
						}
						param = param->next;
					}
					if ((fn != NULL)&& (!strcmp(fn, cnt_info->file->name)) && (body->size.bytes == cnt_info->file->size)) /*  checks to zero in on particular attachmen */ {
						section_list = emcore_add_node(section_list, body);
						if (section_list == NULL) {
							EM_DEBUG_EXCEPTION("adding node to section list failed");
							if(err_code)
								*err_code = EMAIL_ERROR_OUT_OF_MEMORY;
							EM_SAFE_FREE(fn);
							return NULL;
						}
						else {
							EM_SAFE_FREE(fn);
							EM_DEBUG_FUNC_END("section_list [%p]", section_list);
							return section_list; /* single attachment download, so if a match found break the recursion */
						}
					}
					EM_SAFE_FREE(fn);
				}
			}
			else {
				/* get a section list which has only plain, html and inline */
				if (!((body->disposition.type != NULL) && ((body->disposition.type[0] == 'a') || (body->disposition.type[0] == 'A'))))/*  if the body not an attachmen */ {
					section_list = emcore_add_node(section_list, body);
					if (section_list == NULL) {
						EM_DEBUG_EXCEPTION("adding node to section list failed");
						if(err_code)
							*err_code = EMAIL_ERROR_OUT_OF_MEMORY;
						return NULL;
					}
				}
			}

			break;

		default:
			break;
	}

	if(section_list == NULL && err_code != NULL) {
		*err_code = EMAIL_ERROR_ON_PARSING;
	}

	EM_DEBUG_FUNC_END("section_list [%p]", section_list);
	return section_list;
}

INTERNAL_FUNC int emcore_get_body_part_list_full(MAILSTREAM *stream, int msg_uid, int account_id, int mail_id, BODY *body, struct _m_content_info *cnt_info, int *err_code, PARTLIST *section_list, int event_handle)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, body, cnt_info, err_code);

	if (!stream || !body || !cnt_info)  {
		EM_DEBUG_EXCEPTION("stream[%p], msg_uid[%d], body[%p], cnt_info[%p]", stream, msg_uid, body, cnt_info);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return FAILURE;
	}
	section_list = emcore_get_body_full(stream, msg_uid, body, cnt_info, err_code, section_list);

	if (section_list == NULL) {
		/*  assumed at least one body part exist */
		if(err_code)
			EM_DEBUG_EXCEPTION("emcore_get_body_full failed [%d]", *err_code);
		else
			EM_DEBUG_EXCEPTION("emcore_get_body_full failed");
		return FAILURE;
	}

	if (emcore_get_body_part_imap_full(stream, msg_uid, account_id, mail_id, section_list, cnt_info, err_code, event_handle) < 0) {
		EM_DEBUG_EXCEPTION("emcore_get_body_part_imap_full failed");
		emcore_free_section_list(section_list);
		return FAILURE;
	}
	emcore_free_section_list(section_list);
	return SUCCESS;
}

static int emcore_write_response_into_file(char *filename, char *write_mode, char *encoded, int encoding_type, char *subtype, int account_id, int mail_id, int *err)
{
	EM_DEBUG_FUNC_BEGIN();
	int temp_decoded_len = 0;
	int inline_support = 0;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int not_found = true;
	int encoded_len = 0;
	int written_bytes = 0;
	unsigned long decoded_len = 0;
	char *decoded = NULL;
	char *decoded_temp = NULL;
	char save_file_name[MAX_PATH+1] = {0, };
	char html_cid_path[MAX_PATH+1] = {0, };
	FILE *fp = NULL;
	PARAMETER *param = NULL;
	PARAMETER *param1 = NULL;


	if (!encoded || !filename || !write_mode) {
		EM_DEBUG_EXCEPTION("Invalid Param ");
		if( err ) *err = EMAIL_ERROR_INVALID_PARAM;  /* prevent 28347 */
		return false;
	}


	EM_DEBUG_LOG("Encoded buffer length [%d]", EM_SAFE_STRLEN(encoded));
	encoded_len = EM_SAFE_STRLEN(encoded);

	EM_DEBUG_LOG("encoding_type [%d]", encoding_type);
	switch (encoding_type)  {
		case ENCQUOTEDPRINTABLE:   {
			unsigned char *orignal = (unsigned char *)g_strdup_printf("%s\r\n", encoded);
			decoded = (char *)rfc822_qprint(orignal, encoded_len + 2, &decoded_len);
			g_free(orignal);
			break;
		}

		case ENCBASE64:
			decoded = (char *)rfc822_base64((unsigned char *)encoded, encoded_len, &decoded_len);
			break;

		default:  {
			unsigned char *orignal = (unsigned char *)g_strdup_printf("%s\r\n", encoded);
			decoded = em_malloc(encoded_len + 3); /*prevent 28347*/
			if(!decoded) {
				EM_DEBUG_EXCEPTION("em_malloc failed");
				error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			memcpy(decoded, orignal, encoded_len + 3);
			decoded_len = encoded_len + 2;
			g_free(orignal);
		}
		break;
	}

	if (decoded != NULL)  {
		EM_DEBUG_LOG("Decoded Length [%d] " , decoded_len);
		EM_DEBUG_LOG("filename [%s] " , filename);

		if (!(fp = fopen(filename, write_mode)))  {
			EM_DEBUG_EXCEPTION("fopen failed - %s", filename);
			error = EMAIL_ERROR_SYSTEM_FAILURE; /*prevent 28347*/
			goto FINISH_OFF;
		}

		if (subtype && subtype[0] == 'H')  {
			char body_inline_id[512] = {0};

			while (strstr(decoded, "cid:")) {
				EM_DEBUG_LOG("Found cid:");
				not_found = true;
				if (g_inline_count) {
					BODY *body_inline = NULL;
					int   inline_count = 0;
					char *decoded_content_id = NULL;
					while (inline_count < g_inline_count && g_inline_list[inline_count]) {
						EM_DEBUG_LOG("inline_count [%d], g_inline_count [%d]", inline_count, g_inline_count);
						body_inline = g_inline_list[inline_count];
						param = body_inline->disposition.parameter;
						param1 = body_inline->parameter;

						memset(body_inline_id, 0x00, 512);

						if (body_inline->id && EM_SAFE_STRLEN(body_inline->id) > 0) { /*prevent 27454*/
							EM_DEBUG_LOG("body_inline->id - %s", body_inline->id);
							EM_DEBUG_LOG("param - %p param1 - %p", param, param1);
							decoded_content_id = strstr(decoded, "cid:");

							if (body_inline->id[0] == '<')
								memcpy(body_inline_id, body_inline->id + 1, EM_SAFE_STRLEN(body_inline->id) - 2);
							else
								memcpy(body_inline_id, body_inline->id , EM_SAFE_STRLEN(body_inline->id));

							EM_DEBUG_LOG("Inline body_inline_id [%s]  ", body_inline_id);

							if ((param || param1) && 0 == strncmp(body_inline_id , decoded_content_id + strlen("cid:"), EM_SAFE_STRLEN(body_inline_id))) {
								EM_DEBUG_LOG(" Inline CID Found ");

								memset(save_file_name, 0x00, MAX_PATH);
								memset(html_cid_path, 0x00, MAX_PATH);

								/*  Finding 'filename' attribute from content inf */
								emcore_get_file_pointer(body_inline, true, save_file_name, NULL, &error);

								if (EM_SAFE_STRLEN(save_file_name) > 0) {
									/*  Content ID will be replaced with its file name in html */
									memcpy(html_cid_path, decoded_content_id , strlen("cid:") + EM_SAFE_STRLEN(body_inline_id));

									EM_DEBUG_LOG("Replacing %s with %s ", html_cid_path, save_file_name);
									if ((decoded_temp = em_replace_string(decoded, html_cid_path, save_file_name))) {
										EM_SAFE_FREE(decoded);
										decoded = decoded_temp;
										decoded_len = EM_SAFE_STRLEN(decoded);
										EM_DEBUG_LOG("Decoded Length [%d] ", decoded_len);
										inline_support = 1;
										not_found = false;
										/* only_body_download = false */
										break;
									}
								}
							}
						}
						inline_count++;
					}
				}

				if (not_found) {
					EM_DEBUG_LOG("not_found is true");
					memset(body_inline_id, 0x00, sizeof(body_inline_id));
					decoded_temp = em_replace_string_with_split_file_path(decoded, "cid:", body_inline_id);
					if (decoded_temp) {
						/* only_body_download = false */
						/* EM_DEBUG_LOG(">>>> decoded_temp 2 [ %s ] ", decoded_temp) */
						EM_SAFE_FREE(decoded);
						decoded = decoded_temp;
						temp_decoded_len = EM_SAFE_STRLEN(body_inline_id);
						decoded_len = EM_SAFE_STRLEN(decoded);
						EM_DEBUG_LOG("Decoded Length [%d] ", decoded_len);
						inline_support = 1;
					}
				}
			}
			g_inline_count = 0;
			EM_SAFE_FREE(g_inline_list);
		}

		EM_DEBUG_LOG("Trying to fwrite. decoded_len [%d]", decoded_len);

		if (decoded_len > 0 && fwrite(decoded, decoded_len, 1, fp) == 0)  {
			EM_DEBUG_EXCEPTION("Error Occured while writing. fwrite(\"%s\") failed. decoded_len [%d], written_bytes [%d] ", decoded, decoded_len, written_bytes);
			error = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("fwrite succeed");
	}
	else {
		EM_DEBUG_EXCEPTION("Error Occured while decoding ");
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	
	if (err)
		*err = error;

	EM_SAFE_FREE(decoded);

	if (fp != NULL)
		fclose(fp);

	EM_DEBUG_FUNC_END();

	return ret;
}


static BODY *emcore_select_body_structure_from_section_list(PARTLIST *section_list,  char *section)
{
	PARTLIST *temp = section_list;
	BODY *body = NULL;

	while (temp != NULL) {
		body = temp->body;
		if (!strcmp(section, body->sparep))
			return body;
		temp = (PARTLIST *)temp->next;
	}
	return body;
}


#define MAX_WRITE_BUFFER_SIZE 0 /* should be tuned */

static int imap_mail_write_body_to_file(MAILSTREAM *stream, int account_id, int mail_id, int is_attachment, char *filepath, int uid, char *section, int encoding, int *decoded_total, char *section_subtype, int *err_code)
{
	EM_PROFILE_BEGIN(imapMailWriteBodyToFile);
	EM_DEBUG_FUNC_BEGIN("stream[%p], filepath[%s], uid[%d], section[%s], encoding[%d], decoded_total[%p], err_code[%p]", stream, filepath, uid, section, encoding, decoded_total, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	FILE *fp = NULL;
	IMAPLOCAL *imaplocal = NULL;
	char tag[16], command[64];
	char *response = NULL;
	char *decoded = NULL;
	int body_size = 0, total = 0;
	char *file_id = NULL;
	char server_uid[129] = { 0, };
	char *filename = NULL;
	int server_response_yn = 0;
	int write_flag = false;
	unsigned char encoded[DOWNLOAD_MAX_BUFFER_SIZE] = {0, };
	unsigned char test_buffer[LOCAL_MAX_BUFFER_SIZE] = {0, };
	int flag_first_write = true;

	if (!stream || !filepath || !section)  {
		EM_DEBUG_EXCEPTION("stream[%p], filepath[%s], uid[%d], section[%s], encoding[%d], decoded_total[%p]", stream, filepath, uid, section, encoding, decoded_total);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	FINISH_OFF_IF_CANCELED;

	if (!(fp = fopen(filepath, "wb+")))  {
		EM_DEBUG_EXCEPTION("fopen failed - %s", filepath);
		err = EMAIL_ERROR_SYSTEM_FAILURE;		/* EMAIL_ERROR_UNKNOWN */
		goto FINISH_OFF;
	}

	imaplocal = stream->local;

	if (!imaplocal->netstream)  {
		EM_DEBUG_EXCEPTION("invalid IMAP4 stream detected... %p", imaplocal->netstream);
		err = EMAIL_ERROR_INVALID_STREAM;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG(" next_decode_string = false  ");
	next_decode_string = false;

	memset(tag, 0x00, sizeof(tag));
	memset(command, 0x00, sizeof(command));

	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
	SNPRINTF(command, sizeof(command), "%s UID FETCH %d BODY.PEEK[%s]\015\012", tag, uid, section);

	EM_DEBUG_LOG("[IMAP4] >>> [%s]", command);

	/*  send command  :  get msgno/uid for all messag */
	if (!net_sout(imaplocal->netstream, command, (int)EM_SAFE_STRLEN(command)))  {
		EM_DEBUG_EXCEPTION("net_sout failed...");
		err = EMAIL_ERROR_CONNECTION_BROKEN;
		goto FINISH_OFF;
	}

	while (imaplocal->netstream)  {
		char *p = NULL;
		char *s = NULL;

		if (!emcore_check_thread_status())  {
			EM_DEBUG_LOG("Canceled...");
			/* 	Is it realy required ? It might cause crashes.
			if (imaplocal->netstream)
				net_close (imaplocal->netstream);
			*/
			imaplocal->netstream = NULL;
			err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

		/*  receive respons */
		if (!(response = net_getline(imaplocal->netstream)))  {
			EM_DEBUG_EXCEPTION("net_getline failed...");
			err = EMAIL_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}
#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG("recv[%s]", response);
#endif

		write_flag = false;
		if (response[0] == '*' && !server_response_yn)  {		/*  start of respons */

			if ((p = strstr(response, "BODY[")) /* || (p = strstr(s + 1, "BODY["))*/) {
				server_response_yn = 1;
				p += strlen("BODY[");
				s = p;

				while (*s != ']')
					s++;

				*s = '\0';

				if (strcmp(section, p))  {
					err = EMAIL_ERROR_INVALID_RESPONSE;
					goto FINISH_OFF;
				}

				if ((p = strstr(s+1, " {")))  {
					p += strlen(" {");
					s = p;

					while (isdigit(*s))
						s++;

					*s = '\0';

					body_size = atoi(p);
				}
				else {	/*  no body length is replied */
					if ((p = strstr(s+1, " \""))) {	/*  seek the termination of double quot */
						char *t = NULL;
						p += strlen(" \"");
						if ((t = strstr(p, "\""))) {
							body_size = t - p;
							*t = '\0';
							EM_DEBUG_LOG("Body  :  start[%p] end[%p]  :  body[%s]", p, t, p);
							/*  need to decod */
							EM_SAFE_FREE(response);
							response = EM_SAFE_STRDUP(p);
							write_flag = true;
						}
						else {
							err = EMAIL_ERROR_INVALID_RESPONSE;
							goto FINISH_OFF;
						}
					}
					else {
						err = EMAIL_ERROR_INVALID_RESPONSE;
						goto FINISH_OFF;
					}
				}

				/* sending progress noti to application.
				1. mail_id
				2. file_id
				3. bodysize
				*/
				parse_file_path_to_filename(filepath, &file_id);

				filename = file_id;
				sprintf(server_uid, "%d", uid);

				EM_DEBUG_LOG("file_id [%s]", file_id);
				EM_DEBUG_LOG("filename [%p]-[%s]", filename, filename);
				EM_DEBUG_LOG("body_size [%d]", body_size);
				EM_DEBUG_LOG("server_uid [%s]", server_uid);
				EM_DEBUG_LOG("mail_id [%d]", mail_id);

				if (is_attachment) {
					EM_DEBUG_LOG("Attachment number [%d]", is_attachment);
					if (!emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_START, mail_id, filename, is_attachment, 0))
						EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_ATTACH_START] Failed >>>> ");
					_imap4_download_noti_interval_value =  body_size *DOWNLOAD_NOTI_INTERVAL_PERCENT / 100;
					_imap4_total_body_size = body_size;
				}
				else {
					if (multi_part_body_size) {
						EM_DEBUG_LOG("Multipart body size is [%d]", multi_part_body_size);
						if (!emcore_notify_network_event(NOTI_DOWNLOAD_MULTIPART_BODY, mail_id, filename, multi_part_body_size, 0))
							EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>> ");
						_imap4_download_noti_interval_value =  multi_part_body_size *DOWNLOAD_NOTI_INTERVAL_PERCENT / 100;
						/*  _imap4_total_body_size should be set before calling this functio */
						/* _imap4_total_body_size */
					}
					else {
						if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_START, mail_id, filename, body_size, 0))
							EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>>");
						_imap4_download_noti_interval_value =  body_size *DOWNLOAD_NOTI_INTERVAL_PERCENT / 100;
						_imap4_total_body_size = body_size;
					}
				}
				if (_imap4_download_noti_interval_value > DOWNLOAD_NOTI_INTERVAL_SIZE) {
					_imap4_download_noti_interval_value = DOWNLOAD_NOTI_INTERVAL_SIZE;
				}
				if (body_size < DOWNLOAD_MAX_BUFFER_SIZE) {
					if (net_getbuffer (imaplocal->netstream, (long)body_size, (char *)encoded) <= 0) {
						EM_DEBUG_EXCEPTION("net_getbuffer failed...");
						err = EMAIL_ERROR_NO_RESPONSE;
						goto FINISH_OFF;
					}

					if (!emcore_write_response_into_file(filepath, "wb+", (char *)encoded, encoding, section_subtype, account_id, mail_id, &err)) {
						EM_DEBUG_EXCEPTION("write_response_into_file failed [%d]", err);
						goto FINISH_OFF;
					}

					total = EM_SAFE_STRLEN((char *)encoded);
					EM_DEBUG_LOG("total = %d", total);
					EM_DEBUG_LOG("write_response_into_file successful %s.....", filename);

					if (((_imap4_last_notified_body_size  + _imap4_download_noti_interval_value) <=  _imap4_received_body_size)
								|| (_imap4_received_body_size >= _imap4_total_body_size))		/*  100  */ {
						/*  In some situation, total_encoded_len includes the length of dummy bytes. So it might be greater than body_size */
						int gap = 0;
						if (total > body_size)
							gap = total - body_size;
						_imap4_received_body_size -= gap;
						_imap4_last_notified_body_size = _imap4_received_body_size;

						if(body_size > 0)
							EM_DEBUG_LOG("DOWNLOADING STATUS NOTIFY  :  Encoded[%d] / [%d] = %d %% Completed. -- Total Decoded[%d]", total, body_size, 100*total/body_size, total);

						if(_imap4_total_body_size > 0)
							EM_DEBUG_LOG("DOWNLOADING STATUS NOTIFY  :  Total[%d] / [%d] = %d %% Completed.", _imap4_received_body_size, _imap4_total_body_size, 100*_imap4_received_body_size/_imap4_total_body_size);

						if (is_attachment) {
							if (_imap4_total_body_size && !emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_START, mail_id, filename, is_attachment, 100 *_imap4_received_body_size / _imap4_total_body_size))
								EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_ATTACH_START] Failed >>>> ");
						}
						else {
							if (multi_part_body_size) {
								if (!emcore_notify_network_event(NOTI_DOWNLOAD_MULTIPART_BODY, mail_id, filename, _imap4_total_body_size, _imap4_received_body_size))
									EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>> ");
							}
							else {
								if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_START, mail_id, filename, _imap4_total_body_size, _imap4_received_body_size))
									EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>>");
							}
						}	/*  if (is_attachment) .. else .. */
					}
				}
				else {
					int temp_body_size = body_size;
					int x = 0;

					if (encoding == ENCBASE64)
						x = (sizeof(encoded)/78) *78; /*  to solve base64 decoding pro */
					else
						x = sizeof(encoded)-1;

					memset(test_buffer, 0x00, sizeof(test_buffer));
					while (temp_body_size && (total <body_size)) {

						memset(test_buffer, 0x00, sizeof(test_buffer));
						while ((total != body_size) && temp_body_size && ((EM_SAFE_STRLEN((char *)test_buffer) + x) < sizeof(test_buffer))) {
							memset(encoded, 0x00, sizeof(encoded));

							if (net_getbuffer (imaplocal->netstream, (long)x, (char *)encoded) <= 0) {
								EM_DEBUG_EXCEPTION("net_getbuffer failed...");
								err = EMAIL_ERROR_NO_RESPONSE;
								goto FINISH_OFF;
							}

							temp_body_size = temp_body_size - x;
							strncat((char *)test_buffer, (char *)encoded, EM_SAFE_STRLEN((char *)encoded));
							total = total + x;
							_imap4_received_body_size += EM_SAFE_STRLEN((char *)encoded);

							if ( !(temp_body_size/x) && temp_body_size%x)
								x = temp_body_size%x;

							if (((_imap4_last_notified_body_size  + _imap4_download_noti_interval_value) <=  _imap4_received_body_size)
								|| (_imap4_received_body_size >= _imap4_total_body_size))		/*  100  */ {
								/*  In some situation, total_encoded_len includes the length of dummy bytes. So it might be greater than body_size */
								int gap = 0;
								if (total > body_size)
									gap = total - body_size;
								_imap4_received_body_size -= gap;
								_imap4_last_notified_body_size = _imap4_received_body_size;

								/* EM_DEBUG_LOG("DOWNLOADING STATUS NOTIFY  :  Encoded[%d] / [%d] = %d %% Completed. -- Total Decoded[%d]", total, body_size, 100*total/body_size, total) */
								EM_DEBUG_LOG("DOWNLOADING STATUS NOTIFY  :  Total[%d] / [%d] = %d %% Completed.", _imap4_received_body_size, _imap4_total_body_size, 100*_imap4_received_body_size/_imap4_total_body_size);

								if (is_attachment) {
									if (!emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_START, mail_id, filename, is_attachment, 100 *_imap4_received_body_size / _imap4_total_body_size))
										EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_ATTACH_START] Failed >>>> ");
								}
								else {
									if (multi_part_body_size) {
										/* EM_DEBUG_LOG("DOWNLOADING..........  :  Multipart body size is [%d]", multi_part_body_size) */
									if (!emcore_notify_network_event(NOTI_DOWNLOAD_MULTIPART_BODY, mail_id, filename, _imap4_total_body_size, _imap4_received_body_size))
											EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>> ");
									}
									else {
										if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_START, mail_id, filename, _imap4_total_body_size, _imap4_received_body_size))
											EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>>");
									}
								}	/*  if (is_attachment) .. else .. */
							}


		}

							if (flag_first_write == true) {
								if (!emcore_write_response_into_file(filepath, "wb+", (char *)test_buffer, encoding, section_subtype, account_id, mail_id, &err)) {
									EM_DEBUG_EXCEPTION("write_response_into_file %s failed [%d]", filepath, err);
									goto FINISH_OFF;
								}
								flag_first_write = false;
							}
							else {
								if (!emcore_write_response_into_file(filepath, "ab+", (char *)test_buffer, encoding, section_subtype, account_id, mail_id, &err)) /*  append */ {
									EM_DEBUG_EXCEPTION("write_response_into_file %s failed [%d]", filepath, err);
									goto FINISH_OFF;
								}
							}
							EM_DEBUG_LOG("%d has been written", EM_SAFE_STRLEN((char *)test_buffer));
							/*  notif */
						}
				}

			}
			else  {
				err = EMAIL_ERROR_INVALID_RESPONSE;
				goto FINISH_OFF;
			}

		}
		else if (!strncmp(response, tag, EM_SAFE_STRLEN(tag)))  {		/*  end of respons */
			if (!strncmp(response + EM_SAFE_STRLEN(tag) + 1, "OK", 2))  {
				EM_SAFE_FREE(response);
			}
			else  {		/*  'NO' or 'BAD */
				err = EMAIL_ERROR_IMAP4_FETCH_UID_FAILURE;
				goto FINISH_OFF;
			}

			break;
		}
		else if (!strcmp(response, ")"))  {
			/*  The end of response which contains body informatio */
			write_flag = false;
		}

	}	/*  while (imaplocal->netstream)  */

	if (decoded_total != NULL)
		*decoded_total = total;

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE(decoded);
	EM_SAFE_FREE(response);

	if (fp != NULL)
		fclose(fp);

	if (ret == false) {	/*  delete temp fil */
		struct stat temp_file_stat;
		if (filepath &&  stat(filepath, &temp_file_stat) == 0)
			remove(filepath);
	}

	if (err_code != NULL)
		*err_code = err;

	EM_PROFILE_END(imapMailWriteBodyToFile);

	return ret;
}

static int emcore_get_body_part_imap_full(MAILSTREAM *stream, int msg_uid, int account_id, int mail_id, PARTLIST *section_list, struct _m_content_info *cnt_info, int *err_code, int event_handle)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, section_list, cnt_info, err_code);

	int err = EMAIL_ERROR_NONE;
	char sections[IMAP_MAX_COMMAND_LENGTH] = { 0, };
	IMAPLOCAL *imaplocal = NULL;
	char tag[16] = { 0, }, command[IMAP_MAX_COMMAND_LENGTH] = { 0, };
	char section[16] = {0};
	char *response = NULL;
	BODY *body = NULL;
	int server_response_yn = 0;
	int body_size = 0;
	char *buf = NULL;
	char filename[512] = {0, };
	int return_value = 0 ;
	int encoding = 0;
	unsigned char encoded[DOWNLOAD_MAX_BUFFER_SIZE] = {0};
	unsigned char test_buffer[LOCAL_MAX_BUFFER_SIZE] = {0};
	struct attachment_info *ai = NULL;
	int i = 0;
	int total = 0;
	int flag_first_write = 1;
	int imap4_total_body_download_progress = 0, progress = 0;

	if (!(imaplocal = stream->local) || !imaplocal->netstream || !section_list || !cnt_info)  {
		EM_DEBUG_EXCEPTION("invalid IMAP4 stream detected...");
		err = EMAIL_ERROR_INVALID_PARAM;
		return_value = -1;
		goto FINISH_OFF;
	}

	if (section_list != NULL) {
		PARTLIST *temp = section_list;

		if (cnt_info->grab_type == GRAB_TYPE_ATTACHMENT) {
			/*  to download attachment */
			body = temp->body;
			if (body->sparep != NULL)  {
				snprintf(sections, sizeof(sections), "BODY.PEEK[%s]", (char *)body->sparep);
			}
			else {
				EM_DEBUG_EXCEPTION("body->sparep can not be null. ");
				return_value = -1;
				goto FINISH_OFF;

			}
		}
		else {
			while (temp != NULL) {
				char t[64] = {0};
				body = temp->body;

				if ((body->type == TYPETEXT)  || (body->id != NULL) || ((body->disposition.type != NULL) && ((body->disposition.type[0] == 'i') || (body->disposition.type[0] == 'I')))) {
					snprintf(t, sizeof(t), "BODY.PEEK[%s] ", (char *)body->sparep);		/*  body parts seperated by period */
					strcat(sections, t);
				}
				temp = (PARTLIST *)temp->next;
			}
		}
	}

	if ((EM_SAFE_STRLEN(sections) == (sizeof(sections)-1)) || (EM_SAFE_STRLEN(sections) == 0)) {
			EM_DEBUG_EXCEPTION(" Too many body parts or nil. IMAP command may cross 1000bytes.");
			return_value = -1;
			goto FINISH_OFF;
		}

	if (sections[EM_SAFE_STRLEN(sections)-1] == ' ') {
		sections[EM_SAFE_STRLEN(sections)-1] = '\0';
	}

	EM_DEBUG_LOG("sections <%s>", sections);

	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
	SNPRINTF(command, sizeof(command), "%s UID FETCH %d (%s)\015\012", tag, msg_uid, sections);
	EM_DEBUG_LOG("command %s", command);

	if (EM_SAFE_STRLEN(command) == (sizeof(command)-1)) {
		EM_DEBUG_EXCEPTION("Too many body parts. IMAP command will fail.");
		return_value = -1;
		goto FINISH_OFF;
	}

	/*  send command  :  get msgno/uid for all message */
	if (!net_sout(imaplocal->netstream, command, (int)EM_SAFE_STRLEN(command)))  {
		EM_DEBUG_EXCEPTION("net_sout failed...");
		err = EMAIL_ERROR_CONNECTION_BROKEN;
		return_value = -1;
		goto FINISH_OFF;
	}
	while (imaplocal->netstream)  {

		/*  receive respons */
		if (!(response = net_getline(imaplocal->netstream)))  {
			EM_DEBUG_EXCEPTION("net_getline failed...");
			err = EMAIL_ERROR_INVALID_RESPONSE;
			return_value = -1;
			goto FINISH_OFF;
		}

		if (strstr(response, "BODY[")) {

			if (!server_response_yn)  		/*  start of response */ {
				if (response[0] != '*') {
					err = EMAIL_ERROR_INVALID_RESPONSE;
					EM_DEBUG_EXCEPTION("Start of response doesn't contain *");
					return_value = -1;
					goto FINISH_OFF;
				}
				server_response_yn = 1;
			}

			flag_first_write = 1;
			total = 0;
			memset(encoded, 0x00, sizeof(encoded));

			if (emcore_get_section_body_size(response, section, &body_size)<0) {
				EM_DEBUG_EXCEPTION("emcore_get_section_body_size failed [%d]", err);
				err = EMAIL_ERROR_INVALID_RESPONSE;
				return_value = -1;
				goto FINISH_OFF;
			}
			EM_DEBUG_LOG("body_size-%d", body_size);

			/*  get body from seciton_list */
			if ((body = emcore_select_body_structure_from_section_list(section_list, section)) == NULL) {
				EM_DEBUG_EXCEPTION("emcore_select_body_structure_from_section_list failed [%d]", err);
				err = EMAIL_ERROR_INVALID_RESPONSE;
				return_value = -1;
				goto FINISH_OFF;
			}
			encoding = body->encoding;

			/* if (emcore_get_file_pointer(account_id, mail_id, body, buf, cnt_info , err)<0) {
				EM_DEBUG_EXCEPTION("emcore_get_file_pointer failed [%d]", err);
				goto FINISH_OFF;
			}*/

			if (!emcore_get_temp_file_name(&buf, &err))  {
				EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed [%d]", err);
				goto FINISH_OFF;
			}

			EM_DEBUG_LOG("buf :  %s", buf);

			/*  notifying UI start */
			/*  parse_file_path_to_filename(buf, &file_id);			*/
			/*  EM_DEBUG_LOG(">>>> filename - %p >>>>>>", file_id) */

			if (body->type == TYPETEXT && body->subtype && (!body->disposition.type || (body->disposition.type && (body->disposition.type[0] == 'i' || body->disposition.type[0] == 'I')))) {
				if (body->subtype[0] == 'H')
					cnt_info->text.html = buf;
				else
					cnt_info->text.plain = buf;

				PARAMETER *param = NULL;

				param = body->parameter;

				while (param)  {
					if (!strcasecmp(param->attribute, "CHARSET"))  {
						cnt_info->text.plain_charset = EM_SAFE_STRDUP(param->value);
						break;
					}
					param = param->next;
				}
			}
			else if (body->subtype && (body->id || body->location || body->disposition.type)) { /*prevent 23712*/ /*refactoring : body->subtype*/

				if (emcore_get_file_pointer(body, false, filename, cnt_info , &err)<0) {
					EM_DEBUG_EXCEPTION("emcore_get_file_pointer failed [%d]", err);
					goto FINISH_OFF;
				}

				/* Search info from attachment list followed by inline attachment list */

				ai = cnt_info->file;
				EM_DEBUG_LOG("ai - %p ", (ai));

				/* For Inline content append to the end */
				for (i = 1; ai; ai = ai->next, i++) {
					if (ai->save == NULL && (ai->content_id != NULL && EM_SAFE_STRCMP(ai->content_id, body->id) == 0)) {
						EM_DEBUG_LOG("Found matching details ");
						ai->save = buf;
						break;
					}
				}
			}

			FINISH_OFF_IF_CANCELED;

			if (cnt_info->grab_type == GRAB_TYPE_ATTACHMENT) {
				if (!emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_START, mail_id, buf, cnt_info->file_no, 0))
					EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_ATTACH_START] Failed >>>> ");

				_imap4_download_noti_interval_value =  body_size *DOWNLOAD_NOTI_INTERVAL_PERCENT / 100;
				_imap4_total_body_size = body_size;
			}
			else {
				if (multi_part_body_size) {
					EM_DEBUG_LOG("Multipart body size is [%d]", multi_part_body_size);
					if (!emcore_notify_network_event(NOTI_DOWNLOAD_MULTIPART_BODY, mail_id, buf, multi_part_body_size, 0))
						EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>> ");

					_imap4_download_noti_interval_value =  multi_part_body_size *DOWNLOAD_NOTI_INTERVAL_PERCENT / 100;
				}
				else {
					if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_START, mail_id, buf, body_size, 0))
						EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>> ");

					_imap4_download_noti_interval_value =  body_size *DOWNLOAD_NOTI_INTERVAL_PERCENT / 100;
					_imap4_total_body_size = body_size;
				}

			}

			if (_imap4_download_noti_interval_value > DOWNLOAD_NOTI_INTERVAL_SIZE) {
				_imap4_download_noti_interval_value = DOWNLOAD_NOTI_INTERVAL_SIZE;
			}

			/* EM_SAFE_FREE(file_id) */
			/* notifying UI end */

			if (body_size < DOWNLOAD_MAX_BUFFER_SIZE) {
				if (net_getbuffer (imaplocal->netstream, (long)body_size, (char *)encoded) <= 0) {
					EM_DEBUG_EXCEPTION("net_getbuffer failed...");
					err = EMAIL_ERROR_NO_RESPONSE;
					return_value = -1;
					goto FINISH_OFF;
				}
				if (!emcore_write_response_into_file(buf, "wb+", (char *)encoded, encoding, body->subtype, account_id, mail_id, &err)) {
					EM_DEBUG_EXCEPTION("write_response_into_file failed [%d]", err);
					return_value = -1;
					goto FINISH_OFF;
				}

				EM_DEBUG_LOG("total = %d", total);
				EM_DEBUG_LOG("write_response_into_file successful %s.....", buf);

				total = _imap4_received_body_size = EM_SAFE_STRLEN((char *)encoded);

				EM_DEBUG_LOG("_imap4_last_notified_body_size [%d]", _imap4_last_notified_body_size);
				EM_DEBUG_LOG("_imap4_download_noti_interval_value [%d]", _imap4_download_noti_interval_value);
				EM_DEBUG_LOG("_imap4_received_body_size [%d]", _imap4_received_body_size);
				EM_DEBUG_LOG("_imap4_total_body_size [%d] ", _imap4_total_body_size);

				if (((_imap4_last_notified_body_size  + _imap4_download_noti_interval_value) <=  _imap4_received_body_size)
					|| (_imap4_received_body_size >= _imap4_total_body_size))		/*  100  */ {
					/*  In some situation, total_encoded_len includes the length of dummy bytes. So it might be greater than body_size */
					int gap = 0;

					if (total > body_size)
						gap = total - body_size;
					_imap4_received_body_size -= gap;
					_imap4_last_notified_body_size = _imap4_received_body_size;
					if (_imap4_total_body_size)
						imap4_total_body_download_progress = 100*_imap4_received_body_size/_imap4_total_body_size;
					else
						imap4_total_body_download_progress = _imap4_received_body_size;

					EM_DEBUG_LOG("3  :  body_size %d", body_size);

					if (body_size)
						progress = 100*total/body_size;
					else
						progress = body_size;

					EM_DEBUG_LOG("DOWNLOADING STATUS NOTIFY  :  Encoded[%d] / [%d] = %d %% Completed. -- Total Decoded[%d]", total, body_size, progress, total);
					EM_DEBUG_LOG("DOWNLOADING STATUS NOTIFY  :  Total[%d] / [%d] = %d %% Completed.", _imap4_received_body_size, _imap4_total_body_size, imap4_total_body_download_progress);

					if (cnt_info->grab_type == GRAB_TYPE_ATTACHMENT) {
						if (!emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_START, mail_id, buf, cnt_info->file_no, imap4_total_body_download_progress))
							EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_ATTACH_START] Failed >>>> ");
					}
					else {
							if (multi_part_body_size) {
								/* EM_DEBUG_LOG("DOWNLOADING..........  :  Multipart body size is [%d]", multi_part_body_size) */
							if (!emcore_notify_network_event(NOTI_DOWNLOAD_MULTIPART_BODY, mail_id, buf, _imap4_total_body_size, _imap4_received_body_size))
									EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>> ");
							}
							else {
							if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_START, mail_id, buf, _imap4_total_body_size, _imap4_received_body_size))
									EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>>");
							}
					}	/*  if (is_attachment) .. else .. */
				}

				EM_DEBUG_LOG("4");

			}
			else {
				int temp_body_size = body_size;
				int x = 0;

				if (encoding == ENCBASE64)
					x = (sizeof(encoded)/78) *78; /*  to solve base64 decoding pro */
				else
					x = sizeof(encoded)-1;

				memset(test_buffer, 0x00, sizeof(test_buffer));
				while (temp_body_size && (total <body_size)) {

					memset(test_buffer, 0x00, sizeof(test_buffer));
					while ((total != body_size) && temp_body_size && ((EM_SAFE_STRLEN((char *)test_buffer) + x) < sizeof(test_buffer))) {
						memset(encoded, 0x00, sizeof(encoded));

						if (net_getbuffer (imaplocal->netstream, (long)x, (char *)encoded) <= 0) {
							EM_DEBUG_EXCEPTION("net_getbuffer failed...");
							err = EMAIL_ERROR_NO_RESPONSE;
							return_value = -1;
							goto FINISH_OFF;
						}

						temp_body_size = temp_body_size - x;
						strncat((char *)test_buffer, (char *)encoded, EM_SAFE_STRLEN((char *)encoded));
						total = total + x;
						_imap4_received_body_size += EM_SAFE_STRLEN((char *)encoded);

						EM_DEBUG_LOG("total = %d", total);

						if ( !(temp_body_size/x) && temp_body_size%x)
							x = temp_body_size%x;

						EM_DEBUG_LOG(" _imap4_last_notified_body_size - %d ", _imap4_last_notified_body_size);
						EM_DEBUG_LOG(" _imap4_download_noti_interval_value - %d ", _imap4_download_noti_interval_value);
						EM_DEBUG_LOG(" _imap4_received_body_size - %d ", _imap4_received_body_size);
						EM_DEBUG_LOG(" _imap4_received_body_size - %d ", _imap4_received_body_size);
						EM_DEBUG_LOG(" _imap4_total_body_size - %d ", _imap4_total_body_size);

						if (_imap4_total_body_size)
							imap4_total_body_download_progress = 100*_imap4_received_body_size/_imap4_total_body_size;
						else
							imap4_total_body_download_progress = _imap4_received_body_size;

						if (((_imap4_last_notified_body_size  + _imap4_download_noti_interval_value) <=  _imap4_received_body_size)
							|| (_imap4_received_body_size >= _imap4_total_body_size))		/*  100  */ {
							/*  In some situation, total_encoded_len includes the length of dummy bytes. So it might be greater than body_size */
							int gap = 0;
							if (total > body_size)
								gap = total - body_size;
							_imap4_received_body_size -= gap;
							_imap4_last_notified_body_size = _imap4_received_body_size;

							progress = 100*total/body_size; /*prevent 40023*/

							EM_DEBUG_LOG("DOWNLOADING STATUS NOTIFY  :  Encoded[%d] / [%d] = %d %% Completed. -- Total Decoded[%d]", total, body_size, progress, total);
							EM_DEBUG_LOG("DOWNLOADING STATUS NOTIFY  :  Total[%d] / [%d] = %d %% Completed.", _imap4_received_body_size, _imap4_total_body_size, imap4_total_body_download_progress);

							if (cnt_info->grab_type == GRAB_TYPE_ATTACHMENT) {
								if (!emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_START, mail_id, buf, cnt_info->file_no, imap4_total_body_download_progress))
									EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_ATTACH_START] Failed >>>> ");
							}
							else {
								if (multi_part_body_size) {
									/* EM_DEBUG_LOG("DOWNLOADING..........  :  Multipart body size is [%d]", multi_part_body_size) */
									if (!emcore_notify_network_event(NOTI_DOWNLOAD_MULTIPART_BODY, mail_id, buf, _imap4_total_body_size, _imap4_received_body_size))
										EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>> ");
								}
								else {
									if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_START, mail_id, buf, _imap4_total_body_size, _imap4_received_body_size))
										EM_DEBUG_EXCEPTION(" emcore_notify_network_event [ NOTI_DOWNLOAD_BODY_START] Failed >>>>");
								}
							}	/*  if (is_attachment) .. else .. */
						}
					}

					if (flag_first_write == 1) {
						if (!emcore_write_response_into_file(buf, "wb+", (char *)test_buffer, encoding, body->subtype, account_id, mail_id, &err)) {
							EM_DEBUG_EXCEPTION("write_response_into_file %s failed [%d]", buf, err);
							return_value = -1;
							goto FINISH_OFF;
						}
						flag_first_write = 0;
					}
					else {
						if (!emcore_write_response_into_file(buf, "ab+", (char *)test_buffer, encoding, body->subtype, account_id, mail_id, &err)) /*  append */ {
							EM_DEBUG_EXCEPTION("write_response_into_file %s failed [%d]", buf, err);
							return_value = -1;
							goto FINISH_OFF;
						}
					}
					EM_DEBUG_LOG("%d has been written", EM_SAFE_STRLEN((char *)test_buffer));
				}
			}
		}
		else if (!strncmp(response, tag, EM_SAFE_STRLEN(tag)))  /*  end of respons */ {
			if (!strncmp(response + EM_SAFE_STRLEN(tag) + 1, "OK", 2))
				EM_SAFE_FREE(response);
			else 			/*  'NO' or 'BAD */ {
				err = EMAIL_ERROR_IMAP4_FETCH_UID_FAILURE;
				return_value = -1;
				goto FINISH_OFF;
			}

			break;
		}
		else if (!strcmp(response, ")"))  {

		}

		free(response);
		response = NULL;
	}

	return_value = 0;

	FINISH_OFF:

	if (err_code)
		*err_code = err;

	EM_SAFE_FREE(response);

	return return_value;
}

static int _find_duplicated_inline_content_file(char *input_source_file_name, struct _m_content_info *input_content_info, bool *output_result)
{
	EM_DEBUG_FUNC_BEGIN("input_source_file_name [%p], input_content_info [%p], output_result [%p]", input_source_file_name,input_content_info, output_result);
	struct attachment_info *cur_attachment_info = NULL;
	int  err = EMAIL_ERROR_NONE;
	bool result = false;

	if(!input_source_file_name || !input_content_info || !output_result) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	cur_attachment_info = input_content_info->file;

	while(cur_attachment_info) {
		if(strcmp(input_source_file_name, cur_attachment_info->name) == 0) {
			result = true;
			break;
		}
		cur_attachment_info = cur_attachment_info->next;
	}

	*output_result = result;

	EM_DEBUG_FUNC_END("err [%d], result [%d]", err, result);
	return err;
}

static int _modify_file_name_string_for_duplicated_inline_content(char *input_source_file_name, struct _m_content_info *input_content_info)
{
	EM_DEBUG_FUNC_BEGIN("input_source_file_name [%p], input_content_info [%p]", input_source_file_name,input_content_info);
	int   err = EMAIL_ERROR_NONE;
	char  temp_file_name[MAX_PATH] = { 0, };
	char *file_name = NULL;
	char *extension = NULL;

	if(!input_source_file_name || !input_content_info) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if ( (err = em_get_file_name_and_extension_from_file_path(input_source_file_name, &file_name, &extension)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_get_file_name_and_extension_from_file_path failed [%d]", err);
		goto FINISH_OFF;
	}

	if(file_name && extension)
		SNPRINTF(temp_file_name, MAX_PATH, "%s_.%s", file_name, extension);
	else if(!extension)
		SNPRINTF(temp_file_name, MAX_PATH, "%s_", file_name);

	EM_SAFE_STRCPY(input_source_file_name, temp_file_name);

FINISH_OFF:

	EM_SAFE_FREE(file_name);
	EM_SAFE_FREE(extension);

	EM_DEBUG_FUNC_END("err [%d], temp_file_name [%s]", err, temp_file_name);
	return err;
}

static int emcore_get_file_pointer(BODY *body, bool input_check_duplicated_file_name, char *output_file_name_string, struct _m_content_info *cnt_info , int *err)
{
	EM_DEBUG_FUNC_BEGIN();

	char *decoded_filename = NULL;
	char attachment_file_name[MAX_PATH] = { 0, };
	char attachment_file_name_source[MAX_PATH] = {0, };
	int error = EMAIL_ERROR_NONE;
	bool file_name_duplicated = false;

	if ((body->type == TYPETEXT) && (body->disposition.type == NULL)) {
		EM_DEBUG_LOG("body->type == TYPETEXT");
		if (!cnt_info) {
			EM_DEBUG_EXCEPTION("But, cnt_info is null");
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		if (body->subtype[0] == 'H') {
			if (cnt_info->text.plain_charset != NULL) {
				memcpy(output_file_name_string, cnt_info->text.plain_charset, EM_SAFE_STRLEN(cnt_info->text.plain_charset));
				strcat(output_file_name_string, HTML_EXTENSION_STRING);
			}
			else {
				memcpy(output_file_name_string, "UTF-8.htm", strlen("UTF-8.htm"));
			}
			cnt_info->text.html = EM_SAFE_STRDUP(output_file_name_string);
		}
		else {
			PARAMETER *param = body->parameter;
			char charset_string[512];

			if (emcore_get_attribute_value_of_body_part(param, "CHARSET", charset_string, 512, false, &error)) {
				cnt_info->text.plain_charset = EM_SAFE_STRDUP(charset_string);
				memcpy(output_file_name_string, cnt_info->text.plain_charset, EM_SAFE_STRLEN(cnt_info->text.plain_charset));
			}
			else
				memcpy(output_file_name_string, "UTF-8", strlen("UTF-8"));

			cnt_info->text.plain = EM_SAFE_STRDUP(output_file_name_string);
		}

	}
	else if ((body->id != NULL) || ((body->disposition.type != NULL) && ((body->disposition.type[0] == 'i') || (body->disposition.type[0] == 'I')))) {
		/*  body id is exising or disposition type is inline */
		size_t len = 0;
		if (body->parameter) /* Get actual name of file */ {
			PARAMETER *param_body = body->parameter;
			if (!emcore_get_attribute_value_of_body_part(param_body, "NAME", attachment_file_name_source, MAX_PATH, true, &error))
				emcore_get_attribute_value_of_body_part(param_body, "CHARSET", attachment_file_name_source, MAX_PATH, true, &error);
			if (!emcore_make_attachment_file_name_with_extension(attachment_file_name_source, body->subtype, attachment_file_name, MAX_PATH, &error)) {
				EM_DEBUG_EXCEPTION("emcore_make_attachment_file_name_with_extension failed [%d]", error);
				goto FINISH_OFF;
			}
		}
		else if (body->disposition.type)  {
			PARAMETER *param_disposition = body->disposition.parameter;
			EM_DEBUG_LOG("body->disposition.type exist");
			emcore_get_attribute_value_of_body_part(param_disposition, "filename", attachment_file_name_source, MAX_PATH, true, &error);
			if (!emcore_make_attachment_file_name_with_extension(attachment_file_name_source, body->subtype, attachment_file_name, MAX_PATH, &error)) {
				EM_DEBUG_EXCEPTION("emcore_make_attachment_file_name_with_extension failed [%d]", error);
				goto FINISH_OFF;
			}
		}
		else {	/*  body id is not null but disposition type is null */
			if ((body->id[0] == '<'))
				SNPRINTF(attachment_file_name, MAX_PATH, "%s", body->id+1); /*  fname = em_parse_filename(body->id + 1 */
			else
				SNPRINTF(attachment_file_name, MAX_PATH, "%s", body->id); /*  fname = em_parse_filename(body->id */

			len = EM_SAFE_STRLEN(attachment_file_name);

			if ((len > 1) && (attachment_file_name[len-1] == '>'))
				attachment_file_name[len - 1] = '\0';
			decoded_filename = emcore_decode_rfc2047_text(attachment_file_name, &error);
		}
		EM_DEBUG_LOG("attachment_file_name [%s]", attachment_file_name);
		if (decoded_filename != NULL)
			memcpy(output_file_name_string, decoded_filename, EM_SAFE_STRLEN(decoded_filename));
		else
			memcpy(output_file_name_string, attachment_file_name, EM_SAFE_STRLEN(attachment_file_name));

	}
	else if (body->disposition.type != NULL) {	/*  disposition type is existing and not inline and body_id is nul */
		PARAMETER *param = body->parameter;
		if (!emcore_get_attribute_value_of_body_part(param, "NAME", attachment_file_name, MAX_PATH, true, &error))
			emcore_get_attribute_value_of_body_part(param, "FILENAME", attachment_file_name, MAX_PATH, true, &error);
		memcpy(output_file_name_string, attachment_file_name, EM_SAFE_STRLEN(attachment_file_name));
	}

	if(input_check_duplicated_file_name) {
		if ( (error = _find_duplicated_inline_content_file(output_file_name_string, cnt_info, &file_name_duplicated)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("_find_duplicated_inline_content_file failed [%d]", error);
			goto FINISH_OFF;
		}

		if (file_name_duplicated == true) {
			if ( ( error = _modify_file_name_string_for_duplicated_inline_content(output_file_name_string, cnt_info)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("_modify_file_name_string_for_duplicated_inline_content failed [%d]", error);
				goto FINISH_OFF;
			}
		}
	}

FINISH_OFF:

	EM_SAFE_FREE(decoded_filename);

	if (err)
		*err = error;

	EM_DEBUG_FUNC_END("output_file_name_string[%s], error [%d]", output_file_name_string, error);
	return SUCCESS;
}


static PARTLIST *emcore_add_node(PARTLIST *section_list, BODY *body)
{
	PARTLIST *temp = (PARTLIST  *)malloc(sizeof(PARTLIST));

	if (temp == NULL) {
		EM_DEBUG_EXCEPTION("PARTLIST node creation failed");
		return NULL;
	}
	temp->body = body;
	temp->next = NULL;

	if (section_list == NULL)/*  first node in list */ {
		section_list = temp;
	}
	else/*  has min 1 nod */ {
		PARTLIST *t = section_list;
		while (t->next != NULL) /*  go to last nod */ {
			t = (PARTLIST *) t->next;
		}
		t->next = (PART *)temp;/*  I think this should be PARTLIST, but why this is PART */
/*
in imap-2007e/c-client/mail.h
PARTLIST{
  BODY *body;
  PART *next;
};
*/
	}
	return section_list;
}


static void emcore_free_section_list(PARTLIST *section_list)
{
	PARTLIST *temp = NULL;

	while (section_list != NULL) {
		temp = (PARTLIST *)section_list->next;
		EM_SAFE_FREE(section_list);
		section_list = temp;
	}
}

static int emcore_get_section_body_size(char *response, char *section, int *body_size)
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
		if (p)  {
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

		}
		else  {
			return FAILURE;
		}
	}
	else  {
		return FAILURE;
	}
	return SUCCESS;
}


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

	EM_DEBUG_LOG(">>> FileName [ %s ] ", result);

	EM_DEBUG_FUNC_END("parsed_filename [%s] ", parsed_filename);
	return parsed_filename;
}

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
				EM_DEBUG_LOG("result_content_type [%s]", result_content_type);
				*output_content_type = EM_SAFE_STRDUP(result_content_type);
				em_trim_left(*output_content_type);

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

	EM_DEBUG_LOG("output_content_type [%s]", output_content_type);

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_get_attribute_value_of_body_part(PARAMETER *input_param, char *atribute_name, char *output_value, int output_buffer_length, int with_rfc2047_text, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_param [%p], atribute_name [%s], output_buffer_length [%d], with_rfc2047_text [%d]", input_param, atribute_name, output_buffer_length, with_rfc2047_text);
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
					decoded_value = emcore_decode_rfc2047_text(temp_param->value, &err);
					if (decoded_value)
						result_value = decoded_value;
					else
						result_value = decoded_value;
				}
				else
					result_value = temp_param->value;
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

	if(err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


/*
 *download body part of imap mail (body-text and attachment)
 */
static int emcore_get_body_part_imap(MAILSTREAM *stream, int account_id, int mail_id, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, body, cnt_info, err_code);

	int err = EMAIL_ERROR_NONE, ret = -1;
	struct attachment_info **ai;
	struct attachment_info *prev_ai = NULL;
	struct attachment_info *next_ai = NULL;
	char *savefile = NULL;
	char *o_data = NULL;
	char filename[MAX_PATH + 1] = { 0, };
	char *decoded_filename = NULL;
	int is_attachment = 0;
	int o_data_len = 0;
	char *filename_temp = NULL;
	char charset_value_buffer[512] = { 0, };
	char content_type_buffer[512] = { 0, };
/*  { is_pb */
   	PART *part = NULL;
	int dec_len;
	int i = 0;
	char *sparep = NULL;
	unsigned short encode = 0;
	int section_plain = 0;
	int section_html = 0;
	int is_pbd = (account_id == 0 && mail_id == 0) ? true  :  false;
/*   } is_pb */

	EM_DEBUG_LOG("Grab Type [ %d ] ", cnt_info->grab_type);

	/* unknown type */
	if (body->type > TYPEOTHER)  {	/* unknown type */
		EM_DEBUG_EXCEPTION("Unknown type.");
		err = EMAIL_ERROR_NOT_SUPPORTED;
		goto FINISH_OFF;
	}

	if (NULL == body->subtype)	{
		EM_DEBUG_LOG("body->subtype is null. "); /*  not exceptional case */
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return FAILURE;
	}

	if (is_pbd) {
		if (!emcore_get_temp_file_name(&o_data, &err) || !o_data)  {
			EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed [%d]", err);
			if (err_code != NULL)
				*err_code = err;
			return FAILURE;
		}


		if (body->subtype[0] == 'P')  {	/*  Sub type is PLAIN_TEX */
			if (cnt_info->text.plain != NULL)
				EM_SAFE_FREE(o_data);
		}

		if (body->type == TYPETEXT && body->subtype &&
			(!body->disposition.type || (body->disposition.type && (body->disposition.type[0] == 'i' || body->disposition.type[0] == 'I')))) {
			if (body->subtype[0] == 'H') 	/*  HTM */
				cnt_info->text.html = o_data;
			else
				cnt_info->text.plain = o_data;

			memset(charset_value_buffer, 0, 512);

			if (emcore_get_attribute_value_of_body_part(body->parameter, "CHARSET", charset_value_buffer, 512, true, &err)) {
				cnt_info->text.plain_charset = EM_SAFE_STRDUP(charset_value_buffer);
				EM_DEBUG_LOG(">>>>> CHARSET [%s] ", filename);
			}
		}
	} /*  is_pbd */

	if ((body->id) && EM_SAFE_STRLEN(body->id) > 1) {	/* if Content-ID or Content-Location exists, it is inline contents */
		EM_DEBUG_LOG("body->id exist");
		size_t len = 0;
		/*  Get actual name of file - fix for inline images to be stored with actual names and not .jpeg */
		if (body->parameter)  {
			PARAMETER *param1 = body->parameter;
			while (param1)  {
				EM_DEBUG_LOG("param1->attribute - %s ", param1->attribute);
				if (!strcasecmp(param1->attribute, "NAME"))  {	/*  attribute is "NAME" */
					char *extcheck = NULL;

					if (param1->value) {
						decoded_filename = emcore_decode_rfc2047_text(param1->value, &err);
						strncpy(filename, decoded_filename, MAX_PATH);
						EM_SAFE_FREE(decoded_filename);
					}
					EM_DEBUG_LOG(">>>>> FILENAME [%s] ", filename);
					extcheck = strchr(filename, '.');

					if (extcheck)
						EM_DEBUG_LOG(">>>> Extension Exist in the Attachment [ %s ] ", extcheck);
					else /*  No extension attached , So add the Extension based on the subtyp */ {
						if (body->subtype) {
							strcat(filename, ".");
							strcat(filename, body->subtype);
							EM_DEBUG_LOG(">>>>> FILENAME Identified the Extension [%s] ", filename);
						}
						else
							EM_DEBUG_EXCEPTION("UnKnown Extesnsion  : _ (");

					}

					break;
				}
				param1 = param1->next;
			}

		}
		else if (body->disposition.type)  {
			PARAMETER *param = body->disposition.parameter;

			while (param) {
				EM_DEBUG_LOG(">>>>> body->disposition.parameter->attribute [ %s ] ", param->attribute);
				EM_DEBUG_LOG(">>>>> body->disposition.parameter->value [ %s ] ", param->value);

				/*  attribute is "filename" */
				if (!strcasecmp(param->attribute, "filename"))  {
					decoded_filename = emcore_decode_rfc2047_text(param->value, &err);
					strncpy(filename, decoded_filename, MAX_PATH);
					EM_SAFE_FREE(decoded_filename);
					EM_DEBUG_LOG(">>>>> FILENAME [%s] ", filename);
					break;
				}

				param = param->next;
			}
		}
		else {
			if ((body->id[0] == '<'))
				SNPRINTF(filename, MAX_PATH, "%s", body->id+1);
			else
				SNPRINTF(filename, MAX_PATH, "%s", body->id);

			len = EM_SAFE_STRLEN(filename);

			if ((len > 1) && (filename[len-1] == '>'))
				filename[len-1] = '\0';
		}
		/* is_attachment = 1; */
		is_attachment = 0;
	}
	else if (body->location) {
		EM_DEBUG_LOG("body->location exist");
		is_attachment = 1;
		decoded_filename = emcore_decode_rfc2047_text(body->location, &err);
		strncpy(filename, decoded_filename, MAX_PATH);
		EM_SAFE_FREE(decoded_filename);
		EM_DEBUG_LOG("body->location [%s]", body->location);
	}
	else if (is_pbd && (strncmp(body->subtype, "RFC822", strlen("RFC822")) == 0) && (cnt_info->grab_type == 0 || (cnt_info->grab_type & GRAB_TYPE_ATTACHMENT))) {
		EM_DEBUG_LOG("Beause subtype is RFC822. This is ttachment");
		is_attachment = 1;

		if (cnt_info->grab_type == 0) {
			if ((body->nested.msg != NULL) && (body->nested.msg->env != NULL) && (body->nested.msg->env->subject != NULL)) {
				decoded_filename = emcore_decode_rfc2047_text(body->nested.msg->env->subject, &err);
				strncpy(filename, decoded_filename, MAX_PATH);
				EM_SAFE_FREE(decoded_filename);
			}
			else
				strncpy(filename, "Unknown <message/rfc822>", MAX_PATH);
		}
		else if (cnt_info->grab_type & GRAB_TYPE_ATTACHMENT) {
			BODY *temp_body = NULL;
			if (body->nested.msg->env->subject != NULL) {
				int i = 0;
				int subject_count = 0;
				if (g_str_has_prefix(body->nested.msg->env->subject, "= ? ") && g_str_has_suffix(body->nested.msg->env->subject, " ? = "))
					strncpy(filename, "unknown", MAX_PATH);
				else {
					for (subject_count = 0; body->nested.msg->env->subject[subject_count] != '\0' ; subject_count++) {
						if (body->nested.msg->env->subject[subject_count] != ':'  &&
							body->nested.msg->env->subject[subject_count] != ';'  &&
							body->nested.msg->env->subject[subject_count] != '*'  &&
							body->nested.msg->env->subject[subject_count] != '?'  &&
							body->nested.msg->env->subject[subject_count] != '\"' &&
							body->nested.msg->env->subject[subject_count] != '<'  &&
							body->nested.msg->env->subject[subject_count] != '>'  &&
							body->nested.msg->env->subject[subject_count] != '|'  &&
							body->nested.msg->env->subject[subject_count] != '/') {
							filename[i] = body->nested.msg->env->subject[subject_count];
							i++;
						}
						else
							continue;
					}
				}
			}
			else
				strncpy(filename, "Unknown", MAX_PATH);

			body = ((MESSAGE *)body->nested.msg)->body;
			part = body->nested.part;

			if ((body->subtype[0] == 'P') || (body->subtype[0] == 'H'))
				temp_body = body;
			else if (part != NULL) {
				temp_body = &(part->body);
				if ((temp_body->subtype[0] == 'P' || temp_body->subtype[0] == 'H') && part->next != NULL) {
					part = part->next;
					temp_body = &(part->body);
				}
			}

			if (temp_body) {
				if (temp_body->subtype[0] == 'P')
					section_plain = 1;
				else if (temp_body->subtype[0] == 'H')
					section_html = 1;

				sparep = temp_body->sparep;
				encode = temp_body->encoding;
			}

		}
	}
	else if (body->disposition.type) /*  if disposition exists, get filename from disposition parameter */ {	/*  "attachment" or "inline" or etc.. */
		EM_DEBUG_LOG("body->disposition.type exist");
		is_attachment = 1;

		if (emcore_get_attribute_value_of_body_part(body->disposition.parameter, "filename", filename, MAX_PATH, true, &err))
			EM_DEBUG_LOG(">>>>> FILENAME [%s] ", filename);

		if (!*filename)  {	/*  If the part has no filename, it may be report ms */
			if ((body->disposition.type[0] == 'i' || body->disposition.type[0] == 'I') && body->parameter && body->parameter->attribute && strcasecmp(body->parameter->attribute, "NAME"))
				is_attachment = 0;
			else if (body->parameter) /* Fix for the MMS attachment File name as unknown */ {
				char *extcheck = NULL;

				if (emcore_get_attribute_value_of_body_part(body->parameter, "NAME", filename, MAX_PATH, true, &err))
					EM_DEBUG_LOG("NAME [%s] ", filename);

				extcheck = strchr(filename, '.');

				if (extcheck)
					EM_DEBUG_LOG(">>>> Extension Exist in the Attachment [ %s ] ", extcheck);
				else  {	/* No extension attached , So add the Extension based on the subtype */
					if (body->subtype) {
						if (EM_SAFE_STRLEN(filename) + EM_SAFE_STRLEN(body->subtype) + 1 < MAX_PATH) {
							strcat(filename, ".");
							strcat(filename, body->subtype);
						}
						EM_DEBUG_LOG(">>>>> FILENAME Identified the Extension [%s] ", filename);
					}
					else
						EM_DEBUG_EXCEPTION("UnKnown Extesnsion  : _ (");
				}

			}
			else
				strncpy(filename, "unknown", MAX_PATH);
		}
		else {
			if ((body->disposition.type[0] == 'i' || body->disposition.type[0] == 'I'))
				is_attachment = 0;
		}
	}

	/* if (!is_pbd) */ {
		EM_DEBUG_LOG("filename [%s]", filename);
		if (*filename)  {
			decoded_filename = emcore_decode_rfc2047_text(filename, &err);
			strncpy(filename, decoded_filename, MAX_PATH);
			EM_SAFE_FREE(decoded_filename);
			filename_temp = em_parse_filename(filename);
			if (filename_temp) {
				strncpy(filename, filename_temp, MAX_PATH);
				EM_DEBUG_LOG("filename [%s]", filename);
			}
		}
	}
	EM_DEBUG_LOG("is_attachment [%d]", is_attachment);

	if (!is_attachment)  {	/*  Text or RFC822 Message */
		EM_DEBUG_LOG("Multipart is not attachment, body->type = %d", body->type);
		if ((cnt_info->grab_type & GRAB_TYPE_TEXT) && (body->type == TYPEMESSAGE || body->type == TYPETEXT || body->type == TYPEIMAGE))  {
			if (is_pbd)
				return SUCCESS;
			else {	/*  fetch body */
				if (!emcore_get_temp_file_name(&o_data, &err))  {
					EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed [%d]", err);
					goto FINISH_OFF;
				}

				if (!imap_mail_write_body_to_file(stream, account_id, mail_id, 0, o_data, msg_uid, body->sparep, body->encoding, &o_data_len, body->subtype, &err))  {
					EM_DEBUG_EXCEPTION("imap_mail_write_body_to_file failed [%d]", err);
					if(err == EMAIL_ERROR_INVALID_STREAM) {
						email_session_t *session = NULL;
						emcore_get_current_session(&session);
						err = session->error;
					}
					goto FINISH_OFF;
				}
			}
		}

		switch (body->type)  {
			case TYPETEXT:
				EM_DEBUG_LOG("TYPETEXT");
				if (body->subtype[0] == 'H')
					cnt_info->text.html = o_data;
				else  {
					cnt_info->text.plain = o_data;
					memset(charset_value_buffer, 0, 512);
					if (emcore_get_attribute_value_of_body_part(body->parameter, "CHARSET", charset_value_buffer, 512, true, &err))
						cnt_info->text.plain_charset = EM_SAFE_STRDUP(charset_value_buffer);
				}
				break;
			case TYPEIMAGE:
			case TYPEAPPLICATION:
			case TYPEAUDIO:
			case TYPEVIDEO:
			/*  Inline Content - suspect of crash on partial body download */
				if (!is_pbd) {
					EM_DEBUG_LOG("TYPEIMAGE or TYPEAPPLICATION  :  inline content");
					ai = &(cnt_info->file);

					dec_len = body->size.bytes;
					if ((cnt_info->grab_type & GRAB_TYPE_ATTACHMENT) &&
						(cnt_info->grab_type & GRAB_TYPE_TEXT))  {	/*  it is 'download all */
						only_body_download = false;
						cnt_info->file_no = 1;
					}

					/*  add attachment info to content info	*/
					if (!(*ai = em_malloc(sizeof(struct attachment_info))))  {
						EM_DEBUG_EXCEPTION("em_malloc failed...");
						if (err_code != NULL)
							*err_code = EMAIL_ERROR_OUT_OF_MEMORY;
						return FAILURE;
					}

					if (((body->id) || (body->location)) && body->type == TYPEIMAGE)
						(*ai)->type = 1;			/*  inline */
					else
						(*ai)->type = 2;			/*  attachment */

					(*ai)->name                 = EM_SAFE_STRDUP(filename);
					(*ai)->size                 = body->size.bytes;
					(*ai)->save                 = o_data;
					(*ai)->content_id           = EM_SAFE_STRDUP(body->id);

					memset(content_type_buffer, 0, 512);
					if ( (err = emcore_get_content_type_from_mail_bodystruct(body, 512, content_type_buffer) ) == EMAIL_ERROR_NONE)
						(*ai)->attachment_mime_type = EM_SAFE_STRDUP(content_type_buffer);

					EM_DEBUG_LOG("name[%s], size[%d], save[%s], content_id[%s], content_type_buffer [%s]", cnt_info->file->name, cnt_info->file->size, cnt_info->file->save, content_type_buffer);
#ifdef __ATTACHMENT_OPTI__
					(*ai)->encoding = body->encoding;
					if (body->sparep)
						(*ai)->section = EM_SAFE_STRDUP(body->sparep);
					EM_DEBUG_LOG(" Encoding - %d  Section No - %s ", (*ai)->encoding, (*ai)->section);
#endif
				}
				break;

			case TYPEMESSAGE:  /*  RFC822 Message */
				EM_DEBUG_EXCEPTION("MESSAGE/RFC822");
				err = EMAIL_ERROR_NOT_SUPPORTED;
				goto FINISH_OFF;

			default:
				EM_DEBUG_EXCEPTION("Unknown type. body->type [%d]", body->type);
				err = EMAIL_ERROR_NOT_SUPPORTED;
				goto FINISH_OFF;
		}

		stream->text.data = NULL;  /*  ? ? ? ? ? ? ? ? */
	}
	else  {	/*  Attachment */
		prev_ai = NULL;
		next_ai = NULL;
		ai = &cnt_info->file;
		EM_DEBUG_LOG(" ai - %p ", (*ai));

		if ((body->id) || (body->location)) {
			/*  For Inline content append to the end */
			for (i = 1; *ai; ai = &(*ai)->next)
				i++;
			}
		else {	/*  For attachment - search till Inline content found and insert before inline */
			for (i = 1; *ai; ai = &(*ai)->next) {
				if ((*ai)->type == 1)  {
					/*  Means inline image */
					EM_DEBUG_LOG("Found Inline Content ");
					next_ai = (*ai);
					break;
				}
				i++;
				prev_ai = (*ai);
			}
		}

		EM_DEBUG_LOG("i - %d next_ai - %p  prev_ai - %p", i, next_ai, prev_ai);

		if ((cnt_info->grab_type & GRAB_TYPE_ATTACHMENT) &&
  		(cnt_info->grab_type & GRAB_TYPE_TEXT))  {	/*  it is 'download all */
			EM_DEBUG_LOG("Download All");
			only_body_download = false;
			cnt_info->file_no = 1;
			i = 1;
		}
		/*  meaningless code */
		dec_len = body->size.bytes;

		if (body->id)
			EM_DEBUG_LOG("BODY ID [ %s ]", body->id);
		else
			EM_DEBUG_LOG("BODY ID IS NULL");

		EM_DEBUG_LOG("i : %d, cnt_info->file_no : %d", i, cnt_info->file_no);

		if (
			((cnt_info->grab_type & GRAB_TYPE_ATTACHMENT) && i == cnt_info->file_no) ||   /*  Is it correct attachment  */
			(((body->id) || (body->location)) && (cnt_info->grab_type & GRAB_TYPE_TEXT))  /*  Is it inline contents  */
		)  {
			/*  fetch attachment */
			EM_DEBUG_LOG("attachment (enc)  :  %s %ld bytes", filename, body->size.bytes);
			EM_DEBUG_LOG(">>>>> ONLY BODY DOWNLOAD [ %d ] ", only_body_download);

			if (only_body_download == false) {
				if (!emcore_get_temp_file_name(&savefile, &err))  {
					EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed [%d]", err);
					goto FINISH_OFF;
				}

				if (!is_pbd) {
					if (!imap_mail_write_body_to_file(stream, account_id, mail_id, cnt_info->file_no, savefile, msg_uid, body->sparep, body->encoding, &dec_len, body->subtype, &err))  {
						EM_DEBUG_EXCEPTION("imap_mail_write_body_to_file failed [%d]", err);
						goto FINISH_OFF;
					}
				}
			}
		}

		EM_DEBUG_LOG("attachment (dec)  :  %s %d bytes", filename, dec_len);

		/*  add attachment info to content inf */
		if (!(*ai = em_malloc(sizeof(struct attachment_info))))  {
			EM_DEBUG_EXCEPTION("em_malloc failed...");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		if ((body->id) || (body->location))
			(*ai)->type = 1;
		else
			(*ai)->type = 2;

		if (is_pbd) {
			if (savefile != NULL) {
				if (section_plain == 1)
					strcat(filename, ".txt");
				if (section_html == 1)
					strcat(filename, ".html");
				section_plain = 0;
				section_html = 0;
			}
		} /*  is_pbd */
		(*ai)->name                 = EM_SAFE_STRDUP(filename);
		(*ai)->size                 = dec_len;
		(*ai)->save                 = savefile;
		(*ai)->content_id           = EM_SAFE_STRDUP(body->id);

		memset(content_type_buffer, 0, 512);
		if ( (err = emcore_get_content_type_from_mail_bodystruct(body, 512, content_type_buffer) ) == EMAIL_ERROR_NONE)
			(*ai)->attachment_mime_type = EM_SAFE_STRDUP(content_type_buffer);
#ifdef __ATTACHMENT_OPTI__
		(*ai)->encoding = body->encoding;
		if (body->sparep)
			(*ai)->section = EM_SAFE_STRDUP(body->sparep);

		EM_DEBUG_LOG(" Encoding - %d  Section No - %s ", (*ai)->encoding, (*ai)->section);
#endif
		if (body->type == TYPEAPPLICATION)  {
			if (!strcasecmp(body->subtype, MIME_SUBTYPE_DRM_OBJECT))
				(*ai)->drm = EMAIL_ATTACHMENT_DRM_OBJECT;
			else if (!strcasecmp(body->subtype, MIME_SUBTYPE_DRM_RIGHTS))
				(*ai)->drm = EMAIL_ATTACHMENT_DRM_RIGHTS;
			else if (!strcasecmp(body->subtype, MIME_SUBTYPE_DRM_DCF))
				(*ai)->drm = EMAIL_ATTACHMENT_DRM_DCF;
		}

		/*  All inline images information are stored at the end of list */
		if ((*ai)->type != 1 && next_ai != NULL) {
		/*  Means next_ai points to the inline attachment info structure */
			if (prev_ai == NULL) {
				/*  First node is inline attachment */
				(*ai)->next = next_ai;
				cnt_info->file = (*ai);
			}
			else {
				prev_ai->next = (*ai);
				(*ai)->next = next_ai;
			}
		}
	}

	ret = 0;
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
/*  get body-part in nested part */

static int emcore_get_allnested_part(MAILSTREAM *stream, int account_id, int mail_id, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, body, cnt_info, err_code);
	PART *part_child = body->nested.part;

	while (part_child)  {
		if (emcore_get_body(stream, account_id, mail_id, msg_uid, &part_child->body, cnt_info, err_code) < 0)
			return FAILURE;

		part_child = part_child->next;
	}

	EM_DEBUG_FUNC_END();
	return SUCCESS;
}

/* get body-part in alternative multiple part */
static int emcore_get_alternative_multi_part(MAILSTREAM *stream, int account_id, int mail_id, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, body, cnt_info, err_code);

	PART *part_child = body->nested.part;

	/* find the best sub part we can show */
	while (part_child)  {
		if (emcore_get_body(stream, account_id, mail_id, msg_uid, &part_child->body, cnt_info, err_code) < 0)
			return FAILURE;

		part_child = part_child->next;
	}
	EM_DEBUG_FUNC_END();
	return SUCCESS;
}

/* get body part in signed multiple part */
static int emcore_get_signed_multi_part(MAILSTREAM *stream, int account_id, int mail_id, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, body, cnt_info, err_code);

        PART *part_child = body->nested.part;

        /* find the best sub part we can show */
        while (part_child)  {
                if (emcore_get_body(stream, account_id, mail_id, msg_uid, &part_child->body, cnt_info, err_code) < 0)
			return FAILURE;

                part_child = part_child->next;
        }

	EM_DEBUG_FUNC_END();
	return SUCCESS;
}

/* get body part in encrypted multiple part */
static int emcore_get_encrypted_multi_part(MAILSTREAM *stream, int account_id, int mail_id, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, body, cnt_info, err_code);
	EM_DEBUG_FUNC_END();
	return SUCCESS;
}

/* get body part in multiple part */
static int emcore_get_multi_part(MAILSTREAM *stream, int account_id, int mail_id, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, body, cnt_info, err_code);

	if (!body) {
		EM_DEBUG_EXCEPTION("Invalid Parameter.");
		if (err_code)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return FAILURE;
	}

	switch (body->subtype[0])  {
		case 'A': 		/*  ALTERNATIVE */
			EM_DEBUG_LOG("body->subtype[0] = ALTERNATIVE");
			return emcore_get_alternative_multi_part(stream, account_id, mail_id, msg_uid, body, cnt_info, err_code);

		case 'S': 		/*  SIGNED */
			EM_DEBUG_LOG("body->subtype[0] = SIGNED");
			return emcore_get_signed_multi_part(stream, account_id, mail_id, msg_uid, body, cnt_info, err_code);

		case 'E': 		/*  ENCRYPTED */
			EM_DEBUG_LOG("body->subtype[0] = ENCRYPTED");
			return emcore_get_encrypted_multi_part(stream, account_id, mail_id, msg_uid, body, cnt_info, err_code);

		default: 		/*  process all unknown as MIXED (according to the RFC 2047) */
			EM_DEBUG_LOG("body->subtype[0] = [%c].", body->subtype[0]);
			return emcore_get_allnested_part(stream, account_id, mail_id, msg_uid, body, cnt_info, err_code);
	}
	/* Delete the dead code */
}

/*  get body data by body structure */
/*  if POP3, ignored */
INTERNAL_FUNC int emcore_get_body(MAILSTREAM *stream, int account_id, int mail_id, int msg_uid, BODY *body, struct _m_content_info *cnt_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msg_uid[%d], body[%p], cnt_info[%p], err_code[%p]", stream, msg_uid, body, cnt_info, err_code);

	if (!stream || !body || !cnt_info)  {
		EM_DEBUG_EXCEPTION("Invalid parameter");

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return FAILURE;
	}

	EM_DEBUG_LOG("body->type [%d]", body->type);

	switch (body->type)  {
		case TYPEMULTIPART:
			return emcore_get_multi_part(stream, account_id, mail_id, msg_uid, body, cnt_info, err_code);

		case TYPEMESSAGE:  /*  not support */
			if (strcasecmp(body->subtype, "RFC822") == 0)
				return emcore_get_body_part_imap(stream, account_id, mail_id, msg_uid, body, cnt_info, err_code);
			break;

		case TYPETEXT:
		case TYPEAPPLICATION:
		case TYPEAUDIO:
		case TYPEIMAGE:
		case TYPEVIDEO:
		case TYPEMODEL:
		case TYPEOTHER:
			/*  exactly, get a pure body part (text and attachment */
			return emcore_get_body_part_imap(stream, account_id, mail_id, msg_uid, body, cnt_info, err_code);

		default:
			break;
	}
	EM_DEBUG_FUNC_END();
	return SUCCESS;
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

int emcore_set_fetch_part_section(BODY *body, char *section_pfx, int section_subno, int enable_inline_list, int *total_mail_size, int *err_code);

/* set body section to be fetched */
INTERNAL_FUNC int emcore_set_fetch_body_section(BODY *body, int enable_inline_list, int *total_mail_size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("body[%p], err_code[%p]", body, err_code);

	if (!body)  {
		EM_DEBUG_EXCEPTION("body [%p]", body);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return FAILURE;
	}

//	body->id = cpystr("1"); /*  top level body */
	EM_DEBUG_LOG("body->id : [%s]", body->id);

	g_inline_count = 0;
	EM_SAFE_FREE(g_inline_list);
	emcore_set_fetch_part_section(body, (char *)NULL, 0, enable_inline_list, total_mail_size, err_code);

	if (body && body->id)
		EM_DEBUG_LOG(">>>>> FILE NAME [%s] ", body->id);
	else
		EM_DEBUG_LOG(">>>>> BODY NULL ");

	EM_DEBUG_FUNC_END();
	return SUCCESS;
}

/* set part section of body to be fetched */
int emcore_set_fetch_part_section(BODY *body, char *section_pfx, int section_subno, int enable_inline_list, int *total_mail_size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("body[%p], section_pfx[%s], section_subno[%d], err_code[%p]", body, section_pfx, section_subno, err_code);

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
			emcore_set_fetch_part_section(&part->body, section, section_subno++, enable_inline_list, total_mail_size, err_code);
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

		/*  if ((total_mail_size != NULL) && !(body->disposition.type && (body->disposition.type[0] == 'a' || body->disposition.type[0] == 'A')) */
		if (total_mail_size != NULL) {
			*total_mail_size = *total_mail_size + (int)body->size.bytes;
			EM_DEBUG_LOG("body->size.bytes [%d]", body->size.bytes);
		}

		/* encapsulated message ? */
		if ((body->type == TYPEMESSAGE) && !strcasecmp(body->subtype, "RFC822") && (body = ((MESSAGE *)body->nested.msg)->body))  {
			if (body->type == TYPEMULTIPART) {
				section[0] = '\0';
				emcore_set_fetch_part_section(body, section, section_subno-1, enable_inline_list, total_mail_size, err_code);
			}
			else  {		/*  build encapsulation prefi */
				SNPRINTF(section, sizeof(section), "%s%d.", section_pfx, section_subno);
				emcore_set_fetch_part_section(body, section, 0, enable_inline_list, total_mail_size, err_code);
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


static void parse_file_path_to_filename(char *src_string, char **out_string)
{
	char *token = NULL;
	char *filepath = NULL;
	char *str = NULL;
	char *prev1 = NULL;
	char *prev2 = NULL;

	filepath = EM_SAFE_STRDUP(src_string);
	token = strtok_r(filepath, "/", &str);

	do {
		prev2 = prev1;
		prev1 = token;
	} while ((token = strtok_r(NULL , "/", &str)));

	*out_string = EM_SAFE_STRDUP(prev1);
	EM_SAFE_FREE(filepath);
}

static char *emcore_decode_rfc2047_word(char *encoded_word, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("encoded_word[%p], err_code[%p]", encoded_word, err_code);

	int err = EMAIL_ERROR_NONE;
	int base64_encoded = false, length = 0;
	SIZEDTEXT src = { NULL, 0 };
	SIZEDTEXT dst = { NULL, 0 };
	gchar *charset = NULL, *encoded_text = NULL;
	char *decoded_text = NULL, *decoded_word = NULL;
	char *current = NULL, *start = NULL, *end = NULL;
	char *buffer = (char*) em_malloc(EM_SAFE_STRLEN(encoded_word) * 2 + 1);

	if (buffer == NULL) {
		EM_DEBUG_EXCEPTION("Memory allocation fail");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	strcpy(buffer, "");

	/*  encoding format : =?charset?encoding?encoded-text ?=  */
	/*  charset : UTF-8, EUC-KR, ... */
	/*  encoding : b/B (BASE64), q/Q (QUOTED-PRINTABLE) */
	current = encoded_word;

	while (*current != NULL_CHAR) {
		/* search next */
		start = strstr(current, "=?");		/*  start of encoding */
		end = strstr(current, "?="); 		/*  end of encoding */

#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG("current[%p][%s], start[%p][%s], end[%p][%s]", current, current, start, start, end, end);
#endif
		if (start != NULL) {
			if (current != start) {	/*  copy the string between current and start to buffer */
				strncat(buffer, current, start - current);
				current = start;
#ifdef FEATURE_CORE_DEBUG
				EM_DEBUG_LOG("1 - Buffer[%s]", buffer);
#endif
			}

			if (end) {	/*  decode text between start and end */
				char *p = strstr(start, "?b?");

				if (p || (p = strstr(start, "?B?")))		/*  BASE64 */
					base64_encoded = true;
				else {
					p = strstr(start, "?q?");

					if (p || (p = strstr(start, "?Q?")))		/*  QUOTED-PRINTABLE */
						base64_encoded = false;
					else {
						EM_DEBUG_EXCEPTION("unknown encoding found...");

						err = EMAIL_ERROR_UNKNOWN;
						goto FINISH_OFF;
					}
				}

				if (base64_encoded) {	/*  BASE64 */
					charset = g_strndup(start + 2, p - (start + 2));
					encoded_text = g_strndup(p + 3, end - (p + 3));
				}
				else {	/*  QUOTED-PRINTABLE */
					charset = g_strndup(start + 2, p - (start + 2));
					if (*(p+3) == '=') {	/*  encoded text might start with '='. ex) '?Q?=E0' */
						end = strstr(p+3, "?="); 		/*  find new end flag */
						if (end) {
							encoded_text = g_strndup(p + 3, end - (p + 3));
						}
						else {	/*  end flag is not found */
							EM_DEBUG_EXCEPTION("emcore_decode_rfc2047_word decoding error : '?=' is not found...");

							err = EMAIL_ERROR_UNKNOWN;
							goto FINISH_OFF;
						}
					}
					else {
						encoded_text = g_strndup(p + 3, end - (p + 3));
					}
				}

#ifdef FEATURE_CORE_DEBUG
				EM_DEBUG_LOG("\t >>>>>>>>>>>>>>> CHARSET[%s]", charset);
				EM_DEBUG_LOG("\t >>>>>>>>>>>>>>> ENCODED_TEXT[%s]", encoded_text);
#endif

				unsigned long len = 0;
				if (encoded_text != NULL) {
				if (base64_encoded == true) {
					if (!(decoded_text = (char *)rfc822_base64((unsigned char *)encoded_text, EM_SAFE_STRLEN(encoded_text), &len))) {
						EM_DEBUG_EXCEPTION("rfc822_base64 falied...");
						goto FINISH_OFF;
					}
				}
				else  {
					g_strdelimit(encoded_text, "_", ' ');

					if (!(decoded_text = (char *)rfc822_qprint((unsigned char *)encoded_text, EM_SAFE_STRLEN(encoded_text), &len))) {
						EM_DEBUG_EXCEPTION("rfc822_base64 falied...");
						goto FINISH_OFF;
					}
				}

				src.data = (unsigned char *)decoded_text;
				src.size = EM_SAFE_STRLEN(decoded_text);

				if (!utf8_text(&src, charset, &dst, 0))  {
					EM_DEBUG_EXCEPTION("utf8_text falied...");
					if (src.data)
						strncat(buffer, (char *)src.data, src.size); /* Eventhough failed to decode, downloading should go on. Kyuho Jo */
				} else {
					if (dst.data)
						strncat(buffer, (char *)dst.data, dst.size);
				}
#ifdef FEATURE_CORE_DEBUG
				EM_DEBUG_LOG("2 - Buffer[%s]", buffer);
#endif

				/*  free all of the temp variables */
				if (dst.data != NULL && dst.data != src.data)
					EM_SAFE_FREE(dst.data);

				EM_SAFE_FREE(decoded_text);

					g_free(encoded_text);
					encoded_text = NULL;
				}
				if (charset != NULL) {
					g_free(charset);
					charset = NULL;
				}

				current = end + 2;	/*  skip '?=' */
			}
			else {
				/*  unencoded text	*/
				length = EM_SAFE_STRLEN(start);
				strncat(buffer, start, length);
				current = start + length;
#ifdef FEATURE_CORE_DEBUG
				EM_DEBUG_LOG("3 - Buffer[%s]", buffer);
#endif
			}
		}
		else {
			/*  unencoded text	*/
			length = EM_SAFE_STRLEN(current);
			strncat(buffer, current, length);
			current = current + length;
#ifdef FEATURE_CORE_DEBUG
			EM_DEBUG_LOG("4 - Buffer[%s]", buffer);
#endif
		}
	}

	decoded_word = EM_SAFE_STRDUP(buffer);

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG(">>>>>>>>>>>>>>> DECODED_WORD[%s]", decoded_word);
#endif

FINISH_OFF:
	if (dst.data != NULL && dst.data != src.data)
		EM_SAFE_FREE(dst.data);
	EM_SAFE_FREE(decoded_text);
	EM_SAFE_FREE(buffer);

	if (encoded_text != NULL)
		g_free(encoded_text);
	if (charset != NULL)
		g_free(charset);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return decoded_word;
}

INTERNAL_FUNC char *emcore_decode_rfc2047_text(char *rfc2047_text, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("rfc2047_text[%s], err_code[%p]", rfc2047_text, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (!rfc2047_text)  {
		EM_DEBUG_EXCEPTION("rfc2047_text[%p]", rfc2047_text);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return NULL;
	}

	char *text = NULL;

	gchar **encoded_words = g_strsplit_set(rfc2047_text, " \t\r\n", -1);
	gchar **decoded_words = g_new0(char *, g_strv_length(encoded_words) + 1);

	/* EM_DEBUG_LOG("g_strv_length(encoded_words) [%d]", g_strv_length(encoded_words)); */

	if (encoded_words != NULL)  {
		int i = 0;

		while (encoded_words[i] != NULL)  {
			if (!(decoded_words[i] = emcore_decode_rfc2047_word(encoded_words[i], &err)))  {
				EM_DEBUG_EXCEPTION("emcore_decode_rfc2047_word falied [%d]", err);
				goto FINISH_OFF;
			}

			i++;
		}
		text = g_strjoinv(" ", decoded_words);
	}
	else
		text = EM_SAFE_STRDUP(rfc2047_text);

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG(">>>>>>>>>>>>>>>>> TEXT[%s]", text);
#endif	/*  FEATURE_CORE_DEBUG */

	ret = true;

FINISH_OFF:
	g_strfreev(decoded_words);
	g_strfreev(encoded_words);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return text;
}

INTERNAL_FUNC int emcore_make_mail_data_from_mime_data(struct _m_mesg *mmsg, struct _m_content_info *cnt_info, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data, int *output_attachment_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int local_attachment_count = 0;
	int local_inline_content_count = 0;
	int attachment_num = 0;
	int i = 0;
	int eml_mail_id = 0;
	char buf[512];
	char html_body[MAX_PATH] = {0, };
	struct tm temp_time_info;
	struct timeval tv;
	struct attachment_info *ai = NULL;
	char *encoded_subject = NULL;
	char *content_type = NULL;
	email_attachment_data_t *attachment = NULL;
	email_mail_data_t *p_mail_data = NULL;
	MESSAGECACHE mail_cache_element = {0, };

	if (!mmsg || !cnt_info || !output_mail_data || !output_attachment_data) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	p_mail_data = (email_mail_data_t *)em_malloc(sizeof(email_mail_data_t));
	if (p_mail_data == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(&mail_cache_element, 0x00, sizeof(MESSAGECACHE));
	memset((void *)&temp_time_info, 0, sizeof(struct tm));

	/* Create rand mail id of eml */
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);
	eml_mail_id = rand();

	p_mail_data->mail_id = eml_mail_id;
	p_mail_data->account_id = EML_FOLDER;

	if (mmsg->header)
		content_type = emcore_mime_get_header_value(mmsg->header->part_header, CONTENT_TYPE, &err);

	if (strcasestr(content_type, "PKCS7-MIME"))
		p_mail_data->smime_type = EMAIL_SMIME_ENCRYPTED;

	if (mmsg->rfc822header->date)
		mail_parse_date(&mail_cache_element, (unsigned char *)mmsg->rfc822header->date);

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

	encoded_subject = emcore_decode_rfc2047_text(mmsg->rfc822header->subject, NULL);

	p_mail_data->date_time                   = timegm(&temp_time_info);
	p_mail_data->full_address_return         = EM_SAFE_STRDUP(mmsg->rfc822header->return_path);
	p_mail_data->email_address_recipient     = EM_SAFE_STRDUP(mmsg->rfc822header->received);
	p_mail_data->full_address_from           = EM_SAFE_STRDUP(mmsg->rfc822header->from);
	p_mail_data->subject                     = EM_SAFE_STRDUP(encoded_subject);
	p_mail_data->email_address_sender        = EM_SAFE_STRDUP(mmsg->rfc822header->sender);
	p_mail_data->full_address_to             = EM_SAFE_STRDUP(mmsg->rfc822header->to);
	p_mail_data->full_address_cc             = EM_SAFE_STRDUP(mmsg->rfc822header->cc);
	p_mail_data->full_address_bcc            = EM_SAFE_STRDUP(mmsg->rfc822header->bcc);
	p_mail_data->full_address_reply          = EM_SAFE_STRDUP(mmsg->rfc822header->reply_to);
	p_mail_data->body_download_status        = EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;

	EM_DEBUG_LOG("cnt_info->text.plain [%s], cnt_info->text.html [%s]", cnt_info->text.plain, cnt_info->text.html);

	if (cnt_info->text.plain) {
		EM_DEBUG_LOG("cnt_info->text.plain [%s]", cnt_info->text.plain);
		if (!emstorage_create_dir(EML_FOLDER, eml_mail_id, 0, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_get_save_name(EML_FOLDER, eml_mail_id, 0, cnt_info->text.plain_charset ? cnt_info->text.plain_charset : UNKNOWN_CHARSET_PLAIN_TEXT_FILE, buf, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(cnt_info->text.plain, buf, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}

		p_mail_data->file_path_plain = EM_SAFE_STRDUP(buf);
		EM_DEBUG_LOG("mail_data->file_path_plain [%s]", p_mail_data->file_path_plain);
	}

	if (cnt_info->text.html)  {
		if (!emstorage_create_dir(EML_FOLDER, eml_mail_id, 0, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (cnt_info->text.html_charset != NULL) {
			SNPRINTF(html_body, MAX_PATH, "%s%s", cnt_info->text.html_charset, HTML_EXTENSION_STRING);
		} else {
			strcpy(html_body, UNKNOWN_CHARSET_HTML_TEXT_FILE);
		}

		if (!emstorage_get_save_name(EML_FOLDER, eml_mail_id, 0, html_body, buf, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(cnt_info->text.html, buf, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}
		p_mail_data->file_path_html = EM_SAFE_STRDUP(buf);
	}


	for (ai = cnt_info->file; ai; ai = ai->next, attachment_num++) {}
	EM_DEBUG_LOG("attachment_num : [%d]", attachment_num);

	if (attachment_num > 0) {
		attachment = (email_attachment_data_t *)em_malloc(sizeof(email_attachment_data_t) * attachment_num);
		if (attachment == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		for (ai = cnt_info->file; ai; ai = ai->next, i++) {
			attachment[i].attachment_id          = 0;
			attachment[i].attachment_size        = ai->size;
			attachment[i].attachment_name        = EM_SAFE_STRDUP(ai->name);
			attachment[i].drm_status             = ai->drm;
			attachment[i].save_status            = 0;
			attachment[i].inline_content_status  = ai->type == 1;
			attachment[i].attachment_mime_type   = ai->attachment_mime_type;
#ifdef __ATTACHMENT_OPTI__
			attachment[i].encoding               = ai->encoding;
			attachment[i].section                = ai->section;
#endif
			EM_DEBUG_LOG("attachment[%d].attachment_id[%d]", i, attachment[i].attachment_id);
			EM_DEBUG_LOG("attachment[%d].attachment_size[%d]", i, attachment[i].attachment_size);
			EM_DEBUG_LOG("attachment[%d].attachment_name[%s]", i, attachment[i].attachment_name);
			EM_DEBUG_LOG("attachment[%d].drm_status[%d]", i, attachment[i].drm_status);
			EM_DEBUG_LOG("attachment[%d].inline_content_status[%d]", i, attachment[i].inline_content_status);

			if (ai->type == 1)
				local_inline_content_count ++;
			local_attachment_count++;

			if (ai->save)  {
				attachment[i].save_status = 1;
				if (ai->type == 1)  {		/*  it is inline content */
					if (!emstorage_create_dir(EML_FOLDER, eml_mail_id, 0, &err))  {
						EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
						goto FINISH_OFF;
					}
					if (!emstorage_get_save_name(EML_FOLDER, eml_mail_id, 0, attachment[i].attachment_name, buf, &err))  {
						EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
						goto FINISH_OFF;
					}
				}
				else  {
					if (!emstorage_create_dir(EML_FOLDER, eml_mail_id, i, &err)) {
						EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
						goto FINISH_OFF;
					}

					if (!emstorage_get_save_name(EML_FOLDER, eml_mail_id, i, attachment[i].attachment_name, buf, &err))  {
						EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
						goto FINISH_OFF;
					}
				}

				if (!emstorage_move_file(ai->save, buf, false, &err))  {
					EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);

					/*  delete all created files. */
					if (!emstorage_get_save_name(EML_FOLDER, eml_mail_id, 0, NULL, buf, NULL)) {
						EM_DEBUG_EXCEPTION("emstorage_get_save_name failed...");
						/* goto FINISH_OFF; */
					}

					if (!emstorage_delete_dir(buf, NULL)) {
						EM_DEBUG_EXCEPTION("emstorage_delete_dir failed...");
						/* goto FINISH_OFF; */
					}


					goto FINISH_OFF;
				}

				attachment[i].attachment_path = EM_SAFE_STRDUP(buf);

			}

			EM_DEBUG_LOG("attachment[%d].attachment_path[%s]", i, attachment[i].attachment_path);
		}
	}
	EM_DEBUG_LOG("Check #1");

	p_mail_data->attachment_count = local_attachment_count;
	p_mail_data->inline_content_count = local_inline_content_count;

	eml_data_count += 1;
	ret = true;

FINISH_OFF:

	if (ret) {
		if (output_mail_data)
			*output_mail_data = p_mail_data;

		if (output_attachment_data)
			*output_attachment_data = attachment;

		if (output_attachment_count)
			*output_attachment_count = local_attachment_count;
	} else {
		if (p_mail_data) {
			emcore_free_mail_data(p_mail_data);
			EM_SAFE_FREE(p_mail_data);
		}

		if (attachment)
			emcore_free_attachment_data(&attachment, attachment_num, NULL);
	}

	EM_SAFE_FREE(encoded_subject);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret : [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emcore_parse_mime_file_to_mail(char *eml_file_path, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data, int *output_attachment_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("eml_file_path : [%s], output_mail_data : [%p]", eml_file_path, output_mail_data);

	int err = EMAIL_ERROR_NONE;
	int ret = false;
	int is_file = 1;
	FILE *eml_fp = NULL;
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

	eml_fp = fopen(eml_file_path, "r");
	if (eml_fp == NULL) {
		EM_DEBUG_EXCEPTION("file open failed");
		err = EMAIL_ERROR_ON_PARSING;
		goto FINISH_OFF;
	}

	if (!emcore_mime_parse_header(eml_fp, is_file, &mmsg->rfc822header, &mmsg->header, &err)) {
		EM_DEBUG_EXCEPTION("emcore_mime_parse_header failed : [%d]", err);
		err = EMAIL_ERROR_INVALID_DATA;
		goto FINISH_OFF;
	}

	if (!mmsg->header->part_header) {
		EM_DEBUG_EXCEPTION("Invalid eml format");
		err = EMAIL_ERROR_INVALID_DATA;
		goto FINISH_OFF;
	}

	if (!emcore_mime_parse_body(eml_fp, is_file, mmsg, cnt_info, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emcore_mime_parse_body failed : [%d]", err);
		err = EMAIL_ERROR_INVALID_DATA;
		goto FINISH_OFF;
	}

	if (!emcore_make_mail_data_from_mime_data(mmsg, cnt_info, output_mail_data, output_attachment_data, output_attachment_count, &err)) {
		EM_DEBUG_EXCEPTION("emcore_make_mail_tbl_data_from_mime failed : [%d]", err);
		goto FINISH_OFF;

	}

	ret = true;

FINISH_OFF:

	if (eml_fp)
		fclose(eml_fp);

	if (mmsg)
		emcore_mime_free_mime(mmsg);

	if (cnt_info)
		emcore_free_content_info(cnt_info);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("err : %d", err);
	return ret;
}

INTERNAL_FUNC int emcore_delete_parsed_data(email_mail_data_t *input_mail_data, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data : [%p]", input_mail_data);
	int err = EMAIL_ERROR_NONE;
	int ret = false;
	char buf[512];

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

	eml_data_count = eml_data_count - 1;

	if (eml_data_count == 0) {
		SNPRINTF(buf, sizeof(buf), "%s%s%d", MAILHOME, DIR_SEPERATOR, input_mail_data->account_id);
	} else {
		SNPRINTF(buf, sizeof(buf), "%s%s%d%s%d", MAILHOME, DIR_SEPERATOR, input_mail_data->account_id, DIR_SEPERATOR, input_mail_data->mail_id);
	}

	EM_DEBUG_LOG("Directory : [%s]", buf);

	if (!emstorage_delete_dir(buf, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_delete_dir failed");
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (err_code)
		*err_code = err;

	return ret;
}

INTERNAL_FUNC int emcore_get_mime_entity(char *mime_path, char **output_path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mime_path : [%s], output_path : [%p]", mime_path, *output_path);
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int is_file = 1;
	int search = 0;
	long start_mime_entity = 0;
	long end_mime_entity = 0;
	char buf[MIME_LINE_LEN] = {0x00, };
	char *mime_entity_path = NULL;
	char *content_type = NULL;
	char boundary[BOUNDARY_LEN] = {0x00,};
	char *boundary_string = NULL;
	char *p_mime_entity = NULL;
	FILE *fp_read = NULL;
	FILE *fp_write = NULL;
	struct _m_mesg *mmsg = NULL;

	if (!mime_path) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* Get the header info */
	mmsg = (struct _m_mesg *)em_malloc(sizeof(struct _m_mesg));
	if (mmsg == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	fp_read = fopen(mime_path, "r");
	if (fp_read == NULL) {
		EM_DEBUG_EXCEPTION("File open(read) is failed : filename [%s]", mime_path);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if (!emcore_mime_parse_header(fp_read, is_file, &mmsg->rfc822header, &mmsg->header, &err)) {
		EM_DEBUG_EXCEPTION("emcore_mime_parse_header failed : [%d]", err);
		goto FINISH_OFF;
	}

	/* Parsing the mime header */
	content_type = emcore_mime_get_header_value(mmsg->header->part_header, CONTENT_TYPE, NULL);
	EM_DEBUG_LOG("Content_type : [%s]", content_type);
	if (strcasestr(content_type, "signed") == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter : No signed mail");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* Create mime_entity file */
	if (!emcore_get_temp_file_name(&mime_entity_path, &err)) {
		EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed[%d]", err);
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("mime_entity_path = %s", mime_entity_path);

	fp_write = fopen(mime_entity_path, "w");
	if (fp_write == NULL) {
		EM_DEBUG_EXCEPTION("File open(write) is failed : filename [%s]", mime_entity_path);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	boundary_string = emcore_mime_get_header_value(mmsg->header->part_header, CONTENT_BOUNDARY, NULL);
	SNPRINTF(boundary, BOUNDARY_LEN, "--%s%s", boundary_string, CRLF_STRING);

	while (TRUE) {
		if (!emcore_get_line_from_file((void *)fp_read, buf, MIME_LINE_LEN, &err)) {
			EM_DEBUG_EXCEPTION("emcore_mime_get_line_from_file failed [%d]", err);
			if (err == EMAIL_ERROR_NO_MORE_DATA) {
				EM_DEBUG_LOG("this mail is partial body");
				end_mime_entity = ftell(fp_read);
				if ( end_mime_entity < 0 ) {
					EM_DEBUG_EXCEPTION("ftell failed [%s]", EM_STRERROR(errno));
					goto FINISH_OFF;
				}
				break;
			}
			goto FINISH_OFF;
		}

		if (!strcmp(buf, boundary)) {
			search++;
			if (search == 1) {
				start_mime_entity = ftell(fp_read);
				if( start_mime_entity < 0 ) { /*prevent 24473*/
					EM_DEBUG_EXCEPTION("ftell failed [%s]", EM_STRERROR(errno));
					goto FINISH_OFF;
				}
			} else if (search == 2) {
				end_mime_entity = ftell(fp_read);
				if( end_mime_entity < 0 ) { /*prevent 24473*/
					EM_DEBUG_EXCEPTION("ftell failed [%s]", EM_STRERROR(errno));
					goto FINISH_OFF;
				}
				break;
			}
		}
	}

	p_mime_entity = em_malloc(end_mime_entity - start_mime_entity);
	if (p_mime_entity == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (fseek(fp_read, start_mime_entity, SEEK_SET) < 0) {
		EM_DEBUG_EXCEPTION("fseek failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* +2 : CRLF line */
	if ((ret = fread(p_mime_entity, 1, end_mime_entity - (start_mime_entity + EM_SAFE_STRLEN(boundary) + 2), fp_read)) < 0) {
		EM_DEBUG_EXCEPTION("fread failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	fprintf(fp_write, "%s", p_mime_entity);

	ret = true;

FINISH_OFF:

	if (fp_read)
		fclose(fp_read);

	if (fp_write)
		fclose(fp_write);

	if (mmsg)
		emcore_mime_free_mime(mmsg);

	if (output_path)
		*output_path = mime_entity_path;

	if (err_code)
		*err_code = err;

	EM_SAFE_FREE(p_mime_entity);

	EM_DEBUG_FUNC_END();
	return ret;
}

int emcore_decode_body_text_from_file(FILE *stream, char *boundary_str, int encoding, int mode, int is_text, int fd, char **holder, int *end_of_parsing, int *size)
{
	EM_DEBUG_FUNC_BEGIN();
	int error = EMAIL_ERROR_NONE;
	int p_size = 0;
	int partial_body = 0;
	int dec_len = 0;
	long start_location = 0;
	long end_location = 0;
	char boundary[BOUNDARY_LEN] = {0x00, };
	char boundary_end[BOUNDARY_LEN] = {0x00, };
	char buf[MIME_LINE_LEN] = {0x00, };
	char *body = NULL;
	int modified_body_size = 0;
	char *modified_body = NULL;

	if (boundary_str) {
		/*  if there boundary, this content is from current line to ending boundary */
		memset(boundary, 0x00, BOUNDARY_LEN);
		memset(boundary_end, 0x00, BOUNDARY_LEN);

		SNPRINTF(boundary, BOUNDARY_LEN, "--%s%s", boundary_str, CRLF_STRING);
		SNPRINTF(boundary_end, BOUNDARY_LEN, "--%s%s", boundary_str, "--\r\n");
	}

	start_location = ftell(stream);
	if(start_location < 0 ) { /*prevent 35555*/
		error = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("ftell failed : %s", EM_STRERROR(errno));
		goto FINISH_OFF;
	}

	while (TRUE) {
		if (!emcore_get_line_from_file(stream, buf, MIME_LINE_LEN, &error)) {
			if (error != EMAIL_ERROR_NO_MORE_DATA) {
				EM_DEBUG_EXCEPTION("emcore_get_line_from_file failed");
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}

			partial_body = 1;
			*end_of_parsing = 1;
			break;
		}

		if (boundary_str)  {
			if (!strcmp(buf, boundary))  {	/*  the other part started. the parsing of other part will be started */
				*end_of_parsing = 0;
				break;
			}
			else if (!strcmp(buf, boundary_end))  {	/*  if ending boundary, the parsing of other multipart will be started */
				*end_of_parsing = 1;
				break;
			}
		}
	}

	end_location = ftell(stream);
	if(end_location < 0 ) { /*prevent 35555*/
		error = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("ftell failed : %s", EM_STRERROR(errno));
		goto FINISH_OFF;
	}

	if (partial_body)
		p_size = end_location - start_location;
	else
		p_size = end_location - start_location - EM_SAFE_STRLEN(buf);

	body = em_malloc(p_size + 1);
	if (body == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	fseek(stream, start_location, SEEK_SET);
	if (fread(body, sizeof(char), p_size, stream) != p_size) {
		EM_DEBUG_EXCEPTION("fread failed");
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if (mode > SAVE_TYPE_SIZE) {	/*  decode content */
		emcore_decode_body_text(body, p_size, encoding, &dec_len, &error);

		if (is_text) {
			modified_body = em_replace_all_string(body, "cid:", "");
			modified_body_size = EM_SAFE_STRLEN(modified_body); /*prevent 35585 */
		}

		if (modified_body == NULL) {
			modified_body = em_malloc(dec_len + 1);
			if (modified_body == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed");
				error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			memcpy(modified_body, body, dec_len);  /*prevent 35585 */
			modified_body_size = dec_len;
		}

		if (mode == SAVE_TYPE_BUFFER)  {   /*  save content to buffer */
			*holder = EM_SAFE_STRDUP(modified_body);
		} else if (mode == SAVE_TYPE_FILE)  {	/*  save content to file */
			if (write(fd, modified_body, modified_body_size) != modified_body_size)  {
				EM_DEBUG_EXCEPTION("write failed");
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		}
	}

	fseek((FILE *)stream, end_location, SEEK_SET);

FINISH_OFF:

	if (size)
		*size = modified_body_size;

	EM_SAFE_FREE(modified_body);
	EM_SAFE_FREE(body); /*prevent 35585 */
	return error;
}

int emcore_decode_body_text_from_sock(void *stream, char *boundary_str, int encoding, int mode, int is_text, int fd, char **holder, int *end_of_parsing, int *size)
{
	EM_DEBUG_FUNC_BEGIN();
	int error = EMAIL_ERROR_NONE;
	int sz = 0;
	int dec_len = 0;
	char boundary[BOUNDARY_LEN] = {0x00, };
	char boundary_end[BOUNDARY_LEN] = {0x00, };
	char buf[MIME_LINE_LEN] = {0x00, };
	char *result_buffer = NULL;
	int result_buffer_size = 0;
	char *pTemp = NULL;

	if (boundary_str) {
		/*  if there boundary, this content is from current line to ending boundary */
		memset(boundary, 0x00, BOUNDARY_LEN);
		memset(boundary_end, 0x00, BOUNDARY_LEN);

		SNPRINTF(boundary, BOUNDARY_LEN, "--%s%s", boundary_str, CRLF_STRING);
		SNPRINTF(boundary_end, BOUNDARY_LEN, "--%s%s", boundary_str, "--\r\n");
	}

	while (TRUE) {
		if (!emcore_check_thread_status())  {
			EM_DEBUG_EXCEPTION("EMAIL_ERROR_CANCELLED");
			error = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

		if (!emcore_mime_get_line_from_sock(stream, buf, MIME_LINE_LEN, &error)) {
			if (error != EMAIL_ERROR_NO_MORE_DATA) {
				EM_DEBUG_EXCEPTION("emcore_mime_get_line_from_sock failed");
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}

			EM_DEBUG_LOG("This mail is partial body");

			*end_of_parsing = 1;

			error = EMAIL_ERROR_NONE;

			break;
		}

		if (boundary_str)  {
			if (!strcmp(buf, boundary))  {	/*  the other part started. the parsing of other part will be started */
				*end_of_parsing = 0;
				break;
			}
			else if (!strcmp(buf, boundary_end))  {	/*  if ending boundary, the parsing of other multipart will be started */
				*end_of_parsing = 1;
				break;
			}
		}

		/*  parsing string started by '.' in POP3 */
		if ((buf[0] == '.' && buf[1] == '.') && (encoding == ENCQUOTEDPRINTABLE || encoding == ENC7BIT))  {
				strncpy(buf, buf+1, MIME_LINE_LEN-1);
				buf[EM_SAFE_STRLEN(buf)] = NULL_CHAR;
		}

		if (encoding == ENCBASE64)  {
				if (EM_SAFE_STRLEN(buf) >= 2)
					buf[EM_SAFE_STRLEN(buf)-2] = NULL_CHAR;
		} else if (encoding == ENCQUOTEDPRINTABLE)  {
/* 			if (strcmp(buf, CRLF_STRING) == 0 */
/* 					continue */
		}

		dec_len = EM_SAFE_STRLEN(buf);

		if (mode > SAVE_TYPE_SIZE) {	/*  decode content */
			emcore_decode_body_text(buf, dec_len, encoding, &dec_len, &error);

			if (is_text) {
				result_buffer = em_replace_string(buf, "cid:", "");
				if (result_buffer)
					result_buffer_size = EM_SAFE_STRLEN(result_buffer);
			}

			if (result_buffer == NULL) {
				result_buffer      = strdup(buf); /*prevent 35499*/
				result_buffer_size = dec_len;
			}

			if (mode == SAVE_TYPE_BUFFER)  {   /*  save content to buffer */
				pTemp = realloc(*holder, sz + result_buffer_size + 2);
				if (!pTemp)  {
					EM_DEBUG_EXCEPTION("realloc failed...");
						error = EMAIL_ERROR_OUT_OF_MEMORY;

						EM_SAFE_FREE(*holder);
						EM_SAFE_FREE(result_buffer);
						goto FINISH_OFF;
				}
					else
					*holder = pTemp;

					memcpy(*holder + sz, result_buffer, result_buffer_size);
					(*holder)[sz + EM_SAFE_STRLEN(result_buffer) + 1] = NULL_CHAR;
			} else if (mode == SAVE_TYPE_FILE)  {	/*  save content to file */
				if (write(fd, result_buffer, result_buffer_size) != result_buffer_size)  {
					EM_DEBUG_EXCEPTION("write failed");
					error = EMAIL_ERROR_SYSTEM_FAILURE;
					goto FINISH_OFF;
				}
			}

			EM_SAFE_FREE(result_buffer);
		}
		sz += dec_len;
	}

FINISH_OFF:

	EM_SAFE_FREE(result_buffer); /*prevent 35499*/

	if (error == EMAIL_ERROR_NONE) {
		if (size)
			*size = sz;
	}

	EM_DEBUG_FUNC_END("error [%d], sz[%d]", error, sz);
	return error;
}

