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


#ifndef __EMAIL_TYPES_H__
#define __EMAIL_TYPES_H__

/**
 * @file email-types.h
 * @brief This file is the header file of Email Framework library.
 */

/**
 * @addtogroup EMAIL_SERVICE_FRAMEWORK
 * @{
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

#define MAILBOX_NAME_LENGTH               256
#define MAX_EMAIL_ADDRESS_LENGTH          254                                    /* RFC5322, RFC3696 */
#define MAX_USER_NAME_LENGTH              64
#define MAX_DATETIME_STRING_LENGTH        20
#define MAX_PREVIEW_TEXT_LENGTH           1000
#define STRING_LENGTH_FOR_DISPLAY         100
#define MEETING_REQ_OBJECT_ID_LENGTH      256
#define EMAIL_NO_LIMITED_RETRY_COUNT      -1

#define ALL_ACCOUNT                       0
#define NEW_ACCOUNT_ID                    0xFFFFFFFE
#define ALL_MAIL                          -1

#define EMAIL_SEARCH_FILTER_NONE          0x00
#define EMAIL_SEARCH_FILTER_SUBJECT       0x01
#define EMAIL_SEARCH_FILTER_SENDER        0x02
#define EMAIL_SEARCH_FILTER_RECIPIENT     0x04
#define EMAIL_SEARCH_FILTER_ALL           0x07                                   /*  EMAIL_SEARCH_FILTER_SUBJECT + EMAIL_SEARCH_FILTER_SENDER + EMAIL_SEARCH_FILTER_RECIPIEN */

#define EMAIL_SUCCESS                     0                                      /*  we need to modify the success return value */

/** @brief Definition for default values without account name.
 *  @since_tizen 2.3 */
#define EMAIL_ACC_GET_OPT_DEFAULT         0x01

/** @brief Definition for account name.
 *  @since_tizen 2.3 */
#define EMAIL_ACC_GET_OPT_ACCOUNT_NAME    0x02

/** @brief Definition for account with password.
 *  @since_tizen 2.3 */
#define EMAIL_ACC_GET_OPT_PASSWORD        0x04

/** @brief Definition for account with options: #email_option_t.
 *  @since_tizen 2.3 */
#define EMAIL_ACC_GET_OPT_OPTIONS         0x08

/** @brief Definition for account with all data of account.
 *  @since_tizen 2.3 */
#define EMAIL_ACC_GET_OPT_FULL_DATA       0xFF


#define GET_FULL_DATA                     (EMAIL_ACC_GET_OPT_FULL_DATA)
#define GET_FULL_DATA_WITHOUT_PASSWORD    (EMAIL_ACC_GET_OPT_DEFAULT | EMAIL_ACC_GET_OPT_ACCOUNT_NAME | EMAIL_ACC_GET_OPT_OPTIONS )
#define WITHOUT_OPTION                    (EMAIL_ACC_GET_OPT_DEFAULT | EMAIL_ACC_GET_OPT_ACCOUNT_NAME )
#define ONLY_OPTION                       (EMAIL_ACC_GET_OPT_OPTIONS)


/** @brief Definition for the function 'email_activate_pdp'.
 *  @since_tizen 2.3 */
#define THREAD_TYPE_RECEIVING             0

/** @brief Definition for the function 'email_activate_pdp'.
 *  @since_tizen 2.3 */
#define THREAD_TYPE_SENDING               1


/** @brief Definition for the default IMAP port.
 *  @since_tizen 2.3 */
#define EMAIL_IMAP_PORT                   143

/** @brief Definition for the default POP3 port.
 *  @since_tizen 2.3 */
#define EMAIL_POP3_PORT                   110

/** @brief Definition for the default SMTP port.
 *  @since_tizen 2.3 */
#define EMAIL_SMTP_PORT                   25

/** @brief Definition for the default IMAP SSL port.
 *  @since_tizen 2.3 */
#define EMAIL_IMAPS_PORT                  993

/** @brief Definition for the default POP3 SSL port.
 *  @since_tizen 2.3 */
#define EMAIL_POP3S_PORT                  995

/** @brief Definition for the default SMTP SSL port.
 *  @since_tizen 2.3 */
#define EMAIL_SMTPS_PORT                  465

/** @brief Definition for the MAX account.
 *  @since_tizen 2.3 */
#define EMAIL_ACCOUNT_MAX                 10

/** @brief Definition for the name of inbox.
 *  @since_tizen 2.3 */
#define EMAIL_INBOX_NAME                  "INBOX"

/** @brief Definition for the name of draftbox.
 * @since_tizen 2.3 */
#define EMAIL_DRAFTBOX_NAME               "DRAFTBOX"

/** @brief Definition for the name of outbox.
 * @since_tizen 2.3 */
#define EMAIL_OUTBOX_NAME                 "OUTBOX"

/** @brief Definition for the name of sentbox.
 * @since_tizen 2.3 */
#define EMAIL_SENTBOX_NAME                "SENTBOX"

/** @brief Definition for the name of trash.
 * @since_tizen 2.3 */
#define EMAIL_TRASH_NAME                  "TRASH"

/** @brief Definition for the name of spambox.
 * @since_tizen 2.3 */
#define EMAIL_SPAMBOX_NAME                "SPAMBOX"

/** @brief Definition for the display name of inbox.
 * @since_tizen 2.3 */
#define EMAIL_INBOX_DISPLAY_NAME          "Inbox"

/** @brief Definition for the display name of draftbox.
 * @since_tizen 2.3 */
#define EMAIL_DRAFTBOX_DISPLAY_NAME       "Draftbox"

/** @brief Definition for the display name of outbox.
 * @since_tizen 2.3 */
#define EMAIL_OUTBOX_DISPLAY_NAME         "Outbox"

/** @brief Definition for the display name of sentbox.
 * @since_tizen 2.3 */
#define EMAIL_SENTBOX_DISPLAY_NAME        "Sentbox"

/** @brief Definition for the display name of trash.
 * @since_tizen 2.3 */
#define EMAIL_TRASH_DISPLAY_NAME          "Trash"

/** @brief Definition for the display name of spambox.
 * @since_tizen 2.3 */
#define EMAIL_SPAMBOX_DISPLAY_NAME        "Spambox"


/** @brief Definition for the name of search result mailbox.
 * @since_tizen 2.3 */
#define EMAIL_SEARCH_RESULT_MAILBOX_NAME  "_`S1!E2@A3#R4$C5^H6&R7*E8(S9)U0-L=T_"

#define SYNC_STATUS_FINISHED              0                                      /* BIN 00000000 */
#define SYNC_STATUS_SYNCING               1                                      /* BIN 00000001 */
#define SYNC_STATUS_HAVE_NEW_MAILS        2                                      /* BIN 00000010 */

#define UNKNOWN_CHARSET_PLAIN_TEXT_FILE   "unknown"
#define UNKNOWN_CHARSET_HTML_TEXT_FILE    "unknown.htm"

#define FAILURE                           -1
#define SUCCESS                           0

#define DAEMON_EXECUTABLE_PATH            "/usr/bin/email-service"

#ifndef EXPORT_API
#define EXPORT_API                        __attribute__((visibility("default")))
#endif

#ifndef DEPRECATED
#define DEPRECATED                        __attribute__((deprecated))
#endif

#define VCONF_VIP_NOTI_RINGTONE_PATH                 "db/private/email-service/noti_ringtone_path"
#define VCONF_VIP_NOTI_REP_TYPE                      "db/private/email-service/noti_rep_type"
#define VCONF_VIP_NOTI_NOTIFICATION_TICKER           "db/private/email-service/noti_notification_ticker"
#define VCONF_VIP_NOTI_DISPLAY_CONTENT_TICKER        "db/private/email-service/noti_display_content_ticker"
#define VCONF_VIP_NOTI_BADGE_TICKER                  "db/private/email-service/noti_badge_ticker"
#define VCONF_VIP_NOTI_VIBRATION_STATUS_BOOL         "db/private/email-service/noti_vip_vibration_status"
#define VCONF_VIP_NOTI_USE_DEFAULT_RINGTONE_BOOL     "db/private/email-service/noti_vip_use_default_ringtone"
#define VCONF_EMAIL_NOTI_VIBRATION_STATUS_BOOL       "db/private/email-service/noti_vibration_status"
#define VCONF_EMAIL_NOTI_USE_DEFAULT_RINGTONE_BOOL   "db/private/email-service/noti_use_default_ringtone"


#define EMAIL_PEAK_DAYS_SUNDAY            0x01
#define EMAIL_PEAK_DAYS_MONDAY            0x02
#define EMAIL_PEAK_DAYS_TUEDAY            0x04
#define EMAIL_PEAK_DAYS_WEDDAY            0x08
#define EMAIL_PEAK_DAYS_THUDAY            0x10
#define EMAIL_PEAK_DAYS_FRIDAY            0x20
#define EMAIL_PEAK_DAYS_SATDAY            0x40

#define PRIORITY_SENDER_TAG_ID            -30000

/*****************************************************************************/
/*  Enumerations                                                             */
/*****************************************************************************/

typedef enum
{
    EMAIL_DELETE_LOCALLY                     = 0,  /**< Delete mail locally only */
    EMAIL_DELETE_LOCAL_AND_SERVER,                 /**< Delete mail locally and on server */
    EMAIL_DELETE_FOR_SEND_THREAD,                  /**< Check which activity to delete in send thread */
    EMAIL_DELETE_FROM_SERVER,                      /**< Delete mail on server */
    EMAIL_DELETE_MAIL_AND_MEETING_FOR_EAS,         /**< Delete mails and meetings on an EAS account */
} email_delete_option_t;

/**
 * @brief Enumeration for the notification of changes on storage.
 * @since_tizen 2.3
 */
typedef enum
{
    NOTI_MAIL_ADD                            = 10000,    /**< A mail is added */
    NOTI_MAIL_DELETE                         = 10001,    /**< Some mails are removed */
    NOTI_MAIL_DELETE_ALL                     = 10002,    /**< All mails in mailbox are removed */
    NOTI_MAIL_DELETE_WITH_ACCOUNT            = 10003,    /**< All mails of an account removed */
    NOTI_MAIL_DELETE_FAIL                    = 10007,    /**< Removing mails failed */
    NOTI_MAIL_DELETE_FINISH                  = 10008,    /**< Removing mails finished */

    NOTI_MAIL_UPDATE                         = 10004,    /**< Some fields of mail are updated */
    NOTI_MAIL_FIELD_UPDATE                   = 10020,    /**< A field of some mails is updated */

    NOTI_MAIL_MOVE                           = 10005,    /**< Some mail are moved */
    NOTI_MAIL_MOVE_FAIL                      = 10010,    /**< Moving mails failed */
    NOTI_MAIL_MOVE_FINISH                    = 10006,    /**< Moving mails finished */

    NOTI_THREAD_MOVE                         = 11000,    /**< A mail thread is moved */
    NOTI_THREAD_DELETE                       = 11001,    /**< A mail thread is removed */
    NOTI_THREAD_MODIFY_SEEN_FLAG             = 11002,    /**< Seen flag of a mail thread is modified */
    NOTI_THREAD_ID_CHANGED_BY_ADD            = 11003,    /**< The thread ID is changed by adding a new mail */
    NOTI_THREAD_ID_CHANGED_BY_MOVE_OR_DELETE = 11004,    /**< The thread ID is changed by moving or removing mails */

    NOTI_ACCOUNT_ADD                         = 20000,    /**< An account is added */
    NOTI_ACCOUNT_ADD_FINISH                  = 20005,    /**< An account is added and account reference updated */
    NOTI_ACCOUNT_ADD_FAIL                    = 20007,    /**< An account adding failed */
    NOTI_ACCOUNT_DELETE                      = 20001,    /**< An account is removed */
    NOTI_ACCOUNT_DELETE_FAIL                 = 20003,    /**< Removing an account failed */
    NOTI_ACCOUNT_UPDATE                      = 20002,    /**< An account is updated */
    NOTI_ACCOUNT_UPDATE_SYNC_STATUS          = 20010,    /**< Sync status of an account is updated */

    NOTI_MAILBOX_ADD                         = 40000,    /**< A mailbox is added */
    NOTI_MAILBOX_DELETE                      = 40001,    /**< A mailbox is removed */
    NOTI_MAILBOX_UPDATE                      = 40002,    /**< A mailbox is updated */
    NOTI_MAILBOX_FIELD_UPDATE                = 40003,    /**< Some fields of a mailbox are updated */

    NOTI_MAILBOX_RENAME                      = 40010,    /**< A mailbox is renamed */
    NOTI_MAILBOX_RENAME_FAIL                 = 40011,    /**< Renaming a mailbox failed */

    NOTI_CERTIFICATE_ADD                     = 50000,    /**< A certificate is added */
    NOTI_CERTIFICATE_DELETE                  = 50001,    /**< A certificate is removed */
    NOTI_CERTIFICATE_UPDATE                  = 50002,    /**< A certificate is updated */

    NOTI_ACCOUNT_RESTORE_START               = 60001,    /**< Start of restoring accounts */
    NOTI_ACCOUNT_RESTORE_FINISH              = 60002,    /**< Finish of restoring accounts */
    NOTI_ACCOUNT_RESTORE_FAIL                = 60003,    /**< Failure of restoring accounts */

    NOTI_RULE_ADD                            = 70000,    /**< A rule is added in DB */
    NOTI_RULE_APPLY                          = 70001,    /**< A rule is applied */
    NOTI_RULE_DELETE                         = 70002,    /**< A rule is removed in DB */
    NOTI_RULE_UPDATE                         = 70003,    /**< A rule is updated in DB */
    
    /* To be added more */
} email_noti_on_storage_event;

/**
 * @brief Enumeration for the notification of network event.
 * @since_tizen 2.3
 */
