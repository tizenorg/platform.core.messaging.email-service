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
 * File :  email-core-mail.c
 * Desc :  Mail Operation
 *
 * Auth :
 *
 * History :
 *    2006.08.16  :  created
 *****************************************************************************/
#undef close
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <vconf.h>
#include <contacts.h>
#include <contacts_internal.h>

#include "email-internal-types.h"
#include "c-client.h"
#include "lnx_inc.h"
#include "email-utilities.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-core-mime.h"
#include "email-core-mailbox.h"
#include "email-storage.h"
#include "email-network.h"
#include "email-core-mailbox-sync.h"
#include "email-core-event.h"
#include "email-core-account.h"
#include "email-core-signal.h"
#include "email-core-smtp.h"
#include "email-core-container.h"
#include "email-core-gmime.h"

#include "email-convert.h"
#include "email-debug-log.h"



#define ST_SILENT   (long) 0x2	/* don't return results */
#define ST_SET      (long) 0x4	/* set vs. clear */

static char g_new_server_uid[129];
static int g_copyuid_account_id = 0;
static char *g_multi_user_name = NULL;

bool only_body_download = false;

int multi_part_body_size = 0;
int is_multi_part_body_download_all = 0;
int _pop3_receiving_mail_id = 0;
int _pop3_received_body_size = 0;
int _pop3_last_notified_body_size = 0;
int _pop3_total_body_size = 0;

int _imap4_received_body_size = 0;
int _imap4_last_notified_body_size = 0;
int _imap4_total_body_size = 0;
int _imap4_download_noti_interval_value = 0;

static int emcore_delete_mails_from_pop3_server(char *multi_user_name, email_account_t *input_account, int input_mail_ids[], int input_mail_id_count);
static int emcore_mail_cmd_read_mail_pop3(void *stream, int msgno, int limited_size, int *downloded_size, int *total_body_size, int *err_code);

extern long pop3_send(MAILSTREAM *stream, char *command, char *args);


#ifdef FEATURE_CORE_DEBUG
static char *_getType(int type)
{
	switch (type) {
		case 0:  return "TYPETEXT";
		case 1:  return "TYPEMULTIPART";
		case 2:  return "TYPEMESSAGE";
		case 3:  return "TYPEAPPLICATION";
		case 4:  return "TYPEAUDIO";
		case 5:  return "TYPEVIDEO";
		case 6:  return "TYPEMODEL";
		case 7:  return "TYPEOTHER";
	}
	return g_strdup_printf("%d", type);
}

static char *_getEncoding(int encoding)
{
	switch (encoding) {
		case 0:  return "ENC7BIT";
		case 1:  return "ENC8BIT";
		case 2:  return "ENCBINARY";
		case 3:  return "ENCBASE64";
		case 4:  return "ENCQUOTEDPRINTABLE";
		case 5:  return "ENCOTHER";
	}
	return g_strdup_printf("%d", encoding);
}

static void _print_parameter(PARAMETER *param)
{
	while (param != NULL) {
		EM_DEBUG_LOG("param->attribute[%s]", param->attribute);
		EM_DEBUG_LOG("param->value[%s]", param->value);

		param = param->next;
	}
}

static void _print_stringlist(STRINGLIST *stringlist)
{
	while (stringlist != NULL) {
		EM_DEBUG_LOG("stringlist->text.data[%s]", stringlist->text.data);
		EM_DEBUG_LOG("stringlist->text.size[%ld]", stringlist->text.size);

		stringlist = stringlist->next;
	}
}

void _print_body(BODY *body, int recursive)
{
	EM_DEBUG_LOG(" ========================================================== ");

	if (body != NULL) {
		EM_DEBUG_LOG("body->type[%s]", _getType(body->type));
		EM_DEBUG_LOG("body->encoding[%s]", _getEncoding(body->encoding));
		EM_DEBUG_LOG("body->subtype[%s]", body->subtype);

		EM_DEBUG_LOG("body->parameter[%p]", body->parameter);

		_print_parameter(body->parameter);

		EM_DEBUG_LOG_SEC("body->id[%s]", body->id);
		EM_DEBUG_LOG("body->description[%s]", body->description);

		EM_DEBUG_LOG("body->disposition.type[%s]", body->disposition.type);
		EM_DEBUG_LOG("body->disposition.parameter[%p]", body->disposition.parameter);

		_print_parameter(body->disposition.parameter);

		EM_DEBUG_LOG("body->language[%p]", body->language);

		_print_stringlist(body->language);

		EM_DEBUG_LOG_SEC("body->location[%s]", body->location);

		EM_DEBUG_LOG("body->mime.offset[%ld]", body->mime.offset);
		EM_DEBUG_LOG("body->mime.text.data[%s]", body->mime.text.data);
		EM_DEBUG_LOG("body->mime.text.size[%ld]", body->mime.text.size);

		EM_DEBUG_LOG("body->contents.offset[%ld]", body->contents.offset);
		EM_DEBUG_LOG("body->contents.text.data[%p]", body->contents.text.data);
		EM_DEBUG_LOG("body->contents.text.size[%ld]", body->contents.text.size);

		EM_DEBUG_LOG("body->nested.part[%p]", body->nested.part);

		EM_DEBUG_LOG("body->size.lines[%ld]", body->size.lines);
		EM_DEBUG_LOG("body->size.bytes[%ld]", body->size.bytes);

		EM_DEBUG_LOG("body->md5[%s]", body->md5);
		EM_DEBUG_LOG("body->sparep[%p]", body->sparep);

		if (recursive) {
			PART *part = body->nested.part;

			while (part != NULL) {
				_print_body(&(part->body), recursive);
				part = part->next;
			}
		}
	}

	EM_DEBUG_LOG(" ========================================================== ");
}
#endif /*  FEATURE_CORE_DEBUG */

