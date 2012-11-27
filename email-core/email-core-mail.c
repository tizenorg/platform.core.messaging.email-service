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

#include <vconf.h> 
#include <contacts.h>

#include "email-internal-types.h"
#include "c-client.h"
#include "lnx_inc.h"
#include "email-utilities.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-core-mail.h"
#include "email-core-mime.h"
#include "email-core-mailbox.h"
#include "email-storage.h"
#include "email-network.h"
#include "email-core-mailbox-sync.h"
#include "email-core-event.h"
#include "email-core-account.h" 

#include "email-convert.h"
#include "email-debug-log.h"

#ifdef __FEATURE_DRM__
#include <drm_client.h>
#endif /* __FEATURE_DRM__ */

#define ST_SILENT   (long) 0x2	/* don't return results */
#define ST_SET      (long) 0x4	/* set vs. clear */

static char g_new_server_uid[129];

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

BODY **g_inline_list = NULL ;
int g_inline_count = 0;


static int emcore_delete_mails_from_pop3_server(email_account_t *input_account, int input_mail_ids[], int input_mail_id_count);
static int emcore_mail_cmd_read_mail_pop3(void *stream, int msgno, int limited_size, int *downloded_size, int *total_body_size, int *err_code);

extern long pop3_send (MAILSTREAM *stream, char *command, char *args);


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
		EM_DEBUG_EXCEPTION("param->attribute[%s]", param->attribute);
		EM_DEBUG_EXCEPTION("param->value[%s]", param->value);
		
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

static void _print_body(BODY *body, int recursive)
{
	EM_DEBUG_LOG(" ========================================================== ");
	
	if (body != NULL) {
		EM_DEBUG_LOG("body->type[%s]", _getType(body->type));
		EM_DEBUG_LOG("body->encoding[%s]", _getEncoding(body->encoding));
		EM_DEBUG_LOG("body->subtype[%s]", body->subtype);
		
		EM_DEBUG_LOG("body->parameter[%p]", body->parameter);
		
		_print_parameter(body->parameter);
		
		EM_DEBUG_LOG("body->id[%s]", body->id);
		EM_DEBUG_LOG("body->description[%s]", body->description);
		
		EM_DEBUG_LOG("body->disposition.type[%s]", body->disposition.type);
		EM_DEBUG_LOG("body->disposition.parameter[%p]", body->disposition.parameter);
		
		_print_parameter(body->disposition.parameter);
		
		EM_DEBUG_LOG("body->language[%p]", body->language);
		
		_print_stringlist(body->language);
		
		EM_DEBUG_LOG("body->location[%s]", body->location);
	
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
		
		if (recursive)  {
			PART *part = body->nested.part;
			
			while (part != NULL)  {
				_print_body(&(part->body), recursive);
				part = part->next;
			}
		}
	}
	
	EM_DEBUG_LOG(" ========================================================== ");
}
#endif /*  FEATURE_CORE_DEBUG */

static int convert_contact_err_to_email_err(int contact_err)
{
	int err = EMAIL_ERROR_NONE;

	switch (contact_err) {
	case CONTACTS_ERROR_NONE :
		err = EMAIL_ERROR_NONE;
		break;
	case CONTACTS_ERROR_OUT_OF_MEMORY :
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		break;
	case CONTACTS_ERROR_INVALID_PARAMETER :
		err = EMAIL_ERROR_INVALID_PARAM;
		break;
	case CONTACTS_ERROR_NO_DATA :
		err = EMAIL_ERROR_DATA_NOT_FOUND;
		break;
	case CONTACTS_ERROR_DB :
		err = EMAIL_ERROR_DB_FAILURE;
		break;
	case CONTACTS_ERROR_IPC :
		err = EMAIL_ERROR_IPC_CONNECTION_FAILURE;
		break;
	case CONTACTS_ERROR_SYSTEM:
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		break;
	case CONTACTS_ERROR_INTERNAL:
	default:
		err = EMAIL_ERROR_UNKNOWN;
		break;
	}
	return err;
}

