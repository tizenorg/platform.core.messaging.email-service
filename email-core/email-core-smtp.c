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
#include <alarm.h>

#include "email-internal-types.h"
#include "c-client.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-storage.h"
#include "email-core-api.h"
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

#undef min



#ifdef __FEATURE_SUPPORT_REPORT_MAIL__
static int emcore_get_report_mail_body(ENVELOPE *envelope, BODY **multipart_body, int *err_code);
#endif
static int emcore_make_envelope_from_mail(emstorage_mail_tbl_t *input_mail_tbl_data, ENVELOPE **output_envelope);
static int emcore_send_mail_smtp(SENDSTREAM *stream, ENVELOPE *env, char *data_file, int account_id, int mail_id,  int *err_code);

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
static char *emcore_replace_inline_image_path_with_content_id(char *source_string, BODY *html_body, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("source_string[%p], html_body[%p], err_code[%p]", source_string, html_body, err_code);

	int  err = EMAIL_ERROR_NONE;
	char content_id_buffer[CONTENT_ID_BUFFER_SIZE], file_name_buffer[512], new_string[512], *result_string = NULL, *input_string = NULL;
	BODY *cur_body = NULL;
	PART *cur_part = NULL;

	if (!source_string || !html_body) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	input_string = EM_SAFE_STRDUP(source_string);

	cur_part = html_body->nested.part;

	while (cur_part) {
		cur_body = &(cur_part->body);
		result_string = NULL; /*prevent 37865*/
		if (cur_body) {
			EM_DEBUG_LOG("Has body. id[%s]", cur_body->disposition.type);
			if (cur_body->disposition.type && cur_body->disposition.type[0] == 'i') {   /*  Is inline content? */
				EM_DEBUG_LOG("Has inline content");
				memset(content_id_buffer, 0, CONTENT_ID_BUFFER_SIZE);
				if (cur_body->id) {
					EM_SAFE_STRNCPY(content_id_buffer, cur_body->id + 1, CONTENT_ID_BUFFER_SIZE - 1); /*  Removing <, > */
					/* prevent 34413 */
					char *last_bracket = rindex(content_id_buffer, '>');
					*last_bracket = NULL_CHAR;

					/* if (emcore_get_attribute_value_of_body_part(cur_body->parameter, "name", file_name_buffer, CONTENT_ID_BUFFER_SIZE, false, &err)) { */
					if (emcore_get_attribute_value_of_body_part(cur_body->parameter, "name", file_name_buffer, CONTENT_ID_BUFFER_SIZE, true, &err)) {
						EM_DEBUG_LOG("Content-ID[%s], filename[%s]", content_id_buffer, file_name_buffer);
						SNPRINTF(new_string, CONTENT_ID_BUFFER_SIZE, "cid:%s", content_id_buffer);
						result_string = em_replace_string(input_string, file_name_buffer, new_string);
					}
				}
			}
		}
		if (result_string) {
			EM_SAFE_FREE(input_string);
			input_string = result_string;
		}
		cur_part = cur_part->next;
	}

	if (input_string)
		result_string = EM_SAFE_STRDUP(input_string);
FINISH_OFF:

	EM_SAFE_FREE(input_string);
	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret[%s]", result_string);
	return result_string;
}

static int emcore_write_body(BODY *body, BODY *html_body, FILE *fp, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("fp[%d]", fp);
	char *file_path = NULL;
	char buf[RFC822_STRING_BUFFER_SIZE + 1];
	char *img_tag_pos = NULL;
	char *p = NULL;
	char *replaced_string = NULL;
	int fd, nread, nwrite, error = EMAIL_ERROR_NONE;
	unsigned long len;

	file_path = body->sparep;

	if (!file_path || EM_SAFE_STRLEN(file_path) == 0)  {
		EM_DEBUG_LOG("There is no file path");
		switch (body->encoding)  {
			case 0:
				break;
			default:
				p = cpystr((const char *)body->contents.text.data);
				len = body->contents.text.size;
				break;
		}

		if (p)  {
			EM_DEBUG_LOG("p[%s]", p);
			fprintf(fp, "%s"CRLF_STRING CRLF_STRING, p);
			EM_SAFE_FREE(p);
		}

		EM_SAFE_FREE(body->sparep);
		EM_DEBUG_FUNC_END();
		return true;
	}

	EM_DEBUG_LOG("Opening a file[%s]", file_path);
	fd = open(file_path, O_RDONLY);

	if (fd < 0) {
		EM_DEBUG_EXCEPTION("open(\"%s\") failed...", file_path);
		return false;
	}

	while (1) {
		memset(&buf, 0x00, RFC822_STRING_BUFFER_SIZE + 1);
		nread = read(fd, buf, (body->encoding == ENCBASE64 ? 57 : RFC822_READ_BLOCK_SIZE - 2));

		if (nread <= 0)  {
			EM_DEBUG_LOG("Can't read anymore : nread[%d]", nread);
			break;
		}

		p = NULL;
		len = nread;

		/* EM_DEBUG_LOG("body->type[%d], body->subtype[%c]", body->type, body->subtype[0]); */

		if (body->type == TYPETEXT && (body->subtype && (body->subtype[0] == 'H' || body->subtype[0] == 'h'))) {
			EM_DEBUG_LOG("HTML Part");
			img_tag_pos = emcore_find_img_tag(buf);

			if (img_tag_pos) {
				replaced_string = emcore_replace_inline_image_path_with_content_id(buf, html_body, &error);
				if (replaced_string) {
					EM_DEBUG_LOG("emcore_replace_inline_image_path_with_content_id succeeded");
					strcpy(buf, replaced_string);
					nread = len = EM_SAFE_STRLEN(buf);
					EM_DEBUG_LOG("buf[%s], nread[%d], len[%d]", buf, nread, len);
				}
				else
					EM_DEBUG_LOG("emcore_replace_inline_image_path_with_content_id failed[%d]", error);
			}
		}
		/* EM_DEBUG_LOG("body->encoding[%d]", body->encoding); */
//		if (body->subtype[0] != 'S' || body->subtype[0] != 's') {
		switch (body->encoding)  {
			case ENCQUOTEDPRINTABLE:
				p = (char *)rfc822_8bit((unsigned char *)buf, (unsigned long)nread, (unsigned long *)&len);
				break;
			case ENCBASE64:
				p = (char *)rfc822_binary((void *)buf, (unsigned long)nread, (unsigned long *)&len);
				break;
			default:
				buf[len] = '\0';
				break;
		}
//		}

		nwrite = fprintf(fp, "%s", (p ? p : buf));
		if (nwrite != len)  {
			fclose(fp);
			close(fd);
			EM_SAFE_FREE(p);
			EM_DEBUG_EXCEPTION("fprintf failed nwrite[%d], len[%d]", nwrite, len);
			return false;
		}
		EM_SAFE_FREE(p);
	}

	if (body->encoding == ENCQUOTEDPRINTABLE || body->encoding == ENCBASE64)
		fprintf(fp, CRLF_STRING);

	fprintf(fp, CRLF_STRING);

	if (body->sparep)  {
		free(body->sparep);
		body->sparep = NULL;
	}

	close(fd);

	EM_DEBUG_FUNC_END();
	return true;
}

static int emcore_write_rfc822_body(BODY *body, BODY *html_body, FILE *fp, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("body[%p], html_body[%p], fp[%p], err_code[%p]", body, html_body, fp, err_code);

	PARAMETER *param = NULL;
	PART *part = NULL;
	char *p = NULL, *bndry = NULL, buf[1025];
	int error = EMAIL_ERROR_NONE;

	switch (body->type)  {
		case TYPEMULTIPART:
			EM_DEBUG_LOG("body->type = TYPEMULTIPART");
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

				fprintf(fp, "--%s"CRLF_STRING, bndry);
				if (body->subtype[0] == 'S' || body->subtype[0] == 's') {
					if (!emcore_write_body(body, html_body, fp, &error)) {
						EM_DEBUG_EXCEPTION("emcore_write_body failed : [%d]", error);
						return false;
					}
					fprintf(fp, "--%s"CRLF_STRING, bndry);
				}

				fprintf(fp, "%s"CRLF_STRING, buf);

				emcore_write_rfc822_body(&part->body, html_body, fp, err_code);
			} while ((part = part->next));

			fprintf(fp, "--%s--"CRLF_STRING, bndry);
			break;

		default:  {
			EM_DEBUG_LOG("body->type is not TYPEMULTIPART");

			if (!emcore_write_body(body, html_body, fp, &error)) {
				EM_DEBUG_EXCEPTION("emcore_write_body failed : [%d]", error);
				return false;
			}

			break;
		}	/*  default: */
	}
	EM_DEBUG_FUNC_END();
	return true;
}

