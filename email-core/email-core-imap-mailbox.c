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
 * File :  email-core-imap_folder.c
 * Desc :  Mail IMAP mailbox
 *
 * Auth : 
 *
 * History : 
 *    2006.08.01  :  created
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vconf.h>
#include "email-core-global.h"
#include "email-core-utils.h"
#include "c-client.h"
#include "email-storage.h"
#include "email-utilities.h"
#include "email-core-event.h"
#include "email-core-mailbox.h"
#include "email-core-imap-mailbox.h"
#include "email-core-mailbox-sync.h"
#include "email-core-account.h" 
#include "lnx_inc.h"

#include "email-debug-log.h"

INTERNAL_FUNC int emcore_get_default_mail_slot_count(int *output_count, int *err_code)
{	
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_LOG("output_count[%p], err_code[%p]", output_count, err_code);

	int err = EMF_ERROR_NONE;
	int mail_slot_count;
	int ret = false, ret2;

	if (output_count == NULL) {
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ret2 = vconf_get_int(VCONF_KEY_DEFAULT_SLOT_SIZE, &mail_slot_count);

	if (ret2 < 0) {
		EM_DEBUG_EXCEPTION("vconf_get_int() Failed(%d)", ret2);
      	mail_slot_count = 100;
	}

	ret = true;

FINISH_OFF: 
	
	if (output_count)
		*output_count = mail_slot_count;

	if (err_code)
		*err_code = err;

	return ret;
	
}


INTERNAL_FUNC int emcore_remove_overflowed_mails(emstorage_mailbox_tbl_t *intput_mailbox_tbl, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("intput_mailbox_tbl[%p], err_code[%p]", intput_mailbox_tbl, err_code);

	int ret = false; 
	int *mail_id_list = NULL, mail_id_list_count = 0;
	int err = EMF_ERROR_NONE;
	emf_account_t *account_ref = NULL;
	
	if (!intput_mailbox_tbl || intput_mailbox_tbl->account_id < 1) {
		if (intput_mailbox_tbl)
		EM_DEBUG_EXCEPTION("Invalid Parameter. intput_mailbox_tbl->account_id [%d]", intput_mailbox_tbl->account_id);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	account_ref = emcore_get_account_reference(intput_mailbox_tbl->account_id);
	if (account_ref) {
		if (account_ref->receiving_server_type == EMF_SERVER_TYPE_ACTIVE_SYNC) {
			EM_DEBUG_LOG("ActiveSync Account didn't support mail slot");
			err = EMF_ERROR_NOT_SUPPORTED;
			goto FINISH_OFF;
		}
	}
	
	if (!emstorage_get_overflowed_mail_id_list(intput_mailbox_tbl->account_id, intput_mailbox_tbl->mailbox_name, intput_mailbox_tbl->mail_slot_size, &mail_id_list, &mail_id_list_count, true, &err)) {
		if (err == EMF_ERROR_MAIL_NOT_FOUND) {
			EM_DEBUG_LOG("There are enough slot in intput_mailbox_tbl [%s]", intput_mailbox_tbl->mailbox_name);
			err = EMF_ERROR_NONE;
			ret = true;
		}
		else
			EM_DEBUG_EXCEPTION("emstorage_get_overflowed_mail_id_list failed [%d]", err);
		goto FINISH_OFF;
	}

	if (mail_id_list) {
		if (!emcore_delete_mail(intput_mailbox_tbl->account_id, mail_id_list, mail_id_list_count, false, EMF_DELETED_BY_OVERFLOW, false, &err)) {
			EM_DEBUG_EXCEPTION("emcore_delete_mail failed [%d]", err);
			goto FINISH_OFF;
		}
	}
	
	ret = true;
FINISH_OFF: 
	EM_SAFE_FREE(mail_id_list);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emcore_set_mail_slot_size(int account_id, char *mailbox_name, int new_slot_size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_LOG("account_id [%d], mailbox_name[%p], err_code[%p]", account_id, mailbox_name, err_code);

	int ret = false, err = EMF_ERROR_NONE; 
	int i, mailbox_count = 0;
	emf_account_t *account_ref = NULL;
	emstorage_mailbox_tbl_t *mailbox_tbl_list = NULL;

	if (account_id > ALL_ACCOUNT) {
		account_ref = emcore_get_account_reference(account_id);
		if (account_ref && account_ref->receiving_server_type == EMF_SERVER_TYPE_ACTIVE_SYNC) {
			EM_DEBUG_LOG("ActiveSync account didn't support mail slot");
			ret = true;
			goto FINISH_OFF;
		}
		else if (!account_ref) {
			EM_DEBUG_EXCEPTION("emcore_get_account_reference failed");
			goto FINISH_OFF;
		}
	}

	if (!emstorage_set_mail_slot_size(account_id, mailbox_name, new_slot_size, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_set_mail_slot_size failed [%d]", err);
		goto FINISH_OFF;
	}

	if (mailbox_name) {
		mailbox_count = 1;
		if (new_slot_size > 0) {
			mailbox_tbl_list = em_malloc(sizeof(emstorage_mailbox_tbl_t) * mailbox_count);
			if(!mailbox_tbl_list) {
				EM_DEBUG_EXCEPTION("em_malloc failed");
				goto FINISH_OFF;
			}
			mailbox_tbl_list->account_id = account_id;
			mailbox_tbl_list->mailbox_name = EM_SAFE_STRDUP(mailbox_name);
			mailbox_tbl_list->mail_slot_size = new_slot_size;     
		}
		else   {	/*  read information from DB */
			if (!emstorage_get_mailbox_by_name(account_id, EMF_MAILBOX_ALL, mailbox_name, &mailbox_tbl_list, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_get_mailbox failed [%d]", err);
				goto FINISH_OFF;
			}
			
		}
	}
	else {
		if (!emstorage_get_mailbox(account_id, EMF_MAILBOX_ALL, EMAIL_MAILBOX_SORT_BY_NAME_ASC, &mailbox_count, &mailbox_tbl_list, true, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox failed [%d]", err);
			goto FINISH_OFF;
		}
	}

	for (i = 0; i < mailbox_count; i++) {
		if (!emcore_remove_overflowed_mails(mailbox_tbl_list + i, &err)) {
			if (err == EMF_ERROR_MAIL_NOT_FOUND || err == EMF_ERROR_NOT_SUPPORTED)
				err = EMF_ERROR_NONE;
			else
				EM_DEBUG_EXCEPTION("emcore_remove_overflowed_mails failed [%d]", err);
		}
	}

	

	ret = true;
FINISH_OFF: 

	if (mailbox_tbl_list)
		emstorage_free_mailbox(&mailbox_tbl_list, mailbox_count, NULL);


	if (err_code)
		*err_code = err;
	return ret;
}


static int emcore_get_mailbox_connection_path(int account_id, char *mailbox_name, char **path, int *err_code)
{
    emf_account_t *ref_account = NULL;
	size_t path_len = 0;


	ref_account = emcore_get_account_reference(account_id);
	if (!ref_account)	 {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed");
		return 0;
	}

	path_len = strlen(ref_account->receiving_server_addr) +
                          (mailbox_name ? strlen(mailbox_name) : 0) + 50;

    *path = em_malloc(path_len);/* strlen(ref_account->receiving_server_addr) + */
                          /* (mailbox_name ? strlen(mailbox_name) : 0) + 20); */
    if (!*path)
    	return 0;
	memset(*path, 0x00, path_len);
    /* 1. server address / server type */

    if (ref_account->receiving_server_type == EMF_SERVER_TYPE_POP3) {
        SNPRINTF(*path + 1, path_len-1, "%s:%d/pop", ref_account->receiving_server_addr, ref_account->port_num);
    }
    else {
        SNPRINTF(*path + 1, path_len-1, "%s:%d/imap", ref_account->receiving_server_addr, ref_account->port_num);
    }

    /* 2. set tls option if security connection */
/*     if (ref_account->use_security) strncat(*path + 1, "/tls", path_len-(strlen(*path)-1)); */
	if (ref_account->use_security & 0x01) {
		strncat(*path + 1, "/ssl", path_len-(strlen(*path)-1));
	}
	if (ref_account->use_security & 0x02)
		strncat(*path + 1, "/tls", path_len-(strlen(*path)-1));
	else
		strncat(*path + 1, "/notls", path_len-(strlen(*path)-1));

    /*  3. re-format mailbox name (ex:"{mai.test.com:143/imap} or {mai.test.com:143/imap/tls}"} */
    strncat(*path + 1, "}", path_len-strlen(*path)-1);
    **path = '{';

    if (mailbox_name) strncat(*path, mailbox_name, path_len-strlen(*path)-1);

    return 1;
}

INTERNAL_FUNC int emcore_sync_mailbox_list(int account_id, char *mailbox_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%p], err_code[%p]", account_id, mailbox_name, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int status = EMF_DOWNLOAD_FAIL;
	
	MAILSTREAM *stream = NULL;
	emf_mailbox_t *mailbox_list = NULL;
	emf_account_t *ref_account = NULL;
	void *tmp_stream = NULL;
	char *mbox_path = NULL;
	int i = 0, count = 0, counter = 0, mailbox_type_list[EMF_MAILBOX_TYPE_ALL_EMAILS + 1] = {-1, -1, -1, -1, -1, -1, -1, -1};
	char *mailbox_name_for_mailbox_type = NULL;

	
	if (err_code) 
		*err_code = EMF_ERROR_NONE;
	
	if (!emcore_check_thread_status())  {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	ref_account = emcore_get_account_reference(account_id);
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed - %d", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	/* if not imap4 mail, return */
	if (ref_account->account_bind_type != EMF_BIND_TYPE_EM_CORE || ref_account->receiving_server_type != EMF_SERVER_TYPE_IMAP4)  {
		EM_DEBUG_EXCEPTION("unsupported account...");
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	/*  get mail server path */
	/*  mbox_path is not used. the below func might be unnecessary */
	if (!emcore_get_mailbox_connection_path(account_id, NULL, &mbox_path, &err) || !mbox_path)  {
		EM_DEBUG_EXCEPTION("emcore_get_mailbox_connection_path - %d", err);
		goto FINISH_OFF;
	}
	
	
	if (!emcore_check_thread_status())  {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}

	stream = NULL;
	if (!emcore_connect_to_remote_mailbox(account_id, NULL, (void **)&tmp_stream, &err) || !tmp_stream)  {
		EM_DEBUG_EXCEPTION("emcore_connect_to_remote_mailbox failed - %d", err);
		
		if (err == EMF_ERROR_CONNECTION_BROKEN)
			err = EMF_ERROR_CANCELLED;
		else
			err = EMF_ERROR_CONNECTION_FAILURE;
		
		status = EMF_DOWNLOAD_CONNECTION_FAIL;
		goto FINISH_OFF;
	}
	
	EM_SAFE_FREE(mbox_path);
	
	stream = (MAILSTREAM *)tmp_stream;
	
	if (!emcore_check_thread_status())  {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	/*  download mailbox list */
	if (!emcore_download_mailbox_list(stream, mailbox_name, &mailbox_list, &count, &err))  {
		EM_DEBUG_EXCEPTION("emcore_download_mailbox_list failed - %d", err);
		goto FINISH_OFF;
	}

	if (!emcore_check_thread_status())  {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	for (i = 0; i < count; i++) {
		if (!emcore_check_thread_status())  {
			EM_DEBUG_LOG("emcore_check_thread_status - cancelled");
			err = EMF_ERROR_CANCELLED;
			goto FINISH_OFF;
		}
		if (mailbox_list[i].name) {
			EM_DEBUG_LOG("mailbox name - %s", mailbox_list[i].name);
			emcore_get_default_mail_slot_count(&(mailbox_list[i].mail_slot_size), NULL);
			emcore_bind_mailbox_type(mailbox_list + i);

			if (mailbox_list[i].mailbox_type <= EMF_MAILBOX_TYPE_ALL_EMAILS) {	/*  if result mailbox type is duplicated,  */
				if (mailbox_type_list[mailbox_list[i].mailbox_type] != -1) {
					EM_DEBUG_LOG("Mailbox type [%d] of [%s] is duplicated", mailbox_list[i].mailbox_type, mailbox_list[i].name);
					mailbox_list[i].mailbox_type = EMF_MAILBOX_TYPE_USER_DEFINED; /*  ignore latest one  */
				}
				else
					mailbox_type_list[mailbox_list[i].mailbox_type] = i;
			}

			EM_DEBUG_LOG("mailbox type [%d]", mailbox_list[i].mailbox_type);
			if(!emcore_set_sync_imap_mailbox(mailbox_list + i, 1, &err)) {
				EM_DEBUG_EXCEPTION("emcore_set_sync_imap_mailbox failed [%d]", err);
				goto FINISH_OFF;
			}

		}
	}


	for (counter = EMF_MAILBOX_TYPE_INBOX; counter <= EMF_MAILBOX_TYPE_OUTBOX; counter++) {
		/* if (!emstorage_get_mailboxname_by_mailbox_type(account_id, counter, &mailbox_name_for_mailbox_type, false, &err))  */
		if (mailbox_type_list[counter] == -1) {
			/* EM_DEBUG_EXCEPTION("emstorage_get_mailboxname_by_mailbox_type failed - %d", err); */
			/* if (EMF_ERROR_MAILBOX_NOT_FOUND == err)	 */
			/* { */
				emstorage_mailbox_tbl_t mailbox_tbl;
				
				memset(&mailbox_tbl, 0x00, sizeof(mailbox_tbl));
				
				mailbox_tbl.account_id = account_id;
				mailbox_tbl.mailbox_id = 0;
				mailbox_tbl.local_yn = 1; 
				mailbox_tbl.mailbox_type = counter;
				mailbox_tbl.sync_with_server_yn =  1;
				mailbox_tbl.modifiable_yn = 1; 
				mailbox_tbl.total_mail_count_on_server = 0;
				emcore_get_default_mail_slot_count(&mailbox_tbl.mail_slot_size, NULL);
				
				switch (counter) {
					case EMF_MAILBOX_TYPE_SENTBOX:
						mailbox_tbl.mailbox_name = EMF_SENTBOX_NAME;
						mailbox_tbl.alias = EMF_SENTBOX_DISPLAY_NAME;
						break;
						
					case EMF_MAILBOX_TYPE_TRASH:
						mailbox_tbl.mailbox_name = EMF_TRASH_NAME;
						mailbox_tbl.alias = EMF_TRASH_DISPLAY_NAME;
						break;

				    case EMF_MAILBOX_TYPE_DRAFT:
						mailbox_tbl.mailbox_name = EMF_DRAFTBOX_NAME;
						mailbox_tbl.alias = EMF_DRAFTBOX_DISPLAY_NAME;
						break;
						
					case EMF_MAILBOX_TYPE_SPAMBOX:
						mailbox_tbl.mailbox_name = EMF_SPAMBOX_NAME;
						mailbox_tbl.alias = EMF_SPAMBOX_DISPLAY_NAME;
						break;
						
					case EMF_MAILBOX_TYPE_OUTBOX:
						mailbox_tbl.mailbox_name = EMF_OUTBOX_NAME;
						mailbox_tbl.alias = EMF_OUTBOX_DISPLAY_NAME;
						break;

					default: 
						mailbox_tbl.mailbox_name = EMF_INBOX_NAME;
						mailbox_tbl.alias = EMF_INBOX_DISPLAY_NAME;
						break;
				}

				if (!emstorage_add_mailbox(&mailbox_tbl, true, &err)) {
					EM_DEBUG_EXCEPTION("emstorage_add_mailbox failed - %d", err);
					goto FINISH_OFF;
				}
				
			/* }	 */
			/* else */
			/* { */
			/*  */
			/* 	goto FINISH_OFF; */
			/* } */
			
		}
		EM_SAFE_FREE(mailbox_name_for_mailbox_type);
	}

	emstorage_mailbox_tbl_t *local_mailbox_list = NULL;
	int select_num = 0;
	i = 0;
	emf_mailbox_t mailbox;

	if (emstorage_get_mailbox_by_modifiable_yn(account_id, 0 /* modifiable_yn */, &select_num, &local_mailbox_list, true, &err)) {
		if (select_num > 0) {
			for (i = 0; i < select_num; i++) {
				EM_DEBUG_LOG(">>> MailBox needs to be Deleted[ %s ] ", local_mailbox_list[i].mailbox_name);
				mailbox.account_id = local_mailbox_list[i].account_id;
				mailbox.name	   = local_mailbox_list[i].mailbox_name;
				if (!emcore_delete_mailbox_all(&mailbox, &err)) {
					EM_DEBUG_EXCEPTION(" emcore_delete_all of Mailbox [%s] Failed ", mailbox.name);
					emstorage_free_mailbox(&local_mailbox_list, select_num, NULL); 
					local_mailbox_list = NULL;
					goto FINISH_OFF;
				}
			}
			emstorage_free_mailbox(&local_mailbox_list, select_num, NULL); 
			local_mailbox_list = NULL;
		}
	}

	if (!emstorage_set_all_mailbox_modifiable_yn(account_id, 0, true, &err)) {
			EM_DEBUG_EXCEPTION(" >>>> emstorage_set_all_mailbox_modifiable_yn Failed [ %d ]", err);
			goto FINISH_OFF;
	}
	
	if (!emcore_check_thread_status())  {
		err = EMF_ERROR_CANCELLED;
		goto FINISH_OFF;
	}
	
	for (i = 0; i < count; i++)
		mailbox_list[i].account_id = account_id;
	
	
	ret = true;
	
FINISH_OFF: 
	EM_SAFE_FREE(mailbox_name_for_mailbox_type);
	EM_SAFE_FREE(mbox_path);

	if (stream) 
		emcore_close_mailbox(account_id, stream);
	
	if (mailbox_list) 
		emcore_free_mailbox(&mailbox_list, count, NULL);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

int emcore_download_mailbox_list(void *mail_stream, 
										char *mailbox, 
										emf_mailbox_t **mailbox_list, 
										int *count, 
										int *err_code)
{
    MAILSTREAM *stream = mail_stream;
    emf_callback_holder_t holder;
    char *pattern = NULL;
    char *reference = NULL;
    int		err = EMF_ERROR_NONE;
    int		ret = false;

    EM_DEBUG_FUNC_BEGIN();

	if (err_code) {
		*err_code = EMF_ERROR_NONE;
	}

	if (!stream || !mailbox_list || !count) {
        err = EMF_ERROR_INVALID_PARAM;
        goto FINISH_OFF;
	}
	
	memset(&holder, 0x00, sizeof(holder));

    /*  reference (ex : "{mail.test.com}", "{mail.test.com}inbox") */
	if (mailbox)  {
		char *s = NULL;
		reference = em_malloc(strlen(stream->original_mailbox) + strlen(mailbox) + 1);
		if (reference) {
			strncpy(reference, stream->original_mailbox, (size_t)strlen(stream->original_mailbox));
			if ((s = strchr(reference, '}')))
				*(++s) = '\0';
			strcat(reference, mailbox);
		}
	}
	else
		reference = EM_SAFE_STRDUP(stream->original_mailbox);

	pattern = "*";
    stream->sparep = &holder;

	/*  imap command  :  tag LIST reference * */
	mail_list(stream, reference, pattern);

    stream->sparep = NULL;

   	EM_SAFE_FREE(reference);

    *count = holder.num;
    *mailbox_list = (emf_mailbox_t *)holder.data;

	ret = true;

FINISH_OFF: 
	if (err_code) *err_code = err;

	return ret;
}

/* description
 *    check whether this imap mailbox is synchronous mailbox
 * arguments
 *    mailbox  :  imap mailbox to be checked
 *    synchronous  :   boolean variable to be synchronous (1 : sync 0 : non-sync)
 * return
 *    succeed  :  1
 *    fail  :  0
 */
int emcore_check_sync_imap_mailbox(emf_mailbox_t *mailbox, int *synchronous, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	EM_DEBUG_LOG("\t mailbox[%p], synchronous[%p], err_code[%p]", mailbox, synchronous, err_code);
	
	if (err_code) {
		*err_code = EMF_ERROR_NONE;
	}

	if (!mailbox || !synchronous) {
		EM_DEBUG_EXCEPTION("\t mailbox[%p], synchronous[%p]", mailbox, synchronous);
		
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	emstorage_mailbox_tbl_t *imap_mailbox_tbl = NULL;

	if (!emstorage_get_mailbox_by_name(mailbox->account_id, 0, mailbox->name, &imap_mailbox_tbl, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_name failed - %d", err);
		goto FINISH_OFF;
	}
	
	*synchronous = imap_mailbox_tbl ? 1  :  0;
	
	ret = true;
	
FINISH_OFF: 
	if (imap_mailbox_tbl != NULL)
		emstorage_free_mailbox(&imap_mailbox_tbl, 1, NULL);
	
	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}


/* description
 *    set sync imap mailbox
 * arguments
 *    mailbox_list  :  imap mailbox to be synced
 *    syncronous  :  0-sync 1 : non-sync
 * return
 *    succeed  :  1
 *    fail  :  0
 */

INTERNAL_FUNC int emcore_set_sync_imap_mailbox(emf_mailbox_t *mailbox, int synchronous, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], synchronous[%d], err_code[%p]", mailbox, synchronous, err_code);
	
	if (!mailbox)  {
		EM_DEBUG_EXCEPTION("mailbox[%p], synchronous[%d]", mailbox, synchronous);
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	emstorage_mailbox_tbl_t *imap_mailbox_tbl_item = NULL;
	emcore_uid_list *uid_list = NULL;
	emstorage_read_mail_uid_tbl_t *downloaded_uids = NULL;
	MAILSTREAM *stream = mailbox->mail_stream;
	int mailbox_renamed = 0;
	int j = 0;
	int i = 0;
	int temp = 0;
	IMAPLOCAL *imap_local = NULL;
	char cmd[128] = { 0 , }, tag[32] = { 0 , }, *p = NULL;
		
	if (synchronous) {		
		/* if synchcronous, insert imap mailbox to db */
		if (emstorage_get_mailbox_by_name(mailbox->account_id, 0, mailbox->name, &imap_mailbox_tbl_item, true, &err))  {	
			/* mailbox already exists */
			/* mailbox Found, Do set the modifiable_yn = 1 */
			EM_DEBUG_LOG("mailbox already exists and setting modifiable_yn to 1");
			if (!emstorage_update_mailbox_modifiable_yn(mailbox->account_id, 0, mailbox->name, 1, true, &err)) {
				EM_DEBUG_EXCEPTION(" emstorage_update_mailbox_modifiable_yn Failed [ %d ] ", err);
				goto JOB_ERROR;
			}
		}
		else {
			if (err != EMF_ERROR_MAILBOX_NOT_FOUND) {
				EM_DEBUG_EXCEPTION(">>>>.>>>>>Getting mailbox failed>>>>>>>>>>>>>>");
				/* This is error scenario so finish the job */
				goto JOB_ERROR;
			}
			else {
				/* This is not error scenario - mailbox is either new/renamed mailbox and needs to be added/modfied in DB */
				/* Now check if mailbox is renamed */
				EM_DEBUG_LOG(">>>>>>>>>>>>>>>>>>>>>>>MAILBOX NEW OR RENAMED");
				if (stream) {
					imap_local = ((MAILSTREAM *)stream)->local;
					EM_DEBUG_LINE;
					sprintf(tag, "%08lx", 0xffffffff & (((MAILSTREAM *)stream)->gensym++));
					EM_DEBUG_LINE;
					sprintf(cmd, "%s SELECT %s\015\012", tag, mailbox->name);
					EM_DEBUG_LINE;

				}
				
				/* select the mailbox and get its UID */
				if (!imap_local || !imap_local->netstream || !net_sout(imap_local->netstream, cmd, (int)strlen(cmd))) {
  					EM_DEBUG_EXCEPTION("network error - failed to IDLE on Mailbox [%s]", mailbox->name);
					/*
					err = EMF_ERROR_CONNECTION_BROKEN;
					if(imap_local)
						imap_local->netstream = NULL;
					mailbox->mail_stream = NULL;
					goto JOB_ERROR;
					*/
				}
				else {
					EM_DEBUG_LOG("Get response for select call");
					while (imap_local->netstream) {
    					p = net_getline(imap_local->netstream);
 						EM_DEBUG_LOG("p =[%s]", p);
						if (!strncmp(p, "+", 1)) {
							ret = 1;
							break;
						}
						else if (!strncmp(p, "*", 1)) {
		           			EM_SAFE_FREE(p); 
		           			continue;
						}
						else {
							ret = 0;
							break;
						}
    				}
					EM_SAFE_FREE(p); 
					EM_DEBUG_LINE;
					/* check if OK or BAD response comes. */
					/* if response is OK the try getting UID list. */
					if (!strncmp((char *)imap_local->reply.key, "OK", strlen("OK")))  {
						EM_DEBUG_LOG(">>>>>>>>>>Select success on %s mailbox", mailbox->name);
						if (!imap4_mailbox_get_uids(stream, &uid_list, &err)) {
							EM_DEBUG_EXCEPTION("imap4_mailbox_get_uids failed - %d", err);
							EM_SAFE_FREE(uid_list);
						}
						else {
							if (!emstorage_get_downloaded_list(mailbox->account_id, NULL, &downloaded_uids, &j, true, &err)) {
								EM_DEBUG_EXCEPTION("emstorage_get_downloaded_list failed [%d]", err);
						
								downloaded_uids = NULL;
							}
							else /* Prevent Defect - 28497 */ {
								emcore_uid_list *uid_elem = uid_list;
								emcore_uid_list *next_uid_elem = NULL;
								if (uid_elem) {
									for (i = j; (i > 0 && !mailbox_renamed); i--)  {
										if (uid_elem) {
											next_uid_elem = uid_elem->next;
											if (uid_elem->uid && downloaded_uids[i - 1].s_uid && !strcmp(uid_elem->uid, downloaded_uids[i - 1].s_uid)) {
												temp = i-1;
												mailbox_renamed = 1;
												break;
											}
											EM_SAFE_FREE(uid_elem->uid);
											uid_elem = next_uid_elem;
										}
									}
								}
							}
						}
					} /* mailbox selected */
				}	

				if (mailbox_renamed) /* renamed mailbox */ {
					EM_DEBUG_LOG("downloaded_uids[temp].mailbox_name [%s]", downloaded_uids[temp].mailbox_name);
					/* Do a mailbox rename in the DB */
					if (!emstorage_modify_mailbox_of_mails(downloaded_uids[temp].mailbox_name, mailbox->name, true, &err))
						EM_DEBUG_EXCEPTION(" emstorage_modify_mailbox_of_mails Failed [%d]", err);

					mailbox_renamed = 0;

					emstorage_mailbox_tbl_t mailbox_tbl;
					mailbox_tbl.account_id = mailbox->account_id;
					mailbox_tbl.local_yn = 0;
					mailbox_tbl.mailbox_name = mailbox->name;
					mailbox_tbl.mailbox_type = mailbox->mailbox_type;

					/* Get the Alias Name after parsing the Full mailbox Path */
					mailbox->alias = emcore_get_alias_of_mailbox((const char *)mailbox->name);
					
					if (mailbox->alias)
						EM_DEBUG_LOG("mailbox->alias [%s] ", mailbox->alias);

					mailbox_tbl.alias = mailbox->alias;
					mailbox_tbl.sync_with_server_yn  = 0; 
					mailbox_tbl.modifiable_yn = 1;
					mailbox_tbl.total_mail_count_on_server = 0;
					
					/* if non synchronous, delete imap mailbox from db */
					if (!emstorage_update_mailbox(mailbox->account_id, 0, downloaded_uids[temp].mailbox_name, &mailbox_tbl, true, &err)) {
						EM_DEBUG_EXCEPTION(" emstorage_update_mailbox Failed [ %d ] ", err);
						goto JOB_ERROR;
					}
					
				}
				else /* Its a Fresh Mailbox */ {
					emstorage_mailbox_tbl_t mailbox_tbl;
					mailbox_tbl.mailbox_id = mailbox->mailbox_id;
					mailbox_tbl.account_id = mailbox->account_id;
					mailbox_tbl.local_yn = 0; 
					mailbox_tbl.mailbox_type = mailbox->mailbox_type;
					mailbox_tbl.mailbox_name = mailbox->name;
					mailbox_tbl.mail_slot_size = mailbox->mail_slot_size;

					/* Get the Alias Name after Parsing the Full mailbox Path */
					mailbox->alias = emcore_get_alias_of_mailbox((const char *)mailbox->name);

					if (mailbox->alias) {
						EM_DEBUG_LOG("mailbox->alias [%s] ", mailbox->alias);

						mailbox_tbl.alias = mailbox->alias;
						mailbox_tbl.sync_with_server_yn = 1;
						mailbox_tbl.modifiable_yn = 1; 
						mailbox_tbl.total_mail_count_on_server = 0;
							
						EM_DEBUG_LOG("mailbox_tbl.mailbox_type - %d", mailbox_tbl.mailbox_type);

						if (!emstorage_add_mailbox(&mailbox_tbl, true, &err)) {
							EM_DEBUG_EXCEPTION("emstorage_add_mailbox failed - %d", err);
							goto JOB_ERROR;
						}
					}
				}
			}
		}
	}

	/* set sync db mailbox */
	mailbox->synchronous = synchronous;
	
	ret = true;
	
JOB_ERROR:

	if (downloaded_uids) 
		emstorage_free_read_mail_uid(&downloaded_uids, j, NULL);

	if (imap_mailbox_tbl_item)
		emstorage_free_mailbox(&imap_mailbox_tbl_item, 1, NULL);

	if (err_code)
		*err_code = err;
	
	return ret;
}

/* description
 *    create a new imap mailbox
 * arguments
 *    new_mailbox  :  imap mailbox to be created
 * return
 *    succeed  :  1
 *    fail  :  0
 */
INTERNAL_FUNC int emcore_create_imap_mailbox(emf_mailbox_t *mailbox, int *err_code)
{
    MAILSTREAM *stream = NULL;
    char *long_enc_path = NULL;
    void *tmp_stream = NULL;
    int ret = false;
    int err = EMF_ERROR_NONE;

    EM_DEBUG_FUNC_BEGIN();

	if (err_code) {
		*err_code = EMF_ERROR_NONE;
	}

    if (!mailbox)
    {
        err = EMF_ERROR_INVALID_PARAM;
        goto JOB_ERROR;
    }

    /* connect mail server */
    stream = NULL;
    if (!emcore_connect_to_remote_mailbox(mailbox->account_id, NULL, (void **)&tmp_stream, NULL))
    {
        err = EMF_ERROR_CONNECTION_FAILURE;
        goto JOB_ERROR;
    }

    stream = (MAILSTREAM *)tmp_stream;

    /* encode mailbox name by UTF7, and rename to full path (ex : {mail.test.com}child_mailbox) */
    if (!emcore_get_long_encoded_path(mailbox->account_id, mailbox->name, '/', &long_enc_path, err_code))
    {
        err = EMF_ERROR_UNKNOWN;
        goto JOB_ERROR;
    }

    /* create mailbox */
    if (!mail_create(stream, long_enc_path))
    {
        err = EMF_ERROR_UNKNOWN;
        goto JOB_ERROR;
    }

	emcore_close_mailbox(0, stream);				
	stream = NULL;

    EM_SAFE_FREE(long_enc_path);

    ret = true;

JOB_ERROR:
    if (stream)
    {
		emcore_close_mailbox(0, stream);				
		stream = NULL;
    }

    EM_SAFE_FREE(long_enc_path);

    if (err_code)
        *err_code = err;

    return ret;
}


/* description
 *    delete a imap mailbox
 * arguments
 *    mailbox  :  mailbox to be deleted
 * return
 *    succeed  :  1
 *    fail  :  0
 */
INTERNAL_FUNC int emcore_delete_imap_mailbox(emf_mailbox_t *mailbox, int *err_code)
{
    MAILSTREAM *stream = NULL;
    char *long_enc_path = NULL;
    emf_account_t *ref_account = NULL;
    void *tmp_stream = NULL;
    int ret = false;
    int err = EMF_ERROR_NONE;

    EM_DEBUG_FUNC_BEGIN();

	if (err_code) {
		*err_code = EMF_ERROR_NONE;
	}

    if (!mailbox)
    {
        err = EMF_ERROR_INVALID_PARAM;
        goto JOB_ERROR;
    }

    ref_account = emcore_get_account_reference(mailbox->account_id);
    if (!ref_account)
    {
        err = EMF_ERROR_INVALID_PARAM;
        goto JOB_ERROR;
    }

    /* if not imap4 mail, return */
    if (ref_account->account_bind_type != EMF_BIND_TYPE_EM_CORE || 
            ref_account->receiving_server_type != EMF_SERVER_TYPE_IMAP4)
    {
        err = EMF_ERROR_INVALID_PARAM;
        goto JOB_ERROR;
    }

    /* connect mail server */
    stream = NULL;
    if (!emcore_connect_to_remote_mailbox(ref_account->account_id, NULL, (void **)&tmp_stream, NULL))
    {
        err = EMF_ERROR_CONNECTION_FAILURE;
        goto JOB_ERROR;
    }

    stream = (MAILSTREAM *)tmp_stream;

    /* encode mailbox name by UTF7, and rename to full path (ex : {mail.test.com}child_mailbox) */
    if (!emcore_get_long_encoded_path(mailbox->account_id, mailbox->name, '/', &long_enc_path, err_code))
    {
        err = EMF_ERROR_UNKNOWN;
        goto JOB_ERROR;
    }

    /* delete mailbox */
    if (!mail_delete(stream, long_enc_path))
    {
        err = EMF_ERROR_UNKNOWN;
        goto JOB_ERROR;
    }

	emcore_close_mailbox(0, stream);				
	stream = NULL;

    EM_SAFE_FREE(long_enc_path);

    /* if deleted imap mailbox is synchronous mailbox, delete db imap mailbox from db */
    if (!emstorage_delete_mailbox(ref_account->account_id, 0, mailbox->name, true, &err)) {
		EM_DEBUG_EXCEPTION("\t emstorage_delete_mailbox failed - %d", err);
        goto JOB_ERROR;
    }

    ret = true;

JOB_ERROR:
    if (stream)
    {
		emcore_close_mailbox(0, stream);				
		stream = NULL;
    }

    EM_SAFE_FREE(long_enc_path);

    if (err_code)
        *err_code = err;

    return ret;
}


/* description
 *    change imap mailbox name
 * arguments
 *    old_mailbox  :  previous mailbox
 *    new_mailbox  :  renamed mailbox
 * return
 *    succeed  :  1
 *    fail  :  0
 */
INTERNAL_FUNC int emcore_modify_imap_mailbox(emf_mailbox_t *old_mailbox,  emf_mailbox_t *new_mailbox, int *err_code)
{
    MAILSTREAM *stream = NULL;
    char *long_enc_path = NULL;
    char *long_enc_path_new = NULL;
    emstorage_mailbox_tbl_t imap_mailbox_tbl_item;
    emf_account_t *ref_account = NULL;
    void *tmp_stream = NULL;
    int ret = false;
    int err = EMF_ERROR_NONE;

    EM_DEBUG_FUNC_BEGIN();

	if (err_code) {
		*err_code = EMF_ERROR_NONE;
	}

    if (!old_mailbox || !new_mailbox)
    {
        err = EMF_ERROR_INVALID_PARAM;
        goto JOB_ERROR;
    }

    ref_account = emcore_get_account_reference(old_mailbox->account_id);
    if (!ref_account)
    {
        err = EMF_ERROR_INVALID_PARAM;
        goto JOB_ERROR;
    }

    if (!emcore_get_account_reference(new_mailbox->account_id))
    {
        err = EMF_ERROR_INVALID_PARAM;
        goto JOB_ERROR;
    }

    /* if not imap4 mail, return */
    if (ref_account->account_bind_type != EMF_BIND_TYPE_EM_CORE || 
            ref_account->receiving_server_type != EMF_SERVER_TYPE_IMAP4)
    {
        err = EMF_ERROR_INVALID_PARAM;
        goto JOB_ERROR;
    }

    /* connect mail server */
    stream = NULL;
    if (!emcore_connect_to_remote_mailbox(ref_account->account_id, NULL, (void **)&tmp_stream, NULL))
    {
        err = EMF_ERROR_CONNECTION_FAILURE;
        goto JOB_ERROR;
    }

    stream = (MAILSTREAM *)tmp_stream;

    /* encode mailbox name by UTF7, and rename to full path (ex : {mail.test.com}child_mailbox) */
    if (!emcore_get_long_encoded_path(old_mailbox->account_id, old_mailbox->name, '/', &long_enc_path, err_code))
    {
        err = EMF_ERROR_UNKNOWN;
        goto JOB_ERROR;
    }

    /* encode mailbox name by UTF7, and rename to full path (ex : {mail.test.com}child_mailbox) */
    if (!emcore_get_long_encoded_path(new_mailbox->account_id, new_mailbox->name, '/', &long_enc_path_new, err_code))
    {
        err = EMF_ERROR_UNKNOWN;
        goto JOB_ERROR;
    }

    /* rename mailbox */
    if (!mail_rename(stream, long_enc_path, long_enc_path_new))
    {
        err = EMF_ERROR_UNKNOWN;
        goto JOB_ERROR;
    }

    EM_SAFE_FREE(long_enc_path);
    EM_SAFE_FREE(long_enc_path_new);

	emcore_close_mailbox(0, stream);				
	stream = NULL;

    imap_mailbox_tbl_item.alias = NULL;
    imap_mailbox_tbl_item.mailbox_name = new_mailbox->name;
    imap_mailbox_tbl_item.modifiable_yn = 1;

    /* if modified imap mailbox is synchronous mailbox, update mailbox name from imap mailbox table */
    if (!emstorage_update_mailbox(ref_account->account_id, 0, old_mailbox->name, &imap_mailbox_tbl_item, true, &err))
    {
        EM_DEBUG_EXCEPTION("emstorage_update_mailbox failed - %d", err);
        goto JOB_ERROR;
    }

    ret = true;

JOB_ERROR:
    EM_SAFE_FREE(long_enc_path);
    EM_SAFE_FREE(long_enc_path_new);

    if (stream)
    {
		emcore_close_mailbox(0, stream);				
		stream = NULL;
    }

    if (err_code)
        *err_code = err;

    return ret;
}
