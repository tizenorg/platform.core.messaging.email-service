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
 * File :  email-storage.h
 * Desc :  email-core Storage Library Header
 *
 * Auth : 
 *
 * History : 
 *    2006.07.28  :  created
 *****************************************************************************/
#ifndef __EMAIL_STORAGE_H__
#define __EMAIL_STORAGE_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include <sqlite3.h>
#include <time.h>
#include "email-types.h"
#include "email-core-tasks.h"
#include "email-internal-types.h"

#define FIRST_ACCOUNT_ID    1


#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__
#define QUERY_SIZE          8192
#define MAX_INTEGER_LENGTH  5  /*  32767 -> 5 bytes */

typedef struct 
{
	int mail_id;
	unsigned long server_mail_id;
} email_id_set_t;

#endif /* __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__ */


typedef struct 
{
	/* Account information */
	int                                account_id;                               /* Account id  */
	char                              *account_name;                             /* Account name */
	char                              *logo_icon_path;                           /* Account logo icon path */
	int                                account_svc_id;                           /* Specifies id from account-svc */
	int                                sync_status;                              /* Sync Status. SYNC_STATUS_FINISHED, SYNC_STATUS_SYNCING, SYNC_STATUS_HAVE_NEW_MAILS */
	int                                sync_disabled;                            /* If this attriube is set as true, email-service will not synchronize this account. */
	int                                default_mail_slot_size;

	void                              *user_data;                                /* binary user data */
	int                                user_data_length;                         /* user data length */

	/* User information */
	char                              *user_display_name;                        /* User's display name */
	char                              *user_email_address;                       /* User's email address */
	char                              *reply_to_address;                         /* Email address for reply */
	char                              *return_address;                           /* Email address for error from server*/

	/* Configuration for incoming server */
	email_account_server_t             incoming_server_type;                     /* Incoming server type */
	char                              *incoming_server_address;                  /* Incoming server address */
	int                                incoming_server_port_number;              /* Incoming server port number */
	char                              *incoming_server_user_name;                /* Incoming server user name */
	char                              *incoming_server_password;                 /* Incoming server password */
	int                                incoming_server_secure_connection;        /* Does incoming server requires secured connection? */

	/* Options for incoming server */
	email_imap4_retrieval_mode_t       retrieval_mode;                           /* Retrieval mode : EMAIL_IMAP4_RETRIEVAL_MODE_NEW or EMAIL_IMAP4_RETRIEVAL_MODE_ALL */
	int                                keep_mails_on_pop_server_after_download;  /* Keep mails on POP server after download */
	int                                check_interval;                           /* Specifies the interval for checking new mail periodically */
	int                                auto_download_size;                       /* Specifies the size for auto download in bytes. -1 means entire mails body */

	/* Configuration for outgoing server */
	email_account_server_t             outgoing_server_type;
	char                              *outgoing_server_address;
	int                                outgoing_server_port_number;
	char                              *outgoing_server_user_name;
	char                              *outgoing_server_password;
	int                                outgoing_server_secure_connection;        /* Does outgoing server requires secured connection? */
	int                                outgoing_server_need_authentication;      /* Does outgoing server requires authentication? */
	int                                outgoing_server_use_same_authenticator;   /* Use same authenticator for outgoing server */ /* flag2 */

	/* Options for outgoing server */
	email_option_t                     options;

	/* Authentication Options */
	int                                pop_before_smtp;                          /* POP before SMTP Authentication */
	int                                incoming_server_requires_apop;            /* APOP authentication */

	/* S/MIME Options */
	email_smime_type                   smime_type;                               /* Sepeifies the smime type 0=Normal 1=Clear signed 2=encrypted 3=Signed + encrypted */
	char                              *certificate_path;                         /* Sepeifies the certificate path of private*/
	email_cipher_type                  cipher_type;                              /* Sepeifies the encryption algorithm*/
	email_digest_type                  digest_type;                              /* Sepeifies the digest algorithm*/
} emstorage_account_tbl_t;

typedef struct 
{
	int certificate_id;
	int issue_year;
	int issue_month;
	int issue_day;
	int expiration_year;
	int expiration_month;
	int expiration_day;
	char *issue_organization_name;
	char *email_address;
	char *subject_str;
	char *filepath;
	char *password;
} emstorage_certificate_tbl_t;

typedef struct 
{
	int   account_id;      /*  MUST BE '0'  :  currently, only global rule supported. */
	int   rule_id;
	int   type;            /*  from/subject/body  */
	char *value;
	int   action_type;     /*  move/block/delete  */
	int   target_mailbox_id;    /*  destination mailbox  */
	int   flag1;           /*  for rule ON/OFF */
	int   flag2;           /*  For rule type INCLUDE/Exactly SAME AS */
} emstorage_rule_tbl_t;

/* mail_box_tbl table entity */
typedef struct 
{
	int                   mailbox_id;
	int                   account_id;
	int                   local_yn;
	char                 *mailbox_name;
	email_mailbox_type_e  mailbox_type;
	char                 *alias;
	int                   sync_with_server_yn;        /*  whether mailbox is a sync IMAP mailbox */
	int                   modifiable_yn;              /*  whether mailbox is able to be deleted/modified */
	int                   unread_count;               /*  Removed. 16-Dec-2010, count unread mails at the moment it is required. not store in the DB */
	int                   total_mail_count_on_local;  /*  Specifies the total number of mails in the mailbox in the local DB. count unread mails at the moment it is required. not store in the DB */
	int                   total_mail_count_on_server; /*  Specifies the total number of mails in the mailbox in the mail server */
	int                   has_archived_mails;
	int                   mail_slot_size;
	int                   no_select;
	time_t                last_sync_time;             /*  The last synchronization time */
} emstorage_mailbox_tbl_t;

/* mail_read_uid_tbl table entity */
typedef struct 
{
	int     account_id;
	int     mailbox_id;
	int     local_uid;
	char   *mailbox_name;   /*  server mailbox */
	char   *s_uid;          /*  uid on server */
	int     data1;          /*  rfc822 size */
	char   *data2;
	int     flag;           /*  rule id */
	int     reserved;
} emstorage_read_mail_uid_tbl_t;

