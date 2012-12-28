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
 * File :  email-internal-types.h
 * Desc :  defines data structures and macros
 *
 * Auth : 
 *
 * History : 
 *    2011.04.05  :  created
 *****************************************************************************/
#ifndef __EMAIL_INTERNAL_TYPES_H__
#define __EMAIL_INTERNAL_TYPES_H__

#include "email-types.h"

#ifdef __cplusplus
extern "C"
{
#endif /*  __cplusplus */

#ifndef INTERNAL_FUNC
#define INTERNAL_FUNC __attribute__((visibility("default")))
#endif

/* ----------------------------------------------------------------------------- */
/*  Feature definitions */
#define __FEATURE_USING_ACCOUNT_SVC__
#define __FEATURE_BACKUP_ACCOUNT__
#define __FEATURE_MOVE_TO_OUTBOX_FIRST__
/*  #define __FEATURE_PARTIAL_BODY_FOR_POP3__ */
/*  #define __FEATURE_KEEP_CONNECTION__  */
/*  #define __FEATURE_DRM__ */
#define __FEATURE_PARTIAL_BODY_DOWNLOAD__
#define __FEATURE_HEADER_OPTIMIZATION__
#define __FEATURE_SEND_OPTMIZATION__
#define __FEATURE_DOWNLOAD_BODY_ATTACHMENT_OPTIMIZATION__
#define __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__
#define __FEATURE_SYNC_CLIENT_TO_SERVER__
#define __FEATURE_AUTO_POLLING__
#define __FEATURE_DEBUG_LOG__
#define __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__
#define __FEATURE_XLIST_SUPPORT__
#define __FEATURE_SUPPORT_REPORT_MAIL__
#define __FEATURE_SUPPORT_IMAP_ID__
#define __FEATURE_SUPPORT_SYNC_STATE_ON_NOTI_BAR__
#define __FEATURE_SUPPORT_VALIDATION_SYSTEM__
/*  #define __FEATURE_USE_SHARED_MUTEX_FOR_PROTECTED_FUNC_CALL__ */
/*  #define __FEATURE_IMAP_IDLE__ */

/* ----------------------------------------------------------------------------- */
/*  Macro */
#ifndef NULL
#define NULL (char *)0
#endif

#define SESSION_MAX	                        5
#define	IMAP_2004_LOG                       1
#define TEXT_SIZE                           161 
#define MAILBOX_COUNT                       6
#define PARTIAL_DOWNLOAD_SIZE               1024
#define PARTIAL_BODY_SIZE_IN_BYTES          15360     /*  Partial Body download - 15K */
#define NO_LIMITATION                       0
#define MAX_MAILBOX_TYPE                    100
#define EMAIL_SYNC_ALL_MAILBOX              1
#define EMAIL_ATTACHMENT_MAX_COUNT          512
#define DOWNLOAD_MAX_BUFFER_SIZE            8000
#define LOCAL_MAX_BUFFER_SIZE               1000000
#define IMAP_MAX_COMMAND_LENGTH             1000
#define DOWNLOAD_NOTI_INTERVAL_PERCENT      5         /*  notify every 5% */
#define DOWNLOAD_NOTI_INTERVAL_SIZE         51200     /*  notify every 50k */
#define MAX_PATH                            4096      /* /usr/src/linux-2.4.20-8/include/linux/limits.h */
#define DATETIME_LENGTH                     16
#define MAIL_ID_STRING_LENGTH               10
#define	EMAIL_LIMITATION_FREE_SPACE         (5) /*  This value is 5MB */
#define EMAIL_MAIL_MAX_COUNT                5000
#define HTML_EXTENSION_STRING               ".htm"
#define MAX_PATH_HTML                       256
#define MAX_ACTIVE_TASK                     10

#define DIR_SEPERATOR                       "/"

#define USERDATA_PATH                       "/opt/usr"
#define DATA_PATH                           "/opt/usr/data"
#define DB_PATH                             "/opt/usr/dbspace"
#define EMAIL_SERVICE_DB_FILE_PATH          "/opt/usr/dbspace/.email-service.db"

#define EMAILPATH 					        DATA_PATH"/email"
#define MAILHOME 					        DATA_PATH"/email/.email_data"
#define MAILTEMP                            MAILHOME"/tmp"
#define DIRECTORY_PERMISSION                0775

#define MIME_SUBTYPE_DRM_OBJECT             "vnd.oma.drm.message"
#define MIME_SUBTYPE_DRM_RIGHTS             "vnd.oma.drm.rights+xml"
#define MIME_SUBTYPE_DRM_DCF                "vnd.oma.drm.dcf"

#define SHM_FILE_FOR_DB_LOCK                "/.email_shm_db_lock"

#define NATIVE_EMAIL_APPLICATION_PKG        "org.tizen.email"

#define IMAP_ID_OS                          "TIZEN"
#define IMAP_ID_OS_VERSION                  "2.0b"
#define IMAP_ID_VENDOR                      "Samsung Mobile"
#define IMAP_ID_DEVICE_NAME                 "GT-I8800_EUR_XX"
#define IMAP_ID_AGUID                       "1"
#define IMAP_ID_ACLID                       "Samsung"

#ifdef __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__
#define SHM_FILE_FOR_MAIL_ID_LOCK           "/.email_shm_mail_id_lock"
#endif /* __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__ */

#define CR                                  '\r'
#define LF                                  '\n'
#define SPACE                               ' '
#define TAB                                 '\t'
#define NULL_CHAR                           '\0'
#define TAB_STRING                          "\t"
#define CR_STRING                           "\r"
#define LF_STRING                           "\n"
#define CRLF_STRING                         "\r\n"

#define GRAB_TYPE_TEXT                      1        /*  retrieve text and attachment list */
#define GRAB_TYPE_ATTACHMENT                2        /*  retrieve attachment */

#define SAVE_TYPE_SIZE                      1        /*  only get content size */
#define SAVE_TYPE_BUFFER                    2        /*  save content to buffer */
#define SAVE_TYPE_FILE                      3        /*  save content to temporary file */

#define FINISH_OFF_IF_CANCELED              if (!emcore_check_thread_status()) { err = EMAIL_ERROR_CANCELLED; goto FINISH_OFF; }
#define CHECK_JOB_CANCELED()                {if (!emcore_check_thread_status()) goto JOB_CANCEL; }

#define SNPRINTF(buff, size, format, args...)  snprintf(buff, size, format, ##args)
#define SNPRINTF_OFFSET(base_buf, offset, base_size, format, args...) \
			({\
				int _offset = offset;\
				snprintf(base_buf + _offset, base_size - _offset - 1, format, ##args);\
			})

#define THREAD_CREATE(tv, func, param, err)       { EM_DEBUG_LOG("THREAD_CREATE "#tv); err = pthread_create(&tv, NULL, func, param); }
#define THREAD_CREATE_JOINABLE(tv, func, err)     { pthread_attr_t attr; EM_DEBUG_LOG("THREAD_CREATE_JOINABLE "#tv); \
                                                   pthread_attr_init(&attr); pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);\
                                                   err = pthread_create(&tv, &attr, func, NULL); pthread_attr_destroy(&attr); }
