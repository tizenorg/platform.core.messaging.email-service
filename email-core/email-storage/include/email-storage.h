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
#include "email-internal-types.h"


#define MAILTEMP            "tmp"
#define FIRST_ACCOUNT_ID    1


#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__
#define QUERY_SIZE          8192
#define MAX_INTEGER_LENGTH  5  /*  32767 -> 5 bytes */

typedef struct 
{
	int mail_id;
	unsigned long server_mail_id;
} emf_id_set_t;

#endif /* __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__ */


typedef struct 
{
	int            account_bind_type;
	char          *account_name;
	int            receiving_server_type;
	char          *receiving_server_addr;
	char          *email_addr;
	char          *user_name;
	char          *password;
	int            retrieval_mode;
	int            port_num;
	int            use_security;
	int            sending_server_type;
	char          *sending_server_addr;
	int            sending_port_num;
	int            sending_auth;
	int            sending_security;
	char          *sending_user;
	char          *sending_password;
	char          *display_name;
	char          *reply_to_addr;
	char          *return_addr;
	int            account_id;
	int            keep_on_server;
	int            flag1;                     /*  Specifies the downloading option 0 is subject only, 1 is text body, 2 is normal. */
	int            flag2;
	int            pop_before_smtp;           /*  POP before SMTP Authentication   */
	int            apop;                      /*  APOP authentication  */
	char          *logo_icon_path;            /*  Account logo icon  */
	int            preset_account;            /*  Preset account or not  */
	emf_option_t   options;                   /*  Specifies the Sending options  */
	int            target_storage;            /*  Specifies the targetStorage. 0 is phone, 1 is MMC  */
	int            check_interval;            /*  Specifies the check interval  */
	int            my_account_id;             /*  Specifies accout id of my account */
	int            index_color;               /*  index color in RGB */
	int            sync_status;               /*  Sync Status */
} emstorage_account_tbl_t;

typedef struct 
{
	int   account_id;      /*  MUST BE '0'  :  currently, only global rule supported. */
	int   rule_id;
	int   type;            /*  from/subject/body  */
	char *value;
	int   action_type;     /*  move/block/delete  */
	char *dest_mailbox;    /*  destination mailbox  */
	int   flag1;           /*  for rule ON/OFF */
	int   flag2;           /*  For rule type INCLUDE/Exactly SAME AS */
} emstorage_rule_tbl_t;

/* mail_box_tbl table entity */
typedef struct 
{
	int                  mailbox_id;
	int                  account_id;
	int                  local_yn;
	char                *mailbox_name;
	emf_mailbox_type_e   mailbox_type;
	char                *alias;
	int                  sync_with_server_yn;      /*  whether mailbox is a sync IMAP mailbox */
	int                  modifiable_yn;              /*  whether mailbox is able to be deleted/modified */
	int                  unread_count;              /*  Removed. 16-Dec-2010, count unread mails at the moment it is required. not store in the DB */
	int                  total_mail_count_on_local;  /*  Specifies the total number of mails in the mailbox in the local DB. count unread mails at the moment it is required. not store in the DB */
	int                  total_mail_count_on_server;  /*  Specifies the total number of mails in the mailbox in the mail server */
	int                  has_archived_mails;
	int                  mail_slot_size;
} emstorage_mailbox_tbl_t;

/* mail_read_uid_tbl table entity */
typedef struct 
{
	int     account_id;
	char   *local_mbox;
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
	int        mail_id;
	int        account_id;
	int        mailbox_id;         /* For internal use. Currently, account_id and mailbox name is primary key of the table. */
	char      *mailbox_name;
	int        mailbox_type;
	char      *subject;
	time_t     date_time;
	int        server_mail_status;
	char      *server_mailbox_name;
	char      *server_mail_id;
	char      *message_id;
	char      *full_address_from;
	char      *full_address_reply;
	char      *full_address_to;
	char      *full_address_cc;
	char      *full_address_bcc;
	char      *full_address_return;
	char      *email_address_sender;
	char      *email_address_recipient;
	char      *alias_sender;
	char      *alias_recipient;
	int        body_download_status;
	char      *file_path_plain;
	char      *file_path_html;
	int        mail_size;
	char       flags_seen_field;
	char       flags_deleted_field;
	char       flags_flagged_field;
	char       flags_answered_field;
	char       flags_recent_field;
	char       flags_draft_field;
	char       flags_forwarded_field;
	int        DRM_status;
	int        priority;
	int        save_status;
	int        lock_status;
	int        report_status;
	int        attachment_count;
	int        inline_content_count;
	int        thread_id;
	int        thread_item_count;
	char      *preview_text;
	int        meeting_request_status;
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
	char *mailbox_name;
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
	EMF_CREATE_DB_NORMAL = 0, 
	EMF_CREATE_DB_CHECK, 
} emstorage_create_db_type_t;