typedef struct 
{
	int                    mail_id;
	int                    account_id;
	int                    mailbox_id;
	char                  *mailbox_name;
	int                    mailbox_type;
	char                  *subject;
	time_t                 date_time;
	int                    server_mail_status;
	char                  *server_mailbox_name;
	char                  *server_mail_id;
	char                  *message_id;
	int                    reference_mail_id;
	char                  *full_address_from;
	char                  *full_address_reply;
	char                  *full_address_to;
	char                  *full_address_cc;
	char                  *full_address_bcc;
	char                  *full_address_return;
	char                  *email_address_sender;
	char                  *email_address_recipient;
	char                  *alias_sender;
	char                  *alias_recipient;
	int                    body_download_status;
	char                  *file_path_plain;
	char                  *file_path_html;
	char                  *file_path_mime_entity;
	int                    mail_size;
	char                   flags_seen_field;
	char                   flags_deleted_field;
	char                   flags_flagged_field;
	char                   flags_answered_field;
	char                   flags_recent_field;
	char                   flags_draft_field;
	char                   flags_forwarded_field;
	int                    DRM_status;
	email_mail_priority_t  priority;
	email_mail_status_t    save_status;
	int                    lock_status;
	email_mail_report_t    report_status;
	int                    attachment_count;
	int                    inline_content_count;
	int                    thread_id;
	int                    thread_item_count;
	char                  *preview_text;
	email_mail_type_t      meeting_request_status;
	email_message_class    message_class;
	email_digest_type      digest_type;
	email_smime_type       smime_type;
} emstorage_mail_tbl_t;

/* mail_attachment_tbl entity */
typedef struct 
{
	int   attachment_id;
	char *attachment_name;
	char *attachment_path;
	int   attachment_size;
	int   mail_id;
	int   account_id;
	int   mailbox_id;
	int   attachment_save_status;              /* 1 : existing, 0 : not existing */
	int   attachment_drm_type;                 /* 0 : none, 1 : object, 2 : rights, 3 : dcf */
	int   attachment_drm_method;               /* 0 : none, 1 : FL, 2 : CD, 3 : SSD, 4 : SD */
	int   attachment_inline_content_status;    /* 1 : inline content , 0 : not inline content */
	char *attachment_mime_type;
#ifdef __ATTACHMENT_OPTI__	
	int   encoding;
	char *section;	
#endif	
} emstorage_attachment_tbl_t;


typedef enum {
	RETRIEVE_ALL = 1,           /*  mail */
	RETRIEVE_ENVELOPE,          /*  mail envelope */
	RETRIEVE_SUMMARY,           /*  mail summary */
	RETRIEVE_FIELDS_FOR_DELETE, /*  account_id, mail_id, server_mail_yn, server_mailbox, server_mail_id */
	RETRIEVE_ACCOUNT,           /*  account_id */
	RETRIEVE_FLAG,              /*  mailbox, flag1, thread_id */
	RETRIEVE_ID,                /*  mail_id */
	RETRIEVE_ADDRESS,           /*  mail_id, contact_info */
} emstorage_mail_field_type_t;

typedef struct _emstorage_search_filter_t {
	char *key_type;
	char *key_value;
	struct _emstorage_search_filter_t *next;
} emstorage_search_filter_t;

typedef enum {
	EMAIL_CREATE_DB_NORMAL = 0, 
	EMAIL_CREATE_DB_CHECK, 
} emstorage_create_db_type_t;


/*****************************************************************************
								etc
*****************************************************************************/
#define MAX_PW_FILE_NAME_LENGTH                   128 /*  password file name for secure storage */

#ifdef __FEATURE_LOCAL_ACTIVITY__

typedef enum
{
 ACTIVITY_FETCHIMAPFOLDER = 1,  /* Fetch Mail server Activity */
 ACTIVITY_DELETEMAIL, 	 		/* Delete Mail Activity */		
 ACTIVITY_MODIFYFLAG, 			/* Modify Mail flag Activity */		
 ACTIVITY_MODIFYSEENFLAG, 		/* Modify Mail seen flag Activity */
 ACTIVITY_MOVEMAIL, 				/* Move Mail Activity */
 ACTIVITY_DELETEALLMAIL, 		/* Delete All Mails activity */
 ACTIVITY_COPYMAIL, 				/* Copy Mail Activity */
 ACTIVITY_SAVEMAIL, 				/* Save Mail activity */
 ACTIVITY_DELETEMAIL_SEND, 		/* New Delete Mail Activity added to be processed by send thread */
}eActivity_type;


/**
 * emstorage_activity_tbl_t - Email Local activity Structure
 *
 */
typedef struct
{
	int activity_id;				/* Activity ID */
	int account_id;		      		 /* Account ID */
	int mail_id;		    		 /* Mail ID    */
	int activity_type;    /* Local Activity Type */
	char *server_mailid;		     /* Server Mail ID or Mailbox name */
	char *src_mbox;		      		 /* Source Mailbox in case of mail_move */	
	char *dest_mbox;                 /* Destination Mailbox name in case of Mail move operation */

} emstorage_activity_tbl_t;

#endif

INTERNAL_FUNC int emstorage_shm_file_init(const char *shm_file_name);


/************** Database Management ***************/
/*
 * em_db_open
 * description  :  open db and register busy handler
 */
INTERNAL_FUNC int em_db_open(sqlite3 **sqlite_handle, int *err_code);

/*
 * emstorage_db_open
 *
 * description :  open db and set global variable sqlite_emmb 
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC sqlite3* emstorage_db_open(int *err_code);

/*
 * emstorage_db_close
 *
 * description :  close db with global variable sqlite_emmb
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_db_close(int *err_code);

/*
 * emstorage_open
 *
 * description :  initialize storage manager
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_open(int *err_code);

/*
 * emstorage_close
 *
 * description :  cleanup storage manager
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_close(int *err_code);

/*
 * emstorage_create_table
 *
 * description :  create account/address table in database.
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_create_table(emstorage_create_db_type_t type, int *err_code);


/**
 * Check whether there is the same account information in the db
 *
 * @param[in] account	account that 
 * @param[in] transaction	If the argument is true, engine commits transaction or executes rollback.
 * @param[out] err_code	Error code.
 * @remarks N/A
 * @return This function returns true if there is no duplicated account. returns false and set error code if there are duplicated accounts or error 
 */
INTERNAL_FUNC int emstorage_check_duplicated_account(email_account_t *account, int transaction, int *err_code);


/**
 * Get number of accounts from account table.
 *
 * @param[out] count		The number of accounts is saved here.
 * @param[in] transaction	If the argument is true, engine commits transaction or executes rollback.
 * @remarks N/A
 * @return This function returns 0 on success or error code on failure.
 */
INTERNAL_FUNC int emstorage_get_account_count(int *count, int transaction, int *err_code);

/**
 * Get account from account table.
 *
 * @param[in, out] select_num	Upon entry, the argument contains the number of accounts to select.
	                            Upon exit, it is set to the number of accounts to been selected.
 * @param[out] account_list		The selected accounts is saved here. this pointer must be freed after being used.
 * @param[in] transaction		If the argument is true, engine commits transaction or executes rollback.
 * @remarks N/A
 * @return This function returns 0 on success or error code on failure.
 */
INTERNAL_FUNC int emstorage_get_account_list(int *select_num, emstorage_account_tbl_t **account_list, int transaction, int with_password, int *err_code);