static int pop3_mail_delete(MAILSTREAM *stream, int msgno, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msgno[%d], err_code[%p]", stream, msgno, err_code);
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	
	POP3LOCAL *pop3local = NULL;
	char cmd[64];
	char *p = NULL;
	
	if (!stream)  {
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
	if (!net_sout(pop3local->netstream, cmd, (int)strlen(cmd))) {
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
	if (p)
		free(p);
	
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
	new_server_uid = em_malloc(count * sizeof(unsigned long));

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

	/* For loop below updates mail_tbl and mail_read_mail_uid_tbl with new server uids*/
	
	for (i = 0; i <= index; ++i) {

		memset(old_server_uid_char, 0x00, sizeof(old_server_uid_char));
		sprintf(old_server_uid_char, "%ld", old_server_uid[i]);

		EM_DEBUG_LOG("Old Server Uid Char[%s]", old_server_uid_char);
		
		memset(new_server_uid_char, 0x00, sizeof(new_server_uid_char));
		sprintf(new_server_uid_char, "%ld", new_server_uid[i]);

		EM_DEBUG_LOG("New Server Uid Char[%s]", new_server_uid_char);
	
		if (!emstorage_update_server_uid(old_server_uid_char, new_server_uid_char, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_update_server_uid failed...[%d]", err);
		}
		
		if (!emstorage_update_read_mail_uid_by_server_uid(old_server_uid_char, new_server_uid_char, mailbox, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_update_read_mail_uid_by_server_uid failed... [%d]", err);
		}
		
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

		if (false == emstorage_update_pbd_activity(old_server_uid_char, new_server_uid_char, mailbox, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_update_pbd_activity failed... [%d]", err);
		}
		
#endif
	}

	EM_SAFE_FREE(old_server_uid);
	EM_SAFE_FREE(new_server_uid);
	
}

INTERNAL_FUNC int emcore_move_mail_on_server_ex(int account_id, int src_mailbox_id,  int mail_ids[], int num, int dest_mailbox_id, int *error_code)
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
	
	if (num <= 0  || account_id <= 0 || src_mailbox_id <= 0 || dest_mailbox_id <= 0 || NULL == mail_ids)  {
		if (error_code != NULL) {
			*error_code = EMAIL_ERROR_INVALID_PARAM;
		}
		EM_DEBUG_LOG("Invalid Parameters- num[%d], account_id[%d], src_mailbox_id[%d], dest_mailbox_id[%d], mail_ids[%p]", num, account_id, src_mailbox_id, dest_mailbox_id, mail_ids);
		return false;
	}

	ref_account = emcore_get_account_reference(account_id);
	
	if (NULL == ref_account) {
		EM_DEBUG_EXCEPTION(" emcore_get_account_reference failed[%d]", account_id);
		
		*error_code = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

 
	if (ref_account->incoming_server_type != EMAIL_SERVER_TYPE_IMAP4) {
		*error_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}


	if (!emcore_connect_to_remote_mailbox(account_id, src_mailbox_id, (void **)&stream, &err_code))		 {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed[%d]", err_code);
		
		goto FINISH_OFF;
	}

	if (NULL != stream) {
		mail_parameters(stream, SET_COPYUID, emcore_mail_copyuid_ex);
		EM_DEBUG_LOG("calling mail_copy_full FODLER MAIL COPY ");
		/*  [h.gahlaut] Break the set of mail_ids into comma separated strings of given length  */
		/*  Length is decided on the basis of remaining keywords in the Query to be formed later on in emstorage_get_id_set_from_mail_ids  */
		/*  Here about 90 bytes are required for fixed keywords in the query-> SELECT local_uid, s_uid from mail_read_mail_uid_tbl where local_uid in (....) ORDER by s_uid  */
		/*  So length of comma separated strings which will be filled in (.....) in above query can be maximum QUERY_SIZE - 90  */
		
		if (false == emcore_form_comma_separated_strings(mail_ids, num, QUERY_SIZE - 90, &string_list, &string_count, &err_code))   {
			EM_DEBUG_EXCEPTION("emcore_form_comma_separated_strings failed [%d]", err_code);
			goto FINISH_OFF;
		}
		
		if ( (err_code = emstorage_get_mailbox_by_id(dest_mailbox_id, &dest_mailbox)) != EMAIL_ERROR_NONE || !dest_mailbox) {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err_code);
			goto FINISH_OFF;
		}

		/*  Now execute one by one each comma separated string of mail_ids */

		for (i = 0; i < string_count; ++i) {
			/*  Get the set of mail_ds and corresponding server_mail_ids sorted by server mail ids in ascending order */
		
			if (false == emstorage_get_id_set_from_mail_ids(string_list[i], &id_set, &id_set_count, &err_code)) {
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
				uid_range_node->uid_range[strlen(uid_range_node->uid_range) - 1] = '\0';
				EM_DEBUG_LOG("uid_range_node->uid_range - %s", uid_range_node->uid_range);
				if (!mail_copy_full(stream, uid_range_node->uid_range, dest_mailbox->mailbox_name, CP_UID | CP_MOVE)) {
					EM_DEBUG_EXCEPTION("emcore_move_mail_on_server_ex :   Mail cannot be moved failed");
					EM_DEBUG_EXCEPTION("Mail MOVE failed ");
					goto FINISH_OFF;
				}
				else if (!mail_expunge_full(stream, uid_range_node->uid_range, EX_UID)) {
					EM_DEBUG_EXCEPTION("emcore_move_mail_on_server_ex :   Mail cannot be expunged after move. failed!");
						EM_DEBUG_EXCEPTION("Mail Expunge after move failed ");
						  goto FINISH_OFF;
				}
				else {
					EM_DEBUG_LOG("Mail MOVE SUCCESS ");
				}
				
				uid_range_node = uid_range_node->next;
			}	

			emcore_free_uid_range_set(&uid_range_set);

			EM_SAFE_FREE(id_set);
			
			id_set_count = 0;
		}
	
	}
	else {
		EM_DEBUG_EXCEPTION(">>>> STREAM DATA IS NULL >>> ");
		goto FINISH_OFF;
	}
	

	ret = true;

FINISH_OFF: 
	emcore_close_mailbox(0, stream);
	stream = NULL;

#ifdef __FEATURE_LOCAL_ACTIVITY__
	if (ret || ref_account->incoming_server_type != EMAIL_SERVER_TYPE_IMAP4) /* Delete local activity for POP3 mails and successful move operation in IMAP */ {
		emstorage_activity_tbl_t new_activity;
		for (i = 0; i<num ; i++) {		
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

int emcore_delete_mails_from_imap4_server(int mail_ids[], int num, int from_server, int *err_code)
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
	int delete_success = false;
	emstorage_mail_tbl_t *mail_tbl_data = NULL;
		
	if (num <= 0 || !mail_ids) {
		EM_DEBUG_EXCEPTION(" Invalid parameter ");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	for(i = 0; i < num; i++) {
		if (!emstorage_get_downloaded_mail(mail_ids[i], &mail_tbl_data, false, &err) || !mail_tbl_data)  {
			EM_DEBUG_EXCEPTION("emstorage_get_downloaded_mail failed [%d]", err);

			if (err == EMAIL_ERROR_MAIL_NOT_FOUND) {	/* not server mail */
				continue;
			}
			else
				break;
		}
	}

	if (!emcore_connect_to_remote_mailbox(mail_tbl_data->account_id, mail_tbl_data->mailbox_id , (void **)&stream, &err))  {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		goto FINISH_OFF;
	}

	/*  [h.gahlaut] Break the set of mail_ids into comma separated strings of given length  */
	/*  Length is decided on the basis of remaining keywords in the Query to be formed later on in emstorage_get_id_set_from_mail_ids  */
	/*  Here about 90 bytes are required for fixed keywords in the query-> SELECT local_uid, s_uid from mail_read_mail_uid_tbl where local_uid in (....) ORDER by s_uid  */
	/*  So length of comma separated strings which will be filled in (.....) in above query can be maximum QUERY_SIZE - 90  */

	if (false == emcore_form_comma_separated_strings(mail_ids, num, QUERY_SIZE - 90, &string_list, &string_count, &err))   {
		EM_DEBUG_EXCEPTION("emcore_form_comma_separated_strings failed [%d]", err);
		goto FINISH_OFF;
	}

	/*  Now execute one by one each comma separated string of mail_ids  */
	
	for (i = 0; i < string_count; ++i) {
		/*  Get the set of mail_ds and corresponding server_mail_ids sorted by server mail ids in ascending order  */

		if (false == emstorage_get_id_set_from_mail_ids(string_list[i], &id_set, &id_set_count, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_id_set_from_mail_ids failed [%d]", err);
			goto FINISH_OFF;
		}
		
		/*  Convert the sorted sequence of server mail ids to range sequences of given length. A range sequence will be like A : B, C, D: E, H  */
		
		len_of_each_range = MAX_IMAP_COMMAND_LENGTH - 40;		/*   1000 is the maximum length allowed RFC 2683. 40 is left for keywords and tag.  */
		
		if (false == emcore_convert_to_uid_range_set(id_set, id_set_count, &uid_range_set, len_of_each_range, &err)) {
			EM_DEBUG_EXCEPTION("emcore_convert_to_uid_range_set failed [%d]", err);
			goto FINISH_OFF;
		}

		uid_range_node = uid_range_set;

		while (uid_range_node != NULL) {
			/*  Remove comma from end of uid_range  */

			uid_range_node->uid_range[strlen(uid_range_node->uid_range) - 1] = '\0';

			if (!(imaplocal = stream->local) || !imaplocal->netstream)  {
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
			if (!net_sout(imaplocal->netstream, cmd, (int)strlen(cmd)))  {
				EM_DEBUG_EXCEPTION("net_sout failed...");
				
				err = EMAIL_ERROR_CONNECTION_BROKEN;		/* EMAIL_ERROR_UNKNOWN */
				goto FINISH_OFF;
			}

			
			while (imaplocal->netstream)  {
				/* receive response */
				if (!(p = net_getline(imaplocal->netstream))) {
					EM_DEBUG_EXCEPTION("net_getline failed...");
					
					err = EMAIL_ERROR_INVALID_RESPONSE;		/* EMAIL_ERROR_UNKNOWN; */
					goto FINISH_OFF;
				}
				

				EM_DEBUG_LOG("[IMAP4] <<< %s", p);

				/* To confirm - Commented out as FETCH response does not contain the tag - may be a problem for common stream in email-service*/
				/* Success case - delete all local activity and entry from mail_read_mail_uid_tbl 
				if (strstr(p, "FETCH") != NULL)  {
					EM_DEBUG_LOG(" FETCH Response recieved ");
					delete_success = true;
					EM_SAFE_FREE(p);
					break;
				}
				*/
		
			
				if (!strncmp(p, tag, strlen(tag)))  {
					if (!strncmp(p + strlen(tag) + 1, "OK", 2))  {
						/*Error scenario delete all local activity and entry from mail_read_mail_uid_tbl */
						EM_DEBUG_LOG(" OK Response recieved ");
						delete_success = true;
						EM_SAFE_FREE(p);
						break;
					}					
					else  {
						/*  'NO' or 'BAD' */
						delete_success = false;
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
				if (!net_sout(imaplocal->netstream, cmd, (int)strlen(cmd)))  {
					EM_DEBUG_EXCEPTION("net_sout failed...");
					
					err = EMAIL_ERROR_CONNECTION_BROKEN;
					goto FINISH_OFF;
				}

				while (imaplocal->netstream)  {
					/* receive response */
					if (!(p = net_getline(imaplocal->netstream))) {
						EM_DEBUG_EXCEPTION("net_getline failed...");
						
						err = EMAIL_ERROR_INVALID_RESPONSE;		/* EMAIL_ERROR_UNKNOWN; */
						goto FINISH_OFF;
					}
					
					EM_DEBUG_LOG("[IMAP4] <<< %s", p);
					
					if (!strncmp(p, tag, strlen(tag)))  {
						if (!strncmp(p + strlen(tag) + 1, "OK", 2))  {
#ifdef __FEATURE_LOCAL_ACTIVITY__
							int index = 0;
							emstorage_mail_tbl_t **mail = NULL;
							
							mail = (emstorage_mail_tbl_t **) em_malloc(num * sizeof(emstorage_mail_tbl_t *));
							if (!mail) {
								EM_DEBUG_EXCEPTION("em_malloc failed");
								err = EMAIL_ERROR_UNKNOWN;
								goto FINISH_OFF;
							}

							if (delete_success) {
								for (index = 0 ; index < num; index++) {
									if (!emstorage_get_downloaded_mail(mail_ids[index], &mail[index], false, &err))  {
										EM_DEBUG_LOG("emstorage_get_uid_by_mail_id failed [%d]", err);
										
										if (err == EMAIL_ERROR_MAIL_NOT_FOUND)  {		
											EM_DEBUG_LOG("EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER  : ");
											continue;
										}
									}		
								
									if (mail[index]) {				
										/* Clear entry from mail_read_mail_uid_tbl */
										if (mail[index]->server_mail_id != NULL) {
											if (!emstorage_remove_downloaded_mail(mail[index]->account_id, mail[index]->mailbox_name, mail[index]->server_mail_id, true, &err))  {
												EM_DEBUG_LOG("emstorage_remove_downloaded_mail falied [%d]", err);
											}
										}
										
										/* Delete local activity */
										emstorage_activity_tbl_t new_activity;
										memset(&new_activity, 0x00, sizeof(emstorage_activity_tbl_t));
										if (from_server == EMAIL_DELETE_FOR_SEND_THREAD) {
											new_activity.activity_type = ACTIVITY_DELETEMAIL_SEND;
											EM_DEBUG_LOG("from_server == EMAIL_DELETE_FOR_SEND_THREAD ");
										}
										else {
											new_activity.activity_type	= ACTIVITY_DELETEMAIL;
										}

										new_activity.mail_id		= mail[index]->mail_id;
										new_activity.server_mailid	= NULL;
										new_activity.src_mbox		= NULL;
										new_activity.dest_mbox		= NULL;
														
										if (!emcore_delete_activity(&new_activity, &err)) {
											EM_DEBUG_EXCEPTION(" emcore_delete_activity  failed  - %d ", err);
										}
									}
									else {
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
						}
						else  {		/*  'NO' or 'BAD' */
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

typedef enum {
	IMAP4_CMD_EXPUNGE
} imap4_cmd_t;

static int imap4_send_command(MAILSTREAM *stream, imap4_cmd_t cmd_type, int *err_code)
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
	SNPRINTF(cmd, sizeof(cmd), "%s EXPUNGE\015\012", tag);

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG("[IMAP4] >>> %s", cmd);
#endif
	
	/*  send command  :  delete flaged mail */
	if (!net_sout(imaplocal->netstream, cmd, (int)strlen(cmd))) {
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
		
		if (!strncmp(p, tag, strlen(tag))) {
			if (!strncmp(p + strlen(tag) + 1, "OK", 2)) {
				EM_SAFE_FREE(p);
				break;
			}
			else {		/*  'NO' or 'BAD' */
				err = EMAIL_ERROR_IMAP4_EXPUNGE_FAILURE;		/* EMAIL_ERROR_INVALID_RESPONSE; */
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

int emcore_get_mail_contact_info(email_mail_contact_info_t *contact_info, char *full_address, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("contact_info[%p], full_address[%s], err_code[%p]", contact_info, full_address, err_code);
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	
	if (!emcore_get_mail_contact_info_with_update(contact_info, full_address, 0, &err))
		EM_DEBUG_EXCEPTION("emcore_get_mail_contact_info_with_update failed [%d]", err);
	else
		ret = true;
	
	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}

int emcore_search_contact_info_by_address(const char *contact_uri, int property_id, char *address, int limit, contacts_record_h *contacts_record)
{
	EM_DEBUG_FUNC_BEGIN();
	int contact_err = CONTACTS_ERROR_NONE;
	
	contacts_query_h query = NULL;
	contacts_filter_h filter = NULL;
	contacts_list_h list = NULL;

	if ((contact_err = contacts_query_create(contact_uri, &query)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_query_create failed");
		goto FINISH_OFF;
	}
	
	if ((contact_err = contacts_filter_create(contact_uri, &filter)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_filter_create failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_filter_add_str(filter, property_id, CONTACTS_MATCH_EXACTLY, address)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_filter_add_str failed");
		goto FINISH_OFF;
	}
	
	if ((contact_err = contacts_query_set_filter(query, filter)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_query_set_filter failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_db_get_records_with_query(query, 0, limit, &list)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_db_get_record_with_query failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_list_get_current_record_p(list, contacts_record)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_list_get_current_record_p failed");
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (query != NULL)
		contacts_query_destroy(query);

	if (filter != NULL)
		contacts_filter_destroy(filter);

	if (list != NULL)
		contacts_list_destroy(list, false);

	return contact_err;
}

int emcore_set_contacts_log(int account_id, char *email_address, char *subject, time_t date_time, email_action_t action, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id : [%d], address : [%p], subject : [%s], action : [%d], date_time : [%d]", account_id, email_address, subject, action, (int)date_time);
	
	int ret = true;
	int contacts_error = CONTACTS_ERROR_NONE;

	int person_id = 0;
	int action_type = 0;
	
	contacts_record_h phone_record = NULL;
	contacts_record_h person_record = NULL;	

	if ((contacts_error = contacts_connect2()) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("Open connect service failed");
		goto FINISH_OFF;
	}

	if ((contacts_error = contacts_record_create(_contacts_phone_log._uri, &phone_record)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_query_create failed");
		goto FINISH_OFF;
	}

	/* Set email address */
	if ((contacts_error = contacts_record_set_str(phone_record, _contacts_phone_log.address, email_address)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_record_set_str [address] failed");
		goto FINISH_OFF;
	}

	/* Search contact person info */
	if ((contacts_error = emcore_search_contact_info_by_address(_contacts_person_email._uri, _contacts_person_email.email, email_address, 1, &person_record)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_LOG("emcore_search_contact_info_by_address failed");
		EM_DEBUG_LOG("Not match person");
	} else {
		/* Get person_id in contacts_person_email record  */
		if (person_record  && (contacts_error = contacts_record_get_int(person_record, _contacts_person_email.person_id, &person_id)) != CONTACTS_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("contacts_record_get_str failed");
			goto FINISH_OFF;
		}

		/* Set the person id */
		if ((contacts_error = contacts_record_set_int(phone_record, _contacts_phone_log.person_id, person_id)) != CONTACTS_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("contacts_record_set_int [person id] failed");
			goto FINISH_OFF;
		}
	}

	switch (action) {
	case EMAIL_ACTION_SEND_MAIL :
		action_type = CONTACTS_PLOG_TYPE_EMAIL_SENT;
		break;
	case EMAIL_ACTION_SYNC_HEADER :
		action_type = CONTACTS_PLOG_TYPE_EMAIL_RECEIVED;
		break;
	default :
		EM_DEBUG_EXCEPTION("Unknow action type");
		goto FINISH_OFF;
	}

	/* Set log type */
	if ((contacts_error = contacts_record_set_int(phone_record, _contacts_phone_log.log_type, action_type)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_record_set_int [log_type] failed");
		goto FINISH_OFF;
	}

	/* Set log time */
	if ((contacts_error = contacts_record_set_int(phone_record, _contacts_phone_log.log_time, (int)date_time)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_record_set_str [address] failed");
		goto FINISH_OFF;
	}

	/* Set subject */
	if ((contacts_error = contacts_record_set_str(phone_record, _contacts_phone_log.extra_data2, subject)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_record_set_str [subject] failed");
		goto FINISH_OFF;
	}

	/* Set Mail id */
	if ((contacts_error = contacts_record_set_int(phone_record, _contacts_phone_log.extra_data1, account_id)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_record_set_int [mail id] failed");
		goto FINISH_OFF;
	}

	/* Insert the record in DB */
	if ((contacts_error = contacts_db_insert_record(phone_record, NULL)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_db_insert_record failed");
		goto FINISH_OFF;
	}


FINISH_OFF:

	if (phone_record != NULL)
		contacts_record_destroy(phone_record, false);

	if (person_record != NULL)
		contacts_record_destroy(person_record, false);

	if ((contacts_error = contacts_disconnect2()) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("Open connect service failed");
		goto FINISH_OFF;
	}

	if (contacts_error != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts error : [%d]", contacts_error);
		ret = false;
	}

	if (err_code != NULL)
		*err_code = convert_contact_err_to_email_err(contacts_error);

	return ret;
}

INTERNAL_FUNC int emcore_set_sent_contacts_log(emstorage_mail_tbl_t *input_mail_data, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data : [%p]", input_mail_data);
	
	int i = 0, ret = false;
	int err = EMAIL_ERROR_NONE;
	char email_address[MAX_EMAIL_ADDRESS_LENGTH];
	char *address_array[3] = {input_mail_data->full_address_to, input_mail_data->full_address_cc, input_mail_data->full_address_bcc};
	ADDRESS *addr = NULL;
	ADDRESS *p_addr = NULL;

	for (i = 0; i < 3; i++) {
		if (address_array[i] && address_array[i][0] != 0) {
			rfc822_parse_adrlist(&addr, address_array[i], NULL);
			for (p_addr = addr ; p_addr ;p_addr = p_addr->next) {
				SNPRINTF(email_address, MAX_EMAIL_ADDRESS_LENGTH, "%s@%s", addr->mailbox, addr->host);
				if (!emcore_set_contacts_log(input_mail_data->account_id, email_address, input_mail_data->subject, input_mail_data->date_time, EMAIL_ACTION_SEND_MAIL, &err)) {
					EM_DEBUG_EXCEPTION("emcore_set_contacts_log failed : [%d]", err);
					goto FINISH_OFF;
				}
			}
		
			if (addr) {	
				mail_free_address(&addr);
				addr = NULL;
			}
		}
	}

	ret = true;

FINISH_OFF:

	if (addr) 
		mail_free_address(&addr);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_set_received_contacts_log(emstorage_mail_tbl_t *input_mail_data, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data : [%p]", input_mail_data);

	if (!emcore_set_contacts_log(input_mail_data->account_id, input_mail_data->email_address_sender, input_mail_data->subject, input_mail_data->date_time, EMAIL_ACTION_SYNC_HEADER, err_code)) {
		EM_DEBUG_EXCEPTION("emcore_set_contacts_log failed");	
		return false;
	}

	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emcore_delete_contacts_log(int account_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id : [%d]", account_id);

	int ret = false;
	int contacts_error = CONTACTS_ERROR_NONE;

	if ((contacts_error = contacts_connect2()) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("Open connect service failed");
		goto FINISH_OFF;
	}
	
	/* Delete record of the account id */
	if ((contacts_error = contacts_phone_log_delete(CONTACTS_PHONE_LOG_DELETE_BY_EMAIL_EXTRA_DATA1, account_id)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_phone_log_delete failed");
		goto FINISH_OFF;
	}
	
	if ((contacts_error = contacts_disconnect2()) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("Open connect service failed");
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (err_code != NULL)
		*err_code = convert_contact_err_to_email_err(contacts_error);

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_get_mail_display_name(char *email_address, char **contact_display_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("contact_name_value[%s], contact_display_name[%p]", email_address, contact_display_name);

	int contact_err = 0;
	int ret = false;
	char *display = NULL;
	/* Contact variable */
	contacts_record_h record = NULL;

	if ((contact_err = emcore_search_contact_info_by_address(_contacts_contact_email._uri, _contacts_contact_email.email, email_address, 1, &record)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_search_contact_info_by_address failed");
		goto FINISH_OFF;
	}
/*
	if ((contact_err = contacts_list_get_count(list, &list_count)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_list_get_count failed");
		goto FINISH_OFF;
	}

	if (list_count > 1) {
		EM_DEBUG_EXCEPTION("Duplicated contacts info");
		contact_err = CONTACTS_ERROR_INTERNAL;
		goto FINISH_OFF;
	} else if (list_count == 0) {
		EM_DEBUG_EXCEPTION("Not found contact info");
		contact_err = CONTACTS_ERROR_NO_DATA;
		goto FINISH_OFF;
	}
*/

	if (contacts_record_get_str(record, _contacts_contact_email.display_name, &display) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_record_get_str failed");
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (record != NULL)
		contacts_record_destroy(record, false);

	if (ret) {
		*contact_display_name = display;
	} else {
		*contact_display_name = NULL;
		EM_SAFE_FREE(display);
	}

	if (err_code != NULL)
		*err_code = convert_contact_err_to_email_err(contact_err);

	return ret;
}

int emcore_get_mail_contact_info_with_update(email_mail_contact_info_t *contact_info, char *full_address, int mail_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("contact_info[%p], full_address[%s], mail_id[%d], err_code[%p]", contact_info, full_address, mail_id, err_code);
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	ADDRESS *addr = NULL;
	char *address = NULL;
	char *temp_emailaddr = NULL;
	int start_text_ascii = 2;
	int end_text_ascii = 3;	
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

	if (!contact_info)  {
		EM_DEBUG_EXCEPTION("contact_info[%p]", contact_info);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!full_address) {
		full_address = "";
		address_length = 0;
		temp_emailaddr = NULL;
	}
	else {
		address_length = 2 * strlen(full_address);
		temp_emailaddr = (char  *)calloc(1, address_length); 
	}

    em_skip_whitespace(full_address , &address);
	EM_DEBUG_LOG("address[address][%s]", address);  


	/*  ',' -> "%2C" */
	gchar **tokens = g_strsplit(address, ", ", -1);
	char *p = g_strjoinv("%2C", tokens);
	
	g_strfreev(tokens);
	
	/*  ';' -> ',' */
	while (p && p[i] != '\0') 
    {
		if (p[i] == ';') 
			p[i] = ',';
		i++;
	}
	EM_DEBUG_LOG("  2  converted address %s ", p);

	rfc822_parse_adrlist(&addr, p, NULL);

	EM_SAFE_FREE(p);
	EM_DEBUG_LOG("  3  full_address  %s ", full_address);

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
	
	while (addr != NULL)  {
		if (addr->mailbox && addr->host) {	
			if (!strncmp(addr->mailbox , "UNEXPECTED_DATA_AFTER_ADDRESS", strlen("UNEXPECTED_DATA_AFTER_ADDRESS")) || !strncmp(addr->mailbox , "INVALID_ADDRESS", strlen("INVALID_ADDRESS")) || !strncmp(addr->host , ".SYNTAX-ERROR.", strlen(".SYNTAX-ERROR.")))
	       	{
				EM_DEBUG_LOG("Invalid address ");
				addr = addr->next;
				continue;
			}
		}
		else  {
			EM_DEBUG_LOG("Error in parsing..! ");
			addr = addr->next;
			continue;
		}

		EM_SAFE_FREE(email_address);
		email_address = g_strdup_printf("%s@%s", addr->mailbox ? addr->mailbox  :  "", addr->host ? addr->host  :  "");
		
		EM_DEBUG_LOG(" addr->personal[%s]", addr->personal);
		EM_DEBUG_LOG(" email_address[%s]", email_address);
	
		is_searched = false;
		EM_DEBUG_LOG(" >>>>> emcore_get_mail_contact_info - 10");
	
		if (emcore_get_mail_display_name(email_address, &contact_display_name_from_contact_info, &err) && err == EMAIL_ERROR_NONE) {
			contact_display_name = contact_display_name_from_contact_info;

			EM_DEBUG_LOG(">>> contact_name[%s]", contact_display_name);
			/*  Make display name string */
			is_searched = true;

			if (mail_id == 0 || (contact_name_len == 0)) {	/*  save only the first address information - 09-SEP-2010 */
				contact_display_name_len = strlen(contact_display_name);
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
				}
				else {
					snprintf(temp_string, sizeof(temp_string), "\"%s\" <%s>, ", contact_display_name, email_address);
				}					

				contact_display_name_len = strlen(temp_string);
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
				EM_DEBUG_LOG("new contact_name >>>>> %s ", contact_name);
			}
		}
		else {
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
		}
		else {
			/* alias = addr->mailbox ? addr->mailbox  :  ""; */
			alias = email_address;
		}
		contact_info->alias = EM_SAFE_STRDUP(alias);

		if (!is_searched) {
			contact_display_name = alias;
			contact_info->contact_id = -1;		/*  NOTE :  This is valid only if there is only one address. */
			contact_info->storage_type = -1;

			/*  Make display name string */
			EM_DEBUG_LOG("contact_display_name : [%s]", contact_display_name);
			EM_DEBUG_LOG("email_address : [%s]", email_address);

			/*  if mail_id is 0, return only contact info without saving contact info to DB */
			if (mail_id == 0 || (contact_name_len == 0))		 {	/*  save only the first address information - 09-SEP-2010 */
				/* snprintf(temp_string, sizeof(temp_string), "%c%d%c%s <%s>%c", start_text_ascii, contact_index, start_text_ascii, contact_display_name, email_address, end_text_ascii); */
				if (addr->next == NULL) {
					snprintf(temp_string, sizeof(temp_string), "\"%s\" <%s>", contact_display_name, email_address);
				}
				else {
					snprintf(temp_string, sizeof(temp_string), "\"%s\" <%s>, ", contact_display_name, email_address);
				}
				EM_DEBUG_LOG("temp_string[%s]", temp_string);

				contact_display_name_len = strlen(temp_string);
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
				EM_DEBUG_LOG("new contact_name >>>>> %s ", contact_name);
			}
		}

		if (temp_emailaddr && email_address) {	
			if (mail_id == 0) {	/*  if mail_id is 0, return only contact info without saving contact info to DB */
				/* snprintf(temp_emailaddr, 400, "%s", contact_info->email_address); */
				EM_SAFE_STRCAT(temp_emailaddr, email_address);
				if (addr->next != NULL)
					EM_SAFE_STRCAT(temp_emailaddr, ", ");
				EM_DEBUG_LOG(">>>> TEMP EMail Address [ %s ] ", temp_emailaddr);
			}
			else {	/*  save only the first address information - 09-SEP-2010 */
				if (is_saved == 0) {
					is_saved = 1;
					/* snprintf(temp_emailaddr, 400, "%s", contact_info->email_address); */
					EM_SAFE_STRCAT(temp_emailaddr, email_address);
					/*
					if (addr->next != NULL)
						EM_SAFE_STRCAT(temp_emailaddr, ", ");
					*/
					EM_DEBUG_LOG(">>>> TEMP EMail Address [ %s ] ", temp_emailaddr);
				}
			}
		}

		EM_SAFE_FREE(contact_display_name_from_contact_info);
		/*  next address */
		addr = addr->next;
	} /*  while (addr != NULL) */

	if (temp_emailaddr) {
		EM_DEBUG_LOG(">>>> TEMPEMAIL ADDR [ %s ] ", temp_emailaddr);
		contact_info->email_address = temp_emailaddr;
		temp_emailaddr = NULL;
	}

	if (contact_name != NULL) {
		contact_info->contact_name = g_strdup(contact_name);
	}
	else {
		contact_info->contact_name = g_strdup_printf("%c%d%c%s%c", start_text_ascii, 0, start_text_ascii, full_address, end_text_ascii);
		contact_info->contact_id = -1;			
	}

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
	
	if (!contact_info)  {
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

int emcore_sync_contact_info(int mail_id, int *err_code)
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

	if (!emstorage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);

		goto FINISH_OFF;
	}

	if (mail->full_address_from != NULL) {
		if (!emcore_get_mail_contact_info_with_update(&contact_info_from, mail->full_address_from, mail_id, &err))  {
			EM_DEBUG_EXCEPTION("emcore_get_mail_contact_info failed [%d]", err);
		}
	}

	if (mail->full_address_to != NULL)  {
		if (!emcore_get_mail_contact_info_with_update(&contact_info_to, mail->full_address_to, mail_id, &err))  {
			EM_DEBUG_EXCEPTION("emcore_get_mail_contact_info failed [%d]", err);
		}
	}

	if (mail->full_address_cc != NULL)  {
		if (!emcore_get_mail_contact_info_with_update(&contact_info_cc, mail->full_address_cc, mail_id, &err))  {
			EM_DEBUG_EXCEPTION("emcore_get_mail_contact_info failed [%d]", err);
		}
	}

	if (mail->full_address_bcc != NULL)  {
		if (!emcore_get_mail_contact_info_with_update(&contact_info_bcc, mail->full_address_bcc, mail_id, &err))  {
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
	}
	else if (mail->full_address_cc != NULL) {
		mail->email_address_recipient = contact_info_cc.email_address;
		contact_info_cc.contact_name = NULL;
		contact_info_cc.email_address = NULL;
	}
	else if (mail->full_address_bcc != NULL) {
		mail->email_address_recipient = contact_info_bcc.email_address;
		contact_info_bcc.contact_name  = NULL;
		contact_info_bcc.email_address = NULL;
	}
	
	/*  Update DB */
	if (!emstorage_change_mail_field(mail_id, UPDATE_ALL_CONTACT_INFO, mail, false, &err))  {				  	
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
static int emcore_sync_address_info(email_address_type_t address_type, char *full_address, GList **address_info_list, int *err_code)
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
	while (p && p[i] != '\0')  {
		if (p[i] == ';') 
			p[i] = ',';
		i++;
	}

	rfc822_parse_adrlist(&addr, p, NULL);

	EM_SAFE_FREE(p);	

	if (!addr)  {
		EM_DEBUG_EXCEPTION("rfc822_parse_adrlist failed...");
		error = EMAIL_ERROR_INVALID_PARAM;		
		goto FINISH_OFF;
	}

	/*  Get a contact name */
 	while (addr != NULL)  {
		if (addr->mailbox && addr->host) {	
			if (!strcmp(addr->mailbox , "UNEXPECTED_DATA_AFTER_ADDRESS") || !strcmp(addr->mailbox , "INVALID_ADDRESS") || !strcmp(addr->host , ".SYNTAX-ERROR.")) {
				EM_DEBUG_LOG("Invalid address ");
				addr = addr->next;
				continue;
			}
		}
		else  {
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
 
		EM_DEBUG_LOG("Search a contact  :  address[%s]", email_address);

		is_search = false;

		if (emcore_get_mail_display_name(email_address, &contact_display_name_from_contact_info, &error) && error == EMAIL_ERROR_NONE) {
			EM_DEBUG_LOG(">>> contact display name[%s]", contact_display_name_from_contact_info);

			is_search = true;
		}
		else
			EM_DEBUG_EXCEPTION("emcore_get_mail_display_name - Not found contact record(if err is -203) or error [%d]", error);

		if (is_search == true) {
			p_address_info->contact_id = contact_index;		
			p_address_info->storage_type = -1;				
			p_address_info->display_name = contact_display_name_from_contact_info;
			EM_DEBUG_LOG("display_name from contact[%s]", p_address_info->display_name);
		}
		else {
			/*  if contact doesn't exist, use alias or email address as display name */
			if (addr->personal != NULL) {
				/*  "%2C" -> ',' */
				tokens = g_strsplit(addr->personal, "%2C", -1);
				
				EM_SAFE_FREE(addr->personal);
				
				addr->personal = g_strjoinv(", ", tokens);
				
				g_strfreev(tokens);
	 			alias = addr->personal;
			}
			else {
	 			alias = NULL;
			}		
			p_address_info->contact_id = -1;
			p_address_info->storage_type = -1;	
			/*  Use an alias or an email address as a display name */
			if (alias == NULL)
				p_address_info->display_name = EM_SAFE_STRDUP(email_address);
			else
				p_address_info->display_name = EM_SAFE_STRDUP(alias);

			EM_DEBUG_LOG("display_name from email [%s]", p_address_info->display_name);
		}
		
		p_address_info->address = EM_SAFE_STRDUP(email_address);
		p_address_info->address_type = address_type;

		EM_DEBUG_LOG("email address[%s]", p_address_info->address);

		*address_info_list = g_list_append(*address_info_list, p_address_info);
		p_address_info = NULL;

		EM_DEBUG_LOG("after append");

 		alias = NULL;

		EM_DEBUG_LOG("next address[%p]", addr->next);

		/*  next address */
		addr = addr->next;
	}

	ret = true;
	
FINISH_OFF: 

 	EM_SAFE_FREE(address);
	
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

INTERNAL_FUNC GList *emcore_get_recipients_list(GList *old_recipients_list, char *full_address, int *err_code)
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
	
	if (full_address == NULL || strlen(full_address) == 0) {
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

		EM_DEBUG_LOG("Search a contact : address[%s]", email_address);

		if (emcore_get_mail_display_name(email_address, &display_name, &err) && err == EMAIL_ERROR_NONE) {
			EM_DEBUG_LOG(">>> contact display name[%s]", display_name);
			is_search = true;
		} else {
			EM_DEBUG_LOG("emcore_get_mail_display_name - Not found contact record(if err is -203) or error [%d]", err);
 		}

		if (is_search) {
			temp_recipients_list->display_name = display_name;
			EM_DEBUG_LOG("display_name from contact[%s]", temp_recipients_list->display_name);
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

			EM_DEBUG_LOG("display_name from contact[%s]", temp_recipients_list->display_name);
		}

		temp_recipients_list->address = EM_SAFE_STRDUP(email_address);
		EM_DEBUG_LOG("email address[%s]", temp_recipients_list->address);

		EM_SAFE_FREE(display_name);
		EM_DEBUG_LOG("next address[%p]", addr->next);

		recipients_list = g_list_first(new_recipients_list);
		while (recipients_list != NULL) {
			old_recipients_list_t = (email_sender_list_t *)recipients_list->data;
			if (!strcmp(old_recipients_list_t->address, temp_recipients_list->address)) {
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

INTERNAL_FUNC int emcore_get_mail_address_info_list(int mail_id, email_address_info_list_t **address_info_list, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], address_info_list[%p]", mail_id, address_info_list);

	int ret = false, err = EMAIL_ERROR_NONE;
	int failed = true;
	int contact_error;

	emstorage_mail_tbl_t *mail = NULL;
	email_address_info_list_t *p_address_info_list = NULL;

	if (mail_id <= 0 || !address_info_list) {
		EM_DEBUG_EXCEPTION("mail_id[%d], address_info_list[%p]", mail_id, address_info_list);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	/* get mail from mail table */
	if (!emstorage_get_mail_by_id(mail_id, &mail, true, &err) || !mail) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		

		goto FINISH_OFF;
	}

	if (!(p_address_info_list = (email_address_info_list_t *)malloc(sizeof(email_address_info_list_t)))) {
		EM_DEBUG_EXCEPTION("malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}	
	memset(p_address_info_list, 0x00, sizeof(email_address_info_list_t)); 	

	if ((contact_error = contacts_connect2()) == CONTACTS_ERROR_NONE)	 {		
		EM_DEBUG_LOG("Open Contact Service Success");	
	}	
	else	 {		
		EM_DEBUG_EXCEPTION("contact_db_service_connect failed [%d]", contact_error);		
		err = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	if (mail->full_address_from && emcore_sync_address_info(EMAIL_ADDRESS_TYPE_FROM, mail->full_address_from, &p_address_info_list->from, &err))
		failed = false;
	if (mail->full_address_to && emcore_sync_address_info(EMAIL_ADDRESS_TYPE_TO, mail->full_address_to, &p_address_info_list->to, &err))
		failed = false;
	if (mail->full_address_cc && emcore_sync_address_info(EMAIL_ADDRESS_TYPE_CC, mail->full_address_cc, &p_address_info_list->cc, &err))
		failed = false;
	if (mail->full_address_bcc && emcore_sync_address_info(EMAIL_ADDRESS_TYPE_BCC, mail->full_address_bcc, &p_address_info_list->bcc, &err))
		failed = false;

	if ((contact_error = contacts_disconnect2()) == CONTACTS_ERROR_NONE)
		EM_DEBUG_LOG("Close Contact Service Success");
	else
		EM_DEBUG_EXCEPTION("Close Contact Service Fail [%d]", contact_error);

	if (failed == false)
		ret = true;

FINISH_OFF: 
	if (ret == true) 
		*address_info_list = p_address_info_list;
	else if (p_address_info_list != NULL)
		emstorage_free_address_info_list(&p_address_info_list);

	if (!mail)
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
INTERNAL_FUNC int emcore_get_mail_data(int input_mail_id, email_mail_data_t **output_mail_data)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], output_mail_data[%p]", input_mail_id, output_mail_data);
	
	int             error = EMAIL_ERROR_NONE;
	int             result_mail_count = 0;
	char            conditional_clause_string[QUERY_SIZE] = { 0, };
	emstorage_mail_tbl_t *result_mail_tbl = NULL;
	
	if (input_mail_id == 0 || !output_mail_data)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	SNPRINTF(conditional_clause_string, QUERY_SIZE, "WHERE mail_id = %d", input_mail_id);
	
	if(!emstorage_query_mail_tbl(conditional_clause_string, true, &result_mail_tbl, &result_mail_count, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_tbl falied [%d]", error);
		goto FINISH_OFF;
	}

	if(!em_convert_mail_tbl_to_mail_data(result_mail_tbl, 1, output_mail_data, &error)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_tbl_to_mail_data falied [%d]", error);
		goto FINISH_OFF;
	}
	
FINISH_OFF: 
	if (result_mail_tbl)
		emstorage_free_mail(&result_mail_tbl, result_mail_count, NULL);

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}


/* internal function */
void emcore_free_body_sharep(void **p)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_SAFE_FREE(*p);
	EM_DEBUG_FUNC_END();
}

int emcore_check_drm(emstorage_attachment_tbl_t *input_attachment_tb_data)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = 0;
#ifdef __FEATURE_DRM__
	drm_bool_type_e drm_file = DRM_UNKNOWN;
	drm_file_info_s drm_file_info; 

	if (input_attachment_tb_data == NULL)
		return ret;

	ret = drm_is_drm_file(input_attachment_tb_data->attachment_path, &drm_file);

	if (ret == DRM_RETURN_SUCCESS && drm_file == DRM_TRUE) {
		if (drm_get_file_info (input_attachment_tb_data->attachment_path, &drm_file_info) == DRM_RETURN_SUCCESS) {
			input_attachment_tb_data->attachment_drm_type = 0;
			EM_DEBUG_LOG ("fileInfo is [%d]", drm_file_info.oma_info.method);
			if (drm_file_info.oma_info.method != DRM_METHOD_TYPE_UNDEFINED) {
				input_attachment_tb_data->attachment_drm_type = drm_file_info.oma_info.method;
				ret = 1;
			}
		}
	}
	else {
		EM_DEBUG_LOG("not DRM file %s", input_attachment_tb_data->attachment_path);
		input_attachment_tb_data->attachment_drm_type = 0;
		ret = 0;
	}
#endif
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
INTERNAL_FUNC int emcore_get_attachment_info(int attachment_id, email_attachment_data_t **attachment, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_id[%d], attachment[%p], err_code[%p]", attachment_id, attachment, err_code);

	if (attachment == NULL || attachment_id == 0)  {
		EM_DEBUG_EXCEPTION("attachment_id[%d], attachment[%p]", attachment_id, attachment);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	emstorage_attachment_tbl_t *attachment_tbl = NULL;
	
	/* get attachment from attachment tbl */
	if (!emstorage_get_attachment(attachment_id, &attachment_tbl, true, &err) || !attachment_tbl)  {
		EM_DEBUG_EXCEPTION("emstorage_get_attachment failed [%d]", err);

		goto FINISH_OFF;
	}
	
	*attachment = em_malloc(sizeof(email_attachment_data_t));
	if (!*attachment)  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	(*attachment)->attachment_id = attachment_id;
	(*attachment)->attachment_name = attachment_tbl->attachment_name; attachment_tbl->attachment_name = NULL;
	(*attachment)->attachment_size = attachment_tbl->attachment_size;
	(*attachment)->save_status = attachment_tbl->attachment_save_status;
	(*attachment)->attachment_path = attachment_tbl->attachment_path; attachment_tbl->attachment_path = NULL;
	(*attachment)->drm_status = attachment_tbl->attachment_drm_type;
	(*attachment)->inline_content_status = attachment_tbl->attachment_inline_content_status;

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
INTERNAL_FUNC int emcore_get_attachment_data_list(int input_mail_id, email_attachment_data_t **output_attachment_data, int *output_attachment_count)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], output_attachment_data[%p], output_attachment_count[%p]", input_mail_id, output_attachment_data, output_attachment_count);
	
	if (input_mail_id == 0|| output_attachment_data == NULL || output_attachment_count == NULL)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	int                        i = 0;
	int                        err = EMAIL_ERROR_NONE;
	int                        attachment_tbl_count = 0;
	emstorage_attachment_tbl_t *attachment_tbl_list = NULL;
	email_attachment_data_t     *temp_attachment_data = NULL;
	
	/* get attachment from attachment tbl */
	if ( (err = emstorage_get_attachment_list(input_mail_id, true, &attachment_tbl_list, &attachment_tbl_count)) != EMAIL_ERROR_NONE ){
		EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);

		goto FINISH_OFF;
	} 

	if (attachment_tbl_count)  {
		EM_DEBUG_LOG("attchment count %d", attachment_tbl_count);

		*output_attachment_data = em_malloc(sizeof(email_attachment_data_t) * attachment_tbl_count);

		if(*output_attachment_data == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		for (i = 0; i < attachment_tbl_count; i++)  {
			temp_attachment_data = (*output_attachment_data) + i;

			temp_attachment_data->attachment_id         = attachment_tbl_list[i].attachment_id;
			temp_attachment_data->attachment_name       = attachment_tbl_list[i].attachment_name; attachment_tbl_list[i].attachment_name = NULL;
			temp_attachment_data->attachment_path       = attachment_tbl_list[i].attachment_path; attachment_tbl_list[i].attachment_path = NULL;
			temp_attachment_data->attachment_size       = attachment_tbl_list[i].attachment_size;
			temp_attachment_data->mail_id               = attachment_tbl_list[i].mail_id;
			temp_attachment_data->account_id            = attachment_tbl_list[i].account_id;
			temp_attachment_data->mailbox_id            = attachment_tbl_list[i].mailbox_id;
			temp_attachment_data->save_status           = attachment_tbl_list[i].attachment_save_status;
			temp_attachment_data->drm_status            = attachment_tbl_list[i].attachment_drm_type;
			temp_attachment_data->inline_content_status = attachment_tbl_list[i].attachment_inline_content_status;
		}
	}
	
FINISH_OFF: 
	
	*output_attachment_count = attachment_tbl_count;

	if (attachment_tbl_list)
		emstorage_free_attachment(&attachment_tbl_list, attachment_tbl_count, NULL);
	
	return err;
}


INTERNAL_FUNC int emcore_download_attachment(int account_id, int mail_id, int nth, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], nth[%d], err_code[%p]", account_id, mail_id, nth, err_code);

	int err = EMAIL_ERROR_NONE;

	if (mail_id < 1 || !nth)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], nth[%d]",  mail_id, nth);
		err = EMAIL_ERROR_INVALID_PARAM;

		if (err_code != NULL)
			*err_code = err;

		emstorage_notify_network_event(NOTI_DOWNLOAD_ATTACH_FAIL, mail_id, 0, nth, err);
		return false;
	}

	int ret = false;
	int status = EMAIL_DOWNLOAD_FAIL;
	MAILSTREAM *stream = NULL;
	BODY *mbody = NULL;
	emstorage_mail_tbl_t *mail = NULL;
	emstorage_attachment_tbl_t *attachment = NULL;
	struct attachment_info *ai = NULL;
	struct _m_content_info *cnt_info = NULL;
	void *tmp_stream = NULL;
	char *s_uid = NULL, buf[1024];
	int msg_no = 0;
	emstorage_attachment_tbl_t *attachment_list = NULL;
	int current_attachment_no = 0;
	int attachment_count_to_be_downloaded = 0;		/*  how many attachments should be downloaded */
	int i = 0;
	int server_mbox_id = 0;

	if (!emcore_check_thread_status())  {
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	only_body_download = false;
	
	/*  get mail from mail table. */
	if (!emstorage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);

		goto FINISH_OFF;
	}

	if (!mail->server_mail_status)  {
		EM_DEBUG_EXCEPTION("not synchronous mail...");
		err = EMAIL_ERROR_INVALID_MAIL;
		goto FINISH_OFF;
	}
	
	if (nth == 0) {	/*  download all attachments, nth starts from 1, not zero */
		/*  get attachment list from db */
		attachment_count_to_be_downloaded = EMAIL_ATTACHMENT_MAX_COUNT;
		if ( (err = emstorage_get_attachment_list(mail_id, true, &attachment_list, &attachment_count_to_be_downloaded)) != EMAIL_ERROR_NONE ){
			EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);

			goto FINISH_OFF;
		}
	}
	else {	/*  download only nth attachment */
		attachment_count_to_be_downloaded = 1;
		if (!emstorage_get_attachment_nth(mail_id, nth, &attachment_list, true, &err) || !attachment_list)  {
			EM_DEBUG_EXCEPTION("emstorage_get_attachment_nth failed [%d]", err);

			goto FINISH_OFF;
		}
	}
	
	if (!emcore_check_thread_status())  {
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	account_id 		= mail->account_id;
	s_uid 			= EM_SAFE_STRDUP(mail->server_mail_id);
	server_mbox_id 	= mail->mailbox_id;

	/*  open mail server. */
	if (!emcore_connect_to_remote_mailbox(account_id, server_mbox_id, (void **)&tmp_stream, &err) || !tmp_stream)  {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		status = EMAIL_DOWNLOAD_CONNECTION_FAIL;
		goto FINISH_OFF;
	}
	
	stream = (MAILSTREAM *)tmp_stream;
	
	for (i = 0; i < attachment_count_to_be_downloaded; i++) {
		EM_DEBUG_LOG(" >>>>>> Attachment Downloading [%d / %d] start", i + 1, attachment_count_to_be_downloaded);

		attachment = attachment_list + i;
		if (nth == 0) 					/*  download all attachments, nth starts from 1, not zero */
			current_attachment_no = i + 1;			/*  attachment no */
		else 										/*  download only nth attachment */
			current_attachment_no = nth;	/*  attachment no */

		if (!emcore_check_thread_status())  {
			err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}
		
		if (!(cnt_info = em_malloc(sizeof(struct _m_content_info))))  {
			EM_DEBUG_EXCEPTION("malloc failed...");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		cnt_info->grab_type = GRAB_TYPE_ATTACHMENT;     /*  attachment */
		cnt_info->file_no   = current_attachment_no;    /*  attachment no */

#ifdef CHANGE_HTML_BODY_TO_ATTACHMENT
		/*  text/html be changed to attachment, this isn't real attachment in RFC822. */
		if (html_changed)
			cnt_info->file_no--;
#endif

		/*  set sparep(member of BODY) memory free function. */
		mail_parameters(stream, SET_FREEBODYSPAREP, emcore_free_body_sharep);
		
		if (!emcore_check_thread_status()) {
			err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

		msg_no = atoi(s_uid);

		/*  get body structure. */
		/*  don't free mbody because mbody is freed in closing mail_stream. */
		if ((!stream) || emcore_get_body_structure(stream, msg_no, &mbody, &err) < 0)  {
			EM_DEBUG_EXCEPTION("emcore_get_body_structure failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emcore_check_thread_status())  {
			err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

		/*  set body fetch section. */
		if (emcore_set_fetch_body_section(mbody, false, NULL,  &err) < 0) {
			EM_DEBUG_EXCEPTION("emcore_set_fetch_body_section failed [%d]", err);
			goto FINISH_OFF;
		}
	
		/*  download attachment. */
		_imap4_received_body_size = 0;
		_imap4_last_notified_body_size = 0;
		_imap4_total_body_size = 0;					/*  This will be assigned in imap_mail_write_body_to_file() */
		_imap4_download_noti_interval_value = 0;	/*  This will be assigned in imap_mail_write_body_to_file() */

		EM_DEBUG_LOG("cnt_info->file_no[%d], current_attachment_no[%d]", cnt_info->file_no, current_attachment_no);
		if (emcore_get_body(stream, account_id, mail_id, msg_no, mbody, cnt_info, &err) < 0)  {
			EM_DEBUG_EXCEPTION("emcore_get_body failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emcore_check_thread_status())  {
			err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

		/*  select target attachment information. */
		for (ai = cnt_info->file ; ai; ai = ai->next) {
			if (ai->name)
				EM_DEBUG_LOG("[in loop] %s, %d",  ai->name, cnt_info->file_no);

			if (--cnt_info->file_no == 0)
				break;
		}

		EM_DEBUG_LOG("cnt_info->file_no = %d, ai = %p", cnt_info->file_no, ai);
		
		if (cnt_info->file_no == 0 && ai) {
			/*  rename temporary file to real file. */
			if (!emstorage_create_dir(account_id, mail_id, current_attachment_no, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
				goto FINISH_OFF;
			}

			if (!emstorage_get_save_name(account_id, mail_id, current_attachment_no, ai->name, buf, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
				goto FINISH_OFF;
			}

			if (!emstorage_move_file(ai->save, buf, false, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);

				goto FINISH_OFF;
			}

			EM_SAFE_FREE(ai->save);

			EM_DEBUG_LOG("ai->size [%d]", ai->size);
			attachment->attachment_size = ai->size;
			attachment->attachment_path = EM_SAFE_STRDUP(buf);

			/*  update attachment information. */
			if (!emstorage_change_attachment_field(mail_id, UPDATE_SAVENAME, attachment, true, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_change_attachment_field failed [%d]", err);
				/*  delete created file. */
				remove(buf);

				goto FINISH_OFF;
			}

#ifdef __FEATURE_DRM__
			if (emcore_check_drm(attachment))  {	/*  has drm attachment ? */
				if (drm_process_request(DRM_REQUEST_TYPE_REGISTER_FILE, (void *)attachment->attachment_path, NULL) == DRM_RETURN_SUCCESS)
					EM_DEBUG_LOG("drm_svc_register_file success");
				else
					EM_DEBUG_EXCEPTION("drm_svc_register_file fail");
				mail->DRM_status = attachment->attachment_drm_type;
			}
#endif /* __FEATURE_DRM__ */
		}
		else  {
			EM_DEBUG_EXCEPTION("invalid attachment sequence...");
			err = EMAIL_ERROR_INVALID_ATTACHMENT;
			goto FINISH_OFF;
		}
	
		if (cnt_info)  {
			emcore_free_content_info(cnt_info);
			cnt_info = NULL;
		}
		EM_DEBUG_LOG(" >>>>>> Attachment Downloading [%d / %d] completed", i+1, attachment_count_to_be_downloaded);
	}

	if (stream)  {
		emcore_close_mailbox(0, stream);
		stream = NULL;
	}
	
	ret = true;
	
FINISH_OFF:
	if (stream)
		emcore_close_mailbox(account_id, stream);
	if (attachment_list)
		emstorage_free_attachment(&attachment_list, attachment_count_to_be_downloaded, NULL);
	if (cnt_info)
		emcore_free_content_info(cnt_info);
	if (mail)
		emstorage_free_mail(&mail, 1, NULL);

	EM_SAFE_FREE(s_uid);

	if (ret == true)
		emstorage_notify_network_event(NOTI_DOWNLOAD_ATTACH_FINISH, mail_id, NULL, nth, 0);
	else
		emstorage_notify_network_event(NOTI_DOWNLOAD_ATTACH_FAIL, mail_id, NULL, nth, err);

	if (err_code != NULL)
		*err_code = err;
	
	EM_DEBUG_FUNC_END();
	return ret;
}

#ifdef __ATTACHMENT_OPTI__
INTERNAL_FUNC int emcore_download_attachment_bulk(int account_id, int mail_id, int nth, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], nth[%d], err_code[%p]", account_id, mail_id, nth, err_code);

	int err = EMAIL_ERROR_NONE; /* Prevent Defect - 25093 */
	int ret = false;
	int status = EMAIL_DOWNLOAD_FAIL;
	MAILSTREAM *stream = NULL;
	emstorage_mail_tbl_t *mail = NULL;
	emstorage_attachment_tbl_t *attachment = NULL;
	void *tmp_stream = NULL;
	char *s_uid = NULL, *server_mbox = NULL, buf[512];
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
	
	
	memset(buf, 0x00, 512);
	/* CID FIX 31230 */
	if (mail_id < 1 || !nth) {
		EM_DEBUG_EXCEPTION("account_id[%d], mail_id[%d], nth[%p]", account_id, mail_id, nth);

		err = EMAIL_ERROR_INVALID_PARAM;

		if (err_code != NULL)
			*err_code = err;

		if (nth)
			attachment_no = atoi(nth);

		emstorage_notify_network_event(NOTI_DOWNLOAD_ATTACH_FAIL, mail_id, 0, attachment_no, err); 	/*  090525, kwangryul.baek */

		return false;
	}
	
	if (!emcore_check_thread_status()) {
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	only_body_download = false;
	
	attachment_no = atoi(nth);

	if (attachment_no == 0) {
		/*  download all attachments, nth starts from 1, not zero */
		/*  get attachment list from db */
		attachment_count_to_be_downloaded = EMAIL_ATTACHMENT_MAX_COUNT;
		if ( (err = emstorage_get_attachment_list(mail_id, true, &attachment_list, &attachment_count_to_be_downloaded)) != EMAIL_ERROR_NONE || !attachment_list){
			EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);
	
			goto FINISH_OFF;
		}
	}
	else {	/*  download only nth attachment */
		attachment_count_to_be_downloaded = 1;
		if (!emstorage_get_attachment_nth(mail_id, attachment_no, &attachment_list, true, &err) || !attachment_list) {
			EM_DEBUG_EXCEPTION("emstorage_get_attachment_nth failed [%d]", err);


			goto FINISH_OFF;
		}
	}


	if (!emcore_check_thread_status()) {
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	
	/*  get mail from mail table. */
	if (!emstorage_get_mail_by_id(mail_id, &mail, true, &err) || !mail) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		

		goto FINISH_OFF;
	}

	/* if (!mail->server_mail_yn || !mail->text_download_yn) {*/ /*  faizan.h@samsung.com */

	if (!emcore_check_thread_status()) {
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	account_id = mail->account_id;
	s_uid = EM_SAFE_STRDUP(mail->server_mail_id); mail->server_mail_id = NULL;
	server_mbox = EM_SAFE_STRDUP(mail->mailbox); mail->server_mailbox_name = NULL;



	/*  open mail server. */
	if (!emcore_connect_to_remote_mailbox(account_id, server_mbox, (void **)&tmp_stream, &err) || !tmp_stream) {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);

		status = EMAIL_DOWNLOAD_CONNECTION_FAIL;
		goto FINISH_OFF;
	}

	stream = (MAILSTREAM *)tmp_stream;


	if (!emcore_check_thread_status()) {
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}


	for (i = 0; i < attachment_count_to_be_downloaded; i++)	{
		EM_DEBUG_LOG(" >>>>>> Attachment Downloading [%d / %d] start", i+1, attachment_count_to_be_downloaded);

		attachment = attachment_list + i;
		if (attachment_no == 0)	{
			/*  download all attachments, nth starts from 1, not zero */
			current_attachment_no = i + 1;		/*  attachment no */
		}
		else {
			/*  download only nth attachment */
			current_attachment_no = attachment_no;		/*  attachment no */
		}

		if (!emcore_check_thread_status()) {
			err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

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

		if (!imap_mail_write_body_to_file(stream, account_id, mail_id, attachment_no, savefile, uid , attachment->section, attachment->encoding, &dec_len, NULL, &err)) {
			EM_DEBUG_EXCEPTION("imap_mail_write_body_to_file failed [%d]", err);
			if (err_code != NULL)
				*err_code = err;
			goto FINISH_OFF;
		}

#ifdef SUPPORT_EXTERNAL_MEMORY
		iActualSize = emcore_get_actual_mail_size (cnt_info->text.plain , cnt_info->text.html, cnt_info->file , &err);
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

		if (!emstorage_create_dir(account_id, mail_id, attachment_no, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_get_save_name(account_id, mail_id, attachment_no, attachment->name, buf, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(savefile, buf, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);

			goto FINISH_OFF;
		}

		attachment->attachment = EM_SAFE_STRDUP(buf);
		/*  update attachment information. */
		if (!emstorage_change_attachment_field(mail_id, UPDATE_SAVENAME, attachment, true, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_change_attachment_field failed [%d]", err);

			goto FINISH_OFF;
		}

#ifdef __FEATURE_DRM__
		if (emcore_check_drm(attachment)) {
			/*  is drm */
			if (drm_svc_register_file(attachment->attachment) == DRM_RESULT_SUCCESS)
				EM_DEBUG_LOG("drm_svc_register_file success");
			else
				EM_DEBUG_EXCEPTION("drm_svc_register_file fail");
			mail->flag3 = attachment->flag2;
		}
#endif /* __FEATURE_DRM__ */

		if (!emcore_check_thread_status()) {
			err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG(" >>>>>> Attachment Downloading [%d / %d] completed", i+1, attachment_count_to_be_downloaded);
	}

	ret = true;

	FINISH_OFF:

	EM_SAFE_FREE(savefile);

	emcore_close_mailbox(0, stream);
	stream = NULL;

	if (attachment_list) 
		emstorage_free_attachment(&attachment_list, attachment_count_to_be_downloaded, NULL);

	if (mail) 
		emstorage_free_mail(&mail, 1, NULL);

	if (s_uid)
		free(s_uid);

	if (server_mbox)
		free(server_mbox);server_mbox = NULL;

	if (ret == true)
		emstorage_notify_network_event(NOTI_DOWNLOAD_ATTACH_FINISH, mail_id, NULL, attachment_no, 0);
	else if (err != EMAIL_ERROR_CANCELLED)
		emstorage_notify_network_event(NOTI_DOWNLOAD_ATTACH_FAIL, mail_id, NULL, attachment_no, err);

	if (err_code != NULL)
		*err_code = err;

	return ret;
}
#endif


INTERNAL_FUNC int emcore_download_body_multi_sections_bulk(void *mail_stream, int account_id, int mail_id, int verbose, int with_attach, int limited_size, int event_handle , int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_stream[%p], account_id[%d], mail_id[%d], verbose[%d], with_attach[%d], event_handle [ %d ] ", mail_stream, account_id, mail_id, verbose, with_attach, event_handle);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int status = EMAIL_DOWNLOAD_FAIL;
	int pop3_body_size = 0;
	int pop3_downloaded_size = 0;
	MAILSTREAM *stream = NULL;
	BODY *mbody = NULL;
	PARTLIST *section_list = NULL;
	email_internal_mailbox_t mailbox = { 0 };
	emstorage_mail_tbl_t *mail = NULL;
	emstorage_attachment_tbl_t attachment = {0, 0, NULL, };
	email_account_t *ref_account = NULL;
	struct attachment_info *ai = NULL;
	struct _m_content_info *cnt_info = NULL;
	void *tmp_stream = NULL;
	char *s_uid = NULL, *server_mbox = NULL, buf[512];
	int msgno = 0, attachment_num = 1, local_attachment_count = 0, local_inline_content_count = 0;
	int iActualSize = 0;
	char html_body[MAX_PATH] = {0, };
	emcore_uid_list *uid_list = NULL;
	char *mailbox_name = NULL;
#ifdef CHANGE_HTML_BODY_TO_ATTACHMENT
	int html_changed = 0;
#endif
	int mailbox_id = 0;

	if (mail_id < 1)  {
		EM_DEBUG_EXCEPTION("mail_stream[%p], account_id[%d], mail_id[%d], verbose[%d], with_attach[%d]", mail_stream, account_id, mail_id, verbose, with_attach);
		err = EMAIL_ERROR_INVALID_PARAM;
		
		if (err_code != NULL)
			*err_code = err;

		emstorage_notify_network_event(NOTI_DOWNLOAD_BODY_FAIL, mail_id, NULL, event_handle, err);
		return false;
	}
	
	FINISH_OFF_IF_CANCELED;

	only_body_download = true;

	if (!emstorage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);

		goto FINISH_OFF;
	}
	
	if (mail->mailbox_name)
		mailbox_name = EM_SAFE_STRDUP(mail->mailbox_name);
	
	if (1 == mail->body_download_status)  {
		EM_DEBUG_EXCEPTION("not synchronous mail...");
		err = EMAIL_ERROR_INVALID_MAIL;
		goto FINISH_OFF;
	}
	
	account_id                        = mail->account_id;
	s_uid                             = mail->server_mail_id;
	server_mbox                       = mail->server_mailbox_name;
	mail->server_mail_id              = NULL;
    mail->server_mailbox_name         = NULL;

	attachment.account_id             = mail->account_id;
	attachment.mail_id                = mail->mail_id;
	attachment.mailbox_id             = mail->mailbox_id;
	attachment.attachment_save_status = 0;
	mailbox_id 						  = mail->mailbox_id;
	emstorage_free_mail(&mail, 1, NULL);
	mail = NULL;
	
	if (!(ref_account = emcore_get_account_reference(account_id)))   {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	FINISH_OFF_IF_CANCELED;

	/*  open mail server. */
	if (!mail_stream)  {
		if (!emcore_connect_to_remote_mailbox(account_id, mailbox_id, (void **)&tmp_stream, &err) || !tmp_stream)  {
			EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
			status = EMAIL_DOWNLOAD_CONNECTION_FAIL;
			goto FINISH_OFF;
		}
		stream = (MAILSTREAM *)tmp_stream;
	}
	else
		stream = (MAILSTREAM *)mail_stream;
	
	free(server_mbox);
	server_mbox = NULL;
	
	FINISH_OFF_IF_CANCELED;
	
	if (!(cnt_info = em_malloc(sizeof(struct _m_content_info))))  {
		EM_DEBUG_EXCEPTION("em_malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_POP3)  {
		/*  POP3 */
		/*  in POP3 case, both text and attachment are downloaded in this call. */
		cnt_info->grab_type = GRAB_TYPE_TEXT | GRAB_TYPE_ATTACHMENT;
		attachment.attachment_save_status = 1;		/*  all attachments should be downloaded in the case of POP3 */

		mailbox.account_id = account_id;
		mailbox.mail_stream = stream;

		/*  download all uids from server. */
		if (!emcore_download_uid_all(&mailbox, &uid_list, NULL, NULL, 0, EM_CORE_GET_UIDS_FOR_NO_DELETE, &err))  {
			EM_DEBUG_EXCEPTION("emcore_download_uid_all failed [%d]", err);
			goto FINISH_OFF;
		}

		/*  get mesg number to be related to last download mail from uid list file */
		if (!emcore_get_msgno(uid_list, s_uid, &msgno, &err))  {
			EM_DEBUG_EXCEPTION("emcore_get_msgno failed [%d]", err);
			err = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER;
			goto FINISH_OFF;
		}

		free(s_uid); s_uid = NULL;

		if (!emcore_check_thread_status())  {
			err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

		_pop3_received_body_size = 0;
		_pop3_total_body_size = 0;
		_pop3_last_notified_body_size = 0;
		_pop3_receiving_mail_id = mail_id;
		
		/*  send read mail commnad. */
		if (!emcore_mail_cmd_read_mail_pop3(stream, msgno, limited_size, &pop3_downloaded_size, &pop3_body_size, &err)) {
			EM_DEBUG_EXCEPTION("emcore_mail_cmd_read_mail_pop3 failed [%d]", err);
			goto FINISH_OFF;
		}

		_pop3_total_body_size = pop3_body_size;

		if (!emstorage_notify_network_event(NOTI_DOWNLOAD_BODY_START, mail_id, "dummy-file", _pop3_total_body_size, 0))
			EM_DEBUG_EXCEPTION(" emstorage_notify_network_event [ NOTI_DOWNLOAD_BODY_START] failed >>>> ");
		else
			EM_DEBUG_LOG("NOTI_DOWNLOAD_BODY_START notified (%d / %d)", 0, _pop3_total_body_size);

		FINISH_OFF_IF_CANCELED;
		
		/*  save message into tempfile */
		/*  parsing mime from stream. */

		if (!emcore_parse_mime(stream, 0, cnt_info,  &err))  {
			EM_DEBUG_EXCEPTION("emcore_parse_mime failed [%d]", err);
			goto FINISH_OFF;
		}
		
		FINISH_OFF_IF_CANCELED;
	}
	else  {	/*  in IMAP case, both text and attachment list are downloaded in this call. */
		/*  This flag is just for downloading mailbox.(sync header), don't be used when retrieve body. */
		if (with_attach > 0)
			cnt_info->grab_type = GRAB_TYPE_TEXT | GRAB_TYPE_ATTACHMENT;
		else
			cnt_info->grab_type = GRAB_TYPE_TEXT;

		int uid = atoi(s_uid);

		free(s_uid); s_uid = NULL;

		/*  set sparep(member of BODY) memory free function  */
		mail_parameters(stream, SET_FREEBODYSPAREP, emcore_free_body_sharep);

		/*  get body strucutre. */
		/*  don't free mbody because mbody is freed in closing mail_stream. */
		if (emcore_get_body_structure(stream, uid, &mbody, &err) < 0 || (mbody == NULL))  {
			EM_DEBUG_EXCEPTION("emcore_get_body_structure failed [%d]", err);
			err = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER;
			goto FINISH_OFF;
		}

		FINISH_OFF_IF_CANCELED;

		if (mbody->type == TYPEMULTIPART) {
			EM_DEBUG_LOG(">>> check multipart body size to download : only_body_download[%d]", only_body_download);
			PART *part_child = mbody->nested.part;
			int counter = 0;

			char filename[MAX_PATH+1] = {0, };
			int is_attachment = 0;
			while (part_child)  {
				BODY *body = &(part_child->body);
				if (only_body_download == true) {
					if (((body->id) && strlen(body->id) > 1) || (body->location))
						is_attachment = 0;
					else if (body->disposition.type)  {	/*  "attachment" or "inline" or etc... */
						PARAMETER *param = body->disposition.parameter;

						while (param)  {
							EM_DEBUG_LOG("param->attribute [%s], param->value [%s]", param->attribute, param->value);

							if (!strcasecmp(param->attribute, "filename"))  {	/* attribute is "filename" */
								strncpy(filename, param->value, MAX_PATH);
								EM_DEBUG_LOG(">>>>> FILENAME [%s] ", filename);
								break;
							}
							param = param->next;
						}

						is_attachment = 1;

						if (!*filename)  {
							/*  it may be report msg */
							if (body->disposition.type[0] == 'i' || body->disposition.type[0] == 'I')
								is_attachment = 0;
						}
					}

					if (is_attachment == 0) {
						EM_DEBUG_LOG("%d :  body->size.bytes[%ld]", counter+1, body->size.bytes);
						multi_part_body_size = multi_part_body_size + body->size.bytes;
					}
				}
				else {
					/*  download all */
				        EM_DEBUG_LOG("%d :  body->size.bytes[%ld]", counter+1, body->size.bytes);
			        	multi_part_body_size = multi_part_body_size + body->size.bytes;
				}
				part_child = part_child->next;
				counter++;
			}
		}

		/*  set body fetch section. */
		if (emcore_set_fetch_body_section(mbody, true, &iActualSize, &err) < 0) {
			EM_DEBUG_EXCEPTION("emcore_set_fetch_body_section failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("iActualSize [%d]", iActualSize);
		multi_part_body_size = iActualSize;

		_imap4_received_body_size = 0;
		_imap4_last_notified_body_size = 0;
		if (multi_part_body_size > 0) {	/*  download multiparts */
			_imap4_total_body_size = multi_part_body_size;
			_imap4_download_noti_interval_value = DOWNLOAD_NOTI_INTERVAL_PERCENT * multi_part_body_size / 100;
		}
		else {	/*  download only one body part */
			_imap4_total_body_size = 0;					/*  This will be assigned in imap_mail_write_body_to_file() */
			_imap4_download_noti_interval_value = 0;	/*  This will be assigned in imap_mail_write_body_to_file() */
		}

		/*  save message into tempfile */
		/*  download body text and get attachment list. */
		if (emcore_get_body_part_list_full(stream, uid, account_id, mail_id, mbody, cnt_info, &err, section_list, event_handle) < 0)  {
			EM_DEBUG_EXCEPTION("emcore_get_body falied [%d]", err);
			goto FINISH_OFF;
		}
		FINISH_OFF_IF_CANCELED;
	}


	if (false == emstorage_get_mail_by_id(mail_id, &mail, true, &err)) {
		EM_DEBUG_EXCEPTION(" emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (cnt_info->text.plain)  {
		EM_DEBUG_LOG("cnt_info->text.plain [%s]", cnt_info->text.plain);

		if (!emstorage_create_dir(account_id, mail_id, 0, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}


		if (!emstorage_get_save_name(account_id, mail_id, 0, cnt_info->text.plain_charset ? cnt_info->text.plain_charset  :  "UTF-8", buf, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(cnt_info->text.plain, buf, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}

		mail->file_path_plain = EM_SAFE_STRDUP(buf);
		EM_DEBUG_LOG("mail->file_path_plain [%s]", mail->file_path_plain);
	}
	
	if (cnt_info->text.html)  {
		if (!emstorage_create_dir(account_id, mail_id, 0, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}
		
		if (cnt_info->text.plain_charset != NULL) {
			memcpy(html_body, cnt_info->text.plain_charset, strlen(cnt_info->text.plain_charset));
			strcat(html_body, HTML_EXTENSION_STRING);
		}
		else {
			memcpy(html_body, "UTF-8.htm", strlen("UTF-8.htm"));
		}
		if (!emstorage_get_save_name(account_id, mail_id, 0, html_body, buf, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_move_file(cnt_info->text.html, buf, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}
		mail->file_path_html = EM_SAFE_STRDUP(buf);
	}
	
	if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_POP3 && limited_size != NO_LIMITATION && limited_size < pop3_body_size)
 		mail->body_download_status = EMAIL_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED;
	else
		mail->body_download_status = EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;
	
#ifdef CHANGE_HTML_BODY_TO_ATTACHMENT
	if (html_changed) mail->flag2 = 1;
#endif
	
	FINISH_OFF_IF_CANCELED;
	
	for (ai = cnt_info->file; ai; ai = ai->next, attachment_num++)  {
		attachment.attachment_id                    = attachment_num;
		attachment.attachment_size                  = ai->size;
		attachment.attachment_path                  = ai->save;
		attachment.attachment_name                  = ai->name;
		attachment.attachment_drm_type              = ai->drm;
		attachment.attachment_inline_content_status = ai->type == 1;
		attachment.attachment_save_status           = 0;
		attachment.attachment_mime_type             = ai->attachment_mime_type;
#ifdef __ATTACHMENT_OPTI__
		attachment.encoding                         = ai->encoding;
		attachment.section                          = ai->section;
#endif
		EM_DEBUG_LOG("attachment.attachment_id[%d]", attachment.attachment_id);
		EM_DEBUG_LOG("attachment.attachment_size[%d]", attachment.attachment_size);
		EM_DEBUG_LOG("attachment.attachment_path[%s]", attachment.attachment_path);
		EM_DEBUG_LOG("attachment.attachment_name[%s]", attachment.attachment_name);
		EM_DEBUG_LOG("attachment.attachment_drm_type[%d]", attachment.attachment_drm_type);
		EM_DEBUG_LOG("attachment.attachment_inline_content_status[%d]", attachment.attachment_inline_content_status);

		if (ai->type == 1)
			local_inline_content_count++;
		local_attachment_count++;

		if (ai->save)  {
			/*  in POP3 case, rename temporary file to real file. */
			attachment.attachment_save_status = 1;
			if (ai->type == 1)  {		/*  it is inline content */
				if (!emstorage_create_dir(account_id, mail_id, 0, &err))  {
					EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
					goto FINISH_OFF;
				}
				if (!emstorage_get_save_name(account_id, mail_id, 0, attachment.attachment_name, buf, &err))  {
					EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
					goto FINISH_OFF;
				}
			}
			else  {
				if (!emstorage_create_dir(account_id, mail_id, attachment_num, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
					goto FINISH_OFF;
				}

				if (!emstorage_get_save_name(account_id, mail_id, attachment_num, attachment.attachment_name, buf, &err))  {
					EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
					goto FINISH_OFF;
				}
			}

			if (!emstorage_move_file(ai->save, buf, false, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);

				/*  delete all created files. */
				if (!emstorage_get_save_name(account_id, mail_id, 0, NULL, buf, NULL)) {
					EM_DEBUG_EXCEPTION("emstorage_get_save_name failed...");
					/* goto FINISH_OFF; */
				}

				if (!emstorage_delete_dir(buf, NULL)) {
					EM_DEBUG_EXCEPTION("emstorage_delete_dir failed...");
					/* goto FINISH_OFF; */
				}


				goto FINISH_OFF;
			}

			free(ai->save); ai->save = EM_SAFE_STRDUP(buf);

			attachment.attachment_path = ai->save;

#ifdef __FEATURE_DRM__
			if (emcore_check_drm(&attachment)) /*  is drm content ?*/ {
				if (drm_process_request(DRM_REQUEST_TYPE_REGISTER_FILE, attachment.attachment_path, NULL) != DRM_RETURN_SUCCESS)
					EM_DEBUG_EXCEPTION("drm_process_request : register file fail");
				mail->DRM_status = attachment.attachment_drm_type;
			}
#endif/*  __FEATURE_DRM__ */
		}

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
		/*  Information :  Attachment info already saved if partial body is dowloaded. */
		if (ai->type)  {
			emstorage_attachment_tbl_t *attch_info = NULL;
			/*  Get attachment details  */
			if (!emstorage_get_attachment_nth(mail_id, attachment.attachment_id, &attch_info, true, &err) || !attch_info)  {
				EM_DEBUG_EXCEPTION("emstorage_get_attachment_nth failed [%d]", err);
				if (err == EMAIL_ERROR_ATTACHMENT_NOT_FOUND) {   /*  save only attachment file. */
					if (!emstorage_add_attachment(&attachment, 0, false, &err)) {
						EM_DEBUG_EXCEPTION("emstorage_add_attachment failed [%d]", err);
						if (attch_info)
							emstorage_free_attachment(&attch_info, 1, NULL);
						/*  delete all created files. */
						if (!emstorage_get_save_name(account_id, mail_id, 0, NULL, buf, &err))  {
							EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
							goto FINISH_OFF;
						}

						if (!emstorage_delete_dir(buf, &err))  {
							EM_DEBUG_EXCEPTION("emstorage_delete_dir failed [%d]", err);
							goto FINISH_OFF;
						}

						/*  ROLLBACK TRANSACTION; */
						emstorage_rollback_transaction(NULL, NULL, NULL);


						goto FINISH_OFF;
					}
				}
			}
			else {
				EM_DEBUG_LOG("Attachment info already exists...!");
				/* Update attachment size */
				EM_DEBUG_LOG("attachment_size [%d], ai->size [%d]", attch_info->attachment_size, ai->size);
				attch_info->attachment_size = ai->size;
				if (!emstorage_update_attachment(attch_info, true, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_update_attachment failed [%d]", err);

						goto FINISH_OFF;
				}
			}

			if (attch_info)
				emstorage_free_attachment(&attch_info, 1, NULL);
		}

#else
		
		if (ai->type)  {
			mail->attachment_yn = 1;
			/*  save only attachment file. */
			if (!emstorage_add_attachment(&attachment, 0, false, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_add_attachment failed [%d]", err);
				if (bIsAdd_to_mmc)  {
					if (attachment.attachment) {
					}
				}
				else  {
					/*  delete all created files. */
					if (!emstorage_get_save_name(account_id, mail_id, 0, NULL, buf, &err))  {
						EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
						goto FINISH_OFF;
					}

					if (!emstorage_delete_dir(buf, &err))  {
						EM_DEBUG_EXCEPTION("emstorage_delete_dir failed [%d]", err);
						goto FINISH_OFF;
					}

					/*  ROLLBACK TRANSACTION; */
					emstorage_rollback_transaction(NULL, NULL, NULL);
					goto FINISH_OFF;
				}
			}
	    }
#endif	/*  End of #ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__ */

	}

	EM_DEBUG_LOG("Check #1");

	mail->attachment_count = local_attachment_count;
	mail->inline_content_count = local_inline_content_count;

	EM_DEBUG_LOG("Check #2");

	EM_DEBUG_LOG("Mailbox Name [%s]", mailbox_name);
	mail->mailbox_name = EM_SAFE_STRDUP(mailbox_name);	 /*  fix for mailboox sync fail */

	EM_DEBUG_LOG("Check #3");

	/*  change mail's information. */
	if (!emstorage_change_mail_field(mail_id, APPEND_BODY, mail, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);
		emstorage_rollback_transaction(NULL, NULL, NULL); /*  ROLLBACK TRANSACTION; */

		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("cnt_info->text.plain [%s], cnt_info->text.html [%s]", cnt_info->text.plain, cnt_info->text.html);
	
	/*  in pop3 mail case, the mail is deleted from server after being downloaded. */
	if (ref_account->incoming_server_type == EMAIL_SERVER_TYPE_POP3)  {
#ifdef DELETE_AFTER_DOWNLOADING
		char delmsg[24];

		SNPRINTF(delmsg, sizeof(delmsg), "%d", msg_no);

		if (!ref_account->keep_mails_on_pop_server_after_download)  {
			if (!emcore_delete_mails_from_pop3_server(&mbox, delmsg, &err))
				EM_DEBUG_EXCEPTION("emcore_delete_mails_from_pop3_server failed [%d]", err);
		}
#endif
		
		if (!mail_stream)  {
			if (stream != NULL)  {
				emcore_close_mailbox(0, stream);
				stream = NULL;
			}
		}
	}

	FINISH_OFF_IF_CANCELED;
	
	ret = true;

FINISH_OFF:

	if (g_inline_count) {
		g_inline_count = 0;
		EM_SAFE_FREE(g_inline_list);
	}

	multi_part_body_size = 0;
	_pop3_received_body_size = 0;
	_pop3_last_notified_body_size = 0;
	_pop3_total_body_size = 0;
	_pop3_receiving_mail_id = 0;
	
	_imap4_received_body_size = 0;
	_imap4_last_notified_body_size = 0;
	_imap4_total_body_size = 0;
	_imap4_download_noti_interval_value = 0;
	
	if (cnt_info)
		emcore_free_content_info(cnt_info);
	if (mail)
		emstorage_free_mail(&mail, 1, NULL);
	EM_SAFE_FREE(server_mbox);
	EM_SAFE_FREE(s_uid);
	EM_SAFE_FREE(mailbox_name);

	multi_part_body_size = 0;
	
	if (ret == true)
		emstorage_notify_network_event(NOTI_DOWNLOAD_BODY_FINISH, mail_id, NULL, event_handle, 0);
	else
		emstorage_notify_network_event(NOTI_DOWNLOAD_BODY_FAIL, mail_id, NULL, event_handle, err);

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

	EM_DEBUG_LOG("mailbox name - %s", mailbox);
	EM_DEBUG_LOG("first sequence number source- %ld", sourceset->first);
	EM_DEBUG_LOG("last sequence number last- %ld", sourceset->last);
	EM_DEBUG_LOG("first sequence number dest - %ld", destset->first);
	EM_DEBUG_LOG("last sequence number dest- %ld", sourceset->last);

	/* search for server _mail_id with value sourceset->first and update it with destset->first */
	/* faizan.h@samsung.com */
	memset(old_server_uid, 0x00, 129);
	sprintf(old_server_uid, "%ld", sourceset->first);
	EM_DEBUG_LOG(">>>>> old_server_uid = %s", old_server_uid);

	memset(g_new_server_uid, 0x00, 129);
	sprintf(g_new_server_uid, "%ld", destset->first);
	EM_DEBUG_LOG(">>>>> new_server_uid =%s", g_new_server_uid);

	if (!emstorage_update_server_uid(old_server_uid, g_new_server_uid, NULL)) {
		EM_DEBUG_EXCEPTION("emstorage_update_server_uid falied...");
	}
}

static int emcore_delete_mails_from_remote_server(int input_account_id, int input_mail_ids[], int input_mail_id_count, int input_delete_option)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id[%d], input_mail_ids[%p], input_mail_id_count[%d], input_delete_option [%d]", input_account_id, input_mail_ids, input_mail_id_count, input_delete_option);

	int err = EMAIL_ERROR_NONE;
	email_account_t        *account = NULL;
#ifdef 	__FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__
	int bulk_flag = false;
#endif

	if (!(account = emcore_get_account_reference(input_account_id)))  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", input_account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!emnetwork_check_network_status(&err)) {
		EM_DEBUG_EXCEPTION("emnetwork_check_network_status failed [%d]", err);
		goto FINISH_OFF;

	}

	FINISH_OFF_IF_CANCELED;

	if (account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
		if (!bulk_flag && !emcore_delete_mails_from_imap4_server(input_mail_ids, input_mail_id_count, input_delete_option, &err)) {
			EM_DEBUG_EXCEPTION("emcore_delete_mails_from_imap4_server failed [%d]", err);
			if (err == EMAIL_ERROR_IMAP4_STORE_FAILURE)
				err = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER;
			goto FINISH_OFF;
		}
		else
			bulk_flag = true;
	}
	else if (account->incoming_server_type == EMAIL_SERVER_TYPE_POP3) {
		if (!emcore_delete_mails_from_pop3_server(account, input_mail_ids, input_mail_id_count))  {
			EM_DEBUG_EXCEPTION("emcore_delete_mails_from_pop3_server falied [%d]", err);
			goto FINISH_OFF;
		}
#ifdef __FEATURE_LOCAL_ACTIVITY__
		else {
			/* Remove local activity */
			emstorage_activity_tbl_t new_activity;
			memset(&new_activity, 0x00, sizeof(emstorage_activity_tbl_t));
			if (from_server == EMAIL_DELETE_FOR_SEND_THREAD) {
				new_activity.activity_type = ACTIVITY_DELETEMAIL_SEND;
				EM_DEBUG_LOG("from_server == EMAIL_DELETE_FOR_SEND_THREAD ");
			}
			else {
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
			if (!emstorage_remove_downloaded_mail(input_account_id, mail->server_mailbox_name, mail->server_mail_id, true, &err))  {
				EM_DEBUG_LOG("emstorage_remove_downloaded_mail falied [%d]", err);
			}
		}

#endif	/*  __FEATURE_LOCAL_ACTIVITY__ */
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

int emcore_delete_mail(int account_id, int mail_ids[], int num, int from_server, int noti_param_1, int noti_param_2, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_ids[%p], num[%d], from_server[%d], noti_param_1 [%d], noti_param_2 [%d], err_code[%p]", account_id, mail_ids, num, from_server, noti_param_1, noti_param_2, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_account_t *account = NULL;

	if (!account_id || !mail_ids || !num)  {
		EM_DEBUG_EXCEPTION("account_id[%d], mail_ids[%p], num[%d], from_server[%d]", account_id, mail_ids, num, from_server);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(account = emcore_get_account_reference(account_id)))  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	FINISH_OFF_IF_CANCELED;

	if (from_server == EMAIL_DELETE_LOCALLY) /* Delete mails from local storage*/ {
		emcore_delete_mails_from_local_storage(account_id, mail_ids, num, noti_param_1, noti_param_2, err_code);
		emcore_check_unread_mail();
	}
	else {   /* Delete mails from server*/
		emcore_delete_mails_from_remote_server(account_id, mail_ids, num, from_server);
	}


	ret = true;

FINISH_OFF: 
	if (from_server)
		emcore_show_user_message(account_id, EMAIL_ACTION_DELETE_MAIL, ret == true ? 0  :  err);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("err [%d]", err);

	return ret;
}

int emcore_delete_all_mails_of_acount(int input_account_id)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d]");

	int   err = EMAIL_ERROR_NONE;
	char  buf[512] = { 0, };

	/* emstorage_delete_mail_by_account is available only locally */
	if (!emstorage_delete_mail_by_account(input_account_id, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_delete_mail_by_account failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emstorage_delete_attachment_all_on_db(input_account_id, NULL, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_delete_attachment_all_on_db failed [%d]", err);
		goto FINISH_OFF;
	}

	/*  delete mail contents from filesystem */
	if (!emstorage_get_save_name(input_account_id, 0, 0, NULL, buf, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emstorage_delete_dir(buf, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_delete_dir failed [%d]", err);
	}

	/*  delete meeting request */
	if (!emstorage_delete_meeting_request(input_account_id, 0, 0, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_delete_attachment_all_on_db failed [%d]", err);
		goto FINISH_OFF;
	}

	emcore_check_unread_mail();

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]",err);
	return err;
}

int emcore_delete_all_mails_of_mailbox(int input_mailbox_id, int input_from_server, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id [%d], input_from_server [%d], err_code [%p]", input_mailbox_id, input_from_server, err_code);

	int   ret = false;
	int   err = EMAIL_ERROR_NONE;
	int   search_handle = 0;
	int  *mail_ids = NULL;
	int   i = 0;
	int   total = 0;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;

	if (!input_mailbox_id) {
		err = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_mailbox_by_id(input_mailbox_id, &mailbox_tbl) != EMAIL_ERROR_NONE)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err);
		goto FINISH_OFF;
	}
	/* Delete all mails in specific mailbox */
	if (!emstorage_mail_search_start(NULL, mailbox_tbl->account_id, mailbox_tbl->mailbox_name, 0, &search_handle, &total, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_mail_search_start failed [%d]", err);

		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("emstorage_mail_search_start returns [%d]", total);

	if (total > 0) {
		mail_ids = em_malloc(sizeof(int) * total);
		if (mail_ids == NULL)  {
			EM_DEBUG_EXCEPTION("em_malloc failed...");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		for (i = 0; i < total; i++)  {
			if (!emstorage_mail_search_result(search_handle, RETRIEVE_ID, (void**)&mail_ids[i], true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_mail_search_result failed [%d]", err);

				goto FINISH_OFF;
			}
		}

		if (!emcore_delete_mail(mailbox_tbl->account_id, mail_ids, total, input_from_server, EMAIL_DELETED_BY_COMMAND, false, &err)) {
			EM_DEBUG_EXCEPTION("emcore_delete_mail failed [%d]", err);
			goto FINISH_OFF;
		}
	}
	
	ret = true;

FINISH_OFF:
	if (search_handle >= 0)  {
		if (!emstorage_mail_search_end(search_handle, true, &err))
			EM_DEBUG_EXCEPTION("emstorage_mail_search_end failed [%d]", err);
	}
	
	if (mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

	EM_SAFE_FREE(mail_ids);
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d], err [%d]", ret, err);
	return ret;
}

INTERNAL_FUNC int emcore_delete_mails_from_local_storage(int account_id, int *mail_ids, int num, int noti_param_1, int noti_param_2, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_ids[%p], num [%d], noti_param_1 [%d], noti_param_2 [%d], err_code[%p]", account_id, mail_ids, num, noti_param_1, noti_param_2, num, err_code);
	int ret = false, err = EMAIL_ERROR_NONE, i;
	emstorage_mail_tbl_t *result_mail_list = NULL;
	char mail_id_string[10], *noti_param_string = NULL, buf[512] = {0, };

	/* Getting mail list by using select mail_id [in] */
	if(!emstorage_get_mail_field_by_multiple_mail_id(mail_ids, num, RETRIEVE_SUMMARY, &result_mail_list, true, &err) || !result_mail_list) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_field_by_multiple_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Deleting mails by using select mail_id [in] */
	if(!emstorage_delete_multiple_mails(mail_ids, num, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_delete_multiple_mails failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Sending Notification */
	noti_param_string = em_malloc(sizeof(char) * 10 * num);

	if(!noti_param_string) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for(i = 0; i < num; i++) {
		memset(mail_id_string, 0, 10);
		SNPRINTF(mail_id_string, 10, "%d,", mail_ids[i]);
		strcat(noti_param_string, mail_id_string);
		/* can be optimized by appending sub string with directly pointing on string array kyuho.jo 2011-10-07 */
	}

	if (!emstorage_notify_storage_event(NOTI_MAIL_DELETE, account_id, noti_param_1, noti_param_string, noti_param_2))
		EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event failed [ NOTI_MAIL_DELETE_FINISH ] >>>> ");

	/* Updating Thread informations */
	/* Thread information should be updated as soon as possible. */
	for(i = 0; i < num; i++) {
		if (result_mail_list[i].thread_item_count > 1)	{
			if (!emstorage_update_latest_thread_mail(account_id, result_mail_list[i].thread_id, 0, 0, false,  &err)) {
				EM_DEBUG_EXCEPTION("emstorage_update_latest_thread_mail failed [%d]", err);
		
				goto FINISH_OFF;
			}
		}
	}
	if (!emstorage_notify_storage_event(NOTI_MAIL_DELETE_FINISH, account_id, noti_param_1, noti_param_string, noti_param_2))
		EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event failed [ NOTI_MAIL_DELETE_FINISH ] >>>> ");

	for(i = 0; i < num; i++) {
		/* Deleting attachments */
		if (!emstorage_delete_all_attachments_of_mail(result_mail_list[i].mail_id, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_delete_all_attachments_of_mail failed [%d]", err);
			if (err == EMAIL_ERROR_ATTACHMENT_NOT_FOUND)
				err = EMAIL_ERROR_NONE;
		}

		/* Deleting Directories */
		/*  delete mail contents from filesystem */
		if (!emstorage_get_save_name(account_id, result_mail_list[i].mail_id, 0, NULL, buf, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);

			goto FINISH_OFF;
		}

		if (!emstorage_delete_dir(buf, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_delete_dir failed [%d]", err);

		}
		
		/* Deleting Meeting Request */
		if (!emstorage_delete_meeting_request(account_id, result_mail_list[i].mail_id, 0, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_delete_meeting_request failed [%d]", err);
			if (err != EMAIL_ERROR_CONTACT_NOT_FOUND) {
				goto FINISH_OFF;
			}
		}
	}

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE(noti_param_string);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

static int emcore_delete_mails_from_pop3_server(email_account_t *input_account, int input_mail_ids[], int input_mail_id_count)
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

	for (i = 0; i < input_mail_id_count; i++)  {
		FINISH_OFF_IF_CANCELED;

		mail_id = input_mail_ids[i];

		if (!emstorage_get_downloaded_mail(mail_id, &mail_tbl_data, false, &err) || !mail_tbl_data)  {
			EM_DEBUG_EXCEPTION("emstorage_get_downloaded_mail failed [%d]", err);

			if (err == EMAIL_ERROR_MAIL_NOT_FOUND) {	/* not server mail */
				continue;
			}
			else
				goto FINISH_OFF;
		}

		if (stream == NULL)  {
			if (!emcore_connect_to_remote_mailbox(input_account->account_id, 0, (void **)&stream, &err))  {
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

		if (!emcore_get_mail_msgno_by_uid(input_account, &mailbox_data, mail_tbl_data->server_mail_id, &msgno, &err))  {
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

		if (!emstorage_remove_downloaded_mail(input_account->account_id, mail_tbl_data->server_mailbox_name, mail_tbl_data->server_mail_id, true, &err))
			EM_DEBUG_LOG("emstorage_remove_downloaded_mail falied [%d]", err);

NOT_FOUND_ON_SERVER :

		if (mail_tbl_data != NULL)
			emstorage_free_mail(&mail_tbl_data, 1, NULL);
	}

FINISH_OFF: 

	if (mail_tbl_data != NULL)
		emstorage_free_mail(&mail_tbl_data, 1, NULL);

	if (stream) 		 {
		emcore_close_mailbox(0, stream);
		stream = NULL;
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
	EM_DEBUG_FUNC_BEGIN("account[%p], mailbox[%p], uid[%s], msgno[%p], err_code[%p]", account, mailbox, uid, msgno, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	
	emcore_uid_list *uid_list = NULL;
	emcore_uid_list *pTemp_uid_list = NULL;
	
	if (!account || !mailbox || !uid || !msgno)  {
		EM_DEBUG_EXCEPTION("account[%p], mailbox[%p], uid[%s], msgno[%p]", account, mailbox, uid, msgno);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	uid_list = mailbox->user_data;
	
	if (uid_list == NULL)  {
		if (account->incoming_server_type == EMAIL_SERVER_TYPE_POP3)  {
			if (!pop3_mailbox_get_uids(mailbox->mail_stream, &uid_list, &err))  {
				EM_DEBUG_EXCEPTION("pop3_mailbox_get_uids failed [%d]", err);
				goto FINISH_OFF;
			}
		}
		else  {		/*  EMAIL_SERVER_TYPE_IMAP4 */
			if (!imap4_mailbox_get_uids(mailbox->mail_stream, &uid_list, &err))  {
				EM_DEBUG_EXCEPTION("imap4_mailbox_get_uids failed [%d]", err);
				goto FINISH_OFF;
			}
		}
		mailbox->user_data = uid_list;
	}
	pTemp_uid_list = uid_list;
	while (uid_list)  {
		if (!strcmp(uid_list->uid, uid))  {
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
	uid_list = pTemp_uid_list ;
	if (uid_list != NULL)
		emcore_free_uids(uid_list, NULL);
	/*  mailbox->user_data and uid_list both point to same memory address, So when uid_list  is freed then just set  */
	/*  mailbox->user_data to NULL and dont use EM_SAFE_FREE, it will crash  : ) */
	if (mailbox)
	mailbox->user_data = NULL;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_expunge_mails_deleted_flagged_from_local_storage(int input_mailbox_id)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d]", input_mailbox_id);
	int   err = EMAIL_ERROR_NONE;
	char *conditional_clause_string = NULL;
	email_list_filter_t *filter_list = NULL;
	int  filter_count = 0;
	int *result_mail_id_list = NULL;
	int  result_count = 0;
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;

	if ( input_mailbox_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err =  EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_mailbox_by_id(input_mailbox_id, &mailbox_tbl)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	filter_count = 3;

	filter_list = em_malloc(sizeof(email_list_filter_t) * filter_count);

	if (filter_list == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
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

	if ( (err = emstorage_write_conditional_clause_for_getting_mail_list(filter_list, filter_count, NULL, 0, -1, -1, &conditional_clause_string)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_write_conditional_clause_for_getting_mail_list failed[%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("conditional_clause_string[%s].", conditional_clause_string);

	if ((err = emstorage_query_mail_id_list(conditional_clause_string, true, &result_mail_id_list, &result_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_id_list [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_delete_mails_from_local_storage(mailbox_tbl->account_id, result_mail_id_list, result_count, 1, EMAIL_DELETED_BY_COMMAND, &err)) {
		EM_DEBUG_EXCEPTION("emcore_delete_mails_from_local_storage [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	if(mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

	if(filter_list)
		emstorage_free_list_filter(&filter_list, filter_count);

	EM_SAFE_FREE(conditional_clause_string);
	EM_SAFE_FREE(result_mail_id_list);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_expunge_mails_deleted_flagged_from_remote_server(int input_account_id, int input_mailbox_id)
{
	int   err = EMAIL_ERROR_NONE;
	char *conditional_clause_string = NULL;
	email_list_filter_t *filter_list = NULL;
	int  filter_count = 0;
	int *result_mail_id_list = NULL;
	int  result_count = 0;

	if ( input_mailbox_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err =  EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	filter_count = 3;

	filter_list = em_malloc(sizeof(email_list_filter_t) * filter_count);

	if (filter_list == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
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

	if ( (err = emstorage_write_conditional_clause_for_getting_mail_list(filter_list, filter_count, NULL, 0, -1, -1, &conditional_clause_string)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_write_conditional_clause_for_getting_mail_list failed[%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("conditional_clause_string[%s].", conditional_clause_string);

	if ((err = emstorage_query_mail_id_list(conditional_clause_string, true, &result_mail_id_list, &result_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_id_list [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_delete_mail(input_account_id, result_mail_id_list, result_count, EMAIL_DELETE_FROM_SERVER, 1, EMAIL_DELETED_BY_COMMAND, &err)) {
		EM_DEBUG_EXCEPTION("emcore_delete_mail [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	if(filter_list)
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
INTERNAL_FUNC int emcore_mail_add_attachment(int mail_id, email_attachment_data_t *attachment, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], attachment[%p], err_code[%p]", mail_id, attachment, err_code);
	
	if (attachment == NULL)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], attachment[%p]", mail_id, attachment);
		if (err_code)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false, err = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t *mail_table_data = NULL;
	int attachment_id = 0;
	

	
	if (!emstorage_get_mail_field_by_id(mail_id, RETRIEVE_SUMMARY, &mail_table_data, true, &err) || !mail_table_data)  {
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

	/*  BEGIN TRANSACTION; */
	emstorage_begin_transaction(NULL, NULL, NULL);

	if (!emstorage_add_attachment(&attachment_tbl, 0, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_add_attachment failed [%d]", err);

		goto FINISH_OFF;
	}
	
	attachment->attachment_id = attachment_tbl.attachment_id;

	if (attachment->attachment_path)  {
		char buf[512];

		if (!attachment->inline_content_status) {
			if (!emstorage_create_dir(account_id, mail_id, attachment_tbl.attachment_id, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
				goto FINISH_OFF;
			}
			attachment_id = attachment_tbl.attachment_id;
		}

		if (!emstorage_get_save_name(account_id, mail_id, attachment_id, attachment->attachment_name, buf, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}
        attachment_tbl.attachment_path = buf;

		if (!emstorage_change_attachment_field(mail_id, UPDATE_SAVENAME, &attachment_tbl, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_change_attachment_field failed [%d]", err);

			goto FINISH_OFF;
		}

		if (!emstorage_change_mail_field(mail_id, APPEND_BODY, mail_table_data, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);
	
			goto FINISH_OFF;
		}

		if (attachment->save_status) {
			if (!emstorage_move_file(attachment->attachment_path, buf, false, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);

				goto FINISH_OFF;
			}
		}

		/* Here only filename is being updated. Since first add is being done there will not be any old files.
		    So no need to check for old files in this update case */
		if (!emstorage_change_attachment_field(mail_id, UPDATE_SAVENAME, &attachment_tbl, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_change_attachment_field failed [%d]", err);
	
			goto FINISH_OFF;
		}

		EM_SAFE_FREE(attachment->attachment_path);
		attachment->attachment_path = EM_SAFE_STRDUP(buf);
	}
	
	ret = true;
	
FINISH_OFF: 
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
			ret = false;
		}
	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (emstorage_rollback_transaction(NULL, NULL, NULL) == false)
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


INTERNAL_FUNC int emcore_mail_add_attachment_data(int input_mail_id, email_attachment_data_t *input_attachment_data)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], input_attachment_data[%p]", input_mail_id, input_attachment_data);

	int                        err             = EMAIL_ERROR_NONE;
	int                        attachment_id   = 0;
	char                       buf[512] = { 0, };
	emstorage_mail_tbl_t            *mail_table_data = NULL;
	emstorage_attachment_tbl_t  attachment_tbl  = { 0 };
	
	if (input_attachment_data == NULL)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (!emstorage_get_mail_field_by_id(input_mail_id, RETRIEVE_SUMMARY, &mail_table_data, true, &err) || !mail_table_data)  {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_field_by_id failed [%d]", err);

		goto FINISH_OFF2;
	}

	mail_table_data->attachment_count               = mail_table_data->attachment_count + 1;

	attachment_tbl.account_id                       = mail_table_data->account_id;
	attachment_tbl.mailbox_id                       = mail_table_data->mailbox_id;
	attachment_tbl.mail_id 	                        = input_mail_id;
	attachment_tbl.attachment_name                  = input_attachment_data->attachment_name;
	attachment_tbl.attachment_size                  = input_attachment_data->attachment_size;
	attachment_tbl.attachment_save_status           = input_attachment_data->save_status;
	attachment_tbl.attachment_drm_type          = input_attachment_data->drm_status;
	attachment_tbl.attachment_inline_content_status = input_attachment_data->inline_content_status;
	
	/*  BEGIN TRANSACTION; */
	emstorage_begin_transaction(NULL, NULL, NULL);

	if (!emstorage_add_attachment(&attachment_tbl, 0, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_add_attachment failed [%d]", err);

		goto FINISH_OFF;
	}
	
	input_attachment_data->attachment_id = attachment_tbl.attachment_id;

	if (input_attachment_data->attachment_path)  {
		if (!input_attachment_data->inline_content_status) {
			if (!emstorage_create_dir(mail_table_data->account_id, input_mail_id, attachment_tbl.attachment_id, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
				goto FINISH_OFF;
			}
			attachment_id = attachment_tbl.attachment_id;
		}

		if (!emstorage_get_save_name(mail_table_data->account_id, input_mail_id, attachment_id, input_attachment_data->attachment_name, buf, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}
        attachment_tbl.attachment_path = buf;

		if (!emstorage_change_attachment_field(input_mail_id, UPDATE_SAVENAME, &attachment_tbl, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_change_attachment_field failed [%d]", err);

			goto FINISH_OFF;
		}

		if (!emstorage_change_mail_field(input_mail_id, APPEND_BODY, mail_table_data, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);
	
			goto FINISH_OFF;
		}

		if (input_attachment_data->save_status) {
			if (!emstorage_move_file(input_attachment_data->attachment_path, buf, false, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);

				goto FINISH_OFF;
			}
		}
		
		/* Here only filename is being updated. Since first add is being done there will not be any old files.
		    So no need to check for old files in this update case */
		if (!emstorage_change_attachment_field(input_mail_id, UPDATE_SAVENAME, &attachment_tbl, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_change_attachment_field failed [%d]", err);
	
			goto FINISH_OFF;
		}
		
		EM_SAFE_FREE(input_attachment_data->attachment_path);
		input_attachment_data->attachment_path = EM_SAFE_STRDUP(buf);
	}
	
FINISH_OFF:
	if (err == EMAIL_ERROR_NONE) {	/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
		}
	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (emstorage_rollback_transaction(NULL, NULL, NULL) == false)
			err = EMAIL_ERROR_DB_FAILURE;
	}	
	
FINISH_OFF2:
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
int emcore_delete_mail_attachment(int attachment_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_id[%d], err_code[%p]", attachment_id, err_code);
	
	if (attachment_id == 0)  {
		EM_DEBUG_EXCEPTION("attachment_id[%d]", attachment_id);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	/*  BEGIN TRANSACTION; */
	emstorage_begin_transaction(NULL, NULL, NULL);
	
	if (!emstorage_delete_attachment_on_db(attachment_id, false, &error))  {
		EM_DEBUG_EXCEPTION("emstorage_delete_attachment_on_db failed [%d]", error);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(NULL, NULL, NULL) == false) {
			error = EMAIL_ERROR_DB_FAILURE;
			ret = false;
		}
	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (emstorage_rollback_transaction(NULL, NULL, NULL) == false)
			error = EMAIL_ERROR_DB_FAILURE;
	}

	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

static int emcore_mail_update_attachment_data(int input_mail_id, email_attachment_data_t *input_attachment_data)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], input_attachment_data[%p]", input_mail_id, input_attachment_data);

	int                         err = EMAIL_ERROR_NONE;
	int                         attachment_id = 0;
	char                        buf[512] = { 0 , };
	emstorage_attachment_tbl_t *existing_attachment_info = NULL;
	emstorage_attachment_tbl_t  attachment_tbl = { 0 };

	if (input_attachment_data == NULL)  {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF2;
	}

	if (!emstorage_get_attachment(input_attachment_data->attachment_id, &existing_attachment_info, 1, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_attachment failed [%d]", err);

		goto FINISH_OFF2;
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
	attachment_tbl.attachment_id                    = input_attachment_data->attachment_id;

	if (!input_attachment_data->inline_content_status) {
		if (!emstorage_create_dir(attachment_tbl.account_id, input_mail_id, attachment_tbl.attachment_id, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}
		attachment_id = attachment_tbl.attachment_id;
	}
	
	if (!emstorage_get_save_name(attachment_tbl.account_id, input_mail_id, attachment_id, input_attachment_data->attachment_name, buf, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
		goto FINISH_OFF2;
	}
    attachment_tbl.attachment_path = buf;

	EM_DEBUG_LOG("downloaded [%d], savename [%s], attachment_path [%s]", input_attachment_data->save_status, input_attachment_data->attachment_path, existing_attachment_info->attachment_path);
	if (input_attachment_data->save_status && EM_SAFE_STRCMP(input_attachment_data->attachment_path, existing_attachment_info->attachment_path) != 0) {
		if (!emstorage_move_file(input_attachment_data->attachment_path, buf, false ,&err))  {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed [%d]", err);

			goto FINISH_OFF2;
		}
	}
	else
		EM_DEBUG_LOG("no need to move");

	EM_SAFE_FREE(input_attachment_data->attachment_path);
	input_attachment_data->attachment_path = EM_SAFE_STRDUP(buf);

	emstorage_begin_transaction(NULL, NULL, NULL);

	if (!emstorage_update_attachment(&attachment_tbl, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_update_attachment failed [%d]", err);

		goto FINISH_OFF;
	}
	
FINISH_OFF:
	if (err == EMAIL_ERROR_NONE) {	/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
		}
	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (emstorage_rollback_transaction(NULL, NULL, NULL) == false)
			err = EMAIL_ERROR_DB_FAILURE;
	}

FINISH_OFF2:
	if (existing_attachment_info)
		emstorage_free_attachment(&existing_attachment_info, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


static int emcore_mail_compare_filename_of_attachment_data(int input_mail_id, int input_attachment_a_id, email_attachment_data_t *input_attachment_b_data, int *result)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], input_attachment_a_id[%d], input_attachment_b_data[%p], result[%p]", input_mail_id, input_attachment_a_id, input_attachment_b_data, result);

	EM_IF_NULL_RETURN_VALUE(input_attachment_b_data, false);
	EM_IF_NULL_RETURN_VALUE(result, false);

	int err, err_2, ret = EMAIL_ERROR_NONE;
	emstorage_attachment_tbl_t *attachment_a_tbl = NULL;

	if (!emstorage_get_attachment(input_attachment_a_id, &attachment_a_tbl, 1, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_attachment failed [%d]", err);

		goto FINISH_OFF;
	}

	if (attachment_a_tbl->attachment_name && input_attachment_b_data->attachment_name) {
		EM_DEBUG_LOG("attachment_a_tbl->attachment_name [%s], input_attachment_b_data->name [%s]", attachment_a_tbl->attachment_name, input_attachment_b_data->attachment_name);
		*result = strcmp(attachment_a_tbl->attachment_name, input_attachment_b_data->attachment_name);
	}

	ret = true;

FINISH_OFF: 

	if (attachment_a_tbl)
		emstorage_free_attachment(&attachment_a_tbl, 1, &err_2);
	EM_DEBUG_FUNC_END("*result [%d]", *result);
	return ret;
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
INTERNAL_FUNC int emcore_mail_copy(int mail_id, email_mailbox_t *dst_mailbox, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], dst_mailbox[%p], err_code[%p]", mail_id, dst_mailbox, err_code);
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int i;
	emstorage_mail_tbl_t *mail = NULL;
	emstorage_attachment_tbl_t *atch_list = NULL;
	char buf[512];
	int count = EMAIL_ATTACHMENT_MAX_COUNT;
	char *mailbox_name = NULL;

	if (!emstorage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);

		goto FINISH_OFF;
	}

	if ( (err = emstorage_get_attachment_list(mail_id, true, &atch_list, &count)) != EMAIL_ERROR_NONE ){
		EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed [%d]", err);

		goto FINISH_OFF;
	}

	/*  get increased uid. */
	if (!emstorage_increase_mail_id(&mail->mail_id, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_increase_mail_id failed [%d]", err);

		goto FINISH_OFF;
	}

	/*  copy mail body(text) file */
	if (mail->file_path_plain)  {
		if (!emstorage_create_dir(dst_mailbox->account_id, mail->mail_id, 0, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);

			goto FINISH_OFF;
		}

		gchar *filename = g_path_get_basename(mail->file_path_plain);

		if (!emstorage_get_save_name(dst_mailbox->account_id, mail->mail_id, 0, filename, buf, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			g_free(filename);

			goto FINISH_OFF;
		}

		g_free(filename);

		if (!emstorage_copy_file(mail->file_path_plain, buf, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_copy_file failed [%d]", err);

			goto FINISH_OFF;
		}

		mail->file_path_plain = EM_SAFE_STRDUP(buf);
	}

	/*  copy mail body(html) file */
	if (mail->file_path_html)  {
		if (!emstorage_create_dir(dst_mailbox->account_id, mail->mail_id, 0, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);

			goto FINISH_OFF;
		}

		gchar *filename = g_path_get_basename(mail->file_path_html);

		if (!emstorage_get_save_name(dst_mailbox->account_id, mail->mail_id, 0, filename, buf, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
			g_free(filename);

			goto FINISH_OFF;
		}

		g_free(filename);

		if (!emstorage_copy_file(mail->file_path_html, buf, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_copy_file failed [%d]", err);
	
			goto FINISH_OFF;
		}

		mail->file_path_html = EM_SAFE_STRDUP(buf);
	}

	/*  BEGIN TRANSACTION; */
	emstorage_begin_transaction(NULL, NULL, NULL);

	/*  insert mail data */
	EM_SAFE_FREE(mail->mailbox_name);
	
	mail->account_id   = dst_mailbox->account_id;
	mail->mailbox_id   = dst_mailbox->mailbox_id;
	mail->mailbox_name = EM_SAFE_STRDUP(dst_mailbox->mailbox_name);
	mail->mailbox_type = dst_mailbox->mailbox_type;
	
	if (!emstorage_add_mail(mail, 0, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_add_mail failed [%d]", err);

		if (mail->file_path_plain)  {
			if (!emstorage_delete_file(mail->file_path_plain, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_delete_file failed [%d]", err);
				emstorage_rollback_transaction(NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}
		if (mail->file_path_html) {
			if (!emstorage_delete_file(mail->file_path_html, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_delete_file failed [%d]", err);
				emstorage_rollback_transaction(NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}

		/*  ROLLBACK TRANSACTION; */
		emstorage_rollback_transaction(NULL, NULL, NULL);


		goto FINISH_OFF;
	}

	/*  copy attachment file */
	for (i = 0; i<count; i++)  {
		if (atch_list[i].attachment_path)  {
			if (!emstorage_create_dir(dst_mailbox->account_id, mail->mail_id, i+1, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
				break;
			}
			
			if (!emstorage_get_save_name(dst_mailbox->account_id, mail->mail_id, i+1, atch_list[i].attachment_name, buf, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
				break;
			}

			if (!emstorage_copy_file(atch_list[i].attachment_path, buf, false, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_copy_file failed [%d]", err);
				break;
			}

			EM_SAFE_FREE(atch_list[i].attachment_path);

			atch_list[i].attachment_path = EM_SAFE_STRDUP(buf);
		}

		atch_list[i].account_id = dst_mailbox->account_id;
		atch_list[i].mail_id = mail->mail_id;
		atch_list[i].mailbox_id = mail->mailbox_id;

		if (!emstorage_add_attachment(&atch_list[i], 0, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_add_attachment failed [%d]", err);

			break;
		}
	}

	/*  in case error happened, delete copied file. */
	if (i && i != count)  {
		for (;i >= 0; i--)  {
			if (atch_list[i].attachment_path)  {
			
				if (!emstorage_delete_file(atch_list[i].attachment_path, &err))  {
					EM_DEBUG_EXCEPTION("emstorage_delete_file failed [%d]", err);
					emstorage_rollback_transaction(NULL, NULL, NULL);
					goto FINISH_OFF;
				}
			}
		}

		if (mail->file_path_plain)  {
			if (!emstorage_delete_file(mail->file_path_plain, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_delete_file failed [%d]", err);
				emstorage_rollback_transaction(NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}
		if (mail->file_path_html)  {
			if (!emstorage_delete_file(mail->file_path_html, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_delete_file failed [%d]", err);
				emstorage_rollback_transaction(NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}

		/*  ROLLBACK TRANSACTION; */
		emstorage_rollback_transaction(NULL, NULL, NULL);
		goto FINISH_OFF;
	}

	if (emstorage_commit_transaction(NULL, NULL, NULL) == false) {
		err = EMAIL_ERROR_DB_FAILURE;
		if (emstorage_rollback_transaction(NULL, NULL, NULL) == false)
			err = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mailbox_name_by_mailbox_type(dst_mailbox->account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox_name, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_name_by_mailbox_type failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!strcmp(dst_mailbox->mailbox_name, mailbox_name) && !(mail->flags_seen_field))
		emcore_check_unread_mail();

	ret = true;

FINISH_OFF: 
	if (atch_list != NULL)
		emstorage_free_attachment(&atch_list, count, NULL);

	if (mail != NULL)
		emstorage_free_mail(&mail, 1, NULL);

	EM_SAFE_FREE(mailbox_name);

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
INTERNAL_FUNC int emcore_move_mail(int mail_ids[], int mail_ids_count, int dst_mailbox_id, int noti_param_1, int noti_param_2 ,int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], mail_ids_count[%d], dst_mailbox_id[%d], noti_param [%d], err_code[%p]", mail_ids, mail_ids_count, dst_mailbox_id, noti_param_1, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t *mail_list = NULL;
	int account_id = 0;
	int i = 0, parameter_string_length = 0;
	char *parameter_string = NULL, mail_id_string[10];

	if ( dst_mailbox_id <= 0  && mail_ids_count < 1) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mail_field_by_multiple_mail_id(mail_ids, mail_ids_count, RETRIEVE_FLAG, &mail_list, true, &err) || !mail_list) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_field_by_multiple_mail_id failed [%d]", err);

		goto FINISH_OFF;
	}

	account_id = mail_list[0].account_id;

	if(!emstorage_move_multiple_mails(account_id, dst_mailbox_id, mail_ids, mail_ids_count, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_move_multiple_mails failed [%d]", err);
		goto FINISH_OFF;
	}
	
	/* Sending a notification */
	parameter_string_length = sizeof(char) * (mail_ids_count * 10 + 128/*MAILBOX_LEN_IN_MAIL_TBL*/ * 2);
	parameter_string = em_malloc(parameter_string_length);

	if (mail_list[0].mailbox_id > 0)
		SNPRINTF(parameter_string, parameter_string_length, "%d%c%d%c", mail_list[0].mailbox_id, 0x01, dst_mailbox_id , 0x01);

	if (parameter_string == NULL) {
		EM_DEBUG_EXCEPTION("Memory allocation for mail_id_list_string failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < mail_ids_count; i++) {
		memset(mail_id_string, 0, 10);
		SNPRINTF(mail_id_string, 10, "%d,", mail_ids[i]);
		strcat(parameter_string, mail_id_string);
	}
	
	EM_DEBUG_LOG("num : [%d], param string : [%s]", mail_ids_count , parameter_string);

	if (!emstorage_notify_storage_event(NOTI_MAIL_MOVE, account_id, noti_param_1, parameter_string, noti_param_2))
		EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event failed [NOTI_MAIL_MOVE] >>>> ");


	for (i = 0; i < mail_ids_count; i++) {
		if (!emstorage_update_latest_thread_mail(account_id, mail_list[i].thread_id, 0, 0, true, &err))
			EM_DEBUG_EXCEPTION("emstorage_update_latest_thread_mail failed [%d]", err);
	}

	if (!emstorage_notify_storage_event(NOTI_MAIL_MOVE_FINISH, account_id, noti_param_1, parameter_string, noti_param_2))
		EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event failed [NOTI_MAIL_MOVE_FINISH] >>>> ");

	emcore_check_unread_mail();

	ret = true;

FINISH_OFF:
	emstorage_free_mail(&mail_list, mail_ids_count, NULL);
	EM_SAFE_FREE(parameter_string);
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}


INTERNAL_FUNC int emcore_move_mail_on_server(int account_id, int src_mailbox_id,  int mail_ids[], int num, char *dest_mailbox, int *error_code)
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
	
	ref_account = emcore_get_account_reference(account_id);
	if (!ref_account)  {
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

	for (i = 0; i < num; i++)  {
		mail_id = mail_ids[i];

		if (!emstorage_get_mail_by_id(mail_id, &mail, false, &err_code) || !mail)  {
			EM_DEBUG_EXCEPTION("emstorage_get_uid_by_mail_id  :  emstorage_get_downloaded_mail failed [%d]", err_code);
			mail = NULL;
			if (err_code == EMAIL_ERROR_MAIL_NOT_FOUND)  {	/*  not server mail */
				/* err = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER; */
				/* continue; */
			}
			else {
				*error_code = err_code;
			}

			ret = 0;
			goto FINISH_OFF;
		}

		if (!emcore_connect_to_remote_mailbox(account_id, src_mailbox_id, (void **)&stream, &err_code))		/* faizan.h@samsung.com mail_move_fix_07042009 */ {
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
				}
				else {
					/*  send EXPUNGE command */
					if (!imap4_send_command(stream, IMAP4_CMD_EXPUNGE, &err_code)) {
						EM_DEBUG_EXCEPTION("imap4_send_command failed [%d]", err_code);

						if (err_code == EMAIL_ERROR_IMAP4_STORE_FAILURE)
							err_code = EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER;
						goto FINISH_OFF;
					}

					/* faizan.h@samsung.com   duplicate copy while sync mailbox issue fixed */
					EM_DEBUG_LOG(">>>mailbox_name[%s]>>>>>>", mail->mailbox_name);
					EM_DEBUG_LOG(">>>g_new_server_uid[%s]>>>>>>", g_new_server_uid);
					EM_DEBUG_LOG(">>>mail_id[%d]>>>>>>", mail_id);

					if (!emstorage_update_read_mail_uid(mail_id, g_new_server_uid, mail->mailbox_name, &err_code)) {
						EM_DEBUG_EXCEPTION("emstorage_update_read_mail_uid failed [%d]", err_code);
						goto FINISH_OFF;
					}

					EM_DEBUG_LOG("Mail MOVE SUCCESS ");
				}
			}
			else
				EM_DEBUG_EXCEPTION(">>>> Server MAIL ID IS NULL >>>> ");
		}
		else {
			EM_DEBUG_EXCEPTION(">>>> STREAM DATA IS NULL >>> ");
			ret = 0;
			goto FINISH_OFF;
		}
	}

	/* ret = true; */

FINISH_OFF:
	if (stream) emcore_close_mailbox(account_id, stream);

	if (mail != NULL)
		emstorage_free_mail(&mail, 1, NULL);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

static int emcore_save_mail_file(int account_id, int mail_id, int attachment_id, char *src_file_path, char *file_name, char *full_path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], attachment_id[%d] , file_name[%p] , full_path[%p] , err_code[%p]", account_id, mail_id, attachment_id, file_name, full_path, err_code);

	int ret = false, err = EMAIL_ERROR_NONE;

	if (!file_name || !full_path || !src_file_path) {
		EM_DEBUG_EXCEPTION("Invalid paramter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_create_dir(account_id, mail_id, attachment_id, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_create_dir failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!emstorage_get_save_name(account_id, mail_id, attachment_id, file_name, full_path, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_save_name failed [%d]", err);
		goto FINISH_OFF;
	}

	if (strcmp(src_file_path, full_path) != 0)  {
		if (!emstorage_copy_file(src_file_path, full_path, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_copy_file failed [%d]", err);

			goto FINISH_OFF;
		}
	}

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;
	return 1;
}

/* description
 *    update mail information
 */
INTERNAL_FUNC int emcore_update_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list[%p], input_attachment_count[%d], input_meeting_request[%p], input_from_eas[%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas);

	char                   filename_buf[1024]         = {0, };
	int                    i                          = 0;
	int                    err                        = EMAIL_ERROR_NONE;
	int                    local_inline_content_count = 0;
	emstorage_mail_tbl_t  *converted_mail_tbl_data    = NULL;
	email_meeting_request_t *meeting_req = NULL;
	struct stat            st_buf;

	if (!input_mail_data || (input_attachment_count && !input_attachment_data_list) || (!input_attachment_count &&input_attachment_data_list))  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF2;
	}

	if(input_from_eas == 0) {
		if (input_mail_data->file_path_plain)  {
			if (stat(input_mail_data->file_path_plain, &st_buf) < 0)  {
				EM_DEBUG_EXCEPTION("input_mail_data->file_path_plain, stat(\"%s\") failed...", input_mail_data->file_path_plain);
				err = EMAIL_ERROR_INVALID_MAIL;
				goto FINISH_OFF;
			}
		}
		
		if (input_mail_data->file_path_html)  {
			if (stat(input_mail_data->file_path_html, &st_buf) < 0)  {
				EM_DEBUG_EXCEPTION("input_mail_data->file_path_html, stat(\"%s\") failed...", input_mail_data->file_path_html);
				err = EMAIL_ERROR_INVALID_MAIL;
				goto FINISH_OFF;
			}
		}
		
		if (input_attachment_count && input_attachment_data_list)  {
			for (i = 0; i < input_attachment_count; i++)  {
				if (input_attachment_data_list[i].save_status) {
					if (!input_attachment_data_list[i].attachment_path || stat(input_attachment_data_list[i].attachment_path, &st_buf) < 0)  {
						EM_DEBUG_EXCEPTION("stat(\"%s\") failed...", input_attachment_data_list[i].attachment_path);
						err = EMAIL_ERROR_INVALID_ATTACHMENT;
						goto FINISH_OFF;
					}
				}
			}
		}
	}
	
	if(input_mail_data->mail_size == 0) {
		 emcore_calc_mail_size(input_mail_data, input_attachment_data_list, input_attachment_count, &(input_mail_data->mail_size));
	}

	if (!input_mail_data->full_address_from)  {
		email_account_t *ref_account = NULL;
		if (!(ref_account = emcore_get_account_reference(input_mail_data->account_id)))  {
			EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", input_mail_data->account_id);
			err = EMAIL_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF2;
		}
		input_mail_data->full_address_from = ref_account->user_email_address;
	}
	
	if (input_mail_data->file_path_plain)  {   /*  Save plain text body. */
		if (!emcore_save_mail_file(input_mail_data->account_id, input_mail_data->mail_id, 0, input_mail_data->file_path_plain, "UTF-8", filename_buf, &err)) {
			EM_DEBUG_EXCEPTION("emcore_save_mail_file failed [%d]", err);
			goto FINISH_OFF2;
		}
		EM_SAFE_FREE(input_mail_data->file_path_plain);
		input_mail_data->file_path_plain = EM_SAFE_STRDUP(filename_buf);
	}
	
	if (input_mail_data->file_path_html)  {   /*  Save HTML text body. */
		if (!emcore_save_mail_file(input_mail_data->account_id, input_mail_data->mail_id, 0, input_mail_data->file_path_html, "UTF-8.htm", filename_buf, &err)) {
			EM_DEBUG_EXCEPTION("emcore_save_mail_file failed [%d]", err);
			goto FINISH_OFF2;
		}
		EM_SAFE_FREE(input_mail_data->file_path_html);
		input_mail_data->file_path_html = EM_SAFE_STRDUP(filename_buf);
	}

	if (input_mail_data->file_path_mime_entity)  {   /*  Save mime entity. */
		if (!emcore_save_mail_file(input_mail_data->account_id, input_mail_data->mail_id, 0, input_mail_data->file_path_mime_entity, "mime_entity", filename_buf, &err)) {
			EM_DEBUG_EXCEPTION("emcore_save_mail_file failed [%d]", err);
			goto FINISH_OFF2;
		}
		EM_SAFE_FREE(input_mail_data->file_path_mime_entity);
		input_mail_data->file_path_mime_entity = EM_SAFE_STRDUP(filename_buf);
	}

	if (input_attachment_data_list && input_attachment_count)  {
		int i = 0;
		int compare_result = 1;
		email_attachment_data_t *temp_attachment_data = NULL;

		for(i = 0; i < input_attachment_count; i++) {
			temp_attachment_data = input_attachment_data_list + i;
			if ( (err = emcore_mail_compare_filename_of_attachment_data(input_mail_data->mail_id, temp_attachment_data->attachment_id, temp_attachment_data, &compare_result)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_mail_compare_filename_of_attachment_data failed [%d]", err);
				compare_result = 1;
			}
	
			if (compare_result == 0) {
				EM_DEBUG_LOG("file name and attachment id are same, update exising attachment");
				if (!emcore_mail_update_attachment_data(input_mail_data->mail_id, temp_attachment_data))  {
					EM_DEBUG_EXCEPTION("emcore_mail_update_attachment_data failed [%d]", err);
					goto FINISH_OFF2;
				}
			}
			else {
				EM_DEBUG_LOG("save names are different");
				if(temp_attachment_data->attachment_id > 0) {
					if (!emcore_delete_mail_attachment(temp_attachment_data->attachment_id, &err)) {
						EM_DEBUG_EXCEPTION("emcore_delete_mail_attachment failed [%d]", err);
						goto FINISH_OFF2;
					}
				}

				if ( (err = emcore_mail_add_attachment_data(input_mail_data->mail_id, temp_attachment_data)) != EMAIL_ERROR_NONE)  {
					EM_DEBUG_EXCEPTION("emcore_mail_add_attachment failed [%d]", err);
					goto FINISH_OFF2;
				}
			}

			if (temp_attachment_data->inline_content_status)
				local_inline_content_count++;
		}
	}
	
	input_mail_data->attachment_count     = input_attachment_count;
	input_mail_data->inline_content_count = local_inline_content_count;

	if (!input_mail_data->date_time) {
		/* time isn't set */
		input_mail_data->date_time = time(NULL);
	}

	EM_DEBUG_LOG("preview_text[%p]", input_mail_data->preview_text);
	if (input_mail_data->preview_text == NULL) {
		if ( (err =emcore_get_preview_text_from_file(input_mail_data->file_path_plain, input_mail_data->file_path_html, MAX_PREVIEW_TEXT_LENGTH, &(input_mail_data->preview_text))) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_preview_text_from_file failed[%d]", err);
			goto FINISH_OFF2;
		}
	}

	if(!em_convert_mail_data_to_mail_tbl(input_mail_data, 1, &converted_mail_tbl_data,  &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_data_to_mail_tbl failed[%d]", err);
		goto FINISH_OFF2;
	}
	
	/*  BEGIN TRANSACTION; */
	emstorage_begin_transaction(NULL, NULL, NULL);
	
	if (!emstorage_change_mail_field(input_mail_data->mail_id, UPDATE_MAIL, converted_mail_tbl_data, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_change_mail_field failed [%d]", err);

		goto FINISH_OFF;
	}
	
	if (input_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_REQUEST
		|| input_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_RESPONSE
		|| input_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		/*  check where there is a meeting request in DB */
		if (!emstorage_get_meeting_request(input_mail_data->mail_id, &meeting_req, false, &err) && err != EMAIL_ERROR_DATA_NOT_FOUND) {
			EM_DEBUG_EXCEPTION("emstorage_get_meeting_request failed [%d]", err);
			goto FINISH_OFF;
		}
		EM_SAFE_FREE(meeting_req);
		if (err == EMAIL_ERROR_DATA_NOT_FOUND) {	/*  insert */
			emstorage_mail_tbl_t *original_mail = NULL;

			if (!emstorage_get_mail_by_id(input_mail_data->mail_id, &original_mail, false, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
				goto FINISH_OFF;
			}

			if (original_mail)	 {
			if (!emstorage_add_meeting_request(input_mail_data->account_id, original_mail->mailbox_id, input_meeting_request, false, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_add_meeting_request failed [%d]", err);

				goto FINISH_OFF;
			}
				emstorage_free_mail(&original_mail, 1, NULL);
		}
		}
		else {	/*  update */
			if (!emstorage_update_meeting_request(input_meeting_request, false, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_update_meeting_request failed [%d]", err);

				goto FINISH_OFF;
			}
		}
	}
	
FINISH_OFF: 
	if (err == EMAIL_ERROR_NONE) {
		/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
		}

		if (input_mail_data->meeting_request_status && !emstorage_notify_storage_event(NOTI_MAIL_UPDATE, input_mail_data->account_id, input_mail_data->mail_id, NULL, UPDATE_MEETING))
			EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event failed [ NOTI_MAIL_UPDATE : UPDATE_MEETING_RESPONSE ] >>>> ");
	}
	else {
		/*  ROLLBACK TRANSACTION; */
		if (emstorage_rollback_transaction(NULL, NULL, NULL) == false)
			err = EMAIL_ERROR_DB_FAILURE;
	}
	
FINISH_OFF2:

	if(meeting_req)
		emstorage_free_meeting_request(meeting_req);

	if(converted_mail_tbl_data)
		emstorage_free_mail(&converted_mail_tbl_data, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


INTERNAL_FUNC int emcore_set_flags_field(int account_id, int mail_ids[], int num, email_flags_field_type field_type, int value, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mail_ids[%p], num [%d], field_type [%d], value[%d], err_code[%p]", account_id, mail_ids, num, field_type, value, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	char *field_type_name[EMAIL_FLAGS_FIELD_COUNT] = { "flags_seen_field"
		, "flags_deleted_field", "flags_flagged_field", "flags_answered_field"
		, "flags_recent_field", "flags_draft_field", "flags_forwarded_field" };

	if(field_type < 0 || field_type >= EMAIL_FLAGS_FIELD_COUNT || mail_ids == NULL || num <= 0 || account_id == 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto  FINISH_OFF;
	}

	if (!emstorage_set_field_of_mails_with_integer_value(account_id, mail_ids, num, field_type_name[field_type], value, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]", err);
		goto FINISH_OFF;
	}
		
	emcore_check_unread_mail();

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
	
	if (!stream || !result_total_body_size)  {
		EM_DEBUG_EXCEPTION("stream[%p], total_body_size[%p]", stream, msgno, result_total_body_size);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	pop3local = (POP3LOCAL *)(((MAILSTREAM *)stream)->local);

	if (!pop3local  || !pop3local->netstream)  {
		err = EMAIL_ERROR_INVALID_STREAM;
		goto FINISH_OFF;
	}
	memset(command, 0x00, sizeof(command));

	SNPRINTF(command, sizeof(command), "LIST %d\015\012", msgno);

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG("[POP3] >>> [%s]", command);
#endif

	
	/*  send command  :  LIST [msgno] - to get the size of the mail */
	if (!net_sout(pop3local->netstream, command, (int)strlen(command)))  {
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

		if (*response == '.')  {
			EM_SAFE_FREE(response);
			break;
		}

		if (*response == '+')  {		/*  "+ OK" */
			char *p = NULL;

			if (!(p = strchr(response + strlen("+OK "), ' ')))  {
				err = EMAIL_ERROR_INVALID_RESPONSE;
				goto FINISH_OFF;
			}

			total_body_size = atoi(p + 1);
			EM_DEBUG_LOG("Body size [%d]", total_body_size);

			if (result_total_body_size) {
				*result_total_body_size = total_body_size;
				break;
			}
		}
		else if (*response == '-')  {	/*  "- ERR" */
			err = EMAIL_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}
		else  {
			err = EMAIL_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}

		EM_SAFE_FREE(response);
	}

	memset(command, 0x00, sizeof(command));

	if (limited_size && total_body_size > limited_size) {
		int count_of_line = limited_size / 80;
		SNPRINTF(command, sizeof(command), "TOP %d %d", msgno, count_of_line);
	}
	else
		SNPRINTF(command, sizeof(command), "RETR %d", msgno);

	EM_DEBUG_LOG("[POP3] >>> %s", command);

	emcore_set_network_error(EMAIL_ERROR_NONE);		/*  set current network error as EMAIL_ERROR_NONE before network operation */
	/*  get mail from mail server */
	if (!pop3_send((MAILSTREAM *)stream, command, NULL))  {
		EM_DEBUG_EXCEPTION("pop3_send failed...");

		email_session_t *session = NULL;
		
		if (!emcore_get_current_session(&session))  {
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


INTERNAL_FUNC int emcore_sync_flag_with_server(int mail_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%p], err_code[%p]", mail_id, err_code);
		
	if (mail_id < 1)  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int status = EMAIL_DOWNLOAD_FAIL;
	MAILSTREAM *stream = NULL;
	email_internal_mailbox_t mailbox = {0};
	emstorage_mail_tbl_t *mail = NULL;
	email_account_t *ref_account = NULL;
	int account_id = 0;
	int msgno = 0;
	char set_flags[100] = { 0, };
	char clear_flags[100] = { 0, };
	char tmp[100] = { 0, };
	
	if (!emcore_check_thread_status())  {
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	if (!emstorage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);

		goto FINISH_OFF;
	}

	account_id = mail->account_id;

	if (!(ref_account = emcore_get_account_reference(account_id)))   {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	if (!emcore_check_thread_status())  {
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	/*  open mail server. */
	if (!emcore_connect_to_remote_mailbox(account_id, mail->mailbox_id, (void **)&stream, &err) || !stream)  {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		status = EMAIL_LIST_CONNECTION_FAIL;
		goto FINISH_OFF;
	}


	if (!emcore_check_thread_status())  {
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	mailbox.mailbox_name = mail->mailbox_name;	
	mailbox.account_id = account_id;
	mailbox.mail_stream = stream;

	/*  download all uids from server. */
	if (!emcore_get_mail_msgno_by_uid(ref_account, &mailbox, mail->server_mail_id, &msgno, &err))  {
		EM_DEBUG_EXCEPTION("emcore_get_mail_msgno_by_uid failed message_no  :  %d ", err);
		goto FINISH_OFF;
	}
		
	sprintf (tmp, "%d", msgno);

	if (mail->flags_seen_field)
		sprintf(set_flags, "\\Seen");
	else
		sprintf(clear_flags, "\\Seen");

	if (mail->flags_answered_field)
		sprintf(set_flags, "%s \\Answered", set_flags);
	else
		sprintf(clear_flags, "%s \\Answered", clear_flags);
		
	if (mail->flags_flagged_field)
		sprintf(set_flags, "%s \\Flagged", set_flags);
	else
		sprintf(clear_flags, "%s \\Flagged", clear_flags);

	if (mail->flags_forwarded_field)
		sprintf(set_flags, "%s $Forwarded", set_flags);
	else
		sprintf(clear_flags, "%s $Forwarded", clear_flags);

	if (strlen(set_flags) > 0)  {
		EM_DEBUG_LOG(">>>> Calling mail_setflag [%s] ", set_flags);
		mail_flag(stream, tmp, set_flags, ST_SET | ST_SILENT);
		EM_DEBUG_LOG(">>>> End mail_setflag ");
	}

	if (strlen(clear_flags) > 0)  {
		EM_DEBUG_LOG(">>>> Calling mail_clearflag [%s]", clear_flags);
		mail_clearflag(stream, tmp, clear_flags);
		EM_DEBUG_LOG(">>>> End mail_clearflag ");
	}
		
	if (mail->lock_status) {
		memset(set_flags, 0x00, 100);
		sprintf(set_flags, "Sticky");
		if (strlen(set_flags) > 0)  {
			EM_DEBUG_LOG(">>>> Calling mail_setflag [%s]", set_flags);
			mail_flag(stream, tmp, set_flags, ST_SET | ST_SILENT);
			EM_DEBUG_LOG(">>>> End mail_setflag ");
		}
	}

	EM_DEBUG_LOG(">>>> Returning from emcore_sync_flag_with_server ");

	if (!emcore_check_thread_status())  {
		err = EMAIL_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF: 
	
	if (stream)
		emcore_close_mailbox(account_id, stream);

	if (mail)
		emstorage_free_mail(&mail, 1, NULL);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);	
	return ret;
}

INTERNAL_FUNC int emcore_sync_seen_flag_with_server(int mail_ids[], int num, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], err_code[%p]", mail_ids[0], err_code);

	if (mail_ids[0] < 1)  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int status = EMAIL_DOWNLOAD_FAIL;
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

	memset(&mailbox, 0x00, sizeof(email_mailbox_t));

	mail_id = mail_ids[0];

	if (!emstorage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);

		goto FINISH_OFF;
	}

	account_id = mail->account_id;

	if (!(ref_account = emcore_get_account_reference(account_id)))   {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	/*  open mail server. */
	if (!emcore_connect_to_remote_mailbox(account_id, mail->mailbox_id, (void **)&stream, &err) || !stream)  {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed [%d]", err);
		status = EMAIL_LIST_CONNECTION_FAIL;
		goto FINISH_OFF;
	}

	mailbox.mailbox_name = mail->mailbox_name;
	mailbox.account_id = account_id;
	mailbox.mail_stream = stream;
	
	for (i = 0; i < num; i++)  {
		mail_id = mail_ids[i];
	
		if (!emcore_check_thread_status())  {
			err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

		if (!emstorage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
			EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
	
			goto FINISH_OFF;
		}
		
		if (!emcore_check_thread_status())  {
			err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

		/*  download message number from server. */
		if (!emcore_get_mail_msgno_by_uid(ref_account, &mailbox, mail->server_mail_id, &msgno, &err))  {
			EM_DEBUG_LOG("emcore_get_mail_msgno_by_uid failed message_no  :  %d ", err);
			goto FINISH_OFF;
		}
		
		memset(tmp, 0x00, 100);
		sprintf (tmp, "%d", msgno);

		memset(set_flags, 0x00, 100);
		memset(clear_flags, 0x00, 100);

		if (mail->flags_seen_field)
			sprintf(set_flags, "\\Seen");
		else
			sprintf(clear_flags, "\\Seen");
		EM_DEBUG_LOG("new_flag.seen :  %s ", set_flags);

		if (strlen(set_flags) > 0)  {
			EM_DEBUG_LOG(">>>> Calling mail_setflag ");
			mail_flag(stream, tmp, set_flags, ST_SET | ST_SILENT);
			EM_DEBUG_LOG(">>>> End mail_setflag ");
		}
		else {
			EM_DEBUG_LOG(">>>> Calling mail_clearflag ");
			mail_clearflag(stream, tmp, clear_flags);
			EM_DEBUG_LOG(">>>> End mail_clearflag ");
		}

		EM_DEBUG_LOG(">>>> Returning from emcore_sync_flag_with_server ");

		if (!emcore_check_thread_status())  {
			err = EMAIL_ERROR_CANCELLED;
			goto FINISH_OFF;
		}
	}
	ret = true;

FINISH_OFF:
	
	if (stream) emcore_close_mailbox(account_id, stream);
	if (mail) emstorage_free_mail(&mail, 1, NULL);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}

INTERNAL_FUNC void emcore_free_mail_data_list(email_mail_data_t **mail_list, int count)
{
	EM_DEBUG_FUNC_BEGIN("count[%d]", count);
	
	if (count <= 0 || !mail_list || !*mail_list)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");			
		return;
	}
		
	email_mail_data_t* p = *mail_list;
	int i;
	
	for (i = 0; i < count; i++)
		emcore_free_mail_data( p+i);

	EM_SAFE_FREE(*mail_list);

	EM_DEBUG_FUNC_END();
	}
	
INTERNAL_FUNC void emcore_free_mail_data(email_mail_data_t *mail_data)
{
	EM_DEBUG_FUNC_BEGIN();

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
	EM_SAFE_FREE(mail_data->preview_text);
	
	EM_DEBUG_FUNC_END();	
}


INTERNAL_FUNC int emcore_free_attachment_data(email_attachment_data_t **attachment_data_list, int attachment_data_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_data_list[%p], attachment_data_count [%d], err_code[%p]", attachment_data_list, attachment_data_count, err_code);	
	
	if (!attachment_data_list || !*attachment_data_list)  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	email_attachment_data_t* p = *attachment_data_list;
	int i = 0;
	
	for (i = 0; i < attachment_data_count; i++) {
		EM_SAFE_FREE(p[i].attachment_name);
		EM_SAFE_FREE(p[i].attachment_path);
	}

	EM_SAFE_FREE(p); *attachment_data_list = NULL;

	if(err_code)
		*err_code = EMAIL_ERROR_NONE;
	
	EM_DEBUG_FUNC_END();	
	return true;
}



#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

INTERNAL_FUNC int emcore_delete_pbd_activity(int account_id, int mail_id, int activity_id, int *err_code) 
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
	
	emstorage_begin_transaction(NULL, NULL, NULL);
	
	if (!emstorage_delete_pbd_activity(account_id, mail_id, activity_id, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_delete_pbd_activity failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF: 
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
			ret = false;
		}
	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (emstorage_rollback_transaction(NULL, NULL, NULL) == false)
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
	
	emstorage_begin_transaction(NULL, NULL, NULL);
	
	if (!emstorage_add_pbd_activity(local_activity, activity_id, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_add_pbd_activity failed [%d]", err);
		

		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF: 
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (emstorage_commit_transaction(NULL, NULL, NULL) == false) {
			err = EMAIL_ERROR_DB_FAILURE;
			ret = false;
		}
	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (emstorage_rollback_transaction(NULL, NULL, NULL) == false)
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

INTERNAL_FUNC int emcore_sync_flags_field_with_server(int mail_ids[], int num, email_flags_field_type field_type, int value, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids [%p], num [%d], field_type [%d], value [%d], err_code [%p]", mail_ids, num, field_type, value, err_code);
		
	if (NULL == mail_ids || num <= 0 || field_type < 0 || field_type >= EMAIL_FLAGS_FIELD_COUNT)  {
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
	int command_success = false;
	int account_id = 0;
	int mail_id = 0;
	int i = 0;
	int id_set_count = 0;
	int len_of_each_range = 0;
	int string_count = 0;
	email_account_t *temp_account;
	email_id_set_t *id_set = NULL;
	emstorage_mail_tbl_t *mail = NULL;
	email_uid_range_set *uid_range_set = NULL;
	email_uid_range_set *uid_range_node = NULL;
	char *field_type_name[EMAIL_FLAGS_FIELD_COUNT] = { "\\Seen"
		, "\\Deleted", "\\Flagged", "\\Answered"
		, "\\Recent", "\\Draft", "$Forwarded" }; 
	
	mail_id = mail_ids[0];
	
	if (!emstorage_get_mail_field_by_id(mail_id, RETRIEVE_FLAG, &mail, true, &err) || !mail) 		/*To DO :  This is a existing bug. on mail deletion before this call it will fail always */ {
		EM_DEBUG_LOG("emstorage_get_mail_by_id failed [%d]", err);

		goto FINISH_OFF;
	}

	account_id = mail[0].account_id;

	temp_account = emcore_get_account_reference(account_id);

	if (!temp_account)   {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	if (temp_account->incoming_server_type != EMAIL_SERVER_TYPE_IMAP4) {
		EM_DEBUG_EXCEPTION("Syncing seen flag is available only for IMAP4 server. The server type [%d]", temp_account->incoming_server_type);
		err = EMAIL_ERROR_NOT_SUPPORTED;
		goto FINISH_OFF;
	}
	
	if (!emcore_connect_to_remote_mailbox(account_id, mail->mailbox_id, (void **)&stream, &err) || !stream)  {
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

	if (false == emcore_form_comma_separated_strings(mail_ids, num, QUERY_SIZE - 90, &string_list, &string_count, &err))   {
		EM_DEBUG_EXCEPTION("emcore_form_comma_separated_strings failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Now execute one by one each comma separated string of mail_ids */

	for (i = 0; i < string_count; ++i) {
		/* Get the set of mail_ds and corresponding server_mail_ids sorted by server mail ids in ascending order */

		if (false == emstorage_get_id_set_from_mail_ids(string_list[i], &id_set, &id_set_count, &err)) {
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

			uid_range_node->uid_range[strlen(uid_range_node->uid_range) - 1] = '\0';
			
			/* Form the IMAP command */

			SNPRINTF(tag, MAX_TAG_SIZE, "%08lx", 0xffffffff & (stream->gensym++));

			if (value)
				SNPRINTF(cmd, MAX_IMAP_COMMAND_LENGTH, "%s UID STORE %s +FLAGS (%s)\015\012", tag, uid_range_node->uid_range, field_type_name[field_type]);
			else
				SNPRINTF(cmd, MAX_IMAP_COMMAND_LENGTH, "%s UID STORE %s -FLAGS (%s)\015\012", tag, uid_range_node->uid_range, field_type_name[field_type]);

			EM_DEBUG_LOG("[IMAP4] command %s", cmd);

			/* send command */

			if (!(imaplocal = stream->local) || !imaplocal->netstream)  {
				EM_DEBUG_EXCEPTION("invalid IMAP4 stream detected...");
				
				err = EMAIL_ERROR_UNKNOWN;		
				goto FINISH_OFF;
			}
		
			if (!net_sout(imaplocal->netstream, cmd, (int)strlen(cmd))) {
				EM_DEBUG_EXCEPTION("net_sout failed...");
				err = EMAIL_ERROR_CONNECTION_BROKEN;		
				goto FINISH_OFF;
			}

			/* Receive Response */

			command_success = false;
			
			while (imaplocal->netstream) {	
				if (!(p = net_getline(imaplocal->netstream)))  {
					EM_DEBUG_EXCEPTION("net_getline failed...");
				
					err = EMAIL_ERROR_INVALID_RESPONSE;	
					goto FINISH_OFF;
				}
			
				EM_DEBUG_LOG("[IMAP4 Response ] %s", p);
				
				if (!strncmp(p, tag, strlen(tag)))  {
					if (!strncmp(p + strlen(tag) + 1, "OK", 2))  {
						/*Delete all local activities */
						command_success = true;

						EM_SAFE_FREE(p);	
						break;
					}
					else  {
						/* 'NO' or 'BAD' */
						command_success = false;
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

#ifdef __FEATURE_LOCAL_ACTIVITY__
	if (ret) {
		emstorage_activity_tbl_t new_activity;
		for (i = 0; i<num ; i++) {		
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

	emcore_close_mailbox(0, stream);
	stream = NULL;

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;

}
#endif

INTERNAL_FUNC int emcore_mail_filter_by_rule(email_rule_t *filter_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_info[%p], err_code[%p]", filter_info, err_code);

	int ret = false, err = EMAIL_ERROR_NONE;
	int account_index, account_count, mail_id_index = 0;
	email_account_t *account_ref = NULL, *account_list_ref = NULL;
	int filtered_mail_id_count = 0, *filtered_mail_id_list = NULL, parameter_string_length = 0;
	char *parameter_string = NULL, mail_id_string[10] = { 0x00, };
	emstorage_mailbox_tbl_t *spam_mailbox = NULL;

	if (!filter_info)  {
		EM_DEBUG_EXCEPTION("filter_info[%p]", filter_info);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emcore_get_account_reference_list(&account_list_ref, &account_count, &err)) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference_list failed [%d]", err);
		goto FINISH_OFF;
	}

	for (account_index = 0; account_index < account_count; account_index++) {
		account_ref = account_list_ref + account_index;

		if (!emstorage_get_mailbox_by_mailbox_type(account_ref->account_id, EMAIL_MAILBOX_TYPE_SPAMBOX, &spam_mailbox, false, &err))
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type for account_id[%d] failed [%d]", account_ref->account_id, err);
		else if (spam_mailbox && spam_mailbox->mailbox_id > 0) {
			if (!emstorage_filter_mails_by_rule(account_ref->account_id, spam_mailbox->mailbox_id, (emstorage_rule_tbl_t *)filter_info, &filtered_mail_id_list, &filtered_mail_id_count, &err))
				EM_DEBUG_EXCEPTION("emstorage_filter_mails_by_rule failed [%d]", err);
			else {
				if (filtered_mail_id_count) {
					parameter_string_length = 10 /*mailbox_id length*/ + 7 + (10 * filtered_mail_id_count);
					parameter_string = em_malloc(sizeof(char) * parameter_string_length);

					if (parameter_string == NULL) {
						err = EMAIL_ERROR_OUT_OF_MEMORY;
						EM_DEBUG_EXCEPTION("em_malloc failed for parameter_string");
						goto FINISH_OFF;
					}
					SNPRINTF(parameter_string, parameter_string_length, "[NA]%c%d%c", 0x01, spam_mailbox->mailbox_id, 0x01);
					
					for (mail_id_index = 0; mail_id_index < filtered_mail_id_count; mail_id_index++) {
						memset(mail_id_string, 0, 10);
						SNPRINTF(mail_id_string, 10, "%d", filtered_mail_id_list[mail_id_index]);
						strcat(parameter_string, mail_id_string);
						strcat(parameter_string, ",");
					}

					EM_DEBUG_LOG("filtered_mail_id_count [%d]", filtered_mail_id_count);
					EM_DEBUG_LOG("param string [%s]", parameter_string);

					if (!emstorage_notify_storage_event(NOTI_MAIL_MOVE, account_ref->account_id, 0, parameter_string, 0)) 
						EM_DEBUG_EXCEPTION("emstorage_notify_storage_event failed [ NOTI_MAIL_MOVE ] >>>> ");

					EM_SAFE_FREE(filtered_mail_id_list);
					EM_SAFE_FREE(parameter_string);
				}
			}
			emstorage_free_mailbox(&spam_mailbox, 1, &err);
			spam_mailbox = NULL;
		}
	}

	emcore_check_unread_mail();

	ret = true;

FINISH_OFF: 
	EM_SAFE_FREE(account_list_ref);
	EM_SAFE_FREE(filtered_mail_id_list);
	EM_SAFE_FREE(parameter_string);


	if (spam_mailbox)
		emstorage_free_mailbox(&spam_mailbox, 1, &err);

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