typedef enum {
	SET_TYPE_SET        = 1,
	SET_TYPE_UNION      = 2,
	SET_TYPE_MINUS      = 3, 
	SET_TYPE_INTERSECT  = 4 /* Not supported */
} emstorage_set_type_t;


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
INTERNAL_FUNC int emstorage_check_duplicated_account(emf_account_t *account, int transaction, int *err_code);


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
/* sowmya.kr, 281209 Adding signature to options in emf_account_t changes */

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
 *    set_operator  :  set operater. refer emstorage_set_type_t
 *    sync_status : sync status value
 * return  : 
 */

INTERNAL_FUNC int emstorage_update_sync_status_of_account(int account_id, emstorage_set_type_t set_operator, int sync_status, int transaction, int *err_code);

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
INTERNAL_FUNC int emstorage_get_mailbox(int account_id, int local_yn, email_mailbox_sort_type_t sort_type, int *select_num, emstorage_mailbox_tbl_t **mailbox_list,  int transaction, int *err_code);

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
INTERNAL_FUNC int emstorage_get_mailbox_ex(int account_id, int local_yn, int with_count, int *select_num, emstorage_mailbox_tbl_t **mailbox_list, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_get_mailbox_by_mailbox_type(int account_id, emf_mailbox_type_e mailbox_type, emstorage_mailbox_tbl_t **mailbox, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_get_mailboxname_by_mailbox_type(int account_id, emf_mailbox_type_e mailbox_type, char **mailbox, int transaction, int *err_code);

INTERNAL_FUNC int emstorage_update_mailbox_modifiable_yn(int account_id, int local_yn, char *mailbox_name, int modifiable_yn, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_update_mailbox_total_count(int account_id, char *mailbox_name, int total_count_on_server, int transaction, int *err_code);


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
INTERNAL_FUNC int emstorage_update_mailbox(int account_id, int local_yn, char *mailbox_name, emstorage_mailbox_tbl_t *mailbox, int transaction, int *err_code);

/*
 * emstorage_update_mailbox_type
 *
 * description :  change a mailbox from mailbox tbl
 * arguments : 
 * return  : 
 */
INTERNAL_FUNC int emstorage_update_mailbox_type(int account_id, int local_yn, char *mailbox_name, emstorage_mailbox_tbl_t *mailbox, int transaction, int *err_code);

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
INTERNAL_FUNC int emstorage_delete_mailbox(int account_id, int local_yn, char *mailbox_name, int transaction, int *err_code);

INTERNAL_FUNC int emstorage_rename_mailbox(int account_id, char *old_mailbox_name, char *new_mailbox_name, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_get_overflowed_mail_id_list(int account_id, char *mailbox_name, int mail_slot_size, int **mail_id_list, int *mail_id_count, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_set_mail_slot_size(int account_id, char *mailbox_name, int new_slot_size, int transaction, int *err_code);

INTERNAL_FUNC int emstorage_set_all_mailbox_modifiable_yn(int account_id, int modifiable_yn, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_get_mailbox_by_modifiable_yn(int account_id, int modifiable_yn, int *select_num, emstorage_mailbox_tbl_t **mailbox_list, int transaction, int *err_code);

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
 *    local_mbox  :  local mailbox name
 *    local_uid  :  mail uid of local mailbox
 *    mailbox_name  :  server mailbox name
 *    uid  :  mail uid string of server mail
 *    read_mail_uid  :  variable to hold read mail uid information
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_downloaded_list(int account_id, char *local_mbox, emstorage_read_mail_uid_tbl_t **read_mail_uid, int *count, int transaction, int *err_code);

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
INTERNAL_FUNC int emstorage_change_read_mail_uid(int account_id, char *local_mbox, int local_uid, char *mailbox_name, char *uid, 
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


INTERNAL_FUNC int emstorage_filter_mails_by_rule(int account_id, char *dest_mailbox_name, emstorage_rule_tbl_t *rule, int **filtered_mail_id_list, int *count_of_mails, int *err_code);

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
 * emstorage_query_mail_list
 *
 * description :  query mail list information
 */
INTERNAL_FUNC int emstorage_query_mail_list(const char *conditional_clause, int transaction, emf_mail_list_item_t** result_mail_list,  int *result_count,  int *err_code);

/*
 * emstorage_query_mail_tbl
 *
 * description :  query mail table information
 */
INTERNAL_FUNC int emstorage_query_mail_tbl(const char *conditional_clause, int transaction, emstorage_mail_tbl_t** result_mail_tbl, int *result_count, int *err_code);

/*
 * emstorage_get_mail_list
 *
 * description :  search mail list information
 */
INTERNAL_FUNC int emstorage_get_mail_list(int account_id, const char *mailbox_name, emf_email_address_list_t* addr_list, int thread_id, int start_index, int limit_count, int search_type, const char *search_value, emf_sort_type_t sorting, int transaction, emf_mail_list_item_t** mail_list,  int *result_count,  int *err_code);
/*
 * emstorage_get_mails
 *
 * description :  search mail list information
 */
INTERNAL_FUNC int emstorage_get_mails(int account_id, const char *mailbox_name, emf_email_address_list_t* addr_list, int thread_id, int start_index, int limit_count, emf_sort_type_t sorting,  int transaction, emstorage_mail_tbl_t** mail_list, int *result_count, int *err_code);
INTERNAL_FUNC int emstorage_get_searched_mail_list(int account_id, const char *mailbox_name, int thread_id, int search_type, const char *search_value, int start_index, int limit_count, emf_sort_type_t sorting, int transaction, emf_mail_list_item_t **mail_list,  int *result_count,  int *err_code);
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
INTERNAL_FUNC int emstorage_change_mail_field(int mail_id, emf_mail_change_type_t type, emstorage_mail_tbl_t *mail, int transaction, int *err_code);

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
 * emstorage_move_multiple_mails
 *
 * description :
 * arguments : 
 *    account_id  :  
 *   target_mailbox_name :
 *   mail_ids :
 *   number_of_mails :
 *   transaction :
 *   err_code :
 * return  : 
 */
INTERNAL_FUNC int emstorage_move_multiple_mails(int account_id, char *target_mailbox_name, int mail_ids[], int number_of_mails, int transaction, int *err_code);

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
 *    mbox  :  mail box
 * return  : 
 */
INTERNAL_FUNC int emstorage_delete_mail_by_mailbox(int account_id, char *mbox, int transaction, int *err_code);

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
 * return  : This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR__XXX) on failure.
 *    
 */
INTERNAL_FUNC int emstorage_get_attachment_list(int input_mail_id, int input_transaction, emstorage_attachment_tbl_t** output_attachment_list, int *output_attachment_count);


/*
 * emstorage_get_attachment
 *
 * description :  get attachment from attachment table
 * arguments : 
 *    mail_id  :  mail id
 *    no  :  attachment id
 *    attachment  :  double pointer to hold attachment data
 * return  : 
 */
INTERNAL_FUNC int emstorage_get_attachment(int mail_id, int no, emstorage_attachment_tbl_t **attachment, int transaction, int *err_code);

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
INTERNAL_FUNC int emstorage_change_attachment_field(int mail_id, emf_mail_change_type_t type, emstorage_attachment_tbl_t *attachment, int transaction, int *err_code);

/*
 * emstorage_change_attachment_mbox
 *
 * description :  update mailbox from attahcment table
 * arguments : 
 *    account_id  :  account id
 *    old_mailbox_name  :  old mail box
 *    new_mailbox_name  :  new mail box
 * return  : 
 */
INTERNAL_FUNC int emstorage_change_attachment_mbox(int account_id, char *old_mailbox_name, char *new_mailbox_name, int transaction, int *err_code);

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
 *    mail_id  :  mail id to contain attachment
 *    no  :  attachment sequence number
 * return  : 
 */
INTERNAL_FUNC int emstorage_delete_attachment_on_db(int mail_id, int no, int transaction, int *err_code);

/*
 * emstorage_delete_attachment_all_on_db
 *
 * description :  delete attachment from mail table
 * arguments : 
 *    account_id  :  account id.
 *    mbox  :  mail box
 * return  : 
 */
INTERNAL_FUNC int emstorage_delete_attachment_all_on_db(int account_id, char *mbox, int transaction, int *err_code);

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

/*  emstorage_get_mail_count_with_draft_flag - Send number of mails in a mailbox with draft flag enabled */
INTERNAL_FUNC int emstorage_get_mail_count_with_draft_flag(int account_id, const char *mailbox, int *total, int transaction, int *err_code);

/*  emstorage_get_mail_count_on_sending - Send number of mails being sent in a mailbox (status == EMF_MAIL_STATUS_SENDING) */
INTERNAL_FUNC int emstorage_get_mail_count_on_sending(int account_id, const char *mailbox, int *total, int transaction, int * err_code);

INTERNAL_FUNC int emstorage_is_mailbox_full(int account_id, emf_mailbox_t *mailbox, int *result, int *err_code);

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
/**
 * emstorage_notify_storage_event - Notification for storage related operations
 * 
 */
INTERNAL_FUNC int emstorage_notify_storage_event(emf_noti_on_storage_event event_type, int data1, int data2 , char *data3, int data4);

/**
 * emstorage_notify_network_event - Notification for network related operations
 * 
 */
INTERNAL_FUNC int emstorage_notify_network_event(emf_noti_on_network_event event_type, int data1, char *data2, int data3, int data4);
INTERNAL_FUNC int emstorage_get_sender_list(int account_id, const char *mailbox_name, int search_type, const char *search_value, emf_sort_type_t sorting, emf_sender_list_t** sender_list, int *sender_count,  int *err_code);
INTERNAL_FUNC int emstorage_free_sender_list(emf_sender_list_t **sender_list, int count);

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
	MAILBOX_NAME_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL, 
};


INTERNAL_FUNC int   emstorage_get_pbd_activity_data(int account_id, char *mailbox_name, emf_event_partial_body_thd** event_start, int *count, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_add_pbd_activity(emf_event_partial_body_thd *local_activity, int *activity_id, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_get_pbd_mailbox_list(int account_id, char ***mailbox_list, int *count, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_get_pbd_account_list(int **account_list, int *count, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_get_pbd_activity_count(int *activity_id_count, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_delete_full_pbd_activity_data(int account_id, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_delete_pbd_activity(int account_id, int mail_id, int activity_id, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_get_mailbox_pbd_activity_count(int account_id, char *mailbox_name, int *activity_count, int transaction, int *err_code);
INTERNAL_FUNC int   emstorage_update_pbd_activity(char *old_server_uid, char *new_server_uid, char *mbox_name, int *err_code);
INTERNAL_FUNC int   emstorage_create_file(char *buf, size_t file_size, char *dst_file, int *err_code);

#endif  

INTERNAL_FUNC int   emstorage_free_address_info_list(emf_address_info_list_t **address_info_list);


INTERNAL_FUNC void  emstorage_create_dir_if_delete();

#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__
INTERNAL_FUNC int emstorage_update_read_mail_uid_by_server_uid(char *old_server_uid, char *new_server_uid, char *mbox_name, int *err_code);

/**
 * @fn emstorage_get_id_set_from_mail_ids(int mail_ids[], int mail_id_count, emf_id_set_t **server_uids, int *id_set_count, int *err_code);
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

INTERNAL_FUNC int emstorage_get_id_set_from_mail_ids(char *mail_ids, emf_id_set_t **idset, int *id_set_count, int *err_code);

#endif

/**
 * @fn emstorage_filter_mails_by_rule(int account_id, char dest_mailbox_name, emf_rule_t *rule, int **filtered_mail_id_list, int *count_of_mails, int err_code)
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

INTERNAL_FUNC int emstorage_add_meeting_request(int account_id, char *mailbox_name, emf_meeting_request_t *meeting_req, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_get_meeting_request(int mail_id, emf_meeting_request_t **meeting_req, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_update_meeting_request(emf_meeting_request_t *meeting_req, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_change_meeting_request_field(int account_id, char *mailbox_name, emf_mail_change_type_t type, emf_meeting_request_t *meeting_req, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_change_meeting_request_mailbox(int account_id, char *old_mailbox_name, char *new_mailbox_name, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_delete_meeting_request(int account_id, int mail_id, char *mailbox_name, int transaction, int *err_code);
INTERNAL_FUNC int emstorage_free_meeting_request(emf_meeting_request_t **meeting_req, int count, int *err_code);

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __EMAIL_STORAGE_H__ */
/* EOF */