/*
 * emstorage_get_account_by_name
 *
 * description :  get account from account table by account name
 * arguments : 
 *    db  :  database pointer
 *    account_id  :  account id
 *    pulloption  :  option to specify fetching full or partial data
 *    account  :  buffer to hold selected account
 *              this buffer must be free after it has been used.
 * return  : 
 */
/* sowmya.kr, 281209 Adding signature to options in email_account_t changes */

INTERNAL_FUNC int emstorage_get_account_by_id(int account_id, int pulloption, emstorage_account_tbl_t **account, int transaction, int *err_code);


/*
 * emstorage_get_password_length_of_account
 *
 * description: get password length of account.
 * arguments:
 *    account_id : account id
 *    password_length : password length
 * return :
 */

INTERNAL_FUNC int emstorage_get_password_length_of_account(int account_id, int *password_length, int* err_code);

/*
 * emstorage_update_account
 *
 * description :  change a account from account table
 * arguments : 
 *    account_id  :  account id
 *    account  :  buffer to hold selected account
 * return  : 
 */
INTERNAL_FUNC int emstorage_update_account(int account_id, emstorage_account_tbl_t *account, int transaction, int *err_code);


INTERNAL_FUNC int emstorage_set_field_of_accounts_with_integer_value(int account_id, char *field_name, int value, int transaction);

/*
 * emstorage_get_sync_status_of_account
 *
 * description :  get a sync status field from account table
 * arguments : 
 *    account_id  :  account id
 *    result_sync_status : sync status value
 * return  : 
 */

INTERNAL_FUNC int emstorage_get_sync_status_of_account(int account_id, int *result_sync_status,int *err_code);

/*
 * emstorage_update_sync_status_of_account
 *
 * description :  update a sync status field from account table
 * arguments : 
 *    account_id  :  account id
 *    set_operator  :  set operater. refer email_set_type_t
 *    sync_status : sync status value
 * return  : 
 */

INTERNAL_FUNC int emstorage_update_sync_status_of_account(int account_id, email_set_type_t set_operator, int sync_status, int transaction, int *err_code);

/*
 * emstorage_add_account
 *
 * description :  add a account to account table
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_add_account(emstorage_account_tbl_t *account, int transaction, int *err_code);

/*
 * emstorage_delete_account
 *
 * description :  delete a account from account table
 * arguments : 
 *    db  :  database pointer
 *    account_id  :  account id to be deteted
 * return  : 
 */
INTERNAL_FUNC int emstorage_delete_account(int account_id, int transaction, int *err_code);

/*
 * emstorage_free_account
 *
 * description :  free local accout memory
 * arguments : 
 *    account_list  :  double pointer
 *    count  :  count of local account
 * return  : 
 */
INTERNAL_FUNC int emstorage_free_account(emstorage_account_tbl_t **account_list, int count, int *err_code);


/************** Mailbox(Local box And Imap mailbox) Management ******************/

/*
 * emstorage_get_mailbox_count
 *
 * description :  get number of mailbox from local mailbox table
 * arguments : 
 *    db  :  database pointer
 *    count  :  number of accounts
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_mailbox_count(int account_id, int local_yn, int *count, int transaction, int *err_code);

/*
 * emstorage_get_mailbox
 *
 * description :  get local mailbox from local mailbox table
 * arguments : 
 *    db  :  database pointer
 *    sort_type 	 :  	in - sorting type.
 *    select_num 	 :  	in - number of selected account
 *                 		out - number to been selected
 *    mailbox_list 	 :  	buffer to hold selected account
 *                   	this buffer must be free after it has been used.
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_mailbox_list(int account_id, int local_yn, email_mailbox_sort_type_t sort_type, int *select_num, emstorage_mailbox_tbl_t **mailbox_list,  int transaction, int *err_code);

/*
 * emstorage_get_child_mailbox_list
 *
 * description :  get child mailbox list  from given  mailbox 
 * arguments : 
 *    account_id  :  in - account id
 *    parent_mailbox_name  :  in - parent_mailbox_name to fetch child list
 *    local_yn - in - local mailbox or not
 *    select_num - out   :  count of mailboxes
 *    mailbox_list - out  :    list of mailboxes        
 *    err_code - out  :  error code, if any
 */


INTERNAL_FUNC int emstorage_get_child_mailbox_list(int account_id, char *parent_mailbox_name, int *select_num, emstorage_mailbox_tbl_t **mailbox_list, int transaction, int *err_code);

/*
 * emstorage_get_mailbox_by_name
 *
 * description :  get local mailbox from local mailbox table by mailbox name
 * arguments : 
 *    db  :  database pointer
 *    mailbox_name  :  mailbox name
 *    mailbox  :  buffer to hold selected local mailbox
 *              this buffer must be free after it has been used.
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_mailbox_by_name(int account_id, int local_yn, char *mailbox_name, emstorage_mailbox_tbl_t **mailbox, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_get_mailbox_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type, emstorage_mailbox_tbl_t **mailbox, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_get_mailbox_by_id(int input_mailbox_id, emstorage_mailbox_tbl_t** output_mailbox);
INTERNAL_FUNC int emstorage_get_mailbox_list_ex(int account_id, int local_yn, int with_count, int *select_num, emstorage_mailbox_tbl_t **mailbox_list, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_get_mailbox_id_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type, int *mailbox_id, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_get_mailbox_name_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type, char **mailbox_name, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_update_mailbox_modifiable_yn(int account_id, int local_yn, char *mailbox_name, int modifiable_yn, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_update_mailbox_total_count(int account_id, int input_mailbox_id, int total_count_on_server, int transaction, int *err_code);


/*
 * emstorage_update_mailbox
 *
 * description :  change a account from account table
 * arguments : 
 *    db  :  database pointer
 *    mailbox_name  :  mailbox name
 *    mailbox  :  buffer to hold selected local mailbox
 * return  : 
 */
INTERNAL_FUNC int emstorage_update_mailbox(int account_id, int local_yn, int input_mailbox_id, emstorage_mailbox_tbl_t *mailbox, int transaction, int *err_code);

/*
 * emstorage_update_mailbox_type
 *
 * description :  change a mailbox from mailbox tbl
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_update_mailbox_type(int account_id, int local_yn, char *mailbox_name, email_mailbox_type_e new_mailbox_type, int transaction, int *err_code);

/*
 * emstorage_set_local_mailbox
 *
 * description :  change 'local' field on mailbox table.
 * arguments :
 * return  :
 */
INTERNAL_FUNC int emstorage_set_local_mailbox(int input_mailbox_id, int input_is_local_mailbox, int transaction);