typedef enum
{
    NOTI_SEND_START                          = 1002,    /**< Sending a mail started */
    NOTI_SEND_FINISH                         = 1004,    /**< Sending a mail finished */
    NOTI_SEND_FAIL                           = 1005,    /**< Sending a mail failed */
    NOTI_SEND_CANCEL                         = 1003,    /**< Sending a mail canceled */

    NOTI_DOWNLOAD_START                      = 2000,    /**< Syncing header started */
    NOTI_DOWNLOAD_FINISH,                               /**< Syncing header finished */
    NOTI_DOWNLOAD_FAIL,                                 /**< Syncing header failed */
    NOTI_DOWNLOAD_CANCEL                     = 2004,    /**< Syncing header canceled */
    NOTI_DOWNLOAD_NEW_MAIL                   = 2003,    /**< Deprecated */

    NOTI_DOWNLOAD_BODY_START                 = 3000,    /**< Downloading mail body started */
    NOTI_DOWNLOAD_BODY_FINISH                = 3002,    /**< Downloading mail body finished */
    NOTI_DOWNLOAD_BODY_FAIL                  = 3004,    /**< Downloading mail body failed */
    NOTI_DOWNLOAD_BODY_CANCEL                = 3003,    /**< Downloading mail body canceled */
    NOTI_DOWNLOAD_MULTIPART_BODY             = 3001,    /**< Downloading multipart body is in progress */

    NOTI_DOWNLOAD_ATTACH_START               = 4000,    /**< Downloading attachment started */
    NOTI_DOWNLOAD_ATTACH_FINISH,                        /**< Downloading attachment finished */
    NOTI_DOWNLOAD_ATTACH_FAIL,                          /**< Downloading attachment failed */
    NOTI_DOWNLOAD_ATTACH_CANCEL,                        /**< Downloading attachment canceled */

    NOTI_MAIL_DELETE_ON_SERVER_FAIL          = 5000,    /**< Deleting mails on server failed */
    NOTI_MAIL_MOVE_ON_SERVER_FAIL,                      /**< Moving mails on server failed */

    NOTI_SEARCH_ON_SERVER_START              = 6000,    /**< Searching mails on server started */
    NOTI_SEARCH_ON_SERVER_FINISH             = 6001,    /**< Searching mails on server finished */
    NOTI_SEARCH_ON_SERVER_FAIL               = 6002,    /**< Searching mails on server failed */
    NOTI_SEARCH_ON_SERVER_CANCEL             = 6003,    /**< Searching mails on server canceled */

    NOTI_VALIDATE_ACCOUNT_FINISH             = 7000,    /**< Validating an account finished */
    NOTI_VALIDATE_ACCOUNT_FAIL,                         /**< Validating an account failed */
    NOTI_VALIDATE_ACCOUNT_CANCEL,                       /**< Validating an account canceled */

    NOTI_VALIDATE_AND_CREATE_ACCOUNT_FINISH  = 8000,    /**< Validating and creating an account finished */
    NOTI_VALIDATE_AND_CREATE_ACCOUNT_FAIL,              /**< Validating and creating an account failed */
    NOTI_VALIDATE_AND_CREATE_ACCOUNT_CANCEL,            /**< Validating and creating an account canceled */

    NOTI_VALIDATE_AND_UPDATE_ACCOUNT_FINISH  = 9000,    /**< Validating and updating an account finished */
    NOTI_VALIDATE_AND_UPDATE_ACCOUNT_FAIL,              /**< Validating and updating an account failed */
    NOTI_VALIDATE_AND_UPDATE_ACCOUNT_CANCEL,            /**< Validating and updating an account canceled */

    NOTI_VALIDATE_CERTIFICATE_FINISH         = 10000,   /**< Validating a certificate finished */
    NOTI_VALIDATE_CERTIFICATE_FAIL           = 10001,   /**< Validating a certificate failed */
    NOTI_VALIDATE_CERTIFICATE_CANCEL         = 10002,   /**< Validating a certificate canceled */

    NOTI_RESOLVE_RECIPIENT_START             = 11000,   /**< Resolving recipients started */
    NOTI_RESOLVE_RECIPIENT_FINISH,                      /**< Resolving recipients finished */
    NOTI_RESOLVE_RECIPIENT_FAIL,                        /**< Resolving recipients failed */
    NOTI_RESOLVE_RECIPIENT_CANCEL,                      /**< Resolving recipients canceled */

    NOTI_RENAME_MAILBOX_START                = 12000,   /**< Renaming a mailbox started */
    NOTI_RENAME_MAILBOX_FINISH,                         /**< Renaming a mailbox finished */
    NOTI_RENAME_MAILBOX_FAIL,                           /**< Renaming a mailbox failed */
    NOTI_RENAME_MAILBOX_CANCEL,                         /**< Renaming a mailbox canceled */

    NOTI_ADD_MAILBOX_START                   = 12100,   /**< Adding a mailbox started */
    NOTI_ADD_MAILBOX_FINISH,                            /**< Adding a mailbox finished */
    NOTI_ADD_MAILBOX_FAIL,                              /**< Adding a mailbox failed */
    NOTI_ADD_MAILBOX_CANCEL,                            /**< Adding a mailbox canceled */

    NOTI_DELETE_MAILBOX_START                = 12200,   /**< Removing a mailbox started */
    NOTI_DELETE_MAILBOX_FINISH,                         /**< Removing a mailbox finished */
    NOTI_DELETE_MAILBOX_FAIL,                           /**< Removing a mailbox failed */
    NOTI_DELETE_MAILBOX_CANCEL,                         /**< Removing a mailbox canceled */

    NOTI_SYNC_IMAP_MAILBOX_LIST_START        = 12300,   /**< Syncing mailbox list started */
    NOTI_SYNC_IMAP_MAILBOX_LIST_FINISH,                 /**< Syncing mailbox list finished */
    NOTI_SYNC_IMAP_MAILBOX_LIST_FAIL,                   /**< Syncing mailbox list failed */
    NOTI_SYNC_IMAP_MAILBOX_LIST_CANCEL,                 /**< Syncing mailbox list canceled */

    NOTI_DELETE_MAIL_START                   = 12400,   /**< Removing mails started */
    NOTI_DELETE_MAIL_FINISH,                            /**< Removing mails finished */
    NOTI_DELETE_MAIL_FAIL,                              /**< Removing mails failed */
    NOTI_DELETE_MAIL_CANCEL,                            /**< Removing mails canceled */

    NOTI_QUERY_SMTP_MAIL_SIZE_LIMIT_FINISH  = 12500,    /**< Querying limitation of mail size to SMTP finished */
    NOTI_QUERY_SMTP_MAIL_SIZE_LIMIT_FAIL,               /**< Querying limitation of mail size to SMTP failed */

    /* To be added more */
} email_noti_on_network_event;

/**
 * @brief Enumeration for the response to request.
 * @since_tizen 2.3
 */
typedef enum
{
    RESPONSE_SUCCEEDED                       = 0,    /**< The request succeeded */
    RESPONSE_FAILED                          = 1,    /**< The request failed */
    RESPONSE_CANCELED                        = 2     /**< The request is canceled */
    /* To be added more */
} email_response_to_request;

/**
 * @brief Enumeration for the account mail type.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_BIND_TYPE_DISABLE          = 0,          /**< The bind type for Disabled account */
    EMAIL_BIND_TYPE_EM_CORE          = 1,          /**< The bind type for email-service */
} email_account_bind_t DEPRECATED;

/**
 * @brief Enumeration for the account server type.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_SERVER_TYPE_POP3           = 1,          /**< The POP3 Server */
    EMAIL_SERVER_TYPE_IMAP4,                       /**< The IMAP4 Server */
    EMAIL_SERVER_TYPE_SMTP,                        /**< The SMTP Server */
    EMAIL_SERVER_TYPE_NONE,                        /**< The local account */
    EMAIL_SERVER_TYPE_ACTIVE_SYNC,                 /**< The Active Sync */
} email_account_server_t;

/**
 * @brief Enumeration for the retrieval mode.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_IMAP4_RETRIEVAL_MODE_NEW   = 1,          /**< Retrieval mode for new email */
    EMAIL_IMAP4_RETRIEVAL_MODE_ALL   = 2,          /**< Retrieval mode for all email */
    EMAIL_IMAP4_IDLE_SUPPORTED       = 0x00100000  /**< Support for feature 'imap idle' */
} email_imap4_retrieval_mode_t;

/**
 * @brief Enumeration for the filtering type.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_FILTER_FROM                = 1,          /**< Filtering of sender */
    EMAIL_FILTER_SUBJECT             = 2,          /**< Filtering of email subject */
    EMAIL_FILTER_BODY                = 4,          /**< Filtering of email body */
    EMAIL_PRIORITY_SENDER            = 8,          /**< Priority sender of email */
} email_rule_type_t;


/**
 * @brief Enumeration for the rules for filtering type.
 * @since_tizen 2.3
 */
typedef enum
{
    RULE_TYPE_INCLUDES             = 1,          /**< Filtering rule for includes */
    RULE_TYPE_EXACTLY,                           /**< Filtering rule for Exactly same as */
} email_filtering_type_t;


/**
 * @brief Enumeration for the action for filtering type.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_FILTER_MOVE                = 1,          /**< Filter action is moving email */
    EMAIL_FILTER_BLOCK               = 2,          /**< Filtering action is blocking email */
    EMAIL_FILTER_DELETE              = 3,          /**< Filtering action is deleting email */
} email_rule_action_t;

/**
 * @brief Enumeration for the email status.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_MAIL_STATUS_NONE           = 0,          /**< The Mail is in No Operation state */
    EMAIL_MAIL_STATUS_RECEIVED,                    /**< The mail is a received mail */
    EMAIL_MAIL_STATUS_SENT,                        /**< The mail is a sent mail */
    EMAIL_MAIL_STATUS_SAVED,                       /**< The mail is a saved mail */
    EMAIL_MAIL_STATUS_SAVED_OFFLINE,               /**< The mail is a saved mail in off-line mode */
    EMAIL_MAIL_STATUS_SENDING,                     /**< The mail is being sent */
    EMAIL_MAIL_STATUS_SEND_FAILURE,                /**< The mail is a sending failed mail */
    EMAIL_MAIL_STATUS_SEND_CANCELED,               /**< The mail is a canceled mail */
    EMAIL_MAIL_STATUS_SEND_WAIT,                   /**< The mail is waiting to be sent */
    EMAIL_MAIL_STATUS_SEND_SCHEDULED,              /**< The mail is a scheduled mail to be sent later.*/
    EMAIL_MAIL_STATUS_SEND_DELAYED,                /**< The mail is a delayed mail to be sent later */
    EMAIL_MAIL_STATUS_NOTI_WAITED,                 /**< The mail is a waited notification */
} email_mail_status_t;

/**
 * @brief Enumeration for the email priority.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_MAIL_PRIORITY_HIGH         = 1,          /**< The priority is high */
    EMAIL_MAIL_PRIORITY_NORMAL       = 3,          /**< The priority is normal */
    EMAIL_MAIL_PRIORITY_LOW          = 5,          /**< The priority is low */
} email_mail_priority_t;

/**
 * @brief Enumeration for the email status.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_MAIL_REPORT_NONE           = 0x00,       /**< The mail is not report mail */
    EMAIL_MAIL_REPORT_REQUEST        = 0x03,       /**< The mail is to request report mail */
    EMAIL_MAIL_REPORT_DSN            = 0x04,       /**< The mail is a Delivery Status Notifications mail */
    EMAIL_MAIL_REPORT_MDN            = 0x08,       /**< The mail is a Message Disposition Notifications mail */
    EMAIL_MAIL_REQUEST_DSN           = 0x10,       /**< The mail requires Delivery Status Notifications */
    EMAIL_MAIL_REQUEST_MDN           = 0x20,       /**< The mail requires Message Disposition Notifications */
} email_mail_report_t;

/**
 * @brief Enumeration for the DRM type.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_ATTACHMENT_DRM_NONE        = 0,          /**< The mail is not DRM file */
    EMAIL_ATTACHMENT_DRM_OBJECT,                   /**< The mail is a DRM object */
    EMAIL_ATTACHMENT_DRM_RIGHTS,                   /**< The mail is a DRM rights as XML format */
    EMAIL_ATTACHMENT_DRM_DCF,                      /**< The mail is a DRM DCF */
} email_attachment_drm_t;

/**
 * @brief Enumeration for the mail type.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_MAIL_TYPE_NORMAL                     = 0, /**< NOT a meeting request mail. A Normal mail */
    EMAIL_MAIL_TYPE_MEETING_REQUEST            = 1, /**< A meeting request mail */
    EMAIL_MAIL_TYPE_MEETING_RESPONSE           = 2, /**< A response mail about meeting request */
    EMAIL_MAIL_TYPE_MEETING_ORIGINATINGREQUEST = 3  /**< An originating mail about meeting request */
} email_mail_type_t;

/**
 * @brief Enumeration for the meeting response type.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_MEETING_RESPONSE_NONE                = 0,     /**< NOT a response */
    EMAIL_MEETING_RESPONSE_ACCEPT              = 1,     /**< The response is acceptance */
    EMAIL_MEETING_RESPONSE_TENTATIVE           = 2,     /**< The response is tentative */
    EMAIL_MEETING_RESPONSE_DECLINE             = 3,     /**< The response is decline */
    EMAIL_MEETING_RESPONSE_REQUEST             = 4,     /**< The response is request */
    EMAIL_MEETING_RESPONSE_CANCEL              = 5,     /**< The response is cancellation */
    EMAIL_MEETING_RESPONSE_PROPOSE_NEW_TIME_TENTATIVE,  /**< The response proposes new time tentative */
    EMAIL_MEETING_RESPONSE_PROPOSE_NEW_TIME_DECLINE     /**< The response proposes new time decline */
} email_meeting_response_t;

typedef enum
{
    EMAIL_ACTION_SEND_MAIL                     =  0,    /**< Action type for sending mail */
    EMAIL_ACTION_SYNC_HEADER                   =  1,    /**< Action type for syncing header */
    EMAIL_ACTION_DOWNLOAD_BODY                 =  2,    /**< Action type for downloading body */
    EMAIL_ACTION_DOWNLOAD_ATTACHMENT           =  3,    /**< Action type for downloading attachment */
    EMAIL_ACTION_DELETE_MAIL                   =  4,    /**< Action type for deleting mail */
    EMAIL_ACTION_SEARCH_MAIL                   =  5,    /**< Action type for searching mail */
    EMAIL_ACTION_SAVE_MAIL                     =  6,    /**< Action type for saving mail */
    EMAIL_ACTION_SYNC_MAIL_FLAG_TO_SERVER      =  7,    /**< Action type for syncing mail flag */
    EMAIL_ACTION_SYNC_FLAGS_FIELD_TO_SERVER    =  8,    /**< Action type for syncing flags field */
    EMAIL_ACTION_MOVE_MAIL                     =  9,    /**< Action type for moving mail */
    EMAIL_ACTION_CREATE_MAILBOX                = 10,    /**< Action type for creating mailbox */
    EMAIL_ACTION_DELETE_MAILBOX                = 11,    /**< Action type for deleting mailbox */
    EMAIL_ACTION_SYNC_HEADER_OMA               = 12,    /**< Action type for syncing header by oma-emn */
    EMAIL_ACTION_VALIDATE_ACCOUNT              = 13,    /**< Action type for validating account */
    EMAIL_ACTION_VALIDATE_AND_CREATE_ACCOUNT   = 14,    /**< Action type for validating and creating account */
    EMAIL_ACTION_VALIDATE_AND_UPDATE_ACCOUNT   = 15,    /**< Action type for validating and updating account */
    EMAIL_ACTION_VALIDATE_ACCOUNT_EX           = 16,    /**< Action type for validating account */
    EMAIL_ACTION_UPDATE_MAIL                   = 30,    /**< Action type for updating account */
    EMAIL_ACTION_SET_MAIL_SLOT_SIZE            = 31,    /**< Action type for setting mail slot size */
    EMAIL_ACTION_EXPUNGE_MAILS_DELETED_FLAGGED = 32,    /**< Action type for expunge mails deleted flagged */
    EMAIL_ACTION_SEARCH_ON_SERVER              = 33,    /**< Action type for searching on server */
    EMAIL_ACTION_MOVE_MAILBOX                  = 34,    /**< Action type for moving mailbox */
    EMAIL_ACTION_SENDING_MAIL                  = 35,    /**< Action type for sending mail */
    EMAIL_ACTION_NUM,                                   /**< end of email_action_t */
} email_action_t;

/**
 * @brief Enumeration for the status of getting an envelope list.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_LIST_NONE                  = 0,          /**< Initial status */
    EMAIL_LIST_WAITING,                            /**< Waiting status */
    EMAIL_LIST_PREPARE,                            /**< Preparation status */
    EMAIL_LIST_CONNECTION_START,                   /**< Connection start */
    EMAIL_LIST_CONNECTION_SUCCEED,                 /**< Connection success */
    EMAIL_LIST_CONNECTION_FINISH,                  /**< Connection finish */
    EMAIL_LIST_CONNECTION_FAIL,                    /**< Connection failure */
    EMAIL_LIST_START,                              /**< Getting the list started */
    EMAIL_LIST_PROGRESS,                           /**< The progress status of getting */
    EMAIL_LIST_FINISH,                             /**< Getting the list completed */
    EMAIL_LIST_FAIL,                               /**< Getting the list failed */
} email_envelope_list_status_t;

