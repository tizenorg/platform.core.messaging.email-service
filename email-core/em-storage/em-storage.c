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
 * File: em_storage.c
 * Desc: EMail Framework Storage Library on Sqlite3
 *
 * Auth:
 *
 * History:
 *	2007.03.02 : created
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
#include "em-core-utils.h"
#include <pthread.h>
#include "em-storage.h"
#include "emf-dbglog.h"
#include "emf-types.h"

#define DB_STMT sqlite3_stmt *

#define SETTING_MEMORY_TEMP_FILE_PATH "/tmp/email_tmp_file"

#define EMF_STORAGE_CHANGE_NOTI       "User.Email.StorageChange"
#define EMF_NETOWRK_CHANGE_NOTI       "User.Email.NetworkStatus"

#define CONTENT_DATA                  "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset="
#define CONTENT_TYPE_DATA             "<meta http-equiv=\"Content-Type\" content=\"text/html; charset="

#ifndef true
#define true		1
#define false		0
#endif

#define FLAG_SEEN       0x01
#define FLAG_DELETED    0x02
#define FLAG_FLAGGED    0x04
#define FLAG_ANSWERED   0x08
#define FLAG_OLD        0x10
#define FLAG_DRAFT      0x20

#undef close

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
#define DATETIME_LEN_IN_MAIL_TBL                        128
#define MESSAGE_ID_LEN_IN_MAIL_TBL                      256
#define FROM_CONTACT_NAME_LEN_IN_MAIL_TBL               3999
#define FROM_EMAIL_ADDRESS_LEN_IN_MAIL_TBL              3999
#define TO_CONTACT_NAME_LEN_IN_MAIL_TBL                 3999
#define TO_EMAIL_ADDRESS_LEN_IN_MAIL_TBL                3999
#define MAILBOX_LEN_IN_MAIL_MEETING_TBL                 128
#define LOCATION_LEN_IN_MAIL_MEETING_TBL                1024
#define GLOBAL_OBJECT_ID_LEN_IN_MAIL_MEETING_TBL        128
#define STANDARD_NAME_LEN_IN_MAIL_MEETING_TBL           32
#define DAYLIGHT_NAME_LEN_IN_MAIL_MEETING_TBL           32
#define PREVIEWBODY_LEN_IN_MAIL_TBL                     512 

/*  this define is used for query to change data (delete, insert, update) */
#define EM_STORAGE_START_WRITE_TRANSACTION(transaction_flag, error_code) \
	if (transaction_flag)\
	{\
		em_storage_shm_mutex_timedlock(&mapped_for_db_lock, 2);\
		if (em_storage_begin_transaction(NULL, NULL, &error_code) == false) \
		{\
			EM_DEBUG_EXCEPTION("em_storage_begin_transaction() error[%d]", error_code);\
			goto FINISH_OFF;\
		}\
	}

/*  this define is used for query to change data (delete, insert, update) */
#define EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction_flag, result_code, error_code) \
	if (transaction_flag)\
	{\
		if (result_code == true)\
		{\
			if (em_storage_commit_transaction(NULL, NULL, NULL) == false)\
			{\
				error_code = EM_STORAGE_ERROR_DB_FAILURE;\
				result_code = false;\
			}\
		}\
		else\
		{\
			if (em_storage_rollback_transaction(NULL, NULL, NULL) == false)\
				error_code = EM_STORAGE_ERROR_DB_FAILURE;\
		}\
		em_storage_shm_mutex_unlock(&mapped_for_db_lock);\
	}

/*  this define is used for query to read (select) */
#define EM_STORAGE_START_READ_TRANSACTION(transaction_flag) \
	if (transaction_flag)\
	{\
		/*em_storage_shm_mutex_timedlock(&mapped_for_db_lock, 2);*/\
	}

/*  this define is used for query to read (select) */
#define EM_STORAGE_FINISH_READ_TRANSACTION(transaction_flag) \
	if (transaction_flag)\
	{\
		/*em_storage_shm_mutex_unlock(&mapped_for_db_lock);*/\
	}

/*  for safety DB operation */
static char g_db_path[248] = {0x00, };


#ifdef __FEATURE_USE_PTHREAD__
static pthread_mutex_t _transactionBeginLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _transactionEndLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _dbus_noti_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _db_handle_lock = PTHREAD_MUTEX_INITIALIZER;
#else /*  __FEATURE_USE_PTHREAD__ */
static GMutex *_transactionBeginLock = NULL;
static GMutex *_transactionEndLock = NULL;
static GMutex *_dbus_noti_lock = NULL;
static GMutex *_db_handle_lock = NULL;
#endif /*  __FEATURE_USE_PTHREAD__ */

#define	_MULTIPLE_DB_HANDLE

#ifdef _MULTIPLE_DB_HANDLE

#define _DISCONNECT_DB			/* db_util_close(_db_handle); */

typedef struct
{
#ifdef __FEATURE_USE_PTHREAD__
	pthread_t 	thread_id;
#else
	/*  TODO : must write codes for g_thread */
#endif
	sqlite3 *db_handle;
} db_handle_t;

#define MAX_DB_CLIENT 100

/* static int _db_handle_count = 0; */
db_handle_t _db_handle_list[MAX_DB_CLIENT] = {{0, 0}, };

sqlite3 *em_storage_get_db_handle()
{	
	EM_DEBUG_FUNC_BEGIN();
	int i;
	pthread_t current_thread_id = THREAD_SELF();
	sqlite3 *result_db_handle = NULL;

	ENTER_CRITICAL_SECTION(_db_handle_lock);
	for (i = 0; i < MAX_DB_CLIENT; i++)
	{
		if (pthread_equal(current_thread_id, _db_handle_list[i].thread_id))
		{
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

int em_storage_set_db_handle(sqlite3 *db_handle)
{	
	EM_DEBUG_FUNC_BEGIN();
	int i, error_code = EMF_ERROR_MAX_EXCEEDED;
	pthread_t current_thread_id = THREAD_SELF();

	ENTER_CRITICAL_SECTION(_db_handle_lock);
	for (i = 0; i < MAX_DB_CLIENT; i++)	{
		if (_db_handle_list[i].thread_id == 0) {
			_db_handle_list[i].thread_id = current_thread_id;
			_db_handle_list[i].db_handle = db_handle;
			EM_DEBUG_LOG("current_thread_id [%d], index [%d]", current_thread_id, i);
			error_code =  EMF_ERROR_NONE;
			break;
		}
	}
	LEAVE_CRITICAL_SECTION(_db_handle_lock);

	if (error_code == EMF_ERROR_MAX_EXCEEDED)
		EM_DEBUG_EXCEPTION("Exceeded the limitation of db client. Can't find empty slot in _db_handle_list.");

	EM_DEBUG_FUNC_END("error_code [%d]", error_code);
	return error_code;
}

int em_storage_remove_db_handle()
{	
	EM_DEBUG_FUNC_BEGIN();
	int i, error_code = EMF_ERROR_MAX_EXCEEDED;
	ENTER_CRITICAL_SECTION(_db_handle_lock);
	for (i = 0; i < MAX_DB_CLIENT; i++)
	{
		if (_db_handle_list[i].thread_id == THREAD_SELF())
		{
			_db_handle_list[i].thread_id = 0;
			_db_handle_list[i].db_handle = NULL;
			EM_DEBUG_LOG("index [%d]", i);
			error_code = EMF_ERROR_NONE;
			break;
		}
	}
	LEAVE_CRITICAL_SECTION(_db_handle_lock);
	
	if (error_code == EMF_ERROR_MAX_EXCEEDED)
		EM_DEBUG_EXCEPTION("Can't find proper thread_id");

	EM_DEBUG_FUNC_END("error_code [%d]", error_code);
	return error_code;
}


int em_storage_reset_db_handle_list()
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
	return EMF_ERROR_NONE;
}

sqlite3 *em_storage_get_db_connection()
{
	sqlite3 *tmp_db_handle = em_storage_get_db_handle(); 
	if (NULL == tmp_db_handle)
		tmp_db_handle = em_storage_db_open(NULL); 
	return tmp_db_handle;
}


#else	/*  _MULTIPLE_DB_HANDLE */
#define _DISCONNECT_DB			/* db_util_close(_db_handle); */

sqlite3 *_db_handle = NULL;

sqlite3 *em_storage_get_db_connection()
{
	if (NULL == _db_handle)
		em_storage_db_open(NULL); 
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
#define EM_STORAGE_PROTECTED_FUNC_CALL(function_call, return_value) \
	{  em_storage_shm_mutex_timedlock(&mapped_for_db_lock, 2); return_value = function_call; em_storage_shm_mutex_unlock(&mapped_for_db_lock); }
#else /*  __FEATURE_USE_SHARED_MUTEX_FOR_PROTECTED_FUNC_CALL__ */
#define EM_STORAGE_PROTECTED_FUNC_CALL(function_call, return_value) \
	{  return_value = function_call; }
#endif /*  __FEATURE_USE_SHARED_MUTEX_FOR_PROTECTED_FUNC_CALL__ */

EXPORT_API int em_storage_shm_file_init(const char *shm_file_name)
{
	EM_DEBUG_FUNC_BEGIN("shm_file_name [%p]", shm_file_name);

	if(!shm_file_name) {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		return EM_STORAGE_ERROR_INVALID_PARAM;
	}

	int fd = shm_open(shm_file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP); /*  note: permission is not working */

	if (fd > 0) {
		fchmod(fd, 0666);
		EM_DEBUG_LOG("** Create SHM FILE **"); 
		if (ftruncate(fd, sizeof(mmapped_t)) != 0) {
			EM_DEBUG_EXCEPTION("ftruncate failed: %s", strerror(errno));
			return EM_STORAGE_ERROR_SYSTEM_FAILURE;
		}
		
		mmapped_t *m = (mmapped_t *)mmap(NULL, sizeof(mmapped_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (m == MAP_FAILED) {
			EM_DEBUG_EXCEPTION("mmap failed: %s", strerror(errno));
			return EM_STORAGE_ERROR_SYSTEM_FAILURE;
		}

		m->data = 0;

		pthread_mutexattr_t mattr; 
		pthread_mutexattr_init(&mattr);
		pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
		pthread_mutexattr_setrobust_np(&mattr, PTHREAD_MUTEX_ROBUST_NP);
		pthread_mutex_init(&(m->mutex), &mattr);
		pthread_mutexattr_destroy(&mattr);
	}
	else {
		EM_DEBUG_EXCEPTION("shm_open failed: %s", strerror(errno));
		return EM_STORAGE_ERROR_SYSTEM_FAILURE;
	}
	close(fd);
	EM_DEBUG_FUNC_END();
	return EM_STORAGE_ERROR_NONE;
}

int em_storage_shm_file_destroy(const char *shm_file_name)
{
	EM_DEBUG_FUNC_BEGIN("shm_file_name [%p]", shm_file_name);
	if(!shm_file_name) {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		return EM_STORAGE_ERROR_INVALID_PARAM;
	}

	if (shm_unlink(shm_file_name) != 0)
		EM_DEBUG_EXCEPTION("shm_unlink failed: %s", strerror(errno));
	EM_DEBUG_FUNC_END();
	return EM_STORAGE_ERROR_NONE;
}

static int em_storage_shm_mutex_init(const char *shm_file_name, int *param_shm_fd, mmapped_t **param_mapped)
{
	EM_DEBUG_FUNC_BEGIN("shm_file_name [%p] param_shm_fd [%p], param_mapped [%p]", shm_file_name, param_shm_fd, param_mapped);
	
	if(!shm_file_name || !param_shm_fd || !param_mapped) {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		return EM_STORAGE_ERROR_INVALID_PARAM;
	}

	if (!(*param_mapped)) {
		EM_DEBUG_LOG("** mapping begin **");

		if (!(*param_shm_fd)) { /*  open shm_file_name at first. Otherwise, the num of files in /proc/pid/fd will be increasing  */
			*param_shm_fd = shm_open(shm_file_name, O_RDWR, 0);
			if ((*param_shm_fd) == -1) {
				EM_DEBUG_EXCEPTION("FAIL: shm_open(): %s", strerror(errno));
				return EM_STORAGE_ERROR_SYSTEM_FAILURE;
			}
		}
		mmapped_t *tmp = (mmapped_t *)mmap(NULL, sizeof(mmapped_t), PROT_READ|PROT_WRITE, MAP_SHARED, (*param_shm_fd), 0);
		
		if (tmp == MAP_FAILED) {
			EM_DEBUG_EXCEPTION("mmap failed: %s", strerror(errno));
			return EM_STORAGE_ERROR_SYSTEM_FAILURE;
		}
		*param_mapped = tmp;
	}

	EM_DEBUG_FUNC_END();
	return EM_STORAGE_ERROR_NONE;
}

static int em_storage_shm_mutex_timedlock(mmapped_t **param_mapped, int sec)
{
	EM_DEBUG_FUNC_BEGIN("param_mapped [%p], sec [%d]", param_mapped, sec);
	
	if(!param_mapped) {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		return EM_STORAGE_ERROR_INVALID_PARAM;
	}

	struct timespec abs_time;	
	clock_gettime(CLOCK_REALTIME, &abs_time);
	abs_time.tv_sec += sec;

	int err = pthread_mutex_timedlock(&((*param_mapped)->mutex), &abs_time);
	
	if (err == EOWNERDEAD) {
		err = pthread_mutex_consistent_np(&((*param_mapped)->mutex));
		EM_DEBUG_EXCEPTION("Previous owner is dead with lock. Fix mutex : %s", EM_STRERROR(err));
	} 
	else if (err != 0) {
		EM_DEBUG_EXCEPTION("ERROR : %s", EM_STRERROR(err));
		return err;
	}
	
	EM_DEBUG_FUNC_END();
	return EM_STORAGE_ERROR_NONE;
}

void em_storage_shm_mutex_unlock(mmapped_t **param_mapped)
{
	EM_DEBUG_FUNC_BEGIN(); 
	pthread_mutex_unlock(&((*param_mapped)->mutex));
	EM_DEBUG_FUNC_END();
}
/* ------------------------------------------------------------------------------ */


static int _open_counter = 0;
static int g_transaction = false;

static int em_storage_get_password_file_name(int account_id, char *recv_password_file_name, char *send_password_file_name);
static int em_storage_read_password_ss(char *file_name, char **password);


typedef struct {
	const char *object_name;
	unsigned int data_flag;
} emf_db_object_t;

static const emf_db_object_t _g_db_tables[] =
{
	{ "mail_read_mail_uid_tbl", 1},
	{ "mail_tbl", 1},
	{ "mail_attachment_tbl", 1},
	{ NULL,  0},
};

static const emf_db_object_t _g_db_indexes[] =
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
#ifdef __LOCAL_ACTIVITY__
	CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL,
#endif
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
	ACCOUNT_BIND_TYPE_IDX_IN_MAIL_ACCOUNT_TBL = 0,
	ACCOUNT_NAME_IDX_IN_MAIL_ACCOUNT_TBL,
	RECEIVING_SERVER_TYPE_TYPE_IDX_IN_MAIL_ACCOUNT_TBL,
	RECEIVING_SERVER_ADDR_IDX_IN_MAIL_ACCOUNT_TBL,
	EMAIL_ADDR_IDX_IN_MAIL_ACCOUNT_TBL,
	USER_NAME_IDX_IN_MAIL_ACCOUNT_TBL,
	PASSWORD_IDX_IN_MAIL_ACCOUNT_TBL,
	RETRIEVAL_MODE_IDX_IN_MAIL_ACCOUNT_TBL,
	PORT_NUM_IDX_IN_MAIL_ACCOUNT_TBL,
	USE_SECURITY_IDX_IN_MAIL_ACCOUNT_TBL,
	SENDING_SERVER_TYPE_IDX_IN_MAIL_ACCOUNT_TBL,
	SENDING_SERVER_ADDR_IDX_IN_MAIL_ACCOUNT_TBL,
	SENDING_PORT_NUM_IDX_IN_MAIL_ACCOUNT_TBL,
	SENDING_AUTH_IDX_IN_MAIL_ACCOUNT_TBL,
	SENDING_SECURITY_IDX_IN_MAIL_ACCOUNT_TBL,
	SENDING_USER_IDX_IN_MAIL_ACCOUNT_TBL,
	SENDING_PASSWORD_IDX_IN_MAIL_ACCOUNT_TBL,
	DISPLAY_NAME_IDX_IN_MAIL_ACCOUNT_TBL,
	REPLY_TO_ADDR_IDX_IN_MAIL_ACCOUNT_TBL,
	RETURN_ADDR_IDX_IN_MAIL_ACCOUNT_TBL,
	ACCOUNT_ID_IDX_IN_MAIL_ACCOUNT_TBL,
	KEEP_ON_SERVER_IDX_IN_MAIL_ACCOUNT_TBL,
	FLAG1_IDX_IN_MAIL_ACCOUNT_TBL,
	FLAG2_IDX_IN_MAIL_ACCOUNT_TBL,
	POP_BEFORE_SMTP_IDX_IN_MAIL_ACCOUNT_TBL,
	APOP_IDX_IN_MAIL_ACCOUNT_TBL,
	LOGO_ICON_PATH_IDX_IN_MAIL_ACCOUNT_TBL,
	PRESET_ACCOUNT_IDX_IN_MAIL_ACCOUNT_TBL,
	TARGET_STORAGE_IDX_IN_MAIL_ACCOUNT_TBL,
	CHECK_INTERVAL_IDX_IN_MAIL_ACCOUNT_TBL,
	PRIORITY_IDX_IN_MAIL_ACCOUNT_TBL,
	KEEP_LOCAL_COPY_IDX_IN_MAIL_ACCOUNT_TBL,
	REQ_DELIVERY_RECEIPT_IDX_IN_MAIL_ACCOUNT_TBL,
	REQ_READ_RECEIPT_IDX_IN_MAIL_ACCOUNT_TBL,
	DOWNLOAD_LIMIT_IDX_IN_MAIL_ACCOUNT_TBL,
	BLOCK_ADDRESS_IDX_IN_MAIL_ACCOUNT_TBL,
	BLOCK_SUBJECT_IDX_IN_MAIL_ACCOUNT_TBL,
	DISPLAY_NAME_FROM_IDX_IN_MAIL_ACCOUNT_TBL,
	REPLY_WITH_BODY_IDX_IN_MAIL_ACCOUNT_TBL,
	FORWARD_WITH_FILES_IDX_IN_MAIL_ACCOUNT_TBL,
	ADD_MYNAME_CARD_IDX_IN_MAIL_ACCOUNT_TBL,
	ADD_SIGNATURE_IDX_IN_MAIL_ACCOUNT_TBL,
	SIGNATURE_IDX_IN_MAIL_ACCOUNT_TBL,
	ADD_MY_ADDRESS_TO_BCC_IDX_IN_MAIL_ACCOUNT_TBL,
	MY_ACCOUNT_ID_IDX_IN_MAIL_ACCOUNT_TBL,
	INDEX_COLOR_IDX_IN_MAIL_ACCOUNT_TBL,
	SYNC_STATUS_IDX_IN_MAIL_ACCOUNT_TBL,
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
	LOCAL_MBOX_IDX_IN_MAIL_READ_MAIL_UID_TBL,
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
	DEST_MAILBOX_IDX_IN_MAIL_RULE_TBL,
	FLAG1_IDX_IN_MAIL_RULE_TBL,
	FLAG2_IDX_IN_MAIL_RULE_TBL,
};

enum 
{
	MAIL_ID_IDX_IN_MAIL_TBL = 0,
	ACCOUNT_ID_IDX_IN_MAIL_TBL,
	MAILBOX_NAME_IDX_IN_MAIL_TBL,
	MAILBOX_TYPE_IDX_IN_MAIL_TBL,
	SUBJECT_IDX_IN_MAIL_TBL,
	DATETIME_IDX_IN_MAIL_TBL,
	SERVER_MAIL_STATUS_IDX_IN_MAIL_TBL,
	SERVER_MAILBOX_NAME_IDX_IN_MAIL_TBL,
	SERVER_MAIL_ID_IDX_IN_MAIL_TBL,
	MESSAGE_ID_IDX_IN_MAIL_TBL,
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
	FIELD_COUNT_OF_EMF_MAIL_TBL,  /* End of mail_tbl */
};

enum 
{
	ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL = 0,
	ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL,
	ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL,
	ATTACHMENT_SIZE_IDX_IN_MAIL_ATTACHMENT_TBL,
	MAIL_ID_IDX_IN_MAIL_ATTACHMENT_TBL,
	ACCOUNT_ID_IDX_IN_MAIL_ATTACHMENT_TBL,
	MAILBOX_NAME_IDX_IN_MAIL_ATTACHMENT_TBL,
	FILE_YN_IDX_IN_MAIL_ATTACHMENT_TBL,
	FLAG1_IDX_IN_MAIL_ATTACHMENT_TBL,
	FLAG2_IDX_IN_MAIL_ATTACHMENT_TBL,
	FLAG3_IDX_IN_MAIL_ATTACHMENT_TBL,
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
		" account_bind_type, "
		" account_name, "
		" receiving_server_type, "
		" receiving_server_addr, "
		" email_addr, "
		" user_name, "
		" password, "
		" retrieval_mode, "
		" port_num, "
		" use_security, "
		" sending_server_type, "
		" sending_server_addr, "
		" sending_port_num, "
		" sending_auth, "
		" sending_security, "
		" sending_user, "
		" sending_password, "
		" display_name, "
		" reply_to_addr, "
		" return_addr, "
		" account_id, "
		" keep_on_server, "
		" flag1, "
		" flag2, "
		" pop_before_smtp, "
		" apop"	 		   
		", logo_icon_path, "
		" preset_account, "
		" target_storage, "
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
		", my_account_id "
		", index_color "
		", sync_status "
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
		" FROM mail_box_tbl ",
		/*  3. select mail_read_mail_uid_tbl */
		"SELECT  "
		"   account_id,  "
		"   local_mbox,  "
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
		"	dest_mailbox,	"
		"	flag1, "
		"	flag2 "
		" FROM mail_rule_tbl	",
		/*  5. select mail_tbl */
		"SELECT"
		"	mail_id, "
		"	account_id, "
		"	mailbox_name, "
		" mailbox_type, "
		" subject, "
		"	date_time, "
		"	server_mail_status, "
		"	server_mailbox_name, "
		"	server_mail_id, "
		" message_id, "
		"   full_address_from, "
		"   full_address_reply, "
		"   full_address_to, "
		"   full_address_cc, "
		"   full_address_bcc, "
		"   full_address_return, "
		" email_address_sender, "
		" email_address_recipient, "
		" alias_sender, "
		" alias_recipient, "
		"	body_download_status, "
		"	file_path_plain, "
		"	file_path_html, "
		"	mail_size, "
		" flags_seen_field     ,"
		" flags_deleted_field  ,"
		" flags_flagged_field  ,"
		" flags_answered_field ,"
		" flags_recent_field   ,"
		" flags_draft_field    ,"
		" flags_forwarded_field,"
		"	DRM_status, "
		"	priority, "
		"	save_status, "
		"	lock_status, "
		"	report_status, "
		"   attachment_count, "
		"	inline_content_count, "
		"	thread_id, "
		"	thread_item_count, "
		"   preview_text"
		"	meeting_request_status, "
		" FROM mail_tbl",
		/*  6. select mail_attachment_tbl */
		"SELECT "
		"	attachment_id, "
		"	attachment_name, "
		"	attachment_path, "
		"	attachment_size, "
		"	mail_id,  "
		"	account_id, "
		"	mailbox_name, "
		"	file_yn,  "
		"	flag1,  "
		"	flag2,  "
		"	flag3  "
		" FROM mail_attachment_tbl ",

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
		"SELECT  "
		"   account_id, "
		"   mail_id, "
		"   server_mail_id, "
		"   activity_id, "
		"   activity_type, "
		"   mailbox_name "
		" FROM mail_partial_body_activity_tbl ",
#endif

		"SELECT  "
		"   mail_id, "
		"   account_id, "
		"   mailbox_name, "
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

#ifdef __LOCAL_ACTIVITY__
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
	_NOTI_TYPE_STORAGE,
	_NOTI_TYPE_NETWORK
} enotitype_t;


EXPORT_API int em_storage_send_noti(enotitype_t notiType, int subType, int data1, int data2, char *data3, int data4)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_PROFILE_BEGIN(profile_em_storage_send_noti);

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
		signal = dbus_message_new_signal("/User/Email/StorageChange", EMF_STORAGE_CHANGE_NOTI, "email");

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
		signal = dbus_message_new_signal("/User/Email/NetworkStatus", EMF_NETOWRK_CHANGE_NOTI, "email");

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
	else {
		EM_DEBUG_EXCEPTION("Wrong notification type [%d]", notiType);
		error = EMF_ERROR_IPC_CRASH;
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
/* 	dbus_connection_flush(connection); */
/* 	EM_DEBUG_LOG("After dbus_connection_flush");	 */
	
	ret = true;
FINISH_OFF:
	if (signal)
		dbus_message_unref(signal);

	LEAVE_CRITICAL_SECTION(_dbus_noti_lock);
	EM_PROFILE_END(profile_em_storage_send_noti);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_notify_storage_event(emf_noti_on_storage_event transaction_type, int data1, int data2 , char *data3, int data4)
{
	EM_DEBUG_FUNC_BEGIN("transaction_type[%d], data1[%d], data2[%d], data3[%p], data4[%d]", transaction_type, data1, data2, data3, data4);
	return em_storage_send_noti(_NOTI_TYPE_STORAGE, (int)transaction_type, data1, data2, data3, data4);
}

EXPORT_API int em_storage_notify_network_event(emf_noti_on_network_event status_type, int data1, char *data2, int data3, int data4)
{
	EM_DEBUG_FUNC_BEGIN("status_type[%d], data1[%d], data2[%p], data3[%d], data4[%d]", status_type, data1, data2, data3, data4);
	return em_storage_send_noti(_NOTI_TYPE_NETWORK, (int)status_type, data1, data3, data2, data4);
}

/* ----------- Notification Changes End----------- */
static int _getTableFieldDataChar(char  **table, char *buf, int index)
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

static int _getTableFieldDataInt(char  **table, int *buf, int index)
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

static int _getTableFieldDataString(char **table, char **buf, int ucs2, int index)
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
			*buf = (char *) em_core_malloc(sLen + 1);
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
		EM_DEBUG_LOG("_getTableFieldDataString - buf[%s], index[%d]", *buf, index);
	else
		EM_DEBUG_LOG("_getTableFieldDataString - No string got ");
#endif	
	ret = true;
FINISH_OFF:

	return ret;
}

static int
_getTableFieldDataStringWithoutAllocation(char **table, char *buf, int buffer_size, int ucs2, int index)
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
		EM_DEBUG_LOG("_getTableFieldDataString - buf[%s], index[%d]", buf, index);
	else
		EM_DEBUG_LOG("_getTableFieldDataString - No string got ");
#endif	

	return true;
}

static int _getStmtFieldDataChar(DB_STMT hStmt, char *buf, int index)
{
	if ((hStmt == NULL) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION("buf[%p], index[%d]", buf, index);
		return false;
	}

	if (sqlite3_column_text(hStmt, index) != NULL) {
		*buf = (char)sqlite3_column_int(hStmt, index);
#ifdef _PRINT_STORAGE_LOG_
			EM_DEBUG_LOG("_getStmtFieldDataInt [%d]", *buf);
#endif
		return true;
	}
	
	EM_DEBUG_LOG("sqlite3_column_int fail. index [%d]", index);

	return false;
}

static int _getStmtFieldDataInt(DB_STMT hStmt, int *buf, int index)
{
	if ((hStmt == NULL) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION("buf[%p], index[%d]", buf, index);
		return false;
	}

	if (sqlite3_column_text(hStmt, index) != NULL) {
		*buf = sqlite3_column_int(hStmt, index);
#ifdef _PRINT_STORAGE_LOG_
			EM_DEBUG_LOG("_getStmtFieldDataInt [%d]", *buf);
#endif
		return true;
	}
	
	EM_DEBUG_LOG("sqlite3_column_int fail. index [%d]", index);

	return false;
}

static int _getStmtFieldDataString(DB_STMT hStmt, char **buf, int ucs2, int index)
{
	if ((hStmt < 0) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION("hStmt[%d], buf[%p], index[%d]", hStmt, buf, index);
		return false;
	}

	int sLen = 0;	
	sLen = sqlite3_column_bytes(hStmt, index);

#ifdef _PRINT_STORAGE_LOG_
		EM_DEBUG_LOG("_getStmtFieldDataString sqlite3_column_bytes sLen[%d]", sLen);
#endif

	if (sLen > 0) {
		*buf = (char *) em_core_malloc(sLen + 1);
		strncpy(*buf, (char *)sqlite3_column_text(hStmt, index), sLen);
	}
	else
		*buf = NULL;

#ifdef _PRINT_STORAGE_LOG_
	if (*buf)
		EM_DEBUG_LOG("buf[%s], index[%d]", *buf, index);
	else
		EM_DEBUG_LOG("_getStmtFieldDataString - No string got");
#endif	

	return false;
}

static int _getStmtFieldDataStringWithoutAllocation(DB_STMT hStmt, char *buf, int buffer_size, int ucs2, int index)
{
	if ((hStmt < 0) || (buf == NULL) || (index < 0))  {
		EM_DEBUG_EXCEPTION("hStmt[%d], buf[%p], buffer_size[%d], index[%d]", hStmt, buf, buffer_size, index);
		return false;
	}

	int sLen = 0;	
	sLen = sqlite3_column_bytes(hStmt, index);

#ifdef _PRINT_STORAGE_LOG_
		EM_DEBUG_LOG("_getStmtFieldDataStringWithoutAllocation sqlite3_column_bytes sLen[%d]", sLen);
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

static int _bindStmtFieldDataChar(DB_STMT hStmt, int index, char value)
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

static int _bindStmtFieldDataInt(DB_STMT hStmt, int index, int value)
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

static int _bindStmtFieldDataString(DB_STMT hStmt, int index, char *value, int ucs2, int max_len)
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

static int _emf_delete_temp_file(const char *path)
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
		if (!(p = em_core_malloc((int)strlen(src) + 1)))  {
			EM_DEBUG_EXCEPTION("mailoc failed...");
			return NULL;
		}
		strncpy(p, src, strlen(src));
	}
	
	return p;
}

static void _em_storage_close_once(void)
{
	EM_DEBUG_FUNC_BEGIN();
	
	DELETE_CRITICAL_SECTION(_transactionBeginLock);
	DELETE_CRITICAL_SECTION(_transactionEndLock);
	
	EM_DEBUG_FUNC_END();
}

EXPORT_API int em_storage_close(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;

	em_storage_db_close(&error);		
	
	if (--_open_counter == 0)
		_em_storage_close_once();
	
	ret = true;

	if (err_code != NULL)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

static void *_em_storage_open_once(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int error = EM_STORAGE_ERROR_NONE;
	
	mkdir(EMAILPATH, DIRECTORY_PERMISSION);
	mkdir(DATA_PATH, DIRECTORY_PERMISSION);

	mkdir(MAILHOME, DIRECTORY_PERMISSION);
	
	SNPRINTF(g_db_path, sizeof(g_db_path), "%s/%s", MAILHOME, MAILTEMP);
	mkdir(g_db_path, DIRECTORY_PERMISSION);
	
	_emf_delete_temp_file(g_db_path);
	
	g_transaction = false;
	
	if (!em_storage_create_table(EMF_CREATE_DB_NORMAL, &error)) {
		EM_DEBUG_EXCEPTION(" em_storage_create_table failed - %d", error);
		goto FINISH_OFF;
	}
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;
		
	
	return NULL;
}

 /*  pData : a parameter which is registered when busy handler is registerd */
 /*  count : retry count */
#define EMF_STORAGE_MAX_RETRY_COUNT	20
static int 	
em_busy_handler(void *pData, int count)
{
	EM_DEBUG_LOG("Busy Handler Called!!: [%d]", count);
	usleep(200000);   /*   sleep time when SQLITE_LOCK */

	/*  retry will be stopped if  busy handler return 0 */
	return EMF_STORAGE_MAX_RETRY_COUNT - count;   
}

static int
em_storage_delete_all_files_and_directories(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int error = EM_STORAGE_ERROR_NONE;
	int ret = false;

	if (!em_storage_delete_file(EMAIL_SERVICE_DB_FILE_PATH, &error)) {
		if (error != EM_STORAGE_ERROR_FILE_NOT_FOUND) {
			EM_DEBUG_EXCEPTION("remove failed - %s", EMAIL_SERVICE_DB_FILE_PATH);
			goto FINISH_OFF;	
		}
	}

	if (!em_storage_delete_dir(MAILHOME, &error)) {
		EM_DEBUG_EXCEPTION("em_storage_delete_dir failed");
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

static int
em_storage_recovery_from_malformed_db_file(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int error = EM_STORAGE_ERROR_NONE;
	int ret = false;
	
	/* Delete all files and directories */
	if (!em_storage_delete_all_files_and_directories(&error)) {
		EM_DEBUG_EXCEPTION("em_storage_delete_all_files_and_directories failed [%d]", error);
		goto FINISH_OFF;
	}

	/* Delete all accounts on MyAccount */

	/* Delete all managed connection to DB */
	em_storage_reset_db_handle_list();

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}


EXPORT_API int em_db_open(sqlite3 **sqlite_handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int rc = 0;
	int error = EM_STORAGE_ERROR_NONE;
	int ret = false;

	EM_DEBUG_LOG("*sqlite_handle[%p]", *sqlite_handle);

	if (NULL == *sqlite_handle)  {
		/*  db open */
		EM_DEBUG_LOG("Open DB");
		EM_STORAGE_PROTECTED_FUNC_CALL(db_util_open(EMAIL_SERVICE_DB_FILE_PATH, sqlite_handle, DB_UTIL_REGISTER_HOOK_METHOD), rc);
		if (SQLITE_OK != rc) {
			EM_DEBUG_EXCEPTION("db_util_open fail:%d -%s", rc, sqlite3_errmsg(*sqlite_handle));
			error = EM_STORAGE_ERROR_DB_FAILURE;
			db_util_close(*sqlite_handle); 
			*sqlite_handle = NULL; 

			if (SQLITE_CORRUPT == rc) /* SQLITE_CORRUPT : The database disk image is malformed */ {/* Recovery DB file */ 
				EM_DEBUG_LOG("The database disk image is malformed. Trying to remove and create database disk image and directories");
				if (!em_storage_recovery_from_malformed_db_file(&error)) {
					EM_DEBUG_EXCEPTION("em_storage_recovery_from_malformed_db_file failed [%d]", error);
					goto FINISH_OFF;
				}
				
				EM_DEBUG_LOG("Open DB again");
				EM_STORAGE_PROTECTED_FUNC_CALL(db_util_open(EMAIL_SERVICE_DB_FILE_PATH, sqlite_handle, DB_UTIL_REGISTER_HOOK_METHOD), rc);
				if (SQLITE_OK != rc) {
					EM_DEBUG_EXCEPTION("db_util_open fail:%d -%s", rc, sqlite3_errmsg(*sqlite_handle));
					error = EM_STORAGE_ERROR_DB_FAILURE;
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
		rc = sqlite3_busy_handler(*sqlite_handle, em_busy_handler, NULL);  /*  Busy Handler registration, NULL is a parameter which will be passed to handler */
		if (SQLITE_OK != rc) {
			EM_DEBUG_EXCEPTION("sqlite3_busy_handler fail:%d -%s", rc, sqlite3_errmsg(*sqlite_handle));
			error = EM_STORAGE_ERROR_DB_FAILURE;
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

EXPORT_API sqlite3* em_storage_db_open(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
#ifdef _MULTIPLE_DB_HANDLE
	sqlite3 *_db_handle = NULL;
#endif
	
	int error = EM_STORAGE_ERROR_NONE;

	em_storage_shm_mutex_init(SHM_FILE_FOR_DB_LOCK, &shm_fd_for_db_lock, &mapped_for_db_lock);

#ifdef __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__
	em_storage_shm_mutex_init(SHM_FILE_FOR_MAIL_ID_LOCK, &shm_fd_for_generating_mail_id, &mapped_for_generating_mail_id);
#endif /*__FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__ */

	if (!em_db_open(&_db_handle, &error)) {
		EM_DEBUG_EXCEPTION("em_db_open failed[%d]", error);
		goto FINISH_OFF;
	}

#ifdef _MULTIPLE_DB_HANDLE
	em_storage_set_db_handle(_db_handle);
#endif
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;	

	EM_DEBUG_FUNC_END("ret [%p]", _db_handle);
	return _db_handle;	
}

EXPORT_API int em_storage_db_close(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
#ifdef _MULTIPLE_DB_HANDLE
	sqlite3 *_db_handle = em_storage_get_db_handle();
#endif
	
	int error = EM_STORAGE_ERROR_NONE;
	int ret = false;

	DELETE_CRITICAL_SECTION(_transactionBeginLock);
	DELETE_CRITICAL_SECTION(_transactionEndLock);
	DELETE_CRITICAL_SECTION(_dbus_noti_lock); 

	if (_db_handle) {
		ret = db_util_close(_db_handle);
		
		if (ret != SQLITE_OK) {
			EM_DEBUG_EXCEPTION(" db_util_close fail - %d", ret);
			error = EM_STORAGE_ERROR_DB_FAILURE;		
			ret = false;
			goto FINISH_OFF;
		}
#ifdef _MULTIPLE_DB_HANDLE
		em_storage_remove_db_handle();
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


EXPORT_API int em_storage_open(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;

	int retValue;

	retValue = mkdir(DB_PATH, DIRECTORY_PERMISSION);

	EM_DEBUG_LOG("mkdir return- %d", retValue);
	EM_DEBUG_LOG("em_storage_open - before db_util_open - pid = %d", getpid());

	if (em_storage_db_open(&error) == NULL) {
		EM_DEBUG_EXCEPTION("em_storage_db_open failed[%d]", error);
		goto FINISH_OFF;
	}

	
	if (_open_counter++ == 0)
		_em_storage_open_once(&error);

	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;	

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_create_table(emf_create_db_t type, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int error = EM_STORAGE_ERROR_NONE;
	int rc = -1, ret = false;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *create_table_query[] =
{
	/*  1. create mail_account_tbl */
	"CREATE TABLE mail_account_tbl \n"
	"(\n"
	"  account_bind_type            INTEGER \n"
	", account_name                 VARCHAR(51) \n"
	", receiving_server_type        INTEGER \n"
	", receiving_server_addr        VARCHAR(51) \n"
	", email_addr                   VARCHAR(129) \n"
	", user_name                    VARCHAR(51) \n"
	", password                     VARCHAR(51) \n"
	", retrieval_mode               INTEGER \n"
	", port_num                     INTEGER \n"
	", use_security                 INTEGER \n"
	", sending_server_type          INTEGER \n"
	", sending_server_addr          VARCHAR(51) \n"
	", sending_port_num             INTEGER \n"
	", sending_auth                 INTEGER \n"
	", sending_security             INTEGER \n"
	", sending_user                 VARCHAR(51) \n"
	", sending_password             VARCHAR(51) \n"
	", display_name                 VARCHAR(31) \n"
	", reply_to_addr                VARCHAR(129) \n"
	", return_addr                  VARCHAR(129) \n"
	", account_id                   INTEGER PRIMARY KEY \n"
	", keep_on_server               INTEGER \n"
	", flag1                        INTEGER \n"
	", flag2                        INTEGER \n"
	", pop_before_smtp              INTEGER \n"            /*  POP before SMTP authentication  */
	", apop                         INTEGER \n"            /*  APOP authentication */
	", logo_icon_path               VARCHAR(256) \n"       /*  Receiving Option : Account logo icon  */
	", preset_account               INTEGER \n"            /*  Receiving Option : Preset account or not  */
	", target_storage               INTEGER \n"            /*  Receiving Option : Specifies the targetStorage. 0 is phone, 1 is MMC  */
	", check_interval               INTEGER \n"            /*  Receiving Option : Specifies the check interval. Unit is minute  */
	", priority                     INTEGER \n"            /*  Sending Option : Specifies the prority. 1=high 3=normal 5=low  */
	", keep_local_copy              INTEGER \n"            /*  Sending Option :   */
	", req_delivery_receipt         INTEGER \n"            /*  Sending Option :   */
	", req_read_receipt             INTEGER \n"            /*  Sending Option :   */
	", download_limit               INTEGER \n"            /*  Sending Option :   */
	", block_address                INTEGER \n"            /*  Sending Option :   */
	", block_subject                INTEGER \n"            /*  Sending Option :   */
	", display_name_from            VARCHAR(256) \n"       /*  Sending Option :   */
	", reply_with_body              INTEGER \n"            /*  Sending Option :   */
	", forward_with_files           INTEGER \n"            /*  Sending Option :   */
	", add_myname_card              INTEGER \n"            /*  Sending Option :   */
	", add_signature                INTEGER \n"            /*  Sending Option :   */
	", signature                    VARCHAR(256) \n"       /*  Sending Option :   */
	", add_my_address_to_bcc        INTEGER \n"            /*  Sending Option :   */
	", my_account_id                INTEGER \n"            /*  My Account Id  */
	", index_color                  INTEGER \n"            /*  Index color  */
	", sync_status                  INTEGER \n"            /*  Sync Status */
	");",
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
	"); \n ",

	/*  3. create mail_read_mail_uid_tbl */
	"CREATE TABLE mail_read_mail_uid_tbl \n"
	"(\n"
	"  account_id                   INTEGER \n"
	", local_mbox                   VARCHAR(129) \n"
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
	", dest_mailbox                 VARCHAR(129) \n"
	", flag1                        INTEGER \n"
	", flag2                        INTEGER \n"
	"); \n ",
	/*  5. create mail_tbl */
	"CREATE TABLE mail_tbl \n"
	"(\n"
	"  mail_id                      INTEGER PRIMARY KEY \n"
	", account_id                   INTEGER \n"
	", mailbox_name                 VARCHAR(129) \n"
	", mailbox_type                 INTEGER \n"
	", subject                      UCS2TEXT\ n"
	", date_time                    VARCHAR(129) \n"                        /*  type change to int or datetime. */
	", server_mail_status           INTEGER \n"
	", server_mailbox_name          VARCHAR(129) \n"
	", server_mail_id               VARCHAR(129) \n"
	", message_id                   VARCHAR(257) \n"
	", full_address_from            UCS2TEXT \n"
	", full_address_reply           UCS2TEXT \n"
	", full_address_to              UCS2TEXT \n"
	", full_address_cc              UCS2TEXT \n"
	", full_address_bcc             UCS2TEXT \n"
	", full_address_return          UCS2TEXT \n"
	", email_address_sender         UCS2TEXT collation user1 \n"
	", email_address_recipient      UCS2TEXT collation user1 \n"
	", alias_sender                 UCS2TEXT \n"
	", alias_recipient              UCS2TEXT \n"
	", body_download_status         INTEGER \n"
	", file_path_plain              VARCHAR(257) \n"
	", file_path_html               VARCHAR(257) \n"
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
	", preview_text                 UCS2TEXT \n" 
	", meeting_request_status       INTEGER \n"
	", FOREIGN KEY(account_id)      REFERENCES mail_account_tbl(account_id) \n"
	"); \n ",
	
	/*  6. create mail_attachment_tbl */
	"CREATE TABLE mail_attachment_tbl \n"
	"(\n"
	"  attachment_id                INTEGER PRIMARY KEY"
	", attachment_name              VARCHAR(257) \n"
	", attachment_path              VARCHAR(257) \n"
	", attachment_size              INTEGER \n"
	", mail_id                      INTEGER \n"
	", account_id                   INTEGER \n"
	", mailbox_name                 VARCHAR(129) \n"
	", file_yn                      INTEGER \n"
	", flag1                        INTEGER \n"
	", flag2                        INTEGER \n"
	", flag3                        INTEGER \n"
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
	", mailbox_name                 VARCHAR(4000) \n"
	"); \n ",
#endif

	"CREATE TABLE mail_meeting_tbl \n"
	"(\n"
	"  mail_id                     INTEGER PRIMARY KEY \n"
	", account_id                  INTEGER \n"
	", mailbox_name                UCS2TEXT \n"
	", meeting_response            INTEGER \n"
	", start_time                  INTEGER \n"
	", end_time                    INTEGER \n"
	", location                    UCS2TEXT \n"
	", global_object_id            UCS2TEXT \n"
	", offset                      INTEGER \n"
	", standard_name               UCS2TEXT \n"
	", standard_time_start_date    INTEGER \n"
	", standard_bias               INTEGER \n"
	", daylight_name               UCS2TEXT \n"
	", daylight_time_start_date    INTEGER \n"
	", daylight_bias               INTEGER \n"
	"); \n ",

#ifdef __LOCAL_ACTIVITY__
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
	NULL,
};
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
 
	EM_DEBUG_LOG("local_db_handle = %p.", local_db_handle);

	char *sql;
	char **result;
	
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_account_tbl';";
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG("em_storage_create_table - result[1] = %s %c", result[1], result[1]);

	if (atoi(result[1]) < 1) {
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
		
		EM_DEBUG_LOG("CREATE TABLE mail_account_tbl");
		
		SNPRINTF(sql_query_string, sizeof(sql_query_string), create_table_query[CREATE_TABLE_MAIL_ACCOUNT_TBL]);
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		/*  create mail_account_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE UNIQUE INDEX mail_account_idx1 ON mail_account_tbl (account_bind_type, account_id)");
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);

	} /*  mail_account_tbl */
	else if (type == EMF_CREATE_DB_CHECK)  {		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_ACCOUNT_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_ACCOUNT_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}

	sqlite3_free_table(result); 
	
			
	/*  2. create mail_box_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_box_tbl';";
	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);   */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (atoi(result[1]) < 1) {
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

		EM_DEBUG_LOG("CREATE TABLE mail_box_tbl");
		
		SNPRINTF(sql_query_string, sizeof(sql_query_string), create_table_query[CREATE_TABLE_MAIL_BOX_TBL]);
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		
		/*  create mail_local_mailbox_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE UNIQUE INDEX mail_box_idx1 ON mail_box_tbl (account_id, local_yn, mailbox_name)");
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		
	} /*  mail_box_tbl */
	else if (type == EMF_CREATE_DB_CHECK)  {		
		rc = sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_BOX_TBL], NULL, NULL, NULL);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_BOX_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result); 
	
	/*  3. create mail_read_mail_uid_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_read_mail_uid_tbl';";
	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);   */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
	

	if (atoi(result[1]) < 1) {
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
		
		EM_DEBUG_LOG("CREATE TABLE mail_read_mail_uid_tbl");
		
		SNPRINTF(sql_query_string, sizeof(sql_query_string), create_table_query[CREATE_TABLE_MAIL_READ_MAIL_UID_TBL]);
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		
		/*  create mail_read_mail_uid_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE UNIQUE INDEX mail_read_mail_uid_idx1 ON mail_read_mail_uid_tbl (account_id, local_mbox, local_uid, mailbox_name, s_uid)");
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		
	} /*  mail_read_mail_uid_tbl */
	else if (type == EMF_CREATE_DB_CHECK)  {		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_READ_MAIL_UID_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_READ_MAIL_UID_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	
	
	/*  4. create mail_rule_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_rule_tbl';";
	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);   */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
	
	if (atoi(result[1]) < 1) {
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

		EM_DEBUG_LOG("CREATE TABLE mail_rule_tbl");
		
		SNPRINTF(sql_query_string, sizeof(sql_query_string), create_table_query[CREATE_TABLE_MAIL_RULE_TBL]);
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		
	} /*  mail_rule_tbl */
	else if (type == EMF_CREATE_DB_CHECK)  {		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_RULE_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_RULE_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	
	
	/*  5. create mail_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_tbl';";
	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);   */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
	
	if (atoi(result[1]) < 1) {
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
		EM_DEBUG_LOG("CREATE TABLE mail_tbl");
		
		SNPRINTF(sql_query_string, sizeof(sql_query_string), create_table_query[CREATE_TABLE_MAIL_TBL]);
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		
		/*  create mail_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE UNIQUE INDEX mail_idx1 ON mail_tbl (mail_id, account_id)");
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		/*  create mail_tbl index for date_time */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE INDEX mail_idx_date_time ON mail_tbl (date_time)");
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		/*  create mail_tbl index for thread_item_count */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE INDEX mail_idx_thread_mail_count ON mail_tbl (thread_item_count)");
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		
		/*  just one time call */
/* 		EFTSInitFTSIndex(FTS_EMAIL_IDX); */
	} /*  mail_tbl */
	else if (type == EMF_CREATE_DB_CHECK)  {		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	
	
	/*  6. create mail_attachment_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_attachment_tbl';";
	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);   */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (atoi(result[1]) < 1) {
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
		EM_DEBUG_LOG("CREATE TABLE mail_attachment_tbl");
		
		SNPRINTF(sql_query_string, sizeof(sql_query_string), create_table_query[CREATE_TABLE_MAIL_ATTACHMENT_TBL]);
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		
		/*  create mail_attachment_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE UNIQUE INDEX mail_attachment_idx1 ON mail_attachment_tbl (mail_id, attachment_id) ");
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		
	} /*  mail_attachment_tbl */
	else if (type == EMF_CREATE_DB_CHECK)  {		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_ATTACHMENT_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_ATTACHMENT_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_partial_body_activity_tbl';";
	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);   */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
	
	if (atoi(result[1]) < 1) {
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

		EM_DEBUG_LOG("CREATE TABLE mail_partial_body_activity_tbl");
		
		SNPRINTF(sql_query_string, sizeof(sql_query_string), create_table_query[CREATE_TABLE_MAIL_PARTIAL_BODY_ACTIVITY_TBL]);
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		
	} /*  mail_rule_tbl */
	else if (type == EMF_CREATE_DB_CHECK)  {		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_PARTIAL_BODY_ACTIVITY_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_PARTIAL_BODY_ACTIVITY_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	
#endif /*  __FEATURE_PARTIAL_BODY_DOWNLOAD__ */

	/*  create mail_meeting_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_meeting_tbl';";
	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);   */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
	
	if (atoi(result[1]) < 1) {
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
		
		EM_DEBUG_LOG("CREATE TABLE mail_meeting_tbl");
		
		SNPRINTF(sql_query_string, sizeof(sql_query_string), create_table_query[CREATE_TABLE_MAIL_MEETING_TBL]);
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "CREATE UNIQUE INDEX mail_meeting_idx1 ON mail_meeting_tbl (mail_id)");
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		
	} /*  mail_contact_sync_tbl */
	else if (type == EMF_CREATE_DB_CHECK)  {		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_MEETING_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_MEETING_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);

#ifdef __LOCAL_ACTIVITY__
	
		sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_local_activity_tbl';";
		/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);   */
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
		
		if (atoi(result[1]) < 1) {
			
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
			EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
				("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
	
			EM_DEBUG_LOG(" CREATE TABLE mail_local_activity_tbl");
			
			SNPRINTF(sql_query_string, sizeof(sql_query_string), create_table_query[CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL]);
			
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
			EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
			
		} /*  mail_rule_tbl */
		else if (type == EMF_CREATE_DB_CHECK)  { 	
			rc = sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL], NULL, NULL, NULL);
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL], NULL, NULL, NULL), rc);
			EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL], rc, sqlite3_errmsg(local_db_handle)));
		}
		sqlite3_free_table(result);
		
#endif /*  __LOCAL_ACTIVITY__ */

	ret = true;
	
FINISH_OFF:
	if (ret == true) {
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	}
	else {
		/*  sqlite3_exec(local_db_handle, "rollback", NULL, NULL, NULL); */
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "rollback", NULL, NULL, NULL), rc);
	}
 
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/* Query series --------------------------------------------------------------*/

EXPORT_API int em_storage_query_mail_list(const char *conditional_clause, int transaction, emf_mail_list_item_t** result_mail_list,  int *result_count,  int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_PROFILE_BEGIN(em_storage_query_mail_list_func);
		   
	int i = 0, count = 0, rc = -1, to_get_count = (result_mail_list)?0:1;
	int local_inline_content_count = 0, local_attachment_count = 0;
	int cur_query = 0, base_count = 0, col_index;
	int ret = false, error = EM_STORAGE_ERROR_NONE;
	emf_mail_list_item_t *mail_list_item_from_tbl = NULL;
	char **result = NULL, sql_query_string[QUERY_SIZE] = {0, };
	char *field_list = "mail_id, account_id, mailbox_name, full_address_from, email_address_sender, full_address_to, subject, body_download_status, flags_seen_field, flags_deleted_field, flags_flagged_field, flags_answered_field, flags_recent_field, flags_draft_field, flags_forwarded_field, DRM_status, priority, save_status, lock_status, attachment_count, inline_content_count, date_time, preview_text, thread_id, thread_item_count, meeting_request_status ";
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_IF_NULL_RETURN_VALUE(conditional_clause, false);
	EM_IF_NULL_RETURN_VALUE(result_count, false);

	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	/*  select clause */
	if (to_get_count) /*  count only */
		cur_query += SNPRINTF_OFFSET(sql_query_string, cur_query, QUERY_SIZE, "SELECT count(*) FROM mail_tbl");
	else /*  mail list in plain form */
		cur_query += SNPRINTF_OFFSET(sql_query_string, cur_query, QUERY_SIZE, "SELECT %s FROM mail_tbl ", field_list);

	cur_query += SNPRINTF_OFFSET(sql_query_string, cur_query, QUERY_SIZE, conditional_clause);
	
	EM_DEBUG_LOG("em_storage_query_mail_list : query[%s].", sql_query_string);

	/*  performing query		*/	
	EM_PROFILE_BEGIN(em_storage_query_mail_list_performing_query);
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	EM_PROFILE_END(em_storage_query_mail_list_performing_query);
	
	if (!base_count) 
		base_count = ({ int i=0; char *tmp = field_list; for (i=0; tmp && *(tmp + 1); tmp = index(tmp + 1, ','), i++); i; });

	col_index = base_count;

	EM_DEBUG_LOG("base_count [%d]", base_count);

	if (to_get_count) {	
		/*  to get mail count */
		count = atoi(result[1]);
		EM_DEBUG_LOG("There are [%d] mails.", count);
	}
	else {
		/*  to get mail list */
		if (!count) {
			EM_DEBUG_EXCEPTION("No mail found...");			
			ret = false;
			error= EM_STORAGE_ERROR_MAIL_NOT_FOUND;
			goto FINISH_OFF;
		}
		
		EM_DEBUG_LOG("There are [%d] mails.", count);
		if (!(mail_list_item_from_tbl = (emf_mail_list_item_t*)em_core_malloc(sizeof(emf_mail_list_item_t) * count))) {
			EM_DEBUG_EXCEPTION("malloc for mail_list_item_from_tbl failed...");
			error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		EM_PROFILE_BEGIN(em_storage_query_mail_list_loop);
		EM_DEBUG_LOG(">>>> DATA ASSIGN START >> ");	
		for (i = 0; i < count; i++) {
			_getTableFieldDataInt(result, &(mail_list_item_from_tbl[i].mail_id), col_index++);
			_getTableFieldDataInt(result, &(mail_list_item_from_tbl[i].account_id), col_index++);
			_getTableFieldDataStringWithoutAllocation(result, mail_list_item_from_tbl[i].mailbox_name, STRING_LENGTH_FOR_DISPLAY, 1, col_index++);
			_getTableFieldDataStringWithoutAllocation(result, mail_list_item_from_tbl[i].from, STRING_LENGTH_FOR_DISPLAY, 1, col_index++);
			_getTableFieldDataStringWithoutAllocation(result, mail_list_item_from_tbl[i].from_email_address, MAX_EMAIL_ADDRESS_LENGTH, 1, col_index++);
			_getTableFieldDataStringWithoutAllocation(result, mail_list_item_from_tbl[i].recipients, STRING_LENGTH_FOR_DISPLAY,  1, col_index++);			
			_getTableFieldDataStringWithoutAllocation(result, mail_list_item_from_tbl[i].subject, STRING_LENGTH_FOR_DISPLAY, 1, col_index++);
			_getTableFieldDataInt(result, &(mail_list_item_from_tbl[i].is_text_downloaded), col_index++);
			_getTableFieldDataChar(result, &(mail_list_item_from_tbl[i].flags_seen_field), col_index++);
			_getTableFieldDataChar(result, &(mail_list_item_from_tbl[i].flags_deleted_field), col_index++);
			_getTableFieldDataChar(result, &(mail_list_item_from_tbl[i].flags_flagged_field), col_index++);
			_getTableFieldDataChar(result, &(mail_list_item_from_tbl[i].flags_answered_field), col_index++);
			_getTableFieldDataChar(result, &(mail_list_item_from_tbl[i].flags_recent_field), col_index++);
			_getTableFieldDataChar(result, &(mail_list_item_from_tbl[i].flags_draft_field), col_index++);
			_getTableFieldDataChar(result, &(mail_list_item_from_tbl[i].flags_forwarded_field), col_index++);
			_getTableFieldDataInt(result, &(mail_list_item_from_tbl[i].has_drm_attachment), col_index++);
			_getTableFieldDataInt(result, &(mail_list_item_from_tbl[i].priority), col_index++);
			_getTableFieldDataInt(result, &(mail_list_item_from_tbl[i].save_status), col_index++);
			_getTableFieldDataInt(result, &(mail_list_item_from_tbl[i].is_locked), col_index++);
			_getTableFieldDataInt(result, &local_attachment_count, col_index++);
			_getTableFieldDataInt(result, &local_inline_content_count, col_index++);
			_getTableFieldDataStringWithoutAllocation(result, mail_list_item_from_tbl[i].datetime, MAX_DATETIME_STRING_LENGTH, 0, col_index++);
			_getTableFieldDataStringWithoutAllocation(result, mail_list_item_from_tbl[i].previewBodyText, MAX_PREVIEW_TEXT_LENGTH, 1, col_index++);
			_getTableFieldDataInt(result, &(mail_list_item_from_tbl[i].thread_id), col_index++);
			_getTableFieldDataInt(result, &(mail_list_item_from_tbl[i].thread_item_count), col_index++);
			_getTableFieldDataInt(result, &(mail_list_item_from_tbl[i].is_meeting_request), col_index++);
			mail_list_item_from_tbl[i].has_attachment = ((local_attachment_count - local_inline_content_count)>0)?1:0;
		}
		EM_DEBUG_LOG(">>>> DATA ASSIGN END [count : %d] >> ", count);
		EM_PROFILE_END(em_storage_query_mail_list_loop);
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

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(em_storage_query_mail_list_func);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_query_mail_tbl(const char *conditional_clause, int transaction, emf_mail_tbl_t** result_mail_tbl, int *result_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("conditional_clause[%s], result_mail_tbl[%p], result_count [%p], transaction[%d], err_code[%p]", conditional_clause, result_mail_tbl, result_count, transaction, err_code);

	if (!conditional_clause) {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int i, col_index = FIELD_COUNT_OF_EMF_MAIL_TBL, rc, ret = false, count;
	int error = EM_STORAGE_ERROR_NONE;
	char **result = NULL, sql_query_string[QUERY_SIZE] = {0, };
	emf_mail_tbl_t* p_data_tbl = NULL;
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_tbl %s", conditional_clause);

	EM_DEBUG_LOG("Query [%s]", sql_query_string);
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (!count) {
		EM_DEBUG_EXCEPTION("No mail found...");			
		ret = false;
		error= EM_STORAGE_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	EM_DEBUG_LOG("There are [%d] mails.", count);
	if (!(p_data_tbl = (emf_mail_tbl_t*)em_core_malloc(sizeof(emf_mail_tbl_t) * count))) {
		EM_DEBUG_EXCEPTION("malloc for emf_mail_tbl_t failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG(">>>> DATA ASSIGN START >> ");	
	for (i = 0; i < count; i++) {
		_getTableFieldDataInt(result, &(p_data_tbl[i].mail_id), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].account_id), col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].mailbox_name), 0, col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].mailbox_type), col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].subject), 1, col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].datetime), 0, col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].server_mail_status), col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].server_mailbox_name), 0, col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].server_mail_id), 0, col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].message_id), 0, col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].full_address_from), 1, col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].full_address_reply), 1, col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].full_address_to), 1, col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].full_address_cc), 1, col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].full_address_bcc), 1, col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].full_address_return), 1, col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].email_address_sender), 1, col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].email_address_recipient), 1, col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].alias_sender), 1, col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].alias_recipient), 1, col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].body_download_status), col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].file_path_plain), 0, col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].file_path_html), 0, col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].mail_size), col_index++);
		_getTableFieldDataChar(result, &(p_data_tbl[i].flags_seen_field), col_index++);
		_getTableFieldDataChar(result, &(p_data_tbl[i].flags_deleted_field), col_index++);
		_getTableFieldDataChar(result, &(p_data_tbl[i].flags_flagged_field), col_index++);
		_getTableFieldDataChar(result, &(p_data_tbl[i].flags_answered_field), col_index++);
		_getTableFieldDataChar(result, &(p_data_tbl[i].flags_recent_field), col_index++);
		_getTableFieldDataChar(result, &(p_data_tbl[i].flags_draft_field), col_index++);
		_getTableFieldDataChar(result, &(p_data_tbl[i].flags_forwarded_field), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].DRM_status), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].priority), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].save_status), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].lock_status), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].report_status), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].attachment_count), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].inline_content_count), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].thread_id), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].thread_item_count), col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].preview_text), 1, col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].meeting_request_status), col_index++);
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
	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


/* Query series --------------------------------------------------------------*/

EXPORT_API int em_storage_check_duplicated_account(emf_account_t* account, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	if (!account)  {
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char **result;
	int count;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"SELECT COUNT(*) FROM mail_account_tbl "
		" WHERE "
		" email_addr = '%s' AND "
		" user_name = '%s' AND "
		" receiving_server_type = %d AND "
		" receiving_server_addr = '%s' AND "
		" sending_user = '%s' AND "
		" sending_server_type = %d AND "
		" sending_server_addr = '%s'; ",
		account->email_addr,
		account->user_name, account->receiving_server_type, account->receiving_server_addr,
		account->sending_user, account->sending_server_type, account->sending_server_addr
	);
	EM_DEBUG_LOG("Query[%s]", sql_query_string);
	/* rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	count = atoi(result[1]);
	sqlite3_free_table(result);

	EM_DEBUG_LOG("Count of Duplicated Account Information: [%d]", count);

	if (count == 0) {	/*  not duplicated account */
		ret = true;
		EM_DEBUG_LOG("NOT duplicated account: email_addr[%s]", account->email_addr);
	}
	else {	/*  duplicated account */
		ret = false;
		EM_DEBUG_LOG("The same account already exists. Duplicated account: email_addr[%s]", account->email_addr);
		error = EM_STORAGE_ERROR_ALREADY_EXISTS;
	}
	
FINISH_OFF:

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}

EXPORT_API int em_storage_get_account_count(int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	if (!count)  {
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
 
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char err_msg[1024];
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_account_tbl");
	EM_DEBUG_LOG("SQL STMT [ %s ]", sql_query_string);
	/*  rc = sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL);   */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("Before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
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
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
		EM_DEBUG_LOG("sqlite3_finalize- %d", rc);
	}
 
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_get_account_list(int *select_num, emf_mail_account_tbl_t** account_list, int transaction, int with_password, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int i = 0, count = 0, rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	emf_mail_account_tbl_t *p_data_tbl = NULL;
 
	DB_STMT hStmt = NULL;

	if (!select_num || !account_list)  {
		EM_DEBUG_EXCEPTION("select_num[%p], account_list[%p]", select_num, account_list);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);

	char sql_query_string[QUERY_SIZE] = {0, };
	char *sql = "SELECT count(*) FROM mail_account_tbl;";
	char **result;
	
	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	count = atoi(result[1]);
	sqlite3_free_table(result);

	if (!count) {
		EM_DEBUG_EXCEPTION("no account found...");
		error = EM_STORAGE_ERROR_ACCOUNT_NOT_FOUND;		
		ret = true;
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("count = %d", rc);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_account_tbl ORDER BY account_id");

	/*  rc = sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL);   */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG("After sqlite3_prepare_v2 hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no account found...");
		
		error = EM_STORAGE_ERROR_ACCOUNT_NOT_FOUND;
		count = 0;
		ret = true;
		goto FINISH_OFF;
	}

	if (!(p_data_tbl = (emf_mail_account_tbl_t*)malloc(sizeof(emf_mail_account_tbl_t) * count)))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memset(p_data_tbl, 0x00, sizeof(emf_mail_account_tbl_t) * count);
	for (i = 0; i < count; i++)  {
		/*  get recordset */
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].account_bind_type), ACCOUNT_BIND_TYPE_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].account_name), 0, ACCOUNT_NAME_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].receiving_server_type), RECEIVING_SERVER_TYPE_TYPE_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].receiving_server_addr), 0, RECEIVING_SERVER_ADDR_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].email_addr), 0, EMAIL_ADDR_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].user_name), 0, USER_NAME_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].password), 0, PASSWORD_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].retrieval_mode), RETRIEVAL_MODE_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].port_num), PORT_NUM_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].use_security), USE_SECURITY_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].sending_server_type), SENDING_SERVER_TYPE_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].sending_server_addr), 0, SENDING_SERVER_ADDR_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].sending_port_num), SENDING_PORT_NUM_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].sending_auth), SENDING_AUTH_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].sending_security), SENDING_SECURITY_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].sending_user), 0, SENDING_USER_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].sending_password), 0, SENDING_PASSWORD_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].display_name), 0, DISPLAY_NAME_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].reply_to_addr), 0, REPLY_TO_ADDR_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].return_addr), 0, RETURN_ADDR_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].account_id), ACCOUNT_ID_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].keep_on_server), KEEP_ON_SERVER_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].flag1), FLAG1_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].flag2), FLAG2_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].pop_before_smtp), POP_BEFORE_SMTP_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].apop), APOP_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].logo_icon_path), 0, LOGO_ICON_PATH_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].preset_account), PRESET_ACCOUNT_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].target_storage), TARGET_STORAGE_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].check_interval), CHECK_INTERVAL_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].options.priority), PRIORITY_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].options.keep_local_copy), KEEP_LOCAL_COPY_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].options.req_delivery_receipt), REQ_DELIVERY_RECEIPT_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].options.req_read_receipt), REQ_READ_RECEIPT_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].options.download_limit), DOWNLOAD_LIMIT_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].options.block_address), BLOCK_ADDRESS_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].options.block_subject), BLOCK_SUBJECT_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].options.display_name_from), 0, DISPLAY_NAME_FROM_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].options.reply_with_body), REPLY_WITH_BODY_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].options.forward_with_files), FORWARD_WITH_FILES_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].options.add_myname_card), ADD_MYNAME_CARD_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].options.add_signature), ADD_SIGNATURE_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].options.signature), 0, SIGNATURE_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].options.add_my_address_to_bcc), ADD_MY_ADDRESS_TO_BCC_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].my_account_id), MY_ACCOUNT_ID_IDX_IN_MAIL_ACCOUNT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].index_color), INDEX_COLOR_IDX_IN_MAIL_ACCOUNT_TBL);
		if (with_password == true) {
			/*  get password from the secure storage */
			char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];/*  = p_data_tbl[i].password; */
			char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];;/*  = p_data_tbl[i].sending_password; */

			EM_SAFE_FREE(p_data_tbl[i].password);
			EM_SAFE_FREE(p_data_tbl[i].sending_password);

			/*  get password file name */
			if ((error = em_storage_get_password_file_name(p_data_tbl[i].account_id, recv_password_file_name, send_password_file_name)) != EM_STORAGE_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("em_storage_get_password_file_name failed.");
				goto FINISH_OFF;
			}
				
			/*  read password from secure storage */
			if ((error = em_storage_read_password_ss(recv_password_file_name, &(p_data_tbl[i].password))) < 0 ) {
				EM_DEBUG_EXCEPTION("em_storage_read_password_ss()  failed...");
				goto FINISH_OFF;
			}
			EM_DEBUG_LOG("recv_password_file_name[%s], password[%s]", recv_password_file_name,  p_data_tbl[i].password);
			
			if ((error = em_storage_read_password_ss(send_password_file_name, &(p_data_tbl[i].sending_password))) < 0) {
				EM_DEBUG_EXCEPTION("em_storage_read_password_ss()  failed...");
				goto FINISH_OFF;
			}
			EM_DEBUG_LOG("send_password_file_name[%s], password[%s]", send_password_file_name,  p_data_tbl[i].sending_password);
		}

		/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_LOG("after sqlite3_step(), i = %d, rc = %d.", i,  rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
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
		em_storage_free_account(&p_data_tbl, count, NULL);
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);
	
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_get_maildata_by_servermailid(int account_id, char *server_mail_id, emf_mail_tbl_t** mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], server_mail_id[%s], mail[%p], transaction[%d], err_code[%p]", account_id, server_mail_id, mail, transaction, err_code);
	
	int ret = false, error = EM_STORAGE_ERROR_NONE, result_count;
	char conditional_clause[QUERY_SIZE] = {0, };
	emf_mail_tbl_t* p_data_tbl = NULL;

	if (!server_mail_id || !mail) {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (account_id == ALL_ACCOUNT)
		SNPRINTF(conditional_clause, QUERY_SIZE, "WHERE UPPER(server_mail_id) =UPPER('%s')", server_mail_id);
	else
		SNPRINTF(conditional_clause, QUERY_SIZE, "WHERE UPPER(server_mail_id) =UPPER('%s') AND account_id = %d", server_mail_id, account_id);

	EM_DEBUG_LOG("conditional_clause [%s]", conditional_clause);

	if(!em_storage_query_mail_tbl(conditional_clause, transaction, &p_data_tbl, &result_count, &error)) {
		EM_DEBUG_EXCEPTION("em_storage_query_mail_tbl failed [%d]", error);
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


static int em_storage_write_conditional_clause_for_getting_mail_list(int account_id, const char *mailbox_name, emf_email_address_list_t* addr_list, int thread_id, int start_index, int limit_count, int search_type, const char *search_value, emf_sort_type_t sorting, char *conditional_clause_string, int buffer_size, int *err_code)
{
	int cur_clause = 0, conditional_clause_count = 0, i = 0;

	if (account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("Invalid account_id [%d]", account_id);
		EM_RETURN_ERR_CODE(err_code, EM_STORAGE_ERROR_INVALID_PARAM, false);
	}

	/*  where clause */
	if (account_id == ALL_ACCOUNT) {
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE mailbox_type not in (3, 5, 7)"):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND mailbox_type not in (3, 5, 7)");
	}
	else {
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE account_id = %d", account_id):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND account_id = %d", account_id);
	}
	
	if (mailbox_name) {
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE UPPER(mailbox_name) = UPPER('%s')", mailbox_name):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND UPPER(mailbox_name) = UPPER('%s')", mailbox_name);
	}
	else if(account_id != ALL_ACCOUNT) {
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE mailbox_type not in (3, 5, 7)"):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND mailbox_type not in (3, 5, 7)");
	}

	if (thread_id > 0) {
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE thread_id = %d ", thread_id):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND thread_id = %d ", thread_id);
	}
	else if (thread_id == EMF_LIST_TYPE_THREAD)	{
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE thread_item_count > 0"):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND thread_item_count > 0");
	}
	else if (thread_id == EMF_LIST_TYPE_LOCAL) {
		cur_clause += (conditional_clause_count++ == 0)?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE server_mail_status = 0"):
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND server_mail_status = 0");
	}
 	else if (thread_id == EMF_LIST_TYPE_UNREAD) {
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

	if (search_value) {
		switch (search_type) {
			case EMF_SEARCH_FILTER_SUBJECT:
				cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause,
					" %s (UPPER(subject) LIKE UPPER(\'%%%%%s%%%%\')) ", conditional_clause_count++ ? "AND" : "WHERE", search_value);
				break;
			case EMF_SEARCH_FILTER_SENDER:
				cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause,
					" %s  ((UPPER(full_address_from) LIKE UPPER(\'%%%%%s%%%%\')) "
					") ", conditional_clause_count++ ? "AND" : "WHERE", search_value);
				break;
			case EMF_SEARCH_FILTER_RECIPIENT:
				cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause,
					" %s ((UPPER(full_address_to) LIKE UPPER(\'%%%%%s%%%%\')) "	
					"	OR (UPPER(full_address_cc) LIKE UPPER(\'%%%%%s%%%%\')) "
					"	OR (UPPER(full_address_bcc) LIKE UPPER(\'%%%%%s%%%%\')) "
					") ", conditional_clause_count++ ? "AND" : "WHERE", search_value, search_value, search_value);
				break;
			case EMF_SEARCH_FILTER_ALL:
				cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause,
					" %s (UPPER(subject) LIKE UPPER(\'%%%%%s%%%%\') "
					" 	OR (((UPPER(full_address_from) LIKE UPPER(\'%%%%%s%%%%\')) "
					"			OR (UPPER(full_address_to) LIKE UPPER(\'%%%%%s%%%%\')) "
					"			OR (UPPER(full_address_cc) LIKE UPPER(\'%%%%%s%%%%\')) "
					"			OR (UPPER(full_address_bcc) LIKE UPPER(\'%%%%%s%%%%\')) "
					"		) "
					"	)"
					")", conditional_clause_count++ ? "AND" : "WHERE", search_value, search_value, search_value, search_value, search_value);
				break;
		}
	}
	
	/*  EM_DEBUG_LOG("where clause [%s]", conditional_clause_string); */
	static char sorting_str[][50] = { 
			 " ORDER BY date_time DESC",						 /* case EMF_SORT_DATETIME_HIGH: */
			 " ORDER BY date_time ASC",						  /* case EMF_SORT_DATETIME_LOW: */
			 " ORDER BY full_address_from DESC, date_time DESC", /* case EMF_SORT_SENDER_HIGH: */
			 " ORDER BY full_address_from ASC, date_time DESC",  /* case EMF_SORT_SENDER_LOW: */
			 " ORDER BY full_address_to DESC, date_time DESC",   /* case EMF_SORT_RCPT_HIGH: */
			 " ORDER BY full_address_to ASC, date_time DESC",	/* case EMF_SORT_RCPT_LOW: */
			 " ORDER BY subject DESC, date_time DESC",		   /* case EMF_SORT_SUBJECT_HIGH: */
			 " ORDER BY subject ASC, date_time DESC",			/* case EMF_SORT_SUBJECT_LOW: */
			 " ORDER BY priority DESC, date_time DESC",		  /* case EMF_SORT_PRIORITY_HIGH: */
			 " ORDER BY priority ASC, date_time DESC",		   /* case EMF_SORT_PRIORITY_LOW: */
			 " ORDER BY attachment_count DESC, date_time DESC",  /* case EMF_SORT_ATTACHMENT_HIGH: */
			 " ORDER BY attachment_count ASC, date_time DESC",   /* case EMF_SORT_ATTACHMENT_LOW: */
			 " ORDER BY lock_status DESC, date_time DESC",	   /* case EMF_SORT_FAVORITE_HIGH: */
			 " ORDER BY lock_status ASC, date_time DESC",		/* case EMF_SORT_FAVORITE_LOW: */
			 " ORDER BY mailbox_name DESC, date_time DESC",	  /* case EMF_SORT_MAILBOX_NAME_HIGH: */
			 " ORDER BY mailbox_name ASC, date_time DESC"		/* case EMF_SORT_MAILBOX_NAME_LOW: */
			 };

	if (sorting < EMF_SORT_END && sorting >= 0)
		cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size, " %s", sorting_str[sorting]);
	else
		EM_DEBUG_LOG(" Invalid Sorting order ");

	/*  limit clause */
	if (start_index != -1 && limit_count != -1)
		cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size, " LIMIT %d, %d", start_index, limit_count);

	return true;
}


/**
  *	em_storage_get_mail_list - Get the mail list information.
  *	
  *
  */
EXPORT_API int em_storage_get_mail_list(int account_id, const char *mailbox_name, emf_email_address_list_t* addr_list, int thread_id, int start_index, int limit_count, int search_type, const char *search_value, emf_sort_type_t sorting, int transaction, emf_mail_list_item_t** mail_list,  int *result_count,  int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_PROFILE_BEGIN(em_storage_get_mail_list_func);

	int ret = false, error = EM_STORAGE_ERROR_NONE;
	char conditional_clause_string[QUERY_SIZE] = { 0, };

	if (account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("Invalid account_id [%d]", account_id);
		EM_RETURN_ERR_CODE(err_code, EM_STORAGE_ERROR_INVALID_PARAM, false);
	}
	EM_IF_NULL_RETURN_VALUE(result_count, false);

	em_storage_write_conditional_clause_for_getting_mail_list(account_id, mailbox_name, addr_list, thread_id, start_index, limit_count, search_type, search_value, sorting, conditional_clause_string, QUERY_SIZE, &error);
	
	EM_DEBUG_LOG("conditional_clause_string[%s].", conditional_clause_string);

	if(!em_storage_query_mail_list(conditional_clause_string, transaction, mail_list, result_count, &error)) {
		EM_DEBUG_EXCEPTION("em_storage_query_mail_list [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(em_storage_get_mail_list_func);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


/**
  *	em_storage_get_mails - Get the Mail list information based on mailbox_name name
  *	
  *
  */
EXPORT_API int em_storage_get_mails(int account_id, const char *mailbox_name, emf_email_address_list_t* addr_list, int thread_id, int start_index, int limit_count, emf_sort_type_t sorting,  int transaction, emf_mail_tbl_t** mail_list, int *result_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_PROFILE_BEGIN(emStorageGetMails);
	
	int count = 0, ret = false, error = EM_STORAGE_ERROR_NONE;	
	emf_mail_tbl_t *p_data_tbl = NULL;
	char conditional_clause_string[QUERY_SIZE] = {0, };

	EM_IF_NULL_RETURN_VALUE(mail_list, false);
	EM_IF_NULL_RETURN_VALUE(result_count, false);

	if (!result_count || !mail_list || account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	em_storage_write_conditional_clause_for_getting_mail_list(account_id, mailbox_name, addr_list, thread_id, start_index, limit_count, 0, NULL, sorting, conditional_clause_string, QUERY_SIZE, &error);

	EM_DEBUG_LOG("conditional_clause_string [%s]", conditional_clause_string);

	if(!em_storage_query_mail_tbl(conditional_clause_string, transaction, &p_data_tbl, &count,  &error)) {
		EM_DEBUG_EXCEPTION("em_storage_query_mail_tbl failed [%d]", error);
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
		em_storage_free_mail(&p_data_tbl, count, NULL);
	
	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(emStorageGetMails);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}



/**
  *	em_storage_get_searched_mail_list - Get the mail list information after filtering
  *	
  *
  */
EXPORT_API int em_storage_get_searched_mail_list(int account_id, const char *mailbox_name, int thread_id, int search_type, const char *search_value, int start_index, int limit_count, emf_sort_type_t sorting, int transaction, emf_mail_list_item_t** mail_list,  int *result_count,  int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false, error = EM_STORAGE_ERROR_NONE;
	char conditional_clause[QUERY_SIZE] = {0, };
	
	EM_IF_NULL_RETURN_VALUE(mail_list, false);
	EM_IF_NULL_RETURN_VALUE(result_count, false);

	if (!result_count || !mail_list || account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("select_num[%p], Mail_list[%p]", result_count, mail_list);
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	em_storage_write_conditional_clause_for_getting_mail_list(account_id, mailbox_name, NULL, thread_id, start_index, limit_count, search_type, search_value, sorting, conditional_clause, QUERY_SIZE, &error);
	
	EM_DEBUG_LOG("conditional_clause[%s]", conditional_clause);

	if(!em_storage_query_mail_list(conditional_clause, transaction, mail_list, result_count, &error)) {
		EM_DEBUG_EXCEPTION("em_storage_query_mail_list [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:
	EM_DEBUG_LOG("em_storage_get_searched_mail_list finish off");	
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


static int em_storage_get_password_file_name(int account_id, char *recv_password_file_name, char *send_password_file_name)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d]", account_id);

	if (account_id <= 0 || !recv_password_file_name || !send_password_file_name)  {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return EM_STORAGE_ERROR_INVALID_PARAM;
	}

	sprintf(recv_password_file_name, ".email_account_%d_recv", account_id);
	sprintf(send_password_file_name, ".email_account_%d_send", account_id);
	EM_DEBUG_FUNC_END();
	return EM_STORAGE_ERROR_NONE;
}

static int em_storage_read_password_ss(char *file_name, char **password)
{
	EM_DEBUG_FUNC_BEGIN("file_name[%s], password[%p]", file_name, password);

	if (!file_name || !password) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		return EM_STORAGE_ERROR_INVALID_PARAM;
	}	

	size_t buf_len = 0, read_len = 0;
	ssm_file_info_t sfi;
	char *temp_password = NULL;
	int ret = EM_STORAGE_ERROR_NONE;

	if (ssm_getinfo(file_name, &sfi, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION("ssm_getinfo() failed.");
		ret = EM_STORAGE_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	buf_len = sfi.originSize;
	EM_DEBUG_LOG("password buf_len[%d]", buf_len);
	if ((temp_password = (char *)malloc(buf_len + 1)) == NULL) {
		EM_DEBUG_EXCEPTION("malloc failed...");
		
		ret = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memset(temp_password, 0x00, buf_len + 1);

	if (ssm_read(file_name, temp_password, buf_len, &read_len, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION("ssm_read() failed.");
		ret = EM_STORAGE_ERROR_SYSTEM_FAILURE;
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


EXPORT_API int em_storage_get_account_by_id(int account_id, int pulloption, emf_mail_account_tbl_t **account, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], pulloption[%d], account[%p], transaction[%d], err_code[%p]", account_id, pulloption, account, transaction, err_code);

	if (!account)  {
		EM_DEBUG_EXCEPTION("account_id[%d], account[%p]", account_id, account);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	emf_mail_account_tbl_t* p_data_tbl = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	int rc = -1;
	int sql_len = 0;
	char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);

	/*  Make query string */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT ");
	sql_len = strlen(sql_query_string);
		
	if (pulloption & EMF_ACC_GET_OPT_DEFAULT) {
		SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len, "account_bind_type, receiving_server_type, receiving_server_addr, email_addr, user_name, \
		 retrieval_mode, port_num, use_security, sending_server_type, sending_server_addr, sending_port_num, sending_auth, sending_security, sending_user, \
		 display_name, reply_to_addr, return_addr, account_id, keep_on_server, flag1, flag2, pop_before_smtp, apop, logo_icon_path, preset_account, target_storage, check_interval, index_color, sync_status, ");
		sql_len = strlen(sql_query_string); 
	}

	if (pulloption & EMF_ACC_GET_OPT_ACCOUNT_NAME) {
		SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len, " account_name, ");
		sql_len = strlen(sql_query_string); 
	}

	/*  get from secure storage, not from db */
	if (pulloption & EMF_ACC_GET_OPT_OPTIONS) {
		SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len, " priority, keep_local_copy, req_delivery_receipt, req_read_receipt, download_limit, block_address, block_subject, display_name_from, \
			reply_with_body, forward_with_files, add_myname_card, add_signature, signature,  add_my_address_to_bcc, my_account_id, ");
		sql_len = strlen(sql_query_string); 
	}
	/*  dummy value, FROM WHERE clause */
	SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len, "0 FROM mail_account_tbl WHERE account_id = %d", account_id);

	/*  FROM clause */
	EM_DEBUG_LOG("query = [%s]", sql_query_string);

	/*  execute a sql and count rows */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no matched account found...");
		error = EM_STORAGE_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

	/*  Assign query result to structure */
	if (!(p_data_tbl = (emf_mail_account_tbl_t *)malloc(sizeof(emf_mail_account_tbl_t) * 1)))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(p_data_tbl, 0x00, sizeof(emf_mail_account_tbl_t) * 1);
	int col_index = 0;

	if (pulloption & EMF_ACC_GET_OPT_DEFAULT) {
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->account_bind_type), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->receiving_server_type), col_index++);
		_getStmtFieldDataString(hStmt, &(p_data_tbl->receiving_server_addr),  0, col_index++);
		_getStmtFieldDataString(hStmt, &(p_data_tbl->email_addr), 0, col_index++);
		_getStmtFieldDataString(hStmt, &(p_data_tbl->user_name), 0, col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->retrieval_mode), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->port_num), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->use_security), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->sending_server_type), col_index++);
		_getStmtFieldDataString(hStmt, &(p_data_tbl->sending_server_addr), 0, col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->sending_port_num), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->sending_auth), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->sending_security), col_index++);
		_getStmtFieldDataString(hStmt, &(p_data_tbl->sending_user), 0, col_index++);
		_getStmtFieldDataString(hStmt, &(p_data_tbl->display_name), 0, col_index++);
		_getStmtFieldDataString(hStmt, &(p_data_tbl->reply_to_addr), 0, col_index++);
		_getStmtFieldDataString(hStmt, &(p_data_tbl->return_addr), 0, col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->account_id), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->keep_on_server), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->flag1), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->flag2), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->pop_before_smtp), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->apop), col_index++);
		_getStmtFieldDataString(hStmt, &(p_data_tbl->logo_icon_path), 0, col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->preset_account), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->target_storage), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->check_interval), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->index_color), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->sync_status), col_index++);
	}

	if (pulloption & EMF_ACC_GET_OPT_ACCOUNT_NAME)
		_getStmtFieldDataString(hStmt, &(p_data_tbl->account_name), 0, col_index++);

	if (pulloption & EMF_ACC_GET_OPT_PASSWORD) {
		/*  get password file name */
		if ((error = em_storage_get_password_file_name(p_data_tbl->account_id, recv_password_file_name, send_password_file_name)) != EM_STORAGE_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_storage_get_password_file_name failed.");
			goto FINISH_OFF;
		}		

		/*  read password from secure storage */
		if ((error = em_storage_read_password_ss(recv_password_file_name, &(p_data_tbl->password))) < 0) {
			EM_DEBUG_EXCEPTION(" em_storage_read_password_ss()  failed...");
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG("recv_password_file_name[%s], password[%s]", recv_password_file_name,  p_data_tbl->password);
		
		if ((error = em_storage_read_password_ss(send_password_file_name, &(p_data_tbl->sending_password))) < 0) {
			EM_DEBUG_EXCEPTION(" em_storage_read_password_ss()  failed...");
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG("send_password_file_name[%s], password[%s]", send_password_file_name,  p_data_tbl->sending_password);
	}

	if (pulloption & EMF_ACC_GET_OPT_OPTIONS) {
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->options.priority), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->options.keep_local_copy), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->options.req_delivery_receipt), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->options.req_read_receipt), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->options.download_limit), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->options.block_address), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->options.block_subject), col_index++);
		_getStmtFieldDataString(hStmt, &(p_data_tbl->options.display_name_from), 0, col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->options.reply_with_body), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->options.forward_with_files), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->options.add_myname_card), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->options.add_signature), col_index++);
		_getStmtFieldDataString(hStmt, &(p_data_tbl->options.signature), 0, col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->options.add_my_address_to_bcc), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl->my_account_id), col_index++);
	}

	ret = true;

FINISH_OFF:
	if (ret == true)
		*account = p_data_tbl;
	else {
		if (p_data_tbl)
			em_storage_free_account((emf_mail_account_tbl_t **)&p_data_tbl, 1, NULL);
	}
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_password_length_of_account(int account_id, int *password_length, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], password_length[%p], err_code[%p]", account_id, password_length, err_code);

	if (account_id <= 0 || password_length == NULL)  {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char *temp_password = NULL;
	

	/*  get password file name */
	if ((error = em_storage_get_password_file_name(account_id, recv_password_file_name, send_password_file_name)) != EM_STORAGE_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_storage_get_password_file_name failed.");
		goto FINISH_OFF;
	}		

	/*  read password from secure storage */
	if ((error = em_storage_read_password_ss(recv_password_file_name, &temp_password)) < 0 || !temp_password) {
		EM_DEBUG_EXCEPTION(" em_storage_read_password_ss()  failed...");
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

EXPORT_API int em_storage_update_account(int account_id, emf_mail_account_tbl_t* account, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], account[%p], transaction[%d], err_code[%p]", account_id, account, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !account)  {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int error = EM_STORAGE_ERROR_NONE;
	int rc, ret = false;
 
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
		
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_account_tbl SET"
		" account_bind_type = ?"
		", account_name = ?"
		", receiving_server_type = ?"
		", receiving_server_addr = ?"
		", email_addr = ?"
		", user_name = ?"
		", retrieval_mode = ?"
		", port_num = ?"
		", use_security = ?"
		", sending_server_type = ?"
		", sending_server_addr = ?"
		", sending_port_num = ?"
		", sending_auth = ?"
		", sending_security = ?"
		", sending_user = ?"
		", display_name = ?"
		", reply_to_addr = ?"
		", return_addr = ?"
		", keep_on_server = ?"
		", flag1 = ?"
		", flag2 = ?"
		", pop_before_smtp = ?"            /*  POP before SMTP authentication */
		", apop = ?"                       /*  APOP authentication */
		", logo_icon_path = ?"             /*  Receving Option : Account logo icon  */
		", preset_account = ?"             /*  Receving Option : Preset account or not  */
		", target_storage = ?"             /*  Receving Option : Specifies the targetStorage. 0 is phone, 1 is MMC  */
		", check_interval = ?"             /*  Receving Option : Specifies the check interval. Unit is minute  */
		", priority = ?"                   /*  Sending Option : Specifies the prority. 1=high 3=normal 5=low  */
		", keep_local_copy = ?"            /*  Sending Option :   */
		", req_delivery_receipt = ?"       /*  Sending Option :   */
		", req_read_receipt = ?"           /*  Sending Option :   */
		", download_limit = ?"             /*  Sending Option :   */
		", block_address = ?"              /*  Sending Option :   */
		", block_subject = ?"              /*  Sending Option :   */
		", display_name_from = ?"          /*  Sending Option :   */
		", reply_with_body = ?"            /*  Sending Option :   */
		", forward_with_files = ?"         /*  Sending Option :   */
		", add_myname_card = ?"            /*  Sending Option :   */
		", add_signature = ?"              /*  Sending Option :   */
		", signature = ?"                  /*  Sending Option :   */
		", add_my_address_to_bcc = ?"      /*  Sending Option :   */
		", my_account_id = ?"              /*  Support 'My Account' */
		", index_color = ?"                /*  Index color */
		", sync_status = ?"                /*  Sync Status */
		" WHERE account_id = ?");	 
 
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("After sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	int i = 0;
	
	_bindStmtFieldDataInt(hStmt, i++, account->account_bind_type);
	_bindStmtFieldDataString(hStmt, i++, (char *)account->account_name, 0, ACCOUNT_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataInt(hStmt, i++, account->receiving_server_type);
	_bindStmtFieldDataString(hStmt, i++, (char *)account->receiving_server_addr, 0, RECEIVING_SERVER_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)account->email_addr, 0, EMAIL_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)account->user_name, 0, USER_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataInt(hStmt, i++, account->retrieval_mode);
	_bindStmtFieldDataInt(hStmt, i++, account->port_num);
	_bindStmtFieldDataInt(hStmt, i++, account->use_security);
	_bindStmtFieldDataInt(hStmt, i++, account->sending_server_type);
	_bindStmtFieldDataString(hStmt, i++, (char *)account->sending_server_addr, 0, SENDING_SERVER_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataInt(hStmt, i++, account->sending_port_num);
	_bindStmtFieldDataInt(hStmt, i++, account->sending_auth);
	_bindStmtFieldDataInt(hStmt, i++, account->sending_security);
	_bindStmtFieldDataString(hStmt, i++, (char *)account->sending_user, 0, SENDING_USER_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)account->display_name, 0, DISPLAY_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)account->reply_to_addr, 0, REPLY_TO_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)account->return_addr, 0, RETURN_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataInt(hStmt, i++, account->keep_on_server);
	_bindStmtFieldDataInt(hStmt, i++, account->flag1);
	_bindStmtFieldDataInt(hStmt, i++, account->flag2);
	_bindStmtFieldDataInt(hStmt, i++, account->pop_before_smtp);
	_bindStmtFieldDataInt(hStmt, i++, account->apop);
	_bindStmtFieldDataString(hStmt, i++, account->logo_icon_path, 0, LOGO_ICON_PATH_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataInt(hStmt, i++, account->preset_account);
	_bindStmtFieldDataInt(hStmt, i++, account->target_storage);
	_bindStmtFieldDataInt(hStmt, i++, account->check_interval);
	_bindStmtFieldDataInt(hStmt, i++, account->options.priority);
	_bindStmtFieldDataInt(hStmt, i++, account->options.keep_local_copy);
	_bindStmtFieldDataInt(hStmt, i++, account->options.req_delivery_receipt);
	_bindStmtFieldDataInt(hStmt, i++, account->options.req_read_receipt);
	_bindStmtFieldDataInt(hStmt, i++, account->options.download_limit);
	_bindStmtFieldDataInt(hStmt, i++, account->options.block_address);
	_bindStmtFieldDataInt(hStmt, i++, account->options.block_subject);
	_bindStmtFieldDataString(hStmt, i++, account->options.display_name_from, 0, DISPLAY_NAME_FROM_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataInt(hStmt, i++, account->options.reply_with_body);
	_bindStmtFieldDataInt(hStmt, i++, account->options.forward_with_files);
	_bindStmtFieldDataInt(hStmt, i++, account->options.add_myname_card);
	_bindStmtFieldDataInt(hStmt, i++, account->options.add_signature);
	_bindStmtFieldDataString(hStmt, i++, account->options.signature, 0, SIGNATURE_LEN_IN_MAIL_ACCOUNT_TBL);
	if (account->options.add_my_address_to_bcc != 0)
		account->options.add_my_address_to_bcc = 1;
	_bindStmtFieldDataInt(hStmt, i++, account->options.add_my_address_to_bcc);
	_bindStmtFieldDataInt(hStmt, i++, account->my_account_id);
	_bindStmtFieldDataInt(hStmt, i++, account->index_color);
	_bindStmtFieldDataInt(hStmt, i++, account->sync_status);
	_bindStmtFieldDataInt(hStmt, i++, account_id);
 
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	/*  validate account existence */
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION(" no matched account found...");
	
		error = EM_STORAGE_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	/*  get password file name */
	if ((error = em_storage_get_password_file_name(account_id, recv_password_file_name, send_password_file_name)) != EM_STORAGE_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_storage_get_password_file_name failed.");
		goto FINISH_OFF;
	}

	/*  save passwords to the secure storage */
	EM_DEBUG_LOG("save to the secure storage : recv_file[%s], recv_pass[%s], send_file[%s], send_pass[%s]", recv_password_file_name, account->password, send_password_file_name, account->sending_password);
	if (account->password && (strlen(account->password) > 0)) {
		if (ssm_write_buffer(account->password, strlen(account->password), recv_password_file_name, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
			EM_DEBUG_EXCEPTION(" ssm_write_buffer failed -recv password : file[%s]", recv_password_file_name);
			error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;		
		}
	}

	if (account->sending_password && (strlen(account->sending_password) > 0)) {
		if (ssm_write_buffer(account->sending_password, strlen(account->sending_password), send_password_file_name, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
			EM_DEBUG_EXCEPTION(" ssm_write_buffer failed -send password : file[%s]", send_password_file_name);
			error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;		
		}
	}
	
	if (!em_storage_notify_storage_event(NOTI_ACCOUNT_UPDATE, account->account_id, 0, NULL, 0))
		EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event[ NOTI_ACCOUNT_UPDATE] : Notification Failed >>> ");
	ret = true;
	
FINISH_OFF:
 
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_sync_status_of_account(int account_id, int *result_sync_status,int *err_code) 
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], result_sync_status [%p], err_code[%p]", account_id, result_sync_status, err_code);

	if(!result_sync_status) {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int error = EM_STORAGE_ERROR_NONE, rc, ret = false, sync_status, count, i, col_index;
	char sql_query_string[QUERY_SIZE] = {0, };
	char **result = NULL;
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	if(account_id)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT sync_status FROM mail_account_tbl WHERE account_id = %d", account_id);	 
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT sync_status FROM mail_account_tbl");	
 
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (!count) {
		EM_DEBUG_EXCEPTION("no matched account found...");
		error = EM_STORAGE_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

	col_index = 1;
	*result_sync_status = 0;

	for(i = 0; i < count; i++) {
		_getTableFieldDataInt(result, &sync_status, col_index++);
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

EXPORT_API int em_storage_update_sync_status_of_account(int account_id, emf_set_type_t set_operator, int sync_status, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], set_operator[%d], sync_status [%d], transaction[%d], err_code[%p]", account_id, set_operator, sync_status, transaction, err_code);
	
	int error = EM_STORAGE_ERROR_NONE, rc, ret = false, set_value = sync_status, result_sync_status;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	if(set_operator != SET_TYPE_SET && account_id) {
		if(!em_storage_get_sync_status_of_account(account_id, &result_sync_status, &error)) {
			EM_DEBUG_EXCEPTION("em_storage_get_sync_status_of_account failed [%d]", error);
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
				EM_DEBUG_EXCEPTION("EMF_ERROR_NOT_SUPPORTED [%d]", set_operator);
				error = EMF_ERROR_NOT_SUPPORTED;
				break;
		}
		EM_DEBUG_LOG("set_value [%d]", set_value);
	}

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);

	if(account_id)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_account_tbl SET sync_status = %d WHERE account_id = %d", set_value, account_id);	 
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_account_tbl SET sync_status = %d WHERE receiving_server_type <> 5", set_value);	 

	EM_DEBUG_LOG("sql_query_string [%s]", sql_query_string);
 
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	rc = sqlite3_changes(local_db_handle);

	if (rc == 0) {
		EM_DEBUG_EXCEPTION("no matched account found...");
		error = EM_STORAGE_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	if (!em_storage_notify_storage_event(NOTI_ACCOUNT_UPDATE_SYNC_STATUS, account_id, 0, NULL, 0))
		EM_DEBUG_EXCEPTION("em_storage_notify_storage_event[NOTI_ACCOUNT_UPDATE_SYNC_STATUS] : Notification failed");
	ret = true;
	
FINISH_OFF:

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_add_account(emf_mail_account_tbl_t* account, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account[%p], transaction[%d], err_code[%p]", account, transaction, err_code);
	
	if (!account)  {
		EM_DEBUG_EXCEPTION("account[%p], transaction[%d], err_code[%p]", account, transaction, err_code);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);

	char *sql = "SELECT max(rowid) FROM mail_account_tbl;";
	char **result = NULL;
	
	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL==result[1]) rc = 1;
	else rc = atoi(result[1])+1;
	sqlite3_free_table(result);
	result = NULL;

	account->account_id = rc;

	if ((error = em_storage_get_password_file_name(account->account_id, recv_password_file_name, send_password_file_name)) != EM_STORAGE_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_storage_get_password_file_name failed.");
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG(" >>>> ACCOUNT_ID [ %d ] ", account->account_id);
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_account_tbl VALUES "
		"(	  "
		"   ?  "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "		/*  password - for receving */
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "		/*  sending_password */
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "	 /*  POP before SMTP authentication */
		"  , ? "	 /*  APOP Authentication */
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "
		"  , ? "		/*  add full_address_bcc */
		"  , ? "		/*  my account id */
		"  , ? "	  /*  index_color */
		"  , ? "	  /*  Sync Status */
		") ");
	
 
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG(">>>> SQL STMT [ %s ] ", sql_query_string);
	_bindStmtFieldDataInt(hStmt, ACCOUNT_BIND_TYPE_IDX_IN_MAIL_ACCOUNT_TBL, account->account_bind_type);
	_bindStmtFieldDataString(hStmt, ACCOUNT_NAME_IDX_IN_MAIL_ACCOUNT_TBL, (char *)account->account_name, 0, ACCOUNT_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataInt(hStmt, RECEIVING_SERVER_TYPE_TYPE_IDX_IN_MAIL_ACCOUNT_TBL, account->receiving_server_type);
	_bindStmtFieldDataString(hStmt, RECEIVING_SERVER_ADDR_IDX_IN_MAIL_ACCOUNT_TBL, (char *)account->receiving_server_addr, 0, RECEIVING_SERVER_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataString(hStmt, EMAIL_ADDR_IDX_IN_MAIL_ACCOUNT_TBL, (char *)account->email_addr, 0, EMAIL_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataString(hStmt, USER_NAME_IDX_IN_MAIL_ACCOUNT_TBL, (char *)account->user_name, 0, USER_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataString(hStmt, PASSWORD_IDX_IN_MAIL_ACCOUNT_TBL, (char *)"", 0, PASSWORD_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataInt(hStmt, RETRIEVAL_MODE_IDX_IN_MAIL_ACCOUNT_TBL, account->retrieval_mode);
	_bindStmtFieldDataInt(hStmt, PORT_NUM_IDX_IN_MAIL_ACCOUNT_TBL, account->port_num);
	_bindStmtFieldDataInt(hStmt, USE_SECURITY_IDX_IN_MAIL_ACCOUNT_TBL, account->use_security);
	_bindStmtFieldDataInt(hStmt, SENDING_SERVER_TYPE_IDX_IN_MAIL_ACCOUNT_TBL, account->sending_server_type);
	_bindStmtFieldDataString(hStmt, SENDING_SERVER_ADDR_IDX_IN_MAIL_ACCOUNT_TBL, (char *)account->sending_server_addr, 0, SENDING_SERVER_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataInt(hStmt, SENDING_PORT_NUM_IDX_IN_MAIL_ACCOUNT_TBL, account->sending_port_num);
	_bindStmtFieldDataInt(hStmt, SENDING_AUTH_IDX_IN_MAIL_ACCOUNT_TBL, account->sending_auth);
	_bindStmtFieldDataInt(hStmt, SENDING_SECURITY_IDX_IN_MAIL_ACCOUNT_TBL, account->sending_security);
	_bindStmtFieldDataString(hStmt, SENDING_USER_IDX_IN_MAIL_ACCOUNT_TBL, (char *)account->sending_user, 0, SENDING_USER_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataString(hStmt, SENDING_PASSWORD_IDX_IN_MAIL_ACCOUNT_TBL, (char *)"", 0, SENDING_PASSWORD_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataString(hStmt, DISPLAY_NAME_IDX_IN_MAIL_ACCOUNT_TBL, (char *)account->display_name, 0, DISPLAY_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataString(hStmt, REPLY_TO_ADDR_IDX_IN_MAIL_ACCOUNT_TBL, (char *)account->reply_to_addr, 0, REPLY_TO_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataString(hStmt, RETURN_ADDR_IDX_IN_MAIL_ACCOUNT_TBL, (char *)account->return_addr, 0, RETURN_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataInt(hStmt, ACCOUNT_ID_IDX_IN_MAIL_ACCOUNT_TBL, account->account_id);
	_bindStmtFieldDataInt(hStmt, KEEP_ON_SERVER_IDX_IN_MAIL_ACCOUNT_TBL, account->keep_on_server);
	_bindStmtFieldDataInt(hStmt, FLAG1_IDX_IN_MAIL_ACCOUNT_TBL, account->flag1);
	_bindStmtFieldDataInt(hStmt, FLAG2_IDX_IN_MAIL_ACCOUNT_TBL, account->flag2);
	_bindStmtFieldDataInt(hStmt, POP_BEFORE_SMTP_IDX_IN_MAIL_ACCOUNT_TBL, account->pop_before_smtp); /* POP before SMTP authentication [deepam.p@siso.com] */
	_bindStmtFieldDataInt(hStmt, APOP_IDX_IN_MAIL_ACCOUNT_TBL, account->apop); /* APOP Support [deepam.p@siso.com] */
	_bindStmtFieldDataString(hStmt, LOGO_ICON_PATH_IDX_IN_MAIL_ACCOUNT_TBL, account->logo_icon_path, 0, LOGO_ICON_PATH_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataInt(hStmt, PRESET_ACCOUNT_IDX_IN_MAIL_ACCOUNT_TBL, account->preset_account);
	_bindStmtFieldDataInt(hStmt, TARGET_STORAGE_IDX_IN_MAIL_ACCOUNT_TBL, account->target_storage);
	_bindStmtFieldDataInt(hStmt, CHECK_INTERVAL_IDX_IN_MAIL_ACCOUNT_TBL, account->check_interval);
	_bindStmtFieldDataInt(hStmt, PRIORITY_IDX_IN_MAIL_ACCOUNT_TBL, account->options.priority);
	_bindStmtFieldDataInt(hStmt, KEEP_LOCAL_COPY_IDX_IN_MAIL_ACCOUNT_TBL, account->options.keep_local_copy);
	_bindStmtFieldDataInt(hStmt, REQ_DELIVERY_RECEIPT_IDX_IN_MAIL_ACCOUNT_TBL, account->options.req_delivery_receipt);
	_bindStmtFieldDataInt(hStmt, REQ_READ_RECEIPT_IDX_IN_MAIL_ACCOUNT_TBL, account->options.req_read_receipt);
	_bindStmtFieldDataInt(hStmt, DOWNLOAD_LIMIT_IDX_IN_MAIL_ACCOUNT_TBL, account->options.download_limit);
	_bindStmtFieldDataInt(hStmt, BLOCK_ADDRESS_IDX_IN_MAIL_ACCOUNT_TBL, account->options.block_address);
	_bindStmtFieldDataInt(hStmt, BLOCK_SUBJECT_IDX_IN_MAIL_ACCOUNT_TBL, account->options.block_subject);
	_bindStmtFieldDataString(hStmt, DISPLAY_NAME_FROM_IDX_IN_MAIL_ACCOUNT_TBL, account->options.display_name_from, 0, DISPLAY_NAME_FROM_LEN_IN_MAIL_ACCOUNT_TBL);
	_bindStmtFieldDataInt(hStmt, REPLY_WITH_BODY_IDX_IN_MAIL_ACCOUNT_TBL, account->options.reply_with_body);
	_bindStmtFieldDataInt(hStmt, FORWARD_WITH_FILES_IDX_IN_MAIL_ACCOUNT_TBL, account->options.forward_with_files);
	_bindStmtFieldDataInt(hStmt, ADD_MYNAME_CARD_IDX_IN_MAIL_ACCOUNT_TBL, account->options.add_myname_card);
	_bindStmtFieldDataInt(hStmt, ADD_SIGNATURE_IDX_IN_MAIL_ACCOUNT_TBL, account->options.add_signature);
	_bindStmtFieldDataString(hStmt, SIGNATURE_IDX_IN_MAIL_ACCOUNT_TBL, account->options.signature, 0, SIGNATURE_LEN_IN_MAIL_ACCOUNT_TBL);
	if (account->options.add_my_address_to_bcc != 0)
		account->options.add_my_address_to_bcc = 1;
	_bindStmtFieldDataInt(hStmt, ADD_MY_ADDRESS_TO_BCC_IDX_IN_MAIL_ACCOUNT_TBL, account->options.add_my_address_to_bcc);
	_bindStmtFieldDataInt(hStmt, MY_ACCOUNT_ID_IDX_IN_MAIL_ACCOUNT_TBL, account->my_account_id);
	_bindStmtFieldDataInt(hStmt, INDEX_COLOR_IDX_IN_MAIL_ACCOUNT_TBL, account->index_color);
	_bindStmtFieldDataInt(hStmt, SYNC_STATUS_IDX_IN_MAIL_ACCOUNT_TBL, account->sync_status);
	
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);

	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d, errmsg = %s.", rc, sqlite3_errmsg(local_db_handle)));
	

	/*  save passwords to the secure storage */
	EM_DEBUG_LOG("save to the secure storage : recv_file[%s], recv_pass[%s], send_file[%s], send_pass[%s]", recv_password_file_name, account->password, send_password_file_name, account->sending_password);
	if (ssm_write_buffer(account->password, strlen(account->password), recv_password_file_name, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION("ssm_write_buffer failed - recv password : file[%s]", recv_password_file_name);
		error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;		
	}
	if (ssm_write_buffer(account->sending_password, strlen(account->sending_password), send_password_file_name, SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION("ssm_write_buffer failed - send password : file[%s]", send_password_file_name);
		error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;		
	}

	if (!em_storage_notify_storage_event(NOTI_ACCOUNT_ADD, account->account_id, 0, NULL, 0))
		EM_DEBUG_EXCEPTION("em_storage_notify_storage_event[NOTI_ACCOUNT_ADD] : Notification failed");
	ret = true;
	
FINISH_OFF:
	
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}		
	}
	else
		EM_DEBUG_LOG("hStmt is NULL!!!");
	
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_delete_account(int account_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], transaction[%d], err_code[%p]", account_id, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID)  {	
		EM_DEBUG_EXCEPTION(" account_id[%d]", account_id);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);

	/*  TODO : delete password files - file names can be obtained from db or a rule that makes a name */
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];

/*
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT password, sending_password FROM mail_account_tbl WHERE account_id = %d", account_id);
	EM_DEBUG_LOG("query = [%s]", sql_query_string);

	execute a sql and count rows 
	rc = sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL);
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	rc = sqlite3_step(hStmt); 
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_LOG(" no matched account found...");
		
		error = EM_STORAGE_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}
	_getStmtFieldDataString(hStmt, &(recv_password_file_name), 0, 0);
	_getStmtFieldDataString(hStmt, &(send_password_file_name), 0, 1);
*/
	/*  get password file name */
	if ((error = em_storage_get_password_file_name(account_id, recv_password_file_name, send_password_file_name)) != EM_STORAGE_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_storage_get_password_file_name failed.");
		goto FINISH_OFF;
	}

	/*  delete from db */
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_account_tbl WHERE account_id = %d", account_id);
 
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  validate account existence */
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no matched account found...");
		error = EM_STORAGE_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

		/*  delete from secure storage */
	if (ssm_delete_file(recv_password_file_name,  SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION(" ssm_delete_file failed -recv password : file[%s]", recv_password_file_name);
		error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;		
	}
	if (ssm_delete_file(send_password_file_name,  SSM_FLAG_SECRET_OPERATION, NULL) < 0) {
		EM_DEBUG_EXCEPTION(" ssm_delete_file failed -send password : file[%s]", send_password_file_name);
		error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;		
	}
	ret = true;
	
FINISH_OFF:
 
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_free_account(emf_mail_account_tbl_t** account_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_list[%p], count[%d], err_code[%p]", account_list, count, err_code);
	
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	
	if (count > 0)  {
		if (!account_list || !*account_list)  {
			EM_DEBUG_EXCEPTION("account_list[%p], count[%d]", account_list, count);
			error = EM_STORAGE_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		emf_mail_account_tbl_t* p = *account_list;
		int i = 0;
		
		for (; i < count; i++)  {
			EM_SAFE_FREE(p[i].account_name);
			EM_SAFE_FREE(p[i].receiving_server_addr);
			EM_SAFE_FREE(p[i].email_addr);
			EM_SAFE_FREE(p[i].user_name);
			EM_SAFE_FREE(p[i].password);
			EM_SAFE_FREE(p[i].sending_server_addr);
			EM_SAFE_FREE(p[i].sending_user);
			EM_SAFE_FREE(p[i].sending_password);
			EM_SAFE_FREE(p[i].display_name);
			EM_SAFE_FREE(p[i].reply_to_addr);
			EM_SAFE_FREE(p[i].return_addr);
			EM_SAFE_FREE(p[i].logo_icon_path);
			EM_SAFE_FREE(p[i].options.display_name_from);
			EM_SAFE_FREE(p[i].options.signature);
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

EXPORT_API int em_storage_get_mailbox_count(int account_id, int local_yn, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], count[%p], transaction[%d], err_code[%p]", account_id, local_yn, count, transaction, err_code);
	
	if ((account_id < FIRST_ACCOUNT_ID) || (count == NULL))  {
		EM_DEBUG_EXCEPTION(" account_list[%d], local_yn[%d], count[%p]", account_id, local_yn, count);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_box_tbl WHERE account_id = %d AND local_yn = %d", account_id, local_yn);
	
	char **result;
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);
 

	ret = true;
	
FINISH_OFF:
 
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_mailbox(int account_id, int local_yn, email_mailbox_sort_type_t sort_type, int *select_num, emf_mailbox_tbl_t** mailbox_list, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], select_num[%p], mailbox_list[%p], transaction[%d], err_code[%p]", account_id, local_yn, select_num, mailbox_list, transaction, err_code);
	
	if (!select_num || !mailbox_list) {
		EM_DEBUG_EXCEPTION("Invalid parameters");
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;

		return false;
	}

	int i = 0, rc, count = 0, ret = false, col_index;
	int error = EM_STORAGE_ERROR_NONE;
	emf_mailbox_tbl_t* p_data_tbl = NULL;
	char **result;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *fields = "mailbox_id, account_id, local_yn, mailbox_name, mailbox_type, alias, sync_with_server_yn, modifiable_yn, total_mail_count_on_server, has_archived_mails, mail_slot_size ";

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
		
	if (account_id == ALL_ACCOUNT) {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s FROM mail_box_tbl ", fields);
		if (local_yn == EMF_MAILBOX_FROM_SERVER || local_yn == EMF_MAILBOX_FROM_LOCAL)
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " WHERE local_yn = %d ", local_yn);
	}
	else {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s FROM mail_box_tbl WHERE account_id = %d  ", fields, account_id);
	if (local_yn == EMF_MAILBOX_FROM_SERVER || local_yn == EMF_MAILBOX_FROM_LOCAL)
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " AND local_yn = %d ", local_yn);
	}

	switch (sort_type) {
		case EMAIL_MAILBOX_SORT_BY_NAME_ASC :
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " ORDER BY mailbox_name ASC");
			break;

		case EMAIL_MAILBOX_SORT_BY_NAME_DSC :
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " ORDER BY mailbox_name DESC");
			break;

		case EMAIL_MAILBOX_SORT_BY_TYPE_ASC :
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " ORDER BY mailbox_type ASC");
			break;

		case EMAIL_MAILBOX_SORT_BY_TYPE_DSC :
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " ORDER BY mailbox_type DEC");
			break;
	}

	EM_DEBUG_LOG("em_storage_get_mailbox : query[%s]", sql_query_string);


	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)))
 
	if (!(p_data_tbl = (emf_mailbox_tbl_t*)malloc(sizeof(emf_mailbox_tbl_t) * count))) {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emf_mailbox_tbl_t)*count);
	
	col_index = 11;
	
	for (i = 0; i < count; i++)  {
		_getTableFieldDataInt(result, &(p_data_tbl[i].mailbox_id), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].account_id), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].local_yn), col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].mailbox_name), 0, col_index++);
		_getTableFieldDataInt(result, (int *)&(p_data_tbl[i].mailbox_type), col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].alias), 0, col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].sync_with_server_yn), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].modifiable_yn), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].total_mail_count_on_server), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].has_archived_mails), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].mail_slot_size), col_index++);
	}
	
	ret = true;
	
FINISH_OFF:
	if (result)
		sqlite3_free_table(result);

	if (ret == true)  {
		*mailbox_list = p_data_tbl;
		*select_num = count;
		EM_DEBUG_LOG("Mailbox Count [ %d]", count);
	}
	else if (p_data_tbl != NULL)
		em_storage_free_mailbox(&p_data_tbl, count, NULL);
	
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_get_mailbox_ex(int account_id, int local_yn, int with_count, int *select_num, emf_mailbox_tbl_t** mailbox_list, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], select_num[%p], mailbox_list[%p], transaction[%d], err_code[%p]", account_id, local_yn, select_num, mailbox_list, transaction, err_code);
	
	if (!select_num || !mailbox_list) {
		EM_DEBUG_EXCEPTION("Invalid parameters");
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;

		return false;
	}

	int i = 0;
	int rc;
	int count = 0;
	int ret = false;
	int col_index;
	int error = EM_STORAGE_ERROR_NONE;
	emf_mailbox_tbl_t* p_data_tbl = NULL;
	char **result;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *fields = "MBT.mailbox_id, MBT.account_id, local_yn, MBT.mailbox_name, MBT.mailbox_type, alias, sync_with_server_yn, modifiable_yn, total_mail_count_on_server, has_archived_mails, mail_slot_size ";
	int add = 0;
	int read_count = 0;
	int total_count = 0;

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);

	add = 0;
	if (with_count == 0) {	/*  without count  */
		col_index = 11;
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s FROM mail_box_tbl AS MBT", fields);
		if (account_id > ALL_ACCOUNT) {
			add = 1;
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " WHERE account_id = %d  ", account_id);
		}
	}
	else {	/*  with read count and total count */
		col_index = 13;
		if (account_id > ALL_ACCOUNT) {
			add = 1;
			SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s, total, read  FROM mail_box_tbl AS MBT LEFT OUTER JOIN (SELECT mailbox_name, count(mail_id) AS total, SUM(flags_seen_field) AS read FROM mail_tbl WHERE account_id = %d GROUP BY mailbox_name) AS MT ON MBT.mailbox_name = MT.mailbox_name WHERE account_id = %d ", fields, account_id, account_id);
		}
		else {
			SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s, total, read  FROM mail_box_tbl AS MBT LEFT OUTER JOIN (SELECT mailbox_name, count(mail_id) AS total, SUM(flags_seen_field) AS read FROM mail_tbl GROUP BY mailbox_name) AS MT ON MBT.mailbox_name = MT.mailbox_name ", fields);
		}
	}

	if (local_yn == EMF_MAILBOX_FROM_SERVER || local_yn == EMF_MAILBOX_FROM_LOCAL) {
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " %s local_yn = %d ", (add ? "WHERE" : "AND"), local_yn);
	}

	SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " ORDER BY MBT.mailbox_name ");
	EM_DEBUG_LOG("query[%s]", sql_query_string);


	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)))
 
	EM_DEBUG_LOG("result count [%d]", count);

	if (!(p_data_tbl = (emf_mailbox_tbl_t*)malloc(sizeof(emf_mailbox_tbl_t) * count)))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emf_mailbox_tbl_t)*count);
	
	for (i = 0; i < count; i++)  {
		_getTableFieldDataInt(result, &(p_data_tbl[i].mailbox_id), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].account_id), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].local_yn), col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].mailbox_name), 0, col_index++);
		_getTableFieldDataInt(result, (int *)&(p_data_tbl[i].mailbox_type), col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].alias), 0, col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].sync_with_server_yn), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].modifiable_yn), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].total_mail_count_on_server), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].has_archived_mails), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].mail_slot_size), col_index++);

		if (with_count == 1) {
			_getTableFieldDataInt(result, &(total_count), col_index++);
			p_data_tbl[i].total_mail_count_on_local = total_count;
			_getTableFieldDataInt(result, &(read_count), col_index++);
			p_data_tbl[i].unread_count = total_count - read_count;			/*  return unread count, NOT  */
		}
		/*
		EM_DEBUG_LOG("[%d]", i);
		EM_DEBUG_LOG("p_data_tbl[%d].mailbox_id[%d]", i, p_data_tbl[i].mailbox_id);
		EM_DEBUG_LOG("p_data_tbl[%d].account_id[%d]", i, p_data_tbl[i].account_id);
		EM_DEBUG_LOG("p_data_tbl[%d].local_yn[%d]", i, p_data_tbl[i].local_yn);
		EM_DEBUG_LOG("p_data_tbl[%d].mailbox_name[%s]", i, p_data_tbl[i].mailbox_name);
		EM_DEBUG_LOG("p_data_tbl[%d].mailbox_type[%d]", i, p_data_tbl[i].mailbox_type);
		EM_DEBUG_LOG("p_data_tbl[%d].alias[%s]", i, p_data_tbl[i].alias);
		EM_DEBUG_LOG("p_data_tbl[%d].sync_with_server_yn[%d]", i, p_data_tbl[i].sync_with_server_yn);
		EM_DEBUG_LOG("p_data_tbl[%d].modifiable_yn[%d]", i, p_data_tbl[i].modifiable_yn);
		EM_DEBUG_LOG("p_data_tbl[%d].has_archived_mails[%d]", i, p_data_tbl[i].has_archived_mails);
		EM_DEBUG_LOG("p_data_tbl[%d].mail_slot_size[%d]", i, p_data_tbl[i].mail_slot_size);
		EM_DEBUG_LOG("p_data_tbl[%d].unread_count[%d]", i, p_data_tbl[i].unread_count);
		EM_DEBUG_LOG("p_data_tbl[%d].total_mail_count_on_local[%d]", i, p_data_tbl[i].total_mail_count_on_local);
		EM_DEBUG_LOG("p_data_tbl[%d].total_mail_count_on_server[%d]", i, p_data_tbl[i].total_mail_count_on_server);
		*/
	}
	
	ret = true;
	
FINISH_OFF:
	if (result)
		sqlite3_free_table(result);

	if (ret == true)  {
		*mailbox_list = p_data_tbl;
		*select_num = count;
		EM_DEBUG_LOG("Mailbox Count [ %d]", count);
	}
	else if (p_data_tbl != NULL)
		em_storage_free_mailbox(&p_data_tbl, count, NULL);
	
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_child_mailbox_list(int account_id, char *parent_mailbox_name, int *select_num, emf_mailbox_tbl_t** mailbox_list, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], parent_mailbox_name[%p], select_num[%p], mailbox_list[%p], transaction[%d], err_code[%p]", account_id, parent_mailbox_name, select_num, mailbox_list, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID || !select_num || !mailbox_list)  {
		EM_DEBUG_EXCEPTION("account_id[%d], parent_mailbox_name[%p], select_num[%p], mailbox_list[%p]", account_id, parent_mailbox_name, select_num, mailbox_list);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int i = 0, rc, count = 0, ret = false, col_index;
	int error = EM_STORAGE_ERROR_NONE;
	emf_mailbox_tbl_t* p_data_tbl = NULL;
	char **result;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *fields = "mailbox_id, account_id, local_yn, mailbox_name, mailbox_type, alias, sync_with_server_yn, modifiable_yn, total_mail_count_on_server, has_archived_mails, mail_slot_size ";

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);

	if (parent_mailbox_name)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s FROM mail_box_tbl WHERE account_id = %d  AND UPPER(mailbox_name) LIKE UPPER('%s/%%') ORDER BY mailbox_name", fields, account_id, parent_mailbox_name);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s FROM mail_box_tbl WHERE account_id = %d  AND (mailbox_name NOT LIKE '%%/%%') ORDER BY mailbox_name", fields, account_id);

	EM_DEBUG_LOG("query : %s", sql_query_string);

	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	if (!(p_data_tbl = (emf_mailbox_tbl_t*)malloc(sizeof(emf_mailbox_tbl_t) * count)))  {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emf_mailbox_tbl_t)*count);
	col_index = 11;
	
	for (i = 0; i < count; i++)  {
		_getTableFieldDataInt(result, &(p_data_tbl[i].mailbox_id), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].account_id), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].local_yn), col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].mailbox_name), 0, col_index++);
		_getTableFieldDataInt(result, (int *)&(p_data_tbl[i].mailbox_type), col_index++);
		_getTableFieldDataString(result, &(p_data_tbl[i].alias), 0, col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].sync_with_server_yn), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].modifiable_yn), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].total_mail_count_on_server), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].has_archived_mails), col_index++);
		_getTableFieldDataInt(result, &(p_data_tbl[i].mail_slot_size), col_index++);
	}

	if (result)
		sqlite3_free_table(result);

	ret = true;
	
FINISH_OFF:
	if (ret == true)  {
		*mailbox_list = p_data_tbl;
		*select_num = count;
		EM_DEBUG_LOG(" Mailbox Count [ %d] ", count);
	}
	else if (p_data_tbl != NULL)
		em_storage_free_mailbox(&p_data_tbl, count, NULL);
	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
 
EXPORT_API int em_storage_get_mailbox_by_modifiable_yn(int account_id, int local_yn, int *select_num, emf_mailbox_tbl_t** mailbox_list, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], select_num[%p], mailbox_list[%p], transaction[%d], err_code[%p]", account_id, local_yn, select_num, mailbox_list, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID || !select_num || !mailbox_list)  {
		EM_DEBUG_EXCEPTION("\account_id[%d], local_yn[%d], select_num[%p], mailbox_list[%p]", account_id, local_yn, select_num, mailbox_list);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int i = 0, rc, count = 0, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	emf_mailbox_tbl_t* p_data_tbl = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *fields = "mailbox_id, account_id, local_yn, mailbox_name, mailbox_type, alias, sync_with_server_yn, modifiable_yn, total_mail_count_on_server, has_archived_mails, mail_slot_size ";
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s FROM mail_box_tbl WHERE account_id = %d AND modifiable_yn = 0 ORDER BY mailbox_name", fields, account_id);
	EM_DEBUG_LOG("sql[%s]", sql_query_string);
	
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	char **result;
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	sqlite3_free_table(result);
	if (count == 0) {
		EM_DEBUG_EXCEPTION(" no mailbox_name found...");
		
		ret = true;
		goto FINISH_OFF;
	}
	
	if (!(p_data_tbl = (emf_mailbox_tbl_t*)malloc(sizeof(emf_mailbox_tbl_t) * count)))  {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emf_mailbox_tbl_t)*count);
	
	int col_index = 0;
	for (i = 0; i < count; i++)  {
		col_index = 0;
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].mailbox_id), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].account_id), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].local_yn), col_index++);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].mailbox_name), 0, col_index++);
		_getStmtFieldDataInt(hStmt, (int *)&(p_data_tbl[i].mailbox_type), col_index++);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].alias), 0, col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].sync_with_server_yn), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].modifiable_yn), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].total_mail_count_on_server), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].has_archived_mails), col_index++);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].mail_slot_size), col_index++);
 
		/*  rc = sqlite3_step(hStmt); */
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
	}
	
	ret = true;
	
FINISH_OFF:
	if (ret == true) {
		*mailbox_list = p_data_tbl;
		*select_num = count;
		EM_DEBUG_LOG("Mailbox Count[%d]", count);
	}
	else if (p_data_tbl != NULL)
		em_storage_free_mailbox(&p_data_tbl, count, NULL);
 	
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}



EXPORT_API int em_storage_get_mailbox_by_name(int account_id, int local_yn, char *mailbox_name, emf_mailbox_tbl_t** result_mailbox, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], mailbox_name[%s], result_mailbox[%p], transaction[%d], err_code[%p]", account_id, local_yn, mailbox_name, result_mailbox, transaction, err_code);
	EM_PROFILE_BEGIN(profile_em_storage_get_mailbox_by_name);
	
	if (account_id < FIRST_ACCOUNT_ID || !mailbox_name || !result_mailbox)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], local_yn[%d], mailbox_name[%s], result_mailbox[%p]", account_id, local_yn, mailbox_name, result_mailbox);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	emf_mailbox_tbl_t* p_data_tbl = NULL;
 
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *fields = "mailbox_id, account_id, local_yn, mailbox_name, mailbox_type, alias, sync_with_server_yn, modifiable_yn, total_mail_count_on_server, has_archived_mails, mail_slot_size ";
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	if (local_yn == -1)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s FROM mail_box_tbl WHERE account_id = %d AND mailbox_name = '%s'", fields, account_id, mailbox_name);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s FROM mail_box_tbl WHERE account_id = %d AND local_yn = %d AND mailbox_name = '%s'", fields, account_id, local_yn, mailbox_name);

	EM_DEBUG_LOG("query = [%s]", sql_query_string);

	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION("no matched mailbox_name found...");
		error = EM_STORAGE_ERROR_MAILBOX_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	if (!(p_data_tbl = (emf_mailbox_tbl_t*)malloc(sizeof(emf_mailbox_tbl_t))))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emf_mailbox_tbl_t));
	int col_index = 0;	
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->mailbox_id), col_index++);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->account_id), col_index++);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->local_yn), col_index++);
	_getStmtFieldDataString(hStmt, &(p_data_tbl->mailbox_name), 0, col_index++);
	_getStmtFieldDataInt(hStmt, (int *)&(p_data_tbl->mailbox_type), col_index++);
	_getStmtFieldDataString(hStmt, &(p_data_tbl->alias), 0, col_index++);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->sync_with_server_yn), col_index++);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->modifiable_yn), col_index++);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->total_mail_count_on_server), col_index++);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->has_archived_mails), col_index++);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->mail_slot_size), col_index++);

	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*result_mailbox = p_data_tbl;
	else if (p_data_tbl != NULL)
		em_storage_free_mailbox(&p_data_tbl, 1, NULL);
	
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(profile_em_storage_get_mailbox_by_name);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_get_mailbox_by_mailbox_type(int account_id, emf_mailbox_type_e mailbox_type, emf_mailbox_tbl_t** mailbox_name, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_type[%d], mailbox_name[%p], transaction[%d], err_code[%p]", account_id, mailbox_type, mailbox_name, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID || (mailbox_type < EMF_MAILBOX_TYPE_INBOX || mailbox_type > EMF_MAILBOX_TYPE_ALL_EMAILS) || !mailbox_name) {

		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_type[%d], mailbox_name[%p]", account_id, mailbox_type, mailbox_name);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	emf_mailbox_tbl_t *p_data_tbl = NULL;
	emf_mail_account_tbl_t *account = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0,};
	char *fields = "mailbox_id, account_id, local_yn, mailbox_name, mailbox_type, alias, sync_with_server_yn, modifiable_yn, total_mail_count_on_server, has_archived_mails, mail_slot_size ";
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	/* validate account */
	/*  Check whether the account exists. */
	if (!em_storage_get_account_by_id(account_id, EMF_ACC_GET_OPT_ACCOUNT_NAME,  &account, true, &error) || !account) {
		EM_DEBUG_EXCEPTION(" em_storage_get_account_by_id failed - %d", error);
		goto FINISH_OFF;
	}

	if (account)
		em_storage_free_account(&account, 1, NULL);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s FROM mail_box_tbl WHERE account_id = %d AND mailbox_type = %d ", fields, account_id, mailbox_type);

	EM_DEBUG_LOG("query = [%s]", sql_query_string);
 		
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE; goto FINISH_OFF; }, ("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE; goto FINISH_OFF; }, ("sqlite3_step fail:%d", rc));
	
	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION(" no matched mailbox_name found...");
		error = EM_STORAGE_ERROR_MAILBOX_NOT_FOUND;
		goto FINISH_OFF;
	}

	
	if (!(p_data_tbl = (emf_mailbox_tbl_t*)malloc(sizeof(emf_mailbox_tbl_t))))  {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emf_mailbox_tbl_t));
	
	int col_index = 0;

	_getStmtFieldDataInt(hStmt, &(p_data_tbl->mailbox_id), col_index++);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->account_id), col_index++);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->local_yn), col_index++);
	_getStmtFieldDataString(hStmt, &(p_data_tbl->mailbox_name), 0, col_index++);
	_getStmtFieldDataInt(hStmt, (int *)&(p_data_tbl->mailbox_type), col_index++);
	_getStmtFieldDataString(hStmt, &(p_data_tbl->alias), 0, col_index++);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->sync_with_server_yn), col_index++);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->modifiable_yn), col_index++);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->total_mail_count_on_server), col_index++);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->has_archived_mails), col_index++);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->mail_slot_size), col_index++);

	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*mailbox_name = p_data_tbl;
	else if (p_data_tbl != NULL)
		em_storage_free_mailbox(&p_data_tbl, 1, NULL);
	
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_mailboxname_by_mailbox_type(int account_id, emf_mailbox_type_e mailbox_type, char **mailbox_name, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_type[%d], mailbox_name[%p], transaction[%d], err_code[%p]", account_id, mailbox_type, mailbox_name, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID || (mailbox_type < EMF_MAILBOX_TYPE_INBOX || mailbox_type > EMF_MAILBOX_TYPE_ALL_EMAILS) || !mailbox_name)  {
		EM_DEBUG_EXCEPTION("account_id[%d], mailbox_type[%d], mailbox_name[%p]", account_id, mailbox_type, mailbox_name);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char *p_mailbox = NULL;
	emf_mail_account_tbl_t* account = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);

	/*  Check whether the account exists. */
	if (!em_storage_get_account_by_id(account_id, EMF_ACC_GET_OPT_ACCOUNT_NAME,  &account, true, &error) || !account) {
		EM_DEBUG_EXCEPTION("em_storage_get_account_by_id failed - %d", error);
		goto FINISH_OFF;
	}

	if (account )
		em_storage_free_account(&account, 1, NULL);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mailbox_name  FROM mail_box_tbl WHERE account_id = %d AND mailbox_type = %d ", account_id, mailbox_type);

	EM_DEBUG_LOG("query = [%s]", sql_query_string);

	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION("no matched mailbox_name found...");
		error = EM_STORAGE_ERROR_MAILBOX_NOT_FOUND;
		goto FINISH_OFF;
	}
 
	_getStmtFieldDataString(hStmt, &(p_mailbox), 0, 0);

	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*mailbox_name = p_mailbox;
	else 
		EM_SAFE_FREE(p_mailbox);
	
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_update_mailbox_modifiable_yn(int account_id, int local_yn, char *mailbox_name, int modifiable_yn, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], local_yn [%d], mailbox_name [%p], modifiable_yn [%d], transaction [%d], err_code [%p]", account_id, local_yn, mailbox_name, modifiable_yn, transaction, err_code);
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
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
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
 
	ret = true;

FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}


EXPORT_API int em_storage_update_mailbox_total_count(int account_id, char *mailbox_name, int total_count_on_server, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%p], total_count_on_server[%d], transaction[%d], err_code[%p]", account_id, mailbox_name, total_count_on_server,  transaction, err_code);
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	if (account_id <= 0 || !mailbox_name)  {
		EM_DEBUG_EXCEPTION("account_id[%d], mailbox_name[%p]", account_id, mailbox_name);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_box_tbl SET"
		" total_mail_count_on_server = %d"
		" WHERE account_id = %d"
		" AND mailbox_name = '%s'"
		, total_count_on_server
		, account_id
		, mailbox_name);
	EM_DEBUG_LOG("query[%s]", sql_query_string);
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
 
	ret = true;

FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}


EXPORT_API int em_storage_update_mailbox(int account_id, int local_yn, char *mailbox_name, emf_mailbox_tbl_t* result_mailbox, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], mailbox_name[%s], result_mailbox[%p], transaction[%d], err_code[%p]", account_id, local_yn, mailbox_name, result_mailbox, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !mailbox_name || !result_mailbox)  {	
		EM_DEBUG_EXCEPTION(" account_id[%d], local_yn[%d], mailbox_name[%s], result_mailbox[%p]", account_id, local_yn, mailbox_name, result_mailbox);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	DB_STMT hStmt = NULL;
	int i = 0;
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
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
			" AND mailbox_name = '%s'"
			, account_id
			, local_yn
			, mailbox_name);
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
			" AND mailbox_name = '%s'"
			, account_id
			, mailbox_name);
	}

		
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bindStmtFieldDataInt(hStmt, i++, result_mailbox->mailbox_id);
	_bindStmtFieldDataString(hStmt, i++, (char *)result_mailbox->mailbox_name ? result_mailbox->mailbox_name : "", 0, MAILBOX_NAME_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bindStmtFieldDataInt(hStmt, i++, result_mailbox->mailbox_type);
	_bindStmtFieldDataString(hStmt, i++, (char *)result_mailbox->alias ? result_mailbox->alias : "", 0, ALIAS_LEN_IN_MAIL_BOX_TBL);
	_bindStmtFieldDataInt(hStmt, i++, result_mailbox->sync_with_server_yn);
	_bindStmtFieldDataInt(hStmt, i++, result_mailbox->modifiable_yn);
	_bindStmtFieldDataInt(hStmt, i++, result_mailbox->mail_slot_size);
	_bindStmtFieldDataInt(hStmt, i++, result_mailbox->total_mail_count_on_server);
	
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	ret = true;

FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_update_mailbox_type(int account_id, int local_yn, char *mailbox_name, emf_mailbox_tbl_t* target_mailbox, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], mailbox_name[%s], target_mailbox[%p], transaction[%d], err_code[%p]", account_id, local_yn, mailbox_name, target_mailbox, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !mailbox_name || !target_mailbox)  {	
		EM_DEBUG_EXCEPTION(" account_id[%d], local_yn[%d], mailbox_name[%s], target_mailbox[%p]", account_id, local_yn, mailbox_name, target_mailbox);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	EM_DEBUG_LOG("em_storage_update_mailbox_type");
	
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
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bindStmtFieldDataInt(hStmt, i++, target_mailbox->mailbox_type);
	
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
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
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bindStmtFieldDataInt(hStmt, i++, target_mailbox->mailbox_type);
	
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	ret = true;

FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_add_mailbox(emf_mailbox_tbl_t* mailbox_name, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_name[%p], transaction[%d], err_code[%p]", mailbox_name, transaction, err_code);
	
	if (!mailbox_name)  {
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0,};
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_box_tbl VALUES "
		"( ?"  /*  mailbox_id */
		", ?"  /*  account_id */
		", ?"  /*  local_yn */
		", ?"  /*  mailbox_name */
		", ?"  /*  mailbox_type */
		", ?"  /*  alias */
		", ?"  /*  sync_with_server_yn */
		", ?"  /*  modifiable_yn */
		", ?"  /*  total_mail_count_on_server */
		", ?"  /*  has_archived_mails */
		", ? )");/*  mail_slot_size */
	
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("After sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	int col_index = 0;

	_bindStmtFieldDataInt(hStmt, col_index++, mailbox_name->mailbox_id);
	_bindStmtFieldDataInt(hStmt, col_index++, mailbox_name->account_id);
	_bindStmtFieldDataInt(hStmt, col_index++, mailbox_name->local_yn);
	_bindStmtFieldDataString(hStmt, col_index++, (char *)mailbox_name->mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_BOX_TBL);
	_bindStmtFieldDataInt(hStmt, col_index++, mailbox_name->mailbox_type);
	_bindStmtFieldDataString(hStmt, col_index++, (char *)mailbox_name->alias, 0, ALIAS_LEN_IN_MAIL_BOX_TBL);
	_bindStmtFieldDataInt(hStmt, col_index++, mailbox_name->sync_with_server_yn);
	_bindStmtFieldDataInt(hStmt, col_index++, mailbox_name->modifiable_yn);
	_bindStmtFieldDataInt(hStmt, col_index++, mailbox_name->total_mail_count_on_server);
	_bindStmtFieldDataInt(hStmt, col_index++, 0);
	_bindStmtFieldDataInt(hStmt, col_index++, mailbox_name->mail_slot_size);

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%dn", rc));
 
	if (!em_storage_notify_storage_event(NOTI_MAILBOX_ADD, mailbox_name->account_id, 0, mailbox_name->mailbox_name, 0))
		EM_DEBUG_EXCEPTION("em_storage_notify_storage_event[ NOTI_MAILBOX_ADD] : Notification Failed");
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_set_all_mailbox_modifiable_yn(int account_id, int modifiable_yn, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], modifiable_yn[%d], err_code[%p]", account_id, modifiable_yn, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID)  {

		EM_DEBUG_EXCEPTION("account_id[%d]", account_id);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0,};

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);


	SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_box_tbl SET modifiable_yn = %d WHERE account_id = %d", modifiable_yn, account_id);
 
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) 
		EM_DEBUG_EXCEPTION("All mailbox_name modifiable_yn set to 0 already");
 
		
	ret = true;
	
FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
	

}

EXPORT_API int em_storage_delete_mailbox(int account_id, int local_yn, char *mailbox_name, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], mailbox_name[%p], transaction[%d], err_code[%p]", account_id, local_yn, mailbox_name, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID)  {	
		EM_DEBUG_EXCEPTION(" account_id[%d], local_yn[%d], mailbox_name[%p]", account_id, local_yn, mailbox_name);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	if (local_yn == -1)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_box_tbl WHERE account_id = %d ", account_id);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_box_tbl WHERE account_id = %d AND local_yn = %d ", account_id, local_yn);
	
	if (mailbox_name)  {		/*  NULL means all mailbox_name */
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(1+ strlen(sql_query_string)), "AND UPPER(mailbox_name) = UPPER('%s')", mailbox_name);
	}
	
	EM_DEBUG_LOG("mailbox sql_query_string [%s]", sql_query_string);
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no (matched) mailbox_name found...");
			error = EM_STORAGE_ERROR_MAILBOX_NOT_FOUND;
		ret = true;
	}
 	ret = true;
	
FINISH_OFF:

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_free_mailbox(emf_mailbox_tbl_t** mailbox_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_list[%p], count[%d], err_code[%p]", mailbox_list, count, err_code);
	
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
		
	if (count > 0)  {
		if (!mailbox_list || !*mailbox_list)  {
			EM_DEBUG_EXCEPTION(" mailbox_list[%p], count[%d]", mailbox_list, count);
			
			error = EM_STORAGE_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		emf_mailbox_tbl_t* p = *mailbox_list;
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

EXPORT_API int em_storage_get_count_read_mail_uid(int account_id, char *mailbox_name, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%p], count[%p], transaction[%d], err_code[%p]", account_id, mailbox_name , count,  transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !mailbox_name || !count)  {		/* TODO: confirm me */
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_name[%p], count[%p], exist[%p]", account_id, mailbox_name, count);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_read_mail_uid_tbl WHERE account_id = %d AND mailbox_name = '%s'  ", account_id, mailbox_name);
	EM_DEBUG_LOG(">>> SQL [ %s ] ", sql_query_string);

	char **result;
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);
 	
	ret = true;
	
FINISH_OFF:
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}



EXPORT_API int em_storage_check_read_mail_uid(int account_id, char *mailbox_name, char *uid, int *exist, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%p], uid[%p], exist[%p], transaction[%d], err_code[%p]", account_id, mailbox_name , uid, exist, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !uid || !exist)  {	
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_name[%p], uid[%p], exist[%p]", account_id, mailbox_name , uid, exist);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	if (mailbox_name)  {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_read_mail_uid_tbl WHERE account_id = %d AND mailbox_name = '%s' AND s_uid = '%s' ", account_id, mailbox_name, uid);
	}
	else  {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_read_mail_uid_tbl WHERE account_id = %d AND s_uid = '%s' ", account_id, uid);
	}
 	
	char **result;
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*exist = atoi(result[1]);
	sqlite3_free_table(result);
 
	if (*exist > 0)
		*exist = 1;		
	else
		*exist = 0;
	
	ret = true;
	
FINISH_OFF:
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_downloaded_mail(int mail_id, emf_mail_tbl_t** mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], mail[%p], err_code[%p]", mail_id, mail, err_code);
	
	if (!mail)  {
		EM_DEBUG_EXCEPTION(" mail_id[%d], mail[%p]", mail_id, mail);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false, temp_rule;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_read_mail_uid_tbl WHERE local_uid = %d", mail_id);
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	*mail = (emf_mail_tbl_t*)malloc(sizeof(emf_mail_tbl_t));
	if (*mail == NULL) {
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		EM_DEBUG_EXCEPTION("Memory allocation for mail failed.");
		goto FINISH_OFF;

	}
	memset(*mail, 0x00, sizeof(emf_mail_tbl_t));
	
	_getStmtFieldDataInt(hStmt, &((*mail)->account_id), ACCOUNT_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_getStmtFieldDataString(hStmt, &((*mail)->mailbox_name), 0, LOCAL_MBOX_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_getStmtFieldDataInt(hStmt, &((*mail)->mail_id), LOCAL_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_getStmtFieldDataString(hStmt, &((*mail)->server_mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_getStmtFieldDataString(hStmt, &((*mail)->server_mail_id), 0, S_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_getStmtFieldDataInt(hStmt, &((*mail)->mail_size), DATA1_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_getStmtFieldDataInt(hStmt, &temp_rule, FLAG_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	
	(*mail)->server_mail_status = 1;
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("before sqlite3_finalize hStmt = %p", hStmt);
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_downloaded_list(int account_id, char *local_mbox, emf_mail_read_mail_uid_tbl_t** read_mail_uid, int *count, int transaction, int *err_code)
{
	EM_PROFILE_BEGIN(emStorageGetDownloadList);
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_mbox[%s], read_mail_uid[%p], count[%p], transaction[%d], err_code[%p]", account_id, local_mbox, read_mail_uid, count, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID || !read_mail_uid || !count)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], local_mbox[%s], read_mail_uid[%p], count[%p]", account_id, local_mbox, read_mail_uid, count);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
 
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	emf_mail_read_mail_uid_tbl_t* p_data_tbl = NULL;
	int i = 0;
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);

	if (local_mbox)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_read_mail_uid_tbl WHERE account_id = %d AND UPPER(local_mbox) = UPPER('%s')", account_id, local_mbox);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_read_mail_uid_tbl WHERE account_id = %d", account_id);

	EM_DEBUG_LOG(" sql_query_string : %s", sql_query_string);
 
	
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	char **result;
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, count, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	sqlite3_free_table(result);
	if (*count == 0)  {
		EM_DEBUG_LOG("No mail found in mail_read_mail_uid_tbl");
		ret = true;
		goto FINISH_OFF;
	}


	if (!(p_data_tbl = (emf_mail_read_mail_uid_tbl_t*)malloc(sizeof(emf_mail_read_mail_uid_tbl_t) * *count)))  {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(p_data_tbl, 0x00, sizeof(emf_mail_read_mail_uid_tbl_t)* *count);

	for (i = 0; i < *count; ++i)  {
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].account_id), ACCOUNT_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].local_mbox), 0, LOCAL_MBOX_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].local_uid), LOCAL_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].s_uid), 0, S_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].data1), DATA1_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].data2), 0, DATA2_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].flag), FLAG_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		/*  rc = sqlite3_step(hStmt); */
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
	}
	
	ret = true;

FINISH_OFF:
	if (ret == true)
		*read_mail_uid = p_data_tbl;
	else if (p_data_tbl)
		em_storage_free_read_mail_uid(&p_data_tbl, *count, NULL);
	
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" Before sqlite3_finalize hStmt = %p", hStmt);
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(emStorageGetDownloadList);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_downloaded_mail_size(int account_id, char *local_mbox, int local_uid, char *mailbox_name, char *uid, int *mail_size, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_mbox[%p], locacal_uid[%d], mailbox_name[%p], uid[%p], mail_size[%p], transaction[%d], err_code[%p]", account_id, local_mbox, local_uid, mailbox_name, uid, mail_size, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !mail_size)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], local_mbox[%p], locacal_uid[%d], mailbox_name[%p], uid[%p], mail_size[%p]", account_id, local_mbox, local_uid, mailbox_name, uid, mail_size);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	if (mailbox_name) {
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"SELECT IFNULL(MAX(data1), 0) FROM mail_read_mail_uid_tbl "
			"WHERE account_id = %d "
			"AND local_mbox = '%s' "
			"AND local_uid = %d "
			"AND mailbox_name = '%s' "
			"AND s_uid = '%s'",
			account_id, local_mbox, local_uid, mailbox_name, uid);
	}
	else {
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"SELECT IFNULL(MAX(data1), 0) FROM mail_read_mail_uid_tbl "
			"WHERE account_id = %d "
			"AND local_mbox = '%s' "
			"AND local_uid = %d "
			"AND s_uid = '%s'",
			account_id, local_mbox, local_uid, uid);
	}
	
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION(" no matched mail found....");
		error = EM_STORAGE_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;		
	}
 
	_getStmtFieldDataInt(hStmt, mail_size, 0);
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_add_downloaded_mail(emf_mail_read_mail_uid_tbl_t* read_mail_uid, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("read_mail_uid[%p], transaction[%d], err_code[%p]", read_mail_uid, transaction, err_code);
	
	if (!read_mail_uid)  {
		EM_DEBUG_EXCEPTION("read_mail_uid[%p]", read_mail_uid);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, rc2,  ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	char *sql = "SELECT max(rowid) FROM mail_read_mail_uid_tbl;";
	char **result = NULL;

	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);  */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
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
	
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc2);
	if (rc2 != SQLITE_OK)  {
		EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);
		EM_DEBUG_EXCEPTION("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle));
		
		error = EM_STORAGE_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("account_id VALUE [%d] ", read_mail_uid->account_id);
	EM_DEBUG_LOG("local_mbox VALUE [%s] ", read_mail_uid->local_mbox);
	EM_DEBUG_LOG("local_uid VALUE [%d] ", read_mail_uid->local_uid);
	EM_DEBUG_LOG("mailbox_name VALUE [%s] ", read_mail_uid->mailbox_name);
	EM_DEBUG_LOG("s_uid VALUE [%s] ", read_mail_uid->s_uid);
	EM_DEBUG_LOG("data1 VALUE [%d] ", read_mail_uid->data1);
	EM_DEBUG_LOG("data2 VALUE [%s] ", read_mail_uid->data2);
	EM_DEBUG_LOG("flag VALUE [%d] ", read_mail_uid->flag);
	EM_DEBUG_LOG("rc VALUE [%d] ", rc);

	_bindStmtFieldDataInt(hStmt, ACCOUNT_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->account_id);
	_bindStmtFieldDataString(hStmt, LOCAL_MBOX_IDX_IN_MAIL_READ_MAIL_UID_TBL, (char *)read_mail_uid->local_mbox, 0, LOCAL_MBOX_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bindStmtFieldDataInt(hStmt, LOCAL_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->local_uid);
	_bindStmtFieldDataString(hStmt, MAILBOX_NAME_IDX_IN_MAIL_READ_MAIL_UID_TBL, (char *)read_mail_uid->mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bindStmtFieldDataString(hStmt, S_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL, (char *)read_mail_uid->s_uid, 0, S_UID_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bindStmtFieldDataInt(hStmt, DATA1_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->data1);
	_bindStmtFieldDataString(hStmt, DATA2_IDX_IN_MAIL_READ_MAIL_UID_TBL, (char *)read_mail_uid->data2, 0, DATA2_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bindStmtFieldDataInt(hStmt, FLAG_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->flag);
	_bindStmtFieldDataInt(hStmt, IDX_NUM_IDX_IN_MAIL_READ_MAIL_UID_TBL, rc);

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail[%d] [%s]", rc, sqlite3_errmsg(local_db_handle)));

	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_change_read_mail_uid(int account_id, char *local_mbox, int local_uid, char *mailbox_name, char *uid, emf_mail_read_mail_uid_tbl_t* read_mail_uid, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_mbox[%p], local_uid[%d], mailbox_name[%p], uid[%p], read_mail_uid[%p], transaction[%d], err_code[%p]", account_id, local_mbox, local_uid, mailbox_name, uid, read_mail_uid, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !read_mail_uid)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], local_mbox[%p], local_uid[%d], mailbox_name[%p], uid[%p], read_mail_uid[%p]", account_id, local_mbox, local_uid, mailbox_name, uid, read_mail_uid);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_read_mail_uid_tbl SET"
		"  account_id = ?"
		", local_mbox = ?"
		", local_uid  = ?"
		", mailbox_name = ?"
		", s_uid = ?"
		", data1 = ?"
		", data2 = ?"
		", flag  = ?"
		" WHERE account_id = ?"
		" AND local_mbox  = ?"
		" AND local_uid   = ?"
		" AND mailbox_name= ?"
		" AND s_uid = ?");

	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
 
	
	int i = 0;
	
	_bindStmtFieldDataInt(hStmt, i++, read_mail_uid->account_id);
	_bindStmtFieldDataString(hStmt, i++, (char *)read_mail_uid->local_mbox, 0, LOCAL_MBOX_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bindStmtFieldDataInt(hStmt, i++, read_mail_uid->local_uid);
	_bindStmtFieldDataString(hStmt, i++, (char *)read_mail_uid->mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)read_mail_uid->s_uid, 0, S_UID_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bindStmtFieldDataInt(hStmt, i++, read_mail_uid->data1);
	_bindStmtFieldDataString(hStmt, i++, (char *)read_mail_uid->data2, 0, DATA2_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bindStmtFieldDataInt(hStmt, i++, read_mail_uid->flag);
	_bindStmtFieldDataInt(hStmt, i++, account_id);
	_bindStmtFieldDataString(hStmt, i++, (char *)local_mbox, 0, LOCAL_MBOX_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bindStmtFieldDataInt(hStmt, i++, local_uid);
	_bindStmtFieldDataString(hStmt, i++, (char *)mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)uid, 0, S_UID_LEN_IN_MAIL_READ_MAIL_UID_TBL);
 
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_remove_downloaded_mail(int account_id, char *mailbox_name, char *uid, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%s], uid[%s], transaction[%d], err_code[%p]", account_id, mailbox_name, uid, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_name[%s], uid[%s]", account_id, mailbox_name, uid);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
 	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_read_mail_uid_tbl WHERE account_id = %d ", account_id);
	
	if (mailbox_name) {		/*  NULL means all mailbox_name */
		SNPRINTF(sql_query_string+strlen(sql_query_string), sizeof(sql_query_string) - (1 + strlen(sql_query_string)), "AND mailbox_name = '%s' ", mailbox_name);
	}
	
	if (uid) {		/*  NULL means all mail */
		SNPRINTF(sql_query_string+strlen(sql_query_string), sizeof(sql_query_string) - (1 + strlen(sql_query_string)), "AND s_uid='%s' ", uid);
	}
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
 
	
	ret = true;
	
FINISH_OFF:
 
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_free_read_mail_uid(emf_mail_read_mail_uid_tbl_t** read_mail_uid, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("read_mail_uid[%p], count[%d], err_code[%p]", read_mail_uid, count, err_code);
	
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	
	if (count > 0)  {
		if (!read_mail_uid || !*read_mail_uid)  {	
			EM_DEBUG_EXCEPTION(" read_mail_uid[%p], count[%d]", read_mail_uid, count);
			
			error = EM_STORAGE_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		emf_mail_read_mail_uid_tbl_t* p = *read_mail_uid;
		int i;
		
		for (i = 0; i < count; i++)  {
			EM_SAFE_FREE(p[i].local_mbox);
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

EXPORT_API int em_storage_get_rule_count(int account_id, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], count[%p], transaction[%d], err_code[%p]", account_id, count, transaction, err_code);
	
	if (account_id != ALL_ACCOUNT || !count) {		/*  only global rule supported. */
		EM_DEBUG_EXCEPTION(" account_id[%d], count[%p]", account_id, count);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error =  EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_rule_tbl WHERE account_id = %d", account_id);
	
	char **result;
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);
	
	ret = true;
	
FINISH_OFF:
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_rule(int account_id, int type, int start_idx, int *select_num, int *is_completed, emf_mail_rule_tbl_t** rule_list, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], type[%d], start_idx[%d], select_num[%p], is_completed[%p], rule_list[%p], transaction[%d], err_code[%p]", account_id, type, start_idx, select_num, is_completed, rule_list, transaction, err_code);
	
	if (account_id != ALL_ACCOUNT || !select_num || !is_completed || !rule_list) {		/*  only global rule supported. */
		EM_DEBUG_EXCEPTION(" account_id[%d], type[%d], start_idx[%d], select_num[%p], is_completed[%p], rule_list[%p]", account_id, type, start_idx, select_num, is_completed, rule_list);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	
	emf_mail_rule_tbl_t* p_data_tbl = NULL;
	int i = 0, count = 0;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	int rc;
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	if (type)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_rule_tbl WHERE account_id = %d AND type = %d", account_id, type);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_rule_tbl WHERE account_id = %d ORDER BY type", account_id);
		
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	char **result;
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	sqlite3_free_table(result);

	if (count == 0)  {
		EM_DEBUG_LOG(" no matched rule found...");
		ret = true;
		goto FINISH_OFF;
	}

	
	if (!(p_data_tbl = (emf_mail_rule_tbl_t*)malloc(sizeof(emf_mail_rule_tbl_t) * count))) {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emf_mail_rule_tbl_t) * count);
	
	for (i = 0; i < count; i++)  {
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].account_id), ACCOUNT_ID_IDX_IN_MAIL_RULE_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].rule_id), RULE_ID_IDX_IN_MAIL_RULE_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].type), TYPE_IDX_IN_MAIL_RULE_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].value), 0, VALUE_IDX_IN_MAIL_RULE_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].action_type), ACTION_TYPE_IDX_IN_MAIL_RULE_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].dest_mailbox), 0, DEST_MAILBOX_IDX_IN_MAIL_RULE_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].flag1), FLAG1_IDX_IN_MAIL_RULE_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].flag2), FLAG2_IDX_IN_MAIL_RULE_TBL);
		/*  rc = sqlite3_step(hStmt); */
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
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
		em_storage_free_rule(&p_data_tbl, count, NULL); /* CID FIX */
 	
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("  sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_rule_by_id(int account_id, int rule_id, emf_mail_rule_tbl_t** rule, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], rule_id[%d], rule[%p], transaction[%d], err_code[%p]", account_id, rule_id, rule, transaction, err_code);

	if (account_id != ALL_ACCOUNT || !rule)  {	
		EM_DEBUG_EXCEPTION(" account_id[%d], rule_id[%d], rule[%p]", account_id, rule_id, rule);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	emf_mail_rule_tbl_t* p_data_tbl = NULL;
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
 
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_rule_tbl WHERE account_id = %d AND rule_id = %d", account_id, rule_id);
	
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION(" no matched rule found...");
		error = EM_STORAGE_ERROR_RULE_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	if (!(p_data_tbl = (emf_mail_rule_tbl_t*)malloc(sizeof(emf_mail_rule_tbl_t))))  {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emf_mail_rule_tbl_t));
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->account_id), ACCOUNT_ID_IDX_IN_MAIL_RULE_TBL);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->rule_id), RULE_ID_IDX_IN_MAIL_RULE_TBL);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->type), TYPE_IDX_IN_MAIL_RULE_TBL);
	_getStmtFieldDataString(hStmt, &(p_data_tbl->value), 0, VALUE_IDX_IN_MAIL_RULE_TBL);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->action_type), ACTION_TYPE_IDX_IN_MAIL_RULE_TBL);
	_getStmtFieldDataString(hStmt, &(p_data_tbl->dest_mailbox), 0, DEST_MAILBOX_IDX_IN_MAIL_RULE_TBL);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->flag1), FLAG1_IDX_IN_MAIL_RULE_TBL);
	_getStmtFieldDataInt(hStmt, &(p_data_tbl->flag2), FLAG2_IDX_IN_MAIL_RULE_TBL);
	
	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*rule = p_data_tbl;
	else if (p_data_tbl != NULL)
		em_storage_free_rule(&p_data_tbl, 1, NULL);
	
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 
	
	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_change_rule(int account_id, int rule_id, emf_mail_rule_tbl_t* new_rule, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], rule_id[%d], new_rule[%p], transaction[%d], err_code[%p]", account_id, rule_id, new_rule, transaction, err_code);

	if (account_id != ALL_ACCOUNT || !new_rule) {		/*  only global rule supported. */
		EM_DEBUG_EXCEPTION(" account_id[%d], rule_id[%d], new_rule[%p]", account_id, rule_id, new_rule);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
 
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_rule_tbl SET"
		"  type = ?"
		", value = ?"
		", action_type = ?"
		", dest_mailbox = ?"
		", flag1 = ?"
		", flag2 = ?"
		" WHERE account_id = ?"
		" AND rule_id = ?");
 
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG(" Before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	int i = 0;
	
	_bindStmtFieldDataInt(hStmt, i++, new_rule->type);
	_bindStmtFieldDataString(hStmt, i++, (char *)new_rule->value, 0, VALUE_LEN_IN_MAIL_RULE_TBL);
	_bindStmtFieldDataInt(hStmt, i++, new_rule->action_type);
	_bindStmtFieldDataString(hStmt, i++, (char *)new_rule->dest_mailbox, 0, DEST_MAILBOX_LEN_IN_MAIL_RULE_TBL);
	_bindStmtFieldDataInt(hStmt, i++, new_rule->flag1);
	_bindStmtFieldDataInt(hStmt, i++, new_rule->flag2);
	_bindStmtFieldDataInt(hStmt, i++, account_id);
	_bindStmtFieldDataInt(hStmt, i++, rule_id);
 
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_find_rule(emf_mail_rule_tbl_t* rule, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("rule[%p], transaction[%d], err_code[%p]", rule, transaction, err_code);

	if (!rule || rule->account_id != ALL_ACCOUNT) {		/*  only global rule supported. */
		if (rule != NULL)
			EM_DEBUG_EXCEPTION(" rule->account_id[%d]", rule->account_id);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	int error = EM_STORAGE_ERROR_NONE;
	int rc, ret = false;

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"SELECT rule_id FROM mail_rule_tbl WHERE type = %d AND UPPER(value) = UPPER('%s')",
		rule->type, rule->value);
 
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION(" no matched rule found...");
		error = EM_STORAGE_ERROR_RULE_NOT_FOUND;
		goto FINISH_OFF;
	}

	
	ret = true;

FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 
	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;

	if (err_code)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_add_rule(emf_mail_rule_tbl_t* rule, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("rule[%p], transaction[%d], err_code[%p]", rule, transaction, err_code);

	if (!rule || rule->account_id != ALL_ACCOUNT)  {	/*  only global rule supported. */
		if (rule != NULL)
			EM_DEBUG_EXCEPTION(" rule->account_id[%d]", rule->account_id);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, rc_2, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
 	char sql_query_string[QUERY_SIZE] = {0, };

	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	char *sql;
	char **result;
	sql = "SELECT max(rowid) FROM mail_rule_tbl;";
	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);  */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
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
		", ?"		/*  dest_mailbox */
		", ?"		/*  flag1 */
		", ?)");	/*  flag2 */
 
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc_2);
	if (rc_2 != SQLITE_OK)  {
		EM_DEBUG_EXCEPTION("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc_2, sqlite3_errmsg(local_db_handle));
		error = EM_STORAGE_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}
 
	_bindStmtFieldDataInt(hStmt, ACCOUNT_ID_IDX_IN_MAIL_RULE_TBL, rule->account_id);
	_bindStmtFieldDataInt(hStmt, RULE_ID_IDX_IN_MAIL_RULE_TBL, rc);
	_bindStmtFieldDataInt(hStmt, TYPE_IDX_IN_MAIL_RULE_TBL, rule->type);
	_bindStmtFieldDataString(hStmt, VALUE_IDX_IN_MAIL_RULE_TBL, (char *)rule->value, 0, VALUE_LEN_IN_MAIL_RULE_TBL);
	_bindStmtFieldDataInt(hStmt, ACTION_TYPE_IDX_IN_MAIL_RULE_TBL, rule->action_type);
	_bindStmtFieldDataString(hStmt, DEST_MAILBOX_IDX_IN_MAIL_RULE_TBL, (char *)rule->dest_mailbox, 0, DEST_MAILBOX_LEN_IN_MAIL_RULE_TBL);
	_bindStmtFieldDataInt(hStmt, FLAG1_IDX_IN_MAIL_RULE_TBL, rule->flag1);
	_bindStmtFieldDataInt(hStmt, FLAG2_IDX_IN_MAIL_RULE_TBL, rule->flag2);
 
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_delete_rule(int account_id, int rule_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], rule_id[%d], transaction[%d], err_code[%p]", account_id, rule_id, transaction, err_code);
	
	if (account_id != ALL_ACCOUNT) {		/*  only global rule supported. */
		EM_DEBUG_EXCEPTION(" account_id[%d], rule_id[%d]", account_id, rule_id);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_rule_tbl WHERE account_id = %d AND rule_id = %d", account_id, rule_id);
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION(" no matched rule found...");
		
		error = EM_STORAGE_ERROR_RULE_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_free_rule(emf_mail_rule_tbl_t** rule_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("rule_list[%p], conut[%d], err_code[%p]", rule_list, count, err_code);

	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	
	if (count > 0) {
		if (!rule_list || !*rule_list) {
			EM_DEBUG_EXCEPTION(" rule_list[%p], conut[%d]", rule_list, count);
			
			error = EM_STORAGE_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		emf_mail_rule_tbl_t* p = *rule_list;
		int i = 0;
		
		for (; i < count; i++) {
			EM_SAFE_FREE(p[i].value);
			EM_SAFE_FREE(p[i].dest_mailbox);
		}
		
		EM_SAFE_FREE(p); *rule_list = NULL;
	}
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_mail_count(int account_id, const char *mailbox_name, int *total, int *unseen, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%s], total[%p], unseen[%p], transaction[%d], err_code[%p]", account_id, mailbox_name, total, unseen, transaction, err_code);
	
	if (!total && !unseen) {		/* TODO: confirm me */
		EM_DEBUG_EXCEPTION(" accoun_id[%d], mailbox_name[%s], total[%p], unseen[%p]", account_id, mailbox_name, total, unseen);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char err_msg[1024];

	
	memset(&sql_query_string, 0x00, sizeof(sql_query_string));
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
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
		/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL); */
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF2; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		*total = atoi(result[1]);
		sqlite3_free_table(result);
#else
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
		EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF2; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		/*  rc = sqlite3_step(hStmt); */
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF2; },
			("sqlite3_step fail:%d", rc));
		_getStmtFieldDataInt(hStmt, total, 0);
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
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
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
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
#endif

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_mail_field_by_id(int mail_id, int type, emf_mail_tbl_t** mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], type[%d], mail[%p], transaction[%d], err_code[%p]", mail_id, type, mail, transaction, err_code);
	
	if (mail_id <= 0 || !mail)  {	
		EM_DEBUG_EXCEPTION("mail_id[%d], mail[%p]", mail_id, mail);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	emf_mail_tbl_t* p_data_tbl = (emf_mail_tbl_t*)malloc(sizeof(emf_mail_tbl_t));

	if (p_data_tbl == NULL)  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		return false;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emf_mail_tbl_t));
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	switch (type)  {
		case RETRIEVE_SUMMARY:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"SELECT account_id, mail_id, mailbox_name, server_mail_status, server_mailbox_name, server_mail_id, file_path_plain, file_path_html, flags_seen_field, save_status, lock_status, thread_id, thread_item_count FROM mail_tbl WHERE mail_id = %d", mail_id);
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
				"SELECT account_id, mailbox_name, flags_seen_field, thread_id FROM mail_tbl WHERE mail_id = %d", mail_id);
			break;
		
		default :
			EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM : type [%d]", type);
			error = EM_STORAGE_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}
	
	EM_DEBUG_LOG("Query [%s]", sql_query_string);
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no matched mail found...");
		error = EM_STORAGE_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
	switch (type)  {
		case RETRIEVE_SUMMARY:
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->account_id), 0);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->mail_id), 1);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->mailbox_name), 0, 2);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->server_mail_status), 3);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->server_mailbox_name), 0, 4);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->server_mail_id), 0, 5);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->file_path_plain), 0, 6);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->file_path_html), 0, 7);
			_getStmtFieldDataChar(hStmt, &(p_data_tbl->flags_seen_field), 8);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->save_status), 9);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->lock_status), 10);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->thread_id), 11);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->thread_item_count), 12);
			break;
			
		case RETRIEVE_FIELDS_FOR_DELETE:
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->account_id), 0);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->mail_id), 1);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->server_mail_status), 2);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->server_mailbox_name), 0, 3);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->server_mail_id), 0, 4);
			break;
			
		case RETRIEVE_ACCOUNT:
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->account_id), 0);
			break;
			
		case RETRIEVE_FLAG:
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->account_id), 0);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->mailbox_name), 0, 1);
			_getStmtFieldDataChar(hStmt, &(p_data_tbl->flags_seen_field), 2);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->thread_id), 3);
			break;
	}
	
	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*mail = p_data_tbl;
	else if (p_data_tbl != NULL)
		em_storage_free_mail(&p_data_tbl,  1, NULL);

	if (hStmt != NULL)  {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);
	
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 	

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_mail_field_by_multiple_mail_id(int mail_ids[], int number_of_mails, int type, emf_mail_tbl_t** mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], number_of_mails [%d], type[%d], mail[%p], transaction[%d], err_code[%p]", mail_ids, number_of_mails, type, mail, transaction, err_code);
	
	if (number_of_mails <= 0 || !mail_ids)  {	
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	emf_mail_tbl_t* p_data_tbl = (emf_mail_tbl_t*)em_core_malloc(sizeof(emf_mail_tbl_t) * number_of_mails);

	if (p_data_tbl == NULL)  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		return false;
	}
	
	DB_STMT hStmt = NULL;
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	int i = 0, item_count = 0, rc = -1, field_count, col_index, cur_sql_query_string = 0;
	char **result = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);

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
				"SELECT account_id, mailbox_name, flags_seen_field, thread_id FROM mail_tbl WHERE mail_id in (");
			field_count = 4;
			break;
		
		default :
			EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM : type [%d]", type);
			error = EM_STORAGE_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}

	for(i = 0; i < number_of_mails; i++) 		
		cur_sql_query_string += SNPRINTF_OFFSET(sql_query_string, cur_sql_query_string, QUERY_SIZE, "%d,", mail_ids[i]);
	sql_query_string[strlen(sql_query_string) - 1] = ')';
	
	EM_DEBUG_LOG("Query [%s], Length [%d]", sql_query_string, strlen(sql_query_string));

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &item_count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no matched mail found...");
		error = EM_STORAGE_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("item_count [%d]", item_count);

	if(number_of_mails != item_count) {
		EM_DEBUG_EXCEPTION("Can't find all emails");
		/* error = EM_STORAGE_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF; */ /* removed temporarily */
	}

	col_index = field_count;

	for(i = 0; i < item_count; i++)	{
		switch (type) {
			case RETRIEVE_SUMMARY:
				_getTableFieldDataInt(result, &(p_data_tbl[i].account_id), col_index++);
				_getTableFieldDataInt(result, &(p_data_tbl[i].mail_id), col_index++);
				_getTableFieldDataString(result, &(p_data_tbl[i].mailbox_name), 0, col_index++);
				_getTableFieldDataInt(result, &(p_data_tbl[i].server_mail_status), col_index++);
				_getTableFieldDataString(result, &(p_data_tbl[i].server_mailbox_name), 0, col_index++);
				_getTableFieldDataString(result, &(p_data_tbl[i].server_mail_id), 0, col_index++);
				_getTableFieldDataString(result, &(p_data_tbl[i].file_path_plain), 0, col_index++);
				_getTableFieldDataString(result, &(p_data_tbl[i].file_path_html), 0, col_index++);
				_getTableFieldDataChar(result, &(p_data_tbl[i].flags_seen_field), col_index++);
				_getTableFieldDataInt(result, &(p_data_tbl[i].save_status), col_index++);
				_getTableFieldDataInt(result, &(p_data_tbl[i].lock_status), col_index++);
				_getTableFieldDataInt(result, &(p_data_tbl[i].thread_id), col_index++);
				_getTableFieldDataInt(result, &(p_data_tbl[i].thread_item_count), col_index++);
				break;
				
			case RETRIEVE_FIELDS_FOR_DELETE:
				_getTableFieldDataInt(result, &(p_data_tbl[i].account_id), col_index++);
				_getTableFieldDataInt(result, &(p_data_tbl[i].mail_id), col_index++);
				_getTableFieldDataInt(result, &(p_data_tbl[i].server_mail_status), col_index++);
				_getTableFieldDataString(result, &(p_data_tbl[i].server_mailbox_name), 0, col_index++);
				_getTableFieldDataString(result, &(p_data_tbl[i].server_mail_id), 0, col_index++);
				break;
				
			case RETRIEVE_ACCOUNT:
				_getTableFieldDataInt(result, &(p_data_tbl[i].account_id), col_index++);
				break;
				
			case RETRIEVE_FLAG:
				_getTableFieldDataInt(result, &(p_data_tbl[i].account_id), col_index++);
				_getTableFieldDataString(result, &(p_data_tbl[i].mailbox_name), 0, col_index++);
				_getTableFieldDataChar(result, &(p_data_tbl[i].flags_seen_field), col_index++);
				_getTableFieldDataInt(result, &(p_data_tbl[i].thread_id), col_index++);
				break;
		}
	}

	sqlite3_free_table(result);

	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*mail = p_data_tbl;
	else if (p_data_tbl != NULL)
		em_storage_free_mail(&p_data_tbl,  1, NULL);

	if (hStmt != NULL) {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);
	
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_mail_by_id(int mail_id, emf_mail_tbl_t** mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], mail[%p], transaction[%d], err_code[%p]", mail_id, mail, transaction, err_code);
	
	if (mail_id <= 0 || !mail) {	
		EM_DEBUG_EXCEPTION("mail_id[%d], mail[%p]", mail_id, mail);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false, error = EM_STORAGE_ERROR_NONE, count;
	char conditional_clause[QUERY_SIZE] = {0, };
	emf_mail_tbl_t* p_data_tbl = NULL;
	
	SNPRINTF(conditional_clause, QUERY_SIZE, "WHERE mail_id = %d", mail_id);
	EM_DEBUG_LOG("query = [%s]", conditional_clause);

	if(!em_storage_query_mail_tbl(conditional_clause, transaction, &p_data_tbl, &count, &error)) {
		EM_DEBUG_EXCEPTION("em_storage_query_mail_tbl [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:
	if (ret == true)
		*mail = p_data_tbl;
	else if (p_data_tbl != NULL)
		em_storage_free_mail(&p_data_tbl, 1, &error);
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_mail_search_start(emf_mail_search_t* search, int account_id, char *mailbox_name, int sorting, int *search_handle, int *searched, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("search[%p], account_id[%d], mailbox_name[%p], sorting[%d], search_handle[%p], searched[%p], transaction[%d], err_code[%p]", search, account_id, mailbox_name, sorting, search_handle, searched, transaction, err_code);

	if (!search_handle || !searched)  {	
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		EM_DEBUG_FUNC_END("false");
		return false;
	}
	
	emf_mail_search_t* p = search;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	int rc, ret = false;
	int and = false, mail_count = 0;
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
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

	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	char **result;
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &mail_count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
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
				error = EM_STORAGE_ERROR_DB_FAILURE;
			}
		}
		
		EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
		_DISCONNECT_DB;
	}
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_mail_search_result(int search_handle, emf_mail_field_type_t type, void** data, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("search_handle[%d], type[%d], data[%p], transaction[%d], err_code[%p]", search_handle, type, data, transaction, err_code);

	if (search_handle < 0 || !data) {
		EM_DEBUG_EXCEPTION(" search_handle[%d], type[%d], data[%p]", search_handle, type, data);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	emf_mail_tbl_t* p_data_tbl = NULL;
	DB_STMT hStmt = (DB_STMT)search_handle;
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;

	switch (type)  {
		case RETRIEVE_ID:
			_getStmtFieldDataInt(hStmt, (int *)data, MAIL_ID_IDX_IN_MAIL_TBL);
			break;
			
		case RETRIEVE_ENVELOPE:
		case RETRIEVE_ALL:
			if (!(p_data_tbl = em_core_malloc(sizeof(emf_mail_tbl_t)))) {
				EM_DEBUG_EXCEPTION(" em_core_malloc failed...");
				
				error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->mail_id), MAIL_ID_IDX_IN_MAIL_TBL);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->account_id), ACCOUNT_ID_IDX_IN_MAIL_TBL);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_TBL);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->mail_size), MAIL_SIZE_IDX_IN_MAIL_TBL);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->server_mail_id), 0, SERVER_MAIL_ID_IDX_IN_MAIL_TBL);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->full_address_from), 1, FULL_ADDRESS_FROM_IDX_IN_MAIL_TBL);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->full_address_to), 1, FULL_ADDRESS_TO_IDX_IN_MAIL_TBL);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->subject), 1, SUBJECT_IDX_IN_MAIL_TBL);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->body_download_status), BODY_DOWNLOAD_STATUS_IDX_IN_MAIL_TBL);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->file_path_plain), 0, FILE_PATH_PLAIN_IDX_IN_MAIL_TBL);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->file_path_html), 0, FILE_PATH_HTML_IDX_IN_MAIL_TBL);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->datetime), 0, DATETIME_IDX_IN_MAIL_TBL);
			_getStmtFieldDataChar(hStmt, &(p_data_tbl->flags_seen_field), FLAGS_SEEN_FIELD_IDX_IN_MAIL_TBL);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->DRM_status), DRM_STATUS_IDX_IN_MAIL_TBL);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->priority), PRIORITY_IDX_IN_MAIL_TBL);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->save_status), SAVE_STATUS_IDX_IN_MAIL_TBL);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->lock_status), LOCK_STATUS_IDX_IN_MAIL_TBL);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->report_status), REPORT_STATUS_IDX_IN_MAIL_TBL);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->preview_text), 1, PREVIEW_TEXT_IDX_IN_MAIL_TBL);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->meeting_request_status), MEETING_REQUEST_STATUS_IDX_IN_MAIL_TBL);
				
			if (type == RETRIEVE_ALL)  {
				_getStmtFieldDataInt(hStmt, &(p_data_tbl->server_mail_status), SERVER_MAIL_STATUS_IDX_IN_MAIL_TBL);
				_getStmtFieldDataString(hStmt, &(p_data_tbl->server_mailbox_name), 0, SERVER_MAILBOX_NAME_IDX_IN_MAIL_TBL);
				_getStmtFieldDataString(hStmt, &(p_data_tbl->full_address_reply), 1, FULL_ADDRESS_REPLY_IDX_IN_MAIL_TBL);
				_getStmtFieldDataString(hStmt, &(p_data_tbl->full_address_cc), 1, FULL_ADDRESS_CC_IDX_IN_MAIL_TBL);
				_getStmtFieldDataString(hStmt, &(p_data_tbl->full_address_bcc), 1, FULL_ADDRESS_BCC_IDX_IN_MAIL_TBL);
				_getStmtFieldDataString(hStmt, &(p_data_tbl->full_address_return), 1, FULL_ADDRESS_RETURN_IDX_IN_MAIL_TBL);
				_getStmtFieldDataString(hStmt, &(p_data_tbl->message_id), 0, MESSAGE_ID_IDX_IN_MAIL_TBL);
				_getStmtFieldDataString(hStmt, &(p_data_tbl->email_address_sender), 1, EMAIL_ADDRESS_SENDER_IDX_IN_MAIL_TBL);
				_getStmtFieldDataString(hStmt, &(p_data_tbl->email_address_recipient), 1, EMAIL_ADDRESS_RECIPIENT_IDX_IN_MAIL_TBL);
				_getStmtFieldDataInt(hStmt, &(p_data_tbl->attachment_count), ATTACHMENT_COUNT_IDX_IN_MAIL_TBL);
			   _getStmtFieldDataString(hStmt, &(p_data_tbl->preview_text), 1, PREVIEW_TEXT_IDX_IN_MAIL_TBL);
				
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
			
			*((emf_mail_tbl_t**)data) = p_data_tbl;
			break;
		
		case RETRIEVE_SUMMARY:
			if (!(p_data_tbl = malloc(sizeof(emf_mail_tbl_t))))  {
				EM_DEBUG_EXCEPTION(" malloc failed...");
				
				error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			memset(p_data_tbl, 0x00, sizeof(emf_mail_tbl_t));
			
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->mail_id), MAIL_ID_IDX_IN_MAIL_TBL);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->account_id), ACCOUNT_ID_IDX_IN_MAIL_TBL);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_TBL);
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->server_mail_status), SERVER_MAIL_STATUS_IDX_IN_MAIL_TBL);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->server_mailbox_name), 0, SERVER_MAILBOX_NAME_IDX_IN_MAIL_TBL);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->server_mail_id), 0, SERVER_MAIL_ID_IDX_IN_MAIL_TBL);
			
			*((emf_mail_tbl_t**)data) = p_data_tbl;
			break;
		
		case RETRIEVE_ADDRESS:
			if (!(p_data_tbl = malloc(sizeof(emf_mail_tbl_t))))  {
				EM_DEBUG_EXCEPTION(" malloc failed...");
				error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			memset(p_data_tbl, 0x00, sizeof(emf_mail_tbl_t));
			_getStmtFieldDataInt(hStmt, &(p_data_tbl->mail_id), MAIL_ID_IDX_IN_MAIL_TBL);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->email_address_sender), 1, EMAIL_ADDRESS_SENDER_IDX_IN_MAIL_TBL);
			_getStmtFieldDataString(hStmt, &(p_data_tbl->email_address_recipient), 1, EMAIL_ADDRESS_RECIPIENT_IDX_IN_MAIL_TBL);
			*((emf_mail_tbl_t**)data) = p_data_tbl;
			break;
		
		default:
			break;
	}
	
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	ret = true;
	
FINISH_OFF:
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_mail_search_end(int search_handle, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("search_handle[%d], transaction[%d], err_code[%p]", search_handle, transaction, err_code);
	
	int error = EM_STORAGE_ERROR_NONE;
	int rc, ret = false;
	
	if (search_handle < 0)  {
		EM_DEBUG_EXCEPTION(" search_handle[%d]", search_handle);
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	DB_STMT hStmt = (DB_STMT)search_handle;
	
	EM_DEBUG_LOG(" Before sqlite3_finalize hStmt = %p", hStmt);

	rc = sqlite3_finalize(hStmt);
	if (rc != SQLITE_OK)  {
		EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
		error = EM_STORAGE_ERROR_DB_FAILURE;
	}
 
	ret = true;
	
FINISH_OFF:
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_change_mail(int mail_id, emf_mail_tbl_t* mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], mail[%p], transaction[%d], err_code[%p]", mail_id, mail, transaction, err_code);

	if (mail_id <= 0 || !mail) {
		EM_DEBUG_EXCEPTION(" mail_id[%d], mail[%p]", mail_id, mail);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	int rc = -1;				
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_tbl SET"
		"  mail_id = ?"
		", account_id = ?"
		", mailbox_name = ?"
		", mail_size = ?"
		", server_mail_status = ?"
		", server_mailbox_name = ?"
		", server_mail_id = ?"
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
		" WHERE mail_id = %d AND account_id != 0 "
		, mail_id);

	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	
	int i = 0;
	
	_bindStmtFieldDataInt(hStmt, i++, mail->mail_id);
	_bindStmtFieldDataInt(hStmt, i++, mail->account_id);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->mailbox_name, 0, MAILBOX_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataInt(hStmt, i++, mail->mail_size);
	_bindStmtFieldDataInt(hStmt, i++, mail->server_mail_status);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->server_mailbox_name, 0, SERVER_MAILBOX_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->server_mail_id, 0, SERVER_MAIL_ID_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->full_address_from, 1, FROM_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->full_address_reply, 1, REPLY_TO_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->full_address_to, 1, TO_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->full_address_cc, 1, CC_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->full_address_bcc, 1, BCC_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->full_address_return, 1, RETURN_PATH_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->subject, 1, SUBJECT_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataInt(hStmt, i++, mail->body_download_status);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->file_path_html, 0, TEXT_2_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->datetime, 0, DATETIME_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataChar(hStmt, i++, mail->flags_seen_field);
	_bindStmtFieldDataChar(hStmt, i++, mail->flags_deleted_field);
	_bindStmtFieldDataChar(hStmt, i++, mail->flags_flagged_field);
	_bindStmtFieldDataChar(hStmt, i++, mail->flags_answered_field);
	_bindStmtFieldDataChar(hStmt, i++, mail->flags_recent_field);
	_bindStmtFieldDataChar(hStmt, i++, mail->flags_draft_field);
	_bindStmtFieldDataChar(hStmt, i++, mail->flags_forwarded_field);
	_bindStmtFieldDataInt(hStmt, i++, mail->DRM_status);
	_bindStmtFieldDataInt(hStmt, i++, mail->priority);
	_bindStmtFieldDataInt(hStmt, i++, mail->save_status);
	_bindStmtFieldDataInt(hStmt, i++, mail->lock_status);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->message_id, 0, MESSAGE_ID_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataInt(hStmt, i++, mail->report_status);
	_bindStmtFieldDataString(hStmt, i++, (char *)mail->preview_text, 1, PREVIEWBODY_LEN_IN_MAIL_TBL);
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no matched mail found...");
		error = EM_STORAGE_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (!em_storage_notify_storage_event(NOTI_MAIL_UPDATE, mail->account_id, mail->mail_id, mail->mailbox_name, 0))
		EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event Failed [ NOTI_MAIL_UPDATE ] >>>> ");
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_modify_mailbox_of_mails(char *old_mailbox_name, char *new_mailbox_name, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("old_mailbox_name[%p], new_mailbox_name[%p], transaction[%d], err_code[%p]", old_mailbox_name, new_mailbox_name, transaction, err_code);

	if (!old_mailbox_name && !new_mailbox_name)  {
		EM_DEBUG_EXCEPTION(" old_mailbox_name[%p], new_mailbox_name[%p]", old_mailbox_name, new_mailbox_name);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1;
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);

	EM_DEBUG_LOG("Old Mailbox Name  [ %s ] , New Mailbox name [ %s ] ", old_mailbox_name, new_mailbox_name);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET mailbox_name = '%s' WHERE mailbox_name = '%s'", new_mailbox_name, old_mailbox_name);
 
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION(" no matched mail found...");
		
		error = EM_STORAGE_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
 
	EM_DEBUG_LOG(" Modification done in mail_read_mail_uid_tbl based on Mailbox name ");
	/* Modify the mailbox_name name in mail_read_mail_uid_tbl table */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_read_mail_uid_tbl SET mailbox_name = '%s' WHERE mailbox_name = '%s'", new_mailbox_name, old_mailbox_name);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no matched mail found...");
		error = EM_STORAGE_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	if (!em_storage_notify_storage_event(NOTI_MAILBOX_UPDATE, 1, 0, new_mailbox_name, 0))
		EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event[ NOTI_MAILBOX_UPDATE] : Notification Failed >>> ");
	
	ret = true;
	
FINISH_OFF:

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/**
  *  em_storage_clean_save_status(int save_status, int  *err_code) - set the all mail status to the set value
  *
  *
  **/
EXPORT_API int em_storage_clean_save_status(int save_status, int  *err_code)
{
	EM_DEBUG_FUNC_BEGIN("save_status[%d], err_code[%p]", save_status, err_code);

	EM_IF_NULL_RETURN_VALUE(err_code, false);

	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	int rc = 0;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET save_status = %d WHERE save_status = %d", save_status, EMF_MAIL_STATUS_SENDING);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_LOG(" No Matched Mail Exists ");
		error = EM_STORAGE_ERROR_MAIL_NOT_FOUND;
	}
	
	ret = true;

FINISH_OFF:
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_set_field_of_mails_with_integer_value(int account_id, int mail_ids[], int mail_ids_count, char *field_name, int value, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mail_ids[%p], mail_ids_count[%d], field_name[%s], value[%d], transaction[%d], err_code[%p]", account_id, mail_ids, mail_ids_count, field_name, value, transaction, err_code);
	int i, error, rc, ret = false, cur_mail_id_string = 0, mail_id_string_buffer_length = 0;
	char sql_query_string[QUERY_SIZE] = {0, }, *mail_id_string_buffer = NULL, *parameter_string = NULL;
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	
	if (!mail_ids  || !field_name || account_id == 0) {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	/* Generating mail id list string */
	mail_id_string_buffer_length = MAIL_ID_STRING_LENGTH * mail_ids_count;

	mail_id_string_buffer = em_core_malloc(mail_id_string_buffer_length);

	if(!mail_id_string_buffer) {
		EM_DEBUG_EXCEPTION("em_core_malloc failed");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	for(i = 0; i < mail_ids_count; i++) 
		cur_mail_id_string += SNPRINTF_OFFSET(mail_id_string_buffer, cur_mail_id_string, mail_id_string_buffer_length, "%d,", mail_ids[i]);

	if(strlen(mail_id_string_buffer) > 1)
		mail_id_string_buffer[strlen(mail_id_string_buffer) - 1] = NULL_CHAR;

	/* Generating notification parameter string */
	parameter_string = em_core_malloc(mail_id_string_buffer_length + strlen(field_name) + 2);

	if(!parameter_string) {
		EM_DEBUG_EXCEPTION("em_core_malloc failed");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	SNPRINTF(parameter_string, QUERY_SIZE, "%s%c%s", field_name, 0x01, mail_id_string_buffer);

	/* Write query string */
	SNPRINTF(sql_query_string, QUERY_SIZE, "UPDATE mail_tbl SET %s = %d WHERE mail_id in (%s) AND account_id = %d", field_name, value, mail_id_string_buffer, account_id);

	/* Execute query */
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	if (sqlite3_changes(local_db_handle) == 0) 
		EM_DEBUG_LOG("no mail matched...");

	ret = true;

FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (ret && parameter_string && !em_storage_notify_storage_event(NOTI_MAIL_FIELD_UPDATE, account_id, 0, parameter_string, value))
			EM_DEBUG_EXCEPTION("em_storage_notify_storage_event failed : NOTI_MAIL_FIELD_UPDATE [%s,%d]", field_name, value);
	
	EM_SAFE_FREE(mail_id_string_buffer);
	EM_SAFE_FREE(parameter_string);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_change_mail_field(int mail_id, emf_mail_change_type_t type, emf_mail_tbl_t* mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], type[%d], mail[%p], transaction[%d], err_code[%p]", mail_id, type, mail, transaction, err_code);
	
	if (mail_id <= 0 || !mail)  {
		EM_DEBUG_EXCEPTION(" mail_id[%d], type[%d], mail[%p]", mail_id, type, mail);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	int move_flag = 0;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	int rc = 0;
	int i = 0;

	char *mailbox_name = NULL;	

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
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
				" WHERE mail_id = %d AND account_id != 0"
				, mail_id);

			
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			 i = 0;
			
			_bindStmtFieldDataInt(hStmt, i++, mail->body_download_status);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->file_path_html, 0, TEXT_2_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataInt(hStmt, i++, mail->flags_seen_field);
			_bindStmtFieldDataInt(hStmt, i++, mail->flags_deleted_field);
			_bindStmtFieldDataInt(hStmt, i++, mail->flags_deleted_field);
			_bindStmtFieldDataInt(hStmt, i++, mail->flags_flagged_field);
			_bindStmtFieldDataInt(hStmt, i++, mail->flags_answered_field);
			_bindStmtFieldDataInt(hStmt, i++, mail->flags_recent_field);
			_bindStmtFieldDataInt(hStmt, i++, mail->flags_draft_field);
			_bindStmtFieldDataInt(hStmt, i++, mail->flags_forwarded_field);
			_bindStmtFieldDataInt(hStmt, i++, mail->DRM_status);
			_bindStmtFieldDataInt(hStmt, i++, mail->attachment_count);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->preview_text, 0, PREVIEWBODY_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataInt(hStmt, i++, mail->meeting_request_status);
			break;
			
		case UPDATE_MAILBOX: {
				int err;
				emf_mailbox_tbl_t *mailbox_name;
			
				if (!em_storage_get_mailbox_by_name(mail->account_id, -1, mail->mailbox_name, &mailbox_name, false, &err)) {
					EM_DEBUG_EXCEPTION(" em_storage_get_mailbox_by_name failed - %d", err);
					err = em_storage_get_emf_error_from_em_storage_error(err);
					goto FINISH_OFF;
				}

				SNPRINTF(sql_query_string, sizeof(sql_query_string),
					"UPDATE mail_tbl SET"
					" mailbox_name = '%s'"
					",mailbox_type = '%d'"
					" WHERE mail_id = %d AND account_id != 0"
					, mail->mailbox_name ? mail->mailbox_name : ""
					, mailbox_name->mailbox_type
					, mail_id);
					move_flag = 1;


				EM_DEBUG_LOG("Query [%s]", sql_query_string);
				
				EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
				EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
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
 
			
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
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
 
			
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
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
 
			
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			break;
			
		case UPDATE_MAIL:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				" full_address_from = ?"
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
				", mail_size = ?"
				", preview_text = ?"
				", body_download_status = ?"
				", attachment_count = ?"
				", inline_content_count = ?"
				", meeting_request_status = ?"
				" WHERE mail_id = %d AND account_id != 0"
				, mail_id);
 
			
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_LOG(" before sqlite3_prepare hStmt = %p", hStmt);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			i = 0;	
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->full_address_from, 1, FROM_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->full_address_reply, 1, REPLY_TO_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->full_address_to, 1, TO_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->full_address_cc, 1, CC_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->full_address_bcc, 1, BCC_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->full_address_return, 1, RETURN_PATH_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->subject, 1, SUBJECT_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->datetime, 0, DATETIME_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataChar(hStmt, i++, mail->flags_seen_field);
			_bindStmtFieldDataChar(hStmt, i++, mail->flags_deleted_field);
			_bindStmtFieldDataChar(hStmt, i++, mail->flags_flagged_field);
			_bindStmtFieldDataChar(hStmt, i++, mail->flags_answered_field);
			_bindStmtFieldDataChar(hStmt, i++, mail->flags_recent_field);
			_bindStmtFieldDataChar(hStmt, i++, mail->flags_draft_field);
			_bindStmtFieldDataChar(hStmt, i++, mail->flags_forwarded_field);
			_bindStmtFieldDataInt(hStmt, i++, mail->priority);
			_bindStmtFieldDataInt(hStmt, i++, mail->save_status);
			_bindStmtFieldDataInt(hStmt, i++, mail->lock_status);
			_bindStmtFieldDataInt(hStmt, i++, mail->report_status);
			_bindStmtFieldDataInt(hStmt, i++, mail->DRM_status);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->file_path_html, 0, TEXT_2_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataInt(hStmt, i++, mail->mail_size);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->preview_text, 1, PREVIEWBODY_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataInt(hStmt, i++, mail->body_download_status);
			_bindStmtFieldDataInt(hStmt, i++, mail->attachment_count);
			_bindStmtFieldDataInt(hStmt, i++, mail->inline_content_count);
			_bindStmtFieldDataInt(hStmt, i++, mail->meeting_request_status);
			break;
			
		case UPDATE_DATETIME:  {
			static char buf[16];
			time_t t = time(NULL);
			
			struct tm *p_tm = localtime(&t);
			if (!p_tm)  {
				EM_DEBUG_EXCEPTION(" localtime failed...");
				error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
			
			SNPRINTF(buf, sizeof(buf),
				"%04d%02d%02d%02d%02d%02d",
				p_tm->tm_year + 1900,
				p_tm->tm_mon + 1,
				p_tm->tm_mday,
				p_tm->tm_hour,
				p_tm->tm_min,
				p_tm->tm_sec);
			
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				" date_time = '%s'"
				" WHERE mail_id = %d AND account_id != 0"
				, buf
				, mail_id);
 
			
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			break;
		}
			
		case UPDATE_FROM_CONTACT_INFO:
			EM_DEBUG_LOG("NVARCHAR : em_storage_change_mail_field - mail change type is UPDATE_FROM_CONTACT_INFO");
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				" email_address_sender = ?,"
				" WHERE mail_id = %d",
				mail_id);
 
			hStmt = NULL;
			
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			i = 0;
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->email_address_sender, 1, FROM_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
			break;
			
		case UPDATE_TO_CONTACT_INFO:
			EM_DEBUG_LOG("NVARCHAR : em_storage_change_mail_field - mail change type is UPDATE_TO_CONTACT_INFO");
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				" email_address_recipient = ?,"
				" WHERE mail_id = %d",
				mail_id);
 
			hStmt = NULL;
			
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			i = 0;
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->email_address_recipient, 1, TO_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
			break;

			case UPDATE_ALL_CONTACT_INFO:
				EM_DEBUG_LOG("em_storage_change_mail_field - mail change type is UPDATE_ALL_CONTACT_INFO");
				SNPRINTF(sql_query_string, sizeof(sql_query_string),
					"UPDATE mail_tbl SET"
					" email_address_sender = ?,"
					" email_address_recipient = ?,"
					" WHERE mail_id = %d",
					mail_id);

				hStmt = NULL;
				
				EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
				EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
					("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
				i = 0;
				_bindStmtFieldDataString(hStmt, i++, (char *)mail->email_address_sender, 1, FROM_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
				_bindStmtFieldDataString(hStmt, i++, (char *)mail->email_address_recipient, 1, TO_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
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

			
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			 i = 0;
			
			_bindStmtFieldDataInt(hStmt, i++, mail->body_download_status);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->file_path_html,  0, TEXT_2_LEN_IN_MAIL_TBL);
			_bindStmtFieldDataInt(hStmt, i++, mail->attachment_count);
			_bindStmtFieldDataInt(hStmt, i++, mail->inline_content_count);
			_bindStmtFieldDataString(hStmt, i++, (char *)mail->preview_text,    0, PREVIEWBODY_LEN_IN_MAIL_TBL);
	
			break;

#endif
		
		default:
			EM_DEBUG_LOG(" type[%d]", type);
			
			error = EM_STORAGE_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}
 
	if (hStmt != NULL)  {
		/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
		rc = sqlite3_changes(local_db_handle);
		if (rc == 0)  {
			EM_DEBUG_EXCEPTION(" no matched mail found...");
	
			error = EM_STORAGE_ERROR_MAIL_NOT_FOUND;
			goto FINISH_OFF;
		}
	}	

	if (mail->account_id == 0) {
		emf_mail_tbl_t* mail_for_account = NULL;
		if (!em_storage_get_mail_field_by_id(mail_id, RETRIEVE_ACCOUNT, &mail_for_account, true, &error) || !mail_for_account) {
			EM_DEBUG_EXCEPTION(" em_storage_get_mail_field_by_id failed - %d", error);
/* 			error = em_storage_get_emf_error_from_em_storage_error(error); */
			goto FINISH_OFF;
		}
		mail->account_id = mail_for_account->account_id;
		if (mail_for_account) 
			em_storage_free_mail(&mail_for_account, 1, NULL);
	}

	ret = true;
	
FINISH_OFF:

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
		hStmt = NULL;
	}

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	/*h.gahlaut@samsung.com: Moving publication of notification after commiting transaction to DB */
	
	if (ret == true &&  move_flag != 1) {
		if (!em_storage_get_mailboxname_by_mailbox_type(mail->account_id, EMF_MAILBOX_TYPE_SENTBOX, &mailbox_name, false, &error))
			EM_DEBUG_EXCEPTION(" em_storage_get_mailboxname_by_mailbox_type failed - %d", error);

#ifdef __FEATURE_PROGRESS_IN_OUTBOX__

		if (mail->mailbox_name && mailbox_name) {
			if (strcmp(mail->mailbox_name, mailbox_name) != 0) {
				if (!em_storage_notify_storage_event(NOTI_MAIL_UPDATE, mail->account_id, mail_id, mail->mailbox_name, type))
					EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event Failed [ NOTI_MAIL_UPDATE ] >>>> ");
			}
		}
		else {
			/* h.gahlaut@samsung.com: Jan 10, 2011 Publishing noti to refresh outbox when email sending status changes */
			if (!em_storage_notify_storage_event(NOTI_MAIL_UPDATE, mail->account_id, mail_id, NULL, type))
				EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event Failed [ NOTI_MAIL_UPDATE ] ");
		}

#else

		if (mail->mailbox_name && strcmp(mail->mailbox_name, mailbox_name) != 0) {
			if (!em_storage_notify_storage_event(NOTI_MAIL_UPDATE, mail->account_id, mail_id, mail->mailbox_name, type))
				EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event Failed [ NOTI_MAIL_UPDATE ] >>>> ");
		}
	
		
#endif

	}

	EM_SAFE_FREE(mailbox_name);
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_get_mail_count_with_draft_flag(int account_id, const char *mailbox_name, int *total, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%s], total[%p], transaction[%d], err_code[%p]", account_id, mailbox_name, total, transaction, err_code);
	
	if (!total)  {
		EM_DEBUG_EXCEPTION("accoun_id[%d], mailbox_name[%s], total[%p]", account_id, mailbox_name, total);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	memset(&sql_query_string, 0x00, sizeof(sql_query_string));
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	if (total)  {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_tbl WHERE flags_draft_field = 1");
		
		if (account_id != ALL_ACCOUNT)
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " AND account_id = %d", account_id);
		
		if (mailbox_name)
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " AND mailbox_name = '%s'", mailbox_name);
		
#ifdef USE_GET_RECORD_COUNT_API
		EM_DEBUG_LOG("Query : [%s]", sql_query_string);
		char **result;
		/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL); */
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		*total = atoi(result[1]);
		sqlite3_free_table(result);
#else
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
		EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));

		_getStmtFieldDataInt(hStmt, total, 0);
#endif		/*  USE_GET_RECORD_COUNT_API */
	}
	ret = true;		

FINISH_OFF:
#ifndef USE_GET_RECORD_COUNT_API
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
#endif

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_mail_count_on_sending(int account_id, const char *mailbox_name, int *total, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%s], total[%p], transaction[%d], err_code[%p]", account_id, mailbox_name, total, transaction, err_code);
	
	if (!total)  {		
		EM_DEBUG_EXCEPTION(" accoun_id[%d], mailbox_name[%s], total[%p]", account_id, mailbox_name, total);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	memset(&sql_query_string, 0x00, sizeof(sql_query_string));
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	if (total)  {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_tbl WHERE save_status = %d ", EMF_MAIL_STATUS_SENDING);
		
		if (account_id != ALL_ACCOUNT)
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " AND account_id = %d", account_id);
		
		if (mailbox_name)
			SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " AND mailbox_name = '%s'", mailbox_name);
		
#ifdef USE_GET_RECORD_COUNT_API
		char **result;
		/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL); */
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		*total = atoi(result[1]);
		sqlite3_free_table(result);
#else
		
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
		EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));

		_getStmtFieldDataInt(hStmt, total, 0);
#endif		/*  USE_GET_RECORD_COUNT_API */
	}
	ret = true;		

FINISH_OFF:
#ifndef USE_GET_RECORD_COUNT_API
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
#endif

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}

EXPORT_API int em_storage_increase_mail_id(int *mail_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%p], transaction[%d], err_code[%p]", mail_id, transaction, err_code);
	
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	int latest_mail_id = 0;
	sqlite3 *local_db_handle = NULL;
	char *sql = "SELECT MAX(mail_id) FROM mail_tbl;";
	char **result = NULL;

#ifdef __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__
	em_storage_shm_mutex_timedlock(&mapped_for_generating_mail_id, 2);
#endif /* __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__ */

 	ret = vconf_get_int(VCONF_KEY_LATEST_MAIL_ID, &latest_mail_id);
	if (ret < 0 || latest_mail_id == 0) {
		EM_DEBUG_LOG("vconf_get_int() failed [%d] or latest_mail_id is zero", ret);

		local_db_handle = em_storage_get_db_connection();

		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
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
	em_storage_shm_mutex_unlock(&mapped_for_generating_mail_id);
#endif /* __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__ */

	ret = true;
	
FINISH_OFF:
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_add_mail(emf_mail_tbl_t* mail_tbl_data, int get_id, int transaction, int *err_code)
{
	EM_PROFILE_BEGIN(emStorageMailAdd);
	EM_DEBUG_FUNC_BEGIN("mail_tbl_data[%p], get_id[%d], transaction[%d], err_code[%p]", mail_tbl_data, get_id, transaction, err_code);
	
	if (!mail_tbl_data)  {
		EM_DEBUG_EXCEPTION("mail_tbl_data[%p], get_id[%d]", mail_tbl_data, get_id);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	static char bufDatetime[30] = {0, };
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	if (get_id)  {
		/*  increase unique id */
		char *sql = "SELECT max(rowid) FROM mail_tbl;";
		char **result;
		
		/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);  */
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

		if (NULL==result[1])
			rc = 1;
		else 
			rc = atoi(result[1])+1;
		sqlite3_free_table(result);
		mail_tbl_data->mail_id = rc;
		mail_tbl_data->thread_id = mail_tbl_data->mail_id;
	}

	if (!mail_tbl_data->datetime)  {
		time_t t = time(NULL);
		struct tm *p_tm = localtime(&t);
		
		if (p_tm)  {
			SNPRINTF(bufDatetime, sizeof(bufDatetime), "%04d%02d%02d%02d%02d%02d",
				p_tm->tm_year + 1900,
				p_tm->tm_mon + 1,
				p_tm->tm_mday,
				p_tm->tm_hour,
				p_tm->tm_min,
				p_tm->tm_sec);
		}
	}
	else if (strlen(mail_tbl_data->datetime) > 14)  {
		EM_DEBUG_EXCEPTION("WARNING: the given datatime is too big.");
		memcpy(bufDatetime, mail_tbl_data->datetime, 14);
		bufDatetime[14] = NULL_CHAR;
	}
	else
		strncpy(bufDatetime, mail_tbl_data->datetime, sizeof(bufDatetime)-1);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_tbl VALUES "
		"( ?" /*  mail_id */
		", ?" /*  account_id */
		", ?" /*  mailbox_name */
		", ?" /*  mailbox_type */
		", ?" /*  subject */
		", ?" /*  date_time */
		", ?" /*  server_mail_status */
		", ?" /*  server_mailbox_name */
		", ?" /*  server_mail_id */
		", ?" /*  message_id */
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
		", ?"	/*  preview_text */
		", ?"	/*  meeting_request_status */
		")");
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bindStmtFieldDataInt(hStmt, MAIL_ID_IDX_IN_MAIL_TBL, mail_tbl_data->mail_id);
	_bindStmtFieldDataInt(hStmt, ACCOUNT_ID_IDX_IN_MAIL_TBL, mail_tbl_data->account_id);
	_bindStmtFieldDataString(hStmt, MAILBOX_NAME_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->mailbox_name, 0, MAILBOX_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataInt(hStmt, MAILBOX_TYPE_IDX_IN_MAIL_TBL, mail_tbl_data->mailbox_type);
	_bindStmtFieldDataString(hStmt, SUBJECT_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->subject, 1, SUBJECT_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, DATETIME_IDX_IN_MAIL_TBL, (char *)bufDatetime, 0, DATETIME_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataInt(hStmt, SERVER_MAIL_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->server_mail_status);
	_bindStmtFieldDataString(hStmt, SERVER_MAILBOX_NAME_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->server_mailbox_name, 0, SERVER_MAILBOX_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, SERVER_MAIL_ID_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->server_mail_id, 0, SERVER_MAIL_ID_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, MESSAGE_ID_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->message_id, 0, MESSAGE_ID_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, FULL_ADDRESS_FROM_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->full_address_from, 1, FROM_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, FULL_ADDRESS_REPLY_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->full_address_reply, 1, REPLY_TO_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, FULL_ADDRESS_TO_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->full_address_to, 1, TO_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, FULL_ADDRESS_CC_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->full_address_cc, 1, CC_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, FULL_ADDRESS_BCC_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->full_address_bcc, 1, BCC_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, FULL_ADDRESS_RETURN_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->full_address_return, 1, RETURN_PATH_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, EMAIL_ADDRESS_SENDER_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->email_address_sender, 1, FROM_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, EMAIL_ADDRESS_RECIPIENT_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->email_address_recipient, 1, TO_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, ALIAS_SENDER_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->alias_sender, 1, FROM_CONTACT_NAME_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, ALIAS_RECIPIENT_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->alias_recipient, 1, FROM_CONTACT_NAME_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataInt(hStmt, BODY_DOWNLOAD_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->body_download_status);
	_bindStmtFieldDataString(hStmt, FILE_PATH_PLAIN_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, FILE_PATH_HTML_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->file_path_html, 0, TEXT_2_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataInt(hStmt, MAIL_SIZE_IDX_IN_MAIL_TBL, mail_tbl_data->mail_size);
	_bindStmtFieldDataInt(hStmt, FLAGS_SEEN_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_seen_field);
	_bindStmtFieldDataInt(hStmt, FLAGS_DELETED_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_deleted_field);
	_bindStmtFieldDataInt(hStmt, FLAGS_FLAGGED_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_flagged_field);
	_bindStmtFieldDataInt(hStmt, FLAGS_ANSWERED_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_answered_field);
	_bindStmtFieldDataInt(hStmt, FLAGS_RECENT_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_recent_field);
	_bindStmtFieldDataInt(hStmt, FLAGS_DRAFT_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_draft_field);
	_bindStmtFieldDataInt(hStmt, FLAGS_FORWARDED_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_forwarded_field);
	_bindStmtFieldDataInt(hStmt, DRM_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->DRM_status);
	_bindStmtFieldDataInt(hStmt, PRIORITY_IDX_IN_MAIL_TBL, mail_tbl_data->priority);
	_bindStmtFieldDataInt(hStmt, SAVE_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->save_status);
	_bindStmtFieldDataInt(hStmt, LOCK_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->lock_status);
	_bindStmtFieldDataInt(hStmt, REPORT_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->report_status);
	_bindStmtFieldDataInt(hStmt, ATTACHMENT_COUNT_IDX_IN_MAIL_TBL, mail_tbl_data->attachment_count);
	_bindStmtFieldDataInt(hStmt, INLINE_CONTENT_COUNT_IDX_IN_MAIL_TBL, mail_tbl_data->inline_content_count);
	_bindStmtFieldDataInt(hStmt, THREAD_ID_IDX_IN_MAIL_TBL, mail_tbl_data->thread_id);
	_bindStmtFieldDataInt(hStmt, THREAD_ITEM_COUNT_IDX_IN_MAIL_TBL, mail_tbl_data->thread_item_count);
	_bindStmtFieldDataString(hStmt, PREVIEW_TEXT_IDX_IN_MAIL_TBL, (char *)mail_tbl_data->preview_text, 1, PREVIEWBODY_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataInt(hStmt, MEETING_REQUEST_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->meeting_request_status);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; }, ("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("sqlite3_step fail:%d", rc));
	ret = true;
	
FINISH_OFF:

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	

	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(emStorageMailAdd);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_move_multiple_mails(int account_id, char *target_mailbox_name, int mail_ids[], int number_of_mails, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], target_mailbox_name [%s], mail_ids[%p], number_of_mails [%d], transaction[%d], err_code[%p]", account_id, target_mailbox_name, mail_ids, number_of_mails, transaction, err_code);

	int rc, ret = false, i, cur_conditional_clause = 0;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, }, conditional_clause[QUERY_SIZE] = {0, };
	emf_mailbox_tbl_t *result_mailbox = NULL;
	emf_mailbox_type_e target_mailbox_type = EMF_MAILBOX_TYPE_USER_DEFINED;

	if (!mail_ids || !target_mailbox_name) {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}		
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	if(!em_storage_get_mailbox_by_name(account_id, -1, target_mailbox_name, &result_mailbox, true, &error)) {
		EM_DEBUG_EXCEPTION("em_storage_get_mailbox_by_name failed [%d]", error);
		goto FINISH_OFF;
	}

	if(result_mailbox) {
		target_mailbox_type = result_mailbox->mailbox_type;
		em_storage_free_mailbox(&result_mailbox, 1, NULL);
	}
	
	cur_conditional_clause = SNPRINTF(conditional_clause, QUERY_SIZE, "WHERE mail_id in (");

	for(i = 0; i < number_of_mails; i++) 		
		cur_conditional_clause += SNPRINTF_OFFSET(conditional_clause, cur_conditional_clause, QUERY_SIZE, "%d,", mail_ids[i]);

	conditional_clause[strlen(conditional_clause) - 1] = ')';

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);

	/* Updating a mail_tbl */

	memset(sql_query_string, 0x00, QUERY_SIZE);
	SNPRINTF(sql_query_string, QUERY_SIZE, "UPDATE mail_tbl SET mailbox_name = '%s', mailbox_type = %d %s", target_mailbox_name, target_mailbox_type, conditional_clause);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/* Updating a mail_attachment_tbl */

	memset(sql_query_string, 0x00, QUERY_SIZE);
	SNPRINTF(sql_query_string, QUERY_SIZE, "UPDATE mail_attachment_tbl SET mailbox_name = '%s' %s", target_mailbox_name, conditional_clause);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/* Updating a mail_meeting_tbl */
	memset(sql_query_string, 0x00, QUERY_SIZE);
	SNPRINTF(sql_query_string, QUERY_SIZE, "UPDATE mail_meeting_tbl SET mailbox_name = '%s' %s", target_mailbox_name, conditional_clause);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/* Updating a mail_read_mail_uid_tbl */
	memset(conditional_clause, 0x00, QUERY_SIZE);
	cur_conditional_clause = SNPRINTF(conditional_clause, QUERY_SIZE, "WHERE local_uid in (");
	
	for(i = 0; i < number_of_mails; i++) 		
		cur_conditional_clause += SNPRINTF_OFFSET(conditional_clause, cur_conditional_clause, QUERY_SIZE, "%d,", mail_ids[i]);

	conditional_clause[strlen(conditional_clause) - 1] = ')';

	memset(sql_query_string, 0x00, QUERY_SIZE);
	SNPRINTF(sql_query_string, QUERY_SIZE, "UPDATE mail_read_mail_uid_tbl SET mailbox_name = '%s', local_mbox = '%s' %s", target_mailbox_name, target_mailbox_name, conditional_clause);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	ret = true;
	
FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
		
EXPORT_API int em_storage_delete_mail(int mail_id, int from_server, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], transaction[%d], err_code[%p]", mail_id, transaction, err_code);

	if (!mail_id)  {
		EM_DEBUG_EXCEPTION("mail_id[%d]", mail_id);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}		
	
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_tbl WHERE mail_id = %d ", mail_id);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);
 

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	ret = true;
	
FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_delete_multiple_mails(int mail_ids[], int number_of_mails, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], number_of_mails [%d], transaction[%d], err_code[%p]", mail_ids, number_of_mails, transaction, err_code);

	int rc, ret = false, i, cur_sql_query_string = 0;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	if (!mail_ids) {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}		
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	cur_sql_query_string = SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_tbl WHERE mail_id in (");
	
	for(i = 0; i < number_of_mails; i++) 		
		cur_sql_query_string += SNPRINTF_OFFSET(sql_query_string, cur_sql_query_string, QUERY_SIZE, "%d,", mail_ids[i]);

	sql_query_string[strlen(sql_query_string) - 1] = ')';
	
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	ret = true;
	
FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_delete_mail_by_account(int account_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], transaction[%d], err_code[%p]", account_id, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID)  {
		EM_DEBUG_EXCEPTION("account_id[%d]", account_id);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_tbl WHERE account_id = %d", account_id);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no mail found...");
		error = EM_STORAGE_ERROR_MAIL_NOT_FOUND;
	}
 
	/* Delete all mails  mail_read_mail_uid_tbl table based on account id */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_read_mail_uid_tbl WHERE account_id = %d", account_id);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION("no mail found...");
		error = EM_STORAGE_ERROR_MAIL_NOT_FOUND;
	}

	if (!em_storage_notify_storage_event(NOTI_MAIL_DELETE_WITH_ACCOUNT, account_id, 0 , NULL, 0))
		EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event Failed [ NOTI_MAIL_DELETE_ALL ]");
	
	ret = true;
	
FINISH_OFF:

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_delete_mail_by_mailbox(int account_id, char *mbox, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mbox[%p], transaction[%d], err_code[%p]", account_id, mbox, transaction, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !mbox)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], mbox[%p]", account_id, mbox);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_tbl WHERE account_id = %d AND mailbox_name = '%s'", account_id, mbox);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		

	/* Delete Mails from mail_read_mail_uid_tbl */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_read_mail_uid_tbl WHERE account_id = %d AND mailbox_name = '%s'", account_id, mbox);
	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
 	
	if (!em_storage_notify_storage_event(NOTI_MAIL_DELETE_ALL, account_id, 0 , mbox, 0))
		EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event Failed [ NOTI_MAIL_DELETE_ALL ] >>>> ");
	
	ret = true;
	
FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_free_mail(emf_mail_tbl_t** mail_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_list[%p], count[%d], err_code[%p]", mail_list, count, err_code);
	
	if (count > 0)  {
		if ((mail_list == NULL) || (*mail_list == NULL))  {
			EM_DEBUG_EXCEPTION("mail_ilst[%p], count[%d]", mail_list, count);
			
			if (err_code)
				*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
			return false;
		}

		emf_mail_tbl_t* p = *mail_list;
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
			EM_SAFE_FREE(p->datetime);
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
		*err_code = EM_STORAGE_ERROR_NONE;

	EM_DEBUG_FUNC_END();
	return true;
}

EXPORT_API int em_storage_get_attachment_count(int mail_id, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], count[%p], transaction[%d], err_code[%p]", mail_id, count, transaction, err_code);
	
	if (mail_id <= 0 || !count)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], count[%p]", mail_id, count);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_attachment_tbl WHERE mail_id = %d", mail_id);

	char **result;
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);
 
	ret = true;
	
FINISH_OFF:
 
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_attachment_list(int input_mail_id, int input_transaction, emf_mail_attachment_tbl_t** output_attachment_list, int *output_attachment_count)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], input_transaction[%d], output_attachment_list[%p], output_attachment_count[%p]", input_mail_id, output_attachment_list, input_transaction, output_attachment_count);
	
	if (input_mail_id <= 0 || !output_attachment_list || !output_attachment_count)  {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		return EM_STORAGE_ERROR_INVALID_PARAM;
	}
	
	int error = EM_STORAGE_ERROR_NONE;
	int                         i = 0;
	int                         rc = -1;
	char                      **result = NULL;
	char                        sql_query_string[QUERY_SIZE] = {0, };
	emf_mail_attachment_tbl_t* p_data_tbl = NULL;
	DB_STMT hStmt = NULL;
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	
	EM_STORAGE_START_READ_TRANSACTION(input_transaction);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_attachment_tbl WHERE mail_id = %d", input_mail_id);
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*output_attachment_count = atoi(result[1]);
	sqlite3_free_table(result);
	
	if(*output_attachment_count == 0) {
		error = EM_STORAGE_ERROR_NONE;
		goto FINISH_OFF;
	}
	
	p_data_tbl = (emf_mail_attachment_tbl_t*)em_core_malloc(sizeof(emf_mail_attachment_tbl_t) * (*output_attachment_count));
	
	if (!p_data_tbl)  {
		EM_DEBUG_EXCEPTION("em_core_malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_attachment_tbl WHERE mail_id = %d ORDER BY attachment_id", input_mail_id);
	EM_DEBUG_LOG("sql_query_string [%s]", sql_query_string);
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },	("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no matched attachment found...");
		error = EM_STORAGE_ERROR_ATTACHMENT_NOT_FOUND;
		goto FINISH_OFF;
	}
	for (i = 0; i < *output_attachment_count; i++)  {
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].attachment_id), ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].attachment_name), 0, ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].attachment_path), 0, ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].attachment_size), ATTACHMENT_SIZE_IDX_IN_MAIL_ATTACHMENT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].mail_id), MAIL_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].account_id), ACCOUNT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
		_getStmtFieldDataString(hStmt, &(p_data_tbl[i].mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_ATTACHMENT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].file_yn), FILE_YN_IDX_IN_MAIL_ATTACHMENT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].flag1), FLAG1_IDX_IN_MAIL_ATTACHMENT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].flag2), FLAG2_IDX_IN_MAIL_ATTACHMENT_TBL);
		_getStmtFieldDataInt(hStmt, &(p_data_tbl[i].flag3), FLAG3_IDX_IN_MAIL_ATTACHMENT_TBL);
		
		EM_DEBUG_LOG("attachment[%d].attachment_id : %d", i, p_data_tbl[i].attachment_id);
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; }, ("sqlite3_step fail:%d", rc));
	}
	
FINISH_OFF:

	if (error == EM_STORAGE_ERROR_NONE)
		*output_attachment_list = p_data_tbl;
	else if (p_data_tbl != NULL)
		em_storage_free_attachment(&p_data_tbl, *output_attachment_count, NULL);

	rc = sqlite3_finalize(hStmt);
	
	if (rc != SQLITE_OK)  {
		EM_DEBUG_EXCEPTION("sqlite3_finalize failed [%d]", rc);
		error = EM_STORAGE_ERROR_DB_FAILURE;
	}
	
	EM_STORAGE_FINISH_READ_TRANSACTION(input_transaction);
	
	_DISCONNECT_DB;

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

EXPORT_API int em_storage_get_attachment(int mail_id, int attachment_id, emf_mail_attachment_tbl_t** attachment, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], attachment_id[%d], attachment[%p], transaction[%d], err_code[%p]", mail_id, attachment_id, attachment, transaction, err_code);
	
	if (mail_id <= 0 || attachment_id <= 0 || !attachment)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], attachment_id[%d], attachment[%p]", mail_id, attachment_id, attachment);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	emf_mail_attachment_tbl_t* p_data_tbl = NULL;
	char *p = NULL;
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_attachment_tbl WHERE mail_id = %d AND attachment_id = %d", mail_id, attachment_id);

	sqlite3_stmt* hStmt = NULL;
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG(" before sqlite3_prepare hStmt = %p", hStmt);
	
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION("no matched attachment found...");
		error = EM_STORAGE_ERROR_ATTACHMENT_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (!(p_data_tbl = (emf_mail_attachment_tbl_t*)malloc(sizeof(emf_mail_attachment_tbl_t) * 1)))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emf_mail_attachment_tbl_t) * 1);

	p_data_tbl->attachment_id = sqlite3_column_int(hStmt, ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	if ((p = (char *)sqlite3_column_text(hStmt, ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)strlen(p))
		p_data_tbl->attachment_name = cpy_str(p);
	if ((p = (char *)sqlite3_column_text(hStmt, ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)strlen(p))
		p_data_tbl->attachment_path = cpy_str(p);
	p_data_tbl->attachment_size = sqlite3_column_int(hStmt, ATTACHMENT_SIZE_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->mail_id = sqlite3_column_int(hStmt, MAIL_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->account_id = sqlite3_column_int(hStmt, ACCOUNT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	if ((p = (char *)sqlite3_column_text(hStmt, MAILBOX_NAME_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)strlen(p))
		p_data_tbl->mailbox_name = cpy_str(p);
	p_data_tbl->file_yn = sqlite3_column_int(hStmt, FILE_YN_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->flag1 = sqlite3_column_int(hStmt, FLAG1_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->flag2 = sqlite3_column_int(hStmt, FLAG2_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->flag3 = sqlite3_column_int(hStmt, FLAG3_IDX_IN_MAIL_ATTACHMENT_TBL);

#ifdef __ATTACHMENT_OPTI__
		p_data_tbl->encoding = sqlite3_column_int(hStmt, ENCODING_IDX_IN_MAIL_ATTACHMENT_TBL);
		if ((p = (char *)sqlite3_column_text(hStmt, SECTION_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)strlen(p))
			p_data_tbl->section= cpy_str(p);
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
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_attachment_nth(int mail_id, int nth, emf_mail_attachment_tbl_t** attachment_tbl, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], nth[%d], attachment_tbl[%p], transaction[%d], err_code[%p]", mail_id, nth, attachment_tbl, transaction, err_code);

	if (mail_id <= 0 || nth <= 0 || !attachment_tbl)  {
		EM_DEBUG_EXCEPTION(" mail_id[%d], nth[%d], attachment[%p]", mail_id, nth, attachment_tbl);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	emf_mail_attachment_tbl_t* p_data_tbl = NULL;
	char *p = NULL;
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_attachment_tbl WHERE mail_id = %d ORDER BY attachment_id LIMIT %d, 1", mail_id, (nth - 1));
	EM_DEBUG_LOG("query = [%s]", sql_query_string);
	
	DB_STMT hStmt = NULL;
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION(" no matched attachment found...");
		error = EM_STORAGE_ERROR_ATTACHMENT_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	if (!(p_data_tbl = (emf_mail_attachment_tbl_t*)malloc(sizeof(emf_mail_attachment_tbl_t) * 1)))  {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memset(p_data_tbl, 0x00, sizeof(emf_mail_attachment_tbl_t) * 1);

	p_data_tbl->attachment_id = sqlite3_column_int(hStmt, ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	if ((p = (char *)sqlite3_column_text(hStmt, ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)strlen(p))
		p_data_tbl->attachment_name = cpy_str(p);
	if ((p = (char *)sqlite3_column_text(hStmt, ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)strlen(p))
		p_data_tbl->attachment_path = cpy_str(p);
	p_data_tbl->attachment_size = sqlite3_column_int(hStmt, ATTACHMENT_SIZE_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->mail_id = sqlite3_column_int(hStmt, MAIL_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->account_id = sqlite3_column_int(hStmt, ACCOUNT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	if ((p = (char *)sqlite3_column_text(hStmt, MAILBOX_NAME_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)strlen(p))
		p_data_tbl->mailbox_name = cpy_str(p);
	p_data_tbl->file_yn = sqlite3_column_int(hStmt, FILE_YN_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->flag1 = sqlite3_column_int(hStmt, FLAG1_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->flag2 = sqlite3_column_int(hStmt, FLAG2_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->flag3 = sqlite3_column_int(hStmt, FLAG3_IDX_IN_MAIL_ATTACHMENT_TBL);
	
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
		em_storage_free_attachment(&p_data_tbl, 1, NULL);

	if (hStmt != NULL)  {
		EM_DEBUG_LOG("before sqlite3_finalize hStmt = %p", hStmt);
	
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_change_attachment_field(int mail_id, emf_mail_change_type_t type, emf_mail_attachment_tbl_t* attachment, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], type[%d], attachment[%p], transaction[%d], err_code[%p]", mail_id, type, attachment, transaction, err_code);
	
	if (mail_id <= 0 || !attachment)  {
		EM_DEBUG_EXCEPTION(" mail_id[%d], type[%d], attachment[%p]", mail_id, type, attachment);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;;
	}
	
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;	
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	int i = 0;
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	switch (type)  {
		case UPDATE_MAILBOX:
				EM_DEBUG_LOG("UPDATE_MAILBOX");
			if (!attachment->mailbox_name)  {
				EM_DEBUG_EXCEPTION(" attachment->mailbox_name[%p]", attachment->mailbox_name);
				error = EM_STORAGE_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}
#ifdef CONVERT_UTF8_TO_UCS2			
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_attachment_tbl SET mailbox_name = '%s' WHERE mail_id = %d", attachment->mailbox_name, mail_id);
#else  /* CONVERT_UTF8_TO_UCS2 */
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_attachment_tbl SET mailbox_name = ? WHERE mail_id = %d", mail_id);
 
			
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_LOG(" Before sqlite3_prepare hStmt = %p", hStmt);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

			_bindStmtFieldDataString(hStmt, i++, (char *)attachment->mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_ATTACHMENT_TBL);
#endif	/* CONVERT_UTF8_TO_UCS2 */
			break;
			
		case UPDATE_SAVENAME:
			EM_DEBUG_LOG("UPDATE_SAVENAME");
			if (!attachment->attachment_path)  {
				EM_DEBUG_EXCEPTION(" attachment->attachment_path[%p]", attachment->attachment_path);
				error = EM_STORAGE_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}
			
#ifdef CONVERT_UTF8_TO_UCS2
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_attachment_tbl SET"
				"  attachment_size = %d"
				", attachment_path = '%s'"
				", file_yn = 1"
				" WHERE mail_id = %d"
				" AND attachment_id = %d"
				, attachment->attachment_size
				, attachment->attachment_path
				, attachment->mail_id
				, attachment->attachment_id);
#else	/* CONVERT_UTF8_TO_UCS2 */
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_attachment_tbl SET"
				"  attachment_size = ?"
				", file_yn = 1"
				", attachment_path = ?"
				" WHERE mail_id = %d"
				" AND attachment_id = %d"
				, attachment->mail_id
				, attachment->attachment_id);
 
			
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_LOG(" Before sqlite3_prepare hStmt = %p", hStmt);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		
			_bindStmtFieldDataInt(hStmt, i++, attachment->attachment_size);
			_bindStmtFieldDataString(hStmt, i++, (char *)attachment->attachment_path, 0, ATTACHMENT_PATH_LEN_IN_MAIL_ATTACHMENT_TBL);
#endif	/* CONVERT_UTF8_TO_UCS2 */
			break;
			
		default:
			EM_DEBUG_LOG("type[%d]", type);
			error = EM_STORAGE_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}
	EM_DEBUG_LOG("query = [%s]", sql_query_string);

#ifdef CONVERT_UTF8_TO_UCS2	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
#else
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
#endif
	ret = true;
	
FINISH_OFF:
#ifdef CONVERT_UTF8_TO_UCS2
#else  /*  CONVERT_UTF8_TO_UCS2 */
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" Before sqlite3_finalize hStmt = %p", hStmt);
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
#endif /* CONVERT_UTF8_TO_UCS2 */

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_rename_mailbox(int account_id, char *old_mailbox_name, char *new_mailbox_name, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], old_mailbox_name[%p], new_mailbox_name[%p], transaction[%d], err_code[%p]", account_id, old_mailbox_name, new_mailbox_name, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID ||!old_mailbox_name || !new_mailbox_name)  {
		EM_DEBUG_EXCEPTION("Invalid Parameters");
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = true;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_tbl SET"
		" mailbox_name = '%s'"
		" WHERE account_id = %d"
		" AND mailbox_name = '%s'"
		, new_mailbox_name
		, account_id
		, old_mailbox_name);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (sqlite3_changes(local_db_handle) == 0) 
		EM_DEBUG_LOG("no mail matched...");

 
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_attachment_tbl SET"
		" mailbox_name = '%s'"
		" WHERE account_id = %d"
		" AND mailbox_name = '%s'"
		, new_mailbox_name
		, account_id
		, old_mailbox_name);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (sqlite3_changes(local_db_handle) == 0) 
		EM_DEBUG_LOG("no attachment matched...");

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_meeting_tbl SET"
		" mailbox_name = '%s'"
		" WHERE account_id = %d"
		" AND mailbox_name = '%s'"
		, new_mailbox_name
		, account_id
		, old_mailbox_name);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (sqlite3_changes(local_db_handle) == 0) 
		EM_DEBUG_LOG("no mail_meeting_tbl matched...");

	ret = true;
	
FINISH_OFF:

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
	

EXPORT_API int em_storage_change_attachment_mbox(int account_id, char *old_mailbox_name, char *new_mailbox_name, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], old_mailbox_name[%p], new_mailbox_name[%p], transaction[%d], err_code[%p]", account_id, old_mailbox_name, new_mailbox_name, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID || !old_mailbox_name || !new_mailbox_name)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], old_mailbox_name[%p], new_mailbox_name[%p]", account_id, old_mailbox_name, new_mailbox_name);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = true;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
 
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_attachment_tbl SET"
		" mailbox_name = '%s'"
		" WHERE account_id = %d"
		" AND mailbox_name = '%s'"
		, new_mailbox_name
		, account_id
		, old_mailbox_name);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no attachment found...");
	}

	ret = true;
	
FINISH_OFF:

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

	
EXPORT_API int em_storage_get_new_attachment_no(int *attachment_no, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_no [%p], err_code[%p]", attachment_no, err_code);
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char *sql = "SELECT max(rowid) FROM mail_attachment_tbl;";
	char **result;
	
	if (!attachment_no)  {
		EM_DEBUG_EXCEPTION("Invalid attachment");
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	*attachment_no = -1;
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
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
	
EXPORT_API int em_storage_add_attachment(emf_mail_attachment_tbl_t* attachment_tbl, int iscopy, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_tbl[%p], iscopy[%d], transaction[%d], err_code[%p]", attachment_tbl, iscopy, transaction, err_code);

	char *sql = NULL;
	char **result;
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	
	if (!attachment_tbl)  {
		EM_DEBUG_EXCEPTION("attachment_tbl[%p], iscopy[%d]", attachment_tbl, iscopy);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
 
	sql = "SELECT max(rowid) FROM mail_attachment_tbl;";
	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);  */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL==result[1]) rc = 1;
	else rc = atoi(result[1]) + 1;
	sqlite3_free_table(result);

	attachment_tbl->attachment_id = rc;
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_attachment_tbl VALUES "
		"( ?"	/*  attachment_id */
		", ?"	/*  attachment_name */
		", ?"	/*  attachment_path */
		", ?"	/*  attachment_size */
		", ?"	/*  mail_id */
		", ?"	/*  account_id */
		", ?"	/*  mailbox_name */
		", ?"	/*  file_yn */
		", ?"	/*  flag1 */
		", ?"	/*  flag2 */
#ifdef __ATTACHMENT_OPTI__
		", ?"
		", ?"
#endif		
		", ? )");/*  flag3 */
	
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	_bindStmtFieldDataInt(hStmt, ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->attachment_id);
	_bindStmtFieldDataString(hStmt, ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL, (char *)attachment_tbl->attachment_name, 0, ATTACHMENT_NAME_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bindStmtFieldDataString(hStmt, ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL, (char *)attachment_tbl->attachment_path, 0, ATTACHMENT_PATH_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bindStmtFieldDataInt(hStmt, ATTACHMENT_SIZE_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->attachment_size);
	_bindStmtFieldDataInt(hStmt, MAIL_ID_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->mail_id);
	_bindStmtFieldDataInt(hStmt, ACCOUNT_ID_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->account_id);
	_bindStmtFieldDataString(hStmt, MAILBOX_NAME_IDX_IN_MAIL_ATTACHMENT_TBL, (char *)attachment_tbl->mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bindStmtFieldDataInt(hStmt, FILE_YN_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->file_yn);
	_bindStmtFieldDataInt(hStmt, FLAG1_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->flag1);
	_bindStmtFieldDataInt(hStmt, FLAG2_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->flag2);
	_bindStmtFieldDataInt(hStmt, FLAG3_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->flag3);
#ifdef __ATTACHMENT_OPTI__
	_bindStmtFieldDataInt(hStmt, ENCODING_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->encoding);
	_bindStmtFieldDataString(hStmt, SECTION_IDX_IN_MAIL_ATTACHMENT_TBL, (char *)attachment_tbl->section, 0, ATTACHMENT_LEN_IN_MAIL_ATTACHMENT_TBL);
#endif
	
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
/*
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_tbl SET attachment_count = 1 WHERE mail_id = %d", attachment_tbl->mail_id);
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
*/
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no matched mail found...");
		error = EM_STORAGE_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
	*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_update_attachment(emf_mail_attachment_tbl_t* attachment_tbl,	   		 int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_tbl[%p], transaction[%d], err_code[%p]", attachment_tbl, transaction, err_code);
	
	int rc, ret = false, field_idx = 0;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
 	char sql_query_string[QUERY_SIZE] = {0, };

	if (!attachment_tbl)  {
		EM_DEBUG_EXCEPTION(" attachment_tbl[%p] ", attachment_tbl);
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
 
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_attachment_tbl SET  "
		"  attachment_name = ?"   
		", attachment_path =  ?"
		", attachment_size = ?"
		", mail_id = ?"
		", account_id = ?"
		", mailbox_name = ?"
		", file_yn = ?"
		", flag1 = ?"
		", flag2 = ?"
		", flag3 = ? "
		" WHERE attachment_id = ?;");
	
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bindStmtFieldDataString(hStmt, field_idx++ , (char *)attachment_tbl->attachment_name, 0, ATTACHMENT_NAME_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bindStmtFieldDataString(hStmt, field_idx++ , (char *)attachment_tbl->attachment_path, 0, ATTACHMENT_PATH_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bindStmtFieldDataInt(hStmt, field_idx++ , attachment_tbl->attachment_size);
	_bindStmtFieldDataInt(hStmt, field_idx++ , attachment_tbl->mail_id);
	_bindStmtFieldDataInt(hStmt, field_idx++ , attachment_tbl->account_id);
	_bindStmtFieldDataString(hStmt, field_idx++ , (char *)attachment_tbl->mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bindStmtFieldDataInt(hStmt, field_idx++ , attachment_tbl->file_yn);
	_bindStmtFieldDataInt(hStmt, field_idx++ , attachment_tbl->flag1);
	_bindStmtFieldDataInt(hStmt, field_idx++ , attachment_tbl->flag2);
	_bindStmtFieldDataInt(hStmt, field_idx++ , attachment_tbl->flag3);
	_bindStmtFieldDataInt(hStmt, field_idx++ , attachment_tbl->attachment_id);
	
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
/* 
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_tbl SET attachment_count = 1 WHERE mail_id = %d", attachment_tbl->mail_id);
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
*/
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no matched mail found...");
		error = EM_STORAGE_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
	*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_delete_attachment_on_db(int mail_id, int attachment_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], attachment_id[%d], transaction[%d], err_code[%p]", mail_id, attachment_id, transaction, err_code);
	
	if (mail_id < 0 || attachment_id < 0)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], attachment_id[%d]", mail_id, attachment_id);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_attachment_tbl");
	
	if (mail_id)  	/*  '0' means all mail */
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " WHERE mail_id = %d", mail_id);
	if (attachment_id) /*  '0' means all attachment */
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " AND attachment_id = %d", attachment_id);
		
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	ret = true;
	
FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_delete_attachment_all_on_db(int account_id, char *mbox, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mbox[%p], transaction[%d], err_code[%p]", account_id, mbox, transaction, err_code);
	
	int error = EM_STORAGE_ERROR_NONE;
	int rc, ret = false;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_attachment_tbl");
	
	if (account_id != ALL_ACCOUNT) /*  '0' means all account */
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " WHERE account_id = %d", account_id);

	if (mbox) 	/*  NULL means all mailbox_name */
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " %s mailbox_name = '%s'", account_id != ALL_ACCOUNT ? "AND" : "WHERE", mbox);
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	ret = true;
	
FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_free_attachment(emf_mail_attachment_tbl_t** attachment_tbl_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_tbl_list[%p], count[%d], err_code[%p]", attachment_tbl_list, count, err_code);
	
	if (count > 0)  {
		if ((attachment_tbl_list == NULL) || (*attachment_tbl_list == NULL))  {
			EM_DEBUG_EXCEPTION(" attachment_tbl_list[%p], count[%d]", attachment_tbl_list, count);
			if (err_code != NULL)
				*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
			return false;
		}
		
		emf_mail_attachment_tbl_t* p = *attachment_tbl_list;
		int i;
		
		for (i = 0; i < count; i++)  {
			EM_SAFE_FREE(p[i].mailbox_name);
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
		*err_code = EM_STORAGE_ERROR_NONE;
	EM_DEBUG_FUNC_END();
	return true;
}



EXPORT_API int em_storage_begin_transaction(void *d1, void *d2, int *err_code)
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
 
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	int rc;
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN immediate;", NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {ret = false; },
		("SQL(BEGIN) exec error:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
	if (ret == false && err_code != NULL)
		*err_code = EM_STORAGE_ERROR_DB_FAILURE;

	EM_PROFILE_END(emStorageBeginTransaction);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_commit_transaction(void *d1, void *d2, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = true;
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	ENTER_CRITICAL_SECTION(_transactionEndLock);

	int rc;
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {ret = false; }, ("SQL(END) exec error:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
	/*  release the transaction authority. */
	g_transaction = false;

	LEAVE_CRITICAL_SECTION(_transactionEndLock);
	if (ret == false && err_code != NULL)
		*err_code = EM_STORAGE_ERROR_DB_FAILURE;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_rollback_transaction(void *d1, void *d2, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = true;
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	int rc;

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "ROLLBACK;", NULL, NULL, NULL), rc);

	ENTER_CRITICAL_SECTION(_transactionEndLock);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {ret = false; },
		("SQL(ROLLBACK) exec error:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

	/*  release the transaction authority. */
	g_transaction = false;

	LEAVE_CRITICAL_SECTION(_transactionEndLock);

	if (ret == false && err_code != NULL)
		*err_code = EM_STORAGE_ERROR_DB_FAILURE;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_is_mailbox_full(int account_id, emf_mailbox_t* mailbox_name, int *result, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%p], result[%p], err_code[%p]", account_id, mailbox_name, result, err_code);
	
	if (account_id < FIRST_ACCOUNT_ID || !mailbox_name || !result)  {
		if (mailbox_name)
			EM_DEBUG_EXCEPTION("Invalid Parameter. accoun_id[%d], mailbox_name[%p]", account_id, mailbox_name);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;

		return false;
	}

	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	int mail_count = 0;

	if (!em_storage_get_mail_count(account_id, mailbox_name->name, &mail_count, NULL, true, &error)) {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_count failed [%d]", error);
		goto FINISH_OFF;
	}
	
	if (mailbox_name) {
		EM_DEBUG_LOG("mail_count[%d] mail_slot_size[%d]", mail_count, mailbox_name->mail_slot_size);
		if (mail_count >= mailbox_name->mail_slot_size)
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

EXPORT_API int em_storage_clear_mail_data(int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("transaction[%d], err_code[%p]", transaction, err_code);

	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	const emf_db_object_t* tables = _g_db_tables;
	const emf_db_object_t* indexes = _g_db_indexes;
	char data_path[256];
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	if (!em_storage_delete_dir(MAILHOME, &error)) {
		EM_DEBUG_EXCEPTION(" em_storage_delete_dir failed - %d", error);

		goto FINISH_OFF;
	}
	
	SNPRINTF(data_path, sizeof(data_path), "%s/%s", MAILHOME, MAILTEMP);
	
	mkdir(MAILHOME, DIRECTORY_PERMISSION);
	mkdir(data_path, DIRECTORY_PERMISSION);
	
	/*  first clear index. */
	while (indexes->object_name)  {
		if (indexes->data_flag)  {
			SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP index %s", indexes->object_name);
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
			EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		}
		indexes++;
	}
	
	while (tables->object_name)  {
		if (tables->data_flag)  {
			SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP table %s", tables->object_name);
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
			EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
				("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		}
		
		tables++;
	}
	ret = true;
	
FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
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

EXPORT_API int em_storage_get_save_name(int account_id, int mail_id, int atch_id, char *fname, char *name_buf, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], atch_id[%d], fname[%s], name_buf[%p], err_code[%p]", account_id, mail_id, atch_id, fname, name_buf, err_code);
	EM_PROFILE_BEGIN(profile_em_storage_get_save_name);
	
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char *dir_name = NULL;
	char create_dir[1024]={0};
	char *temp_file = NULL;
	
	if (!name_buf || account_id < FIRST_ACCOUNT_ID || mail_id < 0 || atch_id < 0)  {	
		EM_DEBUG_EXCEPTION(" account_id[%d], mail_id[%d], atch_id[%d], fname[%p], name_buf[%p]", account_id, mail_id, atch_id, fname, name_buf);
		error = EM_STORAGE_ERROR_INVALID_PARAM;
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

	EM_PROFILE_END(profile_em_storage_get_save_name);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_dele_name(int account_id, int mail_id, int atch_id, char *fname, char *name_buf, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], atch_id[%d], fname[%p], name_buf[%p], err_code[%p]", account_id, mail_id, atch_id, fname, name_buf, err_code);
	
	if (!name_buf || account_id < FIRST_ACCOUNT_ID)  {
		EM_DEBUG_EXCEPTION(" account_id[%d], mail_id[%d], atch_id[%d], fname[%p], name_buf[%p]", account_id, mail_id, atch_id, fname, name_buf);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
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

EXPORT_API int em_storage_create_dir(int account_id, int mail_id, int atch_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], atch_id[%d], err_code[%p]", account_id, mail_id, atch_id, err_code);
	EM_PROFILE_BEGIN(profile_em_core_save_create_dir);
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	
	char buf[512];
	struct stat sbuf;
	if (account_id >= FIRST_ACCOUNT_ID)  {	
		SNPRINTF(buf, sizeof(buf), "%s%s%d", MAILHOME, DIR_SEPERATOR, account_id);
		
		if (stat(buf, &sbuf) == 0) {
			if ((sbuf.st_mode & S_IFMT) != S_IFDIR)  {
				EM_DEBUG_EXCEPTION(" a object which isn't directory aleady exists");
				error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		}
		else  {
			if (mkdir(buf, DIRECTORY_PERMISSION) != 0) {
				EM_DEBUG_EXCEPTION(" mkdir failed [%s]", buf);
				EM_DEBUG_EXCEPTION("mkdir failed l(Errno=%d)][ErrStr=%s]", errno, strerror(errno));
				error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		}
	}

	if (mail_id > 0)  {
		if (account_id < FIRST_ACCOUNT_ID)  {
			EM_DEBUG_EXCEPTION("account_id[%d], mail_id[%d], atch_id[%d]", account_id, mail_id, atch_id);
			error = EM_STORAGE_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		SNPRINTF(buf+strlen(buf), sizeof(buf), "%s%d", DIR_SEPERATOR, mail_id);
		
		if (stat(buf, &sbuf) == 0) {
			if ((sbuf.st_mode & S_IFMT) != S_IFDIR)  {
				EM_DEBUG_EXCEPTION(" a object which isn't directory aleady exists");
				
				error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		}
		else  {
			if (mkdir(buf, DIRECTORY_PERMISSION) != 0) {
				EM_DEBUG_EXCEPTION(" mkdir failed [%s]", buf);
				EM_DEBUG_EXCEPTION("mkdir failed l (Errno=%d)][ErrStr=%s]", errno, strerror(errno));
				error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		}
	}

	if (atch_id > 0)  {
		if (account_id < FIRST_ACCOUNT_ID || mail_id <= 0)  {
			EM_DEBUG_EXCEPTION(" account_id[%d], mail_id[%d], atch_id[%d]", account_id, mail_id, atch_id);
			
			error = EM_STORAGE_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		SNPRINTF(buf+strlen(buf), sizeof(buf)-(strlen(buf)+1), "%s%d", DIR_SEPERATOR, atch_id);
		
		if (stat(buf, &sbuf) == 0) {
			if ((sbuf.st_mode & S_IFMT) != S_IFDIR)  {
				EM_DEBUG_EXCEPTION(" a object which isn't directory aleady exists");
				
				error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		}
		else  {
			if (mkdir(buf, DIRECTORY_PERMISSION) != 0) {
				EM_DEBUG_EXCEPTION(" mkdir failed [%s]", buf);
				error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		}
	}

	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(profile_em_core_save_create_dir);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_copy_file(char *src_file, char *dst_file, int sync_status, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("src_file[%s], dst_file[%s], err_code[%p]", src_file, dst_file, err_code);
	EM_DEBUG_LOG("Using the fsync function");
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	struct stat st_buf;

	int fp_src = 0;
	int fp_dst = 0;
	int nread = 0;
	int nwritten = 0;
	char *buf =  NULL;
	int buf_size = 0;
	
	if (!src_file || !dst_file)  {
		EM_DEBUG_EXCEPTION("src_file[%p], dst_file[%p]", src_file, dst_file);
		
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (stat(src_file, &st_buf) < 0) {
		EM_DEBUG_EXCEPTION("stat(\"%s\") failed...", src_file);
		
		error = EM_STORAGE_ERROR_SYSTEM_FAILURE;		/* EMF_ERROR_INVALID_PATH; */
		goto FINISH_OFF;
	}
	
	buf_size =  st_buf.st_size;
	EM_DEBUG_LOG(">>>> File Size [ %d ]", buf_size);
	buf = (char *)calloc(1, buf_size+1);

	if (!buf) {
		EM_DEBUG_EXCEPTION(">>> Memory cannot be allocated");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (buf) {
		if (!(fp_src = open(src_file, O_RDONLY))) {
			EM_DEBUG_EXCEPTION(">>>> Source Fail open %s Failed [ %d ] - Error [ %s ]", src_file, errno, strerror(errno));
			error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;				
		}

		if (!(fp_dst = open(dst_file, O_CREAT | O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH))) {
			EM_DEBUG_EXCEPTION(">>>> Destination Fail open %s Failed [ %d ] - Error [ %s ]", dst_file, errno, strerror(errno));
			error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;				
		}

		while ((nread = read(fp_src, buf, buf_size)) > 0) {
			if (nread > 0 && nread <= buf_size)  {		
				EM_DEBUG_LOG("Nread Value [%d]", nread);
				if ((nwritten = write(fp_dst, buf, nread)) != nread) {
					EM_DEBUG_EXCEPTION("fwrite failed...");
					error = EM_STORAGE_ERROR_UNKNOWN;
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
	if (nread < 0 || error == EM_STORAGE_ERROR_UNKNOWN)
		remove(dst_file);
	
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
/* create Directory if user has deleted [deepam.p@samsung.com] */
EXPORT_API void em_storage_create_dir_if_delete()
{
	EM_DEBUG_FUNC_BEGIN();

	mkdir(EMAILPATH, DIRECTORY_PERMISSION);
	mkdir(DATA_PATH, DIRECTORY_PERMISSION);

	mkdir(MAILHOME, DIRECTORY_PERMISSION);
	
	SNPRINTF(g_db_path, sizeof(g_db_path), "%s/%s", MAILHOME, MAILTEMP);
	mkdir(g_db_path, DIRECTORY_PERMISSION);
	
	/* _emf_delete_temp_file(g_db_path); */
	EM_DEBUG_FUNC_END();
}
static 
int em_storage_get_temp_file_name(char **filename, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("filename[%p], err_code[%p]", filename, err_code);
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	
	if (filename == NULL) {
		EM_DEBUG_EXCEPTION(" filename[%p]", filename);
		error = EMF_ERROR_INVALID_PARAM;
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
		error = EMF_ERROR_OUT_OF_MEMORY;
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

EXPORT_API int em_storage_add_content_type(char *file_path, char *char_set, int *err_code)
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
	int error = EM_STORAGE_ERROR_NONE;
	int data_count_to_written = 0;
	char *temp_file_name = NULL;
	int err = 0;
	
	FILE* fp_src = NULL;
	FILE* fp_dest = NULL;
	int nread = 0;
	

	if (stat(file_path, &st_buf) < 0) {
		EM_DEBUG_EXCEPTION(" stat(\"%s\") failed...", file_path);
		
		error = EM_STORAGE_ERROR_SYSTEM_FAILURE;		/* EMF_ERROR_INVALID_PATH; */
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
			
			error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;				/*  20080509 prevent 28806 - forward null */
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

				EM_DEBUG_LOG(">>>>em_storage_add_content_type 1 ");
				
				match_str = strstr(buf, CONTENT_TYPE_DATA);
				EM_DEBUG_LOG(">>>>em_storage_add_content_type 2 ");
				
				if (match_str == NULL) {
					EM_DEBUG_LOG(">>>>em_storage_add_content_type 3 ");
					if (fp_src !=NULL) {
						fclose(fp_src);fp_src = NULL;
					}
				data_count_to_written = strlen(low_char_set)+strlen(CONTENT_DATA)+1;
					EM_DEBUG_LOG(">>>>em_storage_add_content_type 4 ");
				buf1 = (char *)calloc(1, data_count_to_written);
					EM_DEBUG_LOG(">>>>em_storage_add_content_type 5 ");

					if (buf1) {
						EM_DEBUG_LOG(">>>>em_storage_add_content_type 6 ");
					 	strncat(buf1, CONTENT_DATA, strlen(CONTENT_DATA));

						EM_DEBUG_LOG(">>>>> BUF 1 [ %s ] ", buf1);

						strncat(buf1, low_char_set, strlen(low_char_set));

						EM_DEBUG_LOG(">>>> HTML TAG DATA  [ %s ] ", buf1);


					/* 1. Create a temporary file name */
					if (!em_storage_get_temp_file_name(&temp_file_name, &err)) {
							EM_DEBUG_EXCEPTION(" em_core_get_temp_file_name failed - %d", err);
							if (err_code != NULL) *err_code = err;
							EM_SAFE_FREE(temp_file_name);
							goto FINISH_OFF;
					}
					EM_DEBUG_LOG(">>>>>>> TEMP APPEND FILE PATH [ %s ] ", temp_file_name);
					
					/* Open the Temp file in Append mode */
					if (!(fp_dest = fopen(temp_file_name, "ab"))) {
						EM_DEBUG_EXCEPTION(" fopen failed - %s", temp_file_name);
						error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
						goto FINISH_OFF;				/*  20080509 prevent 28806 - forward null */
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
							error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
							goto FINISH_OFF;
						}
						else {
							EM_DEBUG_LOG(">>>> OLD data appended [ %d ] ", nwritten);

							if (!em_storage_move_file(temp_file_name, file_path, false, &err)) {
								EM_DEBUG_EXCEPTION(" em_storage_move_file failed - %d", err);
								goto FINISH_OFF;
							}
						}
							
					}
					else {
						EM_DEBUG_EXCEPTION(" Error Occured while writing New data : [%d ] bytes written ", nwritten);
						error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
						goto FINISH_OFF;
					}
							
					}
		
				}
				EM_DEBUG_LOG(">>>>em_storage_add_content_type 15 ");

	
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

EXPORT_API int em_storage_move_file(char *src_file, char *dst_file, int sync_status, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("src_file[%p], dst_file[%p], err_code[%p]", src_file, dst_file, err_code);
	
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	
	if (src_file == NULL || dst_file == NULL)  {
		EM_DEBUG_EXCEPTION("src_file[%p], dst_file[%p]", src_file, dst_file);
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	EM_DEBUG_LOG("src_file[%s], dst_file[%s]", src_file, dst_file);

	if (strcmp(src_file, dst_file) != 0) {
		if (rename(src_file, dst_file) != 0) {
			if (errno == EXDEV)  {	/* oldpath and newpath are not on the same mounted file system.  (Linux permits a file system to be mounted at multiple points,  but  rename() */
				/*  does not work across different mount points, even if the same file system is mounted on both.)	 */
				EM_DEBUG_LOG("oldpath and newpath are not on the same mounted file system.");
				if (!em_storage_copy_file(src_file, dst_file, sync_status, &error)) {
					EM_DEBUG_EXCEPTION("em_storage_copy_file failed - %d", error);
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
					error = EM_STORAGE_ERROR_FILE_NOT_FOUND;
					goto FINISH_OFF;
					
				}
				else  {
					EM_DEBUG_EXCEPTION("rename failed [%d]", errno);
					error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
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

EXPORT_API int em_storage_delete_file(char *src_file, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("src_file[%p], err_code[%p]", src_file, err_code);
	
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	
	if (src_file == NULL) {
		EM_DEBUG_EXCEPTION(" src_file[%p]", src_file);
		
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (remove(src_file) != 0) {
		if (errno != ENOENT) {
			EM_DEBUG_EXCEPTION(" remove failed - %d", errno);
			
			error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}
		else {
			EM_DEBUG_EXCEPTION(" no file found...");
			
			error = EM_STORAGE_ERROR_FILE_NOT_FOUND;
		}
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_delete_dir(char *src_dir, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("src_dir[%p], err_code[%p]", src_dir, err_code);
	
	if (src_dir == NULL) {
		EM_DEBUG_EXCEPTION(" src_dir[%p]", src_dir);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int error = EM_STORAGE_ERROR_NONE;
	
	DIR *dirp;
	struct dirent *dp;
	struct stat sbuf;
	char buf[512];
	
	dirp = opendir(src_dir);
	
	if (dirp == NULL)  {
		if (errno == ENOENT)  {
			EM_DEBUG_EXCEPTION("directory[%s] does not exist...", src_dir);
			if (err_code != NULL)
				*err_code = EM_STORAGE_ERROR_SYSTEM_FAILURE;	
			return true;
		}
		else  {
			EM_DEBUG_EXCEPTION("opendir failed - %d", errno);
			if (err_code != NULL)
				*err_code = EM_STORAGE_ERROR_SYSTEM_FAILURE;
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
				if (!em_storage_delete_dir(buf, &error)) {
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
						*err_code = EM_STORAGE_ERROR_SYSTEM_FAILURE;
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
			*err_code = EM_STORAGE_ERROR_SYSTEM_FAILURE;
		return false;
	}
	
	if (err_code != NULL)
		*err_code = error;
	
	return true;
}

/* faizan.h@samsung.com */
EXPORT_API int em_storage_update_server_uid(char *old_server_uid, char *new_server_uid, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("new_server_uid[%s], old_server_uid[%s]", new_server_uid, old_server_uid);
	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	int transaction = true;
	
	if (!old_server_uid || !new_server_uid) {	
		EM_DEBUG_EXCEPTION("Invalid parameters");
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		 "UPDATE mail_tbl SET server_mail_id=\'%s\' WHERE server_mail_id=%s ", new_server_uid, old_server_uid);
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	ret = true;
	  
FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
	
}

EXPORT_API int em_storage_update_read_mail_uid(int mail_id, char *new_server_uid, char *mbox_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], new_server_uid[%s], mbox_name[%s]", mail_id, new_server_uid, mbox_name);

	int rc, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	int transaction = true;
	
	if (!mail_id || !new_server_uid || !mbox_name)  {		
		EM_DEBUG_EXCEPTION("Invalid parameters");
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);

	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		 "UPDATE mail_read_mail_uid_tbl SET s_uid=\'%s\', local_mbox=\'%s\', mailbox_name=\'%s\' WHERE local_uid=%d ", new_server_uid, mbox_name, mbox_name, mail_id);
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	ret	= true;
	  
FINISH_OFF:

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
	
}


int em_storage_get_latest_unread_mailid(int account_id, int *mail_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	if ((!mail_id) ||(account_id <= 0 &&  account_id != -1)) {
		EM_DEBUG_EXCEPTION(" mail_id[%p], account_id[%d] ", mail_id, account_id);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}	

	int ret = false;
	int rc = -1;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	int count = 0;
	int mailid = 0;
	int transaction = false;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	if (account_id == -1)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mail_id FROM mail_tbl WHERE flags_seen_field = 0 ORDER BY mail_id DESC");
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mail_id FROM mail_tbl WHERE account_id = %d AND flags_seen_field = 0 ORDER BY mail_id DESC", account_id);
		
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("  sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	char **result;

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	sqlite3_free_table(result);
	if (count == 0)  {
		EM_DEBUG_EXCEPTION("no Mails found...");
		ret = false;
		error= EM_STORAGE_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
 	
		_getStmtFieldDataInt(hStmt, &mailid, 0);

	ret = true;

FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
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


EXPORT_API int em_storage_mail_get_total_diskspace_usage(unsigned long *total_usage,  int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("total_usage[%p],  transaction[%d], err_code[%p]", total_usage, transaction, err_code);

	if (!total_usage) {
		EM_DEBUG_EXCEPTION("total_usage[%p]", total_usage);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int  ret = false;	
	int error = EM_STORAGE_ERROR_NONE;
	char syscmd[256] = {0};
	FILE *fp = NULL;
	unsigned long total_diskusage = 0;

	SNPRINTF(syscmd, sizeof(syscmd), "touch %s", SETTING_MEMORY_TEMP_FILE_PATH);
	if (setting_system_command(syscmd) == -1) {
		EM_DEBUG_EXCEPTION("em_storage_mail_get_total_diskspace_usage : [Setting > Memory] System Command [%s] is failed", syscmd);

		error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;		
	}

	memset(syscmd, 0, sizeof(syscmd));
	SNPRINTF(syscmd, sizeof(syscmd), "du -hsk %s > %s", EMAILPATH, SETTING_MEMORY_TEMP_FILE_PATH);
	EM_DEBUG_LOG(" cmd : %s", syscmd);
	if (setting_system_command(syscmd) == -1) {
		EM_DEBUG_EXCEPTION("em_storage_mail_get_total_diskspace_usage : Setting > Memory] System Command [%s] is failed", syscmd);
		
		error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;	
	}

	fp = fopen(SETTING_MEMORY_TEMP_FILE_PATH, "r");
	if (fp == NULL) {
		perror(SETTING_MEMORY_TEMP_FILE_PATH);
		
		error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;	
	}
 
 	char line[256] = {0};
 	fgets(line, sizeof(line), fp);
 	total_diskusage = strtoul(line, NULL, 10);
 
 	fclose(fp);

 	memset(syscmd, 0, sizeof(syscmd));
 	SNPRINTF(syscmd, sizeof(syscmd), "rm -f %s", SETTING_MEMORY_TEMP_FILE_PATH);
 	if (setting_system_command(syscmd) == -1)
 	{
  		EM_DEBUG_EXCEPTION("em_storage_mail_get_total_diskspace_usage :  [Setting > Memory] System Command [%s] is failed", syscmd);
		
  		error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
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


EXPORT_API int em_storage_test(int mail_id, int account_id, char *full_address_to, char *full_address_cc, char *full_address_bcc, int *err_code)
{
	DB_STMT hStmt = NULL;
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
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
		", ?"	/*  meeting_request_status */
		")");

	int transaction = true;
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
 
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bindStmtFieldDataInt(hStmt, MAIL_ID_IDX_IN_MAIL_TBL, mail_id);
	_bindStmtFieldDataInt(hStmt, ACCOUNT_ID_IDX_IN_MAIL_TBL, account_id);
	_bindStmtFieldDataString(hStmt, MAILBOX_NAME_IDX_IN_MAIL_TBL, "OUTBOX", 0, MAILBOX_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataInt(hStmt, MAILBOX_TYPE_IDX_IN_MAIL_TBL, EMF_MAILBOX_TYPE_OUTBOX);
	_bindStmtFieldDataString(hStmt, SUBJECT_IDX_IN_MAIL_TBL, "save test - long", 1, SUBJECT_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, DATETIME_IDX_IN_MAIL_TBL, "20100316052908", 0, DATETIME_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataInt(hStmt, SERVER_MAIL_STATUS_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataString(hStmt, SERVER_MAILBOX_NAME_IDX_IN_MAIL_TBL, "", 0, SERVER_MAILBOX_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, SERVER_MAIL_ID_IDX_IN_MAIL_TBL, "", 0, SERVER_MAIL_ID_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, MESSAGE_ID_IDX_IN_MAIL_TBL, "", 0, MESSAGE_ID_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, FULL_ADDRESS_FROM_IDX_IN_MAIL_TBL, "<test08@streaming.s3glab.net>", 1, FROM_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, FULL_ADDRESS_REPLY_IDX_IN_MAIL_TBL, "", 1, REPLY_TO_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, FULL_ADDRESS_TO_IDX_IN_MAIL_TBL, full_address_to, 1, TO_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, FULL_ADDRESS_CC_IDX_IN_MAIL_TBL, full_address_cc, 1, CC_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, FULL_ADDRESS_BCC_IDX_IN_MAIL_TBL, full_address_bcc, 1, BCC_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, FULL_ADDRESS_RETURN_IDX_IN_MAIL_TBL, "", 1, RETURN_PATH_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, EMAIL_ADDRESS_SENDER_IDX_IN_MAIL_TBL, "<sender_name@sender_host.com>", 1, FROM_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, EMAIL_ADDRESS_RECIPIENT_IDX_IN_MAIL_TBL, "<recipient_name@recipient_host.com>", 1, TO_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, ALIAS_SENDER_IDX_IN_MAIL_TBL, "send_alias", 1, FROM_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, ALIAS_RECIPIENT_IDX_IN_MAIL_TBL, "recipient_alias", 1, TO_EMAIL_ADDRESS_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataInt(hStmt, BODY_DOWNLOAD_STATUS_IDX_IN_MAIL_TBL, 1);
	_bindStmtFieldDataString(hStmt, FILE_PATH_PLAIN_IDX_IN_MAIL_TBL, "/opt/system/rsr/email/.emfdata/7/348/UTF-8", 0, TEXT_1_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, FILE_PATH_HTML_IDX_IN_MAIL_TBL, "", 0, TEXT_2_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataInt(hStmt, MAIL_SIZE_IDX_IN_MAIL_TBL, 4);
	_bindStmtFieldDataChar(hStmt, FLAGS_SEEN_FIELD_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataChar(hStmt, FLAGS_DELETED_FIELD_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataChar(hStmt, FLAGS_FLAGGED_FIELD_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataChar(hStmt, FLAGS_ANSWERED_FIELD_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataChar(hStmt, FLAGS_RECENT_FIELD_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataChar(hStmt, FLAGS_DRAFT_FIELD_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataChar(hStmt, FLAGS_FORWARDED_FIELD_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataInt(hStmt, DRM_STATUS_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataInt(hStmt, PRIORITY_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataInt(hStmt, SAVE_STATUS_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataInt(hStmt, LOCK_STATUS_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataInt(hStmt, REPORT_STATUS_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataInt(hStmt, ATTACHMENT_COUNT_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataInt(hStmt, INLINE_CONTENT_COUNT_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataInt(hStmt, ATTACHMENT_COUNT_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataInt(hStmt, THREAD_ID_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataInt(hStmt, THREAD_ITEM_COUNT_IDX_IN_MAIL_TBL, 0);
	_bindStmtFieldDataString(hStmt, PREVIEW_TEXT_IDX_IN_MAIL_TBL, "preview body", 1, PREVIEWBODY_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataInt(hStmt, MEETING_REQUEST_STATUS_IDX_IN_MAIL_TBL, 0);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	ret = true;
FINISH_OFF:
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG("sqlite3_finalize failed - %d", rc);
			
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_get_max_mail_count()
{
	return EM_STORAGE_MAIL_MAX;
}

EXPORT_API int em_storage_get_thread_id_of_thread_mails(emf_mail_tbl_t *mail_tbl, int *thread_id, int *result_latest_mail_id_in_thread, int *thread_item_count)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_PROFILE_BEGIN(profile_em_storage_get_thread_id_of_thread_mails);
	int rc = 0, query_size = 0, query_size_account = 0;
	int account_id;
	char *mailbox_name = NULL, *subject = NULL, *date_time = NULL;
	int err_code = EM_STORAGE_ERROR_NONE;
	char *sql_query_string = NULL, *sql_account = NULL;
	char *sql_format = "SELECT thread_id, date_time, mail_id FROM mail_tbl WHERE subject like \'%%%q\' AND mailbox_type NOT IN (3, 5, 7)";
	char *sql_format_account = " AND account_id = %d ";
	char *sql_format_order_by = " ORDER BY date_time DESC ";
	char **result = NULL, latest_date_time[18];
	char stripped_subject[1025];
	int count = 0, result_thread_id = -1, latest_mail_id_in_thread = -1;
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_IF_NULL_RETURN_VALUE(mail_tbl, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(thread_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(result_latest_mail_id_in_thread, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(thread_item_count, EMF_ERROR_INVALID_PARAM);

	if (mail_tbl->mailbox_type == EMF_MAILBOX_TYPE_TRASH ||
	   mail_tbl->mailbox_type == EMF_MAILBOX_TYPE_SPAMBOX ||
	   mail_tbl->mailbox_type == EMF_MAILBOX_TYPE_ALL_EMAILS) {
		EM_DEBUG_LOG("the mail in trash, spambox, all email could not be thread mail.");
		goto FINISH_OFF;
	}

	account_id = mail_tbl->account_id;
	mailbox_name = mail_tbl->mailbox_name;
	subject = mail_tbl->subject;
	date_time = mail_tbl->datetime;
	
	memset(latest_date_time, 0, 18);

	EM_DEBUG_LOG("subject : %s", subject);

	if (em_core_find_pos_stripped_subject_for_thread_view(subject, stripped_subject) != EMF_ERROR_NONE)	{
		EM_DEBUG_EXCEPTION("em_core_find_pos_stripped_subject_for_thread_view  is failed");
		err_code =  EM_STORAGE_ERROR_UNKNOWN;
		result_thread_id = -1;
		goto FINISH_OFF;
	}

	if (strlen(stripped_subject) < 2) {
		result_thread_id = -1;
		goto FINISH_OFF;
	}
	
	EM_DEBUG_LOG("em_core_find_pos_stripped_subject_for_thread_view returns[len = %d] = %s", strlen(stripped_subject), stripped_subject);
	
	if (account_id > 0) 	{
		query_size_account = 3 + strlen(sql_format_account);
		sql_account = malloc(query_size_account);
		if (sql_account == NULL) {
			EM_DEBUG_EXCEPTION("malloc for sql_account  is failed %d", query_size_account);
			err_code =  EM_STORAGE_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		snprintf(sql_account, query_size_account, sql_format_account, account_id);
	}
	
	query_size = strlen(sql_format) + strlen(stripped_subject) + 50 + query_size_account + strlen(sql_format_order_by); /*  + query_size_mailbox; */
	
	sql_query_string = malloc(query_size);
	
	if (sql_query_string == NULL) {
		EM_DEBUG_EXCEPTION("malloc for sql  is failed %d", query_size);
		err_code =  EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	sqlite3_snprintf(query_size, sql_query_string, sql_format, stripped_subject);
	/* snprintf(sql_query_string, query_size, sql_format, stripped_subject); */
	if (account_id > 0)
		strcat(sql_query_string, sql_account);

	strcat(sql_query_string, sql_format_order_by);
	strcat(sql_query_string, ";");

	EM_DEBUG_LOG("Query : %s", sql_query_string);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err_code = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG("Result rows count : %d", count);

	if (count == 0)
		result_thread_id = -1;
	else {
		_getTableFieldDataInt(result, &result_thread_id, 3);
		_getTableFieldDataStringWithoutAllocation(result, (char *)latest_date_time, 18, 1, 4);
		_getTableFieldDataInt(result, &latest_mail_id_in_thread, 5);

		if (mail_tbl->datetime && strcmp(latest_date_time, mail_tbl->datetime) > 0)
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
	
	EM_PROFILE_END(profile_em_storage_get_thread_id_of_thread_mails);
	
	return err_code;
}


EXPORT_API int em_storage_get_thread_information(int thread_id, emf_mail_tbl_t** mail_tbl, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int count = 0, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	emf_mail_tbl_t *p_data_tbl = NULL;
	char conditional_clause[QUERY_SIZE] = {0, };

	EM_IF_NULL_RETURN_VALUE(mail_tbl, false);

	SNPRINTF(conditional_clause, QUERY_SIZE, "WHERE thread_id = %d AND thread_item_count > 0", thread_id);
	EM_DEBUG_LOG("conditional_clause [%s]", conditional_clause);

	if(!em_storage_query_mail_tbl(conditional_clause, transaction, &p_data_tbl, &count, &error)) {
		EM_DEBUG_EXCEPTION("em_storage_query_mail_tbl failed [%d]", error);
		goto FINISH_OFF;
	}

	if(p_data_tbl)
		EM_DEBUG_LOG("thread_id : %d, thread_item_count : %d", p_data_tbl[0].thread_id, p_data_tbl[0].thread_item_count);
	
	ret = true;
	
FINISH_OFF:
	if (ret == true) 
		*mail_tbl = p_data_tbl;
	else if (p_data_tbl != NULL)
		em_storage_free_mail(&p_data_tbl, 1, NULL);
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_get_sender_list(int account_id, const char *mailbox_name, int search_type, const char *search_value, emf_sort_type_t sorting, emf_sender_list_t** sender_list, int *sender_count,  int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mailbox_name [%p], search_type [%d], search_value [%p], sorting [%d], sender_list[%p], sender_count[%p] err_code[%p]"
		, account_id , mailbox_name , search_type , search_value , sorting , sender_list, sender_count, err_code);
	
	if ((!sender_list) ||(!sender_count)) {
		EM_DEBUG_EXCEPTION("EM_STORAGE_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	int count = 0;
	int i, col_index = 0;
	int read_count = 0;
	emf_sender_list_t *p_sender_list = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char **result = NULL;
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"SELECT email_address_sender, alias_sender, COUNT(email_address_sender), SUM(flags_seen_field = 1) "
		"FROM mail_tbl ");

	/*  mailbox_name */
	if (mailbox_name)
		SNPRINTF(sql_query_string + strlen(sql_query_string), QUERY_SIZE-(strlen(sql_query_string)+1), " WHERE UPPER(mailbox_name) = UPPER('%s') ", mailbox_name);
	else	/*  NULL  means all mailbox_name. but except for trash(3), spambox(5), all emails(for GMail, 7) */
		SNPRINTF(sql_query_string + strlen(sql_query_string), QUERY_SIZE-(strlen(sql_query_string)+1), " WHERE mailbox_type not in (3, 5, 7) ");

	/*  account id */
	/*  '0' (ALL_ACCOUNT) means all account */
	if (account_id > ALL_ACCOUNT)
		SNPRINTF(sql_query_string + strlen(sql_query_string), QUERY_SIZE-(strlen(sql_query_string)+1), " AND account_id = %d ", account_id);

	if (search_value) {
		switch (search_type) {
			case EMF_SEARCH_FILTER_SUBJECT:
				SNPRINTF(sql_query_string + strlen(sql_query_string), QUERY_SIZE-(strlen(sql_query_string)+1),
					" AND (UPPER(subject) LIKE UPPER(\'%%%%%s%%%%\')) ", search_value);
				break;
			case EMF_SEARCH_FILTER_SENDER:
				SNPRINTF(sql_query_string + strlen(sql_query_string), QUERY_SIZE-(strlen(sql_query_string)+1),
					" AND  ((UPPER(full_address_from) LIKE UPPER(\'%%%%%s%%%%\')) "
					") ", search_value);
				break;
			case EMF_SEARCH_FILTER_RECIPIENT:
				SNPRINTF(sql_query_string + strlen(sql_query_string), QUERY_SIZE-(strlen(sql_query_string)+1),
					" AND ((UPPER(full_address_to) LIKE UPPER(\'%%%%%s%%%%\')) "	
					"	OR (UPPER(full_address_cc) LIKE UPPER(\'%%%%%s%%%%\')) "
					"	OR (UPPER(full_address_bcc) LIKE UPPER(\'%%%%%s%%%%\')) "
					") ", search_value, search_value, search_value);
				break;
			case EMF_SEARCH_FILTER_ALL:
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
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG("Count of Sender [%d]", count);
	
	if (!(p_sender_list = (emf_sender_list_t*)em_core_malloc(sizeof(emf_sender_list_t) * count))) {
		EM_DEBUG_EXCEPTION("em_core_malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	col_index = 4;

	EM_DEBUG_LOG(">>>> DATA ASSIGN START >>");	
	for (i = 0; i < count; i++)  {
		_getTableFieldDataString(result, &(p_sender_list[i].address), 1, col_index++);
		_getTableFieldDataString(result, &(p_sender_list[i].display_name), 1, col_index++);
		_getTableFieldDataInt(result, &(p_sender_list[i].total_count), col_index++);
		_getTableFieldDataInt(result, &(read_count), col_index++);
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
		em_storage_free_sender_list(&p_sender_list, count);
		
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_free_sender_list(emf_sender_list_t **sender_list, int count)
{
	EM_DEBUG_FUNC_BEGIN("sender_list[%p], count[%d]", sender_list, count);

	int err = EMF_ERROR_NONE;
	
	if (count > 0)  {
		if (!sender_list || !*sender_list)  {
			EM_DEBUG_EXCEPTION("sender_list[%p], count[%d]", sender_list, count);
			err = EMF_ERROR_INVALID_PARAM;
			return err;
		}
		
		emf_sender_list_t* p = *sender_list;
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

#ifdef _CONTACT_SUBSCRIBE_CHANGE_

#define MAX_BUFFER_LEN 					8096
#define DEBUG_EMAIL

#ifdef DEBUG_EMAIL
static void em_storage_contact_sync_print_email_list(int contact_id, char *display_name, GSList *email_list)
{
	EM_DEBUG_FUNC_BEGIN();

	int i;
	GSList *cursor = NULL;

	EM_DEBUG_LOG("====================================================================================");
	EM_DEBUG_LOG("Contact ID : [%d] Display Name : [%s]",  contact_id, display_name);
	EM_DEBUG_LOG("====================================================================================");
	EM_DEBUG_LOG(" order  email_type  email");
	EM_DEBUG_LOG("====================================================================================");
	i = 0;
	for (cursor = email_list; cursor; cursor = g_slist_next(cursor)) {
		EM_DEBUG_LOG("[%d] [%d] [%-35s]",
			i,
			contacts_svc_value_get_int(cursor->data, CTS_EMAIL_VAL_TYPE_INT),
			contacts_svc_value_get_str(cursor->data, CTS_EMAIL_VAL_ADDR_STR));
		i++;
	}	
	EM_DEBUG_LOG("====================================================================================");
}
#endif

EXPORT_API int em_storage_contact_sync_insert(int contact_id, char *display_name, GSList *email_list, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;

	int rc = 0;
	char address_list[MAX_BUFFER_LEN];
	int address_list_len = 0;
	int error;
		int index = 0;
	int is_first = 1;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	GSList *cursor;
	const char *email_address = NULL;
	char *display_name_in_mail_tbl = NULL;

	EM_DEBUG_LOG(" Contact ID : [%d] Display Name : [%s] email_list[%p]", contact_id, display_name, email_list);
	if (display_name == NULL || email_list == NULL) {
		EM_DEBUG_LOG(" Input Parameter is NULL");
		return false;
	}

#ifdef DEBUG_EMAIL
	em_storage_contact_sync_print_email_list(contact_id, display_name, email_list);
#endif

	/*  Make address list */
	cursor = email_list;
	is_first = 1;
	address_list_len = 0;
	for (;cursor; cursor = g_slist_next(cursor)) {
		email_address = contacts_svc_value_get_str(cursor->data, CTS_EMAIL_VAL_ADDR_STR);
		if (strlen(email_address) <= 0)
			continue;
		if (is_first) {
			is_first = 0;
			snprintf(address_list+address_list_len, sizeof(address_list) - (1 + address_list_len), "'%s' ", email_address);
			address_list_len = strlen(address_list);
		}
		else {
			snprintf(address_list+address_list_len, sizeof(address_list) - (1 + address_list_len), ", '%s' ", email_address);
			address_list_len = strlen(address_list);		
		}
	}
	EM_DEBUG_LOG("address list[%s]", address_list);

	int display_name_in_mail_tbl_len = strlen(display_name) + 4;
	display_name_in_mail_tbl = (char *) malloc(sizeof(char)*display_name_in_mail_tbl_len);
	if (display_name_in_mail_tbl == NULL) {
		EM_DEBUG_EXCEPTION("Out of memory");
		goto FINISH_OFF;
	}
	snprintf(display_name_in_mail_tbl, display_name_in_mail_tbl_len - 1, "\"%s\"", display_name);
	EM_DEBUG_LOG("display_name_in_mail_tbl[%s]", display_name_in_mail_tbl);
	
	/*  Update mail_tbl : from_contact_name, to_contact_name */
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	snprintf(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET from_contact_id = %d, from_contact_name = ? WHERE email_address_sender IN ( %s );", contact_id,  address_list);
	EM_DEBUG_LOG(" Query[%s]", sql_query_string);

	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	index = 0;
	_bindStmtFieldDataString(hStmt, index++, (char *)display_name_in_mail_tbl, 0, 0);
	
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			goto FINISH_OFF;
		}
	}	

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	snprintf(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET to_contact_id = %d, to_contact_name = ? WHERE email_address_recipient IN ( %s );", contact_id, address_list);
	EM_DEBUG_LOG(" Query[%s]", sql_query_string);

	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	index = 0;
	_bindStmtFieldDataString(hStmt, index++, (char *)display_name_in_mail_tbl, 0, 0);
	
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			goto FINISH_OFF;
		}
	}

	ret = true;
FINISH_OFF:
	EM_SAFE_FREE(display_name_in_mail_tbl);
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_contact_sync_update(int contact_id, char *display_name, GSList *email_list, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int rc = 0;
	char address_list[MAX_BUFFER_LEN];
	int address_list_len = 0;
	int error;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	int index = 0;
	int is_first = 1;
	int row, col;
	GSList *cursor;
	const char *email_address = NULL;
	char *display_name_in_mail_tbl = NULL;

	EM_DEBUG_LOG(" Contact ID : [%d] Display Name : [%s] email_list[%p]", contact_id, display_name, email_list);
	if (display_name == NULL || email_list == NULL) {
		EM_DEBUG_LOG(" Input Parameter is NULL");
		return false;
	}

#ifdef DEBUG_EMAIL
	em_storage_contact_sync_print_email_list(contact_id, display_name, email_list);
#endif

	/*  Make address list */
	cursor = email_list;
	is_first = 1;
	address_list_len = 0;
	for (;cursor; cursor = g_slist_next(cursor)) {
		email_address = contacts_svc_value_get_str(cursor->data, CTS_EMAIL_VAL_ADDR_STR);
		if (strlen(email_address) <= 0)
			continue;
		if (is_first) {
			is_first = 0;
			snprintf(address_list+address_list_len, sizeof(address_list) - (1 + address_list_len), "'%s' ", email_address);
			address_list_len = strlen(address_list);
		}
		else {
			snprintf(address_list+address_list_len, sizeof(address_list) - (1 + address_list_len), ", '%s' ", email_address);
			address_list_len = strlen(address_list);		
		}
	}
	EM_DEBUG_LOG("address list[%s]", address_list);

	int display_name_in_mail_tbl_len = strlen(display_name) + 4;
	display_name_in_mail_tbl = (char *) malloc(sizeof(char)*display_name_in_mail_tbl_len);
	if (display_name_in_mail_tbl == NULL) {
		EM_DEBUG_EXCEPTION("Out of memory");
		goto FINISH_OFF;
	}
	snprintf(display_name_in_mail_tbl, display_name_in_mail_tbl_len - 1, "\"%s\"", display_name);
	EM_DEBUG_LOG("display_name_in_mail_tbl[%s]", display_name_in_mail_tbl);


	/*  Update mail_tbl : deleted emails full_address_from contact */
	char **result2 = NULL;
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	snprintf(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET from_contact_id = -1, from_contact_name = alias_sender "
		" WHERE from_contact_id = %d "
		" AND email_address_sender NOT IN ( %s ); ",
		contact_id, address_list);
	EM_DEBUG_LOG(" Query[%s]", sql_query_string);
	rc = sqlite3_get_table(local_db_handle, sql_query_string, &result2, &row, &col, NULL);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	sqlite3_free_table(result2);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	snprintf(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET to_contact_id = -1, to_contact_name = alias_recipient "
		" WHERE to_contact_id = %d "
		" AND email_address_recipient NOT IN ( %s ); ",
		contact_id, address_list);
	EM_DEBUG_LOG(" Query[%s]", sql_query_string);
	rc = sqlite3_get_table(local_db_handle, sql_query_string, &result2, &row, &col, NULL);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	sqlite3_free_table(result2);

	/*  Update mail_tbl : inserted emails from contact */
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	snprintf(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET from_contact_name = ?, from_contact_id = %d WHERE email_address_sender IN ( %s ); ",
		contact_id, address_list);
	EM_DEBUG_LOG(" Query[%s]", sql_query_string);
	/*  rc = sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL);   */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	index = 0;
	_bindStmtFieldDataString(hStmt, index++, (char *)display_name_in_mail_tbl, 0, 0);

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			goto FINISH_OFF;
		}
	}	
	
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	snprintf(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET to_contact_name = ?, to_contact_id = %d WHERE email_address_recipient IN ( %s ); ",
		contact_id, address_list);
	EM_DEBUG_LOG(" Query[%s]", sql_query_string);
	/*  rc = sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL);   */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	index = 0;
	_bindStmtFieldDataString(hStmt, index++, (char *)display_name_in_mail_tbl, 0, 0);

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			goto FINISH_OFF;
		}
	}		

	ret = true;
FINISH_OFF:
	EM_SAFE_FREE(display_name_in_mail_tbl);
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_contact_sync_delete(int contact_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int rc;
	int ret = false;
	sqlite3_stmt* hStmt = NULL;
	int error;
	char sql_query_string[QUERY_SIZE] = {0, };

	/*  Update mail_tbl : set display name to alias */
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	snprintf(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET from_contact_id = -1,  from_contact_name = alias_sender  WHERE from_contact_id = %d", contact_id);
	EM_DEBUG_LOG(" Query[%s]", sql_query_string);
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (hStmt != NULL)  {	/*  finalize and reuse hStmt later */
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
		}
	}

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	snprintf(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET to_contact_id = -1, to_contact_name = alias_recipient  WHERE to_contact_id = %d", contact_id);
	EM_DEBUG_LOG(" Query[%s]", sql_query_string);
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (hStmt != NULL)  {	/*  finalize and reuse hStmt later */
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
		}
	}

	ret = true;
FINISH_OFF:
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

#endif /*  _CONTACT_SUBSCRIBE_CHANGE_ */


EXPORT_API int em_storage_free_address_info_list(emf_address_info_list_t **address_info_list)
{
	EM_DEBUG_FUNC_BEGIN("address_info_list[%p]", address_info_list);

	int err = EMF_ERROR_NONE;
	emf_address_info_t *p_address_info = NULL;
	GList *list = NULL;
	GList *node = NULL;
	int i = 0;
	
	if (!address_info_list || !*address_info_list)  {
		EM_DEBUG_EXCEPTION("address_info_list[%p]", address_info_list);
		err = EMF_ERROR_INVALID_PARAM;
		return err;
	}

	/*  delete GLists */
	for (i = EMF_ADDRESS_TYPE_FROM; i <= EMF_ADDRESS_TYPE_BCC; i++) {
		switch (i) {
			case EMF_ADDRESS_TYPE_FROM:
				list = (*address_info_list)->from;
				break;
			case EMF_ADDRESS_TYPE_TO:
				list = (*address_info_list)->to;
				break;
			case EMF_ADDRESS_TYPE_CC:
				list = (*address_info_list)->cc;
				break;
			case EMF_ADDRESS_TYPE_BCC:
				list = (*address_info_list)->bcc;
				break;
		}

		/*  delete dynamic-allocated memory for each item */
		node = g_list_first(list);
		while (node != NULL) {
			p_address_info = (emf_address_info_t*)node->data;
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

EXPORT_API int em_storage_add_pbd_activity(emf_event_partial_body_thd* local_activity, int *activity_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("local_activity[%p], activity_id[%p], transaction[%d], err_code[%p]", local_activity, activity_id, transaction, err_code);

	if (!local_activity || !activity_id) {
		EM_DEBUG_EXCEPTION("local_activity[%p], transaction[%d], activity_id[%p], err_code[%p]", local_activity, activity_id, transaction, err_code);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1;
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	int i = 0;

	char sql_query_string[QUERY_SIZE] = {0, };
	DB_STMT hStmt = NULL;
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_partial_body_activity_tbl VALUES "
		"( "
		"? "  /* Account ID */
		",?"  /* Local Mail ID */
		",?"  /* Server mail ID */
		",?"  /* Activity ID */
		",?"  /* Activity type*/
		",?"  /* Mailbox name*/
		") ");

	char *sql = "SELECT max(rowid) FROM mail_partial_body_activity_tbl;";
	char **result = NULL;

	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);  */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL==result[1]) rc = 1;
	else rc = atoi(result[1])+1;
	sqlite3_free_table(result);
	result = NULL;

	*activity_id = local_activity->activity_id = rc;

	EM_DEBUG_LOG(">>>>> ACTIVITY ID [ %d ], MAIL ID [ %d ], ACTIVITY TYPE [ %d ], SERVER MAIL ID [ %lu ]", \
		local_activity->activity_id, local_activity->mail_id, local_activity->activity_type, local_activity->server_mail_id);

	if (local_activity->mailbox_name)
		EM_DEBUG_LOG(" MAILBOX NAME [ %s ]", local_activity->mailbox_name);
	
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG(">>>> SQL STMT [ %s ]", sql_query_string);
	
	_bindStmtFieldDataInt(hStmt, i++, local_activity->account_id);
	_bindStmtFieldDataInt(hStmt, i++, local_activity->mail_id);
	_bindStmtFieldDataInt(hStmt, i++, local_activity->server_mail_id);
	_bindStmtFieldDataInt(hStmt, i++, local_activity->activity_id);
	_bindStmtFieldDataInt(hStmt, i++, local_activity->activity_type);
	_bindStmtFieldDataString(hStmt, i++ , (char *)local_activity->mailbox_name, 0, 3999);

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);

	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d, errmsg = %s.", rc, sqlite3_errmsg(local_db_handle)));
	
	ret = true;

FINISH_OFF:

	if (hStmt != NULL) {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	 EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	 if (err_code != NULL)
		 *err_code = error;

	 EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_get_pbd_mailbox_list(int account_id, char *** mailbox_list, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_list[%p], count[%p] err_code[%p]", account_id, mailbox_list, count, err_code);

	if (account_id < FIRST_ACCOUNT_ID || NULL == &mailbox_list || NULL == count) {
		EM_DEBUG_EXCEPTION("account_id[%d], mailbox_list[%p], count[%p] err_code[%p]", account_id, mailbox_list, count, err_code);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char **result;
	int i = 0, rc = -1;
	char **mbox_list = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	
	EM_STORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(distinct mailbox_name) FROM mail_partial_body_activity_tbl WHERE account_id = %d order by mailbox_name", account_id);

	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);

	if (!*count) {
		EM_DEBUG_EXCEPTION(" no mailbox_name found...");
		error = EM_STORAGE_ERROR_MAILBOX_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("Mailbox count = %d", *count);
	
	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	/* SNPRINTF(g_sql_query, sizeof(g_sql_query), "SELECT distinct mailbox_name FROM mail_partial_body_activity_tbl WHERE account_id = %d order by activity_id", account_id); */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT distinct mailbox_name FROM mail_partial_body_activity_tbl WHERE account_id = %d order by mailbox_name", account_id);

	EM_DEBUG_LOG(" Query [%s]", sql_query_string);
	
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG(" Bbefore sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	if (NULL == (mbox_list = (char **)em_core_malloc(sizeof(char *) * (*count)))) {
		EM_DEBUG_EXCEPTION(" em_core_malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(mbox_list, 0x00, sizeof(char *) * (*count));
	
	for (i = 0; i < (*count); i++) {
		_getStmtFieldDataString(hStmt, &mbox_list[i], 0, ACCOUNT_IDX_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		/*  rc = sqlite3_step(hStmt); */
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		/* EM_DEBUG_LOG("In em_storage_get_pdb_mailbox_list() loop, After sqlite3_step(), , i = %d, rc = %d.", i,  rc); */
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
		EM_DEBUG_LOG("mbox_list %s", mbox_list[i]);
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
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_get_pbd_account_list(int **account_list, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_list[%p], count[%p] err_code[%p]", account_list, count, err_code);

	if (NULL == &account_list || NULL == count) {
		EM_DEBUG_EXCEPTION("mailbox_list[%p], count[%p] err_code[%p]", account_list, count, err_code);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char *sql;
	char **result;
	int i = 0, rc = -1;
	int *result_account_list = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	
	EM_STORAGE_START_READ_TRANSACTION(transaction);


	sql = "SELECT count(distinct account_id) FROM mail_partial_body_activity_tbl";	

	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL);  */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);

	if (!*count) {
		EM_DEBUG_EXCEPTION("no account found...");
		error = EM_STORAGE_ERROR_MAILBOX_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("Account count [%d]", *count);
	
	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT distinct account_id FROM mail_partial_body_activity_tbl");

	EM_DEBUG_LOG("Query [%s]", sql_query_string);
	
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG("Before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	if (NULL == (result_account_list = (int *)em_core_malloc(sizeof(int) * (*count)))) {
		EM_DEBUG_EXCEPTION(" em_core_malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(result_account_list, 0x00, sizeof(int) * (*count));
	
	for (i = 0; i < (*count); i++) {
		_getStmtFieldDataInt(hStmt, result_account_list + i, 0);
		/*  rc = sqlite3_step(hStmt); */
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
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
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}



EXPORT_API int em_storage_get_pbd_activity_data(int account_id, char *mailbox_name, emf_event_partial_body_thd** event_start, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], event_start[%p], err_code[%p]", account_id, event_start, err_code);

	if (account_id < FIRST_ACCOUNT_ID || NULL == event_start || NULL == mailbox_name || NULL == count) {
		EM_DEBUG_EXCEPTION("account_id[%d], emf_event_partial_body_thd[%p], mailbox_name[%p], count[%p], err_code[%p]", account_id, event_start, mailbox_name, count, err_code);

		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1;
	int ret = false;
	char **result;
	int error = EM_STORAGE_ERROR_NONE;
	int i = 0;
	DB_STMT hStmt = NULL;
	emf_event_partial_body_thd* event_list = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	
	EM_STORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(*) FROM mail_partial_body_activity_tbl WHERE account_id = %d AND mailbox_name = '%s' order by activity_id", account_id, mailbox_name);
	
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);

	EM_DEBUG_LOG("Query = [%s]", sql_query_string);

	if (!*count) {
		EM_DEBUG_EXCEPTION("No matched activity found in mail_partial_body_activity_tbl");

		error = EM_STORAGE_ERROR_MAIL_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("Activity Count = %d", *count);
	
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_partial_body_activity_tbl WHERE account_id = %d AND mailbox_name = '%s' order by activity_id", account_id, mailbox_name);

	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG(" Bbefore sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	if (!(event_list = (emf_event_partial_body_thd*)em_core_malloc(sizeof(emf_event_partial_body_thd) * (*count)))) {
		EM_DEBUG_EXCEPTION("Malloc failed");

		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memset(event_list, 0x00, sizeof(emf_event_partial_body_thd) * (*count));
	
	for (i=0; i < (*count); i++) {
		_getStmtFieldDataInt(hStmt, &(event_list[i].account_id), ACCOUNT_IDX_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_getStmtFieldDataInt(hStmt, &(event_list[i].mail_id), MAIL_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_getStmtFieldDataInt(hStmt, (int *)&(event_list[i].server_mail_id), SERVER_MAIL_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_getStmtFieldDataInt(hStmt, &(event_list[i].activity_id), ACTIVITY_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_getStmtFieldDataInt(hStmt, &(event_list[i].activity_type), ACTIVITY_TYPE_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_getStmtFieldDataString(hStmt, &(event_list[i].mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		
		/*  rc = sqlite3_step(hStmt); */
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		/* EM_DEBUG_LOG("In em_storage_get_pbd_activity_data() loop, After sqlite3_step(), , i = %d, rc = %d.", i,  rc); */
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));

		event_list[i].event_type = 0;
	}

	ret = true;

FINISH_OFF:
	if (true == ret)
	  *event_start = event_list;
	else {
		if (event_list) {
			for (i=0; i < (*count); i++)
				EM_SAFE_FREE(event_list[i].mailbox_name);
			EM_SAFE_FREE(event_list);
		}
	}

	if (hStmt != NULL) {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_LOG("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}



EXPORT_API int em_storage_delete_pbd_activity(int account_id, int mail_id, int activity_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d] , activity_id[%d], transaction[%d], err_code[%p]", account_id, mail_id, activity_id, transaction, err_code);


	if (account_id < FIRST_ACCOUNT_ID || activity_id < 0 || mail_id <= 0) {
		EM_DEBUG_EXCEPTION("account_id[%d], mail_id[%d], activity_id[%d], transaction[%d], err_code[%p]", account_id, mail_id, activity_id, transaction, err_code);

		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1;
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	
	if (activity_id == 0)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_partial_body_activity_tbl WHERE account_id = %d AND mail_id = %d", account_id, mail_id);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_partial_body_activity_tbl WHERE account_id = %d AND activity_id = %d", account_id, activity_id);

	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  validate activity existence */
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION("No matching activity found");
		error = EM_STORAGE_ERROR_DATA_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	
	_DISCONNECT_DB;
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_mailbox_pbd_activity_count(int account_id, char *mailbox_name, int *activity_count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], activity_count[%p], err_code[%p]", account_id, activity_count, err_code);

	if (account_id < FIRST_ACCOUNT_ID || NULL == activity_count || NULL == err_code) {
		EM_DEBUG_EXCEPTION("account_id[%d], activity_count[%p], err_code[%p]", account_id, activity_count, err_code);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	int rc = -1;
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	DB_STMT hStmt = NULL;

	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_READ_TRANSACTION(transaction);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(*) FROM mail_partial_body_activity_tbl WHERE account_id = %d and mailbox_name = '%s'", account_id, mailbox_name);

	EM_DEBUG_LOG(" Query [%s]", sql_query_string);

	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	_getStmtFieldDataInt(hStmt, activity_count, 0);

	EM_DEBUG_LOG("No. of activities in activity table [%d]", *activity_count);

	ret = true;

FINISH_OFF:

	if (hStmt != NULL) {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);
		rc = sqlite3_finalize(hStmt);
		hStmt=NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
		EM_DEBUG_LOG("sqlite3_finalize- %d", rc);
	}

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_get_pbd_activity_count(int *activity_count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("activity_count[%p], err_code[%p]", activity_count, err_code);

	if (NULL == activity_count || NULL == err_code) {
		EM_DEBUG_EXCEPTION("activity_count[%p], err_code[%p]", activity_count, err_code);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	int rc = -1;
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_READ_TRANSACTION(transaction);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(*) FROM mail_partial_body_activity_tbl;");

	EM_DEBUG_LOG(" Query [%s]", sql_query_string);

	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("  before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	_getStmtFieldDataInt(hStmt, activity_count, 0);

	EM_DEBUG_LOG("No. of activities in activity table [%d]", *activity_count);

	ret = true;

FINISH_OFF:


	if (hStmt != NULL) {
		EM_DEBUG_LOG("Before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		hStmt=NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
		EM_DEBUG_LOG("sqlite3_finalize- %d", rc);
	}

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_delete_full_pbd_activity_data(int account_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], transaction[%d], err_code[%p]", account_id, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID) {
		EM_DEBUG_EXCEPTION("account_id[%d], transaction[%d], err_code[%p]", account_id, transaction, err_code);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1;
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_partial_body_activity_tbl WHERE account_id = %d", account_id);

	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION("No matching activities found in mail_partial_body_activity_tbl");
		error = EM_STORAGE_ERROR_DATA_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/*Himanshu[h.gahlaut]-> Added below API to update mail_partial_body_activity_tbl
if a mail is moved before its partial body is downloaded.Currently not used but should be used if mail move from server is enabled*/

EXPORT_API int em_storage_update_pbd_activity(char *old_server_uid, char *new_server_uid, char *mbox_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("old_server_uid[%s], new_server_uid[%s], mbox_name[%s]", old_server_uid, new_server_uid, mbox_name);

	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	int transaction = true;

	if (!old_server_uid || !new_server_uid || !mbox_name)  {
		EM_DEBUG_EXCEPTION("Invalid parameters");
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		 "UPDATE mail_partial_body_activity_tbl SET server_mail_id = %s , mailbox_name=\'%s\' WHERE server_mail_id = %s ", new_server_uid, mbox_name, old_server_uid);

	EM_DEBUG_LOG("Query [%s]", sql_query_string);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION("No matching found in mail_partial_body_activity_tbl");
		error = EM_STORAGE_ERROR_DATA_NOT_FOUND;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_create_file(char *data_string, size_t file_size, char *dst_file_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("file_size[%d] , dst_file_name[%s], err_code[%p]", file_size, dst_file_name, err_code);
	
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	
	FILE* fp_dst = NULL;
	
	if (!data_string || !dst_file_name)  {				
		EM_DEBUG_EXCEPTION("data_string[%p], dst_file_name[%p]", data_string, dst_file_name);
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	fp_dst = fopen(dst_file_name, "w");
	
	if (!fp_dst)  {
		EM_DEBUG_EXCEPTION("fopen failed - %s", dst_file_name);
		if (errno == 28)
			error = EMF_ERROR_MAIL_MEMORY_FULL;
		else
			error = EM_STORAGE_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;			
	}
	
	if (fwrite(data_string, 1, file_size, fp_dst) < 0) {
		EM_DEBUG_EXCEPTION("fwrite failed...");
		error = EM_STORAGE_ERROR_UNKNOWN;
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

EXPORT_API int em_storage_update_read_mail_uid_by_server_uid(char *old_server_uid, char *new_server_uid, char *mbox_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int rc = -1;				   
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	int transaction = true;
		
	if (!old_server_uid || !new_server_uid || !mbox_name)  {		
		EM_DEBUG_EXCEPTION("Invalid parameters");
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	EM_DEBUG_LOG("old_server_uid[%s], new_server_uid[%s], mbox_name[%s]", old_server_uid, new_server_uid, mbox_name);
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);

	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		 "UPDATE mail_read_mail_uid_tbl SET s_uid=\'%s\' , local_mbox=\'%s\', mailbox_name=\'%s\' WHERE s_uid=%s ", new_server_uid, mbox_name, mbox_name, old_server_uid);

	 EM_DEBUG_LOG(" Query [%s]", sql_query_string);
	 
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	 EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		 ("sqlite3_exec fail:%d", rc));
	 EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		 ("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	 
	 
	 rc = sqlite3_changes(local_db_handle);
	 if (rc == 0)
	 {
		 EM_DEBUG_EXCEPTION("No matching found in mail_partial_body_activity_tbl");
		 error = EM_STORAGE_ERROR_DATA_NOT_FOUND;
		 goto FINISH_OFF;
	 }

	  
	 ret = true;
	  
FINISH_OFF:
	  
  	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
 	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
	
}


/**
 * @fn em_storage_get_id_set_from_mail_ids(int mail_ids[], int mail_id_count, emf_id_set_t** server_uids, int *id_set_count, int *err_code);
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
 
EXPORT_API int em_storage_get_id_set_from_mail_ids(char *mail_ids, emf_id_set_t** idset, int *id_set_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_PROFILE_BEGIN(EmStorageGetIdSetFromMailIds);

	int error = EM_STORAGE_ERROR_NONE;
	int ret = false;
	emf_id_set_t* p_id_set = NULL;
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
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	sqlite3 *local_db_handle = em_storage_get_db_connection();

	SNPRINTF(sql_query_string, space_left_in_query_buffer, "SELECT local_uid, s_uid FROM mail_read_mail_uid_tbl WHERE local_uid in (%s) ORDER BY s_uid", mail_ids);

	EM_DEBUG_LOG("SQL Query formed [%s] ", sql_query_string);
	
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG(" Count of mails [%d ]", count);

	if (count <= 0) {
		EM_DEBUG_EXCEPTION("Can't find proper mail");
		error = EM_STORAGE_ERROR_DATA_NOT_FOUND;
		goto FINISH_OFF;
	}


	if (NULL == (p_id_set = (emf_id_set_t*)em_core_malloc(sizeof(emf_id_set_t) * count)))  {
		EM_DEBUG_EXCEPTION(" em_core_malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	col_index = 2;

	EM_PROFILE_BEGIN(EmStorageGetIdSetFromMailIds_Loop);
	EM_DEBUG_LOG(">>>> DATA ASSIGN START");	
	for (i = 0; i < count; i++)  {
		_getTableFieldDataInt(result, &(p_id_set[i].mail_id), col_index++);
		_getTableFieldDataString(result, &server_mail_id, 1, col_index++);
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

EXPORT_API int em_storage_delete_triggers_from_lucene()
{
	EM_DEBUG_FUNC_BEGIN();
	int rc, ret = true, transaction = true;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP TRIGGER triggerDelete;");
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP TRIGGER triggerInsert;");
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP TRIGGER triggerUpdate;");
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	ret = true;
	
FINISH_OFF:

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);

	_DISCONNECT_DB;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_filter_mails_by_rule(int account_id, char *dest_mailbox_name, emf_mail_rule_tbl_t *rule, int ** filtered_mail_id_list, int *count_of_mails, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], dest_mailbox_name[%p] rule[%p], filtered_mail_id_list[%p], count_of_mails[%p], err_code[%p]", account_id, dest_mailbox_name, rule, filtered_mail_id_list, count_of_mails, err_code);
	
	if ((account_id <= 0) || (!dest_mailbox_name) || (!rule) || (!rule->value)|| (!filtered_mail_id_list) || (!count_of_mails)) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1, ret = false, error = EM_STORAGE_ERROR_NONE;
	int count = 0, col_index = 0, i = 0, where_pararaph_length = 0, *mail_list = NULL;
	char **result = NULL, *where_pararaph = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mail_id FROM mail_tbl ");
	
	EM_DEBUG_LOG("rule->value [%s]", rule->value);

	where_pararaph_length = strlen(rule->value) + 100;
	where_pararaph = malloc(sizeof(char) * where_pararaph_length);

	if (where_pararaph == NULL) {
		EM_DEBUG_EXCEPTION("malloc failed for where_pararaph.");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(where_pararaph, 0, sizeof(char) * where_pararaph_length);
	
	EM_DEBUG_LOG("rule->type [%d], rule->flag2[%d]", rule->type, rule->flag2);

	if (rule->type == EMF_FILTER_FROM) {
		if (rule->flag2 == RULE_TYPE_INCLUDES)
			SNPRINTF(where_pararaph, where_pararaph_length, "WHERE account_id = %d AND mailbox_type NOT in (3,5) AND full_address_from like \'%%%s%%\'", account_id, rule->value);
		else /*  RULE_TYPE_EXACTLY */
			SNPRINTF(where_pararaph, where_pararaph_length, "WHERE account_id = %d AND mailbox_type NOT in (3,5) AND full_address_from = \'%s\'", account_id, rule->value);
	}
	else if (rule->type == EMF_FILTER_SUBJECT) {
		if (rule->flag2 == RULE_TYPE_INCLUDES)
			SNPRINTF(where_pararaph, where_pararaph_length, "WHERE account_id = %d AND mailbox_type NOT in (3,5) AND subject like \'%%%s%%\'", account_id, rule->value);
		else /*  RULE_TYPE_EXACTLY */
			SNPRINTF(where_pararaph, where_pararaph_length, "WHERE account_id = %d AND mailbox_type NOT in (3,5) AND subject = \'%s\'", account_id, rule->value);
	}
	else {
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("rule->type is invald");
		goto FINISH_OFF;
	}
	
	if (strlen(sql_query_string) + strlen(where_pararaph) < QUERY_SIZE)
		strcat(sql_query_string, where_pararaph);

	EM_DEBUG_LOG("query[%s]", sql_query_string);
	
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG("Count of mails [%d]", count);

	if (count) {
		mail_list = malloc(sizeof(int) * count);
		if (mail_list == NULL) {
			EM_DEBUG_EXCEPTION("malloc failed for mail_list.");
			error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		col_index = 1;

		for (i = 0; i < count; i++) 
			_getTableFieldDataInt(result, &(mail_list[i]), col_index++);

		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET mailbox_name = \'%s\', mailbox_type = 5 ", dest_mailbox_name);

		if (strlen(sql_query_string) + strlen(where_pararaph) < QUERY_SIZE)
		strcat(sql_query_string, where_pararaph);

		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
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

#define EMF_SLOT_UNIT 25

EXPORT_API int em_storage_set_mail_slot_size(int account_id, char *mailbox_name, int new_slot_size, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%p] new_slot_size[%d], err_code[%p]", account_id, mailbox_name, new_slot_size, err_code);
	int rc = -1, ret = false, err = EM_STORAGE_ERROR_NONE;
	int where_pararaph_length = 0;
	char *where_pararaph = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	int and = 0;
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, err);

	if (new_slot_size > 0)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_box_tbl SET mail_slot_size = %d ", new_slot_size);
	else if (new_slot_size == 0)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_box_tbl SET mail_slot_size = mail_slot_size + %d ", EMF_SLOT_UNIT);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_box_tbl SET mail_slot_size = mail_slot_size + %d ", new_slot_size * -1);


	if (mailbox_name)
		where_pararaph_length = strlen(mailbox_name) + 80;
	else
		where_pararaph_length = 50;

	if (new_slot_size == 0)
		where_pararaph_length += 70;

	where_pararaph = malloc(sizeof(char) * where_pararaph_length);
	if (where_pararaph == NULL) {
		EM_DEBUG_EXCEPTION("Memory allocation failed for where_pararaph");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memset(where_pararaph, 0x00, where_pararaph_length);

	if (account_id > ALL_ACCOUNT) {
		and = 1;
		if (mailbox_name)
			SNPRINTF(where_pararaph, where_pararaph_length, "WHERE account_id = %d AND mailbox_name = '%s' ", account_id, mailbox_name);
		else
			SNPRINTF(where_pararaph, where_pararaph_length, "WHERE account_id = %d ", account_id);
	}

	if (new_slot_size == 0)
		SNPRINTF(where_pararaph + strlen(where_pararaph), where_pararaph_length - strlen(where_pararaph), " %s total_mail_count_on_server > mail_slot_size ", (and ? "AND" : "WHERE"));

	if (strlen(sql_query_string) + strlen(where_pararaph) < QUERY_SIZE)
		strcat(sql_query_string, where_pararaph);
	else {
		EM_DEBUG_EXCEPTION("Query buffer overflowed !!!");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;			
	}

	EM_DEBUG_LOG("query[%s]", sql_query_string);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
	("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	ret = true;
	
FINISH_OFF:

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, err);

	_DISCONNECT_DB;

	EM_SAFE_FREE(where_pararaph);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_add_meeting_request(int account_id, char *mailbox_name, emf_meeting_request_t* meeting_req, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%s], meeting_req[%p], transaction[%d], err_code[%p]", account_id, mailbox_name, meeting_req, transaction, err_code);
	
	if (!meeting_req || meeting_req->mail_id <= 0) {
		if (meeting_req)
		EM_DEBUG_EXCEPTION("mail_id[%]d", meeting_req->mail_id);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;

		return false;
	}
	
	int rc = -1;
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	int col_index = 0;
	time_t temp_unix_time = 0;
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_meeting_tbl VALUES "
		"( ?"		/*  mail_id */
		", ?"		/*  account_id */
		", ?"		/*  mailbox_name */
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
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	if (rc != SQLITE_OK)  {
		EM_DEBUG_LOG(" before sqlite3_prepare hStmt = %p", hStmt);
		EM_DEBUG_EXCEPTION("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle));
		
		error = EM_STORAGE_ERROR_DB_FAILURE;
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
	_bindStmtFieldDataInt(hStmt, col_index++, meeting_req->mail_id);
	_bindStmtFieldDataInt(hStmt, col_index++, account_id);
	_bindStmtFieldDataString(hStmt, col_index++, (char *)mailbox_name, 0, MAILBOX_LEN_IN_MAIL_MEETING_TBL);
	_bindStmtFieldDataInt(hStmt, col_index++, meeting_req->meeting_response);

	temp_unix_time = timegm(&(meeting_req->start_time));
	_bindStmtFieldDataInt(hStmt, col_index++, temp_unix_time);
	temp_unix_time = timegm(&(meeting_req->end_time));
	_bindStmtFieldDataInt(hStmt, col_index++, temp_unix_time);

	_bindStmtFieldDataString(hStmt, col_index++, (char *)meeting_req->location, 0, LOCATION_LEN_IN_MAIL_MEETING_TBL);
	_bindStmtFieldDataString(hStmt, col_index++, (char *)meeting_req->global_object_id, 0, GLOBAL_OBJECT_ID_LEN_IN_MAIL_MEETING_TBL);
	
	_bindStmtFieldDataInt(hStmt, col_index++, meeting_req->time_zone.offset_from_GMT);
	_bindStmtFieldDataString(hStmt, col_index++, (char *)meeting_req->time_zone.standard_name, 0, STANDARD_NAME_LEN_IN_MAIL_MEETING_TBL);
	temp_unix_time = timegm(&(meeting_req->time_zone.standard_time_start_date));
	_bindStmtFieldDataInt(hStmt, col_index++, temp_unix_time);
	_bindStmtFieldDataInt(hStmt, col_index++, meeting_req->time_zone.standard_bias);

	_bindStmtFieldDataString(hStmt, col_index++, (char *)meeting_req->time_zone.daylight_name, 0, DAYLIGHT_NAME_LEN_IN_MAIL_MEETING_TBL);
	temp_unix_time = timegm(&(meeting_req->time_zone.daylight_time_start_date));
	_bindStmtFieldDataInt(hStmt, col_index++, temp_unix_time);
	_bindStmtFieldDataInt(hStmt, col_index++, meeting_req->time_zone.daylight_bias);

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}
 
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_get_meeting_request(int mail_id, emf_meeting_request_t ** meeting_req, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int count = 0;
	int rc = -1;
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	emf_meeting_request_t *p_temp_meeting_req = NULL;
	int col_index = 0;
	time_t temp_unix_time;

	EM_IF_NULL_RETURN_VALUE(meeting_req, false);

	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"SELECT  mail_id, meeting_response, start_time, end_time, location, global_object_id, offset, standard_name, standard_time_start_date, standard_bias, daylight_name, daylight_time_start_date, daylight_bias "
		" FROM mail_meeting_tbl "
		" WHERE mail_id = %d", mail_id);
	EM_DEBUG_LOG("sql : %s ", sql_query_string);

	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	
	if (rc == SQLITE_DONE)  {
		EM_DEBUG_EXCEPTION(" no Meeting request found...");
		count = 0;
		ret = false;
		error= EM_STORAGE_ERROR_DATA_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	if (!(p_temp_meeting_req = (emf_meeting_request_t*)malloc(sizeof(emf_meeting_request_t)))) {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(p_temp_meeting_req, 0x00, sizeof(emf_meeting_request_t));
		
	col_index = 0;

	_getStmtFieldDataInt(hStmt, &(p_temp_meeting_req->mail_id), col_index++);
	_getStmtFieldDataInt(hStmt, (int *)&(p_temp_meeting_req->meeting_response), col_index++);
	_getStmtFieldDataInt(hStmt, (int *)(&temp_unix_time), col_index++);
	gmtime_r(&temp_unix_time, &(p_temp_meeting_req->start_time));
	_getStmtFieldDataInt(hStmt, (int *)(&temp_unix_time), col_index++);
	gmtime_r(&temp_unix_time, &(p_temp_meeting_req->end_time));
	_getStmtFieldDataString(hStmt, &p_temp_meeting_req->location, 1, col_index++);
	_getStmtFieldDataStringWithoutAllocation(hStmt, p_temp_meeting_req->global_object_id, GLOBAL_OBJECT_ID_LEN_IN_MAIL_MEETING_TBL, 1, col_index++);
	_getStmtFieldDataInt(hStmt, &(p_temp_meeting_req->time_zone.offset_from_GMT), col_index++);

	_getStmtFieldDataStringWithoutAllocation(hStmt, p_temp_meeting_req->time_zone.standard_name, STANDARD_NAME_LEN_IN_MAIL_MEETING_TBL, 1, col_index++);
	_getStmtFieldDataInt(hStmt, (int *)(&temp_unix_time), col_index++);
	gmtime_r(&temp_unix_time, &(p_temp_meeting_req->time_zone.standard_time_start_date));
	_getStmtFieldDataInt(hStmt, &(p_temp_meeting_req->time_zone.standard_bias), col_index++);

	_getStmtFieldDataStringWithoutAllocation(hStmt, p_temp_meeting_req->time_zone.daylight_name, DAYLIGHT_NAME_LEN_IN_MAIL_MEETING_TBL, 1, col_index++);
	_getStmtFieldDataInt(hStmt, (int *)(&temp_unix_time), col_index++);
	gmtime_r(&temp_unix_time, &(p_temp_meeting_req->time_zone.daylight_time_start_date));
	_getStmtFieldDataInt(hStmt, &(p_temp_meeting_req->time_zone.daylight_bias), col_index++);

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
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_update_meeting_request(emf_meeting_request_t* meeting_req, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("meeting_req[%p], transaction[%d], err_code[%p]", meeting_req, transaction, err_code);
	
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	int rc;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	time_t temp_unix_time = 0;

	if (!meeting_req) {
		EM_DEBUG_EXCEPTION("Invalid Parameter!");
		error = EM_STORAGE_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);

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
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
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

	_bindStmtFieldDataInt(hStmt, col_index++, meeting_req->meeting_response);
	temp_unix_time = timegm(&(meeting_req->start_time));
	_bindStmtFieldDataInt(hStmt, col_index++, temp_unix_time);
	temp_unix_time = timegm(&(meeting_req->end_time));
	_bindStmtFieldDataInt(hStmt, col_index++, temp_unix_time);
	_bindStmtFieldDataString(hStmt, col_index++, meeting_req->location, 1, LOCATION_LEN_IN_MAIL_MEETING_TBL);
	_bindStmtFieldDataString(hStmt, col_index++, meeting_req->global_object_id, 1, GLOBAL_OBJECT_ID_LEN_IN_MAIL_MEETING_TBL);
	_bindStmtFieldDataInt(hStmt, col_index++, meeting_req->time_zone.offset_from_GMT);
	_bindStmtFieldDataString(hStmt, col_index++, meeting_req->time_zone.standard_name, 1, STANDARD_NAME_LEN_IN_MAIL_MEETING_TBL);
	temp_unix_time = timegm(&(meeting_req->time_zone.standard_time_start_date));
	_bindStmtFieldDataInt(hStmt, col_index++, temp_unix_time);
	_bindStmtFieldDataInt(hStmt, col_index++, meeting_req->time_zone.standard_bias);
	_bindStmtFieldDataString(hStmt, col_index++, meeting_req->time_zone.daylight_name, 1, DAYLIGHT_NAME_LEN_IN_MAIL_MEETING_TBL);
	temp_unix_time = timegm(&(meeting_req->time_zone.daylight_time_start_date));
	_bindStmtFieldDataInt(hStmt, col_index++, temp_unix_time);
	_bindStmtFieldDataInt(hStmt, col_index++, meeting_req->time_zone.daylight_bias);

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	ret = true;


FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	
	if (hStmt != NULL)  {
		EM_DEBUG_LOG("before sqlite3_finalize hStmt = %p", hStmt);

		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);

			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;	
}


EXPORT_API int em_storage_change_meeting_request_field(int account_id, char *mailbox_name, emf_mail_change_type_t type, emf_meeting_request_t* meeting_req, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%s], type[%d], meeting_req[%p], transaction[%d], err_code[%p]", account_id, mailbox_name,  type, meeting_req, transaction, err_code);
	
	if (account_id <= 0 || !meeting_req || meeting_req->mail_id <= 0) {
		if (meeting_req)
			EM_DEBUG_EXCEPTION("mail_id[%d]", meeting_req->mail_id);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;;
	}

	int rc;
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;	
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	int col_index = 0;
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	switch (type)  {
		case UPDATE_MAILBOX:
			EM_DEBUG_LOG(">>>>> UPDATE_MAILBOX : Move");
			if (!mailbox_name)  {
				EM_DEBUG_EXCEPTION(" mailbox_name[%p]", mailbox_name);
				error = EM_STORAGE_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_meeting_tbl SET mailbox_name = ? WHERE mail_id = %d", meeting_req->mail_id);
 
			
			EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_LOG(" Before sqlite3_prepare hStmt = %p", hStmt);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

			_bindStmtFieldDataString(hStmt, col_index++, (char *)mailbox_name, 0, MAILBOX_LEN_IN_MAIL_MEETING_TBL);
			break;

		default:
			EM_DEBUG_LOG("type[%d]", type);
			
			error = EM_STORAGE_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}
	EM_DEBUG_LOG("Query = [%s]", sql_query_string);

	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	ret = true;
	
FINISH_OFF:
	if (hStmt != NULL)  {
		EM_DEBUG_LOG(" Before sqlite3_finalize hStmt = %p", hStmt);
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_EXCEPTION(" sqlite3_finalize failed - %d", rc);
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}
	}

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_change_meeting_request_mailbox(int account_id, char *old_mailbox_name, char *new_mailbox_name, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], old_mailbox_name[%p], new_mailbox_name[%p], transaction[%d], err_code[%p]", account_id, old_mailbox_name, new_mailbox_name, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID || !old_mailbox_name || !new_mailbox_name)  {
		EM_DEBUG_EXCEPTION("account_id[%d], old_mailbox_name[%p], new_mailbox_name[%p]", account_id, old_mailbox_name, new_mailbox_name);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc;
	int ret = true;
	int error = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);

	EM_DEBUG_LOG("Update mailbox_name - searching by mailbox_name name"); 
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_meeting_tbl SET"
		" mailbox_name = '%s'"
		" WHERE account_id = %d"
		" AND mailbox_name = '%s'"
		, new_mailbox_name
		, account_id
		, old_mailbox_name);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_exec fail:%d", rc));
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) 
		EM_DEBUG_EXCEPTION("NO meetring request found...");

	ret = true;
	
FINISH_OFF:

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_delete_meeting_request(int account_id, int mail_id, char *mailbox_name, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], mailbox_name[%s], transaction[%d], err_code[%p]", account_id, mail_id, mailbox_name, transaction, err_code);
	
	if (account_id < ALL_ACCOUNT || mail_id < 0) {
		EM_DEBUG_EXCEPTION(" account_id[%d], mail_id[%d]", account_id, mail_id);
		
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int rc;
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	int and = false;
	char sql_query_string[QUERY_SIZE] = {0, };
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_meeting_tbl ");

	if (account_id != ALL_ACCOUNT) {		/*  NOT '0' means a specific account. '0' means all account */
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " WHERE account_id = %d", account_id);
		and = true;
	}
	if (mail_id > 0) {	
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " %s mail_id = %d", (and ? "AND" : "WHERE"), mail_id);
		and = true;
	}
	if (mailbox_name) {		/*  NULL means all mailbox_name */
		SNPRINTF(sql_query_string + strlen(sql_query_string), sizeof(sql_query_string)-(strlen(sql_query_string)+1), " %s mailbox_name = '%s'",  (and ? "AND" : "WHERE"), mailbox_name);
	}
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	ret = true;	
	
FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;
	
	if (err_code)
		*err_code = error;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_free_meeting_request(emf_meeting_request_t **meeting_req, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("meeting_req[%p], count[%d], err_code[%p]", meeting_req, count, err_code);
	
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	int i = 0;
	emf_meeting_request_t *cursor;

	if (count > 0) {
		if (!meeting_req || !*meeting_req) {
			EM_DEBUG_EXCEPTION("meeting_req[%p], count[%d]", meeting_req, count);
			error = EM_STORAGE_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		for (i = 0; i < count; i++) {
			cursor = (*meeting_req) + i;
			EM_SAFE_FREE(cursor->location);
		}
		EM_SAFE_FREE(*meeting_req);
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

   	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}


EXPORT_API int em_storage_get_overflowed_mail_id_list(int account_id, char *mailbox_name, int mail_slot_size, int **mail_id_list, int *mail_id_count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mailbox_name [%p], mail_slot_size [%d], mail_id_list [%p], mail_id_count [%p], transaction [%d], err_code [%p]", account_id, mailbox_name, mail_slot_size, mail_id_list, mail_id_count, transaction, err_code);
	EM_PROFILE_BEGIN(profile_em_storage_get_overflowed_mail_id_list);
	char sql_query_string[QUERY_SIZE] = {0, };
	char **result = NULL;
	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	int counter = 0, col_index = 0;
	int result_mail_id_count = 0;
	int *result_mail_id_list = NULL;

	if (!mailbox_name || !mail_id_list || !mail_id_count || account_id < 1) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		error = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mail_id FROM mail_tbl WHERE account_id = %d AND mailbox_name = '%s' ORDER BY date_time DESC LIMIT %d, 10000", account_id, mailbox_name, mail_slot_size);
	
	EM_DEBUG_LOG("query[%s].", sql_query_string);

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	EM_STORAGE_START_READ_TRANSACTION(transaction);
	
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_mail_id_count, 0, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_mail_id_count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (!result_mail_id_count) {
		EM_DEBUG_LOG("No mail found...");			
		ret = false;
		error= EM_STORAGE_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	EM_DEBUG_LOG("There are [%d] overflowed mails in mailbox_name [%s]", result_mail_id_count, mailbox_name);
	
	if (!(result_mail_id_list = (int *)malloc(sizeof(int) * result_mail_id_count))) {
		EM_DEBUG_EXCEPTION("malloc for result_mail_id_list failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
		sqlite3_free_table(result);
		goto FINISH_OFF;
	}

	memset(result_mail_id_list, 0x00, sizeof(int) * result_mail_id_count);

	col_index = 1;

	for (counter = 0; counter < result_mail_id_count; counter++) 
		_getTableFieldDataInt(result, result_mail_id_list + counter, col_index++);

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
	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(profile_em_storage_get_overflowed_mail_id_list);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_get_thread_id_by_mail_id(int mail_id, int *thread_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], thread_id[%p], err_code[%p]", mail_id, thread_id, err_code);
	
	if (mail_id == 0 || thread_id == NULL) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	int rc = -1, ret = false;
	int err = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	char **result;
	int result_count = 0;
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	memset(sql_query_string, 0, QUERY_SIZE);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT thread_id FROM mail_tbl WHERE mail_id = %d", mail_id);

	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_count, 0, NULL); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {err = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (!result_count) {
		EM_DEBUG_EXCEPTION("No mail found...");			
		ret = false;
		err= EM_STORAGE_ERROR_MAIL_NOT_FOUND;
		/* sqlite3_free_table(result); */
		goto FINISH_OFF;
	}

	_getTableFieldDataInt(result, thread_id, 1);

	sqlite3_free_table(result);
	
	ret = true;
	
FINISH_OFF:

	_DISCONNECT_DB;

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_update_latest_thread_mail(int account_id, int thread_id, int latest_mail_id, int thread_item_count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], thread_id[%d], latest_mail_id [%d], thread_item_count[%d], err_code[%p]", account_id, thread_id, latest_mail_id, thread_item_count, err_code);
	
	if (thread_id == 0) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	int rc = -1, ret = false;
	int err = EM_STORAGE_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	char **result;
	int result_count = 0;
	
	sqlite3 *local_db_handle = em_storage_get_db_connection();

	if (thread_item_count == 0 || latest_mail_id == 0) {
		memset(sql_query_string, 0, QUERY_SIZE);
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mail_id, count(*) FROM (SELECT account_id, mail_id, thread_id, mailbox_type FROM mail_tbl ORDER BY date_time) WHERE account_id = %d AND thread_id = %d AND mailbox_type NOT in (3,5,7)", account_id, thread_id);

		/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_count, 0, NULL); */
		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_count, 0, NULL), rc);
		EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {err = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		EM_DEBUG_LOG("result_count[%d]", result_count);
		if (result_count == 0) {
			EM_DEBUG_EXCEPTION("No mail found...");			
			ret = false;
			err= EM_STORAGE_ERROR_MAIL_NOT_FOUND;
			sqlite3_free_table(result);
			goto FINISH_OFF;
		}

		_getTableFieldDataInt(result, &latest_mail_id, 2);
		_getTableFieldDataInt(result, &thread_item_count, 3);

		EM_DEBUG_LOG("latest_mail_id[%d]", latest_mail_id);
		EM_DEBUG_LOG("thread_item_count[%d]", thread_item_count);

		sqlite3_free_table(result);
	}

	EM_STORAGE_START_WRITE_TRANSACTION(transaction, err);

	/* if (thread_item_count > 1) */
	/* { */
		memset(sql_query_string, 0, QUERY_SIZE);
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET thread_item_count = 0 WHERE account_id = %d AND thread_id = %d", account_id, thread_id);
		EM_DEBUG_LOG("query[%s]", sql_query_string);

		EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
			EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	/* } */

	memset(sql_query_string, 0, QUERY_SIZE);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET thread_item_count = %d WHERE account_id = %d AND mail_id = %d ", thread_item_count, account_id, latest_mail_id);
	EM_DEBUG_LOG("query[%s]", sql_query_string);

	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	ret = true;
	
FINISH_OFF:
		
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, err);
	
	_DISCONNECT_DB;
	
	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API void
em_storage_flush_db_cache()
{
	sqlite3_release_memory(-1);
}

#ifdef __LOCAL_ACTIVITY__
/**
  * 	em_storage_add_activity - Add Email Local activity during OFFLINE mode
  *
  */
EXPORT_API int em_storage_add_activity(emf_activity_tbl_t* local_activity, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	EM_DEBUG_LOG(" local_activity[%p], transaction[%d], err_code[%p]", local_activity, transaction, err_code);

	int rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[8192] = { 0x00, };
	int i = 0;
	
	if (!local_activity) {
		EM_DEBUG_EXCEPTION(" local_activity[%p], transaction[%d], err_code[%p]", local_activity, transaction, err_code);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = em_storage_get_db_connection();
	memset(sql_query_string, 0x00 , sizeof(sql_query_string));
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "INSERT INTO mail_local_activity_tbl VALUES (?, ?, ?, ?, ?, ?, ?)");

	EM_DEBUG_LOG(">>>>> ACTIVITY ID [ %d ] ", local_activity->activity_id);
	EM_DEBUG_LOG(">>>>> MAIL ID [ %d ] ", local_activity->mail_id);
	EM_DEBUG_LOG(">>>>> ACCOUNT ID [ %d ] ", local_activity->account_id);
	EM_DEBUG_LOG(">>>>> ACTIVITY TYPE [ %d ] ", local_activity->activity_type);
	EM_DEBUG_LOG(">>>>> SERVER MAIL ID [ %s ] ", local_activity->server_mailid);
	EM_DEBUG_LOG(">>>>> SOURCE MAILBOX [ %s ] ", local_activity->src_mbox);
	EM_DEBUG_LOG(">>>>> DEST MAILBOX   [ %s ] ", local_activity->dest_mbox);

	EM_DEBUG_LOG(">>>> SQL STMT [ %s ] ", sql_query_string);
	
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, strlen(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG(">>>> SQL STMT [ %s ] ", sql_query_string);




	_bindStmtFieldDataInt(hStmt, i++, local_activity->activity_id);
	_bindStmtFieldDataInt(hStmt, i++, local_activity->account_id);
	_bindStmtFieldDataInt(hStmt, i++, local_activity->mail_id);
	_bindStmtFieldDataInt(hStmt, i++, local_activity->activity_type);
	_bindStmtFieldDataString(hStmt, i++ , (char *)local_activity->server_mailid, 0, SERVER_MAIL_ID_LEN_IN_MAIL_TBL);
	_bindStmtFieldDataString(hStmt, i++ , (char *)local_activity->src_mbox, 0, MAILBOX_NAME_LEN_IN_MAIL_BOX_TBL);
	_bindStmtFieldDataString(hStmt, i++ , (char *)local_activity->dest_mbox, 0, MAILBOX_NAME_LEN_IN_MAIL_BOX_TBL);
	
	/*  rc = sqlite3_step(hStmt); */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);

	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EM_STORAGE_ERROR_DB_IS_FULL;goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("sqlite3_step fail:%d, errmsg = %s.", rc, sqlite3_errmsg(local_db_handle)));

	ret = true;
	
FINISH_OFF:
	
	if (hStmt != NULL)  {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK)  {
			EM_DEBUG_LOG(" sqlite3_finalize failed - %d", rc);
			
			error = EM_STORAGE_ERROR_DB_FAILURE;
		}		
	}
	else {
		EM_DEBUG_LOG(" >>>>>>>>>> hStmt is NULL!!!");
	}

	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	_DISCONNECT_DB;

	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/**
  *	em_storage_get_activity - Get the Local activity Information
  *	
  *
  */
EXPORT_API int em_storage_get_activity(int account_id, int activityid, emf_activity_tbl_t** activity_list, int *select_num, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int i = 0, count = 0, rc = -1, ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	emf_activity_tbl_t *p_activity_tbl = NULL;
	char sql_query_string[1024] = {0x00, };
	char **result = NULL;
	int col_index ;

	EM_IF_NULL_RETURN_VALUE(activity_list, false);
	EM_IF_NULL_RETURN_VALUE(select_num, false);

	
	if (!select_num || !activity_list || account_id <= 0 || activityid < 0) {
		EM_DEBUG_LOG(" select_num[%p], activity_list[%p] account_id [%d] activityid [%d] ", select_num, activity_list, account_id, activityid);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	EM_STORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	if (activityid == ALL_ACTIVITIES) {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_local_activity_tbl WHERE account_id = %d order by activity_id", account_id);
	}
	else {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_local_activity_tbl WHERE account_id = %d AND activity_id = %d ", account_id, activityid);
	}

	EM_DEBUG_LOG("Query [%s]", sql_query_string);

		
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL);   */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	
	col_index = 7;
	
	if (!(p_activity_tbl = (emf_activity_tbl_t*)em_core_malloc(sizeof(emf_activity_tbl_t) * count))) {
		EM_DEBUG_EXCEPTION(" em_core_malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
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
		em_storage_free_local_activity(&p_activity_tbl, count, NULL);
	}
	
	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int em_storage_get_next_activity_id(int *activity_id, int *err_code)
{

	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int rc = -1;
	char *sql = NULL;
	char **result = NULL;	

	if (NULL == activity_id)
   	{
		EM_DEBUG_EXCEPTION(" activity_id[%p]", activity_id);
		
		err = EM_STORAGE_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/*  increase unique id */

	sql = "SELECT max(rowid) FROM mail_local_activity_tbl;";
	
	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL); n EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc); */
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
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

EXPORT_API int em_storage_get_activity_id_list(int account_id, int ** activity_id_list, int *activity_id_count, int lowest_activity_type, int highest_activity_type, int transaction, int *err_code)
{

	EM_DEBUG_FUNC_BEGIN();
	
	EM_DEBUG_LOG(" account_id[%d], activity_id_list[%p], activity_id_count[%p] err_code[%p]", account_id,  activity_id_list, activity_id_count, err_code);
	
	if (account_id <= 0|| NULL == activity_id_list || NULL == activity_id_count ||lowest_activity_type <=0 || highest_activity_type <= 0)  {
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int error = EM_STORAGE_ERROR_NONE;
	int i = 0, rc = -1, count = 0;
	char sql_query_string[1024] = {0x00, };
	int *activity_ids = NULL;
	int col_index = 0;
	char **result = NULL;
	
	EM_STORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT distinct activity_id FROM mail_local_activity_tbl WHERE account_id = %d AND activity_type >= %d AND activity_type <= %d order by activity_id", account_id, lowest_activity_type, highest_activity_type);

	EM_DEBUG_LOG(" Query [%s]", sql_query_string);

	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL);   */
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EM_STORAGE_ERROR_DB_FAILURE;sqlite3_free_table(result);goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	col_index = 1;

	EM_DEBUG_LOG(" Activity COUNT : %d ... ", count);

	if (NULL == (activity_ids = (int *)em_core_malloc(sizeof(int) * count))) {
		EM_DEBUG_EXCEPTION(" em_core_malloc failed...");
		error = EM_STORAGE_ERROR_OUT_OF_MEMORY;
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
	

	EM_STORAGE_FINISH_READ_TRANSACTION(transaction);
	if (err_code != NULL) {
		*err_code = error;
	}

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int em_storage_free_activity_id_list(int *activity_id_list, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int error = EM_STORAGE_ERROR_NONE;
	int ret = false;

	EM_DEBUG_LOG(" activity_id_list [%p]", activity_id_list);

	if (NULL == activity_id_list) {
		error = EM_STORAGE_ERROR_INVALID_PARAM;
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
 * em_storage_delete_local_activity - Deletes the Local acitivity Generated based on activity_type
 * or based on server mail id
 *
 */
EXPORT_API int em_storage_delete_local_activity(emf_activity_tbl_t* local_activity, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();


	EM_DEBUG_LOG(" local_activity[%p] ", local_activity);
	
	if (!local_activity)  {
		EM_DEBUG_EXCEPTION(" local_activity[%p] ", local_activity);
		if (err_code != NULL)
			*err_code = EM_STORAGE_ERROR_INVALID_PARAM;
		return false;
	}		
	
	int rc = -1, ret = false;			/* Prevent_FIX  */
	int err = EM_STORAGE_ERROR_NONE;
	int query_and = 0;
	int query_where = 0;
	char sql_query_string[8192] = { 0x00, };
	EM_STORAGE_START_WRITE_TRANSACTION(transaction, error);

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
	
	EM_STORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, sql_query_string, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err = EM_STORAGE_ERROR_DB_FAILURE;goto FINISH_OFF; },
		("SQL(%s) exec fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
	
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)  {
		EM_DEBUG_EXCEPTION(" no (matched) mailbox_name found...");
		err = EM_STORAGE_ERROR_MAILBOX_NOT_FOUND;
	}

	ret = true;
	
FINISH_OFF:
	EM_STORAGE_FINISH_WRITE_TRANSACTION(transaction, ret, error);
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/**
*	em_storage_free_local_activity - Free the Local Activity data
*/
EXPORT_API int em_storage_free_local_activity(emf_activity_tbl_t **local_activity_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	EM_DEBUG_LOG(" local_activity_list[%p], count[%d], err_code[%p]", local_activity_list, count, err_code);
	
	int ret = false;
	int error = EM_STORAGE_ERROR_INVALID_PARAM;
	
	if (count > 0) {
		if (!local_activity_list || !*local_activity_list) {
			EM_DEBUG_EXCEPTION(" local_activity_list[%p], count[%d]", local_activity_list, count);
			
			error = EM_STORAGE_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		emf_activity_tbl_t* p = *local_activity_list;
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
#endif /* __LOCAL_ACTIVITY__ */


/*EOF*/
