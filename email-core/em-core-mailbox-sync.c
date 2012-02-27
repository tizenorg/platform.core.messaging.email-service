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
 * File :  em-core-mailbox_sync.c
 * Desc :  Mail Header Sync
 *
 * Auth : 
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "em-core-types.h"

#include "c-client.h"
#include "lnx_inc.h"

#include "Msg_Convert.h" 
#include "em-core-mailbox-sync.h"
#include "em-core-global.h"
#include "em-core-imap-mailbox.h"
#include "em-core-event.h"
#include "em-core-mailbox.h"
#include "em-core-mesg.h"
#include "em-core-mime.h"
#include "em-core-utils.h"
#include "em-core-smtp.h"
#include "em-core-account.h" 
#include "em-storage.h"
#include "flstring.h"
#include "emf-dbglog.h"

#define MAX_CHARSET_VALUE 256

static char g_append_uid_rsp[129]; /* added for getting server response  */

extern void imap_parse_body_structure (MAILSTREAM *stream, BODY *body, unsigned char **txtptr, IMAPPARSEDREPLY *reply);

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
static emf_partial_buffer *em_core_get_response_from_server (NETSTREAM *nstream, char *tag, IMAPPARSEDREPLY **reply);
static int em_core_initiate_pbd(MAILSTREAM *stream, int account_id, int mail_id, char *uid, char *mailbox, int *err_code);
#endif

#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
static char g_append_uid_rsp[129]; /* added for getting server response  */
#endif