/**
 * @brief Enumeration for the downloaded status of an email.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_DOWNLOAD_NONE              = 0,          /**< Initial status */
    EMAIL_DOWNLOAD_WAITING,                        /**< Download is waiting */
    EMAIL_DOWNLOAD_PREPARE,                        /**< Preparing for download */
    EMAIL_DOWNLOAD_CONNECTION_START,               /**< Connection start */
    EMAIL_DOWNLOAD_CONNECTION_SUCCEED,             /**< Connection success */
    EMAIL_DOWNLOAD_CONNECTION_FINISH,              /**< Connection finish */
    EMAIL_DOWNLOAD_CONNECTION_FAIL,                /**< Connection failure */
    EMAIL_DOWNLOAD_START,                          /**< Download start */
    EMAIL_DOWNLOAD_PROGRESS,                       /**< Progress of download */
    EMAIL_DOWNLOAD_FINISH,                         /**< Download complete */
    EMAIL_DOWNLOAD_FAIL,                           /**< Download failure */
} email_download_status_t;

/**
 * @brief Enumeration for the status of sending an email.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_SEND_NONE                  = 0,          /**< Initial status */
    EMAIL_SEND_WAITING,                            /**< Waiting to send */
    EMAIL_SEND_PREPARE,                            /**< Preparing to send */
    EMAIL_SEND_CONNECTION_START,                   /**< Starting the send connection */
    EMAIL_SEND_CONNECTION_SUCCEED,                 /**< Send connection success */
    EMAIL_SEND_CONNECTION_FINISH,                  /**< Send connection finish */
    EMAIL_SEND_CONNECTION_FAIL,                    /**< Send connection failure */
    EMAIL_SEND_START,                              /**< Start sending */
    EMAIL_SEND_PROGRESS,                           /**< Sending status */
    EMAIL_SEND_FINISH,                             /**< Sending complete */
    EMAIL_SEND_FAIL,                               /**< Sending failure */
    EMAIL_SAVE_WAITING,                            /**< Waiting to save */
} email_send_status_t;

typedef enum
{
    EMAIL_SYNC_NONE                  = 0,          /**< Initial status */
    EMAIL_SYNC_WAITING,                            /**< Waiting to sync */
    EMAIL_SYNC_PREPARE,                            /**< Preparing to sync */
    EMAIL_SYNC_CONNECTION_START,                   /**< Starting sync connection */
    EMAIL_SYNC_CONNECTION_SUCCEED,                 /**< Sync connection success */
    EMAIL_SYNC_CONNECTION_FINISH,                  /**< Sync connection finish */
    EMAIL_SYNC_CONNECTION_FAIL,                    /**< Sync connection failure*/
    EMAIL_SYNC_START,                              /**< Start syncing */
    EMAIL_SYNC_PROGRESS,                           /**< Sync status */
    EMAIL_SYNC_FINISH,                             /**< Sync complete */
    EMAIL_SYNC_FAIL,                               /**< Sync failure */
} email_sync_status_t;

/**
* @brief Enumeration for the deleting status of an email.
* @since_tizen 2.3
*/
typedef enum
{
    EMAIL_DELETE_NONE                = 0,          /**< Initial status */
    EMAIL_DELETE_WAITING,                          /**< Waiting to delete */
    EMAIL_DELETE_PREPARE,                          /**< Preparing to delete */
    EMAIL_DELETE_CONNECTION_START,                 /**< Starting delete connection */
    EMAIL_DELETE_CONNECTION_SUCCEED,               /**< Delete connection success */
    EMAIL_DELETE_CONNECTION_FINISH,                /**< Delete connection finish */
    EMAIL_DELETE_CONNECTION_FAIL,                  /**< Delete connection failure */
    EMAIL_DELETE_START,                            /**< Deletion start */
    EMAIL_DELETE_PROGRESS,                         /**< Delete status */
    EMAIL_DELETE_SERVER_PROGRESS,                  /**< Server deleting status */
    EMAIL_DELETE_LOCAL_PROGRESS,                   /**< Local deleting status*/
    EMAIL_DELETE_FINISH,                           /**< Deletion complete */
    EMAIL_DELETE_FAIL,                             /**< Deletion failure */
} email_delete_status_t;

/**
* @brief Enumeration for the status of validating an account.
* @since_tizen 2.3
*/
typedef enum
{
    EMAIL_VALIDATE_ACCOUNT_NONE = 0,               /**< Initial status */
    EMAIL_VALIDATE_ACCOUNT_WAITING,                /**< Waiting to validate account */
    EMAIL_VALIDATE_ACCOUNT_PREPARE,                /**< Preparing to validate account */
    EMAIL_VALIDATE_ACCOUNT_CONNECTION_START,       /**< Starting validate account connection */
    EMAIL_VALIDATE_ACCOUNT_CONNECTION_SUCCEED,     /**< Validate account connection success */
    EMAIL_VALIDATE_ACCOUNT_CONNECTION_FINISH,      /**< Validate account connection finish */
    EMAIL_VALIDATE_ACCOUNT_CONNECTION_FAIL,        /**< Validate account connection failure */
    EMAIL_VALIDATE_ACCOUNT_START,                  /**< Start validating account */
    EMAIL_VALIDATE_ACCOUNT_PROGRESS,               /**< Account validation status */
    EMAIL_VALIDATE_ACCOUNT_FINISH,                 /**< Account validation complete */
    EMAIL_VALIDATE_ACCOUNT_FAIL,                   /**< Account validation failure.*/
} email_validate_account_status_t;

/**
* @brief Enumeration for the status of setting slot size.
* @since_tizen 2.3
*/
typedef enum
{
    EMAIL_SET_SLOT_SIZE_NONE         = 0,          /**< Initial status */
    EMAIL_SET_SLOT_SIZE_WAITING,                   /**< Waiting status*/
    EMAIL_SET_SLOT_SIZE_START,                     /**< Task started  */
    EMAIL_SET_SLOT_SIZE_FINISH,                    /**< Task finished */
    EMAIL_SET_SLOT_SIZE_FAIL,                      /**< Task failed */
} email_set_slot_size_status_e;

/**
* @brief Enumeration for the status of expunging mails.
* @since_tizen 2.3
*/
typedef enum
{
    EMAIL_EXPUNGE_MAILS_DELETED_FLAGGED_NONE         = 0,    /**< Initial status */
    EMAIL_EXPUNGE_MAILS_DELETED_FLAGGED_WAITING,             /**< Waiting status*/
    EMAIL_EXPUNGE_MAILS_DELETED_FLAGGED_START,               /**< Task started  */
    EMAIL_EXPUNGE_MAILS_DELETED_FLAGGED_FINISH,              /**< Task finished */
    EMAIL_EXPUNGE_MAILS_DELETED_FLAGGED_FAIL,                /**< Task failed */
} email_expunge_mails_deleted_flagged_status_e;

/**
* @brief Enumeration for the status of searching mails on server.
* @since_tizen 2.4
*/
typedef enum
{
    EMAIL_SEARCH_ON_SERVER_NONE         = 0,    /**< Initial status */
    EMAIL_SEARCH_ON_SERVER_WAITING,             /**< Waiting status*/
    EMAIL_SEARCH_ON_SERVER_START,               /**< Task started  */
    EMAIL_SEARCH_ON_SERVER_FINISH,              /**< Task finished */
    EMAIL_SEARCH_ON_SERVER_FAIL,                /**< Task failed */
} email_search_on_server_status_e;

/**
* @brief Enumeration for the status of moving mails.
* @since_tizen 2.3
*/
typedef enum
{
    EMAIL_MOVE_MAILBOX_ON_IMAP_SERVER_NONE         = 0,  /**< Initial status */
    EMAIL_MOVE_MAILBOX_ON_IMAP_SERVER_WAITING,           /**< Waiting status*/
    EMAIL_MOVE_MAILBOX_ON_IMAP_SERVER_START,             /**< Task started  */
    EMAIL_MOVE_MAILBOX_ON_IMAP_SERVER_FINISH,            /**< Task finished */
    EMAIL_MOVE_MAILBOX_ON_IMAP_SERVER_FAIL,              /**< Task failed */
} email_move_mailbox_status_e;

/**
* @brief Enumeration for the status of updating mails.
* @since_tizen 2.3
*/
typedef enum
{
    EMAIL_UPDATE_MAIL_NONE         = 0,    /**< Initial status */
    EMAIL_UPDATE_MAIL_WAITING,             /**< Waiting status*/
    EMAIL_UPDATE_MAIL_START,               /**< Task started  */
    EMAIL_UPDATE_MAIL_FINISH,              /**< Task finished */
    EMAIL_UPDATE_MAIL_FAIL,                /**< Task failed */
} email_update_mail_status_e;

/**
* @brief Enumeration for the mailbox type.
* @since_tizen 2.3
*/
typedef enum
{
    EMAIL_MAILBOX_TYPE_NONE          = 0,         /**< Unspecified mailbox type */
    EMAIL_MAILBOX_TYPE_INBOX         = 1,         /**< Specified inbox type */
    EMAIL_MAILBOX_TYPE_SENTBOX       = 2,         /**< Specified sent box type */
    EMAIL_MAILBOX_TYPE_TRASH         = 3,         /**< Specified trash type */
    EMAIL_MAILBOX_TYPE_DRAFT         = 4,         /**< Specified draft box type */
    EMAIL_MAILBOX_TYPE_SPAMBOX       = 5,         /**< Specified spam box type */
    EMAIL_MAILBOX_TYPE_OUTBOX        = 6,         /**< Specified outbox type */
    EMAIL_MAILBOX_TYPE_ALL_EMAILS    = 7,         /**< Specified all emails type of gmail */
    EMAIL_MAILBOX_TYPE_SEARCH_RESULT = 8,         /**< Specified mailbox type for result of search on server */
    EMAIL_MAILBOX_TYPE_FLAGGED       = 9,         /**< Specified flagged mailbox type on gmail */
    EMAIL_MAILBOX_TYPE_USER_DEFINED  = 0xFF,      /**< Specified mailbox type for all other mailboxes */
} email_mailbox_type_e;


/** @brief Enumeration for the sync order.
 * @since_tizen 2.3 */


typedef enum
{
    EMAIL_SYNC_LATEST_MAILS_FIRST    = 0,    /**< Download latest mails first */
    EMAIL_SYNC_OLDEST_MAILS_FIRST,           /**< Download oldest mails first */
    EMAIL_SYNC_ALL_MAILBOX_50_MAILS,         /**< Download latest 50 mails only */
} EMAIL_RETRIEVE_MODE;

/** @brief Enumeration for the event type.
 * @since_tizen 2.3 */
typedef enum
{
    EMAIL_EVENT_NONE                            =  0,          /**< Initial value of #email_event_type_t */
    EMAIL_EVENT_SYNC_HEADER                     =  1,          /**< Synchronize mail headers with server (network used) */
    EMAIL_EVENT_DOWNLOAD_BODY                   =  2,          /**< Download mail body from server (network used)*/
    EMAIL_EVENT_DOWNLOAD_ATTACHMENT             =  3,          /**< Download mail attachment from server (network used) */
    EMAIL_EVENT_SEND_MAIL                       =  4,          /**< Send a mail (network used) */
    EMAIL_EVENT_SEND_MAIL_SAVED                 =  5,          /**< Send all mails in 'outbox' (network used) */
    EMAIL_EVENT_SYNC_IMAP_MAILBOX               =  6,          /**< Download IMAP mailboxes from server (network used) */
    EMAIL_EVENT_DELETE_MAIL                     =  7,          /**< Delete mails (network not used) */
    EMAIL_EVENT_DELETE_MAIL_ALL                 =  8,          /**< Delete all mails (network not used) */
    EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER        =  9,          /**< Sync mail flag to server */
    EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER      = 10,          /**< Sync a field of flags to server */
    EMAIL_EVENT_SAVE_MAIL                       = 11,          /**< Add mail on server */
    EMAIL_EVENT_MOVE_MAIL                       = 12,          /**< Move mails to specific mailbox on server */
    EMAIL_EVENT_CREATE_MAILBOX                  = 13,          /**< Create a mailbox */
    EMAIL_EVENT_UPDATE_MAILBOX                  = 14,          /**< Update a mailbox */
    EMAIL_EVENT_DELETE_MAILBOX                  = 15,          /**< Delete a mailbox */
    EMAIL_EVENT_ISSUE_IDLE                      = 16,          /**< Deprecated */
    EMAIL_EVENT_SYNC_HEADER_OMA                 = 17,          /**< Sync mail headers by OMA-EMN */
    EMAIL_EVENT_VALIDATE_ACCOUNT                = 18,          /**< Validate account */
    EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT     = 19,          /**< Validate and create account */
    EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT     = 20,          /**< Validate and update account */
    EMAIL_EVENT_SEARCH_ON_SERVER                = 21,          /**< Search mails on server */
    EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER   = 22,          /**< Rename a mailbox on imap server */
    EMAIL_EVENT_VALIDATE_ACCOUNT_EX             = 23,          /**< Validate account */
    EMAIL_EVENT_QUERY_SMTP_MAIL_SIZE_LIMIT      = 24,          /**< Query limitation of mail size to SMTP server */

    EMAIL_EVENT_ADD_MAIL                        = 10001,       /**< Deprecated */
    EMAIL_EVENT_UPDATE_MAIL_OLD                 = 10002,       /**< Deprecated */
    EMAIL_EVENT_UPDATE_MAIL                     = 10003,       /**< Update a mail */
    EMAIL_EVENT_SET_MAIL_SLOT_SIZE              = 20000,       /**< Set mail slot size */
    EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED   = 20001,       /**< Expunge mails deleted flagged */

/*  EMAIL_EVENT_LOCAL_ACTIVITY,                     __LOCAL_ACTIVITY_ */

    EMAIL_EVENT_BULK_PARTIAL_BODY_DOWNLOAD      = 20002,       /**< Download body partially */
    EMAIL_EVENT_LOCAL_ACTIVITY_SYNC_BULK_PBD    = 20003,       /**< Deprecated */
    EMAIL_EVENT_GET_PASSWORD_LENGTH             = 20004,       /**< Get password length of an account */
} email_event_type_t;

/** @brief Enumeration for the event status.
 * @since_tizen 2.3 */
typedef enum
{
    EMAIL_EVENT_STATUS_UNUSED        = 0,    /**< Initial status of event */
    EMAIL_EVENT_STATUS_WAIT,                 /**< Waiting status */
    EMAIL_EVENT_STATUS_STARTED,              /**< Event handling is started */
    EMAIL_EVENT_STATUS_CANCELED,             /**< Event handling is canceled */
} email_event_status_type_t;


/** @brief Enumeration for the srting type.
 * @since_tizen 2.3 */
