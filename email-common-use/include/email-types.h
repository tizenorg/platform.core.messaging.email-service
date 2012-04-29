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


#ifndef __EMAIL_TYPES_H__
#define __EMAIL_TYPES_H__

/**
* @defgroup EMAIL_SERVICE Email Service
* @{
*/

/**
* @ingroup EMAIL_SERVICE
* @defgroup EMAIL_TYPES Email Types
* @{
*/
/**
 * This file defines structures and enums of Email Framework.
 * @file	email-types.h
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
#include "email-errors.h"


/*****************************************************************************/
/*  Macros                                                                   */
/*****************************************************************************/

#define MAILBOX_NAME_LENGTH             256
#define MAX_EMAIL_ADDRESS_LENGTH        320                                    /*  64(user name) + 1(@) + 255(host name */
#define MAX_DATETIME_STRING_LENGTH      20
#define MAX_PREVIEW_TEXT_LENGTH         512
#define STRING_LENGTH_FOR_DISPLAY       100
#define MEETING_REQ_OBJECT_ID_LENGTH    256

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

#define DAEMON_EXECUTABLE_PATH          "/usr/bin/email-service"

#ifndef EXPORT_API
#define EXPORT_API                      __attribute__((visibility("default")))
#endif

#ifndef DEPRECATED
#define DEPRECATED                      __attribute__((deprecated))
#endif


/* __FEATURE_LOCAL_ACTIVITY__ supported
#define BULK_OPERATION_COUNT            50
#define ALL_ACTIVITIES                  0
*/

/*****************************************************************************/
/*  Enumerations                                                             */
/*****************************************************************************/
enum {
	// Account
	_EMAIL_API_ADD_ACCOUNT                             = 0x01000000,
	_EMAIL_API_DELETE_ACCOUNT                          = 0x01000001,
	_EMAIL_API_UPDATE_ACCOUNT                          = 0x01000002,
	_EMAIL_API_GET_ACCOUNT                             = 0x01000003,
	_EMAIL_API_GET_ACCOUNT_LIST                        = 0x01000005,
	_EMAIL_API_GET_MAILBOX_COUNT                       = 0x01000007,
	_EMAIL_API_VALIDATE_ACCOUNT                        = 0x01000008,
	_EMAIL_API_ADD_ACCOUNT_WITH_VALIDATION             = 0x01000009,
	_EMAIL_API_BACKUP_ACCOUNTS                         = 0x0100000A,
	_EMAIL_API_RESTORE_ACCOUNTS                        = 0x0100000B,
	_EMAIL_API_GET_PASSWORD_LENGTH_OF_ACCOUNT          = 0x0100000C,

	// Message
	_EMAIL_API_DELETE_MAIL                             = 0x01100002,
	_EMAIL_API_DELETE_ALL_MAIL                         = 0x01100004,
	_EMAIL_API_GET_MAILBOX_LIST                        = 0x01100006,
	_EMAIL_API_GET_SUBMAILBOX_LIST                     = 0x01100007,
	_EMAIL_API_CLEAR_DATA                              = 0x01100009,
	_EMAIL_API_MOVE_MAIL                               = 0x0110000A,
	_EMAIL_API_MOVE_ALL_MAIL                           = 0x0110000B,
	_EMAIL_API_ADD_ATTACHMENT                          = 0x0110000C,
	_EMAIL_API_GET_ATTACHMENT                          = 0x0110000D,
	_EMAIL_API_DELETE_ATTACHMENT                       = 0x0110000E,
	_EMAIL_API_MODIFY_MAIL_FLAG                        = 0x0110000F,
	_EMAIL_API_MODIFY_MAIL_EXTRA_FLAG                  = 0x01100011,
	_EMAIL_API_SET_FLAGS_FIELD                         = 0x01100016,
	_EMAIL_API_ADD_MAIL                                = 0x01100017,
	_EMAIL_API_UPDATE_MAIL                             = 0x01100018,
	_EMAIL_API_ADD_READ_RECEIPT                        = 0x01100019,

	// Thread
	_EMAIL_API_MOVE_THREAD_TO_MAILBOX                  = 0x01110000,
	_EMAIL_API_DELETE_THREAD                           = 0x01110001,
	_EMAIL_API_MODIFY_SEEN_FLAG_OF_THREAD              = 0x01110002,

