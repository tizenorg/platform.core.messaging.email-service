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


/******************************************************************************
 * File :  email-core-mm_callbacks.c
 * Desc :  mm_callbacks for IMAP-2004g
 *
 * Auth :
 *
 * History :
 * 2006.08.22  :  created
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "email-internal-types.h"
#include "c-client.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-debug-log.h"
#include "email-core-mailbox.h"
#include "email-core-account.h"

static void mm_get_error(char *string, int *err_code);

/*
 * callback mm_lsub
 *		get subscribed mailbox list
 */
INTERNAL_FUNC void mm_lsub(MAILSTREAM *stream, int delimiter, char *mailbox, long attributes)
{
	EM_DEBUG_FUNC_BEGIN();
	email_callback_holder_t *p_holder = (email_callback_holder_t *)stream->sparep;
	email_mailbox_t *p, *p_old = p_holder->data;
	int count = p_holder->num;
	char *s, *enc_path;

	/* memory allocation */
	p = realloc(p_old, sizeof(email_mailbox_t) * (count + 1));
	if (!p) return ;

	/* uw-imap mailbox name format (ex : "{mail.test.com...}inbox" or "{mail.test.com...}anybox/anysubbox") */
	enc_path = strchr(mailbox, '}');
	if (enc_path)
		enc_path += 1;
	else {
		emcore_free_mailbox_list(&p, count+1);
		return ;
	}

	/* Convert UTF7 mailbox name to UTF8 mailbox name */

	/* convert directory delimiter to '/' */
	for (s = enc_path; *s; s++) {
		if (*s == (char)delimiter) {
			*s = '/';
		}
	}

	/* coyp string */
	p[count].mailbox_name = cpystr(enc_path);
	p[count].alias        = cpystr(enc_path);
	p[count].local        = 0;
	p[count].account_id   = stream->spare8;


	p_holder->data = p;
	p_holder->num++;

	/* ignore attributes */
/*  if (attributes & latt_noinferiors) fputs (", no inferiors", fp_log); */
/*  if (attributes & latt_noselect) fputs (", no select", fp_log); */
/*  if (attributes & latt_marked) fputs (", marked", fp_log); */
/*  if (attributes & latt_unmarked) fputs (", unmarked", fp_log); */
	EM_DEBUG_FUNC_END();
}


/*
 * callback mm_lsub
 * get mailbox list
 */
INTERNAL_FUNC void mm_list(MAILSTREAM *stream, int delimiter, char *mailbox, long attributes)
{
	EM_DEBUG_FUNC_BEGIN();
	email_callback_holder_t *p_holder = (email_callback_holder_t *)stream->sparep;
	email_internal_mailbox_t *p, *p_old = p_holder->data;
	int count = p_holder->num;
	char *s, *enc_path;

	/* memory allocation */
	p = realloc(p_old, sizeof(email_internal_mailbox_t) * (count + 1));
	if (!p) return ;

	/* uw-imap mailbox name format (ex : "{mail.test.com...}inbox" or "{mail.test.com...}anybox/anysubbox") */
	enc_path = strchr(mailbox, '}');
	if (enc_path)
		enc_path += 1;
	else {
		emcore_free_internal_mailbox(&p, count+1, NULL);
		return ;
	}

	/* convert directory delimiter to '/' */
	for (s = enc_path; *s; s++)
		if (*s == (char)delimiter)
			*s = '/';

	/* copy string */
	memset(p + count, 0x00, sizeof(email_internal_mailbox_t));

#ifdef __FEATURE_XLIST_SUPPORT__
	if (attributes & LATT_XLIST_INBOX)
		p[count].mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
	else if (attributes & LATT_XLIST_ALL)
		p[count].mailbox_type = EMAIL_MAILBOX_TYPE_ALL_EMAILS;
	else if (attributes & LATT_XLIST_DRAFTS)
		p[count].mailbox_type = EMAIL_MAILBOX_TYPE_DRAFT;
	else if (attributes & LATT_XLIST_SENT)
		p[count].mailbox_type = EMAIL_MAILBOX_TYPE_SENTBOX;
	else if (attributes & LATT_XLIST_JUNK)
		p[count].mailbox_type = EMAIL_MAILBOX_TYPE_SPAMBOX;
	else if (attributes & LATT_XLIST_FLAGGED)
		p[count].mailbox_type = EMAIL_MAILBOX_TYPE_USER_DEFINED; 		//EMAIL_MAILBOX_TYPE_FLAGGED; P141122-00523 sync starred folder as Inbox sync
	else if (attributes & LATT_XLIST_TRASH)
		p[count].mailbox_type = EMAIL_MAILBOX_TYPE_TRASH;
#endif /* __FEATURE_XLIST_SUPPORT__ */

	if (attributes & LATT_NOSELECT)
		p[count].no_select    = true;

	if (p[count].mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) /* For exception handling of Gmail inbox*/
		p[count].mailbox_name  = cpystr("INBOX");
	else
		p[count].mailbox_name  = cpystr(enc_path);

	EM_DEBUG_LOG_SEC("mailbox name [%s] mailbox_type [%d] no_select [%d]", p[count].mailbox_name, p[count].mailbox_type, p[count].no_select);

	p[count].alias = emcore_get_alias_of_mailbox((const char *)enc_path);
	p[count].local = 0;

	EM_DEBUG_LOG("mm_list account_id %d", stream->spare8);

	char *tmp = NULL;
	/* in mailbox name parse n get user = %d - which is account_id */
	tmp = strstr(mailbox, "user=");
	if (tmp) {
		tmp = tmp+5;
		for (s = tmp; *s != '/' &&  *s != '}' && *s != '\0'; s++);
		*s = '\0';
		if (strlen(tmp) > 0)
			p[count].account_id = atoi(tmp);
	}
	EM_DEBUG_LOG("mm_list account_id %d ", p[count].account_id);

	p_holder->data = p;
	p_holder->num++;

	/* ignore attributes */
	EM_DEBUG_FUNC_END();
}


