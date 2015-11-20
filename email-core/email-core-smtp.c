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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <alarm.h>
#include <dlfcn.h>
#include <ctype.h>
#include <sys/shm.h>

#include "email-internal-types.h"
#include "c-client.h"
#include "email-daemon.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-storage.h"
#include "email-core-smtp.h"
#include "email-core-event.h"
#include "email-core-mailbox.h"
#include "email-core-mail.h"
#include "email-core-mime.h"
#include "email-core-smime.h"
#include "email-core-account.h"
#include "email-core-imap-mailbox.h"
#include "email-core-mailbox-sync.h"
#include "email-core-signal.h"
#include "email-core-alarm.h"
#include "email-utilities.h"
#include "email-convert.h"
#include "email-debug-log.h"
#include "email-core-gmime.h"
#include "email-core-container.h"
#include "email-network.h"

#undef min



#ifdef __FEATURE_SUPPORT_REPORT_MAIL__
static int emcore_get_report_mail_body(char *multi_user_name, ENVELOPE *envelope, BODY **multipart_body, int *err_code);
#endif
static int emcore_make_envelope_from_mail(char *multi_user_name, emstorage_mail_tbl_t *input_mail_tbl_data, ENVELOPE **output_envelope);
static int emcore_send_mail_smtp(char *multi_user_name, SENDSTREAM *stream, ENVELOPE *env, char *data_file, int account_id, int mail_id,  int *err_code);
char *emcore_generate_content_id_string(const char *hostname, int *err);

/* Functions from uw-imap-toolkit */
/* extern void *fs_get(size_t size); */
extern void rfc822_date(char *date);
extern long smtp_send(SENDSTREAM *stream, char *command, char *args);
extern long smtp_rcpt(SENDSTREAM *stream, ADDRESS *adr, long* error);

#ifndef __FEATURE_SEND_OPTMIZATION__
extern long smtp_soutr(void *stream, char *s);
#endif

#ifdef __FEATURE_SEND_OPTMIZATION__
extern long smtp_soutr_test(void *stream, char *s);
#endif




void mail_send_notify(email_send_status_t status, int total, int sent, int account_id, int mail_id,  int err_code)
{
	EM_DEBUG_FUNC_BEGIN("status[%d], total[%d], sent[%d], account_id[%d], mail_id[%d], err_code[%d]", status, total, sent, account_id, mail_id, err_code);

	switch (status) {
		case EMAIL_SEND_CONNECTION_FAIL:
		case EMAIL_SEND_FINISH:
		case EMAIL_SEND_FAIL:
			break;

		case EMAIL_SEND_PROGRESS:
		default:
			break;
	}
	emcore_execute_event_callback(EMAIL_ACTION_SEND_MAIL, total, sent, status, account_id, mail_id, -1, err_code);
	EM_DEBUG_FUNC_END();
}

/* ------ rfc822 handle ---------------------------------------------------*/
long buf_flush(void *stream, char *string)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], string[%s]", stream, string);
	EM_DEBUG_FUNC_END();
	return 1;
}


#define RFC822_READ_BLOCK_SIZE	  1024
#define RFC822_STRING_BUFFER_SIZE 1536

static char *emcore_find_img_tag(char *source_string)
{
	EM_DEBUG_FUNC_BEGIN("source_string[%p]", source_string);

	int cur = 0, string_length;
	if (!source_string)
		return false;

	string_length = EM_SAFE_STRLEN(source_string);

	for (cur = 0; cur < string_length; cur++) {
		if (source_string[cur] == 'I' || source_string[cur] == 'i') {
			cur++;
			if (source_string[cur] == 'M' || source_string[cur] == 'm') {
				cur++;
				if (source_string[cur] == 'G' || source_string[cur] == 'g') {
					EM_DEBUG_FUNC_END("%s", source_string + cur - 2);
					return source_string + cur - 2;
				}
			}
		}
	}
	EM_DEBUG_FUNC_END("Can't find");
	return NULL;
}

#define CONTENT_ID_BUFFER_SIZE 512
static char *emcore_replace_inline_image_path_with_content_id(char *source_string, BODY *root_body, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("source_string[%p], root_body[%p], err_code[%p]", source_string, root_body, err_code);

	int  err = EMAIL_ERROR_NONE;
	char content_id_buffer[CONTENT_ID_BUFFER_SIZE] = {0,};
	char file_name_buffer[512] = {0,};
	char old_string[512] = {0,};
	char new_string[512] = {0,};
	char *result_string = NULL;
	char *input_string = NULL;
	BODY *cur_body = NULL;
	PART *cur_part = NULL;

	if (!source_string || !root_body) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM [%p] [%p]", source_string, root_body);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	input_string = EM_SAFE_STRDUP(source_string);

	cur_body = root_body;
	if (root_body->type != TYPEMULTIPART) {
		EM_DEBUG_LOG("The body is not multipart : type[%d]", root_body->type);
		goto FINISH_OFF;
	}

	do {
		cur_part = cur_body->nested.part;

		if (strcasecmp(cur_body->subtype, "RELATED") == 0) {
			EM_DEBUG_LOG("Related part");
			while (cur_part) {
				cur_body = &(cur_part->body);

				if (cur_body->disposition.type && cur_body->disposition.type[0] == 'i') {   /*  Is inline content? */
					EM_DEBUG_LOG("Has inline content");
					memset(content_id_buffer, 0, CONTENT_ID_BUFFER_SIZE);
					if (cur_body->id) {
						EM_SAFE_STRNCPY(content_id_buffer, cur_body->id + 1, CONTENT_ID_BUFFER_SIZE - 1); /*  Removing <, > */
						/* prevent 34413 */
						char *last_bracket = rindex(content_id_buffer, '>');
						if (last_bracket) *last_bracket = NULL_CHAR;

						/* if (emcore_get_attribute_value_of_body_part(cur_body->parameter, "name", file_name_buffer, CONTENT_ID_BUFFER_SIZE, false, &err)) { */
						if (emcore_get_attribute_value_of_body_part(cur_body->parameter, "name", file_name_buffer, CONTENT_ID_BUFFER_SIZE, true, &err)) {
							EM_DEBUG_LOG_SEC("Content-ID[%s], filename[%s]", content_id_buffer, file_name_buffer);
							SNPRINTF(new_string, CONTENT_ID_BUFFER_SIZE, "\"cid:%s\"", content_id_buffer);
							SNPRINTF(old_string, CONTENT_ID_BUFFER_SIZE, "\"%s\"", file_name_buffer);
							result_string = em_replace_string(input_string, old_string, new_string);
						}
					}
				}

				if (result_string) {
					EM_SAFE_FREE(input_string);
					input_string = result_string;
					result_string = NULL; /* prevent 34868 */
				}

				cur_part = cur_part->next;
			}
		}

		if (cur_part)
			cur_body = &(cur_part->body);
		else
			cur_body = NULL;

	} while (cur_body);

	if (input_string)
		result_string = EM_SAFE_STRDUP(input_string);
FINISH_OFF:

	EM_SAFE_FREE(input_string);
	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret[%s]", result_string);
	return result_string;
}

static int emcore_write_body (BODY *body, BODY *root_body, FILE *fp)
{
	EM_DEBUG_FUNC_BEGIN("fp[%d]", fp);
	char *file_path = NULL;
	char buf[RFC822_STRING_BUFFER_SIZE + 1] = {0};
	char *img_tag_pos = NULL;
	char *p = NULL;
	char *replaced_string = NULL;
	int fd = 0;
	int nread = 0;
	int nwrite = 0;
	int error = EMAIL_ERROR_NONE;
	unsigned long len;
	char *tmp_p = NULL;
	char *tmp_file_path = NULL;
	FILE *fp_html = NULL;
	FILE *fp_write = NULL;
	char *full_buf = NULL;

	file_path = body->sparep;
	if (!file_path || EM_SAFE_STRLEN (file_path) == 0) {
		EM_DEBUG_LOG("There is no file path");
		switch (body->encoding) {
			case 0:
			default:
				p = cpystr((const char *)body->contents.text.data);
				len = body->contents.text.size;
				break;
		}

		if (p) {
			EM_DEBUG_LOG("p[%s]", p);
			fprintf(fp, "%s"CRLF_STRING CRLF_STRING, p);
			EM_SAFE_FREE(p);
		}

		EM_SAFE_FREE(body->sparep);
		EM_DEBUG_FUNC_END();
		return error;
	}

	if (body->type == TYPETEXT && body->subtype && toupper(body->subtype[0]) == 'H') {
		EM_DEBUG_LOG ("HTML Part");

		struct stat st_buf;
		unsigned int byte_read = 0;

		if (stat (file_path, &st_buf) < 0) {
			EM_DEBUG_EXCEPTION ("stat [%s] error [%d]", file_path, errno);
			error = EMAIL_ERROR_FILE ;
			goto FINISH_OFF;
		}

		error = em_fopen(file_path, "r", &fp_html);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_fopen [%s] error [%d] ", file_path, error);
			goto FINISH_OFF;
		}

		if (S_ISREG (st_buf.st_mode) && st_buf.st_size == 0) {
			EM_DEBUG_LOG ("file_path[%s] is empty size", file_path);
			error = EMAIL_ERROR_FILE ;
			goto FINISH_OFF;
		}

		if (!(full_buf = (char*) em_malloc (sizeof (char) * (st_buf.st_size + 1)))) {
			EM_DEBUG_EXCEPTION ("em_malloc failed");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		byte_read = fread (full_buf, sizeof(char), st_buf.st_size, fp_html);

		if (byte_read <= 0) {
			EM_SAFE_FREE (full_buf);
			if (ferror (fp_html)) {
				EM_DEBUG_EXCEPTION("fread [%s] error [%d]", file_path, errno);
				error = EMAIL_ERROR_FILE ;
				goto FINISH_OFF;
			}
		} else {
			EM_DEBUG_LOG_DEV("==================================================");
			EM_DEBUG_LOG_DEV("fread : %s", full_buf);
			EM_DEBUG_LOG_DEV("==================================================");

			img_tag_pos = emcore_find_img_tag(full_buf);

			if (img_tag_pos) {
				replaced_string = emcore_replace_inline_image_path_with_content_id(full_buf, root_body, &error);
				if (replaced_string) {
					EM_DEBUG_LOG("emcore_replace_inline_image_path_with_content_id succeeded");
					EM_DEBUG_LOG_DEV("==================================================");
					EM_DEBUG_LOG_DEV("replaced_string : %s", replaced_string);
					EM_DEBUG_LOG_DEV("==================================================");

					if (!emcore_get_temp_file_name (&tmp_file_path, &error))  {
						EM_DEBUG_EXCEPTION (" em_core_get_temp_file_name failed[%d]", error);
						goto FINISH_OFF;
					}
					EM_DEBUG_LOG_DEV("tmp file path : %s", tmp_file_path);

					error = em_fopen(tmp_file_path, "w", &fp_write);
					if (error != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION_SEC ("em_fopen [%s] error [%d]", tmp_file_path, error);
						goto FINISH_OFF;
					}

					if (fseek (fp_write, 0, SEEK_SET) < 0) {
						EM_DEBUG_EXCEPTION("fseek failed : error [%d]", errno);
						error = EMAIL_ERROR_FILE;
						goto FINISH_OFF;
					}

					if (fprintf(fp_write, "%s", replaced_string) <= 0) {
						EM_DEBUG_EXCEPTION("fprintf failed : error [%d]", errno);
						error = EMAIL_ERROR_FILE;
						goto FINISH_OFF;
					}

					file_path = tmp_file_path;
					fclose(fp_write);
					fp_write = NULL;
				}
			}
		}
	}

	EM_DEBUG_LOG_DEV("Opening a file[%s]", file_path);

	error = em_open(file_path, O_RDONLY, 0, &fd);
	if (error != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION_SEC("open[%s] error [%d]", file_path, error);
		goto FINISH_OFF;
	}

	while (1) {
		EM_SAFE_FREE (p);
		memset(&buf, 0x00, RFC822_STRING_BUFFER_SIZE + 1);
		nread = read(fd, buf, (body->encoding == ENCBASE64 ? 57 : RFC822_READ_BLOCK_SIZE - 2));

		if (nread == 0)  {
			EM_DEBUG_LOG_DEV("Can't read anymore : nread[%d]", nread);
			break;
		} else if (nread < 0) {
			EM_DEBUG_EXCEPTION ("read [%s] error [%d]", file_path, errno);
			error = EMAIL_ERROR_FILE;
			break;
		}

		len = nread;

		/* EM_DEBUG_LOG("body->encoding[%d]", body->encoding); */
		switch (body->encoding)  {
			case ENCQUOTEDPRINTABLE:
				p = (char *) rfc822_8bit ((unsigned char *)buf, (unsigned long)nread, (unsigned long *)&len);
				break;
			case ENCBASE64:
				tmp_p = g_base64_encode((guchar *)buf, (gsize)nread);
				p = g_strconcat(tmp_p, "\r\n", NULL);
				EM_SAFE_FREE(tmp_p);
				len = EM_SAFE_STRLEN(p);
				break;
		}

		//prepend additional dot to the string start with dot
		if (p && p[0] == '.') {
			tmp_p = g_strdup(p);
			EM_SAFE_FREE(p);
			p = g_strconcat(".", tmp_p, NULL);
			EM_SAFE_FREE(tmp_p);
		}

		if (!p && buf[0] == '.') {
			p = g_strconcat(".", buf, NULL);
		}

		len = EM_SAFE_STRLEN((p?p:buf));
		EM_DEBUG_LOG_DEV("line[%s]", (p?p:buf));

		nwrite = fprintf (fp, "%s", (p?p:buf));
		if (nwrite != len) {
			EM_DEBUG_EXCEPTION("fprintf error: nwrite[%d] len[%d] error[%d]", nwrite, len, errno);
			error = EMAIL_ERROR_FILE ;
			goto FINISH_OFF;
		}
	}

	if (body->encoding == ENCQUOTEDPRINTABLE || body->encoding == ENCBASE64)
		fprintf(fp, CRLF_STRING);

FINISH_OFF: /* prevent 34226 */

	/* cleanup local vars */
	EM_SAFE_FREE (body->sparep);
	EM_SAFE_CLOSE (fd); /*prevent 34498*/
	EM_SAFE_FREE (p);
	if (tmp_file_path)
		g_remove(tmp_file_path);
	EM_SAFE_FREE(tmp_file_path);
	if (fp_html)
		fclose (fp_html);
	if (fp_write)
		fclose (fp_write);
	EM_SAFE_FREE(full_buf);
	EM_SAFE_FREE (replaced_string);
	EM_DEBUG_FUNC_END();
	return error;
}

