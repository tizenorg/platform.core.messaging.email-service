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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "em-core-types.h"
#include "c-client.h"
#include "em-core-global.h"
#include "em-core-utils.h"
#include "em-storage.h"
#include "em-core-api.h"
#include "em-core-smtp.h"
#include "em-core-event.h"
#include "em-core-mailbox.h"
#include "em-core-mesg.h"
#include "em-core-mime.h"
#include "em-core-account.h" 
#include "em-core-imap-mailbox.h"
#include "em-core-mailbox-sync.h"
#include "Msg_Convert.h"


#include <unistd.h>
#include "emf-dbglog.h"


/* Functions from uw-imap-toolkit */
/* extern void *fs_get(size_t size); */
extern void rfc822_date(char *date);
extern long smtp_soutr(void *stream, char *s);
extern long smtp_send(SENDSTREAM *stream, char *command, char *args);
extern long smtp_rcpt(SENDSTREAM *stream, ADDRESS *adr, long* error);

#ifdef __FEATURE_SEND_OPTMIZATION__
extern long smtp_soutr_test(void *stream, char *s);
#endif


static int em_core_mail_get_report_body(ENVELOPE *envelope, BODY **multipart_body, int *err_code);
static int em_core_mail_send_smtp(SENDSTREAM *stream, ENVELOPE *env, char *data_file, int account_id, int mail_id,  int *err_code);