static int emcore_write_rfc822(ENVELOPE *env, BODY *body, BODY *html_body, email_mail_priority_t input_priority, email_mail_report_t input_report_flag, char **data, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("env[%p], body[%p], data[%p], err_code[%p]", env, body, data, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;

	FILE *fp = NULL;
	char *fname = NULL;
	char *p = NULL;
	size_t p_len = 0;

	if (!env || !data)  {
		EM_DEBUG_EXCEPTION("Invalid Parameters");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	srand(time(NULL));

	rfc822_encode_body_7bit(env, body); /*  if contents.text.data isn't NULL, the data will be encoded. */

	/*  FIXME : create memory map for this file */
	p_len = (env->subject ? EM_SAFE_STRLEN(env->subject) : 0) + 8192;

	if (!(p = em_malloc(p_len)))  {		/* (env->subject ? EM_SAFE_STRLEN(env->subject) : 0) + 8192))) */
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}


	RFC822BUFFER buf;

	/* write at start of buffer */
	buf.end = (buf.beg = buf.cur = p) + p_len - 1;
	/* buf.f = NIL; */
	buf.f = buf_flush;
	buf.s = NIL;

	/*  rfc822_output_header(&buf, env, body, NIL, T); */		/*  including BCC  */
	rfc822_output_header(&buf, env, body, NIL, NIL);		/*  Excluding BCC */

	*buf.cur = '\0';		/* tie off buffer */
	{
		gchar **tokens = g_strsplit(p, "CHARSET=X-UNKNOWN", 2);

		if (g_strv_length(tokens) > 1)  {
			gchar *charset;

			if (body->sparep) {
				charset = g_path_get_basename(body->sparep);
				char *pHtml = NULL;
				if (charset != NULL) {
					if ((pHtml = strstr(charset, ".htm")) != NULL)
						charset[pHtml-charset] = '\0';
				}

				SNPRINTF(p, p_len, "%sCHARSET=%s%s", tokens[0], charset, tokens[1]);
				g_free(charset);
			}
			else
				EM_DEBUG_EXCEPTION("body->sparep is NULL");
		}

		g_strfreev(tokens);
	} {
		gchar **tokens = g_strsplit(p, "To: undisclosed recipients: ;\015\012", 2);
		if (g_strv_length(tokens) > 1)
			SNPRINTF(p, p_len, "%s%s", tokens[0], tokens[1]);
		g_strfreev(tokens);
	}


	EM_DEBUG_LOG(" =============================================================================== "
		LF_STRING"%s"LF_STRING
		" =============================================================================== ", p);

	if (EM_SAFE_STRLEN(p) > 2)
		*(p + EM_SAFE_STRLEN(p) - 2) = '\0';


	if (input_report_flag)  {
		char buf[512] = {0x00, };

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
			rfc822_address(buf, env->from);
			if (EM_SAFE_STRLEN(buf))
				SNPRINTF(p + EM_SAFE_STRLEN(p), p_len-(EM_SAFE_STRLEN(p)), "Disposition-Notification-To: %s"CRLF_STRING, buf);
		}
	}

	if (input_priority)  {		/*  priority (1:high 3:normal 5:low) */
		SNPRINTF(p + EM_SAFE_STRLEN(p), p_len-(EM_SAFE_STRLEN(p)), "X-Priority: %d"CRLF_STRING, input_priority);

		switch (input_priority)  {
			case EMAIL_MAIL_PRIORITY_HIGH:
				SNPRINTF(p + EM_SAFE_STRLEN(p), p_len-(EM_SAFE_STRLEN(p)), "X-MSMail-Priority: HIgh"CRLF_STRING);
				break;
			case EMAIL_MAIL_PRIORITY_NORMAL:
				SNPRINTF(p + EM_SAFE_STRLEN(p), p_len-(EM_SAFE_STRLEN(p)), "X-MSMail-Priority: Normal"CRLF_STRING);
				break;
			case EMAIL_MAIL_PRIORITY_LOW:
				SNPRINTF(p + EM_SAFE_STRLEN(p), p_len-(EM_SAFE_STRLEN(p)), "X-MSMail-Priority: Low"CRLF_STRING);
				break;
		}
	}

	SNPRINTF(p + EM_SAFE_STRLEN(p), p_len-(EM_SAFE_STRLEN(p)), CRLF_STRING);

	if (!emcore_get_temp_file_name(&fname, &error))  {
		EM_DEBUG_EXCEPTION(" emcore_get_temp_file_name failed[%d]", error);
		goto FINISH_OFF;
	}

	if (!(fp = fopen(fname, "w+")))  {
		EM_DEBUG_EXCEPTION("fopen failed[%s]", fname);
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	fprintf(fp, "%s", p);

	if (body)  {
		if (!emcore_write_rfc822_body(body, html_body, fp, &error))  {
			EM_DEBUG_EXCEPTION("emcore_write_rfc822_body failed[%d]", error);
			goto FINISH_OFF;
		}
	}

	ret = true;


FINISH_OFF:
	if (fp != NULL)
		fclose(fp);

#ifdef USE_SYNC_LOG_FILE
	emstorage_copy_file(fname, "/tmp/phone2pc.eml", false, NULL);
#endif

	if (ret == true)
		*data = fname;
	else if (fname != NULL)  {
		remove(fname);
		EM_SAFE_FREE(fname);
	}

	EM_SAFE_FREE(p);

	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_add_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t *input_meeting_request, int input_from_eas)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list [%p], input_attachment_count [%d], input_meeting_request [%p], input_from_eas[%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas);

	int err = EMAIL_ERROR_NONE;
	int attachment_id = 0, thread_id = -1, thread_item_count = 0, latest_mail_id_in_thread = -1;
	int i = 0, rule_len, rule_matched = -1, local_attachment_count = 0, local_inline_content_count = 0;
	int mailbox_id_spam = 0, mailbox_id_target = 0;
	char *ext = NULL;
	char name_buf[MAX_PATH] = {0x00, };
	char *body_text_file_name = NULL;

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

	/* Validating parameters */
	if (!input_mail_data || !(input_mail_data->account_id) || !(input_mail_data->mailbox_id))  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (emcore_is_storage_full(&err) == true) {
		EM_DEBUG_EXCEPTION("Storage is full");
		goto FINISH_OFF;
	}

	if (!emstorage_get_account_by_id(input_mail_data->account_id, EMAIL_ACC_GET_OPT_DEFAULT | EMAIL_ACC_GET_OPT_OPTIONS, &account_tbl_item, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed. account_id[%d] err[%d]", input_mail_data->account_id, err);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (input_from_eas == 0 && input_mail_data->smime_type && input_mail_data->mailbox_type != EMAIL_MAILBOX_TYPE_DRAFT) {
		if (!emcore_convert_mail_data_to_smime_data(account_tbl_item, input_mail_data, input_attachment_data_list, input_attachment_count, &mail_data, &attachment_data_list, &attachment_count)) {
			EM_DEBUG_EXCEPTION("S/MIME failed");
			goto FINISH_OFF;
		}
	} else {
		mail_data = input_mail_data;
		attachment_data_list = input_attachment_data_list;
		attachment_count = input_attachment_count;
	}

	mailbox_id_target = mail_data->mailbox_id;

	if (input_from_eas == 0 &&
			!(input_mail_data->message_class & EMAIL_MESSAGE_CLASS_SMART_REPLY) &&
			!(input_mail_data->message_class & EMAIL_MESSAGE_CLASS_SMART_FORWARD) ) {
		if (mail_data->file_path_plain)  {
			if (stat(mail_data->file_path_plain, &st_buf) < 0)  {
				EM_DEBUG_EXCEPTION("mail_data->file_path_plain, stat(\"%s\") failed...", mail_data->file_path_plain);
				err = EMAIL_ERROR_INVALID_MAIL;
				goto FINISH_OFF;
			}
		}

		if (mail_data->file_path_html)  {
			if (stat(mail_data->file_path_html, &st_buf) < 0)  {
				EM_DEBUG_EXCEPTION("mail_data->file_path_html, stat(\"%s\") failed...", mail_data->file_path_html);
				err = EMAIL_ERROR_INVALID_MAIL;
				goto FINISH_OFF;
			}
		}

		if (attachment_count && attachment_data_list)  {
			for (i = 0; i < attachment_count; i++)  {
				if (attachment_data_list[i].save_status) {
					if (!attachment_data_list[i].attachment_path || stat(attachment_data_list[i].attachment_path, &st_buf) < 0)  {
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
		if (!em_verify_email_address_of_mail_data(mail_data, false, &err)) {
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
			if (!emcore_make_rfc822_file_from_mail(mail_src, NULL, NULL, NULL, &err))  {
				EM_DEBUG_EXCEPTION("emcore_make_rfc822_file_from_mail failed [%d]", err);
				goto FINISH_OFF;
			}
			*/
		}
	}
	else {	/*  For Spam handling */
		email_option_t *opt = &account_tbl_item->options;
		EM_DEBUG_LOG("block_address [%d], block_subject [%d]", opt->block_address, opt->block_subject);

		if (opt->block_address || opt->block_subject)  {
			int is_completed = false;
			int type = 0;

			if (!opt->block_address)
				type = EMAIL_FILTER_SUBJECT;
			else if (!opt->block_subject)
				type = EMAIL_FILTER_FROM;

			if (!emstorage_get_rule(ALL_ACCOUNT, type, 0, &rule_len, &is_completed, &rule, true, &err) || !rule)
				EM_DEBUG_LOG("No proper rules. emstorage_get_rule returns [%d]", err);
		}

		if (rule) {
			if (!emstorage_get_mailbox_id_by_mailbox_type(mail_data->account_id, EMAIL_MAILBOX_TYPE_SPAMBOX, &mailbox_id_spam, false, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_get_mailbox_name_by_mailbox_type failed [%d]", err);
				mailbox_id_spam = 0;
			}

			if (mailbox_id_spam && !emcore_check_rule(mail_data->full_address_from, mail_data->subject, rule, rule_len, &rule_matched, &err))  {
				EM_DEBUG_EXCEPTION("emcore_check_rule failed [%d]", err);
				goto FINISH_OFF;
			}
		}

		if (rule_matched >= 0 && mailbox_id_spam)
			mailbox_id_target = mailbox_id_spam;
	}

	if ((err = emstorage_get_mailbox_by_id(mailbox_id_target, (emstorage_mailbox_tbl_t**)&mailbox_tbl)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emstorage_increase_mail_id(&mail_data->mail_id, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_increase_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("mail_data->mail_size [%d]", mail_data->mail_size);

	if(mail_data->mail_size == 0)
		emcore_calc_mail_size(mail_data, attachment_data_list, attachment_count, &(mail_data->mail_size)); /*  Getting file size before file moved.  */

	EM_DEBUG_LOG("input_from_eas [%d] mail_data->body_download_status [%d]", input_from_eas, mail_data->body_download_status);

	if (input_from_eas == 0|| mail_data->body_download_status) {
		if (!emstorage_create_dir(mail_data->account_id, mail_data->mail_id, 0, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (mail_data->file_path_plain) {
			EM_DEBUG_LOG("mail_data->file_path_plain [%s]", mail_data->file_path_plain);
			/* EM_SAFE_STRNCPY(body_text_file_name, "UTF-8", MAX_PATH); */

			if ( (err = em_get_file_name_from_file_path(mail_data->file_path_plain, &body_text_file_name)) != EMAIL_ERROR_NONE) {
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

			if (!emstorage_get_save_name(mail_data->account_id, mail_data->mail_id, 0, body_text_file_name, name_buf, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
				goto FINISH_OFF;
			}

			if (!emstorage_move_file(mail_data->file_path_plain, name_buf, input_from_eas, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
				goto FINISH_OFF;
			}
			if (mail_data->body_download_status == EMAIL_BODY_DOWNLOAD_STATUS_NONE)
				mail_data->body_download_status = EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;

			EM_SAFE_FREE(mail_data->file_path_plain);
			mail_data->file_path_plain = EM_SAFE_STRDUP(name_buf);
		}

		if (mail_data->file_path_html) {
			EM_DEBUG_LOG("mail_data->file_path_html [%s]", mail_data->file_path_html);
			/* EM_SAFE_STRNCPY(body_text_file_name, "UTF-8.htm", MAX_PATH); */

			EM_SAFE_FREE(body_text_file_name);

			if ( (err = em_get_file_name_from_file_path(mail_data->file_path_html, &body_text_file_name)) != EMAIL_ERROR_NONE) {
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

			if (!emstorage_get_save_name(mail_data->account_id, mail_data->mail_id, 0, body_text_file_name, name_buf, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
				goto FINISH_OFF;
			}

			if (!emstorage_move_file(mail_data->file_path_html, name_buf, input_from_eas, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
				goto FINISH_OFF;
			}

			if (mail_data->body_download_status == EMAIL_BODY_DOWNLOAD_STATUS_NONE)
				mail_data->body_download_status = EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;

			EM_SAFE_FREE(mail_data->file_path_html);
			mail_data->file_path_html = EM_SAFE_STRDUP(name_buf);
		}
	}

	if (mail_data->file_path_mime_entity) {
		EM_DEBUG_LOG("mail_data->file_path_mime_entity [%s]", mail_data->file_path_mime_entity);

		if (!emstorage_get_save_name(mail_data->account_id, mail_data->mail_id, 0, "mime_entity", name_buf, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(mail_data->file_path_mime_entity, name_buf, input_from_eas, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_SAFE_FREE(mail_data->file_path_mime_entity);
		mail_data->file_path_mime_entity = EM_SAFE_STRDUP(name_buf);
	}

	if (!mail_data->date_time)  {
		/* time isn't set */
		mail_data->date_time = time(NULL);
	}


	mail_data->mailbox_id           = mailbox_id_target;
	mail_data->mailbox_type         = mailbox_tbl->mailbox_type;
	mail_data->server_mail_status   = !input_from_eas;
	mail_data->save_status          = EMAIL_MAIL_STATUS_SAVED;

	/*  Getting attachment count */
	for (i = 0; i < attachment_count; i++) {
		if (attachment_data_list[i].inline_content_status== 1)
			local_inline_content_count++;
		local_attachment_count++;
	}

	mail_data->inline_content_count = local_inline_content_count;
	mail_data->attachment_count     = local_attachment_count;

	EM_DEBUG_LOG("inline_content_count   [%d]", local_inline_content_count);
	EM_DEBUG_LOG("attachment_count [%d]", local_attachment_count);

	EM_DEBUG_LOG("preview_text[%p]", mail_data->preview_text);
	if (mail_data->preview_text == NULL) {
		if ( (err = emcore_get_preview_text_from_file(mail_data->file_path_plain, mail_data->file_path_html, MAX_PREVIEW_TEXT_LENGTH, &(mail_data->preview_text))) != EMAIL_ERROR_NONE) {
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
	emcore_fill_address_information_of_mail_tbl(converted_mail_tbl);

	/* Fill thread id */
	if(mail_data->thread_id == 0) {
		if (emstorage_get_thread_id_of_thread_mails(converted_mail_tbl, &thread_id, &latest_mail_id_in_thread, &thread_item_count) != EMAIL_ERROR_NONE)
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

	emstorage_begin_transaction(NULL, NULL, NULL);

	/*  insert mail to mail table */
	if (!emstorage_add_mail(converted_mail_tbl, 0, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_add_mail failed [%d]", err);
		/*  ROLLBACK TRANSACTION; */
		emstorage_rollback_transaction(NULL, NULL, NULL);

		goto FINISH_OFF;
	}

	/* Update thread information */
	EM_DEBUG_LOG("thread_item_count [%d]", thread_item_count);

	if (thread_item_count > 1) {
		if (!emstorage_update_latest_thread_mail(mail_data->account_id, converted_mail_tbl->thread_id, 0, 0, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_update_latest_thread_mail failed [%d]", err);
			emstorage_rollback_transaction(NULL, NULL, NULL);

			goto FINISH_OFF;
		}
	}

	/*  Insert attachment information to DB */

	for (i = 0; i < attachment_count; i++) {
		if (attachment_data_list[i].attachment_size == 0) {
			/* set attachment size */
			if(attachment_data_list[i].attachment_path && stat(attachment_data_list[i].attachment_path, &st_buf) < 0)
				attachment_data_list[i].attachment_size = st_buf.st_size;
		}

		if (!attachment_data_list[i].inline_content_status) {
			if (!emstorage_get_new_attachment_no(&attachment_id, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_get_new_attachment_no failed [%d]", err);
				emstorage_rollback_transaction(NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}

		if (!emstorage_create_dir(mail_data->account_id, mail_data->mail_id, attachment_data_list[i].inline_content_status ? 0  :  attachment_id, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			emstorage_rollback_transaction(NULL, NULL, NULL);
			goto FINISH_OFF;
		}

		if (!emstorage_get_save_name(mail_data->account_id, mail_data->mail_id, attachment_data_list[i].inline_content_status ? 0  :  attachment_id, attachment_data_list[i].attachment_name, name_buf, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			emstorage_rollback_transaction(NULL, NULL, NULL);
			goto FINISH_OFF;
		}
		/* if (input_from_eas == 0 || attachment_data_list[i].save_status) { */
		if (attachment_data_list[i].save_status) {
			if (!emstorage_copy_file(attachment_data_list[i].attachment_path, name_buf, input_from_eas, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_copy_file failed [%d]", err);
				emstorage_rollback_transaction(NULL, NULL, NULL);
				goto FINISH_OFF;
			}

			if ((ext = strrchr(attachment_data_list[i].attachment_name, '.'))) {
				if (!strncmp(ext, ".vcs", strlen(".vcs")))
					remove(attachment_data_list[i].attachment_path);
				else if (!strncmp(ext, ".vcf", strlen(".vcf")))
					remove(attachment_data_list[i].attachment_path);
				else if (!strncmp(ext, ".vnt", strlen(".vnt")))
					remove(attachment_data_list[i].attachment_path);
			}
		}

		memset(&attachment_tbl, 0, sizeof(emstorage_attachment_tbl_t));
		attachment_tbl.attachment_name                  = attachment_data_list[i].attachment_name;
		attachment_tbl.attachment_path                  = name_buf;
		attachment_tbl.attachment_size                  = attachment_data_list[i].attachment_size;
		attachment_tbl.mail_id                          = mail_data->mail_id;
		attachment_tbl.account_id                       = mail_data->account_id;
		attachment_tbl.mailbox_id                       = mail_data->mailbox_id;
		attachment_tbl.attachment_save_status           = attachment_data_list[i].save_status;
		attachment_tbl.attachment_drm_type              = attachment_data_list[i].drm_status;
		attachment_tbl.attachment_inline_content_status = attachment_data_list[i].inline_content_status;

		if (!emstorage_add_attachment(&attachment_tbl, 0, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_add_attachment failed [%d]", err);
			emstorage_rollback_transaction(NULL, NULL, NULL);

			goto FINISH_OFF;
		}
		attachment_data_list[i].attachment_id = attachment_tbl.attachment_id;
	}

#ifdef __FEATURE_BODY_SEARCH__
	/* Insert mail_text to DB */
	char *stripped_text = NULL;
	if (!emcore_strip_mail_body_from_file(converted_mail_tbl, &stripped_text, &err) || stripped_text == NULL) {
		EM_DEBUG_EXCEPTION("emcore_strip_mail_body_from_file failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_add_mail_text(mailbox_tbl, converted_mail_tbl, stripped_text, &err)) {
		EM_DEBUG_EXCEPTION("emcore_add_mail_text failed [%d]", err);
		emstorage_rollback_transaction(NULL, NULL, NULL);
		goto FINISH_OFF;
	}
#endif

	/*  Insert Meeting request to DB */
	if (mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_REQUEST
		|| mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_RESPONSE
		|| mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		EM_DEBUG_LOG("This mail has the meeting request");
		input_meeting_request->mail_id = mail_data->mail_id;
		if (!emstorage_add_meeting_request(mail_data->account_id, mailbox_tbl->mailbox_id, input_meeting_request, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_add_meeting_request failed [%d]", err);

			goto FINISH_OFF;
		}
	}

	emstorage_commit_transaction(NULL, NULL, NULL);

	SNPRINTF(mailbox_id_param_string, 10, "%d", mailbox_tbl->mailbox_id);
	if (!emcore_notify_storage_event(NOTI_MAIL_ADD, converted_mail_tbl->account_id, converted_mail_tbl->mail_id, mailbox_id_param_string, converted_mail_tbl->thread_id))
		EM_DEBUG_LOG("emcore_notify_storage_event [NOTI_MAIL_ADD] failed.");

	if (account_tbl_item->incoming_server_type != EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
		if (!emcore_remove_overflowed_mails(mailbox_tbl, &err)) {
			if (err == EMAIL_ERROR_MAIL_NOT_FOUND || err == EMAIL_ERROR_NOT_SUPPORTED)
				err = EMAIL_ERROR_NONE;
			else
				EM_DEBUG_LOG("emcore_remove_overflowed_mails failed [%d]", err);
		}
	}

	if ( input_from_eas && (mail_data->flags_seen_field == 0)
				&& mail_data->mailbox_type != EMAIL_MAILBOX_TYPE_TRASH
				&& mail_data->mailbox_type != EMAIL_MAILBOX_TYPE_SPAMBOX) {
		if ((err = emcore_update_sync_status_of_account(mail_data->account_id, SET_TYPE_SET, SYNC_STATUS_SYNCING | SYNC_STATUS_HAVE_NEW_MAILS)) != EMAIL_ERROR_NONE)
				EM_DEBUG_LOG("emcore_update_sync_status_of_account failed [%d]", err);
//		emcore_add_notification_for_unread_mail(converted_mail_tbl);
		emcore_display_unread_in_badge();
	}

FINISH_OFF:

	EM_SAFE_FREE(body_text_file_name);

#ifdef __FEATURE_BODY_SEARCH__
	EM_SAFE_FREE(stripped_text);
#endif

	if (account_tbl_item)
		emstorage_free_account(&account_tbl_item, 1, NULL);

	if (mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

	if (converted_mail_tbl)
		emstorage_free_mail(&converted_mail_tbl, 1, NULL);

	EM_DEBUG_FUNC_END();
	return err;
}

INTERNAL_FUNC int emcore_add_read_receipt(int input_read_mail_id, int *output_receipt_mail_id)
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

	if( (err = emcore_get_mail_data(input_read_mail_id, &read_mail_data)) != EMAIL_ERROR_NONE) {
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

	if (!emstorage_get_mailbox_by_mailbox_type(receipt_mail_data->account_id, EMAIL_MAILBOX_TYPE_OUTBOX, &mailbox_tbl, true, &err))  {
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

	if ( (err = emcore_make_envelope_from_mail(receipt_mail_tbl_data, &envelope)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_make_envelope_from_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	envelope->references = EM_SAFE_STRDUP(read_mail_data->message_id);

	if (!emcore_get_report_mail_body(envelope, &root_body, &err))  {
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

	if ( (err = emcore_add_mail(receipt_mail_data, attachment_data, attachment_count, NULL, 0)) != EMAIL_ERROR_NONE) {
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

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_add_meeting_request(int account_id, int input_mailbox_id, email_meeting_request_t *meeting_req, int *err_code)
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

	if (!emstorage_add_meeting_request(account_id, input_mailbox_id, meeting_req, 1, &err))  {
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

/*  send a mail */
INTERNAL_FUNC int emcore_send_mail(int mail_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], err_code[%p]", mail_id, err_code);
	EM_PROFILE_BEGIN(profile_emcore_send_mail);
	int ret = false;
	int err = EMAIL_ERROR_NONE, err2 = EMAIL_ERROR_NONE;
	int status = EMAIL_SEND_FAIL;
	int attachment_tbl_count = 0;
	int i = 0;
	int account_id = 0;
	SENDSTREAM *stream = NULL;
	ENVELOPE *envelope = NULL;
	sslstart_t stls = NULL;
	emstorage_mail_tbl_t       *mail_tbl_data = NULL;
	emstorage_attachment_tbl_t *attachment_tbl_data = NULL;
	email_account_t *ref_account = NULL;
	email_option_t *opt = NULL;
	void *tmp_stream = NULL;
	char *fpath = NULL;
	int sent_box = 0;
	emstorage_mailbox_tbl_t* local_mailbox = NULL;
	int dst_mailbox_id = 0;

	if (!mail_id)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/*  get mail to send */
	if (!emstorage_get_mail_by_id(mail_id, &mail_tbl_data, false, &err) || err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	account_id = mail_tbl_data->account_id;

	if (!(ref_account = emcore_get_account_reference(account_id))) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_attachment_list(mail_id, false, &attachment_tbl_data, &attachment_tbl_count)) != EMAIL_ERROR_NONE) {
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
		if ((err = em_verify_email_address_of_mail_tbl(mail_tbl_data, false)) != EMAIL_ERROR_NONE) {
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
	EM_VALIDATION_SYSTEM_LOG("INFO", mail_id, "Email Send Start, %s -> %s, success", mail_tbl_data->full_address_from, mail_tbl_data->full_address_to);
	for (i = 0; i < attachment_tbl_count; i++) {
		if(attachment_tbl_data)
			EM_VALIDATION_SYSTEM_LOG("FILE", mail_id, "[%s], %d", attachment_tbl_data[i].attachment_path, attachment_tbl_data[i].attachment_size);
	}
#endif /* __FEATURE_SUPPORT_VALIDATION_SYSTEM__ */

	/*Update status flag to DB*/

	/*  get rfc822 data */
	if (!emcore_make_rfc822_file_from_mail(mail_tbl_data, attachment_tbl_data, attachment_tbl_count, &envelope, &fpath, opt, &err)) {
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
		if (!emcore_connect_to_remote_mailbox(account_id, 0, (void **)&tmp_stream, &err))  {
			EM_DEBUG_EXCEPTION(" POP before SMTP Authentication failed [%d]", err);
			status = EMAIL_LIST_CONNECTION_FAIL;
			if (err == EMAIL_ERROR_CONNECTION_BROKEN)
				err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}
	}
	if (!emstorage_get_mailbox_by_mailbox_type(account_id, EMAIL_MAILBOX_TYPE_DRAFT, &local_mailbox, false, &err))  {
		EM_DEBUG_EXCEPTION(" emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
		goto FINISH_OFF;
	}
	dst_mailbox_id = local_mailbox->mailbox_id;
	emstorage_free_mailbox(&local_mailbox, 1, NULL);


	if (!emcore_connect_to_remote_mailbox(account_id, EMAIL_CONNECT_FOR_SENDING, (void **)&tmp_stream, &err))  {
		EM_DEBUG_EXCEPTION(" emcore_connect_to_remote_mailbox failed [%d]", err);

		if (err == EMAIL_ERROR_CONNECTION_BROKEN)
			err = EMAIL_ERROR_CANCELLED;
		status = EMAIL_SEND_CONNECTION_FAIL;
		goto FINISH_OFF;
	}

	stream = (SENDSTREAM *)tmp_stream;

	if (!emcore_check_send_mail_thread_status()) {
		EM_DEBUG_EXCEPTION(" emcore_check_send_mail_thread_status failed...");
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	/*  set request of delivery status. */
	EM_DEBUG_LOG("opt->req_delivery_receipt [%d]", opt->req_delivery_receipt);
	EM_DEBUG_LOG("mail_tbl_data->report_status [%d]", mail_tbl_data->report_status);

	if (opt->req_delivery_receipt == EMAIL_OPTION_REQ_DELIVERY_RECEIPT_ON || (mail_tbl_data->report_status & EMAIL_MAIL_REQUEST_DSN))  {
		EM_DEBUG_LOG("DSN is required.");
		stream->protocol.esmtp.dsn.want = 1;
		stream->protocol.esmtp.dsn.full = 0;
		stream->protocol.esmtp.dsn.notify.failure = 1;
		stream->protocol.esmtp.dsn.notify.success = 1;
	}

	mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SENDING;

	/*Update status save_status to DB*/
	if (!emstorage_set_field_of_mails_with_integer_value(account_id, &mail_id, 1, "save_status", mail_tbl_data->save_status, false, &err))
		EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err);

	/*  send mail to server. */
	if (!emcore_send_mail_smtp(stream, envelope, fpath, account_id, mail_id, &err)) {
		EM_DEBUG_EXCEPTION(" emcore_send_mail_smtp failed [%d]", err);
#ifndef __FEATURE_MOVE_TO_OUTBOX_FIRST__
		if (!emstorage_get_mailbox_by_mailbox_type(account_id, EMAIL_MAILBOX_TYPE_OUTBOX, &local_mailbox, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
			goto FINISH_OFF;
		}
		dst_mailbox_id = local_mailbox->mailbox_id;
		emstorage_free_mailbox(&local_mailbox, 1, NULL);

		/*  unsent mail is moved to 'OUTBOX'. */
		if (!emcore_move_mail(&mail_id, 1, dst_mailbox_id, EMAIL_MOVED_BY_COMMAND, 0, NULL))
			EM_DEBUG_EXCEPTION(" emcore_mail_move falied...");
#endif
		goto FINISH_OFF;
	}

	/*  sent mail is moved to 'SENT' box or deleted. */
	if (opt->keep_local_copy)  {
		if (!emstorage_get_mailbox_by_mailbox_type(account_id, EMAIL_MAILBOX_TYPE_SENTBOX, &local_mailbox, true, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
			goto FINISH_OFF;
		}
		dst_mailbox_id = local_mailbox->mailbox_id;

		if (!emcore_move_mail(&mail_id, 1, dst_mailbox_id, EMAIL_MOVED_AFTER_SENDING, 0, &err))
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

			if (!emcore_move_mail_on_server(dest_mbox.account_id, dst_mailbox_id, &mail_id, 1, dest_mbox.name, &err)) {
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
			if (!emstorage_get_mailbox_by_mailbox_type(account_id, EMAIL_MAILBOX_TYPE_OUTBOX, &src_mailbox, true, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
				goto FINISH_OFF;
			}

			if (src_mailbox->local_yn)
				emcore_sync_mail_from_client_to_server(mail_id);
			else {
				if (!emcore_move_mail_on_server(account_id, src_mailbox->mailbox_id, &mail_id, 1, local_mailbox->mailbox_name, &err)) {
					EM_DEBUG_EXCEPTION(" emcore_move_mail_on_server falied [%d]", err);
				}
			}

			emstorage_free_mailbox(&src_mailbox, 1, NULL);
		}

		/* On Successful Mail sent remove the Draft flag */
		mail_tbl_data->flags_draft_field = 0;

		if (!emstorage_set_field_of_mails_with_integer_value(account_id, &mail_id, 1, "flags_draft_field", mail_tbl_data->flags_draft_field, false, &err))
			EM_DEBUG_EXCEPTION("Failed to modify extra flag [%d]", err);

		sent_box = 1;
	}
	else  {
		if (!emcore_delete_mail(account_id, &mail_id, 1, EMAIL_DELETE_LOCALLY, EMAIL_DELETED_AFTER_SENDING, false, &err))
			EM_DEBUG_EXCEPTION(" emcore_delete_mail failed [%d]", err);
	}

	/* Set the phone log */
	if ((err = emcore_set_sent_contacts_log(mail_tbl_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_set_sent_contacts_log failed : [%d]", err);
	}

	/*Update status save_status to DB*/
	mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SENT;
	if (!emstorage_set_field_of_mails_with_integer_value(account_id, &mail_id, 1, "save_status", mail_tbl_data->save_status, false, &err))
		EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err);

	if (!emcore_delete_transaction_info_by_mailId(mail_id))
		EM_DEBUG_EXCEPTION(" emcore_delete_transaction_info_by_mailId failed for mail_id[%d]", mail_id);

	ret = true;

FINISH_OFF:
	if (ret == false && err != EMAIL_ERROR_INVALID_PARAM && mail_tbl_data)  {
		if (err != EMAIL_ERROR_CANCELLED) {
			mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SEND_FAILURE;
			if (!emstorage_set_field_of_mails_with_integer_value(account_id, &mail_id, 1, "save_status", mail_tbl_data->save_status, false, &err2))
				EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err2);
		}
		else {
			if (EMAIL_MAIL_STATUS_SEND_CANCELED == mail_tbl_data->save_status)
				EM_DEBUG_LOG("EMAIL_MAIL_STATUS_SEND_CANCELED Already set for ");
			else {
				mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SEND_CANCELED;
				if (!emstorage_set_field_of_mails_with_integer_value(account_id, &mail_id, 1, "save_status", mail_tbl_data->save_status, false, &err2))
					EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err2);
			}
		}
	}

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

#ifndef __FEATURE_KEEP_CONNECTION__
	if (stream)
		smtp_close(stream);
#endif /* __FEATURE_KEEP_CONNECTION__ */
	if (stls)
		mail_parameters(NULL, SET_SSLSTART, (void  *)stls);

	if (attachment_tbl_data)
		emstorage_free_attachment(&attachment_tbl_data, attachment_tbl_count, NULL);

	if (envelope)
		mail_free_envelope(&envelope);

	if (fpath) {
		EM_DEBUG_LOG("REMOVE TEMP FILE  :  %s", fpath);
		remove(fpath);
		free(fpath);
	}

	if(local_mailbox)
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
		emcore_show_user_message(mail_id, EMAIL_ACTION_SEND_MAIL, err);
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
INTERNAL_FUNC int emcore_send_saved_mail(int account_id, char *input_mailbox_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], input_mailbox_name[%p], err_code[%p]", account_id, input_mailbox_name, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int status = EMAIL_SEND_FAIL;
	int *mail_ids = NULL;
	int handle = 0;
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


	if (!(ref_account = emcore_get_account_reference(account_id)))  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	FINISH_OFF_IF_CANCELED;

	opt = &(ref_account->options);

	/*  search mail. */
	if (!emstorage_mail_search_start(NULL, account_id, input_mailbox_name, 0, &handle, &total, true, &err))  {
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

	for (i = 0; i < total; i++) {
		FINISH_OFF_IF_CANCELED;

		if (!emstorage_get_mail_by_id(mail_ids[i], &searched_mail_tbl_data, false, &err)) {
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

		if ( (err = emstorage_get_attachment_list(mail_ids[i], false, &attachment_tbl_data, &attachment_tbl_count)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);
			goto FINISH_OFF;
		}

		/* check for email_address validation */
		if ( (err = em_verify_email_address_of_mail_tbl(searched_mail_tbl_data, false)) != EMAIL_ERROR_NONE ) {
			err = EMAIL_ERROR_INVALID_ADDRESS;
			EM_DEBUG_EXCEPTION("em_verify_email_address_of_mail_tbl failed [%d]", err);
			goto FINISH_OFF;
		}

		searched_mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SENDING;

		if (!emcore_make_rfc822_file_from_mail(searched_mail_tbl_data, attachment_tbl_data, attachment_tbl_count, &envelope, &fpath, opt, &err))  {
			EM_DEBUG_EXCEPTION("emcore_make_rfc822_file_from_mail falied [%d]", err);
			goto FINISH_OFF;
		}

		FINISH_OFF_IF_CANCELED;

		/*  connect mail server. */
		if (!stream) {
			/*  if there no security option, unset security. */
			if (!ref_account->outgoing_server_secure_connection) {
				stls = (sslstart_t)mail_parameters(NULL, GET_SSLSTART, NULL);
				mail_parameters(NULL, SET_SSLSTART, NULL);
			}

			stream = NULL;
			if (!emcore_connect_to_remote_mailbox(account_id, EMAIL_CONNECT_FOR_SENDING, &tmp_stream, &err) || !tmp_stream)  {
				EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);

				if (err == EMAIL_ERROR_CONNECTION_BROKEN)
					err = EMAIL_ERROR_CANCELLED;

				status = EMAIL_SEND_CONNECTION_FAIL;
				goto FINISH_OFF;
			}

			stream = (SENDSTREAM *)tmp_stream;

			FINISH_OFF_IF_CANCELED;

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
		if (!emstorage_change_mail_field(mail_ids[i], UPDATE_EXTRA_FLAG, searched_mail_tbl_data, true, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);

			goto FINISH_OFF;
		}

		if (!emcore_send_mail_smtp(stream, envelope, fpath, account_id, mail_ids[i], &err))  {
			EM_DEBUG_EXCEPTION("emcore_send_mail_smtp failed [%d]", err);

			searched_mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SEND_FAILURE;

			/*  update mail status to failure. */
			if (!emstorage_change_mail_field(mail_ids[i], UPDATE_EXTRA_FLAG, searched_mail_tbl_data, true, &err))
				EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);

			if (!emstorage_get_mailbox_by_mailbox_type(account_id, EMAIL_MAILBOX_TYPE_OUTBOX, &local_mailbox, true, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
				goto FINISH_OFF;
			}
			dst_mailbox_id = local_mailbox->mailbox_id;

			emcore_move_mail(&mail_ids[i], 1, dst_mailbox_id, EMAIL_MOVED_AFTER_SENDING, 0, NULL);

			if(local_mailbox)
				emstorage_free_mailbox(&local_mailbox, 1, NULL);

			goto FINISH_OFF;
		}

		searched_mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SENT;

		/*  update mail status to sent mail. */
		if (!emstorage_change_mail_field(mail_ids[i], UPDATE_EXTRA_FLAG, searched_mail_tbl_data, true, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);
			goto FINISH_OFF;
		}

		/*  sent mail is moved to 'SENT' box or deleted. */
		if (opt->keep_local_copy)  {
			if (!emstorage_get_mailbox_by_mailbox_type(account_id, EMAIL_MAILBOX_TYPE_SENTBOX, &local_mailbox, true, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
				goto FINISH_OFF;
			}
			dst_mailbox_id = local_mailbox->mailbox_id;

			if (!emcore_move_mail(&mail_ids[i], 1, dst_mailbox_id, EMAIL_MOVED_AFTER_SENDING, 0, &err))
				EM_DEBUG_EXCEPTION("emcore_mail_move falied [%d]", err);

			if(local_mailbox)
				emstorage_free_mailbox(&local_mailbox, 1, NULL);
		}
		else  {
			if (!emcore_delete_mail(account_id, &mail_ids[i], 1, EMAIL_DELETE_LOCALLY, EMAIL_DELETED_AFTER_SENDING, false, &err))
				EM_DEBUG_EXCEPTION("emcore_delete_mail falied [%d]", err);
		}

		/* Set the phone log */
		if ((err = emcore_set_sent_contacts_log(searched_mail_tbl_data)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_set_sent_contacts_log failed : [%d]", err);
		}

		if(searched_mail_tbl_data) {
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
		if (!emstorage_mail_search_end(handle, true, &err))
			EM_DEBUG_EXCEPTION("emstorage_mail_search_end failed [%d]", err);
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

	if (ret == true)
		mail_send_notify(EMAIL_SEND_FINISH, 0, 0, account_id, mail_ids[total], err);
	else {
		if(mail_ids) /* prevent 34385 */
			mail_send_notify(status, 0, 0, account_id, mail_ids[total], err);
		emcore_show_user_message(account_id, EMAIL_ACTION_SEND_MAIL, err);
	}

	EM_SAFE_FREE(mail_ids);

	if (err_code != NULL)
		*err_code = err;

	return ret;
}

static int emcore_send_mail_smtp(SENDSTREAM *stream, ENVELOPE *env, char *data_file, int account_id, int mail_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], env[%p], data_file[%s], account_id[%d], mail_id[%d], err_code[%p]", stream, env, data_file, account_id, mail_id, err_code);
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

	if (!(ref_account = emcore_get_account_reference(account_id))) {
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
			strncat (buf, stream->protocol.esmtp.dsn.full ? " RET=FULL" : " RET=HDRS", sizeof(buf)-EM_SAFE_STRLEN(buf)-1);
			if (stream->protocol.esmtp.dsn.envid)
				SNPRINTF (buf + EM_SAFE_STRLEN (buf), sizeof(buf)-(EM_SAFE_STRLEN(buf)), " ENVID=%.100s", stream->protocol.esmtp.dsn.envid);
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
		EM_DEBUG_LOG("[SMTP] RCPT TO : <%s@%s> ... --------> %s", env->to->mailbox, env->to->host, env->to->error ? env->to->error  :  stream->reply);
		if (send_ret) {
			err = stream->replycode;
			goto FINISH_OFF;
		}

		if (!send_err)
			recipients++;
	}

	if (env->cc) {
		send_ret = smtp_rcpt(stream, env->cc, &send_err);
		EM_DEBUG_LOG("[SMTP] RCPT TO : <%s@%s> ... --------> %s", env->cc->mailbox, env->cc->host, env->cc->error ? env->cc->error  :  stream->reply);
		if (send_ret) {
			err = stream->replycode;
			goto FINISH_OFF;
		}

		if (!send_err)
			recipients++;
	}

	if (env->bcc) {
		send_ret = smtp_rcpt(stream, env->bcc, &send_err);
		EM_DEBUG_LOG("[SMTP] RCPT TO : <%s@%s> ... --------> %s", env->bcc->mailbox, env->bcc->host, env->bcc->error ? env->bcc->error  :  stream->reply);
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
	EM_DEBUG_LOG("[SMTP] DATA --------> %s", stream->reply);
	EM_PROFILE_END(profile_prepare_and_head);

	if (send_ret != SMTP_RESPONSE_READY) {
		err = send_ret;
		goto FINISH_OFF;
	}

	if (data_file) {
		EM_PROFILE_BEGIN(profile_open_file);
		if (!(fp = fopen(data_file, "r+"))) {
			EM_DEBUG_EXCEPTION("fopen(\"%s\") failed...", data_file);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}
		EM_PROFILE_END(profile_open_file);


#ifdef __FEATURE_SEND_OPTMIZATION__
	{
		char *data = NULL;
		int read_size, allocSize, dataSize, gMaxAllocSize = 40960; /*  40KB */

		fseek(fp, 0, SEEK_END);
		total = ftell(fp);
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
			if (total  < allocSize)
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

			EM_DEBUG_LOG("before smtp_soutr_test");
			if (!(send_ret = smtp_soutr_test(stream->netstream, data))) {
				EM_SAFE_FREE(data);
				EM_DEBUG_EXCEPTION("Failed to send the data ");
				err = EMAIL_ERROR_SMTP_SEND_FAILURE;
				goto FINISH_OFF;
			}
			else {
				sent_percent = (int) ((double)sent / (double)total * 100.0);
				if (last_sent_percent + 5 <= sent_percent) {
					if (!emcore_notify_network_event(NOTI_SEND_START, account_id, NULL, mail_id, sent_percent))
						EM_DEBUG_EXCEPTION("emcore_notify_network_event [NOTI_SEND_START] Failed >>>>");
					last_sent_percent = sent_percent;
				}
				EM_DEBUG_LOG("Sent data Successfully. sent[%d] total[%d]", sent, total);
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

#endif
		if (!send_ret) {
			EM_DEBUG_EXCEPTION("smtp_soutr failed - %ld", send_ret);
			err = EMAIL_ERROR_SMTP_SEND_FAILURE;
			goto FINISH_OFF;
		}
	}

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
static int attach_part(BODY *body, const unsigned char *data, int data_len, char *filename, char *content_sub_type, int is_inline, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("body[%p], data[%s], data_len[%d], filename[%s], content_sub_type[%s], err_code[%p]", body, data, data_len, filename, content_sub_type, err_code);

	int        ret = false;
	int        error = EMAIL_ERROR_NONE;
	int        has_special_character = 0;
	int        base64_file_name_length = 0;
	int        i= 0;
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
			EM_DEBUG_LOG("return_charset->name [%s]", result_charset->name);
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

		EM_DEBUG_LOG("encoded_file_name [%s]", encoded_file_name);

		if(encoded_file_name == NULL)
			encoded_file_name = strdup(filename);

		if(!em_encode_base64(encoded_file_name, EM_SAFE_STRLEN(encoded_file_name), &base64_file_name, (unsigned long*)&base64_file_name_length, &error)) {
			EM_DEBUG_EXCEPTION("em_encode_base64 failed. error [%d]", error);
			goto FINISH_OFF;
		}

		result_file_name = em_replace_string(base64_file_name, "\015\012", "");

		EM_DEBUG_LOG("base64_file_name_length [%d]", base64_file_name_length);

		if(result_file_name) {
			EM_SAFE_FREE(encoded_file_name);
			encoded_file_name = em_malloc(EM_SAFE_STRLEN(result_file_name) + 15);
			if(!encoded_file_name) {
				EM_DEBUG_EXCEPTION("em_malloc failed.");
				goto FINISH_OFF;
			}
			snprintf(encoded_file_name, EM_SAFE_STRLEN(result_file_name) + 15, "=?UTF-8?B?%s?=", result_file_name);
			EM_DEBUG_LOG("encoded_file_name [%s]", encoded_file_name);
		}

		extension = em_get_extension_from_file_path(filename, NULL);

		part->body.type = em_get_content_type_from_extension_string(extension, NULL);
		if(part->body.type == TYPEIMAGE) {
			part->body.subtype = strdup(extension);
		} else if (part->body.type == TYPEPKCS7_SIGN) {
			part->body.subtype = strdup("pkcs7-signature");
			part->body.type = TYPEAPPLICATION;
		} else if (part->body.type == TYPEPKCS7_MIME) {
			part->body.subtype = strdup("pkcs7-mime");
			part->body.type = TYPEAPPLICATION;
		} else
			part->body.subtype = strdup("octet-stream");

		part->body.encoding = ENCBINARY;
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
			part->body.id = emcore_generate_content_id_string("com.samsung.slp.email", &error);
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

static PART *attach_mutipart_with_sub_type(BODY *parent_body, char *sub_type, int *err_code)
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
static int attach_attachment_to_body(BODY **multipart_body, BODY *text_body, emstorage_attachment_tbl_t *input_attachment_tbl, int input_attachment_tbl_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("multipart_body[%p], text_body[%p], input_attachment_tbl[%p], input_attachment_tbl_count [%d], err_code[%p]", multipart_body, text_body, input_attachment_tbl, input_attachment_tbl_count, err_code);

	int ret = false;
	int i = 0;
	int error = EMAIL_ERROR_NONE;
	BODY *frame_body = NULL;
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

	for(i = 0; i < input_attachment_tbl_count; i++) {
		temp_attachment_tbl = input_attachment_tbl + i;

		EM_DEBUG_LOG("insert files - attachment id[%d]", temp_attachment_tbl->attachment_id);

		if (stat(temp_attachment_tbl->attachment_path, &st_buf) == 0)  {
			if (!temp_attachment_tbl->attachment_name)  {
				if (!emcore_get_file_name(temp_attachment_tbl->attachment_path, &name, &error))  {
					EM_DEBUG_EXCEPTION("emcore_get_file_name failed [%d]", error);
					goto FINISH_OFF;
				}
			}
			else
				name = temp_attachment_tbl->attachment_name;

			if (!attach_part(frame_body, (unsigned char *)temp_attachment_tbl->attachment_path, 0, name, NULL, false, &error))  {
				EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
				goto FINISH_OFF;
			}
		}
	}

	ret = true;

FINISH_OFF:
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

	if (len > 0)
		return g_strdup_printf("=?UTF-8?B?%s?=", g_base64_encode((const guchar  *)utf8_text, len));
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

static int emcore_make_envelope_from_mail(emstorage_mail_tbl_t *input_mail_tbl_data, ENVELOPE **output_envelope)
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
		ref_account = emcore_get_account_reference(input_mail_tbl_data->account_id);
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
				em_skip_whitespace(input_mail_tbl_data->full_address_from , &temp_address_string);
				EM_DEBUG_LOG("address[temp_address_string][%s]", temp_address_string);
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
		if (!input_mail_tbl_data->full_address_from || !input_mail_tbl_data->full_address_to)  {
			EM_DEBUG_EXCEPTION("input_mail_tbl_data->full_address_from[%p], input_mail_tbl_data->full_address_to[%p]", input_mail_tbl_data->full_address_from, input_mail_tbl_data->full_address_to);
			error = EMAIL_ERROR_INVALID_MAIL;
			goto FINISH_OFF;
		}

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
		em_skip_whitespace(input_mail_tbl_data->full_address_from , &pAdd);
		EM_DEBUG_LOG("address[pAdd][%s]", pAdd);

		rfc822_parse_adrlist(&envelope->from, pAdd, NULL);
		EM_SAFE_FREE(pAdd);
		pAdd = NULL;

		em_skip_whitespace(input_mail_tbl_data->full_address_return , &pAdd);
		EM_DEBUG_LOG("address[pAdd][%s]", pAdd);

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

	em_skip_whitespace(input_mail_tbl_data->full_address_to , &pAdd);
	EM_DEBUG_LOG("address[pAdd][%s]", pAdd);

	rfc822_parse_adrlist(&envelope->to, pAdd, NULL);
	EM_SAFE_FREE(pAdd);
	pAdd = NULL ;

	EM_DEBUG_LOG("address[input_mail_tbl_data->full_address_cc][%s]", input_mail_tbl_data->full_address_cc);
	em_skip_whitespace(input_mail_tbl_data->full_address_cc , &pAdd);
	EM_DEBUG_LOG("address[pAdd][%s]", pAdd);

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

	if (input_mail_tbl_data->subject)
		envelope->subject = emcore_encode_rfc2047_text(input_mail_tbl_data->subject, &error);

	char rfc822_date_string[DATE_STR_LENGTH] = { 0, };
	rfc822_date(rfc822_date_string);

	if (!is_incomplete)  {
		char  localtime_string[DATE_STR_LENGTH] = { 0, };
		strftime(localtime_string, 128, "%a, %e %b %Y %H : %M : %S ", localtime(&input_mail_tbl_data->date_time));
		/* append last 5byes("+0900") */
		g_strlcat(localtime_string, rfc822_date_string + (EM_SAFE_STRLEN(rfc822_date_string) -  5), DATE_STR_LENGTH);
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
/*  Description : Make RFC822 text file from mail_tbl data */
/*  Parameters :  */
/* 			input_mail_tbl_data :   */
/* 			is_draft  :  this mail is draft mail. */
/* 			file_path :  path of file that rfc822 data will be written to. */
INTERNAL_FUNC int emcore_make_rfc822_file_from_mail(emstorage_mail_tbl_t *input_mail_tbl_data, emstorage_attachment_tbl_t *input_attachment_tbl, int input_attachment_count, ENVELOPE **env, char **file_path, email_option_t *sending_option, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_tbl_data[%p], env[%p], file_path[%p], sending_option[%p], err_code[%p]", input_mail_tbl_data, env, file_path, sending_option, err_code);

	int       ret = false;
	int       error = EMAIL_ERROR_NONE;
	int       i = 0;
	ENVELOPE *envelope      = NULL;
	BODY     *text_body     = NULL;
	BODY     *html_body     = NULL;
	BODY     *root_body     = NULL;
	PART     *part_for_html = NULL;
	PARAMETER *param = NULL;
	PART     *part_for_text = NULL;
	char     *fname = NULL;

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

	if ( (error = emcore_make_envelope_from_mail(input_mail_tbl_data, &envelope)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_make_envelope_from_mail failed [%d]", error);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("input_mail_tbl_data->file_path_plain[%s]", input_mail_tbl_data->file_path_plain);
	EM_DEBUG_LOG("input_mail_tbl_data->file_path_html[%s]", input_mail_tbl_data->file_path_html);
	EM_DEBUG_LOG("input_mail_tbl_data->file_path_mime_entity[%s]", input_mail_tbl_data->file_path_mime_entity);
	EM_DEBUG_LOG("input_mail_tbl_data->body->attachment_num[%d]", input_mail_tbl_data->attachment_count);

	if ((input_mail_tbl_data->attachment_count > 0) || (input_mail_tbl_data->file_path_plain && input_mail_tbl_data->file_path_html))  {
		EM_DEBUG_LOG("attachment_num [%d]", input_mail_tbl_data->attachment_count);

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

		if (input_mail_tbl_data->smime_type == EMAIL_SMIME_NONE) {

			root_body->type               = TYPEMULTIPART;
			root_body->subtype            = strdup("MIXED");

			mail_free_body_parameter(&param);
			param = NULL;

		} else if (input_mail_tbl_data->smime_type == EMAIL_SMIME_SIGNED) {
			PARAMETER *protocol_param     = mail_newbody_parameter();

			root_body->type               = TYPEMULTIPART;
			root_body->subtype            = strdup("SIGNED");

			param->attribute       = cpystr("micalg");
			switch (input_mail_tbl_data->digest_type) {
			case DIGEST_TYPE_SHA1:
				param->value   = cpystr("sha1");
				break;
			case DIGEST_TYPE_MD5:
				param->value   = cpystr("md5");
				break;
			default:
				EM_DEBUG_EXCEPTION("Invalid digest type");
				break;
			}

			protocol_param->attribute   = cpystr("protocol");
			protocol_param->value       = cpystr("application/pkcs7-signature");
			protocol_param->next        = NULL;
			param->next	            = protocol_param;

			input_mail_tbl_data->file_path_plain = NULL;
			input_mail_tbl_data->file_path_html = NULL;

			input_attachment_tbl = input_attachment_tbl + (input_attachment_count - 1);

			input_attachment_count = 1;

		} else {

			root_body->type    = TYPEAPPLICATION;
			root_body->subtype = strdup("PKCS7-MIME");

			param->attribute = cpystr("name");
			param->value = cpystr("smime.p7m");
			param->next = NULL;

			input_mail_tbl_data->file_path_plain = NULL;
			input_mail_tbl_data->file_path_html = NULL;
			input_mail_tbl_data->file_path_mime_entity = NULL;

			input_attachment_count = 1;
		}

		root_body->contents.text.data = NULL;
		root_body->contents.text.size = 0;
		root_body->size.bytes         = 0;
		root_body->parameter          = param;

		if (input_mail_tbl_data->smime_type == EMAIL_SMIME_NONE && input_mail_tbl_data->file_path_plain && input_mail_tbl_data->file_path_html) {
			part_for_text = attach_mutipart_with_sub_type(root_body, "ALTERNATIVE", &error);

			if (!part_for_text) {
				EM_DEBUG_EXCEPTION("attach_mutipart_with_sub_type [part_for_text] failed [%d]", error);
				goto FINISH_OFF;
			}

			text_body = &part_for_text->body;

			if (input_mail_tbl_data->file_path_plain && EM_SAFE_STRLEN(input_mail_tbl_data->file_path_plain) > 0) {
				EM_DEBUG_LOG("file_path_plain[%s]", input_mail_tbl_data->file_path_plain);
				if (!attach_part(text_body, (unsigned char *)input_mail_tbl_data->file_path_plain, 0, NULL, NULL, false, &error)) {
					EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
					goto FINISH_OFF;
				}
			}

			if (input_mail_tbl_data->file_path_html && EM_SAFE_STRLEN(input_mail_tbl_data->file_path_html) > 0) {
				EM_DEBUG_LOG("file_path_html[%s]", input_mail_tbl_data->file_path_html);

				part_for_html = attach_mutipart_with_sub_type(text_body, "RELATED", &error);
				if (!part_for_html) {
					EM_DEBUG_EXCEPTION("attach_mutipart_with_sub_type [part_for_html] failed [%d]", error);
					goto FINISH_OFF;
				}

				if (!attach_part(&(part_for_html->body) , (unsigned char *)input_mail_tbl_data->file_path_html, 0, NULL, "html", false, &error)) {
					EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
					goto FINISH_OFF;
				}
			}
		}

		if (input_mail_tbl_data->file_path_mime_entity && EM_SAFE_STRLEN(input_mail_tbl_data->file_path_mime_entity) > 0) {
			EM_DEBUG_LOG("file_path_mime_entity[%s]", input_mail_tbl_data->file_path_mime_entity);
			root_body->sparep = EM_SAFE_STRDUP(input_mail_tbl_data->file_path_mime_entity);
		}

		if (input_attachment_tbl && input_attachment_count)  {
			emstorage_attachment_tbl_t *temp_attachment_tbl = NULL;
			char *name = NULL;
			BODY *body_to_attach = NULL;
			struct stat st_buf;

			for(i = 0; i < input_attachment_count; i++) {
				temp_attachment_tbl = input_attachment_tbl + i;
				EM_DEBUG_LOG("attachment_name[%s], attachment_path[%s]", temp_attachment_tbl->attachment_name, temp_attachment_tbl->attachment_path);
				if (stat(temp_attachment_tbl->attachment_path, &st_buf) == 0)  {
					if (!temp_attachment_tbl->attachment_name)  {
						if (!emcore_get_file_name(temp_attachment_tbl->attachment_path, &name, &error))  {
							EM_DEBUG_EXCEPTION("emcore_get_file_name failed [%d]", error);
							continue;
						}
					}
					else
						name = temp_attachment_tbl->attachment_name;

					EM_DEBUG_LOG("name[%s]", name);

					if (temp_attachment_tbl->attachment_inline_content_status && part_for_html)
						body_to_attach = &(part_for_html->body);
					else
						body_to_attach = root_body;

					if (!attach_part(body_to_attach, (unsigned char *)temp_attachment_tbl->attachment_path, 0, name, NULL, temp_attachment_tbl->attachment_inline_content_status, &error))  {
						EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
						continue;
					}
				}
			}
		}
		text_body = NULL;
	}
	else {
		text_body = mail_newbody();

		if (text_body == NULL)  {
			EM_DEBUG_EXCEPTION("mail_newbody failed...");

			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		text_body->type = TYPETEXT;
		text_body->encoding = ENC8BIT;
		if (input_mail_tbl_data->file_path_plain || input_mail_tbl_data->file_path_html)
			text_body->sparep = EM_SAFE_STRDUP(input_mail_tbl_data->file_path_plain ? input_mail_tbl_data->file_path_plain  :  input_mail_tbl_data->file_path_html);
		else
			text_body->sparep = NULL;

		if (input_mail_tbl_data->file_path_html != NULL && input_mail_tbl_data->file_path_html[0] != '\0')
			text_body->subtype = strdup("html");
		if (text_body->sparep)
			text_body->size.bytes = EM_SAFE_STRLEN(text_body->sparep);
		else
			text_body->size.bytes = 0;
	}


	if (input_mail_tbl_data->report_status & EMAIL_MAIL_REPORT_MDN) {
		/*  Report mail */
		EM_DEBUG_LOG("REPORT MAIL");
		envelope->references = cpystr(input_mail_tbl_data->message_id);
	}

	if (file_path)  {
		EM_DEBUG_LOG("write rfc822 : file_path[%p]", file_path);

		if (part_for_html)
			html_body = &(part_for_html->body);

		if (!emcore_write_rfc822(envelope, root_body ? root_body : text_body, html_body, input_mail_tbl_data->priority, input_mail_tbl_data->report_status, &fname, &error))  {
			EM_DEBUG_EXCEPTION("emcore_write_rfc822 failed [%d]", error);
			goto FINISH_OFF;
		}

		*file_path = fname;
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

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emcore_make_rfc822_file(email_mail_data_t *input_mail_tbl_data, email_attachment_data_t *input_attachment_tbl, int input_attachment_count, char **file_path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_tbl_data[%p], file_path[%p], err_code[%p]", input_mail_tbl_data, file_path, err_code);

	int       ret                  = false;
	int       error                = EMAIL_ERROR_NONE;
	int       is_incomplete        = 0;
	int       i                    = 0;
	ENVELOPE *envelope             = NULL;
	BODY     *text_body            = NULL;
	BODY     *html_body            = NULL;
	BODY     *root_body            = NULL;
	PART     *part_for_html        = NULL;
	PART     *part_for_text        = NULL;
	char      temp_file_path_plain[512];
	char      temp_file_path_html[512];
	char     *pAdd                 = NULL;
	char     *fname                = NULL;
	emstorage_account_tbl_t *ref_account     = NULL;

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

	if (!emstorage_get_account_by_id(input_mail_tbl_data->account_id, GET_FULL_DATA_WITHOUT_PASSWORD, &ref_account, true, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed : [%d]", error);
		goto FINISH_OFF;
	}

	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", input_mail_tbl_data->account_id);
		error = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!(envelope = mail_newenvelope()))  {
		EM_DEBUG_EXCEPTION("mail_newenvelope failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	is_incomplete = input_mail_tbl_data->flags_draft_field || (input_mail_tbl_data->save_status == EMAIL_MAIL_STATUS_SENDING);

	if (is_incomplete)  {
		if (ref_account->user_email_address && ref_account->user_email_address[0] != '\0')  {
			char *p = cpystr(ref_account->user_email_address);

			if (p == NULL)  {
				EM_DEBUG_EXCEPTION("cpystr failed...");
				error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			EM_DEBUG_LOG("Assign envelop->from");

			if (input_mail_tbl_data->full_address_from) {
				char *temp_address_string = NULL ;
				em_skip_whitespace(input_mail_tbl_data->full_address_from , &temp_address_string);
				EM_DEBUG_LOG("address[temp_address_string][%s]", temp_address_string);
				rfc822_parse_adrlist(&envelope->from, temp_address_string, ref_account->outgoing_server_address);
				EM_SAFE_FREE(temp_address_string);
				temp_address_string = NULL ;
			}
			else
				envelope->from = rfc822_parse_mailbox(&p, NULL);

			EM_SAFE_FREE(p);
			if (!envelope->from)  {
				EM_DEBUG_EXCEPTION("rfc822_parse_mailbox failed...");
				error = EMAIL_ERROR_INVALID_ADDRESS;
				goto FINISH_OFF;
			}
			else  {

				if (envelope->from->personal == NULL) {
					envelope->from->personal =
						(ref_account->user_display_name && ref_account->user_display_name[0] != '\0')?
						cpystr(ref_account->user_display_name)  :  NULL;
				}
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
	else  {
		if (!input_mail_tbl_data->full_address_from || !input_mail_tbl_data->full_address_to)  {
			EM_DEBUG_EXCEPTION("input_mail_tbl_data->full_address_from[%p], input_mail_tbl_data->full_address_to[%p]", input_mail_tbl_data->full_address_from, input_mail_tbl_data->full_address_to);
			error = EMAIL_ERROR_INVALID_MAIL;
			goto FINISH_OFF;
		}

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
			em_skip_whitespace(input_mail_tbl_data->full_address_from , &pAdd);
		EM_DEBUG_LOG("address[pAdd][%s]", pAdd);

		rfc822_parse_adrlist(&envelope->from, pAdd, ref_account->outgoing_server_address);
		EM_SAFE_FREE(pAdd);
			pAdd = NULL ;

		em_skip_whitespace(input_mail_tbl_data->full_address_return , &pAdd);
		EM_DEBUG_LOG("address[pAdd][%s]", pAdd);

		rfc822_parse_adrlist(&envelope->return_path, pAdd, ref_account->outgoing_server_address);
		EM_SAFE_FREE(pAdd);
		pAdd = NULL ;
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

	em_skip_whitespace(input_mail_tbl_data->full_address_to , &pAdd);
	EM_DEBUG_LOG("address[pAdd][%s]", pAdd);

	rfc822_parse_adrlist(&envelope->to, pAdd, ref_account->outgoing_server_address);
	EM_SAFE_FREE(pAdd);
	pAdd = NULL ;

	EM_DEBUG_LOG("address[input_mail_tbl_data->full_address_cc][%s]", input_mail_tbl_data->full_address_cc);
	em_skip_whitespace(input_mail_tbl_data->full_address_cc , &pAdd);
	EM_DEBUG_LOG("address[pAdd][%s]", pAdd);

	rfc822_parse_adrlist(&envelope->cc, pAdd, ref_account->outgoing_server_address);
	EM_SAFE_FREE(pAdd);
		pAdd = NULL ;

	em_skip_whitespace(input_mail_tbl_data->full_address_bcc , &pAdd);
	rfc822_parse_adrlist(&envelope->bcc, pAdd, ref_account->outgoing_server_address);
	EM_SAFE_FREE(pAdd);
		pAdd = NULL ;

	emcore_encode_rfc2047_address(envelope->return_path, &error);
	emcore_encode_rfc2047_address(envelope->from, &error);
	emcore_encode_rfc2047_address(envelope->sender, &error);
	emcore_encode_rfc2047_address(envelope->reply_to, &error);
	emcore_encode_rfc2047_address(envelope->to, &error);
	emcore_encode_rfc2047_address(envelope->cc, &error);
	emcore_encode_rfc2047_address(envelope->bcc, &error);

	if (input_mail_tbl_data->subject)
		envelope->subject = emcore_encode_rfc2047_text(input_mail_tbl_data->subject, &error);

	char rfc822_date_string[DATE_STR_LENGTH] = { 0, };
	char localtime_string[DATE_STR_LENGTH] = {0, };

	rfc822_date(rfc822_date_string);

	if (!is_incomplete)  {
		strftime(localtime_string, 128, "%a, %e %b %Y %H : %M : %S ", localtime(&input_mail_tbl_data->date_time));
		/*  append last 5byes("+0900") */
		g_strlcat(localtime_string, rfc822_date_string + (EM_SAFE_STRLEN(rfc822_date_string) -  5), DATE_STR_LENGTH);
		envelope->date = (unsigned char *)cpystr((const char *)localtime_string);
	}
	else {
		envelope->date = (unsigned char *)cpystr((const char *)rfc822_date_string);
	}
	/*  check report input_mail_tbl_data */

	/* Non-report input_mail_tbl_data */
	EM_DEBUG_LOG("input_mail_tbl_data->file_path_plain[%s]", input_mail_tbl_data->file_path_plain);
	EM_DEBUG_LOG("input_mail_tbl_data->file_path_html[%s]", input_mail_tbl_data->file_path_html);
	EM_DEBUG_LOG("input_mail_tbl_data->body->attachment_num[%d]", input_mail_tbl_data->attachment_count);

	if (input_mail_tbl_data->file_path_plain) {
		memset(temp_file_path_plain, 0x00, sizeof(temp_file_path_plain));
		SNPRINTF(temp_file_path_plain, sizeof(temp_file_path_plain), "%s%s%s", MAILTEMP, DIR_SEPERATOR, "UTF-8");

		if (!emstorage_copy_file(input_mail_tbl_data->file_path_plain, temp_file_path_plain, 0, &error)) {
			EM_DEBUG_EXCEPTION("emstorage_copy_file failed : [%d]", error);
			goto FINISH_OFF;
		}
	}

	if (input_mail_tbl_data->file_path_html) {
		memset(temp_file_path_html, 0x00, sizeof(temp_file_path_html));
		SNPRINTF(temp_file_path_html, sizeof(temp_file_path_html), "%s%s%s", MAILTEMP, DIR_SEPERATOR, "UTF-8.htm");

		if (!emstorage_copy_file(input_mail_tbl_data->file_path_html, temp_file_path_html, 0, &error)) {
			EM_DEBUG_EXCEPTION("emstorage_copy_file failed : [%d]", error);
			goto FINISH_OFF;
		}
	}

	if ((input_mail_tbl_data->attachment_count > 0) || (input_mail_tbl_data->file_path_plain && input_mail_tbl_data->file_path_html))  {
		EM_DEBUG_LOG("attachment_num  :  %d", input_mail_tbl_data->attachment_count);
		root_body = mail_newbody();

		if (root_body == NULL)  {
			EM_DEBUG_EXCEPTION("mail_newbody failed...");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		root_body->type               = TYPEMULTIPART;
		root_body->subtype            = strdup("MIXED");
		root_body->contents.text.data = NULL;
		root_body->contents.text.size = 0;
		root_body->size.bytes         = 0;

		part_for_text = attach_mutipart_with_sub_type(root_body, "ALTERNATIVE", &error);

		if (!part_for_text) {
			EM_DEBUG_EXCEPTION("attach_mutipart_with_sub_type [part_for_text] failed [%d]", error);
			goto FINISH_OFF;
		}

		text_body = &part_for_text->body;

		if (input_mail_tbl_data->file_path_plain && EM_SAFE_STRLEN(input_mail_tbl_data->file_path_plain) > 0)  {
			EM_DEBUG_LOG("file_path_plain[%s]", input_mail_tbl_data->file_path_plain);
			if (!attach_part(text_body, (unsigned char *)temp_file_path_plain, 0, NULL, NULL, false, &error))  {
				EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
				goto FINISH_OFF;
			}
		}

		if (input_mail_tbl_data->file_path_html && EM_SAFE_STRLEN(input_mail_tbl_data->file_path_html) > 0)  {
			EM_DEBUG_LOG("file_path_html[%s]", input_mail_tbl_data->file_path_html);

			part_for_html = attach_mutipart_with_sub_type(text_body, "RELATED", &error);
			if (!part_for_html) {
				EM_DEBUG_EXCEPTION("attach_mutipart_with_sub_type [part_for_html] failed [%d]", error);
				goto FINISH_OFF;
			}

			if (!attach_part(&(part_for_html->body) , (unsigned char *)temp_file_path_html, 0, NULL, "html", false, &error))  {
				EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
				goto FINISH_OFF;
			}
		}

		if (input_attachment_tbl && input_attachment_count)  {
			email_attachment_data_t *temp_attachment_tbl = NULL;
			char *name = NULL;
			BODY *body_to_attach = NULL;
			struct stat st_buf;

			for(i = 0; i < input_attachment_count; i++) {
				temp_attachment_tbl = input_attachment_tbl + i;
				EM_DEBUG_LOG("attachment_name[%s], attachment_path[%s]", temp_attachment_tbl->attachment_name, temp_attachment_tbl->attachment_path);
				if (stat(temp_attachment_tbl->attachment_path, &st_buf) == 0)  {
					if (!temp_attachment_tbl->attachment_name)  {
						if (!emcore_get_file_name(temp_attachment_tbl->attachment_path, &name, &error))  {
							EM_DEBUG_EXCEPTION("emcore_get_file_name failed [%d]", error);
							continue;
						}
					}
					else
						name = temp_attachment_tbl->attachment_name;
					EM_DEBUG_LOG("name[%s]", name);

					if (temp_attachment_tbl->inline_content_status && part_for_html)
						body_to_attach = &(part_for_html->body);
					else
						body_to_attach = root_body;

					if (!attach_part(body_to_attach, (unsigned char *)temp_attachment_tbl->attachment_path, 0, name, NULL, temp_attachment_tbl->inline_content_status, &error))  {
						EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
						continue;
					}
				}
			}
		}
		text_body = NULL;
	} else  {
		text_body = mail_newbody();

		if (text_body == NULL)  {
			EM_DEBUG_EXCEPTION("mail_newbody failed...");

			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		text_body->type = TYPETEXT;
		text_body->encoding = ENC8BIT;
		if (input_mail_tbl_data->file_path_plain || input_mail_tbl_data->file_path_html)
			text_body->sparep = EM_SAFE_STRDUP(input_mail_tbl_data->file_path_plain ? temp_file_path_plain  :  temp_file_path_html);
		else
			text_body->sparep = NULL;

		if (input_mail_tbl_data->file_path_html != NULL && input_mail_tbl_data->file_path_html[0] != '\0')
			text_body->subtype = strdup("html");
		if (text_body->sparep)
			text_body->size.bytes = EM_SAFE_STRLEN(text_body->sparep);
		else
			text_body->size.bytes = 0;
	}

	if (file_path)  {
		EM_DEBUG_LOG("write rfc822  :  file_path[%s]", file_path);

		if (part_for_html)
			html_body = &(part_for_html->body);

		if (!emcore_write_rfc822(envelope, root_body ? root_body  :  text_body, html_body, input_mail_tbl_data->priority, input_mail_tbl_data->report_status, &fname, &error))  {
			EM_DEBUG_EXCEPTION("emcore_write_rfc822 failed [%d]", error);
			goto FINISH_OFF;
		}

		*file_path = fname;
	}

	if (EM_SAFE_STRLEN(temp_file_path_plain) > 0)  {
		if (!emstorage_delete_file(temp_file_path_plain, &error))  {
			EM_DEBUG_EXCEPTION("emstorage_delete_file failed [%d]", error);
			goto FINISH_OFF;
		}
	}

	if (EM_SAFE_STRLEN(temp_file_path_html) > 0) {
		if (!emstorage_delete_file(temp_file_path_html, &error))  {
			EM_DEBUG_EXCEPTION("emstorage_delete_file failed [%d]", error);
			goto FINISH_OFF;
		}
	}

	ret = true;

FINISH_OFF:

	emstorage_free_account(&ref_account, 1, NULL);

	if (envelope != NULL)
		mail_free_envelope(&envelope);

	if (text_body != NULL)
		mail_free_body(&text_body);

	if (root_body != NULL)
		mail_free_body(&root_body);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

#ifdef __FEATURE_SUPPORT_REPORT_MAIL__
static int emcore_get_report_mail_body(ENVELOPE *envelope, BODY **multipart_body, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("envelope[%p], mulitpart_body[%p], err_code[%p]", envelope, multipart_body, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	BODY *m_body = NULL;
	BODY *p_body = NULL;
	BODY *text_body = NULL;
	PARAMETER *param = NULL;
	emstorage_attachment_tbl_t temp_attachment_tbl;
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

	if (!(fp = fopen(fname, "wb+")))  {
		EM_DEBUG_EXCEPTION(" fopen failed - %s", fname);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
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

	if (!(fp = fopen(fname, "wb+")))  {
		EM_DEBUG_EXCEPTION(" fopen failed - %s", fname);
		err = EMAIL_ERROR_SYSTEM_FAILURE;		/* EMAIL_ERROR_UNKNOWN; */
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

	memset(&temp_attachment_tbl, 0x00, sizeof(emstorage_attachment_tbl_t));

	temp_attachment_tbl.attachment_path = EM_SAFE_STRDUP(fname);

	if (!emcore_get_file_size(fname, &temp_attachment_tbl.attachment_size, &err))  {
		EM_DEBUG_EXCEPTION(" emcore_get_file_size failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!attach_attachment_to_body(&m_body, text_body, &temp_attachment_tbl, 1, &err))  {
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
	int read_size = 0;
	int ret = false;
	char *read_buff = NULL;


	if (file_path)
		r_fp = fopen(file_path, "r");

	if (!r_fp) {
		EM_DEBUG_EXCEPTION(" Filename %s failed to open", file_path);
		goto FINISH_OFF;
	}

	struct stat stbuf;
	if (stat(file_path, &stbuf) == 0 && stbuf.st_size > 0) {
		EM_DEBUG_LOG(" File Size [ %d ] ", stbuf.st_size);
		read_buff = calloc((stbuf.st_size+ 1), sizeof(char));
		read_size = fread(read_buff, 1, stbuf.st_size, r_fp);
		read_buff[stbuf.st_size] = '\0';
	}

	if (ferror(r_fp)) {
		EM_DEBUG_EXCEPTION("file read failed - %s", file_path);
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

static int emcore_copy_attachment_from_original_mail(int input_original_mail_id, int input_target_mail_id)
{
	EM_DEBUG_FUNC_BEGIN("input_original_mail_id[%d] input_target_mail_id[%d]", input_original_mail_id, input_target_mail_id);
	int err = EMAIL_ERROR_NONE;
	int i = 0, j = 0;
	int original_mail_attachment_count = 0;
	int target_mail_attachment_count = 0;
	int attachment_id = 0;
	char output_file_name[MAX_PATH] = { 0, };
	char output_file_path[MAX_PATH] = { 0, };
	emstorage_attachment_tbl_t *original_mail_attachment_array = NULL;
	emstorage_attachment_tbl_t *target_mail_attachment_array = NULL;
	emstorage_attachment_tbl_t *target_attach = NULL;

	if((err = emstorage_get_attachment_list(input_original_mail_id, false, &original_mail_attachment_array, &original_mail_attachment_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);
		goto FINISH_OFF;
	}

	if((err = emstorage_get_attachment_list(input_target_mail_id, false, &target_mail_attachment_array, &target_mail_attachment_count)) != EMAIL_ERROR_NONE) {
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

				if(!emcore_save_mail_file(target_attach->account_id, target_attach->mail_id, attachment_id, original_mail_attachment_array[i].attachment_path, original_mail_attachment_array[i].attachment_name, output_file_path, &err)) {
					EM_DEBUG_EXCEPTION("emcore_save_mail_file failed [%d]", err);
					goto FINISH_OFF;
				}

				EM_SAFE_FREE(target_attach->attachment_path);
				target_attach->attachment_path = EM_SAFE_STRDUP(output_file_path);
				target_attach->attachment_save_status = 1;

				if(!emstorage_update_attachment(target_attach, false, &err)) {
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
INTERNAL_FUNC int emcore_send_mail_with_downloading_attachment_of_original_mail(int input_mail_id)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d]", input_mail_id);
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int attachment_count = 0;
	email_mail_data_t *mail_to_be_sent = NULL;
	email_mail_data_t *original_mail = NULL;
	email_attachment_data_t *attachment_array = NULL;

	/* Get mail data */
	if((err = emcore_get_mail_data(input_mail_id, &mail_to_be_sent)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_data failed [%d]", err);
		goto FINISH_OFF;
	}

	if(mail_to_be_sent->reference_mail_id <= 0) {
		err = EMAIL_ERROR_INVALID_REFERENCE_MAIL;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_REFERENCE_MAIL");
		goto FINISH_OFF;
	}

	/* Get original mail data */
	if((err = emcore_get_mail_data(mail_to_be_sent->reference_mail_id, &original_mail)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_data failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Check necessity of download */
	if((err = emcore_get_attachment_data_list(original_mail->mail_id, &attachment_array, &attachment_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_attachment_data_list failed [%d]", err);
		goto FINISH_OFF;
	}

	/* If need be, download attachments */
	for(i = 0; i < attachment_count; i++) {
		if(attachment_array[i].save_status != 1) {
			if(!emcore_download_attachment(original_mail->account_id, original_mail->mail_id, i + 1, &err)) {
				EM_DEBUG_EXCEPTION("emcore_download_attachment failed [%d]", err);
				goto FINISH_OFF;
			}
		}
	}

	/* Copy attachment to the mail to be sent */
	if((err = emcore_copy_attachment_from_original_mail(original_mail->mail_id, mail_to_be_sent->mail_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_download_attachment failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Send the mail */
	if(!emcore_send_mail(mail_to_be_sent->mail_id, &err)) {
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


static int emcore_sending_alarm_cb(int input_timer_id, void *user_parameter)
{
	EM_DEBUG_FUNC_BEGIN("input_timer_id [%d] user_parameter [%p]", input_timer_id, user_parameter);
	int err = EMAIL_ERROR_NONE;
	int ret = 0;
	email_alarm_data_t *alarm_data = NULL;

	if (((err = emcore_get_alarm_data_by_alarm_id(input_timer_id, &alarm_data)) != EMAIL_ERROR_NONE) || alarm_data == NULL) {
		EM_DEBUG_EXCEPTION("emcore_get_alarm_data_by_alarm_id failed [%d]", err);
		goto FINISH_OFF;
	}

	/* send mail here */
	if(!emcore_send_mail(alarm_data->reference_id, &err)) {
		EM_DEBUG_EXCEPTION("emcore_send_mail failed [%d]", ret);
		goto FINISH_OFF;
	}

	/* delete alarm info*/
	emcore_delete_alram_data_from_alarm_data_list(alarm_data);

FINISH_OFF:

	EM_SAFE_FREE(alarm_data);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


INTERNAL_FUNC int emcore_schedule_sending_mail(int input_mail_id, time_t input_time_to_send)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d] input_time_to_send[%d]", input_mail_id, input_time_to_send);
	int err = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t *mail_data = NULL;

	/* get mail data */
	if (!emstorage_get_mail_by_id(input_mail_id, &mail_data, true, &err) || mail_data == NULL) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	/* set save_status as EMAIL_MAIL_STATUS_SEND_SCHEDULED */
	if (!emstorage_set_field_of_mails_with_integer_value(mail_data->account_id, &(mail_data->mail_id), 1, "save_status", EMAIL_MAIL_STATUS_SEND_SCHEDULED, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emstorage_set_field_of_mails_with_integer_value(mail_data->account_id, &(mail_data->mail_id), 1, "scheduled_sending_time", input_time_to_send, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err);
		goto FINISH_OFF;
	}

	/* add alarm */
	if ((err = emcore_add_alarm(input_time_to_send, EMAIL_ALARM_CLASS_SCHEDULED_SENDING, input_mail_id, emcore_sending_alarm_cb, NULL)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_add_alarm failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:
	if(mail_data)
		emstorage_free_mail(&mail_data, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
/* Scheduled sending ------------------------------------------------ */
