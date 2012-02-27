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
 * File :  em-core-mm_callbacks.c
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
#include "em-core-types.h"
#include "c-client.h"
#include "em-core-global.h"
#include "em-core-utils.h"
#include "emf-dbglog.h"
#include "em-core-mailbox.h"
#include "em-core-account.h"

static void mm_get_error(char *string, int *err_code);

/*
 * callback mm_lsub
 *		get subscribed mailbox list
 */
EXPORT_API void mm_lsub(MAILSTREAM *stream, int delimiter, char *mailbox, long attributes)
{
	EM_DEBUG_FUNC_BEGIN();
	emf_callback_holder_t *p_holder = (emf_callback_holder_t *)stream->sparep;
	emf_mailbox_t *p, *p_old = p_holder->data;
	int count = p_holder->num;
	char *s, *enc_path;

	/* memory allocation */
	p = realloc(p_old, sizeof(emf_mailbox_t) * (count + 1));
	if (!p) return ;

	/* uw-imap mailbox name format (ex : "{mail.test.com...}inbox" or "{mail.test.com...}anybox/anysubbox") */
	enc_path = strchr(mailbox, '}');
	if (enc_path)	
		enc_path += 1;
	else {
		em_core_mailbox_free(&p, count+1, NULL);				
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
	p[count].name = cpystr(enc_path);
	p[count].alias = cpystr(enc_path);
	p[count].local = 0;
	p[count].account_id = stream->spare8;


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
EXPORT_API void mm_list(MAILSTREAM *stream, int delimiter, char *mailbox, long attributes)
{
	EM_DEBUG_FUNC_BEGIN();
	emf_callback_holder_t *p_holder = (emf_callback_holder_t *)stream->sparep;
	emf_mailbox_t *p, *p_old = p_holder->data;
	int count = p_holder->num;
	char *s, *enc_path;

	/* memory allocation */
	p = realloc(p_old, sizeof(emf_mailbox_t) * (count + 1));
	if (!p) return ;

	/* uw-imap mailbox name format (ex : "{mail.test.com...}inbox" or "{mail.test.com...}anybox/anysubbox") */
	enc_path = strchr(mailbox, '}');
	if (enc_path)	
		enc_path += 1;
	else {
		em_core_mailbox_free(&p, count+1, NULL);
		return ;
	}

	/* convert directory delimiter to '/' */
	for (s = enc_path;*s;s++) 
		if (*s == (char)delimiter) 
			*s = '/';

	/* coyp string */
	memset(p + count, 0x00, sizeof(emf_mailbox_t));
	p[count].name = cpystr(enc_path);
	p[count].alias = cpystr(enc_path);
	p[count].local = 0;
	EM_DEBUG_LOG("mm_list account_id %d", stream->spare8);

	char *tmp = NULL;
	/* in mailbox name parse n get user = %d - which is account_id */
	tmp = strstr(mailbox, "user=");
	if (tmp) {
		tmp = tmp+5;
		for (s = tmp; *s != '/'; s++);
		*s = '\0';
		p[count].account_id = atoi(tmp);
	}
	EM_DEBUG_LOG("mm_list account_id1 %d ", p[count].account_id);
	EM_DEBUG_LOG("mm_list mailbox name r %s ", p[count].name);
	p_holder->data = p;
	p_holder->num++;

	/* ignore attributes */
	EM_DEBUG_FUNC_END();
}


/*
 * callback mm_status
 * get mailbox status
 */
EXPORT_API void mm_status(MAILSTREAM *stream, char *mailbox, MAILSTATUS* status)
{
	EM_DEBUG_FUNC_BEGIN();
	emf_callback_holder_t *p = stream->sparep;

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

EXPORT_API void mm_login(NETMBX *mb, char *user, char *pwd, long trial)
{
	EM_DEBUG_FUNC_BEGIN();
	int account_id;
	emf_account_t *ref_account;
	char *username = NULL;
	char *password = NULL;

	if (!mb->user[0])  {
		EM_DEBUG_EXCEPTION("invalid account_id...");
		return;
	}
	
	account_id = atoi(mb->user);

	ref_account = em_core_get_account_reference(account_id);

	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("em_core_get_account_reference failed");
		return;
	}

	if (ref_account->user_name == NULL) {
		EM_DEBUG_EXCEPTION("invalid user_name...");
		return;
	}
	username = EM_SAFE_STRDUP(ref_account->user_name);

	if (ref_account->password == NULL) {
		EM_SAFE_FREE(username);
		EM_DEBUG_EXCEPTION("invalid password...");
		return;
	}

	password = EM_SAFE_STRDUP(ref_account->password);

	if (username && password && strlen(username) > 0 && strlen(password) > 0) {
		strcpy(user, username);
		strcpy(pwd, password);
	}
	else
		EM_DEBUG_EXCEPTION("User Information is NULL || strlen is 0 ");
		
	EM_SAFE_FREE(username);
	EM_SAFE_FREE(password);

	EM_DEBUG_FUNC_END();
}


EXPORT_API void mm_dlog(char *string)
{
#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG("IMAP_TOOLKIT_DLOG [%s]", string);
	/* Write into debug file
	FILE *fp_dlog = NULL; 
	fp_dlog = fopen("/opt/data/email/.emfdata/core_debug", "a");
    if(fp_dlog) {
		fprintf(fp_dlog, "%s\n", string); 
		fclose(fp_dlog);
	}
	*/
#endif
}

EXPORT_API void mm_log(char *string, long errflg)
{
	/* EM_DEBUG_FUNC_BEGIN(); */
	
	switch ((short)errflg)  {
		case NIL:
			EM_DEBUG_LOG("IMAP_TOOLKIT_LOG NIL [%s]", string);
			break;
			
		case WARN:
			EM_DEBUG_LOG("IMAP_TOOLKIT_LOG WARN [%s]", string);
			break;
			
		case PARSE:
			EM_DEBUG_LOG("IMAP_TOOLKIT_LOG PARSE [%s]", string);
			break;
			
		case BYE:
			EM_DEBUG_LOG("IMAP_TOOLKIT_LOG BYE [%s]", string);
			break;
			
		case TCPDEBUG:
			EM_DEBUG_LOG("IMAP_TOOLKIT_LOG TCPDEBUG [%s]", string);
			break;
			
		case ERROR: {
			emf_session_t *session = NULL;
			
			EM_DEBUG_EXCEPTION("IMAP_TOOLKIT_LOG ERROR [%s]", string);

			em_core_get_current_session(&session);
			
			if (session) {
				mm_get_error(string, &session->error);
				EM_DEBUG_EXCEPTION("IMAP_TOOLKIT_LOG ERROR [%d]", session->error);
			}
			
			/*  Handling exceptional case of connection failures.  */
			/*
			if (string) {
				if (strstr(string, "15 minute") != 0) {
					if (session)
						session->error = EMF_ERROR_LOGIN_ALLOWED_EVERY_15_MINS;
				}
				else if (strstr(string, "Too many login failures") == 0) {
					if (session)
						session->error = EMF_ERROR_TOO_MANY_LOGIN_FAILURE;
				}
			}
			*/
			
			break;
		}
	}
}

EXPORT_API void mm_searched(MAILSTREAM *stream, unsigned long number)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p], number [%d]", stream, number);
	EM_DEBUG_FUNC_END();
}