	// Mailbox
	_EMAIL_API_ADD_MAILBOX                             = 0x01200000,
	_EMAIL_API_DELETE_MAILBOX                          = 0x01200001,
	_EMAIL_API_UPDATE_MAILBOX                          = 0x01200002,
	_EMAIL_API_SET_MAIL_SLOT_SIZE                      = 0x01200007,

	// Network
	_EMAIL_API_SEND_MAIL                               = 0x01300000,
	_EMAIL_API_SYNC_HEADER                             = 0x01300001,
	_EMAIL_API_DOWNLOAD_BODY                           = 0x01300002,
	_EMAIL_API_DOWNLOAD_ATTACHMENT                     = 0x01300003,
	_EMAIL_API_NETWORK_GET_STATUS                      = 0x01300004,
	_EMAIL_API_SEND_SAVED                              = 0x01300005,
	_EMAIL_API_DELETE_EMAIL                            = 0x01300007,
	_EMAIL_API_DELETE_EMAIL_ALL                        = 0x01300008,
	_EMAIL_API_GET_IMAP_MAILBOX_LIST                   = 0x01300015,
	_EMAIL_API_SEND_MAIL_CANCEL_JOB                    = 0x01300017,
	_EMAIL_API_SEARCH_MAIL_ON_SERVER                   = 0x01300019,

	// Rule
	_EMAIL_API_ADD_RULE                                = 0x01400000,
	_EMAIL_API_GET_RULE                                = 0x01400001,
	_EMAIL_API_GET_RULE_LIST                           = 0x01400002,
	_EMAIL_API_FIND_RULE                               = 0x01400003,
	_EMAIL_API_DELETE_RULE                             = 0x01400004,
	_EMAIL_API_UPDATE_RULE                             = 0x01400005,
	_EMAIL_API_CANCEL_JOB                              = 0x01400006,
	_EMAIL_API_GET_PENDING_JOB                         = 0x01400007,
	_EMAIL_API_SEND_RETRY                              = 0x01400008,
	_EMAIL_API_UPDATE_ACTIVITY                         = 0x01400009,
	_EMAIL_API_SYNC_LOCAL_ACTIVITY                     = 0x0140000A,
	_EMAIL_API_PRINT_RECEIVING_EVENT_QUEUE             = 0x0140000B,

	// Etc
	_EMAIL_API_PING_SERVICE                            = 0x01500000,
	_EMAIL_API_UPDATE_NOTIFICATION_BAR_FOR_UNREAD_MAIL = 0x01500001,
};

enum
{
	EMF_DELETE_LOCALLY = 0,                      /**< Specifies Mail Delete local only */
	EMF_DELETE_LOCAL_AND_SERVER,                 /**< Specifies Mail Delete local & server */
	EMF_DELETE_FOR_SEND_THREAD,                  /**< Created to check which activity to delete in send thread */
};