typedef enum
{
    EMAIL_SORT_DATETIME_HIGH         = 0,    /**< Sort mails by datetime ascending order */
    EMAIL_SORT_DATETIME_LOW,                 /**< Sort mails by datetime descending order */
    EMAIL_SORT_SENDER_HIGH,                  /**< Sort mails by sender ascending order */
    EMAIL_SORT_SENDER_LOW,                   /**< Sort mails by sender descending order */
    EMAIL_SORT_RCPT_HIGH,                    /**< Sort mails by recipient ascending order */
    EMAIL_SORT_RCPT_LOW,                     /**< Sort mails by recipient descending order */
    EMAIL_SORT_SUBJECT_HIGH,                 /**< Sort mails by subject ascending order */
    EMAIL_SORT_SUBJECT_LOW,                  /**< Sort mails by subject descending order */
    EMAIL_SORT_PRIORITY_HIGH,                /**< Sort mails by priority ascending order */
    EMAIL_SORT_PRIORITY_LOW,                 /**< Sort mails by priority descending order */
    EMAIL_SORT_ATTACHMENT_HIGH,              /**< Sort mails by attachment ascending order */
    EMAIL_SORT_ATTACHMENT_LOW,               /**< Sort mails by attachment descending order */
    EMAIL_SORT_FAVORITE_HIGH,                /**< Sort mails by favorite ascending order */
    EMAIL_SORT_FAVORITE_LOW,                 /**< Sort mails by favorite descending order */
    EMAIL_SORT_MAILBOX_ID_HIGH,              /**< Sort mails by mailbox ID ascending order */
    EMAIL_SORT_MAILBOX_ID_LOW,               /**< Sort mails by mailbox ID descending order */
    EMAIL_SORT_FLAGGED_FLAG_HIGH,            /**< Sort mails by flagged flag ID ascending order */
    EMAIL_SORT_FLAGGED_FLAG_LOW,             /**< Sort mails by flagged flag ID descending order */
    EMAIL_SORT_SEEN_FLAG_HIGH,               /**< Sort mails by seen flag ID ascending order */
    EMAIL_SORT_SEEN_FLAG_LOW,                /**< Sort mails by seen flag ID descending order */
    EMAIL_SORT_END,                          /**< end of #email_sort_type_t */
}email_sort_type_t;

typedef enum
{
    EMAIL_MAILBOX_SORT_BY_NAME_ASC  = 0,  /**< Sort mailbox by name ascending order */
    EMAIL_MAILBOX_SORT_BY_NAME_DSC,       /**< Sort mailbox by name descending order */
    EMAIL_MAILBOX_SORT_BY_TYPE_ASC,       /**< Sort mailbox by type ascending order */
    EMAIL_MAILBOX_SORT_BY_TYPE_DSC,       /**< Sort mailbox by type descending order */
} email_mailbox_sort_type_t;


/**
* @brief Enumeration for the priority.
* @since_tizen 2.3
*/
enum
{
    EMAIL_OPTION_PRIORITY_HIGH       = 1,          /**< High priority */
    EMAIL_OPTION_PRIORITY_NORMAL     = 3,          /**< Normal priority */
    EMAIL_OPTION_PRIORITY_LOW        = 5,          /**< Low priority */
};

/**
* @brief Enumeration for the saving a copy after sending.
* @since_tizen 2.3
*/
enum
{
    EMAIL_OPTION_KEEP_LOCAL_COPY_OFF = 0,          /**< Keeping local copy is not enabled */
    EMAIL_OPTION_KEEP_LOCAL_COPY_ON  = 1,          /**< Keeping local copy is enabled */
};

/**
* @brief Enumeration for the request of a delivery report.
* @since_tizen 2.3
*/
enum
{
    EMAIL_OPTION_REQ_DELIVERY_RECEIPT_OFF = 0,     /**< Requesting delivery receipt is not enabled */
    EMAIL_OPTION_REQ_DELIVERY_RECEIPT_ON  = 1,     /**< Requesting delivery receipt is enabled */
};

/**
* @brief Enumeration for the request of a read receipt.
* @since_tizen 2.3
*/
enum
{
    EMAIL_OPTION_REQ_READ_RECEIPT_OFF = 0,         /**< Requesting read receipt is not enabled */
    EMAIL_OPTION_REQ_READ_RECEIPT_ON  = 1,         /**< Requesting read receipt is enabled */
};

/**
* @brief Enumeration for the blocking of an address.
* @since_tizen 2.3
*/
enum
{
    EMAIL_OPTION_BLOCK_ADDRESS_OFF   = 0,          /**< Blocking an address is not enabled */
    EMAIL_OPTION_BLOCK_ADDRESS_ON    = 1,          /**< Blocking an address is enabled */
};

/**
* @brief Enumeration for the blocking of a subject.
* @since_tizen 2.3
*/
enum
{
    EMAIL_OPTION_BLOCK_SUBJECT_OFF   = 0,          /**< Blocking by subject is not enabled */
    EMAIL_OPTION_BLOCK_SUBJECT_ON    = 1,          /**< Blocking by subject is enabled */
};

enum
{
    EMAIL_LIST_TYPE_UNREAD           = -3,        /**< List unread mails */
    EMAIL_LIST_TYPE_LOCAL            = -2,        /**< List local mails */
    EMAIL_LIST_TYPE_THREAD           = -1,        /**< List thread */
    EMAIL_LIST_TYPE_NORMAL           = 0          /**< List all mails */
};

/**
* @brief Enumeration for the mailbox sync type.
* @since_tizen 2.3
*/
enum
{
    EMAIL_MAILBOX_ALL                = -1,         /**< All mailboxes */
    EMAIL_MAILBOX_FROM_SERVER        = 0,          /**< Mailboxes from server */
    EMAIL_MAILBOX_FROM_LOCAL         = 1,          /**< Mailboxes from local */
};

/**
* @brief Enumeration for the mail change type.
* @since_tizen 2.3
*/
typedef enum
{
    APPEND_BODY                    = 1,    /**< Body is appended */
    UPDATE_MAILBOX,                        /**< Mailbox is updated */
    UPDATE_ATTACHMENT_INFO,                /**< Attachment info is updated */
    UPDATE_FLAG,                           /**< Flag is updated */
    UPDATE_SAVENAME,                       /**< Savename is updated */
    UPDATE_EXTRA_FLAG,                     /**< Extra flag is updated */
    UPDATE_MAIL,                           /**< Mail information is updated */
    UPDATE_DATETIME,                       /**< Datetime is updated */
    UPDATE_FROM_CONTACT_INFO,              /**< From contact info is updated */
    UPDATE_TO_CONTACT_INFO,                /**< To contact info is updated */
    UPDATE_ALL_CONTACT_NAME,               /**< All contact name is updated */
    UPDATE_ALL_CONTACT_INFO,               /**< All contact info is updated */
    UPDATE_STICKY_EXTRA_FLAG,              /**< Sticky extra flag is updated */
    UPDATE_PARTIAL_BODY_DOWNLOAD,          /**< Partial body download is updated */
    UPDATE_MEETING,                        /**< Meeting is updated */
    UPDATE_SEEN_FLAG_OF_THREAD,            /**< Seen flag of thread is updated */
    UPDATE_FILE_PATH,                      /**< File path is updated */
} email_mail_change_type_t;


/**
* @brief Enumeration for the address type.
* @since_tizen 2.3
*/
typedef enum
{
    EMAIL_ADDRESS_TYPE_FROM          = 1,          /**< From address */
    EMAIL_ADDRESS_TYPE_TO,                         /**< To recipient address */
    EMAIL_ADDRESS_TYPE_CC,                         /**< CC recipient address */
    EMAIL_ADDRESS_TYPE_BCC,                        /**< BCC recipient address */
    EMAIL_ADDRESS_TYPE_REPLY,                      /**< Reply recipient address */
    EMAIL_ADDRESS_TYPE_RETURN,                     /**< Return recipient address */
} email_address_type_t;

/**
 * @brief Enumeration for the search type for searching a mailbox.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_MAILBOX_SEARCH_KEY_TYPE_SUBJECT,         /**< The search key for searching subject */
    EMAIL_MAILBOX_SEARCH_KEY_TYPE_FROM,            /**< The search key for searching sender address */
    EMAIL_MAILBOX_SEARCH_KEY_TYPE_BODY,            /**< The search key for searching body */
    EMAIL_MAILBOX_SEARCH_KEY_TYPE_TO,              /**< The search key for searching recipient address */
} email_mailbox_search_key_t;

/**
 * @brief Enumeration for the download status of a mail body.
 * @since_tizen 2.3
 */

typedef enum
{
    EMAIL_BODY_DOWNLOAD_STATUS_NONE = 0,                  /**< The mail is not downloaded yet */
    EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED = 1,      /**< The mail is fully downloaded */
    EMAIL_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED = 2,  /**< The mail is partially downloaded */
} email_body_download_status_t;

/**
 * @brief Enumeration for the download status of a mail body.
 * @since_tizen 2.3
 */

typedef enum
{
    EMAIL_PART_DOWNLOAD_STATUS_NONE = 0,                   /**< The part is not downloaded yet */
    EMAIL_PART_DOWNLOAD_STATUS_FULLY_DOWNLOADED = 1,       /**< The part is fully downloaded */
    EMAIL_PART_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED = 2,   /**< The part is partially downloaded */
} email_part_download_status_t;

/**
 * @brief Enumeration for the moving type.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_MOVED_BY_COMMAND = 0,      /**< The mails are moved by user */
    EMAIL_MOVED_BY_MOVING_THREAD,    /**< The mails are moved with moving thread mails*/
    EMAIL_MOVED_AFTER_SENDING,       /**< The mails are moved by 'move after sending' option */
    EMAIL_MOVED_CANCELATION_MAIL     /**< The mails are moved by cancellation of meeting request */
} email_move_type;

/**
 * @brief Enumeration for the deletion type.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_DELETED_BY_COMMAND = 0,                /**< The mails are deleted by user */
    EMAIL_DELETED_BY_OVERFLOW,                   /**< The mails are deleted by overflow */
    EMAIL_DELETED_BY_DELETING_THREAD,            /**< The mails are deleted with removing thread mails */
    EMAIL_DELETED_BY_MOVING_TO_OTHER_ACCOUNT,    /**< The mails are deleted by moving to other account */
    EMAIL_DELETED_AFTER_SENDING,                 /**< The mails are deleted by 'delete after sending' option */
    EMAIL_DELETED_FROM_SERVER,                   /**< The mails are deleted because the mails are deleted on server */
} email_delete_type;

/**
 * @brief Enumeration for the status field type.
 * @since_tizen 2.3
 */
typedef enum
{
    EMAIL_FLAGS_SEEN_FIELD = 0,    /**< mail flag : \Seen */
    EMAIL_FLAGS_DELETED_FIELD,     /**< mail flag : \Deleted */
    EMAIL_FLAGS_FLAGGED_FIELD,     /**< mail flag : \Flagged */
    EMAIL_FLAGS_ANSWERED_FIELD,    /**< mail flag : \Answered */
    EMAIL_FLAGS_RECENT_FIELD,      /**< mail flag : \Recent */
    EMAIL_FLAGS_DRAFT_FIELD,       /**< mail flag : \Draft */
    EMAIL_FLAGS_FORWARDED_FIELD,   /**< mail flag : \Forwarded */
    EMAIL_FLAGS_FIELD_COUNT,       /**< field count */
} email_flags_field_type;

typedef enum {
    EMAIL_FLAG_NONE                  = 0,    /**< No flag */
    EMAIL_FLAG_FLAGED                = 1,    /**< Flagged */
    EMAIL_FLAG_TASK_STATUS_CLEAR     = 2,    /**< For EAS task management : No task */
    EMAIL_FLAG_TASK_STATUS_COMPLETE  = 3,    /**< For EAS task management : Completed task */
    EMAIL_FLAG_TASK_STATUS_ACTIVE    = 4,    /**< For EAS task management : Active task */
} email_flagged_type;

typedef enum {
    EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO       =  1,  /* integer type */    /**< Messages with specified message no */
    EMAIL_SEARCH_FILTER_TYPE_UID              =  2,  /* integer type */    /**< Messages with unique identifiers corresponding to the specified unique identifier set */
    EMAIL_SEARCH_FILTER_TYPE_ALL              =  3,  /* integer type */    /**< All messages in the mailbox; the default initial key for ANDing */
    EMAIL_SEARCH_FILTER_TYPE_BCC              =  7,  /* string type */     /**< Messages that contain the specified string in the envelope structure's BCC field */
    EMAIL_SEARCH_FILTER_TYPE_BODY             =  8,  /* string type */     /**< Messages that contain the specified string in the body of the message */
    EMAIL_SEARCH_FILTER_TYPE_CC               =  9,  /* string type */     /**< Messages that contain the specified string in the envelope structure's CC field */
    EMAIL_SEARCH_FILTER_TYPE_FROM             = 10,  /* string type */     /**< Messages that contain the specified string in the envelope structure's FROM field */
    EMAIL_SEARCH_FILTER_TYPE_KEYWORD          = 11,  /* string type */     /**< Messages with the specified keyword set */
    EMAIL_SEARCH_FILTER_TYPE_TEXT             = 12,  /* string type */     /**< Messages that contain the specified string in the header or body of the message */
    EMAIL_SEARCH_FILTER_TYPE_SUBJECT          = 13,  /* string type */     /**< Messages that contain the specified string in the envelope structure's SUBJECT field */
    EMAIL_SEARCH_FILTER_TYPE_TO               = 15,  /* string type */     /**< Messages that contain the specified string in the envelope structure's TO field */
    EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER      = 16,  /* integer type */    /**< Messages with a size larger than the specified number of octets */
    EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER     = 17,  /* integer type */    /**< Messages with a size smaller than the specified number of octets */
    EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE = 20,  /* time type */       /**< Messages whose Date: header is earlier than the specified date */
    EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON     = 21,  /* time type */       /**< Messages whose Date: header is within the specified date */
    EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE  = 22,  /* time type */       /**< Messages whose Date: header is within or later than the specified date */
    EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED   = 26,  /* integer type */    /**< Messages with the \Answered flag set */
    EMAIL_SEARCH_FILTER_TYPE_FLAGS_NEW        = 27,  /* integer type */    /**< Messages that have the \Recent flag set but not the \Seen flag */
    EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED    = 28,  /* integer type */    /**< Messages with the \Deleted flag set */
    EMAIL_SEARCH_FILTER_TYPE_FLAGS_OLD        = 29,  /* integer type */    /**< Messages that do not have the \Recent flag set */
    EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT      = 30,  /* integer type */    /**< Messages with the \Draft flag set */
    EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED     = 32,  /* integer type */    /**< Messages with the \Flagged flag set */
    EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT     = 34,  /* integer type */    /**< Messages that have the \Recent flag set */
    EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN       = 36,  /* integer type */    /**< Messages that have the \Seen flag set */
    EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID       = 43,  /* string type */     /**< Messages with specified message ID */
    EMAIL_SEARCH_FILTER_TYPE_HEADER_PRIORITY  = 50,  /* integer type */    /**< Messages that have a header with the specified priority */
    EMAIL_SEARCH_FILTER_TYPE_ATTACHMENT_NAME  = 60,  /* string type */     /**< Messages that contain the specified string in attachment name */
	EMAIL_SEARCH_FILTER_TYPE_CHARSET          = 61,  /* string type */     /**< Messages of encoded type */
	EMAIL_SEARCH_FILTER_TYPE_USER_DEFINED     = 62,  /* string type */     /**< Messages that extend and user defined string */ 
} email_search_filter_type;

typedef enum {
    EMAIL_DRM_TYPE_NONE                       = 0,    /**< No DRM type */
    EMAIL_DRM_TYPE_OBJECT                     = 1,    /**< DRM object */
    EMAIL_DRM_TYPE_RIGHT                      = 2,    /**< DRM right */
    EMAIL_DRM_TYPE_DCF                        = 3     /**< DRM Content Format */
} email_drm_type;

typedef enum {
    EMAIL_DRM_METHOD_NONE                     = 0,    /**< No DRM method */
    EMAIL_DRM_METHOD_FL                       = 1,    /**< Forward lock method */
    EMAIL_DRM_METHOD_CD                       = 2,    /**< Combined Delivery method */
    EMAIL_DRM_METHOD_SSD                      = 3,    /**< Deprecated */
    EMAIL_DRM_METHOD_SD                       = 4     /**< Separated Delivery method */
} email_drm_method;