int pop3_mail_calc_rfc822_size(MAILSTREAM *stream, int msgno, int *size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	POP3LOCAL *pop3local = NULL;
	char command[16];
	char *response = NULL;

	if (!stream || !size) {
		EM_DEBUG_EXCEPTION(" stream[%p], msgno[%d], size[%p]\n", stream, msgno, size);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!(pop3local = stream->local) || !pop3local->netstream) {
		err = EMF_ERROR_INVALID_STREAM;
		goto FINISH_OFF;
	}

	memset(command, 0x00, sizeof(command));
	
	SNPRINTF(command, sizeof(command), "LIST %d\015\012", msgno);

	/* EM_DEBUG_LOG(" [POP3] >>> %s", command); */
	
	/*  send command  :  get rfc822 size by msgno */
	if (!net_sout(pop3local->netstream, command, (int)strlen(command))) {
		EM_DEBUG_EXCEPTION(" net_sout failed...");
		
		err = EMF_ERROR_INVALID_RESPONSE;
		goto FINISH_OFF;
	}
	
	/*  receive response */
	if (!(response = net_getline(pop3local->netstream))) {
		err = EMF_ERROR_CONNECTION_BROKEN;		/* EMF_ERROR_UNKNOWN; */
		goto FINISH_OFF;
	}
	
	/* EM_DEBUG_LOG(" [POP3] <<< %s", response); */
	
	if (*response == '+') {		/*  "+ OK" */
		char *p = NULL;
		
		if (!(p = strchr(response + strlen("+OK "), ' '))) {
			err = EMF_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}
		
		*size = atoi(p + 1);
	}
	else if (*response == '-') {		/*  "- ERR" */
		err = EMF_ERROR_POP3_LIST_FAILURE;
		goto FINISH_OFF;
	}
	else {
		err = EMF_ERROR_INVALID_RESPONSE;
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF: 
	EM_SAFE_FREE(response);
	
	if (err_code  != NULL)
		*err_code = err;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

int imap4_mail_calc_rfc822_size(MAILSTREAM *stream, int msgno, int *size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	IMAPLOCAL *imaplocal = NULL;
	char tag[32], command[128];
	char *response = NULL;

	if (!stream || !size) {
		EM_DEBUG_EXCEPTION("stream[%p], msgno[%d], size[%p]", stream, msgno, size);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!(imaplocal = stream->local) || !imaplocal->netstream) {
		err = EMF_ERROR_INVALID_STREAM;
		goto FINISH_OFF;
	}

	memset(tag, 0x00, sizeof(tag));
	memset(command, 0x00, sizeof(command));

	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
	SNPRINTF(command, sizeof(command), "%s FETCH %d RFC822.SIZE\015\012", tag, msgno);

	/* EM_DEBUG_LOG(" [IMAP4] >>> %s", command); */
	
	/*  send command  :  get rfc822 size by msgno */
	if (!net_sout(imaplocal->netstream, command, (int)strlen(command))) {
		EM_DEBUG_EXCEPTION(" net_sout failed...");
		
		err = EMF_ERROR_INVALID_RESPONSE;
		goto FINISH_OFF;
	}
	
	while (imaplocal->netstream) {
		char *s = NULL;
		char *t = NULL;
		
		/*  receive response */
		if (!(response = net_getline(imaplocal->netstream)))
			break;
		
		/* EM_DEBUG_LOG(" [IMAP4] <<< %s", response); */
		
		if (!strncmp(response, tag, strlen(tag))) {
			if (!strncmp(response + strlen(tag) + 1, "OK", 2)) {
				EM_SAFE_FREE(response);
				break;
			}
			else {		/*  'NO' or 'BAD' */
				err = EMF_ERROR_IMAP4_FETCH_SIZE_FAILURE;		/* EMF_ERROR_INVALID_RESPONSE; */
				goto FINISH_OFF;
			}
		}
		else {		/*  untagged response */
			if (*response == '*') {
				if (!(t = strstr(response, "FETCH (RFC822.SIZE "))) {
					EM_SAFE_FREE(response);
					continue;
				}
				
				s = t + strlen("FETCH (RFC822.SIZE ");
				
				if (!(t = strchr(s, ' '))) {
					err = EMF_ERROR_INVALID_RESPONSE;
					goto FINISH_OFF;
				}
				
				*t = '\0';
				
				*size = atoi(s);
			}
		}
		
		EM_SAFE_FREE(response);
	}
	
	ret = true;
	
FINISH_OFF: 
	EM_SAFE_FREE(response);
	
	if (err_code  != NULL)
		*err_code = err;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

int pop3_mailbox_get_uids(MAILSTREAM *stream, em_core_uid_list** uid_list, int *err_code)
{
	EM_PROFILE_BEGIN(pop3MailboxGetuid);
	EM_DEBUG_FUNC_BEGIN("stream[%p], uid_list[%p], err_code[%p]", stream, uid_list, err_code);

	int ret = false;
	int err = EMF_ERROR_NONE;
	
	POP3LOCAL *pop3local = NULL;
	char command[64];
	char *response = NULL;
	em_core_uid_list *uid_elem = NULL;
	
	if (!stream || !uid_list) {
		EM_DEBUG_EXCEPTION("stream[%p], uid_list[%p]n", stream, uid_list);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!(pop3local = stream->local) || !pop3local->netstream) {
		EM_DEBUG_EXCEPTION("invalid POP3 stream detected...");
		err = EMF_ERROR_INVALID_STREAM;
		goto FINISH_OFF;
	}
	
	memset(command, 0x00, sizeof(command));
	
	SNPRINTF(command, sizeof(command), "UIDL\015\012");
	
#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG(" [POP3] >>> [%s]", command);
#endif
	
	/*  send command  :  get msgno/uid for all message */
	if (!net_sout(pop3local->netstream, command, (int)strlen(command))) {
		EM_DEBUG_EXCEPTION("net_sout failed...");
		err = EMF_ERROR_CONNECTION_BROKEN;		/* EMF_ERROR_UNKNOWN; */
		goto FINISH_OFF;
	}
	
	*uid_list = NULL;
	
	while (pop3local->netstream) {
		char *p = NULL;
		
		/*  receive response */
		if (!(response = net_getline(pop3local->netstream))) {
			EM_DEBUG_EXCEPTION("net_getline failed...");
			err = EMF_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}
		
#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG(" [POP3] <<< [%s]", response);
#endif
		
		if (*response == '-') {		/*  "-ERR" */
			err = EMF_ERROR_POP3_UIDL_FAILURE;		/* EMF_ERROR_INVALID_RESPONSE; */
			goto FINISH_OFF;
		}
		
		if (*response == '+') {		/*  "+OK" */
			free(response); response = NULL;
			continue;
		}
		
		if (*response == '.') {
			free(response); response = NULL;
			break;
		}
		
		if ((p = strchr(response, ' '))) { 
			*p = '\0';
			
			if (!(uid_elem = em_core_malloc(sizeof(em_core_uid_list)))) {
				EM_DEBUG_EXCEPTION("malloc failed...");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			uid_elem->msgno = atoi(response);
			uid_elem->uid = EM_SAFE_STRDUP(p + 1);
			
			if (*uid_list  != NULL)
				uid_elem->next = *uid_list;		/*  prepend new data to table */
			
			*uid_list = uid_elem;
		}
		else {
			err = EMF_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}
		
		free(response); response = NULL;
	}
	
	ret = true;

FINISH_OFF: 
	if (response  != NULL)
		free(response);
	
	if (err_code  != NULL)
		*err_code = err;
	
	EM_PROFILE_END(pop3MailboxGetuid);
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

int imap4_mailbox_get_uids(MAILSTREAM *stream, em_core_uid_list** uid_list, int *err_code)
{
	EM_PROFILE_BEGIN(ImapMailboxGetUids);
	EM_DEBUG_FUNC_BEGIN("stream[%p], uid_list[%p], err_code[%p]", stream, uid_list, err_code);

	int ret = false;
	int err = EMF_ERROR_NONE;
	
	IMAPLOCAL *imaplocal = NULL;
	char tag[16], command[64];
	char *response = NULL;
	em_core_uid_list *uid_elem = NULL;
	
	if (!stream || !uid_list) {
		EM_DEBUG_EXCEPTION("stream[%p], uid_list[%p]", stream, uid_list);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!(imaplocal = stream->local) || !imaplocal->netstream) {
		EM_DEBUG_EXCEPTION("invalid IMAP4 stream detected...");
		err = EMF_ERROR_INVALID_PARAM;		/* EMF_ERROR_UNKNOWN */
		goto FINISH_OFF;
	}
	
	if (stream->nmsgs == 0){
		err = EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER;
		goto FINISH_OFF;
	}
	memset(tag, 0x00, sizeof(tag));
	memset(command, 0x00, sizeof(command));
	
	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
	SNPRINTF(command, sizeof(command), "%s FETCH 1:* (FLAGS UID)\015\012", tag);		/* TODO :  confirm me */
	EM_DEBUG_LOG("COMMAND [%s] \n", command);
#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG(" [IMAP4] >>> [%s]", command);
#endif
	
	/*  send command  :  get msgno/uid for all message */
	if (!net_sout(imaplocal->netstream, command, (int)strlen(command))) {
		EM_DEBUG_EXCEPTION(" net_sout failed...\n");
		err = EMF_ERROR_CONNECTION_BROKEN;	
		goto FINISH_OFF;
	}
	
	*uid_list = NULL;
	
	while (imaplocal->netstream) {
		char *p = NULL;
		char *s = NULL;
		int seen = 0;
		int forwarded = 0;	
		int draft = 0;
		/*  receive response */
		if (!(response = net_getline(imaplocal->netstream))) {
			EM_DEBUG_EXCEPTION("net_getline failed...");
			err = EMF_ERROR_INVALID_RESPONSE;		
			goto FINISH_OFF;
		}
		
#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG(" [IMAP4] <<< [%s]", response);
#endif
		
		if (!strncmp(response, tag, strlen(tag))) {
			if (!strncmp(response + strlen(tag) + 1, "OK", 2)) {
				free(response); response = NULL;
				break;
			}
			else {		/*  'NO' or 'BAD' */
				err = EMF_ERROR_IMAP4_FETCH_UID_FAILURE;		/* EMF_ERROR_INVALID_RESPONSE; */
				goto FINISH_OFF;
			}
		}
		
		if ((p = strstr(response, " FETCH ("))) { 
			if (!strstr(p, "\\Deleted")) {	/*  undeleted only */
				*p = '\0'; p  += strlen(" FETCH ");
				
				seen = strstr(p, "\\Seen") ? 1  :  0;
				draft = strstr(p, "\\Draft") ? 1  :  0;
				forwarded = strstr(p, "$Forwarded") ? 1  :  0;
				
				if ((p = strstr(p, "UID "))) {
					s = p + strlen("UID ");
					
					while (isdigit(*s))
						s++;
					
					*s = '\0';
					
					if (!(uid_elem = em_core_malloc(sizeof(em_core_uid_list)))) {
						EM_DEBUG_EXCEPTION("em_core_malloc failed...");
						err = EMF_ERROR_OUT_OF_MEMORY;
						goto FINISH_OFF;
					}
					
					uid_elem->msgno = atoi(response + strlen("* "));
					uid_elem->uid = EM_SAFE_STRDUP(p + strlen("UID "));
					uid_elem->flag.seen = seen;
					uid_elem->flag.draft = draft;
					uid_elem->flag.forwarded = forwarded;
					if (*uid_list  != NULL)
						uid_elem->next = *uid_list;		/*  prepend new data to list */
					
					*uid_list = uid_elem;
				}
				else {
					err = EMF_ERROR_INVALID_RESPONSE;
					goto FINISH_OFF;
				}
			}
		}
		else {
			err = EMF_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}
		
		EM_SAFE_FREE(response);;
	}
	
	ret = true;

FINISH_OFF: 
	EM_SAFE_FREE(response);

	if (err_code  != NULL)
		*err_code = err;
	
	EM_PROFILE_END(ImapMailboxGetUids);
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

static char *__em_get_month_in_string(int month)
{
	EM_DEBUG_FUNC_BEGIN("month [%d]", month);

	char *mon = NULL;

	switch (month){
	    case 0:
			mon = EM_SAFE_STRDUP("jan");
	    	break;
	    case 1:
			mon = EM_SAFE_STRDUP("feb");
	    	break;
	    case 2:
			mon = EM_SAFE_STRDUP("mar");
	    	break;
	    case 3:
			mon = EM_SAFE_STRDUP("apr");
	    	break;
	    case 4:
			mon = EM_SAFE_STRDUP("may");
	    	break;
	    case 5:
			mon = EM_SAFE_STRDUP("jun");
	    	break;
	    case 6:
			mon = EM_SAFE_STRDUP("jul");
	    	break;
	    case 7:
			mon = EM_SAFE_STRDUP("aug");
	    	break;
	    case 8:
			mon = EM_SAFE_STRDUP("sep");
	    	break;
	    case 9:
			mon = EM_SAFE_STRDUP("oct");
	    	break;
	    case 10:
			mon = EM_SAFE_STRDUP("nov");
	    	break;
	    case 11:
			mon = EM_SAFE_STRDUP("dec");
	    	break;
	}
	return mon;
}
	
int imap4_mailbox_get_uids_by_timestamp(MAILSTREAM *stream, em_core_uid_list** uid_list,  int *err_code)
{
	EM_PROFILE_BEGIN(emCoreMailboxuidsbystamp);
	EM_DEBUG_FUNC_BEGIN("stream[%p], uid_list[%p],  err_code[%p]", stream, uid_list, err_code);

	int ret = false;
	int err = EMF_ERROR_NONE;
	
	IMAPLOCAL *imaplocal = NULL;
	char tag[16], command[64];
	char *response = NULL;
	em_core_uid_list *uid_elem = NULL;
	char delims[] = " ";
	char *result = NULL;

	struct tm   *timeinfo = NULL;
	time_t         RawTime = 0;
	time_t         week_before_RawTime = 0;	
	char  date_string[16];
	char *mon = NULL;
	
	if (!stream || !uid_list) {
		EM_DEBUG_EXCEPTION(" stream[%p], uid_list[%p]", stream, uid_list);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!(imaplocal = stream->local) || !imaplocal->netstream) {
		EM_DEBUG_EXCEPTION(" invalid IMAP4 stream detected...");
		err = EMF_ERROR_INVALID_PARAM;		/* EMF_ERROR_UNKNOWN */
		goto FINISH_OFF;
	}

	/* Fetch the System time and Retrieve the a Week before time */
	/* 	tzset(); */
	time(&RawTime);

	EM_DEBUG_LOG("RawTime Info [%lu]", RawTime);

	timeinfo = localtime (&RawTime);

	EM_DEBUG_LOG(">>>>>Current TIme %d %d %d %d %d %d", 1900+timeinfo->tm_year, timeinfo->tm_mon+1, timeinfo->tm_mday);

	week_before_RawTime = RawTime - 604800;

	/* Reading the current timeinfo */
	timeinfo = localtime (&week_before_RawTime);

	EM_DEBUG_LOG(">>>>>Mobile Date a Week before %d %d %d %d %d %d", 1900 + timeinfo->tm_year, timeinfo->tm_mon+1, timeinfo->tm_mday);

	memset(&date_string, 0x00, 16);

	mon = __em_get_month_in_string(timeinfo->tm_mon);

	if (mon){
		snprintf(date_string, 16, "%d-%s-%04d", timeinfo->tm_mday, mon, 1900 + timeinfo->tm_year);
		EM_DEBUG_LOG("DATE IS [ %s ] ", date_string);
		EM_SAFE_FREE(mon);
	}
	
	memset(tag, 0x00, sizeof(tag));
	memset(command, 0x00, sizeof(command));
	
	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));

	/* SNPRINTF(command, sizeof(command), "%s UID SEARCH 1:* SINCE 17-Aug-2009\015\012", tag, date_string); */		/* TODO :  confirm me */

	SNPRINTF(command, sizeof(command), "%s UID SEARCH 1:* SINCE %s\015\012", tag, date_string);		/* TODO :  confirm me */
	
	EM_DEBUG_LOG("COMMAND [%s] ", command);

	
#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG(" [IMAP4] >>> [%s]", command);
#endif

	/*  send command  :  get msgno/uid for all message */
	if (!net_sout(imaplocal->netstream, command, (int)strlen(command))) {
		EM_DEBUG_EXCEPTION(" net_sout failed...");
		err = EMF_ERROR_CONNECTION_BROKEN;		/* EMF_ERROR_UNKNOWN */
		goto FINISH_OFF;
	}

	*uid_list = NULL;

	while (imaplocal->netstream) {
		char *p = NULL;
		/*  receive response */
		if (!(response = net_getline(imaplocal->netstream))) {
			EM_DEBUG_EXCEPTION(" net_getline failed...");
			err = EMF_ERROR_INVALID_RESPONSE;		/* EMF_ERROR_UNKNOWN; */
			goto FINISH_OFF;
		}
#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG(" [IMAP4] <<< [%s]", response);
#endif

		if (!strncmp(response, tag, strlen(tag))) {
			if (!strncmp(response + strlen(tag) + 1, "OK", 2)) {
				free(response); response = NULL;
				break;
			}
			else {	/*  'NO' or 'BAD' */
				err = EMF_ERROR_IMAP4_FETCH_UID_FAILURE;		/* EMF_ERROR_INVALID_RESPONSE; */
				goto FINISH_OFF;
			}
		}

		if ((p = strstr(response, " SEARCH "))){
		    *p = '\0'; p  += strlen(" SEARCH ");

		    result = strtok(p, delims);

		    while (result  != NULL)
		    {
				EM_DEBUG_LOG("UID VALUE DEEP is [%s]", result);

				if (!(uid_elem = em_core_malloc(sizeof(em_core_uid_list)))) {
					EM_DEBUG_EXCEPTION(" malloc failed...");
					err = EMF_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}

				uid_elem->uid = EM_SAFE_STRDUP(result);

				if (*uid_list  != NULL)
					uid_elem->next = *uid_list;
				*uid_list = uid_elem;
				result = strtok(NULL, delims);
		    }

			EM_SAFE_FREE(response);
		    return 1;
		}
		else {
			err = EMF_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}
		
		free(response); response = NULL;
	}

	ret = true;

FINISH_OFF: 
	if (response  != NULL)
		free(response);

	if (err_code  != NULL)
		*err_code = err;
	EM_PROFILE_END(emCoreMailboxuidsbystamp);
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

#define PARSE_BUFFER_LENGTH 4096
static int em_core_mailbox_parse_header(char *rfc822_header, int *req_read_receipt, int *priority, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("rfc822_header[%p], req_read_receipt[%p], priority[%p], err_code[%p]", rfc822_header, req_read_receipt, priority,  err_code);

	if (!rfc822_header || !priority) 
		return false;

	if (err_code)
		*err_code = EMF_ERROR_NONE;

	EM_PROFILE_BEGIN(emCoreMailboxParseHeader);
	
	/*  char *buf = NULL; */
	char buf[PARSE_BUFFER_LENGTH];
	int len, i, j;
	/* int buf_length = strlen(rfc822_header)+1; */
	
	EM_DEBUG_LOG("Buffer length [%d]", PARSE_BUFFER_LENGTH);
	/*
	buf = (char *) malloc(sizeof(char) * (buf_length));
	
	if (!buf) {
		EM_DEBUG_EXCEPTION("buf is NULL");
		return false;
	}
	*/
	*priority = 3;
		
	memset(buf, 0x00, PARSE_BUFFER_LENGTH);
	
	for (len = strlen(rfc822_header), i = 0, j = 0; i < len; i++) {
		if (rfc822_header[i] == CR && rfc822_header[i+1] == LF){
			if (j + 3 < PARSE_BUFFER_LENGTH) /* '3' include CR LF NULL */
				strncpy(buf + j, CRLF_STRING, PARSE_BUFFER_LENGTH - (j + 2)); /* '3' include CR LF */ 
			else
				EM_DEBUG_EXCEPTION("buf is too small.");

			i++;
			j = 0;
			
			/*  parsing data */
			em_core_upper_string(buf);
			
			/*  message-id */
/* 			if (buf[0] == 'M' && buf[3] == 'S' && buf[6] == 'E' && buf[7] == '-' && buf[8] == 'I') { */
/* 				buf[strlen(buf)-2] = '\0'; */
/* 				if (buf[12])	*message_id = EM_SAFE_STRDUP(buf+12); */
/* 				memset(buf, 0x00, sizeof(buf)); */
/* 				continue; */
/* 			} */
			
			/*  disposition_notification_to */
			if (buf[0] == 'D' && buf[11] == '-' && buf[12] == 'N' && buf[24] == '-' && buf[25] == 'T') {
				if (req_read_receipt) 
					*req_read_receipt = 1;
				memset(buf, 0x00, PARSE_BUFFER_LENGTH);
				continue;
			}
			
			/*  x-priority */
			if (buf[0] == 'X' && buf[2] == 'P' && buf[9] == 'Y'){
				size_t len_2 = strlen(buf);
				if (len_2 >= 12){	
					buf[len_2 - 2] = '\0';
					*priority = atoi(buf + 11);
					memset(buf, 0x00, PARSE_BUFFER_LENGTH);
					continue;
				}
			}
			
			/*  x-msmail-priority */
			if (buf[0] == 'X' && buf[2] == 'M' && buf[9] == 'P' && buf[16] == 'Y'){
				if (strstr(buf, "HIGH")) 
					*priority = 1;
				if (strstr(buf, "NORMAL")) 
					*priority = 3;
				if (strstr(buf, "LOW")) 
					*priority = 5;
				memset(buf, 0x00, PARSE_BUFFER_LENGTH);
				continue;
			}
			
			memset(buf, 0x00, PARSE_BUFFER_LENGTH);
			continue;
		}
		
		if (j + 1 < PARSE_BUFFER_LENGTH)
			buf[j++] = rfc822_header[i];
		else
			EM_DEBUG_EXCEPTION("buf is too small.");
	}
	
	/*
    EM_SAFE_FREE(buf);		
	*/
	EM_PROFILE_END(emCoreMailboxParseHeader);

	if (err_code)
		*err_code = EMF_ERROR_NONE;

	return true;
}


static int em_core_mail_get_extra_info(MAILSTREAM *stream, int msgno, int *req_read_receipt, int *priority, int *err_code)
{
	EM_PROFILE_BEGIN(emCoreMailGetExtraInfo);
	EM_DEBUG_FUNC_BEGIN("stream[%p], msgno[%d], req_read_receipt[%p], priority[%p], err_code[%p]", stream, msgno, req_read_receipt, priority, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	char *rfc822_header = NULL;
	unsigned long len = 0;
	
	EM_PROFILE_BEGIN(MaiFetchHeader);
#ifdef __FEATURE_HEADER_OPTIMIZATION__
	/* Check if header already available in cache */
	if (stream && stream->cache && stream->cache[msgno-1]->private.msg.header.text.data){
		EM_DEBUG_LOG("I found the header in stream->cache!!");
		rfc822_header = (char *) stream->cache[msgno-1]->private.msg.header.text.data;
	}
	else{
		EM_DEBUG_LOG("I couldn't find the header. I'll fetch the header from server again.");
		if (stream)
			rfc822_header = mail_fetch_header(stream, msgno, NULL, NULL, &len, FT_PEEK);
	}

#else
	rfc822_header = mail_fetch_header(stream, msgno, NULL, NULL, &len, FT_PEEK);
#endif
	EM_PROFILE_END(MaiFetchHeader);

	if (!rfc822_header || !*rfc822_header) {
		EM_DEBUG_EXCEPTION("mail_fetch_header failed...");
		err = EMF_ERROR_IMAP4_FETCH_UID_FAILURE;		/* EMF_ERROR_UNKNOWN; */
		goto FINISH_OFF;
	}
	
	if (!em_core_mailbox_parse_header(rfc822_header, req_read_receipt, priority, &err)) {
		EM_DEBUG_EXCEPTION("em_core_mailbox_parse_header falied - %d", err);
		goto FINISH_OFF;
	}
	ret = true;
	
FINISH_OFF: 
	
	if (err_code  != NULL)
		*err_code = err;

	EM_PROFILE_END(emCoreMailGetExtraInfo);
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

static int em_core_mailbox_get_uids_to_download(MAILSTREAM *stream, emf_account_t *account, emf_mailbox_tbl_t *input_mailbox_tbl, int limit_count, em_core_uid_list** uid_list, int *uids, int retrieve_mode ,  int *err_code)
{
	EM_PROFILE_BEGIN(emCoreGetUidsDownload);
	EM_DEBUG_FUNC_BEGIN("account[%p], input_mailbox_tbl[%p], limit_count[%d], uid_list[%p], err_code[%p]", account, input_mailbox_tbl, limit_count, uid_list, err_code);

	int ret = false;
	int err = EMF_ERROR_NONE;
	
	emf_mail_read_mail_uid_tbl_t *downloaded_uids = NULL;
	emf_mail_info_t *mail_info = NULL;
	int i = 0, j = 0, uid_count = 0, uid_to_be_downloaded_count = 0;		
	em_core_uid_list *uid_elem = NULL;
	em_core_uid_list *head_uid_elem = NULL, *end =  NULL;
	em_core_uid_list *next_uid_elem = NULL;
	emf_mail_tbl_t *mail = NULL;
	
	if (!account || !input_mailbox_tbl || !uid_list) {
		EM_DEBUG_EXCEPTION("account[%p], input_mailbox_tbl[%p], uid_list[%p]", account, input_mailbox_tbl, uid_list);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	*uid_list = NULL;
  
	if (account->receiving_server_type == EMF_SERVER_TYPE_POP3) {
		if (!pop3_mailbox_get_uids(stream, uid_list, &err)) {
			EM_DEBUG_EXCEPTION("pop3_mailbox_get_uids failed - %d", err);
			goto FINISH_OFF;
		}
	}
	else {	/*  EMF_SERVER_TYPE_IMAP4 */
		/*  sowmya.kr commented , since imap4_mailbox_get_uids_by_timestamp will fetch mails since last week and not all mails  */
				
		EM_DEBUG_LOG("calling imap4_mailbox_get_uids");
		if (!imap4_mailbox_get_uids(stream, uid_list, &err)) {
			EM_DEBUG_EXCEPTION("imap4_mailbox_get_uids failed [%d]", err);
			if (err  != EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER)
				goto FINISH_OFF;
	    }
	}
	
	if (!em_storage_get_downloaded_list(input_mailbox_tbl->account_id, input_mailbox_tbl->mailbox_name, &downloaded_uids, &j, true, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_get_downloaded_list failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("Number of Mails in Downloaded Table [%d]", j);

	uid_elem = *uid_list;
	uid_count = 0;

	if(!uid_elem) { /* If there is no mail in the input_mailbox_tbl, remove all mails in the input_mailbox_tbl */
		for (i = 0; i < j; i++) {
			downloaded_uids[i].reserved = 0;
		}
	}
	
	EM_PROFILE_BEGIN(emCoreGetUidsDownloadWhilwLoop);	
	
	while (uid_elem) {
		next_uid_elem = uid_elem->next;

		if ((account->retrieval_mode == EMF_IMAP4_RETRIEVAL_MODE_NEW) && (uid_elem->flag.seen != 0)){		/*  already seen */
			if (uid_elem->uid)
				free(uid_elem->uid);
			
			free(uid_elem);
		}
		else {
			int to_be_downloaded = 1;
			
			if (limit_count > 0 && uid_count >= limit_count){
				/*  EM_DEBUG_LOG("hit the limit[%d] for [%s]", limit_count, uid_elem->uid);		 */
				to_be_downloaded = 0;	
			}
			else{
				for (i = j; i > 0; i--) {
					if (downloaded_uids[i - 1].reserved == 0 && !strcmp(uid_elem->uid, downloaded_uids[i - 1].s_uid)) {
						downloaded_uids[i - 1].reserved = uid_elem->flag.seen ? 2 : 1;
						to_be_downloaded = 0;
						break;
					}
				}
			}

			/*  EM_DEBUG_LOG("Is uid[%s] going to be downloded ? [%d]", uid_elem->uid, to_be_downloaded); */
			
			if (to_be_downloaded) {		
				if (retrieve_mode == EMF_SYNC_OLDEST_MAILS_FIRST){
					uid_elem->next = head_uid_elem;
					head_uid_elem = uid_elem;
				}
				else{	/* if retrieve_mode is EMF_SYNC_LATEST_MAILS_FIRST, add uid elem to end so that latest mails are in front of list */
					if (head_uid_elem == NULL){
						uid_elem->next = head_uid_elem;
						head_uid_elem = uid_elem;						
						end = head_uid_elem;
					}
					else{
				  		end->next = uid_elem;
						uid_elem->next = NULL;
						end = uid_elem;					
					}
				}
				uid_to_be_downloaded_count++;
				
			}
			else {
				if (uid_elem->uid)
					free(uid_elem->uid);
				free(uid_elem);
			}
			
			uid_count++;
		}

		uid_elem = next_uid_elem;
	}

	EM_PROFILE_END(emCoreGetUidsDownloadWhilwLoop);	
	EM_PROFILE_BEGIN(emCoreGetUidsDownloadForLoop);	
	
	for (i = 0; i < j; i++) {
		/*  EM_DEBUG_LOG("input_mailbox_tbl[%s] reserved[%d]", input_mailbox_tbl->name, downloaded_uids[i].reserved); */
		if (downloaded_uids[i].reserved == 0) {		/*  deleted on server */
			if (!em_storage_get_maildata_by_servermailid(input_mailbox_tbl->account_id, downloaded_uids[i].s_uid, &mail, true, &err)){
				EM_DEBUG_EXCEPTION("em_storage_get_maildata_by_servermailid for uid[%s] Failed [%d] \n ", downloaded_uids[i].s_uid, err);
				if (err == EM_STORAGE_ERROR_MAIL_NOT_FOUND){
					continue;
				}
			}
						
      if (account->receiving_server_type == EMF_SERVER_TYPE_IMAP4) {
				if (!em_core_mail_delete_from_local(input_mailbox_tbl->account_id, &(mail->mail_id), 1, EMF_DELETED_FROM_SERVER, false, &err)) {
					EM_DEBUG_EXCEPTION("em_core_mail_delete_from_local falied - %d", err);
					goto FINISH_OFF;
				}
				/* em_core_delete_notification_for_read_mail(mail->mail_id); */
				em_core_check_unread_mail(); 
			}
			
			if (!em_storage_remove_downloaded_mail(input_mailbox_tbl->account_id, input_mailbox_tbl->mailbox_name, downloaded_uids[i].s_uid, true, &err)) {   /*  remove uid from uid list */
				EM_DEBUG_EXCEPTION("em_storage_remove_downloaded_mail failed - %d", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				/* goto FINISH_OFF; */
			}
			
		}
		else if (account->receiving_server_type == EMF_SERVER_TYPE_IMAP4 && downloaded_uids[i].reserved == 1) {	
			/*  unseen on server */
			if  (!em_core_mail_get_info(downloaded_uids[i].local_uid, &mail_info, &err)){
				EM_DEBUG_EXCEPTION("em_core_mail_get_info failed for [%d] - [%d]", downloaded_uids[i].local_uid,  err);
				continue;
			}
			
			if (mail_info) {		
				if (mail_info->body_downloaded && mail_info->flags.seen){
					EM_DEBUG_LOG("Set flag as seen on server");
					mail_setflag_full(stream, downloaded_uids[i].s_uid, "\\Seen", ST_UID);
				}
				em_core_mail_info_free(&mail_info, 1, NULL);
				mail_info = NULL;
			}

		}
	}
	EM_PROFILE_END(emCoreGetUidsDownloadForLoop);	
	
	*uid_list = head_uid_elem;
	*uids = uid_to_be_downloaded_count;
	
	ret = true;
	
FINISH_OFF: 
	if (ret == false){
		if (head_uid_elem)
			em_core_mailbox_free_uids(head_uid_elem, NULL);
	}

	if (mail_info  != NULL)
		em_core_mail_info_free(&mail_info, 1, NULL);
	
	if (downloaded_uids  != NULL)
		em_storage_free_read_mail_uid(&downloaded_uids, j, NULL);

	if (mail  != NULL)
		em_storage_free_mail(&mail, 1, NULL);
	
	if (err_code  != NULL)
		*err_code = err;
	
	EM_PROFILE_END(emCoreGetUidsDownload);
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

/* insert received mail UID to read mail uid table */
static int em_core_mailbox_add_read_mail_uid(emf_mailbox_tbl_t *input_maibox_data, char *server_mailbox_name, int mail_id, char *uid, int rfc822_size, int rule_id, int *err_code)
{
	EM_PROFILE_BEGIN(emCoreMailboxAddReadMailUid);
	EM_DEBUG_FUNC_BEGIN("input_maibox_data[%p], server_mailbox_name[%s], uid[%s], rfc822_size[%d], rule_id[%d], err_code[%p]", input_maibox_data, server_mailbox_name, uid, rfc822_size, rule_id, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	emf_mail_read_mail_uid_tbl_t read_mail_uid = { 0 };
	char *mailbox_name = NULL;
	
	if (!input_maibox_data || !uid) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	read_mail_uid.account_id = input_maibox_data->account_id;
	
	if (!(input_maibox_data->mailbox_name) || !(server_mailbox_name)){
		if (!em_storage_get_mailboxname_by_mailbox_type(input_maibox_data->account_id, EMF_MAILBOX_TYPE_INBOX, &mailbox_name, false, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
	}
	
	if (input_maibox_data->mailbox_name) 
		read_mail_uid.local_mbox = input_maibox_data->mailbox_name;
	else
		read_mail_uid.local_mbox = mailbox_name;
	
	read_mail_uid.local_uid = mail_id;
	EM_DEBUG_LOG("MAIL ID [%d] LOCAL_UID [%d]", mail_id, read_mail_uid.local_uid); 
	
	if (server_mailbox_name)
		read_mail_uid.mailbox_name = server_mailbox_name;
	else
		read_mail_uid.mailbox_name = mailbox_name;
	
	read_mail_uid.s_uid = uid;
	read_mail_uid.data1 = rfc822_size;
	read_mail_uid.flag = rule_id;
	
	if (!em_storage_add_downloaded_mail(&read_mail_uid, false, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_add_downloaded_mail failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF: 
	EM_SAFE_FREE(mailbox_name);

	if (err_code)
		*err_code = err;
	
	EM_PROFILE_END(emCoreMailboxAddReadMailUid);
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

static int em_core_mail_add_header(MAILSTREAM *mail_stream, emf_mailbox_tbl_t *input_maibox_data, emf_mail_head_t *head, int to_num, em_core_uid_list *uid_elem, int *mail_id, int *result_thread_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_stream [%p], input_maibox_data[%p], head[%p], to_num[%d], uid_elem[%p], mail_id[%p], err_code[%p]", mail_stream, input_maibox_data, head, to_num, uid_elem, mail_id, err_code);
	
	int ret = false, err = EMF_ERROR_NONE;
	int req_read_receipt = 0;
	int priority = 0;
	int thread_id = -1, thread_item_count = 0, latest_mail_id_in_thread = -1;
	char datetime[16] = { 0, };
	MESSAGECACHE *elt = NULL;
	emf_mail_tbl_t mail_table_data = {0};

	if (!input_maibox_data || !mail_stream || !head || !uid_elem) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}	
	
	if (!(elt = mail_elt(mail_stream, uid_elem->msgno))) {
		EM_DEBUG_EXCEPTION("mail_elt failed...");
		err = EMF_ERROR_UNKNOWN;
		goto FINISH_OFF;
	}
	
	if (!em_core_mail_get_extra_info(mail_stream, uid_elem->msgno, &req_read_receipt, &priority, &err)) {
		EM_DEBUG_EXCEPTION("em_core_mail_get_extra_info failed - %d", err);
		goto FINISH_OFF;
	}

	SNPRINTF(datetime, sizeof(datetime), "%04d%02d%02d%02d%02d%02d", BASEYEAR + elt->year, elt->month, elt->day, elt->hours, elt->minutes, elt->seconds);

	head->datetime.year = elt->year;
	head->datetime.month = elt->month;
	head->datetime.day = elt->day;
	head->datetime.hour = elt->hours;
	head->datetime.minute = elt->minutes;
	head->datetime.second = elt->seconds;

	mail_table_data.account_id            = input_maibox_data->account_id;
	mail_table_data.mailbox_name          = input_maibox_data->mailbox_name;
	mail_table_data.mailbox_type          = input_maibox_data->mailbox_type;
	mail_table_data.subject               = head->subject;
	mail_table_data.datetime              = EM_SAFE_STRDUP(datetime);
	mail_table_data.server_mail_status    = 1;
	mail_table_data.server_mail_id        = uid_elem->uid;
	mail_table_data.message_id            = head->mid;
	mail_table_data.full_address_from     = head->from;
	mail_table_data.full_address_to       = head->to;
	mail_table_data.full_address_cc       = head->cc;
	mail_table_data.full_address_bcc      = head->bcc;
	mail_table_data.full_address_reply    = head->reply_to;
	mail_table_data.full_address_return   = head->return_path;
	mail_table_data.mail_size             = elt->rfc822_size;
	mail_table_data.flags_seen_field      = uid_elem->flag.seen;
	mail_table_data.flags_deleted_field   = elt->deleted;
	mail_table_data.flags_flagged_field   = elt->flagged;
	mail_table_data.flags_answered_field  = elt->answered;
	mail_table_data.flags_recent_field    = elt->recent;
	mail_table_data.flags_draft_field     = elt->draft;
	mail_table_data.flags_forwarded_field = uid_elem->flag.forwarded;
	mail_table_data.priority              = priority;
	mail_table_data.report_status         = (req_read_receipt ? 3  :  0);
	mail_table_data.attachment_count      = 0;

	em_core_fill_address_information_of_mail_tbl(&mail_table_data);

	em_storage_begin_transaction(NULL, NULL, NULL);

	/* Get the Mail_id */
	if (!em_storage_increase_mail_id(&(mail_table_data.mail_id), false, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_increase_mail_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	if (em_storage_get_thread_id_of_thread_mails(&mail_table_data, &thread_id, &latest_mail_id_in_thread, &thread_item_count)  != EMF_ERROR_NONE)
		EM_DEBUG_LOG(" em_storage_get_thread_id_of_thread_mails is failed.");

	if (thread_id == -1){
		mail_table_data.thread_id = mail_table_data.mail_id;
		mail_table_data.thread_item_count = thread_item_count = 1;
	}
	else {
		mail_table_data.thread_id = thread_id;
		thread_item_count++;
	}

	if (result_thread_id)
		*result_thread_id = mail_table_data.thread_id;

	EM_DEBUG_LOG("mail_table_data.mail_id [%d]", mail_table_data.mail_id);
	EM_DEBUG_LOG("mail_table_data.thread_id [%d]", mail_table_data.thread_id);
	
	if (!em_storage_add_mail(&mail_table_data, 0, false, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_add_mail failed [%d]", err);
		em_storage_rollback_transaction(NULL, NULL, NULL);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
		
	if (thread_item_count > 1){
		if (!em_storage_update_latest_thread_mail(mail_table_data.account_id, mail_table_data.thread_id, 0, 0, false, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_update_latest_thread_mail failed [%d]", err);
			em_storage_rollback_transaction(NULL, NULL, NULL);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
	}
	
	if (mail_id  != NULL)
		*mail_id = mail_table_data.mail_id;
	
	if (!em_core_mailbox_add_read_mail_uid(input_maibox_data, input_maibox_data->mailbox_name, mail_table_data.mail_id, uid_elem->uid, mail_table_data.mail_size, 0, &err)) {
		EM_DEBUG_EXCEPTION("em_core_mailbox_add_read_mail_uid failed [%d]", err);
		em_storage_rollback_transaction(NULL, NULL, NULL);
		goto FINISH_OFF;
	}
	
	em_storage_commit_transaction(NULL, NULL, NULL);
	ret = true;
	
FINISH_OFF: 

	EM_SAFE_FREE(mail_table_data.datetime);
	EM_SAFE_FREE(mail_table_data.email_address_sender);
	EM_SAFE_FREE(mail_table_data.alias_sender);
	EM_SAFE_FREE(mail_table_data.email_address_recipient);
	EM_SAFE_FREE(mail_table_data.alias_recipient);

	if (err_code  != NULL)
		*err_code = err;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

int em_core_mail_check_rule(emf_mail_head_t *head, emf_mail_rule_tbl_t *rule, int rule_len, int *matched, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("head [%p], rule [%p], rule_len [%d], matched [%p], err_code [%p]", head, rule, rule_len, matched, err_code);
	
	int ret = false, err = EMF_ERROR_NONE, i;
	size_t len = 0;
	char *src = NULL; 	/*  string which will be compared with rules */
	char *from_address = NULL;
	ADDRESS *addr = NULL;
	
	if (!matched || !head || !head->from || !head->subject) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	*matched = -1;
	
	for (i = 0; i < rule_len; i++) {
		if (!(rule + i)) {
			EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
			err = EMF_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("rule[%d].flag1(rule id[%d]) is %d", i, rule[i].rule_id, rule[i].flag1);

		if (rule[i].flag1){	
			/*  'ON' */
			EM_DEBUG_LOG("rule[%d].flag2(rule id[%d]) is %d", i, rule[i].rule_id, rule[i].flag2);
			switch (rule[i].type) {
				case EMF_FILTER_FROM:
					if (from_address == NULL) {
					    from_address = EM_SAFE_STRDUP(head->from);
					    rfc822_parse_adrlist(&addr, from_address, NULL);
						if(addr) {
						    EM_DEBUG_LOG("rule  :  head->from[%s], addr->mailbox[%s], addr->host[%s]", head->from, addr->mailbox, addr->host);
						    EM_SAFE_FREE(from_address);
					
						    if (addr->mailbox)
								len = strlen(addr->mailbox);
						    if (addr->host)
								len  += strlen(addr->host);
							    len  += 2;

						    if (!(from_address = em_core_malloc(len))) {
							    EM_DEBUG_EXCEPTION("em_core_malloc failed...");
							    err = EMF_ERROR_OUT_OF_MEMORY;
							    goto FINISH_OFF;
						    }
						
						    SNPRINTF(from_address, len, "%s@%s", addr->mailbox, addr->host); 
						}
						else  {
							EM_DEBUG_EXCEPTION("rfc822_parse_adrlist failed.");
							err = EMF_ERROR_INVALID_ADDRESS;
							goto FINISH_OFF;
						}
					}

					src = from_address;
					break;
				case EMF_FILTER_SUBJECT:
					src = head->subject;
					break;
				case EMF_FILTER_BODY: 
					err = EMF_ERROR_NOT_SUPPORTED;
					goto FINISH_OFF;
					break;
		 	}
			EM_DEBUG_LOG("rule src[%s], value[%s]\n", src, rule[i].value);

			if (src && rule[i].value) {
			    if (RULE_TYPE_INCLUDES == rule[i].flag2) {
					if (strstr(src, rule[i].value)) {
						*matched = i; 
						break;
					}
				}
				else if (RULE_TYPE_EXACTLY == rule[i].flag2) {
					if (!strcmp(src, rule[i].value)) {
						*matched = i; 
						break;
					}
				}
			}
		}
		else
			EM_DEBUG_LOG("Invald src or rule[i].value");
	}
	ret = true;

	EM_DEBUG_LOG("i [%d], matched [%d]", i, *matched);
FINISH_OFF: 

	EM_SAFE_FREE(from_address);
	
	if (addr  != NULL)
		mail_free_address(&addr);	
	
	if (err_code  != NULL)
		*err_code = err;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

static int em_core_get_utf8_address(char **dest, ADDRESS *address, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("dest[%p], address[%p], err_code[%p]", dest, address, err_code);
	
	if (!dest || !address)  {
		EM_DEBUG_EXCEPTION("dest[%p], address[%p]", dest, address);
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	gchar *concatenated = NULL;
	gchar *utf8_address = NULL;
	gchar *temp = NULL;
	char *nickname = NULL;
	
	while (address)  {
		EM_DEBUG_LOG("address->mailbox[%s], address->host[%s]", address->mailbox, address->host);
		if (!address->mailbox || !address->host) {
			address = address->next;
			continue;
		}
		EM_DEBUG_LOG("address->mailbox[%p]", address->personal);
		if (address->personal)  {
			if (!(nickname = em_core_decode_rfc2047_text(address->personal, &err)))  {
				EM_DEBUG_EXCEPTION("em_core_decode_rfc2047_text failed - %d", err);
				goto FINISH_OFF;
			}
			EM_DEBUG_LOG("nickname[%s]", nickname);
			if (*nickname != '\0') 
				utf8_address = g_strdup_printf("\"%s\" <%s@%s>", nickname, address->mailbox ? address->mailbox : "", address->host ? address->host : "");
			else
				utf8_address = g_strdup_printf("<%s@%s>", address->mailbox ? address->mailbox : "", address->host ? address->host : "");
			
			EM_SAFE_FREE(nickname);
		}
		else
			utf8_address = g_strdup_printf("<%s@%s>", address->mailbox ? address->mailbox : "", address->host ? address->host : "");
		
		EM_DEBUG_LOG("utf8_address[%s]", utf8_address);
		
		if (concatenated != NULL)  {
			temp = concatenated;
			concatenated = g_strdup_printf("%s; %s", temp, utf8_address);
			g_free(temp);
		}
		else
			concatenated = g_strdup(utf8_address);
		
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


static int em_core_mail_parse_envelope(ENVELOPE *envelope, emf_mail_head_t **head, int *to_num, int *err_code)
{
	EM_PROFILE_BEGIN(emCoreParseEnvelope);
	EM_DEBUG_FUNC_BEGIN("envelope[%p], head[%p], to_num[%p], err_code[%p]", envelope, head, to_num, err_code);

	int ret = false;
	int err = EMF_ERROR_NONE;
	
	emf_mail_head_t *p = NULL;
	int i = 0;
	
	if (!head) {
		EM_DEBUG_EXCEPTION("envelope[%p], head[%p], to_num[%p]", envelope, head, to_num);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!(p = em_core_malloc(sizeof(emf_mail_head_t)))) {
		EM_DEBUG_EXCEPTION("em_core_malloc failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	if (envelope->subject) {
		p->subject = em_core_decode_rfc2047_text(envelope->subject, &err);
		EM_DEBUG_LOG("subject[%s]", p->subject);
	}
	
	if (envelope->from) {
		if (!em_core_get_utf8_address(&p->from, envelope->from, &err)) {
			EM_DEBUG_EXCEPTION("em_core_get_utf8_address failed [%d]", err);
			goto FINISH_OFF;
		}
		
		EM_DEBUG_LOG(" head->from[%s]", p->from);
	}
	
	if (envelope->to) {
		/*
		ADDRESS *addr = NULL;		
		
		for (addr = envelope->to, i = 0; addr  != NULL; addr = addr->next, i++)
			EM_DEBUG_LOG("envelope->personal[%s], mailbox[%s], host[%s]", addr->personal, addr->mailbox, addr->host);
		*/

		EM_DEBUG_LOG("envelope->to");
		if (!em_core_get_utf8_address(&p->to, envelope->to, &err)) {
			EM_DEBUG_EXCEPTION("em_core_get_utf8_address failed [%d]", err);
			goto FINISH_OFF;
		}
		
		EM_DEBUG_LOG("head->to[%s]", p->to);
	}
	
	if (envelope->cc) {
		EM_DEBUG_LOG("envelope->cc");
		if (!em_core_get_utf8_address(&p->cc, envelope->cc, &err)) {
			EM_DEBUG_EXCEPTION("em_core_get_utf8_address failed [%d]", err);
			goto FINISH_OFF;
		}
		
		EM_DEBUG_LOG("head->cc[%s]", p->cc);
	}
	
	if (envelope->bcc) {
		if (!em_core_get_utf8_address(&p->bcc, envelope->bcc, &err)) {
			EM_DEBUG_EXCEPTION("em_core_get_utf8_address failed [%d]", err);
			goto FINISH_OFF;
		}
		
		EM_DEBUG_LOG("head->bcc[%s]", p->bcc);
	}
	
	if (envelope->reply_to) {
		if (!em_core_get_utf8_address(&p->reply_to, envelope->reply_to, &err)) {
			EM_DEBUG_EXCEPTION("em_core_get_utf8_address failed [%d]", err);
			goto FINISH_OFF;
		}
		
		EM_DEBUG_LOG("  head->reply_to[%s]\n", p->reply_to);
	}
	
	if (envelope->return_path) {
		if (!em_core_get_utf8_address(&p->return_path, envelope->return_path, &err)) {
			EM_DEBUG_EXCEPTION("em_core_get_utf8_address failed [%d]", err);
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG("head->return_path[%s]", p->return_path);
	}
	
	p->mid = EM_SAFE_STRDUP(envelope->message_id);
	
	*head = p; 
	p = NULL;
	
	if (to_num  != NULL)
		*to_num = i;
	
	ret = true;
	
FINISH_OFF: 

	if (ret != true)
		EM_SAFE_FREE(p);
	
	if (err_code  != NULL)
		*err_code = err;
	
	EM_PROFILE_END(emCoreParseEnvelope);
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

EXPORT_API int em_core_mailbox_sync_header(emf_mailbox_tbl_t *input_mailbox_tbl, emf_mailbox_tbl_t *input_mailbox_tbl_spam, void *stream_recycle, em_core_uid_list **input_uid_list, int *unread_mail, int *err_code)
{
	EM_PROFILE_BEGIN(emCoreSyncHeader);
	EM_DEBUG_FUNC_BEGIN("input_mailbox_tbl[%p], input_mailbox_tbl_spam[%p], input_uid_list [%p], err_code[%p]", input_mailbox_tbl, input_mailbox_tbl_spam, input_uid_list, err_code);

	int ret = false;
	int err = EMF_ERROR_NONE, err_2 = EMF_ERROR_NONE;
	int status = EMF_LIST_FAIL;
	int download_limit_count;
	emf_account_t *account_ref = NULL;
	emf_mail_rule_tbl_t *rule = NULL;
	em_core_uid_list *uid_list = NULL;
	em_core_uid_list *uid_elem = NULL;
	emf_mail_head_t *head = NULL;
	ENVELOPE *env = NULL;
	int account_id = 0, mail_id = 0, rule_len = 1000, total = 0, unread = 0, i = 0, percentage  = 0, thread_id = -1;
	void *stream = NULL;
	char *uid_range = NULL;
	
	if (!input_mailbox_tbl) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM:input_mailbox_tbl[%p]", input_mailbox_tbl);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	account_id = input_mailbox_tbl->account_id;
	account_ref = em_core_get_account_reference(account_id);
	if (!account_ref) {		
		EM_DEBUG_EXCEPTION("em_core_get_account_reference failed - %d", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	FINISH_OFF_IF_CANCELED;
	
#ifndef 	__FEATURE_KEEP_CONNECTION__
	/*  h.gahlaut :  Recycling of stream is taken care internally in em_core_mailbox_open so no need of this code here */
	if (stream_recycle)
		stream = stream_recycle; /*  set stream for recycling connection. */
#endif
	
	if (!em_core_mailbox_open(account_id, input_mailbox_tbl->mailbox_name, (void **)&stream, &err) || !stream){
		EM_DEBUG_EXCEPTION("em_core_mailbox_open failed - %d", err);
		status = EMF_LIST_CONNECTION_FAIL;
		goto FINISH_OFF;
	}
	
	FINISH_OFF_IF_CANCELED;
	
	/*  save total mail count on server to DB */
	if (!em_storage_update_mailbox_total_count(account_id, input_mailbox_tbl->mailbox_name, ((MAILSTREAM *)stream)->nmsgs, 1, &err)){
		EM_DEBUG_EXCEPTION("em_storage_update_mailbox_total_count failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	/* if (((MAILSTREAM *)stream)->nmsgs > 0) */
	{		
		emf_option_t *opt_ref = &account_ref->options;
		EM_DEBUG_LOG("block_address = %d, block_subject = %d", opt_ref->block_address, opt_ref->block_subject);
		
		if (opt_ref->block_address || opt_ref->block_subject) {
			int is_completed = false;
			int type = 0;
			
			if (!opt_ref->block_address)
				type = EMF_FILTER_SUBJECT;
			else if (!opt_ref->block_subject)
				type = EMF_FILTER_FROM;
			
			if (!em_storage_get_rule(ALL_ACCOUNT, type, 0, &rule_len, &is_completed, &rule, true, &err) || !rule) 
				EM_DEBUG_EXCEPTION("em_storage_get_rule failed - %d", err);
		}
		download_limit_count = input_mailbox_tbl->mail_slot_size;
		if (!em_core_mailbox_get_uids_to_download(stream, account_ref, input_mailbox_tbl, download_limit_count,  &uid_list, &total, EMF_SYNC_LATEST_MAILS_FIRST, &err)){
			EM_DEBUG_EXCEPTION("em_core_mailbox_get_uids_to_download failed [%d]", err);
			uid_list = NULL; 
			goto FINISH_OFF;
		}
		
		FINISH_OFF_IF_CANCELED;
		
		if (input_uid_list && *input_uid_list){
			em_core_mailbox_free_uids(*input_uid_list, NULL);		
			*input_uid_list = uid_list;
		}
		uid_elem = uid_list;
		i = 0;
		EM_PROFILE_BEGIN(emCoreSyncHeaderwhileloop);

#ifdef  __FEATURE_HEADER_OPTIMIZATION__
		/* g.shyamakshi@samsung.com : Bulk fetch of headers only if the recieving server type is IMAP */

		EM_DEBUG_LOG("((MAILSTREAM *)stream)->nmsgs [%d]", ((MAILSTREAM *)stream)->nmsgs);
		EM_DEBUG_LOG("uid_list [%p]", uid_list);
		if (account_ref->receiving_server_type == EMF_SERVER_TYPE_IMAP4 && uid_list  != NULL){
			em_core_uid_list *uid_list_prev = NULL;
			em_core_uid_list *uid_list_fast = uid_list;  
			int index = 0;
			int msg_count = total;
			int uid_range_size = msg_count * 8 + 1000;

			EM_DEBUG_LOG("memory allocation for uid_range [%d, %d]", msg_count, uid_range_size);
			uid_range = malloc(sizeof(char) * uid_range_size);

			if (uid_range == NULL){
				EM_DEBUG_EXCEPTION("memory allocation for uid_range failed");
				err  = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			uid_list_prev = uid_list_fast;

			if (uid_list_fast->next == NULL){
				/* Single list entry */
				snprintf(uid_range, uid_range_size, "%d", atoi(uid_list_fast->uid));
			}
			else{
				/* forming range of uids to be passed */
				while (uid_list_fast  != NULL){
					/* uid_list_fast = uid_list_fast->next; */

					if ((uid_list_fast->next != NULL) && (((atoi(uid_list_prev->uid)) - (atoi(uid_list_fast->next->uid))) == 1)){
						index += snprintf(uid_range+index, uid_range_size, "%d", atoi(uid_list_prev->uid));

						uid_list_fast = uid_list_fast->next;
						uid_list_prev = uid_list_fast;
						
						/* to make UID range string "abc, XX : YY" */
						while (uid_list_fast  != NULL){
							if (uid_list_fast->next == NULL)
								break;
							if (((atoi(uid_list_prev->uid)) - (atoi(uid_list_fast->next->uid))) == 1){
								uid_list_fast = uid_list_fast->next;
								uid_list_prev = uid_list_fast;
							}
							else
								break;
						}
						if ((uid_list_fast  != NULL) && (uid_list_fast->next  != NULL))
							index  += snprintf(uid_range+index, uid_range_size, ":%d,", atoi(uid_list_prev->uid));
						else
							index  += snprintf(uid_range+index, uid_range_size, ":%d", atoi(uid_list_prev->uid));
						
						uid_list_fast = uid_list_fast->next;
						uid_list_prev = uid_list_fast;
					}
					else{
						if (uid_list_fast->next  != NULL)
							index  += snprintf(uid_range+index, uid_range_size, "%d,", atoi(uid_list_prev->uid));
						else
							index  += snprintf(uid_range+index, uid_range_size, "%d", atoi(uid_list_prev->uid));
						uid_list_fast = uid_list_fast->next;
						uid_list_prev = uid_list_fast;
					}
				}
			}
			
			/* h.gahlaut :  [Start] */
			/* Adding this check here to check if application has called cancel job. */
			/* This checking should be done because fetching 50 headers will take time  */
			FINISH_OFF_IF_CANCELED;

			EM_DEBUG_LOG("index [%d]", index);

			/*  h.gahlaut :  [End] */
			uid_elem = uid_list;
			if (stream && uid_elem){
				EM_DEBUG_LOG("msgno : %d", uid_elem->msgno);
				((MAILSTREAM *)stream)->nmsgs = uid_elem->msgno;
			}
			else{
				EM_DEBUG_LOG("Uid List Null");
			}
			EM_DEBUG_LOG("Calling ... mail_fetch_fast. uid_range [%s]", uid_range);
			mail_fetch_fast(stream, uid_range, FT_UID | FT_PEEK | FT_NEEDENV); 
			EM_SAFE_FREE(uid_range);
		}
#endif
		
		/*  h.gahlaut@samsung.com :  Clear the event queue of partial body download thread before starting fetching new headers  */
#ifndef __PARTIAL_BODY_FOR_POP3__
		if (account_ref->receiving_server_type == EMF_SERVER_TYPE_IMAP4){
#endif /*  __PARTIAL_BODY_FOR_POP3__ */
			/*  Partial body download feature is only for IMAP accounts  */
			if (false == em_core_clear_partial_body_thd_event_que(&err))
				EM_DEBUG_LOG("em_core_clear_partial_body_thd_event_que failed [%d]", err);
#ifndef __PARTIAL_BODY_FOR_POP3__
		}
#endif /*  __PARTIAL_BODY_FOR_POP3__ */
		while (uid_elem) {
			EM_PROFILE_BEGIN(emCoreSyncHeaderEachMail);
			EM_DEBUG_LOG("mail_fetchstructure_full  :  uid_elem->msgno[%d]", uid_elem->msgno);

			env = NULL;

			if (uid_elem->msgno > ((MAILSTREAM *)stream)->nmsgs)
				EM_DEBUG_EXCEPTION("Warnings! msgno[%d] can't be greater than nmsgs[%d]. It might cause crash.", uid_elem->msgno, ((MAILSTREAM *)stream)->nmsgs);	
			else{
			
#ifdef __FEATURE_HEADER_OPTIMIZATION__
				if (account_ref->receiving_server_type == EMF_SERVER_TYPE_IMAP4) /* Fetch env from cache in case of IMAP */
					env = mail_fetchstructure_full(stream, uid_elem->msgno, NULL, FT_PEEK, 0);
				else /* Fetch header from network in case of POP */
					env = mail_fetchstructure_full(stream, uid_elem->msgno, NULL, FT_PEEK, 1);
#else
				env = mail_fetchstructure_full(stream, uid_elem->msgno, NULL, FT_PEEK);
#endif			
			}
			FINISH_OFF_IF_CANCELED;
			
			if (env  != NULL){
				int to_num = 0;
				int matched = -1;
				
				if (!em_core_mail_parse_envelope(env, &head, &to_num, &err) || !head) {
					EM_DEBUG_EXCEPTION("em_core_mail_parse_envelope failed [%d]", err);
					goto FINISH_OFF;
				}

				if (rule && input_mailbox_tbl_spam && !em_core_mail_check_rule(head, rule, rule_len, &matched, &err)) {
					EM_DEBUG_EXCEPTION("em_core_mail_check_rule failed [%d]", err);
					goto FINISH_OFF;
				}
				
				if (matched >= 0 && input_mailbox_tbl_spam){	/*  add filtered mails to SPAMBOX */
					EM_DEBUG_LOG("mail[%d] added to spambox", mail_id);
					
					if (!em_core_mail_add_header(stream, input_mailbox_tbl_spam, head, to_num, uid_elem, &mail_id, &thread_id, &err)) {
						EM_DEBUG_EXCEPTION("em_core_mail_add_header falied [%d]", err);
						goto FINISH_OFF;
					}
					
					if (account_ref->receiving_server_type == EMF_SERVER_TYPE_IMAP4){
						if (!em_core_mail_move_from_server(account_id, input_mailbox_tbl->mailbox_name,  &mail_id, 1, input_mailbox_tbl_spam->mailbox_name, &err)){
							EM_DEBUG_EXCEPTION("em_core_mail_move_from_server falied [%d]", err);
							goto FINISH_OFF;
						}
					}
				} else {	
					/*  add mails to specified mail box */
					EM_DEBUG_LOG("mail[%d] moved to input_mailbox_tbl [%s]", mail_id, input_mailbox_tbl->mailbox_name);
					if (!em_core_mail_add_header(stream, input_mailbox_tbl, head, to_num, uid_elem, &mail_id, &thread_id,  &err)) {
						EM_DEBUG_EXCEPTION("em_core_mail_add_header falied [%d]", err);
						goto FINISH_OFF;
					}
					
					/*h.gahlaut :  Start partial body dowload using partial body thread only for IMAP accounts*/
#ifndef __PARTIAL_BODY_FOR_POP3__
					if (account_ref->receiving_server_type == EMF_SERVER_TYPE_IMAP4) {
#endif /*  __PARTIAL_BODY_FOR_POP3__ */
						if (false == em_core_initiate_pbd(stream, account_id, mail_id, uid_elem->uid, input_mailbox_tbl->mailbox_name, &err))
							EM_DEBUG_LOG("Partial body download initiation failed [%d]", err);
#ifndef __PARTIAL_BODY_FOR_POP3__
					}
#endif /*  __PARTIAL_BODY_FOR_POP3__ */

					if (!uid_elem->flag.seen && input_mailbox_tbl->mailbox_type != EMF_MAILBOX_TYPE_SPAMBOX)
						em_core_add_notification_for_unread_mail_by_mail_header(account_id, mail_id, head);
					
					FINISH_OFF_IF_CANCELED;
					
					if (!uid_elem->flag.seen)
						unread++;

					percentage = ((i+1) * 100) / total ;
					EM_DEBUG_LOG("Header Percentage Completed [%d] : [%d/%d]  mail_id [%d]", percentage, i+1, total, mail_id);

					if (!em_storage_notify_storage_event(NOTI_MAIL_ADD, account_id, mail_id, input_mailbox_tbl->mailbox_name, thread_id))
						EM_DEBUG_EXCEPTION("em_storage_notify_storage_event [NOTI_MAIL_ADD] failed");
				}
				
				/*  Release for envelope is not required and it may cause crash. Don't free the memory for envelope here. */
				/*  Envelope data will be freed by garbage collector in mail_close_full */
				if (head){
					em_core_mail_head_free(&head, 1, NULL); 
					head = NULL;
				}

				FINISH_OFF_IF_CANCELED;
			}
			
			uid_elem = uid_elem->next;
			i++;
			EM_PROFILE_END(emCoreSyncHeaderEachMail);
		}
		EM_PROFILE_END(emCoreSyncHeaderwhileloop);
	}
	
	*unread_mail = unread;
	ret = true;
FINISH_OFF: 

	if (!em_core_mailbox_remove_overflowed_mails(input_mailbox_tbl, &err_2))
		EM_DEBUG_EXCEPTION("em_core_mailbox_remove_overflowed_mails failed - %d", err_2);

	if (head  != NULL)
		em_core_mail_head_free(&head, 1, NULL);
	
	if (uid_list != NULL){
		em_core_mailbox_free_uids(uid_list, NULL);
		/*  uid_list point to the same memory with input_mailbox_tbl->user_data.  */
		/*  input_mailbox_tbl->user_data should be set NULL if uid_list is freed */
		*input_uid_list = NULL;
	}

	EM_SAFE_FREE(uid_range);

	if (rule  != NULL)
		em_storage_free_rule(&rule, rule_len, NULL); 

#ifdef __FEATURE_KEEP_CONNECTION__	
	if (stream  != NULL) {
#else /*  __FEATURE_KEEP_CONNECTION__	 */
	if (stream  != NULL && stream_recycle == NULL) 	{
#endif /*  __FEATURE_KEEP_CONNECTION__	 */
		em_core_mailbox_close(0, stream);				
		stream = NULL;
	}

	if (err_code  != NULL)
		*err_code = err;
	
	EM_PROFILE_END(emCoreSyncHeader);
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}


em_core_uid_list *__ReverseList(em_core_uid_list *uid_list)
{
	em_core_uid_list *temp, *current, *result;

	temp = NULL;
	result = NULL;
	current = uid_list;

	while (current != NULL){
		temp = current->next;
		current->next = result;
		result = current;
		current = temp;
	}
	uid_list = result;
	return uid_list;
}

			

int em_core_mailbox_download_uid_all(emf_mailbox_t *mailbox, em_core_uid_list** uid_list, int *total, emf_mail_read_mail_uid_tbl_t *downloaded_uids, int count, em_core_get_uids_for_delete_t for_delete, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], uid_list[%p], total[%p], downloaded_uids[%p], count[%d], for_delete[%d], err_code[%p]", mailbox, uid_list, total, downloaded_uids, count, for_delete, err_code);

	int ret = false;
	int err = EMF_ERROR_NONE;
	
	MAILSTREAM *stream = NULL;
	emf_account_t *ref_account = NULL;
	em_core_uid_list *uid_elem = NULL;
	em_core_uid_list *fetch_data_p = NULL;
	void *tmp_stream = NULL;
	char cmd[64] = {0x00, };
	char *p = NULL;

	if (!mailbox || !uid_list) {
		EM_DEBUG_EXCEPTION("mailbox[%p], uid_list[%p], total[%p], downloaded_uids[%p], count[%d], for_delete[%d]", mailbox, uid_list, total, downloaded_uids, count, for_delete);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(ref_account = em_core_get_account_reference(mailbox->account_id))) {
		EM_DEBUG_EXCEPTION("em_core_get_account_reference failed - %d", mailbox->account_id);
		
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!mailbox->mail_stream) {
		if (!em_core_mailbox_open(mailbox->account_id, mailbox->name, (void **)&tmp_stream, &err)){
			EM_DEBUG_EXCEPTION("em_core_mailbox_open failed...");
			
			goto FINISH_OFF;
		}
		
		stream = (MAILSTREAM *)tmp_stream;
	} 
	else 
		stream = mailbox->mail_stream;
	
	if (ref_account->receiving_server_type == EMF_SERVER_TYPE_POP3) {		/*  POP3 */
		POP3LOCAL *pop3local = NULL;
		
		if (!stream || !(pop3local = stream->local) || !pop3local->netstream) {
			err = EMF_ERROR_INVALID_PARAM;		/* EMF_ERROR_UNKNOWN; */
			goto FINISH_OFF;
		}
		
		/*  send UIDL */
		memset(cmd, 0x00, sizeof(cmd));
		
		SNPRINTF(cmd, sizeof(cmd), "UIDL\015\012");
		
#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG("[POP3] >>> [%s]", cmd);
#endif

		if (!net_sout(pop3local->netstream, cmd, (int)strlen(cmd))) {
			EM_DEBUG_EXCEPTION("net_sout failed...");
			
			err = EMF_ERROR_CONNECTION_BROKEN;
			goto FINISH_OFF;
		}
		
		/*  get uid from replied data */
		while (pop3local->netstream) {
			char *s = NULL;
			
			if (!(p = net_getline(pop3local->netstream)))
				break;
			
#ifdef FEATURE_CORE_DEBUG
			EM_DEBUG_LOG(" [POP3] <<< [%s]", p);
#endif
			
			/*  replied error "-ERR" */
			if (*p == '-') {
				err = EMF_ERROR_MAIL_NOT_FOUND;		
				goto FINISH_OFF;
			}
			
			/*  replied success "+OK" */
			if (*p == '+') {
				free(p); p = NULL;
				continue;
			}
			
			/*  end of command */
			if (*p == '.')
				break;
			
			/* EM_DEBUG_LOG("UID list [%s]", p); */
			
			uid_elem = (em_core_uid_list *)malloc(sizeof(em_core_uid_list));
			if (!uid_elem) {
				EM_DEBUG_EXCEPTION("malloc falied...");
				
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			memset(uid_elem, 0x00, sizeof(em_core_uid_list));
			
			/*  format  :  "1 AAA6FHEAAAQrB6c1ymXxty04yks7hcQ7" */
			
			/*  save uid			 */
			s = strstr(p, " ");
			if (s) {
				*s = '\0';
				uid_elem->msgno = atoi(p);
				uid_elem->uid = EM_SAFE_STRDUP(s+1);
			}
			
			/*  check downloaded_uids */
			if (downloaded_uids) {
				int i;
				for (i = 0; i < count; ++i) {
					if (!strcmp(uid_elem->uid, downloaded_uids[i].s_uid)) {
						downloaded_uids[i].flag = 1;
						break;
					}
				}
			}
			
			if (*uid_list) {		/* TODO :  modify me */
				fetch_data_p = *uid_list;
				
				while (fetch_data_p->next)
					fetch_data_p = fetch_data_p->next;
				
				fetch_data_p->next = uid_elem;
			}
			else
				*uid_list = uid_elem;
			
			if (total)
				++(*total);
			
			free(p); p = NULL;
		}
	}
	else {		/*  IMAP */
		IMAPLOCAL *imaplocal = NULL;
		char tag[16];
		char *s = NULL;
		char *t = NULL;
		
		if (!stream || !(imaplocal = stream->local) || !imaplocal->netstream) {
			err = EMF_ERROR_INVALID_PARAM;		/* EMF_ERROR_UNKNOWN; */
			goto FINISH_OFF;
		}
		
		/*  send FETCH UID */
		memset(tag, 0x00, sizeof(tag));
		memset(cmd, 0x00, sizeof(cmd));
		
		SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
		SNPRINTF(cmd, sizeof(cmd), "%s UID FETCH %d:* (FLAGS)\015\012", tag, 1);
#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG("[IMAP] >>> %s", cmd);		
#endif
		if (!net_sout(imaplocal->netstream, cmd, (int)strlen(cmd))) {
			EM_DEBUG_EXCEPTION("net_sout failed...");
			
			err = EMF_ERROR_CONNECTION_BROKEN;
			goto FINISH_OFF;
		}
		
		/*  get uid from replied data */
		while (imaplocal->netstream) {
			if (!(p = net_getline(imaplocal->netstream)))
				break;
			
			/* EM_DEBUG_LOG(" [IMAP] <<< %s", p); */
			
			/*  tagged line - end of command */
			if (!strncmp(p, tag, strlen(tag)))
				break;
			
			/*  check that reply is reply to our command */
			/*  format  :  "* 9 FETCH (UID 68)" */
			if (!strstr(p, "FETCH (FLAGS")) {
				free(p); p = NULL;
				continue;
			}
			
			if (for_delete == EM_CORE_GET_UIDS_FOR_NO_DELETE) {
				if ((ref_account->retrieval_mode == EMF_IMAP4_RETRIEVAL_MODE_NEW) && (strstr(p, "\\Seen"))) {
					free(p); p = NULL;
					continue;
				}
			}
			
			uid_elem = (em_core_uid_list *)malloc(sizeof(em_core_uid_list));
			if (!uid_elem) {
				EM_DEBUG_EXCEPTION("malloc failed...");
				
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			memset(uid_elem, 0x00, sizeof(em_core_uid_list));
			/*  parse uid, sequence, flags from replied data */
			
			/*  parse uid from replied data */
			s = p+2;
			t = strchr(s, ' ');
			
			if (!t) {
				err = EMF_ERROR_INVALID_RESPONSE;		
				goto FINISH_OFF;
			}
			
			*t = '\0';
			
			/*  save sequence */
			uid_elem->msgno = atoi(s);
			
			if (strstr(++t, "\\Deleted"))		
				uid_elem->flag.deleted = 1;
			
			/*  parse uid */
			s  = strstr(++t, "UID ");
			if (s) {
				s  += strlen("UID ");
				t  = strchr(s, ')');
				
				if (!t) {
					err = EMF_ERROR_INVALID_RESPONSE;		
					goto FINISH_OFF;
				}
				
				*t = '\0';
			}
			else {		
				err = EMF_ERROR_INVALID_RESPONSE;		
				goto FINISH_OFF;
			}
			
			/*  save uid */
			uid_elem->uid = EM_SAFE_STRDUP(s);
			
			/*  check downloaded_uids */
			if (downloaded_uids) {
				int i;
				for (i = 0; i < count; ++i) {
					if (uid_elem->uid && !strcmp(uid_elem->uid, downloaded_uids[i].s_uid)) {		/*  20080308 - prevent 25231  :  Reverse Null. Pointer "(uid_elem)->uid" dereferenced before NULL check */
						downloaded_uids[i].flag = 1;
						free(uid_elem->uid);
						free(uid_elem); uid_elem = NULL;
						break;
					}
				}
			}
			
			if (uid_elem) {
				if (*uid_list) {
					fetch_data_p = *uid_list;
					
					while (fetch_data_p->next)
						fetch_data_p = fetch_data_p->next;
					
					fetch_data_p->next = uid_elem;
				}
				else {
					*uid_list = uid_elem;
				}
				
				if (total)
					++(*total);
			}
			
			free(p); p = NULL;
		}
	}

	ret = true;

FINISH_OFF: 
	if (uid_elem && ret == false)		
		free(uid_elem);
	
	if (p)
		free(p);
	
	if (mailbox && !mailbox->mail_stream) {
		em_core_mailbox_close(0, stream);
		stream = NULL;
	}
	
	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

int em_core_mailbox_download_imap_msgno(emf_mailbox_t *mailbox, char *uid, int *msgno, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], uid[%p], msgno[%p], err_code[%p]", mailbox, uid, msgno, err_code);

	int ret = false;
	int err = EMF_ERROR_NONE;
	MAILSTREAM *stream = NULL;
	IMAPLOCAL *imaplocal = NULL;
	emf_account_t *ref_account = NULL;
	void *tmp_stream = NULL;
	char tag[32], cmd[64];
	char *p = NULL;

	if (!mailbox || !uid) {
		EM_DEBUG_EXCEPTION("mailbox[%p], uid[%p], msgno[%p]", mailbox, uid, msgno);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(ref_account = em_core_get_account_reference(mailbox->account_id))) {
		EM_DEBUG_EXCEPTION("em_core_get_account_reference failed [%d]", mailbox->account_id);
		
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (ref_account->receiving_server_type  != EMF_SERVER_TYPE_IMAP4) {
		err = EMF_ERROR_INVALID_ACCOUNT;		/* EMF_ERROR_INVALID_PARAM; */
		goto FINISH_OFF;
	}

	if (!mailbox->mail_stream) {
		if (!em_core_mailbox_open(mailbox->account_id, mailbox->name, (void **)&tmp_stream, &err)) {
			EM_DEBUG_EXCEPTION("em_core_mailbox_open failed - %d", err);
			
			goto FINISH_OFF;
		}
		
		stream = (MAILSTREAM *)tmp_stream;
	} 
	else
		stream = mailbox->mail_stream;

	if (!stream || !(imaplocal = stream->local) || !imaplocal->netstream) {
		err = EMF_ERROR_INVALID_PARAM;		/* EMF_ERROR_UNKNOWN; */
		goto FINISH_OFF;
	}

	/*  send SEARCH UID */
	memset(tag, 0x00, sizeof(tag));
	memset(cmd, 0x00, sizeof(cmd));

	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
	SNPRINTF(cmd, sizeof(cmd), "%s SEARCH UID %s\015\012", tag, uid);

	if (!net_sout(imaplocal->netstream, cmd, (int)strlen(cmd))) {
		EM_DEBUG_EXCEPTION("net_sout failed...");
		
		err = EMF_ERROR_CONNECTION_BROKEN;
		goto FINISH_OFF;
	}

	/*  get message number from replied data */
	while (imaplocal->netstream) {
		if (!(p = net_getline(imaplocal->netstream)))
			break;
		
		/*  tagged line - end of command */
		if (!strncmp(p, tag, strlen(tag)))
			break;
		
		/*  check that reply is reply to our command */
		/*  format :  "* SEARCH 68", if not found, "* SEARCH" */
		if (!strstr(p, "SEARCH ") || (p[9] < '0' || p[9] > '9')) {
			free(p); p = NULL;
			continue;
		}
		
		if (msgno)
			*msgno = atoi(p+9);
		
		free(p); p = NULL;
		
		ret = true;
	}
	
	if (ret  != true)
		err = EMF_ERROR_MAIL_NOT_FOUND;
	
FINISH_OFF: 
	if (p)
		free(p);
	
	if (mailbox && !mailbox->mail_stream){
		em_core_mailbox_close(0, stream);				
		stream = NULL;
	}
	
	if (err_code)
		*err_code = err;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

int em_core_mailbox_get_msgno(em_core_uid_list *uid_list, char *uid, int *msgno, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("uid_list[%p], uid[%s], msgno[%p], err_code[%p]", uid_list, uid, msgno, err_code);
	
	int ret = false;
	int err = EMF_ERROR_MAIL_NOT_FOUND;		/* EMF_ERROR_NONE; */
	
	if (!uid || !msgno || !uid_list) {
		EM_DEBUG_EXCEPTION("uid_list[%p], uid[%p], msgno[%p]", uid_list, uid, msgno);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	EM_DEBUG_LOG(" >> uid[%s]", uid);
	
	while (uid_list) {
		if (!strcmp(uid_list->uid, uid)) {
			*msgno = uid_list->msgno;
			
			EM_DEBUG_LOG("*msgno[%d]", *msgno);
			
			err = EMF_ERROR_NONE;
			
			ret = true;
			break;
		}
		
		uid_list = uid_list->next;
	}
	
FINISH_OFF: 
	if (err_code  != NULL)
		*err_code = err;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

int em_core_mailbox_get_uid(em_core_uid_list *uid_list, int msgno, char **uid, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false, err = EMF_ERROR_NONE;
	
	if (!uid || msgno <= 0 || !uid_list){
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	while (uid_list) {
		if (uid_list->msgno == msgno) {
			if (uid) {
				if (!(*uid = EM_SAFE_STRDUP(uid_list->uid))) {
					EM_DEBUG_EXCEPTION("strdup failed...");
					err = EMF_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}
				
				ret = true;
				break;
			}
		}
		
		uid_list = uid_list->next;
	}
	
	if (ret  != true)
		err = EMF_ERROR_MAIL_NOT_FOUND;

FINISH_OFF: 
	if (err_code)
		*err_code = err;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

int em_core_mailbox_free_uids(em_core_uid_list *uid_list, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("uid_list[%p], err_code[%p]", uid_list, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	em_core_uid_list *p = NULL;
	
	if (!uid_list) {
		EM_DEBUG_EXCEPTION(" uid_list[%p]\n", uid_list);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	while (uid_list) {
		p = uid_list; uid_list = uid_list->next;
		
		EM_SAFE_FREE(p->uid);
		
		free(p);
		p = NULL;
	}
	
	ret = true;

FINISH_OFF: 
	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}


#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
/* callback for GET_APPENDUID - shasikala.p */
void mail_appenduid(char *mailbox, unsigned long uidvalidity, SEARCHSET *set)
{
	EM_DEBUG_FUNC_BEGIN("mailbox - %s", mailbox);
    EM_DEBUG_LOG("UID - %ld", set->first);

    memset(g_append_uid_rsp, 0x00, 129);

    sprintf(g_append_uid_rsp, "%ld", set->first);
    EM_DEBUG_LOG("append uid - %s", g_append_uid_rsp);
}

EXPORT_API int em_core_mail_sync_from_client_to_server(int account_id, int mail_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mail_id [%d], mail_id [%p]", account_id, mail_id, *err_code);

	int                             err             = EMF_ERROR_NONE;
	int                             ret             = false;
	int                             len             = 0;
	int                             read_size       = 0;
	char                           *fname           = NULL;
	char                           *long_enc_path   = NULL;
	char                           *data            = NULL;
	char                            set_flags[100]  = { 0, };
	ENVELOPE                       *envelope        = NULL;
	FILE                           *fp              = NULL;
 	STRING                          str;
	MAILSTREAM                     *stream          = NULL;
	emf_mail_t                     *mail            = NULL;
	emf_mail_tbl_t                 *mail_table_data = NULL;
	emf_mailbox_tbl_t              *mailbox_tbl     = NULL;

	if (mail_id < 1){
        EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
        goto FINISH_OFF;
	}

	/*  get a mail from mail table */
	if (!em_storage_get_mail_by_id(mail_id, &mail_table_data, true, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("mailbox_name [%s]", mail_table_data->mailbox_name);
	if (!em_storage_get_mailbox_by_name(mail_table_data->account_id, 0, mail_table_data->mailbox_name, &mailbox_tbl, false, &err) || !mailbox_tbl){
		EM_DEBUG_EXCEPTION("em_storage_get_mailbox_by_name failed [%d]", err);
		goto FINISH_OFF;
	}

	if (mailbox_tbl->sync_with_server_yn == 0) {
		EM_DEBUG_EXCEPTION("The mailbox [%s] is not on server.", mail_table_data->mailbox_name);
		err = EMF_ERROR_INVALID_MAILBOX;
		goto FINISH_OFF;
	}

	/*  get a mail to send */
	if (!em_core_mail_get_mail(mail_id, &mail, &err)){
		EM_DEBUG_EXCEPTION(" em_core_mail_get_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!em_core_get_long_encoded_path(mail_table_data->account_id, mail_table_data->mailbox_name, '/', &long_enc_path, &err)) {
		EM_DEBUG_EXCEPTION(">>em_core_get_long_encoded_path  :  Failed [%d] ", err);
		goto FINISH_OFF;
	}

	if (!long_enc_path) {
	    EM_DEBUG_EXCEPTION(">>long_enc_path  :  NULL ");
	    goto FINISH_OFF;
	}

	if (!em_core_mail_get_rfc822(mail, &envelope, &fname, NULL, &err)){
	    EM_DEBUG_EXCEPTION(" em_core_mail_get_rfc822 failed [%d]", err);
	    goto FINISH_OFF;                        
	}

	if (fname){
	    if (!(fp = fopen(fname, "a+"))) 
	    {
	        EM_DEBUG_EXCEPTION("fopen failed - %s", fname);
			err = EMF_ERROR_SYSTEM_FAILURE;
	        goto FINISH_OFF;
	    }
	}

	if (!fp) {
	    EM_DEBUG_EXCEPTION("fp is NULL..!");
		err = EMF_ERROR_SYSTEM_FAILURE;
	    goto FINISH_OFF;
	}

	rewind(fp);

	ret = fseek(fp, 0, SEEK_END) == 0 && (len = ftell(fp)) != -1;

	if (fname)
		EM_DEBUG_LOG("Composed file name [%s] and file size [%d]", fname, len);

	rewind(fp);

	ret = 0;
	stream = NULL;
	if (!em_core_mailbox_open(mail_table_data->account_id, NULL, (void **)&stream, &err)){
	    EM_DEBUG_EXCEPTION("em_core_mail_move_from_server failed :  Mailbox open[%d]", err);
	    goto FINISH_OFF;
	}

	/* added for copying server UID to DB */
	mail_parameters(stream, SET_APPENDUID, mail_appenduid);

	data = (char *)malloc(len + 1);
	/* copy data from file to data */
	read_size = fread(data, sizeof (char), len, fp);
	if (read_size  != len){
	    /* read faiil. */
	    EM_DEBUG_EXCEPTION("Read from file failed");
	}

	INIT(&str, mail_string, data, len);

	sprintf(set_flags, "\\Seen");

	if ((mail->info->flags).seen){
		if (!mail_append_full(stream, long_enc_path, set_flags, NULL, &str)) {
		    EM_DEBUG_EXCEPTION("mail_append  failed -");
		    goto FINISH_OFF;
		}
	}
	else{
		if (!mail_append_full(stream, long_enc_path, NULL, NULL, &str)) {
			EM_DEBUG_EXCEPTION("mail_append  failed -");
			goto FINISH_OFF;
		}
	}

	/* Update read_mail_uid tbl */
	if (!em_core_mailbox_add_read_mail_uid(mailbox_tbl, mail_table_data->mailbox_name, mail_table_data->mail_id, g_append_uid_rsp, mail_table_data->mail_size, 0, &err)) {
		EM_DEBUG_EXCEPTION(" em_core_mailbox_add_read_mail_uid failed [%d]", err);
		em_storage_rollback_transaction(NULL, NULL, NULL);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF: 

#ifdef __LOCAL_ACTIVITY__
	if (ret){
		emf_activity_tbl_t new_activity;
		memset(&new_activity, 0x00, sizeof(emf_activity_tbl_t));
		new_activity.activity_type = ACTIVITY_SAVEMAIL;
		new_activity.account_id    = account_id;
		new_activity.mail_id	   = mail_id;
		new_activity.dest_mbox	   = NULL;
		new_activity.server_mailid = NULL;
		new_activity.src_mbox	   = NULL;

		if (!em_core_activity_delete(&new_activity, &err)){
			EM_DEBUG_EXCEPTION(">>>>>>Local Activity [ACTIVITY_SAVEMAIL] [%d] ", err);
		}
	}
#endif /* __LOCAL_ACTIVITY__ */	

	EM_SAFE_FREE(data);
	EM_SAFE_FREE(long_enc_path);

	if (fp)
		fclose(fp);

	if (envelope)
		mail_free_envelope(&envelope);

	if (mail) 
		em_core_mail_free(&mail, 1, NULL);

	if (mail_table_data)
		em_storage_free_mail(&mail_table_data, 1, NULL);

	if (mailbox_tbl)
		em_storage_free_mailbox(&mailbox_tbl, 1, NULL);

	if (fname) {
		remove(fname);
		EM_SAFE_FREE(fname);
	}

	if (stream){
		em_core_mailbox_close(0, stream);				
		stream = NULL;	
	}

	if (err_code  != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

#endif

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

static int em_core_initiate_pbd(MAILSTREAM *stream, int account_id, int mail_id, char *uid, char *input_mailbox_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mail_id[%d], uid[%p], input_mailbox_name[%p]", account_id, mail_id, uid, input_mailbox_name);

	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_account_t *account_ref;

	if (account_id < FIRST_ACCOUNT_ID || mail_id < 0 || NULL == uid || NULL == input_mailbox_name){
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	account_ref = em_core_get_account_reference(account_id);

	emf_event_partial_body_thd pbd_event;

	memset(&pbd_event, 0x00, sizeof(emf_event_partial_body_thd));

	pbd_event.account_id = account_id;
	if (account_ref && account_ref->receiving_server_type == EMF_SERVER_TYPE_POP3)
		pbd_event.activity_type = ACTIVITY_PARTIAL_BODY_DOWNLOAD_POP3_WAIT;
	else
		pbd_event.activity_type = ACTIVITY_PARTIAL_BODY_DOWNLOAD_IMAP4;
		
	pbd_event.mailbox_name = EM_SAFE_STRDUP(input_mailbox_name);
	pbd_event.mail_id = mail_id;
	pbd_event.server_mail_id = strtoul(uid, NULL, 0);

	EM_DEBUG_LOG("input_mailbox_name name [%s]", pbd_event.mailbox_name);
	EM_DEBUG_LOG("uid [%s]", uid);
	EM_DEBUG_LOG("pbd_event.account_id[%d], pbd_event.mail_id[%d], pbd_event.server_mail_id [%d]", pbd_event.account_id, pbd_event.mail_id , pbd_event.server_mail_id);
	
	if (!em_core_insert_pbd_activity(&pbd_event, &pbd_event.activity_id, &err)){
		EM_DEBUG_EXCEPTION("Inserting Partial Body Download activity failed with error[%d]", err);
		goto FINISH_OFF;
	}
	else{
		if (false == em_core_is_partial_body_thd_que_full()){
			/* h.gahaut :  Before inserting the event into event queue activity_type should be made 0
			Because partial body thread differentiates events coming from DB and event queue 
			on the basis of activity_type and event_type fields */

			pbd_event.activity_type = 0;
			pbd_event.event_type = EMF_EVENT_BULK_PARTIAL_BODY_DOWNLOAD;
			
			if (!em_core_insert_partial_body_thread_event(&pbd_event, &err)){
				EM_DEBUG_EXCEPTION("Inserting Partial body thread event failed with error[%d]", err);
				goto FINISH_OFF;
			}

			/*h.gahlaut :  Partial body thread has created a copy of event for itself so this local event should be freed here*/
			if (!em_core_free_partial_body_thd_event(&pbd_event, &err))
				EM_DEBUG_EXCEPTION("Freeing Partial body thread event failed with error[%d]", err);
		}
		else{
			EM_DEBUG_LOG(" Activity inserted only in DB .. Queue is Full");
		}
	}

	ret = true;
	
	FINISH_OFF: 

	if (NULL  != err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

#define UID_RANGE_STRING_LENGTH 3000
#define TEMP_STRING_LENGTH		50

static int em_core_parse_html_part_for_partial_body(char *start_header, char *boundary_string, char *bufsendforparse, char *text_html, int body_size)
{
	EM_DEBUG_FUNC_BEGIN("start_header [%s], boundary_string [%s], bufsendforparse [%s], text_html [%s], body_size [%d]", start_header, boundary_string, bufsendforparse, text_html, body_size);

	int   err = EMF_ERROR_NONE;
	int   html_uidno = 0;
	int   iEncodingHeader = 0;
	int   enc_type = ENCOTHER, dec_len = 0, html_length = 0;
	char  EncodingHeader[40] = {0};
	char  Encoding[30] = {0};
	char *pEncodingHeaderEnd = NULL;
	char *txt_html = NULL;
	char *pHeaderStart = NULL;
	char *start = NULL, *end = NULL;
	char *temp_enc1 = NULL;

	EM_DEBUG_LOG("Content-Type : text/html or message/rfc822 or text/rfc822-headers");

	pHeaderStart = start_header;
	pHeaderStart = pHeaderStart-2;
	do{
		pHeaderStart--;
	} while (*pHeaderStart != LF && bufsendforparse < pHeaderStart);
	
	pHeaderStart++;
	
	memcpy(EncodingHeader, pHeaderStart, 25);

	if (strcasecmp(EncodingHeader, "Content-Transfer-Encoding") == 0){
		pEncodingHeaderEnd = strstr(pHeaderStart, CRLF_STRING);
		memcpy(Encoding, pHeaderStart + 27, pEncodingHeaderEnd - (pHeaderStart+27));
		iEncodingHeader = 1;
	}

	/* HTML Content found */
	
	txt_html = start_header;
	txt_html = strstr(txt_html, CRLF_STRING CRLF_STRING);

	if (txt_html != NULL){
		txt_html += 4; /*  txt_html points at html content */
		start = txt_html;
		char multipart_boundary[100] = {0};
		char *multipart_related_boundry = NULL;
		char *multipart_related_boundry_end = NULL;
		if (iEncodingHeader == 1)
			multipart_related_boundry = pHeaderStart;
		else
			multipart_related_boundry = start_header;
		
		multipart_related_boundry = multipart_related_boundry - 3;
		multipart_related_boundry_end = multipart_related_boundry;
		
		while (bufsendforparse < multipart_related_boundry && *multipart_related_boundry != LF && *multipart_related_boundry != NULL_CHAR)
			multipart_related_boundry -= 1;
		
		multipart_related_boundry  += 1;
		memcpy(multipart_boundary, multipart_related_boundry, multipart_related_boundry_end - multipart_related_boundry);
		
		EM_DEBUG_LOG("multipart_boundary [%s], boundary_string [%s]", multipart_boundary, boundary_string);

		if (strcmp(multipart_boundary, boundary_string) == 0)
			end = strstr(txt_html, boundary_string);
		else
			end = strstr(txt_html, multipart_boundary);

		memset(multipart_boundary, 0, strlen(multipart_boundary));

		EM_DEBUG_LOG("end [%p], txt_html [%p]", end, txt_html);

		if (end == NULL) {
			EM_DEBUG_LOG("HTML body contents exceeds %d Bytes", PARTIAL_BODY_SIZE_IN_BYTES);
			
			end = txt_html + body_size - (txt_html - bufsendforparse);
			html_uidno = 1;
		}
		else if(end == txt_html) { /* empty multipart */
			EM_DEBUG_LOG("Emtpy HTML multipart");
			return false;
		}						
		else {
			if ((*(end-2) == CR) && (*(end-1) == LF))
				end -= 2;
			else if ((*(end-2) == CR) && (*(end-1) == LF) 
				    && (*(end-4) == CR) && (*(end-3) == LF))
				end -= 4;
			else
				EM_DEBUG_EXCEPTION(" Content not per as grammar.");
		}

		EM_DEBUG_LOG("end [%p], txt_html [%p]", end, txt_html);

		EM_DEBUG_LOG("iEncodingHeader [%d]", iEncodingHeader);

		if (iEncodingHeader == 1){
			enc_type = ENCOTHER;
			if (strncasecmp(Encoding, "base64", strlen("base64")) == 0)
				enc_type = ENCBASE64;
			else if (strncasecmp(Encoding, "quoted-printable", strlen("quoted-printable")) == 0)
				enc_type = ENCQUOTEDPRINTABLE;
	
			EM_DEBUG_LOG("enc_type [%d]", enc_type);

			memcpy(text_html, start, end - txt_html);
			
			if (em_core_decode_body_text(text_html, end - txt_html, enc_type , &dec_len, &err) < 0) 
				EM_DEBUG_EXCEPTION("em_core_decode_body_text failed [%d]", err);
		}
		else if (start_header && ((temp_enc1 = (char *)strcasestr(start_header, "Content-transfer-encoding:"))  != NULL) && !(temp_enc1 && temp_enc1 >= end)){
			if (temp_enc1)
				start_header = temp_enc1;

			start_header += strlen("Content-Transfer-Encoding:");
			start_header = em_core_skip_whitespace_without_strdup(start_header);

			if (!start_header)
				EM_DEBUG_EXCEPTION(" Invalid parsing ");
			else{
				enc_type = ENCOTHER;
				if (strncasecmp(start_header, "base64", strlen("base64")) == 0)
					enc_type = ENCBASE64;
				else if (strncasecmp(start_header, "quoted-printable", strlen("quoted-printable")) == 0)
					enc_type = ENCQUOTEDPRINTABLE;

				EM_DEBUG_LOG("enc_type [%d]", enc_type);
				
				memcpy(text_html, start, end - txt_html);
				
				if (em_core_decode_body_text(text_html, end - txt_html, enc_type , &dec_len, &err) < 0) 
					EM_DEBUG_EXCEPTION("em_core_decode_body_text failed [%d]", err);
				html_length = dec_len;
			}
			
			EM_DEBUG_LOG("Decoded length = %d", dec_len);
			EM_DEBUG_LOG("start - %s", start);
		}
		else{
			memcpy(text_html, start, end-txt_html);
			html_length = (end-txt_html);
		}

		/* EM_DEBUG_LOG(" Content-Type:  text/html [%s]\n", text_html);								 */
	}
	else
		EM_DEBUG_EXCEPTION(" Invalid html body content ");

	EM_DEBUG_FUNC_END();
	return true;
}



/*For the following scenario
*Content-Transfer-Encoding :  base64
*Content-Type :  text/plain; charset = "windows-1252"
*MIME-Version :  1.0
*Message-ID:  <11512468.945901271910226702.JavaMail.weblogic@epml03>
*/

#define CONTENT_TRANSFER_ENCODING "Content-Transfer-Encoding"

static int em_core_parse_plain_part_for_partial_body(char *header_start_string, char *start_header, char *boundary_string, char *bufsendforparse, char *text_plain, int body_size)
{
	EM_DEBUG_FUNC_BEGIN("header_start_string[%s], start_header[%s], boundary_string [%s], bufsendforparse [%s], text_plain [%s]", header_start_string, start_header, boundary_string, bufsendforparse, text_plain);
	int   err = EMF_ERROR_NONE, iEncodingHeader = 0, enc_type = ENCOTHER;
	int  dec_len = 0, strcmpret = -1;
	char *pHeaderStart = NULL, *pEncodingHeaderEnd = NULL;
	char  EncodingHeader[40] = {0, };
	char  Encoding[30] = {0, };
	char *temp_text_boundary = NULL;
	char *start = NULL, *end = NULL, *txt_plain = NULL, *temp_enc1 = NULL;

	EM_DEBUG_LOG("Content-Type : text/plain");

	pHeaderStart = header_start_string; 
	temp_text_boundary = start_header; 

	memcpy(EncodingHeader, pHeaderStart, 25);

	if (strcasecmp(EncodingHeader, "Content-Transfer-Encoding") == 0){
		pEncodingHeaderEnd = strstr(pHeaderStart, CRLF_STRING);
		memcpy(Encoding, pHeaderStart + 27, pEncodingHeaderEnd - (pHeaderStart + 27));
		iEncodingHeader = 1;
	}
	
	/*  Plain text content found  */
	
	txt_plain = start_header;
	txt_plain = strstr(txt_plain, CRLF_STRING CRLF_STRING);

	if (txt_plain != NULL){
		txt_plain += strlen(CRLF_STRING CRLF_STRING); /*  txt_plain points at plain text content  */
		
		/* Fix is done for mail having "Content-Type: text/plain" but there is no content but having only attachment. */
		
		strcmpret = strncmp(txt_plain, boundary_string, strlen(boundary_string));
		if (strcmpret == 0){
		}
		else{
			start = txt_plain;
			end = strstr(txt_plain, boundary_string);
			
			if (end == NULL){
				EM_DEBUG_LOG("Text body contents exceeds %d Bytes", PARTIAL_BODY_SIZE_IN_BYTES);
				end = txt_plain + body_size - (txt_plain - bufsendforparse);
			}
			else{
				/* EM_DEBUG_LOG("pbd_event[temp_count].partial_body_complete - %d", partial_body_complete); */
	
				if ((*(end-2) == CR) && (*(end-1) == LF))
					end -= 2;
				else if ((*(end-2) == CR) && (*(end-1) == LF) 
					    && (*(end-4) == CR) && (*(end-3) == LF))
					end -= 4;
				else
					EM_DEBUG_EXCEPTION(" Content not per as grammar.");
			}

			if (iEncodingHeader == 1){
				enc_type = ENCOTHER;
				if (strncasecmp(Encoding, "base64", strlen("base64")) == 0)
					enc_type = ENCBASE64;
				else if (strncasecmp(Encoding, "quoted-printable", strlen("quoted-printable")) == 0)
					enc_type = ENCQUOTEDPRINTABLE;
		
				EM_DEBUG_LOG("enc_type [%d]", enc_type);

				memcpy(text_plain, start, end - txt_plain);
				
				if (em_core_decode_body_text(text_plain, end - txt_plain, enc_type , &dec_len, &err) < 0) 
					EM_DEBUG_EXCEPTION("em_core_decode_body_text failed [%d]", err);
			}
			else if (start_header && ((temp_enc1 = (char *)strcasestr(start_header, "Content-transfer-encoding:"))  != NULL) && !(temp_enc1 && temp_enc1 >= end)){
				if (temp_enc1)
					start_header = temp_enc1;

				start_header += strlen("Content-Transfer-Encoding:");
				start_header = em_core_skip_whitespace_without_strdup(start_header);

				if (!start_header)
					EM_DEBUG_EXCEPTION(" Invalid parsing ");
				else{
					enc_type = ENCOTHER;
					if (strncasecmp(start_header, "base64", strlen("base64")) == 0)
						enc_type = ENCBASE64;
					else if (strncasecmp(start_header, "quoted-printable", strlen("quoted-printable")) == 0)
						enc_type = ENCQUOTEDPRINTABLE;
			
					EM_DEBUG_LOG("enc_type [%d]", enc_type);
					memcpy(text_plain, start, end - txt_plain);
					if (em_core_decode_body_text(text_plain, end - txt_plain, enc_type , &dec_len, &err) < 0) 
						EM_DEBUG_EXCEPTION("em_core_decode_body_text failed [%d]", err);
				}
				
				EM_DEBUG_LOG("Decoded length = %d", dec_len);
				/*  EM_DEBUG_LOG("start - %s", start); */ /* print raw MIME content. */
			}
			else
				memcpy(text_plain, start, end-txt_plain);

			/* EM_DEBUG_LOG(" Content-type: text/plain [%s]\n", text_plain); */
		}
	}
	else
		EM_DEBUG_EXCEPTION(" Invalid text body content ");

	EM_DEBUG_FUNC_END();
	return 1;
}



/*  Content-Type :  IMAGE/octet-stream; name = Default.png */
/*  Content-Transfer-Encoding :  BASE64 */
/*  Content-ID:  <4b0d6810b17291f9438783a8eb9d5228@email-service> */
/*  Content-Disposition :  inline; filename = Default.png */

static int em_core_parse_image_part_for_partial_body(char *header_start_string, char *start_header, char *boundary_string, char *bufsendforparse, emf_image_data *image_data, int body_size)
{
	EM_DEBUG_FUNC_BEGIN();

	int   err = EMF_ERROR_NONE;
	char *multiple_image = NULL;
	int donot_parse_next_image = 0;
	char *image_boundary = NULL;
	char *image_boundary_end = NULL;
	char temp_image_boundary[256] = {0};
	int i = 0, ch_image = 0, cidno = 0;
	int   enc_type = ENCOTHER, dec_len = 0, image_length = 0;
	char *p = header_start_string;
	char *start = NULL, *end = NULL, *txt_image = NULL;
	char *temp_image = NULL;
	char *temp_cid1 = NULL;
	char *cid_end = NULL;
	char *temp_enc1 = NULL;


	image_boundary     = start_header;
	image_boundary_end = image_boundary - 2;
	image_boundary     = image_boundary - 2;
	
	EM_DEBUG_LOG("Content-type: image");
	
	while (*image_boundary != LF)
		image_boundary--;
	
	image_boundary++;
	
	if (image_boundary  != NULL && image_boundary_end != NULL)
		memcpy(temp_image_boundary, image_boundary, image_boundary_end-image_boundary);
	
	if ((char *)strcasestr((const char *)temp_image_boundary, "Content-type:") == NULL)
		boundary_string = temp_image_boundary;
	do {
		if (multiple_image  != NULL){
			p = multiple_image;
			start_header = multiple_image;
		}
		if ((strcasestr(p, "Content-Disposition: attachment")) || (!strcasestr(p, "Content-ID: <"))){
			EM_DEBUG_EXCEPTION(" Body has attachment no need to parse ");
			end = NULL;
			multiple_image = NULL;
		}
		else{	/*  HTML Content found */
			ch_image = 0;
			cidno = 0;
			int   boundarylen = -1;
			char *cid = NULL;
			char *temp_name = NULL;

			memset(image_data[i].image_file_name, 0, 100);
			txt_image  = start_header;
			temp_image = start_header;

			temp_name = strstr(txt_image, "name=");
			if (temp_name  != NULL){
				temp_image = temp_name;
				if (*(temp_image+5) == '"')
					temp_image = temp_image+5;
				
				while (*temp_image  != CR){
					temp_image++;
					memcpy(image_data[i].image_file_name+ch_image, temp_image, 1);
					ch_image++;
				}
				
				if ((*(temp_name+4) == '=') && (*(temp_name+5) == '\"'))
					image_data[i].image_file_name[ch_image-2] = '\0';
				
				if ((*(temp_name+4) == '=') && (*(temp_name+5) != '\"'))
					image_data[i].image_file_name[ch_image-1] = '\0';
			}		
			
			if (((temp_cid1 = (char *)strcasestr((const char *)start_header, "Content-ID: <")) != NULL)){
				if (temp_cid1){
					cid = temp_cid1;
					temp_image = temp_cid1;
				}
				
				cid += 13;
				cid_end = strstr(temp_image, "\076");		/*  076 == '>' */
			
				image_data[i].content_id = (char *)em_core_malloc(cid_end-cid+5);
				if (image_data[i].content_id  != NULL){
					strcpy(image_data[i].content_id, "cid:");
					memcpy(image_data[i].content_id+4, cid, cid_end-cid);
				}
				else
					EM_DEBUG_EXCEPTION("em_core_malloc() failed");
				
				if (image_data[i].image_file_name[0] == '\0')
					memcpy(image_data[i].image_file_name, cid, cid_end - cid);
 		   	}

			txt_image = strstr(txt_image, CRLF_STRING CRLF_STRING);

			if (txt_image  != NULL){
				txt_image  += 4; /*  txt_image points at image content */
				start = txt_image;
				end = strstr(txt_image, boundary_string);


				if (end == NULL){
					EM_DEBUG_LOG("HTML body contents exceeds %d Bytes", PARTIAL_BODY_SIZE_IN_BYTES);
					/*  end points to end of partial body data */
					end = txt_image + body_size - (txt_image-bufsendforparse);
				}
				else{
					boundarylen = strlen(boundary_string);
					end -= 2;
				}
						
				if (start_header && ((temp_enc1 = (char *)strcasestr((const char *)start_header, "Content-transfer-encoding:"))  != NULL)){
					if (temp_enc1)
						start_header = temp_enc1;

					start_header  += strlen("Content-Transfer-Encoding:");
					start_header = em_core_skip_whitespace_without_strdup(start_header);
					
					if (!start_header)
						EM_DEBUG_EXCEPTION(" Invalid parsing ");
					else{
						enc_type = ENCOTHER;
						if (strncasecmp(start_header, "base64", strlen("base64")) == 0)
							enc_type = ENCBASE64;
						else if (strncasecmp(start_header, "quoted-printable", strlen("quoted-printable")) == 0)
							enc_type = ENCQUOTEDPRINTABLE;

						EM_DEBUG_LOG("enc_type [%d]", enc_type);
						
						image_data[i].text_image = (char *)em_core_malloc((end-txt_image)+1);
						if (image_data[i].text_image){
							memcpy(image_data[i].text_image, start, end-txt_image);
							if (em_core_decode_body_text(image_data[i].text_image, end-txt_image, enc_type , &(image_data[i].dec_len), &err) < 0) 
								EM_DEBUG_EXCEPTION("em_core_decode_body_text failed [%d]", err);
							else
								image_length = image_data[i].dec_len;
						}
						else
							EM_DEBUG_EXCEPTION("em_core_malloc() failed");
					}
					
					EM_DEBUG_LOG("Decoded length [%d]", dec_len);
				}
				else{
					image_data[i].text_image = (char *)em_core_malloc(end-txt_image);
					if (image_data[i].text_image)
						memcpy(image_data[i].text_image, start, end - txt_image);
					else
						EM_DEBUG_EXCEPTION("em_core_malloc() failed");
				}							
			}
			else{
				donot_parse_next_image = 1;
				EM_DEBUG_EXCEPTION(" Invalid html body content ");
			}
		}

		if (end != NULL) {
			multiple_image = (char *)strcasestr((const char *)end, "Content-type: image");
			i++;
		}
	} while (multiple_image != NULL && donot_parse_next_image != 1 && (i < IMAGE_DISPLAY_PARTIAL_BODY_COUNT));

	EM_DEBUG_FUNC_END();
	return 1;
}

static int em_core_find_boundary_string_of_the_part(const char *whole_string, const char *first_line_of_part, char **result_boundary_string, int *error)
{
	EM_DEBUG_FUNC_BEGIN("whole_string[%p], first_line_of_part[%p], result_boundary_string[%p]", whole_string, first_line_of_part, result_boundary_string);
	int ret = false, err = EMF_ERROR_NONE;
	char *boundary_cur = NULL, *boundary_end = NULL, *boundary_string = NULL;

	if(!whole_string || !first_line_of_part || !result_boundary_string) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if(first_line_of_part - 2 > whole_string) {
		boundary_cur = (char*)first_line_of_part - 2; /* 2 means CRLF. */
		boundary_end = boundary_cur;

		do{
			boundary_cur--;
		} while (whole_string < boundary_cur && *boundary_cur != LF && *boundary_cur != NULL_CHAR);
		
		boundary_cur++;

		if(boundary_end > boundary_cur && boundary_cur > whole_string) {
			EM_DEBUG_LOG("boundary_end - boundary_cur + 15 [%d]", boundary_end - boundary_cur + 15);
			boundary_string = em_core_malloc(boundary_end - boundary_cur + 15);
			if(!boundary_string) {
				EM_DEBUG_EXCEPTION("em_core_malloc failed");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			memcpy(boundary_string, boundary_cur, boundary_end - boundary_cur);
			EM_DEBUG_LOG("boundary_string [%s]", boundary_string);
			*result_boundary_string = boundary_string;
		}
		else {
			EM_DEBUG_EXCEPTION("There is no string before the part");
			err = EMF_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}
	}
	else {
		EM_DEBUG_EXCEPTION("There is no string before the part");
		err = EMF_ERROR_ON_PARSING;
		goto FINISH_OFF;	
	}
	ret = true;
FINISH_OFF:

	if(error)
		*error = err;

	EM_DEBUG_FUNC_END("ret[%d], err[%d]", ret, err);
	return ret;
} 

#define TAG_LENGTH 16
#define COMMAND_LENGTH 2000
EXPORT_API int em_core_bulk_partial_mailbody_download_for_imap(MAILSTREAM *stream, emf_event_partial_body_thd *pbd_event, int item_count, int *error)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p], pbd_event [%p], item_count [%d], error [%p]", stream, pbd_event, item_count, error);
	
	int ret = false, err = EMF_ERROR_NONE;
	int encoding = 0;
	int j = 0;
	int i32_index = 0, temp_string_index = 0;
	int no_alternative_part_flag = 0;
	int dec_len = 0, response_buffer_length = 0, mailparselen = 0, image_length = 0, tempmailparselen = 0;
	int temp_count = 0, total_mail_size = 0, attachment_num, body_size = 0, total_mail_size_except_attach = 0;
	int total_parsed_len_per_uid = 0, total_parsed_len = 0;
	unsigned long uidno = 0;
	char buf[512] = {0, };
	char text_plain[PARTIAL_BODY_SIZE_IN_BYTES + 1] = {0, };	
	char text_html[PARTIAL_BODY_SIZE_IN_BYTES + 1] = {0, };	
	char temp_text_buf[PARTIAL_BODY_SIZE_IN_BYTES + 1] = {0, };
	char uid_range_string_to_be_downloaded[UID_RANGE_STRING_LENGTH] = {0, };	
	char imap_tag[TAG_LENGTH] = {0, };
	char command[COMMAND_LENGTH] = {0, };
	char *p = NULL, *s = NULL, *decoded_text_buffer = NULL;
	char *response_buffer = NULL;
	char *bufsendforparse = NULL;
	char *start_header = NULL;
	char *boundary_string = NULL;
	char *temp_content_type1 = NULL;
	char *bodystructure_start = NULL, *bodystructure_end = NULL, *body_structure_string = NULL, *moified_body_structure_string = NULL;
	char *plain_text_file_name_from_content_info = NULL, *html_text_file_name_from_content_info = NULL, *plain_charset_from_content_info = NULL;
	char temp_string[TEMP_STRING_LENGTH] = {0, };
	IMAPLOCAL *imaplocal = NULL;
	IMAPPARSEDREPLY *reply_from_server = NULL;
	emf_mail_tbl_t *mail = NULL;
	emf_partial_buffer *imap_response = NULL;
	BODY *body = NULL;
	struct _m_content_info *cnt_info = NULL;
	struct attachment_info *attach_info = NULL;
	emf_mail_attachment_tbl_t attachment_tbl;
	emf_event_partial_body_thd *stSectionNo = NULL;
	emf_image_data  image_data[IMAGE_DISPLAY_PARTIAL_BODY_COUNT];

	if (!(stream) || !(imaplocal = stream->local) || !imaplocal->netstream || !pbd_event) {
		EM_DEBUG_EXCEPTION("invalid parameter");
		err = EMF_ERROR_INVALID_PARAM;		
		EM_DEBUG_FUNC_END("ret [%d]", ret);	
		return ret;
	}

	memset(image_data, 0x00 , sizeof(emf_image_data) * IMAGE_DISPLAY_PARTIAL_BODY_COUNT);
	
	EM_DEBUG_LOG("Start of em_core_get_section_for_partial_download, item_count = %d ", item_count);
		
	/* For constructing UID list which is having 10 UID or less at a time */
	for (j = 0, stSectionNo = pbd_event; (stSectionNo  != NULL && j < item_count); j++)
 	{		
 		EM_DEBUG_LOG("pbd_event[%d].account_id [%d], mail_id [%d], server_mail_id [%d], activity_id [%d]", \
			j, stSectionNo[j].account_id, stSectionNo[j].mail_id, stSectionNo[j].server_mail_id, stSectionNo[j].activity_id);		
		
		if (i32_index >= UID_RANGE_STRING_LENGTH){
			EM_DEBUG_EXCEPTION("String length exceeded its limitation!");
			goto FINISH_OFF;
		}			

		if (j == item_count - 1)
			i32_index += SNPRINTF(uid_range_string_to_be_downloaded + i32_index, UID_RANGE_STRING_LENGTH, "%lu", stSectionNo[j].server_mail_id);
		else
			i32_index += SNPRINTF(uid_range_string_to_be_downloaded + i32_index, UID_RANGE_STRING_LENGTH, "%lu,", stSectionNo[j].server_mail_id);
	}	

	SNPRINTF(imap_tag, TAG_LENGTH, "%08lx", 0xffffffff & (stream->gensym++));
	SNPRINTF(command, COMMAND_LENGTH, "%s UID FETCH %s (BODYSTRUCTURE BODY.PEEK[TEXT]<0.15360>)\015\012", imap_tag, uid_range_string_to_be_downloaded);

	mm_dlog(command);

	/*  Sending out the IMAP request */
	if (!net_sout(imaplocal->netstream, command, (int)strlen(command))) {
		EM_DEBUG_EXCEPTION("net_sout failed...");
		err = EMF_ERROR_CONNECTION_BROKEN;		
		goto FINISH_OFF;
	}

	/*  responce from the server */
	imap_response = em_core_get_response_from_server(imaplocal->netstream, imap_tag, &reply_from_server);

	if (!imap_response || !reply_from_server ){
		EM_DEBUG_EXCEPTION(" Invalid response from em_core_get_response_from_server");
		goto FINISH_OFF;
	}
	
	if (!imap_response->buffer || imap_response->buflen == 0){
		EM_DEBUG_EXCEPTION(" NULL partial BODY Content ");
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("imap_response->buffer [%s]",imap_response->buffer);
	response_buffer        = imap_response->buffer;
	response_buffer_length = imap_response->buflen;

	while (response_buffer && (bodystructure_start = strstr(response_buffer, "BODYSTRUCTURE (")) != NULL){
		/*  if it has BODYSTRUCTURE */
		EM_DEBUG_LOG("response_buffer [%s]",response_buffer);
		bodystructure_start = bodystructure_start + strlen("BODYSTRUCTURE");
		bodystructure_end = strstr(bodystructure_start, "BODY[");
		
		if (bodystructure_end != NULL){
			int bodystructure_length = bodystructure_end - bodystructure_start;
			
			EM_DEBUG_LOG("bodystructure_length [%d]", bodystructure_length);

			if (bodystructure_length > response_buffer_length){
				EM_DEBUG_EXCEPTION("bodystructure_length[%d] is longer than response_buffer_length[%d]", bodystructure_length, response_buffer_length);
				err = EMF_ERROR_INVALID_RESPONSE;
				goto FINISH_OFF;
			}
			
			body_structure_string = (char *)em_core_malloc(sizeof(char) * bodystructure_length + 1);

			if (NULL == body_structure_string){
				EM_DEBUG_EXCEPTION("em_core_malloc failed...!");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			memcpy(body_structure_string, bodystructure_start, bodystructure_length);

			body = mail_newbody();
			
			if (NULL == body){
				EM_DEBUG_EXCEPTION("New body creation failed...!");
				EM_SAFE_FREE(body_structure_string);
				goto FINISH_OFF;
			}

			/*  Parse body_structure_string to BODY  */
			EM_DEBUG_LOG("body_structure_string [%s]", body_structure_string);
			/* body_structure_string modified */
			moified_body_structure_string = em_core_replace_string(body_structure_string, "}\r\n", "} ");
			if (moified_body_structure_string != NULL) {
				EM_SAFE_STRNCPY(body_structure_string, moified_body_structure_string, strlen(moified_body_structure_string));
				EM_DEBUG_LOG("moified_body_structure_string [%s]", moified_body_structure_string);
			}
			imap_parse_body_structure (stream, body, (unsigned char **)&body_structure_string, reply_from_server);
			
			total_mail_size = 0;

			if (em_core_set_fetch_body_section(body, true, &total_mail_size, &err) < 0) {
				EM_DEBUG_EXCEPTION("em_core_set_fetch_body_section failed - %d", err);
				goto FINISH_OFF;
			}
			
			if (!(cnt_info = em_core_malloc(sizeof(struct _m_content_info)))) {
				EM_DEBUG_EXCEPTION("em_core_malloc failed...");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
	
			memset(cnt_info, 0, sizeof(struct _m_content_info));

			/*  getting content info from body  */
						
			if (em_core_get_body(stream, 0, 0, 0, body, cnt_info, &err) < 0 || !cnt_info) {
				EM_DEBUG_EXCEPTION("em_core_get_body falied [%d]", err);
				err = EMF_ERROR_IMAP4_FETCH_UID_FAILURE;
				goto FINISH_OFF;
			}

			EM_DEBUG_LOG("Start parsing partial body...");
			
			int no_html = 0;
			char *temp_alternative_plain_header = NULL;
			
			p = (char *)strstr(response_buffer, " {");
			if (p  != NULL){
				EM_DEBUG_LOG("Getting the body size");
				p += strlen(" {");
				s = p;

				temp_string_index = 0;
				memset(temp_string, 0, TEMP_STRING_LENGTH);
			
				while (isdigit(*s) && temp_string_index < TEMP_STRING_LENGTH){
					memcpy(temp_string + temp_string_index, s, 1); /* ! */
					s++;
					temp_string_index++;
				}
				
				body_size = atoi(temp_string);
				EM_DEBUG_LOG("body_size [%d]", body_size);
			}
			else{
				body_size = 0;
				EM_DEBUG_EXCEPTION("Can't find body size from MIME header");
				/* err = EMF_ERROR_INVALID_RESPONSE; */
				/* goto FINISH_OFF; */
			}
					
			/*  Find next line */
			tempmailparselen = 0;
			while (tempmailparselen < response_buffer_length && response_buffer[tempmailparselen] != LF)
				tempmailparselen++;
			tempmailparselen++;
			
			if (imap_response->buflen < (total_parsed_len + body_size)){
				err = EMF_ERROR_CONNECTION_BROKEN;
				EM_DEBUG_EXCEPTION("EMF_ERROR_CONNECTION_BROKEN  :  imap_response->buflen [%d], total_parsed_len [%d], body_size [%d]", imap_response->buflen, total_parsed_len, body_size);
				goto FINISH_OFF;
			}
			else {   
				if ((p = strstr(response_buffer, "UID "))) {
					EM_DEBUG_LOG("getting the UID number");
					p += strlen("UID ");
					s = p;
					
					temp_string_index = 0;
					memset(temp_string, 0, TEMP_STRING_LENGTH);			
		
					while (isdigit(*s) && temp_string_index < TEMP_STRING_LENGTH){
						memcpy(temp_string + temp_string_index, s, 1);
						s++;
						temp_string_index++;
					}
					
					uidno = strtoul(temp_string, NULL, 0);
					EM_DEBUG_LOG("UID [%d]", uidno);
					
					for (temp_count = 0; temp_count < BULK_PARTIAL_BODY_DOWNLOAD_COUNT && 
										pbd_event[temp_count].server_mail_id  != uidno && temp_count  != item_count; temp_count++)
						continue;

					if (temp_count >= BULK_PARTIAL_BODY_DOWNLOAD_COUNT){    
						EM_DEBUG_EXCEPTION("Can't find proper server_mail_id");
						goto FINISH_OFF;
					}

					EM_SAFE_FREE(plain_text_file_name_from_content_info);
					EM_SAFE_FREE(html_text_file_name_from_content_info);
					EM_SAFE_FREE(plain_charset_from_content_info);

					/*  partial_body_complete = -1; */ /* Meaningless */
					/*  encoding = -1; */ /* Meaningless */
					
					plain_text_file_name_from_content_info    = EM_SAFE_STRDUP(cnt_info->text.plain);
					html_text_file_name_from_content_info     = EM_SAFE_STRDUP(cnt_info->text.html);
					plain_charset_from_content_info           = EM_SAFE_STRDUP(cnt_info->text.plain_charset);

					EM_DEBUG_LOG("plain_text_file_name_from_content_info    [%s]", plain_text_file_name_from_content_info);
					EM_DEBUG_LOG("html_text_file_name_from_content_info     [%s]", html_text_file_name_from_content_info);
					EM_DEBUG_LOG("plain_charset_from_content_info           [%s]", plain_charset_from_content_info);

					encoding = body->encoding;
					
					if (!em_storage_get_maildata_by_servermailid(pbd_event[temp_count].account_id, temp_string, &mail , true, &err) || !mail){
						EM_DEBUG_EXCEPTION("em_storage_get_maildata_by_servermailid failed [%d]", err);
						if (err == EM_STORAGE_ERROR_MAIL_NOT_FOUND || !mail)
						  	goto FINISH_OFF;
					}
					
					/*  Assign calculated mail size  */
					mail->mail_size               = total_mail_size;
					total_mail_size_except_attach = total_mail_size;

					/*  Update attachment details except inline content */
					if (cnt_info->file && cnt_info->file->name){
						memset(&attachment_tbl, 0x00, sizeof(emf_attachment_info_t));

						attachment_tbl.account_id   = pbd_event[temp_count].account_id;
						attachment_tbl.mail_id      = mail->mail_id;
						attachment_tbl.mailbox_name = pbd_event[temp_count].mailbox_name; 	
						attachment_tbl.file_yn      = 0; 

						for (attachment_num = 1, attach_info = cnt_info->file; attach_info; attach_info = attach_info->next, attachment_num++) {
							total_mail_size_except_attach -= attach_info->size;
							attachment_tbl.attachment_size = attach_info->size;
							attachment_tbl.attachment_path = attach_info->save;
							attachment_tbl.attachment_name = attach_info->name;
							attachment_tbl.flag2 = attach_info->drm;
							attachment_tbl.flag3 = (attach_info->type == 1) ? 1 : 0;
#ifdef __ATTACHMENT_OPTI__
							attachment_tbl.encoding = attach_info->encoding;
							attachment_tbl.section = attach_info->section;
#endif
							
							if (attach_info->type == 1) {
								EM_DEBUG_LOG("Breaking attachment_num [%d] attach_info->type [%d]", attachment_num, attach_info->type);
								break;		/*  Inline images at end of list  not to be added  */
							}

							EM_DEBUG_LOG("attach_info->type  != 1 [%d]", attachment_num);

							mail->attachment_count++;
							if (!em_storage_add_attachment(&attachment_tbl, 0, false, &err)) {
								EM_DEBUG_EXCEPTION("em_storage_add_attachment failed [%d]", err);					
								err = em_storage_get_emf_error_from_em_storage_error(err);
								goto FINISH_OFF;
							}
							
						}								
					}
											
					em_core_mime_free_content_info(cnt_info);
					cnt_info = NULL;
					
					mail_free_body(&body);	
					body = NULL;

				}
		
				/*  Find next line. */
				mailparselen = 0;
				
				while (mailparselen < response_buffer_length && response_buffer[mailparselen] != LF)
					mailparselen++;
				
				mailparselen++;
	
				/*  Removed the response header and send the buffer for parsing */
				response_buffer        = response_buffer + mailparselen;
				response_buffer_length = response_buffer_length - mailparselen;

				bufsendforparse = em_core_malloc(body_size + 1); /*  old */
				/*  bufsendforparse = em_core_malloc(response_buffer_length + 1); */ /* new */

				EM_DEBUG_LOG("body_size [%d], response_buffer_length [%d]", body_size, response_buffer_length);

				if (bufsendforparse == NULL){
					EM_DEBUG_EXCEPTION("Allocation for bufsendforparse failed.");
					err = EMF_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}
				
				memcpy(bufsendforparse, response_buffer, body_size); /* old */
				bufsendforparse[body_size] = NULL_CHAR; /* old */
				/* EM_SAFE_STRNCPY(bufsendforparse, response_buffer, response_buffer_length);*/ /* new */
				/* bufsendforparse[response_buffer_length] = '\0'; */ /* new */
		
				if (strlen(bufsendforparse) == 0)
					EM_DEBUG_EXCEPTION(" NULL partial BODY Content ");

				EM_DEBUG_LOG("string bufendforparse :[%s]", bufsendforparse);
				p = bufsendforparse;
				EM_DEBUG_LOG("string p :[%s]", p);
				if (mail && mail->body_download_status == EMF_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED) /*  No need to save */
					goto NEXTUIDPARSING;
				
				if (!strcasestr(p, "Content-Type: text/plain") &&  !strcasestr(p, "Content-Type: text/html") &&  !strcasestr(p, "Content-type: image/jpeg")
					&&  !strcasestr(p, "Content-Type: image/gif") && !strcasestr(p, "Content-Type: image/bmp")&&(plain_text_file_name_from_content_info || html_text_file_name_from_content_info)){
					/*  Encoded Content-Type: text/html or Content-Type: text/plain  */
					/*  No Partial body has No headers with Content-Type: text/html or Content-Type: text/plain  */
					
					EM_DEBUG_LOG("plain_text_file_name_from_content_info [%p] html_text_file_name_from_content_info[%p] ", plain_text_file_name_from_content_info, html_text_file_name_from_content_info);
					EM_DEBUG_LOG("mbody->encoding [%d] ", encoding);

					if (em_core_decode_body_text(p, strlen(p), encoding , &dec_len, &err) < 0) {
						EM_DEBUG_EXCEPTION("em_core_decode_body_text failed [%d]", err);
						goto FINISH_OFF;
					}

					decoded_text_buffer = p;

					EM_DEBUG_LOG("Decoded length [%d]", dec_len);
					/*  EM_DEBUG_LOG("p - %s", p); */
		
					if (dec_len > 0){
						if (plain_text_file_name_from_content_info){
							EM_DEBUG_LOG(" plain_text_file_name_from_content_info [%s]", plain_text_file_name_from_content_info);
							memcpy(text_plain, decoded_text_buffer, dec_len);
							/* EM_DEBUG_LOG(" Content-Type :  text/plain [%s]", text_plain); */
						}

						if (html_text_file_name_from_content_info){
							EM_DEBUG_LOG("html_text_file_name_from_content_info [%s]", html_text_file_name_from_content_info);
							memcpy(text_html, decoded_text_buffer, dec_len);
							/* EM_DEBUG_LOG(" Content-Type: text/html [%s]", text_html); */
						}
					}
				}
				else{   /*  Partial body has headers with Content-Type: text/html or Content-Type: text/plain */
					no_alternative_part_flag = 0;

					if (((temp_alternative_plain_header = (char *)strcasestr(p, "Content-type: multipart/alternative"))  != NULL)){	/*  Found 'alternative' */
						if (((temp_content_type1 = (char *)strcasestr(p, "Content-type: text/plain"))  != NULL)){   
							if (temp_content_type1 < temp_alternative_plain_header) /*  This part is text/plain not alternative. */
								no_html = 1;
							no_alternative_part_flag = 1;
						}
						else{
							EM_DEBUG_LOG(" Content-type: multipart/alternative ");
							p = strstr(bufsendforparse, CRLF_STRING CRLF_STRING);
							
							if (p == NULL)
								EM_DEBUG_EXCEPTION(" Incorrect parsing ");
							else{
								p += strlen(CRLF_STRING CRLF_STRING);
								boundary_string = p;
								if (boundary_string[0] == CR){
									EM_DEBUG_EXCEPTION(" Incorrect Body structure ");
									goto NEXTUIDPARSING;
								}
								
								p = strstr(p, CRLF_STRING);
								
								if (p == NULL)
									EM_DEBUG_EXCEPTION(" Invalid parsing ");
							}
						}
					}				
					else
						no_alternative_part_flag = 1; 

					if (no_alternative_part_flag){
						p = strstr(bufsendforparse, "--");

						boundary_string = p;
						if (boundary_string == NULL){
							EM_DEBUG_EXCEPTION("Should not have come here ");
							goto NEXTUIDPARSING;
						}
						
						if (boundary_string[0] == CR){
							EM_DEBUG_EXCEPTION(" Incorrect Body structure ");
							goto NEXTUIDPARSING;
						}
						
						p = strstr(p, CRLF_STRING);
						if (p == NULL)
							EM_DEBUG_EXCEPTION(" Invalid parsing ");
					}
		
					/* EM_DEBUG_LOG("p[%s]", p); */
	
					if (p  != NULL){
						*p = NULL_CHAR; /*  Boundary value set */
						EM_DEBUG_LOG("Boundary value [%s]", boundary_string);
						p  += 2; /*  p  points to content after boundary_string */
			
						if (((start_header = (char *)strcasestr(p, "Content-Type: text/html"))  != NULL) && (no_html  != 1) &&(((char *)strcasestr(p, "Content-Type: message/rfc822")) == NULL) &&
							(((char *)strcasestr(p, "Content-Type: text/rfc822-headers")) == NULL))
							em_core_parse_html_part_for_partial_body(start_header, boundary_string, bufsendforparse, text_html, body_size);

						if (((start_header = (char *)strcasestr(p, "Content-Type: text/plain"))  != NULL)) {
							char *internal_boundary_string = NULL;
							if(!em_core_find_boundary_string_of_the_part(bufsendforparse, start_header, &internal_boundary_string, &err)) {
								EM_DEBUG_EXCEPTION("internal_boundary_string failed [%d]", err);
							}

							if(!internal_boundary_string)
								internal_boundary_string = EM_SAFE_STRDUP(boundary_string);
								
							em_core_parse_plain_part_for_partial_body(p, start_header, internal_boundary_string, bufsendforparse, text_plain, body_size);
							EM_SAFE_FREE(internal_boundary_string);
						}

						if (((start_header = (char *)strcasestr((const char *)p, "Content-type: image/jpeg"))  != (char *)NULL) || 
							((start_header = (char *)strcasestr((const char *)p, "Content-Type: image/gif"))  != (char *)NULL) || 
							((start_header = (char *)strcasestr((const char *)p, "Content-Type: image/bmp"))  != (char *)NULL))
							em_core_parse_image_part_for_partial_body(p, start_header, boundary_string, bufsendforparse, image_data, body_size);
					}
				}
				
				/*  Updating mail information  */
				memset(buf, 0x00, sizeof(buf));
			
				if (!em_storage_create_dir(pbd_event[temp_count].account_id, mail->mail_id, 0, &err))
					EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
	
				if (!em_storage_get_save_name(pbd_event[temp_count].account_id, mail->mail_id, 0, 
									plain_charset_from_content_info ? plain_charset_from_content_info  :  "UTF-8", buf, &err)) 
					EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
	
				if (!em_storage_create_file(text_plain, strlen(text_plain), buf, &err))
					EM_DEBUG_EXCEPTION("em_storage_create_file failed [%d]", err);

				mail->file_path_plain = EM_SAFE_STRDUP(buf); 
				EM_DEBUG_LOG("mail->file_path_plain [%s]", mail->file_path_plain);

				if (image_data[0].text_image  != NULL && image_data[0].text_image[0]  != NULL_CHAR){
					char *result_string_of_replcaing = NULL;
					char temp_data_html[PARTIAL_BODY_SIZE_IN_BYTES + 1] = {0};
					int store_file = 0;
					int content_index = 0;

					memset(buf, 0x00, sizeof(buf));
					if (text_html[0]  != NULL_CHAR)
						memcpy(temp_data_html, text_html, strlen(text_html));
						/* EM_SAFE_STRNCPY(temp_data_html, text_html, text_html); */

					do {
						if (!em_storage_create_dir(pbd_event[temp_count].account_id, mail->mail_id, 0, &err))
							EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);

						if (!em_storage_get_save_name(pbd_event[temp_count].account_id, mail->mail_id, 0, image_data[store_file].image_file_name, buf, &err)) 
							EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);

						if (!em_storage_create_file(image_data[store_file].text_image, image_data[store_file].dec_len, buf, &err))
							EM_DEBUG_EXCEPTION("em_storage_create_file failed [%d]", err);
						
						if (mail->body_download_status  != EMF_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED){
							memset(&attachment_tbl, 0x00, sizeof(emf_mail_attachment_tbl_t));
							attachment_tbl.mail_id = mail->mail_id;
							attachment_tbl.account_id = pbd_event[temp_count].account_id;
							attachment_tbl.mailbox_name = stream->mailbox;
							attachment_tbl.attachment_name = image_data[store_file].image_file_name;
							attachment_tbl.attachment_size = image_data[store_file].dec_len;
							attachment_tbl.attachment_path = buf;
							attachment_tbl.file_yn = 1;
							attachment_tbl.flag3 = 1; /*  set to 1 for inline image */
							mail->attachment_count++;
							mail->inline_content_count++;
							if (!em_storage_add_attachment (&attachment_tbl, false, false, &err))
								EM_DEBUG_EXCEPTION("em_storage_add_attachment failed - %d", err);									
						}
						
						store_file++;
					} while (image_data[store_file].text_image  != NULL && image_data[store_file].text_image[0]  != NULL_CHAR && (store_file < IMAGE_DISPLAY_PARTIAL_BODY_COUNT));
					
					while (image_data[content_index].text_image  != NULL && image_data[content_index].text_image[0]  != NULL_CHAR &&
						  image_data[content_index].content_id && image_data[content_index].content_id[0]  != NULL_CHAR && (content_index < IMAGE_DISPLAY_PARTIAL_BODY_COUNT)){   /*  Finding CID in HTML and replacing with image name. */
						result_string_of_replcaing = em_core_replace_string((char *)temp_data_html, (char *)image_data[content_index].content_id, (char *)image_data[content_index].image_file_name);
						
						EM_SAFE_STRNCPY(temp_data_html, result_string_of_replcaing, PARTIAL_BODY_SIZE_IN_BYTES);
						EM_SAFE_FREE(result_string_of_replcaing);

						if (strstr(temp_data_html, image_data[content_index].content_id)  != NULL)
							continue; /*  Replace content id on HTML with same file name one more time. */
						
						EM_SAFE_FREE(image_data[content_index].content_id);
						EM_SAFE_FREE(image_data[content_index].text_image);
						memset(image_data[content_index].image_file_name, 0x00, 100);
						image_data[content_index].dec_len = 0;
						content_index++;
					}
					
					image_length = 0;
					memset(text_html, 0, PARTIAL_BODY_SIZE_IN_BYTES+1);
		
					if (temp_data_html[0]  != NULL_CHAR)
						memcpy(text_html, temp_data_html, strlen(temp_data_html));
					memset(temp_data_html, 0x00, PARTIAL_BODY_SIZE_IN_BYTES+1);
				}
				
				if (strlen(text_html) > 0){
					memset(buf, 0x00, sizeof(buf));
					char html_body[MAX_CHARSET_VALUE] = {0x00, };
					if (plain_charset_from_content_info != NULL){
						if (strlen(plain_charset_from_content_info) < MAX_CHARSET_VALUE)
			 		 		memcpy(html_body, plain_charset_from_content_info, strlen(plain_charset_from_content_info));
						else
							memcpy(html_body, "UTF-8", strlen("UTF-8"));
					}
					if (html_body[0]  != NULL_CHAR)
						strcat(html_body, HTML_EXTENSION_STRING);
					else
						memcpy(html_body, "UTF-8.htm", strlen("UTF-8.htm"));

					if (!em_storage_create_dir(pbd_event[temp_count].account_id, mail->mail_id, 0, &err))
						EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);

					if (!em_storage_get_save_name(pbd_event[temp_count].account_id, mail->mail_id, 0, html_body, buf, &err)) 
						EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);

					if (!em_storage_create_file(text_html, strlen(text_html), buf, &err))
						EM_DEBUG_EXCEPTION("em_storage_create_file failed [%d]", err);

					mail->file_path_html = EM_SAFE_STRDUP(buf); 
							
				}
	
				mail->body_download_status = (total_mail_size_except_attach < PARTIAL_BODY_SIZE_IN_BYTES) ? 1 : 2;
				EM_DEBUG_LOG("total_mail_size_except_attach [%d], mail->body_download_status [%d]", total_mail_size_except_attach, mail->body_download_status); 

				/* Get preview text */ 
				if ( (err = em_core_get_preview_text_from_file(mail->file_path_plain, mail->file_path_html, MAX_PREVIEW_TEXT_LENGTH, &(mail->preview_text))) != EMF_ERROR_NONE)
					EM_DEBUG_EXCEPTION("em_core_get_preview_text_from_file() failed[%d]", err);
				
				/* Update body contents */
				if (!em_storage_change_mail_field(mail->mail_id, UPDATE_PARTIAL_BODY_DOWNLOAD, mail, true, &err))
					EM_DEBUG_EXCEPTION("em_storage_change_mail_field failed - %d", err);
				
NEXTUIDPARSING: 

				response_buffer = response_buffer + body_size; /*  Set pointer for next mail body.  */
	
				/*  Find end of body */
				if ((response_buffer[0] == CR) && (response_buffer[1] == LF)&& (response_buffer[2] == ')')
					&& (response_buffer[3] == CR) && (response_buffer[4] == LF)){
					response_buffer = response_buffer + 5;
					total_parsed_len_per_uid = body_size+tempmailparselen + 5;
				}
				else if ((response_buffer[0] == ')') && (response_buffer[1] == CR) && (response_buffer[2] == LF)){
					response_buffer = response_buffer + 3;
					total_parsed_len_per_uid = body_size+tempmailparselen + 3;
				}
				else
					EM_DEBUG_EXCEPTION("Mail response end could not found, - %c : %c : %c", response_buffer[0], response_buffer[1], response_buffer[2]);
			
				EM_SAFE_FREE(bufsendforparse);
				
				memset(text_html, 0, PARTIAL_BODY_SIZE_IN_BYTES+1);
				memset(text_plain, 0, PARTIAL_BODY_SIZE_IN_BYTES+1);
				memset(temp_text_buf, 0, PARTIAL_BODY_SIZE_IN_BYTES+1);

				if (mail)
					em_storage_free_mail(&mail, 1, NULL);
				
				/*  Free the activity for mail id in partial body activity table */
				if (false == em_core_delete_pbd_activity(pbd_event[temp_count].account_id, pbd_event[temp_count].mail_id, pbd_event[temp_count].activity_id, &err)){
					EM_DEBUG_EXCEPTION("em_core_delete_pbd_activity failed [%d]", err);
					goto FINISH_OFF;
				}
			}
			
			total_parsed_len  += total_parsed_len_per_uid;
		}
	} 

	EM_DEBUG_LOG("imap_response buflen is [%d]", imap_response->buflen);
	ret = true;
	
FINISH_OFF: 

	if (error)
		*error = err;

	if (true != ret)
		EM_DEBUG_EXCEPTION("Failed download for the uid list %s", command);

	if(reply_from_server) {
		EM_SAFE_FREE(reply_from_server->key);
		EM_SAFE_FREE(reply_from_server->line);
		EM_SAFE_FREE(reply_from_server->tag);
		EM_SAFE_FREE(reply_from_server->text);
		EM_SAFE_FREE(reply_from_server);
	}

	EM_SAFE_FREE(bufsendforparse);
	EM_SAFE_FREE(plain_text_file_name_from_content_info);
	EM_SAFE_FREE(html_text_file_name_from_content_info);
	EM_SAFE_FREE(plain_charset_from_content_info);

	if (cnt_info)
		em_core_mime_free_content_info(cnt_info);

	if (body)
		mail_free_body(&body);

	if (imap_response)
		EM_SAFE_FREE(imap_response->buffer);	
	EM_SAFE_FREE(imap_response);	

	if (mail)
		em_storage_free_mail(&mail, 1, NULL);
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);	
	return ret;
}

EXPORT_API int em_core_bulk_partial_mailbody_download_for_pop3(MAILSTREAM *stream, emf_event_partial_body_thd *pbd_event, int item_count, int *error)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p], pbd_event [%p], item_count [%d], error [%p]", stream, pbd_event, item_count, error);
	int ret = false, err = EMF_ERROR_NONE;
	int i; 

	if (!stream || !pbd_event) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;		
		goto FINISH_OFF;
	}
	
	for (i = 0; i < item_count; i++) {		
 		EM_DEBUG_LOG("pbd_event[%d].account_id [%d], mail_id [%d], server_mail_id [%d], activity_id [%d]", \
			i, pbd_event[i].account_id, pbd_event[i].mail_id, pbd_event[i].server_mail_id, pbd_event[i].activity_id);		

		if (!em_core_mail_download_body_multi_sections_bulk(stream, pbd_event[i].account_id, pbd_event[i].mail_id, false, false, PARTIAL_BODY_SIZE_IN_BYTES, 0 , &err)){
			EM_DEBUG_EXCEPTION("em_core_mail_download_body_multi_sections_bulk failed");
			goto FINISH_OFF;
		}
		
		if (false == em_core_delete_pbd_activity(pbd_event[i].account_id, pbd_event[i].mail_id, pbd_event[i].activity_id, &err)){
			EM_DEBUG_EXCEPTION("em_core_delete_pbd_activity failed [%d]", err);
			goto FINISH_OFF;
		}
	}	

	ret = true;

FINISH_OFF: 
	if (error)
		*error = err;

	EM_DEBUG_FUNC_END("ret [%d] err [%d]", ret, err);	
	return ret;
}



EXPORT_API int em_core_bulk_partial_mailbody_download(MAILSTREAM *stream, emf_event_partial_body_thd *pbd_event, int item_count, int *error)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p], pbd_event [%p], item_count [%d], error [%p]", stream, pbd_event, item_count, error);
	int ret = false, err = EMF_ERROR_NONE;
	emf_account_t *pbd_account = NULL;

	if (!stream || !pbd_event) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;		
		goto FINISH_OFF;
	}

	pbd_account = em_core_get_account_reference(pbd_event[0].account_id);

	if (pbd_account == NULL){
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_ACCOUNT");
		err = EMF_ERROR_INVALID_ACCOUNT;		
		goto FINISH_OFF;
	}

	switch (pbd_account->receiving_server_type){
		case EMF_SERVER_TYPE_IMAP4:
			ret = em_core_bulk_partial_mailbody_download_for_imap(stream, pbd_event, item_count, &err);
			break;
		case EMF_SERVER_TYPE_POP3: 
			ret = em_core_bulk_partial_mailbody_download_for_pop3(stream, pbd_event, item_count, &err);
			break;
		default: 
			err = EMF_ERROR_NOT_SUPPORTED;
			ret = false;
			break;
	}	

	ret = true;
FINISH_OFF: 
	if (error)
		*error = err;

	EM_DEBUG_FUNC_END("ret [%d] err [%d]", ret, err);	
	return ret;
}


static emf_partial_buffer *em_core_get_response_from_server (NETSTREAM *nstream, char *tag, IMAPPARSEDREPLY **reply)
{
	EM_DEBUG_FUNC_BEGIN();

	if (!nstream || !tag || !reply){
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return NIL;
	}

	emf_partial_buffer *retPartialBuffer = NULL;
	IMAPPARSEDREPLY *ret_reply = NULL;
	char *pline = NULL;
	int linelen = 0;
	int bytes_copied = 0;
	int allocated_buffer_size = (BULK_PARTIAL_BODY_DOWNLOAD_COUNT + 1) * PARTIAL_BODY_SIZE_IN_BYTES ;

	retPartialBuffer = (emf_partial_buffer *)em_core_malloc(sizeof(emf_partial_buffer));

	if (NULL == retPartialBuffer){
		EM_DEBUG_EXCEPTION("em_core_malloc failed");
		return NIL;
	}

	retPartialBuffer->buffer = (char *)em_core_malloc(allocated_buffer_size);

	if (NULL == retPartialBuffer->buffer){
		EM_DEBUG_EXCEPTION("em_core_malloc failed");
		EM_SAFE_FREE(retPartialBuffer);
		return NIL;
	}

	while (nstream){	
		if (!(pline = net_getline(nstream))) {
			EM_DEBUG_EXCEPTION("net_getline failed...");
			EM_SAFE_FREE(retPartialBuffer->buffer);
			EM_SAFE_FREE(retPartialBuffer);

			return NIL;
		}

		linelen = strlen(pline);
		memcpy(retPartialBuffer->buffer + bytes_copied, pline, linelen);
		bytes_copied  += linelen;
		memcpy(retPartialBuffer->buffer + bytes_copied, CRLF_STRING, 2);
		bytes_copied += 2;				

		if (0 == strncmp(pline, tag, strlen(tag))) {

			ret_reply = em_core_malloc(sizeof(IMAPPARSEDREPLY));

			if (!ret_reply){
				EM_DEBUG_EXCEPTION("em_core_malloc failed");
				return NIL;
			}

			if(reply)
				*reply = ret_reply;
	
			if (0 == strncmp(pline + strlen(tag) + 1, "OK", 2)) {
				ret_reply->line = (unsigned char*)EM_SAFE_STRDUP(tag);
				ret_reply->tag  = (unsigned char*)EM_SAFE_STRDUP(tag);
				ret_reply->key  = (unsigned char*)EM_SAFE_STRDUP("OK");
				ret_reply->text = (unsigned char*)EM_SAFE_STRDUP("Success");
				EM_SAFE_FREE(pline);	
				break;
			}
			else {
				EM_DEBUG_EXCEPTION("Tagged Response not OK. IMAP4 Response -> [%s]", pline);	
				ret_reply->line = (unsigned char*)EM_SAFE_STRDUP(tag);
				ret_reply->tag  = (unsigned char*)EM_SAFE_STRDUP(tag);
				ret_reply->key  = (unsigned char*)EM_SAFE_STRDUP("NO");
				ret_reply->text = (unsigned char*)EM_SAFE_STRDUP("Fail");

				EM_SAFE_FREE(pline);
				EM_SAFE_FREE(retPartialBuffer->buffer);
				EM_SAFE_FREE(retPartialBuffer);
				
				return NIL;
			}	

			EM_DEBUG_LOG("ret_reply->line [%s]", ret_reply->line);
			EM_DEBUG_LOG("ret_reply->tag [%s]",  ret_reply->tag);
			EM_DEBUG_LOG("ret_reply->key [%s]",  ret_reply->key);
			EM_DEBUG_LOG("ret_reply->text [%s]", ret_reply->text);
	
		}	
		EM_SAFE_FREE(pline);		
	}
	
	retPartialBuffer->buflen = strlen(retPartialBuffer->buffer);
	
	EM_DEBUG_FUNC_END("retPartialBuffer [%p]", retPartialBuffer);	
	return retPartialBuffer;
}
		
#endif /*  __FEATURE_PARTIAL_BODY_DOWNLOAD__ */
/* EOF */