static int pop3_mail_delete(MAILSTREAM *stream, int msgno, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msgno[%d], err_code[%p]", stream, msgno, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	POP3LOCAL *pop3local = NULL;
	char cmd[64];
	char *p = NULL;

	if (!stream) {
		EM_DEBUG_EXCEPTION("stream[%p]", stream);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(pop3local = stream->local) || !pop3local->netstream) {
		EM_DEBUG_EXCEPTION("invalid POP3 stream detected...");
		err = EMAIL_ERROR_UNKNOWN;
		goto FINISH_OFF;
	}

	memset(cmd, 0x00, sizeof(cmd));

	SNPRINTF(cmd, sizeof(cmd), "DELE %d\015\012", msgno);

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG("[POP3] >>> %s", cmd);
#endif

	/*  send command  :  delete specified mail */
	if (!net_sout(pop3local->netstream, cmd, (int)EM_SAFE_STRLEN(cmd))) {
		EM_DEBUG_EXCEPTION("net_sout failed...");
		err = EMAIL_ERROR_CONNECTION_BROKEN;		/* EMAIL_ERROR_UNKNOWN; */
		goto FINISH_OFF;
	}

	/*  receive response */
	if (!(p = net_getline(pop3local->netstream))) {
		EM_DEBUG_EXCEPTION("net_getline failed...");
		err = EMAIL_ERROR_INVALID_RESPONSE;
		goto FINISH_OFF;
	}

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG("[POP3] <<< %s", p);
#endif

	if (*p == '-') {		/*  '-ERR' */
		err = EMAIL_ERROR_POP3_DELE_FAILURE;		/* EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER; */
		goto FINISH_OFF;
	}

	if (*p != '+') {		/*  '+OK' ... */
		err = EMAIL_ERROR_INVALID_RESPONSE;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE(p);

	if (err_code)
		*err_code = err;

	return ret;
}

#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__

static void emcore_mail_copyuid_ex(MAILSTREAM *stream, char *mailbox, unsigned long uidvalidity, SEARCHSET *sourceset, SEARCHSET *destset)
{

	EM_DEBUG_FUNC_BEGIN();

	int index = -1;
	int i = 0;
	int err = EMAIL_ERROR_NONE;

	unsigned long  first_uid = 0;
	unsigned long last_uid = 0;

	unsigned long *old_server_uid = NULL;
	unsigned long *new_server_uid = NULL;
	int count = 0;
	SEARCHSET *temp = sourceset;

	char old_server_uid_char[129];
	char new_server_uid_char[129];
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;

	if (NULL == sourceset || NULL == destset) {
		/*
		sourceset will be NULL when the sequence of UIDs sent to server for mail move operation has all invalid old server uids
		if sourceset is NULL then corresponding dest set will be NULL
		*/

		EM_DEBUG_LOG("emcore_mail_copyuid_ex failed :  Invalid Parameters--> sourceset[%p] , destset[%p]", sourceset, destset);
		return;
	}

	/* To get count of mails actually moved */

	while (temp) {
		if (temp->first > 0) {
			first_uid = temp->first;

			count++;

			if (temp->last > 0) {
				last_uid = temp->last;

				while (first_uid < last_uid) {
					first_uid++;
					count++;
				}
			}
		}

		temp = temp->next;
	}


	EM_DEBUG_LOG("Count of mails copied [%d]", count);
	old_server_uid = em_malloc(count * sizeof(unsigned long));
	if (old_server_uid == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed : EMAIL_ERROR_OUT_OF_MEMORY");
		return;
	}

	new_server_uid = em_malloc(count * sizeof(unsigned long));
	if (new_server_uid == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed : EMAIL_ERROR_OUT_OF_MEMORY");
		EM_SAFE_FREE(old_server_uid);
		return;
	}

	/* While loop below will collect all old server uid from sourceset into old_server_uid array */
	while (sourceset) {
		if (sourceset->first > 0) {
			first_uid = sourceset->first;

			index++;
			old_server_uid[index] = first_uid;

			if (sourceset->last > 0) {
				last_uid = sourceset->last;

				while (first_uid < last_uid) {
					first_uid++;
					index++;

					old_server_uid[index] = first_uid;
				}
			}
		}

		sourceset = sourceset->next;
	}

	/* While loop below will collect all new server uid from destset into new_server_uid array */

	index = -1;
	first_uid = last_uid = 0;

	while (destset) {
		if (destset->first > 0) {
			first_uid = destset->first;

			index++;
			new_server_uid[index] = first_uid;

			if (destset->last > 0) {
				last_uid = destset->last;

				while (first_uid < last_uid) {
					first_uid++;
					index++;

					new_server_uid[index] = first_uid;
				}
			}
		}

		destset = destset->next;
	}

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
	if (!emstorage_get_mailbox_by_name(g_multi_user_name, g_copyuid_account_id, -1, mailbox, &mailbox_tbl, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_name failed [%d", err);
	}
#endif

	/* For loop below updates mail_tbl and mail_read_mail_uid_tbl with new server uids*/

	for (i = 0; i <= index; ++i) {

		memset(old_server_uid_char, 0x00, sizeof(old_server_uid_char));
		snprintf(old_server_uid_char, sizeof(old_server_uid_char), "%ld", old_server_uid[i]);

		EM_DEBUG_LOG("Old Server Uid Char[%s]", old_server_uid_char);

		memset(new_server_uid_char, 0x00, sizeof(new_server_uid_char));
		snprintf(new_server_uid_char, sizeof(new_server_uid_char),"%ld", new_server_uid[i]);

		EM_DEBUG_LOG("New Server Uid Char[%s]", new_server_uid_char);

		if (!emstorage_update_server_uid(g_multi_user_name, 0, old_server_uid_char, new_server_uid_char, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_update_server_uid failed...[%d]", err);
		}

		if (!emstorage_update_read_mail_uid_by_server_uid(g_multi_user_name, old_server_uid_char, new_server_uid_char, mailbox, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_update_read_mail_uid_by_server_uid failed... [%d]", err);
		}

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
		if (false == emstorage_update_pbd_activity(g_multi_user_name, old_server_uid_char, new_server_uid_char, mailbox, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_update_pbd_activity failed... [%d]", err);
		}

#endif

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
		if (mailbox_tbl) {
			if (!emstorage_update_auto_download_activity(g_multi_user_name, old_server_uid_char, new_server_uid_char, NULL, mailbox_tbl->mailbox_id, &err))
				EM_DEBUG_EXCEPTION("emstorage_update_auto_download_activity failed : [%d]", err);
		}
#endif

	}

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
	if (mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

	g_copyuid_account_id = 0;
    EM_SAFE_FREE(g_multi_user_name);
#endif

	EM_SAFE_FREE(old_server_uid);
	EM_SAFE_FREE(new_server_uid);
}

INTERNAL_FUNC int emcore_move_mail_on_server_ex(char *multi_user_name,
												int account_id,
												int src_mailbox_id,
												int mail_ids[],
												int num,
												int dest_mailbox_id,
												int *error_code)
{
	EM_DEBUG_FUNC_BEGIN();
	MAILSTREAM *stream = NULL;
	int err_code = EMAIL_ERROR_NONE;
	email_account_t *ref_account = NULL;
	int ret = false;
	int i = 0;
	email_id_set_t *id_set = NULL;
	int id_set_count = 0;

	email_uid_range_set *uid_range_set = NULL;
	int len_of_each_range = 0;

	email_uid_range_set *uid_range_node = NULL;

	char **string_list = NULL;
	int string_count = 0;
	emstorage_mailbox_tbl_t* dest_mailbox = NULL;

	if (num <= 0  || account_id <= 0 || src_mailbox_id <= 0 || dest_mailbox_id <= 0 || NULL == mail_ids) {
		if (error_code != NULL) {
			*error_code = EMAIL_ERROR_INVALID_PARAM;
		}
		EM_DEBUG_LOG("Invalid Parameters- num[%d], account_id[%d], src_mailbox_id[%d], dest_mailbox_id[%d], mail_ids[%p]", num, account_id, src_mailbox_id, dest_mailbox_id, mail_ids);
		return false;
	}

	ref_account = emcore_get_account_reference(multi_user_name, account_id, false);
	if (NULL == ref_account) {
		EM_DEBUG_EXCEPTION(" emcore_get_account_reference failed[%d]", account_id);
		*error_code = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (ref_account->incoming_server_type != EMAIL_SERVER_TYPE_IMAP4) {
		*error_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											account_id,
											src_mailbox_id,
											true,
											(void **)&stream,
											&err_code)) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed[%d]", err_code);
		goto FINISH_OFF;
	}

	if (NULL != stream) {
		g_copyuid_account_id = account_id;
        g_multi_user_name = EM_SAFE_STRDUP(multi_user_name);
		mail_parameters(stream, SET_COPYUID, emcore_mail_copyuid_ex);
		EM_DEBUG_LOG("calling mail_copy_full FODLER MAIL COPY ");
		/*  [h.gahlaut] Break the set of mail_ids into comma separated strings of given length  */
		/*  Length is decided on the basis of remaining keywords in the Query to be formed later on in emstorage_get_id_set_from_mail_ids  */
		/*  Here about 90 bytes are required for fixed keywords in the query-> SELECT local_uid, server_uid from mail_read_mail_uid_tbl where local_uid in (....) ORDER by server_uid  */
		/*  So length of comma separated strings which will be filled in (.....) in above query can be maximum QUERY_SIZE - 90  */

		if (false == emcore_form_comma_separated_strings(mail_ids, num, QUERY_SIZE - 90, &string_list, &string_count, &err_code))  {
			EM_DEBUG_EXCEPTION("emcore_form_comma_separated_strings failed [%d]", err_code);
			goto FINISH_OFF;
		}

		if ((err_code = emstorage_get_mailbox_by_id(multi_user_name, dest_mailbox_id, &dest_mailbox)) != EMAIL_ERROR_NONE || !dest_mailbox) {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err_code);
			goto FINISH_OFF;
		}

		/*  Now execute one by one each comma separated string of mail_ids */

		for (i = 0; i < string_count; ++i) {
			/*  Get the set of mail_ds and corresponding server_mail_ids sorted by server mail ids in ascending order */
			if (false == emstorage_get_id_set_from_mail_ids(multi_user_name, string_list[i], &id_set, &id_set_count, &err_code)) {
				EM_DEBUG_EXCEPTION("emstorage_get_id_set_from_mail_ids failed [%d]", err_code);
				goto FINISH_OFF;
			}

			/*  Convert the sorted sequence of server mail ids to range sequences of given length. A range sequence will be like A : B, C, D: E, H */

			len_of_each_range = MAX_IMAP_COMMAND_LENGTH - 40; /*  1000 is the maximum length allowed RFC 2683. 40 is left for keywords and tag. */

			if (false == emcore_convert_to_uid_range_set(id_set, id_set_count, &uid_range_set, len_of_each_range, &err_code)) {
				EM_DEBUG_EXCEPTION("emcore_convert_to_uid_range_set failed [%d]", err_code);
				goto FINISH_OFF;
			}

			uid_range_node = uid_range_set;

			while (uid_range_node != NULL) {
				/* Remove comma from end of uid_range */
				uid_range_node->uid_range[EM_SAFE_STRLEN(uid_range_node->uid_range) - 1] = '\0';
				EM_DEBUG_LOG("uid_range_node->uid_range - %s", uid_range_node->uid_range);
				if (!mail_copy_full(stream, uid_range_node->uid_range, dest_mailbox->mailbox_name, CP_UID | CP_MOVE)) {
					EM_DEBUG_EXCEPTION("emcore_move_mail_on_server_ex :   Mail cannot be moved failed");
					EM_DEBUG_EXCEPTION("Mail MOVE failed ");
					goto FINISH_OFF;
				} else if (!mail_expunge_full(stream, uid_range_node->uid_range, EX_UID)) {
					EM_DEBUG_EXCEPTION("emcore_move_mail_on_server_ex :   Mail cannot be expunged after move. failed!");
						EM_DEBUG_EXCEPTION("Mail Expunge after move failed ");
						  goto FINISH_OFF;
				} else {
					EM_DEBUG_LOG("Mail MOVE SUCCESS ");
				}

				uid_range_node = uid_range_node->next;
			}

			emcore_free_uid_range_set(&uid_range_set);

			EM_SAFE_FREE(id_set);

			id_set_count = 0;
		}

	} else {
		EM_DEBUG_EXCEPTION(">>>> STREAM DATA IS NULL >>> ");
		goto FINISH_OFF;
	}


	ret = true;

FINISH_OFF:
	emcore_free_comma_separated_strings(&string_list, &string_count); /*prevent 17958*/

	stream = mail_close(stream);

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

#ifdef __FEATURE_LOCAL_ACTIVITY__
	if (ret || ref_account->incoming_server_type != EMAIL_SERVER_TYPE_IMAP4) /* Delete local activity for POP3 mails and successful move operation in IMAP */ {
		emstorage_activity_tbl_t new_activity;
		for (i = 0; i < num; i++) {
			memset(&new_activity, 0x00, sizeof(emstorage_activity_tbl_t));
			new_activity.activity_type = ACTIVITY_MOVEMAIL;
			new_activity.account_id    = account_id;
			new_activity.mail_id	   = mail_ids[i];
			new_activity.src_mbox	   = src_mailbox;
			new_activity.dest_mbox	   = dest_mailbox;

			if (!emcore_delete_activity(&new_activity, &err_code)) {
				EM_DEBUG_EXCEPTION(">>>>>>Local Activity ACTIVITY_MOVEMAIL [%d] ", err_code);
			}
		}
	}
#endif
	if (dest_mailbox) {
		emstorage_free_mailbox(&dest_mailbox, 1, &err_code);
	}

	if (error_code != NULL) {
		*error_code = err_code;
	}

	return ret;
}

int emcore_delete_mails_from_imap4_server(char *multi_user_name,
											int account_id,
											int mailbox_id,
											int mail_ids[],
											int num,
											int from_server,
											int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	IMAPLOCAL *imaplocal = NULL;
	char *p = NULL;
	MAILSTREAM *stream = NULL;
	char tag[MAX_TAG_SIZE];
	char cmd[MAX_IMAP_COMMAND_LENGTH];
	email_id_set_t *id_set = NULL;
	int id_set_count = 0;
	int i = 0;
	email_uid_range_set *uid_range_set = NULL;
	int len_of_each_range = 0;
	email_uid_range_set *uid_range_node = NULL;
	char **string_list = NULL;
	int string_count = 0;

	if (num <= 0 || !mail_ids || account_id <= 0 || mailbox_id <= 0) {
		EM_DEBUG_EXCEPTION(" Invalid parameter ");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											account_id,
											mailbox_id,
											true,
											(void **)&stream,
											&err)) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}

	/* [h.gahlaut] Break the set of mail_ids into comma separated strings of given length  */
	/* Length is decided on the basis of remaining keywords in the Query */
	/* to be formed later on in emstorage_get_id_set_from_mail_ids  */
	/* Here about 90 bytes are required for fixed keywords in the query-> SELECT local_uid, server_uid */
	/* from mail_read_mail_uid_tbl where local_uid in (....) ORDER by server_uid  */
	/* So length of comma separated strings which will be filled in (.....) in above query */
	/* can be maximum QUERY_SIZE - 90  */
	if (false == emcore_form_comma_separated_strings(mail_ids,
														num,
														QUERY_SIZE - 90,
														&string_list,
														&string_count,
														&err)) {
		EM_DEBUG_EXCEPTION("emcore_form_comma_separated_strings failed [%d]", err);
		goto FINISH_OFF;
	}

	/*  Now execute one by one each comma separated string of mail_ids  */

	for (i = 0; i < string_count; ++i) {
		/*  Get the set of mail_ds and corresponding server_mail_ids sorted by server mail ids in ascending order  */
		if (false == emstorage_get_id_set_from_mail_ids(multi_user_name,
														string_list[i],
														&id_set,
														&id_set_count,
														&err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_id_set_from_mail_ids failed [%d]", err);
			goto FINISH_OFF;
		}

		/* Convert the sorted sequence of server mail ids to range sequences of given length. */
		/* A range sequence will be like A : B, C, D: E, H  */
		/* 1000 is the maximum length allowed RFC 2683. 40 is left for keywords and tag.  */
		len_of_each_range = MAX_IMAP_COMMAND_LENGTH - 40;
		if (false == emcore_convert_to_uid_range_set(id_set,
														id_set_count,
														&uid_range_set,
														len_of_each_range,
														&err)) {
			EM_DEBUG_EXCEPTION("emcore_convert_to_uid_range_set failed [%d]", err);
			goto FINISH_OFF;
		}

		uid_range_node = uid_range_set;

		while (uid_range_node != NULL) {
			/*  Remove comma from end of uid_range  */
			uid_range_node->uid_range[EM_SAFE_STRLEN(uid_range_node->uid_range) - 1] = '\0';

			if (!(imaplocal = stream->local) || !imaplocal->netstream) {
				EM_DEBUG_EXCEPTION("invalid IMAP4 stream detected...");

				err = EMAIL_ERROR_CONNECTION_BROKEN;
				goto FINISH_OFF;
			}

			memset(tag, 0x00, sizeof(tag));
			memset(cmd, 0x00, sizeof(cmd));

			SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
			SNPRINTF(cmd, sizeof(cmd), "%s UID STORE %s +FLAGS (\\Deleted)\015\012", tag, uid_range_node->uid_range);

			EM_DEBUG_LOG("[IMAP4] >>> %s", cmd);


			/* send command  :  set deleted flag */
			if (!net_sout(imaplocal->netstream, cmd, (int)EM_SAFE_STRLEN(cmd))) {
				EM_DEBUG_EXCEPTION("net_sout failed...");

				err = EMAIL_ERROR_CONNECTION_BROKEN;		/* EMAIL_ERROR_UNKNOWN */
				goto FINISH_OFF;
			}


			while (imaplocal->netstream) {
				/* receive response */
				if (!(p = net_getline(imaplocal->netstream))) {
					EM_DEBUG_EXCEPTION("net_getline failed...");

					err = EMAIL_ERROR_INVALID_RESPONSE;		/* EMAIL_ERROR_UNKNOWN; */
					goto FINISH_OFF;
				}


				EM_DEBUG_LOG("[IMAP4] <<< %s", p);

				/* To confirm - Commented out as FETCH response does not contain the tag - may be a problem for common stream in email-service*/
				/* Success case - delete all local activity and entry from mail_read_mail_uid_tbl
				if (strstr(p, "FETCH") != NULL) {
					EM_DEBUG_LOG(" FETCH Response recieved ");
					delete_success = true;
					EM_SAFE_FREE(p);
					break;
				}
				*/


				if (!strncmp(p, tag, EM_SAFE_STRLEN(tag))) {
					if (!strncmp(p + EM_SAFE_STRLEN(tag) + 1, "OK", 2)) {
						/*Error scenario delete all local activity and entry from mail_read_mail_uid_tbl */
						EM_DEBUG_LOG(" OK Response recieved ");
						EM_SAFE_FREE(p);
						break;
					} else {
						/*  'NO' or 'BAD' */
						err = EMAIL_ERROR_IMAP4_STORE_FAILURE;		/* EMAIL_ERROR_INVALID_RESPONSE; */
						goto FINISH_OFF;
					}
				}

				EM_SAFE_FREE(p);
				}

				memset(tag, 0x00, sizeof(tag));
				memset(cmd, 0x00, sizeof(cmd));

				EM_DEBUG_LOG("Calling Expunge");

				SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
				SNPRINTF(cmd, sizeof(cmd), "%s EXPUNGE\015\012", tag);

				EM_DEBUG_LOG("[IMAP4] >>> %s", cmd);

				/* send command  :   EXPUNGE */
				if (!net_sout(imaplocal->netstream, cmd, (int)EM_SAFE_STRLEN(cmd))) {
					EM_DEBUG_EXCEPTION("net_sout failed...");

					err = EMAIL_ERROR_CONNECTION_BROKEN;
					goto FINISH_OFF;
				}

				while (imaplocal->netstream) {
					/* receive response */
					if (!(p = net_getline(imaplocal->netstream))) {
						EM_DEBUG_EXCEPTION("net_getline failed...");

						err = EMAIL_ERROR_INVALID_RESPONSE;		/* EMAIL_ERROR_UNKNOWN; */
						goto FINISH_OFF;
					}

					EM_DEBUG_LOG("[IMAP4] <<< %s", p);

					if (!strncmp(p, tag, EM_SAFE_STRLEN(tag))) {
						if (!strncmp(p + EM_SAFE_STRLEN(tag) + 1, "OK", 2)) {
#ifdef __FEATURE_LOCAL_ACTIVITY__
							int index = 0;
							emstorage_mail_tbl_t **mail = NULL;

							mail = (emstorage_mail_tbl_t **) em_malloc(num * sizeof(emstorage_mail_tbl_t *));
							if (!mail) {
								EM_DEBUG_EXCEPTION("em_mallocfailed");
								err = EMAIL_ERROR_UNKNOWN;
								goto FINISH_OFF;
							}

							if (delete_success) {
								for (index = 0 ; index < num; index++) {
									if (!emstorage_get_downloaded_mail(multi_user_name, mail_ids[index], &mail[index], false, &err)) {
										EM_DEBUG_LOG("emstorage_get_uid_by_mail_id failed [%d]", err);

										if (err == EMAIL_ERROR_MAIL_NOT_FOUND) {
											EM_DEBUG_LOG("EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER  : ");
											continue;
										}
									}

									if (mail[index]) {
										/* Clear entry from mail_read_mail_uid_tbl */
										if (mail[index]->server_mail_id != NULL) {
											if (!emstorage_remove_downloaded_mail(multi_user_name,
																					mail[index]->account_id,
																					mail[index]->mailbox_id,
																					mail[index]->mailbox_name,
																					mail[index]->server_mail_id,
																					true,
																					&err)) {
												EM_DEBUG_LOG("emstorage_remove_downloaded_mail falied [%d]", err);
											}
										}

										/* Delete local activity */
										emstorage_activity_tbl_t new_activity;
										memset(&new_activity, 0x00, sizeof(emstorage_activity_tbl_t));
										if (from_server == EMAIL_DELETE_FOR_SEND_THREAD) {
											new_activity.activity_type = ACTIVITY_DELETEMAIL_SEND;
											EM_DEBUG_LOG("from_server == EMAIL_DELETE_FOR_SEND_THREAD ");
										} else {
											new_activity.activity_type	= ACTIVITY_DELETEMAIL;
										}

										new_activity.mail_id		= mail[index]->mail_id;
										new_activity.server_mailid	= NULL;
										new_activity.src_mbox		= NULL;
										new_activity.dest_mbox		= NULL;

										if (!emcore_delete_activity(&new_activity, &err)) {
											EM_DEBUG_EXCEPTION(" emcore_delete_activity  failed  - %d ", err);
										}
									} else {
										/* Fix for crash seen while deleting Outbox mails which are moved to Trash. Outbox mails do not have server mail id and are not updated in mail_read_mail_uid_tbl.
										  * So there is no need of deleting entry in mail_read_mail_uid_tbl. However local activity has to be deleted.
										*/
										/* Delete local activity */
										emstorage_activity_tbl_t new_activity;
										memset(&new_activity, 0x00, sizeof(emstorage_activity_tbl_t));
										new_activity.activity_type	= ACTIVITY_DELETEMAIL;
										new_activity.mail_id		= mail_ids[index]; /* valid mail id passed for outbox mails*/
										new_activity.server_mailid	= NULL;
										new_activity.src_mbox		= NULL;
										new_activity.dest_mbox		= NULL;

										if (!emcore_delete_activity(&new_activity, &err)) {
											EM_DEBUG_EXCEPTION(" emcore_delete_activity  failed  - %d ", err);
										}
									}
								}

								for (index = 0; index < num; index++) {
									if (!emstorage_free_mail(&mail[index], 1, &err)) {
										EM_DEBUG_EXCEPTION(" emstorage_free_mail [%d]", err);
									}
								}

								EM_SAFE_FREE(mail);
							}

#endif
	EM_SAFE_FREE(p);
							break;
						} else {		/*  'NO' or 'BAD' */
							err = EMAIL_ERROR_IMAP4_EXPUNGE_FAILURE;		/* EMAIL_ERROR_INVALID_RESPONSE; */
							goto FINISH_OFF;
						}
					}

					EM_SAFE_FREE(p);
				}
				uid_range_node = uid_range_node->next;
		}

		emcore_free_uid_range_set(&uid_range_set);

		EM_SAFE_FREE(id_set);

		id_set_count = 0;
	}

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(p);
	EM_SAFE_FREE(id_set); /*prevent 17954*/

	if (stream)
		stream = mail_close(stream);

	emcore_free_comma_separated_strings(&string_list, &string_count);

	if (false == ret) {
		emcore_free_uid_range_set(&uid_range_set);
	}

	if (err_code) {
		*err_code = err;
	}

	return ret;

}

#endif

INTERNAL_FUNC int emcore_imap4_send_command(MAILSTREAM *stream, imap4_cmd_t cmd_type, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], cmd_type[%d], err_code[%p]", stream, cmd_type, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	IMAPLOCAL *imaplocal = NULL;
	char tag[16], cmd[64];
	char *p = NULL;

	if (!(imaplocal = stream->local) || !imaplocal->netstream) {
		EM_DEBUG_EXCEPTION("invalid IMAP4 stream detected...");

		err = EMAIL_ERROR_INVALID_PARAM;		/* EMAIL_ERROR_UNKNOWN */
		goto FINISH_OFF;
	}

	memset(tag, 0x00, sizeof(tag));
	memset(cmd, 0x00, sizeof(cmd));

	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));

	if (cmd_type == IMAP4_CMD_EXPUNGE) {
		SNPRINTF(cmd, sizeof(cmd), "%s EXPUNGE\015\012", tag);
	} else if (cmd_type == IMAP4_CMD_NOOP) {
		SNPRINTF(cmd, sizeof(cmd), "%s NOOP\015\012", tag);
	}

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG("[IMAP4] >>> %s", cmd);
#endif

	/*  send command  :  delete flaged mail */
	if (!net_sout(imaplocal->netstream, cmd, (int)EM_SAFE_STRLEN(cmd))) {
		EM_DEBUG_EXCEPTION("net_sout failed...");

		err = EMAIL_ERROR_CONNECTION_BROKEN;
		goto FINISH_OFF;
	}

	while (imaplocal->netstream) {
		/*  receive response */
		if (!(p = net_getline(imaplocal->netstream))) {
			EM_DEBUG_EXCEPTION("net_getline failed...");
			err = EMAIL_ERROR_INVALID_RESPONSE;		/* EMAIL_ERROR_UNKNOWN; */
			goto FINISH_OFF;
		}

#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG("[IMAP4] <<< %s", p);
#endif

		if (!strncmp(p, tag, EM_SAFE_STRLEN(tag))) {
			if (!strncmp(p + EM_SAFE_STRLEN(tag) + 1, "OK", 2)) {
				EM_SAFE_FREE(p);
				break;
			} else {		/*  'NO' or 'BAD' */
				if (cmd_type == IMAP4_CMD_EXPUNGE) {
					err = EMAIL_ERROR_IMAP4_EXPUNGE_FAILURE;		/* EMAIL_ERROR_INVALID_RESPONSE; */
				} else if (cmd_type == IMAP4_CMD_NOOP) {
					err = EMAIL_ERROR_IMAP4_NOOP_FAILURE;
				}
				goto FINISH_OFF;
			}
		}

		EM_SAFE_FREE(p);
	}

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE(p);

	if (err_code)
		*err_code = err;

	return ret;
}

INTERNAL_FUNC int emcore_pop3_send_command(MAILSTREAM *stream, pop3_cmd_t cmd_type, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], err_code[%p]", stream, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	POP3LOCAL *pop3local = NULL;
	char cmd[64];
	char *p = NULL;

	if (!stream) {
		EM_DEBUG_EXCEPTION("stream[%p]", stream);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(pop3local = stream->local) || !pop3local->netstream) {
		EM_DEBUG_EXCEPTION("invalid POP3 stream detected...");
		err = EMAIL_ERROR_UNKNOWN;
		goto FINISH_OFF;
	}

	memset(cmd, 0x00, sizeof(cmd));

	if (cmd_type == POP3_CMD_NOOP)
		SNPRINTF(cmd, sizeof(cmd), "NOOP\015\012");

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG("[POP3] >>> %s", cmd);
#endif

	/*  send command  :  delete specified mail */
	if (!net_sout(pop3local->netstream, cmd, (int)EM_SAFE_STRLEN(cmd))) {
		EM_DEBUG_EXCEPTION("net_sout failed...");
		err = EMAIL_ERROR_CONNECTION_BROKEN;		/* EMAIL_ERROR_UNKNOWN; */
		goto FINISH_OFF;
	}

	/*  receive response */
	if (!(p = net_getline(pop3local->netstream))) {
		EM_DEBUG_EXCEPTION("net_getline failed...");
		err = EMAIL_ERROR_INVALID_RESPONSE;
		goto FINISH_OFF;
	}

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG("[POP3] <<< %s", p);
#endif

	if (*p == '-') {		/*  '-ERR' */
		if (cmd_type == POP3_CMD_NOOP)
			err = EMAIL_ERROR_POP3_NOOP_FAILURE;
		goto FINISH_OFF;
	}

	if (*p != '+') {		/*  '+OK' ... */
		err = EMAIL_ERROR_INVALID_RESPONSE;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE(p);

	if (err_code)
		*err_code = err;

	return ret;
}

int emcore_get_mail_contact_info_with_update(char *multi_user_name, email_mail_contact_info_t *contact_info, char *full_address, int mail_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("contact_info[%p], full_address[%s], mail_id[%d], err_code[%p]", contact_info, full_address, mail_id, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	ADDRESS *addr = NULL;
	char *address = NULL;
	char *temp_emailaddr = NULL;
	char *alias = NULL;
	int is_searched = false;
	int address_length = 0;
	char *email_address = NULL;
	int contact_name_len = 0;
	char temp_string[1024] = { 0 , };
	int is_saved = 0;
	char *contact_display_name = NULL;
	char *contact_display_name_from_contact_info = NULL;
	int contact_display_name_len = 0;
	int i = 0;
	int contact_name_buffer_size = 0;
	char *contact_name = NULL;

	if (!contact_info) {
		EM_DEBUG_EXCEPTION("contact_info[%p]", contact_info);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!full_address) {
		full_address = "";
		address_length = 0;
		temp_emailaddr = NULL;
	} else {
		address_length = 2 * EM_SAFE_STRLEN(full_address);
		temp_emailaddr = (char  *)calloc(1, address_length);
	}

	em_skip_whitespace(full_address , &address);
	EM_DEBUG_LOG_SEC("address[address][%s]", address);


	/*  ',' -> "%2C" */
	gchar **tokens = g_strsplit(address, ", ", -1);
	char *p = g_strjoinv("%2C", tokens);

	g_strfreev(tokens);

	/*  ';' -> ',' */
	while (p && p[i] != '\0') {
		if (p[i] == ';')
			p[i] = ',';
		i++;
	}
	EM_DEBUG_LOG_SEC("  2  converted address %s ", p);

	rfc822_parse_adrlist(&addr, p, NULL);

	EM_SAFE_FREE(p);
	EM_DEBUG_LOG_SEC("  3  full_address  %s ", full_address);

	if (!addr) {
		EM_DEBUG_EXCEPTION("rfc822_parse_adrlist failed...");
		err = EMAIL_ERROR_INVALID_ADDRESS;
		goto FINISH_OFF;
	}

	contact_name_buffer_size = address_length;
	contact_name = (char*)em_malloc(contact_name_buffer_size);

	if (!contact_name) {
		EM_DEBUG_EXCEPTION("Memory allocation error!");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	while (addr != NULL) {
		if (addr->mailbox && addr->host) {
			if (!strncmp(addr->mailbox, "UNEXPECTED_DATA_AFTER_ADDRESS", strlen("UNEXPECTED_DATA_AFTER_ADDRESS")) ||
				!strncmp(addr->mailbox, "INVALID_ADDRESS", strlen("INVALID_ADDRESS")) ||
				!strncmp(addr->host, ".SYNTAX-ERROR.", strlen(".SYNTAX-ERROR."))) {
				EM_DEBUG_LOG("Invalid address ");
				addr = addr->next;
				continue;
			}
		} else {
			EM_DEBUG_LOG("Error in parsing..! ");
			addr = addr->next;
			continue;
		}

		EM_SAFE_FREE(email_address);
		email_address = g_strdup_printf("%s@%s", addr->mailbox ? addr->mailbox  :  "", addr->host ? addr->host  :  "");

		EM_DEBUG_LOG(" addr->personal[%s]", addr->personal);
		EM_DEBUG_LOG_SEC(" email_address[%s]", email_address);

		is_searched = false;
		EM_DEBUG_LOG(" >>>>> emcore_get_mail_contact_info - 10");

		err = emcore_get_mail_display_name(multi_user_name, email_address,
                                            &contact_display_name_from_contact_info);
		if (err == EMAIL_ERROR_NONE) {
			contact_display_name = contact_display_name_from_contact_info;

			EM_DEBUG_LOG_SEC(">>> contact_name[%s]", contact_display_name);
			/*  Make display name string */
			is_searched = true;

			if (mail_id == 0 || (contact_name_len == 0)) {	/*  save only the first address information - 09-SEP-2010 */
				contact_display_name_len = EM_SAFE_STRLEN(contact_display_name);
				if (contact_name_len + contact_display_name_len >= contact_name_buffer_size) {	/*  re-alloc memory */
					char *temp = contact_name;
					contact_name_buffer_size += contact_name_buffer_size;
					contact_name = (char  *)calloc(1, contact_name_buffer_size);
					if (contact_name == NULL) {
						EM_DEBUG_EXCEPTION("Memory allocation failed.");
						EM_SAFE_FREE(temp);
						goto FINISH_OFF;
					}
					snprintf(contact_name, contact_name_buffer_size, "%s", temp);
					EM_SAFE_FREE(temp);
				}

				/* snprintf(temp_string, sizeof(temp_string), "%c%d%c%s <%s>%c", start_text_ascii, contact_index, start_text_ascii, contact_display_name, email_address, end_text_ascii); */
				if (addr->next == NULL) {
					snprintf(temp_string, sizeof(temp_string), "\"%s\" <%s>", contact_display_name, email_address);
				} else {
					snprintf(temp_string, sizeof(temp_string), "\"%s\" <%s>, ", contact_display_name, email_address);
				}

				contact_display_name_len = EM_SAFE_STRLEN(temp_string);
				if (contact_name_len + contact_display_name_len >= contact_name_buffer_size) {	/*  re-alloc memory */
					char *temp = contact_name;
					contact_name_buffer_size += contact_name_buffer_size;
					contact_name = (char  *)calloc(1, contact_name_buffer_size);
					if (contact_name == NULL) {
						EM_DEBUG_EXCEPTION("Memory allocation failed.");
						EM_SAFE_FREE(temp);
						err = EMAIL_ERROR_OUT_OF_MEMORY;
						goto FINISH_OFF;
					}
					snprintf(contact_name, contact_name_buffer_size, "%s", temp);
					EM_SAFE_FREE(temp);
				}
				snprintf(contact_name + contact_name_len, contact_name_buffer_size - contact_name_len, "%s", temp_string);
				contact_name_len += contact_display_name_len;
				EM_DEBUG_LOG_SEC("new contact_name >>>>> %s ", contact_name);
			}
		} else {
			EM_DEBUG_LOG("emcore_get_mail_display_name - Not found contact record(if err is 203) or error [%d]", err);
		}

		/*  if contact doesn't exist, use alias or email address as display name */
		if (addr->personal != NULL) {
			/*  "%2C" -> ',' */
			tokens = g_strsplit(addr->personal, "%2C", -1);

			EM_SAFE_FREE(addr->personal);

			addr->personal = g_strjoinv(", ", tokens);

			g_strfreev(tokens);
			/* contact_info->contact_name = EM_SAFE_STRDUP(addr->personal); */
			alias = addr->personal;
		} else {
			/* alias = addr->mailbox ? addr->mailbox  :  ""; */
			alias = email_address;
		}
		contact_info->alias = EM_SAFE_STRDUP(alias);

		if (!is_searched) {
			contact_display_name = alias;
			contact_info->contact_id = -1;		/*  NOTE :  This is valid only if there is only one address. */
			contact_info->storage_type = -1;

			/*  Make display name string */
			EM_DEBUG_LOG_SEC("contact_display_name : [%s]", contact_display_name);
			EM_DEBUG_LOG_SEC("email_address : [%s]", email_address);

			/*  if mail_id is 0, return only contact info without saving contact info to DB */
			if (mail_id == 0 || (contact_name_len == 0))		 {	/*  save only the first address information - 09-SEP-2010 */
				/* snprintf(temp_string, sizeof(temp_string), "%c%d%c%s <%s>%c", start_text_ascii, contact_index, start_text_ascii, contact_display_name, email_address, end_text_ascii); */
				if (addr->next == NULL) {
					snprintf(temp_string, sizeof(temp_string), "\"%s\" <%s>", contact_display_name, email_address);
				} else {
					snprintf(temp_string, sizeof(temp_string), "\"%s\" <%s>, ", contact_display_name, email_address);
				}
				EM_DEBUG_LOG_SEC("temp_string[%s]", temp_string);

				contact_display_name_len = EM_SAFE_STRLEN(temp_string);
				if (contact_name_len + contact_display_name_len >= contact_name_buffer_size) {	/*  re-alloc memory */
					char *temp = contact_name;
					contact_name_buffer_size += contact_name_buffer_size;
					contact_name = (char  *)calloc(1, contact_name_buffer_size);
					if (contact_name == NULL) {
						EM_DEBUG_EXCEPTION("Memory allocation failed.");
						EM_SAFE_FREE(temp);
						err = EMAIL_ERROR_OUT_OF_MEMORY;
						goto FINISH_OFF;
					}
					snprintf(contact_name, contact_name_buffer_size, "%s", temp);
					EM_SAFE_FREE(temp);
				}

				snprintf(contact_name + contact_name_len, contact_name_buffer_size - contact_name_len, "%s", temp_string);
				contact_name_len += contact_display_name_len;
				EM_DEBUG_LOG_SEC("new contact_name >>>>> %s ", contact_name);
			}
		}

		if (temp_emailaddr && email_address) {
			if (mail_id == 0) {	/*  if mail_id is 0, return only contact info without saving contact info to DB */
				/* snprintf(temp_emailaddr, 400, "%s", contact_info->email_address); */
				EM_SAFE_STRCAT(temp_emailaddr, email_address);
				if (addr->next != NULL)
					EM_SAFE_STRCAT(temp_emailaddr, ", ");
				EM_DEBUG_LOG_SEC(">>>> TEMP EMail Address [ %s ] ", temp_emailaddr);
			} else {	/*  save only the first address information - 09-SEP-2010 */
				if (is_saved == 0) {
					is_saved = 1;
					/* snprintf(temp_emailaddr, 400, "%s", contact_info->email_address); */
					EM_SAFE_STRCAT(temp_emailaddr, email_address);
					/*
					if (addr->next != NULL)
						EM_SAFE_STRCAT(temp_emailaddr, ", ");
					*/
					EM_DEBUG_LOG_SEC(">>>> TEMP EMail Address [ %s ] ", temp_emailaddr);
				}
			}
		}

		EM_SAFE_FREE(contact_display_name_from_contact_info);
		/*  next address */
		addr = addr->next;
	} /*  while (addr != NULL) */

	if (temp_emailaddr) {
		EM_DEBUG_LOG_SEC(">>>> TEMPEMAIL ADDR [ %s ] ", temp_emailaddr);
		contact_info->email_address = temp_emailaddr;
		temp_emailaddr = NULL;
	}


	contact_info->contact_name = g_strdup(contact_name); /*prevent 40020*/

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(email_address);
	EM_SAFE_FREE(address);
	EM_SAFE_FREE(temp_emailaddr);
	EM_SAFE_FREE(contact_name);
	EM_SAFE_FREE(contact_display_name_from_contact_info);

	if (err_code != NULL)
		*err_code = err;

	return ret;
}

int emcore_free_contact_info(email_mail_contact_info_t *contact_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("contact_info[%p], err_code[%p]", contact_info, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (!contact_info) {
		EM_DEBUG_EXCEPTION("contact_info[%p]", contact_info);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	EM_SAFE_FREE(contact_info->contact_name);
	EM_SAFE_FREE(contact_info->email_address);
	EM_SAFE_FREE(contact_info->alias);

	contact_info->storage_type = -1;
	contact_info->contact_id = -1;

	ret = true;

FINISH_OFF:
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

int emcore_sync_contact_info(char *multi_user_name, int mail_id, int *err_code)
{
	EM_PROFILE_BEGIN(emCoreMailContactSync);
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	emstorage_mail_tbl_t *mail = NULL;

	email_mail_contact_info_t contact_info_from;
	email_mail_contact_info_t contact_info_to;
	email_mail_contact_info_t contact_info_cc;
	email_mail_contact_info_t contact_info_bcc;

	EM_DEBUG_LOG("mail_id[%d], err_code[%p]", mail_id, err_code);

	memset(&contact_info_from, 0x00, sizeof(email_mail_contact_info_t));
	memset(&contact_info_to, 0x00, sizeof(email_mail_contact_info_t));
	memset(&contact_info_cc, 0x00, sizeof(email_mail_contact_info_t));
	memset(&contact_info_bcc, 0x00, sizeof(email_mail_contact_info_t));

	if (!emstorage_get_mail_by_id(multi_user_name, mail_id, &mail, true, &err) || !mail) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (mail->full_address_from != NULL) {
		if (!emcore_get_mail_contact_info_with_update(multi_user_name, &contact_info_from, mail->full_address_from, mail_id, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_mail_contact_info failed [%d]", err);
		}
	}

	if (mail->full_address_to != NULL) {
		if (!emcore_get_mail_contact_info_with_update(multi_user_name, &contact_info_to, mail->full_address_to, mail_id, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_mail_contact_info failed [%d]", err);
		}
	}

	if (mail->full_address_cc != NULL) {
		if (!emcore_get_mail_contact_info_with_update(multi_user_name, &contact_info_cc, mail->full_address_cc, mail_id, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_mail_contact_info failed [%d]", err);
		}
	}

	if (mail->full_address_bcc != NULL) {
		if (!emcore_get_mail_contact_info_with_update(multi_user_name, &contact_info_bcc, mail->full_address_bcc, mail_id, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_mail_contact_info failed [%d]", err);
		}
	}

	EM_SAFE_FREE(mail->email_address_sender);
	mail->email_address_sender = contact_info_from.email_address;
	contact_info_from.contact_name = NULL;
	contact_info_from.email_address = NULL;
	EM_SAFE_FREE(mail->email_address_recipient);
	if (mail->full_address_to != NULL) {
		mail->email_address_recipient = contact_info_to.email_address;
		contact_info_to.contact_name = NULL;
		contact_info_to.email_address = NULL;
	} else if (mail->full_address_cc != NULL) {
		mail->email_address_recipient = contact_info_cc.email_address;
		contact_info_cc.contact_name = NULL;
		contact_info_cc.email_address = NULL;
	} else if (mail->full_address_bcc != NULL) {
		mail->email_address_recipient = contact_info_bcc.email_address;
		contact_info_bcc.contact_name  = NULL;
		contact_info_bcc.email_address = NULL;
	}

	/*  Update DB */
	if (!emstorage_change_mail_field(multi_user_name, mail_id, UPDATE_ALL_CONTACT_INFO, mail, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (mail != NULL)
		emstorage_free_mail(&mail, 1, NULL);

	emcore_free_contact_info(&contact_info_from, NULL);
	emcore_free_contact_info(&contact_info_to, NULL);
	emcore_free_contact_info(&contact_info_cc, NULL);
	emcore_free_contact_info(&contact_info_bcc, NULL);

	if (err_code != NULL)
		*err_code = err;

	EM_PROFILE_END(emCoreMailContactSync);
	return ret;
}

/*  1. parsing  :  alias and address */
/*  2. sync with contact */
/*  3. make glist of address info */
static int emcore_sync_address_info(char *multi_user_name, email_address_type_t address_type, char *full_address, GList **address_info_list, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("address type[%d], address_info_list[%p], full_address[%p]", address_type, address_info_list, full_address);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int contact_index = -1;
	int is_search = false;
	char *alias = NULL;
	char *address = NULL;
	char *contact_display_name_from_contact_info = NULL;
	char email_address[MAX_EMAIL_ADDRESS_LENGTH];
	email_address_info_t *p_address_info = NULL;
	ADDRESS *addr = NULL;

	if (address_info_list == NULL) {
		EM_DEBUG_EXCEPTION("Invalid param : address_info_list is NULL");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/*  Parsing */
	address = EM_SAFE_STRDUP(full_address);

	/*  ',' -> "%2C" */
	gchar **tokens = g_strsplit(address, ", ", -1);
	char *p = g_strjoinv("%2C", tokens);
	int i = 0;

	g_strfreev(tokens);

	/*  ';' -> ',' */
	while (p && p[i] != '\0') {
		if (p[i] == ';')
			p[i] = ',';
		i++;
	}

	rfc822_parse_adrlist(&addr, p, NULL);

	EM_SAFE_FREE(p);

	if (!addr) {
		EM_DEBUG_EXCEPTION("rfc822_parse_adrlist failed...");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/*  Get a contact name */
 	while (addr != NULL) {
		if (addr->mailbox && addr->host) {
			if (!strcmp(addr->mailbox , "UNEXPECTED_DATA_AFTER_ADDRESS") || !strcmp(addr->mailbox , "INVALID_ADDRESS") || !strcmp(addr->host , ".SYNTAX-ERROR.")) {
				EM_DEBUG_LOG("Invalid address ");
				addr = addr->next;
				continue;
			}
		} else {
			EM_DEBUG_LOG("Error in parsing..! ");
			addr = addr->next;
			continue;
		}

		/*   set display name  */
		/*    1) contact name */
		/*    2) alias (if a alias in an original mail doesn't exist, this field is set with email address */
		/*    3) email address	 */

		if (!(p_address_info = (email_address_info_t *)malloc(sizeof(email_address_info_t)))) {
			EM_DEBUG_EXCEPTION("malloc failed...");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		memset(p_address_info, 0x00, sizeof(email_address_info_t));

		SNPRINTF(email_address, MAX_EMAIL_ADDRESS_LENGTH, "%s@%s", addr->mailbox ? addr->mailbox  :  "", addr->host ? addr->host  :  "");

		EM_DEBUG_LOG_SEC("Search a contact  :  address[%s]", email_address);

		is_search = false;
		error = emcore_get_mail_display_name(multi_user_name, email_address,
                                            &contact_display_name_from_contact_info);
		if (error == EMAIL_ERROR_NONE) {
			EM_DEBUG_LOG_SEC(">>> contact display name[%s]", contact_display_name_from_contact_info);
			is_search = true;
		} else if (error != EMAIL_ERROR_DATA_NOT_FOUND)
			EM_DEBUG_EXCEPTION("emcore_get_mail_display_name - Not found contact record(if err is -203) or error [%d]", error);

		if (is_search == true) {
			p_address_info->contact_id = contact_index;
			p_address_info->storage_type = -1;
			p_address_info->display_name = contact_display_name_from_contact_info;
			EM_DEBUG_LOG_SEC("display_name from contact[%s]", p_address_info->display_name);
		} else {
			/*  if contact doesn't exist, use alias or email address as display name */
			if (addr->personal != NULL) {
				/*  "%2C" -> ',' */
				tokens = g_strsplit(addr->personal, "%2C", -1);

				EM_SAFE_FREE(addr->personal);

				addr->personal = g_strjoinv(", ", tokens);

				g_strfreev(tokens);
	 			alias = addr->personal;
			} else {
	 			alias = NULL;
			}
			p_address_info->contact_id = -1;
			p_address_info->storage_type = -1;
			/*  Use an alias or an email address as a display name */
			if (alias == NULL)
				p_address_info->display_name = EM_SAFE_STRDUP(email_address);
			else
				p_address_info->display_name = EM_SAFE_STRDUP(alias);

			EM_DEBUG_LOG_SEC("display_name from email [%s]", p_address_info->display_name);
		}

		p_address_info->address = EM_SAFE_STRDUP(email_address);
		p_address_info->address_type = address_type;

		EM_DEBUG_LOG_SEC("email address[%s]", p_address_info->address);

		*address_info_list = g_list_append(*address_info_list, p_address_info);
		p_address_info = NULL;

		EM_DEBUG_LOG_DEV("after append");

 		alias = NULL;

		EM_DEBUG_LOG_DEV("next address[%p]", addr->next);

		/*  next address */
		addr = addr->next;
	}

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(address);

	if (addr) {
		mail_free_address(&addr);
	}

	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

static gint address_compare(gconstpointer a, gconstpointer b)
{
	EM_DEBUG_FUNC_BEGIN();
	email_sender_list_t *recipients_list1 = (email_sender_list_t *)a;
	email_sender_list_t *recipients_list2 = (email_sender_list_t *)b;

	EM_DEBUG_FUNC_END();
	return strcmp(recipients_list1->address, recipients_list2->address);
}

INTERNAL_FUNC GList *emcore_get_recipients_list(char *multi_user_name, GList *old_recipients_list, char *full_address, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int i = 0, err = EMAIL_ERROR_NONE;
	int is_search = false;
	char *address = NULL;
	char email_address[MAX_EMAIL_ADDRESS_LENGTH];
	char *display_name = NULL;
	char *alias = NULL;
	ADDRESS *addr = NULL;
	GList *new_recipients_list = old_recipients_list;
	GList *recipients_list;

	email_sender_list_t *temp_recipients_list = NULL;
	email_sender_list_t *old_recipients_list_t = NULL;

	if (full_address == NULL || EM_SAFE_STRLEN(full_address) == 0) {
		EM_DEBUG_EXCEPTION("Invalid param : full_address NULL or empty");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	address = EM_SAFE_STRDUP(full_address);

	gchar **tokens = g_strsplit(address, ", ", -1);
	char *p = g_strjoinv("%2C", tokens);

	g_strfreev(tokens);

	while (p && p[i] != '\0') {
		if (p[i] == ';')
			p[i] = ',';
		i++;
	}

	rfc822_parse_adrlist(&addr, p, NULL);

	EM_SAFE_FREE(p);

	if (!addr) {
		EM_DEBUG_EXCEPTION("rfc822_parse_adrlist failed...");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	while (addr != NULL) {
		if (addr->mailbox && addr->host) {
			if (!strcmp(addr->mailbox , "UNEXPECTED_DATA_AFTER_ADDRESS") || !strcmp(addr->mailbox , "INVALID_ADDRESS") || !strcmp(addr->host , ".SYNTAX-ERROR.")) {
				EM_DEBUG_LOG("Invalid address ");
				addr = addr->next;
				continue;
			}
		} else {
			EM_DEBUG_LOG("Error in parsing..! ");
			addr = addr->next;
			continue;
		}

		if ((temp_recipients_list = g_new0(email_sender_list_t, 1)) == NULL) {
			EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}


		SNPRINTF(email_address, MAX_EMAIL_ADDRESS_LENGTH, "%s@%s", addr->mailbox ? addr->mailbox : "", addr->host ? addr->host : "");

		EM_DEBUG_LOG_SEC("Search a contact : address[%s]", email_address);

		err = emcore_get_mail_display_name(multi_user_name, email_address, &display_name);
		if (err == EMAIL_ERROR_NONE) {
			EM_DEBUG_LOG_SEC(">>> contact display name[%s]", display_name);
			is_search = true;
		} else {
			EM_DEBUG_LOG("emcore_get_mail_display_name - Not found contact record(if err is -203) or error [%d]", err);
 		}

		if (is_search) {
			temp_recipients_list->display_name = display_name;
			EM_DEBUG_LOG_SEC("display_name from contact[%s]", temp_recipients_list->display_name);
		} else {
			if (addr->personal != NULL) {
				tokens = g_strsplit(addr->personal, "%2C", -1);
				EM_SAFE_FREE(addr->personal);
				addr->personal = g_strjoinv(", ", tokens);
				g_strfreev(tokens);
				alias = addr->personal;
			} else {
				alias = NULL;
			}

			if (alias == NULL)
				temp_recipients_list->display_name = EM_SAFE_STRDUP(email_address);
			else
				temp_recipients_list->display_name = EM_SAFE_STRDUP(alias);

			EM_DEBUG_LOG_SEC("display_name from contact[%s]", temp_recipients_list->display_name);
		}

		temp_recipients_list->address = EM_SAFE_STRDUP(email_address);
		EM_DEBUG_LOG_SEC("email address[%s]", temp_recipients_list->address);

		EM_SAFE_FREE(display_name);
		EM_DEBUG_LOG("next address[%p]", addr->next);

		recipients_list = g_list_first(new_recipients_list);
		while (recipients_list != NULL) {
			old_recipients_list_t = (email_sender_list_t *)recipients_list->data;
			if (!EM_SAFE_STRCMP(old_recipients_list_t->address, temp_recipients_list->address)) {
				old_recipients_list_t->total_count = old_recipients_list_t->total_count + 1;
				g_free(temp_recipients_list);
				goto FINISH_OFF;
			}
			recipients_list = g_list_next(recipients_list);
		}

		new_recipients_list = g_list_insert_sorted(new_recipients_list, temp_recipients_list, address_compare);

		g_free(temp_recipients_list);
		temp_recipients_list = NULL;

		alias = NULL;
		addr = addr->next;
	}

FINISH_OFF:

	EM_SAFE_FREE(address);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return new_recipients_list;
}

INTERNAL_FUNC int emcore_get_mail_address_info_list(char *multi_user_name, int mail_id, email_address_info_list_t **address_info_list, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], address_info_list[%p]", mail_id, address_info_list);

	int ret = false, err = EMAIL_ERROR_NONE;
	int failed = true;

	emstorage_mail_tbl_t *mail = NULL;
	email_address_info_list_t *p_address_info_list = NULL;

	if (mail_id <= 0 || !address_info_list) {
		EM_DEBUG_EXCEPTION("mail_id[%d], address_info_list[%p]", mail_id, address_info_list);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* get mail from mail table */
	if (!emstorage_get_mail_by_id(multi_user_name, mail_id, &mail, true, &err) || !mail) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!(p_address_info_list = (email_address_info_list_t *)malloc(sizeof(email_address_info_list_t)))) {
		EM_DEBUG_EXCEPTION("malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memset(p_address_info_list, 0x00, sizeof(email_address_info_list_t));

	if ((err = emcore_connect_contacts_service(multi_user_name)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_connect_contacts_service error : [%d]", err);
		goto FINISH_OFF;
	}

	if (mail->full_address_from && emcore_sync_address_info(multi_user_name, EMAIL_ADDRESS_TYPE_FROM, mail->full_address_from, &p_address_info_list->from, &err))
		failed = false;
	if (mail->full_address_to && emcore_sync_address_info(multi_user_name, EMAIL_ADDRESS_TYPE_TO, mail->full_address_to, &p_address_info_list->to, &err))
		failed = false;
	if (mail->full_address_cc && emcore_sync_address_info(multi_user_name, EMAIL_ADDRESS_TYPE_CC, mail->full_address_cc, &p_address_info_list->cc, &err))
		failed = false;
	if (mail->full_address_bcc && emcore_sync_address_info(multi_user_name, EMAIL_ADDRESS_TYPE_BCC, mail->full_address_bcc, &p_address_info_list->bcc, &err))
		failed = false;

	if ((err = emcore_disconnect_contacts_service(multi_user_name)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_disconnect_contacts_service failed : [%d]", err);
	}

	if (failed == false)
		ret = true;

FINISH_OFF:
	if (ret == true)
		*address_info_list = p_address_info_list;
	else if (p_address_info_list != NULL)
		emstorage_free_address_info_list(&p_address_info_list);

	emstorage_free_mail(&mail, 1, NULL);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

/* description
 *    get a mail data
 * arguments
 *    input_mail_id     : [in]  mail id
 *    output_mail_data  : [out] double pointer to hold mail data.
 * return
 *    succeed  :  EMAIL_ERROR_NONE
 *    fail     :  error code
 */
INTERNAL_FUNC int emcore_get_mail_data(char *multi_user_name, int input_mail_id, email_mail_data_t **output_mail_data)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], output_mail_data[%p]", input_mail_id, output_mail_data);

	int             error = EMAIL_ERROR_NONE;
	int             result_mail_count = 0;
	char            conditional_clause_string[QUERY_SIZE] = { 0, };
	emstorage_mail_tbl_t *result_mail_tbl = NULL;

	if (input_mail_id == 0 || !output_mail_data) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	SNPRINTF(conditional_clause_string, QUERY_SIZE, "WHERE mail_id = %d", input_mail_id);

	if (!emstorage_query_mail_tbl(multi_user_name, conditional_clause_string, true, &result_mail_tbl, &result_mail_count, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_tbl falied [%d]", error);
		goto FINISH_OFF;
	}

	if (!em_convert_mail_tbl_to_mail_data(result_mail_tbl, 1, output_mail_data, &error)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_tbl_to_mail_data falied [%d]", error);
		goto FINISH_OFF;
	}

FINISH_OFF:
	if (result_mail_tbl)
		emstorage_free_mail(&result_mail_tbl, result_mail_count, NULL);

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}


int emcore_check_drm(emstorage_attachment_tbl_t *input_attachment_tb_data)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = 0;
	EM_DEBUG_FUNC_END();
	return ret;
}


/* description
 *    get mail attachment from local mailbox
 * arguments
 *    mailbox  :  server mailbox
 *    mail_id  :  mai id to own attachment
 *    attachment  :  the number string to be downloaded
 *    callback  :  function callback. if NULL, ignored.
 * return
 *     succeed  :  1
 *     fail  :  0
 */
INTERNAL_FUNC int emcore_get_attachment_info(char *multi_user_name, int attachment_id, email_attachment_data_t **attachment, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_id[%d], attachment[%p], err_code[%p]", attachment_id, attachment, err_code);

	if (attachment == NULL || attachment_id == 0) {
		EM_DEBUG_EXCEPTION("attachment_id[%d], attachment[%p]", attachment_id, attachment);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	emstorage_attachment_tbl_t *attachment_tbl = NULL;

	/* get attachment from attachment tbl */
	if (!emstorage_get_attachment(multi_user_name, attachment_id, &attachment_tbl, true, &err) || !attachment_tbl) {
		EM_DEBUG_EXCEPTION("emstorage_get_attachment failed [%d]", err);
		goto FINISH_OFF;
	}

	*attachment = em_malloc(sizeof(email_attachment_data_t));
	if (!*attachment) {
		EM_DEBUG_EXCEPTION("malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	(*attachment)->attachment_id         = attachment_id;
	(*attachment)->attachment_name       = attachment_tbl->attachment_name; attachment_tbl->attachment_name = NULL;
	(*attachment)->content_id            = attachment_tbl->content_id; attachment_tbl->content_id = NULL;
	(*attachment)->attachment_size       = attachment_tbl->attachment_size;
	(*attachment)->save_status           = attachment_tbl->attachment_save_status;
	(*attachment)->attachment_path       = attachment_tbl->attachment_path; attachment_tbl->attachment_path = NULL;
	(*attachment)->drm_status            = attachment_tbl->attachment_drm_type;
	(*attachment)->inline_content_status = attachment_tbl->attachment_inline_content_status;
	(*attachment)->attachment_mime_type  = attachment_tbl->attachment_mime_type; attachment_tbl->attachment_mime_type = NULL;

	ret = true;

FINISH_OFF:
	if (attachment_tbl)
		emstorage_free_attachment(&attachment_tbl, 1, NULL);

	if (err_code)
		*err_code = err;

	return ret;
}

/* description
 *    get mail attachment
 * arguments
 *    input_mail_id           :  mail id to own attachment
 *    output_attachment_data  :  result attahchment data
 *    output_attachment_count :  result attahchment count
 * return
 *     succeed : EMAIL_ERROR_NONE
 *     fail    : error code
 */
INTERNAL_FUNC int emcore_get_attachment_data_list(char *multi_user_name, int input_mail_id, email_attachment_data_t **output_attachment_data, int *output_attachment_count)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], output_attachment_data[%p], output_attachment_count[%p]", input_mail_id, output_attachment_data, output_attachment_count);

	if (input_mail_id == 0 || output_attachment_data == NULL || output_attachment_count == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	int                        i = 0;
	int                        err = EMAIL_ERROR_NONE;
	int                        attachment_tbl_count = 0;
	emstorage_attachment_tbl_t *attachment_tbl_list = NULL;
	email_attachment_data_t     *temp_attachment_data = NULL;

	/* get attachment from attachment tbl */
	if ((err = emstorage_get_attachment_list(multi_user_name, input_mail_id, true, &attachment_tbl_list, &attachment_tbl_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);
		goto FINISH_OFF;
	}

	if (attachment_tbl_count) {
		EM_DEBUG_LOG("attachment count %d", attachment_tbl_count);

		*output_attachment_data = em_malloc(sizeof(email_attachment_data_t) * attachment_tbl_count);

		if (*output_attachment_data == NULL) {
			EM_DEBUG_EXCEPTION("em_mallocfailed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		for (i = 0; i < attachment_tbl_count; i++) {
			temp_attachment_data = (*output_attachment_data) + i;

			temp_attachment_data->attachment_id         = attachment_tbl_list[i].attachment_id;
			temp_attachment_data->attachment_name       = attachment_tbl_list[i].attachment_name; attachment_tbl_list[i].attachment_name = NULL;
			temp_attachment_data->attachment_path       = attachment_tbl_list[i].attachment_path; attachment_tbl_list[i].attachment_path = NULL;
			temp_attachment_data->content_id            = attachment_tbl_list[i].content_id; attachment_tbl_list[i].content_id = NULL;
			temp_attachment_data->attachment_size       = attachment_tbl_list[i].attachment_size;
			temp_attachment_data->mail_id               = attachment_tbl_list[i].mail_id;
			temp_attachment_data->account_id            = attachment_tbl_list[i].account_id;
			temp_attachment_data->mailbox_id            = attachment_tbl_list[i].mailbox_id;
			temp_attachment_data->save_status           = attachment_tbl_list[i].attachment_save_status;
			temp_attachment_data->drm_status            = attachment_tbl_list[i].attachment_drm_type;
			temp_attachment_data->inline_content_status = attachment_tbl_list[i].attachment_inline_content_status;
			temp_attachment_data->attachment_mime_type  = attachment_tbl_list[i].attachment_mime_type; attachment_tbl_list[i].attachment_mime_type = NULL;
		}
	}

FINISH_OFF:

	*output_attachment_count = attachment_tbl_count;

	if (attachment_tbl_list)
		emstorage_free_attachment(&attachment_tbl_list, attachment_tbl_count, NULL);

	return err;
}

INTERNAL_FUNC int emcore_gmime_download_attachment(char *multi_user_name, int mail_id, int nth,
		int cancellable, int event_handle, int auto_download, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], nth[%d], err_code[%p]", mail_id, nth, err_code);

	int err = EMAIL_ERROR_NONE;

	if (mail_id < 1) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;

		if (err_code != NULL)
			*err_code = err;

		if (!auto_download)
			emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_FAIL, mail_id, 0, nth, err);
		return FALSE;
	}

	int ret = FALSE;
	MAILSTREAM *stream = NULL;
	BODY *mbody = NULL;
	emstorage_mail_tbl_t *mail = NULL;
	emstorage_attachment_tbl_t *attachment = NULL;
	emstorage_attachment_tbl_t *attachment_list = NULL;

	struct attachment_info *ai = NULL;
	struct _m_content_info *cnt_info = NULL;
	void *tmp_stream = NULL;
	char *server_uid = NULL;
	char move_buf[512], path_buf[512];

	int msg_no = 0;
	int account_id = 0;
	int server_mbox_id = 0;

	int current_attachment_no = 0;
	int attachment_count = 0;
	int decoded_attachment_size = 0;
	int i = 0;

	GMimeMessage *message = NULL;

	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	/*  get mail from mail table. */
	if (!emstorage_get_mail_by_id(multi_user_name, mail_id, &mail, true, &err) || !mail) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!mail->server_mail_status) {
		EM_DEBUG_EXCEPTION("not synchronous mail...");
		err = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER;
		goto FINISH_OFF;
	}

	if (nth == 0) {	/* download all attachments, nth starts from 1, not zero */
		if ((err = emstorage_get_attachment_list(multi_user_name, mail_id, true, &attachment_list, &attachment_count)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);
			goto FINISH_OFF;
		}
	} else {	/* download only nth attachment */
		attachment_count = 1;
		if (!emstorage_get_attachment_nth(multi_user_name, mail_id, nth, &attachment_list, true, &err) || !attachment_list) {
			EM_DEBUG_EXCEPTION("emstorage_get_attachment_nth failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	account_id = mail->account_id;
	server_uid = EM_SAFE_STRDUP(mail->server_mail_id);
	server_mbox_id = mail->mailbox_id;

	if (attachment_count == 1 && attachment_list) {
		if (!auto_download) {
			if (!emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_START, mail_id, attachment_list[0].attachment_name, nth, 0))
				EM_DEBUG_EXCEPTION("emcore_notify_network_event[ NOTI_DOWNLOAD_ATTACH_START] Failed >>>>");
		}
	}

	/* open mail server. */
	if (!auto_download) {
		if (!emcore_connect_to_remote_mailbox(multi_user_name,
												account_id,
												server_mbox_id,
												true,
												(void **)&tmp_stream,
												&err) || !tmp_stream) {
			EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
			if (err == EMAIL_ERROR_NO_SUCH_HOST)
				err = EMAIL_ERROR_CONNECTION_FAILURE;
			goto FINISH_OFF;
		}

		stream = (MAILSTREAM *)tmp_stream;
	} else {
		MAILSTREAM **mstream = NULL;
		mstream = emcore_get_recv_stream(multi_user_name, account_id, server_mbox_id, &err);

		if (!mstream) {
			EM_DEBUG_EXCEPTION("emcore_get_recv_stream failed [%d]", err);
			goto FINISH_OFF;
		}
		stream = (MAILSTREAM *)*mstream;
	}

	for (i = 0; i < attachment_count; i++) {
		EM_DEBUG_LOG(" >>>>>> Attachment Downloading [%d / %d] start", i+1, attachment_count);

		attachment = attachment_list + i;

		if (nth == 0) 						/*  download all attachments, nth starts from 1, not zero */
			current_attachment_no = i + 1;	/*  attachment no */
		else 								/*  download only nth attachment */
			current_attachment_no = nth;	/*  attachment no */

		if (cancellable)
			FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		/*free cnt_info before reusing*/
		if (cnt_info) {
			emcore_free_content_info(cnt_info);
			EM_SAFE_FREE(cnt_info);
		}

		if (!(cnt_info = em_malloc(sizeof(struct _m_content_info)))) {
			EM_DEBUG_EXCEPTION("malloc failed...");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		cnt_info->grab_type = GRAB_TYPE_ATTACHMENT;     /*  attachment */
		cnt_info->file_no   = current_attachment_no;    /*  attachment no */

		/* set sparep(member of BODY) memory free function. */
		mail_parameters(stream, SET_FREEBODYSPAREP, emcore_free_body_sparep);

		if (cancellable)
			FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		msg_no = server_uid ? atoi(server_uid) : 0;

		/*  get body structure. */
		/*  don't free mbody because mbody is freed in closing mail_stream. */
		if ((!stream) || emcore_get_body_structure(stream, msg_no, &mbody, &err) < 0) {
			EM_DEBUG_EXCEPTION("emcore_get_body_structure failed [%d]", err);
			goto FINISH_OFF;
		}

		if (cancellable)
			FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		if (!emcore_gmime_construct_mime_part_with_bodystructure(mbody, &message, "1", NULL)) {
			EM_DEBUG_EXCEPTION("emcore_gmime_construct_mime_part_with_bodystructure failed");
			goto FINISH_OFF;
		}

		if (g_strrstr(mail->full_address_from, "mmsc.plusnet.pl") != NULL ||
			g_strrstr(mail->full_address_from, "mms.t-mobile.pl") != NULL) {
			cnt_info->attachment_only = 1;
		}

		/* fill up section list */
		g_mime_message_foreach(message, emcore_gmime_get_attachment_section_foreach_cb, (gpointer)cnt_info);
		cnt_info->file_no = current_attachment_no;

		/* FETCH sections */
		if (!emcore_gmime_fetch_imap_attachment_section(stream, mail_id, msg_no,
				current_attachment_no, cnt_info, message, auto_download, event_handle, &err)) {
			EM_DEBUG_EXCEPTION("emcore_gmime_fetch_imap_attachment_section failed");
			goto FINISH_OFF;
		}

		/* decode contents */
		g_mime_message_foreach(message, emcore_gmime_imap_parse_full_foreach_cb, (gpointer)cnt_info);

		if (message) {
			g_object_unref(message);
			message = NULL;
		}

		if (cancellable)
			FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		/* select target attachment information. */
		for (ai = cnt_info->file ; ai; ai = ai->next) {
			EM_DEBUG_LOG_SEC("[in loop] save[%s] name[%s] no[%d]", ai->save, ai->name, cnt_info->file_no);

			if (--cnt_info->file_no == 0)
				break;
		}
		EM_DEBUG_LOG("selected cnt_info->file_no = %d, ai = %p", cnt_info->file_no, ai);

		if (cnt_info->file_no != 0 || ai == NULL) {
			EM_DEBUG_EXCEPTION("invalid attachment sequence...");
			err = EMAIL_ERROR_INVALID_ATTACHMENT;
			goto FINISH_OFF;
		}

		/* rename temporary file to real file. */
        memset(move_buf, 0x00, sizeof(move_buf));
        memset(path_buf, 0x00, sizeof(path_buf));

		if (!emstorage_create_dir(multi_user_name, account_id, mail_id, current_attachment_no, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_get_save_name(multi_user_name, account_id, mail_id, current_attachment_no, ai->name, move_buf, path_buf, sizeof(path_buf), &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(ai->save, move_buf, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			err = EMAIL_ERROR_INVALID_ATTACHMENT_SAVE_NAME;
			goto FINISH_OFF;
		}

		emcore_get_file_size(move_buf, &decoded_attachment_size, NULL);
		EM_DEBUG_LOG("decoded_attachment_size [%d]", decoded_attachment_size);
		attachment->attachment_size        = decoded_attachment_size;
		attachment->attachment_path        = EM_SAFE_STRDUP(path_buf);
		attachment->content_id             = EM_SAFE_STRDUP(ai->content_id);
		attachment->attachment_save_status = 1;

		/*  update attachment information. */
		if (!emstorage_change_attachment_field(multi_user_name, mail_id, UPDATE_SAVENAME, attachment, true, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_change_attachment_field failed [%d]", err);
			/*  delete created file. */
			g_remove(move_buf);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG(">>>>>> Attachment Downloading [%d/%d] completed", i+1, attachment_count);
	}

	ret = TRUE;

FINISH_OFF:

	if (!auto_download) {
		if (ret == TRUE)
			emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_FINISH, mail_id, NULL, nth, 0);
		else {
			if (err != EMAIL_ERROR_CANCELLED)
				emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_FAIL, mail_id, NULL, nth, err);
		}
	}

	EM_SAFE_FREE(server_uid);

	if (message) {
		g_object_unref(message);
		message = NULL;
	}

	if (stream && !auto_download) {
		stream = mail_close(stream);
	}

	if (attachment_list)
		emstorage_free_attachment(&attachment_list, attachment_count, NULL);

	if (cnt_info) {
		emcore_free_content_info(cnt_info);
		EM_SAFE_FREE(cnt_info);
	}

	if (mail)
		emstorage_free_mail(&mail, 1, NULL);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}


#ifdef __ATTACHMENT_OPTI__
INTERNAL_FUNC int emcore_download_attachment_bulk(int account_id, int mail_id, int nth, int event_handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], nth[%d], err_code[%p]", account_id, mail_id, nth, err_code);

	int err = EMAIL_ERROR_NONE; /* Prevent Defect - 25093 */
	int ret = false;
	int status = EMAIL_DOWNLOAD_FAIL;
	MAILSTREAM *stream = NULL;
	emstorage_mail_tbl_t *mail = NULL;
	emstorage_attachment_tbl_t *attachment = NULL;
	void *tmp_stream = NULL;
	char *s_uid = NULL, *server_mbox = NULL, move_buf[512], path_buf[512];
	emstorage_attachment_tbl_t *attachment_list = NULL;
	int current_attachment_no = 0;
	int attachment_count_to_be_downloaded = 0;		/*  how many attachments should be downloaded */
	int i = 0;
	char *savefile = NULL;
	int dec_len = 0;
	int uid = 0;
#ifdef SUPPORT_EXTERNAL_MEMORY
	int iActualSize = 0;
	int is_add = 0;
	char dirName[512];
	int bIs_empty = 0;
	int bIs_full = 0;
	int bIsAdd_to_mmc = false;
	int is_on_mmc = false;
	email_file_list *pFileListMMc = NULL;
	email_file_list *pFileList = NULL;
#endif /*  SUPPORT_EXTERNAL_MEMORY */
	int decoded_attachment_size = 0;

	memset(move_buf, 0x00, 512);
	memset(path_buf, 0x00, 512);

	/* CID FIX 31230 */
	if (mail_id < 1 || !nth) {
		EM_DEBUG_EXCEPTION("account_id[%d], mail_id[%d], nth[%p]", account_id, mail_id, nth);

		err = EMAIL_ERROR_INVALID_PARAM;

		if (err_code != NULL)
			*err_code = err;

		if (nth)
			attachment_no = atoi(nth);

		emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_FAIL, mail_id, 0, attachment_no, err); 	/*  090525, kwangryul.baek */

		return false;
	}

	FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	only_body_download = false;

	attachment_no = atoi(nth);

	if (attachment_no == 0) {
		/*  download all attachments, nth starts from 1, not zero */
		/*  get attachment list from db */
		attachment_count_to_be_downloaded = EMAIL_ATTACHMENT_MAX_COUNT;
		if ((err = emstorage_get_attachment_list(multi_user_name, mail_id, true, &attachment_list, &attachment_count_to_be_downloaded)) != EMAIL_ERROR_NONE || !attachment_list) {
			EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);
			goto FINISH_OFF;
		}
	} else {	/*  download only nth attachment */
		attachment_count_to_be_downloaded = 1;
		if (!emstorage_get_attachment_nth(multi_user_name, mail_id, attachment_no, &attachment_list, true, &err) || !attachment_list) {
			EM_DEBUG_EXCEPTION("emstorage_get_attachment_nth failed [%d]", err);
			goto FINISH_OFF;
		}
	}


	FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	/*  get mail from mail table. */
	if (!emstorage_get_mail_by_id(multi_user_name, mail_id, &mail, true, &err) || !mail) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	/* if (!mail->server_mail_yn || !mail->text_download_yn) {*/ /*  faizan.h@samsung.com */

	FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	account_id = mail->account_id;
	s_uid = EM_SAFE_STRDUP(mail->server_mail_id); mail->server_mail_id = NULL;
	server_mbox = EM_SAFE_STRDUP(mail->mailbox); mail->server_mailbox_name = NULL;



	/*  open mail server. */
	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											account_id,
											server_mbox,
											true,
											(void **)&tmp_stream,
											&err) || !tmp_stream) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);

		status = EMAIL_DOWNLOAD_CONNECTION_FAIL;
		goto FINISH_OFF;
	}

	stream = (MAILSTREAM *)tmp_stream;

	FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	for (i = 0; i < attachment_count_to_be_downloaded; i++)	{
		EM_DEBUG_LOG(" >>>>>> Attachment Downloading [%d / %d] start", i+1, attachment_count_to_be_downloaded);

		attachment = attachment_list + i;
		if (attachment_no == 0)	{
			/*  download all attachments, nth starts from 1, not zero */
			current_attachment_no = i + 1;		/*  attachment no */
		} else {
			/*  download only nth attachment */
			current_attachment_no = attachment_no;		/*  attachment no */
		}

		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		_imap4_received_body_size = 0;
		_imap4_last_notified_body_size = 0;
		_imap4_total_body_size = 0; 				/*  This will be assigned in imap_mail_write_body_to_file() */
		_imap4_download_noti_interval_value = 0;	/*  This will be assigned in imap_mail_write_body_to_file() */


		EM_SAFE_FREE(savefile);
		if (!emcore_get_temp_file_name(&savefile, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_temp_file_name failed [%d]", err);

			if (err_code != NULL)
				*err_code = err;
			goto FINISH_OFF;
		}

		if (s_uid)
			uid = atoi(s_uid);

		EM_DEBUG_LOG("uid [%d]", uid);

		if (!imap_mail_write_body_to_file(stream, account_id, mail_id, attachment_no, savefile, uid, attachment->section, attachment->encoding, &dec_len, NULL, &err)) {
			EM_DEBUG_EXCEPTION("imap_mail_write_body_to_file failed [%d]", err);
			if (err_code != NULL)
				*err_code = err;
			goto FINISH_OFF;
		}

#ifdef SUPPORT_EXTERNAL_MEMORY
		iActualSize = emcore_get_actual_mail_size(cnt_info->text.plain, cnt_info->text.html, cnt_info->file, &err);
		if (!emstorage_mail_check_free_space(iActualSize, &bIs_full, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_mail_check_free_space failed [%d]", err);
			goto FINISH_OFF;
		}

		if (bIs_full) {
			/* If external memory not present, return error */
			if (PS_MMC_REMOVED == emstorage_get_mmc_status()) {
				err = EMAIL_ERROR_MAIL_MEMORY_FULL;
				goto FINISH_OFF;
			}
			bIsAdd_to_mmc = true;
		}
#endif /*  SUPPORT_EXTERNAL_MEMORY */

		if (!emstorage_create_dir(multi_user_name, account_id, mail_id, attachment_no, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_get_save_name(account_id, mail_id, attachment_no, attachment->name, move_buf, path_buf, sizeof(path_buf), &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(savefile, move_buf, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}

		emcore_get_file_size(move_buf, &decoded_attachment_size, NULL);
		EM_DEBUG_LOG("decoded_attachment_size [%d]", decoded_attachment_size);
		attachment->attachment_size        = decoded_attachment_size;
		attachment->attachment_path        = EM_SAFE_STRDUP(path_buf);
		attachment->attachment_save_status = 1;
		/*  update attachment information. */
		if (!emstorage_change_attachment_field(multi_user_name, mail_id, UPDATE_SAVENAME, attachment, true, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_change_attachment_field failed [%d]", err);
			goto FINISH_OFF;
		}

		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		EM_DEBUG_LOG(" >>>>>> Attachment Downloading [%d / %d] completed", i+1, attachment_count_to_be_downloaded);
	}

	ret = true;

	FINISH_OFF:

	EM_SAFE_FREE(savefile);

	if (stream) {
		stream = mail_close(stream);
	}

	if (attachment_list)
		emstorage_free_attachment(&attachment_list, attachment_count_to_be_downloaded, NULL);

	if (mail)
		emstorage_free_mail(&mail, 1, NULL);

	EM_SAFE_FREE(s_uid);

	EM_SAFE_FREE(server_mbox);

	if (ret == true)
		emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_FINISH, mail_id, NULL, attachment_no, 0);
	else if (err != EMAIL_ERROR_CANCELLED)
		emcore_notify_network_event(NOTI_DOWNLOAD_ATTACH_FAIL, mail_id, NULL, attachment_no, err);

	if (err_code != NULL)
		*err_code = err;

	return ret;
}
#endif

INTERNAL_FUNC int emcore_gmime_download_body_sections(char *multi_user_name,
														void *mail_stream,
														int account_id,
														int mail_id,
														int with_attach,
														int limited_size,
														int event_handle,
														int cancellable,
														int auto_download,
														int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_stream[%p], account_id[%d], mail_id[%d], "
			"with_attach[%d], event_handle [%d]", mail_stream, account_id, mail_id,
			with_attach, event_handle);

	int ret = FALSE;
	int err = EMAIL_ERROR_NONE;
	int b_with_attach = with_attach;
	int need_update_attachment = 0;

	BODY *mbody = NULL;
	MAILSTREAM *stream = NULL;

	email_account_t *ref_account = NULL;
	emstorage_mail_tbl_t *mail = NULL;
	emstorage_attachment_tbl_t attachment = {0,};
	emstorage_attachment_tbl_t *attch_info = NULL;

	struct _m_content_info *cnt_info = NULL;
	struct attachment_info *ai = NULL;

	char *s_uid = NULL;
	char move_buf[512] = {0};
    char path_buf[512] = {0};

	int msgno = 0;
	int attachment_num = 1;
	int local_attachment_count = 0;
	int local_inline_content_count = 0;

	char html_body[MAX_PATH] = {0,};

	emcore_uid_list *uid_list = NULL;

	GMimeMessage *message1 = NULL;

	if (mail_id < 1) {
		EM_DEBUG_EXCEPTION("mail_stream[%p], account_id[%d], mail_id[%d],"
				"with_attach[%d]", mail_stream, account_id, mail_id, with_attach);

		err = EMAIL_ERROR_INVALID_PARAM;
		if (err_code != NULL)
			*err_code = err;

		if (!auto_download)
			emcore_notify_network_event(NOTI_DOWNLOAD_BODY_FAIL, mail_id, NULL, event_handle, err);
		return ret;
	}

	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	/* looking for mail by mail_id in DB*/
	if (!emstorage_get_mail_by_id(multi_user_name, mail_id, &mail, true, &err) || !mail) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	/* if mail is downloaded */
	if (mail->body_download_status & EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED) {
		/* after entering viewer, the mail has been just downloaded */
		if (!auto_download) {
			if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_START, mail_id,
					"dummy-file", mail->mail_size, 0))
				EM_DEBUG_EXCEPTION("emcore_notify_network_event[ NOTI_DOWNLOAD_BODY_START] error >>>>");
			else
				EM_DEBUG_LOG("NOTI_DOWNLOAD_BODY_START notified (%d / %d)", 0, mail->mail_size);
		}

		err = EMAIL_ERROR_NONE; //EMAIL_ERROR_MAIL_IS_ALREADY_DOWNLOADED;
		ret = TRUE;
		goto FINISH_OFF;
	} else if (!mail->body_download_status)
		need_update_attachment = 1;

	s_uid = EM_SAFE_STRDUP(mail->server_mail_id);

	if (!(ref_account = emcore_get_account_reference(multi_user_name, account_id, false))) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	/*  open mail server. */
	if (!mail_stream) {
#if 0
		MAILSTREAM *tmp_stream = NULL;
		if (!emcore_connect_to_remote_mailbox(multi_user_name,
												account_id,
												mail->mailbox_id,
												true,
												(void **)&tmp_stream,
												&err) || !tmp_stream) {
			EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
			goto FINISH_OFF;
		}
		stream = (MAILSTREAM *)tmp_stream;
#endif

		MAILSTREAM **mstream = NULL;
		mstream = emcore_get_recv_stream(multi_user_name, account_id, mail->mailbox_id, &err);

		if (!mstream) {
			EM_DEBUG_EXCEPTION("emcore_get_recv_stream failed [%d]", err);
			goto FINISH_OFF;
		}
		stream = (MAILSTREAM *)*mstream;
	} else
		stream = (MAILSTREAM *)mail_stream;

	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	if (!(cnt_info = em_malloc(sizeof(struct _m_content_info)))) {
		EM_DEBUG_EXCEPTION("em_mallocfailed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	/* POP3 */
	/* in POP3 case, both text and attachment are downloaded in this call. */
	if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_POP3) {
		int fd = 0;
		int pop3_total_body_size = 0;
		int pop3_downloaded_size = 0;
		char sock_buf[1024] = {0,};
		char *tmp_path = NULL;
		email_internal_mailbox_t mailbox = {0,};

		cnt_info->grab_type = GRAB_TYPE_TEXT | GRAB_TYPE_ATTACHMENT;
		mailbox.account_id = account_id;

		/* download all uids from server. */
		if (!emcore_download_uid_all(multi_user_name, stream, &mailbox, &uid_list, NULL, NULL, 0, EM_CORE_GET_UIDS_FOR_NO_DELETE, &err)) {
			EM_DEBUG_EXCEPTION("emcore_download_uid_all failed [%d]", err);
			goto FINISH_OFF;
		}

		/* get mesg number to be related to last download mail from uid list file */
		if (!emcore_get_msgno(uid_list, s_uid, &msgno, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_msgno failed [%d]", err);
			err = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER;
			goto FINISH_OFF;
		}

		if (cancellable)
			FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		_pop3_received_body_size = 0;
		_pop3_total_body_size = 0;
		_pop3_last_notified_body_size = 0;
		_pop3_receiving_mail_id = mail_id;

		/* send read mail commnad. */
		if (!emcore_mail_cmd_read_mail_pop3(stream, msgno, limited_size, &pop3_downloaded_size, &pop3_total_body_size, &err)) {
			EM_DEBUG_EXCEPTION("emcore_mail_cmd_read_mail_pop3 failed [%d]", err);
			goto FINISH_OFF;
		}

		_pop3_total_body_size = pop3_total_body_size;

		if (!auto_download) {
			if (!emcore_notify_network_event(NOTI_DOWNLOAD_BODY_START, mail_id, "dummy-file", _pop3_total_body_size, 0))
				EM_DEBUG_EXCEPTION(" emcore_notify_network_event[ NOTI_DOWNLOAD_BODY_START] failed >>>> ");
			else
				EM_DEBUG_LOG("NOTI_DOWNLOAD_BODY_START notified (%d / %d)", 0, _pop3_total_body_size);
		}

		if (cancellable)
			FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		tmp_path = emcore_mime_get_save_file_name(&err);
		EM_DEBUG_LOG("tmp_path[%s]", tmp_path);

		err = em_open(tmp_path, O_WRONLY | O_CREAT, 0644, &fd);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_open error [%d]", err);
			EM_SAFE_FREE(tmp_path);
			goto FINISH_OFF;
		}

		while (emcore_mime_get_line_from_sock((void *)stream, sock_buf, 1024, &err)) {
			if (write(fd, sock_buf, EM_SAFE_STRLEN(sock_buf)) != EM_SAFE_STRLEN(sock_buf)) {
				EM_DEBUG_EXCEPTION("write failed");
			}

			if (cancellable) {
				int type = 0;
				if (!emcore_check_event_thread_status(&type, event_handle)) {
					EM_DEBUG_LOG("CANCELED EVENT: type [%d]", type);
					err = EMAIL_ERROR_CANCELLED;
					EM_SAFE_CLOSE(fd);
					g_remove(tmp_path);
					EM_SAFE_FREE(tmp_path);
					goto FINISH_OFF;
				}
			}
		}
		EM_SAFE_CLOSE(fd);

		emcore_gmime_pop3_parse_mime(tmp_path, cnt_info, &err);

		g_remove(tmp_path);
		EM_SAFE_FREE(tmp_path);

		if (cancellable)
			FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		if (limited_size != NO_LIMITATION && limited_size < pop3_total_body_size) {
	 		mail->body_download_status = (mail->body_download_status & ~0x00000003) | EMAIL_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED;
		} else {
			mail->body_download_status = (mail->body_download_status & ~0x00000003) | EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;
			b_with_attach = 1;
		}
	}
	/* IMAP */
	else if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
		int total_mail_size = 0;

		if (b_with_attach > 0 || need_update_attachment)
			cnt_info->grab_type = GRAB_TYPE_TEXT | GRAB_TYPE_ATTACHMENT;
		else
			cnt_info->grab_type = GRAB_TYPE_TEXT;

		int uid = s_uid ? atoi(s_uid) : 0;

		/*  set sparep(member of BODY) memory free function  */
		mail_parameters(stream, SET_FREEBODYSPAREP, emcore_free_body_sparep);

		/*  get body strucutre. */
		/*  don't free mbody because mbody is freed in closing mail_stream. */
		if (emcore_get_body_structure(stream, uid, &mbody, &err) < 0 || (mbody == NULL)) {
			EM_DEBUG_EXCEPTION("emcore_get_body_structure failed [%d]", err);
			err = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER;
			goto FINISH_OFF;
		}

		if (cancellable)
			FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		/* construct GMimeMessage from bodystructure*/
		if (!emcore_gmime_construct_mime_part_with_bodystructure(mbody, &message1, "1", &total_mail_size)) {
			EM_DEBUG_EXCEPTION("emcore_gmime_construct_mime_part_with_bodystructure failed");
			err = EMAIL_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}

		if (g_strrstr(mail->full_address_from, "mmsc.plusnet.pl") != NULL ||
			g_strrstr(mail->full_address_from, "mms.t-mobile.pl") != NULL) {
			cnt_info->attachment_only = 1;
		}

		/* fill up cnt_info */
		g_mime_message_foreach(message1, emcore_gmime_imap_parse_bodystructure_foreach_cb, (gpointer)cnt_info);

		/* fill up section list */
		g_mime_message_foreach(message1, emcore_gmime_get_body_sections_foreach_cb, (gpointer)cnt_info);

		/* FETCH sections and set content to message */
		if (!emcore_gmime_fetch_imap_body_sections(stream, uid, mail_id,
												cnt_info, message1, event_handle,
												auto_download, &err)) {
			EM_DEBUG_EXCEPTION("emcore_gmime_get_imap_sections failed");
			goto FINISH_OFF;
		}

		/* decoding contents */
		g_mime_message_foreach(message1, emcore_gmime_imap_parse_full_foreach_cb, (gpointer)cnt_info);

		/* Get mime entity */
		if (cnt_info->content_type == 1)
			g_mime_message_foreach(message1, emcore_gmime_get_mime_entity_cb, (gpointer)cnt_info);

		/* free resources */
		if (message1) {
			g_object_unref(message1);
			message1 = NULL;
		}

		mail->body_download_status = (mail->body_download_status & ~0x00000003) | EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;
	}

	/* text/plain */
	if (cnt_info->text.plain) {
		char *charset_plain_text = NULL;
		char *file_content = NULL;
		int content_size = 0;

		EM_DEBUG_LOG("cnt_info->text.plain [%s]", cnt_info->text.plain);

		if (emcore_get_content_from_file(cnt_info->text.plain, &file_content, &content_size) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_content_from_file failed");
		}

		if (file_content && content_size > 0) {
			char escape = 0x1b;
			char detector[25] = {0,};
			snprintf(detector, sizeof(detector), "%c$B", escape);
			if (g_strrstr(cnt_info->text.plain_charset, "UTF-8") && g_strrstr(file_content, detector)) {
				cnt_info->text.plain_charset = "ISO-2022-JP";
			}
		}

		EM_SAFE_FREE(file_content);

		if (cnt_info->text.plain_charset)
			charset_plain_text = cnt_info->text.plain_charset;
		else {
			if (mail->default_charset) {
				charset_plain_text = mail->default_charset;
			} else {
				charset_plain_text = "UTF-8";
			}
		}
		EM_DEBUG_LOG("PLAIN DEFAULT CHARSET : %s", charset_plain_text);

        memset(move_buf, 0x00, sizeof(move_buf));
        memset(path_buf, 0x00, sizeof(path_buf));

		if (!emstorage_create_dir(multi_user_name, account_id, mail_id, 0, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_get_save_name(multi_user_name, account_id, mail_id, 0,
									charset_plain_text, move_buf, path_buf,
									sizeof(path_buf), &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(cnt_info->text.plain, move_buf, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_SAFE_FREE(mail->file_path_plain);
		mail->file_path_plain = EM_SAFE_STRDUP(path_buf);
		EM_DEBUG_LOG_SEC("mail->file_path_plain [%s]", mail->file_path_plain);
	}

	/* text/html */
	if (cnt_info->text.html) {
        memset(move_buf, 0x00, sizeof(move_buf));
        memset(path_buf, 0x00, sizeof(path_buf));

		if (!emstorage_create_dir(multi_user_name, account_id, mail_id, 0, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (cnt_info->text.html_charset != NULL) {
			memcpy(html_body, cnt_info->text.html_charset, EM_SAFE_STRLEN(cnt_info->text.html_charset));
			strcat(html_body, HTML_EXTENSION_STRING);
		} else if (cnt_info->text.plain_charset != NULL) {
			memcpy(html_body, cnt_info->text.plain_charset, EM_SAFE_STRLEN(cnt_info->text.plain_charset));
			strcat(html_body, HTML_EXTENSION_STRING);
		} else {
			memcpy(html_body, "UTF-8.htm", strlen("UTF-8.htm"));
		}

		if (!emstorage_get_save_name(multi_user_name, account_id, mail_id, 0, html_body, move_buf, path_buf, sizeof(path_buf), &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(cnt_info->text.html, move_buf, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_SAFE_FREE(mail->file_path_html);
		mail->file_path_html = EM_SAFE_STRDUP(path_buf);
	}

	/* mime_entity */
	if (cnt_info->text.mime_entity) {
        memset(move_buf, 0x00, sizeof(move_buf));
        memset(path_buf, 0x00, sizeof(path_buf));

		if (!emstorage_get_save_name(multi_user_name, account_id, mail_id,
									0, "mime_entity", move_buf, path_buf,
									sizeof(path_buf), &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(cnt_info->text.mime_entity, move_buf, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_SAFE_FREE(mail->file_path_mime_entity);
		mail->file_path_mime_entity = EM_SAFE_STRDUP(path_buf);
	}

	/* Update local_preview_text */
	if ((err = emcore_get_preview_text_from_file(multi_user_name,
                                                    mail->file_path_plain,
                                                    mail->file_path_html,
                                                    MAX_PREVIEW_TEXT_LENGTH,
                                                    &mail->preview_text)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_preview_text_from_file error [%d]", err);
	}

	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	attachment.account_id             = mail->account_id;
	attachment.mail_id                = mail->mail_id;
	attachment.mailbox_id             = mail->mailbox_id;
	attachment.attachment_save_status = 0;

	/* attachments */
	if (need_update_attachment) {
		int total_attachment_size;
		int attachment_num_from_cnt_info;
		int inline_attachment_num_from_cnt_info;

		if ((err = emcore_update_attachment_except_inline(multi_user_name,
                                                cnt_info,
                                                mail->account_id,
                                                mail->mail_id,
                                                mail->mailbox_id,
                                                &total_attachment_size,
                                                &attachment_num_from_cnt_info,
                                                &inline_attachment_num_from_cnt_info)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_update_attachment_except_inline failed : [%d]", err);
			goto FINISH_OFF;
		}
	}

	ai = cnt_info->file;

	while (ai) {
		local_attachment_count++;

		if (b_with_attach == 0) {
			attachment_num++;
			if (ai->save) g_remove(ai->save);
			ai = ai->next;
			continue;
		}

		attachment.attachment_id                    = attachment_num;
		attachment.attachment_size                  = ai->size;
		attachment.attachment_path                  = ai->save;
		attachment.content_id                       = ai->content_id;
		attachment.attachment_name                  = ai->name;
		attachment.attachment_drm_type              = ai->drm;
		attachment.attachment_inline_content_status = 0;
		attachment.attachment_mime_type             = ai->attachment_mime_type;
		attachment.attachment_save_status           = 1;

		EM_DEBUG_LOG("attachment.attachment_id[%d]", attachment.attachment_id);
		EM_DEBUG_LOG("attachment.attachment_size[%d]", attachment.attachment_size);
		EM_DEBUG_LOG_SEC("attachment.attachment_path[%s]", attachment.attachment_path);
		EM_DEBUG_LOG("attachment.content_id[%s]", attachment.content_id);
		EM_DEBUG_LOG_SEC("attachment.attachment_name[%s]", attachment.attachment_name);
		EM_DEBUG_LOG("attachment.attachment_drm_type[%d]", attachment.attachment_drm_type);
		EM_DEBUG_LOG("attachment.attachment_save_status[%d]", attachment.attachment_save_status);
		EM_DEBUG_LOG("attachment.attachment_inline_content_status[%d]", attachment.attachment_inline_content_status);

		if (!ai->save) {
			attachment_num++;
			ai = ai->next;
			continue;
		}

        memset(move_buf, 0x00, sizeof(move_buf));
        memset(path_buf, 0x00, sizeof(path_buf));

		if (!emstorage_create_dir(multi_user_name, account_id, mail_id, attachment_num, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_get_save_name(multi_user_name, account_id, mail_id, attachment_num, attachment.attachment_name, move_buf, path_buf, sizeof(path_buf), &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(ai->save, move_buf, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);

			/*  delete all created files. */
			if (!emstorage_get_save_name(multi_user_name, account_id, mail_id, 0, NULL, move_buf, path_buf, sizeof(path_buf), NULL)) {
				EM_DEBUG_EXCEPTION("emstorage_get_save_name failed...");
				/* goto FINISH_OFF; */
			}

			if (!emstorage_delete_dir(move_buf, NULL)) {
				EM_DEBUG_EXCEPTION("emstorage_delete_dir failed...");
				/* goto FINISH_OFF; */
			}

			goto FINISH_OFF;
		}

		EM_SAFE_FREE(ai->save);
		ai->save = EM_SAFE_STRDUP(path_buf);
		attachment.attachment_path = ai->save;

		/*  Get attachment details  */
		if (!emstorage_get_attachment_nth(multi_user_name, mail_id, attachment.attachment_id, &attch_info, true, &err) || !attch_info) {
			EM_DEBUG_EXCEPTION("emstorage_get_attachment_nth failed [%d]", err);

			if (err == EMAIL_ERROR_ATTACHMENT_NOT_FOUND) { /*  save only attachment file. */
				if (!emstorage_add_attachment(multi_user_name, &attachment, 0, false, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_add_attachment failed [%d]", err);

					if (attch_info)
						emstorage_free_attachment(&attch_info, 1, NULL);

					/*  delete all created files. */
					if (!emstorage_get_save_name(multi_user_name, account_id, mail_id, 0, NULL, move_buf, path_buf, sizeof(path_buf), &err)) {
						EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
						goto FINISH_OFF;
					}

					if (!emstorage_delete_dir(move_buf, &err)) {
						EM_DEBUG_EXCEPTION("emstorage_delete_dir failed [%d]", err);
						goto FINISH_OFF;
					}

					/*  ROLLBACK TRANSACTION; */
					emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);

					goto FINISH_OFF;
				}
			}
		} else {
			EM_DEBUG_LOG("Attachment info already exists...!");
			/* Update attachment size */
			EM_DEBUG_LOG("attachment_size [%d], ai->size [%d]", attch_info->attachment_size, ai->size);
			EM_SAFE_FREE(attch_info->attachment_name);
			EM_SAFE_FREE(attch_info->attachment_path);
			EM_SAFE_FREE(attch_info->content_id);
			EM_SAFE_FREE(attch_info->attachment_mime_type);

			attch_info->attachment_size = ai->size;
			attch_info->attachment_save_status = attachment.attachment_save_status;
			attch_info->attachment_path = EM_SAFE_STRDUP(attachment.attachment_path);
			attch_info->content_id = EM_SAFE_STRDUP(attachment.content_id);
			attch_info->attachment_name = EM_SAFE_STRDUP(attachment.attachment_name);
			attch_info->attachment_drm_type = attachment.attachment_drm_type;
			attch_info->attachment_inline_content_status = 0;
			attch_info->attachment_mime_type = EM_SAFE_STRDUP(attachment.attachment_mime_type);

			if (!emstorage_update_attachment(multi_user_name, attch_info, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_update_attachment failed [%d]", err);
				emstorage_free_attachment(&attch_info, 1, NULL); /*prevent 17956*/
				goto FINISH_OFF;
			}
		}

		if (attch_info)
			emstorage_free_attachment(&attch_info, 1, NULL);

		attachment_num++;
		ai = ai->next;
	}

	/* inline attachments */
	ai = cnt_info->inline_file;
	while (ai) {
		local_inline_content_count++;

		attachment.attachment_id                    = attachment_num;
		attachment.attachment_size                  = ai->size;
		attachment.attachment_path                  = ai->save;
		attachment.content_id                       = ai->content_id;
		attachment.attachment_name                  = ai->name;
		attachment.attachment_drm_type              = ai->drm;
		attachment.attachment_inline_content_status = 1;
		attachment.attachment_mime_type             = ai->attachment_mime_type;
		attachment.attachment_save_status           = 1;

		EM_DEBUG_LOG("attachment.attachment_id[%d]", attachment.attachment_id);
		EM_DEBUG_LOG("attachment.attachment_size[%d]", attachment.attachment_size);
		EM_DEBUG_LOG_SEC("attachment.attachment_path[%s]", attachment.attachment_path);
		EM_DEBUG_LOG("attachment.content_id[%s]", attachment.content_id);
		EM_DEBUG_LOG_SEC("attachment.attachment_name[%s]", attachment.attachment_name);
		EM_DEBUG_LOG("attachment.attachment_drm_type[%d]", attachment.attachment_drm_type);
		EM_DEBUG_LOG("attachment.attachment_save_status[%d]", attachment.attachment_save_status);
		EM_DEBUG_LOG("attachment.attachment_inline_content_status[%d]", attachment.attachment_inline_content_status);

		if (!ai->save) {
			attachment_num++;
			ai = ai->next;
			continue;
		}

        memset(move_buf, 0x00, sizeof(move_buf));
        memset(path_buf, 0x00, sizeof(path_buf));

		if (!emstorage_create_dir(multi_user_name, account_id, mail_id, 0, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_get_save_name(multi_user_name, account_id, mail_id, 0, attachment.attachment_name, move_buf, path_buf, sizeof(path_buf), &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(ai->save, move_buf, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);

			/*  delete all created files. */
			if (!emstorage_get_save_name(multi_user_name, account_id, mail_id, 0, NULL, move_buf, path_buf, sizeof(path_buf), NULL)) {
				EM_DEBUG_EXCEPTION("emstorage_get_save_name failed...");
				/* goto FINISH_OFF; */
			}

			if (!emstorage_delete_dir(move_buf, NULL)) {
				EM_DEBUG_EXCEPTION("emstorage_delete_dir failed...");
				/* goto FINISH_OFF; */
			}

			goto FINISH_OFF;
		}

		EM_SAFE_FREE(ai->save);
		ai->save = EM_SAFE_STRDUP(path_buf);
		attachment.attachment_path = ai->save;

		/*  Get attachment details  */
		if (!emstorage_get_attachment_nth(multi_user_name, mail_id, attachment.attachment_id, &attch_info, true, &err) || !attch_info) {
			EM_DEBUG_EXCEPTION("emstorage_get_attachment_nth failed [%d]", err);

			if (err == EMAIL_ERROR_ATTACHMENT_NOT_FOUND) { /*  save only attachment file. */
				if (!emstorage_add_attachment(multi_user_name, &attachment, 0, false, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_add_attachment failed [%d]", err);

					if (attch_info)
						emstorage_free_attachment(&attch_info, 1, NULL);

					/*  delete all created files. */
					if (!emstorage_get_save_name(multi_user_name, account_id, mail_id, 0, NULL, move_buf, path_buf, sizeof(path_buf), &err)) {
						EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
						goto FINISH_OFF;
					}

					if (!emstorage_delete_dir(move_buf, &err)) {
						EM_DEBUG_EXCEPTION("emstorage_delete_dir failed [%d]", err);
						goto FINISH_OFF;
					}

					/*  ROLLBACK TRANSACTION; */
					emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
					goto FINISH_OFF;
				}
			}
		} else {
			EM_DEBUG_LOG("Attachment info already exists...!");
			/* Update attachment size */
			EM_DEBUG_LOG("attachment_size [%d], ai->size [%d]", attch_info->attachment_size, ai->size);
			EM_SAFE_FREE(attch_info->attachment_name);
			EM_SAFE_FREE(attch_info->attachment_path);
			EM_SAFE_FREE(attch_info->content_id);
			EM_SAFE_FREE(attch_info->attachment_mime_type);

			attch_info->attachment_size = ai->size;
			attch_info->attachment_save_status = attachment.attachment_save_status;
			attch_info->attachment_path = EM_SAFE_STRDUP(attachment.attachment_path);
			attch_info->content_id = EM_SAFE_STRDUP(attachment.content_id);
			attch_info->attachment_name = EM_SAFE_STRDUP(attachment.attachment_name);
			attch_info->attachment_drm_type = attachment.attachment_drm_type;
			attch_info->attachment_inline_content_status = 1;
			attch_info->attachment_mime_type = EM_SAFE_STRDUP(attachment.attachment_mime_type);

			if (!emstorage_update_attachment(multi_user_name, attch_info, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_update_attachment failed [%d]", err);
				emstorage_free_attachment(&attch_info, 1, NULL); /*prevent 17956*/
				goto FINISH_OFF;
			}
		}

		if (attch_info)
			emstorage_free_attachment(&attch_info, 1, NULL);

		attachment_num++;
		ai = ai->next;
	}

	mail->attachment_count = local_attachment_count;
	mail->inline_content_count = local_inline_content_count;

	/* change mail's information. */
	if (!emstorage_change_mail_field(multi_user_name, mail_id, APPEND_BODY, mail, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL); /*  ROLLBACK TRANSACTION; */
		goto FINISH_OFF;
	}

#ifdef __FEATURE_BODY_SEARCH__
	/* strip html content and save into mail_text_tbl */
	char *stripped_text = NULL;
	if (!emcore_strip_mail_body_from_file(multi_user_name, mail, &stripped_text, &err) || stripped_text == NULL) {
		EM_DEBUG_EXCEPTION("emcore_strip_mail_body_from_file failed [%d]", err);
	}

	emstorage_mail_text_tbl_t *mail_text;
	if (!emstorage_get_mail_text_by_id(multi_user_name, mail_id, &mail_text, true, &err) || !mail_text) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_text_by_id failed [%d]", err);
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL); /*  ROLLBACK TRANSACTION; */
		EM_SAFE_FREE(stripped_text);
		goto FINISH_OFF;
	}

	EM_SAFE_FREE(mail_text->body_text);
	mail_text->body_text = stripped_text;

	if (!emstorage_change_mail_text_field(multi_user_name, mail_id, mail_text, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_change_mail_text_field failed [%d]", err);
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL); /*  ROLLBACK TRANSACTION; */
		emstorage_free_mail_text(&mail_text, 1, NULL); /*prevent 17957*/
		goto FINISH_OFF;
	}

	if (mail_text)
		emstorage_free_mail_text(&mail_text, 1, NULL);
#endif

	EM_DEBUG_LOG_SEC("cnt_info->text.plain [%s], cnt_info->text.html [%s]", cnt_info->text.plain, cnt_info->text.html);

	/*  in pop3 mail case, the mail is deleted from server after being downloaded. */
	if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_POP3) {
#ifdef DELETE_AFTER_DOWNLOADING
		char delmsg[24];

		SNPRINTF(delmsg, sizeof(delmsg), "%d", msg_no);

		if (!ref_account->keep_mails_on_pop_server_after_download) {
			if (!emcore_delete_mails_from_pop3_server(multi_user_name, &mbox, delmsg, &err))
				EM_DEBUG_EXCEPTION("emcore_delete_mails_from_pop3_server failed [%d]", err);
		}
#endif
	}

	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	ret = TRUE;

FINISH_OFF:

	if (ref_account) {
		emcore_free_account(ref_account);
		free(ref_account);
	}

#if 0
	/* note that local stream should be freed here*/
	if (mail_stream == NULL && stream != NULL) {
		stream = mail_close(stream);
	}
#endif

	_pop3_received_body_size = 0;
	_pop3_last_notified_body_size = 0;
	_pop3_total_body_size = 0;
	_pop3_receiving_mail_id = 0;

	EM_SAFE_FREE(s_uid);

	if (cnt_info) {
		emcore_free_content_info(cnt_info);
		free(cnt_info);
	}

	if (mail)
		emstorage_free_mail(&mail, 1, NULL);

	if (message1) {
		g_object_unref(message1);
		message1 = NULL;
	}

	if (!auto_download) {
		if (ret == TRUE)
			emcore_notify_network_event(NOTI_DOWNLOAD_BODY_FINISH, mail_id, NULL, event_handle, 0);
		else
			emcore_notify_network_event(NOTI_DOWNLOAD_BODY_FAIL, mail_id, NULL, event_handle, err);
	}

	if (err_code != NULL)
		*err_code = err;

	return ret;
}


void emcore_mail_copyuid(MAILSTREAM *stream, char *mailbox,
			   unsigned long uidvalidity, SEARCHSET *sourceset,
			   SEARCHSET *destset)
{
	EM_DEBUG_FUNC_BEGIN();
	char  old_server_uid[129];

	EM_DEBUG_LOG_SEC("mailbox name - %s", mailbox);
	EM_DEBUG_LOG("first sequence number source- %ld", sourceset->first);
	EM_DEBUG_LOG("last sequence number last- %ld", sourceset->last);
	EM_DEBUG_LOG("first sequence number dest - %ld", destset->first);
	EM_DEBUG_LOG("last sequence number dest- %ld", sourceset->last);

	/* search for server _mail_id with value sourceset->first and update it with destset->first */
	/* faizan.h@samsung.com */
	memset(old_server_uid, 0x00, 129);
	snprintf(old_server_uid, sizeof(old_server_uid), "%ld", sourceset->first);
	EM_DEBUG_LOG(">>>>> old_server_uid = %s", old_server_uid);

	memset(g_new_server_uid, 0x00, 129);
	snprintf(g_new_server_uid, sizeof(g_new_server_uid),"%ld", destset->first);
	EM_DEBUG_LOG(">>>>> new_server_uid =%s", g_new_server_uid);

	if (!emstorage_update_server_uid(NULL, 0, old_server_uid, g_new_server_uid, NULL)) {
		EM_DEBUG_EXCEPTION("emstorage_update_server_uid falied...");
	}
}

static int emcore_delete_mails_from_remote_server(char *multi_user_name,
													int input_account_id,
													int input_mailbox_id,
													int input_mail_ids[],
													int input_mail_id_count,
													int input_delete_option)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id[%d], input_mailbox_id[%d], input_mail_ids[%p], "
						"input_mail_id_count[%d], input_delete_option [%d]",
						input_account_id, input_mailbox_id, input_mail_ids, input_mail_id_count, input_delete_option);

	int err = EMAIL_ERROR_NONE;
	int i = 0;
	char *noti_param_string = NULL;
	char mail_id_string[10] = { 0, };
	email_account_t *account = NULL;
	emstorage_mail_tbl_t *mail_tbl_data = NULL;
#ifdef 	__FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__
	int bulk_flag = false;
#endif

	if (!(account = emcore_get_account_reference(multi_user_name, input_account_id, false))) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", input_account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!emnetwork_check_network_status(&err)) {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
		goto FINISH_OFF;
	}
	/* don't delete this comment, several threads including event thread call it */
	/* FINISH_OFF_IF_CANCELED; */

	/* Sending Notification */
	noti_param_string = em_malloc(sizeof(char) * 10 * input_mail_id_count);
	if (!noti_param_string) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < input_mail_id_count; i++) {
		memset(mail_id_string, 0, sizeof(mail_id_string));
		SNPRINTF(mail_id_string, sizeof(mail_id_string), "%d,", input_mail_ids[i]);
		EM_SAFE_STRNCAT(noti_param_string, mail_id_string,(sizeof(char) * 10 * input_mail_id_count) - EM_SAFE_STRLEN(noti_param_string) - 1 );
		/* can be optimized by appending sub string with directly pointing on string array kyuho.jo 2011-10-07 */
	}

	if (!emcore_notify_network_event(NOTI_DELETE_MAIL_START, input_account_id, noti_param_string, 0, 0))
		EM_DEBUG_EXCEPTION(" emcore_notify_storage_eventfailed [ NOTI_DELETE_MAIL_START ] >>>> ");

	if (account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
		if (!bulk_flag && !emcore_delete_mails_from_imap4_server(multi_user_name,
																	input_account_id,
																	input_mailbox_id,
																	input_mail_ids,
																	input_mail_id_count,
																	input_delete_option,
																	&err)) {
			EM_DEBUG_EXCEPTION("emcore_delete_mails_from_imap4_server failed [%d]", err);
			if (err == EMAIL_ERROR_IMAP4_STORE_FAILURE)
				err = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER;
			goto FINISH_OFF;
		} else {
			bulk_flag = true;

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
			for (i = 0; i < input_mail_id_count; i++) {
				if (!emcore_delete_auto_download_activity(multi_user_name,
															input_account_id,
															input_mail_ids[i],
															0,
															&err)) {
					EM_DEBUG_EXCEPTION("emcore_delete_auto_download_activity failed [%d]", err);
				}
			}
#endif
		}
	} else if (account->incoming_server_type == EMAIL_SERVER_TYPE_POP3) {
		if (!emcore_delete_mails_from_pop3_server(multi_user_name, account, input_mail_ids, input_mail_id_count)) {
			EM_DEBUG_EXCEPTION("emcore_delete_mails_from_pop3_server falied [%d]", err);
			goto FINISH_OFF;
		}
#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
		else {
			for (i = 0; i < input_mail_id_count; i++) {
				if (!emcore_delete_auto_download_activity(multi_user_name,
															input_account_id,
															input_mail_ids[i],
															0,
															&err)) {
					EM_DEBUG_EXCEPTION("emcore_delete_auto_download_activity failed [%d]", err);
				}
			}
		}
#endif

#ifdef __FEATURE_LOCAL_ACTIVITY__
		else {
			/* Remove local activity */
			emstorage_activity_tbl_t new_activity;
			memset(&new_activity, 0x00, sizeof(emstorage_activity_tbl_t));
			if (from_server == EMAIL_DELETE_FOR_SEND_THREAD) {
				new_activity.activity_type = ACTIVITY_DELETEMAIL_SEND;
				EM_DEBUG_LOG("from_server == EMAIL_DELETE_FOR_SEND_THREAD ");
			} else {
				new_activity.activity_type	= ACTIVITY_DELETEMAIL;
			}

			new_activity.mail_id		= mail->mail_id;
			new_activity.server_mailid	= mail->server_mail_id;
			new_activity.src_mbox		= NULL;
			new_activity.dest_mbox		= NULL;

			if (!emcore_delete_activity(&new_activity, &err)) {
				EM_DEBUG_EXCEPTION(" emcore_delete_activity  failed  - %d ", err);
			}

			/* Fix for issue - Sometimes mail move and immediately followed by mail delete is not reflected on server */
			if (!emstorage_remove_downloaded_mail(multi_user_name,
													input_account_id,
													mail->mailbox_id,
													mail->server_mailbox_name,
													mail->server_mail_id,
													true,
													&err)) {
				EM_DEBUG_LOG("emstorage_remove_downloaded_mail falied [%d]", err);
			}
		}

#endif	/*  __FEATURE_LOCAL_ACTIVITY__ */
	}

	for (i = 0; i < input_mail_id_count; i++) {
		if (!emstorage_get_downloaded_mail(multi_user_name,
											input_mail_ids[i],
											&mail_tbl_data,
											false,
											&err)) {
			/* not server mail */
			EM_DEBUG_EXCEPTION("emstorage_get_downloaded_mail failed [%d]", err);
			continue;
		}

		if (!emstorage_remove_downloaded_mail(multi_user_name,
												mail_tbl_data->account_id,
												mail_tbl_data->mailbox_id,
												mail_tbl_data->server_mailbox_name,
												mail_tbl_data->server_mail_id,
												true,
												&err)) {
			EM_DEBUG_EXCEPTION("emstorage_remove_downloaded_mail feiled : [%d]", err);
		}

		if (mail_tbl_data)
			emstorage_free_mail(&mail_tbl_data, 1, NULL);

		mail_tbl_data = NULL;
	}

FINISH_OFF:

	if (err == EMAIL_ERROR_NONE) {
		if (!emcore_notify_network_event(NOTI_DELETE_MAIL_FINISH,  input_account_id, noti_param_string, 0, 0))
			EM_DEBUG_EXCEPTION(" emcore_notify_network_eventfailed [ NOTI_DELETE_MAIL_FINISH ] >>>> ");
	} else {
		if (!emcore_notify_network_event(NOTI_DELETE_MAIL_FAIL,  input_account_id, noti_param_string, 0, 0))
			EM_DEBUG_EXCEPTION(" emcore_notify_network_eventfailed [ NOTI_DELETE_MAIL_FAIL ] >>>> ");
	}

	if (mail_tbl_data)
		emstorage_free_mail(&mail_tbl_data, 1, NULL);

	EM_SAFE_FREE(noti_param_string);

	if (account) {
		emcore_free_account(account);
		EM_SAFE_FREE(account);
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

int emcore_delete_mail(char *multi_user_name,
						int account_id,
						int mailbox_id,
						int mail_ids[],
						int num,
						int from_server,
						int noti_param_1,
						int noti_param_2,
						int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_id[%d], mail_ids[%p], num[%d], from_server[%d], "
						"noti_param_1 [%d], noti_param_2 [%d], err_code[%p]",
						account_id, mailbox_id, mail_ids, num, from_server, noti_param_1, noti_param_2, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_account_t *account = NULL;

	if (!account_id || !mail_ids || !num) {
		EM_DEBUG_EXCEPTION("account_id[%d], mail_ids[%p], num[%d], from_server[%d]",
							account_id, mail_ids, num, from_server);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(account = emcore_get_account_reference(multi_user_name, account_id, false))) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/* don't delete this comment, several threads including event thread call it */
	/* FINISH_OFF_IF_CANCELED; */

	if (from_server == EMAIL_DELETE_LOCALLY) /* Delete mails from local storage*/ {
		emcore_delete_mails_from_local_storage(multi_user_name,
												account_id,
												mail_ids,
												num,
												noti_param_1,
												noti_param_2,
												err_code);
		emcore_display_unread_in_badge(multi_user_name);
	} else {   /* Delete mails from server*/
		emcore_delete_mails_from_remote_server(multi_user_name,
												account_id,
												mailbox_id,
												mail_ids,
												num,
												from_server);
	}

	ret = true;

FINISH_OFF:

	if (from_server)
		emcore_show_user_message(multi_user_name, account_id, EMAIL_ACTION_DELETE_MAIL, ret == true ? 0  :  err);

	if (account) {
		emcore_free_account(account);
		EM_SAFE_FREE(account);
	}

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("err [%d]", err);

	return ret;
}

email_thread_handle_t *del_thd = NULL;

static void* del_dir(void* arg)
{
	char* buf = (char*) arg;
	int err = EMAIL_ERROR_NONE;
	if (!emstorage_delete_dir(buf, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_delete_dir failed [%d]", err);
	}
	return NULL;
}

static void* free_buf(void* arg)
{
	EM_SAFE_FREE(arg);
	return NULL;
}

int pipefd[2] = {0}; /* defined in main.c */
pthread_mutex_t mu_del_account = PTHREAD_MUTEX_INITIALIZER;

static void* del_exit(void* arg)
{
	emcore_send_signal_for_del_account(EMAIL_SIGNAL_FILE_DELETED);
	EM_DEBUG_LOG("publish all file deleted");
	return NULL;
}

INTERNAL_FUNC int *emcore_init_pipe_for_del_account()
{
	int err = pipe(pipefd);
	if (err < 0) {
		EM_DEBUG_EXCEPTION("pipe error [%d]", errno);
		return NULL;
	}
	return pipefd;
}

INTERNAL_FUNC void emcore_send_signal_for_del_account(int signal)
{
	pthread_mutex_lock(&mu_del_account);
	write(pipefd[1], (char *)&signal, sizeof(signal));
	pthread_mutex_unlock(&mu_del_account);
}

int emcore_delete_all_mails_of_acount(char *multi_user_name, int input_account_id)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d]");

	int   err = EMAIL_ERROR_NONE;
	char* move_buf = NULL;
    char path_buf[512] = {0};

	/* emstorage_delete_mail_by_account is available only locally */
	if (!emstorage_delete_mail_by_account(multi_user_name, input_account_id, false, &err)) {
		if (err != EMAIL_ERROR_MAIL_NOT_FOUND) {
			EM_DEBUG_EXCEPTION("emstorage_delete_mail_by_account failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	if (!emstorage_delete_attachment_all_on_db(multi_user_name, input_account_id, NULL, false, &err)) {
		if (err != EMAIL_ERROR_MAIL_NOT_FOUND) {
			EM_DEBUG_EXCEPTION("emstorage_delete_attachment_all_on_db failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	/*  delete mail contents from filesystem */
	move_buf = em_malloc(512);
	if (!move_buf) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		goto FINISH_OFF;
	}

	if (!emstorage_get_save_name(multi_user_name, input_account_id, 0, 0, NULL, move_buf, path_buf, sizeof(path_buf), &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!del_thd)
		del_thd = em_thread_create(del_exit, NULL);

	em_thread_run(del_thd, del_dir, free_buf, move_buf);

	/*  delete meeting request */
	if (!emstorage_delete_meeting_request(multi_user_name, input_account_id, 0, 0, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_delete_attachment_all_on_db failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_delete_all_mails_of_mailbox(char *multi_user_name,
														int input_account_id,
														int input_mailbox_id,
														int input_mailbox_type,
														int input_from_server,
														int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id[%d] input_mailbox_id[%d] input_from_server[%d] err_code[%p]",
							input_account_id, input_mailbox_id, input_from_server, err_code);

	int   ret = false;
	int   err = EMAIL_ERROR_NONE;
	int  *mail_id_array = NULL;
	int   mail_id_count = 0;
	char  conditional_clause[QUERY_SIZE] = { 0, };

	if (input_mailbox_id <= 0 && input_mailbox_type <= 0) {
		err = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		goto FINISH_OFF;
	}

	/* Delete all mails in specific mailbox */
	SNPRINTF(conditional_clause, QUERY_SIZE, " where ");

	if (input_mailbox_id <= 0 && input_mailbox_type > 0) {
		SNPRINTF(conditional_clause + strlen(conditional_clause),
					QUERY_SIZE - strlen(conditional_clause),
					"mailbox_type = %d ", input_mailbox_type);
	} else if (input_mailbox_id > 0 && input_mailbox_type <= 0) {
		SNPRINTF(conditional_clause + strlen(conditional_clause),
					QUERY_SIZE - strlen(conditional_clause),
					"mailbox_id = %d ", input_mailbox_id);
	} else {
		SNPRINTF(conditional_clause + strlen(conditional_clause),
					QUERY_SIZE - strlen(conditional_clause),
					"mailbox_type = %d and mailbox_id = %d ",
					input_mailbox_type, input_mailbox_id);
	}

	EM_DEBUG_LOG("command : [%s]", conditional_clause);

	err = emstorage_query_mail_id_list(multi_user_name, conditional_clause, false, &mail_id_array, &mail_id_count);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_id_list failed : [%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("emstorage_query_mail_id_list returns [%d]", mail_id_count);

	if (mail_id_count > 0) {
		if (!emcore_delete_mail(multi_user_name,
								input_account_id,
								input_mailbox_id,
								mail_id_array,
								mail_id_count,
								input_from_server,
								EMAIL_DELETED_BY_COMMAND,
								false,
								&err)) {
			EM_DEBUG_EXCEPTION("emcore_delete_mail failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(mail_id_array);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d], err [%d]", ret, err);
	return ret;
}

INTERNAL_FUNC int emcore_delete_mails_from_local_storage(char *multi_user_name,
															int account_id,
															int *mail_ids,
															int num,
															int noti_param_1,
															int noti_param_2,
															int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_ids[%p], num [%d], "
						"noti_param_1 [%d], noti_param_2 [%d], err_code[%p]",
						account_id, mail_ids, num, noti_param_1, noti_param_2, num, err_code);

	int ret = false, err = EMAIL_ERROR_NONE, i;
	emstorage_mail_tbl_t *result_mail_list = NULL;
	char mail_id_string[10], *noti_param_string = NULL;
    char move_buf[512] = {0, };
    char path_buf[512] = {0,};

	/* Getting mail list by using select mail_id [in] */
	if (!emstorage_get_mail_field_by_multiple_mail_id(multi_user_name,
														mail_ids,
														num,
														RETRIEVE_SUMMARY,
														&result_mail_list, false,
														&err) || !result_mail_list) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_field_by_multiple_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Deleting mails by using select mail_id [in] */
	if (!emstorage_delete_multiple_mails(multi_user_name, mail_ids, num, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_delete_multiple_mails failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Sending Notification */
	noti_param_string = em_malloc(sizeof(char) * 10 * num);
	if (!noti_param_string) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < num; i++) {
		memset(mail_id_string, 0, sizeof(mail_id_string));
		SNPRINTF(mail_id_string, sizeof(mail_id_string), "%d,", mail_ids[i]);
		EM_SAFE_STRNCAT(noti_param_string, mail_id_string, (sizeof(char) * 10 * num)- EM_SAFE_STRLEN(noti_param_string) -  1);
		/* can be optimized by appending sub string with directly pointing on string array kyuho.jo 2011-10-07 */
	}

	if (!emcore_notify_storage_event(NOTI_MAIL_DELETE, account_id, noti_param_1, noti_param_string, noti_param_2))
		EM_DEBUG_EXCEPTION(" emcore_notify_storage_eventfailed [ NOTI_MAIL_DELETE_FINISH ] >>>> ");

	/* Updating Thread informations */
	for (i = 0; i < num; i++) {
		if (result_mail_list[i].thread_id != 0) {
			if (!emstorage_update_latest_thread_mail(multi_user_name,
														account_id,
														result_mail_list[i].mailbox_id,
														result_mail_list[i].mailbox_type,
														result_mail_list[i].thread_id,
														NULL,
														0,
														0,
														NOTI_THREAD_ID_CHANGED_BY_MOVE_OR_DELETE,
														false,
														&err)) {
				EM_DEBUG_EXCEPTION("emstorage_update_latest_thread_mail failed [%d]", err);
				goto FINISH_OFF;
			}
		}
	}

	/* Thread information should be updated as soon as possible. */
	if (!emcore_notify_storage_event(NOTI_MAIL_DELETE_FINISH,
									account_id,
									noti_param_1,
									noti_param_string,
									noti_param_2))
		EM_DEBUG_EXCEPTION(" emcore_notify_storage_eventfailed [ NOTI_MAIL_DELETE_FINISH ] >>>> ");

	for (i = 0; i < num; i++) {
		/* Deleting attachments */
		if (!emstorage_delete_all_attachments_of_mail(multi_user_name, result_mail_list[i].mail_id, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_delete_all_attachments_of_mail failed [%d]", err);
			if (err == EMAIL_ERROR_ATTACHMENT_NOT_FOUND)
				err = EMAIL_ERROR_NONE;
		}

		/* Deleting Directories */
		/*  delete mail contents from filesystem */
		if (!emstorage_get_save_name(multi_user_name,
									account_id,
									result_mail_list[i].mail_id,
									0,
									NULL,
									move_buf,
									path_buf,
									sizeof(path_buf),
									&err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_delete_dir(move_buf, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_delete_dir failed [%d]", err);
		}

		/* Deleting Meeting Request */
		if (!emstorage_delete_meeting_request(multi_user_name,
											account_id,
											result_mail_list[i].mail_id,
											0,
											false,
											&err)) {
			EM_DEBUG_EXCEPTION("emstorage_delete_meeting_request failed [%d]", err);
			if (err != EMAIL_ERROR_CONTACT_NOT_FOUND) {
				goto FINISH_OFF;
			}
		}

		/* Deleting mail_read_mail_uid_tbl */
		if (noti_param_1 != EMAIL_DELETE_LOCAL_AND_SERVER) {
			if (!emstorage_remove_downloaded_mail(multi_user_name,
													account_id,
													result_mail_list[i].mailbox_id,
													result_mail_list[i].server_mailbox_name,
													result_mail_list[i].server_mail_id,
													false,
													&err)) {
				EM_DEBUG_EXCEPTION("emstorage_remove_downloaded_mail failed [%d]", err);
				goto FINISH_OFF;
			}
		}
	}

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
	for (i = 0; i < num; i++) {
		if (!emcore_delete_auto_download_activity(multi_user_name, account_id, mail_ids[i], 0, &err)) {
			EM_DEBUG_EXCEPTION("emcore_delete_auto_download_activity failed [%d]", err);
		}
	}
#endif

	ret = true;

FINISH_OFF:

	if (ret == false) {
		if (!emcore_notify_storage_event(NOTI_MAIL_DELETE_FAIL,
										account_id,
										noti_param_1,
										noti_param_string,
										noti_param_2))
			EM_DEBUG_EXCEPTION(" emcore_notify_storage_eventfailed [ NOTI_MAIL_DELETE_FAIL ] >>>> ");
	}

	EM_SAFE_FREE(noti_param_string);

	if (result_mail_list)
		emstorage_free_mail(&result_mail_list, num, NULL);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

static int emcore_delete_mails_from_pop3_server(char *multi_user_name, email_account_t *input_account, int input_mail_ids[], int input_mail_id_count)
{
	EM_DEBUG_FUNC_BEGIN("input_account[%p], input_mail_ids[%p], input_mail_id_count[%d]", input_account, input_mail_ids, input_mail_id_count);

	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int mail_id = 0;
	int msgno = 0;
	void *stream = NULL;
	email_internal_mailbox_t mailbox_data = { 0, };
	emstorage_mail_tbl_t    *mail_tbl_data = NULL;

	if (!input_account || !input_mail_ids) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	for (i = 0; i < input_mail_id_count; i++) {
		/*several threads calls this function. don't use this statement */
		/* FINISH_OFF_IF_CANCELED; */

		mail_id = input_mail_ids[i];

		if (!emstorage_get_downloaded_mail(multi_user_name, mail_id, &mail_tbl_data, false, &err) || !mail_tbl_data) {
			EM_DEBUG_EXCEPTION("emstorage_get_downloaded_mail failed [%d]", err);
			if (err == EMAIL_ERROR_MAIL_NOT_FOUND) {	/* not server mail */
				continue;
			} else
				goto FINISH_OFF;
		}

		if (stream == NULL) {
			if (!emcore_connect_to_remote_mailbox(multi_user_name,
													input_account->account_id,
													0,
													true,
													(void **)&stream,
													&err)) {
				EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
				goto FINISH_OFF;
			}

			mailbox_data.mail_stream  = stream;
			mailbox_data.account_id   = input_account->account_id;
			mailbox_data.mailbox_id   = mail_tbl_data->mailbox_id;
		}

		if (mailbox_data.user_data != NULL) {
			emcore_free_uids(mailbox_data.user_data, NULL);
			mailbox_data.user_data = NULL;
		}

		if (!emcore_get_mail_msgno_by_uid(input_account, &mailbox_data, mail_tbl_data->server_mail_id, &msgno, &err)) {
			EM_DEBUG_EXCEPTION("emcore_get_mail_msgno_by_uid faild [%d]", err);
			if (err == EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER)
				goto NOT_FOUND_ON_SERVER;
			else
				goto FINISH_OFF;
		}

		if (!pop3_mail_delete(mailbox_data.mail_stream, msgno, &err)) {
			EM_DEBUG_EXCEPTION("pop3_mail_delete failed [%d]", err);

			if (err == EMAIL_ERROR_POP3_DELE_FAILURE)
				err = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER;
			goto FINISH_OFF;
		}

		if (!emstorage_remove_downloaded_mail(multi_user_name,
												input_account->account_id,
												mail_tbl_data->mailbox_id,
												mail_tbl_data->server_mailbox_name,
												mail_tbl_data->server_mail_id,
												true,
												&err))
			EM_DEBUG_LOG("emstorage_remove_downloaded_mail falied [%d]", err);

NOT_FOUND_ON_SERVER:

		if (mail_tbl_data != NULL)
			emstorage_free_mail(&mail_tbl_data, 1, NULL);
	}

FINISH_OFF:

	if (mail_tbl_data != NULL)
		emstorage_free_mail(&mail_tbl_data, 1, NULL);

	if (stream) {
		stream = mail_close(stream);
	}

	if (mailbox_data.user_data != NULL) {
		emcore_free_uids(mailbox_data.user_data, NULL);
		mailbox_data.user_data = NULL;
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_get_mail_msgno_by_uid(email_account_t *account, email_internal_mailbox_t *mailbox, char *uid, int *msgno, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("account[%p], mailbox[%p], uid[%s], msgno[%p], err_code[%p]", account, mailbox, uid, msgno, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	emcore_uid_list *uid_list = NULL;

	if (!account || !mailbox || !uid || !msgno) {
		EM_DEBUG_EXCEPTION_SEC("account[%p], mailbox[%p], uid[%s], msgno[%p]", account, mailbox, uid, msgno);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	uid_list = mailbox->user_data;

	if (uid_list == NULL) {
		if (account->incoming_server_type == EMAIL_SERVER_TYPE_POP3) {
			if (!pop3_mailbox_get_uids(mailbox->mail_stream, &uid_list, &err)) {
				EM_DEBUG_EXCEPTION("pop3_mailbox_get_uids failed [%d]", err);
				goto FINISH_OFF;
			}
		} else {		/*  EMAIL_SERVER_TYPE_IMAP4 */
			if (!imap4_mailbox_get_uids(mailbox->mail_stream, NULL, &uid_list, &err)) {
				EM_DEBUG_EXCEPTION("imap4_mailbox_get_uids failed [%d]", err);
				goto FINISH_OFF;
			}
		}
		mailbox->user_data = uid_list;
	}

	while (uid_list) {
		if (!strcmp(uid_list->uid, uid)) {
			*msgno = uid_list->msgno;
			EM_DEBUG_LOG("uid_list->msgno[%d]", uid_list->msgno);
			ret = true;
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG("other uid_list->msgno[%d]", uid_list->msgno);
		uid_list = uid_list->next;
	}

	err = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER;

FINISH_OFF:
	if (err_code != NULL)
		*err_code = err;

	if (uid_list != NULL)
		emcore_free_uids(uid_list, NULL);
	/*  mailbox->user_data and uid_list both point to same memory address, So when uid_list  is freed then just set  */
	/*  mailbox->user_data to NULL and dont use EM_SAFE_FREE, it will crash  :) */
	if (mailbox)
	mailbox->user_data = NULL;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_expunge_mails_deleted_flagged_from_local_storage(char *multi_user_name, int input_mailbox_id)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d]", input_mailbox_id);
	int   err = EMAIL_ERROR_NONE;
	char *conditional_clause_string = NULL;
	email_list_filter_t *filter_list = NULL;
	int  filter_count = 0;
	int *result_mail_id_list = NULL;
	int  result_count = 0;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;

	if (input_mailbox_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err =  EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &mailbox_tbl)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	filter_count = 3;

	filter_list = em_malloc(sizeof(email_list_filter_t) * filter_count);

	if (filter_list == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	filter_list[0].list_filter_item_type                              = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule.rule_type                    = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[0].list_filter_item.rule.target_attribute             = EMAIL_MAIL_ATTRIBUTE_MAILBOX_ID;
	filter_list[0].list_filter_item.rule.key_value.integer_type_value = input_mailbox_id;

	filter_list[1].list_filter_item_type                              = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[1].list_filter_item.operator_type                     = EMAIL_LIST_FILTER_OPERATOR_AND;

	filter_list[2].list_filter_item_type                              = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[2].list_filter_item.rule.rule_type                    = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[2].list_filter_item.rule.target_attribute             = EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD;
	filter_list[2].list_filter_item.rule.key_value.integer_type_value = 1;

	if ((err = emstorage_write_conditional_clause_for_getting_mail_list(multi_user_name, filter_list, filter_count, NULL, 0, -1, -1, &conditional_clause_string)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_write_conditional_clause_for_getting_mail_list failed[%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("conditional_clause_string[%s].", conditional_clause_string);

	if ((err = emstorage_query_mail_id_list(multi_user_name,
											conditional_clause_string,
											false,
											&result_mail_id_list,
											&result_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_id_list [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_delete_mails_from_local_storage(multi_user_name,
												mailbox_tbl->account_id,
												result_mail_id_list,
												result_count,
												EMAIL_DELETE_LOCALLY,
												EMAIL_DELETED_BY_COMMAND,
												&err)) {
		EM_DEBUG_EXCEPTION("emcore_delete_mails_from_local_storage [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

	if (filter_list)
		emstorage_free_list_filter(&filter_list, filter_count);

	EM_SAFE_FREE(conditional_clause_string);
	EM_SAFE_FREE(result_mail_id_list);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_expunge_mails_deleted_flagged_from_remote_server(char *multi_user_name, int input_account_id, int input_mailbox_id)
{
	int   err = EMAIL_ERROR_NONE;
	char *conditional_clause_string = NULL;
	email_list_filter_t *filter_list = NULL;
	int  filter_count = 0;
	int *result_mail_id_list = NULL;
	int  result_count = 0;

	if (input_mailbox_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err =  EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	filter_count = 3;

	filter_list = em_malloc(sizeof(email_list_filter_t) * filter_count);

	if (filter_list == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	filter_list[0].list_filter_item_type                              = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule.rule_type                    = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[0].list_filter_item.rule.target_attribute             = EMAIL_MAIL_ATTRIBUTE_MAILBOX_ID;
	filter_list[0].list_filter_item.rule.key_value.integer_type_value = input_mailbox_id;

	filter_list[1].list_filter_item_type                              = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[1].list_filter_item.operator_type                     = EMAIL_LIST_FILTER_OPERATOR_AND;

	filter_list[2].list_filter_item_type                              = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[2].list_filter_item.rule.rule_type                    = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[2].list_filter_item.rule.target_attribute             = EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD;
	filter_list[2].list_filter_item.rule.key_value.integer_type_value = 1;

	if ((err = emstorage_write_conditional_clause_for_getting_mail_list(multi_user_name, filter_list, filter_count, NULL, 0, -1, -1, &conditional_clause_string)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_write_conditional_clause_for_getting_mail_list failed[%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("conditional_clause_string[%s].", conditional_clause_string);

	if ((err = emstorage_query_mail_id_list(multi_user_name, conditional_clause_string, true, &result_mail_id_list, &result_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_id_list [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_delete_mail(multi_user_name,
							input_account_id,
							input_mailbox_id,
							result_mail_id_list,
							result_count,
							EMAIL_DELETE_FROM_SERVER,
							1,
							EMAIL_DELETED_BY_COMMAND,
							&err)) {
		EM_DEBUG_EXCEPTION("emcore_delete_mail [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (filter_list)
		emstorage_free_list_filter(&filter_list, filter_count);

	EM_SAFE_FREE(result_mail_id_list);
	EM_SAFE_FREE(conditional_clause_string);

	EM_DEBUG_FUNC_END("err [%d]", err);

	return err;
}

/* description
 *    add a attachment to mail.
 * arguments
 *    mailbox  :  mail box
 *    mail_id  :  mail id
 *    attachment  :  attachment to be added
 * return
 *    succeed  :  1
 *    fail  :  0
 */
INTERNAL_FUNC int emcore_add_attachment(char *multi_user_name, int mail_id, email_attachment_data_t *attachment, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], attachment[%p], err_code[%p]", mail_id, attachment, err_code);

	if (attachment == NULL) {
		EM_DEBUG_EXCEPTION("mail_id[%d], attachment[%p]", mail_id, attachment);
		if (err_code)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false, err = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t *mail_table_data = NULL;
	int attachment_id = 0;
	int before_tr_begin = 0;

	if (!emstorage_get_mail_field_by_id(multi_user_name, mail_id, RETRIEVE_SUMMARY, &mail_table_data, true, &err) || !mail_table_data) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_field_by_id failed [%d]", err);
		goto FINISH_OFF2;
	}

	int account_id = mail_table_data->account_id;
	emstorage_attachment_tbl_t attachment_tbl;

	memset(&attachment_tbl, 0x00, sizeof(emstorage_attachment_tbl_t));

	mail_table_data->attachment_count               = mail_table_data->attachment_count + 1;
	attachment_tbl.account_id                       = mail_table_data->account_id;
	attachment_tbl.mailbox_id                       = mail_table_data->mailbox_id;
	attachment_tbl.mail_id 	                        = mail_id;
	attachment_tbl.attachment_name                  = attachment->attachment_name;
	attachment_tbl.attachment_size                  = attachment->attachment_size;
	attachment_tbl.attachment_save_status           = attachment->save_status;
	attachment_tbl.attachment_drm_type              = attachment->drm_status;
	attachment_tbl.attachment_inline_content_status = attachment->inline_content_status;
	attachment_tbl.attachment_mime_type             = attachment->attachment_mime_type;
	attachment_tbl.content_id                       = attachment->content_id;

	/*  BEGIN TRANSACTION; */
	if (!emstorage_begin_transaction(multi_user_name, NULL, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_begin_transaction failed [%d]", err);
		before_tr_begin = 1;
		goto FINISH_OFF;
	}

	if (!emstorage_add_attachment(multi_user_name, &attachment_tbl, 0, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_add_attachment failed [%d]", err);
		goto FINISH_OFF;
	}

	attachment->attachment_id = attachment_tbl.attachment_id;

	if (attachment->attachment_path) {
		char move_buf[512] = {0};
		char path_buf[512] = {0};

		if (!attachment->inline_content_status) {
			if (!emstorage_create_dir(multi_user_name, account_id, mail_id, attachment_tbl.attachment_id, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
				goto FINISH_OFF;
			}
			attachment_id = attachment_tbl.attachment_id;
		}

		if (!emstorage_get_save_name(multi_user_name, account_id, mail_id, attachment_id, attachment->attachment_name, move_buf, path_buf, sizeof(path_buf), &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

	    attachment_tbl.attachment_path = path_buf;

		if (!emstorage_change_attachment_field(multi_user_name, mail_id, UPDATE_SAVENAME, &attachment_tbl, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_change_attachment_field failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_change_mail_field(multi_user_name, mail_id, APPEND_BODY, mail_table_data, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);
			goto FINISH_OFF;
		}

		if (attachment->save_status) {
			if (!emstorage_move_file(attachment->attachment_path, move_buf, false, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
				goto FINISH_OFF;
			}
		}

		/* Here only filename is being updated. Since first add is being done there will not be any old files.
		    So no need to check for old files in this update case */
		if (!emstorage_change_attachment_field(multi_user_name, mail_id, UPDATE_SAVENAME, &attachment_tbl, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_change_attachment_field failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_SAFE_FREE(attachment->attachment_path);
		attachment->attachment_path = EM_SAFE_STRDUP(path_buf);
	}

	ret = true;

FINISH_OFF:
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(multi_user_name, NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
			ret = false;
		}
	} else {	/*  ROLLBACK TRANSACTION; */
		if (!before_tr_begin && emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL) == false)
			err = EMAIL_ERROR_DB_FAILURE;
	}

FINISH_OFF2:
	if (mail_table_data != NULL)
		emstorage_free_mail(&mail_table_data, 1, NULL);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}


INTERNAL_FUNC int emcore_add_attachment_data(char *multi_user_name, int input_mail_id, email_attachment_data_t *input_attachment_data)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], input_attachment_data[%p]", input_mail_id, input_attachment_data);

	int err = EMAIL_ERROR_NONE;
	int attachment_id = 0;
	int ret = false;
	int before_tr_begin = 0;
	char move_buf[512] = { 0, };
	char path_buf[512] = { 0, };

	emstorage_mail_tbl_t *mail_table_data = NULL;
	emstorage_attachment_tbl_t attachment_tbl  = { 0 };

	if (input_attachment_data == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (!emstorage_get_mail_field_by_id(multi_user_name, input_mail_id, RETRIEVE_SUMMARY, &mail_table_data, true, &err) || !mail_table_data) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_field_by_id failed [%d]", err);
		before_tr_begin = 1;
		goto FINISH_OFF;
	}

	mail_table_data->attachment_count               = mail_table_data->attachment_count + 1;
	attachment_tbl.account_id                       = mail_table_data->account_id;
	attachment_tbl.mailbox_id                       = mail_table_data->mailbox_id;
	attachment_tbl.mail_id 	                        = input_mail_id;
	attachment_tbl.attachment_name                  = input_attachment_data->attachment_name;
	attachment_tbl.attachment_size                  = input_attachment_data->attachment_size;
	attachment_tbl.attachment_save_status           = input_attachment_data->save_status;
	attachment_tbl.attachment_drm_type              = input_attachment_data->drm_status;
	attachment_tbl.attachment_inline_content_status = input_attachment_data->inline_content_status;
	attachment_tbl.attachment_mime_type             = input_attachment_data->attachment_mime_type;
	attachment_tbl.content_id                       = input_attachment_data->content_id;

	/*  BEGIN TRANSACTION; */
	if (!emstorage_begin_transaction(multi_user_name, NULL, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_begin_transaction failed [%d]", err);
		before_tr_begin = 1;
		goto FINISH_OFF;
	}

	if (!emstorage_add_attachment(multi_user_name, &attachment_tbl, 0, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_add_attachment failed [%d]", err);
		goto FINISH_OFF;
	}

	input_attachment_data->attachment_id = attachment_tbl.attachment_id;

	if (input_attachment_data->attachment_path) {
		if (!input_attachment_data->inline_content_status) {
			if (!emstorage_create_dir(multi_user_name, mail_table_data->account_id, input_mail_id, attachment_tbl.attachment_id, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
				goto FINISH_OFF;
			}
			attachment_id = attachment_tbl.attachment_id;
		}

		if (!emstorage_get_save_name(multi_user_name, mail_table_data->account_id, input_mail_id, attachment_id, input_attachment_data->attachment_name, move_buf, path_buf, sizeof(path_buf), &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		attachment_tbl.attachment_path = path_buf;

		if (!emstorage_change_attachment_field(multi_user_name, input_mail_id, UPDATE_SAVENAME, &attachment_tbl, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_change_attachment_field failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_change_mail_field(multi_user_name, input_mail_id, APPEND_BODY, mail_table_data, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);
			goto FINISH_OFF;
		}

		if (input_attachment_data->save_status) {
			if (!emstorage_copy_file(input_attachment_data->attachment_path, move_buf, false, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
				goto FINISH_OFF;
			}
		}

		/* Here only filename is being updated. Since first add is being done there will not be any old files.
			So no need to check for old files in this update case */
		if (!emstorage_change_attachment_field(multi_user_name, input_mail_id, UPDATE_SAVENAME, &attachment_tbl, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_change_attachment_field failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_SAFE_FREE(input_attachment_data->attachment_path);
		input_attachment_data->attachment_path = EM_SAFE_STRDUP(path_buf);
	}

	ret = true;

FINISH_OFF:
	if (ret) { /*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(multi_user_name, NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
		}
	} else { /*  ROLLBACK TRANSACTION; */
		if (!before_tr_begin && emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL) == false)
			err = EMAIL_ERROR_DB_FAILURE;
	}

	if (mail_table_data != NULL)
		emstorage_free_mail(&mail_table_data, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


/* description
 *    delete a attachment from mail.
 * arguments
 *    mailbox  :  mail box
 *    mail_id  :  mail id
 *    attachment_id  :  number string of attachment-id to be deleted
 *                 (ex :  if attachment id is 2, number stirng will be "2")
 * return
 *    succeed  :  1
 *    fail  :  0
 */
int emcore_delete_mail_attachment(char *multi_user_name, int attachment_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_id[%d], err_code[%p]", attachment_id, err_code);

	if (attachment_id == 0) {
		EM_DEBUG_EXCEPTION("attachment_id[%d]", attachment_id);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int before_tr_begin = 0;
	char attachment_folder_path[MAX_PATH] = {0, };
	emstorage_attachment_tbl_t *attachment_tbl = NULL;

	if (!emstorage_get_attachment(multi_user_name, attachment_id, &attachment_tbl, true, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_get_attachment failed");
		return false;
	}

	/*  BEGIN TRANSACTION; */
	if (!emstorage_begin_transaction(multi_user_name, NULL, NULL, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_begin_transaction failed [%d]", error);
		before_tr_begin = 1;
		goto FINISH_OFF;
	}

	if (!emstorage_delete_attachment_on_db(multi_user_name, attachment_id, false, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_delete_attachment_on_db failed [%d]", error);
		if (emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL) == false)
			error = EMAIL_ERROR_DB_FAILURE;

		goto FINISH_OFF;
	}

	if (attachment_tbl->attachment_inline_content_status != INLINE_ATTACHMENT) {
		SNPRINTF(attachment_folder_path, sizeof(attachment_folder_path), "%s/%d/%d/%d", MAILHOME, attachment_tbl->account_id, attachment_tbl->mail_id, attachment_id);

		if (!emstorage_delete_dir(attachment_folder_path, NULL)) {
			EM_DEBUG_EXCEPTION("emstorage_delete_dir failed");
		}
	} else {
		if (!emstorage_delete_file(attachment_tbl->attachment_path, NULL)) {
			EM_DEBUG_EXCEPTION("emstorage_delete_file failed");
		}
	}

	ret = true;

FINISH_OFF:

	if (ret == true) {
		if (emstorage_commit_transaction(multi_user_name, NULL, NULL, NULL) == false) {
			error = EMAIL_ERROR_DB_FAILURE;
			ret = false;
		}
	} else {
		if (!before_tr_begin && emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL) == false) {
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (attachment_tbl)
		emstorage_free_attachment(&attachment_tbl, 1, NULL);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

static int emcore_mail_update_attachment_data(char *multi_user_name, int input_mail_id, email_attachment_data_t *input_attachment_data)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], input_attachment_data[%p]", input_mail_id, input_attachment_data);

	int err = EMAIL_ERROR_NONE;
	int attachment_id = 0;
	char move_buf[512] = { 0 , };
	char path_buf[512] = {0,};
	emstorage_attachment_tbl_t *existing_attachment_info = NULL;
	emstorage_attachment_tbl_t  attachment_tbl = { 0 };

	if (input_attachment_data == NULL) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_get_attachment(multi_user_name, input_attachment_data->attachment_id, &existing_attachment_info, 1, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_attachment failed [%d]", err);
		goto FINISH_OFF;
	}

	attachment_tbl.mail_id                          = input_mail_id;
	attachment_tbl.account_id                       = existing_attachment_info->account_id;
	attachment_tbl.mailbox_id                       = existing_attachment_info->mailbox_id;
	attachment_tbl.attachment_name                  = input_attachment_data->attachment_name;
	attachment_tbl.attachment_size                  = input_attachment_data->attachment_size;
	attachment_tbl.attachment_path                  = input_attachment_data->attachment_path;
	attachment_tbl.attachment_save_status           = input_attachment_data->save_status;
	attachment_tbl.attachment_drm_type              = input_attachment_data->drm_status;
	attachment_tbl.attachment_inline_content_status = input_attachment_data->inline_content_status;
	attachment_tbl.attachment_mime_type             = input_attachment_data->attachment_mime_type;
	attachment_tbl.attachment_id                    = input_attachment_data->attachment_id;
	attachment_tbl.content_id                       = input_attachment_data->content_id;

	if (!input_attachment_data->inline_content_status) {
		if (!emstorage_create_dir(multi_user_name, attachment_tbl.account_id, input_mail_id, attachment_tbl.attachment_id, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}
		attachment_id = attachment_tbl.attachment_id;
	}

	if (!emstorage_get_save_name(multi_user_name, attachment_tbl.account_id, input_mail_id, attachment_id, input_attachment_data->attachment_name, move_buf, path_buf, sizeof(path_buf), &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
		goto FINISH_OFF;
	}

	attachment_tbl.attachment_path = path_buf;

	EM_DEBUG_LOG_SEC("downloaded [%d], savename [%s], attachment_path [%s]", input_attachment_data->save_status, input_attachment_data->attachment_path, existing_attachment_info->attachment_path);
	if (input_attachment_data->save_status && EM_SAFE_STRCMP(input_attachment_data->attachment_path, existing_attachment_info->attachment_path) != 0) {
		if (!emstorage_move_file(input_attachment_data->attachment_path, move_buf, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}
	} else
		EM_DEBUG_LOG("no need to move");

	EM_SAFE_FREE(input_attachment_data->attachment_path);
	input_attachment_data->attachment_path = EM_SAFE_STRDUP(path_buf);

	if (!emstorage_begin_transaction(multi_user_name, NULL, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_begin_transaction failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emstorage_update_attachment(multi_user_name, &attachment_tbl, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_update_attachment failed [%d]", err);
	}

	if (err == EMAIL_ERROR_NONE) {	/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(multi_user_name, NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
		}
	} else {	/*  ROLLBACK TRANSACTION; */
		if (emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL) == false)
			err = EMAIL_ERROR_DB_FAILURE;
	}

FINISH_OFF:
	if (existing_attachment_info)
		emstorage_free_attachment(&existing_attachment_info, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


static int emcore_mail_compare_filename_of_attachment_data(char *multi_user_name, int input_mail_id, int input_attachment_a_id, email_attachment_data_t *input_attachment_b_data, int *result)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], input_attachment_a_id[%d], input_attachment_b_data[%p], result[%p]", input_mail_id, input_attachment_a_id, input_attachment_b_data, result);

	if (!input_attachment_b_data || !result) {
		return EMAIL_ERROR_INVALID_PARAM;
	}

	int err = EMAIL_ERROR_NONE;
	ssize_t ret_readlink;
	char *linkpath = NULL;
	struct stat st_buf;

	emstorage_attachment_tbl_t *attachment_a_tbl = NULL;

	if (input_attachment_b_data->attachment_path && (stat(input_attachment_b_data->attachment_path, &st_buf) == 0)) {
		linkpath = em_malloc(st_buf.st_size + 1);
		if (linkpath == NULL) {
			EM_DEBUG_EXCEPTION("em_mallocfailed");
			goto FINISH_OFF;
		}

		ret_readlink = readlink(input_attachment_b_data->attachment_path, linkpath, st_buf.st_size + 1);
		if (ret_readlink > 0) {
			linkpath[st_buf.st_size] = '\0';
			EM_DEBUG_LOG("symbolic link path : [%s]", linkpath);

			if (emstorage_get_attachment_by_attachment_path(multi_user_name, linkpath, &attachment_a_tbl, false, &err)) {
				if (attachment_a_tbl->mail_id == input_mail_id) {
					input_attachment_b_data->attachment_id = attachment_a_tbl->attachment_id;
					*result = 2;
					goto FINISH_OFF;
				}
			}

			if (attachment_a_tbl)
				emstorage_free_attachment(&attachment_a_tbl, 1, NULL);
		}
	}

	if (!emstorage_get_attachment(multi_user_name, input_attachment_a_id, &attachment_a_tbl, 1, &err)) {
		if (err == EMAIL_ERROR_ATTACHMENT_NOT_FOUND)
			EM_DEBUG_LOG("no attachment found");
		else
			EM_DEBUG_EXCEPTION("emstorage_get_attachment failed [%d]", err);

		*result = 1; /* matching attachment file is not found */

		goto FINISH_OFF;
	}

	if (attachment_a_tbl->attachment_path && input_attachment_b_data->attachment_path) {
		EM_DEBUG_LOG_SEC("attachment_a_tbl->attachment_path [%s], input_attachment_b_data->attachment_path [%s]", attachment_a_tbl->attachment_path, input_attachment_b_data->attachment_path);
		if (strcmp(attachment_a_tbl->attachment_path, input_attachment_b_data->attachment_path) == 0)
			*result = 0;
		else
			*result = 1;
	}

FINISH_OFF:

	if (attachment_a_tbl)
		emstorage_free_attachment(&attachment_a_tbl, 1, NULL);

	EM_SAFE_FREE(linkpath);

	EM_DEBUG_FUNC_END("*result [%d]", *result);
	return err;
}


/* description
 *    copy a mail to mail box
 * arguments
 *    src_mailbox  :  source mail box
 *    msgno  :  mail sequence
 *    dst_mailbox  :  target mail box
 * return
 *    succeed  :  1
 *    fail  :  0
 */
INTERNAL_FUNC int emcore_mail_copy(char *multi_user_name, int mail_id, email_mailbox_t *dst_mailbox, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], dst_mailbox[%p], err_code[%p]", mail_id, dst_mailbox, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int i;
	emstorage_mail_tbl_t *mail = NULL;
	emstorage_attachment_tbl_t *atch_list = NULL;
	int count = EMAIL_ATTACHMENT_MAX_COUNT;
	char *mailbox_name = NULL;
	char *stripped_text = NULL;
    char *prefix_path = NULL;
	char virtual_path[512];
    char real_path[512];
    char real_file_path[512];

	if (!emstorage_get_mail_by_id(multi_user_name, mail_id, &mail, true, &err) || !mail) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);

		goto FINISH_OFF;
	}

	if ((err = emstorage_get_attachment_list(multi_user_name, mail_id, true, &atch_list, &count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);
		goto FINISH_OFF;
	}

	/*  get increased uid. */
	if (!emstorage_increase_mail_id(multi_user_name, &mail->mail_id, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_increase_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

    if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		err = emcore_get_container_path(multi_user_name, &prefix_path);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_container_path failed : [%d]", err);
			goto FINISH_OFF;
		}
	} else {
		prefix_path = strdup("");
	}

	/*  copy mail body(text) file */
	if (mail->file_path_plain) {
		if (!emstorage_create_dir(multi_user_name, dst_mailbox->account_id, mail->mail_id, 0, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		gchar *filename = g_path_get_basename(mail->file_path_plain);

        memset(virtual_path, 0x00, sizeof(virtual_path));
        memset(real_path, 0x00, sizeof(virtual_path));

        memset(real_file_path, 0x00, sizeof(real_file_path));
        SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, mail->file_path_plain);

		if (!emstorage_get_save_name(multi_user_name, dst_mailbox->account_id, mail->mail_id, 0, filename, real_path, virtual_path, sizeof(virtual_path), &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			g_free(filename);
			goto FINISH_OFF;
		}

		g_free(filename);

		if (!emstorage_copy_file(real_file_path, real_path, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_copy_file failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_SAFE_FREE(mail->file_path_plain);
		mail->file_path_plain = EM_SAFE_STRDUP(virtual_path);
	}

	/*  copy mail body(html) file */
	if (mail->file_path_html) {
		if (!emstorage_create_dir(multi_user_name, dst_mailbox->account_id, mail->mail_id, 0, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);

			goto FINISH_OFF;
		}

		gchar *filename = g_path_get_basename(mail->file_path_html);

        memset(virtual_path, 0x00, sizeof(virtual_path));
        memset(real_path, 0x00, sizeof(virtual_path));

        memset(real_file_path, 0x00, sizeof(real_file_path));
        SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, mail->file_path_html);

		if (!emstorage_get_save_name(multi_user_name,
                                    dst_mailbox->account_id,
                                    mail->mail_id,
                                    0,
                                    filename,
                                    real_path,
                                    virtual_path,
                                    sizeof(virtual_path),
                                    &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			g_free(filename);
			goto FINISH_OFF;
		}

		g_free(filename);

		if (!emstorage_copy_file(real_file_path, real_path, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_copy_file failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_SAFE_FREE(mail->file_path_html); /*valgrind*/
		mail->file_path_html = EM_SAFE_STRDUP(virtual_path);
	}

	/*  BEGIN TRANSACTION; */
	if (!emstorage_begin_transaction(multi_user_name, NULL, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_begin_transaction failed [%d]", err);
		goto FINISH_OFF;
	}

	/*  insert mail data */

	mail->account_id   = dst_mailbox->account_id;
	mail->mailbox_id   = dst_mailbox->mailbox_id;
	mail->mailbox_type = dst_mailbox->mailbox_type;

	if (!emstorage_add_mail(multi_user_name, mail, 0, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_add_mail failed [%d]", err);

		if (mail->file_path_plain) {
            memset(real_file_path, 0x00, sizeof(real_file_path));
            SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, mail->file_path_plain);

			if (!emstorage_delete_file(mail->file_path_plain, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_delete_file failed [%d]", err);
				emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}

		if (mail->file_path_html) {
            memset(real_file_path, 0x00, sizeof(real_file_path));
            SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, mail->file_path_html);

			if (!emstorage_delete_file(real_file_path, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_delete_file failed [%d]", err);
				emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}

		/*  ROLLBACK TRANSACTION; */
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);

		goto FINISH_OFF;
	}

	/*  copy attachment file */
	for (i = 0; i < count; i++) {
		if (atch_list[i].attachment_path) {
			if (!emstorage_create_dir(multi_user_name, dst_mailbox->account_id, mail->mail_id, i+1, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
				break;
			}

            memset(virtual_path, 0x00, sizeof(virtual_path));
            memset(real_path, 0x00, sizeof(virtual_path));

            memset(real_file_path, 0x00, sizeof(real_file_path));
            SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, atch_list[i].attachment_path);

			if (!emstorage_get_save_name(multi_user_name,
                                        dst_mailbox->account_id,
                                        mail->mail_id,
                                        i+1,
                                        atch_list[i].attachment_name,
                                        real_path,
                                        virtual_path,
                                        sizeof(virtual_path),
                                        &err)) {
				EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
				break;
			}

			if (!emstorage_copy_file(real_file_path, real_path, false, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_copy_file failed [%d]", err);
				break;
			}

			EM_SAFE_FREE(atch_list[i].attachment_path);
			atch_list[i].attachment_path = EM_SAFE_STRDUP(virtual_path);
		}

		atch_list[i].account_id = dst_mailbox->account_id;
		atch_list[i].mail_id = mail->mail_id;
		atch_list[i].mailbox_id = mail->mailbox_id;

		if (!emstorage_add_attachment(multi_user_name, &atch_list[i], 0, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_add_attachment failed [%d]", err);
			break;
		}
	}

	/*  in case error happened, delete copied file. */
	if (i && i != count) {
		for (; i >= 0; i--) {
			if (atch_list[i].attachment_path) {
                memset(real_file_path, 0x00, sizeof(real_file_path));
                SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, atch_list[i].attachment_path);

				if (!emstorage_delete_file(real_file_path, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_delete_file failed [%d]", err);
					emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
					goto FINISH_OFF;
				}
			}
		}

		if (mail->file_path_plain) {
            memset(real_file_path, 0x00, sizeof(real_file_path));
            SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, mail->file_path_plain);

			if (!emstorage_delete_file(real_file_path, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_delete_file failed [%d]", err);
				emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}

        if (mail->file_path_html) {
            memset(real_file_path, 0x00, sizeof(real_file_path));
            SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, mail->file_path_html);

			if (!emstorage_delete_file(real_file_path, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_delete_file failed [%d]", err);
				emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}

		/*  ROLLBACK TRANSACTION; */
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
		goto FINISH_OFF;
	}

#ifdef __FEATURE_BODY_SEARCH__
	/* Insert mail_text to DB */
	emstorage_mailbox_tbl_t *output_mailbox;
	if (!emcore_strip_mail_body_from_file(multi_user_name, mail, &stripped_text, &err) || stripped_text == NULL) {
		EM_DEBUG_EXCEPTION("emcore_strip_mail_body_from_file failed [%d]", err);
	}

	if (emstorage_get_mailbox_by_id(multi_user_name, dst_mailbox->mailbox_id, &output_mailbox) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed");
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
		goto FINISH_OFF;
	}

	if (!emcore_add_mail_text(multi_user_name, output_mailbox, mail, stripped_text, &err)) {
		EM_DEBUG_EXCEPTION("emcore_add_mail_text failed [%d]", err);
		emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL);
		goto FINISH_OFF;
	}

	if (output_mailbox != NULL)
		emstorage_free_mailbox(&output_mailbox, 1, NULL);

#endif

	if (emstorage_commit_transaction(multi_user_name, NULL, NULL, NULL) == false) {
		err = EMAIL_ERROR_DB_FAILURE;
		if (emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL) == false)
			err = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mailbox_name_by_mailbox_type(multi_user_name, dst_mailbox->account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox_name, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_name_by_mailbox_type failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!strcmp(dst_mailbox->mailbox_name, mailbox_name) && !(mail->flags_seen_field))
		emcore_display_unread_in_badge(multi_user_name);

	ret = true;

FINISH_OFF:
	if (atch_list != NULL)
		emstorage_free_attachment(&atch_list, count, NULL);

	if (mail != NULL)
		emstorage_free_mail(&mail, 1, NULL);

#ifdef __FEATURE_BODY_SEARCH__
	EM_SAFE_FREE(stripped_text);
#endif
	EM_SAFE_FREE(mailbox_name);
	EM_SAFE_FREE(prefix_path);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}

/* description
 *    move a mail to mail box
 * arguments
 *    old_mailbox  :  previous mail box
 *    msgno  :  msgno
 *    new_mailbox  :  target mail box
 * return
 *    succeed  :  1
 *    fail  :  0
 */
INTERNAL_FUNC int emcore_move_mail(char *multi_user_name, int mail_ids[], int mail_ids_count, int dst_mailbox_id, int noti_param_1, int noti_param_2, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], mail_ids_count[%d], dst_mailbox_id[%d], noti_param [%d], err_code[%p]", mail_ids, mail_ids_count, dst_mailbox_id, noti_param_1, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t *mail_list = NULL;
	emstorage_mail_tbl_t *p_mail_data = NULL;
	int account_id = 0;
	int i = 0, j = 0, parameter_string_length = 0;
	int p_thread_id = 0;
	int p_thread_item_count = 0;
	int p_lastest_mail_id = 0;
	char *parameter_string = NULL, mail_id_string[10];

	int *dest_prev_thread_id_list = NULL;
	int dest_prev_thread_id = 0;
	int dest_prev_thread_item_count = 0;
	int find_striped_subject = 0;
	char stripped_subject[4086];

	if (dst_mailbox_id <= 0  && mail_ids_count < 1) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mail_field_by_multiple_mail_id(multi_user_name, mail_ids, mail_ids_count, RETRIEVE_SUMMARY, &mail_list, true, &err) || !mail_list) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_field_by_multiple_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

	account_id = mail_list[0].account_id;

	dest_prev_thread_id_list = em_malloc(sizeof(int) * mail_ids_count);
	if (dest_prev_thread_id_list == NULL) {
		EM_DEBUG_EXCEPTION("Memory allocation for mail_id_list_string failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < mail_ids_count; i++) {
		/* Get the thread_id before move */
		if (emstorage_get_thread_id_from_mailbox(multi_user_name, account_id, dst_mailbox_id, mail_list[i].subject, &dest_prev_thread_id, &dest_prev_thread_item_count) != EMAIL_ERROR_NONE)
			EM_DEBUG_LOG("emstorage_get_thread_id_of_thread_mails is failed.");

		dest_prev_thread_id_list[i] = dest_prev_thread_id;
	}

	if (!emstorage_move_multiple_mails_on_db(multi_user_name,
												account_id,
												dst_mailbox_id,
												mail_ids,
												mail_ids_count,
												true,
												&err)) {
		EM_DEBUG_EXCEPTION("emstorage_move_multiple_mails_on_db failed [%d]", err);
		goto FINISH_OFF;
	}

	for (i = 0; i < mail_ids_count; i++) {
		if (mail_list[i].subject == NULL)
			continue;

		if (dest_prev_thread_id_list[i] == -1) {
			if (em_find_pos_stripped_subject_for_thread_view(mail_list[i].subject,
																stripped_subject,
																sizeof(stripped_subject)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("em_find_pos_stripped_subject_for_thread_view  is failed");
			} else {
				EM_DEBUG_LOG_SEC("subject: [%s]", mail_list[i].subject);
				if (EM_SAFE_STRLEN(stripped_subject) >= 2) {
					find_striped_subject = 1;
				}
				EM_DEBUG_LOG_SEC("em_find_pos_stripped_subject_for_thread_view returns[len = %d] = %s",
									EM_SAFE_STRLEN(stripped_subject), stripped_subject);
			}

			if (find_striped_subject) {
				for (j = 0; j < i; j++) {
					if (g_strrstr(mail_list[j].subject, stripped_subject)) {
						dest_prev_thread_id_list[i] = mail_ids[j];
						break;
					}
				}

				if (j == i)
					dest_prev_thread_id_list[i] = mail_ids[i];
			} else {
				dest_prev_thread_id_list[i] = mail_ids[i];
			}
		}

		if (!emstorage_update_thread_id_of_mail(multi_user_name,
												account_id,
												dst_mailbox_id,
												mail_ids[i],
												dest_prev_thread_id_list[i],
												0,
												false,
												&err)) {
			EM_DEBUG_EXCEPTION("emstorage_update_latest_thread_mail failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	/* Sending a notification */
	parameter_string_length = sizeof(char) * (mail_ids_count * 10 + 128/*MAILBOX_LEN_IN_MAIL_TBL*/ * 2);
	parameter_string = em_malloc(parameter_string_length);

	if (parameter_string == NULL) {
		EM_DEBUG_EXCEPTION("Memory allocation for mail_id_list_string failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (mail_list[0].mailbox_id > 0)
		SNPRINTF(parameter_string, parameter_string_length, "%d%c%d%c", mail_list[0].mailbox_id, 0x01, dst_mailbox_id , 0x01);

	for (i = 0; i < mail_ids_count; i++) {
		memset(mail_id_string, 0, 10);
		SNPRINTF(mail_id_string, 10, "%d,", mail_ids[i]);
		EM_SAFE_STRNCAT(parameter_string, mail_id_string, parameter_string_length - EM_SAFE_STRLEN(parameter_string) - 1);
	}

	EM_DEBUG_LOG("num : [%d], param string : [%s]", mail_ids_count , parameter_string);

	if (!emcore_notify_storage_event(NOTI_MAIL_MOVE, account_id, noti_param_1, parameter_string, noti_param_2))
		EM_DEBUG_EXCEPTION(" emcore_notify_storage_eventfailed [NOTI_MAIL_MOVE] >>>> ");


	for (i = 0; i < mail_ids_count; i++) {
		if (mail_list[i].subject == NULL)
			continue;
		p_thread_id = -1;
		p_thread_item_count = 0;
		p_lastest_mail_id = -1;

		/* Get the information of moved mail */
		if (!emstorage_get_mail_by_id(multi_user_name, mail_ids[i], &p_mail_data, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed : [%d]", err);
			goto FINISH_OFF;
		}

		/* Get the thread_id of moved mail */
		if (emstorage_get_thread_id_of_thread_mails(multi_user_name, p_mail_data, &p_thread_id, &p_lastest_mail_id, &p_thread_item_count) != EMAIL_ERROR_NONE)
			EM_DEBUG_LOG("emstorage_get_thread_id_of_thread_mails is failed.");

		/* Original mailbox replace thread id */
		if (!emstorage_update_latest_thread_mail(multi_user_name,
													account_id,
													mail_list[i].mailbox_id,
													mail_list[i].mailbox_type,
													mail_list[i].thread_id,
													NULL,
													0,
													0,
													NOTI_THREAD_ID_CHANGED_BY_MOVE_OR_DELETE,
													false,
													&err)) {
			EM_DEBUG_EXCEPTION("emstorage_update_latest_thread_mail failed [%d]", err);
			goto FINISH_OFF;
		}

		/* Destination mailbox replace thread id */
		if (p_thread_id == -1) {
			if (!emstorage_update_latest_thread_mail(multi_user_name,
														account_id,
														p_mail_data->mailbox_id,
														p_mail_data->mailbox_type,
														p_mail_data->mail_id,
														NULL,
														p_mail_data->mail_id,
														1,
														NOTI_THREAD_ID_CHANGED_BY_MOVE_OR_DELETE,
														false,
														&err)) {
				EM_DEBUG_EXCEPTION("emstorage_update_latest_thread_mail failed [%d]", err);
				goto FINISH_OFF;
			}
		} else {
			if (!emstorage_update_latest_thread_mail(multi_user_name,
														account_id,
														p_mail_data->mailbox_id,
														p_mail_data->mailbox_type,
														dest_prev_thread_id_list[i],
														NULL,
														0,
														0,
														NOTI_THREAD_ID_CHANGED_BY_MOVE_OR_DELETE,
														false,
														&err)) {
				EM_DEBUG_EXCEPTION("emstorage_update_latest_thread_mail failed [%d]", err);
				goto FINISH_OFF;
			}
		}
		emstorage_free_mail(&p_mail_data, 1, NULL);
	}

	if (!emcore_notify_storage_event(NOTI_MAIL_MOVE_FINISH, account_id, noti_param_1, parameter_string, noti_param_2))
		EM_DEBUG_EXCEPTION(" emcore_notify_storage_eventfailed [NOTI_MAIL_MOVE_FINISH] >>>> ");

	emcore_display_unread_in_badge(multi_user_name);

	ret = true;

FINISH_OFF:
	if (ret == false) {
		if (!emcore_notify_storage_event(NOTI_MAIL_MOVE_FAIL, account_id, noti_param_1, parameter_string, noti_param_2))
			EM_DEBUG_EXCEPTION(" emcore_notify_storage_eventfailed [ NOTI_MAIL_MOVE_FAIL ] >>>> ");
	}

	emstorage_free_mail(&p_mail_data, 1, NULL);
	emstorage_free_mail(&mail_list, mail_ids_count, NULL);

	EM_SAFE_FREE(parameter_string);
	EM_SAFE_FREE(dest_prev_thread_id_list); /*prevent 38972*/

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}


INTERNAL_FUNC int emcore_move_mail_on_server(char *multi_user_name, int account_id, int src_mailbox_id,  int mail_ids[], int num, char *dest_mailbox, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN();
	MAILSTREAM *stream = NULL;
	int err_code = 0;
	email_account_t *ref_account = NULL;
	emstorage_mail_tbl_t *mail = NULL;
	int ret = 1;
	int mail_id = 0;
	int i = 0;

	mail_id = mail_ids[0];

	ref_account = emcore_get_account_reference(multi_user_name, account_id, false);
	if (!ref_account) {
		EM_DEBUG_EXCEPTION("emcore_move_mail_on_server failed  :  get account reference[%d]", account_id);
		*error_code = EMAIL_ERROR_INVALID_ACCOUNT;
		ret = 0;
		goto FINISH_OFF;
	}

	/* if not imap4 mail, return */
 	if (ref_account->incoming_server_type != EMAIL_SERVER_TYPE_IMAP4) {
		*error_code = EMAIL_ERROR_INVALID_PARAM;
		ret = 0;
		goto FINISH_OFF;
	}

	for (i = 0; i < num; i++) {
		mail_id = mail_ids[i];

		if (!emstorage_get_mail_by_id(multi_user_name, mail_id, &mail, false, &err_code) || !mail) {
			EM_DEBUG_EXCEPTION("emstorage_get_uid_by_mail_id  :  emstorage_get_downloaded_mail failed [%d]", err_code);
			mail = NULL;
			if (err_code == EMAIL_ERROR_MAIL_NOT_FOUND) {	/*  not server mail */
				/* err = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER; */
				/* continue; */
			} else {
				*error_code = err_code;
			}

			ret = 0;
			goto FINISH_OFF;
		}

		if (!emcore_connect_to_remote_mailbox(multi_user_name,
												account_id,
												src_mailbox_id,
												true,
												(void **)&stream,
												&err_code))		/* faizan.h@samsung.com mail_move_fix_07042009 */ {
			EM_DEBUG_EXCEPTION("emcore_move_mail_on_server failed :  Mailbox open[%d]", err_code);
			ret = 0;
			goto FINISH_OFF;
		}

		if (stream) {
			/* set callback for COPY_UID */
			mail_parameters(stream, SET_COPYUID, emcore_mail_copyuid);

			EM_DEBUG_LOG("calling mail_copy_full FODLER MAIL COPY ");

			if (mail->server_mail_id) {
				if (!mail_copy_full(stream, mail->server_mail_id, dest_mailbox, CP_UID | CP_MOVE)) {
					EM_DEBUG_EXCEPTION("emcore_move_mail_on_server :   Mail cannot be moved failed");
					ret = 0;
				} else {
					/*  send EXPUNGE command */
					if (!emcore_imap4_send_command(stream, IMAP4_CMD_EXPUNGE, &err_code)) {
						EM_DEBUG_EXCEPTION("imap4_send_command failed [%d]", err_code);

						if (err_code == EMAIL_ERROR_IMAP4_STORE_FAILURE)
							err_code = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER;
						goto FINISH_OFF;
					}

					if (!emstorage_update_read_mail_uid(multi_user_name, mail_id, g_new_server_uid, mail->server_mailbox_name, &err_code)) {
						EM_DEBUG_EXCEPTION("emstorage_update_read_mail_uid failed [%d]", err_code);
						goto FINISH_OFF;
					}
				}
			} else {
				EM_DEBUG_EXCEPTION(">>>> Server MAIL ID IS NULL >>>> ");
				ret = 0;
				goto FINISH_OFF;
			}
		} else {
			EM_DEBUG_EXCEPTION(">>>> STREAM DATA IS NULL >>> ");
			ret = 0;
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	if (stream)
		stream = mail_close(stream);

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (mail != NULL)
		emstorage_free_mail(&mail, 1, NULL);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emcore_move_mail_on_server_by_server_mail_id(void *mail_stream, char *server_mail_id, char *dest_mailbox_name)
{
	EM_DEBUG_FUNC_BEGIN();
	int err_code = EMAIL_ERROR_NONE;

	if (mail_stream == NULL || server_mail_id == 0 || dest_mailbox_name == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* set callback for COPY_UID */
	mail_parameters((MAILSTREAM*)mail_stream, SET_COPYUID, emcore_mail_copyuid);

	EM_DEBUG_LOG("calling mail_copy_full FODLER MAIL COPY ");

	if (!mail_copy_full((MAILSTREAM*)mail_stream, server_mail_id, dest_mailbox_name, CP_UID | CP_MOVE)) {
		EM_DEBUG_EXCEPTION("emcore_move_mail_on_server :   Mail cannot be moved failed");
		err_code = EMAIL_ERROR_IMAP4_COPY_FAILURE;
	} else {
		/*  send EXPUNGE command */
		if (!emcore_imap4_send_command((MAILSTREAM*)mail_stream, IMAP4_CMD_EXPUNGE, &err_code)) {
			EM_DEBUG_EXCEPTION("imap4_send_command failed [%d]", err_code);

			if (err_code == EMAIL_ERROR_IMAP4_STORE_FAILURE)
				err_code = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER;
			goto FINISH_OFF;
		}
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err_code [%d]", err_code);
	return err_code;
}

static int emcore_copy_mail_to_another_account_on_local_storeage(char *multi_user_name, int input_mail_id, emstorage_mailbox_tbl_t *input_source_mailbox, emstorage_mailbox_tbl_t *input_target_mailbox, int input_task_id, int *output_mail_id)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d] input_source_mailbox[%p] input_target_mailbox[%p] input_task_id [%d] output_mail_id[%p]", input_mail_id, input_source_mailbox, input_target_mailbox, input_task_id, output_mail_id);

	int err = EMAIL_ERROR_NONE;
	int   attachment_count = 0;
	email_mail_data_t       *mail_data = NULL;
	email_attachment_data_t *attachment_data = NULL;

	if (input_source_mailbox == NULL || input_target_mailbox == NULL || input_mail_id <= 0 || output_mail_id == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emcore_get_mail_data(multi_user_name, input_mail_id, &mail_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_data failed [%d]", err);
		err = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	/* Check download status */
	if (!(mail_data->body_download_status & EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED)) {
		/* If not downloaded, download fully */
		if (!emcore_gmime_download_body_sections(multi_user_name,
					NULL,
					input_source_mailbox->account_id,
					input_mail_id,
					(mail_data->attachment_count > 0) ? 1 : 0,
					NO_LIMITATION,
					input_task_id,
					0,
					0,
					&err)) {
			EM_DEBUG_EXCEPTION("emcore_gmime_download_body_sections failed - %d", err);
			goto FINISH_OFF;
		}
	}

	/* Get attachments */
	if ((err = emcore_get_attachment_data_list(multi_user_name, input_mail_id, &attachment_data, &attachment_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_attachment_data_list failed [%d]", err);
		goto FINISH_OFF;
	}

	mail_data->account_id        = input_target_mailbox->account_id;
	mail_data->mail_id           = 0;
	mail_data->mailbox_id        = input_target_mailbox->mailbox_id;
	mail_data->mailbox_type      = input_target_mailbox->mailbox_type;
	mail_data->thread_id         = 0;
	mail_data->thread_item_count = 0;

	if ((err = emcore_add_mail(multi_user_name, mail_data, attachment_data, attachment_count, NULL, false, true)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_add_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	*output_mail_id = mail_data->mail_id;

FINISH_OFF:
	if (mail_data) {
		emcore_free_mail_data(mail_data);
		EM_SAFE_FREE(mail_data); /* prevent 34648 */
	}

	if (attachment_data)
		emcore_free_attachment_data(&attachment_data, attachment_count, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_move_mail_to_another_account(char *multi_user_name, int input_mail_id, int input_source_mailbox_id, int input_target_mailbox_id, int input_task_id)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d] input_source_mailbox_id[%d] input_target_mailbox_id[%d] result_mail_id[%p] input_task_id [%d]", input_mail_id, input_source_mailbox_id, input_target_mailbox_id, input_task_id);
	int err = EMAIL_ERROR_NONE;
	int err_for_delete_mail = EMAIL_ERROR_NONE;
	int moved_mail_id = 0;
	emstorage_mailbox_tbl_t *source_mailbox = NULL;
	emstorage_mailbox_tbl_t *target_mailbox = NULL;
	email_account_t *source_account_ref = NULL;
	email_account_t *target_account_ref = NULL;

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_source_mailbox_id, &source_mailbox)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed for source_mailbox [%d]", err);
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_mailbox_by_id(multi_user_name, input_target_mailbox_id, &target_mailbox)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed for target_mailbox [%d]", err);
		goto FINISH_OFF;
	}

	/* Check account type */
	/* POP  -> IMAP possible */
	/* IMAP -> POP  possible, but the mail would not be on server */
	/* EAS  -> X    impossible */
	/* X    -> EAS  impossible */

	source_account_ref = emcore_get_account_reference(multi_user_name, source_mailbox->account_id, false);

	if (source_account_ref == NULL || source_account_ref->incoming_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
		EM_DEBUG_EXCEPTION("Invalid account");
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	target_account_ref = emcore_get_account_reference(multi_user_name, target_mailbox->account_id, false);

	if (target_account_ref == NULL || target_account_ref->incoming_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
		EM_DEBUG_EXCEPTION("Invalid account");
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}


	if ((err = emcore_copy_mail_to_another_account_on_local_storeage(multi_user_name, input_mail_id, source_mailbox, target_mailbox, input_task_id, &moved_mail_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_copy_mail_to_another_account_on_local_storeage failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_set_flags_field(multi_user_name, source_mailbox->account_id, &input_mail_id, 1, EMAIL_FLAGS_DELETED_FIELD, 1 , &err)) {
		EM_DEBUG_EXCEPTION("emcore_set_flags_field failed [%d]", err);
		goto FINISH_OFF;
	}

	if (target_account_ref->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
		if ((err = emcore_sync_mail_from_client_to_server(multi_user_name, moved_mail_id)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_sync_mail_from_client_to_server failed [%d]", err);

			/* if append is failed, restore source mail and delete copied mail. */
			if (!emcore_set_flags_field(multi_user_name, source_mailbox->account_id, &input_mail_id, 1, EMAIL_FLAGS_DELETED_FIELD, 0 , &err)) {
				EM_DEBUG_EXCEPTION("emcore_set_flags_field failed [%d]", err);
				goto FINISH_OFF;
			}

			if (!emcore_delete_mail(multi_user_name,
									target_mailbox->account_id,
									target_mailbox->mailbox_id,
									&moved_mail_id,
									1,
									EMAIL_DELETE_LOCALLY,
									EMAIL_DELETED_BY_COMMAND,
									0,
									&err_for_delete_mail))
				EM_DEBUG_EXCEPTION("emcore_delete_mail failed [%d]", err_for_delete_mail);
			goto FINISH_OFF;
		}
	}

	if (!emcore_delete_mail(multi_user_name,
							source_mailbox->account_id,
							source_mailbox->mailbox_id,
							&input_mail_id,
							1,
							EMAIL_DELETE_FROM_SERVER,
							EMAIL_DELETED_BY_COMMAND,
							0,
							&err)) {
		EM_DEBUG_EXCEPTION("emcore_delete_mail failed [%d]", err);
		goto FINISH_OFF;
	}


FINISH_OFF:
	if (source_mailbox)
		emstorage_free_mailbox(&source_mailbox, 1, NULL);

	if (target_mailbox)
		emstorage_free_mailbox(&target_mailbox, 1, NULL);

	if (source_account_ref) {
		emcore_free_account(source_account_ref);
		EM_SAFE_FREE(source_account_ref);
	}

	if (target_account_ref) {
		emcore_free_account(target_account_ref);
		EM_SAFE_FREE(target_account_ref);
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_save_mail_file(char *multi_user_name, int account_id, int mail_id, int attachment_id,
										char *src_file_path, char *file_name, char *full_path, char *virtual_path,
										int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], attachment_id[%d], file_name[%p], full_path[%p], err_code[%p]",
						account_id, mail_id, attachment_id, file_name, full_path, err_code);

	int err = EMAIL_ERROR_NONE;

	if (!file_name || !full_path || !src_file_path) {
		EM_DEBUG_EXCEPTION("Invalid paramter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_create_dir(multi_user_name, account_id, mail_id, attachment_id, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emstorage_get_save_name(multi_user_name,
								account_id,
								mail_id,
								attachment_id,
								file_name,
								full_path,
								virtual_path,
								MAX_PATH,
								&err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
		goto FINISH_OFF;
	}

	if (strcmp(src_file_path, full_path) != 0) {
		if (!emstorage_copy_file(src_file_path, full_path, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_copy_file failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	if (err_code)
		*err_code = err;

	return 1;
}

/* description : update mail information */
INTERNAL_FUNC int emcore_update_mail(char *multi_user_name, email_mail_data_t *input_mail_data,
									email_attachment_data_t *input_attachment_data_list,
									int input_attachment_count,
									email_meeting_request_t* input_meeting_request,
									int input_from_eas)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list[%p], input_attachment_count[%d], "
						"input_meeting_request[%p], input_from_eas[%d]",
						input_mail_data, input_attachment_data_list, input_attachment_count,
						input_meeting_request, input_from_eas);

	char                   filename_buf[1024]          = {0, };
	char                   virtual_path[1024]          = {0, };
	char                   mailbox_id_param_string[10] = {0, };
	char                  *body_text_file_name         = NULL;
	int                    i                           = 0;
	int                    err                         = EMAIL_ERROR_NONE;
	int                    local_inline_content_count  = 0;
	emstorage_mail_tbl_t  *converted_mail_tbl_data     = NULL;
	email_meeting_request_t *meeting_req               = NULL;
	struct stat            st_buf;
	int ori_attachment_count                           = 0;
	int *temp_attachment_id_array                      = NULL;
	email_attachment_data_t *ori_attachment_data_list  = NULL;
    char                   *prefix_path                = NULL;
    char                   real_file_path[MAX_PATH]    = {0};

	if (!input_mail_data || (input_attachment_count && !input_attachment_data_list) ||
		(!input_attachment_count && input_attachment_data_list)) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF2;
	}

    if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		err = emcore_get_container_path(multi_user_name, &prefix_path);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_container_path failed : [%d]", err);
			goto FINISH_OFF2;
		}
	} else {
		prefix_path = strdup("");
	}

	if (input_from_eas == 0) {
		if (input_mail_data->file_path_plain) {
            memset(real_file_path, 0x00, sizeof(real_file_path));
            SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_mail_data->file_path_plain);

			if (stat(real_file_path, &st_buf) < 0) {
				EM_DEBUG_EXCEPTION_SEC("input_mail_data->file_path_plain, stat(\"%s\") failed...", input_mail_data->file_path_plain);
				err = EMAIL_ERROR_FILE_NOT_FOUND;
				goto FINISH_OFF2;
			}
		}

		if (input_mail_data->file_path_html) {
            memset(real_file_path, 0x00, sizeof(real_file_path));
            SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_mail_data->file_path_html);

			if (stat(real_file_path, &st_buf) < 0) {
				EM_DEBUG_EXCEPTION_SEC("input_mail_data->file_path_html, stat(\"%s\") failed...", input_mail_data->file_path_html);
				err = EMAIL_ERROR_FILE_NOT_FOUND;
				goto FINISH_OFF2;
			}
		}

		if (input_attachment_count && input_attachment_data_list) {
			for (i = 0; i < input_attachment_count; i++) {
				if (input_attachment_data_list[i].save_status) {
                    memset(real_file_path, 0x00, sizeof(real_file_path));
                    SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_attachment_data_list[i].attachment_path);

					if (!input_attachment_data_list[i].attachment_path || stat(real_file_path, &st_buf) < 0) {
						EM_DEBUG_EXCEPTION("stat(\"%s\") failed...", input_attachment_data_list[i].attachment_path);
						err = EMAIL_ERROR_FILE_NOT_FOUND;
						goto FINISH_OFF2;
					}
				}
			}
		}
	}

	if (input_mail_data->mail_size == 0) {
		 emcore_calc_mail_size(multi_user_name, input_mail_data, input_attachment_data_list, input_attachment_count, &(input_mail_data->mail_size));
	}

	if (input_mail_data->file_path_plain) {
		/* Save plain text body. */
		if ((err = em_get_file_name_from_file_path(input_mail_data->file_path_plain, &body_text_file_name)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_get_file_name_from_file_path failed [%d]", err);
			err = EMAIL_ERROR_INVALID_FILE_PATH;
			goto FINISH_OFF2;
		}

		if (!emcore_save_mail_file(multi_user_name,
                                    input_mail_data->account_id,
                                    input_mail_data->mail_id,
									0,
                                    input_mail_data->file_path_plain,
                                    body_text_file_name,
                                    filename_buf,
                                    virtual_path,
                                    &err)) {
			EM_DEBUG_EXCEPTION("emcore_save_mail_file failed [%d]", err);
			goto FINISH_OFF2;
		}
		EM_SAFE_FREE(input_mail_data->file_path_plain);
		input_mail_data->file_path_plain = EM_SAFE_STRDUP(virtual_path);
	}

	if (input_mail_data->file_path_html) {
		/*  Save HTML text body. */
		EM_SAFE_FREE(body_text_file_name);
		if ((err = em_get_file_name_from_file_path(input_mail_data->file_path_html, &body_text_file_name)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_get_file_name_from_file_path failed [%d]", err);
			err = EMAIL_ERROR_INVALID_FILE_PATH;
			goto FINISH_OFF2;
		}

		if (!emcore_save_mail_file(multi_user_name,
                                    input_mail_data->account_id,
                                    input_mail_data->mail_id,
									0,
                                    input_mail_data->file_path_html,
                                    body_text_file_name,
                                    filename_buf,
                                    virtual_path,
                                    &err)) {
			EM_DEBUG_EXCEPTION("emcore_save_mail_file failed [%d]", err);
			goto FINISH_OFF2;
		}
		EM_SAFE_FREE(input_mail_data->file_path_html);
		input_mail_data->file_path_html = EM_SAFE_STRDUP(virtual_path);
	}

	if (input_mail_data->file_path_mime_entity) {
		/*  Save mime entity. */
		if (!emcore_save_mail_file(multi_user_name,
                                    input_mail_data->account_id,
                                    input_mail_data->mail_id,
									0,
                                    input_mail_data->file_path_mime_entity,
                                    "mime_entity",
                                    filename_buf,
                                    virtual_path,
                                    &err)) {
			EM_DEBUG_EXCEPTION("emcore_save_mail_file failed [%d]", err);
			goto FINISH_OFF2;
		}
		EM_SAFE_FREE(input_mail_data->file_path_mime_entity);
		input_mail_data->file_path_mime_entity = EM_SAFE_STRDUP(virtual_path);
	}

	if (input_attachment_data_list && input_attachment_count) {
		int i = 0;
		int j = 0;
		int compare_result = 1;
		email_attachment_data_t *temp_attachment_data = NULL;

		if ((err = emcore_get_attachment_data_list(multi_user_name, input_mail_data->mail_id, &ori_attachment_data_list, &ori_attachment_count)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_attachment_data_list failed : [%d]", err);
		}

		temp_attachment_id_array = em_malloc(sizeof(int) * input_attachment_count);
		if (temp_attachment_id_array == NULL) {
			EM_DEBUG_EXCEPTION("em_mallocfailed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF2;
		}

		for (i = 0; i < input_attachment_count; i++) {
			temp_attachment_data = input_attachment_data_list + i;
			if ((err = emcore_mail_compare_filename_of_attachment_data(multi_user_name, input_mail_data->mail_id, \
				temp_attachment_data->attachment_id, temp_attachment_data, &compare_result)) != EMAIL_ERROR_NONE) {
				if (err == EMAIL_ERROR_ATTACHMENT_NOT_FOUND)
					EM_DEBUG_LOG("no attachment found");
				else
					EM_DEBUG_EXCEPTION("emcore_mail_compare_filename_of_attachment_data failed [%d]", err);
			}

			switch (compare_result) {
			case 0:
				EM_DEBUG_LOG("file name and attachment id are same, update exising attachment");
				if (!emcore_mail_update_attachment_data(multi_user_name, input_mail_data->mail_id, temp_attachment_data)) {
					EM_DEBUG_EXCEPTION("emcore_mail_update_attachment_data failed [%d]", err);
					goto FINISH_OFF2;
				}
				temp_attachment_id_array[i] = temp_attachment_data->attachment_id;
				break;
			case 1:
				EM_DEBUG_LOG("save names are different");
				if ((err = emcore_add_attachment_data(multi_user_name, input_mail_data->mail_id, temp_attachment_data)) != EMAIL_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("emcore_add_attachment failed [%d]", err);
					goto FINISH_OFF2;
				}

				temp_attachment_id_array[i] = temp_attachment_data->attachment_id;
				break;
			case 2:
				EM_DEBUG_LOG("No chagned the attachment info");
				temp_attachment_id_array[i] = temp_attachment_data->attachment_id;
				break;
			}

			if (temp_attachment_data->inline_content_status)
				local_inline_content_count++;
		}

		for (i = 0; i < ori_attachment_count; i++) {
			emstorage_attachment_tbl_t *temp_attachment_tbl_t = NULL;

			temp_attachment_data = ori_attachment_data_list + i;
			compare_result = 0;

			for (j = 0; j < input_attachment_count; j++) {
				if (!emstorage_get_attachment(multi_user_name, temp_attachment_id_array[j], &temp_attachment_tbl_t, false, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_get_attachment failed : [%d]", err);
					continue;
				}

				if (temp_attachment_id_array[j] == temp_attachment_data->attachment_id) {
					compare_result = 1;
					break;
				}

				if ((temp_attachment_data->inline_content_status == INLINE_ATTACHMENT) && (strcmp(temp_attachment_tbl_t->attachment_name, temp_attachment_data->attachment_name) == 0))
					compare_result = 2;

				emstorage_free_attachment(&temp_attachment_tbl_t, 1, NULL);

			}

			switch (compare_result) {
			case 0: /* Delete the attachment on db and file */
				if (!emcore_delete_mail_attachment(multi_user_name, temp_attachment_data->attachment_id, &err)) {
					EM_DEBUG_EXCEPTION("emcore_delete_mail_attachment failed [%d]", err);
				}
				break;
			case 2: /* Delete the attachment on db */
				if (!emstorage_delete_attachment_on_db(multi_user_name, temp_attachment_data->attachment_id, true, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_delete_attachment_on_db failed : [%d]", err);
				}
				break;
			case 1:
				break;
			}

			emstorage_free_attachment(&temp_attachment_tbl_t, 1, NULL);
		}
	}

	input_mail_data->attachment_count     = input_attachment_count - local_inline_content_count;
	input_mail_data->inline_content_count = local_inline_content_count;

	if (!input_mail_data->date_time) {
		/* time isn't set */
		input_mail_data->date_time = time(NULL);
	}

	EM_DEBUG_LOG("preview_text[%p]", input_mail_data->preview_text);
	if (input_mail_data->preview_text == NULL) {
		if ((err = emcore_get_preview_text_from_file(multi_user_name,
                                                    input_mail_data->file_path_plain,
                                                    input_mail_data->file_path_html,
                                                    MAX_PREVIEW_TEXT_LENGTH,
                                                    &(input_mail_data->preview_text))) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_preview_text_from_file failed[%d]", err);
			if (err != EMAIL_ERROR_EMPTY_FILE)
				goto FINISH_OFF2;
		}
	}

	if (!em_convert_mail_data_to_mail_tbl(input_mail_data, 1, &converted_mail_tbl_data,  &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_data_to_mail_tbl failed[%d]", err);
		goto FINISH_OFF2;
	}

	/*  BEGIN TRANSACTION; */
	if (!emstorage_begin_transaction(multi_user_name, NULL, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_begin_transaction failed [%d]", err);
		goto FINISH_OFF2;
	}

	if (!emstorage_change_mail_field(multi_user_name, input_mail_data->mail_id, UPDATE_MAIL, converted_mail_tbl_data, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);
		goto FINISH_OFF;
	}

#ifdef __FEATURE_BODY_SEARCH__
	/* strip html content and save into mail_text_tbl */
	char *stripped_text = NULL;
	if (!emcore_strip_mail_body_from_file(multi_user_name, converted_mail_tbl_data, &stripped_text, &err) || stripped_text == NULL) {
		EM_DEBUG_EXCEPTION("emcore_strip_mail_body_from_file failed [%d]", err);
	}

	emstorage_mail_text_tbl_t *mail_text;

	if (!emstorage_get_mail_text_by_id(multi_user_name, input_mail_data->mail_id, &mail_text, true, &err) || !mail_text) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_text_by_id failed [%d]", err);
		EM_SAFE_FREE(stripped_text);
		goto FINISH_OFF;
	}

	EM_SAFE_FREE(mail_text->body_text);
	mail_text->body_text = stripped_text;

	if (!emstorage_change_mail_text_field(multi_user_name, input_mail_data->mail_id, mail_text, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_change_mail_text_field failed [%d]", err);
		emstorage_free_mail_text(&mail_text, 1, NULL); /*prevent 17955*/
		goto FINISH_OFF;
	}

	if (mail_text)
		emstorage_free_mail_text(&mail_text, 1, NULL);
#endif

	if (input_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_REQUEST
		|| input_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_RESPONSE
		|| input_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		/*  check where there is a meeting request in DB */
		if (!emstorage_get_meeting_request(multi_user_name, input_mail_data->mail_id, &meeting_req, false, &err) && err != EMAIL_ERROR_DATA_NOT_FOUND) {
			EM_DEBUG_EXCEPTION("emstorage_get_meeting_request failed [%d]", err);
			goto FINISH_OFF;
		}

		if (err == EMAIL_ERROR_DATA_NOT_FOUND) {	/*  insert */
			emstorage_mail_tbl_t *original_mail = NULL;

			if (!emstorage_get_mail_by_id(multi_user_name, input_mail_data->mail_id, &original_mail, false, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
				goto FINISH_OFF;
			}

			if (original_mail)	{
				if (!emstorage_add_meeting_request(multi_user_name, input_mail_data->account_id, original_mail->mailbox_id, input_meeting_request, false, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_add_meeting_request failed [%d]", err);
					emstorage_free_mail(&original_mail, 1, NULL);
					goto FINISH_OFF;
				}
					emstorage_free_mail(&original_mail, 1, NULL);
			}
		} else {	/*  update */
			if (!emstorage_update_meeting_request(multi_user_name, input_meeting_request, false, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_update_meeting_request failed [%d]", err);
				goto FINISH_OFF;
			}
		}
	}

FINISH_OFF:

	if (err == EMAIL_ERROR_NONE) {
		/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(multi_user_name, NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
		}

		SNPRINTF(mailbox_id_param_string, 10, "%d", input_mail_data->mailbox_id);

		if (!emcore_notify_storage_event(NOTI_MAIL_UPDATE, input_mail_data->account_id, input_mail_data->mail_id, mailbox_id_param_string, input_mail_data->meeting_request_status ? UPDATE_MEETING : UPDATE_MAIL))
			EM_DEBUG_EXCEPTION(" emcore_notify_storage_eventfailed [NOTI_MAIL_UPDATE] ");
	} else {
		/*  ROLLBACK TRANSACTION; */
		if (emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL) == false)
			err = EMAIL_ERROR_DB_FAILURE;
	}

FINISH_OFF2:

	EM_SAFE_FREE(body_text_file_name);
	EM_SAFE_FREE(temp_attachment_id_array);
	EM_SAFE_FREE(prefix_path);

	if (ori_attachment_data_list)
		emcore_free_attachment_data(&ori_attachment_data_list, ori_attachment_count, NULL);

	if (meeting_req) {
		emstorage_free_meeting_request(meeting_req);
		EM_SAFE_FREE(meeting_req);
	}

	if (converted_mail_tbl_data)
		emstorage_free_mail(&converted_mail_tbl_data, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


INTERNAL_FUNC int emcore_set_flags_field(char *multi_user_name, int account_id, int mail_ids[], int num, email_flags_field_type field_type, int value, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mail_ids[%p], num [%d], field_type [%d], value[%d], err_code[%p]", account_id, mail_ids, num, field_type, value, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	char *field_type_name[EMAIL_FLAGS_FIELD_COUNT] = { "flags_seen_field"
		, "flags_deleted_field", "flags_flagged_field", "flags_answered_field"
		, "flags_recent_field", "flags_draft_field", "flags_forwarded_field" };

	if (field_type < 0 || field_type >= EMAIL_FLAGS_FIELD_COUNT || mail_ids == NULL || num <= 0 || account_id == 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto  FINISH_OFF;
	}

	if (!emstorage_set_field_of_mails_with_integer_value(multi_user_name, account_id, mail_ids, num, field_type_name[field_type], value, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err);
		goto FINISH_OFF;
	}

	if (field_type == EMAIL_FLAGS_SEEN_FIELD)
		emcore_display_unread_in_badge(multi_user_name);

	ret = true;

FINISH_OFF:

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}

int emcore_mail_cmd_read_mail_pop3(void *stream, int msgno, int limited_size, int *downloded_size, int *result_total_body_size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msgno[%d], limited_size[%d], err_code[%p]", stream, msgno, limited_size, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int total_body_size = 0;
	char command[32];
	char *response = NULL;
	POP3LOCAL *pop3local;

	if (!stream || !result_total_body_size) {
		EM_DEBUG_EXCEPTION("stream[%p], total_body_size[%p]", stream, msgno, result_total_body_size);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	pop3local = (POP3LOCAL *)(((MAILSTREAM *)stream)->local);

	if (!pop3local  || !pop3local->netstream) {
		err = EMAIL_ERROR_INVALID_STREAM;
		goto FINISH_OFF;
	}
	memset(command, 0x00, sizeof(command));

	SNPRINTF(command, sizeof(command), "LIST %d\015\012", msgno);

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG("[POP3] >>> [%s]", command);
#endif


	/*  send command  :  LIST [msgno] - to get the size of the mail */
	if (!net_sout(pop3local->netstream, command, (int)EM_SAFE_STRLEN(command))) {
		EM_DEBUG_EXCEPTION("net_sout failed...");
		err = EMAIL_ERROR_INVALID_RESPONSE;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("Sending command success");

	while (pop3local->netstream) {
		/*  receive response */
		if (!(response = net_getline(pop3local->netstream)))
			break;

		if (response)
			EM_DEBUG_LOG("[POP3] <<< %s", response);

		if (*response == '.') {
			EM_SAFE_FREE(response);
			break;
		}

		if (*response == '+') {		/*  "+ OK" */
			char *p = NULL;

			if (!(p = strchr(response + strlen("+OK "), ' '))) {
				err = EMAIL_ERROR_INVALID_RESPONSE;
				goto FINISH_OFF;
			}

			total_body_size = atoi(p + 1);
			EM_DEBUG_LOG("Body size [%d]", total_body_size);

			if (result_total_body_size) {
				*result_total_body_size = total_body_size;
				break;
			}
		} else if (*response == '-') {	/*  "- ERR" */
			err = EMAIL_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		} else {
			err = EMAIL_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}

		EM_SAFE_FREE(response);
	}

	memset(command, 0x00, sizeof(command));

	if (limited_size && total_body_size > limited_size) {
		int count_of_line = limited_size / 80;
		SNPRINTF(command, sizeof(command), "TOP %d %d", msgno, count_of_line);
	} else
		SNPRINTF(command, sizeof(command), "RETR %d", msgno);

	EM_DEBUG_LOG("[POP3] >>> %s", command);

	emcore_set_network_error(EMAIL_ERROR_NONE);		/*  set current network error as EMAIL_ERROR_NONE before network operation */
	/*  get mail from mail server */
	if (!pop3_send((MAILSTREAM *)stream, command, NULL)) {
		EM_DEBUG_EXCEPTION("pop3_send failed...");

		email_session_t *session = NULL;

		if (!emcore_get_current_session(&session)) {
			EM_DEBUG_EXCEPTION("emcore_get_current_session failed...");
			err = EMAIL_ERROR_SESSION_NOT_FOUND;
			goto FINISH_OFF;
		}

		if (session->network == EMAIL_ERROR_NONE)
			err = EMAIL_ERROR_UNKNOWN;
		else
			err = session->network;

		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE(response);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}


INTERNAL_FUNC int emcore_sync_flag_with_server(char *multi_user_name, int mail_id, int event_handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%p], err_code[%p]", mail_id, err_code);

	if (mail_id < 1) {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	MAILSTREAM *stream = NULL;
	email_internal_mailbox_t mailbox = {0};
	emstorage_mail_tbl_t *mail = NULL;
	email_account_t *ref_account = NULL;
	int account_id = 0;
	int msgno = 0;
	char set_flags[100] = { 0, };
	char clear_flags[100] = { 0, };
	char tmp[100] = { 0, };

	FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	if (!emstorage_get_mail_by_id(multi_user_name, mail_id, &mail, true, &err) || !mail) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	account_id = mail->account_id;

	if (!(ref_account = emcore_get_account_reference(multi_user_name, account_id, false)))  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	/*  open mail server. */
	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											account_id,
											mail->mailbox_id,
											true,
											(void **)&stream,
											&err) || !stream) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}

	FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	mailbox.mailbox_id = mail->mailbox_id;
	mailbox.account_id = account_id;
	mailbox.mail_stream = stream;

	/*  download all uids from server. */
	if (!emcore_get_mail_msgno_by_uid(ref_account, &mailbox, mail->server_mail_id, &msgno, &err)) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_msgno_by_uid failed message_no  :  %d ", err);
		goto FINISH_OFF;
	}

	snprintf(tmp, sizeof(tmp),"%d", msgno);

	if (mail->flags_seen_field)
		snprintf(set_flags, sizeof(set_flags), "\\Seen");
	else
		snprintf(clear_flags, sizeof(clear_flags),"\\Seen");

	if (mail->flags_answered_field)
		snprintf(set_flags, sizeof(set_flags),"%s \\Answered", set_flags);
	else
		snprintf(clear_flags, sizeof(clear_flags),"%s \\Answered", clear_flags);

	if (mail->flags_flagged_field)
		snprintf(set_flags, sizeof(set_flags), "%s \\Flagged", set_flags);
	else
		snprintf(clear_flags, sizeof(clear_flags),"%s \\Flagged", clear_flags);

	if (mail->flags_forwarded_field)
		snprintf(set_flags, sizeof(set_flags),"%s $Forwarded", set_flags);
	else
		snprintf(clear_flags, sizeof(clear_flags),"%s $Forwarded", clear_flags);

	if (EM_SAFE_STRLEN(set_flags) > 0) {
		EM_DEBUG_LOG(">>>> Calling mail_setflag [%s] ", set_flags);
		mail_flag(stream, tmp, set_flags, ST_SET | ST_SILENT);
		EM_DEBUG_LOG(">>>> End mail_setflag ");
	}

	if (EM_SAFE_STRLEN(clear_flags) > 0) {
		EM_DEBUG_LOG(">>>> Calling mail_clearflag [%s]", clear_flags);
		mail_clearflag(stream, tmp, clear_flags);
		EM_DEBUG_LOG(">>>> End mail_clearflag ");
	}

	if (mail->lock_status) {
		memset(set_flags, 0x00, 100);
		snprintf(set_flags, sizeof(set_flags), "Sticky");
		if (EM_SAFE_STRLEN(set_flags) > 0) {
			EM_DEBUG_LOG(">>>> Calling mail_setflag [%s]", set_flags);
			mail_flag(stream, tmp, set_flags, ST_SET | ST_SILENT);
			EM_DEBUG_LOG(">>>> End mail_setflag ");
		}
	}

	EM_DEBUG_LOG(">>>> Returning from emcore_sync_flag_with_server ");

	FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	ret = true;

FINISH_OFF:

	if (stream)
		stream = mail_close(stream);

	if (mail)
		emstorage_free_mail(&mail, 1, NULL);

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}

INTERNAL_FUNC int emcore_sync_seen_flag_with_server(char *multi_user_name, int mail_ids[], int num, int event_handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], err_code[%p]", mail_ids[0], err_code);

	if (mail_ids[0] < 1) {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	MAILSTREAM *stream = NULL;
	email_internal_mailbox_t mailbox;
	emstorage_mail_tbl_t *mail = NULL;
	email_account_t *ref_account = NULL;
	int account_id = 0;
	int msgno = 0;
	char set_flags[100];
	char clear_flags[100];
	char tmp[100];
	int mail_id = 0;
	int i = 0;

	memset(&mailbox, 0x00, sizeof(email_internal_mailbox_t)); /*prevent 20575*/

	mail_id = mail_ids[0];

	if (!emstorage_get_mail_by_id(multi_user_name, mail_id, &mail, true, &err) || !mail) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	account_id = mail->account_id;

	if (!(ref_account = emcore_get_account_reference(multi_user_name, account_id, false))) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/*  open mail server. */
	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											account_id,
											mail->mailbox_id,
											true,
											(void **)&stream,
											&err) || !stream) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}

	mailbox.mailbox_id = mail->mailbox_id;
	mailbox.account_id = account_id;
	mailbox.mail_stream = stream;

	for (i = 0; i < num; i++) {
		mail_id = mail_ids[i];

		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		if (!emstorage_get_mail_by_id(multi_user_name, mail_id, &mail, true, &err) || !mail) {
			EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
			goto FINISH_OFF;
		}

		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		/*  download message number from server. */
		if (!emcore_get_mail_msgno_by_uid(ref_account, &mailbox, mail->server_mail_id, &msgno, &err)) {
			EM_DEBUG_LOG("emcore_get_mail_msgno_by_uid failed message_no  :  %d ", err);
			goto FINISH_OFF;
		}

		memset(tmp, 0x00, 100);
		snprintf(tmp, sizeof(tmp),"%d", msgno);

		memset(set_flags, 0x00, 100);
		memset(clear_flags, 0x00, 100);

		if (mail->flags_seen_field)
			snprintf(set_flags, sizeof(set_flags),"\\Seen");
		else
			snprintf(clear_flags, sizeof(clear_flags),"\\Seen");
		EM_DEBUG_LOG("new_flag.seen :  %s ", set_flags);

		if (EM_SAFE_STRLEN(set_flags) > 0) {
			EM_DEBUG_LOG(">>>> Calling mail_setflag ");
			mail_flag(stream, tmp, set_flags, ST_SET | ST_SILENT);
			EM_DEBUG_LOG(">>>> End mail_setflag ");
		} else {
			EM_DEBUG_LOG(">>>> Calling mail_clearflag ");
			mail_clearflag(stream, tmp, clear_flags);
			EM_DEBUG_LOG(">>>> End mail_clearflag ");
		}

		EM_DEBUG_LOG(">>>> Returning from emcore_sync_flag_with_server ");

		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);
	}
	ret = true;

FINISH_OFF:

	if (stream)
		stream = mail_close(stream);
	if (mail)
		emstorage_free_mail(&mail, 1, NULL);

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}

INTERNAL_FUNC void emcore_free_mail_data_list(email_mail_data_t **mail_list, int count)
{
	EM_DEBUG_FUNC_BEGIN("count[%d]", count);

	if (count <= 0 || !mail_list || !*mail_list)
		return;

	email_mail_data_t* p = *mail_list;
	int i;

	for (i = 0; i < count; i++)
		emcore_free_mail_data(p+i);

	EM_SAFE_FREE(*mail_list);

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void emcore_free_mail_data(email_mail_data_t *mail_data)
{
	EM_DEBUG_FUNC_BEGIN();

	if (!mail_data) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		return;
	}

	EM_SAFE_FREE(mail_data->subject);
	EM_SAFE_FREE(mail_data->server_mailbox_name);
	EM_SAFE_FREE(mail_data->server_mail_id);
	EM_SAFE_FREE(mail_data->message_id);
	EM_SAFE_FREE(mail_data->full_address_from);
	EM_SAFE_FREE(mail_data->full_address_reply);
	EM_SAFE_FREE(mail_data->full_address_to);
	EM_SAFE_FREE(mail_data->full_address_cc);
	EM_SAFE_FREE(mail_data->full_address_bcc);
	EM_SAFE_FREE(mail_data->full_address_return);
	EM_SAFE_FREE(mail_data->email_address_sender);
	EM_SAFE_FREE(mail_data->email_address_recipient);
	EM_SAFE_FREE(mail_data->alias_sender);
	EM_SAFE_FREE(mail_data->alias_recipient);
	EM_SAFE_FREE(mail_data->file_path_plain);
	EM_SAFE_FREE(mail_data->file_path_html);
	EM_SAFE_FREE(mail_data->file_path_mime_entity);
	EM_SAFE_FREE(mail_data->preview_text);
	EM_SAFE_FREE(mail_data->pgp_password);
	EM_SAFE_FREE(mail_data->eas_data);

	EM_DEBUG_FUNC_END();
}


INTERNAL_FUNC int emcore_free_attachment_data(email_attachment_data_t **attachment_data_list, int attachment_data_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_data_list[%p], attachment_data_count [%d], err_code[%p]", attachment_data_list, attachment_data_count, err_code);

	if (!attachment_data_list || !*attachment_data_list) {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	email_attachment_data_t* p = *attachment_data_list;
	int i = 0;

	for (i = 0; i < attachment_data_count; i++) {
		EM_SAFE_FREE(p[i].attachment_name);
		EM_SAFE_FREE(p[i].attachment_path);
		EM_SAFE_FREE(p[i].attachment_mime_type);
		EM_SAFE_FREE(p[i].content_id);
	}

	EM_SAFE_FREE(p); *attachment_data_list = NULL;

	if (err_code)
		*err_code = EMAIL_ERROR_NONE;

	EM_DEBUG_FUNC_END();
	return true;
}



#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

INTERNAL_FUNC int emcore_delete_pbd_activity(char *multi_user_name, int account_id, int mail_id, int activity_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], err_code[%p]", account_id, mail_id, err_code);

	if (account_id < FIRST_ACCOUNT_ID || mail_id < 0) {
		EM_DEBUG_EXCEPTION("account_id[%d], mail_id[%d], err_code[%p]", account_id, mail_id, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int before_tr_begin = 0;

	if (!emstorage_begin_transaction(multi_user_name, NULL, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_begin_transaction failed [%d]", err);
		before_tr_begin = 1;
		goto FINISH_OFF;
	}

	if (!emstorage_delete_pbd_activity(multi_user_name, account_id, mail_id, activity_id, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_delete_pbd_activity failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(multi_user_name, NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
			ret = false;
		}
	} else {	/*  ROLLBACK TRANSACTION; */
		if (!before_tr_begin && emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL) == false)
			err = EMAIL_ERROR_DB_FAILURE;
	}

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_insert_pbd_activity(email_event_partial_body_thd *local_activity, int *activity_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("local_activity[%p], activity_id[%p], err_code[%p]", local_activity, activity_id, err_code);

	if (!local_activity || !activity_id) {
		EM_DEBUG_EXCEPTION("local_activity[%p], activity_id[%p] err_code[%p]", local_activity, activity_id, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int before_tr_begin = 0;

	if (!emstorage_begin_transaction(local_activity->multi_user_name, NULL, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_begin_transaction failed [%d]", err);
		before_tr_begin = 1;
		goto FINISH_OFF;
	}

	if (!emstorage_add_pbd_activity(local_activity->multi_user_name, local_activity, activity_id, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_add_pbd_activity failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(local_activity->multi_user_name, NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
			ret = false;
		}
	} else {	/*  ROLLBACK TRANSACTION; */
		if (!before_tr_begin && emstorage_rollback_transaction(local_activity->multi_user_name, NULL, NULL, NULL) == false)
			err = EMAIL_ERROR_DB_FAILURE;
	}

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

#endif

#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__

/* API to set or unset a field of flags on server in single IMAP request to server */

INTERNAL_FUNC int emcore_sync_flags_field_with_server(char *multi_user_name, int mail_ids[], int num, email_flags_field_type field_type, int value, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids [%p], num [%d], field_type [%d], value [%d], err_code [%p]", mail_ids, num, field_type, value, err_code);

	if (NULL == mail_ids || num <= 0 || field_type < 0 || field_type >= EMAIL_FLAGS_FIELD_COUNT) {
			EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL) {
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		}
		return false;
	}

	MAILSTREAM *stream = NULL;
	IMAPLOCAL *imaplocal = NULL;
	char tag[MAX_TAG_SIZE] = {0, };
	char cmd[MAX_IMAP_COMMAND_LENGTH] = {0, };
	char *p = NULL;
	char **string_list = NULL;
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int account_id = 0;
	int mail_id = 0;
	int i = 0;
	int id_set_count = 0;
	int len_of_each_range = 0;
	int string_count = 0;
	email_account_t *temp_account = NULL;
	email_id_set_t *id_set = NULL;
	emstorage_mail_tbl_t *mail = NULL;
	email_uid_range_set *uid_range_set = NULL;
	email_uid_range_set *uid_range_node = NULL;
	char *field_type_name[EMAIL_FLAGS_FIELD_COUNT] = { "\\Seen"
		, "\\Deleted", "\\Flagged", "\\Answered"
		, "\\Recent", "\\Draft", "$Forwarded" };

	mail_id = mail_ids[0];

	if (!emstorage_get_mail_field_by_id(multi_user_name, mail_id, RETRIEVE_FLAG, &mail, true, &err) || !mail) 		/*To DO :  This is a existing bug. on mail deletion before this call it will fail always */ {
		EM_DEBUG_LOG("emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	account_id = mail[0].account_id;

	temp_account = emcore_get_account_reference(multi_user_name, account_id, false);
	if (!temp_account)  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (temp_account->incoming_server_type != EMAIL_SERVER_TYPE_IMAP4) {
		EM_DEBUG_EXCEPTION("Syncing seen flag is available only for IMAP4 server. The server type [%d]", temp_account->incoming_server_type);
		err = EMAIL_ERROR_NOT_SUPPORTED;
		goto FINISH_OFF;
	}

	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											account_id,
											mail->mailbox_id,
											true,
											(void **)&stream,
											&err) || !stream) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}

	if (false == emstorage_free_mail(&mail, 1, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_free_mail failed - %d ", err);
		goto FINISH_OFF;
	}

	/* [h.gahlaut] Break the set of mail_ids into comma separated strings of given length */
	/* Length is decided on the basis of remaining keywords in the Query to be formed later on in emstorage_get_id_set_from_mail_ids */
	/* Here about 90 bytes are required for fixed keywords in the query-> SELECT local_uid, s_uid from mail_read_mail_uid_tbl where local_uid in (....) ORDER by s_uid */
	/* So length of comma separated strings which will be filled in (.....) in above query can be maximum QUERY_SIZE - 90 */

	if (false == emcore_form_comma_separated_strings(mail_ids, num, QUERY_SIZE - 90, &string_list, &string_count, &err))  {
		EM_DEBUG_EXCEPTION("emcore_form_comma_separated_strings failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Now execute one by one each comma separated string of mail_ids */

	for (i = 0; i < string_count; ++i) {
		/* Get the set of mail_ds and corresponding server_mail_ids sorted by server mail ids in ascending order */
		if (false == emstorage_get_id_set_from_mail_ids(multi_user_name, string_list[i], &id_set, &id_set_count, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_id_set_from_mail_ids failed [%d]", err);
			goto FINISH_OFF;
		}

		/* Convert the sorted sequence of server mail ids to range sequences of given length. A range sequence will be like A : B, C, D: E, H */

		len_of_each_range = MAX_IMAP_COMMAND_LENGTH - 40;		/*  1000 is the maximum length allowed RFC 2683. 40 is left for keywords and tag. */

		if (false == emcore_convert_to_uid_range_set(id_set, id_set_count, &uid_range_set, len_of_each_range, &err)) {
			EM_DEBUG_EXCEPTION("emcore_convert_to_uid_range_set failed [%d]", err);
			goto FINISH_OFF;
		}

		uid_range_node = uid_range_set;

		while (uid_range_node != NULL) {
			/* Remove comma from end of uid_range */

			uid_range_node->uid_range[EM_SAFE_STRLEN(uid_range_node->uid_range) - 1] = '\0';

			/* Form the IMAP command */

			SNPRINTF(tag, MAX_TAG_SIZE, "%08lx", 0xffffffff & (stream->gensym++));

			if (value)
				SNPRINTF(cmd, MAX_IMAP_COMMAND_LENGTH, "%s UID STORE %s +FLAGS (%s)\015\012", tag, uid_range_node->uid_range, field_type_name[field_type]);
			else
				SNPRINTF(cmd, MAX_IMAP_COMMAND_LENGTH, "%s UID STORE %s -FLAGS (%s)\015\012", tag, uid_range_node->uid_range, field_type_name[field_type]);

			EM_DEBUG_LOG("[IMAP4] command %s", cmd);

			/* send command */

			if (!(imaplocal = stream->local) || !imaplocal->netstream) {
				EM_DEBUG_EXCEPTION("invalid IMAP4 stream detected...");

				err = EMAIL_ERROR_UNKNOWN;
				goto FINISH_OFF;
			}

			if (!net_sout(imaplocal->netstream, cmd, (int)EM_SAFE_STRLEN(cmd))) {
				EM_DEBUG_EXCEPTION("net_sout failed...");
				err = EMAIL_ERROR_CONNECTION_BROKEN;
				goto FINISH_OFF;
			}

			/* Receive Response */

			while (imaplocal->netstream) {
				if (!(p = net_getline(imaplocal->netstream))) {
					EM_DEBUG_EXCEPTION("net_getline failed...");

					err = EMAIL_ERROR_INVALID_RESPONSE;
					goto FINISH_OFF;
				}

				EM_DEBUG_LOG("[IMAP4 Response ] %s", p);

				if (!strncmp(p, tag, EM_SAFE_STRLEN(tag))) {
					if (!strncmp(p + EM_SAFE_STRLEN(tag) + 1, "OK", 2)) {
						/*Delete all local activities */
						EM_SAFE_FREE(p);
						break;
					} else {
						/* 'NO' or 'BAD' */
						err = EMAIL_ERROR_IMAP4_STORE_FAILURE;
						EM_SAFE_FREE(p);
						goto FINISH_OFF;
					}
				}

				EM_SAFE_FREE(p);
			}

			uid_range_node = uid_range_node->next;
		}

		emcore_free_uid_range_set(&uid_range_set);

		EM_SAFE_FREE(id_set);

		id_set_count = 0;
	}

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(id_set);

#ifdef __FEATURE_LOCAL_ACTIVITY__
	if (ret) {
		emstorage_activity_tbl_t new_activity;
		for (i = 0; i < num; i++) {
			memset(&new_activity, 0x00, sizeof(emstorage_activity_tbl_t));
			new_activity.activity_type = ACTIVITY_MODIFYSEENFLAG;
			new_activity.account_id    = account_id;
			new_activity.mail_id	   = mail_ids[i];

			if (!emcore_delete_activity(&new_activity, &err))
				EM_DEBUG_EXCEPTION("Local Activity ACTIVITY_MOVEMAIL [%d] ", err);
		}

	}
#endif

	if (NULL != mail) {
		if (false == emstorage_free_mail(&mail, 1, &err))
			EM_DEBUG_EXCEPTION("emstorage_free_mail failed - %d ", err);
	}

	emcore_free_comma_separated_strings(&string_list, &string_count);

	if (false == ret)
		emcore_free_uid_range_set(&uid_range_set);

	if (stream)
		stream = mail_close(stream);

	if (temp_account) {
		emcore_free_account(temp_account);
		EM_SAFE_FREE(temp_account);
	}

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}
#endif

static int emcore_mail_move_by_filter_rule(char *multi_user_name, int account_id, int mailbox_id, int mailbox_type, emstorage_rule_tbl_t *filter_info)
{
	EM_DEBUG_FUNC_BEGIN("account_id:[%d], mailbox_id:[%d], filter_info:[%p]", account_id, mailbox_id, filter_info);
	int err = EMAIL_ERROR_NONE;
	int mail_id_index = 0;
	int filter_mail_id_count = 0;
	int *filter_mail_id_list = NULL;
	int parameter_string_length = 0;
	char mail_id_string[10] = { 0x00, };
	char *parameter_string = NULL;
	emstorage_mailbox_tbl_t *src_mailbox_tbl = NULL;
	emstorage_mailbox_tbl_t *dst_mailbox_tbl = NULL;
	emstorage_mail_tbl_t *mail_tbl = NULL;

	if ((account_id < 0) && !filter_info) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

    if (filter_info->rule_id > 0 && !emstorage_update_tag_id(multi_user_name, filter_info->rule_id, 0, &err))
            EM_DEBUG_EXCEPTION("emstorage_update_tag_id failed : [%d]", err);

	if (!emstorage_filter_mails_by_rule(multi_user_name, account_id, mailbox_id, mailbox_type, false, filter_info, &filter_mail_id_list, &filter_mail_id_count, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_filter_mails_by_rule failed [%d]", err);
		goto FINISH_OFF;
	}

	if (filter_mail_id_count) {
		parameter_string_length = 10 /*mailbox_id length*/ + 7 + (10 * filter_mail_id_count);
		parameter_string = em_malloc(sizeof(char) * parameter_string_length);
		if (parameter_string == NULL) {
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			EM_DEBUG_EXCEPTION("em_mallocfailed for parameter_string");
			goto FINISH_OFF;
		}

		for (mail_id_index = 0; mail_id_index < filter_mail_id_count; mail_id_index++) {
			memset(mail_id_string, 0, 10);
			if ((filter_mail_id_count - 1) == mail_id_index)
				SNPRINTF(mail_id_string, 10, "%d", filter_mail_id_list[mail_id_index]);
			else
				SNPRINTF(mail_id_string, 10, "%d,", filter_mail_id_list[mail_id_index]);
			EM_SAFE_STRNCAT(parameter_string, mail_id_string, (sizeof(char) * parameter_string_length) - EM_SAFE_STRLEN(parameter_string) - 1);
		}

		EM_DEBUG_LOG("filtered_mail_id_count [%d]", filter_mail_id_count);
		EM_DEBUG_LOG("param string [%s]", parameter_string);

		if (filter_info->type == EMAIL_PRIORITY_SENDER) {
			if (!emcore_notify_storage_event(NOTI_RULE_APPLY, account_id, filter_info->rule_id, parameter_string, 0))
				EM_DEBUG_EXCEPTION("emcore_notify_storage_eventfailed [ NOTI_MAIL_MOVE ] >>>> ");
		} else {
			if (!emcore_notify_storage_event(NOTI_MAIL_MOVE, account_id, 0, parameter_string, 0))
				EM_DEBUG_EXCEPTION("emcore_notify_storage_eventfailed [ NOTI_MAIL_MOVE ] >>>> ");
		}
	}

	if (filter_info->action_type != EMAIL_FILTER_MOVE) {
		/* Move the mails on server */
		if ((err = emstorage_get_mailbox_by_id(multi_user_name, mailbox_id, &dst_mailbox_tbl)) != EMAIL_ERROR_NONE || !dst_mailbox_tbl) {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed : [%d]", err);
			goto FINISH_OFF;
		}

		for (mail_id_index = 0; mail_id_index < filter_mail_id_count; mail_id_index++) {
			if (!emstorage_get_mail_by_id(multi_user_name, filter_mail_id_list[mail_id_index], &mail_tbl, false, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed : [%d]", err);
				goto FINISH_OFF;
			}

			if ((err = emstorage_get_mailbox_by_id(multi_user_name, mail_tbl->mailbox_id, &src_mailbox_tbl)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed : [%d]", err);
				goto FINISH_OFF;
			}

			if (!emcore_move_mail_on_server(multi_user_name, account_id, src_mailbox_tbl->mailbox_id, filter_mail_id_list, filter_mail_id_count, dst_mailbox_tbl->mailbox_name, &err)) {
				EM_DEBUG_EXCEPTION("emcore_move_mail_on_server failed : [%d]", err);
				goto FINISH_OFF;
			}

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
			if (!emstorage_update_auto_download_activity(multi_user_name, mail_tbl->server_mail_id, g_new_server_uid, NULL, dst_mailbox_tbl->mailbox_id, &err))
				EM_DEBUG_EXCEPTION("emstorage_update_auto_download_activity failed : [%d]", err);
#endif

			emstorage_free_mailbox(&src_mailbox_tbl, 1, NULL); /*prevent 46750*/
		}
	}

FINISH_OFF:

	if (src_mailbox_tbl)
		emstorage_free_mailbox(&src_mailbox_tbl, 1, NULL);

	if (dst_mailbox_tbl)
		emstorage_free_mailbox(&dst_mailbox_tbl, 1, NULL);

	EM_SAFE_FREE(filter_mail_id_list);
	EM_SAFE_FREE(parameter_string);

	EM_DEBUG_FUNC_END("err : [%d]", err);
	return err;
}


INTERNAL_FUNC int emcore_mail_filter_by_rule(char *multi_user_name, email_rule_t *filter_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_info[%p], err_code[%p]", filter_info, err_code);

	int ret = false, err = EMAIL_ERROR_NONE;
	int account_index, account_count;
	emstorage_mailbox_tbl_t *spam_mailbox = NULL;
	emstorage_account_tbl_t *account_tbl_t_list = NULL, *account_tbl_t = NULL;

	if (!filter_info) {
		EM_DEBUG_EXCEPTION("filter_info[%p]", filter_info);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	switch (filter_info->faction) {
	case EMAIL_FILTER_MOVE:
		if (filter_info->type != EMAIL_PRIORITY_SENDER)
			break;

		if ((err = emcore_mail_move_by_filter_rule(multi_user_name, filter_info->account_id, 0, 0, (emstorage_rule_tbl_t *)filter_info)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_mail_move_by_filter_rule failed : [%d]", err);
			goto FINISH_OFF;
		}
		break;
	case EMAIL_FILTER_BLOCK:
		if (!emstorage_get_account_list(multi_user_name, &account_count, &account_tbl_t_list, false, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_account_list failed [%d]", err);
			goto FINISH_OFF;
		}

		for (account_index = 0; account_index < account_count; account_index++) {
			account_tbl_t = account_tbl_t_list + account_index;

			if (!emstorage_get_mailbox_by_mailbox_type(multi_user_name, account_tbl_t->account_id, EMAIL_MAILBOX_TYPE_SPAMBOX, &spam_mailbox, false, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type for account_id[%d] failed : [%d]", account_tbl_t->account_id, err);
				continue;
			}

			if ((err = emcore_mail_move_by_filter_rule(multi_user_name, account_tbl_t->account_id, spam_mailbox->mailbox_id, spam_mailbox->mailbox_type, (emstorage_rule_tbl_t *)filter_info)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_mail_move_by_filter_rule failed : [%d]", err);
				goto FINISH_OFF;
			}


		}
		break;
	case EMAIL_FILTER_DELETE:
	default:
		EM_DEBUG_LOG("filter_faction : [%d]", filter_info->faction);
		break;
	}

	emcore_display_unread_in_badge(multi_user_name);

	ret = true;

FINISH_OFF:

	if (account_tbl_t_list)
		emstorage_free_account(&account_tbl_t_list, account_count, NULL);

	if (spam_mailbox)
		emstorage_free_mailbox(&spam_mailbox, 1, &err);

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_add_rule(char *multi_user_name, email_rule_t *filter_info)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

	if (!emstorage_find_rule(multi_user_name, (emstorage_rule_tbl_t*)filter_info, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_find_rule failed [%d]", err);
		goto FINISH_OFF;
	} else {
		if (err != EMAIL_ERROR_FILTER_NOT_FOUND) {
			EM_DEBUG_LOG("filter already exist");
			err = EMAIL_ERROR_ALREADY_EXISTS;
			goto FINISH_OFF;
		}
	}

	switch (filter_info->faction) {
	case EMAIL_FILTER_MOVE:
		if (filter_info->account_id < 0) {
			EM_DEBUG_EXCEPTION("Invalid Param : target_mailbox_id[%d], account_id[%d]", filter_info->target_mailbox_id, filter_info->account_id);
			err = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		break;
	case EMAIL_FILTER_BLOCK:
		filter_info->account_id = ALL_ACCOUNT;
		break;
	case EMAIL_FILTER_DELETE:
	default:
		EM_DEBUG_LOG("filter_faction : [%d]", filter_info->faction);
		break;
	}

	if (!emstorage_add_rule(multi_user_name, (emstorage_rule_tbl_t*)filter_info, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_add_rule failed [%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("filter_id : [%d]", filter_info->filter_id);

	if (!emcore_notify_storage_event(NOTI_RULE_ADD, filter_info->account_id, filter_info->filter_id, NULL, 0))
		EM_DEBUG_EXCEPTION("emcore_notify_storage_eventfailed [NOTI_RULE_ADD]");

FINISH_OFF:

	EM_DEBUG_FUNC_END();
	return err;
}

INTERNAL_FUNC int emcore_update_rule(char *multi_user_name, int filter_id, email_rule_t *filter_info)
{
    EM_DEBUG_FUNC_BEGIN();

    int err = EMAIL_ERROR_NONE;
    int filter_mail_id_count = 0;
    int mail_id_index = 0;
    int *filter_mail_id_list = NULL;
    int parameter_string_length = 0;
    char mail_id_string[10] = { 0x00, };
    char *parameter_string = NULL;

    emstorage_rule_tbl_t *old_filter_info = NULL;

    if (filter_id <= 0 || !filter_info) {
        EM_DEBUG_EXCEPTION("filter_id [%d], filter_info[%p]", filter_id, filter_info);
        err = EMAIL_ERROR_INVALID_PARAM;
        return err;
    }

    if (!emstorage_get_rule_by_id(multi_user_name, filter_id, &old_filter_info, false, &err)) {
            EM_DEBUG_EXCEPTION("emstorage_get_rule_by_id failed : [%d]", err);
            goto FINISH_OFF;
    }

    if (!emstorage_filter_mails_by_rule(multi_user_name, old_filter_info->account_id, old_filter_info->target_mailbox_id, false, true, old_filter_info, &filter_mail_id_list, &filter_mail_id_count, &err)) {
            EM_DEBUG_EXCEPTION("emstorage_filter_mails_by_rule failed : [%d]", err);
            goto FINISH_OFF;
    }

    if (filter_mail_id_count) {
            parameter_string_length = 10 /*mailbox_id length*/ + 7 + (10 * filter_mail_id_count);
            parameter_string = em_malloc(sizeof(char) * parameter_string_length);
            if (parameter_string == NULL) {
                    err = EMAIL_ERROR_OUT_OF_MEMORY;
                    EM_DEBUG_EXCEPTION("em_mallocfailed for parameter_string");
                    goto FINISH_OFF;
            }

            for (mail_id_index = 0; mail_id_index < filter_mail_id_count; mail_id_index++) {
                    memset(mail_id_string, 0, 10);
                    if ((filter_mail_id_count - 1) == mail_id_index)
                            SNPRINTF(mail_id_string, 10, "%d", filter_mail_id_list[mail_id_index]);
                    else
                            SNPRINTF(mail_id_string, 10, "%d,", filter_mail_id_list[mail_id_index]);
                    EM_SAFE_STRNCAT(parameter_string, mail_id_string, (sizeof(char) * parameter_string_length) - EM_SAFE_STRLEN(parameter_string) - 1);
            }

            EM_DEBUG_LOG("filtered_mail_id_count [%d]", filter_mail_id_count);
            EM_DEBUG_LOG("param string [%s]", parameter_string);
    }

    if (!emstorage_change_rule(multi_user_name, filter_id, (emstorage_rule_tbl_t *)filter_info, true, &err)) {
        EM_DEBUG_EXCEPTION("emstorage_change_rule failed [%d]", err);
        goto FINISH_OFF;
    }

    if (!emcore_notify_storage_event(NOTI_RULE_UPDATE, filter_info->account_id, filter_info->filter_id, parameter_string, 0))
        EM_DEBUG_EXCEPTION("emcore_notify_storage_eventfailed [NOTI_RULE_UPDATE]");

FINISH_OFF:

    if (old_filter_info)
        emstorage_free_rule(&old_filter_info, 1, NULL);

    EM_SAFE_FREE(filter_mail_id_list);
    EM_SAFE_FREE(parameter_string);

    EM_DEBUG_FUNC_END();
    return err;
}

INTERNAL_FUNC int emcore_delete_rule(char *multi_user_name, int filter_id)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int filter_mail_id_count = 0;
	int mail_id_index = 0;
	int *filter_mail_id_list = NULL;
	int parameter_string_length = 0;
	char mail_id_string[10] = { 0x00, };
	char *parameter_string = NULL;

	emstorage_rule_tbl_t *filter_info = NULL;

	if (filter_id <= 0) {
		EM_DEBUG_EXCEPTION(" fliter_id[%d]", filter_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_get_rule_by_id(multi_user_name, filter_id, &filter_info, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_rule_by_id failed : [%d]", err);
		goto FINISH_OFF;
	}

	if (!emstorage_filter_mails_by_rule(multi_user_name, filter_info->account_id, filter_info->target_mailbox_id, false, true, filter_info, &filter_mail_id_list, &filter_mail_id_count, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_filter_mails_by_rule failed : [%d]", err);
		goto FINISH_OFF;
	}

	if (filter_mail_id_count) {
		parameter_string_length = 10 /*mailbox_id length*/ + 7 + (10 * filter_mail_id_count);
		parameter_string = em_malloc(sizeof(char) * parameter_string_length);
		if (parameter_string == NULL) {
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			EM_DEBUG_EXCEPTION("em_mallocfailed for parameter_string");
			goto FINISH_OFF;
		}

		for (mail_id_index = 0; mail_id_index < filter_mail_id_count; mail_id_index++) {
			memset(mail_id_string, 0, 10);
			if ((filter_mail_id_count - 1) == mail_id_index)
				SNPRINTF(mail_id_string, 10, "%d", filter_mail_id_list[mail_id_index]);
			else
				SNPRINTF(mail_id_string, 10, "%d,", filter_mail_id_list[mail_id_index]);
			EM_SAFE_STRNCAT(parameter_string, mail_id_string,( sizeof(char) * parameter_string_length) - EM_SAFE_STRLEN(parameter_string) - 1 );
		}

		EM_DEBUG_LOG("filtered_mail_id_count [%d]", filter_mail_id_count);
		EM_DEBUG_LOG("param string [%s]", parameter_string);
	}

	if (!emstorage_delete_rule(multi_user_name, filter_id, true, &err)) {
		EM_DEBUG_EXCEPTION(" emstorage_delete_rule failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_notify_storage_event(NOTI_RULE_DELETE, 0, filter_id, parameter_string, 0))
		EM_DEBUG_EXCEPTION("emcore_notify_storage_eventfailed [NOTI_RULE_DELETE]");

FINISH_OFF:

	if (filter_info)
		emstorage_free_rule(&filter_info, 1, NULL);

	EM_SAFE_FREE(filter_mail_id_list);
	EM_SAFE_FREE(parameter_string);

	EM_DEBUG_FUNC_END();
	return err;
}

INTERNAL_FUNC void emcore_free_content_info(struct _m_content_info *cnt_info)
{
	EM_DEBUG_FUNC_BEGIN();
	struct attachment_info *p;

	if (!cnt_info) return ;
	EM_SAFE_FREE(cnt_info->text.plain);
	EM_SAFE_FREE(cnt_info->text.plain_charset);
	EM_SAFE_FREE(cnt_info->text.html);
	EM_SAFE_FREE(cnt_info->text.html_charset);
	EM_SAFE_FREE(cnt_info->text.mime_entity);
	EM_SAFE_FREE(cnt_info->sections);

	while (cnt_info->file) {
		p = cnt_info->file->next;
		EM_SAFE_FREE(cnt_info->file->name);
		EM_SAFE_FREE(cnt_info->file->save);
		EM_SAFE_FREE(cnt_info->file->attachment_mime_type);
		EM_SAFE_FREE(cnt_info->file->content_id);
		EM_SAFE_FREE(cnt_info->file);
		cnt_info->file = p;
	}

	while (cnt_info->inline_file) {
		p = cnt_info->inline_file->next;
		EM_SAFE_FREE(cnt_info->inline_file->name);
		EM_SAFE_FREE(cnt_info->inline_file->save);
		EM_SAFE_FREE(cnt_info->inline_file->attachment_mime_type);
		EM_SAFE_FREE(cnt_info->inline_file->content_id);
		EM_SAFE_FREE(cnt_info->inline_file);
		cnt_info->inline_file = p;
	}

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void emcore_free_attachment_info(struct attachment_info *attchment)
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

static char *get_charset_in_search_filter_string(email_search_filter_t *search_filter, int search_filter_count)
{
	EM_DEBUG_FUNC_BEGIN("search_filter : [%p], search_filter_count : [%d]", search_filter, search_filter_count);

	int i = 0;
	char *charset = NULL;

	if (search_filter == NULL || search_filter_count <= 0) {
		EM_DEBUG_EXCEPTION("Invalid paramter");
		return NULL;
	}

	for (i = 0; i < search_filter_count; i++) {
		EM_DEBUG_LOG("current : [%d]", i);
		EM_DEBUG_LOG("search_filter_type : [%d]", search_filter[i].search_filter_type);

		switch (search_filter[i].search_filter_type) {
		case EMAIL_SEARCH_FILTER_TYPE_CHARSET:
			EM_DEBUG_LOG("string_type_key_value [%s]",
							search_filter[i].search_filter_key_value.string_type_key_value);
			charset = g_strdup(search_filter[i].search_filter_key_value.string_type_key_value);
			break;

		default:
			break;
		}
	}

	return charset;
}

static int create_searchpgm_from_filter_string(email_search_filter_t *search_filter,
												int start_index,
												int end_index,
												int *current_index,
												SEARCHPGM **output_searchpgm)
{
	EM_DEBUG_FUNC_BEGIN("search_filter:[%p], start_index[%d], end_index[%d]",
						search_filter, start_index, end_index);

	int err = EMAIL_ERROR_NONE;

	if (search_filter == NULL || output_searchpgm == NULL) {
		EM_DEBUG_EXCEPTION("Invalid paramter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	EM_DEBUG_LOG("start index : [%d]", start_index);

	int i = 0;
	int p_current_index = 0;
	char *p = NULL;
	char *temp_string = NULL;

	SEARCHPGM *search_program = NULL;
	SEARCHOR *temp_or_program = NULL;

	search_program = mail_newsearchpgm();
	if (search_program == NULL) {
		EM_DEBUG_EXCEPTION("mail_newsearchpgm failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = start_index; i < end_index; i++) {
		p_current_index = i;
		EM_DEBUG_LOG("search_filter_type : [%d]", search_filter[i].search_filter_type);

		switch (search_filter[i].search_filter_type) {
		case EMAIL_SEARCH_FILTER_TYPE_ALL:
			EM_DEBUG_LOG("integer_type_key_value [%d]",
							search_filter[i].search_filter_key_value.integer_type_key_value);

			if (search_filter[i].search_filter_key_value.integer_type_key_value == 0) {
				temp_or_program = mail_newsearchor();
				if (temp_or_program == NULL) {
					EM_DEBUG_EXCEPTION("em_mallocfailed");
					err = EMAIL_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}

				p_current_index++;
				err = create_searchpgm_from_filter_string(search_filter,
															p_current_index,
															p_current_index + 1,
															&p_current_index,
															&temp_or_program->first);
				if (err != EMAIL_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("create_searchpgm_from_filter_string failed : [%d]", err);
					goto FINISH_OFF;
				}

				p_current_index++;
				err = create_searchpgm_from_filter_string(search_filter,
															p_current_index,
															p_current_index + 1,
															&p_current_index,
															&temp_or_program->second);
				if (err != EMAIL_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("create_searchpgm_from_filter_string failed : [%d]", err);
					goto FINISH_OFF;
				}

				i = p_current_index;
			}

			if (search_program->or) {
				SEARCHOR *temp_or = search_program->or;
				while (temp_or->next)
					temp_or = temp_or->next;

				temp_or->next = temp_or_program;
			} else {
				search_program->or = temp_or_program;
			}

			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED:
			EM_DEBUG_LOG("integer_type_key_value [%d]",
							search_filter[i].search_filter_key_value.integer_type_key_value);

			if (search_filter[i].search_filter_key_value.integer_type_key_value)
				search_program->answered = 1;
			else
				search_program->unanswered = 1;

			break;

		case EMAIL_SEARCH_FILTER_TYPE_BCC:
		    EM_DEBUG_LOG("string_type_key_value [%s]",
							search_filter[i].search_filter_key_value.string_type_key_value);

			temp_string = g_strdup_printf("bcc %s",
							search_filter[i].search_filter_key_value.string_type_key_value);
			p = temp_string;
			if (!mail_criteria_header_string(&(search_program->header), &p)) {
				EM_DEBUG_EXCEPTION("mail_criteria_header_string failed");
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}

			EM_SAFE_FREE(temp_string);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE:
			EM_DEBUG_LOG("time_type_key_value [%d]", search_filter[i].search_filter_key_value.time_type_key_value);

			err = emcore_make_date_string_for_search(search_filter[i].search_filter_key_value.time_type_key_value,
														&temp_string);
			if (err != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_make_date_string_for_search failed : [%d]", err);
				goto FINISH_OFF;
			}

			p = temp_string;
			if (!mail_criteria_date(&(search_program->before), &p)) {
				EM_DEBUG_EXCEPTION("mail_criteria_date failed");
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}

			EM_SAFE_FREE(temp_string);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_BODY:
		    EM_DEBUG_LOG("string_type_key_value [%s]",
							search_filter[i].search_filter_key_value.string_type_key_value);

			p = search_filter[i].search_filter_key_value.string_type_key_value;
			if (!mail_criteria_string(&(search_program->body), &p)) {
				EM_DEBUG_EXCEPTION("mail_criteria_string failed");
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}

			break;

		case EMAIL_SEARCH_FILTER_TYPE_CC:
		    EM_DEBUG_LOG("string_type_key_value [%s]",
							search_filter[i].search_filter_key_value.string_type_key_value);

 			temp_string = g_strdup_printf("cc %s",
							search_filter[i].search_filter_key_value.string_type_key_value);
			p = temp_string;
			if (!mail_criteria_header_string(&(search_program->header), &p)) {
				EM_DEBUG_EXCEPTION("mail_criteria_header_string failed");
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}

			EM_SAFE_FREE(temp_string);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED:
			EM_DEBUG_LOG("integer_type_key_value [%d]",
							search_filter[i].search_filter_key_value.integer_type_key_value);

			if (search_filter[i].search_filter_key_value.integer_type_key_value)
				search_program->deleted = 1;
			else
				search_program->undeleted = 1;

			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED:
			EM_DEBUG_LOG("integer_type_key_value [%d]",
							search_filter[i].search_filter_key_value.integer_type_key_value);

			if (search_filter[i].search_filter_key_value.integer_type_key_value)
				search_program->flagged = 1;
			else
				search_program->unflagged = 1;

			break;

		case EMAIL_SEARCH_FILTER_TYPE_FROM:
		    EM_DEBUG_LOG("string_type_key_value [%s]",
							search_filter[i].search_filter_key_value.string_type_key_value);

 			temp_string = g_strdup_printf("from %s",
							search_filter[i].search_filter_key_value.string_type_key_value);
			p = temp_string;
			if (!mail_criteria_header_string(&(search_program->header), &p)) {
				EM_DEBUG_EXCEPTION("mail_criteria_header_string failed");
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}

			EM_SAFE_FREE(temp_string);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_KEYWORD:
		    EM_DEBUG_LOG("string_type_key_value [%s]",
							search_filter[i].search_filter_key_value.string_type_key_value);

			p = search_filter[i].search_filter_key_value.string_type_key_value;
			if (!mail_criteria_string(&(search_program->keyword), &p)) {
				EM_DEBUG_EXCEPTION("mail_criteria_string failed");
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}

			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_NEW:
			EM_DEBUG_LOG("integer_type_key_value [%d]",
							search_filter[i].search_filter_key_value.integer_type_key_value);

			if (search_filter[i].search_filter_key_value.integer_type_key_value) {
				search_program->unseen = 1;
				search_program->recent = 1;
			}

			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_OLD:
			EM_DEBUG_LOG("integer_type_key_value [%d]",
							search_filter[i].search_filter_key_value.integer_type_key_value);

			if (search_filter[i].search_filter_key_value.integer_type_key_value)
				search_program->old = 1;

			break;

		case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON:
			EM_DEBUG_LOG("time_type_key_value [%d]",
							search_filter[i].search_filter_key_value.time_type_key_value);

			err = emcore_make_date_string_for_search(search_filter[i].search_filter_key_value.time_type_key_value,
														&temp_string);
			if (err != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_make_date_string_for_search failed : [%d]", err);
				goto FINISH_OFF;
			}

			p = temp_string;
			if (!mail_criteria_date(&(search_program->on), &p)) {
				EM_DEBUG_EXCEPTION("mail_criteria_date failed");
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}

			EM_SAFE_FREE(temp_string);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT:
			EM_DEBUG_LOG("integer_type_key_value [%d]",
							search_filter[i].search_filter_key_value.integer_type_key_value);

			if (search_filter[i].search_filter_key_value.integer_type_key_value)
				search_program->recent = 1;

			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN:
			EM_DEBUG_LOG("integer_type_key_value [%d]",
							search_filter[i].search_filter_key_value.integer_type_key_value);

			if (search_filter[i].search_filter_key_value.integer_type_key_value)
				search_program->seen = 1;
			else
				search_program->unseen = 1;

			break;

		case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE:
			EM_DEBUG_LOG("time_type_key_value [%d]",
							search_filter[i].search_filter_key_value.time_type_key_value);

			err = emcore_make_date_string_for_search(search_filter[i].search_filter_key_value.time_type_key_value,
														&temp_string);
			if (err != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_make_date_string_for_search failed : [%d]", err);
				goto FINISH_OFF;
			}

			p = temp_string;
			if (!mail_criteria_date(&(search_program->since), &p)) {
				EM_DEBUG_EXCEPTION("mail_criteria_date failed");
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}

			EM_SAFE_FREE(temp_string);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_SUBJECT:
			EM_DEBUG_LOG("string_type_key_value [%s]",
							search_filter[i].search_filter_key_value.string_type_key_value);

			temp_string = g_strdup_printf("subject %s",
							search_filter[i].search_filter_key_value.string_type_key_value);
			p = temp_string;
			if (!mail_criteria_header_string(&(search_program->header), &p)) {
				EM_DEBUG_EXCEPTION("mail_criteria_header_string failed");
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}

			EM_SAFE_FREE(temp_string);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_TEXT:
		    EM_DEBUG_LOG("string_type_key_value [%s]",
							search_filter[i].search_filter_key_value.string_type_key_value);

			p = search_filter[i].search_filter_key_value.string_type_key_value;
			if (!mail_criteria_string(&(search_program->text), &p)) {
				EM_DEBUG_EXCEPTION("mail_criteria_string failed");
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}

			break;

		case EMAIL_SEARCH_FILTER_TYPE_TO:
		    EM_DEBUG_LOG("string_type_key_value [%s]",
							search_filter[i].search_filter_key_value.string_type_key_value);

			temp_string = g_strdup_printf("to %s",
							search_filter[i].search_filter_key_value.string_type_key_value);
			p = temp_string;
			if (!mail_criteria_header_string(&(search_program->header), &p)) {
				EM_DEBUG_EXCEPTION("mail_criteria_header_string failed");
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}

			EM_SAFE_FREE(temp_string);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_HEADER_PRIORITY:
			EM_DEBUG_LOG("integer_type_key_value [%d]",
							search_filter[i].search_filter_key_value.integer_type_key_value);

			temp_string = g_strdup_printf("x-priority %d",
											search_filter[i].search_filter_key_value.integer_type_key_value);
			p = temp_string;
			if (!mail_criteria_header_string(&(search_program->header), &p)) {
				EM_DEBUG_EXCEPTION("mail_criteria_header_string failed");
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}

			EM_SAFE_FREE(temp_string);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER:
			EM_DEBUG_LOG("integer_type_key_value [%d]",
							search_filter[i].search_filter_key_value.integer_type_key_value);

			search_program->larger = search_filter[i].search_filter_key_value.integer_type_key_value;

			break;

		case EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER:
			EM_DEBUG_LOG("integer_type_key_value [%d]",
							search_filter[i].search_filter_key_value.integer_type_key_value);

			search_program->smaller = search_filter[i].search_filter_key_value.integer_type_key_value;

			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT:
			EM_DEBUG_LOG("integer_type_key_value [%d]",
							search_filter[i].search_filter_key_value.integer_type_key_value);

			if (search_filter[i].search_filter_key_value.integer_type_key_value)
				search_program->draft = 1;
			else
				search_program->undraft = 1;

			break;

		case EMAIL_SEARCH_FILTER_TYPE_ATTACHMENT_NAME:
		    EM_DEBUG_LOG("string_type_key_value [%s]",
							search_filter[i].search_filter_key_value.string_type_key_value);

#ifdef __FEATURE_GMIME_SEARCH_EXTENTION__
			p = search_filter[i].search_filter_key_value.string_type_key_value;
			if (!mail_criteria_string(&(search_program->attachment_name), &p)) {
				EM_DEBUG_EXCEPTION("mail_criteria_string failed");
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}
#endif /* __FEATURE_GMIME_SEARCH_EXTENTION__ */
			break;

		case EMAIL_SEARCH_FILTER_TYPE_CHARSET:
			EM_DEBUG_LOG("string_type_key_value [%s]",
							search_filter[i].search_filter_key_value.string_type_key_value);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_USER_DEFINED:
			EM_DEBUG_LOG("string_type_key_value [%s]",
							search_filter[i].search_filter_key_value.string_type_key_value);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO:
		case EMAIL_SEARCH_FILTER_TYPE_UID:
			EM_DEBUG_LOG("integer_type_key_value [%d]",
							search_filter[i].search_filter_key_value.integer_type_key_value);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID:
			EM_DEBUG_LOG("string_type_key_value [%s]",
							search_filter[i].search_filter_key_value.string_type_key_value);

			temp_string = g_strdup_printf("message-id %s",
							search_filter[i].search_filter_key_value.string_type_key_value);
			p = temp_string;
			if (!mail_criteria_header_string(&(search_program->header), &p)) {
				EM_DEBUG_EXCEPTION("mail_criteria_header_string failed");
				err = EMAIL_ERROR_INVALID_DATA;
				goto FINISH_OFF;
			}

			EM_SAFE_FREE(temp_string);
			break;

		default:
			EM_DEBUG_EXCEPTION("Invalid list_filter_item_type [%d]", search_filter);
			err = EMAIL_ERROR_INVALID_PARAM;
			break;
		}
	}

	if (current_index) {
		EM_DEBUG_LOG("swap current_index[%d], p_current_index[%d]", *current_index, p_current_index);
		*current_index = p_current_index;
	}

FINISH_OFF:

	EM_SAFE_FREE(temp_string);

	if (err != EMAIL_ERROR_NONE) {
		if (search_program)
			mail_free_searchpgm(&search_program);
	} else {
		*output_searchpgm = search_program;
	}

	EM_DEBUG_FUNC_END();
	return err;
}

static int get_search_filter_string(email_search_filter_t *search_filter, int search_filter_count, char **output_filter_string)
{
	EM_DEBUG_FUNC_BEGIN("search_filter : [%p], search_filter_count : [%d]", search_filter, search_filter_count);

	int i = 0, j = 0;
	int err = EMAIL_ERROR_NONE;
	char *time_string = NULL;
	char filter_string[MAX_PREVIEW_TEXT_LENGTH] = { 0, };
	char temp_criteria[MAX_PREVIEW_TEXT_LENGTH] = { 0, };

	if (search_filter == NULL || search_filter_count < 0 || output_filter_string == NULL) {
		EM_DEBUG_EXCEPTION("Invalid paramter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	for (i = 0; i < search_filter_count; i++) {
		EM_DEBUG_LOG("search_filter_type [%d]", search_filter[i].search_filter_type);
		memset(temp_criteria, 0x00, MAX_PREVIEW_TEXT_LENGTH);

		switch (search_filter[i].search_filter_type) {
		case EMAIL_SEARCH_FILTER_TYPE_ALL:
			EM_DEBUG_LOG("integer_type_key_value [%d]",
							search_filter[i].search_filter_key_value.integer_type_key_value);
			if (search_filter[i].search_filter_key_value.integer_type_key_value == 0) {
				for (j = i + 1; j < search_filter_count; j++) {
					if (search_filter[j + 1].search_filter_type == EMAIL_SEARCH_FILTER_TYPE_ALL &&
						search_filter[j + 1].search_filter_key_value.integer_type_key_value) {
						break;
					}

					SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "OR ");
					strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
				}
			}
			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED:
			EM_DEBUG_LOG("integer_type_key_value [%d]", search_filter[i].search_filter_key_value.integer_type_key_value);
			if (search_filter[i].search_filter_key_value.integer_type_key_value)
				SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "ANSWERED ");
			else
				SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "UNANSWERED ");
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_BCC:
		    EM_DEBUG_LOG_SEC("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "BCC \"%s\" ",
						search_filter[i].search_filter_key_value.string_type_key_value);
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE:
			EM_DEBUG_LOG("time_type_key_value [%d]", search_filter[i].search_filter_key_value.time_type_key_value);
			emcore_make_date_string_for_search(search_filter[i].search_filter_key_value.time_type_key_value,
												&time_string);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "BEFORE %s ", time_string);
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			EM_SAFE_FREE(time_string);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_BODY:
		    EM_DEBUG_LOG_SEC("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "BODY \"%s\" ",
						search_filter[i].search_filter_key_value.string_type_key_value);
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_CC:
		    EM_DEBUG_LOG_SEC("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "CC \"%s\" ",
						search_filter[i].search_filter_key_value.string_type_key_value);
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED:
			EM_DEBUG_LOG("integer_type_key_value [%d]", search_filter[i].search_filter_key_value.integer_type_key_value);
			if (search_filter[i].search_filter_key_value.integer_type_key_value)
				SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "DELETED ");
			else
				SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "UNDELETED ");

			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED:
			EM_DEBUG_LOG("integer_type_key_value [%d]", search_filter[i].search_filter_key_value.integer_type_key_value);
			if (search_filter[i].search_filter_key_value.integer_type_key_value)
				SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "FLAGGED ");
			else
				SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "UNFLAGGED ");

			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_FROM:
		    EM_DEBUG_LOG_SEC("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "FROM \"%s\" ",
						search_filter[i].search_filter_key_value.string_type_key_value);
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_KEYWORD:
		    EM_DEBUG_LOG_SEC("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "KEYWORD \"%s\" ",
						search_filter[i].search_filter_key_value.string_type_key_value);

			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_NEW:
			EM_DEBUG_LOG("integer_type_key_value [%d]", search_filter[i].search_filter_key_value.integer_type_key_value);
			if (search_filter[i].search_filter_key_value.integer_type_key_value)
				SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "NEW ");

			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_OLD:
			EM_DEBUG_LOG("integer_type_key_value [%d]", search_filter[i].search_filter_key_value.integer_type_key_value);
			if (search_filter[i].search_filter_key_value.integer_type_key_value)
				SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "OLD ");

			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON:
			EM_DEBUG_LOG("time_type_key_value [%d]", search_filter[i].search_filter_key_value.time_type_key_value);
			emcore_make_date_string_for_search(search_filter[i].search_filter_key_value.time_type_key_value,
												&time_string);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "ON %s ", time_string);
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			EM_SAFE_FREE(time_string);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT:
			EM_DEBUG_LOG("integer_type_key_value [%d]", search_filter[i].search_filter_key_value.integer_type_key_value);
			if (search_filter[i].search_filter_key_value.integer_type_key_value)
				SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "RECENT ");
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN:
			EM_DEBUG_LOG("integer_type_key_value [%d]", search_filter[i].search_filter_key_value.integer_type_key_value);
			if (search_filter[i].search_filter_key_value.integer_type_key_value)
				SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "SEEN ");
			else
				SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "UNSEEN ");

			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE:
			EM_DEBUG_LOG("time_type_key_value [%d]", search_filter[i].search_filter_key_value.time_type_key_value);
			emcore_make_date_string_for_search(search_filter[i].search_filter_key_value.time_type_key_value,
												&time_string);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "SINCE %s ", time_string);
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			EM_SAFE_FREE(time_string);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_SUBJECT:
			EM_DEBUG_LOG_SEC("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "SUBJECT \"%s\" ",
						search_filter[i].search_filter_key_value.string_type_key_value);
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_TEXT:
		    EM_DEBUG_LOG_SEC("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "TEXT \"%s\" ",
						search_filter[i].search_filter_key_value.string_type_key_value);
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_TO:
		    EM_DEBUG_LOG_SEC("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "TO \"%s\" ",
						search_filter[i].search_filter_key_value.string_type_key_value);
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_HEADER_PRIORITY:
			EM_DEBUG_LOG("integer_type_key_value [%d]", search_filter[i].search_filter_key_value.integer_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "HEADER x-priority %d ", search_filter[i].search_filter_key_value.integer_type_key_value);
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER:
			EM_DEBUG_LOG("integer_type_key_value [%d]", search_filter[i].search_filter_key_value.integer_type_key_value);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER:
			EM_DEBUG_LOG("integer_type_key_value [%d]", search_filter[i].search_filter_key_value.integer_type_key_value);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT:
			EM_DEBUG_LOG("integer_type_key_value [%d]", search_filter[i].search_filter_key_value.integer_type_key_value);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_ATTACHMENT_NAME:
		    EM_DEBUG_LOG_SEC("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "X-GM-RAW \"has:attachment filename:%s\" ",
						search_filter[i].search_filter_key_value.string_type_key_value);
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_CHARSET:
			EM_DEBUG_LOG("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "charset %s ", search_filter[i].search_filter_key_value.string_type_key_value);
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;


		case EMAIL_SEARCH_FILTER_TYPE_USER_DEFINED:
			EM_DEBUG_LOG("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "%s", search_filter[i].search_filter_key_value.string_type_key_value);
			strncat(filter_string, temp_criteria, STRING_LENGTH_FOR_DISPLAY);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO:
		case EMAIL_SEARCH_FILTER_TYPE_UID:
			EM_DEBUG_LOG("integer_type_key_value [%d]", search_filter[i].search_filter_key_value.integer_type_key_value);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID:
			EM_DEBUG_LOG("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			break;

		default:
			EM_DEBUG_EXCEPTION("Invalid list_filter_item_type [%d]", search_filter);
			err = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
	}

	em_trim_right(filter_string);

	EM_DEBUG_LOG_SEC("filter_string[%s]", filter_string);

FINISH_OFF:

	if (output_filter_string)
		*output_filter_string = EM_SAFE_STRDUP(filter_string);

	EM_DEBUG_FUNC_END();
	return err;
}

int emcore_search_mail_and_uids(MAILSTREAM *stream, email_search_filter_t *input_search_filter,
								int input_search_filter_count, emcore_uid_list** output_uid_list,
								int *output_uid_count)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p] input_search_filter[%p] input_search_filter_count[%d] "
						"output_uid_list[%p] output_uid_count[%p]",
						stream, input_search_filter_count, output_uid_list, output_uid_count);

	int err = EMAIL_ERROR_NONE;

	IMAPLOCAL *imaplocal = NULL;
	char tag[16], command[MAX_PREVIEW_TEXT_LENGTH];
	char *response = NULL;
	emcore_uid_list *uid_elem = NULL;
	char delims[] = " ";
	char *result = NULL;
	int  uid_count = 0;
	char          *search_filter_string = NULL;
	char          *uid_range_string = NULL;
	char *ptr = NULL;
	emcore_uid_list *uid_list_for_listing = NULL;

	if (stream  == NULL || output_uid_list == NULL || input_search_filter == NULL || output_uid_count == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(imaplocal = stream->local) || !imaplocal->netstream) {
		EM_DEBUG_EXCEPTION(" invalid IMAP4 stream detected...");
		err = EMAIL_ERROR_INVALID_PARAM;		/* EMAIL_ERROR_UNKNOWN */
		goto FINISH_OFF;
	}

	memset(tag, 0x00, sizeof(tag));
	memset(command, 0x00, sizeof(command));

	if ((err = get_search_filter_string(input_search_filter,
										input_search_filter_count,
										&search_filter_string)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("get_search_filter_string failed [%d]", err);
		goto FINISH_OFF;
	}

	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
	SNPRINTF(command, MAX_PREVIEW_TEXT_LENGTH, "%s UID SEARCH %s\015\012", tag, search_filter_string);
	EM_DEBUG_LOG_SEC("COMMAND [%s] ", command);

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG_SEC(" [IMAP4] >>> [%s]", command);
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
				EM_SAFE_FREE(response);
				break;
			} else {	/*  'NO' or 'BAD' */
				err = EMAIL_ERROR_IMAP4_FETCH_UID_FAILURE;		/* EMAIL_ERROR_INVALID_RESPONSE; */
				goto FINISH_OFF;
			}
		}

		if ((p = strstr(response, " SEARCH "))) {
			*p = '\0'; p  += strlen(" SEARCH ");

			result = strtok_r(p, delims, &ptr);

			while (result  != NULL) {
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
				result = strtok_r(NULL, delims, &ptr);
				uid_count++;
			}

			EM_SAFE_FREE(response);
			continue;
		} else if ((p = strstr(response, " SEARCH"))) {
			/* there is no mail */
			continue;
		} else {
			err = EMAIL_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG("uid_count [%d] ", uid_count);
	if (uid_count > 0) {
		if ((err = emcore_make_uid_range_string(uid_list_for_listing,
												uid_count,
												&uid_range_string)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_make_uid_range_string failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("uid_range_string [%s] ", uid_range_string);

		/* Get uids with flags */
		if (!imap4_mailbox_get_uids(stream, uid_range_string, output_uid_list, &err)) {
			EM_DEBUG_EXCEPTION("imap4_mailbox_get_uids failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:

	if (output_uid_count)
		*output_uid_count = uid_count;

	if (uid_list_for_listing)
		emcore_free_uids(uid_list_for_listing, NULL);

	EM_SAFE_FREE(response);
	EM_SAFE_FREE(uid_range_string);
	EM_SAFE_FREE(search_filter_string);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_search_on_server_ex(char *multi_user_name,
												int account_id,
												int mailbox_id,
												email_search_filter_t *input_search_filter,
												int input_search_filter_count,
												int cancellable,
												int event_handle)
{
	EM_DEBUG_FUNC_BEGIN("account_id : [%d], mailbox_id : [%d]", account_id, mailbox_id);
	int err = EMAIL_ERROR_NONE;

	if (account_id <= 0 || mailbox_id <= 0 || input_search_filter_count <= 0 || input_search_filter == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	int uid_count = 0;
	int mail_id = 0;
	int thread_id = 0;
	long message_count = 0, i = 0;
	char *charset = NULL;
	char *uid_range = NULL;
	char *mailbox_id_string = NULL;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	emcore_uid_list *uid_elem = NULL;
	emcore_uid_list *uid_list = NULL;
	emstorage_mailbox_tbl_t *search_mailbox = NULL;
	emstorage_mail_tbl_t *new_mail_tbl_data = NULL;

	ENVELOPE *env = NULL;
	MAILSTREAM *stream = NULL;
	SEARCHPGM *search_program = NULL;
	MESSAGECACHE *search_message_cache = NULL;

	/* Get charset */
	charset = get_charset_in_search_filter_string(input_search_filter, input_search_filter_count);

	/* Get the search program from search_filter */
	err = create_searchpgm_from_filter_string(input_search_filter,
												0,
												input_search_filter_count,
												NULL,
												&search_program);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("create_searchpgm_from_filter_string failed : [%d]", err);
		goto FINISH_OFF;
	}



	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	/* Create new stream.
	   This stream is certainly freed. */
	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											account_id,
											mailbox_id,
											false,
											(void **)&stream,
											&err)) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed");
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("nmsgs : [%ld]", stream->nmsgs);
	message_count = stream->nmsgs;

	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	if (!mail_search_full(stream, charset, search_program, 0)) {
		EM_DEBUG_EXCEPTION("mail_search_full failed");
		goto FINISH_OFF;
	}

	for (i = 1 ; i <= message_count; i++) {
		search_message_cache = mail_elt(stream, i);
		if (!search_message_cache->searched)
			continue;

		uid_elem = em_malloc(sizeof(emcore_uid_list));
		if (uid_elem == NULL) {
			EM_DEBUG_EXCEPTION("em_mallocfailed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		uid_count++;
		uid_elem->uid = g_strdup_printf("%lu", mail_uid(stream, i));
		uid_elem->msgno = i;

		if (uid_list != NULL)
			uid_elem->next = uid_list;

		uid_list = uid_elem;
	}

	uid_elem = NULL;

	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	EM_DEBUG_LOG("uid_count : [%d]", uid_count);
	if (uid_count > 0) {
		err = emcore_make_uid_range_string(uid_list, uid_count,	&uid_range);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_make_uid_range_string failed : [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("Calling ... mail_fetch_fast. uid_range [%s]", uid_range);
		mail_fetch_fast(stream, uid_range, FT_UID | FT_PEEK | FT_NEEDENV);
		EM_SAFE_FREE(uid_range);
	}

	/* Create the mailbox information for adding */
	search_mailbox = em_malloc(sizeof(emstorage_mailbox_tbl_t));
	if (search_mailbox == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	search_mailbox->account_id = account_id;
	search_mailbox->mailbox_id = mailbox_id;
	search_mailbox->mailbox_name = EM_SAFE_STRDUP(EMAIL_SEARCH_RESULT_MAILBOX_NAME);
	search_mailbox->mailbox_type = EMAIL_MAILBOX_TYPE_SEARCH_RESULT;

	mailbox_id_string = g_strdup_printf("%d", mailbox_id);
	if (mailbox_id_string == NULL) {
		EM_DEBUG_EXCEPTION("g_strdup_printf failed : [%d][%s]", errno, EM_STRERROR(errno_buf));
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	/* Add the mail */
	uid_elem = uid_list;
	while (uid_elem) {
		search_message_cache = NULL;

		if (cancellable)
			FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

#ifdef __FEATURE_HEADER_OPTIMIZATION__
		env = mail_fetchstructure_full(stream, uid_elem->msgno, NULL, FT_PEEK, 0);
#else
		env = mail_fetchstructure_full(stream, uid_elem->msgno, NULL, FT_PEEK);
#endif
		search_message_cache = mail_elt(stream, uid_elem->msgno);
		if (search_message_cache) {
			uid_elem->flag.seen = search_message_cache->seen;
			uid_elem->flag.draft = search_message_cache->draft;
			uid_elem->flag.flagged = search_message_cache->flagged;
		}

		if (!emcore_make_mail_tbl_data_from_envelope(multi_user_name,
													account_id,
													stream,
													env,
													uid_elem,
													&new_mail_tbl_data,
													&err)) {
			EM_DEBUG_EXCEPTION("emcore_make_mail_tbl_data_from_envelope failed : [%d]", err);
			goto FINISH_OFF;
		}


		if ((err = emcore_add_mail_to_mailbox(multi_user_name,
												search_mailbox,
												new_mail_tbl_data,
												&mail_id,
												&thread_id)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_add_mail_to_mailbox failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emcore_initiate_pbd(multi_user_name,
									stream,
									account_id,
									mail_id,
									uid_elem->uid,
									search_mailbox->mailbox_id,
									search_mailbox->mailbox_name,
									&err)) {
			EM_DEBUG_EXCEPTION("emcore_initiate_pbd failed : [%d]", err);
		}

		if (!emcore_notify_storage_event(NOTI_MAIL_ADD, account_id, mail_id, mailbox_id_string, thread_id)) {
			EM_DEBUG_EXCEPTION("emcore_notify_storage_event[NOTI_MAIL_ADD] failed");
		}

		if (new_mail_tbl_data) {
			emstorage_free_mail(&new_mail_tbl_data, 1, NULL);
			new_mail_tbl_data = NULL;
		}

		uid_elem = uid_elem->next;
	}

FINISH_OFF:

	if (stream)
		stream = mail_close(stream);

	if (search_program)
		mail_free_searchpgm(&search_program);

	if (new_mail_tbl_data) {
		emstorage_free_mail(&new_mail_tbl_data, 1, NULL);
		new_mail_tbl_data = NULL;
	}

	if (uid_list)
		emcore_free_uids(uid_list, NULL);

	if (search_mailbox)
		emstorage_free_mailbox(&search_mailbox, 1, NULL);

	EM_SAFE_FREE(charset);
	EM_SAFE_FREE(uid_range);
	EM_SAFE_FREE(mailbox_id_string);

	EM_DEBUG_FUNC_END();
	return err;
}

/*
	It is fast. because this api search directly on server.
	but do not search encoded words.
 */
INTERNAL_FUNC int emcore_search_on_server(char *multi_user_name, int account_id, int mailbox_id,
										email_search_filter_t *input_search_filter,
										int input_search_filter_count,
										int cancellable,
										int event_handle)
{
	EM_DEBUG_FUNC_BEGIN_SEC("account_id : [%d], mailbox_id : [%d]", account_id, mailbox_id);

	int err = EMAIL_ERROR_NONE;
	int mail_id = 0;
	int thread_id = 0;
	int uid_count = 0;
	char *uid_range = NULL;
	char mailbox_id_param_string[10] = {0,};
	emcore_uid_list *uid_list = NULL;
	emcore_uid_list *uid_elem = NULL;
	emstorage_mailbox_tbl_t *search_mailbox = NULL;
	emstorage_mail_tbl_t *new_mail_tbl_data = NULL;

	MAILSTREAM *stream = NULL;
	ENVELOPE *env = NULL;

	if (account_id < 0 || mailbox_id == 0) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	if (!emcore_connect_to_remote_mailbox(multi_user_name,
											account_id,
											mailbox_id,
											true,
											(void **)&stream,
											&err)) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed");
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("nmsgs : [%ld]", stream->nmsgs);

	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	if ((err = emcore_search_mail_and_uids(stream,
											input_search_filter,
											input_search_filter_count,
											&uid_list,
											&uid_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_search_mail_and_uids failed [%d]", err);
		goto FINISH_OFF;
	}

	if (cancellable)
		FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

	uid_elem = uid_list;

	if (uid_count > 0) {
		if ((err = emcore_make_uid_range_string(uid_list, uid_count, &uid_range)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_make_uid_range_string failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("Calling ... mail_fetch_fast. uid_range [%s]", uid_range);
		mail_fetch_fast(stream, uid_range, FT_UID | FT_PEEK | FT_NEEDENV);
		EM_SAFE_FREE(uid_range);

		EM_DEBUG_LOG("stream->nmsgs : [%d]", stream->nmsgs);
	}

	search_mailbox = em_malloc(sizeof(emstorage_mailbox_tbl_t));
	if (search_mailbox == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	search_mailbox->account_id = account_id;
	search_mailbox->mailbox_id = mailbox_id;
	search_mailbox->mailbox_name = EM_SAFE_STRDUP(EMAIL_SEARCH_RESULT_MAILBOX_NAME);
	search_mailbox->mailbox_type = EMAIL_MAILBOX_TYPE_SEARCH_RESULT;

	memset(mailbox_id_param_string, 0, 10);
	SNPRINTF(mailbox_id_param_string, 10, "%d", search_mailbox->mailbox_id);

	while (uid_elem) {
		if (cancellable)
			FINISH_OFF_IF_EVENT_CANCELED(err, event_handle);

		EM_DEBUG_LOG("msgno : [%4lu]", uid_elem->msgno);
#ifdef __FEATURE_HEADER_OPTIMIZATION__
		env = mail_fetchstructure_full(stream, uid_elem->msgno, NULL, FT_PEEK, 0);
#else
		env = mail_fetchstructure_full(stream, uid_elem->msgno, NULL, FT_PEEK);
#endif
		EM_DEBUG_LOG("env[%p]", env);
		if (env)
			EM_DEBUG_LOG("message_id[%s]", env->message_id);

		if (!emcore_make_mail_tbl_data_from_envelope(multi_user_name,
													account_id,
													stream,
													env,
													uid_elem,
													&new_mail_tbl_data,
													&err) || !new_mail_tbl_data) {
			EM_DEBUG_EXCEPTION("emcore_make_mail_tbl_data_from_envelope failed [%d]", err);
			goto FINISH_OFF;
		}

		if ((err = emcore_add_mail_to_mailbox(multi_user_name,
												search_mailbox,
												new_mail_tbl_data,
												&mail_id,
												&thread_id)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_add_mail_to_mailbox failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emcore_initiate_pbd(multi_user_name,
									stream,
									account_id,
									mail_id,
									uid_elem->uid,
									search_mailbox->mailbox_id,
									search_mailbox->mailbox_name,
									&err)) {
			EM_DEBUG_EXCEPTION("emcore_initiate_pbd failed : [%d]", err);
		}

		if (!emcore_notify_storage_event(NOTI_MAIL_ADD, account_id, mail_id, mailbox_id_param_string, thread_id)) {
			EM_DEBUG_EXCEPTION("emcore_notify_storage_event[NOTI_MAIL_ADD] failed");
		}

		if (new_mail_tbl_data) {
			emstorage_free_mail(&new_mail_tbl_data, 1, NULL);
			new_mail_tbl_data = NULL;
		}

		uid_elem = uid_elem->next;
	}

FINISH_OFF:

	if (stream)
		mail_close(stream);

	if (uid_list)
		emcore_free_uids(uid_list, NULL);

	if (search_mailbox != NULL)
		emstorage_free_mailbox(&search_mailbox, 1, NULL);

	if (new_mail_tbl_data)
		emstorage_free_mail(&new_mail_tbl_data, 1, NULL);

	EM_DEBUG_FUNC_END();
	return err;

}