EXPORT_API void mm_exists(MAILSTREAM *stream, unsigned long number)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p], number [%d]", stream, number);
	EM_DEBUG_FUNC_END();
}

EXPORT_API void mm_expunged(MAILSTREAM *stream, unsigned long number)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p], number [%d]", stream, number);
	EM_DEBUG_FUNC_END();
}

EXPORT_API void mm_flags(MAILSTREAM *stream, unsigned long number)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p], number [%d]", stream, number);
	EM_DEBUG_FUNC_END();
}

EXPORT_API void mm_notify(MAILSTREAM *stream, char *string, long errflg)
{
	EM_DEBUG_FUNC_BEGIN();
	mm_log(string, errflg);
	EM_DEBUG_FUNC_END();
}

EXPORT_API void mm_critical(MAILSTREAM *stream)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p]", stream);
	EM_DEBUG_FUNC_END();
}

EXPORT_API void mm_nocritical(MAILSTREAM *stream)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p]", stream);
	EM_DEBUG_FUNC_END();
}

EXPORT_API long mm_diskerror(MAILSTREAM *stream, long errcode, long serious)
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

EXPORT_API void mm_fatal(char *string)
{
	EM_DEBUG_EXCEPTION("%s", string);
}

EXPORT_API void mm_get_error(char *string, int *err_code)
{
	if (!string || !err_code)
		return ;

	EM_DEBUG_LOG("string [%s]", string);

	if (strstr(string, "login failure") || strstr(string, "Login aborted") || strstr(string, "Can't login"))
		*err_code = EMF_ERROR_LOGIN_FAILURE;
	else if (strstr(string, "Scan not valid"))
		*err_code = EMF_ERROR_MAILBOX_FAILURE;
	else if (strstr(string, "Authentication cancelled"))
		*err_code = EMF_ERROR_AUTHENTICATE;
	else if (strstr(string, "authuser"))
		*err_code = EMF_ERROR_AUTH_NOT_SUPPORTED;
	else if (strstr(string, "negotiate TLS"))
		*err_code = EMF_ERROR_CANNOT_NEGOTIATE_TLS;
	else if (strstr(string, "TLS/SSL failure"))
		*err_code = EMF_ERROR_TLS_SSL_FAILURE;
	else if (strstr(string, "STARTLS"))
		*err_code = EMF_ERROR_STARTLS;
	else if (strstr(string, "TLS unavailable"))
		*err_code = EMF_ERROR_TLS_NOT_SUPPORTED;
	else if (strstr(string, "Can't access"))
		*err_code = EMF_ERROR_APPEND_FAILURE;
	else if (strstr(string, "Can not authenticate"))
		*err_code = EMF_ERROR_AUTHENTICATE;
	else if (strstr(string, "Unexpected IMAP response") || strstr(string, "hello"))
		*err_code = EMF_ERROR_INVALID_RESPONSE;
	else if (strstr(string, "NOTIMAP4REV1"))
		*err_code = EMF_ERROR_COMMAND_NOT_SUPPORTED;
	else if (strstr(string, "Anonymous"))
		*err_code = EMF_ERROR_ANNONYM_NOT_SUPPORTED;
	else if (strstr(string, "connection broken"))
		*err_code = EMF_ERROR_CONNECTION_BROKEN;
	else if (strstr(string, "SMTP greeting"))
		*err_code = EMF_ERROR_NO_RESPONSE;
	else if (strstr(string, "ESMTP failure"))
		*err_code = EMF_ERROR_SMTP_SEND_FAILURE;
	else if (strstr(string, "socket") || strstr(string, "Socket"))
		*err_code = EMF_ERROR_SOCKET_FAILURE;
	else if (strstr(string, "connect to") || strstr(string, "Connection failed"))
		*err_code = EMF_ERROR_CONNECTION_FAILURE;
	else if (strstr(string, "Certificate failure"))
		*err_code = EMF_ERROR_CERTIFICATE_FAILURE;
	else if (strstr(string, "ESMTP failure"))
		*err_code = EMF_ERROR_INVALID_PARAM;
	else if (strstr(string, "No such host"))
		*err_code = EMF_ERROR_NO_SUCH_HOST;
	else if (strstr(string, "host") || strstr(string, "Host"))
		*err_code = EMF_ERROR_INVALID_SERVER;
	else if (strstr(string, "SELECT failed"))
		*err_code = EMF_ERROR_MAILBOX_NOT_FOUND;
	else
		*err_code = EMF_ERROR_UNKNOWN;
}
/* EOF */
