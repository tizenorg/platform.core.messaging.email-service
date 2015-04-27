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
 * File :  email-core-mailbox-sync.c
 * Desc :  Mail Header Sync
 *
 * Auth :
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <vconf.h>
#include <unicode/ucsdet.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "email-internal-types.h"

#ifdef __FEATURE_SUPPORT_SYNC_STATE_ON_NOTI_BAR__
#include <vconf/vconf-internal-email-keys.h>
#endif /* __FEATURE_SUPPORT_SYNC_STATE_ON_NOTI_BAR__ */

#include "c-client.h"
#include "lnx_inc.h"

#include "email-utilities.h"
#include "email-convert.h"
#include "email-core-mailbox-sync.h"
#include "email-core-imap-mailbox.h"
#include "email-core-event.h"
#include "email-core-mailbox.h"
#include "email-core-mail.h"
#include "email-core-mime.h"
#include "email-core-utils.h"
#include "email-core-smtp.h"
#include "email-core-account.h"
#include "email-storage.h"
#include "email-core-signal.h"
#include "email-core-gmime.h"
#include "flstring.h"
#include "email-debug-log.h"

#define MAX_CHARSET_VALUE 256

static int g_current_sync_account_id = 0;
static char g_append_uid_rsp[129]; /* added for getting server response  */

extern void imap_parse_body_structure (MAILSTREAM *stream, BODY *body, unsigned char **txtptr, IMAPPARSEDREPLY *reply);

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
static void emcore_free_email_partial_buffer(email_partial_buffer **partial_buffer, int item_count);
static email_partial_buffer *emcore_get_response_from_server (NETSTREAM *nstream, char *tag, IMAPPARSEDREPLY **reply, int input_download_size, int item_count);
static int emcore_initiate_pbd(char *multi_user_name, MAILSTREAM *stream, int account_id, int mail_id, char *uid, int mailbox_id, int *err_code);
#endif

#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
static char g_append_uid_rsp[129]; /* added for getting server response  */
#endif

static struct {
	const char *charset;
	const char *tm_zone;
} known_zone_charset[] = {
	{"euc-kr",      "kst"},
        {"euc-jp",      "jst"}
#if 0
        {"Big5",        "zh" },
        {"BIG5HKSCS",   "zh" },
        {"gb2312",      "zh" },
        {"gb18030",     "zh" },
        {"gbk",         "zh" },
        {"euc-tw",      "zh" },
        {"iso-2022-jp", "ja" },
        {"Shift-JIS",   "ja" },
        {"sjis",        "ja" },
        {"ujis",        "ja" },
        {"eucJP",       "ja" },
        {"euc-kr",      "ko" },
        {"koi8-r",      "ru" },
        {"koi8-u",      "uk" }
#endif
};