/*
 * callback mm_status
 * get mailbox status
 */
INTERNAL_FUNC void mm_status(MAILSTREAM *stream, char *mailbox, MAILSTATUS* status)
{
	EM_DEBUG_FUNC_BEGIN();
	email_callback_holder_t *p = stream->sparep;

	EM_DEBUG_FUNC_BEGIN();
	if (status->flags & SA_MESSAGES)
		p->num = status->messages;
	if (status->flags & SA_UNSEEN)
		p->data = (void *)status->unseen;
	EM_DEBUG_FUNC_END();
}

/* callback mm_login
 * get user_name and password
 */

INTERNAL_FUNC void mm_login(NETMBX *mb, char *user, char *pwd, long trial)
{
	EM_DEBUG_FUNC_BEGIN();
	int account_id;
	email_account_t *ref_account = NULL;
	char *username = NULL;
	char *password = NULL;
	char *token    = NULL;
	char *save_ptr = NULL;
	char *user_info = NULL;
    char *temp = NULL;
	char *multi_user_name = NULL;

	if (!mb->user[0])  {
		EM_DEBUG_EXCEPTION("invalid account_id...");
		goto FINISH_OFF;
	}

	user_info = EM_SAFE_STRDUP(mb->user);

	token = strtok_r(user_info, TOKEN_FOR_MULTI_USER, &temp);
	EM_DEBUG_LOG_SEC("Token : [%s], multi_user_name:[%s][%d]", token, temp, EM_SAFE_STRLEN(temp));
	account_id = atoi(token);
	token = NULL;

    if (temp != NULL && EM_SAFE_STRLEN(temp) > 0)
        multi_user_name = EM_SAFE_STRDUP(temp);

	ref_account = emcore_get_account_reference(multi_user_name, account_id, true);
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed");
		goto FINISH_OFF;
	}

	if (ref_account->incoming_server_user_name == NULL) {
		EM_DEBUG_EXCEPTION("invalid incoming_server_user_name...");
		goto FINISH_OFF;
	}
	username = EM_SAFE_STRDUP(ref_account->incoming_server_user_name);

	if (ref_account->incoming_server_password == NULL) {
		EM_SAFE_FREE(username);
		EM_DEBUG_EXCEPTION("Empty password");
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("incoming_server_authentication_method [%d]", ref_account->incoming_server_authentication_method);

	if (ref_account->incoming_server_authentication_method == EMAIL_AUTHENTICATION_METHOD_XOAUTH2) {
		token = strtok_r(ref_account->incoming_server_password, "\001", &save_ptr);
		EM_DEBUG_LOG_SEC("token [%s]", token);
		password = EM_SAFE_STRDUP(token);
	} else {
		password = EM_SAFE_STRDUP(ref_account->incoming_server_password);
	}

	if (EM_SAFE_STRLEN(username) > 0 && EM_SAFE_STRLEN(password) > 0) {
		strcpy(user, username);
		strcpy(pwd, password);
	} else
		EM_DEBUG_EXCEPTION("User Information is NULL || EM_SAFE_STRLEN is 0 ");

FINISH_OFF:

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	EM_SAFE_FREE(user_info);
	EM_SAFE_FREE(username);
	EM_SAFE_FREE(password);
    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_FUNC_END();
}


