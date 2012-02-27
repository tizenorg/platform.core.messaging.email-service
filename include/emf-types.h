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


#ifndef __EMF_TYPES_H__
#define __EMF_TYPES_H__

/**
* @defgroup EMAIL_FRAMEWORK Email Service
* @{
*/

/**
* @ingroup EMAIL_FRAMEWORK
* @defgroup EMAIL_TYPES Email Types
* @{
*/
/**
 * This file defines structures and enums of Email Framework.
 * @file	emf-types.h
 * @author	Kyu-ho Jo(kyuho.jo@samsung.com)
 * @author	Choongho Lee(ch715.lee@samsung.com)
 * @version	0.1
 * @brief	This file is the header file of Email Framework library.
 */


#ifdef __cplusplus
extern "C" 
{
#endif /* __cplusplus */

#include <glib.h>
#include <stdbool.h>


/*****************************************************************************/
/*  Macros                                                                   */
/*****************************************************************************/

#define MAILBOX_NAME_LENGTH             256
#define MAX_EMAIL_ADDRESS_LENGTH        320                                    /*  64(user name) + 1(@) + 255(host name */
#define MAX_DATETIME_STRING_LENGTH      20
#define MAX_PREVIEW_TEXT_LENGTH         50                                     /* should be 512 */
#define STRING_LENGTH_FOR_DISPLAY       100
#define MEETING_REQ_OBJECT_ID_LENGTH    128                                    /* should be 256 */

#define ALL_ACCOUNT                     0
#define NEW_ACCOUNT_ID                  0xFFFFFFFE
#define ALL_MAIL                        -1

#define EMF_SEARCH_FILTER_NONE          0x00
#define EMF_SEARCH_FILTER_SUBJECT       0x01
#define EMF_SEARCH_FILTER_SENDER        0x02
#define EMF_SEARCH_FILTER_RECIPIENT     0x04
#define EMF_SEARCH_FILTER_ALL           0x07                                   /*  EMF_SEARCH_FILTER_SUBJECT + EMF_SEARCH_FILTER_SENDER + EMF_SEARCH_FILTER_RECIPIEN */

#define EMF_SUCCESS                     0                                      /*  we need to modify the success return valu */

#define EMF_ACC_GET_OPT_DEFAULT         0x01                                   /**< Default values without account name */
#define EMF_ACC_GET_OPT_ACCOUNT_NAME    0x02                                   /**< Account name */
#define EMF_ACC_GET_OPT_PASSWORD        0x04                                   /**< With password */
#define EMF_ACC_GET_OPT_OPTIONS         0x08                                   /**< Account options : emf_option_t */
#define EMF_ACC_GET_OPT_FULL_DATA       0xFF                                   /**< With all data of account */

#define GET_FULL_DATA                   0x00
#define GET_FULL_DATA_WITHOUT_PASSWORD  (EMF_ACC_GET_OPT_DEFAULT | EMF_ACC_GET_OPT_ACCOUNT_NAME | EMF_ACC_GET_OPT_OPTIONS )
#define WITHOUT_OPTION                  (EMF_ACC_GET_OPT_DEFAULT | EMF_ACC_GET_OPT_ACCOUNT_NAME )
#define ONLY_OPTION                     (EMF_ACC_GET_OPT_OPTIONS)

#define THREAD_TYPE_RECEIVING           0                                      /**< for function 'email_activate_pdp' */
#define THREAD_TYPE_SENDING             1                                      /**< for function 'email_activate_pdp' */

#define EMF_IMAP_PORT                   143                                    /**< Specifies the default IMAP port.*/
#define EMF_POP3_PORT                   110                                    /**< Specifies the default POP3 port.*/
#define EMF_SMTP_PORT                   25                                     /**< Specifies the default SMTP port.*/
#define EMF_IMAPS_PORT                  993                                    /**< Specifies the default IMAP SSL port.*/
#define EMF_POP3S_PORT                  995                                    /**< Specifies the default POP3 SSL port.*/
#define EMF_SMTPS_PORT                  465                                    /**< Specifies the default SMTP SSL port.*/
#define EMF_ACCOUNT_MAX                 10                                     /**< Specifies the MAX account.*/

#define EMF_INBOX_NAME                  "INBOX"                                /**< Specifies the name of inbox.*/
#define EMF_DRAFTBOX_NAME               "DRAFTBOX"                             /**< Specifies the name of draftbox.*/
#define EMF_OUTBOX_NAME                 "OUTBOX"                               /**< Specifies the name of outbox.*/
#define EMF_SENTBOX_NAME                "SENTBOX"                              /**< Specifies the name of sentbox.*/
#define EMF_TRASH_NAME                  "TRASH"                                /**< Specifies the name of trash.*/
#define EMF_SPAMBOX_NAME                "SPAMBOX"                              /**< Specifies the name of spambox.*/

#define EMF_INBOX_DISPLAY_NAME          "Inbox"                                /**< Specifies the display name of inbox.*/
#define EMF_DRAFTBOX_DISPLAY_NAME       "Draftbox"                             /**< Specifies the display name of draftbox.*/
#define EMF_OUTBOX_DISPLAY_NAME         "Outbox"                               /**< Specifies the display name of outbox.*/
#define EMF_SENTBOX_DISPLAY_NAME        "Sentbox"                              /**< Specifies the display name of sentbox.*/
#define EMF_TRASH_DISPLAY_NAME          "Trash"                                /**< Specifies the display name of sentbox.*/
#define EMF_SPAMBOX_DISPLAY_NAME        "Spambox"                              /**< Specifies the display name of spambox.*/

#define EMF_SEARCH_RESULT_MAILBOX_NAME  "_`S1!E2@A3#R4$C5^H6&R7*E8(S9)U0-L=T_" /**< Specifies the name of search mailbox.*/

#define FAILURE                         -1
#define SUCCESS                         0

#define NATIVE_EMAIL_APPLICATION_PKG    "org.tizen.email"
#define DAEMON_EXECUTABLE_PATH          "/usr/bin/email-service"

#ifndef DEPRECATED 
#define DEPRECATED                      __attribute__((deprecated))
#endif

#ifndef EXPORT_API
#define EXPORT_API                      __attribute__((visibility("default")))
#endif

/* __LOCAL_ACTIVITY__ supported
#define BULK_OPERATION_COUNT            50
#define ALL_ACTIVITIES                  0
*/

/*****************************************************************************/
/*  Enumerations                                                             */
/*****************************************************************************/
enum
{
	EMF_DELETE_LOCALLY = 0,                      /**< Specifies Mail Delete local only */
	EMF_DELETE_LOCAL_AND_SERVER,                 /**< Specifies Mail Delete local & server */
	EMF_DELETE_FOR_SEND_THREAD,                  /**< Created to check which activity to delete in send thread */
};

typedef enum
{
	NOTI_MAIL_ADD                       = 10000,
	NOTI_MAIL_DELETE                    = 10001,
	NOTI_MAIL_DELETE_ALL                = 10002,
	NOTI_MAIL_DELETE_WITH_ACCOUNT       = 10003,
	NOTI_MAIL_DELETE_FAIL               = 10007,
	NOTI_MAIL_DELETE_FINISH             = 10008,

	NOTI_MAIL_UPDATE                    = 10004,
	NOTI_MAIL_FIELD_UPDATE              = 10020,	

	NOTI_MAIL_MOVE                      = 10005,
	NOTI_MAIL_MOVE_FAIL                 = 10010,
	NOTI_MAIL_MOVE_FINISH               = 10006,

	NOTI_THREAD_MOVE                    = 11000,
	NOTI_THREAD_DELETE                  = 11001,
	NOTI_THREAD_MODIFY_SEEN_FLAG        = 11002,

	NOTI_ACCOUNT_ADD                    = 20000,
	NOTI_ACCOUNT_DELETE                 = 20001,
	NOTI_ACCOUNT_DELETE_FAIL            = 20003, 
	NOTI_ACCOUNT_UPDATE                 = 20002,
	NOTI_ACCOUNT_UPDATE_SYNC_STATUS     = 20010,
	

	NOTI_MAILBOX_ADD                    = 40000,
	NOTI_MAILBOX_DELETE,
	NOTI_MAILBOX_UPDATE,
	/* To be added more */
}emf_noti_on_storage_event;

typedef enum
{
	NOTI_SEND_START                          = 1002,
	NOTI_SEND_FINISH                         = 1004,
	NOTI_SEND_FAIL                           = 1005, 
	NOTI_SEND_CANCEL                         = 1003,

	NOTI_DOWNLOAD_START                      = 2000,
	NOTI_DOWNLOAD_FINISH,
	NOTI_DOWNLOAD_FAIL,
	NOTI_DOWNLOAD_CANCEL                     = 2004,
	NOTI_DOWNLOAD_NEW_MAIL                   = 2003, 

	NOTI_DOWNLOAD_BODY_START                 = 3000,
	NOTI_DOWNLOAD_BODY_FINISH                = 3002,
	NOTI_DOWNLOAD_BODY_FAIL                  = 3004,
	NOTI_DOWNLOAD_BODY_CANCEL                = 3003,
	NOTI_DOWNLOAD_MULTIPART_BODY             = 3001,

	NOTI_DOWNLOAD_ATTACH_START               = 4000,
	NOTI_DOWNLOAD_ATTACH_FINISH,
	NOTI_DOWNLOAD_ATTACH_FAIL,
	NOTI_DOWNLOAD_ATTACH_CANCEL,

	NOTI_MAIL_DELETE_ON_SERVER_FAIL          = 5000,         
	NOTI_MAIL_MOVE_ON_SERVER_FAIL,

	NOTI_VALIDATE_ACCOUNT_FINISH             = 7000, 
	NOTI_VALIDATE_ACCOUNT_FAIL,
	NOTI_VALIDATE_ACCOUNT_CANCEL,

	NOTI_VALIDATE_AND_CREATE_ACCOUNT_FINISH  = 8000,
	NOTI_VALIDATE_AND_CREATE_ACCOUNT_FAIL,
	NOTI_VALIDATE_AND_CREATE_ACCOUNT_CANCEL,

	NOTI_VALIDATE_AND_UPDATE_ACCOUNT_FINISH  = 9000,
	NOTI_VALIDATE_AND_UPDATE_ACCOUNT_FAIL,
	NOTI_VALIDATE_AND_UPDATE_ACCOUNT_CANCEL,

	/* To be added more */
}emf_noti_on_network_event;



/** 
 * This enumeration specifies the core engine type.
 */
typedef enum      
{
	EMF_ENGINE_TYPE_EM_CORE        = 1,          /**< Specifies the Callia mail core engine.*/
} emf_engine_type_t;

/** 
 * This enumeration specifies the mail type of account.
 */
typedef enum
{
	EMF_BIND_TYPE_DISABLE          = 0,          /**< Specifies the bind type for Disabled account.*/
	EMF_BIND_TYPE_EM_CORE          = 1,          /**< Specifies the bind type for Callia.*/
} emf_account_bind_t;

/** 
 * This enumeration specifies the server type of account.
 */
typedef enum
{
	EMF_SERVER_TYPE_POP3           = 1,          /**< Specifies the POP3 Server.*/
	EMF_SERVER_TYPE_IMAP4,                       /**< Specifies the IMAP4 Server.*/
	EMF_SERVER_TYPE_SMTP,                        /**< Specifies the SMTP Server.*/
	EMF_SERVER_TYPE_NONE,                        /**< Specifies the Local.*/
	EMF_SERVER_TYPE_ACTIVE_SYNC,                 /** < Specifies the Active Sync.  */
} emf_account_server_t;

/** 
 * This enumeration specifies the mode of retrieval.
 */
typedef enum
{
	EMF_IMAP4_RETRIEVAL_MODE_NEW   = 0,          /**< Specifies the retrieval mode for new email.*/
	EMF_IMAP4_RETRIEVAL_MODE_ALL,                /**< Specifies the retrieval mode for all email.*/
} emf_imap4_retrieval_mode_t;

/** 
 * This enumeration specifies the filtering type.
 */
typedef enum
{
	EMF_FILTER_FROM                = 1,          /**< Specifies the filtering of sender.*/
	EMF_FILTER_SUBJECT,                          /**< Specifies the filtering of email subject.*/
	EMF_FILTER_BODY,                             /** < Specifies the filterinf of email body.*/
} emf_rule_type_t;


/** 
 * This enumeration specifies the rules for filtering type.
 */
typedef enum
{
	RULE_TYPE_INCLUDES             = 1,          /**< Specifies the filtering rule for includes.*/
	RULE_TYPE_EXACTLY,                           /**< Specifies the filtering rule for Exactly same as.*/
} emf_filtering_type_t;


/** 
 * This enumeration specifies the action for filtering type.
 */
typedef enum
{
	EMF_FILTER_MOVE                = 1,          /**< Specifies the move of email.*/
	EMF_FILTER_BLOCK               = 2,          /**< Specifies the block of email.*/
	EMF_FILTER_DELETE              = 3,          /**< Specifies delete  email.*/
} emf_rule_action_t;

/** 
 * This enumeration specifies the email status. 
 */
typedef enum
{
	EMF_MAIL_STATUS_NONE           = 0,          /**< The Mail is in No Operation state */
	EMF_MAIL_STATUS_RECEIVED,                    /**< The mail is a received mail.*/
	EMF_MAIL_STATUS_SENT,                        /**< The mail is a sent mail.*/
	EMF_MAIL_STATUS_SAVED,                       /**< The mail is a saved mail.*/
	EMF_MAIL_STATUS_SAVED_OFFLINE,               /**< The mail is a saved mail in offline-mode.*/
	EMF_MAIL_STATUS_SENDING,                     /**< The mail is being sent.*/
	EMF_MAIL_STATUS_SEND_FAILURE,                /**< The mail is a mail to been failed to send.*/
	EMF_MAIL_STATUS_SEND_CANCELED,               /**< The mail is a cancelled mail.*/ 
	EMF_MAIL_STATUS_SEND_WAIT,                   /**< The mail is a mail to be send .*/
} emf_mail_status_t;

/** 
 * This enumeration specifies the email priority. 
 */
typedef enum
{
	EMF_MAIL_PRIORITY_HIGH         = 1,          /**< The priority is high.*/
	EMF_MAIL_PRIORITY_NORMAL       = 3,          /**< The priority is normal.*/
	EMF_MAIL_PRIORITY_LOW          = 5,          /**< The priority is low.*/
} emf_mail_priority_t;

/** 
 * This enumeration specifies the email status. 
 */
typedef enum
{
	EMF_MAIL_REPORT_NONE           = 0,          /**< The mail isn't report mail.*/
	EMF_MAIL_REPORT_DSN,                         /**< The mail is a delivery-status report mail.*/
	EMF_MAIL_REPORT_MDN,                         /**< The mail is a read-status report mail.*/
	EMF_MAIL_REPORT_REQUEST,                     /**< The mail require a read-status report mail.*/
} emf_mail_report_t;

/** 
 * This enumeration specifies the DRM type
 */
typedef enum 
{
	EMF_ATTACHMENT_DRM_NONE        = 0,          /**< The mail isn't DRM file.*/
	EMF_ATTACHMENT_DRM_OBJECT,                   /**< The mail is a DRM object.*/
	EMF_ATTACHMENT_DRM_RIGHTS,                   /**< The mail is a DRM rights as xml format.*/
	EMF_ATTACHMENT_DRM_DCF,                      /**< The mail is a DRM dcf.*/
} emf_attachment_drm_t;

typedef enum 
{
	EMF_MAIL_TYPE_NORMAL           = 0,          /*  NOT a meeting request mail. A Normal mail */
	EMF_MAIL_TYPE_MEETING_REQUEST  = 1,          /*  a meeting request mail from a serve */
	EMF_MAIL_TYPE_MEETING_RESPONSE = 2,          /*  a response mail about meeting reques */
	EMF_MAIL_TYPE_MEETING_ORIGINATINGREQUEST = 3 /*  a originating mail about meeting reques */
} emf_mail_type_t;

typedef enum 
{
	EMF_MEETING_RESPONSE_NONE      = 0,
	EMF_MEETING_RESPONSE_ACCEPT,
	EMF_MEETING_RESPONSE_TENTATIVE,
	EMF_MEETING_RESPONSE_DECLINE,
	EMF_MEETING_RESPONSE_REQUEST,                /*  create a meeting reques */
	EMF_MEETING_RESPONSE_CANCEL,                 /*  create a meeting cancelatio */
} emf_meeting_response_t;


typedef enum 
{
	EMF_ACTION_SEND_MAIL           = 0,
	EMF_ACTION_SYNC_HEADER,
	EMF_ACTION_DOWNLOAD_BODY,
	EMF_ACTION_DOWNLOAD_ATTACHMENT,
	EMF_ACTION_DELETE_MAIL,
	EMF_ACTION_SEARCH_MAIL, 
	EMF_ACTION_SAVE_MAIL,
	EMF_ACTION_SYNC_MAIL_FLAG_TO_SERVER,
	EMF_ACTION_SYNC_FLAGS_FIELD_TO_SERVER,
	EMF_ACTION_MOVE_MAIL,
	EMF_ACTION_CREATE_MAILBOX,                   /*  = 1 */
	EMF_ACTION_DELETE_MAILBOX,
	EMF_ACTION_SYNC_HEADER_OMA,
	EMF_ACTION_VALIDATE_ACCOUNT,
	EMF_ACTION_VALIDATE_AND_CREATE_ACCOUNT,
	EMF_ACTION_VALIDATE_AND_UPDATE_ACCOUNT,
	EMF_ACTION_ACTIVATE_PDP        = 20,
	EMF_ACTION_DEACTIVATE_PDP,
	EMF_ACTION_UPDATE_MAIL,
	EMF_ACTION_SET_MAIL_SLOT_SIZE,
	EMF_ACTION_NUM,
} emf_action_t;

/** 
 * This enumeration specifies the status of getting envelope list.
 */
typedef enum
{
	EMF_LIST_NONE                  = 0,
	EMF_LIST_WAITING,
	EMF_LIST_PREPARE,                            /**< Specifies the preparation.*/
	EMF_LIST_CONNECTION_START,                   /**< Specifies the connection start.*/
	EMF_LIST_CONNECTION_SUCCEED,                 /**< Specifies the success of connection.*/
	EMF_LIST_CONNECTION_FINISH,                  /**< Specifies the connection finish.*/
	EMF_LIST_CONNECTION_FAIL,                    /**< Specifies the connection failure.*/
	EMF_LIST_START,                              /**< Specifies the getting start.*/
	EMF_LIST_PROGRESS,                           /**< Specifies the status of getting.*/
	EMF_LIST_FINISH,                             /**< Specifies the getting complete.*/
	EMF_LIST_FAIL,                               /**< Specifies the download failure.*/
} emf_envelope_list_status_t;

/** 
 * This enumeration specifies the downloaded status of email. 
 */
typedef enum
{
	EMF_DOWNLOAD_NONE              = 0,
	EMF_DOWNLOAD_WAITING,
	EMF_DOWNLOAD_PREPARE,                        /**< Specifies the preparation.*/
	EMF_DOWNLOAD_CONNECTION_START,               /**< Specifies the connection start.*/
	EMF_DOWNLOAD_CONNECTION_SUCCEED,             /**< Specifies the success of connection.*/
	EMF_DOWNLOAD_CONNECTION_FINISH,              /**< Specifies the connection finish.*/
	EMF_DOWNLOAD_CONNECTION_FAIL,                /**< Specifies the connection failure.*/
	EMF_DOWNLOAD_START,                          /**< Specifies the download start.*/
	EMF_DOWNLOAD_PROGRESS,                       /**< Specifies the status of download.*/
	EMF_DOWNLOAD_FINISH,                         /**< Specifies the download complete.*/
	EMF_DOWNLOAD_FAIL,                           /**< Specifies the download failure.*/
} emf_download_status_t;

/** 
 * This enumeration specifies the status of sending email.
 */
typedef enum
{
	EMF_SEND_NONE                  = 0,
	EMF_SEND_WAITING,
	EMF_SEND_PREPARE,                            /**< Specifies the preparation.*/
	EMF_SEND_CONNECTION_START,                   /**< Specifies the connection start.*/
	EMF_SEND_CONNECTION_SUCCEED,                 /**< Specifies the success of connection.*/
	EMF_SEND_CONNECTION_FINISH,                  /**< Specifies the connection finish.*/
	EMF_SEND_CONNECTION_FAIL,                    /**< Specifies the connection failure.*/
	EMF_SEND_START,                              /**< Specifies the sending start.*/
	EMF_SEND_PROGRESS,                           /**< Specifies the status of sending.*/
	EMF_SEND_FINISH,                             /**< Specifies the sending complete.*/
	EMF_SEND_FAIL,                               /**< Specifies the sending failure.*/
	EMF_SAVE_WAITING,                            /**< Specfies the Waiting of Sync */
} emf_send_status_t;

typedef enum
{
	EMF_SYNC_NONE                  = 0,
	EMF_SYNC_WAITING,
	EMF_SYNC_PREPARE,
	EMF_SYNC_CONNECTION_START,
	EMF_SYNC_CONNECTION_SUCCEED,
	EMF_SYNC_CONNECTION_FINISH,
	EMF_SYNC_CONNECTION_FAIL,
	EMF_SYNC_START,
	EMF_SYNC_PROGRESS,
	EMF_SYNC_FINISH,
	EMF_SYNC_FAIL,
} emf_sync_status_t;

/** 
* This enumeration specifies the deleting status of email. 
*/
typedef enum
{
	EMF_DELETE_NONE                = 0,
	EMF_DELETE_WAITING,
	EMF_DELETE_PREPARE,                          /**< Specifies the preparation.*/
	EMF_DELETE_CONNECTION_START,                 /**< Specifies the connection start.*/
	EMF_DELETE_CONNECTION_SUCCEED,               /**< Specifies the success of connection.*/
	EMF_DELETE_CONNECTION_FINISH,                /**< Specifies the connection finish.*/
	EMF_DELETE_CONNECTION_FAIL,                  /**< Specifies the connection failure.*/
	EMF_DELETE_START,                            /**< Specifies the deletion start.*/
	EMF_DELETE_PROGRESS,                         /**< Specifies the status of deleting.*/
	EMF_DELETE_SERVER_PROGRESS,                  /**< Specifies the status of server deleting.*/ 
	EMF_DELETE_LOCAL_PROGRESS,                   /**< Specifies the status of local deleting.*/
	EMF_DELETE_FINISH,                           /**< Specifies the deletion complete.*/
	EMF_DELETE_FAIL,                             /**< Specifies the deletion failure.*/
} emf_delete_status_t;

/** 
* This enumeration specifies the status of validating account
*/
typedef enum
{
	EMF_VALIDATE_ACCOUNT_NONE = 0,
	EMF_VALIDATE_ACCOUNT_WAITING,
	EMF_VALIDATE_ACCOUNT_PREPARE,                /**< Specifies the preparation.*/
	EMF_VALIDATE_ACCOUNT_CONNECTION_START,       /**< Specifies the connection start.*/
	EMF_VALIDATE_ACCOUNT_CONNECTION_SUCCEED,     /**< Specifies the success of connection.*/
	EMF_VALIDATE_ACCOUNT_CONNECTION_FINISH,      /**< Specifies the connection finish.*/
	EMF_VALIDATE_ACCOUNT_CONNECTION_FAIL,        /**< Specifies the connection failure.*/
	EMF_VALIDATE_ACCOUNT_START,                  /**< Specifies the getting start.*/
	EMF_VALIDATE_ACCOUNT_PROGRESS,               /**< Specifies the status of getting.*/
	EMF_VALIDATE_ACCOUNT_FINISH,                 /**< Specifies the getting complete.*/
	EMF_VALIDATE_ACCOUNT_FAIL,                   /**< Specifies the validation failure.*/
} emf_validate_account_status_t;

typedef enum 
{
	EMF_SET_SLOT_SIZE_NONE         = 0,
	EMF_SET_SLOT_SIZE_WAITING,
	EMF_SET_SLOT_SIZE_START,  
	EMF_SET_SLOT_SIZE_FINISH,  
	EMF_SET_SLOT_SIZE_FAIL,   
}emf_set_slot_size_status_e;

/**
* This enumeration specifies the type of mailbox
*/
typedef enum 
{
	EMF_MAILBOX_TYPE_NONE          = 0,         /**< Unspecified mailbox type*/
	EMF_MAILBOX_TYPE_INBOX         = 1,         /**< Specified inbox type*/
	EMF_MAILBOX_TYPE_SENTBOX       = 2,         /**< Specified sent box type*/
	EMF_MAILBOX_TYPE_TRASH         = 3,         /**< Specified trash type*/
	EMF_MAILBOX_TYPE_DRAFT         = 4,         /**< Specified draft box type*/
	EMF_MAILBOX_TYPE_SPAMBOX       = 5,         /**< Specified spam box type*/
	EMF_MAILBOX_TYPE_OUTBOX        = 6,         /**< Specified outbox type*/
	EMF_MAILBOX_TYPE_ALL_EMAILS    = 7,         /**< Specified all emails type of gmail*/
	EMF_MAILBOX_TYPE_SEARCH_RESULT = 8,         /**< Specified mailbox type for result of search on server */
	EMF_MAILBOX_TYPE_USER_DEFINED  = 0xFF,      /**< Specified mailbox type for all other mailboxes */
}emf_mailbox_type_e;

typedef enum
{
	EMF_SYNC_LATEST_MAILS_FIRST    = 0,
	EMF_SYNC_OLDEST_MAILS_FIRST,
	EMF_SYNC_ALL_MAILBOX_50_MAILS,
} EMF_RETRIEVE_MODE;

/*  event typ */
typedef enum 
{
	EMF_EVENT_NONE                 = 0,
	EMF_EVENT_SYNC_HEADER          = 1,          /*  synchronize mail headers with server (network used) */
	EMF_EVENT_DOWNLOAD_BODY,                     /*  download mail body from server (network used)*/
	EMF_EVENT_DOWNLOAD_ATTACHMENT,               /*  download mail attachment from server (network used) */
	EMF_EVENT_SEND_MAIL,                         /*  send a mail (network used) */
	EMF_EVENT_SEND_MAIL_SAVED,                   /*  send all mails in 'outbox' box (network used) */
	EMF_EVENT_SYNC_IMAP_MAILBOX,                 /*  download imap mailboxes from server (network used) */
	EMF_EVENT_DELETE_MAIL,                       /*  delete mails (network unused) */
	EMF_EVENT_DELETE_MAIL_ALL,                   /*  delete all mails (network unused) */
	EMF_EVENT_SYNC_MAIL_FLAG_TO_SERVER,          /*  sync mail flag to server */
	EMF_EVENT_SYNC_FLAGS_FIELD_TO_SERVER,        /*  sync a field of flags to server */
	EMF_EVENT_SAVE_MAIL,
	EMF_EVENT_MOVE_MAIL,                         /*  move mails to specific mailbox on server */
	EMF_EVENT_CREATE_MAILBOX,  
	EMF_EVENT_UPDATE_MAILBOX,
	EMF_EVENT_DELETE_MAILBOX,  
	EMF_EVENT_ISSUE_IDLE,
	EMF_EVENT_SYNC_HEADER_OMA,
	EMF_EVENT_VALIDATE_ACCOUNT,
	EMF_EVENT_VALIDATE_AND_CREATE_ACCOUNT, 
	EMF_EVENT_VALIDATE_AND_UPDATE_ACCOUNT,  

	EMF_EVENT_ADD_MAIL             = 10001,
	EMF_EVENT_UPDATE_MAIL_OLD      = 10002,
	EMF_EVENT_UPDATE_MAIL          = 10003,
	EMF_EVENT_SET_MAIL_SLOT_SIZE   = 20000,
  
/* 	EMF_EVENT_LOCAL_ACTIVITY,                    // __LOCAL_ACTIVITY_ */

	EMF_EVENT_BULK_PARTIAL_BODY_DOWNLOAD,        /*  __FEATURE_PARTIAL_BODY_DOWNLOAD__ supported */
	EMF_EVENT_LOCAL_ACTIVITY_SYNC_BULK_PBD,      /*  __FEATURE_PARTIAL_BODY_DOWNLOAD__ supported */
	EMF_EVENT_GET_PASSWORD_LENGTH                /*  get password length of an account */
} emf_event_type_t;

/*  event statu */
typedef enum 
{
	EMF_EVENT_STATUS_UNUSED        = 0,
	EMF_EVENT_STATUS_WAIT,
	EMF_EVENT_STATUS_STARTED,
	EMF_EVENT_STATUS_CANCELED,
} emf_event_status_type_t;


/* sorting_orde */
typedef enum 
{
	EMF_SORT_DATETIME_HIGH         = 0,
	EMF_SORT_DATETIME_LOW,    
	EMF_SORT_SENDER_HIGH,
	EMF_SORT_SENDER_LOW,   
	EMF_SORT_RCPT_HIGH,
	EMF_SORT_RCPT_LOW,    
	EMF_SORT_SUBJECT_HIGH,
	EMF_SORT_SUBJECT_LOW,   
	EMF_SORT_PRIORITY_HIGH,
	EMF_SORT_PRIORITY_LOW,   
	EMF_SORT_ATTACHMENT_HIGH,
	EMF_SORT_ATTACHMENT_LOW,   
	EMF_SORT_FAVORITE_HIGH,
	EMF_SORT_FAVORITE_LOW,
	EMF_SORT_MAILBOX_NAME_HIGH,
	EMF_SORT_MAILBOX_NAME_LOW,
	EMF_SORT_FLAGGED_FLAG_HIGH,
	EMF_SORT_FLAGGED_FLAG_LOW,
	EMF_SORT_SEEN_FLAG_HIGH,
	EMF_SORT_SEEN_FLAG_LOW,
	EMF_SORT_END,
}emf_sort_type_t;

typedef enum
{
	EMAIL_MAILBOX_SORT_BY_NAME_ASC  = 0, 
	EMAIL_MAILBOX_SORT_BY_NAME_DSC,
	EMAIL_MAILBOX_SORT_BY_TYPE_ASC, 
	EMAIL_MAILBOX_SORT_BY_TYPE_DSC,
} email_mailbox_sort_type_t;


/** 
* This enumeration specifies the priority.
*/
enum
{
	EMF_OPTION_PRIORITY_HIGH       = 1,          /**< Specifies the high priority.*/
	EMF_OPTION_PRIORITY_NORMAL     = 3,          /**< Specifies the normal priority*/
	EMF_OPTION_PRIORITY_LOW        = 5,          /**< Specifies the low priority.*/
};

/** 
* This enumeration specifies the saving save a copy after sending.
*/
enum
{
	EMF_OPTION_KEEP_LOCAL_COPY_OFF = 0,          /**< Specifies off the keeping local copy.*/
	EMF_OPTION_KEEP_LOCAL_COPY_ON  = 1,          /**< Specifies on the keeping local copy.*/
};

/** 
* This enumeration specifies the request of delivery report.
*/
enum
{
	EMF_OPTION_REQ_DELIVERY_RECEIPT_OFF = 0,     /**< Specifies off the requesting delivery receipt.*/
	EMF_OPTION_REQ_DELIVERY_RECEIPT_ON  = 1,     /**< Specifies on the requesting delivery receipt.*/
};

/** 
* This enumeration specifies the request of read receipt.
*/
enum
{
	EMF_OPTION_REQ_READ_RECEIPT_OFF = 0,         /**< Specifies off the requesting read receipt.*/
	EMF_OPTION_REQ_READ_RECEIPT_ON  = 1,         /**< Specifies on the requesting read receipt.*/
};

/** 
* This enumeration specifies the blocking of address.
*/
enum
{
	EMF_OPTION_BLOCK_ADDRESS_OFF   = 0,          /**< Specifies off the blocking by address.*/
	EMF_OPTION_BLOCK_ADDRESS_ON    = 1,          /**< Specifies on the blocking by address.*/
};

/** 
* This enumeration specifies the blocking of subject.
*/
enum
{
	EMF_OPTION_BLOCK_SUBJECT_OFF   = 0,          /**< Specifies off the blocking by subject.*/
	EMF_OPTION_BLOCK_SUBJECT_ON    = 1,          /**< Specifies on the blocking by subject.*/
};

enum
{
	EMF_LIST_TYPE_UNREAD           = -3,
	EMF_LIST_TYPE_LOCAL            = -2,
	EMF_LIST_TYPE_THREAD           = -1,
	EMF_LIST_TYPE_NORMAL           = 0
};

/** 
* This enumeration specifies the mailbox sync type.
*/
enum
{
	EMF_MAILBOX_ALL                = -1,         /**< All mailboxes.*/
	EMF_MAILBOX_FROM_SERVER        = 0,          /**< Mailboxes from server. */
	EMF_MAILBOX_FROM_LOCAL         = 1,          /**< Mailboxes from local. */
};

typedef enum 
{
	APPEND_BODY                    = 1,
	UPDATE_MAILBOX,
	UPDATE_ATTACHMENT_INFO,
	UPDATE_FLAG,
	UPDATE_SAVENAME,
	UPDATE_EXTRA_FLAG,
	UPDATE_MAIL,
	UPDATE_DATETIME,
	UPDATE_FROM_CONTACT_INFO,
	UPDATE_TO_CONTACT_INFO,
	UPDATE_ALL_CONTACT_NAME,
	UPDATE_ALL_CONTACT_INFO,
	UPDATE_STICKY_EXTRA_FLAG, 
	UPDATE_PARTIAL_BODY_DOWNLOAD,
	UPDATE_MEETING,
	UPDATE_SEEN_FLAG_OF_THREAD,
} emf_mail_change_type_t;


/** 
* This enumeration specifies the address type.
*/
typedef enum 
{
	EMF_ADDRESS_TYPE_FROM          = 1,          /**< Specifies the from address.*/
	EMF_ADDRESS_TYPE_TO,                         /**< Specifies the to receipient address.*/
	EMF_ADDRESS_TYPE_CC,                         /**< Specifies the cc receipient address.*/
	EMF_ADDRESS_TYPE_BCC,                        /**< Specifies the bcc receipient address.*/
	EMF_ADDRESS_TYPE_REPLY,                      /**< Specifies the reply receipient address.*/
	EMF_ADDRESS_TYPE_RETURN,                     /**< Specifies the return receipient address.*/
} emf_address_type_t;

/** 
 * This enumeration specifies the search type for searching mailbox.
 */
typedef enum 
{
	EMF_MAILBOX_SEARCH_KEY_TYPE_SUBJECT,         /**< The search key is for searching subject.*/
	EMF_MAILBOX_SEARCH_KEY_TYPE_FROM,            /**< The search key is for searching sender address.*/
	EMF_MAILBOX_SEARCH_KEY_TYPE_BODY,            /**< The search key is for searching body.*/
	EMF_MAILBOX_SEARCH_KEY_TYPE_TO,              /**< The search key is for searching recipient address.*/
} emf_mailbox_search_key_t;

/** 
 * This enumeration specifies the download status of mail body.
 */

typedef enum
{
	EMF_BODY_DOWNLOAD_STATUS_NONE = 0,
	EMF_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED = 1,
	EMF_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED = 2,
} emf_body_download_status_t;

/** 
 * This enumeration specifies the moving type.
 */
typedef enum
{
	EMF_MOVED_BY_COMMAND = 0,
	EMF_MOVED_BY_MOVING_THREAD,
	EMF_MOVED_AFTER_SENDING
} emf_move_type;

/** 
 * This enumeration specifies the deletion type.
 */
typedef enum
{
	EMF_DELETED_BY_COMMAND = 0,
	EMF_DELETED_BY_OVERFLOW,
	EMF_DELETED_BY_DELETING_THREAD,
	EMF_DELETED_BY_MOVING_TO_OTHER_ACCOUNT,
	EMF_DELETED_AFTER_SENDING,
	EMF_DELETED_FROM_SERVER,
} emf_delete_type;

/** 
 * This enumeration specifies the status field type.
 */
typedef enum
{
	EMF_FLAGS_SEEN_FIELD = 0,
	EMF_FLAGS_DELETED_FIELD,
	EMF_FLAGS_FLAGGED_FIELD,
	EMF_FLAGS_ANSWERED_FIELD,
	EMF_FLAGS_RECENT_FIELD,
	EMF_FLAGS_DRAFT_FIELD,
	EMF_FLAGS_FORWARDED_FIELD,
	EMF_FLAGS_FIELD_COUNT,
} emf_flags_field_type;

typedef enum
{
	EMF_FLAG_NONE                  = 0,
	EMF_FLAG_FLAGED                = 1,
	EMF_FLAG_TASK_STATUS_CLEAR     = 2,
	EMF_FLAG_TASK_STATUS_ACTIVE    = 3,
	EMF_FLAG_TASK_STATUS_COMPLETE  = 4,
} emf_flagged_type;


/*****************************************************************************/
/*  Data Structures                                                          */
/*****************************************************************************/

/** 
 * This structure is used to save mail time.
 */
typedef struct
{
	unsigned short year;                         /**< Specifies the Year.*/
	unsigned short month;                        /**< Specifies the Month.*/
	unsigned short day;                          /**< Specifies the Day.*/
	unsigned short hour;                         /**< Specifies the Hour.*/
	unsigned short minute;                       /**< Specifies the Minute.*/
	unsigned short second;                       /**< Specifies the Second.*/
} emf_datetime_t;

/** 
 * This structure is used to save the options.
 */
typedef struct
{
	int   priority;                              /**< Specifies the prority. 1=high 3=normal 5=low.*/
	int   keep_local_copy;                       /**< Specifies the saving save a copy after sending.*/
	int   req_delivery_receipt;                  /**< Specifies the request of delivery report. 0=off 1=on*/
	int   req_read_receipt;                      /**< Specifies the request of read receipt. 0=off 1=on*/
	int   download_limit;                        /**< Specifies the limit of size for downloading.*/
	int   block_address;                         /**< Specifies the blocking of address. 0=off 1=on*/
	int   block_subject;                         /**< Specifies the blocking of subject. 0=off 1=on*/
	char *display_name_from;                     /**< Specifies the display name of from.*/
	int   reply_with_body;                       /**< Specifies the replying with body 0=off 1=on*/
	int   forward_with_files;                    /**< Specifies the fowarding with files 0=off 1=on*/
	int   add_myname_card;                       /**< Specifies the adding name card 0=off 1=on*/
	int   add_signature;                         /**< Specifies the adding signature 0=off 1=on*/
	char *signature;                             /**< Specifies the signature*/
	int   add_my_address_to_bcc;                 /**< Specifies whether bcc field should be always filled with my address. 0=off 1=on*/
} emf_option_t;

/** 
 * This structure is used to save the information of email account.
 */
typedef struct 
{
	emf_account_bind_t          account_bind_type;      /**< Specifies the Bind Type.*/
	char                       *account_name;           /**< Specifies the account name.*/
	emf_account_server_t        receiving_server_type;  /**< Specifies the receiving server type.*/
	char                       *receiving_server_addr;  /**< Specifies the address of receiving server.*/
	char                       *email_addr;             /**< Specifies the email adderee.*/
	char                       *user_name;              /**< Specifies the user name.*/
	char                       *password;               /**< Specifies the password.*/
	emf_imap4_retrieval_mode_t  retrieval_mode;         /**< Specifies the retrieval mode in IMAP case.*/
	int                         port_num;               /**< Specifies the port number of receiving server.*/
	int                         use_security;           /**< Specifies the security such as SSL.*/
	emf_account_server_t        sending_server_type;    /**< Specifies the type of sending server.*/
	char                       *sending_server_addr;    /**< Specifies the address of sending server.*/
	int                         sending_port_num;       /**< Specifies the port number of sending server.*/
	int                         sending_auth;           /**< Specifies the authentication of sending server.*/
	int                         sending_security;       /**< Specifies the security such as SSL.*/
	char                       *sending_user;           /**< Specifies the user name of SMTP server.*/
	char                       *sending_password;       /**< Specifies the user password of SMTP server.*/
	char                       *display_name;           /**< Specifies the display name.*/
	char                       *reply_to_addr;          /**< Specifies the reply email address.*/
	char                       *return_addr;            /**< Specifies the email address for return.*/
	int                         account_id;             /**< Specifies the ID of account. Especially, 1 is assigned to Local Account.*/
	int                         keep_on_server;         /**< Specifies the keeping mail on server.*/
	int                         flag1;                  /**< Specifies the downloading option. 0 is subject only, 1 is text body, 2 is normal.*/
	int                         flag2;                  /**< Specifies the 'Same as POP3' option. 0 is none, 1 is 'Same as POP3'.*/
	int                         pop_before_smtp;        /**< POP before SMTP authentication */  
	int                         apop;                   /**< APOP Authentication */
	char                       *logo_icon_path;         /**< Account logo icon */
	int                         preset_account;         /**< Preset account or not */
	emf_option_t                options;                /**< Specifies the Sending options */
	int                         target_storage;         /**< Specifies the targetStorage. 0 is phone, 1 is MMC */
	int                         check_interval;         /**< Specifies the Check interval. Unit is minutes */
	int                         my_account_id;          /**< Specifies accout id of my account */
	int                         index_color;            /**< Specifies index color for displaying classifying accounts */
}emf_account_t;

/**
 * This structure is used to save the information of email server.
 */

typedef struct
{
	int                         configuration_id;      /**< Specifies the id of configuration.*/
	emf_account_server_t        protocol_type;         /**< Specifies the type of configuration.*/
	char                       *server_addr;           /**< Specifies the address of configuration.*/
	int                         port_number;           /**< Specifies the port number of configuration.*/
	int                         security_type;         /**< Specifies the security such as SSL.*/
	int                         auth_type;             /**< Specifies the authentication type of configuration.*/
} emf_protocol_config_t;

typedef struct
{
	char                       *service_name;           /**< Specifies the name of service.*/
	int                         authname_format;        /**< Specifies the type of user name for authentication.*/
	int                         protocol_conf_count;    /**< Specifies count of protocol configurations.*/
	emf_protocol_config_t      *protocol_config_array;  /**< Specifies array of protocol configurations.*/
} emf_server_info_t;

typedef struct 
{
	int   mailbox_type;
	char  mailbox_name[MAILBOX_NAME_LENGTH];
} emf_mailbox_type_item_t;

/** 
 * This structure is contains the Mail information.
 */

typedef struct 
{
	int    mail_id;                 /**< Specifies the Mail ID.*/
	int    account_id;              /**< Specifies the Account ID.*/
	char  *mailbox_name;            /**< Specifies the Mailbox Name.*/
	int    mailbox_type;            /**< Specifies the mailbox type of the mail. */
	char  *subject;                 /**< Specifies the subject.*/
	char  *datetime;                /**< Specifies the Date time.*/
	int    server_mail_status;      /**< Specifies the Whether sever mail or not.*/
	char  *server_mailbox_name;     /**< Specifies the server mailbox.*/
	char  *server_mail_id;          /**< Specifies the Server Mail ID.*/
	char  *message_id;              /**< Specifies the message id */ 
	char  *full_address_from;       /**< Specifies the From Addr.*/
	char  *full_address_reply;      /**< Specifies the Reply to addr */
	char  *full_address_to;         /**< Specifies the To addr.*/
	char  *full_address_cc;         /**< Specifies the CC addr.*/
	char  *full_address_bcc;        /**< Specifies the BCC addr*/
	char  *full_address_return;     /**< Specifies the return Path*/
	char  *email_address_sender;    /**< Specifies the email address of sender.*/
	char  *email_address_recipient; /**< Specifies the email address of recipients.*/
	char  *alias_sender;            /**< Specifies the alias of sender. */
	char  *alias_recipient;         /**< Specifies the alias of recipients. */
	int    body_download_status;    /**< Specifies the Text donwloaded or not.*/
	char  *file_path_plain;         /**< Specifies the path of text mail body.*/    
	char  *file_path_html;          /**< Specifies the path of HTML mail body.*/       
	int    mail_size;               /**< Specifies the Mail Size.*/   
	char   flags_seen_field;        /**< Specifies the seen flags*/
	char   flags_deleted_field;     /**< Specifies the deleted flags*/
	char   flags_flagged_field;     /**< Specifies the flagged flags*/
	char   flags_answered_field;    /**< Specifies the answered flags*/
	char   flags_recent_field;      /**< Specifies the recent flags*/
	char   flags_draft_field;       /**< Specifies the draft flags*/
	char   flags_forwarded_field;   /**< Specifies the forwarded flags*/
	int    DRM_status;              /**< Specifies the DRM state. */
	int    priority;                /**< Specifies the priority of Mails.*/
	int    save_status;             /**< Specifies the save status*/
	int    lock_status;             /**< Specifies the Locked*/  
	int    report_status;           /**< Specifies the Mail Report.*/
	int    attachment_count;        /**< Specifies the attachment count. */
	int    inline_content_count;    /**< Specifies the inline content count. */
	int    thread_id;               /**< Specifies the thread id for thread view. */	
	int    thread_item_count;       /**< Specifies the item count of specific thread. */
	char  *preview_text;            /**< Specifies the preview body. */	       
	int    meeting_request_status;  /**< Specifies the status of meeting request. */
}emf_mail_data_t;

/** 
 * This structure is contains information for displaying mail list.
 */
typedef struct 
{
	int  mail_id;                                          /**< Specifies the Mail ID.*/
	int  account_id;                                       /**< Specifies the Account ID.*/
	char mailbox_name[STRING_LENGTH_FOR_DISPLAY];          /**< Specifies the Mailbox Name.*/
	char from[STRING_LENGTH_FOR_DISPLAY];                  /**< Specifies the from display name.*/
	char from_email_address[MAX_EMAIL_ADDRESS_LENGTH];     /**< Specifies the from email address.*/
	char recipients[STRING_LENGTH_FOR_DISPLAY];            /**< Specifies the recipients display name.*/
	char subject[STRING_LENGTH_FOR_DISPLAY];               /**< Specifies the subject.*/
	int  is_text_downloaded;                               /**< Specifies the Text donwloaded or not.*/
	char datetime[MAX_DATETIME_STRING_LENGTH];             /**< Specifies the Date time.*/
	char flags_seen_field;                                 /**< Specifies the seen flags*/
	char flags_deleted_field;                              /**< Specifies the deleted flags*/
	char flags_flagged_field;                              /**< Specifies the flagged flags*/
	char flags_answered_field;                             /**< Specifies the answered flags*/
	char flags_recent_field;                               /**< Specifies the recent flags*/
	char flags_draft_field;                                /**< Specifies the draft flags*/
	char flags_forwarded_field;                            /**< Specifies the forwarded flags*/
	int  priority;                                         /**< Specifies the priority of Mails.*/
	int  save_status;                                      /**< Specifies the save status*/
	int  is_locked;                                        /**< Specifies the Locked*/
	int  is_report_mail;                                   /**< Specifies the Mail Report.*/
	int  recipients_count;                                 /**< Specifies the number of to Recipients*/
	int  has_attachment;                                   /**< the mail has attachments or not[ 0: none, 1: over one] */
	int  has_drm_attachment;                               /**< the mail has drm attachment or not*/
	char previewBodyText[MAX_PREVIEW_TEXT_LENGTH];         /**< text for preview body*/
	int  thread_id;                                        /**< thread id for thread view*/
	int  thread_item_count;                                /**< item count of specific thread */
	int  is_meeting_request;                               /**< Whether the mail is a meeting request or not */
}emf_mail_list_item_t;

/** 
 * This structure is used to save the filtering structure.
 */
typedef struct 
{
	int                account_id;   /**< Specifies the account ID.*/
	int                filter_id;    /**< Specifies the filtering ID.*/
	emf_rule_type_t    type;         /**< Specifies the filtering type.*/
	char              *value;        /**< Specifies the filtering value.*/
	emf_rule_action_t  faction;      /**< Specifies the action type for filtering.*/
	char              *mailbox;      /**< Specifies the mail box if action type means move.*/
	int                flag1;        /**< Specifies the activation.*/
	int                flag2;        /**< Reserved.*/
} emf_rule_t;

/** 
 * This structure is used to save the information of mail flag.
 */
typedef struct
{
	unsigned int  seen           : 1; /**< Specifies the read email.*/
	unsigned int  deleted        : 1; /**< Reserved.*/
	unsigned int  flagged        : 1; /**< Specifies the flagged email.*/
	unsigned int  answered       : 1; /**< Reserved.*/
	unsigned int  recent         : 1; /**< Reserved.*/
	unsigned int  draft          : 1; /**< Specifies the draft email.*/
	unsigned int  has_attachment : 1; /**< Reserved.*/
	unsigned int  forwarded      : 1; /**< Reserved.*/
	unsigned int  sticky         : 1; /**< Sticky flagged mails cannot be deleted */
} emf_mail_flag_t;


/** 
 * This structure is used to save the information of mail extra flag.
 */
typedef struct
{
	unsigned int  priority         : 3; /**< Specifies the mail priority. 
                                           The value is greater than or equal to EMF_MAIL_PRIORITY_HIGH.
                                           The value is less than or eqult to EMF_MAIL_PRIORITY_LOW.*/
	unsigned int  status           : 4; /**< Specifies the mail status. 
	                                       The value is a value of enumeration emf_mail_status_t.*/
	unsigned int  noti             : 1; /**< Specifies the notified mail.*/ 
	unsigned int  lock             : 1; /**< Specifies the locked mail.*/   
	unsigned int  report           : 2; /**< Specifies the MDN/DSN mail. The value is a value of enumeration emf_mail_report_t.*/
	unsigned int  drm              : 1; /**< Specifies the drm mail.*/
	unsigned int  text_download_yn : 2; /**< body download y/n*/            /* To be removed */
} emf_extra_flag_t;

/** 
 * This structure is used to save the email basic information.
 */
typedef struct 
{
	int               account_id;           /**< Specifies the email's account ID.*/
	int               uid;                  /**< Specifies the email's UID on local.*/
	int               rfc822_size;          /**< Specifies the email's size.*/
	int               body_downloaded;      /**< Specifies the download of email body.*/
	emf_mail_flag_t   flags;                /**< Specifies the email's flag information.*/
	emf_extra_flag_t  extra_flags;          /**< Specifies the email's extra flag information.*/
	char             *sid;                  /**< Specifies the email's UID on server.*/
	int               is_meeting_request;   /**< Specifies whether this mail is a meeting request mail or not. 0: normal mail, 1: meeting request mail from server, 2: response mail about meeting request mail */
	int               thread_id;            /**< Specifies the email's thread id.*/
	int               thread_item_count;    /**< Specifies the item count of thread*/
} emf_mail_info_t;

/** 
 * This structure is used to save the information of email header.
 */
typedef struct 
{
	char             *mid;                  /**< Specifies the message ID.*/
	char             *subject;              /**< Specifies the email subject.*/
	char             *to;                   /**< Specifies the recipient.*/
	char             *from;                 /**< Specifies the sender.*/
	char             *cc;                   /**< Specifies the carbon copy.*/
	char             *bcc;                  /**< Specifies the blind carbon copy.*/
	char             *reply_to;             /**< Specifies the replier.*/
	char             *return_path;          /**< Specifies the address of return.*/
	emf_datetime_t    datetime;             /**< Specifies the sending or receiving time.*/
	char             *from_contact_name;
	char             *to_contact_name;
	char             *cc_contact_name;
	char             *bcc_contact_name;
	char             *previewBodyText;      /**< Specifies the text contains for preview body.*/
} emf_mail_head_t;

/** 
 * This structure is used to save the information of attachment.
 */
typedef struct st_emf_attachment_info 
{
	int                            inline_content;
	int                            attachment_id;   /**< Specifies the attachment ID*/
	char                          *name;            /**< Specifies the attachment name.*/
	int                            size;            /**< Specifies the attachment size.*/
	int                            downloaded;      /**< Specifies the download of attachment.*/
	char                          *savename;        /**< Specifies the absolute path of attachment.*/
	int                            drm;             /**< Specifies the drm type.*/
	struct st_emf_attachment_info *next;            /**< Specifies the pointer of next attachment.*/
} emf_attachment_info_t;

typedef struct 
{
	int   attachment_id;
	char *attachment_name;
	char *attachment_path;
	int   attachment_size;
	int   mail_id;
	int   account_id;
	char *mailbox_name;
	int   save_status;
	int   drm_status;
	int   inline_content_status;
} emf_attachment_data_t;

/** 
 * This structure is used to save the information of email body
 */
typedef struct 
{
	char                  *plain;          /**< Specifies the absolute path of file to contain email body (Plain Text).*/
	char                  *plain_charset;  /**< Specifies the character set of plain text boy. */
	char                  *html;           /**< Specifies the absolute path of file to contain email body (HTML).*/
	int                    attachment_num; /**< Specifies the count of attachment.*/
	emf_attachment_info_t *attachment;     /**< Specifies the structure of attachment.*/
} emf_mail_body_t;

/** 
 * This structure is used to save the information of email
 */
typedef struct 
{
	emf_mail_info_t  *info;  /**< Specifies the structure pointer of mail basic information.*/
	emf_mail_head_t  *head;  /**< Specifies the structure pointer of email header.*/
	emf_mail_body_t  *body;  /**< Specifies the structure pointer of email body.*/
} emf_mail_t;


typedef struct 
{
	int        offset_from_GMT;
	char       standard_name[32];
	struct tm  standard_time_start_date;
	int        standard_bias;
	char       daylight_name[32];
	struct tm  daylight_time_start_date;
	int        daylight_bias;
} emf_time_zone_t;

typedef struct 
{
	int                     mail_id;                                        /**< Specifies the mail id of meeting request on DB. This is the primary key. */
	emf_meeting_response_t  meeting_response;                               /**< Specifies the meeting response. */
	struct tm               start_time;
	struct tm               end_time;
	char                   *location;                                       /**< Specifies the location of meeting. Maximum length of this string is 32768 */
	char                    global_object_id[MEETING_REQ_OBJECT_ID_LENGTH]; /**< Specifies the object id. */
	emf_time_zone_t         time_zone;
} emf_meeting_request_t;

/**
 * This structure is used to save the informatioin of sender list with unread/total mail counts
 */
typedef struct 
{
	char *address;         /**< Specifies the address of a sender.*/
	char *display_name;    /**< Specifies a display name. This may be one of contact name, alias in original mail and email address of sender. (Priority order : contact name, alias, email address) */
	int   unread_count;    /**< Specifies the number of unread mails received from sender address*/
	int   total_count;     /**< Specifies the total number of  mails which are received from sender address*/
} emf_sender_list_t;


/* Creates a type name for structure emf_mailbox_st */
typedef struct emf_mailbox_st emf_mailbox_t; /**< This is an information of mail box. */

/** 
 * This structure is used to save the information of mailbox.
 */
struct emf_mailbox_st
{
	int                 mailbox_id;                 /**< Unique id on mailbox table.*/
	char               *name;                       /**< Specifies the path of mailbox.*/
	emf_mailbox_type_e  mailbox_type;
	char               *alias;                      /**< Specifies the display name of mailbox.*/
	int                 unread_count;               /**< Specifies the Unread Mail count in the mailbox.*/
	int                 total_mail_count_on_local;  /**< Specifies the total number of mails in the mailbox in the local DB.*/
	int                 total_mail_count_on_server; /**< Specifies the total number of mails in the mailbox in the mail server.*/
	int                 hold_connection;            /**< Will have a valid socket descriptor when connection to server is active.. else 0>*/
	int                 local;                      /**< Specifies the local mailbox.*/
	int                 synchronous;                /**< Specifies the mailbox with synchronized the server.*/
	int                 account_id;                 /**< Specifies the account ID for mailbox.*/
	void               *user_data;                  /**< Specifies the internal data.*/
	void               *mail_stream;                /**< Specifies the internal data.*/
	int                 has_archived_mails;         /**< Specifies the archived mails.*/
	int                 mail_slot_size;             /**< Specifies how many mails can be stored.*/           
	char               *account_name;               /**< Specifies the name of account.*/           
	emf_mailbox_t      *next;                       /**< Reserved.*/
};


typedef struct 
{
	char *contact_name;
	char *email_address;
	char *alias;
	int   storage_type;
	int   contact_id;
} emf_mail_contact_info_t;

typedef struct 
{
	emf_address_type_t  address_type;
	char               *address;
	char               *display_name;
	int                 storage_type;
	int                 contact_id;
} emf_address_info_t;

typedef struct
{
	GList *from;
	GList *to;
	GList *cc;
	GList *bcc;
} emf_address_info_list_t;

typedef struct 
{
	int     contact_id;     /*  Inpu */
	int     storage_type;   /*  Input, but not used in email-servic */
	int     address_count;  /*  Inpu */
	char  **address_list;   /*  Input, array of email account */
	int     unread_count;   /*  Outpu */
} emf_contact_item_for_count_t;

typedef struct 
{
	int     contact_id;     /*  Inpu */
	int     storage_type;   /*  Input, but not used in email-servic */
	int     address_count;  /*  Inpu */
	char  **address_list;   /*   Input array of email account */
	int     mail_id_count;  /*  Outpu */
	int    *mail_id_list;   /*   Outpu */
} emf_contact_item_for_mail_id_t;

typedef struct 
{
	int	address_type;		/* type of mail (sender : 0, recipient : 1)*/
	int     address_count;  /*  The number of email addresse */
	char  **address_list;   /*  strings of email addresse */
}emf_email_address_list_t;

/*  global account lis */
typedef struct emf_account_list_t emf_account_list_t;
struct emf_account_list_t
{
    emf_account_t *account;
    emf_account_list_t *next;
};


/*****************************************************************************/
/*  Errors                                                                   */
/*****************************************************************************/

#define EMF_ERROR_NONE                                 1       /*  There is no error */
#define EMF_ERROR_INVALID_PARAM                       -1001    /*  invalid parameter was given. - Invalid input paramete */
#define EMF_ERROR_INVALID_ACCOUNT                     -1002    /*  invalid account information was given. - Unsupported accoun */
#define EMF_ERROR_INVALID_USER                        -1003    /*  NOT USED : unknown user ID was given. - Invalid user or passwor */
#define EMF_ERROR_INVALID_PASSWORD                    -1004    /*  NOT USED : invalid password was given. - Invalid user or passwor */
#define EMF_ERROR_INVALID_SERVER                      -1005    /*  invalid server information was given. - Server unavailabl */
#define EMF_ERROR_INVALID_MAIL                        -1006    /*  invalid mail information was given */
#define EMF_ERROR_INVALID_ADDRESS                     -1007    /*  invalid address information was given. - Incorrect addres */
#define EMF_ERROR_INVALID_ATTACHMENT                  -1008    /*  invalid attachment information was given */
#define EMF_ERROR_INVALID_MAILBOX                     -1009    /*  invalid mailbox information was given */
#define EMF_ERROR_INVALID_FILTER                      -1010    /*  invalid filter information was given */
#define EMF_ERROR_INVALID_PATH                        -1011    /*  invalid flle path was given */
#define EMF_ERROR_INVALID_DATA                        -1012    /*  NOT USE */
#define EMF_ERROR_INVALID_RESPONSE                    -1013    /*  unexpected network response was given. - Invalid server respons */
#define EMF_ERROR_ACCOUNT_NOT_FOUND                   -1014    /*  no matched account was found */
#define EMF_ERROR_MAIL_NOT_FOUND                      -1015    /*  no matched mail was found */
#define EMF_ERROR_MAILBOX_NOT_FOUND                   -1016    /*  no matched mailbox was found */
#define EMF_ERROR_ATTACHMENT_NOT_FOUND                -1017    /*  no matched attachment was found */
#define EMF_ERROR_FILTER_NOT_FOUND                    -1018    /*  no matched filter was found */
#define EMF_ERROR_CONTACT_NOT_FOUND                   -1019    /*  no matched contact was found */
#define EMF_ERROR_FILE_NOT_FOUND                      -1020    /*  no matched file was found */
#define EMF_ERROR_DATA_NOT_FOUND                      -1021    /*  NOT USE */
#define EMF_ERROR_NO_MORE_DATA                        -1022    /*  NOT USE */
#define EMF_ERROR_ALREADY_EXISTS                      -1023    /*  data duplicate */
#define EMF_ERROR_MAX_EXCEEDED                        -1024    /*  NOT USE */
#define EMF_ERROR_DATA_TOO_LONG                       -1025    /*  NOT USE */
#define EMF_ERROR_DATA_TOO_SMALL                      -1026    /*  NOT USE */
#define EMF_ERROR_NETWORK_TOO_BUSY                    -1027    /*  NOT USE */
#define EMF_ERROR_OUT_OF_MEMORY                       -1028    /*  There is not enough memory */
#define EMF_ERROR_DB_FAILURE                          -1029    /*  database operation failed */
#define EMF_ERROR_PROFILE_FAILURE                     -1030    /*  no proper profile was found */
#define EMF_ERROR_SOCKET_FAILURE                      -1031    /*  socket operation failed */
#define EMF_ERROR_CONNECTION_FAILURE                  -1032    /*  network connection failed */
#define EMF_ERROR_CONNECTION_BROKEN                   -1033    /*  network connection was broken */
#define EMF_ERROR_DISCONNECTED                        -1034    /*  NOT USED : connection was disconnected */
#define EMF_ERROR_LOGIN_FAILURE                       -1035    /*  login failed */
#define EMF_ERROR_NO_RESPONSE                         -1036    /*  There is no server response */
#define EMF_ERROR_MAILBOX_FAILURE                     -1037    /*  The agent failed to scan mailboxes in server */
#define EMF_ERROR_AUTH_NOT_SUPPORTED                  -1038    /*  The server doesn't support authentication */
#define EMF_ERROR_AUTHENTICATE                        -1039    /*  The server failed to authenticate user */
#define EMF_ERROR_TLS_NOT_SUPPORTED                   -1040    /*  The server doesn't support TLS */
#define EMF_ERROR_TLS_SSL_FAILURE                     -1041    /*  The agent failed TLS/SSL */
#define EMF_ERROR_APPEND_FAILURE                      -1042    /*  The agent failed to append mail to server */
#define EMF_ERROR_COMMAND_NOT_SUPPORTED               -1043    /*  The server doesn't support this command */
#define EMF_ERROR_ANNONYM_NOT_SUPPORTED               -1044    /*  The server doesn't support anonymous user */
#define EMF_ERROR_CERTIFICATE_FAILURE                 -1045    /*  certificate failure - Invalid server certificat */
#define EMF_ERROR_CANCELLED                           -1046    /*  The job was canceled by user */
#define EMF_ERROR_NOT_IMPLEMENTED                     -1047    /*  The function was not implemented */
#define EMF_ERROR_NOT_SUPPORTED                       -1048    /*  The function is not supported */
#define EMF_ERROR_MAIL_LOCKED                         -1049    /*  The mail was locked */
#define EMF_ERROR_SYSTEM_FAILURE                      -1050    /*  There is a system error */
#define EMF_ERROR_MAIL_MAX_COUNT                      -1052    /*  The mailbox is full */
#define EMF_ERROR_ACCOUNT_MAX_COUNT                   -1053    /*  There is too many account */
#define EMF_ERROR_MAIL_MEMORY_FULL                    -1054    /*  There is no more storage */
#define EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER            -1055    /*  The expected mail is not found in server */
#define EMF_ERROR_LOAD_ENGINE_FAILURE                 -1056    /*  loading engine failed */
#define EMF_ERROR_CLOSE_FAILURE                       -1057    /*  engine is still used */
#define EMF_ERROR_GCONF_FAILURE                       -1058    /*  The error occurred on accessing Gconf */
#define EMF_ERROR_NO_SUCH_HOST                        -1059    /*  no such host was found */
#define EMF_ERROR_EVENT_QUEUE_FULL                    -1060    /*  event queue is full */
#define EMF_ERROR_EVENT_QUEUE_EMPTY                   -1061    /*  event queue is empty */
#define EMF_ERROR_NO_RECIPIENT                        -1062    /*  no recipients information was found */
#define EMF_ERROR_SMTP_SEND_FAILURE                   -1063    /*  SMTP send failed */
#define EMF_ERROR_MAILBOX_OPEN_FAILURE                -1064    /*  accessing mailbox failed */
#define EMF_ERROR_RETRIEVE_HEADER_DATA_FAILURE        -1065    /*  retrieving header failed */
#define EMF_ERROR_XML_PARSER_FAILURE                  -1066    /*  XML parsing failed */
#define EMF_ERROR_SESSION_NOT_FOUND                   -1067    /*  no matched session was found */
#define EMF_ERROR_INVALID_STREAM                      -1068
#define EMF_ERROR_AUTH_REQUIRED                       -1069    /*  SMTP Authentication needed */
#define EMF_ERROR_POP3_DELE_FAILURE                   -1100
#define EMF_ERROR_POP3_UIDL_FAILURE                   -1101
#define EMF_ERROR_POP3_LIST_FAILURE                   -1102
#define EMF_ERROR_IMAP4_STORE_FAILURE                 -1200
#define EMF_ERROR_IMAP4_EXPUNGE_FAILURE               -1201
#define EMF_ERROR_IMAP4_FETCH_UID_FAILURE             -1202
#define EMF_ERROR_IMAP4_FETCH_SIZE_FAILURE            -1203
#define EMF_ERROR_IMAP4_IDLE_FAILURE                  -1204    /* IDLE faile */
#define EMF_ERROR_NO_SIM_INSERTED                     -1205
#define EMF_ERROR_FLIGHT_MODE                         -1206
#define EMF_SUCCESS_VALIDATE_ACCOUNT                  -1207
#define EMF_ERROR_VALIDATE_ACCOUNT                    -1208
#define EMF_ERROR_NO_MMC_INSERTED                     -1209
#define EMF_ERROR_ACTIVE_SYNC_NOTI_FAILURE            -1300
#define EMF_ERROR_HANDLE_NOT_FOUND                    -1301
#define EMF_ERROR_NULL_VALUE                          -1302
#define EMF_ERROR_FAILED_BY_SECURITY_POLICY           -1303
#define EMF_ERROR_CANNOT_NEGOTIATE_TLS                -1400    /*  "Cannot negotiate TLS" */
#define EMF_ERROR_STARTLS                             -1401    /*  "STARTLS" */
#define EMF_ERROR_IPC_CRASH                           -1500
#define EMF_ERROR_IPC_CONNECTION_FAILURE              -1501
#define EMF_ERROR_IPC_SOCKET_FAILURE                  -1502
#define EMF_ERROR_LOGIN_ALLOWED_EVERY_15_MINS         -1600    /*  "login allowed only every 15 minutes" */
#define EMF_ERROR_TOO_MANY_LOGIN_FAILURE              -1601    /*  "Too many login failure" */
#define EMF_ERROR_ON_PARSING                          -1700
#define EMF_ERROR_NETWORK_NOT_AVAILABLE               -1800    /* WIFI not availble*/
#define EMF_ERROR_CANNOT_STOP_THREAD                  -2000
#define EMF_ERROR_UNKNOWN                             -8000    /*  unknown erro */



/*****************************************************************************/
/*  For Active Sync                                                          */
/*****************************************************************************/

#define VCONFKEY_EMAIL_SERVICE_ACTIVE_SYNC_HANDLE "db/email_handle/active_sync_handle"
#define EMF_ACTIVE_SYNC_NOTI                      "User.Email.ActiveSync"

typedef enum
{
	ACTIVE_SYNC_NOTI_SEND_MAIL,                 /*  a sending notification to ASE (active sync engine */
	ACTIVE_SYNC_NOTI_SEND_SAVED,                /*  a sending notification to ASE (active sync engine), All saved mails should be sent */
	ACTIVE_SYNC_NOTI_SEND_REPORT,               /*  a sending notification to ASE (active sync engine), report should be sen */
	ACTIVE_SYNC_NOTI_SYNC_HEADER,               /*  a sync header - download mails from server. */
                                                /*  It is depended on account/s flag1 field whether it excutes downloading header only or downloading header + body */
                                                /*  downloading option : 0 is subject only, 1 is text body, 2 is normal */
	ACTIVE_SYNC_NOTI_DOWNLOAD_BODY,             /*  a downloading body notification to AS */
	ACTIVE_SYNC_NOTI_DOWNLOAD_ATTACHMENT,       /*  a downloading attachment notification to AS */
	ACTIVE_SYNC_NOTI_VALIDATE_ACCOUNT,          /*  a account validating notification to AS */
	ACTIVE_SYNC_NOTI_CANCEL_JOB,                /*  a cancling job notification to AS */
	ACTIVE_SYNC_NOTI_SEARCH_ON_SERVER,          /*  a searching on server notification to AS */
}	eactivesync_noti_t;

typedef union
{
	struct _send_mail 
	{
		int           handle;
		int           account_id;
		char         *mailbox_name;
		int           mail_id;
		emf_option_t  options;
	} send_mail;

	struct _send_mail_saved 
	{/*  not defined ye */
		int           handle;
		int           account_id;
	} send_mail_saved;

	struct _send_report 
	{/*  not defined ye */
		int           handle;
		int           account_id;
	} send_report;

	struct _sync_header 
	{
		int           handle;
		int           account_id;
		char         *mailbox_name;
	} sync_header;

	struct _download_body
	{
		int           handle;
		int           account_id;
		int           mail_id;
		int           with_attachment;      /*  0: without attachments, 1: with attachment */
	} download_body;

	struct _download_attachment 
	{
		int           handle;
		int           account_id;
		int           mail_id;
		char         *attachment_order;
	} download_attachment;

	struct _cancel_job
	{
		int           account_id;
		int           handle;               /*  job handle to be canceled. this value is issued by email-service (actually in Emf_Mapi_xxx() */
	} cancel_job;

	struct _validate_account
	{/*  not defined yet */
		int           handle;
		int           account_id;
	} validate_account;
	
	struct _find_mail_on_server
	{
		int           handle;
		int           account_id;
		char         *mailbox_name;
		int           search_type;
		char         *search_value;
	} find_mail_on_server;

} ASNotiData;


/*  types for noti string */
typedef enum 
{
	EMF_CONVERT_STRUCT_TYPE_MAIL_LIST_ITEM,      /** specifies emf_mail_list_t */
} emf_convert_struct_type_e;

#ifdef __cplusplus
}
#endif

/**
* @} @}
*/

#endif /* __EMF_LIB_H__ */