#define THREAD_JOIN(tv)                           {EM_DEBUG_LOG("THREAD_JOIN "#tv); pthread_join(tv, NULL); }
#define THREAD_SELF()                             pthread_self()
#define THREAD_DETACH(tv)                         pthread_detach(tv)
#define INITIALIZE_CRITICAL_SECTION(cs)           {EM_DEBUG_LOG("INITIALIZE_CRITICAL_SECTION "#cs); pthread_mutex_init(&cs, NULL); }
#define ENTER_CRITICAL_SECTION(cs)                {EM_DEBUG_LOG("ENTER_CRITICAL_SECTION "#cs); pthread_mutex_lock(&cs); }
#define TRY_ENTER_CRITICAL_SECTION(cs)            {EM_DEBUG_LOG("TRY_ENTER_CRITICAL_SECTION "#cs); pthread_mutex_trylock(&cs); }
#define LEAVE_CRITICAL_SECTION(cs)                {EM_DEBUG_LOG("LEAVE_CRITICAL_SECTION "#cs); pthread_mutex_unlock(&cs); }
#define DELETE_CRITICAL_SECTION(cs)               {EM_DEBUG_LOG("DELETE_CRITICAL_SECTION "#cs); pthread_mutex_destroy(&cs); }

#define INITIALIZE_CONDITION_VARIABLE(cv)         {EM_DEBUG_LOG("INITIALIZE_CONDITION_VARIABLE "#cv); pthread_cond_init(&cv, NULL); } 
#define SLEEP_CONDITION_VARIABLE(cv, cs)          {EM_DEBUG_LOG("SLEEP_CONDITION_VARIABLE "#cv); pthread_cond_wait(&cv, &cs); } 
#define WAKE_CONDITION_VARIABLE(cv)               {EM_DEBUG_LOG("WAKE_CONDITION_VARIABLE "#cv); pthread_cond_signal(&cv); } 
#define DELETE_CONDITION_VARIABLE(cv)             {EM_DEBUG_LOG("DELETE_CONDITION_VARIABLE "#cv); pthread_cond_destroy(&cv); } 

