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
 * File :  em-core-mesg.c
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
#include <contacts-svc.h>

#include "em-core-types.h"
#include "c-client.h"
#include "lnx_inc.h"
#include "em-core-global.h"
#include "em-core-utils.h"
#include "em-core-mesg.h"
#include "em-core-mime.h"
#include "em-core-mailbox.h"
#include "em-storage.h"
#include "em-core-mailbox-sync.h"
#include "em-core-event.h"
#include "em-core-account.h" 

#include "Msg_Convert.h"
#include "emf-dbglog.h"

#ifdef __FEATURE_DRM__
#include <drm-service.h>
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

static int  em_core_mail_delete_from_server(emf_account_t *account, emf_mailbox_t *mailbox, int msgno, int *err_code);
static int  em_core_mail_cmd_read_mail_pop3(void *stream, int msgno, int limited_size, int *downloded_size, int *total_body_size, int *err_code);
static void em_core_mail_fill_attachment(emf_attachment_info_t *atch , emf_mail_attachment_tbl_t *attachment_tbl);

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


static int pop3_mail_delete(MAILSTREAM *stream, int msgno, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msgno[%d], err_code[%p]", stream, msgno, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	POP3LOCAL *pop3local = NULL;
	char cmd[64];
	char *p = NULL;
	
	if (!stream)  {
		EM_DEBUG_EXCEPTION("stream[%p]", stream);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!(pop3local = stream->local) || !pop3local->netstream) {
		EM_DEBUG_EXCEPTION("invalid POP3 stream detected...");
		err = EMF_ERROR_UNKNOWN;
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
		err = EMF_ERROR_CONNECTION_BROKEN;		/* EMF_ERROR_UNKNOWN; */
		goto FINISH_OFF;
	}
	
	/*  receive response */
	if (!(p = net_getline(pop3local->netstream))) {
		EM_DEBUG_EXCEPTION("net_getline failed...");
		err = EMF_ERROR_INVALID_RESPONSE;
		goto FINISH_OFF;
	}
	
#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG("[POP3] <<< %s", p);
#endif

	if (*p == '-') {		/*  '-ERR' */
		err = EMF_ERROR_POP3_DELE_FAILURE;		/* EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER; */
		goto FINISH_OFF;
	}
	
	if (*p != '+') {		/*  '+OK' ... */
		err = EMF_ERROR_INVALID_RESPONSE;
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

static void em_core_mail_copyuid_ex(MAILSTREAM *stream, char *mailbox, unsigned long uidvalidity, SEARCHSET *sourceset, SEARCHSET *destset)
{
	
	EM_DEBUG_FUNC_BEGIN();

	int index = -1;
	int i = 0;
	int err = EMF_ERROR_NONE;
	
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
		
		EM_DEBUG_LOG("em_core_mail_copyuid_ex failed :  Invalid Parameters--> sourceset[%p] , destset[%p]", sourceset, destset);
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
	old_server_uid = em_core_malloc(count * sizeof(unsigned long));
	new_server_uid = em_core_malloc(count * sizeof(unsigned long));

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
	
		if (!em_storage_update_server_uid(old_server_uid_char, new_server_uid_char, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_update_server_uid failed...[%d]", err);
		}
		
		if (!em_storage_update_read_mail_uid_by_server_uid(old_server_uid_char, new_server_uid_char, mailbox, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_update_read_mail_uid_by_server_uid failed... [%d]", err);
		}
		
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

		if (false == em_storage_update_pbd_activity(old_server_uid_char, new_server_uid_char, mailbox, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_update_pbd_activity failed... [%d]", err);
		}
		
#endif
	}

	EM_SAFE_FREE(old_server_uid);
	EM_SAFE_FREE(new_server_uid);
	
}


EXPORT_API int em_core_mail_move_from_server_ex(int account_id, char *src_mailbox,  int mail_ids[], int num, char *dest_mailbox, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN();
	MAILSTREAM *stream = NULL;
	int err_code = EMF_ERROR_NONE;
	emf_account_t *ref_account = NULL;
	int ret = false;
	int i = 0;
	emf_id_set_t *id_set = NULL;
	int id_set_count = 0;

	emf_uid_range_set *uid_range_set = NULL;
	int len_of_each_range = 0;

	emf_uid_range_set *uid_range_node = NULL;

	char **string_list = NULL;	
	int string_count = 0;
	
	if (num <= 0  || account_id <= 0 || NULL == src_mailbox || NULL == dest_mailbox || NULL == mail_ids)  {
		if (error_code != NULL) {
			*error_code = EMF_ERROR_INVALID_PARAM;
		}
		EM_DEBUG_LOG("Invalid Parameters- num[%d], account_id[%d], src_mailbox[%p], dest_mailbox[%p], mail_ids[%p]", num, account_id, src_mailbox, dest_mailbox, mail_ids);
		return false;
	}

	ref_account = em_core_get_account_reference(account_id);
	
	if (NULL == ref_account) {
		EM_DEBUG_EXCEPTION(" em_core_get_account_reference failed[%d]", account_id);
		
		*error_code = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

 
	if (ref_account->receiving_server_type != EMF_SERVER_TYPE_IMAP4) {
		*error_code = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}


	if (!em_core_mailbox_open(account_id, src_mailbox, (void **)&stream, &err_code))		 {
		EM_DEBUG_EXCEPTION("em_core_mailbox_open failed[%d]", err_code);
		
		goto FINISH_OFF;
	}

	if (NULL != stream) {
		mail_parameters(stream, SET_COPYUID, em_core_mail_copyuid_ex);
		EM_DEBUG_LOG("calling mail_copy_full FODLER MAIL COPY ");
		/*  [h.gahlaut] Break the set of mail_ids into comma separated strings of given length  */
		/*  Length is decided on the basis of remaining keywords in the Query to be formed later on in em_storage_get_id_set_from_mail_ids  */
		/*  Here about 90 bytes are required for fixed keywords in the query-> SELECT local_uid, s_uid from mail_read_mail_uid_tbl where local_uid in (....) ORDER by s_uid  */
		/*  So length of comma separated strings which will be filled in (.....) in above query can be maximum QUERY_SIZE - 90  */
		
		if (false == em_core_form_comma_separated_strings(mail_ids, num, QUERY_SIZE - 90, &string_list, &string_count, &err_code))   {
			EM_DEBUG_EXCEPTION("em_core_form_comma_separated_strings failed [%d]", err_code);
			goto FINISH_OFF;
		}
		
		/*  Now execute one by one each comma separated string of mail_ids */

		for (i = 0; i < string_count; ++i) {
			/*  Get the set of mail_ds and corresponding server_mail_ids sorted by server mail ids in ascending order */
		
			if (false == em_storage_get_id_set_from_mail_ids(string_list[i], &id_set, &id_set_count, &err_code)) {
				EM_DEBUG_EXCEPTION("em_storage_get_id_set_from_mail_ids failed [%d]", err_code);
				goto FINISH_OFF;
			}
			
			/*  Convert the sorted sequence of server mail ids to range sequences of given length. A range sequence will be like A : B, C, D: E, H */
			
			len_of_each_range = MAX_IMAP_COMMAND_LENGTH - 40; /*  1000 is the maximum length allowed RFC 2683. 40 is left for keywords and tag. */
			
			if (false == em_core_convert_to_uid_range_set(id_set, id_set_count, &uid_range_set, len_of_each_range, &err_code)) {
				EM_DEBUG_EXCEPTION("em_core_convert_to_uid_range_set failed [%d]", err_code);
				goto FINISH_OFF;
			}
		
			uid_range_node = uid_range_set;
		
			while (uid_range_node != NULL) {
				/* Remove comma from end of uid_range */
				uid_range_node->uid_range[strlen(uid_range_node->uid_range) - 1] = '\0';
				EM_DEBUG_LOG("uid_range_node->uid_range - %s", uid_range_node->uid_range);
				if (!mail_copy_full(stream, uid_range_node->uid_range, dest_mailbox, CP_UID | CP_MOVE)) {
					EM_DEBUG_EXCEPTION("em_core_mail_move_from_server_ex :   Mail cannot be moved failed");
					EM_DEBUG_EXCEPTION("Mail MOVE failed ");
					goto FINISH_OFF;
				}
				else if (!mail_expunge_full(stream, uid_range_node->uid_range, EX_UID)) {
					EM_DEBUG_EXCEPTION("em_core_mail_move_from_server_ex :   Mail cannot be expunged after move. failed!");
						EM_DEBUG_EXCEPTION("Mail Expunge after move failed ");
						  goto FINISH_OFF;
				}
				else {
					EM_DEBUG_LOG("Mail MOVE SUCCESS ");
				}
				
				uid_range_node = uid_range_node->next;
			}	

			em_core_free_uid_range_set(&uid_range_set);

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
	em_core_mailbox_close(0, stream);
	stream = NULL;

#ifdef __LOCAL_ACTIVITY__
	if (ret || ref_account->receiving_server_type != EMF_SERVER_TYPE_IMAP4) /* Delete local activity for POP3 mails and successful move operation in IMAP */ {
		emf_activity_tbl_t new_activity;
		for (i = 0; i<num ; i++) {		
			memset(&new_activity, 0x00, sizeof(emf_activity_tbl_t));
			new_activity.activity_type = ACTIVITY_MOVEMAIL;
			new_activity.account_id    = account_id;
			new_activity.mail_id	   = mail_ids[i];
			new_activity.src_mbox	   = src_mailbox;
			new_activity.dest_mbox	   = dest_mailbox;
		
			if (!em_core_activity_delete(&new_activity, &err_code)) {
				EM_DEBUG_EXCEPTION(">>>>>>Local Activity ACTIVITY_MOVEMAIL [%d] ", err_code);
			}
		}

	}
#endif

	if (error_code != NULL) {
		*error_code = err_code;
	}

	return ret;
}

#ifdef __LOCAL_ACTIVITY__
int imap4_mail_delete_ex(emf_mailbox_t *mailbox,  int mail_ids[], int num, int from_server, int *err_code)
#else
int imap4_mail_delete_ex(emf_mailbox_t *mailbox,  int mail_ids[], int num, int *err_code)
#endif
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	IMAPLOCAL *imaplocal = NULL;
	char *p = NULL;
	MAILSTREAM *stream = NULL;
	char tag[MAX_TAG_SIZE];
	char cmd[MAX_IMAP_COMMAND_LENGTH];
	emf_id_set_t *id_set = NULL;
	int id_set_count = 0;
	int i = 0;
	emf_uid_range_set *uid_range_set = NULL;
	int len_of_each_range = 0;
	emf_uid_range_set *uid_range_node = NULL;
	char **string_list = NULL;	
	int string_count = 0;
	int delete_success = false;
		
	if (num <= 0 || !mail_ids || !mailbox) {
		EM_DEBUG_EXCEPTION(" Invalid parameter ");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	stream = mailbox->mail_stream;
	if (!stream) {
		EM_DEBUG_EXCEPTION(" Stream is NULL ");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
		
	}

	/*  [h.gahlaut] Break the set of mail_ids into comma separated strings of given length  */
	/*  Length is decided on the basis of remaining keywords in the Query to be formed later on in em_storage_get_id_set_from_mail_ids  */
	/*  Here about 90 bytes are required for fixed keywords in the query-> SELECT local_uid, s_uid from mail_read_mail_uid_tbl where local_uid in (....) ORDER by s_uid  */
	/*  So length of comma separated strings which will be filled in (.....) in above query can be maximum QUERY_SIZE - 90  */

	if (false == em_core_form_comma_separated_strings(mail_ids, num, QUERY_SIZE - 90, &string_list, &string_count, &err))   {
		EM_DEBUG_EXCEPTION("em_core_form_comma_separated_strings failed [%d]", err);
		goto FINISH_OFF;
	}

	/*  Now execute one by one each comma separated string of mail_ids  */
	
	for (i = 0; i < string_count; ++i) {
		/*  Get the set of mail_ds and corresponding server_mail_ids sorted by server mail ids in ascending order  */

		if (false == em_storage_get_id_set_from_mail_ids(string_list[i], &id_set, &id_set_count, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_get_id_set_from_mail_ids failed [%d]", err);
			goto FINISH_OFF;
		}
		
		/*  Convert the sorted sequence of server mail ids to range sequences of given length. A range sequence will be like A : B, C, D: E, H  */
		
		len_of_each_range = MAX_IMAP_COMMAND_LENGTH - 40;		/*   1000 is the maximum length allowed RFC 2683. 40 is left for keywords and tag.  */
		
		if (false == em_core_convert_to_uid_range_set(id_set, id_set_count, &uid_range_set, len_of_each_range, &err)) {
			EM_DEBUG_EXCEPTION("em_core_convert_to_uid_range_set failed [%d]", err);
			goto FINISH_OFF;
		}

		uid_range_node = uid_range_set;

		while (uid_range_node != NULL) {
			/*  Remove comma from end of uid_range  */

			uid_range_node->uid_range[strlen(uid_range_node->uid_range) - 1] = '\0';

			if (!(imaplocal = stream->local) || !imaplocal->netstream)  {
				EM_DEBUG_EXCEPTION("invalid IMAP4 stream detected...");
		
				err = EMF_ERROR_CONNECTION_BROKEN;
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
				
				err = EMF_ERROR_CONNECTION_BROKEN;		/* EMF_ERROR_UNKNOWN */
				goto FINISH_OFF;
			}

			
			while (imaplocal->netstream)  {
				/* receive response */
				if (!(p = net_getline(imaplocal->netstream))) {
					EM_DEBUG_EXCEPTION("net_getline failed...");
					
					err = EMF_ERROR_INVALID_RESPONSE;		/* EMF_ERROR_UNKNOWN; */
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
						err = EMF_ERROR_IMAP4_STORE_FAILURE;		/* EMF_ERROR_INVALID_RESPONSE; */
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
					
					err = EMF_ERROR_CONNECTION_BROKEN;
					goto FINISH_OFF;
				}

				while (imaplocal->netstream)  {
					/* receive response */
					if (!(p = net_getline(imaplocal->netstream))) {
						EM_DEBUG_EXCEPTION("net_getline failed...");
						
						err = EMF_ERROR_INVALID_RESPONSE;		/* EMF_ERROR_UNKNOWN; */
						goto FINISH_OFF;
					}
					
					EM_DEBUG_LOG("[IMAP4] <<< %s", p);
					
					if (!strncmp(p, tag, strlen(tag)))  {
						if (!strncmp(p + strlen(tag) + 1, "OK", 2))  {
#ifdef __LOCAL_ACTIVITY__
							int index = 0;
							emf_mail_tbl_t **mail = NULL;
							
							mail = (emf_mail_tbl_t **) em_core_malloc(num * sizeof(emf_mail_tbl_t *));
							if (!mail) {
								EM_DEBUG_EXCEPTION("em_core_malloc failed");
								err = EMF_ERROR_UNKNOWN;
								goto FINISH_OFF;
							}

							if (delete_success) {
								for (index = 0 ; index < num; index++) {
									if (!em_storage_get_downloaded_mail(mail_ids[index], &mail[index], false, &err))  {
										EM_DEBUG_LOG("em_storage_get_uid_by_mail_id failed [%d]", err);
										
										if (err == EM_STORAGE_ERROR_MAIL_NOT_FOUND)  {		
											EM_DEBUG_LOG("EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER  : ");
											continue;
										}
									}		
								
									if (mail[index]) {				
										/* Clear entry from mail_read_mail_uid_tbl */
										if (mail[index]->server_mail_id != NULL) {
											if (!em_storage_remove_downloaded_mail(mailbox->account_id, mailbox->name, mail[index]->server_mail_id, true, &err))  {
												EM_DEBUG_LOG("em_storage_remove_downloaded_mail falied [%d]", err);
											}
										}
										
										/* Delete local activity */
										emf_activity_tbl_t new_activity;
										memset(&new_activity, 0x00, sizeof(emf_activity_tbl_t));
										if (from_server == EMF_DELETE_FOR_SEND_THREAD) {
											new_activity.activity_type = ACTIVITY_DELETEMAIL_SEND;
											EM_DEBUG_LOG("from_server == EMF_DELETE_FOR_SEND_THREAD ");
										}
										else {
											new_activity.activity_type	= ACTIVITY_DELETEMAIL;
										}

										new_activity.mail_id		= mail[index]->mail_id;
										new_activity.server_mailid	= NULL;
										new_activity.src_mbox		= NULL;
										new_activity.dest_mbox		= NULL;
														
										if (!em_core_activity_delete(&new_activity, &err)) {
											EM_DEBUG_EXCEPTION(" em_core_activity_delete  failed  - %d ", err);
										}
									}
									else {
										/* Fix for crash seen while deleting Outbox mails which are moved to Trash. Outbox mails do not have server mail id and are not updated in mail_read_mail_uid_tbl.
										  * So there is no need of deleting entry in mail_read_mail_uid_tbl. However local activity has to be deleted. 
										*/
										/* Delete local activity */
										emf_activity_tbl_t new_activity;
										memset(&new_activity, 0x00, sizeof(emf_activity_tbl_t));
										new_activity.activity_type	= ACTIVITY_DELETEMAIL;
										new_activity.mail_id		= mail_ids[index]; /* valid mail id passed for outbox mails*/
										new_activity.server_mailid	= NULL;
										new_activity.src_mbox		= NULL;
										new_activity.dest_mbox		= NULL;
														
										if (!em_core_activity_delete(&new_activity, &err)) {
											EM_DEBUG_EXCEPTION(" em_core_activity_delete  failed  - %d ", err);
										}
									}
								}

								for (index = 0; index < num; index++) {
									if (!em_storage_free_mail(&mail[index], 1, &err)) {
										EM_DEBUG_EXCEPTION(" em_storage_free_mail [%d]", err);
									}
								}

								EM_SAFE_FREE(mail);
							}

#endif
	EM_SAFE_FREE(p);
							break;
						}
						else  {		/*  'NO' or 'BAD' */
							err = EMF_ERROR_IMAP4_EXPUNGE_FAILURE;		/* EMF_ERROR_INVALID_RESPONSE; */
							goto FINISH_OFF;
						}
					}
					
					EM_SAFE_FREE(p);
				}
				uid_range_node = uid_range_node->next;
		}	

		em_core_free_uid_range_set(&uid_range_set);

		EM_SAFE_FREE(id_set);
		
		id_set_count = 0;
	}
	
	ret = true;

FINISH_OFF: 
	EM_SAFE_FREE(p);
	
	em_core_free_comma_separated_strings(&string_list, &string_count);

	if (false == ret) {
		em_core_free_uid_range_set(&uid_range_set);
	}
	
	if (err_code) {
		*err_code = err;
	}

	return ret;

}

#endif

static int imap4_mail_delete(MAILSTREAM *stream, int msgno, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msgno[%d], err_code[%p]", stream, msgno, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	IMAPLOCAL *imaplocal = NULL;
	char tag[16], cmd[64];
	char *p = NULL;
	
	if (!(imaplocal = stream->local) || !imaplocal->netstream) {
		EM_DEBUG_EXCEPTION("invalid IMAP4 stream detected...");
		
		err = EMF_ERROR_INVALID_PARAM;		/* EMF_ERROR_UNKNOWN */
		goto FINISH_OFF;
	}
	
	memset(tag, 0x00, sizeof(tag));
	memset(cmd, 0x00, sizeof(cmd));
	
	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
	SNPRINTF(cmd, sizeof(cmd), "%s STORE %d +FLAGS.SILENT (\\Deleted)\015\012", tag, msgno);

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG("[IMAP4] >>> %s", cmd);
#endif
	
	/*  send command  :  set deleted flag */
	if (!net_sout(imaplocal->netstream, cmd, (int)strlen(cmd))) {
		EM_DEBUG_EXCEPTION("net_sout failed...");
		
		err = EMF_ERROR_CONNECTION_BROKEN;		/* EMF_ERROR_UNKNOWN */
		goto FINISH_OFF;
	}
	
	while (imaplocal->netstream) {
		/*  receive response */
		if (!(p = net_getline(imaplocal->netstream))) {
			EM_DEBUG_EXCEPTION("net_getline failed...");
			
			err = EMF_ERROR_INVALID_RESPONSE;		/* EMF_ERROR_UNKNOWN; */
			goto FINISH_OFF;
		}
		
#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG("[IMAP4] <<< %s", p);
#endif		
		if (!strncmp(p, tag, strlen(tag))) {
			if (!strncmp(p + strlen(tag) + 1, "OK", 2))  {
				EM_SAFE_FREE(p);
				break;
			}
			else  {		/*  'NO' or 'BAD' */
				err = EMF_ERROR_IMAP4_STORE_FAILURE;		/* EMF_ERROR_INVALID_RESPONSE; */
				goto FINISH_OFF;
			}
		}
		
		EM_SAFE_FREE(p);
	}
	
	memset(tag, 0x00, sizeof(tag));
	memset(cmd, 0x00, sizeof(cmd));
	
	SNPRINTF(tag, sizeof(tag), "%08lx", 0xffffffff & (stream->gensym++));
	SNPRINTF(cmd, sizeof(cmd), "%s EXPUNGE\015\012", tag);

#ifdef FEATURE_CORE_DEBUG
	EM_DEBUG_LOG("[IMAP4] >>> %s", cmd);
#endif
	
	/*  send command  :  delete flaged mail */
	if (!net_sout(imaplocal->netstream, cmd, (int)strlen(cmd)))  {
		EM_DEBUG_EXCEPTION("net_sout failed...");
		
		err = EMF_ERROR_CONNECTION_BROKEN;
		goto FINISH_OFF;
	}
	
	while (imaplocal->netstream)  {
		/*  receive response */
		if (!(p = net_getline(imaplocal->netstream)))  {
			EM_DEBUG_EXCEPTION("net_getline failed...");
			
			err = EMF_ERROR_INVALID_RESPONSE;		/* EMF_ERROR_UNKNOWN; */
			goto FINISH_OFF;
		}
		
#ifdef FEATURE_CORE_DEBUG
		EM_DEBUG_LOG("[IMAP4] <<< %s", p);
#endif
		
		if (!strncmp(p, tag, strlen(tag)))  {
			if (!strncmp(p + strlen(tag) + 1, "OK", 2))  {
				EM_SAFE_FREE(p);
				break;
			}
			else  {		/*  'NO' or 'BAD' */
				err = EMF_ERROR_IMAP4_EXPUNGE_FAILURE;		/* EMF_ERROR_INVALID_RESPONSE; */
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

typedef enum {
	IMAP4_CMD_EXPUNGE
} imap4_cmd_t;

static int imap4_send_command(MAILSTREAM *stream, imap4_cmd_t cmd_type, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], cmd_type[%d], err_code[%p]", stream, cmd_type, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	IMAPLOCAL *imaplocal = NULL;
	char tag[16], cmd[64];
	char *p = NULL;
	
	if (!(imaplocal = stream->local) || !imaplocal->netstream) {
		EM_DEBUG_EXCEPTION("invalid IMAP4 stream detected...");
		
		err = EMF_ERROR_INVALID_PARAM;		/* EMF_ERROR_UNKNOWN */
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
		
		err = EMF_ERROR_CONNECTION_BROKEN;
		goto FINISH_OFF;
	}
	
	while (imaplocal->netstream) {
		/*  receive response */
		if (!(p = net_getline(imaplocal->netstream))) {
			EM_DEBUG_EXCEPTION("net_getline failed...");
			
			err = EMF_ERROR_INVALID_RESPONSE;		/* EMF_ERROR_UNKNOWN; */
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
				err = EMF_ERROR_IMAP4_EXPUNGE_FAILURE;		/* EMF_ERROR_INVALID_RESPONSE; */
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
	


/* description 
 *    get mail header from local mailbox
 * arguments  
 *    mailbox  :  message no
 *    header  :  buffer to hold header information
 * return  
 *    succeed  :  1
 *    fail  :  0
 */
int em_core_mail_get_header(/*emf_mailbox_t *mailbox, */ int mail_id, emf_mail_head_t **header, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], header[%p], err_code[%p]", mail_id, header, err_code);
	
	if (!mail_id || !header)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], header[%p]", mail_id, header);
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	emf_mail_head_t *p = NULL;
	emf_mail_tbl_t *mail = NULL;
	
	*header = NULL;
	
	/* get mail from mail table */
	if (!em_storage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	if (!em_core_mail_get_header_from_mail_tbl(&p, mail, &err))  {
		EM_DEBUG_EXCEPTION("em_core_mail_get_header_from_mail_tbl failed [%d]", err);		
		goto FINISH_OFF;
	}
	
	*header = p;
	
	ret = true;
	
FINISH_OFF: 
	if (ret == false && p != NULL)
		em_core_mail_head_free(&p, 1, NULL);
	
	if (mail != NULL)
		em_storage_free_mail(&mail, 1, NULL);
	
	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}
EXPORT_API int em_core_mail_get_header_from_mail_tbl(emf_mail_head_t **header, emf_mail_tbl_t *mail , int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail[%p], header[%p], err_code[%p]", mail, header, err_code);
	
	if (!mail || !header)  {
		EM_DEBUG_EXCEPTION("mail[%p], header[%p]", mail, header);
		if (err_code != NULL)		
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	emf_mail_head_t *p = NULL;

	if (!(p = em_core_malloc(sizeof(emf_mail_head_t))))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(p, 0x00, sizeof(emf_mail_head_t));
	
	p->subject = mail->subject; mail->subject = NULL;
	p->mid = mail->message_id; mail->message_id = NULL;
	p->to_contact_name =  NULL;
	p->from_contact_name = NULL;
	p->cc_contact_name = NULL;
	p->bcc_contact_name = NULL;
	
	EM_DEBUG_LOG("to_contact_name [%s]", p->to_contact_name);
	EM_DEBUG_LOG("from_contact_name [%s]", p->from_contact_name);

	p->from = mail->full_address_from; mail->full_address_from = NULL;
	p->to = mail->full_address_to; mail->full_address_to = NULL;
	p->cc = mail->full_address_cc; mail->full_address_cc = NULL;
	p->bcc = mail->full_address_bcc; mail->full_address_bcc = NULL;
	p->reply_to = mail->full_address_reply; mail->full_address_reply = NULL;
	p->return_path = mail->full_address_return; mail->full_address_return = NULL;
	p->previewBodyText = mail->preview_text; mail->preview_text = NULL;

	/* mail received time */
	if (!em_convert_string_to_datetime(mail->datetime, &p->datetime, &err))  {
		EM_DEBUG_EXCEPTION("em_convert_string_to_datetime failed [%d]", err);
		/* goto FINISH_OFF; */
	}
	ret = true;
	
FINISH_OFF: 
	if (ret == true)
		*header = p;
	else if (p != NULL) 
		em_core_mail_head_free(&p, 1, NULL);
	
	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}

int em_core_mail_get_contact_info(emf_mail_contact_info_t *contact_info, char *full_address, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("contact_info[%p], full_address[%s], err_code[%p]", contact_info, full_address, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!em_core_mail_get_contact_info_with_update(contact_info, full_address, 0, &err)) 
		EM_DEBUG_EXCEPTION("em_core_mail_get_contact_info_with_update failed [%d]", err);
	else
		ret = true;
	
	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}

EXPORT_API int em_core_mail_get_display_name(CTSvalue *contact_name_value, char **contact_display_name)
{
	EM_DEBUG_FUNC_BEGIN("contact_name_value[%p], contact_display_name[%p]", contact_name_value, contact_display_name);
	char *display = NULL;
	const char *first = contacts_svc_value_get_str(contact_name_value, CTS_NAME_VAL_FIRST_STR);
	const char *last = contacts_svc_value_get_str(contact_name_value, CTS_NAME_VAL_LAST_STR);

	EM_DEBUG_LOG(">>>>>> first[%s] last[%s]", first, last);
	if (first != NULL && last != NULL) {
		/* if (CTS_ORDER_NAME_FIRSTLAST  == contacts_svc_get_name_order()) */
		if (CTS_ORDER_NAME_FIRSTLAST == contacts_svc_get_order(CTS_ORDER_OF_DISPLAY))
			display = g_strconcat(first, " ", last, NULL);
		else
			display = g_strconcat(last, " ", first, NULL);

	}
	else if (first != NULL || last != NULL) {
		if (first != NULL)
			display = (char *)EM_SAFE_STRDUP(first);
		else
			display = (char *)EM_SAFE_STRDUP(last);
	}
	else
		display = g_strdup(contacts_svc_value_get_str(contact_name_value, CTS_NAME_VAL_DISPLAY_STR));

	if (contact_display_name != NULL)
		*contact_display_name = display;

	return true;
}

int em_core_mail_get_contact_info_with_update(emf_mail_contact_info_t *contact_info, char *full_address, int mail_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("contact_info[%p], full_address[%s], mail_id[%d], err_code[%p]", contact_info, full_address, mail_id, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	ADDRESS *addr = NULL;
	char *address = NULL;
	char *temp_emailaddr = NULL;
	int start_text_ascii = 2;
	int end_text_ascii = 3;	

	char *alias = NULL;
	int is_searched = false;
	int address_length = 0;

	if (!contact_info)  {
		EM_DEBUG_EXCEPTION("contact_info[%p]", contact_info);
		err = EMF_ERROR_INVALID_PARAM;
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

    em_core_skip_whitespace(full_address , &address);
	EM_DEBUG_LOG("address[address][%s]", address);  


	/*  ',' -> "%2C" */
	gchar **tokens = g_strsplit(address, ", ", -1);
	char *p = g_strjoinv("%2C", tokens);
	int i = 0;
	
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
		
		err = EMF_ERROR_INVALID_ADDRESS;
		goto FINISH_OFF;
	}
	
	CTSstruct *contact =  NULL;
	CTSvalue *contact_name_value = NULL;
	int contact_index = -1;
	char *contact_display_name = NULL;
	char *contact_display_name_from_contact_info = NULL;
	int contact_display_name_len = 0;
	
	/* char **/
	char *email_address = NULL;
	int contact_name_buffer_size = address_length;
	int contact_name_len = 0;
	char *contact_name = (char  *)calloc(1, contact_name_buffer_size); 
	char temp_string[1024];
	
	int is_saved = 0;
	
	if (!contact_name) {
		EM_DEBUG_EXCEPTION("Memory allocation error!");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(contact_name, 0x00, contact_name_buffer_size);   
	
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
		EM_DEBUG_LOG(" >>>>> em_core_mail_get_contact_info - 10");
			
		err = contacts_svc_find_contact_by(CTS_FIND_BY_EMAIL, email_address);
		if (err > CTS_SUCCESS) {
			contact_index = err;
			if ((err = contacts_svc_get_contact(contact_index, &contact)) == CTS_SUCCESS) {
				/*  get  contact name */
				if (contacts_svc_struct_get_value(contact, CTS_CF_NAME_VALUE, &contact_name_value) == CTS_SUCCESS) {	/*  set contact display name name */
					contact_info->contact_id = contact_index;		/*  NOTE :  This is valid only if there is only one address. */
					em_core_mail_get_display_name(contact_name_value, &contact_display_name_from_contact_info);

					contact_display_name = contact_display_name_from_contact_info;

					
					EM_DEBUG_LOG(">>> contact_index[%d]", contact_index);
					EM_DEBUG_LOG(">>> contact_name[%s]", contact_display_name);

					/*  Make display name string */
					if (contact_display_name != NULL) {
						is_searched = true;

						if (mail_id == 0 || (contact_name_len == 0))		 {	/*  save only the first address information - 09-SEP-2010 */
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
									err = EMF_ERROR_OUT_OF_MEMORY;
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
				}
				else  {
						EM_DEBUG_LOG("contacts_svc_struct_get_value error[%d]", err);
				}
			}
			else {
				EM_DEBUG_LOG("contacts_svc_get_contact error [%d]", err);
			}
		}
		else {
			EM_DEBUG_LOG("contacts_svc_find_contact_by - Not found contact record(if err is 203) or error [%d]", err);
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
						err = EMF_ERROR_OUT_OF_MEMORY;
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

		if (contact != NULL) {
			contacts_svc_struct_free(contact);
			contact = NULL;
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

	if (contact != NULL)
		contacts_svc_struct_free(contact);
	EM_SAFE_FREE(email_address);
	EM_SAFE_FREE(address);
	EM_SAFE_FREE(temp_emailaddr);
	EM_SAFE_FREE(contact_name);		
	EM_SAFE_FREE(contact_display_name_from_contact_info);
	
	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}

int em_core_mail_free_contact_info(emf_mail_contact_info_t *contact_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("contact_info[%p], err_code[%p]", contact_info, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!contact_info)  {
		EM_DEBUG_EXCEPTION("contact_info[%p]", contact_info);
		err = EMF_ERROR_INVALID_PARAM;
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

int em_core_mail_contact_sync(int mail_id, int *err_code)
{
	EM_PROFILE_BEGIN(emCoreMailContactSync);
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int err = EMF_ERROR_NONE;

	emf_mail_tbl_t *mail = NULL;

	emf_mail_contact_info_t contact_info_from;
	emf_mail_contact_info_t contact_info_to;
	emf_mail_contact_info_t contact_info_cc;
	emf_mail_contact_info_t contact_info_bcc;
	
	EM_DEBUG_LOG("mail_id[%d], err_code[%p]", mail_id, err_code);
	
	memset(&contact_info_from, 0x00, sizeof(emf_mail_contact_info_t));	
	memset(&contact_info_to, 0x00, sizeof(emf_mail_contact_info_t));
	memset(&contact_info_cc, 0x00, sizeof(emf_mail_contact_info_t));
	memset(&contact_info_bcc, 0x00, sizeof(emf_mail_contact_info_t));	

	if (!em_storage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	if (mail->full_address_from != NULL) {
		if (!em_core_mail_get_contact_info_with_update(&contact_info_from, mail->full_address_from, mail_id, &err))  {
			EM_DEBUG_EXCEPTION("em_core_mail_get_contact_info failed [%d]", err);
		}
	}

	if (mail->full_address_to != NULL)  {
		if (!em_core_mail_get_contact_info_with_update(&contact_info_to, mail->full_address_to, mail_id, &err))  {
			EM_DEBUG_EXCEPTION("em_core_mail_get_contact_info failed [%d]", err);
		}
	}

	if (mail->full_address_cc != NULL)  {
		if (!em_core_mail_get_contact_info_with_update(&contact_info_cc, mail->full_address_cc, mail_id, &err))  {
			EM_DEBUG_EXCEPTION("em_core_mail_get_contact_info failed [%d]", err);
		}
	}

	if (mail->full_address_bcc != NULL)  {
		if (!em_core_mail_get_contact_info_with_update(&contact_info_bcc, mail->full_address_bcc, mail_id, &err))  {
			EM_DEBUG_EXCEPTION("em_core_mail_get_contact_info failed [%d]", err);
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
	if (!em_storage_change_mail_field(mail_id, UPDATE_ALL_CONTACT_INFO, mail, false, &err))  {				  	
		EM_DEBUG_EXCEPTION("em_storage_change_mail_field failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}	

	ret = true;

FINISH_OFF: 
	if (mail != NULL)
		em_storage_free_mail(&mail, 1, NULL);

	em_core_mail_free_contact_info(&contact_info_from, NULL);
	em_core_mail_free_contact_info(&contact_info_to, NULL);
	em_core_mail_free_contact_info(&contact_info_cc, NULL);
	em_core_mail_free_contact_info(&contact_info_bcc, NULL);	

	if (err_code != NULL)
		*err_code = err;

	EM_PROFILE_END(emCoreMailContactSync);
	return ret;
}

/*  1. parsing  :  alias and address */
/*  2. sync with contact */
/*  3. make glist of address info */
static int em_core_sync_address_info(emf_address_type_t address_type, char *full_address, GList **address_info_list, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("address type[%d], address_info_list[%p], full_address[%p]", address_type, address_info_list, full_address);

	int ret = false;
	int error = EMF_ERROR_NONE;
	int contact_index = -1;
	int is_search = false;
	char *alias = NULL;
	char *address = NULL;
	char *contact_display_name_from_contact_info = NULL;
	char email_address[MAX_EMAIL_ADDRESS_LENGTH];
	emf_address_info_t *p_address_info = NULL;
	ADDRESS *addr = NULL;
	CTSstruct *contact =  NULL;
	CTSvalue *contact_name_value = NULL;
	
	if (full_address == NULL || address_info_list == NULL) {
		EM_DEBUG_EXCEPTION("Invalid param :  full_address or address_info_list is NULL");
		error = EMF_ERROR_INVALID_PARAM;
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
		error = EM_STORAGE_ERROR_INVALID_PARAM;		
		goto FINISH_OFF;
	}

	/*  Get a contact name */
 	while (addr != NULL)  {
		if (addr->mailbox && addr->host) {	
			if (!strcmp(addr->mailbox , "UNEXPECTED_DATA_AFTER_ADDRESS") || !strcmp(addr->mailbox , "INVALID_ADDRESS") || !strcmp(addr->host , ".SYNTAX-ERROR."))
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

		/*   set display name  */
		/*    1) contact name */
		/*    2) alias (if a alias in an original mail doesn't exist, this field is set with email address */
		/*    3) email address	 */

		if (!(p_address_info = (emf_address_info_t *)malloc(sizeof(emf_address_info_t)))) {
			EM_DEBUG_EXCEPTION("malloc failed...");
			error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}	
		memset(p_address_info, 0x00, sizeof(emf_address_info_t)); 

		SNPRINTF(email_address, MAX_EMAIL_ADDRESS_LENGTH, "%s@%s", addr->mailbox ? addr->mailbox  :  "", addr->host ? addr->host  :  "");
 
		EM_DEBUG_LOG("Search a contact  :  address[%s]", email_address);

		is_search = false;

		error = contacts_svc_find_contact_by(CTS_FIND_BY_EMAIL, email_address);
		if (error > CTS_SUCCESS) {
			contact_index = error;
			if ((error = contacts_svc_get_contact(contact_index, &contact)) == CTS_SUCCESS) {
				/*  get  contact name */
				if (contacts_svc_struct_get_value(contact, CTS_CF_NAME_VALUE, &contact_name_value) == CTS_SUCCESS) {	/*  set contact display name name */
					em_core_mail_get_display_name(contact_name_value, &contact_display_name_from_contact_info);
					EM_DEBUG_LOG(">>> contact index[%d]", contact_index);
					EM_DEBUG_LOG(">>> contact display name[%s]", contact_display_name_from_contact_info);

					is_search = true;
				}
				else
					EM_DEBUG_EXCEPTION("contacts_svc_struct_get_value error[%d]", error);
			}
			else
				EM_DEBUG_EXCEPTION("contacts_svc_get_contact error [%d]", error);
		}
		else
			EM_DEBUG_EXCEPTION("contacts_svc_find_contact_by - Not found contact record(if err is -203) or error [%d]", error);

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
		if (contact != NULL) {
			contacts_svc_struct_free(contact);
			contact = NULL;
		}

		EM_DEBUG_LOG("next address[%p]", addr->next);

		/*  next address */
		addr = addr->next;
	}

	ret = true;
	
FINISH_OFF: 
	if (contact != NULL)
		contacts_svc_struct_free(contact);

 	EM_SAFE_FREE(address);
	
	if (err_code != NULL)
		*err_code = error;	
	EM_DEBUG_FUNC_END();
	return ret;
}

static gint address_compare(gconstpointer a, gconstpointer b)
{
	EM_DEBUG_FUNC_BEGIN();
	emf_sender_list_t *recipients_list1 = (emf_sender_list_t *)a;
	emf_sender_list_t *recipients_list2 = (emf_sender_list_t *)b;	

	EM_DEBUG_FUNC_END();	
	return strcmp(recipients_list1->address, recipients_list2->address);
}

EXPORT_API GList *em_core_get_recipients_list(GList *old_recipients_list, char *full_address, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int i = 0, err;
	int contact_index = -1;
	int is_search = false;
	
	char *address = NULL;
	char email_address[MAX_EMAIL_ADDRESS_LENGTH];
	char *display_name = NULL;
	char *alias = NULL;
	ADDRESS *addr = NULL;
	CTSstruct *contact = NULL;
	CTSvalue *contact_name_value = NULL;
	GList *new_recipients_list = old_recipients_list;
	GList *recipients_list;

	emf_sender_list_t *temp_recipients_list = NULL;
	emf_sender_list_t *old_recipients_list_t = NULL;
	
	if (full_address == NULL || strlen(full_address) == 0) {
		EM_DEBUG_EXCEPTION("Invalid param : full_address NULL or empty");
		err = EMF_ERROR_INVALID_PARAM;
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
		err = EM_STORAGE_ERROR_INVALID_PARAM;
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
	
		temp_recipients_list = g_new0(emf_sender_list_t, 1);
		
		SNPRINTF(email_address, MAX_EMAIL_ADDRESS_LENGTH, "%s@%s", addr->mailbox ? addr->mailbox : "", addr->host ? addr->host : "");

		EM_DEBUG_LOG("Search a contact : address[%s]", email_address);

		err = contacts_svc_find_contact_by(CTS_FIND_BY_EMAIL, email_address);
		if (err > CTS_SUCCESS) {
			contact_index = err;
			if ((err = contacts_svc_get_contact(contact_index, &contact)) == CTS_SUCCESS) {
				if (contacts_svc_struct_get_value(contact, CTS_CF_NAME_VALUE, &contact_name_value) == CTS_SUCCESS) {
					em_core_mail_get_display_name(contact_name_value, &display_name);
					EM_DEBUG_LOG(">>> contact index[%d]", contact_index);
					EM_DEBUG_LOG(">>> contact display name[%s]", display_name);

					is_search = true;
				} else {
						EM_DEBUG_LOG("contacts_svc_struct_get_value error[%d]", err);
				}
			} else {
				EM_DEBUG_LOG("contacts_svc_get_contact error [%d]", err);
			}
		} else {
			EM_DEBUG_LOG("contacts_svc_find_contact_by - Not found contact record(if err is -203) or error [%d]", err);
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
		if (contact != NULL) {
			contacts_svc_struct_free(contact);
			contact = NULL;
		}
		EM_DEBUG_LOG("next address[%p]", addr->next);

		recipients_list = g_list_first(new_recipients_list);
		while (recipients_list != NULL) {
			old_recipients_list_t = (emf_sender_list_t *)recipients_list->data;
			if (!strcmp(old_recipients_list_t->address, temp_recipients_list->address)) {
				old_recipients_list_t->total_count = old_recipients_list_t->total_count + 1;
				if (temp_recipients_list != NULL)
					g_free(temp_recipients_list);
				
				goto FINISH_OFF;
			}
			recipients_list = g_list_next(recipients_list);
		}

		new_recipients_list = g_list_insert_sorted(new_recipients_list, temp_recipients_list, address_compare);

		temp_recipients_list = NULL;

		alias = NULL;
		if (contact != NULL) {
			contacts_svc_struct_free(contact);
			contact = NULL;
		}
		addr = addr->next;
	}

FINISH_OFF:

	if (contact != NULL)
		contacts_svc_struct_free(contact);

	EM_SAFE_FREE(address);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return new_recipients_list;			
}

EXPORT_API int em_core_mail_get_address_info_list(int mail_id, emf_address_info_list_t **address_info_list, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], address_info_list[%p]", mail_id, address_info_list);

	int ret = false, err = EMF_ERROR_NONE;
	int failed = true;
	int contact_error;

	emf_mail_tbl_t *mail = NULL;
	emf_address_info_list_t *p_address_info_list = NULL;

	if (mail_id <= 0 || !address_info_list) {
		EM_DEBUG_EXCEPTION("mail_id[%d], address_info_list[%p]", mail_id, address_info_list);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	/* get mail from mail table */
	if (!em_storage_get_mail_by_id(mail_id, &mail, true, &err) || !mail) {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
		
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	if (!(p_address_info_list = (emf_address_info_list_t *)malloc(sizeof(emf_address_info_list_t)))) {
		EM_DEBUG_EXCEPTION("malloc failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}	
	memset(p_address_info_list, 0x00, sizeof(emf_address_info_list_t)); 	

	if ((contact_error = contacts_svc_connect()) == CTS_SUCCESS)	 {		
		EM_DEBUG_LOG("Open Contact Service Success");	
	}	
	else	 {		
		EM_DEBUG_EXCEPTION("contact_db_service_connect failed [%d]", contact_error);		
		err = EMF_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	if (em_core_sync_address_info(EMF_ADDRESS_TYPE_FROM, mail->full_address_from, &p_address_info_list->from, &err))
		failed = false;
	if (em_core_sync_address_info(EMF_ADDRESS_TYPE_TO, mail->full_address_to, &p_address_info_list->to, &err))
		failed = false;
	if (em_core_sync_address_info(EMF_ADDRESS_TYPE_CC, mail->full_address_cc, &p_address_info_list->cc, &err))
		failed = false;
	if (em_core_sync_address_info(EMF_ADDRESS_TYPE_BCC, mail->full_address_bcc, &p_address_info_list->bcc, &err))
		failed = false;

	if ((contact_error = contacts_svc_disconnect()) == CTS_SUCCESS)
		EM_DEBUG_LOG("Close Contact Service Success");
	else
		EM_DEBUG_EXCEPTION("Close Contact Service Fail [%d]", contact_error);

	if (failed == false)
		ret = true;

FINISH_OFF: 
	if (ret == true) 
		*address_info_list = p_address_info_list;
	else if (p_address_info_list != NULL)
		em_storage_free_address_info_list(&p_address_info_list);

	if (!mail)
		em_storage_free_mail(&mail, 1, NULL);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

	

/* description 
 *    get mail header from local mailbox
 * arguments  
 *    mailbox  :  message no
 *    header  :  buffer to hold header information
 * return  
 *    succeed  :  1
 *    fail  :  0
 */
int em_core_mail_get_info(int mail_id, emf_mail_info_t **info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], info[%p], err_code[%p]", mail_id, info, err_code);
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	
	emf_mail_info_t *temp_mail_info = NULL;
	emf_mail_tbl_t *mail = NULL;
	
	if (!mail_id || !info)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], info[%p]", mail_id, info);
		error = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	/* get mail from mail table */
	if (!em_storage_get_mail_by_id(mail_id, &mail, true, &error) || !mail)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", error);
		error = em_storage_get_emf_error_from_em_storage_error(error);
		goto FINISH_OFF;
	}
	
	if (!em_core_mail_get_info_from_mail_tbl(&temp_mail_info, mail, &error))  {
		EM_DEBUG_EXCEPTION("em_core_mail_get_info_from_mail_tbl failed [%d]", error);		
		goto FINISH_OFF;
	}	

	ret = true;
	
FINISH_OFF: 
	if (ret == true)
		*info = temp_mail_info;
	else if (temp_mail_info != NULL) 
		em_core_mail_info_free(&temp_mail_info, 1, NULL);
	
	if (mail != NULL)
		em_storage_free_mail(&mail, 1, NULL);
	
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int em_core_mail_get_info_from_mail_tbl(emf_mail_info_t **pp_mail_info, emf_mail_tbl_t *mail_tbl_data , int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("pp_mail_info[%p], mail_tbl_data[%p], err_code[%p]", pp_mail_info, mail_tbl_data, err_code);
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	emf_mail_info_t *p = NULL;

	if (!pp_mail_info || !mail_tbl_data)  {
		EM_DEBUG_EXCEPTION("pp_mail_info[%p], mail_tbl_data[%p]", pp_mail_info, mail_tbl_data);
		error = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* memory allocate */
	if (!(p = em_core_malloc(sizeof(emf_mail_info_t))))  {
		EM_DEBUG_EXCEPTION("em_core_malloc failed...");
		error = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	p->account_id = mail_tbl_data->account_id;
	p->uid        = mail_tbl_data->mail_id;
	p->sid        = EM_SAFE_STRDUP(mail_tbl_data->server_mail_id);
	
	/* mail_tbl_data download status */
	if (mail_tbl_data->server_mail_status == 0 || mail_tbl_data->body_download_status == EMF_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED)  {	
		p->body_downloaded = EMF_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;
		p->extra_flags.text_download_yn = EMF_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;
	} 
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
	else if (mail_tbl_data->body_download_status == EMF_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED) {
		p->body_downloaded = EMF_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED;
		p->extra_flags.text_download_yn = EMF_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED;
	}
#endif
	else  {
		p->body_downloaded = EMF_BODY_DOWNLOAD_STATUS_NONE; 
		p->extra_flags.text_download_yn = EMF_BODY_DOWNLOAD_STATUS_NONE;
	}
	
	/*  mail_tbl_data rfc822.size */
	p->rfc822_size = mail_tbl_data->mail_size;
	
	/*  mail_tbl_data flags  */
	if (!em_convert_mail_tbl_to_mail_flag(mail_tbl_data, &p->flags, &error))  {
		EM_DEBUG_EXCEPTION("em_convert_mail_tbl_to_mail_flag failed [%d]", error);
		goto FINISH_OFF;
	}

	/*  mail_tbl_data extra flag */
	p->extra_flags.lock     = mail_tbl_data->lock_status;
	p->extra_flags.priority = mail_tbl_data->priority;
	p->extra_flags.report   = mail_tbl_data->report_status;
	p->extra_flags.status   = mail_tbl_data->save_status;
	p->flags.sticky         = mail_tbl_data->lock_status;
	p->extra_flags.drm      = mail_tbl_data->DRM_status;
	p->is_meeting_request   = mail_tbl_data->meeting_request_status;
	ret = true;
	
FINISH_OFF: 
	if (ret == true)
		*pp_mail_info = p;
	else if (p != NULL) 
		em_core_mail_info_free(&p, 1, NULL);
	
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

/* description
 *    get a mail body from local mailbox
 *    mail body contain body text and attachment list.
 * arguments  
 *    mailbox  :  mail box
 *    msgno  :  mail no
 *    mail  :  [out] double pointer to hold mail data. (mail info, mail header, mail body text and attachment list)
 * return  
 *    succeed  :  1
 *    fail  :  0
 */
EXPORT_API int em_core_mail_get_mail(int mail_id, emf_mail_t **mail, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], mail[%p], err_code[%p]", mail_id, mail, err_code);
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	
	emf_mail_t *p = NULL;
	
	if (!mail_id || !mail)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], mail[%p]", mail_id, mail);
		error = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	/* memory allocate */
	if (!(p = em_core_malloc(sizeof(emf_mail_t))))  {
		EM_DEBUG_EXCEPTION("malloc falied...");
		error = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memset(p, 0x00, sizeof(emf_mail_t));
	
	/* get mail info */
	if (!em_core_mail_get_info(mail_id, &p->info, &error))  {
		EM_DEBUG_EXCEPTION("em_core_mail_get_info failed [%d]", error);
		goto FINISH_OFF;
	}
	
	/* get mail header */
	if (!em_core_mail_get_header(mail_id, &p->head, &error))  {
		EM_DEBUG_EXCEPTION("em_core_mail_get_header failed [%d]", error);
		goto FINISH_OFF;
	}
	
	/* if body downloaded, get mail body */
	EM_DEBUG_LOG("p->info->body_downloaded [%d]", p->info->body_downloaded);
	
	/* if (p->info->body_downloaded)  { */
		if (!em_core_mail_get_body(mail_id, &p->body, &error))  {
			EM_DEBUG_EXCEPTION("em_core_mail_get_body failed [%d]", error);
			goto FINISH_OFF;
		}
	/* } */
	
	ret = true;
	
FINISH_OFF: 
	if (ret == true)
		*mail = p;
	else if (p != NULL)
		em_core_mail_free(&p, 1, NULL);
	
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

/* description
 *    get a mail data
 * arguments  
 *    input_mail_id     : [in]  mail id
 *    output_mail_data  : [out] double pointer to hold mail data.
 * return  
 *    succeed  :  EMF_ERROR_NONE
 *    fail     :  error code
 */
EXPORT_API int em_core_get_mail_data(int input_mail_id, emf_mail_data_t **output_mail_data)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], output_mail_data[%p]", input_mail_id, output_mail_data);
	
	int             error = EMF_ERROR_NONE;
	int             result_mail_count = 0;
	char            conditional_clause_string[QUERY_SIZE] = { 0, };
	emf_mail_tbl_t *result_mail_tbl = NULL;
	
	if (input_mail_id == 0 || !output_mail_data)  {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		error = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	SNPRINTF(conditional_clause_string, QUERY_SIZE, "WHERE mail_id = %d", input_mail_id);
	
	if(!em_storage_query_mail_tbl(conditional_clause_string, true, &result_mail_tbl, &result_mail_count, &error)) {
		EM_DEBUG_EXCEPTION("em_storage_query_mail_tbl falied [%d]", error);
		goto FINISH_OFF;
	}

	if(!em_convert_mail_tbl_to_mail_data(result_mail_tbl, 1, output_mail_data, &error)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_tbl_to_mail_data falied [%d]", error);
		goto FINISH_OFF;
	}
	
FINISH_OFF: 
	if (result_mail_tbl)
		em_storage_free_mail(&result_mail_tbl, result_mail_count, NULL);
	
	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}


/* internal function */
void 
em_core_free_body_sharep(void **p)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_SAFE_FREE(*p);
	EM_DEBUG_FUNC_END();
}

/* description 
 *    get mail rfc822 size
 * arguments  
 *    mailbox  :  mailbox name
 *    msgno  :  mail sequence
 *    size  :  out body size
 * return  
 *    succeed  :  1
 *    fail  :  0
 */
EXPORT_API int em_core_mail_get_size(/*emf_mailbox_t *mailbox, */ int mail_id, int *mail_size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], mail_size[%p], err_code[%p]", mail_id, mail_size, err_code);

	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_mail_tbl_t *mail = NULL;
	
	if (mail_size == NULL)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], mail_size[%p]", mail_id, mail_size);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* get mail from mail table */
	if (!em_storage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	*mail_size = mail->mail_size;
	
	ret = true;
	
FINISH_OFF: 
	if (mail != NULL)
		em_storage_free_mail(&mail, 1, NULL);
	
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

/**
 * description 
 *    get mail body from local mailbox.
 *    if no body in local mailbox, return error.
 * arguments  
 *    mailbox  :  server mailbox
 *    mail_id  :  mai id to be downloaded
 *    callback  :  function callback. if NULL, ignored.
 * return  
 *     succeed  :  1
 *     fail  :  0
 */
EXPORT_API int em_core_mail_get_body(int mail_id, emf_mail_body_t **body, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], body[%p], err_code[%p]", mail_id, body, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int count = EMF_ATTACHMENT_MAX_COUNT;
	emf_mail_body_t *p_body = NULL;
	emf_mail_tbl_t *mail = NULL;
	emf_mail_attachment_tbl_t *attachment_tbl_list = NULL;
	
	/*  get mail from mail table */
	if (!em_storage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	EM_DEBUG_LOG("file_path_plain [%s]", mail->file_path_plain);

	/*
	if (!mail->body_download_status)  {
		EM_DEBUG_EXCEPTION("This mail body is not received.");
		*body = NULL;
		em_storage_free_mail(&mail, 1, NULL);
		return true;
	}
	*/

	em_core_mail_get_body_from_mail_tbl(&p_body, mail, NULL);	
	
	/*  retrieve attachment info */
	if ( (err = em_storage_get_attachment_list(mail_id, true, &attachment_tbl_list, &count)) != EM_STORAGE_ERROR_NONE ){
		EM_DEBUG_EXCEPTION("em_storage_get_attachment_list failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	} 
	else if (count)  {
		emf_attachment_info_t **atch = &p_body->attachment;
		int i;

		EM_DEBUG_LOG("attchment count %d", count);
		for (i = 0; i < count; i++)  {
			*atch = em_core_malloc(sizeof(emf_attachment_info_t));
			if (!(*atch))	 {	
				EM_DEBUG_EXCEPTION("malloc failed...");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			em_core_mail_fill_attachment(*atch, &attachment_tbl_list[i]);			
			atch = &(*atch)->next;
		}
	}
	
	p_body->attachment_num = count;
	
	ret = true;
	
FINISH_OFF: 
	if (ret == true)
		*body = p_body;
	else if (p_body != NULL)
		em_core_mail_body_free(&p_body, 1, NULL);
	
	if (attachment_tbl_list != NULL)
		em_storage_free_attachment(&attachment_tbl_list, count, NULL);
	
	if (mail != NULL)
		em_storage_free_mail(&mail, 1, NULL);			
	
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int em_core_mail_get_body_from_mail_tbl(emf_mail_body_t **p_body, emf_mail_tbl_t *mail, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = 0, err = EMF_ERROR_NONE;
	emf_mail_body_t *temp_body = NULL;

	if (!p_body || !mail) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	temp_body = em_core_malloc(sizeof(emf_mail_body_t));

	if (!temp_body)  {
		EM_DEBUG_EXCEPTION("em_core_malloc failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	temp_body->plain = EM_SAFE_STRDUP(mail->file_path_plain);
	temp_body->html  = EM_SAFE_STRDUP(mail->file_path_html);

	if (mail->file_path_plain && mail->file_path_plain[0] != NULL_CHAR)
		temp_body->plain_charset = g_path_get_basename(mail->file_path_plain);
	else
		temp_body->plain_charset = NULL;

	EM_DEBUG_LOG("temp_body->plain_charset [%s]", temp_body->plain_charset);

	*p_body = temp_body;

	ret = 1;
FINISH_OFF: 

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}
	
static void  
em_core_mail_fill_attachment(emf_attachment_info_t *atch , emf_mail_attachment_tbl_t *attachment_tbl)
{
	EM_DEBUG_FUNC_BEGIN();
	if (!atch || !attachment_tbl)
		return;
	
	atch->attachment_id = attachment_tbl->attachment_id;
	atch->size = attachment_tbl->attachment_size;
	atch->name = EM_SAFE_STRDUP(attachment_tbl->attachment_name); 
	atch->downloaded = attachment_tbl->file_yn ? 1  :  0;  
	atch->savename = EM_SAFE_STRDUP(attachment_tbl->attachment_path); 
	atch->drm = attachment_tbl->flag2;
	atch->inline_content = attachment_tbl->flag3;

	EM_DEBUG_FUNC_END();
}

int em_core_check_drm(emf_mail_attachment_tbl_t *attachment)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = 0;
#ifdef __FEATURE_DRM__
	drm_dcf_info_t	fileInfo;

	if (attachment == NULL)
		return ret;

	if (drm_svc_is_drm_file (attachment->attachment_path)) {
		if (drm_svc_get_dcf_file_info (attachment->attachment_path, &fileInfo) == DRM_RESULT_SUCCESS) {
			attachment->flag2 = 0;
			EM_DEBUG_LOG ("fileInfo is [%d]", fileInfo.method);
			if (fileInfo.method != DRM_METHOD_UNDEFINED) {
				attachment->flag2 = fileInfo.method;
				ret = 1;
			}
		}
	}
	else {
		EM_DEBUG_LOG("not DRM file %s", attachment->attachment_path);
		attachment->flag2 = 0;
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
EXPORT_API int em_core_mail_get_attachment(int mail_id, char *attachment_id_string, emf_attachment_info_t **attachment, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], attachment_id_string[%p], attachment[%p], err_code[%p]", mail_id, attachment_id_string, attachment, err_code);
	
	if (attachment == NULL || attachment_id_string == NULL)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], attachment_id_string[%p], attachment[%p]", mail_id, attachment_id_string, attachment);
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_mail_attachment_tbl_t *attachment_tbl = NULL;
	int attachment_id = atoi(attachment_id_string);
	
	/* get attachment from attachment tbl */
	if (!em_storage_get_attachment(mail_id, attachment_id, &attachment_tbl, true, &err) || !attachment_tbl)  {
		EM_DEBUG_EXCEPTION("em_storage_get_attachment failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	*attachment = em_core_malloc(sizeof(emf_attachment_info_t));
	if (!*attachment)  {	
		EM_DEBUG_EXCEPTION("malloc failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	(*attachment)->attachment_id = attachment_id;
	(*attachment)->name = attachment_tbl->attachment_name; attachment_tbl->attachment_name = NULL;
	(*attachment)->size = attachment_tbl->attachment_size;
	(*attachment)->downloaded = attachment_tbl->file_yn;
	(*attachment)->savename = attachment_tbl->attachment_path; attachment_tbl->attachment_path = NULL;
	(*attachment)->drm = attachment_tbl->flag2;
	(*attachment)->inline_content = attachment_tbl->flag3;
	(*attachment)->next = NULL;
	
	ret = true;
	
FINISH_OFF: 
	if (attachment_tbl)
		em_storage_free_attachment(&attachment_tbl, 1, NULL);
	
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
 *     succeed : EMF_ERROR_NONE
 *     fail    : error code
 */
EXPORT_API int em_core_get_attachment_data_list(int input_mail_id, emf_attachment_data_t **output_attachment_data, int *output_attachment_count)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], output_attachment_data[%p], output_attachment_count[%p]", input_mail_id, output_attachment_data, output_attachment_count);
	
	if (input_mail_id == 0|| output_attachment_data == NULL || output_attachment_count == NULL)  {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return EMF_ERROR_INVALID_PARAM;
	}

	int                        i = 0;
	int                        err = EMF_ERROR_NONE;
	int                        attachment_tbl_count = 0;
	emf_mail_attachment_tbl_t *attachment_tbl_list = NULL;
	emf_attachment_data_t     *temp_attachment_data = NULL;
	
	/* get attachment from attachment tbl */
	if ( (err = em_storage_get_attachment_list(input_mail_id, true, &attachment_tbl_list, &attachment_tbl_count)) != EM_STORAGE_ERROR_NONE ){
		EM_DEBUG_EXCEPTION("em_storage_get_attachment_list failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	} 

	if (attachment_tbl_count)  {
		EM_DEBUG_LOG("attchment count %d", attachment_tbl_count);
		
		*output_attachment_data = em_core_malloc(sizeof(emf_attachment_data_t) * attachment_tbl_count);

		if(*output_attachment_data == NULL) {
			EM_DEBUG_EXCEPTION("em_core_malloc failed");
			err = EMF_ERROR_OUT_OF_MEMORY;
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
			temp_attachment_data->mailbox_name          = attachment_tbl_list[i].mailbox_name; attachment_tbl_list[i].mailbox_name = NULL;
			temp_attachment_data->save_status           = attachment_tbl_list[i].file_yn;
			temp_attachment_data->drm_status            = attachment_tbl_list[i].flag2;
			temp_attachment_data->inline_content_status = attachment_tbl_list[i].flag3;
		}
	}
	
FINISH_OFF: 
	
	*output_attachment_count = attachment_tbl_count;

	if (attachment_tbl_list)
		em_storage_free_attachment(&attachment_tbl_list, attachment_tbl_count, NULL);
	
	return err;
}


EXPORT_API int em_core_mail_download_attachment(int account_id, int mail_id, char *nth, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], nth[%p], err_code[%p]", account_id, mail_id, nth, err_code);

	int attachment_no = 0;
	int err = EMF_ERROR_NONE; 
	
	if (mail_id < 1 || !nth)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], nth[%p]",  mail_id, nth);
		err = EMF_ERROR_INVALID_PARAM;
		
		if (err_code != NULL)
			*err_code = err;

		if (nth)
			attachment_no = atoi(nth);

		em_storage_notify_network_event(NOTI_DOWNLOAD_ATTACH_FAIL, mail_id, 0, attachment_no, err);	
		return false;
	}

	int ret = false;
	int status = EMF_DOWNLOAD_FAIL;
	MAILSTREAM *stream = NULL;
	BODY *mbody = NULL;
	emf_mail_tbl_t *mail = NULL;
	emf_mail_attachment_tbl_t *attachment = NULL;
	struct attachment_info *ai = NULL;
	struct _m_content_info *cnt_info = NULL;
	void *tmp_stream = NULL;
	char *s_uid = NULL, *server_mbox = NULL, buf[1024];
	int msg_no = 0;
	emf_mail_attachment_tbl_t *attachment_list = NULL;
	int current_attachment_no = 0;	
	int attachment_count_to_be_downloaded = 0;		/*  how many attachments should be downloaded */
	int i = 0;
	
	if (!em_core_check_thread_status())  {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	only_body_download = false;
	
	/*  get mail from mail table. */
	if (!em_storage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	if (!mail->server_mail_status)  {
		EM_DEBUG_EXCEPTION("not synchronous mail...");
		err = EMF_ERROR_INVALID_MAIL;	
		goto FINISH_OFF;
	}
	
	attachment_no = atoi(nth);	
	if (attachment_no == 0) {	/*  download all attachments, nth starts from 1, not zero */
		/*  get attachment list from db */
		attachment_count_to_be_downloaded = EMF_ATTACHMENT_MAX_COUNT;
		if ( (err = em_storage_get_attachment_list(mail_id, true, &attachment_list, &attachment_count_to_be_downloaded)) != EM_STORAGE_ERROR_NONE ){
			EM_DEBUG_EXCEPTION("em_storage_get_attachment_list failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
	}
	else {	/*  download only nth attachment */
		attachment_count_to_be_downloaded = 1;
		if (!em_storage_get_attachment_nth(mail_id, attachment_no, &attachment_list, true, &err) || !attachment_list)  {
			EM_DEBUG_EXCEPTION("em_storage_get_attachment_nth failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
	}
	
	if (!em_core_check_thread_status())  {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	account_id 		= mail->account_id;
	s_uid 			= EM_SAFE_STRDUP(mail->server_mail_id); 
	server_mbox 	= EM_SAFE_STRDUP(mail->mailbox_name); 
	
	/*  open mail server. */
	if (!em_core_mailbox_open(account_id, server_mbox, (void **)&tmp_stream, &err) || !tmp_stream)  {
		EM_DEBUG_EXCEPTION("em_core_mailbox_open failed [%d]", err);
		status = EMF_DOWNLOAD_CONNECTION_FAIL;
		goto FINISH_OFF;
	}
	
	stream = (MAILSTREAM *)tmp_stream;
	
	for (i = 0; i < attachment_count_to_be_downloaded; i++) {
		EM_DEBUG_LOG(" >>>>>> Attachment Downloading [%d / %d] start", i + 1, attachment_count_to_be_downloaded);

		attachment = attachment_list + i;
		if (attachment_no == 0) 					/*  download all attachments, nth starts from 1, not zero */
			current_attachment_no = i + 1;			/*  attachment no */
		else 										/*  download only nth attachment */
			current_attachment_no = attachment_no;	/*  attachment no */
				
		if (!em_core_check_thread_status())  {
			err = EMF_ERROR_CANCELLED;
			goto FINISH_OFF;
		}
		
		if (!(cnt_info = em_core_malloc(sizeof(struct _m_content_info))))  {
			EM_DEBUG_EXCEPTION("malloc failed...");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		cnt_info->grab_type = GRAB_TYPE_ATTACHMENT;		/*  attachment */
		cnt_info->file_no = current_attachment_no;		/*  attachment no */
		
#ifdef CHANGE_HTML_BODY_TO_ATTACHMENT
		/*  text/html be changed to attachment, this isn't real attachment in RFC822. */
		if (html_changed) cnt_info->file_no--;       
#endif
	
		/*  set sparep(member of BODY) memory free function. */
		mail_parameters(stream, SET_FREEBODYSPAREP, em_core_free_body_sharep);
		
		if (!em_core_check_thread_status()) {
			err = EMF_ERROR_CANCELLED;
			goto FINISH_OFF;
		}
		
		msg_no = atoi(s_uid);

		/*  get body structure. */
		/*  don't free mbody because mbody is freed in closing mail_stream. */
		if ((!stream) || em_core_get_body_structure(stream, msg_no, &mbody, &err) < 0)  {	
			EM_DEBUG_EXCEPTION("em_core_get_body_structure failed [%d]", err);
			goto FINISH_OFF;
		}
		
		if (!em_core_check_thread_status())  {
			err = EMF_ERROR_CANCELLED;
			goto FINISH_OFF;
		}
		
		/*  set body fetch section. */
		if (em_core_set_fetch_body_section(mbody, false, NULL,  &err) < 0) {
			EM_DEBUG_EXCEPTION("em_core_set_fetch_body_section failed [%d]", err);
			goto FINISH_OFF;
		}
	
		/*  download attachment. */
		_imap4_received_body_size = 0;
		_imap4_last_notified_body_size = 0;
		_imap4_total_body_size = 0;					/*  This will be assigned in imap_mail_write_body_to_file() */
		_imap4_download_noti_interval_value = 0;	/*  This will be assigned in imap_mail_write_body_to_file() */

		EM_DEBUG_LOG("cnt_info->file_no[%d], current_attachment_no[%d]", cnt_info->file_no, current_attachment_no);
		if (em_core_get_body(stream, account_id, mail_id, msg_no, mbody, cnt_info, &err) < 0)  {
			EM_DEBUG_EXCEPTION("em_core_get_body failed [%d]", err);
			goto FINISH_OFF;
		}
		
		if (!em_core_check_thread_status())  {
			err = EMF_ERROR_CANCELLED;
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
			if (!em_storage_create_dir(account_id, mail_id, current_attachment_no, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
				goto FINISH_OFF;
			}
			
			if (!em_storage_get_save_name(account_id, mail_id, current_attachment_no, ai->name, buf, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
				goto FINISH_OFF;
			}
			
			if (!em_storage_move_file(ai->save, buf, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_move_file failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}
			
			EM_SAFE_FREE(ai->save);
			
			EM_DEBUG_LOG("ai->size [%d]", ai->size);
			attachment->attachment_size = ai->size;
			attachment->attachment_path = EM_SAFE_STRDUP(buf);
	
			/*  update attachment information. */
			if (!em_storage_change_attachment_field(mail_id, UPDATE_SAVENAME, attachment, true, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_change_attachment_field failed [%d]", err);
				/*  delete created file. */
				remove(buf);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}
		
#ifdef __FEATURE_DRM__		
			if (em_core_check_drm(attachment))  {	/*  has drm attachment ? */
				if (drm_svc_register_file(attachment->attachment_path) == DRM_RESULT_SUCCESS) 
					EM_DEBUG_LOG("drm_svc_register_file success");	
				else 
					EM_DEBUG_EXCEPTION("drm_svc_register_file fail");
				mail->DRM_status = attachment->flag2;
			}
#endif /* __FEATURE_DRM__ */
		}
		else  {
			EM_DEBUG_EXCEPTION("invalid attachment sequence...");
			err = EMF_ERROR_INVALID_ATTACHMENT;		
			goto FINISH_OFF;
		}
	
		if (cnt_info)  {
			em_core_mime_free_content_info(cnt_info);		
			cnt_info = NULL;
		}
		EM_DEBUG_LOG(" >>>>>> Attachment Downloading [%d / %d] completed", i+1, attachment_count_to_be_downloaded);
	}

	if (stream)  {
		em_core_mailbox_close(0, stream);				
		stream = NULL;	
	}
	
	ret = true;
	
FINISH_OFF: 
	if (stream) 
		em_core_mailbox_close(account_id, stream);
	if (attachment_list) 
		em_storage_free_attachment(&attachment_list, attachment_count_to_be_downloaded, NULL);
	if (cnt_info) 
		em_core_mime_free_content_info(cnt_info);
	if (mail) 
		em_storage_free_mail(&mail, 1, NULL);

	EM_SAFE_FREE(s_uid);
	EM_SAFE_FREE(server_mbox);

	if (ret == true)
		em_storage_notify_network_event(NOTI_DOWNLOAD_ATTACH_FINISH, mail_id, NULL, attachment_no, 0);
	else
		em_storage_notify_network_event(NOTI_DOWNLOAD_ATTACH_FAIL, mail_id, NULL, attachment_no, err);

	if (err_code != NULL)
		*err_code = err;
	
	EM_DEBUG_FUNC_END();
	return ret;
}

#ifdef __ATTACHMENT_OPTI__
EXPORT_API int em_core_mail_download_attachment_bulk(int account_id, int mail_id, char *nth, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], nth[%p], err_code[%p]", account_id, mail_id, nth, err_code);

	int attachment_no = 0;
	int err = EMF_ERROR_NONE; /* Prevent Defect - 25093 */
	int ret = false;
	int status = EMF_DOWNLOAD_FAIL;
	MAILSTREAM *stream = NULL;
	emf_mail_tbl_t *mail = NULL;
	emf_mail_attachment_tbl_t *attachment = NULL;
	void *tmp_stream = NULL;
	char *s_uid = NULL, *server_mbox = NULL, buf[512];
	emf_mail_attachment_tbl_t *attachment_list = NULL;
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
	emf_file_list *pFileListMMc = NULL;
	emf_file_list *pFileList = NULL;
#endif /*  SUPPORT_EXTERNAL_MEMORY */
	
	
	memset(buf, 0x00, 512);
	/* CID FIX 31230 */
	if (mail_id < 1 || !nth) {
		EM_DEBUG_EXCEPTION("account_id[%d], mail_id[%d], nth[%p]", account_id, mail_id, nth);

		err = EMF_ERROR_INVALID_PARAM;
		
		if (err_code != NULL)
			*err_code = err;

		if (nth)
			attachment_no = atoi(nth);

		em_storage_notify_network_event(NOTI_DOWNLOAD_ATTACH_FAIL, mail_id, 0, attachment_no, err); 	/*  090525, kwangryul.baek */
		
		return false;
	}
	
	if (!em_core_check_thread_status()) {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	only_body_download = false;
	
	attachment_no = atoi(nth);

	if (attachment_no == 0) {	
		/*  download all attachments, nth starts from 1, not zero */
		/*  get attachment list from db */
		attachment_count_to_be_downloaded = EMF_ATTACHMENT_MAX_COUNT;
		if ( (err = em_storage_get_attachment_list(mail_id, true, &attachment_list, &attachment_count_to_be_downloaded)) != EM_STORAGE_ERROR_NONE || !attachment_list){
			EM_DEBUG_EXCEPTION("em_storage_get_attachment_list failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
	}
	else {	/*  download only nth attachment */
		attachment_count_to_be_downloaded = 1;
		if (!em_storage_get_attachment_nth(mail_id, attachment_no, &attachment_list, true, &err) || !attachment_list) {
			EM_DEBUG_EXCEPTION("em_storage_get_attachment_nth failed [%d]", err);
			
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
	}
	
	
	if (!em_core_check_thread_status()) {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	
	/*  get mail from mail table. */
	if (!em_storage_get_mail_by_id(mail_id, &mail, true, &err) || !mail) {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
		
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	/* if (!mail->server_mail_yn || !mail->text_download_yn) {*/ /*  faizan.h@samsung.com */
	
	if (!em_core_check_thread_status()) {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	account_id = mail->account_id;
	s_uid = EM_SAFE_STRDUP(mail->server_mail_id); mail->server_mail_id = NULL;
	server_mbox = EM_SAFE_STRDUP(mail->mailbox); mail->server_mailbox_name = NULL;

	
	
	/*  open mail server. */
	if (!em_core_mailbox_open(account_id, server_mbox, (void **)&tmp_stream, &err) || !tmp_stream) {
		EM_DEBUG_EXCEPTION("em_core_mailbox_open failed [%d]", err);
		
		status = EMF_DOWNLOAD_CONNECTION_FAIL;
		goto FINISH_OFF;
	}
	
	stream = (MAILSTREAM *)tmp_stream;
	
	
	if (!em_core_check_thread_status()) {
		err = EMF_ERROR_CANCELLED;
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
				
		if (!em_core_check_thread_status()) {
			err = EMF_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

		_imap4_received_body_size = 0;
		_imap4_last_notified_body_size = 0;
		_imap4_total_body_size = 0; 				/*  This will be assigned in imap_mail_write_body_to_file() */
		_imap4_download_noti_interval_value = 0;	/*  This will be assigned in imap_mail_write_body_to_file() */

		
		EM_SAFE_FREE(savefile);
		if (!em_core_get_temp_file_name(&savefile, &err)) {
			EM_DEBUG_EXCEPTION("em_core_get_temp_file_name failed [%d]", err); 		
			
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
		iActualSize = em_core_get_actual_mail_size (cnt_info->text.plain , cnt_info->text.html, cnt_info->file , &err);
		if (!em_storage_mail_check_free_space(iActualSize, &bIs_full, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_mail_check_free_space failed [%d]", err);
			goto FINISH_OFF;
		}
		
		if (bIs_full) {
			/* If external memory not present, return error */
			if (PS_MMC_REMOVED == em_storage_get_mmc_status()) {
				err = EMF_ERROR_MAIL_MEMORY_FULL;
				goto FINISH_OFF;
			}		
			bIsAdd_to_mmc = true;
		}
#endif /*  SUPPORT_EXTERNAL_MEMORY */

		if (!em_storage_create_dir(account_id, mail_id, attachment_no, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!em_storage_get_save_name(account_id, mail_id, attachment_no, attachment->name, buf, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!em_storage_move_file(savefile, buf, false, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_move_file failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}

		attachment->attachment = EM_SAFE_STRDUP(buf);
		/*  update attachment information. */
		if (!em_storage_change_attachment_field(mail_id, UPDATE_SAVENAME, attachment, true, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_change_attachment_field failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
			
#ifdef __FEATURE_DRM__		
		if (em_core_check_drm(attachment)) {		
			/*  is drm */
			if (drm_svc_register_file(attachment->attachment) == DRM_RESULT_SUCCESS)
				EM_DEBUG_LOG("drm_svc_register_file success");	
			else 
				EM_DEBUG_EXCEPTION("drm_svc_register_file fail");
			mail->flag3 = attachment->flag2;
		}
#endif /* __FEATURE_DRM__ */

		if (!em_core_check_thread_status()) {
			err = EMF_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG(" >>>>>> Attachment Downloading [%d / %d] completed", i+1, attachment_count_to_be_downloaded);
	}

	ret = true;

	FINISH_OFF: 

	EM_SAFE_FREE(savefile);

	em_core_mailbox_close(0, stream);				
	stream = NULL;	

	if (attachment_list) 
		em_storage_free_attachment(&attachment_list, attachment_count_to_be_downloaded, NULL);

	if (mail) 
		em_storage_free_mail(&mail, 1, NULL);

	if (s_uid) 
		free(s_uid);

	if (server_mbox) 
		free(server_mbox);server_mbox = NULL;

	if (ret == true)
		em_storage_notify_network_event(NOTI_DOWNLOAD_ATTACH_FINISH, mail_id, NULL, attachment_no, 0);
	else if (err != EMF_ERROR_CANCELLED)
		em_storage_notify_network_event(NOTI_DOWNLOAD_ATTACH_FAIL, mail_id, NULL, attachment_no, err);

	if (err_code != NULL)
		*err_code = err;

	return ret;
}
#endif


EXPORT_API int em_core_mail_download_body_multi_sections_bulk(void *mail_stream, int account_id, int mail_id, int verbose, int with_attach, int limited_size, int event_handle , int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_stream[%p], account_id[%d], mail_id[%d], verbose[%d], with_attach[%d], event_handle [ %d ] ", mail_stream, account_id, mail_id, verbose, with_attach, event_handle);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int status = EMF_DOWNLOAD_FAIL;
	int pop3_body_size = 0;
	int pop3_downloaded_size = 0;
	MAILSTREAM *stream = NULL;
	BODY *mbody = NULL;
	PARTLIST *section_list = NULL;
	emf_mailbox_t mbox = { 0 };
	emf_mail_tbl_t *mail = NULL;
	emf_mail_attachment_tbl_t attachment = {0, 0, NULL, };
	emf_account_t *ref_account = NULL;
	struct attachment_info *ai = NULL;
	struct _m_content_info *cnt_info = NULL;
	void *tmp_stream = NULL;
	char *s_uid = NULL, *server_mbox = NULL, buf[512];
	int msgno = 0, attachment_num = 1, local_attachment_count = 0, local_inline_content_count = 0;
	int iActualSize = 0;
	char html_body[MAX_PATH] = {0, };
	em_core_uid_list *uid_list = NULL;
	char *mailbox_name = NULL;
#ifdef CHANGE_HTML_BODY_TO_ATTACHMENT
	int html_changed = 0;
#endif
 
	if (mail_id < 1)  {
		EM_DEBUG_EXCEPTION("mail_stream[%p], account_id[%d], mail_id[%d], verbose[%d], with_attach[%d]", mail_stream, account_id, mail_id, verbose, with_attach);
		err = EMF_ERROR_INVALID_PARAM;
		
		if (err_code != NULL)
			*err_code = err;

		em_storage_notify_network_event(NOTI_DOWNLOAD_BODY_FAIL, mail_id, NULL, event_handle, err);
		return false;
	}
	
	FINISH_OFF_IF_CANCELED;

	only_body_download = true;
	
	if (!em_storage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	if (mail->mailbox_name)
		mailbox_name = EM_SAFE_STRDUP(mail->mailbox_name);
	
	if (1 == mail->body_download_status)  {
		EM_DEBUG_EXCEPTION("not synchronous mail...");
		err = EMF_ERROR_INVALID_MAIL;
		goto FINISH_OFF;
	}
	
	account_id = mail->account_id;
	s_uid                     = mail->server_mail_id; 
	server_mbox               = mail->server_mailbox_name; 
	mail->server_mail_id      = NULL;
    mail->server_mailbox_name = NULL;
	
	attachment.account_id = mail->account_id;
	attachment.mail_id = mail->mail_id;
	attachment.mailbox_name = mail->mailbox_name; mail->mailbox_name = NULL;		
	attachment.file_yn = 0;
	
	em_storage_free_mail(&mail, 1, NULL); 
	mail = NULL;	
	
	if (!(ref_account = em_core_get_account_reference(account_id)))   {
		EM_DEBUG_EXCEPTION("em_core_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	FINISH_OFF_IF_CANCELED;

	/*  open mail server. */
	if (!mail_stream)  {
		if (!em_core_mailbox_open(account_id, mailbox_name, (void **)&tmp_stream, &err) || !tmp_stream)  {
			EM_DEBUG_EXCEPTION("em_core_mailbox_open failed [%d]", err);
			status = EMF_DOWNLOAD_CONNECTION_FAIL;
			goto FINISH_OFF;
		}
		stream = (MAILSTREAM *)tmp_stream;
	}
	else
		stream = (MAILSTREAM *)mail_stream;
	
	free(server_mbox); server_mbox = NULL;
	
	
	FINISH_OFF_IF_CANCELED;
	
	if (!(cnt_info = em_core_malloc(sizeof(struct _m_content_info))))  {
		EM_DEBUG_EXCEPTION("em_core_malloc failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (ref_account->receiving_server_type == EMF_SERVER_TYPE_POP3)  {	/*  POP3 */
		/*  in POP3 case, both text and attachment are downloaded in this call. */
		cnt_info->grab_type = GRAB_TYPE_TEXT | GRAB_TYPE_ATTACHMENT;
		attachment.file_yn = 1;		/*  all attachments should be downloaded in the case of POP3 */
		
		mbox.account_id = account_id;
		mbox.mail_stream = stream;
		
		/*  download all uids from server. */
		if (!em_core_mailbox_download_uid_all(&mbox, &uid_list, NULL, NULL, 0, EM_CORE_GET_UIDS_FOR_NO_DELETE, &err))  {
			EM_DEBUG_EXCEPTION("em_core_mailbox_download_uid_all failed [%d]", err);
			goto FINISH_OFF;
		}
		
		/*  get mesg number to be related to last download mail from uid list file */
		if (!em_core_mailbox_get_msgno(uid_list, s_uid, &msgno, &err))  {
			EM_DEBUG_EXCEPTION("em_core_mailbox_get_msgno failed [%d]", err);
			err = EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER;
			goto FINISH_OFF;
		}
		
		free(s_uid); s_uid = NULL;
		
		if (!em_core_check_thread_status())  {
			err = EMF_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

		_pop3_received_body_size = 0;
		_pop3_total_body_size = 0;
		_pop3_last_notified_body_size = 0;
		_pop3_receiving_mail_id = mail_id;
		
		/*  send read mail commnad. */
		if (!em_core_mail_cmd_read_mail_pop3(stream, msgno, limited_size, &pop3_downloaded_size, &pop3_body_size, &err)) 
		/* if (!em_core_mail_cmd_read_mail_pop3(stream, msgno, PARTIAL_BODY_SIZE_IN_BYTES, &pop3_downloaded_size, &pop3_body_size, &err))  */ {
			EM_DEBUG_EXCEPTION("em_core_mail_cmd_read_mail_pop3 failed [%d]", err);
			goto FINISH_OFF;
		}

		_pop3_total_body_size = pop3_body_size;

		if (!em_storage_notify_network_event(NOTI_DOWNLOAD_BODY_START, mail_id, "dummy-file", _pop3_total_body_size, 0))
			EM_DEBUG_EXCEPTION(" em_storage_notify_network_event [ NOTI_DOWNLOAD_BODY_START] failed >>>> ");
		else
			EM_DEBUG_LOG("NOTI_DOWNLOAD_BODY_START notified (%d / %d)", 0, _pop3_total_body_size);
		
		FINISH_OFF_IF_CANCELED;
		
		/*  save message into tempfile */
		/*  parsing mime from stream. */

		if (!em_core_mime_parse_mime(stream, 0, cnt_info,  &err))  {
			EM_DEBUG_EXCEPTION("em_core_mime_parse_mime failed [%d]", err);
			goto FINISH_OFF;
		}
		
		FINISH_OFF_IF_CANCELED;
	}
	else  {	/*  in IMAP case, both text and attachment list are downloaded in this call. */
		/* if (ref_account->flag1 == 2) *//*  This flag is just for downloading mailbox.(sync header), don't be used when retrieve body. */
		if (with_attach > 0)
			cnt_info->grab_type = GRAB_TYPE_TEXT | GRAB_TYPE_ATTACHMENT;
		else
			cnt_info->grab_type = GRAB_TYPE_TEXT;
		
		int uid = atoi(s_uid);
		
		free(s_uid); s_uid = NULL;
		
		/*  set sparep(member of BODY) memory free function  */
		mail_parameters(stream, SET_FREEBODYSPAREP, em_core_free_body_sharep);
		
		/*  get body strucutre. */
		/*  don't free mbody because mbody is freed in closing mail_stream. */
		if (em_core_get_body_structure(stream, uid, &mbody, &err) < 0 || (mbody == NULL))  {  
			EM_DEBUG_EXCEPTION("em_core_get_body_structure failed [%d]", err);
			err = EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER;		
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
						
						if (!*filename)  {		/*  it may be report msg */
							if (body->disposition.type[0] == 'i' || body->disposition.type[0] == 'I')	
								is_attachment = 0;
						}
					}

					if (is_attachment == 0) {
						EM_DEBUG_LOG("%d :  body->size.bytes[%ld]", counter+1, body->size.bytes);
						multi_part_body_size = multi_part_body_size + body->size.bytes;
					}	
				}
				else {	/*  download all */
			        EM_DEBUG_LOG("%d :  body->size.bytes[%ld]", counter+1, body->size.bytes);
			        multi_part_body_size = multi_part_body_size + body->size.bytes;
				}
				part_child = part_child->next;
				counter++;
			}
		}
				
		/*  set body fetch section. */
		if (em_core_set_fetch_body_section(mbody, true, &iActualSize, &err) < 0) {
			EM_DEBUG_EXCEPTION("em_core_set_fetch_body_section failed [%d]", err);
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
		if (em_core_get_body_part_list_full(stream, uid, account_id, mail_id, mbody, cnt_info, &err, section_list, event_handle) < 0)  {
			EM_DEBUG_EXCEPTION("em_core_get_body falied [%d]", err);
			goto FINISH_OFF;
		}
		FINISH_OFF_IF_CANCELED;
	}

	
	if (false == em_storage_get_mail_by_id(mail_id, &mail, true, &err)) {
		EM_DEBUG_EXCEPTION(" em_storage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (cnt_info->text.plain)  {
		EM_DEBUG_LOG("cnt_info->text.plain [%s]", cnt_info->text.plain); 

		if (!em_storage_create_dir(account_id, mail_id, 0, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}
		
		
		if (!em_storage_get_save_name(account_id, mail_id, 0, cnt_info->text.plain_charset ? cnt_info->text.plain_charset  :  "UTF-8", buf, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}		

		if (!em_storage_move_file(cnt_info->text.plain, buf, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}
		
		mail->file_path_plain = EM_SAFE_STRDUP(buf);
		EM_DEBUG_LOG("> mail->file_path_plain [%s]", mail->file_path_plain);
	}
	
	if (cnt_info->text.html)  {
		if (!em_storage_create_dir(account_id, mail_id, 0, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}
		
		if (cnt_info->text.plain_charset != NULL) {
			memcpy(html_body, cnt_info->text.plain_charset, strlen(cnt_info->text.plain_charset));
			strcat(html_body, HTML_EXTENSION_STRING);
		}
		else {
			memcpy(html_body, "UTF-8.htm", strlen("UTF-8.htm"));
		}
		if (!em_storage_get_save_name(account_id, mail_id, 0, html_body, buf, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!em_storage_move_file(cnt_info->text.html, buf, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_move_file failed [%d]", err);
			goto FINISH_OFF;
		}
		mail->file_path_html = EM_SAFE_STRDUP(buf);
	}
	
	if (ref_account->receiving_server_type == EMF_SERVER_TYPE_POP3 && limited_size != NO_LIMITATION && limited_size < pop3_body_size)
 		mail->body_download_status = EMF_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED;
	else
		mail->body_download_status = EMF_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;
	
#ifdef CHANGE_HTML_BODY_TO_ATTACHMENT
	if (html_changed) mail->flag2 = 1;
#endif
	
	FINISH_OFF_IF_CANCELED;
	
	for (ai = cnt_info->file; ai; ai = ai->next, attachment_num++)  {
		attachment.attachment_id = attachment_num;
		attachment.attachment_size = ai->size;
		attachment.attachment_path = ai->save;
		attachment.attachment_name = ai->name;
		attachment.flag1 = ai->drm;
		attachment.flag3 = ai->type == 1;
		attachment.file_yn = 0;

		EM_DEBUG_LOG("attachment.attachment_id[%d]", attachment.attachment_id);
		EM_DEBUG_LOG("attachment.attachment_size[%d]", attachment.attachment_size);
		EM_DEBUG_LOG("attachment.attachment_path[%s]", attachment.attachment_path);
		EM_DEBUG_LOG("attachment.attachment_name[%s]", attachment.attachment_name);
		EM_DEBUG_LOG("attachment.flag1[%d]", attachment.flag1);
		EM_DEBUG_LOG("attachment.flag3[%d]", attachment.flag3);
		EM_DEBUG_LOG("ai->save [%d]", ai->save);
#ifdef __ATTACHMENT_OPTI__
		attachment.encoding = ai->encoding;
		attachment.section = ai->section;
#endif

		if (ai->type == 1) 
			local_inline_content_count++;
		local_attachment_count++;

		if (ai->save)  {
			/*  in POP3 case, rename temporary file to real file. */
			attachment.file_yn = 1;
			if (ai->type == 1)  {		/*  it is inline content */
				if (!em_storage_create_dir(account_id, mail_id, 0, &err))  {
					EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
					goto FINISH_OFF;
				}
				if (!em_storage_get_save_name(account_id, mail_id, 0, attachment.attachment_name, buf, &err))  {
					EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
					goto FINISH_OFF;
				}
			}
			else  {
				if (!em_storage_create_dir(account_id, mail_id, attachment_num, &err)) {
					EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
					goto FINISH_OFF;
				}

				if (!em_storage_get_save_name(account_id, mail_id, attachment_num, attachment.attachment_name, buf, &err))  {
					EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
					goto FINISH_OFF;
				}
			}
			
			if (!em_storage_move_file(ai->save, buf, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_move_file failed [%d]", err);
				
				/*  delete all created files. */
				if (!em_storage_get_save_name(account_id, mail_id, 0, NULL, buf, NULL)) {
					EM_DEBUG_EXCEPTION("em_storage_get_save_name failed...");
					/* goto FINISH_OFF; */
				}
				
				if (!em_storage_delete_dir(buf, NULL)) {
					EM_DEBUG_EXCEPTION("em_storage_delete_dir failed...");
					/* goto FINISH_OFF; */
				}
				
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}
			
			free(ai->save); ai->save = EM_SAFE_STRDUP(buf);
			
			attachment.attachment_path = ai->save;
			
#ifdef __FEATURE_DRM__			
			if (em_core_check_drm(&attachment))  {		/*  is drm */
				if (!drm_svc_register_file(attachment.attachment_path)) 
					EM_DEBUG_EXCEPTION("drm_svc_register_file fail");
				mail->DRM_status = attachment.flag2;
			}
#endif/*  __FEATURE_DRM__ */
		}

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
		/*  Information :  Attachment info already saved if partial body is dowloaded. */
		if (ai->type)  {   /*  Get attachment details  */
			emf_mail_attachment_tbl_t *attch_info = NULL;
		
			if (!em_storage_get_attachment_nth(mail_id, attachment.attachment_id, &attch_info, true, &err) || !attch_info)  {
				EM_DEBUG_EXCEPTION("em_storage_get_attachment_nth failed [%d]", err);
				if (err == EM_STORAGE_ERROR_ATTACHMENT_NOT_FOUND) {   /*  save only attachment file. */
					if (!em_storage_add_attachment(&attachment, 0, false, &err)) {
						EM_DEBUG_EXCEPTION("em_storage_add_attachment failed [%d]", err);
						if (attch_info)
							em_storage_free_attachment(&attch_info, 1, NULL);		
						/*  delete all created files. */
						if (!em_storage_get_save_name(account_id, mail_id, 0, NULL, buf, &err))  {
							EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
							goto FINISH_OFF;
						}
						
						if (!em_storage_delete_dir(buf, &err))  {
							EM_DEBUG_EXCEPTION("em_storage_delete_dir failed [%d]", err);
							goto FINISH_OFF;
						}
						
						/*  ROLLBACK TRANSACTION; */
						em_storage_rollback_transaction(NULL, NULL, NULL);
						
						err = em_storage_get_emf_error_from_em_storage_error(err);
						goto FINISH_OFF;						
					}
				}
			}
			else {
				EM_DEBUG_LOG("Attachment info already exists...!");
				/* Update attachment size */
				EM_DEBUG_LOG("attachment_size [%d], ai->size [%d]", attch_info->attachment_size, ai->size);
				attch_info->attachment_size = ai->size;
				if (!em_storage_update_attachment(attch_info, true, &err)) {
					EM_DEBUG_EXCEPTION("em_storage_update_attachment failed [%d]", err);
					err = em_storage_get_emf_error_from_em_storage_error(err);
						goto FINISH_OFF;		
				}
			}

			if (attch_info)
				em_storage_free_attachment(&attch_info, 1, NULL);		
		}

#else
		
		if (ai->type)  {
			mail->attachment_yn = 1;
			/*  save only attachment file. */
			if (!em_storage_add_attachment(&attachment, 0, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_add_attachment failed [%d]", err);
				if (bIsAdd_to_mmc)  {
					if (attachment.attachment) {				
					}
				}
				else  {				
					/*  delete all created files. */
					if (!em_storage_get_save_name(account_id, mail_id, 0, NULL, buf, &err))  {
						EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
						goto FINISH_OFF;
					}
				
					if (!em_storage_delete_dir(buf, &err))  {
						EM_DEBUG_EXCEPTION("em_storage_delete_dir failed [%d]", err);
						goto FINISH_OFF;
					}
				
					/*  ROLLBACK TRANSACTION; */
					em_storage_rollback_transaction(NULL, NULL, NULL);
					
					err = em_storage_get_emf_error_from_em_storage_error(err);
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
	if (!em_storage_change_mail_field(mail_id, APPEND_BODY, mail, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_change_mail_field failed [%d]", err);
		em_storage_rollback_transaction(NULL, NULL, NULL); /*  ROLLBACK TRANSACTION; */
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("cnt_info->text.plain [%s], cnt_info->text.html [%s]", cnt_info->text.plain, cnt_info->text.html);
	
	/*  in pop3 mail case, the mail is deleted from server after being downloaded. */
	if (ref_account->receiving_server_type == EMF_SERVER_TYPE_POP3)  {
#ifdef DELETE_AFTER_DOWNLOADING
		char delmsg[24];
		
		SNPRINTF(delmsg, sizeof(delmsg), "%d", msg_no);
		
		if (!ref_account->keep_on_server)  {
			if (!em_core_mail_delete_from_server(&mbox, delmsg, &err)) 
				EM_DEBUG_EXCEPTION("em_core_mail_delete_from_server failed [%d]", err);
		}
#endif
		
		if (!mail_stream)  {
			if (stream != NULL)  {
				em_core_mailbox_close(0, stream);				
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
		em_core_mime_free_content_info(cnt_info);
	if (mail) 
		em_storage_free_mail(&mail, 1, NULL);
	if (attachment.mailbox_name) 
		free(attachment.mailbox_name);
	if (server_mbox) 
		free(server_mbox);
	if (s_uid) 
		free(s_uid);

	multi_part_body_size = 0;
	
	if (ret == true)
		em_storage_notify_network_event(NOTI_DOWNLOAD_BODY_FINISH, mail_id, NULL, event_handle, 0);
	else
		em_storage_notify_network_event(NOTI_DOWNLOAD_BODY_FAIL, mail_id, NULL, event_handle, err);

	if (mailbox_name)
		free (mailbox_name);
	
	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}



void em_core_mail_copyuid(MAILSTREAM *stream, char *mailbox, 
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
	
	if (!em_storage_update_server_uid(old_server_uid, g_new_server_uid, NULL)) {
		EM_DEBUG_EXCEPTION("em_storage_update_server_uid falied...");
	}
}

int em_core_mail_delete(int account_id, int mail_ids[], int num, int from_server, int noti_param_1, int noti_param_2, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_ids[%p], num[%d], from_server[%d], noti_param_1 [%d], noti_param_2 [%d], err_code[%p]", account_id, mail_ids, num, from_server, noti_param_1, noti_param_2, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int status = EMF_DELETE_FAIL;
	
	emf_account_t *account = NULL;
	emf_mailbox_t mailbox;
	emf_mail_tbl_t *mail = NULL;
	void *stream = NULL;
	int mail_id = 0;
	int i = 0;
	int msgno = 0, parameter_string_length = 0;
	char *parameter_string = NULL, mail_id_string[10];
	#ifdef 	__FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__
	int bulk_flag = false;
	#endif
	
	memset(&mailbox, 0x00, sizeof(emf_mailbox_t));
	
	if (!account_id || !mail_ids || !num)  {
		EM_DEBUG_EXCEPTION("account_id[%d], mail_ids[%p], num[%d], from_server[%d]", account_id, mail_ids, num, from_server);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	mail_id = mail_ids[0];
	
	if (!(account = em_core_get_account_reference(account_id)))  {
		EM_DEBUG_EXCEPTION("em_core_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (account->receiving_server_type == EMF_SERVER_TYPE_ACTIVE_SYNC)
		from_server = EMF_DELETE_LOCALLY;
	
	FINISH_OFF_IF_CANCELED;
	
	parameter_string_length = sizeof(char) * (num * 8 + 128 /* MAILBOX_LEN_IN_MAIL_TBL */ * 2);
	parameter_string = malloc(parameter_string_length);

	if (parameter_string == NULL) {
		EM_DEBUG_EXCEPTION("Memory allocation for mail_id_list_string failed");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(parameter_string, 0, parameter_string_length);
	
	if (from_server == EMF_DELETE_LOCAL_AND_SERVER || from_server == EMF_DELETE_FOR_SEND_THREAD)  {   /* server delete */
		for (i = 0; i < num; i++)  {
			mail_id = mail_ids[i];
			
			if (!em_storage_get_downloaded_mail(mail_id, &mail, false, &err) || !mail)  {
				EM_DEBUG_EXCEPTION("em_storage_get_uid_by_mail_id failed [%d]", err);
				
				if (err == EM_STORAGE_ERROR_MAIL_NOT_FOUND)  {	/*  not server mail */
					/* err = EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER; */
					continue;
				}
				else
					err = em_storage_get_emf_error_from_em_storage_error(err);
				
				goto FINISH_OFF;
			}
					
			EM_SAFE_FREE(mailbox.user_data); 
			
			if (stream == NULL)  {					
				if (!em_core_mailbox_open(account_id, mail->server_mailbox_name , (void **)&stream, &err))  {
					EM_DEBUG_EXCEPTION("em_core_mailbox_open failed [%d]", err);
					status = EMF_DELETE_CONNECTION_FAIL;
					goto FINISH_OFF;
				}
					
				mailbox.account_id = account_id;
				mailbox.name = mail->server_mailbox_name;		
				mailbox.mail_stream = stream;
					
				FINISH_OFF_IF_CANCELED;
			}
				
#ifdef 	__FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__
			if (account->receiving_server_type == EMF_SERVER_TYPE_IMAP4) {
#ifdef __LOCAL_ACTIVITY__
				if (!bulk_flag && !imap4_mail_delete_ex(&mailbox, mail_ids, num, from_server, &err)) {
#else /* __LOCAL_ACTIVITY__ */
				if (!bulk_flag && !imap4_mail_delete_ex(&mailbox, mail_ids, num, &err)) {
#endif /* __LOCAL_ACTIVITY__ */
					EM_DEBUG_EXCEPTION("imap4_mail_delete_ex failed [%d]", err);
					if (err == EMF_ERROR_IMAP4_STORE_FAILURE)
						err = EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER;
					goto FINISH_OFF;
				}
				else
					bulk_flag = true;

			}
			else if (account->receiving_server_type == EMF_SERVER_TYPE_POP3) {
				if (!em_core_mail_get_msgno_by_uid(account, &mailbox, mail->server_mail_id, &msgno, &err))  {
					EM_DEBUG_EXCEPTION("em_core_mail_get_msgno_by_uid faild [%d]", err);
					if (err == EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER)
						goto NOT_FOUND_ON_SERVER;
					else
						goto FINISH_OFF;
				}
				
				FINISH_OFF_IF_CANCELED;
				
				if (!em_core_mail_delete_from_server(account, &mailbox, msgno, &err))  {
					EM_DEBUG_EXCEPTION("em_core_mail_delete_from_server falied [%d]", err);
					goto FINISH_OFF;
				}
#ifdef __LOCAL_ACTIVITY__				
				else {
					/* Remove local activity */
					emf_activity_tbl_t new_activity;
					memset(&new_activity, 0x00, sizeof(emf_activity_tbl_t));
					if (from_server == EMF_DELETE_FOR_SEND_THREAD) {
						new_activity.activity_type = ACTIVITY_DELETEMAIL_SEND;
						EM_DEBUG_LOG("from_server == EMF_DELETE_FOR_SEND_THREAD ");
					}
					else {
						new_activity.activity_type	= ACTIVITY_DELETEMAIL;
					}
				
					new_activity.mail_id		= mail->mail_id;
					new_activity.server_mailid	= mail->server_mail_id;
					new_activity.src_mbox		= NULL;
					new_activity.dest_mbox		= NULL;
								
					if (!em_core_activity_delete(&new_activity, &err)) {
						EM_DEBUG_EXCEPTION(" em_core_activity_delete  failed  - %d ", err);
					}
				
					/* Fix for issue - Sometimes mail move and immediately followed by mail delete is not reflected on server */
					if (!em_storage_remove_downloaded_mail(account_id, mail->server_mailbox_name, mail->server_mail_id, true, &err))  {
						EM_DEBUG_LOG("em_storage_remove_downloaded_mail falied [%d]", err);
					}	
				}
				
#endif	/*  __LOCAL_ACTIVITY__ */
			}
#else	/*  __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__ */
			if (!em_core_mail_get_msgno_by_uid(account, &mailbox, mail->server_mail_id, &msgno, &err)) {
				EM_DEBUG_LOG("em_core_mail_get_msgno_by_uid faild [%d]", err);
					
				if (err == EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER) /* Prevent Defect - 9561 */
					goto NOT_FOUND_ON_SERVER;
				else
					goto FINISH_OFF;
			}
				
			FINISH_OFF_IF_CANCELED;
				
			if (!em_core_mail_delete_from_server(account, &mailbox, msgno, &err)) {
				EM_DEBUG_LOG("em_core_mail_delete_from_server falied [%d]", err);
					
				goto FINISH_OFF;
			}
#endif	/*  __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__ */
				
			if (!em_storage_remove_downloaded_mail(account_id, mail->server_mailbox_name, mail->server_mail_id, true, &err)) 
				EM_DEBUG_LOG("em_storage_remove_downloaded_mail falied [%d]", err);

			/* em_core_delete_notification_for_read_mail(mail_id); */
				
NOT_FOUND_ON_SERVER: 				
			memset(mail_id_string, 0, 10);
			SNPRINTF(mail_id_string, 10, "%d,", mail_id);
			strcat(parameter_string, mail_id_string);
				
			FINISH_OFF_IF_CANCELED;
		
			em_storage_free_mail(&mail, 1, NULL);
		}
	}
	else if (from_server == EMF_DELETE_LOCALLY) /* Local Delete */ {
		em_core_mail_delete_from_local(account_id, mail_ids, num, noti_param_1, noti_param_2, err_code);
		for (i = 0; i < num; i++) {
			/* em_core_delete_notification_for_read_mail(mail_id); */
			SNPRINTF(mail_id_string, 10, "%d,", mail_id);
			strcat(parameter_string, mail_id_string);
		}

		em_core_check_unread_mail();
	}
	
	ret = true;

FINISH_OFF: 

	if (stream) 		 {
		em_core_mailbox_close(0, stream);				
		stream = NULL;	
	}
	
	if (mailbox.user_data != NULL) {
		em_core_mailbox_free_uids(mailbox.user_data, NULL);		
		mailbox.user_data = NULL;
	}
	
	EM_SAFE_FREE(parameter_string);
		
	if (mail != NULL)
		em_storage_free_mail(&mail, 1, NULL);


	if (from_server)		
		em_core_show_popup(account_id, EMF_ACTION_DELETE_MAIL, ret == true ? 0  :  err);

	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}


int em_core_mail_delete_all(emf_mailbox_t *mailbox, int with_server, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox [%p], with_server [%d], err_code [%p]", mailbox, with_server, err_code);

	int   ret = false;
	int   err = EMF_ERROR_NONE;
	int   search_handle = 0;
	int  *mail_ids = NULL;
	int   i = 0;
	int   total = 0;
	char  buf[512] = { 0, };

	if (!mailbox || mailbox->account_id < FIRST_ACCOUNT_ID) {
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (mailbox->name != NULL) {
		/* Delete all mails in specific mailbox */
		if (!em_storage_mail_search_start(NULL, mailbox->account_id, mailbox->name, 0, &search_handle, &total, true, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_mail_search_start failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("em_storage_mail_search_start returns [%d]", total);

		if (total > 0) {
			mail_ids = em_core_malloc(sizeof(int) * total);
			if (mail_ids == NULL)  {
				EM_DEBUG_EXCEPTION("em_core_malloc failed...");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			for (i = 0; i < total; i++)  {
				if (!em_storage_mail_search_result(search_handle, RETRIEVE_ID, (void**)&mail_ids[i], true, &err)) {
					EM_DEBUG_EXCEPTION("em_storage_mail_search_result failed [%d]", err);
					err = em_storage_get_emf_error_from_em_storage_error(err);
					goto FINISH_OFF;
				}
			}

			if (!em_core_mail_delete(mailbox->account_id, mail_ids, total, with_server, EMF_DELETED_BY_COMMAND, false, &err)) {
				EM_DEBUG_EXCEPTION("em_core_mail_delete failed [%d]", err);
				goto FINISH_OFF;
			}
		}
	}
	else if (with_server == EMF_DELETE_LOCALLY){
		/* em_storage_delete_mail_by_account is available only locally */
		if (!em_storage_delete_mail_by_account(mailbox->account_id, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_delete_mail_by_account failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
			
		if (!em_storage_delete_attachment_all_on_db(mailbox->account_id, NULL, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_delete_attachment_all_on_db failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		/*  delete mail contents from filesystem */
		if (!em_storage_get_save_name(mailbox->account_id, 0, 0, NULL, buf, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		if (!em_storage_delete_dir(buf, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_delete_dir failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
		}
			
		/*  delete meeting request */
		if (!em_storage_delete_meeting_request(mailbox->account_id, 0, NULL, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_delete_attachment_all_on_db failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}		

		em_core_check_unread_mail();
	}
	
	ret = true;

FINISH_OFF: 
	if (search_handle >= 0)  {
		if (!em_storage_mail_search_end(search_handle, true, &err))
			EM_DEBUG_EXCEPTION("em_storage_mail_search_end failed [%d]", err);
	}
	
	EM_SAFE_FREE(mail_ids);
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d], err [%d]", ret, err);
	return ret;
}

EXPORT_API int em_core_mail_delete_from_local(int account_id, int *mail_ids, int num, int noti_param_1, int noti_param_2, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_ids[%p], num [%d], noti_param_1 [%d], noti_param_2 [%d], err_code[%p]", account_id, mail_ids, num, noti_param_1, noti_param_2, num, err_code);
	int ret = false, err = EMF_ERROR_NONE, i;
	emf_mail_tbl_t *result_mail_list;
	char mail_id_string[10], *noti_param_string = NULL, buf[512] = {0, };

	/* Getting mail list by using select mail_id [in] */

	if(!em_storage_get_mail_field_by_multiple_mail_id(mail_ids, num, RETRIEVE_SUMMARY, &result_mail_list, true, &err) || !result_mail_list) {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_field_by_multiple_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Deleting mails by using select mail_id [in] */
	if(!em_storage_delete_multiple_mails(mail_ids, num, true, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_delete_multiple_mails failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Sending Notification */
	noti_param_string = em_core_malloc(sizeof(char) * 10 * num);

	if(!noti_param_string) {
		EM_DEBUG_EXCEPTION("em_core_malloc failed");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for(i = 0; i < num; i++) {
		memset(mail_id_string, 0, 10);
		SNPRINTF(mail_id_string, 10, "%d,", mail_ids[i]);
		strcat(noti_param_string, mail_id_string);
		/* can be optimized by appending sub string with directly pointing on string array kyuho.jo 2011-10-07 */
	}

	if (!em_storage_notify_storage_event(NOTI_MAIL_DELETE, account_id, noti_param_1, noti_param_string, noti_param_2)) 
		EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event failed [ NOTI_MAIL_DELETE_FINISH ] >>>> ");

	/* Updating Thread informations */
	/* Thread information should be updated as soon as possible. */
	for(i = 0; i < num; i++) {
		if (result_mail_list[i].thread_item_count > 1)	{
			if (!em_storage_update_latest_thread_mail(account_id, result_mail_list[i].thread_id, 0, 0, false,  &err)) {
				EM_DEBUG_EXCEPTION("em_storage_update_latest_thread_mail failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}
		}
	}
	if (!em_storage_notify_storage_event(NOTI_MAIL_DELETE_FINISH, account_id, noti_param_1, noti_param_string, noti_param_2)) 
		EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event failed [ NOTI_MAIL_DELETE_FINISH ] >>>> ");

	for(i = 0; i < num; i++) {
		/* Deleting attachments */
		if (!em_storage_delete_attachment_on_db(result_mail_list[i].mail_id, 0, false, &err)) {		
			EM_DEBUG_EXCEPTION("em_storage_delete_attachment_on_db failed [%d]", err);
			
			if (err == EM_STORAGE_ERROR_ATTACHMENT_NOT_FOUND)
				err = EMF_ERROR_NONE;
			else 
				err = em_storage_get_emf_error_from_em_storage_error(err);
		}

		/* Deleting Directories */	
		/*  delete mail contents from filesystem */
		if (!em_storage_get_save_name(account_id, result_mail_list[i].mail_id, 0, NULL, buf, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
	
		if (!em_storage_delete_dir(buf, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_delete_dir failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
		}
		
		/* Deleting Meeting Request */
		if (!em_storage_delete_meeting_request(account_id, result_mail_list[i].mail_id, NULL, false, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_delete_meeting_request failed [%d]", err);			
			if (err != EM_STORAGE_ERROR_CONTACT_NOT_FOUND) {
				err = em_storage_get_emf_error_from_em_storage_error(err);
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

int em_core_mail_delete_from_server(emf_account_t *account, emf_mailbox_t *mailbox, int msgno, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account[%p], mailbox[%p], msgno[%d], err_code[%p]", account, mailbox, msgno, err_code);

	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!account || !mailbox) {
		EM_DEBUG_EXCEPTION("account[%p], mailbox[%p], msgno[%d]", account, mailbox, msgno);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (account->receiving_server_type == EMF_SERVER_TYPE_POP3) {
		if (!pop3_mail_delete(mailbox->mail_stream, msgno, &err)) {
			EM_DEBUG_EXCEPTION("pop3_mail_delete failed [%d]", err);
			
			if (err == EMF_ERROR_POP3_DELE_FAILURE)
				err = EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER;
			goto FINISH_OFF;
		}
	}
	else {		/*  EMF_SERVER_TYPE_IMAP4 */
		if (!imap4_mail_delete(mailbox->mail_stream, msgno, &err)) {
			EM_DEBUG_EXCEPTION("imap4_mail_delete failed [%d]", err);
			
			if (err == EMF_ERROR_IMAP4_STORE_FAILURE)
				err = EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER;
			goto FINISH_OFF;
		}
	}
	
	ret = true;

FINISH_OFF: 
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int em_core_mail_get_msgno_by_uid(emf_account_t *account, emf_mailbox_t *mailbox, char *uid, int *msgno, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account[%p], mailbox[%p], uid[%s], msgno[%p], err_code[%p]", account, mailbox, uid, msgno, err_code);

	int ret = false;
	int err = EMF_ERROR_NONE;
	
	em_core_uid_list *uid_list = NULL;
	em_core_uid_list *pTemp_uid_list = NULL;
	
	if (!account || !mailbox || !uid || !msgno)  {
		EM_DEBUG_EXCEPTION("account[%p], mailbox[%p], uid[%s], msgno[%p]", account, mailbox, uid, msgno);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	uid_list = mailbox->user_data;
	
	if (uid_list == NULL)  {
		if (account->receiving_server_type == EMF_SERVER_TYPE_POP3)  {
			if (!pop3_mailbox_get_uids(mailbox->mail_stream, &uid_list, &err))  {
				EM_DEBUG_EXCEPTION("pop3_mailbox_get_uids failed [%d]", err);
				goto FINISH_OFF;
			}
		}
		else  {		/*  EMF_SERVER_TYPE_IMAP4 */
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
	
	err = EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER;

FINISH_OFF: 
	if (err_code != NULL)
		*err_code = err;
	uid_list = pTemp_uid_list ;
	if (uid_list != NULL)
		em_core_mailbox_free_uids(uid_list, NULL);		
	/*  mailbox->user_data and uid_list both point to same memory address, So when uid_list  is freed then just set  */
	/*  mailbox->user_data to NULL and dont use EM_SAFE_FREE, it will crash  : ) */
	if (mailbox)
	mailbox->user_data = NULL; 
	EM_DEBUG_FUNC_END();
	return ret;
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
EXPORT_API int em_core_mail_add_attachment(int mail_id, emf_attachment_info_t *attachment, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], attachment[%p], err_code[%p]", mail_id, attachment, err_code);
	
	if (attachment == NULL)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], attachment[%p]", mail_id, attachment);
		if (err_code)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false, err = EMF_ERROR_NONE;
	emf_mail_tbl_t *mail_table_data = NULL;
	int attachment_id = 0;
	

	
	if (!em_storage_get_mail_field_by_id(mail_id, RETRIEVE_SUMMARY, &mail_table_data, true, &err) || !mail_table_data)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_field_by_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF2;
	}
	
	int account_id = mail_table_data->account_id;
	emf_mail_attachment_tbl_t attachment_tbl;
	
	memset(&attachment_tbl, 0x00, sizeof(emf_mail_attachment_tbl_t));

	mail_table_data->attachment_count = mail_table_data->attachment_count + 1;
	attachment_tbl.account_id         = mail_table_data->account_id;
	attachment_tbl.mailbox_name       = mail_table_data->mailbox_name;
	attachment_tbl.mail_id 	          = mail_id;
	attachment_tbl.attachment_name    = attachment->name;
	attachment_tbl.attachment_size    = attachment->size;
	attachment_tbl.file_yn            = attachment->downloaded;
	attachment_tbl.flag1              = attachment->drm;
	attachment_tbl.flag3              = attachment->inline_content;
	
	/*  BEGIN TRANSACTION; */
	em_storage_begin_transaction(NULL, NULL, NULL);
	
	if (!em_storage_add_attachment(&attachment_tbl, 0, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_add_attachment failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	attachment->attachment_id = attachment_tbl.attachment_id;
	
	if (attachment->savename)  {
		char buf[512];
		
		if (!attachment->inline_content) {
			if (!em_storage_create_dir(account_id, mail_id, attachment_tbl.attachment_id, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
				goto FINISH_OFF;
			}
			attachment_id = attachment_tbl.attachment_id;
		}
		
		if (!em_storage_get_save_name(account_id, mail_id, attachment_id, attachment->name, buf, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}
        attachment_tbl.attachment_path = buf;
		
		if (!em_storage_change_attachment_field(mail_id, UPDATE_SAVENAME, &attachment_tbl, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_change_attachment_field failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}

		if (!em_storage_change_mail_field(mail_id, APPEND_BODY, mail_table_data, false, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_change_mail_field failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}			

		if (attachment->downloaded) {
			if (!em_storage_move_file(attachment->savename, buf, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_move_file failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}
		}
		
		/* Here only filename is being updated. Since first add is being done there will not be any old files.
		    So no need to check for old files in this update case */
		if (!em_storage_change_attachment_field(mail_id, UPDATE_SAVENAME, &attachment_tbl, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_change_attachment_field failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		EM_SAFE_FREE(attachment->savename);
		attachment->savename = EM_SAFE_STRDUP(buf);
	}
	
	ret = true;
	
FINISH_OFF: 
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (em_storage_commit_transaction(NULL, NULL, NULL) == false) {
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
			ret = false;
		}
	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (em_storage_rollback_transaction(NULL, NULL, NULL) == false)
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
	}	
	
FINISH_OFF2: 
	if (mail_table_data != NULL)
		em_storage_free_mail(&mail_table_data, 1, NULL);
	
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}


EXPORT_API int em_core_mail_add_attachment_data(int input_mail_id, emf_attachment_data_t *input_attachment_data)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], input_attachment_data[%p]", input_mail_id, input_attachment_data);

	int                        err             = EMF_ERROR_NONE;
	int                        attachment_id   = 0;
	char                       buf[512] = { 0, };
	emf_mail_tbl_t            *mail_table_data = NULL;
	emf_mail_attachment_tbl_t  attachment_tbl  = { 0 };
	
	if (input_attachment_data == NULL)  {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return EMF_ERROR_INVALID_PARAM;
	}

	if (!em_storage_get_mail_field_by_id(input_mail_id, RETRIEVE_SUMMARY, &mail_table_data, true, &err) || !mail_table_data)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_field_by_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF2;
	}

	mail_table_data->attachment_count = mail_table_data->attachment_count + 1;
	
	attachment_tbl.account_id         = mail_table_data->account_id;
	attachment_tbl.mailbox_name       = mail_table_data->mailbox_name;
	attachment_tbl.mail_id 	          = input_mail_id;
	attachment_tbl.attachment_name    = input_attachment_data->attachment_name;
	attachment_tbl.attachment_size    = input_attachment_data->attachment_size;
	attachment_tbl.file_yn            = input_attachment_data->save_status;
	attachment_tbl.flag1              = input_attachment_data->drm_status;
	attachment_tbl.flag3              = input_attachment_data->inline_content_status;
	
	/*  BEGIN TRANSACTION; */
	em_storage_begin_transaction(NULL, NULL, NULL);
	
	if (!em_storage_add_attachment(&attachment_tbl, 0, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_add_attachment failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	input_attachment_data->attachment_id = attachment_tbl.attachment_id;
	
	if (input_attachment_data->attachment_path)  {
		if (!input_attachment_data->inline_content_status) {
			if (!em_storage_create_dir(mail_table_data->account_id, input_mail_id, attachment_tbl.attachment_id, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
				goto FINISH_OFF;
			}
			attachment_id = attachment_tbl.attachment_id;
		}
		
		if (!em_storage_get_save_name(mail_table_data->account_id, input_mail_id, attachment_id, input_attachment_data->attachment_name, buf, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
			goto FINISH_OFF;
		}
        attachment_tbl.attachment_path = buf;
		
		if (!em_storage_change_attachment_field(input_mail_id, UPDATE_SAVENAME, &attachment_tbl, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_change_attachment_field failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}

		if (!em_storage_change_mail_field(input_mail_id, APPEND_BODY, mail_table_data, false, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_change_mail_field failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}			

		if (input_attachment_data->save_status) {
			if (!em_storage_move_file(input_attachment_data->attachment_path, buf, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_move_file failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}
		}
		
		/* Here only filename is being updated. Since first add is being done there will not be any old files.
		    So no need to check for old files in this update case */
		if (!em_storage_change_attachment_field(input_mail_id, UPDATE_SAVENAME, &attachment_tbl, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_change_attachment_field failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		EM_SAFE_FREE(input_attachment_data->attachment_path);
		input_attachment_data->attachment_path = EM_SAFE_STRDUP(buf);
	}
	
FINISH_OFF: 
	if (err == EMF_ERROR_NONE) {	/*  COMMIT TRANSACTION; */
		if (em_storage_commit_transaction(NULL, NULL, NULL) == false) {
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
		}
	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (em_storage_rollback_transaction(NULL, NULL, NULL) == false)
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
	}	
	
FINISH_OFF2: 
	if (mail_table_data != NULL)
		em_storage_free_mail(&mail_table_data, 1, NULL);
	
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
int em_core_mail_delete_attachment(int mail_id,    char *attachment_id_string, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], attachment_id_string[%s], err_code[%p]", mail_id, attachment_id_string, err_code);
	
	if (attachment_id_string == NULL)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], attachment_id[%p]", mail_id, attachment_id_string);
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	int attachment_id = atoi(attachment_id_string);
	
	/*  BEGIN TRANSACTION; */
	em_storage_begin_transaction(NULL, NULL, NULL);
	
	if (!em_storage_delete_attachment_on_db(mail_id, attachment_id, false, &error))  {
		EM_DEBUG_EXCEPTION("em_storage_delete_attachment_on_db failed [%d]", error);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF: 
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (em_storage_commit_transaction(NULL, NULL, NULL) == false) {
			error = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
			ret = false;
		}
	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (em_storage_rollback_transaction(NULL, NULL, NULL) == false)
			error = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
	}	
	
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/* description 
 *    update a attachment to mail.
 * arguments  
 *    mailbox  :  mail box
 *    mail_id  :  mail id
 *    attachment  :  attachment to be added
 * return  
 *    succeed  :  1
 *    fail  :  0
 */
static int em_core_mail_update_attachment(int mail_id, emf_attachment_info_t *updated_attachment, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], attachment[%p], err_code[%p]", mail_id, updated_attachment, err_code);

	int ret = false, err = EMF_ERROR_NONE;
	emf_mail_attachment_tbl_t *existing_attachment_info = NULL;
	emf_mail_attachment_tbl_t attachment_tbl;

	if (updated_attachment == NULL)  {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF2;
	}

	if (!em_storage_get_attachment(mail_id, updated_attachment->attachment_id, &existing_attachment_info, 1, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_get_attachment failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF2;
	}
	
	memset(&attachment_tbl, 0x00, sizeof(emf_mail_attachment_tbl_t));
	
	attachment_tbl.mail_id         = mail_id;
	attachment_tbl.account_id      = existing_attachment_info->account_id;
	attachment_tbl.mailbox_name    = existing_attachment_info->mailbox_name;
	attachment_tbl.attachment_name = updated_attachment->name;
	attachment_tbl.attachment_size = updated_attachment->size;
	attachment_tbl.attachment_path = updated_attachment->savename;
	attachment_tbl.file_yn         = updated_attachment->downloaded;
	attachment_tbl.flag1           = updated_attachment->drm;
	attachment_tbl.flag3           = updated_attachment->inline_content;
	attachment_tbl.attachment_id   = updated_attachment->attachment_id;

	char buf[512];
	int attachment_id = 0;

	/* EM_DEBUG_LOG("file is downloaded and different from old one. it should be moved"); */

	if (!updated_attachment->inline_content) {
		if (!em_storage_create_dir(attachment_tbl.account_id, mail_id, attachment_tbl.attachment_id, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}
		attachment_id = attachment_tbl.attachment_id;
	}
	
	if (!em_storage_get_save_name(attachment_tbl.account_id, mail_id, attachment_id, updated_attachment->name, buf, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
		goto FINISH_OFF2;
	}
    attachment_tbl.attachment_path = buf;

	EM_DEBUG_LOG("downloaded [%d], savename [%s], attachment_path [%s]", updated_attachment->downloaded, updated_attachment->savename, existing_attachment_info->attachment_path);
	if (updated_attachment->downloaded && EM_SAFE_STRCMP(updated_attachment->savename, existing_attachment_info->attachment_path) != 0) {
		if (!em_storage_move_file(updated_attachment->savename, buf, false ,&err))  {
			EM_DEBUG_EXCEPTION("em_storage_move_file failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF2;
		}
	}
	else
		EM_DEBUG_LOG("no need to move");

	EM_SAFE_FREE(updated_attachment->savename);
	updated_attachment->savename = EM_SAFE_STRDUP(buf);

	em_storage_begin_transaction(NULL, NULL, NULL);
	
	if (!em_storage_update_attachment(&attachment_tbl, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_update_attachment failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF: 
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (em_storage_commit_transaction(NULL, NULL, NULL) == false) {
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
			ret = false;
		}
	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (em_storage_rollback_transaction(NULL, NULL, NULL) == false)
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
	}	

FINISH_OFF2: 
	if (existing_attachment_info)
		em_storage_free_attachment(&existing_attachment_info, 1, NULL);

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}

static int em_core_mail_update_attachment_data(int input_mail_id, emf_attachment_data_t *input_attachment_data)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], input_attachment_data[%p]", input_mail_id, input_attachment_data);

	int                        err = EMF_ERROR_NONE;
	int                        attachment_id = 0;
	char                       buf[512] = { 0 , };
	emf_mail_attachment_tbl_t *existing_attachment_info = NULL;
	emf_mail_attachment_tbl_t  attachment_tbl = { 0 };

	if (input_attachment_data == NULL)  {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF2;
	}

	if (!em_storage_get_attachment(input_mail_id, input_attachment_data->attachment_id, &existing_attachment_info, 1, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_get_attachment failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF2;
	}
	
	attachment_tbl.mail_id         = input_mail_id;
	attachment_tbl.account_id      = existing_attachment_info->account_id;
	attachment_tbl.mailbox_name    = existing_attachment_info->mailbox_name;
	attachment_tbl.attachment_name = input_attachment_data->attachment_name;
	attachment_tbl.attachment_size = input_attachment_data->attachment_size;
	attachment_tbl.attachment_path = input_attachment_data->attachment_path;
	attachment_tbl.file_yn         = input_attachment_data->save_status;
	attachment_tbl.flag1           = input_attachment_data->drm_status;
	attachment_tbl.flag3           = input_attachment_data->inline_content_status;
	attachment_tbl.attachment_id   = input_attachment_data->attachment_id;

	if (!input_attachment_data->inline_content_status) {
		if (!em_storage_create_dir(attachment_tbl.account_id, input_mail_id, attachment_tbl.attachment_id, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
			goto FINISH_OFF;
		}
		attachment_id = attachment_tbl.attachment_id;
	}
	
	if (!em_storage_get_save_name(attachment_tbl.account_id, input_mail_id, attachment_id, input_attachment_data->attachment_name, buf, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
		goto FINISH_OFF2;
	}
    attachment_tbl.attachment_path = buf;

	EM_DEBUG_LOG("downloaded [%d], savename [%s], attachment_path [%s]", input_attachment_data->save_status, input_attachment_data->attachment_path, existing_attachment_info->attachment_path);
	if (input_attachment_data->save_status && EM_SAFE_STRCMP(input_attachment_data->attachment_path, existing_attachment_info->attachment_path) != 0) {
		if (!em_storage_move_file(input_attachment_data->attachment_path, buf, false ,&err))  {
			EM_DEBUG_EXCEPTION("em_storage_move_file failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF2;
		}
	}
	else
		EM_DEBUG_LOG("no need to move");

	EM_SAFE_FREE(input_attachment_data->attachment_path);
	input_attachment_data->attachment_path = EM_SAFE_STRDUP(buf);

	em_storage_begin_transaction(NULL, NULL, NULL);
	
	if (!em_storage_update_attachment(&attachment_tbl, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_update_attachment failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
FINISH_OFF: 
	if (err == EMF_ERROR_NONE) {	/*  COMMIT TRANSACTION; */
		if (em_storage_commit_transaction(NULL, NULL, NULL) == false) {
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
		}
	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (em_storage_rollback_transaction(NULL, NULL, NULL) == false)
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
	}	

FINISH_OFF2: 
	if (existing_attachment_info)
		em_storage_free_attachment(&existing_attachment_info, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


static int em_core_mail_compare_filename_of_attachment(int mail_id, int attachment_a_id, emf_attachment_info_t *attachment_b_info, int *result, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], attachment_a_id[%d], attachment_b_info[%p], result[%p], err_code[%p]", mail_id, attachment_a_id, attachment_b_info, result, err_code);

	EM_IF_NULL_RETURN_VALUE(attachment_b_info, false);
	EM_IF_NULL_RETURN_VALUE(result, false);

	int err, err_2, ret = false;
	emf_mail_attachment_tbl_t *attachment_a_tbl = NULL;

	if (!em_storage_get_attachment(mail_id, attachment_a_id, &attachment_a_tbl, 1, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_get_attachment failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	if (attachment_a_tbl->attachment_name && attachment_b_info->savename) {
		EM_DEBUG_LOG("attachment_a_tbl->attachment_name [%s], ttachment_b_info->name [%s]", attachment_a_tbl->attachment_name, attachment_b_info->name);
		*result = strcmp(attachment_a_tbl->attachment_name, attachment_b_info->name);
	}

	ret = true;

FINISH_OFF: 

	if (attachment_a_tbl)
		em_storage_free_attachment(&attachment_a_tbl, 1, &err_2);
	EM_DEBUG_FUNC_END("*result [%d]", *result);
	return ret;
}

static int em_core_mail_compare_filename_of_attachment_data(int input_mail_id, int input_attachment_a_id, emf_attachment_data_t *input_attachment_b_data, int *result)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], input_attachment_a_id[%d], input_attachment_b_data[%p], result[%p]", input_mail_id, input_attachment_a_id, input_attachment_b_data, result);

	EM_IF_NULL_RETURN_VALUE(input_attachment_b_data, false);
	EM_IF_NULL_RETURN_VALUE(result, false);

	int err, err_2, ret = EMF_ERROR_NONE;
	emf_mail_attachment_tbl_t *attachment_a_tbl = NULL;

	if (!em_storage_get_attachment(input_mail_id, input_attachment_a_id, &attachment_a_tbl, 1, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_get_attachment failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	if (attachment_a_tbl->attachment_name && input_attachment_b_data->attachment_name) {
		EM_DEBUG_LOG("attachment_a_tbl->attachment_name [%s], input_attachment_b_data->name [%s]", attachment_a_tbl->attachment_name, input_attachment_b_data->attachment_name);
		*result = strcmp(attachment_a_tbl->attachment_name, input_attachment_b_data->attachment_name);
	}

	ret = true;

FINISH_OFF: 

	if (attachment_a_tbl)
		em_storage_free_attachment(&attachment_a_tbl, 1, &err_2);
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
EXPORT_API int em_core_mail_copy(int mail_id, emf_mailbox_t *dst_mailbox, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], dst_mailbox[%p], err_code[%p]", mail_id, dst_mailbox, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int i;
	emf_mail_tbl_t *mail = NULL;
	emf_mail_attachment_tbl_t *atch_list = NULL;
	char buf[512];
	int count = EMF_ATTACHMENT_MAX_COUNT;
	char *mailbox_name = NULL;

	if (!em_storage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	if ( (err = em_storage_get_attachment_list(mail_id, true, &atch_list, &count)) != EM_STORAGE_ERROR_NONE ){
		EM_DEBUG_EXCEPTION("em_storage_get_attachment_list failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	/*  get increased uid. */
	if (!em_storage_increase_mail_id(&mail->mail_id, true, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_increase_mail_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	/*  copy mail body(text) file */
	if (mail->file_path_plain)  {
		if (!em_storage_create_dir(dst_mailbox->account_id, mail->mail_id, 0, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		gchar *filename = g_path_get_basename(mail->file_path_plain);
		
		if (!em_storage_get_save_name(dst_mailbox->account_id, mail->mail_id, 0, filename, buf, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
			g_free(filename);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		g_free(filename);
		
		if (!em_storage_copy_file(mail->file_path_plain, buf, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_copy_file failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		mail->file_path_plain = EM_SAFE_STRDUP(buf);
	}	
	
	/*  copy mail body(html) file */
	if (mail->file_path_html)  {
		if (!em_storage_create_dir(dst_mailbox->account_id, mail->mail_id, 0, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		gchar *filename = g_path_get_basename(mail->file_path_html);
		
		if (!em_storage_get_save_name(dst_mailbox->account_id, mail->mail_id, 0, filename, buf, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
			g_free(filename);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		g_free(filename);
		
		if (!em_storage_copy_file(mail->file_path_html, buf, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_copy_file failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		mail->file_path_html = EM_SAFE_STRDUP(buf);
	}	

	/*  BEGIN TRANSACTION; */
	em_storage_begin_transaction(NULL, NULL, NULL);
	
	/*  insert mail data */
	EM_SAFE_FREE(mail->mailbox_name);
	
	mail->account_id   = dst_mailbox->account_id;		/*  MUST BE */
	mail->mailbox_name = EM_SAFE_STRDUP(dst_mailbox->name);
	mail->mailbox_type = dst_mailbox->mailbox_type;
	
	if (!em_storage_add_mail(mail, 0, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_add_mail failed [%d]", err);
		
		if (mail->file_path_plain)  {
			if (!em_storage_delete_file(mail->file_path_plain, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_delete_file failed [%d]", err);
				em_storage_rollback_transaction(NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}
		if (mail->file_path_html) {
			if (!em_storage_delete_file(mail->file_path_html, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_delete_file failed [%d]", err);
				em_storage_rollback_transaction(NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}
		
		/*  ROLLBACK TRANSACTION; */
		em_storage_rollback_transaction(NULL, NULL, NULL);
		
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	/*  copy attachment file */
	for (i = 0; i<count; i++)  {
		if (atch_list[i].attachment_path)  {
			if (!em_storage_create_dir(dst_mailbox->account_id, mail->mail_id, i+1, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
				break;
			}
			
			if (!em_storage_get_save_name(dst_mailbox->account_id, mail->mail_id, i+1, atch_list[i].attachment_name, buf, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
				break;
			}
			
			if (!em_storage_copy_file(atch_list[i].attachment_path, buf, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_copy_file failed [%d]", err);
				break;
			}
			
			EM_SAFE_FREE(atch_list[i].attachment_path);
			
			atch_list[i].attachment_path = EM_SAFE_STRDUP(buf);
		}
		
		atch_list[i].account_id = dst_mailbox->account_id;
		atch_list[i].mail_id = mail->mail_id;
		
		EM_SAFE_FREE(atch_list[i].mailbox_name);
		
		atch_list[i].mailbox_name = EM_SAFE_STRDUP(mail->mailbox_name);
		
		if (!em_storage_add_attachment(&atch_list[i], 0, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_add_attachment failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			break;
		}
	}
	
	/*  in case error happened, delete copied file. */
	if (i && i != count)  {
		for (;i >= 0; i--)  {
			if (atch_list[i].attachment_path)  {
			
				if (!em_storage_delete_file(atch_list[i].attachment_path, &err))  {
					EM_DEBUG_EXCEPTION("em_storage_delete_file failed [%d]", err);
					em_storage_rollback_transaction(NULL, NULL, NULL);
					goto FINISH_OFF;
				}
			}
		}
		
		if (mail->file_path_plain)  {
			if (!em_storage_delete_file(mail->file_path_plain, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_delete_file failed [%d]", err);
				em_storage_rollback_transaction(NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}
		if (mail->file_path_html)  {
			if (!em_storage_delete_file(mail->file_path_html, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_delete_file failed [%d]", err);
				em_storage_rollback_transaction(NULL, NULL, NULL);
				goto FINISH_OFF;
			}
		}
		
		/*  ROLLBACK TRANSACTION; */
		em_storage_rollback_transaction(NULL, NULL, NULL);
		goto FINISH_OFF;
	}
	
	if (em_storage_commit_transaction(NULL, NULL, NULL) == false) {
		err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
		if (em_storage_rollback_transaction(NULL, NULL, NULL) == false)
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
		goto FINISH_OFF;
	}

	if (!em_storage_get_mailboxname_by_mailbox_type(dst_mailbox->account_id, EMF_MAILBOX_TYPE_INBOX, &mailbox_name, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	if (!strcmp(dst_mailbox->name, mailbox_name) && !(mail->flags_seen_field))
		em_core_check_unread_mail();
	
	ret = true;
	
FINISH_OFF: 
	if (atch_list != NULL)
		em_storage_free_attachment(&atch_list, count, NULL);
	
	if (mail != NULL)
		em_storage_free_mail(&mail, 1, NULL);
	
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
EXPORT_API int em_core_mail_move(int mail_ids[], int mail_ids_count, char *dst_mailbox_name, int noti_param_1, int noti_param_2 ,int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], mail_ids_count[%d], dst_mailbox_name[%s], noti_param [%d], err_code[%p]", mail_ids, mail_ids_count, dst_mailbox_name, noti_param_1, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_mail_tbl_t *mail_list = NULL;
	int account_id = 0;
	int i = 0, parameter_string_length = 0;
	char *parameter_string = NULL, mail_id_string[10];
	emf_mail_attachment_tbl_t attachment;

	if (!dst_mailbox_name && mail_ids_count < 1) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!em_storage_get_mail_field_by_multiple_mail_id(mail_ids, mail_ids_count, RETRIEVE_FLAG, &mail_list, true, &err) || !mail_list) {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_field_by_multiple_mail_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
 
	account_id = mail_list[0].account_id;

	if(!em_storage_move_multiple_mails(account_id, dst_mailbox_name, mail_ids, mail_ids_count, true, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_move_multiple_mails failed [%d]", err);
		goto FINISH_OFF;
	}
	
	/* Sending a notification */
	parameter_string_length = sizeof(char) * (mail_ids_count * 10 + 128/*MAILBOX_LEN_IN_MAIL_TBL*/ * 2);
	parameter_string = em_core_malloc(parameter_string_length);

	if (mail_list[0].mailbox_name)
		SNPRINTF(parameter_string, parameter_string_length, "%s%c%s%c", mail_list[0].mailbox_name, 0x01, dst_mailbox_name , 0x01);

	if (parameter_string == NULL) {
		EM_DEBUG_EXCEPTION("Memory allocation for mail_id_list_string failed");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < mail_ids_count; i++) {
		memset(mail_id_string, 0, 10);
		SNPRINTF(mail_id_string, 10, "%d,", mail_ids[i]);
		strcat(parameter_string, mail_id_string);
	}
	
	EM_DEBUG_LOG("num : [%d], param string : [%s]", mail_ids_count , parameter_string);

	if (!em_storage_notify_storage_event(NOTI_MAIL_MOVE, account_id, noti_param_1, parameter_string, noti_param_2)) 
		EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event failed [NOTI_MAIL_MOVE] >>>> ");

	/* Upating thread mail info should be done as soon as possible */ 
	attachment.mailbox_name = dst_mailbox_name;

	for (i = 0; i < mail_ids_count; i++) {
		if (!em_storage_update_latest_thread_mail(account_id, mail_list[i].thread_id, 0, 0, true, &err))
			EM_DEBUG_EXCEPTION("em_storage_update_latest_thread_mail failed [%d]", err);
	}

	if (!em_storage_notify_storage_event(NOTI_MAIL_MOVE_FINISH, account_id, noti_param_1, parameter_string, noti_param_2)) 
		EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event failed [NOTI_MAIL_MOVE_FINISH] >>>> ");

	em_core_check_unread_mail();

	ret = true;
	
FINISH_OFF: 
	em_storage_free_mail(&mail_list, mail_ids_count, NULL);
	EM_SAFE_FREE(parameter_string);
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);			
	return ret;
}


EXPORT_API int em_core_mail_move_from_server(int account_id, char *src_mailbox,  int mail_ids[], int num, char *dest_mailbox, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN();
	MAILSTREAM *stream = NULL;
	int err_code = 0;
	emf_account_t *ref_account = NULL;
	emf_mail_tbl_t *mail = NULL;
	int ret = 1;
	int mail_id = 0;
	int i = 0;
	
	mail_id = mail_ids[0];
	
	ref_account = em_core_get_account_reference(account_id);
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("em_core_mail_move_from_server failed  :  get account reference[%d]", account_id);
		*error_code = EMF_ERROR_INVALID_ACCOUNT;
		ret = 0;
		goto FINISH_OFF;
	}

		    /* if not imap4 mail, return */
 	if (ref_account->account_bind_type != EMF_BIND_TYPE_EM_CORE || 
		ref_account->receiving_server_type != EMF_SERVER_TYPE_IMAP4) {
		*error_code = EMF_ERROR_INVALID_PARAM;
		ret = 0;
		goto FINISH_OFF;
	}

	for (i = 0; i < num; i++)  {
		mail_id = mail_ids[i];
			
		if (!em_storage_get_mail_by_id(mail_id, &mail, false, &err_code) || !mail)  {
			EM_DEBUG_EXCEPTION("em_storage_get_uid_by_mail_id  :  em_storage_get_downloaded_mail failed [%d]", err_code);
			mail = NULL;		
			if (err_code == EM_STORAGE_ERROR_MAIL_NOT_FOUND)  {	/*  not server mail */
				/* err = EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER; */
				/* continue; */
			}
			else {
				*error_code = em_storage_get_emf_error_from_em_storage_error(err_code);
			}

			ret = 0;
			goto FINISH_OFF;
		}

		if (!em_core_mailbox_open(account_id, src_mailbox, (void **)&stream, &err_code))		/* faizan.h@samsung.com mail_move_fix_07042009 */ {
			EM_DEBUG_EXCEPTION("em_core_mail_move_from_server failed :  Mailbox open[%d]", err_code);
			
			ret = 0;
			goto FINISH_OFF;
		}

		if (stream) {
			/* set callback for COPY_UID */
			mail_parameters(stream, SET_COPYUID, em_core_mail_copyuid);
			
			EM_DEBUG_LOG("calling mail_copy_full FODLER MAIL COPY ");
			
			if (mail->server_mail_id) {
				if (!mail_copy_full(stream, mail->server_mail_id, dest_mailbox, CP_UID | CP_MOVE)) {
					EM_DEBUG_EXCEPTION("em_core_mail_move_from_server :   Mail cannot be moved failed");
					ret = 0;
				}
				else {
					/*  send EXPUNGE command */
					if (!imap4_send_command(stream, IMAP4_CMD_EXPUNGE, &err_code)) {
						EM_DEBUG_EXCEPTION("imap4_send_command failed [%d]", err_code);
						
						if (err_code == EMF_ERROR_IMAP4_STORE_FAILURE)
							err_code = EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER;
						goto FINISH_OFF;
					}					
					
					/* faizan.h@samsung.com   duplicate copy while sync mailbox issue fixed */
					EM_DEBUG_LOG(">>>mailbox_name[%s]>>>>>>", mail->mailbox_name);
					EM_DEBUG_LOG(">>>g_new_server_uid[%s]>>>>>>", g_new_server_uid);
					EM_DEBUG_LOG(">>>mail_id[%d]>>>>>>", mail_id);

					if (!em_storage_update_read_mail_uid(mail_id, g_new_server_uid, mail->mailbox_name, &err_code)) {
						EM_DEBUG_EXCEPTION("em_storage_update_read_mail_uid failed [%d]", err_code);
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
	if (stream) em_core_mailbox_close(account_id, stream);

	if (mail != NULL)
		em_storage_free_mail(&mail, 1, NULL);
	EM_DEBUG_FUNC_END("ret [%d]", ret);		
	return ret;
}

static int em_core_mail_save_file(int account_id, int mail_id, int attachment_id, char *src_file_path, char *file_name, char *full_path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], attachment_id[%d] , file_name[%p] , full_path[%p] , err_code[%p]", account_id, mail_id, attachment_id, file_name, full_path, err_code);

	int ret = false, err = EMF_ERROR_NONE;

	if (!file_name || !full_path || !src_file_path) {
		EM_DEBUG_EXCEPTION("Invalid paramter");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!em_storage_create_dir(account_id, mail_id, attachment_id, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_create_dir failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!em_storage_get_save_name(account_id, mail_id, attachment_id, file_name, full_path, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_get_save_name failed [%d]", err);
		goto FINISH_OFF;
	}

	if (strcmp(src_file_path, full_path) != 0)  {
		if (!em_storage_copy_file(src_file_path, full_path, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_copy_file failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
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
 * arguments  
 *    mailbox  :  mail box
 *    mail_id  :  mail id to be updated
 *    mail  :  updating mail header, body, flags, extra_flags
 * return  
 *    succeed  :  1
 *    fail  :  0
 */
EXPORT_API int em_core_mail_update_old(int mail_id,  emf_mail_t *src_mail, emf_meeting_request_t *src_meeting_req,  int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], mail[%p], err_code[%p]", mail_id, src_mail, err_code);

	int ret = false, err = EMF_ERROR_NONE;
	int local_attachment_count = 0, local_inline_content_count = 0;
	emf_mail_tbl_t mail_table_data = {0};
	emf_attachment_info_t *attachment_info = NULL;
	char filename_buf[1024] = {0, }, buf_date[512] = {0, }; 
	char html_body[MAX_PATH] = {0, };
	
	if (!mail_id || !src_mail || !src_mail->info || !src_mail->head)  {	
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF2;
	}
	
	mail_table_data.full_address_from    = src_mail->head->from;
	mail_table_data.full_address_reply   = src_mail->head->reply_to;
	mail_table_data.full_address_to      = src_mail->head->to;
	mail_table_data.full_address_cc      = src_mail->head->cc;
	mail_table_data.full_address_bcc     = src_mail->head->bcc;
	mail_table_data.full_address_return  = src_mail->head->return_path;
	mail_table_data.subject              = src_mail->head->subject;
	mail_table_data.preview_text         = src_mail->head->previewBodyText ;
	mail_table_data.body_download_status = src_mail->info->body_downloaded;
	mail_table_data.mailbox_name         = NULL;	/*  UPDATE_MAIL type couldn't change mailbox.  */
	mail_table_data.priority             = src_mail->info->extra_flags.priority;
	mail_table_data.report_status        = src_mail->info->extra_flags.report;
	mail_table_data.save_status          = src_mail->info->extra_flags.status;
	mail_table_data.lock_status          = src_mail->info->extra_flags.lock;
	mail_table_data.DRM_status           = src_mail->info->extra_flags.drm;
	mail_table_data.mail_size			 = src_mail->info->rfc822_size;

	if(mail_table_data.mail_size == 0)
		mail_table_data.mail_size = em_core_get_mail_size(src_mail, &err);

	if (!mail_table_data.full_address_from)  {
		emf_account_t *ref_account = NULL;
		if (!(ref_account = em_core_get_account_reference(src_mail->info->account_id)))  {
			EM_DEBUG_EXCEPTION("em_core_get_account_reference failed [%d]", src_mail->info->account_id);
			err = EMF_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF2;
		}
		mail_table_data.full_address_from = ref_account->email_addr;
	}

	if(!em_convert_mail_flag_to_mail_tbl(&(src_mail->info->flags), &mail_table_data, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_flag_to_mail_tbl failed [%d]", err);
		goto FINISH_OFF2;
	}
	
	if (src_mail->body && src_mail->body->plain)  {   /*  Save plain text body. */
		if (!em_core_mail_save_file(src_mail->info->account_id, mail_id, 0, src_mail->body->plain, src_mail->body->plain_charset ? src_mail->body->plain_charset  :  "UTF-8", filename_buf, &err)) {
			EM_DEBUG_EXCEPTION("em_core_mail_save_file failed [%d]", err);
			goto FINISH_OFF2;
		}
		mail_table_data.file_path_plain = EM_SAFE_STRDUP(filename_buf);
	} 
	else 
		mail_table_data.file_path_plain = NULL;
	
	if (src_mail->body && src_mail->body->html)  {   /*  Save HTML text body. */
		if (src_mail->body->plain_charset != NULL && src_mail->body->plain_charset[0] != '\0') {
			memcpy(html_body, src_mail->body->plain_charset, strlen(src_mail->body->plain_charset));
			strcat(html_body, ".htm");
		}
		else
			memcpy(html_body, "UTF-8.htm", strlen("UTF-8.htm"));
		if (!em_core_mail_save_file(src_mail->info->account_id, mail_id, 0, src_mail->body->html, html_body, filename_buf, &err)) {
			EM_DEBUG_EXCEPTION("em_core_mail_save_file failed [%d]", err);
			goto FINISH_OFF2;
		}
		mail_table_data.file_path_html = EM_SAFE_STRDUP(filename_buf);
	} 
	else 
		mail_table_data.file_path_html = NULL;
	
	if (src_mail->body && src_mail->body->attachment)  {
		int compare_result = 1;
		char attachment_id[20];

		attachment_info = src_mail->body->attachment;
		do  {
			if (!em_core_mail_compare_filename_of_attachment(mail_id, attachment_info->attachment_id, attachment_info, &compare_result, &err)) {
				EM_DEBUG_EXCEPTION("em_core_mail_compare_filename_of_attachment failed [%d]", err);
				compare_result = 1;
			}
	
			if (compare_result == 0) {
				EM_DEBUG_LOG("file name and attachment id are same, update exising attachment");
				if (!em_core_mail_update_attachment(mail_id, attachment_info, &err))  {
					EM_DEBUG_EXCEPTION("em_core_mail_update_attachment failed [%d]", err);
					goto FINISH_OFF2;
				}
			}
			else {
				EM_DEBUG_LOG("save names are different");
				SNPRINTF(attachment_id, sizeof(attachment_id), "%d", attachment_info->attachment_id);
			
				if(attachment_info->attachment_id > 0) {
					if (!em_core_mail_delete_attachment(mail_id, attachment_id, &err)) {
						EM_DEBUG_EXCEPTION("em_core_mail_delete_attachment failed [%d]", err);
						goto FINISH_OFF2;
					}
				}

				if (!em_core_mail_add_attachment(mail_id, attachment_info, &err))  {
					EM_DEBUG_EXCEPTION("em_core_mail_add_attachment failed [%d]", err);
					goto FINISH_OFF2;
				}
			}

			if (attachment_info->inline_content)
				local_inline_content_count++;
			local_attachment_count++;
			attachment_info = attachment_info->next;
		}
		while (attachment_info != NULL);
	}
	
	mail_table_data.attachment_count     = local_attachment_count;
	mail_table_data.inline_content_count = local_inline_content_count;
	
	if (!src_mail->head->datetime.year && !src_mail->head->datetime.month && !src_mail->head->datetime.day)  {
		time_t t = time(NULL);	
		struct tm *p_tm = localtime(&t);
		
		if (!p_tm)  {
			EM_DEBUG_EXCEPTION("localtime failed...");
			err = EM_STORAGE_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF2;
		}
		
		SNPRINTF(buf_date, sizeof(buf_date), "%04d%02d%02d%02d%02d%02d", 
			p_tm->tm_year + 1900, 
			p_tm->tm_mon + 1, 
			p_tm->tm_mday, 
			p_tm->tm_hour, 
			p_tm->tm_min, 
			p_tm->tm_sec);
	}
	else  {
		SNPRINTF(buf_date, sizeof(buf_date), "%04d%02d%02d%02d%02d%02d", 
			src_mail->head->datetime.year, 
			src_mail->head->datetime.month, 
			src_mail->head->datetime.day, 
			src_mail->head->datetime.hour, 
			src_mail->head->datetime.minute, 
			src_mail->head->datetime.second);
	}
	
	mail_table_data.datetime = buf_date;
	
	EM_DEBUG_LOG("preview_text[%p]", mail_table_data.preview_text);
	if (mail_table_data.preview_text == NULL) {
		if ( (err = em_core_get_preview_text_from_file(mail_table_data.file_path_plain, mail_table_data.file_path_html, MAX_PREVIEW_TEXT_LENGTH, &(mail_table_data.preview_text))) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_core_get_preview_text_from_file failed[%d]", err);
			goto FINISH_OFF2;
		}
	}

	mail_table_data.meeting_request_status = src_mail->info->is_meeting_request;

	/*  BEGIN TRANSACTION; */
	em_storage_begin_transaction(NULL, NULL, NULL);
	
	if (!em_storage_change_mail_field(mail_id, UPDATE_MAIL, &mail_table_data, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_change_mail_field failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	if (src_mail->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_REQUEST
		|| src_mail->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_RESPONSE
		|| src_mail->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		emf_meeting_request_t *meeting_req = NULL;
		/*  check where there is a meeting request in DB */
		if (!em_storage_get_meeting_request(mail_id, &meeting_req, false, &err) && err != EM_STORAGE_ERROR_DATA_NOT_FOUND) {
			EM_DEBUG_EXCEPTION("em_storage_get_meeting_request failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		EM_SAFE_FREE(meeting_req);
		if (err == EM_STORAGE_ERROR_DATA_NOT_FOUND) {	/*  insert */
			emf_mail_tbl_t *original_mail = NULL;
			
			if (!em_storage_get_mail_by_id(mail_id, &original_mail, false, &err)) {
				EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}
				
			if (original_mail)	 {
			if (!em_storage_add_meeting_request(src_mail->info->account_id, original_mail->mailbox_name, src_meeting_req, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_add_meeting_request failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}	
				em_storage_free_mail(&original_mail, 1, NULL);
		}
		}
		else {	/*  update */
			if (!em_storage_update_meeting_request(src_meeting_req, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_update_meeting_request failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}
		}
	}
	
	ret = true;
	
FINISH_OFF: 
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (em_storage_commit_transaction(NULL, NULL, NULL) == false) {
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
			ret = false;
		}
		/*  publish a notification  :  to Active Sync Engine */
		if (!em_storage_notify_storage_event(NOTI_MAIL_UPDATE, src_mail->info->account_id, mail_id, NULL, UPDATE_MEETING))
			EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event failed [ NOTI_MAIL_UPDATE  :  UPDATE_MEETING_RESPONSE ] >>>> ");
	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (em_storage_rollback_transaction(NULL, NULL, NULL) == false)
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
	}	
	
FINISH_OFF2: 

	EM_SAFE_FREE(mail_table_data.file_path_plain);
	EM_SAFE_FREE(mail_table_data.file_path_html);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d]", ret);		
	return ret;
}


/* description 
 *    update mail information
 *    'em_core_mail_update_old' will be replaced with this function.
 */
EXPORT_API int em_core_update_mail(emf_mail_data_t *input_mail_data, emf_attachment_data_t *input_attachment_data_list, int input_attachment_count, emf_meeting_request_t* input_meeting_request, int input_sync_server)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list[%p], input_attachment_count[%d], input_meeting_request[%p], input_sync_server[%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_sync_server);

	char            filename_buf[1024]         = {0, };
	int             i                          = 0;
	int             err                        = EMF_ERROR_NONE;
	int             local_inline_content_count = 0;
	emf_mail_tbl_t *converted_mail_tbl_data    = NULL;
	struct stat     st_buf;
	
	
	if (!input_mail_data || (input_attachment_count && !input_attachment_data_list) || (!input_attachment_count &&input_attachment_data_list))  {	
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF2;
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
	
	if(input_mail_data->mail_size == 0) {
		 em_core_calc_mail_size(input_mail_data, input_attachment_data_list, input_attachment_count, &(input_mail_data->mail_size));
	}

	if (!input_mail_data->full_address_from)  {
		emf_account_t *ref_account = NULL;
		if (!(ref_account = em_core_get_account_reference(input_mail_data->account_id)))  {
			EM_DEBUG_EXCEPTION("em_core_get_account_reference failed [%d]", input_mail_data->account_id);
			err = EMF_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF2;
		}
		input_mail_data->full_address_from = ref_account->email_addr;
	}
	
	if (input_mail_data->file_path_plain)  {   /*  Save plain text body. */
		if (!em_core_mail_save_file(input_mail_data->account_id, input_mail_data->mail_id, 0, input_mail_data->file_path_plain, "UTF-8", filename_buf, &err)) {
			EM_DEBUG_EXCEPTION("em_core_mail_save_file failed [%d]", err);
			goto FINISH_OFF2;
		}
		EM_SAFE_FREE(input_mail_data->file_path_plain);
		input_mail_data->file_path_plain = EM_SAFE_STRDUP(filename_buf);
	} 
	
	if (input_mail_data->file_path_html)  {   /*  Save HTML text body. */
		if (!em_core_mail_save_file(input_mail_data->account_id, input_mail_data->mail_id, 0, input_mail_data->file_path_html, "UTF-8.htm", filename_buf, &err)) {
			EM_DEBUG_EXCEPTION("em_core_mail_save_file failed [%d]", err);
			goto FINISH_OFF2;
		}
		EM_SAFE_FREE(input_mail_data->file_path_html);
		input_mail_data->file_path_html = EM_SAFE_STRDUP(filename_buf);
	} 
	
	if (input_attachment_data_list && input_attachment_count)  {
		int i = 0;
		int compare_result = 1;
		char attachment_id[20];
		emf_attachment_data_t *temp_attachment_data = NULL;

		for(i = 0; i < input_attachment_count; i++) {
			temp_attachment_data = input_attachment_data_list + i;
			if ( (err = em_core_mail_compare_filename_of_attachment_data(input_mail_data->mail_id, temp_attachment_data->attachment_id, temp_attachment_data, &compare_result)) != EMF_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("em_core_mail_compare_filename_of_attachment failed [%d]", err);
				compare_result = 1;
			}
	
			if (compare_result == 0) {
				EM_DEBUG_LOG("file name and attachment id are same, update exising attachment");
				if (!em_core_mail_update_attachment_data(input_mail_data->mail_id, temp_attachment_data))  {
					EM_DEBUG_EXCEPTION("em_core_mail_update_attachment failed [%d]", err);
					goto FINISH_OFF2;
				}
			}
			else {
				EM_DEBUG_LOG("save names are different");
				SNPRINTF(attachment_id, sizeof(attachment_id), "%d", temp_attachment_data->attachment_id);
			
				if(temp_attachment_data->attachment_id > 0) {
					if (!em_core_mail_delete_attachment(input_mail_data->mail_id, attachment_id, &err)) {
						EM_DEBUG_EXCEPTION("em_core_mail_delete_attachment failed [%d]", err);
						goto FINISH_OFF2;
					}
				}

				if ( (err = em_core_mail_add_attachment_data(input_mail_data->mail_id, temp_attachment_data)) != EMF_ERROR_NONE)  {
					EM_DEBUG_EXCEPTION("em_core_mail_add_attachment failed [%d]", err);
					goto FINISH_OFF2;
				}
			}

			if (temp_attachment_data->inline_content_status)
				local_inline_content_count++;
		}
	}
	
	input_mail_data->attachment_count     = input_attachment_count;
	input_mail_data->inline_content_count = local_inline_content_count;

	if (!input_mail_data->datetime)  {
		/* time isn't set */
		input_mail_data->datetime = em_core_get_current_time_string(&err);
		if(!input_mail_data->datetime) {
			EM_DEBUG_EXCEPTION("em_core_get_current_time_string failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG("preview_text[%p]", input_mail_data->preview_text);
	if (input_mail_data->preview_text == NULL) {
		if ( (err =em_core_get_preview_text_from_file(input_mail_data->file_path_plain, input_mail_data->file_path_html, MAX_PREVIEW_TEXT_LENGTH, &(input_mail_data->preview_text))) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_core_get_preview_text_from_file failed[%d]", err);
			goto FINISH_OFF2;
		}
	}

	if(!em_convert_mail_data_to_mail_tbl(input_mail_data, 1, &converted_mail_tbl_data,  &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_data_to_mail_tbl failed[%d]", err);
		goto FINISH_OFF2;
	}
	
	/*  BEGIN TRANSACTION; */
	em_storage_begin_transaction(NULL, NULL, NULL);
	
	if (!em_storage_change_mail_field(input_mail_data->mail_id, UPDATE_MAIL, converted_mail_tbl_data, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_change_mail_field failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	if (input_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_REQUEST
		|| input_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_RESPONSE
		|| input_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		emf_meeting_request_t *meeting_req = NULL;
		/*  check where there is a meeting request in DB */
		if (!em_storage_get_meeting_request(input_mail_data->mail_id, &meeting_req, false, &err) && err != EM_STORAGE_ERROR_DATA_NOT_FOUND) {
			EM_DEBUG_EXCEPTION("em_storage_get_meeting_request failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		EM_SAFE_FREE(meeting_req);
		if (err == EM_STORAGE_ERROR_DATA_NOT_FOUND) {	/*  insert */
			emf_mail_tbl_t *original_mail = NULL;
			
			if (!em_storage_get_mail_by_id(input_mail_data->mail_id, &original_mail, false, &err)) {
				EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}
				
			if (original_mail)	 {
			if (!em_storage_add_meeting_request(input_mail_data->account_id, original_mail->mailbox_name, input_meeting_request, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_add_meeting_request failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}	
				em_storage_free_mail(&original_mail, 1, NULL);
		}
		}
		else {	/*  update */
			if (!em_storage_update_meeting_request(input_meeting_request, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_update_meeting_request failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}
		}
	}
	
FINISH_OFF: 
	if (err == EMF_ERROR_NONE) {
		/*  COMMIT TRANSACTION; */
		if (em_storage_commit_transaction(NULL, NULL, NULL) == false) {
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
		}

		if (input_meeting_request && !em_storage_notify_storage_event(NOTI_MAIL_UPDATE, input_mail_data->account_id, input_mail_data->mail_id, NULL, UPDATE_MEETING))
			EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event failed [ NOTI_MAIL_UPDATE : UPDATE_MEETING_RESPONSE ] >>>> ");
	}
	else {	
		/*  ROLLBACK TRANSACTION; */
		if (em_storage_rollback_transaction(NULL, NULL, NULL) == false)
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
	}	
	
FINISH_OFF2: 
	if(converted_mail_tbl_data)
		em_storage_free_mail(&converted_mail_tbl_data, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);		
	return err;
}

EXPORT_API int em_core_fetch_flags(int account_id, int mail_id, emf_mail_flag_t *mail_flag, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int err = 0;
	emf_mail_tbl_t *mail = NULL;
	
	EM_IF_NULL_RETURN_VALUE(mail_flag, false);
	if (account_id < FIRST_ACCOUNT_ID || mail_id < 1) {
		EM_DEBUG_EXCEPTION("em_core_fetch_flags :  Invalid Param ");
		goto FINISH_OFF;
	}

	if (!em_storage_get_mail_field_by_id(mail_id, RETRIEVE_FLAG, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_field_by_id failed - %d ", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	if (!em_convert_mail_tbl_to_mail_flag(mail, mail_flag, &err))  {
		EM_DEBUG_EXCEPTION("em_convert_mail_flag_to_int failed - %d ", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF: 
	if(mail)
		em_storage_free_mail(&mail, 1, &err); 

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("err [%d]", err);		
	return ret;
}
/* description :  modify flag of msgno.
   return 1(success), 0(failure)
   */
EXPORT_API int em_core_mail_modify_flag(int mail_id, emf_mail_flag_t new_flag, int sticky_flag, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], err_code[%p]", mail_id, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int old_seen, new_seen;
	
	emf_mail_tbl_t *mail = NULL;
	emf_mail_flag_t old_flag;

	if (!em_storage_get_mail_field_by_id(mail_id, RETRIEVE_FLAG, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_field_by_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	if (!em_convert_mail_tbl_to_mail_flag(mail, &old_flag, &err))  {
		EM_DEBUG_EXCEPTION("em_convert_mail_tbl_to_mail_flag failed [%d]", err);
		goto  FINISH_OFF;
	}

	old_seen = mail->flags_seen_field;
	new_seen = new_flag.seen;

	if (!em_convert_mail_flag_to_mail_tbl(&new_flag, mail, &err))  {
		EM_DEBUG_EXCEPTION("em_convert_mail_flag_to_mail_tbl failed [%d]", err);
		goto FINISH_OFF;
	}

		mail->lock_status = sticky_flag;
		
	if (!em_storage_change_mail_field(mail_id, UPDATE_FLAG, mail, true, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_change_mail_field failed [%d]", err);
		
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
	
	if (old_seen != new_seen && new_seen == 0)
		em_core_delete_notification_for_read_mail(mail_id);

	em_core_check_unread_mail();

	ret = true;
	
FINISH_OFF: 
	if (mail)
		em_storage_free_mail(&mail, 1, NULL);
	
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);	
	return ret;
}

EXPORT_API int em_core_mail_set_flags_field(int account_id, int mail_ids[], int num, emf_flags_field_type field_type, int value, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mail_ids[%p], num [%d], field_type [%d], value[%d], err_code[%p]", account_id, mail_ids, num, field_type, value, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	char *field_type_name[EMF_FLAGS_FIELD_COUNT] = { "flags_seen_field"
		, "flags_deleted_field", "flags_flagged_field", "flags_answered_field"
		, "flags_recent_field", "flags_draft_field", "flags_forwarded_field" }; 
	
	if(field_type < 0 || field_type >= EMF_FLAGS_FIELD_COUNT || mail_ids == NULL || num <= 0 || account_id == 0) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
			goto  FINISH_OFF;
		}
	
	if (!em_storage_set_field_of_mails_with_integer_value(account_id, mail_ids, num, field_type_name[field_type], value, true, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_set_field_of_mails_with_integer_value failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		em_core_check_unread_mail();

	ret = true;
	
FINISH_OFF: 
	
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);	
	return ret;
}


/* description :  modify extra flag of msgno.
   return 0(success), -1(failure)
   */
EXPORT_API int em_core_mail_modify_extra_flag(int mail_id, emf_extra_flag_t new_flag, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], err_code[%p]", mail_id, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_mail_tbl_t mail;
	
	memset(&mail, 0x00, sizeof(emf_mail_tbl_t));

	mail.mail_id 	= mail_id;
	mail.save_status = new_flag.status;
	mail.lock_status = new_flag.report;
	mail.lock_status	= new_flag.lock;		
	mail.priority 	= new_flag.priority;
	mail.DRM_status  = new_flag.drm;

	if (!em_storage_change_mail_field(mail_id, UPDATE_EXTRA_FLAG, &mail, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_change_mail_field failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF: 
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);	
	return ret;
}


int em_core_mail_cmd_read_mail_pop3(void *stream, int msgno, int limited_size, int *downloded_size, int *result_total_body_size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p], msgno[%d], limited_size[%d], err_code[%p]", stream, msgno, limited_size, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int total_body_size = 0;
	char command[32];
	char *response = NULL;
	POP3LOCAL *pop3local;
	
	if (!stream || !result_total_body_size)  {
		EM_DEBUG_EXCEPTION("stream[%p], total_body_size[%p]", stream, msgno, result_total_body_size);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	pop3local = (POP3LOCAL *)(((MAILSTREAM *)stream)->local);

	if (!pop3local  || !pop3local->netstream)  {
		err = EMF_ERROR_INVALID_STREAM;
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
		
		err = EMF_ERROR_INVALID_RESPONSE;
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
				err = EMF_ERROR_INVALID_RESPONSE;
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
			err = EMF_ERROR_INVALID_RESPONSE;
			goto FINISH_OFF;
		}
		else  {
			err = EMF_ERROR_INVALID_RESPONSE;
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
	
	em_core_set_network_error(EMF_ERROR_NONE);		/*  set current network error as EMF_ERROR_NONE before network operation */
	/*  get mail from mail server */
	if (!pop3_send((MAILSTREAM *)stream, command, NULL))  {
		EM_DEBUG_EXCEPTION("pop3_send failed...");
		
		emf_session_t *session = NULL;
		
		if (!em_core_get_current_session(&session))  {
			EM_DEBUG_EXCEPTION("em_core_get_current_session failed...");
			err = EMF_ERROR_SESSION_NOT_FOUND;
			goto FINISH_OFF;
		}
		
		if (session->network == EMF_ERROR_NONE)
			err = EMF_ERROR_UNKNOWN;
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


EXPORT_API int em_core_mail_sync_flag_with_server(int mail_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%p], err_code[%p]", mail_id, err_code);
		
	if (mail_id < 1)  {
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int status = EMF_DOWNLOAD_FAIL;
	MAILSTREAM *stream = NULL;
	emf_mailbox_t mbox = {0};
	emf_mail_tbl_t *mail = NULL;
	emf_account_t *ref_account = NULL;
	int account_id = 0;
	int msgno = 0;
	emf_mail_flag_t new_flag = {0};
	char set_flags[100] = { 0, };
	char clear_flags[100] = { 0, };
	char tmp[100] = { 0, };
	
	if (!em_core_check_thread_status())  {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	if (!em_storage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	account_id = mail->account_id;
	em_convert_mail_tbl_to_mail_flag(mail, &new_flag, NULL);
	
	if (!(ref_account = em_core_get_account_reference(account_id)))   {
		EM_DEBUG_EXCEPTION("em_core_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	if (!em_core_check_thread_status())  {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	/*  open mail server. */
	if (!em_core_mailbox_open(account_id, mail->mailbox_name, (void **)&stream, &err) || !stream)  {
		EM_DEBUG_EXCEPTION("em_core_mailbox_open failed [%d]", err);
		status = EMF_LIST_CONNECTION_FAIL;
		goto FINISH_OFF;
	}
			
	
	if (!em_core_check_thread_status())  {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	mbox.name = mail->mailbox_name;	
	mbox.account_id = account_id;
	mbox.mail_stream = stream;

	/*  download all uids from server. */
	if (!em_core_mail_get_msgno_by_uid(ref_account, &mbox, mail->server_mail_id, &msgno, &err))  {
		EM_DEBUG_EXCEPTION("em_core_mail_get_msgno_by_uid failed message_no  :  %d ", err);
		goto FINISH_OFF;
	}
		
	sprintf (tmp, "%d", msgno);

	if (new_flag.seen)
		sprintf(set_flags, "\\Seen");
	else
		sprintf(clear_flags, "\\Seen");

	if (new_flag.answered)
		sprintf(set_flags, "%s \\Answered", set_flags);
	else
		sprintf(clear_flags, "%s \\Answered", clear_flags);
		
	if (new_flag.flagged)
		sprintf(set_flags, "%s \\Flagged", set_flags);
	else
		sprintf(clear_flags, "%s \\Flagged", clear_flags);

	if (new_flag.forwarded)
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

	EM_DEBUG_LOG(">>>> Returning from em_core_mail_sync_flag_with_server ");

	if (!em_core_check_thread_status())  {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	ret = true;

FINISH_OFF: 
	
	if (stream) em_core_mailbox_close(account_id, stream);
	if (mail) em_storage_free_mail(&mail, 1, NULL);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);	
	return ret;
}

EXPORT_API int em_core_mail_sync_seen_flag_with_server(int mail_ids[], int num, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], err_code[%p]", mail_ids[0], err_code);
		
	if (mail_ids[0] < 1)  {
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int status = EMF_DOWNLOAD_FAIL;
	MAILSTREAM *stream = NULL;
	emf_mailbox_t mbox;
	emf_mail_tbl_t *mail = NULL;
	emf_account_t *ref_account = NULL;
	int account_id = 0;
	int msgno = 0;
	emf_mail_flag_t new_flag;
	char set_flags[100];
	char clear_flags[100];
	char tmp[100];
	int mail_id = 0;
	int i = 0;
	
	memset(&mbox, 0x00, sizeof(emf_mailbox_t));

	mail_id = mail_ids[0];
	
	if (!em_storage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	account_id = mail->account_id;

	if (!(ref_account = em_core_get_account_reference(account_id)))   {
		EM_DEBUG_EXCEPTION("em_core_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	/*  open mail server. */
	if (!em_core_mailbox_open(account_id, mail->mailbox_name, (void **)&stream, &err) || !stream)  {
		EM_DEBUG_EXCEPTION("em_core_mailbox_open failed [%d]", err);
		status = EMF_LIST_CONNECTION_FAIL;
		goto FINISH_OFF;
	}

	mbox.name = mail->mailbox_name;	
	mbox.account_id = account_id;
	mbox.mail_stream = stream;
	
	for (i = 0; i < num; i++)  {
		mail_id = mail_ids[i];
	
		if (!em_core_check_thread_status())  {
			err = EMF_ERROR_CANCELLED;
			goto FINISH_OFF;
		}
		
		if (!em_storage_get_mail_by_id(mail_id, &mail, true, &err) || !mail)  {
			EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		
		/* account_id = mail->account_id; */
		em_convert_mail_tbl_to_mail_flag(mail, &new_flag, NULL);
		
		if (!em_core_check_thread_status())  {
			err = EMF_ERROR_CANCELLED;
			goto FINISH_OFF;
		}

		/*  download message number from server. */
		if (!em_core_mail_get_msgno_by_uid(ref_account, &mbox, mail->server_mail_id, &msgno, &err))  {
			EM_DEBUG_LOG("em_core_mail_get_msgno_by_uid failed message_no  :  %d ", err);
			goto FINISH_OFF;
		}
		
		memset(tmp, 0x00, 100);
		sprintf (tmp, "%d", msgno);

		memset(set_flags, 0x00, 100);
		memset(clear_flags, 0x00, 100);

		if (new_flag.seen)
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
			
		EM_DEBUG_LOG(">>>> Returning from em_core_mail_sync_flag_with_server ");

		if (!em_core_check_thread_status())  {
			err = EMF_ERROR_CANCELLED;
			goto FINISH_OFF;
		}
	}
	ret = true;

FINISH_OFF: 
	
	if (stream) em_core_mailbox_close(account_id, stream);
	if (mail) em_storage_free_mail(&mail, 1, NULL);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("err [%d]", err);	
	return ret;
}



EXPORT_API int em_core_mail_info_free(emf_mail_info_t **mail_info_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_info_list[%p], count[%d], err_code[%p]", mail_info_list, count, err_code);	
	
	if (count > 0)  {
		if (!mail_info_list || !*mail_info_list)  {
			EM_DEBUG_EXCEPTION("mail_info_list[%p]", mail_info_list);
			if (err_code != NULL)
				*err_code = EMF_ERROR_INVALID_PARAM;
			return false;
		}
		
		emf_mail_info_t *p = *mail_info_list;
		int i = 0;
		
		for (; i < count; i++) 
			EM_SAFE_FREE(p[i].sid);
		
		free(p); 
		*mail_info_list = NULL;
	}
	
	if (err_code != NULL)
		*err_code = EMF_ERROR_NONE;
	EM_DEBUG_FUNC_END();
	return true;
}

EXPORT_API int em_core_mail_head_free(emf_mail_head_t **mail_head_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_head_list[%p], count[%d], err_code[%p]", mail_head_list, count, err_code);
	EM_PROFILE_BEGIN(emCoreMailHeadeFree);
	
	if (count > 0)  {
		if (!mail_head_list || !*mail_head_list)  {
			EM_DEBUG_EXCEPTION("mail_head_list[%p], count[%d]", mail_head_list, count);
			
			if (err_code != NULL)
				*err_code = EMF_ERROR_INVALID_PARAM;
			return false;
		}
		
		int i;
		emf_mail_head_t *p = *mail_head_list;
		
		for (i = 0; i < count; i++)  {
			EM_SAFE_FREE(p[i].mid);
			EM_SAFE_FREE(p[i].subject);
			EM_SAFE_FREE(p[i].to);
			EM_SAFE_FREE(p[i].from);
			EM_SAFE_FREE(p[i].cc);
			EM_SAFE_FREE(p[i].bcc);
			EM_SAFE_FREE(p[i].reply_to);
			EM_SAFE_FREE(p[i].return_path);
			EM_SAFE_FREE(p[i].previewBodyText);
		}
		
		free(p); *mail_head_list = NULL;
	}
	
	if (err_code != NULL)
		*err_code = EMF_ERROR_NONE;
	
	EM_PROFILE_END(emCoreMailHeadeFree);
	EM_DEBUG_FUNC_END();
	return true;
}

EXPORT_API int em_core_mail_body_free(emf_mail_body_t **mail_body_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_body_list[%p], count[%d], err_code[%p]", mail_body_list, count, err_code);	
	
	if (count > 0)  {
		if (!mail_body_list || !*mail_body_list)  {
			EM_DEBUG_EXCEPTION("mail_body_list[%p]", mail_body_list);
			if (err_code != NULL)
				*err_code = EMF_ERROR_INVALID_PARAM;
			return false;
		}
		
		emf_mail_body_t *p = *mail_body_list;
		int i = 0;
		
		for (; i < count; i++)  {
			EM_SAFE_FREE(p[i].plain);
			EM_SAFE_FREE(p[i].plain_charset);
			EM_SAFE_FREE(p[i].html);
			if (p[i].attachment) em_core_mail_attachment_info_free(&p[i].attachment, NULL);
		}
		
		free(p); *mail_body_list = NULL;
	}
	
	if (err_code != NULL)
		*err_code = EMF_ERROR_NONE;
	EM_DEBUG_FUNC_END();
	return true;
}

EXPORT_API int em_core_mail_free(emf_mail_t **mail_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_list[%p], count[%d], err_code[%p]", mail_list, count, err_code);
	
	if (count > 0)  {
		if (!mail_list || !*mail_list)  {
			EM_DEBUG_EXCEPTION("mail_list[%p], count[%d]", mail_list, count);
			if (err_code != NULL)
				*err_code = EMF_ERROR_INVALID_PARAM;
			return false;
		}
		
		emf_mail_t *p = *mail_list;
		int i = 0;
		
		for (; i < count; i++)  {
			if (p[i].info) 
				em_core_mail_info_free(&p[i].info, 1, NULL);
			
			if (p[i].head)
				em_core_mail_head_free(&p[i].head, 1, NULL);
			
			if (p[i].body)
				em_core_mail_body_free(&p[i].body, 1, NULL);
		}
		
		free(p); *mail_list = NULL;
	}

	if (err_code != NULL)
		*err_code = EMF_ERROR_NONE;
	EM_DEBUG_FUNC_END();
	return true;
}

EXPORT_API int em_core_free_mail_data(emf_mail_data_t **mail_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_list[%p], count[%d]", mail_list, count);
	
	if (count <= 0 || !mail_list || !*mail_list)  {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");			
		if(err_code)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
		
	emf_mail_data_t* p = *mail_list;
	int i;
	
	for (i = 0; i < count; i++) {
		EM_SAFE_FREE(p[i].mailbox_name);
		EM_SAFE_FREE(p[i].subject);
		EM_SAFE_FREE(p[i].datetime);
		EM_SAFE_FREE(p[i].server_mailbox_name);
		EM_SAFE_FREE(p[i].server_mail_id);
		EM_SAFE_FREE(p[i].message_id);
		EM_SAFE_FREE(p[i].full_address_from);
		EM_SAFE_FREE(p[i].full_address_reply);
		EM_SAFE_FREE(p[i].full_address_to);
		EM_SAFE_FREE(p[i].full_address_cc);
		EM_SAFE_FREE(p[i].full_address_bcc);
		EM_SAFE_FREE(p[i].full_address_return);
		EM_SAFE_FREE(p[i].email_address_sender);
		EM_SAFE_FREE(p[i].email_address_recipient);
		EM_SAFE_FREE(p[i].alias_sender);
		EM_SAFE_FREE(p[i].alias_recipient);
		EM_SAFE_FREE(p[i].file_path_plain);
		EM_SAFE_FREE(p[i].file_path_html);
		EM_SAFE_FREE(p[i].preview_text);
	}
	
	EM_SAFE_FREE(p); *mail_list = NULL;

	if(err_code)
		*err_code = EMF_ERROR_NONE;
	
	EM_DEBUG_FUNC_END();	
	return true;
}


EXPORT_API int em_core_mail_attachment_info_free(emf_attachment_info_t **atch_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("atch_info[%p], err_code[%p]", atch_info, err_code);	
	
	if (!atch_info || !*atch_info)  {
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	emf_attachment_info_t *p = *atch_info;
	emf_attachment_info_t *t;
	
	while (p)  {
		EM_SAFE_FREE(p->name);
		EM_SAFE_FREE(p->savename);
		t = p->next;
		free(p); p = NULL;
		p = t;
	}
	
	if (err_code != NULL)
		*err_code = EMF_ERROR_NONE;
	EM_DEBUG_FUNC_END();
	return true;
}


EXPORT_API int em_core_free_attachment_data(emf_attachment_data_t **attachment_data_list, int attachment_data_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_data_list[%p], attachment_data_count [%d], err_code[%p]", attachment_data_list, attachment_data_count, err_code);	
	
	if (!attachment_data_list || !*attachment_data_list)  {
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	emf_attachment_data_t* p = *attachment_data_list;
	int i = 0;
	
	for (i = 0; i < attachment_data_count; i++) {
		EM_SAFE_FREE(p[i].mailbox_name);
		EM_SAFE_FREE(p[i].attachment_name);
		EM_SAFE_FREE(p[i].attachment_path);
	}

	EM_SAFE_FREE(p); *attachment_data_list = NULL;

	if(err_code)
		*err_code = EMF_ERROR_NONE;
	
	EM_DEBUG_FUNC_END();	
	return true;
}



#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

EXPORT_API int em_core_delete_pbd_activity(int account_id, int mail_id, int activity_id, int *err_code) 
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], err_code[%p]", account_id, mail_id, err_code);

	if (account_id < FIRST_ACCOUNT_ID || mail_id < 0) {
		EM_DEBUG_EXCEPTION("account_id[%d], mail_id[%d], err_code[%p]", account_id, mail_id, err_code);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int err = EMF_ERROR_NONE;
	
	em_storage_begin_transaction(NULL, NULL, NULL);
	
	if (!em_storage_delete_pbd_activity(account_id, mail_id, activity_id, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_delete_pbd_activity failed [%d]", err);
		
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF: 
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (em_storage_commit_transaction(NULL, NULL, NULL) == false) {
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
			ret = false;
		}
	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (em_storage_rollback_transaction(NULL, NULL, NULL) == false)
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
	}	
	
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int em_core_insert_pbd_activity(emf_event_partial_body_thd *local_activity, int *activity_id, int *err_code) 
{
	EM_DEBUG_FUNC_BEGIN("local_activity[%p], activity_id[%p], err_code[%p]", local_activity, activity_id, err_code);

	if (!local_activity || !activity_id) {
		EM_DEBUG_EXCEPTION("local_activity[%p], activity_id[%p] err_code[%p]", local_activity, activity_id, err_code);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	em_storage_begin_transaction(NULL, NULL, NULL);
	
	if (!em_storage_add_pbd_activity(local_activity, activity_id, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_add_pbd_activity failed [%d]", err);
		
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF: 
	if (ret == true) {	/*  COMMIT TRANSACTION; */
		if (em_storage_commit_transaction(NULL, NULL, NULL) == false) {
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
			ret = false;
		}
	}
	else {	/*  ROLLBACK TRANSACTION; */
		if (em_storage_rollback_transaction(NULL, NULL, NULL) == false)
			err = em_storage_get_emf_error_from_em_storage_error(EM_STORAGE_ERROR_DB_FAILURE);
	}	
	
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

#endif

#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__

/* API to set or unset a field of flags on server in single IMAP request to server */

EXPORT_API int em_core_mail_sync_flags_field_with_server(int mail_ids[], int num, emf_flags_field_type field_type, int value, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids [%p], num [%d], field_type [%d], value [%d], err_code [%p]", mail_ids, num, field_type, value, err_code);
		
	if (NULL == mail_ids || num <= 0 || field_type < 0 || field_type >= EMF_FLAGS_FIELD_COUNT)  {
			EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		if (err_code != NULL) {
			*err_code = EMF_ERROR_INVALID_PARAM;
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
	int err = EMF_ERROR_NONE;
	int command_success = false;
	int account_id = 0;
	int mail_id = 0;
	int i = 0;
	int id_set_count = 0;
	int len_of_each_range = 0;
	int string_count = 0;
	emf_account_t *temp_account;
	emf_id_set_t *id_set = NULL;
	emf_mail_tbl_t *mail = NULL;
	emf_uid_range_set *uid_range_set = NULL;
	emf_uid_range_set *uid_range_node = NULL;
	char *field_type_name[EMF_FLAGS_FIELD_COUNT] = { "\\Seen"
		, "\\Deleted", "\\Flagged", "\\Answered"
		, "\\Recent", "\\Draft", "$Forwarded" }; 
	
	mail_id = mail_ids[0];
	
	if (!em_storage_get_mail_field_by_id(mail_id, RETRIEVE_FLAG, &mail, true, &err) || !mail) 		/*To DO :  This is a existing bug. on mail deletion before this call it will fail always */ {
		EM_DEBUG_LOG("em_storage_get_mail_by_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	account_id = mail[0].account_id;

	temp_account = em_core_get_account_reference(account_id);

	if (!temp_account)   {
		EM_DEBUG_EXCEPTION("em_core_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	if (temp_account->receiving_server_type != EMF_SERVER_TYPE_IMAP4) {
		EM_DEBUG_EXCEPTION("Syncing seen flag is available only for IMAP4 server. The server type [%d]", temp_account->receiving_server_type);
		err = EMF_ERROR_NOT_SUPPORTED;
		goto FINISH_OFF;
	}
	
	if (!em_core_mailbox_open(account_id, mail->mailbox_name, (void **)&stream, &err) || !stream)  {
		EM_DEBUG_EXCEPTION("em_core_mailbox_open failed [%d]", err);
		goto FINISH_OFF;
	}

	if (false == em_storage_free_mail(&mail, 1, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_free_mail failed - %d ", err);
		goto FINISH_OFF;
	}

	/* [h.gahlaut] Break the set of mail_ids into comma separated strings of given length */
	/* Length is decided on the basis of remaining keywords in the Query to be formed later on in em_storage_get_id_set_from_mail_ids */
	/* Here about 90 bytes are required for fixed keywords in the query-> SELECT local_uid, s_uid from mail_read_mail_uid_tbl where local_uid in (....) ORDER by s_uid */
	/* So length of comma separated strings which will be filled in (.....) in above query can be maximum QUERY_SIZE - 90 */

	if (false == em_core_form_comma_separated_strings(mail_ids, num, QUERY_SIZE - 90, &string_list, &string_count, &err))   {
		EM_DEBUG_EXCEPTION("em_core_form_comma_separated_strings failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Now execute one by one each comma separated string of mail_ids */

	for (i = 0; i < string_count; ++i) {
		/* Get the set of mail_ds and corresponding server_mail_ids sorted by server mail ids in ascending order */

		if (false == em_storage_get_id_set_from_mail_ids(string_list[i], &id_set, &id_set_count, &err)) {
			EM_DEBUG_EXCEPTION("em_storage_get_id_set_from_mail_ids failed [%d]", err);
			goto FINISH_OFF;
		}
		
		/* Convert the sorted sequence of server mail ids to range sequences of given length. A range sequence will be like A : B, C, D: E, H */
		
		len_of_each_range = MAX_IMAP_COMMAND_LENGTH - 40;		/*  1000 is the maximum length allowed RFC 2683. 40 is left for keywords and tag. */
		
		if (false == em_core_convert_to_uid_range_set(id_set, id_set_count, &uid_range_set, len_of_each_range, &err)) {
			EM_DEBUG_EXCEPTION("em_core_convert_to_uid_range_set failed [%d]", err);
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
				
				err = EMF_ERROR_UNKNOWN;		
				goto FINISH_OFF;
			}
		
			if (!net_sout(imaplocal->netstream, cmd, (int)strlen(cmd))) {
				EM_DEBUG_EXCEPTION("net_sout failed...");
				err = EMF_ERROR_CONNECTION_BROKEN;		
				goto FINISH_OFF;
			}

			/* Receive Response */

			command_success = false;
			
			while (imaplocal->netstream) {	
				if (!(p = net_getline(imaplocal->netstream)))  {
					EM_DEBUG_EXCEPTION("net_getline failed...");
				
					err = EMF_ERROR_INVALID_RESPONSE;	
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
						err = EMF_ERROR_IMAP4_STORE_FAILURE;		
						EM_SAFE_FREE(p);
						goto FINISH_OFF;
					}		
				}
				
				EM_SAFE_FREE(p);		
			}

			uid_range_node = uid_range_node->next;
		}	

		em_core_free_uid_range_set(&uid_range_set);

		EM_SAFE_FREE(id_set);
		
		id_set_count = 0;
	}

	ret = true;

FINISH_OFF: 

#ifdef __LOCAL_ACTIVITY__
	if (ret) {
		emf_activity_tbl_t new_activity;
		for (i = 0; i<num ; i++) {		
			memset(&new_activity, 0x00, sizeof(emf_activity_tbl_t));
			new_activity.activity_type = ACTIVITY_MODIFYSEENFLAG;
			new_activity.account_id    = account_id;
			new_activity.mail_id	   = mail_ids[i];
		
			if (!em_core_activity_delete(&new_activity, &err))
				EM_DEBUG_EXCEPTION("Local Activity ACTIVITY_MOVEMAIL [%d] ", err);
		}

	}
#endif

	if (NULL != mail) {
		if (false == em_storage_free_mail(&mail, 1, &err))			
			EM_DEBUG_EXCEPTION("em_storage_free_mail failed - %d ", err);
	}
	
	em_core_free_comma_separated_strings(&string_list, &string_count);

	if (false == ret)
		em_core_free_uid_range_set(&uid_range_set);

	em_core_mailbox_close(0, stream);
	stream = NULL;

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;

}
#endif

EXPORT_API int em_core_mail_filter_by_rule(emf_rule_t *filter_info, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_info[%p], err_code[%p]", filter_info, err_code);

	int ret = false, err = EMF_ERROR_NONE;
	int account_index, account_count, mail_id_index = 0;
	emf_account_t *account_ref = NULL, *account_list_ref = NULL;
	int filtered_mail_id_count = 0, *filtered_mail_id_list = NULL, parameter_string_length = 0;
	char *parameter_string = NULL, mail_id_string[10] = { 0x00, };
	emf_mailbox_tbl_t *spam_mailbox = NULL;

	if (!filter_info)  {
		EM_DEBUG_EXCEPTION("filter_info[%p]", filter_info);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!em_core_account_get_list_refer(&account_list_ref, &account_count, &err)) {
		EM_DEBUG_EXCEPTION("em_core_account_get_list_refer failed [%d]", err);
		goto FINISH_OFF;
	}

	for (account_index = 0; account_index < account_count; account_index++) {
		account_ref = account_list_ref + account_index;

		if (!em_storage_get_mailbox_by_mailbox_type(account_ref->account_id, EMF_MAILBOX_TYPE_SPAMBOX, &spam_mailbox, false, &err))
			EM_DEBUG_EXCEPTION("em_storage_get_mailbox_by_mailbox_type for account_id[%d] failed [%d]", account_ref->account_id, err);
		else if (spam_mailbox && spam_mailbox->mailbox_name) {
			if (!em_storage_filter_mails_by_rule(account_ref->account_id, spam_mailbox->mailbox_name, (emf_mail_rule_tbl_t *)filter_info, &filtered_mail_id_list, &filtered_mail_id_count, &err))
				EM_DEBUG_EXCEPTION("em_storage_filter_mails_by_rule failed [%d]", err);
			else {
				if (filtered_mail_id_count) {
					parameter_string_length = strlen(spam_mailbox->mailbox_name) + 7 + (10 * filtered_mail_id_count);
					parameter_string = em_core_malloc(sizeof(char) * parameter_string_length);

					if (parameter_string == NULL) {
						err = EMF_ERROR_OUT_OF_MEMORY;
						EM_DEBUG_EXCEPTION("em_core_malloc failed for parameter_string");
						goto FINISH_OFF;
					}

					SNPRINTF(parameter_string, parameter_string_length, "[NA]%c%s%c", 0x01, spam_mailbox->mailbox_name, 0x01);
					
					for (mail_id_index = 0; mail_id_index < filtered_mail_id_count; mail_id_index++) {
						memset(mail_id_string, 0, 10);
						SNPRINTF(mail_id_string, 10, "%d", filtered_mail_id_list[mail_id_index]);
						strcat(parameter_string, mail_id_string);
						strcat(parameter_string, ",");
					}

					EM_DEBUG_LOG("filtered_mail_id_count [%d]", filtered_mail_id_count);
					EM_DEBUG_LOG("param string [%s]", parameter_string);

					if (!em_storage_notify_storage_event(NOTI_MAIL_MOVE, account_ref->account_id, 0, parameter_string, 0)) 
						EM_DEBUG_EXCEPTION("em_storage_notify_storage_event failed [ NOTI_MAIL_MOVE ] >>>> ");

					EM_SAFE_FREE(filtered_mail_id_list);
					EM_SAFE_FREE(parameter_string);
				}
			}
			em_storage_free_mailbox(&spam_mailbox, 1, &err);
			spam_mailbox = NULL;
		}
	}

	em_core_check_unread_mail();

	ret = true;

FINISH_OFF: 
	EM_SAFE_FREE(account_list_ref);
	EM_SAFE_FREE(filtered_mail_id_list);
	EM_SAFE_FREE(parameter_string);


	if (spam_mailbox)
		em_storage_free_mailbox(&spam_mailbox, 1, &err);

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}