INTERNAL_FUNC void mm_dlog(char *string)
{
#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG("IMAP_TOOLKIT_DLOG [%s]", string);
#endif
}

INTERNAL_FUNC void mm_log(char *string, long errflg)
{

	switch (errflg)  {
		case NIL:
			EM_DEBUG_LOG("IMAP_TOOLKIT_LOG [%s]", string);
			break;

		case WARN:
			EM_DEBUG_EXCEPTION("IMAP_TOOLKIT_LOG WARN [%s]", string);
			break;

		case PARSE:
			EM_DEBUG_LOG("IMAP_TOOLKIT_LOG PARSE [%s]", string);
			break;

		case BYE:
			EM_DEBUG_LOG("IMAP_TOOLKIT_LOG BYE [%s]", string);
			break;

		case TCPDEBUG:
			EM_DEBUG_LOG_SEC("IMAP_TOOLKIT_LOG TCPDEBUG [%s]", string);
			break;

		case ERROR: {
			email_session_t *session = NULL;

			EM_DEBUG_EXCEPTION("IMAP_TOOLKIT_LOG ERROR [%s]", string);

			emcore_get_current_session(&session);

			if (session) {
				mm_get_error(string, &session->error);
				EM_DEBUG_EXCEPTION("IMAP_TOOLKIT_LOG ERROR [%d]", session->error);
				if (session->error == EMAIL_ERROR_XOAUTH_BAD_REQUEST ||
                            session->error == EMAIL_ERROR_XOAUTH_INVALID_UNAUTHORIZED) {
					session->network = session->error;
				}
			}

			break;
		}
	}
}

INTERNAL_FUNC void mm_searched(MAILSTREAM *stream, unsigned long number)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p], number [%d]", stream, number);
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void mm_exists(MAILSTREAM *stream, unsigned long number)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p], number [%d]", stream, number);
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void mm_expunged(MAILSTREAM *stream, unsigned long number)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p], number [%d]", stream, number);
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void mm_flags(MAILSTREAM *stream, unsigned long number)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p], number [%d]", stream, number);
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void mm_notify(MAILSTREAM *stream, char *string, long errflg)
{
	EM_DEBUG_FUNC_BEGIN();
#ifdef FEATURE_CORE_DEBUG
	mm_log(string, errflg);
#endif /* FEATURE_CORE_DEBUG */
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void mm_critical(MAILSTREAM *stream)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p]", stream);
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void mm_nocritical(MAILSTREAM *stream)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p]", stream);
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC long mm_diskerror(MAILSTREAM *stream, long errcode, long serious)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p] errcode[%d] serious[%d]", stream, errcode, serious);
#if UNIXLIKE
	kill(getpid(), SIGSTOP);
#else
	abort();
#endif
	EM_DEBUG_FUNC_END();
	return NIL;
}

INTERNAL_FUNC void mm_fatal(char *string)
{
	EM_DEBUG_EXCEPTION("%s", string);
}