#define INITIALIZE_RECURSIVE_CRITICAL_SECTION(cs) { EM_DEBUG_LOG("INITIALIZE_RECURSIVE_CRITICAL_SECTION "#cs);  \
                                                   if (cs == NULL) {pthread_mutex_lock(&_send_event_available_lock); pthread_mutexattr_t attr; cs = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));\
                                                   pthread_mutexattr_init(&attr); pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);\
                                                   pthread_mutex_init(cs, &attr);pthread_mutexattr_destroy(&attr); pthread_mutex_unlock(&_send_event_available_lock);}}
#define ENTER_RECURSIVE_CRITICAL_SECTION(cs)      {EM_DEBUG_LOG("ENTER_RECURSIVE_CRITICAL_SECTION "#cs); if(cs) pthread_mutex_lock(cs);}
#define TRY_ENTER_RECURSIVE_CRITICAL_SECTION(cs)  {EM_DEBUG_LOG("TRY_ENTER_RECURSIVE_CRITICAL_SECTION "#cs); if(cs)  pthread_mutex_trylock(cs);}
#define LEAVE_RECURSIVE_CRITICAL_SECTION(cs)      {EM_DEBUG_LOG("LEAVE_RECURSIVE_CRITICAL_SECTION "#cs); if(cs) pthread_mutex_unlock(cs);}
#define DELETE_RECURSIVE_CRITICAL_SECTION(cs)     {EM_DEBUG_LOG("DELETE_RECURSIVE_CRITICAL_SECTION "#cs); if(cs) pthread_mutex_destroy(cs);}
typedef pthread_t thread_t;

#define SMTP_RESPONSE_OK		        250
#define SMTP_RESPONSE_READY		        354
#define SMTP_RESPONSE_WANT_AUTH		    505
#define SMTP_RESPONSE_WANT_AUTH2	    530
#define SMTP_RESPONSE_UNAVAIL		    550

#define VCONF_KEY_DEFAULT_SLOT_SIZE     "db/private/email-service/slot_size"
#define VCONF_KEY_LATEST_MAIL_ID        "db/private/email-service/latest_mail_id"
#define VCONF_KEY_DEFAULT_ACCOUNT_ID    "db/private/email-service/default_account_id"
#define VCONF_KEY_NOTI_PRIVATE_ID       "db/private/email-service/noti_private_id"

#define OUTMODE "wb"
#define INMODE "rb"
#define READMODE "r"

#define TYPEPKCS7_SIGN 10	
#define TYPEPKCS7_MIME 11

/* __FEATURE_LOCAL_ACTIVITY__ supported
#define BULK_OPERATION_COUNT              50
#define ALL_ACTIVITIES                    0
*/

/* ----------------------------------------------------------------------------- */
/*  Type */
typedef enum
{
	_SERVICE_THREAD_TYPE_NONE      = 0,  
	_SERVICE_THREAD_TYPE_RECEIVING = 1, 
	_SERVICE_THREAD_TYPE_SENDING   = 2, 
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__ 
	_SERVICE_THREAD_TYPE_PBD       = 3, 
#endif /*  __FEATURE_PARTIAL_BODY_DOWNLOAD__ */
} email_service_thread_type;


typedef enum
{
	EMAIL_PROTOCOL_NONE                        = 0,
	EMAIL_PROTOCOL_POP3                        = 1,
	EMAIL_PROTOCOL_IMAP                        = 2,
	EMAIL_PROTOCOL_SMTP                        = 3,
} email_protocol_type_t;

typedef enum
{
	ACCOUNT_SVC_SYNC_STATUS_RUNNING            = 0,
	ACCOUNT_SVC_SYNC_STATUS_IDLE               = 1,
	ACCOUNT_SVC_SYNC_STATUS_OFF                = 2,
} email_account_svc_sync_status;

typedef enum {
	SET_TYPE_SET        = 1,
	SET_TYPE_UNION      = 2,
	SET_TYPE_MINUS      = 3,
	SET_TYPE_INTERSECT  = 4 /* Not supported */
} email_set_type_t;

#ifdef __FEATURE_KEEP_CONNECTION__
enum
{
	EMAIL_STREAM_STATUS_DISCONNECTED = 0,
	EMAIL_STREAM_STATUS_CONNECTED = 1
} ;
#endif /* __FEATURE_KEEP_CONNECTION__ */

enum 
{
	EXTENSION_JPEG   = 0,
	EXTENSION_JPG    = 1,
	EXTENSION_PNG    = 2,
	EXTENSION_GIF    = 3,
	EXTENSION_BMP    = 4,
	EXTENSION_PIC    = 5,
	EXTENSION_AGIF   = 6,
	EXTENSION_TIF    = 7,
	EXTENSION_WBMP   = 8,
	EXTENSION_P7S    = 9,
	EXTENSION_P7M    = 10
};