typedef enum {
    EMAIL_CANCELED_BY_USER                    = 0,    /**< Canceled by user */
    EMAIL_CANCELED_BY_MDM                     = 1,    /**< Canceled by MDM policy */
} email_cancelation_type;

typedef enum {
    EMAIL_MAIL_ATTRIBUTE_MAIL_ID                 =  0,  /* integer type */    /**< @a mail_id field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID              =  1,  /* integer type */    /**< @a account_id field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_MAILBOX_ID              =  2,  /* integer type */    /**< @a mailbox_id field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_MAILBOX_NAME            =  3,  /* string type */     /**< @a mailbox_name field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE            =  4,  /* integer type */    /**< @a mailbox_type field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_SUBJECT                 =  5,  /* string type */     /**< @a subject field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_DATE_TIME               =  6,  /* datetime type */   /**< @a date_time field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_SERVER_MAIL_STATUS      =  7,  /* integer type */    /**< @a server_mail_status field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_SERVER_MAILBOX_NAME     =  8,  /* string type */     /**< @a server_mailbox_name field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_SERVER_MAIL_ID          =  9,  /* string type */     /**< @a server_mail_id field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_MESSAGE_ID              = 10,  /* string type */     /**< @a message_id field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_REFERENCE_MAIL_ID       = 11,  /* integer type */    /**< @a reference_mail_id field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_FROM                    = 12,  /* string type */     /**< @a full_address_from field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_TO                      = 13,  /* string type */     /**< @a full_address_to field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_CC                      = 14,  /* string type */     /**< @a full_address_cc field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_BCC                     = 15,  /* string type */     /**< @a full_address_bcc field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_BODY_DOWNLOAD_STATUS    = 16,  /* integer type */    /**< @a body_download_status field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_MAIL_SIZE               = 17,  /* integer type */    /**< @a mail_size field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_FILE_PATH_PLAIN         = 18,  /* string type */     /**< @a file_path_plain field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_FILE_PATH_HTML          = 19,  /* string type */     /**< @a file_path_html field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_FILE_SIZE               = 20,  /* integer type */    /* Deprecated */  /**< @a file_size field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_FLAGS_SEEN_FIELD        = 21,  /* integer type */    /**< @a flags_seen_field field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD     = 22,  /* integer type */    /**< @a flags_deleted_field field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_FLAGS_FLAGGED_FIELD     = 23,  /* integer type */    /**< @a flags_flagged_field field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_FLAGS_ANSWERED_FIELD    = 24,  /* integer type */    /**< @a flags_answered_field field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_FLAGS_RECENT_FIELD      = 25,  /* integer type */    /**< @a flags_recent_field field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_FLAGS_DRAFT_FIELD       = 26,  /* integer type */    /**< @a flags_draft_field field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_FLAGS_FORWARDED_FIELD   = 27,  /* integer type */    /**< @a flags_forwarded_field field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_DRM_STATUS              = 28,  /* integer type */    /**< @a drm_status field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_PRIORITY                = 29,  /* integer type */    /**< @a priority field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS             = 30,  /* integer type */    /**< @a save_status field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_LOCK_STATUS             = 31,  /* integer type */    /**< @a lock_status field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_REPORT_STATUS           = 32,  /* integer type */    /**< @a report_status field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_ATTACHMENT_COUNT        = 33,  /* integer type */    /**< @a attachment_count field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_INLINE_CONTENT_COUNT    = 34,  /* integer type */    /**< @a inline_content_count field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_THREAD_ID               = 35,  /* integer type */    /**< @a thread_id field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_THREAD_ITEM_COUNT       = 36,  /* integer type */    /**< @a thread_item_count field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_PREVIEW_TEXT            = 37,  /* string type */     /**< @a preview_text field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_MEETING_REQUEST_STATUS  = 38,  /* integer type */    /**< @a meeting_request_status field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_MESSAGE_CLASS           = 39,  /* integer type */    /**< @a message_class field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_DIGEST_TYPE             = 40,  /* integer type */    /**< @a digest_type field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_SMIME_TYPE              = 41,  /* integer type */    /**< @a smime_type field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_SCHEDULED_SENDING_TIME  = 42,  /* integer type */    /**< @a scheduled_sending_time field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_REMAINING_RESEND_TIMES  = 43,  /* integer type */    /**< @a remaining_resend_times field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_TAG_ID                  = 44,  /* integer type */    /**< @a tag_id field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_REPLIED_TIME            = 45,  /* integer type */    /**< @a replied_time field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_FORWARDED_TIME          = 46,  /* integer type */    /**< @a forwared_time field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_RECIPIENT_ADDRESS       = 47,  /* string type */     /**< @a recipient_address field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_EAS_DATA_LENGTH_TYPE    = 48,  /* integer type */    /**< @a eas_data_length_type field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_EAS_DATA_TYPE           = 49,  /* binary type */     /**< @a eas_data_type field of email_mail_data_t */
    EMAIL_MAIL_ATTRIBUTE_END                            /**< The end of attribute */
} email_mail_attribute_type;

typedef enum {
    EMAIL_MAIL_TEXT_ATTRIBUTE_FULL_TEXT = 1,    /**< Full text attribute of email_mail_text */
    EMAIL_MAIL_TEXT_ATTRIBUTE_END               /**< The end of email_mail_text_attribute */
} email_mail_text_attribute_type;

typedef enum {
    EMAIL_MAIL_ATTACH_ATTRIBUTE_ATTACHMENT_NAME = 1,    /**< Attachment name */
    EMAIL_MAIL_ATTCH_ATTRIBUTE_END                      /**< The end of email_mail_attach_attribute_type */
} email_mail_attach_attribute_type;

typedef enum {
    EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_NONE         = 0,    /**< The attribute type is none */
    EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER      = 1,    /**< The attribute type is integer */
    EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING       = 2,    /**< The attribute type is string */
    EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_TIME         = 3,    /**< The attribute type is time */
    EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_BINARY       = 4     /**< The attribute type is binary */
} email_mail_attribute_value_type;

typedef enum {
    EMAIL_ADD_MY_ADDRESS_OPTION_DO_NOT_ADD            = 0,    /**< Address option type is not added */
    EMAIL_ADD_MY_ADDRESS_OPTION_ALWAYS_ADD_TO_CC      = 1,    /**< Address option type is added to cc */
    EMAIL_ADD_MY_ADDRESS_OPTION_ALWAYS_ADD_TO_BCC     = 2,    /**< Address option type is added to bcc */
} email_add_my_address_option_type;

typedef enum {
    EMAIL_MESSAGE_CLASS_UNSPECIFIED,                                           /**< The message class is unspecified */
    EMAIL_MESSAGE_CLASS_UNKNOWN,                                               /**< The message class is unknown */
    EMAIL_MESSAGE_CLASS_NOTE,                                                  /**< The message class is note */
    EMAIL_MESSAGE_CLASS_NOTE_RULES_OF_TEMPLATE_MICROSOFT,                      /**< The message class is note_rule_of_template_microsoft */
    EMAIL_MESSAGE_CLASS_NOTE_SMIME,                                            /**< The message class is note_smime */
    EMAIL_MESSAGE_CLASS_NOTE_SMIME_MULTIPART_SIGNED,                           /**< The message class is note_smime_multipart_signed */
    EMAIL_MESSAGE_CLASS_NOTIFICATION_MEETING,                                  /**< The message class is notification meeting */ 
    EMAIL_MESSAGE_CLASS_OCTEL_VOICE,                                           /**< The message class is octel voice */ 
    EMAIL_MESSAGE_CLASS_SCHEDULE_MEETING_REQUEST,                              /**< The message class is meeting request */
    EMAIL_MESSAGE_CLASS_SCHEDULE_MEETING_CANCELED,                             /**< The message class is meeting canceled */
    EMAIL_MESSAGE_CLASS_SCHEDULE_MEETING_RESP_POS,                             /**< The message class is meeting resp pos */
    EMAIL_MESSAGE_CLASS_SCHEDULE_MEETING_RESP_TENT,                            /**< The message class is meeting resp tent */
    EMAIL_MESSAGE_CLASS_SCHEDULE_MEETING_RESP_NEG,                             /**< The message class is meeting resp neg */
    EMAIL_MESSAGE_CLASS_POST,                                                  /**< The message class is post */
    EMAIL_MESSAGE_CLASS_INFO_PATH_FORM,                                        /**< The message class is info path form */
    EMAIL_MESSAGE_CLASS_VOICE_NOTES,                                           /**< The message class is voice notes */
    EMAIL_MESSAGE_CLASS_SHARING,                                               /**< The message class is sharing */
    EMAIL_MESSAGE_CLASS_NOTE_EXCHANGE_ACTIVE_SYNC_REMOTE_WIPE_CONFIRMATION,    /**< The message class is note exchange active sync remote wipe confirmation */
    EMAIL_MESSAGE_CLASS_VOICE_MAIL,                                            /**< The message class is voice mail */
    EMAIL_MESSAGE_CLASS_SMS,                                                   /**< The message class is SMS */
    EMAIL_MESSAGE_CLASS_IRM_MESSAGE                           = 0x00010000,    /**< The message class is IRM message */
    EMAIL_MESSAGE_CLASS_SMART_REPLY                           = 0x00100000,    /**< The message class is smart reply */
    EMAIL_MESSAGE_CLASS_SMART_FORWARD                         = 0x00200000,    /**< The message class is smart forward */
    EMAIL_MESSAGE_CLASS_REPORT_NOT_READ_REPORT                = 0x01000000,    /**< The message class is report not read report */
    EMAIL_MESSAGE_CLASS_REPORT_READ_REPORT                    = 0x02000000,    /**< The message class is report read report */
    EMAIL_MESSAGE_CLASS_REPORT_NON_DELIVERY_RECEIPT           = 0x04000000,    /**< The message class is report non delivery receipt */
    EMAIL_MESSAGE_CLASS_REPORT_DELIVERY_RECEIPT               = 0x08000000,    /**< The message class is report delivery receipt */
    EMAIL_MESSAGE_CLASS_CALENDAR_FORWARD                      = 0x00400000     /**< The message class is calendar forward */
} email_message_class; 

typedef enum{
    EMAIL_SMIME_NONE                          = 0,   /**< Not using smime : Normal mail */ 
    EMAIL_SMIME_SIGNED,                              /**< Signed mail of smime */
    EMAIL_SMIME_ENCRYPTED,                           /**< Encrypted mail of smime */
    EMAIL_SMIME_SIGNED_AND_ENCRYPTED,                /**< Signed/encrypted mail of smime */
    EMAIL_PGP_SIGNED,                                /**< Signed mail of pgp */
    EMAIL_PGP_ENCRYPTED,                             /**< Encrypted mail of pgp */
    EMAIL_PGP_SIGNED_AND_ENCRYPTED                   /**< Signed/encrypted mail of pgp */
} email_smime_type;

typedef enum {
    CIPHER_TYPE_NONE                          = 0,   /**< None of cipher type */
    CIPHER_TYPE_DES3,                                /**< DES3 of cipher type */
    CIPHER_TYPE_DES,                                 /**< DES of cipher type */
    CIPHER_TYPE_RC2_128,                             /**< RC2 128 of cipher type */
    CIPHER_TYPE_RC2_64,                              /**< RC2 64 of cipher type */
    CIPHER_TYPE_RC2_40,                              /**< RC2 40 of cipher type */
} email_cipher_type;

typedef enum {
    DIGEST_TYPE_NONE                          = 0,    /**< None of digest type */
    DIGEST_TYPE_SHA1                          = 1,    /**< SHA1 of digest type */
    DIGEST_TYPE_MD5                           = 2,    /**< MD5 of digest type */
    DIGEST_TYPE_RIPEMD160                     = 3,    /**< RIPEMD160 of digest type */
    DIGEST_TYPE_MD2                           = 4,    /**< MD2 of digest type */
    DIGEST_TYPE_TIGER192                      = 5,    /**< TIGER192 of digest type */
    DIGEST_TYPE_HAVAL5160                     = 6,    /**< HAVAL5160 of digest type */
    DIGEST_TYPE_SHA256                        = 7,    /**< SHA256 of digest type */
    DIGEST_TYPE_SHA384                        = 8,    /**< SHA384 of digest type */
    DIGEST_TYPE_SHA512                        = 9,    /**< SHA512 of digest type */
    DIGEST_TYPE_SHA224                        = 10,   /**< SHA224 of digest type */
    DIGEST_TYPE_MD4                           = 11,   /**< MD4 of digest type */                   
} email_digest_type;

typedef enum {
    EMAIL_AUTHENTICATION_METHOD_NO_AUTH          = 0,    /**< The authentication method is no auth */
    EMAIL_AUTHENTICATION_METHOD_DEFAULT          = 1,    /**< The authentication method is default(SSL/TLS) */
    EMAIL_AUTHENTICATION_METHOD_XOAUTH2          = 2,    /**< The authentication method is xoauth2 */
} email_authentication_method_t;

typedef enum {
    EMAIL_ROAMING_OPTION_RESTRICTED_BACKGROUND_TASK  = 0,    /**< The roaming option is restricted background task */
    EMAIL_ROAMING_OPTION_UNRESTRICTED                = 1,    /**< The roaming option is unrestricted */
} email_roaming_option_t;

typedef enum {
    EMAIL_GET_INCOMING_PASSWORD_LENGTH = 1,    /**< Length of receiving password */
    EMAIL_GET_OUTGOING_PASSWORD_LENGTH         /**< Length of SMTP password */
} email_get_password_length_type;

/*****************************************************************************/
/*  Data Structures                                                          */
/*****************************************************************************/

/**
 * @brief The structure type to save the mail time.
 * @since_tizen 2.3
 */
typedef struct
{
    unsigned short year;                         /**< The Year */
    unsigned short month;                        /**< The Month */
    unsigned short day;                          /**< The Day */
    unsigned short hour;                         /**< The Hour */
    unsigned short minute;                       /**< The Minute */
    unsigned short second;                       /**< The Second */
} email_datetime_t DEPRECATED;

/**
 * @brief The structure type to save the options.
 * @since_tizen 2.3
 */
typedef struct
{
    email_mail_priority_t              priority;                /**< The priority. 1=high 3=normal 5=low */
    int                                keep_local_copy;         /**< Saves a copy after sending */
    int                                req_delivery_receipt;    /**< The request of delivery report. 0=off 1=on */
    int                                req_read_receipt;        /**< The request of read receipt. 0=off 1=on */
    int                                download_limit;          /**< The limit of size for downloading */
    int                                block_address;           /**< Specifies the blocking of address. 0=off 1=on */
    int                                block_subject;           /**< The blocking of subject. 0=off 1=on */
    char                              *display_name_from;       /**< The display name of from */
    int                                reply_with_body;         /**< The replying with body 0=off 1=on */
    int                                forward_with_files;      /**< The forwarding with files 0=off 1=on */
    int                                add_myname_card;         /**< The adding name card 0=off 1=on */
    int                                add_signature;           /**< The adding signature 0=off 1=on */
    char                              *signature;               /**< The signature */
    email_add_my_address_option_type   add_my_address_to_bcc;   /**< The flag specifying whether cc or bcc field should be always filled with my address */
    int                                notification_status;     /**< The notification status. 1 = ON, 0 = OFF */
    int                                vibrate_status;          /**< The repetition type */
    int                                display_content_status;  /**< The display_content status. 1 = ON, 0 = OFF */
    int                                default_ringtone_status; /**< The badge status.  1 = ON, 0 = OFF */
    char                               *alert_ringtone_path;    /**< The ringtone path */
} email_option_t;

