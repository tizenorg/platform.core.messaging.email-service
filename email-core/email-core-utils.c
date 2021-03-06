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
 * File: email-core-utils.c
 * Desc: Mail Utils
 *
 * Auth:
 *
 * History:
 *	2006.08.16 : created
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <dlfcn.h>
#include <glib.h>
#include <glib-object.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <vconf.h>
#include <regex.h>
#include <pthread.h>

#include <contacts-svc.h>
#include <notification.h>

#include "email-types.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-debug-log.h"
#include "email-core-mail.h"
#include "email-core-event.h"
#include "email-core-mailbox.h"
#include "email-core-account.h" 
#include "email-core-mailbox-sync.h" 
#include "email-core-mime.h"
#include "email-core-sound.h" 
#include "email-utilities.h"
#include "email-convert.h"

#define LED_TIMEOUT_SECS          12 
#define G_DISPLAY_LENGTH          256

#define DIR_SEPERATOR_CH          '/'
#define EMAIL_CH_QUOT             '"'
#define EMAIL_CH_BRACKET_S        '<'
#define EMAIL_CH_BRACKET_E        '>'
#define EMAIL_CH_COMMA            ','
#define EMAIL_CH_SEMICOLON        ';'
#define EMAIL_CH_ROUND_BRACKET_S  '('
#define EMAIL_CH_ROUND_BRACKET_E  ')'
#define EMAIL_CH_SQUARE_BRACKET_S '['
#define EMAIL_CH_SQUARE_BRACKET_E ']'
#define EMAIL_CH_SPACE            ' '
#define EMAIL_NOTI_ICON_PATH      "/opt/data/email/res/image/Q02_Notification_email.png"

static char _g_display[G_DISPLAY_LENGTH];


typedef struct  _em_transaction_info_type_t {
	int mail_id;
	int	handle;	
	struct _em_transaction_info_type_t *next;

} em_transaction_info_type_t;

em_transaction_info_type_t  *g_transaction_info_list;

static email_option_t g_mail_option = 
{
	0, /* priority			*/
	1, /* keep_local_copy */
	0, /* req_delivery_receipt */
	0, /* req_read_receipt */
	0, /* download_limit */
	0, /* block_address */
	0, /* block_subject */
	NULL, /*  diplay name */
};

static email_session_t g_session_list[SESSION_MAX] = { {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0},};


typedef struct emcore_account_list_t emcore_account_list_t;
struct emcore_account_list_t {
	email_account_t *account;
	emcore_account_list_t *next;
};

static emcore_account_list_t **g_account_reference = NULL;

INTERNAL_FUNC char *emcore_convert_mutf7_to_utf8(char *mailbox_name)
{
	EM_DEBUG_FUNC_BEGIN();
	return (char *)(utf8_from_mutf7((unsigned char *)mailbox_name));
}

INTERNAL_FUNC int emcore_set_account_reference(emcore_account_list_t **account_list, int account_num, int *err_code)
{
	g_account_reference = (emcore_account_list_t **)account_list;
	return 1;
}

email_option_t *emcore_get_option(int *err_code)
{
	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	return &g_mail_option;
}