static int emcore_write_rfc822_body(BODY *body, BODY *root_body, FILE *fp, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("body[%p], fp[%p], err_code[%p]", body, fp, err_code);

	PARAMETER *param = NULL;
	PART *part = NULL;
	char *p = NULL, *bndry = NULL, buf[SENDBUFLEN];
	int error = EMAIL_ERROR_NONE;

	switch (body->type)  {
		case TYPEMULTIPART:
			EM_DEBUG_LOG_DEV("body->type = TYPEMULTIPART");

			part = body->nested.part;

			for (param = body->parameter; param; param = param->next)  {
				if (strcasecmp(param->attribute, "BOUNDARY") == 0) {
					bndry = param->value;
					break;
				}
			}

			do  {
				p = buf; p[0] = '\0';

				rfc822_write_body_header(&p, &part->body);

				fprintf(fp, CRLF_STRING"--%s"CRLF_STRING, bndry);
				if (body->subtype && (body->subtype[0] == 'S' || body->subtype[0] == 's')) {
					if ((error = emcore_write_body (body, root_body, fp)) != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("emcore_write_body failed : [%d]", error);
						return false;
					}
					fprintf(fp, CRLF_STRING"--%s"CRLF_STRING, bndry);
				}

				fprintf(fp, "%s", buf);
				emcore_write_rfc822_body(&part->body, root_body, fp, err_code);

			} while ((part = part->next));

			fprintf(fp, CRLF_STRING"--%s--"CRLF_STRING, bndry);
			break;

		default:
			EM_DEBUG_LOG_DEV("body->type is not TYPEMULTIPART");

			fprintf(fp, CRLF_STRING);
			if ((error = emcore_write_body (body, root_body, fp)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_write_body failed : [%d]", error);
				return false;
			}

			break;
	}
	EM_DEBUG_FUNC_END();
	return true;
}

static int emcore_write_rfc822 (ENVELOPE *env, BODY *body, email_mail_priority_t input_priority,
                                                     email_mail_report_t input_report_flag, char **data)
{
	EM_DEBUG_FUNC_BEGIN("env[%p], body[%p], data[%p]", env, body, data);

	int error = EMAIL_ERROR_NONE;
	FILE *fp = NULL;
	char *fname = NULL;
	int file_exist = 0;
	char  *header_buffer = NULL;
	size_t header_buffer_lenth = 0;
	RFC822BUFFER buf;
	int  address_count = 0;
	ADDRESS *index = NULL;

	if (!env || !data)  {
		EM_DEBUG_EXCEPTION("Invalid Parameters");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	srand(time(NULL));

	rfc822_encode_body_7bit(env, body); /*  if contents.text.data isn't NULL, the data will be encoded. */

	index = env->to;
	while(index) {
		address_count++;
		index = index->next;
	}

	index = env->cc;
	while(index) {
		address_count++;
		index = index->next;
	}

	header_buffer_lenth = (env->subject ? EM_SAFE_STRLEN(env->subject) : 0);
	header_buffer_lenth += address_count * MAX_EMAIL_ADDRESS_LENGTH;
	header_buffer_lenth += 8192;

	EM_DEBUG_LOG("header_buffer_lenth [%d]", header_buffer_lenth);

	if (!(header_buffer = em_malloc(header_buffer_lenth)))  {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	/* write at start of buffer */
	buf.beg = buf.cur = header_buffer;
	buf.end = header_buffer + header_buffer_lenth - 1;
	buf.f   = buf_flush;
	buf.s   = NULL;

	/*  rfc822_output_header(&buf, env, body, NIL, T); */		/*  including BCC  */
	rfc822_output_header(&buf, env, body, NIL, NIL);		/*  Excluding BCC */

	EM_DEBUG_LOG("header_buffer [%d]", strlen(header_buffer));
	{
		gchar **tokens = g_strsplit(header_buffer, "CHARSET=X-UNKNOWN", 2);

		if (g_strv_length(tokens) > 1)  {
			gchar *charset;

			if (body->sparep) {
				charset = g_path_get_basename(body->sparep);
				char *pHtml = NULL;
				if (charset != NULL) {
					if ((pHtml = strstr(charset, ".htm")) != NULL)
						charset[pHtml-charset] = '\0';
				}

				SNPRINTF(header_buffer, header_buffer_lenth, "%sCHARSET=%s%s", tokens[0], charset, tokens[1]);
				g_free(charset);
			}
			else
				EM_DEBUG_EXCEPTION("body->sparep is NULL");
		}

		g_strfreev(tokens);
	}

	{
		gchar **tokens = g_strsplit(header_buffer, "To: undisclosed recipients: ;\015\012", 2);
		if (g_strv_length(tokens) > 1)
			SNPRINTF(header_buffer, header_buffer_lenth, "%s%s", tokens[0], tokens[1]);
		g_strfreev(tokens);
	}

	EM_DEBUG_LOG_DEV(" =============================================================================== "
		LF_STRING"%s"LF_STRING
		" =============================================================================== ", header_buffer);

	if (EM_SAFE_STRLEN(header_buffer) > 2)
		*(header_buffer + EM_SAFE_STRLEN(header_buffer) - 2) = '\0';

	if (input_report_flag)  {
		char string_buf[512] = {0x00, };

		if(input_report_flag & EMAIL_MAIL_REPORT_DSN) {
			/*  DSN (delivery status) */
			/*  change content-type */
			/*  Content-Type: multipart/report; */
			/* 		report-type= delivery-status; */
			/* 		boundary="----=_NextPart_000_004F_01C76EFF.54275C50" */
		}

		if(input_report_flag & EMAIL_MAIL_REPORT_MDN) {
			/*  MDN (read receipt) */
			/*  Content-Type: multipart/report; */
			/* 		report-type= disposition-notification; */
			/* 		boundary="----=_NextPart_000_004F_01C76EFF.54275C50" */
		}

		if(input_report_flag & EMAIL_MAIL_REQUEST_MDN) {
			/*  require read status */
			rfc822_address(string_buf, env->from);
			if (EM_SAFE_STRLEN(string_buf))
				SNPRINTF(header_buffer + EM_SAFE_STRLEN(header_buffer), header_buffer_lenth - EM_SAFE_STRLEN(header_buffer), "Disposition-Notification-To: %s"CRLF_STRING, string_buf);
		}
	}

	if (input_priority > 0)  {		/*  priority (1:high 3:normal 5:low) */
		SNPRINTF(header_buffer + EM_SAFE_STRLEN(header_buffer), header_buffer_lenth-(EM_SAFE_STRLEN(header_buffer)), "X-Priority: %d"CRLF_STRING, input_priority);

		switch (input_priority)  {
			case EMAIL_MAIL_PRIORITY_HIGH:
				SNPRINTF(header_buffer + EM_SAFE_STRLEN(header_buffer), header_buffer_lenth-(EM_SAFE_STRLEN(header_buffer)), "X-MSMail-Priority: High"CRLF_STRING);
				break;
			case EMAIL_MAIL_PRIORITY_NORMAL:
				SNPRINTF(header_buffer + EM_SAFE_STRLEN(header_buffer), header_buffer_lenth-(EM_SAFE_STRLEN(header_buffer)), "X-MSMail-Priority: Normal"CRLF_STRING);
				break;
			case EMAIL_MAIL_PRIORITY_LOW:
				SNPRINTF(header_buffer + EM_SAFE_STRLEN(header_buffer), header_buffer_lenth-(EM_SAFE_STRLEN(header_buffer)), "X-MSMail-Priority: Low"CRLF_STRING);
				break;
		}
	}

//	SNPRINTF(header_buffer + EM_SAFE_STRLEN(header_buffer), header_buffer_lenth-(EM_SAFE_STRLEN(header_buffer)), CRLF_STRING);
	if (data && EM_SAFE_STRLEN(*data) > 0) {
		fname = EM_SAFE_STRDUP(*data);
		file_exist = 1;
	}
	else {
		if (!emcore_get_temp_file_name(&fname, &error))  {
			EM_DEBUG_EXCEPTION(" emcore_get_temp_file_name failed[%d]", error);
			goto FINISH_OFF;
		}
	}

	error = em_fopen(fname, "w+", &fp);
	if (error != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_fopen failed[%s] [%d]", fname, error);
		goto FINISH_OFF;
	}

	fprintf(fp, "%s", header_buffer);

	if (body)  {
		if (!emcore_write_rfc822_body(body, body, fp, &error))  {
			EM_DEBUG_EXCEPTION("emcore_write_rfc822_body failed[%d]", error);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	if (fp != NULL)
		fclose(fp);

#ifdef USE_SYNC_LOG_FILE
	emstorage_copy_file(fname, "/tmp/phone2pc.eml", false, NULL);
#endif

	if (error == EMAIL_ERROR_NONE) {
		if (!file_exist)
			*data = EM_SAFE_STRDUP(fname);
	}
	else if (fname != NULL) {
		remove(fname);
	}

	EM_SAFE_FREE(fname);
	EM_SAFE_FREE(header_buffer);

	EM_DEBUG_FUNC_END();
	return error;
}

INTERNAL_FUNC int emcore_add_mail(char *multi_user_name, email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t *input_meeting_request, int input_from_eas, int move_flag)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list [%p], input_attachment_count [%d], input_meeting_request [%p], input_from_eas[%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas);

	/* IF an account is being deleted, there is no need to add the mail of the account */
	int shmid;
	extern key_t del_account_key; /* in emcore_delete_account */
	int *del_account_id = NULL;
	/* get the segment.*/

	if ((shmid = shmget (del_account_key, sizeof (int), 0666)) != -1) {
		/* attach the segment to current process space */
		if ((del_account_id = (int*) shmat (shmid, NULL, 0)) != (int*) -1) {
			/* compare two account ids */
			EM_DEBUG_LOG ("del_id[%d] account_id[%d]",*del_account_id, input_mail_data->account_id);
			if (*del_account_id == input_mail_data->account_id) {
				EM_DEBUG_LOG ("SKIP adding mail: the account is being deleted");
				return EMAIL_ERROR_ACCOUNT_NOT_FOUND;
			}
		}
	}
	EM_DEBUG_LOG ("read del_id, account_id [%d]", (del_account_id? *del_account_id:-100),input_mail_data->account_id);


	int err = EMAIL_ERROR_NONE;
	int attachment_id = 0, thread_id = -1, thread_item_count = 0, latest_mail_id_in_thread = -1;
	int i = 0, rule_len, priority_sender = 0, blocked = 0, local_attachment_count = 0, local_inline_content_count = 0;
	int mailbox_id_spam = 0, mailbox_id_target = 0;
	int mail_smime_flag = 0;
	char name_buf[MAX_PATH] = {0x00, };
    char path_buf[MAX_PATH] = {0, };
	char *body_text_file_name = NULL;
	char *dl_error = NULL;
	void *dl_handle = NULL;
	int (*convert_mail_data_to_smime)(char*, emstorage_account_tbl_t*, email_mail_data_t*, email_attachment_data_t*, int, email_mail_data_t**, email_attachment_data_t**, int*);

	int attachment_count = 0;
	email_mail_data_t *mail_data = NULL;
	email_attachment_data_t *attachment_data_list = NULL;
	emstorage_mail_tbl_t    *converted_mail_tbl = NULL;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;
	emstorage_attachment_tbl_t attachment_tbl = { 0 };
	emstorage_account_tbl_t *account_tbl_item = NULL;
	emstorage_rule_tbl_t *rule = NULL;
	struct stat st_buf = { 0 };
	char mailbox_id_param_string[10] = {0,};
	char errno_buf[ERRNO_BUF_SIZE] = {0};
	int updated_thread_id = 0;
    char *prefix_path = NULL;
    char real_file_path[MAX_PATH] = {0};

#ifdef __FEATURE_BODY_SEARCH__
	char *stripped_text = NULL;
#endif

	/* Validating parameters */
	if (!input_mail_data || !(input_mail_data->account_id) || !(input_mail_data->mailbox_id))  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emcore_is_storage_full()) == EMAIL_ERROR_MAIL_MEMORY_FULL) {
		EM_DEBUG_EXCEPTION("Storage is full");
		goto FINISH_OFF;
	}

	if (!emstorage_get_account_by_id(multi_user_name, input_mail_data->account_id, EMAIL_ACC_GET_OPT_DEFAULT | EMAIL_ACC_GET_OPT_OPTIONS, &account_tbl_item, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed. account_id[%d] err[%d]", input_mail_data->account_id, err);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (input_from_eas == 0 && input_mail_data->smime_type && input_mail_data->mailbox_type != EMAIL_MAILBOX_TYPE_DRAFT) {
		dl_handle = dlopen("libemail-smime-api.so.1", RTLD_LAZY);
		if (!dl_handle) {
			EM_DEBUG_EXCEPTION("Open failed : [%s]", dl_handle);
			err = EMAIL_ERROR_INVALID_PATH;
			goto FINISH_OFF;
		}

		dlerror();
		convert_mail_data_to_smime = dlsym(dl_handle, "emcore_convert_mail_data_to_smime_data");
		if ((dl_error = dlerror()) != NULL) {
			EM_DEBUG_EXCEPTION("Symbol open failed [%s]", err);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		if ((err = convert_mail_data_to_smime(multi_user_name, account_tbl_item, input_mail_data, input_attachment_data_list, input_attachment_count, &mail_data, &attachment_data_list, &attachment_count)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("S/MIME failed");
			goto FINISH_OFF;
		}

		mail_smime_flag = 1;
	} else {
		mail_data = input_mail_data;
		attachment_data_list = input_attachment_data_list;
		attachment_count = input_attachment_count;
	}

	mailbox_id_target = mail_data->mailbox_id;

    if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		err = emcore_get_container_path(multi_user_name, &prefix_path);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_container_path failed : [%d]", err);
			goto FINISH_OFF;
		}
	} else {
		prefix_path = strdup("");
	}

	if (input_from_eas == 0 &&
			!(input_mail_data->message_class & EMAIL_MESSAGE_CLASS_SMART_REPLY) &&
			!(input_mail_data->message_class & EMAIL_MESSAGE_CLASS_SMART_FORWARD) ) {
		if (mail_data->file_path_plain)  {
            memset(real_file_path, 0x00, sizeof(real_file_path));
            SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, mail_data->file_path_plain);

			if (stat(real_file_path, &st_buf) < 0)  {
				EM_DEBUG_EXCEPTION_SEC("mail_data->file_path_plain, stat(\"%s\") failed...", mail_data->file_path_plain);
				EM_DEBUG_EXCEPTION("%s", EM_STRERROR(errno_buf));
				err = EMAIL_ERROR_INVALID_MAIL;
				goto FINISH_OFF;
			}
		}

		if (mail_data->file_path_html)  {
            memset(real_file_path, 0x00, sizeof(real_file_path));
            SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, mail_data->file_path_html);

			if (stat(real_file_path, &st_buf) < 0)  {
				EM_DEBUG_EXCEPTION_SEC("mail_data->file_path_html, stat(\"%s\") failed...", mail_data->file_path_html);
				EM_DEBUG_EXCEPTION("%s", EM_STRERROR(errno_buf) );
				err = EMAIL_ERROR_INVALID_MAIL;
				goto FINISH_OFF;
			}
		}

		if (attachment_count && attachment_data_list)  {
			for (i = 0; i < attachment_count; i++)  {
				if (attachment_data_list[i].save_status) {
                    memset(real_file_path, 0x00, sizeof(real_file_path));
                    SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, attachment_data_list[i].attachment_path);

					if (!attachment_data_list[i].attachment_path || stat(real_file_path, &st_buf) < 0)  {
						EM_DEBUG_EXCEPTION("stat(\"%s\") failed...", attachment_data_list[i].attachment_path);
						err = EMAIL_ERROR_INVALID_ATTACHMENT;
						goto FINISH_OFF;
					}
				}
			}
		}

		if (!input_mail_data->full_address_from)
			input_mail_data->full_address_from = EM_SAFE_STRDUP(account_tbl_item->user_email_address);

		/* check for email_address validation */
		if ((err = em_verify_email_address_of_mail_data (mail_data)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_verify_email_address_of_mail_data failed [%d]", err);
			goto FINISH_OFF;
		}

		if (mail_data->report_status & EMAIL_MAIL_REPORT_MDN)  {
			/* check read-report mail */
			if(!mail_data->full_address_to) { /* A report mail should have 'to' address */
				EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
				err = EMAIL_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}
			/* Create report mail body */
			/*
			if (!emcore_make_rfc822_file_from_mail(multi_user_name, mail_src, NULL, NULL, NULL, &err))  {
				EM_DEBUG_EXCEPTION("emcore_make_rfc822_file_from_mail failed [%d]", err);
				goto FINISH_OFF;
			}
			*/
		}
	}
	else {	/*  For Spam handling */
		email_option_t *opt = &account_tbl_item->options;
		EM_DEBUG_LOG_SEC("block_address [%d], block_subject [%d]", opt->block_address, opt->block_subject);

        /* For eas moving from spambox to other mailbox */
        if (mail_data->save_status != EMAIL_MAIL_STATUS_SAVED) {
            if (opt->block_address || opt->block_subject)  {
                int is_completed = false;
                int type = 0;

                if (!opt->block_address)
                    type = EMAIL_FILTER_SUBJECT;
                else if (!opt->block_subject)
                    type = EMAIL_FILTER_FROM;

                if (!emstorage_get_rule(multi_user_name, ALL_ACCOUNT, type, 0, &rule_len, &is_completed, &rule, true, &err) || !rule)
                    EM_DEBUG_LOG("No proper rules. emstorage_get_rule returns [%d]", err);

                if (rule && !emcore_check_rule(mail_data->full_address_from, mail_data->subject, rule, rule_len, &priority_sender, &blocked, &err)) {
                    EM_DEBUG_EXCEPTION("emcore_check_rule failed [%d]", err);
                }

                if (priority_sender)
                    mail_data->tag_id = PRIORITY_SENDER_TAG_ID;

                if (blocked && (mail_data->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX)) {
                    EM_DEBUG_LOG("mail[%d] added to spambox", mail_data->mail_id);
                    if (!emstorage_get_mailbox_id_by_mailbox_type(multi_user_name, mail_data->account_id, EMAIL_MAILBOX_TYPE_SPAMBOX, &mailbox_id_spam, false, &err))  {
                        EM_DEBUG_EXCEPTION("emstorage_get_mailbox_name_by_mailbox_type failed [%d]", err);
                        mailbox_id_spam = 0;
                    }

                    if (mailbox_id_spam)
                        mailbox_id_target = mailbox_id_spam;
                }
            }
        }
	}

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, mailbox_id_target, (emstorage_mailbox_tbl_t**)&mailbox_tbl)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emstorage_increase_mail_id(multi_user_name, &mail_data->mail_id, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_increase_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("mail_data->mail_size [%d]", mail_data->mail_size);

	if(mail_data->mail_size == 0)
		emcore_calc_mail_size(multi_user_name, mail_data, attachment_data_list, attachment_count, &(mail_data->mail_size)); /*  Getting file size before file moved.  */

	EM_DEBUG_LOG("input_from_eas [%d] mail_data->body_download_status [%d]", input_from_eas, mail_data->body_download_status);

	if (input_from_eas == 0|| mail_data->body_download_status) {
		if (!emstorage_create_dir(multi_user_name, mail_data->account_id, mail_data->mail_id, 0, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (mail_data->file_path_plain) {
            memset(name_buf, 0x00, sizeof(name_buf));
            memset(path_buf, 0x00, sizeof(path_buf));
            memset(real_file_path, 0x00, sizeof(real_file_path));
            SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, mail_data->file_path_plain);

			EM_DEBUG_LOG_SEC("mail_data->file_path_plain [%s]", mail_data->file_path_plain);
			/* EM_SAFE_STRNCPY(body_text_file_name, "UTF-8", MAX_PATH); */

			if ( (err = em_get_file_name_from_file_path(real_file_path, &body_text_file_name)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("em_get_file_name_from_file_path failed [%d]", err);
				err = EMAIL_ERROR_INVALID_FILE_PATH;
				goto FINISH_OFF;
			}

			/*
			if (input_from_eas)
				EM_SAFE_STRNCPY(body_text_file_name, UNKNOWN_CHARSET_PLAIN_TEXT_FILE, MAX_PATH);
			else
				EM_SAFE_STRNCPY(body_text_file_name, "UTF-8", MAX_PATH);
			*/

			if (!emstorage_get_save_name(multi_user_name, mail_data->account_id, mail_data->mail_id,
										0, body_text_file_name, name_buf, path_buf,
										sizeof(path_buf), &err))  {
				EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
				goto FINISH_OFF;
			}

			if (!emstorage_move_file(real_file_path, name_buf, input_from_eas, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
				goto FINISH_OFF;
			}
			if (!mail_data->body_download_status)
				mail_data->body_download_status = EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;

			EM_SAFE_FREE(mail_data->file_path_plain);
			mail_data->file_path_plain = EM_SAFE_STRDUP(path_buf);
		}

		if (mail_data->file_path_html) {
            memset(name_buf, 0x00, sizeof(name_buf));
            memset(path_buf, 0x00, sizeof(path_buf));
            memset(real_file_path, 0x00, sizeof(real_file_path));
            SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, mail_data->file_path_html);

			EM_DEBUG_LOG_SEC("mail_data->file_path_html [%s]", mail_data->file_path_html);
			/* EM_SAFE_STRNCPY(body_text_file_name, "UTF-8.htm", MAX_PATH); */

			EM_SAFE_FREE(body_text_file_name);

			if ( (err = em_get_file_name_from_file_path(real_file_path, &body_text_file_name)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("em_get_file_name_from_file_path failed [%d]", err);
				err = EMAIL_ERROR_INVALID_FILE_PATH;
				goto FINISH_OFF;
			}
			/*
			if (input_from_eas)
				EM_SAFE_STRNCPY(body_text_file_name, UNKNOWN_CHARSET_HTML_TEXT_FILE, MAX_PATH);
			else
				EM_SAFE_STRNCPY(body_text_file_name, "UTF-8.htm", MAX_PATH);
			*/

			if (!emstorage_get_save_name(multi_user_name, mail_data->account_id, mail_data->mail_id,
										0, body_text_file_name, name_buf, path_buf,
										sizeof(path_buf), &err))  {
				EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
				goto FINISH_OFF;
			}

			if (!emstorage_move_file(real_file_path, name_buf, input_from_eas, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
				goto FINISH_OFF;
			}

			if (!mail_data->body_download_status)
				mail_data->body_download_status = EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;

			EM_SAFE_FREE(mail_data->file_path_html);
			mail_data->file_path_html = EM_SAFE_STRDUP(path_buf);
		}
	}

	if (mail_data->file_path_mime_entity) {
        memset(name_buf, 0x00, sizeof(name_buf));
        memset(path_buf, 0x00, sizeof(path_buf));
        memset(real_file_path, 0x00, sizeof(real_file_path));
        SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, mail_data->file_path_mime_entity);

		EM_DEBUG_LOG_SEC("mail_data->file_path_mime_entity [%s]", mail_data->file_path_mime_entity);

		if (!emstorage_get_save_name(multi_user_name, mail_data->account_id, mail_data->mail_id,
									0, "mime_entity", name_buf, path_buf,
									sizeof(path_buf), &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(real_file_path, name_buf, input_from_eas, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_SAFE_FREE(mail_data->file_path_mime_entity);
		mail_data->file_path_mime_entity = EM_SAFE_STRDUP(path_buf);
	}

	if (!mail_data->date_time)  {
		/* time isn't set */
		mail_data->date_time = time(NULL);
	}

	/* Generate message_id */
	if (!input_from_eas) {
		mail_data->message_id     = emcore_generate_content_id_string("org.tizen.email", NULL);
		mail_data->server_mail_id = strdup("0");
	}

	mail_data->mailbox_id           = mailbox_id_target;
	mail_data->mailbox_type         = mailbox_tbl->mailbox_type;
	mail_data->server_mail_status   = !input_from_eas;
	if(mail_data->save_status == EMAIL_MAIL_STATUS_NONE)
		mail_data->save_status      = EMAIL_MAIL_STATUS_SAVED;

	/*  Getting attachment count */
	for (i = 0; i < attachment_count; i++) {
		if (attachment_data_list[i].inline_content_status== 1)
			local_inline_content_count++;
		else
			local_attachment_count++;
	}

	mail_data->inline_content_count = local_inline_content_count;
	mail_data->attachment_count     = local_attachment_count;

	EM_DEBUG_LOG("inline_content_count   [%d]", local_inline_content_count);
	EM_DEBUG_LOG("attachment_count [%d]", local_attachment_count);

	EM_DEBUG_LOG("preview_text[%p]", mail_data->preview_text);
	if (mail_data->preview_text == NULL) {
		if ( (err = emcore_get_preview_text_from_file(multi_user_name,
                                                    mail_data->file_path_plain,
                                                    mail_data->file_path_html,
                                                    MAX_PREVIEW_TEXT_LENGTH,
                                                    &(mail_data->preview_text))) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_preview_text_from_file failed[%d]", err);

			if (err != EMAIL_ERROR_EMPTY_FILE)
				goto FINISH_OFF;
		}
	}

	if (!em_convert_mail_data_to_mail_tbl(mail_data, 1, &converted_mail_tbl, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_data_to_mail_tbl failed [%d]", err);
		goto FINISH_OFF;
	}

	converted_mail_tbl->mailbox_id        = mailbox_tbl->mailbox_id;

	/* Fill address information */
	emcore_fill_address_information_of_mail_tbl(multi_user_name, converted_mail_tbl);

	/* Fill thread id */
	if(mail_data->thread_id == 0) {
		if (emstorage_get_thread_id_of_thread_mails(multi_user_name, converted_mail_tbl, &thread_id, &latest_mail_id_in_thread, &thread_item_count) != EMAIL_ERROR_NONE)
			EM_DEBUG_LOG(" emstorage_get_thread_id_of_thread_mails is failed");

		if (thread_id == -1) {
			converted_mail_tbl->thread_id         = mail_data->mail_id;
			converted_mail_tbl->thread_item_count = thread_item_count = 1;
		}
		else  {
			converted_mail_tbl->thread_id         = thread_id;
			thread_item_count++;
		}
	}
	else {
		thread_item_count                         = 2;
	}

	mail_data->thread_id = converted_mail_tbl->thread_id;
	converted_mail_tbl->user_name = EM_SAFE_STRDUP(account_tbl_item->user_name);

	if (!emstorage_begin_transaction(multi_user_name, NULL, NULL, NULL)) {
		EM_DEBUG_EXCEPTION("emstorage_begin_transaction failed [%d]");
		goto FINISH_OFF;
	}

	/*  insert mail to mail table */
	if (!emstorage_add_mail(multi_user_name, converted_mail_tbl, 0, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_add_mail failed [%d]", err);
		/*  ROLLBACK TRANSACTION; */
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
		goto FINISH_OFF;
	}

	/* Update thread information */
	EM_DEBUG_LOG("thread_item_count [%d]", thread_item_count);

	if (thread_item_count > 1) {
		if (!emstorage_update_latest_thread_mail(multi_user_name,
													mail_data->account_id,
													mail_data->mailbox_id,
													mail_data->mailbox_type,
													converted_mail_tbl->thread_id,
													&updated_thread_id,
													0,
													0,
													NOTI_THREAD_ID_CHANGED_BY_ADD,
													false,
													&err)) {
			EM_DEBUG_EXCEPTION("emstorage_update_latest_thread_mail failed [%d]", err);
			emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
			goto FINISH_OFF;
		}

		if (updated_thread_id > 0)
			input_mail_data->thread_id = updated_thread_id;
	}

	/*  Insert attachment information to DB */

	for (i = 0; i < attachment_count; i++) {
        memset(name_buf, 0x00, sizeof(name_buf));
        memset(path_buf, 0x00, sizeof(path_buf));

        /* set attachment size */
        memset(real_file_path, 0x00, sizeof(real_file_path));
        SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, attachment_data_list[i].attachment_path);

		if (attachment_data_list[i].attachment_size == 0) {
			if(attachment_data_list[i].attachment_path && stat(real_file_path, &st_buf) < 0)
				attachment_data_list[i].attachment_size = st_buf.st_size;
		}

		if (!attachment_data_list[i].inline_content_status) {
			if (!emstorage_get_new_attachment_no(multi_user_name, &attachment_id, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_get_new_attachment_no failed [%d]", err);
				emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}

		if (!emstorage_create_dir(multi_user_name, mail_data->account_id, mail_data->mail_id, attachment_data_list[i].inline_content_status ? 0  :  attachment_id, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
			goto FINISH_OFF;
		}

		if (!emstorage_get_save_name(multi_user_name,
                                        mail_data->account_id,
                                        mail_data->mail_id, attachment_data_list[i].inline_content_status ? 0  :  attachment_id,
                                        attachment_data_list[i].attachment_name,
                                        name_buf,
                                        path_buf,
                                        sizeof(path_buf),
                                        &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
			goto FINISH_OFF;
		}
		/* if (input_from_eas == 0 || attachment_data_list[i].save_status) { */
		if (attachment_data_list[i].save_status) {
			if (attachment_data_list[i].attachment_mime_type && strcasestr(attachment_data_list[i].attachment_mime_type, "PKCS7")) {
				if (!emstorage_move_file(real_file_path, name_buf, input_from_eas, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
					emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
					goto FINISH_OFF;
				}
			} else {
				if (!emstorage_copy_file(real_file_path, name_buf, input_from_eas, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_copy_file failed [%d]", err);
					emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
					goto FINISH_OFF;
				}
			}
		}

		memset(&attachment_tbl, 0, sizeof(emstorage_attachment_tbl_t));
		attachment_tbl.attachment_name                  = attachment_data_list[i].attachment_name;
		attachment_tbl.attachment_path                  = path_buf;
		attachment_tbl.attachment_size                  = attachment_data_list[i].attachment_size;
		attachment_tbl.mail_id                          = mail_data->mail_id;
		attachment_tbl.account_id                       = mail_data->account_id;
		attachment_tbl.mailbox_id                       = mail_data->mailbox_id;
		attachment_tbl.attachment_save_status           = attachment_data_list[i].save_status;
		attachment_tbl.attachment_drm_type              = attachment_data_list[i].drm_status;
		attachment_tbl.attachment_inline_content_status = attachment_data_list[i].inline_content_status;
		attachment_tbl.attachment_mime_type             = attachment_data_list[i].attachment_mime_type;
		attachment_tbl.content_id                       = attachment_data_list[i].content_id;

		if (!emstorage_add_attachment(multi_user_name, &attachment_tbl, 0, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_add_attachment failed [%d]", err);
			emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
			goto FINISH_OFF;
		}

		if (!mail_smime_flag) {
			attachment_data_list[i].attachment_id = attachment_tbl.attachment_id;
		}
	}

#ifdef __FEATURE_BODY_SEARCH__
	/* Insert mail_text to DB */
	if (!emcore_strip_mail_body_from_file(multi_user_name, converted_mail_tbl, &stripped_text, &err) || stripped_text == NULL) {
		EM_DEBUG_EXCEPTION("emcore_strip_mail_body_from_file failed [%d]", err);
	}

	if (!emcore_add_mail_text(multi_user_name, mailbox_tbl, converted_mail_tbl, stripped_text, &err)) {
		EM_DEBUG_EXCEPTION("emcore_add_mail_text failed [%d]", err);
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
		goto FINISH_OFF;
	}
#endif

	/*  Insert Meeting request to DB */
	if (mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_REQUEST
		|| mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_RESPONSE
		|| mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		EM_DEBUG_LOG("This mail has the meeting request");
		input_meeting_request->mail_id = mail_data->mail_id;

		if (!emstorage_add_meeting_request(multi_user_name, mail_data->account_id, mailbox_tbl->mailbox_id, input_meeting_request, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_add_meeting_request failed [%d]", err);
			emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
			goto FINISH_OFF;
		}
	}

	emstorage_commit_transaction(multi_user_name, NULL, NULL, NULL);

	if (emstorage_get_thread_id_of_thread_mails(multi_user_name, converted_mail_tbl, &thread_id, &latest_mail_id_in_thread, &thread_item_count) != EMAIL_ERROR_NONE)
		EM_DEBUG_LOG(" emstorage_get_thread_id_of_thread_mails is failed.");

	SNPRINTF(mailbox_id_param_string, 10, "%d", mailbox_tbl->mailbox_id);
	if (!emcore_notify_storage_event(NOTI_MAIL_ADD, converted_mail_tbl->account_id, converted_mail_tbl->mail_id, mailbox_id_param_string, thread_id))
		EM_DEBUG_LOG("emcore_notify_storage_event [NOTI_MAIL_ADD] failed.");

	if (account_tbl_item->incoming_server_type != EMAIL_SERVER_TYPE_ACTIVE_SYNC && !move_flag) {
		if (!emcore_remove_overflowed_mails(multi_user_name, mailbox_tbl, &err)) {
			if (err == EMAIL_ERROR_MAIL_NOT_FOUND || err == EMAIL_ERROR_NOT_SUPPORTED)
				err = EMAIL_ERROR_NONE;
			else
				EM_DEBUG_LOG("emcore_remove_overflowed_mails failed [%d]", err);
		}
	}

	if ( input_from_eas && (mail_data->flags_seen_field == 0)) {
//				&& mail_data->mailbox_type != EMAIL_MAILBOX_TYPE_TRASH
//				&& mail_data->mailbox_type != EMAIL_MAILBOX_TYPE_SPAMBOX) {
		if ((err = emcore_update_sync_status_of_account(multi_user_name, mail_data->account_id, SET_TYPE_SET, SYNC_STATUS_SYNCING | SYNC_STATUS_HAVE_NEW_MAILS)) != EMAIL_ERROR_NONE)
				EM_DEBUG_LOG("emcore_update_sync_status_of_account failed [%d]", err);
	}

FINISH_OFF:
	if (dl_handle)
		dlclose(dl_handle);

	EM_SAFE_FREE(body_text_file_name);
	EM_SAFE_FREE(prefix_path);

#ifdef __FEATURE_BODY_SEARCH__
	EM_SAFE_FREE(stripped_text);
#endif

	if (mail_smime_flag)
		emcore_free_attachment_data(&attachment_data_list, attachment_count, NULL);

	if (account_tbl_item)
		emstorage_free_account(&account_tbl_item, 1, NULL);

	if (mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

	if (converted_mail_tbl)
		emstorage_free_mail(&converted_mail_tbl, 1, NULL);

	EM_DEBUG_FUNC_END();
	return err;
}

INTERNAL_FUNC int emcore_add_read_receipt(char *multi_user_name, int input_read_mail_id, int *output_receipt_mail_id)
{
	EM_DEBUG_FUNC_BEGIN("input_read_mail_id [%d], output_receipt_mail_id [%p]", input_read_mail_id, output_receipt_mail_id);
	int                      err = EMAIL_ERROR_NONE;
	int                      attachment_count = 0;
	ENVELOPE                *envelope = NULL;
	email_mail_data_t       *read_mail_data = NULL;
	email_mail_data_t       *receipt_mail_data = NULL;
	emstorage_mail_tbl_t    *receipt_mail_tbl_data = NULL;
	email_attachment_data_t *attachment_data = NULL;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;
	BODY                    *root_body = NULL;

	if( (err = emcore_get_mail_data(multi_user_name, input_read_mail_id, &read_mail_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_data failed [%d]", err);
		goto FINISH_OFF;
	}

	receipt_mail_data = em_malloc(sizeof(email_mail_data_t));

	if (!receipt_mail_data)  {
		EM_DEBUG_EXCEPTION("em_malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memcpy(receipt_mail_data, read_mail_data, sizeof(email_mail_data_t));

	receipt_mail_data->full_address_to = EM_SAFE_STRDUP(read_mail_data->full_address_from);
	receipt_mail_data->message_id      = EM_SAFE_STRDUP(read_mail_data->message_id);

	if (read_mail_data->subject)  {
		receipt_mail_data->subject = em_malloc(EM_SAFE_STRLEN(read_mail_data->subject) + 7);
		if (!(receipt_mail_data->subject))  {
			EM_DEBUG_EXCEPTION("em_malloc failed...");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		SNPRINTF(receipt_mail_data->subject, EM_SAFE_STRLEN(read_mail_data->subject) + 7,  "Read: %s", read_mail_data->subject);
	}

	if (!emstorage_get_mailbox_by_mailbox_type(multi_user_name, receipt_mail_data->account_id, EMAIL_MAILBOX_TYPE_OUTBOX, &mailbox_tbl, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_name_by_mailbox_type failed [%d]", err);
		goto FINISH_OFF;
	}

	receipt_mail_data->mailbox_id           = mailbox_tbl->mailbox_id;
	receipt_mail_data->mailbox_type         = EMAIL_MAILBOX_TYPE_OUTBOX;
	receipt_mail_data->file_path_html       = NULL;
	receipt_mail_data->flags_draft_field    = 1;
	receipt_mail_data->body_download_status = 1;
	receipt_mail_data->save_status          = (unsigned char)EMAIL_MAIL_STATUS_SENDING;
	receipt_mail_data->report_status        = (unsigned char)EMAIL_MAIL_REPORT_MDN;

	if (!em_convert_mail_data_to_mail_tbl(receipt_mail_data, 1, &receipt_mail_tbl_data, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_data_to_mail_tbl failed [%d]", err);
		goto FINISH_OFF;
	}

	if ( (err = emcore_make_envelope_from_mail(multi_user_name, receipt_mail_tbl_data, &envelope)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_make_envelope_from_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	envelope->references = EM_SAFE_STRDUP(read_mail_data->message_id);

	if (!emcore_get_report_mail_body(multi_user_name, envelope, &root_body, &err))  {
		EM_DEBUG_EXCEPTION("emcore_get_report_mail_body failed [%d]", err);
		goto FINISH_OFF;
	}

	receipt_mail_data->file_path_plain  = EM_SAFE_STRDUP(root_body->nested.part->body.sparep);

	/* Report attachment */
	/* Final-Recipient :  rfc822;digipop@gmail.com
	   Original-Message-ID:  <r97a77ag0jdhkvvxke58u9i5.1345611508570@email.android.com>
	   Disposition :  manual-action/MDN-sent-manually; displayed */

	/*
	receipt_mail_data->attachment_count = 1;
	attachment_count                    = 1;

	attachment_data = em_malloc(sizeof(email_attachment_data_t));
	if (!attachment_data)  {
		EM_DEBUG_EXCEPTION("em_malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	attachment_data->save_status     = 1;
	attachment_data->attachment_path = EM_SAFE_STRDUP(root_body->nested.part->next->body.sparep);

	if (!emcore_get_file_name(attachment_data->attachment_path, &p, &err))  {
		EM_DEBUG_EXCEPTION("emcore_get_file_name failed [%d]", err);
		goto FINISH_OFF;
	}

	attachment_data->attachment_name = cpystr(p);
	*/

	if ( (err = emcore_add_mail(multi_user_name, receipt_mail_data, attachment_data, attachment_count, NULL, 0, false)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_add_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	*output_receipt_mail_id = receipt_mail_data->mail_id;

FINISH_OFF:
	if(receipt_mail_data) {
		EM_SAFE_FREE(receipt_mail_data->full_address_to);
		EM_SAFE_FREE(receipt_mail_data->message_id);
		EM_SAFE_FREE(receipt_mail_data->subject);
		EM_SAFE_FREE(receipt_mail_data);
	}

	if(mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

	if(read_mail_data)
		emcore_free_mail_data_list(&read_mail_data, 1);

	if(attachment_data)
		emcore_free_attachment_data(&attachment_data, 1, NULL);

	if(receipt_mail_tbl_data)
		emstorage_free_mail(&receipt_mail_tbl_data, 1, NULL);

    if (root_body)
        mail_free_body(&root_body);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_add_meeting_request(char *multi_user_name, int account_id, int input_mailbox_id, email_meeting_request_t *meeting_req, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], input_mailbox_id[%d], meeting_req[%p], err_code[%p]", account_id, input_mailbox_id, meeting_req, err_code);
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (!meeting_req || meeting_req->mail_id <= 0) {
		if (meeting_req)
			EM_DEBUG_EXCEPTION("mail_id[%d]", meeting_req->mail_id);

		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_add_meeting_request(multi_user_name, account_id, input_mailbox_id, meeting_req, 1, &err))  {
		EM_DEBUG_EXCEPTION(" emstorage_add_meeting_request failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_query_mail_size_limit(char *multi_user_name, int account_id, int handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], err_code[%p]", account_id, err_code);
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	SENDSTREAM *stream = NULL;
	void *tmp_stream = NULL;
	int mail_size_limit = -1;
	email_account_t *ref_account = NULL;
	sslstart_t stls = NULL;
	MAILSTREAM *mail_stream = NULL;

	if (account_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(ref_account = emcore_get_account_reference(multi_user_name, account_id, false))) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/*  if there is no security option, unset security. */
	if (!ref_account->outgoing_server_secure_connection) {
		stls = (sslstart_t)mail_parameters(NULL, GET_SSLSTART, NULL);
		mail_parameters(NULL, SET_SSLSTART, NULL);
	}

	if (ref_account->pop_before_smtp != FALSE) {
		if (!emcore_connect_to_remote_mailbox(multi_user_name,
												account_id,
												0,
												true,
												(void **)&mail_stream,
												&err)) {
			EM_DEBUG_EXCEPTION(" POP before SMTP Authentication failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											account_id,
											EMAIL_CONNECT_FOR_SENDING,
											true,
											(void **)&tmp_stream,
											&err)) {
		EM_DEBUG_EXCEPTION(" emcore_connect_to_remote_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}

	stream = (SENDSTREAM *)tmp_stream;

	if (stream && stream->protocol.esmtp.ok) {
		if (stream->protocol.esmtp.size.ok && stream->protocol.esmtp.size.limit > 0) {
			EM_DEBUG_LOG("Server size limit : %ld", stream->protocol.esmtp.size.limit);
			mail_size_limit = stream->protocol.esmtp.size.limit;
		}
	}

	ret = true;

FINISH_OFF:

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

#ifndef __FEATURE_KEEP_CONNECTION__
	if (stream)
		smtp_close(stream);
#endif /* __FEATURE_KEEP_CONNECTION__ */

	if (mail_stream)
		mail_stream = mail_close (mail_stream);

	if (stls)
		mail_parameters(NULL, SET_SSLSTART, (void  *)stls);

	if (ret == true) {
		if (!emcore_notify_network_event(NOTI_QUERY_SMTP_MAIL_SIZE_LIMIT_FINISH, account_id, NULL, mail_size_limit, handle))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_QUERY_SMTP_MAIL_SIZE_LIMIT_FINISH] Failed");
	} else {
		if (!emcore_notify_network_event(NOTI_QUERY_SMTP_MAIL_SIZE_LIMIT_FAIL, account_id, NULL, handle, err))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_QUERY_SMTP_MAIL_SIZE_LIMIT_FAIL] Failed");
	}

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d], err [%d]", ret, err);
	return ret;
}

/* thread local variable for reuse */
__thread GList* g_send_stream_list = NULL;
typedef struct {
	int account_id;
	SENDSTREAM **send_stream;
} email_send_stream_list_t;

/*
stmp stream should be closed when threads exit, otherwise memory leaks
*/
INTERNAL_FUNC void emcore_close_smtp_stream_list ()
{
	EM_DEBUG_FUNC_BEGIN();
	GList* cur = g_send_stream_list;
	email_send_stream_list_t* data = NULL;

	while (cur) {
		data = cur->data;
		if (data) *(data->send_stream) = smtp_close (*(data->send_stream));
		g_send_stream_list = g_list_delete_link (g_send_stream_list, cur);
		g_free (data);
		cur = g_send_stream_list;
	}

	EM_DEBUG_FUNC_END();
}

/*
if threads exit after calling the function, emcore_close_smtp_stream_list should be called.
Otherwise, memory leaks
*/
INTERNAL_FUNC SENDSTREAM** emcore_get_smtp_stream (char *multi_user_name, int account_id, int *error)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d]", account_id);
	GList* cur = g_send_stream_list;
	email_send_stream_list_t* data = NULL;
	SENDSTREAM** ret = NULL;
	int err = EMAIL_ERROR_NONE;

	for ( ; cur ; cur = g_list_next(cur) ) {
		data = cur->data;
		if (data->account_id == account_id) {
			if (data->send_stream == NULL || *(data->send_stream) == NULL) {
				EM_DEBUG_LOG ("smtp_stream was closed before");
				g_send_stream_list = g_list_delete_link (g_send_stream_list, cur);
				g_free (data);
				break;
			}

			int reply = smtp_send ( *(data->send_stream), "NOOP", NULL);
			if (reply/100 == 2) { /* 2xx means a success */
				EM_DEBUG_LOG ("reusable smtp_stream found");
				return data->send_stream;
			}
			else {
				EM_DEBUG_LOG ("smtp_stream is not reusable");
				*(data->send_stream) = smtp_close (*(data->send_stream));
				g_send_stream_list = g_list_delete_link (g_send_stream_list, cur);
				g_free (data->send_stream);
				break;
			}
		}
	}

	ret = em_malloc (sizeof(SENDSTREAM*));
	if (!ret) {
		EM_DEBUG_EXCEPTION("em_malloc error");
		goto FINISH_OFF;
	}

	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											account_id,
											EMAIL_CONNECT_FOR_SENDING,
											true,
											(void **)ret,
											&err))  {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);

		EM_SAFE_FREE(ret);
		goto FINISH_OFF;
	}

	email_send_stream_list_t *node = em_malloc (sizeof(email_send_stream_list_t));
	if (!node) {
		EM_DEBUG_EXCEPTION ("em_malloc error");
		*ret = smtp_close (*ret);
		EM_SAFE_FREE(ret);
		goto FINISH_OFF;
	}

	node->account_id = account_id;
	node->send_stream = ret;

	g_send_stream_list = g_list_prepend (g_send_stream_list, node);

FINISH_OFF:

	if (error)
		*error = err;

	EM_DEBUG_FUNC_END();

	return ret;
}


/*
send a mail
3 threads call this function :
worker_send_event_queue
mainloop (by alarm),
thread_func_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL,
*/

INTERNAL_FUNC int emcore_send_mail(char *multi_user_name, int mail_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], err_code[%p]", mail_id, err_code);
	EM_PROFILE_BEGIN(profile_emcore_send_mail);
	int ret = false;
	int err = EMAIL_ERROR_NONE, err2 = EMAIL_ERROR_NONE;
	int attachment_tbl_count = 0;
	int account_id = 0;
	SENDSTREAM** send_stream = NULL;
	ENVELOPE *envelope = NULL;
	sslstart_t stls = NULL;
	emstorage_mail_tbl_t       *mail_tbl_data = NULL;
	emstorage_attachment_tbl_t *attachment_tbl_data = NULL;
	email_account_t *ref_account = NULL;
	email_option_t *opt = NULL;
	char *fpath = NULL;
	emstorage_mailbox_tbl_t* local_mailbox = NULL;
	emstorage_read_mail_uid_tbl_t *downloaded_mail = NULL;
	int dst_mailbox_id = 0;
	int total_mail_size = 0;
	int sent_flag = 0;
	char *server_uid = NULL;
	MAILSTREAM *mail_stream = NULL;

	if (!mail_id)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/*  get mail to send */
	if (!emstorage_get_mail_by_id(multi_user_name, mail_id, &mail_tbl_data, false, &err) || err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	account_id = mail_tbl_data->account_id;

	if (!(ref_account = emcore_get_account_reference(multi_user_name, account_id, false))) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_attachment_list(multi_user_name, mail_id, false, &attachment_tbl_data, &attachment_tbl_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_check_send_mail_thread_status()) {
		EM_DEBUG_EXCEPTION("emcore_check_send_mail_thread_status failed...");
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	if ((!mail_tbl_data->full_address_to) && (!mail_tbl_data->full_address_cc) && (!mail_tbl_data->full_address_bcc)) {
		err = EMAIL_ERROR_NO_RECIPIENT;
		EM_DEBUG_EXCEPTION("No Recipient information [%d]", err);
		goto FINISH_OFF;
	}
	else {
		if ((err = em_verify_email_address_of_mail_tbl(mail_tbl_data)) != EMAIL_ERROR_NONE) {
			err = EMAIL_ERROR_INVALID_ADDRESS;
			EM_DEBUG_EXCEPTION("em_verify_email_address_of_mail_tbl failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	if (!emcore_check_send_mail_thread_status()) {
		EM_DEBUG_EXCEPTION("emcore_check_send_mail_thread_status failed...");
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	opt = &(ref_account->options);

#ifdef __FEATURE_SUPPORT_VALIDATION_SYSTEM__
	int i = 0;
	EM_VALIDATION_SYSTEM_LOG("INFO", mail_id, "Email Send Start, %s -> %s, success", mail_tbl_data->full_address_from, mail_tbl_data->full_address_to);
	for (i = 0; i < attachment_tbl_count; i++) {
		if(attachment_tbl_data)
			EM_VALIDATION_SYSTEM_LOG("FILE", mail_id, "[%s], %d", attachment_tbl_data[i].attachment_path, attachment_tbl_data[i].attachment_size);
	}
#endif /* __FEATURE_SUPPORT_VALIDATION_SYSTEM__ */

	/*Update status flag to DB*/

	/*  get rfc822 data */
	if (!emcore_make_rfc822_file_from_mail(multi_user_name, mail_tbl_data, attachment_tbl_data, attachment_tbl_count, &envelope, &fpath, opt, &err)) {
		EM_DEBUG_EXCEPTION("emcore_make_rfc822_file_from_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!envelope || (!envelope->to && !envelope->cc && !envelope->bcc))  {
		EM_DEBUG_EXCEPTION(" no recipients found...");
		err = EMAIL_ERROR_NO_RECIPIENT;
		goto FINISH_OFF;
	}

	/*  if there is no security option, unset security. */
	if (!ref_account->outgoing_server_secure_connection)  {
		stls = (sslstart_t)mail_parameters(NULL, GET_SSLSTART, NULL);
		mail_parameters(NULL, SET_SSLSTART, NULL);
	}

	if (!emcore_check_send_mail_thread_status()) {
		EM_DEBUG_EXCEPTION("emcore_check_send_mail_thread_status failed...");
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	if (ref_account->pop_before_smtp != FALSE)  {
		if (!emcore_connect_to_remote_mailbox(multi_user_name,
												account_id,
												0,
												true,
												(void **)&mail_stream,
												&err))  {
			EM_DEBUG_EXCEPTION(" POP before SMTP Authentication failed [%d]", err);
			if (err == EMAIL_ERROR_CONNECTION_BROKEN)
				err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}
	}

	if (!emstorage_get_mailbox_by_mailbox_type(multi_user_name, account_id, EMAIL_MAILBOX_TYPE_DRAFT, &local_mailbox, false, &err))  {
		EM_DEBUG_EXCEPTION(" emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
		goto FINISH_OFF;
	}
	dst_mailbox_id = local_mailbox->mailbox_id;
	emstorage_free_mailbox(&local_mailbox, 1, NULL);

	if (!emnetwork_check_network_status(&err)) {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
		goto FINISH_OFF;
	}

	send_stream = emcore_get_smtp_stream (multi_user_name, account_id, &err);
	if (!send_stream) {
		EM_DEBUG_EXCEPTION(" emcore_get_smtp_stream failed [%d]", err);
		if (err == EMAIL_ERROR_CONNECTION_BROKEN)
			err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

#if 0
	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											account_id,
											EMAIL_CONNECT_FOR_SENDING,
											true,
											(void **)&tmp_stream,
											&err))  {
		EM_DEBUG_EXCEPTION(" emcore_connect_to_remote_mailbox failed [%d]", err);

		if (err == EMAIL_ERROR_CONNECTION_BROKEN)
			err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	stream = (SENDSTREAM *)tmp_stream;
#endif

	if (!emcore_check_send_mail_thread_status()) {
		EM_DEBUG_EXCEPTION(" emcore_check_send_mail_thread_status failed...");
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	if (*send_stream && (*send_stream)->protocol.esmtp.ok) {
		if ((*send_stream)->protocol.esmtp.size.ok && (*send_stream)->protocol.esmtp.size.limit > 0) {
			EM_DEBUG_LOG("Server size limit : %ld", (*send_stream)->protocol.esmtp.size.limit);
			emcore_get_file_size(fpath, &total_mail_size, NULL);
			EM_DEBUG_LOG("mail size : %d", total_mail_size);
			if (total_mail_size > (*send_stream)->protocol.esmtp.size.limit) {
				err = EMAIL_ERROR_SMTP_SEND_FAILURE_BY_OVERSIZE;
				goto FINISH_OFF;
			}
		}
	}

	/*  set request of delivery status. */
	EM_DEBUG_LOG("opt->req_delivery_receipt [%d]", opt->req_delivery_receipt);
	EM_DEBUG_LOG("mail_tbl_data->report_status [%d]", mail_tbl_data->report_status);

	if (opt->req_delivery_receipt == EMAIL_OPTION_REQ_DELIVERY_RECEIPT_ON || (mail_tbl_data->report_status & EMAIL_MAIL_REQUEST_DSN))  {
		EM_DEBUG_LOG("DSN is required.");
		(*send_stream)->protocol.esmtp.dsn.want = 1;
		(*send_stream)->protocol.esmtp.dsn.full = 0;
		(*send_stream)->protocol.esmtp.dsn.notify.failure = 1;
		(*send_stream)->protocol.esmtp.dsn.notify.success = 1;
	}

	mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SENDING;
	emcore_show_user_message(multi_user_name, mail_id, EMAIL_ACTION_SENDING_MAIL, EMAIL_ERROR_NONE);

	/*Update status save_status to DB*/
	if (!emstorage_set_field_of_mails_with_integer_value(multi_user_name, account_id, &mail_id, 1, "save_status", mail_tbl_data->save_status, true, &err))
		EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err);

	/*  send mail to server. */
	if (!emcore_send_mail_smtp(multi_user_name, *send_stream, envelope, fpath, account_id, mail_id, &err)) {
		EM_DEBUG_EXCEPTION(" emcore_send_mail_smtp failed [%d]", err);
		if (err == SMTP_RESPONSE_EXCEED_SIZE_LIMIT)
			err = EMAIL_ERROR_SMTP_SEND_FAILURE_BY_OVERSIZE;

#ifndef __FEATURE_MOVE_TO_OUTBOX_FIRST__
		if (!emstorage_get_mailbox_by_mailbox_type(multi_user_name, account_id, EMAIL_MAILBOX_TYPE_OUTBOX, &local_mailbox, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
			goto FINISH_OFF;
		}
		dst_mailbox_id = local_mailbox->mailbox_id;
		emstorage_free_mailbox(&local_mailbox, 1, NULL);

		/*  unsent mail is moved to 'OUTBOX'. */
		if (!emcore_move_mail(multi_user_name, &mail_id, 1, dst_mailbox_id, EMAIL_MOVED_BY_COMMAND, 0, NULL))
			EM_DEBUG_EXCEPTION(" emcore_mail_move falied...");
#endif
		if (err > 0)
			err = EMAIL_ERROR_SMTP_SEND_FAILURE;
		goto FINISH_OFF;
	}

	emcore_show_user_message(multi_user_name, mail_id, EMAIL_ACTION_SEND_MAIL, err);
	sent_flag = 1;

	/*  sent mail is moved to 'SENT' box or deleted. */
	if (opt->keep_local_copy)  {
		if (!emstorage_get_mailbox_by_mailbox_type(multi_user_name,
													account_id,
													EMAIL_MAILBOX_TYPE_SENTBOX,
													&local_mailbox,
													true,
													&err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
			goto FINISH_OFF;
		}
		dst_mailbox_id = local_mailbox->mailbox_id;

		if (!emcore_move_mail(multi_user_name, &mail_id, 1, dst_mailbox_id, EMAIL_MOVED_AFTER_SENDING, 0, &err))
			EM_DEBUG_EXCEPTION(" emcore_mail_move falied [%d]", err);
#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
#ifdef __FEATURE_LOCAL_ACTIVITY__
		else if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) /* To be synced to Sent box only IMAP not for POP */ {
			emstorage_activity_tbl_t new_activity;
			int activityid = 0;

			if (false == emcore_get_next_activity_id(&activityid, &err)) {
				EM_DEBUG_EXCEPTION(" emcore_get_next_activity_id Failed [%d] ", err);
			}

			memset(&new_activity, 0x00, sizeof(emstorage_activity_tbl_t));
			new_activity.activity_id  =  activityid;
			new_activity.server_mailid = NULL;
			new_activity.account_id	= account_id;
			new_activity.mail_id	= mail_id;
			new_activity.activity_type = ACTIVITY_SAVEMAIL;
			new_activity.dest_mbox	= NULL;
			new_activity.src_mbox	= NULL;

			if (!emcore_add_activity(&new_activity, &err)) {
				EM_DEBUG_EXCEPTION(" emcore_add_activity Failed [%d] ", err);
			}

			if (!emcore_move_mail_on_server(multi_user_name, dest_mbox.account_id, dst_mailbox_id, &mail_id, 1, dest_mbox.name, &err)) {
				EM_DEBUG_EXCEPTION(" emcore_move_mail_on_server falied [%d]", err);
			}
			else {
				/* Remove ACTIVITY_SAVEMAIL activity */
				new_activity.activity_id  =  activityid;
				new_activity.activity_type = ACTIVITY_SAVEMAIL;
				new_activity.account_id	= account_id;
				new_activity.mail_id	= mail_id;
				new_activity.dest_mbox	= NULL;
				new_activity.server_mailid = NULL;
				new_activity.src_mbox	= NULL;

				if (!emcore_delete_activity(&new_activity, &err)) {
					EM_DEBUG_EXCEPTION(">>>>>>Local Activity [ACTIVITY_SAVEMAIL] [%d] ", err);
				}
			}
			sent_box = 1;
		}
#endif
#endif
		if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
			emstorage_mailbox_tbl_t* src_mailbox = NULL;
//			emstorage_mail_tbl_t *temp_mail = NULL;

			if (!emstorage_get_mailbox_by_mailbox_type(multi_user_name,
														account_id,
														EMAIL_MAILBOX_TYPE_OUTBOX,
														&src_mailbox,
														true,
														&err))  {
				EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
				goto FINISH_OFF;
			}

			EM_DEBUG_LOG("local_yn:[%d]", src_mailbox->local_yn);
			if (src_mailbox->local_yn) {
				/* This is syncing operation in sent box
				   but it slowed operation */
				/*
				void *local_stream = NULL;
				if (!emcore_sync_header (multi_user_name, local_mailbox, &local_stream, NULL, NULL, NULL, NULL, NULL, 0, -1, &err)) {
					EM_DEBUG_EXCEPTION("emcore_sync_header failed");
				}
				mail_close (local_stream);
				if (!emstorage_get_maildata_by_servermailid(multi_user_name,
															"0",
															local_mailbox->mailbox_id,
															&temp_mail,
															false,
															&err)) {
					if (err != EMAIL_ERROR_MAIL_NOT_FOUND) {
						EM_DEBUG_EXCEPTION("emstorage_get_maildata_by_servermailid failed : [%d]", err);
						goto FINISH_OFF;
					}
				}

				if (temp_mail) {
					emcore_sync_mail_from_client_to_server(multi_user_name, mail_id);
					emstorage_free_mail(&temp_mail, 1, NULL);
				}
				*/

				/* sent box exception list : gmail (After the mail sent, moved the sent box) */
				if (ref_account->outgoing_server_address) {
					if (!strcasestr(ref_account->outgoing_server_address, "gmail")) {
						emcore_sync_mail_from_client_to_server(multi_user_name, mail_id);
					} else {
						err = emcore_sync_mail_by_message_id(multi_user_name,
																mail_id,
																dst_mailbox_id,
																&server_uid);
						if (err != EMAIL_ERROR_NONE) {
							EM_DEBUG_EXCEPTION("emcore_sync_mail_by_message_id failed : [%d]", err);
						}

						EM_DEBUG_LOG("server_uid : [%s]", server_uid);

						if (server_uid) {
							if (!emstorage_update_server_uid(multi_user_name,
																mail_id,
																NULL,
																server_uid,
																&err)) {
								EM_DEBUG_EXCEPTION("emstorage_update_server_uid failed : [%d]", err);
							}

							downloaded_mail = em_malloc(sizeof(emstorage_read_mail_uid_tbl_t));
							if (downloaded_mail == NULL) {
								EM_DEBUG_EXCEPTION("em_malloc failed");
								err = EMAIL_ERROR_OUT_OF_MEMORY;
								goto FINISH_OFF;
							}

							downloaded_mail->account_id = account_id;
							downloaded_mail->mailbox_id = dst_mailbox_id;
							downloaded_mail->local_uid = mail_id;
							downloaded_mail->mailbox_name = g_strdup(local_mailbox->mailbox_name);
							downloaded_mail->server_uid = g_strdup(server_uid);
							downloaded_mail->rfc822_size = mail_tbl_data->mail_size;

							if (!emstorage_add_downloaded_mail(multi_user_name,
																downloaded_mail,
																true,
																&err)) {
								EM_DEBUG_EXCEPTION("emstorage_add_downloaded_mail failed : [%d]", err);
							}
						}
					}
				}
			} else {
				if (!emcore_move_mail_on_server(multi_user_name,
												account_id,
												src_mailbox->mailbox_id,
												&mail_id,
												1,
												local_mailbox->mailbox_name,
												&err)) {
					EM_DEBUG_EXCEPTION(" emcore_move_mail_on_server falied [%d]", err);
				}
			}

			emstorage_free_mailbox(&src_mailbox, 1, NULL);
		}

		/* On Successful Mail sent remove the Draft flag */
		mail_tbl_data->flags_draft_field = 0;

		if (!emstorage_set_field_of_mails_with_integer_value(multi_user_name,
															account_id,
															&mail_id,
															1,
															"flags_draft_field",
															mail_tbl_data->flags_draft_field,
															true,
															&err))
			EM_DEBUG_EXCEPTION("Failed to modify extra flag [%d]", err);
	} else {
		if (!emcore_delete_mail(multi_user_name,
								account_id,
								0,
								&mail_id,
								1,
								EMAIL_DELETE_LOCALLY,
								EMAIL_DELETED_AFTER_SENDING,
								false,
								&err))
			EM_DEBUG_EXCEPTION(" emcore_delete_mail failed [%d]", err);
	}

	/* Set the phone log */
	if ((err = emcore_set_sent_contacts_log(multi_user_name, mail_tbl_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_set_sent_contacts_log failed : [%d]", err);
	}

	/*Update save_status */
	mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SENT;
	if (!emstorage_set_field_of_mails_with_integer_value(multi_user_name,
														account_id,
														&mail_id,
														1,
														"save_status",
														mail_tbl_data->save_status,
														true,
														&err))
		EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err);

	if (!emcore_delete_transaction_info_by_mailId(mail_id))
		EM_DEBUG_EXCEPTION(" emcore_delete_transaction_info_by_mailId failed for mail_id[%d]", mail_id);

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(server_uid);

	if (ret == false && sent_flag == 0) {
		emcore_show_user_message(multi_user_name, mail_id, EMAIL_ACTION_SEND_MAIL, err);
	}

	if (ret == false && err != EMAIL_ERROR_INVALID_PARAM && mail_tbl_data)  {
		if (err != EMAIL_ERROR_CANCELLED) {
			mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SEND_FAILURE;
			if (!emstorage_set_field_of_mails_with_integer_value(multi_user_name, account_id, &mail_id, 1, "save_status", mail_tbl_data->save_status, true, &err2))
				EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err2);
		}
		else {
			if (EMAIL_MAIL_STATUS_SEND_CANCELED == mail_tbl_data->save_status)
				EM_DEBUG_LOG("EMAIL_MAIL_STATUS_SEND_CANCELED Already set for ");
			else {
				mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SEND_CANCELED;
				if (!emstorage_set_field_of_mails_with_integer_value(multi_user_name,
																	account_id,
																	&mail_id,
																	1,
																	"save_status",
																	mail_tbl_data->save_status,
																	true,
																	&err2))
					EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err2);
			}
		}
	}

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

#if 0
#ifndef __FEATURE_KEEP_CONNECTION__
	if (stream)
		smtp_close(stream);
#endif /* __FEATURE_KEEP_CONNECTION__ */
#endif

	if (downloaded_mail)
		emstorage_free_read_mail_uid(&downloaded_mail, 1, NULL);

	if (mail_stream)
		mail_stream = mail_close (mail_stream);

	if (stls)
		mail_parameters(NULL, SET_SSLSTART, (void  *)stls);

	if (attachment_tbl_data)
		emstorage_free_attachment(&attachment_tbl_data, attachment_tbl_count, NULL);

	if (envelope)
		mail_free_envelope(&envelope);

	if (fpath) {
		EM_DEBUG_LOG_SEC("REMOVE TEMP FILE  :  %s", fpath);
		remove(fpath);
		EM_SAFE_FREE (fpath);
	}

	if (local_mailbox)
		emstorage_free_mailbox(&local_mailbox, 1, NULL);

	if (ret == true) {
		if (!emcore_notify_network_event(NOTI_SEND_FINISH, account_id, NULL, mail_id, 0))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_SEND_FINISH] Failed");
#ifdef __FEATURE_SUPPORT_VALIDATION_SYSTEM__
		if(mail_tbl_data)
			EM_VALIDATION_SYSTEM_LOG("INFO", mail_id, "Email Send End, %s -> %s, success", mail_tbl_data->full_address_from, mail_tbl_data->full_address_to);
#endif
	}
	else {
		if (!emcore_notify_network_event(NOTI_SEND_FAIL, account_id, NULL, mail_id, err))
			EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_SEND_FAIL] Failed");
#ifdef __FEATURE_SUPPORT_VALIDATION_SYSTEM__
		if(mail_tbl_data)
			EM_VALIDATION_SYSTEM_LOG("INFO", mail_id, "Email Send End, %s -> %s, failed", mail_tbl_data->full_address_from, mail_tbl_data->full_address_to);
#endif

		if (err != EMAIL_ERROR_SMTP_SEND_FAILURE_BY_OVERSIZE) {
			/* Add alarm for next sending mails */
			if( (err2 = emcore_create_alarm_for_auto_resend(multi_user_name, AUTO_RESEND_INTERVAL)) != EMAIL_ERROR_NONE) {
				if (err2 != EMAIL_ERROR_MAIL_NOT_FOUND)
					EM_DEBUG_EXCEPTION("emcore_create_alarm_for_auto_resend failed [%d]", err2);
			}
		}
	}

	if (mail_tbl_data)
		emstorage_free_mail(&mail_tbl_data, 1, NULL);

	if (err_code != NULL)
		*err_code = err;
	EM_PROFILE_END(profile_emcore_send_mail);
	EM_DEBUG_FUNC_END("ret [%d], err [%d]", ret, err);
	return ret;
}

/*  send a saved all mails */
INTERNAL_FUNC int emcore_send_saved_mail(char *multi_user_name, int account_id, char *input_mailbox_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], input_mailbox_name[%p], err_code[%p]", account_id, input_mailbox_name, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int err2 = EMAIL_ERROR_NONE;
	int status = EMAIL_SEND_FAIL;
	int *mail_ids = NULL;
	DB_STMT handle = NULL;
	int i = 0;
	int total = 0;
	int attachment_tbl_count = 0;
	char *fpath = NULL;
	SENDSTREAM *stream = NULL;
	ENVELOPE *envelope = NULL;
	email_account_t *ref_account = NULL;
	emstorage_mail_tbl_t       *searched_mail_tbl_data = NULL;
	emstorage_attachment_tbl_t *attachment_tbl_data    = NULL;
	email_option_t *opt = NULL;
	sslstart_t	stls = NULL;
	void *tmp_stream = NULL;
	emstorage_mailbox_tbl_t* local_mailbox = NULL;
	int dst_mailbox_id = 0;

	if (!account_id || !input_mailbox_name)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}


	if (!(ref_account = emcore_get_account_reference(multi_user_name, account_id, false)))  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

/*don't delete the comment. several threads including event thread call this func */
/*	FINISH_OFF_IF_CANCELED; */

	if (!emnetwork_check_network_status(&err)) {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
		goto FINISH_OFF;
	}

	opt = &(ref_account->options);

	if (!emstorage_get_mailbox_by_name(multi_user_name, account_id, -1, input_mailbox_name, &local_mailbox, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_name failed : [%d]", err);
		goto FINISH_OFF;
	}

	/*  search mail. */
	if (!emstorage_mail_search_start(multi_user_name, NULL, account_id, local_mailbox->mailbox_id, 0, &handle, &total, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_mail_search_start failed [%d]", err);
		goto FINISH_OFF;
	}

	mail_ids = em_malloc(sizeof(int) * total);
	if (mail_ids == NULL) {
		EM_DEBUG_EXCEPTION("malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < total ; i++) {
		if (!emstorage_mail_search_result(handle, RETRIEVE_ID, (void **)&mail_ids[i], true, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_mail_search_result failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	if (!emstorage_mail_search_end(handle, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_mail_search_end failed [%d]", err);
		goto FINISH_OFF;
	}

	handle = 0;

	mail_send_notify(EMAIL_SEND_PREPARE, 0, 0, account_id, mail_ids[total], err);

	if(local_mailbox)
		emstorage_free_mailbox(&local_mailbox, 1, NULL);

	for (i = 0; i < total; i++) {
/*don't delete the comment. several threads including event thread call this func */
/*		FINISH_OFF_IF_CANCELED;*/

		if (!emstorage_get_mail_by_id(multi_user_name, mail_ids[i], &searched_mail_tbl_data, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
			goto FINISH_OFF;
		}

		/* Skip the mail canceled */
		if (searched_mail_tbl_data->save_status == EMAIL_MAIL_STATUS_SEND_CANCELED)  {
			EM_DEBUG_EXCEPTION("The mail was canceled. [%d]", mail_ids[i]);
			emstorage_free_mail(&searched_mail_tbl_data, 1, &err);
			searched_mail_tbl_data = NULL;
			continue;
		}

		if ( (err = emstorage_get_attachment_list(multi_user_name, mail_ids[i], false, &attachment_tbl_data, &attachment_tbl_count)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);
			goto FINISH_OFF;
		}

		/* check for email_address validation */
		if ( (err = em_verify_email_address_of_mail_tbl(searched_mail_tbl_data)) != EMAIL_ERROR_NONE ) {
			err = EMAIL_ERROR_INVALID_ADDRESS;
			EM_DEBUG_EXCEPTION("em_verify_email_address_of_mail_tbl failed [%d]", err);
			goto FINISH_OFF;
		}

		searched_mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SENDING;

		if (!emcore_make_rfc822_file_from_mail(multi_user_name, searched_mail_tbl_data, attachment_tbl_data, attachment_tbl_count, &envelope, &fpath, opt, &err))  {
			EM_DEBUG_EXCEPTION("emcore_make_rfc822_file_from_mail falied [%d]", err);
			goto FINISH_OFF;
		}

/*don't delete the comment. several threads including event thread call this func */
/*		FINISH_OFF_IF_CANCELED;*/

		/*  connect mail server. */
		if (!stream) {
			/*  if there no security option, unset security. */
			if (!ref_account->outgoing_server_secure_connection) {
				stls = (sslstart_t)mail_parameters(NULL, GET_SSLSTART, NULL);
				mail_parameters(NULL, SET_SSLSTART, NULL);
			}

			stream = NULL;
			if (!emcore_connect_to_remote_mailbox(multi_user_name,
													account_id,
													EMAIL_CONNECT_FOR_SENDING,
													true,
													&tmp_stream,
													&err) || !tmp_stream)  {
				EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);

				if (err == EMAIL_ERROR_CONNECTION_BROKEN)
					err = EMAIL_ERROR_CANCELLED;

				status = EMAIL_SEND_CONNECTION_FAIL;
				goto FINISH_OFF;
			}

			stream = (SENDSTREAM *)tmp_stream;

/*don't delete the comment. several threads including event thread call this func */
/*			FINISH_OFF_IF_CANCELED;*/

			mail_send_notify(EMAIL_SEND_CONNECTION_SUCCEED, 0, 0, account_id, mail_ids[i], err);

			/*  reqest of delivery status. */
			if (opt && opt->req_delivery_receipt == EMAIL_OPTION_REQ_DELIVERY_RECEIPT_ON)  {
				stream->protocol.esmtp.dsn.want = 1;
				stream->protocol.esmtp.dsn.full = 0;
				stream->protocol.esmtp.dsn.notify.failure = 1;
				stream->protocol.esmtp.dsn.notify.success = 1;
			}

			mail_send_notify(EMAIL_SEND_START, 0, 0, account_id, mail_ids[i], err);
		}

		searched_mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SENDING;

		/*  update mail status to sending. */
		if (!emstorage_change_mail_field(multi_user_name, mail_ids[i], UPDATE_EXTRA_FLAG, searched_mail_tbl_data, true, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);

			goto FINISH_OFF;
		}

		if (!emcore_send_mail_smtp(multi_user_name, stream, envelope, fpath, account_id, mail_ids[i], &err))  {
			EM_DEBUG_EXCEPTION("emcore_send_mail_smtp failed [%d]", err);
			if (err == SMTP_RESPONSE_EXCEED_SIZE_LIMIT) err = EMAIL_ERROR_SMTP_SEND_FAILURE_BY_OVERSIZE;

			searched_mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SEND_FAILURE;

			/*  update mail status to failure. */
			if (!emstorage_change_mail_field(multi_user_name, mail_ids[i], UPDATE_EXTRA_FLAG, searched_mail_tbl_data, true, &err2))
				EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);

			if (!emstorage_get_mailbox_by_mailbox_type(multi_user_name,
														account_id,
														EMAIL_MAILBOX_TYPE_OUTBOX,
														&local_mailbox,
														false,
														&err))  {
				EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
				goto FINISH_OFF;
			}
			dst_mailbox_id = local_mailbox->mailbox_id;

			emcore_move_mail(multi_user_name, &mail_ids[i], 1, dst_mailbox_id, EMAIL_MOVED_AFTER_SENDING, 0, NULL);

			if(local_mailbox)
				emstorage_free_mailbox(&local_mailbox, 1, NULL);

			goto FINISH_OFF;
		}

		searched_mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SENT;

		/*  update mail status to sent mail. */
		if (!emstorage_change_mail_field(multi_user_name,
											mail_ids[i],
											UPDATE_EXTRA_FLAG,
											searched_mail_tbl_data,
											true,
											&err))  {
			EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);
			goto FINISH_OFF;
		}

		/*  sent mail is moved to 'SENT' box or deleted. */
		if (opt->keep_local_copy) {
			if (!emstorage_get_mailbox_by_mailbox_type(multi_user_name,
														account_id,
														EMAIL_MAILBOX_TYPE_SENTBOX,
														&local_mailbox,
														false,
														&err))  {
				EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
				goto FINISH_OFF;
			}

			dst_mailbox_id = local_mailbox->mailbox_id;
			if (!emcore_move_mail(multi_user_name,
									&mail_ids[i],
									1,
									dst_mailbox_id,
									EMAIL_MOVED_AFTER_SENDING,
									0,
									&err))
				EM_DEBUG_EXCEPTION("emcore_mail_move falied [%d]", err);

			if(local_mailbox)
				emstorage_free_mailbox(&local_mailbox, 1, NULL);
		} else {
			if (!emcore_delete_mail(multi_user_name,
									account_id,
									0,
									&mail_ids[i],
									1,
									EMAIL_DELETE_LOCALLY,
									EMAIL_DELETED_AFTER_SENDING,
									false,
									&err))
				EM_DEBUG_EXCEPTION("emcore_delete_mail falied [%d]", err);
		}

		/* Set the phone log */
		if ((err = emcore_set_sent_contacts_log(multi_user_name, searched_mail_tbl_data)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_set_sent_contacts_log failed : [%d]", err);
		}

		if (searched_mail_tbl_data) {
			emstorage_free_mail(&searched_mail_tbl_data, 1, NULL);
			searched_mail_tbl_data = NULL;
		}

		if(attachment_tbl_data)
			emstorage_free_attachment(&attachment_tbl_data, attachment_tbl_count, NULL);

		mail_free_envelope(&envelope); envelope = NULL;

		if (fpath)  {
			remove(fpath);
			EM_SAFE_FREE(fpath);
		}
	}


	ret = true;

FINISH_OFF:

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (stream)
		smtp_close(stream);

	if (stls)
		mail_parameters(NIL, SET_SSLSTART, (void  *)stls);

	if (envelope)
		mail_free_envelope(&envelope);

	if (handle)  {
		if (!emstorage_mail_search_end(handle, true, &err2))
			EM_DEBUG_EXCEPTION("emstorage_mail_search_end failed [%d]", err2);
	}

	if (searched_mail_tbl_data)
		emstorage_free_mail(&searched_mail_tbl_data, 1, NULL);

	if(attachment_tbl_data)
		emstorage_free_attachment(&attachment_tbl_data, attachment_tbl_count, NULL);

	if (fpath)  {
		remove(fpath);
		EM_SAFE_FREE(fpath);
	}

	if(local_mailbox)
		emstorage_free_mailbox(&local_mailbox, 1, NULL);

	if (ret == true) {
		mail_send_notify(EMAIL_SEND_FINISH, 0, 0, account_id, mail_ids[total], err);
		emcore_show_user_message(multi_user_name, account_id, EMAIL_ACTION_SEND_MAIL, err);
	} else {
		if(mail_ids) /* prevent 34385 */
			mail_send_notify(status, 0, 0, account_id, mail_ids[total], err);
		emcore_show_user_message(multi_user_name, account_id, EMAIL_ACTION_SEND_MAIL, err);
	}

	EM_SAFE_FREE(mail_ids);

	if (err_code != NULL)
		*err_code = err;

	return ret;
}

static int emcore_send_mail_smtp(char *multi_user_name,
									SENDSTREAM *stream,
									ENVELOPE *env,
									char *data_file,
									int account_id,
									int mail_id,
									int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("stream[%p], env[%p], data_file[%s], account_id[%d], mail_id[%d], err_code[%p]",
								stream, env, data_file, account_id, mail_id, err_code);
	EM_PROFILE_BEGIN(profile_emcore_send_mail_smtp);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int recipients = 0;
	long total = 0, sent = 0, send_ret = 0, send_err = 0, sent_percent = 0, last_sent_percent = 0;
	char buf[2048] = { 0, };
	email_account_t *ref_account = NULL;
	FILE *fp = NULL;

	if (!env || !env->from || (!env->to && !env->cc && !env->bcc)) {
		if (env != NULL)
			EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!env->from->mailbox || !env->from->host) {
		EM_DEBUG_EXCEPTION("env->from->mailbox[%p], env->from->host[%p]", env->from->mailbox, env->from->host);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(ref_account = emcore_get_account_reference(multi_user_name, account_id, false))) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("Modifying - MAIL FROM ");
	if (ref_account->user_email_address == NULL) {
		EM_DEBUG_LOG("ref_account->user_email_address is null!!");
		SNPRINTF(buf, sizeof(buf), "FROM:<%s@%s>", env->from->mailbox, env->from->host);
	}
	else
		SNPRINTF(buf, sizeof(buf), "FROM:<%s>", ref_account->user_email_address);

	/*  set DSN for ESMTP */
	if (stream->protocol.esmtp.ok) {
		if (stream->protocol.esmtp.eightbit.ok && stream->protocol.esmtp.eightbit.want)
			strncat (buf, " BODY=8BITMIME", sizeof(buf)-(EM_SAFE_STRLEN(buf)+1));

		EM_DEBUG_LOG("stream->protocol.esmtp.dsn.ok [%d]", stream->protocol.esmtp.dsn.ok);

		if (stream->protocol.esmtp.dsn.ok && stream->protocol.esmtp.dsn.want) {
			EM_DEBUG_LOG("stream->protocol.esmtp.dsn.want is required");
			strncat(buf, stream->protocol.esmtp.dsn.full ? " RET=FULL" : " RET=HDRS",
					sizeof(buf)-EM_SAFE_STRLEN(buf)-1);

			if (stream->protocol.esmtp.dsn.envid)
				SNPRINTF (buf + EM_SAFE_STRLEN (buf), sizeof(buf)-(EM_SAFE_STRLEN(buf)),
							" ENVID=%.100s", stream->protocol.esmtp.dsn.envid);
		}
		else
			EM_DEBUG_LOG("stream->protocol.esmtp.dsn.want is not required or DSN is not supported");
	}

	EM_PROFILE_BEGIN(profile_prepare_and_head);
	send_ret = smtp_send(stream, "RSET", 0);
	EM_DEBUG_LOG("[SMTP] RSET --------> %s", stream->reply);

	if (send_ret != SMTP_RESPONSE_OK) {
		err = send_ret;
		goto FINISH_OFF;
	}

	send_ret = smtp_send(stream, "MAIL", buf);
	EM_DEBUG_LOG("[SMTP] MAIL %s --------> %s", buf, stream->reply);

	switch (send_ret) {
		case SMTP_RESPONSE_OK:
			break;

		case SMTP_RESPONSE_WANT_AUTH  :
		case SMTP_RESPONSE_WANT_AUTH2:
			EM_DEBUG_EXCEPTION("SMTP error : authentication required...");
			err = EMAIL_ERROR_AUTH_REQUIRED;
			goto FINISH_OFF;

		case SMTP_RESPONSE_UNAVAIL:
			EM_DEBUG_EXCEPTION("SMTP error : sending unavailable...");
			err = EMAIL_ERROR_SMTP_SEND_FAILURE;
			goto FINISH_OFF;
		case SMTP_RESPONSE_CONNECTION_BROKEN:
			EM_DEBUG_EXCEPTION("SMTP error : SMTP connection broken...");
			err = EMAIL_ERROR_SMTP_SEND_FAILURE;
			goto FINISH_OFF;
		default:
			EM_DEBUG_EXCEPTION("SMTP error : sending unavailable...");
			err = EMAIL_ERROR_SMTP_SEND_FAILURE;
			goto FINISH_OFF;
	}

	if (env->to) {
		send_ret = smtp_rcpt(stream, env->to, &send_err);
		EM_DEBUG_LOG_SEC("[SMTP] RCPT TO : <%s@%s> ... --------> %s", env->to->mailbox, env->to->host, env->to->error ? env->to->error  :  stream->reply);
		if (send_ret) {
			err = stream->replycode;
			goto FINISH_OFF;
		}

		if (!send_err)
			recipients++;
	}

	if (env->cc) {
		send_ret = smtp_rcpt(stream, env->cc, &send_err);
		EM_DEBUG_LOG_SEC("[SMTP] RCPT TO : <%s@%s> ... --------> %s", env->cc->mailbox, env->cc->host, env->cc->error ? env->cc->error  :  stream->reply);
		if (send_ret) {
			err = stream->replycode;
			goto FINISH_OFF;
		}

		if (!send_err)
			recipients++;
	}

	if (env->bcc) {
		send_ret = smtp_rcpt(stream, env->bcc, &send_err);
		EM_DEBUG_LOG_SEC("[SMTP] RCPT TO : <%s@%s> ... --------> %s", env->bcc->mailbox, env->bcc->host, env->bcc->error ? env->bcc->error  :  stream->reply);
		if (send_ret) {
			err = stream->replycode;
			goto FINISH_OFF;
		}

		if (!send_err)
			recipients++;
	}

	if (send_err) {
		EM_DEBUG_EXCEPTION("One or more recipients failed...");
		err = EMAIL_ERROR_INVALID_ADDRESS;
	}

	if (!recipients) {
		EM_DEBUG_EXCEPTION("No valid recipients...");

		switch (stream->replycode) {
			case SMTP_RESPONSE_UNAVAIL:
			case SMTP_RESPONSE_WANT_AUTH  :
			case SMTP_RESPONSE_WANT_AUTH2:
				err = EMAIL_ERROR_AUTH_REQUIRED;
				break;

			default:
				err = EMAIL_ERROR_INVALID_ADDRESS;
				break;
		}
		goto FINISH_OFF;
	}

	send_ret = smtp_send(stream, "DATA", 0);
	EM_DEBUG_LOG_SEC("[SMTP] DATA --------> %s", stream->reply);
	EM_PROFILE_END(profile_prepare_and_head);

	if (send_ret != SMTP_RESPONSE_READY) {
		err = send_ret;
		goto FINISH_OFF;
	}

	if (data_file) {
		EM_PROFILE_BEGIN(profile_open_file);

		err = em_fopen(data_file, "r+", &fp);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_fopen(\"%s\") failed..., error:[%d]", data_file, err);
			goto FINISH_OFF;
		}
		EM_PROFILE_END(profile_open_file);

#ifdef __FEATURE_SEND_OPTMIZATION__
		char *data = NULL;
		int read_size, allocSize, dataSize, gMaxAllocSize = 40960; /*  40KB */
		int total_fixed = 0;
		fseek(fp, 0, SEEK_END);
		total = ftell(fp);
		total_fixed = total;
		fseek(fp, 0, SEEK_SET);
		EM_DEBUG_LOG("total size [%d]", total);

		if (total < gMaxAllocSize)
			allocSize = total + 1;
		else
			allocSize = gMaxAllocSize;

		EM_PROFILE_BEGIN(profile_allocation);
		/* Allocate a buffer of max 2MB to read from file */
		data = (char *)em_malloc(allocSize);
		allocSize--;
		EM_PROFILE_END(profile_allocation);

		if (NULL == data) {
			err = EMAIL_ERROR_SMTP_SEND_FAILURE;
			goto FINISH_OFF;
		}

		while (total) {
			/* Cancel the sending event */
			if (!emcore_check_send_mail_thread_status()) {
				EM_SAFE_FREE(data);
				EM_DEBUG_EXCEPTION(" emcore_check_send_mail_thread_status failed...");
				err = EMAIL_ERROR_CANCELLED;
				goto FINISH_OFF;
			}

			if (total < allocSize)
				dataSize = total;
			else
				dataSize = allocSize;

			memset(data, 0x0, dataSize+1);
			read_size = fread(data, sizeof (char), dataSize, fp);

			if (read_size != dataSize) {
				/* read fail. */
				EM_SAFE_FREE(data);
				EM_DEBUG_EXCEPTION("Read from file failed");
				err = EMAIL_ERROR_SMTP_SEND_FAILURE;
				goto FINISH_OFF;
			}
			sent += read_size;

			if (!(send_ret = smtp_soutr_test(stream->netstream, data))) {
				EM_SAFE_FREE(data);
				EM_DEBUG_EXCEPTION("Failed to send the data ");
				err = EMAIL_ERROR_SMTP_SEND_FAILURE;
				goto FINISH_OFF;
			}
			else {
				sent_percent = (int) ((double)sent / (double)total_fixed * 100.0);
				if (last_sent_percent + 5 <= sent_percent) {
					double progress = (double)sent / (double)total_fixed;
					if (!emcore_notify_network_event(NOTI_SEND_START, account_id, NULL, mail_id, sent_percent))
						EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_SEND_START] Failed >>>>");
					last_sent_percent = sent_percent;

					emcore_update_notification_for_send(account_id, mail_id, progress);
				}
				EM_DEBUG_LOG("Sent data Successfully. sent[%d] total[%d] datasize[%d]",
								sent, total, dataSize);
			}
			total -= dataSize;
		}

		EM_SAFE_FREE(data);
	}
#else
		fseek(fp, 0, SEEK_END);
		total = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		while (fgets(buf, 1024, fp)) {
#ifdef FEATURE_SEND_DATA_DEBUG
			EM_DEBUG_LOG("%s", buf);
#endif
			sent += EM_SAFE_STRLEN(buf);

			if (!(send_ret = smtp_soutr(stream->netstream, buf)))
				break;
			/*  Sending Progress Notification */
			sent_percent = (int) ((double)sent / (double)total * 100.0);
			if (last_sent_percent + 5 <= sent_percent) {
				/* Disabled Temporary
				if (!emcore_notify_network_event(NOTI_SEND_START, account_id, NULL, mail_id, sent_percent))
					EM_DEBUG_EXCEPTION(" emcore_notify_network_event [NOTI_SEND_START] Failed >>>>");
				*/
				last_sent_percent = sent_percent;
			}
		}

		if (!send_ret) {
			EM_DEBUG_EXCEPTION("smtp_soutr failed - %ld", send_ret);
			err = EMAIL_ERROR_SMTP_SEND_FAILURE;
			goto FINISH_OFF;
		}
	}
#endif

	send_ret = smtp_send(stream, ".", 0);
	EM_DEBUG_LOG("[SMTP] . --------> %s", stream->reply);

	if (send_ret != SMTP_RESPONSE_OK) {
		err = send_ret;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (ret == false)
		smtp_send(stream, "RSET", 0);

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (err_code)
		*err_code = err;

	if (fp)
		fclose(fp);

	EM_PROFILE_END(profile_emcore_send_mail_smtp);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/* ------ rfc822 handle --------------------------------------------------- */
#define RANDOM_NUMBER_LENGTH 35

char *emcore_generate_content_id_string(const char *hostname, int *err)
{
	EM_DEBUG_FUNC_BEGIN("hostname[%p]", hostname);

	if (!hostname) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err)
			*err = EMAIL_ERROR_INVALID_PARAM;
		return NULL;
	}

	int cid_length = RANDOM_NUMBER_LENGTH + EM_SAFE_STRLEN(hostname) + 2, random_number_1, random_number_2, random_number_3, random_number_4;
	char *cid_string = NULL;

	cid_string = malloc(cid_length);

	if (!cid_string) {
		if (err)
			*err = EMAIL_ERROR_OUT_OF_MEMORY;
		return NULL;
	}

	memset(cid_string, 0, cid_length);

	srand(time(NULL) + rand());
	random_number_1 = rand() * rand();
	random_number_2 = rand() * rand();
	random_number_3 = rand() * rand();
	random_number_4 = rand() * rand();

	SNPRINTF(cid_string, cid_length, "<%08x%08x%08x%08x@%s>", random_number_1, random_number_2, random_number_3, random_number_4, hostname);

	if (err)
		*err = EMAIL_ERROR_NONE;

	EM_DEBUG_FUNC_END("cid_string [%s]", cid_string);
	return cid_string;
}


/* ------ attach_part ----------------------------------------------------- */
/*  data  :  if filename NULL, content data. */
/*		else absolute path of file to be attached. */
/*  data_len  :  length of data. if filename not NULL, ignored. */
/*  file_name :  attahcment name. */
static int attach_part (BODY *body, const unsigned char *data, int data_len,
                                   char *filename, char *content_sub_type, int is_inline, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("body[%p], data[%s], data_len[%d], filename[%s], content_sub_type[%s], err_code[%p]", body, data, data_len, filename, content_sub_type, err_code);

	int        ret = false;
	int        error = EMAIL_ERROR_NONE;
	int        has_special_character = 0;
	int        base64_file_name_length = 0;
	int        i= 0;
	int        mail_type = EMAIL_SMIME_NONE;
	gsize      bytes_read;
	gsize      bytes_written;
	char      *encoded_file_name = NULL;
	char      *extension = NULL;
	char      *base64_file_name = NULL;
	char      *result_file_name = NULL;
	char       content_disposition[100] = { 0, };
	PARAMETER *last_param = NULL;
	PARAMETER *param = NULL;
	PART      *last_part = NULL;
	PART      *part = NULL;
	SIZEDTEXT  source_text;
	GError    *glib_error = NULL;
	CHARSET   *result_charset = NULL;

	if (!body)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (body->nested.part)  {
		last_part = body->nested.part;

		if (last_part != NULL)  {
			while (last_part->next)
				last_part = last_part->next;
		}
	}

	/*  PART */
	part = mail_newbody_part();
	if (part == NULL)  {
		EM_DEBUG_EXCEPTION("mail_newbody_part failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	/* add it to the last */
	part->next = NULL;
	if (last_part)
		last_part->next = part;
	else
		body->nested.part = part;

	last_part = part;

	/* set data..  */
	/* content_data = (unsigned char  *)fs_get(data_len + 1); */
	/* memcpy(content_data, data, data_len); */
	/* content_data[data_len] = 0; */

	/* part->body.contents.text.data = content_data; */
	/* part->body.contents.text.size = data_len; */

	if (filename)  {   /*  attachment */
		source_text.data = (unsigned char*)filename;
		source_text.size = EM_SAFE_STRLEN(filename);

		result_charset   = (CHARSET*)utf8_infercharset(&source_text);

		if(result_charset) {
			EM_DEBUG_LOG_SEC("return_charset->name [%s]", result_charset->name);
			encoded_file_name = (char*)g_convert (filename, -1, "UTF-8", result_charset->name, &bytes_read, &bytes_written, &glib_error);
		}
		else {
			i = 0;
			while(filename[i]) {
				if(filename[i++] & 0x80) {
					has_special_character = 1;
					break;
				}
			}
			EM_DEBUG_LOG("has_special_character [%d]", has_special_character);
			if(has_special_character)
				encoded_file_name = (char*)g_convert (filename, -1, "UTF-8", "EUC-KR", &bytes_read, &bytes_written, &glib_error);
		}

		EM_DEBUG_LOG_SEC("encoded_file_name [%s]", encoded_file_name);

		if(encoded_file_name == NULL)
			encoded_file_name = strdup(filename);

		if(!em_encode_base64(encoded_file_name, EM_SAFE_STRLEN(encoded_file_name), &base64_file_name, (unsigned long*)&base64_file_name_length, &error)) {
			EM_DEBUG_EXCEPTION("em_encode_base64 failed. error [%d]", error);
			goto FINISH_OFF;
		}

		result_file_name = em_replace_all_string(base64_file_name, "\015\012", "");

		EM_DEBUG_LOG("base64_file_name_length [%d]", base64_file_name_length);

		if(result_file_name) {
			EM_SAFE_FREE(encoded_file_name);
			encoded_file_name = em_malloc(EM_SAFE_STRLEN(result_file_name) + 15);
			if(!encoded_file_name) {
				EM_DEBUG_EXCEPTION("em_malloc failed.");
				goto FINISH_OFF;
			}
			snprintf(encoded_file_name, EM_SAFE_STRLEN(result_file_name) + 15, "=?UTF-8?B?%s?=", result_file_name);
			EM_DEBUG_LOG_SEC("encoded_file_name [%s]", encoded_file_name);
		}

		extension = em_get_extension_from_file_path(filename, NULL);

		part->body.type = em_get_content_type_from_extension_string(extension, NULL);
		if(part->body.type == TYPEIMAGE) {
			part->body.subtype = strdup(extension);
			part->body.encoding = ENCBINARY;
		} else if (part->body.type == TYPEPKCS7_SIGN) {
			part->body.subtype = strdup("pkcs7-signature");
			part->body.type = TYPEAPPLICATION;
			part->body.encoding = ENCBINARY;
		} else if (part->body.type == TYPEPKCS7_MIME) {
			part->body.subtype = strdup("pkcs7-mime");
			part->body.type = TYPEAPPLICATION;
			part->body.encoding = ENCBINARY;
		} else if (part->body.type == TYPEPGP) {
			part->body.type     = TYPEAPPLICATION;
			part->body.subtype  = EM_SAFE_STRDUP(content_sub_type);
			part->body.encoding = ENC7BIT;
		} else {
			part->body.subtype = strdup("octet-stream");
			part->body.encoding = ENCBINARY;
		}

		if (!extension && content_sub_type) {
			char *p = NULL;

			if (strcasecmp(content_sub_type, "IMAGE") == 0) {
				part->body.type = TYPEIMAGE;
				if ((p = strstr(content_sub_type, "/"))) {
					EM_SAFE_FREE(part->body.subtype);
					part->body.subtype = EM_SAFE_STRDUP(p+1);
				}
			}
		}

		part->body.size.bytes = data_len;

		if (data)
			part->body.sparep = EM_SAFE_STRDUP((char *)data); /*  file path */
		else
			part->body.sparep = NULL;

		SNPRINTF(content_disposition, sizeof(content_disposition), "%s", "attachment");

		part->body.disposition.type = cpystr(content_disposition);

		/*  BODY PARAMETER */
		/*  another parameter or get parameter-list from this   function-parameter */
		param = mail_newbody_parameter();
		if (param == NULL)  {
			EM_DEBUG_EXCEPTION("mail_newbody_parameter failed...");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		param->attribute          = cpystr("name");
		param->value              = cpystr(encoded_file_name);
		param->next               = NULL;
		last_param                = param;
		last_part->body.parameter = last_param;

		if (is_inline) {
			/*  CONTENT-ID */
			part->body.id = emcore_generate_content_id_string("org.tizen.email", &error);
			part->body.type = TYPEIMAGE;
			/*  EM_SAFE_FREE(part->body.subtype); */
			/*  part->body.subtype = EM_SAFE_STRDUP(content_sub_type); */
		}

		/*  DISPOSITION PARAMETER */
		param = mail_newbody_parameter();
		if (param == NULL)  {
			EM_DEBUG_EXCEPTION("mail_newbody_parameter failed...");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		param->attribute                      = cpystr("filename");
		param->value                          = cpystr(encoded_file_name);
		param->next                           = NULL;
		last_param                            = param;
		last_part->body.disposition.parameter = last_param;

		if (is_inline)
			last_part->body.disposition.type = strdup("inline");
	}
	else  {
		if (content_sub_type && !strcasecmp(content_sub_type, "pgp-encrypted"))
			mail_type = EMAIL_PGP_ENCRYPTED;

		if (mail_type != EMAIL_PGP_ENCRYPTED) {
			/*  text body (plain/html) */
			part->body.type = TYPETEXT;
			part->body.size.bytes = data_len;

			if (data)
				part->body.sparep = EM_SAFE_STRDUP((char *)data); /*  file path */
			else
				part->body.sparep = NULL;


			if (!content_sub_type)  {
				/* Plain text body */
				part->body.encoding = ENC8BIT;
				part->body.subtype = cpystr("plain");
				last_param = part->body.parameter;

				if (last_param != NULL)  {
					while (last_param->next)
						last_param = last_param->next;
				}

				param = mail_newbody_parameter();

				if (param == NULL)  {
					EM_DEBUG_EXCEPTION("mail_newbody_parameter failed...");
					error = EMAIL_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}

				param->attribute = cpystr("CHARSET");

				if (data != NULL) {
					gchar *extract_charset_plain = g_path_get_basename((const gchar *)data);
					if (extract_charset_plain != NULL && extract_charset_plain[0] != '\0')
						param->value = cpystr(extract_charset_plain);
					g_free(extract_charset_plain);
				}
				else
					param->value = cpystr("UTF-8");

				if(!param->value)
					param->value = cpystr("UTF-8");

				param->next = NULL;

				if (last_param != NULL)
					last_param->next = param;
				else
					part->body.parameter = param;
			}
			else {
				/* HTML text body */
				part->body.encoding = ENC8BIT;
				part->body.subtype  = cpystr(content_sub_type);

				last_param = part->body.parameter;

				if (last_param != NULL)  {
					while (last_param->next)
						last_param = last_param->next;
				}

				param = mail_newbody_parameter();

				if (param == NULL)  {
					EM_DEBUG_EXCEPTION("mail_newbody_parameter failed...");
					error = EMAIL_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}

				param->attribute = cpystr("CHARSET");

				char *pHtml = NULL;
				if (data != NULL) {
					gchar *extract_charset = g_path_get_basename((const gchar *)data);
					if (extract_charset != NULL) {
						if ((pHtml = strstr(extract_charset, ".htm")) != NULL) {
							extract_charset[pHtml-extract_charset] = '\0';
							param->value = cpystr(extract_charset);
						}
					}

					if(!param->value)
						param->value = cpystr("UTF-8");

					EM_SAFE_FREE(extract_charset);
				}
				else
					param->value = cpystr("UTF-8");
				param->next = NULL;

				if (last_param != NULL)
					last_param->next = param;
				else
					part->body.parameter = param;
			}

			/* NOTE : need to require this code. */
			/* sprintf(content_disposition, "%s\0", "inline"); */
			if (is_inline) {
				SNPRINTF(content_disposition, sizeof(content_disposition), "%s", "inline");
				part->body.disposition.type = cpystr(content_disposition);
			}
		} else {
			part->body.type = TYPEAPPLICATION;
			part->body.subtype = strdup(content_sub_type);
			part->body.size.bytes = data_len;
			part->body.encoding = ENC7BIT;

			if (data)
				part->body.sparep = EM_SAFE_STRDUP((char *)data); /*  file path */
			else {
				part->body.contents.text.data = (unsigned char *)strdup("Version: 1");
				part->body.contents.text.size = strlen("Version: 1");
			}
		}
	}

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE(encoded_file_name);
	EM_SAFE_FREE(result_file_name); /*prevent 26242*/
	EM_SAFE_FREE(base64_file_name);
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

static PART *attach_multipart_with_sub_type(BODY *parent_body, char *sub_type, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("parent_body[%p], sub_type [%s], err_code[%p]", parent_body, sub_type, err_code);

	int error = EMAIL_ERROR_NONE;

	PART *tail_part_cur = NULL;
	PART *new_part = NULL;

	if (!parent_body || !sub_type)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (parent_body->nested.part)  {
		tail_part_cur = parent_body->nested.part;

		if (tail_part_cur != NULL)  {
			while (tail_part_cur->next)
				tail_part_cur = tail_part_cur->next;
		}
	}

	new_part = mail_newbody_part();

	if (new_part == NULL)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;

	}

	new_part->next = NULL;
	new_part->body.type = TYPEMULTIPART;
	new_part->body.subtype = EM_SAFE_STRDUP(sub_type);

	if (tail_part_cur)
		tail_part_cur->next = new_part;
	else
		parent_body->nested.part = new_part;

FINISH_OFF:

	if (err_code)
		*err_code = error;

	EM_DEBUG_FUNC_END();

	return new_part;
}
#ifdef __FEATURE_SUPPORT_REPORT_MAIL__
static int attach_attachment_to_body(char *multi_user_name, BODY **multipart_body, BODY *text_body, emstorage_attachment_tbl_t *input_attachment_tbl, int input_attachment_tbl_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("multipart_body[%p], text_body[%p], input_attachment_tbl[%p], input_attachment_tbl_count [%d], err_code[%p]", multipart_body, text_body, input_attachment_tbl, input_attachment_tbl_count, err_code);

	int ret = false;
	int i = 0;
	int error = EMAIL_ERROR_NONE;
	BODY *frame_body = NULL;
	char *prefix_path = NULL;
	char real_file_path[MAX_PATH] = {0};

	/*  make multipart body(multipart frame_body..) .. that has not content..  */

	if (!multipart_body || !text_body || !input_attachment_tbl) {
		EM_DEBUG_EXCEPTION(" multipart_body[%p], text_body[%p], input_attachment_tbl[%p]", multipart_body, text_body, input_attachment_tbl);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	frame_body = mail_newbody();
	if (frame_body == NULL) {
		EM_DEBUG_EXCEPTION("mail_newbody failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	frame_body->type = TYPEMULTIPART;
	frame_body->contents.text.data = NULL;
	frame_body->contents.text.size = 0;
	frame_body->size.bytes = 0;

	/*  insert original text_body to frame_body.. */
	if (!attach_part(frame_body, text_body->sparep, 0, NULL, NULL, false, &error))  {
		EM_DEBUG_EXCEPTION(" attach_part failed [%d]", error);
		goto FINISH_OFF;
	}

	/*  insert files..  */
	emstorage_attachment_tbl_t *temp_attachment_tbl = NULL;
	char *name = NULL;
	struct stat st_buf;

    if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		error = emcore_get_container_path(multi_user_name, &prefix_path);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_container_path failed : [%d]", error);
			goto FINISH_OFF;
		}
	} else {
		prefix_path = strdup("");
	}

	for(i = 0; i < input_attachment_tbl_count; i++) {
		temp_attachment_tbl = input_attachment_tbl + i;

		EM_DEBUG_LOG("insert files - attachment id[%d]", temp_attachment_tbl->attachment_id);

        memset(real_file_path, 0x00, sizeof(real_file_path));
        SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, temp_attachment_tbl->attachment_path);

		if (stat(temp_attachment_tbl->attachment_path, &st_buf) == 0)  {
			if (!temp_attachment_tbl->attachment_name)  {
				if (!emcore_get_file_name(real_file_path, &name, &error))  {
					EM_DEBUG_EXCEPTION("emcore_get_file_name failed [%d]", error);
					goto FINISH_OFF;
				}
			}
			else
				name = temp_attachment_tbl->attachment_name;

			if (!attach_part(frame_body, (unsigned char *)real_file_path, 0, name, NULL, false, &error))  {
				EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
				goto FINISH_OFF;
			}
		}
	}

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(prefix_path);

	if (ret == true)
		*multipart_body = frame_body;
	else if (frame_body != NULL)
		mail_free_body(&frame_body);

	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}
#endif
static char *emcore_encode_rfc2047_text(char *utf8_text, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("utf8_text[%s], err_code[%p]", utf8_text, err_code);

	if (utf8_text == NULL)  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return NULL;
	}

	gsize len = EM_SAFE_STRLEN(utf8_text);

	EM_DEBUG_FUNC_END();


	if (len > 0) {
		return emcore_gmime_get_encoding_to_utf8(utf8_text);
//              return g_strdup(utf8_text);    /* emoji handle */
        }
	else
		return strdup("");
}

static void emcore_encode_rfc2047_address(ADDRESS *address, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("address[%p], err_code[%p]", address, err_code);

	while (address)  {
		if (address->personal)  {
			char *rfc2047_personal = emcore_encode_rfc2047_text(address->personal, err_code);
			EM_SAFE_FREE(address->personal);
			address->personal = rfc2047_personal;
		}
		address = address->next;
	}
	EM_DEBUG_FUNC_END();
}

#define DATE_STR_LENGTH 100

static int emcore_make_envelope_from_mail(char *multi_user_name, emstorage_mail_tbl_t *input_mail_tbl_data, ENVELOPE **output_envelope)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_tbl_data[%p], output_envelope[%p]", input_mail_tbl_data, output_envelope);

	int 	ret = false;
	int       error                   = EMAIL_ERROR_NONE;
	int       is_incomplete           = 0;
	char     *pAdd                    = NULL;
	ENVELOPE *envelope                = NULL;
	email_account_t *ref_account      = NULL;

	if (!input_mail_tbl_data || !output_envelope)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error = EMAIL_ERROR_INVALID_PARAM;
		return error; /* prevent 32729 */
	}

	if ( (input_mail_tbl_data->report_status & EMAIL_MAIL_REPORT_MDN) != 0 && !input_mail_tbl_data->body_download_status) {
		EM_DEBUG_EXCEPTION("input_mail_tbl_data->body_download_status[%p]", input_mail_tbl_data->body_download_status);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(envelope = mail_newenvelope()))  {
		EM_DEBUG_EXCEPTION("mail_newenvelope failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	is_incomplete = input_mail_tbl_data->flags_draft_field || (input_mail_tbl_data->save_status == EMAIL_MAIL_STATUS_SENDING);

	if (is_incomplete && (input_mail_tbl_data->account_id > 0))  {
		ref_account = emcore_get_account_reference(multi_user_name, input_mail_tbl_data->account_id, false);
		if (!ref_account)  {
			EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", input_mail_tbl_data->account_id);
			error = EMAIL_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF;
		}

		if (ref_account->user_email_address && ref_account->user_email_address[0] != '\0')  {
			char *p = cpystr(ref_account->user_email_address);

			if (p == NULL)  {
				EM_DEBUG_EXCEPTION("cpystr failed...");
				error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			EM_DEBUG_LOG("Assign envelope->from");

			if (input_mail_tbl_data->full_address_from) {
				char *temp_address_string = NULL ;
				em_skip_whitespace_without_alias(input_mail_tbl_data->full_address_from , &temp_address_string);
				EM_DEBUG_LOG_SEC("address[temp_address_string][%s]", temp_address_string);
				rfc822_parse_adrlist(&envelope->from, temp_address_string, NULL);
				EM_SAFE_FREE(temp_address_string);
				temp_address_string = NULL ;
			}
			else
				envelope->from = rfc822_parse_mailbox(&p, NULL);

			EM_SAFE_FREE(p);
			if (!envelope->from) {
				EM_DEBUG_EXCEPTION("rfc822_parse_mailbox failed...");
				error = EMAIL_ERROR_INVALID_ADDRESS;
				goto FINISH_OFF;
			}
			else  {
				if (envelope->from->personal == NULL) {
					if (ref_account->options.display_name_from && ref_account->options.display_name_from[0] != '\0')
						envelope->from->personal = cpystr(ref_account->options.display_name_from);
					else
						envelope->from->personal =
								(ref_account->user_display_name && ref_account->user_display_name[0] != '\0') ?
								cpystr(ref_account->user_display_name)  :  NULL;
				}
			}
		}

		if (ref_account->return_address && ref_account->return_address[0] != '\0')  {
			char *p = cpystr(ref_account->return_address);

			if (p == NULL)  {
				EM_DEBUG_EXCEPTION("cpystr failed...");
				error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			envelope->return_path = rfc822_parse_mailbox(&p, NULL);
			EM_SAFE_FREE(p);
		}
	}
	else  {
/*
		if (input_mail_tbl_data->full_address_from == NULL)  {
			EM_DEBUG_EXCEPTION("input_mail_tbl_data->full_address_from[%p]", input_mail_tbl_data->full_address_from);
			error = EMAIL_ERROR_INVALID_MAIL;
			goto FINISH_OFF;
		}
*/
		int i, j;

		if (input_mail_tbl_data->full_address_from)  {
			for (i = 0, j = EM_SAFE_STRLEN(input_mail_tbl_data->full_address_from); i < j; i++)  {
				if (input_mail_tbl_data->full_address_from[i] == ';')
					input_mail_tbl_data->full_address_from[i] = ',';
			}
		}

		if (input_mail_tbl_data->full_address_return)  {
			for (i = 0, j = EM_SAFE_STRLEN(input_mail_tbl_data->full_address_return); i < j; i++)  {
				if (input_mail_tbl_data->full_address_return[i] == ';')
					input_mail_tbl_data->full_address_return[i] = ',';
			}
		}
		em_skip_whitespace_without_alias(input_mail_tbl_data->full_address_from , &pAdd);
		EM_DEBUG_LOG_SEC("address[pAdd][%s]", pAdd);

		rfc822_parse_adrlist(&envelope->from, pAdd, NULL);
		EM_SAFE_FREE(pAdd);
		pAdd = NULL;

		em_skip_whitespace(input_mail_tbl_data->full_address_return , &pAdd);
		EM_DEBUG_LOG_SEC("address[pAdd][%s]", pAdd);

		rfc822_parse_adrlist(&envelope->return_path, pAdd, NULL);
		EM_SAFE_FREE(pAdd);
		pAdd = NULL;
	}

	{
		int i, j;

		if (input_mail_tbl_data->full_address_to)  {
			for (i = 0, j = EM_SAFE_STRLEN(input_mail_tbl_data->full_address_to); i < j; i++)  {
				if (input_mail_tbl_data->full_address_to[i] == ';')
					input_mail_tbl_data->full_address_to[i] = ',';
			}
		}

		if (input_mail_tbl_data->full_address_cc)  {
			for (i = 0, j = EM_SAFE_STRLEN(input_mail_tbl_data->full_address_cc); i < j; i++)  {
				if (input_mail_tbl_data->full_address_cc[i] == ';')
					input_mail_tbl_data->full_address_cc[i] = ',';
			}
		}

		if (input_mail_tbl_data->full_address_bcc)  {
			for (i = 0, j = EM_SAFE_STRLEN(input_mail_tbl_data->full_address_bcc); i < j; i++)  {
				if (input_mail_tbl_data->full_address_bcc[i] == ';')
					input_mail_tbl_data->full_address_bcc[i] = ',';
			}
		}
	}

	envelope->message_id = EM_SAFE_STRDUP(input_mail_tbl_data->message_id);
	EM_DEBUG_LOG_SEC("message_id[%s]", envelope->message_id);

	em_skip_whitespace(input_mail_tbl_data->full_address_to , &pAdd);
	EM_DEBUG_LOG_SEC("address[pAdd][%s]", pAdd);

	rfc822_parse_adrlist(&envelope->to, pAdd, NULL);
	EM_SAFE_FREE(pAdd);
	pAdd = NULL ;

	EM_DEBUG_LOG_SEC("address[input_mail_tbl_data->full_address_cc][%s]", input_mail_tbl_data->full_address_cc);
	em_skip_whitespace(input_mail_tbl_data->full_address_cc , &pAdd);
	EM_DEBUG_LOG_SEC("address[pAdd][%s]", pAdd);

	rfc822_parse_adrlist(&envelope->cc, pAdd, NULL);
	EM_SAFE_FREE(pAdd);
	pAdd = NULL ;

	em_skip_whitespace(input_mail_tbl_data->full_address_bcc , &pAdd);
	rfc822_parse_adrlist(&envelope->bcc, pAdd, NULL);
	EM_SAFE_FREE(pAdd);
		pAdd = NULL ;

	emcore_encode_rfc2047_address(envelope->return_path, &error);
	emcore_encode_rfc2047_address(envelope->from, &error);
	emcore_encode_rfc2047_address(envelope->sender, &error);
	emcore_encode_rfc2047_address(envelope->reply_to, &error);
	emcore_encode_rfc2047_address(envelope->to, &error);
	emcore_encode_rfc2047_address(envelope->cc, &error);
	emcore_encode_rfc2047_address(envelope->bcc, &error);

	if (input_mail_tbl_data->subject) {
		envelope->subject = emcore_encode_rfc2047_text(input_mail_tbl_data->subject, &error);
	}

	char rfc822_date_string[DATE_STR_LENGTH] = { 0, };
	rfc822_date(rfc822_date_string);

	EM_DEBUG_LOG("rfc822_date : [%s]", rfc822_date_string);

	if (!is_incomplete)  {
		char  localtime_string[DATE_STR_LENGTH] = { 0, };
		time_t tn     = time(0);
		struct tm *t  = gmtime(&tn);
		if (t == NULL) {
			EM_DEBUG_EXCEPTION("gmtime failed");
			error = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		int zone      = t->tm_hour * 60 + t->tm_min;
		int julian    = t->tm_yday;

		t = localtime(&input_mail_tbl_data->date_time);
		if (t == NULL) {
			EM_DEBUG_EXCEPTION("localtime failed");
			error = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		zone = t->tm_hour * 60 + t->tm_min - zone;

		if ((julian = t->tm_yday - julian))
			zone += ((julian < 0) == (abs(julian) == 1)) ? -24 * 60 : 24*60;

		SNPRINTF(localtime_string, DATE_STR_LENGTH, "%s, %d %s %d %02d:%02d:%02d "
                                                          , days[t->tm_wday]
                                                          , t->tm_mday
                                                          , months[t->tm_mon]
                                                          , t->tm_year + 1900
                                                          , t->tm_hour
                                                          , t->tm_min
                                                          , t->tm_sec
		);

		EM_DEBUG_LOG("localtime string : [%s]", localtime_string);
		/* append last 5byes("+0900") */
		g_strlcat(localtime_string, strchr(rfc822_date_string, '+'), DATE_STR_LENGTH);
		envelope->date = (unsigned char *)cpystr((const char *)localtime_string);
	}
	else {
		envelope->date = (unsigned char *)cpystr((const char *)rfc822_date_string);
	}

	ret = true;

FINISH_OFF:

	if (ret)
		*output_envelope = envelope;
	else {
		mail_free_envelope(&envelope);
		*output_envelope = NULL;
	}

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

static char *emcore_get_digest_string(int digest_type, int mime_type)
{
	EM_DEBUG_FUNC_BEGIN();
	char *digest_string = NULL;
	char p_digest_string[100] = {0, };

	switch (mime_type) {
	case EMAIL_SMIME_SIGNED :
	case EMAIL_SMIME_ENCRYPTED :
	case EMAIL_SMIME_SIGNED_AND_ENCRYPTED :
		memset(p_digest_string, 0x00, sizeof(p_digest_string));
		break;
	case EMAIL_PGP_SIGNED :
	case EMAIL_PGP_ENCRYPTED :
	case EMAIL_PGP_SIGNED_AND_ENCRYPTED :
		memset(p_digest_string, 0x00, sizeof(p_digest_string));
		strcpy(p_digest_string, "pgp-");
		break;
	}

	switch (digest_type) {
	case DIGEST_TYPE_SHA1 :
		strcat(p_digest_string, "sha1");
		break;
	case DIGEST_TYPE_MD5 :
		strcat(p_digest_string, "md5");
		break;
	case DIGEST_TYPE_RIPEMD160 :
		strcat(p_digest_string, "ripemd160");
		break;
	case DIGEST_TYPE_MD2 :
		strcat(p_digest_string, "md2");
		break;
	case DIGEST_TYPE_TIGER192 :
		strcat(p_digest_string, "tiger192");
		break;
	case DIGEST_TYPE_HAVAL5160 :
		strcat(p_digest_string, "haval5160");
		break;
	case DIGEST_TYPE_SHA256 :
		strcat(p_digest_string, "sha256");
		break;
	case DIGEST_TYPE_SHA384 :
		strcat(p_digest_string, "sha384");
		break;
	case DIGEST_TYPE_SHA512 :
		strcat(p_digest_string, "sha512");
		break;
	case DIGEST_TYPE_SHA224 :
		strcat(p_digest_string, "sha224");
		break;
	case DIGEST_TYPE_MD4 :
		strcat(p_digest_string, "md4");
		break;
	}

	digest_string = EM_SAFE_STRDUP(p_digest_string);

	EM_DEBUG_FUNC_END();
	return digest_string;
}


/*  Description : Make RFC822 text file from mail_tbl data */
/*  Parameters :  */
/* 			input_mail_tbl_data :   */
/* 			is_draft  :  this mail is draft mail. */
/* 			file_path :  path of file that rfc822 data will be written to. */
INTERNAL_FUNC int emcore_make_rfc822_file_from_mail(char *multi_user_name, emstorage_mail_tbl_t *input_mail_tbl_data, emstorage_attachment_tbl_t *input_attachment_tbl, int input_attachment_count, ENVELOPE **env, char **file_path, email_option_t *sending_option, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_tbl_data[%p], env[%p], file_path[%p], sending_option[%p], err_code[%p]", input_mail_tbl_data, env, file_path, sending_option, err_code);

	int       ret = false;
	int       error = EMAIL_ERROR_NONE;
	int       i = 0;
	char     *digest_string = NULL;

	ENVELOPE *envelope      = NULL;
	BODY     *root_body     = NULL;
	BODY     *text_body     = NULL;
	BODY     *html_body     = NULL;
	PART     *part_for_alternative = NULL;
	PART     *part_for_related = NULL;
	PARAMETER *param = NULL;

	char *prefix_path = NULL;
	char real_file_path[MAX_PATH] = {0};

	if (!input_mail_tbl_data)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ( (input_mail_tbl_data->report_status & EMAIL_MAIL_REPORT_MDN) != 0 && !input_mail_tbl_data->body_download_status) {
		EM_DEBUG_EXCEPTION("input_mail_tbl_data->body_download_status[%p]", input_mail_tbl_data->body_download_status);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ( (error = emcore_make_envelope_from_mail(multi_user_name, input_mail_tbl_data, &envelope)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_make_envelope_from_mail failed [%d]", error);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_SEC("input_mail_tbl_data->file_path_plain[%s]", input_mail_tbl_data->file_path_plain);
	EM_DEBUG_LOG_SEC("input_mail_tbl_data->file_path_html[%s]", input_mail_tbl_data->file_path_html);
	EM_DEBUG_LOG_SEC("input_mail_tbl_data->file_path_mime_entity[%s]", input_mail_tbl_data->file_path_mime_entity);
	EM_DEBUG_LOG("input_mail_tbl_data->body->attachment_num[%d]", input_mail_tbl_data->attachment_count);

	root_body = mail_newbody();
	if (root_body == NULL)  {
		EM_DEBUG_EXCEPTION("mail_newbody failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	param = mail_newbody_parameter();
	if (param == NULL) {
		EM_DEBUG_EXCEPTION("mail_newbody_parameter failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

    if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		error = emcore_get_container_path(multi_user_name, &prefix_path);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_container_path failed : [%d]", error);
			goto FINISH_OFF;
		}
	} else {
		prefix_path = strdup("");
	}

	if (input_attachment_count > 0) {
		/* handle the Multipart/mixed, Multipart/related and S/MIME */
		EM_DEBUG_LOG("input_attachment_num [%d]", input_attachment_count);
		EM_DEBUG_LOG("inline_attachment_num [%d]", input_mail_tbl_data->inline_content_count);
		EM_DEBUG_LOG("attachment_num [%d]", input_mail_tbl_data->attachment_count);

		if (input_mail_tbl_data->smime_type == EMAIL_SMIME_NONE) {
			if (input_mail_tbl_data->attachment_count > 0) {
				root_body->type    = TYPEMULTIPART;
				root_body->subtype = strdup("MIXED");
			} else {
				root_body->type    = TYPEMULTIPART;
				root_body->subtype = strdup("RELATED");
			}

			mail_free_body_parameter(&param);
			param = NULL;
		} else if (input_mail_tbl_data->smime_type == EMAIL_SMIME_SIGNED) {
			PARAMETER *protocol_param   = mail_newbody_parameter();

			root_body->type             = TYPEMULTIPART;
			root_body->subtype          = strdup("SIGNED");

			param->attribute            = cpystr("micalg");

			digest_string               = emcore_get_digest_string(input_mail_tbl_data->digest_type, input_mail_tbl_data->smime_type);
			param->value                = cpystr(digest_string);

			protocol_param->attribute   = cpystr("protocol");
			protocol_param->value       = cpystr("application/pkcs7-signature");
			protocol_param->next        = NULL;
			param->next	            = protocol_param;
		} else if (input_mail_tbl_data->smime_type == EMAIL_SMIME_ENCRYPTED || input_mail_tbl_data->smime_type == EMAIL_SMIME_SIGNED_AND_ENCRYPTED) {
			root_body->type    = TYPEAPPLICATION;
			root_body->subtype = strdup("PKCS7-MIME");

			param->attribute = cpystr("name");
			param->value = cpystr("smime.p7m");
			param->next = NULL;
		} else if (input_mail_tbl_data->smime_type == EMAIL_PGP_SIGNED) {
			PARAMETER *protocol_param = mail_newbody_parameter();

			root_body->type           = TYPEMULTIPART;
			root_body->subtype        = strdup("SIGNED");

			param->attribute          = cpystr("micalg");

			digest_string             = emcore_get_digest_string(input_mail_tbl_data->digest_type, input_mail_tbl_data->smime_type);
			param->value              = cpystr(digest_string);

			protocol_param->attribute = cpystr("protocol");
			protocol_param->value     = cpystr("application/pgp-signature");
			protocol_param->next      = NULL;
			param->next               = protocol_param;
		} else {
			root_body->type           = TYPEMULTIPART;
			root_body->subtype        = strdup("encrypted");

			param->attribute          = cpystr("protocol");
			param->value              = cpystr("application/pgp-encrypted");
			param->next               = NULL;
		}

		root_body->contents.text.data = NULL;
		root_body->contents.text.size = 0;
		root_body->size.bytes         = 0;
		root_body->parameter          = param;

		if (input_mail_tbl_data->smime_type == EMAIL_SMIME_NONE &&
			                   (input_mail_tbl_data->file_path_plain && input_mail_tbl_data->file_path_html)) {
			/* Multipart/mixed -> multipart/related -> multipart/alternative  : has inline content and */
			/* Multipart/mixed -> Multipart/alternative */

			if (input_mail_tbl_data->inline_content_count > 0 && (strcasecmp(root_body->subtype, "RELATED") != 0)) {
				part_for_related = attach_multipart_with_sub_type(root_body, "RELATED", &error);
				if (!part_for_related) {
					EM_DEBUG_EXCEPTION("attach_multipart_with_sub_type [related] failed [%d]", error);
					goto FINISH_OFF;
				}
			}

			if (part_for_related)
				part_for_alternative = attach_multipart_with_sub_type(&(part_for_related->body), "ALTERNATIVE", &error);
			else
				part_for_alternative = attach_multipart_with_sub_type(root_body, "ALTERNATIVE", &error);
			if (!part_for_alternative) {
				EM_DEBUG_EXCEPTION("attach_multipart_with_sub_type [alternative] failed [%d]", error);
				goto FINISH_OFF;
			}

			if (strlen(input_mail_tbl_data->file_path_plain) > 0) {
				EM_DEBUG_LOG_SEC("file_path_plain[%s]", input_mail_tbl_data->file_path_plain);

                memset(real_file_path, 0x00, sizeof(real_file_path));
                SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_mail_tbl_data->file_path_plain);

				text_body = &(part_for_alternative->body);
				if (!attach_part(text_body, (unsigned char *)real_file_path, 0, NULL, "plain", false, &error)) {
					EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
					goto FINISH_OFF;
				}
			}

			if (strlen(input_mail_tbl_data->file_path_html) > 0) {
				EM_DEBUG_LOG_SEC("file_path_html[%s]", input_mail_tbl_data->file_path_html);

                memset(real_file_path, 0x00, sizeof(real_file_path));
                SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_mail_tbl_data->file_path_html);

				html_body = &(part_for_alternative->body);
				if (!attach_part (html_body, (unsigned char *)real_file_path, 0, NULL, "html", false, &error)) {
					EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
					goto FINISH_OFF;
				}
			}
		}
		else if (input_mail_tbl_data->smime_type == EMAIL_SMIME_NONE &&
                                          (input_mail_tbl_data->file_path_plain || input_mail_tbl_data->file_path_html)) {
			if (input_mail_tbl_data->file_path_plain && EM_SAFE_STRLEN(input_mail_tbl_data->file_path_plain) > 0) {
				EM_DEBUG_LOG_SEC("file_path_plain[%s]", input_mail_tbl_data->file_path_plain);

                memset(real_file_path, 0x00, sizeof(real_file_path));
                SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_mail_tbl_data->file_path_plain);

				if (!attach_part (root_body, (unsigned char *)real_file_path, 0, NULL, "plain", false, &error)) {
					EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
					goto FINISH_OFF;
				}
			}

			if (input_mail_tbl_data->file_path_html && EM_SAFE_STRLEN(input_mail_tbl_data->file_path_html) > 0) {
				EM_DEBUG_LOG_SEC("file_path_html[%s]", input_mail_tbl_data->file_path_html);
				if (input_mail_tbl_data->inline_content_count > 0 &&
						(root_body->subtype && (strcasecmp(root_body->subtype, "RELATED") != 0))) {
					part_for_related = attach_multipart_with_sub_type(root_body, "RELATED", &error);
					if (!part_for_related) {
						EM_DEBUG_EXCEPTION("attach_multipart_with_sub_type [related] failed [%d]", error);
						goto FINISH_OFF;
					}
				}

                memset(real_file_path, 0x00, sizeof(real_file_path));
                SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_mail_tbl_data->file_path_html);

				if (part_for_related) {
					if (!attach_part(&(part_for_related->body), (unsigned char *)real_file_path, 0,
                                                                                         NULL, "html", false, &error)) {
						EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
						goto FINISH_OFF;
					}
				} else {
					if (!attach_part (root_body, (unsigned char *)real_file_path, 0,
                                                                                         NULL, "html", false, &error)) {
						EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
						goto FINISH_OFF;
					}
				}
			}
		} else if (input_mail_tbl_data->smime_type == EMAIL_SMIME_SIGNED || input_mail_tbl_data->smime_type == EMAIL_PGP_SIGNED) {
			if (input_mail_tbl_data->file_path_mime_entity && EM_SAFE_STRLEN(input_mail_tbl_data->file_path_mime_entity) > 0) {
				EM_DEBUG_LOG_SEC("file_path_mime_entity[%s]", input_mail_tbl_data->file_path_mime_entity);

                memset(real_file_path, 0x00, sizeof(real_file_path));
                SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_mail_tbl_data->file_path_mime_entity);

				root_body->sparep = EM_SAFE_STRDUP(input_mail_tbl_data->file_path_mime_entity);
			}
		} else if (input_mail_tbl_data->smime_type == EMAIL_PGP_ENCRYPTED || input_mail_tbl_data->smime_type == EMAIL_PGP_SIGNED_AND_ENCRYPTED) {
			if (input_mail_tbl_data->file_path_plain && EM_SAFE_STRLEN(input_mail_tbl_data->file_path_plain) > 0) {
				EM_DEBUG_LOG_SEC("file_path_plain[%s]", input_mail_tbl_data->file_path_plain);
				if (!attach_part (root_body, NULL, 0, NULL, "pgp-encrypted", false, &error)) {
					EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
					goto FINISH_OFF;
				}
			}
		} else {
			EM_DEBUG_LOG("S/MIME encrypted type");
		}

		if (input_attachment_tbl && input_attachment_count) {
			emstorage_attachment_tbl_t *temp_attachment_tbl = NULL;
			char *name = NULL;
			BODY *body_to_attach = NULL;

			for(i = 0; i < input_attachment_count; i++) {
				temp_attachment_tbl = input_attachment_tbl + i;

				EM_DEBUG_LOG_SEC("attachment_name[%s], attachment_path[%s]", temp_attachment_tbl->attachment_name, temp_attachment_tbl->attachment_path);

				if (!temp_attachment_tbl->attachment_name)  {
					if (!emcore_get_file_name(temp_attachment_tbl->attachment_path, &name, &error))  {
						EM_DEBUG_EXCEPTION("emcore_get_file_name failed [%d]", error);
						continue;
					}
				}
				else
					name = temp_attachment_tbl->attachment_name;

				EM_DEBUG_LOG_SEC("name[%s]", name);

				if (temp_attachment_tbl->attachment_inline_content_status && part_for_related)
					body_to_attach = &(part_for_related->body);
				else
					body_to_attach = root_body;

                memset(real_file_path, 0x00, sizeof(real_file_path));
                SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, temp_attachment_tbl->attachment_path);

				if (!attach_part(body_to_attach,
						(unsigned char *)real_file_path,
						0,
						name,
						temp_attachment_tbl->attachment_mime_type,
						temp_attachment_tbl->attachment_inline_content_status,
						&error))
				{
					EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
					continue;
				}
			}
		}
		text_body = NULL;
	}
	else if (input_mail_tbl_data->file_path_plain && input_mail_tbl_data->file_path_html) {
		/* Handle the Multipart/alternative */
		root_body->type    = TYPEMULTIPART;
		root_body->subtype = strdup("ALTERNATIVE");

		mail_free_body_parameter(&param);
		param = NULL;

		root_body->contents.text.data = NULL;
		root_body->contents.text.size = 0;
		root_body->size.bytes         = 0;
		root_body->parameter          = param;

		if (strlen(input_mail_tbl_data->file_path_plain) > 0) {
			EM_DEBUG_LOG_SEC("file_path_plain[%s]", input_mail_tbl_data->file_path_plain);

            memset(real_file_path, 0x00, sizeof(real_file_path));
            SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_mail_tbl_data->file_path_plain);

			if (!attach_part(root_body, (unsigned char *)real_file_path, 0, NULL, "plain", false, &error)) {
				EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
				goto FINISH_OFF;
			}
		}

		if (strlen(input_mail_tbl_data->file_path_html) > 0) {
			EM_DEBUG_LOG_SEC("file_path_html[%s]", input_mail_tbl_data->file_path_html);

            memset(real_file_path, 0x00, sizeof(real_file_path));
            SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_mail_tbl_data->file_path_html);

			if (!attach_part (root_body, (unsigned char *)real_file_path, 0, NULL, "html", false, &error)) {
				EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
				goto FINISH_OFF;
			}
		}
	}
	else {
		root_body->type = TYPETEXT;
		root_body->encoding = ENC8BIT;
		if (input_mail_tbl_data->file_path_plain || input_mail_tbl_data->file_path_html) {
            if (input_mail_tbl_data->file_path_plain) {
                memset(real_file_path, 0x00, sizeof(real_file_path));
                SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_mail_tbl_data->file_path_plain);
            } else {
                memset(real_file_path, 0x00, sizeof(real_file_path));
                SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_mail_tbl_data->file_path_html);
            }

			root_body->sparep = EM_SAFE_STRDUP(real_file_path);
        }
		else
			root_body->sparep = NULL;

		if (input_mail_tbl_data->file_path_html != NULL && input_mail_tbl_data->file_path_html[0] != '\0')
			root_body->subtype = strdup("html");
		if (root_body->sparep)
			root_body->size.bytes = EM_SAFE_STRLEN(root_body->sparep);
		else
			root_body->size.bytes = 0;
	}


	if (input_mail_tbl_data->report_status & EMAIL_MAIL_REPORT_MDN) {
		/*  Report mail */
		EM_DEBUG_LOG("REPORT MAIL");
		envelope->references = cpystr(input_mail_tbl_data->message_id);
	}

	if (file_path)  {
		EM_DEBUG_LOG("write rfc822 : file_path[%p]", file_path);

		if ((error = emcore_write_rfc822 (envelope, root_body, input_mail_tbl_data->priority,
                                                    input_mail_tbl_data->report_status, file_path)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_write_rfc822 failed [%d]", error);
			goto FINISH_OFF;
		}
	}

	ret = true;

FINISH_OFF:
	if ((ret == true) && (env != NULL))
		*env = envelope;
	else if (envelope != NULL)
		mail_free_envelope(&envelope);

	if (text_body != NULL)
		mail_free_body(&text_body);

	if (root_body != NULL)
		mail_free_body(&root_body);

	EM_SAFE_FREE(digest_string);
	EM_SAFE_FREE(prefix_path);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emcore_make_rfc822_file(char *multi_user_name, email_mail_data_t *input_mail_tbl_data, email_attachment_data_t *input_attachment_tbl, int input_attachment_count, int mime_entity_status, char **file_path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_tbl_data[%p], file_path[%p], err_code[%p]", input_mail_tbl_data, file_path, err_code);

	int err = EMAIL_ERROR_NONE;
	int ret = false;
	int i = 0;
	int attachment_count = 0;
	int inline_content_count = 0;

	emstorage_mail_tbl_t *modified_mail_data = NULL;
	emstorage_attachment_tbl_t *modified_attachment_data = NULL;

	/* Convert from email_mail_data_t to emstorage_mail_tbl_t */
	if (!em_convert_mail_data_to_mail_tbl(input_mail_tbl_data, 1, &modified_mail_data, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_tbl_to_mail_data falied [%d]", err);
		goto FINISH_OFF;
	}

	if (mime_entity_status)
		modified_mail_data->smime_type = EMAIL_SMIME_NONE;

	/* Convert from email_attachment_data_t to emstorage_attachment_tbl_t */
	if (input_attachment_count > 0) {
		modified_attachment_data = em_malloc(sizeof(emstorage_attachment_tbl_t) * input_attachment_count);
		if (modified_attachment_data == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
	}

	for (i = 0; i < input_attachment_count; i++) {
		modified_attachment_data[i].attachment_id                    = input_attachment_tbl[i].attachment_id;
		modified_attachment_data[i].attachment_name                  = EM_SAFE_STRDUP(input_attachment_tbl[i].attachment_name);
		modified_attachment_data[i].attachment_path                  = EM_SAFE_STRDUP(input_attachment_tbl[i].attachment_path);
		modified_attachment_data[i].content_id                       = EM_SAFE_STRDUP(input_attachment_tbl[i].content_id);
		modified_attachment_data[i].attachment_id                    = input_attachment_tbl[i].attachment_size;
		modified_attachment_data[i].mail_id                          = input_attachment_tbl[i].mail_id;
		modified_attachment_data[i].account_id                       = input_attachment_tbl[i].account_id;
		modified_attachment_data[i].mailbox_id                       = input_attachment_tbl[i].mailbox_id;
		modified_attachment_data[i].attachment_save_status           = input_attachment_tbl[i].save_status;
		modified_attachment_data[i].attachment_drm_type              = input_attachment_tbl[i].drm_status;
		modified_attachment_data[i].attachment_inline_content_status = input_attachment_tbl[i].inline_content_status;
		modified_attachment_data[i].attachment_mime_type             = EM_SAFE_STRDUP(input_attachment_tbl[i].attachment_mime_type);

		if (input_attachment_tbl[i].inline_content_status == INLINE_ATTACHMENT)
			inline_content_count += 1;
		else
			attachment_count += 1;
	}

	modified_mail_data->attachment_count = attachment_count;
	modified_mail_data->inline_content_count = inline_content_count;

	if (!emcore_make_rfc822_file_from_mail(multi_user_name, modified_mail_data, modified_attachment_data, input_attachment_count, NULL, file_path, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emcore_make_rfc822_file_from_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (modified_mail_data)
		emstorage_free_mail(&modified_mail_data, 1, NULL);

	if (modified_attachment_data)
		emstorage_free_attachment(&modified_attachment_data, input_attachment_count, NULL);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

#ifdef __FEATURE_SUPPORT_REPORT_MAIL__
static int emcore_get_report_mail_body(char *multi_user_name, ENVELOPE *envelope, BODY **multipart_body, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("envelope[%p], mulitpart_body[%p], err_code[%p]", envelope, multipart_body, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	BODY *m_body = NULL;
	BODY *p_body = NULL;
	BODY *text_body = NULL;
	PARAMETER *param = NULL;
	emstorage_attachment_tbl_t *temp_attachment_tbl = NULL;
	FILE *fp = NULL;
	char *fname = NULL;
	char buf[512] = {0x00, };
	int sz = 0;

	if (!envelope || !multipart_body)  {
		EM_DEBUG_EXCEPTION(" envelope[%p], mulitpart_body[%p]", envelope, multipart_body);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(text_body = mail_newbody()))  {
		EM_DEBUG_EXCEPTION(" mail_newbody failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (!emcore_get_temp_file_name(&fname, &err))  {
		EM_DEBUG_EXCEPTION(" emcore_get_temp_file_name failed [%d]", err);
		goto FINISH_OFF;
	}

	err = em_fopen(fname, "wb+", &fp);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_fopen failed - %s, error : [%d]", fname, err);
		goto FINISH_OFF;
	}

	if (!envelope->from || !envelope->from->mailbox || !envelope->from->host)  {
		if (!envelope->from)
			EM_DEBUG_EXCEPTION(" envelope->from[%p]", envelope->from);
		else
			EM_DEBUG_LOG(" envelope->from->mailbox[%p], envelope->from->host[%p]", envelope->from->mailbox, envelope->from->host);

		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/*
	if (envelope->from->personal)
		SNPRINTF(buf, sizeof(buf), "%s <%s@%s>", envelope->from->personal, envelope->from->mailbox, envelope->from->host);
	else
	*/
		SNPRINTF(buf, sizeof(buf), "%s@%s", envelope->from->mailbox, envelope->from->host);

	fprintf(fp, "Your message has been read by %s"CRLF_STRING, buf);
	fprintf(fp, "Date :  %s", envelope->date);

	fclose(fp); fp = NULL;

	if (!emcore_get_file_size(fname, &sz, &err))  {
		EM_DEBUG_EXCEPTION(" emcore_get_file_size failed [%d]", err);
		goto FINISH_OFF;
	}

	text_body->type = TYPETEXT;
	text_body->encoding = ENC8BIT;
	text_body->sparep = EM_SAFE_STRDUP(fname);
	text_body->size.bytes = (unsigned long)sz;

	if (!emcore_get_temp_file_name(&fname, &err))  {
		EM_DEBUG_EXCEPTION(" emcore_get_temp_file_name failed [%d]", err);
		goto FINISH_OFF;
	}

	err = em_fopen(fname, "wb+", &fp);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_fopen failed - %s, error : [%d]", fname, err);
		goto FINISH_OFF;
	}

	if (!envelope->references)  {
		EM_DEBUG_EXCEPTION(" envelope->references[%p]", envelope->references);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	fprintf(fp, "Final-Recipient :  rfc822;%s@%s\r", envelope->from->mailbox, envelope->from->host);
	fprintf(fp, "Original-Message-ID:  %s\r", envelope->references);
	fprintf(fp, "Disposition :  manual-action/MDN-sent-manually; displayed");

	fclose(fp); fp = NULL;

    temp_attachment_tbl = (emstorage_attachment_tbl_t *)em_malloc(sizeof(emstorage_attachment_tbl_t));
    if (temp_attachment_tbl == NULL) {
        EM_DEBUG_EXCEPTION("em_malloc failed");
        err = EMAIL_ERROR_OUT_OF_MEMORY;
        goto FINISH_OFF;
    }

	temp_attachment_tbl->attachment_path = EM_SAFE_STRDUP(fname);

	if (!emcore_get_file_size(fname, &temp_attachment_tbl->attachment_size, &err))  {
		EM_DEBUG_EXCEPTION(" emcore_get_file_size failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!attach_attachment_to_body(multi_user_name, &m_body, text_body, temp_attachment_tbl, 1, &err))  {
		EM_DEBUG_EXCEPTION(" attach_attachment_to_body failed [%d]", err);
		goto FINISH_OFF;
	}

	text_body->contents.text.data = NULL;

	/*  change mail header */

	/*  set content-type to multipart/report */
	m_body->subtype = strdup("report");

	/*  set report-type parameter in content-type */
	param = em_malloc(sizeof(PARAMETER));
	if (!param)  {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	param->attribute  = strdup("report-type");
	param->value      = strdup("disposition-notification");
	param->next       = m_body->parameter;

	m_body->parameter = param;

	/*  change body-header */

	p_body = &m_body->nested.part->next->body;

	/*  set content-type to message/disposition-notification */
	p_body->type      = TYPEMESSAGE;
	p_body->encoding  = ENC7BIT;

	EM_SAFE_FREE(p_body->subtype);

	p_body->subtype = strdup("disposition-notification");

	/*  set parameter */
	mail_free_body_parameter(&p_body->parameter);
	mail_free_body_parameter(&p_body->disposition.parameter);

	EM_SAFE_FREE(p_body->disposition.type);

	p_body->disposition.type = strdup("inline");

	ret = true;

FINISH_OFF:
	if ((ret == true) && (multipart_body != NULL))
		*multipart_body = m_body;
	else if (m_body != NULL)
		mail_free_body(&m_body);

	if (text_body != NULL)
		mail_free_body(&text_body);

	if (fp != NULL)
		fclose(fp);

	EM_SAFE_FREE(fname);

    if (temp_attachment_tbl)
        emstorage_free_attachment(&temp_attachment_tbl, 1, NULL);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}
#endif

INTERNAL_FUNC int emcore_get_body_buff(char *file_path, char **buff)
{
	EM_DEBUG_FUNC_BEGIN();

	FILE *r_fp = NULL;
	int ret = false;
	char *read_buff = NULL;

	if (file_path)
		r_fp = fopen(file_path, "r");

	if (!r_fp) {
		EM_DEBUG_EXCEPTION_SEC(" Filename %s failed to open", file_path);
		goto FINISH_OFF;
	}

	struct stat stbuf;
	if (stat(file_path, &stbuf) == 0 && stbuf.st_size > 0) {
		EM_DEBUG_LOG(" File Size [ %d ] ", stbuf.st_size);
		read_buff = calloc((stbuf.st_size+ 1), sizeof(char));
		if (read_buff == NULL) {
			EM_DEBUG_EXCEPTION("calloc failed");
			goto FINISH_OFF;
		}
		read_buff[stbuf.st_size] = '\0';
	}

	if (ferror(r_fp)) {
		EM_DEBUG_EXCEPTION_SEC("file read failed - %s", file_path);
		EM_SAFE_FREE(read_buff);
		goto FINISH_OFF;
	}

	ret = true;
	*buff = read_buff;

FINISH_OFF:
	if (r_fp)
		fclose(r_fp);

	return ret;
}

static int emcore_copy_attachment_from_original_mail(char *multi_user_name, int input_original_mail_id, int input_target_mail_id)
{
	EM_DEBUG_FUNC_BEGIN("input_original_mail_id[%d] input_target_mail_id[%d]", input_original_mail_id, input_target_mail_id);
	int err = EMAIL_ERROR_NONE;
	int i = 0, j = 0;
	int original_mail_attachment_count = 0;
	int target_mail_attachment_count = 0;
	int attachment_id = 0;
	char output_file_name[MAX_PATH] = { 0, };
	char output_file_path[MAX_PATH] = { 0, };
	char virtual_file_path[MAX_PATH] = { 0, };
	emstorage_attachment_tbl_t *original_mail_attachment_array = NULL;
	emstorage_attachment_tbl_t *target_mail_attachment_array = NULL;
	emstorage_attachment_tbl_t *target_attach = NULL;

	if((err = emstorage_get_attachment_list(multi_user_name, input_original_mail_id, false, &original_mail_attachment_array, &original_mail_attachment_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);
		goto FINISH_OFF;
	}

	if((err = emstorage_get_attachment_list(multi_user_name, input_target_mail_id, false, &target_mail_attachment_array, &target_mail_attachment_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);
		goto FINISH_OFF;
	}

	for(i = 0; i < original_mail_attachment_count; i++) {
		for(j = 0; j < target_mail_attachment_count; j++) {
			if(strcmp(original_mail_attachment_array[i].attachment_name, target_mail_attachment_array[j].attachment_name) == 0 ) {
				target_attach = target_mail_attachment_array + j;

				/* If attachment is inline content, fild path should not include attachment id */
				if(target_attach->attachment_inline_content_status == 1)
					attachment_id = 0;
				else
					attachment_id = target_attach->attachment_id;

				EM_DEBUG_LOG("attachment_inline_content_status [%d] attachment_id[%d]", target_attach->attachment_inline_content_status, attachment_id);

				if(!emcore_save_mail_file(multi_user_name,
                                            target_attach->account_id,
                                            target_attach->mail_id,
                                            attachment_id,
                                            original_mail_attachment_array[i].attachment_path,
                                            original_mail_attachment_array[i].attachment_name,
                                            output_file_path,
                                            virtual_file_path,
                                            &err)) {
					EM_DEBUG_EXCEPTION("emcore_save_mail_file failed [%d]", err);
					goto FINISH_OFF;
				}

				EM_SAFE_FREE(target_attach->attachment_path);
				target_attach->attachment_path = EM_SAFE_STRDUP(virtual_file_path);
				target_attach->attachment_save_status = 1;

				if(!emstorage_update_attachment(multi_user_name, target_attach, false, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_update_attachment failed [%d]", err);
					goto FINISH_OFF;
				}

				memset(output_file_path, 0, MAX_PATH);
				memset(output_file_name, 0, MAX_PATH);
				break;
			}
		}
	}

FINISH_OFF:
	if(original_mail_attachment_array)
		emstorage_free_attachment(&original_mail_attachment_array, original_mail_attachment_count, NULL);
	if(target_mail_attachment_array)
		emstorage_free_attachment(&target_mail_attachment_array, target_mail_attachment_count, NULL);


	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

/*  send a mail */
INTERNAL_FUNC int emcore_send_mail_with_downloading_attachment_of_original_mail(char *multi_user_name, int input_mail_id)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d]", input_mail_id);
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int attachment_count = 0;
	email_mail_data_t *mail_to_be_sent = NULL;
	email_mail_data_t *original_mail = NULL;
	email_attachment_data_t *attachment_array = NULL;

	/* Get mail data */
	if((err = emcore_get_mail_data(multi_user_name, input_mail_id, &mail_to_be_sent)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_data failed [%d]", err);
		goto FINISH_OFF;
	}

	if(mail_to_be_sent->reference_mail_id <= 0) {
		err = EMAIL_ERROR_INVALID_REFERENCE_MAIL;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_REFERENCE_MAIL");
		goto FINISH_OFF;
	}

	/* Get original mail data */
	if((err = emcore_get_mail_data(multi_user_name, mail_to_be_sent->reference_mail_id, &original_mail)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_data failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Check necessity of download */
	if((err = emcore_get_attachment_data_list(multi_user_name, original_mail->mail_id, &attachment_array, &attachment_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_attachment_data_list failed [%d]", err);
		goto FINISH_OFF;
	}

	/* If need be, download attachments */
	for(i = 0; i < attachment_count; i++) {
		if(attachment_array[i].save_status != 1) {
			/* this function is not run by event thread,
			so cancellable should be set to 0*/
			if(!emcore_gmime_download_attachment(multi_user_name, original_mail->mail_id, i + 1, 0, -1, 0, &err)) {
				EM_DEBUG_EXCEPTION("emcore_gmime_download_attachment failed [%d]", err);
				goto FINISH_OFF;
			}
		}
	}

	/* Copy attachment to the mail to be sent */
	if((err = emcore_copy_attachment_from_original_mail(multi_user_name, original_mail->mail_id, mail_to_be_sent->mail_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_copy_attachment_from_original failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Send the mail */
	if(!emcore_send_mail(multi_user_name, mail_to_be_sent->mail_id, &err)) {
		EM_DEBUG_EXCEPTION("emcore_send_mail failed [%d]", err);
		goto FINISH_OFF;
	}
FINISH_OFF:

	if(attachment_array)
		emcore_free_attachment_data(&attachment_array, attachment_count, NULL);

	if(mail_to_be_sent) {
		emcore_free_mail_data(mail_to_be_sent);
		EM_SAFE_FREE(mail_to_be_sent);
	}

	if(original_mail) {
		emcore_free_mail_data(original_mail);
		EM_SAFE_FREE(original_mail);
	}


	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

/* Scheduled sending ------------------------------------------------ */

static int emcore_sending_alarm_cb(email_alarm_data_t *alarm_data, void *user_parameter)
{
	EM_DEBUG_FUNC_BEGIN("alarm_data [%p] user_parameter [%p]", alarm_data, user_parameter);
	int err = EMAIL_ERROR_NONE;

	if (alarm_data == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	if (alarm_data->reference_id <= 0) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	/* Insert the sending mail event */
	int account_id = 0;
	int input_mail_id = alarm_data->reference_id;
	int dst_mailbox_id = 0;
	int result_handle = 0;
	char *multi_user_name = alarm_data->multi_user_name;

	email_event_t *event_data = NULL;
	emstorage_mail_tbl_t *mail_data = NULL;
	emstorage_mailbox_tbl_t *outbox_tbl = NULL;

	if (!emstorage_get_mail_by_id(multi_user_name, input_mail_id, &mail_data, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed : [%d]", err);
		goto FINISH_OFF;
	}

	account_id = mail_data->account_id;

	if (!emnetwork_check_network_status(&err)) {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed : [%d]", err);
		goto FINISH_OFF;
	}

#ifdef __FEATURE_MOVE_TO_OUTBOX_FIRST__
	if (!emstorage_get_mailbox_by_mailbox_type(multi_user_name,
												account_id,
												EMAIL_MAILBOX_TYPE_OUTBOX,
												&outbox_tbl,
												false,
												&err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed : [%d]", err);
		goto FINISH_OFF;
	}

	dst_mailbox_id = outbox_tbl->mailbox_id;
	emstorage_free_mailbox(&outbox_tbl, 1, NULL);
	outbox_tbl = NULL;

	if (mail_data->mailbox_id != dst_mailbox_id) {
		if (!emcore_move_mail(multi_user_name,
								&input_mail_id,
								1,
								dst_mailbox_id,
								EMAIL_MOVED_AFTER_SENDING,
								0,
								&err)) {
			EM_DEBUG_EXCEPTION("emcore_move_mail failed : [%d]", err);
			goto FINISH_OFF;
		}
	}
#endif /* __FEATURE_MOVE_TO_OUTBOX_FIRST__ */

	if (!emstorage_set_field_of_mails_with_integer_value(multi_user_name,
															account_id,
															&input_mail_id,
															1,
															"save_status",
															EMAIL_MAIL_STATUS_SEND_WAIT,
															true,
															&err)) {
		EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed : [%d]", err);
		goto FINISH_OFF;
	}

	event_data = em_malloc(sizeof(email_event_t));
	if (event_data == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	event_data->type               = EMAIL_EVENT_SEND_MAIL;
	event_data->account_id         = account_id;
	event_data->event_param_data_4 = input_mail_id;
	event_data->event_param_data_5 = mail_data->mailbox_id;
	event_data->multi_user_name    = EM_SAFE_STRDUP(multi_user_name);

	if (!emcore_insert_event_for_sending_mails(event_data, &result_handle, &err)) {
		EM_DEBUG_EXCEPTION(" emcore_insert_event failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	emcore_add_transaction_info(input_mail_id, result_handle, NULL);

	if (err != EMAIL_ERROR_NONE) {
		if (!emstorage_set_field_of_mails_with_integer_value(multi_user_name,
															account_id,
															&input_mail_id,
															1,
															"save_status",
															EMAIL_MAIL_STATUS_SAVED,
															true,
															&err))
			EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed : [%d]", err);

		if (event_data) {
			emcore_free_event(event_data);
			EM_SAFE_FREE(event_data);
		}
	}

	if (mail_data)
		emstorage_free_mail(&mail_data, 1, NULL);

	if (outbox_tbl)
		emstorage_free_mailbox(&outbox_tbl, 1, NULL);


	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


INTERNAL_FUNC int emcore_schedule_sending_mail(char *multi_user_name, int input_mail_id, time_t input_time_to_send)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d] input_time_to_send[%d]", input_mail_id, input_time_to_send);
	int err = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t *mail_data = NULL;

	/* get mail data */
	if (!emstorage_get_mail_by_id(multi_user_name, input_mail_id, &mail_data, true, &err) || mail_data == NULL) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emstorage_set_field_of_mails_with_integer_value(multi_user_name,
															mail_data->account_id,
															&(mail_data->mail_id),
															1,
															"scheduled_sending_time",
															input_time_to_send,
															true,
															&err)) {
		EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err);
		goto FINISH_OFF;
	}

	/* add alarm */
	if ((err = emcore_add_alarm(multi_user_name,
								input_time_to_send,
								EMAIL_ALARM_CLASS_SCHEDULED_SENDING,
								input_mail_id,
								emcore_sending_alarm_cb,
								NULL)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_add_alarm failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (mail_data)
		emstorage_free_mail(&mail_data, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
/* Scheduled sending ------------------------------------------------ */

#ifdef __FEATURE_AUTO_RETRY_SEND__

static int emcore_auto_resend_cb(email_alarm_data_t *alarm_data, void *user_parameter)
{
	EM_DEBUG_FUNC_BEGIN("alarm_data [%p] user_parameter [%p]", alarm_data, user_parameter);
	int err = EMAIL_ERROR_NONE;
	char *conditional_clause_string = NULL;
	char *attribute_field_name = NULL;
	email_list_filter_t filter_list[5];
	email_mail_list_item_t *result_mail_list = NULL;
	email_list_sorting_rule_t sorting_rule_list[2];
	int filter_rule_count = 5;
	int sorting_rule_count = 2;
	int result_mail_count = 0;
	int i = 0;
	char *multi_user_name = (char *)user_parameter;

	/* Get all mails have remaining resend counts in outbox with status 'EMAIL_MAIL_STATUS_SEND_FAILURE or EMAIL_MAIL_STATUS_SEND_WAIT' */
	attribute_field_name = emcore_get_mail_field_name_by_attribute_type(EMAIL_MAIL_ATTRIBUTE_REMAINING_RESEND_TIMES);

	memset(filter_list, 0 , sizeof(email_list_filter_t) * filter_rule_count);
	memset(sorting_rule_list, 0 , sizeof(email_list_sorting_rule_t) * sorting_rule_count);

	filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_REMAINING_RESEND_TIMES;
	filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_GREATER_THAN;
	filter_list[0].list_filter_item.rule.key_value.integer_type_value  = 0;

	filter_list[1].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[1].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_AND;

	filter_list[2].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[2].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[2].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[2].list_filter_item.rule.key_value.integer_type_value  = EMAIL_MAILBOX_TYPE_OUTBOX;

	filter_list[3].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[3].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_AND;

	filter_list[4].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[4].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS;
	filter_list[4].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[4].list_filter_item.rule.key_value.integer_type_value  = EMAIL_MAIL_STATUS_SEND_FAILURE;

	sorting_rule_list[0].target_attribute                              = EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID;
	sorting_rule_list[0].sort_order                                    = EMAIL_SORT_ORDER_ASCEND;

	sorting_rule_list[1].target_attribute                              = EMAIL_MAIL_ATTRIBUTE_MAIL_ID;
	sorting_rule_list[1].sort_order                                    = EMAIL_SORT_ORDER_ASCEND;

	if( (err = emstorage_write_conditional_clause_for_getting_mail_list(multi_user_name, filter_list, filter_rule_count, sorting_rule_list, sorting_rule_count, -1, -1, &conditional_clause_string)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_write_conditional_clause_for_getting_mail_list failed[%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("conditional_clause_string[%s].", conditional_clause_string);

	if(!emstorage_query_mail_list(multi_user_name, conditional_clause_string, true, &result_mail_list, &result_mail_count, &err) && !result_mail_list) {
		EM_DEBUG_LOG("There is no mails to be sent [%d]", err);
		goto FINISH_OFF;
	}

	/* Send mails in loop */

	for(i = 0; i < result_mail_count; i++) {
		if(!emcore_send_mail(multi_user_name, result_mail_list[i].mail_id, &err)) {
			EM_DEBUG_EXCEPTION("emcore_send_mail failed [%d]", err);
		}

		if(attribute_field_name) {
			if(!emstorage_set_field_of_mails_with_integer_value(multi_user_name, result_mail_list[i].account_id, &(result_mail_list[i].mail_id), 1, attribute_field_name, result_mail_list[i].remaining_resend_times - 1, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err);
			}
		}
	}

FINISH_OFF:
	EM_SAFE_FREE (conditional_clause_string); /* detected by valgrind */
	EM_SAFE_FREE(result_mail_list);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_create_alarm_for_auto_resend(char *multi_user_name, int input_alarm_interval_in_second)
{
	EM_DEBUG_FUNC_BEGIN("input_alarm_interval_in_second[%d]", input_alarm_interval_in_second);
	int err = EMAIL_ERROR_NONE;
	time_t current_time;
	time_t trigger_at_time;
	char *conditional_clause_string = NULL;
	email_list_filter_t filter_list[5];
	email_mail_list_item_t *result_mail_list = NULL;
	email_list_sorting_rule_t sorting_rule_list[2];
	int filter_rule_count = 5;
	int sorting_rule_count = 2;
	int result_mail_count = 0;

	if(input_alarm_interval_in_second <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* Check whether the alarm is already existing */
	if(emcore_check_alarm_by_class_id(EMAIL_ALARM_CLASS_AUTO_RESEND) == EMAIL_ERROR_NONE) {
		/* already exist */
		err = EMAIL_ERROR_ALREADY_EXISTS;
		goto FINISH_OFF;
	}
	/* Get all mails have remaining resend counts in outbox with status 'EMAIL_MAIL_STATUS_SEND_FAILURE or EMAIL_MAIL_STATUS_SEND_WAIT' */

	memset(filter_list, 0 , sizeof(email_list_filter_t) * filter_rule_count);
	memset(sorting_rule_list, 0 , sizeof(email_list_sorting_rule_t) * sorting_rule_count);

	filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_REMAINING_RESEND_TIMES;
	filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_GREATER_THAN;
	filter_list[0].list_filter_item.rule.key_value.integer_type_value  = 0;

	filter_list[1].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[1].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_AND;

	filter_list[2].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[2].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[2].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[2].list_filter_item.rule.key_value.integer_type_value  = EMAIL_MAILBOX_TYPE_OUTBOX;

	filter_list[3].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[3].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_AND;

	filter_list[4].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[4].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS;
	filter_list[4].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[4].list_filter_item.rule.key_value.integer_type_value  = EMAIL_MAIL_STATUS_SEND_FAILURE;

	sorting_rule_list[0].target_attribute                              = EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID;
	sorting_rule_list[0].sort_order                                    = EMAIL_SORT_ORDER_ASCEND;

	sorting_rule_list[1].target_attribute                              = EMAIL_MAIL_ATTRIBUTE_MAIL_ID;
	sorting_rule_list[1].sort_order                                    = EMAIL_SORT_ORDER_ASCEND;

	if( (err = emstorage_write_conditional_clause_for_getting_mail_list(multi_user_name, filter_list, filter_rule_count, sorting_rule_list, sorting_rule_count, -1, -1, &conditional_clause_string)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_write_conditional_clause_for_getting_mail_list failed[%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("conditional_clause_string[%s].", conditional_clause_string);

	if(!emstorage_query_mail_list(multi_user_name, conditional_clause_string, true, &result_mail_list, &result_mail_count, &err) && !result_mail_list) {
		if (err == EMAIL_ERROR_MAIL_NOT_FOUND)
			EM_DEBUG_LOG ("no mail found");
		else
			EM_DEBUG_EXCEPTION("emstorage_query_mail_list [%d]", err);
		goto FINISH_OFF;
	}

	if(result_mail_count > 0) {
		/* Add alarm */
		time(&current_time);

		trigger_at_time = current_time + input_alarm_interval_in_second;

		if ((err = emcore_add_alarm(multi_user_name, trigger_at_time, EMAIL_ALARM_CLASS_AUTO_RESEND, 0, emcore_auto_resend_cb, NULL)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_add_alarm failed [%d]",err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	EM_SAFE_FREE(result_mail_list);
	EM_SAFE_FREE(conditional_clause_string);
	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}
#endif /* __FEATURE_AUTO_RETRY_SEND__ */