/**
 * @brief The structure type to save the information of an email account.
 * @since_tizen 2.3
 */
typedef struct
{
    /* General account information */
    int                             account_id;                               /**< Account ID */
    char                           *account_name;                             /**< Account name */
    int                             account_svc_id;                           /**< AccountSvc priv data: Specifies ID from account-svc */
    int                             sync_status;                              /**< Sync Status. SYNC_STATUS_FINISHED, SYNC_STATUS_SYNCING, SYNC_STATUS_HAVE_NEW_MAILS */
    int                             sync_disabled;                            /**< Flag to enable or disable syncing. If this attribute is set as true, email-service will not synchronize this account. */
    int                             default_mail_slot_size;                   /**< Synced mail count in mailbox */
    char                           *logo_icon_path;                           /**< Account logo icon (used by account svc and email app) */
    email_roaming_option_t          roaming_option;                           /**< Roaming option */
    int                             color_label;                              /**< Account color label */
    void                           *user_data;                                /**< Binary user data */
    int                             user_data_length;                         /**< User data length */

    /* User information */
    char                           *user_display_name;                        /**< User's display */
    char                           *user_email_address;                       /**< User's email address */
    char                           *reply_to_address;                         /**< Email address for reply */
    char                           *return_address;                           /**< Email address for error from server*/

    /* Configuration for incoming server */
    email_account_server_t          incoming_server_type;                     /**< Incoming server type */
    char                           *incoming_server_address;                  /**< Incoming server address */
    int                             incoming_server_port_number;              /**< Incoming server port number */
    char                           *incoming_server_user_name;                /**< Incoming server user name */
    char                           *incoming_server_password;                 /**< Incoming server password */
    int                             incoming_server_secure_connection;        /**< Flag indicating whether incoming server requires secured connection */
    email_authentication_method_t   incoming_server_authentication_method;    /**< Incoming server authentication method */

    /* Options for incoming server */
    email_imap4_retrieval_mode_t    retrieval_mode;                           /**< Retrieval mode : EMAIL_IMAP4_RETRIEVAL_MODE_NEW or EMAIL_IMAP4_RETRIEVAL_MODE_ALL */
    int                             keep_mails_on_pop_server_after_download;  /**< Keep mails on POP server after download */
    int                             check_interval;                           /**< The interval for checking new mail periodically */
    int                             auto_download_size;                       /**< The size for auto download in bytes. @c -1 means entire mails body */
    int                             peak_interval;                            /**< The interval for checking new mail periodically of peak schedule */
    int                             peak_days;                                /**< The weekdays of peak schedule */
    int                             peak_start_time;                          /**< The start time of peak schedule */
    int                             peak_end_time;                            /**< The end time of peak schedule */

    /* Configuration for outgoing server */
    email_account_server_t          outgoing_server_type;                     /**< Outgoing server type */
    char                           *outgoing_server_address;                  /**< Outgoing server address */
    int                             outgoing_server_port_number;              /**< Outgoing server port number */
    char                           *outgoing_server_user_name;                /**< Outgoing server user name */
    char                           *outgoing_server_password;                 /**< Outgoing server password */
    int                             outgoing_server_secure_connection;        /**< Flag indicating whether outgoing server requires secured connection */
    int                             outgoing_server_need_authentication;      /**< Flag indicating whether outgoing server requires authentication */
    int                             outgoing_server_use_same_authenticator;   /**< Use same authenticator for outgoing server - email_authentication_method_t - */


    /* Options for outgoing server */
    email_option_t                  options;                                  /**< Account options for setting */
    int                             auto_resend_times;                        /**< Auto retry count for sending a email */
    int                             outgoing_server_size_limit;               /**< Mail size limitation for SMTP sending */

    /* Auto download */
    int                             wifi_auto_download;                        /**< Auto attachment download in WiFi connection */

    /* Authentication Options */
    int                             pop_before_smtp;                          /**< POP before SMTP Authentication */
    int                             incoming_server_requires_apop;            /**< APOP authentication */

    /* S/MIME Options */
    email_smime_type                smime_type;                               /**< The smime type 0=Normal 1=Clear signed 2=encrypted 3=Signed + encrypted */
    char                           *certificate_path;                         /**< The certificate path of private */
    email_cipher_type               cipher_type;                              /**< The encryption algorithm */
    email_digest_type               digest_type;                              /**< The digest algorithm */
	char                           *user_name;                                /**< The user name for multi user (Since 2.4) */   
} email_account_t;

/**
 * @brief The structure type to save the certificate information.
 * @since_tizen 2.3
 */

typedef struct 
{
    int certificate_id;               /**< The saved certificate ID */
    int issue_year;                   /**< The issue year information of certificate */
    int issue_month;                  /**< The issue month information of certificate */
    int issue_day;                    /**< The issue day information of certificate */
    int expiration_year;              /**< The expiration year information of certificate */
    int expiration_month;             /**< The expiration month information of certificate */
    int expiration_day;               /**< The expiration day information of certificate */
    char *issue_organization_name;    /**< The issue organization information of certificate */
    char *email_address;              /**< The email address of certificate */
    char *subject_str;                /**< The subject information of certificate */
    char *filepath;                   /**< The saved path of certificate */
} email_certificate_t;

/**
 * @brief The structure type to save the email server information.
 * @since_tizen 2.3
 */

typedef struct
{
    int                         configuration_id;      /**< The configuration ID */
    email_account_server_t      protocol_type;         /**< The type of configuration */
    char                       *server_addr;           /**< The address of configuration */
    int                         port_number;           /**< The port number of configuration */
    int                         security_type;         /**< The security such as SSL */
    int                         auth_type;             /**< The authentication type of configuration */
} email_protocol_config_t;

typedef struct
{
    char                       *service_name;           /**< The name of service */
    int                         authname_format;        /**< The type of user name for authentication */
    int                         protocol_conf_count;    /**< The count of protocol configurations */
    email_protocol_config_t    *protocol_config_array;  /**< The array of protocol configurations */
} email_server_info_t;

typedef struct
{
    int   mailbox_type;                         /**< The mailbox type  */
    char  mailbox_name[MAILBOX_NAME_LENGTH];    /**< The mailbox name */
} email_mailbox_type_item_t;

/**
 * @brief The structure type which contains the Mail information.
 * @since_tizen 2.3
 */

typedef struct
{
    int                   mail_id;                 /**< The Mail ID */
    int                   account_id;              /**< The Account ID */
    int                   mailbox_id;              /**< The mailbox ID */
    email_mailbox_type_e  mailbox_type;            /**< The mailbox type of the mail */
    char                 *subject;                 /**< The subject */
    time_t                date_time;               /**< The Date time */
    int                   server_mail_status;      /**< The flag indicating whether sever mail or not */
    char                 *server_mailbox_name;     /**< The server mailbox */
    char                 *server_mail_id;          /**< The Server Mail ID */
    char                 *message_id;              /**< The message ID */
    int                   reference_mail_id;       /**< The reference mail ID */
    char                 *full_address_from;       /**< The From address */
    char                 *full_address_reply;      /**< The Reply to address */
    char                 *full_address_to;         /**< The To address */
    char                 *full_address_cc;         /**< The CC address */
    char                 *full_address_bcc;        /**< The BCC address */
    char                 *full_address_return;     /**< The return path */
    char                 *email_address_sender;    /**< The email address of sender */
    char                 *email_address_recipient; /**< The email address of recipients */
    char                 *alias_sender;            /**< The alias of sender */
    char                 *alias_recipient;         /**< The alias of recipients */
    int                   body_download_status;    /**< The text download status */
    char                 *file_path_plain;         /**< The path of text mail body */
    char                 *file_path_html;          /**< The path of HTML mail body */
    char                 *file_path_mime_entity;   /**< The path of MIME entity */
    int                   mail_size;               /**< The mail size */
    char                  flags_seen_field;        /**< The seen flags */
    char                  flags_deleted_field;     /**< The deleted flags */
    char                  flags_flagged_field;     /**< The flagged flags */
    char                  flags_answered_field;    /**< The answered flags */
    char                  flags_recent_field;      /**< The recent flags */
    char                  flags_draft_field;       /**< The draft flags */
    char                  flags_forwarded_field;   /**< The forwarded flags */
    int                   DRM_status;              /**< The flag indicating whether the mail has DRM content (1 : true, 0 : false) */
    email_mail_priority_t priority;                /**< The priority of the mail */
    email_mail_status_t   save_status;             /**< The save status */
    int                   lock_status;             /**< The mail is locked */
    email_mail_report_t   report_status;           /**< The mail report */
    int                   attachment_count;        /**< The attachment count */
    int                   inline_content_count;    /**< The inline content count */
    int                   thread_id;               /**< The thread ID for thread view */
    int                   thread_item_count;       /**< The item count of specific thread */
    char                 *preview_text;            /**< The preview body */
    email_mail_type_t     meeting_request_status;  /**< The status of meeting request */
    int                   message_class;           /**< The class of message for EAS */ /* email_message_class */
    email_digest_type     digest_type;             /**< The digest algorithm */
    email_smime_type      smime_type;              /**< The SMIME type */
    time_t                scheduled_sending_time;  /**< The scheduled sending time */
    int                   remaining_resend_times;  /**< The remaining resend times */
    int                   tag_id;                  /**< The data for filtering */
    time_t                replied_time;            /**< The time of replied */
    time_t                forwarded_time;          /**< The time of forwarded */
    char                 *pgp_password;            /**< The password of PGP. */
    char                 *user_name;               /**< The user information for multi user (Since 2.4) */
	char                 *key_list;                /**< The key list encryption of mail (Since 2.4) */
	char                 *key_id;                  /**< The key ID for signing of pgp mail (Since 2.4) */
    int                   eas_data_length;         /**< The length of eas_data */
    void                 *eas_data;                /**< Extended Application Specific data */
} email_mail_data_t;

/**
 * @brief The structure type which contains information for displaying a mail list.
 * @since_tizen 2.3
 */
typedef struct
{
    int                   mail_id;                                            /**< The mail ID */
    int                   account_id;                                         /**< The account ID */
    int                   mailbox_id;                                         /**< The mailbox ID */
    email_mailbox_type_e  mailbox_type;                                       /**< The mailbox type of the mail */
    char                  full_address_from[STRING_LENGTH_FOR_DISPLAY];       /**< The full from email address */
    char                  email_address_sender[MAX_EMAIL_ADDRESS_LENGTH];     /**< The sender email address */
    char                  email_address_recipient[STRING_LENGTH_FOR_DISPLAY]; /**< The recipients email address */
    char                  subject[STRING_LENGTH_FOR_DISPLAY];                 /**< The subject */
    int                   body_download_status;                               /**< The text download status */
    int                   mail_size;                                          /**< The mail size */
    time_t                date_time;                                          /**< The date time */
    char                  flags_seen_field;                                   /**< The seen flags */
    char                  flags_deleted_field;                                /**< The deleted flags */
    char                  flags_flagged_field;                                /**< The flagged flags */
    char                  flags_answered_field;                               /**< The answered flags */
    char                  flags_recent_field;                                 /**< The recent flags */
    char                  flags_draft_field;                                  /**< The draft flags */
    char                  flags_forwarded_field;                              /**< The forwarded flags */
    int                   DRM_status;                                         /**< The flag indicating whether the has mail DRM content (1 : true, 0 : false) */
    email_mail_priority_t priority;                                           /**< The priority of mails */ /* email_mail_priority_t*/
    email_mail_status_t   save_status;                                        /**< The save status */ /* email_mail_status_t */
    int                   lock_status;                                        /**< The lock status */
    email_mail_report_t   report_status;                                      /**< The mail report */ /* email_mail_report_t */
    int                   attachment_count;                                   /**< The attachment count */
    int                   inline_content_count;                               /**< The inline content count */
    char                  preview_text[MAX_PREVIEW_TEXT_LENGTH];              /**< The text for preview body */
    int                   thread_id;                                          /**< The thread ID for thread view */
    int                   thread_item_count;                                  /**< The item count of specific thread */
    email_mail_type_t     meeting_request_status;                             /**< The flag indicating whether the mail is a meeting request or not */ /* email_mail_type_t */
    int                   message_class;                                      /**< The message class */ /* email_message_class */
    email_smime_type      smime_type;                                         /**< The smime type */ /* email_smime_type */
    time_t                scheduled_sending_time;                             /**< The scheduled sending time */
    int                   remaining_resend_times;                             /**< The remaining resend times */
    int                   tag_id;                                             /**< The data for filtering */
    int                   eas_data_length;                                    /**< The length of eas_data */
    void                 *eas_data;                                           /**< Extended Application Specific data */
} email_mail_list_item_t;

/**
 * @brief The structure type used to save the filtering structure.
 * @since_tizen 2.3
 */
typedef struct
{
    int                  account_id;          /**< The account ID */
    int                  filter_id;           /**< The filtering ID */
    char                *filter_name;         /**< The filtering name */
    email_rule_type_t    type;                /**< The filtering type */
    char                *value;               /**< The filtering value : subject */
    char                *value2;              /**< The filtering value2 : sender address */
    email_rule_action_t  faction;             /**< The action type for filtering */
    int                  target_mailbox_id;   /**< The mail box if action type means move */
    int                  flag1;               /**< The activation */
    int                  flag2;               /**< Reserved */
} email_rule_t;

/**
 * @brief The structure type used to save the information of a mail flag.
 * @since_tizen 2.3
 */
typedef struct
{
    unsigned int  seen           : 1;    /**< The read email */
    unsigned int  deleted        : 1;    /**< The deleted email */
    unsigned int  flagged        : 1;    /**< The flagged email */
    unsigned int  answered       : 1;    /**< The answered email */
    unsigned int  recent         : 1;    /**< The recent email */
    unsigned int  draft          : 1;    /**< The draft email */
    unsigned int  attachment_count : 1;  /**< The attachment count */
    unsigned int  forwarded      : 1;    /**< The forwarded email. */
    unsigned int  sticky         : 1;    /**< The sticky flagged mails cannot be deleted */
} email_mail_flag_t /* DEPRECATED */;


/**
 * @brief The structure type used to save the information of a mail extra flag.
 * @since_tizen 2.3
 */
typedef struct
{
    unsigned int  priority         : 3; /**< The mail priority \n
                                             The value is greater than or equal to EMAIL_MAIL_PRIORITY_HIGH.
                                             The value is less than or equal to EMAIL_MAIL_PRIORITY_LOW.*/
    unsigned int  status           : 4; /**< The mail status \n
                                             The value is a value of enumeration email_mail_status_t.*/
    unsigned int  noti             : 1; /**< The notified mail */
    unsigned int  lock             : 1; /**< The locked mail */
    unsigned int  report           : 2; /**< The MDN/DSN mail \n 
                                             The value is a value of enumeration email_mail_report_t.*/
    unsigned int  drm              : 1; /**< The DRM mail */
    unsigned int  text_download_yn : 2; /**< The body download y/n */
} email_extra_flag_t DEPRECATED;

/**
 * @brief The structure type used to save the information of an attachment.
 * @since_tizen 2.3
 */