INTERNAL_FUNC int emcore_set_option(email_option_t *opt, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("opt[%p], err_code[%p]", opt, err_code);
	
	int err = EMAIL_ERROR_NONE;
	
	if (!opt) {
		EM_DEBUG_EXCEPTION("opt[%p]", opt);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	memset(_g_display, 0, G_DISPLAY_LENGTH);
	memcpy(&g_mail_option, opt, sizeof(g_mail_option));

	if (opt->display_name_from && opt->display_name_from[0] != '\0')  {
		strncpy(_g_display, opt->display_name_from, G_DISPLAY_LENGTH - 1);
		g_mail_option.display_name_from = _g_display;
	}
	else
		g_mail_option.display_name_from = NULL;
	
	if (err_code != NULL)
		*err_code = err;
	
	return true;
}

	



/*  in smtp case, path argument must be ENCODED_PATH_SMTP */
int emcore_get_long_encoded_path_with_account_info(email_account_t *account, char *path, int delimiter, char **long_enc_path, int *err_code)
{
	EM_PROFILE_BEGIN(emCorelongEncodedpath);
	EM_DEBUG_FUNC_BEGIN("account[%p], path[%s], delimiter[%d], long_enc_path[%p], err_code[%p]", account, path, delimiter, long_enc_path, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char *p = NULL;
	
	size_t long_enc_path_len = 0;
	
	if (path == NULL || (path && strncmp(path, ENCODED_PATH_SMTP, strlen(ENCODED_PATH_SMTP)) != 0)) {		/*  imap or pop3 */
		EM_DEBUG_LOG("account->incoming_server_address[%p]", account->incoming_server_address);
		EM_DEBUG_LOG("account->incoming_server_address[%s]", account->incoming_server_address);
		
		if (!account->incoming_server_address) {
			EM_DEBUG_EXCEPTION("account->incoming_server_address is null");
			error = EMAIL_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF;
		}
	
		long_enc_path_len = strlen(account->incoming_server_address) + (path ? strlen(path) : 0) + 64;
		
		*long_enc_path = em_malloc(long_enc_path_len);
		if (!*long_enc_path)  {
			EM_DEBUG_EXCEPTION("malloc failed...");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		p = *long_enc_path;
		
		/*  ex:"{mai.test.com:143/imap} or {mai.test.com:143/imap/tls}my-mailbox" */

		SNPRINTF(p, long_enc_path_len, "{%s:%d/%s/user=%d",
			account->incoming_server_address,
			account->incoming_server_port_number,
			account->incoming_server_type == EMAIL_SERVER_TYPE_POP3 ? "pop3" : "imap", account->account_id);
		
		if (account->incoming_server_secure_connection & 0x01)  {
			strncat(p, "/ssl", long_enc_path_len-(strlen(p)+1));
			/* strcat(p, "/tryssl"); */
		}

		/* Currently, receiving servers doesn't require tls. 
		if (account->incoming_server_secure_connection & 0x02)
			strncat(p, "/tls", long_enc_path_len-(strlen(p)+1));
		else
			strncat(p, "/notls", long_enc_path_len-(strlen(p)+1));
		*/

		if (account->incoming_server_requires_apop) {
			EM_DEBUG_LOG("emcore_get_long_encoded_path - incoming_server_requires_apop - %d", account->incoming_server_requires_apop);
			strncat(p, "/apop", long_enc_path_len-(strlen(p)+1));
			EM_DEBUG_LOG("long_enc_path - %s", p);
		}
	}
	else  {		/*  smtp */
		long_enc_path_len = strlen(account->outgoing_server_address) + 64;
		
		*long_enc_path = em_malloc(strlen(account->outgoing_server_address) + 64);
		if (!*long_enc_path) {
			EM_DEBUG_EXCEPTION("\t malloc failed...\n");
			
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		p = *long_enc_path;
		
		/*  ex:"mail.test.com:25/smtp" */

		SNPRINTF(p, long_enc_path_len, "%s:%d/%s",
			account->outgoing_server_address,
			account->outgoing_server_port_number,
			"smtp");
		
		if (account->outgoing_server_need_authentication) {
			SNPRINTF(p + strlen(p), long_enc_path_len-(strlen(p)), "/user=%d", account->account_id);
		}
		
		if (account->outgoing_server_secure_connection & 0x01) {
			strncat(p, "/ssl", long_enc_path_len-(strlen(p)+1));
			/* strcat(p, "/tryssl"); */
		}
		if (account->outgoing_server_secure_connection & 0x02)
			strncat(p, "/tls", long_enc_path_len-(strlen(p)+1));
		else
			strncat(p, "/notls", long_enc_path_len-(strlen(p)+1));
	}

	if (path == NULL || (path && strncmp(path, ENCODED_PATH_SMTP, strlen(ENCODED_PATH_SMTP)) != 0)) {
		strncat(p, "}", long_enc_path_len-(strlen(p)+1));
		
		if (path != NULL) {
			char *enc_name = NULL;
			
			if (!emcore_get_encoded_mailbox_name(path, &enc_name, &error))  {
				EM_DEBUG_EXCEPTION("emcore_get_encoded_mailbox_name failed - %d", error);
				*long_enc_path = NULL;
				goto FINISH_OFF;
			}
			
			if (enc_name)  {
				strncat(p, enc_name, long_enc_path_len-(strlen(p)+1));
				EM_SAFE_FREE(enc_name);
			}
		}
	}
	
	ret = true;
	
FINISH_OFF:
	if (ret != true)
		EM_SAFE_FREE(p);
	
	if (err_code != NULL)
		*err_code = error;
	EM_PROFILE_END(emCorelongEncodedpath);
	return ret;
}

int emcore_get_long_encoded_path(int account_id, char *path, int delimiter, char **long_enc_path, int *err_code)
{
	EM_PROFILE_BEGIN(emCorelongEncodedpath);
	EM_DEBUG_FUNC_BEGIN("account_id[%d], delimiter[%d], long_enc_path[%p], err_code[%p]", account_id, delimiter, long_enc_path, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	email_account_t *ref_account = emcore_get_account_reference(account_id);
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		error = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (emcore_get_long_encoded_path_with_account_info(ref_account, path, delimiter, long_enc_path, &error) == false) {
		EM_DEBUG_EXCEPTION("emcore_get_long_encoded_path_with_account_info failed [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;
	EM_PROFILE_END(emCorelongEncodedpath);
	return ret;
}

int emcore_get_encoded_mailbox_name(char *name, char **enc_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("name[%s], enc_name[%p], err_code[%p]", name, enc_name, err_code);
	
	if (!name || !enc_name)  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_FUNC_END();
		return false;
	}
	
	/* encoding mailbox name (Charset->UTF8->UTF7) */

	*enc_name = em_malloc(strlen(name)+1);
	if (*enc_name == NULL) {
		EM_DEBUG_EXCEPTION("malloc failed...");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_OUT_OF_MEMORY;
		EM_DEBUG_FUNC_END();
		return false;
	}
	
	strcpy(*enc_name, name);
	
	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	EM_DEBUG_FUNC_END();
	return true;
}

int emcore_get_temp_file_name(char **filename, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("filename[%p], err_code[%p]", filename, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	if (filename == NULL) {
		EM_DEBUG_EXCEPTION("\t filename[%p]\n", filename);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	char tempname[512] = {0x00, };
	struct timeval tv;
	

	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);

	/* Create Directory If deleted by user*/
	emstorage_create_dir_if_delete();
	
	SNPRINTF(tempname, sizeof(tempname), "%s%c%s%c%d", MAILHOME, DIR_SEPERATOR_CH, MAILTEMP, DIR_SEPERATOR_CH, rand());
	
	char *p = EM_SAFE_STRDUP(tempname);
	if (p == NULL) {
		EM_DEBUG_EXCEPTION("\t strdup failed...\n");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	*filename = p;
	
	ret = true;

FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

int emcore_get_file_name(char *path, char **filename, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("path[%s], filename[%p], err_code[%p]", path, filename, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	if (!path || !filename) {
		EM_DEBUG_EXCEPTION("path[%p], filename[%p]", path, filename);
		
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	int i = (int)strlen(path);
	
	/*  get filename */
	for (; i >= 0; i--)
		if (path[i] == DIR_SEPERATOR_CH)
			break;
	
	*filename = path + i + 1;
	
	ret = true;
	
FINISH_OFF:
		if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

int emcore_get_file_size(char *path, int *size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("path[%s], size[%p], err_code[%p]", path, size, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	if ((path == NULL) || (size == NULL)) {
		EM_DEBUG_EXCEPTION("\t path[%p], size[%p]\n", path, size);
		
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	struct stat st_buf;
	
	if (stat(path, &st_buf) < 0)  {
		EM_DEBUG_EXCEPTION("\t stat failed - %s\n", path);
		
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
	
	*size = st_buf.st_size;
	
	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}



static int _emcore_check_host(char *host)
{
	if (!host)
		return 0;
	return strncmp(host, ".SYNTAX-ERROR.", strlen(".SYNTAX-ERROR."));
}



int emcore_get_address_count(char *addr_str, int *count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("addr_str[%s], count[%p], err_code[%p]", addr_str, count, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	ADDRESS *addr = NULL;
	ADDRESS *p_addr = NULL;
	int i = 0, j;
	char *p = NULL;


	if (!count)  {
		EM_DEBUG_EXCEPTION("addr_str[%s], count[%p]", addr_str, count);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (addr_str != NULL)  {
		em_skip_whitespace(addr_str, &p);
		EM_DEBUG_LOG("em_skip_whitespace[p][%s]", p);


		for (i = 0, j = strlen(p); i < j; i++) 
			if (p[i] == ';') p[i] = ',';
		rfc822_parse_adrlist(&addr, p, NULL);
		EM_SAFE_FREE(p);

	
		for (p_addr = addr, i = 0; p_addr; p_addr = p_addr->next, i++)  {
			if (p_addr->mailbox && p_addr->host) {	
				if (!strncmp(p_addr->mailbox, "UNEXPECTED_DATA_AFTER_ADDRESS", strlen("UNEXPECTED_DATA_AFTER_ADDRESS")) || !strncmp(p_addr->mailbox, "INVALID_ADDRESS", strlen("INVALID_ADDRESS")) || !strncmp(p_addr->host, ".SYNTAX-ERROR.", strlen(".SYNTAX-ERROR."))) {
					EM_DEBUG_LOG("Invalid address ");
					continue;
				}
			}			
			if ((!p_addr->mailbox) || (_emcore_check_host(p_addr->host) == 0)) {
				EM_DEBUG_EXCEPTION("\t invalid address : mailbox[%s], host[%s]\n", p_addr->mailbox, p_addr->host);
				
				error = EMAIL_ERROR_INVALID_ADDRESS;
				/* goto FINISH_OFF; */
			}
		}
	}

	*count = i;
	if (error != EMAIL_ERROR_INVALID_ADDRESS)
	ret = true;
	
FINISH_OFF:
	if (addr) 
		mail_free_address(&addr);
	
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_set_network_error(int err_code)
{
	email_session_t *session = NULL;

	EM_DEBUG_FUNC_BEGIN();

	emcore_get_current_session(&session);

	if (!session)
		return false;

	session->network = err_code;
	EM_DEBUG_FUNC_END();
	return true;
}

int emcore_get_empty_session(email_session_t **session)
{
	EM_DEBUG_FUNC_BEGIN("session[%p]", session);
	
	/*  lock()... */
	
	int i;
	
	for (i = 0; i < SESSION_MAX; i++)  {
		if (!g_session_list[i].status)  {
			memset(g_session_list+i, 0x00, sizeof(email_session_t));
			g_session_list[i].tid = GPOINTER_TO_INT(THREAD_SELF());
			g_session_list[i].status = true;
			break;
		}
	}
	
	/*  unlock()... */
	
	if (session != NULL)
		*session = (i != SESSION_MAX) ? &g_session_list[i] : NULL;
	EM_DEBUG_FUNC_END();
	return (i != SESSION_MAX) ? true : false;
}

int emcore_clear_session(email_session_t *session)
{
	EM_DEBUG_FUNC_BEGIN();
	
	if (session)
		memset(session, 0x00, sizeof(email_session_t));
	EM_DEBUG_FUNC_END();
	return true;
}

int emcore_get_current_session(email_session_t **session)
{
	EM_DEBUG_FUNC_BEGIN("session[%p]", session);
	
	int i;
	
	for (i = 0; i < SESSION_MAX; i++)  {
		if (g_session_list[i].tid == GPOINTER_TO_INT(THREAD_SELF())) {
			if (session)
				*session = g_session_list + i;
			
			break;
		}
	}
	
	if (session)
		*session = (i != SESSION_MAX) ? g_session_list + i : NULL;
	EM_DEBUG_FUNC_END();
	return (i != SESSION_MAX) ? true : false;
}

int emcore_check_unread_mail()
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int total_unread_count = 0;
	int total_mail_count = 0;
	email_mailbox_t mailbox;
	
	memset(&mailbox, 0x00, sizeof(email_mailbox_t));

	/* ALL_ACCOUNT used, so not calling emstorage_get_mailbox_name_by_mailbox_type to get mailbox name */
	mailbox.account_id = ALL_ACCOUNT;
	mailbox.mailbox_name = NULL;
	
	if (!emcore_get_mail_count(&mailbox, &total_mail_count, &total_unread_count, &err))  {
		EM_DEBUG_EXCEPTION("emcore_get_mail_count failed [%d]", err);
		goto FINISH_OFF;
	}
	
	EM_DEBUG_LOG("total_unread_count [%d]", total_unread_count);
	
	/* temporarily disable : set unread count to badge */
	/*
	if ( vconf_set_int(VCONF_KEY_UNREAD_MAIL_COUNT, total_unread_count) != 0 ) {
		EM_DEBUG_EXCEPTION("vconf_set_int failed");
		err = EMAIL_ERROR_GCONF_FAILURE;
		goto FINISH_OFF;
	}

	*/
	ret = true;
FINISH_OFF:

	return ret;
}

static int emcore_add_notification_for_user_message(int account_id, int mail_id, char *title, char *content, time_t log_time)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = true;
	int err = EMAIL_ERROR_NONE;
	notification_h noti = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	emstorage_account_tbl_t *account_tbl = NULL;

	if (!emstorage_get_account_by_id(account_id, EMAIL_ACC_GET_OPT_ACCOUNT_NAME, &account_tbl, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	noti = notification_new(NOTIFICATION_TYPE_NOTI, account_id, mail_id);

	if(noti == NULL) {
		EM_DEBUG_EXCEPTION("notification_new failed");
		goto FINISH_OFF;
	}
	
	if( (noti_err = notification_set_time(noti, log_time)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_time failed [%d]", noti_err);
		goto FINISH_OFF;
	}

	if( (noti_err = notification_set_text_domain(noti, NATIVE_EMAIL_APPLICATION_PKG, "/opt/apps/org.tizen.email/res/locale/")) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_text_domain failed [%d]", noti_err);
		goto FINISH_OFF;
	}

	if( (noti_err = notification_set_title(noti, title, NULL)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_title failed [%d]", noti_err);
		goto FINISH_OFF;
	}

	if( (noti_err = notification_set_content(noti, content, NULL)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_content failed [%d]", noti_err);
		goto FINISH_OFF;
	}
	
	if ((noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_GROUP_TITLE, title, NULL, NOTIFICATION_VARIABLE_TYPE_NONE)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_text failed [%d]", noti_err);
		goto FINISH_OFF;
	}

	if ((noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_GROUP_CONTENT, content, NULL, NOTIFICATION_VARIABLE_TYPE_STRING, NOTIFICATION_COUNT_DISPLAY_TYPE_LEFT, NOTIFICATION_VARIABLE_TYPE_NONE)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_text failed [%d]", noti_err);
		goto FINISH_OFF;
	}

	if ((noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, EMAIL_NOTI_ICON_PATH)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_image failed [%d]", noti_err);
		goto FINISH_OFF;
	}

	if ((noti_err = notification_set_pkgname(noti, NATIVE_EMAIL_APPLICATION_PKG)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_pkgname failed [%d]", noti_err);
		goto FINISH_OFF;
	}		

	if( (noti_err = notification_set_application(noti, NATIVE_EMAIL_APPLICATION_PKG)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_application failed [%d]", noti_err);
		goto FINISH_OFF;
	}

	if( (noti_err = notification_insert(noti, NULL)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_insert failed [%d]", noti_err);
		goto FINISH_OFF;
	}

FINISH_OFF:
	if( (noti_err = notification_free(noti)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_free failed [%d]", noti_err);
	}

	if (noti_err != NOTIFICATION_ERROR_NONE)
		ret = false;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;	
}

INTERNAL_FUNC int emcore_show_user_message(int id, email_action_t action, int error)
{
	EM_DEBUG_FUNC_BEGIN("id[%d], action[%d], error[%d]", id, action, error);

	int ret = false;
	time_t log_time = 0;
	struct tm *log_time_tm;

	time(&log_time);
	log_time_tm = localtime(&log_time);
	log_time = mktime(log_time_tm);

	EM_DEBUG_LOG("sec[%d], min[%d], hour[%d], day[%d], month[%d], year[%d]" ,log_time_tm->tm_sec, log_time_tm->tm_min, log_time_tm->tm_hour, log_time_tm->tm_mday, log_time_tm->tm_mon, log_time_tm->tm_year);

	if (action == EMAIL_ACTION_SEND_MAIL && error != EMAIL_ERROR_CANCELLED) {
	/*  In case email is cancelled using cancel button in Outbox there is no need to show Cancel/Retry Pop up */
		emstorage_mail_tbl_t *mail_table_data = NULL;

		if (error == 0) /*  error 0 means 'this is not error' */
			return true;

		if (id <= 0) {
			EM_DEBUG_LOG("Invalid mail_id");
			return false;
		}
		
		if (!emstorage_get_mail_by_id(id, &mail_table_data, true, NULL)) {
			EM_DEBUG_LOG("Mail not found");
			return false;
		}

		if (!emcore_add_notification_for_user_message(mail_table_data->account_id, id, "Failed to send a mail.", mail_table_data->subject, log_time)) {
			EM_DEBUG_EXCEPTION("emcore_notification_set error");
			return false;
		}

		if (!emstorage_free_mail(&mail_table_data, 1, NULL))
			EM_DEBUG_EXCEPTION("emstorage_free_mail Failed");
		
		ret = true;
	}
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
	

/* storage space handling - 210709 */
int emcore_get_storage_status(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int storage_status = 0, nError = 0;
	
	g_type_init();

#ifdef STORAGE_STATUS	
	nError = vconf_get_int(PS_KEY_SYSTEM_STORAGE_MOVI_STATUS,
							&storage_status);	
#endif /*  STORAGE_STATUS */

	if (nError == -1) {
		EM_DEBUG_EXCEPTION("vconf_get_int Failed");
		return false;
	}
	EM_DEBUG_FUNC_END();
	return storage_status; 
}

int emcore_is_storage_full(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	struct statfs buf = {0}; 
	
	if (statfs(DATA_PATH, &buf) == -1) {
		EM_DEBUG_EXCEPTION("statfs(\"%s\") failed - %d", DATA_PATH, errno);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
	else  {
		long i_free = (buf.f_bfree * buf.f_bsize) / (1024 * 1024);
		EM_DEBUG_LOG("f_bfree[%d] f_bsize[%d]", buf.f_bfree, buf.f_bsize);
		EM_DEBUG_LOG("Free space of storage is[%ld] MB.", i_free);
		if (i_free < EMAIL_LIMITATION_FREE_SPACE)
			err = EMAIL_ERROR_MAIL_MEMORY_FULL;
	}
	
	if (err == EMAIL_ERROR_MAIL_MEMORY_FULL)
		ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret[%d]", ret);
	return ret;
}

int emcore_calc_mail_size(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, int *output_size)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list[%p], input_attachment_count[%d], output_size[%p]", input_mail_data, input_attachment_data_list, input_attachment_count, output_size);

	struct stat            st_buf;
	int                    mail_size = 0; /*  size of the plain text body and attachments */
	int                    err       = EMAIL_ERROR_NONE;
	int                    i         = 0;
	
	if (!input_mail_data || (input_attachment_count && !input_attachment_data_list) || (!input_attachment_count &&input_attachment_data_list) || !output_size)  {	
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (input_mail_data->file_path_plain != NULL) {
		if (stat(input_mail_data->file_path_plain, &st_buf) < 0)  {
			EM_DEBUG_EXCEPTION("input_mail_data->file_path_plain : stat(\"%s\") failed...", input_mail_data->file_path_plain);
			err = EMAIL_ERROR_INVALID_MAIL;
			goto FINISH_OFF;
		}
		
		mail_size += st_buf.st_size;

	}

	if (input_mail_data->file_path_html != NULL) {
		if (stat(input_mail_data->file_path_html, &st_buf) < 0) {
			EM_DEBUG_EXCEPTION("input_mail_data->file_path_html : stat(\"%s\") failed...", input_mail_data->file_path_html);
			err = EMAIL_ERROR_INVALID_MAIL;
			goto FINISH_OFF;
		}
		
		mail_size += st_buf.st_size;
	}
	
	for(i = 0; i < input_attachment_count; i++)  {
		if (stat(input_attachment_data_list[i].attachment_path, &st_buf) < 0)  {
			EM_DEBUG_EXCEPTION("stat(\"%s\") failed...", input_attachment_data_list[i].attachment_path);
			err = EMAIL_ERROR_INVALID_MAIL;
			goto FINISH_OFF;
		}
		mail_size += st_buf.st_size;
	}

	*output_size = mail_size;
	
FINISH_OFF:
	
	EM_DEBUG_FUNC_END("mail_size [%d]", mail_size);
	return err;
}


/* parse the Full mailbox Path and Get the Alias Name of the Mailbox */
char *emcore_get_alias_of_mailbox(const char *mailbox_path)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_IF_NULL_RETURN_VALUE(mailbox_path, NULL);

	guint index = 0;
	gchar **token_list = NULL;
	gchar *mailbox = NULL, *name = NULL;
	char *converted_name;
	
	
	mailbox = g_strdup(mailbox_path);
	token_list = g_strsplit_set(mailbox, "/", -1);
	
	if (mailbox)
		g_free(mailbox);

	while (token_list[index] != NULL)
		index++;

	name = g_strdup(token_list[index - 1]);
	g_strfreev(token_list);

	converted_name = emcore_convert_mutf7_to_utf8(name);
	
	if (name)
		g_free(name);

	EM_DEBUG_FUNC_END();
	return converted_name;
}


static int emcore_get_first_address(const char *full_address, char **alias, char **address)
{
	EM_DEBUG_FUNC_BEGIN();

	if (full_address == NULL || alias == NULL || address == NULL){
		EM_DEBUG_EXCEPTION("Invalid Param  :  full_address[%p], alias[%p], address[%p]", full_address, alias, address);
		return false;
	}

	char *s = NULL;
	char *alias_start = NULL;
	char *alias_end = NULL;
	char *alias_cursor = NULL;
	char *address_start = NULL;
	char *address_end = NULL;
	char *first_address = NULL;
	
	if (full_address){
		s = (char *)strchr((char *)full_address, ';');
		if (s == NULL)
			first_address = strdup(full_address);	/*  only one  address */
		else
			first_address = strndup(full_address, s - full_address);	/*  over two addresses */

		/*  get alias */
		*alias = NULL;
		if ((alias_start = (char *)strchr((char *)first_address, '\"'))){	
			alias_start++;
			alias_cursor = alias_start;
			while ((alias_cursor = (char *)strchr((char *)(alias_cursor), '\"'))){
				alias_end = alias_cursor;
				alias_cursor++;
				if (*alias_cursor == 0)
					break;
			}
			if (alias_end)	{	/*  there is "alias" */
				*alias = strndup(alias_start, alias_end - alias_start);	
				EM_DEBUG_LOG("alias [%s]", *alias);
			}
		}

		/*  get address */
		*address = NULL;
		if (alias_end == NULL)
			s = first_address;
		else
			s = alias_end+1;
		if ((address_start = (char *)strchr((char  *)s, '<'))){	
			address_start++;
			if ((address_end = (char *)strchr((char  *)address_start, '>')))
				*address = strndup(address_start, address_end - address_start);	/*  (alias) <(addr)>  ... */
			else
				*address = strdup(s);
		}
		else
	       *address = strdup(s);	/*  (addr) ; ...		 :  no alias */
	}

	EM_SAFE_FREE(first_address);
	EM_DEBUG_FUNC_END();	
	return true;
}		

void emcore_fill_address_information_of_mail_tbl(emstorage_mail_tbl_t *mail_data)
{
	EM_DEBUG_FUNC_BEGIN("mail_data [%p]", mail_data);

	char *first_alias   = NULL;
	char *first_address = NULL;
	char *recipient     = NULL;

	/*  sender alias & address */
	if (emcore_get_first_address(mail_data->full_address_from, &first_alias, &first_address) == true) {
		if (first_alias == NULL) {
			mail_data->alias_sender = EM_SAFE_STRDUP(first_address);
		}
		else {
			mail_data->alias_sender = first_alias;
			first_alias = NULL;
		}
		mail_data->email_address_sender = first_address;
		first_address = NULL;
	}

	/*  recipient alias & address */
	if (mail_data->full_address_to != NULL)
		recipient = mail_data->full_address_to;
	else if (mail_data->full_address_cc != NULL)
		recipient = mail_data->full_address_cc;
	else if (mail_data->full_address_bcc != NULL)
		recipient = mail_data->full_address_bcc;
	
	if (emcore_get_first_address(recipient, &first_alias, &first_address) == true) {
		if (first_alias == NULL)
			mail_data->alias_recipient = EM_SAFE_STRDUP(first_address);
		else
			mail_data->alias_recipient = first_alias;

		mail_data->email_address_recipient = first_address;
	}
	EM_DEBUG_FUNC_END();	
}


int emcore_get_preview_text_from_file(const char *input_plain_path, const char *input_html_path, int input_preview_buffer_length, char **output_preview_buffer)
{
	EM_DEBUG_FUNC_BEGIN("input_plain_path[%p], input_html_path[%p], input_preview_buffer_length [%d], output_preview_buffer[%p]", input_plain_path, input_html_path, input_preview_buffer_length, output_preview_buffer);
	
	int          err = EMAIL_ERROR_NONE;
	unsigned int byte_read = 0;
	unsigned int byte_written = 0;
	int          result_strlen = 0;
	int          local_preview_buffer_length = 0;
	char        *local_preview_text = NULL;
	char        *encoding_type = NULL;
	char        *utf8_encoded_string = NULL;
	FILE        *fp = NULL;
	GError      *glib_error = NULL;
	struct stat  st_buf;

	if (!output_preview_buffer) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	local_preview_buffer_length = input_preview_buffer_length * 2;

	if (input_html_path != NULL) {	
		/*	get preview text from html file */
		if( (err = em_get_encoding_type_from_file_path(input_html_path, &encoding_type)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_get_encoding_type_from_file_path failed [%s]", err);
			goto FINISH_OFF;
		}
		
		if (stat(input_html_path, &st_buf) < 0)  {
			EM_DEBUG_EXCEPTION("stat(\"%s\") failed...", input_html_path);
			err = EMAIL_ERROR_INVALID_MAIL;
			goto FINISH_OFF;
		}
		
		if (!(fp = fopen(input_html_path, "r")))	{
			EM_DEBUG_EXCEPTION("fopen failed [%s]", input_html_path);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		if (!(local_preview_text = (char*)em_malloc(sizeof(char) * (st_buf.st_size + 1)))) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		byte_read = fread(local_preview_text, sizeof(char), st_buf.st_size, fp);
		
		if (ferror(fp)) {
			EM_DEBUG_EXCEPTION("fread failed [%s]", input_plain_path);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		if ( (err = emcore_strip_HTML(local_preview_text)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_strip failed");
			goto FINISH_OFF;
		}

		result_strlen = EM_SAFE_STRLEN(local_preview_text);
	}

	if (local_preview_text == NULL && input_plain_path != NULL) {	
		/*  get preview text from plain text file */
		if( (err = em_get_encoding_type_from_file_path(input_plain_path, &encoding_type)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_get_encoding_type_from_file_path failed [%s]", err);
			goto FINISH_OFF;
		}
		
		if (!(fp = fopen(input_plain_path, "r")))  {
			EM_DEBUG_EXCEPTION("fopen failed [%s]", input_plain_path);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		if (!(local_preview_text = (char*)em_malloc(sizeof(char) * local_preview_buffer_length))) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			goto FINISH_OFF;
		}
		
		byte_read = fread(local_preview_text, sizeof(char), local_preview_buffer_length - 1, fp);
		
		if (ferror(fp)) {
			EM_DEBUG_EXCEPTION("fread failed [%s]", input_plain_path);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		reg_replace(local_preview_text, CR_STRING, "");
		reg_replace(local_preview_text, LF_STRING, "");
		reg_replace(local_preview_text, TAB_STRING, "");
			
		result_strlen = EM_SAFE_STRLEN(local_preview_text);
	}	
	

	if(local_preview_text) {
		if(encoding_type && strcasecmp(encoding_type, "UTF-8") != 0) {
			EM_DEBUG_LOG("encoding_type [%s]", encoding_type);
			utf8_encoded_string = (char*)g_convert (local_preview_text, -1, "UTF-8", encoding_type, &byte_read, &byte_written, &glib_error);

			if(utf8_encoded_string) {
				EM_SAFE_FREE(local_preview_text);
				local_preview_text = utf8_encoded_string;
			}
			else 
				EM_DEBUG_EXCEPTION("g_convert failed");
		}
		
		if (!(*output_preview_buffer = (char*)em_malloc(sizeof(char) * (result_strlen + 1)))) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		EM_SAFE_STRNCPY(*output_preview_buffer, local_preview_text, result_strlen);
		/* EM_DEBUG_LOG("local_preview_text[%s], byte_read[%d], result_strlen[%d]", local_preview_text, byte_read, result_strlen); */
	}

FINISH_OFF:

	EM_SAFE_FREE(local_preview_text);
	EM_SAFE_FREE(encoding_type);
	
	if (fp != NULL)
		fclose(fp);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_add_transaction_info(int mail_id, int handle , int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], handle[%d]", mail_id, handle);

	int ret = false;
	int err = EMAIL_ERROR_NONE ;
	em_transaction_info_type_t  *pTransinfo = NULL ;
	em_transaction_info_type_t	*pTemp = NULL;

	EM_DEBUG_LOG("g_transaction_info_list[%p]", g_transaction_info_list);
	pTransinfo = g_transaction_info_list ;
	
	if (!(pTemp = em_malloc(sizeof(em_transaction_info_type_t))))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	pTemp->mail_id = mail_id ;
	pTemp->handle = handle;
		
	if (!pTransinfo) {
		pTransinfo = pTemp ;
		g_transaction_info_list = pTransinfo ;
	}
	else {
		while (pTransinfo->next)
			pTransinfo = pTransinfo->next;
		pTransinfo->next = pTemp;
	}
	ret = true ;
	
FINISH_OFF:

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END("g_transaction_info_list[%p]", g_transaction_info_list);
	return ret;	
}

INTERNAL_FUNC int emcore_get_handle_by_mailId_from_transaction_info(int mail_id, int *pHandle)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], handle[%p]", mail_id, pHandle);

	int ret = false;
	em_transaction_info_type_t  *pTransinfo = NULL ;
	
	if (g_transaction_info_list == NULL) {
		EM_DEBUG_EXCEPTION("g_transaction_info_list NULL");
		return false;
	}
	pTransinfo = g_transaction_info_list;

	do {
		EM_DEBUG_LOG("pTransinfo->mail_id[%d]", pTransinfo->mail_id);
		if (pTransinfo->mail_id == mail_id) {
			*pHandle = pTransinfo->handle;
			ret = true;
			EM_DEBUG_LOG("*pHandle[%d]", *pHandle);
			break;
		}
		else
			pTransinfo = pTransinfo->next ;
	}while (pTransinfo);
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_delete_transaction_info_by_mailId(int mail_id )
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d]", mail_id);

	em_transaction_info_type_t  *pTransinfo ;
	em_transaction_info_type_t *pTemp = NULL;

	if (g_transaction_info_list == NULL) {
		EM_DEBUG_EXCEPTION("g_transaction_info_list NULL");
		return false;
	}
	pTransinfo = g_transaction_info_list;

	EM_DEBUG_LOG("pTransinfo[%p]", pTransinfo);

	do {
		EM_DEBUG_LOG("pTransinfo->mail_id[%d]", pTransinfo->mail_id);
		if (pTransinfo->mail_id == mail_id) {
			pTemp = pTransinfo->next ;
			if (!pTemp) {
				EM_SAFE_FREE(pTransinfo) ;
				g_transaction_info_list = NULL;
			}
			else {
				pTransinfo->mail_id = pTransinfo->next->mail_id;
				pTransinfo->handle = pTransinfo->next->handle ;
				pTransinfo->next = pTransinfo->next->next;

				EM_SAFE_FREE(pTemp);
			}
			break;
		}
		else {
			pTransinfo = pTransinfo->next ;
		}	

	}while (pTransinfo);
	EM_DEBUG_FUNC_END();
	return true;
}


#include <regex.h>

int reg_replace (char *input_source_text, char *input_old_pattern_string, char *input_new_string)
{
	EM_DEBUG_FUNC_BEGIN("input_source_text [%p], input_old_pattern_string [%p], input_new_string [%p]", input_source_text, input_old_pattern_string, input_new_string);
	int         error_code = EMAIL_ERROR_NONE; 
	char       *pos = NULL;
	int         so, n, nmatch, source_text_length, n_count = 1;
	regmatch_t *pmatch = NULL; 
	regex_t     reg_pattern;

	if(!input_source_text || !input_old_pattern_string || !input_new_string) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;		
	}

	source_text_length = strlen(input_source_text);

	regcomp(&reg_pattern, input_old_pattern_string, REG_ICASE);

	nmatch = reg_pattern.re_nsub + 1;

	EM_DEBUG_LOG("nmatch [%d]", nmatch);

	if(nmatch < 1) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_DATA");
		error_code = EMAIL_ERROR_INVALID_DATA;
		goto FINISH_OFF;		
	}
	
	pmatch = (regmatch_t*)em_malloc(sizeof(regmatch_t) * nmatch);

	if(pmatch == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		error_code = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;		
	}
	
	for (pos = input_new_string; *pos ; pos++) {
		if (*pos == '\\' && *(pos + 1) > '0' && *(pos + 1) <= '9') {
		
			so = pmatch[*(pos + 1) - 48].rm_so;
			n  = pmatch[*(pos + 1) - 48].rm_eo - so;

			EM_DEBUG_LOG("so [%d], n [%d]", so, n);
			
			if (so < 0 || strlen (input_new_string) + n - 1 > source_text_length) 
				break;

			memmove (pos + n, pos + 2, strlen (pos) - 1);
			memmove (pos, input_source_text + so, n);
			pos = pos + n - 2;
		}
	}
	
	for (pos = input_source_text; !regexec (&reg_pattern, pos, 1, pmatch, 0);) {
		n = pmatch[0].rm_eo - pmatch[0].rm_so;
		pos += pmatch[0].rm_so;

		memmove (pos + strlen (input_new_string), pos + n, strlen (pos) - n + 1);
		memmove (pos, input_new_string, strlen (input_new_string));
		pos += strlen (input_new_string);
		n_count++;
	}
	
FINISH_OFF:

	EM_SAFE_FREE(pmatch);
	regfree (&reg_pattern);
	
	EM_DEBUG_FUNC_END("error_code [%d]", error_code);
	return error_code;
}


int emcore_strip_HTML(char *source_string)
{
	EM_DEBUG_FUNC_BEGIN("source_string [%p]", source_string);

	int result = EMAIL_ERROR_NONE;
	
	reg_replace(source_string, CR_STRING, " ");
	reg_replace(source_string, LF_STRING, " ");
	reg_replace(source_string, TAB_STRING, " ");
	reg_replace(source_string, "<head[^>]*>", "<head>"); /*  "<()*head([^>])*>", "<head>" */
	reg_replace(source_string, "<*/head>", "</head>");  /*  "(<()*(/)()*head()*>)", "</head>" */
	reg_replace(source_string, "<head>.*</head>", ""); /*  "(<head>).*(</head>)", "" */

	reg_replace(source_string, "<*/p>", " ");
	reg_replace(source_string, "<br>", " ");

	/*   "<[^>]*>", " */
	reg_replace(source_string, "<[^>]*>", "");


	/*   "&bull;", " *  */
	/* reg_replace(source_string, "&bull;", " * "); */

	/*   "&lsaquo;", "< */
	/* reg_replace(source_string, "&lsaquo;", "<"); */

	/*   "&rsaquo;", "> */
	/* reg_replace(source_string, "&rsaquo;", ">"); */

	/*   "&trade;", "(tm) */
	/* reg_replace(source_string, "&trade;", "(tm)"); */

	/*   "&frasl;", "/ */
	/* reg_replace(source_string, "&frasl;", "/"); */

	/*  "&lt;", "< */
	reg_replace(source_string, "&lt;", "<");

	/*  "&gt;", "> */
	reg_replace(source_string, "&gt;", ">");

	/*  "&copy;", "(c) */
	/* reg_replace(source_string, "&copy;", "(c)"); */

	/* "&quot;", "\' */
	reg_replace(source_string, "&quot;", "\'");

	/*  "&nbsp;", " */
	reg_replace(source_string, "&nbsp;", " ");

	reg_replace(source_string, "  ", " ");

	EM_DEBUG_FUNC_END();

	return result;
}

#define MAX_NOTI_STRING_LENGTH		8096

INTERNAL_FUNC int emcore_convert_structure_to_string(void *struct_var, char **encoded_string, email_convert_struct_type_e type)
{
	EM_DEBUG_FUNC_BEGIN("Struct type[%d]", type);
	
	char *buf = NULL;
	char delimiter[] = {0x01, 0x00};
	int error_code = EMAIL_ERROR_NONE;

	buf = (char *) malloc(MAX_NOTI_STRING_LENGTH * sizeof(char));
	if (NULL == buf) {
		error_code = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;		
	}
	switch (type) {
		case EMAIL_CONVERT_STRUCT_TYPE_MAIL_LIST_ITEM: {
			email_mail_list_item_t *item = (email_mail_list_item_t *)struct_var;
			SNPRINTF(buf, MAX_NOTI_STRING_LENGTH, 
				"%d%c"	/*	int  mail_id                                    ; */
				"%d%c"	/* 	int  account_id									; */
				"%d%c"	/*	int  mailbox_id                                 ; */
				"%s%c"	/* 	char from[STRING_LENGTH_FOR_DISPLAY]			; */
				"%s%c"	/* 	char from_email_address[MAX_EMAIL_ADDRESS_LENGTH]; */
				"%s%c"	/* 	char recipients[STRING_LENGTH_FOR_DISPLAY]      ; */
				"%s%c"	/* 	char subject[STRING_LENGTH_FOR_DISPLAY] 		; */
				"%d%c"	/* 	int  is_text_downloaded 						; */
				"%d%c"	/* 	time_t date_time                                ; */
				"%d%c"	/* 	int  flags_seen_field                           ; */
				"%d%c"	/* 	int  priority									; */
				"%d%c"	/* 	int  save_status                                ; */
				"%d%c"	/* 	int  is_locked									; */
				"%d%c"	/* 	int  is_report_mail								; */
				"%d%c"	/* 	int  recipients_count  							; */
				"%d%c"	/* 	int  has_attachment                             ; */
				"%d%c"	/* 	int  has_drm_attachment							; */
				"%s%c"	/* 	char previewBodyText[MAX_PREVIEW_TEXT_LENGTH]	; */
				"%d%c"	/* 	int  thread_id									; */
				"%d%c",	/* 	int  thread_item_count 							; */

				item->mail_id,              delimiter[0],
				item->account_id,           delimiter[0],
				item->mailbox_id,           delimiter[0],
				item->from,                 delimiter[0],
				item->from_email_address,   delimiter[0],
				item->recipients,           delimiter[0],
				item->subject,              delimiter[0],
				item->is_text_downloaded,   delimiter[0],
				(unsigned int)item->date_time,       delimiter[0],
				item->flags_seen_field,     delimiter[0],
				item->priority, 			delimiter[0],
				item->save_status,          delimiter[0],
				item->is_locked,            delimiter[0],
				item->is_report_mail,       delimiter[0],
				item->recipients_count,     delimiter[0],
				item->has_attachment,       delimiter[0],
				item->has_drm_attachment,   delimiter[0],
				item->previewBodyText,      delimiter[0],
				item->thread_id,            delimiter[0],
				item->thread_item_count,    delimiter[0]
			);
		}		
			break;
	}

FINISH_OFF:
	if (encoded_string)
		*encoded_string = buf;
	EM_DEBUG_FUNC_END("Struct -> String:[%s]\n", buf);
	return error_code;
}

INTERNAL_FUNC int emcore_convert_string_to_structure(const char *encoded_string, void **struct_var, email_convert_struct_type_e type)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	void *temp_struct = NULL;
	char *buff = NULL;
	char *current_pos = NULL;
	char *found_pos = NULL;
	char delimiter[] = {0x01, 0x00};
	int error_code = EMAIL_ERROR_NONE;

	EM_DEBUG_LOG("Struct Type[%d], String:[%s]", type, encoded_string);

	buff = (char *)EM_SAFE_STRDUP(encoded_string);
	if (NULL == buff) {
		error_code = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;		
	}
	
	switch (type) {
		case EMAIL_CONVERT_STRUCT_TYPE_MAIL_LIST_ITEM: {
			email_mail_list_item_t *item = (email_mail_list_item_t *)malloc(sizeof(email_mail_list_item_t));
			if (NULL == item) {
				error_code = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;		
			}
			temp_struct = (void *)item;

			current_pos = buff;

			/*  mail_id */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			item->mail_id = atoi(current_pos);
			EM_DEBUG_LOG("mail_id[%d]", item->mail_id);
			current_pos = found_pos + 1;

			/*  account_id */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			item->account_id = atoi(current_pos);
			EM_DEBUG_LOG("account_id[%d]", item->account_id);
			current_pos = found_pos + 1;

			/*  mailbox_id */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			item->mailbox_id = atoi(current_pos);
			EM_DEBUG_LOG("mailbox_id[%s]", item->mailbox_id);
			current_pos = found_pos + 1;

			/*  from */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			strncpy(item->from, current_pos, STRING_LENGTH_FOR_DISPLAY-1);
			EM_DEBUG_LOG("from[%s]", item->from);
			current_pos = found_pos + 1;

			/*  from_email_address */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			strncpy(item->from_email_address, current_pos, STRING_LENGTH_FOR_DISPLAY-1);
			EM_DEBUG_LOG("from_email_address[%s]", item->from_email_address);
			current_pos = found_pos + 1;

			/*  recipients */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			strncpy(item->recipients, current_pos, STRING_LENGTH_FOR_DISPLAY-1);
			EM_DEBUG_LOG("recipients[%s]", item->recipients);
			current_pos = found_pos + 1;			

			/*  subject */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			strncpy(item->subject, current_pos, STRING_LENGTH_FOR_DISPLAY-1);
			EM_DEBUG_LOG("subject[%s]", item->subject);
			current_pos = found_pos + 1;			

			/*  is_text_downloaded */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			item->is_text_downloaded = atoi(current_pos);
			EM_DEBUG_LOG("is_text_downloaded[%d]", item->is_text_downloaded);
			current_pos = found_pos + 1;

			/*  datatime */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			item->date_time = atoi(current_pos);
			EM_DEBUG_LOG("date_time[%d]", item->date_time);
			current_pos = found_pos + 1;

			/*  flags_seen_field */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			item->flags_seen_field = atoi(current_pos);
			EM_DEBUG_LOG("flags_seen_field[%d]", item->flags_seen_field);
			current_pos = found_pos + 1;

			/*  priority */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			item->priority = atoi(current_pos);
			EM_DEBUG_LOG("priority[%d]", item->priority);
			current_pos = found_pos + 1;			

			/*  save_status */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			item->save_status = atoi(current_pos);
			EM_DEBUG_LOG("save_status[%d]", item->save_status);
			current_pos = found_pos + 1;	

			/*  is_locked */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			item->is_locked = atoi(current_pos);
			EM_DEBUG_LOG("is_locked[%d]", item->is_locked);
			current_pos = found_pos + 1;	

			/*  is_report_mail */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			item->is_report_mail = atoi(current_pos);
			EM_DEBUG_LOG("is_report_mail[%d]", item->is_report_mail);
			current_pos = found_pos + 1;	

			/*  recipients_count */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			item->recipients_count = atoi(current_pos);
			EM_DEBUG_LOG("is_report_mail[%d]", item->recipients_count);
			current_pos = found_pos + 1;				

			/*  has_attachment */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			item->has_attachment = atoi(current_pos);
			EM_DEBUG_LOG("has_attachment[%d]", item->has_attachment);
			current_pos = found_pos + 1;				

			/*  has_drm_attachment */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			item->has_drm_attachment = atoi(current_pos);
			EM_DEBUG_LOG("has_drm_attachment[%d]", item->has_drm_attachment);
			current_pos = found_pos + 1;				

			/*  previewBodyText */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			strncpy(item->previewBodyText, current_pos, MAX_PREVIEW_TEXT_LENGTH-1);
			EM_DEBUG_LOG("previewBodyText[%s]", item->previewBodyText);
			current_pos = found_pos + 1;	

			/*  thread_id */
			found_pos = strstr(current_pos, delimiter);
			if (NULL == found_pos) {
				error_code = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;		
			}
			*found_pos = NULL_CHAR;
			item->thread_id = atoi(current_pos);
			EM_DEBUG_LOG("thread_id[%d]", item->thread_id);
			current_pos = found_pos + 1;	

			/*  thread_item_count - the last item */
 			item->thread_item_count = atoi(current_pos);
			EM_DEBUG_LOG("thread_item_count[%d]", item->thread_item_count);
 			
 		}
			break;
			
		default:
			EM_DEBUG_EXCEPTION("Unknown structure type");
			break;
	}

	ret = true;
	
FINISH_OFF:
	EM_SAFE_FREE(buff);
	if (ret == true) {
		if (struct_var)
			*struct_var = temp_struct;
	}
	else {
		switch (type) {
			case EMAIL_CONVERT_STRUCT_TYPE_MAIL_LIST_ITEM:
				EM_SAFE_FREE(temp_struct);
				break;
			default:
				break;
		}
	}
	EM_DEBUG_FUNC_END();
	return error_code;
}


/*  emcore_send_noti_for_new_mail is not used currently because DBUS could not send very long message.*/
/*  But I think it can be used to notify incomming new mail for replacing NOTI_MAIL_ADD with some modification(uid should be replaced with mail_id).  */
/*  This notification is including addtional information comparing NOTI_MAIL_ADD. */
/*  By this change, email application will be able to add email item without additional DB query.  */
/*  It might improve performance of sync email.  */
/*  kyuho.jo 2010-09-07 */

INTERNAL_FUNC int emcore_send_noti_for_new_mail(int account_id, char *mailbox_name, char *subject, char *from, char *uid, char *datetime)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_name(%s) subject(%s), from(%s), uid(%s), datetime(%s)", mailbox_name, subject, from, uid, datetime);
	int error_code = EMAIL_ERROR_NONE;
	char *param_string = NULL;

	if (mailbox_name == NULL || subject == NULL || from == NULL || uid == NULL || datetime == NULL) {
		error_code = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("Invalid parameter, mailbox_name(%p), subject(%p), from(%p), uid(%p), datetime(%p)", mailbox_name, subject, from, uid, datetime);
		goto FINISH_OFF;
	}

	param_string = malloc(strlen(mailbox_name) + strlen(subject) + strlen(from) + strlen(uid) + strlen(datetime) + 5);

	if (param_string == NULL) {
		error_code = EMAIL_ERROR_OUT_OF_MEMORY;
		EM_DEBUG_EXCEPTION("Memory allocation for 'param_string' is failed");
		goto FINISH_OFF;
	}

	memset(param_string, 0x00, sizeof(param_string));

	SNPRINTF(param_string, sizeof(param_string), "%s%c%s%c%s%c%s%c%s", mailbox_name, 0x01, subject, 0x01, from, 0x01, uid, 0x01, datetime);

	if (emstorage_notify_network_event(NOTI_DOWNLOAD_NEW_MAIL, account_id, param_string, 0, 0) == 0) {	/*  failed */
		error_code = EMAIL_ERROR_UNKNOWN;
		EM_DEBUG_EXCEPTION("emstorage_notify_network_event is failed");
		goto FINISH_OFF;
	}

FINISH_OFF:
	if (param_string)
		free(param_string);	
   	EM_DEBUG_FUNC_END();
	return error_code;
}



#define MAX_TITLE_LENGTH 1024
int emcore_update_notification_for_unread_mail(int account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d]", account_id); 
	int error_code = EMAIL_ERROR_NONE;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	if((noti_err = notification_update(NULL)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_update failed");
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("return [%d]", error_code);
	return error_code;
}

INTERNAL_FUNC int emcore_finalize_sync(int account_id, int *error)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], error [%p]", account_id, error);
	int err = EMAIL_ERROR_NONE, ret = true, result_sync_status = SYNC_STATUS_FINISHED;
	emstorage_account_tbl_t *account_tbl = NULL;

	if ((err = emcore_update_sync_status_of_account(account_id, SET_TYPE_MINUS, SYNC_STATUS_SYNCING)) != EMAIL_ERROR_NONE)
		EM_DEBUG_EXCEPTION("emcore_update_sync_status_of_account failed [%d]", err);

	if (!emstorage_get_sync_status_of_account(ALL_ACCOUNT, &result_sync_status, &err))
		EM_DEBUG_EXCEPTION("emstorage_get_sync_status_of_account failed [%d]", err);

	if (result_sync_status == SYNC_STATUS_HAVE_NEW_MAILS) {
		if (!emcore_update_notification_for_unread_mail(ALL_ACCOUNT))
			EM_DEBUG_EXCEPTION("emcore_update_notification_for_unread_mail failed");	
			emcore_check_unread_mail();
			/* Temp.. exception for EAS */
			if(account_id >= FIRST_ACCOUNT_ID)
				emstorage_get_account_by_id(account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account_tbl, true, &err);
			if(account_tbl && account_tbl->incoming_server_type != EMAIL_SERVER_TYPE_ACTIVE_SYNC)
				emcore_start_alert();

			if ((err = emcore_update_sync_status_of_account(account_id, SET_TYPE_MINUS, SYNC_STATUS_HAVE_NEW_MAILS)) != EMAIL_ERROR_NONE)
				EM_DEBUG_EXCEPTION("emcore_update_sync_status_of_account failed [%d]", err);

	}
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_clear_all_notifications()
{
	int account_count = 0, i;
	emstorage_account_tbl_t *account_list;
	int error_code = EMAIL_ERROR_NONE;
	
	if(!emstorage_get_account_list(&account_count, &account_list, true, false, &error_code)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_list failed");
		goto FINISH_OFF;
	}

	for(i = 0; i < account_count; i++) {
		emcore_delete_notification_by_account(account_list[i].account_id);
	}

FINISH_OFF:
	if(account_count) {
		emstorage_free_account(&account_list, account_count, NULL);
	}

	EM_DEBUG_FUNC_END("return[%d]", error_code);
	return error_code;
}

INTERNAL_FUNC int emcore_add_notification_for_unread_mail(emstorage_mail_tbl_t *input_mail_tbl_data)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_tbl_data[%p]", input_mail_tbl_data);

	int err = EMAIL_ERROR_NONE;

	if (input_mail_tbl_data == NULL) {
		EM_DEBUG_EXCEPTION("input_mail_tbl_data is NULL");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}


INTERNAL_FUNC int emcore_delete_notification_for_read_mail(int mail_id)
{
	EM_DEBUG_FUNC_BEGIN();
	int error_code = EMAIL_ERROR_NONE;

	EM_DEBUG_FUNC_END();
	return error_code;
}

#define EAS_EXECUTABLE_PATH "/usr/bin/eas-engine"

INTERNAL_FUNC int emcore_delete_notification_by_account(int account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d]", account_id);
	int error_code = EMAIL_ERROR_NONE;
	EM_DEBUG_FUNC_END();
	return error_code;
}

#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__

/**
 * @fn emcore_convert_to_uid_range_set(email_id_set_t* id_set, int id_set_count, email_uid_range_set **uid_range_set, int range_len, int *err_code)
 * Prepare a linked list of uid ranges with each node having a uid_range and lowest and highest uid in it.
 *
 *@author 					h.gahlaut@samsung.com
 * @param[in] id_set			Specifies the array of mail_id and corresponding server_mail_id sorted by server_mail_ids in ascending order
 * @param[in] id_set_count		Specifies the no. of cells in id_set array i.e. no. of sets of mail_ids and server_mail_ids
 * @param[in] range_len		Specifies the maximum length of string of range allowed. 
 * @param[out] uid_range_set	Returns the uid_ranges formed in the form of a linked list with head stored in uid_range_set pointer
 * @param[out] err_code		Returns the error code.
 * @remarks 					An example of a uid_range formed is 2:6,8,10,14:15,89, 
 *							While using it the caller should remove the ending, (comma)
 * @return This function returns true on success or false on failure.
 */
 
INTERNAL_FUNC int emcore_convert_to_uid_range_set(email_id_set_t *id_set, int id_set_count, email_uid_range_set **uid_range_set, int range_len, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int error = EMAIL_ERROR_NONE;

	if (NULL == id_set || id_set_count  <= 0 || NULL == uid_range_set) {
		EM_DEBUG_EXCEPTION(" Invalid Parameter id_set[%p] id_set_count[%d] uid_range_set[%p]", id_set, id_set_count, uid_range_set);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	int i = 0;
	unsigned long current_uid = 0;
	unsigned long first_uid = 0;
	unsigned long last_uid = 0;
	const int max_subset_string_size = MAX_SUBSET_STRING_SIZE;			
	char subset_string[MAX_SUBSET_STRING_SIZE] = {0,}; 	
	email_uid_range_set *current_node = NULL;	/* current_node denotes the current node under processing in the linked list of uid_range_set that is to be formed*/

	if (range_len < (max_subset_string_size + 1))		/* 1 for ending NULL character */ {
		EM_DEBUG_EXCEPTION(" Invalid Parameter range_len[%d]", range_len);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	EM_DEBUG_LOG("id set count[%d] range_len[%d]", id_set_count, range_len);
	
	do {
		first_uid = last_uid = current_uid = id_set[i].server_mail_id;
		/* Start subset string by putting first server mail id in it from id_set*/
		memset(subset_string, 0x00, max_subset_string_size);
		SNPRINTF(subset_string, max_subset_string_size, "%lu", first_uid);
		++i;
		
		/* Check if only one server mail id was left in id_set */
		if (i >= id_set_count) {
			/* No more server mail id left in id_set */
			if (false == emcore_append_subset_string_to_uid_range(subset_string, &current_node, uid_range_set, range_len, first_uid, last_uid)) {
				EM_DEBUG_EXCEPTION("emcore_append_subset_string_to_uid_range failed");
				goto FINISH_OFF;
			}
			break;
		}
		else {
			/* More server mail id are present in id_set. Find out if first:last_uid is to be formed or only first_uid will be subset string */
			do {
				current_uid = id_set[i].server_mail_id;
				if (current_uid == (last_uid + 1)) {
					last_uid = current_uid;
					++i;			
				}
				else {	
					memset(subset_string, 0x00, max_subset_string_size);
					if (first_uid != last_uid)	/* Form subset string by first_uid:last_uid */
						SNPRINTF(subset_string, max_subset_string_size, "%lu:%lu", first_uid, last_uid);
					else	/* Form subset string by first_uid */
						SNPRINTF(subset_string, max_subset_string_size, "%lu", first_uid);
					
					if (false == emcore_append_subset_string_to_uid_range(subset_string, &current_node, uid_range_set, range_len, first_uid, last_uid)) {
						EM_DEBUG_EXCEPTION("emcore_append_subset_string_to_uid_range failed");
						goto FINISH_OFF;
					}
					/* To Start formation of new subset string break out of inner loop */
					break;					
				}
				
			} while (i < id_set_count);		

			/* Flow comes here in two cases :
			1. id_set ended and has continuous numbers at end of id_set so form subset string by first_uid:last_uid . in this  case last_uid == current_uid
			2. due to break statement */

			if (last_uid == current_uid) {
				/* Case 1 */
				
				memset(subset_string, 0x00, max_subset_string_size);
				SNPRINTF(subset_string, max_subset_string_size, "%lu:%lu", first_uid, last_uid);
			
				if (false == emcore_append_subset_string_to_uid_range(subset_string, &current_node, uid_range_set, range_len, first_uid, last_uid)) {
					EM_DEBUG_EXCEPTION("emcore_append_subset_string_to_uid_range failed");
					goto FINISH_OFF;
				}
			}
			else {
				/* Case 2: Do Nothing */
			}
				
		}		
	} while (i < id_set_count);

	ret = true;
	
FINISH_OFF:
	if (NULL != err_code)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
	
}  

/**
 * @fn emcore_append_subset_string_to_uid_range(char *subset_string, email_uid_range_set **uid_range_set, int range_len, unsigned long luid, unsigned long huid)
 * Appends the subset_string to uid range if the uid range has not exceeded maximum length(range_len), otherwise creates a new node in linked list of uid range set 
 * and stores the subset_string in its uid_range. Also sets the lowest and highest uids for the corresponsing uid_range
 * 
 * @author					h.gahlaut@samsung.com
 * @param[in] subset_string	Specifies the subset string to be appended. A subset string can be like X:Y or X where X and Y are uids.
 * @param[in] range_len		Specifies the maximum length of range string allowed. 
 * @param[in] luid			Specifies the lowest uid in subset string
 * @param[in] huid			Specifies the highest uid in subset string
 * @param[out] uid_range_set	Returns the uid_ranges formed in the form of a linked list with head stored in uid_range_set pointer
 * @param[out] err_code		Returns the error code.
 * @remarks 										
 * @return This function returns true on success or false on failure.
 */
 
int emcore_append_subset_string_to_uid_range(char *subset_string, email_uid_range_set **current_node_adr, email_uid_range_set **uid_range_set, int range_len, unsigned long luid, unsigned long huid)
{
	EM_DEBUG_FUNC_BEGIN();
	email_uid_range_set *current_node = NULL;

	if (NULL == (*uid_range_set)) {
		/*This happens only once when list  creation starts. Head Node is allocated */
		current_node = (email_uid_range_set *)em_malloc(sizeof(email_uid_range_set));
		if (NULL == current_node) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			return false;
		}	

		current_node->uid_range = (char *)em_malloc(range_len);
			
		if (NULL == current_node->uid_range) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			EM_SAFE_FREE(current_node);
			return false;
		}

		SNPRINTF(current_node->uid_range, range_len, "%s,", subset_string);

		current_node->lowest_uid = luid;
		current_node->highest_uid = huid;
		(*uid_range_set) = current_node;

		(*current_node_adr) = current_node; 
		
	}
	else {
		/* Apart from first call to this function flow will always come here */
		current_node = (*current_node_adr);
		int len_sub_string = strlen(subset_string);
		int space_left_in_buffer = range_len - strlen(current_node->uid_range);

		if ((len_sub_string + 1 + 1) <= space_left_in_buffer) 	/* 1 for comma + 1 for ending null character */ {
			SNPRINTF(current_node->uid_range + strlen(current_node->uid_range), space_left_in_buffer, "%s,", subset_string);
			current_node->highest_uid = huid;
		}
		else {
			/* No more space left in uid_range string.If continued on it, it will exceeded max size of range_len */
			/* Allocate new node in Uid Range set */
			email_uid_range_set *new_node = NULL;

			new_node = (email_uid_range_set *)em_malloc(sizeof(email_uid_range_set));

			if (NULL == new_node) {
				EM_DEBUG_EXCEPTION("em_malloc failed");
				return false;
			}

			/* Allocate uid_range of new node */
		
			new_node->uid_range =  (char *)em_malloc(range_len);

			if (NULL == new_node->uid_range) {
				EM_DEBUG_EXCEPTION("em_malloc failed");
				EM_SAFE_FREE(new_node);
				return false;
			}

			SNPRINTF(new_node->uid_range, range_len, "%s, ", subset_string);

			new_node->lowest_uid = luid;
			new_node->highest_uid = huid;

			current_node->next = new_node;

			(*current_node_adr) = new_node;	
		}
	}
	EM_DEBUG_FUNC_END();
	return true;
}

/**
 * void emcore_free_uid_range_set(email_uid_range_set **uid_range_head)
 * Frees the linked list of uid ranges 
 *
 * @author 					h.gahlaut@samsung.com
 * @param[in] uid_range_head	Head pointer of linked list of uid ranges		
 * @remarks 									
 * @return This function does not return anything.
 */
 
INTERNAL_FUNC
void emcore_free_uid_range_set(email_uid_range_set **uid_range_set)
{
	EM_DEBUG_FUNC_BEGIN();

	email_uid_range_set *current_node = NULL;
	email_uid_range_set *uid_range_head = NULL;
	
	current_node = uid_range_head = (*uid_range_set);	/* Make the current node and head ptr point to starting of  uid_range_set */

	while (current_node) {		
		uid_range_head = current_node->next;		/* Move the head ptr to next node*/

		EM_SAFE_FREE(current_node->uid_range);
		EM_SAFE_FREE(current_node);				/* Free the current node */

		current_node = uid_range_head;			/* Make the current node point to head ptr */
	}

	(*uid_range_set) = NULL;
	EM_DEBUG_FUNC_END();
}


/**
 * @fn emcore_form_comma_separated_strings(int numbers[], int num_count, int max_string_len, char *** strings, int *string_count, int *err_code)
 * Forms comma separated strings of a give max_string_len from an array of numbers 
 * 
 * @author 					h.gahlaut@samsung.com
 * @param[in] numbers			Specifies the array of numbers to be converted into comma separated strings.
 * @param[in] num_count		Specifies the count of numbers in numbers array. 
 * @param[in] max_string_len	Specifies the maximum length of comma separated strings that are to be formed.
 * @param[out] strings			Returns the base address of a double dimension array which stores the strings.
 * @param[out] string_count		Returns the number of strings formed.
 * @param[out] err_code		Returns the error code.
 * @remarks 					If Input to the function is five numbers like 2755 2754 2748 2749 2750 and a given max_string_len is 20.
 *							Then this function will form two comma separated strings as follows -
 *							"2755, 2754, 2748"
 *							"2749, 2750"
 * @return This function returns true on success or false on failure.
 */

INTERNAL_FUNC int emcore_form_comma_separated_strings(int numbers[], int num_count, int max_string_len, char *** strings, int *string_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int error = EMAIL_ERROR_NONE;
	int ret = false;

	char **string_list = NULL;
	int num_of_strings = 0;
	int i = 0;
	int j =0;
	char num[MAX_INTEGER_LENGTH + 1] = {0, };
	int num_len = 0;
	int space_in_buffer = 0;
	int len_of_string_formed = 0;

	if (NULL == numbers || num_count <= 0 || \
		max_string_len < (MAX_INTEGER_LENGTH + 2)|| NULL == strings || NULL == string_count)			/*  32767, is the highest integer possible in string.This requires 7 bytes of storage in character type array (1 byte for ending NULL and 1 byte for ending comma) so max_string_len should not be less than worst case possible.  */ {
		EM_DEBUG_EXCEPTION("Invalid Parameter numbers[%p] num_count [%d] max_string_len [%d] strings [%p] string_count[%p]", \
			numbers, num_count, max_string_len, strings, string_count);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("num_count [%d] max_string_len [%d]", num_count, max_string_len);

	string_list = em_malloc(sizeof(char *));

	if (NULL == string_list) {
		EM_DEBUG_EXCEPTION("em_malloc failed ");
		goto FINISH_OFF;
	}
	
	string_list[num_of_strings] = em_malloc(max_string_len);
	
	if (NULL == string_list[num_of_strings]) {
		EM_DEBUG_EXCEPTION("em_malloc failed ");
		goto FINISH_OFF;
	}
	
	++num_of_strings;
	space_in_buffer = max_string_len;
	
	for (j = 0; j < num_count;++j) {
		memset(num, 0x00, MAX_INTEGER_LENGTH + 1);
		SNPRINTF(num, MAX_INTEGER_LENGTH + 1, "%d", numbers[j]);

		num_len = strlen(num);
		
		len_of_string_formed = strlen(string_list[num_of_strings - 1]);

		space_in_buffer = max_string_len - len_of_string_formed ;

		if (space_in_buffer >= (num_len+1+1))			/*  1 for comma and 1 for ending NULL */ {
			SNPRINTF(string_list[num_of_strings - 1] + len_of_string_formed, max_string_len, "%d,", numbers[j]);
		}
		else {	/*  Removing comma at end of string  */
			string_list[num_of_strings - 1][len_of_string_formed-1] = '\0';		
			char **temp = NULL;
			temp = (char **)realloc(string_list, sizeof(char *) * (num_of_strings + 1));	/*  Allocate new buffer to store a pointer to a new string */

			if (NULL == temp) {	
				EM_DEBUG_EXCEPTION("realloc failed");
				goto FINISH_OFF;
			}

			memset(temp + num_of_strings, 0X00, sizeof(char *));

			string_list = temp;
			temp = NULL;
			string_list[num_of_strings] = em_malloc(max_string_len);/*  Allocate new buffer to store the string */

			if (NULL == string_list[num_of_strings]) {
				EM_DEBUG_EXCEPTION(" em_malloc failed ");
				goto FINISH_OFF;
			}
			++num_of_strings;
			SNPRINTF(string_list[num_of_strings - 1] , max_string_len, "%d,", numbers[j]);/*  Start making new string */
		}
	}

	/*  Removing comma at end of string  */
	len_of_string_formed = strlen(string_list[num_of_strings - 1]);
	string_list[num_of_strings - 1][len_of_string_formed-1] = '\0';	
	ret = true;

FINISH_OFF:

	if (false == ret)
		emcore_free_comma_separated_strings(&string_list, &num_of_strings);

	if (true == ret) {
		for (i = 0; i < num_of_strings;++i)
			EM_DEBUG_LOG("%s", string_list[i]);
		*strings = string_list;
		*string_count = num_of_strings;
	}
	

	if (NULL != err_code)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
/**
 * @fn emcore_free_comma_separated_strings(char *** string_list, int *string_count)
 * Frees the double dimensional array of strings. 
 *
 * @author 					h.gahlaut@samsung.com
 * @param[in] uid_range_head	Address of base address of double dimensional array of strings.
 * @param[in] string_count		Address of variable holding the count of strings.
 * @remarks 									
 * @return This function does not return anything.
 */
INTERNAL_FUNC void emcore_free_comma_separated_strings(char *** string_list, int *string_count)
{
	EM_DEBUG_FUNC_BEGIN();
	int i = 0;
	char **str_list = NULL;
	int count = 0;

	if (NULL != string_list) {
		str_list = *string_list;

		if (0 != *string_count) {
			count = *string_count;
			for (i = 0; i < count; ++i)
				EM_SAFE_FREE(str_list[i]);
		}
		
		EM_SAFE_FREE(str_list);
		*string_list = NULL;	/*  This makes the pointer to be freed as NULL in calling function and saves it from being a dangling pointer for sometime in calling function */
		*string_count = 0;
	}
	EM_DEBUG_FUNC_END();
}


#endif




int emcore_make_attachment_file_name_with_extension(char *source_file_name, char *sub_type, char *result_file_name, int result_file_name_buffer_length, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("source_file_name[%s], sub_type[%s], result_file_name_buffer_length[%d] ", source_file_name, sub_type, result_file_name_buffer_length);
	int ret = false, err = EMAIL_ERROR_NONE;
	char *extcheck = NULL;
	char attachment_file_name[MAX_PATH + 1] = { 0, };

	if (!source_file_name || !result_file_name) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err  = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
				
	strncpy(attachment_file_name, source_file_name, MAX_PATH);
	extcheck = strchr(attachment_file_name, '.');

	if (extcheck)
		EM_DEBUG_LOG("Extension Exist in the Attachment [%s] ", extcheck);
	else  {	/* No extension attached, So add the Extension based on the subtype */
		if (sub_type) {
			strcat(attachment_file_name, ".");
			strcat(attachment_file_name, sub_type);
			EM_DEBUG_LOG("attachment_file_name with extension[%s] ", attachment_file_name);
		}
		else
			EM_DEBUG_LOG("UnKnown Extesnsion");

	}
	memset(result_file_name, 0 , result_file_name_buffer_length);
	EM_SAFE_STRNCPY(result_file_name, attachment_file_name, result_file_name_buffer_length - 1);
	EM_DEBUG_LOG("*result_file_name[%s]", result_file_name);
	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

#ifdef __FEATURE_LOCAL_ACTIVITY__
INTERNAL_FUNC int emcore_add_activity(emstorage_activity_tbl_t *new_activity, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	EM_DEBUG_LOG("\t new_activity[%p], err_code[%p]", new_activity, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	
	if (!new_activity) {
		EM_DEBUG_LOG("\t new_activity[%p]\n", new_activity);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	if (!emstorage_add_activity(new_activity, false, &err)) {
		EM_DEBUG_LOG("\t emstorage_add_activity falied - %d\n", err);
		
		goto FINISH_OFF;
	}
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	
	return ret;
}

INTERNAL_FUNC int emcore_delete_activity(emstorage_activity_tbl_t *activity, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	EM_DEBUG_LOG("\t new_activity[%p], err_code[%p]", activity, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	if (!activity) {
		EM_DEBUG_LOG("\t new_activity[%p]\n", activity);
		
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	if (!emstorage_delete_local_activity(activity, true, &err)) {
		EM_DEBUG_LOG("\t emstorage_delete_local_activity falied - %d\n", err);
		
		goto FINISH_OFF;
	}
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	
	return ret;
}

INTERNAL_FUNC int emcore_get_next_activity_id(int *activity_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (NULL == activity_id)
   	{
		EM_DEBUG_EXCEPTION("\t activity_id[%p]", activity_id);
		
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (false == emstorage_get_next_activity_id(activity_id, &err)) {
		EM_DEBUG_LOG("\t emstorage_get_next_activity_id failed - %d\n", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
	FINISH_OFF:
	if (NULL != err_code) {
		*err_code = err;
	}
	
	return ret;

}

#endif /* __FEATURE_LOCAL_ACTIVITY__ */


INTERNAL_FUNC void emcore_free_rule(email_rule_t* rule)
{
	EM_DEBUG_FUNC_BEGIN();

	if (!rule)
		return;

	EM_SAFE_FREE(rule->value);

	EM_DEBUG_FUNC_END();
}


/* EOF */