typedef enum
{
	NOTI_MAIL_ADD                            = 10000,
	NOTI_MAIL_DELETE                         = 10001,
	NOTI_MAIL_DELETE_ALL                     = 10002,
	NOTI_MAIL_DELETE_WITH_ACCOUNT            = 10003,
	NOTI_MAIL_DELETE_FAIL                    = 10007,
	NOTI_MAIL_DELETE_FINISH                  = 10008,

	NOTI_MAIL_UPDATE                         = 10004,
	NOTI_MAIL_FIELD_UPDATE                   = 10020,

	NOTI_MAIL_MOVE                           = 10005,
	NOTI_MAIL_MOVE_FAIL                      = 10010,
	NOTI_MAIL_MOVE_FINISH                    = 10006,

	NOTI_THREAD_MOVE                         = 11000,
	NOTI_THREAD_DELETE                       = 11001,
	NOTI_THREAD_MODIFY_SEEN_FLAG             = 11002,

	NOTI_ACCOUNT_ADD                         = 20000,
	NOTI_ACCOUNT_DELETE                      = 20001,
	NOTI_ACCOUNT_DELETE_FAIL                 = 20003,
	NOTI_ACCOUNT_UPDATE                      = 20002,
	NOTI_ACCOUNT_UPDATE_SYNC_STATUS          = 20010,


	NOTI_MAILBOX_ADD                         = 40000,
	NOTI_MAILBOX_DELETE                      = 40001,
	NOTI_MAILBOX_UPDATE                      = 40002,
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

/**
 * This enumeration specifies the meeting request type
 */
typedef enum
{
	EMF_MAIL_TYPE_NORMAL                     = 0, /**< NOT a meeting request mail. A Normal mail */
	EMF_MAIL_TYPE_MEETING_REQUEST            = 1, /**< a meeting request mail from a serve */
	EMF_MAIL_TYPE_MEETING_RESPONSE           = 2, /**< a response mail about meeting reques */
	EMF_MAIL_TYPE_MEETING_ORIGINATINGREQUEST = 3  /**< a originating mail about meeting reques */
} emf_mail_type_t;

/**
 * This enumeration specifies the meeting response type
 */
typedef enum
{
	EMF_MEETING_RESPONSE_NONE                = 0, /**< NOT response */
	EMF_MEETING_RESPONSE_ACCEPT              = 1, /**< The response is acceptance */
	EMF_MEETING_RESPONSE_TENTATIVE           = 2, /**< The response is tentative */
	EMF_MEETING_RESPONSE_DECLINE             = 3, /**< The response is decline */
	EMF_MEETING_RESPONSE_REQUEST             = 4, /**< The response is request */
	EMF_MEETING_RESPONSE_CANCEL              = 5, /**< The response is cancelation */
} emf_meeting_response_t;

typedef enum
{
	EMF_ACTION_SEND_MAIL                     =  0,
	EMF_ACTION_SYNC_HEADER                   =  1,
	EMF_ACTION_DOWNLOAD_BODY                 =  2,
	EMF_ACTION_DOWNLOAD_ATTACHMENT           =  3,
	EMF_ACTION_DELETE_MAIL                   =  4,
	EMF_ACTION_SEARCH_MAIL                   =  5,
	EMF_ACTION_SAVE_MAIL                     =  6,
	EMF_ACTION_SYNC_MAIL_FLAG_TO_SERVER      =  7,
	EMF_ACTION_SYNC_FLAGS_FIELD_TO_SERVER    =  8,
	EMF_ACTION_MOVE_MAIL                     =  9,
	EMF_ACTION_CREATE_MAILBOX                = 10,
	EMF_ACTION_DELETE_MAILBOX                = 11,
	EMF_ACTION_SYNC_HEADER_OMA               = 12,
	EMF_ACTION_VALIDATE_ACCOUNT              = 13,
	EMF_ACTION_VALIDATE_AND_CREATE_ACCOUNT   = 14,
	EMF_ACTION_VALIDATE_AND_UPDATE_ACCOUNT   = 15,
	EMF_ACTION_ACTIVATE_PDP                  = 20,
	EMF_ACTION_DEACTIVATE_PDP                = 21,
	EMF_ACTION_UPDATE_MAIL                   = 30,
	EMF_ACTION_SET_MAIL_SLOT_SIZE            = 31,
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
	EMF_EVENT_NONE                        =  0,
	EMF_EVENT_SYNC_HEADER                 =  1,          /*  synchronize mail headers with server (network used) */
	EMF_EVENT_DOWNLOAD_BODY               =  2,          /*  download mail body from server (network used)*/
	EMF_EVENT_DOWNLOAD_ATTACHMENT         =  3,          /*  download mail attachment from server (network used) */
	EMF_EVENT_SEND_MAIL                   =  4,          /*  send a mail (network used) */
	EMF_EVENT_SEND_MAIL_SAVED             =  5,          /*  send all mails in 'outbox' box (network used) */
	EMF_EVENT_SYNC_IMAP_MAILBOX           =  6,          /*  download imap mailboxes from server (network used) */
	EMF_EVENT_DELETE_MAIL                 =  7,          /*  delete mails (network unused) */
	EMF_EVENT_DELETE_MAIL_ALL             =  8,          /*  delete all mails (network unused) */
	EMF_EVENT_SYNC_MAIL_FLAG_TO_SERVER    =  9,          /*  sync mail flag to server */
	EMF_EVENT_SYNC_FLAGS_FIELD_TO_SERVER  = 10,          /*  sync a field of flags to server */
	EMF_EVENT_SAVE_MAIL                   = 11,
	EMF_EVENT_MOVE_MAIL                   = 12,          /*  move mails to specific mailbox on server */
	EMF_EVENT_CREATE_MAILBOX              = 13,
	EMF_EVENT_UPDATE_MAILBOX              = 14,
	EMF_EVENT_DELETE_MAILBOX              = 15,
	EMF_EVENT_ISSUE_IDLE                  = 16,
	EMF_EVENT_SYNC_HEADER_OMA             = 17,
	EMF_EVENT_VALIDATE_ACCOUNT            = 18,
	EMF_EVENT_VALIDATE_AND_CREATE_ACCOUNT = 19,
	EMF_EVENT_VALIDATE_AND_UPDATE_ACCOUNT = 20,

	EMF_EVENT_ADD_MAIL                    = 10001,
	EMF_EVENT_UPDATE_MAIL_OLD             = 10002,
	EMF_EVENT_UPDATE_MAIL                 = 10003,
	EMF_EVENT_SET_MAIL_SLOT_SIZE          = 20000,

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

typedef enum {
	EMF_FLAG_NONE                  = 0,
	EMF_FLAG_FLAGED                = 1,
	EMF_FLAG_TASK_STATUS_CLEAR     = 2,
	EMF_FLAG_TASK_STATUS_ACTIVE    = 3,
	EMF_FLAG_TASK_STATUS_COMPLETE  = 4,
} emf_flagged_type;

typedef enum {
	EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO       =  1,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_UID              =  2,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_BCC              =  7,  /* string type */
	EMAIL_SEARCH_FILTER_TYPE_CC               =  9,  /* string type */
	EMAIL_SEARCH_FILTER_TYPE_FROM             = 10,  /* string type */
	EMAIL_SEARCH_FILTER_TYPE_KEYWORD          = 11,  /* string type */
	EMAIL_SEARCH_FILTER_TYPE_SUBJECT          = 13,  /* string type */
	EMAIL_SEARCH_FILTER_TYPE_TO               = 15,  /* string type */
	EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER      = 16,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER     = 17,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE = 20,  /* time type */
	EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON     = 21,  /* time type */
	EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE  = 22,  /* time type */
	EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED   = 26,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED    = 28,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT      = 30,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED     = 32,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT     = 34,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN       = 36,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID       = 43,  /* string type */
} email_search_filter_type;

typedef enum {
	EMAIL_DRM_TYPE_NONE                       = 0,
	EMAIL_DRM_TYPE_OBJECT                     = 1,
	EMAIL_DRM_TYPE_RIGHT                      = 2,
	EMAIL_DRM_TYPE_DCF                        = 3
} email_drm_type;

typedef enum {
	EMAIL_DRM_METHOD_NONE                     = 0,
	EMAIL_DRM_METHOD_FL                       = 1,
	EMAIL_DRM_METHOD_CD                       = 2,
	EMAIL_DRM_METHOD_SSD                      = 3,
	EMAIL_DRM_METHOD_SD                       = 4
} email_drm_method;

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
	int     mail_id;                 /**< Specifies the Mail ID.*/
	int     account_id;              /**< Specifies the Account ID.*/
	char   *mailbox_name;            /**< Specifies the Mailbox Name.*/
	int     mailbox_type;            /**< Specifies the mailbox type of the mail. */
	char   *subject;                 /**< Specifies the subject.*/
	time_t  date_time;               /**< Specifies the Date time.*/
	int     server_mail_status;      /**< Specifies the Whether sever mail or not.*/
	char   *server_mailbox_name;     /**< Specifies the server mailbox.*/
	char   *server_mail_id;          /**< Specifies the Server Mail ID.*/
	char   *message_id;              /**< Specifies the message id */
	char   *full_address_from;       /**< Specifies the From Addr.*/
	char   *full_address_reply;      /**< Specifies the Reply to addr */
	char   *full_address_to;         /**< Specifies the To addr.*/
	char   *full_address_cc;         /**< Specifies the CC addr.*/
	char   *full_address_bcc;        /**< Specifies the BCC addr*/
	char   *full_address_return;     /**< Specifies the return Path*/
	char   *email_address_sender;    /**< Specifies the email address of sender.*/
	char   *email_address_recipient; /**< Specifies the email address of recipients.*/
	char   *alias_sender;            /**< Specifies the alias of sender. */
	char   *alias_recipient;         /**< Specifies the alias of recipients. */
	int     body_download_status;    /**< Specifies the Text donwloaded or not.*/
	char   *file_path_plain;         /**< Specifies the path of text mail body.*/
	char   *file_path_html;          /**< Specifies the path of HTML mail body.*/
	int     mail_size;               /**< Specifies the Mail Size.*/
	char    flags_seen_field;        /**< Specifies the seen flags*/
	char    flags_deleted_field;     /**< Specifies the deleted flags*/
	char    flags_flagged_field;     /**< Specifies the flagged flags*/
	char    flags_answered_field;    /**< Specifies the answered flags*/
	char    flags_recent_field;      /**< Specifies the recent flags*/
	char    flags_draft_field;       /**< Specifies the draft flags*/
	char    flags_forwarded_field;   /**< Specifies the forwarded flags*/
	int     DRM_status;              /**< Has the mail DRM content? (1 : true, 0 : false) */
	int     priority;                /**< Specifies the priority of the mail.*/
	int     save_status;             /**< Specifies the save status*/
	int     lock_status;             /**< Specifies the Locked*/
	int     report_status;           /**< Specifies the Mail Report.*/
	int     attachment_count;        /**< Specifies the attachment count. */
	int     inline_content_count;    /**< Specifies the inline content count. */
	int     thread_id;               /**< Specifies the thread id for thread view. */
	int     thread_item_count;       /**< Specifies the item count of specific thread. */
	char   *preview_text;            /**< Specifies the preview body. */
	int     meeting_request_status;  /**< Specifies the status of meeting request. */
}emf_mail_data_t;

/**
 * This structure is contains information for displaying mail list.
 */
typedef struct
{
	int    mail_id;                                          /**< Specifies the Mail ID.*/
	int    account_id;                                       /**< Specifies the Account ID.*/
	char   mailbox_name[STRING_LENGTH_FOR_DISPLAY];          /**< Specifies the Mailbox Name.*/
	char   from[STRING_LENGTH_FOR_DISPLAY];                  /**< Specifies the from display name.*/
	char   from_email_address[MAX_EMAIL_ADDRESS_LENGTH];     /**< Specifies the from email address.*/
	char   recipients[STRING_LENGTH_FOR_DISPLAY];            /**< Specifies the recipients display name.*/
	char   subject[STRING_LENGTH_FOR_DISPLAY];               /**< Specifies the subject.*/
	int    is_text_downloaded;                               /**< Specifies the Text donwloaded or not.*/
	time_t date_time;                                        /**< Specifies the Date time.*/
	char   flags_seen_field;                                 /**< Specifies the seen flags*/
	char   flags_deleted_field;                              /**< Specifies the deleted flags*/
	char   flags_flagged_field;                              /**< Specifies the flagged flags*/
	char   flags_answered_field;                             /**< Specifies the answered flags*/
	char   flags_recent_field;                               /**< Specifies the recent flags*/
	char   flags_draft_field;                                /**< Specifies the draft flags*/
	char   flags_forwarded_field;                            /**< Specifies the forwarded flags*/
	int    priority;                                         /**< Specifies the priority of Mails.*/
	int    save_status;                                      /**< Specifies the save status*/
	int    is_locked;                                        /**< Specifies the Locked*/
	int    is_report_mail;                                   /**< Specifies the Mail Report.*/
	int    recipients_count;                                 /**< Specifies the number of to Recipients*/
	int    has_attachment;                                   /**< the mail has attachments or not[ 0: none, 1: over one] */
	int    has_drm_attachment;                               /**< the mail has drm attachment or not*/
	char   previewBodyText[MAX_PREVIEW_TEXT_LENGTH];         /**< text for preview body*/
	int    thread_id;                                        /**< thread id for thread view*/
	int    thread_item_count;                                /**< item count of specific thread */
	int    is_meeting_request;                               /**< Whether the mail is a meeting request or not */
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
 * This structure is used to save the information of attachment.
 */
typedef struct st_emf_attachment_info
{
	int                            inline_content;
	int                            attachment_id;        /**< Specifies the attachment ID*/
	char                          *name;                 /**< Specifies the attachment name.*/
	int                            size;                 /**< Specifies the attachment size.*/
	int                            downloaded;           /**< Specifies the download of attachment.*/
	char                          *savename;             /**< Specifies the absolute path of attachment.*/
	int                            drm;                  /**< Specifies the drm type.*/
	char                          *attachment_mime_type; /**< Specifies the context mime type.*/
	struct st_emf_attachment_info *next;                 /**< Specifies the pointer of next attachment.*/
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
	char *attachment_mime_type; /**< Specifies the context mime type.*/
} emf_attachment_data_t;

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
	char                   *global_object_id;                               /**< Specifies the object id. */
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


typedef struct _email_search_filter_t {
	email_search_filter_type search_filter_type; /* type of search filter */
	union {
		int            integer_type_key_value;
		struct tm      time_type_key_value;
		char          *string_type_key_value;
	} search_filter_key_value;
} email_search_filter_t;


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

	struct _search_mail_on_server
	{
		int                    handle;
		int                    account_id;
		char                  *mailbox_name;
		email_search_filter_t *search_filter_list;
		int                    search_filter_count;
	} search_mail_on_server;

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