typedef struct
{
    int   attachment_id;         /**< The attachment ID */
    char *attachment_name;       /**< The attachment name */
    char *attachment_path;       /**< The path of a downloaded attachment */
    char *content_id;            /**< The content ID string of an attachment */
    int   attachment_size;       /**< The size of an attachment */
    int   mail_id;               /**< The mail ID of an included attachment */
    int   account_id;            /**< The account ID of an included attachment */
    char  mailbox_id;            /**< The mailbox ID of an include attachment */
    int   save_status;           /**< The save status of an attachment */
    int   drm_status;            /**< The DRM status of an attachment */
    int   inline_content_status; /**< The distinguished inline attachment and normal attachment */
    char *attachment_mime_type;  /**< The context MIME type */
} email_attachment_data_t;

typedef struct
{
    int        offset_from_GMT;             /**< The time from base GMT  */
    char       standard_name[32];           /**< The name of time zone */
    struct tm  standard_time_start_date;    /**< The start date of time zone */
    int        standard_bias;               /**< The bias of time zone */
    char       daylight_name[32];           /**< The daylight name of time zone */
    struct tm  daylight_time_start_date;    /**< The daylight start date of time zone */
    int        daylight_bias;               /**< The daylight bias of time zone */
} email_time_zone_t;

/**
 * @brief Structure used to save the information of a meeting request
 * @since_tizen 2.3
 */
typedef struct
{
    int                        mail_id;                                        /**< The mail ID of meeting request on DB <br> 
                                                                                    This is the primary key. */
    email_meeting_response_t   meeting_response;                               /**< The meeting response */
    struct tm                  start_time;                                     /**< The meeting start time */
    struct tm                  end_time;                                       /**< The meeting end time */
    char                      *location;                                       /**< The location of meeting <br> 
                                                                                    Maximum length of this string is 32768. */
    char                      *global_object_id;                               /**< The object ID */
    email_time_zone_t          time_zone;                                      /**< The time zone of meeting */
} email_meeting_request_t;

/**
 * @brief The structure type used to save the information of a sender list with unread/total mail counts.
 * @since_tizen 2.3
 */
typedef struct
{
    char *address;         /**< The address of a sender */
    char *display_name;    /**< The display name <br> 
                                This may be one of contact name, alias in original mail and email address of sender. 
                                (Priority order : contact name, alias, email address) */
    int   unread_count;    /**< The number of unread mails received from sender address */
    int   total_count;     /**< The total number of  mails which are received from sender address */
} email_sender_list_t /* DEPRECATED */;


/**
 * @brief The structure type used to save the information of a mailbox.
 * @since_tizen 2.3
 */
typedef struct
{
    int                   mailbox_id;                 /**< The unique ID on mailbox table */
    char                 *mailbox_name;               /**< The path of mailbox */
    email_mailbox_type_e  mailbox_type;               /**< The type of mailbox */
    char                 *alias;                      /**< The display name of mailbox */
    int                   unread_count;               /**< The unread mail count in the mailbox */
    int                   total_mail_count_on_local;  /**< The total number of mails in the mailbox in the local DB */
    int                   total_mail_count_on_server; /**< The total number of mails in the mailbox in the mail server */
    int                   local;                      /**< The local mailbox */
    int                   account_id;                 /**< The account ID for mailbox */
    int                   mail_slot_size;             /**< The number of mails that can be stored in local mailbox */
    int                   no_select;                  /**< The 'no_select' attribute from xlist */
    time_t                last_sync_time;             /**< The last sync time of the mailbox */
    int                   deleted_flag;               /**< The flag specifying whether mailbox is deleted */
    int                   eas_data_length;            /**< The length of eas_data */
    void                 *eas_data;                   /**< The data for eas engine. */
} email_mailbox_t;

/**
 * @brief Structure used to save the information of email a address.
 * @since_tizen 2.3
 */

typedef struct
{
    email_address_type_t  address_type;    /**< The address type using the email (TO, CC, BCC, etc..) */
    char                 *address;         /**< The Email address */
    char                 *display_name;    /**< The alias */
    int                   storage_type;    /**< The type of saved storage on contact-service */
    int                   contact_id;      /**< The ID of saved storage on contact-service */
} email_address_info_t;

/**
 * @brief Structure used to save the set of email addresses
 * @since_tizen 2.3
 */
typedef struct
{
    GList *from;    /**< The From address list */
    GList *to;      /**< The TO address list */
    GList *cc;      /**< The CC address list */
    GList *bcc;     /**< The BCC address list */
} email_address_info_list_t;

/**
 * @brief Structure used to save the list of email addresses
 * @since_tizen 2.3
 */
typedef struct
{
    int address_type;               /**< The type of mail (sender : 0, recipient : 1)*/
    int     address_count;          /**< The number of email addresses */
    char  **address_list;           /**< The strings of email addresses */
} email_email_address_list_t;


/**
 * @brief Structure used to save the information of a search filter
 * @since_tizen 2.3
 */
typedef struct _email_search_filter_t {
    email_search_filter_type search_filter_type;      /**< The type of search filter */
    union {
        int            integer_type_key_value;        /**< The integer value for searching */
        time_t         time_type_key_value;           /**< The time value for searching */
        char          *string_type_key_value;         /**< The string value for searching */
    } search_filter_key_value;                        /**< The key value of search filter */
} email_search_filter_t;

typedef enum {
    EMAIL_LIST_FILTER_RULE_EQUAL                  = 0,    /**< EQUAL(=) of filter type */
    EMAIL_LIST_FILTER_RULE_NOT_EQUAL              = 1,    /**< NOT EQUAL(!=) of filter type */
    EMAIL_LIST_FILTER_RULE_LESS_THAN              = 2,    /**< LESS THAN(<) of filter type */
    EMAIL_LIST_FILTER_RULE_GREATER_THAN           = 3,    /**< GREATER THAN(>) of filter type */
    EMAIL_LIST_FILTER_RULE_LESS_THAN_OR_EQUAL     = 4,    /**< LESS THAN OR EQUAL(<=) of filter type */
    EMAIL_LIST_FILTER_RULE_GREATER_THAN_OR_EQUAL  = 5,    /**< GREATER THAN OR EQUAL(>=) of filter type */
    EMAIL_LIST_FILTER_RULE_INCLUDE                = 6,    /**< LIKE operator in SQL statements */
    EMAIL_LIST_FILTER_RULE_MATCH                  = 7,    /**< MATCH operator in SQL statements */
    EMAIL_LIST_FILTER_RULE_IN                     = 8,    /**< IN operator in SQL statements */
    EMAIL_LIST_FILTER_RULE_NOT_IN                 = 9     /**< NOT IN operator in SQL statements */
} email_list_filter_rule_type_t;

typedef enum {
    EMAIL_CASE_SENSITIVE                          = 0,    /**< Case sensitive search */
    EMAIL_CASE_INSENSITIVE                        = 1,    /**< Case insensitive search */
} email_list_filter_case_sensitivity_t;

typedef union {
    int                                    integer_type_value;    /**< Container for integer type value */
    char                                  *string_type_value;     /**< Container for string type value  */
    time_t                                 datetime_type_value;   /**< Container for datetime type value  */
} email_mail_attribute_value_t;

typedef struct {
    email_list_filter_rule_type_t          rule_type;             /**< Operator type of the filter rule */
    email_mail_attribute_type              target_attribute;      /**< Target attribute type of the filter rule */
    email_mail_attribute_value_t           key_value;             /**< Target attribute value of the filter rule */
    email_list_filter_case_sensitivity_t   case_sensitivity;      /**< Specifies case sensitivity of the filter rule */
} email_list_filter_rule_t;

typedef struct {
    email_list_filter_rule_type_t          rule_type;             /**< Operator type of the filter rule for full text search */
    email_mail_text_attribute_type         target_attribute;      /**< Target attribute type of the filter rule */
    email_mail_attribute_value_t           key_value;             /**< Target attribute value of the filter rule */
} email_list_filter_rule_fts_t;

typedef struct {
    email_list_filter_rule_type_t          rule_type;             /**< Operator type of the filter rule for searching attachment */
    email_mail_attach_attribute_type       target_attribute;      /**< Target attribute type of the filter rule */
    email_mail_attribute_value_t           key_value;             /**< Target attribute value of the filter rule */
    email_list_filter_case_sensitivity_t   case_sensitivity;      /**< Specifies case sensitivity of the filter rule */
} email_list_filter_rule_attach_t;

typedef enum {
    EMAIL_LIST_FILTER_ITEM_RULE                     = 0,    /**< Normal search rule */
    EMAIL_LIST_FILTER_ITEM_RULE_FTS                 = 1,    /**< Full text search rule */
    EMAIL_LIST_FILTER_ITEM_RULE_ATTACH              = 2,    /**< Searching attachment rule */
    EMAIL_LIST_FILTER_ITEM_OPERATOR                 = 3,    /**< Operator */
} email_list_filter_item_type_t;

typedef enum {
    EMAIL_LIST_FILTER_OPERATOR_AND                  = 0,    /**< AND operator */
    EMAIL_LIST_FILTER_OPERATOR_OR                   = 1,    /**< OR operator */
    EMAIL_LIST_FILTER_OPERATOR_LEFT_PARENTHESIS     = 2,    /**< ( operator */
    EMAIL_LIST_FILTER_OPERATOR_RIGHT_PARENTHESIS    = 3     /**< ) operator */
} email_list_filter_operator_type_t;

/**
 * @brief Structure used to save the information of a list filter
 * @since_tizen 2.3
 */
typedef struct {
    email_list_filter_item_type_t          list_filter_item_type;    /**< Filter item type */

    union {
        email_list_filter_rule_t           rule;           /**< Rule type */
        email_list_filter_rule_fts_t       rule_fts;       /**< Rule type for full text search */
        email_list_filter_rule_attach_t    rule_attach;    /**< Rule type for searching attachment */
        email_list_filter_operator_type_t  operator_type;  /**< Operator type */
    } list_filter_item;    /**< filter item of list */

} email_list_filter_t;

typedef enum {
    EMAIL_SORT_ORDER_ASCEND                             = 0,    /**< Ascending order sorting */
    EMAIL_SORT_ORDER_DESCEND                            = 1,    /**< Descending order sorting */
    EMAIL_SORT_ORDER_TO_CCBCC                           = 2,    /**< full_address_to sorting */
    EMAIL_SORT_ORDER_TO_CC_BCC                          = 3,    /**< full_address_to, full_address_cc sorting */
    EMAIL_SORT_ORDER_TO_CCBCC_ALL                       = 4,    /**< full_address_to of all account sorting */
    EMAIL_SORT_ORDER_NOCASE_ASCEND                      = 5,    /**< Ascending order with COLLATE NOCASE option*/
    EMAIL_SORT_ORDER_NOCASE_DESCEND                     = 6,    /**< Descending order with COLLATE NOCASE option */
    EMAIL_SORT_ORDER_LOCALIZE_ASCEND                    = 7,    /**< Localize ascending order sorting */
    EMAIL_SORT_ORDER_LOCALIZE_DESCEND                   = 8     /**< Localize descending order sorting */
} email_list_filter_sort_order_t;


typedef struct {
    email_mail_attribute_type              target_attribute;    /**< The attribute type of mail for sorting*/
    email_mail_attribute_value_t           key_value;           /**< The attribute value of mail for sorting*/
    bool                                   force_boolean_check; /**< The distinguished true/false for sorting  */
    email_list_filter_sort_order_t         sort_order;          /**< The ordering rule for sorting */
} email_list_sorting_rule_t;


typedef struct {
    int                                    handle;        /**< The job handle to be canceled */
    int                                    account_id;    /**< The account ID for task information */
    email_event_type_t                     type;          /**< The type for task information */
    email_event_status_type_t              status;        /**< The status for task information */
} email_task_information_t;

typedef enum {
    EMAIL_MESSAGE_CONTEXT_VOICE_MESSAGE                 = 1,    /**< Voice message context */
    EMAIL_MESSAGE_CONTEXT_VIDEO_MESSAGE                 = 2,    /**< Video message context */
    EMAIL_MESSAGE_CONTEXT_FAX_MESSAGE                   = 3,    /**< Fax message context */
    EMAIL_MESSAGE_CONTEXT_X_EMPTY_CALL_CAPTURE_MESSAGE  = 4,    /**< X empty call capture message context */
    EMAIL_MESSAGE_CONTEXT_X_NUMBER_MESSAGE              = 5,    /**< X number message context */
    EMAIL_MESSAGE_CONTEXT_X_VOICE_INFOTAINMENT_MESSAGE  = 6     /**< X voice infotainment message context */
} email_message_context_t;

typedef enum {
    EMAIL_MESSAGE_SENSITIVITY_PRIVATE                   = 1,    /**< Private sensitivity of message */
    EMAIL_MESSAGE_SENSITIVITY_CONFIDENTIAL              = 2,    /**< Confidential sensitivity of message */
    EMAIL_MESSAGE_SENSITIVITY_PERSONAL                  = 3     /**< Personal sensitivity of message */
} email_message_sensitivity_t;

typedef struct {
    email_message_context_t                message_context;    /**< Message context for visual voice mail */
    int                                    content_length;     /**< Content-Duration for audio/video or X-Content-Pages for fax */
    email_message_sensitivity_t            sensitivity;        /**< Sensitivity of message for visual voice mail */
} email_vvm_specific_data_t;

typedef struct {
    char *quota_root;       /**< Quota root for IMAP quota */
    char *resource_name;    /**< Resource name for IMAP quota */
    int   usage;            /**< Usage for IMAP quota */
    int   limits;           /**< Limit for IMAP quota */
} email_quota_resource_t;

/*****************************************************************************/
/*  For Active Sync                                                          */
/*****************************************************************************/

#define VCONFKEY_EMAIL_SERVICE_ACTIVE_SYNC_HANDLE   "db/email_handle/active_sync_handle"
#define EMAIL_ACTIVE_SYNC_NOTI                      "User.Email.ActiveSync"

typedef enum
{
    ACTIVE_SYNC_NOTI_SEND_MAIL,                                   /**<  Sending notification to ASE (active sync engine) */
    ACTIVE_SYNC_NOTI_SEND_SAVED,                                  /**<  Sending notification to ASE (active sync engine), all saved mails should be sent */
    ACTIVE_SYNC_NOTI_SEND_REPORT,                                 /**<  Sending notification to ASE (active sync engine), report should be sent */
    ACTIVE_SYNC_NOTI_SYNC_HEADER,                                 /**<  Sync header - download mails from server <br>
                                                                        It depends on account/s flag1 field whether it executes downloading header only or downloading header + body
                                                                        downloading option : 0 is subject only, 1 is text body, 2 is normal */
    ACTIVE_SYNC_NOTI_DOWNLOAD_BODY,                               /**<  Downloading body notification to AS */
    ACTIVE_SYNC_NOTI_DOWNLOAD_ATTACHMENT,                         /**<  Downloading attachment notification to AS */
    ACTIVE_SYNC_NOTI_VALIDATE_ACCOUNT,                            /**<  Account validating notification to AS */
    ACTIVE_SYNC_NOTI_CANCEL_JOB,                                  /**<  Canceling job notification to AS */
    ACTIVE_SYNC_NOTI_SEARCH_ON_SERVER,                            /**<  Searching on server notification to AS */
    ACTIVE_SYNC_NOTI_CLEAR_RESULT_OF_SEARCH_ON_SERVER,            /**<  Notification for clearing result of search on server to AS */
    ACTIVE_SYNC_NOTI_EXPUNGE_MAILS_DELETED_FLAGGED,               /**<  Notification to expunge deleted flagged mails */
    ACTIVE_SYNC_NOTI_RESOLVE_RECIPIENT,                           /**<  Notification to get the resolve recipients */
    ACTIVE_SYNC_NOTI_VALIDATE_CERTIFICATE,                        /**<  Notification to validate certificate */
    ACTIVE_SYNC_NOTI_ADD_MAILBOX,                                 /**<  Notification to add mailbox */
    ACTIVE_SYNC_NOTI_RENAME_MAILBOX,                              /**<  Notification to rename mailbox */
    ACTIVE_SYNC_NOTI_DELETE_MAILBOX,                              /**<  Notification to delete mailbox */
    ACTIVE_SYNC_NOTI_CANCEL_SENDING_MAIL,                         /**<  Notification to cancel a sending mail */
    ACTIVE_SYNC_NOTI_DELETE_MAILBOX_EX,                           /**<  Notification to delete multiple mailboxes */
    ACTIVE_SYNC_NOTI_SEND_MAIL_WITH_DOWNLOADING_OF_ORIGINAL_MAIL, /**<  Notification to send a mail with downloading attachment of original mail */
    ACTIVE_SYNC_NOTI_SCHEDULE_SENDING_MAIL,                       /**<  Notification to schedule a mail to send later*/
}   eactivesync_noti_t;