int pop3_mail_calc_rfc822_size(MAILSTREAM *stream, int msgno, int *size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	POP3LOCAL *pop3local = NULL;
	char command[16];
	char *response = NULL;

	if (!stream || !size) {
		EM_DEBUG_EXCEPTION(" stream[%p], msgno[%d], size[%p]\n", stream, msgno, size);

		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(pop3local = stream->local) || !pop3local->netstream) {
		err = EMAIL_ERROR_INVALID_STREAM;
		goto FINISH_OFF;
	}

	memset(command, 0x00, sizeof(command));

	SNPRINTF(command, sizeof(command), "LIST %d\015\012", msgno);

	/* EM_DEBUG_LOG(" [POP3] >>> %s", command); */

	/*  send command  :  get rfc822 size by msgno */
	if (!net_sout(pop3local->netstream, command, (int)EM_SAFE_STRLEN(command))) {
		EM_DEBUG_EXCEPTION(" net_sout failed...");

		err = EMAIL_ERROR_INVALID_RESPONSE;
		goto FINISH_OFF;
	}

	/*  receive response */
	if (!(response = net_getline(pop3local->netstream))) {
		err = EMAIL_ERROR_CONNECTION_BROKEN;		/* EMAIL_ERROR_UNKNOWN; */
		goto FINISH_OFF;
	}

	/* EM_DEBUG_LOG(" [POP3] <<< %s", response); */

	if (*response == '+') {		/*  "+ OK" */
		char *p = NULL;

		if (!(p = strchr(response + strlen("+OK "), ' '))) {
			err = EMAIL_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}

		*size = atoi(p + 1);
	}
	else if (*response == '-') {		/*  "- ERR" */
		err = EMAIL_ERROR_POP3_LIST_FAILURE;
		goto FINISH_OFF;
	}
	else {
		err = EMAIL_ERROR_INVALID_RESPONSE;
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
	int err = EMAIL_ERROR_NONE;

	IMAPLOCAL *imaplocal = NULL;
	char tag[32], command[128];
	char *response = NULL;

	if (!stream || !size) {
		EM_DEBUG_EXCEPTION("stream[%p], msgno[%d], size[%p]", stream, msgno, size);

		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(imaplocal = stream->local) || !imaplocal->netstream) {
		err = EMAIL_ERROR_INVALID_STREAM;
		goto FINISH_OFF;
	}

	memset(tag, 0x00, sizeof(tag));
	memset(command, 0x00, sizeof(command));

	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
	SNPRINTF(command, sizeof(command), "%s FETCH %d RFC822.SIZE\015\012", tag, msgno);

	/* EM_DEBUG_LOG(" [IMAP4] >>> %s", command); */

	/*  send command  :  get rfc822 size by msgno */
	if (!net_sout(imaplocal->netstream, command, (int)EM_SAFE_STRLEN(command))) {
		EM_DEBUG_EXCEPTION(" net_sout failed...");

		err = EMAIL_ERROR_INVALID_RESPONSE;
		goto FINISH_OFF;
	}

	while (imaplocal->netstream) {
		char *s = NULL;
		char *t = NULL;

		/*  receive response */
		if (!(response = net_getline(imaplocal->netstream)))
			break;

		/* EM_DEBUG_LOG(" [IMAP4] <<< %s", response); */

		if (!strncmp(response, tag, EM_SAFE_STRLEN(tag))) {
			if (!strncmp(response + EM_SAFE_STRLEN(tag) + 1, "OK", 2)) {
				EM_SAFE_FREE(response);
				break;
			}
			else {		/*  'NO' or 'BAD' */
				err = EMAIL_ERROR_IMAP4_FETCH_SIZE_FAILURE;		/* EMAIL_ERROR_INVALID_RESPONSE; */
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
					err = EMAIL_ERROR_INVALID_RESPONSE;
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

int pop3_mailbox_get_uids(MAILSTREAM *stream, emcore_uid_list** uid_list, int *err_code)
{
	EM_PROFILE_BEGIN(pop3MailboxGetuid);
	EM_DEBUG_FUNC_BEGIN("stream[%p], uid_list[%p], err_code[%p]", stream, uid_list, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	POP3LOCAL *pop3local = NULL;
	char command[64];
	char *response = NULL;
	emcore_uid_list *uid_elem = NULL;

	if (!stream || !uid_list) {
		EM_DEBUG_EXCEPTION("stream[%p], uid_list[%p]n", stream, uid_list);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(pop3local = stream->local) || !pop3local->netstream) {
		EM_DEBUG_EXCEPTION("invalid POP3 stream detected...");
		err = EMAIL_ERROR_INVALID_STREAM;
		goto FINISH_OFF;
	}

	memset(command, 0x00, sizeof(command));

	SNPRINTF(command, sizeof(command), "UIDL\015\012");

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG(" [POP3] >>> [%s]", command);
#endif

	/*  send command  :  get msgno/uid for all message */
	if (!net_sout(pop3local->netstream, command, (int)EM_SAFE_STRLEN(command))) {
		EM_DEBUG_EXCEPTION("net_sout failed...");
		err = EMAIL_ERROR_CONNECTION_BROKEN;		/* EMAIL_ERROR_UNKNOWN; */
		goto FINISH_OFF;
	}

	*uid_list = NULL;

	while (pop3local->netstream) {
		char *p = NULL;

		/*  receive response */
		if (!(response = net_getline(pop3local->netstream))) {
			EM_DEBUG_EXCEPTION("net_getline failed...");
			err = EMAIL_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}

#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG(" [POP3] <<< [%s]", response);
#endif

		if (*response == '-') {		/*  "-ERR" */
			err = EMAIL_ERROR_POP3_UIDL_FAILURE;		/* EMAIL_ERROR_INVALID_RESPONSE; */
			goto FINISH_OFF;
		}

		if (*response == '+') {		/*  "+OK" */
			EM_SAFE_FREE (response);
			continue;
		}

		if (*response == '.') {
			EM_SAFE_FREE (response);
			break;
		}

		if ((p = strchr(response, ' '))) {
			*p = '\0';

			if (!(uid_elem = em_malloc(sizeof(emcore_uid_list)))) {
				EM_DEBUG_EXCEPTION("malloc failed...");
				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			uid_elem->msgno = atoi(response);
			uid_elem->uid = EM_SAFE_STRDUP(p + 1);

			if (*uid_list  != NULL)
				uid_elem->next = *uid_list;		/*  prepend new data to table */

			*uid_list = uid_elem;
		}
		else {
			err = EMAIL_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}

		EM_SAFE_FREE (response);
	}

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE(response);

	if (err_code  != NULL)
		*err_code = err;

	EM_PROFILE_END(pop3MailboxGetuid);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

int imap4_mailbox_get_uids(MAILSTREAM *stream, char *input_target_uid_string, emcore_uid_list** uid_list, int *err_code)
{
	EM_PROFILE_BEGIN(ImapMailboxGetUids);
	EM_DEBUG_FUNC_BEGIN("stream[%p] input_target_uid_string[%p] uid_list[%p] err_code[%p]", stream, input_target_uid_string, uid_list, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int command_length = 0;
	IMAPLOCAL *imaplocal = NULL;
	char tag[16];
	char *command= NULL;
	char *response = NULL;
	emcore_uid_list *uid_elem = NULL;

	if (!stream || !uid_list) {
		EM_DEBUG_EXCEPTION("stream[%p], uid_list[%p]", stream, uid_list);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(imaplocal = stream->local) || !imaplocal->netstream) {
		EM_DEBUG_EXCEPTION("invalid IMAP4 stream detected...");
		err = EMAIL_ERROR_INVALID_PARAM;		/* EMAIL_ERROR_UNKNOWN */
		goto FINISH_OFF;
	}

	if (stream->nmsgs == 0){
		err = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER;
		goto FINISH_OFF;
	}

	command_length = sizeof(char) * (EM_SAFE_STRLEN(input_target_uid_string) + sizeof(tag) + 100);
	command = em_malloc(command_length);
	if (command == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(tag, 0x00, sizeof(tag));

	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
	if (input_target_uid_string)
		SNPRINTF(command, command_length, "%s UID FETCH %s (FLAGS UID INTERNALDATE)\015\012", tag, input_target_uid_string);
	else
		SNPRINTF(command, command_length, "%s FETCH 1:* (FLAGS UID INTERNALDATE)\015\012", tag);
	EM_DEBUG_LOG("COMMAND [%s] \n", command);
#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG(" [IMAP4] >>> [%s]", command);
#endif

	/*  send command  :  get msgno/uid for all message */
	if (!net_sout(imaplocal->netstream, command, (int)EM_SAFE_STRLEN(command))) {
		EM_DEBUG_EXCEPTION(" net_sout failed...\n");
		err = EMAIL_ERROR_CONNECTION_BROKEN;
		goto FINISH_OFF;
	}

	*uid_list = NULL;

	while (imaplocal->netstream) {
		char *p = NULL;
		char *p_date = NULL;
		char *s = NULL;
		int seen = 0;
		int forwarded = 0;
		int draft = 0;
		int flagged = 0;
		/*  receive response */
		if (!(response = net_getline(imaplocal->netstream))) {
			EM_DEBUG_EXCEPTION("net_getline failed...");
			err = EMAIL_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}

#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG(" [IMAP4] <<< [%s]", response);
#endif

		if (!strncmp(response, tag, EM_SAFE_STRLEN(tag))) {
			if (!strncmp(response + EM_SAFE_STRLEN(tag) + 1, "OK", 2)) {
				EM_SAFE_FREE (response);
				break;
			}
			else {		/*  'NO' or 'BAD' */
				err = EMAIL_ERROR_IMAP4_FETCH_UID_FAILURE;		/* EMAIL_ERROR_INVALID_RESPONSE; */
				goto FINISH_OFF;
			}
		}

		if ((p = strstr(response, " FETCH ("))) {
			if (!strstr(p, "\\Deleted")) {	/*  undeleted only */
				*p = '\0'; p  += strlen(" FETCH ");

				seen = strstr(p, "\\Seen") ? 1  :  0;
				draft = strstr(p, "\\Draft") ? 1  :  0;
				flagged = strstr(p, "\\Flagged") ? 1  :  0;
				forwarded = strstr(p, "$Forwarded") ? 1  :  0;

				if ((p = strstr(p, "UID "))) {

					/* get INTERNALDATE */
					char internaldate[100] = {0,};
					if ((p_date = strstr(p, "INTERNALDATE \""))) {
						int i = 0;
						p_date += strlen("INTERNALDATE \"");
						while (*p_date != '"' && i < 99) {
							internaldate[i] = *p_date;
							i++;
							p_date++;
						}
					}

					s = p + strlen("UID ");

					while (isdigit(*s))
						s++;

					*s = '\0';

					if (!(uid_elem = em_malloc(sizeof(emcore_uid_list)))) {
						EM_DEBUG_EXCEPTION("em_malloc failed...");
						err = EMAIL_ERROR_OUT_OF_MEMORY;
						goto FINISH_OFF;
					}

					uid_elem->msgno = atoi(response + strlen("* "));
					uid_elem->uid = EM_SAFE_STRDUP(p + strlen("UID "));
					uid_elem->flag.seen = seen;
					uid_elem->flag.draft = draft;
					uid_elem->flag.forwarded = forwarded;
					uid_elem->flag.flagged = flagged;
					uid_elem->internaldate = EM_SAFE_STRDUP(internaldate);
					if (*uid_list  != NULL)
						uid_elem->next = *uid_list;		/*  prepend new data to list */

					*uid_list = uid_elem;
				}
				else {
					err = EMAIL_ERROR_INVALID_RESPONSE;
					goto FINISH_OFF;
				}
			}
		}
		else {
			err = EMAIL_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}

		EM_SAFE_FREE(response);
	}

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE(response);
	EM_SAFE_FREE(command);

	if (err_code  != NULL)
		*err_code = err;

	EM_PROFILE_END(ImapMailboxGetUids);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

int emcore_get_uids_order_by_datetime_from_imap_server(MAILSTREAM *stream, int count_to_download, emcore_uid_list** output_uid_list)
{
	EM_PROFILE_BEGIN(emcore_get_uids_order_by_datetime_from_imap_server);
	EM_DEBUG_FUNC_BEGIN("stream[%p] count_to_download [%d] output_uid_list[%p]", stream, count_to_download, output_uid_list);

	int err = EMAIL_ERROR_NONE;

	IMAPLOCAL *imaplocal = NULL;
	char tag[16], command[64];
	char *response = NULL;
	emcore_uid_list *uid_elem = NULL;
	char delims[] = " ";
	char *result = NULL;
	int  uid_count = 0;
	time_t         RawTime = 0;
	char           since_date_string[20];
	char           before_date_string[20];
	char          *uid_range_string = NULL;
	emcore_uid_list *uid_list_for_listing = NULL;

	if (!stream || !output_uid_list) {
		EM_DEBUG_EXCEPTION(" stream[%p], output_uid_list[%p]", stream, output_uid_list);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(imaplocal = stream->local) || !imaplocal->netstream) {
		EM_DEBUG_EXCEPTION(" invalid IMAP4 stream detected...");
		err = EMAIL_ERROR_INVALID_PARAM;		/* EMAIL_ERROR_UNKNOWN */
		goto FINISH_OFF;
	}

	/* Fetch the System time and Retrieve the a Week before time */
	time(&RawTime);

	while (uid_count <= count_to_download) {
		RawTime = RawTime - (604800 * 4); //4 Weeks Before

		emcore_make_date_string_for_search(RawTime, since_date_string);

		memset(tag, 0x00, sizeof(tag));
		memset(command, 0x00, sizeof(command));

		SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
		if (EM_SAFE_STRLEN(before_date_string) > 0)
			SNPRINTF(command, sizeof(command), "%s UID SEARCH 1:* SINCE %s BEFORE %s\015\012", tag, since_date_string, before_date_string);
		else
			SNPRINTF(command, sizeof(command), "%s UID SEARCH 1:* SINCE %s\015\012", tag, since_date_string);
		EM_DEBUG_LOG("COMMAND [%s] ", command);

#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG(" [IMAP4] >>> [%s]", command);
#endif

		/*  send command  :  get msgno/uid for all message */
		if (!net_sout(imaplocal->netstream, command, (int)EM_SAFE_STRLEN(command))) {
			EM_DEBUG_EXCEPTION(" net_sout failed...");
			err = EMAIL_ERROR_CONNECTION_BROKEN;
			goto FINISH_OFF;
		}

		while (imaplocal->netstream) {
			char *p = NULL;
			/*  receive response */
			if (!(response = net_getline(imaplocal->netstream))) {
				EM_DEBUG_EXCEPTION(" net_getline failed...");
				err = EMAIL_ERROR_INVALID_RESPONSE;
				goto FINISH_OFF;
			}
#ifdef FEATURE_CORE_DEBUG
			EM_DEBUG_LOG(" [IMAP4] <<< [%s]", response);
#endif

			if (!strncmp(response, tag, EM_SAFE_STRLEN(tag))) {
				if (!strncmp(response + EM_SAFE_STRLEN(tag) + 1, "OK", 2)) {
					EM_SAFE_FREE (response);
					break;
				}
				else {	/*  'NO' or 'BAD' */
					err = EMAIL_ERROR_IMAP4_FETCH_UID_FAILURE;		/* EMAIL_ERROR_INVALID_RESPONSE; */
					goto FINISH_OFF;
				}
			}

			if ((p = strstr(response, " SEARCH "))) {
				*p = '\0'; p  += strlen(" SEARCH ");

				result = strtok(p, delims);

				while (result  != NULL)
				{
					EM_DEBUG_LOG("UID VALUE DEEP is [%s]", result);

					if (!(uid_elem = em_malloc(sizeof(emcore_uid_list)))) {
						EM_DEBUG_EXCEPTION(" malloc failed...");
						err = EMAIL_ERROR_OUT_OF_MEMORY;
						goto FINISH_OFF;
					}

					uid_elem->uid = EM_SAFE_STRDUP(result);

					if (uid_list_for_listing != NULL)
						uid_elem->next = uid_list_for_listing;
					uid_list_for_listing = uid_elem;
					result = strtok(NULL, delims);
					uid_count++;
				}

				EM_SAFE_FREE(response);
				continue;
			}
			else if ((p = strstr(response, " SEARCH"))){
				/* there is no mail */
				continue;
			}
			else {
				err = EMAIL_ERROR_INVALID_RESPONSE;
				goto FINISH_OFF;
			}
		}

		EM_SAFE_STRCPY(before_date_string, since_date_string);
		EM_DEBUG_LOG("uid_count [%d] ", uid_count);
	}

	EM_DEBUG_LOG("uid_count [%d] ", uid_count);

	if ((err = emcore_make_uid_range_string(uid_list_for_listing, uid_count, &uid_range_string)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_make_uid_range_string failed [%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("uid_range_string [%s] ", uid_range_string);

	/* Get uids with flags */
	if(!imap4_mailbox_get_uids(stream, uid_range_string, output_uid_list, &err)) {
		EM_DEBUG_EXCEPTION("imap4_mailbox_get_uids failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (uid_list_for_listing)
		emcore_free_uids(uid_list_for_listing, NULL);

	EM_SAFE_FREE (response);
        EM_SAFE_FREE (uid_range_string);

	EM_PROFILE_END(emcore_get_uids_order_by_datetime_from_imap_server);
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


int imap4_mailbox_get_uids_by_timestamp(MAILSTREAM *stream, emcore_uid_list** uid_list,  int *err_code)
{
	EM_PROFILE_BEGIN(emCoreMailboxuidsbystamp);
	EM_DEBUG_FUNC_BEGIN("stream[%p], uid_list[%p],  err_code[%p]", stream, uid_list, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	IMAPLOCAL *imaplocal = NULL;
	char tag[16], command[64];
	char *response = NULL;
	emcore_uid_list *uid_elem = NULL;
	char delims[] = " ";
	char *result = NULL;

	struct tm   *timeinfo = NULL;
	time_t         RawTime = 0;
	time_t         week_before_RawTime = 0;
	char  date_string[16];
	char *mon = NULL;

	if (!stream || !uid_list) {
		EM_DEBUG_EXCEPTION(" stream[%p], uid_list[%p]", stream, uid_list);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(imaplocal = stream->local) || !imaplocal->netstream) {
		EM_DEBUG_EXCEPTION(" invalid IMAP4 stream detected...");
		err = EMAIL_ERROR_INVALID_PARAM;		/* EMAIL_ERROR_UNKNOWN */
		goto FINISH_OFF;
	}

	/* Fetch the System time and Retrieve the a Week before time */
	/* 	tzset(); */
	time(&RawTime);

	EM_DEBUG_LOG("RawTime Info [%lu]", RawTime);

	timeinfo = localtime (&RawTime);
	if (timeinfo == NULL) {
		EM_DEBUG_EXCEPTION("localtime failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

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
	SNPRINTF(command, sizeof(command), "%s UID SEARCH 1:* SINCE %s\015\012", tag, date_string);
	EM_DEBUG_LOG("COMMAND [%s] ", command);

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG(" [IMAP4] >>> [%s]", command);
#endif

	/*  send command  :  get msgno/uid for all message */
	if (!net_sout(imaplocal->netstream, command, (int)EM_SAFE_STRLEN(command))) {
		EM_DEBUG_EXCEPTION(" net_sout failed...");
		err = EMAIL_ERROR_CONNECTION_BROKEN;		/* EMAIL_ERROR_UNKNOWN */
		goto FINISH_OFF;
	}

	*uid_list = NULL;

	while (imaplocal->netstream) {
		char *p = NULL;
		/*  receive response */
		if (!(response = net_getline(imaplocal->netstream))) {
			EM_DEBUG_EXCEPTION(" net_getline failed...");
			err = EMAIL_ERROR_INVALID_RESPONSE;		/* EMAIL_ERROR_UNKNOWN; */
			goto FINISH_OFF;
		}
#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG(" [IMAP4] <<< [%s]", response);
#endif

		if (!strncmp(response, tag, EM_SAFE_STRLEN(tag))) {
			if (!strncmp(response + EM_SAFE_STRLEN(tag) + 1, "OK", 2)) {
				EM_SAFE_FREE (response);
				break;
			}
			else {	/*  'NO' or 'BAD' */
				err = EMAIL_ERROR_IMAP4_FETCH_UID_FAILURE;		/* EMAIL_ERROR_INVALID_RESPONSE; */
				goto FINISH_OFF;
			}
		}

		if ((p = strstr(response, " SEARCH "))){
		    *p = '\0'; p  += strlen(" SEARCH ");

		    result = strtok(p, delims);

		    while (result  != NULL)
		    {
				EM_DEBUG_LOG("UID VALUE DEEP is [%s]", result);

				if (!(uid_elem = em_malloc(sizeof(emcore_uid_list)))) {
					EM_DEBUG_EXCEPTION(" malloc failed...");
					err = EMAIL_ERROR_OUT_OF_MEMORY;
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
			err = EMAIL_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}
		/* Delete the dead code */
	}

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE (response);

	if (err_code  != NULL)
		*err_code = err;
	EM_PROFILE_END(emCoreMailboxuidsbystamp);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

#define PARSE_BUFFER_LENGTH 4096
static int emcore_parse_header(char *rfc822_header, char **subject, char **from, int *req_read_receipt, int *priority, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("rfc822_header[%p], req_read_receipt[%p], priority[%p], err_code[%p]", rfc822_header, req_read_receipt, priority,  err_code);

	if (!rfc822_header || !priority)
		return false;

	if (err_code)
		*err_code = EMAIL_ERROR_NONE;

	EM_PROFILE_BEGIN(emCoreMailboxParseHeader);

	char buf[PARSE_BUFFER_LENGTH];
	int len, i, j;
	int subject_flag = 0;



	*priority = 3;

	memset(buf, 0x00, PARSE_BUFFER_LENGTH);

	for (len = EM_SAFE_STRLEN(rfc822_header), i = 0, j = 0; i < len; i++) {
		if (rfc822_header[i] == CR && rfc822_header[i+1] == LF){
			if (j + 3 < PARSE_BUFFER_LENGTH) /* '3' include CR LF NULL */
				strncpy(buf + j, CRLF_STRING, PARSE_BUFFER_LENGTH - (j + 2)); /* '3' include CR LF */
			else
				EM_DEBUG_EXCEPTION("buf is too small.");

			i++;
			j = 0;

			/*  parsing data */
			EM_DEBUG_LOG_DEV("buf:%s", buf);

			/*  disposition_notification_to */
			if ((buf[0] == 'D' || buf[0] == 'd') && buf[11] == '-' && (buf[12] == 'N' || buf[12] == 'n') && buf[24] == '-' && (buf[25] == 'T' || buf[25] == 't')) {
				em_upper_string(buf);
				if (req_read_receipt)
					*req_read_receipt = 1;
				memset(buf, 0x00, PARSE_BUFFER_LENGTH);
				continue;
			}

			/*  x-priority */
			if ((buf[0] == 'X' || buf[0] == 'x') && (buf[2] == 'P' || buf[2] == 'p') && (buf[9] == 'Y' || buf[9] == 'y')) {
				em_upper_string(buf);
				size_t len_2 = EM_SAFE_STRLEN(buf);
				if (len_2 >= 12){
					buf[len_2 - 2] = '\0';
					*priority = atoi(buf + 11);
					memset(buf, 0x00, PARSE_BUFFER_LENGTH);
					continue;
				}
			}

			/*  x-msmail-priority */
			if ((buf[0] == 'X' || buf[0] == 'x') && (buf[2] == 'M' || buf[2] == 'm') && (buf[9] == 'P' || buf[9] == 'p') && (buf[16] == 'Y' || buf[16] == 'y')) {
				em_upper_string(buf);
				if (strcasestr(buf, "HIGH"))
					*priority = 1;
				if (strcasestr(buf, "NORMAL"))
					*priority = 3;
				if (strcasestr(buf, "LOW"))
					*priority = 5;
				memset(buf, 0x00, PARSE_BUFFER_LENGTH);
				continue;
			}

			/* Importance */
			if ((buf[0] == 'I' || buf[0] == 'i') && (buf[2] == 'P' || buf[2] == 'p') && (buf[6] == 'A' || buf[6] == 'a')) {
				em_upper_string(buf);
				if (strcasestr(buf, "HIGH"))
					*priority = 1;
				if (strcasestr(buf, "NORMAL"))
					*priority = 3;
				if (strcasestr(buf, "LOW"))
					*priority = 5;
			}

			/* subject */
			if (subject) {
				if (buf[0] != ' ' && buf[0] != '\t') {
					subject_flag = 0;
				}

				if ((buf[0] == 'S' || buf[0] == 's') && (buf[1] == 'u' || buf[1] == 'U') && (buf[2] == 'b' || buf[2] == 'B')) {
					char *deli = NULL;
					char *r = NULL;

					deli = strstr(buf, ":");
					r = strstr(buf, "\r\n");
					if (r) *r = '\0';

					if (deli) {
						if (*subject) EM_SAFE_FREE(*subject);
						*subject = g_strdup(deli+1);
						EM_DEBUG_LOG_DEV("subject:%s", *subject);
					}

					subject_flag = 1;
				}

				if (subject_flag && (buf[0] == ' ' || buf[0] == '\t')) {
					char *subject_add = NULL;
					char *tmp = NULL;
					char *r = NULL;

					r = strstr(buf, "\r\n");
					if (r) *r = '\0';

					subject_add = g_strdup(buf+1);

					tmp = *subject;
					*subject = g_strconcat(*subject, "\t", subject_add, NULL);
					EM_SAFE_FREE(tmp);
					EM_SAFE_FREE(subject_add);

					EM_DEBUG_LOG_DEV("subject:%s", *subject);
				}
			}

			/* From */
			if (from) {
				if ((buf[0] == 'F' || buf[0] == 'f') && (buf[1] == 'r' || buf[1] == 'R') && (buf[2] == 'o' || buf[2] == 'O')) {
					char *deli = NULL;
					char *r = NULL;

					deli = strstr(buf, ":");
					r = g_strrstr(buf, "<");
					if (r) *r = '\0';

					if (deli) {
						if (*from) EM_SAFE_FREE(*from);
						*from = g_strdup(deli+1);
						EM_DEBUG_LOG_DEV("from:%s", *from);
					}
				}
			}

			memset(buf, 0x00, PARSE_BUFFER_LENGTH);
			continue;
		}

		if (j + 1 < PARSE_BUFFER_LENGTH)
			buf[j++] = rfc822_header[i];
		else
			EM_DEBUG_EXCEPTION("buf is too small.");
	}

	EM_PROFILE_END(emCoreMailboxParseHeader);

	if (err_code)
		*err_code = EMAIL_ERROR_NONE;

	return true;
}


static int emcore_get_mail_extra_info(MAILSTREAM *stream, int msgno, char **subject, char **from, int *req_read_receipt, int *priority, int *err_code)
{
	EM_PROFILE_BEGIN(emCoreMailGetExtraInfo);
	EM_DEBUG_FUNC_BEGIN("stream[%p], msgno[%d], req_read_receipt[%p], priority[%p], err_code[%p]", stream, msgno, req_read_receipt, priority, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	char *rfc822_header = NULL;
	unsigned long len = 0;

	EM_PROFILE_BEGIN(MaiFetchHeader);
#ifdef __FEATURE_HEADER_OPTIMIZATION__
	/* Check if header already available in cache */
	if (stream && stream->cache && stream->cache[msgno-1]->private.msg.header.text.data){

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
		err = EMAIL_ERROR_IMAP4_FETCH_UID_FAILURE;		/* EMAIL_ERROR_UNKNOWN; */
		goto FINISH_OFF;
	}

	if (!emcore_parse_header(rfc822_header, subject, from, req_read_receipt, priority, &err)) {
		EM_DEBUG_EXCEPTION("emcore_parse_header falied - %d", err);
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

#define EMAIL_SYNC_STATUS_NOT_EXIST_ON_SERVER  0
#define EMAIL_SYNC_STATUS_EXIST_ON_SERVER      1
#define EMAIL_SYNC_STATUS_SEEN_FLAG_CHANGED    2
#define EMAIL_SYNC_STATUS_FLAG_CHANGED         3

static GDateTime *emcore_convert_strftime(char *time_str)
{
	GDateTime *datetime = NULL;
	gchar **token_list = NULL;
	int i = 0;
	int index = 0;
	int day = 0;
	int month = 0;
	int year = 0;
	int hh = 0;
	int mm = 0;
	int ss = 0;
	char *month_str = NULL;
	char *ar_month[13] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL};

	token_list = g_strsplit_set(time_str, "-: ", -1);

	if(token_list == NULL) {
		EM_DEBUG_LOG("g_strsplit_set failed.");
		return NULL;
	}

	while (token_list[index] != NULL) {
		index++;
	}

	for (i = 0; i < index; i++) {
		switch (i) {
		case 0:		//Day
			day = atoi(token_list[i]);
			break;
		case 1:		//Month
			month_str = g_strdup(token_list[i]);
			break;
		case 2:		//Year
			year = atoi(token_list[i]);
			break;
		case 3:		//HH
			hh = atoi(token_list[i]);
			break;
		case 4:		//MM
			mm = atoi(token_list[i]);
			break;
		case 5:		//SS
			ss = atoi(token_list[i]);
			break;
		}
	}

	if (month_str) {
		i = 0;
		while (ar_month[i]) {
			if (g_ascii_strcasecmp(month_str, ar_month[i]) == 0) {
				month = i+1;
				break;
			}
			i++;
		}
	}

	EM_DEBUG_LOG_DEV("%d/%d/%d %d:%d:%d", day, month, year, hh, mm, ss);

	datetime = g_date_time_new_utc(year, month, day, hh, mm, ss);

	g_strfreev(token_list);
	g_free(month_str);

	return datetime;
}

static gint emcore_compare_uid_elem(gconstpointer a, gconstpointer b)
{
	emcore_uid_list *item_a = NULL;
	emcore_uid_list *item_b = NULL;
	GDateTime *datetime_a = NULL;
	GDateTime *datetime_b = NULL;
	int ret = 0;

	item_a = (emcore_uid_list *)a;
	item_b = (emcore_uid_list *)b;

	datetime_a = emcore_convert_strftime(item_a->internaldate);
	datetime_b = emcore_convert_strftime(item_b->internaldate);

	ret = g_date_time_compare(datetime_b, datetime_a);

	return ret;
}

static int emcore_get_uids_to_download(char *multi_user_name, MAILSTREAM *stream, email_account_t *account, emstorage_mailbox_tbl_t *input_mailbox_tbl, int limit_count, emcore_uid_list** uid_list, int *uids, int retrieve_mode ,  int *err_code)
{
	EM_PROFILE_BEGIN(emCoreGetUidsDownload);
	EM_DEBUG_FUNC_BEGIN("account[%p], input_mailbox_tbl[%p], limit_count[%d], uid_list[%p], err_code[%p]", account, input_mailbox_tbl, limit_count, uid_list, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	emstorage_read_mail_uid_tbl_t *downloaded_uids = NULL;
	int i = 0, downloaded_uid_count = 0, uid_count = 0, uid_to_be_downloaded_count = 0;
	emcore_uid_list *uid_elem = NULL;
	emcore_uid_list *head_uid_elem = NULL, *end =  NULL;
	emcore_uid_list *next_uid_elem = NULL;
	emstorage_mail_tbl_t *mail = NULL;

	emcore_uid_list *elem_item = NULL;
	GList *item_list = NULL;
	GList *item_iter = NULL;
	GList *item_next = NULL;

	if (!account || !input_mailbox_tbl || !uid_list) {
		EM_DEBUG_EXCEPTION("account[%p], input_mailbox_tbl[%p], uid_list[%p]", account, input_mailbox_tbl, uid_list);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	*uid_list = NULL;

	if (account->incoming_server_type == EMAIL_SERVER_TYPE_POP3) {
		if (!pop3_mailbox_get_uids(stream, uid_list, &err)) {
			EM_DEBUG_EXCEPTION("pop3_mailbox_get_uids failed - %d", err);
			goto FINISH_OFF;
		}
	}
	else {	/*  EMAIL_SERVER_TYPE_IMAP4 */
		EM_DEBUG_LOG("nmsgs[%d]", ((MAILSTREAM *)stream)->nmsgs);
		if ((limit_count < ((MAILSTREAM *)stream)->nmsgs)) {
			if ((err = emcore_get_uids_order_by_datetime_from_imap_server(stream, limit_count, uid_list)) != EMAIL_ERROR_NONE ) {
				EM_DEBUG_EXCEPTION("emcore_get_uids_order_by_datetime_from_imap_server failed [%d]", err);
				if (err  != EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER)
					goto FINISH_OFF;
			}
		}
		else {
			if(!imap4_mailbox_get_uids(stream, NULL, uid_list, &err)) {
				EM_DEBUG_EXCEPTION("imap4_mailbox_get_uids failed [%d]", err);
				if (err  != EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER)
					goto FINISH_OFF;
			}
		}
	}

	if (!emstorage_get_downloaded_list(multi_user_name, input_mailbox_tbl->account_id,
		(account->incoming_server_type == EMAIL_SERVER_TYPE_POP3) ? 0 : input_mailbox_tbl->mailbox_id,
		&downloaded_uids, &downloaded_uid_count, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_downloaded_list failed [%d]", err);
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("Number of Mails in Downloaded Table [%d]", downloaded_uid_count);

	uid_elem = *uid_list;
	uid_count = 0;

	if(!uid_elem) { /* If there is no mail in the input_mailbox_tbl, remove all mails in the input_mailbox_tbl */
		for (i = 0; i < downloaded_uid_count; i++) {
			downloaded_uids[i].sync_status = EMAIL_SYNC_STATUS_NOT_EXIST_ON_SERVER;
		}
	}

	EM_PROFILE_BEGIN(emCoreGetUidsDownloadWhilwLoop);

	/* Sort uid_elem list based on datetime */
	elem_item = uid_elem;

	while (elem_item) {
		item_list = g_list_append(item_list, elem_item);
		elem_item = elem_item->next;
	}

	if (item_list) {
		item_list = g_list_sort(item_list, emcore_compare_uid_elem);
		item_iter = g_list_first(item_list);

		while (item_iter) {
			item_next = g_list_next(item_iter);
			if (item_next) {
				elem_item = item_iter->data;
				elem_item->next = (emcore_uid_list *)item_next->data;
			} else {
				elem_item = item_iter->data;
				elem_item->next = NULL;
			}

			item_iter = g_list_next(item_iter);
		}

		item_iter = g_list_first(item_list);
		uid_elem = item_iter->data;

		g_list_free(item_list);
	}
	/* Sorting completed */

	while (uid_elem) {
		next_uid_elem = uid_elem->next;

		if ((account->retrieval_mode & EMAIL_IMAP4_RETRIEVAL_MODE_NEW) && (uid_elem->flag.seen != 0)) {		/*  already seen */
			EM_SAFE_FREE (uid_elem->uid);
			EM_SAFE_FREE (uid_elem->internaldate);
			EM_SAFE_FREE (uid_elem);
		}
		else {
			int to_be_downloaded = 1;

			if (limit_count > 0 && uid_count >= limit_count){
				/* EM_DEBUG_LOG("hit the limit[%d] for [%s]", limit_count, uid_elem->uid); */
				to_be_downloaded = 0;
			}
			else{
				for (i = downloaded_uid_count; i > 0; i--) {
					if (downloaded_uids[i - 1].sync_status == EMAIL_SYNC_STATUS_NOT_EXIST_ON_SERVER
							&& !EM_SAFE_STRCMP(uid_elem->uid, downloaded_uids[i - 1].server_uid)) {
						if (downloaded_uids[i - 1].mailbox_id != input_mailbox_tbl->mailbox_id) {
							uid_count--;
						}
						else {
							/* The mail exists on server and local storage, so it should not be downloaded, just check seen flag */
							if(downloaded_uids[i - 1].flags_seen_field != uid_elem->flag.seen ||
									downloaded_uids[i - 1].flags_flagged_field != uid_elem->flag.flagged) {
								if(downloaded_uids[i - 1].flags_seen_field != uid_elem->flag.seen) {
									downloaded_uids[i - 1].sync_status = EMAIL_SYNC_STATUS_FLAG_CHANGED;
									downloaded_uids[i - 1].flags_seen_field = uid_elem->flag.seen;
								}
								if (downloaded_uids[i - 1].flags_flagged_field != uid_elem->flag.flagged) {
									downloaded_uids[i - 1].sync_status = EMAIL_SYNC_STATUS_FLAG_CHANGED;
									downloaded_uids[i - 1].flags_flagged_field = uid_elem->flag.flagged;
								}
							} else {
								downloaded_uids[i - 1].sync_status = EMAIL_SYNC_STATUS_EXIST_ON_SERVER;
							}
						}
						to_be_downloaded = 0;
						break;
					}
				}
			}

			//EM_DEBUG_LOG("Is uid[%s] datetime[%s] going to be downloaded ? [%d]", uid_elem->uid, uid_elem->internaldate, to_be_downloaded);

			if (to_be_downloaded) {
				if (retrieve_mode == EMAIL_SYNC_OLDEST_MAILS_FIRST){
					uid_elem->next = head_uid_elem;
					head_uid_elem = uid_elem;
				}
				else{	/* if retrieve_mode is EMAIL_SYNC_LATEST_MAILS_FIRST, add uid elem to end so that latest mails are in front of list */
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
				EM_SAFE_FREE (uid_elem->uid);
				EM_SAFE_FREE (uid_elem);
			}

			uid_count++;
		}

		uid_elem = next_uid_elem;
	}

	EM_PROFILE_END(emCoreGetUidsDownloadWhilwLoop);
	EM_PROFILE_BEGIN(emCoreGetUidsDownloadForLoop);

	for (i = 0; i < downloaded_uid_count; i++) {
		/*  EM_DEBUG_LOG("input_mailbox_tbl[%s] sync_status[%d]", input_mailbox_tbl->name, downloaded_uids[i].sync_status); */
		if ((downloaded_uids[i].sync_status == EMAIL_SYNC_STATUS_NOT_EXIST_ON_SERVER) &&
			(EM_SAFE_STRCMP(downloaded_uids[i].mailbox_name, EMAIL_SEARCH_RESULT_MAILBOX_NAME) != 0)) {		/*  deleted on server */
			if (!emstorage_get_maildata_by_servermailid(multi_user_name, input_mailbox_tbl->mailbox_id, downloaded_uids[i].server_uid, &mail, true, &err)){
				EM_DEBUG_EXCEPTION("emstorage_get_maildata_by_servermailid for uid[%s] Failed [%d] \n ", downloaded_uids[i].server_uid, err);
				if (err == EMAIL_ERROR_MAIL_NOT_FOUND){
					continue;
				}
			}

			if (account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
				ret = emcore_delete_mails_from_local_storage(multi_user_name, input_mailbox_tbl->account_id, &(mail->mail_id), 1, EMAIL_DELETED_FROM_SERVER, false, &err);
				/* Update badge count */
				emcore_display_unread_in_badge(multi_user_name);
				if (!ret) {
					EM_DEBUG_EXCEPTION("emcore_delete_mails_from_local_storage falied - %d", err);
					goto FINISH_OFF;
				}
			}

			if (!emstorage_remove_downloaded_mail(multi_user_name, input_mailbox_tbl->account_id, input_mailbox_tbl->mailbox_name, downloaded_uids[i].server_uid, true, &err)) {   /*  remove uid from uid list */
				EM_DEBUG_EXCEPTION("emstorage_remove_downloaded_mail failed - %d", err);
				/* goto FINISH_OFF; */
			}

		}
		else if (account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
			/* set seen flag of downloaded mail */
			if (downloaded_uids[i].sync_status == EMAIL_SYNC_STATUS_FLAG_CHANGED) {
				if (!emcore_set_flags_field(multi_user_name, downloaded_uids[i].account_id, &(downloaded_uids[i].local_uid), 1, EMAIL_FLAGS_FLAGGED_FIELD, downloaded_uids[i].flags_flagged_field, &err)) {
					EM_DEBUG_EXCEPTION("emcore_set_flags_field failed [%d]", err);
				}

				if (!emcore_set_flags_field(multi_user_name, downloaded_uids[i].account_id, &(downloaded_uids[i].local_uid), 1, EMAIL_FLAGS_SEEN_FIELD, downloaded_uids[i].flags_seen_field, &err)) {
					EM_DEBUG_EXCEPTION("emcore_set_flags_field failed [%d]", err);
				}
				else
					emcore_display_unread_in_badge(multi_user_name);
			}


		}
		if (mail != NULL)
			emstorage_free_mail(&mail, 1, NULL);
		mail = NULL;
	}
	EM_PROFILE_END(emCoreGetUidsDownloadForLoop);

	*uid_list = head_uid_elem;
	*uids = uid_to_be_downloaded_count;

	ret = true;

FINISH_OFF:
	if (ret == false){
		if (head_uid_elem)
			emcore_free_uids(head_uid_elem, NULL);
	}

	if (downloaded_uids != NULL)
		emstorage_free_read_mail_uid(&downloaded_uids, downloaded_uid_count, NULL);

	if (mail != NULL)
		emstorage_free_mail(&mail, 1, NULL);

	if (err_code  != NULL)
		*err_code = err;

	EM_PROFILE_END(emCoreGetUidsDownload);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/* insert received mail UID to read mail uid table */
static int emcore_add_read_mail_uid(char *multi_user_name, emstorage_mailbox_tbl_t *input_maibox_data, char *server_mailbox_name, int mail_id, char *uid, int rfc822_size, int rule_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("input_maibox_data[%p], server_mailbox_name[%s], uid[%s], rfc822_size[%d], rule_id[%d], err_code[%p]", input_maibox_data, server_mailbox_name, uid, rfc822_size, rule_id, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	emstorage_read_mail_uid_tbl_t  read_mail_uid = { 0 };
	emstorage_mailbox_tbl_t       *mailbox_tbl = NULL;

	if (!input_maibox_data || !uid) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	read_mail_uid.account_id = input_maibox_data->account_id;

	if (!(input_maibox_data->mailbox_id) || !(server_mailbox_name)) {
		if (!emstorage_get_mailbox_by_mailbox_type(multi_user_name, input_maibox_data->account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox_tbl, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	if (input_maibox_data->mailbox_id)
		read_mail_uid.mailbox_id = input_maibox_data->mailbox_id;
	else
		read_mail_uid.mailbox_id = mailbox_tbl->mailbox_id;

	read_mail_uid.local_uid = mail_id;


	if (server_mailbox_name)
		read_mail_uid.mailbox_name = server_mailbox_name;
	else
		read_mail_uid.mailbox_name = mailbox_tbl->mailbox_name;

	read_mail_uid.server_uid = uid;
	read_mail_uid.rfc822_size = rfc822_size;

	if (!emstorage_add_downloaded_mail(multi_user_name, &read_mail_uid, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_add_downloaded_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

#ifdef __FEATURE_BODY_SEARCH__
int emcore_add_mail_text(char *multi_user_name, emstorage_mailbox_tbl_t *input_maibox_data, emstorage_mail_tbl_t *input_new_mail_tbl_data, char *stripped_text, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_maibox_data[%p], input_new_mail_tbl_data[%p], err_code[%p]", input_maibox_data, input_new_mail_tbl_data, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	emstorage_mail_text_tbl_t mail_text = { 0, };

	if (!input_maibox_data || !input_new_mail_tbl_data) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	mail_text.mail_id    = input_new_mail_tbl_data->mail_id;
	mail_text.account_id = input_maibox_data->account_id;
	mail_text.mailbox_id = input_maibox_data->mailbox_id;

	if (stripped_text)
		mail_text.body_text = EM_SAFE_STRDUP(stripped_text);
	else
		mail_text.body_text = NULL;

	if (!emstorage_add_mail_text(multi_user_name, &mail_text, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_add_mail_text failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(mail_text.body_text);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
#endif

INTERNAL_FUNC int emcore_add_mail_to_mailbox(char *multi_user_name, emstorage_mailbox_tbl_t *input_maibox_data, emstorage_mail_tbl_t *input_new_mail_tbl_data, int *output_mail_id, int *output_thread_id)
{
	EM_DEBUG_FUNC_BEGIN("input_maibox_data[%p], input_new_mail_tbl_data[%p], uid_elem[%p], output_mail_id[%p], output_thread_id[%p]", mail_stream, input_maibox_data, input_new_mail_tbl_data, output_mail_id, output_thread_id);

	int                   err = EMAIL_ERROR_NONE;
	int                   thread_id = -1;
	int                   thread_item_count = 0;
	int                   latest_mail_id_in_thread = -1;

	if (!input_maibox_data || !input_new_mail_tbl_data) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	input_new_mail_tbl_data->account_id            = input_maibox_data->account_id;
	input_new_mail_tbl_data->mailbox_id            = input_maibox_data->mailbox_id;
	input_new_mail_tbl_data->mailbox_type          = input_maibox_data->mailbox_type;

	emstorage_begin_transaction(multi_user_name, NULL, NULL, NULL);

	/* Get the Mail_id */
	if (!emstorage_increase_mail_id(multi_user_name, &(input_new_mail_tbl_data->mail_id), false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_increase_mail_id failed [%d]", err);
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
		goto FINISH_OFF;
	}

	if (emstorage_get_thread_id_of_thread_mails(multi_user_name, input_new_mail_tbl_data, &thread_id, &latest_mail_id_in_thread, &thread_item_count) != EMAIL_ERROR_NONE)
		EM_DEBUG_LOG(" emstorage_get_thread_id_of_thread_mails is failed.");

	if (thread_id == -1){
		input_new_mail_tbl_data->thread_id = input_new_mail_tbl_data->mail_id;
		input_new_mail_tbl_data->thread_item_count = thread_item_count = 1;
	}
	else {
		input_new_mail_tbl_data->thread_id = thread_id;
		thread_item_count++;
	}

        EM_DEBUG_LOG("MULTI USER NAME: [%s]", multi_user_name);
        input_new_mail_tbl_data->user_name = EM_SAFE_STRDUP(multi_user_name);

	if (!emstorage_add_mail(multi_user_name, input_new_mail_tbl_data, 0, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_add_mail failed [%d]", err);
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
		goto FINISH_OFF;
	}

	if (thread_item_count > 1) {
		if (!emstorage_update_latest_thread_mail(multi_user_name, input_new_mail_tbl_data->account_id, input_new_mail_tbl_data->mailbox_id, input_new_mail_tbl_data->thread_id, NULL, -1, -1, NOTI_THREAD_ID_CHANGED_BY_ADD, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_update_latest_thread_mail failed [%d]", err);
			emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
			goto FINISH_OFF;
		}

		if (!emstorage_update_latest_thread_mail(multi_user_name, input_new_mail_tbl_data->account_id, input_new_mail_tbl_data->mailbox_id, input_new_mail_tbl_data->thread_id, NULL, 0, 0, NOTI_THREAD_ID_CHANGED_BY_ADD, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_update_latest_thread_mail failed [%d]", err);
			emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
			goto FINISH_OFF;
		}
	}

	if (emstorage_get_thread_id_of_thread_mails(multi_user_name, input_new_mail_tbl_data, &thread_id, &latest_mail_id_in_thread, &thread_item_count) != EMAIL_ERROR_NONE)
		EM_DEBUG_LOG(" emstorage_get_thread_id_of_thread_mails is failed.");

	if (output_thread_id)
		*output_thread_id = thread_id;

	if (output_mail_id != NULL)
		*output_mail_id = input_new_mail_tbl_data->mail_id;

	if (!emcore_add_read_mail_uid(multi_user_name, input_maibox_data, input_maibox_data->mailbox_name, input_new_mail_tbl_data->mail_id, input_new_mail_tbl_data->server_mail_id, input_new_mail_tbl_data->mail_size, 0, &err)) {
		EM_DEBUG_EXCEPTION("emcore_add_read_mail_uid failed [%d]", err);
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
		goto FINISH_OFF;
	}

#ifdef __FEATURE_BODY_SEARCH__
	if (!emcore_add_mail_text(multi_user_name, input_maibox_data, input_new_mail_tbl_data, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emcore_add_mail_text failed [%d]", err);
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
		goto FINISH_OFF;
	}
#endif

	emstorage_commit_transaction(multi_user_name, NULL, NULL, NULL);

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

int emcore_check_rule(const char *input_full_address_from, const char *input_subject, emstorage_rule_tbl_t *rule, int rule_count, int *priority_sender, int *blocked, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_full_address_from [%p], input_subject [%p], rule [%p], rule_count [%d], priority_sender [%p], blocked [%p], err_code [%p]", input_full_address_from, input_subject, rule, rule_count, priority_sender, blocked, err_code);

	int ret = false, err = EMAIL_ERROR_NONE, i = 0;
	size_t len = 0;
	char *from_address = NULL;
	char *p_input_full_address_from = NULL;
	ADDRESS *addr = NULL;

	if (!input_full_address_from ) { /*input_subject could be null*/
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		return ret; /*prevent 54914*/
	}

	p_input_full_address_from = strdup((char *)input_full_address_from);

	rfc822_parse_adrlist(&addr, p_input_full_address_from, NULL);

	if(addr) {
		EM_DEBUG_LOG_SEC("rule : full_address_from[%s], addr->mailbox[%s], addr->host[%s]", p_input_full_address_from, addr->mailbox, addr->host);

		if (addr->mailbox)
			len = EM_SAFE_STRLEN(addr->mailbox);
		if (addr->host)
			len  += EM_SAFE_STRLEN(addr->host);
			len  += 2;

		if (!(from_address = em_malloc(len))) {
			EM_DEBUG_EXCEPTION("em_malloc failed...");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		SNPRINTF(from_address, len, "%s@%s", addr->mailbox, addr->host);
	}
	else  {
		EM_DEBUG_EXCEPTION("rfc822_parse_adrlist failed.");
		err = EMAIL_ERROR_INVALID_ADDRESS;
		goto FINISH_OFF;
	}

	for (i = 0; i < rule_count ; i++) {

		EM_DEBUG_LOG("rule[%d]: rule_id[%d], flag1[%d], action_type[%d]", i, rule[i].rule_id, rule[i].flag1, rule[i].action_type);

		if (!rule[i].flag1) { /*flag1 means "OFF(0)/ON(1)"*/
			continue;
		}

		switch (rule[i].action_type) {
		case EMAIL_FILTER_DELETE :
			EM_DEBUG_LOG("Not support action type");
			break;

		case EMAIL_FILTER_BLOCK :
			if (rule[i].type == EMAIL_FILTER_SUBJECT) {
				if (input_subject && rule[i].value) {
					if (RULE_TYPE_INCLUDES == rule[i].flag2) {
						if (strcasestr(input_subject, rule[i].value))
							*blocked = true;
					}
					else if (RULE_TYPE_EXACTLY == rule[i].flag2) {
						if (!strcasecmp(input_subject, rule[i].value))
							*blocked = true;
					}
				}
			}

			if (rule[i].type == EMAIL_FILTER_FROM) {
				if (from_address && rule[i].value2) {
					if (RULE_TYPE_INCLUDES == rule[i].flag2) {
						if (strcasestr(from_address, rule[i].value2))
							*blocked = true;
					}
					else if (RULE_TYPE_EXACTLY == rule[i].flag2) {
						if (!strcasecmp(from_address, rule[i].value2))
							*blocked = true;
					}
#ifdef __FEATURE_COMPARE_DOMAIN__
					else if (RULE_TYPE_COMPARE_DOMAIN == rule[i].flag2) {
						char *domain_start = NULL;
						domain_start = strchr(from_address, '@');
						if (domain_start && strncmp(domain_start, rule[i].value2, strlen(rule[i].value2)) == 0)
							*blocked = true;
					}
#endif /* __FEATURE_COMPARE_DOMAIN__ */
				}
			}

			break;

		default:
			if (rule[i].type != EMAIL_PRIORITY_SENDER)
				break;

			if (from_address && rule[i].value2) {
				if (RULE_TYPE_INCLUDES == rule[i].flag2) {
					if (strcasestr(from_address, rule[i].value2))
						*priority_sender = 1;
				}
				else if (RULE_TYPE_EXACTLY == rule[i].flag2) {
					if (!strcasecmp(from_address, rule[i].value2))
						*priority_sender = 1;
				}
			}
			break;
		}
	}
	ret = true;

FINISH_OFF:

	EM_DEBUG_LOG("blocked [%d], priority_sender [%d]", *blocked, *priority_sender);

	EM_SAFE_FREE(from_address);
	EM_SAFE_FREE(p_input_full_address_from);

	if (addr  != NULL)
		mail_free_address(&addr);

	if (err_code  != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emcore_make_mail_tbl_data_from_envelope(char *multi_user_name, int account_id, MAILSTREAM *mail_stream, ENVELOPE *input_envelope, emcore_uid_list *input_uid_elem, emstorage_mail_tbl_t **output_mail_tbl_data,  int *err_code)
{
	EM_PROFILE_BEGIN(emCoreParseEnvelope);
	EM_DEBUG_FUNC_BEGIN("input_envelope[%p], input_uid_elem [%p], output_mail_tbl_data[%p],  err_code[%p]", input_envelope, input_uid_elem, output_mail_tbl_data, err_code);

	int zone_hour = 0;
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int priority = 3;
	int req_read_receipt = 0;
	struct tm             temp_time_info;
	MESSAGECACHE         *mail_cache_element = NULL;
	emstorage_mail_tbl_t *temp_mail_tbl_data = NULL;
	email_account_t      *account_ref = NULL;
	char *rfc822_subject = NULL;
	char *rfc822_from = NULL;

	account_ref = emcore_get_account_reference(multi_user_name, account_id);
	if (!account_ref) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed - %d", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!output_mail_tbl_data) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!input_envelope) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(temp_mail_tbl_data = em_malloc(sizeof(emstorage_mail_tbl_t)))) {
		EM_DEBUG_EXCEPTION("em_malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (!(mail_cache_element = mail_elt(mail_stream, input_uid_elem->msgno))) {
		EM_DEBUG_EXCEPTION("mail_elt failed...");
		err = EMAIL_ERROR_UNKNOWN;
		goto FINISH_OFF;
	}

	if (g_strrstr(account_ref->incoming_server_address, "outlook.com")) {
		if (!emcore_get_mail_extra_info(mail_stream, input_uid_elem->msgno, &rfc822_subject, &rfc822_from, &req_read_receipt, &priority, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_mail_extra_info failed [%d]", err);
			goto FINISH_OFF;
		}
	} else {
		if (!emcore_get_mail_extra_info(mail_stream, input_uid_elem->msgno, NULL, NULL, &req_read_receipt, &priority, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_mail_extra_info failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	if (rfc822_subject) {
		EM_SAFE_FREE(input_envelope->subject);
		input_envelope->subject = g_strdup(rfc822_subject);
		EM_DEBUG_LOG_SEC("rfc822_subject:%s", input_envelope->subject);
		EM_SAFE_FREE(rfc822_subject);
	}

	if (rfc822_from) {
		if (input_envelope->from) {
			EM_SAFE_FREE(input_envelope->from->personal);
			input_envelope->from->personal = g_strdup(rfc822_from);
			EM_DEBUG_LOG_SEC("rfc822_from:%s", input_envelope->from->personal);
		}
		EM_SAFE_FREE(rfc822_from);
	}

	if (input_envelope->subject) {
		temp_mail_tbl_data->subject = emcore_gmime_get_decoding_text(input_envelope->subject);
		em_trim_left(temp_mail_tbl_data->subject);

		char *charset = NULL;
		char *charset_end = NULL;
		char *charset_start = NULL;
		if(g_str_has_prefix(input_envelope->subject, "=?")) {
			charset_start = input_envelope->subject + 2;
			charset_end = strstr(charset_start, "?");
			if (charset_end) {
				charset = g_strndup(charset_start, charset_end - charset_start);
			}
		}

		EM_DEBUG_LOG("DEFAULT CHARSET : %s", charset);
		if (charset) {
			temp_mail_tbl_data->default_charset = g_ascii_strup(charset, -1);
			EM_SAFE_FREE(charset);
		}
	}

	if (input_envelope->from) {
		if (!emcore_get_utf8_address(&temp_mail_tbl_data->full_address_from, input_envelope->from, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_utf8_address failed [%d]", err);
			goto FINISH_OFF;
		}


	}

	if (input_envelope->to) {
		if (!emcore_get_utf8_address(&temp_mail_tbl_data->full_address_to, input_envelope->to, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_utf8_address failed [%d]", err);
			goto FINISH_OFF;
		}


	}

	if (input_envelope->cc) {
		if (!emcore_get_utf8_address(&temp_mail_tbl_data->full_address_cc, input_envelope->cc, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_utf8_address failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG_SEC("full_address_cc [%s]", temp_mail_tbl_data->full_address_cc);
	}

	if (input_envelope->bcc) {
		if (!emcore_get_utf8_address(&temp_mail_tbl_data->full_address_bcc, input_envelope->bcc, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_utf8_address failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG_SEC("full_address_bcc [%s]", temp_mail_tbl_data->full_address_bcc);
	}

	if (input_envelope->reply_to) {
		if (!emcore_get_utf8_address(&temp_mail_tbl_data->full_address_reply, input_envelope->reply_to, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_utf8_address failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG_SEC("full_address_reply [%s]\n", temp_mail_tbl_data->full_address_reply);
	}

	if (input_envelope->return_path) {
		if (!emcore_get_utf8_address(&temp_mail_tbl_data->full_address_return , input_envelope->return_path, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_utf8_address failed [%d]", err);
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG_SEC("full_address_return[%s]", temp_mail_tbl_data->full_address_return);
	}

	temp_mail_tbl_data->message_id            = EM_SAFE_STRDUP(input_envelope->message_id);

	memset((void*)&temp_time_info, 0,  sizeof(struct tm));



	zone_hour = mail_cache_element->zhours * (mail_cache_element->zoccident?-1:1);

	temp_time_info.tm_sec  = mail_cache_element->seconds;
	temp_time_info.tm_min  = mail_cache_element->minutes + mail_cache_element->zminutes;
	temp_time_info.tm_hour = mail_cache_element->hours - zone_hour;

	if(temp_time_info.tm_hour < 0) {
		temp_time_info.tm_mday = mail_cache_element->day - 1;
		temp_time_info.tm_hour += 24;
	}
	else
		temp_time_info.tm_mday = mail_cache_element->day;

	temp_time_info.tm_mon  = mail_cache_element->month - 1;
	temp_time_info.tm_year = mail_cache_element->year + 70;

	temp_mail_tbl_data->date_time             = timegm(&temp_time_info);

	temp_mail_tbl_data->server_mail_status    = 1;
	temp_mail_tbl_data->server_mail_id        = EM_SAFE_STRDUP(input_uid_elem->uid);
	temp_mail_tbl_data->mail_size             = mail_cache_element->rfc822_size;
	temp_mail_tbl_data->flags_seen_field      = input_uid_elem->flag.seen;
	temp_mail_tbl_data->flags_deleted_field   = mail_cache_element->deleted;
	temp_mail_tbl_data->flags_flagged_field   = mail_cache_element->flagged;
	temp_mail_tbl_data->flags_answered_field  = mail_cache_element->answered;
	temp_mail_tbl_data->flags_recent_field    = mail_cache_element->recent;
	temp_mail_tbl_data->flags_draft_field     = mail_cache_element->draft;
	temp_mail_tbl_data->flags_forwarded_field = input_uid_elem->flag.forwarded;
	temp_mail_tbl_data->priority              = priority;
	temp_mail_tbl_data->report_status         = (req_read_receipt ? EMAIL_MAIL_REQUEST_MDN :  0);
	temp_mail_tbl_data->attachment_count      = 0;
	temp_mail_tbl_data->eas_data_length       = 0;
	temp_mail_tbl_data->eas_data              = NULL;

	temp_mail_tbl_data->account_id            = account_id;
	emcore_fill_address_information_of_mail_tbl(multi_user_name, temp_mail_tbl_data);

	*output_mail_tbl_data = temp_mail_tbl_data;
	temp_mail_tbl_data = NULL;

	ret = true;

FINISH_OFF:

	if (ret != true) {
		if (temp_mail_tbl_data) {
			emstorage_free_mail(&temp_mail_tbl_data, 1, NULL);
			temp_mail_tbl_data = NULL;
		}
	}

	if (account_ref) {
		emcore_free_account(account_ref);
		EM_SAFE_FREE(account_ref);
	}

	if (err_code  != NULL)
		*err_code = err;

	EM_PROFILE_END(emCoreParseEnvelope);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emcore_sync_header (char *multi_user_name, 
                                       emstorage_mailbox_tbl_t *input_mailbox_tbl, 
                                       void **stream, 
                                       emcore_uid_list **input_uid_list, 
                                       int *mail_count, 
                                       int *unread_mail,
                                       int *vip_mail_count,
                                       int *vip_unread_mail,
                                       int  cancellable, /*0: excluding event thd*/
                                       int  event_handle,
                                       int *err_code)
{
	EM_PROFILE_BEGIN(emCoreSyncHeader);
	EM_DEBUG_FUNC_BEGIN("input_mailbox_tbl[%p], input_uid_list [%p], err_code[%p]", input_mailbox_tbl, input_uid_list, err_code);
	if (!stream) {
		EM_DEBUG_EXCEPTION ("NULL stream");
		if (err_code)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int err = EMAIL_ERROR_NONE, err_2 = EMAIL_ERROR_NONE;
	int err_from_vconf = 0;
	int download_limit_count;
	email_account_t      *account_ref = NULL;
	emstorage_rule_tbl_t *rule = NULL;
	emcore_uid_list      *uid_list = NULL;
	emcore_uid_list      *uid_elem = NULL;
	emstorage_mail_tbl_t *new_mail_tbl_data = NULL;
	emstorage_mailbox_tbl_t *destination_mailbox = NULL;

	ENVELOPE *env = NULL;
	int account_id = 0, mail_id = 0, rule_count = 1000, i = 0, percentage  = 0, thread_id = -1;
	int total = 0, unread = 0, vip_total = 0, vip_unread = 0;
	char *uid_range = NULL;
	char mailbox_id_param_string[10] = {0,};

	if (!input_mailbox_tbl) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM:input_mailbox_tbl[%p]", input_mailbox_tbl);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	account_id = input_mailbox_tbl->account_id;
	g_current_sync_account_id = account_id;
	account_ref = emcore_get_account_reference(multi_user_name, account_id);
	if (!account_ref) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed - %d", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (account_ref->sync_disabled == 1) {
		err = EMAIL_ERROR_ACCOUNT_SYNC_IS_DISALBED;
		EM_DEBUG_LOG("Sync disabled for this account. Do not sync.");
		goto FINISH_OFF;
	}


	if (account_ref->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4 && input_mailbox_tbl->local_yn == 1) {
		/* mailbox recovery starts */
		if (input_mailbox_tbl->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) {
			if (!emcore_sync_mailbox_list (multi_user_name, account_id, "", event_handle, &err)) {
				EM_DEBUG_EXCEPTION("emcore_sync_mailbox_list error [%d] account_id [%d]", err, account_id);
				goto FINISH_OFF;
			}
		}
		else {
			EM_DEBUG_EXCEPTION("because mailbox_name[%s] mailbox_type[%d] of account[%d] is a local box, it cant be synced",
                              input_mailbox_tbl->mailbox_name, input_mailbox_tbl->mailbox_type, account_id);
			err = EMAIL_ERROR_INVALID_MAILBOX;
			goto FINISH_OFF;
		}
	}

	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

#ifdef __FEATURE_SUPPORT_SYNC_STATE_ON_NOTI_BAR__
	if ((err_from_vconf = vconf_set_int(VCONFKEY_EMAIL_SYNC_STATE, 1)) != 0 ) {
		EM_DEBUG_EXCEPTION("vconf_set_int failed (sync state) [%d]", err_from_vconf);
	}
#endif /* __FEATURE_SUPPORT_SYNC_STATE_ON_NOTI_BAR__ */

	if (!emcore_connect_to_remote_mailbox(multi_user_name, account_id, input_mailbox_tbl->mailbox_id, (void **)stream, &err) || !*stream) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed - %d", err);
		goto FINISH_OFF;
	}

	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	/*  save total mail count on server to DB */
	if (!emstorage_update_mailbox_total_count(multi_user_name, account_id, input_mailbox_tbl->mailbox_id, ((MAILSTREAM *)*stream)->nmsgs, 1, &err)){
		EM_DEBUG_EXCEPTION("emstorage_update_mailbox_total_count failed [%d]", err);
		goto FINISH_OFF;
	}

	/* if (((MAILSTREAM *)stream)->nmsgs > 0) */
	{
		email_option_t *opt_ref = &account_ref->options;
		EM_DEBUG_LOG_SEC("block_address = %d, block_subject = %d", opt_ref->block_address, opt_ref->block_subject);

		if (opt_ref->block_address || opt_ref->block_subject) {
			int is_completed = false;
			int type = 0;

			if (!opt_ref->block_address)
				type = EMAIL_FILTER_SUBJECT;
			else if (!opt_ref->block_subject)
				type = EMAIL_FILTER_FROM;

			if (!emstorage_get_rule(multi_user_name, ALL_ACCOUNT, type, 0, &rule_count, &is_completed, &rule, true, &err) || !rule) {
				if ( err != EMAIL_ERROR_FILTER_NOT_FOUND )
					EM_DEBUG_EXCEPTION ("emstorage_get_rule error [%d]", err);
			}
		}

		download_limit_count = input_mailbox_tbl->mail_slot_size;
		if (!emcore_get_uids_to_download(multi_user_name, *stream, account_ref, input_mailbox_tbl, download_limit_count, &uid_list, 
                                                                         &total, EMAIL_SYNC_LATEST_MAILS_FIRST, &err)){
			EM_DEBUG_EXCEPTION("emcore_get_uids_to_download failed [%d]", err);
			goto FINISH_OFF;
		}

		if (cancellable)
			FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		if (input_uid_list && *input_uid_list){
			emcore_free_uids(*input_uid_list, NULL);
			*input_uid_list = uid_list;
		}
		uid_elem = uid_list;
		i = 0;
		EM_PROFILE_BEGIN(emCoreSyncHeaderwhileloop);

#ifdef  __FEATURE_HEADER_OPTIMIZATION__
		/* g.shyamakshi@samsung.com : Bulk fetch of headers only if the receiving server type is IMAP */

		EM_DEBUG_LOG("((MAILSTREAM *)stream)->nmsgs [%d]", ((MAILSTREAM *)*stream)->nmsgs);
		EM_DEBUG_LOG("uid_list [%p]", uid_list);

		if (account_ref->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4 && uid_list  != NULL) {

			if ((err = emcore_make_uid_range_string(uid_list, total, &uid_range)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_make_uid_range_string failed [%d]", err);
				goto FINISH_OFF;
			}

			if (cancellable)
				FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

			uid_elem = uid_list;
			if (*stream && uid_elem){
				EM_DEBUG_LOG("msgno : %d", uid_elem->msgno);
				((MAILSTREAM *)*stream)->nmsgs = uid_elem->msgno;
			}
			else{
				EM_DEBUG_LOG("Uid List Null");
			}
			EM_DEBUG_LOG("Calling ... mail_fetch_fast. uid_range [%s]", uid_range);
			mail_fetch_fast(*stream, uid_range, FT_UID | FT_PEEK | FT_NEEDENV);
			EM_SAFE_FREE(uid_range);
		}
#endif

		/*  h.gahlaut@samsung.com :  Clear the event queue of partial body download thread before starting fetching new headers  */
#ifndef __FEATURE_PARTIAL_BODY_FOR_POP3__
		if (account_ref->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4){
#endif /*  __FEATURE_PARTIAL_BODY_FOR_POP3__ */
			/*  Partial body download feature is only for IMAP accounts  */
			if (false == emcore_clear_partial_body_thd_event_que(&err))
				EM_DEBUG_LOG("emcore_clear_partial_body_thd_event_que failed [%d]", err);
#ifndef __FEATURE_PARTIAL_BODY_FOR_POP3__
		}
#endif /*  __FEATURE_PARTIAL_BODY_FOR_POP3__ */
		while (uid_elem) {
			EM_PROFILE_BEGIN(emCoreSyncHeaderEachMail);
			EM_DEBUG_LOG("mail_fetchstructure_full  :  uid_elem->msgno[%d]", uid_elem->msgno);

			env = NULL;

			if (uid_elem->msgno > ((MAILSTREAM *)*stream)->nmsgs) {
				EM_DEBUG_LOG ("WARN: msgno[%d] can't be greater than nmsgs[%d].",
						uid_elem->msgno, ((MAILSTREAM *)*stream)->nmsgs);
			}
			else{
#ifdef __FEATURE_HEADER_OPTIMIZATION__
				if (account_ref->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) /* Fetch env from cache in case of IMAP */
					env = mail_fetchstructure_full (*stream, uid_elem->msgno, NULL, FT_PEEK, 0);
				else /* Fetch header from network in case of POP */
					env = mail_fetchstructure_full (*stream, uid_elem->msgno, NULL, FT_PEEK, 1);
#else
				env = mail_fetchstructure_full (*stream, uid_elem->msgno, NULL, FT_PEEK);
#endif
			}

			if (cancellable)
				FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

			if (env != NULL) {
				int searched_mail_id = 0;
				int blocked = false;
				int priority_sender = false;

				if (!emcore_make_mail_tbl_data_from_envelope (multi_user_name, account_id, *stream, env, uid_elem, 
                                                                     &new_mail_tbl_data, &err) || !new_mail_tbl_data) {
					EM_DEBUG_EXCEPTION("emcore_make_mail_tbl_data_from_envelope failed [%d]", err);
					goto CONTINUE_NEXT;
				}

				/* Check message_id for duplicated mail in sentbox */
				if ((input_mailbox_tbl->mailbox_type == EMAIL_MAILBOX_TYPE_SENTBOX) && 
                                        (emstorage_check_and_update_server_uid_by_message_id(multi_user_name, 
                                                                                             new_mail_tbl_data->account_id, 
                                                                                             input_mailbox_tbl->mailbox_type, 
                                                                                             new_mail_tbl_data->message_id, 
                                                                                             new_mail_tbl_data->server_mail_id, 
                                                                                             &searched_mail_id) == EMAIL_ERROR_NONE)) {
					EM_DEBUG_LOG("Existed the duplicated mail : message_id[%s]", new_mail_tbl_data->message_id);

					if (!emcore_add_read_mail_uid(multi_user_name, input_mailbox_tbl, input_mailbox_tbl->mailbox_name, searched_mail_id, new_mail_tbl_data->server_mail_id, new_mail_tbl_data->mail_size, 0, &err)) {
						EM_DEBUG_EXCEPTION("emcore_add_read_mail_uid failed");
					}

					goto CONTINUE_NEXT;
				}

				if (rule && !emcore_check_rule(new_mail_tbl_data->full_address_from, new_mail_tbl_data->subject, rule, rule_count, &priority_sender, &blocked, &err)) {
					EM_DEBUG_EXCEPTION("emcore_check_rule failed [%d]", err);
				}

				if(destination_mailbox) /* cleanup before reusing */
					emstorage_free_mailbox(&destination_mailbox, 1, NULL);

				if (priority_sender) {
					new_mail_tbl_data->tag_id = PRIORITY_SENDER_TAG_ID;
					vip_total ++;
				}

				if (blocked && (input_mailbox_tbl->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX)) {
					if (input_mailbox_tbl->mailbox_type != EMAIL_MAILBOX_TYPE_SPAMBOX) {
						EM_DEBUG_LOG("This mail would be added to spambox");
						if (!emstorage_get_mailbox_by_mailbox_type(multi_user_name, 
                                                                account_id, 
                                                                EMAIL_MAILBOX_TYPE_SPAMBOX, 
                                                                &destination_mailbox, 
                                                                false, 
                                                                &err)) {
							EM_DEBUG_LOG("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
						}
					}
					else
						blocked = 0;
				}

				/* Set the noti waited */
				new_mail_tbl_data->save_status = EMAIL_MAIL_STATUS_NOTI_WAITED;

				if (destination_mailbox) {
					if (destination_mailbox->local_yn == 0 || account_ref->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
						if ((err = emcore_move_mail_on_server_by_server_mail_id((void*)*stream, new_mail_tbl_data->server_mail_id, destination_mailbox->mailbox_name)) != EMAIL_ERROR_NONE){
							EM_DEBUG_EXCEPTION("emcore_move_mail_on_server_by_server_mail_id falied [%d]", err);
							goto CONTINUE_NEXT;
						}
					}
					else { /* local mailbox */
						if ((err = emcore_add_mail_to_mailbox(multi_user_name, 
                                                            destination_mailbox, 
                                                            new_mail_tbl_data, 
                                                            &mail_id, 
                                                            &thread_id)) != EMAIL_ERROR_NONE) {
							EM_DEBUG_EXCEPTION("emcore_add_mail_to_mailbox falied [%d]", err);
							goto CONTINUE_NEXT;
						}

					}
					SNPRINTF(mailbox_id_param_string, 10, "%d", destination_mailbox->mailbox_id);
				} else {
					/*  add mails to specified mail box */
					EM_DEBUG_LOG_SEC("mail[%d] moved to input_mailbox_tbl [%s]", mail_id, input_mailbox_tbl->mailbox_name);
					if ( (err = emcore_add_mail_to_mailbox(multi_user_name, 
                                                        input_mailbox_tbl, 
                                                        new_mail_tbl_data, 
                                                        &mail_id, 
                                                        &thread_id)) != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("emcore_add_mail_to_mailbox falied [%d]", err);
						goto CONTINUE_NEXT;
					}

#ifndef __FEATURE_PARTIAL_BODY_FOR_POP3__
					if (account_ref->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
#endif /*  __FEATURE_PARTIAL_BODY_FOR_POP3__ */
						if (account_ref->auto_download_size != 0) {
							if (false == emcore_initiate_pbd (multi_user_name, 
                                                            *stream, 
                                                            account_id, 
                                                            mail_id, 
                                                            uid_elem->uid,
                                                            input_mailbox_tbl->mailbox_id, 
                                                            &err))
								EM_DEBUG_LOG("Partial body download initiation failed [%d]", err);
						}
#ifndef __FEATURE_PARTIAL_BODY_FOR_POP3__
					}
#endif /*  __FEATURE_PARTIAL_BODY_FOR_POP3__ */

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
					if (input_mailbox_tbl->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) {
						if (!emcore_insert_auto_download_job(multi_user_name, 
                                                            account_id, 
                                                            input_mailbox_tbl->mailbox_id, 
                                                            mail_id, 
                                                            /*account_ref->wifi_auto_download*/ 1, 
                                                            uid_elem->uid, 
                                                            &err))
							EM_DEBUG_LOG("emcore_insert_auto_download_job failed [%d]", err);
					}
#endif
/*
					if (!uid_elem->flag.seen && input_mailbox_tbl->mailbox_type != EMAIL_MAILBOX_TYPE_SPAMBOX)
						emcore_add_notification_for_unread_mail(new_mail_tbl_data);
*/
					if (cancellable)
						FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

					if (!uid_elem->flag.seen) {
						unread++;
						if (priority_sender) {
							vip_unread++;
						}
					}

					percentage = ((i+1) * 100) / total ;
					EM_DEBUG_LOG("Header Percentage Completed [%d] : [%d/%d]  mail_id [%d]", percentage, i+1,
                                                                                                        total, mail_id);
					SNPRINTF(mailbox_id_param_string, 10, "%d", input_mailbox_tbl->mailbox_id);
				}

				if (!emcore_notify_storage_event (NOTI_MAIL_ADD, account_id, mail_id, mailbox_id_param_string, thread_id))
					EM_DEBUG_EXCEPTION ("emcore_notify_storage_event [NOTI_MAIL_ADD] failed");

#ifdef __FEATURE_BLOCKING_MODE__
				/* Check the blocking mode */
				int blocking_mode = false;

				blocking_mode = emcore_get_blocking_mode_status();
				if (!blocking_mode) {
					if ((err = emcore_check_blocking_mode (multi_user_name, new_mail_tbl_data->email_address_sender, 
                                                                                &blocking_mode)) != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION ("emcore_check_blocking_mode failed : [%d]", err);
					}

					if (blocking_mode)
						emcore_set_blocking_mode_status (blocking_mode);
				}
#endif /* __FEATURE_BLOCKING_MODE__ */

				/* Set contact log */
				switch (input_mailbox_tbl->mailbox_type) {
				case EMAIL_MAILBOX_TYPE_INBOX :
					if ((err = emcore_set_received_contacts_log(multi_user_name, new_mail_tbl_data)) != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("emcore_set_received_contacts_log failed : [%d]", err);
					}
					break;
				case EMAIL_MAILBOX_TYPE_SENTBOX:
				case EMAIL_MAILBOX_TYPE_OUTBOX:
					if ((err = emcore_set_sent_contacts_log(multi_user_name, new_mail_tbl_data)) != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION("emcore_set_sent_contacts_log failed : [%d]", err);
					}
					break;
				default:
					EM_DEBUG_LOG("Mailbox type : [%d]", input_mailbox_tbl->mailbox_type);
					break;
				}

				/*  Release for envelope is not required and it may cause crash. Don't free the memory for envelope here. */
				/*  Envelope data will be freed by garbage collector in mail_close_full */

				if (cancellable)
					FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);
			}

CONTINUE_NEXT:
			if (new_mail_tbl_data){
				emstorage_free_mail(&new_mail_tbl_data, 1, NULL);
				new_mail_tbl_data = NULL;
			}
			uid_elem = uid_elem->next;
			i++;
			EM_PROFILE_END(emCoreSyncHeaderEachMail);
		}
		EM_PROFILE_END(emCoreSyncHeaderwhileloop);
	}

	ret = true;
FINISH_OFF:

	if (mail_count != NULL)
		*mail_count = total;

	if (unread_mail != NULL)
		*unread_mail = unread;

	if (vip_mail_count != NULL)
		*vip_mail_count = vip_total;

	if (vip_unread_mail != NULL)
		*vip_unread_mail = vip_unread;

	if (account_ref) {
		emcore_free_account(account_ref);
		EM_SAFE_FREE(account_ref);
	}

	if (!emcore_remove_overflowed_mails(multi_user_name, input_mailbox_tbl, &err_2))
		EM_DEBUG_EXCEPTION("emcore_remove_overflowed_mails failed - %d", err_2);

	if (ret && input_mailbox_tbl)
		emstorage_stamp_last_sync_time_of_mailbox(multi_user_name, input_mailbox_tbl->mailbox_id, 1);

#ifdef __FEATURE_SUPPORT_SYNC_STATE_ON_NOTI_BAR__
	if ((err_from_vconf = vconf_set_int(VCONFKEY_EMAIL_SYNC_STATE, 0)) != 0 ) {
		EM_DEBUG_EXCEPTION("vconf_set_int failed (sync state) [%d]", err_from_vconf);
	}
#endif /* __FEATURE_SUPPORT_SYNC_STATE_ON_NOTI_BAR__ */

	if (new_mail_tbl_data)
		emstorage_free_mail(&new_mail_tbl_data, 1, NULL);

	if (uid_list != NULL){
		emcore_free_uids(uid_list, NULL);
		/*  uid_list point to the same memory with input_mailbox_tbl->user_data.  */
		/*  input_mailbox_tbl->user_data should be set NULL if uid_list is freed */
		if (input_uid_list && *input_uid_list)
			*input_uid_list = NULL;
	}

	EM_SAFE_FREE(uid_range);

	if(destination_mailbox)
		emstorage_free_mailbox(&destination_mailbox, 1, NULL);

	if (rule  != NULL)
		emstorage_free_rule(&rule, rule_count, NULL);

	if (err_code)
		*err_code = err;

	EM_PROFILE_END(emCoreSyncHeader);
	EM_DEBUG_FUNC_END("ret [%d], err [%d]", ret, err);
	return ret;
}


emcore_uid_list *__ReverseList(emcore_uid_list *uid_list)
{
	emcore_uid_list *temp, *current, *result;

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



int emcore_download_uid_all(char *multi_user_name, MAILSTREAM *mail_stream, email_internal_mailbox_t *mailbox, emcore_uid_list** uid_list, int *total, emstorage_read_mail_uid_tbl_t *downloaded_uids, int count, emcore_get_uids_for_delete_t for_delete, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], uid_list[%p], total[%p], downloaded_uids[%p], count[%d], for_delete[%d], err_code[%p]", mailbox, uid_list, total, downloaded_uids, count, for_delete, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	MAILSTREAM *stream = NULL;
	email_account_t *ref_account = NULL;
	emcore_uid_list *uid_elem = NULL;
	emcore_uid_list *fetch_data_p = NULL;
	void *tmp_stream = NULL;
	char cmd[64] = {0x00, };
	char *p = NULL;

	if (!mailbox || !uid_list) {
		EM_DEBUG_EXCEPTION("mailbox[%p], uid_list[%p], total[%p], downloaded_uids[%p], count[%d], for_delete[%d]", mailbox, uid_list, total, downloaded_uids, count, for_delete);

		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(ref_account = emcore_get_account_reference(multi_user_name, mailbox->account_id))) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed - %d", mailbox->account_id);

		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!mail_stream) {
		if (!emcore_connect_to_remote_mailbox(multi_user_name, mailbox->account_id, mailbox->mailbox_id, (void **)&tmp_stream, &err)){
			EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed...");

			goto FINISH_OFF;
		}

		stream = (MAILSTREAM *)tmp_stream;
	}
	else
		stream = mail_stream;

	if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_POP3) {		/*  POP3 */
		POP3LOCAL *pop3local = NULL;

		if (!stream || !(pop3local = stream->local) || !pop3local->netstream) {
			err = EMAIL_ERROR_INVALID_PARAM;		/* EMAIL_ERROR_UNKNOWN; */
			goto FINISH_OFF;
		}

		/*  send UIDL */
		memset(cmd, 0x00, sizeof(cmd));

		SNPRINTF(cmd, sizeof(cmd), "UIDL\015\012");

#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG("[POP3] >>> [%s]", cmd);
#endif

		if (!net_sout(pop3local->netstream, cmd, (int)EM_SAFE_STRLEN(cmd))) {
			EM_DEBUG_EXCEPTION("net_sout failed...");

			err = EMAIL_ERROR_CONNECTION_BROKEN;
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
				err = EMAIL_ERROR_MAIL_NOT_FOUND;
				goto FINISH_OFF;
			}

			/*  replied success "+OK" */
			if (*p == '+') {
				EM_SAFE_FREE (p);
				continue;
			}

			/*  end of command */
			if (*p == '.')
				break;

			/*EM_DEBUG_LOG("UID list [%s]", p);*/

			uid_elem = (emcore_uid_list *)malloc(sizeof(emcore_uid_list));
			if (!uid_elem) {
				EM_DEBUG_EXCEPTION("malloc falied...");

				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			memset(uid_elem, 0x00, sizeof(emcore_uid_list));

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
					if (uid_elem->uid && !strcmp(uid_elem->uid, downloaded_uids[i].server_uid)) {
						break;
					}
				}
			}

			if (*uid_list) {
				fetch_data_p = *uid_list;

				while (fetch_data_p->next)
					fetch_data_p = fetch_data_p->next;

				fetch_data_p->next = uid_elem;
			}
			else
				*uid_list = uid_elem;

			if (total)
				++(*total);

			EM_SAFE_FREE (p);
		}
	}
	else {		/*  IMAP */
		IMAPLOCAL *imaplocal = NULL;
		char tag[16];
		char *s = NULL;
		char *t = NULL;

		if (!stream || !(imaplocal = stream->local) || !imaplocal->netstream) {
			err = EMAIL_ERROR_INVALID_PARAM;		/* EMAIL_ERROR_UNKNOWN; */
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
		if (!net_sout(imaplocal->netstream, cmd, (int)EM_SAFE_STRLEN(cmd))) {
			EM_DEBUG_EXCEPTION("net_sout failed...");

			err = EMAIL_ERROR_CONNECTION_BROKEN;
			goto FINISH_OFF;
		}

		/*  get uid from replied data */
		while (imaplocal->netstream) {
			if (!(p = net_getline(imaplocal->netstream)))
				break;

			/* EM_DEBUG_LOG(" [IMAP] <<< %s", p); */

			/*  tagged line - end of command */
			if (!strncmp(p, tag, EM_SAFE_STRLEN(tag)))
				break;

			/*  check that reply is reply to our command */
			/*  format  :  "* 9 FETCH (UID 68)" */
			if (!strstr(p, "FETCH (FLAGS")) {
				EM_SAFE_FREE (p);
				continue;
			}

			if (for_delete == EM_CORE_GET_UIDS_FOR_NO_DELETE) {
				if ((ref_account->retrieval_mode & EMAIL_IMAP4_RETRIEVAL_MODE_NEW) && (strstr(p, "\\Seen"))) {
					EM_SAFE_FREE (p);
					continue;
				}
			}

			uid_elem = (emcore_uid_list *)malloc(sizeof(emcore_uid_list));
			if (!uid_elem) {
				EM_DEBUG_EXCEPTION("malloc failed...");

				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			memset(uid_elem, 0x00, sizeof(emcore_uid_list));
			/*  parse uid, sequence, flags from replied data */

			/*  parse uid from replied data */
			s = p+2;
			t = strchr(s, ' ');

			if (!t) {
				err = EMAIL_ERROR_INVALID_RESPONSE;
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
					err = EMAIL_ERROR_INVALID_RESPONSE;
					goto FINISH_OFF;
				}

				*t = '\0';
			}
			else {
				err = EMAIL_ERROR_INVALID_RESPONSE;
				goto FINISH_OFF;
			}

			/*  save uid */
			uid_elem->uid = EM_SAFE_STRDUP(s);

			/*  check downloaded_uids */
			if (downloaded_uids) {
				int i;
				for (i = 0; i < count; ++i) {
					if (uid_elem->uid && !strcmp(uid_elem->uid, downloaded_uids[i].server_uid)) {
						EM_SAFE_FREE (uid_elem->uid);
						EM_SAFE_FREE (uid_elem);
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

			EM_SAFE_FREE (p);
		}
	}

	ret = true;

FINISH_OFF:
	if (uid_elem && ret == false)
		EM_SAFE_FREE (uid_elem);

	EM_SAFE_FREE (p);

	if (!mail_stream && stream) {
		stream = mail_close (stream);
	}

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

int emcore_download_imap_msgno(char *multi_user_name, email_internal_mailbox_t *mailbox, char *uid, int *msgno, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], uid[%p], msgno[%p], err_code[%p]", mailbox, uid, msgno, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	MAILSTREAM *stream = NULL;
	IMAPLOCAL *imaplocal = NULL;
	email_account_t *ref_account = NULL;
	void *tmp_stream = NULL;
	char tag[32], cmd[64];
	char *p = NULL;

	if (!mailbox || !uid) {
		EM_DEBUG_EXCEPTION("mailbox[%p], uid[%p], msgno[%p]", mailbox, uid, msgno);

		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(ref_account = emcore_get_account_reference(multi_user_name, mailbox->account_id))) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", mailbox->account_id);

		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (ref_account->incoming_server_type  != EMAIL_SERVER_TYPE_IMAP4) {
		err = EMAIL_ERROR_INVALID_ACCOUNT;		/* EMAIL_ERROR_INVALID_PARAM; */
		goto FINISH_OFF;
	}

	if (!mailbox->mail_stream) {
		if (!emcore_connect_to_remote_mailbox(multi_user_name, mailbox->account_id, mailbox->mailbox_id, (void **)&tmp_stream, &err)) {
			EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed - %d", err);

			goto FINISH_OFF;
		}

		stream = (MAILSTREAM *)tmp_stream;
	}
	else
		stream = mailbox->mail_stream;

	if (!stream || !(imaplocal = stream->local) || !imaplocal->netstream) {
		err = EMAIL_ERROR_INVALID_PARAM;		/* EMAIL_ERROR_UNKNOWN; */
		goto FINISH_OFF;
	}

	/*  send SEARCH UID */
	memset(tag, 0x00, sizeof(tag));
	memset(cmd, 0x00, sizeof(cmd));

	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
	SNPRINTF(cmd, sizeof(cmd), "%s SEARCH UID %s\015\012", tag, uid);

	if (!net_sout(imaplocal->netstream, cmd, (int)EM_SAFE_STRLEN(cmd))) {
		EM_DEBUG_EXCEPTION("net_sout failed...");

		err = EMAIL_ERROR_CONNECTION_BROKEN;
		goto FINISH_OFF;
	}

	/*  get message number from replied data */
	while (imaplocal->netstream) {
		if (!(p = net_getline(imaplocal->netstream)))
			break;

		/*  tagged line - end of command */
		if (!strncmp(p, tag, EM_SAFE_STRLEN(tag)))
			break;

		/*  check that reply is reply to our command */
		/*  format :  "* SEARCH 68", if not found, "* SEARCH" */
		if (!strstr(p, "SEARCH ") || (p[9] < '0' || p[9] > '9')) {
			EM_SAFE_FREE (p);
			continue;
		}

		if (msgno)
			*msgno = atoi(p+9);

		EM_SAFE_FREE (p);

		ret = true;
	}

	if (ret  != true)
		err = EMAIL_ERROR_MAIL_NOT_FOUND;

FINISH_OFF:
	EM_SAFE_FREE (p);

	if (mailbox && !mailbox->mail_stream)
		stream = mail_close (stream);


	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

int emcore_get_msgno(emcore_uid_list *uid_list, char *uid, int *msgno, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("uid_list[%p], uid[%s], msgno[%p], err_code[%p]", uid_list, uid, msgno, err_code);

	int ret = false;
	int err = EMAIL_ERROR_MAIL_NOT_FOUND;		/* EMAIL_ERROR_NONE; */

	if (!uid || !msgno || !uid_list) {
		EM_DEBUG_EXCEPTION("uid_list[%p], uid[%p], msgno[%p]", uid_list, uid, msgno);

		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG(" >> uid[%s]", uid);

	while (uid_list) {
		if (!strcmp(uid_list->uid, uid)) {
			*msgno = uid_list->msgno;

			EM_DEBUG_LOG("*msgno[%d]", *msgno);

			err = EMAIL_ERROR_NONE;

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

int emcore_get_uid(emcore_uid_list *uid_list, int msgno, char **uid, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false, err = EMAIL_ERROR_NONE;

	if (!uid || msgno <= 0 || !uid_list){
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	while (uid_list) {
		if (uid_list->msgno == msgno) {
			if (uid) {
				if (!(*uid = EM_SAFE_STRDUP(uid_list->uid))) {
					EM_DEBUG_EXCEPTION("strdup failed...");
					err = EMAIL_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}

				ret = true;
				break;
			}
		}

		uid_list = uid_list->next;
	}

	if (ret  != true)
		err = EMAIL_ERROR_MAIL_NOT_FOUND;

FINISH_OFF:
	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

int emcore_free_uids(emcore_uid_list *uid_list, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("uid_list[%p], err_code[%p]", uid_list, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	emcore_uid_list *p = NULL;

	if (!uid_list) {
		EM_DEBUG_EXCEPTION(" uid_list[%p]\n", uid_list);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	while (uid_list) {
		p = uid_list;
		uid_list = uid_list->next;
		EM_SAFE_FREE (p->uid);
		EM_SAFE_FREE (p->internaldate);
		EM_SAFE_FREE (p);
	}

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC char *emcore_guess_charset(char *multi_user_name, char *source_string)
{
	EM_DEBUG_FUNC_BEGIN("source - %s", source_string);

	UErrorCode err = U_ZERO_ERROR;
	UCharsetDetector* detector = NULL;
	const UCharsetMatch** match = NULL;
	int count = 0;
	int i = 0;
	int confidence = 0;
	int most_confidence = 0;
	char *temp_charset = NULL;
	char *uscdet_result_charset = NULL;
	const char *charset = NULL;
	const char *detected_charset = NULL;

	char *ret = NULL;
	email_account_t *account_ref = NULL;
	SIZEDTEXT source_text;
	CHARSET *result_charset = NULL;
	source_text.data = (unsigned char*)source_string;
	source_text.size = EM_SAFE_STRLEN(source_string);
	result_charset   = (CHARSET*)utf8_infercharset(&source_text);

	if(result_charset) {
		return EM_SAFE_STRDUP(result_charset->name);
	}

	time_t t = time(0);
	struct tm *data = localtime(&t);
	if (data == NULL) {
		EM_DEBUG_EXCEPTION("localtime failed");
		return NULL;
	}

	for (i = 0 ; i < G_N_ELEMENTS(known_zone_charset); i++) {
		if (!g_ascii_strcasecmp(known_zone_charset[i].tm_zone, data->tm_zone)) {
			temp_charset = strdup(known_zone_charset[i].charset);
			break;
		}
	}

	detector = ucsdet_open(&err);
	if(U_FAILURE(err))
		EM_DEBUG_LOG("ucsdet_open failed");

	ucsdet_setText(detector, source_string, EM_SAFE_STRLEN(source_string), &err);
	if(U_FAILURE(err))
		EM_DEBUG_LOG("ucsdet_setText failed");

	match = ucsdet_detectAll(detector, &count, &err);
	if(U_FAILURE(err))
		EM_DEBUG_LOG("ucsdet_detectAll failed");

	for (i = 0 ; i < count ; i++) {
		confidence = ucsdet_getConfidence(match[i], &err);
		if(U_FAILURE(err))
			EM_DEBUG_LOG("ucsdet_getConfidence failed");

		charset = ucsdet_getName(match[i], &err);
		if(U_FAILURE(err))
			EM_DEBUG_LOG("ucsdet_getName failed");

		if (most_confidence < confidence) {
			most_confidence = confidence;
			detected_charset = charset;
		} else if (most_confidence == confidence) {
			if ((temp_charset && charset) && !strcasecmp(charset, temp_charset))
				detected_charset = charset;
		}

		EM_DEBUG_LOG_DEV("UCSDET DETECTED CHARSET:%s, %d", ucsdet_getName(match[i], &err), confidence);
	}

	if (detected_charset)
		uscdet_result_charset = EM_SAFE_STRDUP(detected_charset);

	ucsdet_close(detector);

	EM_SAFE_FREE(temp_charset);

	if (uscdet_result_charset)
		return uscdet_result_charset;

	account_ref = emcore_get_account_reference(multi_user_name, g_current_sync_account_id);
	if (!account_ref) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed - %d",
				g_current_sync_account_id);
		return NULL;
	}

	if (g_str_has_suffix(account_ref->user_email_address, ".ru")) {
		ret = EM_SAFE_STRDUP("windows-1251");
	}

	if (account_ref) {
		emcore_free_account(account_ref);
		EM_SAFE_FREE(account_ref);
	}

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

INTERNAL_FUNC int emcore_sync_mail_from_client_to_server(char *multi_user_name, int mail_id)
{
	EM_DEBUG_FUNC_BEGIN("mail_id [%d]", mail_id);

	int                         err                  = EMAIL_ERROR_NONE;
	int                         len                  = 0;
	int                         read_size            = 0;
	int                         attachment_tbl_count = 0;
	char                       *fname                = NULL;
	char                       *long_enc_path        = NULL;
	char                       *data                 = NULL;
	char                        set_flags[100]       = {0,};
	char                        message_size[100]    = {0,};
	ENVELOPE                   *envelope             = NULL;
	FILE                       *fp                   = NULL;
 	STRING                     str;
 	STRING                     str_data;
	MAILSTREAM                 *stream               = NULL;
	email_account_t            *account_ref          = NULL;
	emstorage_mail_tbl_t       *mail_table_data      = NULL;
	emstorage_attachment_tbl_t *attachment_tbl_data  = NULL;
	emstorage_mailbox_tbl_t    *mailbox_tbl          = NULL;
	int max_alloc_size = 40960; /*40K*/
	int alloc_size = 0;

	/*  get a mail from mail table */
	if (!emstorage_get_mail_by_id(multi_user_name, mail_id, &mail_table_data, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	account_ref = emcore_get_account_reference(multi_user_name, mail_table_data->account_id);

	if(account_ref == NULL || account_ref->incoming_server_type != EMAIL_SERVER_TYPE_IMAP4) {
		EM_DEBUG_EXCEPTION("This account doesn't support sync");
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_attachment_list(multi_user_name, mail_id, false, &attachment_tbl_data, &attachment_tbl_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("mailbox_id [%d]", mail_table_data->mailbox_id);

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, mail_table_data->mailbox_id, &mailbox_tbl)) != EMAIL_ERROR_NONE || !mailbox_tbl){
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_name failed [%d]", err);
		goto FINISH_OFF;
	}

	if (mailbox_tbl->local_yn) {
		EM_DEBUG_EXCEPTION_SEC("The mailbox [%s] is not on server.", mailbox_tbl->mailbox_name);
		err = EMAIL_ERROR_INVALID_MAILBOX;
		goto FINISH_OFF;
	}

	if (!emcore_get_long_encoded_path(multi_user_name, mailbox_tbl->account_id, mailbox_tbl->mailbox_name, '/', &long_enc_path, &err)) {
		EM_DEBUG_EXCEPTION(">>emcore_get_long_encoded_path  :  Failed [%d] ", err);
		goto FINISH_OFF;
	}

	if (!long_enc_path) {
	    EM_DEBUG_EXCEPTION(">>long_enc_path  :  NULL ");
	    goto FINISH_OFF;
	}

	if (!emcore_make_rfc822_file_from_mail(multi_user_name, mail_table_data, attachment_tbl_data, attachment_tbl_count, &envelope, &fname, NULL, &err)){
	    EM_DEBUG_EXCEPTION(" emcore_make_rfc822_file_from_mail failed [%d]", err);
	    goto FINISH_OFF;
	}

	if (fname) {
	    err = em_fopen(fname, "a+", &fp);
	    if (err != EMAIL_ERROR_NONE) {
	        EM_DEBUG_EXCEPTION("em_fopen failed - %s", fname);
	        goto FINISH_OFF;
	    }
	}

	if (!fp) {
	    EM_DEBUG_EXCEPTION("fp is NULL..!");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
	    goto FINISH_OFF;
	}

	rewind(fp);

	/* get file length */
	if(fseek(fp, 0, SEEK_END) != 0 || (len = ftell(fp)) == -1) {
		EM_DEBUG_EXCEPTION("fseek or ftell failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if (fname)
		EM_DEBUG_LOG_SEC("Composed file name [%s] and file size [%d]", fname, len);

	rewind(fp);

	if (!emcore_connect_to_remote_mailbox(multi_user_name, mail_table_data->account_id, 0, (void **)&stream, &err)){
	    EM_DEBUG_EXCEPTION("emcore_move_mail_on_server failed :  Mailbox open[%d]", err);
	    goto FINISH_OFF;
	}

	/* added for copying server UID to DB */
	mail_parameters(stream, SET_APPENDUID, mail_appenduid);

	if (len < max_alloc_size)
		alloc_size = len;
	else
		alloc_size = max_alloc_size;

	data = (char *)malloc(alloc_size + 1);


	snprintf(message_size, sizeof(message_size), "%d", len);
	INIT(&str, mail_string, message_size, EM_SAFE_STRLEN(message_size));

	sprintf(set_flags, "\\Seen");

	int total_size = len;
	int data_size = 0;
	int sent_size = 0;

	if (mail_table_data->flags_seen_field){
		if (!mail_append_command(stream, long_enc_path, set_flags, NULL, &str)) {
			EM_DEBUG_EXCEPTION("mail_append  failed -");
			err = EMAIL_ERROR_IMAP4_APPEND_FAILURE;
			goto FINISH_OFF;
		}

		while (total_size > 0) {
			if (total_size < max_alloc_size)
				data_size = total_size;
			else
				data_size = max_alloc_size;

			memset(data, 0x0, data_size+1);
			read_size = fread(data, sizeof(char), data_size, fp);

			if (read_size != data_size) {
				/* read fail. */
				EM_SAFE_FREE(data);
				EM_DEBUG_EXCEPTION("Read from file failed");
			}
			sent_size += read_size;

			INIT(&str_data, mail_string, data, read_size);
			if (!mail_append_message(stream, long_enc_path, &str_data)) {
				EM_DEBUG_EXCEPTION("mail_append  failed -");
				err = EMAIL_ERROR_IMAP4_APPEND_FAILURE;
				goto FINISH_OFF;
			} else {
				EM_DEBUG_LOG("Sent data Successfully. sent[%d] total[%d]", sent_size, total_size);
			}
			total_size -= data_size;
		}
		EM_SAFE_FREE(data);

		if (!mail_append_end(stream, long_enc_path)) {
			EM_DEBUG_EXCEPTION("mail_append  failed -");
			err = EMAIL_ERROR_IMAP4_APPEND_FAILURE;
			goto FINISH_OFF;
		}
	}
	else{
		if (!mail_append_command(stream, long_enc_path, NULL, NULL, &str)) {
			EM_DEBUG_EXCEPTION("mail_append  failed -");
			err = EMAIL_ERROR_IMAP4_APPEND_FAILURE;
			goto FINISH_OFF;
		}

		while (total_size > 0) {
			if (total_size < max_alloc_size)
				data_size = total_size;
			else
				data_size = max_alloc_size;

			memset(data, 0x0, data_size+1);
			read_size = fread(data, sizeof(char), data_size, fp);

			if (read_size != data_size) {
				/* read fail. */
				EM_SAFE_FREE(data);
				EM_DEBUG_EXCEPTION("Read from file failed");
			}
			sent_size += read_size;

			INIT(&str_data, mail_string, data, read_size);
			if (!mail_append_message(stream, long_enc_path, &str_data)) {
				EM_DEBUG_EXCEPTION("mail_append  failed -");
				err = EMAIL_ERROR_IMAP4_APPEND_FAILURE;
				goto FINISH_OFF;
			} else {
				EM_DEBUG_LOG_DEV("Sent data Successfully. sent[%d] total[%d]", sent_size, len);
			}
			total_size -= data_size;
		}
		EM_SAFE_FREE(data);

		if (!mail_append_end(stream, long_enc_path)) {
			EM_DEBUG_EXCEPTION("mail_append  failed -");
			err = EMAIL_ERROR_IMAP4_APPEND_FAILURE;
			goto FINISH_OFF;
		}
	}

	/* Update read_mail_uid tbl */
	if (!emcore_add_read_mail_uid(multi_user_name, mailbox_tbl, mailbox_tbl->mailbox_name, mail_table_data->mail_id, g_append_uid_rsp, mail_table_data->mail_size, 0, &err)) {
		EM_DEBUG_EXCEPTION(" emcore_add_read_mail_uid failed [%d]", err);
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
		goto FINISH_OFF;
	}

	/* Update mail_data tbl */
	if (!emstorage_update_server_uid(multi_user_name, mail_table_data->server_mail_id, g_append_uid_rsp, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_update_server_uid failed : [%d]", err);
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
		goto FINISH_OFF;
	}

FINISH_OFF:

#ifdef __FEATURE_LOCAL_ACTIVITY__
	if (ret){
		emstorage_activity_tbl_t new_activity;
		memset(&new_activity, 0x00, sizeof(emstorage_activity_tbl_t));
		new_activity.activity_type = ACTIVITY_SAVEMAIL;
		new_activity.account_id    = account_id;
		new_activity.mail_id	   = mail_id;
		new_activity.dest_mbox	   = NULL;
		new_activity.server_mailid = NULL;
		new_activity.src_mbox	   = NULL;

		if (!emcore_delete_activity(&new_activity, &err)){
			EM_DEBUG_EXCEPTION(">>>>>>Local Activity [ACTIVITY_SAVEMAIL] [%d] ", err);
		}
	}
#endif /* __FEATURE_LOCAL_ACTIVITY__ */

	EM_SAFE_FREE(data);
	EM_SAFE_FREE(long_enc_path);

	if (account_ref) {
		emcore_free_account(account_ref);
		EM_SAFE_FREE(account_ref);
	}

	if (fp)
		fclose(fp);

	if (envelope)
		mail_free_envelope(&envelope);

	if (mail_table_data)
		emstorage_free_mail(&mail_table_data, 1, NULL);

	if (attachment_tbl_data)
		emstorage_free_attachment(&attachment_tbl_data, attachment_tbl_count, NULL);

	if (mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

	if (fname) {
		remove(fname);
		EM_SAFE_FREE(fname);
	}

	if (stream)
		stream = mail_close (stream);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

#endif

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

static int emcore_initiate_pbd(char *multi_user_name, MAILSTREAM *stream, int account_id, int mail_id, char *uid, int input_maibox_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mail_id[%d], uid[%p], input_maibox_id[%d]", account_id, mail_id, uid, input_maibox_id);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_account_t *account_ref = NULL;
	emstorage_mailbox_tbl_t* mailbox = NULL;

	if (account_id < FIRST_ACCOUNT_ID || mail_id < 0 || NULL == uid || 0 == input_maibox_id){
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	account_ref = emcore_get_account_reference(multi_user_name, account_id);

	email_event_partial_body_thd pbd_event;

	memset(&pbd_event, 0x00, sizeof(email_event_partial_body_thd));

	pbd_event.account_id = account_id;
	if (account_ref && account_ref->incoming_server_type == EMAIL_SERVER_TYPE_POP3)
		pbd_event.activity_type = ACTIVITY_PARTIAL_BODY_DOWNLOAD_POP3_WAIT;
	else
		pbd_event.activity_type = ACTIVITY_PARTIAL_BODY_DOWNLOAD_IMAP4;

	if ( (err = emstorage_get_mailbox_by_id(multi_user_name, input_maibox_id, &mailbox)) != EMAIL_ERROR_NONE || !mailbox) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	pbd_event.mailbox_id      = input_maibox_id;
	pbd_event.mail_id         = mail_id;
	pbd_event.server_mail_id  = strtoul(uid, NULL, 0);
	pbd_event.mailbox_name    = EM_SAFE_STRDUP(mailbox->mailbox_name);
	pbd_event.multi_user_name = EM_SAFE_STRDUP(multi_user_name);

	EM_DEBUG_LOG("input_mailbox_name name [%d]", pbd_event.mailbox_id);
	EM_DEBUG_LOG("uid [%s]", uid);
	EM_DEBUG_LOG("pbd_event.account_id[%d], pbd_event.mail_id[%d], pbd_event.server_mail_id [%d]", pbd_event.account_id, pbd_event.mail_id , pbd_event.server_mail_id);

	if (!emcore_insert_pbd_activity(&pbd_event, &pbd_event.activity_id, &err)){
		EM_DEBUG_EXCEPTION("Inserting Partial Body Download activity failed with error[%d]", err);
		goto FINISH_OFF;
	}
	else{
		if (false == emcore_is_partial_body_thd_que_full()){
			/* h.gahaut :  Before inserting the event into event queue activity_type should be made 0
			Because partial body thread differentiates events coming from DB and event queue
			on the basis of activity_type and event_type fields */

			pbd_event.activity_type = 0;
			pbd_event.event_type = EMAIL_EVENT_BULK_PARTIAL_BODY_DOWNLOAD;

			if (!emcore_insert_partial_body_thread_event(&pbd_event, &err)){
				EM_DEBUG_EXCEPTION("Inserting Partial body thread event failed with error[%d]", err);
				goto FINISH_OFF;
			}

			/*h.gahlaut :  Partial body thread has created a copy of event for itself so this local event should be freed here*/
			if (!emcore_free_partial_body_thd_event(&pbd_event, &err))
				EM_DEBUG_EXCEPTION("Freeing Partial body thread event failed with error[%d]", err);
		}
		else{
			EM_DEBUG_LOG(" Activity inserted only in DB .. Queue is Full");
		}
	}

	ret = true;

FINISH_OFF:

	if (mailbox) {
		emstorage_free_mailbox(&mailbox, 1, &err);
	}

	if (account_ref) {
		emcore_free_account(account_ref);
		EM_SAFE_FREE(account_ref);
	}

	emcore_free_partial_body_thd_event(&pbd_event, NULL);

	if (NULL  != err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

#ifndef __FEATURE_USE_GMIME__
static int emcore_parse_bodystructure(void *stream, IMAPPARSEDREPLY *reply_from_server, char *bodystructure, BODY **body, struct _m_content_info **cnt_info, int *total_mail_size)
{
	EM_DEBUG_FUNC_BEGIN("stream:[%p], reply_from_server:[%p], bodystructure:[%p]", stream, reply_from_server, bodystructure);

	int err = EMAIL_ERROR_NONE;
	char* ptr = NULL;

	if (!stream || !reply_from_server || !bodystructure || !cnt_info || !body || !total_mail_size) {
		EM_DEBUG_EXCEPTION("Invalid paramter stream[%p] reply_from_server[%p] bodystructure[%p] cnt_info[%p] body[%p] total_mail_size[%p]",
					stream, reply_from_server, bodystructure, cnt_info, body, total_mail_size);
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	int p_total_mail_size = 0;
	char *bodystructure_start = NULL;
	char *bodystructure_string = NULL;
	BODY *p_body = NULL;
	struct _m_content_info *p_cnt_info = NULL;

	/* Get the body strcuture string */
	bodystructure_start = strstr(bodystructure, "BODYSTRUCTURE (");
	if (!bodystructure_start) {
		EM_DEBUG_EXCEPTION("Invalid bodystructure :[%s]", bodystructure);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	bodystructure_start = bodystructure_start + strlen("BODYSTRUCTURE");
	ptr = bodystructure_string = strdup(bodystructure_start);

	EM_DEBUG_LOG_DEV("BODYSTRUCTURE:%s", bodystructure_string);

	/* Parse the bodystructure string */
	p_body = mail_newbody();
	if (p_body == NULL) {
		EM_DEBUG_EXCEPTION("New body creationg failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	/* valgrind : free body->sparep */
	mail_parameters(stream, SET_FREEBODYSPAREP, emcore_free_body_sparep);

	/*imap_parse_body_structure changes param pointer*/
	imap_parse_body_structure(stream, p_body, (unsigned char **)&bodystructure_string, reply_from_server);

	/* Get the total mail size */
	if (emcore_set_fetch_body_section(p_body, true, &p_total_mail_size, NULL, &err) < 0) {
		EM_DEBUG_EXCEPTION("emcore_set_fetch_body_section failed:[%d]", err);
		goto FINISH_OFF;
	}

	/* Fill the content_info structure */
	if (!(p_cnt_info = em_malloc(sizeof(struct _m_content_info)))) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (emcore_get_body(stream, 0, 0, 0, p_body, NULL, p_cnt_info, &err) < 0 || !p_cnt_info) {
		EM_DEBUG_EXCEPTION("emcore_get_body failed[%d]", err);
		err = EMAIL_ERROR_IMAP4_FETCH_UID_FAILURE;
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_SAFE_FREE(ptr);

	if (err != EMAIL_ERROR_NONE) {
		if (p_cnt_info) {
			emcore_free_content_info(p_cnt_info);
			EM_SAFE_FREE(p_cnt_info);
		}
		if (p_body)
			mail_free_body(&p_body);
	} else {
		*cnt_info = p_cnt_info;
		*body = p_body;
		*total_mail_size = p_total_mail_size;
	}

	EM_DEBUG_FUNC_END("Err:[%d]", err);
	return err;
}
#endif

INTERNAL_FUNC int emcore_update_attachment_except_inline(char *multi_user_name, 
																	struct _m_content_info *cnt_info, 
																	int account_id, 
																	int mail_id, 
																	int mailbox_id, 
																	int *output_total_attachment_size, 
																	int *output_attachment_count, 
																	int *output_inline_attachment_count)
{
	EM_DEBUG_FUNC_BEGIN("cnt_info : [%p], account_id : [%d], mail_id : [%d], mailbox_id : [%d]", cnt_info, account_id, mail_id, mailbox_id);
	int err = EMAIL_ERROR_NONE;

	if (!cnt_info || !account_id || !mail_id || !mailbox_id) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;

	}

	int attachment_count = 0;
	int total_attach_size = 0;
	int inline_attachment_count = 0;
	emstorage_attachment_tbl_t attachment_tbl = {0};
	struct attachment_info *attach_info = NULL;

	attachment_tbl.account_id = account_id;
	attachment_tbl.mail_id = mail_id;
	attachment_tbl.mailbox_id = mailbox_id;
	attachment_tbl.attachment_save_status = 0;

#ifdef __FEATURE_USE_GMIME__
	for (inline_attachment_count = 0, attach_info = cnt_info->inline_file; attach_info; attach_info = attach_info->next) {
		if (attach_info->type == INLINE_ATTACHMENT) {
			inline_attachment_count++;
		}
	}
#endif

	for (attachment_count = 0, attach_info = cnt_info->file; attach_info; attach_info = attach_info->next, attachment_count++) {

#ifndef __FEATURE_USE_GMIME__
		if (attach_info->type == INLINE_ATTACHMENT) {
			EM_DEBUG_LOG("INLINE ATTACHMENT");
			inline_attachment_count++;
			continue;
		}
#endif
		total_attach_size                              += attach_info->size;
		attachment_tbl.attachment_size                  = attach_info->size;
		attachment_tbl.attachment_path                  = attach_info->save;
		attachment_tbl.attachment_name                  = attach_info->name;
		attachment_tbl.content_id                       = attach_info->content_id;
		attachment_tbl.attachment_drm_type              = attach_info->drm;
		attachment_tbl.attachment_mime_type             = attach_info->attachment_mime_type;
#ifdef __ATTACHMENT_OPTI__
		attachment_tbl.encoding                         = attach_info->encoding;
		attachment_tbl.section                          = attach_info->section;
#endif
		if (!emstorage_add_attachment(multi_user_name, &attachment_tbl, 0, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_add_attachment failed : [%d]", err);
			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG("attachment_count : [%d], inline_attachment_count : [%d]", attachment_count, inline_attachment_count);


FINISH_OFF:

	if (output_attachment_count)
#ifdef __FEATURE_USE_GMIME__
		*output_attachment_count = attachment_count;
#else
		*output_attachment_count = attachment_count - inline_attachment_count;
#endif

	if (output_inline_attachment_count)
		*output_inline_attachment_count = inline_attachment_count;

	if (output_total_attachment_size)
		*output_total_attachment_size = total_attach_size;

	EM_DEBUG_FUNC_END("err : [%d]", err);
	return err;
}

#define UID_RANGE_STRING_LENGTH          3000
#define MULTIPART_BOUNDARY_LENGTH        1024
#define TEMP_STRING_LENGTH		         50
#define CONTENT_TRANSFER_ENCODING_STRING "Content-Transfer-Encoding"

#ifndef __FEATURE_USE_GMIME__
static int emcore_parse_html_part_for_partial_body(char *start_header, char *boundary_string, char *bufsendforparse, char *text_html, int body_size)
{
	EM_DEBUG_FUNC_BEGIN("start_header [%p], boundary_string [%s], bufsendforparse [%s], text_html [%s], body_size [%d]", start_header, boundary_string, bufsendforparse, text_html, body_size);

	int   err = EMAIL_ERROR_NONE;
	int   iEncodingHeader = 0;
	int   enc_type = ENCOTHER, dec_len = 0;
	char  EncodingHeader[40] = {0};
	char  Encoding[30] = {0};
	char *pEncodingHeaderEnd = NULL;
	char *txt_html = NULL;
	char *pHeaderStart = NULL;
	char *start = NULL, *end = NULL;
	char *temp_enc1 = NULL;

	EM_DEBUG_LOG("Content-Type : text/html or message/rfc822 or text/rfc822-headers");

	if(start_header == NULL) { /*prevent 27448*/
		EM_DEBUG_EXCEPTION("start_header NULL");
		return false;
	}

	pHeaderStart = start_header;
	pHeaderStart = pHeaderStart-2;
	do{
		pHeaderStart--;
	} while (*pHeaderStart != LF && bufsendforparse < pHeaderStart);

	pHeaderStart++;

	/* Check string size */
	if (EM_SAFE_STRLEN(pHeaderStart) < strlen(CONTENT_TRANSFER_ENCODING_STRING)) {
		EM_DEBUG_EXCEPTION("pHeaderStart is truncated.");
		return false;
	}

	memcpy(EncodingHeader, pHeaderStart, strlen(CONTENT_TRANSFER_ENCODING_STRING));

	if (strcasecmp(EncodingHeader, CONTENT_TRANSFER_ENCODING_STRING) == 0){
		pEncodingHeaderEnd = strstr(pHeaderStart, CRLF_STRING);

		if ((pEncodingHeaderEnd - (pHeaderStart+27)) > 30) {
			EM_DEBUG_EXCEPTION("Encoding is too long");
			return false;
		}
		memcpy(Encoding, pHeaderStart + 27, pEncodingHeaderEnd - (pHeaderStart+27));
		iEncodingHeader = 1;
	}

	/* HTML Content found */
	txt_html = start_header;
	txt_html = strstr(txt_html, CRLF_STRING CRLF_STRING);

	if (txt_html != NULL){
		txt_html += 4; /*  txt_html points at html content */
		start = txt_html;
		char multipart_boundary[MULTIPART_BOUNDARY_LENGTH] = {0};
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

		if ((multipart_related_boundry_end - multipart_related_boundry) > MULTIPART_BOUNDARY_LENGTH)	{
			EM_DEBUG_EXCEPTION("Encoding is too long");
			return false;
		}

		memcpy(multipart_boundary, multipart_related_boundry, multipart_related_boundry_end - multipart_related_boundry);

		if (strcmp(multipart_boundary, boundary_string) == 0)
			end = strstr(txt_html, boundary_string);
		else
			end = strstr(txt_html, multipart_boundary);

		memset(multipart_boundary, 0, EM_SAFE_STRLEN(multipart_boundary));

		if (end == NULL) {
			EM_DEBUG_LOG("HTML body contents exceeds limited Bytes");
			end = txt_html + body_size - (txt_html - bufsendforparse);
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
		EM_DEBUG_LOG("iEncodingHeader [%d]", iEncodingHeader);

		if (iEncodingHeader == 1){
			enc_type = ENCOTHER;
			if (strncasecmp(Encoding, "base64", strlen("base64")) == 0)
				enc_type = ENCBASE64;
			else if (strncasecmp(Encoding, "quoted-printable", strlen("quoted-printable")) == 0)
				enc_type = ENCQUOTEDPRINTABLE;

			EM_DEBUG_LOG("enc_type [%d]", enc_type);

			memcpy(text_html, start, end - txt_html);

			if (emcore_decode_body_text(text_html, end - txt_html, enc_type , &dec_len, &err) < 0)
				EM_DEBUG_EXCEPTION("emcore_decode_body_text failed [%d]", err);
		}
		else if ((temp_enc1 = (char *)strcasestr(start_header, "Content-transfer-encoding:")) && (temp_enc1 < end)){ /*prevent 27448*/
			if (temp_enc1)
				start_header = temp_enc1;

			start_header += strlen(CONTENT_TRANSFER_ENCODING_STRING);
			start_header = em_skip_whitespace_without_strdup(start_header);

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

				if (emcore_decode_body_text(text_html, end - txt_html, enc_type , &dec_len, &err) < 0)
					EM_DEBUG_EXCEPTION("emcore_decode_body_text failed [%d]", err);
			}

			EM_DEBUG_LOG("Decoded length = %d", dec_len);
		}
		else{
			memcpy(text_html, start, end-txt_html);
		}

		/* EM_DEBUG_LOG(" Content-Type:  text/html [%s]\n", text_html);								 */
	}
	else
		EM_DEBUG_EXCEPTION(" Invalid html body content ");

	EM_DEBUG_FUNC_END();
	return true;
}



/*For the following scenario
*------= SAMSUNG_mySingle_MIME_MULTIPART_BOUNDARY
*Content-Transfer-Encoding :  base64
*Content-Type :  text/plain; charset = "windows-1252"
*MIME-Version :  1.0
*Message-ID:  <11512468.945901271910226702.JavaMail.weblogic@epml03>
*/

#define CONTENT_TRANSFER_ENCODING "Content-Transfer-Encoding"

static int emcore_parse_plain_part_for_partial_body(char *header_start_string, char *start_header, char *boundary_string, char *bufsendforparse, char *text_plain, int body_size)
{
	EM_DEBUG_FUNC_BEGIN("header_start_string[%p], start_header[%p], boundary_string [%p], bufsendforparse [%p], text_plain [%p]", header_start_string, start_header, boundary_string, bufsendforparse, text_plain);
	int   err = EMAIL_ERROR_NONE, iEncodingHeader = 0, enc_type = ENCOTHER;
	int  dec_len = 0, strcmpret = -1;
	char *pHeaderStart = NULL, *pEncodingHeaderEnd = NULL;
	char  EncodingHeader[40] = {0, };
	char  Encoding[30] = {0, };
	char *start = NULL, *end = NULL, *txt_plain = NULL, *temp_enc1 = NULL;

	EM_DEBUG_LOG("Content-Type : text/plain");

	pHeaderStart = header_start_string;

	memcpy(EncodingHeader, pHeaderStart, 25);

	if (strcasecmp(EncodingHeader, CONTENT_TRANSFER_ENCODING_STRING) == 0){
		pEncodingHeaderEnd = strstr(pHeaderStart, CRLF_STRING);
		memcpy(Encoding, pHeaderStart + 27, pEncodingHeaderEnd - (pHeaderStart + 27));
		iEncodingHeader = 1;
	}

	/*  Plain text content found  */
	if(!start_header) { /*prevent 27450*/
		EM_DEBUG_EXCEPTION("start_header NULL");
		return false;
	}
	txt_plain = start_header;
	txt_plain = strstr(txt_plain, CRLF_STRING CRLF_STRING);

	if (txt_plain != NULL){
		txt_plain += EM_SAFE_STRLEN(CRLF_STRING CRLF_STRING); /*  txt_plain points at plain text content  */

		/* Fix is done for mail having "Content-Type: text/plain" but there is no content but having only attachment. */

		strcmpret = strncmp(txt_plain, boundary_string, EM_SAFE_STRLEN(boundary_string));
		if (strcmpret == 0){
		}
		else{
			start = txt_plain;
			end = strstr(txt_plain, boundary_string);

			if (end == NULL){
				EM_DEBUG_LOG("Text body contents exceeds limited Bytes");
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

				if (emcore_decode_body_text(text_plain, end - txt_plain, enc_type , &dec_len, &err) < 0)
					EM_DEBUG_EXCEPTION("emcore_decode_body_text failed [%d]", err);
			}
			else if (start_header && ((temp_enc1 = (char *)strcasestr(start_header, "Content-transfer-encoding:"))  != NULL) && !(temp_enc1 && temp_enc1 >= end)){
				if (temp_enc1)
					start_header = temp_enc1;

				start_header += strlen(CONTENT_TRANSFER_ENCODING_STRING);
				start_header = em_skip_whitespace_without_strdup(start_header);

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
					if (emcore_decode_body_text(text_plain, end - txt_plain, enc_type , &dec_len, &err) < 0)
						EM_DEBUG_EXCEPTION("emcore_decode_body_text failed [%d]", err);
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



/*  Content-Type:  IMAGE/octet-stream; name = Default.png */
/*  Content-Transfer-Encoding:  BASE64 */
/*  Content-ID:  <4b0d6810b17291f9438783a8eb9d5228@org.tizen.email> */
/*  Content-Disposition:  inline; filename = Default.png */

static void emcore_free_email_image_data(email_image_data **image_data, int count)
{
	EM_DEBUG_FUNC_BEGIN("image_data : [%p], count : [%d]", image_data, count);

	if ((image_data == NULL) || (*image_data == NULL) || (count <= 0)) {
		EM_DEBUG_EXCEPTION("Invalid paramter");
		return;
	}

	email_image_data *p = *image_data;
	int i = 0;

	for (i = 0; i < count; i++) {
		EM_SAFE_FREE(p[i].image_file_name);
		EM_SAFE_FREE(p[i].text_image);
		EM_SAFE_FREE(p[i].content_id);
		EM_SAFE_FREE(p[i].mime_type);
	}

	EM_SAFE_FREE(p);
	*image_data = NULL;

	EM_DEBUG_FUNC_END();
}

static int emcore_parse_image_part_for_partial_body(char *header_start_string, char *start_header, char *boundary_string, char *bufsendforparse, email_image_data *image_data, int body_size)
{
	EM_DEBUG_FUNC_BEGIN("boundary_string : [%s]", boundary_string);

	int   err = EMAIL_ERROR_NONE;
	char *multiple_image = NULL;
	int   donot_parse_next_image = 0;
	char *image_boundary = NULL;
	char *image_boundary_end = NULL;
	char *temp_image_boundary = NULL;
	int   i = 0, ch_image = 0;
	int   enc_type = ENCOTHER;
	char *p = header_start_string;
	char *start = NULL, *end = NULL, *txt_image = NULL;
	char *temp_image = NULL;
	char *temp_cid1 = NULL;
	char *cid_end = NULL;
	char *temp_enc1 = NULL;
	char *p_boundary_string = NULL;

	if(image_data == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	image_boundary     = start_header;
	image_boundary_end = image_boundary - 2;
	image_boundary     = image_boundary - 2;

	EM_DEBUG_LOG("Content-type: image");

	while (bufsendforparse < image_boundary && image_boundary && *image_boundary != LF)
		image_boundary--;

	image_boundary++;

	temp_image_boundary = em_malloc((image_boundary_end - image_boundary) + 1);
	if (temp_image_boundary == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		return false;
	}

	if (image_boundary  != NULL && image_boundary_end != NULL)
		memcpy(temp_image_boundary, image_boundary, image_boundary_end-image_boundary);


	if (((char *)strcasestr((const char *)temp_image_boundary, "Content-type:") == NULL) && (temp_image_boundary[0] == '-'))
		p_boundary_string = strdup(temp_image_boundary);
	else
		p_boundary_string = EM_SAFE_STRDUP(boundary_string);

	EM_SAFE_FREE(temp_image_boundary);

	do {
		if (multiple_image != NULL){
			p = multiple_image;
			start_header = multiple_image;
		}

		emcore_get_content_type_from_mime_string(start_header, &(image_data[i].mime_type));
		EM_DEBUG_LOG("image_data[i].mime_type : [%s]", image_data[i].mime_type);

		if ((strcasestr(p, "Content-Disposition: attachment")) || (!strcasestr(p, "Content-ID: <"))) {
			EM_DEBUG_LOG("Body has attachment no need to parse ");
			end = NULL;
			multiple_image = NULL;
		}
		else {  /*  HTML Content found */
			ch_image = 0;
			char *cid = NULL;
			char *temp_name = NULL;
			char *decoded_filename = NULL;

			image_data[i].image_file_name = NULL;

			if(!start_header) { /*prevent 27449*/
				EM_DEBUG_EXCEPTION("start_header NULL");
				EM_SAFE_FREE(p_boundary_string);
				return false;
			}

			txt_image  = start_header;
			temp_image = start_header;

			temp_name = strstr(txt_image, "name=");
			if (temp_name  != NULL){
				temp_image = temp_name;
				if (*(temp_image + 5) == '"') {
					temp_image = temp_image + 6;
					temp_name = temp_name + 6 ;
				} else {
					temp_image = temp_image + 5;
					temp_name = temp_name + 5;
				}

				while (*temp_image != CR){
					temp_image++;
					ch_image++;
				}

				if (ch_image > 0) {
					image_data[i].image_file_name = em_malloc(ch_image+3);
					if (image_data[i].image_file_name == NULL) {
						EM_DEBUG_EXCEPTION("em_malloc failed");
						EM_SAFE_FREE(p_boundary_string);
						return false;
					}
					if (*(temp_image - 1) == '"') {
						memcpy(image_data[i].image_file_name, temp_name, ch_image - 2);
					} else {
						memcpy(image_data[i].image_file_name, temp_name, ch_image);
					}
				}

				decoded_filename = emcore_decode_rfc2047_text(image_data[i].image_file_name, &err);
				if(decoded_filename) {
					memset(image_data[i].image_file_name, 0, ch_image);
					memcpy(image_data[i].image_file_name, decoded_filename, EM_SAFE_STRLEN(decoded_filename));
					EM_SAFE_FREE(decoded_filename);
				}
			}

			if (((temp_cid1 = (char *)strcasestr((const char *)start_header, "Content-ID: <")) != NULL)){
				if (temp_cid1) {
					cid = temp_cid1;
					temp_image = temp_cid1;
				}

				cid += 13;
				cid_end = strstr(temp_image, "\076");		/*  076 == '>' */

				image_data[i].content_id = (char *)em_malloc(cid_end-cid+5);
				if (image_data[i].content_id != NULL){
					strcpy(image_data[i].content_id, "cid:");
					memcpy(image_data[i].content_id+4, cid, cid_end-cid);
				}
				else
					EM_DEBUG_EXCEPTION("em_malloc() failed");

				if (image_data[i].image_file_name == NULL) {
					image_data[i].image_file_name = em_malloc((cid_end - cid) + 1);
					if (image_data[i].image_file_name == NULL) {
						EM_DEBUG_EXCEPTION("em_malloc failed");
						EM_SAFE_FREE(p_boundary_string);
						return false;
					}
					memcpy(image_data[i].image_file_name, cid, cid_end - cid);
				}
 		   	}

			txt_image = strstr(txt_image, CRLF_STRING CRLF_STRING);

			if (txt_image != NULL){
				txt_image += 4; /*  txt_image points at image content */
				start = txt_image;

				if (p_boundary_string) 
					end = strstr(txt_image, p_boundary_string);

				if (end == NULL){
					EM_DEBUG_LOG("HTML body contents exceeds limited Bytes");
					/*  end points to end of partial body data */
					end = txt_image + body_size - (txt_image-bufsendforparse);
				}
				else{
					EM_DEBUG_LOG("This image is fully downloaded");
					end -= 2;
					image_data[i].fully_downloaded = 1;
				}

				if ((temp_enc1 = (char *)strcasestr((const char *)start_header, "Content-transfer-encoding:"))  != NULL){ /*prevent 27449*/
					if (temp_enc1)
						start_header = temp_enc1;

					start_header  += strlen(CONTENT_TRANSFER_ENCODING_STRING);
					start_header = em_skip_whitespace_without_strdup(start_header);

					if (!start_header)
						EM_DEBUG_EXCEPTION(" Invalid parsing ");
					else{
						enc_type = ENCOTHER;
						if (strncasecmp(start_header, "base64", strlen("base64")) == 0)
							enc_type = ENCBASE64;
						else if (strncasecmp(start_header, "quoted-printable", strlen("quoted-printable")) == 0)
							enc_type = ENCQUOTEDPRINTABLE;

						EM_DEBUG_LOG("enc_type [%d]", enc_type);

						image_data[i].text_image = (char *)em_malloc((end-txt_image)+1);
						if (image_data[i].text_image){
							memcpy(image_data[i].text_image, start, end-txt_image);
							if (emcore_decode_body_text(image_data[i].text_image, end-txt_image, enc_type , &(image_data[i].dec_len), &err) < 0)
								EM_DEBUG_EXCEPTION("emcore_decode_body_text failed [%d]", err);
						}
						else
							EM_DEBUG_EXCEPTION("em_malloc() failed");
					}

					EM_DEBUG_LOG("Decoded length [%d]", image_data[i].dec_len);
				}
				else{
					image_data[i].text_image = (char *)em_malloc(end-txt_image);
					if (image_data[i].text_image)
						memcpy(image_data[i].text_image, start, end - txt_image);
					else
						EM_DEBUG_EXCEPTION("em_malloc() failed");
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

	EM_SAFE_FREE(p_boundary_string);

	EM_DEBUG_FUNC_END();
	return 1;
}

static int emcore_find_boundary_string_of_the_part(const char *whole_string, const char *first_line_of_part, char **result_boundary_string, int *error)
{
	EM_DEBUG_FUNC_BEGIN("whole_string[%p], first_line_of_part[%p], result_boundary_string[%p]", whole_string, first_line_of_part, result_boundary_string);
	int ret = false, err = EMAIL_ERROR_NONE;
	char *boundary_cur = NULL, *boundary_end = NULL, *boundary_string = NULL;

	if(!whole_string || !first_line_of_part || !result_boundary_string) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if(first_line_of_part - 2 > whole_string) {
		boundary_cur = (char*)first_line_of_part - 2; /* 2 means CRLF. */
		boundary_end = boundary_cur;

		do{
			boundary_cur--;
		} while (whole_string <= boundary_cur && *boundary_cur != LF && *boundary_cur != NULL_CHAR);

		boundary_cur++;

		if(boundary_end > boundary_cur && boundary_cur >= whole_string) {

			boundary_string = em_malloc(boundary_end - boundary_cur + 15);
			if(!boundary_string) {
				EM_DEBUG_EXCEPTION("em_malloc failed");
				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			memcpy(boundary_string, boundary_cur, boundary_end - boundary_cur);
			EM_DEBUG_LOG("boundary_string [%s]", boundary_string);
			*result_boundary_string = boundary_string;
		}
		else {
			EM_DEBUG_EXCEPTION("There is no string before the part");
			err = EMAIL_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}
	}
	else {
		EM_DEBUG_EXCEPTION("There is no string before the part");
		err = EMAIL_ERROR_ON_PARSING;
		goto FINISH_OFF;
	}
	ret = true;
FINISH_OFF:

	if(error)
		*error = err;

	EM_DEBUG_FUNC_END("ret[%d], err[%d]", ret, err);
	return ret;
}

static int emcore_parse_body_for_imap(char *body_str, int body_size, struct _m_content_info *cnt_info, int encoding_type, char **output_text_plain, char **output_text_html, email_image_data **output_image_data)
{
	EM_DEBUG_FUNC_BEGIN("body_str:[%p], cnt_info:[%p], encoding_type:[%d]", body_str, cnt_info, encoding_type);
	int err = EMAIL_ERROR_NONE;

	if (!body_str || strlen(body_str) == 0 || !cnt_info) {
		EM_DEBUG_EXCEPTION("Invalid paramter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	if (!output_text_plain || !output_text_html || !output_image_data) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	int dec_len = 0;
	int no_alternative_part_flag = 0;
	int no_html = 0;
	char *boundary_start = NULL;
	char *boundary_end = NULL;
	char *boundary_string = NULL;
	char *p_body_str = NULL;
	char *start_header = NULL;
	char *text_plain = NULL;
	char *text_html = NULL;
	email_image_data *image_data = NULL;

	char *temp_alternative_plain_header = NULL;
	char *temp_content_type = NULL;

	char *local_encoding_str = NULL;
	char *local_boundary_str = NULL;
	char *local_body_str = NULL;

	p_body_str = g_strdup(body_str);

	text_plain = em_malloc(body_size + 1);
	text_html  = em_malloc(body_size + 1);

	if (!text_plain || !text_html) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if ((cnt_info->text.plain && cnt_info->text.html) || cnt_info->file) {
		/* Start multipart parsing */
		/*  Partial body has headers with Content-Type: text/html or Content-Type: text/plain */
		no_alternative_part_flag = 0;
		if (((temp_alternative_plain_header = (char *)strcasestr(p_body_str, "Content-type: multipart/alternative")) != NULL)) {	/*  Found 'alternative' */
			if (((temp_content_type = (char *)strcasestr(p_body_str, "Content-type: text/plain")) != NULL)) {
				if (temp_content_type < temp_alternative_plain_header) {
					/*  This part is text/plain not alternative. */
					no_html = 1;
					no_alternative_part_flag = 1;
				} else {
					EM_DEBUG_LOG(" Content-type: multipart/alternative ");
					boundary_start = strstr(temp_alternative_plain_header, "--");
					if(!boundary_start) { /*prevent 37946 */
						err = EMAIL_ERROR_INVALID_DATA;
						goto FINISH_OFF;
					}
					boundary_end = strcasestr(boundary_start, "Content-type:");
					if(!boundary_end) { /*prevent 37946 */
						err = EMAIL_ERROR_INVALID_DATA;
						goto FINISH_OFF;
					}

					boundary_string = em_malloc(boundary_end - (boundary_start + strlen("--")));
					if (boundary_string == NULL) {
						EM_DEBUG_EXCEPTION("em_malloc failed");
						err = EMAIL_ERROR_OUT_OF_MEMORY;
						goto FINISH_OFF;
					}

					memcpy(boundary_string, boundary_start, boundary_end - (boundary_start + strlen("--")));
				}
			}
		} else
			no_alternative_part_flag = 1;

		if (no_alternative_part_flag) {
			boundary_start = strstr(p_body_str, "--");
			if(!boundary_start) { /*prevent 37946 */
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}
			boundary_end = strcasestr(boundary_start, "\r\n");
			if(!boundary_end) { /*prevent 37946 */
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}

			boundary_string = em_malloc(boundary_end - (boundary_start + strlen("--")));
			if (boundary_string == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed");
				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			memcpy(boundary_string, boundary_start, boundary_end - (boundary_start + strlen("--")));
		}

		if (boundary_string && boundary_end) { /*prevent 37946 */
			/* EM_DEBUG_LOG("boundary_string : [%s]", boundary_string); */

			if (((start_header = (char *)strcasestr(boundary_end, "Content-Type: text/html"))  != NULL) && (no_html  != 1) &&(((char *)strcasestr(boundary_end, "Content-Type: message/rfc822")) == NULL) &&
				(((char *)strcasestr(boundary_end, "Content-Type: text/rfc822-headers")) == NULL))
				emcore_parse_html_part_for_partial_body(start_header, boundary_string, p_body_str, text_html, body_size);

			if (((start_header = (char *)strcasestr(boundary_end, "Content-Type: text/plain"))  != NULL)) {
				char *internal_boundary_string = NULL;

				if(!emcore_find_boundary_string_of_the_part(p_body_str, start_header, &internal_boundary_string, &err)) {
					EM_DEBUG_EXCEPTION("internal_boundary_string failed [%d]", err);
				}

				if(!internal_boundary_string)
					internal_boundary_string = EM_SAFE_STRDUP(boundary_string);

				emcore_parse_plain_part_for_partial_body(boundary_end, start_header, internal_boundary_string, p_body_str, text_plain, body_size);
				EM_SAFE_FREE(internal_boundary_string);
			}

			if (((start_header = (char *)strcasestr((const char *)boundary_end, "Content-type: image/jpeg")) != (char *)NULL) ||
				((start_header = (char *)strcasestr((const char *)boundary_end, "Content-Type: image/jpg"))  != (char *)NULL) ||
				((start_header = (char *)strcasestr((const char *)boundary_end, "Content-Type: image/gif"))  != (char *)NULL) ||
				((start_header = (char *)strcasestr((const char *)boundary_end, "Content-Type: image/bmp"))  != (char *)NULL) ||
				((start_header = (char *)strcasestr((const char *)boundary_end, "Content-Type: image/png"))  != (char *)NULL)) {
				image_data = em_malloc(sizeof(email_image_data) * IMAGE_DISPLAY_PARTIAL_BODY_COUNT);
				if (image_data == NULL) {
					EM_DEBUG_EXCEPTION("em_malloc failed");
					err = EMAIL_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}
				emcore_parse_image_part_for_partial_body(boundary_end, start_header, boundary_string, p_body_str, image_data, body_size);
			}
		}
	}
	else {
		/*  Encoded Content-Type: text/html or Content-Type: text/plain  */
		/*  No Partial body has No headers with Content-Type: text/html or Content-Type: text/plain  */
		char *start_of_body = NULL;
		char *end_of_body = NULL;
		char *local_encoding = NULL;
		char *local_encoding_end = NULL;
		char *local_boundary_end = NULL;
		int local_encoding_len = 0;
		int local_boundary_len = 0;
		int enc_type = encoding_type;
		EM_DEBUG_LOG_DEV("mbody->encoding [%d] ", enc_type);

		if (EM_SAFE_STRLEN(p_body_str) > 2 && p_body_str[0] == '-' && p_body_str[1] == '-') {

			local_boundary_end = strstr(p_body_str, "\r\n");
			if (local_boundary_end) {
				local_boundary_len = local_boundary_end - p_body_str;
				local_boundary_str = em_malloc(local_boundary_len + 2);
				memcpy(local_boundary_str, p_body_str+2, local_boundary_len-2);
				EM_DEBUG_LOG_DEV("local_boundary_str : %s", local_boundary_str);
				end_of_body = strstr(local_boundary_end, local_boundary_str);
				if (end_of_body) end_of_body -= 2;
			}

			local_encoding = strcasestr(p_body_str, "Content-Transfer-Encoding: ");
			if (local_encoding) {
				local_encoding_end = strstr(local_encoding, "\r\n");

				if (local_encoding && (local_encoding_end > local_encoding)) {
					local_encoding_len = local_encoding_end - local_encoding;
					local_encoding_str = em_malloc(local_encoding_len + 2);
					if (local_encoding_str == NULL) {
						EM_DEBUG_EXCEPTION("em_malloc failed");
						err = EMAIL_ERROR_OUT_OF_MEMORY;
						goto FINISH_OFF;
					}

					if (local_encoding_str) {
						memcpy(local_encoding_str, local_encoding + EM_SAFE_STRLEN("Content-Transfer-Encoding: "), local_encoding_len);

						enc_type = ENCOTHER;
						if (strncasecmp(local_encoding_str, "base64", strlen("base64")) == 0)
							enc_type = ENCBASE64;
						else if (strncasecmp(local_encoding_str, "quoted-printable", strlen("quoted-printable")) == 0)
							enc_type = ENCQUOTEDPRINTABLE;

						EM_DEBUG_LOG_DEV("local enc_type [%d]", enc_type);
					}
				}
			}

			start_of_body = strstr(p_body_str, "\r\n\r\n");
			if (start_of_body) {
				start_of_body = start_of_body+strlen("\r\n\r\n");
				if (end_of_body && (end_of_body > start_of_body)) {
					local_body_str = em_malloc(end_of_body - start_of_body + 2);
					memcpy(local_body_str, start_of_body, end_of_body - start_of_body);
				} else {
					local_body_str = EM_SAFE_STRDUP(start_of_body);
				}
			}
		}

		if (!local_body_str)
			local_body_str = g_strdup(p_body_str);

		if (emcore_decode_body_text(local_body_str, EM_SAFE_STRLEN(local_body_str), enc_type, &dec_len, &err) < 0) {
			EM_DEBUG_EXCEPTION("emcore_decode_body_text failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("Decoded length [%d]", dec_len);

		if (dec_len > 0) {
			if (cnt_info->text.plain) {
				memcpy(text_plain, local_body_str, dec_len);
			}

			if (cnt_info->text.html) {
				memcpy(text_html, local_body_str, dec_len);
			}
		}
	}

FINISH_OFF:

	EM_SAFE_FREE(p_body_str);
	EM_SAFE_FREE(boundary_string);
	EM_SAFE_FREE(local_boundary_str);
	EM_SAFE_FREE(local_encoding_str);
	EM_SAFE_FREE(local_body_str);

	if (err != EMAIL_ERROR_NONE) {
		EM_SAFE_FREE(text_plain);
		EM_SAFE_FREE(text_html);

		if (image_data)
			emcore_free_email_image_data(&image_data, IMAGE_DISPLAY_PARTIAL_BODY_COUNT);
	} else {
		*output_text_plain = text_plain;
		*output_text_html = text_html;
		*output_image_data = image_data;
	}

	EM_DEBUG_FUNC_END("err : [%d]", err);
	return err;
}
#endif

#define TAG_LENGTH 16
#define COMMAND_LENGTH 2000
#ifdef __FEATURE_USE_GMIME__
static int emcore_gmime_download_imap_partial_mail_body(MAILSTREAM *stream, int input_download_size, email_event_partial_body_thd *pbd_event, int item_count, int *error)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p], input_download_size[%d], pbd_event [%p], item_count [%d], error [%p]", stream, input_download_size, pbd_event, item_count, error);

	int ret = false, err = EMAIL_ERROR_NONE;
	int j = 0, i = 0;
	int i32_index = 0;
	int total_mail_size = 0, total_attachment_size = 0;
	int temp_count = 0, attachment_num = 0, inline_attachment_num = 0;
	int inline_download_count = 0;
	char path_buf[512] = {0,};
	char move_buf[512] = {0,};
	char uid_range_string_to_be_downloaded[UID_RANGE_STRING_LENGTH] = {0,};
	char imap_tag[TAG_LENGTH] = {0, };
	char command[COMMAND_LENGTH] = {0, };
	char uid_string[TEMP_STRING_LENGTH] = {0, };
	char rfc822_micalg[TEMP_STRING_LENGTH] = {0, };
	char rfc822_protocol[TEMP_STRING_LENGTH] = {0, };
	const char *sender = NULL;
	IMAPLOCAL *imaplocal = NULL;
	IMAPPARSEDREPLY *reply_from_server = NULL;
	emstorage_mail_tbl_t *mail = NULL;
	email_partial_buffer *imap_response = NULL;
	BODY *body = NULL;
	struct _m_content_info *cnt_info = NULL;
	emstorage_attachment_tbl_t attachment_tbl;
	email_event_partial_body_thd *stSectionNo = NULL;
	emstorage_mail_text_tbl_t *mail_text = NULL; /* prevent */

	GMimeMessage *message1 = NULL;
	GMimeParser *parser1 = NULL;
	GMimeStream *stream1 = NULL;
	char *bodystructure_start = NULL;
	char *bodystructure_buf = NULL;
	char *bodystructure_ptr = NULL;

	GMimeStream *stream2 = NULL;
	GMimeMessage *message2 = NULL;
	GMimeParser *parser2 = NULL;
	char *fulltext = NULL;

	GMimePartIter *iter1 = NULL;
	GMimePartIter *iter2 = NULL;
	GMimeObject *part_tmp1 = NULL;
	GMimeObject *part_tmp2 = NULL;
	char *part_path = NULL;

	if (!(stream) || !(imaplocal = stream->local) || !imaplocal->netstream || !pbd_event) {
		EM_DEBUG_EXCEPTION("invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_FUNC_END("ret [%d]", ret);
		return ret;
	}

	EM_DEBUG_LOG("Start of emcore_get_section_for_partial_download, item_count = %d ", item_count);

	/* For constructing UID list which is having 10 UID or less at a time */
	for (j = 0, stSectionNo = pbd_event; (stSectionNo  != NULL && j < item_count); j++)
 	{
		/* delete log before uploading master branch. it is flooding */

		if (i32_index >= UID_RANGE_STRING_LENGTH){
			EM_DEBUG_EXCEPTION("String length exceeded its limitation!");
			goto FINISH_OFF;
		}

		if (j == item_count - 1)
			i32_index += SNPRINTF(uid_range_string_to_be_downloaded + i32_index,
					UID_RANGE_STRING_LENGTH - i32_index, "%lu", stSectionNo[j].server_mail_id);
		else
			i32_index += SNPRINTF(uid_range_string_to_be_downloaded + i32_index,
					UID_RANGE_STRING_LENGTH - i32_index, "%lu,", stSectionNo[j].server_mail_id);
	}

	SNPRINTF(imap_tag, TAG_LENGTH, "%08lx", 0xffffffff & (stream->gensym++));
	SNPRINTF(command, COMMAND_LENGTH, "%s UID FETCH %s (RFC822.HEADER BODYSTRUCTURE BODY.PEEK[TEXT]<0.%d>)\015\012",
			imap_tag, uid_range_string_to_be_downloaded, input_download_size);

	EM_DEBUG_LOG("command : %s", command);

	/*  Sending out the IMAP request */
	if (!net_sout(imaplocal->netstream, command, (int)EM_SAFE_STRLEN(command))) {
		EM_DEBUG_EXCEPTION("net_sout failed...");
		err = EMAIL_ERROR_CONNECTION_BROKEN;
		goto FINISH_OFF;
	}

	/*  responce from the server */
	imap_response = emcore_get_response_from_server(imaplocal->netstream,
			imap_tag, &reply_from_server, input_download_size, item_count);

	if (!imap_response || !reply_from_server ) {
		EM_DEBUG_EXCEPTION(" Invalid response from emcore_get_response_from_server");
		goto FINISH_OFF;
	}

	for (i = 0; i < item_count ; i++) {

		total_mail_size = 0;
		total_attachment_size = 0;
		attachment_num = 0;

		if(!(imap_response[i].bodystructure) || imap_response[i].bodystructure_len <= 0) continue;

		/* Search the account id of pbd_event */
		for (temp_count = 0; temp_count <= item_count && pbd_event[temp_count].server_mail_id != imap_response[i].uid_no; temp_count++)
			continue;

		if (temp_count > item_count) {
			EM_DEBUG_EXCEPTION("Can't find proper server_mail_id");
			goto FINISH_OFF;
		}

		/* Start to parse the body */
		EM_DEBUG_LOG("Start partial body of server_mail_id %d", imap_response[i].uid_no);

		/* Check the body download status and body size */
		SNPRINTF(uid_string, sizeof(uid_string), "%ld", imap_response[i].uid_no);
		if (!emstorage_get_maildata_by_servermailid(pbd_event[temp_count].multi_user_name, pbd_event[temp_count].mailbox_id, uid_string, &mail, true, &err) || !mail) {
			EM_DEBUG_EXCEPTION("emstorage_get_mail_data_by_servermailid failed : [%d]", err);
			if (err == EMAIL_ERROR_MAIL_NOT_FOUND || !mail)
				goto FINISH_OFF;
		}

		if (mail->body_download_status & EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED) {
			EM_DEBUG_LOG("Downloaded mail");
			continue;
		}

		if (cnt_info) {
			emcore_free_content_info(cnt_info);
			EM_SAFE_FREE(cnt_info);
		}

		if (body) {
			mail_free_body(&body);
			body = NULL;
		}

		EM_SAFE_FREE(fulltext);

		if (!(cnt_info = em_malloc(sizeof(struct _m_content_info)))) {
			EM_DEBUG_EXCEPTION("em_malloc failed...");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		body = mail_newbody();
		if (body == NULL) {
			EM_DEBUG_EXCEPTION("New body creationg failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		/* Get the body strcuture string */
		bodystructure_start = strstr(imap_response[i].bodystructure, "BODYSTRUCTURE (");
		if (!bodystructure_start) {
			EM_DEBUG_EXCEPTION("Invalid bodystructure :[%s]", imap_response[i].bodystructure);
			err = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		bodystructure_start = bodystructure_start + strlen("BODYSTRUCTURE");
		bodystructure_ptr = bodystructure_buf = EM_SAFE_STRDUP(bodystructure_start);
		EM_DEBUG_LOG_DEV("GMIME BODYSTRUCTURE:%s", bodystructure_buf);

		mail_parameters(stream, SET_FREEBODYSPAREP, emcore_free_body_sparep);
		imap_parse_body_structure(stream, body,
				(unsigned char **)&bodystructure_buf, reply_from_server);

		EM_SAFE_FREE(bodystructure_ptr);

		/* Construct message1(fake gmime message object) with rfc822 header */
		stream1 = g_mime_stream_mem_new_with_buffer(imap_response[i].rfc822header, imap_response[i].rfc822header_len);
		parser1 = g_mime_parser_new_with_stream(stream1);
		if (stream1) g_object_unref(stream1);

		message1 = g_mime_parser_construct_message(parser1);
		if (parser1) g_object_unref(parser1);

		/* message1 is multipart? */
		if (GMIME_IS_MULTIPART (message1->mime_part)) {

			/* Fill up mime part of message1 using bodystructure info */
			emcore_gmime_construct_multipart((GMimeMultipart *)message1->mime_part, body, "1", &total_mail_size);

			/* Construct message2 with partial body text */
			EM_DEBUG_LOG_DEV("RFC822H:%s", imap_response[i].rfc822header);
			EM_DEBUG_LOG_DEV("BODYTEXT:%s", imap_response[i].bodytext);
			fulltext = g_strconcat(imap_response[i].rfc822header, "\r\n\r\n", imap_response[i].bodytext, NULL);

			stream2 = g_mime_stream_mem_new_with_buffer(fulltext, EM_SAFE_STRLEN(fulltext));
			parser2 = g_mime_parser_new_with_stream(stream2);
			if (stream2) g_object_unref(stream2);
			EM_SAFE_FREE(fulltext);

			message2 = g_mime_parser_construct_message(parser2);
			if (parser2) g_object_unref(parser2);

			/* Merge message2 with message1 to make complete gmime message object */

			iter1 = g_mime_part_iter_new((GMimeObject *)message1);
			iter2 = g_mime_part_iter_new((GMimeObject *)message2);

			if (!g_mime_part_iter_is_valid(iter1) || !g_mime_part_iter_is_valid(iter2)) {
				EM_DEBUG_EXCEPTION("Part iterator is not valid");
				goto FINISH_OFF;
			}

			do {
				part_tmp2 = g_mime_part_iter_get_current(iter2);
				if (part_tmp2 && GMIME_IS_PART(part_tmp2)) {
					part_path = g_mime_part_iter_get_path(iter2);

					if (g_mime_part_iter_jump_to(iter1, part_path)) {
						EM_DEBUG_LOG_DEV("g_mime_part_iter_jump_to: %s", part_path);
						part_tmp1 = g_mime_part_iter_get_current(iter1);
						if (part_tmp1 && GMIME_IS_PART(part_tmp1)) {
							GMimeContentType *ctype_tmp1 = NULL;
							GMimeContentType *ctype_tmp2 = NULL;
							char *ctype_str1 = NULL;
							char *ctype_str2 = NULL;
							ctype_tmp1 = g_mime_object_get_content_type(part_tmp1);
							ctype_tmp2 = g_mime_object_get_content_type(part_tmp2);
							ctype_str1 = g_mime_content_type_to_string(ctype_tmp1);
							ctype_str2 = g_mime_content_type_to_string(ctype_tmp2);
							EM_DEBUG_LOG_DEV("%s", ctype_str1);
							EM_DEBUG_LOG_DEV("%s", ctype_str2);

							if (g_ascii_strcasecmp(ctype_str1, ctype_str2) != 0) {
								EM_DEBUG_EXCEPTION("PART of fake_message is different from message");
							} else {
								char *ctype_size = NULL;
								GMimeDataWrapper *content_tmp2 = NULL;

								ctype_size = (char *)g_mime_content_type_get_parameter(ctype_tmp1, "part_size");
								EM_DEBUG_LOG("Part.size.bytes[%s]", ctype_size);
								if (ctype_size)
									g_mime_object_set_content_type_parameter(part_tmp2, "part_size", ctype_size);

								content_tmp2 = g_mime_part_get_content_object(GMIME_PART(part_tmp2));
								if (content_tmp2 && part_tmp1 && GMIME_IS_PART(part_tmp1)) {
									g_mime_part_set_content_object(GMIME_PART(part_tmp1), content_tmp2);
								}

								/*part_idx = g_mime_multipart_index_of(GMIME_MULTIPART(message1->mime_part), part_tmp1);
								replaced = g_mime_multipart_replace(GMIME_MULTIPART(message1->mime_part), part_idx, part_tmp2);
								if (!replaced) {
									EM_DEBUG_EXCEPTION("g_mime_multipart_replace failed");
								}
								if (replaced) g_object_unref(replaced);*/

								/*content_tmp2 = g_mime_part_get_content_object(GMIME_PART(part_tmp2));
								part_tmp1 = g_mime_part_iter_get_current(iter1);
								if (content_tmp2 && part_tmp1 && GMIME_IS_PART(part_tmp1)) {
									g_mime_part_set_content_object(GMIME_PART(part_tmp1), content_tmp2);
									g_object_unref(content_tmp2);
								}*/

							}

							EM_SAFE_FREE(ctype_str1);
							EM_SAFE_FREE(ctype_str2);
						}
					} else {
						EM_DEBUG_LOG_SEC("g_mime_part_iter_jump_to: failed to jump to %s", part_path);
						EM_SAFE_FREE(part_path);
						goto FINISH_OFF;
					}

					EM_SAFE_FREE(part_path);
				}
			} while (g_mime_part_iter_next(iter2));

			if (iter1) {
				g_mime_part_iter_free(iter1);
				iter1 = NULL;
			}

			if (iter2) {
				g_mime_part_iter_free(iter2);
				iter2 = NULL;
			}

			if (message2) {
				g_object_unref(message2);
				message2 = NULL;
			}

			if (g_strrstr(g_mime_message_get_sender(message1), "mmsc.plusnet.pl") != NULL ||
				g_strrstr(g_mime_message_get_sender(message1), "mms.t-mobile.pl") != NULL) {
				cnt_info->attachment_only = 1;
			}

			g_mime_message_foreach(message1, emcore_gmime_imap_parse_foreach_cb, (gpointer)cnt_info);

		} else if (GMIME_IS_PART (message1->mime_part)) {
			GMimeDataWrapper *content = NULL;
			GMimePart *single_part = GMIME_PART(message1->mime_part);

			EM_DEBUG_LOG("constructing a %s/%s part", body_types[body->type], body->subtype);
			emcore_gmime_construct_part(single_part, body, "1", &total_mail_size);

			stream1 = g_mime_stream_mem_new_with_buffer(imap_response[i].bodytext, imap_response[i].bodytext_len);
			//parser1 = g_mime_parser_new_with_stream(stream1);
			content = g_mime_data_wrapper_new_with_stream(stream1, single_part->encoding);
			if (stream1) g_object_unref (stream1);

			g_mime_part_set_content_object(single_part, content);
			if (content) g_object_unref(content);

			sender = g_mime_message_get_sender(message1);

			if (sender) {
				if (g_strrstr(sender, "mmsc.plusnet.pl") != NULL ||
					g_strrstr(sender, "mms.t-mobile.pl") != NULL) {
					cnt_info->attachment_only = 1;
				}
			}

			g_mime_message_foreach(message1, emcore_gmime_imap_parse_foreach_cb, (gpointer)cnt_info);
		}

		if (!strcasecmp(body->subtype, "pkcs7-mime")) {
			if (emcore_get_attribute_value_of_body_part(body->parameter, "PROTOCOL", rfc822_protocol, TEMP_STRING_LENGTH, false, &err)) {
				if (strcasestr(rfc822_protocol, "enveloped-data"))
					mail->smime_type = EMAIL_SMIME_ENCRYPTED;
				else
					mail->smime_type = EMAIL_SMIME_SIGNED_AND_ENCRYPTED;
			}
		} else if (!strcasecmp(body->subtype, "encrypted")) {
			mail->smime_type = EMAIL_PGP_ENCRYPTED;
		} else if (!strcasecmp(body->subtype, "signed")) {
			if (emcore_get_attribute_value_of_body_part(body->parameter, "MICALG", rfc822_micalg, TEMP_STRING_LENGTH, false, &err)) {
				mail->digest_type = emcore_get_digest_type(rfc822_micalg);
			}

			if (emcore_get_attribute_value_of_body_part(body->parameter, "PROTOCOL", rfc822_protocol, TEMP_STRING_LENGTH, false, &err)) {
				if (strcasestr(rfc822_protocol, "pkcs7-signature"))
					mail->smime_type = EMAIL_SMIME_SIGNED;
				else
					mail->smime_type = EMAIL_PGP_SIGNED;
			}
		} else {
			mail->smime_type = EMAIL_SMIME_NONE;
		}

		if (body) {
			mail_free_body(&body);
			body = NULL;
		}

		if (message1) {
			g_object_unref(message1);
			message1 = NULL;
		}

		/* Update the attachment info except inline attachment */
		if ((err = emcore_update_attachment_except_inline(pbd_event[temp_count].multi_user_name, 
                                                        cnt_info, 
                                                        pbd_event[temp_count].account_id, 
                                                        mail->mail_id, 
                                                        pbd_event[temp_count].mailbox_id, 
                                                        &total_attachment_size, 
                                                        &attachment_num, 
                                                        &inline_attachment_num)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_update_attachment_except_inline failed : [%d]", err);
			goto FINISH_OFF;
		}

		mail->mail_size            = total_mail_size;
		mail->attachment_count     = attachment_num;
		mail->inline_content_count = inline_attachment_num;

		if (imap_response[i].bodytext_len == 0) {
			EM_DEBUG_LOG("BODY size is zero");
			continue;
		}

		/* text/plain */
		if (cnt_info->text.plain) {
			char *charset_plain_text = NULL;
			memset(path_buf, 0x00, sizeof(path_buf));
			memset(move_buf, 0x00, sizeof(move_buf));

			if (cnt_info->text.plain_charset)
				charset_plain_text = cnt_info->text.plain_charset;
			else {
				if (mail->default_charset) {
					charset_plain_text = mail->default_charset;
				} else {
					charset_plain_text = "UTF-8";
				}
			}

			if (!emstorage_create_dir(pbd_event[temp_count].multi_user_name, pbd_event[temp_count].account_id, mail->mail_id, 0, &err))
				EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);

			if (!emstorage_get_save_name(pbd_event[temp_count].multi_user_name, 
                                        pbd_event[temp_count].account_id, 
                                        mail->mail_id, 
                                        0, 
                                        charset_plain_text, 
                                        move_buf, 
                                        path_buf, 
                                        sizeof(path_buf),
                                        &err))
				EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);

			if (!emstorage_move_file(cnt_info->text.plain, move_buf, false, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
				mail->file_path_plain = NULL;
			}
			else
				mail->file_path_plain = EM_SAFE_STRDUP(path_buf);
			EM_DEBUG_LOG_SEC("mail->file_path_plain [%s]", mail->file_path_plain);
		}

		/* text/html */
		if (cnt_info->text.html) {
			char *charset_html_text = NULL;
			memset(path_buf, 0x00, sizeof(path_buf));
			memset(move_buf, 0x00, sizeof(move_buf));

			if (cnt_info->text.html_charset)
				charset_html_text = cnt_info->text.html_charset;
			else {
				if (mail->default_charset) {
					charset_html_text = mail->default_charset;
				} else {
					charset_html_text = "UTF-8";
				}
			}

			charset_html_text = g_strconcat(charset_html_text, HTML_EXTENSION_STRING, NULL);

			if (!emstorage_create_dir(pbd_event[temp_count].multi_user_name, pbd_event[temp_count].account_id, mail->mail_id, 0, &err))
				EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);

			if (!emstorage_get_save_name(pbd_event[temp_count].multi_user_name, 
                                        pbd_event[temp_count].account_id, 
                                        mail->mail_id, 
                                        0, 
                                        charset_html_text, 
                                        move_buf, 
                                        path_buf, 
                                        sizeof(path_buf), 
                                        &err))
				EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);

			if (!emstorage_move_file(cnt_info->text.html, move_buf, false, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
				mail->file_path_html = NULL;
			}
			else
				mail->file_path_html = EM_SAFE_STRDUP(path_buf);
			g_free(charset_html_text);
		}

		/* inline attachment */
		inline_download_count = 0;
		if (cnt_info->inline_file) {
			struct attachment_info *temp_file = NULL;

			if (inline_attachment_num > 0) {
				temp_file = cnt_info->inline_file;
				while (temp_file) {
					if (temp_file->type == INLINE_ATTACHMENT && temp_file->save_status == 1) {
						inline_download_count++;
                    memset(path_buf, 0x00, sizeof(path_buf));
                    memset(move_buf, 0x00, sizeof(move_buf));

						if (!emstorage_create_dir(pbd_event[temp_count].multi_user_name, pbd_event[temp_count].account_id, mail->mail_id, 0, &err))
							EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);

						if (!emstorage_get_save_name(pbd_event[temp_count].multi_user_name, 
                                                    pbd_event[temp_count].account_id, 
                                                    mail->mail_id, 
                                                    0, 
                                                    temp_file->name, 
                                                    move_buf, 
                                                    path_buf, 
                                                    sizeof(path_buf), 
                                                    &err))
							EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);

						if (!emstorage_move_file(temp_file->save, move_buf, false, &err)) {
							EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
							goto FINISH_OFF;
						}

						if (!(mail->body_download_status & EMAIL_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED)) {
							memset(&attachment_tbl, 0x00, sizeof(emstorage_attachment_tbl_t));
							attachment_tbl.mail_id                          = mail->mail_id;
							attachment_tbl.account_id                       = pbd_event[temp_count].account_id;
							attachment_tbl.mailbox_id                       = pbd_event[temp_count].mailbox_id;
							attachment_tbl.attachment_name                  = temp_file->name;
							attachment_tbl.attachment_size                  = temp_file->size;
							attachment_tbl.attachment_path                  = path_buf;
							attachment_tbl.attachment_save_status           = 1;
							attachment_tbl.attachment_inline_content_status = 1; /* set to 1 for inline image */
							attachment_tbl.attachment_mime_type             = temp_file->attachment_mime_type;
							attachment_tbl.content_id                       = temp_file->content_id;
							EM_DEBUG_LOG("mime_type : [%s]", temp_file->attachment_mime_type);

							if (!emstorage_add_attachment(pbd_event[temp_count].multi_user_name, &attachment_tbl, false, false, &err))
								EM_DEBUG_EXCEPTION("emstorage_add_attachment failed - %d", err);
						}
					}
					else {
						if (temp_file->save)
							g_remove(temp_file->save);
					}
					temp_file = temp_file->next;
				}
			}
		}

		mail->body_download_status = (mail->body_download_status & ~0x00000003) | (( (total_mail_size - total_attachment_size <= input_download_size) && (inline_download_count == inline_attachment_num))?
				EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED: EMAIL_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED);

		EM_DEBUG_LOG("inline_download_count[%d] inline_attachment_num[%d]", inline_download_count, inline_attachment_num);
		EM_DEBUG_LOG("total_mail_size[%d] total_attachment_size[%d] input_download_size[%d]", total_mail_size, total_attachment_size, input_download_size);
		EM_DEBUG_LOG("mail_id [%d] body_download_status [%d]", mail->mail_id, mail->body_download_status);

		/* Get preview text */
		if ( (err = emcore_get_preview_text_from_file(pbd_event[temp_count].multi_user_name, 
                                                        mail->file_path_plain, 
                                                        mail->file_path_html, 
                                                        MAX_PREVIEW_TEXT_LENGTH, 
                                                        &(mail->preview_text))) != EMAIL_ERROR_NONE)
			EM_DEBUG_EXCEPTION("emcore_get_preview_text_from_file() failed[%d]", err);

		/* Update body contents */
		if (!emstorage_change_mail_field(pbd_event[temp_count].multi_user_name, mail->mail_id, UPDATE_PARTIAL_BODY_DOWNLOAD, mail, true, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed - %d", err);
			goto FINISH_OFF;
		}

#ifdef __FEATURE_BODY_SEARCH__
		/* strip html content and save into mail_text_tbl */
		char *stripped_text = NULL;
		if (!emcore_strip_mail_body_from_file(pbd_event[temp_count].multi_user_name, mail, &stripped_text, &err) || stripped_text == NULL) {
			EM_DEBUG_EXCEPTION("emcore_strip_mail_body_from_file failed [%d]", err);
		}

		if (!emstorage_get_mail_text_by_id(pbd_event[temp_count].multi_user_name, mail->mail_id, &mail_text, true, &err) || !mail_text) {
			EM_DEBUG_EXCEPTION("emstorage_get_mail_text_by_id failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_SAFE_FREE(mail_text->body_text);
		mail_text->body_text = stripped_text;

		if (!emstorage_change_mail_text_field(pbd_event[temp_count].multi_user_name, mail->mail_id, mail_text, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_change_mail_text_field failed [%d]", err);
			goto FINISH_OFF;
		}

		if (mail_text)
			emstorage_free_mail_text(&mail_text, 1, NULL);
#endif

		if (mail)
			emstorage_free_mail(&mail, 1, NULL);

		if (false == emcore_delete_pbd_activity(pbd_event[temp_count].multi_user_name, pbd_event[temp_count].account_id, pbd_event[temp_count].mail_id, pbd_event[temp_count].activity_id, &err)){
			EM_DEBUG_EXCEPTION("emcore_delete_pbd_activity failed [%d]", err);
			goto FINISH_OFF;
		}
	}

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

	if (message1) {
		g_object_unref(message1);
		message1 = NULL;
	}

	if (message2) {
		g_object_unref(message2);
		message2 = NULL;
	}

	if (iter1) {
		g_mime_part_iter_free(iter1);
		iter1 = NULL;
	}

	if (iter2) {
		g_mime_part_iter_free(iter2);
		iter2 = NULL;
	}

	if (cnt_info) {
		emcore_free_content_info(cnt_info);
		EM_SAFE_FREE(cnt_info);
	}

	EM_SAFE_FREE(fulltext);

	if (body)
		mail_free_body(&body);

	if (mail)
		emstorage_free_mail(&mail, 1, NULL);

	if (mail_text)
		emstorage_free_mail_text(&mail_text, 1, NULL);

	if (imap_response)
		emcore_free_email_partial_buffer(&imap_response, item_count);

	EM_SAFE_FREE(bodystructure_ptr);

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
#endif

INTERNAL_FUNC int emcore_download_bulk_partial_mail_body_for_pop3(MAILSTREAM *stream, int input_download_size, email_event_partial_body_thd *pbd_event, int item_count, int *error)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p], pbd_event [%p], item_count [%d], error [%p]", stream, pbd_event, item_count, error);
	int ret = false, err = EMAIL_ERROR_NONE;
	int i;

	if (!stream || !pbd_event) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	for (i = 0; i < item_count; i++) {
 		EM_DEBUG_LOG_SEC("pbd_event[%d].account_id [%d], mail_id [%d], server_mail_id [%d], activity_id [%d]", \
			i, pbd_event[i].account_id, pbd_event[i].mail_id, pbd_event[i].server_mail_id, pbd_event[i].activity_id);

#ifdef __FEATURE_USE_GMIME__
		if (!emcore_gmime_download_body_sections(pbd_event[i].multi_user_name, stream, pbd_event[i].account_id,
				pbd_event[i].mail_id, 0, input_download_size, -1, 0, 0, &err)) {
			EM_DEBUG_EXCEPTION("emcore_gmime_download_body_sections failed - %d", err);
			goto FINISH_OFF;
		}
#else
		if (!emcore_download_body_multi_sections_bulk (stream,
                                                   pbd_event[i].account_id,
                                                   pbd_event[i].mail_id,
                                                   false,
                                                   false,
                                                   input_download_size,
                                                   -1,
                                                   0,
                                                   &err)) {
			EM_DEBUG_EXCEPTION("emcore_download_body_multi_sections_bulk failed");
			goto FINISH_OFF;
		}
#endif

		if (false == emcore_delete_pbd_activity(pbd_event[i].multi_user_name, pbd_event[i].account_id, pbd_event[i].mail_id, pbd_event[i].activity_id, &err)){
			EM_DEBUG_EXCEPTION("emcore_delete_pbd_activity failed [%d]", err);
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



INTERNAL_FUNC int emcore_download_bulk_partial_mail_body(MAILSTREAM *stream, email_event_partial_body_thd *pbd_event, int item_count, int *error)
{
	EM_DEBUG_FUNC_BEGIN("stream [%p], pbd_event [%p], item_count [%d], error [%p]", stream, pbd_event, item_count, error);
	int ret = false, err = EMAIL_ERROR_NONE;
	emstorage_account_tbl_t *pbd_account_tbl = NULL;
	int auto_download_size = 0;

	if (!stream || !pbd_event) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_get_account_by_id(pbd_event[0].multi_user_name, pbd_event[0].account_id, EMAIL_ACC_GET_OPT_DEFAULT, &pbd_account_tbl, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	auto_download_size = (pbd_account_tbl->auto_download_size<4096)?4096:pbd_account_tbl->auto_download_size;

	switch (pbd_account_tbl->incoming_server_type){
		case EMAIL_SERVER_TYPE_IMAP4:
#ifdef __FEATURE_USE_GMIME__
			ret = emcore_gmime_download_imap_partial_mail_body(stream, auto_download_size, pbd_event, item_count, &err);
#endif
			break;

		case EMAIL_SERVER_TYPE_POP3:
			ret = emcore_download_bulk_partial_mail_body_for_pop3(stream, auto_download_size, pbd_event, item_count, &err);
			break;

		default:
			err = EMAIL_ERROR_NOT_SUPPORTED;
			ret = false;
			break;
	}

	ret = true;
FINISH_OFF:
	if (error)
		*error = err;

	emstorage_free_account(&pbd_account_tbl, 1, NULL);

	EM_DEBUG_FUNC_END("ret [%d] err [%d]", ret, err);
	return ret;
}

static void emcore_free_email_partial_buffer(email_partial_buffer **partial_buffer, int item_count)
{
	EM_DEBUG_FUNC_BEGIN("count : [%d]", item_count);

	if (item_count <= 0 || !partial_buffer || !*partial_buffer) {
		return;
	}

	email_partial_buffer *p = *partial_buffer;
	int i;

	for (i = 0; i < item_count ; i++, p++) {
		EM_SAFE_FREE(p->bodystructure);
		EM_SAFE_FREE(p->bodytext);
		EM_SAFE_FREE(p->rfc822header);
	}

	EM_SAFE_FREE(*partial_buffer);
	EM_DEBUG_FUNC_END();
}

static email_partial_buffer *emcore_get_response_from_server (NETSTREAM *nstream, char *tag, IMAPPARSEDREPLY **reply, int input_download_size, int item_count)
{
	EM_DEBUG_FUNC_BEGIN();

	if (!nstream || !tag || !reply || item_count <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return NIL;
	}

	email_partial_buffer *server_response = NULL;
	IMAPPARSEDREPLY *ret_reply = NULL;

	char *cur_line = NULL;
	char *full_line = NULL;
	char *tmp = NULL;

	int rfc822header_size = 0;
	int body_size = 0;
	int count = 0;
	int ret = false;

	char *p_uid = NULL;
	char *p_bodystructure = NULL;
	char *p_bodystructure_end = NULL;
	char *p_body_text = NULL;
	char *p_rfc822header = NULL;

	server_response = (email_partial_buffer *)em_malloc(sizeof(email_partial_buffer) * item_count);
	if (NULL == server_response) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		return NIL;
	}

	while (nstream) {

		if (!(cur_line = net_getline(nstream))) {
			EM_DEBUG_EXCEPTION("net_getline failed...");
			goto FINISH_OFF;
		}

		if (count < item_count) {
			tmp = full_line;
			if (tmp)
				full_line = g_strconcat(tmp, " ", cur_line, NULL);
			else
				full_line = g_strdup(cur_line);

			EM_SAFE_FREE(cur_line);
			EM_SAFE_FREE(tmp);

			p_rfc822header = strcasestr(full_line, "RFC822.HEADER {");
			if (p_rfc822header && server_response[count].rfc822header == NULL) {
				p_rfc822header += strlen("RFC822.HEADER {");
				rfc822header_size = atoi(p_rfc822header);

				server_response[count].rfc822header_len = rfc822header_size;
				server_response[count].rfc822header = em_malloc(server_response[count].rfc822header_len + 1);
				if (server_response[count].rfc822header == NULL) {
					EM_DEBUG_EXCEPTION("em_malloc failed");
					goto FINISH_OFF;
				}

				if (net_getbuffer(nstream, server_response[count].rfc822header_len, server_response[count].rfc822header) <= 0) {
					EM_DEBUG_EXCEPTION("net_getbuffer failed");
					goto FINISH_OFF;
				}

				p_rfc822header = NULL;
				continue;
			}

			p_bodystructure = strcasestr(full_line, "BODYSTRUCTURE");
			p_bodystructure_end = strcasestr(full_line, "BODY[TEXT]");

			/* check whether full header is received */
			if (p_bodystructure && p_bodystructure_end) {

				/* get UID */
				if ((p_uid = strcasestr(full_line, "UID")) != NULL) {
					p_uid = p_uid + strlen("UID ");
					server_response[count].uid_no = atol(p_uid);
				}

				/* get BODYSTRUCTURE */
				server_response[count].bodystructure_len = p_bodystructure_end - p_bodystructure;
				server_response[count].bodystructure = em_malloc(server_response[count].bodystructure_len + 1);
				if (server_response[count].bodystructure == NULL) {
					EM_DEBUG_EXCEPTION("em_malloc failed");
					goto FINISH_OFF;
				}
				SNPRINTF(server_response[count].bodystructure, server_response[count].bodystructure_len, "%s", p_bodystructure);
				/*EM_DEBUG_LOG("BODYSTRUCTURE(%d)[%s]", server_response[count].header_len, server_response[count].header);*/

				/* get BODY size & text */
				p_body_text = strcasestr(full_line, "BODY[TEXT]<0> {");
				if (!p_body_text) {
					EM_DEBUG_EXCEPTION("can't find BODY[TEXT] size");
					goto FINISH_OFF;
				}

				p_body_text += strlen("BODY[TEXT]<0> {");
				body_size = atoi(p_body_text);
				server_response[count].bodytext_len = (body_size > input_download_size) ? input_download_size : body_size;
				server_response[count].bodytext = em_malloc(server_response[count].bodytext_len + 1);
				if (server_response[count].bodytext == NULL) {
					EM_DEBUG_EXCEPTION("em_malloc failed");
					goto FINISH_OFF;
				}

				if (net_getbuffer(nstream, server_response[count].bodytext_len, server_response[count].bodytext) <= 0) {
					EM_DEBUG_EXCEPTION("net_getbuffer failed");
					goto FINISH_OFF;
				}
				/*EM_DEBUG_LOG("BODYSTR(%d)[%s]", server_response[count].body_len, server_response[count].body);*/

				EM_SAFE_FREE(full_line);
				count++;
				p_bodystructure = NULL;
				p_bodystructure_end = NULL;
				continue;
			} else {
				/* not a full header, accumulate into buffer util get the end of header */
				p_bodystructure = NULL;
				p_bodystructure_end = NULL;
				continue;
			}
		}

		/* get tag and result */
		if (strncmp(cur_line, tag, EM_SAFE_STRLEN(tag)) == 0) {
			ret_reply = em_malloc(sizeof(IMAPPARSEDREPLY));
			if (!ret_reply){
				EM_DEBUG_EXCEPTION("em_malloc failed");
				goto FINISH_OFF;
			}

			if(reply)
				*reply = ret_reply;

			if (strncmp(cur_line + EM_SAFE_STRLEN(tag) + 1, "OK", 2) == 0) {
				ret_reply->line = (unsigned char*)EM_SAFE_STRDUP(tag);
				ret_reply->tag  = (unsigned char*)EM_SAFE_STRDUP(tag);
				ret_reply->key  = (unsigned char*)strdup("OK");
				ret_reply->text = (unsigned char*)strdup("Success");
				ret = true;
				break;
			} else {
				EM_DEBUG_EXCEPTION("Tagged Response not OK. IMAP4 Response -> [%s]", cur_line);
				ret_reply->line = (unsigned char*)EM_SAFE_STRDUP(tag);
				ret_reply->tag  = (unsigned char*)EM_SAFE_STRDUP(tag);
				ret_reply->key  = (unsigned char*)strdup("NO");
				ret_reply->text = (unsigned char*)strdup("Fail");
				goto FINISH_OFF;
			}
		}

		EM_SAFE_FREE(cur_line);
	}

FINISH_OFF:

	EM_SAFE_FREE(tmp);
	EM_SAFE_FREE(cur_line);
	EM_SAFE_FREE(full_line);

	if (!ret || count != item_count ) {
		EM_DEBUG_EXCEPTION("emcore_get_response_from_server may be failed ret[%d] count[%d]", ret, count);
		emcore_free_email_partial_buffer(&server_response, item_count);
	}

	EM_DEBUG_FUNC_END("server_response [%p]", server_response);
	return server_response;
}

#endif /*  __FEATURE_PARTIAL_BODY_DOWNLOAD__ */
/* EOF */