INTERNAL_FUNC void mm_get_error(char *string, int *err_code)
{
	if (!string || !err_code)
		return ;

	EM_DEBUG_LOG("string [%s]", string);

	if (strstr(string, "login failure") || strstr(string, "Login aborted") || strstr(string, "Can't login"))
		*err_code = EMAIL_ERROR_LOGIN_FAILURE;
	else if (strstr(string, "Scan not valid"))
		*err_code = EMAIL_ERROR_SCAN_NOT_SUPPORTED;
	else if (strstr(string, "Authentication cancelled"))
		*err_code = EMAIL_ERROR_AUTHENTICATE;
	else if (strstr(string, "authuser"))
		*err_code = EMAIL_ERROR_AUTH_NOT_SUPPORTED;
	else if (strstr(string, "negotiate TLS"))
		*err_code = EMAIL_ERROR_CANNOT_NEGOTIATE_TLS;
	else if (strstr(string, "TLS/SSL failure"))
		*err_code = EMAIL_ERROR_TLS_SSL_FAILURE;
	else if (strstr(string, "STARTLS"))
		*err_code = EMAIL_ERROR_STARTLS;
	else if (strstr(string, "TLS unavailable"))
		*err_code = EMAIL_ERROR_TLS_NOT_SUPPORTED;
	else if (strstr(string, "Can't access"))
		*err_code = EMAIL_ERROR_IMAP4_APPEND_FAILURE;
	else if (strstr(string, "Can not authenticate"))
		*err_code = EMAIL_ERROR_AUTHENTICATE;
	else if (strstr(string, "Unexpected IMAP response") || strstr(string, "hello"))
		*err_code = EMAIL_ERROR_INVALID_RESPONSE;
	else if (strstr(string, "NOTIMAP4REV1"))
		*err_code = EMAIL_ERROR_COMMAND_NOT_SUPPORTED;
	else if (strstr(string, "Anonymous"))
		*err_code = EMAIL_ERROR_ANNONYM_NOT_SUPPORTED;
	else if (strstr(string, "connection broken"))
		*err_code = EMAIL_ERROR_CONNECTION_BROKEN;
	else if (strstr(string, "SMTP greeting"))
		*err_code = EMAIL_ERROR_NO_RESPONSE;
	else if (strstr(string, "ESMTP failure"))
		*err_code = EMAIL_ERROR_SMTP_SEND_FAILURE;
	else if (strstr(string, "socket") || strstr(string, "Socket"))
		*err_code = EMAIL_ERROR_SOCKET_FAILURE;
	else if (strstr(string, "connect to") || strstr(string, "Connection failed"))
		*err_code = EMAIL_ERROR_CONNECTION_FAILURE;
	else if (strstr(string, "Certificate failure"))
		*err_code = EMAIL_ERROR_CERTIFICATE_FAILURE;
	else if (strstr(string, "ESMTP failure"))
		*err_code = EMAIL_ERROR_INVALID_PARAM;
	else if (strstr(string, "No such host"))
		*err_code = EMAIL_ERROR_NO_SUCH_HOST;
	else if (strstr(string, "host") || strstr(string, "Host"))
		*err_code = EMAIL_ERROR_INVALID_SERVER;
	else if (strstr(string, "SELECT failed"))
		*err_code = EMAIL_ERROR_MAILBOX_NOT_FOUND;
	else if (strstr(string, "15 minute"))
		*err_code = EMAIL_ERROR_LOGIN_ALLOWED_EVERY_15_MINS;
	else if (strstr(string, "\"status\":\"400"))
		*err_code = EMAIL_ERROR_XOAUTH_BAD_REQUEST;
	else if (strstr(string, "\"status\":\"401"))
		*err_code = EMAIL_ERROR_XOAUTH_INVALID_UNAUTHORIZED;
	else if (strstr(string, "ALREADYEXISTS"))
		*err_code = EMAIL_ERROR_ALREADY_EXISTS;
	else
		*err_code = EMAIL_ERROR_UNKNOWN;
}

#ifdef __FEATURE_SUPPORT_IMAP_ID__
INTERNAL_FUNC void mm_imap_id(char **id_string)
{
	EM_DEBUG_FUNC_BEGIN("id_string [%p]", id_string);

	/*
	char *result_string = NULL;
	char *tag_string = "ID (\"os\" \"" IMAP_ID_OS "\" \"os-version\" \"" IMAP_ID_OS_VERSION "\" \"vendor\" \"" IMAP_ID_VENDOR "\" \"device\" \"" IMAP_ID_DEVICE_NAME "\" \"AGUID\" \"" IMAP_ID_AGUID "\" \"ACLID\" \"" IMAP_ID_ACLID "\"";
	int   tag_length = 0;
	*/

	if (id_string == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		goto FINISH_OFF;
	}

	*id_string = NULL;

	/*
	tag_length = EM_SAFE_STRLEN(tag_string);
	result_string = EM_SAFE_STRDUP(tag_string);

	if (result_string == NULL) {
		EM_DEBUG_EXCEPTION("malloc failed");
		goto FINISH_OFF;
	}

	*id_string = result_string;
	*/
FINISH_OFF:
	return ;
}
#endif /* __FEATURE_SUPPORT_IMAP_ID__ */
/* EOF */