/*  event information */
typedef struct
{
	int                      account_id;         /*  in general, account id */
	email_event_type_t         type;
	email_event_status_type_t  status;
	char                    *event_param_data_1; /*  in general, mailbox name (exception in emcore_send_mail, emcore_send_saved_mail it is email_option_t **/
	char                    *event_param_data_2;
	char                    *event_param_data_3;
	int                      event_param_data_4;
	int                      event_param_data_5;
	int                      event_param_data_6; /* in general, notification parameter #1 */
	int                      event_param_data_7; /* in general, notification parameter #2 */
	int                      event_param_data_8;
} email_event_t;


typedef struct
{
	int   num;
	void *data;
} email_callback_holder_t;


typedef struct email_search_key_t email_search_key_t;
struct email_search_key_t
{
	int               type;
	char             *value;
	email_search_key_t *next;
};

typedef struct
{
	int                  tid;
	email_protocol_type_t  protocol;
	void                *stream;
	int                  auth;
	int                  network;
	int                  error;
	int                  status;
} email_session_t;

typedef struct
{
	int                    mailbox_id;                 /**< Unique id on mailbox table.*/
	char                  *mailbox_name;               /**< Specifies the path of mailbox.*/
	email_mailbox_type_e   mailbox_type;               /**< Specifies the type of mailbox */
	char                  *alias;                      /**< Specifies the display name of mailbox.*/
	int                    unread_count;               /**< Specifies the Unread Mail count in the mailbox.*/
	int                    total_mail_count_on_local;  /**< Specifies the total number of mails in the mailbox in the local DB.*/
	int                    total_mail_count_on_server; /**< Specifies the total number of mails in the mailbox in the mail server.*/
	int                    local;                      /**< Specifies the local mailbox.*/
	int                    synchronous;                /**< Specifies the mailbox with synchronized the server.*/
	int                    account_id;                 /**< Specifies the account ID for mailbox.*/
	int                    has_archived_mails;         /**< Specifies the archived mails.*/
	int                    mail_slot_size;             /**< Specifies how many mails can be stored in local mailbox.*/
	int                    no_select;                  /**< Specifies the 'no_select' attribute from xlist.*/
	void                  *user_data;                  /**< Specifies the internal data.*/
	void                  *mail_stream;                /**< Specifies the internal data.*/
} email_internal_mailbox_t;

#ifdef __FEATURE_KEEP_CONNECTION__
typedef struct email_connection_info
{
	int                    account_id;
	int                    sending_server_stream_status;
	void                  *sending_server_stream;
	int                    receiving_server_stream_status;
	void                  *receiving_server_stream;
	struct email_connection_info *next;
} email_connection_info_t;
#endif /* __FEATURE_KEEP_CONNECTION__ */

typedef struct
{
	char *contact_name;
	char *email_address;
	char *alias;
	int   storage_type;
	int   contact_id;
} email_mail_contact_info_t;

/*  global account list */
typedef struct email_account_list {
    email_account_t *account;
    struct email_account_list *next;
} email_account_list_t;

typedef struct {
	int                task_id;
	email_task_type_t  task_type;
	thread_t           thread_id;
} email_active_task_t;

typedef void (*email_event_callback)(int total, int done, int status, int account_id, int mail_id, int handle, void *user_data, int error);

/* ----------------------------------------------------------------------------- */
/*  Please contact Himanshu [h.gahlaut@samsung.com] for any explanation in code here under __FEATURE_PARTIAL_BODY_DOWNLOAD__ MACRO */
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
typedef enum
{
	ACTIVITY_PARTIAL_BODY_DOWNLOAD_IMAP4 = 1,
	ACTIVITY_PARTIAL_BODY_DOWNLOAD_POP3_WAIT,
	ACTIVITY_PARTIAL_BODY_DOWNLOAD_POP3_ACTIVE
}	email_pdb_activity_type_e;

typedef struct 
{
    int account_id;
    int mail_id;
    unsigned long server_mail_id;
    int activity_id;
    int mailbox_id;
    char *mailbox_name;
    email_event_type_t event_type;   /*  Event Type Null means event is created from local activitys    */
    int activity_type;             /*  Activity Type Null means event is created from event queue */

} email_event_partial_body_thd;
#endif /*  __FEATURE_PARTIAL_BODY_DOWNLOAD__ */

#ifdef __cplusplus
}
#endif /*  __cplusplus */

#endif /*  __EMAIL_INTERNAL_TYPES_H__ */