void mail_send_notify(emf_send_status_t status, int total, int sent, int account_id, int mail_id,  int err_code)
{
	EM_DEBUG_FUNC_BEGIN("status[%d], total[%d], sent[%d], account_id[%d], mail_id[%d], err_code[%d]", status, total, sent, account_id, mail_id, err_code);
	
	switch (status) {
		case EMF_SEND_CONNECTION_FAIL: 
		case EMF_SEND_FINISH:
		case EMF_SEND_FAIL: 
			break;
		
		case EMF_SEND_PROGRESS:
		default:
			break;
	}
	em_core_execute_event_callback(EMF_ACTION_SEND_MAIL, total, sent, status, account_id, mail_id, -1, err_code);
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

static char *em_core_find_img_tag(char *source_string)
{
	EM_DEBUG_FUNC_BEGIN("source_string[%p]", source_string);
	
	int cur = 0, string_length;
	if (!source_string)
		return false;

	string_length = strlen(source_string);

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
static char *em_core_replace_inline_image_path_with_content_id(char *source_string, BODY *html_body, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("source_string[%p], html_body[%p], err_code[%p]", source_string, html_body, err_code);

	int  err = EMF_ERROR_NONE;
	char content_id_buffer[CONTENT_ID_BUFFER_SIZE], file_name_buffer[512], new_string[512], *result_string = NULL, *input_string = NULL;
	BODY *cur_body = NULL;
	PART *cur_part = NULL;

	if (!source_string || !html_body) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	input_string = EM_SAFE_STRDUP(source_string); 

	cur_part = html_body->nested.part;

	while (cur_part) {
		cur_body = &(cur_part->body);
		if (cur_body) {
			EM_DEBUG_LOG("Has body. id[%s]", cur_body->disposition.type);
			if (cur_body->disposition.type && cur_body->disposition.type[0] == 'i') {   /*  Is inline content? */
				EM_DEBUG_LOG("Has inline content");
				memset(content_id_buffer, 0, CONTENT_ID_BUFFER_SIZE);
				if (cur_body->id) {
					EM_SAFE_STRNCPY(content_id_buffer, cur_body->id + 1, CONTENT_ID_BUFFER_SIZE - 1); /*  Removing <, > */
					content_id_buffer[strlen(content_id_buffer) - 1] = NULL_CHAR;
					/* if (em_core_get_attribute_value_of_body_part(cur_body->parameter, "name", file_name_buffer, CONTENT_ID_BUFFER_SIZE, false, &err)) { */
					if (em_core_get_attribute_value_of_body_part(cur_body->parameter, "name", file_name_buffer, CONTENT_ID_BUFFER_SIZE, true, &err)) {
						EM_DEBUG_LOG("Content-ID[%s], filename[%s]", content_id_buffer, file_name_buffer);
						SNPRINTF(new_string, CONTENT_ID_BUFFER_SIZE, "cid:%s", content_id_buffer);
						result_string = em_core_replace_string(input_string, file_name_buffer, new_string);
						if (input_string)
							EM_SAFE_FREE(input_string);
					}
				}
			}
		}
		if (result_string)
			input_string = result_string;
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

static int em_core_write_rfc822_body(BODY *body, BODY *html_body, FILE *fp, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("body[%p], html_body[%p], fp[%p], err_code[%p]", body, html_body, fp, err_code);
	
	PARAMETER *param = NULL;
	PART *part = NULL;
	char *p = NULL, *bndry = NULL, buf[1025], *replaced_string = NULL;
	int fd, nread, nwrite, error = EMF_ERROR_NONE;
	
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
				fprintf(fp, "%s"CRLF_STRING, buf);
				
				em_core_write_rfc822_body(&part->body, html_body, fp, err_code);
			} while ((part = part->next));
			
			fprintf(fp, "--%s--"CRLF_STRING, bndry);
			break;
		
		default:  {
			EM_DEBUG_LOG("body->type is not TYPEMULTIPART");

			char *file_path = body->sparep;
			char buf[RFC822_STRING_BUFFER_SIZE + 1] = { 0, }, *img_tag_pos = NULL;
			unsigned long len;
			
			p = NULL;
			
			if (!file_path || strlen(file_path) == 0)  {
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

			if (fd < 0)  {
				EM_DEBUG_EXCEPTION("open(\"%s\") failed...", file_path);
				return false;
			}
			
			while (1)  {
				nread = read(fd, buf, (body->encoding == ENCBASE64 ? 57 : RFC822_READ_BLOCK_SIZE - 2));

				if (nread < 0)  {
					close(fd);
					return false;
				}
				
				if (nread == 0) {
					close(fd);
					break;
				}
			
				p = NULL;
				len = nread;

				/*  EM_DEBUG_LOG("body->type[%d], body->subtype[%c]", body->type, body->subtype[0]); */
				if (body->type == TYPETEXT && (body->subtype && (body->subtype[0] == 'H' || body->subtype[0] == 'h'))) {   
					EM_DEBUG_LOG("HTML Part");
					img_tag_pos = em_core_find_img_tag(buf);

					if (img_tag_pos) {
						replaced_string = em_core_replace_inline_image_path_with_content_id(buf, html_body, &error);
						if (replaced_string) {
							EM_DEBUG_LOG("em_core_replace_inline_image_path_with_content_id succeeded");
							strcpy(buf, replaced_string);
							nread = len = strlen(buf);
							EM_DEBUG_LOG("buf[%s], nread[%d], len[%d]", buf, nread, len);
						}
						else
							EM_DEBUG_LOG("em_core_replace_inline_image_path_with_content_id failed[%d]", error);
					}
				}

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
			
			break;
		}	/*  default: */
	}
	EM_DEBUG_FUNC_END();
	return true;
}

static int em_core_write_rfc822(ENVELOPE *env, BODY *body, BODY *html_body, emf_extra_flag_t flag, char **data, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("env[%p], body[%p], data[%p], err_code[%p]", env, body, data, err_code);
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	
	FILE *fp = NULL;
	char *fname = NULL;
	char *p = NULL;
	size_t p_len = 0;
	
	if (!env || !data)  {
		EM_DEBUG_EXCEPTION("Invalid Parameters");
		error = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	srand(time(NULL));
	
	rfc822_encode_body_7bit(env, body); /*  if contents.text.data isn't NULL, the data will be encoded. */
	
	/*  FIXME : create memory map for this file */
	p_len = (env->subject ? strlen(env->subject) : 0) + 8192;
	
	if (!(p = em_core_malloc(p_len)))  {		/* (env->subject ? strlen(env->subject) : 0) + 8192))) */
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EMF_ERROR_OUT_OF_MEMORY;
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

	if (strlen(p) > 2)		
		*(p + strlen(p) - 2) = '\0';
	

	if (flag.report)  {
		char buf[512] = {0x00, };
		switch (flag.report)  {
			case EMF_MAIL_REPORT_DSN: /*  DSN (delivery status) */
				/*  change content-type */
				/*  Content-Type: multipart/report; */
				/* 		report-type= delivery-status; */
				/* 		boundary="----=_NextPart_000_004F_01C76EFF.54275C50" */
				break;
			
			case EMF_MAIL_REPORT_MDN: /*  MDN (read receipt) */
				/*  Content-Type: multipart/report; */
				/* 		report-type= disposition-notification; */
				/* 		boundary="----=_NextPart_000_004F_01C76EFF.54275C50" */
				break;
			
			case EMF_MAIL_REPORT_REQUEST: /*  require read status */
				rfc822_address(buf, env->from);
				if (strlen(buf))
					SNPRINTF(p + strlen(p), p_len-(strlen(p)), "Disposition-Notification-To: %s"CRLF_STRING, buf);
				break;
				
			default:
				break;
		}
	}
	
	if (flag.priority)  {		/*  priority (1:high 3:normal 5:low) */
		SNPRINTF(p + strlen(p), p_len-(strlen(p)), "X-Priority: %d"CRLF_STRING, flag.priority);
		
		switch (flag.priority)  {
			case EMF_MAIL_PRIORITY_HIGH:
				SNPRINTF(p + strlen(p), p_len-(strlen(p)), "X-MSMail-Priority: HIgh"CRLF_STRING);
				break;
			case EMF_MAIL_PRIORITY_NORMAL:
				SNPRINTF(p + strlen(p), p_len-(strlen(p)), "X-MSMail-Priority: Normal"CRLF_STRING);
				break;
			case EMF_MAIL_PRIORITY_LOW:
				SNPRINTF(p + strlen(p), p_len-(strlen(p)), "X-MSMail-Priority: Low"CRLF_STRING);
				break;
		}
	}
	
	SNPRINTF(p + strlen(p), p_len-(strlen(p)), CRLF_STRING);
	
	if (!em_core_get_temp_file_name(&fname, &error))  {
		EM_DEBUG_EXCEPTION(" em_core_get_temp_file_name failed[%d]", error);
		goto FINISH_OFF;
	}
	
	if (!(fp = fopen(fname, "w+")))  {
		EM_DEBUG_EXCEPTION("fopen failed[%s]", fname);
		error = EMF_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
	
	fprintf(fp, "%s", p);
	
	if (body)  {
		if (!em_core_write_rfc822_body(body, html_body, fp, &error))  {
			EM_DEBUG_EXCEPTION("em_core_write_rfc822_body failed[%d]", error);
			goto FINISH_OFF;
		}
	}

	ret = true;

	
FINISH_OFF:
	if (fp != NULL)
		fclose(fp);

#ifdef USE_SYNC_LOG_FILE
	em_storage_copy_file(fname, "/tmp/phone2pc.eml", false, NULL);	
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

static int em_core_set_current_time_to_mail_header(emf_mail_head_t *head, int *err)
{
	EM_DEBUG_FUNC_BEGIN("head[%p], err [%p]", head, err);
	
	int err_code = EMF_ERROR_NONE, ret = false;
	time_t t = time(NULL);
	struct tm *p_tm = NULL;		
	
	if(!head) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err_code = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	p_tm = gmtime(&t);

	if (!p_tm)  {
		EM_DEBUG_EXCEPTION("localtime failed...");
		err_code = EMF_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
	
	head->datetime.year   = p_tm->tm_year + 1900;
	head->datetime.month  = p_tm->tm_mon + 1;
	head->datetime.day    = p_tm->tm_mday;
	head->datetime.hour   = p_tm->tm_hour;
	head->datetime.minute = p_tm->tm_min;
	head->datetime.second = p_tm->tm_sec;

	ret = true;

FINISH_OFF:

	if(err)
		*err = err_code;

	EM_DEBUG_FUNC_END();
	return ret;
}



EXPORT_API int em_core_mail_save(int account_id, char *mailbox_name, emf_mail_t *mail_src, emf_meeting_request_t *meeting_req, int from_composer, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%p], mail_src[%p], from_composer[%d], err_code[%p]", account_id, mailbox_name, mail_src, from_composer, err_code);
	
	int ret = false, err = EMF_ERROR_NONE;
	int attachment_id = 0, mail_id = 0, thread_id = -1, thread_item_count = 0, latest_mail_id_in_thread = -1;
	int rule_len, rule_matched = -1, local_attachment_count = 0, local_inline_content_count = 0;
	char *ext = NULL, *mailbox_name_spam = NULL, *mailbox_name_target = NULL;
	char name_buf[MAX_PATH] = {0x00, }, html_body[MAX_PATH] = {0x00, };
	char datetime[DATETIME_LENGTH] = { 0, };
	emf_mail_tbl_t *mail_table_data = NULL;
	emf_mailbox_tbl_t *mailbox_tbl_data = NULL;
	emf_attachment_info_t *atch = NULL;
	emf_mail_account_tbl_t *account_tbl_item = NULL;
	emf_mail_rule_tbl_t *rule = NULL;
	emf_mail_attachment_tbl_t attachment_tbl = { 0 };
	struct stat st_buf = { 0 };

	/* Validating parameters */
	
	if (!account_id || !mailbox_name || !mail_src || !mail_src->info || !mail_src->head)  {	
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!em_storage_get_account_by_id(account_id, EMF_ACC_GET_OPT_DEFAULT | EMF_ACC_GET_OPT_OPTIONS, &account_tbl_item, true, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_get_account_by_id failed. account_id[%d] err[%d]", account_id, err);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (from_composer) {
		if (!mail_src->head->from) 
			mail_src->head->from = EM_SAFE_STRDUP(account_tbl_item->email_addr);

		/* check for email_address validation */
		if (!em_core_verify_email_address_of_mail_header(mail_src->head, false, &err)) {
			EM_DEBUG_EXCEPTION("em_core_verify_email_address_of_mail_header failed [%d]", err);
			goto FINISH_OFF;
		}
		
		if (mail_src->info->extra_flags.report == EMF_MAIL_REPORT_MDN)  {	
			/* check read-report mail */
			if(!mail_src->head->to) { /* A report mail should have 'to' address */
				EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
				err = EMF_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}
			/* Create report mail body */
			if (!em_core_mail_get_rfc822(mail_src, NULL, NULL, NULL, &err))  {
				EM_DEBUG_EXCEPTION("em_core_mail_get_rfc822 failed [%d]", err);
				goto FINISH_OFF;
			}
		}

		mailbox_name_target = EM_SAFE_STRDUP(mailbox_name);
	}
	else {	/*  For Spam handling */
		emf_option_t *opt = &account_tbl_item->options;
		EM_DEBUG_LOG("block_address [%d], block_subject [%d]", opt->block_address, opt->block_subject);
		
		if (opt->block_address || opt->block_subject)  {
			int is_completed = false;
			int type = 0;
			
			if (!opt->block_address)
				type = EMF_FILTER_SUBJECT;
			else if (!opt->block_subject)
				type = EMF_FILTER_FROM;
			
			if (!em_storage_get_rule(ALL_ACCOUNT, type, 0, &rule_len, &is_completed, &rule, true, &err) || !rule) 
				EM_DEBUG_LOG("No proper rules. em_storage_get_rule returnes [%d]", err);
		}

		if (rule) {
			if (!em_storage_get_mailboxname_by_mailbox_type(account_id, EMF_MAILBOX_TYPE_SPAMBOX, &mailbox_name_spam, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				mailbox_name_spam = NULL;
			}

			if (mailbox_name_spam && !em_core_mail_check_rule(mail_src->head, rule, rule_len, &rule_matched, &err))  {
				EM_DEBUG_EXCEPTION("em_core_mail_check_rule failed [%d]", err);
				goto FINISH_OFF;
			}
		}

		if (rule_matched >= 0 && mailbox_name_spam) 
			mailbox_name_target = EM_SAFE_STRDUP(mailbox_name_spam);
		else
			mailbox_name_target = EM_SAFE_STRDUP(mailbox_name);
	}
	
	if (!em_storage_get_mailbox_by_name(account_id, -1, mailbox_name_target, (emf_mailbox_tbl_t **)&mailbox_tbl_data, false, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);		
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	if (!em_storage_increase_mail_id(&mail_id, true, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_increase_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

	mail_table_data = (emf_mail_tbl_t  *)em_core_malloc(sizeof(emf_mail_tbl_t));

	if(!mail_table_data) {
		EM_DEBUG_EXCEPTION("em_core_malloc failed");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	mail_table_data->account_id           = account_id;
	mail_table_data->mail_id              = mail_id;
	mail_table_data->mailbox_name         = EM_SAFE_STRDUP(mailbox_name_target);
	mail_table_data->mail_size            = mail_src->info->rfc822_size;
	mail_table_data->server_mail_status   = !from_composer;
	mail_table_data->server_mail_id       = EM_SAFE_STRDUP(mail_src->info->sid);
	mail_table_data->full_address_from    = EM_SAFE_STRDUP(mail_src->head->from);
	mail_table_data->full_address_to      = EM_SAFE_STRDUP(mail_src->head->to);
	mail_table_data->full_address_cc      = EM_SAFE_STRDUP(mail_src->head->cc);
	mail_table_data->full_address_bcc     = EM_SAFE_STRDUP(mail_src->head->bcc);
	mail_table_data->full_address_reply   = EM_SAFE_STRDUP(mail_src->head->reply_to);
	mail_table_data->full_address_return  = EM_SAFE_STRDUP(mail_src->head->return_path);
	mail_table_data->subject              = EM_SAFE_STRDUP(mail_src->head->subject);
	mail_table_data->body_download_status = mail_src->info->extra_flags.text_download_yn;	
	mail_table_data->mailbox_type         = mailbox_tbl_data->mailbox_type;

	em_core_fill_address_information_of_mail_tbl(mail_table_data);

	EM_DEBUG_LOG("mail_table_data->mail_size [%d]", mail_table_data->mail_size);
	if(mail_table_data->mail_size == 0)
		mail_table_data->mail_size = em_core_get_mail_size(mail_src, &err); /*  Getting file size before file moved.  */
 
	if (mail_src->body)  {	
		if (from_composer || mail_table_data->body_download_status) {
			if (!em_storage_create_dir(account_id, mail_id, 0, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
				goto FINISH_OFF;
			}

			if (mail_src->body->plain) {
				EM_DEBUG_LOG("mail_src->body->plain [%s]", mail_src->body->plain);
				if (!em_storage_get_save_name(account_id, mail_id, 0, mail_src->body->plain_charset ? mail_src->body->plain_charset  :  "UTF-8", name_buf, &err))  {
					EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
					goto FINISH_OFF;
				}
			
				if (!em_storage_move_file(mail_src->body->plain, name_buf, from_composer, &err)) {
					EM_DEBUG_EXCEPTION("em_storage_move_file failed [%d]", err);
					goto FINISH_OFF;
				}
				if (mail_table_data->body_download_status == 0)
					mail_table_data->body_download_status = 1;
				mail_table_data->file_path_plain = EM_SAFE_STRDUP(name_buf);		
				EM_SAFE_FREE(mail_src->body->plain);
				mail_src->body->plain = EM_SAFE_STRDUP(name_buf);
			}

			if (mail_src->body->html) {
				EM_DEBUG_LOG("mail_src->body->html [%s]", mail_src->body->html);
				if (mail_src->body->plain_charset != NULL && mail_src->body->plain_charset[0] != NULL_CHAR) {
					memcpy(html_body, mail_src->body->plain_charset, strlen(mail_src->body->plain_charset));
					strcat(html_body, ".htm");
				}
				else {
					memcpy(html_body, "UTF-8.htm", strlen("UTF-8.htm"));
				}
					
				if (!em_storage_get_save_name(account_id, mail_id, 0, html_body, name_buf, &err))  {
					EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
					goto FINISH_OFF;
				}

				if (!em_storage_move_file(mail_src->body->html, name_buf, from_composer, &err))  {
					EM_DEBUG_EXCEPTION("em_storage_move_file failed [%d]", err);
					goto FINISH_OFF;
				}

				if (mail_table_data->body_download_status == EMF_BODY_DOWNLOAD_STATUS_NONE)
					mail_table_data->body_download_status = EMF_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;

				mail_table_data->file_path_html = EM_SAFE_STRDUP(name_buf);
				EM_SAFE_FREE(mail_src->body->html);
				mail_src->body->html = EM_SAFE_STRDUP(name_buf);
			}
		}
	}

	
	if (!mail_src->head->datetime.year && !mail_src->head->datetime.month && !mail_src->head->datetime.day)  {
		/* time isn't set */
		if(!em_core_set_current_time_to_mail_header(mail_src->head, &err)) {
			EM_DEBUG_EXCEPTION("em_core_set_current_time_to_mail_header failed [%d]", err);
			goto FINISH_OFF;
		}
	}
	
	SNPRINTF(datetime, sizeof(datetime), "%04d%02d%02d%02d%02d%02d", 
	mail_src->head->datetime.year, mail_src->head->datetime.month, mail_src->head->datetime.day, mail_src->head->datetime.hour, mail_src->head->datetime.minute, mail_src->head->datetime.second);
	
	mail_table_data->datetime = EM_SAFE_STRDUP(datetime);
	mail_table_data->message_id = EM_SAFE_STRDUP(mail_src->head->mid);

	if (mail_src->info)  {
		mail_src->info->flags.draft = 1;
		if(!em_convert_mail_flag_to_mail_tbl(&(mail_src->info->flags), mail_table_data, &err)) {
			EM_DEBUG_EXCEPTION("em_convert_mail_flag_to_mail_tbl failed [%d]", err);
			goto FINISH_OFF;
		}
		mail_table_data->priority = mail_src->info->extra_flags.priority;
		mail_table_data->lock_status = mail_src->info->extra_flags.lock;
		mail_table_data->report_status = mail_src->info->extra_flags.report;
	}
	mail_table_data->save_status = EMF_MAIL_STATUS_SAVED;
	
	mail_id = mail_table_data->mail_id;
	mail_src->info->uid = mail_id;

	mail_table_data->meeting_request_status = mail_src->info->is_meeting_request;
	
	if(mail_src->info->thread_id == 0) {
		if (em_storage_get_thread_id_of_thread_mails(mail_table_data, &thread_id, &latest_mail_id_in_thread, &thread_item_count) != EMF_ERROR_NONE)
			EM_DEBUG_LOG(" em_storage_get_thread_id_of_thread_mails is failed");
		
		if (thread_id == -1) {
			mail_table_data->thread_id = mail_id;
			mail_table_data->thread_item_count = thread_item_count = 1;
		}
		else  {
			mail_table_data->thread_id = thread_id;
			thread_item_count++;
		}
	}
	else {
		mail_table_data->thread_id = mail_src->info->thread_id;
		thread_item_count = 2;
	}

	/*  Getting attachment count */
	if (mail_src->body)	
		atch = mail_src->body->attachment;
	while (atch)  {
		if (atch->inline_content)
			local_inline_content_count++;
		local_attachment_count++;
		atch = atch->next;
	}

	mail_table_data->inline_content_count = local_inline_content_count;
	mail_table_data->attachment_count     = local_attachment_count;

	EM_DEBUG_LOG("inline_content_count [%d]", local_inline_content_count);
	EM_DEBUG_LOG("attachment_count [%d]", local_attachment_count);
	
	EM_DEBUG_LOG("preview_text[%p]", mail_table_data->preview_text);
	if (mail_table_data->preview_text == NULL) {
		if ( (err = em_core_get_preview_text_from_file(mail_table_data->file_path_plain, mail_table_data->file_path_html, MAX_PREVIEW_TEXT_LENGTH, &(mail_table_data->preview_text))) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_core_get_preview_text_from_file failed[%d]", err);
			goto FINISH_OFF;
		}
	}

	em_storage_begin_transaction(NULL, NULL, NULL);

	/*  insert mail to mail table */
	if (!em_storage_add_mail(mail_table_data, 0, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_add_mail failed [%d]", err);
		/*  ROLLBACK TRANSACTION; */
		em_storage_rollback_transaction(NULL, NULL, NULL);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("thread_item_count [%d]", thread_item_count);
	if (thread_item_count > 1) {
		if (!em_storage_update_latest_thread_mail(mail_table_data->account_id, mail_table_data->thread_id, 0, 0, false, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_update_latest_thread_mail failed [%d]", err);
			em_storage_rollback_transaction(NULL, NULL, NULL);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
	}

	/*  Insert attachment information to DB */
	if (mail_src->body)	
		atch = mail_src->body->attachment;
	EM_DEBUG_LOG("atch [%p]", atch);
	while (atch)  {
		if (from_composer && stat(atch->savename, &st_buf) < 0)  {
			atch = atch->next;
			continue;
		}
		
		if (!atch->inline_content) {
			if (!em_storage_get_new_attachment_no(&attachment_id, &err)) {
				EM_DEBUG_EXCEPTION("em_storage_get_new_attachment_no failed [%d]", err);
				em_storage_rollback_transaction(NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}

		if (!em_storage_create_dir(account_id, mail_id, atch->inline_content ? 0  :  attachment_id, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
			em_storage_rollback_transaction(NULL, NULL, NULL);
			goto FINISH_OFF;
		}
		
		if (!em_storage_get_save_name(account_id, mail_id, atch->inline_content ? 0  :  attachment_id, atch->name, name_buf, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
			em_storage_rollback_transaction(NULL, NULL, NULL);
			goto FINISH_OFF;
		}
		
		if (from_composer || atch->downloaded) {	
			if (!em_storage_copy_file(atch->savename, name_buf, from_composer, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_copy_file failed [%d]", err);
				em_storage_rollback_transaction(NULL, NULL, NULL);
				goto FINISH_OFF;
			}
			
			if ((ext = strrchr(atch->name, '.'))) {	
				if (!strncmp(ext, ".vcs", strlen(".vcs")))
					remove(atch->savename);
				else if (!strncmp(ext, ".vcf", strlen(".vcf")))
					remove(atch->savename);
				else if (!strncmp(ext, ".vnt", strlen(".vnt")))
					remove(atch->savename);
			}
		}
		
		memset(&attachment_tbl, 0x00, sizeof(emf_mail_attachment_tbl_t));
		
		attachment_tbl.mail_id         = mail_id;
		attachment_tbl.account_id      = account_id;
		attachment_tbl.mailbox_name    = mailbox_name_target;
		attachment_tbl.file_yn         = atch->downloaded;
		attachment_tbl.flag2           = mail_src->body->attachment->drm;  
		attachment_tbl.attachment_name = atch->name;
		attachment_tbl.attachment_path = name_buf;
		attachment_tbl.flag3           = atch->inline_content;
		if (from_composer)
			attachment_tbl.attachment_size = st_buf.st_size;
		else
			attachment_tbl.attachment_size = atch->size;

		if (!em_storage_add_attachment(&attachment_tbl, 0, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_add_attachment failed [%d]", err);
			em_storage_rollback_transaction(NULL, NULL, NULL);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		atch->attachment_id = attachment_tbl.attachment_id;		
		atch = atch->next;
	}

	/*  Insert Meeting request to DB */
	if (mail_src->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_REQUEST
		|| mail_src->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_RESPONSE
		|| mail_src->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		EM_DEBUG_LOG("This mail has the meeting request");
		meeting_req->mail_id = mail_id;
		if (!em_storage_add_meeting_request(account_id, mailbox_name_target, meeting_req, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_add_meeting_request failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}			
	}

	em_storage_commit_transaction(NULL, NULL, NULL);

	if (!em_storage_notify_storage_event(NOTI_MAIL_ADD, account_id, mail_id, mailbox_name_target, mail_table_data->thread_id))
			EM_DEBUG_LOG("em_storage_notify_storage_event [NOTI_MAIL_ADD] failed.");

	if (account_tbl_item->receiving_server_type != EMF_SERVER_TYPE_ACTIVE_SYNC) {
		if (!em_core_mailbox_remove_overflowed_mails(mailbox_tbl_data, &err)) {
			if (err == EM_STORAGE_ERROR_MAIL_NOT_FOUND || err == EMF_ERROR_NOT_SUPPORTED)
				err = EMF_ERROR_NONE;
			else
				EM_DEBUG_LOG("em_core_mailbox_remove_overflowed_mails failed [%d]", err);
		}	
	}

	if ( !from_composer && (mail_src->info->flags.seen == 0) 
				&& mail_table_data->mailbox_type != EMF_MAILBOX_TYPE_TRASH 
				&& mail_table_data->mailbox_type != EMF_MAILBOX_TYPE_SPAMBOX) { /* 0x01 fSeen */
		if (!em_storage_update_sync_status_of_account(account_id, SET_TYPE_SET, SYNC_STATUS_SYNCING | SYNC_STATUS_HAVE_NEW_MAILS, true, &err)) 
				EM_DEBUG_LOG("em_storage_update_sync_status_of_account failed [%d]", err);
		em_core_add_notification_for_unread_mail_by_mail_header(account_id, mail_id, mail_src->head);
		em_core_check_unread_mail();
	}

	ret = true;
	
FINISH_OFF: 
	if (mail_table_data)
		em_storage_free_mail(&mail_table_data, 1, NULL);

	if (account_tbl_item)
		em_storage_free_account(&account_tbl_item, 1, NULL);

	if (mailbox_tbl_data)
		em_storage_free_mailbox(&mailbox_tbl_data, 1, NULL);
	
	EM_SAFE_FREE(mailbox_name_spam);
	EM_SAFE_FREE(mailbox_name_target);
	
	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}


EXPORT_API int em_core_add_mail(emf_mail_data_t *input_mail_data, emf_attachment_data_t *input_attachment_data_list, int input_attachment_count, emf_meeting_request_t *input_meeting_request, int input_sync_server)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list [%p], input_attachment_count [%d], input_meeting_request [%p], input_sync_server[%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_sync_server);
	
	int err = EMF_ERROR_NONE;
	int attachment_id = 0, thread_id = -1, thread_item_count = 0, latest_mail_id_in_thread = -1;
	int i = 0, rule_len, rule_matched = -1, local_attachment_count = 0, local_inline_content_count = 0;
	char *ext = NULL, *mailbox_name_spam = NULL, *mailbox_name_target = NULL;
	char name_buf[MAX_PATH] = {0x00, }, html_body[MAX_PATH] = {0x00, };
	emf_mail_tbl_t    *converted_mail_tbl = NULL;
	emf_mailbox_tbl_t *mailbox_tbl = NULL;
	emf_mail_attachment_tbl_t attachment_tbl = { 0 };
	emf_mail_account_tbl_t *account_tbl_item = NULL;
	emf_mail_rule_tbl_t *rule = NULL;
	struct stat st_buf = { 0 };

	/* Validating parameters */
	
	if (!input_mail_data || !(input_mail_data->account_id) || !(input_mail_data->mailbox_name))  {	
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!em_storage_get_account_by_id(input_mail_data->account_id, EMF_ACC_GET_OPT_DEFAULT | EMF_ACC_GET_OPT_OPTIONS, &account_tbl_item, true, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_get_account_by_id failed. account_id[%d] err[%d]", input_mail_data->account_id, err);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if(input_sync_server) {
		if (input_mail_data->file_path_plain)  {
			if (stat(input_mail_data->file_path_plain, &st_buf) < 0)  {
				EM_DEBUG_EXCEPTION("input_mail_data->file_path_plain, stat(\"%s\") failed...", input_mail_data->file_path_plain);
				err = EMF_ERROR_INVALID_MAIL;
				goto FINISH_OFF;
			}
		}
		
		if (input_mail_data->file_path_html)  {
			if (stat(input_mail_data->file_path_html, &st_buf) < 0)  {
				EM_DEBUG_EXCEPTION("input_mail_data->file_path_html, stat(\"%s\") failed...", input_mail_data->file_path_html);
				err = EMF_ERROR_INVALID_MAIL;
				goto FINISH_OFF;
			}
		}
		
		if (input_attachment_count && input_attachment_data_list)  {
			for (i = 0; i < input_attachment_count; i++)  {
				if (input_attachment_data_list[i].save_status) {
					if (!input_attachment_data_list[i].attachment_path || stat(input_attachment_data_list[i].attachment_path, &st_buf) < 0)  {
						EM_DEBUG_EXCEPTION("stat(\"%s\") failed...", input_attachment_data_list[i].attachment_path);
						err = EMF_ERROR_INVALID_ATTACHMENT;		
						goto FINISH_OFF;
					}
				}
			}
		}
	}

	if (input_sync_server) {
		if (!input_mail_data->full_address_from) 
			input_mail_data->full_address_from = EM_SAFE_STRDUP(account_tbl_item->email_addr);

		/* check for email_address validation */
		if (!em_core_verify_email_address_of_mail_data(input_mail_data, false, &err)) {
			EM_DEBUG_EXCEPTION("em_core_verify_email_address_of_mail_data failed [%d]", err);
			goto FINISH_OFF;
		}
		
		if (input_mail_data->report_status == EMF_MAIL_REPORT_MDN)  {	
			/* check read-report mail */
			if(!input_mail_data->full_address_to) { /* A report mail should have 'to' address */
				EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
				err = EMF_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}
			/* Create report mail body */
			/*
			if (!em_core_mail_get_rfc822(mail_src, NULL, NULL, NULL, &err))  {
				EM_DEBUG_EXCEPTION("em_core_mail_get_rfc822 failed [%d]", err);
				goto FINISH_OFF;
			}
			*/
		}

		mailbox_name_target = EM_SAFE_STRDUP(input_mail_data->mailbox_name);
	}
	else {	/*  For Spam handling */
		emf_option_t *opt = &account_tbl_item->options;
		EM_DEBUG_LOG("block_address [%d], block_subject [%d]", opt->block_address, opt->block_subject);
		
		if (opt->block_address || opt->block_subject)  {
			int is_completed = false;
			int type = 0;
			
			if (!opt->block_address)
				type = EMF_FILTER_SUBJECT;
			else if (!opt->block_subject)
				type = EMF_FILTER_FROM;
			
			if (!em_storage_get_rule(ALL_ACCOUNT, type, 0, &rule_len, &is_completed, &rule, true, &err) || !rule) 
				EM_DEBUG_LOG("No proper rules. em_storage_get_rule returns [%d]", err);
		}

		if (rule) {
			if (!em_storage_get_mailboxname_by_mailbox_type(input_mail_data->account_id, EMF_MAILBOX_TYPE_SPAMBOX, &mailbox_name_spam, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				mailbox_name_spam = NULL;
			}

			/*
			if (mailbox_name_spam && !em_core_mail_check_rule(mail_src->head, rule, rule_len, &rule_matched, &err))  {
				EM_DEBUG_EXCEPTION("em_core_mail_check_rule failed [%d]", err);
				goto FINISH_OFF;
			}
			*/
		}

		if (rule_matched >= 0 && mailbox_name_spam) 
			mailbox_name_target = EM_SAFE_STRDUP(mailbox_name_spam);
		else
			mailbox_name_target = EM_SAFE_STRDUP(input_mail_data->mailbox_name);
	}
	
	if (!em_storage_get_mailbox_by_name(input_mail_data->account_id, -1, mailbox_name_target, (emf_mailbox_tbl_t **)&mailbox_tbl, false, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);		
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	if (!em_storage_increase_mail_id(&input_mail_data->mail_id, true, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_increase_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("input_mail_data->mail_size [%d]", input_mail_data->mail_size);

	if(input_mail_data->mail_size == 0)
		em_core_calc_mail_size(input_mail_data, input_attachment_data_list, input_attachment_count, &(input_mail_data->mail_size)); /*  Getting file size before file moved.  */

	if (input_sync_server || input_mail_data->body_download_status) {
		if (!em_storage_create_dir(input_mail_data->account_id, input_mail_data->mail_id, 0, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (input_mail_data->file_path_plain) {
			EM_DEBUG_LOG("input_mail_data->file_path_plain [%s]", input_mail_data->file_path_plain);
			/* if (!em_storage_get_save_name(account_id, mail_id, 0, input_mail_data->body->plain_charset ? input_mail_data->body->plain_charset  :  "UTF-8", name_buf, &err))  {*/
			if (!em_storage_get_save_name(input_mail_data->account_id, input_mail_data->mail_id, 0, "UTF-8", name_buf, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
				goto FINISH_OFF;
			}
		
			if (!em_storage_move_file(input_mail_data->file_path_plain, name_buf, input_sync_server, &err)) {
				EM_DEBUG_EXCEPTION("em_storage_move_file failed [%d]", err);
				goto FINISH_OFF;
			}
			if (input_mail_data->body_download_status == EMF_BODY_DOWNLOAD_STATUS_NONE)
				input_mail_data->body_download_status = EMF_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;

			EM_SAFE_FREE(input_mail_data->file_path_plain);
			input_mail_data->file_path_plain = EM_SAFE_STRDUP(name_buf);
		}

		if (input_mail_data->file_path_html) {
			EM_DEBUG_LOG("input_mail_data->file_path_html [%s]", input_mail_data->file_path_html);
			memcpy(html_body, "UTF-8.htm", strlen("UTF-8.htm"));
				
			if (!em_storage_get_save_name(input_mail_data->account_id, input_mail_data->mail_id, 0, html_body, name_buf, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
				goto FINISH_OFF;
			}

			if (!em_storage_move_file(input_mail_data->file_path_html, name_buf, input_sync_server, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_move_file failed [%d]", err);
				goto FINISH_OFF;
			}

			if (input_mail_data->body_download_status == EMF_BODY_DOWNLOAD_STATUS_NONE)
				input_mail_data->body_download_status = EMF_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;

			EM_SAFE_FREE(input_mail_data->file_path_html);
			input_mail_data->file_path_html = EM_SAFE_STRDUP(name_buf);
		}
	}

	
	if (!input_mail_data->datetime)  {
		/* time isn't set */
		input_mail_data->datetime = em_core_get_current_time_string(&err);
		if(!input_mail_data->datetime) {
			EM_DEBUG_EXCEPTION("em_core_get_current_time_string failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	EM_SAFE_FREE(input_mail_data->mailbox_name);

	input_mail_data->mailbox_name         = EM_SAFE_STRDUP(mailbox_name_target);
	input_mail_data->mailbox_type         = mailbox_tbl->mailbox_type;
	input_mail_data->server_mail_status   = !input_sync_server;
	input_mail_data->save_status          = EMF_MAIL_STATUS_SAVED;

	/*  Getting attachment count */
	for (i = 0; i < input_attachment_count; i++) {
		if (input_attachment_data_list[i].inline_content_status== 1)
			local_inline_content_count++;
		local_attachment_count++;
	}

	input_mail_data->inline_content_count = local_inline_content_count;
	input_mail_data->attachment_count     = local_attachment_count;

	EM_DEBUG_LOG("inline_content_count   [%d]", local_inline_content_count);
	EM_DEBUG_LOG("input_attachment_count [%d]", local_attachment_count);

	EM_DEBUG_LOG("preview_text[%p]", input_mail_data->preview_text);
	if (input_mail_data->preview_text == NULL) {
		if ( (err = em_core_get_preview_text_from_file(input_mail_data->file_path_plain, input_mail_data->file_path_html, MAX_PREVIEW_TEXT_LENGTH, &(input_mail_data->preview_text))) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_core_get_preview_text_from_file failed[%d]", err);
			goto FINISH_OFF;
		}
	}

	if (!em_convert_mail_data_to_mail_tbl(input_mail_data, 1, &converted_mail_tbl, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_data_to_mail_tbl failed [%d]", err);
		goto FINISH_OFF;
	}
	
	/* Fill address information */
	em_core_fill_address_information_of_mail_tbl(converted_mail_tbl);

	/* Fill thread id */
	if(input_mail_data->thread_id == 0) {
		if (em_storage_get_thread_id_of_thread_mails(converted_mail_tbl, &thread_id, &latest_mail_id_in_thread, &thread_item_count) != EMF_ERROR_NONE)
			EM_DEBUG_LOG(" em_storage_get_thread_id_of_thread_mails is failed");
		
		if (thread_id == -1) {
			converted_mail_tbl->thread_id         = input_mail_data->mail_id;
			converted_mail_tbl->thread_item_count = thread_item_count = 1;
		}
		else  {
			converted_mail_tbl->thread_id         = thread_id;
			thread_item_count++;
		}
	}
	else {
		thread_item_count                    = 2;
	}

	em_storage_begin_transaction(NULL, NULL, NULL);

	/*  insert mail to mail table */
	if (!em_storage_add_mail(converted_mail_tbl, 0, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_add_mail failed [%d]", err);
		/*  ROLLBACK TRANSACTION; */
		em_storage_rollback_transaction(NULL, NULL, NULL);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	/* Update thread information */
	EM_DEBUG_LOG("thread_item_count [%d]", thread_item_count);

	if (thread_item_count > 1) {
		if (!em_storage_update_latest_thread_mail(input_mail_data->account_id, converted_mail_tbl->thread_id, 0, 0, false, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_update_latest_thread_mail failed [%d]", err);
			em_storage_rollback_transaction(NULL, NULL, NULL);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
	}

	/*  Insert attachment information to DB */

	for (i = 0; i < input_attachment_count; i++) {
		if (input_attachment_data_list[i].attachment_size == 0) { 
			/* set attachment size */
			if(input_attachment_data_list[i].attachment_path && stat(input_attachment_data_list[i].attachment_path, &st_buf) < 0) 
				input_attachment_data_list[i].attachment_size = st_buf.st_size;
		}
		
		if (!input_attachment_data_list[i].inline_content_status) {
			if (!em_storage_get_new_attachment_no(&attachment_id, &err)) {
				EM_DEBUG_EXCEPTION("em_storage_get_new_attachment_no failed [%d]", err);
				em_storage_rollback_transaction(NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}

		if (!em_storage_create_dir(input_mail_data->account_id, input_mail_data->mail_id, input_attachment_data_list[i].inline_content_status ? 0  :  attachment_id, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
			em_storage_rollback_transaction(NULL, NULL, NULL);
			goto FINISH_OFF;
		}
		
		if (!em_storage_get_save_name(input_mail_data->account_id, input_mail_data->mail_id, input_attachment_data_list[i].inline_content_status ? 0  :  attachment_id, input_attachment_data_list[i].attachment_name, name_buf, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
			em_storage_rollback_transaction(NULL, NULL, NULL);
			goto FINISH_OFF;
		}
		
		if (input_sync_server || input_attachment_data_list[i].save_status) {	
			if (!em_storage_copy_file(input_attachment_data_list[i].attachment_path, name_buf, input_sync_server, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_copy_file failed [%d]", err);
				em_storage_rollback_transaction(NULL, NULL, NULL);
				goto FINISH_OFF;
			}
			
			if ((ext = strrchr(input_attachment_data_list[i].attachment_name, '.'))) {	
				if (!strncmp(ext, ".vcs", strlen(".vcs")))
					remove(input_attachment_data_list[i].attachment_path);
				else if (!strncmp(ext, ".vcf", strlen(".vcf")))
					remove(input_attachment_data_list[i].attachment_path);
				else if (!strncmp(ext, ".vnt", strlen(".vnt")))
					remove(input_attachment_data_list[i].attachment_path);
			}
		}

		memset(&attachment_tbl, 0, sizeof(emf_mail_attachment_tbl_t));
		attachment_tbl.attachment_name = input_attachment_data_list[i].attachment_name;
		attachment_tbl.attachment_path = name_buf;
		attachment_tbl.attachment_size = input_attachment_data_list[i].attachment_size;
		attachment_tbl.mail_id         = input_mail_data->mail_id;
		attachment_tbl.account_id      = input_mail_data->account_id;
		attachment_tbl.mailbox_name    = input_mail_data->mailbox_name;
		attachment_tbl.file_yn         = input_attachment_data_list[i].save_status;
		attachment_tbl.flag2           = input_attachment_data_list[i].drm_status;
		attachment_tbl.flag3           = input_attachment_data_list[i].inline_content_status;

		if (!em_storage_add_attachment(&attachment_tbl, 0, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_add_attachment failed [%d]", err);
			em_storage_rollback_transaction(NULL, NULL, NULL);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		input_attachment_data_list[i].attachment_id = attachment_tbl.attachment_id;
	}

	/*  Insert Meeting request to DB */
	if (input_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_REQUEST
		|| input_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_RESPONSE
		|| input_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		EM_DEBUG_LOG("This mail has the meeting request");
		input_meeting_request->mail_id = input_mail_data->mail_id;
		if (!em_storage_add_meeting_request(input_mail_data->account_id, mailbox_name_target, input_meeting_request, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_add_meeting_request failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}			
	}

	em_storage_commit_transaction(NULL, NULL, NULL);

	if (!em_storage_notify_storage_event(NOTI_MAIL_ADD, converted_mail_tbl->account_id, converted_mail_tbl->mail_id, mailbox_name_target, converted_mail_tbl->thread_id))
			EM_DEBUG_LOG("em_storage_notify_storage_event [NOTI_MAIL_ADD] failed.");

	if (account_tbl_item->receiving_server_type != EMF_SERVER_TYPE_ACTIVE_SYNC) {
		if (!em_core_mailbox_remove_overflowed_mails(mailbox_tbl, &err)) {
			if (err == EM_STORAGE_ERROR_MAIL_NOT_FOUND || err == EMF_ERROR_NOT_SUPPORTED)
				err = EMF_ERROR_NONE;
			else
				EM_DEBUG_LOG("em_core_mailbox_remove_overflowed_mails failed [%d]", err);
		}	
	}

	if ( !input_sync_server && (input_mail_data->flags_seen_field == 0) 
				&& input_mail_data->mailbox_type != EMF_MAILBOX_TYPE_TRASH 
				&& input_mail_data->mailbox_type != EMF_MAILBOX_TYPE_SPAMBOX) { 
		if (!em_storage_update_sync_status_of_account(input_mail_data->account_id, SET_TYPE_SET, SYNC_STATUS_SYNCING | SYNC_STATUS_HAVE_NEW_MAILS, true, &err)) 
				EM_DEBUG_LOG("em_storage_update_sync_status_of_account failed [%d]", err);
		em_core_add_notification_for_unread_mail(input_mail_data);
		em_core_check_unread_mail();
	}
	
FINISH_OFF: 
	if (account_tbl_item)
		em_storage_free_account(&account_tbl_item, 1, NULL);

	if (mailbox_tbl)
		em_storage_free_mailbox(&mailbox_tbl, 1, NULL);

	if (converted_mail_tbl)
		em_storage_free_mail(&converted_mail_tbl, 1, NULL);
	
	EM_SAFE_FREE(mailbox_name_spam);
	EM_SAFE_FREE(mailbox_name_target);
	
	EM_DEBUG_FUNC_END();
	return err;
}

EXPORT_API int  
em_core_mail_add_meeting_request(int account_id, char *mailbox_name, emf_meeting_request_t *meeting_req, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%s], meeting_req[%p], err_code[%p]", account_id, mailbox_name, meeting_req, err_code);
	int ret = false;
	int err = EMF_ERROR_NONE;

	if (!meeting_req || meeting_req->mail_id <= 0) {
		if (meeting_req)
			EM_DEBUG_EXCEPTION("mail_id[%d]", meeting_req->mail_id);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!em_storage_add_meeting_request(account_id, mailbox_name, meeting_req, 1, &err))  {
		EM_DEBUG_EXCEPTION(" em_storage_add_meeting_request failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}	

	ret = true;
	
FINISH_OFF: 
	
	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;	
}

/*  send a message (not saved) */
EXPORT_API int em_core_mail_send(int account_id, char *input_mailbox_name, int mail_id, emf_option_t *sending_option, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], input_mailbox_name[%s], mail_id[%d], sending_option[%p], err_code[%p]", account_id, input_mailbox_name, mail_id, sending_option, err_code);
	EM_PROFILE_BEGIN(profile_em_core_mail_send);
	int ret = false;
	int err = EMF_ERROR_NONE, err2 = EMF_ERROR_NONE;
	int status = EMF_SEND_FAIL;
	
	SENDSTREAM *stream = NULL;
	ENVELOPE *envelope = NULL;
	sslstart_t stls = NULL;
	emf_mail_t *mail = NULL;
	emf_account_t *ref_account = NULL;
	emf_option_t *opt = NULL;
	emf_mailbox_t dest_mbox = {0};
	void *tmp_stream = NULL;
	char *fpath = NULL;
	int sent_box = 0;
	char *mailbox_name = NULL;
	
	if (!account_id || !mail_id)  {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!(ref_account = em_core_get_account_reference(account_id)))  {
		EM_DEBUG_EXCEPTION("em_core_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/*  get mail to send */
	if (!em_core_mail_get_mail(mail_id, &mail, &err))  {
		EM_DEBUG_EXCEPTION("em_core_mail_get_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!em_core_check_send_mail_thread_status()) {
		EM_DEBUG_EXCEPTION("em_core_check_send_mail_thread_status failed...");
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	if ((!mail->head->to) && (!mail->head->cc) && (!mail->head->bcc)) {
		err = EMF_ERROR_NO_RECIPIENT;
		EM_DEBUG_EXCEPTION("No Recipient information [%d]", err);
		goto FINISH_OFF;
	}
	else {
		if (!em_core_verify_email_address_of_mail_header(mail->head, false, &err)) {
			err = EMF_ERROR_INVALID_ADDRESS;
			EM_DEBUG_EXCEPTION("em_core_verify_email_address_of_mail_header failed [%d]", err);
			goto FINISH_OFF;
		}
	}
	
	if (mail->info)
		mail->info->account_id = account_id;		
	
	if (!em_core_check_send_mail_thread_status()) {
		EM_DEBUG_EXCEPTION("em_core_check_send_mail_thread_status failed...");
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	if (sending_option != NULL)
		opt = sending_option;
	else
		opt = em_core_get_option(&err);
	
	
	/*Update status flag to DB*/
	
	/*  get rfc822 data */
	if (!em_core_mail_get_rfc822(mail, &envelope, &fpath, opt, &err))  {
		EM_DEBUG_EXCEPTION("em_core_mail_get_rfc822 failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!envelope || (!envelope->to && !envelope->cc && !envelope->bcc))  {
		EM_DEBUG_EXCEPTION(" no recipients found...");
		
		err = EMF_ERROR_NO_RECIPIENT;
		goto FINISH_OFF;
	}
	
	/*  if there is no security option, unset security. */
	if (!ref_account->sending_security)  {
		stls = (sslstart_t)mail_parameters(NULL, GET_SSLSTART, NULL);
		mail_parameters(NULL, SET_SSLSTART, NULL);
	}

	if (!em_core_check_send_mail_thread_status()) {
		EM_DEBUG_EXCEPTION("em_core_check_send_mail_thread_status failed...");
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	if (ref_account->pop_before_smtp != FALSE)  {
		if (!em_core_mailbox_open(account_id, NULL, (void **)&tmp_stream, &err))  {
			EM_DEBUG_EXCEPTION(" POP before SMTP Authentication failed [%d]", err);
			status = EMF_LIST_CONNECTION_FAIL;
			if (err == EMF_ERROR_CONNECTION_BROKEN)
				err = EMF_ERROR_CANCELLED;		
			goto FINISH_OFF;
		}
	}


	if (!em_storage_get_mailboxname_by_mailbox_type(account_id, EMF_MAILBOX_TYPE_DRAFT, &mailbox_name, false, &err))  {
		EM_DEBUG_EXCEPTION(" em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	if (!em_core_mailbox_open(account_id, (char *)ENCODED_PATH_SMTP, (void **)&tmp_stream, &err))  {
		EM_DEBUG_EXCEPTION(" em_core_mailbox_open failed [%d]", err);
		
		if (err == EMF_ERROR_CONNECTION_BROKEN)
			err = EMF_ERROR_CANCELLED;
		
		status = EMF_SEND_CONNECTION_FAIL;
		goto FINISH_OFF;
	}
	
	stream = (SENDSTREAM *)tmp_stream;

	if (!em_core_check_send_mail_thread_status()) {
		EM_DEBUG_EXCEPTION(" em_core_check_send_mail_thread_status failed...");
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	/*  set request of delivery status. */
	if (opt->req_delivery_receipt == EMF_OPTION_REQ_DELIVERY_RECEIPT_ON)  { 
		stream->protocol.esmtp.dsn.want = 1;
		stream->protocol.esmtp.dsn.full = 0;
		stream->protocol.esmtp.dsn.notify.failure = 1;
		stream->protocol.esmtp.dsn.notify.success = 1;
		EM_DEBUG_LOG("opt->req_delivery_receipt == EMF_OPTION_REQ_DELIVERY_RECEIPT_ON");
	}
	
	mail->info->extra_flags.status = EMF_MAIL_STATUS_SENDING;

	/*Update status flag to DB*/
	if (!em_core_mail_modify_extra_flag(mail_id, mail->info->extra_flags, &err))
		EM_DEBUG_EXCEPTION("Failed to modify extra flag [%d]", err);

	/*  send mail to server. */
	if (!em_core_mail_send_smtp(stream, envelope, fpath, account_id, mail_id, &err)) {
		EM_DEBUG_EXCEPTION(" em_core_mail_send_smtp failed [%d]", err);

		
#ifndef __FEATURE_MOVE_TO_OUTBOX_FIRST__
	EM_SAFE_FREE(mailbox_name);
	if (!em_storage_get_mailboxname_by_mailbox_type(account_id, EMF_MAILBOX_TYPE_OUTBOX, &mailbox_name, false, &err))  {
		EM_DEBUG_EXCEPTION(" em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	dest_mbox.name = mailbox_name;
	dest_mbox.account_id = account_id;
		
	/*  unsent mail is moved to 'OUTBOX'. */
	if (!em_core_mail_move(&mail_id, 1, dest_mbox.name, EMF_MOVED_BY_COMMAND, 0, NULL)) 
		EM_DEBUG_EXCEPTION(" em_core_mail_move falied...");
#endif
	

		goto FINISH_OFF;
	}
	
	/*  sent mail is moved to 'SENT' box or deleted. */
	if (opt->keep_local_copy)  {
		EM_SAFE_FREE(mailbox_name);
		
		if (!em_storage_get_mailboxname_by_mailbox_type(account_id, EMF_MAILBOX_TYPE_SENTBOX, &mailbox_name, false, &err))  {
			EM_DEBUG_EXCEPTION(" em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		dest_mbox.name = mailbox_name;
		dest_mbox.account_id = account_id;
		
		if (!em_core_mail_move(&mail_id, 1, dest_mbox.name, EMF_MOVED_AFTER_SENDING, 0, &err)) 
			EM_DEBUG_EXCEPTION(" em_core_mail_move falied [%d]", err);
#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__		
#ifdef __LOCAL_ACTIVITY__		
		else if (ref_account->receiving_server_type == EMF_SERVER_TYPE_IMAP4) /* To be synced to Sent box only IMAP not for POP */ {

			emf_activity_tbl_t new_activity;
			int activityid = 0;
			
			if (false == em_core_get_next_activity_id(&activityid, &err)) {
				EM_DEBUG_EXCEPTION(" em_core_get_next_activity_id Failed [%d] ", err);
			}

			memset(&new_activity, 0x00, sizeof(emf_activity_tbl_t));
			new_activity.activity_id  =  activityid;
			new_activity.server_mailid = NULL;
			new_activity.account_id	= account_id;
			new_activity.mail_id	= mail_id;
			new_activity.activity_type = ACTIVITY_SAVEMAIL;
			new_activity.dest_mbox	= NULL;
			new_activity.src_mbox	= NULL;
		
			if (!em_core_activity_add(&new_activity, &err)) {
				EM_DEBUG_EXCEPTION(" em_core_activity_add Failed [%d] ", err);
			}
			
			if (!em_core_mail_move_from_server(dest_mbox.account_id, mailbox_name, &mail_id, 1, dest_mbox.name, &err)) {
				EM_DEBUG_EXCEPTION(" em_core_mail_move_from_server falied [%d]", err);
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
				
				if (!em_core_activity_delete(&new_activity, &err)) {
					EM_DEBUG_EXCEPTION(">>>>>>Local Activity [ACTIVITY_SAVEMAIL] [%d] ", err);
				}
			}
			sent_box = 1;			
		}
#endif
#endif		

		/* On Successful Mail sent remove the Draft flag */
		emf_mail_flag_t update_flag;
		if (mail->info)
			memcpy(&update_flag, &(mail->info->flags), sizeof(emf_mail_flag_t));
		else
			memset(&update_flag, 0x00, sizeof(emf_mail_flag_t));
		update_flag.draft = 0;
		int sticky_flag = 1;
		if (!em_core_mail_modify_flag(mail_id, update_flag, sticky_flag, &err))
			EM_DEBUG_EXCEPTION(" Flag Modification Failed [ %d] ", err);
		sent_box = 1; 
	}
	else  {
		if (!em_core_mail_delete(account_id, &mail_id, 1, 0, EMF_DELETED_AFTER_SENDING, false, &err)) 
			EM_DEBUG_EXCEPTION(" em_core_mail_delete failed [%d]", err);
	}
	
	/*Update status flag to DB*/
		mail->info->extra_flags.status = EMF_MAIL_STATUS_SENT;
	if (!em_core_mail_modify_extra_flag(mail_id, mail->info->extra_flags, &err))
		EM_DEBUG_EXCEPTION("Failed to modify extra flag [%d]", err);
	/*Update status flag to DB*/
		if (!em_core_delete_transaction_info_by_mailId(mail_id))
			EM_DEBUG_EXCEPTION(" em_core_delete_transaction_info_by_mailId failed for mail_id[%d]", mail_id);

	ret = true;
	
FINISH_OFF: 
	if (ret == false && err != EMF_ERROR_INVALID_PARAM && mail)  {
		if (err != EMF_ERROR_CANCELLED) {
			mail->info->extra_flags.status = EMF_MAIL_STATUS_SEND_FAILURE;
			if (!em_core_mail_modify_extra_flag(mail_id, mail->info->extra_flags, &err2))
				EM_DEBUG_EXCEPTION("Failed to modify extra flag [%d]", err2);
		}
		else {
			if (EMF_MAIL_STATUS_SEND_CANCELED == mail->info->extra_flags.status)
				EM_DEBUG_LOG("EMF_MAIL_STATUS_SEND_CANCELED Already set for ");
			else {	
				mail->info->extra_flags.status = EMF_MAIL_STATUS_SEND_CANCELED;
				if (!em_core_mail_modify_extra_flag(mail_id, mail->info->extra_flags, &err2))
					EM_DEBUG_EXCEPTION("Failed to modify extra flag [%d]", err2);
			}
		}
	}

#ifndef __FEATURE_KEEP_CONNECTION__
	if (stream) 
		smtp_close(stream);
#endif /* __FEATURE_KEEP_CONNECTION__ */
	if (stls) 
		mail_parameters(NULL, SET_SSLSTART, (void  *)stls);
	if (mail) 
		em_core_mail_free(&mail, 1, NULL);
	if (envelope) 
		mail_free_envelope(&envelope);
	
	if (fpath) {
		EM_DEBUG_LOG("REMOVE TEMP FILE  :  %s", fpath);
		remove(fpath);
		free(fpath);
	}
	
	if (ret == true) {
		if (!em_storage_notify_network_event(NOTI_SEND_FINISH, account_id, NULL, mail_id, 0))
			EM_DEBUG_EXCEPTION("em_storage_notify_network_event [NOTI_SEND_FINISH] Failed");
	}
	else {
		if (!em_storage_notify_network_event(NOTI_SEND_FAIL, account_id, NULL, mail_id, err))
			EM_DEBUG_EXCEPTION("em_storage_notify_network_event [NOTI_SEND_FAIL] Failed");
		em_core_show_popup(mail_id, EMF_ACTION_SEND_MAIL, err);
	}
	
	EM_SAFE_FREE(mailbox_name);
	
	if (err_code != NULL)
		*err_code = err;
	EM_PROFILE_END(profile_em_core_mail_send);
	EM_DEBUG_FUNC_END("ret [%d], err [%d]", ret, err);
	return ret;
}

/*  send a saved message */
EXPORT_API int em_core_mail_send_saved(int account_id, char *input_mailbox_name, emf_option_t *sending_option,  int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], input_mailbox_name[%p], sending_option[%p], err_code[%p]", account_id, input_mailbox_name, sending_option, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int status = EMF_SEND_FAIL;
	
	SENDSTREAM *stream = NULL;
	ENVELOPE *envelope = NULL;
	emf_mailbox_t	dest_mbox = {0};
	emf_mail_t *mail = NULL;
	emf_account_t *ref_account = NULL;
	emf_mail_tbl_t	mail_table_data = {0};
	emf_option_t *opt = NULL;
	sslstart_t	stls = NULL;
	void *tmp_stream = NULL;
	void *p = NULL;
	char *fpath = NULL;
	int mail_id = 0; 
	int handle = 0;
	int i = 0;
	int total = 0;
	char *mailbox_name = NULL;	
	
	if (!account_id || !input_mailbox_name)  {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	mail_send_notify(EMF_SEND_PREPARE, 0, 0, account_id, mail_id, err);
	
	if (!(ref_account = em_core_get_account_reference(account_id)))  {
		EM_DEBUG_EXCEPTION("em_core_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	FINISH_OFF_IF_CANCELED;
	
	if (sending_option)
		opt = sending_option;
	else
		opt = em_core_get_option(&err);
	
	/*  search mail. */
	if (!em_storage_mail_search_start(NULL, account_id, input_mailbox_name, 0, &handle, &total, true, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_mail_search_start failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	while (i++ < total)  {
		FINISH_OFF_IF_CANCELED;
		
		p = NULL;
		if (!em_storage_mail_search_result(handle, RETRIEVE_ID, (void **)&p, true, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_mail_search_result failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		mail_id = (int)p;
		
		if (!em_core_mail_get_mail(mail_id, &mail, &err))  {
			EM_DEBUG_EXCEPTION("em_core_mail_get_mail failed [%d]", err);
			goto FINISH_OFF;
		}

		/* check for email_address validation */
		if (!em_core_verify_email_address_of_mail_header(mail->head, false, &err)) {
			EM_DEBUG_EXCEPTION("em_core_verify_email_address_of_mail_header failed [%d]", err);
			goto FINISH_OFF;
		}

		/*  check that this mail was saved in offline-mode. */
		if (mail->info->extra_flags.status != EMF_MAIL_STATUS_SAVED_OFFLINE)  {
			EM_DEBUG_EXCEPTION(" mail was not saved in offline mode...");
			em_core_mail_free(&mail, 1, &err); mail = NULL;
			continue;
		}
		
		if(!em_convert_mail_flag_to_mail_tbl(&(mail->info->flags), &mail_table_data, &err)) {
			EM_DEBUG_EXCEPTION("em_convert_mail_flag_to_mail_tbl failed [%d]", err);
			goto FINISH_OFF;
		}
		
		mail_table_data.save_status = mail->info->extra_flags.status;
		mail_table_data.lock_status = mail->info->extra_flags.lock;
		mail_table_data.priority = mail->info->extra_flags.priority;
		mail_table_data.report_status = mail->info->extra_flags.report;
		
		mail->info->extra_flags.status = EMF_MAIL_STATUS_SENDING;
		
		if (!em_core_mail_get_rfc822(mail, &envelope, &fpath, opt, &err))  {
			EM_DEBUG_EXCEPTION("em_core_mail_get_rfc822 falied [%d]", err);
			goto FINISH_OFF;
		}
		
		FINISH_OFF_IF_CANCELED;
		
		/*  connect mail server. */
		if (!stream) {
			/*  if there no security option, unset security. */
			if (!ref_account->sending_security) {
				stls = (sslstart_t)mail_parameters(NULL, GET_SSLSTART, NULL);
				mail_parameters(NULL, SET_SSLSTART, NULL);
			}
			
			stream = NULL;
			if (!em_core_mailbox_open(account_id, (char *)ENCODED_PATH_SMTP, &tmp_stream, &err) || !tmp_stream)  {
				EM_DEBUG_EXCEPTION("em_core_mailbox_open failed [%d]", err);

				if (err == EMF_ERROR_CONNECTION_BROKEN)
					err = EMF_ERROR_CANCELLED;
				
				status = EMF_SEND_CONNECTION_FAIL;
				goto FINISH_OFF;
			}
			
			stream = (SENDSTREAM *)tmp_stream;
			
			FINISH_OFF_IF_CANCELED;
			
			mail_send_notify(EMF_SEND_CONNECTION_SUCCEED, 0, 0, account_id, mail_id, err);
			
			/*  reqest of delivery status. */
			if (opt && opt->req_delivery_receipt == EMF_OPTION_REQ_DELIVERY_RECEIPT_ON)  { 
				stream->protocol.esmtp.dsn.want = 1;
				stream->protocol.esmtp.dsn.full = 0;
				stream->protocol.esmtp.dsn.notify.failure = 1;
				stream->protocol.esmtp.dsn.notify.success = 1;
			}
			
			mail_send_notify(EMF_SEND_START, 0, 0, account_id, mail_id, err);
		}
		
		mail_table_data.save_status = EMF_MAIL_STATUS_SENDING;
		
		/*  update mail status to sending. */
		if (!em_storage_change_mail_field(mail_id, UPDATE_EXTRA_FLAG, &mail_table_data, true, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_change_mail_field failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		if (!em_core_mail_send_smtp(stream, envelope, fpath, account_id, mail_id, &err))  {
			EM_DEBUG_EXCEPTION("em_core_mail_send_smtp failed [%d]", err);
			
			mail_table_data.save_status = EMF_MAIL_STATUS_SEND_FAILURE;
			
			/*  update mail status to failure. */
			if (!em_storage_change_mail_field(mail_id, UPDATE_EXTRA_FLAG, &mail_table_data, true, &err)) 
				EM_DEBUG_EXCEPTION("em_storage_change_mail_field failed [%d]", err);
						
			if (!em_storage_get_mailboxname_by_mailbox_type(account_id, EMF_MAILBOX_TYPE_OUTBOX, &mailbox_name, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}	
			dest_mbox.name = mailbox_name;
			dest_mbox.account_id = account_id;
			
			em_core_mail_move(&mail_id, 1, dest_mbox.name, EMF_MOVED_AFTER_SENDING, 0, NULL);
			
			goto FINISH_OFF;
		}
		
		mail_table_data.save_status = EMF_MAIL_STATUS_SENT;
		
		/*  update mail status to sent mail. */
		if (!em_storage_change_mail_field(mail_id, UPDATE_EXTRA_FLAG, &mail_table_data, true, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_change_mail_field failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		/*  sent mail is moved to 'SENT' box or deleted. */
		if (opt->keep_local_copy)  {
			EM_SAFE_FREE(mailbox_name);
			if (!em_storage_get_mailboxname_by_mailbox_type(account_id, EMF_MAILBOX_TYPE_SENTBOX, &mailbox_name, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}	
			dest_mbox.name = mailbox_name;
			dest_mbox.account_id = account_id;
			
			if (!em_core_mail_move(&mail_id, 1, dest_mbox.name, EMF_MOVED_AFTER_SENDING, 0, &err)) 
				EM_DEBUG_EXCEPTION("em_core_mail_move falied [%d]", err);
		}
		else  {
			if (!em_core_mail_delete(account_id, &mail_id, 1, 0, EMF_DELETED_AFTER_SENDING, false, &err)) 
				EM_DEBUG_EXCEPTION("em_core_mail_delete falied [%d]", err);
		}
		
		em_core_mail_free(&mail, 1, NULL); mail = NULL;
		mail_free_envelope(&envelope); envelope = NULL;
		
		if (fpath)  {
			remove(fpath);
			EM_SAFE_FREE(fpath);
		}
	}
	
	
	ret = true;

FINISH_OFF: 
	if (stream) 
		smtp_close(stream);

	if (stls) 
		mail_parameters(NIL, SET_SSLSTART, (void  *)stls);

	if (envelope) 
		mail_free_envelope(&envelope);

	if (handle)  {
		if (!em_storage_mail_search_end(handle, true, &err))
			EM_DEBUG_EXCEPTION("em_storage_mail_search_end failed [%d]", err);
	}

	if (mail) 
		em_core_mail_free(&mail, 1, NULL);

	if (fpath)  {
		remove(fpath);
		EM_SAFE_FREE(fpath);
	}

	EM_SAFE_FREE(mailbox_name);

	if (ret == true) 
		mail_send_notify(EMF_SEND_FINISH, 0, 0, account_id, mail_id, err);
	else {
		mail_send_notify(status, 0, 0, account_id, mail_id, err);
		em_core_show_popup(account_id, EMF_ACTION_SEND_MAIL, err);
	}
	
	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}	

static int em_core_mail_send_smtp(SENDSTREAM *stream, ENVELOPE *env, char *data_file, int account_id, int mail_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], env[%p], data_file[%s], account_id[%d], mail_id[%d], err_code[%p]", stream, env, data_file, account_id, mail_id, err_code);
	EM_PROFILE_BEGIN(profile_em_core_mail_send_smtp);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int recipients = 0;
	long total = 0, sent = 0, send_ret = 0, send_err = 0, sent_percent = 0, last_sent_percent = 0;
	char buf[2048] = { 0, };
	emf_account_t *ref_account = NULL;
	FILE *fp = NULL;
	
	if (!env || !env->from || (!env->to && !env->cc && !env->bcc)) {
		if (env != NULL)
			EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!env->from->mailbox || !env->from->host) {
		EM_DEBUG_EXCEPTION("env->from->mailbox[%p], env->from->host[%p]", env->from->mailbox, env->from->host);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!(ref_account = em_core_get_account_reference(account_id))) {
		EM_DEBUG_EXCEPTION("em_core_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("Modifying - MAIL FROM ");
	if (ref_account->email_addr == NULL) {
		EM_DEBUG_LOG("ref_account->email_addr is null!!");
		SNPRINTF(buf, sizeof(buf), "FROM:<%s@%s>", env->from->mailbox, env->from->host);
	}
	else
		SNPRINTF(buf, sizeof(buf), "FROM:<%s>", ref_account->email_addr);
	
	/*  set DSN for ESMTP */
	if (stream->protocol.esmtp.ok) {
		if (stream->protocol.esmtp.eightbit.ok && stream->protocol.esmtp.eightbit.want)
			strncat (buf, " BODY=8BITMIME", sizeof(buf)-(strlen(buf)+1));
		
		if (stream->protocol.esmtp.dsn.ok && stream->protocol.esmtp.dsn.want) {
			EM_DEBUG_LOG("stream->protocol.esmtp.dsn.want is required");
			strncat (buf, stream->protocol.esmtp.dsn.full ? " RET=FULL" : " RET=HDRS", sizeof(buf)-strlen(buf)-1);
			if (stream->protocol.esmtp.dsn.envid) 
				SNPRINTF (buf + strlen (buf), sizeof(buf)-(strlen(buf)), " ENVID=%.100s", stream->protocol.esmtp.dsn.envid);
		}
		else
			EM_DEBUG_LOG("stream->protocol.esmtp.dsn.want is not required");
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
			EM_DEBUG_EXCEPTION("SMTP error :  authentication required...");
			err = EMF_ERROR_AUTHENTICATE;
			goto FINISH_OFF;
		
		case SMTP_RESPONSE_UNAVAIL:
			EM_DEBUG_EXCEPTION("SMTP error :  mailbox unavailable...");
			err = EMF_ERROR_MAILBOX_FAILURE;
			goto FINISH_OFF;
		
		default: 
			err = send_ret;
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
		err = EMF_ERROR_INVALID_ADDRESS;
	}
	
	if (!recipients) {
		EM_DEBUG_EXCEPTION("No valid recipients...");
		
		switch (stream->replycode) {
			case SMTP_RESPONSE_UNAVAIL: 
			case SMTP_RESPONSE_WANT_AUTH  : 
			case SMTP_RESPONSE_WANT_AUTH2: 
				err = EMF_ERROR_AUTH_REQUIRED;
				break;
			
			default: 
				err = EMF_ERROR_INVALID_ADDRESS;
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
			err = EMF_ERROR_SYSTEM_FAILURE;
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
		data = (char *)em_core_malloc(allocSize);		
		allocSize--;
		EM_PROFILE_END(profile_allocation);

		if (NULL == data) {
			err = EMF_ERROR_SMTP_SEND_FAILURE;
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
				err = EMF_ERROR_SMTP_SEND_FAILURE;
				goto FINISH_OFF;
			}
			sent += read_size;

			EM_DEBUG_LOG("before smtp_soutr_test");
			if (!(send_ret = smtp_soutr_test(stream->netstream, data))) {
				EM_SAFE_FREE(data);
				EM_DEBUG_EXCEPTION("Failed to send the data ");
				err = EMF_ERROR_SMTP_SEND_FAILURE;
				goto FINISH_OFF;
			}		
			else {
				sent_percent = (int) ((double)sent / (double)total * 100.0);
				if (last_sent_percent + 5 <= sent_percent) {
					if (!em_storage_notify_network_event(NOTI_SEND_START, account_id, NULL, mail_id, sent_percent))
						EM_DEBUG_EXCEPTION("em_storage_notify_network_event [NOTI_SEND_START] Failed >>>>");
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
			sent += strlen(buf);
			
			if (!(send_ret = smtp_soutr(stream->netstream, buf))) 
				break;
			/*  Sending Progress Notification */
			sent_percent = (int) ((double)sent / (double)total * 100.0);
			if (last_sent_percent + 5 <= sent_percent) {
				/* Disabled Temporary
				if (!em_storage_notify_network_event(NOTI_SEND_START, account_id, NULL, mail_id, sent_percent))
					EM_DEBUG_EXCEPTION(" em_storage_notify_network_event [NOTI_SEND_START] Failed >>>>");
				*/
				last_sent_percent = sent_percent;
			}
		}
		
#endif
		if (!send_ret) {
			EM_DEBUG_EXCEPTION("smtp_soutr failed - %ld", send_ret);
			err = EMF_ERROR_SMTP_SEND_FAILURE;
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
	
	if (err_code)
		*err_code = err;
	
	if (fp)
		fclose(fp); 
	EM_PROFILE_END(profile_em_core_mail_send_smtp);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/* ------ rfc822 handle --------------------------------------------------- */
#define RANDOM_NUMBER_LENGTH 35

char *em_core_generate_content_id_string(const char *hostname, int *err)
{
	EM_DEBUG_FUNC_BEGIN("hostname[%p]", hostname);

	if (!hostname) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		if (err)
			*err = EMF_ERROR_INVALID_PARAM;
		return NULL;
	}
	
	int cid_length = RANDOM_NUMBER_LENGTH + strlen(hostname) + 2, random_number_1, random_number_2, random_number_3, random_number_4;
	char *cid_string = NULL;

	cid_string = malloc(cid_length);

	if (!cid_string) {
		if (err)
			*err = EMF_ERROR_OUT_OF_MEMORY;
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
		*err = EMF_ERROR_NONE;

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
	int        error = EMF_ERROR_NONE;
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
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		error = EMF_ERROR_INVALID_PARAM;
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
		error = EMF_ERROR_OUT_OF_MEMORY;
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
		source_text.size = strlen(filename);

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

		if(!em_core_encode_base64(encoded_file_name, strlen(encoded_file_name), &base64_file_name, (unsigned long*)&base64_file_name_length, &error)) {
			EM_DEBUG_EXCEPTION("em_core_encode_base64 failed. error [%d]", error);
			goto FINISH_OFF;
		}
		
		result_file_name = em_core_replace_string(base64_file_name, "\015\012", "");
		
		EM_DEBUG_LOG("base64_file_name_length [%d]", base64_file_name_length);
		
		if(result_file_name) {
			EM_SAFE_FREE(encoded_file_name);
			encoded_file_name = em_core_malloc(strlen(result_file_name) + 15);
			if(!encoded_file_name) {
				EM_DEBUG_EXCEPTION("em_core_malloc failed.");
				goto FINISH_OFF;
			}
			snprintf(encoded_file_name, strlen(result_file_name) + 15, "=?UTF-8?B?%s?=", result_file_name);
			EM_DEBUG_LOG("encoded_file_name [%s]", encoded_file_name);
		}

		extension = em_core_get_extension_from_file_path(filename, NULL);

		part->body.type = em_core_get_content_type(extension, NULL);
		if(part->body.type == TYPEIMAGE)
			part->body.subtype = strdup(extension);
		else
			part->body.subtype = cpystr("octet-stream");

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
			error = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		param->attribute          = cpystr("name");
		param->value              = cpystr(encoded_file_name);
		param->next               = NULL;
		last_param                = param;
		last_part->body.parameter = last_param;

		if (is_inline) {	
			/*  CONTENT-ID */
			part->body.id = em_core_generate_content_id_string("com.samsung.slp.email", &error);
			part->body.type = TYPEIMAGE;
			/*  EM_SAFE_FREE(part->body.subtype); */
			/*  part->body.subtype = EM_SAFE_STRDUP(content_sub_type); */
		}

		/*  DISPOSITION PARAMETER */
		param = mail_newbody_parameter();
		if (param == NULL)  {
			EM_DEBUG_EXCEPTION("mail_newbody_parameter failed...");
			error = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		param->attribute                      = cpystr("filename");
		param->value                          = cpystr(encoded_file_name);
		param->next                           = NULL;
		last_param                            = param;
		last_part->body.disposition.parameter = last_param;
		
		if (is_inline)
			last_part->body.disposition.type = EM_SAFE_STRDUP("inline");
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
				error = EMF_ERROR_OUT_OF_MEMORY;
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
				error = EMF_ERROR_OUT_OF_MEMORY;
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
	EM_SAFE_FREE(base64_file_name);	
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

static PART *attach_mutipart_with_sub_type(BODY *parent_body, char *sub_type, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("parent_body[%p], sub_type [%s], err_code[%p]", parent_body, sub_type, err_code);
	
	int error = EMF_ERROR_NONE;
	
	PART *tail_part_cur = NULL;
	PART *new_part = NULL;
	
	if (!parent_body || !sub_type)  {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		error = EMF_ERROR_INVALID_PARAM;
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
		EM_DEBUG_EXCEPTION("EMF_ERROR_OUT_OF_MEMORY");
		error = EMF_ERROR_OUT_OF_MEMORY;
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

static int attach_attachment_to_body(BODY **multipart_body, BODY *text_body, emf_attachment_info_t *atch, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("multipart_body[%p], text_body[%p], atch[%p], err_code[%p]", multipart_body, text_body, atch, err_code);
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	BODY *frame_body = NULL;
	/*  make multipart body(multipart frame_body..) .. that has not content..  */
	
	if (!multipart_body || !text_body || !atch) {
		EM_DEBUG_EXCEPTION(" multipart_body[%p], text_body[%p], atch[%p]", multipart_body, text_body, atch);
		error = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	frame_body = mail_newbody();
	if (frame_body == NULL) {
		EM_DEBUG_EXCEPTION("mail_newbody failed...");
		error = EMF_ERROR_OUT_OF_MEMORY;
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
	emf_attachment_info_t *p = atch;
	char *name = NULL;
	struct stat st_buf;
	
	while (p)  {
		EM_DEBUG_LOG("insert files - attachment id[%d]", p->attachment_id);
		if (stat(p->savename, &st_buf) == 0)  {
			if (!p->name)  {
				if (!em_core_get_file_name(p->savename, &name, &error))  {
					EM_DEBUG_EXCEPTION("em_core_get_file_name failed [%d]", error);
					goto FINISH_OFF;
				}
			}
			else 
				name = p->name;
			
			if (!attach_part(frame_body, (unsigned char *)p->savename, 0, name, NULL, false, &error))  {
				EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
				goto FINISH_OFF;
			}
		}
		
		p = p->next;
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

static char *em_core_encode_rfc2047_text(char *utf8_text, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("utf8_text[%s], err_code[%p]", utf8_text, err_code);
	
	if (utf8_text == NULL)  {
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return NULL;
	}
	
	gsize len = strlen(utf8_text);
	
	EM_DEBUG_FUNC_END();

	if (len > 0)
		return g_strdup_printf("=?UTF-8?B?%s?=", g_base64_encode((const guchar  *)utf8_text, len));
	else
		return EM_SAFE_STRDUP("");
}

static void em_core_encode_rfc2047_address(ADDRESS *address, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("address[%p], err_code[%p]", address, err_code);
	
	while (address)  {
		if (address->personal)  {
			char *rfc2047_personal = em_core_encode_rfc2047_text(address->personal, err_code);
			EM_SAFE_FREE(address->personal);
			address->personal = rfc2047_personal;
		}
		address = address->next;
	}
	EM_DEBUG_FUNC_END();
}

#define DATE_STR_LENGTH 100
/*  Description :  send mail to network(and save to sent-mailbox) or draft-mailbox, */
/*  Parameters :  */
/* 			mail :   */
/* 			is_draft  :  this mail is draft mail. */
/* 			file_path :  path of file that rfc822 data will be written to. */
EXPORT_API int em_core_mail_get_rfc822(emf_mail_t *mail, ENVELOPE **env, char **file_path, emf_option_t *sending_option, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail[%p], env[%p], file_path[%p], sending_option[%p], err_code[%p]", mail, env, file_path, sending_option, err_code);
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	
	ENVELOPE *envelope = NULL;
	BODY *text_body = NULL, *html_body = NULL;
	BODY *root_body = NULL;
	PART *part_for_html = NULL, *part_for_text = NULL;
	emf_extra_flag_t extra_flag;
	char *fname = NULL;
	int is_incomplete = 0;
	emf_account_t *ref_account = NULL;
	char *pAdd = NULL;
	
	if (!mail || !mail->info || !mail->head)  {
		if (mail != NULL)
			EM_DEBUG_EXCEPTION("mail->info[%p], mail->head[%p]", mail->info, mail->head);
		
		error = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (mail->info->extra_flags.report != EMF_MAIL_REPORT_MDN && !mail->body) {
		EM_DEBUG_EXCEPTION("mail->body[%p]", mail->body);
		error = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	
	ref_account = em_core_get_account_reference(mail->info->account_id);
	if (!ref_account)  {	
		EM_DEBUG_EXCEPTION("em_core_get_account_reference failed [%d]", mail->info->account_id);
		error = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	if (!(envelope = mail_newenvelope()))  {
		EM_DEBUG_EXCEPTION("mail_newenvelope failed...");
		error = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	is_incomplete = mail->info->flags.draft || (mail->info->extra_flags.status == EMF_MAIL_STATUS_SENDING);/* 4 */
	
	if (is_incomplete)  {
		if (ref_account->email_addr && ref_account->email_addr[0] != '\0')  {
			char *p = cpystr(ref_account->email_addr);
			
			if (p == NULL)  {
				EM_DEBUG_EXCEPTION("cpystr failed...");
				error = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			EM_DEBUG_LOG("Assign envelop->from");
			if (mail->head && mail->head->from) {
				char *pAdd = NULL ;
				em_core_skip_whitespace(mail->head->from , &pAdd);
				EM_DEBUG_LOG("address[pAdd][%s]", pAdd);

				rfc822_parse_adrlist(&envelope->from, pAdd, ref_account->sending_server_addr);
				EM_SAFE_FREE(pAdd);
				pAdd = NULL ;
			}
			else
				envelope->from = rfc822_parse_mailbox(&p, NULL);


			EM_SAFE_FREE(p);		
			if (!envelope->from)  {
				EM_DEBUG_EXCEPTION("rfc822_parse_mailbox failed...");
				
				error = EMF_ERROR_INVALID_ADDRESS;
				goto FINISH_OFF;		
			}
			else  {

				if (envelope->from->personal == NULL) {
					if (sending_option && sending_option->display_name_from && sending_option->display_name_from[0] != '\0') 
					envelope->from->personal = cpystr(sending_option->display_name_from);
				else
					envelope->from->personal = (ref_account->display_name && ref_account->display_name[0] != '\0') ? cpystr(ref_account->display_name)  :  NULL;
			}
		}
	}
		
	if (ref_account->return_addr && ref_account->return_addr[0] != '\0')  {
		char *p = cpystr(ref_account->return_addr); 
		
		if (p == NULL)  {
			EM_DEBUG_EXCEPTION("cpystr failed...");
			
			error = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		envelope->return_path = rfc822_parse_mailbox(&p, NULL);
		EM_SAFE_FREE(p);
	}
	}
	else  {
		if (!mail->head->from || !mail->head->to)  {
			EM_DEBUG_EXCEPTION("mail->head->from[%p], mail->head->to[%p]", mail->head->from, mail->head->to);
			error = EMF_ERROR_INVALID_MAIL;
			goto FINISH_OFF;
		}
		
		int i, j;
		
		if (mail->head->from)  {
			for (i = 0, j = strlen(mail->head->from); i < j; i++)  {
				if (mail->head->from[i] == ';')
					mail->head->from[i] = ',';
			}
		}
		
		if (mail->head->return_path)  {
			for (i = 0, j = strlen(mail->head->return_path); i < j; i++)  {
				if (mail->head->return_path[i] == ';')
					mail->head->return_path[i] = ',';
			}
		}
			em_core_skip_whitespace(mail->head->from , &pAdd);
		EM_DEBUG_LOG("address[pAdd][%s]", pAdd);
	
		rfc822_parse_adrlist(&envelope->from, pAdd, ref_account->sending_server_addr);
		EM_SAFE_FREE(pAdd);
			pAdd = NULL ;

		em_core_skip_whitespace(mail->head->return_path , &pAdd);
		EM_DEBUG_LOG("address[pAdd][%s]", pAdd);
	
		rfc822_parse_adrlist(&envelope->return_path, pAdd, ref_account->sending_server_addr);
		EM_SAFE_FREE(pAdd);
			pAdd = NULL ;
	}

	{
		int i, j;
		
		if (mail->head->to)  {
			for (i = 0, j = strlen(mail->head->to); i < j; i++)  {
				if (mail->head->to[i] == ';')
					mail->head->to[i] = ',';
			}
		}
		
		if (mail->head->cc)  {
			for (i = 0, j = strlen(mail->head->cc); i < j; i++)  {
				if (mail->head->cc[i] == ';')
					mail->head->cc[i] = ',';
			}
		}
		
		if (mail->head->bcc)  {
			for (i = 0, j = strlen(mail->head->bcc); i < j; i++)  {
				if (mail->head->bcc[i] == ';')
					mail->head->bcc[i] = ',';
			}
		}
	}

	em_core_skip_whitespace(mail->head->to , &pAdd);
	EM_DEBUG_LOG("address[pAdd][%s]", pAdd);
	
	rfc822_parse_adrlist(&envelope->to, pAdd, ref_account->sending_server_addr);
	EM_SAFE_FREE(pAdd);
	pAdd = NULL ;
	
	EM_DEBUG_LOG("address[mail->head->cc][%s]", mail->head->cc);
	em_core_skip_whitespace(mail->head->cc , &pAdd);
	EM_DEBUG_LOG("address[pAdd][%s]", pAdd);
	
	rfc822_parse_adrlist(&envelope->cc, pAdd, ref_account->sending_server_addr);
	EM_SAFE_FREE(pAdd);
		pAdd = NULL ;

	em_core_skip_whitespace(mail->head->bcc , &pAdd);
	rfc822_parse_adrlist(&envelope->bcc, pAdd, ref_account->sending_server_addr);
	EM_SAFE_FREE(pAdd);
		pAdd = NULL ;

	em_core_encode_rfc2047_address(envelope->return_path, &error);
	em_core_encode_rfc2047_address(envelope->from, &error);
	em_core_encode_rfc2047_address(envelope->sender, &error);
	em_core_encode_rfc2047_address(envelope->reply_to, &error);
	em_core_encode_rfc2047_address(envelope->to, &error);
	em_core_encode_rfc2047_address(envelope->cc, &error);
	em_core_encode_rfc2047_address(envelope->bcc, &error);

	if (mail->head->subject)
		envelope->subject = em_core_encode_rfc2047_text(mail->head->subject, &error);

	char date_str[DATE_STR_LENGTH + 1] = { 0, };
	
	rfc822_date(date_str);

	if (!is_incomplete)  {
		struct tm tm1;
		
		/*  modified by stonyroot - prevent issue */
		memset(&tm1,  0x00, sizeof(tm1));
		
		tm1.tm_year = mail->head->datetime.year - 1900;
		tm1.tm_mon  = mail->head->datetime.month - 1;
		tm1.tm_mday = mail->head->datetime.day;
		tm1.tm_hour = mail->head->datetime.hour;
		tm1.tm_min  = mail->head->datetime.minute;
		tm1.tm_sec  = mail->head->datetime.second;
		
		/* tzset(); */
		time_t t = mktime(&tm1);
		
		char buf[256] = {0, };
		
		if (localtime(&t))
			strftime(buf, 128, "%a, %e %b %Y %H : %M : %S ", localtime(&t));
		else
			strftime(buf, 128, "%a, %e %b %Y %H : %M : %S ", (const struct tm *)&tm1);	
		/*  append last 5byes("+0900") */
		strncat(buf, date_str + (strlen(date_str) -  5), DATE_STR_LENGTH);
		strncpy(date_str, buf, DATE_STR_LENGTH);
	}
	
	envelope->date = (unsigned char *)cpystr((const char *)date_str);
	
	memcpy(&extra_flag, &mail->info->extra_flags, sizeof(emf_extra_flag_t));
	
	/*  check report mail */
	if (mail->info->extra_flags.report != EMF_MAIL_REPORT_MDN)  {		
		/* Non-report mail */
		EM_DEBUG_LOG("mail->body->plain[%s]", mail->body->plain);
		EM_DEBUG_LOG("mail->body->html[%s]", mail->body->html);
		EM_DEBUG_LOG("mail->body->attachment_num[%d]", mail->body->attachment_num);
		
		
		if ((mail->body->attachment_num > 0) || (mail->body->plain && mail->body->html))  {
			EM_DEBUG_LOG("attachment_num  :  %d", mail->body->attachment_num);
			root_body = mail_newbody();

			if (root_body == NULL)  {
				EM_DEBUG_EXCEPTION("mail_newbody failed...");
				error = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			root_body->type = TYPEMULTIPART;
			root_body->subtype = EM_SAFE_STRDUP("MIXED");
			root_body->contents.text.data = NULL;
			root_body->contents.text.size = 0;
			root_body->size.bytes = 0;

			part_for_text = attach_mutipart_with_sub_type(root_body, "ALTERNATIVE", &error);

			if (!part_for_text) {
				EM_DEBUG_EXCEPTION("attach_mutipart_with_sub_type [part_for_text] failed [%d]", error);
				goto FINISH_OFF;
			}

			text_body = &part_for_text->body;
			
			if (mail->body->plain && strlen(mail->body->plain) > 0)  {
				EM_DEBUG_LOG("body->plain[%s]", mail->body->plain);
				if (!attach_part(text_body, (unsigned char *)mail->body->plain, 0, NULL, NULL, false, &error))  {
					EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
					goto FINISH_OFF;
				}
			}
			
			if (mail->body->html && strlen(mail->body->html) > 0)  {
				EM_DEBUG_LOG("body->html[%s]", mail->body->html);

				part_for_html = attach_mutipart_with_sub_type(text_body, "RELATED", &error);
				if (!part_for_html) {
					EM_DEBUG_EXCEPTION("attach_mutipart_with_sub_type [part_for_html] failed [%d]", error);
					goto FINISH_OFF;
				}

				if (!attach_part(&(part_for_html->body) , (unsigned char *)mail->body->html, 0, NULL, "html", false, &error))  {
					EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
					goto FINISH_OFF;
				}
				/*
				if (mail->body->plain)  {
					EM_SAFE_FREE(part_for_text->subtype);
					part_for_text->subtype = EM_SAFE_STRDUP("ALTERNATIVE");
				}
				*/
			}
			
			if (mail->body->attachment)  {
				emf_attachment_info_t *atch = mail->body->attachment;
				char *name = NULL;
				BODY *body_to_attach = NULL; 
				struct stat st_buf;
				
				do  {
					EM_DEBUG_LOG("atch->savename[%s], atch->name[%s]", atch->savename, atch->name);
					if (stat(atch->savename, &st_buf) == 0)  {
						EM_DEBUG_LOG("atch->name[%s]", atch->name);
						if (!atch->name)  {
							if (!em_core_get_file_name(atch->savename, &name, &error))  {
								EM_DEBUG_EXCEPTION("em_core_get_file_name failed [%d]", error);
								goto TRY_NEXT;
							}
						}
						else 
							name = atch->name;
						EM_DEBUG_LOG("name[%s]", name);

						if (atch->inline_content && part_for_html)
							body_to_attach = &(part_for_html->body);
						else
							body_to_attach = root_body;
						
						if (!attach_part(body_to_attach, (unsigned char *)atch->savename, 0, name, NULL, atch->inline_content, &error))  {
							EM_DEBUG_EXCEPTION("attach_part failed [%d]", error);
							goto TRY_NEXT;
						}
					}
					
TRY_NEXT:
					atch = atch->next;
				}
				while (atch);
			}
			text_body = NULL; 
		}
		else  {
			text_body = mail_newbody();
			
			if (text_body == NULL)  {
				EM_DEBUG_EXCEPTION("mail_newbody failed...");
				
				error = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			text_body->type = TYPETEXT;
			text_body->encoding = ENC8BIT;
			if (mail->body->plain || mail->body->html)
				text_body->sparep = EM_SAFE_STRDUP(mail->body->plain ? mail->body->plain  :  mail->body->html);
			else
				text_body->sparep = NULL;
			
			if (mail->body->html != NULL && mail->body->html[0] != '\0')
				text_body->subtype = EM_SAFE_STRDUP("html");
			if (text_body->sparep)
				text_body->size.bytes = strlen(text_body->sparep);
			else
				text_body->size.bytes = 0;
		}
	}
	else  {	/*  Report mail */
		EM_DEBUG_LOG("REPORT MAIL");
		envelope->references = cpystr(mail->head->mid);
		
		if (em_core_mail_get_report_body(envelope, &root_body, &error))  {
			if (!mail->body)  {
				mail->body = em_core_malloc(sizeof(emf_mail_body_t));
				if (!mail->body)  {
					EM_DEBUG_EXCEPTION("malloc failed...");
					error = EMF_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}
				
				mail->body->plain = EM_SAFE_STRDUP(root_body->nested.part->body.sparep);
				mail->body->attachment_num = 1;
				
				mail->body->attachment = em_core_malloc(sizeof(emf_attachment_info_t));
				if (!mail->body->attachment)  {
					EM_DEBUG_EXCEPTION("malloc failed...");
					EM_SAFE_FREE(mail->body->plain);
					error = EMF_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}
				
				mail->body->attachment->downloaded = 1;
				mail->body->attachment->savename = EM_SAFE_STRDUP(root_body->nested.part->next->body.sparep);
				
				char *p = NULL;
				
				if (!em_core_get_file_name(mail->body->attachment->savename, &p, &error))  {
					EM_DEBUG_EXCEPTION("em_core_get_file_name failed [%d]", error);
					goto FINISH_OFF;
				}
				
				mail->body->attachment->name = cpystr(p);
			}
		}
			
	}
	
	if (file_path)  {
		EM_DEBUG_LOG("write rfc822  :  file_path[%s]", file_path);

		if (part_for_html)
			html_body = &(part_for_html->body);

		if (!em_core_write_rfc822(envelope, root_body ? root_body  :  text_body, html_body, extra_flag, &fname, &error))  {
			EM_DEBUG_EXCEPTION("em_core_write_rfc822 failed [%d]", error);
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

static int em_core_mail_get_report_body(ENVELOPE *envelope, BODY **multipart_body, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("envelope[%p], mulitpart_body[%p], err_code[%p]", envelope, multipart_body, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	BODY *m_body = NULL;
	BODY *p_body = NULL;
	BODY *text_body = NULL;
	PARAMETER *param = NULL;
	emf_attachment_info_t atch;
	FILE *fp = NULL;
	char *fname = NULL;
	char buf[512] = {0x00, };
	int sz = 0;
	
	if (!envelope || !multipart_body)  {
		EM_DEBUG_EXCEPTION(" envelope[%p], mulitpart_body[%p]", envelope, multipart_body);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!(text_body = mail_newbody()))  {
		EM_DEBUG_EXCEPTION(" mail_newbody failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	if (!em_core_get_temp_file_name(&fname, &err))  {
		EM_DEBUG_EXCEPTION(" em_core_get_temp_file_name failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!(fp = fopen(fname, "wb+")))  {
		EM_DEBUG_EXCEPTION(" fopen failed - %s", fname);
		err = EMF_ERROR_SYSTEM_FAILURE;		/* EMF_ERROR_UNKNOWN; */
		goto FINISH_OFF;
	}
	
	if (!envelope->from || !envelope->from->mailbox || !envelope->from->host)  {
		if (!envelope->from)
			EM_DEBUG_EXCEPTION(" envelope->from[%p]", envelope->from);
		else
			EM_DEBUG_LOG(" envelope->from->mailbox[%p], envelope->from->host[%p]", envelope->from->mailbox, envelope->from->host);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (envelope->from->personal) 
		SNPRINTF(buf, sizeof(buf), "%s <%s@%s>", envelope->from->personal, envelope->from->mailbox, envelope->from->host);
	else 
		SNPRINTF(buf, sizeof(buf), "%s@%s", envelope->from->mailbox, envelope->from->host);
	
	fprintf(fp, "Your message has been read by %s"CRLF_STRING, buf);
	fprintf(fp, "Date :  %s", envelope->date);
	
	fclose(fp); fp = NULL;
	
	if (!em_core_get_file_size(fname, &sz, &err))  {
		EM_DEBUG_EXCEPTION(" em_core_get_file_size failed [%d]", err);
		goto FINISH_OFF;
	}
	
	text_body->type = TYPETEXT;
	text_body->encoding = ENC8BIT;
	text_body->sparep = fname;
	text_body->size.bytes = (unsigned long)sz;
	
	if (!em_core_get_temp_file_name(&fname, &err))  {
		EM_DEBUG_EXCEPTION(" em_core_get_temp_file_name failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!(fp = fopen(fname, "wb+")))  {
		EM_DEBUG_EXCEPTION(" fopen failed - %s", fname);
		err = EMF_ERROR_SYSTEM_FAILURE;		/* EMF_ERROR_UNKNOWN; */
		goto FINISH_OFF;
	}
	
	if (!envelope->references)  {
		EM_DEBUG_EXCEPTION(" envelope->references[%p]", envelope->references);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	fprintf(fp, "Final-Recipient :  rfc822;%s@%s\r", envelope->from->mailbox, envelope->from->host);
	fprintf(fp, "Original-Message-ID:  %s\r", envelope->references);
	fprintf(fp, "Disposition :  manual-action/MDN-sent-manually; displayed");
	
	fclose(fp); fp = NULL;
	
	memset(&atch, 0x00, sizeof(atch));
	
	atch.savename = fname;
	
	if (!em_core_get_file_size(fname, &atch.size, &err))  {
		EM_DEBUG_EXCEPTION(" em_core_get_file_size failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!attach_attachment_to_body(&m_body, text_body, &atch, &err))  {
		EM_DEBUG_EXCEPTION(" attach_attachment_to_body failed [%d]", err);
		goto FINISH_OFF;
	}
	
	text_body->contents.text.data = NULL;
	
	/*  change mail header */
	
	/*  set content-type to multipart/report */
	m_body->subtype = EM_SAFE_STRDUP("report");
	
	/*  set report-type parameter in content-type */
	param = em_core_malloc(sizeof(PARAMETER));
	if (!param)  {		
		EM_DEBUG_EXCEPTION(" malloc failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	param->attribute = EM_SAFE_STRDUP("report-type");
	param->value = EM_SAFE_STRDUP("disposition-notification");
	param->next = m_body->parameter;
	
	m_body->parameter = param;
	
	/*  change body-header */
	
	p_body = &m_body->nested.part->next->body;
	
	/*  set content-type to message/disposition-notification */
	p_body->type = TYPEMESSAGE;
	p_body->encoding = ENC7BIT;
	
	EM_SAFE_FREE(p_body->subtype);
	
	p_body->subtype = EM_SAFE_STRDUP("disposition-notification");
	
	/*  set parameter */
	mail_free_body_parameter(&p_body->parameter);
	mail_free_body_parameter(&p_body->disposition.parameter);
	
	EM_SAFE_FREE(p_body->disposition.type);
	
	p_body->disposition.type = EM_SAFE_STRDUP("inline");
	
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
	
	return ret;
}

EXPORT_API   int em_core_get_body_buff(char *file_path, char **buff)
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
  	stat(file_path, &stbuf); 
	EM_DEBUG_LOG(" File Size [ %d ] ", stbuf.st_size);
  	read_buff = calloc(1, (stbuf.st_size+ 1));
	read_size = fread(read_buff, 1, stbuf.st_size, r_fp);
	read_buff[stbuf.st_size] = '\0';

	if (ferror(r_fp)) {
		EM_DEBUG_EXCEPTION("file read failed - %s", file_path);
		EM_SAFE_FREE(read_buff);
		goto FINISH_OFF;
	}		


	ret = true;
	*buff = read_buff;
	
	FINISH_OFF: 
	if (r_fp)	/* Prevent Defect - 17424 */
		fclose(r_fp);

	return ret;
}

EXPORT_API int em_core_mail_get_envelope_body_struct(emf_mail_t *mail, 
											ENVELOPE **env, 
											BODY **text_body, 
											BODY **multipart_body, 
											int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail[%p], env[%p], text_body[%p], multipart_body[%p], err_code[%p]", mail, env, text_body, multipart_body, err_code);
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	ENVELOPE *envelope = NULL;
	BODY *txt_body = NULL;
	BODY *multi_part_body = NULL;
	emf_extra_flag_t extra_flag;
	int is_incomplete = 0;
	emf_account_t *ref_account = NULL;
	char *pAdd = NULL ;
	
	if (!mail || !mail->info || !mail->head || !mail->body) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		error = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	ref_account = em_core_get_account_reference(mail->info->account_id);
	
	if (!ref_account) {
		EM_DEBUG_EXCEPTION(" em_core_get_account_reference failed [%d]", mail->info->account_id);
		error = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!(envelope = mail_newenvelope())) {
		EM_DEBUG_EXCEPTION(" mail_newenvelope failed...");
		error = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG(" mail_newenvelope created...");
	is_incomplete = mail->info->flags.draft || mail->info->extra_flags.status == EMF_MAIL_STATUS_SENDING;/* 4; */
	if (is_incomplete) {
		if (ref_account->email_addr) {
			char *p = EM_SAFE_STRDUP(ref_account->email_addr);
			
			envelope->from = rfc822_parse_mailbox(&p, NULL);
			/* mailbox = user id; host = mail-server-addr; */
			if (!envelope->from)  {
				EM_DEBUG_EXCEPTION("rfc822_parse_mailbox failed...");
				error = EMF_ERROR_INVALID_ADDRESS;
				goto FINISH_OFF;
			}
			else  {
				envelope->from->personal = ref_account->display_name ? cpystr(ref_account->display_name)  :  NULL;
			}
		}
		
		if (ref_account->return_addr)  {
			char *p = EM_SAFE_STRDUP(ref_account->return_addr); 
			envelope->return_path = rfc822_parse_mailbox(&p, NULL);
		}
	}
	else  {
		if (!mail->head->from || !mail->head->to)  {
			EM_DEBUG_EXCEPTION(" mail->head->from[%p], mail->head->to[%p]", mail->head->from, mail->head->to);
			error = EMF_ERROR_INVALID_MAIL;
			goto FINISH_OFF;
		}

		em_core_skip_whitespace(mail->head->from , &pAdd);
		EM_DEBUG_LOG("address[pAdd][%s]", pAdd);
		
		rfc822_parse_adrlist(&envelope->from, pAdd, ref_account->sending_server_addr);
		EM_SAFE_FREE(pAdd);
			pAdd = NULL ;

		em_core_skip_whitespace(mail->head->return_path , &pAdd);
		EM_DEBUG_LOG("address[pAdd][%s]", pAdd);
		rfc822_parse_adrlist(&envelope->return_path, pAdd, ref_account->sending_server_addr);
		EM_SAFE_FREE(pAdd);
			pAdd = NULL ;
	}
	
	{
		int i, j;
		
		if (mail->head->to) {
			for (i = 0, j = strlen(mail->head->to); i < j; i++) {
				if (mail->head->to[i] == ';')
					mail->head->to[i] = ',';
			}
		}
		
		if (mail->head->cc) {
			for (i = 0, j = strlen(mail->head->cc); i < j; i++) {
				if (mail->head->cc[i] == ';')
					mail->head->cc[i] = ',';
			}
		}
		
		if (mail->head->bcc) {
			for (i = 0, j = strlen(mail->head->bcc); i < j; i++) {
				if (mail->head->bcc[i] == ';')
					mail->head->bcc[i] = ',';
			}
		}
	}

	em_core_skip_whitespace(mail->head->to , &pAdd);
	EM_DEBUG_LOG("address[pAdd][%s]", pAdd);
		
	rfc822_parse_adrlist(&envelope->to, pAdd, ref_account->sending_server_addr);
	EM_SAFE_FREE(pAdd);
	pAdd = NULL ;
	
	em_core_skip_whitespace(mail->head->cc , &pAdd);

	rfc822_parse_adrlist(&envelope->cc, pAdd, ref_account->sending_server_addr);
	EM_SAFE_FREE(pAdd);
	pAdd = NULL ;
	
	em_core_skip_whitespace(mail->head->bcc , &pAdd);
	EM_DEBUG_LOG("address[pAdd][%s]", pAdd);
	rfc822_parse_adrlist(&envelope->bcc, pAdd, ref_account->sending_server_addr);
	EM_SAFE_FREE(pAdd);
		pAdd = NULL ;
	em_core_encode_rfc2047_address(envelope->return_path, &error);
	em_core_encode_rfc2047_address(envelope->from, &error);
	em_core_encode_rfc2047_address(envelope->sender, &error);
	em_core_encode_rfc2047_address(envelope->reply_to, &error);
	em_core_encode_rfc2047_address(envelope->to, &error);
	em_core_encode_rfc2047_address(envelope->cc, &error);
	em_core_encode_rfc2047_address(envelope->bcc, &error);


	if (mail->head->subject)
		envelope->subject = em_core_encode_rfc2047_text(mail->head->subject, &error);

	char date_str[DATE_STR_LENGTH] = { 0, };
	
	rfc822_date(date_str);

	if (!is_incomplete) {
		struct tm tm1;
		
		/*  modified by stonyroot - prevent issue */
		memset(&tm1,  0x00, sizeof(tm1));
		
		tm1.tm_year = mail->head->datetime.year - 1900;
		tm1.tm_mon = mail->head->datetime.month - 1;
		tm1.tm_mday = mail->head->datetime.day;
		tm1.tm_hour = mail->head->datetime.hour;
		tm1.tm_min = mail->head->datetime.minute;
		tm1.tm_sec = mail->head->datetime.second;
		
		/* tzset(); */
		time_t t = mktime(&tm1);
		
		char buf[256] = {0x00, };
		
		if (localtime(&t))
			strftime(buf, 128, "%a, %e %b %Y %H : %M : %S ", localtime(&t));
		else
			strftime(buf, 128, "%a, %e %b %Y %H : %M : %S ", (const struct tm *)&tm1);	

		/*  append last 5byes("+0900") */
		strncat(buf, date_str + (strlen(date_str) -  5), sizeof(buf) - 1);
		strncpy(date_str, buf, DATE_STR_LENGTH - 1);
	}
	
	envelope->date = (unsigned char *)cpystr((const char *)date_str);
	
	memcpy(&extra_flag, &mail->info->extra_flags, sizeof(emf_extra_flag_t));
	
	/*  check report mail */
	if (mail->info->extra_flags.report != EMF_MAIL_REPORT_MDN)  {		
		txt_body = mail_newbody();
		if (txt_body == NULL) {
			EM_DEBUG_EXCEPTION(" mail_newbody failed...");
			
			error = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		txt_body->type = TYPETEXT;
		txt_body->encoding = ENC8BIT;
		if (mail->body && mail->body->plain) {
			if (mail->body->plain)
			txt_body->sparep = EM_SAFE_STRDUP(mail->body->plain);
			else
			txt_body->sparep = NULL;				
			
			txt_body->size.bytes = (int)strlen(mail->body->plain);
		}
		
		if (mail->body) {
			if (mail->body->attachment) {
				if (!attach_attachment_to_body(&multi_part_body, txt_body, mail->body->attachment, &error)) {
					EM_DEBUG_EXCEPTION("attach_attachment_to_body failed [%d]", error);
					goto FINISH_OFF;
				}
			}
		}
	}
	else  {		/*  Report mail */
		if (mail->head || mail->head->mid)
			envelope->references = cpystr(mail->head->mid);
		if (em_core_mail_get_report_body(envelope, &multi_part_body, &error)) {
			if (!mail->body) {
				mail->body = em_core_malloc(sizeof(emf_mail_body_t));
				if (!mail->body) {		
					EM_DEBUG_EXCEPTION("malloc failed...");
					
					error = EMF_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}
				
				mail->body->plain = EM_SAFE_STRDUP(multi_part_body->nested.part->body.sparep);
				mail->body->attachment_num = 1;
				
				mail->body->attachment = em_core_malloc(sizeof(emf_attachment_info_t));
				if (!mail->body->attachment)  {	
					EM_DEBUG_EXCEPTION("malloc failed...");
					
					EM_SAFE_FREE(mail->body->plain);
					error = EMF_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}
				
				mail->body->attachment->downloaded = 1;
				mail->body->attachment->savename = EM_SAFE_STRDUP(multi_part_body->nested.part->next->body.sparep);
				
				char *p = NULL;
				if (!em_core_get_file_name(mail->body->attachment->savename, &p, &error))  {
					EM_DEBUG_EXCEPTION("em_core_get_file_name failed [%d]", error);
					
					goto FINISH_OFF;
				}
				
				mail->body->attachment->name = cpystr(p);
			}
		}
	}
	ret = true;
	
FINISH_OFF: 
	if (ret == true) {
		if (env != NULL)
			*env = envelope;
		if (txt_body != NULL)
			*text_body = txt_body;
		if (multi_part_body != NULL)
			*multipart_body = multi_part_body;
	}
	else  {
		if (envelope != NULL)
   			mail_free_envelope(&envelope);
		if (txt_body != NULL)
			mail_free_body(&txt_body);
		if (multi_part_body != NULL)
			mail_free_body(&multi_part_body);
	}

	if (err_code != NULL)
		*err_code = error;
	return ret;
}
