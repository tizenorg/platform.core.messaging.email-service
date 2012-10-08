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
 * File: email-storage.c
 * Desc: email-service Storage Library on Sqlite3
 *
 * Auth:
 *
 * History:
 *	2007.03.02 : created
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h> 
#include <sys/wait.h>
#include <glib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <vconf.h>
#include <sys/mman.h>
#include <ss_manager.h>
#include <fcntl.h>
#include <dbus/dbus.h> 
#include <db-util.h>

#define __USE_UNIX98
#define __USE_GNU
#include <pthread.h>

#include "email-internal-types.h"
#include "email-convert.h"
#include "email-core-utils.h"
#include "email-utilities.h"
#include "email-storage.h"
#include "email-debug-log.h"
#include "email-types.h"
#include "email-convert.h"

#define DB_STMT sqlite3_stmt *

#define SETTING_MEMORY_TEMP_FILE_PATH "/tmp/email_tmp_file"

#define EMAIL_STORAGE_CHANGE_NOTI       "User.Email.StorageChange"
#define EMAIL_NETOWRK_CHANGE_NOTI       "User.Email.NetworkStatus"
#define EMAIL_RESPONSE_TO_API_NOTI      "User.Email.ResponseToAPI"

#define CONTENT_DATA                  "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset="
#define CONTENT_TYPE_DATA             "<meta http-equiv=\"Content-Type\" content=\"text/html; charset="

#define FLAG_SEEN       0x01
#define FLAG_DELETED    0x02
#define FLAG_FLAGGED    0x04
#define FLAG_ANSWERED   0x08
#define FLAG_OLD        0x10
#define FLAG_DRAFT      0x20

#undef close

#define ISSUE_ORGANIZATION_LEN_IN_MAIL_CERTIFICATE_TBL	256
#define EMAIL_ADDRESS_LEN_IN_MAIL_CERTIFICATE_TBL	128
#define SUBJECT_STRING_LEN_IN_MAIL_CERTIFICATE_TBL	1027
#define FILE_NAME_LEN_IN_MAIL_CERTIFICATE_TBL		256

#define ACCOUNT_NAME_LEN_IN_MAIL_ACCOUNT_TBL            50
#define RECEIVING_SERVER_ADDR_LEN_IN_MAIL_ACCOUNT_TBL   50
#define EMAIL_ADDR_LEN_IN_MAIL_ACCOUNT_TBL              128
#define USER_NAME_LEN_IN_MAIL_ACCOUNT_TBL               50
#define PASSWORD_LEN_IN_MAIL_ACCOUNT_TBL                50
#define SENDING_SERVER_ADDR_LEN_IN_MAIL_ACCOUNT_TBL     50
#define SENDING_USER_LEN_IN_MAIL_ACCOUNT_TBL            50
#define SENDING_PASSWORD_LEN_IN_MAIL_ACCOUNT_TBL        50
#define DISPLAY_NAME_LEN_IN_MAIL_ACCOUNT_TBL            30
#define REPLY_TO_ADDR_LEN_IN_MAIL_ACCOUNT_TBL           128
#define RETURN_ADDR_LEN_IN_MAIL_ACCOUNT_TBL             128
#define LOGO_ICON_PATH_LEN_IN_MAIL_ACCOUNT_TBL          256
#define DISPLAY_NAME_FROM_LEN_IN_MAIL_ACCOUNT_TBL       256
#define SIGNATURE_LEN_IN_MAIL_ACCOUNT_TBL               256
#define MAILBOX_NAME_LEN_IN_MAIL_BOX_TBL                128
#define ALIAS_LEN_IN_MAIL_BOX_TBL                       128
#define LOCAL_MBOX_LEN_IN_MAIL_READ_MAIL_UID_TBL        128
#define MAILBOX_NAME_LEN_IN_MAIL_READ_MAIL_UID_TBL      128
#define S_UID_LEN_IN_MAIL_READ_MAIL_UID_TBL             128
#define DATA2_LEN_IN_MAIL_READ_MAIL_UID_TBL             256
#define VALUE_LEN_IN_MAIL_RULE_TBL                      256
#define DEST_MAILBOX_LEN_IN_MAIL_RULE_TBL               128
#define MAILBOX_NAME_LEN_IN_MAIL_ATTACHMENT_TBL         128
#define ATTACHMENT_PATH_LEN_IN_MAIL_ATTACHMENT_TBL      256
#define ATTACHMENT_NAME_LEN_IN_MAIL_ATTACHMENT_TBL      256
#define CONTENT_ID_LEN_IN_MAIL_ATTACHMENT_TBL           256
#define ATTACHMENT_MIME_TYPE_LEN_IN_MAIL_ATTACHMENT_TBL 128
#define MAILBOX_LEN_IN_MAIL_TBL                         128
#define SERVER_MAILBOX_LEN_IN_MAIL_TBL                  128
#define SERVER_MAIL_ID_LEN_IN_MAIL_TBL                  128
#define FROM_LEN_IN_MAIL_TBL                            256
#define SENDER_LEN_IN_MAIL_TBL                          256
#define REPLY_TO_LEN_IN_MAIL_TBL                        256
#define TO_LEN_IN_MAIL_TBL                              3999
#define CC_LEN_IN_MAIL_TBL                              3999
#define BCC_LEN_IN_MAIL_TBL                             3999
#define RETURN_PATH_LEN_IN_MAIL_TBL                     3999
#define SUBJECT_LEN_IN_MAIL_TBL                         1027
#define THREAD_TOPIC_LEN_IN_MAIL_TBL                    256
#define TEXT_1_LEN_IN_MAIL_TBL                          256
#define TEXT_2_LEN_IN_MAIL_TBL                          256
#define MIME_ENTITY_LEN_IN_MAIL_TBL                     256
#define DATETIME_LEN_IN_MAIL_TBL                        128
#define MESSAGE_ID_LEN_IN_MAIL_TBL                      256
#define FROM_CONTACT_NAME_LEN_IN_MAIL_TBL               3999
#define FROM_EMAIL_ADDRESS_LEN_IN_MAIL_TBL              3999
#define TO_CONTACT_NAME_LEN_IN_MAIL_TBL                 3999
#define TO_EMAIL_ADDRESS_LEN_IN_MAIL_TBL                3999
#define MAILBOX_LEN_IN_MAIL_MEETING_TBL                 128
#define LOCATION_LEN_IN_MAIL_MEETING_TBL                1024
#define GLOBAL_OBJECT_ID_LEN_IN_MAIL_MEETING_TBL        512
#define STANDARD_NAME_LEN_IN_MAIL_MEETING_TBL           32
#define DAYLIGHT_NAME_LEN_IN_MAIL_MEETING_TBL           32
#define PREVIEWBODY_LEN_IN_MAIL_TBL                     512
#define CERTIFICATE_PATH_LEN_IN_MAIL_ACCOUNT_TBL        256 

/*  this define is used for query to change data (delete, insert, update) */
#define EMSTORAGE_START_WRITE_TRANSACTION(transaction_flag, error_code) \
	if (transaction_flag)\
	{\
		_timedlock_shm_mutex(&mapped_for_db_lock, 2);\
		if (emstorage_begin_transaction(NULL, NULL, &error_code) == false) \
		{\
			EM_DEBUG_EXCEPTION("emstorage_begin_transaction() error[%d]", error_code);\
			goto FINISH_OFF;\
		}\
	}

/*  this define is used for query to change data (delete, insert, update) */
#define EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction_flag, result_code, error_code) \
	if (transaction_flag)\
	{\
		if (result_code == true)\
		{\
			if (emstorage_commit_transaction(NULL, NULL, NULL) == false)\
			{\
				error_code = EMAIL_ERROR_DB_FAILURE;\
				result_code = false;\
			}\
		}\
		else\
		{\
			if (emstorage_rollback_transaction(NULL, NULL, NULL) == false)\
				error_code = EMAIL_ERROR_DB_FAILURE;\
		}\
		_unlockshm_mutex(&mapped_for_db_lock);\
	}

/*  this define is used for query to read (select) */
#define EMSTORAGE_START_READ_TRANSACTION(transaction_flag) \
	if (transaction_flag)\
	{\
		/*_timedlock_shm_mutex(&mapped_for_db_lock, 2);*/\
	}

/*  this define is used for query to read (select) */
#define EMSTORAGE_FINISH_READ_TRANSACTION(transaction_flag) \
	if (transaction_flag)\
	{\
		/*_unlockshm_mutex(&mapped_for_db_lock);*/\
	}

/*  for safety DB operation */
static char g_db_path[248] = {0x00, };

static pthread_mutex_t _transactionBeginLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _transactionEndLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _dbus_noti_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _db_handle_lock = PTHREAD_MUTEX_INITIALIZER;

static int _get_attribute_type_by_mail_field_name(char *input_mail_field_name, email_mail_attribute_type *output_mail_attribute_type);

#define	_MULTIPLE_DB_HANDLE

#ifdef _MULTIPLE_DB_HANDLE

#define _DISCONNECT_DB			/* db_util_close(_db_handle); */

typedef struct
{
	pthread_t 	thread_id;
	sqlite3 *db_handle;
} db_handle_t;

#define MAX_DB_CLIENT 100

/* static int _db_handle_count = 0; */
db_handle_t _db_handle_list[MAX_DB_CLIENT] = {{0, 0}, };

sqlite3 *emstorage_get_db_handle()
{	
	EM_DEBUG_FUNC_BEGIN();
	int i;
	pthread_t current_thread_id = THREAD_SELF();
	sqlite3 *result_db_handle = NULL;

	ENTER_CRITICAL_SECTION(_db_handle_lock);
	for (i = 0; i < MAX_DB_CLIENT; i++) {
		if (pthread_equal(current_thread_id, _db_handle_list[i].thread_id)) {
			EM_DEBUG_LOG("found db handle at [%d]", i);
			result_db_handle = _db_handle_list[i].db_handle;
			break;
		}
	}
	LEAVE_CRITICAL_SECTION(_db_handle_lock);
	
	if (!result_db_handle)
		EM_DEBUG_EXCEPTION("Can't find proper handle for [%d]", current_thread_id);

	EM_DEBUG_FUNC_END();
	return result_db_handle;
}

int emstorage_set_db_handle(sqlite3 *db_handle)
{	
	EM_DEBUG_FUNC_BEGIN();
	int i, error_code = EMAIL_ERROR_MAX_EXCEEDED;
	pthread_t current_thread_id = THREAD_SELF();

	ENTER_CRITICAL_SECTION(_db_handle_lock);
	for (i = 0; i < MAX_DB_CLIENT; i++)	{
		if (_db_handle_list[i].thread_id == 0) {
			_db_handle_list[i].thread_id = current_thread_id;
			_db_handle_list[i].db_handle = db_handle;
			EM_DEBUG_LOG("current_thread_id [%d], index [%d]", current_thread_id, i);
			error_code =  EMAIL_ERROR_NONE;
			break;
		}
	}
	LEAVE_CRITICAL_SECTION(_db_handle_lock);

	if (error_code == EMAIL_ERROR_MAX_EXCEEDED)
		EM_DEBUG_EXCEPTION("Exceeded the limitation of db client. Can't find empty slot in _db_handle_list.");

	EM_DEBUG_FUNC_END("error_code [%d]", error_code);
	return error_code;
}

int emstorage_remove_db_handle()
{	
	EM_DEBUG_FUNC_BEGIN();
	int i, error_code = EMAIL_ERROR_MAX_EXCEEDED;
	ENTER_CRITICAL_SECTION(_db_handle_lock);
	for (i = 0; i < MAX_DB_CLIENT; i++)
	{
		if (_db_handle_list[i].thread_id == THREAD_SELF())
		{
			_db_handle_list[i].thread_id = 0;
			_db_handle_list[i].db_handle = NULL;
			EM_DEBUG_LOG("index [%d]", i);
			error_code = EMAIL_ERROR_NONE;
			break;
		}
	}
	LEAVE_CRITICAL_SECTION(_db_handle_lock);
	
	if (error_code == EMAIL_ERROR_MAX_EXCEEDED)
		EM_DEBUG_EXCEPTION("Can't find proper thread_id");

	EM_DEBUG_FUNC_END("error_code [%d]", error_code);
	return error_code;
}


int emstorage_reset_db_handle_list()
{	
	EM_DEBUG_FUNC_BEGIN();
	int i;

	ENTER_CRITICAL_SECTION(_db_handle_lock);
	for (i = 0; i < MAX_DB_CLIENT; i++)
	{
		_db_handle_list[i].thread_id = 0;
		_db_handle_list[i].db_handle = NULL;
	}
	LEAVE_CRITICAL_SECTION(_db_handle_lock)
	
	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

sqlite3 *emstorage_get_db_connection()
{
	sqlite3 *tmp_db_handle = emstorage_get_db_handle(); 
	if (NULL == tmp_db_handle)
		tmp_db_handle = emstorage_db_open(NULL); 
	return tmp_db_handle;
}


#else	/*  _MULTIPLE_DB_HANDLE */
#define _DISCONNECT_DB			/* db_util_close(_db_handle); */

sqlite3 *_db_handle = NULL;

sqlite3 *emstorage_get_db_connection()
{
	if (NULL == _db_handle)
		emstorage_db_open(NULL); 
	return _db_handle;
}
#endif	/*  _MULTIPLE_DB_HANDLE */

/* ------------------------------------------------------------------------------ */
/*  Mutex using shared memory */
typedef struct 
{
	pthread_mutex_t mutex;
	int data;
} mmapped_t;

mmapped_t       *mapped_for_db_lock = NULL;
int              shm_fd_for_db_lock = 0;

#ifdef __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__
mmapped_t       *mapped_for_generating_mail_id = NULL;
int              shm_fd_for_generating_mail_id = 0;
#endif /* __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__ */

#ifdef __FEATURE_USE_SHARED_MUTEX_FOR_PROTECTED_FUNC_CALL__
#define EMSTORAGE_PROTECTED_FUNC_CALL(function_call, return_value) \
	{  _timedlock_shm_mutex(&mapped_for_db_lock, 2); return_value = function_call; _unlockshm_mutex(&mapped_for_db_lock); }
#else /*  __FEATURE_USE_SHARED_MUTEX_FOR_PROTECTED_FUNC_CALL__ */
#define EMSTORAGE_PROTECTED_FUNC_CALL(function_call, return_value) \
	{  return_value = function_call; }
#endif /*  __FEATURE_USE_SHARED_MUTEX_FOR_PROTECTED_FUNC_CALL__ */

INTERNAL_FUNC int emstorage_shm_file_init(const char *shm_file_name)
{
	EM_DEBUG_FUNC_BEGIN("shm_file_name [%p]", shm_file_name);

	if(!shm_file_name) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	int fd = shm_open(shm_file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP); /*  note: permission is not working */

	if (fd > 0) {
		fchmod(fd, 0666);
		EM_DEBUG_LOG("** Create SHM FILE **"); 
		if (ftruncate(fd, sizeof(mmapped_t)) != 0) {
			EM_DEBUG_EXCEPTION("ftruncate failed: %s", strerror(errno));
			return EMAIL_ERROR_SYSTEM_FAILURE;
		}
		
		mmapped_t *m = (mmapped_t *)mmap(NULL, sizeof(mmapped_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (m == MAP_FAILED) {
			EM_DEBUG_EXCEPTION("mmap failed: %s", strerror(errno));
			return EMAIL_ERROR_SYSTEM_FAILURE;
		}

		m->data = 0;

		pthread_mutexattr_t mattr; 
		pthread_mutexattr_init(&mattr);
		pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
		pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST_NP);
		pthread_mutex_init(&(m->mutex), &mattr);
		pthread_mutexattr_destroy(&mattr);
	}
	else {
		EM_DEBUG_EXCEPTION("shm_open failed: %s", strerror(errno));
		return EMAIL_ERROR_SYSTEM_FAILURE;
	}
	close(fd);
	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

int emstorage_shm_file_destroy(const char *shm_file_name)
{
	EM_DEBUG_FUNC_BEGIN("shm_file_name [%p]", shm_file_name);
	if(!shm_file_name) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (shm_unlink(shm_file_name) != 0)
		EM_DEBUG_EXCEPTION("shm_unlink failed: %s", strerror(errno));
	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

static int _initialize_shm_mutex(const char *shm_file_name, int *param_shm_fd, mmapped_t **param_mapped)
{
	EM_DEBUG_FUNC_BEGIN("shm_file_name [%p] param_shm_fd [%p], param_mapped [%p]", shm_file_name, param_shm_fd, param_mapped);
	
	if(!shm_file_name || !param_shm_fd || !param_mapped) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (!(*param_mapped)) {
		EM_DEBUG_LOG("** mapping begin **");

		if (!(*param_shm_fd)) { /*  open shm_file_name at first. Otherwise, the num of files in /proc/pid/fd will be increasing  */
			*param_shm_fd = shm_open(shm_file_name, O_RDWR, 0);
			if ((*param_shm_fd) == -1) {
				EM_DEBUG_EXCEPTION("FAIL: shm_open(): %s", strerror(errno));
				return EMAIL_ERROR_SYSTEM_FAILURE;
			}
		}
		mmapped_t *tmp = (mmapped_t *)mmap(NULL, sizeof(mmapped_t), PROT_READ|PROT_WRITE, MAP_SHARED, (*param_shm_fd), 0);
		
		if (tmp == MAP_FAILED) {
			EM_DEBUG_EXCEPTION("mmap failed: %s", strerror(errno));
			return EMAIL_ERROR_SYSTEM_FAILURE;
		}
		*param_mapped = tmp;
	}

	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

static int _timedlock_shm_mutex(mmapped_t **param_mapped, int sec)
{
	EM_DEBUG_FUNC_BEGIN("param_mapped [%p], sec [%d]", param_mapped, sec);
	
	if(!param_mapped) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	struct timespec abs_time;	
	clock_gettime(CLOCK_REALTIME, &abs_time);
	abs_time.tv_sec += sec;

	int err = pthread_mutex_timedlock(&((*param_mapped)->mutex), &abs_time);
	
	if (err == EOWNERDEAD) {
		err = pthread_mutex_consistent(&((*param_mapped)->mutex));
		EM_DEBUG_EXCEPTION("Previous owner is dead with lock. Fix mutex : %s", EM_STRERROR(err));
	} 
	else if (err != 0) {
		EM_DEBUG_EXCEPTION("ERROR : %s", EM_STRERROR(err));
		return err;
	}
	
	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

void _unlockshm_mutex(mmapped_t **param_mapped)
{
	EM_DEBUG_FUNC_BEGIN(); 
	pthread_mutex_unlock(&((*param_mapped)->mutex));
	EM_DEBUG_FUNC_END();
}
/* ------------------------------------------------------------------------------ */


static int _open_counter = 0;
static int g_transaction = false;

static int _get_password_file_name(int account_id, char *recv_password_file_name, char *send_password_file_name);
static int _read_password_from_secure_storage(char *file_name, char **password);

#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__
static int _get_cert_password_file_name(int index, char *cert_password_file_name);
#endif

typedef struct {
	const char *object_name;
	unsigned int data_flag;
} email_db_object_t;

static const email_db_object_t _g_db_tables[] =
{
	{ "mail_read_mail_uid_tbl", 1},
	{ "mail_tbl", 1},
	{ "mail_attachment_tbl", 1},
	{ NULL,  0},
};

static const email_db_object_t _g_db_indexes[] =
{
	{ "mail_read_mail_uid_idx1", 1},
	{ "mail_idx1", 1},
	{ "mail_attachment_idx1", 1},
	{ NULL,  1},
};

enum
{
	CREATE_TABLE_MAIL_ACCOUNT_TBL,
	CREATE_TABLE_MAIL_BOX_TBL,
	CREATE_TABLE_MAIL_READ_MAIL_UID_TBL,
	CREATE_TABLE_MAIL_RULE_TBL,
	CREATE_TABLE_MAIL_TBL,
	CREATE_TABLE_MAIL_ATTACHMENT_TBL,
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
	CREATE_TABLE_MAIL_PARTIAL_BODY_ACTIVITY_TBL,
#endif
	CREATE_TABLE_MAIL_MEETING_TBL,
#ifdef __FEATURE_LOCAL_ACTIVITY__
	CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL,
#endif
	CREATE_TABLE_MAIL_CERTIFICATE_TBL,
	CREATE_TABLE_MAX,
};

enum
{
	DATA1_IDX_IN_MAIL_ACTIVITY_TBL = 0,
	TRANSTYPE_IDX_IN_MAIL_ACTIVITY_TBL,
	FLAG_IDX_IN_MAIL_ACTIVITY_TBL,
};

enum
{
	CERTFICATE_BIND_TYPE_IDX_IN_MAIL_CERTIFICATE_TBL = 0,
	ISSUE_YEAR_IDX_IN_MAIL_CERTIFICATE_TBL,
	ISSUE_MONTH_IDX_IN_MAIL_CERTIFICATE_TBL,
	ISSUE_DAY_IDX_IN_MAIL_CERTIFICATE_TBL,
	EXPIRE_YEAR_IDX_IN_MAIL_CERTIFICATE_TBL,
	EXPIRE_MONTH_IDX_IN_MAIL_CERTIFICATE_TBL,
	EXPIRE_DAY_IDX_IN_MAIL_CERTIFICATE_TBL, 
	ISSUE_ORGANIZATION_IDX_IN_MAIL_CERTIFICATE_TBL,
	EMAIL_ADDRESS_IDX_IN_MAIL_CERTIFICATE_TBL,
	SUBJECT_STRING_IDX_IN_MAIL_CERTIFICATE_TBL,     
	FILE_PATH_IDX_IN_MAIL_CERTIFICATE_TBL, 
};

enum
{
	TO_RECIPIENT = 0,
	CC_RECIPIENT,
	BCC_RECIPIENT,
};
enum 
{
	ACCOUNT_ID_IDX_IN_MAIL_BOX_TBL = 0,
	LOCAL_YN_IDX_IN_MAIL_BOX_TBL,
	MAILBOX_NAME_IDX_IN_MAIL_BOX_TBL,
	MAILBOX_TYPE_IDX_IN_MAIL_BOX_TBL,
	ALIAS_IDX_IN_MAIL_BOX_TBL,
	SYNC_WITH_SERVER_YN_IDX_IN_MAIL_BOX_TBL,
	MODIFIABLE_YN_IDX_IN_MAIL_BOX_TBL,
	TOTAL_MAIL_COUNT_ON_SERVER_IDX_IN_MAIL_BOX_TBL,
	ARCHIVE_IDX_IN_MAIL_BOX_TBL,
	MAIL_SLOT_SIZE_IDX_IN_MAIL_BOX_TBL,
};

enum 
{
	ACCOUNT_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL = 0,
	LOCAL_MAILBOX_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	LOCAL_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	MAILBOX_NAME_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	S_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	DATA1_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	DATA2_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	FLAG_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	IDX_NUM_IDX_IN_MAIL_READ_MAIL_UID_TBL, 		/* unused */
};

enum 
{
	ACCOUNT_ID_IDX_IN_MAIL_RULE_TBL = 0,
	RULE_ID_IDX_IN_MAIL_RULE_TBL,
	TYPE_IDX_IN_MAIL_RULE_TBL,
	VALUE_IDX_IN_MAIL_RULE_TBL,
	ACTION_TYPE_IDX_IN_MAIL_RULE_TBL,
	TARGET_MAILBOX_ID_IDX_IN_MAIL_RULE_TBL,
	FLAG1_IDX_IN_MAIL_RULE_TBL,
	FLAG2_IDX_IN_MAIL_RULE_TBL,
};

enum 
{
	MAIL_ID_IDX_IN_MAIL_TBL = 0,
	ACCOUNT_ID_IDX_IN_MAIL_TBL,
	MAILBOX_ID_IDX_IN_MAIL_TBL,
	MAILBOX_NAME_IDX_IN_MAIL_TBL,
	MAILBOX_TYPE_IDX_IN_MAIL_TBL,
	SUBJECT_IDX_IN_MAIL_TBL,
	DATETIME_IDX_IN_MAIL_TBL,
	SERVER_MAIL_STATUS_IDX_IN_MAIL_TBL,
	SERVER_MAILBOX_NAME_IDX_IN_MAIL_TBL,
	SERVER_MAIL_ID_IDX_IN_MAIL_TBL,
	MESSAGE_ID_IDX_IN_MAIL_TBL,
	REFERENCE_ID_IDX_IN_MAIL_TBL,
	FULL_ADDRESS_FROM_IDX_IN_MAIL_TBL,
	FULL_ADDRESS_REPLY_IDX_IN_MAIL_TBL,
	FULL_ADDRESS_TO_IDX_IN_MAIL_TBL,
	FULL_ADDRESS_CC_IDX_IN_MAIL_TBL,
	FULL_ADDRESS_BCC_IDX_IN_MAIL_TBL,
	FULL_ADDRESS_RETURN_IDX_IN_MAIL_TBL,
	EMAIL_ADDRESS_SENDER_IDX_IN_MAIL_TBL,
	EMAIL_ADDRESS_RECIPIENT_IDX_IN_MAIL_TBL,
	ALIAS_SENDER_IDX_IN_MAIL_TBL,
	ALIAS_RECIPIENT_IDX_IN_MAIL_TBL,
	BODY_DOWNLOAD_STATUS_IDX_IN_MAIL_TBL,
	FILE_PATH_PLAIN_IDX_IN_MAIL_TBL,
	FILE_PATH_HTML_IDX_IN_MAIL_TBL,
	FILE_PATH_MIME_ENTITY_IDX_IN_MAIL_TBL,
	MAIL_SIZE_IDX_IN_MAIL_TBL,
	FLAGS_SEEN_FIELD_IDX_IN_MAIL_TBL,
	FLAGS_DELETED_FIELD_IDX_IN_MAIL_TBL,
	FLAGS_FLAGGED_FIELD_IDX_IN_MAIL_TBL,
	FLAGS_ANSWERED_FIELD_IDX_IN_MAIL_TBL,
	FLAGS_RECENT_FIELD_IDX_IN_MAIL_TBL,
	FLAGS_DRAFT_FIELD_IDX_IN_MAIL_TBL,
	FLAGS_FORWARDED_FIELD_IDX_IN_MAIL_TBL,
	DRM_STATUS_IDX_IN_MAIL_TBL,
	PRIORITY_IDX_IN_MAIL_TBL,
	SAVE_STATUS_IDX_IN_MAIL_TBL,
	LOCK_STATUS_IDX_IN_MAIL_TBL,
	REPORT_STATUS_IDX_IN_MAIL_TBL,
	ATTACHMENT_COUNT_IDX_IN_MAIL_TBL,
	INLINE_CONTENT_COUNT_IDX_IN_MAIL_TBL,
	THREAD_ID_IDX_IN_MAIL_TBL,
	THREAD_ITEM_COUNT_IDX_IN_MAIL_TBL,
	PREVIEW_TEXT_IDX_IN_MAIL_TBL,
	MEETING_REQUEST_STATUS_IDX_IN_MAIL_TBL,
	MESSAGE_CLASS_IDX_IN_MAIL_TBL,
	DIGEST_TYPE_IDX_IN_MAIL_TBL,
	SMIME_TYPE_IDX_IN_MAIL_TBL,
	FIELD_COUNT_OF_MAIL_TBL,  /* End of mail_tbl */
};

enum 
{
	ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL = 0,
	ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL,
	ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL,
	ATTACHMENT_SIZE_IDX_IN_MAIL_ATTACHMENT_TBL,
	MAIL_ID_IDX_IN_MAIL_ATTACHMENT_TBL,
	ACCOUNT_ID_IDX_IN_MAIL_ATTACHMENT_TBL,
	MAILBOX_ID_IDX_IN_MAIL_ATTACHMENT_TBL,
	ATTACHMENT_SAVE_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL,
	ATTACHMENT_DRM_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL,
	ATTACHMENT_DRM_METHOD_IDX_IN_MAIL_ATTACHMENT_TBL,
	ATTACHMENT_INLINE_CONTENT_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL,
	ATTACHMENT_MIME_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL,
#ifdef __ATTACHMENT_OPTI__	
	ENCODING_IDX_IN_MAIL_ATTACHMENT_TBL,
	SECTION_IDX_IN_MAIL_ATTACHMENT_TBL,
#endif	
};

enum {
	IDX_IDX_IN_MAIL_CONTACT_SYNC_TBL = 0,
#ifndef USE_SIMPLE_CONTACT_SYNC_ATTRIBUTES
	MAIL_ID_IDX_IN_MAIL_CONTACT_SYNC_TBL,
	ACCOUNT_ID_IDX_IN_MAIL_CONTACT_SYNC_TBL,
	ADDRESS_TYPE_IDX_IN_MAIL_CONTACT_SYNC_TBL,
	ADDRESS_IDX_IDX_IN_MAIL_CONTACT_SYNC_TBL,
#endif
	ADDRESS_IDX_IN_MAIL_CONTACT_SYNC_TBL,
	CONTACT_ID_IDX_IN_MAIL_CONTACT_SYNC_TBL,
	STORAGE_TYPE_IDX_IN_MAIL_CONTACT_SYNC_TBL,
	CONTACT_NAME_IDX_IN_MAIL_CONTACT_SYNC_TBL,
#ifndef USE_SIMPLE_CONTACT_SYNC_ATTRIBUTES
	DISPLAY_NAME_IDX_IN_MAIL_CONTACT_SYNC_TBL,
	FLAG1_IDX_IN_MAIL_CONTACT_SYNC_TBL,
#endif
};

/* sowmya.kr 03032010, changes for get list of mails for given addr list */
typedef struct _em_mail_id_list {
	int mail_id;	
	struct _em_mail_id_list *next;
} em_mail_id_list;

static char *g_test_query[] = {
		/*  1. select mail_account_tbl */
		"SELECT"
		" account_name, "
		" incoming_server_type, "
		" incoming_server_address, "
		" user_email_address, "
		" incoming_server_user_name, "
		" incoming_server_password, "
		" retrieval_mode, "
		" incoming_server_port_number, "
		" incoming_server_secure_connection, "
		" outgoing_server_type, "
		" outgoing_server_address, "
		" outgoing_server_port_number, "
		" outgoing_server_need_authentication, "
		" outgoing_server_secure_connection, "
		" outgoing_server_user_name, "
		" outgoing_server_password, "
		" display_name, "
		" reply_to_addr, "
		" return_addr, "
		" account_id, "
		" keep_mails_on_pop_server_after_download, "
		" flag1, "
		" flag2, "
		" pop_before_smtp, "
		" incoming_server_requires_apop"	 		   
		", logo_icon_path, "
		" is_preset_account, "
		" check_interval, "
		" priority, "
		" keep_local_copy, "
		" req_delivery_receipt, "
		" req_read_receipt, "
		" download_limit, "
		" block_address, "
		" block_subject, "
		" display_name_from, "
		" reply_with_body, "
		" forward_with_files, "
		" add_myname_card, "
		" add_signature, "
		" signature"
		", add_my_address_to_bcc"
		", account_svc_id "
		", index_color "
		", sync_status "
		", sync_disabled "
		", smime_type"
		", certificate_path"
		", cipher_type"
		", digest_type"
		" FROM mail_account_tbl",
		/*  2. select mail_box_tbl */
		"SELECT "
		"   mailbox_id, "
		"   account_id, "
		"   local_yn,  "
		"   mailbox_name,  "
		"   mailbox_type,   "
		"   alias,  "
		"   sync_with_server_yn,  "
		"   modifiable_yn,  "
		"   total_mail_count_on_server,  "
		"   has_archived_mails, "
		"   mail_slot_size "
		"   last_sync_time "
		" FROM mail_box_tbl ",
		/*  3. select mail_read_mail_uid_tbl */
		"SELECT  "
		"   account_id,  "
		"   mailbox_id,  "
		"   local_uid,  "
		"   mailbox_name,  "
		"   s_uid,  "
		"   data1 ,  "
		"   data2,  "
		"   flag,  "
		"   idx_num "
		" FROM mail_read_mail_uid_tbl ",
		/*  4. select mail_rule_tbl */
		"SELECT "
		"   account_id, "
		"   rule_id, "
		"	type, "
		"	value, "
		"	action_type, "
		"	target_mailbox_id,	"
		"	flag1, "
		"	flag2 "
		" FROM mail_rule_tbl	",
		/*  5. select mail_tbl */
		"SELECT"
		"	mail_id, "
		"	account_id, "
		"	mailbox_id, "
		"	mailbox_name, "
		"   mailbox_type, "
		"   subject, "
		"	date_time, "
		"	server_mail_status, "
		"	server_mailbox_name, "
		"	server_mail_id, "
		"   message_id, "
		"	reference_mail_id, "
		"   full_address_from, "
		"   full_address_reply, "
		"   full_address_to, "
		"   full_address_cc, "
		"   full_address_bcc, "
		"   full_address_return, "
		"   email_address_sender, "
		"   email_address_recipient, "
		"   alias_sender, "
		"   alias_recipient, "
		"	body_download_status, "
		"	file_path_plain, "
		"	file_path_html, "
		"   file_path_mime_entity, "
		"	mail_size, "
		"   flags_seen_field     ,"
		"   flags_deleted_field  ,"
		"   flags_flagged_field  ,"
		"   flags_answered_field ,"
		"   flags_recent_field   ,"
		"   flags_draft_field    ,"
		"   flags_forwarded_field,"
		"	DRM_status, "
		"	priority, "
		"	save_status, "
		"	lock_status, "
		"	report_status, "
		"   attachment_count, "
		"	inline_content_count, "
		"	thread_id, "
		"	thread_item_count, "
		"   preview_text, "
		"	meeting_request_status, "
		"   message_class, "
		"   digest_type, "
		"   smime_type "
		" FROM mail_tbl",
		/*  6. select mail_attachment_tbl */
		"SELECT "
		"	attachment_id, "
		"	attachment_name, "
		"	attachment_path, "
		"	attachment_size, "
		"	mail_id,  "
		"	account_id, "
		"	mailbox_id, "
		"	attachment_save_status,  "
		"	attachment_drm_type,  "
		"	attachment_drm_method,  "
		"	attachment_inline_content_status,  "
		"	attachment_mime_type  "
		" FROM mail_attachment_tbl ",

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
		"SELECT  "
		"   account_id, "
		"   mail_id, "
		"   server_mail_id, "
		"   activity_id, "
		"   activity_type, "
		"   mailbox_id, "
		"   mailbox_name "
		" FROM mail_partial_body_activity_tbl ",
#endif

		"SELECT  "
		"   mail_id, "
		"   account_id, "
		"   mailbox_id, "
		"   meeting_response, "
		"   start_time, "
		"   end_time, "
		"   location, "
		"   global_object_id, "
		"   offset, "
		"   standard_name, "
		"   standard_time_start_date, "
		"   standard_bias, "
		"   daylight_name, "
		"   daylight_time_start_date, "
		"   daylight_bias "
		" FROM mail_meeting_tbl ",

#ifdef __FEATURE_LOCAL_ACTIVITY__
		"SELECT	"
		"	activity_id, "
		"   account_id, "
		"   mail_id, "
		"   activity_type, "
		"   server_mailid, "
		"   src_mbox , "
		"	dest_mbox "
		" FROM mail_local_activity_tbl	",
#endif
		NULL,
};


/* ----------- Notification Changes ----------- */
typedef enum
{
	_NOTI_TYPE_STORAGE         = 0,
	_NOTI_TYPE_NETWORK         = 1,
	_NOTI_TYPE_RESPONSE_TO_API = 2,
} enotitype_t;


INTERNAL_FUNC int emstorage_send_noti(enotitype_t notiType, int subType, int data1, int data2, char *data3, int data4)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_PROFILE_BEGIN(profile_emstorage_send_noti);

	int ret = 0;
	DBusConnection *connection;
	DBusMessage	*signal = NULL;
	DBusError	   dbus_error;
	dbus_uint32_t   error; 
	const char	 *nullString = "";

	ENTER_CRITICAL_SECTION(_dbus_noti_lock);

	dbus_error_init (&dbus_error);
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_error);

	if (connection == NULL) {
		EM_DEBUG_LOG("dbus_bus_get is failed");
		goto FINISH_OFF;
	}

	if (notiType == _NOTI_TYPE_STORAGE) {
		signal = dbus_message_new_signal("/User/Email/StorageChange", EMAIL_STORAGE_CHANGE_NOTI, "email");

		if (signal == NULL) {
			EM_DEBUG_EXCEPTION("dbus_message_new_signal is failed");
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG("/User/Email/StorageChange Signal is created by dbus_message_new_signal");

		dbus_message_append_args(signal, DBUS_TYPE_INT32, &subType, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data1, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data2, DBUS_TYPE_INVALID);
		if (data3 == NULL)
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &nullString, DBUS_TYPE_INVALID);
		else
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &data3, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data4, DBUS_TYPE_INVALID);
	}
	else if (notiType == _NOTI_TYPE_NETWORK) {
		signal = dbus_message_new_signal("/User/Email/NetworkStatus", EMAIL_NETOWRK_CHANGE_NOTI, "email");

		if (signal == NULL) {
			EM_DEBUG_EXCEPTION("dbus_message_new_signal is failed");
			goto FINISH_OFF;
		}
			
		EM_DEBUG_LOG("/User/Email/NetworkStatus Signal is created by dbus_message_new_signal");

		dbus_message_append_args(signal, DBUS_TYPE_INT32, &subType, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data1, DBUS_TYPE_INVALID);
		if (data3 == NULL)
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &nullString, DBUS_TYPE_INVALID);
		else
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &data3, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data2, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data4, DBUS_TYPE_INVALID);
	}
	else if (notiType == _NOTI_TYPE_RESPONSE_TO_API) {
		signal = dbus_message_new_signal("/User/Email/ResponseToAPI", EMAIL_RESPONSE_TO_API_NOTI, "email");

		if (signal == NULL) {
			EM_DEBUG_EXCEPTION("dbus_message_new_signal is failed");
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("/User/Email/ResponseToAPI Signal is created by dbus_message_new_signal");

		dbus_message_append_args(signal, DBUS_TYPE_INT32, &subType, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data1, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data2, DBUS_TYPE_INVALID);
	}
	else {
		EM_DEBUG_EXCEPTION("Wrong notification type [%d]", notiType);
		error = EMAIL_ERROR_IPC_CRASH;
		goto FINISH_OFF;
	}

	if (!dbus_connection_send(connection, signal, &error)) {
		EM_DEBUG_LOG("dbus_connection_send is failed [%d]", error);
	}
	else {
		EM_DEBUG_LOG("dbus_connection_send is successful");
		ret = 1;
	}
		
/* 	EM_DEBUG_LOG("Before dbus_connection_flush");	 */
/* 	dbus_connection_flush(connection);               */
/* 	EM_DEBUG_LOG("After dbus_connection_flush");	 */
	
	ret = true;
FINISH_OFF:
	if (signal)
		dbus_message_unref(signal);

	LEAVE_CRITICAL_SECTION(_dbus_noti_lock);
	EM_PROFILE_END(profile_emstorage_send_noti);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_notify_storage_event(email_noti_on_storage_event transaction_type, int data1, int data2 , char *data3, int data4)
{
	EM_DEBUG_FUNC_BEGIN("transaction_type[%d], data1[%d], data2[%d], data3[%p], data4[%d]", transaction_type, data1, data2, data3, data4);
	return emstorage_send_noti(_NOTI_TYPE_STORAGE, (int)transaction_type, data1, data2, data3, data4);
}

INTERNAL_FUNC int emstorage_notify_network_event(email_noti_on_network_event status_type, int data1, char *data2, int data3, int data4)
{
	EM_DEBUG_FUNC_BEGIN("status_type[%d], data1[%d], data2[%p], data3[%d], data4[%d]", status_type, data1, data2, data3, data4);
	return emstorage_send_noti(_NOTI_TYPE_NETWORK, (int)status_type, data1, data3, data2, data4);
}

INTERNAL_FUNC int emstorage_notify_response_to_api(email_event_type_t event_type, int data1, int data2)
{
	EM_DEBUG_FUNC_BEGIN("event_type[%d], data1[%d], data2[%p], data3[%d], data4[%d]", event_type, data1, data2);
	return emstorage_send_noti(_NOTI_TYPE_RESPONSE_TO_API, (int)event_type, data1, data2, NULL, 0);
}

/* ----------- Notification Changes End----------- */
static int _get_table_field_data_char(char  **table, char *buf, int index)
{
	if ((table == NULL) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION("table[%p], buf[%p], index[%d]", table, buf, index);
		return false;
	}

	if (table[index] != NULL) {
		*buf = (char)atoi(table[index]);
		return true;
	}
	
	/*  EM_DEBUG_LOG("Empty field. Set as zero"); */
	
	*buf = 0;
	return false;
}

static int _get_table_field_data_int(char  **table, int *buf, int index)
{
	if ((table == NULL) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION("table[%p], buf[%p], index[%d]", table, buf, index);
		return false;
	}

	if (table[index] != NULL) {
		*buf = atoi(table[index]);
		return true;
	}
	
	/*  EM_DEBUG_LOG("Empty field. Set as zero"); */
	
	*buf = 0;
	return false;
}

static int _get_table_field_data_time_t(char  **table, time_t *buf, int index)
{
	if ((table == NULL) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION("table[%p], buf[%p], index[%d]", table, buf, index);
		return false;
	}

	if (table[index] != NULL) {
		*buf = (time_t)atol(table[index]);
		return true;
	}

	/*  EM_DEBUG_LOG("Empty field. Set as zero"); */

	*buf = 0;
	return false;
}

static int _get_table_field_data_string(char **table, char **buf, int ucs2, int index)
{
	int ret = false;

	if ((table == NULL) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION("table[%p], buf[%p], index[%d]", table, buf, index);
		return false;
	}
 
	char *pTemp = table[index];
	int sLen = 0;	
	if (pTemp == NULL) 
		*buf = NULL;
	else {
		sLen = strlen(pTemp);
		if(sLen) {
			*buf = (char *) em_malloc(sLen + 1);
			if (*buf == NULL) {
				EM_DEBUG_EXCEPTION("malloc is failed");
				goto FINISH_OFF;
			}
			strncpy(*buf, pTemp, sLen);
		}
		else 
			*buf = NULL;
	}
#ifdef _PRINT_STORAGE_LOG_
	if (*buf)
		EM_DEBUG_LOG("_get_table_field_data_string - buf[%s], index[%d]", *buf, index);
	else
		EM_DEBUG_LOG("_get_table_field_data_string - No string got ");
#endif	
	ret = true;
FINISH_OFF:

	return ret;
}

static int _get_table_field_data_string_without_allocation(char **table, char *buf, int buffer_size, int ucs2, int index)
{
	if ((table == NULL) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION(" table[%p], buf[%p], index[%d]", table, buf, index);
			return false;
	}

	char *pTemp = table[index];
	if (pTemp == NULL)
		buf = NULL;
	else {
		memset(buf, 0, buffer_size);
		strncpy(buf, pTemp, buffer_size - 1);
	}
#ifdef _PRINT_STORAGE_LOG_
	if (buf)
		EM_DEBUG_LOG("_get_table_field_data_string - buf[%s], index[%d]", buf, index);
	else
		EM_DEBUG_LOG("_get_table_field_data_string - No string got ");
#endif	

	return true;
}

static int _get_stmt_field_data_char(DB_STMT hStmt, char *buf, int index)
{
	if ((hStmt == NULL) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION("buf[%p], index[%d]", buf, index);
		return false;
	}

	if (sqlite3_column_text(hStmt, index) != NULL) {
		*buf = (char)sqlite3_column_int(hStmt, index);
#ifdef _PRINT_STORAGE_LOG_
			EM_DEBUG_LOG("_get_stmt_field_data_int [%d]", *buf);
#endif
		return true;
	}
	
	EM_DEBUG_LOG("sqlite3_column_int fail. index [%d]", index);

	return false;
}

static int _get_stmt_field_data_int(DB_STMT hStmt, int *buf, int index)
{
	if ((hStmt == NULL) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION("buf[%p], index[%d]", buf, index);
		return false;
	}

	if (sqlite3_column_text(hStmt, index) != NULL) {
		*buf = sqlite3_column_int(hStmt, index);
#ifdef _PRINT_STORAGE_LOG_
			EM_DEBUG_LOG("_get_stmt_field_data_int [%d]", *buf);
#endif
		return true;
	}
	
	EM_DEBUG_LOG("sqlite3_column_int fail. index [%d]", index);

	return false;
}

static int _get_stmt_field_data_time_t(DB_STMT hStmt, time_t *buf, int index)
{
	if ((hStmt == NULL) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION("buf[%p], index[%d]", buf, index);
		return false;
	}

	if (sqlite3_column_text(hStmt, index) != NULL) {
		*buf = (time_t)sqlite3_column_int(hStmt, index);
#ifdef _PRINT_STORAGE_LOG_
		EM_DEBUG_LOG("_get_stmt_field_data_time_t [%d]", *buf);
#endif
		return true;
	}

	EM_DEBUG_LOG("_get_stmt_field_data_time_t fail. index [%d]", index);
	return false;
}

static int _get_stmt_field_data_string(DB_STMT hStmt, char **buf, int ucs2, int index)
{
	if ((hStmt < 0) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION("hStmt[%d], buf[%p], index[%d]", hStmt, buf, index);
		return false;
	}

	int sLen = 0;	
	sLen = sqlite3_column_bytes(hStmt, index);

#ifdef _PRINT_STORAGE_LOG_
		EM_DEBUG_LOG("_get_stmt_field_data_string sqlite3_column_bytes sLen[%d]", sLen);
#endif

	if (sLen > 0) {
		*buf = (char *) em_malloc(sLen + 1);
		strncpy(*buf, (char *)sqlite3_column_text(hStmt, index), sLen);
	}
	else
		*buf = NULL;

#ifdef _PRINT_STORAGE_LOG_
	if (*buf)
		EM_DEBUG_LOG("buf[%s], index[%d]", *buf, index);
	else
		EM_DEBUG_LOG("_get_stmt_field_data_string - No string got");
#endif	

	return false;
}

static void _get_stmt_field_data_blob(DB_STMT hStmt, void **buf, int index)
{
	if ((hStmt < 0) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION("hStmt[%d], buf[%p], index[%d]", hStmt, buf, index);
		return;
	}

	int sLen = 0;
	sLen = sqlite3_column_bytes(hStmt, index);

#ifdef _PRINT_STORAGE_LOG_
	EM_DEBUG_LOG("_get_stmt_field_data_blob sqlite3_column_bytes sLen[%d]", sLen);
#endif

	if (sLen > 0) {
		*buf = (char *) em_malloc(sLen);
		memcpy(*buf, (void *)sqlite3_column_blob(hStmt, index), sLen);
	}
	else
		*buf = NULL;

}

static int _get_stmt_field_data_string_without_allocation(DB_STMT hStmt, char *buf, int buffer_size, int ucs2, int index)
{
	if ((hStmt < 0) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION("hStmt[%d], buf[%p], buffer_size[%d], index[%d]", hStmt, buf, buffer_size, index);
		return false;
	}

	int sLen = 0;	
	sLen = sqlite3_column_bytes(hStmt, index);

#ifdef _PRINT_STORAGE_LOG_
		EM_DEBUG_LOG("_get_stmt_field_data_string_without_allocation sqlite3_column_bytes sLen[%d]", sLen);
#endif

	if (sLen > 0) {
		memset(buf, 0, buffer_size);
		strncpy(buf, (char *)sqlite3_column_text(hStmt, index), buffer_size - 1);
	}
	else
		strcpy(buf, "");

#ifdef _PRINT_STORAGE_LOG_
	EM_DEBUG_LOG("buf[%s], index[%d]", buf, index);
#endif	
	
	return false;
}

static int _bind_stmt_field_data_char(DB_STMT hStmt, int index, char value)
{
	if ((hStmt == NULL) || (index < 0)) {
		EM_DEBUG_EXCEPTION("index[%d]", index);
		return false;
	}

	int ret = sqlite3_bind_int(hStmt, index+1, (int)value);
	
	if (ret != SQLITE_OK)  {
		EM_DEBUG_EXCEPTION("sqlite3_bind_int fail - %d", ret);
		return false;
	}
	
	return true;
}

static int _bind_stmt_field_data_int(DB_STMT hStmt, int index, int value)
{
	if ((hStmt == NULL) || (index < 0)) {
		EM_DEBUG_EXCEPTION("index[%d]", index);
		return false;
	}
	
	int ret = sqlite3_bind_int(hStmt, index+1, value);
	
	if (ret != SQLITE_OK)  {
		EM_DEBUG_EXCEPTION("sqlite3_bind_int fail - %d", ret);
		return false;
	}
	
	return true;
}

static int _bind_stmt_field_data_time_t(DB_STMT hStmt, int index, time_t value)
{
	if ((hStmt == NULL) || (index < 0)) {
		EM_DEBUG_EXCEPTION("index[%d]", index);
		return false;
	}

	int ret = sqlite3_bind_int(hStmt, index+1, (int)value);

	if (ret != SQLITE_OK)  {
		EM_DEBUG_EXCEPTION("sqlite3_bind_int fail - %d", ret);
		return false;
	}

	return true;
}

static int _bind_stmt_field_data_string(DB_STMT hStmt, int index, char *value, int ucs2, int max_len)
{
	if ((hStmt == NULL) || (index < 0)) {
		EM_DEBUG_EXCEPTION("index[%d], max_len[%d]", index, max_len);
		return false;
	}

#ifdef _PRINT_STORAGE_LOG_
	EM_DEBUG_LOG("hStmt = %p, index = %d, max_len = %d, value = [%s]", hStmt, index, max_len, value);
#endif

	int ret = 0;
	if (value != NULL)
		ret = sqlite3_bind_text(hStmt, index+1, value, -1, SQLITE_STATIC);
	else 
		ret = sqlite3_bind_text(hStmt, index+1, "", -1, NULL);

	if (ret != SQLITE_OK)  {
		EM_DEBUG_EXCEPTION("sqlite3_bind_text fail [%d]", ret);
		return false;
	}
	return true;
}


static int _bind_stmt_field_data_blob(DB_STMT hStmt, int index, void *blob, int blob_size)
{
	if ((hStmt == NULL) || (index < 0)) {
		EM_DEBUG_EXCEPTION("index[%d], blob_size[%d]", index, blob_size);
		return false;
	}

#ifdef _PRINT_STORAGE_LOG_
	EM_DEBUG_LOG("hStmt = %p, index = %d, blob_size = %d, blob = [%p]", hStmt, index, blob_size, blob);
#endif

	int ret = 0;
	if (blob_size>0)
		ret = sqlite3_bind_blob(hStmt, index+1, blob, blob_size, SQLITE_STATIC);
	else
		ret = sqlite3_bind_null(hStmt, index+1);

	if (ret != SQLITE_OK)  {
		EM_DEBUG_EXCEPTION("sqlite3_bind_blob fail [%d]", ret);
		return false;
	}
	return true;
}


static int _delete_temp_file(const char *path)
{
	EM_DEBUG_FUNC_BEGIN("path[%p]", path);
	
	DIR *dp = NULL;
	struct dirent *entry = NULL;
	
	char buf[1024] = {0x00, };
	
	if ((dp = opendir(path)) == NULL)  {
		EM_DEBUG_EXCEPTION("opendir(\"%s\") failed...", path);
		return false;
	}
	
	while ((entry = readdir(dp)) != NULL)  {
		SNPRINTF(buf, sizeof(buf), "%s/%s", path, entry->d_name);
		remove(buf);
	}
	
	closedir(dp);	
	EM_DEBUG_FUNC_END();	
	return true;
}

char *cpy_str(char *src)
{
	char *p = NULL;
	
	if (src)  {
		if (!(p = em_malloc((int)strlen(src) + 1)))  {
			EM_DEBUG_EXCEPTION("mailoc failed...");
			return NULL;
		}
		strncpy(p, src, strlen(src));
	}
	
	return p;
}

static void _emstorage_close_once(void)
{
	EM_DEBUG_FUNC_BEGIN();
	
	DELETE_CRITICAL_SECTION(_transactionBeginLock);
	DELETE_CRITICAL_SECTION(_transactionEndLock);
	
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int emstorage_close(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;

	emstorage_db_close(&error);		
	
	if (--_open_counter == 0)
		_emstorage_close_once();
	
	ret = true;

	if (err_code != NULL)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

static void *_emstorage_open_once(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int error = EMAIL_ERROR_NONE;
	
	mkdir(EMAILPATH, DIRECTORY_PERMISSION);
	mkdir(DATA_PATH, DIRECTORY_PERMISSION);

	mkdir(MAILHOME, DIRECTORY_PERMISSION);
	
	SNPRINTF(g_db_path, sizeof(g_db_path), "%s/%s", MAILHOME, MAILTEMP);
	mkdir(g_db_path, DIRECTORY_PERMISSION);
	
	_delete_temp_file(g_db_path);
	
	g_transaction = false;
	
	if (!emstorage_create_table(EMAIL_CREATE_DB_NORMAL, &error)) {
		EM_DEBUG_EXCEPTION(" emstorage_create_table failed - %d", error);
		goto FINISH_OFF;
	}
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;
		
	
	return NULL;
}

 /*  pData : a parameter which is registered when busy handler is registerd */
 /*  count : retry count */
#define EMAIL_STORAGE_MAX_RETRY_COUNT	20
static int _callback_sqlite_busy_handler(void *pData, int count)
{
	EM_DEBUG_LOG("Busy Handler Called!!: [%d]", count);
	usleep(200000);   /*   sleep time when SQLITE_LOCK */

	/*  retry will be stopped if  busy handler return 0 */
	return EMAIL_STORAGE_MAX_RETRY_COUNT - count;   
}

static int _delete_all_files_and_directories(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int error = EMAIL_ERROR_NONE;
	int ret = false;

	if (!emstorage_delete_file(EMAIL_SERVICE_DB_FILE_PATH, &error)) {
		if (error != EMAIL_ERROR_FILE_NOT_FOUND) {
			EM_DEBUG_EXCEPTION("remove failed - %s", EMAIL_SERVICE_DB_FILE_PATH);
			goto FINISH_OFF;	
		}
	}

	if (!emstorage_delete_dir(MAILHOME, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_delete_dir failed");
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

static int _recovery_from_malformed_db_file(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int error = EMAIL_ERROR_NONE;
	int ret = false;
	
	/* Delete all files and directories */
	if (!_delete_all_files_and_directories(&error)) {
		EM_DEBUG_EXCEPTION("_delete_all_files_and_directories failed [%d]", error);
		goto FINISH_OFF;
	}

	/* Delete all accounts on MyAccount */

	/* Delete all managed connection to DB */
	emstorage_reset_db_handle_list();

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}


INTERNAL_FUNC int em_db_open(sqlite3 **sqlite_handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int rc = 0;
	int error = EMAIL_ERROR_NONE;
	int ret = false;

	EM_DEBUG_LOG("*sqlite_handle[%p]", *sqlite_handle);

	if (NULL == *sqlite_handle)  {
		/*  db open */
		EM_DEBUG_LOG("Open DB");
		EMSTORAGE_PROTECTED_FUNC_CALL(db_util_open(EMAIL_SERVICE_DB_FILE_PATH, sqlite_handle, DB_UTIL_REGISTER_HOOK_METHOD), rc);
		if (SQLITE_OK != rc) {
			EM_DEBUG_EXCEPTION("db_util_open fail:%d -%s", rc, sqlite3_errmsg(*sqlite_handle));
			error = EMAIL_ERROR_DB_FAILURE;
			db_util_close(*sqlite_handle); 
			*sqlite_handle = NULL; 

			if (SQLITE_CORRUPT == rc) /* SQLITE_CORRUPT : The database disk image is malformed */ {/* Recovery DB file */ 
				EM_DEBUG_LOG("The database disk image is malformed. Trying to remove and create database disk image and directories");
				if (!_recovery_from_malformed_db_file(&error)) {
					EM_DEBUG_EXCEPTION("_recovery_from_malformed_db_file failed [%d]", error);
					goto FINISH_OFF;
				}
				
				EM_DEBUG_LOG("Open DB again");
				EMSTORAGE_PROTECTED_FUNC_CALL(db_util_open(EMAIL_SERVICE_DB_FILE_PATH, sqlite_handle, DB_UTIL_REGISTER_HOOK_METHOD), rc);
				if (SQLITE_OK != rc) {
					EM_DEBUG_EXCEPTION("db_util_open fail:%d -%s", rc, sqlite3_errmsg(*sqlite_handle));
					error = EMAIL_ERROR_DB_FAILURE;
					db_util_close(*sqlite_handle); 
					*sqlite_handle = NULL; 
				}
			}
			else
				goto FINISH_OFF;	
		}
		EM_DEBUG_LOG(">>>>> DB Handle : *sqlite_handle[%p]", *sqlite_handle);

		/* register busy handler */
		EM_DEBUG_LOG(">>>>> Register busy handler.....");
		rc = sqlite3_busy_handler(*sqlite_handle, _callback_sqlite_busy_handler, NULL);  /*  Busy Handler registration, NULL is a parameter which will be passed to handler */
		if (SQLITE_OK != rc) {
			EM_DEBUG_EXCEPTION("sqlite3_busy_handler fail:%d -%s", rc, sqlite3_errmsg(*sqlite_handle));
			error = EMAIL_ERROR_DB_FAILURE;
			db_util_close(*sqlite_handle); 
			*sqlite_handle = NULL; 
			goto FINISH_OFF;		
		}
	}
	else {
		EM_DEBUG_LOG(">>>>> DB Already Opened......");
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;	

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;	
}

INTERNAL_FUNC sqlite3* emstorage_db_open(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
#ifdef _MULTIPLE_DB_HANDLE
	sqlite3 *_db_handle = NULL;
#endif
	
	int error = EMAIL_ERROR_NONE;

	_initialize_shm_mutex(SHM_FILE_FOR_DB_LOCK, &shm_fd_for_db_lock, &mapped_for_db_lock);

#ifdef __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__
	_initialize_shm_mutex(SHM_FILE_FOR_MAIL_ID_LOCK, &shm_fd_for_generating_mail_id, &mapped_for_generating_mail_id);
#endif /*__FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__ */

	if (!em_db_open(&_db_handle, &error)) {
		EM_DEBUG_EXCEPTION("em_db_open failed[%d]", error);
		goto FINISH_OFF;
	}

#ifdef _MULTIPLE_DB_HANDLE
	emstorage_set_db_handle(_db_handle);
#endif
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;	

	EM_DEBUG_FUNC_END("ret [%p]", _db_handle);
	return _db_handle;	
}

INTERNAL_FUNC int emstorage_db_close(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
#ifdef _MULTIPLE_DB_HANDLE
	sqlite3 *_db_handle = emstorage_get_db_handle();
#endif
	
	int error = EMAIL_ERROR_NONE;
	int ret = false;

	DELETE_CRITICAL_SECTION(_transactionBeginLock);
	DELETE_CRITICAL_SECTION(_transactionEndLock);
	DELETE_CRITICAL_SECTION(_dbus_noti_lock); 

	if (_db_handle) {
		ret = db_util_close(_db_handle);
		
		if (ret != SQLITE_OK) {
			EM_DEBUG_EXCEPTION(" db_util_close fail - %d", ret);
			error = EMAIL_ERROR_DB_FAILURE;
			ret = false;
			goto FINISH_OFF;
		}
#ifdef _MULTIPLE_DB_HANDLE
		emstorage_remove_db_handle();
#endif
		_db_handle = NULL;
	}

	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;	

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;	
}


INTERNAL_FUNC int emstorage_open(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;

	int retValue;

	retValue = mkdir(DB_PATH, DIRECTORY_PERMISSION);

	EM_DEBUG_LOG("mkdir return- %d", retValue);
	EM_DEBUG_LOG("emstorage_open - before db_util_open - pid = %d", getpid());

	if (emstorage_db_open(&error) == NULL) {
		EM_DEBUG_EXCEPTION("emstorage_db_open failed[%d]", error);
		goto FINISH_OFF;
	}

	
	if (_open_counter++ == 0)
		_emstorage_open_once(&error);

	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;	

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_create_table(emstorage_create_db_type_t type, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int error = EMAIL_ERROR_NONE;
	int rc = -1, ret = false;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *create_table_query[] =
{
	/*  1. create mail_account_tbl */
	"CREATE TABLE mail_account_tbl \n"
	"(\n"
	"account_id                               INTEGER PRIMARY KEY,\n"
	"account_name                             VARCHAR(51),\n"
	"logo_icon_path                           VARCHAR(256),\n"
	"user_data                                BLOB,\n"
	"user_data_length                         INTEGER,\n"
	"account_svc_id                           INTEGER,\n"
	"sync_status                              INTEGER,\n"
	"sync_disabled                            INTEGER,\n"
	"default_mail_slot_size                   INTEGER,\n"
	"user_display_name                        VARCHAR(31),\n"
	"user_email_address                       VARCHAR(129),\n"
	"reply_to_address                         VARCHAR(129),\n"
	"return_address                           VARCHAR(129),\n"
	"incoming_server_type                     INTEGER,\n"
	"incoming_server_address                  VARCHAR(51),\n"
	"incoming_server_port_number              INTEGER,\n"
	"incoming_server_user_name                VARCHAR(51),\n"
	"incoming_server_password                 VARCHAR(51),\n"
	"incoming_server_secure_connection        INTEGER,\n"
	"retrieval_mode                           INTEGER,\n"
	"keep_mails_on_pop_server_after_download  INTEGER,\n"
	"check_interval                           INTEGER,\n"
	"auto_download_size                       INTEGER,\n"
	"outgoing_server_type                     INTEGER,\n"
	"outgoing_server_address                  VARCHAR(51),\n"
	"outgoing_server_port_number              INTEGER,\n"
	"outgoing_server_user_name                VARCHAR(51),\n"
	"outgoing_server_password                 VARCHAR(51),\n"
	"outgoing_server_secure_connection        INTEGER,\n"
	"outgoing_server_need_authentication      INTEGER,\n"
	"outgoing_server_use_same_authenticator   INTEGER,\n"
	"priority                                 INTEGER,\n"
	"keep_local_copy                          INTEGER,\n"
	"req_delivery_receipt                     INTEGER,\n"
	"req_read_receipt                         INTEGER,\n"
	"download_limit                           INTEGER,\n"
	"block_address                            INTEGER,\n"
	"block_subject                            INTEGER,\n"
	"display_name_from                        VARCHAR(256),\n"
	"reply_with_body                          INTEGER,\n"
	"forward_with_files                       INTEGER,\n"
	"add_myname_card                          INTEGER,\n"
	"add_signature                            INTEGER,\n"
	"signature                                VARCHAR(256),\n"
	"add_my_address_to_bcc                    INTEGER,\n"
	"pop_before_smtp                          INTEGER,\n"
	"incoming_server_requires_apop            INTEGER,\n"
	"smime_type                               INTEGER,\n"
	"certificate_path                         VARCHAR(256),\n"
	"cipher_type                              INTEGER,\n"
	"digest_type                              INTEGER\n"
	"); \n ",
	/*  2. create mail_box_tbl */
	"CREATE TABLE mail_box_tbl \n"
	"(\n"
	"  mailbox_id                   INTEGER \n"
	", account_id                   INTEGER \n"
	", local_yn                     INTEGER \n"
	", mailbox_name                 VARCHAR(256) \n"
	", mailbox_type                 INTEGER \n"
	", alias                        VARCHAR(256) \n"
	", sync_with_server_yn          INTEGER \n"
	", modifiable_yn                INTEGER \n"
	", total_mail_count_on_server   INTEGER \n"
	", has_archived_mails           INTEGER \n"
	", mail_slot_size               INTEGER \n"
	", last_sync_time               DATETIME \n"
	"); \n ",

	/*  3. create mail_read_mail_uid_tbl */
	"CREATE TABLE mail_read_mail_uid_tbl \n"
	"(\n"
	"  account_id                   INTEGER \n"
	", mailbox_id                   VARCHAR(129) \n"
	", local_uid                    INTEGER \n"
	", mailbox_name                 VARCHAR(256) \n"
	", s_uid                        VARCHAR(129) \n"
	", data1                        INTEGER \n"
	", data2                        VARCHAR(257) \n"
	", flag                         INTEGER \n"
	", idx_num                      INTEGER PRIMARY KEY \n"
	"); \n",
	/*  4. create mail_rule_tbl */
	"CREATE TABLE mail_rule_tbl \n"
	"(\n"
	"  account_id                   INTEGER \n"
	", rule_id                      INTEGER PRIMARY KEY\n"
	", type                         INTEGER \n"
	", value                        VARCHAR(257) \n"
	", action_type                  INTEGER \n"
	", target_mailbox_id            INTEGER \n"
	", flag1                        INTEGER \n"
	", flag2                        INTEGER \n"
	"); \n ",
	/*  5. create mail_tbl */
	"CREATE TABLE mail_tbl \n"
	"(\n"
	"  mail_id                      INTEGER PRIMARY KEY \n"
	", account_id                   INTEGER \n"
	", mailbox_id                   INTEGER \n"
	", mailbox_name                 VARCHAR(129) \n"
	", mailbox_type                 INTEGER \n"
	", subject                      TEXT \n"
	", date_time                    DATETIME \n"
	", server_mail_status           INTEGER \n"
	", server_mailbox_name          VARCHAR(129) \n"
	", server_mail_id               VARCHAR(129) \n"
	", message_id                   VARCHAR(257) \n"
	", reference_mail_id            INTEGER \n"
	", full_address_from            TEXT \n"
	", full_address_reply           TEXT \n"
	", full_address_to              TEXT \n"
	", full_address_cc              TEXT \n"
	", full_address_bcc             TEXT \n"
	", full_address_return          TEXT \n"
	", email_address_sender         TEXT collation user1 \n"
	", email_address_recipient      TEXT collation user1 \n"
	", alias_sender                 TEXT \n"
	", alias_recipient              TEXT \n"
	", body_download_status         INTEGER \n"
	", file_path_plain              VARCHAR(257) \n"
	", file_path_html               VARCHAR(257) \n"
	", file_path_mime_entity        VARCHAR(257) \n"
	", mail_size                    INTEGER \n"
	", flags_seen_field             BOOLEAN \n"
	", flags_deleted_field          BOOLEAN \n"
	", flags_flagged_field          BOOLEAN \n"
	", flags_answered_field         BOOLEAN \n"
	", flags_recent_field           BOOLEAN \n"
	", flags_draft_field            BOOLEAN \n"
	", flags_forwarded_field        BOOLEAN \n"
	", DRM_status                   INTEGER \n"
	", priority                     INTEGER \n"
	", save_status                  INTEGER \n"
	", lock_status                  INTEGER \n"
	", report_status                INTEGER \n"
	", attachment_count             INTEGER \n"
	", inline_content_count         INTEGER \n"
	", thread_id                    INTEGER \n"
	", thread_item_count            INTEGER \n"
	", preview_text                 TEXT \n"
	", meeting_request_status       INTEGER \n"
	", message_class                INTEGER \n"
	", digest_type                  INTEGER \n"
	", smime_type                   INTEGER \n"
	", FOREIGN KEY(account_id)      REFERENCES mail_account_tbl(account_id) \n"
	"); \n ",
	
	/*  6. create mail_attachment_tbl */
	"CREATE TABLE mail_attachment_tbl \n"
	"(\n"
	"  attachment_id                    INTEGER PRIMARY KEY"
	", attachment_name                  VARCHAR(257) \n"
	", attachment_path                  VARCHAR(257) \n"
	", attachment_size                  INTEGER \n"
	", mail_id                          INTEGER \n"
	", account_id                       INTEGER \n"
	", mailbox_id                       INTEGER \n"
	", attachment_save_status           INTEGER \n"
	", attachment_drm_type              INTEGER \n"
	", attachment_drm_method            INTEGER \n"
	", attachment_inline_content_status INTEGER \n"
	", attachment_mime_type             VARCHAR(257) \n"
#ifdef __ATTACHMENT_OPTI__
	", encoding                     INTEGER \n"
	", section                      varchar(257) \n"
#endif
	"); \n ",

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

	"CREATE TABLE mail_partial_body_activity_tbl  \n"
	"( \n"
	"  account_id                   INTEGER \n"
	", mail_id                      INTEGER \n"
	", server_mail_id               INTEGER \n"
	", activity_id                  INTEGER PRIMARY KEY \n"
	", activity_type                INTEGER \n"
	", mailbox_id                   INTEGER \n"
	", mailbox_name                 VARCHAR(4000) \n"
	"); \n ",
#endif

	"CREATE TABLE mail_meeting_tbl \n"
	"(\n"
	"  mail_id                     INTEGER PRIMARY KEY \n"
	", account_id                  INTEGER \n"
	", mailbox_id                  INTEGER \n"
	", meeting_response            INTEGER \n"
	", start_time                  INTEGER \n"
	", end_time                    INTEGER \n"
	", location                    TEXT \n"
	", global_object_id            TEXT \n"
	", offset                      INTEGER \n"
	", standard_name               TEXT \n"
	", standard_time_start_date    INTEGER \n"
	", standard_bias               INTEGER \n"
	", daylight_name               TEXT \n"
	", daylight_time_start_date    INTEGER \n"
	", daylight_bias               INTEGER \n"
	"); \n ",

#ifdef __FEATURE_LOCAL_ACTIVITY__
	"CREATE TABLE mail_local_activity_tbl \n"
	"( \n"
	"  activity_id                 INTEGER \n"
	", account_id                  INTEGER \n"
	", mail_id                     INTEGER \n"
	", activity_type               INTEGER \n"
	", server_mailid               VARCHAR(129) \n"
	", src_mbox                    VARCHAR(129) \n"
	", dest_mbox                   VARCHAR(129) \n"
	"); \n ",
#endif
	"CREATE TABLE mail_certificate_tbl \n"
	"( \n"
	"  certificate_id              INTEGER \n"
	", issue_year                  INTEGER \n"
	", issue_month                 INTEGER \n"
	", issue_day                   INTEGER \n"
	", expiration_year             INTEGER \n"
	", expiration_month            INTEGER \n"
	", expiration_day              INTEGER \n"
	", issue_organization_name     VARCHAR(256) \n"
	", email_address               VARCHAR(129) \n"
	", subject_str                 VARCHAR(256) \n"
	", filepath                    VARCHAR(256) \n"
	", password                    VARCHAR(51) \n"
	"); \n ",
	NULL,
};
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
 
	EM_DEBUG_LOG("local_db_handle = %p.", local_db_handle);

	char *sql;
	char **result;
	
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_account_tbl';";
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG("emstorage_create_table - result[1] = %s %c", result[1], result[1]);

	if (atoi(result[1]) < 1) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
		
		EM_DEBUG_LOG("CREATE TABLE mail_account_tbl");
		
		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_ACCOUNT_TBL], sizeof(sql_query_string));

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		/*  create mail_account_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE UNIQUE INDEX mail_account_idx1 ON mail_account_tbl (account_id)");
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);

	} /*  mail_account_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK)  {		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_ACCOUNT_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_ACCOUNT_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}

	sqlite3_free_table(result); 
	
			
	/*  2. create mail_box_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_box_tbl';";

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (atoi(result[1]) < 1) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

		EM_DEBUG_LOG("CREATE TABLE mail_box_tbl");
		
		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_BOX_TBL], sizeof(sql_query_string));
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		
		/*  create mail_local_mailbox_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE UNIQUE INDEX mail_box_idx1 ON mail_box_tbl (account_id, local_yn, mailbox_name)");
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		
	} /*  mail_box_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK)  {		
		rc = sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_BOX_TBL], NULL, NULL, NULL);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_BOX_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result); 
	
	/*  3. create mail_read_mail_uid_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_read_mail_uid_tbl';";
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
	

	if (atoi(result[1]) < 1) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
		
		EM_DEBUG_LOG("CREATE TABLE mail_read_mail_uid_tbl");
		
		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_READ_MAIL_UID_TBL], sizeof(sql_query_string));
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		
		/*  create mail_read_mail_uid_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE UNIQUE INDEX mail_read_mail_uid_idx1 ON mail_read_mail_uid_tbl (account_id, mailbox_id, local_uid, mailbox_name, s_uid)");
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		
	} /*  mail_read_mail_uid_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK)  {		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_READ_MAIL_UID_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_READ_MAIL_UID_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	
	
	/*  4. create mail_rule_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_rule_tbl';";

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
	
	if (atoi(result[1]) < 1) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

		EM_DEBUG_LOG("CREATE TABLE mail_rule_tbl");
		
		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_RULE_TBL], sizeof(sql_query_string));
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		
	} /*  mail_rule_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK)  {		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_RULE_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_RULE_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	
	
	/*  5. create mail_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_tbl';";
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
	
	if (atoi(result[1]) < 1) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
		EM_DEBUG_LOG("CREATE TABLE mail_tbl");
		
		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_TBL], sizeof(sql_query_string));
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		
		/*  create mail_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE UNIQUE INDEX mail_idx1 ON mail_tbl (mail_id, account_id)");
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		/*  create mail_tbl index for date_time */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE INDEX mail_idx_date_time ON mail_tbl (date_time)");
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		/*  create mail_tbl index for thread_item_count */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE INDEX mail_idx_thread_mail_count ON mail_tbl (thread_item_count)");
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		
		/*  just one time call */
/* 		EFTSInitFTSIndex(FTS_EMAIL_IDX); */
	} /*  mail_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK)  {		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	
	
	/*  6. create mail_attachment_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_attachment_tbl';";
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (atoi(result[1]) < 1) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
		EM_DEBUG_LOG("CREATE TABLE mail_attachment_tbl");
		
		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_ATTACHMENT_TBL], sizeof(sql_query_string));
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		
		/*  create mail_attachment_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE UNIQUE INDEX mail_attachment_idx1 ON mail_attachment_tbl (mail_id, attachment_id) ");
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		
	} /*  mail_attachment_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK)  {		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_ATTACHMENT_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_ATTACHMENT_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_partial_body_activity_tbl';";
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
	
	if (atoi(result[1]) < 1) {
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

		EM_DEBUG_LOG("CREATE TABLE mail_partial_body_activity_tbl");
		
		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_PARTIAL_BODY_ACTIVITY_TBL], sizeof(sql_query_string));
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		
	} /*  mail_rule_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK)  {		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_PARTIAL_BODY_ACTIVITY_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_PARTIAL_BODY_ACTIVITY_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	
#endif /*  __FEATURE_PARTIAL_BODY_DOWNLOAD__ */

	/*  create mail_meeting_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_meeting_tbl';";
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
	
	if (atoi(result[1]) < 1) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
		
		EM_DEBUG_LOG("CREATE TABLE mail_meeting_tbl");
		
		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_MEETING_TBL], sizeof(sql_query_string));
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE UNIQUE INDEX mail_meeting_idx1 ON mail_meeting_tbl (mail_id)");
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		
	} /*  mail_contact_sync_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK)  {		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_MEETING_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_MEETING_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);

#ifdef __FEATURE_LOCAL_ACTIVITY__
	
		sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_local_activity_tbl';";
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
		
		if (atoi(result[1]) < 1) {
			
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
			EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
				("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
	
			EM_DEBUG_LOG(" CREATE TABLE mail_local_activity_tbl");
			
			SNPRINTF(sql_query_string, sizeof(sql_query_string), create_table_query[CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL]);
			
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
			EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
			
		} /*  mail_rule_tbl */
		else if (type == EMAIL_CREATE_DB_CHECK)  { 	
			rc = sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL], NULL, NULL, NULL);
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL], NULL, NULL, NULL), rc);
			EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL], rc, sqlite3_errmsg(local_db_handle)));
		}
		sqlite3_free_table(result);
		
#endif /*  __FEATURE_LOCAL_ACTIVITY__ */
	/*  create mail_certificate_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_certificate_tbl';";
	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);   */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; }, ("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (atoi(result[1]) < 1) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; }, ("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

		EM_DEBUG_LOG("CREATE TABLE mail_certificate_tbl");

		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_CERTIFICATE_TBL], sizeof(sql_query_string));

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE UNIQUE INDEX mail_certificate_idx1 ON mail_certificate_tbl (certificate_id)");

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} /*  mail_contact_sync_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK)  {                
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_CERTIFICATE_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_CERTIFICATE_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);

	ret = true;
	
FINISH_OFF:
	if (ret == true) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	}
	else {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "rollback", NULL, NULL, NULL), rc);
	}
 
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/* Query series --------------------------------------------------------------*/

INTERNAL_FUNC int emstorage_query_mail_count(const char *input_conditional_clause, int input_transaction, int *output_total_mail_count, int *output_unseen_mail_count)
{
	EM_DEBUG_FUNC_BEGIN("input_conditional_clause[%p], input_transaction[%d], output_total_mail_count[%p], output_unseen_mail_count[%p]", input_conditional_clause, input_transaction, output_total_mail_count, output_unseen_mail_count);
	int rc = -1;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char err_msg[1024];
	char **result;
	sqlite3 *local_db_handle = NULL;

	if (!input_conditional_clause || (!output_total_mail_count && !output_unseen_mail_count)) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	memset(&sql_query_string, 0x00, sizeof(sql_query_string));
	local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_READ_TRANSACTION(input_transaction);

	SNPRINTF(sql_query_string, QUERY_SIZE, "SELECT COUNT(*) FROM mail_tbl");
	EM_SAFE_STRCAT(sql_query_string, (char*)input_conditional_clause);

	if (output_total_mail_count)  {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
		EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
		_get_stmt_field_data_int(hStmt, output_total_mail_count, 0);
	}

	if (output_unseen_mail_count)  {
		EM_SAFE_STRCAT(sql_query_string, " AND flags_seen_field = 0 ");

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		*output_unseen_mail_count = atoi(result[1]);
		sqlite3_free_table(result);
	}

FINISH_OFF:

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG("sqlite3_finalize failed [%d] : %s", rc, err_msg);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(input_transaction);
	_DISCONNECT_DB;

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int emstorage_query_mail_id_list(const char *input_conditional_clause, int input_transaction, int **output_mail_id_list, int *output_mail_id_count)
{
	EM_DEBUG_FUNC_BEGIN("input_conditional_clause [%p], input_transaction [%d], output_mail_id_list [%p], output_mail_id_count [%p]", input_conditional_clause, input_transaction, output_mail_id_list, output_mail_id_count);

	int      i = 0;
	int      count = 0;
	int      rc = -1;
	int      cur_query = 0;
	int      col_index;
	int      error = EMAIL_ERROR_NONE;
	int     *result_mail_id_list = NULL;
	char   **result = NULL;
	char     sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EM_IF_NULL_RETURN_VALUE(input_conditional_clause, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(output_mail_id_list, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(output_mail_id_count, EMAIL_ERROR_INVALID_PARAM);

	EMSTORAGE_START_READ_TRANSACTION(input_transaction);

	/* Composing query */
	SNPRINTF_OFFSET(sql_query_string, cur_query, QUERY_SIZE, "SELECT mail_id FROM mail_tbl ");
	EM_SAFE_STRCAT(sql_query_string, (char*)input_conditional_clause);

	EM_DEBUG_LOG("query[%s].", sql_query_string);

	/* Performing query */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	col_index = 1;

	/*  to get mail list */
	if (count == 0) {
		EM_DEBUG_EXCEPTION("No mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("There are [%d] mails.", count);

	if (!(result_mail_id_list = (int*)em_malloc(sizeof(int) * count))) {
		EM_DEBUG_EXCEPTION("malloc for result_mail_id_list failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG(">>>> DATA ASSIGN START >> ");

	for (i = 0; i < count; i++)
		_get_table_field_data_int(result, result_mail_id_list + i, col_index++);

	EM_DEBUG_LOG(">>>> DATA ASSIGN END [count : %d] >> ", count);

	*output_mail_id_list  = result_mail_id_list;
	*output_mail_id_count = count;

FINISH_OFF:

	if(result)
		sqlite3_free_table(result);

	EMSTORAGE_FINISH_READ_TRANSACTION(input_transaction);
	_DISCONNECT_DB;

	if(error != EMAIL_ERROR_NONE)
		EM_SAFE_FREE(result_mail_id_list);

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int emstorage_query_mail_list(const char *conditional_clause, int transaction, email_mail_list_item_t** result_mail_list,  int *result_count,  int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_PROFILE_BEGIN(emstorage_query_mail_list_func);
		   
	int i = 0, count = 0, rc = -1, to_get_count = (result_mail_list)?0:1;
	int local_inline_content_count = 0, local_attachment_count = 0;
	int cur_query = 0, base_count = 0, col_index;
	int ret = false, error = EMAIL_ERROR_NONE;
	char *date_time_string = NULL;
	char **result = NULL, sql_query_string[QUERY_SIZE] = {0, };
	char *field_list = "mail_id, account_id, mailbox_id, full_address_from, email_address_sender, full_address_to, subject, body_download_status, flags_seen_field, flags_deleted_field, flags_flagged_field, flags_answered_field, flags_recent_field, flags_draft_field, flags_forwarded_field, DRM_status, priority, save_status, lock_status, attachment_count, inline_content_count, date_time, preview_text, thread_id, thread_item_count, meeting_request_status, message_class, smime_type ";
	email_mail_list_item_t *mail_list_item_from_tbl = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EM_IF_NULL_RETURN_VALUE(conditional_clause, false);
	EM_IF_NULL_RETURN_VALUE(result_count, false);

	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	/*  select clause */
	if (to_get_count) /*  count only */
		cur_query += SNPRINTF_OFFSET(sql_query_string, cur_query, QUERY_SIZE, "SELECT mail_id FROM mail_tbl");
	else /*  mail list in plain form */
		cur_query += SNPRINTF_OFFSET(sql_query_string, cur_query, QUERY_SIZE, "SELECT %s FROM mail_tbl ", field_list);

	/* cur_query += SNPRINTF_OFFSET(sql_query_string, cur_query, QUERY_SIZE, conditional_clause); This code caused some crashes.*/
	strncat(sql_query_string, conditional_clause, QUERY_SIZE - cur_query);
	
	EM_DEBUG_LOG("emstorage_query_mail_list : query[%s].", sql_query_string);

	/*  performing query		*/	
	EM_PROFILE_BEGIN(emstorage_query_mail_list_performing_query);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	EM_PROFILE_END(emstorage_query_mail_list_performing_query);
	
	if (!base_count) 
		base_count = ({ int i=0; char *tmp = field_list; for (i=0; tmp && *(tmp + 1); tmp = index(tmp + 1, ','), i++); i; });

	col_index = base_count;

	EM_DEBUG_LOG("base_count [%d]", base_count);

	if (to_get_count) {	
		/*  to get count */
		if (!count) {
			EM_DEBUG_EXCEPTION("No mail found...");
			ret = false;
			error= EMAIL_ERROR_MAIL_NOT_FOUND;
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG("There are [%d] mails.", count);
	}
	else {
		/*  to get mail list */
		if (!count) {
			EM_DEBUG_EXCEPTION("No mail found...");			
			ret = false;
			error= EMAIL_ERROR_MAIL_NOT_FOUND;
			goto FINISH_OFF;
		}
		
		EM_DEBUG_LOG("There are [%d] mails.", count);
		if (!(mail_list_item_from_tbl = (email_mail_list_item_t*)em_malloc(sizeof(email_mail_list_item_t) * count))) {
			EM_DEBUG_EXCEPTION("malloc for mail_list_item_from_tbl failed...");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		EM_PROFILE_BEGIN(emstorage_query_mail_list_loop);
		EM_DEBUG_LOG(">>>> DATA ASSIGN START >> ");	
		for (i = 0; i < count; i++) {
			_get_table_field_data_int(result, &(mail_list_item_from_tbl[i].mail_id), col_index++);
			_get_table_field_data_int(result, &(mail_list_item_from_tbl[i].account_id), col_index++);
			_get_table_field_data_int(result, &(mail_list_item_from_tbl[i].mailbox_id), col_index++);
			_get_table_field_data_string_without_allocation(result, mail_list_item_from_tbl[i].full_address_from, STRING_LENGTH_FOR_DISPLAY, 1, col_index++);
			_get_table_field_data_string_without_allocation(result, mail_list_item_from_tbl[i].email_address_sender, MAX_EMAIL_ADDRESS_LENGTH, 1, col_index++);
			_get_table_field_data_string_without_allocation(result, mail_list_item_from_tbl[i].email_address_recipient, STRING_LENGTH_FOR_DISPLAY,  1, col_index++);
			_get_table_field_data_string_without_allocation(result, mail_list_item_from_tbl[i].subject, STRING_LENGTH_FOR_DISPLAY, 1, col_index++);
			_get_table_field_data_int(result, &(mail_list_item_from_tbl[i].body_download_status), col_index++);
			_get_table_field_data_char(result, &(mail_list_item_from_tbl[i].flags_seen_field), col_index++);
			_get_table_field_data_char(result, &(mail_list_item_from_tbl[i].flags_deleted_field), col_index++);
			_get_table_field_data_char(result, &(mail_list_item_from_tbl[i].flags_flagged_field), col_index++);
			_get_table_field_data_char(result, &(mail_list_item_from_tbl[i].flags_answered_field), col_index++);
			_get_table_field_data_char(result, &(mail_list_item_from_tbl[i].flags_recent_field), col_index++);
			_get_table_field_data_char(result, &(mail_list_item_from_tbl[i].flags_draft_field), col_index++);
			_get_table_field_data_char(result, &(mail_list_item_from_tbl[i].flags_forwarded_field), col_index++);
			_get_table_field_data_int(result, &(mail_list_item_from_tbl[i].DRM_status), col_index++);
			_get_table_field_data_int(result, (int*)&(mail_list_item_from_tbl[i].priority), col_index++);
			_get_table_field_data_int(result, (int*)&(mail_list_item_from_tbl[i].save_status), col_index++);
			_get_table_field_data_int(result, &(mail_list_item_from_tbl[i].lock_status), col_index++);
			_get_table_field_data_int(result, &local_attachment_count, col_index++);
			_get_table_field_data_int(result, &local_inline_content_count, col_index++);
			_get_table_field_data_time_t(result, &(mail_list_item_from_tbl[i].date_time), col_index++);
			_get_table_field_data_string_without_allocation(result, mail_list_item_from_tbl[i].preview_text, MAX_PREVIEW_TEXT_LENGTH, 1, col_index++);
			_get_table_field_data_int(result, &(mail_list_item_from_tbl[i].thread_id), col_index++);
			_get_table_field_data_int(result, &(mail_list_item_from_tbl[i].thread_item_count), col_index++);
			_get_table_field_data_int(result, (int*)&(mail_list_item_from_tbl[i].meeting_request_status), col_index++);
			_get_table_field_data_int(result, (int*)&(mail_list_item_from_tbl[i].message_class), col_index++);
			_get_table_field_data_int(result, (int*)&(mail_list_item_from_tbl[i].smime_type), col_index++);

			mail_list_item_from_tbl[i].attachment_count = ((local_attachment_count - local_inline_content_count)>0)?1:0;
		}
		EM_DEBUG_LOG(">>>> DATA ASSIGN END [count : %d] >> ", count);
		EM_PROFILE_END(emstorage_query_mail_list_loop);
	}

	sqlite3_free_table(result);
	result = NULL;
	ret = true;
	
FINISH_OFF:
	EM_DEBUG_LOG("COUNT [%d]", count);

	if (to_get_count)
		*result_count = count;
	else {
		if (ret == true)  {
			if (result_mail_list)
				*result_mail_list = mail_list_item_from_tbl;
			*result_count = count;
		}
		else 
			EM_SAFE_FREE(mail_list_item_from_tbl);
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	sqlite3_release_memory(-1);

	_DISCONNECT_DB;
	
	EM_SAFE_FREE(date_time_string);

	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(emstorage_query_mail_list_func);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_query_mail_tbl(const char *conditional_clause, int transaction, emstorage_mail_tbl_t** result_mail_tbl, int *result_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("conditional_clause[%s], result_mail_tbl[%p], result_count [%p], transaction[%d], err_code[%p]", conditional_clause, result_mail_tbl, result_count, transaction, err_code);

	if (!conditional_clause) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int i, col_index = FIELD_COUNT_OF_MAIL_TBL, rc, ret = false, count;
	int error = EMAIL_ERROR_NONE;
	char **result = NULL, sql_query_string[QUERY_SIZE] = {0, };
	emstorage_mail_tbl_t* p_data_tbl = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_tbl %s", conditional_clause);

	EM_DEBUG_LOG("Query [%s]", sql_query_string);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (!count) {
		EM_DEBUG_EXCEPTION("No mail found...");			
		ret = false;
		error= EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	EM_DEBUG_LOG("There are [%d] mails.", count);
	if (!(p_data_tbl = (emstorage_mail_tbl_t*)em_malloc(sizeof(emstorage_mail_tbl_t) * count))) {
		EM_DEBUG_EXCEPTION("malloc for emstorage_mail_tbl_t failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG(">>>> DATA ASSIGN START >> ");	
	for (i = 0; i < count; i++) {
		_get_table_field_data_int   (result, &(p_data_tbl[i].mail_id), col_index++);
		_get_table_field_data_int   (result, &(p_data_tbl[i].account_id), col_index++);
		_get_table_field_data_int   (result, &(p_data_tbl[i].mailbox_id), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].mailbox_name), 0, col_index++);
		_get_table_field_data_int   (result, &(p_data_tbl[i].mailbox_type), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].subject), 1, col_index++);
		_get_table_field_data_time_t (result, &(p_data_tbl[i].date_time), col_index++);
		_get_table_field_data_int   (result, &(p_data_tbl[i].server_mail_status), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].server_mailbox_name), 0, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].server_mail_id), 0, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].message_id), 0, col_index++);
		_get_table_field_data_int   (result, &(p_data_tbl[i].reference_mail_id), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].full_address_from), 1, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].full_address_reply), 1, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].full_address_to), 1, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].full_address_cc), 1, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].full_address_bcc), 1, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].full_address_return), 1, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].email_address_sender), 1, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].email_address_recipient), 1, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].alias_sender), 1, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].alias_recipient), 1, col_index++);
		_get_table_field_data_int   (result, &(p_data_tbl[i].body_download_status), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].file_path_plain), 0, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].file_path_html), 0, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].file_path_mime_entity), 0, col_index++);
		_get_table_field_data_int   (result, &(p_data_tbl[i].mail_size), col_index++);
		_get_table_field_data_char  (result, &(p_data_tbl[i].flags_seen_field), col_index++);
		_get_table_field_data_char  (result, &(p_data_tbl[i].flags_deleted_field), col_index++);
		_get_table_field_data_char  (result, &(p_data_tbl[i].flags_flagged_field), col_index++);
		_get_table_field_data_char  (result, &(p_data_tbl[i].flags_answered_field), col_index++);
		_get_table_field_data_char  (result, &(p_data_tbl[i].flags_recent_field), col_index++);
		_get_table_field_data_char  (result, &(p_data_tbl[i].flags_draft_field), col_index++);
		_get_table_field_data_char  (result, &(p_data_tbl[i].flags_forwarded_field), col_index++);
		_get_table_field_data_int   (result, &(p_data_tbl[i].DRM_status), col_index++);
		_get_table_field_data_int   (result, (int*)&(p_data_tbl[i].priority), col_index++);
		_get_table_field_data_int   (result, (int*)&(p_data_tbl[i].save_status), col_index++);
		_get_table_field_data_int   (result, &(p_data_tbl[i].lock_status), col_index++);
		_get_table_field_data_int   (result, (int*)&(p_data_tbl[i].report_status), col_index++);
		_get_table_field_data_int   (result, &(p_data_tbl[i].attachment_count), col_index++);
		_get_table_field_data_int   (result, &(p_data_tbl[i].inline_content_count), col_index++);
		_get_table_field_data_int   (result, &(p_data_tbl[i].thread_id), col_index++);
		_get_table_field_data_int   (result, &(p_data_tbl[i].thread_item_count), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].preview_text), 1, col_index++);
		_get_table_field_data_int   (result, (int*)&(p_data_tbl[i].meeting_request_status), col_index++);
		_get_table_field_data_int   (result, (int*)&(p_data_tbl[i].message_class), col_index++);
		_get_table_field_data_int   (result, (int*)&(p_data_tbl[i].digest_type), col_index++);
		_get_table_field_data_int   (result, (int*)&(p_data_tbl[i].smime_type), col_index++);
		/*  check real body file... */
		if (p_data_tbl[i].body_download_status) {
			struct stat buf;
			
			if (p_data_tbl[i].file_path_html && strlen(p_data_tbl[i].file_path_html) > 0) {
				if (stat(p_data_tbl[i].file_path_html, &buf) == -1) {
					EM_DEBUG_LINE;
					p_data_tbl[i].body_download_status = 0;
				}
			}
			else if (p_data_tbl[i].file_path_plain && strlen(p_data_tbl[i].file_path_plain) > 0) {
				if (stat(p_data_tbl[i].file_path_plain, &buf) == -1){
					EM_DEBUG_LINE;
					p_data_tbl[i].body_download_status = 0;
				}
			}
			else 
				p_data_tbl[i].body_download_status = 0;
		}
	}
	
	ret = true;
	
FINISH_OFF:
	if(result)
		sqlite3_free_table(result);

	if (ret == true)  {
		if (result_mail_tbl)
			*result_mail_tbl = p_data_tbl;
		*result_count = count;
	}
	else 
		EM_SAFE_FREE(p_data_tbl);
	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	sqlite3_release_memory(-1);

	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_query_mailbox_tbl(const char *input_conditional_clause, const char *input_ordering_clause, int input_get_mail_count,  int input_transaction, emstorage_mailbox_tbl_t **output_mailbox_list, int *output_mailbox_count)
{
	EM_DEBUG_FUNC_BEGIN("input_conditional_clause[%p], input_ordering_clause [%p], input_get_mail_count[%d], input_transaction[%d], output_mailbox_list[%p], output_mailbox_count[%d]", input_conditional_clause, input_ordering_clause, input_get_mail_count, input_transaction, output_mailbox_list, output_mailbox_count);

	int i = 0;
	int rc;
	int count = 0;
	int col_index = 0;
	int error = EMAIL_ERROR_NONE;
	int read_count = 0;
	int total_count = 0;
	char **result;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *fields = "MBT.mailbox_id, MBT.account_id, local_yn, MBT.mailbox_name, MBT.mailbox_type, alias, sync_with_server_yn, modifiable_yn, total_mail_count_on_server, has_archived_mails, mail_slot_size, last_sync_time ";
	emstorage_mailbox_tbl_t* p_data_tbl = NULL;
	DB_STMT hStmt = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_READ_TRANSACTION(input_transaction);

	if (input_get_mail_count == 0) {	/*  without mail count  */
		col_index = 12;
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s FROM mail_box_tbl AS MBT %s", fields, input_conditional_clause);
	}
	else {	/*  with read count and total count */
		col_index = 14;
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s, total, read  FROM mail_box_tbl AS MBT LEFT OUTER JOIN (SELECT mailbox_name, count(mail_id) AS total, SUM(flags_seen_field) AS read FROM mail_tbl %s GROUP BY mailbox_name) AS MT ON MBT.mailbox_name = MT.mailbox_name %s %s", fields, input_conditional_clause, input_conditional_clause, input_ordering_clause);
	}

	EM_DEBUG_LOG("query[%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)))

	EM_DEBUG_LOG("result count [%d]", count);

	if(count == 0) {
		EM_DEBUG_EXCEPTION("Can't find mailbox");
		error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
		goto FINISH_OFF;
	}

	if ((p_data_tbl = (emstorage_mailbox_tbl_t*)em_malloc(sizeof(emstorage_mailbox_tbl_t) * count)) == NULL) {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < count; i++)  {
		_get_table_field_data_int(result, &(p_data_tbl[i].mailbox_id), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].account_id), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].local_yn), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].mailbox_name), 0, col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].mailbox_type), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].alias), 0, col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].sync_with_server_yn), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].modifiable_yn), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].total_mail_count_on_server), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].has_archived_mails), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].mail_slot_size), col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].last_sync_time), col_index++);

		if (input_get_mail_count == 1) {
			_get_table_field_data_int(result, &(total_count), col_index++);
			p_data_tbl[i].total_mail_count_on_local = total_count;
			_get_table_field_data_int(result, &(read_count), col_index++);
			p_data_tbl[i].unread_count = total_count - read_count;			/*  return unread count, NOT  */
		}
	}


FINISH_OFF:
	if (result)
		sqlite3_free_table(result);

	if (error == EMAIL_ERROR_NONE)  {
		*output_mailbox_list = p_data_tbl;
		*output_mailbox_count = count;
		EM_DEBUG_LOG("Mailbox Count [ %d]", count);
	}
	else if (p_data_tbl != NULL)
		emstorage_free_mailbox(&p_data_tbl, count, NULL);

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);

			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(input_transaction);
	_DISCONNECT_DB;

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

/* Query series --------------------------------------------------------------*/

INTERNAL_FUNC int emstorage_check_duplicated_account(email_account_t* account, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	if (!account)  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	char **result;
	int count;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"SELECT COUNT(*) FROM mail_account_tbl "
		" WHERE "
		" user_email_address = '%s' AND "
		" incoming_server_user_name = '%s' AND "
		" incoming_server_type = %d AND "
		" incoming_server_address = '%s' AND "
		" outgoing_server_user_name = '%s' AND "
		" outgoing_server_type = %d AND "
		" outgoing_server_address = '%s'; ",
		account->user_email_address,
		account->incoming_server_user_name, account->incoming_server_type, account->incoming_server_address,
		account->outgoing_server_user_name, account->outgoing_server_type, account->outgoing_server_address
	);
	EM_DEBUG_LOG("Query[%s]", sql_query_string);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	count = atoi(result[1]);
	sqlite3_free_table(result);

	EM_DEBUG_LOG("Count of Duplicated Account Information: [%d]", count);

	if (count == 0) {	/*  not duplicated account */
		ret = true;
		EM_DEBUG_LOG("NOT duplicated account: user_email_address[%s]", account->user_email_address);
	}
	else {	/*  duplicated account */
		ret = false;
		EM_DEBUG_LOG("The same account already exists. Duplicated account: user_email_address[%s]", account->user_email_address);
		error = EMAIL_ERROR_ALREADY_EXISTS;
	}
	
FINISH_OFF:

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}

INTERNAL_FUNC int emstorage_get_account_count(int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	if (!count)  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
 
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char err_msg[1024];
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_account_tbl");
	EM_DEBUG_LOG("SQL STMT [ %s ]", sql_query_string);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("Before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	*count = sqlite3_column_int(hStmt, 0);
	
	ret = true;
	
FINISH_OFF:

	if (hStmt != NULL)  {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		hStmt=NULL;
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d: %s", rc, err_msg);
			error = EMAIL_ERROR_DB_FAILURE;
		}
		EM_DEBUG_LOG("sqlite3_finalize- %d", rc);
	}
 
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_account_list(int *select_num, emstorage_account_tbl_t** account_list, int transaction, int with_password, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int i = 0, count = 0, rc = -1, ret = false;
	int field_index = 0;
	int error = EMAIL_ERROR_NONE;
	emstorage_account_tbl_t *p_data_tbl = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *sql = "SELECT count(*) FROM mail_account_tbl;";
	char **result;
	sqlite3 *local_db_handle = NULL;
	DB_STMT hStmt = NULL;

	if (!select_num || !account_list)  {
		EM_DEBUG_EXCEPTION("select_num[%p], account_list[%p]", select_num, account_list);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	count = atoi(result[1]);
	sqlite3_free_table(result);

	EM_DEBUG_LOG("count = %d", rc);

	if (count == 0) {
		EM_DEBUG_EXCEPTION("no account found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_account_tbl ORDER BY account_id");

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG("After sqlite3_prepare_v2 hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no account found...");
		
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		count = 0;
		ret = true;
		goto FINISH_OFF;
	}

	if (!(p_data_tbl = (emstorage_account_tbl_t*)malloc(sizeof(emstorage_account_tbl_t) * count)))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memset(p_data_tbl, 0x00, sizeof(emstorage_account_tbl_t) * count);
	for (i = 0; i < count; i++)  {
		/*  get recordset */
		field_index = 0;
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl[i].account_id), field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].account_name), 0, field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].logo_icon_path), 0, field_index++);
		_get_stmt_field_data_blob(hStmt, &(p_data_tbl[i].user_data), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].user_data_length), field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl[i].account_svc_id), field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl[i].sync_status), field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl[i].sync_disabled), field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl[i].default_mail_slot_size), field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].user_display_name), 0, field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].user_email_address), 0, field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].reply_to_address), 0, field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].return_address), 0, field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl[i].incoming_server_type), field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].incoming_server_address), 0, field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].incoming_server_port_number), field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].incoming_server_user_name), 0, field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].incoming_server_password), 0, field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].incoming_server_secure_connection), field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl[i].retrieval_mode), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].keep_mails_on_pop_server_after_download), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].check_interval), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].auto_download_size), field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl[i].outgoing_server_type), field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].outgoing_server_address), 0, field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].outgoing_server_port_number), field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].outgoing_server_user_name), 0, field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].outgoing_server_password), 0, field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].outgoing_server_secure_connection), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].outgoing_server_need_authentication), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].outgoing_server_use_same_authenticator), field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl[i].options.priority), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].options.keep_local_copy), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].options.req_delivery_receipt), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].options.req_read_receipt), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].options.download_limit), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].options.block_address), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].options.block_subject), field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].options.display_name_from), 0, field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].options.reply_with_body), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].options.forward_with_files), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].options.add_myname_card), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].options.add_signature), field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].options.signature), 0, field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl[i].options.add_my_address_to_bcc), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].pop_before_smtp), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].incoming_server_requires_apop), field_index++);
		_get_stmt_field_data_int(hStmt, (int *)&(p_data_tbl[i].smime_type), field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].certificate_path), 0, field_index++);
		_get_stmt_field_data_int(hStmt, (int *)&(p_data_tbl[i].cipher_type), field_index++);
		_get_stmt_field_data_int(hStmt, (int *)&(p_data_tbl[i].digest_type), field_index++);

		if (with_password == true) {
			/*  get password from the secure storage */
			char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];
			char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];

			EM_SAFE_FREE(p_data_tbl[i].incoming_server_password);
			EM_SAFE_FREE(p_data_tbl[i].outgoing_server_password);

			/*  get password file name */
			if ((error = _get_password_file_name(p_data_tbl[i].account_id, recv_password_file_name, send_password_file_name)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("_get_password_file_name failed. [%d]", error);
				error = EMAIL_ERROR_SECURED_STORAGE_FAILURE;
				goto FINISH_OFF;
			}
				
			/*  read password from secure storage */
			if ((error = _read_password_from_secure_storage(recv_password_file_name, &(p_data_tbl[i].incoming_server_password))) < 0 ) {
				EM_DEBUG_EXCEPTION("_read_password_from_secure_storage()  failed. [%d]", error);
				error = EMAIL_ERROR_SECURED_STORAGE_FAILURE;
				goto FINISH_OFF;
			}
			EM_DEBUG_LOG("recv_password_file_name[%s], incoming_server_password[%s]", recv_password_file_name,  p_data_tbl[i].incoming_server_password);
			
			if ((error = _read_password_from_secure_storage(send_password_file_name, &(p_data_tbl[i].outgoing_server_password))) < 0) {
				EM_DEBUG_EXCEPTION("_read_password_from_secure_storage()  failed. [%d]", error);
				error = EMAIL_ERROR_SECURED_STORAGE_FAILURE;
				goto FINISH_OFF;
			}
			EM_DEBUG_LOG("send_password_file_name[%s], password[%s]", send_password_file_name,  p_data_tbl[i].outgoing_server_password);
		}

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_LOG("after sqlite3_step(), i = %d, rc = %d.", i,  rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
	} 
	
	ret = true;
	
FINISH_OFF:
	if (ret == true)  {
		*account_list = p_data_tbl;
		*select_num = count;
		EM_DEBUG_LOG("COUNT : %d", count);
	}
	else if (p_data_tbl != NULL)
		emstorage_free_account(&p_data_tbl, count, NULL);

	if (hStmt != NULL)  {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);
	
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_maildata_by_servermailid(int account_id, char *server_mail_id, emstorage_mail_tbl_t** mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], server_mail_id[%s], mail[%p], transaction[%d], err_code[%p]", account_id, server_mail_id, mail, transaction, err_code);
	
	int ret = false, error = EMAIL_ERROR_NONE, result_count;
	char conditional_clause[QUERY_SIZE] = {0, };
	emstorage_mail_tbl_t* p_data_tbl = NULL;

	if (!server_mail_id || !mail) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (account_id == ALL_ACCOUNT)
		SNPRINTF(conditional_clause, QUERY_SIZE, "WHERE UPPER(server_mail_id) =UPPER('%s')", server_mail_id);
	else
		SNPRINTF(conditional_clause, QUERY_SIZE, "WHERE UPPER(server_mail_id) =UPPER('%s') AND account_id = %d", server_mail_id, account_id);

	EM_DEBUG_LOG("conditional_clause [%s]", conditional_clause);

	if(!emstorage_query_mail_tbl(conditional_clause, transaction, &p_data_tbl, &result_count, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_tbl failed [%d]", error);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*mail = p_data_tbl;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


static int _write_conditional_clause_for_getting_mail_list(int account_id, int mailbox_id, email_email_address_list_t* addr_list, int thread_id, int start_index, int limit_count, int search_type, const char *search_value, email_sort_type_t sorting, bool input_except_delete_flagged_mails, char *conditional_clause_string, int buffer_size, int *err_code)
{
	int cur_clause = 0, conditional_clause_count = 0, i = 0;

	if (account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("Invalid account_id [%d]", account_id);
		EM_RETURN_ERR_CODE(err_code, EMAIL_ERROR_INVALID_PARAM, false);
	}

	/*  where clause */
	if (account_id == ALL_ACCOUNT) {
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE mailbox_type not in (3, 5, 7, 8)"):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND mailbox_type not in (3, 5, 7, 8)");
	}
	else {
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE account_id = %d", account_id):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND account_id = %d", account_id);
	}
	
	if (mailbox_id > 0) {
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE mailbox_id = %d", mailbox_id):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND mailbox_id = %d", mailbox_id);
	}
	else if(account_id != ALL_ACCOUNT) {
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE mailbox_type not in (3, 5, 7, 8)"):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND mailbox_type not in (3, 5, 7, 8)");
	}

	if (thread_id > 0) {
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE thread_id = %d ", thread_id):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND thread_id = %d ", thread_id);
	}
	else if (thread_id == EMAIL_LIST_TYPE_THREAD)	{
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE thread_item_count > 0"):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND thread_item_count > 0");
	}
	else if (thread_id == EMAIL_LIST_TYPE_LOCAL) {
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE server_mail_status = 0"):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND server_mail_status = 0");
	}
 	else if (thread_id == EMAIL_LIST_TYPE_UNREAD) {
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE flags_seen_field == 0"):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND flags_seen_field == 0");
	}

	/*  EM_DEBUG_LOG("where clause added [%s]", conditional_clause_string); */
	if (addr_list && addr_list->address_count > 0) {
		if (!addr_list->address_type) {
			cur_clause += (conditional_clause_count++ == 0)?
				SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE email_address_sender IN(\"%s\"", addr_list->address_list[0]):
				SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND email_address_sender IN(\"%s\"", addr_list->address_list[0]);

			for (i = 1; i < addr_list->address_count; i++)
				cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, ",\"%s\"", addr_list->address_list[i]);

			cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, ")");
		} else {
			cur_clause += (conditional_clause_count++ == 0)?
				SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE full_address_to IN(\"%s\") or full_address_cc IN(\"%s\"", addr_list->address_list[0], addr_list->address_list[0]):
				SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND full_address_to IN(\"%s\") or full_address_cc IN(\"%s\"", addr_list->address_list[0], addr_list->address_list[0]);

			for (i = 1; i < addr_list->address_count; i++)
				cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, ",\"%s\"", addr_list->address_list[i]);

			cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, ")");		
		}
	}

	if (input_except_delete_flagged_mails) {
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE flags_deleted_field = 0"):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND flags_deleted_field = 0");
	}

	if (search_value) {
		switch (search_type) {
			case EMAIL_SEARCH_FILTER_SUBJECT:
				cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause,
					" %s (UPPER(subject) LIKE UPPER(\'%%%%%s%%%%\') ESCAPE '\\') ", conditional_clause_count++ ? "AND" : "WHERE", search_value);
				break;
			case EMAIL_SEARCH_FILTER_SENDER:
				cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause,
					" %s  ((UPPER(full_address_from) LIKE UPPER(\'%%%%%s%%%%\') ESCAPE '\\') "
					") ", conditional_clause_count++ ? "AND" : "WHERE", search_value);
				break;
			case EMAIL_SEARCH_FILTER_RECIPIENT:
				cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause,
					" %s ((UPPER(full_address_to) LIKE UPPER(\'%%%%%s%%%%\') ESCAPE '\\') "	
					"	OR (UPPER(full_address_cc) LIKE UPPER(\'%%%%%s%%%%\') ESCAPE '\\') "
					"	OR (UPPER(full_address_bcc) LIKE UPPER(\'%%%%%s%%%%\') ESCAPE '\\') "
					") ", conditional_clause_count++ ? "AND" : "WHERE", search_value, search_value, search_value);
				break;
			case EMAIL_SEARCH_FILTER_ALL:
				cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause,
					" %s (UPPER(subject) LIKE UPPER(\'%%%%%s%%%%\') ESCAPE '\\' "
					" 	OR (((UPPER(full_address_from) LIKE UPPER(\'%%%%%s%%%%\') ESCAPE '\\') "
					"			OR (UPPER(full_address_to) LIKE UPPER(\'%%%%%s%%%%\') ESCAPE '\\') "
					"			OR (UPPER(full_address_cc) LIKE UPPER(\'%%%%%s%%%%\') ESCAPE '\\') "
					"			OR (UPPER(full_address_bcc) LIKE UPPER(\'%%%%%s%%%%\') ESCAPE '\\') "
					"		) "
					"	)"
					")", conditional_clause_count++ ? "AND" : "WHERE", search_value, search_value, search_value, search_value, search_value);
				break;
		}
	}
	
	/*  EM_DEBUG_LOG("where clause [%s]", conditional_clause_string); */
	static char sorting_str[][50] = { 
			 " ORDER BY date_time DESC",						 /* case EMAIL_SORT_DATETIME_HIGH: */
			 " ORDER BY date_time ASC",						  /* case EMAIL_SORT_DATETIME_LOW: */
			 " ORDER BY full_address_from DESC, date_time DESC", /* case EMAIL_SORT_SENDER_HIGH: */
			 " ORDER BY full_address_from ASC, date_time DESC",  /* case EMAIL_SORT_SENDER_LOW: */
			 " ORDER BY full_address_to DESC, date_time DESC",   /* case EMAIL_SORT_RCPT_HIGH: */
			 " ORDER BY full_address_to ASC, date_time DESC",	/* case EMAIL_SORT_RCPT_LOW: */
			 " ORDER BY subject DESC, date_time DESC",		   /* case EMAIL_SORT_SUBJECT_HIGH: */
			 " ORDER BY subject ASC, date_time DESC",			/* case EMAIL_SORT_SUBJECT_LOW: */
			 " ORDER BY priority DESC, date_time DESC",		  /* case EMAIL_SORT_PRIORITY_HIGH: */
			 " ORDER BY priority ASC, date_time DESC",		   /* case EMAIL_SORT_PRIORITY_LOW: */
			 " ORDER BY attachment_count DESC, date_time DESC",  /* case EMAIL_SORT_ATTACHMENT_HIGH: */
			 " ORDER BY attachment_count ASC, date_time DESC",   /* case EMAIL_SORT_ATTACHMENT_LOW: */
			 " ORDER BY lock_status DESC, date_time DESC",	   /* case EMAIL_SORT_FAVORITE_HIGH: */
			 " ORDER BY lock_status ASC, date_time DESC",		/* case EMAIL_SORT_FAVORITE_LOW: */
			 " ORDER BY mailbox_name DESC, date_time DESC",	  /* case EMAIL_SORT_MAILBOX_NAME_HIGH: */
			 " ORDER BY mailbox_name ASC, date_time DESC"		/* case EMAIL_SORT_MAILBOX_NAME_LOW: */
			 };

	if (sorting < EMAIL_SORT_END && sorting >= 0)
		cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size, " %s", sorting_str[sorting]);
	else
		EM_DEBUG_LOG(" Invalid Sorting order ");

	/*  limit clause */
	if (start_index != -1 && limit_count != -1)
		cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size, " LIMIT %d, %d", start_index, limit_count);

	return true;
}


/**
  *	emstorage_get_mail_list - Get the mail list information.
  *	
  *
  */
INTERNAL_FUNC int emstorage_get_mail_list(int account_id, int mailbox_id, email_email_address_list_t* addr_list, int thread_id, int start_index, int limit_count, int search_type, const char *search_value, email_sort_type_t sorting, int transaction, email_mail_list_item_t** mail_list,  int *result_count,  int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_PROFILE_BEGIN(emstorage_get_mail_list_func);

	int ret = false, error = EMAIL_ERROR_NONE;
	char conditional_clause_string[QUERY_SIZE] = { 0, };

	if (account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("Invalid account_id [%d]", account_id);
		EM_RETURN_ERR_CODE(err_code, EMAIL_ERROR_INVALID_PARAM, false);
	}
	EM_IF_NULL_RETURN_VALUE(result_count, false);

	_write_conditional_clause_for_getting_mail_list(account_id, mailbox_id, addr_list, thread_id, start_index, limit_count, search_type, search_value, sorting, true, conditional_clause_string, QUERY_SIZE, &error);
	
	EM_DEBUG_LOG("conditional_clause_string[%s].", conditional_clause_string);

	if(!emstorage_query_mail_list(conditional_clause_string, transaction, mail_list, result_count, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_list [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(emstorage_get_mail_list_func);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


/**
  *	emstorage_get_mails - Get the Mail list information based on mailbox_name name
  *	
  *
  */
INTERNAL_FUNC int emstorage_get_mails(int account_id, int mailbox_id, email_email_address_list_t* addr_list, int thread_id, int start_index, int limit_count, email_sort_type_t sorting,  int transaction, emstorage_mail_tbl_t** mail_list, int *result_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_PROFILE_BEGIN(emStorageGetMails);
	
	int count = 0, ret = false, error = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t *p_data_tbl = NULL;
	char conditional_clause_string[QUERY_SIZE] = {0, };

	EM_IF_NULL_RETURN_VALUE(mail_list, false);
	EM_IF_NULL_RETURN_VALUE(result_count, false);

	if (!result_count || !mail_list || account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	_write_conditional_clause_for_getting_mail_list(account_id, mailbox_id, addr_list, thread_id, start_index, limit_count, 0, NULL, sorting, true, conditional_clause_string, QUERY_SIZE, &error);

	EM_DEBUG_LOG("conditional_clause_string [%s]", conditional_clause_string);

	if(!emstorage_query_mail_tbl(conditional_clause_string, transaction, &p_data_tbl, &count,  &error)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_tbl failed [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:
	if (ret == true) {
		*mail_list = p_data_tbl;
		*result_count = count;
		EM_DEBUG_LOG("COUNT : %d", count);
	}
	else if (p_data_tbl != NULL)
		emstorage_free_mail(&p_data_tbl, count, NULL);
	
	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(emStorageGetMails);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}



/**
  *	emstorage_get_searched_mail_list - Get the mail list information after filtering
  *	
  *
  */
INTERNAL_FUNC int emstorage_get_searched_mail_list(int account_id, int mailbox_id, int thread_id, int search_type, const char *search_value, int start_index, int limit_count, email_sort_type_t sorting, int transaction, email_mail_list_item_t** mail_list,  int *result_count,  int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false, error = EMAIL_ERROR_NONE;
	char conditional_clause[QUERY_SIZE] = {0, };
	char *temp_search_value = (char *)search_value;

	EM_IF_NULL_RETURN_VALUE(mail_list, false);
	EM_IF_NULL_RETURN_VALUE(result_count, false);

	if (!result_count || !mail_list || account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("select_num[%p], Mail_list[%p]", result_count, mail_list);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	temp_search_value = em_replace_all_string(temp_search_value, "_", "\\_");
	temp_search_value = em_replace_all_string(temp_search_value, "%", "\\%");

	_write_conditional_clause_for_getting_mail_list(account_id, mailbox_id, NULL, thread_id, start_index, limit_count, search_type, temp_search_value, sorting, true, conditional_clause, QUERY_SIZE, &error);
	
	EM_DEBUG_LOG("conditional_clause[%s]", conditional_clause);

	if(!emstorage_query_mail_list(conditional_clause, transaction, mail_list, result_count, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_list [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:
	EM_DEBUG_LOG("emstorage_get_searched_mail_list finish off");	
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


static int _get_password_file_name(int account_id, char *recv_password_file_name, char *send_password_file_name)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d]", account_id);

	if (account_id <= 0 || !recv_password_file_name || !send_password_file_name)  {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	sprintf(recv_password_file_name, ".email_account_%d_recv", account_id);
	sprintf(send_password_file_name, ".email_account_%d_send", account_id);
	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

static int _read_password_from_secure_storage(char *file_name, char **password)
{
	EM_DEBUG_FUNC_BEGIN("file_name[%s], password[%p]", file_name, password);

	if (!file_name || !password) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		return EMAIL_ERROR_INVALID_PARAM;
	}	

	size_t buf_len = 0, read_len = 0;
	ssm_file_info_t sfi;
	char *temp_password = NULL;
	int error_code_from_ssm = 0;
	int ret = EMAIL_ERROR_NONE;

	if ( (error_code_from_ssm = ssm_getinfo(file_name, &sfi, SSM_FLAG_SECRET_OPERATION, NULL)) < 0) {
		EM_DEBUG_EXCEPTION("ssm_getinfo() failed. [%d]", error_code_from_ssm);
		ret = EMAIL_ERROR_SECURED_STORAGE_FAILURE;
		goto FINISH_OFF;
	}

	buf_len = sfi.originSize;
	EM_DEBUG_LOG("password buf_len[%d]", buf_len);
	if ((temp_password = (char *)malloc(buf_len + 1)) == NULL) {
		EM_DEBUG_EXCEPTION("malloc failed...");
		ret = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memset(temp_password, 0x00, buf_len + 1);

	if ( (error_code_from_ssm = ssm_read(file_name, temp_password, buf_len, &read_len, SSM_FLAG_SECRET_OPERATION, NULL)) < 0) {
		EM_DEBUG_EXCEPTION("ssm_read() failed. [%d]", error_code_from_ssm);
		ret = EMAIL_ERROR_SECURED_STORAGE_FAILURE;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("password_file_name[%s], password[%s], originSize[%d], read len[%d]", file_name,  temp_password, sfi.originSize, read_len);

	*password = temp_password;
	temp_password = NULL;

FINISH_OFF:
	EM_SAFE_FREE(temp_password);
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_account_by_id(int account_id, int pulloption, emstorage_account_tbl_t **account, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], pulloption[%d], account[%p], transaction[%d], err_code[%p]", account_id, pulloption, account, transaction, err_code);

	if (!account)  {
		EM_DEBUG_EXCEPTION("account_id[%d], account[%p]", account_id, account);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	emstorage_account_tbl_t* p_data_tbl = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	int rc = -1;
	int sql_len = 0;
	char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	/*  Make query string */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT ");
	sql_len = strlen(sql_query_string);
		
	if (pulloption & EMAIL_ACC_GET_OPT_DEFAULT) {
		SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len,
			"incoming_server_type,"
			"incoming_server_address,"
			"user_email_address,"
			"incoming_server_user_name,"
			"retrieval_mode,"
			"incoming_server_port_number,"
			"incoming_server_secure_connection,"
			"outgoing_server_type,"
			"outgoing_server_address,"
			"outgoing_server_port_number,"
			"outgoing_server_need_authentication,"
			"outgoing_server_secure_connection,"
			"outgoing_server_user_name,"
			"user_display_name,"
			"reply_to_address,"
			"return_address,"
			"account_id,"
			"keep_mails_on_pop_server_after_download,"
			"auto_download_size,"
			"outgoing_server_use_same_authenticator,"
			"pop_before_smtp,"
			"incoming_server_requires_apop,"
			"logo_icon_path,"
			"user_data,"
			"user_data_length,"
			"check_interval,"
			"sync_status,");
		sql_len = strlen(sql_query_string); 
	}

	if (pulloption & EMAIL_ACC_GET_OPT_ACCOUNT_NAME) {
		SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len, " account_name, ");
		sql_len = strlen(sql_query_string); 
	}

	/*  get from secure storage, not from db */
	if (pulloption & EMAIL_ACC_GET_OPT_OPTIONS) {
		SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len,
			"priority,"
			"keep_local_copy,"
			"req_delivery_receipt,"
			"req_read_receipt,"
			"download_limit,"
			"block_address,"
			"block_subject,"
			"display_name_from,"
			"reply_with_body,"
			"forward_with_files,"
			"add_myname_card,"
			"add_signature,"
			"signature,"
			"add_my_address_to_bcc,"
			"account_svc_id,"
			"sync_disabled,"
			"default_mail_slot_size,"
			"smime_type,"
			"certificate_path,"
			"cipher_type,"
			"digest_type,");

		sql_len = strlen(sql_query_string); 
	}

	/*  dummy value, FROM WHERE clause */
	SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len, "0 FROM mail_account_tbl WHERE account_id = %d", account_id);

	/*  FROM clause */
	EM_DEBUG_LOG("query = [%s]", sql_query_string);

	/*  execute a sql and count rows */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no matched account found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

	/*  Assign query result to structure */
	if (!(p_data_tbl = (emstorage_account_tbl_t *)malloc(sizeof(emstorage_account_tbl_t) * 1)))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(p_data_tbl, 0x00, sizeof(emstorage_account_tbl_t) * 1);
	int col_index = 0;

	if (pulloption & EMAIL_ACC_GET_OPT_DEFAULT) {
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->incoming_server_type), col_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->incoming_server_address),  0, col_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->user_email_address), 0, col_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->incoming_server_user_name), 0, col_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->retrieval_mode), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->incoming_server_port_number), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->incoming_server_secure_connection), col_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->outgoing_server_type), col_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->outgoing_server_address), 0, col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->outgoing_server_port_number), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->outgoing_server_need_authentication), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->outgoing_server_secure_connection), col_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->outgoing_server_user_name), 0, col_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->user_display_name), 0, col_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->reply_to_address), 0, col_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->return_address), 0, col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->keep_mails_on_pop_server_after_download), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->auto_download_size), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->outgoing_server_use_same_authenticator), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->pop_before_smtp), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->incoming_server_requires_apop), col_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->logo_icon_path), 0, col_index++);
		_get_stmt_field_data_blob(hStmt, &p_data_tbl->user_data, col_index++);
		_get_stmt_field_data_int(hStmt, &p_data_tbl->user_data_length, col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->check_interval), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->sync_status), col_index++);
	}

	if (pulloption & EMAIL_ACC_GET_OPT_ACCOUNT_NAME)
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->account_name), 0, col_index++);

	if (pulloption & EMAIL_ACC_GET_OPT_PASSWORD) {
		/*  get password file name */
		if ((error = _get_password_file_name(p_data_tbl->account_id, recv_password_file_name, send_password_file_name)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("_get_password_file_name failed. [%d]", error);
			goto FINISH_OFF;
		}		

		/*  read password from secure storage */
		if ((error = _read_password_from_secure_storage(recv_password_file_name, &(p_data_tbl->incoming_server_password))) < 0) {
			EM_DEBUG_EXCEPTION(" _read_password_from_secure_storage()  failed. [%d]", error);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("recv_password_file_name[%s], password[%s]", recv_password_file_name,  p_data_tbl->incoming_server_password);
		
		if ((error = _read_password_from_secure_storage(send_password_file_name, &(p_data_tbl->outgoing_server_password))) < 0) {
			EM_DEBUG_EXCEPTION(" _read_password_from_secure_storage()  failed. [%d]", error);
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG("send_password_file_name[%s], password[%s]", send_password_file_name,  p_data_tbl->outgoing_server_password);
	}

	if (pulloption & EMAIL_ACC_GET_OPT_OPTIONS) {
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->options.priority), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->options.keep_local_copy), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->options.req_delivery_receipt), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->options.req_read_receipt), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->options.download_limit), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->options.block_address), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->options.block_subject), col_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->options.display_name_from), 0, col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->options.reply_with_body), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->options.forward_with_files), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->options.add_myname_card), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->options.add_signature), col_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->options.signature), 0, col_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->options.add_my_address_to_bcc), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_svc_id), col_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->sync_disabled), col_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->default_mail_slot_size), col_index++);
		_get_stmt_field_data_int(hStmt, (int *)&(p_data_tbl->smime_type), col_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->certificate_path), 0, col_index++);
		_get_stmt_field_data_int(hStmt, (int *)&(p_data_tbl->cipher_type), col_index++);
		_get_stmt_field_data_int(hStmt, (int *)&(p_data_tbl->digest_type), col_index++);
	}

	ret = true;

FINISH_OFF:
	if (ret == true)
		*account = p_data_tbl;
	else {
		if (p_data_tbl)
			emstorage_free_account((emstorage_account_tbl_t **)&p_data_tbl, 1, NULL);
	}
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_password_length_of_account(int account_id, int *password_length, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], password_length[%p], err_code[%p]", account_id, password_length, err_code);

	if (account_id <= 0 || password_length == NULL)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char *temp_password = NULL;
	

	/*  get password file name */
	if ((error = _get_password_file_name(account_id, recv_password_file_name, send_password_file_name)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_get_password_file_name failed.");
		goto FINISH_OFF;
	}		

	/*  read password from secure storage */
	if ((error = _read_password_from_secure_storage(recv_password_file_name, &temp_password)) < 0 || !temp_password) {
		EM_DEBUG_EXCEPTION(" _read_password_from_secure_storage()  failed...");
		goto FINISH_OFF;
	}

	*password_length = strlen(temp_password);
	
	EM_DEBUG_LOG("recv_password_file_name[%s], *password_length[%d]", recv_password_file_name,  *password_length);

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE(temp_password);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_update_account(int account_id, emstorage_account_tbl_t* account_tbl, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], account[%p], transaction[%d], err_code[%p]", account_id, account_tbl, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !account_tbl)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int error = EMAIL_ERROR_NONE;
	int rc, ret = false;
 
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
		
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_account_tbl SET"
		"  account_name = ?"
		", logo_icon_path = ?"
		", user_data = ?"
		", user_data_length = ?"
		", account_svc_id = ?"
		", sync_status = ?"
		", sync_disabled = ?"
		", default_mail_slot_size = ?"
		", user_display_name = ?"
		", user_email_address = ?"
		", reply_to_address = ?"
		", return_address = ?"
		", incoming_server_type = ?"
		", incoming_server_address = ?"
		", incoming_server_port_number = ?"
		", incoming_server_user_name = ?"
		", incoming_server_secure_connection = ?"
		", retrieval_mode = ?"
		", keep_mails_on_pop_server_after_download = ?"
		", check_interval = ?"
		", auto_download_size = ?"
		", outgoing_server_type = ?"
		", outgoing_server_address = ?"
		", outgoing_server_port_number = ?"
		", outgoing_server_user_name = ?"
		", outgoing_server_secure_connection = ?"
		", outgoing_server_need_authentication = ?"
		", outgoing_server_use_same_authenticator = ?"
		", priority = ?"
		", keep_local_copy = ?"
		", req_delivery_receipt = ?"
		", req_read_receipt = ?"
		", download_limit = ?"
		", block_address = ?"
		", block_subject = ?"
		", display_name_from = ?"
		", reply_with_body = ?"
		", forward_with_files = ?"
		", add_myname_card = ?"
		", add_signature = ?"
		", signature = ?"
		", add_my_address_to_bcc = ?"
		", pop_before_smtp = ?"
		", incoming_server_requires_apop = ?"
		", smime_type = ?"
		", certificate_path = ?"
		", cipher_type = ?"
		", digest_type = ?"
		" WHERE account_id = ?");	 
 
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("After sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_LOG("SQL[%s]", sql_query_string);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_prepare fail:(%d) %s", rc, sqlite3_errmsg(local_db_handle)));
	
	int i = 0;

	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->account_name, 0, ACCOUNT_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->logo_icon_path, 0, LOGO_ICON_PATH_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_blob(hStmt, i++, account_tbl->user_data, account_tbl->user_data_length);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->user_data_length);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->account_svc_id);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->sync_status);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->sync_disabled);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->default_mail_slot_size);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->user_display_name, 0, DISPLAY_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->user_email_address, 0, EMAIL_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->reply_to_address, 0, REPLY_TO_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->return_address, 0, RETURN_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_type);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->incoming_server_address, 0, RECEIVING_SERVER_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_port_number);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->incoming_server_user_name, 0, USER_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_secure_connection);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->retrieval_mode);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->keep_mails_on_pop_server_after_download);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->check_interval);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->auto_download_size);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->outgoing_server_type);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->outgoing_server_address, 0, SENDING_SERVER_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->outgoing_server_port_number);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->outgoing_server_user_name, 0, SENDING_USER_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->outgoing_server_secure_connection);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->outgoing_server_need_authentication);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->outgoing_server_use_same_authenticator);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.priority);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.keep_local_copy);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.req_delivery_receipt);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.req_read_receipt);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.download_limit);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.block_address);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.block_subject);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->options.display_name_from, 0, DISPLAY_NAME_FROM_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.reply_with_body);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.forward_with_files);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.add_myname_card);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.add_signature);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->options.signature, 0, SIGNATURE_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.add_my_address_to_bcc);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->pop_before_smtp);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_requires_apop);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->smime_type);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->certificate_path, 0, CERTIFICATE_PATH_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->cipher_type);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->digest_type);
	_bind_stmt_field_data_int(hStmt, i++, account_id);
 
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	/*  validate account existence */
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION(" no matched account found...");
	
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	/*  get password file name */
	if ((error = _get_password_file_name(account_id, recv_password_file_name, send_password_file_name)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_get_password_file_name failed.");
		goto FINISH_OFF;
	}

	/*  save passwords to the secure storage */
	EM_DEBUG_LOG("save to the secure storage : recv_file[%s], recv_pass[%s], send_file[%s], send_pass[%s]", recv_password_file_name, account_tbl->incoming_server_password, send_password_file_name, account_tbl->outgoing_server_password);
	if (account_tbl->incoming_server_password && (strlen(account_tbl->incoming_server_password) > 0)) {
		if (ssm_write_buffer(account_tbl->incoming_server_password, strlen(account_tbl->incoming_server_password), recv_password_file_name, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
			EM_DEBUG_EXCEPTION(" ssm_write_buffer failed -recv incoming_server_password : file[%s]", recv_password_file_name);
			error = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;		
		}
	}

	if (account_tbl->outgoing_server_password && (strlen(account_tbl->outgoing_server_password) > 0)) {
		if (ssm_write_buffer(account_tbl->outgoing_server_password, strlen(account_tbl->outgoing_server_password), send_password_file_name, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
			EM_DEBUG_EXCEPTION(" ssm_write_buffer failed -send password : file[%s]", send_password_file_name);
			error = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;		
		}
	}
	
	if (!emstorage_notify_storage_event(NOTI_ACCOUNT_UPDATE, account_tbl->account_id, 0, NULL, 0))
		EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event[ NOTI_ACCOUNT_UPDATE] : Notification Failed >>> ");
	ret = true;
	
FINISH_OFF:
 
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_set_field_of_accounts_with_integer_value(int account_id, char *field_name, int value, int transaction)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], field_name[%s], value[%d], transaction[%d]", account_id, field_name, value, transaction);
	int error = EMAIL_ERROR_NONE;
	int rc = 0;
	int result = 0;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = NULL;

	if (!account_id  || !field_name) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	local_db_handle = emstorage_get_db_connection();

	/* Write query string */
	SNPRINTF(sql_query_string, QUERY_SIZE, "UPDATE mail_account_tbl SET %s = %d WHERE account_id = %d", field_name, value, account_id);

	EM_DEBUG_LOG("sql_query_string [%s]", sql_query_string);

	/* Execute query */
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	if (sqlite3_changes(local_db_handle) == 0)
		EM_DEBUG_LOG("no mail matched...");


FINISH_OFF:
	result = (error == EMAIL_ERROR_NONE) ? true : false;
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, result, error);
	_DISCONNECT_DB;

	if (!emstorage_notify_storage_event(NOTI_ACCOUNT_UPDATE, account_id, 0, field_name, value))
		EM_DEBUG_EXCEPTION("emstorage_notify_storage_event failed : NOTI_ACCOUNT_UPDATE [%s,%d]", field_name, value);

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int emstorage_get_sync_status_of_account(int account_id, int *result_sync_status,int *err_code) 
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], result_sync_status [%p], err_code[%p]", account_id, result_sync_status, err_code);

	if(!result_sync_status) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int error = EMAIL_ERROR_NONE, rc, ret = false, sync_status, count, i, col_index;
	char sql_query_string[QUERY_SIZE] = {0, };
	char **result = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	if(account_id)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT sync_status FROM mail_account_tbl WHERE account_id = %d", account_id);	 
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT sync_status FROM mail_account_tbl");	
 
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (!count) {
		EM_DEBUG_EXCEPTION("no matched account found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

	col_index = 1;
	*result_sync_status = 0;

	for(i = 0; i < count; i++) {
		_get_table_field_data_int(result, &sync_status, col_index++);
		*result_sync_status |= sync_status;
	}
	
	EM_DEBUG_LOG("sync_status [%d]", sync_status);

	sqlite3_free_table(result);

	ret = true;
	
FINISH_OFF:
	
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_update_sync_status_of_account(int account_id, email_set_type_t set_operator, int sync_status, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], set_operator[%d], sync_status [%d], transaction[%d], err_code[%p]", account_id, set_operator, sync_status, transaction, err_code);
	
	int error = EMAIL_ERROR_NONE, rc, ret = false, set_value = sync_status, result_sync_status;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	if(set_operator != SET_TYPE_SET && account_id) {
		if(!emstorage_get_sync_status_of_account(account_id, &result_sync_status, &error)) {
			EM_DEBUG_EXCEPTION("emstorage_get_sync_status_of_account failed [%d]", error);
			goto FINISH_OFF;
		}
		switch(set_operator) {
			case SET_TYPE_UNION :
				set_value = result_sync_status | set_value;
				break;
			case SET_TYPE_MINUS :
				set_value = result_sync_status & (~set_value);
				break;
			default:
				EM_DEBUG_EXCEPTION("EMAIL_ERROR_NOT_SUPPORTED [%d]", set_operator);
				error = EMAIL_ERROR_NOT_SUPPORTED;
				break;
		}
		EM_DEBUG_LOG("set_value [%d]", set_value);
	}

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	if(account_id)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_account_tbl SET sync_status = %d WHERE account_id = %d", set_value, account_id);	 
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_account_tbl SET sync_status = %d WHERE incoming_server_type <> 5", set_value);	 

	EM_DEBUG_LOG("sql_query_string [%s]", sql_query_string);
 
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	rc = sqlite3_changes(local_db_handle);

	if (rc == 0) {
		EM_DEBUG_EXCEPTION("no matched account found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	if (!emstorage_notify_storage_event(NOTI_ACCOUNT_UPDATE_SYNC_STATUS, account_id, 0, NULL, 0))
		EM_DEBUG_EXCEPTION("emstorage_notify_storage_event[NOTI_ACCOUNT_UPDATE_SYNC_STATUS] : Notification failed");
	ret = true;
	
FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_add_account(emstorage_account_tbl_t* account_tbl, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account[%p], transaction[%d], err_code[%p]", account_tbl, transaction, err_code);
	
	if (!account_tbl)  {
		EM_DEBUG_EXCEPTION("account[%p], transaction[%d], err_code[%p]", account_tbl, transaction, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	int error_from_ssm = 0;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	char *sql = "SELECT max(rowid) FROM mail_account_tbl;";
	char **result = NULL;
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL==result[1]) rc = 1;
	else rc = atoi(result[1])+1;
	sqlite3_free_table(result);
	result = NULL;

	account_tbl->account_id = rc;

	if ((error = _get_password_file_name(account_tbl->account_id, recv_password_file_name, send_password_file_name)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_get_password_file_name failed.");
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG(" >>>> ACCOUNT_ID [ %d ] ", account_tbl->account_id);
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_account_tbl VALUES "
		"(	  "
		"    ? "  /*   account_id */
		"  , ? "  /*   account_name */
		"  , ? "  /*   logo_icon_path */
		"  , ? "  /*   user_data */
		"  , ? "  /*   user_data_length */
		"  , ? "  /*   account_svc_id */
		"  , ? "  /*   sync_status */
		"  , ? "  /*   sync_disabled */
		"  , ? "  /*   default_mail_slot_size */
		"  , ? "  /*   user_display_name */
		"  , ? "  /*   user_email_address */
		"  , ? "  /*   reply_to_address */
		"  , ? "  /*   return_address */
		"  , ? "  /*   incoming_server_type */
		"  , ? "  /*   incoming_server_address */
		"  , ? "  /*   incoming_server_port_number */
		"  , ? "  /*   incoming_server_user_name */
		"  , ? "  /*   incoming_server_password */
		"  , ? "  /*   incoming_server_secure_connection */
		"  , ? "  /*   retrieval_mode */
		"  , ? "  /*   keep_mails_on_pop_server_after_download */
		"  , ? "  /*   check_interval */
		"  , ? "  /*   auto_download_size */
		"  , ? "  /*   outgoing_server_type */
		"  , ? "  /*   outgoing_server_address */
		"  , ? "  /*   outgoing_server_port_number */
		"  , ? "  /*   outgoing_server_user_name */
		"  , ? "  /*   outgoing_server_password */
		"  , ? "  /*   outgoing_server_secure_connection */
		"  , ? "  /*   outgoing_server_need_authentication */
		"  , ? "  /*   outgoing_server_use_same_authenticator */
		"  , ? "  /*   priority */
		"  , ? "  /*   keep_local_copy */
		"  , ? "  /*   req_delivery_receipt */
		"  , ? "  /*   req_read_receipt */
		"  , ? "  /*   download_limit */
		"  , ? "  /*   block_address */
		"  , ? "  /*   block_subject */
		"  , ? "  /*   display_name_from */
		"  , ? "  /*   reply_with_body */
		"  , ? "  /*   forward_with_files */
		"  , ? "  /*   add_myname_card */
		"  , ? "  /*   add_signature */
		"  , ? "  /*   signature */
		"  , ? "  /*   add_my_address_to_bcc */
		"  , ? "  /*   pop_before_smtp */
		"  , ? "  /*   incoming_server_requires_apop */
		"  , ? "  /*   smime_type */
		"  , ? "  /*   certificate_path */
		"  , ? "  /*   cipher_type */
		"  , ? "  /*   digest_type */
		") ");
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG(">>>> SQL STMT [ %s ] ", sql_query_string);
	int i = 0;

	_bind_stmt_field_data_int(hStmt, i++, account_tbl->account_id);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->account_name, 0, ACCOUNT_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->logo_icon_path, 0, LOGO_ICON_PATH_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_blob(hStmt, i++, account_tbl->user_data, account_tbl->user_data_length);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->user_data_length);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->account_svc_id);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->sync_status);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->sync_disabled);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->default_mail_slot_size);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->user_display_name, 0, DISPLAY_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->user_email_address, 0, EMAIL_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->reply_to_address, 0, REPLY_TO_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->return_address, 0, RETURN_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_type);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->incoming_server_address, 0, RECEIVING_SERVER_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_port_number);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->incoming_server_user_name, 0, USER_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)"", 0, PASSWORD_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_secure_connection);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->retrieval_mode);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->keep_mails_on_pop_server_after_download);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->check_interval);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->auto_download_size);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->outgoing_server_type);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->outgoing_server_address, 0, SENDING_SERVER_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->outgoing_server_port_number);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->outgoing_server_user_name, 0, SENDING_USER_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)"", 0, SENDING_PASSWORD_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->outgoing_server_secure_connection);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->outgoing_server_need_authentication);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->outgoing_server_use_same_authenticator);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.priority);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.keep_local_copy);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.req_delivery_receipt);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.req_read_receipt);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.download_limit);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.block_address);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.block_subject);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->options.display_name_from, 0, DISPLAY_NAME_FROM_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.reply_with_body);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.forward_with_files);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.add_myname_card);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.add_signature);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->options.signature, 0, SIGNATURE_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.add_my_address_to_bcc);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->pop_before_smtp);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_requires_apop);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->smime_type);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->certificate_path, 0, FILE_NAME_LEN_IN_MAIL_CERTIFICATE_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->cipher_type);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->digest_type);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);

	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d, errmsg = %s.", rc, sqlite3_errmsg(local_db_handle)));
	

	/*  save passwords to the secure storage */
	EM_DEBUG_LOG("save to the secure storage : recv_file[%s], send_file[%s]", recv_password_file_name, send_password_file_name);
	if ( (error_from_ssm = ssm_write_buffer(account_tbl->incoming_server_password, strlen(account_tbl->incoming_server_password), recv_password_file_name, SSM_FLAG_SECRET_OPERATION, NULL)) < 0) {
		EM_DEBUG_EXCEPTION("ssm_write_buffer failed [%d] - recv password : file[%s]", error_from_ssm, recv_password_file_name);
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;		
	}
	if ( (error_from_ssm = ssm_write_buffer(account_tbl->outgoing_server_password, strlen(account_tbl->outgoing_server_password), send_password_file_name, SSM_FLAG_SECRET_OPERATION, NULL)) < 0) {
		EM_DEBUG_EXCEPTION("ssm_write_buffer failed [%d] - send password : file[%s]", error_from_ssm, send_password_file_name);
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;		
	}

	ret = true;

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);

	if (!emstorage_notify_storage_event(NOTI_ACCOUNT_ADD, account_tbl->account_id, 0, NULL, 0))
		EM_DEBUG_EXCEPTION("emstorage_notify_storage_event[NOTI_ACCOUNT_ADD] : Notification failed");
	
FINISH_OFF:
	
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG("sqlite3_finalize failed [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}		
	}
	else
		EM_DEBUG_LOG("hStmt is NULL!!!");
	
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_delete_account(int account_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], transaction[%d], err_code[%p]", account_id, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID)  {	
		EM_DEBUG_EXCEPTION(" account_id[%d]", account_id);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	/*  TODO : delete password files - file names can be obtained from db or a rule that makes a name */
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];

	/*  get password file name */
	if ((error = _get_password_file_name(account_id, recv_password_file_name, send_password_file_name)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_get_password_file_name failed.");
		goto FINISH_OFF;
	}

	/*  delete from db */
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_account_tbl WHERE account_id = %d", account_id);
 
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  validate account existence */
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no matched account found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

		/*  delete from secure storage */
	if (ssm_delete_file(recv_password_file_name,  SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION(" ssm_delete_file failed -recv password : file[%s]", recv_password_file_name);
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;		
	}
	if (ssm_delete_file(send_password_file_name,  SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION(" ssm_delete_file failed -send password : file[%s]", send_password_file_name);
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;		
	}
	ret = true;
	
FINISH_OFF:
 
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_free_account(emstorage_account_tbl_t** account_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_list[%p], count[%d], err_code[%p]", account_list, count, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	if (count > 0)  {
		if (!account_list || !*account_list)  {
			EM_DEBUG_EXCEPTION("account_list[%p], count[%d]", account_list, count);
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		emstorage_account_tbl_t* p = *account_list;
		int i = 0;
		
		for (; i < count; i++)  {
			EM_SAFE_FREE(p[i].account_name);
			EM_SAFE_FREE(p[i].incoming_server_address);
			EM_SAFE_FREE(p[i].user_email_address);
			EM_SAFE_FREE(p[i].user_data);
			EM_SAFE_FREE(p[i].incoming_server_user_name);
			EM_SAFE_FREE(p[i].incoming_server_password);
			EM_SAFE_FREE(p[i].outgoing_server_address);
			EM_SAFE_FREE(p[i].outgoing_server_user_name);
			EM_SAFE_FREE(p[i].outgoing_server_password);
			EM_SAFE_FREE(p[i].user_display_name);
			EM_SAFE_FREE(p[i].reply_to_address);
			EM_SAFE_FREE(p[i].return_address);
			EM_SAFE_FREE(p[i].logo_icon_path);
			EM_SAFE_FREE(p[i].options.display_name_from);
			EM_SAFE_FREE(p[i].options.signature);
			EM_SAFE_FREE(p[i].certificate_path);
		}
		
		EM_SAFE_FREE(p); 
		*account_list = NULL;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mailbox_count(int account_id, int local_yn, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], count[%p], transaction[%d], err_code[%p]", account_id, local_yn, count, transaction, err_code);
	
	if ((account_id < FIRST_ACCOUNT_ID) || (count == NULL))  {
		EM_DEBUG_EXCEPTION(" account_list[%d], local_yn[%d], count[%p]", account_id, local_yn, count);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_box_tbl WHERE account_id = %d AND local_yn = %d", account_id, local_yn);
	
	char **result;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);
 

	ret = true;
	
FINISH_OFF:
 
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mailbox_list(int account_id, int local_yn, email_mailbox_sort_type_t sort_type, int *select_num, emstorage_mailbox_tbl_t** mailbox_list, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], select_num[%p], mailbox_list[%p], transaction[%d], err_code[%p]", account_id, local_yn, select_num, mailbox_list, transaction, err_code);
	
	if (!select_num || !mailbox_list) {
		EM_DEBUG_EXCEPTION("Invalid parameters");
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;

		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char conditional_clause_string[QUERY_SIZE] = {0, };
	char ordering_clause_string[QUERY_SIZE] = {0, };
		
	if (account_id == ALL_ACCOUNT) {
		if (local_yn == EMAIL_MAILBOX_FROM_SERVER || local_yn == EMAIL_MAILBOX_FROM_LOCAL)
			SNPRINTF(conditional_clause_string + strlen(conditional_clause_string), sizeof(conditional_clause_string)-(strlen(conditional_clause_string)+1), " WHERE local_yn = %d ", local_yn);
	}
	else {
		SNPRINTF(conditional_clause_string, sizeof(conditional_clause_string), "WHERE account_id = %d  ", account_id);
		if (local_yn == EMAIL_MAILBOX_FROM_SERVER || local_yn == EMAIL_MAILBOX_FROM_LOCAL)
			SNPRINTF(conditional_clause_string + strlen(conditional_clause_string), sizeof(conditional_clause_string)-(strlen(conditional_clause_string)+1), " AND local_yn = %d ", local_yn);
	}

	EM_DEBUG_LOG("conditional_clause_string[%s]", conditional_clause_string);

	switch (sort_type) {
		case EMAIL_MAILBOX_SORT_BY_NAME_ASC :
			SNPRINTF(ordering_clause_string, QUERY_SIZE, " ORDER BY mailbox_name ASC");
			break;

		case EMAIL_MAILBOX_SORT_BY_NAME_DSC :
			SNPRINTF(ordering_clause_string, QUERY_SIZE, " ORDER BY mailbox_name DESC");
			break;

		case EMAIL_MAILBOX_SORT_BY_TYPE_ASC :
			SNPRINTF(ordering_clause_string, QUERY_SIZE, " ORDER BY mailbox_type ASC");
			break;

		case EMAIL_MAILBOX_SORT_BY_TYPE_DSC :
			SNPRINTF(ordering_clause_string, QUERY_SIZE, " ORDER BY mailbox_type DEC");
			break;
	}

	EM_DEBUG_LOG("ordering_clause_string[%s]", ordering_clause_string);

	if( (error = emstorage_query_mailbox_tbl(conditional_clause_string, ordering_clause_string, 0, transaction, mailbox_list, select_num)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_mailbox_tbl failed [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mailbox_list_ex(int account_id, int local_yn, int with_count, int *select_num, emstorage_mailbox_tbl_t** mailbox_list, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], select_num[%p], mailbox_list[%p], transaction[%d], err_code[%p]", account_id, local_yn, select_num, mailbox_list, transaction, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int where_clause_count = 0;
	char conditional_clause_string[QUERY_SIZE] = {0, };
	char ordering_clause_string[QUERY_SIZE] = {0, };

	if (!select_num || !mailbox_list) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (local_yn == EMAIL_MAILBOX_FROM_SERVER || local_yn == EMAIL_MAILBOX_FROM_LOCAL) {
		SNPRINTF(conditional_clause_string + strlen(conditional_clause_string), sizeof(conditional_clause_string)-(strlen(conditional_clause_string)+1), "WHERE local_yn = %d ",  local_yn);
		where_clause_count++;
	}

	if (account_id > 0) {
		if (where_clause_count == 0) {
			SNPRINTF(conditional_clause_string + strlen(conditional_clause_string), sizeof(conditional_clause_string)-(strlen(conditional_clause_string)+1), " WHERE ");
			SNPRINTF(conditional_clause_string + strlen(conditional_clause_string), sizeof(conditional_clause_string)-(strlen(conditional_clause_string)+1), " account_id = %d ", account_id);
		}
		else
			SNPRINTF(conditional_clause_string + strlen(conditional_clause_string), sizeof(conditional_clause_string)-(strlen(conditional_clause_string)+1), " AND account_id = %d ", account_id);
	}

	SNPRINTF(ordering_clause_string, QUERY_SIZE, " ORDER BY MBT.mailbox_name ");
	EM_DEBUG_LOG("conditional_clause_string[%s]", conditional_clause_string);
	EM_DEBUG_LOG("ordering_clause_string[%s]", ordering_clause_string);

	if( (error = emstorage_query_mailbox_tbl(conditional_clause_string, ordering_clause_string, 1, 1, mailbox_list, select_num)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_mailbox_tbl failed [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_child_mailbox_list(int account_id, char *parent_mailbox_name, int *select_num, emstorage_mailbox_tbl_t** mailbox_list, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], parent_mailbox_name[%p], select_num[%p], mailbox_list[%p], transaction[%d], err_code[%p]", account_id, parent_mailbox_name, select_num, mailbox_list, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID || !select_num || !mailbox_list)  {
		EM_DEBUG_EXCEPTION("account_id[%d], parent_mailbox_name[%p], select_num[%p], mailbox_list[%p]", account_id, parent_mailbox_name, select_num, mailbox_list);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char conditional_clause_string[QUERY_SIZE] = {0, };

	if (parent_mailbox_name)
		SNPRINTF(conditional_clause_string, sizeof(conditional_clause_string), "WHERE account_id = %d  AND UPPER(mailbox_name) LIKE UPPER('%s/%%')", account_id, parent_mailbox_name);
	else
		SNPRINTF(conditional_clause_string, sizeof(conditional_clause_string), "WHERE account_id = %d  AND (mailbox_name NOT LIKE '%%/%%')", account_id);

	EM_DEBUG_LOG("conditional_clause_string", conditional_clause_string);

	if( (error = emstorage_query_mailbox_tbl(conditional_clause_string, " ORDER BY mailbox_name", 0, transaction, mailbox_list, select_num)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_mailbox_tbl failed [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
 
INTERNAL_FUNC int emstorage_get_mailbox_by_modifiable_yn(int account_id, int local_yn, int *select_num, emstorage_mailbox_tbl_t** mailbox_list, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], select_num[%p], mailbox_list[%p], transaction[%d], err_code[%p]", account_id, local_yn, select_num, mailbox_list, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID || !select_num || !mailbox_list)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;

		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char conditional_clause_string[QUERY_SIZE] = {0, };
	
	SNPRINTF(conditional_clause_string, sizeof(conditional_clause_string), "WHERE account_id = %d AND modifiable_yn = 0", account_id);
	EM_DEBUG_LOG("conditional_clause_string [%s]", conditional_clause_string);
	
	if( (error = emstorage_query_mailbox_tbl(conditional_clause_string, " ORDER BY mailbox_name", 0, transaction, mailbox_list, select_num)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_mailbox_tbl failed [%d]", error);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_stamp_last_sync_time_of_mailbox(int input_mailbox_id, int input_transaction)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id [%d], input_transaction [%d]", input_mailbox_id, input_transaction);

	int      result_code = false;
	int      error = EMAIL_ERROR_NONE;
	int      rc;
	time_t   current_time = 0;
	char     sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = NULL;

	if (!input_mailbox_id )  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	time(&current_time);

	local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(input_transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_box_tbl SET"
		" last_sync_time = %d"
		" WHERE mailbox_id = %d"
		, (int)current_time
		, input_mailbox_id);

	EM_DEBUG_LOG("sql_query_string [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

FINISH_OFF:

	if(error == EMAIL_ERROR_NONE)
		result_code = true;

	EMSTORAGE_FINISH_WRITE_TRANSACTION(input_transaction, result_code, error);
	_DISCONNECT_DB;

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int emstorage_get_mailbox_by_name(int account_id, int local_yn, char *mailbox_name, emstorage_mailbox_tbl_t** result_mailbox, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], mailbox_name[%s], result_mailbox[%p], transaction[%d], err_code[%p]", account_id, local_yn, mailbox_name, result_mailbox, transaction, err_code);
	EM_PROFILE_BEGIN(profile_emstorage_get_mailbox_by_name);
	
	if (account_id < FIRST_ACCOUNT_ID || !mailbox_name || !result_mailbox)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], local_yn[%d], mailbox_name[%s], result_mailbox[%p]", account_id, local_yn, mailbox_name, result_mailbox);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int result_count = 0;
	char conditional_clause_string[QUERY_SIZE] = {0, };

	if(strcmp(mailbox_name, EMAIL_SEARCH_RESULT_MAILBOX_NAME) == 0) {
		if (!(*result_mailbox = (emstorage_mailbox_tbl_t*)em_malloc(sizeof(emstorage_mailbox_tbl_t))))  {
			EM_DEBUG_EXCEPTION("malloc failed...");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		(*result_mailbox)->mailbox_id                 = 0;
		(*result_mailbox)->account_id                 = account_id;
		(*result_mailbox)->local_yn                   = 1;
		(*result_mailbox)->mailbox_name               = EM_SAFE_STRDUP(mailbox_name);
		(*result_mailbox)->mailbox_type               = EMAIL_MAILBOX_TYPE_SEARCH_RESULT;
		(*result_mailbox)->alias                      = EM_SAFE_STRDUP(mailbox_name);
		(*result_mailbox)->sync_with_server_yn        = 0;
		(*result_mailbox)->modifiable_yn              = 1;
		(*result_mailbox)->total_mail_count_on_server = 1;
		(*result_mailbox)->has_archived_mails         = 0;
		(*result_mailbox)->mail_slot_size             = 0x0FFFFFFF;
	}
	else {

		if (local_yn == -1)
			SNPRINTF(conditional_clause_string, sizeof(conditional_clause_string), "WHERE account_id = %d AND mailbox_name = '%s'", account_id, mailbox_name);
		else
			SNPRINTF(conditional_clause_string, sizeof(conditional_clause_string), "WHERE account_id = %d AND local_yn = %d AND mailbox_name = '%s'", account_id, local_yn, mailbox_name);
	
		EM_DEBUG_LOG("conditional_clause_string = [%s]", conditional_clause_string);

		if( (error = emstorage_query_mailbox_tbl(conditional_clause_string, "", 0, transaction, result_mailbox, &result_count)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_query_mailbox_tbl failed [%d]", error);
			goto FINISH_OFF;
		}
	}

	ret = true;
	
FINISH_OFF:

	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(profile_emstorage_get_mailbox_by_name);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mailbox_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type, emstorage_mailbox_tbl_t** mailbox_name, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_type[%d], mailbox_name[%p], transaction[%d], err_code[%p]", account_id, mailbox_type, mailbox_name, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID || (mailbox_type < EMAIL_MAILBOX_TYPE_INBOX || mailbox_type > EMAIL_MAILBOX_TYPE_USER_DEFINED) || !mailbox_name) {

		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_type[%d], mailbox_name[%p]", account_id, mailbox_type, mailbox_name);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	emstorage_mailbox_tbl_t *p_data_tbl = NULL;
	emstorage_account_tbl_t *account = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0,};
	char *fields = "mailbox_id, account_id, local_yn, mailbox_name, mailbox_type, alias, sync_with_server_yn, modifiable_yn, total_mail_count_on_server, has_archived_mails, mail_slot_size ";
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	/* validate account */
	/*  Check whether the account exists. */
	if (!emstorage_get_account_by_id(account_id, EMAIL_ACC_GET_OPT_ACCOUNT_NAME,  &account, true, &error) || !account) {
		EM_DEBUG_EXCEPTION(" emstorage_get_account_by_id failed - %d", error);
		goto FINISH_OFF;
	}

	if (account)
		emstorage_free_account(&account, 1, NULL);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s FROM mail_box_tbl WHERE account_id = %d AND mailbox_type = %d ", fields, account_id, mailbox_type);

	EM_DEBUG_LOG("query = [%s]", sql_query_string);
 		
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; }, ("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; }, ("sqlite3_step fail:%d", rc));
	
	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION(" no matched mailbox_name found...");
		error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
		goto FINISH_OFF;
	}

	
	if (!(p_data_tbl = (emstorage_mailbox_tbl_t*)malloc(sizeof(emstorage_mailbox_tbl_t))))  {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emstorage_mailbox_tbl_t));
	
	int col_index = 0;

	_get_stmt_field_data_int(hStmt, &(p_data_tbl->mailbox_id), col_index++);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), col_index++);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->local_yn), col_index++);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->mailbox_name), 0, col_index++);
	_get_stmt_field_data_int(hStmt, (int *)&(p_data_tbl->mailbox_type), col_index++);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->alias), 0, col_index++);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->sync_with_server_yn), col_index++);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->modifiable_yn), col_index++);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->total_mail_count_on_server), col_index++);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->has_archived_mails), col_index++);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->mail_slot_size), col_index++);

	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*mailbox_name = p_data_tbl;
	else if (p_data_tbl != NULL)
		emstorage_free_mailbox(&p_data_tbl, 1, NULL);
	
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
 	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mailbox_by_id(int input_mailbox_id, emstorage_mailbox_tbl_t** output_mailbox)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d], output_mailbox[%p]", input_mailbox_id, output_mailbox);

	if (input_mailbox_id <= 0 || !output_mailbox)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return  EMAIL_ERROR_INVALID_PARAM;
	}

	int  ret = EMAIL_ERROR_NONE;
	int  result_count = 0;
	char conditional_clause_string[QUERY_SIZE] = {0, };

	SNPRINTF(conditional_clause_string, sizeof(conditional_clause_string), "WHERE mailbox_id = %d", input_mailbox_id);

	EM_DEBUG_LOG("conditional_clause_string = [%s]", conditional_clause_string);

	if( (ret = emstorage_query_mailbox_tbl(conditional_clause_string, "", false, false, output_mailbox, &result_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_mailbox_tbl failed [%d]", ret);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mailbox_id_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type, int *mailbox_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_type[%d], mailbox_id[%p], transaction[%d], err_code[%p]", account_id, mailbox_type, mailbox_id, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID || (mailbox_type < EMAIL_MAILBOX_TYPE_INBOX || mailbox_type > EMAIL_MAILBOX_TYPE_ALL_EMAILS) || !mailbox_id)  {
		EM_DEBUG_EXCEPTION("account_id[%d], mailbox_type[%d], mailbox_id[%p]", account_id, mailbox_type, mailbox_id);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	emstorage_account_tbl_t* account = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	/*  Check whether the account exists. */
	if (!emstorage_get_account_by_id(account_id, EMAIL_ACC_GET_OPT_ACCOUNT_NAME,  &account, true, &error) || !account) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed - %d", error);
		goto FINISH_OFF;
	}

	if (account )
		emstorage_free_account(&account, 1, NULL);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mailbox_id  FROM mail_box_tbl WHERE account_id = %d AND mailbox_type = %d ", account_id, mailbox_type);

	EM_DEBUG_LOG("query = [%s]", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION("no matched mailbox_name found...");
		error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
		goto FINISH_OFF;
	}

	_get_stmt_field_data_int(hStmt, mailbox_id, 0);

	ret = true;

FINISH_OFF:
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mailbox_name_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type, char **mailbox_name, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_type[%d], mailbox_name[%p], transaction[%d], err_code[%p]", account_id, mailbox_type, mailbox_name, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID || (mailbox_type < EMAIL_MAILBOX_TYPE_INBOX || mailbox_type > EMAIL_MAILBOX_TYPE_ALL_EMAILS) || !mailbox_name)  {
		EM_DEBUG_EXCEPTION("account_id[%d], mailbox_type[%d], mailbox_name[%p]", account_id, mailbox_type, mailbox_name);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	emstorage_account_tbl_t* account = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	/*  Check whether the account exists. */
	if (!emstorage_get_account_by_id(account_id, EMAIL_ACC_GET_OPT_ACCOUNT_NAME,  &account, true, &error) || !account) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed - %d", error);
		goto FINISH_OFF;
	}

	if (account )
		emstorage_free_account(&account, 1, NULL);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mailbox_name  FROM mail_box_tbl WHERE account_id = %d AND mailbox_type = %d ", account_id, mailbox_type);

	EM_DEBUG_LOG("query = [%s]", sql_query_string);

	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION("no matched mailbox_name found...");
		error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
		goto FINISH_OFF;
	}
 
	_get_stmt_field_data_string(hStmt, mailbox_name, 0, 0);

	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_update_mailbox_modifiable_yn(int account_id, int local_yn, char *mailbox_name, int modifiable_yn, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], local_yn [%d], mailbox_name [%p], modifiable_yn [%d], transaction [%d], err_code [%p]", account_id, local_yn, mailbox_name, modifiable_yn, transaction, err_code);
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_box_tbl SET"
		" modifiable_yn = %d"
		" WHERE account_id = %d"
		" AND local_yn = %d"
		" AND mailbox_name = '%s'"
		, modifiable_yn
		, account_id
		, local_yn
		, mailbox_name);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
 
	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}


INTERNAL_FUNC int emstorage_update_mailbox_total_count(int account_id, int input_mailbox_id, int total_count_on_server, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], input_mailbox_id[%d], total_count_on_server[%d], transaction[%d], err_code[%p]", account_id, input_mailbox_id, total_count_on_server,  transaction, err_code);
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	if (account_id <= 0 || input_mailbox_id <= 0)  {
		EM_DEBUG_EXCEPTION("account_id[%d], input_mailbox_id[%d]", account_id, input_mailbox_id);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_box_tbl SET"
		" total_mail_count_on_server = %d"
		" WHERE account_id = %d"
		" AND mailbox_id = %d"
		, total_count_on_server
		, account_id
		, input_mailbox_id);
	EM_DEBUG_LOG("query[%s]", sql_query_string);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
 
	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}


INTERNAL_FUNC int emstorage_update_mailbox(int account_id, int local_yn, int input_mailbox_id, emstorage_mailbox_tbl_t* result_mailbox, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], input_mailbox_id[%d], result_mailbox[%p], transaction[%d], err_code[%p]", account_id, local_yn, input_mailbox_id, result_mailbox, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || input_mailbox_id <= 0 || !result_mailbox)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], local_yn[%d], input_mailbox_id[%d], result_mailbox[%p]", account_id, local_yn, input_mailbox_id, result_mailbox);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	DB_STMT hStmt = NULL;
	int i = 0;
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	if (local_yn != -1) {
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"UPDATE mail_box_tbl SET"
			"  mailbox_id = ?"
			", mailbox_name = ?"
			", mailbox_type = ?"
			", alias = ?"
			", sync_with_server_yn = ?"
			", modifiable_yn= ?"
			", mail_slot_size= ?"
			", total_mail_count_on_server = ?"
			" WHERE account_id = %d"
			" AND local_yn = %d"
			" AND mailbox_id = '%d'"
			, account_id
			, local_yn
			, input_mailbox_id);
	}
	else {
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"UPDATE mail_box_tbl SET"
			"  mailbox_id = ?"
			", mailbox_name = ?"
			", mailbox_type = ?"
			", alias = ?"
			", sync_with_server_yn = ?"
			", modifiable_yn= ?"
			", mail_slot_size= ?"
			", total_mail_count_on_server = ?"
			" WHERE account_id = %d"
			" AND mailbox_id = '%d'"
			, account_id
			, input_mailbox_id);
	}

		
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bind_stmt_field_data_int(hStmt, i++, result_mailbox->mailbox_id);
	_bind_stmt_field_data_string(hStmt, i++, (char *)result_mailbox->mailbox_name ? result_mailbox->mailbox_name : "", 0, MAILBOX_NAME_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bind_stmt_field_data_int(hStmt, i++, result_mailbox->mailbox_type);
	_bind_stmt_field_data_string(hStmt, i++, (char *)result_mailbox->alias ? result_mailbox->alias : "", 0, ALIAS_LEN_IN_MAIL_BOX_TBL);
	_bind_stmt_field_data_int(hStmt, i++, result_mailbox->sync_with_server_yn);
	_bind_stmt_field_data_int(hStmt, i++, result_mailbox->modifiable_yn);
	_bind_stmt_field_data_int(hStmt, i++, result_mailbox->mail_slot_size);
	_bind_stmt_field_data_int(hStmt, i++, result_mailbox->total_mail_count_on_server);
	

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
 	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_update_mailbox_type(int account_id, int local_yn, char *mailbox_name, email_mailbox_type_e new_mailbox_type, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], mailbox_name[%s], new_mailbox_type[%d], transaction[%d], err_code[%p]", account_id, local_yn, mailbox_name, new_mailbox_type, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !mailbox_name)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], local_yn[%d], mailbox_name[%s]", account_id, local_yn, mailbox_name);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	EM_DEBUG_LOG("emstorage_update_mailbox_type");
	
	DB_STMT hStmt = NULL;
	int i = 0;

	/*  Update mail_box_tbl */
	if (local_yn != -1) {
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"UPDATE mail_box_tbl SET"
			" mailbox_type = ?"
			" WHERE account_id = %d"
			" AND local_yn = %d"
			" AND mailbox_name = '%s'"
			, account_id
			, local_yn
			, mailbox_name);
	}
	else {
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"UPDATE mail_box_tbl SET"
			" mailbox_type = ?"
			" WHERE account_id = %d"
			" AND mailbox_name = '%s'"
			, account_id
			, mailbox_name);
	}

	EM_DEBUG_LOG("SQL(%s)", sql_query_string);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bind_stmt_field_data_int(hStmt, i++, new_mailbox_type);
	

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
		hStmt = NULL;
	}
	
	/*  Update mail_tbl */
	i = 0;
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"UPDATE mail_tbl SET"
			" mailbox_type = ?"
			" WHERE account_id = %d"
			" AND mailbox_name = '%s'"
			, account_id
			, mailbox_name);

	EM_DEBUG_LOG("SQL[%s]", sql_query_string);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bind_stmt_field_data_int(hStmt, i++, new_mailbox_type);
	

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
 	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_set_local_mailbox(int input_mailbox_id, int input_is_local_mailbox, int transaction)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d], new_mailbox_type[%d], transaction[%d], err_code[%p]", input_mailbox_id, input_is_local_mailbox, transaction);

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	if (input_mailbox_id < 0)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	EM_DEBUG_LOG("emstorage_update_mailbox_type");

	DB_STMT hStmt = NULL;
	int i = 0;

	/*  Update mail_box_tbl */
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_box_tbl SET"
		" local_yn = ?"
		" WHERE mailbox_id = %d"
		, input_mailbox_id);

	EM_DEBUG_LOG("SQL(%s)", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bind_stmt_field_data_int(hStmt, i++, input_is_local_mailbox);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
		hStmt = NULL;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}


INTERNAL_FUNC int emstorage_add_mailbox(emstorage_mailbox_tbl_t* mailbox_tbl, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_tbl[%p], transaction[%d], err_code[%p]", mailbox_tbl, transaction, err_code);
	
	if (!mailbox_tbl)  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0,};
	char **result = NULL;
	time_t current_time;
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	EM_SAFE_STRCPY(sql_query_string, "SELECT max(rowid) FROM mail_box_tbl;");

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	time(&current_time);

	if (NULL == result[1])
		rc = 1;
	else
		rc = atoi(result[1]) + 1;
	sqlite3_free_table(result);

	memset(sql_query_string, 0, sizeof(char) * QUERY_SIZE);

	mailbox_tbl->mailbox_id = rc;

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_box_tbl VALUES "
		"( ?"    /* mailbox_id */
		", ?"    /* account_id */
		", ?"    /* local_yn */
		", ?"    /* mailbox_name */
		", ?"    /* mailbox_type */
		", ?"    /* alias */
		", ?"    /* sync_with_server_yn */
		", ?"    /* modifiable_yn */
		", ?"    /* total_mail_count_on_server */
		", ?"    /* has_archived_mails */
		", ?"    /* mail_slot_size */
		", ? )");/* last_sync_time */
	
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("After sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	int col_index = 0;

	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->mailbox_id);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->account_id);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->local_yn);
	_bind_stmt_field_data_string(hStmt, col_index++, (char *)mailbox_tbl->mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_BOX_TBL);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->mailbox_type);
	_bind_stmt_field_data_string(hStmt, col_index++, (char *)mailbox_tbl->alias, 0, ALIAS_LEN_IN_MAIL_BOX_TBL);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->sync_with_server_yn);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->modifiable_yn);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->total_mail_count_on_server);
	_bind_stmt_field_data_int(hStmt, col_index++, 0);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->mail_slot_size);
	_bind_stmt_field_data_int(hStmt, col_index++, current_time);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%dn", rc));

	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (error == EMAIL_ERROR_NONE && !emstorage_notify_storage_event(NOTI_MAILBOX_ADD, mailbox_tbl->account_id, mailbox_tbl->mailbox_id, mailbox_tbl->mailbox_name, 0))
		EM_DEBUG_EXCEPTION("emstorage_notify_storage_event[ NOTI_MAILBOX_ADD] : Notification Failed");

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_set_all_mailbox_modifiable_yn(int account_id, int modifiable_yn, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], modifiable_yn[%d], err_code[%p]", account_id, modifiable_yn, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID)  {

		EM_DEBUG_EXCEPTION("account_id[%d]", account_id);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0,};

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);


	SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_box_tbl SET modifiable_yn = %d WHERE account_id = %d", modifiable_yn, account_id);
 
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) 
		EM_DEBUG_EXCEPTION("All mailbox_name modifiable_yn set to 0 already");
 
		
	ret = true;
	
FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
	

}

INTERNAL_FUNC int emstorage_delete_mailbox(int account_id, int local_yn, int input_mailbox_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], input_mailbox_id[%d], transaction[%d], err_code[%p]", account_id, local_yn, input_mailbox_id, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID)  {	
		EM_DEBUG_EXCEPTION(" account_id[%d], local_yn[%d], input_mailbox_id[%d]", account_id, local_yn, input_mailbox_id);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	if (local_yn == -1)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_box_tbl WHERE account_id = %d ", account_id);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_box_tbl WHERE account_id = %d AND local_yn = %d ", account_id, local_yn);
	
	if (input_mailbox_id > 0)  {		/*  0 means all mailbox_name */
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(1+ strlen(sql_query_string)), "AND mailbox_id = %d", input_mailbox_id);
	}
	
	EM_DEBUG_LOG("mailbox sql_query_string [%s]", sql_query_string);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no (matched) mailbox_name found...");
		error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
		ret = true;
	}
 	ret = true;
	
FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if(error == EMAIL_ERROR_NONE) {
		if (!emstorage_notify_storage_event(NOTI_MAILBOX_DELETE, account_id, input_mailbox_id, NULL, 0))
			EM_DEBUG_EXCEPTION("emstorage_notify_storage_event[ NOTI_MAILBOX_ADD] : Notification Failed");
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_free_mailbox(emstorage_mailbox_tbl_t** mailbox_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_list[%p], count[%d], err_code[%p]", mailbox_list, count, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
		
	if (count > 0)  {
		if (!mailbox_list || !*mailbox_list)  {
			EM_DEBUG_EXCEPTION(" mailbox_list[%p], count[%d]", mailbox_list, count);
			
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		emstorage_mailbox_tbl_t* p = *mailbox_list;
		int i = 0;
		
		for (; i < count; i++) {
			EM_SAFE_FREE(p[i].mailbox_name);
			EM_SAFE_FREE(p[i].alias);
		}
		
		EM_SAFE_FREE(p); *mailbox_list = NULL;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

   	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_count_read_mail_uid(int account_id, char *mailbox_name, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%p], count[%p], transaction[%d], err_code[%p]", account_id, mailbox_name , count,  transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !mailbox_name || !count)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_name[%p], count[%p], exist[%p]", account_id, mailbox_name, count);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_read_mail_uid_tbl WHERE account_id = %d AND mailbox_name = '%s'  ", account_id, mailbox_name);
	EM_DEBUG_LOG(">>> SQL [ %s ] ", sql_query_string);

	char **result;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);
 	
	ret = true;
	
FINISH_OFF:
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}



INTERNAL_FUNC int emstorage_check_read_mail_uid(int account_id, char *mailbox_name, char *uid, int *exist, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%p], uid[%p], exist[%p], transaction[%d], err_code[%p]", account_id, mailbox_name , uid, exist, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !uid || !exist)  {	
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_name[%p], uid[%p], exist[%p]", account_id, mailbox_name , uid, exist);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	if (mailbox_name)  {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_read_mail_uid_tbl WHERE account_id = %d AND mailbox_name = '%s' AND s_uid = '%s' ", account_id, mailbox_name, uid);
	}
	else  {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_read_mail_uid_tbl WHERE account_id = %d AND s_uid = '%s' ", account_id, uid);
	}
 	
	char **result;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*exist = atoi(result[1]);
	sqlite3_free_table(result);
 
	if (*exist > 0)
		*exist = 1;		
	else
		*exist = 0;
	
	ret = true;
	
FINISH_OFF:
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_downloaded_mail(int mail_id, emstorage_mail_tbl_t** mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], mail[%p], err_code[%p]", mail_id, mail, err_code);
	
	if (!mail)  {
		EM_DEBUG_EXCEPTION(" mail_id[%d], mail[%p]", mail_id, mail);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false, temp_rule;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_read_mail_uid_tbl WHERE local_uid = %d", mail_id);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	*mail = (emstorage_mail_tbl_t*)malloc(sizeof(emstorage_mail_tbl_t));
	if (*mail == NULL) {
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		EM_DEBUG_EXCEPTION("Memory allocation for mail failed.");
		goto FINISH_OFF;

	}
	memset(*mail, 0x00, sizeof(emstorage_mail_tbl_t));

	_get_stmt_field_data_int(hStmt, &((*mail)->account_id), ACCOUNT_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_get_stmt_field_data_int(hStmt, &((*mail)->mailbox_id), LOCAL_MAILBOX_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_get_stmt_field_data_string(hStmt, &((*mail)->mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_get_stmt_field_data_int(hStmt, &((*mail)->mail_id), LOCAL_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_get_stmt_field_data_string(hStmt, &((*mail)->server_mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_get_stmt_field_data_string(hStmt, &((*mail)->server_mail_id), 0, S_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_get_stmt_field_data_int(hStmt, &((*mail)->mail_size), DATA1_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_get_stmt_field_data_int(hStmt, &temp_rule, FLAG_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	
	(*mail)->server_mail_status = 1;
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("before sqlite3_finalize hStmt = %p", hStmt);
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_downloaded_list(int account_id, int mailbox_id, emstorage_read_mail_uid_tbl_t** read_mail_uid, int *count, int transaction, int *err_code)
{
	EM_PROFILE_BEGIN(emStorageGetDownloadList);
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_id[%d], read_mail_uid[%p], count[%p], transaction[%d], err_code[%p]", account_id, mailbox_id, read_mail_uid, count, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID || !read_mail_uid || !count)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_id[%s], read_mail_uid[%p], count[%p]", account_id, mailbox_id, read_mail_uid, count);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
 
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	emstorage_read_mail_uid_tbl_t* p_data_tbl = NULL;
	int i = 0;
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	if (mailbox_id)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_read_mail_uid_tbl WHERE account_id = %d AND mailbox_id = %d", account_id, mailbox_id);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_read_mail_uid_tbl WHERE account_id = %d", account_id);

	EM_DEBUG_LOG(" sql_query_string : %s", sql_query_string);
 
	
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	char **result;
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, count, NULL, NULL); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	sqlite3_free_table(result);
	if (*count == 0)  {
		EM_DEBUG_LOG("No mail found in mail_read_mail_uid_tbl");
		ret = true;
		goto FINISH_OFF;
	}


	if (!(p_data_tbl = (emstorage_read_mail_uid_tbl_t*)malloc(sizeof(emstorage_read_mail_uid_tbl_t) * *count)))  {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(p_data_tbl, 0x00, sizeof(emstorage_read_mail_uid_tbl_t)* *count);

	for (i = 0; i < *count; ++i)  {
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].account_id), ACCOUNT_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].mailbox_id),LOCAL_MAILBOX_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].local_uid), LOCAL_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].s_uid), 0, S_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].data1), DATA1_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].data2), 0, DATA2_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].flag), FLAG_IDX_IN_MAIL_READ_MAIL_UID_TBL);

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
	}
	
	ret = true;

FINISH_OFF:
	if (ret == true)
		*read_mail_uid = p_data_tbl;
	else if (p_data_tbl)
		emstorage_free_read_mail_uid(&p_data_tbl, *count, NULL);
	
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" Before sqlite3_finalize hStmt = %p", hStmt);
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
 	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(emStorageGetDownloadList);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_downloaded_mail_size(int account_id, char *mailbox_id, int local_uid, char *mailbox_name, char *uid, int *mail_size, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_id[%p], locacal_uid[%d], mailbox_name[%p], uid[%p], mail_size[%p], transaction[%d], err_code[%p]", account_id, mailbox_id, local_uid, mailbox_name, uid, mail_size, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !mail_size)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_id[%p], locacal_uid[%d], mailbox_name[%p], uid[%p], mail_size[%p]", account_id, mailbox_id, local_uid, mailbox_name, uid, mail_size);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	if (mailbox_name) {
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"SELECT IFNULL(MAX(data1), 0) FROM mail_read_mail_uid_tbl "
			"WHERE account_id = %d "
			"AND mailbox_id = '%s' "
			"AND local_uid = %d "
			"AND mailbox_name = '%s' "
			"AND s_uid = '%s'",
			account_id, mailbox_id, local_uid, mailbox_name, uid);
	}
	else {
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"SELECT IFNULL(MAX(data1), 0) FROM mail_read_mail_uid_tbl "
			"WHERE account_id = %d "
			"AND mailbox_id = '%s' "
			"AND local_uid = %d "
			"AND s_uid = '%s'",
			account_id, mailbox_id, local_uid, uid);
	}
	
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION(" no matched mail found....");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;		
	}
 
	_get_stmt_field_data_int(hStmt, mail_size, 0);
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_add_downloaded_mail(emstorage_read_mail_uid_tbl_t* read_mail_uid, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("read_mail_uid[%p], transaction[%d], err_code[%p]", read_mail_uid, transaction, err_code);
	
	if (!read_mail_uid)  {
		EM_DEBUG_EXCEPTION("read_mail_uid[%p]", read_mail_uid);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, rc2,  ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	char *sql = "SELECT max(rowid) FROM mail_read_mail_uid_tbl;";
	char **result = NULL;


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL==result[1]) rc = 1;
	else rc = atoi(result[1])+1;
	sqlite3_free_table(result);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_read_mail_uid_tbl VALUES "
		"( ?"
		", ?"
		", ?"
		", ?"
		", ?"
		", ?"
		", ?"
		", ?"
		", ? )");
	
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc2);
	if (rc2 != SQLITE_OK)  {
		EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);
		EM_DEBUG_EXCEPTION("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle));
		
		error = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("account_id VALUE [%d] ", read_mail_uid->account_id);
	EM_DEBUG_LOG("mailbox_id VALUE [%d] ", read_mail_uid->mailbox_id);
	EM_DEBUG_LOG("local_uid VALUE [%d] ", read_mail_uid->local_uid);
	EM_DEBUG_LOG("mailbox_name VALUE [%s] ", read_mail_uid->mailbox_name);
	EM_DEBUG_LOG("s_uid VALUE [%s] ", read_mail_uid->s_uid);
	EM_DEBUG_LOG("data1 VALUE [%d] ", read_mail_uid->data1);
	EM_DEBUG_LOG("data2 VALUE [%s] ", read_mail_uid->data2);
	EM_DEBUG_LOG("flag VALUE [%d] ", read_mail_uid->flag);
	EM_DEBUG_LOG("rc VALUE [%d] ", rc);

	_bind_stmt_field_data_int(hStmt, ACCOUNT_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->account_id);
	_bind_stmt_field_data_int(hStmt, LOCAL_MAILBOX_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->mailbox_id);
	_bind_stmt_field_data_int(hStmt, LOCAL_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->local_uid);
	_bind_stmt_field_data_string(hStmt, MAILBOX_NAME_IDX_IN_MAIL_READ_MAIL_UID_TBL, (char *)read_mail_uid->mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bind_stmt_field_data_string(hStmt, S_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL, (char *)read_mail_uid->s_uid, 0, S_UID_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bind_stmt_field_data_int(hStmt, DATA1_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->data1);
	_bind_stmt_field_data_string(hStmt, DATA2_IDX_IN_MAIL_READ_MAIL_UID_TBL, (char *)read_mail_uid->data2, 0, DATA2_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bind_stmt_field_data_int(hStmt, FLAG_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->flag);
	_bind_stmt_field_data_int(hStmt, IDX_NUM_IDX_IN_MAIL_READ_MAIL_UID_TBL, rc);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail[%d] [%s]", rc, sqlite3_errmsg(local_db_handle)));

	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
 
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_change_read_mail_uid(int account_id, int mailbox_id, int local_uid, char *mailbox_name, char *uid, emstorage_read_mail_uid_tbl_t* read_mail_uid, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_id[%d], local_uid[%d], mailbox_name[%p], uid[%p], read_mail_uid[%p], transaction[%d], err_code[%p]", account_id, mailbox_id, local_uid, mailbox_name, uid, read_mail_uid, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !read_mail_uid)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_id[%d], local_uid[%d], mailbox_name[%p], uid[%p], read_mail_uid[%p]", account_id, mailbox_id, local_uid, mailbox_name, uid, read_mail_uid);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_read_mail_uid_tbl SET"
		"  account_id = ?"
		", mailbox_id = ?"
		", local_uid  = ?"
		", mailbox_name = ?"
		", s_uid = ?"
		", data1 = ?"
		", data2 = ?"
		", flag  = ?"
		" WHERE account_id = ?"
		" AND mailbox_id  = ?"
		" AND local_uid   = ?"
		" AND mailbox_name= ?"
		" AND s_uid = ?");

	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
 
	
	int i = 0;
	
	_bind_stmt_field_data_int(hStmt, i++, read_mail_uid->account_id);
	_bind_stmt_field_data_int(hStmt, i++, read_mail_uid->mailbox_id);
	_bind_stmt_field_data_int(hStmt, i++, read_mail_uid->local_uid);
	_bind_stmt_field_data_string(hStmt, i++, (char*)read_mail_uid->mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char*)read_mail_uid->s_uid, 0, S_UID_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bind_stmt_field_data_int(hStmt, i++, read_mail_uid->data1);
	_bind_stmt_field_data_string(hStmt, i++, (char*)read_mail_uid->data2, 0, DATA2_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bind_stmt_field_data_int(hStmt, i++, read_mail_uid->flag);
	_bind_stmt_field_data_int(hStmt, i++, account_id);
	_bind_stmt_field_data_int(hStmt, i++, mailbox_id);
	_bind_stmt_field_data_int(hStmt, i++, local_uid);
	_bind_stmt_field_data_string(hStmt, i++, (char*)mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char*)uid, 0, S_UID_LEN_IN_MAIL_READ_MAIL_UID_TBL);
 

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
 
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_remove_downloaded_mail(int account_id, char *mailbox_name, char *uid, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%s], uid[%s], transaction[%d], err_code[%p]", account_id, mailbox_name, uid, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_name[%s], uid[%s]", account_id, mailbox_name, uid);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
 	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_read_mail_uid_tbl WHERE account_id = %d ", account_id);
	
	if (mailbox_name) {		/*  NULL means all mailbox_name */
		SNPRINTF(sql_query_string+strlen(sql_query_string), sizeof(sql_query_string) - (1 + strlen(sql_query_string)), "AND mailbox_name = '%s' ", mailbox_name);
	}
	
	if (uid) {		/*  NULL means all mail */
		SNPRINTF(sql_query_string+strlen(sql_query_string), sizeof(sql_query_string) - (1 + strlen(sql_query_string)), "AND s_uid='%s' ", uid);
	}
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
 
	
	ret = true;
	
FINISH_OFF:
 
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_free_read_mail_uid(emstorage_read_mail_uid_tbl_t** read_mail_uid, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("read_mail_uid[%p], count[%d], err_code[%p]", read_mail_uid, count, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	if (count > 0)  {
		if (!read_mail_uid || !*read_mail_uid)  {	
			EM_DEBUG_EXCEPTION(" read_mail_uid[%p], count[%d]", read_mail_uid, count);
			
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		emstorage_read_mail_uid_tbl_t* p = *read_mail_uid;
		int i;
		
		for (i = 0; i < count; i++)  {
			EM_SAFE_FREE(p[i].mailbox_name);
			EM_SAFE_FREE(p[i].s_uid);
			EM_SAFE_FREE(p[i].data2);
		}
		
		EM_SAFE_FREE(p); *read_mail_uid = NULL;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_rule_count(int account_id, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], count[%p], transaction[%d], err_code[%p]", account_id, count, transaction, err_code);
	
	if (account_id != ALL_ACCOUNT || !count) {		/*  only global rule supported. */
		EM_DEBUG_EXCEPTION(" account_id[%d], count[%p]", account_id, count);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error =  EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_rule_tbl WHERE account_id = %d", account_id);
	
	char **result;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);
	
	ret = true;
	
FINISH_OFF:
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_rule(int account_id, int type, int start_idx, int *select_num, int *is_completed, emstorage_rule_tbl_t** rule_list, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], type[%d], start_idx[%d], select_num[%p], is_completed[%p], rule_list[%p], transaction[%d], err_code[%p]", account_id, type, start_idx, select_num, is_completed, rule_list, transaction, err_code);
	
	if (account_id != ALL_ACCOUNT || !select_num || !is_completed || !rule_list) {		/*  only global rule supported. */
		EM_DEBUG_EXCEPTION(" account_id[%d], type[%d], start_idx[%d], select_num[%p], is_completed[%p], rule_list[%p]", account_id, type, start_idx, select_num, is_completed, rule_list);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	emstorage_rule_tbl_t* p_data_tbl = NULL;
	int i = 0, count = 0;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	int rc;
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	if (type)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_rule_tbl WHERE account_id = %d AND type = %d", account_id, type);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_rule_tbl WHERE account_id = %d ORDER BY type", account_id);
		
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	char **result;
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	sqlite3_free_table(result);

	if (count == 0)  {
		EM_DEBUG_LOG(" no matched rule found...");
		ret = true;
		goto FINISH_OFF;
	}

	
	if (!(p_data_tbl = (emstorage_rule_tbl_t*)malloc(sizeof(emstorage_rule_tbl_t) * count))) {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emstorage_rule_tbl_t) * count);
	
	for (i = 0; i < count; i++)  {
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].account_id), ACCOUNT_ID_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].rule_id), RULE_ID_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].type), TYPE_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].value), 0, VALUE_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].action_type), ACTION_TYPE_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].target_mailbox_id), TARGET_MAILBOX_ID_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].flag1), FLAG1_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].flag2), FLAG2_IDX_IN_MAIL_RULE_TBL);

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
	}
	
	ret = true;
	
FINISH_OFF:

	EM_DEBUG_LOG("[%d] rules found.", count);

	if (ret == true)  {
		*rule_list = p_data_tbl;
		*select_num = count;
	}
	else if (p_data_tbl != NULL)
		emstorage_free_rule(&p_data_tbl, count, NULL); /* CID FIX */
 	
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("  sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_rule_by_id(int account_id, int rule_id, emstorage_rule_tbl_t** rule, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], rule_id[%d], rule[%p], transaction[%d], err_code[%p]", account_id, rule_id, rule, transaction, err_code);

	if (account_id != ALL_ACCOUNT || !rule)  {	
		EM_DEBUG_EXCEPTION(" account_id[%d], rule_id[%d], rule[%p]", account_id, rule_id, rule);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	emstorage_rule_tbl_t* p_data_tbl = NULL;
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
 
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_rule_tbl WHERE account_id = %d AND rule_id = %d", account_id, rule_id);
	
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION(" no matched rule found...");
		error = EMAIL_ERROR_FILTER_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	if (!(p_data_tbl = (emstorage_rule_tbl_t*)malloc(sizeof(emstorage_rule_tbl_t))))  {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emstorage_rule_tbl_t));
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), ACCOUNT_ID_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->rule_id), RULE_ID_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->type), TYPE_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->value), 0, VALUE_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->action_type), ACTION_TYPE_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->target_mailbox_id), TARGET_MAILBOX_ID_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->flag1), FLAG1_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->flag2), FLAG2_IDX_IN_MAIL_RULE_TBL);
	
	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*rule = p_data_tbl;
	else if (p_data_tbl != NULL)
		emstorage_free_rule(&p_data_tbl, 1, NULL);
	
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
 
	
	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_change_rule(int account_id, int rule_id, emstorage_rule_tbl_t* new_rule, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], rule_id[%d], new_rule[%p], transaction[%d], err_code[%p]", account_id, rule_id, new_rule, transaction, err_code);

	if (account_id != ALL_ACCOUNT || !new_rule) {		/*  only global rule supported. */
		EM_DEBUG_EXCEPTION(" account_id[%d], rule_id[%d], new_rule[%p]", account_id, rule_id, new_rule);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
 
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_rule_tbl SET"
		"  type = ?"
		", value = ?"
		", action_type = ?"
		", target_mailbox_id = ?"
		", flag1 = ?"
		", flag2 = ?"
		" WHERE account_id = ?"
		" AND rule_id = ?");
 
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG(" Before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	int i = 0;
	
	_bind_stmt_field_data_int(hStmt, i++, new_rule->type);
	_bind_stmt_field_data_string(hStmt, i++, (char *)new_rule->value, 0, VALUE_LEN_IN_MAIL_RULE_TBL);
	_bind_stmt_field_data_int(hStmt, i++, new_rule->action_type);
	_bind_stmt_field_data_int(hStmt, i++, new_rule->target_mailbox_id);
	_bind_stmt_field_data_int(hStmt, i++, new_rule->flag1);
	_bind_stmt_field_data_int(hStmt, i++, new_rule->flag2);
	_bind_stmt_field_data_int(hStmt, i++, account_id);
	_bind_stmt_field_data_int(hStmt, i++, rule_id);
 

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_find_rule(emstorage_rule_tbl_t* rule, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("rule[%p], transaction[%d], err_code[%p]", rule, transaction, err_code);

	if (!rule || rule->account_id != ALL_ACCOUNT) {		/*  only global rule supported. */
		if (rule != NULL)
			EM_DEBUG_EXCEPTION(" rule->account_id[%d]", rule->account_id);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	int error = EMAIL_ERROR_NONE;
	int rc, ret = false;

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"SELECT rule_id FROM mail_rule_tbl WHERE type = %d AND UPPER(value) = UPPER('%s')",
		rule->type, rule->value);
 
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION(" no matched rule found...");
		error = EMAIL_ERROR_FILTER_NOT_FOUND;
		goto FINISH_OFF;
	}

	
	ret = true;

FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
 
	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;

	if (err_code)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_add_rule(emstorage_rule_tbl_t* rule, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("rule[%p], transaction[%d], err_code[%p]", rule, transaction, err_code);

	if (!rule || rule->account_id != ALL_ACCOUNT)  {	/*  only global rule supported. */
		if (rule != NULL)
			EM_DEBUG_EXCEPTION(" rule->account_id[%d]", rule->account_id);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, rc_2, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
 	char sql_query_string[QUERY_SIZE] = {0, };

	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	char *sql;
	char **result;
	sql = "SELECT max(rowid) FROM mail_rule_tbl;";

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
	
	if (NULL==result[1])
		rc = 1;
	else 
		rc = atoi(result[1])+1;
	sqlite3_free_table(result);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_rule_tbl VALUES "
		"( ?"		/*  account id */
		", ?"		/*  rule_id */
		", ?"		/*  type */
		", ?"		/*  value */
		", ?"		/*  action_type */
		", ?"		/*  target_mailbox_id */
		", ?"		/*  flag1 */
		", ?)");	/*  flag2 */
 
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc_2);
	if (rc_2 != SQLITE_OK)  {
		EM_DEBUG_EXCEPTION("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc_2, sqlite3_errmsg(local_db_handle));
		error = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}
 
	_bind_stmt_field_data_int(hStmt, ACCOUNT_ID_IDX_IN_MAIL_RULE_TBL, rule->account_id);
	_bind_stmt_field_data_int(hStmt, RULE_ID_IDX_IN_MAIL_RULE_TBL, rc);
	_bind_stmt_field_data_int(hStmt, TYPE_IDX_IN_MAIL_RULE_TBL, rule->type);
	_bind_stmt_field_data_string(hStmt, VALUE_IDX_IN_MAIL_RULE_TBL, (char*)rule->value, 0, VALUE_LEN_IN_MAIL_RULE_TBL);
	_bind_stmt_field_data_int(hStmt, ACTION_TYPE_IDX_IN_MAIL_RULE_TBL, rule->action_type);
	_bind_stmt_field_data_int(hStmt, TARGET_MAILBOX_ID_IDX_IN_MAIL_RULE_TBL, rule->target_mailbox_id);
	_bind_stmt_field_data_int(hStmt, FLAG1_IDX_IN_MAIL_RULE_TBL, rule->flag1);
	_bind_stmt_field_data_int(hStmt, FLAG2_IDX_IN_MAIL_RULE_TBL, rule->flag2);
 
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_rule(int account_id, int rule_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], rule_id[%d], transaction[%d], err_code[%p]", account_id, rule_id, transaction, err_code);
	
	if (account_id != ALL_ACCOUNT) {		/*  only global rule supported. */
		EM_DEBUG_EXCEPTION(" account_id[%d], rule_id[%d]", account_id, rule_id);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_rule_tbl WHERE account_id = %d AND rule_id = %d", account_id, rule_id);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION(" no matched rule found...");
		
		error = EMAIL_ERROR_FILTER_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_free_rule(emstorage_rule_tbl_t** rule_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("rule_list[%p], conut[%d], err_code[%p]", rule_list, count, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	if (count > 0) {
		if (!rule_list || !*rule_list) {
			EM_DEBUG_EXCEPTION(" rule_list[%p], conut[%d]", rule_list, count);
			
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		emstorage_rule_tbl_t* p = *rule_list;
		int i = 0;
		
		for (; i < count; i++) {
			EM_SAFE_FREE(p[i].value);
		}
		
		EM_SAFE_FREE(p); *rule_list = NULL;
	}
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mail_count(int account_id, const char *mailbox_name, int *total, int *unseen, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%s], total[%p], unseen[%p], transaction[%d], err_code[%p]", account_id, mailbox_name, total, unseen, transaction, err_code);
	
	if (!total && !unseen) {
		EM_DEBUG_EXCEPTION(" accoun_id[%d], mailbox_name[%s], total[%p], unseen[%p]", account_id, mailbox_name, total, unseen);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char err_msg[1024];

	
	memset(&sql_query_string, 0x00, sizeof(sql_query_string));
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	if (total)  {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_tbl");
		
		if (account_id != ALL_ACCOUNT)  {
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " WHERE account_id = %d", account_id);
			if (mailbox_name)
				SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " AND UPPER(mailbox_name) =  UPPER('%s')", mailbox_name);
		}
		else if (mailbox_name)
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " WHERE UPPER(mailbox_name) =  UPPER('%s')", mailbox_name);
		
#ifdef USE_GET_RECORD_COUNT_API
	 	char **result;

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF2; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		*total = atoi(result[1]);
		sqlite3_free_table(result);
#else
		
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
		EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF2; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF2; },
			("sqlite3_step fail:%d", rc));
		_get_stmt_field_data_int(hStmt, total, 0);
#endif		/*  USE_GET_RECORD_COUNT_API */
	}
	
	if (unseen)  {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_tbl WHERE flags_seen_field = 0");		/*  fSEEN = 0x01 */
		
		if (account_id != ALL_ACCOUNT)
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " AND account_id = %d", account_id);
		
		if (mailbox_name)
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " AND mailbox_name = '%s'", mailbox_name);
		else
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " AND mailbox_type NOT IN (3, 5)");
		
 		char **result;
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		*unseen = atoi(result[1]);
		sqlite3_free_table(result);
 		
	}
FINISH_OFF:
	ret = true;
	
FINISH_OFF2:
 
#ifndef USE_PREPARED_QUERY_
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" EDBStmtClearRow failed - %d: %s", rc, err_msg);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
#endif

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mail_field_by_id(int mail_id, int type, emstorage_mail_tbl_t** mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], type[%d], mail[%p], transaction[%d], err_code[%p]", mail_id, type, mail, transaction, err_code);
	
	if (mail_id <= 0 || !mail)  {	
		EM_DEBUG_EXCEPTION("mail_id[%d], mail[%p]", mail_id, mail);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	emstorage_mail_tbl_t* p_data_tbl = (emstorage_mail_tbl_t*)malloc(sizeof(emstorage_mail_tbl_t));

	if (p_data_tbl == NULL)  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_OUT_OF_MEMORY;
		return false;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emstorage_mail_tbl_t));
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	switch (type)  {
		case RETRIEVE_SUMMARY:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"SELECT account_id, mail_id, mailbox_id, mailbox_name, server_mail_status, server_mailbox_name, server_mail_id, file_path_plain, file_path_html, flags_seen_field, save_status, lock_status, thread_id, thread_item_count FROM mail_tbl WHERE mail_id = %d", mail_id);
			break;
		
		case RETRIEVE_FIELDS_FOR_DELETE:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"SELECT account_id, mail_id, server_mail_status, server_mailbox_name, server_mail_id FROM mail_tbl WHERE mail_id = %d", mail_id);
			break;			
		
		case RETRIEVE_ACCOUNT:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"SELECT account_id FROM mail_tbl WHERE mail_id = %d", mail_id);
			break;
		
		case RETRIEVE_FLAG:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"SELECT account_id, mailbox_name, flags_seen_field, thread_id, mailbox_id FROM mail_tbl WHERE mail_id = %d", mail_id);
			break;
		
		default :
			EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM : type [%d]", type);
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}
	
	EM_DEBUG_LOG("Query [%s]", sql_query_string);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no matched mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
	switch (type)  {
		case RETRIEVE_SUMMARY:
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), 0);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->mail_id), 1);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->mailbox_id), 2);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->mailbox_name), 0, 3);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->server_mail_status), 4);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mailbox_name), 0, 5);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mail_id), 0, 6);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->file_path_plain), 0, 7);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->file_path_html), 0, 8);
			_get_stmt_field_data_char(hStmt, &(p_data_tbl->flags_seen_field), 9);
			_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->save_status), 10);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->lock_status), 11);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->thread_id), 12);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->thread_item_count), 13);
			break;
			
		case RETRIEVE_FIELDS_FOR_DELETE:
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), 0);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->mail_id), 1);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->server_mail_status), 2);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mailbox_name), 0, 3);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mail_id), 0, 4);
			break;
			
		case RETRIEVE_ACCOUNT:
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), 0);
			break;
			
		case RETRIEVE_FLAG:
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), 0);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->mailbox_name), 0, 1);
			_get_stmt_field_data_char(hStmt, &(p_data_tbl->flags_seen_field), 2);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->thread_id), 3);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->mailbox_id), 4);
			break;
	}
	
	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*mail = p_data_tbl;
	else if (p_data_tbl != NULL)
		emstorage_free_mail(&p_data_tbl,  1, NULL);

	if (hStmt != NULL)  {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);
	
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
 	

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mail_field_by_multiple_mail_id(int mail_ids[], int number_of_mails, int type, emstorage_mail_tbl_t** mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], number_of_mails [%d], type[%d], mail[%p], transaction[%d], err_code[%p]", mail_ids, number_of_mails, type, mail, transaction, err_code);
	
	if (number_of_mails <= 0 || !mail_ids)  {	
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	emstorage_mail_tbl_t* p_data_tbl = (emstorage_mail_tbl_t*)em_malloc(sizeof(emstorage_mail_tbl_t) * number_of_mails);

	if (p_data_tbl == NULL)  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_OUT_OF_MEMORY;
		return false;
	}
	
	DB_STMT hStmt = NULL;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int i = 0, item_count = 0, rc = -1, field_count, col_index, cur_sql_query_string = 0;
	char **result = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	switch (type) {
		case RETRIEVE_SUMMARY:
			cur_sql_query_string = SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"SELECT account_id, mail_id, mailbox_name, server_mail_status, server_mailbox_name, server_mail_id, file_path_plain, file_path_html, flags_seen_field, save_status, lock_status, thread_id, thread_item_count FROM mail_tbl WHERE mail_id in (");
			field_count = 13;
			break;
		
		case RETRIEVE_FIELDS_FOR_DELETE:
			cur_sql_query_string = SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"SELECT account_id, mail_id, server_mail_status, server_mailbox_name, server_mail_id FROM mail_tbl WHERE mail_id in (");
			field_count = 5;
			break;			
		
		case RETRIEVE_ACCOUNT:
			cur_sql_query_string = SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"SELECT account_id FROM mail_tbl WHERE mail_id in (");
			field_count = 1;
			break;
		
		case RETRIEVE_FLAG:
			cur_sql_query_string = SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"SELECT account_id, mailbox_id, flags_seen_field, thread_id FROM mail_tbl WHERE mail_id in (");
			field_count = 4;
			break;
		
		default :
			EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM : type [%d]", type);
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}

	for(i = 0; i < number_of_mails; i++) 		
		cur_sql_query_string += SNPRINTF_OFFSET(sql_query_string, cur_sql_query_string, QUERY_SIZE, "%d,", mail_ids[i]);
	sql_query_string[strlen(sql_query_string) - 1] = ')';
	
	EM_DEBUG_LOG("Query [%s], Length [%d]", sql_query_string, strlen(sql_query_string));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &item_count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no matched mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("item_count [%d]", item_count);

	if(number_of_mails != item_count) {
		EM_DEBUG_EXCEPTION("Can't find all emails");
		/* error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF; */ /* removed temporarily */
	}

	col_index = field_count;

	for(i = 0; i < item_count; i++)	{
		switch (type) {
			case RETRIEVE_SUMMARY:
				_get_table_field_data_int(result, &(p_data_tbl[i].account_id), col_index++);
				_get_table_field_data_int(result, &(p_data_tbl[i].mail_id), col_index++);
				_get_table_field_data_string(result, &(p_data_tbl[i].mailbox_name), 0, col_index++);
				_get_table_field_data_int(result, &(p_data_tbl[i].server_mail_status), col_index++);
				_get_table_field_data_string(result, &(p_data_tbl[i].server_mailbox_name), 0, col_index++);
				_get_table_field_data_string(result, &(p_data_tbl[i].server_mail_id), 0, col_index++);
				_get_table_field_data_string(result, &(p_data_tbl[i].file_path_plain), 0, col_index++);
				_get_table_field_data_string(result, &(p_data_tbl[i].file_path_html), 0, col_index++);
				_get_table_field_data_char(result, &(p_data_tbl[i].flags_seen_field), col_index++);
				_get_table_field_data_int(result, (int*)&(p_data_tbl[i].save_status), col_index++);
				_get_table_field_data_int(result, &(p_data_tbl[i].lock_status), col_index++);
				_get_table_field_data_int(result, &(p_data_tbl[i].thread_id), col_index++);
				_get_table_field_data_int(result, &(p_data_tbl[i].thread_item_count), col_index++);
				break;
				
			case RETRIEVE_FIELDS_FOR_DELETE:
				_get_table_field_data_int(result, &(p_data_tbl[i].account_id), col_index++);
				_get_table_field_data_int(result, &(p_data_tbl[i].mail_id), col_index++);
				_get_table_field_data_int(result, &(p_data_tbl[i].server_mail_status), col_index++);
				_get_table_field_data_string(result, &(p_data_tbl[i].server_mailbox_name), 0, col_index++);
				_get_table_field_data_string(result, &(p_data_tbl[i].server_mail_id), 0, col_index++);
				break;
				
			case RETRIEVE_ACCOUNT:
				_get_table_field_data_int(result, &(p_data_tbl[i].account_id), col_index++);
				break;
				
			case RETRIEVE_FLAG:
				_get_table_field_data_int(result, &(p_data_tbl[i].account_id), col_index++);
				_get_table_field_data_int(result, &(p_data_tbl[i].mailbox_id), col_index++);
				_get_table_field_data_char(result, &(p_data_tbl[i].flags_seen_field), col_index++);
				_get_table_field_data_int(result, &(p_data_tbl[i].thread_id), col_index++);
				break;
		}
	}

	sqlite3_free_table(result);

	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*mail = p_data_tbl;
	else if (p_data_tbl != NULL)
		emstorage_free_mail(&p_data_tbl,  1, NULL);

	if (hStmt != NULL) {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);
	
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mail_by_id(int mail_id, emstorage_mail_tbl_t** mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], mail[%p], transaction[%d], err_code[%p]", mail_id, mail, transaction, err_code);
	
	if (mail_id <= 0 || !mail) {	
		EM_DEBUG_EXCEPTION("mail_id[%d], mail[%p]", mail_id, mail);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false, error = EMAIL_ERROR_NONE, count;
	char conditional_clause[QUERY_SIZE] = {0, };
	emstorage_mail_tbl_t* p_data_tbl = NULL;
	
	SNPRINTF(conditional_clause, QUERY_SIZE, "WHERE mail_id = %d", mail_id);
	EM_DEBUG_LOG("query = [%s]", conditional_clause);

	if(!emstorage_query_mail_tbl(conditional_clause, transaction, &p_data_tbl, &count, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_tbl [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*mail = p_data_tbl;
	else if (p_data_tbl != NULL)
		emstorage_free_mail(&p_data_tbl, 1, &error);
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_mail_search_start(emstorage_search_filter_t* search, int account_id, char *mailbox_name, int sorting, int *search_handle, int *searched, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("search[%p], account_id[%d], mailbox_name[%p], sorting[%d], search_handle[%p], searched[%p], transaction[%d], err_code[%p]", search, account_id, mailbox_name, sorting, search_handle, searched, transaction, err_code);

	if (!search_handle || !searched)  {	
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		EM_DEBUG_FUNC_END("false");
		return false;
	}
	
	emstorage_search_filter_t* p = search;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	int rc, ret = false;
	int and = false, mail_count = 0;
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_tbl");
	
	if (account_id != ALL_ACCOUNT)  {
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " WHERE account_id = %d", account_id);
		and = true;
	}
	
	if (mailbox_name)  {
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " %s mailbox_name = '%s'", and ? "AND" : "WHERE", mailbox_name);
		and = true;
	}
	
	while (p)  {

		if (p->key_type) {
			if (!strncmp(p->key_type, "subject", strlen("subject"))) {
				SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " %s subject LIKE '%%%%%s%%%%'", and ? "AND" : "WHERE", p->key_value);
				and = true;
			}
			else if (!strncmp(p->key_type, "full_address_from", strlen("full_address_from"))) {
				SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " %s full_address_from LIKE '%%%%%s%%%%'", and ? "AND" : "WHERE", p->key_value);
				and = true;
			}
			else if (!strncmp(p->key_type, "full_address_to", strlen("full_address_to"))) {
				SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " %s full_address_to LIKE '%%%%%s%%%%'", and ? "AND" : "WHERE", p->key_value);
				and = true;
			}
			else if (!strncmp(p->key_type, "email_address", strlen("email_address"))) {
				SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " %s email_address_sender = '%s' OR email_address_recipient = '%s'", and ? "AND" : "WHERE", p->key_value, p->key_value);
				and = true;
			}
			p = p->next;
		}
	}
	
	if (sorting)
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " ORDER BY date_time");

	EM_DEBUG_LOG("sql_query_string [%s]", sql_query_string);

	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	char **result;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &mail_count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	sqlite3_free_table(result);
	
	ret = true;
	
FINISH_OFF:
	if (ret == true)  {
		*search_handle = (int)hStmt;
		*searched = mail_count;
		EM_DEBUG_LOG("mail_count [%d]", mail_count);
	}
	else  {
		if (hStmt != NULL)  {
			rc = sqlite3_finalize(hStmt);
			if (rc != SQLITE_OK)  {
				EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
				error = EMAIL_ERROR_DB_FAILURE;
			}
		}
		
		EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
		_DISCONNECT_DB;
	}
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_mail_search_result(int search_handle, emstorage_mail_field_type_t type, void** data, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("search_handle[%d], type[%d], data[%p], transaction[%d], err_code[%p]", search_handle, type, data, transaction, err_code);

	if (search_handle < 0 || !data) {
		EM_DEBUG_EXCEPTION(" search_handle[%d], type[%d], data[%p]", search_handle, type, data);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	emstorage_mail_tbl_t* p_data_tbl = NULL;
	DB_STMT hStmt = (DB_STMT)search_handle;
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;

	switch (type)  {
		case RETRIEVE_ID:
			_get_stmt_field_data_int(hStmt, (int *)data, MAIL_ID_IDX_IN_MAIL_TBL);
			break;
			
		case RETRIEVE_ENVELOPE:
		case RETRIEVE_ALL:
			if (!(p_data_tbl = em_malloc(sizeof(emstorage_mail_tbl_t)))) {
				EM_DEBUG_EXCEPTION(" em_malloc failed...");
				
				error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			_get_stmt_field_data_int   (hStmt, &(p_data_tbl->mail_id), MAIL_ID_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int   (hStmt, &(p_data_tbl->account_id), ACCOUNT_ID_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int   (hStmt, &(p_data_tbl->mail_size), MAIL_SIZE_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mail_id), 0, SERVER_MAIL_ID_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->full_address_from), 1, FULL_ADDRESS_FROM_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->full_address_to), 1, FULL_ADDRESS_TO_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->subject), 1, SUBJECT_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int   (hStmt, &(p_data_tbl->body_download_status), BODY_DOWNLOAD_STATUS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->file_path_plain), 0, FILE_PATH_PLAIN_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->file_path_html), 0, FILE_PATH_HTML_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->file_path_mime_entity), 0, FILE_PATH_HTML_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_time_t(hStmt, &(p_data_tbl->date_time), DATETIME_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_char  (hStmt, &(p_data_tbl->flags_seen_field), FLAGS_SEEN_FIELD_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int   (hStmt, &(p_data_tbl->DRM_status), DRM_STATUS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int   (hStmt, (int*)&(p_data_tbl->priority), PRIORITY_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int   (hStmt, (int*)&(p_data_tbl->save_status), SAVE_STATUS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int   (hStmt, &(p_data_tbl->lock_status), LOCK_STATUS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int   (hStmt, (int*)&(p_data_tbl->report_status), REPORT_STATUS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->preview_text), 1, PREVIEW_TEXT_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int   (hStmt, (int*)&(p_data_tbl->meeting_request_status), MEETING_REQUEST_STATUS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int   (hStmt, (int*)&(p_data_tbl->message_class), MESSAGE_CLASS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int   (hStmt, (int*)&(p_data_tbl->digest_type), DIGEST_TYPE_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int   (hStmt, (int*)&(p_data_tbl->smime_type), SMIME_TYPE_IDX_IN_MAIL_TBL);
				
			if (type == RETRIEVE_ALL)  {
				_get_stmt_field_data_int   (hStmt, &(p_data_tbl->server_mail_status), SERVER_MAIL_STATUS_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mailbox_name), 0, SERVER_MAILBOX_NAME_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->full_address_reply), 1, FULL_ADDRESS_REPLY_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->full_address_cc), 1, FULL_ADDRESS_CC_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->full_address_bcc), 1, FULL_ADDRESS_BCC_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->full_address_return), 1, FULL_ADDRESS_RETURN_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->message_id), 0, MESSAGE_ID_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->email_address_sender), 1, EMAIL_ADDRESS_SENDER_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->email_address_recipient), 1, EMAIL_ADDRESS_RECIPIENT_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_int   (hStmt, &(p_data_tbl->attachment_count), ATTACHMENT_COUNT_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->preview_text), 1, PREVIEW_TEXT_IDX_IN_MAIL_TBL);
				
			}
			
			if (p_data_tbl->body_download_status)  {
				struct stat buf;
				
				if (p_data_tbl->file_path_html)  {
					if (stat(p_data_tbl->file_path_html, &buf) == -1)
						p_data_tbl->body_download_status = 0;
				}
				else if (p_data_tbl->file_path_plain)  {
					if (stat(p_data_tbl->file_path_plain, &buf) == -1)
						p_data_tbl->body_download_status = 0;
				}
				else 
					p_data_tbl->body_download_status = 0;
			}
			
			*((emstorage_mail_tbl_t**)data) = p_data_tbl;
			break;
		
		case RETRIEVE_SUMMARY:
			if (!(p_data_tbl = malloc(sizeof(emstorage_mail_tbl_t))))  {
				EM_DEBUG_EXCEPTION(" malloc failed...");
				
				error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			memset(p_data_tbl, 0x00, sizeof(emstorage_mail_tbl_t));
			
			_get_stmt_field_data_int   (hStmt, &(p_data_tbl->mail_id), MAIL_ID_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int   (hStmt, &(p_data_tbl->account_id), ACCOUNT_ID_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int   (hStmt, &(p_data_tbl->server_mail_status), SERVER_MAIL_STATUS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mailbox_name), 0, SERVER_MAILBOX_NAME_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mail_id), 0, SERVER_MAIL_ID_IDX_IN_MAIL_TBL);
			
			*((emstorage_mail_tbl_t**)data) = p_data_tbl;
			break;
		
		case RETRIEVE_ADDRESS:
			if (!(p_data_tbl = malloc(sizeof(emstorage_mail_tbl_t))))  {
				EM_DEBUG_EXCEPTION(" malloc failed...");
				error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			memset(p_data_tbl, 0x00, sizeof(emstorage_mail_tbl_t));
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->mail_id), MAIL_ID_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->email_address_sender), 1, EMAIL_ADDRESS_SENDER_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->email_address_recipient), 1, EMAIL_ADDRESS_RECIPIENT_IDX_IN_MAIL_TBL);
			*((emstorage_mail_tbl_t**)data) = p_data_tbl;
			break;
		
		default:
			break;
	}
	

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	ret = true;
	
FINISH_OFF:
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_mail_search_end(int search_handle, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("search_handle[%d], transaction[%d], err_code[%p]", search_handle, transaction, err_code);
	
	int error = EMAIL_ERROR_NONE;
	int rc, ret = false;
	
	if (search_handle < 0)  {
		EM_DEBUG_EXCEPTION(" search_handle[%d]", search_handle);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	DB_STMT hStmt = (DB_STMT)search_handle;
	
	EM_DEBUG_LOG(" Before sqlite3_finalize hStmt = %p", hStmt);

	rc = sqlite3_finalize(hStmt);
	if (rc != SQLITE_OK)  {
		EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
		error = EMAIL_ERROR_DB_FAILURE;
	}
 
	ret = true;
	
FINISH_OFF:
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_change_mail(int mail_id, emstorage_mail_tbl_t* mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], mail[%p], transaction[%d], err_code[%p]", mail_id, mail, transaction, err_code);

	if (mail_id <= 0 || !mail) {
		EM_DEBUG_EXCEPTION(" mail_id[%d], mail[%p]", mail_id, mail);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	int rc = -1;				
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int i = 0;
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	char mailbox_id_param_string[10] = {0,};

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_tbl SET"
		"  mail_id = ?"
		", account_id = ?"
		", mailbox_id = ?"
		", mailbox_name = ?"
		", mail_size = ?"
		", server_mail_status = ?"
		", server_mailbox_name = ?"
		", server_mail_id = ?"
		", reference_mail_id = ?"
		", full_address_from = ?"
		", full_address_reply = ?"  /* 10 */
		", full_address_to = ?"
		", full_address_cc = ?"
		", full_address_bcc = ?"
		", full_address_return = ?"
		", subject = ?"
		", body_download_status = ?"
		", file_path_plain = ?"
		", file_path_html = ?"
		", date_time = ?"
		", flags_seen_field      = ?"  
		", flags_deleted_field   = ?"  
		", flags_flagged_field   = ?"  
		", flags_answered_field  = ?"  
		", flags_recent_field    = ?"  
		", flags_draft_field     = ?"  
		", flags_forwarded_field = ?"  
		", DRM_status = ?"
		", priority = ?"
		", save_status = ?"
		", lock_status = ?"
		", message_id = ?"
		", report_status = ?"
		", preview_text = ?"
		", smime_type = ?"
		" WHERE mail_id = %d AND account_id != 0 "
		, mail_id);

	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	_bind_stmt_field_data_int   (hStmt, i++, mail->mail_id);
	_bind_stmt_field_data_int   (hStmt, i++, mail->account_id);
	_bind_stmt_field_data_int   (hStmt, i++, mail->mailbox_id);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->mailbox_name, 0, MAILBOX_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int   (hStmt, i++, mail->mail_size);
	_bind_stmt_field_data_int   (hStmt, i++, mail->server_mail_status);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->server_mailbox_name, 0, SERVER_MAILBOX_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->server_mail_id, 0, SERVER_MAIL_ID_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int   (hStmt, i++, mail->reference_mail_id);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->full_address_from, 1, FROM_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->full_address_reply, 1, REPLY_TO_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->full_address_to, 1, TO_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->full_address_cc, 1, CC_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->full_address_bcc, 1, BCC_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->full_address_return, 1, RETURN_PATH_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->subject, 1, SUBJECT_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int   (hStmt, i++, mail->body_download_status);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_html, 0, TEXT_2_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int   (hStmt, i++, mail->date_time);
	_bind_stmt_field_data_char  (hStmt, i++, mail->flags_seen_field);
	_bind_stmt_field_data_char  (hStmt, i++, mail->flags_deleted_field);
	_bind_stmt_field_data_char  (hStmt, i++, mail->flags_flagged_field);
	_bind_stmt_field_data_char  (hStmt, i++, mail->flags_answered_field);
	_bind_stmt_field_data_char  (hStmt, i++, mail->flags_recent_field);
	_bind_stmt_field_data_char  (hStmt, i++, mail->flags_draft_field);
	_bind_stmt_field_data_char  (hStmt, i++, mail->flags_forwarded_field);
	_bind_stmt_field_data_int   (hStmt, i++, mail->DRM_status);
	_bind_stmt_field_data_int   (hStmt, i++, mail->priority);
	_bind_stmt_field_data_int   (hStmt, i++, mail->save_status);
	_bind_stmt_field_data_int   (hStmt, i++, mail->lock_status);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->message_id, 0, MESSAGE_ID_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int   (hStmt, i++, mail->report_status);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->preview_text, 1, PREVIEWBODY_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int   (hStmt, i++, mail->smime_type);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no matched mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
	SNPRINTF(mailbox_id_param_string, 10, "%d", mail->mailbox_id);
	if (!emstorage_notify_storage_event(NOTI_MAIL_UPDATE, mail->account_id, mail->mail_id, mailbox_id_param_string, 0))
		EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event Failed [ NOTI_MAIL_UPDATE ] >>>> ");
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_modify_mailbox_of_mails(char *old_mailbox_name, char *new_mailbox_name, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("old_mailbox_name[%p], new_mailbox_name[%p], transaction[%d], err_code[%p]", old_mailbox_name, new_mailbox_name, transaction, err_code);

	if (!old_mailbox_name && !new_mailbox_name)  {
		EM_DEBUG_EXCEPTION(" old_mailbox_name[%p], new_mailbox_name[%p]", old_mailbox_name, new_mailbox_name);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	EM_DEBUG_LOG("Old Mailbox Name  [ %s ] , New Mailbox name [ %s ] ", old_mailbox_name, new_mailbox_name);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET mailbox_name = '%s' WHERE mailbox_name = '%s'", new_mailbox_name, old_mailbox_name);
 
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION(" no matched mail found...");
		
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
 
	EM_DEBUG_LOG(" Modification done in mail_read_mail_uid_tbl based on Mailbox name ");
	/* Modify the mailbox_name name in mail_read_mail_uid_tbl table */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_read_mail_uid_tbl SET mailbox_name = '%s' WHERE mailbox_name = '%s'", new_mailbox_name, old_mailbox_name);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no matched mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	if (!emstorage_notify_storage_event(NOTI_MAILBOX_UPDATE, 1, 0, new_mailbox_name, 0))
		EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event[ NOTI_MAILBOX_UPDATE] : Notification Failed >>> ");
	
	ret = true;
	
FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/**
  *  emstorage_clean_save_status(int save_status, int  *err_code) - set the all mail status to the set value
  *
  *
  **/
INTERNAL_FUNC int emstorage_clean_save_status(int save_status, int  *err_code)
{
	EM_DEBUG_FUNC_BEGIN("save_status[%d], err_code[%p]", save_status, err_code);

	EM_IF_NULL_RETURN_VALUE(err_code, false);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int rc = 0;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET save_status = %d WHERE save_status = %d", save_status, EMAIL_MAIL_STATUS_SENDING);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_LOG(" No Matched Mail Exists ");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
	}
	
	ret = true;

FINISH_OFF:
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_set_field_of_mails_with_integer_value(int account_id, int mail_ids[], int mail_ids_count, char *field_name, int value, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mail_ids[%p], mail_ids_count[%d], field_name[%s], value[%d], transaction[%d], err_code[%p]", account_id, mail_ids, mail_ids_count, field_name, value, transaction, err_code);
	int i = 0;
	int error = EMAIL_ERROR_NONE;
	int rc = 0;
	int ret = false;
	int cur_mail_id_string = 0;
	int mail_id_string_buffer_length = 0;
	char  sql_query_string[QUERY_SIZE] = {0, };
	char *mail_id_string_buffer = NULL;
	char *parameter_string = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	email_mail_attribute_type target_mail_attribute_type = 0;
	
	if (!mail_ids  || !field_name || account_id == 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	if( (error = _get_attribute_type_by_mail_field_name(field_name, &target_mail_attribute_type)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_attribute_type_by_mail_field_name failed [%d]", error);
		if (err_code != NULL)
			*err_code = error;
		return false;
	}

	/* Generating mail id list string */
	mail_id_string_buffer_length = MAIL_ID_STRING_LENGTH * mail_ids_count;

	mail_id_string_buffer = em_malloc(mail_id_string_buffer_length);

	if(!mail_id_string_buffer) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	for(i = 0; i < mail_ids_count; i++) 
		cur_mail_id_string += SNPRINTF_OFFSET(mail_id_string_buffer, cur_mail_id_string, mail_id_string_buffer_length, "%d,", mail_ids[i]);

	if(strlen(mail_id_string_buffer) > 1)
		mail_id_string_buffer[strlen(mail_id_string_buffer) - 1] = NULL_CHAR;

	/* Generating notification parameter string */
	parameter_string = em_malloc(mail_id_string_buffer_length + strlen(field_name) + 2);

	if(!parameter_string) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	SNPRINTF(parameter_string, QUERY_SIZE, "%s%c%s", field_name, 0x01, mail_id_string_buffer);

	/* Write query string */
	SNPRINTF(sql_query_string, QUERY_SIZE, "UPDATE mail_tbl SET %s = %d WHERE mail_id in (%s) AND account_id = %d", field_name, value, mail_id_string_buffer, account_id);

	EM_DEBUG_LOG("sql_query_string [%s]", sql_query_string);

	/* Execute query */
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	if (sqlite3_changes(local_db_handle) == 0) 
		EM_DEBUG_LOG("no mail matched...");

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (ret && parameter_string && !emstorage_notify_storage_event(NOTI_MAIL_FIELD_UPDATE, account_id, target_mail_attribute_type, parameter_string, value))
		EM_DEBUG_EXCEPTION("emstorage_notify_storage_event failed : NOTI_MAIL_FIELD_UPDATE [%s,%d]", field_name, value);
	
	EM_SAFE_FREE(mail_id_string_buffer);
	EM_SAFE_FREE(parameter_string);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("error [%d]", error);
	return ret;
}

INTERNAL_FUNC int emstorage_change_mail_field(int mail_id, email_mail_change_type_t type, emstorage_mail_tbl_t* mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], type[%d], mail[%p], transaction[%d], err_code[%p]", mail_id, type, mail, transaction, err_code);
	
	if (mail_id <= 0 || !mail)  {
		EM_DEBUG_EXCEPTION(" mail_id[%d], type[%d], mail[%p]", mail_id, type, mail);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int move_flag = 0;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	int rc = 0;
	int i = 0;

	char *mailbox_name = NULL;
	char mailbox_id_param_string[10] = {0,};

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	switch (type) {
		case APPEND_BODY:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				"  body_download_status = ?"
				", file_path_plain = ?"
				", file_path_html = ?"
				", flags_seen_field      = ?"  
				", flags_deleted_field   = ?"  
				", flags_flagged_field   = ?"  
				", flags_answered_field  = ?"  
				", flags_recent_field    = ?"  
				", flags_draft_field     = ?"  
				", flags_forwarded_field = ?"  
				", DRM_status = ?"
				", attachment_count = ?"
				", preview_text= ?"
				", meeting_request_status = ? "
				", message_class = ? "
				", digest_type = ? "
				", smime_type = ? "
				" WHERE mail_id = %d AND account_id != 0"
				, mail_id);

			
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			 i = 0;
			
			_bind_stmt_field_data_int(hStmt, i++, mail->body_download_status);
			_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_html, 0, TEXT_2_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_int(hStmt, i++, mail->flags_seen_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->flags_deleted_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->flags_flagged_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->flags_answered_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->flags_recent_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->flags_draft_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->flags_forwarded_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->DRM_status);
			_bind_stmt_field_data_int(hStmt, i++, mail->attachment_count);
			_bind_stmt_field_data_string(hStmt, i++, (char *)mail->preview_text, 0, PREVIEWBODY_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_int(hStmt, i++, mail->meeting_request_status);
			_bind_stmt_field_data_int(hStmt, i++, mail->message_class);
			_bind_stmt_field_data_int(hStmt, i++, mail->digest_type);
			_bind_stmt_field_data_int(hStmt, i++, mail->smime_type);
			break;
			
		case UPDATE_MAILBOX: {
				int err;
				emstorage_mailbox_tbl_t *mailbox_tbl;
			
				if (!emstorage_get_mailbox_by_name(mail->account_id, -1, mail->mailbox_name, &mailbox_tbl, false, &err)) {
					EM_DEBUG_EXCEPTION(" emstorage_get_mailbox_by_name failed - %d", err);
			
					goto FINISH_OFF;
				}

				SNPRINTF(sql_query_string, sizeof(sql_query_string),
					"UPDATE mail_tbl SET"
					" mailbox_id = '%d'"
					" mailbox_name = '%s'"
					",mailbox_type = '%d'"
					" WHERE mail_id = %d AND account_id != 0"
					, mailbox_tbl->mailbox_id
					, mail->mailbox_name ? mail->mailbox_name : ""
					, mailbox_tbl->mailbox_type
					, mail_id);
					move_flag = 1;


				EM_DEBUG_LOG("Query [%s]", sql_query_string);
				
				EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
				EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
					("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			}
			break;
			
		case UPDATE_FLAG:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				" flags_seen_field      = %d"
				",flags_deleted_field   = %d"
				",flags_flagged_field   = %d"
				",flags_answered_field  = %d"
				",flags_recent_field    = %d"
				",flags_draft_field     = %d"
				",flags_forwarded_field = %d"
				"  WHERE mail_id = %d AND account_id != 0"
				, mail->flags_seen_field
				, mail->flags_deleted_field
				, mail->flags_flagged_field
				, mail->flags_answered_field
				, mail->flags_recent_field
				, mail->flags_draft_field
				, mail->flags_forwarded_field
				, mail_id);
			EM_DEBUG_LOG("Query [%s]", sql_query_string);
 
			
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

			break;
			
		case UPDATE_EXTRA_FLAG:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				"  priority = %d"
				", save_status = %d"
				", lock_status = %d"
				", report_status = %d"
				", DRM_status = %d"
				" WHERE mail_id = %d AND account_id != 0"
				, mail->priority
				, mail->save_status
				, mail->lock_status
				, mail->report_status
				, mail->DRM_status
				, mail_id);
			EM_DEBUG_LOG("Query [%s]", sql_query_string);
 
			
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			break;

		case UPDATE_STICKY_EXTRA_FLAG:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				"  lock_status = %d"
				"  WHERE mail_id = %d AND account_id != 0"
				, mail->lock_status
				, mail_id);
			EM_DEBUG_LOG("Query [%s]", sql_query_string);
 
			
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			break;
			
		case UPDATE_MAIL:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				"  full_address_from = ?"
				", full_address_reply = ?"
				", full_address_to = ?"
				", full_address_cc = ?"
				", full_address_bcc = ?"
				", full_address_return = ?"
				", subject = ?"
				", file_path_plain = ?"
				", date_time = ?"
				", flags_seen_field = ?"
				", flags_deleted_field = ?"
				", flags_flagged_field = ?"
				", flags_answered_field = ?"
				", flags_recent_field = ?"
				", flags_draft_field = ?"
				", flags_forwarded_field = ?"
				", priority = ?"
				", save_status = ?"
				", lock_status = ?"
				", report_status = ?"
				", DRM_status = ?"
				", file_path_html = ?"
				", file_path_mime_entity = ?"
				", mail_size = ?"
				", preview_text = ?"
				", body_download_status = ?"
				", attachment_count = ?"
				", inline_content_count = ?"
				", meeting_request_status = ?"
				", message_class = ?"
				", digest_type = ?"
				", smime_type = ?"
				" WHERE mail_id = %d AND account_id != 0"
				, mail_id);
 
			
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_LOG(" before sqlite3_prepare hStmt = %p", hStmt);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			i = 0;	
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->full_address_from, 1, FROM_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->full_address_reply, 1, REPLY_TO_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->full_address_to, 1, TO_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->full_address_cc, 1, CC_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->full_address_bcc, 1, BCC_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->full_address_return, 1, RETURN_PATH_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->subject, 1, SUBJECT_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_time_t(hStmt, i++, mail->date_time);
			_bind_stmt_field_data_char  (hStmt, i++, mail->flags_seen_field);
			_bind_stmt_field_data_char  (hStmt, i++, mail->flags_deleted_field);
			_bind_stmt_field_data_char  (hStmt, i++, mail->flags_flagged_field);
			_bind_stmt_field_data_char  (hStmt, i++, mail->flags_answered_field);
			_bind_stmt_field_data_char  (hStmt, i++, mail->flags_recent_field);
			_bind_stmt_field_data_char  (hStmt, i++, mail->flags_draft_field);
			_bind_stmt_field_data_char  (hStmt, i++, mail->flags_forwarded_field);
			_bind_stmt_field_data_int   (hStmt, i++, mail->priority);
			_bind_stmt_field_data_int   (hStmt, i++, mail->save_status);
			_bind_stmt_field_data_int   (hStmt, i++, mail->lock_status);
			_bind_stmt_field_data_int   (hStmt, i++, mail->report_status);
			_bind_stmt_field_data_int   (hStmt, i++, mail->DRM_status);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->file_path_html, 0, TEXT_2_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->file_path_mime_entity, 0, MIME_ENTITY_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_int   (hStmt, i++, mail->mail_size);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->preview_text, 1, PREVIEWBODY_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_int   (hStmt, i++, mail->body_download_status);
			_bind_stmt_field_data_int   (hStmt, i++, mail->attachment_count);
			_bind_stmt_field_data_int   (hStmt, i++, mail->inline_content_count);
			_bind_stmt_field_data_int   (hStmt, i++, mail->meeting_request_status);
			_bind_stmt_field_data_int   (hStmt, i++, mail->message_class);
			_bind_stmt_field_data_int   (hStmt, i++, mail->digest_type);
			_bind_stmt_field_data_int   (hStmt, i++, mail->smime_type);
			break;
			
		case UPDATE_DATETIME:  {
			time_t now = time(NULL);

			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				" date_time = '%d'"
				" WHERE mail_id = %d AND account_id != 0"
				, (int)now
				, mail_id);

			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			break;
		}
			
		case UPDATE_FROM_CONTACT_INFO:
			EM_DEBUG_LOG("NVARCHAR : emstorage_change_mail_field - mail change type is UPDATE_FROM_CONTACT_INFO");
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				" email_address_sender = ?,"
				" WHERE mail_id = %d",
				mail_id);
 
			hStmt = NULL;
			
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			i = 0;
			_bind_stmt_field_data_string(hStmt, i++, (char *)mail->email_address_sender, 1, FROM_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
			break;
			
		case UPDATE_TO_CONTACT_INFO:
			EM_DEBUG_LOG("NVARCHAR : emstorage_change_mail_field - mail change type is UPDATE_TO_CONTACT_INFO");
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				" email_address_recipient = ?,"
				" WHERE mail_id = %d",
				mail_id);
 
			hStmt = NULL;
			
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			i = 0;
			_bind_stmt_field_data_string(hStmt, i++, (char *)mail->email_address_recipient, 1, TO_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
			break;

			case UPDATE_ALL_CONTACT_INFO:
				EM_DEBUG_LOG("emstorage_change_mail_field - mail change type is UPDATE_ALL_CONTACT_INFO");
				SNPRINTF(sql_query_string, sizeof(sql_query_string),
					"UPDATE mail_tbl SET"
					" email_address_sender = ?,"
					" email_address_recipient = ?,"
					" WHERE mail_id = %d",
					mail_id);

				hStmt = NULL;
				
				EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
				EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
					("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
				i = 0;
				_bind_stmt_field_data_string(hStmt, i++, (char *)mail->email_address_sender, 1, FROM_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
				_bind_stmt_field_data_string(hStmt, i++, (char *)mail->email_address_recipient, 1, TO_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
				break;

		
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
			case UPDATE_PARTIAL_BODY_DOWNLOAD:

			SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"UPDATE mail_tbl SET"
			"  body_download_status = ?"
			", file_path_plain = ?"
			", file_path_html = ?"
			", attachment_count = ?"
			", inline_content_count = ?"
			", preview_text= ?"
			" WHERE mail_id = %d"
			, mail_id);

			
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			 i = 0;
			
			_bind_stmt_field_data_int(hStmt, i++, mail->body_download_status);
			_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_html,  0, TEXT_2_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_int(hStmt, i++, mail->attachment_count);
			_bind_stmt_field_data_int(hStmt, i++, mail->inline_content_count);
			_bind_stmt_field_data_string(hStmt, i++, (char *)mail->preview_text,    0, PREVIEWBODY_LEN_IN_MAIL_TBL);
	
			break;

#endif
		
		default:
			EM_DEBUG_LOG(" type[%d]", type);
			
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}
 
	if (hStmt != NULL)  {

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
		rc = sqlite3_changes(local_db_handle);
		if (rc == 0)  {
			EM_DEBUG_EXCEPTION(" no matched mail found...");
	
			error = EMAIL_ERROR_MAIL_NOT_FOUND;
			goto FINISH_OFF;
		}
	}	

	if (mail->account_id == 0) {
		emstorage_mail_tbl_t* mail_for_account_tbl = NULL;
		if (!emstorage_get_mail_field_by_id(mail_id, RETRIEVE_ACCOUNT, &mail_for_account_tbl, true, &error) || !mail_for_account_tbl) {
			EM_DEBUG_EXCEPTION(" emstorage_get_mail_field_by_id failed - %d", error);
/* 		 */
			goto FINISH_OFF;
		}
		mail->account_id = mail_for_account_tbl->account_id;
		if (mail_for_account_tbl)
			emstorage_free_mail(&mail_for_account_tbl, 1, NULL);
	}

	ret = true;
	
FINISH_OFF:

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
		hStmt = NULL;
	}

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	/*h.gahlaut@samsung.com: Moving publication of notification after commiting transaction to DB */
	
	if (ret == true &&  move_flag != 1) {
		if (!emstorage_get_mailbox_name_by_mailbox_type(mail->account_id, EMAIL_MAILBOX_TYPE_SENTBOX, &mailbox_name, false, &error))
			EM_DEBUG_EXCEPTION(" emstorage_get_mailbox_name_by_mailbox_type failed - %d", error);

		if (mail->mailbox_name && mailbox_name) {
			if (strcmp(mail->mailbox_name, mailbox_name) != 0) {
				SNPRINTF(mailbox_id_param_string, 10, "%d", mail->mailbox_id);
				if (!emstorage_notify_storage_event(NOTI_MAIL_UPDATE, mail->account_id, mail_id, mailbox_id_param_string, type))
					EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event Failed [ NOTI_MAIL_UPDATE ] >>>> ");
			}
		}
		else {
			/* h.gahlaut@samsung.com: Jan 10, 2011 Publishing noti to refresh outbox when email sending status changes */
			if (!emstorage_notify_storage_event(NOTI_MAIL_UPDATE, mail->account_id, mail_id, NULL, type))
				EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event Failed [ NOTI_MAIL_UPDATE ] ");
		}
	}

	EM_SAFE_FREE(mailbox_name);
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
INTERNAL_FUNC int emstorage_increase_mail_id(int *mail_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%p], transaction[%d], err_code[%p]", mail_id, transaction, err_code);
	
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	int latest_mail_id = 0;
	sqlite3 *local_db_handle = NULL;
	char *sql = "SELECT MAX(mail_id) FROM mail_tbl;";
	char **result = NULL;

#ifdef __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__
	_timedlock_shm_mutex(&mapped_for_generating_mail_id, 2);
#endif /* __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__ */

 	ret = vconf_get_int(VCONF_KEY_LATEST_MAIL_ID, &latest_mail_id);
	if (ret < 0 || latest_mail_id == 0) {
		EM_DEBUG_LOG("vconf_get_int() failed [%d] or latest_mail_id is zero", ret);

		local_db_handle = emstorage_get_db_connection();

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
		if (NULL == result[1])
			rc = 1;
		else 
			rc = atoi(result[1]) + 1;

		sqlite3_free_table(result);
		latest_mail_id = rc; 
	}

	latest_mail_id++;

	ret = vconf_set_int(VCONF_KEY_LATEST_MAIL_ID, latest_mail_id);

	if (mail_id)
		*mail_id = latest_mail_id;

#ifdef __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__
	_unlockshm_mutex(&mapped_for_generating_mail_id);
#endif /* __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__ */

	ret = true;
	
FINISH_OFF:
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_add_mail(emstorage_mail_tbl_t *mail_tbl_data, int get_id, int transaction, int *err_code)
{
	EM_PROFILE_BEGIN(profile_emstorage_add_mail);
	EM_DEBUG_FUNC_BEGIN("mail_tbl_data[%p], get_id[%d], transaction[%d], err_code[%p]", mail_tbl_data, get_id, transaction, err_code);
	
	if (!mail_tbl_data)  {
		EM_DEBUG_EXCEPTION("mail_tbl_data[%p], get_id[%d]", mail_tbl_data, get_id);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	DB_STMT hStmt = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	if (get_id)  {
		/*  increase unique id */
		char *sql = "SELECT max(rowid) FROM mail_tbl;";
		char **result;

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
			("SQL[%s] sqlite3_get_table fail[%d] [%s]", sql, rc, sqlite3_errmsg(local_db_handle)));

		if (NULL == result[1])
			rc = 1;
		else 
			rc = atoi(result[1])+1;

		sqlite3_free_table(result);

		mail_tbl_data->mail_id   = rc;
		mail_tbl_data->thread_id = rc;
	}

	if (mail_tbl_data->date_time == 0)
		mail_tbl_data->date_time = time(NULL);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_tbl VALUES "
		"( ?" /*  mail_id */
		", ?" /*  account_id */
		", ?" /*  mailbox_id */
		", ?" /*  mailbox_name */
		", ?" /*  mailbox_type */
		", ?" /*  subject */
		", ?" /*  date_time */
		", ?" /*  server_mail_status */
		", ?" /*  server_mailbox_name */
		", ?" /*  server_mail_id */
		", ?" /*  message_id */
		", ?" /*  reference_mail_id */
		", ?" /*  full_address_from */
		", ?" /*  full_address_reply */
		", ?" /*  full_address_to */
		", ?" /*  full_address_cc */
		", ?" /*  full_address_bcc */
		", ?" /*  full_address_return */
		", ?" /*  email_address_sender */
		", ?" /*  email_address_recipient */
		", ?" /*  alias_sender */
		", ?" /*  alias_recipient */
		", ?" /*  body_download_status */
		", ?" /*  file_path_plain */
		", ?" /*  file_path_html */
		", ?" /*  file_path_mime_entity */
		", ?" /*  mail_size */
		", ?" /*  flags_seen_field */
		", ?" /*  flags_deleted_field */
		", ?" /*  flags_flagged_field */
		", ?" /*  flags_answered_field */
		", ?" /*  flags_recent_field */
		", ?" /*  flags_draft_field */
		", ?" /*  flags_forwarded_field */
		", ?" /*  DRM_status */
		", ?" /*  priority */
		", ?" /*  save_status */
		", ?" /*  lock_status */
		", ?" /*  report_status */
		", ?" /*  attachment_count */
		", ?" /*  inline_content_count */
		", ?" /*  thread_id */
		", ?" /*  thread_item_count */
		", ?" /*  preview_text */
		", ?" /*  meeting_request_status */
		", ?" /*  message_class */
		", ?" /*  digest_type */
		", ?" /*  smime_type */
		")");
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bind_stmt_field_data_int   (hStmt, MAIL_ID_IDX_IN_MAIL_TBL, mail_tbl_data->mail_id);
	_bind_stmt_field_data_int   (hStmt, ACCOUNT_ID_IDX_IN_MAIL_TBL, mail_tbl_data->account_id);
	_bind_stmt_field_data_int   (hStmt, MAILBOX_ID_IDX_IN_MAIL_TBL, mail_tbl_data->mailbox_id);
	_bind_stmt_field_data_string(hStmt, MAILBOX_NAME_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->mailbox_name, 0, MAILBOX_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int   (hStmt, MAILBOX_TYPE_IDX_IN_MAIL_TBL, mail_tbl_data->mailbox_type);
	_bind_stmt_field_data_string(hStmt, SUBJECT_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->subject, 1, SUBJECT_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int   (hStmt, DATETIME_IDX_IN_MAIL_TBL, mail_tbl_data->date_time);
	_bind_stmt_field_data_int   (hStmt, SERVER_MAIL_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->server_mail_status);
	_bind_stmt_field_data_string(hStmt, SERVER_MAILBOX_NAME_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->server_mailbox_name, 0, SERVER_MAILBOX_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, SERVER_MAIL_ID_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->server_mail_id, 0, SERVER_MAIL_ID_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, MESSAGE_ID_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->message_id, 0, MESSAGE_ID_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int   (hStmt, REFERENCE_ID_IDX_IN_MAIL_TBL, mail_tbl_data->reference_mail_id);
	_bind_stmt_field_data_string(hStmt, FULL_ADDRESS_FROM_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->full_address_from, 1, FROM_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FULL_ADDRESS_REPLY_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->full_address_reply, 1, REPLY_TO_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FULL_ADDRESS_TO_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->full_address_to, 1, TO_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FULL_ADDRESS_CC_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->full_address_cc, 1, CC_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FULL_ADDRESS_BCC_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->full_address_bcc, 1, BCC_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FULL_ADDRESS_RETURN_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->full_address_return, 1, RETURN_PATH_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, EMAIL_ADDRESS_SENDER_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->email_address_sender, 1, FROM_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, EMAIL_ADDRESS_RECIPIENT_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->email_address_recipient, 1, TO_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, ALIAS_SENDER_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->alias_sender, 1, FROM_CONTACT_NAME_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, ALIAS_RECIPIENT_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->alias_recipient, 1, FROM_CONTACT_NAME_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int   (hStmt, BODY_DOWNLOAD_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->body_download_status);
	_bind_stmt_field_data_string(hStmt, FILE_PATH_PLAIN_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FILE_PATH_HTML_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->file_path_html, 0, TEXT_2_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FILE_PATH_MIME_ENTITY_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->file_path_mime_entity, 0, MIME_ENTITY_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int   (hStmt, MAIL_SIZE_IDX_IN_MAIL_TBL, mail_tbl_data->mail_size);
	_bind_stmt_field_data_int   (hStmt, FLAGS_SEEN_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_seen_field);
	_bind_stmt_field_data_int   (hStmt, FLAGS_DELETED_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_deleted_field);
	_bind_stmt_field_data_int   (hStmt, FLAGS_FLAGGED_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_flagged_field);
	_bind_stmt_field_data_int   (hStmt, FLAGS_ANSWERED_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_answered_field);
	_bind_stmt_field_data_int   (hStmt, FLAGS_RECENT_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_recent_field);
	_bind_stmt_field_data_int   (hStmt, FLAGS_DRAFT_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_draft_field);
	_bind_stmt_field_data_int   (hStmt, FLAGS_FORWARDED_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_forwarded_field);
	_bind_stmt_field_data_int   (hStmt, DRM_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->DRM_status);
	_bind_stmt_field_data_int   (hStmt, PRIORITY_IDX_IN_MAIL_TBL, mail_tbl_data->priority);
	_bind_stmt_field_data_int   (hStmt, SAVE_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->save_status);
	_bind_stmt_field_data_int   (hStmt, LOCK_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->lock_status);
	_bind_stmt_field_data_int   (hStmt, REPORT_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->report_status);
	_bind_stmt_field_data_int   (hStmt, ATTACHMENT_COUNT_IDX_IN_MAIL_TBL, mail_tbl_data->attachment_count);
	_bind_stmt_field_data_int   (hStmt, INLINE_CONTENT_COUNT_IDX_IN_MAIL_TBL, mail_tbl_data->inline_content_count);
	_bind_stmt_field_data_int   (hStmt, THREAD_ID_IDX_IN_MAIL_TBL, mail_tbl_data->thread_id);
	_bind_stmt_field_data_int   (hStmt, THREAD_ITEM_COUNT_IDX_IN_MAIL_TBL, mail_tbl_data->thread_item_count);
	_bind_stmt_field_data_string(hStmt, PREVIEW_TEXT_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->preview_text, 1, PREVIEWBODY_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int   (hStmt, MEETING_REQUEST_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->meeting_request_status);
	_bind_stmt_field_data_int   (hStmt, MESSAGE_CLASS_IDX_IN_MAIL_TBL, mail_tbl_data->message_class);
	_bind_stmt_field_data_int   (hStmt, DIGEST_TYPE_IDX_IN_MAIL_TBL, mail_tbl_data->digest_type);
	_bind_stmt_field_data_int   (hStmt, SMIME_TYPE_IDX_IN_MAIL_TBL, mail_tbl_data->smime_type);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; }, ("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("sqlite3_step fail:%d", rc));
	ret = true;
	
FINISH_OFF:

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG("sqlite3_finalize failed [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	

	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(profile_emstorage_add_mail);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_move_multiple_mails(int account_id, int input_mailbox_id, int mail_ids[], int number_of_mails, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], input_mailbox_id [%d], mail_ids[%p], number_of_mails [%d], transaction[%d], err_code[%p]", account_id, input_mailbox_id, mail_ids, number_of_mails, transaction, err_code);

	int rc, ret = false, i, cur_conditional_clause = 0;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, }, conditional_clause[QUERY_SIZE] = {0, };
	emstorage_mailbox_tbl_t *result_mailbox = NULL;
	email_mailbox_type_e target_mailbox_type = EMAIL_MAILBOX_TYPE_USER_DEFINED;
	char* target_mailbox_name = NULL;

	if (!mail_ids || input_mailbox_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}		
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	if ((error = emstorage_get_mailbox_by_id(input_mailbox_id, &result_mailbox)) != EMAIL_ERROR_NONE || !result_mailbox) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", error);
		goto FINISH_OFF;
	}

	if(result_mailbox) {
		target_mailbox_name = EM_SAFE_STRDUP(result_mailbox->mailbox_name);
		target_mailbox_type = result_mailbox->mailbox_type;
		emstorage_free_mailbox(&result_mailbox, 1, NULL);
	}
	
	cur_conditional_clause = SNPRINTF(conditional_clause, QUERY_SIZE, "WHERE mail_id in (");

	for(i = 0; i < number_of_mails; i++) 		
		cur_conditional_clause += SNPRINTF_OFFSET(conditional_clause, cur_conditional_clause, QUERY_SIZE, "%d,", mail_ids[i]);

	conditional_clause[strlen(conditional_clause) - 1] = ')';

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	/* Updating a mail_tbl */

	memset(sql_query_string, 0x00, QUERY_SIZE);
	SNPRINTF(sql_query_string, QUERY_SIZE, "UPDATE mail_tbl SET mailbox_name = '%s', mailbox_type = %d, mailbox_id = %d %s", target_mailbox_name, target_mailbox_type, input_mailbox_id, conditional_clause);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/* Updating a mail_attachment_tbl */

	memset(sql_query_string, 0x00, QUERY_SIZE);
	SNPRINTF(sql_query_string, QUERY_SIZE, "UPDATE mail_attachment_tbl SET mailbox_id = '%d' %s", input_mailbox_id, conditional_clause);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/* Updating a mail_meeting_tbl */
	memset(sql_query_string, 0x00, QUERY_SIZE);
	SNPRINTF(sql_query_string, QUERY_SIZE, "UPDATE mail_meeting_tbl SET mailbox_id = %d %s", input_mailbox_id, conditional_clause);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/* Updating a mail_read_mail_uid_tbl */
	memset(conditional_clause, 0x00, QUERY_SIZE);
	cur_conditional_clause = SNPRINTF(conditional_clause, QUERY_SIZE, "WHERE local_uid in (");
	
	for(i = 0; i < number_of_mails; i++) 		
		cur_conditional_clause += SNPRINTF_OFFSET(conditional_clause, cur_conditional_clause, QUERY_SIZE, "%d,", mail_ids[i]);

	conditional_clause[strlen(conditional_clause) - 1] = ')';

	memset(sql_query_string, 0x00, QUERY_SIZE);
	SNPRINTF(sql_query_string, QUERY_SIZE, "UPDATE mail_read_mail_uid_tbl SET mailbox_name = '%s', mailbox_id = %d %s", target_mailbox_name, input_mailbox_id, conditional_clause);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	ret = true;
	
FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	EM_SAFE_FREE(target_mailbox_name);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
		
INTERNAL_FUNC int emstorage_delete_mail(int mail_id, int from_server, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], transaction[%d], err_code[%p]", mail_id, transaction, err_code);

	if (!mail_id)  {
		EM_DEBUG_EXCEPTION("mail_id[%d]", mail_id);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}		
	
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_tbl WHERE mail_id = %d ", mail_id);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);
 

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	ret = true;
	
FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_multiple_mails(int mail_ids[], int number_of_mails, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], number_of_mails [%d], transaction[%d], err_code[%p]", mail_ids, number_of_mails, transaction, err_code);

	int rc, ret = false, i, cur_sql_query_string = 0;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	if (!mail_ids) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}		
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	cur_sql_query_string = SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_tbl WHERE mail_id in (");
	
	for(i = 0; i < number_of_mails; i++) 		
		cur_sql_query_string += SNPRINTF_OFFSET(sql_query_string, cur_sql_query_string, QUERY_SIZE, "%d,", mail_ids[i]);

	sql_query_string[strlen(sql_query_string) - 1] = ')';
	
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	ret = true;
	
FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_mail_by_account(int account_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], transaction[%d], err_code[%p]", account_id, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID)  {
		EM_DEBUG_EXCEPTION("account_id[%d]", account_id);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_tbl WHERE account_id = %d", account_id);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
	}
 
	/* Delete all mails  mail_read_mail_uid_tbl table based on account id */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_read_mail_uid_tbl WHERE account_id = %d", account_id);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION("no mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
	}

	if (!emstorage_notify_storage_event(NOTI_MAIL_DELETE_WITH_ACCOUNT, account_id, 0 , NULL, 0))
		EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event Failed [ NOTI_MAIL_DELETE_ALL ]");
	
	ret = true;
	
FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_mail_by_mailbox(int account_id, char *mailbox, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox[%p], transaction[%d], err_code[%p]", account_id, mailbox, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !mailbox)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox[%p]", account_id, mailbox);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_tbl WHERE account_id = %d AND mailbox_name = '%s'", account_id, mailbox);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		

	/* Delete Mails from mail_read_mail_uid_tbl */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_read_mail_uid_tbl WHERE account_id = %d AND mailbox_name = '%s'", account_id, mailbox);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
 	
	if (!emstorage_notify_storage_event(NOTI_MAIL_DELETE_ALL, account_id, 0 , mailbox, 0))
		EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event Failed [ NOTI_MAIL_DELETE_ALL ] >>>> ");
	
	ret = true;
	
FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_free_mail(emstorage_mail_tbl_t** mail_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_list[%p], count[%d], err_code[%p]", mail_list, count, err_code);
	
	if (count > 0)  {
		if ((mail_list == NULL) || (*mail_list == NULL))  {
			EM_DEBUG_EXCEPTION("mail_ilst[%p], count[%d]", mail_list, count);
			
			if (err_code)
				*err_code = EMAIL_ERROR_INVALID_PARAM;
			return false;
		}

		emstorage_mail_tbl_t* p = *mail_list;
		int i = 0;

		for (; i < count; i++, p++) {
			EM_SAFE_FREE(p->mailbox_name);
			EM_SAFE_FREE(p->server_mailbox_name);
			EM_SAFE_FREE(p->server_mail_id);
			EM_SAFE_FREE(p->full_address_from);
			EM_SAFE_FREE(p->full_address_reply);
			EM_SAFE_FREE(p->full_address_to);
			EM_SAFE_FREE(p->full_address_cc);
			EM_SAFE_FREE(p->full_address_bcc);
			EM_SAFE_FREE(p->full_address_return);
			EM_SAFE_FREE(p->subject);
			EM_SAFE_FREE(p->file_path_plain);
			EM_SAFE_FREE(p->file_path_html);
			EM_SAFE_FREE(p->file_path_mime_entity);
			EM_SAFE_FREE(p->message_id);
			EM_SAFE_FREE(p->email_address_sender);
			EM_SAFE_FREE(p->email_address_recipient);
			EM_SAFE_FREE(p->preview_text);
			EM_SAFE_FREE(p->alias_sender);
			EM_SAFE_FREE(p->alias_recipient);
		}
		EM_SAFE_FREE(*mail_list);
	}
	
	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emstorage_get_attachment_count(int mail_id, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], count[%p], transaction[%d], err_code[%p]", mail_id, count, transaction, err_code);
	
	if (mail_id <= 0 || !count)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], count[%p]", mail_id, count);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_attachment_tbl WHERE mail_id = %d", mail_id);

	char **result;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);
 
	ret = true;
	
FINISH_OFF:
 
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_attachment_list(int input_mail_id, int input_transaction, emstorage_attachment_tbl_t** output_attachment_list, int *output_attachment_count)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], input_transaction[%d], output_attachment_list[%p], output_attachment_count[%p]", input_mail_id, output_attachment_list, input_transaction, output_attachment_count);
	
	if (input_mail_id <= 0 || !output_attachment_list || !output_attachment_count)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}
	
	int                         error = EMAIL_ERROR_NONE;
	int                         i = 0;
	int                         rc = -1;
	char                      **result = NULL;
	char                        sql_query_string[QUERY_SIZE] = {0, };
	emstorage_attachment_tbl_t* p_data_tbl = NULL;
	DB_STMT hStmt = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	
	EMSTORAGE_START_READ_TRANSACTION(input_transaction);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_attachment_tbl WHERE mail_id = %d", input_mail_id);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*output_attachment_count = atoi(result[1]);
	sqlite3_free_table(result);
	
	if(*output_attachment_count == 0) {
		error = EMAIL_ERROR_NONE;
		goto FINISH_OFF;
	}
	
	p_data_tbl = (emstorage_attachment_tbl_t*)em_malloc(sizeof(emstorage_attachment_tbl_t) * (*output_attachment_count));
	
	if (!p_data_tbl)  {
		EM_DEBUG_EXCEPTION("em_malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_attachment_tbl WHERE mail_id = %d ORDER BY attachment_id", input_mail_id);
	EM_DEBUG_LOG("sql_query_string [%s]", sql_query_string);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },	("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no matched attachment found...");
		error = EMAIL_ERROR_ATTACHMENT_NOT_FOUND;
		goto FINISH_OFF;
	}
	for (i = 0; i < *output_attachment_count; i++)  {
		_get_stmt_field_data_int   (hStmt, &(p_data_tbl[i].attachment_id), ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].attachment_name), 0, ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].attachment_path), 0, ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int   (hStmt, &(p_data_tbl[i].attachment_size), ATTACHMENT_SIZE_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int   (hStmt, &(p_data_tbl[i].mail_id), MAIL_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int   (hStmt, &(p_data_tbl[i].account_id), ACCOUNT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int   (hStmt, &(p_data_tbl[i].mailbox_id), MAILBOX_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int   (hStmt, &(p_data_tbl[i].attachment_save_status), ATTACHMENT_SAVE_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int   (hStmt, &(p_data_tbl[i].attachment_drm_type), ATTACHMENT_DRM_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int   (hStmt, &(p_data_tbl[i].attachment_drm_method), ATTACHMENT_DRM_METHOD_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int   (hStmt, &(p_data_tbl[i].attachment_inline_content_status), ATTACHMENT_INLINE_CONTENT_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].attachment_mime_type), 0, ATTACHMENT_MIME_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL);
		
		EM_DEBUG_LOG("attachment[%d].attachment_id : %d", i, p_data_tbl[i].attachment_id);
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("sqlite3_step fail:%d", rc));
	}
	
FINISH_OFF:

	if (error == EMAIL_ERROR_NONE)
		*output_attachment_list = p_data_tbl;
	else if (p_data_tbl != NULL)
		emstorage_free_attachment(&p_data_tbl, *output_attachment_count, NULL);

	rc = sqlite3_finalize(hStmt);
	
	if (rc != SQLITE_OK)  {
		EM_DEBUG_EXCEPTION("sqlite3_finalize failed [%d]", rc);
		error = EMAIL_ERROR_DB_FAILURE;
	}
	
	EMSTORAGE_FINISH_READ_TRANSACTION(input_transaction);
	
	_DISCONNECT_DB;

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int emstorage_get_attachment(int attachment_id, emstorage_attachment_tbl_t** attachment, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_id[%d], attachment[%p], transaction[%d], err_code[%p]", attachment_id, attachment, transaction, err_code);
	
	if (attachment_id <= 0 || !attachment)  {
		EM_DEBUG_EXCEPTION("attachment_id[%d], attachment[%p]", attachment_id, attachment);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	emstorage_attachment_tbl_t* p_data_tbl = NULL;
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_attachment_tbl WHERE attachment_id = %d",  attachment_id);

	sqlite3_stmt* hStmt = NULL;
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG(" before sqlite3_prepare hStmt = %p", hStmt);
	
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no matched attachment found...");
		error = EMAIL_ERROR_ATTACHMENT_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (!(p_data_tbl = (emstorage_attachment_tbl_t*)em_malloc(sizeof(emstorage_attachment_tbl_t) * 1)))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	_get_stmt_field_data_int(hStmt, &(p_data_tbl->attachment_id), ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->attachment_name), 0, ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->attachment_path), 0, ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->attachment_size), ATTACHMENT_SIZE_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->mail_id), MAIL_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), ACCOUNT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->mailbox_id), MAILBOX_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->attachment_save_status), ATTACHMENT_SAVE_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->attachment_drm_type), ATTACHMENT_DRM_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->attachment_drm_method), ATTACHMENT_DRM_METHOD_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->attachment_inline_content_status), ATTACHMENT_INLINE_CONTENT_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->attachment_mime_type), 0, ATTACHMENT_MIME_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL);

#ifdef __ATTACHMENT_OPTI__
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->encoding), ENCODING_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->section), 0, SECTION_IDX_IN_MAIL_ATTACHMENT_TBL);
#endif	
 
	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*attachment = p_data_tbl;

	if (hStmt != NULL)  {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_attachment_nth(int mail_id, int nth, emstorage_attachment_tbl_t** attachment_tbl, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], nth[%d], attachment_tbl[%p], transaction[%d], err_code[%p]", mail_id, nth, attachment_tbl, transaction, err_code);

	if (mail_id <= 0 || nth <= 0 || !attachment_tbl)  {
		EM_DEBUG_EXCEPTION(" mail_id[%d], nth[%d], attachment[%p]", mail_id, nth, attachment_tbl);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	emstorage_attachment_tbl_t* p_data_tbl = NULL;
	char *p = NULL;
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_attachment_tbl WHERE mail_id = %d ORDER BY attachment_id LIMIT %d, 1", mail_id, (nth - 1));
	EM_DEBUG_LOG("query = [%s]", sql_query_string);
	
	DB_STMT hStmt = NULL;
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION(" no matched attachment found...");
		error = EMAIL_ERROR_ATTACHMENT_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	if (!(p_data_tbl = (emstorage_attachment_tbl_t*)em_malloc(sizeof(emstorage_attachment_tbl_t) * 1)))  {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	p_data_tbl->attachment_id = sqlite3_column_int(hStmt, ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	if ((p = (char *)sqlite3_column_text(hStmt, ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)strlen(p))
		p_data_tbl->attachment_name = cpy_str(p);
	if ((p = (char *)sqlite3_column_text(hStmt, ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)strlen(p))
		p_data_tbl->attachment_path = cpy_str(p);
	p_data_tbl->attachment_size = sqlite3_column_int(hStmt, ATTACHMENT_SIZE_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->mail_id = sqlite3_column_int(hStmt, MAIL_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->account_id = sqlite3_column_int(hStmt, ACCOUNT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->mailbox_id = sqlite3_column_int(hStmt, MAILBOX_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->attachment_save_status = sqlite3_column_int(hStmt, ATTACHMENT_SAVE_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->attachment_drm_type = sqlite3_column_int(hStmt, ATTACHMENT_DRM_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->attachment_drm_method = sqlite3_column_int(hStmt, ATTACHMENT_DRM_METHOD_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->attachment_inline_content_status = sqlite3_column_int(hStmt, ATTACHMENT_INLINE_CONTENT_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL);
	if ((p = (char *)sqlite3_column_text(hStmt, ATTACHMENT_MIME_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)strlen(p))
		p_data_tbl->attachment_mime_type = cpy_str(p);
#ifdef __ATTACHMENT_OPTI__
		p_data_tbl->encoding = sqlite3_column_int(hStmt, ENCODING_IDX_IN_MAIL_ATTACHMENT_TBL);
		if ((p = (char *)sqlite3_column_text(hStmt, SECTION_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)strlen(p))
			p_data_tbl->section= cpy_str(p);
#endif	
 	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*attachment_tbl = p_data_tbl;
	else if (p_data_tbl != NULL)
		emstorage_free_attachment(&p_data_tbl, 1, NULL);

	if (hStmt != NULL)  {
		EM_DEBUG_LOG("before sqlite3_finalize hStmt = %p", hStmt);
	
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
 	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_change_attachment_field(int mail_id, email_mail_change_type_t type, emstorage_attachment_tbl_t* attachment, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], type[%d], attachment[%p], transaction[%d], err_code[%p]", mail_id, type, attachment, transaction, err_code);
	
	if (mail_id <= 0 || !attachment)  {
		EM_DEBUG_EXCEPTION(" mail_id[%d], type[%d], attachment[%p]", mail_id, type, attachment);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;;
	}
	
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	int i = 0;
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	switch (type)  {
		case UPDATE_MAILBOX:
				EM_DEBUG_LOG("UPDATE_MAILBOX");
			if (!attachment->mailbox_id)  {
				EM_DEBUG_EXCEPTION(" attachment->mailbox_id[%d]", attachment->mailbox_id);
				error = EMAIL_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_attachment_tbl SET mailbox_id = ? WHERE mail_id = %d", mail_id);

			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_LOG(" Before sqlite3_prepare hStmt = %p", hStmt);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

			_bind_stmt_field_data_int(hStmt, i++, attachment->mailbox_id);
			break;
			
		case UPDATE_SAVENAME:
			EM_DEBUG_LOG("UPDATE_SAVENAME");
			if (!attachment->attachment_path)  {
				EM_DEBUG_EXCEPTION(" attachment->attachment_path[%p]", attachment->attachment_path);
				error = EMAIL_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}

			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_attachment_tbl SET"
				"  attachment_size = ?"
				", attachment_save_status = 1"
				", attachment_path = ?"
				" WHERE mail_id = %d"
				" AND attachment_id = %d"
				, attachment->mail_id
				, attachment->attachment_id);
 
			
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_LOG(" Before sqlite3_prepare hStmt = %p", hStmt);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		
			_bind_stmt_field_data_int(hStmt, i++, attachment->attachment_size);
			_bind_stmt_field_data_string(hStmt, i++, (char *)attachment->attachment_path, 0, ATTACHMENT_PATH_LEN_IN_MAIL_ATTACHMENT_TBL);
			break;
			
		default:
			EM_DEBUG_LOG("type[%d]", type);
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}
	EM_DEBUG_LOG("query = [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" Before sqlite3_finalize hStmt = %p", hStmt);
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_rename_mailbox(int input_mailbox_id, char *input_new_mailbox_name, char *input_new_mailbox_alias, int input_transaction)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d], input_new_mailbox_name[%p], input_new_mailbox_alias [%p], input_transaction[%d]", input_mailbox_id, input_new_mailbox_name, input_new_mailbox_alias, input_transaction);

	int rc = 0;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = NULL;
	int account_id = 0;
	emstorage_mailbox_tbl_t *old_mailbox_data = NULL;

	if (input_mailbox_id <= 0 || !input_new_mailbox_name || !input_new_mailbox_alias)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	local_db_handle = emstorage_get_db_connection();

	if ((error = emstorage_get_mailbox_by_id(input_mailbox_id, &old_mailbox_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", error);
		goto FINISH_OFF;
	}
	account_id = old_mailbox_data->account_id;

	EMSTORAGE_START_WRITE_TRANSACTION(input_transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_box_tbl SET"
		" mailbox_name = '%s'"
		",alias = '%s'"
		" WHERE mailbox_id = %d"
		, input_new_mailbox_name
		, input_new_mailbox_alias
		, input_mailbox_id);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (sqlite3_changes(local_db_handle) == 0)
		EM_DEBUG_LOG("no mail_meeting_tbl matched...");

	/* Update mail_tbl */
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_tbl SET"
		" mailbox_name = '%s'"
		" WHERE mailbox_id = %d"
		, input_new_mailbox_name
		, input_mailbox_id);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (sqlite3_changes(local_db_handle) == 0) 
		EM_DEBUG_LOG("no mail matched...");

	ret = true;
	
FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(input_transaction, ret, error);

	if (ret) {
		if (!emstorage_notify_storage_event(NOTI_MAILBOX_RENAME, account_id, input_mailbox_id, input_new_mailbox_name, 0))
			EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event Failed [ NOTI_MAILBOX_RENAME ] >>>> ");
	}
	else {
		if (!emstorage_notify_storage_event(NOTI_MAILBOX_RENAME_FAIL, account_id, input_mailbox_id, input_new_mailbox_name, 0))
			EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event Failed [ NOTI_MAILBOX_RENAME_FAIL ] >>>> ");
	}

	if (old_mailbox_data)
		emstorage_free_mailbox(&old_mailbox_data, 1, NULL);

	_DISCONNECT_DB;

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}
	
INTERNAL_FUNC int emstorage_get_new_attachment_no(int *attachment_no, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_no [%p], err_code[%p]", attachment_no, err_code);
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char *sql = "SELECT max(rowid) FROM mail_attachment_tbl;";
	char **result;
	
	if (!attachment_no)  {
		EM_DEBUG_EXCEPTION("Invalid attachment");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	*attachment_no = -1;
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL == result[1])
		rc = 1;
	else 
		rc = atoi(result[1])+1;

	sqlite3_free_table(result);

	*attachment_no = rc;
	EM_DEBUG_LOG("attachment_no [%d]", *attachment_no);
	ret = true;
	
FINISH_OFF:
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
	
INTERNAL_FUNC int emstorage_add_attachment(emstorage_attachment_tbl_t* attachment_tbl, int iscopy, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_tbl[%p], iscopy[%d], transaction[%d], err_code[%p]", attachment_tbl, iscopy, transaction, err_code);

	char *sql = NULL;
	char **result;
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	
	if (!attachment_tbl)  {
		EM_DEBUG_EXCEPTION("attachment_tbl[%p], iscopy[%d]", attachment_tbl, iscopy);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
 
	sql = "SELECT max(rowid) FROM mail_attachment_tbl;";

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL==result[1]) rc = 1;
	else rc = atoi(result[1]) + 1;
	sqlite3_free_table(result);

	attachment_tbl->attachment_id = rc;
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_attachment_tbl VALUES "
		"( ?"	/* attachment_id */
		", ?"	/* attachment_name */
		", ?"	/* attachment_path */
		", ?"	/* attachment_size */
		", ?"	/* mail_id */
		", ?"	/* account_id */
		", ?"	/* mailbox_id */
		", ?"	/* attachment_save_status */
		", ?"	/* attachment_drm_type */
		", ?"	/* attachment_drm_method */
		", ?"   /* attachment_inline_content_status */
		", ?"   /* attachment_mime_type */
#ifdef __ATTACHMENT_OPTI__
		", ?"
		", ?"
#endif		
		")");
	
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	_bind_stmt_field_data_int   (hStmt, ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->attachment_id);
	_bind_stmt_field_data_string(hStmt, ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL, (char*)attachment_tbl->attachment_name, 0, ATTACHMENT_NAME_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bind_stmt_field_data_string(hStmt, ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL, (char*)attachment_tbl->attachment_path, 0, ATTACHMENT_PATH_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bind_stmt_field_data_int   (hStmt, ATTACHMENT_SIZE_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->attachment_size);
	_bind_stmt_field_data_int   (hStmt, MAIL_ID_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->mail_id);
	_bind_stmt_field_data_int   (hStmt, ACCOUNT_ID_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->account_id);
	_bind_stmt_field_data_int   (hStmt, MAILBOX_ID_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->mailbox_id);
	_bind_stmt_field_data_int   (hStmt, ATTACHMENT_SAVE_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->attachment_save_status);
	_bind_stmt_field_data_int   (hStmt, ATTACHMENT_DRM_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->attachment_drm_type);
	_bind_stmt_field_data_int   (hStmt, ATTACHMENT_DRM_METHOD_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->attachment_drm_method);
	_bind_stmt_field_data_int   (hStmt, ATTACHMENT_INLINE_CONTENT_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->attachment_inline_content_status);
	_bind_stmt_field_data_string(hStmt, ATTACHMENT_MIME_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL, (char*)attachment_tbl->attachment_mime_type, 0, ATTACHMENT_MIME_TYPE_LEN_IN_MAIL_ATTACHMENT_TBL);
#ifdef __ATTACHMENT_OPTI__
	_bind_stmt_field_data_int(hStmt, ENCODING_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->encoding);
	_bind_stmt_field_data_string(hStmt, SECTION_IDX_IN_MAIL_ATTACHMENT_TBL, (char*)attachment_tbl->section, 0, ATTACHMENT_LEN_IN_MAIL_ATTACHMENT_TBL);
#endif
	

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
/*
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_tbl SET attachment_count = 1 WHERE mail_id = %d", attachment_tbl->mail_id);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
*/
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no matched mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
	*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_update_attachment(emstorage_attachment_tbl_t* attachment_tbl, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_tbl[%p], transaction[%d], err_code[%p]", attachment_tbl, transaction, err_code);
	
	int rc, ret = false, field_idx = 0;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
 	char sql_query_string[QUERY_SIZE] = {0, };

	if (!attachment_tbl)  {
		EM_DEBUG_EXCEPTION(" attachment_tbl[%p] ", attachment_tbl);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
 
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_attachment_tbl SET  "
		"  attachment_name = ?"   
		", attachment_path =  ?"
		", attachment_size = ?"
		", mail_id = ?"
		", account_id = ?"
		", mailbox_id = ?"
		", attachment_save_status = ?"
		", attachment_drm_type = ?"
		", attachment_drm_method = ?"
		", attachment_inline_content_status = ? "
		", attachment_mime_type = ? "
		" WHERE attachment_id = ?;");
	
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bind_stmt_field_data_string(hStmt, field_idx++ , (char*)attachment_tbl->attachment_name, 0, ATTACHMENT_NAME_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bind_stmt_field_data_string(hStmt, field_idx++ , (char*)attachment_tbl->attachment_path, 0, ATTACHMENT_PATH_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bind_stmt_field_data_int   (hStmt, field_idx++ , attachment_tbl->attachment_size);
	_bind_stmt_field_data_int   (hStmt, field_idx++ , attachment_tbl->mail_id);
	_bind_stmt_field_data_int   (hStmt, field_idx++ , attachment_tbl->account_id);
	_bind_stmt_field_data_int   (hStmt, field_idx++ , attachment_tbl->mailbox_id);
	_bind_stmt_field_data_int   (hStmt, field_idx++ , attachment_tbl->attachment_save_status);
	_bind_stmt_field_data_int   (hStmt, field_idx++ , attachment_tbl->attachment_drm_type);
	_bind_stmt_field_data_int   (hStmt, field_idx++ , attachment_tbl->attachment_drm_method);
	_bind_stmt_field_data_int   (hStmt, field_idx++ , attachment_tbl->attachment_inline_content_status);
	_bind_stmt_field_data_string(hStmt, field_idx++ , (char*)attachment_tbl->attachment_mime_type, 0, ATTACHMENT_MIME_TYPE_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bind_stmt_field_data_int   (hStmt, field_idx++ , attachment_tbl->attachment_id);
	

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
/* 
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_tbl SET attachment_count = 1 WHERE mail_id = %d", attachment_tbl->mail_id);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
*/
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no matched mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
	*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_attachment_on_db(int attachment_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_id[%d], transaction[%d], err_code[%p]", attachment_id, transaction, err_code);
	
	if (attachment_id < 0)  {
		EM_DEBUG_EXCEPTION("attachment_id[%d]", attachment_id);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_attachment_tbl WHERE attachment_id = %d", attachment_id);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	if (err_code)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_all_attachments_of_mail(int mail_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], transaction[%d], err_code[%p]", mail_id, transaction, err_code);

	if (mail_id < 0)  {
		EM_DEBUG_EXCEPTION("mail_id[%d]", mail_id);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_attachment_tbl WHERE mail_id = %d", mail_id);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	ret = true;
	
FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_attachment_all_on_db(int account_id, char *mailbox, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox[%p], transaction[%d], err_code[%p]", account_id, mailbox, transaction, err_code);
	
	int error = EMAIL_ERROR_NONE;
	int rc, ret = false;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_attachment_tbl");
	
	if (account_id != ALL_ACCOUNT) /*  '0' means all account */
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " WHERE account_id = %d", account_id);

	if (mailbox) 	/*  NULL means all mailbox_name */
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " %s mailbox_name = '%s'", account_id != ALL_ACCOUNT ? "AND" : "WHERE", mailbox);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	ret = true;
	
FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_free_attachment(emstorage_attachment_tbl_t** attachment_tbl_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_tbl_list[%p], count[%d], err_code[%p]", attachment_tbl_list, count, err_code);
	
	if (count > 0)  {
		if ((attachment_tbl_list == NULL) || (*attachment_tbl_list == NULL))  {
			EM_DEBUG_EXCEPTION(" attachment_tbl_list[%p], count[%d]", attachment_tbl_list, count);
			if (err_code != NULL)
				*err_code = EMAIL_ERROR_INVALID_PARAM;
			return false;
		}
		
		emstorage_attachment_tbl_t* p = *attachment_tbl_list;
		int i;
		
		for (i = 0; i < count; i++)  {
			EM_SAFE_FREE(p[i].attachment_name);
			EM_SAFE_FREE(p[i].attachment_path);
#ifdef __ATTACHMENT_OPTI__
			EM_SAFE_FREE(p[i].section);
#endif			
		}
		
		EM_SAFE_FREE(p); 
		*attachment_tbl_list = NULL;
	}
	
	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;
	EM_DEBUG_FUNC_END();
	return true;
}



INTERNAL_FUNC int emstorage_begin_transaction(void *d1, void *d2, int *err_code)
{
	EM_PROFILE_BEGIN(emStorageBeginTransaction);
	int ret = true;

	ENTER_CRITICAL_SECTION(_transactionBeginLock);
	
	/*  wait for the trnasaction authority to be changed. */
	while (g_transaction)  {
		EM_DEBUG_LOG(">>>>>>>> Wait for the transaction authority to be changed");
		usleep(50000);
	}

	/*  take the transaction authority. */
	g_transaction = true;

	LEAVE_CRITICAL_SECTION(_transactionBeginLock);
 
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	int rc;
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN immediate;", NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {ret = false; },
		("SQL(BEGIN) exec error:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
	if (ret == false && err_code != NULL)
		*err_code = EMAIL_ERROR_DB_FAILURE;

	EM_PROFILE_END(emStorageBeginTransaction);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_commit_transaction(void *d1, void *d2, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = true;
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	ENTER_CRITICAL_SECTION(_transactionEndLock);

	int rc;
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {ret = false; }, ("SQL(END) exec error:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
	/*  release the transaction authority. */
	g_transaction = false;

	LEAVE_CRITICAL_SECTION(_transactionEndLock);
	if (ret == false && err_code != NULL)
		*err_code = EMAIL_ERROR_DB_FAILURE;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_rollback_transaction(void *d1, void *d2, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = true;
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	int rc;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "ROLLBACK;", NULL, NULL, NULL), rc);

	ENTER_CRITICAL_SECTION(_transactionEndLock);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {ret = false; },
		("SQL(ROLLBACK) exec error:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

	/*  release the transaction authority. */
	g_transaction = false;

	LEAVE_CRITICAL_SECTION(_transactionEndLock);

	if (ret == false && err_code != NULL)
		*err_code = EMAIL_ERROR_DB_FAILURE;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_is_mailbox_full(int account_id, email_mailbox_t *mailbox, int *result, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox[%p], result[%p], err_code[%p]", account_id, mailbox, result, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !mailbox || !result)  {
		if (mailbox)
			EM_DEBUG_EXCEPTION("Invalid Parameter. accoun_id[%d], mailbox[%p]", account_id, mailbox);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;

		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int mail_count = 0;

	if (!emstorage_get_mail_count(account_id, mailbox->mailbox_name, &mail_count, NULL, true, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_count failed [%d]", error);
		goto FINISH_OFF;
	}
	
	if (mailbox) {
		EM_DEBUG_LOG("mail_count[%d] mail_slot_size[%d]", mail_count, mailbox->mail_slot_size);
		if (mail_count >= mailbox->mail_slot_size)
			*result = true;
		else
			*result = false;

		ret = true;
	}
	
	ret = true;	
FINISH_OFF:

	if (err_code != NULL)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_clear_mail_data(int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("transaction[%d], err_code[%p]", transaction, err_code);

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	const email_db_object_t* tables = _g_db_tables;
	const email_db_object_t* indexes = _g_db_indexes;
	char data_path[256];
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	if (!emstorage_delete_dir(MAILHOME, &error)) {
		EM_DEBUG_EXCEPTION(" emstorage_delete_dir failed - %d", error);

		goto FINISH_OFF;
	}
	
	SNPRINTF(data_path, sizeof(data_path), "%s/%s", MAILHOME, MAILTEMP);
	
	mkdir(MAILHOME, DIRECTORY_PERMISSION);
	mkdir(data_path, DIRECTORY_PERMISSION);
	
	/*  first clear index. */
	while (indexes->object_name)  {
		if (indexes->data_flag)  {
			SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP index %s", indexes->object_name);
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
			EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		}
		indexes++;
	}
	
	while (tables->object_name)  {
		if (tables->data_flag)  {
			SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP table %s", tables->object_name);
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
			EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		}
		
		tables++;
	}
	ret = true;
	
FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
/*======================= DB File Utils =============================================*/
#include <dirent.h>
#include <sys/types.h>
#define  DIR_SEPERATOR "/"

char *__em_create_dir_by_file_name(char *file_name)
{
	EM_DEBUG_FUNC_BEGIN("Filename [ %p ]", file_name);
	char delims[] = "/";
	char *result = NULL;

	result = strtok(file_name, delims);

	if (result)
		EM_DEBUG_LOG(">>>> Directory_name [ %s ]", result);
	else
		EM_DEBUG_LOG(">>>> No Need to create Directory");

	return result;
}

INTERNAL_FUNC int emstorage_get_save_name(int account_id, int mail_id, int atch_id, char *fname, char *name_buf, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], atch_id[%d], fname[%s], name_buf[%p], err_code[%p]", account_id, mail_id, atch_id, fname, name_buf, err_code);
	EM_PROFILE_BEGIN(profile_emstorage_get_save_name);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char *dir_name = NULL;
	char create_dir[1024]={0};
	char *temp_file = NULL;
	
	if (!name_buf || account_id < FIRST_ACCOUNT_ID || mail_id < 0 || atch_id < 0)  {	
		EM_DEBUG_EXCEPTION(" account_id[%d], mail_id[%d], atch_id[%d], fname[%p], name_buf[%p]", account_id, mail_id, atch_id, fname, name_buf);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	sprintf(name_buf, "%s", MAILHOME);
	sprintf(name_buf+strlen(name_buf), 	"%s%d", DIR_SEPERATOR, account_id);
	
	if (mail_id > 0)
		sprintf(name_buf+strlen(name_buf), 	"%s%d", DIR_SEPERATOR, mail_id);

	if (atch_id > 0)
		sprintf(name_buf+strlen(name_buf), 	"%s%d", DIR_SEPERATOR, atch_id);

	if (fname) {
		temp_file = EM_SAFE_STRDUP(fname);
		if (strstr(temp_file, "/")) {
			dir_name = __em_create_dir_by_file_name(temp_file);
		}
	}

	if (dir_name) {
		sprintf(create_dir, 	"%s%s%s", name_buf, DIR_SEPERATOR, dir_name);
		EM_DEBUG_LOG(">>>>> DIR PATH [ %s ]", create_dir);
		mkdir(create_dir, DIRECTORY_PERMISSION);
		EM_SAFE_FREE(temp_file);
	}

	
	if (fname) {
		EM_DEBUG_LOG(">>>>> fname [ %s ]", fname);
		sprintf(name_buf+strlen(name_buf), 	"%s%s", DIR_SEPERATOR, fname);
	}
		

	EM_DEBUG_LOG(">>>>> name_buf [ %s ]", name_buf);
	
	ret = true;
	
FINISH_OFF:
	EM_SAFE_FREE(temp_file);

	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(profile_emstorage_get_save_name);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_dele_name(int account_id, int mail_id, int atch_id, char *fname, char *name_buf, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], atch_id[%d], fname[%p], name_buf[%p], err_code[%p]", account_id, mail_id, atch_id, fname, name_buf, err_code);
	
	if (!name_buf || account_id < FIRST_ACCOUNT_ID)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], mail_id[%d], atch_id[%d], fname[%p], name_buf[%p]", account_id, mail_id, atch_id, fname, name_buf);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sprintf(name_buf+strlen(name_buf), 	"%s%s%d", MAILHOME, DIR_SEPERATOR, account_id);
	
	if (mail_id > 0)
		sprintf(name_buf+strlen(name_buf), 	"%s%d", DIR_SEPERATOR, mail_id);
	else
		goto FINISH_OFF;
	
	if (atch_id > 0)
		sprintf(name_buf+strlen(name_buf), 	"%s%d", DIR_SEPERATOR, atch_id);
	else
		goto FINISH_OFF;

FINISH_OFF:
	sprintf(name_buf+strlen(name_buf), 	".DELE");

	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emstorage_create_dir(int account_id, int mail_id, int atch_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], atch_id[%d], err_code[%p]", account_id, mail_id, atch_id, err_code);
	EM_PROFILE_BEGIN(profile_emcore_save_create_dir);
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	char buf[512];
	struct stat sbuf;
	if (account_id >= FIRST_ACCOUNT_ID)  {	
		SNPRINTF(buf, sizeof(buf), "%s%s%d", MAILHOME, DIR_SEPERATOR, account_id);
		
		if (stat(buf, &sbuf) == 0) {
			if ((sbuf.st_mode & S_IFMT) != S_IFDIR)  {
				EM_DEBUG_EXCEPTION(" a object which isn't directory aleady exists");
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		}
		else  {
			if (mkdir(buf, DIRECTORY_PERMISSION) != 0) {
				EM_DEBUG_EXCEPTION(" mkdir failed [%s]", buf);
				EM_DEBUG_EXCEPTION("mkdir failed l(Errno=%d)][ErrStr=%s]", errno, strerror(errno));
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		}
	}

	if (mail_id > 0)  {
		if (account_id < FIRST_ACCOUNT_ID)  {
			EM_DEBUG_EXCEPTION("account_id[%d], mail_id[%d], atch_id[%d]", account_id, mail_id, atch_id);
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		SNPRINTF(buf+strlen(buf), sizeof(buf), "%s%d", DIR_SEPERATOR, mail_id);
		
		if (stat(buf, &sbuf) == 0) {
			if ((sbuf.st_mode & S_IFMT) != S_IFDIR)  {
				EM_DEBUG_EXCEPTION(" a object which isn't directory aleady exists");
				
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		}
		else  {
			if (mkdir(buf, DIRECTORY_PERMISSION) != 0) {
				EM_DEBUG_EXCEPTION(" mkdir failed [%s]", buf);
				EM_DEBUG_EXCEPTION("mkdir failed l (Errno=%d)][ErrStr=%s]", errno, strerror(errno));
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		}
	}

	if (atch_id > 0)  {
		if (account_id < FIRST_ACCOUNT_ID || mail_id <= 0)  {
			EM_DEBUG_EXCEPTION(" account_id[%d], mail_id[%d], atch_id[%d]", account_id, mail_id, atch_id);
			
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		SNPRINTF(buf+strlen(buf), sizeof(buf)-(strlen(buf)+1), "%s%d", DIR_SEPERATOR, atch_id);
		
		if (stat(buf, &sbuf) == 0) {
			if ((sbuf.st_mode & S_IFMT) != S_IFDIR)  {
				EM_DEBUG_EXCEPTION(" a object which isn't directory aleady exists");
				
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		}
		else  {
			if (mkdir(buf, DIRECTORY_PERMISSION) != 0) {
				EM_DEBUG_EXCEPTION(" mkdir failed [%s]", buf);
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		}
	}

	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(profile_emcore_save_create_dir);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_copy_file(char *src_file, char *dst_file, int sync_status, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("src_file[%s], dst_file[%s], err_code[%p]", src_file, dst_file, err_code);
	EM_DEBUG_LOG("Using the fsync function");
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	struct stat st_buf;

	int fp_src = 0;
	int fp_dst = 0;
	int nread = 0;
	int nwritten = 0;
	char *buf =  NULL;
	int buf_size = 0;
	
	if (!src_file || !dst_file)  {
		EM_DEBUG_EXCEPTION("src_file[%p], dst_file[%p]", src_file, dst_file);
		
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (stat(src_file, &st_buf) < 0) {
		EM_DEBUG_EXCEPTION("stat(\"%s\") failed...", src_file);
		
		error = EMAIL_ERROR_SYSTEM_FAILURE;		/* EMAIL_ERROR_INVALID_PATH; */
		goto FINISH_OFF;
	}
	
	buf_size =  st_buf.st_size;
	EM_DEBUG_LOG(">>>> File Size [ %d ]", buf_size);
	buf = (char *)calloc(1, buf_size+1);

	if (!buf) {
		EM_DEBUG_EXCEPTION(">>> Memory cannot be allocated");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (buf) {
		if (!(fp_src = open(src_file, O_RDONLY))) {
			EM_DEBUG_EXCEPTION(">>>> Source Fail open %s Failed [ %d ] - Error [ %s ]", src_file, errno, strerror(errno));
			error = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;				
		}

		if (!(fp_dst = open(dst_file, O_CREAT | O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH))) {
			EM_DEBUG_EXCEPTION(">>>> Destination Fail open %s Failed [ %d ] - Error [ %s ]", dst_file, errno, strerror(errno));
			error = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;				
		}

		while ((nread = read(fp_src, buf, buf_size)) > 0) {
			if (nread > 0 && nread <= buf_size)  {		
				EM_DEBUG_LOG("Nread Value [%d]", nread);
				if ((nwritten = write(fp_dst, buf, nread)) != nread) {
					EM_DEBUG_EXCEPTION("fwrite failed...[%d] : [%s]", errno, strerror(errno));
					error = EMAIL_ERROR_UNKNOWN;
					goto FINISH_OFF;
				}
				EM_DEBUG_LOG("NWRITTEN [%d]", nwritten);
			}
		}
	}

	ret = true;
	
FINISH_OFF:
	if (fp_src)
		close(fp_src);			
	
	if (fp_dst) {
		if (sync_status) {
			EM_DEBUG_LOG("Before fsync");
			fsync(fp_dst);
		}
		close(fp_dst);			
	}
	EM_SAFE_FREE(buf);
	if (nread < 0 || error == EMAIL_ERROR_UNKNOWN)
		remove(dst_file);
	
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
/* create Directory if user has deleted [deepam.p@samsung.com] */
INTERNAL_FUNC void emstorage_create_dir_if_delete()
{
	EM_DEBUG_FUNC_BEGIN();

	mkdir(EMAILPATH, DIRECTORY_PERMISSION);
	mkdir(DATA_PATH, DIRECTORY_PERMISSION);

	mkdir(MAILHOME, DIRECTORY_PERMISSION);
	
	SNPRINTF(g_db_path, sizeof(g_db_path), "%s/%s", MAILHOME, MAILTEMP);
	mkdir(g_db_path, DIRECTORY_PERMISSION);
	
	/* _delete_temp_file(g_db_path); */
	EM_DEBUG_FUNC_END();
}
static int _get_temp_file_name(char **filename, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("filename[%p], err_code[%p]", filename, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	if (filename == NULL) {
		EM_DEBUG_EXCEPTION(" filename[%p]", filename);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	char tempname[512] = {0x00, };
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);
	
	SNPRINTF(tempname, sizeof(tempname), "%s%c%s%c%d", MAILHOME, '/', MAILTEMP, '/', rand());
	
	char *p = EM_SAFE_STRDUP(tempname);
	if (p == NULL)  {
		EM_DEBUG_EXCEPTION(" strdup failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	*filename = p;
	
	ret = true;

FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_add_content_type(char *file_path, char *char_set, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("File path [ %p ]  Character Set [ %p ] err_code [ %p]", file_path, char_set, err_code);

	EM_IF_NULL_RETURN_VALUE(file_path, false);
	EM_IF_NULL_RETURN_VALUE(char_set, false);
	EM_IF_NULL_RETURN_VALUE(err_code, false);

	char *buf =  NULL;
	char *buf1 = NULL;
	struct stat st_buf;
	int buf_size = 0;
	char *low_char_set = NULL;
	char *match_str = NULL;
	int nwritten = 0;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int data_count_to_written = 0;
	char *temp_file_name = NULL;
	int err = 0;
	
	FILE* fp_src = NULL;
	FILE* fp_dest = NULL;
	int nread = 0;
	

	if (stat(file_path, &st_buf) < 0) {
		EM_DEBUG_EXCEPTION(" stat(\"%s\") failed...", file_path);
		
		error = EMAIL_ERROR_SYSTEM_FAILURE;		/* EMAIL_ERROR_INVALID_PATH; */
		goto FINISH_OFF;
	}

	buf_size =  st_buf.st_size;

	EM_DEBUG_LOG(">>>> File Size [ %d ] ", buf_size);

	buf = (char *)calloc(1, buf_size+1);

	if (!buf) {
		EM_DEBUG_LOG(">>> Memory cannot be allocated ");
		goto FINISH_OFF;
	}

	if (!(fp_src = fopen(file_path, "rb"))) {
		EM_DEBUG_EXCEPTION(" file_path fopen failed - %s", file_path);
			
			error = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}
				
		if ((nread = fread(buf, 1, buf_size, fp_src)) > 0) {
			if (nread > 0 && nread <= buf_size)  {		
				EM_DEBUG_LOG(">>>> Nread Value [ %d ] ", nread);
				
				/**
				  *   1.Add check for whether content type is there. 
				  *   2. If not based on the character set, Append it in File
				  **/

				low_char_set = calloc(1, strlen(char_set) + strlen(" \" /></head>") +1);
				
				strncat(low_char_set, char_set, strlen(char_set));
				
				EM_DEBUG_LOG(">>>> CHAR SET [ %s ] ", low_char_set);
				
				strncat(low_char_set, " \" /></head>", strlen(" \" /></head>"));
					
				EM_DEBUG_LOG(">>> CHARSET [ %s ] ", low_char_set);

				EM_DEBUG_LOG(">>>>emstorage_add_content_type 1 ");
				
				match_str = strstr(buf, CONTENT_TYPE_DATA);
				EM_DEBUG_LOG(">>>>emstorage_add_content_type 2 ");
				
				if (match_str == NULL) {
					EM_DEBUG_LOG(">>>>emstorage_add_content_type 3 ");
					if (fp_src !=NULL) {
						fclose(fp_src);fp_src = NULL;
					}
				data_count_to_written = strlen(low_char_set)+strlen(CONTENT_DATA)+1;
					EM_DEBUG_LOG(">>>>emstorage_add_content_type 4 ");
				buf1 = (char *)calloc(1, data_count_to_written);
					EM_DEBUG_LOG(">>>>emstorage_add_content_type 5 ");

					if (buf1) {
						EM_DEBUG_LOG(">>>>emstorage_add_content_type 6 ");
					 	strncat(buf1, CONTENT_DATA, strlen(CONTENT_DATA));

						EM_DEBUG_LOG(">>>>> BUF 1 [ %s ] ", buf1);

						strncat(buf1, low_char_set, strlen(low_char_set));

						EM_DEBUG_LOG(">>>> HTML TAG DATA  [ %s ] ", buf1);


					/* 1. Create a temporary file name */
					if (!_get_temp_file_name(&temp_file_name, &err)) {
							EM_DEBUG_EXCEPTION(" emcore_get_temp_file_name failed - %d", err);
							if (err_code != NULL) *err_code = err;
							EM_SAFE_FREE(temp_file_name);
							goto FINISH_OFF;
					}
					EM_DEBUG_LOG(">>>>>>> TEMP APPEND FILE PATH [ %s ] ", temp_file_name);
					
					/* Open the Temp file in Append mode */
					if (!(fp_dest = fopen(temp_file_name, "ab"))) {
						EM_DEBUG_EXCEPTION(" fopen failed - %s", temp_file_name);
						error = EMAIL_ERROR_SYSTEM_FAILURE;
						goto FINISH_OFF;
					}

					/* 2. write the Latest data */
					nwritten = fwrite(buf1, data_count_to_written-1, 1, fp_dest);

					if (nwritten > 0) {
						EM_DEBUG_LOG(" Latest Data  : [%d ] bytes written ", nwritten);
						nwritten = 0;
						/* 3. Append old data */
						nwritten = fwrite(buf, nread-1, 1, fp_dest);

						if (nwritten <= 0) {
							EM_DEBUG_EXCEPTION(" Error Occured while writing Old data : [%d ] bytes written ", nwritten);
							error = EMAIL_ERROR_SYSTEM_FAILURE;
							goto FINISH_OFF;
						}
						else {
							EM_DEBUG_LOG(">>>> OLD data appended [ %d ] ", nwritten);

							if (!emstorage_move_file(temp_file_name, file_path, false, &err)) {
								EM_DEBUG_EXCEPTION(" emstorage_move_file failed - %d", err);
								goto FINISH_OFF;
							}
						}
							
					}
					else {
						EM_DEBUG_EXCEPTION(" Error Occured while writing New data : [%d ] bytes written ", nwritten);
						error = EMAIL_ERROR_SYSTEM_FAILURE;
						goto FINISH_OFF;
					}
							
					}
		
				}
				EM_DEBUG_LOG(">>>>emstorage_add_content_type 15 ");

	
			}
		}

	ret = true;
FINISH_OFF:

	EM_SAFE_FREE(buf);
	EM_SAFE_FREE(buf1);
	EM_SAFE_FREE(low_char_set);

	if (fp_src != NULL) {
		fclose(fp_src);
		fp_src = NULL;
	}
	
	if (fp_dest != NULL) {
		fclose(fp_dest);
		fp_dest = NULL;
	}
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}

INTERNAL_FUNC int emstorage_move_file(char *src_file, char *dst_file, int sync_status, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("src_file[%p], dst_file[%p], err_code[%p]", src_file, dst_file, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	if (src_file == NULL || dst_file == NULL)  {
		EM_DEBUG_EXCEPTION("src_file[%p], dst_file[%p]", src_file, dst_file);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	EM_DEBUG_LOG("src_file[%s], dst_file[%s]", src_file, dst_file);

	if (strcmp(src_file, dst_file) != 0) {
		if (rename(src_file, dst_file) != 0) {
			if (errno == EXDEV)  {	/* oldpath and newpath are not on the same mounted file system.  (Linux permits a file system to be mounted at multiple points,  but  rename() */
				/*  does not work across different mount points, even if the same file system is mounted on both.)	 */
				EM_DEBUG_LOG("oldpath and newpath are not on the same mounted file system.");
				if (!emstorage_copy_file(src_file, dst_file, sync_status, &error)) {
					EM_DEBUG_EXCEPTION("emstorage_copy_file failed - %d", error);
					goto FINISH_OFF;
				}
				remove(src_file);
				EM_DEBUG_LOG("src[%s] removed", src_file);
		
			}
			else  {
				if (errno == ENOENT)  {
					struct stat temp_file_stat;
					if (stat(src_file, &temp_file_stat) < 0)
						EM_DEBUG_EXCEPTION("no src file found [%s]", src_file);
					if (stat(dst_file, &temp_file_stat) < 0)
						EM_DEBUG_EXCEPTION("no dst file found [%s]", src_file);

					EM_DEBUG_EXCEPTION("no file found [%d]", errno);
					error = EMAIL_ERROR_FILE_NOT_FOUND;
					goto FINISH_OFF;
					
				}
				else  {
					EM_DEBUG_EXCEPTION("rename failed [%d]", errno);
					error = EMAIL_ERROR_SYSTEM_FAILURE;
					goto FINISH_OFF;
				}
			}
		}
	}
	else {
		EM_DEBUG_LOG("src[%s] = dst[%d]", src_file, dst_file);
	}
	
	ret = true;

FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_file(char *src_file, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("src_file[%p], err_code[%p]", src_file, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	if (src_file == NULL) {
		EM_DEBUG_EXCEPTION(" src_file[%p]", src_file);
		
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (remove(src_file) != 0) {
		if (errno != ENOENT) {
			EM_DEBUG_EXCEPTION(" remove failed - %d", errno);
			
			error = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}
		else {
			EM_DEBUG_EXCEPTION(" no file found...");
			
			error = EMAIL_ERROR_FILE_NOT_FOUND;
		}
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_delete_dir(char *src_dir, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("src_dir[%p], err_code[%p]", src_dir, err_code);
	
	if (src_dir == NULL) {
		EM_DEBUG_EXCEPTION(" src_dir[%p]", src_dir);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int error = EMAIL_ERROR_NONE;
	
	DIR *dirp;
	struct dirent *dp;
	struct stat sbuf;
	char buf[512];
	
	dirp = opendir(src_dir);
	
	if (dirp == NULL)  {
		if (errno == ENOENT)  {
			EM_DEBUG_EXCEPTION("directory[%s] does not exist...", src_dir);
			if (err_code != NULL)
				*err_code = EMAIL_ERROR_SYSTEM_FAILURE;
			return true;
		}
		else  {
			EM_DEBUG_EXCEPTION("opendir failed - %d", errno);
			if (err_code != NULL)
				*err_code = EMAIL_ERROR_SYSTEM_FAILURE;
			return false;
		}
	}

	while ((dp=readdir(dirp)))  {
		if (strncmp(dp->d_name, ".", strlen(".")) == 0 || strncmp(dp->d_name, "..", strlen("..")) == 0)
			continue;
		
		SNPRINTF(buf, sizeof(buf), "%s/%s", src_dir, dp->d_name);
		
		if (lstat(buf, &sbuf) == 0 || stat(buf, &sbuf) == 0) {
			/*  check directory */
			if ((sbuf.st_mode & S_IFMT) == S_IFDIR)  {	/*  directory */
				/*  recursive call */
				if (!emstorage_delete_dir(buf, &error)) {
					closedir(dirp); 
					if (err_code != NULL)
						*err_code = error;
					return false;
				}
			}
			else  {	/*  file */
				if (remove(buf) < 0)  {
					EM_DEBUG_EXCEPTION("remove failed - %s", buf);
					closedir(dirp);
					if (err_code != NULL)
						*err_code = EMAIL_ERROR_SYSTEM_FAILURE;
					return false;
				}
			}
		}
		else 
			EM_DEBUG_EXCEPTION("content does not exist...");
	}
	
	closedir(dirp);
	
	EM_DEBUG_LOG("remove direcotory [%s]", src_dir);
	
	/* EM_DEBUG_FUNC_BEGIN(); */
	
	if (remove(src_dir) < 0)  {
		EM_DEBUG_EXCEPTION("remove failed [%s]", src_dir);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_SYSTEM_FAILURE;
		return false;
	}
	
	if (err_code != NULL)
		*err_code = error;
	
	return true;
}

/* faizan.h@samsung.com */
INTERNAL_FUNC int emstorage_update_server_uid(char *old_server_uid, char *new_server_uid, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("new_server_uid[%s], old_server_uid[%s]", new_server_uid, old_server_uid);
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	int transaction = true;
	
	if (!old_server_uid || !new_server_uid) {	
		EM_DEBUG_EXCEPTION("Invalid parameters");
		error = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		 "UPDATE mail_tbl SET server_mail_id=\'%s\' WHERE server_mail_id=%s ", new_server_uid, old_server_uid);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	ret = true;
	  
FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
	
}

INTERNAL_FUNC int emstorage_update_read_mail_uid(int mail_id, char *new_server_uid, char *mbox_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], new_server_uid[%s], mbox_name[%s]", mail_id, new_server_uid, mbox_name);

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	int transaction = true;
	
	if (!mail_id || !new_server_uid || !mbox_name)  {		
		EM_DEBUG_EXCEPTION("Invalid parameters");
		error = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		 "UPDATE mail_read_mail_uid_tbl SET s_uid=\'%s\', mailbox_id=\'%s\', mailbox_name=\'%s\' WHERE local_uid=%d ", new_server_uid, mbox_name, mbox_name, mail_id);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	ret	= true;
	  
FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
	
}


int emstorage_get_latest_unread_mailid(int account_id, int *mail_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	if ((!mail_id) ||(account_id <= 0 &&  account_id != -1)) {
		EM_DEBUG_EXCEPTION(" mail_id[%p], account_id[%d] ", mail_id, account_id);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}	

	int ret = false;
	int rc = -1;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	int count = 0;
	int mailid = 0;
	int transaction = false;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	if (account_id == -1)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mail_id FROM mail_tbl WHERE flags_seen_field = 0 ORDER BY mail_id DESC");
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mail_id FROM mail_tbl WHERE account_id = %d AND flags_seen_field = 0 ORDER BY mail_id DESC", account_id);
		
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("  sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	char **result;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	sqlite3_free_table(result);
	if (count == 0)  {
		EM_DEBUG_EXCEPTION("no Mails found...");
		ret = false;
		error= EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
 	
		_get_stmt_field_data_int(hStmt, &mailid, 0);

	ret = true;

FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

int setting_system_command(const char *command)

 {
 	int pid = 0, status = 0;
	char *const environ[] = { NULL };

	if (command == 0)
		return 1;

	pid = fork();

	if (pid == -1)
		return -1;

	if (pid == 0) {
		char *argv[4];
		
		argv[0] = "sh";
		argv[1] = "-c";
		argv[2] = (char *)command;
		argv[3] = 0;

		execve("/bin/sh", argv, environ);
		abort();
	}
	do{
	 	if (waitpid(pid, &status, 0) == -1) {
			if (errno != EINTR)
				return -1;
		}
		else {
			return status;
		}
	} while (1);

	return 0;
}


INTERNAL_FUNC int emstorage_mail_get_total_diskspace_usage(unsigned long *total_usage,  int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("total_usage[%p],  transaction[%d], err_code[%p]", total_usage, transaction, err_code);

	if (!total_usage) {
		EM_DEBUG_EXCEPTION("total_usage[%p]", total_usage);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int   ret = false;
	int   error = EMAIL_ERROR_NONE;
	char  syscmd[256] = {0, };
	char  line[256] = {0, };
	char *line_from_file = NULL;
	FILE *fp = NULL;
	unsigned long total_diskusage = 0;

	SNPRINTF(syscmd, sizeof(syscmd), "touch %s", SETTING_MEMORY_TEMP_FILE_PATH);
	if (setting_system_command(syscmd) == -1) {
		EM_DEBUG_EXCEPTION("emstorage_mail_get_total_diskspace_usage : [Setting > Memory] System Command [%s] is failed", syscmd);

		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;		
	}

	SNPRINTF(syscmd, sizeof(syscmd), "du -hsk %s > %s", EMAILPATH, SETTING_MEMORY_TEMP_FILE_PATH);
	EM_DEBUG_LOG(" cmd : %s", syscmd);
	if (setting_system_command(syscmd) == -1) {
		EM_DEBUG_EXCEPTION("emstorage_mail_get_total_diskspace_usage : Setting > Memory] System Command [%s] is failed", syscmd);
		
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;	
	}

	fp = fopen(SETTING_MEMORY_TEMP_FILE_PATH, "r");
	if (fp == NULL) {
		perror(SETTING_MEMORY_TEMP_FILE_PATH);
		
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;	
	}
 
 	line_from_file = fgets(line, sizeof(line), fp);

 	if(line_from_file == NULL) {
 		EM_DEBUG_EXCEPTION("fgets failed");
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
 	}
 	total_diskusage = strtoul(line, NULL, 10);
 
 	fclose(fp);

 	memset(syscmd, 0, sizeof(syscmd));
 	SNPRINTF(syscmd, sizeof(syscmd), "rm -f %s", SETTING_MEMORY_TEMP_FILE_PATH);
 	if (setting_system_command(syscmd) == -1) {
  		EM_DEBUG_EXCEPTION("emstorage_mail_get_total_diskspace_usage :  [Setting > Memory] System Command [%s] is failed", syscmd);
  		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;	
 	}

 	EM_DEBUG_LOG("[Setting > Memory] @@@@@ Size of Directory [%s] is %ld KB", EMAILPATH, total_diskusage);

	ret = true;

FINISH_OFF:	
 	if (err_code != NULL)
		*err_code = error;

	if (ret)
		*total_usage = total_diskusage;
	else
		*total_usage = 0;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_test(int mail_id, int account_id, char *full_address_to, char *full_address_cc, char *full_address_bcc, int *err_code)
{
	DB_STMT hStmt = NULL;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int rc = 0;
	char sql_query_string[QUERY_SIZE] = {0, };

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_tbl VALUES "
		"( ?" /*  mail_id */
		", ?" /*  account_id */
		", ?" /*  mailbox_name */
		", ?" /*  mail_size */
		", ?" /*  server_mail_status */
		", ?" /*  server_mailbox_name */
		", ?" /*  server_mail_id */
		", ?" /*  reference_mail_id */
		", ?" /*  full_address_from */
		", ?" /*  full_address_reply */
		", ?" /*  full_address_to */
		", ?" /*  full_address_cc */
		", ?" /*  full_address_bcc */
		", ?" /*  full_address_return */
		", ?" /*  subject */
		", ?" /*  body_download_status */
		", ?" /*  file_path_plain */
		", ?" /*  file_path_html */
		", ?" /*  date_time */
		", ?" /*  flags_seen_field */
		", ?" /*  flags_deleted_field */
		", ?" /*  flags_flagged_field */
		", ?" /*  flags_answered_field */
		", ?" /*  flags_recent_field */
		", ?" /*  flags_draft_field */
		", ?" /*  flags_forwarded_field */
		", ?" /*  DRM_status */
		", ?" /*  priority */
		", ?" /*  save_status */
		", ?" /*  lock_status */
		", ?" /*  message_id */
		", ?" /*  report_status */
		", ?" /*  email_address_sender */
		", ?" /*  email_address_recipient */
		", ?" /*  attachment_count */
		", ?" /*  inline_content_count */
		", ?" /*  preview_text */
		", ?" /*  thread_id */
		", ?" /*  mailbox_type */
		", ?" /*  alias_sender */
		", ?" /*  alias_recipient */
		", ?" /*  thread_item_count */
		", ?" /*  meeting_request_status */
		", ?" /*  message_class */
		", ?" /*  digest_type */
		", ?" /*  smime_type */
		")");

	int transaction = true;
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
 
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bind_stmt_field_data_int(hStmt, MAIL_ID_IDX_IN_MAIL_TBL, mail_id);
	_bind_stmt_field_data_int(hStmt, ACCOUNT_ID_IDX_IN_MAIL_TBL, account_id);
	_bind_stmt_field_data_string(hStmt, MAILBOX_NAME_IDX_IN_MAIL_TBL, "OUTBOX", 0, MAILBOX_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, MAILBOX_TYPE_IDX_IN_MAIL_TBL, EMAIL_MAILBOX_TYPE_OUTBOX);
	_bind_stmt_field_data_string(hStmt, SUBJECT_IDX_IN_MAIL_TBL, "save test - long", 1, SUBJECT_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, DATETIME_IDX_IN_MAIL_TBL, "20100316052908", 0, DATETIME_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, SERVER_MAIL_STATUS_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_string(hStmt, SERVER_MAILBOX_NAME_IDX_IN_MAIL_TBL, "", 0, SERVER_MAILBOX_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, SERVER_MAIL_ID_IDX_IN_MAIL_TBL, "", 0, SERVER_MAIL_ID_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, MESSAGE_ID_IDX_IN_MAIL_TBL, "", 0, MESSAGE_ID_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, REFERENCE_ID_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_string(hStmt, FULL_ADDRESS_FROM_IDX_IN_MAIL_TBL, "<test08@streaming.s3glab.net>", 1, FROM_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FULL_ADDRESS_REPLY_IDX_IN_MAIL_TBL, "", 1, REPLY_TO_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FULL_ADDRESS_TO_IDX_IN_MAIL_TBL, full_address_to, 1, TO_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FULL_ADDRESS_CC_IDX_IN_MAIL_TBL, full_address_cc, 1, CC_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FULL_ADDRESS_BCC_IDX_IN_MAIL_TBL, full_address_bcc, 1, BCC_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FULL_ADDRESS_RETURN_IDX_IN_MAIL_TBL, "", 1, RETURN_PATH_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, EMAIL_ADDRESS_SENDER_IDX_IN_MAIL_TBL, "<sender_name@sender_host.com>", 1, FROM_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, EMAIL_ADDRESS_RECIPIENT_IDX_IN_MAIL_TBL, "<recipient_name@recipient_host.com>", 1, TO_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, ALIAS_SENDER_IDX_IN_MAIL_TBL, "send_alias", 1, FROM_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, ALIAS_RECIPIENT_IDX_IN_MAIL_TBL, "recipient_alias", 1, TO_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, BODY_DOWNLOAD_STATUS_IDX_IN_MAIL_TBL, 1);
	_bind_stmt_field_data_string(hStmt, FILE_PATH_PLAIN_IDX_IN_MAIL_TBL, "/opt/system/rsr/email/.emfdata/7/348/UTF-8", 0, TEXT_1_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FILE_PATH_HTML_IDX_IN_MAIL_TBL, "", 0, TEXT_2_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, MAIL_SIZE_IDX_IN_MAIL_TBL, 4);
	_bind_stmt_field_data_char(hStmt, FLAGS_SEEN_FIELD_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_char(hStmt, FLAGS_DELETED_FIELD_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_char(hStmt, FLAGS_FLAGGED_FIELD_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_char(hStmt, FLAGS_ANSWERED_FIELD_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_char(hStmt, FLAGS_RECENT_FIELD_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_char(hStmt, FLAGS_DRAFT_FIELD_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_char(hStmt, FLAGS_FORWARDED_FIELD_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, DRM_STATUS_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, PRIORITY_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, SAVE_STATUS_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, LOCK_STATUS_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, REPORT_STATUS_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, ATTACHMENT_COUNT_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, INLINE_CONTENT_COUNT_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, ATTACHMENT_COUNT_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, THREAD_ID_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, THREAD_ITEM_COUNT_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_string(hStmt, PREVIEW_TEXT_IDX_IN_MAIL_TBL, "preview body", 1, PREVIEWBODY_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, MEETING_REQUEST_STATUS_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, MESSAGE_CLASS_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, DIGEST_TYPE_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, SMIME_TYPE_IDX_IN_MAIL_TBL, 0);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	ret = true;

FINISH_OFF:
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
 
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_max_mail_count()
{
	return EMAIL_MAIL_MAX_COUNT;
}

INTERNAL_FUNC int emstorage_get_thread_id_of_thread_mails(emstorage_mail_tbl_t *mail_tbl, int *thread_id, int *result_latest_mail_id_in_thread, int *thread_item_count)
{
	EM_DEBUG_FUNC_BEGIN("mail_tbl [%p], thread_id [%p], result_latest_mail_id_in_thread [%p], thread_item_count [%p]", mail_tbl, thread_id, result_latest_mail_id_in_thread, thread_item_count);
	EM_PROFILE_BEGIN(profile_emstorage_get_thread_id_of_thread_mails);
	int      rc = 0, query_size = 0, query_size_account = 0;
	int      account_id = 0;
	int      err_code = EMAIL_ERROR_NONE;
	int      count = 0, result_thread_id = -1, latest_mail_id_in_thread = -1;
	time_t   latest_date_time = 0;
	time_t   date_time = 0;
	char    *mailbox_name = NULL, *subject = NULL;
	char    *sql_query_string = NULL, *sql_account = NULL;
	char    *sql_format = "SELECT thread_id, date_time, mail_id FROM mail_tbl WHERE subject like \'%%%q\' AND mailbox_id = %d";
	char    *sql_format_account = " AND account_id = %d ";
	char    *sql_format_order_by = " ORDER BY date_time DESC ";
	char   **result = NULL;
	char     stripped_subject[1025];
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EM_IF_NULL_RETURN_VALUE(mail_tbl, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(thread_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(result_latest_mail_id_in_thread, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(thread_item_count, EMAIL_ERROR_INVALID_PARAM);

	account_id   = mail_tbl->account_id;
	mailbox_name = mail_tbl->mailbox_name;
	subject      = mail_tbl->subject;
	date_time    = mail_tbl->date_time;
	
	EM_DEBUG_LOG("subject : %s", subject);

	if (em_find_pos_stripped_subject_for_thread_view(subject, stripped_subject) != EMAIL_ERROR_NONE)	{
		EM_DEBUG_EXCEPTION("em_find_pos_stripped_subject_for_thread_view  is failed");
		err_code =  EMAIL_ERROR_UNKNOWN;
		result_thread_id = -1;
		goto FINISH_OFF;
	}

	if (strlen(stripped_subject) < 2) {
		result_thread_id = -1;
		goto FINISH_OFF;
	}
	
	EM_DEBUG_LOG("em_find_pos_stripped_subject_for_thread_view returns[len = %d] = %s", strlen(stripped_subject), stripped_subject);
	
	if (account_id > 0) 	{
		query_size_account = 3 + strlen(sql_format_account);
		sql_account = malloc(query_size_account);
		if (sql_account == NULL) {
			EM_DEBUG_EXCEPTION("malloc for sql_account  is failed %d", query_size_account);
			err_code =  EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		snprintf(sql_account, query_size_account, sql_format_account, account_id);
	}
	
	query_size = strlen(sql_format) + strlen(stripped_subject) + 50 + query_size_account + strlen(sql_format_order_by); /*  + query_size_mailbox; */
	
	sql_query_string = malloc(query_size);
	
	if (sql_query_string == NULL) {
		EM_DEBUG_EXCEPTION("malloc for sql  is failed %d", query_size);
		err_code =  EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	sqlite3_snprintf(query_size, sql_query_string, sql_format, stripped_subject, mail_tbl->mailbox_id);

	if (account_id > 0)
		strcat(sql_query_string, sql_account);

	strcat(sql_query_string, sql_format_order_by);
	strcat(sql_query_string, ";");

	EM_DEBUG_LOG("Query : %s", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err_code = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG("Result rows count : %d", count);

	if (count == 0)
		result_thread_id = -1;
	else {
		_get_table_field_data_int   (result, &result_thread_id, 3);
		_get_table_field_data_time_t(result, &latest_date_time, 4);
		_get_table_field_data_int   (result, &latest_mail_id_in_thread, 5);

		if (latest_date_time < mail_tbl->date_time)
			*result_latest_mail_id_in_thread = latest_mail_id_in_thread;
		else 
			*result_latest_mail_id_in_thread = mail_tbl->mail_id;
		EM_DEBUG_LOG("latest_mail_id_in_thread [%d], mail_id [%d]", latest_mail_id_in_thread, mail_tbl->mail_id);
	}

FINISH_OFF:
	*thread_id = result_thread_id;
	*thread_item_count = count;
	
	EM_DEBUG_LOG("Result thread id : %d", *thread_id);
	EM_DEBUG_LOG("Result count : %d", *thread_item_count);
	EM_DEBUG_LOG("err_code : %d", err_code);

	EM_SAFE_FREE(sql_account);
	EM_SAFE_FREE(sql_query_string);

	sqlite3_free_table(result);
	
	EM_PROFILE_END(profile_emstorage_get_thread_id_of_thread_mails);
	
	return err_code;
}


INTERNAL_FUNC int emstorage_get_thread_information(int thread_id, emstorage_mail_tbl_t** mail_tbl, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int count = 0, ret = false;
	int error = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t *p_data_tbl = NULL;
	char conditional_clause[QUERY_SIZE] = {0, };

	EM_IF_NULL_RETURN_VALUE(mail_tbl, false);

	SNPRINTF(conditional_clause, QUERY_SIZE, "WHERE thread_id = %d AND thread_item_count > 0", thread_id);
	EM_DEBUG_LOG("conditional_clause [%s]", conditional_clause);

	if(!emstorage_query_mail_tbl(conditional_clause, transaction, &p_data_tbl, &count, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_tbl failed [%d]", error);
		goto FINISH_OFF;
	}

	if(p_data_tbl)
		EM_DEBUG_LOG("thread_id : %d, thread_item_count : %d", p_data_tbl[0].thread_id, p_data_tbl[0].thread_item_count);
	
	ret = true;
	
FINISH_OFF:
	if (ret == true) 
		*mail_tbl = p_data_tbl;
	else if (p_data_tbl != NULL)
		emstorage_free_mail(&p_data_tbl, 1, NULL);
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_sender_list(int account_id, const char *mailbox_name, int search_type, const char *search_value, email_sort_type_t sorting, email_sender_list_t** sender_list, int *sender_count,  int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mailbox_name [%p], search_type [%d], search_value [%p], sorting [%d], sender_list[%p], sender_count[%p] err_code[%p]"
		, account_id , mailbox_name , search_type , search_value , sorting , sender_list, sender_count, err_code);
	
	if ((!sender_list) ||(!sender_count)) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	int count = 0;
	int i, col_index = 0;
	int read_count = 0;
	email_sender_list_t *p_sender_list = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char **result = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"SELECT email_address_sender, alias_sender, COUNT(email_address_sender), SUM(flags_seen_field = 1) "
		"FROM mail_tbl ");

	/*  mailbox_name */
	if (mailbox_name)
		SNPRINTF(sql_query_string + strlen(sql_query_string), QUERY_SIZE-(strlen(sql_query_string)+1), " WHERE UPPER(mailbox_name) = UPPER('%s') ", mailbox_name);
	else	/*  NULL  means all mailbox_name. but except for trash(3), spambox(5), all emails(for GMail, 7) */
		SNPRINTF(sql_query_string + strlen(sql_query_string), QUERY_SIZE-(strlen(sql_query_string)+1), " WHERE mailbox_type not in (3, 5, 7, 8) ");

	/*  account id */
	/*  '0' (ALL_ACCOUNT) means all account */
	if (account_id > ALL_ACCOUNT)
		SNPRINTF(sql_query_string + strlen(sql_query_string), QUERY_SIZE-(strlen(sql_query_string)+1), " AND account_id = %d ", account_id);

	if (search_value) {
		switch (search_type) {
			case EMAIL_SEARCH_FILTER_SUBJECT:
				SNPRINTF(sql_query_string + strlen(sql_query_string), QUERY_SIZE-(strlen(sql_query_string)+1),
					" AND (UPPER(subject) LIKE UPPER(\'%%%%%s%%%%\')) ", search_value);
				break;
			case EMAIL_SEARCH_FILTER_SENDER:
				SNPRINTF(sql_query_string + strlen(sql_query_string), QUERY_SIZE-(strlen(sql_query_string)+1),
					" AND  ((UPPER(full_address_from) LIKE UPPER(\'%%%%%s%%%%\')) "
					") ", search_value);
				break;
			case EMAIL_SEARCH_FILTER_RECIPIENT:
				SNPRINTF(sql_query_string + strlen(sql_query_string), QUERY_SIZE-(strlen(sql_query_string)+1),
					" AND ((UPPER(full_address_to) LIKE UPPER(\'%%%%%s%%%%\')) "	
					"	OR (UPPER(full_address_cc) LIKE UPPER(\'%%%%%s%%%%\')) "
					"	OR (UPPER(full_address_bcc) LIKE UPPER(\'%%%%%s%%%%\')) "
					") ", search_value, search_value, search_value);
				break;
			case EMAIL_SEARCH_FILTER_ALL:
				SNPRINTF(sql_query_string + strlen(sql_query_string), QUERY_SIZE-(strlen(sql_query_string)+1),
					" AND (UPPER(subject) LIKE UPPER(\'%%%%%s%%%%\') "
					" 	OR (((UPPER(full_address_from) LIKE UPPER(\'%%%%%s%%%%\')) "
					"			OR (UPPER(full_address_to) LIKE UPPER(\'%%%%%s%%%%\')) "
					"			OR (UPPER(full_address_cc) LIKE UPPER(\'%%%%%s%%%%\')) "
					"			OR (UPPER(full_address_bcc) LIKE UPPER(\'%%%%%s%%%%\')) "
					"		) "
					"	)"
					")", search_value, search_value, search_value, search_value, search_value);
				break;
		}
	}


	/*  sorting option is not available now. The order of sender list is ascending order by display name */
	SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1),
		"GROUP BY email_address_sender "
		"ORDER BY UPPER(alias_sender) ");

	EM_DEBUG_LOG("query[%s]", sql_query_string);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG("Count of Sender [%d]", count);
	
	if (!(p_sender_list = (email_sender_list_t*)em_malloc(sizeof(email_sender_list_t) * count))) {
		EM_DEBUG_EXCEPTION("em_malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	col_index = 4;

	EM_DEBUG_LOG(">>>> DATA ASSIGN START >>");	
	for (i = 0; i < count; i++)  {
		_get_table_field_data_string(result, &(p_sender_list[i].address), 1, col_index++);
		_get_table_field_data_string(result, &(p_sender_list[i].display_name), 1, col_index++);
		_get_table_field_data_int(result, &(p_sender_list[i].total_count), col_index++);
		_get_table_field_data_int(result, &(read_count), col_index++);
		p_sender_list[i].unread_count = p_sender_list[i].total_count - read_count;		/*  unread count = total - read		 */
	}
	EM_DEBUG_LOG(">>>> DATA ASSIGN END [count : %d] >>", count);

	sqlite3_free_table(result);
	result = NULL;

	ret = true;
	
FINISH_OFF:
	if (ret == true)  {
		*sender_list = p_sender_list;
		*sender_count = count;
		EM_DEBUG_LOG(">>>> COUNT : %d >>", count);
	}
	else if (p_sender_list != NULL)
		emstorage_free_sender_list(&p_sender_list, count);
		
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_free_sender_list(email_sender_list_t **sender_list, int count)
{
	EM_DEBUG_FUNC_BEGIN("sender_list[%p], count[%d]", sender_list, count);

	int err = EMAIL_ERROR_NONE;
	
	if (count > 0)  {
		if (!sender_list || !*sender_list)  {
			EM_DEBUG_EXCEPTION("sender_list[%p], count[%d]", sender_list, count);
			err = EMAIL_ERROR_INVALID_PARAM;
			return err;
		}
		
		email_sender_list_t* p = *sender_list;
		int i = 0;
		
		for (; i < count; i++)  {
			EM_SAFE_FREE(p[i].address);
			EM_SAFE_FREE(p[i].display_name);
		}
		
		EM_SAFE_FREE(p); 
		*sender_list = NULL;
	}	
	
	return err;
}


INTERNAL_FUNC int emstorage_free_address_info_list(email_address_info_list_t **address_info_list)
{
	EM_DEBUG_FUNC_BEGIN("address_info_list[%p]", address_info_list);

	int err = EMAIL_ERROR_NONE;
	email_address_info_t *p_address_info = NULL;
	GList *list = NULL;
	GList *node = NULL;
	int i = 0;
	
	if (!address_info_list || !*address_info_list)  {
		EM_DEBUG_EXCEPTION("address_info_list[%p]", address_info_list);
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	/*  delete GLists */
	for (i = EMAIL_ADDRESS_TYPE_FROM; i <= EMAIL_ADDRESS_TYPE_BCC; i++) {
		switch (i) {
			case EMAIL_ADDRESS_TYPE_FROM:
				list = (*address_info_list)->from;
				break;
			case EMAIL_ADDRESS_TYPE_TO:
				list = (*address_info_list)->to;
				break;
			case EMAIL_ADDRESS_TYPE_CC:
				list = (*address_info_list)->cc;
				break;
			case EMAIL_ADDRESS_TYPE_BCC:
				list = (*address_info_list)->bcc;
				break;
		}

		/*  delete dynamic-allocated memory for each item */
		node = g_list_first(list);
		while (node != NULL) {
			p_address_info = (email_address_info_t*)node->data;
			EM_SAFE_FREE(p_address_info->address);
			EM_SAFE_FREE(p_address_info->display_name);
			EM_SAFE_FREE(node->data);
			
			node = g_list_next(node);
		}
	}

	EM_SAFE_FREE(*address_info_list); 
	*address_info_list = NULL;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

INTERNAL_FUNC int emstorage_add_pbd_activity(email_event_partial_body_thd* local_activity, int *activity_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("local_activity[%p], activity_id[%p], transaction[%d], err_code[%p]", local_activity, activity_id, transaction, err_code);

	if (!local_activity || !activity_id) {
		EM_DEBUG_EXCEPTION("local_activity[%p], transaction[%d], activity_id[%p], err_code[%p]", local_activity, activity_id, transaction, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int i = 0;

	char sql_query_string[QUERY_SIZE] = {0, };
	DB_STMT hStmt = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_partial_body_activity_tbl VALUES "
		"( "
		"? "  /* Account ID */
		",?"  /* Local Mail ID */
		",?"  /* Server mail ID */
		",?"  /* Activity ID */
		",?"  /* Activity type*/
		",?"  /* Mailbox ID*/
		",?"  /* Mailbox name*/
		") ");

	char *sql = "SELECT max(rowid) FROM mail_partial_body_activity_tbl;";
	char **result = NULL;


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL==result[1]) rc = 1;
	else rc = atoi(result[1])+1;
	sqlite3_free_table(result);
	result = NULL;

	*activity_id = local_activity->activity_id = rc;

	EM_DEBUG_LOG(">>>>> ACTIVITY ID [ %d ], MAIL ID [ %d ], ACTIVITY TYPE [ %d ], SERVER MAIL ID [ %lu ]", \
		local_activity->activity_id, local_activity->mail_id, local_activity->activity_type, local_activity->server_mail_id);

	if (local_activity->mailbox_id)
		EM_DEBUG_LOG(" MAILBOX ID [ %d ]", local_activity->mailbox_id);
	
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG(">>>> SQL STMT [ %s ]", sql_query_string);
	
	_bind_stmt_field_data_int(hStmt, i++, local_activity->account_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->mail_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->server_mail_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->activity_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->activity_type);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->mailbox_id);
	_bind_stmt_field_data_string(hStmt, i++ , (char *)local_activity->mailbox_name, 0, 3999);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);

	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d, errmsg = %s.", rc, sqlite3_errmsg(local_db_handle)));
	
	ret = true;

FINISH_OFF:

	if (hStmt != NULL) {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	 EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	 if (err_code != NULL)
		 *err_code = error;

	 EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_pbd_mailbox_list(int account_id, int **mailbox_list, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_list[%p], count[%p] err_code[%p]", account_id, mailbox_list, count, err_code);

	if (account_id < FIRST_ACCOUNT_ID || NULL == &mailbox_list || NULL == count) {
		EM_DEBUG_EXCEPTION("account_id[%d], mailbox_list[%p], count[%p] err_code[%p]", account_id, mailbox_list, count, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char **result;
	int i = 0, rc = -1;
	int *mbox_list = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(distinct mailbox_id) FROM mail_partial_body_activity_tbl WHERE account_id = %d order by mailbox_id", account_id);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);

	if (!*count) {
		EM_DEBUG_EXCEPTION(" no mailbox_name found...");
		error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("Mailbox count = %d", *count);
	
	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	/* SNPRINTF(g_sql_query, sizeof(g_sql_query), "SELECT distinct mailbox_name FROM mail_partial_body_activity_tbl WHERE account_id = %d order by activity_id", account_id); */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT distinct mailbox_id FROM mail_partial_body_activity_tbl WHERE account_id = %d order by mailbox_id", account_id);

	EM_DEBUG_LOG(" Query [%s]", sql_query_string);
	
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG(" Bbefore sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	if (NULL == (mbox_list = (int *)em_malloc(sizeof(int *) * (*count)))) {
		EM_DEBUG_EXCEPTION(" em_malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(mbox_list, 0x00, sizeof(int) * (*count));
	
	for (i = 0; i < (*count); i++) {
		_get_stmt_field_data_int(hStmt, mbox_list + i, 0);
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		/* EM_DEBUG_LOG("In emstorage_get_pdb_mailbox_list() loop, After sqlite3_step(), , i = %d, rc = %d.", i,  rc); */
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
		EM_DEBUG_LOG("mbox_list %d", mbox_list + i);
	}

	ret = true;

FINISH_OFF:
	if (ret == true)
		*mailbox_list = mbox_list;

	if (hStmt != NULL) {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_pbd_account_list(int **account_list, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_list[%p], count[%p] err_code[%p]", account_list, count, err_code);

	if (NULL == &account_list || NULL == count) {
		EM_DEBUG_EXCEPTION("mailbox_list[%p], count[%p] err_code[%p]", account_list, count, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char *sql;
	char **result;
	int i = 0, rc = -1;
	int *result_account_list = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	
	EMSTORAGE_START_READ_TRANSACTION(transaction);


	sql = "SELECT count(distinct account_id) FROM mail_partial_body_activity_tbl";	


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);

	if (!*count) {
		EM_DEBUG_EXCEPTION("no account found...");
		error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("Account count [%d]", *count);
	
	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT distinct account_id FROM mail_partial_body_activity_tbl");

	EM_DEBUG_LOG("Query [%s]", sql_query_string);
	
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG("Before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	if (NULL == (result_account_list = (int *)em_malloc(sizeof(int) * (*count)))) {
		EM_DEBUG_EXCEPTION(" em_malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(result_account_list, 0x00, sizeof(int) * (*count));
	
	for (i = 0; i < (*count); i++) {
		_get_stmt_field_data_int(hStmt, result_account_list + i, 0);

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
		EM_DEBUG_LOG("account id -> %d", result_account_list[i]);
	}

	ret = true;

FINISH_OFF:
	if (ret == true)
		*account_list = result_account_list;

	if (hStmt != NULL) {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}



INTERNAL_FUNC int emstorage_get_pbd_activity_data(int account_id, int input_mailbox_id, email_event_partial_body_thd** event_start, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], event_start[%p], err_code[%p]", account_id, event_start, err_code);

	if (account_id < FIRST_ACCOUNT_ID || NULL == event_start || 0 == input_mailbox_id || NULL == count) {
		EM_DEBUG_EXCEPTION("account_id[%d], email_event_partial_body_thd[%p], input_mailbox_id[%d], count[%p], err_code[%p]", account_id, event_start, input_mailbox_id, count, err_code);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1;
	int ret = false;
	char **result;
	int error = EMAIL_ERROR_NONE;
	int i = 0;
	DB_STMT hStmt = NULL;
	email_event_partial_body_thd* event_list = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(*) FROM mail_partial_body_activity_tbl WHERE account_id = %d AND mailbox_id = '%d' order by activity_id", account_id, input_mailbox_id);
	

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);

	EM_DEBUG_LOG("Query = [%s]", sql_query_string);

	if (!*count) {
		EM_DEBUG_EXCEPTION("No matched activity found in mail_partial_body_activity_tbl");

		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("Activity Count = %d", *count);
	
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_partial_body_activity_tbl WHERE account_id = %d AND mailbox_id = '%d' order by activity_id", account_id, input_mailbox_id);

	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG(" Bbefore sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	if (!(event_list = (email_event_partial_body_thd*)em_malloc(sizeof(email_event_partial_body_thd) * (*count)))) {
		EM_DEBUG_EXCEPTION("Malloc failed");

		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memset(event_list, 0x00, sizeof(email_event_partial_body_thd) * (*count));
	
	for (i=0; i < (*count); i++) {
		_get_stmt_field_data_int(hStmt, &(event_list[i].account_id), ACCOUNT_IDX_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, &(event_list[i].mail_id), MAIL_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, (int *)&(event_list[i].server_mail_id), SERVER_MAIL_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, &(event_list[i].activity_id), ACTIVITY_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, &(event_list[i].activity_type), ACTIVITY_TYPE_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, &(event_list[i].mailbox_id), MAILBOX_ID_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_get_stmt_field_data_string(hStmt, &(event_list[i].mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		/* EM_DEBUG_LOG("In emstorage_get_pbd_activity_data() loop, After sqlite3_step(), , i = %d, rc = %d.", i,  rc); */
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));

		event_list[i].event_type = 0;
	}

	ret = true;

FINISH_OFF:
	if (true == ret)
	  *event_start = event_list;
	else {
		EM_SAFE_FREE(event_list);
	}

	if (hStmt != NULL) {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_LOG("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}



INTERNAL_FUNC int emstorage_delete_pbd_activity(int account_id, int mail_id, int activity_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d] , activity_id[%d], transaction[%d], err_code[%p]", account_id, mail_id, activity_id, transaction, err_code);


	if (account_id < FIRST_ACCOUNT_ID || activity_id < 0 || mail_id <= 0) {
		EM_DEBUG_EXCEPTION("account_id[%d], mail_id[%d], activity_id[%d], transaction[%d], err_code[%p]", account_id, mail_id, activity_id, transaction, err_code);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	
	if (activity_id == 0)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_partial_body_activity_tbl WHERE account_id = %d AND mail_id = %d", account_id, mail_id);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_partial_body_activity_tbl WHERE account_id = %d AND activity_id = %d", account_id, activity_id);

	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  validate activity existence */
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION("No matching activity found");
		error = EMAIL_ERROR_DATA_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	
	_DISCONNECT_DB;
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mailbox_pbd_activity_count(int account_id, int input_mailbox_id, int *activity_count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], activity_count[%p], err_code[%p]", account_id, activity_count, err_code);

	if (account_id < FIRST_ACCOUNT_ID || NULL == activity_count || NULL == err_code) {
		EM_DEBUG_EXCEPTION("account_id[%d], activity_count[%p], err_code[%p]", account_id, activity_count, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	int rc = -1;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	DB_STMT hStmt = NULL;

	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_READ_TRANSACTION(transaction);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(*) FROM mail_partial_body_activity_tbl WHERE account_id = %d and mailbox_id = '%d'", account_id, input_mailbox_id);

	EM_DEBUG_LOG(" Query [%s]", sql_query_string);

	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	_get_stmt_field_data_int(hStmt, activity_count, 0);

	EM_DEBUG_LOG("No. of activities in activity table [%d]", *activity_count);

	ret = true;

FINISH_OFF:

	if (hStmt != NULL) {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);
		rc = sqlite3_finalize(hStmt);
		hStmt=NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
		EM_DEBUG_LOG("sqlite3_finalize- %d", rc);
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_pbd_activity_count(int *activity_count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("activity_count[%p], err_code[%p]", activity_count, err_code);

	if (NULL == activity_count || NULL == err_code) {
		EM_DEBUG_EXCEPTION("activity_count[%p], err_code[%p]", activity_count, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	int rc = -1;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_READ_TRANSACTION(transaction);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(*) FROM mail_partial_body_activity_tbl;");

	EM_DEBUG_LOG(" Query [%s]", sql_query_string);

	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("  before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	_get_stmt_field_data_int(hStmt, activity_count, 0);

	EM_DEBUG_LOG("No. of activities in activity table [%d]", *activity_count);

	ret = true;

FINISH_OFF:


	if (hStmt != NULL) {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		hStmt=NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
		EM_DEBUG_LOG("sqlite3_finalize- %d", rc);
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_full_pbd_activity_data(int account_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], transaction[%d], err_code[%p]", account_id, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID) {
		EM_DEBUG_EXCEPTION("account_id[%d], transaction[%d], err_code[%p]", account_id, transaction, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_partial_body_activity_tbl WHERE account_id = %d", account_id);

	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION("No matching activities found in mail_partial_body_activity_tbl");
		error = EMAIL_ERROR_DATA_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/*Himanshu[h.gahlaut]-> Added below API to update mail_partial_body_activity_tbl
if a mail is moved before its partial body is downloaded.Currently not used but should be used if mail move from server is enabled*/

INTERNAL_FUNC int emstorage_update_pbd_activity(char *old_server_uid, char *new_server_uid, char *mbox_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("old_server_uid[%s], new_server_uid[%s], mbox_name[%s]", old_server_uid, new_server_uid, mbox_name);

	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	int transaction = true;

	if (!old_server_uid || !new_server_uid || !mbox_name)  {
		EM_DEBUG_EXCEPTION("Invalid parameters");
		error = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		 "UPDATE mail_partial_body_activity_tbl SET server_mail_id = %s , mailbox_name=\'%s\' WHERE server_mail_id = %s ", new_server_uid, mbox_name, old_server_uid);

	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION("No matching found in mail_partial_body_activity_tbl");
		error = EMAIL_ERROR_DATA_NOT_FOUND;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_create_file(char *data_string, size_t file_size, char *dst_file_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("file_size[%d] , dst_file_name[%s], err_code[%p]", file_size, dst_file_name, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	FILE* fp_dst = NULL;
	
	if (!data_string || !dst_file_name)  {				
		EM_DEBUG_EXCEPTION("data_string[%p], dst_file_name[%p]", data_string, dst_file_name);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	fp_dst = fopen(dst_file_name, "w");
	
	if (!fp_dst)  {
		EM_DEBUG_EXCEPTION("fopen failed - %s", dst_file_name);
		if (errno == 28)
			error = EMAIL_ERROR_MAIL_MEMORY_FULL;
		else
			error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;			
	}
	
	if (fwrite(data_string, 1, file_size, fp_dst) < 0) {
		EM_DEBUG_EXCEPTION("fwrite failed...");
		error = EMAIL_ERROR_UNKNOWN;
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	
	if (fp_dst != NULL)
		fclose(fp_dst);			

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

#endif /*  __FEATURE_PARTIAL_BODY_DOWNLOAD__ */



#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__

INTERNAL_FUNC int emstorage_update_read_mail_uid_by_server_uid(char *old_server_uid, char *new_server_uid, char *mbox_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int rc = -1;				   
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	int transaction = true;
		
	if (!old_server_uid || !new_server_uid || !mbox_name)  {		
		EM_DEBUG_EXCEPTION("Invalid parameters");
		error = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	EM_DEBUG_LOG("old_server_uid[%s], new_server_uid[%s], mbox_name[%s]", old_server_uid, new_server_uid, mbox_name);
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		 "UPDATE mail_read_mail_uid_tbl SET s_uid=\'%s\' , mailbox_id=\'%s\', mailbox_name=\'%s\' WHERE s_uid=%s ", new_server_uid, mbox_name, mbox_name, old_server_uid);

	 EM_DEBUG_LOG(" Query [%s]", sql_query_string);
	 
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	 EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		 ("sqlite3_exec fail:%d", rc));
	 EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		 ("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	 
	 
	 rc = sqlite3_changes(local_db_handle);
	 if (rc == 0)
	 {
		 EM_DEBUG_EXCEPTION("No matching found in mail_partial_body_activity_tbl");
		 error = EMAIL_ERROR_DATA_NOT_FOUND;
		 goto FINISH_OFF;
	 }

	  
	 ret = true;
	  
FINISH_OFF:
	  
  	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
 	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
	
}


/**
 * @fn emstorage_get_id_set_from_mail_ids(int mail_ids[], int mail_id_count, email_id_set_t** server_uids, int *id_set_count, int *err_code);
 * Prepare an array of mail_id and corresponding server mail id.
 *
 *@author 					h.gahlaut@samsung.com
 * @param[in] mail_ids			Specifies the comma separated string of mail_ids. Maximaum size of string should be less than or equal to (QUERY_SIZE - 88)
 *							where 88 is the length of fixed keywords including ending null character in the QUERY to be formed
 * @param[out] idset			Returns the array of mail_id and corresponding server_mail_id sorted by server_mail_ids
 * @param[out] id_set_count		Returns the no. of cells in idset array i.e. no. of sets of mail_ids and server_mail_ids
 * @param[out] err_code		Returns the error code.
 * @remarks 					An Example of Query to be exexuted in this API:
 *							SELECT local_uid, s_uid from mail_read_mail_uid_tbl where local_uid in (12, 13, 56, 78);
 * @return This function returns true on success or false on failure.
 */
 
INTERNAL_FUNC int emstorage_get_id_set_from_mail_ids(char *mail_ids, email_id_set_t** idset, int *id_set_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_PROFILE_BEGIN(EmStorageGetIdSetFromMailIds);

	int error = EMAIL_ERROR_NONE;
	int ret = false;
	email_id_set_t* p_id_set = NULL;
	int count = 0;
	const int buf_size = QUERY_SIZE;
	char sql_query_string[QUERY_SIZE] = {0, };
	int space_left_in_query_buffer = buf_size;
	int i = 0;
	int rc = -1;
	char *server_mail_id = NULL;
	char **result = NULL;
	int col_index = 0;
	
	
	if (NULL == mail_ids || NULL == idset || NULL == id_set_count) {
		EM_DEBUG_EXCEPTION("Invalid Parameters mail_ids[%p] idset[%p]  id_set_count [%p]", mail_ids, idset, id_set_count);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection();

	SNPRINTF(sql_query_string, space_left_in_query_buffer, "SELECT local_uid, s_uid FROM mail_read_mail_uid_tbl WHERE local_uid in (%s) ORDER BY s_uid", mail_ids);

	EM_DEBUG_LOG("SQL Query formed [%s] ", sql_query_string);
	
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG(" Count of mails [%d ]", count);

	if (count <= 0) {
		EM_DEBUG_EXCEPTION("Can't find proper mail");
		error = EMAIL_ERROR_DATA_NOT_FOUND;
		goto FINISH_OFF;
	}


	if (NULL == (p_id_set = (email_id_set_t*)em_malloc(sizeof(email_id_set_t) * count)))  {
		EM_DEBUG_EXCEPTION(" em_malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	col_index = 2;

	EM_PROFILE_BEGIN(EmStorageGetIdSetFromMailIds_Loop);
	EM_DEBUG_LOG(">>>> DATA ASSIGN START");	
	for (i = 0; i < count; i++)  {
		_get_table_field_data_int(result, &(p_id_set[i].mail_id), col_index++);
		_get_table_field_data_string(result, &server_mail_id, 1, col_index++);
		p_id_set[i].server_mail_id = strtoul(server_mail_id, NULL, 10);
		EM_SAFE_FREE(server_mail_id);		
	}
	EM_DEBUG_LOG(">>>> DATA ASSIGN END [count : %d]", count);
	EM_PROFILE_END(EmStorageGetIdSetFromMailIds_Loop);

	sqlite3_free_table(result);
	result = NULL;

	ret = true;

	FINISH_OFF:

	if (ret == true)  {
		*idset = p_id_set;
		*id_set_count = count;
		EM_DEBUG_LOG(" idset[%p] id_set_count [%d]", *idset, *id_set_count);
	}
	else 
		EM_SAFE_FREE(p_id_set);
	
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(EmStorageGetIdSetFromMailIds);

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}



#endif

INTERNAL_FUNC int emstorage_delete_triggers_from_lucene()
{
	EM_DEBUG_FUNC_BEGIN();
	int rc, ret = true, transaction = true;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP TRIGGER triggerDelete;");
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP TRIGGER triggerInsert;");
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP TRIGGER triggerUpdate;");
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	ret = true;
	
FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);

	_DISCONNECT_DB;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_filter_mails_by_rule(int account_id, int dest_mailbox_id, emstorage_rule_tbl_t *rule, int ** filtered_mail_id_list, int *count_of_mails, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], dest_mailbox_id[%d] rule[%p], filtered_mail_id_list[%p], count_of_mails[%p], err_code[%p]", account_id, dest_mailbox_id, rule, filtered_mail_id_list, count_of_mails, err_code);
	
	if ((account_id <= 0) || (dest_mailbox_id <= 0) || (!rule) || (!rule->value)|| (!filtered_mail_id_list) || (!count_of_mails)) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1, ret = false, error = EMAIL_ERROR_NONE;
	int count = 0, col_index = 0, i = 0, where_pararaph_length = 0, *mail_list = NULL;
	char **result = NULL, *where_pararaph = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mail_id FROM mail_tbl ");
	
	EM_DEBUG_LOG("rule->value [%s]", rule->value);

	where_pararaph_length = strlen(rule->value) + 100;
	where_pararaph = malloc(sizeof(char) * where_pararaph_length);

	if (where_pararaph == NULL) {
		EM_DEBUG_EXCEPTION("malloc failed for where_pararaph.");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(where_pararaph, 0, sizeof(char) * where_pararaph_length);
	
	EM_DEBUG_LOG("rule->type [%d], rule->flag2[%d]", rule->type, rule->flag2);

	if (rule->type == EMAIL_FILTER_FROM) {
		if (rule->flag2 == RULE_TYPE_INCLUDES)
			SNPRINTF(where_pararaph, where_pararaph_length, "WHERE account_id = %d AND mailbox_type NOT in (3,5) AND full_address_from like \'%%%s%%\'", account_id, rule->value);
		else /*  RULE_TYPE_EXACTLY */
			SNPRINTF(where_pararaph, where_pararaph_length, "WHERE account_id = %d AND mailbox_type NOT in (3,5) AND full_address_from = \'%s\'", account_id, rule->value);
	}
	else if (rule->type == EMAIL_FILTER_SUBJECT) {
		if (rule->flag2 == RULE_TYPE_INCLUDES)
			SNPRINTF(where_pararaph, where_pararaph_length, "WHERE account_id = %d AND mailbox_type NOT in (3,5) AND subject like \'%%%s%%\'", account_id, rule->value);
		else /*  RULE_TYPE_EXACTLY */
			SNPRINTF(where_pararaph, where_pararaph_length, "WHERE account_id = %d AND mailbox_type NOT in (3,5) AND subject = \'%s\'", account_id, rule->value);
	}
	else {
		error = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("rule->type is invald");
		goto FINISH_OFF;
	}
	
	if (strlen(sql_query_string) + strlen(where_pararaph) < QUERY_SIZE)
		strcat(sql_query_string, where_pararaph);

	EM_DEBUG_LOG("query[%s]", sql_query_string);
	
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG("Count of mails [%d]", count);

	if (count) {
		mail_list = malloc(sizeof(int) * count);
		if (mail_list == NULL) {
			EM_DEBUG_EXCEPTION("malloc failed for mail_list.");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		col_index = 1;

		for (i = 0; i < count; i++) 
			_get_table_field_data_int(result, &(mail_list[i]), col_index++);

		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET mailbox_id = %d, mailbox_type = 5 ", dest_mailbox_id);

		if (strlen(sql_query_string) + strlen(where_pararaph) < QUERY_SIZE)
		strcat(sql_query_string, where_pararaph);

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	}	
	
	*filtered_mail_id_list = mail_list;
	
	ret = true;
	
FINISH_OFF:
		
	sqlite3_free_table(result);
	result = NULL;

	_DISCONNECT_DB;

	EM_SAFE_FREE(where_pararaph);

	if (ret && count_of_mails)
		*count_of_mails = count;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

#define EMAIL_SLOT_UNIT 25

INTERNAL_FUNC int emstorage_set_mail_slot_size(int account_id, int mailbox_id, int new_slot_size, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_id[%p] new_slot_size[%d], err_code[%p]", account_id, mailbox_id, new_slot_size, err_code);
	int rc = -1, ret = false, err = EMAIL_ERROR_NONE;
	int where_pararaph_length = 0;
	char *where_pararaph = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	int and = 0;
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, err);

	if (new_slot_size > 0)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_box_tbl SET mail_slot_size = %d ", new_slot_size);
	else if (new_slot_size == 0)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_box_tbl SET mail_slot_size = mail_slot_size + %d ", EMAIL_SLOT_UNIT);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_box_tbl SET mail_slot_size = mail_slot_size + %d ", new_slot_size * -1);


	if (mailbox_id)
		where_pararaph_length = 80;
	else
		where_pararaph_length = 50;

	if (new_slot_size == 0)
		where_pararaph_length += 70;

	where_pararaph = malloc(sizeof(char) * where_pararaph_length);
	if (where_pararaph == NULL) {
		EM_DEBUG_EXCEPTION("Memory allocation failed for where_pararaph");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memset(where_pararaph, 0x00, where_pararaph_length);

	if (account_id > ALL_ACCOUNT) {
		and = 1;
		if (mailbox_id)
			SNPRINTF(where_pararaph, where_pararaph_length, "WHERE mailbox_id = %d ", mailbox_id);
		else
			SNPRINTF(where_pararaph, where_pararaph_length, "WHERE account_id = %d ", account_id);
	}

	if (new_slot_size == 0)
		SNPRINTF(where_pararaph + strlen(where_pararaph), where_pararaph_length - strlen(where_pararaph), " %s total_mail_count_on_server > mail_slot_size ", (and ? "AND" : "WHERE"));

	if (strlen(sql_query_string) + strlen(where_pararaph) < QUERY_SIZE)
		strcat(sql_query_string, where_pararaph);
	else {
		EM_DEBUG_EXCEPTION("Query buffer overflowed !!!");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;			
	}

	EM_DEBUG_LOG("query[%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
	("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	ret = true;
	
FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, err);

	_DISCONNECT_DB;

	EM_SAFE_FREE(where_pararaph);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_add_meeting_request(int account_id, int input_mailbox_id, email_meeting_request_t* meeting_req, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], input_mailbox_id[%d], meeting_req[%p], transaction[%d], err_code[%p]", account_id, input_mailbox_id, meeting_req, transaction, err_code);
	
	if (!meeting_req || meeting_req->mail_id <= 0) {
		if (meeting_req)
		EM_DEBUG_EXCEPTION("mail_id[%]d", meeting_req->mail_id);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;

		return false;
	}
	
	int rc = -1;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	int col_index = 0;
	time_t temp_unix_time = 0;
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_meeting_tbl VALUES "
		"( ?"		/*  mail_id */
		", ?"		/*  account_id */
		", ?"		/*  mailbox_id */
		", ?"		/*  meeting_response */
		", ?"		/*  start_time */
		", ?"		/*  end_time */
		", ?"		/*  location */
		", ?"		/*  global_object_id */
		", ?"		/*  offset */
		", ?"		/*  standard_name */
		", ?"		/*  standard_time_start_date */
		", ?"		/*  standard_biad */
		", ?"		/*  daylight_name */
		", ?"		/*  daylight_time_start_date */
		", ?"		/*  daylight_bias */
		" )");
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	if (rc != SQLITE_OK)  {
		EM_DEBUG_LOG(" before sqlite3_prepare hStmt = %p", hStmt);
		EM_DEBUG_EXCEPTION("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle));
		
		error = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	col_index = 0;
	/*
	EM_DEBUG_LOG(">>>>> meeting_req->mail_id[%d]", meeting_req->mail_id);
	EM_DEBUG_LOG(">>>>> account_id[%d]", account_id);
	EM_DEBUG_LOG(">>>>> mailbox_name[%s]", mailbox_name);
	EM_DEBUG_LOG(">>>>> meeting_req->meeting_response[%d]", meeting_req->meeting_response);
	EM_DEBUG_LOG(">>>>> meeting_req->start_time[%s]", asctime(&(meeting_req->start_time)));
	EM_DEBUG_LOG(">>>>> meeting_req->end_time[%s]", asctime(&(meeting_req->end_time)));
	EM_DEBUG_LOG(">>>>> meeting_req->location[%s]", meeting_req->location);
	EM_DEBUG_LOG(">>>>> meeting_req->global_object_id[%s]", meeting_req->global_object_id);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.offset_from_GMT[%d]", meeting_req->time_zone.offset_from_GMT);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.standard_name[%s]", meeting_req->time_zone.standard_name);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.standard_time_start_date[%s]", asctime(&(meeting_req->time_zone.standard_time_start_date)));
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.standard_bias[%d]", meeting_req->time_zone.standard_bias);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.daylight_name[%s]", meeting_req->time_zone.daylight_name);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.daylight_time_start_date[%s]", asctime(&(meeting_req->time_zone.daylight_time_start_date)));
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.daylight_bias[%d]", meeting_req->time_zone.daylight_bias);
	*/
	_bind_stmt_field_data_int(hStmt, col_index++, meeting_req->mail_id);
	_bind_stmt_field_data_int(hStmt, col_index++, account_id);
	_bind_stmt_field_data_int(hStmt, col_index++, input_mailbox_id);
	_bind_stmt_field_data_int(hStmt, col_index++, meeting_req->meeting_response);

	temp_unix_time = timegm(&(meeting_req->start_time));
	_bind_stmt_field_data_int(hStmt, col_index++, temp_unix_time);
	temp_unix_time = timegm(&(meeting_req->end_time));
	_bind_stmt_field_data_int(hStmt, col_index++, temp_unix_time);

	_bind_stmt_field_data_string(hStmt, col_index++, (char *)meeting_req->location, 0, LOCATION_LEN_IN_MAIL_MEETING_TBL);
	_bind_stmt_field_data_string(hStmt, col_index++, (char *)meeting_req->global_object_id, 0, GLOBAL_OBJECT_ID_LEN_IN_MAIL_MEETING_TBL);
	
	_bind_stmt_field_data_int(hStmt, col_index++, meeting_req->time_zone.offset_from_GMT);
	_bind_stmt_field_data_string(hStmt, col_index++, (char *)meeting_req->time_zone.standard_name, 0, STANDARD_NAME_LEN_IN_MAIL_MEETING_TBL);
	temp_unix_time = timegm(&(meeting_req->time_zone.standard_time_start_date));
	_bind_stmt_field_data_int(hStmt, col_index++, temp_unix_time);
	_bind_stmt_field_data_int(hStmt, col_index++, meeting_req->time_zone.standard_bias);

	_bind_stmt_field_data_string(hStmt, col_index++, (char *)meeting_req->time_zone.daylight_name, 0, DAYLIGHT_NAME_LEN_IN_MAIL_MEETING_TBL);
	temp_unix_time = timegm(&(meeting_req->time_zone.daylight_time_start_date));
	_bind_stmt_field_data_int(hStmt, col_index++, temp_unix_time);
	_bind_stmt_field_data_int(hStmt, col_index++, meeting_req->time_zone.daylight_bias);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
 
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_meeting_request(int mail_id, email_meeting_request_t ** meeting_req, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int count = 0;
	int rc = -1;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	email_meeting_request_t *p_temp_meeting_req = NULL;
	int col_index = 0;
	time_t temp_unix_time;

	EM_IF_NULL_RETURN_VALUE(meeting_req, false);

	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"SELECT  mail_id, meeting_response, start_time, end_time, location, global_object_id, offset, standard_name, standard_time_start_date, standard_bias, daylight_name, daylight_time_start_date, daylight_bias "
		" FROM mail_meeting_tbl "
		" WHERE mail_id = %d", mail_id);
	EM_DEBUG_LOG("sql : %s ", sql_query_string);

	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION(" no Meeting request found...");
		count = 0;
		ret = false;
		error= EMAIL_ERROR_DATA_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	if (!(p_temp_meeting_req = (email_meeting_request_t*)malloc(sizeof(email_meeting_request_t)))) {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(p_temp_meeting_req, 0x00, sizeof(email_meeting_request_t));
		
	col_index = 0;

	_get_stmt_field_data_int(hStmt, &(p_temp_meeting_req->mail_id), col_index++);
	_get_stmt_field_data_int(hStmt, (int *)&(p_temp_meeting_req->meeting_response), col_index++);
	_get_stmt_field_data_int(hStmt, (int *)(&temp_unix_time), col_index++);
	gmtime_r(&temp_unix_time, &(p_temp_meeting_req->start_time));
	_get_stmt_field_data_int(hStmt, (int *)(&temp_unix_time), col_index++);
	gmtime_r(&temp_unix_time, &(p_temp_meeting_req->end_time));
	_get_stmt_field_data_string(hStmt, &p_temp_meeting_req->location, 1, col_index++);
	_get_stmt_field_data_string(hStmt, &p_temp_meeting_req->global_object_id, 1, col_index++);
	_get_stmt_field_data_int(hStmt, &(p_temp_meeting_req->time_zone.offset_from_GMT), col_index++);

	_get_stmt_field_data_string_without_allocation(hStmt, p_temp_meeting_req->time_zone.standard_name, STANDARD_NAME_LEN_IN_MAIL_MEETING_TBL, 1, col_index++);
	_get_stmt_field_data_int(hStmt, (int *)(&temp_unix_time), col_index++);
	gmtime_r(&temp_unix_time, &(p_temp_meeting_req->time_zone.standard_time_start_date));
	_get_stmt_field_data_int(hStmt, &(p_temp_meeting_req->time_zone.standard_bias), col_index++);

	_get_stmt_field_data_string_without_allocation(hStmt, p_temp_meeting_req->time_zone.daylight_name, DAYLIGHT_NAME_LEN_IN_MAIL_MEETING_TBL, 1, col_index++);
	_get_stmt_field_data_int(hStmt, (int *)(&temp_unix_time), col_index++);
	gmtime_r(&temp_unix_time, &(p_temp_meeting_req->time_zone.daylight_time_start_date));
	_get_stmt_field_data_int(hStmt, &(p_temp_meeting_req->time_zone.daylight_bias), col_index++);

	/*
	EM_DEBUG_LOG(">>>>> meeting_req->mail_id[%d]", p_temp_meeting_req->mail_id);
	EM_DEBUG_LOG(">>>>> meeting_req->meeting_response[%d]", p_temp_meeting_req->meeting_response);
	EM_DEBUG_LOG(">>>>> meeting_req->start_time[%s]", asctime(&(p_temp_meeting_req->start_time)));
	EM_DEBUG_LOG(">>>>> meeting_req->end_time[%s]", asctime(&(p_temp_meeting_req->end_time)));
	EM_DEBUG_LOG(">>>>> meeting_req->location[%s]", p_temp_meeting_req->location);
	EM_DEBUG_LOG(">>>>> meeting_req->global_object_id[%s]", p_temp_meeting_req->global_object_id);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.offset_from_GMT[%d]", p_temp_meeting_req->time_zone.offset_from_GMT);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.standard_name[%s]", p_temp_meeting_req->time_zone.standard_name);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.standard_time_start_date[%s]", asctime(&(p_temp_meeting_req->time_zone.standard_time_start_date)));
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.standard_bias[%d]", p_temp_meeting_req->time_zone.standard_bias);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.daylight_name[%s]", p_temp_meeting_req->time_zone.daylight_name);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.daylight_time_start_date[%s]", asctime(&(p_temp_meeting_req->time_zone.daylight_time_start_date)));
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.daylight_bias[%d]", p_temp_meeting_req->time_zone.daylight_bias);
	*/
	ret = true;
	
FINISH_OFF:
	if (ret == true) 
		*meeting_req = p_temp_meeting_req;
	else  {	
		EM_SAFE_FREE(p_temp_meeting_req);
	}
	 	
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_update_meeting_request(email_meeting_request_t* meeting_req, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("meeting_req[%p], transaction[%d], err_code[%p]", meeting_req, transaction, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int rc;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	time_t temp_unix_time = 0;

	if (!meeting_req) {
		EM_DEBUG_EXCEPTION("Invalid Parameter!");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_meeting_tbl "
		"SET "
		/* "  account_id = ?, "		//  not update here, this can be changed when move or copy */
		/* "  mailbox_name = ?, "		//  not update here, this can be changed when move or copy */
		"  meeting_response = ?, "
		"  start_time = ?, "
		"  end_time = ?, "
		"  location = ?, "
		"  global_object_id = ?, "
		"  offset = ?, "
		"  standard_name = ?, "
		"  standard_time_start_date = ?, "
		"  standard_bias = ?, "
		"  daylight_name = ?, "
		"  daylight_time_start_date = ?, "
		"  daylight_bias = ? "
		"WHERE mail_id = %d",
		meeting_req->mail_id);	
	
	EM_DEBUG_LOG("SQL(%s)", sql_query_string);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
/*
	EM_DEBUG_LOG(">>>>> meeting_req->mail_id[%d]", meeting_req->mail_id);
	EM_DEBUG_LOG(">>>>> meeting_req->meeting_response[%d]", meeting_req->meeting_response);
	EM_DEBUG_LOG(">>>>> meeting_req->start_time[%s]", asctime(&(meeting_req->start_time)));
	EM_DEBUG_LOG(">>>>> meeting_req->end_time[%s]", asctime(&(meeting_req->end_time)));
	EM_DEBUG_LOG(">>>>> meeting_req->location[%s]", meeting_req->location);
	EM_DEBUG_LOG(">>>>> meeting_req->global_object_id[%s]", meeting_req->global_object_id);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.offset_from_GMT[%d]", meeting_req->time_zone.offset_from_GMT);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.standard_name[%s]", meeting_req->time_zone.standard_name);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.standard_time_start_date[%s]", asctime(&(meeting_req->time_zone.standard_time_start_date)));
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.standard_bias[%d]", meeting_req->time_zone.standard_bias);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.daylight_name[%s]", meeting_req->time_zone.daylight_name);
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.daylight_time_start_date[%s]", asctime(&(meeting_req->time_zone.daylight_time_start_date)));
	EM_DEBUG_LOG(">>>>> meeting_req->time_zone.daylight_bias[%d]", meeting_req->time_zone.daylight_bias);
*/
	int col_index = 0;

	_bind_stmt_field_data_int(hStmt, col_index++, meeting_req->meeting_response);
	temp_unix_time = timegm(&(meeting_req->start_time));
	_bind_stmt_field_data_int(hStmt, col_index++, temp_unix_time);
	temp_unix_time = timegm(&(meeting_req->end_time));
	_bind_stmt_field_data_int(hStmt, col_index++, temp_unix_time);
	_bind_stmt_field_data_string(hStmt, col_index++, meeting_req->location, 1, LOCATION_LEN_IN_MAIL_MEETING_TBL);
	_bind_stmt_field_data_string(hStmt, col_index++, meeting_req->global_object_id, 1, GLOBAL_OBJECT_ID_LEN_IN_MAIL_MEETING_TBL);
	_bind_stmt_field_data_int(hStmt, col_index++, meeting_req->time_zone.offset_from_GMT);
	_bind_stmt_field_data_string(hStmt, col_index++, meeting_req->time_zone.standard_name, 1, STANDARD_NAME_LEN_IN_MAIL_MEETING_TBL);
	temp_unix_time = timegm(&(meeting_req->time_zone.standard_time_start_date));
	_bind_stmt_field_data_int(hStmt, col_index++, temp_unix_time);
	_bind_stmt_field_data_int(hStmt, col_index++, meeting_req->time_zone.standard_bias);
	_bind_stmt_field_data_string(hStmt, col_index++, meeting_req->time_zone.daylight_name, 1, DAYLIGHT_NAME_LEN_IN_MAIL_MEETING_TBL);
	temp_unix_time = timegm(&(meeting_req->time_zone.daylight_time_start_date));
	_bind_stmt_field_data_int(hStmt, col_index++, temp_unix_time);
	_bind_stmt_field_data_int(hStmt, col_index++, meeting_req->time_zone.daylight_bias);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	ret = true;


FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);

			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;	
}

INTERNAL_FUNC int emstorage_delete_meeting_request(int account_id, int mail_id, int input_mailbox_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], input_mailbox_id[%d], transaction[%d], err_code[%p]", account_id, mail_id, input_mailbox_id, transaction, err_code);
	
	if (account_id < ALL_ACCOUNT || mail_id < 0) {
		EM_DEBUG_EXCEPTION(" account_id[%d], mail_id[%d]", account_id, mail_id);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int and = false;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_meeting_tbl ");

	if (account_id != ALL_ACCOUNT) {		/*  NOT '0' means a specific account. '0' means all account */
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " WHERE account_id = %d", account_id);
		and = true;
	}
	if (mail_id > 0) {	
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " %s mail_id = %d", (and ? "AND" : "WHERE"), mail_id);
		and = true;
	}
	if (input_mailbox_id > 0) {		/*  0 means all mailbox_id */
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " %s mailbox_id = '%d'",  (and ? "AND" : "WHERE"), input_mailbox_id);
	}
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	ret = true;	
	
FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC void emstorage_free_meeting_request(email_meeting_request_t *meeting_req)
{
	EM_DEBUG_FUNC_BEGIN();
	
	if (!meeting_req ) {
		EM_DEBUG_EXCEPTION("NULL PARAM");
		return;
		}
		
	EM_SAFE_FREE(meeting_req->location);
	EM_SAFE_FREE(meeting_req->global_object_id);

   	EM_DEBUG_FUNC_END();
}


INTERNAL_FUNC int emstorage_get_overflowed_mail_id_list(int account_id, int input_mailbox_id, int mail_slot_size, int **mail_id_list, int *mail_id_count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], input_mailbox_id [%d], mail_slot_size [%d], mail_id_list [%p], mail_id_count [%p], transaction [%d], err_code [%p]", account_id, input_mailbox_id, mail_slot_size, mail_id_list, mail_id_count, transaction, err_code);
	EM_PROFILE_BEGIN(profile_emstorage_get_overflowed_mail_id_list);
	char sql_query_string[QUERY_SIZE] = {0, };
	char **result = NULL;
	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	int counter = 0, col_index = 0;
	int result_mail_id_count = 0;
	int *result_mail_id_list = NULL;

	if (input_mailbox_id <= 0 || !mail_id_list || !mail_id_count || account_id < 1) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mail_id FROM mail_tbl WHERE account_id = %d AND mailbox_id = %d ORDER BY date_time DESC LIMIT %d, 10000", account_id, input_mailbox_id, mail_slot_size);
	
	EM_DEBUG_LOG("query[%s].", sql_query_string);

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_mail_id_count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (!result_mail_id_count) {
		EM_DEBUG_LOG("No mail found...");			
		ret = false;
		error= EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	EM_DEBUG_LOG("There are [%d] overflowed mails in mailbox_id [%d]", result_mail_id_count, input_mailbox_id);
	
	if (!(result_mail_id_list = (int *)malloc(sizeof(int) * result_mail_id_count))) {
		EM_DEBUG_EXCEPTION("malloc for result_mail_id_list failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		sqlite3_free_table(result);
		goto FINISH_OFF;
	}

	memset(result_mail_id_list, 0x00, sizeof(int) * result_mail_id_count);

	col_index = 1;

	for (counter = 0; counter < result_mail_id_count; counter++) 
		_get_table_field_data_int(result, result_mail_id_list + counter, col_index++);

	ret = true;
	
FINISH_OFF:
	EM_DEBUG_LOG("finish off [%d]", ret);

	if (result)
		sqlite3_free_table(result);

	if (ret == true)  {
		*mail_id_list = result_mail_id_list;
		*mail_id_count = result_mail_id_count;
	}
	else 
		EM_SAFE_FREE(result_mail_id_list);
	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(profile_emstorage_get_overflowed_mail_id_list);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_thread_id_by_mail_id(int mail_id, int *thread_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], thread_id[%p], err_code[%p]", mail_id, thread_id, err_code);
	
	int rc = -1, ret = false;
	int err = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	char **result;
	int result_count = 0;

	if (mail_id == 0 || thread_id == NULL) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection();

	memset(sql_query_string, 0, QUERY_SIZE);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT thread_id FROM mail_tbl WHERE mail_id = %d", mail_id);

	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_count, 0, NULL); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {err = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (!result_count) {
		EM_DEBUG_EXCEPTION("No mail found...");			
		ret = false;
		err= EMAIL_ERROR_MAIL_NOT_FOUND;
		/* sqlite3_free_table(result); */
		goto FINISH_OFF;
	}

	_get_table_field_data_int(result, thread_id, 1);

	sqlite3_free_table(result);
	
	ret = true;
	
FINISH_OFF:

	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_update_latest_thread_mail(int account_id, int thread_id, int latest_mail_id, int thread_item_count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], thread_id[%d], latest_mail_id [%d], thread_item_count[%d], err_code[%p]", account_id, thread_id, latest_mail_id, thread_item_count, err_code);
	
	int rc = -1, ret = false;
	int err = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	char **result;
	int result_count = 0;

	if (thread_id == 0) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	if (thread_item_count == 0 || latest_mail_id == 0) {
		memset(sql_query_string, 0, QUERY_SIZE);
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mail_id, count(*) FROM (SELECT account_id, mail_id, thread_id, mailbox_type FROM mail_tbl ORDER BY date_time) WHERE account_id = %d AND thread_id = %d AND mailbox_type NOT in (3,5,7)", account_id, thread_id);

		/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_count, 0, NULL); */
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_count, 0, NULL), rc);
		EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {err = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		EM_DEBUG_LOG("result_count[%d]", result_count);
		if (result_count == 0) {
			EM_DEBUG_EXCEPTION("No mail found...");			
			ret = false;
			err= EMAIL_ERROR_MAIL_NOT_FOUND;
			sqlite3_free_table(result);
			goto FINISH_OFF;
		}

		_get_table_field_data_int(result, &latest_mail_id, 2);
		_get_table_field_data_int(result, &thread_item_count, 3);

		EM_DEBUG_LOG("latest_mail_id[%d]", latest_mail_id);
		EM_DEBUG_LOG("thread_item_count[%d]", thread_item_count);

		sqlite3_free_table(result);
	}

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, err);

	/* if (thread_item_count > 1) */
	/* { */
		memset(sql_query_string, 0, QUERY_SIZE);
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET thread_item_count = 0 WHERE account_id = %d AND thread_id = %d", account_id, thread_id);
		EM_DEBUG_LOG("query[%s]", sql_query_string);

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
			EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	/* } */

	memset(sql_query_string, 0, QUERY_SIZE);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET thread_item_count = %d WHERE account_id = %d AND mail_id = %d ", thread_item_count, account_id, latest_mail_id);
	EM_DEBUG_LOG("query[%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	ret = true;
	
FINISH_OFF:
		
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, err);
	
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC void emstorage_flush_db_cache()
{
	sqlite3_release_memory(-1);
}

#ifdef __FEATURE_LOCAL_ACTIVITY__
/**
  * 	emstorage_add_activity - Add Email Local activity during OFFLINE mode
  *
  */
INTERNAL_FUNC int emstorage_add_activity(emstorage_activity_tbl_t* local_activity, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	EM_DEBUG_LOG(" local_activity[%p], transaction[%d], err_code[%p]", local_activity, transaction, err_code);

	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[8192] = { 0x00, };
	int i = 0;
	
	if (!local_activity) {
		EM_DEBUG_EXCEPTION(" local_activity[%p], transaction[%d], err_code[%p]", local_activity, transaction, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	memset(sql_query_string, 0x00 , sizeof(sql_query_string));
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "INSERT INTO mail_local_activity_tbl VALUES (?, ?, ?, ?, ?, ?, ?)");

	EM_DEBUG_LOG(">>>>> ACTIVITY ID [ %d ] ", local_activity->activity_id);
	EM_DEBUG_LOG(">>>>> MAIL ID [ %d ] ", local_activity->mail_id);
	EM_DEBUG_LOG(">>>>> ACCOUNT ID [ %d ] ", local_activity->account_id);
	EM_DEBUG_LOG(">>>>> ACTIVITY TYPE [ %d ] ", local_activity->activity_type);
	EM_DEBUG_LOG(">>>>> SERVER MAIL ID [ %s ] ", local_activity->server_mailid);
	EM_DEBUG_LOG(">>>>> SOURCE MAILBOX [ %s ] ", local_activity->src_mbox);
	EM_DEBUG_LOG(">>>>> DEST MAILBOX   [ %s ] ", local_activity->dest_mbox);

	EM_DEBUG_LOG(">>>> SQL STMT [ %s ] ", sql_query_string);
	
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG(">>>> SQL STMT [ %s ] ", sql_query_string);




	_bind_stmt_field_data_int(hStmt, i++, local_activity->activity_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->account_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->mail_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->activity_type);
	_bind_stmt_field_data_string(hStmt, i++ , (char *)local_activity->server_mailid, 0, SERVER_MAIL_ID_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++ , (char *)local_activity->src_mbox, 0, MAILBOX_NAME_LEN_IN_MAIL_BOX_TBL);
	_bind_stmt_field_data_string(hStmt, i++ , (char *)local_activity->dest_mbox, 0, MAILBOX_NAME_LEN_IN_MAIL_BOX_TBL);
	

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);

	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d, errmsg = %s.", rc, sqlite3_errmsg(local_db_handle)));

	ret = true;
	
FINISH_OFF:
	
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EMAIL_ERROR_DB_FAILURE;
		}		
	}
	else {
		EM_DEBUG_LOG(" >>>>>>>>>> hStmt is NULL!!!");
	}

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/**
  *	emstorage_get_activity - Get the Local activity Information
  *	
  *
  */
INTERNAL_FUNC int emstorage_get_activity(int account_id, int activityid, emstorage_activity_tbl_t** activity_list, int *select_num, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int i = 0, count = 0, rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	emstorage_activity_tbl_t *p_activity_tbl = NULL;
	char sql_query_string[1024] = {0x00, };
	char **result = NULL;
	int col_index ;

	EM_IF_NULL_RETURN_VALUE(activity_list, false);
	EM_IF_NULL_RETURN_VALUE(select_num, false);

	
	if (!select_num || !activity_list || account_id <= 0 || activityid < 0) {
		EM_DEBUG_LOG(" select_num[%p], activity_list[%p] account_id [%d] activityid [%d] ", select_num, activity_list, account_id, activityid);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	if (activityid == ALL_ACTIVITIES) {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_local_activity_tbl WHERE account_id = %d order by activity_id", account_id);
	}
	else {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_local_activity_tbl WHERE account_id = %d AND activity_id = %d ", account_id, activityid);
	}

	EM_DEBUG_LOG("Query [%s]", sql_query_string);

		

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	
	col_index = 7;
	
	if (!(p_activity_tbl = (emstorage_activity_tbl_t*)em_malloc(sizeof(emstorage_activity_tbl_t) * count))) {
		EM_DEBUG_EXCEPTION(" em_malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	
	for (i = 0; i < count; i++)  {
		EM_DEBUG_LOG("result[%d] - %s ", col_index, result[col_index]);
		if (result[col_index])
			p_activity_tbl[i].activity_id = atoi(result[col_index++]);

		EM_DEBUG_LOG("result[%d] - %s ", col_index, result[col_index]);
		if (result[col_index])
			p_activity_tbl[i].account_id = atoi(result[col_index++]);

		EM_DEBUG_LOG("result[%d] - %s ", col_index, result[col_index]);
		if (result[col_index])
			p_activity_tbl[i].mail_id = atoi(result[col_index++]);

		EM_DEBUG_LOG("result[%d] - %s ", col_index, result[col_index]);
		if (result[col_index])
			p_activity_tbl[i].activity_type = atoi(result[col_index++]);


		EM_DEBUG_LOG("result[%d] - %s ", col_index, result[col_index]);
		if (result[col_index] && strlen(result[col_index])>0)
			p_activity_tbl[i].server_mailid = EM_SAFE_STRDUP(result[col_index++]);
		else
			col_index++;

		EM_DEBUG_LOG("result[%d] - %s ", col_index, result[col_index]);
		if (result[col_index] && strlen(result[col_index])>0)
			p_activity_tbl[i].src_mbox = EM_SAFE_STRDUP(result[col_index++]);
		else
			col_index++;		

		EM_DEBUG_LOG("result[%d] - %s ", col_index, result[col_index]);
		if (result[col_index] && strlen(result[col_index])>0)
			p_activity_tbl[i].dest_mbox = EM_SAFE_STRDUP(result[col_index++]);
		else
			col_index++;

	}

	if (result)
		sqlite3_free_table(result);

	ret = true;
	
FINISH_OFF:

	
	if (ret == true)  {
		*activity_list = p_activity_tbl;
		*select_num = count;
		EM_DEBUG_LOG(">>>> COUNT : %d >> ", count);
	}
	else if (p_activity_tbl != NULL) {
		emstorage_free_local_activity(&p_activity_tbl, count, NULL);
	}
	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_next_activity_id(int *activity_id, int *err_code)
{

	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int rc = -1;
	char *sql = NULL;
	char **result = NULL;	

	if (NULL == activity_id)
   	{
		EM_DEBUG_EXCEPTION(" activity_id[%p]", activity_id);
		
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/*  increase unique id */

	sql = "SELECT max(rowid) FROM mail_local_activity_tbl;";
	
	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL); n EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc); */
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL==result[1])
		rc = 1;
	else 
		rc = atoi(result[1])+1;

	*activity_id = rc;

	if (result)
		sqlite3_free_table(result);
	
	ret = true;
	
	FINISH_OFF:
		
	if (NULL != err_code) {
		*err_code = err;
	}
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}

INTERNAL_FUNC int emstorage_get_activity_id_list(int account_id, int ** activity_id_list, int *activity_id_count, int lowest_activity_type, int highest_activity_type, int transaction, int *err_code)
{

	EM_DEBUG_FUNC_BEGIN();
	
	EM_DEBUG_LOG(" account_id[%d], activity_id_list[%p], activity_id_count[%p] err_code[%p]", account_id,  activity_id_list, activity_id_count, err_code);
	
	if (account_id <= 0|| NULL == activity_id_list || NULL == activity_id_count ||lowest_activity_type <=0 || highest_activity_type <= 0)  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int i = 0, rc = -1, count = 0;
	char sql_query_string[1024] = {0x00, };
	int *activity_ids = NULL;
	int col_index = 0;
	char **result = NULL;
	
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT distinct activity_id FROM mail_local_activity_tbl WHERE account_id = %d AND activity_type >= %d AND activity_type <= %d order by activity_id", account_id, lowest_activity_type, highest_activity_type);

	EM_DEBUG_LOG(" Query [%s]", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	col_index = 1;

	EM_DEBUG_LOG(" Activity COUNT : %d ... ", count);

	if (NULL == (activity_ids = (int *)em_malloc(sizeof(int) * count))) {
		EM_DEBUG_EXCEPTION(" em_malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < count; i++)  {
		activity_ids[i] = atoi(result[col_index]);
		col_index++;
		EM_DEBUG_LOG("activity_id %d", activity_ids[i]);
	}
	
	ret = true;
	
FINISH_OFF:

	
	if (ret == true)  {
		*activity_id_count = count;
		*activity_id_list = activity_ids;
		
	}
	else if (activity_ids != NULL) /* Prevent defect - 216566 */
		EM_SAFE_FREE(activity_ids);
	

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	if (err_code != NULL) {
		*err_code = error;
	}

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_free_activity_id_list(int *activity_id_list, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int error = EMAIL_ERROR_NONE;
	int ret = false;

	EM_DEBUG_LOG(" activity_id_list [%p]", activity_id_list);

	if (NULL == activity_id_list) {
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	else {
		free(activity_id_list);
		activity_id_list = NULL;
	}
	

	ret= true;

	FINISH_OFF:

	if (NULL != error_code) {
		*error_code = error;
	}

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/**
 * emstorage_delete_local_activity - Deletes the Local acitivity Generated based on activity_type
 * or based on server mail id
 *
 */
INTERNAL_FUNC int emstorage_delete_local_activity(emstorage_activity_tbl_t* local_activity, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();


	EM_DEBUG_LOG(" local_activity[%p] ", local_activity);
	
	if (!local_activity)  {
		EM_DEBUG_EXCEPTION(" local_activity[%p] ", local_activity);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}		
	
	int rc = -1, ret = false;			/* Prevent_FIX  */
	int err = EMAIL_ERROR_NONE;
	int query_and = 0;
	int query_where = 0;
	char sql_query_string[8192] = { 0x00, };
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_local_activity_tbl ");

	EM_DEBUG_LOG(">>> Query [ %s ] ", sql_query_string);

	if (local_activity->account_id) {
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1),
		" WHERE account_id = %d ", local_activity->account_id);
		query_and = 1;
		query_where = 1;
	}

	if (local_activity->server_mailid) {
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1),
		" %s %s server_mailid = '%s' ", query_where? "": "WHERE", query_and? "AND":"", local_activity->server_mailid);
		query_and = 1;
		query_where = 1;
	}


	if (local_activity->mail_id) {
		EM_DEBUG_LOG(">>>> MAIL ID [ %d ] , ACTIVITY TYPE [%d ]", local_activity->mail_id, local_activity->activity_type);
		
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1),
		" %s %s mail_id = %d  ", query_where? "": "WHERE", query_and? "AND":"", local_activity->mail_id);
		
		query_and = 1;
		query_where = 1;
		
	}

	if (local_activity->activity_type > 0) {
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1),
		" %s %s activity_type = %d ", query_where? "": "WHERE", query_and? "AND" : "" , local_activity->activity_type);
	}

	if (local_activity->activity_id > 0) {
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1),
		" %s %s activity_id = %d ", query_where? "": "WHERE", query_and? "AND" : "" , local_activity->activity_id);

	}	

	EM_DEBUG_LOG(">>>>> Query [ %s ] ", sql_query_string);
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no (matched) mailbox_name found...");
		err = EMAIL_ERROR_MAILBOX_NOT_FOUND;
	}

	ret = true;
	
FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/**
*	emstorage_free_local_activity - Free the Local Activity data
*/
INTERNAL_FUNC int emstorage_free_local_activity(emstorage_activity_tbl_t **local_activity_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	EM_DEBUG_LOG(" local_activity_list[%p], count[%d], err_code[%p]", local_activity_list, count, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_INVALID_PARAM;
	
	if (count > 0) {
		if (!local_activity_list || !*local_activity_list) {
			EM_DEBUG_EXCEPTION(" local_activity_list[%p], count[%d]", local_activity_list, count);
			
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		emstorage_activity_tbl_t* p = *local_activity_list;
		int i = 0;
		if (p) {
			for (; i < count; i++)  {
				EM_SAFE_FREE(p[i].dest_mbox);
				EM_SAFE_FREE(p[i].src_mbox);
				EM_SAFE_FREE(p[i].server_mailid);
			}
		
			free(p); 	*local_activity_list = NULL;
		}
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

   	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}
#endif /* __FEATURE_LOCAL_ACTIVITY__ */

static char *_mail_field_name_list[] = {
		"mail_id",
		"account_id",
		"mailbox_id",
		"mailbox_name",
		"mailbox_type",
		"subject",
		"date_time",
		"server_mail_status",
		"server_mailbox_name",
		"server_mail_id",
		"message_id",
		"reference_mail_id",
		"full_address_from",
		"full_address_to",
		"full_address_cc",
		"full_address_bcc",
		"body_download_status",
		"file_path_plain",
		"file_path_html",
		"file_size",
		"flags_seen_field",
		"flags_deleted_field",
		"flags_flagged_field",
		"flags_answered_field",
		"flags_recent_field",
		"flags_draft_field",
		"flags_forwarded_field",
		"drm_status",
		"priority",
		"save_status",
		"lock_status",
		"report_status",
		"attachment_count",
		"inline_content_count",
		"thread_id",
		"thread_item_count",
		"preview_text",
		"meeting_request_status",
		"message_class",
		"digest_type",
		"smime_type"
};

static char*_get_mail_field_name_by_attribute_type(email_mail_attribute_type input_attribute_type)
{
	EM_DEBUG_FUNC_BEGIN("input_attribute_type [%d]", input_attribute_type);

	if(input_attribute_type > EMAIL_MAIL_ATTRIBUTE_MEETING_REQUEST_STATUS || input_attribute_type < 0) {
		EM_DEBUG_EXCEPTION("Invalid input_attribute_type [%d]", input_attribute_type);
		return NULL;
	}

	EM_DEBUG_FUNC_END("return [%s]", _mail_field_name_list[input_attribute_type]);
	return _mail_field_name_list[input_attribute_type];
}

static int _get_attribute_type_by_mail_field_name(char *input_mail_field_name, email_mail_attribute_type *output_mail_attribute_type)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_field_name [%p], output_mail_attribute_type [%p]", input_mail_field_name, output_mail_attribute_type);
	int i = 0;
	int err = EMAIL_ERROR_DATA_NOT_FOUND;

	if(!input_mail_field_name || !output_mail_attribute_type) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	for (i = 0; i < EMAIL_MAIL_ATTRIBUTE_END; i++) {
		if (EM_SAFE_STRCMP(input_mail_field_name, _mail_field_name_list[i]) == 0) {
			*output_mail_attribute_type = i;
			err = EMAIL_ERROR_NONE;
			break;
		}
	}

	EM_DEBUG_FUNC_END("found mail attribute type [%d]", (int)*output_mail_attribute_type);
	return err;
}

static int _get_key_value_string_for_list_filter_rule(email_list_filter_rule_t *input_list_filter_rule,char **output_key_value_string)
{
	EM_DEBUG_FUNC_BEGIN("input_list_filter_rule [%p], output_key_value_string [%p]", input_list_filter_rule, output_key_value_string);

	int  ret = EMAIL_ERROR_NONE;
	char key_value_string[QUERY_SIZE] = { 0, };
	char *temp_key_value_1 = NULL;
	char *temp_key_value_2 = NULL;

	if(input_list_filter_rule == NULL || output_key_value_string == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	switch(input_list_filter_rule->target_attribute) {
	case EMAIL_MAIL_ATTRIBUTE_MAIL_ID                 :
	case EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID              :
	case EMAIL_MAIL_ATTRIBUTE_MAILBOX_ID              :
	case EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE            :
	case EMAIL_MAIL_ATTRIBUTE_SERVER_MAIL_STATUS      :
	case EMAIL_MAIL_ATTRIBUTE_REFERENCE_MAIL_ID       :
	case EMAIL_MAIL_ATTRIBUTE_BODY_DOWNLOAD_STATUS    :
	case EMAIL_MAIL_ATTRIBUTE_FILE_PATH_PLAIN         :
	case EMAIL_MAIL_ATTRIBUTE_FILE_PATH_HTML          :
	case EMAIL_MAIL_ATTRIBUTE_FILE_SIZE               :
	case EMAIL_MAIL_ATTRIBUTE_FLAGS_SEEN_FIELD        :
	case EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD     :
	case EMAIL_MAIL_ATTRIBUTE_FLAGS_FLAGGED_FIELD     :
	case EMAIL_MAIL_ATTRIBUTE_FLAGS_ANSWERED_FIELD    :
	case EMAIL_MAIL_ATTRIBUTE_FLAGS_RECENT_FIELD      :
	case EMAIL_MAIL_ATTRIBUTE_FLAGS_DRAFT_FIELD       :
	case EMAIL_MAIL_ATTRIBUTE_FLAGS_FORWARDED_FIELD   :
	case EMAIL_MAIL_ATTRIBUTE_DRM_STATUS              :
	case EMAIL_MAIL_ATTRIBUTE_PRIORITY                :
	case EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS             :
	case EMAIL_MAIL_ATTRIBUTE_LOCK_STATUS             :
	case EMAIL_MAIL_ATTRIBUTE_REPORT_STATUS           :
	case EMAIL_MAIL_ATTRIBUTE_ATTACHMENT_COUNT        :
	case EMAIL_MAIL_ATTRIBUTE_INLINE_CONTENT_COUNT    :
	case EMAIL_MAIL_ATTRIBUTE_THREAD_ID               :
	case EMAIL_MAIL_ATTRIBUTE_THREAD_ITEM_COUNT       :
	case EMAIL_MAIL_ATTRIBUTE_MEETING_REQUEST_STATUS  :
	case EMAIL_MAIL_ATTRIBUTE_MESSAGE_CLASS           :
	case EMAIL_MAIL_ATTRIBUTE_DIGEST_TYPE             :
	case EMAIL_MAIL_ATTRIBUTE_SMIME_TYPE              :
		SNPRINTF(key_value_string, QUERY_SIZE, "%d", input_list_filter_rule->key_value.integer_type_value);
		break;

	case EMAIL_MAIL_ATTRIBUTE_MAILBOX_NAME            :
	case EMAIL_MAIL_ATTRIBUTE_SUBJECT                 :
	case EMAIL_MAIL_ATTRIBUTE_SERVER_MAILBOX_NAME     :
	case EMAIL_MAIL_ATTRIBUTE_SERVER_MAIL_ID          :
	case EMAIL_MAIL_ATTRIBUTE_MESSAGE_ID              :
	case EMAIL_MAIL_ATTRIBUTE_FROM                    :
	case EMAIL_MAIL_ATTRIBUTE_TO                      :
	case EMAIL_MAIL_ATTRIBUTE_CC                      :
	case EMAIL_MAIL_ATTRIBUTE_BCC                     :
	case EMAIL_MAIL_ATTRIBUTE_PREVIEW_TEXT            :
		if(input_list_filter_rule->key_value.string_type_value == NULL) {
			EM_DEBUG_EXCEPTION("Invalid string_type_value [%p]", input_list_filter_rule->key_value.string_type_value);
			ret = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		temp_key_value_1 = input_list_filter_rule->key_value.string_type_value;

		temp_key_value_2 = em_replace_all_string(temp_key_value_1, "_", "\\_");
		temp_key_value_1 = em_replace_all_string(temp_key_value_2, "%", "\\%");

		if(input_list_filter_rule->rule_type == EMAIL_LIST_FILTER_RULE_INCLUDE)
			SNPRINTF(key_value_string, QUERY_SIZE, "\'%%%s%%\'", temp_key_value_1);
		else
			SNPRINTF(key_value_string, QUERY_SIZE, "\'%s\'", temp_key_value_1);
		break;

	case EMAIL_MAIL_ATTRIBUTE_DATE_TIME               :
		SNPRINTF(key_value_string, QUERY_SIZE, "%d", (int)input_list_filter_rule->key_value.datetime_type_value);
		break;

	default :
		ret = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("Invalid target_attribute [%d]", input_list_filter_rule->target_attribute);
		break;
	}

	if(ret == EMAIL_ERROR_NONE && strlen(key_value_string) > 0) {
		*output_key_value_string = strdup(key_value_string);
	}

FINISH_OFF:

	EM_SAFE_FREE(temp_key_value_1);
	EM_SAFE_FREE(temp_key_value_2);

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__
static int _get_cert_password_file_name(int index, char *cert_password_file_name)
{
	EM_DEBUG_FUNC_BEGIN("index : [%d]", index);

	if (index <= 0 || !cert_password_file_name) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	sprintf(cert_password_file_name, ".email_cert_%d", index);

	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}
#endif

static int _make_filter_rule_string(email_list_filter_rule_t *input_list_filter_rule, char **output_string)
{
	EM_DEBUG_FUNC_BEGIN("input_list_filter_rule [%p], output_string [%p]", input_list_filter_rule, output_string);

	int   ret = EMAIL_ERROR_NONE;
	int   is_alpha = 0;
	int   length_field_name = 0;
	int   length_value = 0;
	char  result_rule_string[QUERY_SIZE] = { 0 , };
	char *mod_field_name_string = NULL;
	char *mod_value_string = NULL;
	char *temp_field_name_string = NULL;
	char *temp_key_value_string = NULL;

	if(input_list_filter_rule == NULL || output_string == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return  EMAIL_ERROR_INVALID_PARAM;
	}

	temp_field_name_string = _get_mail_field_name_by_attribute_type(input_list_filter_rule->target_attribute);

	if(temp_field_name_string == NULL) {
		EM_DEBUG_EXCEPTION("Invalid target_attribute [%d]", input_list_filter_rule->target_attribute);
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if( _get_key_value_string_for_list_filter_rule(input_list_filter_rule, &temp_key_value_string) != EMAIL_ERROR_NONE || temp_key_value_string == NULL) {
		EM_DEBUG_EXCEPTION("_get_key_value_string_for_list_filter_rule failed");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	length_field_name = strlen(temp_field_name_string);
	length_value      = strlen(temp_key_value_string);

	switch(input_list_filter_rule->target_attribute) {
	case EMAIL_MAIL_ATTRIBUTE_MAILBOX_NAME            :
	case EMAIL_MAIL_ATTRIBUTE_SUBJECT                 :
	case EMAIL_MAIL_ATTRIBUTE_SERVER_MAILBOX_NAME     :
	case EMAIL_MAIL_ATTRIBUTE_SERVER_MAIL_ID          :
	case EMAIL_MAIL_ATTRIBUTE_MESSAGE_ID              :
	case EMAIL_MAIL_ATTRIBUTE_FROM                    :
	case EMAIL_MAIL_ATTRIBUTE_TO                      :
	case EMAIL_MAIL_ATTRIBUTE_CC                      :
	case EMAIL_MAIL_ATTRIBUTE_BCC                     :
	case EMAIL_MAIL_ATTRIBUTE_PREVIEW_TEXT            :
		is_alpha = 1;
		break;
	default :
		is_alpha = 0;
		break;
	}

	if(is_alpha == 1 && input_list_filter_rule->case_sensitivity == false) {
		length_field_name += strlen("UPPER() ");
		length_value      += strlen("UPPER() ");
		mod_field_name_string = em_malloc(sizeof(char) * length_field_name);
		mod_value_string      = em_malloc(sizeof(char) * length_value);
		SNPRINTF(mod_field_name_string, length_field_name, "UPPER(%s)", temp_field_name_string);
		SNPRINTF(mod_value_string,      length_value, "UPPER(%s)", temp_key_value_string);
		EM_SAFE_FREE(temp_key_value_string);
	}
	else {
		mod_field_name_string = strdup(temp_field_name_string);
		mod_value_string      = temp_key_value_string;
	}

	switch (input_list_filter_rule->rule_type) {
	case EMAIL_LIST_FILTER_RULE_EQUAL     :
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s = %s ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_NOT_EQUAL :
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s = %s ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_LESS_THAN :
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s < %s ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_GREATER_THAN :
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s > %s ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_LESS_THAN_OR_EQUAL :
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s <= %s ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_GREATER_THAN_OR_EQUAL :
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s >= %s ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_INCLUDE   :
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s LIKE %s ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_IN        :
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s IN (%s) ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_NOT_IN    :
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s NOT IN (%s) ", mod_field_name_string, mod_value_string);
		break;
	default :
		EM_DEBUG_EXCEPTION("Invalid rule_type [%d]", input_list_filter_rule->rule_type);
		ret = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	*output_string = strdup(result_rule_string);

FINISH_OFF:
	EM_SAFE_FREE(mod_field_name_string);
	EM_SAFE_FREE(mod_value_string);

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

static int _make_order_rule_string(email_list_sorting_rule_t *input_sorting_rule, char **output_string) {
	EM_DEBUG_FUNC_BEGIN("input_sorting_rule [%p], output_string [%p]", input_sorting_rule, output_string);

	char  result_rule_string[QUERY_SIZE] = { 0 , };
	int   ret = EMAIL_ERROR_NONE;

	if(input_sorting_rule->force_boolean_check) {
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s = 0 ", _get_mail_field_name_by_attribute_type(input_sorting_rule->target_attribute));
	}
	else
		EM_SAFE_STRCPY(result_rule_string, _get_mail_field_name_by_attribute_type(input_sorting_rule->target_attribute));

	switch (input_sorting_rule->sort_order) {
		case EMAIL_SORT_ORDER_ASCEND     :
			EM_SAFE_STRCAT(result_rule_string, " ASC ");
			break;

		case EMAIL_SORT_ORDER_DESCEND    :
			EM_SAFE_STRCAT(result_rule_string, " DESC ");
			break;

		default :
			EM_DEBUG_EXCEPTION("Invalid sort_order [%d]", input_sorting_rule->sort_order);
			ret = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}

	*output_string = strdup(result_rule_string);

FINISH_OFF:
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_write_conditional_clause_for_getting_mail_list(email_list_filter_t *input_filter_list, int input_filter_count, email_list_sorting_rule_t *input_sorting_rule_list, int input_sorting_rule_count, int input_start_index, int input_limit_count, char **output_conditional_clause)
{
	EM_DEBUG_FUNC_BEGIN("input_filter_list [%p], input_filter_count[%d], input_sorting_rule_list[%p], input_sorting_rule_count [%d], input_start_index [%d], input_limit_count [%d], output_conditional_clause [%p]", input_filter_list, input_filter_count, input_sorting_rule_list, input_sorting_rule_count, input_start_index, input_limit_count, output_conditional_clause);
	int ret = EMAIL_ERROR_NONE;
	int i = 0;
	char conditional_clause_string[QUERY_SIZE] = {0, };
	char *result_string_for_a_item = NULL;

	if ( (input_filter_count > 0 && !input_filter_list) || (input_sorting_rule_count > 0 && !input_sorting_rule_list) || output_conditional_clause == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if(input_filter_count > 0) {
		strcpy(conditional_clause_string,  " WHERE ");

		for ( i = 0; i < input_filter_count; i++) {
			switch (input_filter_list[i].list_filter_item_type) {
			case EMAIL_LIST_FILTER_ITEM_RULE :
				EM_DEBUG_LOG("[%d]list_filter_item_type is EMAIL_LIST_FILTER_ITEM_RULE", i);
				_make_filter_rule_string(&(input_filter_list[i].list_filter_item.rule), &result_string_for_a_item);
				break;

			case EMAIL_LIST_FILTER_ITEM_OPERATOR :
				EM_DEBUG_LOG("[%d]list_filter_item_type is EMAIL_LIST_FILTER_ITEM_OPERATOR", i);
				switch(input_filter_list[i].list_filter_item.operator_type) {
				case EMAIL_LIST_FILTER_OPERATOR_AND :
					result_string_for_a_item = strdup("AND ");
					break;
				case EMAIL_LIST_FILTER_OPERATOR_OR :
					result_string_for_a_item = strdup("OR ");
					break;
				case EMAIL_LIST_FILTER_OPERATOR_LEFT_PARENTHESIS :
					result_string_for_a_item = strdup(" (");
					break;
				case EMAIL_LIST_FILTER_OPERATOR_RIGHT_PARENTHESIS :
					result_string_for_a_item = strdup(") ");
					break;
				}
				break;

			default :
				EM_DEBUG_EXCEPTION("Invalid list_filter_item_type [%d]", input_filter_list[i].list_filter_item_type);
				ret = EMAIL_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}

			if(result_string_for_a_item == NULL) {
				EM_DEBUG_EXCEPTION("result_string_for_a_item is null");
				ret = EMAIL_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}

			if(strlen(conditional_clause_string) + strlen(result_string_for_a_item) >= QUERY_SIZE) {
				EM_DEBUG_EXCEPTION("Query is too long");
				ret = EMAIL_ERROR_DATA_TOO_LONG;
				goto FINISH_OFF;
			}
			strcat(conditional_clause_string, result_string_for_a_item);
			EM_SAFE_FREE(result_string_for_a_item);
		}
	}

	if(input_sorting_rule_count > 0) {
		strcat(conditional_clause_string, "ORDER BY ");

		for ( i = 0; i < input_sorting_rule_count; i++) {
			if( (ret = _make_order_rule_string(&input_sorting_rule_list[i], &result_string_for_a_item)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("_make_order_rule_string failed. [%d]", ret);
				goto FINISH_OFF;
			}
			if(i > 0)
				strcat(conditional_clause_string, ", ");
			strcat(conditional_clause_string, result_string_for_a_item);
			EM_SAFE_FREE(result_string_for_a_item);
		}
	}

	*output_conditional_clause = strdup(conditional_clause_string);

FINISH_OFF:
	EM_SAFE_FREE(result_string_for_a_item);

   	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_free_list_filter(email_list_filter_t **input_filter_list, int input_filter_count)
{
	EM_DEBUG_FUNC_BEGIN("input_filter_list [%p], input_filter_count[%d]", input_filter_list, input_filter_count);
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	email_list_filter_t *temp_filter_list = NULL;

	EM_IF_NULL_RETURN_VALUE(input_filter_list, EMAIL_ERROR_INVALID_PARAM);

	for ( i = 0; i < input_filter_count; i++) {
		temp_filter_list = (*input_filter_list) + i;
		if(temp_filter_list && temp_filter_list->list_filter_item_type == EMAIL_LIST_FILTER_ITEM_RULE) {
			switch(temp_filter_list->list_filter_item.rule.target_attribute) {
			case EMAIL_MAIL_ATTRIBUTE_MAILBOX_NAME :
			case EMAIL_MAIL_ATTRIBUTE_SUBJECT :
			case EMAIL_MAIL_ATTRIBUTE_SERVER_MAILBOX_NAME :
			case EMAIL_MAIL_ATTRIBUTE_SERVER_MAIL_ID :
			case EMAIL_MAIL_ATTRIBUTE_MESSAGE_ID :
			case EMAIL_MAIL_ATTRIBUTE_FROM :
			case EMAIL_MAIL_ATTRIBUTE_TO :
			case EMAIL_MAIL_ATTRIBUTE_CC :
			case EMAIL_MAIL_ATTRIBUTE_BCC :
			case EMAIL_MAIL_ATTRIBUTE_FILE_PATH_PLAIN :
			case EMAIL_MAIL_ATTRIBUTE_FILE_PATH_HTML :
			case EMAIL_MAIL_ATTRIBUTE_PREVIEW_TEXT :
				EM_SAFE_FREE(temp_filter_list->list_filter_item.rule.key_value.string_type_value);
				break;
			default :
				break;
			}
		}
	}

	free(*input_filter_list);
	*input_filter_list = NULL;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emstorage_add_certificate(emstorage_certificate_tbl_t *certificate, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("certificate:[%p], transaction:[%d], err_code:[%p]", certificate, transaction, err_code);

	if (!certificate) {
		EM_DEBUG_EXCEPTION("certificate:[%p], transaction:[%d], err_code:[%p]", certificate, transaction, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;

		return false;
	}

	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__
	char cert_password_file_name[MAX_PW_FILE_NAME_LENGTH];
#endif
	sqlite3 *local_db_handle = emstorage_get_db_connection();

	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	char *sql = "SELECT max(rowid) FROM mail_certificate_tbl;";
	char **result = NULL;

	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL==result[1]) rc = 1;
	else rc = atoi(result[1])+1;
	sqlite3_free_table(result);

	certificate->certificate_id = rc;
#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__
	if ((error = _get_cert_password_file_name(certificate->certificate_id, cert_password_file_name)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_storage_get_password_file_name failed.");
		goto FINISH_OFF;
	}
#endif
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_certificate_tbl VALUES "
		"(	  "
		"   ?  "		/* Index of certificate */
		"  , ? "		/* Select the account */
		"  , ? "		/* Year of issue */
		"  , ? "		/* Month of issue */
		"  , ? "		/* Day of issue */
		"  , ? "		/* Year of expiration */
		"  , ? "		/* Month of expiration */
		"  , ? "		/* Day of expiration */
		"  , ? "		/* Organization of issue */
		"  , ? "		/* Email address */
		"  , ? "		/* Subject of certificate */
		"  , ? "		/* Name of saved certificate */
		") ");


	/*  rc = sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG(">>>> SQL STMT [ %s ] ", sql_query_string);
	_bind_stmt_field_data_int(hStmt, CERTFICATE_BIND_TYPE_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->certificate_id);
	_bind_stmt_field_data_int(hStmt, ISSUE_YEAR_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->issue_year);
	_bind_stmt_field_data_int(hStmt, ISSUE_MONTH_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->issue_month);
	_bind_stmt_field_data_int(hStmt, ISSUE_DAY_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->issue_day);
	_bind_stmt_field_data_int(hStmt, EXPIRE_YEAR_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->expiration_year);
	_bind_stmt_field_data_int(hStmt, EXPIRE_MONTH_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->expiration_month);
	_bind_stmt_field_data_int(hStmt, EXPIRE_DAY_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->expiration_day);
	_bind_stmt_field_data_string(hStmt, ISSUE_ORGANIZATION_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->issue_organization_name, 0, ISSUE_ORGANIZATION_LEN_IN_MAIL_CERTIFICATE_TBL);
	_bind_stmt_field_data_string(hStmt, EMAIL_ADDRESS_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->email_address, 0, EMAIL_ADDRESS_LEN_IN_MAIL_CERTIFICATE_TBL);
	_bind_stmt_field_data_string(hStmt, SUBJECT_STRING_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->subject_str, 0, SUBJECT_STRING_LEN_IN_MAIL_CERTIFICATE_TBL);
	_bind_stmt_field_data_string(hStmt, FILE_PATH_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->filepath, 0, FILE_NAME_LEN_IN_MAIL_CERTIFICATE_TBL);
	/*  rc = sqlite3_step(hStmt); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);

	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d, errmsg = %s.", rc, sqlite3_errmsg(local_db_handle)));
#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__
	if (ssm_write_buffer(certificate->password, strlen(certificate->password), cert_password_file_name, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION("ssm_write_buffer failed - Private certificate password : [%s]", cert_password_file_name);
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
#endif
	ret = true;

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);

	if (!emstorage_notify_storage_event(NOTI_CERTIFICATE_ADD, certificate->certificate_id, 0, NULL, 0))
		EM_DEBUG_EXCEPTION("emstorage_notify_storage_event(NOTI_CERTIFICATE_ADD] : Notification failed");

FINISH_OFF:

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
	else
		EM_DEBUG_LOG("hStmt is NULL!!!");

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_free_certificate(emstorage_certificate_tbl_t **certificate_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("certificate_list[%p], count[%d], err_code[%p]", certificate_list, count, err_code);
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	if (count > 0)  {
		if (!certificate_list || !*certificate_list)  {
			EM_DEBUG_EXCEPTION("certificate_list[%p], count[%d]", certificate_list, count);
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		emstorage_certificate_tbl_t *p = *certificate_list;
		int i = 0;
		
		for (; i < count; i++)  {
			EM_SAFE_FREE(p[i].issue_organization_name);
			EM_SAFE_FREE(p[i].email_address);
			EM_SAFE_FREE(p[i].subject_str);
			EM_SAFE_FREE(p[i].filepath);
			EM_SAFE_FREE(p[i].password);
		}
		
		EM_SAFE_FREE(p); 
		*certificate_list = NULL;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_certificate_list(int *select_num, emstorage_certificate_tbl_t **certificate_list, int transaction, int with_password, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int i = 0, count = 0, rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	emstorage_certificate_tbl_t *p_data_tbl = NULL;

	DB_STMT hStmt = NULL;

	if (!select_num || !certificate_list)  {
		EM_DEBUG_EXCEPTION("select_num[%p], account_list[%p]", select_num, certificate_list);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	char sql_query_string[QUERY_SIZE] = {0, };
	char *sql = "SELECT count(*) FROM mail_certificate_tbl;";
	char **result;

	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	count = atoi(result[1]);
	sqlite3_free_table(result);

	if (!count) {
		EM_DEBUG_EXCEPTION("no account found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("count = %d", rc);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_certificate_tbl ORDER BY account_id");

	/*  rc = sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL);   */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG("After sqlite3_prepare_v2 hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no account found...");

		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		count = 0;
		ret = true;
		goto FINISH_OFF;
	}

	if (!(p_data_tbl = (emstorage_certificate_tbl_t *)malloc(sizeof(emstorage_certificate_tbl_t) * count)))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memset(p_data_tbl, 0x00, sizeof(emstorage_certificate_tbl_t) * count);
	for (i = 0; i < count; i++)  {
		/*  get recordset */
		_get_stmt_field_data_int(hStmt,  &(p_data_tbl[i].certificate_id), CERTFICATE_BIND_TYPE_IDX_IN_MAIL_CERTIFICATE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].issue_year), ISSUE_YEAR_IDX_IN_MAIL_CERTIFICATE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].issue_month), ISSUE_MONTH_IDX_IN_MAIL_CERTIFICATE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].issue_day), ISSUE_DAY_IDX_IN_MAIL_CERTIFICATE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].expiration_year), EXPIRE_YEAR_IDX_IN_MAIL_CERTIFICATE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].expiration_month), EXPIRE_MONTH_IDX_IN_MAIL_CERTIFICATE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].expiration_day), EXPIRE_DAY_IDX_IN_MAIL_CERTIFICATE_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].issue_organization_name), 0, ISSUE_ORGANIZATION_IDX_IN_MAIL_CERTIFICATE_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].email_address), 0, EMAIL_ADDRESS_IDX_IN_MAIL_CERTIFICATE_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].subject_str), 0, SUBJECT_STRING_IDX_IN_MAIL_CERTIFICATE_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].filepath), 0, FILE_PATH_IDX_IN_MAIL_CERTIFICATE_TBL);
		if (with_password == true) {
#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__
			/*  get password from the secure storage */
			char cert_password_file_name[MAX_PW_FILE_NAME_LENGTH];

			EM_SAFE_FREE(p_data_tbl[i].password);

			/*  get password file name */
			if ((error = _get_cert_password_file_name(p_data_tbl[i].certificate_id, cert_password_file_name)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("em_storage_get_password_file_name failed.");
				goto FINISH_OFF;
			}

			/*  read password from secure storage */
			if ((error = _read_password_from_secure_storage(cert_password_file_name, &(p_data_tbl[i].password))) < 0) {
				EM_DEBUG_EXCEPTION("_read_password_from_secure_storage() failed...");
				goto FINISH_OFF;
			}
			EM_DEBUG_LOG("recv_password_file_name[%s], password[%s]", cert_password_file_name,  p_data_tbl[i].password);
#endif
		}

		/*  rc = sqlite3_step(hStmt); */
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_LOG("after sqlite3_step(), i = %d, rc = %d.", i,  rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
	}

	ret = true;

FINISH_OFF:
	if (ret == true)  {
		*certificate_list = p_data_tbl;
		*select_num = count;
		EM_DEBUG_LOG("COUNT : %d", count);
	}
	else if (p_data_tbl != NULL)
		emstorage_free_certificate(&p_data_tbl, count, NULL);
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_certificate_by_email_address(char *email_address, emstorage_certificate_tbl_t **certificate, int transaction, int with_password, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("email_address[%s], certificate[%p], transaction[%d], err_code[%p]", email_address, certificate, transaction, err_code);

	if (!certificate)  {
		EM_DEBUG_EXCEPTION("email_address[%s], certificate[%p]", email_address, certificate);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	emstorage_certificate_tbl_t *p_data_tbl = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	int rc = -1;
	int sql_len = 0;
#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__
	char cert_password_file_name[MAX_PW_FILE_NAME_LENGTH];
#endif		
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	/*  Make query string */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT ");
	sql_len = strlen(sql_query_string);
		
	/*  dummy value, FROM WHERE clause */
	SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len, "* FROM mail_certificate_tbl WHERE email_address = '%s'", email_address);

	/*  FROM clause */
	EM_DEBUG_LOG("query = [%s]", sql_query_string);

	/*  execute a sql and count rows */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no matched certificate found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

	/*  Assign query result to structure */
	if (!(p_data_tbl = (emstorage_certificate_tbl_t *)malloc(sizeof(emstorage_certificate_tbl_t))))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(p_data_tbl, 0x00, sizeof(emstorage_certificate_tbl_t));
	_get_stmt_field_data_int(hStmt,  &(p_data_tbl->certificate_id), CERTFICATE_BIND_TYPE_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->issue_year), ISSUE_YEAR_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->issue_month), ISSUE_MONTH_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->issue_day), ISSUE_DAY_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->expiration_year), EXPIRE_YEAR_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->expiration_month), EXPIRE_MONTH_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->expiration_day), EXPIRE_DAY_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->issue_organization_name), 0, ISSUE_ORGANIZATION_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->email_address), 0, EMAIL_ADDRESS_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->subject_str), 0, SUBJECT_STRING_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->filepath), 0, FILE_PATH_IDX_IN_MAIL_CERTIFICATE_TBL);

	if (with_password) {
#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__
		/*  get password file name */
		if ((error = _get_cert_password_file_name(p_data_tbl->certificate_id, cert_password_file_name)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_storage_get_password_file_name failed.");
			goto FINISH_OFF;
		}		

		/*  read password from secure storage */
		if ((error = _read_password_from_secure_storage(cert_password_file_name, &(p_data_tbl->password))) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION(" _read_password_from_secure_storage()  failed...");
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG("cert_password_file_name[%s], password[%s]", cert_password_file_name,  p_data_tbl->password);
#endif
	}
	ret = true;

FINISH_OFF:
	if (ret == true)
		*certificate = p_data_tbl;
	else {
		if (p_data_tbl)
			emstorage_free_certificate((emstorage_certificate_tbl_t **)&p_data_tbl, 1, NULL);
	}
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_certificate_by_index(int index, emstorage_certificate_tbl_t **certificate, int transaction, int with_password, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("index[%d], certificate[%p], transaction[%d], err_code[%p]", index, certificate, transaction, err_code);

	if (!certificate)  {
		EM_DEBUG_EXCEPTION("index[%d], account[%p]", index, certificate);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	emstorage_certificate_tbl_t *p_data_tbl = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	int rc = -1;
	int sql_len = 0;
#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__
	char cert_password_file_name[MAX_PW_FILE_NAME_LENGTH];
#endif		
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	/*  Make query string */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT ");
	sql_len = strlen(sql_query_string);
		
	/*  dummy value, FROM WHERE clause */
	SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len, "* FROM mail_certificate_tbl WHERE certificate_id = %d", index);

	/*  FROM clause */
	EM_DEBUG_LOG("query = [%s]", sql_query_string);

	/*  execute a sql and count rows */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no matched certificate found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

	/*  Assign query result to structure */
	if (!(p_data_tbl = (emstorage_certificate_tbl_t *)malloc(sizeof(emstorage_certificate_tbl_t))))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(p_data_tbl, 0x00, sizeof(emstorage_certificate_tbl_t));

	_get_stmt_field_data_int(hStmt,  &(p_data_tbl->certificate_id), CERTFICATE_BIND_TYPE_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->issue_year), ISSUE_YEAR_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->issue_month), ISSUE_MONTH_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->issue_day), ISSUE_DAY_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->expiration_year), EXPIRE_YEAR_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->expiration_month), EXPIRE_MONTH_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->expiration_day), EXPIRE_DAY_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->issue_organization_name), 0, ISSUE_ORGANIZATION_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->email_address), 0, EMAIL_ADDRESS_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->subject_str), 0, SUBJECT_STRING_IDX_IN_MAIL_CERTIFICATE_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->filepath), 0, FILE_PATH_IDX_IN_MAIL_CERTIFICATE_TBL);

	if (with_password) {
#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__
		/*  get password file name */
		if ((error = _get_cert_password_file_name(p_data_tbl->certificate_id, cert_password_file_name)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_storage_get_password_file_name failed.");
			goto FINISH_OFF;
		}		

		/*  read password from secure storage */
		if ((error = _read_password_from_secure_storage(cert_password_file_name, &(p_data_tbl->password))) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION(" _read_password_from_secure_storage()  failed...");
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG("cert_password_file_name[%s], password[%s]", cert_password_file_name,  p_data_tbl->password);
#endif
	}
	ret = true;

FINISH_OFF:
	if (ret == true)
		*certificate = p_data_tbl;
	else {
		if (p_data_tbl)
			emstorage_free_certificate((emstorage_certificate_tbl_t **)&p_data_tbl, 1, NULL);
	}
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
	
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_certificate(int certificate_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("certificate_id[%d], transaction[%d], err_code[%p]", certificate_id, transaction, err_code);
	
	if (certificate_id < 1)  {	
		EM_DEBUG_EXCEPTION(" certificate_id[%d]", certificate_id);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	/*  TODO : delete password files - file names can be obtained from db or a rule that makes a name */
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__	
	char cert_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	/*  get password file name */
	if ((error = _get_cert_password_file_name(certificate_id, cert_password_file_name)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_storage_get_password_file_name failed.");
		goto FINISH_OFF;
	}
#endif
	/*  delete from db */
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_certificate_tbl WHERE certificate_id = %d", certificate_id);
 
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  validate account existence */
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no matched certificate found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}
#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__	
	/*  delete from secure storage */
	if (ssm_delete_file(cert_password_file_name,  SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION(" ssm_delete_file failed -cert password : file[%s]", cert_password_file_name);
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;		
	}
#endif	
	ret = true;

FINISH_OFF:
 
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_update_certificate(int certificate_id, emstorage_certificate_tbl_t *certificate, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("certificate_id[%d], certificate[%p], transaction[%d], err_code[%p]", certificate_id, certificate, transaction, err_code);
	
	if (certificate_id < 1)  {	
		EM_DEBUG_EXCEPTION(" certificate_id[%d]", certificate_id);
		
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	
	int error = EMAIL_ERROR_NONE;
	int rc, ret = false;
 
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__	
	char cert_password_file_name[MAX_PW_FILE_NAME_LENGTH];
#endif
	sqlite3 *local_db_handle = emstorage_get_db_connection();
	EMSTORAGE_START_WRITE_TRANSACTION(transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_certificate_tbl SET"
		" issue_year = ?"
		", issue_month = ?"		/* Index of certificate */
		", issue_day = ?"		/* Select the account */
		", expiration_year = ?"		/* Year of issue */
		", expiration_month = ?"		/* Month of issue */
		", expiration_day = ?"		/* Day of issue */
		", issue_organization_name = ?"		/* Year of expiration */
		", email_address = ?"		/* Month of expiration */
		", subject_str = ?"		/* Day of expiration */
		", filepath = ?"		/* Organization of issue */
		", password = ?"
		" WHERE certificate_id = ?");
        
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("After sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	_bind_stmt_field_data_int(hStmt, ISSUE_YEAR_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->issue_year);
	_bind_stmt_field_data_int(hStmt, ISSUE_MONTH_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->issue_month);
	_bind_stmt_field_data_int(hStmt, ISSUE_DAY_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->issue_day);
	_bind_stmt_field_data_int(hStmt, EXPIRE_YEAR_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->expiration_year);
	_bind_stmt_field_data_int(hStmt, EXPIRE_MONTH_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->expiration_month);
	_bind_stmt_field_data_int(hStmt, EXPIRE_DAY_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->expiration_day);
	_bind_stmt_field_data_string(hStmt, ISSUE_ORGANIZATION_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->issue_organization_name, 0, ISSUE_ORGANIZATION_LEN_IN_MAIL_CERTIFICATE_TBL);
	_bind_stmt_field_data_string(hStmt, EMAIL_ADDRESS_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->email_address, 0, EMAIL_ADDRESS_LEN_IN_MAIL_CERTIFICATE_TBL);
	_bind_stmt_field_data_string(hStmt, SUBJECT_STRING_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->subject_str, 0, SUBJECT_STRING_LEN_IN_MAIL_CERTIFICATE_TBL);
	_bind_stmt_field_data_string(hStmt, FILE_PATH_IDX_IN_MAIL_CERTIFICATE_TBL, certificate->filepath, 0, FILE_NAME_LEN_IN_MAIL_CERTIFICATE_TBL);
 
	/*  rc = sqlite3_step(hStmt); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EMAIL_ERROR_MAIL_MEMORY_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	/*  validate account existence */
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION(" no matched account found...");
	
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}
	
#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__	
	/*  get password file name */
	if ((error = _get_cert_password_file_name(certificate->certificate_id, cert_password_file_name)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_storage_get_password_file_name failed.");
		goto FINISH_OFF;
	}		

	/*  save passwords to the secure storage */
	if (ssm_write_buffer(certificate->password, strlen(certificate->password), cert_password_file_name, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION("ssm_write_buffer failed - Private certificate password : [%s]", cert_password_file_name);
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
#endif	
	if (!emstorage_notify_storage_event(NOTI_CERTIFICATE_UPDATE, certificate->certificate_id, 0, NULL, 0))
		EM_DEBUG_EXCEPTION(" emstorage_notify_storage_event[ NOTI_CERTIFICATE_UPDATE] : Notification Failed >>> ");
	
	ret = true;
	
FINISH_OFF:
 
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/*EOF*/