/*
 * emstorage_add_mailbox
 *
 * description :  add a local mailbox to local mailbox table
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_add_mailbox(emstorage_mailbox_tbl_t *mailbox, int transaction, int *err_code);

/*
 * emstorage_delete_mailbox
 *
 * description :  delete a local mailbox from local mailbox table
 * arguments : 
 *    db  :  database pointer
 *    mailbox_name  :  mailbox name of record to be deteted
 * return  : 
 */
INTERNAL_FUNC int emstorage_delete_mailbox(int account_id, int local_yn, int input_mailbox_id, int transaction, int *err_code);

INTERNAL_FUNC int emstorage_rename_mailbox(int input_mailbox_id, char *input_new_mailbox_path, char *input_new_mailbox_alias, int input_transaction);
INTERNAL_FUNC int emstorage_get_overflowed_mail_id_list(int account_id, int input_mailbox_id, int mail_slot_size, int **mail_id_list, int *mail_id_count, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_set_mail_slot_size(int account_id, int mailbox_id, int new_slot_size, int transaction, int *err_code);

INTERNAL_FUNC int emstorage_set_all_mailbox_modifiable_yn(int account_id, int modifiable_yn, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_get_mailbox_by_modifiable_yn(int account_id, int modifiable_yn, int *select_num, emstorage_mailbox_tbl_t **mailbox_list, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_stamp_last_sync_time_of_mailbox(int input_mailbox_id, int input_transaction);

/*
 * emstorage_free_mailbox
 *
 * description :  free local mailbox memory
 * arguments : 
 *    mailbox_list  :  double pointer
 *    count  :  count of local mailbox
 * return  : 
 */
INTERNAL_FUNC int emstorage_free_mailbox(emstorage_mailbox_tbl_t **mailbox_list, int count, int *err_code);


/************** Read Mail UID Management ******************/



INTERNAL_FUNC int emstorage_get_count_read_mail_uid(int account_id, char *mailbox_name, int *count, int transaction, int *err_code);


/*
 * emstorage_check_read_mail_uid
 *
 * description :  check that this uid exists in uid table
 * arguments : 
 *    db  :  database pointer
 *    mailbox_name  :  mailbox name
 *    s_uid  :  uid string to be checked
 *    exist  :  variable to hold checking result. (0 : not exist, 1 : exist)
 * return  : 
 */
INTERNAL_FUNC int emstorage_check_read_mail_uid(int account_id, char *mailbox_name, char *uid, int *exist, int transaction, int *err_code);

/*
 * emstorage_get_read_mail_size
 *
 * description :  get mail size from read mail uid
 * arguments : 
 *    db  :  database pointer
 *    mailbox_id  :  local mailbox id
 *    local_uid  :  mail uid of local mailbox
 *    mailbox_name  :  server mailbox name
 *    uid  :  mail uid string of server mail
 *    read_mail_uid  :  variable to hold read mail uid information
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_downloaded_list(int account_id, int mailbox_id, emstorage_read_mail_uid_tbl_t **read_mail_uid, int *count, int transaction, int *err_code);

INTERNAL_FUNC int emstorage_get_downloaded_mail(int mail_id, emstorage_mail_tbl_t **mail, int transaction, int *err_code);

/*
 * emstorage_get_read_mail_size
 *
 * description :  get mail size from read mail uid
 * arguments : 
 *    db  :  database pointer
 *    mailbox_name  :  mailbox name
 *    s_uid  :  mail uid string
 *    size  :  variable to hold mail size
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_downloaded_mail_size(int account_id, char *local_mbox, int local_uid, char *mailbox_name, char *uid, int *mail_size, int transaction, int *err_code);

/*
 * emstorage_add_downloaded_mail
 *
 * description :  add read mail uid
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_add_downloaded_mail(emstorage_read_mail_uid_tbl_t *read_mail_uid, int transaction, int *err_code);

/*
 * emstorage_change_read_mail_uid
 *
 * description :  modify read mail uid
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_change_read_mail_uid(int account_id, int local_mailbox_id, int local_uid, char *mailbox_name, char *uid,
	                             emstorage_read_mail_uid_tbl_t *read_mail_uid, int transaction, int *err_code);

/*
 * emstorage_remove_downloaded_mail
 *
 * description :  
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_remove_downloaded_mail(int account_id, char *mailbox_name, char *uid, int transaction, int *err_code);


INTERNAL_FUNC int emstorage_update_read_mail_uid(int mail_id, char *new_server_uid, char *mbox_name, int *err_code);

/*
 *  free memroy
 */
INTERNAL_FUNC int emstorage_free_read_mail_uid(emstorage_read_mail_uid_tbl_t **read_mail_uid, int count, int *err_code);



/************** Rules(Filtering) Management ******************/

/*
 * emstorage_get_rule_count
 *
 * description :  get number of rules from rule table
 * arguments : 
 *    db  :  database pointer
 *    count  :  number of accounts
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_rule_count(int account_id, int *count, int transaction, int *err_code);

/*
 * emstorage_get_rule
 *
 * description :  get rules from rule table
 * arguments : 
 *    db  :  database pointer
 *    start_idx  :  the first index to be selected (min : 0)
 *    select_num  :  in - number of selected account
 *                 out - number to been selected
 *    is_completed  :  is completed ? 
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_rule(int account_id, int type, int start_idx, int *select_num, int *is_completed, emstorage_rule_tbl_t **rule_list, int transaction, int *err_code);

/*
 * emstorage_get_rule
 *
 * description :  get rules from rule table
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_rule_by_id(int account_id, int rule_id, emstorage_rule_tbl_t **rule, int transaction, int *err_code);

/*
 * emstorage_change_rule
 *
 * description :  change a account from account table
 * arguments : 
 *    db  :  database pointer
 *    mailbox_name  :  mailbox name
 *    rule  :  buffer to hold selected rule
 * return  : 
 */
INTERNAL_FUNC int emstorage_change_rule(int account_id, int rule_id, emstorage_rule_tbl_t *rule, int transaction, int *err_code);

/*
 * emstorage_find_rule
 *
 * description :  find a rule already exists
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_find_rule(emstorage_rule_tbl_t *rule, int transaction, int *err_code);

/*
 * emstorage_add_rule
 *
 * description :  add a rule to rule table
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_add_rule(emstorage_rule_tbl_t *rule, int transaction, int *err_code);

/*
 * emstorage_delete_rule
 *
 * description :  delete a rule from rule table
 * arguments : 
 *    db  :  database pointer
 *    rule  :  rule to be deteted
 * return  : 
 */
INTERNAL_FUNC int emstorage_delete_rule(int account_id, int rule_id, int transaction, int *err_code);

/*
 * emstorage_free_rule
 *
 * description :  free rule memory
 * arguments : 
 *    count  :  count of rule
 * return  : 
 */
INTERNAL_FUNC int emstorage_free_rule(emstorage_rule_tbl_t **rule_list, int count, int *err_code);


/************** Mail Management ******************/

/*
 * emstorage_get_mail_count
 *
 * description :  get mail total and unseen count from mail table
 * arguments : 
 *    total  :  total count
 *    unseen  :  unseen mail count
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_mail_count(int account_id, const char *mailbox, int *total, int *unseen, int transaction, int *err_code);

/*
 * emstorage_get_mail_by_id
 *
 * description :  get mail from mail table by mail id
 * arguments : 
 *    mail_id  :  mail id
 *    mail  :  double pointer to hold mail
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_mail_by_id(int mail_id, emstorage_mail_tbl_t **mail, int transaction, int *err_code);

/*
 * emstorage_get_mail
 *
 * description :  get mail from mail table by mail sequence
 * arguments : 
 *    account_id  :  account id
 *    mailbox  :  mailbox name
 *    mail_no  :  mail sequence number (not mail id)
 *    mail  :  double pointer to hold mail
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_mail_field_by_id(int mail_id, int type, emstorage_mail_tbl_t **mail, int transaction, int *err_code);

/*
 * emstorage_get_mail_field_by_multiple_mail_id
 *
 * description :  
 * arguments : 
 *    mail_ids  : 
 *    number_of_mails  :  
 *    type  :  
 *    mail  :  
 *    transaction :
 *    err_code :
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_mail_field_by_multiple_mail_id(int mail_ids[], int number_of_mails, int type, emstorage_mail_tbl_t** mail, int transaction, int *err_code);

/*
 * emstorage_query_mail_count
 *
 * description :  query mail count
 */
INTERNAL_FUNC int emstorage_query_mail_count(const char *input_conditional_clause, int input_transaction, int *output_total_mail_count, int *output_unseen_mail_count);

/*
 * emstorage_query_mail_list
 *
 * description :  query mail id list
 */
INTERNAL_FUNC int emstorage_query_mail_id_list(const char *input_conditional_clause, int input_transaction, int **output_mail_id_list, int *output_mail_id_count);

/*
 * emstorage_query_mail_list
 *
 * description :  query mail list information
 */
INTERNAL_FUNC int emstorage_query_mail_list(const char *conditional_clause, int transaction, email_mail_list_item_t** result_mail_list,  int *result_count,  int *err_code);

/*
 * emstorage_query_mail_tbl
 *
 * description :  query mail table information
 */
INTERNAL_FUNC int emstorage_query_mail_tbl(const char *conditional_clause, int transaction, emstorage_mail_tbl_t** result_mail_tbl, int *result_count, int *err_code);

/*
 * emstorage_query_mailbox_tbl
 *
 * description :  query mail box table information
 */

INTERNAL_FUNC int emstorage_query_mailbox_tbl(const char *input_conditional_clause, const char *input_ordering_clause, int input_get_mail_count,  int input_transaction, emstorage_mailbox_tbl_t **output_mailbox_list, int *output_mailbox_count);

/*
 * emstorage_get_mail_list
 *
 * description :  search mail list information
 */

INTERNAL_FUNC int emstorage_get_mail_list(int account_id, int mailbox_id, email_email_address_list_t* addr_list, int thread_id, int start_index, int limit_count, int search_type, const char *search_value, email_sort_type_t sorting, int transaction, email_mail_list_item_t** mail_list,  int *result_count,  int *err_code);
/*
 * emstorage_get_mails
 *
 * description :  search mail list information
 */
INTERNAL_FUNC int emstorage_get_mails(int account_id, int mailbox_id, email_email_address_list_t* addr_list, int thread_id, int start_index, int limit_count, email_sort_type_t sorting,  int transaction, emstorage_mail_tbl_t** mail_list, int *result_count, int *err_code);
INTERNAL_FUNC int emstorage_get_searched_mail_list(int account_id, int mailbox_id, int thread_id, int search_type, const char *search_value, int start_index, int limit_count, email_sort_type_t sorting, int transaction, email_mail_list_item_t **mail_list,  int *result_count,  int *err_code);
INTERNAL_FUNC int emstorage_get_maildata_by_servermailid(int account_id, char *server_mail_id, emstorage_mail_tbl_t **mail, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_get_latest_unread_mailid(int account_id, int *mail_id, int *err_code);


/**
 * Prepare mail search.
 *
 * @param[in] search			Specifies the searching condition.
 * @param[in] account_id		Specifies the account id. if 0, all accounts.
 * @param[in] mailbox			Specifies the mailbox name. if NULL, all mailboxes.
 * @param[in] sorting			Specifies the sorting condition.
 * @param[out] search_handle	the searching handle is saved here.
 * @param[out] searched			the result count is saved here.
 * @remarks N/A
 * @return This function returns 0 on success or error code on failure.
 */
INTERNAL_FUNC int emstorage_mail_search_start(emstorage_search_filter_t *search, int account_id, char *mailbox, int sorting, int *search_handle, int *searched, int transaction, int *err_code);

/*
 * emstorage_mail_search_result
 *
 * description :  retrieve mail as searching result
 * arguments : 
 *    search_handle  :  handle to been gotten from emstorage_mail_search_start
 *    mail  :  double pointer to hold mail
 * return  : 
 */
INTERNAL_FUNC int emstorage_mail_search_result(int search_handle, emstorage_mail_field_type_t type, void **data, int transaction, int *err_code);

/*
 * emstorage_mail_search_end
 *
 * description :  finish searching
 * arguments : 
 *    search_handle  :  handle to be finished
 * return  : 
 */
INTERNAL_FUNC int emstorage_mail_search_end(int search_handle, int transaction, int *err_code);



/*
 * emstorage_set_field_of_mails_with_integer_value
 *
 * description       :  update a filed of mail
 * arguments : 
 *    account_id		 :  Specifies the account id. 
 *    mail_ids       :  mail id list to be changed
 *    mail_ids_count :  count of mail list
 *    field_name     :  specified field name
 *    value          :  specified value
 * return       : 
 */
INTERNAL_FUNC int emstorage_set_field_of_mails_with_integer_value(int account_id, int mail_ids[], int mail_ids_count, char *field_name, int value, int transaction, int *err_code);

/*
 * emstorage_change_mail_field
 *
 * description :  update partial mail data
 * arguments : 
 *    mail_id  :  mail id
 *    type  :  changing type
 *    mail  :  mail pointer
 * return  : 
 */
INTERNAL_FUNC int emstorage_change_mail_field(int mail_id, email_mail_change_type_t type, emstorage_mail_tbl_t *mail, int transaction, int *err_code);

/*
 * emstorage_increase_mail_id
 *
 * description :  increase unique mail id
 * arguments : 
 *    mail_id  :  pointer to store the unique id
 * return  : 
 */

/*
 * emstorage_change_mail
 *
 * description :  update mail
 * arguments : 
 *    mail_id  :  mail id to be changed
 *    mail  :  mail pointer
 * return  : 
 */
INTERNAL_FUNC int emstorage_change_mail(int mail_id, emstorage_mail_tbl_t *mail, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_clean_save_status(int save_status, int  *err_code);
INTERNAL_FUNC int emstorage_update_server_uid(char *old_server_uid, char *new_server_uid, int *err_code);
INTERNAL_FUNC int emstorage_modify_mailbox_of_mails(char *old_mailbox_name, char *new_mailbox_name, int transaction, int *err_code);

INTERNAL_FUNC int emstorage_increase_mail_id(int *mail_id, int transaction, int *err_code);

/*
 * emstorage_add_mail
 *
 * description :  add a mail to mail table
 * arguments : 
 *    mail  :  mail pointer to be inserted
 *   get_id :  must get uinque id in function
 * return  : 
 */
INTERNAL_FUNC int emstorage_add_mail(emstorage_mail_tbl_t *mail, int get_id, int transaction, int *err_code);

/*
 * emstorage_move_multiple_mails_on_db
 *
 * description :
 * arguments : 
 *    account_id  :  
 *   input_mailbox_id :
 *   mail_ids :
 *   number_of_mails :
 *   transaction :
 *   err_code :
 * return  : 
 */
INTERNAL_FUNC int emstorage_move_multiple_mails_on_db(int account_id, int input_mailbox_id, int mail_ids[], int number_of_mails, int transaction, int *err_code);

/*
 * emstorage_delete_mail
 *
 * description :  delete mail from mail table
 * arguments : 
 *    mail_id  :  mail id to be deleted. if 0, all mail will be deleted.
 *    from_server  :  delete mail on server.
 * return  : 
 */
INTERNAL_FUNC int emstorage_delete_mail(int mail_id, int from_server, int transaction, int *err_code);

/*
 * emstorage_delete_mail_by_account
 *
 * description :  delete mail from mail table by account id
 * arguments : 
 *    account_id  :  account id.
 * return  : 
 */
INTERNAL_FUNC int emstorage_delete_mail_by_account(int account_id, int transaction, int *err_code);

/*
 * emstorage_delete_mail
 *
 * description :  delete mail from mail table
 * arguments : 
 *    account_id  :  account id.
 *    mailbox  :  mail box
 * return  : 
 */
INTERNAL_FUNC int emstorage_delete_mail_by_mailbox(int account_id, char *mailbox, int transaction, int *err_code);

/*
 * emstorage_delete_multiple_mails
 *
 * description :  
 * arguments : 
 *    mail_ids  :
 *    number_of_mails  :
 *    transaction  :
 *    err_code  :
 * return  : 
 */
INTERNAL_FUNC int emstorage_delete_multiple_mails(int mail_ids[], int number_of_mails, int transaction, int *err_code);

/*
 * emstorage_free_mail
 *
 * description :  free memory
 * arguments : 
 *    mail_list  :  mail array
 *    count  :  the number of array element
 * return  : 
 */
INTERNAL_FUNC int emstorage_free_mail(emstorage_mail_tbl_t **mail_list, int count, int *err_code);

/*
 * emstorage_get_attachment
 *
 * description :  get attachment from attachment table
 * arguments : 
 *    mail_id  :  mail id
 *    no  :  attachment sequence
 *    attachment  :  double pointer to hold attachment data
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_attachment_count(int mail_id, int *count, int transaction, int *err_code);

/*
 * emstorage_get_attachment_list
 *
 * description :  get attachment list from attachment table
 * arguments : 
 *    input_mail_id           : mail id
 *    input_transaction       : transaction option
 *    attachment_list         : result attachment list
 *    output_attachment_count : result attachment count
 * return  : This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR__XXX) on failure.
 *    
 */
INTERNAL_FUNC int emstorage_get_attachment_list(int input_mail_id, int input_transaction, emstorage_attachment_tbl_t** output_attachment_list, int *output_attachment_count);


/*
 * emstorage_get_attachment
 *
 * description :  get attachment from attachment table
 * arguments : 
 *    attachment_id  :  attachment id
 *    attachment  :  double pointer to hold attachment data
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_attachment(int attachment_id, emstorage_attachment_tbl_t **attachment, int transaction, int *err_code);

/*
 * emstorage_get_attachment
 *
 * description :  get nth-attachment from attachment table
 * arguments : 
 *    mail_id  :  mail id
 *    nth  :  index of the desired attachment (min : 1)
 *    attachment  :  double pointer to hold attachment data
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_attachment_nth(int mail_id, int nth, emstorage_attachment_tbl_t **attachment, int transaction, int *err_code);

/*
 * emstorage_change_attachment_field
 *
 * description :  update partial mail attachment data
 * arguments : 
 *    mail_id  :  mail id
 *    type  :  changing type
 *    mail  :  mail pointer
 * return  : 
 */
INTERNAL_FUNC int emstorage_change_attachment_field(int mail_id, email_mail_change_type_t type, emstorage_attachment_tbl_t *attachment, int transaction, int *err_code);

/* Get new attachment id */
/*
 * emstorage_get_new_attachment_no
 *
 * description :  Get new attachment id
 * arguments : 
 *    attachment_no  :  attachment id pointer
 * return  : 
 */

INTERNAL_FUNC int emstorage_get_new_attachment_no(int *attachment_no, int *err_code);

/* insert attachment to mail attachment table */
/*
 * emstorage_add_attachment
 *
 * description :  insert a attachment to attachment table
 * arguments : 
 *    attachment  :  attachment pointer
 * return  : 
 */
INTERNAL_FUNC int emstorage_add_attachment(emstorage_attachment_tbl_t *attachment, int iscopy, int transaction, int *err_code);


INTERNAL_FUNC int emstorage_update_attachment(emstorage_attachment_tbl_t *attachment, int transaction, int *err_code);

/* delete a mail from mail table */
/*
 * emstorage_delete_attachment_on_db
 *
 * description :  delete attachment from attachment table
 * arguments : 
 *    attachment_id  :  attachment id
 * return  : 
 */
INTERNAL_FUNC int emstorage_delete_attachment_on_db(int attachment_id, int transaction, int *err_code);

INTERNAL_FUNC int emstorage_delete_all_attachments_of_mail(int mail_id, int transaction, int *err_code);

/*
 * emstorage_delete_attachment_all_on_db
 *
 * description :  delete attachment from mail table
 * arguments : 
 *    account_id  :  account id.
 *    mailbox  :  mail box
 * return  : 
 */
INTERNAL_FUNC int emstorage_delete_attachment_all_on_db(int account_id, char *mailbox, int transaction, int *err_code);

/*
 * emstorage_free_attachment
 *
 * description :  free memory
 * arguments : 
 *    mail_list  :  mail array
 *    count  :  the number of array element
 * return  : 
 */
INTERNAL_FUNC int emstorage_free_attachment(emstorage_attachment_tbl_t **attachment_list, int count, int *err_code);

INTERNAL_FUNC int emstorage_is_mailbox_full(int account_id, email_mailbox_t *mailbox, int *result, int *err_code);

INTERNAL_FUNC int emstorage_get_max_mail_count();

INTERNAL_FUNC int emstorage_mail_get_total_diskspace_usage(unsigned long *total_usage,  int transaction, int *err_code);


/**
 * begin a transaction.
 *
 * @param[in] d1	Reserved.
 * @param[in] d2	Reserved.
 * @remarks emstorage_commit_transaction or emstorage_commit_transaction must be called after this function.
 * @return This function returns 0 on success or error code on failure.
 */

INTERNAL_FUNC int emstorage_begin_transaction(void *d1, void *d2, int *err_code);

/**
 * commit a transaction.
 *
 * @param[in] d1	Reserved.
 * @param[in] d2	Reserved.
 * @remarks N/A
 * @return This function returns 0 on success or error code on failure.
 */
INTERNAL_FUNC int emstorage_commit_transaction(void *d1, void *d2, int *err_code);

/**
 * rollback db.
 *
 * @param[in] d1	Reserved.
 * @param[in] d2	Reserved.
 * @remarks N/A
 * @return This function returns 0 on success or error code on failure.
 */
INTERNAL_FUNC int emstorage_rollback_transaction(void *d1, void *d2, int *err_code);

/**
 * clear mail data from db.
 *
 * @param[in]  transaction	
 * @param[out] err_code	
 * @remarks N/A
 * @return This function returns 0 on success or error code on failure.
 */

INTERNAL_FUNC int emstorage_clear_mail_data(int transaction, int *err_code);


INTERNAL_FUNC char *emstorage_make_directory_path_from_file_path(char *file_name);

/*
 * emstorage_get_save_name
 *
 * description :  get file name for saving data
 * arguments : 
 *    account_id  :  account id
 *    mail_id  :  mail id
 *    atch_id  :  attachment id
 *    fname  :  file name
 *    name_buf  :  buffer to hold file name. (MAX : 512Bytes)
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_save_name(int account_id, int mail_id, int atch_id, char *fname, char *name_buf, int *err_code);

/*
 * emstorage_get_dele_name
 *
 * description :  get a name for deleting contents from file system.
 * arguments : 
 *    account_id  :  account id
 *    mail_id  :  mail id
 *    atch_id  :  attachment id
 *    fname  :  reserved
 *    name_buf  :  buffer to hold file name. (MAX : 512Bytes)
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_dele_name(int account_id, int mail_id, int atch_id, char *fname, char *name_buf, int *err_code);

/*
 * emstorage_create_dir
 *
 * description :  create directory
 * arguments : 
 *    name_buf  :  buffer to hold file name. (MAX : 512Bytes)
 *    no  :  attachment no.
 * return  : 
 */
INTERNAL_FUNC int emstorage_create_dir(int account_id, int mail_id, int atch_id, int *err_code);

/*
 * emstorage_copy_file
 *
 * description :  copy a attachment file
 * arguments : 
 *    src_file  :  source file
 *    dst_file  :  destination file
 * return  : 
 */
INTERNAL_FUNC int emstorage_copy_file(char *src_file, char *dst_file, int sync_file, int *err_code);

/*
 * emstorage_move_file
 *
 * description :  move a file
 * arguments : 
 *    src_file  :  source file
 *    dst_file  :  destination file
 * return  : 
 */
INTERNAL_FUNC int emstorage_move_file(char *src_file, char *dst_file, int sync_status, int *err_code);

/*
 * emstorage_move_file
 *
 * description :  delete a file
 * arguments : 
 *    src_file  :  file to be deleted
 * return  : 
 */
INTERNAL_FUNC int emstorage_delete_file(char *src_file, int *err_code);

/*
 * emstorage_delete_dir
 *
 * description :  delete a directory
 * arguments : 
 *    src_dir  :  directory to be deleted
 * return  : 
 */
INTERNAL_FUNC int emstorage_delete_dir(char *src_dir, int *err_code);



INTERNAL_FUNC void emstorage_flush_db_cache();
INTERNAL_FUNC int emstorage_test(int mail_id, int account_id, char *full_address_to, char *full_address_cc, char *full_address_bcc, int *err_code);

INTERNAL_FUNC int emstorage_get_sender_list(int account_id, const char *mailbox_name, int search_type, const char *search_value, email_sort_type_t sorting, email_sender_list_t** sender_list, int *sender_count,  int *err_code);
INTERNAL_FUNC int emstorage_free_sender_list(email_sender_list_t **sender_list, int count);

/* Handling Thread mail */
INTERNAL_FUNC int emstorage_get_thread_information(int thread_id, emstorage_mail_tbl_t **mail_table_data, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_get_thread_id_of_thread_mails(emstorage_mail_tbl_t *mail_table_data, int *thread_id, int *result_latest_mail_id_in_thread, int *thread_item_count);
INTERNAL_FUNC int emstorage_get_thread_id_by_mail_id(int mail_id, int *thread_id, int *err_code);
INTERNAL_FUNC int emstorage_update_latest_thread_mail(int account_id, int thread_id, int latest_mail_id, int thread_item_count, int transaction, int *err_code);

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

#define BULK_PARTIAL_BODY_DOWNLOAD_COUNT 10
enum
{
	ACCOUNT_IDX_MAIL_PARTIAL_BODY_ACTIVITY_TBL = 0, 
	MAIL_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL, 
	SERVER_MAIL_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL, 
	ACTIVITY_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL, 
	ACTIVITY_TYPE_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL, 
	MAILBOX_ID_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL, 
	MAILBOX_NAME_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL,
};


INTERNAL_FUNC int   emstorage_get_pbd_activity_data(int account_id, int input_mailbox_id, email_event_partial_body_thd** event_start, int *count, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_add_pbd_activity(email_event_partial_body_thd *local_activity, int *activity_id, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_get_pbd_mailbox_list(int account_id, int **mailbox_list, int *count, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_get_pbd_account_list(int **account_list, int *count, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_get_pbd_activity_count(int *activity_id_count, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_delete_full_pbd_activity_data(int account_id, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_delete_pbd_activity(int account_id, int mail_id, int activity_id, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_get_mailbox_pbd_activity_count(int account_id, int input_mailbox_id, int *activity_count, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_update_pbd_activity(char *old_server_uid, char *new_server_uid, char *mbox_name, int *err_code);
INTERNAL_FUNC int   emstorage_create_file(char *buf, size_t file_size, char *dst_file, int *err_code);

#endif  

INTERNAL_FUNC int   emstorage_free_address_info_list(email_address_info_list_t **address_info_list);

INTERNAL_FUNC void  emstorage_create_dir_if_delete();

#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__
INTERNAL_FUNC int emstorage_update_read_mail_uid_by_server_uid(char *old_server_uid, char *new_server_uid, char *mbox_name, int *err_code);

/**
 * @fn emstorage_get_id_set_from_mail_ids(int mail_ids[], int mail_id_count, email_id_set_t **server_uids, int *id_set_count, int *err_code);
 * Prepare an array of mail_id and corresponding server mail id.
 *
 *@author 					h.gahlaut@samsung.com
 * @param[in] mail_ids			Specifies the comma separated string of mail_ids. Maximaum size of string should be less than or equal to (QUERY_SIZE - 88)
 *							where 88 is the length of fixed keywords including ending null character in the QUERY to be formed
 * @param[out] idset			Returns the array of mail_id and corresponding server_mail_id sorted by server_mail_ids in ascending order
 * @param[out] id_set_count		Returns the no. of cells in idset array i.e. no. of sets of mail_ids and server_mail_ids
 * @param[out] err_code		Returns the error code.
 * @remarks 					An Example of Query to be exexuted in this API : 
 *							SELECT local_uid, s_uid from mail_read_mail_uid_tbl where local_uid in (12, 13, 56, 78);
 * @return This function returns true on success or false on failure.
 */

INTERNAL_FUNC int emstorage_get_id_set_from_mail_ids(char *mail_ids, email_id_set_t **idset, int *id_set_count, int *err_code);

#endif

/**
 * @fn emstorage_filter_mails_by_rule(int account_id, char dest_mailbox_name, email_rule_t *rule, int **filtered_mail_id_list, int *count_of_mails, int err_code)
 * Move mails by specified rule for spam filtering. 
 *
 * @author 								kyuho.jo@samsung.com
 * @param[in] account_id				Account id of the mails and the mailboxes.
 * @param[in] dest_mailbox_name			Mailbox name of spam mailbox.
 * @param[in] rule						Filtering rule.
 * @param[out] filtered_mail_id_list	Mail id list which are filtered by the rule.
 * @param[out] count_of_mails			Count of mails which are filtered by the rule.
 * @param[out] err_code					Returns the error code.

 * @remarks 									
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emstorage_filter_mails_by_rule(int account_id, int dest_mailbox_id, emstorage_rule_tbl_t *rule, int **filtered_mail_id_list, int *count_of_mails, int *err_code);

INTERNAL_FUNC int emstorage_add_meeting_request(int account_id, int input_mailbox_id, email_meeting_request_t *meeting_req, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_get_meeting_request(int mail_id, email_meeting_request_t **meeting_req, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_update_meeting_request(email_meeting_request_t *meeting_req, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_delete_meeting_request(int account_id, int mail_id, int input_mailbox_id, int transaction, int *err_code);
INTERNAL_FUNC void emstorage_free_meeting_request(email_meeting_request_t *meeting_req);

INTERNAL_FUNC int emstorage_write_conditional_clause_for_getting_mail_list(email_list_filter_t *input_filter_list, int input_filter_count, email_list_sorting_rule_t *input_sorting_rule_list, int input_sorting_rule_count, int input_start_index, int input_limit_count, char **output_conditional_clause);
INTERNAL_FUNC int emstorage_free_list_filter(email_list_filter_t **input_filter_list, int input_filter_count);

#ifdef __FEATURE_LOCAL_ACTIVITY__
/*
*emstorage_get_next_activity_id
*
*description :  get an activity id for a new activity
*/
INTERNAL_FUNC int emstorage_get_next_activity_id(int *activity_id, int *err_code);

 /*
 *emstorage_get_activity_id_list
 *description  :  get the list of activity ids
 *arguments  : 
 *return  : 
 *
 */
INTERNAL_FUNC int emstorage_get_activity_id_list(int account_id, int **activity_id_list, int *activity_count, int lowest_activity_type, int highest_activity_type, int transaction, int*err_code);
 /*
 * emstorage_add_activity
 *
 * description :  add an activity to activity table
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_add_activity(emstorage_activity_tbl_t *local_activity, int transaction, int *err_code);

/**
  *	emstorage_get_activity - Get the Local activity from the Mail activity table
  *
  */
INTERNAL_FUNC int emstorage_get_activity(int account_id, int activity_id, emstorage_activity_tbl_t **activity_list, int *select_num, int transaction, int *err_code);

/**
 * emstorage_delete_local_activity - Deletes the Local acitivity Generated based on activity_type
 * or based on server mail id
 *
 */
INTERNAL_FUNC int emstorage_delete_local_activity(emstorage_activity_tbl_t *local_activity, int transaction, int *err_code);

/**
 * emstorage_free_local_activity - Free the allocated Activity data
 * 
 *
 */
INTERNAL_FUNC int emstorage_free_local_activity(emstorage_activity_tbl_t **local_activity_list, int count, int *err_code);

/**
 * emstorage_free_activity_id_list - Free the allocated Activity List data
 * 
 *
 */
INTERNAL_FUNC int emstorage_free_activity_id_list(int *activity_id_list, int *error_code);

#endif

INTERNAL_FUNC int emstorage_add_certificate(emstorage_certificate_tbl_t *certificate, int transaction, int *err_code);

INTERNAL_FUNC int emstorage_free_certificate(emstorage_certificate_tbl_t **certificate_list, int count, int *err_code);

INTERNAL_FUNC int emstorage_get_certificate_list(int *select_num, emstorage_certificate_tbl_t **certificate_list, int transaction, int with_password, int *err_code);

INTERNAL_FUNC int emstorage_get_certificate_by_email_address(char *email_address, emstorage_certificate_tbl_t **certificate, int transaction, int with_password, int *err_code);

INTERNAL_FUNC int emstorage_get_certificate_by_index(int index, emstorage_certificate_tbl_t **certificate, int transaction, int with_password, int *err_code);

INTERNAL_FUNC int emstorage_delete_certificate(int index, int transaction, int *err_code);

/* task begin */
INTERNAL_FUNC int emstorage_add_task(email_task_type_t input_task_type, email_task_priority_t input_task_priority, char *input_task_parameter, int input_task_parameter_length, int input_transaction, int *output_task_id);

INTERNAL_FUNC int emstorage_delete_task(int task_id, int transaction);

INTERNAL_FUNC int emstorage_update_task_status(int task_id, email_task_status_type_t task_status, int transaction);

INTERNAL_FUNC int emstorage_query_task(const char *input_conditional_clause, const char *input_ordering_clause, email_task_t **output_task_list, int *output_task_count);
/* task end*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __EMAIL_STORAGE_H__ */
/* EOF */