typedef union
{
    struct _send_mail
    {
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for sending the mail */
        int                     mail_id;           /**< The mail ID for sending the mail */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */    
	} send_mail;    /**< Noti data for sending the mail */

    struct _send_mail_saved
    {/*  not defined ye */
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for sending the saved mail */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */    
	} send_mail_saved;    /**< Noti data for sending the saved mail */

    struct _send_report
    {/*  not defined ye */
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for sending the report */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } send_report;    /**< Noti data for sending the report */

    struct _sync_header
    {
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for syncing the header */
        int                     mailbox_id;        /**< The mailbox ID for syncing the header */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } sync_header;    /**< Noti data for syncing the header */

    struct _download_body
    {
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for downloading the body */
        int                     mail_id;           /**< The mail ID for downloading the body */
        int                     with_attachment;   /**< 0: without attachments, 1: with attachment */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } download_body;    /**< Noti data for downloading the body */

    struct _download_attachment
    {
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for downloading the attachment */
        int                     mail_id;           /**< The mail ID for downloading the attachment */
        int                     attachment_order;  /**< The ordered attachment for downloading the attachment */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } download_attachment;    /**< Noti data for downloading the attachment */

    struct _cancel_job
    {
        int                     account_id;        /**< The account ID for canceling the job */
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     cancel_type;       /**< The canceling type for canceling the job */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } cancel_job;    /**< Noti data for canceling the job */

    struct _validate_account
    {/*  not defined yet */
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for validating the account */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } validate_account;    /**< Noti data for validating the account */

    struct _search_mail_on_server
    {
        int                     handle;              /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;          /**< The account ID for searching the mail on server */
        int                     mailbox_id;          /**< The mailbox ID for searching the mail on server */
        email_search_filter_t  *search_filter_list;  /**< The list of search filter for searching the mail on server */
        int                     search_filter_count; /**< The count of search filter for searching the mail on server */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } search_mail_on_server;    /**< Noti data for searching the mail on server */

    struct _clear_result_of_search_mail_on_server
    { 
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for clearing the result of search mail on server */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } clear_result_of_search_mail_on_server;    /**< Noti data for clearing the result of search mail on server */

    struct _expunge_mails_deleted_flagged
    {
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for expunging the mails flagged for deleting */
        int                     mailbox_id;        /**< The mailbox ID for expunging the mails flagged for deleting */
        int                     on_server;         /**< The flag indicating whether the mail is on server for expunging the mails flagged for deleting */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } expunge_mails_deleted_flagged;    /**< Noti data for expunging the mails flagged for deleting */

    struct _get_resolve_recipients
    {
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for getting the resolve recipients */
        char                   *email_address;     /**< The email address for getting the resolve recipients */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } get_resolve_recipients;    /**< Noti data for getting the resolve recipients */

    struct _validate_certificate
    {
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for validating the certificate */
        char                   *email_address;     /**< The email address for validating the certifiate */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } validate_certificate;    /**< Noti data for validating the certificate */

    struct _add_mailbox
    {
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for adding the mailbox */
        char                   *mailbox_path;      /**< The path of mailbox for adding the mailbox */
        char                   *mailbox_alias;     /**< The alias of mailbox for adding the mailbox */
        void                   *eas_data;          /**< The eas-data for adding the mailbox */
        int                     eas_data_length;   /**< The length of eas-data for adding the mailbox */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } add_mailbox;    /**< Noti data for adding the mailbox */

    struct _rename_mailbox
    {
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for renaming the mailbox */
        int                     mailbox_id;        /**< The mailbox ID for renaming the mailbox */
        char                   *mailbox_name;      /**< The mailbox name for renaming the mailbox */
        char                   *mailbox_alias;     /**< The alias of mailbox for renaming the mailbox */
        void                   *eas_data;          /**< The eas-data for renaming the mailbox */
        int                     eas_data_length;   /**< The length of eas-data for renaming the mailbox */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } rename_mailbox;    /**< Noti data for renaming the mailbox */

    struct _delete_mailbox
    {
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for deleting mailbox */
        int                     mailbox_id;        /**< The mailbox ID for deleting mailbox */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } delete_mailbox;    /**< Noti data for deleting mailbox */

    struct _cancel_sending_mail
    {
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for canceled sending mail */
        int                     mail_id;           /**< The mail ID for canceled sending mail */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } cancel_sending_mail;    /**< Noti data for canceled sending mail */

    struct _delete_mailbox_ex
    {
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for deleting mailbox (extention) */
        int                    *mailbox_id_array;  /**< The mailbox ID array for deleting mailbox (extention) */
        int                     mailbox_id_count;  /**< The mailbox ID count for deleting mailbox (extention) */
        int                     on_server;         /**< The on server for deleting mailbox (extention) */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } delete_mailbox_ex;    /**< Noti data for deleting mailbox (extention) */

    struct _send_mail_with_downloading_attachment_of_original_mail
    {
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for downloading attachment */
        int                     mail_id;           /**< The mail ID for download attachment */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } send_mail_with_downloading_attachment_of_original_mail;    /**< Noti data for send mail with downloading attachment of original mail */

    struct _schedule_sending_mail
    {
        int                     handle;            /**< The job handle to be canceled. This value is issued by email-service. */
        int                     account_id;        /**< The account ID for scheduled sending mail */
        int                     mail_id;           /**< The mail ID for scheduled sending mail */
        time_t                  scheduled_time;    /**< The scheduled time for scheduled sending mail */
        char                   *multi_user_name;   /**< Speicifes the supporting for multi user (Since 2.4) */
    } schedule_sending_mail;    /**< Noti data for schedule sending mail */
} ASNotiData;

/** @brief Enumeration for noti string types.
 * @since_tizen 2.3 */
typedef enum
{
    EMAIL_CONVERT_STRUCT_TYPE_MAIL_LIST_ITEM,      /**< specifies email_mail_list_t */
} email_convert_struct_type_e;

/** @brief Enumeration for task types.
 * @since_tizen 2.3 */
typedef enum {
    /* Sync tasks */
    EMAIL_SYNC_TASK_BOUNDARY_START                                          = 11000,    /**< Sync task for boundary start */
    /* Sync tasks for account - from 11000 */
    EMAIL_SYNC_TASK_ADD_ACCOUNT                                             = 11010,    /**< Sync task for adding the account */
    EMAIL_SYNC_TASK_DELETE_ACCOUNT                                          = 11020,    /**< Sync task for deleting the account */
    EMAIL_SYNC_TASK_UPDATE_ACCOUNT                                          = 11030,    /**< Sync task for updating the account */
    EMAIL_SYNC_TASK_GET_ACCOUNT                                             = 11040,    /**< Sync task for getting the account */
    EMAIL_SYNC_TASK_GET_ACCOUNT_LIST                                        = 11050,    /**< Sync task for getting the account list */
    EMAIL_SYNC_TASK_BACKUP_ACCOUNTS                                         = 11060,    /**< Sync task for backup the account */
    EMAIL_SYNC_TASK_RESTORE_ACCOUNTS                                        = 11070,    /**< Sync task for restoring the account */
    EMAIL_SYNC_TASK_GET_PASSWORD_LENGTH_OF_ACCOUNT                          = 11090,    /**< Sync task for getting the password length of account */

    /* Sync tasks for mailbox - from 12000 */
    EMAIL_SYNC_TASK_GET_MAILBOX_COUNT                                       = 12010,    /**< Sync task for getting the mailbox count */
    EMAIL_SYNC_TASK_GET_MAILBOX_LIST                                        = 12020,    /**< Sync task for getting the mailbox list */
    EMAIL_SYNC_TASK_GET_SUB_MAILBOX_LIST                                    = 12030,    /**< Sync task for getting the sub-mailbox list */
    EMAIL_SYNC_TASK_SET_MAIL_SLOT_SIZE                                      = 12040,    /**< Sync task for setting the mail slot size */
    EMAIL_SYNC_TASK_SET_MAILBOX_TYPE                                        = 12050,    /**< Sync task for setting the mailbox type */
    EMAIL_SYNC_TASK_SET_LOCAL_MAILBOX                                       = 12060,    /**< Sync task for setting the local mailbox */

    /* Sync tasks for mail - from 13000 */
    EMAIL_SYNC_TASK_GET_ATTACHMENT                                          = 13010,    /**< Sync task for getting the attachment */
    EMAIL_SYNC_TASK_CLEAR_RESULT_OF_SEARCH_MAIL_ON_SERVER                   = 13020,    /**< Sync task for clearing the result of search mail on server */
    EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL                                   = 13030,    /**< Sync task for scheduling the sending mail */
    EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE                                        = 13040,    /**< Sync task for updating the attribute */

    /* Sync tasks for mail thread - from 14000 */

    /* Sync tasks for rule - from 15000 */

    /* Sync tasks for etc - from 16000 */
    EMAIL_SYNC_TASK_BOUNDARY_END                                            = 59999,    /**< Sync task for boundary end */
    /* Async tasks */
    EMAIL_ASYNC_TASK_BOUNDARY_START                                         = 60000,    /**< Async task for boundary start */
    /* Async tasks for account - from 61000 */
    EMAIL_ASYNC_TASK_VALIDATE_ACCOUNT                                       = 61010,    /**< Async task for validating the account */
    EMAIL_ASYNC_TASK_ADD_ACCOUNT_WITH_VALIDATION                            = 61020,    /**< Async task for adding the account with validation */

    /* Async tasks for mailbox - from 62000 */
    EMAIL_ASYNC_TASK_ADD_MAILBOX                                            = 62010,    /**< Async task for adding the mailbox */
    EMAIL_ASYNC_TASK_DELETE_MAILBOX                                         = 62020,    /**< Async task for deleting the mailbox */
    EMAIL_ASYNC_TASK_RENAME_MAILBOX                                         = 62030,    /**< Async task for renaming the mailbox */
    EMAIL_ASYNC_TASK_DOWNLOAD_IMAP_MAILBOX_LIST                             = 62040,    /**< Async task for downloading the imap mailbox list */
    EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX                                      = 62050,    /**< Async task for deleting the mailbox : extentiong version */

    /* Async tasks for mail - from 63000 */
    EMAIL_ASYNC_TASK_ADD_MAIL                                               = 63010,    /**< Async task for adding the mail */
    EMAIL_ASYNC_TASK_ADD_READ_RECEIPT                                       = 63020,    /**< Async task for adding the read receipt */

    EMAIL_ASYNC_TASK_UPDATE_MAIL                                            = 63030,    /**< Async task for updating the mail */

    EMAIL_ASYNC_TASK_DELETE_MAIL                                            = 63040,    /**< Async task for deleting the mail */
    EMAIL_ASYNC_TASK_DELETE_ALL_MAIL                                        = 63050,    /**< Async task for deleting the all mails */
    EMAIL_ASYNC_TASK_EXPUNGE_MAILS_DELETED_FLAGGED                          = 63060,    /**< Async task for expunging the deleted flagged mails */

    EMAIL_ASYNC_TASK_MOVE_MAIL                                              = 63070,    /**< Async task for moving the mail */
    EMAIL_ASYNC_TASK_MOVE_ALL_MAIL                                          = 63080,    /**< Async task for moving the all mails */
    EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT               = 63090,    /**< Async task for moving the mails to mailbox of another account */

    EMAIL_ASYNC_TASK_SET_FLAGS_FIELD                                        = 63300,    /**< Async task for setting the flags field */

    EMAIL_ASYNC_TASK_DOWNLOAD_MAIL_LIST                                     = 63400,    /**< Async task for downloading the mail list */
    EMAIL_ASYNC_TASK_DOWNLOAD_BODY                                          = 63410,    /**< Async taks for downloading the body */
    EMAIL_ASYNC_TASK_DOWNLOAD_ATTACHMENT                                    = 63420,    /**< Async task for downloading the attachment */

    EMAIL_ASYNC_TASK_SEND_MAIL                                              = 63500,    /**< Async task for sending the mail */
    EMAIL_ASYNC_TASK_SEND_SAVED                                             = 63510,    /**< Async task for sending the saved */
    EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL = 63520,    /**< Async task for sending the mail with downloading attachment of original mail */

    EMAIL_ASYNC_TASK_SEARCH_MAIL_ON_SERVER                                  = 63600,    /**< Async task for searching the mail on server */

    /* Async tasks for mail thread - from 64000 */
    EMAIL_ASYNC_TASK_MOVE_THREAD_TO_MAILBOX                                 = 64010,    /**< Async task for moving the thread to mailbox */
    EMAIL_ASYNC_TASK_DELETE_THREAD                                          = 64020,    /**< Async task for deleting the thread */
    EMAIL_ASYNC_TASK_MODIFY_SEEN_FLAG_OF_THREAD                             = 64030,    /**< Async task for modified the seen flag of thread */

    /* Async tasks for rule - from 65000 */

    /* Async tasks for etc - from 66000 */
    EMAIL_ASYNC_TASK_BOUNDARY_END                                           = 99999,    /**< Async tasks for boundary end*/
} email_task_type_t;

typedef enum
{
    EMAIL_TASK_STATUS_UNUSED                   = 0,    /**< The status of task is unused */
    EMAIL_TASK_STATUS_WAIT                     = 1,    /**< The status of task is waited */
    EMAIL_TASK_STATUS_STARTED                  = 2,    /**< The status of task is started */
    EMAIL_TASK_STATUS_IN_PROGRESS              = 3,    /**< The status of task is in progress */
    EMAIL_TASK_STATUS_FINISHED                 = 4,    /**< The status of task is finished */
    EMAIL_TASK_STATUS_FAILED                   = 5,    /**< The status of task is failed */
    EMAIL_TASK_STATUS_CANCELED                 = 6,    /**< The status of task is canceled */
    EMAIL_TASK_STATUS_SCHEDULED                = 7,    /**< The status of task is scheduled */
} email_task_status_type_t;

typedef enum
{
    EMAIL_TASK_PRIORITY_UNUSED                 = 0,    /**< Unused the priority task*/
    EMAIL_TASK_PRIORITY_SCHEDULED              = 1,    /**< Scheduled the priority task */
    EMAIL_TASK_PRIORITY_LOW                    = 3,    /**< The priority task is low */
    EMAIL_TASK_PRIORITY_MID                    = 5,    /**< The priority task is MID */
    EMAIL_TASK_PRIORITY_HIGH                   = 7,    /**< The priority task is HIGH */
    EMAIL_TASK_PRIORITY_BACK_GROUND_TASK       = 9,    /**< The priority task is back ground */
} email_task_priority_t;
/* Tasks */

#ifdef __cplusplus
}
#endif

/**
* @}
*/

#endif /* __EMAIL_TYPES_H__ */
