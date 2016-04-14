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
#include <fcntl.h>
#include <tzplatform_config.h>

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
#include "email-core-signal.h"
#include "email-core-event.h"
#include "email-core-container.h"
#include "email-core-gmime.h"
#include "email-core-key-manager.h"

#define SETTING_MEMORY_TEMP_FILE_PATH "/tmp/email_tmp_file"
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
#define FILTER_NAME_LEN_IN_MAIL_RULE_TBL                256
#define VALUE_LEN_IN_MAIL_RULE_TBL                      256
#define VALUE2_LEN_IN_MAIL_RULE_TBL                     256
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
#define EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction_flag, error_code) \
	do { \
		if (transaction_flag) { \
			if (emstorage_begin_transaction(multi_user_name, NULL, NULL, &error_code) == false) { \
				EM_DEBUG_EXCEPTION("emstorage_begin_transaction error [%d]", error_code); \
				goto FINISH_OFF; \
			} \
		} \
	} while (0)

/*  this define is used for query to change data (delete, insert, update) */
#define EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction_flag, result_code, error_code) \
	do { \
		if (transaction_flag) { \
			if (result_code == true) { \
				if (emstorage_commit_transaction(multi_user_name, NULL, NULL, NULL) == false) { \
					EM_DEBUG_EXCEPTION("emstorage_commit_transaction error"); \
					error_code = EMAIL_ERROR_DB_FAILURE; \
					result_code = false; \
				} \
			} \
			else { \
				if (emstorage_rollback_transaction(multi_user_name, NULL, NULL, NULL) == false) { \
					EM_DEBUG_EXCEPTION("emstorage_rollback_transaction error"); \
					error_code = EMAIL_ERROR_DB_FAILURE; \
				} \
			} \
		} \
	} while (0)

/*  this define is used for query to read (select) */
#define EMSTORAGE_START_READ_TRANSACTION(transaction_flag) \
	if (transaction_flag) { \
		/*_timedlock_shm_mutex(mapped_for_db_lock, 2);*/\
	}

/*  this define is used for query to read (select) */
#define EMSTORAGE_FINISH_READ_TRANSACTION(transaction_flag) \
	if (transaction_flag) { \
		/*_unlockshm_mutex(mapped_for_db_lock);*/\
	}

/*  for safety DB operation */
static pthread_mutex_t _db_handle_lock = PTHREAD_MUTEX_INITIALIZER;


#define	_MULTIPLE_DB_HANDLE

#ifdef _MULTIPLE_DB_HANDLE

typedef struct {
    char *user_name;
	pthread_t 	thread_id;
	sqlite3 *db_handle;
} db_handle_t;

#define MAX_DB_CLIENT 100

/* static int _db_handle_count = 0; */
db_handle_t _db_handle_list[MAX_DB_CLIENT] = {{NULL, 0, 0}, };


sqlite3 *emstorage_get_db_handle(char *multi_user_name)
{
	EM_DEBUG_FUNC_BEGIN();
	int i;
	pthread_t current_thread_id = THREAD_SELF();
	sqlite3 *result_db_handle = NULL;

	if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		ENTER_CRITICAL_SECTION(_db_handle_lock);
		for (i = 0; i < MAX_DB_CLIENT; i++) {
			if (EM_SAFE_STRCASECMP(_db_handle_list[i].user_name, multi_user_name) != 0)
				continue;

			if (pthread_equal(current_thread_id, _db_handle_list[i].thread_id)) {
				EM_DEBUG_LOG_DEV("found db handle at [%d]", i);
				result_db_handle = _db_handle_list[i].db_handle;
				break;
			}
		}
		LEAVE_CRITICAL_SECTION(_db_handle_lock);
	} else {
		ENTER_CRITICAL_SECTION(_db_handle_lock);
		for (i = 0; i < MAX_DB_CLIENT; i++) {
			if (EM_SAFE_STRLEN(_db_handle_list[i].user_name) > 0)
				continue;

			if (pthread_equal(current_thread_id, _db_handle_list[i].thread_id)) {
				EM_DEBUG_LOG_DEV("found db handle at [%d]", i);
				result_db_handle = _db_handle_list[i].db_handle;
				break;
			}
		}
		LEAVE_CRITICAL_SECTION(_db_handle_lock);
	}

	if (!result_db_handle)
		EM_DEBUG_LOG("no db_handle for [%d] found", current_thread_id);

	EM_DEBUG_FUNC_END();
	return result_db_handle;
}

int emstorage_set_db_handle(char *multi_user_name, sqlite3 *db_handle)
{
	EM_DEBUG_FUNC_BEGIN();
	int i, error_code = EMAIL_ERROR_MAX_EXCEEDED;
	pthread_t current_thread_id = THREAD_SELF();

	ENTER_CRITICAL_SECTION(_db_handle_lock);
	for (i = 0; i < MAX_DB_CLIENT; i++)	{
		if (_db_handle_list[i].thread_id == 0) {
			_db_handle_list[i].thread_id = current_thread_id;
			_db_handle_list[i].db_handle = db_handle;
            /* Only distinguished container and host  */
            _db_handle_list[i].user_name = EM_SAFE_STRDUP(multi_user_name);
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
	for (i = 0; i < MAX_DB_CLIENT; i++) {
		if (_db_handle_list[i].thread_id == THREAD_SELF()) {
			_db_handle_list[i].thread_id = 0;
			_db_handle_list[i].db_handle = NULL;
            EM_SAFE_FREE(_db_handle_list[i].user_name);

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
	for (i = 0; i < MAX_DB_CLIENT; i++) {
		_db_handle_list[i].thread_id = 0;
		_db_handle_list[i].db_handle = NULL;
        EM_SAFE_FREE(_db_handle_list[i].user_name);
	}
	LEAVE_CRITICAL_SECTION(_db_handle_lock)

	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}


sqlite3 *emstorage_get_db_connection(char *multi_user_name)
{
	return emstorage_db_open(multi_user_name, NULL);
}

#else	/*  _MULTIPLE_DB_HANDLE */

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
typedef struct {
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
	do {\
		_timedlock_shm_mutex(mapped_for_db_lock, 2);\
		return_value = function_call;\
		_unlockshm_mutex(mapped_for_db_lock);\
	} while (0)

#else /*  __FEATURE_USE_SHARED_MUTEX_FOR_PROTECTED_FUNC_CALL__ */
#define EMSTORAGE_PROTECTED_FUNC_CALL(function_call, return_value) \
	{  return_value = function_call; }
#endif /*  __FEATURE_USE_SHARED_MUTEX_FOR_PROTECTED_FUNC_CALL__ */

static int emstorage_exec_query_by_prepare_v2(sqlite3 *local_db_handle, char *query_string)
{
	EM_DEBUG_FUNC_BEGIN("local_db_handle[%p] query_string[%p]", local_db_handle, query_string);
	int error = EMAIL_ERROR_NONE;
	int rc = 0;
	DB_STMT db_statement = NULL;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, query_string, EM_SAFE_STRLEN(query_string), &db_statement, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_prepare failed [%d] [%s]", rc, query_string));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(db_statement), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
	("sqlite3_step failed [%d] [%s]", rc, query_string));

FINISH_OFF:

	if (db_statement != NULL) {
		rc = sqlite3_finalize(db_statement);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	return error;
}

INTERNAL_FUNC int emstorage_shm_file_init(const char *shm_file_name)
{
	EM_DEBUG_FUNC_BEGIN("shm_file_name [%p]", shm_file_name);
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (!shm_file_name) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	int fd = shm_open(shm_file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP); /*  note: permission is not working */
	if (fd < 0) {
		EM_DEBUG_EXCEPTION("shm_open errno [%d] [%s]", errno, EM_STRERROR(errno_buf));
		return EMAIL_ERROR_SYSTEM_FAILURE;
	}

	fchmod(fd, 0666);
	EM_DEBUG_LOG("** Create SHM FILE **");
	if (ftruncate(fd, sizeof(mmapped_t)) != 0) {
		EM_DEBUG_EXCEPTION("ftruncate errno [%d]", errno);
		return EMAIL_ERROR_SYSTEM_FAILURE;
	}

	mmapped_t *m = (mmapped_t *)mmap(NULL, sizeof(mmapped_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (m == MAP_FAILED) {
		EM_DEBUG_EXCEPTION("mmap errno [%d]", errno);
		EM_SAFE_CLOSE(fd);
		return EMAIL_ERROR_SYSTEM_FAILURE;
	}

	m->data = 0;

	pthread_mutexattr_t mattr;
	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
	pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST_NP);
	pthread_mutex_init(&(m->mutex), &mattr);
	pthread_mutexattr_destroy(&mattr);

    pthread_mutex_destroy(&(m->mutex));
    munmap(m, sizeof(mmapped_t));

	EM_SAFE_CLOSE(fd);
	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

int emstorage_shm_file_destroy(const char *shm_file_name)
{
	EM_DEBUG_FUNC_BEGIN("shm_file_name [%p]", shm_file_name);
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (!shm_file_name) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (shm_unlink(shm_file_name) != 0)
		EM_DEBUG_EXCEPTION("shm_unlink failed: %s", EM_STRERROR(errno_buf));
	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

int _initialize_shm_mutex(const char *shm_file_name, int *param_shm_fd, mmapped_t **param_mapped)
{
	EM_DEBUG_FUNC_BEGIN("shm_file_name [%p] param_shm_fd [%p], param_mapped [%p]", shm_file_name, param_shm_fd, param_mapped);
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (!shm_file_name || !param_shm_fd || !param_mapped) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (!(*param_mapped)) {
		EM_DEBUG_LOG("** mapping begin **");
		if (!(*param_shm_fd)) { /*  open shm_file_name at first. Otherwise, the num of files in /proc/pid/fd will be increasing  */
			*param_shm_fd = shm_open(shm_file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
			if ((*param_shm_fd) == -1) {
				EM_DEBUG_EXCEPTION("FAIL: shm_open(): %s", EM_STRERROR(errno_buf));
				return EMAIL_ERROR_SYSTEM_FAILURE;
			}
		}

        fchmod((*param_shm_fd), 0666);
        EM_DEBUG_LOG("** Create SHM FILE **");
        if (ftruncate((*param_shm_fd), sizeof(mmapped_t)) != 0) {
            EM_DEBUG_EXCEPTION("ftruncate errno [%d]", errno);
            return EMAIL_ERROR_SYSTEM_FAILURE;
        }

		mmapped_t *tmp = (mmapped_t *)mmap(NULL, sizeof(mmapped_t), PROT_READ|PROT_WRITE, MAP_SHARED, (*param_shm_fd), 0);
		if (tmp == MAP_FAILED) {
			EM_DEBUG_EXCEPTION("mmap failed: %s", EM_STRERROR(errno_buf));
			return EMAIL_ERROR_SYSTEM_FAILURE;
		}

        tmp->data = 0;

        pthread_mutexattr_t mattr;
        pthread_mutexattr_init(&mattr);
        pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
        pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST_NP);
        pthread_mutex_init(&(tmp->mutex), &mattr);
        pthread_mutexattr_destroy(&mattr);

		*param_mapped = tmp;
	}

	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

int _timedlock_shm_mutex(mmapped_t *param_mapped, int sec)
{
	EM_DEBUG_FUNC_BEGIN("param_mapped [%p], sec [%d]", param_mapped, sec);

	if (!param_mapped) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	struct timespec abs_time;
	clock_gettime(CLOCK_REALTIME, &abs_time);
	abs_time.tv_sec += sec;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	int err = pthread_mutex_timedlock(&(param_mapped->mutex), &abs_time);

	if (err == EOWNERDEAD) {
		err = pthread_mutex_consistent(&(param_mapped->mutex));
		EM_DEBUG_EXCEPTION("Previous owner is dead with lock. Fix mutex : %s", EM_STRERROR(errno_buf));
	} else if (err != 0) {
		EM_DEBUG_EXCEPTION("ERROR : %s", EM_STRERROR(errno_buf));
		return err;
	}

	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

void _unlockshm_mutex(mmapped_t *param_mapped)
{
	EM_DEBUG_FUNC_BEGIN();
	pthread_mutex_unlock(&(param_mapped->mutex));
	EM_DEBUG_FUNC_END();
}
/* ------------------------------------------------------------------------------ */


static int _open_counter = 0;

static int _get_password_file_name(char *multi_user_name, int account_id, char *recv_password_file_name, char *send_password_file_name);
static int _read_password_from_secure_storage(char *file_name, char **password);

#ifdef __FEATURE_SUPPORT_PRIVATE_CERTIFICATE__
static int _get_cert_password_file_name(int index, char *cert_password_file_name);
#endif

typedef struct {
	const char *object_name;
	unsigned int data_flag;
} email_db_object_t;

static const email_db_object_t _g_db_tables[] = {
	{ "mail_read_mail_uid_tbl", 1},
	{ "mail_tbl", 1},
	{ "mail_attachment_tbl", 1},
	{ NULL,  0},
};

static const email_db_object_t _g_db_indexes[] = {
	{ "mail_read_mail_uid_idx1", 1},
	{ "mail_idx1", 1},
	{ "mail_attachment_idx1", 1},
	{ NULL,  1},
};

enum {
	CREATE_TABLE_MAIL_ACCOUNT_TBL,
	CREATE_TABLE_MAIL_BOX_TBL,
	CREATE_TABLE_MAIL_READ_MAIL_UID_TBL,
	CREATE_TABLE_MAIL_RULE_TBL,
	CREATE_TABLE_MAIL_TBL,
	CREATE_TABLE_MAIL_ATTACHMENT_TBL,
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
	CREATE_TABLE_MAIL_PARTIAL_BODY_ACTIVITY_TBL,
#else
	CREATE_TABLE_DUMMY_INDEX1,
#endif
	CREATE_TABLE_MAIL_MEETING_TBL,
#ifdef __FEATURE_LOCAL_ACTIVITY__
	CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL,
#else
	CREATE_TABLE_DUMMY_INDEX2,
#endif
	CREATE_TABLE_MAIL_CERTIFICATE_TBL,
	CREATE_TABLE_MAIL_TASK_TBL,
#ifdef __FEATURE_BODY_SEARCH__
	CREATE_TABLE_MAIL_TEXT_TBL,
#else
	CREATE_TABLE_DUMMY_INDEX3,
#endif

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
	CREATE_TABLE_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL,
#else
	CREATE_TABLE_DUMMY_INDEX4,
#endif

	/*CREATE INDEX*/
	CREATE_TABLE_MAIL_ACCOUNT_IDX,
	CREATE_TABLE_MAIL_BOX_IDX,
	CREATE_TABLE_MAIL_READ_MAIL_UID_IDX,
	CREATE_TABLE_MAIL_IDX,
	CREATE_TABLE_MAIL_ATTACHMENT_IDX,
	CREATE_TABLE_MAIL_MEETING_IDX,
	CREATE_TABLE_MAIL_TASK_IDX,
	CREATE_TABLE_MAIL_DATETIME_IDX,
	CREATE_TABLE_MAIL_THREAD_IDX,
	CREATE_TABLE_MAX,
};

enum {
	DATA1_IDX_IN_MAIL_ACTIVITY_TBL = 0,
	TRANSTYPE_IDX_IN_MAIL_ACTIVITY_TBL,
	FLAG_IDX_IN_MAIL_ACTIVITY_TBL,
};

enum {
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

enum {
	TO_RECIPIENT = 0,
	CC_RECIPIENT,
	BCC_RECIPIENT,
};
enum {
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

enum {
	ACCOUNT_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL = 0,
	LOCAL_MAILBOX_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	MAILBOX_NAME_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	LOCAL_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	SERVER_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	RFC822_SIZE_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	SYNC_STATUS_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	FLAGS_SEEN_FIELD_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	FLAGS_FLAGGED_FIELD_IDX_IN_MAIL_READ_MAIL_UID_TBL,
	IDX_NUM_IDX_IN_MAIL_READ_MAIL_UID_TBL, 		/* unused */
};

#ifdef __FEATURE_BODY_SEARCH__
enum {
	MAIL_ID_IDX_IN_MAIL_TEXT_TBL = 0,
	ACCOUNT_ID_IDX_IN_MAIL_TEXT_TBL,
	MAILBOX_ID_IDX_IN_MAIL_TEXT_TBL,
	BODY_TEXT_IDX_IN_MAIL_TEXT_TBL,
	FIELD_COUNT_OF_MAIL_TEXT_TBL,
};
#endif

enum {
	ACCOUNT_ID_IDX_IN_MAIL_RULE_TBL = 0,
	RULE_ID_IDX_IN_MAIL_RULE_TBL,
	FILTER_NAME_IDX_IN_MAIL_RULE_TBL,
	TYPE_IDX_IN_MAIL_RULE_TBL,
	VALUE_IDX_IN_MAIL_RULE_TBL,
	VALUE2_IDX_IN_MAIL_RULE_TBL,
	ACTION_TYPE_IDX_IN_MAIL_RULE_TBL,
	TARGET_MAILBOX_ID_IDX_IN_MAIL_RULE_TBL,
	FLAG1_IDX_IN_MAIL_RULE_TBL,
	FLAG2_IDX_IN_MAIL_RULE_TBL,
};

enum {
	MAIL_ID_IDX_IN_MAIL_TBL = 0,
	ACCOUNT_ID_IDX_IN_MAIL_TBL,
	MAILBOX_ID_IDX_IN_MAIL_TBL,
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
	SCHEDULED_SENDING_TIME_IDX_IN_MAIL_TBL,
	REMAINING_RESEND_TIMES_IDX_IN_MAIL_TBL,
	TAG_ID_IDX_IN_MAIL_TBL,
	REPLIED_TIME_IDX_IN_MAIL_TBL,
	FORWARDED_TIME_IDX_IN_MAIL_TBL,
	DEFAULT_CHARSET_IDX_IN_MAIL_TBL,
	EAS_DATA_LENGTH_IDX_IN_MAIL_TBL,
	EAS_DATA_IDX_IN_MAIL_TBL,
        USER_NAME_IDX_IN_MAIL_TBL,
	FIELD_COUNT_OF_MAIL_TBL,  /* End of mail_tbl */
};

enum {
	ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL = 0,
	ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL,
	ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL,
	CONTENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL,
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

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
enum {
	ACTIVITY_ID_IDX_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL = 0,
	STATUS_IDX_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL,
	ACCOUNT_ID_IDX_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL,
	MAIL_ID_IDX_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL,
	SERVER_MAIL_ID_IDX_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL,
	MAILBOX_ID_IDX_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL,
    MULTI_USER_NAME_IDX_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL,
};
#endif

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
		" incoming_server_authentication_method,"
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
		" auto_resend_times, "
		" outgoing_server_size_limit, "
		" wifi_auto_download, "
		" pop_before_smtp, "
		" incoming_server_requires_apop,"
		" logo_icon_path, "
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
		", notification_status "
		", vibrate_status "
		", display_content_status "
		", default_ringtone_status "
		", alert_ringtone_path "
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
		"   deleted_flag,  "
		"   modifiable_yn,  "
		"   total_mail_count_on_server,  "
		"   has_archived_mails, "
		"   mail_slot_size, "
		"   no_select, "
		"   last_sync_time "
		" FROM mail_box_tbl ",
		/*  3. select mail_read_mail_uid_tbl */
		"SELECT  "
		"   account_id,  "
		"   mailbox_id,  "
		"   mailbox_name,  "
		"   local_uid,  "
		"   server_uid,  "
		"   rfc822_size ,  "
		"   sync_status,  "
		"   flags_seen_field,  "
		"   idx_num "
		" FROM mail_read_mail_uid_tbl ",
		/*  4. select mail_rule_tbl */
		"SELECT "
		"   account_id, "
		"   rule_id, "
		"   filter_name, "
		"   type, "
		"   value, "
		"   value2, "
		"   action_type, "
		"   target_mailbox_id,	"
		"   flag1, "
		"   flag2 "
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
                "   multi_user_name "
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
		"SELECT	"
		"	certificate_id, "
		"   issue_year, "
		"   issue_month, "
		"   issue_day, "
		"   expiration_year, "
		"   expiration_month, "
		"   expiration_day, "
		"   issue_organization_name, "
		"   email_address, "
		"   subject_str, "
		"   filepath, "
		"   password "
		" FROM mail_certificate_tbl	",
		"SELECT	"
		"	task_id, "
		"   task_type, "
		"   task_status, "
		"   task_priority, "
		"   task_parameter_length, "
		"   task_parameter , "
		"	date_time "
		" FROM mail_task_tbl	",
#ifdef __FEATURE_BODY_SEARCH__
		"SELECT	"
		"	mail_id, "
		"   account_id, "
		"   mailbox_id, "
		"   body_text "
		" FROM mail_text_tbl	",
#endif
#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
		"SELECT  "
		"   activity_id, "
		"   status, "
		"   account_id, "
		"   mail_id, "
		"   server_mail_id, "
		"   mailbox_id, "
                "   multi_user_name, "
		" FROM mail_auto_download_activity_tbl ",
#endif
		NULL,
};

int _field_count_of_table[CREATE_TABLE_MAX] = { 0, };

static int _get_table_field_data_char(char  **table, char *buf, int index)
{
	if ((table == NULL) || (buf == NULL) || (index < 0)) {
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
	if ((table == NULL) || (buf == NULL) || (index < 0)) {
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
	if ((table == NULL) || (buf == NULL) || (index < 0)) {
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

	if ((table == NULL) || (buf == NULL) || (index < 0)) {
		EM_DEBUG_EXCEPTION("table[%p], buf[%p], index[%d]", table, buf, index);
		return false;
	}

	char *pTemp = table[index];
	int sLen = 0;
	if (pTemp == NULL)
		*buf = NULL;
	else {
		sLen = EM_SAFE_STRLEN(pTemp);
		if (sLen) {
			*buf = (char *) em_malloc(sLen + 1);
			if (*buf == NULL) {
				EM_DEBUG_EXCEPTION("malloc is failed");
				goto FINISH_OFF;
			}
			strncpy(*buf, pTemp, sLen);
		} else
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
	if ((table == NULL) || (buf == NULL) || (index < 0)) {
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

static int _get_table_field_data_blob(char **table, void **buffer, int buffer_size, int index)
{
	if ((table == NULL) || (buffer == NULL) || (index < 0)) {
		EM_DEBUG_EXCEPTION(" table[%p], buffer[%p], buffer_size [%d], index[%d]", table, buffer, buffer_size, index);
		return false;
	}

	char *temp_buffer = table[index];

	if (temp_buffer == NULL)
		buffer = NULL;
	else {
		*buffer = malloc(buffer_size);
		if (*buffer == NULL) {
			EM_DEBUG_EXCEPTION("allocation failed.");
			return false;
		}
		memset(*buffer, 0, buffer_size);
		memcpy(*buffer, temp_buffer, buffer_size);
	}
#ifdef _PRINT_STORAGE_LOG_
	if (buf)
		EM_DEBUG_LOG("_get_table_field_data_string - buffer[%s], index[%d]", buffer, index);
	else
		EM_DEBUG_LOG("_get_table_field_data_string - No string got ");
#endif

	return true;
}

static int _get_stmt_field_data_char(DB_STMT hStmt, char *buf, int index)
{
	if ((hStmt == NULL) || (buf == NULL) || (index < 0)) {
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
	if ((hStmt == NULL) || (buf == NULL) || (index < 0)) {
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
	if ((hStmt == NULL) || (buf == NULL) || (index < 0)) {
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
	if (!hStmt || !buf || (index < 0)) { /*prevent 39619*/
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
		if (*buf == NULL) {
			EM_DEBUG_EXCEPTION("em_mallocfailed");
			return false;
		}

		strncpy(*buf, (char *)sqlite3_column_text(hStmt, index), sLen);
	} else
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
	if (!hStmt || !buf || (index < 0)) { /*prevent 39618*/
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
		if (*buf == NULL) {
			EM_DEBUG_EXCEPTION("em_mallocfailed");
			return;
		}

		memcpy(*buf, (void *)sqlite3_column_blob(hStmt, index), sLen);
	} else
		*buf = NULL;

}

static int _bind_stmt_field_data_char(DB_STMT hStmt, int index, char value)
{
	if ((hStmt == NULL) || (index < 0)) {
		EM_DEBUG_EXCEPTION("index[%d]", index);
		return false;
	}

	int ret = sqlite3_bind_int(hStmt, index+1, (int)value);

	if (ret != SQLITE_OK) {
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

	if (ret != SQLITE_OK) {
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

	if (ret != SQLITE_OK) {
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

	if (ret != SQLITE_OK) {
		EM_DEBUG_EXCEPTION("sqlite3_bind_text fail [%d]", ret);
		return false;
	}
	return true;
}

/* destroy function for sqlite3_bind_text */
void _bind_stmt_free_string(void* arg)
{
	EM_DEBUG_FUNC_BEGIN();
	char* p = (char*) arg;
	EM_SAFE_FREE(p);
	EM_DEBUG_FUNC_END();
}

static int _bind_stmt_field_data_nstring(DB_STMT hStmt, int index, char *value, int ucs2, int max_len)
{
	if ((hStmt == NULL) || (index < 0)) {
		EM_DEBUG_EXCEPTION("index[%d], max_len[%d]", index, max_len);
		return false;
	}

#ifdef _PRINT_STORAGE_LOG_
	EM_DEBUG_LOG("hStmt = %p, index = %d, max_len = %d, value = [%s]", hStmt, index, max_len, value);
#endif

	int ret = 0;
	if (value != NULL) {
		if (strlen(value) <= max_len)
			ret = sqlite3_bind_text(hStmt, index+1, value, -1, SQLITE_STATIC);
		else {
			char *buf = (char*)em_malloc(sizeof(char) * (max_len));
			if (buf == NULL) {
				EM_DEBUG_EXCEPTION("em_mallocfailed");
				return false;
			}
			snprintf(buf, max_len-1, "%s", value);
			ret = sqlite3_bind_text(hStmt, index+1, buf, -1, _bind_stmt_free_string);
		}
	} else
		ret = sqlite3_bind_text(hStmt, index+1, "", -1, NULL);

	if (ret != SQLITE_OK) {
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
	if (blob_size > 0)
		ret = sqlite3_bind_blob(hStmt, index+1, blob, blob_size, SQLITE_STATIC);
	else
		ret = sqlite3_bind_null(hStmt, index+1);

	if (ret != SQLITE_OK) {
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

	if ((dp = opendir(path)) == NULL) {
		EM_DEBUG_EXCEPTION("opendir(\"%s\") failed...", path);
		return false;
	}

	while ((entry = readdir(dp)) != NULL) {
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

	if (src) {
		if (!(p = em_malloc((int)EM_SAFE_STRLEN(src) + 1))) {
			EM_DEBUG_EXCEPTION("mailoc failed...");
			return NULL;
		}
		strncpy(p, src, EM_SAFE_STRLEN(src));
	}

	return p;
}

static void _emstorage_close_once(void)
{
	EM_DEBUG_FUNC_BEGIN();

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int emstorage_close(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int error = EMAIL_ERROR_NONE;

	if (!emstorage_db_close(NULL, &error))

	if (--_open_counter == 0)
		_emstorage_close_once();

	ret = true;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

static void *_emstorage_open_once(char *multi_user_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int error = EMAIL_ERROR_NONE;

    if (EM_SAFE_STRLEN(multi_user_name) > 0) {
        char buf[MAX_PATH] = {0};
		char *prefix_path = NULL;

		error = emcore_get_container_path(multi_user_name, &prefix_path);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_container_path failed : [%d]", error);
			goto FINISH_OFF;
		}

        memset(buf, 0x00, sizeof(buf));
        SNPRINTF(buf, sizeof(buf), "%s%s", prefix_path, EMAILPATH);
        mkdir(buf, DIRECTORY_PERMISSION);

        memset(buf, 0x00, sizeof(buf));
        SNPRINTF(buf, sizeof(buf), "%s%s", prefix_path, MAILHOME);
        mkdir(buf, DIRECTORY_PERMISSION);

        memset(buf, 0x00, sizeof(buf));
        SNPRINTF(buf, sizeof(buf), "%s%s", prefix_path, MAILTEMP);
        mkdir(buf, DIRECTORY_PERMISSION);

        _delete_temp_file(buf);
		EM_SAFE_FREE(prefix_path);
    } else {
        mkdir(DATA_PATH, DIRECTORY_PERMISSION);
        mkdir(EMAILPATH, DIRECTORY_PERMISSION);
        mkdir(MAILHOME, DIRECTORY_PERMISSION);
        mkdir(MAILTEMP, DIRECTORY_PERMISSION);

        _delete_temp_file(MAILTEMP);
    }

	if (!emstorage_create_table(multi_user_name, EMAIL_CREATE_DB_NORMAL, &error)) {
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
static int _callback_sqlite_busy_handler(void *pData, int count)
{
	if (10 - count > 0) {
		struct timespec time = {
			.tv_sec = 0,
			.tv_nsec = (count + 1) * 100 * 1000 * 1000
		};
		EM_DEBUG_LOG("Busy handler called!!: PID[%d] / CNT [%d]", getpid(), count);
		nanosleep(&time, NULL);
		return 1;
	} else {
		EM_DEBUG_EXCEPTION("Busy handler will be returned SQLITE_BUSY error PID[%d] / CNT[%d]", getpid(), count);
		return 0;
	}
}

static int _callback_collation_utf7_sort(void *data, int length_text_a, const void *text_a,
													 int length_text_b, const void *text_b)
{
    EM_DEBUG_FUNC_BEGIN();
    int result = 0;
    char *converted_string_a = NULL;
    char *converted_string_b = NULL;

	EM_DEBUG_LOG_DEV("text_a : [%s]", text_a);
	converted_string_a = emcore_convert_mutf7_to_utf8((char *)text_a);
	EM_DEBUG_LOG_DEV("Converted text_a : [%s]", converted_string_a);

	EM_DEBUG_LOG_DEV("text_b : [%s]", text_b);
	converted_string_b = emcore_convert_mutf7_to_utf8((char *)text_b);
	EM_DEBUG_LOG_DEV("Converted text_b : [%s]", converted_string_b);

	if (converted_string_a && converted_string_b)
		result = strcmp(converted_string_a, converted_string_b);

	EM_SAFE_FREE(converted_string_a);
	EM_SAFE_FREE(converted_string_b);

	EM_DEBUG_FUNC_END();
	return result;
}

static int _delete_all_files_and_directories(char *db_file_path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int error = EMAIL_ERROR_NONE;
	int ret = false;

	if (!emstorage_delete_file(db_file_path, &error)) {
		if (error != EMAIL_ERROR_FILE_NOT_FOUND) {
			EM_DEBUG_EXCEPTION_SEC("remove failed - %s", EMAIL_SERVICE_DB_FILE_PATH);
			goto FINISH_OFF;
		}
	}

	if (!emstorage_delete_dir((char *)MAILHOME, &error)) {
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

static int _recovery_from_malformed_db_file(char *db_file_path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int error = EMAIL_ERROR_NONE;
	int ret = false;

	/* Delete all files and directories */
	if (!_delete_all_files_and_directories(db_file_path, &error)) {
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

int _xsystem(const char *argv[])
{
	int status = 0;
	pid_t pid;
	pid = fork();

	switch (pid) {
		case -1:
			perror("fork failed");
			return -1;
		case 0:
			if (execvp(argv[0], (char *const *)argv) == -1) {
				perror("execute init db script");
				_exit(-1);
			}

			_exit(2);
		default:
			/* parent */
			break;
	}

	if (waitpid(pid, &status, 0) == -1) {
		perror("waitpid failed");
		return -1;
	}

	if (WIFSIGNALED(status)) {
		perror("signal");
		return -1;
	}

	if (!WIFEXITED(status)) {
		perror("should not happen");
		return -1;
	}

	return WEXITSTATUS(status);
}

#define SCRIPT_INIT_DB "/usr/bin/email-service_init_db.sh"

INTERNAL_FUNC int emstorage_init_db(char *multi_user_name)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	char *prefix_path = NULL;
	char *output_file_path = NULL;
	char temp_file_path[MAX_PATH] = {0};

	if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		err = emcore_get_container_path(multi_user_name, &prefix_path);
		if (err != EMAIL_ERROR_NONE) {
			if (err != EMAIL_ERROR_CONTAINER_NOT_INITIALIZATION) {
				EM_DEBUG_EXCEPTION("emcore_get_container_path failed :[%d]", err);
				goto FINISH_OFF;
			}
		}
	} else {
		prefix_path = strdup("");
	}

	if (err == EMAIL_ERROR_CONTAINER_NOT_INITIALIZATION) {
		err = emcore_get_canonicalize_path((char *)EMAIL_SERVICE_DB_FILE_PATH, &output_file_path);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_canonicalize_path failed : [%d]", err);
			goto FINISH_OFF;
		}

		SNPRINTF(temp_file_path, sizeof(temp_file_path), "%s", output_file_path);
	} else {
		SNPRINTF(temp_file_path, sizeof(temp_file_path), "%s%s", prefix_path, EMAIL_SERVICE_DB_FILE_PATH);
	}

	EM_DEBUG_LOG("db file path : [%s]", temp_file_path);

	if (!g_file_test(temp_file_path, G_FILE_TEST_EXISTS)) {
		int ret = true;
		const char *argv_script[] = {"/bin/sh", SCRIPT_INIT_DB, NULL};
		ret = _xsystem(argv_script);

		if (ret == -1) {
			EM_DEBUG_EXCEPTION("_xsystem failed");
			err = EMAIL_ERROR_SYSTEM_FAILURE;
		}
	}

FINISH_OFF:

	EM_SAFE_FREE(prefix_path);
	EM_SAFE_FREE(output_file_path);

	EM_DEBUG_FUNC_END();
	return err;
}

INTERNAL_FUNC int em_db_open(char *db_file_path, sqlite3 **sqlite_handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int rc = 0;
	int error = EMAIL_ERROR_NONE;
	int ret = false;

	EM_DEBUG_LOG_DEV("*sqlite_handle[%p]", *sqlite_handle);

	if (*sqlite_handle) { /*prevent 33351*/
		EM_DEBUG_LOG_DEV(">>>>> DB Already Opened......");
		if (err_code != NULL)
			*err_code = error;
		return true;
	}

    EM_DEBUG_LOG("DB file path : [%s]", db_file_path);

	/*  db open */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_open(db_file_path, sqlite_handle), rc);
	if (SQLITE_OK != rc) {
		EM_DEBUG_EXCEPTION("sqlite3_open fail:%d -%s", rc, sqlite3_errmsg(*sqlite_handle));
		if (SQLITE_PERM == rc || SQLITE_CANTOPEN == rc) {
			error = EMAIL_ERROR_PERMISSION_DENIED;
		} else {
			error = EMAIL_ERROR_DB_FAILURE;
		}
		sqlite3_close(*sqlite_handle);
		*sqlite_handle = NULL;

		if (SQLITE_CORRUPT == rc) /* SQLITE_CORRUPT : The database disk image is malformed */ {/* Recovery DB file */
			EM_DEBUG_LOG("The database disk image is malformed. Trying to remove and create database disk image and directories");
			if (!_recovery_from_malformed_db_file(db_file_path, &error)) {
				EM_DEBUG_EXCEPTION("_recovery_from_malformed_db_file failed [%d]", error);
				goto FINISH_OFF;
			}

			EM_DEBUG_LOG("Open DB again");
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_open(db_file_path, sqlite_handle), rc);
			if (SQLITE_OK != rc) {
				EM_DEBUG_EXCEPTION("sqlite3_open fail:%d -%s", rc, sqlite3_errmsg(*sqlite_handle));
				if (SQLITE_PERM == rc) {
					error = EMAIL_ERROR_PERMISSION_DENIED;
				} else {
					error = EMAIL_ERROR_DB_FAILURE;
				}
				sqlite3_close(*sqlite_handle);
				*sqlite_handle = NULL;
				goto FINISH_OFF; /*prevent 33351*/
			}
		} else
			goto FINISH_OFF;
	}

	/* register busy handler */
	EM_DEBUG_LOG_DEV(">>>>> Register DB Handle to busy handler: *sqlite_handle[%p]", *sqlite_handle);
	rc = sqlite3_busy_handler(*sqlite_handle, _callback_sqlite_busy_handler, NULL);  /*  Busy Handler registration, NULL is a parameter which will be passed to handler */
	if (SQLITE_OK != rc) {
		EM_DEBUG_EXCEPTION("sqlite3_busy_handler fail:%d -%s", rc, sqlite3_errmsg(*sqlite_handle));
		error = EMAIL_ERROR_DB_FAILURE;
		sqlite3_close(*sqlite_handle);
		*sqlite_handle = NULL;
		goto FINISH_OFF;
	}

	/* Register collation callback function */
	rc = sqlite3_create_collation(*sqlite_handle, "CONVERTUTF8", SQLITE_UTF8, NULL, _callback_collation_utf7_sort);
	if (SQLITE_OK != rc) {
		EM_DEBUG_EXCEPTION("sqlite3_create_collation failed : [%d][%s]", rc, sqlite3_errmsg(*sqlite_handle));
		error = EMAIL_ERROR_DB_FAILURE;
		sqlite3_close(*sqlite_handle);
		*sqlite_handle = NULL;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC sqlite3* emstorage_db_open(char *multi_user_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	sqlite3 *_db_handle = NULL;

	int error = EMAIL_ERROR_NONE;
	char *prefix_path = NULL;

	_db_handle = emstorage_get_db_handle(multi_user_name);

	if (_db_handle == NULL) {
		char *output_file_path = NULL;
        char temp_file_path[MAX_PATH] = {0};

        if (EM_SAFE_STRLEN(multi_user_name) > 0) {
			error = emcore_get_container_path(multi_user_name, &prefix_path);
			if (error != EMAIL_ERROR_CONTAINER_NOT_INITIALIZATION && error != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_get_container_path failed :[%d]", error);
				goto FINISH_OFF;
			}
        } else {
            prefix_path = strdup("");
        }

		if (error == EMAIL_ERROR_CONTAINER_NOT_INITIALIZATION) {
			if ((error = emcore_get_canonicalize_path((char *)EMAIL_SERVICE_DB_FILE_PATH, &output_file_path)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_get_canonicalize_path failed : [%d]", error);
				goto FINISH_OFF;
			}

			SNPRINTF(temp_file_path, sizeof(temp_file_path), "%s", output_file_path);
			EM_SAFE_FREE(output_file_path);
		} else {
			SNPRINTF(temp_file_path, sizeof(temp_file_path), "%s%s", prefix_path, EMAIL_SERVICE_DB_FILE_PATH);
		}

		if (!em_db_open(temp_file_path, &_db_handle, &error)) {
			EM_DEBUG_EXCEPTION("em_db_open failed[%d]", error);
			goto FINISH_OFF;
		}

		_initialize_shm_mutex(SHM_FILE_FOR_DB_LOCK, &shm_fd_for_db_lock, &mapped_for_db_lock);

#ifdef __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__
		_initialize_shm_mutex(SHM_FILE_FOR_MAIL_ID_LOCK, &shm_fd_for_generating_mail_id, &mapped_for_generating_mail_id);
#endif /*__FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__ */

		emstorage_set_db_handle(multi_user_name, _db_handle);

		emstorage_initialize_field_count();
	}

FINISH_OFF:

	EM_SAFE_FREE(prefix_path);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%p]", _db_handle);
	return _db_handle;
}

INTERNAL_FUNC int emstorage_db_close(char *multi_user_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
#ifdef _MULTIPLE_DB_HANDLE
	sqlite3 *_db_handle = emstorage_get_db_handle(multi_user_name);
#endif

	int error = EMAIL_ERROR_NONE;
	int ret = false;

	if (_db_handle) {
		ret = sqlite3_close(_db_handle);
		if (ret != SQLITE_OK) {
			EM_DEBUG_EXCEPTION(" sqlite3_close fail - %d", ret);
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

INTERNAL_FUNC int emstorage_open(char *multi_user_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

    int ret = false;
    int error = EMAIL_ERROR_NONE;
    int retValue;
	char *prefix_path = NULL;
    char buf[MAX_PATH] = {0};

    if (EM_SAFE_STRLEN(multi_user_name) <= 0) {
        SNPRINTF(buf, sizeof(buf), "%s", DB_PATH);
    } else {
		error = emcore_get_container_path(multi_user_name, &prefix_path);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_container_path failed : [%d]", error);
			goto FINISH_OFF;
		}
        SNPRINTF(buf, sizeof(buf), "%s/%s", prefix_path, DB_PATH);
    }

	if (!g_file_test(buf, G_FILE_TEST_EXISTS)) {
	    retValue = mkdir(buf, DIRECTORY_PERMISSION);

		EM_DEBUG_LOG("mkdir return- %d", retValue);
	    EM_DEBUG_LOG("emstorage_open - before sqlite3_open - pid = %d", getpid());
	}

    if (emstorage_db_open(multi_user_name, &error) == NULL) {
        EM_DEBUG_EXCEPTION("emstorage_db_open failed[%d]", error);
        goto FINISH_OFF;
    }

    if (_open_counter++ == 0) {
        _emstorage_open_once(multi_user_name, &error);
    }

    ret = true;

FINISH_OFF:

	EM_SAFE_FREE(prefix_path);

    if (err_code != NULL)
        *err_code = error;

    EM_DEBUG_FUNC_END("ret [%d]", ret);
    return ret;
}

static int emstorage_get_field_count_from_create_table_query(char *input_create_table_query, int *output_field_count)
{
	EM_DEBUG_FUNC_BEGIN("input_create_table_query[%d], output_field_count[%p]", input_create_table_query, output_field_count);
	int err = EMAIL_ERROR_NONE;
	int field_count = 0;
	char *pos = NULL;

	if (input_create_table_query == NULL || output_field_count == NULL) {
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	pos = input_create_table_query;

	do {
		field_count++;
		if (pos == NULL || *pos == (char)0)
			break;
		pos += 1;
		pos = strchr(pos, ',');
	} while (pos);

	*output_field_count = field_count;

	EM_DEBUG_LOG_DEV("field_count [%d]", field_count);

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emstorage_initialize_field_count()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int query_len = 0;
	char **create_table_query = NULL;

	if (_field_count_of_table[CREATE_TABLE_MAIL_ACCOUNT_TBL] != 0) {
		err = EMAIL_ERROR_ALREADY_INITIALIZED;
		goto FINISH_OFF;
	}

	err = emcore_load_query_from_file((char *)EMAIL_SERVICE_CREATE_TABLE_QUERY_FILE_PATH, &create_table_query, &query_len);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_load_sql_from_file failed [%d]", err);
		goto FINISH_OFF;
	}

	emstorage_get_field_count_from_create_table_query(create_table_query[CREATE_TABLE_MAIL_ACCOUNT_TBL], &(_field_count_of_table[CREATE_TABLE_MAIL_ACCOUNT_TBL]));
	emstorage_get_field_count_from_create_table_query(create_table_query[CREATE_TABLE_MAIL_BOX_TBL], &(_field_count_of_table[CREATE_TABLE_MAIL_BOX_TBL]));
	emstorage_get_field_count_from_create_table_query(create_table_query[CREATE_TABLE_MAIL_READ_MAIL_UID_TBL], &(_field_count_of_table[CREATE_TABLE_MAIL_READ_MAIL_UID_TBL]));
	emstorage_get_field_count_from_create_table_query(create_table_query[CREATE_TABLE_MAIL_RULE_TBL], &(_field_count_of_table[CREATE_TABLE_MAIL_RULE_TBL]));
	emstorage_get_field_count_from_create_table_query(create_table_query[CREATE_TABLE_MAIL_TBL], &(_field_count_of_table[CREATE_TABLE_MAIL_TBL]));
	emstorage_get_field_count_from_create_table_query(create_table_query[CREATE_TABLE_MAIL_ATTACHMENT_TBL], &(_field_count_of_table[CREATE_TABLE_MAIL_ATTACHMENT_TBL]));
	emstorage_get_field_count_from_create_table_query(create_table_query[CREATE_TABLE_MAIL_PARTIAL_BODY_ACTIVITY_TBL], &(_field_count_of_table[CREATE_TABLE_MAIL_PARTIAL_BODY_ACTIVITY_TBL]));
	emstorage_get_field_count_from_create_table_query(create_table_query[CREATE_TABLE_MAIL_MEETING_TBL], &(_field_count_of_table[CREATE_TABLE_MAIL_MEETING_TBL]));
#ifdef __FEATURE_LOCAL_ACTIVITY__
	emstorage_get_field_count_from_create_table_query(create_table_query[CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL], &(_field_count_of_table[CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL]));
#endif /* __FEATURE_LOCAL_ACTIVITY__ */
	emstorage_get_field_count_from_create_table_query(create_table_query[CREATE_TABLE_MAIL_CERTIFICATE_TBL], &(_field_count_of_table[CREATE_TABLE_MAIL_CERTIFICATE_TBL]));
	emstorage_get_field_count_from_create_table_query(create_table_query[CREATE_TABLE_MAIL_TASK_TBL], &(_field_count_of_table[CREATE_TABLE_MAIL_TASK_TBL]));
	emstorage_get_field_count_from_create_table_query(create_table_query[CREATE_TABLE_MAIL_TEXT_TBL], &(_field_count_of_table[CREATE_TABLE_MAIL_TEXT_TBL]));
FINISH_OFF:

	if (create_table_query) {
		int i = 0;
		for (i = 0; i < query_len; i++) {
			if (create_table_query[i]) {
				EM_SAFE_FREE(create_table_query[i]);
			}
		}
		EM_SAFE_FREE(create_table_query);
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emstorage_create_table(char *multi_user_name, emstorage_create_db_type_t type, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int error = EMAIL_ERROR_NONE;
	int rc = -1, ret = false;
	int query_len = 0;
	char sql_query_string[QUERY_SIZE] = {0, };
	char **create_table_query = NULL;

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	error = emcore_load_query_from_file((char *)EMAIL_SERVICE_CREATE_TABLE_QUERY_FILE_PATH, &create_table_query, &query_len);
	if (error != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_load_sql_from_file failed [%d]", error);
		goto FINISH_OFF2;
	}

	if (query_len < CREATE_TABLE_MAX) {
		EM_DEBUG_EXCEPTION("SQL string array length is difference from CREATE_TABLE_MAX");
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF2;
	}

	EM_DEBUG_LOG("local_db_handle = %p.", local_db_handle);

	char *sql;
	char **result = NULL;

	/*  1. create mail_account_tbl */
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
		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_ACCOUNT_TBL], sizeof(sql_query_string)-1); /*prevent 21984*/
		error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
		}

		/*  create mail_account_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "%s", create_table_query[CREATE_TABLE_MAIL_ACCOUNT_IDX]);
		error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
		}

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} /*  mail_account_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_ACCOUNT_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; }, ("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_ACCOUNT_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}

	sqlite3_free_table(result);
	result = NULL;

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

		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_BOX_TBL], sizeof(sql_query_string)-1); /*prevent 21984*/
		error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
		}

		/*  create mail_local_mailbox_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "%s", create_table_query[CREATE_TABLE_MAIL_BOX_IDX]);
		error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
		}

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} /*  mail_box_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK) {
		rc = sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_BOX_TBL], NULL, NULL, NULL);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_BOX_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	result = NULL;

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

		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_READ_MAIL_UID_TBL], sizeof(sql_query_string)-1); /*prevent 21984*/
		error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
		}

		/*  create mail_read_mail_uid_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "%s", create_table_query[CREATE_TABLE_MAIL_READ_MAIL_UID_IDX]);
		error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
		}

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} /*  mail_read_mail_uid_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_READ_MAIL_UID_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_READ_MAIL_UID_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	result = NULL;

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

		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_RULE_TBL], sizeof(sql_query_string)-1); /*prevent 21984*/
                error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
                if (error != EMAIL_ERROR_NONE) {
                        EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
                        goto FINISH_OFF;
                }

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} /*  mail_rule_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_RULE_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_RULE_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	result = NULL;

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

		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_TBL], sizeof(sql_query_string)-1); /*prevent 21984*/
                error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
                if (error != EMAIL_ERROR_NONE) {
                        EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
                        goto FINISH_OFF;
                }

		/*  create mail_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "%s", create_table_query[CREATE_TABLE_MAIL_IDX]);
                error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
                if (error != EMAIL_ERROR_NONE) {
                        EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
                        goto FINISH_OFF;
                }

		/*  create mail_tbl index for date_time */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "%s", create_table_query[CREATE_TABLE_MAIL_DATETIME_IDX]);
                error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
                if (error != EMAIL_ERROR_NONE) {
                        EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
                        goto FINISH_OFF;
                }

		/*  create mail_tbl index for thread_item_count */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "%s", create_table_query[CREATE_TABLE_MAIL_THREAD_IDX]);
                error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
                if (error != EMAIL_ERROR_NONE) {
                        EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
                        goto FINISH_OFF;
                }

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		/*  just one time call */
/* 		EFTSInitFTSIndex(FTS_EMAIL_IDX); */
	} /*  mail_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	result = NULL;

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

		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_ATTACHMENT_TBL], sizeof(sql_query_string)-1); /*prevent 21984*/
                error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
                if (error != EMAIL_ERROR_NONE) {
                        EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
                        goto FINISH_OFF;
                }

		/*  create mail_attachment_tbl unique index */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "%s", create_table_query[CREATE_TABLE_MAIL_ATTACHMENT_IDX]);
                error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
                if (error != EMAIL_ERROR_NONE) {
                        EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
                        goto FINISH_OFF;
                }

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} /*  mail_attachment_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_ATTACHMENT_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_ATTACHMENT_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	result = NULL;

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

		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_PARTIAL_BODY_ACTIVITY_TBL], sizeof(sql_query_string)-1); /*prevent 21984*/
                error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
                if (error != EMAIL_ERROR_NONE) {
                        EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
                        goto FINISH_OFF;
                }

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} /*  mail_rule_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_PARTIAL_BODY_ACTIVITY_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_PARTIAL_BODY_ACTIVITY_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	result = NULL;

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

		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_MEETING_TBL], sizeof(sql_query_string)-1); /*prevent 21984*/
                error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
                if (error != EMAIL_ERROR_NONE) {
                        EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
                        goto FINISH_OFF;
                }

		SNPRINTF(sql_query_string, sizeof(sql_query_string), "%s", create_table_query[CREATE_TABLE_MAIL_MEETING_IDX]);
                error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
                if (error != EMAIL_ERROR_NONE) {
                        EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
                        goto FINISH_OFF;
                }

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} /*  mail_contact_sync_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_MEETING_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_MEETING_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}
	sqlite3_free_table(result);
	result = NULL;

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
			error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
			if (error != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
				goto FINISH_OFF;
			}

			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
		} /*  mail_rule_tbl */
		else if (type == EMAIL_CREATE_DB_CHECK) {
			rc = sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL], NULL, NULL, NULL);
			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL], NULL, NULL, NULL), rc);
			EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
				("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_LOCAL_ACTIVITY_TBL], rc, sqlite3_errmsg(local_db_handle)));
		}
		sqlite3_free_table(result);
		result = NULL;
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

		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_CERTIFICATE_TBL], sizeof(sql_query_string)-1); /*prevent 21984*/
                error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
                if (error != EMAIL_ERROR_NONE) {
                        EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
                        goto FINISH_OFF;
                }

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} /*  mail_contact_sync_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_CERTIFICATE_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; }, ("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_CERTIFICATE_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}

	sqlite3_free_table(result);
	result = NULL;

	/*  create mail_task_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_task_tbl';";
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; }, ("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (atoi(result[1]) < 1) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; }, ("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

		EM_DEBUG_LOG("CREATE TABLE mail_task_tbl");

		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_TASK_TBL], sizeof(sql_query_string)-1); /*prevent 21984 */
                error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
                if (error != EMAIL_ERROR_NONE) {
                        EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
                        goto FINISH_OFF;
                }

		SNPRINTF(sql_query_string, sizeof(sql_query_string), "%s", create_table_query[CREATE_TABLE_MAIL_TASK_IDX]);
                error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
                if (error != EMAIL_ERROR_NONE) {
                        EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
                        goto FINISH_OFF;
                }

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} /*  mail_task_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_TASK_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; }, ("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_TASK_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}

	sqlite3_free_table(result);
	result = NULL;

#ifdef __FEATURE_BODY_SEARCH__
	/*  create mail_text_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_text_tbl';";
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; }, ("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (atoi(result[1]) < 1) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; }, ("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

		EM_DEBUG_LOG("CREATE TABLE mail_text_tbl");

		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_TEXT_TBL], sizeof(sql_query_string)-1); /*prevent 21984 */
		error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
		}

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} /*  mail_text_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_TEXT_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; }, ("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_TEXT_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}

	sqlite3_free_table(result);
	result = NULL;

#endif

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
	/*  create mail_auto_download_activity_tbl */
	sql = "SELECT count(name) FROM sqlite_master WHERE name='mail_auto_download_activity_tbl';";
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; }, ("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (atoi(result[1]) < 1) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; }, ("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

		EM_DEBUG_LOG("CREATE TABLE mail_auto_download_activity_tbl");

		EM_SAFE_STRNCPY(sql_query_string, create_table_query[CREATE_TABLE_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL], sizeof(sql_query_string)-1);
		error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
		}

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} /*  mail_auto_download_activity_tbl */
	else if (type == EMAIL_CREATE_DB_CHECK) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, g_test_query[CREATE_TABLE_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL], NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; }, ("SQL(%s) exec fail:%d -%s", g_test_query[CREATE_TABLE_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL], rc, sqlite3_errmsg(local_db_handle)));
	}

	sqlite3_free_table(result);
	result = NULL;
#endif


	ret = true;

FINISH_OFF:

	if (result) sqlite3_free_table(result);

	if (ret == true) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} else {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "rollback", NULL, NULL, NULL), rc);
	}

FINISH_OFF2:
	if (create_table_query) {
		int i = 0;
		for (i = 0; i < query_len; i++) {
			if (create_table_query[i]) {
				EM_SAFE_FREE(create_table_query[i]);
			}
		}
		EM_SAFE_FREE(create_table_query);
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/* Query series --------------------------------------------------------------*/
INTERNAL_FUNC int emstorage_query_mail_count(char *multi_user_name, const char *input_conditional_clause, int input_transaction, int *output_total_mail_count, int *output_unseen_mail_count)
{
	EM_DEBUG_FUNC_BEGIN("input_conditional_clause[%p], input_transaction[%d], output_total_mail_count[%p], output_unseen_mail_count[%p]", input_conditional_clause, input_transaction, output_total_mail_count, output_unseen_mail_count);
	int rc = -1;
	int query_size = 0;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char *sql_query_string = NULL;
	char **result;
	sqlite3 *local_db_handle = NULL;

	if (!input_conditional_clause || (!output_total_mail_count && !output_unseen_mail_count)) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	query_size = EM_SAFE_STRLEN(input_conditional_clause) + QUERY_SIZE;
	sql_query_string = em_malloc(query_size);
	if (sql_query_string == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(input_transaction);

	SNPRINTF(sql_query_string, query_size, "SELECT COUNT(*) FROM mail_tbl");
	EM_SAFE_STRCAT(sql_query_string, (char*)input_conditional_clause);

	if (output_total_mail_count) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
		EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("sqlite3_prepare failed [%d] [%s]", rc, sql_query_string));

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("sqlite3_step failed [%d] [%s]", rc, sql_query_string));
		_get_stmt_field_data_int(hStmt, output_total_mail_count, 0);
	}

	if (output_unseen_mail_count) {
		EM_SAFE_STRCAT(sql_query_string, " AND flags_seen_field = 0 ");

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
			("sqlite3_get_table failed [%d] [%s]", rc, sql_query_string));

		*output_unseen_mail_count = atoi(result[1]);
		sqlite3_free_table(result);
	}

FINISH_OFF:

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(input_transaction);

	EM_SAFE_FREE(sql_query_string);

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int emstorage_query_mail_id_list(char *multi_user_name, const char *input_conditional_clause, int input_transaction, int **output_mail_id_list, int *output_mail_id_count)
{
	EM_DEBUG_FUNC_BEGIN("input_conditional_clause [%p], input_transaction [%d], output_mail_id_list [%p], output_mail_id_count [%p]", input_conditional_clause, input_transaction, output_mail_id_list, output_mail_id_count);

	int      i = 0;
	int      count = 0;
	int      rc = -1;
	int      cur_query = 0;
	int      col_index;
	int	 query_size = 0;
	int      error = EMAIL_ERROR_NONE;
	int     *result_mail_id_list = NULL;
	char   **result = NULL;
	char     *sql_query_string = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EM_IF_NULL_RETURN_VALUE(input_conditional_clause, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(output_mail_id_list, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(output_mail_id_count, EMAIL_ERROR_INVALID_PARAM);

	query_size = strlen(input_conditional_clause) + strlen("SELECT mail_id FROM mail_tbl ") + 10;  // 10 is extra space
	sql_query_string = em_malloc(query_size);
	if (sql_query_string == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	EMSTORAGE_START_READ_TRANSACTION(input_transaction);

	/* Composing query */
	SNPRINTF_OFFSET(sql_query_string, cur_query, query_size, "SELECT mail_id FROM mail_tbl ");
	EM_SAFE_STRCAT(sql_query_string, (char*)input_conditional_clause);

	EM_DEBUG_LOG_SEC("query[%s].", sql_query_string);

	/* Performing query */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	col_index = 1;

	/*  to get mail list */
	if (count == 0) {
		EM_DEBUG_LOG("No mail found...");
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

	if (result)
		sqlite3_free_table(result);

	EMSTORAGE_FINISH_READ_TRANSACTION(input_transaction);

	EM_SAFE_FREE(sql_query_string);

	if (error != EMAIL_ERROR_NONE)
		EM_SAFE_FREE(result_mail_id_list);

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int emstorage_query_mail_list(char *multi_user_name, const char *conditional_clause, int transaction, email_mail_list_item_t** result_mail_list,  int *result_count,  int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_PROFILE_BEGIN(emstorage_query_mail_list_func);

	int i = 0, count = 0, rc = -1, to_get_count = (result_mail_list) ? 0 : 1;
	int sql_query_string_length = 0;
	int local_inline_content_count = 0, local_attachment_count = 0;
	int cur_query = 0, base_count = 0, col_index;
	int ret = false, error = EMAIL_ERROR_NONE;
	char *date_time_string = NULL;
	char **result = NULL;
	char *field_mail_id = "mail_id";
	char *field_all     = "mail_id, account_id, mailbox_id, mailbox_type, full_address_from, email_address_sender, full_address_to, subject, body_download_status, mail_size, flags_seen_field, flags_deleted_field, flags_flagged_field, flags_answered_field, flags_recent_field, flags_draft_field, flags_forwarded_field, DRM_status, priority, save_status, lock_status, attachment_count, inline_content_count, date_time, preview_text, thread_id, thread_item_count, meeting_request_status, message_class, smime_type, scheduled_sending_time, remaining_resend_times, tag_id, eas_data_length, eas_data ";
	char *select_query_form = "SELECT %s FROM mail_tbl ";
	char *target_field = NULL;
	char *sql_query_string = NULL;
	email_mail_list_item_t *mail_list_item_from_tbl = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EM_IF_NULL_RETURN_VALUE(conditional_clause, false);
	EM_IF_NULL_RETURN_VALUE(result_count, false);

	EMSTORAGE_START_READ_TRANSACTION(transaction);

	/*  select clause */
	if (to_get_count) /*  count only */
		target_field = field_mail_id;
	else /* mail list in plain form */
		target_field = field_all;

	sql_query_string_length = EM_SAFE_STRLEN(select_query_form) + EM_SAFE_STRLEN(target_field) + EM_SAFE_STRLEN(conditional_clause);

	if (sql_query_string_length)
		sql_query_string = em_malloc(sql_query_string_length);

	if (sql_query_string == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	cur_query += SNPRINTF_OFFSET(sql_query_string, cur_query, sql_query_string_length, select_query_form, target_field);

	strncat(sql_query_string, conditional_clause, sql_query_string_length - cur_query);

	EM_DEBUG_LOG_DEV("query[%s]", sql_query_string);

	/*  performing query */
	EM_PROFILE_BEGIN(emstorage_query_mail_list_performing_query);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_get_table failed [%d] [%s]", rc, sql_query_string));
	EM_PROFILE_END(emstorage_query_mail_list_performing_query);

	if (!base_count)
		base_count = ({
				int i = 0;
				char *tmp = NULL;
				for (tmp = field_all; tmp && *(tmp + 1); tmp = index(tmp + 1, ',')) i++ ;
				i;
				});

	col_index = base_count;

	EM_DEBUG_LOG_DEV("base_count [%d]", base_count);

	if (to_get_count) {
		/*  to get count */
		if (!count) {
			EM_DEBUG_LOG_DEV("No mail found...");
			ret = false;
			error = EMAIL_ERROR_MAIL_NOT_FOUND;
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG_DEV("There are [%d] mails.", count);
	} else {
		/*  to get mail list */
		if (!count) {
			EM_DEBUG_LOG_DEV("No mail found...");
			ret = false;
			error = EMAIL_ERROR_MAIL_NOT_FOUND;
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG_DEV("There are [%d] mails.", count);
		if (!(mail_list_item_from_tbl = (email_mail_list_item_t*)em_malloc(sizeof(email_mail_list_item_t) * count))) {
			EM_DEBUG_EXCEPTION("malloc for mail_list_item_from_tbl failed...");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		EM_PROFILE_BEGIN(emstorage_query_mail_list_loop);
		EM_DEBUG_LOG_DEV(">>>> DATA ASSIGN START >> ");
		for (i = 0; i < count; i++) {
			_get_table_field_data_int(result, &(mail_list_item_from_tbl[i].mail_id), col_index++);
			_get_table_field_data_int(result, &(mail_list_item_from_tbl[i].account_id), col_index++);
			_get_table_field_data_int(result, &(mail_list_item_from_tbl[i].mailbox_id), col_index++);
			_get_table_field_data_int(result, (int*)&(mail_list_item_from_tbl[i].mailbox_type), col_index++);
			_get_table_field_data_string_without_allocation(result, mail_list_item_from_tbl[i].full_address_from, STRING_LENGTH_FOR_DISPLAY, 1, col_index++);
			_get_table_field_data_string_without_allocation(result, mail_list_item_from_tbl[i].email_address_sender, MAX_EMAIL_ADDRESS_LENGTH, 1, col_index++);
			_get_table_field_data_string_without_allocation(result, mail_list_item_from_tbl[i].email_address_recipient, STRING_LENGTH_FOR_DISPLAY,  1, col_index++);
			_get_table_field_data_string_without_allocation(result, mail_list_item_from_tbl[i].subject, STRING_LENGTH_FOR_DISPLAY, 1, col_index++);
			_get_table_field_data_int(result, &(mail_list_item_from_tbl[i].body_download_status), col_index++);
			_get_table_field_data_int(result, &(mail_list_item_from_tbl[i].mail_size), col_index++);
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
			_get_table_field_data_int(result, (int*)&(mail_list_item_from_tbl[i].scheduled_sending_time), col_index++);
			_get_table_field_data_int(result, (int*)&(mail_list_item_from_tbl[i].remaining_resend_times), col_index++);
			_get_table_field_data_int(result, (int*)&(mail_list_item_from_tbl[i].tag_id), col_index++);
			_get_table_field_data_int(result, (int*)&(mail_list_item_from_tbl[i].eas_data_length), col_index++);
			_get_table_field_data_blob(result, (void**)&(mail_list_item_from_tbl[i].eas_data), mail_list_item_from_tbl[i].eas_data_length, col_index++);

			mail_list_item_from_tbl[i].attachment_count = (local_attachment_count > 0) ? 1 : 0;
		}
		EM_DEBUG_LOG_DEV(">>>> DATA ASSIGN END [count : %d] >> ", count);
		EM_PROFILE_END(emstorage_query_mail_list_loop);
	}

	ret = true;

FINISH_OFF:
	EM_DEBUG_LOG("MAIL_COUNT [%d]", count);

	if (result)
		sqlite3_free_table(result);

	if (to_get_count)
		*result_count = count;
	else {
		if (ret == true) {
			if (result_mail_list)
				*result_mail_list = mail_list_item_from_tbl;
			*result_count = count;
		} else
			EM_SAFE_FREE(mail_list_item_from_tbl);
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

//	sqlite3_db_release_memory(local_db_handle);

	EM_SAFE_FREE(sql_query_string);
	EM_SAFE_FREE(date_time_string);

	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(emstorage_query_mail_list_func);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_query_mail_tbl(char *multi_user_name, const char *conditional_clause, int transaction, emstorage_mail_tbl_t** result_mail_tbl, int *result_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("conditional_clause[%s], result_mail_tbl[%p], result_count [%p], transaction[%d], err_code[%p]", conditional_clause, result_mail_tbl, result_count, transaction, err_code);

	if (!conditional_clause || !result_mail_tbl || !result_count) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM: [%p] [%p] [%p]", conditional_clause, result_mail_tbl, result_mail_tbl);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int i, col_index = FIELD_COUNT_OF_MAIL_TBL, rc, ret = false, count;
	int error = EMAIL_ERROR_NONE;
	char **result = NULL, sql_query_string[QUERY_SIZE] = {0, };
	emstorage_mail_tbl_t* p_data_tbl = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_tbl %s", conditional_clause);

	EM_DEBUG_LOG_DEV("Query[%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("sqlite3_get_table failed [%d] [%s]", rc, sql_query_string));

	if (!count) {
		EM_DEBUG_LOG("No mail found...");
		ret = false;
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("There are [%d] mails.", count);
	if (!(p_data_tbl = (emstorage_mail_tbl_t*)em_malloc(sizeof(emstorage_mail_tbl_t) * count))) {
		EM_DEBUG_EXCEPTION("malloc for emstorage_mail_tbl_t failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_DEV(">>>> DATA ASSIGN START >> ");
	for (i = 0; i < count; i++) {
		_get_table_field_data_int(result, &(p_data_tbl[i].mail_id), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].account_id), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].mailbox_id), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].mailbox_type), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].subject), 1, col_index++);
		_get_table_field_data_time_t (result, &(p_data_tbl[i].date_time), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].server_mail_status), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].server_mailbox_name), 0, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].server_mail_id), 0, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].message_id), 0, col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].reference_mail_id), col_index++);
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
		_get_table_field_data_int(result, &(p_data_tbl[i].body_download_status), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].file_path_plain), 0, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].file_path_html), 0, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].file_path_mime_entity), 0, col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].mail_size), col_index++);
		_get_table_field_data_char  (result, &(p_data_tbl[i].flags_seen_field), col_index++);
		_get_table_field_data_char  (result, &(p_data_tbl[i].flags_deleted_field), col_index++);
		_get_table_field_data_char  (result, &(p_data_tbl[i].flags_flagged_field), col_index++);
		_get_table_field_data_char  (result, &(p_data_tbl[i].flags_answered_field), col_index++);
		_get_table_field_data_char  (result, &(p_data_tbl[i].flags_recent_field), col_index++);
		_get_table_field_data_char  (result, &(p_data_tbl[i].flags_draft_field), col_index++);
		_get_table_field_data_char  (result, &(p_data_tbl[i].flags_forwarded_field), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].DRM_status), col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].priority), col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].save_status), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].lock_status), col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].report_status), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].attachment_count), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].inline_content_count), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].thread_id), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].thread_item_count), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].preview_text), 1, col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].meeting_request_status), col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].message_class), col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].digest_type), col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].smime_type), col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].scheduled_sending_time), col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].remaining_resend_times), col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].tag_id), col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].replied_time), col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].forwarded_time), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].default_charset), 0, col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].eas_data_length), col_index++);
		_get_table_field_data_blob(result, (void**)&(p_data_tbl[i].eas_data), p_data_tbl[i].eas_data_length, col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].user_name), 0, col_index++);
	}

	ret = true;

FINISH_OFF:
	if (result)
		sqlite3_free_table(result);

	if (ret) {
		*result_mail_tbl = p_data_tbl;
		*result_count = count;
	} else {
		*result_mail_tbl = NULL;
		*result_count = 0;
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

//	sqlite3_db_release_memory(local_db_handle);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

#ifdef __FEATURE_BODY_SEARCH__
INTERNAL_FUNC int emstorage_query_mail_text_tbl(char *multi_user_name, const char *conditional_clause, int transaction, emstorage_mail_text_tbl_t** result_mail_text_tbl, int *result_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("conditional_clause[%s], result_mail_text_tbl[%p], result_count [%p], transaction[%d], err_code[%p]", conditional_clause, result_mail_text_tbl, result_count, transaction, err_code);

	if (!conditional_clause || !result_mail_text_tbl || !result_count) { /*prevent 50930*/
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int i = 0;
	int rc = 0;
	int ret = false;
	int count = 0;
	int col_index = FIELD_COUNT_OF_MAIL_TEXT_TBL;
	int error = EMAIL_ERROR_NONE;
	char **result = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	emstorage_mail_text_tbl_t* p_data_tbl = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_text_tbl %s", conditional_clause);

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (!count) {
		EM_DEBUG_LOG("No mail found...");
		ret = false;
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("There are [%d] mails.", count);
	if (!(p_data_tbl = (emstorage_mail_text_tbl_t *)em_malloc(sizeof(emstorage_mail_text_tbl_t) * count))) {
		EM_DEBUG_EXCEPTION("malloc for emstorage_mail_text_tbl_t failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_DEV(">>>> DATA ASSIGN START >>");
	for (i = 0; i < count; i++) {
		_get_table_field_data_int(result, &(p_data_tbl[i].mail_id), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].account_id), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].mailbox_id), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].body_text), 0, col_index++);
	}

	ret = true;

FINISH_OFF:
	if (result)
		sqlite3_free_table(result);

	if (ret == true) {
		*result_mail_text_tbl = p_data_tbl;
		*result_count = count;
	} else
		EM_SAFE_FREE(p_data_tbl);

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

//	sqlite3_db_release_memory(local_db_handle);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
#endif

INTERNAL_FUNC int emstorage_query_mailbox_tbl(char *multi_user_name, const char *input_conditional_clause, const char *input_ordering_clause, int input_get_mail_count,  int input_transaction, emstorage_mailbox_tbl_t **output_mailbox_list, int *output_mailbox_count)
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
	char *fields = "MBT.mailbox_id, MBT.account_id, local_yn, MBT.mailbox_name, MBT.mailbox_type, alias, deleted_flag, modifiable_yn, total_mail_count_on_server, has_archived_mails, mail_slot_size, no_select, last_sync_time, MBT.eas_data_length, MBT.eas_data ";
	emstorage_mailbox_tbl_t* p_data_tbl = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(input_transaction);

	if (input_get_mail_count == 0) {	/* without mail count */
		col_index = 15;
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s FROM mail_box_tbl AS MBT %s %s", fields, input_conditional_clause, input_ordering_clause);
	} else {	/* with read count and total count */
		col_index = 17;
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT %s, total, read  FROM mail_box_tbl AS MBT LEFT OUTER JOIN (SELECT mailbox_id, count(mail_id) AS total, SUM(flags_seen_field) AS read FROM mail_tbl WHERE flags_deleted_field = 0 GROUP BY mailbox_id) AS MT ON MBT.mailbox_id = MT.mailbox_id %s %s", fields, input_conditional_clause, input_ordering_clause);
	}

	EM_DEBUG_LOG_DEV("query[%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("sqlite3_get_table failed [%d] [%s]", rc, sql_query_string))

	EM_DEBUG_LOG_DEV("result count [%d]", count);

	if (count == 0) {
		EM_DEBUG_LOG_SEC("Can't find mailbox query[%s]", sql_query_string);
		error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
		goto FINISH_OFF;
	}

	if ((p_data_tbl = (emstorage_mailbox_tbl_t*)em_malloc(sizeof(emstorage_mailbox_tbl_t) * count)) == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < count; i++) {
		_get_table_field_data_int(result, &(p_data_tbl[i].mailbox_id), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].account_id), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].local_yn), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].mailbox_name), 0, col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].mailbox_type), col_index++);
		_get_table_field_data_string(result, &(p_data_tbl[i].alias), 0, col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].deleted_flag), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].modifiable_yn), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].total_mail_count_on_server), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].has_archived_mails), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].mail_slot_size), col_index++);
		_get_table_field_data_int(result, &(p_data_tbl[i].no_select), col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].last_sync_time), col_index++);
		_get_table_field_data_int(result, (int*)&(p_data_tbl[i].eas_data_length), col_index++);
		_get_table_field_data_blob(result, (void**)&(p_data_tbl[i].eas_data), p_data_tbl[i].eas_data_length, col_index++);

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

	if (error == EMAIL_ERROR_NONE) {
		*output_mailbox_list = p_data_tbl;
		*output_mailbox_count = count;
		EM_DEBUG_LOG("Mailbox Count [%d]", count);
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(input_transaction);

//	sqlite3_db_release_memory(local_db_handle);

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

/* Query series --------------------------------------------------------------*/
INTERNAL_FUNC int emstorage_check_duplicated_account(char *multi_user_name, email_account_t* account, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	char **result;
	int count;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = NULL;

	if (account == NULL || account->user_email_address == NULL || account->incoming_server_user_name == NULL || account->incoming_server_address == NULL ||
		account->outgoing_server_user_name == NULL || account->outgoing_server_address == NULL) {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

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
	EM_DEBUG_LOG_SEC("Query[%s]", sql_query_string);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	count = atoi(result[1]);
	sqlite3_free_table(result);

	EM_DEBUG_LOG("Count of Duplicated Account Information: [%d]", count);

	if (count == 0) {	/*  not duplicated account */
		ret = true;
		EM_DEBUG_LOG_SEC("NOT duplicated account: user_email_address[%s]", account->user_email_address);
	} else {	/*  duplicated account */
		ret = false;
		EM_DEBUG_LOG_SEC("The same account already exists. Duplicated account: user_email_address[%s]", account->user_email_address);
		error = EMAIL_ERROR_ALREADY_EXISTS;
	}

FINISH_OFF:

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}

INTERNAL_FUNC int emstorage_get_account_count(char *multi_user_name, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	if (!count) {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;

	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };


	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_account_tbl");
	EM_DEBUG_LOG_SEC("SQL STMT [%s]", sql_query_string);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string,
                                                          EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("Before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	*count = sqlite3_column_int(hStmt, 0);

	ret = true;

FINISH_OFF:

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_account_list(char *multi_user_name, int *select_num, emstorage_account_tbl_t **account_list, int transaction, int with_password, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int i = 0, count = 0, rc = -1, ret = false;
	int field_index = 0;
	int sql_len = 0;
	int error = EMAIL_ERROR_NONE;
	emstorage_account_tbl_t *p_data_tbl = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *sql = "SELECT count(*) FROM mail_account_tbl;";
	char **result;
	sqlite3 *local_db_handle = NULL;
	DB_STMT hStmt = NULL;

	if (!select_num || !account_list) {
		EM_DEBUG_EXCEPTION("select_num[%p], account_list[%p]", select_num, account_list);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	count = atoi(result[1]);
	sqlite3_free_table(result);

	EM_DEBUG_LOG_DEV("count = %d", rc);

	if (count == 0) {
		EM_DEBUG_LOG("no account found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT ");
	sql_len = EM_SAFE_STRLEN(sql_query_string);

	SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len,
		"account_id, "
		"account_name, "
		"logo_icon_path, "
		"user_data, "
		"user_data_length, "
		"account_svc_id, "
		"sync_status, "
		"sync_disabled, "
		"default_mail_slot_size, "
		"roaming_option, "
		"color_label, "
		"user_display_name, "
		"user_email_address, "
		"reply_to_address, "
		"return_address, "
		"incoming_server_type, "
		"incoming_server_address, "
		"incoming_server_port_number, "
		"incoming_server_user_name, "
		"incoming_server_password, "
		"incoming_server_secure_connection, "
		"incoming_server_authentication_method, "
		"retrieval_mode, "
		"keep_mails_on_pop_server_after_download, "
		"check_interval, "
		"auto_download_size, "
		"peak_interval, "
		"peak_days, "
		"peak_start_time, "
		"peak_end_time, "
		"outgoing_server_type, "
		"outgoing_server_address, "
		"outgoing_server_port_number, "
		"outgoing_server_user_name, "
		"outgoing_server_password, "
		"outgoing_server_secure_connection, "
		"outgoing_server_need_authentication, "
		"outgoing_server_use_same_authenticator, "
		"priority, "
		"keep_local_copy, "
		"req_delivery_receipt, "
		"req_read_receipt, "
		"download_limit, "
		"block_address, "
		"block_subject, "
		"display_name_from, "
		"reply_with_body, "
		"forward_with_files, "
		"add_myname_card, "
		"add_signature, "
		"signature, "
		"add_my_address_to_bcc, "
		"notification_status, "
		"vibrate_status, "
		"display_content_status, "
		"default_ringtone_status, "
		"alert_ringtone_path, "
		"auto_resend_times, "
		"outgoing_server_size_limit, "
		"wifi_auto_download, "
		"pop_before_smtp, "
		"incoming_server_requires_apop, "
		"smime_type, "
		"certificate_path, "
		"cipher_type, "
		"digest_type"
	);

	sql_len = EM_SAFE_STRLEN(sql_query_string);

	SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len, " FROM mail_account_tbl ORDER BY account_id");

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG_DEV("After sqlite3_prepare_v2 hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION("no account found...");

		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		count = 0;
		ret = true;
		goto FINISH_OFF;
	}

	if (!(p_data_tbl = (emstorage_account_tbl_t*)malloc(sizeof(emstorage_account_tbl_t) * count))) {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memset(p_data_tbl, 0x00, sizeof(emstorage_account_tbl_t) * count);
	for (i = 0; i < count; i++) {
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
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl[i].roaming_option), field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl[i].color_label), field_index++);
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
		_get_stmt_field_data_int(hStmt, (int *)&(p_data_tbl[i].incoming_server_authentication_method), field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl[i].retrieval_mode), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].keep_mails_on_pop_server_after_download), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].check_interval), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].auto_download_size), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].peak_interval), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].peak_days), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].peak_start_time), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].peak_end_time), field_index++);
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
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].options.notification_status), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].options.vibrate_status), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].options.display_content_status), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].options.default_ringtone_status), field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].options.alert_ringtone_path), 0, field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].auto_resend_times), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].outgoing_server_size_limit), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].wifi_auto_download), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].pop_before_smtp), field_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].incoming_server_requires_apop), field_index++);
		_get_stmt_field_data_int(hStmt, (int *)&(p_data_tbl[i].smime_type), field_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].certificate_path), 0, field_index++);
		_get_stmt_field_data_int(hStmt, (int *)&(p_data_tbl[i].cipher_type), field_index++);
		_get_stmt_field_data_int(hStmt, (int *)&(p_data_tbl[i].digest_type), field_index++);

		/* EAS passwd is not accessible */
		if (with_password == true && p_data_tbl[i].incoming_server_type != EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
			/*  get password from the secure storage */
			char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH] = {0};
			char send_password_file_name[MAX_PW_FILE_NAME_LENGTH] = {0};

			EM_SAFE_FREE(p_data_tbl[i].incoming_server_password);
			EM_SAFE_FREE(p_data_tbl[i].outgoing_server_password);

			/*  get password file name */
			error = _get_password_file_name(multi_user_name, p_data_tbl[i].account_id,
                                                   recv_password_file_name,
                                                   send_password_file_name);
			if (error != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("_get_password_file_name error [%d]", error);
				error = EMAIL_ERROR_SECURED_STORAGE_FAILURE;
				goto FINISH_OFF;
			}

			/*  read password from secure storage */
			error = _read_password_from_secure_storage(recv_password_file_name,
                                          &(p_data_tbl[i].incoming_server_password));
			if (error < 0) {
				EM_DEBUG_EXCEPTION("_read_password_from_secure_storage()[%s] error [%d]",
                                                    recv_password_file_name, error);
				error = EMAIL_ERROR_SECURED_STORAGE_FAILURE;
				goto FINISH_OFF;
			}
			error = _read_password_from_secure_storage(send_password_file_name,
                                          &(p_data_tbl[i].outgoing_server_password));
			if (error < 0) {
				EM_DEBUG_EXCEPTION("_read_password_from_secure_storage()[%s]  error [%d]",
                                                     send_password_file_name, error);
				error = EMAIL_ERROR_SECURED_STORAGE_FAILURE;
				goto FINISH_OFF;
			}
		}

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_LOG_DEV("after sqlite3_step(), i = %d, rc = %d.", i,  rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
	}

	ret = true;

FINISH_OFF:
	if (ret == true) {
		*account_list = p_data_tbl;
		*select_num = count;
		EM_DEBUG_LOG_DEV("COUNT : %d", count);
	} else if (p_data_tbl != NULL)
		emstorage_free_account(&p_data_tbl, count, NULL);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_maildata_by_servermailid(char *multi_user_name,
															char *server_mail_id,
															int mailbox_id,
															emstorage_mail_tbl_t **mail,
															int transaction,
															int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("mailbox_id [%d], server_mail_id[%s], mail[%p], transaction[%d], err_code[%p]",
							mailbox_id, server_mail_id, mail, transaction, err_code);

	int ret = false, error = EMAIL_ERROR_NONE, result_count;
	char conditional_clause[QUERY_SIZE] = {0, };
	emstorage_mail_tbl_t* p_data_tbl = NULL;

	if (!server_mail_id || !mail) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	sqlite3_snprintf(sizeof(conditional_clause), conditional_clause, "WHERE server_mail_id = '%q'",	server_mail_id);

	if (mailbox_id > 0) {
		SNPRINTF(conditional_clause + strlen(conditional_clause),
					QUERY_SIZE - strlen(conditional_clause),
					" AND mailbox_id = %d",
					mailbox_id);
	}

	EM_DEBUG_LOG("conditional_clause [%s]", conditional_clause);
	if (!emstorage_query_mail_tbl(multi_user_name,
									conditional_clause,
									transaction,
									&p_data_tbl,
									&result_count,
									&error)) {
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
	if (account_id != ALL_ACCOUNT) {
		cur_clause += (conditional_clause_count++ == 0) ?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE account_id = %d", account_id) :
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND account_id = %d", account_id);
	}

	if (mailbox_id > 0) {
		cur_clause += (conditional_clause_count++ == 0) ?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE mailbox_id = %d", mailbox_id) :
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND mailbox_id = %d", mailbox_id);
	}

	if (thread_id > 0) {
		cur_clause += (conditional_clause_count++ == 0) ?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE thread_id = %d ", thread_id) :
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND thread_id = %d ", thread_id);
	} else if (thread_id == EMAIL_LIST_TYPE_THREAD)	{
		cur_clause += (conditional_clause_count++ == 0) ?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE thread_item_count > 0") :
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND thread_item_count > 0");
	} else if (thread_id == EMAIL_LIST_TYPE_LOCAL) {
		cur_clause += (conditional_clause_count++ == 0) ?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE server_mail_status = 0") :
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND server_mail_status = 0");
	} else if (thread_id == EMAIL_LIST_TYPE_UNREAD) {
		cur_clause += (conditional_clause_count++ == 0) ?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE flags_seen_field == 0") :
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND flags_seen_field == 0");
	}

	/*  EM_DEBUG_LOG("where clause added [%s]", conditional_clause_string); */
	if (addr_list && addr_list->address_count > 0) {
		if (!addr_list->address_type) {
			cur_clause += (conditional_clause_count++ == 0) ?
				SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE email_address_sender IN(\"%s\"", addr_list->address_list[0]) :
				SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND email_address_sender IN(\"%s\"", addr_list->address_list[0]);

			for (i = 1; i < addr_list->address_count; i++)
				cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, ",\"%s\"", addr_list->address_list[i]);

			cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, ")");
		} else {
			cur_clause += (conditional_clause_count++ == 0) ?
				SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE full_address_to IN(\"%s\") or full_address_cc IN(\"%s\"", addr_list->address_list[0], addr_list->address_list[0]) :
				SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " AND full_address_to IN(\"%s\") or full_address_cc IN(\"%s\"", addr_list->address_list[0], addr_list->address_list[0]);

			for (i = 1; i < addr_list->address_count; i++)
				cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, ",\"%s\"", addr_list->address_list[i]);

			cur_clause += SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, ")");
		}
	}

	if (input_except_delete_flagged_mails) {
		cur_clause += (conditional_clause_count++ == 0) ?
			SNPRINTF_OFFSET(conditional_clause_string, cur_clause, buffer_size - cur_clause, " WHERE flags_deleted_field = 0") :
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
			 " ORDER BY mailbox_id DESC, date_time DESC",	  /* case EMAIL_SORT_MAILBOX_ID_HIGH: */
			 " ORDER BY mailbox_id ASC, date_time DESC",		/* case EMAIL_SORT_MAILBOX_ID_LOW: */
			 " ORDER BY flags_flagged_field DESC, date_time DESC",  /* case EMAIL_SORT_FLAGGED_FLAG_HIGH: */
			 " ORDER BY flags_flagged_field ASC, date_time DESC",   /* case EMAIL_SORT_FLAGGED_FLAG_LOW: */
			 " ORDER BY flags_seen_field DESC, date_time DESC",     /* case EMAIL_SORT_SEEN_FLAG_HIGH: */
			 " ORDER BY flags_seen_field ASC, date_time DESC"       /* case EMAIL_SORT_SEEN_FLAG_LOW: */
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
INTERNAL_FUNC int emstorage_get_mail_list(char *multi_user_name, int account_id, int mailbox_id, email_email_address_list_t* addr_list, int thread_id, int start_index, int limit_count, int search_type, const char *search_value, email_sort_type_t sorting, int transaction, email_mail_list_item_t** mail_list,  int *result_count,  int *err_code)
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

	if (!emstorage_query_mail_list(multi_user_name, conditional_clause_string, transaction, mail_list, result_count, &error)) {
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
INTERNAL_FUNC int emstorage_get_mails(char *multi_user_name, int account_id, int mailbox_id, email_email_address_list_t* addr_list, int thread_id, int start_index, int limit_count, email_sort_type_t sorting,  int transaction, emstorage_mail_tbl_t** mail_list, int *result_count, int *err_code)
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

	if (!emstorage_query_mail_tbl(multi_user_name, conditional_clause_string, transaction, &p_data_tbl, &count,  &error)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_tbl failed [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (ret == true) {
		*mail_list = p_data_tbl;
		*result_count = count;
		EM_DEBUG_LOG("COUNT : %d", count);
	} else if (p_data_tbl != NULL)
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
INTERNAL_FUNC int emstorage_get_searched_mail_list(char *multi_user_name, int account_id, int mailbox_id, int thread_id, int search_type, const char *search_value, int start_index, int limit_count, email_sort_type_t sorting, int transaction, email_mail_list_item_t **mail_list,  int *result_count,  int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false, error = EMAIL_ERROR_NONE;
	char conditional_clause[QUERY_SIZE] = {0, };
	char *temp_search_value = NULL;
	char *temp_search_value2 = NULL;

	EM_IF_NULL_RETURN_VALUE(mail_list, false);
	EM_IF_NULL_RETURN_VALUE(result_count, false);

	if (!result_count || !mail_list || account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("select_num[%p], Mail_list[%p]", result_count, mail_list);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	temp_search_value = em_replace_all_string((char*)search_value, "_", "\\_");
	temp_search_value2 = em_replace_all_string(temp_search_value, "%", "\\%");

	_write_conditional_clause_for_getting_mail_list(account_id, mailbox_id, NULL, thread_id, start_index, limit_count, search_type, temp_search_value2, sorting, true, conditional_clause, QUERY_SIZE, &error);

	EM_DEBUG_LOG("conditional_clause[%s]", conditional_clause);

	if (!emstorage_query_mail_list(multi_user_name, conditional_clause, transaction, mail_list, result_count, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_list [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EM_DEBUG_LOG("emstorage_get_searched_mail_list finish off");

	if (err_code != NULL)
		*err_code = error;

	EM_SAFE_FREE(temp_search_value);
	EM_SAFE_FREE(temp_search_value2);

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


static int _get_password_file_name(char *multi_user_name, int account_id, char *recv_password_file_name, char *send_password_file_name)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d]", account_id);

	if (account_id <= 0 || !recv_password_file_name || !send_password_file_name) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return EMAIL_ERROR_INVALID_PARAM;
	}

    EM_DEBUG_LOG("MULTI_USER_NAME : [%s]", multi_user_name);

	if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		snprintf(recv_password_file_name, MAX_PW_FILE_NAME_LENGTH, ".email_account_%d_recv_%s", account_id, multi_user_name);
		snprintf(send_password_file_name, MAX_PW_FILE_NAME_LENGTH, ".email_account_%d_send_%s", account_id, multi_user_name);
	} else {
		snprintf(recv_password_file_name, MAX_PW_FILE_NAME_LENGTH, ".email_account_%d_recv", account_id);
		snprintf(send_password_file_name, MAX_PW_FILE_NAME_LENGTH, ".email_account_%d_send", account_id);
	}

	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

static int _read_password_from_secure_storage(char *file_name, char **password)
{
	EM_DEBUG_FUNC_BEGIN_SEC("file_name[%s]", file_name);

	if (!file_name || !password) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	int ret = EMAIL_ERROR_NONE;

	ret = emcore_get_password_in_key_manager(file_name, password);
	if (ret != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_password_in_key_manager failed : [%d]", ret);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_account_by_id(char *multi_user_name, int account_id, int pulloption, emstorage_account_tbl_t **account, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], pulloption[%d], account[%p], transaction[%d], err_code[%p]", account_id, pulloption, account, transaction, err_code);

	if (!account) {
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);

	/*  Make query string */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT ");
	sql_len = EM_SAFE_STRLEN(sql_query_string);

	if (pulloption & EMAIL_ACC_GET_OPT_DEFAULT) {
		SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len,
			"incoming_server_type,"
			"incoming_server_address,"
			"user_email_address,"
			"incoming_server_user_name,"
			"retrieval_mode,"
			"incoming_server_port_number,"
			"incoming_server_secure_connection,"
			"incoming_server_authentication_method,"
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
			"peak_interval,"
			"peak_days,"
			"peak_start_time,"
			"peak_end_time,"
			"outgoing_server_use_same_authenticator,"
			"auto_resend_times,"
			"outgoing_server_size_limit,"
			"wifi_auto_download,"
			"pop_before_smtp,"
			"incoming_server_requires_apop,"
			"logo_icon_path,"
			"user_data,"
			"user_data_length,"
			"color_label,"
			"check_interval,"
			"sync_status,");
		sql_len = EM_SAFE_STRLEN(sql_query_string);
	}

	if (pulloption & EMAIL_ACC_GET_OPT_ACCOUNT_NAME) {
		SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len, " account_name, ");
		sql_len = EM_SAFE_STRLEN(sql_query_string);
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
			"notification_status,"
			"vibrate_status,"
			"display_content_status,"
			"default_ringtone_status,"
			"alert_ringtone_path,"
			"account_svc_id,"
			"sync_disabled,"
			"default_mail_slot_size,"
			"roaming_option,"
			"smime_type,"
			"certificate_path,"
			"cipher_type,"
			"digest_type,");

		sql_len = EM_SAFE_STRLEN(sql_query_string);
	}

	/*  dummy value, FROM WHERE clause */
	SNPRINTF(sql_query_string + sql_len, sizeof(sql_query_string) - sql_len, "0 FROM mail_account_tbl WHERE account_id = %d", account_id);

	/*  FROM clause */
	EM_DEBUG_LOG_SEC("query = [%s]", sql_query_string);

	/*  execute a sql and count rows */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION("no matched account found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

	/*  Assign query result to structure */
	if (!(p_data_tbl = (emstorage_account_tbl_t *)malloc(sizeof(emstorage_account_tbl_t) * 1))) {
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
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->incoming_server_authentication_method), col_index++);
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
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->peak_interval), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->peak_days), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->peak_start_time), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->peak_end_time), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->outgoing_server_use_same_authenticator), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->auto_resend_times), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->outgoing_server_size_limit), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->wifi_auto_download), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->pop_before_smtp), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->incoming_server_requires_apop), col_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->logo_icon_path), 0, col_index++);
		_get_stmt_field_data_blob(hStmt, &p_data_tbl->user_data, col_index++);
		_get_stmt_field_data_int(hStmt, &p_data_tbl->user_data_length, col_index++);
		_get_stmt_field_data_int(hStmt, &p_data_tbl->color_label, col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->check_interval), col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->sync_status), col_index++);
	}

	if (pulloption & EMAIL_ACC_GET_OPT_ACCOUNT_NAME)
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->account_name), 0, col_index++);

	if (pulloption & EMAIL_ACC_GET_OPT_PASSWORD) {
		/*  get password file name */
		if ((error = _get_password_file_name(multi_user_name, p_data_tbl->account_id, recv_password_file_name, send_password_file_name)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("_get_password_file_name failed [%d]", error);
			goto FINISH_OFF;
		}

		/*  read password from secure storage */
		if ((error = _read_password_from_secure_storage(recv_password_file_name, &(p_data_tbl->incoming_server_password))) < 0) {
			EM_DEBUG_EXCEPTION(" _read_password_from_secure_storage failed [%d]", error);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG_DEV("recv_password_file_name[%s], password[%s]", recv_password_file_name,  p_data_tbl->incoming_server_password);

		if (p_data_tbl->outgoing_server_use_same_authenticator == 0) {
			if ((error = _read_password_from_secure_storage(send_password_file_name, &(p_data_tbl->outgoing_server_password))) < 0) {
				EM_DEBUG_EXCEPTION(" _read_password_from_secure_storage failed [%d]", error);
				goto FINISH_OFF;
			}
			EM_DEBUG_LOG_DEV("send_password_file_name[%s], password[%s]", send_password_file_name,  p_data_tbl->outgoing_server_password);
		}
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
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->options.notification_status), col_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->options.vibrate_status), col_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->options.display_content_status), col_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->options.default_ringtone_status), col_index++);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl->options.alert_ringtone_path), 0, col_index++);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_svc_id), col_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->sync_disabled), col_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->default_mail_slot_size), col_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->roaming_option), col_index++);
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
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_password_length_of_account(char *multi_user_name, int account_id, int password_type, int *password_length, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], password_length[%p], err_code[%p]", account_id, password_length, err_code);

	if (account_id <= 0 || password_length == NULL) {
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
	if ((error = _get_password_file_name(multi_user_name, account_id, recv_password_file_name, send_password_file_name)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_get_password_file_name failed.");
		goto FINISH_OFF;
	}

	/*  read password from secure storage */
	if (password_type == EMAIL_GET_INCOMING_PASSWORD_LENGTH) {
		if ((error = _read_password_from_secure_storage(recv_password_file_name, &temp_password)) < 0 || !temp_password) {
			EM_DEBUG_EXCEPTION(" _read_password_from_secure_storage()  failed...");
			goto FINISH_OFF;
		}
	} else if (password_type == EMAIL_GET_OUTGOING_PASSWORD_LENGTH) {
		if ((error = _read_password_from_secure_storage(send_password_file_name, &temp_password)) < 0 || !temp_password) {
			EM_DEBUG_EXCEPTION(" _read_password_from_secure_storage()  failed...");
			goto FINISH_OFF;
		}
	} else {
		EM_DEBUG_LOG("Invalid password type");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	*password_length = EM_SAFE_STRLEN(temp_password);

	EM_DEBUG_LOG_SEC("recv_password_file_name[%s], *password_length[%d]", recv_password_file_name,  *password_length);

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE(temp_password);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_update_account_password(char *multi_user_name, int input_account_id, char *input_incoming_server_password, char *input_outgoing_server_password)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id[%d], input_incoming_server_password[%p], input_outgoing_server_password[%p]", input_account_id, input_incoming_server_password, input_outgoing_server_password);

	int err = EMAIL_ERROR_NONE;
	char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];

	if (input_incoming_server_password == NULL && input_outgoing_server_password == NULL) {
		EM_DEBUG_EXCEPTION_SEC("Invalid param");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/*  get password file name */
	if ((err = _get_password_file_name(multi_user_name,
										input_account_id,
										recv_password_file_name,
										send_password_file_name)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_get_password_file_name failed.");
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_SEC("recv_password_file_name [%s] input_incoming_server_password [%s]",
						recv_password_file_name, input_incoming_server_password);
	EM_DEBUG_LOG_SEC("send_password_file_name [%s] input_outgoing_server_password [%s]",
						send_password_file_name, input_outgoing_server_password);

	if (input_incoming_server_password) {
		err = emcore_remove_password_in_key_manager(recv_password_file_name);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION_SEC("emcore_remove_password_in_key_manager: file[%s]", recv_password_file_name);
			goto FINISH_OFF;
		}

		/*  save recv passwords to the secure storage */
		err = emcore_add_password_in_key_manager(recv_password_file_name, input_incoming_server_password);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_add_password_in_key_manager failed : [%d]", err);
			goto FINISH_OFF;
		}
	}

	if (input_outgoing_server_password) {
		err = emcore_remove_password_in_key_manager(send_password_file_name);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION_SEC("emcore_remove_password_in_key_manager: file[%s]", send_password_file_name);
			goto FINISH_OFF;
		}

		/*  save send passwords to the secure storage */
		err = emcore_add_password_in_key_manager(send_password_file_name, input_outgoing_server_password);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_add_password_in_key_manager failed : [%d]", err);
			goto FINISH_OFF;
		}
	}
FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emstorage_update_account(char *multi_user_name, int account_id, emstorage_account_tbl_t *account_tbl, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], account[%p], transaction[%d], err_code[%p]", account_id, account_tbl, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID || !account_tbl) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int error = EMAIL_ERROR_NONE;
	int rc, ret = false;

	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

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
		", roaming_option = ?"
		", color_label = ?"
		", user_display_name = ?"
		", user_email_address = ?"
		", reply_to_address = ?"
		", return_address = ?"
		", incoming_server_type = ?"
		", incoming_server_address = ?"
		", incoming_server_port_number = ?"
		", incoming_server_user_name = ?"
		", incoming_server_secure_connection = ?"
		", incoming_server_authentication_method = ?"
		", retrieval_mode = ?"
		", keep_mails_on_pop_server_after_download = ?"
		", check_interval = ?"
		", auto_download_size = ?"
		", peak_interval = ?"
		", peak_days = ?"
		", peak_start_time = ?"
		", peak_end_time = ?"
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
		", notification_status = ?"
		", vibrate_status = ?"
		", display_content_status = ?"
		", default_ringtone_status = ?"
		", alert_ringtone_path = ?"
		", auto_resend_times = ?"
		", outgoing_server_size_limit = ?"
		", wifi_auto_download = ?"
		", pop_before_smtp = ?"
		", incoming_server_requires_apop = ?"
		", smime_type = ?"
		", certificate_path = ?"
		", cipher_type = ?"
		", digest_type = ?"
		", user_name = ?"
		" WHERE account_id = ?");

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("After sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_LOG_SEC("SQL[%s]", sql_query_string);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
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
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->roaming_option);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->color_label);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->user_display_name, 0, DISPLAY_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->user_email_address, 0, EMAIL_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->reply_to_address, 0, REPLY_TO_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->return_address, 0, RETURN_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_type);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->incoming_server_address, 0, RECEIVING_SERVER_ADDR_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_port_number);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->incoming_server_user_name, 0, USER_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_secure_connection);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_authentication_method);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->retrieval_mode);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->keep_mails_on_pop_server_after_download);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->check_interval);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->auto_download_size);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->peak_interval);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->peak_days);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->peak_start_time);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->peak_end_time);
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
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.notification_status);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.vibrate_status);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.display_content_status);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.default_ringtone_status);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->options.alert_ringtone_path, 0, CERTIFICATE_PATH_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->auto_resend_times);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->outgoing_server_size_limit);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->wifi_auto_download);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->pop_before_smtp);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_requires_apop);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->smime_type);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->certificate_path, 0, CERTIFICATE_PATH_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->cipher_type);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->digest_type);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->user_name, 0, DISPLAY_NAME_FROM_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_id);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	/*  validate account existence */
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION("no matched account found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (account_tbl->incoming_server_password || account_tbl->outgoing_server_password) {
		if ((error = emstorage_update_account_password(multi_user_name, account_id, account_tbl->incoming_server_password, account_tbl->outgoing_server_password)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_update_account_password failed [%d]", error);
			goto FINISH_OFF;
		}
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (error == EMAIL_ERROR_NONE) {
		if (!emcore_notify_storage_event(NOTI_ACCOUNT_UPDATE, account_tbl->account_id, 0, NULL, 0))
			EM_DEBUG_EXCEPTION(" emcore_notify_storage_event[ NOTI_ACCOUNT_UPDATE] : Notification Failed >>> ");
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_set_field_of_accounts_with_integer_value(char *multi_user_name, int account_id, char *field_name, int value, int transaction)
{
	EM_DEBUG_FUNC_BEGIN_SEC("account_id[%d], field_name[%s], value[%d], transaction[%d]", account_id, field_name, value, transaction);
	int error = EMAIL_ERROR_NONE;
	int result = 0;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = NULL;

	if (!account_id  || !field_name) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	/* Write query string */
	SNPRINTF(sql_query_string, QUERY_SIZE, "UPDATE mail_account_tbl SET %s = %d WHERE account_id = %d", field_name, value, account_id);

	EM_DEBUG_LOG_SEC("sql_query_string [%s]", sql_query_string);

	/* Execute query */
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
		goto FINISH_OFF;
	}

	if (sqlite3_changes(local_db_handle) == 0)
		EM_DEBUG_LOG("no mail matched...");


FINISH_OFF:
	result = (error == EMAIL_ERROR_NONE) ? true : false;
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, result, error);

	if (error == EMAIL_ERROR_NONE) {
		if (!emcore_notify_storage_event(NOTI_ACCOUNT_UPDATE, account_id, 0, field_name, value))
			EM_DEBUG_EXCEPTION_SEC("emcore_notify_storage_eventfailed : NOTI_ACCOUNT_UPDATE [%s,%d]", field_name, value);
	}
	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int emstorage_get_sync_status_of_account(char *multi_user_name, int account_id, int *result_sync_status, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], result_sync_status [%p], err_code[%p]", account_id, result_sync_status, err_code);

	if (!result_sync_status) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int error = EMAIL_ERROR_NONE, rc, ret = false, sync_status, count, i, col_index;
	char sql_query_string[QUERY_SIZE] = {0, };
	char **result = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	if (account_id)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT sync_status FROM mail_account_tbl WHERE account_id = %d", account_id);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT sync_status FROM mail_account_tbl");

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (!count) {
		EM_DEBUG_EXCEPTION("no matched account found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

	col_index = 1;
	*result_sync_status = 0;

	for (i = 0; i < count; i++) {
		_get_table_field_data_int(result, &sync_status, col_index++);
		*result_sync_status |= sync_status;
	}

	EM_DEBUG_LOG("sync_status [%d]", sync_status);

	sqlite3_free_table(result);

	ret = true;

FINISH_OFF:


	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_update_sync_status_of_account(char *multi_user_name, int account_id, email_set_type_t set_operator, int sync_status, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], set_operator[%d], sync_status [%d], transaction[%d], err_code[%p]", account_id, set_operator, sync_status, transaction, err_code);

	int error = EMAIL_ERROR_NONE, rc, ret = false, set_value = sync_status, result_sync_status;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	if (set_operator != SET_TYPE_SET && account_id) {
		if (!emstorage_get_sync_status_of_account(multi_user_name, account_id, &result_sync_status, &error)) {
			EM_DEBUG_EXCEPTION("emstorage_get_sync_status_of_account failed [%d]", error);
			if (err_code != NULL)
				*err_code = error;
			return false;
		}
		switch (set_operator) {
			case SET_TYPE_UNION:
				set_value = result_sync_status | set_value;
				break;
			case SET_TYPE_MINUS:
				set_value = result_sync_status & (~set_value);
				break;
			default:
				EM_DEBUG_EXCEPTION("EMAIL_ERROR_NOT_SUPPORTED [%d]", set_operator);
				error = EMAIL_ERROR_NOT_SUPPORTED;
				break;
		}
		EM_DEBUG_LOG("set_value [%d]", set_value);
	}

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	if (account_id)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_account_tbl SET sync_status = %d WHERE account_id = %d", set_value, account_id);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_account_tbl SET sync_status = %d WHERE incoming_server_type <> 5", set_value);

	EM_DEBUG_LOG_SEC("sql_query_string [%s]", sql_query_string);

	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
		goto FINISH_OFF;
	}

	rc = sqlite3_changes(local_db_handle);

	if (rc == 0) {
		EM_DEBUG_EXCEPTION("no matched account found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (error == EMAIL_ERROR_NONE) {
		if (!emcore_notify_storage_event(NOTI_ACCOUNT_UPDATE_SYNC_STATUS, account_id, set_value, NULL, 0))
			EM_DEBUG_EXCEPTION("emcore_notify_storage_event[NOTI_ACCOUNT_UPDATE_SYNC_STATUS] : Notification failed");
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_add_account(char *multi_user_name, emstorage_account_tbl_t *account_tbl, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account[%p], transaction[%d], err_code[%p]", account_tbl, transaction, err_code);

	if (!account_tbl) {
		EM_DEBUG_EXCEPTION("account[%p], transaction[%d], err_code[%p]", account_tbl, transaction, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int row_count = 0;
	int i = 0;
	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	char *sql = "SELECT rowid FROM mail_account_tbl;";
	char **result = NULL;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, &row_count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL == result[1]) rc = 1;
	else {
		for (i = 1; i <= row_count; i++) {
			if (i != atoi(result[i])) {
				break;
			}
		}

		rc = i;
	}
	sqlite3_free_table(result);
	result = NULL;

	if (rc < 0 && rc > EMAIL_ACCOUNT_MAX) {
		EM_DEBUG_EXCEPTION("OVERFLOWED THE MAX ACCOUNT");
		error = EMAIL_ERROR_ACCOUNT_MAX_COUNT;
		goto FINISH_OFF;
	}

	account_tbl->account_id = rc;

	if ((error = _get_password_file_name(multi_user_name, account_tbl->account_id, recv_password_file_name, send_password_file_name)) != EMAIL_ERROR_NONE) {
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
		"  , ? "  /*   roaming_option */
		"  , ? "  /*   color_label */
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
		"  , ? "  /*   incoming_server_authentication_method */
		"  , ? "  /*   retrieval_mode */
		"  , ? "  /*   keep_mails_on_pop_server_after_download */
		"  , ? "  /*   check_interval */
		"  , ? "  /*   auto_download_size */
		"  , ? "  /*   peak_interval */
		"  , ? "  /*   peak_days */
		"  , ? "  /*   peak_start_time */
		"  , ? "  /*   peak_end_time */
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
		"  , ? "  /*   auto_resend_times */
		"  , ? "  /*   outgoing_server_size_limit */
		"  , ? "  /*   wifi_auto_download */
		"  , ? "  /*   pop_before_smtp */
		"  , ? "  /*   incoming_server_requires_apop */
		"  , ? "  /*   smime_type */
		"  , ? "  /*   certificate_path */
		"  , ? "  /*   cipher_type */
		"  , ? "  /*   digest_type */
		"  , ? "  /*   notification_status */
		"  , ? "  /*   vibrate_status */
		"  , ? "  /*   display_content_status */
		"  , ? "  /*   default_ringtone_status */
		"  , ? "  /*   alert_ringtone_path */
		"  , ? "  /*   user_name */
		") ");

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	i = 0;

	_bind_stmt_field_data_int(hStmt, i++, account_tbl->account_id);
	_bind_stmt_field_data_string(hStmt, i++, (char *)account_tbl->account_name, 0, ACCOUNT_NAME_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->logo_icon_path, 0, LOGO_ICON_PATH_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_blob(hStmt, i++, account_tbl->user_data, account_tbl->user_data_length);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->user_data_length);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->account_svc_id);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->sync_status);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->sync_disabled);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->default_mail_slot_size);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->roaming_option);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->color_label);
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
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_authentication_method);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->retrieval_mode);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->keep_mails_on_pop_server_after_download);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->check_interval);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->auto_download_size);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->peak_interval);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->peak_days);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->peak_start_time);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->peak_end_time);
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
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->auto_resend_times);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->outgoing_server_size_limit);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->wifi_auto_download);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->pop_before_smtp);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->incoming_server_requires_apop);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->smime_type);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->certificate_path, 0, FILE_NAME_LEN_IN_MAIL_CERTIFICATE_TBL);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->cipher_type);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->digest_type);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.notification_status);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.vibrate_status);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.display_content_status);
	_bind_stmt_field_data_int(hStmt, i++, account_tbl->options.default_ringtone_status);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->options.alert_ringtone_path, 0, CERTIFICATE_PATH_LEN_IN_MAIL_ACCOUNT_TBL);
	_bind_stmt_field_data_string(hStmt, i++, account_tbl->user_name, 0, CERTIFICATE_PATH_LEN_IN_MAIL_ACCOUNT_TBL);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);

	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d, errmsg = %s.", rc, sqlite3_errmsg(local_db_handle)));


	/*  save passwords to the secure storage */
	EM_DEBUG_LOG_SEC("save to the secure storage : recv_file[%s], send_file[%s]", recv_password_file_name, send_password_file_name);
	error = emcore_add_password_in_key_manager(recv_password_file_name, account_tbl->incoming_server_password);
	if (error != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_add_password_in_key_manager failed : [%d]", error);
		goto FINISH_OFF;
	}

	error = emcore_add_password_in_key_manager(send_password_file_name, account_tbl->outgoing_server_password);
	if (error != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_add_password_in_key_manager failed : [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (error == EMAIL_ERROR_NONE) {
		if (!emcore_notify_storage_event(NOTI_ACCOUNT_ADD, account_tbl->account_id, 0, NULL, 0))
			EM_DEBUG_EXCEPTION("emcore_notify_storage_event[NOTI_ACCOUNT_ADD] : Notification failed");
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_account(char *multi_user_name, int account_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], transaction[%d], err_code[%p]", account_id, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID) {
		EM_DEBUG_EXCEPTION(" account_id[%d]", account_id);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	/*  TODO : delete password files - file names can be obtained from db or a rule that makes a name */
	char sql_query_string[QUERY_SIZE] = {0, };
	char recv_password_file_name[MAX_PW_FILE_NAME_LENGTH];
	char send_password_file_name[MAX_PW_FILE_NAME_LENGTH];

	/*  get password file name */
	if ((error = _get_password_file_name(multi_user_name, account_id, recv_password_file_name, send_password_file_name)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_get_password_file_name failed.");
		goto FINISH_OFF;
	}

	/*  delete from db */
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_account_tbl WHERE account_id = %d", account_id);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	/*  validate account existence */
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION(" no matched account found...");
		error = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

		/*  delete from secure storage */
	error = emcore_remove_password_in_key_manager(recv_password_file_name);
	if (error != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_remove_password_in_key_manager failed : [%d]", error);
		goto FINISH_OFF;
	}

	error = emcore_remove_password_in_key_manager(send_password_file_name);
	if (error != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_remove_password_in_key_manager failed : [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

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

	if (count > 0) {
		if (!account_list || !*account_list) {
			EM_DEBUG_EXCEPTION("account_list[%p], count[%d]", account_list, count);
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		emstorage_account_tbl_t* p = *account_list;
		int i = 0;

		for (; i < count; i++) {
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
			EM_SAFE_FREE(p[i].options.alert_ringtone_path);
			EM_SAFE_FREE(p[i].certificate_path);
			EM_SAFE_FREE(p[i].user_name);
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

INTERNAL_FUNC int emstorage_get_mailbox_count(char *multi_user_name, int account_id, int local_yn, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], count[%p], transaction[%d], err_code[%p]", account_id, local_yn, count, transaction, err_code);

	if ((account_id < FIRST_ACCOUNT_ID) || (count == NULL)) {
		EM_DEBUG_EXCEPTION(" account_list[%d], local_yn[%d], count[%p]", account_id, local_yn, count);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_box_tbl WHERE account_id = %d AND local_yn = %d", account_id, local_yn);

	char **result;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);


	ret = true;

FINISH_OFF:

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mailbox_list(char *multi_user_name, int account_id, int local_yn, email_mailbox_sort_type_t sort_type, int *select_num, emstorage_mailbox_tbl_t** mailbox_list, int transaction, int *err_code)
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
			SNPRINTF(conditional_clause_string + EM_SAFE_STRLEN(conditional_clause_string), sizeof(conditional_clause_string)-(EM_SAFE_STRLEN(conditional_clause_string)+1), " WHERE local_yn = %d ", local_yn);
	} else {
		SNPRINTF(conditional_clause_string, sizeof(conditional_clause_string), "WHERE account_id = %d  ", account_id);
		if (local_yn == EMAIL_MAILBOX_FROM_SERVER || local_yn == EMAIL_MAILBOX_FROM_LOCAL)
			SNPRINTF(conditional_clause_string + EM_SAFE_STRLEN(conditional_clause_string), sizeof(conditional_clause_string)-(EM_SAFE_STRLEN(conditional_clause_string)+1), " AND local_yn = %d ", local_yn);
	}

	EM_DEBUG_LOG("conditional_clause_string[%s]", conditional_clause_string);

	switch (sort_type) {
		case EMAIL_MAILBOX_SORT_BY_NAME_ASC:
			SNPRINTF(ordering_clause_string, QUERY_SIZE, " ORDER BY mailbox_name ASC");
			break;

		case EMAIL_MAILBOX_SORT_BY_NAME_DSC:
			SNPRINTF(ordering_clause_string, QUERY_SIZE, " ORDER BY mailbox_name DESC");
			break;

		case EMAIL_MAILBOX_SORT_BY_TYPE_ASC:
			SNPRINTF(ordering_clause_string, QUERY_SIZE, " ORDER BY mailbox_type ASC");
			break;

		case EMAIL_MAILBOX_SORT_BY_TYPE_DSC:
			SNPRINTF(ordering_clause_string, QUERY_SIZE, " ORDER BY mailbox_type DEC");
			break;
	}

	EM_DEBUG_LOG("ordering_clause_string[%s]", ordering_clause_string);

	if ((error = emstorage_query_mailbox_tbl(multi_user_name, conditional_clause_string, ordering_clause_string, 0, transaction, mailbox_list, select_num)) != EMAIL_ERROR_NONE) {
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

INTERNAL_FUNC int emstorage_get_mailbox_list_ex(char *multi_user_name, int account_id, int local_yn, int with_count, int *select_num, emstorage_mailbox_tbl_t **mailbox_list, int transaction, int *err_code)
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
		SNPRINTF(conditional_clause_string + EM_SAFE_STRLEN(conditional_clause_string), sizeof(conditional_clause_string)-(EM_SAFE_STRLEN(conditional_clause_string)+1), "WHERE local_yn = %d ",  local_yn);
		where_clause_count++;
	}

	if (account_id > 0) {
		if (where_clause_count == 0) {
			SNPRINTF(conditional_clause_string + EM_SAFE_STRLEN(conditional_clause_string), sizeof(conditional_clause_string)-(EM_SAFE_STRLEN(conditional_clause_string)+1), " WHERE ");
			SNPRINTF(conditional_clause_string + EM_SAFE_STRLEN(conditional_clause_string), sizeof(conditional_clause_string)-(EM_SAFE_STRLEN(conditional_clause_string)+1), " account_id = %d ", account_id);
		} else
			SNPRINTF(conditional_clause_string + EM_SAFE_STRLEN(conditional_clause_string), sizeof(conditional_clause_string)-(EM_SAFE_STRLEN(conditional_clause_string)+1), " AND account_id = %d ", account_id);
	}

	SNPRINTF(ordering_clause_string, QUERY_SIZE, " ORDER BY CASE WHEN MBT.mailbox_name"
												 " GLOB \'[][~`!@#$%%^&*()_-+=|\\{}:;<>,.?/ ]*\'"
												 " THEN 2 ELSE 1 END ASC,"
												 " MBT.mailbox_name COLLATE CONVERTUTF8 ASC ");
	EM_DEBUG_LOG("conditional_clause_string[%s]", conditional_clause_string);
	EM_DEBUG_LOG("ordering_clause_string[%s]", ordering_clause_string);

	if ((error = emstorage_query_mailbox_tbl(multi_user_name, conditional_clause_string, ordering_clause_string, 1, 1, mailbox_list, select_num)) != EMAIL_ERROR_NONE) {
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

INTERNAL_FUNC int emstorage_get_child_mailbox_list(char *multi_user_name, int account_id, char *parent_mailbox_name, int *select_num, emstorage_mailbox_tbl_t **mailbox_list, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], parent_mailbox_name[%p], select_num[%p], mailbox_list[%p], transaction[%d], err_code[%p]", account_id, parent_mailbox_name, select_num, mailbox_list, transaction, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char conditional_clause_string[QUERY_SIZE] = {0, };

	if (account_id < FIRST_ACCOUNT_ID || !select_num || !mailbox_list || !parent_mailbox_name) {
		EM_DEBUG_EXCEPTION("account_id[%d], parent_mailbox_name[%p], select_num[%p], mailbox_list[%p]", account_id, parent_mailbox_name, select_num, mailbox_list);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3_snprintf(sizeof(conditional_clause_string), conditional_clause_string, "WHERE account_id = %d  AND UPPER(mailbox_name) LIKE UPPER('%q%%')", account_id, parent_mailbox_name);
	EM_DEBUG_LOG("conditional_clause_string[%s]", conditional_clause_string);

	if ((error = emstorage_query_mailbox_tbl(multi_user_name, conditional_clause_string, " ORDER BY mailbox_name DESC ", 0, transaction, mailbox_list, select_num)) != EMAIL_ERROR_NONE) {
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

INTERNAL_FUNC int emstorage_get_mailbox_by_modifiable_yn(char *multi_user_name, int account_id, int local_yn, int *select_num, emstorage_mailbox_tbl_t** mailbox_list, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], select_num[%p], mailbox_list[%p], transaction[%d], err_code[%p]", account_id, local_yn, select_num, mailbox_list, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID || !select_num || !mailbox_list) {
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

	if ((error = emstorage_query_mailbox_tbl(multi_user_name, conditional_clause_string, " ORDER BY mailbox_name", 0, transaction, mailbox_list, select_num)) != EMAIL_ERROR_NONE) {
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

INTERNAL_FUNC int emstorage_stamp_last_sync_time_of_mailbox(char *multi_user_name, int input_mailbox_id, int input_transaction)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id [%d], input_transaction [%d]", input_mailbox_id, input_transaction);

	int      result_code = false;
	int      error = EMAIL_ERROR_NONE;
	time_t   current_time = 0;
	char     sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = NULL;

	if (!input_mailbox_id) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	time(&current_time);

	local_db_handle = emstorage_get_db_connection(multi_user_name);
    EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, input_transaction, error);

    SNPRINTF(sql_query_string, sizeof(sql_query_string),
        "UPDATE mail_box_tbl SET"
		" last_sync_time = %d"
		" WHERE mailbox_id = %d"
		, (int)current_time
		, input_mailbox_id);

	EM_DEBUG_LOG_SEC("sql_query_string [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

FINISH_OFF:

	if (error == EMAIL_ERROR_NONE)
		result_code = true;

	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, input_transaction, result_code, error);

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int emstorage_get_mailbox_by_name(char *multi_user_name, int account_id, int local_yn, char *mailbox_name, emstorage_mailbox_tbl_t **result_mailbox, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("account_id[%d], local_yn[%d], mailbox_name[%s], result_mailbox[%p], transaction[%d], err_code[%p]", account_id, local_yn, mailbox_name, result_mailbox, transaction, err_code);
	EM_PROFILE_BEGIN(profile_emstorage_get_mailbox_by_name);

	if (account_id < FIRST_ACCOUNT_ID || !mailbox_name || !result_mailbox) {
		EM_DEBUG_EXCEPTION_SEC(" account_id[%d], local_yn[%d], mailbox_name[%s], result_mailbox[%p]", account_id, local_yn, mailbox_name, result_mailbox);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int result_count = 0;
	char conditional_clause_string[QUERY_SIZE] = {0, };

	if (strcmp(mailbox_name, EMAIL_SEARCH_RESULT_MAILBOX_NAME) == 0) {
		if (!(*result_mailbox = (emstorage_mailbox_tbl_t*)em_malloc(sizeof(emstorage_mailbox_tbl_t)))) {
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
		(*result_mailbox)->deleted_flag               = 0;
		(*result_mailbox)->modifiable_yn              = 1;
		(*result_mailbox)->total_mail_count_on_server = 1;
		(*result_mailbox)->has_archived_mails         = 0;
		(*result_mailbox)->mail_slot_size             = 0x0FFFFFFF;
		(*result_mailbox)->no_select                  = 0;
	} else {
		if (local_yn == -1)
			sqlite3_snprintf(sizeof(conditional_clause_string), conditional_clause_string, "WHERE account_id = %d AND mailbox_name = '%q'", account_id, mailbox_name);
		else
			sqlite3_snprintf(sizeof(conditional_clause_string), conditional_clause_string, "WHERE account_id = %d AND local_yn = %d AND mailbox_name = '%q'", account_id, local_yn, mailbox_name);

		EM_DEBUG_LOG("conditional_clause_string = [%s]", conditional_clause_string);

		if ((error = emstorage_query_mailbox_tbl(multi_user_name, conditional_clause_string, "", 0, transaction, result_mailbox, &result_count)) != EMAIL_ERROR_NONE) {
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

INTERNAL_FUNC int emstorage_get_mailbox_by_mailbox_type(char *multi_user_name, int account_id, email_mailbox_type_e mailbox_type, emstorage_mailbox_tbl_t **output_mailbox, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_type[%d], output_mailbox[%p], transaction[%d], err_code[%p]", account_id, mailbox_type, output_mailbox, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID || (mailbox_type < EMAIL_MAILBOX_TYPE_INBOX || mailbox_type > EMAIL_MAILBOX_TYPE_USER_DEFINED) || !output_mailbox) {
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_type[%d], output_mailbox[%p]", account_id, mailbox_type, output_mailbox);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int result_count = 0;
	emstorage_mailbox_tbl_t *result_mailbox = NULL;
	char conditional_clause_string[QUERY_SIZE] = {0,};


	SNPRINTF(conditional_clause_string, QUERY_SIZE, "WHERE account_id = %d AND mailbox_type = %d ", account_id, mailbox_type);

	EM_DEBUG_LOG("conditional_clause_string = [%s]", conditional_clause_string);

	if ((error = emstorage_query_mailbox_tbl(multi_user_name, conditional_clause_string, "", true, false, &result_mailbox, &result_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_mailbox_tbl error [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (ret == true)
		*output_mailbox = result_mailbox;

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mailbox_by_id(char *multi_user_name, int input_mailbox_id, emstorage_mailbox_tbl_t** output_mailbox)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d], output_mailbox[%p]", input_mailbox_id, output_mailbox);

	if (input_mailbox_id <= 0 || !output_mailbox) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM: input_mailbox_id[%d] output_mailbox[%p]", input_mailbox_id, output_mailbox);
		return  EMAIL_ERROR_INVALID_PARAM;
	}

	int  ret = EMAIL_ERROR_NONE;
	int  result_count = 0;
	char conditional_clause_string[QUERY_SIZE] = {0, };

	SNPRINTF(conditional_clause_string, sizeof(conditional_clause_string), "WHERE MBT.mailbox_id = %d", input_mailbox_id);

	EM_DEBUG_LOG("conditional_clause_string = [%s]", conditional_clause_string);

	if ((ret = emstorage_query_mailbox_tbl(multi_user_name,
											conditional_clause_string,
											"", true, false, output_mailbox, &result_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_mailbox_tbl failed [%d]", ret);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mailbox_by_keyword(char *multi_user_name, int account_id, char *keyword, emstorage_mailbox_tbl_t** result_mailbox, int * result_count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("account_id[%d], keyword[%s], result_mailbox[%p], transaction[%d], err_code[%p]", account_id, keyword, result_mailbox, transaction, err_code);

	if (account_id < 0 || !keyword || !result_mailbox) {
		EM_DEBUG_EXCEPTION_SEC(" account_id[%d], keyword[%s], result_mailbox[%p]", account_id, keyword, result_mailbox);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char conditional_clause_string[QUERY_SIZE] = {0, };

	if (account_id == 0)
		sqlite3_snprintf(sizeof(conditional_clause_string), conditional_clause_string,
				"WHERE alias LIKE \'%%%q%%\'", keyword);
	else if (account_id > 0)
		sqlite3_snprintf(sizeof(conditional_clause_string), conditional_clause_string,
				"WHERE account_id = %d AND alias LIKE \'%%%q%%\'", account_id, keyword);

	EM_DEBUG_LOG("conditional_clause_string = [%s]", conditional_clause_string);

	if ((error = emstorage_query_mailbox_tbl(multi_user_name, conditional_clause_string, "", 0, transaction, result_mailbox, result_count)) != EMAIL_ERROR_NONE) {
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

INTERNAL_FUNC int emstorage_get_mailbox_id_by_mailbox_type(char *multi_user_name, int account_id, email_mailbox_type_e mailbox_type, int *mailbox_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_type[%d], mailbox_id[%p], transaction[%d], err_code[%p]", account_id, mailbox_type, mailbox_id, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID || (mailbox_type < EMAIL_MAILBOX_TYPE_INBOX || mailbox_type > EMAIL_MAILBOX_TYPE_ALL_EMAILS) || !mailbox_id) {
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	/*  Check whether the account exists. */
	if (!emstorage_get_account_by_id(multi_user_name, account_id, EMAIL_ACC_GET_OPT_ACCOUNT_NAME,  &account, true, &error) || !account) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed - %d", error);
		goto FINISH_OFF;
	}

	if (account)
		emstorage_free_account(&account, 1, NULL);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mailbox_id  FROM mail_box_tbl WHERE account_id = %d AND mailbox_type = %d ", account_id, mailbox_type);

	EM_DEBUG_LOG_SEC("query = [%s]", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION("no matched mailbox_name found...");
		error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
		goto FINISH_OFF;
	}

	_get_stmt_field_data_int(hStmt, mailbox_id, 0);

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mailbox_name_by_mailbox_type(char *multi_user_name, int account_id, email_mailbox_type_e mailbox_type, char **mailbox_name, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_type[%d], mailbox_name[%p], transaction[%d], err_code[%p]", account_id, mailbox_type, mailbox_name, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID || (mailbox_type < EMAIL_MAILBOX_TYPE_INBOX || mailbox_type > EMAIL_MAILBOX_TYPE_ALL_EMAILS) || !mailbox_name) {
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	/*  Check whether the account exists. */
	if (!emstorage_get_account_by_id(multi_user_name, account_id, EMAIL_ACC_GET_OPT_ACCOUNT_NAME,  &account, true, &error) || !account) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed - %d", error);
		goto FINISH_OFF;
	}

	if (account)
		emstorage_free_account(&account, 1, NULL);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mailbox_name  FROM mail_box_tbl WHERE account_id = %d AND mailbox_type = %d ", account_id, mailbox_type);

	EM_DEBUG_LOG_SEC("query = [%s]", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION("no matched mailbox_name found...");
		error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
		goto FINISH_OFF;
	}

	_get_stmt_field_data_string(hStmt, mailbox_name, 0, 0);

	ret = true;

FINISH_OFF:
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_update_mailbox_modifiable_yn(char *multi_user_name, int account_id, int local_yn, char *mailbox_name, int modifiable_yn, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], local_yn [%d], mailbox_name [%p], modifiable_yn [%d], transaction [%d], err_code [%p]", account_id, local_yn, mailbox_name, modifiable_yn, transaction, err_code);
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *replaced_mailbox_name = NULL;

	if (mailbox_name) {
		if (strstr(mailbox_name, "'")) {
			replaced_mailbox_name = em_replace_all_string(mailbox_name, "'", "''");
		} else {
			replaced_mailbox_name = strdup(mailbox_name);
		}
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_box_tbl SET"
		" modifiable_yn = %d"
		" WHERE account_id = %d"
		" AND local_yn = %d"
		" AND mailbox_name = '%s'"
		, modifiable_yn
		, account_id
		, local_yn
		, replaced_mailbox_name);

	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

  	EM_SAFE_FREE(replaced_mailbox_name);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}

INTERNAL_FUNC int emstorage_update_mailbox_total_count(char *multi_user_name,
														int account_id,
														int input_mailbox_id,
														int total_count_on_server,
														int transaction,
														int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], input_mailbox_id[%d], total_count_on_server[%d], "
						"transaction[%d], err_code[%p]",
						account_id, input_mailbox_id, total_count_on_server,  transaction, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	if (account_id <= 0 || input_mailbox_id <= 0) {
		EM_DEBUG_EXCEPTION("account_id[%d], input_mailbox_id[%d]", account_id, input_mailbox_id);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_FUNC_END("ret [%d]", ret);
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_box_tbl SET"
		" total_mail_count_on_server = %d"
		" WHERE account_id = %d"
		" AND mailbox_id = %d"
		, total_count_on_server
		, account_id
		, input_mailbox_id);

	EM_DEBUG_LOG_SEC("query[%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}

INTERNAL_FUNC int emstorage_update_mailbox(char *multi_user_name, int account_id, int local_yn, int input_mailbox_id, emstorage_mailbox_tbl_t *result_mailbox, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], input_mailbox_id[%d], result_mailbox[%p], transaction[%d], err_code[%p]", account_id, local_yn, input_mailbox_id, result_mailbox, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID || input_mailbox_id <= 0 || !result_mailbox) {
		EM_DEBUG_EXCEPTION(" account_id[%d], local_yn[%d], input_mailbox_id[%d], result_mailbox[%p]", account_id, local_yn, input_mailbox_id, result_mailbox);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_FUNC_END("ret [%d]", EMAIL_ERROR_INVALID_PARAM);
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	DB_STMT hStmt = NULL;
	int i = 0;

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	if (local_yn != -1) {
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"UPDATE mail_box_tbl SET"
			"  mailbox_id = ?"
			", mailbox_name = ?"
			", mailbox_type = ?"
			", alias = ?"
			", deleted_flag = ?"
			", modifiable_yn= ?"
			", mail_slot_size= ?"
			", total_mail_count_on_server = ?"
			" WHERE account_id = %d"
			" AND local_yn = %d"
			" AND mailbox_id = '%d'"
			, account_id
			, local_yn
			, input_mailbox_id);
	} else {
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"UPDATE mail_box_tbl SET"
			"  mailbox_id = ?"
			", mailbox_name = ?"
			", mailbox_type = ?"
			", alias = ?"
			", deleted_flag = ?"
			", modifiable_yn= ?"
			", mail_slot_size= ?"
			", total_mail_count_on_server = ?"
			" WHERE account_id = %d"
			" AND mailbox_id = '%d'"
			, account_id
			, input_mailbox_id);
	}



	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bind_stmt_field_data_int(hStmt, i++, result_mailbox->mailbox_id);
	_bind_stmt_field_data_string(hStmt, i++, (char *)result_mailbox->mailbox_name ? result_mailbox->mailbox_name : "", 0, MAILBOX_NAME_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bind_stmt_field_data_int(hStmt, i++, result_mailbox->mailbox_type);
	_bind_stmt_field_data_string(hStmt, i++, (char *)result_mailbox->alias ? result_mailbox->alias : "", 0, ALIAS_LEN_IN_MAIL_BOX_TBL);
	_bind_stmt_field_data_int(hStmt, i++, result_mailbox->deleted_flag);
	_bind_stmt_field_data_int(hStmt, i++, result_mailbox->modifiable_yn);
	_bind_stmt_field_data_int(hStmt, i++, result_mailbox->mail_slot_size);
	_bind_stmt_field_data_int(hStmt, i++, result_mailbox->total_mail_count_on_server);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_update_mailbox_type(char *multi_user_name, int account_id, int local_yn, int input_mailbox_id, email_mailbox_type_e new_mailbox_type, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("account_id[%d], local_yn[%d], input_mailbox_id[%d], new_mailbox_type[%d], transaction[%d], err_code[%p]", account_id, local_yn, input_mailbox_id, new_mailbox_type, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID) {
		EM_DEBUG_EXCEPTION_SEC(" account_id[%d], local_yn[%d], input_mailbox_id[%d]", account_id, local_yn, input_mailbox_id);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	EM_DEBUG_LOG("emstorage_update_mailbox_type");

	DB_STMT hStmt_box_tbl = NULL;
	DB_STMT hStmt_mail_tbl = NULL;
	int i = 0;

	/*  Update mail_box_tbl */
	if (local_yn != -1) {
		SNPRINTF(sql_query_string, sizeof(sql_query_string)-1,
			"UPDATE mail_box_tbl SET"
			" mailbox_type = ?"
			" WHERE account_id = %d"
			" AND local_yn = %d"
			" AND mailbox_id = '%d'"
			, account_id
			, local_yn
			, input_mailbox_id);
	} else {
		SNPRINTF(sql_query_string, sizeof(sql_query_string)-1,
			"UPDATE mail_box_tbl SET"
			" mailbox_type = ?"
			" WHERE account_id = %d"
			" AND mailbox_id = '%d'"
			, account_id
			, input_mailbox_id);
	}

	EM_DEBUG_LOG_SEC("SQL(%s)", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt_box_tbl, NULL), rc);

	if (SQLITE_OK != rc) {
		EM_DEBUG_EXCEPTION("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle));
		error = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	_bind_stmt_field_data_int(hStmt_box_tbl, i++, new_mailbox_type);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt_box_tbl), rc);

	if (rc == SQLITE_FULL) {
		EM_DEBUG_EXCEPTION("sqlite3_step fail:%d", rc);
		error	= EMAIL_ERROR_MAIL_MEMORY_FULL;
		goto FINISH_OFF;
	}

	if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
		EM_DEBUG_EXCEPTION("sqlite3_step fail:%d", rc);
		error = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}


	/*  Update mail_tbl */
	i = 0;
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"UPDATE mail_tbl SET"
			" mailbox_type = ?"
			" WHERE account_id = %d"
			" AND mailbox_id = '%d'"
			, account_id
			, input_mailbox_id);

	EM_DEBUG_LOG_SEC("SQL[%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt_mail_tbl, NULL), rc);
	if (SQLITE_OK != rc) {
		EM_DEBUG_EXCEPTION("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle));
		error = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	_bind_stmt_field_data_int(hStmt_mail_tbl, i++, new_mailbox_type);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt_mail_tbl), rc);
	if (rc == SQLITE_FULL) {
		EM_DEBUG_EXCEPTION("sqlite3_step fail:%d", rc);
		error = EMAIL_ERROR_MAIL_MEMORY_FULL;
		goto FINISH_OFF;
	}

	if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
		EM_DEBUG_EXCEPTION("sqlite3_step fail:%d", rc);
		error = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (hStmt_box_tbl != NULL) {
		rc = sqlite3_finalize(hStmt_box_tbl);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (hStmt_mail_tbl != NULL) {
		rc = sqlite3_finalize(hStmt_mail_tbl);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_set_local_mailbox(char *multi_user_name, int input_mailbox_id, int input_is_local_mailbox, int transaction)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d], new_mailbox_type[%d], transaction[%d], err_code[%p]", input_mailbox_id, input_is_local_mailbox, transaction);

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	if (input_mailbox_id < 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	EM_DEBUG_LOG("emstorage_update_mailbox_type");

	DB_STMT hStmt = NULL;
	int i = 0;

	/*  Update mail_box_tbl */
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_box_tbl SET"
		" local_yn = ?"
		" WHERE mailbox_id = %d"
		, input_mailbox_id);

	EM_DEBUG_LOG_SEC("SQL(%s)", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bind_stmt_field_data_int(hStmt, i++, input_is_local_mailbox);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
/*
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
		hStmt = NULL;
	}
*/
	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int emstorage_set_field_of_mailbox_with_integer_value(char *multi_user_name, int input_account_id, int *input_mailbox_id_array, int input_mailbox_id_count, char *input_field_name, int input_value, int transaction)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d] input_mailbox_id_array[%p] input_mailbox_id_count[%d] input_field_name[%p] input_value[%d] err_code[%p]", input_account_id, input_mailbox_id_array, input_mailbox_id_count, input_field_name, input_value, transaction);
	int i = 0;
	int err = EMAIL_ERROR_NONE;
	int result = false;
	int cur_mailbox_id_string = 0;
	int mailbox_id_string_buffer_length = 0;
	char  sql_query_string[QUERY_SIZE] = {0, };
	char *mailbox_id_string_buffer = NULL;
	char *parameter_string = NULL;
	sqlite3 *local_db_handle = NULL;

	if (input_mailbox_id_array == NULL || input_mailbox_id_count == 0 || input_field_name == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	/* Generating mail id list string */
	mailbox_id_string_buffer_length = MAILBOX_ID_STRING_LENGTH * input_mailbox_id_count;

	mailbox_id_string_buffer = em_malloc(mailbox_id_string_buffer_length);

	if (!mailbox_id_string_buffer) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < input_mailbox_id_count; i++)
		cur_mailbox_id_string += SNPRINTF_OFFSET(mailbox_id_string_buffer, cur_mailbox_id_string, mailbox_id_string_buffer_length, "%d,", input_mailbox_id_array[i]);

	if (EM_SAFE_STRLEN(mailbox_id_string_buffer) > 1)
		mailbox_id_string_buffer[EM_SAFE_STRLEN(mailbox_id_string_buffer) - 1] = NULL_CHAR;

	/* Generating notification parameter string */
	parameter_string = em_malloc(mailbox_id_string_buffer_length + EM_SAFE_STRLEN(input_field_name) + 2);

	if (!parameter_string) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	SNPRINTF(parameter_string, QUERY_SIZE, "%s%c%s", input_field_name, 0x01, mailbox_id_string_buffer);

	/* Write query string */
	SNPRINTF(sql_query_string, QUERY_SIZE, "UPDATE mail_box_tbl SET %s = %d WHERE mailbox_id in (%s) ", input_field_name, input_value, mailbox_id_string_buffer);

	EM_DEBUG_LOG_SEC("sql_query_string [%s]", sql_query_string);

	/* Execute query */
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, err);
	err = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", err);
			goto FINISH_OFF;
	}

	if (sqlite3_changes(local_db_handle) == 0)
		EM_DEBUG_LOG("no mail matched...");

	result = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, result, err);

	if (err == EMAIL_ERROR_NONE && parameter_string) {
		if (!emcore_notify_storage_event(NOTI_MAILBOX_FIELD_UPDATE, input_account_id, 0, parameter_string, input_value))
			EM_DEBUG_EXCEPTION_SEC("emcore_notify_storage_eventfailed : NOTI_MAILBOX_FIELD_UPDATE [%s,%d]",
                                                                                         input_field_name, input_value);
	}

	EM_SAFE_FREE(mailbox_id_string_buffer);
	EM_SAFE_FREE(parameter_string);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emstorage_add_mailbox(char *multi_user_name, emstorage_mailbox_tbl_t *mailbox_tbl, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_tbl[%p], transaction[%d], err_code[%p]", mailbox_tbl, transaction, err_code);

	if (!mailbox_tbl) {
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
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	EM_SAFE_STRCPY(sql_query_string, "SELECT max(rowid) FROM mail_box_tbl;");

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
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
		"(?"    /* mailbox_id */
		", ?"    /* account_id */
		", ?"    /* local_yn */
		", ?"    /* mailbox_name */
		", ?"    /* mailbox_type */
		", ?"    /* alias */
		", ?"    /* deleted_flag */
		", ?"    /* modifiable_yn */
		", ?"    /* total_mail_count_on_server */
		", ?"    /* has_archived_mails */
		", ?"    /* mail_slot_size */
		", ?"    /* no_select */
		", ?"    /* last_sync_time */
		", ?"    /* eas_data_length */
		", ?"    /* eas_data */
		")");


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG_DEV("After sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	int col_index = 0;

	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->mailbox_id);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->account_id);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->local_yn);
	_bind_stmt_field_data_string(hStmt, col_index++, (char *)mailbox_tbl->mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_BOX_TBL);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->mailbox_type);
	_bind_stmt_field_data_string(hStmt, col_index++, (char *)mailbox_tbl->alias, 0, ALIAS_LEN_IN_MAIL_BOX_TBL);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->deleted_flag);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->modifiable_yn);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->total_mail_count_on_server);
	_bind_stmt_field_data_int(hStmt, col_index++, 0);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->mail_slot_size);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->no_select);
	_bind_stmt_field_data_int(hStmt, col_index++, current_time);
	_bind_stmt_field_data_int(hStmt, col_index++, mailbox_tbl->eas_data_length);
	_bind_stmt_field_data_blob(hStmt, col_index++, (void*)mailbox_tbl->eas_data, mailbox_tbl->eas_data_length);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%dn", rc));

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (error == EMAIL_ERROR_NONE) {
		if (!emcore_notify_storage_event(NOTI_MAILBOX_ADD, mailbox_tbl->account_id, mailbox_tbl->mailbox_id,
                                                                  mailbox_tbl->mailbox_name, mailbox_tbl->mailbox_type))
			EM_DEBUG_EXCEPTION("emcore_notify_storage_event[ NOTI_MAILBOX_ADD] : Notification Failed");
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_set_all_mailbox_modifiable_yn(char *multi_user_name, int account_id, int modifiable_yn, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], modifiable_yn[%d], err_code[%p]", account_id, modifiable_yn, err_code);

	if (account_id < FIRST_ACCOUNT_ID) {

		EM_DEBUG_EXCEPTION("account_id[%d]", account_id);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0,};
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);


	SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_box_tbl SET modifiable_yn = %d WHERE account_id = %d", modifiable_yn, account_id);

	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0)
		EM_DEBUG_EXCEPTION("All mailbox_name modifiable_yn set to 0 already");


	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;


}

INTERNAL_FUNC int emstorage_delete_mailbox(char *multi_user_name, int account_id, int local_yn, int input_mailbox_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], local_yn[%d], input_mailbox_id[%d], transaction[%d], err_code[%p]", account_id, local_yn, input_mailbox_id, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID) {
		EM_DEBUG_EXCEPTION(" account_id[%d], local_yn[%d], input_mailbox_id[%d]", account_id, local_yn, input_mailbox_id);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	if (local_yn == -1)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_box_tbl WHERE account_id = %d ", account_id);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_box_tbl WHERE account_id = %d AND local_yn = %d ", account_id, local_yn);

	if (input_mailbox_id > 0) {		/* 0 means all mailbox */
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(1 + EM_SAFE_STRLEN(sql_query_string)), "AND mailbox_id = %d", input_mailbox_id);
	}

	EM_DEBUG_LOG_SEC("mailbox sql_query_string [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION(" no (matched) mailbox_name found...");
		error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
		ret = true;
	}
 	ret = true;

FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (error == EMAIL_ERROR_NONE) {
		if (!emcore_notify_storage_event(NOTI_MAILBOX_DELETE, account_id, input_mailbox_id, NULL, 0))
			EM_DEBUG_EXCEPTION("emcore_notify_storage_event[ NOTI_MAILBOX_ADD] : Notification Failed");
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

	if (count > 0) {
		if (!mailbox_list || !*mailbox_list) {
			EM_DEBUG_EXCEPTION(" mailbox_list[%p], count[%d]", mailbox_list, count);

			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		emstorage_mailbox_tbl_t* p = *mailbox_list;
		int i = 0;

		for (; i < count; i++) {
			EM_SAFE_FREE(p[i].mailbox_name);
			EM_SAFE_FREE(p[i].alias);
			EM_SAFE_FREE(p[i].eas_data); /*valgrind*/
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

INTERNAL_FUNC int emstorage_get_count_read_mail_uid(char *multi_user_name, int account_id, char *mailbox_name, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%p], count[%p], transaction[%d], err_code[%p]", account_id, mailbox_name , count,  transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID || !mailbox_name || !count) {
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_name[%p], count[%p], exist[%p]", account_id, mailbox_name, count);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *replaced_mailbox_name = NULL;

	if (strstr(mailbox_name, "'")) {
		replaced_mailbox_name = em_replace_all_string(mailbox_name, "'", "''");
	} else {
		replaced_mailbox_name = EM_SAFE_STRDUP(mailbox_name);
	}

	EM_DEBUG_LOG_SEC("replaced_mailbox_name : [%s]", replaced_mailbox_name);


	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_read_mail_uid_tbl WHERE account_id = %d AND mailbox_name = '%s'  ", account_id, replaced_mailbox_name);
	EM_DEBUG_LOG_SEC(">>> SQL [ %s ] ", sql_query_string);

	char **result;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	EM_SAFE_FREE(replaced_mailbox_name);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_check_read_mail_uid(char *multi_user_name, int account_id, char *mailbox_name, char *uid, int *exist, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%p], uid[%p], exist[%p], transaction[%d], err_code[%p]", account_id, mailbox_name , uid, exist, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID || !uid || !exist) {
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_name[%p], uid[%p], exist[%p]", account_id, mailbox_name , uid, exist);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *replaced_mailbox_name = NULL;

	EM_DEBUG_LOG_SEC("replaced_mailbox_name : [%s]", replaced_mailbox_name);

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	if (mailbox_name) {
		if (strstr(mailbox_name, "'")) {
			replaced_mailbox_name = em_replace_all_string(mailbox_name, "'", "''");
		} else {
			replaced_mailbox_name = strdup(mailbox_name);
		}

		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_read_mail_uid_tbl WHERE account_id = %d AND mailbox_name = '%s' AND server_uid = '%s' ", account_id, replaced_mailbox_name, uid);
	} else {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_read_mail_uid_tbl WHERE account_id = %d AND server_uid = '%s' ", account_id, uid);
	}

	char **result = NULL;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
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

	EM_SAFE_FREE(replaced_mailbox_name);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_downloaded_mail(char *multi_user_name, int mail_id, emstorage_mail_tbl_t **mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], mail[%p], err_code[%p]", mail_id, mail, err_code);

	if (!mail || mail_id <= 0) {
		EM_DEBUG_EXCEPTION("mail_id[%d], mail[%p]", mail_id, mail);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_read_mail_uid_tbl WHERE local_uid = %d", mail_id);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
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
	_get_stmt_field_data_string(hStmt, &((*mail)->server_mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_get_stmt_field_data_int(hStmt, &((*mail)->mail_id), LOCAL_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_get_stmt_field_data_string(hStmt, &((*mail)->server_mail_id), 0, SERVER_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_get_stmt_field_data_int(hStmt, &((*mail)->mail_size), RFC822_SIZE_IDX_IN_MAIL_READ_MAIL_UID_TBL);
	_get_stmt_field_data_char(hStmt, &((*mail)->flags_seen_field), FLAGS_SEEN_FIELD_IDX_IN_MAIL_READ_MAIL_UID_TBL);

	(*mail)->server_mail_status = 1;

	ret = true;

FINISH_OFF:

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_downloaded_list(char *multi_user_name, int account_id, int mailbox_id, emstorage_read_mail_uid_tbl_t **read_mail_uid, int *count, int transaction, int *err_code)
{
	EM_PROFILE_BEGIN(emStorageGetDownloadList);
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_id[%d], read_mail_uid[%p], count[%p], transaction[%d], err_code[%p]", account_id, mailbox_id, read_mail_uid, count, transaction, err_code);
	if (account_id < FIRST_ACCOUNT_ID || !read_mail_uid || !count) {
		EM_DEBUG_EXCEPTION_SEC(" account_id[%d], mailbox_id[%s], read_mail_uid[%p], count[%p]", account_id, mailbox_id, read_mail_uid, count);

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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	if (mailbox_id)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_read_mail_uid_tbl WHERE account_id = %d AND mailbox_id = %d", account_id, mailbox_id);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_read_mail_uid_tbl WHERE account_id = %d", account_id);

	EM_DEBUG_LOG_SEC(" sql_query_string : %s", sql_query_string);



	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	char **result;
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, count, NULL, NULL); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	sqlite3_free_table(result);
	if (*count == 0) {
		EM_DEBUG_LOG("No mail found in mail_read_mail_uid_tbl");
		ret = true;
		goto FINISH_OFF;
	}


	if (!(p_data_tbl = (emstorage_read_mail_uid_tbl_t*)malloc(sizeof(emstorage_read_mail_uid_tbl_t) * *count))) {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(p_data_tbl, 0x00, sizeof(emstorage_read_mail_uid_tbl_t)*(*count));

	for (i = 0; i < *count; ++i) {
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].account_id), ACCOUNT_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].mailbox_id), LOCAL_MAILBOX_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].local_uid), LOCAL_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].server_uid), 0, SERVER_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].rfc822_size), RFC822_SIZE_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_get_stmt_field_data_char(hStmt, &(p_data_tbl[i].flags_seen_field), FLAGS_SEEN_FIELD_IDX_IN_MAIL_READ_MAIL_UID_TBL);
		_get_stmt_field_data_char(hStmt, &(p_data_tbl[i].flags_flagged_field), FLAGS_FLAGGED_FIELD_IDX_IN_MAIL_READ_MAIL_UID_TBL);

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
	}

	ret = true;

FINISH_OFF:
	if (ret == true)
		*read_mail_uid = p_data_tbl;
	else if (p_data_tbl)
		emstorage_free_read_mail_uid(&p_data_tbl, *count, NULL);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(emStorageGetDownloadList);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_downloaded_mail_size(char *multi_user_name, int account_id, char *mailbox_id, int local_uid, char *mailbox_name, char *uid, int *mail_size, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_id[%p], locacal_uid[%d], mailbox_name[%p], uid[%p], mail_size[%p], transaction[%d], err_code[%p]", account_id, mailbox_id, local_uid, mailbox_name, uid, mail_size, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID || !mail_size) {
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_id[%p], locacal_uid[%d], mailbox_name[%p], uid[%p], mail_size[%p]", account_id, mailbox_id, local_uid, mailbox_name, uid, mail_size);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *replaced_mailbox_name = NULL;

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	if (mailbox_name) {
		if (strstr(mailbox_name, "'")) {
			replaced_mailbox_name = em_replace_all_string(mailbox_name, "'", "''");
		} else {
			replaced_mailbox_name = strdup(mailbox_name);
		}

		EM_DEBUG_LOG_SEC("replaced_mailbox_name : [%s]", replaced_mailbox_name);

		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"SELECT IFNULL(MAX(data1), 0) FROM mail_read_mail_uid_tbl "
			"WHERE account_id = %d "
			"AND mailbox_id = '%s' "
			"AND local_uid = %d "
			"AND mailbox_name = '%s' "
			"AND server_uid = '%s'",
			account_id, mailbox_id, local_uid, replaced_mailbox_name, uid);
	} else {
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"SELECT IFNULL(MAX(data1), 0) FROM mail_read_mail_uid_tbl "
			"WHERE account_id = %d "
			"AND mailbox_id = '%s' "
			"AND local_uid = %d "
			"AND server_uid = '%s'",
			account_id, mailbox_id, local_uid, uid);
	}


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_LOG("no matched mail found....");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	_get_stmt_field_data_int(hStmt, mail_size, 0);

	ret = true;

FINISH_OFF:
	EM_SAFE_FREE(replaced_mailbox_name);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_add_downloaded_mail(char *multi_user_name, emstorage_read_mail_uid_tbl_t *read_mail_uid, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("read_mail_uid[%p], transaction[%d], err_code[%p]", read_mail_uid, transaction, err_code);

	if (!read_mail_uid) {
		EM_DEBUG_EXCEPTION("read_mail_uid[%p]", read_mail_uid);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, rc2,  ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	char *sql = "SELECT max(rowid) FROM mail_read_mail_uid_tbl;";
	char **result = NULL;


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL == result[1]) rc = 1;
	else rc = atoi(result[1])+1;
	sqlite3_free_table(result);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_read_mail_uid_tbl VALUES "
		"(?"  /* account_id */
		", ?"  /* mailbox_id */
		", ?"  /* mailbox_name */
		", ?"  /* local_uid */
		", ?"  /* server_uid */
		", ?"  /* rfc822_size */
		", ?"  /* sync_status */
		", ?"  /* flags_seen_field */
		", ?"  /* flags_flagged_field */
		", ?)");


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc2);
	if (rc2 != SQLITE_OK) {
		EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);
		EM_DEBUG_EXCEPTION("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle));

		error = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("account_id[%d] mailbox_id[%d] local_uid [%d]"
                   "server_uid[%s] rfc822_size[%d] rc[%d]",
         read_mail_uid->account_id, read_mail_uid->mailbox_id, read_mail_uid->local_uid,
         read_mail_uid->server_uid, read_mail_uid->rfc822_size, rc);

	_bind_stmt_field_data_int(hStmt, ACCOUNT_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->account_id);
	_bind_stmt_field_data_int(hStmt, LOCAL_MAILBOX_ID_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->mailbox_id);
	_bind_stmt_field_data_int(hStmt, LOCAL_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->local_uid);
	_bind_stmt_field_data_string(hStmt, MAILBOX_NAME_IDX_IN_MAIL_READ_MAIL_UID_TBL, (char *)read_mail_uid->mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bind_stmt_field_data_string(hStmt, SERVER_UID_IDX_IN_MAIL_READ_MAIL_UID_TBL, (char *)read_mail_uid->server_uid, 0, S_UID_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bind_stmt_field_data_int(hStmt, RFC822_SIZE_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->rfc822_size);
	_bind_stmt_field_data_int(hStmt, FLAGS_SEEN_FIELD_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->flags_seen_field);
	_bind_stmt_field_data_int(hStmt, FLAGS_FLAGGED_FIELD_IDX_IN_MAIL_READ_MAIL_UID_TBL, read_mail_uid->flags_flagged_field);
	_bind_stmt_field_data_int(hStmt, IDX_NUM_IDX_IN_MAIL_READ_MAIL_UID_TBL, rc);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail[%d] [%s]", rc, sqlite3_errmsg(local_db_handle)));


	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

#ifdef __FEATURE_BODY_SEARCH__
INTERNAL_FUNC int emstorage_add_mail_text(char *multi_user_name, emstorage_mail_text_tbl_t* mail_text, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_text[%p], transaction[%d], err_code[%p]", mail_text, transaction, err_code);

	if (!mail_text) {
		EM_DEBUG_EXCEPTION("mail_text[%p]", mail_text);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, rc2,  ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	char *sql = "SELECT max(rowid) FROM mail_text_tbl;";
	char **result = NULL;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));
	sqlite3_free_table(result);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_text_tbl VALUES "
		"(?"
		", ?"
		", ?"
		", ?)");

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc2);
	if (rc2 != SQLITE_OK) {
		EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);
		EM_DEBUG_EXCEPTION("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc2, sqlite3_errmsg(local_db_handle));

		error = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("mail_id[%d] account_id[%d] mailbox_id[%d]", mail_text->mail_id,
                                       mail_text->account_id, mail_text->mailbox_id);
	EM_DEBUG_LOG_DEV("body_text VALUE [%s] ", mail_text->body_text);

	_bind_stmt_field_data_int(hStmt, MAIL_ID_IDX_IN_MAIL_TEXT_TBL, mail_text->mail_id);
	_bind_stmt_field_data_int(hStmt, ACCOUNT_ID_IDX_IN_MAIL_TEXT_TBL, mail_text->account_id);
	_bind_stmt_field_data_int(hStmt, MAILBOX_ID_IDX_IN_MAIL_TEXT_TBL, mail_text->mailbox_id);
	_bind_stmt_field_data_string(hStmt, BODY_TEXT_IDX_IN_MAIL_TEXT_TBL, (char *)mail_text->body_text, 0, -1);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail[%d] [%s]", rc, sqlite3_errmsg(local_db_handle)));

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
#endif

INTERNAL_FUNC int emstorage_change_read_mail_uid(char *multi_user_name, int account_id, int mailbox_id, int local_uid, char *mailbox_name, char *uid, emstorage_read_mail_uid_tbl_t* read_mail_uid, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_id[%d], local_uid[%d], mailbox_name[%p], uid[%p], read_mail_uid[%p], transaction[%d], err_code[%p]", account_id, mailbox_id, local_uid, mailbox_name, uid, read_mail_uid, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID || !read_mail_uid) {
		EM_DEBUG_EXCEPTION(" account_id[%d], mailbox_id[%d], local_uid[%d], mailbox_name[%p], uid[%p], read_mail_uid[%p]", account_id, mailbox_id, local_uid, mailbox_name, uid, read_mail_uid);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_read_mail_uid_tbl SET"
		"  account_id = ?"
		", mailbox_id = ?"
		", mailbox_name = ?"
		", local_uid  = ?"
		", server_uid = ?"
		", rfc822_size = ?"
		", flags_seen_field  = ?"
		", flags_flagged_field  = ?"
		" WHERE account_id = ?"
		" AND mailbox_id  = ?"
		" AND local_uid   = ?"
		" AND mailbox_name= ?"
		" AND server_uid = ?");


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	int i = 0;

	_bind_stmt_field_data_int(hStmt, i++, read_mail_uid->account_id);
	_bind_stmt_field_data_int(hStmt, i++, read_mail_uid->mailbox_id);
	_bind_stmt_field_data_string(hStmt, i++, (char*)read_mail_uid->mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bind_stmt_field_data_int(hStmt, i++, read_mail_uid->local_uid);
	_bind_stmt_field_data_string(hStmt, i++, (char*)read_mail_uid->server_uid, 0, S_UID_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bind_stmt_field_data_int(hStmt, i++, read_mail_uid->rfc822_size);
	_bind_stmt_field_data_int(hStmt, i++, read_mail_uid->flags_seen_field);
	_bind_stmt_field_data_int(hStmt, i++, read_mail_uid->flags_flagged_field);
	_bind_stmt_field_data_int(hStmt, i++, account_id);
	_bind_stmt_field_data_int(hStmt, i++, mailbox_id);
	_bind_stmt_field_data_int(hStmt, i++, local_uid);
	_bind_stmt_field_data_string(hStmt, i++, (char*)mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_READ_MAIL_UID_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char*)uid, 0, S_UID_LEN_IN_MAIL_READ_MAIL_UID_TBL);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_remove_downloaded_mail(char *multi_user_name,
													int account_id,
													int mailbox_id,
													char *mailbox_name,
													char *uid,
													int transaction,
													int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("account_id[%d], mailbox_id[%d], mailbox_name[%s], "
							"uid[%s], transaction[%d], err_code[%p]",
							account_id, mailbox_id, mailbox_name, uid, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID) {
		EM_DEBUG_EXCEPTION_SEC(" account_id[%d], mailbox_name[%s], uid[%s]", account_id, mailbox_name, uid);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
 	char sql_query_string[QUERY_SIZE] = {0, };
	char *replaced_mailbox_name = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"DELETE FROM mail_read_mail_uid_tbl WHERE account_id = %d ", account_id);

	if (mailbox_id > 0) {
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string),
					sizeof(sql_query_string) - (1 + EM_SAFE_STRLEN(sql_query_string)),
					"AND mailbox_id = %d ", mailbox_id);
	}

	if (mailbox_name) {		/*  NULL means all mailbox_name */
		if (strstr(mailbox_name, "'")) {
			replaced_mailbox_name = em_replace_all_string(mailbox_name, "'", "''");
		} else {
			replaced_mailbox_name = strdup(mailbox_name);
		}

		SNPRINTF(sql_query_string+EM_SAFE_STRLEN(sql_query_string),
					sizeof(sql_query_string) - (1 + EM_SAFE_STRLEN(sql_query_string)),
					"AND mailbox_name = '%s' ", replaced_mailbox_name);
	}

	if (uid) {		/*  NULL means all mail */
		sqlite3_snprintf(sizeof(sql_query_string) - (1 + EM_SAFE_STRLEN(sql_query_string)),
						 sql_query_string + EM_SAFE_STRLEN(sql_query_string), "AND server_uid = '%q' ",
						 uid);
	}

	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	EM_SAFE_FREE(replaced_mailbox_name);

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

	if (count > 0) {
		if (!read_mail_uid || !*read_mail_uid) {
			EM_DEBUG_EXCEPTION(" read_mail_uid[%p], count[%d]", read_mail_uid, count);

			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		emstorage_read_mail_uid_tbl_t* p = *read_mail_uid;
		int i;

		for (i = 0; i < count; i++) {
			EM_SAFE_FREE(p[i].mailbox_name);
			EM_SAFE_FREE(p[i].server_uid);
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

INTERNAL_FUNC int emstorage_get_rule_count_by_account_id(char *multi_user_name, int account_id, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], count[%p], transaction[%d], err_code[%p]", count, transaction, err_code);

	if (!count) {
		EM_DEBUG_EXCEPTION("count[%p]", count);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1, ret = false;
	int error =  EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	if (account_id != ALL_ACCOUNT)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_rule_tbl where account_id = %d", account_id);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_rule_tbl");

	char **result;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_rule(char *multi_user_name, int account_id, int type, int start_idx, int *select_num, int *is_completed, emstorage_rule_tbl_t** rule_list, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], type[%d], start_idx[%d], select_num[%p], is_completed[%p], rule_list[%p], transaction[%d], err_code[%p]", account_id, type, start_idx, select_num, is_completed, rule_list, transaction, err_code);

	if (!select_num || !is_completed || !rule_list) {		/*  only global rule supported. */
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	if (account_id != ALL_ACCOUNT) {
		if (type)
			SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_rule_tbl WHERE account_id = %d AND type = %d", account_id, type);
		else
			SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_rule_tbl WHERE account_id = %d ORDER BY type", account_id);
	} else {
		if (type)
			SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_rule_tbl WHERE type = %d", type);
		else
			SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_rule_tbl ORDER BY type");
	}

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
/*	EM_DEBUG_LOG("sqlite3_prepare hStmt = %p", hStmt); */
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	char **result;
	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	sqlite3_free_table(result);

	if (count == 0) {
		EM_DEBUG_LOG_DEV("No matching rule found...");
		ret = true;
		error = EMAIL_ERROR_FILTER_NOT_FOUND; /*there is no matched rule*/
		goto FINISH_OFF;
	}


	if (!(p_data_tbl = (emstorage_rule_tbl_t*)malloc(sizeof(emstorage_rule_tbl_t) * count))) {
		EM_DEBUG_EXCEPTION(" malloc failed...");

		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(p_data_tbl, 0x00, sizeof(emstorage_rule_tbl_t) * count);

	for (i = 0; i < count; i++) {
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].account_id), ACCOUNT_ID_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].rule_id), RULE_ID_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].filter_name), 0, FILTER_NAME_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].type), TYPE_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].value), 0, VALUE_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].value2), 0, VALUE2_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].action_type), ACTION_TYPE_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].target_mailbox_id), TARGET_MAILBOX_ID_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].flag1), FLAG1_IDX_IN_MAIL_RULE_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].flag2), FLAG2_IDX_IN_MAIL_RULE_TBL);

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
	}

	ret = true;

FINISH_OFF:

	EM_DEBUG_LOG("[%d] rules found.", count);

	if (ret == true) {
		*rule_list = p_data_tbl;
		*select_num = count;
	} else if (p_data_tbl != NULL)
		emstorage_free_rule(&p_data_tbl, count, NULL); /* CID FIX */

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_rule_by_id(char *multi_user_name, int rule_id, emstorage_rule_tbl_t** rule, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("rule_id[%d], rule[%p], transaction[%d], err_code[%p]", rule_id, rule, transaction, err_code);
	int error = EMAIL_ERROR_NONE;
	int ret = false;
	DB_STMT hStmt = NULL;

	if (rule_id <= 0) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!rule) {
		EM_DEBUG_EXCEPTION("rule_id[%d], rule[%p]", rule_id, rule);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	emstorage_rule_tbl_t* p_data_tbl = NULL;
	int rc;

	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_rule_tbl WHERE rule_id = %d", rule_id);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION(" no matched rule found...");
		error = EMAIL_ERROR_FILTER_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (!(p_data_tbl = (emstorage_rule_tbl_t*)malloc(sizeof(emstorage_rule_tbl_t)))) {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(p_data_tbl, 0x00, sizeof(emstorage_rule_tbl_t));
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), ACCOUNT_ID_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->rule_id), RULE_ID_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->filter_name), 0, FILTER_NAME_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->type), TYPE_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->value), 0, VALUE_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->value2), 0, VALUE2_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->action_type), ACTION_TYPE_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->target_mailbox_id), TARGET_MAILBOX_ID_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->flag1), FLAG1_IDX_IN_MAIL_RULE_TBL);
	_get_stmt_field_data_int(hStmt, &(p_data_tbl->flag2), FLAG2_IDX_IN_MAIL_RULE_TBL);

	ret = true;

FINISH_OFF:

	if (ret == true)
		*rule = p_data_tbl;

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_change_rule(char *multi_user_name, int rule_id, emstorage_rule_tbl_t* new_rule, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("rule_id[%d], new_rule[%p], transaction[%d], err_code[%p]", rule_id, new_rule, transaction, err_code);

	if (!new_rule) {		/*  only global rule supported. */
		EM_DEBUG_EXCEPTION("rule_id[%d], new_rule[%p]", rule_id, new_rule);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;

	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_rule_tbl SET"
		"  filter_name = ?"
		", type = ?"
		", value = ?"
		", value2 = ?"
		", action_type = ?"
		", target_mailbox_id = ?"
		", flag1 = ?"
		", flag2 = ?"
		", account_id = ?"
		", rule_id = ?"
                " WHERE rule_id = %d"
                , rule_id);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG(" Before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	int i = 0;

	_bind_stmt_field_data_string(hStmt, i++, (char *)new_rule->filter_name, 0, FILTER_NAME_LEN_IN_MAIL_RULE_TBL);
	_bind_stmt_field_data_int(hStmt, i++, new_rule->type);
	_bind_stmt_field_data_string(hStmt, i++, (char *)new_rule->value, 0, VALUE_LEN_IN_MAIL_RULE_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)new_rule->value2, 0, VALUE2_LEN_IN_MAIL_RULE_TBL);
	_bind_stmt_field_data_int(hStmt, i++, new_rule->action_type);
	_bind_stmt_field_data_int(hStmt, i++, new_rule->target_mailbox_id);
	_bind_stmt_field_data_int(hStmt, i++, new_rule->flag1);
	_bind_stmt_field_data_int(hStmt, i++, new_rule->flag2);
	_bind_stmt_field_data_int(hStmt, i++, new_rule->account_id);
	_bind_stmt_field_data_int(hStmt, i++, rule_id);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_find_rule(char *multi_user_name, emstorage_rule_tbl_t* rule, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("rule[%p], transaction[%d], err_code[%p]", rule, transaction, err_code);

	if (!rule) {
		EM_DEBUG_LOG("rule is NULL");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0,};
	int error = EMAIL_ERROR_NONE;
	int rc = 0;
	int ret = false;

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	switch (rule->action_type) {
	case EMAIL_FILTER_MOVE:
		if (rule->type == EMAIL_PRIORITY_SENDER) {
			sqlite3_snprintf(sizeof(sql_query_string), sql_query_string,
				"SELECT rule_id FROM mail_rule_tbl WHERE action_type = %d AND type = %d AND UPPER(value2) = UPPER(\'%q\')",
				rule->action_type, rule->type, rule->value2);
		} else {
				sqlite3_snprintf(sizeof(sql_query_string), sql_query_string,
					"SELECT rule_id FROM mail_rule_tbl WHERE action_type = %d AND type = %d AND UPPER(filter_name) = UPPER(\'%q\')", rule->action_type, rule->type, rule->filter_name);
		}
		break;
	case EMAIL_FILTER_BLOCK:
		if (rule->type == EMAIL_FILTER_FROM)
			sqlite3_snprintf(sizeof(sql_query_string), sql_query_string,
				"SELECT rule_id FROM mail_rule_tbl WHERE action_type = %d AND type = %d AND UPPER(value2) = UPPER(\'%q\')",
				rule->action_type, rule->type, rule->value2);
		else if (rule->type == EMAIL_FILTER_SUBJECT)
			sqlite3_snprintf(sizeof(sql_query_string), sql_query_string,
				"SELECT rule_id FROM mail_rule_tbl WHERE action_type = %d AND type = %d AND UPPER(value) = UPPER(\'%q\')",
				rule->action_type, rule->type, rule->value);
		else if (rule->type == (EMAIL_FILTER_SUBJECT | EMAIL_FILTER_FROM))
			sqlite3_snprintf(sizeof(sql_query_string), sql_query_string,
				"SELECT rule_id FROM mail_rule_tbl WHERE action_type = %d AND (type = %d AND UPPER(value) = UPPER(\'%q\')) OR (type = %d AND UPPER(value2) = UPPER(\'%q\'))",
				rule->action_type, EMAIL_FILTER_SUBJECT, rule->value, EMAIL_FILTER_FROM, rule->value2);
		break;

	default:
		EM_DEBUG_EXCEPTION("Invalid parameter : rule->action_type[%d]", rule->action_type);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
		break;
	}

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION(" no matched rule found...");
		error = EMAIL_ERROR_FILTER_NOT_FOUND;
	}

	ret = true;

FINISH_OFF:

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_add_rule(char *multi_user_name, emstorage_rule_tbl_t* rule, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("rule[%p], transaction[%d], err_code[%p]", rule, transaction, err_code);

	if (!rule) {	/*  only global rule supported. */
		EM_DEBUG_LOG("rule is NULL");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, rc_2, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
 	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	char *sql;
	char **result;
	sql = "SELECT max(rowid) FROM mail_rule_tbl;";

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL == result[1])
		rc = 1;
	else
		rc = atoi(result[1])+1;

	sqlite3_free_table(result);

	rule->rule_id = rc;

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_rule_tbl VALUES "
		"(?"		/*  account id */
		", ?"		/*  rule_id */
		", ?"		/*  filter_name */
		", ?"		/*  type */
		", ?"		/*  value */
		", ?"		/*  value2 */
		", ?"		/*  action_type */
		", ?"		/*  target_mailbox_id */
		", ?"		/*  flag1 */
		", ?)");	/*  flag2 */

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc_2);
	if (rc_2 != SQLITE_OK) {
		EM_DEBUG_EXCEPTION("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc_2, sqlite3_errmsg(local_db_handle));
		error = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	_bind_stmt_field_data_int(hStmt, ACCOUNT_ID_IDX_IN_MAIL_RULE_TBL, rule->account_id);
	_bind_stmt_field_data_int(hStmt, RULE_ID_IDX_IN_MAIL_RULE_TBL, rule->rule_id);
	_bind_stmt_field_data_string(hStmt, FILTER_NAME_IDX_IN_MAIL_RULE_TBL, (char*)rule->filter_name, 0, FILTER_NAME_LEN_IN_MAIL_RULE_TBL);
	_bind_stmt_field_data_int(hStmt, TYPE_IDX_IN_MAIL_RULE_TBL, rule->type);
	_bind_stmt_field_data_string(hStmt, VALUE_IDX_IN_MAIL_RULE_TBL, (char*)rule->value, 0, VALUE_LEN_IN_MAIL_RULE_TBL);
	_bind_stmt_field_data_string(hStmt, VALUE2_IDX_IN_MAIL_RULE_TBL, (char*)rule->value2, 0, VALUE2_LEN_IN_MAIL_RULE_TBL);
	_bind_stmt_field_data_int(hStmt, ACTION_TYPE_IDX_IN_MAIL_RULE_TBL, rule->action_type);
	_bind_stmt_field_data_int(hStmt, TARGET_MAILBOX_ID_IDX_IN_MAIL_RULE_TBL, rule->target_mailbox_id);
	_bind_stmt_field_data_int(hStmt, FLAG1_IDX_IN_MAIL_RULE_TBL, rule->flag1);
	_bind_stmt_field_data_int(hStmt, FLAG2_IDX_IN_MAIL_RULE_TBL, rule->flag2);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_rule(char *multi_user_name, int rule_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("rule_id[%d], transaction[%d], err_code[%p]", rule_id, transaction, err_code);

	if (rule_id <= 0) {		/*  only global rule supported. */
		EM_DEBUG_EXCEPTION("rule_id[%d]", rule_id);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_rule_tbl WHERE rule_id = %d", rule_id);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION(" no matched rule found...");

		error = EMAIL_ERROR_FILTER_NOT_FOUND;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

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

INTERNAL_FUNC int emstorage_get_mail_count(char *multi_user_name, int account_id, int mailbox_id, int *total, int *unseen, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_id[%d], total[%p], unseen[%p], transaction[%d], err_code[%p]", account_id, mailbox_id, total, unseen, transaction, err_code);

	if (!total && !unseen) {
		EM_DEBUG_EXCEPTION(" accoun_id[%d], mailbox_id[%d], total[%p], unseen[%p]", account_id, mailbox_id, total, unseen);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *replaced_mailbox_name = NULL;

	memset(&sql_query_string, 0x00, sizeof(sql_query_string));
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	if (total) {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_tbl");

		if (account_id != ALL_ACCOUNT) {
			SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " WHERE account_id = %d", account_id);
			if (mailbox_id)
				SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " AND mailbox_id = %d", mailbox_id);
		} else if (mailbox_id)
			SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " WHERE mailbox_id = %d", mailbox_id);

#ifdef USE_GET_RECORD_COUNT_API
	 	char **result;

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF2; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		*total = atoi(result[1]);
		sqlite3_free_table(result);
#else

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
		EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF2; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF2; },
			("sqlite3_step fail:%d", rc));
		_get_stmt_field_data_int(hStmt, total, 0);
#endif		/*  USE_GET_RECORD_COUNT_API */
	}

	if (unseen) {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_tbl WHERE flags_seen_field = 0");		/*  fSEEN = 0x01 */

		if (account_id != ALL_ACCOUNT)
			SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " AND account_id = %d", account_id);

		if (mailbox_id) {
			SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " AND mailbox_id = %d", mailbox_id);
		} else
			SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " AND mailbox_type NOT IN (3, 5)");

 		char **result;
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		*unseen = atoi(result[1]);
		sqlite3_free_table(result);

	}
FINISH_OFF:
	ret = true;

FINISH_OFF2:

#ifndef USE_PREPARED_QUERY_
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}
#endif

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	EM_SAFE_FREE(replaced_mailbox_name);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mail_field_by_id(char *multi_user_name, int mail_id, int type, emstorage_mail_tbl_t** mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], type[%d], mail[%p], transaction[%d], err_code[%p]", mail_id, type, mail, transaction, err_code);

	if (mail_id <= 0 || !mail) {
		EM_DEBUG_EXCEPTION("mail_id[%d], mail[%p]", mail_id, mail);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int col_index = 0;
	emstorage_mail_tbl_t* p_data_tbl = (emstorage_mail_tbl_t*)malloc(sizeof(emstorage_mail_tbl_t));

	if (p_data_tbl == NULL) {
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	switch (type) {
		case RETRIEVE_SUMMARY:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"SELECT	account_id, "
				"mail_id, "
				"mailbox_id, "
				"server_mail_status, "
				"server_mailbox_name, "
				"server_mail_id, "
				"file_path_plain, "
				"file_path_html,"
				"file_path_mime_entity, "
				"flags_seen_field, "
				"save_status, "
				"lock_status, "
				"thread_id, "
				"thread_item_count "
				"FROM mail_tbl WHERE mail_id = %d", mail_id);
			break;

		case RETRIEVE_FIELDS_FOR_DELETE:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"SELECT account_id, "
				"mail_id, "
				"server_mail_status, "
				"server_mailbox_name, "
				"server_mail_id "
				"FROM mail_tbl WHERE mail_id = %d", mail_id);
			break;

		case RETRIEVE_ACCOUNT:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"SELECT account_id "
				"FROM mail_tbl WHERE mail_id = %d", mail_id);
			break;

		case RETRIEVE_FLAG:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"SELECT account_id, "
				"flags_seen_field, "
				"thread_id, "
				"mailbox_id "
				"FROM mail_tbl WHERE mail_id = %d", mail_id);
			break;

		default:
			EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM : type [%d]", type);
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_LOG("no matched mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}
	switch (type) {
		case RETRIEVE_SUMMARY:
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), col_index++);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->mail_id), col_index++);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->mailbox_id), col_index++);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->server_mail_status), col_index++);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mailbox_name), 0, col_index++);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mail_id), 0, col_index++);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->file_path_plain), 0, col_index++);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->file_path_html), 0, col_index++);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->file_path_mime_entity), 0, col_index++);
			_get_stmt_field_data_char(hStmt, &(p_data_tbl->flags_seen_field), col_index++);
			_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->save_status), col_index++);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->lock_status), col_index++);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->thread_id), col_index++);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->thread_item_count), col_index++);
			break;

		case RETRIEVE_FIELDS_FOR_DELETE:
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), col_index++);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->mail_id), col_index++);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->server_mail_status), col_index++);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mailbox_name), 0, col_index++);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mail_id), 0, col_index++);
			break;

		case RETRIEVE_ACCOUNT:
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), col_index++);
			break;

		case RETRIEVE_FLAG:
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), col_index++);
			_get_stmt_field_data_char(hStmt, &(p_data_tbl->flags_seen_field), col_index++);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->thread_id), col_index++);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->mailbox_id), col_index++);
			break;
	}

	ret = true;

FINISH_OFF:
	if (ret == true)
		*mail = p_data_tbl;
	else if (p_data_tbl != NULL)
		emstorage_free_mail(&p_data_tbl,  1, NULL);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}


	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mail_field_by_multiple_mail_id(char *multi_user_name, int mail_ids[], int number_of_mails, int type, emstorage_mail_tbl_t** mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], number_of_mails [%d], type[%d], mail[%p], transaction[%d], err_code[%p]", mail_ids, number_of_mails, type, mail, transaction, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int query_string_length = 0;
	int i = 0, item_count = 0, rc = -1, field_count, col_index, cur_sql_query_string = 0;
	char **result = NULL;
	char *sql_query_string = NULL;
	emstorage_mail_tbl_t* p_data_tbl = NULL;
	sqlite3 *local_db_handle = NULL;

	if (number_of_mails <= 0 || !mail_ids) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	p_data_tbl = (emstorage_mail_tbl_t*)em_malloc(sizeof(emstorage_mail_tbl_t) * number_of_mails);

	query_string_length = (sizeof(char) * 8 * number_of_mails) + 512;
	sql_query_string = (char*)em_malloc(query_string_length);

	if (p_data_tbl == NULL || sql_query_string == NULL) {
		EM_DEBUG_EXCEPTION("malloc failed...");

		EM_SAFE_FREE(p_data_tbl);
		EM_SAFE_FREE(sql_query_string);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_OUT_OF_MEMORY;
		return false;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);

	switch (type) {
		case RETRIEVE_SUMMARY:
			cur_sql_query_string = SNPRINTF(sql_query_string, query_string_length,
				"SELECT account_id, "
				"mail_id, "
				"mailbox_id, "
				"server_mail_status, "
				"server_mailbox_name, "
				"server_mail_id, "
				"file_path_plain, "
				"file_path_html, "
				"file_path_mime_entity, "
				"subject, "
				"flags_seen_field, "
				"save_status, "
				"lock_status, "
				"thread_id, "
				"thread_item_count "
				"FROM mail_tbl WHERE mail_id in (");
			field_count = 15;
			break;

		case RETRIEVE_FIELDS_FOR_DELETE:
			cur_sql_query_string = SNPRINTF(sql_query_string, query_string_length,
				"SELECT account_id, "
				"mail_id, "
				"server_mail_status, "
				"server_mailbox_name, "
				"server_mail_id "
				"FROM mail_tbl WHERE mail_id in (");
			field_count = 5;
			break;

		case RETRIEVE_ACCOUNT:
			cur_sql_query_string = SNPRINTF(sql_query_string, query_string_length,
				"SELECT account_id FROM mail_tbl WHERE mail_id in (");
			field_count = 1;
			break;

		case RETRIEVE_FLAG:
			cur_sql_query_string = SNPRINTF(sql_query_string, query_string_length,
				"SELECT account_id, "
				"mail_id, "
				"mailbox_id, "
				"flags_seen_field, "
				"thread_id "
				"FROM mail_tbl WHERE mail_id in (");
			field_count = 5;
			break;

		default:
			EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM : type [%d]", type);
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}

	for (i = 0; i < number_of_mails; i++)
		cur_sql_query_string += SNPRINTF_OFFSET(sql_query_string, cur_sql_query_string, query_string_length, "%d,", mail_ids[i]);
	sql_query_string[EM_SAFE_STRLEN(sql_query_string) - 1] = ')';

	EM_DEBUG_LOG_SEC("Query [%s], Length [%d]", sql_query_string, EM_SAFE_STRLEN(sql_query_string));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &item_count, 0, NULL), rc);
	if (SQLITE_OK != rc && -1 != rc) {
		EM_DEBUG_EXCEPTION("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle));
		error = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	if (rc == SQLITE_DONE) {
		EM_DEBUG_LOG("no matched mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("item_count [%d]", item_count);

	if (number_of_mails != item_count) {
		EM_DEBUG_EXCEPTION("Can't find all emails");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	col_index = field_count;

	for (i = 0; i < item_count; i++)	{
		switch (type) {
			case RETRIEVE_SUMMARY:
				_get_table_field_data_int(result, &(p_data_tbl[i].account_id), col_index++);
				_get_table_field_data_int(result, &(p_data_tbl[i].mail_id), col_index++);
				_get_table_field_data_int(result, &(p_data_tbl[i].mailbox_id), col_index++);
				_get_table_field_data_int(result, &(p_data_tbl[i].server_mail_status), col_index++);
				_get_table_field_data_string(result, &(p_data_tbl[i].server_mailbox_name), 0, col_index++);
				_get_table_field_data_string(result, &(p_data_tbl[i].server_mail_id), 0, col_index++);
				_get_table_field_data_string(result, &(p_data_tbl[i].file_path_plain), 0, col_index++);
				_get_table_field_data_string(result, &(p_data_tbl[i].file_path_html), 0, col_index++);
				_get_table_field_data_string(result, &(p_data_tbl[i].file_path_mime_entity), 0, col_index++);
				_get_table_field_data_string(result, &(p_data_tbl[i].subject), 0, col_index++);
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
				_get_table_field_data_int(result, &(p_data_tbl[i].mail_id), col_index++);
				_get_table_field_data_int(result, &(p_data_tbl[i].mailbox_id), col_index++);
				_get_table_field_data_char(result, &(p_data_tbl[i].flags_seen_field), col_index++);
				_get_table_field_data_int(result, &(p_data_tbl[i].thread_id), col_index++);
				break;
		}
	}

	ret = true;

FINISH_OFF:
	if (ret == true)
		*mail = p_data_tbl;
	else
		emstorage_free_mail(&p_data_tbl, number_of_mails, NULL);

	if (result)
		sqlite3_free_table(result);

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	EM_SAFE_FREE(sql_query_string);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mail_by_id(char *multi_user_name, int mail_id, emstorage_mail_tbl_t **mail, int transaction, int *err_code)
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
	EM_DEBUG_LOG_SEC("query = [%s]", conditional_clause);

	if (!emstorage_query_mail_tbl(multi_user_name, conditional_clause, transaction, &p_data_tbl, &count, &error)) {
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

#ifdef __FEATURE_BODY_SEARCH__
INTERNAL_FUNC int emstorage_get_mail_text_by_id(char *multi_user_name, int mail_id, emstorage_mail_text_tbl_t **mail_text, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], mail_text[%p], transaction[%d], err_code[%p]", mail_id, mail_text, transaction, err_code);

	if (mail_id <= 0 || !mail_text) {
		EM_DEBUG_EXCEPTION("mail_id[%d], mail_text[%p]", mail_id, mail_text);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int count = 0;
	char conditional_clause[QUERY_SIZE] = {0, };
	emstorage_mail_text_tbl_t *p_data_tbl = NULL;

	SNPRINTF(conditional_clause, QUERY_SIZE, "WHERE mail_id = %d", mail_id);
	EM_DEBUG_LOG_SEC("query = [%s]", conditional_clause);

	if (!emstorage_query_mail_text_tbl(multi_user_name, conditional_clause, transaction, &p_data_tbl, &count, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_tbl [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (ret == true)
		*mail_text = p_data_tbl;
	else if (p_data_tbl != NULL)
		emstorage_free_mail_text(&p_data_tbl, 1, &error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
#endif

INTERNAL_FUNC int emstorage_mail_search_start(char *multi_user_name,
											emstorage_search_filter_t *search,
											int account_id,
											int mailbox_id,
											int sorting,
											DB_STMT *search_handle,
											int *searched,
											int transaction,
											int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("search[%p], account_id[%d], mailbox_id[%d], sorting[%d], "
						"search_handle[%p], searched[%p], transaction[%d], err_code[%p]",
						search, account_id, mailbox_id, sorting, search_handle,
						searched, transaction, err_code);

	if (!search_handle || !searched) {
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_tbl");

	if (account_id != ALL_ACCOUNT) {
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " WHERE account_id = %d", account_id);
		and = true;
	}

	if (mailbox_id) {
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " %s mailbox_id = %d", and ? "AND" : "WHERE", mailbox_id);
		and = true;
	}

	while (p) {
		if (p->key_type) {
			if (!strncmp(p->key_type, "subject", strlen("subject"))) {
				SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " %s subject LIKE '%%%%%s%%%%'", and ? "AND" : "WHERE", p->key_value);
				and = true;
			} else if (!strncmp(p->key_type, "full_address_from", strlen("full_address_from"))) {
				SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " %s full_address_from LIKE '%%%%%s%%%%'", and ? "AND" : "WHERE", p->key_value);
				and = true;
			} else if (!strncmp(p->key_type, "full_address_to", strlen("full_address_to"))) {
				SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " %s full_address_to LIKE '%%%%%s%%%%'", and ? "AND" : "WHERE", p->key_value);
				and = true;
			} else if (!strncmp(p->key_type, "email_address", strlen("email_address"))) {
				SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " %s email_address_sender = '%s' OR email_address_recipient = '%s'", and ? "AND" : "WHERE", p->key_value, p->key_value);
				and = true;
			}
			p = p->next;
		}
	}

	if (sorting)
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " ORDER BY date_time");

	EM_DEBUG_LOG_SEC("sql_query_string [%s]", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	char **result;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &mail_count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	sqlite3_free_table(result);

	ret = true;

FINISH_OFF:
	if (ret == true) {
		*search_handle = hStmt;
		*searched = mail_count;
		EM_DEBUG_LOG("mail_count [%d]", mail_count);
	} else {
		if (hStmt != NULL) {
			rc = sqlite3_finalize(hStmt);
			if (rc != SQLITE_OK) {
				EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
				error = EMAIL_ERROR_DB_FAILURE;
			}
		}

		EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_mail_search_result(DB_STMT search_handle, emstorage_mail_field_type_t type, void** data, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("search_handle[%d], type[%d], data[%p], transaction[%d], err_code[%p]", search_handle, type, data, transaction, err_code);

	if (search_handle == 0 || !data) {
		EM_DEBUG_EXCEPTION(" search_handle[%d], type[%d], data[%p]", search_handle, type, data);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	emstorage_mail_tbl_t* p_data_tbl = NULL;
	DB_STMT hStmt = search_handle;
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;

	switch (type) {
		case RETRIEVE_ID:
			_get_stmt_field_data_int(hStmt, (int *)data, MAIL_ID_IDX_IN_MAIL_TBL);
			break;

		case RETRIEVE_ENVELOPE:
		case RETRIEVE_ALL:
			if (!(p_data_tbl = em_malloc(sizeof(emstorage_mail_tbl_t)))) {
				EM_DEBUG_EXCEPTION(" em_mallocfailed...");
				error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			_get_stmt_field_data_int(hStmt, &(p_data_tbl->mail_id), MAIL_ID_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), ACCOUNT_ID_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->mail_size), MAIL_SIZE_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mail_id), 0, SERVER_MAIL_ID_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->full_address_from), 1, FULL_ADDRESS_FROM_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->full_address_to), 1, FULL_ADDRESS_TO_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->subject), 1, SUBJECT_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->body_download_status), BODY_DOWNLOAD_STATUS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->file_path_plain), 0, FILE_PATH_PLAIN_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->file_path_html), 0, FILE_PATH_HTML_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->file_path_mime_entity), 0, FILE_PATH_HTML_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_time_t(hStmt, &(p_data_tbl->date_time), DATETIME_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_char(hStmt, &(p_data_tbl->flags_seen_field), FLAGS_SEEN_FIELD_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->DRM_status), DRM_STATUS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->priority), PRIORITY_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->save_status), SAVE_STATUS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->lock_status), LOCK_STATUS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->report_status), REPORT_STATUS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->preview_text), 1, PREVIEW_TEXT_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->meeting_request_status), MEETING_REQUEST_STATUS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->message_class), MESSAGE_CLASS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->digest_type), DIGEST_TYPE_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->smime_type), SMIME_TYPE_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->scheduled_sending_time), SCHEDULED_SENDING_TIME_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->remaining_resend_times), SCHEDULED_SENDING_TIME_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->tag_id), TAG_ID_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->eas_data_length), EAS_DATA_LENGTH_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_blob(hStmt, (void**)&(p_data_tbl->eas_data), EAS_DATA_IDX_IN_MAIL_TBL);

			if (type == RETRIEVE_ALL) {
				_get_stmt_field_data_int(hStmt, &(p_data_tbl->server_mail_status), SERVER_MAIL_STATUS_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mailbox_name), 0, SERVER_MAILBOX_NAME_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->full_address_reply), 1, FULL_ADDRESS_REPLY_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->full_address_cc), 1, FULL_ADDRESS_CC_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->full_address_bcc), 1, FULL_ADDRESS_BCC_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->full_address_return), 1, FULL_ADDRESS_RETURN_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->message_id), 0, MESSAGE_ID_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->email_address_sender), 1, EMAIL_ADDRESS_SENDER_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->email_address_recipient), 1, EMAIL_ADDRESS_RECIPIENT_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_int(hStmt, &(p_data_tbl->attachment_count), ATTACHMENT_COUNT_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->preview_text), 1, PREVIEW_TEXT_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->replied_time), REPLIED_TIME_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_int(hStmt, (int*)&(p_data_tbl->forwarded_time), FORWARDED_TIME_IDX_IN_MAIL_TBL);
				_get_stmt_field_data_string(hStmt, &(p_data_tbl->default_charset), 0, DEFAULT_CHARSET_IDX_IN_MAIL_TBL);
			}

			if (p_data_tbl->body_download_status) {
				struct stat buf;

				if (p_data_tbl->file_path_html) {
					if (stat(p_data_tbl->file_path_html, &buf) == -1)
						p_data_tbl->body_download_status = 0;
				} else if (p_data_tbl->file_path_plain) {
					if (stat(p_data_tbl->file_path_plain, &buf) == -1)
						p_data_tbl->body_download_status = 0;
				} else
					p_data_tbl->body_download_status = 0;
			}

			*((emstorage_mail_tbl_t**)data) = p_data_tbl;
			break;

		case RETRIEVE_SUMMARY:
			if (!(p_data_tbl = malloc(sizeof(emstorage_mail_tbl_t)))) {
				EM_DEBUG_EXCEPTION(" malloc failed...");

				error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			memset(p_data_tbl, 0x00, sizeof(emstorage_mail_tbl_t));

			_get_stmt_field_data_int(hStmt, &(p_data_tbl->mail_id), MAIL_ID_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->account_id), ACCOUNT_ID_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_int(hStmt, &(p_data_tbl->server_mail_status), SERVER_MAIL_STATUS_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mailbox_name), 0, SERVER_MAILBOX_NAME_IDX_IN_MAIL_TBL);
			_get_stmt_field_data_string(hStmt, &(p_data_tbl->server_mail_id), 0, SERVER_MAIL_ID_IDX_IN_MAIL_TBL);

			*((emstorage_mail_tbl_t**)data) = p_data_tbl;
			break;

		case RETRIEVE_ADDRESS:
			if (!(p_data_tbl = malloc(sizeof(emstorage_mail_tbl_t)))) {
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
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	ret = true;

FINISH_OFF:

	if (err_code != NULL)
		*err_code = error;

	if (ret == false && p_data_tbl)
		emstorage_free_mail(&p_data_tbl, 1, NULL);

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_mail_search_end(DB_STMT search_handle, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("search_handle[%d], transaction[%d], err_code[%p]", search_handle, transaction, err_code);

	int error = EMAIL_ERROR_NONE;
	int rc, ret = false;

	if (search_handle == 0) {
		EM_DEBUG_EXCEPTION(" search_handle[%d]", search_handle);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	DB_STMT hStmt = search_handle;

	rc = sqlite3_finalize(hStmt);
	if (rc != SQLITE_OK) {
		EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
		error = EMAIL_ERROR_DB_FAILURE;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_change_mail(char *multi_user_name, int mail_id, emstorage_mail_tbl_t *mail, int transaction, int *err_code)
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
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	char mailbox_id_param_string[10] = {0,};

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_tbl SET"
		"  mail_id = ?"
		", account_id = ?"
		", mailbox_id = ?"
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
		", file_path_mime_entity = ?"
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
		", scheduled_sending_time = ?"
		", remaining_resend_times = ?"
		", tag_id = ?"
		", replied_time = ?"
		", forwarded_time = ?"
		", default_charset = ?"
		", eas_data_length = ?"
		", eas_data = ?"
		" WHERE mail_id = %d AND account_id != 0 "
		, mail_id);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bind_stmt_field_data_int(hStmt, i++, mail->mail_id);
	_bind_stmt_field_data_int(hStmt, i++, mail->account_id);
	_bind_stmt_field_data_int(hStmt, i++, mail->mailbox_id);
	_bind_stmt_field_data_int(hStmt, i++, mail->mail_size);
	_bind_stmt_field_data_int(hStmt, i++, mail->server_mail_status);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->server_mailbox_name, 0, SERVER_MAILBOX_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->server_mail_id, 0, SERVER_MAIL_ID_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, i++, mail->reference_mail_id);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->full_address_from, 1, FROM_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->full_address_reply, 1, REPLY_TO_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->full_address_to, 1, TO_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->full_address_cc, 1, CC_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->full_address_bcc, 1, BCC_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->full_address_return, 1, RETURN_PATH_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->subject, 1, SUBJECT_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, i++, mail->body_download_status);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_html, 0, TEXT_2_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_mime_entity, 0, MIME_ENTITY_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, i++, mail->date_time);
	_bind_stmt_field_data_char(hStmt, i++, mail->flags_seen_field);
	_bind_stmt_field_data_char(hStmt, i++, mail->flags_deleted_field);
	_bind_stmt_field_data_char(hStmt, i++, mail->flags_flagged_field);
	_bind_stmt_field_data_char(hStmt, i++, mail->flags_answered_field);
	_bind_stmt_field_data_char(hStmt, i++, mail->flags_recent_field);
	_bind_stmt_field_data_char(hStmt, i++, mail->flags_draft_field);
	_bind_stmt_field_data_char(hStmt, i++, mail->flags_forwarded_field);
	_bind_stmt_field_data_int(hStmt, i++, mail->DRM_status);
	_bind_stmt_field_data_int(hStmt, i++, mail->priority);
	_bind_stmt_field_data_int(hStmt, i++, mail->save_status);
	_bind_stmt_field_data_int(hStmt, i++, mail->lock_status);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->message_id, 0, MESSAGE_ID_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, i++, mail->report_status);
	_bind_stmt_field_data_nstring(hStmt, i++, (char *)mail->preview_text, 1, PREVIEWBODY_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, i++, mail->smime_type);
	_bind_stmt_field_data_int(hStmt, i++, mail->scheduled_sending_time);
	_bind_stmt_field_data_int(hStmt, i++, mail->remaining_resend_times);
	_bind_stmt_field_data_int(hStmt, i++, mail->tag_id);
	_bind_stmt_field_data_int(hStmt, i++, mail->replied_time);
	_bind_stmt_field_data_int(hStmt, i++, mail->forwarded_time);
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail->default_charset, 0, TEXT_2_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, i++, mail->eas_data_length);
	_bind_stmt_field_data_blob(hStmt, i++, (void*)mail->eas_data, mail->eas_data_length);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_LOG(" no matched mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}


	if (error == EMAIL_ERROR_NONE && mail) {
		SNPRINTF(mailbox_id_param_string, 10, "%d", mail->mailbox_id);
		if (!emcore_notify_storage_event(NOTI_MAIL_UPDATE, mail->account_id, mail->mail_id, mailbox_id_param_string, 0))
			EM_DEBUG_EXCEPTION("emcore_notify_storage_eventfailed [NOTI_MAIL_UPDATE]");
	}

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
INTERNAL_FUNC int emstorage_clean_save_status(char *multi_user_name, int save_status, int  *err_code)
{
	EM_DEBUG_FUNC_BEGIN("save_status[%d], err_code[%p]", save_status, err_code);

	EM_IF_NULL_RETURN_VALUE(err_code, false);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int rc = 0;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET save_status = %d WHERE save_status = %d", save_status, EMAIL_MAIL_STATUS_SENDING);

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

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

INTERNAL_FUNC int emstorage_set_field_of_mails_with_integer_value(char *multi_user_name, int account_id, int mail_ids[], int mail_ids_count, char *field_name, int value, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("account_id [%d], mail_ids[%p], mail_ids_count[%d], field_name[%s], value[%d], transaction[%d], err_code[%p]", account_id, mail_ids, mail_ids_count, field_name, value, transaction, err_code);
	int i = 0;
	int error = EMAIL_ERROR_NONE;
	int ret = false;
	int query_size = 0;
	int cur_mail_id_string = 0;
	int mail_id_string_buffer_length = 0;
	int parameter_string_length = 0;
	char  *sql_query_string = NULL;
	char *mail_id_string_buffer = NULL;
	char *parameter_string = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	email_mail_attribute_type target_mail_attribute_type = 0;

	if (!mail_ids  || !field_name || account_id == 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	if ((error = emcore_get_attribute_type_by_mail_field_name(field_name, &target_mail_attribute_type)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorageemcore_get_attribute_type_by_mail_field_name failed [%d]", error);
		if (err_code != NULL)
			*err_code = error;
		return false;
	}

	/* Generating mail id list string */
	mail_id_string_buffer_length = MAIL_ID_STRING_LENGTH * mail_ids_count;

	mail_id_string_buffer = em_malloc(mail_id_string_buffer_length);

	if (!mail_id_string_buffer) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_OUT_OF_MEMORY;
		return false;
	}

	for (i = 0; i < mail_ids_count; i++)
		cur_mail_id_string += SNPRINTF_OFFSET(mail_id_string_buffer, cur_mail_id_string, mail_id_string_buffer_length, "%d,", mail_ids[i]);

	if (EM_SAFE_STRLEN(mail_id_string_buffer) > 1)
		mail_id_string_buffer[EM_SAFE_STRLEN(mail_id_string_buffer) - 1] = NULL_CHAR;

	/* Generating notification parameter string */
	parameter_string_length = mail_id_string_buffer_length + EM_SAFE_STRLEN(field_name) + 2;
	parameter_string = em_malloc(parameter_string_length);

	if (!parameter_string) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_OUT_OF_MEMORY;
		EM_SAFE_FREE(mail_id_string_buffer);
		return false;
	}

	SNPRINTF(parameter_string, parameter_string_length, "%s%c%s", field_name, 0x01, mail_id_string_buffer);
	query_size = EM_SAFE_STRLEN(mail_id_string_buffer) + EM_SAFE_STRLEN(field_name) + 250;

	sql_query_string = em_malloc(query_size);
	if (sql_query_string == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	/* Write query string */
	SNPRINTF(sql_query_string, query_size, "UPDATE mail_tbl SET %s = %d WHERE mail_id in (%s) AND account_id = %d", field_name, value, mail_id_string_buffer, account_id);

	EM_DEBUG_LOG_DEV("sql_query_string [%s]", sql_query_string);

	/* Execute query */
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	if (sqlite3_changes(local_db_handle) == 0)
		EM_DEBUG_LOG("no mail matched...");

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (error == EMAIL_ERROR_NONE && parameter_string) {
		if (!emcore_notify_storage_event(NOTI_MAIL_FIELD_UPDATE, account_id, target_mail_attribute_type, parameter_string, value))
			EM_DEBUG_EXCEPTION_SEC("emcore_notify_storage_eventfailed : NOTI_MAIL_FIELD_UPDATE [%s,%d]", field_name, value);
	}

	EM_SAFE_FREE(mail_id_string_buffer);
	EM_SAFE_FREE(parameter_string);
	EM_SAFE_FREE(sql_query_string);


	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("error [%d]", error);
	return ret;
}

#ifdef __FEATURE_BODY_SEARCH__
INTERNAL_FUNC int emstorage_change_mail_text_field(char *multi_user_name, int mail_id, emstorage_mail_text_tbl_t* mail_text, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], mail_text[%p], transaction[%d], err_code[%p]", mail_id, mail_text, transaction, err_code);

	if (mail_id <= 0 || !mail_text) {
		EM_DEBUG_EXCEPTION(" mail_id[%d], mail_text[%p]", mail_id, mail_text);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	int i = 0;
	int rc = 0;

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_text_tbl SET"
		" body_text = ?"
		" WHERE mail_id = %d AND account_id != 0"
		, mail_id);
	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	i = 0;
	_bind_stmt_field_data_string(hStmt, i++, (char *)mail_text->body_text, 0, -1);

	if (hStmt != NULL) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; }, ("sqlite3_step fail:%d", rc));
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; }, ("sqlite3_step fail:%d", rc));

		rc = sqlite3_changes(local_db_handle);
		if (rc == 0) {
			EM_DEBUG_LOG(" no matched mail found...");
			error = EMAIL_ERROR_MAIL_NOT_FOUND;
			goto FINISH_OFF;
		}
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
		hStmt = NULL;
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
#endif

INTERNAL_FUNC int emstorage_change_mail_field(char *multi_user_name, int mail_id, email_mail_change_type_t type, emstorage_mail_tbl_t *mail, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], type[%d], mail[%p], transaction[%d], err_code[%p]", mail_id, type, mail, transaction, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int move_flag = 0;
	int rc = 0;
	int i = 0;
	int mailbox_id = 0;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char mailbox_id_param_string[10] = {0,};
	sqlite3 *local_db_handle = NULL;

	if (mail_id <= 0 || !mail) {
		EM_DEBUG_EXCEPTION(" mail_id[%d], type[%d], mail[%p]", mail_id, type, mail);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	switch (type) {
		case APPEND_BODY:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				"  body_download_status = ?"
				", file_path_plain = ?"
				", file_path_html = ?"
				", file_path_mime_entity = ?"
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


			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			 i = 0;

			_bind_stmt_field_data_int(hStmt, i++, mail->body_download_status);
			_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_html, 0, TEXT_2_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_mime_entity, 0, MIME_ENTITY_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_int(hStmt, i++, mail->flags_seen_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->flags_deleted_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->flags_flagged_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->flags_answered_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->flags_recent_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->flags_draft_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->flags_forwarded_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->DRM_status);
			_bind_stmt_field_data_int(hStmt, i++, mail->attachment_count);
			_bind_stmt_field_data_nstring(hStmt, i++, (char *)mail->preview_text, 0, PREVIEWBODY_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_int(hStmt, i++, mail->meeting_request_status);
			_bind_stmt_field_data_int(hStmt, i++, mail->message_class);
			_bind_stmt_field_data_int(hStmt, i++, mail->digest_type);
			_bind_stmt_field_data_int(hStmt, i++, mail->smime_type);
			break;

		case UPDATE_MAILBOX: {
				int err;
				emstorage_mailbox_tbl_t *mailbox_tbl;

				if ((err = emstorage_get_mailbox_by_id(multi_user_name, mail->mailbox_id, &mailbox_tbl)) != EMAIL_ERROR_NONE) {
					EM_DEBUG_EXCEPTION(" emstorage_get_mailbox_by_id failed [%d]", err);
					goto FINISH_OFF;
				}

				SNPRINTF(sql_query_string, sizeof(sql_query_string),
					"UPDATE mail_tbl SET"
					" mailbox_id = '%d'"
					",mailbox_type = '%d'"
					" WHERE mail_id = %d AND account_id != 0"
					, mailbox_tbl->mailbox_id
					, mailbox_tbl->mailbox_type
					, mail_id);
					move_flag = 1;


				EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
				EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
					("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

				i = 0;
				_bind_stmt_field_data_string(hStmt, i++, (char *)mailbox_tbl->mailbox_name, 0, MAILBOX_NAME_LEN_IN_MAIL_BOX_TBL);

				emstorage_free_mailbox(&mailbox_tbl, 1, NULL); /*prevent 26251*/

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
			EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);


			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
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
			EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);


			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			break;

		case UPDATE_STICKY_EXTRA_FLAG:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				"  lock_status = %d"
				"  WHERE mail_id = %d AND account_id != 0"
				, mail->lock_status
				, mail_id);
			EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);


			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
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
				", scheduled_sending_time = ?"
				", remaining_resend_times = ?"
				", tag_id = ?"
				", replied_time = ?"
				", forwarded_time = ?"
				", default_charset = ?"
				", eas_data_length = ?"
				", eas_data = ?"
				" WHERE mail_id = %d AND account_id != 0"
				, mail_id);


			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_LOG_DEV(" before sqlite3_prepare hStmt = %p", hStmt);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
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
			_bind_stmt_field_data_char(hStmt, i++, mail->flags_seen_field);
			_bind_stmt_field_data_char(hStmt, i++, mail->flags_deleted_field);
			_bind_stmt_field_data_char(hStmt, i++, mail->flags_flagged_field);
			_bind_stmt_field_data_char(hStmt, i++, mail->flags_answered_field);
			_bind_stmt_field_data_char(hStmt, i++, mail->flags_recent_field);
			_bind_stmt_field_data_char(hStmt, i++, mail->flags_draft_field);
			_bind_stmt_field_data_char(hStmt, i++, mail->flags_forwarded_field);
			_bind_stmt_field_data_int(hStmt, i++, mail->priority);
			_bind_stmt_field_data_int(hStmt, i++, mail->save_status);
			_bind_stmt_field_data_int(hStmt, i++, mail->lock_status);
			_bind_stmt_field_data_int(hStmt, i++, mail->report_status);
			_bind_stmt_field_data_int(hStmt, i++, mail->DRM_status);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->file_path_html, 0, TEXT_2_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->file_path_mime_entity, 0, MIME_ENTITY_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_int(hStmt, i++, mail->mail_size);
			_bind_stmt_field_data_nstring(hStmt, i++, (char*)mail->preview_text, 1, PREVIEWBODY_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_int(hStmt, i++, mail->body_download_status);
			_bind_stmt_field_data_int(hStmt, i++, mail->attachment_count);
			_bind_stmt_field_data_int(hStmt, i++, mail->inline_content_count);
			_bind_stmt_field_data_int(hStmt, i++, mail->meeting_request_status);
			_bind_stmt_field_data_int(hStmt, i++, mail->message_class);
			_bind_stmt_field_data_int(hStmt, i++, mail->digest_type);
			_bind_stmt_field_data_int(hStmt, i++, mail->smime_type);
			_bind_stmt_field_data_int(hStmt, i++, mail->scheduled_sending_time);
			_bind_stmt_field_data_int(hStmt, i++, mail->remaining_resend_times);
			_bind_stmt_field_data_int(hStmt, i++, mail->tag_id);
			_bind_stmt_field_data_int(hStmt, i++, mail->replied_time);
			_bind_stmt_field_data_int(hStmt, i++, mail->forwarded_time);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->default_charset, 0, TEXT_2_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_int(hStmt, i++, mail->eas_data_length);
			_bind_stmt_field_data_blob(hStmt, i++, (void*)mail->eas_data, mail->eas_data_length);
			break;

		case UPDATE_DATETIME: {
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				" date_time = '%ld'"
				" WHERE mail_id = %d AND account_id != 0"
				, mail->date_time
				, mail_id);

			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
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

			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
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

			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
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

				EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
				EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
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
			", file_path_mime_entity = ?"
			", attachment_count = ?"
			", inline_content_count = ?"
			", preview_text = ?"
			", digest_type = ?"
                        ", smime_type = ?"
			" WHERE mail_id = %d"
			, mail_id);


			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			 i = 0;

			_bind_stmt_field_data_int(hStmt, i++, mail->body_download_status);
			_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_html,  0, TEXT_2_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char *)mail->file_path_mime_entity, 0, MIME_ENTITY_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_int(hStmt, i++, mail->attachment_count);
			_bind_stmt_field_data_int(hStmt, i++, mail->inline_content_count);
			_bind_stmt_field_data_nstring(hStmt, i++, (char *)mail->preview_text,    0, PREVIEWBODY_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_int(hStmt, i++, mail->digest_type);
			_bind_stmt_field_data_int(hStmt, i++, mail->smime_type);

			break;

#endif
		case UPDATE_FILE_PATH:
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_tbl SET"
				", file_path_plain = ?"
				", file_path_html = ?"
				", file_path_mime_entity = ?"
				" WHERE mail_id = %d"
				, mail_id);


			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_LOG(" before sqlite3_prepare hStmt = %p", hStmt);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
				("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
			i = 0;
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->file_path_html, 0, TEXT_2_LEN_IN_MAIL_TBL);
			_bind_stmt_field_data_string(hStmt, i++, (char*)mail->file_path_mime_entity, 0, MIME_ENTITY_LEN_IN_MAIL_TBL);
			break;

		default:
			EM_DEBUG_LOG(" type[%d]", type);

			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}

	if (hStmt != NULL) {

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
		rc = sqlite3_changes(local_db_handle);
		if (rc == 0) {
			EM_DEBUG_LOG(" no matched mail found...");
			error = EMAIL_ERROR_MAIL_NOT_FOUND;
			goto FINISH_OFF;
		}
	}

	if (mail->account_id == 0) {
		emstorage_mail_tbl_t* mail_for_account_tbl = NULL;
		if (!emstorage_get_mail_field_by_id(multi_user_name, mail_id, RETRIEVE_ACCOUNT, &mail_for_account_tbl, true, &error) || !mail_for_account_tbl) {
			EM_DEBUG_EXCEPTION("emstorage_get_mail_field_by_id error [%d]", error);
			goto FINISH_OFF;
		}
		mail->account_id = mail_for_account_tbl->account_id;
		if (mail_for_account_tbl)
			emstorage_free_mail(&mail_for_account_tbl, 1, NULL);
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
		hStmt = NULL;
	}

	if (error == EMAIL_ERROR_NONE &&  move_flag != 1 && transaction) {
		if (!emstorage_get_mailbox_id_by_mailbox_type(multi_user_name, mail->account_id, EMAIL_MAILBOX_TYPE_SENTBOX, &mailbox_id, false, &error))
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_id_by_mailbox_type error [%d]", error);

		if (mail->mailbox_id == mailbox_id) {
			SNPRINTF(mailbox_id_param_string, 10, "%d", mail->mailbox_id);
			if (!emcore_notify_storage_event(NOTI_MAIL_UPDATE, mail->account_id, mail_id, mailbox_id_param_string, type))
				EM_DEBUG_EXCEPTION("emcore_notify_storage_eventerror [ NOTI_MAIL_UPDATE ] >>>> ");
		} else {
			/* h.gahlaut@samsung.com: Jan 10, 2011 Publishing noti to refresh outbox when email sending status changes */
			if (!emcore_notify_storage_event(NOTI_MAIL_UPDATE, mail->account_id, mail_id, NULL, type))
				EM_DEBUG_EXCEPTION(" emcore_notify_storage_eventerror [ NOTI_MAIL_UPDATE ]");
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_increase_mail_id(char *multi_user_name, int *mail_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%p], transaction[%d], err_code[%p]", mail_id, transaction, err_code);

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	int latest_mail_id = 0;
	sqlite3 *local_db_handle = NULL;
	char *sql = "SELECT MAX(mail_id) FROM mail_tbl;";
	char **result = NULL;

#ifdef __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__
	_timedlock_shm_mutex(mapped_for_generating_mail_id, 2);
#endif /* __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__ */

 	ret = vconf_get_int(VCONF_KEY_LATEST_MAIL_ID, &latest_mail_id);
	if (ret < 0 || latest_mail_id == 0) {
		EM_DEBUG_LOG("vconf_get_int() failed [%d] or latest_mail_id is zero", ret);

        local_db_handle = emstorage_get_db_connection(multi_user_name);

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
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
	_unlockshm_mutex(mapped_for_generating_mail_id);
#endif /* __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__ */

	ret = true;

FINISH_OFF:

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_add_mail(char *multi_user_name, emstorage_mail_tbl_t *mail_tbl_data, int get_id, int transaction, int *err_code)
{
	EM_PROFILE_BEGIN(profile_emstorage_add_mail);
	EM_DEBUG_FUNC_BEGIN("mail_tbl_data[%p], get_id[%d], transaction[%d], err_code[%p]", mail_tbl_data, get_id, transaction, err_code);

	if (!mail_tbl_data) {
		EM_DEBUG_EXCEPTION("mail_tbl_data[%p], get_id[%d]", mail_tbl_data, get_id);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	DB_STMT hStmt = NULL;

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	if (get_id) {
		/*  increase unique id */
		char *sql = "SELECT max(rowid) FROM mail_tbl;";
		char **result;

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
/*		EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
			("SQL[%s] sqlite3_get_table fail[%d] [%s]", sql, rc, sqlite3_errmsg(local_db_handle))); */
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("SQL[%s] sqlite3_get_table fail[%d] [%s]", sql, rc, sqlite3_errmsg(local_db_handle));
			error = EMAIL_ERROR_DB_FAILURE;
			sqlite3_free_table(result);
			goto FINISH_OFF;
		}

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
		"(?" /*  mail_id */
		", ?" /*  account_id */
		", ?" /*  mailbox_id */
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
		", ?" /*  scheduled_sending_time */
		", ?" /*  remaining_resend_times */
		", ?" /*  tag_id */

		", ?" /*  replied_time */
		", ?" /*  forwarded_time */
		", ?" /*  default charset */
		", ?" /*  eas_data_length */
		", ?" /*  eas_data */
		", ?" /*  user_name */
		")");

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle,
                                                   sql_query_string,
                                                   EM_SAFE_STRLEN(sql_query_string),
                                                   &hStmt,
                                                   NULL),
                                    rc);
	if (rc != SQLITE_OK) {
		EM_DEBUG_EXCEPTION("sqlite3_prepare error [%d] [%s] SQL(%s) ",
                             rc, sql_query_string, sqlite3_errmsg(local_db_handle));
		error = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	_bind_stmt_field_data_int(hStmt, MAIL_ID_IDX_IN_MAIL_TBL, mail_tbl_data->mail_id);
	_bind_stmt_field_data_int(hStmt, ACCOUNT_ID_IDX_IN_MAIL_TBL, mail_tbl_data->account_id);
	_bind_stmt_field_data_int(hStmt, MAILBOX_ID_IDX_IN_MAIL_TBL, mail_tbl_data->mailbox_id);
	_bind_stmt_field_data_int(hStmt, MAILBOX_TYPE_IDX_IN_MAIL_TBL, mail_tbl_data->mailbox_type);
	_bind_stmt_field_data_string(hStmt, SUBJECT_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->subject, 1, SUBJECT_LEN_IN_MAIL_TBL);

	_bind_stmt_field_data_int(hStmt, DATETIME_IDX_IN_MAIL_TBL, mail_tbl_data->date_time);
	_bind_stmt_field_data_int(hStmt, SERVER_MAIL_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->server_mail_status);
	_bind_stmt_field_data_string(hStmt, SERVER_MAILBOX_NAME_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->server_mailbox_name, 0, SERVER_MAILBOX_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, SERVER_MAIL_ID_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->server_mail_id, 0, SERVER_MAIL_ID_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, MESSAGE_ID_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->message_id, 0, MESSAGE_ID_LEN_IN_MAIL_TBL);

	_bind_stmt_field_data_int(hStmt, REFERENCE_ID_IDX_IN_MAIL_TBL, mail_tbl_data->reference_mail_id);
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
	_bind_stmt_field_data_int(hStmt, BODY_DOWNLOAD_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->body_download_status);
	_bind_stmt_field_data_string(hStmt, FILE_PATH_PLAIN_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->file_path_plain, 0, TEXT_1_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FILE_PATH_HTML_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->file_path_html, 0, TEXT_2_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, FILE_PATH_MIME_ENTITY_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->file_path_mime_entity, 0, MIME_ENTITY_LEN_IN_MAIL_TBL);

	_bind_stmt_field_data_int(hStmt, MAIL_SIZE_IDX_IN_MAIL_TBL, mail_tbl_data->mail_size);
	_bind_stmt_field_data_int(hStmt, FLAGS_SEEN_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_seen_field);
	_bind_stmt_field_data_int(hStmt, FLAGS_DELETED_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_deleted_field);
	_bind_stmt_field_data_int(hStmt, FLAGS_FLAGGED_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_flagged_field);
	_bind_stmt_field_data_int(hStmt, FLAGS_ANSWERED_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_answered_field);

	_bind_stmt_field_data_int(hStmt, FLAGS_RECENT_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_recent_field);
	_bind_stmt_field_data_int(hStmt, FLAGS_DRAFT_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_draft_field);
	_bind_stmt_field_data_int(hStmt, FLAGS_FORWARDED_FIELD_IDX_IN_MAIL_TBL, mail_tbl_data->flags_forwarded_field);
	_bind_stmt_field_data_int(hStmt, DRM_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->DRM_status);
	_bind_stmt_field_data_int(hStmt, PRIORITY_IDX_IN_MAIL_TBL, mail_tbl_data->priority);

	_bind_stmt_field_data_int(hStmt, SAVE_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->save_status);
	_bind_stmt_field_data_int(hStmt, LOCK_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->lock_status);
	_bind_stmt_field_data_int(hStmt, REPORT_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->report_status);
	_bind_stmt_field_data_int(hStmt, ATTACHMENT_COUNT_IDX_IN_MAIL_TBL, mail_tbl_data->attachment_count);
	_bind_stmt_field_data_int(hStmt, INLINE_CONTENT_COUNT_IDX_IN_MAIL_TBL, mail_tbl_data->inline_content_count);

	_bind_stmt_field_data_int(hStmt, THREAD_ID_IDX_IN_MAIL_TBL, mail_tbl_data->thread_id);
	_bind_stmt_field_data_int(hStmt, THREAD_ITEM_COUNT_IDX_IN_MAIL_TBL, mail_tbl_data->thread_item_count);
	_bind_stmt_field_data_nstring(hStmt, PREVIEW_TEXT_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->preview_text, 1, PREVIEWBODY_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, MEETING_REQUEST_STATUS_IDX_IN_MAIL_TBL, mail_tbl_data->meeting_request_status);
	_bind_stmt_field_data_int(hStmt, MESSAGE_CLASS_IDX_IN_MAIL_TBL, mail_tbl_data->message_class);

	_bind_stmt_field_data_int(hStmt, DIGEST_TYPE_IDX_IN_MAIL_TBL, mail_tbl_data->digest_type);
	_bind_stmt_field_data_int(hStmt, SMIME_TYPE_IDX_IN_MAIL_TBL, mail_tbl_data->smime_type);
	_bind_stmt_field_data_int(hStmt, SCHEDULED_SENDING_TIME_IDX_IN_MAIL_TBL, mail_tbl_data->scheduled_sending_time);
	_bind_stmt_field_data_int(hStmt, REMAINING_RESEND_TIMES_IDX_IN_MAIL_TBL, mail_tbl_data->remaining_resend_times);
	_bind_stmt_field_data_int(hStmt, TAG_ID_IDX_IN_MAIL_TBL, mail_tbl_data->tag_id);

	_bind_stmt_field_data_int(hStmt, REPLIED_TIME_IDX_IN_MAIL_TBL, mail_tbl_data->replied_time);
	_bind_stmt_field_data_int(hStmt, FORWARDED_TIME_IDX_IN_MAIL_TBL, mail_tbl_data->forwarded_time);
	_bind_stmt_field_data_string(hStmt, DEFAULT_CHARSET_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->default_charset, 0, TEXT_2_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, EAS_DATA_LENGTH_IDX_IN_MAIL_TBL, mail_tbl_data->eas_data_length);
	_bind_stmt_field_data_blob(hStmt, EAS_DATA_IDX_IN_MAIL_TBL, (void*)mail_tbl_data->eas_data, mail_tbl_data->eas_data_length);
	_bind_stmt_field_data_string(hStmt, USER_NAME_IDX_IN_MAIL_TBL, (char*)mail_tbl_data->user_name, 0, TEXT_2_LEN_IN_MAIL_TBL);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	if (rc == SQLITE_FULL) {
		EM_DEBUG_EXCEPTION("sqlite3_step error [%d]", rc);
		error = EMAIL_ERROR_MAIL_MEMORY_FULL;
		goto FINISH_OFF;
	}
	if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
		EM_DEBUG_EXCEPTION("sqlite3_step error [%d]", rc);
		error = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}
	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(profile_emstorage_add_mail);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_move_multiple_mails_on_db(char *multi_user_name, int input_source_account_id, int input_mailbox_id, int mail_ids[], int number_of_mails, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_source_account_id [%d], input_mailbox_id [%d], mail_ids[%p], number_of_mails [%d], transaction[%d], err_code[%p]", input_source_account_id, input_mailbox_id, mail_ids, number_of_mails, transaction, err_code);

	int ret = false, i, cur_conditional_clause = 0;
	int error = EMAIL_ERROR_NONE;
	int target_account_id;
	int conditional_clause_len = 0;
	char *sql_query_string = NULL, *conditional_clause = NULL;
	emstorage_mailbox_tbl_t *result_mailbox = NULL;
	email_mailbox_type_e target_mailbox_type = EMAIL_MAILBOX_TYPE_USER_DEFINED;
	char* target_mailbox_name = NULL;

	if (!mail_ids || input_mailbox_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	if ((error = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &result_mailbox)) != EMAIL_ERROR_NONE || !result_mailbox) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed. [%d]", error);
		if (err_code != NULL)
			*err_code = error;
		return false;
	}

	if (result_mailbox->mailbox_name) {
		if (strstr(result_mailbox->mailbox_name, "'")) {
			target_mailbox_name = em_replace_all_string(result_mailbox->mailbox_name, "'", "''");
		} else {
			target_mailbox_name = strdup(result_mailbox->mailbox_name);
		}
	}

	target_mailbox_type = result_mailbox->mailbox_type;
	target_account_id   = result_mailbox->account_id;
	emstorage_free_mailbox(&result_mailbox, 1, NULL);

	conditional_clause_len =  (sizeof(char) * 8 * number_of_mails) + 512;
	conditional_clause = em_malloc(conditional_clause_len);
	if (conditional_clause == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	cur_conditional_clause = SNPRINTF(conditional_clause, conditional_clause_len, "WHERE mail_id in (");

	for (i = 0; i < number_of_mails; i++)
		cur_conditional_clause += SNPRINTF_OFFSET(conditional_clause, cur_conditional_clause, conditional_clause_len, "%d,", mail_ids[i]);

	/* prevent 34415 */
	char *last_comma = rindex(conditional_clause, ',');
	if (last_comma) *last_comma = ')'; /* replace , with) */

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	/* Updating a mail_tbl */

	sql_query_string = em_malloc(conditional_clause_len);
	if (sql_query_string == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	SNPRINTF(sql_query_string, conditional_clause_len, "UPDATE mail_tbl SET mailbox_type = %d, mailbox_id = %d, account_id = %d %s", target_mailbox_type, input_mailbox_id, target_account_id, conditional_clause);
	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);

	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	/* Updating a mail_attachment_tbl */
	memset(sql_query_string, 0x00, conditional_clause_len);
	SNPRINTF(sql_query_string, conditional_clause_len, "UPDATE mail_attachment_tbl SET mailbox_id = '%d', account_id = %d %s", input_mailbox_id, target_account_id, conditional_clause);
	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	/* Updating a mail_meeting_tbl */
	memset(sql_query_string, 0x00, conditional_clause_len);
	SNPRINTF(sql_query_string, conditional_clause_len, "UPDATE mail_meeting_tbl SET mailbox_id = %d, account_id = %d %s", input_mailbox_id, target_account_id, conditional_clause);
	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

#ifdef __FEATURE_BODY_SEARCH__
	/* Updating mail_text_tbl */
	memset(sql_query_string, 0x00, conditional_clause_len);
	SNPRINTF(sql_query_string, conditional_clause_len, "UPDATE mail_text_tbl SET mailbox_id = %d, account_id = %d %s", input_mailbox_id, target_account_id, conditional_clause);
	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}
#endif

	/* Updating a mail_read_mail_uid_tbl */
	memset(conditional_clause, 0x00, conditional_clause_len);
	cur_conditional_clause = SNPRINTF(conditional_clause, conditional_clause_len, "WHERE local_uid in (");

	for (i = 0; i < number_of_mails; i++)
		cur_conditional_clause += SNPRINTF_OFFSET(conditional_clause, cur_conditional_clause, conditional_clause_len, "%d,", mail_ids[i]);

	/* prevent 34415 */
	last_comma = rindex(conditional_clause, ',');
	if (last_comma) *last_comma = ')'; /* replace , with) */

	memset(sql_query_string, 0x00, conditional_clause_len);
	SNPRINTF(sql_query_string, conditional_clause_len, "UPDATE mail_read_mail_uid_tbl SET mailbox_name = '%s', mailbox_id = %d, account_id = %d %s", target_mailbox_name, input_mailbox_id, target_account_id, conditional_clause);
	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	EM_SAFE_FREE(target_mailbox_name);
	EM_SAFE_FREE(conditional_clause);
	EM_SAFE_FREE(sql_query_string);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_mail(char *multi_user_name, int mail_id, int from_server, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], transaction[%d], err_code[%p]", mail_id, transaction, err_code);

	if (!mail_id) {
		EM_DEBUG_EXCEPTION("mail_id[%d]", mail_id);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_tbl WHERE mail_id = %d ", mail_id);
	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_multiple_mails(char *multi_user_name, int mail_ids[], int number_of_mails, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], number_of_mails [%d], transaction[%d], err_code[%p]", mail_ids, number_of_mails, transaction, err_code);

	int ret = false, i, cur_sql_query_string = 0;
	int error = EMAIL_ERROR_NONE;
	int query_size = 0;
	char *sql_query_string = NULL;

	if (!mail_ids) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	query_size = (sizeof(char) * 8 * number_of_mails) + 512;
	sql_query_string =  em_malloc(query_size);
	if (sql_query_string == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	cur_sql_query_string = SNPRINTF(sql_query_string, query_size, "DELETE FROM mail_tbl WHERE mail_id in (");

	for (i = 0; i < number_of_mails; i++)
		cur_sql_query_string += SNPRINTF_OFFSET(sql_query_string, cur_sql_query_string, query_size, "%d,", mail_ids[i]);

	/* prevent 34414 */
	char *last_comma = rindex(sql_query_string, ',');
	if (last_comma != NULL) *last_comma = ')'; /* replace , with) */

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

#ifdef __FEATURE_BODY_SEARCH__
	/* delete mail_text from mail_text_tbl */
	cur_sql_query_string = SNPRINTF(sql_query_string, query_size, "DELETE FROM mail_text_tbl WHERE mail_id in (");

	for (i = 0; i < number_of_mails; i++)
		cur_sql_query_string += SNPRINTF_OFFSET(sql_query_string, cur_sql_query_string, query_size, "%d,", mail_ids[i]);

	last_comma = rindex(sql_query_string, ',');
	*last_comma = ')'; /* replace , with) */

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}
#endif

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	EM_SAFE_FREE(sql_query_string);
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_mail_by_account(char *multi_user_name, int account_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], transaction[%d], err_code[%p]", account_id, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID) {
		EM_DEBUG_EXCEPTION("account_id[%d]", account_id);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_tbl WHERE account_id = %d", account_id);
	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_LOG(" no mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
	}

	/* Delete all mails  mail_read_mail_uid_tbl table based on account id */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_read_mail_uid_tbl WHERE account_id = %d", account_id);
	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_LOG("No mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
	}

#ifdef __FEATURE_BODY_SEARCH__
	/* Delete all mail_text in mail_text_tbl table based on account id */
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_text_tbl WHERE account_id = %d", account_id);
	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_LOG("No mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
	}
#endif

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (error == EMAIL_ERROR_NONE) {
		if (!emcore_notify_storage_event(NOTI_MAIL_DELETE_WITH_ACCOUNT, account_id, 0 , NULL, 0))
			EM_DEBUG_EXCEPTION("emcore_notify_storage_eventFailed [ NOTI_MAIL_DELETE_ALL ]");
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_mail_by_mailbox(char *multi_user_name, emstorage_mailbox_tbl_t *mailbox, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], transaction[%d], err_code[%p]", mailbox, transaction, err_code);

	if (mailbox == NULL) {
		EM_DEBUG_EXCEPTION("mailbox [%p]", mailbox);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	if (strcmp(mailbox->mailbox_name, EMAIL_SEARCH_RESULT_MAILBOX_NAME) == 0) {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_tbl WHERE account_id = %d AND mailbox_type = %d", mailbox->account_id, mailbox->mailbox_type);
		EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
		error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
		}

		/* Delete Mails from mail_read_mail_uid_tbl */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_read_mail_uid_tbl WHERE account_id = %d AND mailbox_name = '%s'", mailbox->account_id, mailbox->mailbox_name);
		EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
		error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
		}
	} else {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_tbl WHERE account_id = %d AND mailbox_id = %d", mailbox->account_id, mailbox->mailbox_id);
		EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
		error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
		}

		/* Delete Mails from mail_read_mail_uid_tbl */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_read_mail_uid_tbl WHERE account_id = %d AND mailbox_id = %d", mailbox->account_id, mailbox->mailbox_id);
		EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
		error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
		}

#ifdef __FEATURE_BODY_SEARCH__
		/* Delete Mails from mail_text_tbl */
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_text_tbl WHERE account_id = %d AND mailbox_id = %d", mailbox->account_id, mailbox->mailbox_id);
		EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
		error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
		}
#endif
	}
	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (error == EMAIL_ERROR_NONE) {
		if (!emcore_notify_storage_event(NOTI_MAIL_DELETE_ALL, mailbox->account_id, mailbox->mailbox_id , mailbox->mailbox_name, 0))
			EM_DEBUG_EXCEPTION(" emcore_notify_storage_eventFailed [ NOTI_MAIL_DELETE_ALL ] >>>> ");
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_free_mail(emstorage_mail_tbl_t** mail_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_list[%p], count[%d], err_code[%p]", mail_list, count, err_code);

	if (count > 0) {
		if ((mail_list == NULL) || (*mail_list == NULL)) {
			EM_DEBUG_EXCEPTION("mail_ilst[%p], count[%d]", mail_list, count);

			if (err_code)
				*err_code = EMAIL_ERROR_INVALID_PARAM;
			return false;
		}

		emstorage_mail_tbl_t* p = *mail_list;
		int i = 0;

		for (; i < count; i++, p++) {
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
			EM_SAFE_FREE(p->default_charset);
			EM_SAFE_FREE(p->pgp_password);
			EM_SAFE_FREE(p->eas_data);
			EM_SAFE_FREE(p->user_name);
		}
		EM_SAFE_FREE(*mail_list);
	}

	if (err_code != NULL)
		*err_code = EMAIL_ERROR_NONE;

	EM_DEBUG_FUNC_END();
	return true;
}

#ifdef __FEATURE_BODY_SEARCH__
INTERNAL_FUNC void emstorage_free_mail_text(emstorage_mail_text_tbl_t** mail_text_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_text_list[%p], count[%d], err_code[%p]", mail_text_list, count, err_code);

	if (count > 0) {
		if ((mail_text_list == NULL) || (*mail_text_list == NULL)) {
			EM_DEBUG_LOG("Nothing to free: mail_text_list[%p]", mail_text_list);
			return;
		}

		emstorage_mail_text_tbl_t *p = *mail_text_list;
		int i = 0;

		for (; i < count; i++, p++) {
			EM_SAFE_FREE(p->body_text);
		}
		EM_SAFE_FREE(*mail_text_list);
	}

	EM_DEBUG_FUNC_END();
}
#endif

INTERNAL_FUNC int emstorage_get_attachment_count(char *multi_user_name, int mail_id, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], count[%p], transaction[%d], err_code[%p]", mail_id, count, transaction, err_code);

	if (mail_id <= 0 || !count) {
		EM_DEBUG_EXCEPTION("mail_id[%d], count[%p]", mail_id, count);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_attachment_tbl WHERE mail_id = %d", mail_id);

	char **result;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_attachment_list(char *multi_user_name, int input_mail_id, int input_transaction, emstorage_attachment_tbl_t** output_attachment_list, int *output_attachment_count)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], input_transaction[%d], output_attachment_list[%p], output_attachment_count[%p]", input_mail_id, input_transaction, output_attachment_list, output_attachment_count);

	if (input_mail_id <= 0 || !output_attachment_list || !output_attachment_count) {
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
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(input_transaction);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT COUNT(*) FROM mail_attachment_tbl WHERE mail_id = %d", input_mail_id);
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*output_attachment_count = atoi(result[1]);
	sqlite3_free_table(result);

	if (*output_attachment_count == 0) {
		error = EMAIL_ERROR_NONE;
		goto FINISH_OFF;
	}

	p_data_tbl = (emstorage_attachment_tbl_t*)em_malloc(sizeof(emstorage_attachment_tbl_t) * (*output_attachment_count));

	if (!p_data_tbl) {
		EM_DEBUG_EXCEPTION("em_mallocfailed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_attachment_tbl WHERE mail_id = %d ORDER BY attachment_id", input_mail_id);
	EM_DEBUG_LOG_SEC("sql_query_string [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; }, ("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },	("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION("no matched attachment found...");
		error = EMAIL_ERROR_ATTACHMENT_NOT_FOUND;
		goto FINISH_OFF;
	}
	for (i = 0; i < *output_attachment_count; i++) {
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].attachment_id), ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].attachment_name), 0, ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].attachment_path), 0, ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].content_id), 0, CONTENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].attachment_size), ATTACHMENT_SIZE_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].mail_id), MAIL_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].account_id), ACCOUNT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].mailbox_id), MAILBOX_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].attachment_save_status), ATTACHMENT_SAVE_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].attachment_drm_type), ATTACHMENT_DRM_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].attachment_drm_method), ATTACHMENT_DRM_METHOD_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_int(hStmt, &(p_data_tbl[i].attachment_inline_content_status), ATTACHMENT_INLINE_CONTENT_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL);
		_get_stmt_field_data_string(hStmt, &(p_data_tbl[i].attachment_mime_type), 0, ATTACHMENT_MIME_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL);
		EM_DEBUG_LOG("attachment[%d].attachment_id : %d", i, p_data_tbl[i].attachment_id);
		EM_DEBUG_LOG("attachment_mime_type : %s", p_data_tbl[i].attachment_mime_type);
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; }, ("sqlite3_step fail:%d", rc));
	}

FINISH_OFF:

	if (error == EMAIL_ERROR_NONE)
		*output_attachment_list = p_data_tbl;
	else if (p_data_tbl != NULL)
		emstorage_free_attachment(&p_data_tbl, *output_attachment_count, NULL);

	if (hStmt) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
			if (*output_attachment_list)
				emstorage_free_attachment(output_attachment_list, *output_attachment_count, NULL); /* prevent */
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(input_transaction);

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int emstorage_get_attachment(char *multi_user_name, int attachment_id, emstorage_attachment_tbl_t **attachment, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_id[%d], attachment[%p], transaction[%d], err_code[%p]", attachment_id, attachment, transaction, err_code);

	if (attachment_id <= 0 || !attachment) {
		EM_DEBUG_EXCEPTION("attachment_id[%d], attachment[%p]", attachment_id, attachment);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	emstorage_attachment_tbl_t* p_data_tbl = NULL;
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_attachment_tbl WHERE attachment_id = %d",  attachment_id);

	sqlite3_stmt* hStmt = NULL;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG_DEV(" before sqlite3_prepare hStmt = %p", hStmt);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_prepare failed [%d] [%s]", rc, sql_query_string));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step failed [%d] [%s]", rc, sql_query_string));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_LOG("no matched attachment found...");
		error = EMAIL_ERROR_ATTACHMENT_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (!(p_data_tbl = (emstorage_attachment_tbl_t*)em_malloc(sizeof(emstorage_attachment_tbl_t) * 1))) {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	_get_stmt_field_data_int(hStmt, &(p_data_tbl->attachment_id), ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->attachment_name), 0, ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->attachment_path), 0, ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->content_id), 0, CONTENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
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

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_attachment_nth(char *multi_user_name, int mail_id, int nth, emstorage_attachment_tbl_t **attachment_tbl, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], nth[%d], attachment_tbl[%p], transaction[%d], err_code[%p]", mail_id, nth, attachment_tbl, transaction, err_code);

	if (mail_id <= 0 || nth <= 0 || !attachment_tbl) {
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_attachment_tbl WHERE mail_id = %d ORDER BY attachment_id LIMIT %d, 1", mail_id, (nth - 1));
	EM_DEBUG_LOG_SEC("query = [%s]", sql_query_string);

	DB_STMT hStmt = NULL;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION("no matched attachment found: mail_id[%d] nth[%d]", mail_id, nth);
		error = EMAIL_ERROR_ATTACHMENT_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (!(p_data_tbl = (emstorage_attachment_tbl_t*)em_malloc(sizeof(emstorage_attachment_tbl_t) * 1))) {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	p_data_tbl->attachment_id = sqlite3_column_int(hStmt, ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	if ((p = (char *)sqlite3_column_text(hStmt, ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)EM_SAFE_STRLEN(p))
		p_data_tbl->attachment_name = cpy_str(p);
	if ((p = (char *)sqlite3_column_text(hStmt, ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)EM_SAFE_STRLEN(p))
		p_data_tbl->attachment_path = cpy_str(p);
	if ((p = (char *)sqlite3_column_text(hStmt, CONTENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)EM_SAFE_STRLEN(p))
		p_data_tbl->content_id = cpy_str(p);
	p_data_tbl->attachment_size = sqlite3_column_int(hStmt, ATTACHMENT_SIZE_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->mail_id = sqlite3_column_int(hStmt, MAIL_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->account_id = sqlite3_column_int(hStmt, ACCOUNT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->mailbox_id = sqlite3_column_int(hStmt, MAILBOX_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->attachment_save_status = sqlite3_column_int(hStmt, ATTACHMENT_SAVE_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->attachment_drm_type = sqlite3_column_int(hStmt, ATTACHMENT_DRM_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->attachment_drm_method = sqlite3_column_int(hStmt, ATTACHMENT_DRM_METHOD_IDX_IN_MAIL_ATTACHMENT_TBL);
	p_data_tbl->attachment_inline_content_status = sqlite3_column_int(hStmt, ATTACHMENT_INLINE_CONTENT_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL);
	if ((p = (char *)sqlite3_column_text(hStmt, ATTACHMENT_MIME_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)EM_SAFE_STRLEN(p))
		p_data_tbl->attachment_mime_type = cpy_str(p);
#ifdef __ATTACHMENT_OPTI__
		p_data_tbl->encoding = sqlite3_column_int(hStmt, ENCODING_IDX_IN_MAIL_ATTACHMENT_TBL);
		if ((p = (char *)sqlite3_column_text(hStmt, SECTION_IDX_IN_MAIL_ATTACHMENT_TBL)) && (int)EM_SAFE_STRLEN(p))
			p_data_tbl->section = cpy_str(p);
#endif
 	ret = true;

FINISH_OFF:
	if (ret == true)
		*attachment_tbl = p_data_tbl;

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_attachment_by_attachment_path(char *multi_user_name, char *attachment_path, emstorage_attachment_tbl_t **attachment, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_path[%p], attachment[%p], transaction[%d], err_code[%p]", attachment_path, attachment, transaction, err_code);

	if (attachment_path == NULL || !attachment) {
		EM_DEBUG_EXCEPTION("attachment_path[%p], attachment[%p]", attachment_path, attachment);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	emstorage_attachment_tbl_t* p_data_tbl = NULL;
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_attachment_tbl WHERE attachment_path = '%s'", attachment_path);

	sqlite3_stmt* hStmt = NULL;

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG_DEV("before sqlite3_prepare hStmt = %p", hStmt);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_prepare failed [%d] [%s]", rc, sql_query_string));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step failed [%d] [%s]", rc, sql_query_string));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_LOG("no matched attachment found...");
		error = EMAIL_ERROR_ATTACHMENT_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (!(p_data_tbl = (emstorage_attachment_tbl_t*)em_malloc(sizeof(emstorage_attachment_tbl_t) * 1))) {
		EM_DEBUG_EXCEPTION("malloc failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	_get_stmt_field_data_int(hStmt, &(p_data_tbl->attachment_id), ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->attachment_name), 0, ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->attachment_path), 0, ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL);
	_get_stmt_field_data_string(hStmt, &(p_data_tbl->content_id), 0, CONTENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL);
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

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize failed [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_change_attachment_field(char *multi_user_name, int mail_id, email_mail_change_type_t type, emstorage_attachment_tbl_t *attachment, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], type[%d], attachment[%p], transaction[%d], err_code[%p]", mail_id, type, attachment, transaction, err_code);

	if (mail_id <= 0 || !attachment) {
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	switch (type) {
		case UPDATE_MAILBOX:
				EM_DEBUG_LOG("UPDATE_MAILBOX");
			if (!attachment->mailbox_id) {
				EM_DEBUG_EXCEPTION(" attachment->mailbox_id[%d]", attachment->mailbox_id);
				error = EMAIL_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}
			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_attachment_tbl SET mailbox_id = ? WHERE mail_id = %d", mail_id);

			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_LOG(" Before sqlite3_prepare hStmt = %p", hStmt);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

			_bind_stmt_field_data_int(hStmt, i++, attachment->mailbox_id);
			break;

		case UPDATE_SAVENAME:
			EM_DEBUG_LOG("UPDATE_SAVENAME");
			if (!attachment->attachment_path) {
				EM_DEBUG_EXCEPTION(" attachment->attachment_path[%p]", attachment->attachment_path);
				error = EMAIL_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}

			SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_attachment_tbl SET"
				"  attachment_size = ?"
				", attachment_save_status = ?"
				", attachment_path = ?"
				" WHERE mail_id = %d"
				" AND attachment_id = %d"
				, attachment->mail_id
				, attachment->attachment_id);


			EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
			EM_DEBUG_LOG(" Before sqlite3_prepare hStmt = %p", hStmt);
			EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

			_bind_stmt_field_data_int(hStmt, i++, attachment->attachment_size);
			_bind_stmt_field_data_int(hStmt, i++, attachment->attachment_save_status);
			_bind_stmt_field_data_string(hStmt, i++, (char *)attachment->attachment_path, 0, ATTACHMENT_PATH_LEN_IN_MAIL_ATTACHMENT_TBL);
			break;

		default:
			EM_DEBUG_LOG("type[%d]", type);
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}
	EM_DEBUG_LOG_SEC("query = [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	ret = true;

FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_rename_mailbox(char *multi_user_name, int input_mailbox_id, char *input_new_mailbox_name, char *input_new_mailbox_alias, void *input_eas_data, int input_eas_data_length, int input_transaction)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_id[%d] input_new_mailbox_name[%p] input_new_mailbox_alias[%p] input_eas_data[%p] input_eas_data_length[%d] input_transaction[%d]", input_mailbox_id, input_new_mailbox_name, input_new_mailbox_alias, input_eas_data, input_eas_data_length, input_transaction);

	int rc = 0;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int field_idx = 0;
	int account_id = 0;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *replaced_mailbox_name = NULL;
	char *replaced_alias = NULL;
	sqlite3 *local_db_handle = NULL;
	DB_STMT hStmt = NULL;
	emstorage_mailbox_tbl_t *old_mailbox_data = NULL;

	if (input_mailbox_id <= 0 || !input_new_mailbox_name || !input_new_mailbox_alias) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (strstr(input_new_mailbox_name, "'")) {
		replaced_mailbox_name = em_replace_all_string(input_new_mailbox_name, "'", "''");
	} else {
		replaced_mailbox_name = strdup(input_new_mailbox_name);
	}

	if (strstr(input_new_mailbox_alias, "'")) {
		replaced_alias = em_replace_all_string(input_new_mailbox_alias, "'", "''");
	} else {
		replaced_alias = strdup(input_new_mailbox_alias);
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	if ((error = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &old_mailbox_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed [%d]", error);
		EM_SAFE_FREE(replaced_mailbox_name);
		EM_SAFE_FREE(replaced_alias);
		return error;
	}

    if (old_mailbox_data == NULL) {
        EM_DEBUG_LOG("old_mailbox_data is NULL");
        error = EMAIL_ERROR_MAILBOX_NOT_FOUND;
        goto FINISH_OFF;
    }

	account_id = old_mailbox_data->account_id;

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, input_transaction, error);

	if (input_eas_data && input_eas_data_length > 0) {
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			"UPDATE mail_box_tbl SET"
			" mailbox_name = ?"
			",alias = ?"
			",eas_data = ?"
			",eas_data_length = ?"
			" WHERE mailbox_id = ?");

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);

		EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		_bind_stmt_field_data_string(hStmt, field_idx++, input_new_mailbox_name, 0, ATTACHMENT_NAME_LEN_IN_MAIL_ATTACHMENT_TBL);
		_bind_stmt_field_data_string(hStmt, field_idx++, input_new_mailbox_alias, 0, ATTACHMENT_PATH_LEN_IN_MAIL_ATTACHMENT_TBL);
		_bind_stmt_field_data_blob(hStmt, field_idx++, input_eas_data, input_eas_data_length);
		_bind_stmt_field_data_int(hStmt, field_idx++, input_eas_data_length);
		_bind_stmt_field_data_int(hStmt, field_idx++, input_mailbox_id);
	} else {
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
				"UPDATE mail_box_tbl SET"
				" mailbox_name = ?"
				",alias = ?"
				" WHERE mailbox_id = ?");

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);

		EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

		_bind_stmt_field_data_string(hStmt, field_idx++ , input_new_mailbox_name, 0, ATTACHMENT_NAME_LEN_IN_MAIL_ATTACHMENT_TBL);
		_bind_stmt_field_data_string(hStmt, field_idx++ , input_new_mailbox_alias, 0, ATTACHMENT_PATH_LEN_IN_MAIL_ATTACHMENT_TBL);
		_bind_stmt_field_data_int(hStmt, field_idx++ , input_mailbox_id);
	}


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (sqlite3_changes(local_db_handle) == 0)
		EM_DEBUG_LOG("no mail_meeting_tbl matched...");

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, input_transaction, ret, error);
	if (error == EMAIL_ERROR_NONE) {
		if (!emcore_notify_storage_event(NOTI_MAILBOX_RENAME, account_id, input_mailbox_id, input_new_mailbox_name, 0))
			EM_DEBUG_EXCEPTION("emcore_notify_storage_eventFailed [ NOTI_MAILBOX_RENAME ] >>>> ");
	} else {
		if (!emcore_notify_storage_event(NOTI_MAILBOX_RENAME_FAIL, account_id, input_mailbox_id, input_new_mailbox_name, error))
			EM_DEBUG_EXCEPTION("emcore_notify_storage_eventFailed [ NOTI_MAILBOX_RENAME_FAIL ] >>>> ");
	}

	EM_SAFE_FREE(replaced_mailbox_name);
	EM_SAFE_FREE(replaced_alias);

	if (old_mailbox_data)
		emstorage_free_mailbox(&old_mailbox_data, 1, NULL);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int emstorage_get_new_attachment_no(char *multi_user_name, int *attachment_no, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_no [%p], err_code[%p]", attachment_no, err_code);
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char *sql = "SELECT max(rowid) FROM mail_attachment_tbl;";
	char **result;

	if (!attachment_no) {
		EM_DEBUG_EXCEPTION("Invalid attachment");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	*attachment_no = -1;

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
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

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_add_attachment(char *multi_user_name, emstorage_attachment_tbl_t *attachment_tbl, int iscopy, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_tbl[%p], iscopy[%d], transaction[%d], err_code[%p]", attachment_tbl, iscopy, transaction, err_code);

	char *sql = NULL;
	char **result;
	int rc, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	DB_STMT hStmt = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	if (!attachment_tbl) {
		EM_DEBUG_EXCEPTION("attachment_tbl[%p], iscopy[%d]", attachment_tbl, iscopy);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	sql = "SELECT max(rowid) FROM mail_attachment_tbl;";

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL == result[1]) rc = 1;
	else rc = atoi(result[1]) + 1;
	sqlite3_free_table(result);

	attachment_tbl->attachment_id = rc;

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_attachment_tbl VALUES "
		"(?"	/* attachment_id */
		", ?"	/* attachment_name */
		", ?"	/* attachment_path */
		", ?"   /* content_id */
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


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bind_stmt_field_data_int(hStmt, ATTACHMENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->attachment_id);
	_bind_stmt_field_data_string(hStmt, ATTACHMENT_NAME_IDX_IN_MAIL_ATTACHMENT_TBL, (char*)attachment_tbl->attachment_name, 0, ATTACHMENT_NAME_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bind_stmt_field_data_string(hStmt, ATTACHMENT_PATH_IDX_IN_MAIL_ATTACHMENT_TBL, (char*)attachment_tbl->attachment_path, 0, ATTACHMENT_PATH_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bind_stmt_field_data_string(hStmt, CONTENT_ID_IDX_IN_MAIL_ATTACHMENT_TBL, (char*)attachment_tbl->content_id, 0, CONTENT_ID_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bind_stmt_field_data_int(hStmt, ATTACHMENT_SIZE_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->attachment_size);
	_bind_stmt_field_data_int(hStmt, MAIL_ID_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->mail_id);
	_bind_stmt_field_data_int(hStmt, ACCOUNT_ID_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->account_id);
	_bind_stmt_field_data_int(hStmt, MAILBOX_ID_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->mailbox_id);
	_bind_stmt_field_data_int(hStmt, ATTACHMENT_SAVE_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->attachment_save_status);
	_bind_stmt_field_data_int(hStmt, ATTACHMENT_DRM_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->attachment_drm_type);
	_bind_stmt_field_data_int(hStmt, ATTACHMENT_DRM_METHOD_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->attachment_drm_method);
	_bind_stmt_field_data_int(hStmt, ATTACHMENT_INLINE_CONTENT_STATUS_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->attachment_inline_content_status);
	_bind_stmt_field_data_string(hStmt, ATTACHMENT_MIME_TYPE_IDX_IN_MAIL_ATTACHMENT_TBL, (char*)attachment_tbl->attachment_mime_type, 0, ATTACHMENT_MIME_TYPE_LEN_IN_MAIL_ATTACHMENT_TBL);
#ifdef __ATTACHMENT_OPTI__
	_bind_stmt_field_data_int(hStmt, ENCODING_IDX_IN_MAIL_ATTACHMENT_TBL, attachment_tbl->encoding);
	_bind_stmt_field_data_string(hStmt, SECTION_IDX_IN_MAIL_ATTACHMENT_TBL, (char*)attachment_tbl->section, 0, ATTACHMENT_LEN_IN_MAIL_ATTACHMENT_TBL);
#endif


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_LOG(" no matched mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
	*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_update_attachment(char *multi_user_name, emstorage_attachment_tbl_t *attachment_tbl, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_tbl[%p], transaction[%d], err_code[%p]", attachment_tbl, transaction, err_code);

	int rc, ret = false, field_idx = 0;
	int error = EMAIL_ERROR_NONE;
	DB_STMT hStmt = NULL;
 	char sql_query_string[QUERY_SIZE] = {0, };

	if (!attachment_tbl) {
		EM_DEBUG_EXCEPTION(" attachment_tbl[%p] ", attachment_tbl);
		if (err_code)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_attachment_tbl SET  "
		"  attachment_name = ?"
		", attachment_path =  ?"
		", content_id = ?"
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


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bind_stmt_field_data_string(hStmt, field_idx++ , (char*)attachment_tbl->attachment_name, 0, ATTACHMENT_NAME_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bind_stmt_field_data_string(hStmt, field_idx++ , (char*)attachment_tbl->attachment_path, 0, ATTACHMENT_PATH_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bind_stmt_field_data_string(hStmt, field_idx++ , (char*)attachment_tbl->content_id, 0, CONTENT_ID_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bind_stmt_field_data_int(hStmt, field_idx++ , attachment_tbl->attachment_size);
	_bind_stmt_field_data_int(hStmt, field_idx++ , attachment_tbl->mail_id);
	_bind_stmt_field_data_int(hStmt, field_idx++ , attachment_tbl->account_id);
	_bind_stmt_field_data_int(hStmt, field_idx++ , attachment_tbl->mailbox_id);
	_bind_stmt_field_data_int(hStmt, field_idx++ , attachment_tbl->attachment_save_status);
	_bind_stmt_field_data_int(hStmt, field_idx++ , attachment_tbl->attachment_drm_type);
	_bind_stmt_field_data_int(hStmt, field_idx++ , attachment_tbl->attachment_drm_method);
	_bind_stmt_field_data_int(hStmt, field_idx++ , attachment_tbl->attachment_inline_content_status);
	_bind_stmt_field_data_string(hStmt, field_idx++ , (char*)attachment_tbl->attachment_mime_type, 0, ATTACHMENT_MIME_TYPE_LEN_IN_MAIL_ATTACHMENT_TBL);
	_bind_stmt_field_data_int(hStmt, field_idx++ , attachment_tbl->attachment_id);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((SQLITE_FULL == rc), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_LOG(" no matched mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
	*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_attachment_on_db(char *multi_user_name, int attachment_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_id[%d], transaction[%d], err_code[%p]", attachment_id, transaction, err_code);

	if (attachment_id < 0) {
		EM_DEBUG_EXCEPTION("attachment_id[%d]", attachment_id);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_attachment_tbl WHERE attachment_id = %d", attachment_id);

	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_all_attachments_of_mail(char *multi_user_name, int mail_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], transaction[%d], err_code[%p]", mail_id, transaction, err_code);
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = NULL;

	if (mail_id <= 0) {
		EM_DEBUG_EXCEPTION("mail_id[%d]", mail_id);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_attachment_tbl WHERE mail_id = %d", mail_id);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_attachment_all_on_db(char *multi_user_name, int account_id, char *mailbox, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox[%p], transaction[%d], err_code[%p]", account_id, mailbox, transaction, err_code);

	int error = EMAIL_ERROR_NONE;
	int ret = false;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *replaced_mailbox = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_attachment_tbl");

	if (account_id != ALL_ACCOUNT) /*  '0' means all account */
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " WHERE account_id = %d", account_id);

	if (mailbox) 	/*  NULL means all mailbox_name */ {
		if (strstr(mailbox, "'")) {
			replaced_mailbox = em_replace_all_string(mailbox, "'", "''");
		} else {
			replaced_mailbox = strdup(mailbox);
		}

		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " %s mailbox_name = '%s'", account_id != ALL_ACCOUNT ? "AND" : "WHERE", replaced_mailbox);
		EM_SAFE_FREE(replaced_mailbox); /*prevent 49434*/
	}

	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_free_attachment(emstorage_attachment_tbl_t** attachment_tbl_list, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("attachment_tbl_list[%p], count[%d], err_code[%p]", attachment_tbl_list, count, err_code);

	if (count > 0) {
		if ((attachment_tbl_list == NULL) || (*attachment_tbl_list == NULL)) {
			EM_DEBUG_LOG("Nothing to free: attachment_tbl_list[%p], count[%d]", attachment_tbl_list, count);
			if (err_code != NULL)
				*err_code = EMAIL_ERROR_INVALID_PARAM;
			return false;
		}

		emstorage_attachment_tbl_t* p = *attachment_tbl_list;
		int i;

		for (i = 0; i < count; i++) {
			EM_SAFE_FREE(p[i].attachment_name);
			EM_SAFE_FREE(p[i].attachment_path);
			EM_SAFE_FREE(p[i].content_id);
			EM_SAFE_FREE(p[i].attachment_mime_type);
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

INTERNAL_FUNC int emstorage_begin_transaction(char *multi_user_name, void *d1, void *d2, int *err_code)
{
	EM_PROFILE_BEGIN(emStorageBeginTransaction);
	int ret = true;

	_timedlock_shm_mutex(mapped_for_db_lock, 2);

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	int rc;
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN immediate;", NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {ret = false; },
		("SQL(BEGIN) exec error:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

	if (ret == false) {
		if (err_code != NULL) *err_code = EMAIL_ERROR_DB_FAILURE;
	}

	EM_PROFILE_END(emStorageBeginTransaction);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_commit_transaction(char *multi_user_name, void *d1, void *d2, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = true;
	int rc;

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {ret = false; }, ("SQL(END) exec error:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

	if (ret == false && err_code != NULL)
		*err_code = EMAIL_ERROR_DB_FAILURE;

	_unlockshm_mutex(mapped_for_db_lock);

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_rollback_transaction(char *multi_user_name, void *d1, void *d2, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = true;
	int rc;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "ROLLBACK;", NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {ret = false; },
		("SQL(ROLLBACK) exec error:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

	if (ret == false && err_code != NULL)
		*err_code = EMAIL_ERROR_DB_FAILURE;

	_unlockshm_mutex(mapped_for_db_lock);

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_is_mailbox_full(char *multi_user_name, int account_id, email_mailbox_t *mailbox, int *result, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox[%p], result[%p], err_code[%p]", account_id, mailbox, result, err_code);

	if (account_id < FIRST_ACCOUNT_ID || !mailbox || !result) {
		if (mailbox)
			EM_DEBUG_EXCEPTION("Invalid Parameter. accoun_id[%d], mailbox[%p]", account_id, mailbox);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;

		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int mail_count = 0;

	if (!emstorage_get_mail_count(multi_user_name, account_id, mailbox->mailbox_id, &mail_count, NULL, true, &error)) {
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

INTERNAL_FUNC int emstorage_clear_mail_data(char *multi_user_name, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("transaction[%d], err_code[%p]", transaction, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	const email_db_object_t* tables = _g_db_tables;
	const email_db_object_t* indexes = _g_db_indexes;

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	if (!emstorage_delete_dir((char *)MAILHOME, &error)) {
		EM_DEBUG_EXCEPTION(" emstorage_delete_dir failed - %d", error);

		goto FINISH_OFF;
	}

	mkdir(MAILHOME, DIRECTORY_PERMISSION);
	mkdir(MAILTEMP, DIRECTORY_PERMISSION);
	chmod(MAILTEMP, 0777);

	/*  first clear index. */
	while (indexes->object_name) {
		if (indexes->data_flag) {
			SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP index %s", indexes->object_name);
			error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
			if (error != EMAIL_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
					goto FINISH_OFF;
			}
		}
		indexes++;
	}

	while (tables->object_name) {
		if (tables->data_flag) {
			SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP table %s", tables->object_name);
			error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
			if (error != EMAIL_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
					goto FINISH_OFF;
			}
		}

		tables++;
	}
	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
/*======================= DB File Utils =============================================*/
#include <dirent.h>
#include <sys/types.h>
#define  DIR_SEPERATOR "/"

INTERNAL_FUNC char *emstorage_make_directory_path_from_file_path(char *file_name)
{
	EM_DEBUG_FUNC_BEGIN("Filename [ %p ]", file_name);
	char delims[] = "/";
	char *result = NULL;
	gchar **token = NULL;

	token = g_strsplit_set(file_name, delims, 1);

	if (token && token[0]) {
		EM_DEBUG_LOG_SEC(">>>> Directory_name [ %s ]", token[0]);
		result = EM_SAFE_STRDUP(token[0]);
	} else
		EM_DEBUG_LOG(">>>> No Need to create Directory");

	g_strfreev(token);

	return result;
}

INTERNAL_FUNC int emstorage_get_save_name(char *multi_user_name, int account_id, int mail_id, int atch_id,
											char *fname, char *move_buf, char *path_buf, int maxlen, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("account_id[%d], mail_id[%d], atch_id[%d], fname[%s], move_buf[%p], path_buf[%p], err_code[%p]", account_id, mail_id, atch_id, fname, move_buf, path_buf, err_code);
	EM_PROFILE_BEGIN(profile_emstorage_get_save_name);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char *dir_name = NULL;
	char create_dir[1024] = {0};
	char *temp_file = NULL;
	char *prefix_path = NULL;
	char *modified_fname = NULL;

	if (!move_buf || !path_buf || account_id < FIRST_ACCOUNT_ID || mail_id < 0 || atch_id < 0) {
		EM_DEBUG_EXCEPTION(" account_id[%d], mail_id[%d], atch_id[%d], fname[%p], move_buf[%p], path_buf[%p]", account_id, mail_id, atch_id, fname, move_buf, path_buf);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	snprintf(path_buf, 512, "%s", MAILHOME);
	snprintf(path_buf+EM_SAFE_STRLEN(path_buf), 512 - EM_SAFE_STRLEN(path_buf), "%s%d", DIR_SEPERATOR, account_id);

	if (mail_id > 0)
		snprintf(path_buf+EM_SAFE_STRLEN(path_buf),   512 - EM_SAFE_STRLEN(path_buf), "%s%d", DIR_SEPERATOR, mail_id);

	if (atch_id > 0)
		snprintf(path_buf+EM_SAFE_STRLEN(path_buf), 512 - EM_SAFE_STRLEN(path_buf), "%s%d", DIR_SEPERATOR, atch_id);

	if (fname) {
		temp_file = EM_SAFE_STRDUP(fname);
		if (temp_file && strstr(temp_file, "/")) {
			dir_name = emstorage_make_directory_path_from_file_path(temp_file);
		}
	}

	if (dir_name) {
		snprintf(create_dir, sizeof(create_dir), "%s%s%s", path_buf, DIR_SEPERATOR, dir_name);
		EM_DEBUG_LOG(">>>>> DIR PATH [%s]", create_dir);
		mkdir(create_dir, DIRECTORY_PERMISSION);
		EM_SAFE_FREE(dir_name);
	}

	if (fname) {
		EM_DEBUG_LOG_DEV(">>>>> fname [%s]", fname);

		/* Did not allow the slash */
		modified_fname = reg_replace_new(fname, "/", "_");
		EM_DEBUG_LOG("modified_fname : [%s]", modified_fname);

		if (modified_fname == NULL) modified_fname = g_strdup(fname);

		if (EM_SAFE_STRLEN(modified_fname) + EM_SAFE_STRLEN(path_buf) + strlen(DIR_SEPERATOR) > maxlen - 1) {
			char *modified_name = NULL;
			int remain_len  = (maxlen - 1) - EM_SAFE_STRLEN(path_buf) - strlen(DIR_SEPERATOR);

			if (remain_len <= 0) {
				error = EMAIL_ERROR_MAX_EXCEEDED;
				goto FINISH_OFF;
			}

			if (remain_len > MAX_FILENAME) {
				remain_len = MAX_FILENAME;
			}

			modified_name = em_shrink_filename(modified_fname, remain_len);

			if (!modified_name) {
				error = EMAIL_ERROR_MAX_EXCEEDED;
				goto FINISH_OFF;
			}

			snprintf(path_buf+EM_SAFE_STRLEN(path_buf), 512 - EM_SAFE_STRLEN(path_buf),"%s%s", DIR_SEPERATOR, modified_name);
			EM_DEBUG_LOG(">>>>> Modified fname [%s]", modified_name);
			EM_SAFE_FREE(modified_name);
		} else {
			if (EM_SAFE_STRLEN(modified_fname) > MAX_FILENAME - 1) {
				char *modified_name = NULL;

				modified_name = em_shrink_filename(modified_fname, MAX_FILENAME);
				if (!modified_name) {
					error = EMAIL_ERROR_MAX_EXCEEDED;
					goto FINISH_OFF;
				}

				snprintf(path_buf+EM_SAFE_STRLEN(path_buf), 512 - EM_SAFE_STRLEN(path_buf),"%s%s", DIR_SEPERATOR, modified_name);
				EM_DEBUG_LOG(">>>>> Modified fname [%s]", modified_name);
				EM_SAFE_FREE(modified_name);
			} else {
				snprintf(path_buf+EM_SAFE_STRLEN(path_buf), 512 - EM_SAFE_STRLEN(path_buf),"%s%s", DIR_SEPERATOR, modified_fname);
			}
		}
	}

	EM_DEBUG_LOG_SEC(">>>>> path_buf [%s]", path_buf);

    if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		error = emcore_get_container_path(multi_user_name, &prefix_path);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_container_path failed : [%d]", error);
			goto FINISH_OFF;
		}
        snprintf(move_buf, 512, "%s/%s", prefix_path, path_buf);
        EM_DEBUG_LOG("move_buf : [%s]", move_buf);
    } else {
        snprintf(move_buf, 512, "%s", path_buf);
        EM_DEBUG_LOG("move_buf : [%s]", move_buf);
    }

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(temp_file);
	EM_SAFE_FREE(prefix_path);
	EM_SAFE_FREE(modified_fname);

	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(profile_emstorage_get_save_name);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/*
INTERNAL_FUNC int emstorage_get_dele_name(char *multi_user_name, int account_id, int mail_id, int atch_id, char *fname, char *name_buf, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], atch_id[%d], fname[%p], name_buf[%p], err_code[%p]", account_id, mail_id, atch_id, fname, name_buf, err_code);

	if (!name_buf || account_id < FIRST_ACCOUNT_ID) {
		EM_DEBUG_EXCEPTION(" account_id[%d], mail_id[%d], atch_id[%d], fname[%p], name_buf[%p]", account_id, mail_id, atch_id, fname, name_buf);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sprintf(name_buf+EM_SAFE_STRLEN(name_buf), 	"%s%s%d", MAILHOME, DIR_SEPERATOR, account_id);

	if (mail_id > 0)
		sprintf(name_buf+EM_SAFE_STRLEN(name_buf), 	"%s%d", DIR_SEPERATOR, mail_id);
	else
		goto FINISH_OFF;

	if (atch_id > 0)
		sprintf(name_buf+EM_SAFE_STRLEN(name_buf), 	"%s%d", DIR_SEPERATOR, atch_id);
	else
		goto FINISH_OFF;

FINISH_OFF:
	sprintf(name_buf+EM_SAFE_STRLEN(name_buf), 	".DELE");

	EM_DEBUG_FUNC_END();
	return true;
}
*/

INTERNAL_FUNC int emstorage_create_dir(char *multi_user_name, int account_id, int mail_id, int atch_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], atch_id[%d], err_code[%p]", account_id, mail_id, atch_id, err_code);
	EM_PROFILE_BEGIN(profile_emcore_save_create_dir);
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char buf[512];
	struct stat sbuf;
	char *prefix_path = NULL;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	memset(buf, 0x00, sizeof(buf));

    if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		error = emcore_get_container_path(multi_user_name, &prefix_path);
		if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_container_path failed : [%d]", error);
			goto FINISH_OFF;
		}
	} else {
		prefix_path = strdup("");
	}

	if (account_id >= FIRST_ACCOUNT_ID) {
		SNPRINTF(buf, sizeof(buf), "%s%s%s%s%d", prefix_path,
												DIR_SEPERATOR,
												MAILHOME,
												DIR_SEPERATOR,
												account_id);

		if (stat(buf, &sbuf) == 0) {
			if ((sbuf.st_mode & S_IFMT) != S_IFDIR) {
				EM_DEBUG_EXCEPTION(" a object which isn't directory aleady exists");
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		} else {
			if (mkdir(buf, DIRECTORY_PERMISSION) != 0) {
				EM_DEBUG_EXCEPTION(" mkdir failed [%s]", buf);
				EM_DEBUG_EXCEPTION("mkdir failed: %s", EM_STRERROR(errno_buf));
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				if (errno == 28)
					error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			if (account_id == EML_FOLDER) {
				chmod(buf, 0777);
			}
		}
	}

	if (mail_id > 0) {
		int space_left_in_buffer = sizeof(buf) - EM_SAFE_STRLEN(buf);

		if (account_id < FIRST_ACCOUNT_ID) {
			EM_DEBUG_EXCEPTION("account_id[%d], mail_id[%d], atch_id[%d]", account_id, mail_id, atch_id);
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		if (space_left_in_buffer + 10 > sizeof(buf)) {
			EM_DEBUG_EXCEPTION("Buffer overflowed");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		SNPRINTF(buf+EM_SAFE_STRLEN(buf), space_left_in_buffer, "%s%d", DIR_SEPERATOR, mail_id);

		if (stat(buf, &sbuf) == 0) {
			if ((sbuf.st_mode & S_IFMT) != S_IFDIR) {
				EM_DEBUG_EXCEPTION(" a object which isn't directory aleady exists");
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		} else {
			if (mkdir(buf, DIRECTORY_PERMISSION) != 0) {
				EM_DEBUG_EXCEPTION("mkdir failed [%s]", buf);
				EM_DEBUG_EXCEPTION("mkdir failed [%d][%s]", errno, EM_STRERROR(errno_buf));
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				if (errno == 28)
					error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			if (account_id == EML_FOLDER) {
				chmod(buf, 0777);
			}
		}
	}

	if (atch_id > 0) {
		if (account_id < FIRST_ACCOUNT_ID || mail_id <= 0) {
			EM_DEBUG_EXCEPTION(" account_id[%d], mail_id[%d], atch_id[%d]", account_id, mail_id, atch_id);
			error = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		SNPRINTF(buf+EM_SAFE_STRLEN(buf), sizeof(buf)-(EM_SAFE_STRLEN(buf)+1), "%s%d", DIR_SEPERATOR, atch_id);

		if (stat(buf, &sbuf) == 0) {
			if ((sbuf.st_mode & S_IFMT) != S_IFDIR) {
				EM_DEBUG_EXCEPTION(" a object which isn't directory aleady exists");
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				goto FINISH_OFF;
			}
		} else {
			if (mkdir(buf, DIRECTORY_PERMISSION) != 0) {
				EM_DEBUG_EXCEPTION(" mkdir failed [%s]", buf);
				error = EMAIL_ERROR_SYSTEM_FAILURE;
				if (errno == 28)
					error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			if (account_id == EML_FOLDER) {
				chmod(buf, 0777);
			}
		}
	}

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(prefix_path);

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
	char buf[FILE_MAX_BUFFER_SIZE] = {0};
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (!src_file || !dst_file) {
		EM_DEBUG_EXCEPTION("src_file[%p], dst_file[%p]", src_file, dst_file);

		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (stat(src_file, &st_buf) < 0) {
		EM_DEBUG_EXCEPTION("stat(\"%s\") failed...", src_file);

		error = EMAIL_ERROR_SYSTEM_FAILURE;		/* EMAIL_ERROR_INVALID_PATH; */
		goto FINISH_OFF;
	}

	error = em_open(src_file, O_RDONLY, 0, &fp_src);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION(">>>> Source Fail em_open %s Failed: %d", src_file, error);
			goto FINISH_OFF;
	}

	error = em_open(dst_file, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH, &fp_dst); /*prevent 24474*/
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION(">>>> Destination Fail em_open %s:  %d", dst_file, error);
			goto FINISH_OFF;
	}

	memset(buf, 0x00, FILE_MAX_BUFFER_SIZE);

	while ((nread = read(fp_src, buf, FILE_MAX_BUFFER_SIZE)) > 0) {
		if (nread > 0 && nread <= FILE_MAX_BUFFER_SIZE) {
			EM_DEBUG_LOG("Nread Value [%d]", nread);
			char *buf_ptr;
			ssize_t byte_written = 0;
			size_t remain_byte = nread;
			buf_ptr = buf;
			errno = 0;

			while (remain_byte > 0 && buf_ptr && errno == 0) {
				byte_written = write(fp_dst, buf_ptr, remain_byte);

				if (byte_written < 0) {
					/* interrupted by a signal */
					if (errno == EINTR) {
						errno = 0;
						continue;
					}

					EM_DEBUG_EXCEPTION("fwrite failed: %s", EM_STRERROR(errno_buf));
					error = EMAIL_ERROR_UNKNOWN;
					goto FINISH_OFF;
				}
				EM_DEBUG_LOG("NWRITTEN [%d]", byte_written);
				remain_byte -= byte_written;
				buf_ptr += byte_written;
			}
		}

		memset(buf, 0x00, FILE_MAX_BUFFER_SIZE);
	}

	ret = true;

FINISH_OFF:
	EM_SAFE_CLOSE(fp_src);

	if (fp_dst >= 0) { /*prevent 24474*/
		if (sync_status) {
			EM_DEBUG_LOG("Before fsync");
			fsync(fp_dst);
		}
		close(fp_dst);
	}

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

	mkdir(DATA_PATH, DIRECTORY_PERMISSION);
	mkdir(EMAILPATH, DIRECTORY_PERMISSION);
	mkdir(MAILHOME, DIRECTORY_PERMISSION);
	mkdir(MAILTEMP, DIRECTORY_PERMISSION);
	chmod(MAILTEMP, 0777);

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
	unsigned int seed = time(NULL);
	SNPRINTF(tempname, sizeof(tempname), "%s%c%d", MAILTEMP, '/', rand_r(&seed));

	char *p = EM_SAFE_STRDUP(tempname);
	if (p == NULL) {
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
		EM_DEBUG_EXCEPTION_SEC(" stat(\"%s\") failed...", file_path);
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	buf_size =  st_buf.st_size;
	EM_DEBUG_LOG(">>>> File Size [ %d ] ", buf_size);
	buf = (char *)calloc(1, buf_size+1);

	if (!buf) {
		EM_DEBUG_LOG(">>> Memory cannot be allocated ");
		goto FINISH_OFF;
	}

	error = em_fopen(file_path, "rb", &fp_src);
	if (error != EMAIL_ERROR_NONE || fp_src == NULL) {
		EM_DEBUG_EXCEPTION_SEC(" file_path fopen failed - %s [%d]", file_path, error);
		goto FINISH_OFF;
	}

	if ((nread = fread(buf, 1, buf_size, fp_src)) > 0) {
		if (nread > 0 && nread <= buf_size) {
			EM_DEBUG_LOG(">>>> Nread Value [ %d ] ", nread);

			/**
			  *   1.Add check for whether content type is there.
			  *   2. If not based on the character set, Append it in File
			  **/

			low_char_set = calloc(1, EM_SAFE_STRLEN(char_set) + strlen(" \" /></head>") +1); /*prevent 34359*/
			if (low_char_set == NULL) {
				EM_DEBUG_EXCEPTION("calloc failed");
				error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			strncat(low_char_set, char_set, EM_SAFE_STRLEN(char_set));
			EM_DEBUG_LOG(">>>> CHAR SET [ %s ] ", low_char_set);
			strncat(low_char_set, " \" /></head>", strlen(" \" /></head>")); /*prevent 34359*/
			EM_DEBUG_LOG(">>> CHARSET [ %s ] ", low_char_set);
			match_str = strstr(buf, CONTENT_TYPE_DATA);

			if (match_str == NULL) {
				EM_DEBUG_LOG(">>>>emstorage_add_content_type 3 ");
				if (fp_src != NULL) {
					fclose(fp_src);
					fp_src = NULL;
				}
				data_count_to_written = EM_SAFE_STRLEN(low_char_set)+strlen(CONTENT_DATA)+1; /*prevent 34359*/
				buf1 = (char *)calloc(1, data_count_to_written);

				if (buf1) {
					strncat(buf1, CONTENT_DATA, strlen(CONTENT_DATA)); /*prevent 34359*/
					EM_DEBUG_LOG(">>>>> BUF 1 [ %s ] ", buf1);
					strncat(buf1, low_char_set, EM_SAFE_STRLEN(low_char_set));
					EM_DEBUG_LOG(">>>> HTML TAG DATA  [ %s ] ", buf1);

					/* 1. Create a temporary file name */
					if (!_get_temp_file_name(&temp_file_name, &err)) {
							EM_DEBUG_EXCEPTION(" emcore_get_temp_file_name failed - %d", err);
							if (err_code != NULL) *err_code = err;
							goto FINISH_OFF;
					}
					EM_DEBUG_LOG_SEC(">>>>>>> TEMP APPEND FILE PATH [ %s ] ", temp_file_name);

					/* Open the Temp file in Append mode */
					error = em_fopen(temp_file_name, "ab", &fp_dest);
					if (error != EMAIL_ERROR_NONE) {
						EM_DEBUG_EXCEPTION_SEC(" fopen failed - %s [%d]", temp_file_name, error);
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
						} else {
							EM_DEBUG_LOG(">>>> OLD data appended [ %d ] ", nwritten);

							if (!emstorage_move_file(temp_file_name, file_path, false, &err)) {
								EM_DEBUG_EXCEPTION(" emstorage_move_file failed - %d", err);
								goto FINISH_OFF;
							}
						}

					} else {
						EM_DEBUG_EXCEPTION(" Error Occured while writing New data : [%d ] bytes written ", nwritten);
						error = EMAIL_ERROR_SYSTEM_FAILURE;
						goto FINISH_OFF;
					}
				}
			}
		}
	}

	ret = true;
FINISH_OFF:

	EM_SAFE_FREE(buf);
	EM_SAFE_FREE(buf1);
	EM_SAFE_FREE(low_char_set);
	EM_SAFE_FREE(temp_file_name);

	if (fp_src != NULL) {
		fclose(fp_src);
		fp_src = NULL;
	}

	if (fp_dest != NULL) {
		fclose(fp_dest);
		fp_dest = NULL;
	}

	if (err_code)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}

INTERNAL_FUNC int emstorage_move_file(char *src_file, char *dst_file, int sync_status, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("src_file[%p], dst_file[%p], err_code[%p]", src_file, dst_file, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (src_file == NULL || dst_file == NULL) {
		EM_DEBUG_EXCEPTION("src_file[%p], dst_file[%p]", src_file, dst_file);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_SEC("src_file[%s], dst_file[%s]", src_file, dst_file);

	if (strcmp(src_file, dst_file) != 0) {
		if (rename(src_file, dst_file) != 0) {
			/* EM_DEBUG_EXCEPTION("%s", strerror(errno)); */
			if (errno == EXDEV) {	/* oldpath and newpath are not on the same mounted file system.  (Linux permits a file system to be mounted at multiple points,  but  rename() */
				/*  does not work across different mount points, even if the same file system is mounted on both.)	 */
				EM_DEBUG_LOG("oldpath and newpath are not on the same mounted file system.");
				if (!emstorage_copy_file(src_file, dst_file, sync_status, &error)) {
					EM_DEBUG_EXCEPTION("emstorage_copy_file failed - %d", error);
					goto FINISH_OFF;
				}
				remove(src_file);
				EM_DEBUG_LOG("src[%s] removed", src_file);
			} else {
				if (errno == ENOENT) {
					struct stat temp_file_stat;
					if (stat(src_file, &temp_file_stat) < 0) {
						EM_DEBUG_EXCEPTION("no src file found [%s] : %s", src_file, EM_STRERROR(errno_buf));
					}
					if (stat(dst_file, &temp_file_stat) < 0)
						EM_DEBUG_EXCEPTION("no dst file found [%s] : %s", dst_file, EM_STRERROR(errno_buf));

					error = EMAIL_ERROR_FILE_NOT_FOUND;
					goto FINISH_OFF;
				} else {
					EM_DEBUG_EXCEPTION("rename failed: %s", EM_STRERROR(errno_buf));
					error = EMAIL_ERROR_SYSTEM_FAILURE;
					goto FINISH_OFF;
				}
			}
		}
	} else {
		EM_DEBUG_LOG("src[%s] = dst[%s]", src_file, dst_file);
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
		} else {
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
		EM_DEBUG_EXCEPTION("src_dir[%p]", src_dir);

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

	if (dirp == NULL) {
		if (errno == ENOENT) {
			EM_DEBUG_EXCEPTION("directory[%s] does not exist...", src_dir);
			if (err_code != NULL)
				*err_code = EMAIL_ERROR_SYSTEM_FAILURE;
			return true;
		} else {
			EM_DEBUG_EXCEPTION("opendir failed [%s] [%d]", src_dir, errno);
			if (err_code != NULL)
				*err_code = EMAIL_ERROR_SYSTEM_FAILURE;
			return false;
		}
	}

	while ((dp = readdir(dirp))) {
		if (strncmp(dp->d_name, ".", 1) == 0 || strncmp(dp->d_name, "..", 2) == 0) /* prevent 34360 */
			continue;

		SNPRINTF(buf, sizeof(buf), "%s/%s", src_dir, dp->d_name);

		if (lstat(buf, &sbuf) == 0 || stat(buf, &sbuf) == 0) {
			/*  check directory */
			if ((sbuf.st_mode & S_IFMT) == S_IFDIR) {	/*  directory */
				/*  recursive call */
				if (!emstorage_delete_dir(buf, &error)) {
					closedir(dirp);
					if (err_code != NULL)
						*err_code = error;
					return false;
				}
			} else {	/*  file */
				if (remove(buf) < 0) {
					EM_DEBUG_EXCEPTION("remove failed [%s] [%d]", buf, errno);
					closedir(dirp);
					if (err_code != NULL)
						*err_code = EMAIL_ERROR_SYSTEM_FAILURE;
					return false;
				}
			}
		} else
			EM_DEBUG_EXCEPTION("content does not exist...");
	}

	closedir(dirp);

	EM_DEBUG_LOG_DEV("remove direcotory [%s]", src_dir);

	/* EM_DEBUG_FUNC_BEGIN(); */

	if (remove(src_dir) < 0) {
		EM_DEBUG_EXCEPTION("remove failed [%s] [%d]", src_dir, errno);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_SYSTEM_FAILURE;
		return false;
	}

	if (err_code != NULL)
		*err_code = error;

	return true;
}

/* faizan.h@samsung.com */
INTERNAL_FUNC int emstorage_update_server_uid(char *multi_user_name,
												int mail_id,
												char *old_server_uid,
												char *new_server_uid,
												int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("new_server_uid[%s], old_server_uid[%s]", new_server_uid, old_server_uid);
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int transaction = true;
	int temp_strlen = 0;
	int and_operation = 0;
	char sql_query_string[QUERY_SIZE] = {0, };
	char conditional_clause_string[QUERY_SIZE] = {0};

	if ((mail_id <= 0 || !old_server_uid) && !new_server_uid) {
		EM_DEBUG_EXCEPTION("Invalid parameters");
		if (err_code)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(conditional_clause_string, sizeof(conditional_clause_string), "WHERE ");

	if (mail_id > 0) {
		temp_strlen = strlen(conditional_clause_string);
		SNPRINTF(conditional_clause_string + temp_strlen, sizeof(conditional_clause_string) - temp_strlen,
					"mail_id = %d ", mail_id);
		and_operation = 1;
	}

	if (old_server_uid) {
		temp_strlen = strlen(conditional_clause_string);
		if (!and_operation) {
			sqlite3_snprintf(sizeof(conditional_clause_string) - temp_strlen, conditional_clause_string + temp_strlen,
							"server_mail_id = '%q'", old_server_uid);
		} else {
			sqlite3_snprintf(sizeof(conditional_clause_string) - temp_strlen, conditional_clause_string + temp_strlen,
							"and server_mail_id = '%q'", old_server_uid);
		}
	}

	sqlite3_snprintf(sizeof(sql_query_string), sql_query_string,
						"UPDATE mail_tbl SET server_mail_id = '%q' %s", new_server_uid, conditional_clause_string);

	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}

INTERNAL_FUNC int emstorage_update_read_mail_uid(char *multi_user_name, int mail_id, char *new_server_uid, char *mbox_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("mail_id[%d], new_server_uid[%s], mbox_name[%s]", mail_id, new_server_uid, mbox_name);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	int transaction = true;

	if (!mail_id || !new_server_uid || !mbox_name) {
		EM_DEBUG_EXCEPTION("Invalid parameters");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);


	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		 "UPDATE mail_read_mail_uid_tbl SET server_uid=\'%s\', mailbox_id=\'%s\', mailbox_name=\'%s\' WHERE local_uid=%d ", new_server_uid, mbox_name, mbox_name, mail_id);

	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	ret	= true;

FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}

INTERNAL_FUNC int emstorage_update_save_status(char *multi_user_name, int account_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int transaction = true;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0,};

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	if (account_id <= ALL_ACCOUNT)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET save_status = %d WHERE (save_status = %d or save_status = %d)", EMAIL_MAIL_STATUS_NONE, EMAIL_MAIL_STATUS_NOTI_WAITED, EMAIL_MAIL_STATUS_RECEIVED);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET save_status = %d WHERE (save_status = %d or save_status = %d) and account_id = %d ", EMAIL_MAIL_STATUS_NONE, EMAIL_MAIL_STATUS_NOTI_WAITED, EMAIL_MAIL_STATUS_RECEIVED, account_id);

	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}

int emstorage_get_unread_mailid(char *multi_user_name, int account_id, int vip_mode, int **mail_ids, int *mail_number, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	if ((!mail_ids) || (account_id <= 0 &&  account_id != -1)) {
		EM_DEBUG_EXCEPTION(" mail_id[%p], account_id[%d] ", mail_ids, account_id);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int rc = -1;
	int error = EMAIL_ERROR_NONE;
	int count = 0;
	int i = 0;
	int col_index = 0;
	int *p_mail_ids = NULL;
	int transaction = false;
	char **result = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char temp_query_string[QUERY_SIZE] = {0,};
	char sql_select_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	if (account_id == -1)
		SNPRINTF(sql_select_query_string, sizeof(sql_select_query_string), "SELECT mail_id FROM mail_tbl WHERE flags_seen_field = 0 AND (save_status = %d or save_status = %d)", EMAIL_MAIL_STATUS_NOTI_WAITED, EMAIL_MAIL_STATUS_RECEIVED);
	else
		SNPRINTF(sql_select_query_string, sizeof(sql_select_query_string), "SELECT mail_id FROM mail_tbl WHERE account_id = %d AND flags_seen_field = 0 AND (save_status = %d or save_status = %d)", account_id, EMAIL_MAIL_STATUS_NOTI_WAITED, EMAIL_MAIL_STATUS_RECEIVED);

	if (vip_mode) {
		SNPRINTF(temp_query_string, sizeof(temp_query_string), "%s AND tag_id < 0", sql_select_query_string);
	} else {
		SNPRINTF(temp_query_string, sizeof(temp_query_string), "%s", sql_select_query_string);
	}

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "%s ORDER BY date_time ASC", temp_query_string);

	EM_DEBUG_LOG_SEC("query: [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG("Count : %d", count);

	if (count == 0) {
		EM_DEBUG_EXCEPTION("no Mails found...");
		ret = false;
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;
	}

	p_mail_ids = em_malloc(count * sizeof(int));
	if (p_mail_ids == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	col_index = 1;

	for (i = 0; i < count; i++) {
		_get_table_field_data_int(result, &(p_mail_ids[i]), col_index++);
	}

	ret = true;

FINISH_OFF:

	if (result)
		sqlite3_free_table(result);

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

//	sqlite3_db_release_memory(local_db_handle);

	if (ret == true) {
		if (mail_ids != NULL)
			*mail_ids = p_mail_ids;

		if (mail_number != NULL)
			*mail_number = count;
	} else {
		EM_SAFE_FREE(p_mail_ids);
	}

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

	do {
	 	if (waitpid(pid, &status, 0) == -1) {
			if (errno != EINTR)
				return -1;
		} else {
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

	error = em_fopen(SETTING_MEMORY_TEMP_FILE_PATH, "r", &fp);
	if (error != EMAIL_ERROR_NONE) {
		perror(SETTING_MEMORY_TEMP_FILE_PATH);
		goto FINISH_OFF;
	}

 	line_from_file = fgets(line, sizeof(line), fp);

 	if (line_from_file == NULL) {
 		EM_DEBUG_EXCEPTION("fgets failed");
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
 	}
 	total_diskusage = strtoul(line, NULL, 10);

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

 	if (fp) fclose(fp); /* prevent 32730 */

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
#define MAILHOME_UTF8    tzplatform_mkpath(TZ_USER_DATA, "email/.email_data/7/348/UTF-8")

INTERNAL_FUNC int emstorage_test(char *multi_user_name, int mail_id, int account_id, char *full_address_to, char *full_address_cc, char *full_address_bcc, int *err_code)
{
	DB_STMT hStmt = NULL;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int rc = 0;
	char sql_query_string[QUERY_SIZE] = {0, };

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_tbl VALUES "
		"(?" /*  mail_id */
		", ?" /*  account_id */
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
		", ?" /*  scheduled_sending_time */
		", ?" /*  remaining_resend_times */
		", ?" /*  tag_id   */
		", ?" /*  replied_time */
		", ?" /*  forwarded_time */
		", ?" /*  default_charset */
		", ?" /*  eas_data_length */
		", ?" /*  eas_data */
		")");

	int transaction = true;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	_bind_stmt_field_data_int(hStmt, MAIL_ID_IDX_IN_MAIL_TBL, mail_id);
	_bind_stmt_field_data_int(hStmt, ACCOUNT_ID_IDX_IN_MAIL_TBL, account_id);
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
	_bind_stmt_field_data_string(hStmt, FILE_PATH_PLAIN_IDX_IN_MAIL_TBL, (char *)MAILHOME_UTF8, 0, TEXT_1_LEN_IN_MAIL_TBL);
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
	_bind_stmt_field_data_int(hStmt, SCHEDULED_SENDING_TIME_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, REMAINING_RESEND_TIMES_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, TAG_ID_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, REPLIED_TIME_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_int(hStmt, FORWARDED_TIME_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_string(hStmt, DEFAULT_CHARSET_IDX_IN_MAIL_TBL, "UTF-8", 0, TEXT_2_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_int(hStmt, EAS_DATA_LENGTH_IDX_IN_MAIL_TBL, 0);
	_bind_stmt_field_data_blob(hStmt, EAS_DATA_IDX_IN_MAIL_TBL, NULL, 0);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_max_mail_count()
{
	return EMAIL_MAIL_MAX_COUNT;
}

#define STRIPPED_SUBJECT_BUFFER_SIZE 4086

INTERNAL_FUNC int emstorage_get_thread_id_of_thread_mails(char *multi_user_name,
															emstorage_mail_tbl_t *mail_tbl,
															int *thread_id,
															int *result_latest_mail_id_in_thread,
															int *thread_item_count)
{
	EM_DEBUG_FUNC_BEGIN("mail_tbl [%p], thread_id [%p], "
						"result_latest_mail_id_in_thread [%p], thread_item_count [%p]",
						mail_tbl, thread_id, result_latest_mail_id_in_thread, thread_item_count);
	EM_PROFILE_BEGIN(profile_emstorage_get_thread_id_of_thread_mails);

	int rc = 0, query_size = 0, query_size_account = 0;
	int i = 0;
	int search_thread = false;
	int account_id = 0;
	int err_code = EMAIL_ERROR_NONE;
	int count = 0, result_thread_id = -1, latest_mail_id_in_thread = -1;
	time_t latest_date_time = 0;
	char *subject = NULL;
	char *p_subject = NULL;
	char *sql_query_string = NULL, *sql_account = NULL;
	int col_index = 4;
	int temp_thread_id = -1;
	char *sql_format = "SELECT mail_id, thread_id, date_time, subject "
						"FROM mail_tbl WHERE subject like \'%%%q\' AND mailbox_id = %d";
	char *sql_format_account = " AND account_id = %d ";
	char *sql_format_order_by = " ORDER BY thread_id, date_time DESC ";
	char **result = NULL;
	char stripped_subject[STRIPPED_SUBJECT_BUFFER_SIZE];
	char stripped_subject2[STRIPPED_SUBJECT_BUFFER_SIZE];

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EM_DEBUG_LOG("subject: [%p], mail_id: [%d]", subject, mail_tbl->mail_id);

	EM_IF_NULL_RETURN_VALUE(mail_tbl, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(thread_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(result_latest_mail_id_in_thread, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(thread_item_count, EMAIL_ERROR_INVALID_PARAM);

	account_id   = mail_tbl->account_id;
	subject      = mail_tbl->subject;

	EM_DEBUG_LOG_SEC("subject: [%s]", subject);

	if (EM_SAFE_STRLEN(subject) == 0 && mail_tbl->mail_id != 0) {
		result_thread_id = mail_tbl->mail_id;
		count = 1;
		goto FINISH_OFF;
	}

	if (em_find_pos_stripped_subject_for_thread_view(subject,
														stripped_subject,
														STRIPPED_SUBJECT_BUFFER_SIZE) != EMAIL_ERROR_NONE)	{
		EM_DEBUG_EXCEPTION("em_find_pos_stripped_subject_for_thread_view is failed");
		err_code =  EMAIL_ERROR_UNKNOWN;
		result_thread_id = -1;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_SEC("stripped_subject: [%s]", stripped_subject);

	if (EM_SAFE_STRLEN(stripped_subject) == 0) {
		result_thread_id = -1;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_SEC("em_find_pos_stripped_subject_for_thread_view returns[len = %d] = %s",
						EM_SAFE_STRLEN(stripped_subject), stripped_subject);

	if (account_id > 0) 	{
		query_size_account = 3 + EM_SAFE_STRLEN(sql_format_account);
		sql_account = malloc(query_size_account);
		if (sql_account == NULL) {
			EM_DEBUG_EXCEPTION("malloc for sql_account  is failed %d", query_size_account);
			err_code =  EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		snprintf(sql_account, query_size_account, sql_format_account, account_id);
	}

	/* prevent 34362 */
	query_size = strlen(sql_format) + strlen(stripped_subject)*2 + 50 + query_size_account + strlen(sql_format_order_by); /*  + query_size_mailbox; */
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

	EM_DEBUG_LOG_SEC("Query : %s", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL),
									rc);

	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err_code = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG("Result rows count : %d", count);

	if (count == 0)
		result_thread_id = -1;
	else {
		for (i = 0; i < count; i++) {
			EM_SAFE_FREE(p_subject);

			_get_table_field_data_int(result, &latest_mail_id_in_thread, col_index++);
			_get_table_field_data_int(result, &result_thread_id, col_index++);
			_get_table_field_data_time_t(result, &latest_date_time, col_index++);
			_get_table_field_data_string(result, &p_subject, 0, col_index++);

			if (temp_thread_id == result_thread_id)
				continue;

			temp_thread_id = result_thread_id;

			if (em_find_pos_stripped_subject_for_thread_view(p_subject,
																stripped_subject2,
																STRIPPED_SUBJECT_BUFFER_SIZE) != EMAIL_ERROR_NONE)	{
				EM_DEBUG_EXCEPTION("em_find_pos_stripped_subject_for_thread_view is failed");
				err_code = EMAIL_ERROR_UNKNOWN;
				result_thread_id = -1;
				goto FINISH_OFF;
			}

			if (g_strcmp0(stripped_subject2, stripped_subject) == 0) {
				if (latest_date_time < mail_tbl->date_time)
					*result_latest_mail_id_in_thread = latest_mail_id_in_thread;
				else
					*result_latest_mail_id_in_thread = mail_tbl->mail_id;

				search_thread = true;
				break;
			}

			if (search_thread) {
				EM_DEBUG_LOG("latest_mail_id_in_thread [%d], mail_id [%d]",
								latest_mail_id_in_thread, mail_tbl->mail_id);
			} else {
				result_thread_id = -1;
				count = 0;
			}
		}

	}

FINISH_OFF:

	*thread_id = result_thread_id;
	*thread_item_count = count;

	EM_DEBUG_LOG("Result thread id : %d", *thread_id);
	EM_DEBUG_LOG("Result count : %d", *thread_item_count);
	EM_DEBUG_LOG("err_code : %d", err_code);

	EM_SAFE_FREE(sql_account);
	EM_SAFE_FREE(sql_query_string);
	EM_SAFE_FREE(p_subject);

	sqlite3_free_table(result);

	EM_PROFILE_END(profile_emstorage_get_thread_id_of_thread_mails);

	return err_code;
}

INTERNAL_FUNC int emstorage_get_thread_id_from_mailbox(char *multi_user_name, int account_id, int mailbox_id, char *mail_subject, int *thread_id, int *thread_item_count)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_id [%d], subject [%p], thread_id [%p], thread_item_count [%p]", mailbox_id, mail_subject, thread_id, thread_item_count);
	EM_PROFILE_BEGIN(profile_emstorage_get_thread_id_of_thread_mails);

	int rc = 0;
	int query_size = 0;
	int query_size_account = 0;
	int err_code = EMAIL_ERROR_NONE;
	int count = 0;
	int result_thread_id = -1;
	char *sql_query_string = NULL;
	char *sql_account = NULL;
	char *sql_format = "SELECT thread_id FROM mail_tbl WHERE subject like \'%%%q\' AND mailbox_id = %d";
	char *sql_format_account = " AND account_id = %d ";
	char *sql_format_order_by = " ORDER BY date_time DESC ";
	char **result = NULL;
	char stripped_subject[STRIPPED_SUBJECT_BUFFER_SIZE];
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EM_IF_NULL_RETURN_VALUE(mail_subject, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(thread_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(thread_item_count, EMAIL_ERROR_INVALID_PARAM);

	EM_DEBUG_LOG_SEC("subject: [%s]", mail_subject);

	if (em_find_pos_stripped_subject_for_thread_view(mail_subject, stripped_subject, STRIPPED_SUBJECT_BUFFER_SIZE) != EMAIL_ERROR_NONE)	{
		EM_DEBUG_EXCEPTION("em_find_pos_stripped_subject_for_thread_view  is failed");
		err_code =  EMAIL_ERROR_UNKNOWN;
		result_thread_id = -1;
		goto FINISH_OFF;
	}

	if (EM_SAFE_STRLEN(stripped_subject) == 0) {
		result_thread_id = -1;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("em_find_pos_stripped_subject_for_thread_view returns[len = %d] = %s", EM_SAFE_STRLEN(stripped_subject), stripped_subject);

	if (account_id > 0) {
		query_size_account = 3 + EM_SAFE_STRLEN(sql_format_account);
		sql_account = malloc(query_size_account);
		if (sql_account == NULL) {
			EM_DEBUG_EXCEPTION("malloc for sql_account  is failed %d", query_size_account);
			err_code =  EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		snprintf(sql_account, query_size_account, sql_format_account, account_id);
	}

	query_size = EM_SAFE_STRLEN(sql_format) + EM_SAFE_STRLEN(stripped_subject)*2 + 50 + query_size_account + EM_SAFE_STRLEN(sql_format_order_by); /*  + query_size_mailbox; */
	sql_query_string = malloc(query_size);

	if (sql_query_string == NULL) {
		EM_DEBUG_EXCEPTION("malloc for sql  is failed %d", query_size);
		err_code =  EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	sqlite3_snprintf(query_size, sql_query_string, sql_format, stripped_subject, mailbox_id);

	if (account_id > 0)
		strcat(sql_query_string, sql_account);

	strcat(sql_query_string, sql_format_order_by);
	strcat(sql_query_string, ";");

	EM_DEBUG_LOG_SEC("Query : %s", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err_code = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG("Result rows count : %d", count);

	if (count == 0)
		result_thread_id = -1;
	else {
		_get_table_field_data_int(result, &result_thread_id, 1);
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

INTERNAL_FUNC int emstorage_get_thread_information(char *multi_user_name, int thread_id, emstorage_mail_tbl_t** mail_tbl, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int count = 0, ret = false;
	int error = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t *p_data_tbl = NULL;
	char conditional_clause[QUERY_SIZE] = {0, };

	EM_IF_NULL_RETURN_VALUE(mail_tbl, false);

	SNPRINTF(conditional_clause, QUERY_SIZE, "WHERE thread_id = %d AND thread_item_count > 0", thread_id);
	EM_DEBUG_LOG("conditional_clause [%s]", conditional_clause);

	if (!emstorage_query_mail_tbl(multi_user_name, conditional_clause, transaction, &p_data_tbl, &count, &error)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_tbl failed [%d]", error);
		goto FINISH_OFF;
	}

	if (p_data_tbl)
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

INTERNAL_FUNC int emstorage_get_sender_list(char *multi_user_name, int account_id, int mailbox_id, int search_type, const char *search_value, email_sort_type_t sorting, email_sender_list_t** sender_list, int *sender_count,  int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("account_id [%d], mailbox_id [%d], search_type [%d], search_value [%p], sorting [%d], sender_list[%p], sender_count[%p] err_code[%p]"
		, account_id , mailbox_id , search_type , search_value , sorting , sender_list, sender_count, err_code);

	if ((!sender_list) || (!sender_count)) {
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
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"SELECT email_address_sender, alias_sender, COUNT(email_address_sender), SUM(flags_seen_field = 1) "
		"FROM mail_tbl ");

	/*  mailbox_id */
	if (mailbox_id)
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), QUERY_SIZE-(EM_SAFE_STRLEN(sql_query_string)+1), " WHERE mailbox_id = %d ", mailbox_id);
	else	/*  NULL  means all mailbox_name. but except for trash(3), spambox(5), all emails(for GMail, 7) */
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), QUERY_SIZE-(EM_SAFE_STRLEN(sql_query_string)+1), " WHERE mailbox_type not in (3, 5, 7, 8) ");

	/*  account id */
	/*  '0' (ALL_ACCOUNT) means all account */
	if (account_id > ALL_ACCOUNT)
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), QUERY_SIZE-(EM_SAFE_STRLEN(sql_query_string)+1), " AND account_id = %d ", account_id);

	if (search_value) {
		switch (search_type) {
			case EMAIL_SEARCH_FILTER_SUBJECT:
				SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), QUERY_SIZE-(EM_SAFE_STRLEN(sql_query_string)+1),
					" AND (UPPER(subject) LIKE UPPER(\'%%%%%s%%%%\')) ", search_value);
				break;
			case EMAIL_SEARCH_FILTER_SENDER:
				SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), QUERY_SIZE-(EM_SAFE_STRLEN(sql_query_string)+1),
					" AND  ((UPPER(full_address_from) LIKE UPPER(\'%%%%%s%%%%\')) "
					") ", search_value);
				break;
			case EMAIL_SEARCH_FILTER_RECIPIENT:
				SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), QUERY_SIZE-(EM_SAFE_STRLEN(sql_query_string)+1),
					" AND ((UPPER(full_address_to) LIKE UPPER(\'%%%%%s%%%%\')) "
					"	OR (UPPER(full_address_cc) LIKE UPPER(\'%%%%%s%%%%\')) "
					"	OR (UPPER(full_address_bcc) LIKE UPPER(\'%%%%%s%%%%\')) "
					") ", search_value, search_value, search_value);
				break;
			case EMAIL_SEARCH_FILTER_ALL:
				SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), QUERY_SIZE-(EM_SAFE_STRLEN(sql_query_string)+1),
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
	SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1),
		"GROUP BY email_address_sender "
		"ORDER BY UPPER(alias_sender) ");

	EM_DEBUG_LOG_SEC("query[%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG("Count of Sender [%d]", count);

	if (!(p_sender_list = (email_sender_list_t*)em_malloc(sizeof(email_sender_list_t) * count))) {
		EM_DEBUG_EXCEPTION("em_mallocfailed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	col_index = 4;

	EM_DEBUG_LOG(">>>> DATA ASSIGN START >>");
	for (i = 0; i < count; i++) {
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
	if (ret == true) {
		*sender_list = p_sender_list;
		*sender_count = count;
		EM_DEBUG_LOG(">>>> COUNT : %d >>", count);
	}


	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_free_sender_list(email_sender_list_t **sender_list, int count)
{
	EM_DEBUG_FUNC_BEGIN("sender_list[%p], count[%d]", sender_list, count);

	int err = EMAIL_ERROR_NONE;

	if (count > 0) {
		if (!sender_list || !*sender_list) {
			EM_DEBUG_EXCEPTION("sender_list[%p], count[%d]", sender_list, count);
			err = EMAIL_ERROR_INVALID_PARAM;
			return err;
		}

		email_sender_list_t* p = *sender_list;
		int i = 0;

		for (; i < count; i++) {
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

	if (!address_info_list || !*address_info_list) {
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
		g_list_free(list);
	}

	EM_SAFE_FREE(*address_info_list);
	*address_info_list = NULL;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__

INTERNAL_FUNC int emstorage_add_pbd_activity(char *multi_user_name, email_event_partial_body_thd* local_activity, int *activity_id, int transaction, int *err_code)
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
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_partial_body_activity_tbl VALUES "
		"("
		"? "  /* Account ID */
		",?"  /* Local Mail ID */
		",?"  /* Server mail ID */
		",?"  /* Activity ID */
		",?"  /* Activity type*/
		",?"  /* Mailbox ID*/
		",?"  /* Mailbox name*/
		",?"  /* Multi User Name */
		") ");

	char *sql = "SELECT max(rowid) FROM mail_partial_body_activity_tbl;";
	char **result = NULL;


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL == result[1]) rc = 1;
	else rc = atoi(result[1])+1;
	sqlite3_free_table(result);
	result = NULL;

	*activity_id = local_activity->activity_id = rc;

	EM_DEBUG_LOG_SEC(">>>>> ACTIVITY ID [ %d ], MAIL ID [ %d ], ACTIVITY TYPE [ %d ], SERVER MAIL ID [ %lu ]", \
		local_activity->activity_id, local_activity->mail_id, local_activity->activity_type, local_activity->server_mail_id);

	if (local_activity->mailbox_id)
		EM_DEBUG_LOG(" MAILBOX ID [ %d ]", local_activity->mailbox_id);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	_bind_stmt_field_data_int(hStmt, i++, local_activity->account_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->mail_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->server_mail_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->activity_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->activity_type);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->mailbox_id);
	_bind_stmt_field_data_string(hStmt, i++ , (char *)local_activity->mailbox_name, 0, 3999);
	_bind_stmt_field_data_string(hStmt, i++ , (char *)local_activity->multi_user_name, 0, MAX_USER_NAME_LENGTH);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);

	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d, errmsg = %s.", rc, sqlite3_errmsg(local_db_handle)));

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_pbd_mailbox_list(char *multi_user_name, int account_id, int **mailbox_list, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_list[%p], count[%p] err_code[%p]", account_id, mailbox_list, count, err_code);

	if (account_id < FIRST_ACCOUNT_ID || NULL == mailbox_list || *mailbox_list == NULL || NULL == count) {
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(distinct mailbox_id) FROM mail_partial_body_activity_tbl WHERE account_id = %d order by mailbox_id", account_id);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
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

	EM_DEBUG_LOG_SEC(" Query [%s]", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);


	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	mbox_list = (int *)em_malloc(sizeof(int) * (*count));
	if (NULL == mbox_list) {
		EM_DEBUG_EXCEPTION(" em_mallocfailed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(mbox_list, 0x00, sizeof(int) * (*count));

	for (i = 0; i < (*count); i++) {
		_get_stmt_field_data_int(hStmt, mbox_list + i, 0);
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		/* EM_DEBUG_LOG("In emstorage_get_pdb_mailbox_list() loop, After sqlite3_step(), , i = %d, rc = %d.", i,  rc); */
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
		EM_DEBUG_LOG("mbox_list %d", mbox_list[i]);
	}

	ret = true;

FINISH_OFF:
	if (ret == true)
		*mailbox_list = mbox_list;
	else
		EM_SAFE_FREE(mbox_list);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_pbd_account_list(char *multi_user_name, int **account_list, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_list[%p], count[%p] err_code[%p]", account_list, count, err_code);

	if (NULL == account_list || NULL == count) {
		EM_DEBUG_EXCEPTION("account_list[%p], count[%p] err_code[%p]", account_list, count, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char *sql = "SELECT count(distinct account_id) FROM mail_partial_body_activity_tbl";
	char **result;
	int i = 0, rc = -1;
	int *result_account_list = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
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

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG("Before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (NULL == (result_account_list = (int *)em_malloc(sizeof(int) * (*count)))) {
		EM_DEBUG_EXCEPTION(" em_mallocfailed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(result_account_list, 0x00, sizeof(int) * (*count));

	for (i = 0; i < (*count); i++) {
		_get_stmt_field_data_int(hStmt, result_account_list + i, 0);

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
		EM_DEBUG_LOG("account id -> %d", result_account_list[i]);
	}

	ret = true;

FINISH_OFF:
	if (ret == true)
		*account_list = result_account_list;
	else
		EM_SAFE_FREE(result_account_list);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_pbd_activity_data(char *multi_user_name, int account_id, int input_mailbox_id, email_event_partial_body_thd** event_start, int *count, int transaction, int *err_code)
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(*) FROM mail_partial_body_activity_tbl WHERE account_id = %d AND mailbox_id = '%d' order by activity_id", account_id, input_mailbox_id);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);

	EM_DEBUG_LOG_SEC("Query = [%s]", sql_query_string);

	if (!*count) {
		EM_DEBUG_LOG("No matched activity found in mail_partial_body_activity_tbl");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("Activity Count = %d", *count);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_partial_body_activity_tbl WHERE account_id = %d AND mailbox_id = '%d' order by activity_id", account_id, input_mailbox_id);

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG(" Bbefore sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (!(event_list = (email_event_partial_body_thd*)em_malloc(sizeof(email_event_partial_body_thd) * (*count)))) {
		EM_DEBUG_EXCEPTION("Malloc failed");

		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memset(event_list, 0x00, sizeof(email_event_partial_body_thd) * (*count));

	for (i = 0; i < (*count); i++) {
		_get_stmt_field_data_int(hStmt, &(event_list[i].account_id), ACCOUNT_IDX_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, &(event_list[i].mail_id), MAIL_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, (int *)&(event_list[i].server_mail_id), SERVER_MAIL_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, &(event_list[i].activity_id), ACTIVITY_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, &(event_list[i].activity_type), ACTIVITY_TYPE_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, &(event_list[i].mailbox_id), MAILBOX_ID_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_get_stmt_field_data_string(hStmt, &(event_list[i].mailbox_name), 0, MAILBOX_NAME_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);
		_get_stmt_field_data_string(hStmt, &(event_list[i].multi_user_name), 0, MULTI_USER_NAME_IDX_IN_MAIL_PARTIAL_BODY_ACTIVITY_TBL);

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		/* EM_DEBUG_LOG("In emstorage_get_pbd_activity_data() loop, After sqlite3_step(), , i = %d, rc = %d.", i,  rc); */
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));

		event_list[i].event_type = 0;
	}

	ret = true;

FINISH_OFF:
	if (true == ret)
	  *event_start = event_list;
	else {
		for (i = 0; i < (*count); i++)
			emcore_free_partial_body_thd_event(event_list, NULL);
		EM_SAFE_FREE(event_list); /*prevent 54559*/
		*event_start = NULL;
		*count = 0;
	}

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_pbd_activity(char *multi_user_name, int account_id, int mail_id, int activity_id, int transaction, int *err_code)
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
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	if (activity_id == 0)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_partial_body_activity_tbl WHERE account_id = %d AND mail_id = %d", account_id, mail_id);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_partial_body_activity_tbl WHERE account_id = %d AND activity_id = %d", account_id, activity_id);

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	/*  validate activity existence */
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION("No matching activity found");
		error = EMAIL_ERROR_DATA_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_mailbox_pbd_activity_count(char *multi_user_name, int account_id, int input_mailbox_id, int *activity_count, int transaction, int *err_code)
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(*) FROM mail_partial_body_activity_tbl WHERE account_id = %d and mailbox_id = '%d'", account_id, input_mailbox_id);

	EM_DEBUG_LOG_SEC(" Query [%s]", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	_get_stmt_field_data_int(hStmt, activity_count, 0);

	EM_DEBUG_LOG("No. of activities in activity table [%d]", *activity_count);

	ret = true;

FINISH_OFF:

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_pbd_activity_count(char *multi_user_name, int *activity_count, int transaction, int *err_code)
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(*) FROM mail_partial_body_activity_tbl;");

	EM_DEBUG_LOG_DEV(" Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG_DEV("  before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	_get_stmt_field_data_int(hStmt, activity_count, 0);

	EM_DEBUG_LOG("No. of activities in activity table [%d]", *activity_count);

	ret = true;

FINISH_OFF:


	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_full_pbd_activity_data(char *multi_user_name, int account_id, int transaction, int *err_code)
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_partial_body_activity_tbl WHERE account_id = %d", account_id);

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION("No matching activities found in mail_partial_body_activity_tbl");
		error = EMAIL_ERROR_DATA_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

/*Himanshu[h.gahlaut]-> Added below API to update mail_partial_body_activity_tbl
if a mail is moved before its partial body is downloaded.Currently not used but should be used if mail move from server is enabled*/

INTERNAL_FUNC int emstorage_update_pbd_activity(char *multi_user_name, char *old_server_uid, char *new_server_uid, char *mbox_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("old_server_uid[%s], new_server_uid[%s], mbox_name[%s]", old_server_uid, new_server_uid, mbox_name);

	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	int transaction = true;

	if (!old_server_uid || !new_server_uid || !mbox_name) {
		EM_DEBUG_EXCEPTION("Invalid parameters");
		error = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		 "UPDATE mail_partial_body_activity_tbl SET server_mail_id = %s , mailbox_name=\'%s\' WHERE server_mail_id = %s ", new_server_uid, mbox_name, old_server_uid);

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_LOG("No matching found in mail_partial_body_activity_tbl");
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_create_file(char *data_string, size_t file_size, char *dst_file_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("file_size[%d] , dst_file_name[%s], err_code[%p]", file_size, dst_file_name, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	FILE* fp_dst = NULL;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (!data_string || !dst_file_name) {
		EM_DEBUG_LOG("data_string[%p], dst_file_name[%p]", data_string, dst_file_name);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	error = em_fopen(dst_file_name, "w", &fp_dst);
	if (error != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION_SEC("em_fopen failed - %s: %d", dst_file_name, error);
		goto FINISH_OFF;
	}

	if (fwrite(data_string, 1, file_size, fp_dst) == 0) {
		EM_DEBUG_EXCEPTION("fwrite failed: %s", EM_STRERROR(errno_buf));
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
INTERNAL_FUNC int emstorage_update_read_mail_uid_by_server_uid(char *multi_user_name, char *old_server_uid, char *new_server_uid, char *mbox_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int rc = -1;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	int transaction = true;

	if (!old_server_uid || !new_server_uid || !mbox_name) {
		EM_DEBUG_EXCEPTION("Invalid parameters");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	EM_DEBUG_LOG_SEC("old_server_uid[%s], new_server_uid[%s], mbox_name[%s]", old_server_uid, new_server_uid, mbox_name);

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);


	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		 "UPDATE mail_read_mail_uid_tbl SET server_uid=\'%s\' , mailbox_name=\'%s\' WHERE server_uid=%s ", new_server_uid, mbox_name, old_server_uid);

	EM_DEBUG_LOG_SEC(" Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION("No matching found in mail_partial_body_activity_tbl");
		error = EMAIL_ERROR_DATA_NOT_FOUND;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
  	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

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
INTERNAL_FUNC int emstorage_get_id_set_from_mail_ids(char *multi_user_name,
														char *mail_ids,
														email_id_set_t** idset,
														int *id_set_count,
														int *err_code)
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
		EM_DEBUG_EXCEPTION("Invalid Parameters mail_ids[%p] idset[%p] id_set_count [%p]",
							mail_ids, idset, id_set_count);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	SNPRINTF(sql_query_string, space_left_in_query_buffer,
			"SELECT local_uid, server_uid FROM mail_read_mail_uid_tbl WHERE local_uid in (%s) ORDER BY server_uid",
			mail_ids);

	EM_DEBUG_LOG_SEC("SQL Query formed [%s] ", sql_query_string);

	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result);
						goto FINISH_OFF; },	("SQL(%s) sqlite3_get_table fail:%d -%s",
						sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG(" Count of mails [%d ]", count);

	if (count <= 0) {
		EM_DEBUG_EXCEPTION("Can't find proper mail");
		error = EMAIL_ERROR_DATA_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (NULL == (p_id_set = (email_id_set_t*)em_malloc(sizeof(email_id_set_t) * count))) {
		EM_DEBUG_EXCEPTION(" em_mallocfailed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	col_index = 2;

	EM_PROFILE_BEGIN(EmStorageGetIdSetFromMailIds_Loop);
	EM_DEBUG_LOG(">>>> DATA ASSIGN START");
	for (i = 0; i < count; i++) {
		_get_table_field_data_int(result, &(p_id_set[i].mail_id), col_index++);
		_get_table_field_data_string(result, &server_mail_id, 1, col_index++);
		if (server_mail_id) {
			p_id_set[i].server_mail_id = strtoul(server_mail_id, NULL, 10);
			EM_SAFE_FREE(server_mail_id);
		}
	}
	EM_DEBUG_LOG(">>>> DATA ASSIGN END [count : %d]", count);
	EM_PROFILE_END(EmStorageGetIdSetFromMailIds_Loop);

	sqlite3_free_table(result);
	result = NULL;

	ret = true;

FINISH_OFF:

	if (ret == true) {
		*idset = p_id_set;
		*id_set_count = count;
		EM_DEBUG_LOG(" idset[%p] id_set_count [%d]", *idset, *id_set_count);
	} else
		EM_SAFE_FREE(p_id_set);


	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(EmStorageGetIdSetFromMailIds);

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}



#endif

INTERNAL_FUNC int emstorage_delete_triggers_from_lucene(char *multi_user_name)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = true, transaction = true;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP TRIGGER triggerDelete;");
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP TRIGGER triggerInsert;");
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DROP TRIGGER triggerUpdate;");
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_update_tag_id(char *multi_user_name, int old_filter_id, int new_filter_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("new_filter_id[%d], old_filter_id[%d]", new_filter_id, old_filter_id);
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	int transaction = true;

	if (old_filter_id < 0 || new_filter_id < 0) {
		EM_DEBUG_EXCEPTION("Invalid parameters");
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		 "UPDATE mail_tbl SET tag_id=%d WHERE tag_id=%d ", new_filter_id, old_filter_id);

	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_filter_mails_by_rule(char *multi_user_name, int account_id, int dest_mailbox_id, int dest_mailbox_type, int reset, emstorage_rule_tbl_t *rule, int ** filtered_mail_id_list, int *count_of_mails, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], dest_mailbox_id[%d] rule[%p], filtered_mail_id_list[%p], count_of_mails[%p], err_code[%p]", account_id, dest_mailbox_id, rule, filtered_mail_id_list, count_of_mails, err_code);

	if ((account_id < 0) || (dest_mailbox_id < 0) || (!rule)) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1, ret = false, error = EMAIL_ERROR_NONE;
	int count = 0, col_index = 0, i = 0, where_pararaph_length = 0, *mail_list = NULL;
	int tag_id = rule->rule_id;
	char **result = NULL, *where_pararaph = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mail_id FROM mail_tbl ");

	where_pararaph_length = EM_SAFE_STRLEN(rule->value) + 2 * (EM_SAFE_STRLEN(rule->value2)) + 100;
	where_pararaph = em_malloc(sizeof(char) * where_pararaph_length);
	if (where_pararaph == NULL) {
		EM_DEBUG_EXCEPTION("malloc failed for where_pararaph.");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (account_id != ALL_ACCOUNT)
		SNPRINTF(where_pararaph, where_pararaph_length, "WHERE account_id = %d AND mailbox_type NOT in (0)", account_id);
	else
		SNPRINTF(where_pararaph, where_pararaph_length, "WHERE mailbox_type NOT in (0)");

	if (rule->type & EMAIL_FILTER_SUBJECT) {
		if (rule->flag2 == RULE_TYPE_INCLUDES)
			sqlite3_snprintf(where_pararaph_length - (EM_SAFE_STRLEN(where_pararaph) + 1), where_pararaph + EM_SAFE_STRLEN(where_pararaph), " AND subject like \'%%%q%%\'", rule->value);
		else /*  RULE_TYPE_EXACTLY */
			sqlite3_snprintf(where_pararaph_length - (EM_SAFE_STRLEN(where_pararaph) + 1), where_pararaph + EM_SAFE_STRLEN(where_pararaph), " AND subject = \'%q\'", rule->value);
	}

	if (rule->type & EMAIL_FILTER_FROM) {
		if (rule->flag2 == RULE_TYPE_INCLUDES)
			sqlite3_snprintf(where_pararaph_length - (EM_SAFE_STRLEN(where_pararaph) + 1), where_pararaph + EM_SAFE_STRLEN(where_pararaph), " AND full_address_from like \'%%%q%%\'", rule->value2);
#ifdef __FEATURE_COMPARE_DOMAIN__
		else if (rule->flag2 == RULE_TYPE_COMPARE_DOMAIN)
			sqlite3_snprintf(where_pararaph_length - (EM_SAFE_STRLEN(where_pararaph) + 1), where_pararaph + EM_SAFE_STRLEN(where_pararaph), " AND (full_address_from like \'@%%%q\' OR full_address_from like \'@%%%q>%%\')", rule->value2, rule->value2);
#endif /*__FEATURE_COMPARE_DOMAIN__ */
		else /*  RULE_TYPE_EXACTLY */
			sqlite3_snprintf(where_pararaph_length - (EM_SAFE_STRLEN(where_pararaph) + 1), where_pararaph + EM_SAFE_STRLEN(where_pararaph), " AND full_address_from = \'%q\'", rule->value2);
	}

	if (rule->type == EMAIL_PRIORITY_SENDER) {
		if (rule->flag2 == RULE_TYPE_INCLUDES)
			sqlite3_snprintf(where_pararaph_length - (EM_SAFE_STRLEN(where_pararaph) + 1), where_pararaph + EM_SAFE_STRLEN(where_pararaph), " AND full_address_from like \'%%%q%%\'", rule->value2);
		else /*  RULE_TYPE_EXACTLY */
			sqlite3_snprintf(where_pararaph_length - (EM_SAFE_STRLEN(where_pararaph) + 1), where_pararaph + EM_SAFE_STRLEN(where_pararaph), " AND full_address_from = \'%q\' OR email_address_sender = \'%q\'", rule->value2, rule->value2);

		tag_id = PRIORITY_SENDER_TAG_ID;
	}

	/* prevent 34361 */
	if (strlen(sql_query_string) + strlen(where_pararaph) < QUERY_SIZE)
		strcat(sql_query_string, where_pararaph);

	EM_DEBUG_LOG_SEC("query[%s]", sql_query_string);

	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
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

		memset(sql_query_string, 0x00, QUERY_SIZE);
		if (reset) {
			switch (rule->action_type) {
			case EMAIL_FILTER_MOVE:
				SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET tag_id = 0 ");
				break;
			case EMAIL_FILTER_BLOCK:
			default:
				EM_DEBUG_LOG("Not support : action_type[%d]", rule->action_type);
				ret = true;
				goto FINISH_OFF;
			}
		} else {
			switch (rule->action_type) {
			case EMAIL_FILTER_MOVE:
				SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET tag_id = %d ", tag_id);
				break;
			case EMAIL_FILTER_BLOCK:
				SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET mailbox_id = %d, mailbox_type = %d ", dest_mailbox_id, dest_mailbox_type);
				break;
			default:
				EM_DEBUG_LOG("Not support");
				ret = true;
				goto FINISH_OFF;
			}
		}
		/* prevent 34361 */
		if (strlen(sql_query_string) + strlen(where_pararaph) < QUERY_SIZE)
			strcat(sql_query_string, where_pararaph);

		error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (error != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
				goto FINISH_OFF;
		}

#ifdef __FEATURE_BODY_SEARCH__
		/* Updating mail_text_tbl */
		if (rule->action_type == EMAIL_FILTER_BLOCK) {
			memset(sql_query_string, 0x00, QUERY_SIZE);
			SNPRINTF(sql_query_string, QUERY_SIZE, "UPDATE mail_text_tbl SET mailbox_id = %d ", dest_mailbox_id);
			if (strlen(sql_query_string) + strlen(where_pararaph) < QUERY_SIZE)
				strcat(sql_query_string, where_pararaph);

			EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
			error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
			if (error != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
				goto FINISH_OFF;
			}
		}
#endif
	}

	ret = true;

FINISH_OFF:

	if (ret) {
		if (filtered_mail_id_list)
			*filtered_mail_id_list = mail_list;

		if (count_of_mails)
			*count_of_mails = count;
	} else
		EM_SAFE_FREE(mail_list);

	sqlite3_free_table(result);
	result = NULL;


	EM_SAFE_FREE(where_pararaph);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

#define EMAIL_SLOT_UNIT 25
INTERNAL_FUNC int emstorage_set_mail_slot_size(char *multi_user_name, int account_id, int mailbox_id, int new_slot_size, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_id[%p] new_slot_size[%d], err_code[%p]", account_id, mailbox_id, new_slot_size, err_code);
	int ret = false, err = EMAIL_ERROR_NONE;
	int where_pararaph_length = 0;
	char *where_pararaph = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	int and = 0;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, err);

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
		SNPRINTF(where_pararaph + EM_SAFE_STRLEN(where_pararaph), where_pararaph_length - EM_SAFE_STRLEN(where_pararaph), " %s total_mail_count_on_server > mail_slot_size ", (and ? "AND" : "WHERE"));

	if (strlen(sql_query_string) + EM_SAFE_STRLEN(where_pararaph) < QUERY_SIZE) /* prevent 34363 */
		strcat(sql_query_string, where_pararaph);
	else {
		EM_DEBUG_EXCEPTION("Query buffer overflowed !!!");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_SEC("query[%s]", sql_query_string);
	err = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", err);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, err);

	EM_SAFE_FREE(where_pararaph);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_add_meeting_request(char *multi_user_name, int account_id, int input_mailbox_id, email_meeting_request_t* meeting_req, int transaction, int *err_code)
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_meeting_tbl VALUES "
		"(?"		/*  mail_id */
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
		")");

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	if (rc != SQLITE_OK) {
		EM_DEBUG_LOG(" before sqlite3_prepare hStmt = %p", hStmt);
		EM_DEBUG_EXCEPTION("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle));

		error = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	col_index = 0;
	/*
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->mail_id[%d]", meeting_req->mail_id);
	EM_DEBUG_LOG_SEC(">>>>> account_id[%d]", account_id);
	EM_DEBUG_LOG_SEC(">>>>> mailbox_name[%s]", mailbox_name);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->meeting_response[%d]", meeting_req->meeting_response);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->start_time[%s]", asctime(&(meeting_req->start_time)));
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->end_time[%s]", asctime(&(meeting_req->end_time)));
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->location[%s]", meeting_req->location);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->global_object_id[%s]", meeting_req->global_object_id);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->time_zone.offset_from_GMT[%d]", meeting_req->time_zone.offset_from_GMT);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->time_zone.standard_name[%s]", meeting_req->time_zone.standard_name);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->time_zone.standard_time_start_date[%s]", asctime(&(meeting_req->time_zone.standard_time_start_date)));
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->time_zone.standard_bias[%d]", meeting_req->time_zone.standard_bias);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->time_zone.daylight_name[%s]", meeting_req->time_zone.daylight_name);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->time_zone.daylight_time_start_date[%s]", asctime(&(meeting_req->time_zone.daylight_time_start_date)));
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->time_zone.daylight_bias[%d]", meeting_req->time_zone.daylight_bias);
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
	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_query_meeting_request(char *multi_user_name, const char *conditional_clause, email_meeting_request_t **output_meeting_req, int *output_result_count, int transaction)
{
	EM_DEBUG_FUNC_BEGIN("conditional_clause[%s] output_meeting_req[%p] output_result_count[%p] transaction[%d]", conditional_clause, output_meeting_req, output_result_count, transaction);

	int i = 0;
	int col_index = 0;
	int rc;
	int count = 0;
	int dummy = 0;
	int err = EMAIL_ERROR_NONE;
	char **result = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	email_meeting_request_t* p_temp_meeting_req = NULL;
	sqlite3 *local_db_handle = NULL;
	time_t temp_unix_time;

	if (conditional_clause == NULL || output_meeting_req == NULL || output_result_count == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	col_index = _field_count_of_table[CREATE_TABLE_MAIL_MEETING_TBL];
	EM_DEBUG_LOG("col_index [%d]", col_index);

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_meeting_tbl %s", conditional_clause);

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {err = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (!count) {
		EM_DEBUG_EXCEPTION("No meeting_request found...");
		err = EMAIL_ERROR_DATA_NOT_FOUND;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("There are [%d] meeting requests.", count);
	if (!(p_temp_meeting_req = (email_meeting_request_t*)em_malloc(sizeof(email_meeting_request_t) * count))) {
		EM_DEBUG_EXCEPTION("malloc for emstorage_mail_tbl_t failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG(">>>> DATA ASSIGN START >> ");

	for (i = 0; i < count; i++) {
		_get_table_field_data_int(result, &(p_temp_meeting_req[i].mail_id), col_index++);
		_get_table_field_data_int(result, &dummy, col_index++); /* account_id. but why should this field exist in DB table? */
		_get_table_field_data_int(result, &dummy, col_index++); /* mailbox_id */
		_get_table_field_data_int(result, (int*)&(p_temp_meeting_req[i].meeting_response), col_index++);
		_get_table_field_data_int(result, (int*)(&temp_unix_time), col_index++); /* start time */
		gmtime_r(&temp_unix_time, &(p_temp_meeting_req[i].start_time));
		_get_table_field_data_int(result, (int*)(&temp_unix_time), col_index++); /* end time */
		gmtime_r(&temp_unix_time, &(p_temp_meeting_req[i].end_time));
		_get_table_field_data_string(result, &p_temp_meeting_req[i].location, 1, col_index++);
		_get_table_field_data_string(result, &p_temp_meeting_req[i].global_object_id, 1, col_index++);
		_get_table_field_data_int(result, &(p_temp_meeting_req[i].time_zone.offset_from_GMT), col_index++);
		_get_table_field_data_string_without_allocation(result, p_temp_meeting_req[i].time_zone.standard_name, STANDARD_NAME_LEN_IN_MAIL_MEETING_TBL, 1, col_index++);
		_get_table_field_data_int(result, (int*)(&temp_unix_time), col_index++);
		gmtime_r(&temp_unix_time, &(p_temp_meeting_req[i].time_zone.standard_time_start_date));
		_get_table_field_data_int(result, &(p_temp_meeting_req[i].time_zone.standard_bias), col_index++);
		_get_table_field_data_string_without_allocation(result, p_temp_meeting_req[i].time_zone.daylight_name, DAYLIGHT_NAME_LEN_IN_MAIL_MEETING_TBL, 1, col_index++);
		_get_table_field_data_int(result, (int*)(&temp_unix_time), col_index++);
		gmtime_r(&temp_unix_time, &(p_temp_meeting_req[i].time_zone.daylight_time_start_date));
		_get_table_field_data_int(result, &(p_temp_meeting_req[i].time_zone.daylight_bias), col_index++);
	}


FINISH_OFF:
	if (result)
		sqlite3_free_table(result);

	if (err == EMAIL_ERROR_NONE) {
		if (p_temp_meeting_req)
			*output_meeting_req = p_temp_meeting_req;
		*output_result_count = count;
	} else
		EM_SAFE_FREE(p_temp_meeting_req);

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

//	sqlite3_db_release_memory(local_db_handle);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emstorage_get_meeting_request(char *multi_user_name, int mail_id, email_meeting_request_t ** meeting_req, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int count = 0;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char conditional_clause[QUERY_SIZE] = {0, };

	EM_IF_NULL_RETURN_VALUE(meeting_req, false);


	SNPRINTF(conditional_clause, QUERY_SIZE, " WHERE mail_id = %d", mail_id);
	EM_DEBUG_LOG("conditional_clause [%s]", conditional_clause);

	if ((error = emstorage_query_meeting_request(multi_user_name, conditional_clause, meeting_req, &count, transaction)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_meeting_request failed. [%d]", error);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_SEC(">>>>> (*meeting_req)->mail_id[%d]", (*meeting_req)->mail_id);
	EM_DEBUG_LOG_SEC(">>>>> (*meeting_req)->meeting_response[%d]", (*meeting_req)->meeting_response);
	EM_DEBUG_LOG_SEC(">>>>> (*meeting_req)->start_time[%s]", asctime(&((*meeting_req)->start_time)));
	EM_DEBUG_LOG_SEC(">>>>> (*meeting_req)->end_time[%s]", asctime(&((*meeting_req)->end_time)));
	EM_DEBUG_LOG_SEC(">>>>> (*meeting_req)->location[%s]", (*meeting_req)->location);
	EM_DEBUG_LOG_SEC(">>>>> (*meeting_req)->global_object_id[%s]", (*meeting_req)->global_object_id);
	EM_DEBUG_LOG_SEC(">>>>> (*meeting_req)->time_zone.offset_from_GMT[%d]", (*meeting_req)->time_zone.offset_from_GMT);
	EM_DEBUG_LOG_SEC(">>>>> (*meeting_req)->time_zone.standard_name[%s]", (*meeting_req)->time_zone.standard_name);
	EM_DEBUG_LOG_SEC(">>>>> (*meeting_req)->time_zone.standard_time_start_date[%s]", asctime(&((*meeting_req)->time_zone.standard_time_start_date)));
	EM_DEBUG_LOG_SEC(">>>>> (*meeting_req)->time_zone.standard_bias[%d]", (*meeting_req)->time_zone.standard_bias);
	EM_DEBUG_LOG_SEC(">>>>> (*meeting_req)->time_zone.daylight_name[%s]", (*meeting_req)->time_zone.daylight_name);
	EM_DEBUG_LOG_SEC(">>>>> (*meeting_req)->time_zone.daylight_time_start_date[%s]", asctime(&((*meeting_req)->time_zone.daylight_time_start_date)));
	EM_DEBUG_LOG_SEC(">>>>> (*meeting_req)->time_zone.daylight_bias[%d]", (*meeting_req)->time_zone.daylight_bias);
	ret = true;

FINISH_OFF:

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_update_meeting_request(char *multi_user_name, email_meeting_request_t* meeting_req, int transaction, int *err_code)
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
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

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

	EM_DEBUG_LOG_SEC("SQL(%s)", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
/*
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->mail_id[%d]", meeting_req->mail_id);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->meeting_response[%d]", meeting_req->meeting_response);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->start_time[%s]", asctime(&(meeting_req->start_time)));
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->end_time[%s]", asctime(&(meeting_req->end_time)));
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->location[%s]", meeting_req->location);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->global_object_id[%s]", meeting_req->global_object_id);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->time_zone.offset_from_GMT[%d]", meeting_req->time_zone.offset_from_GMT);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->time_zone.standard_name[%s]", meeting_req->time_zone.standard_name);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->time_zone.standard_time_start_date[%s]", asctime(&(meeting_req->time_zone.standard_time_start_date)));
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->time_zone.standard_bias[%d]", meeting_req->time_zone.standard_bias);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->time_zone.daylight_name[%s]", meeting_req->time_zone.daylight_name);
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->time_zone.daylight_time_start_date[%s]", asctime(&(meeting_req->time_zone.daylight_time_start_date)));
	EM_DEBUG_LOG_SEC(">>>>> meeting_req->time_zone.daylight_bias[%d]", meeting_req->time_zone.daylight_bias);
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
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	ret = true;


FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_meeting_request(char *multi_user_name, int account_id, int mail_id, int input_mailbox_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], input_mailbox_id[%d], transaction[%d], err_code[%p]", account_id, mail_id, input_mailbox_id, transaction, err_code);

	if (account_id < ALL_ACCOUNT || mail_id < 0) {
		EM_DEBUG_EXCEPTION(" account_id[%d], mail_id[%d]", account_id, mail_id);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	int and = false;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_meeting_tbl ");

	if (account_id != ALL_ACCOUNT) {		/*  NOT '0' means a specific account. '0' means all account */
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " WHERE account_id = %d", account_id);
		and = true;
	}
	if (mail_id > 0) {
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " %s mail_id = %d", (and ? "AND" : "WHERE"), mail_id);
		and = true;
	}
	if (input_mailbox_id > 0) {		/*  0 means all mailbox_id */
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1), " %s mailbox_id = '%d'",  (and ? "AND" : "WHERE"), input_mailbox_id);
	}

	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC void emstorage_free_meeting_request(email_meeting_request_t *meeting_req)
{
	EM_DEBUG_FUNC_BEGIN();

	if (!meeting_req) return;

	EM_SAFE_FREE(meeting_req->location);
	EM_SAFE_FREE(meeting_req->global_object_id);

   	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int emstorage_get_overflowed_mail_id_list(char *multi_user_name, int account_id, int input_mailbox_id, int mail_slot_size, int **mail_id_list, int *mail_id_count, int transaction, int *err_code)
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
		if (err_code)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mail_id FROM mail_tbl WHERE account_id = %d AND mailbox_id = %d ORDER BY date_time DESC LIMIT %d, 10000", account_id, input_mailbox_id, mail_slot_size);

	EM_DEBUG_LOG_SEC("query[%s].", sql_query_string);

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_mail_id_count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (!result_mail_id_count) {
		EM_DEBUG_LOG("No mail found...");
		ret = false;
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
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

	if (ret == true) {
		*mail_id_list = result_mail_id_list;
		*mail_id_count = result_mail_id_count;
	} else
		EM_SAFE_FREE(result_mail_id_list);

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_PROFILE_END(profile_emstorage_get_overflowed_mail_id_list);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_get_thread_id_by_mail_id(char *multi_user_name, int mail_id, int *thread_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], thread_id[%p], err_code[%p]", mail_id, thread_id, err_code);

	int rc = -1, ret = false;
	int err = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	char **result;
	int result_count = 0;

	if (mail_id == 0 || thread_id == NULL) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		if (err_code)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	memset(sql_query_string, 0, QUERY_SIZE);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT thread_id FROM mail_tbl WHERE mail_id = %d", mail_id);

	/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_count, 0, NULL); */
	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {err = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	if (!result_count) {
		EM_DEBUG_LOG("No mail found...");
		ret = false;
		err = EMAIL_ERROR_MAIL_NOT_FOUND;
		/* sqlite3_free_table(result); */
		goto FINISH_OFF;
	}

	_get_table_field_data_int(result, thread_id, 1);

	sqlite3_free_table(result);

	ret = true;

FINISH_OFF:

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_update_latest_thread_mail(char *multi_user_name,
														int account_id,
														int mailbox_id,
														int mailbox_type,
														int thread_id,
														int *updated_thread_id,
														int latest_mail_id,
														int thread_item_count,
														int noti_type,
														int transaction,
														int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mailbox_id [%d], thread_id[%d], updated_thread_id[%p], "
						"latest_mail_id [%d], thread_item_count[%d], err_code[%p]",
						account_id, mailbox_id, thread_id, updated_thread_id,
						latest_mail_id, thread_item_count, err_code);

	int rc = -1, ret = false;
	int err = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	char **result = NULL;
	int result_count = 0;

	if (thread_id == 0) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		if (err_code)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	if (thread_item_count == 0 && latest_mail_id == 0) {
		memset(sql_query_string, 0, QUERY_SIZE);
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mail_id, count(*) FROM (SELECT account_id, mail_id, thread_id, mailbox_id FROM mail_tbl ORDER BY date_time) WHERE account_id = %d AND thread_id = %d AND mailbox_id = %d", account_id, thread_id, mailbox_id);

		/*  rc = sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_count, 0, NULL); */
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &result_count, 0, NULL), rc);
		EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {err = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
			("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));
		EM_DEBUG_LOG("result_count[%d]", result_count);
		if (result_count == 0) {
			EM_DEBUG_LOG("No mail found...");
			ret = false;
			if (err_code)
				*err_code =  EMAIL_ERROR_MAIL_NOT_FOUND;
			sqlite3_free_table(result);
			return false;
		}

		_get_table_field_data_int(result, &latest_mail_id, 2);
		_get_table_field_data_int(result, &thread_item_count, 3);

		EM_DEBUG_LOG("latest_mail_id[%d]", latest_mail_id);
		EM_DEBUG_LOG("thread_item_count[%d]", thread_item_count);

		sqlite3_free_table(result);
	}

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, err);

	if (thread_item_count < 0) {
		memset(sql_query_string, 0, QUERY_SIZE);
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET thread_item_count = 0 WHERE account_id = %d AND thread_id = %d", account_id, thread_id);
		EM_DEBUG_LOG_SEC("query[%s]", sql_query_string);
		err = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (err != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", err);
				goto FINISH_OFF;
		}
	} else if (thread_id != latest_mail_id) {
		/* Initialize the thread id */
		memset(sql_query_string, 0, QUERY_SIZE);
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET thread_item_count = 0, thread_id = %d WHERE account_id = %d AND mailbox_id = %d AND thread_id = %d", latest_mail_id, account_id, mailbox_id, thread_id);
		err = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (err != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", err);
				goto FINISH_OFF;
		}

		/* update the thread item count */
		memset(sql_query_string, 0, QUERY_SIZE);
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET thread_item_count = %d WHERE account_id = %d AND mail_id = %d ", thread_item_count, account_id, latest_mail_id);
		err = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (err != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", err);
				goto FINISH_OFF;
		}
	} else {
		memset(sql_query_string, 0, QUERY_SIZE);
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET thread_item_count = %d WHERE account_id = %d AND mail_id = %d ", thread_item_count, account_id, latest_mail_id);
		EM_DEBUG_LOG_SEC("query[%s]", sql_query_string);
		err = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (err != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", err);
				goto FINISH_OFF;
		}
	}
	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, err);

	if (thread_id != latest_mail_id) {
		if (err == EMAIL_ERROR_NONE) {
			EM_DEBUG_LOG("noti_type[%d]", noti_type);

			if (latest_mail_id > 0 && thread_id > 0 && noti_type > 0) {
				char mailbox_id_str[25] = {0,};
				snprintf(mailbox_id_str, sizeof(mailbox_id_str), "%d", mailbox_id);
				if (!emcore_notify_storage_event(noti_type, thread_id, latest_mail_id, mailbox_id_str, account_id))
					EM_DEBUG_EXCEPTION(" emcore_notify_storage_eventfailed [NOTI_THREAD_ID_CHANGED] >>>> ");

				if (updated_thread_id) *updated_thread_id = latest_mail_id;
			}
		}
	} else if (thread_item_count >= 0) {
		if (err == EMAIL_ERROR_NONE) {
			char parameter_string[500] = {0,};
			SNPRINTF(parameter_string, sizeof(parameter_string), "%s%c%d", "thread_item_count", 0x01, latest_mail_id);
			if (!emcore_notify_storage_event(NOTI_MAIL_FIELD_UPDATE, account_id, EMAIL_MAIL_ATTRIBUTE_THREAD_ITEM_COUNT, parameter_string, thread_item_count))
				EM_DEBUG_EXCEPTION(" emcore_notify_storage_eventfailed [NOTI_MAIL_FIELD_UPDATE] >>>> ");
		}
	}

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_update_thread_id_of_mail(char *multi_user_name, int account_id, int mailbox_id, int mail_id, int thread_id, int thread_item_count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mailbox_id [%d], mail_id[%d], thread_id[%d], thread_item_count[%d], err_code[%p]", account_id, mailbox_id, mail_id, thread_id, thread_item_count, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	if (thread_id == 0) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		if (err_code)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, err);

	memset(sql_query_string, 0, QUERY_SIZE);
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "UPDATE mail_tbl SET thread_item_count = %d, thread_id = %d WHERE account_id = %d AND mail_id = %d", thread_item_count, thread_id, account_id, mail_id);
	err = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, err);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	memset(sql_query_string, 0x00 , sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "INSERT INTO mail_local_activity_tbl VALUES (?, ?, ?, ?, ?, ?, ?)");

	EM_DEBUG_LOG(">>>>> ACTIVITY ID [ %d ] ", local_activity->activity_id);
	EM_DEBUG_LOG(">>>>> MAIL ID [ %d ] ", local_activity->mail_id);
	EM_DEBUG_LOG(">>>>> ACCOUNT ID [ %d ] ", local_activity->account_id);
	EM_DEBUG_LOG(">>>>> ACTIVITY TYPE [ %d ] ", local_activity->activity_type);
	EM_DEBUG_LOG_SEC(">>>>> SERVER MAIL ID [ %s ] ", local_activity->server_mailid);
	EM_DEBUG_LOG(">>>>> SOURCE MAILBOX [ %s ] ", local_activity->src_mbox);
	EM_DEBUG_LOG(">>>>> DEST MAILBOX   [ %s ] ", local_activity->dest_mbox);

	EM_DEBUG_LOG_SEC(">>>> SQL STMT [ %s ] ", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG_SEC(">>>> SQL STMT [ %s ] ", sql_query_string);

	_bind_stmt_field_data_int(hStmt, i++, local_activity->activity_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->account_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->mail_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->activity_type);
	_bind_stmt_field_data_string(hStmt, i++ , (char *)local_activity->server_mailid, 0, SERVER_MAIL_ID_LEN_IN_MAIL_TBL);
	_bind_stmt_field_data_string(hStmt, i++ , (char *)local_activity->src_mbox, 0, MAILBOX_NAME_LEN_IN_MAIL_BOX_TBL);
	_bind_stmt_field_data_string(hStmt, i++ , (char *)local_activity->dest_mbox, 0, MAILBOX_NAME_LEN_IN_MAIL_BOX_TBL);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);

	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d, errmsg = %s.", rc, sqlite3_errmsg(local_db_handle)));

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

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
		if (err_code)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	if (activityid == ALL_ACTIVITIES) {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_local_activity_tbl WHERE account_id = %d order by activity_id", account_id);
	} else {
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_local_activity_tbl WHERE account_id = %d AND activity_id = %d ", account_id, activityid);
	}

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);



	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	col_index = 7;

	if (!(p_activity_tbl = (emstorage_activity_tbl_t*)em_malloc(sizeof(emstorage_activity_tbl_t) * count))) {
		EM_DEBUG_EXCEPTION(" em_mallocfailed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}


	for (i = 0; i < count; i++) {
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
		if (result[col_index] && EM_SAFE_STRLEN(result[col_index]) > 0)
			p_activity_tbl[i].server_mailid = EM_SAFE_STRDUP(result[col_index++]);
		else
			col_index++;

		EM_DEBUG_LOG("result[%d] - %s ", col_index, result[col_index]);
		if (result[col_index] && EM_SAFE_STRLEN(result[col_index]) > 0)
			p_activity_tbl[i].src_mbox = EM_SAFE_STRDUP(result[col_index++]);
		else
			col_index++;

		EM_DEBUG_LOG("result[%d] - %s ", col_index, result[col_index]);
		if (result[col_index] && EM_SAFE_STRLEN(result[col_index]) > 0)
			p_activity_tbl[i].dest_mbox = EM_SAFE_STRDUP(result[col_index++]);
		else
			col_index++;

	}

	if (result)
		sqlite3_free_table(result);

	ret = true;

FINISH_OFF:

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (ret == true) {
		*activity_list = p_activity_tbl;
		*select_num = count;
		EM_DEBUG_LOG(">>>> COUNT : %d >> ", count);
	} else if (p_activity_tbl != NULL) {
		emstorage_free_local_activity(&p_activity_tbl, count, NULL);
	}


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

	if (NULL == activity_id) {
		EM_DEBUG_EXCEPTION(" activity_id[%p]", activity_id);
		if (err_code)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	/*  increase unique id */

	sql = "SELECT max(rowid) FROM mail_local_activity_tbl;";

	/*  rc = sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL); n EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc); */
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL == result[1])
		rc = 1;
	else
		rc = atoi(result[1])+1;

	*activity_id = rc;

	if (result)
		sqlite3_free_table(result);

	ret = true;

	FINISH_OFF:

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;

}

INTERNAL_FUNC int emstorage_get_activity_id_list(char *multi_user_name, int account_id, int ** activity_id_list, int *activity_id_count, int lowest_activity_type, int highest_activity_type, int transaction, int *err_code)
{

	EM_DEBUG_FUNC_BEGIN();

	EM_DEBUG_LOG(" account_id[%d], activity_id_list[%p], activity_id_count[%p] err_code[%p]", account_id,  activity_id_list, activity_id_count, err_code);

	if (account_id <= 0 || NULL == activity_id_list || NULL == activity_id_count || lowest_activity_type <= 0 || highest_activity_type <= 0) {
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
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT distinct activity_id FROM mail_local_activity_tbl WHERE account_id = %d AND activity_type >= %d AND activity_type <= %d order by activity_id", account_id, lowest_activity_type, highest_activity_type);

	EM_DEBUG_LOG_SEC(" Query [%s]", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	col_index = 1;

	EM_DEBUG_LOG(" Activity COUNT : %d ... ", count);

	if (NULL == (activity_ids = (int *)em_malloc(sizeof(int) * count))) {
		EM_DEBUG_EXCEPTION(" em_mallocfailed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < count; i++) {
		activity_ids[i] = atoi(result[col_index]);
		col_index++;
		EM_DEBUG_LOG("activity_id %d", activity_ids[i]);
	}

	ret = true;

FINISH_OFF:


	if (ret == true) {
		*activity_id_count = count;
		*activity_id_list = activity_ids;
	} else if (activity_ids != NULL) /* Prevent defect - 216566 */
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
	} else {
		EM_SAFE_FREE(activity_id_list);
	}


	ret = true;

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

	if (!local_activity) {
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
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_local_activity_tbl ");

	EM_DEBUG_LOG_SEC(">>> Query [ %s ] ", sql_query_string);

	if (local_activity->account_id) {
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1),
		" WHERE account_id = %d ", local_activity->account_id);
		query_and = 1;
		query_where = 1;
	}

	if (local_activity->server_mailid) {
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1),
		" %s %s server_mailid = '%s' ", query_where ? "" : "WHERE", query_and ? "AND" : "", local_activity->server_mailid);
		query_and = 1;
		query_where = 1;
	}


	if (local_activity->mail_id) {
		EM_DEBUG_LOG(">>>> MAIL ID [ %d ] , ACTIVITY TYPE [%d ]", local_activity->mail_id, local_activity->activity_type);

		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1),
		" %s %s mail_id = %d  ", query_where ? "" : "WHERE", query_and ? "AND" : "", local_activity->mail_id);

		query_and = 1;
		query_where = 1;

	}

	if (local_activity->activity_type > 0) {
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1),
		" %s %s activity_type = %d ", query_where ? "" : "WHERE", query_and ? "AND" : "" , local_activity->activity_type);
	}

	if (local_activity->activity_id > 0) {
		SNPRINTF(sql_query_string + EM_SAFE_STRLEN(sql_query_string), sizeof(sql_query_string)-(EM_SAFE_STRLEN(sql_query_string)+1),
		" %s %s activity_id = %d ", query_where ? "" : "WHERE", query_and ? "AND" : "" , local_activity->activity_id);

	}

	EM_DEBUG_LOG_SEC(">>>>> Query [ %s ] ", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION(" no (matched) mailbox_name found...");
		err = EMAIL_ERROR_MAILBOX_NOT_FOUND;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

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
			for (; i < count; i++) {
				EM_SAFE_FREE(p[i].dest_mbox);
				EM_SAFE_FREE(p[i].src_mbox);
				EM_SAFE_FREE(p[i].server_mailid);
			}

			EM_SAFE_FREE(p);
			*local_activity_list = NULL;
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


static int _get_key_value_string_for_list_filter_rule(email_list_filter_rule_t *input_list_filter_rule, char **output_key_value_string)
{
	EM_DEBUG_FUNC_BEGIN("input_list_filter_rule [%p], output_key_value_string [%p]", input_list_filter_rule, output_key_value_string);

	int  ret = EMAIL_ERROR_NONE;
	char key_value_string[QUERY_SIZE] = { 0, };
	char *temp_key_value_1 = NULL;
	char *temp_key_value_2 = NULL;

	if (input_list_filter_rule == NULL || output_key_value_string == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	switch (input_list_filter_rule->target_attribute) {
	case EMAIL_MAIL_ATTRIBUTE_MAIL_ID:
	case EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID:
	case EMAIL_MAIL_ATTRIBUTE_MAILBOX_ID:
	case EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE:
	case EMAIL_MAIL_ATTRIBUTE_SERVER_MAIL_STATUS:
	case EMAIL_MAIL_ATTRIBUTE_REFERENCE_MAIL_ID:
	case EMAIL_MAIL_ATTRIBUTE_BODY_DOWNLOAD_STATUS:
	case EMAIL_MAIL_ATTRIBUTE_MAIL_SIZE:
	case EMAIL_MAIL_ATTRIBUTE_FILE_PATH_PLAIN:
	case EMAIL_MAIL_ATTRIBUTE_FILE_PATH_HTML:
	case EMAIL_MAIL_ATTRIBUTE_FILE_SIZE:
	case EMAIL_MAIL_ATTRIBUTE_FLAGS_SEEN_FIELD:
	case EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD:
	case EMAIL_MAIL_ATTRIBUTE_FLAGS_FLAGGED_FIELD:
	case EMAIL_MAIL_ATTRIBUTE_FLAGS_ANSWERED_FIELD:
	case EMAIL_MAIL_ATTRIBUTE_FLAGS_RECENT_FIELD:
	case EMAIL_MAIL_ATTRIBUTE_FLAGS_DRAFT_FIELD:
	case EMAIL_MAIL_ATTRIBUTE_FLAGS_FORWARDED_FIELD:
	case EMAIL_MAIL_ATTRIBUTE_DRM_STATUS:
	case EMAIL_MAIL_ATTRIBUTE_PRIORITY:
	case EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS:
	case EMAIL_MAIL_ATTRIBUTE_LOCK_STATUS:
	case EMAIL_MAIL_ATTRIBUTE_REPORT_STATUS:
	case EMAIL_MAIL_ATTRIBUTE_ATTACHMENT_COUNT:
	case EMAIL_MAIL_ATTRIBUTE_INLINE_CONTENT_COUNT:
	case EMAIL_MAIL_ATTRIBUTE_THREAD_ID:
	case EMAIL_MAIL_ATTRIBUTE_THREAD_ITEM_COUNT:
	case EMAIL_MAIL_ATTRIBUTE_MEETING_REQUEST_STATUS:
	case EMAIL_MAIL_ATTRIBUTE_MESSAGE_CLASS:
	case EMAIL_MAIL_ATTRIBUTE_DIGEST_TYPE:
	case EMAIL_MAIL_ATTRIBUTE_SMIME_TYPE:
	case EMAIL_MAIL_ATTRIBUTE_REMAINING_RESEND_TIMES:
	case EMAIL_MAIL_ATTRIBUTE_TAG_ID:
	case EMAIL_MAIL_ATTRIBUTE_EAS_DATA_LENGTH_TYPE:
		SNPRINTF(key_value_string, QUERY_SIZE, "%d", input_list_filter_rule->key_value.integer_type_value);
		break;

	case EMAIL_MAIL_ATTRIBUTE_MAILBOX_NAME:
	case EMAIL_MAIL_ATTRIBUTE_SUBJECT:
	case EMAIL_MAIL_ATTRIBUTE_SERVER_MAILBOX_NAME:
	case EMAIL_MAIL_ATTRIBUTE_SERVER_MAIL_ID:
	case EMAIL_MAIL_ATTRIBUTE_MESSAGE_ID:
	case EMAIL_MAIL_ATTRIBUTE_FROM:
	case EMAIL_MAIL_ATTRIBUTE_TO:
	case EMAIL_MAIL_ATTRIBUTE_CC:
	case EMAIL_MAIL_ATTRIBUTE_BCC:
	case EMAIL_MAIL_ATTRIBUTE_PREVIEW_TEXT:
		if (input_list_filter_rule->key_value.string_type_value == NULL) {
			EM_DEBUG_EXCEPTION("Invalid string_type_value [%p]", input_list_filter_rule->key_value.string_type_value);
			ret = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		temp_key_value_1 = input_list_filter_rule->key_value.string_type_value;

		temp_key_value_2 = em_replace_all_string(temp_key_value_1, "_", "\\_");
		temp_key_value_1 = em_replace_all_string(temp_key_value_2, "%", "\\%");

		if (input_list_filter_rule->rule_type == EMAIL_LIST_FILTER_RULE_INCLUDE)
			SNPRINTF(key_value_string, QUERY_SIZE, "\'%%%s%%\'", temp_key_value_1);
		else
			SNPRINTF(key_value_string, QUERY_SIZE, "\'%s\'", temp_key_value_1);
		break;

	case EMAIL_MAIL_ATTRIBUTE_DATE_TIME:
	case EMAIL_MAIL_ATTRIBUTE_SCHEDULED_SENDING_TIME:
	case EMAIL_MAIL_ATTRIBUTE_REPLIED_TIME:
	case EMAIL_MAIL_ATTRIBUTE_FORWARDED_TIME:
		SNPRINTF(key_value_string, QUERY_SIZE, "%d", (int)input_list_filter_rule->key_value.datetime_type_value);
		break;

	default:
		ret = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("Invalid target_attribute [%d]", input_list_filter_rule->target_attribute);
		break;
	}

	if (ret == EMAIL_ERROR_NONE && EM_SAFE_STRLEN(key_value_string) > 0) {
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

	if (input_list_filter_rule == NULL || output_string == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return  EMAIL_ERROR_INVALID_PARAM;
	}

	temp_field_name_string = emcore_get_mail_field_name_by_attribute_type(input_list_filter_rule->target_attribute);

	if (temp_field_name_string == NULL) {
		EM_DEBUG_EXCEPTION("Invalid target_attribute [%d]", input_list_filter_rule->target_attribute);
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (_get_key_value_string_for_list_filter_rule(input_list_filter_rule, &temp_key_value_string) != EMAIL_ERROR_NONE || temp_key_value_string == NULL) {
		EM_DEBUG_EXCEPTION("_get_key_value_string_for_list_filter_rule failed");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	length_field_name = EM_SAFE_STRLEN(temp_field_name_string);
	length_value      = EM_SAFE_STRLEN(temp_key_value_string);

	switch (input_list_filter_rule->target_attribute) {
	case EMAIL_MAIL_ATTRIBUTE_MAILBOX_NAME:
	case EMAIL_MAIL_ATTRIBUTE_SUBJECT:
	case EMAIL_MAIL_ATTRIBUTE_SERVER_MAILBOX_NAME:
	case EMAIL_MAIL_ATTRIBUTE_SERVER_MAIL_ID:
	case EMAIL_MAIL_ATTRIBUTE_MESSAGE_ID:
	case EMAIL_MAIL_ATTRIBUTE_FROM:
	case EMAIL_MAIL_ATTRIBUTE_TO:
	case EMAIL_MAIL_ATTRIBUTE_CC:
	case EMAIL_MAIL_ATTRIBUTE_BCC:
	case EMAIL_MAIL_ATTRIBUTE_PREVIEW_TEXT:
		is_alpha = 1;
		break;
	default:
		is_alpha = 0;
		break;
	}

	if (is_alpha == 1 && input_list_filter_rule->case_sensitivity == false) {
		length_field_name += strlen("UPPER() ");
		length_value      += strlen("UPPER() ");
		mod_field_name_string = em_malloc(sizeof(char) * length_field_name);
		if (mod_field_name_string == NULL) {
			EM_DEBUG_EXCEPTION("em_mallocfailed");
			EM_SAFE_FREE(temp_field_name_string);
			ret = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		mod_value_string = em_malloc(sizeof(char) * length_value);
		if (mod_value_string == NULL) {
			EM_DEBUG_EXCEPTION("em_mallocfailed");
			EM_SAFE_FREE(temp_field_name_string);
			ret = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		SNPRINTF(mod_field_name_string, length_field_name, "UPPER(%s)", temp_field_name_string);
		SNPRINTF(mod_value_string,      length_value, "UPPER(%s)", temp_key_value_string);
		EM_SAFE_FREE(temp_key_value_string);
	} else {
		mod_field_name_string = strdup(temp_field_name_string);
		mod_value_string      = temp_key_value_string;
	}

	switch (input_list_filter_rule->rule_type) {
	case EMAIL_LIST_FILTER_RULE_EQUAL:
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s = %s ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_NOT_EQUAL:
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s != %s ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_LESS_THAN:
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s < %s ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_GREATER_THAN:
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s > %s ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_LESS_THAN_OR_EQUAL:
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s <= %s ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_GREATER_THAN_OR_EQUAL:
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s >= %s ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_INCLUDE:
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s LIKE %s ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_IN:
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s IN (%s) ", mod_field_name_string, mod_value_string);
		break;
	case EMAIL_LIST_FILTER_RULE_NOT_IN:
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s NOT IN (%s) ", mod_field_name_string, mod_value_string);
		break;
	default:
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

static int _make_filter_attach_rule_string(char *multi_user_name, email_list_filter_rule_attach_t *input_list_filter_rule, char **output_string)
{
	EM_DEBUG_FUNC_BEGIN("input_list_filter_rule [%p], output_string [%p]", input_list_filter_rule, output_string);

	char *field_name_string = NULL;
	char  key_value_string[QUERY_SIZE] = {0,};
	char  result_rule_string[QUERY_SIZE] = {0,};
	int rc = -1;
	int count = 0;
	int query_size = 0;
	int cur_query = 0;
	int col_index = 0;
	int error = EMAIL_ERROR_NONE;
	char **result = NULL;
	char sql_query_string[QUERY_SIZE] = {0,};
	char *sql_query_string2 = NULL;
	sqlite3 *local_db_handle = NULL;
	int *mail_ids = NULL;

	if (input_list_filter_rule == NULL || output_string == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return  EMAIL_ERROR_INVALID_PARAM;
	}

	field_name_string = EM_SAFE_STRDUP("attachment_name");
	SNPRINTF(key_value_string, QUERY_SIZE, "%s", input_list_filter_rule->key_value.string_type_value);

	switch (input_list_filter_rule->rule_type) {

	case EMAIL_LIST_FILTER_RULE_INCLUDE:
		SNPRINTF(result_rule_string, QUERY_SIZE, "WHERE %s LIKE \'%%%s%%\' ", field_name_string, key_value_string);
		break;

	case EMAIL_LIST_FILTER_RULE_MATCH:
		SNPRINTF(result_rule_string, QUERY_SIZE, "WHERE %s MATCH \'%s\' ", field_name_string, key_value_string);
		break;

	default:
		EM_DEBUG_EXCEPTION("Invalid rule_type [%d]", input_list_filter_rule->rule_type);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(true);
	SNPRINTF(sql_query_string, QUERY_SIZE, "SELECT mail_id FROM mail_attachment_tbl %s", result_rule_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	col_index = 1;

	if (!count) {
		EM_DEBUG_LOG("No mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		*output_string = strdup("mail_id IN () ");
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_DEV(">>>> DATA ASSIGN START >>");
	int i = 0;
	if (!(mail_ids = (int *)em_malloc(sizeof(int) * count))) {
		EM_DEBUG_EXCEPTION("malloc for mail_ids failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < count; i++) {
		_get_table_field_data_int(result, &(mail_ids[i]), col_index++);
		EM_DEBUG_LOG_DEV(">>>> DATA ASSIGN [mail_id : %d] >>", mail_ids[i]);
	}

	EM_DEBUG_LOG_DEV(">>>> DATA ASSIGN END [count : %d] >>", count);
	sqlite3_free_table(result);
	EMSTORAGE_FINISH_READ_TRANSACTION(true);

//	sqlite3_db_release_memory(local_db_handle);


	query_size = (10 * count) + strlen("mail_id IN ()  ");

	sql_query_string2 = em_malloc(query_size);
	if (sql_query_string2 == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	cur_query += SNPRINTF_OFFSET(sql_query_string2, cur_query, query_size, "mail_id IN (");
	for (i = 0; i < count-1; i++) {
		cur_query += SNPRINTF_OFFSET(sql_query_string2, cur_query, query_size, "%d, ", mail_ids[i]);
	}
	cur_query += SNPRINTF_OFFSET(sql_query_string2, cur_query, query_size, "%d) ", mail_ids[count-1]);

	*output_string = strdup(sql_query_string2);
FINISH_OFF:

	EM_SAFE_FREE(mail_ids); /* prevent */
	EM_SAFE_FREE(sql_query_string2);
	EM_SAFE_FREE(field_name_string);
	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

static int _make_filter_fts_rule_string(char *multi_user_name, email_list_filter_rule_fts_t *input_list_filter_rule, char **output_string)
{
	EM_DEBUG_FUNC_BEGIN("input_list_filter_rule [%p], output_string [%p]", input_list_filter_rule, output_string);
	char *field_name_string = NULL;
	char key_value_string[QUERY_SIZE] = {0,};
	char  result_rule_string[QUERY_SIZE] = {0,};
	int rc = -1;
	int count = 0;
	int col_index = 0;
	int query_size = 0;
	int error = EMAIL_ERROR_NONE;
	char **result = NULL;
	char sql_query_string[QUERY_SIZE] = {0,};
	char *sql_query_string2 = NULL;
	sqlite3 *local_db_handle = NULL;
	int *mail_ids = NULL;

	if (input_list_filter_rule == NULL || output_string == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return  EMAIL_ERROR_INVALID_PARAM;
	}

	field_name_string = EM_SAFE_STRDUP("body_text");
	SNPRINTF(key_value_string, QUERY_SIZE, "%s", input_list_filter_rule->key_value.string_type_value);

	switch (input_list_filter_rule->rule_type) {

	case EMAIL_LIST_FILTER_RULE_INCLUDE:
		SNPRINTF(result_rule_string, QUERY_SIZE, "WHERE %s LIKE \'%%%s%%\' ", field_name_string, key_value_string);
		break;

	case EMAIL_LIST_FILTER_RULE_MATCH:
		SNPRINTF(result_rule_string, QUERY_SIZE, "WHERE %s MATCH \'%s\' ", field_name_string, key_value_string);
		break;

	default:
		EM_DEBUG_EXCEPTION("Invalid rule_type [%d]", input_list_filter_rule->rule_type);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(true);
	SNPRINTF(sql_query_string, QUERY_SIZE, "SELECT mail_id FROM mail_text_tbl %s", result_rule_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	col_index = 1;

	if (!count) {
		EM_DEBUG_LOG("No mail found...");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		*output_string = strdup("mail_id IN () ");
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_DEV(">>>> DATA ASSIGN START >>");
	int i = 0;

	if (!(mail_ids = (int *)em_malloc(sizeof(int) * count))) {
		EM_DEBUG_EXCEPTION("malloc for mail_ids failed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < count; i++) {
		_get_table_field_data_int(result, &(mail_ids[i]), col_index++);
		EM_DEBUG_LOG_DEV(">>>> DATA ASSIGN [mail_id : %d] >>", mail_ids[i]);
	}

	EM_DEBUG_LOG_DEV(">>>> DATA ASSIGN END [count : %d] >>", count);
	sqlite3_free_table(result);
	EMSTORAGE_FINISH_READ_TRANSACTION(true);

//	sqlite3_db_release_memory(local_db_handle);


	query_size = (10 * count) + strlen("mail_id IN ()  ");
	sql_query_string2 = em_malloc(query_size);
	if (sql_query_string2 == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	int cur_query = 0;
	cur_query += SNPRINTF_OFFSET(sql_query_string2, cur_query, query_size, "mail_id IN (");
	for (i = 0; i < count-1; i++) {
		cur_query += SNPRINTF_OFFSET(sql_query_string2, cur_query, query_size, "%d, ", mail_ids[i]);
	}
	cur_query += SNPRINTF_OFFSET(sql_query_string2, cur_query, query_size, "%d) ", mail_ids[count-1]);

	*output_string = strdup(sql_query_string2);

FINISH_OFF:
	EM_SAFE_FREE(mail_ids); /* prevent */
	EM_SAFE_FREE(sql_query_string2);
	EM_SAFE_FREE(field_name_string);
	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

static int _make_order_rule_string(char *multi_user_name, email_list_sorting_rule_t *input_sorting_rule, char **output_string)
{
	EM_DEBUG_FUNC_BEGIN("input_sorting_rule [%p], output_string [%p]", input_sorting_rule, output_string);

	char  result_rule_string[QUERY_SIZE] = { 0 , };
	int   ret = EMAIL_ERROR_NONE;

	emstorage_account_tbl_t *account_tbl_array = NULL;
	int count = 0;
	int i = 0;
	char *result_str = NULL;
	char *tmp_str1 = NULL;
	char *tmp_str2 = NULL;
	char query_per_account[QUERY_SIZE] = { 0 , };

	if (input_sorting_rule->force_boolean_check) {
		SNPRINTF(result_rule_string, QUERY_SIZE, "%s = 0 ", emcore_get_mail_field_name_by_attribute_type(input_sorting_rule->target_attribute));
	} else
		EM_SAFE_STRCPY(result_rule_string, emcore_get_mail_field_name_by_attribute_type(input_sorting_rule->target_attribute));

	switch (input_sorting_rule->sort_order) {
		case EMAIL_SORT_ORDER_ASCEND:
			EM_SAFE_STRCAT(result_rule_string, " ASC ");
			break;

		case EMAIL_SORT_ORDER_DESCEND:
			EM_SAFE_STRCAT(result_rule_string, " DESC ");
			break;

		case EMAIL_SORT_ORDER_NOCASE_ASCEND:
			EM_SAFE_STRCAT(result_rule_string, " COLLATE NOCASE ASC ");
			break;

		case EMAIL_SORT_ORDER_NOCASE_DESCEND:
			EM_SAFE_STRCAT(result_rule_string, " COLLATE NOCASE DESC ");
			break;

		case EMAIL_SORT_ORDER_TO_CCBCC:
			memset(result_rule_string, 0, QUERY_SIZE);
			if (input_sorting_rule->key_value.string_type_value)
				sqlite3_snprintf(QUERY_SIZE, result_rule_string,
						" CASE WHEN full_address_to LIKE \'%%%q%%\' THEN 1 ELSE 2 END ",
						input_sorting_rule->key_value.string_type_value);
			break;

		case EMAIL_SORT_ORDER_TO_CC_BCC:
			memset(result_rule_string, 0, QUERY_SIZE);
			if (input_sorting_rule->key_value.string_type_value)
				sqlite3_snprintf(QUERY_SIZE, result_rule_string,
						" CASE WHEN full_address_to LIKE \'%%%q%%\' THEN 1 WHEN full_address_cc LIKE \'%%%q%%\' THEN 2 ELSE 3 END ",
						input_sorting_rule->key_value.string_type_value, input_sorting_rule->key_value.string_type_value);
			break;

		case EMAIL_SORT_ORDER_TO_CCBCC_ALL:
			if (!emstorage_get_account_list(multi_user_name, &count, &account_tbl_array, true, false, NULL)) {
				EM_DEBUG_EXCEPTION("emstorage_get_account_list failed");
				goto FINISH_OFF;
			}

			if (!count) {
				EM_DEBUG_LOG("No account exist");
				ret = EMAIL_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}

			for (i = 0; i < count; i++) {
				if (i > 0 && result_str) {
					tmp_str2 = result_str;
					result_str = g_strconcat(tmp_str2, " OR ", NULL);
					EM_SAFE_FREE(tmp_str2);
				}

				memset(query_per_account, 0, QUERY_SIZE);
				snprintf(query_per_account, QUERY_SIZE,
						"(account_id = %d AND full_address_to LIKE \'%%%s%%\')",
						account_tbl_array[i].account_id, account_tbl_array[i].user_email_address);

				tmp_str1 = result_str;
				if (tmp_str1)
					result_str = g_strconcat(tmp_str1, query_per_account, NULL);
				else
					result_str = g_strdup(query_per_account);
				EM_SAFE_FREE(tmp_str1);
			}

			snprintf(result_rule_string, QUERY_SIZE,
					" CASE WHEN %s THEN 1 ELSE 2 END ", result_str);

			EM_SAFE_FREE(result_str);
			if (account_tbl_array)
				emstorage_free_account(&account_tbl_array, count, NULL);
			break;

		case EMAIL_SORT_ORDER_LOCALIZE_ASCEND:
			memset(result_rule_string, 0, QUERY_SIZE);
			sqlite3_snprintf(QUERY_SIZE, result_rule_string,
				" CASE WHEN %s GLOB \'[][~`!@#$%%^&*()_-+=|\\{}:;<>,.?/ ]*\' THEN 1 ELSE 2 END ASC, %s COLLATE NOCASE ASC ",
				emcore_get_mail_field_name_by_attribute_type(input_sorting_rule->target_attribute),
				emcore_get_mail_field_name_by_attribute_type(input_sorting_rule->target_attribute));
			break;

		case EMAIL_SORT_ORDER_LOCALIZE_DESCEND:
			memset(result_rule_string, 0, QUERY_SIZE);
			sqlite3_snprintf(QUERY_SIZE, result_rule_string,
				" CASE WHEN %s GLOB \'[][~`!@#$%%^&*()_-+=|\\{}:;<>,.?/ ]*\' THEN 1 ELSE 2 END DESC, %s COLLATE NOCASE DESC ",
				emcore_get_mail_field_name_by_attribute_type(input_sorting_rule->target_attribute),
				emcore_get_mail_field_name_by_attribute_type(input_sorting_rule->target_attribute));
			break;

		default:
			EM_DEBUG_EXCEPTION("Invalid sort_order [%d]", input_sorting_rule->sort_order);
			ret = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
	}

	*output_string = strdup(result_rule_string);

FINISH_OFF:
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_write_conditional_clause_for_getting_mail_list(char *multi_user_name, email_list_filter_t *input_filter_list, int input_filter_count, email_list_sorting_rule_t *input_sorting_rule_list, int input_sorting_rule_count, int input_start_index, int input_limit_count, char **output_conditional_clause)
{
	EM_DEBUG_FUNC_BEGIN("input_filter_list [%p], input_filter_count[%d], input_sorting_rule_list[%p], input_sorting_rule_count [%d], input_start_index [%d], input_limit_count [%d], output_conditional_clause [%p]", input_filter_list, input_filter_count, input_sorting_rule_list, input_sorting_rule_count, input_start_index, input_limit_count, output_conditional_clause);
	int ret = EMAIL_ERROR_NONE;
	int i = 0;
	int string_offset = 0;
	int query_size = 0;
	int new_query_size = 0;
	char *conditional_clause_string = NULL;
	char *result_string_for_a_item = NULL;

	if ((input_filter_count > 0 && !input_filter_list) || (input_sorting_rule_count > 0 && !input_sorting_rule_list) || output_conditional_clause == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	conditional_clause_string = em_malloc(QUERY_SIZE);
	if (conditional_clause_string == NULL) {
		EM_DEBUG_EXCEPTION("Memory is full");
		return EMAIL_ERROR_OUT_OF_MEMORY;
	}

	if (input_filter_count > 0) {
		query_size = QUERY_SIZE;
		g_strlcpy(conditional_clause_string, " WHERE ", QUERY_SIZE);

		for (i = 0; i < input_filter_count; i++) {
			switch (input_filter_list[i].list_filter_item_type) {
			case EMAIL_LIST_FILTER_ITEM_RULE:
				EM_DEBUG_LOG_DEV("[%d]list_filter_item_type is EMAIL_LIST_FILTER_ITEM_RULE", i);
				_make_filter_rule_string(&(input_filter_list[i].list_filter_item.rule), &result_string_for_a_item);
				break;

			case EMAIL_LIST_FILTER_ITEM_RULE_FTS:
				EM_DEBUG_LOG_DEV("[%d]list_filter_item_type is EMAIL_LIST_FILTER_ITEM_RULE_FTS", i);
				_make_filter_fts_rule_string(multi_user_name, &(input_filter_list[i].list_filter_item.rule_fts), &result_string_for_a_item);
				break;

			case EMAIL_LIST_FILTER_ITEM_RULE_ATTACH:
				EM_DEBUG_LOG_DEV("[%d]list_filter_item_type is EMAIL_LIST_FILTER_ITEM_RULE_ATTACH", i);
				_make_filter_attach_rule_string(multi_user_name, &(input_filter_list[i].list_filter_item.rule_attach), &result_string_for_a_item);
				break;

			case EMAIL_LIST_FILTER_ITEM_OPERATOR:
				EM_DEBUG_LOG_DEV("[%d]list_filter_item_type is EMAIL_LIST_FILTER_ITEM_OPERATOR", i);
				switch (input_filter_list[i].list_filter_item.operator_type) {
				case EMAIL_LIST_FILTER_OPERATOR_AND:
					result_string_for_a_item = strdup("AND ");
					break;
				case EMAIL_LIST_FILTER_OPERATOR_OR:
					result_string_for_a_item = strdup("OR ");
					break;
				case EMAIL_LIST_FILTER_OPERATOR_LEFT_PARENTHESIS:
					result_string_for_a_item = strdup(" (");
					break;
				case EMAIL_LIST_FILTER_OPERATOR_RIGHT_PARENTHESIS:
					result_string_for_a_item = strdup(") ");
					break;
				}
				break;

			default:
				EM_DEBUG_EXCEPTION("Invalid list_filter_item_type [%d]", input_filter_list[i].list_filter_item_type);
				ret = EMAIL_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}

			if (result_string_for_a_item == NULL) {
				EM_DEBUG_EXCEPTION("result_string_for_a_item is null");
				ret = EMAIL_ERROR_INVALID_PARAM;
				goto FINISH_OFF;
			}

			if (strlen(conditional_clause_string) + EM_SAFE_STRLEN(result_string_for_a_item) >= query_size) { /* prevent 34364 */
				EM_DEBUG_LOG("QUERY is too long");
				new_query_size = EM_SAFE_STRLEN(result_string_for_a_item) + EM_SAFE_STRLEN(conditional_clause_string) + QUERY_SIZE;
				conditional_clause_string = realloc(conditional_clause_string, new_query_size);
				if (conditional_clause_string == NULL) {
					EM_DEBUG_EXCEPTION("realloc failed");
					ret = EMAIL_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}

				query_size = new_query_size;
			}

			strcat(conditional_clause_string, result_string_for_a_item);
			EM_SAFE_FREE(result_string_for_a_item);
		}
	}

	if (input_sorting_rule_count > 0) {
		strcat(conditional_clause_string, "ORDER BY ");

		for (i = 0; i < input_sorting_rule_count; i++) {
			if ((ret = _make_order_rule_string(multi_user_name, &input_sorting_rule_list[i], &result_string_for_a_item)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("_make_order_rule_string failed. [%d]", ret);
				goto FINISH_OFF;
			}
			if (i > 0)
				strcat(conditional_clause_string, ", ");
			strcat(conditional_clause_string, result_string_for_a_item);
			EM_SAFE_FREE(result_string_for_a_item);
		}
	}

	if (input_start_index != -1 && input_limit_count != -1) {
		string_offset = strlen(conditional_clause_string);
		SNPRINTF_OFFSET(conditional_clause_string, string_offset, query_size, " LIMIT %d, %d", input_start_index, input_limit_count);
	}

	*output_conditional_clause = strdup(conditional_clause_string);

FINISH_OFF:
	EM_SAFE_FREE(result_string_for_a_item);
	EM_SAFE_FREE(conditional_clause_string);

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

	for (i = 0; i < input_filter_count; i++) {
		temp_filter_list = (*input_filter_list) + i;
		if (!temp_filter_list) {
			continue;
		}

		if (temp_filter_list->list_filter_item_type == EMAIL_LIST_FILTER_ITEM_RULE) {
			switch (temp_filter_list->list_filter_item.rule.target_attribute) {
			case EMAIL_MAIL_ATTRIBUTE_MAILBOX_NAME:
			case EMAIL_MAIL_ATTRIBUTE_SUBJECT:
			case EMAIL_MAIL_ATTRIBUTE_SERVER_MAILBOX_NAME:
			case EMAIL_MAIL_ATTRIBUTE_SERVER_MAIL_ID:
			case EMAIL_MAIL_ATTRIBUTE_MESSAGE_ID:
			case EMAIL_MAIL_ATTRIBUTE_FROM:
			case EMAIL_MAIL_ATTRIBUTE_TO:
			case EMAIL_MAIL_ATTRIBUTE_CC:
			case EMAIL_MAIL_ATTRIBUTE_BCC:
			case EMAIL_MAIL_ATTRIBUTE_FILE_PATH_PLAIN:
			case EMAIL_MAIL_ATTRIBUTE_FILE_PATH_HTML:
			case EMAIL_MAIL_ATTRIBUTE_PREVIEW_TEXT:
				EM_SAFE_FREE(temp_filter_list->list_filter_item.rule.key_value.string_type_value);
				break;
			default:
				break;
			}
		} else if (temp_filter_list->list_filter_item_type == EMAIL_LIST_FILTER_ITEM_RULE_FTS && temp_filter_list->list_filter_item.rule_fts.target_attribute == EMAIL_MAIL_TEXT_ATTRIBUTE_FULL_TEXT) {
				EM_SAFE_FREE(temp_filter_list->list_filter_item.rule_fts.key_value.string_type_value);
		} else if (temp_filter_list->list_filter_item_type == EMAIL_LIST_FILTER_ITEM_RULE_ATTACH && temp_filter_list->list_filter_item.rule_attach.target_attribute == EMAIL_MAIL_ATTACH_ATTRIBUTE_ATTACHMENT_NAME) {
				EM_SAFE_FREE(temp_filter_list->list_filter_item.rule_attach.key_value.string_type_value);
		}
	}

	EM_SAFE_FREE(*input_filter_list);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

/* Tasks --------------------------------------------------------------------------*/
INTERNAL_FUNC int emstorage_add_task(char *multi_user_name, email_task_type_t input_task_type, email_task_priority_t input_task_priority, char *input_task_parameter, int input_task_parameter_length, int input_transaction, int *output_task_id)
{
	EM_DEBUG_FUNC_BEGIN("input_task_type [%d] input_task_priority[%p], input_task_parameter[%p] input_task_parameter_length[%d] input_transaction[%d] output_task_id[%p]", input_task_type, input_task_priority, input_task_parameter, input_task_parameter_length, input_transaction, output_task_id);
	int ret = 0;
	int i = 0;
	int task_id = 0;
	int err = EMAIL_ERROR_NONE;
	int rc = -1;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = NULL;
	char *sql = "SELECT max(rowid) FROM mail_task_tbl;";
	char **result = NULL;

	if (input_task_parameter == NULL || output_task_id == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, input_transaction, err);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL == result[1])
		task_id = 1;
	else
		task_id = atoi(result[1])+1;

	*output_task_id = task_id;

	sqlite3_free_table(result);
	result = NULL;

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_task_tbl VALUES "
		"(	  "
		"    ? "  /*   task_id */
		"  , ? "  /*   task_type */
		"  , ? "  /*   task_status */
		"  , ? "  /*   task_priority */
		"  , ? "  /*   task_parameter_length */
		"  , ? "  /*   task_parameter */
		"  , ? "  /*   date_time */
		") ");

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {err = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG_SEC(">>>> SQL STMT [%s] ", sql_query_string);


	_bind_stmt_field_data_int(hStmt, i++, task_id);
	_bind_stmt_field_data_int(hStmt, i++, input_task_type);
	_bind_stmt_field_data_int(hStmt, i++, EMAIL_TASK_STATUS_WAIT);
	_bind_stmt_field_data_int(hStmt, i++, input_task_priority);
	_bind_stmt_field_data_int(hStmt, i++, input_task_parameter_length);
	_bind_stmt_field_data_blob(hStmt, i++, input_task_parameter, input_task_parameter_length);
	_bind_stmt_field_data_int(hStmt, i++, time(NULL));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);

	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {err = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {err = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d, errmsg = %s.", rc, sqlite3_errmsg(local_db_handle)));

	ret = (err == EMAIL_ERROR_NONE);

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, input_transaction, ret, err);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			err = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emstorage_delete_task(char *multi_user_name, int task_id, int transaction)
{
	EM_DEBUG_FUNC_BEGIN("task_id[%d], transaction[%d]", task_id, transaction);
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = NULL;

	if (task_id < 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, err);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_task_tbl WHERE task_id = %d", task_id);
	err = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", err);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, err);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emstorage_update_task_status(char *multi_user_name, int task_id, email_task_status_type_t task_status, int transaction)
{
	EM_DEBUG_FUNC_BEGIN("task_id[%d] task_status[%d] transaction[%d]", task_id, task_status, transaction);
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, err);

	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"UPDATE mail_task_tbl SET"
		" task_status = %d"
		" WHERE task_id = %d"
		, task_status
		, task_id);
	err = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", err);
			goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, err);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emstorage_query_task(char *multi_user_name,
									const char *input_conditional_clause,
									const char *input_ordering_clause,
									email_task_t **output_task_list,
									int *output_task_count)
{
	EM_DEBUG_FUNC_BEGIN("input_conditional_clause[%p], input_ordering_clause [%p], "
						"output_task_list[%p], output_task_count[%d]",
						input_conditional_clause, input_ordering_clause, output_task_list, output_task_count);
	int i = 0, count = 0, rc = -1;
	int cur_query = 0;
	int field_index = 0;
	int err = EMAIL_ERROR_NONE;
	email_task_t *task_item_from_tbl = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char *field_list = "task_id, task_type, task_status, task_priority, task_parameter_length, task_parameter ";
	char **result;
	sqlite3 *local_db_handle = NULL;
	DB_STMT hStmt = NULL;

	EM_IF_NULL_RETURN_VALUE(input_conditional_clause, false);
	EM_IF_NULL_RETURN_VALUE(output_task_count, false);

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	SNPRINTF_OFFSET(sql_query_string, cur_query, QUERY_SIZE,
					"SELECT COUNT(*) FROM mail_task_tbl %s", input_conditional_clause);
	EM_DEBUG_LOG_SEC("emstorage_query_mail_list : query[%s].", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {err = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	count = atoi(result[1]);
	sqlite3_free_table(result);

	EM_DEBUG_LOG("count = %d", rc);

	if (count == 0) {
		EM_DEBUG_EXCEPTION("no task found...");
		err = EMAIL_ERROR_TASK_NOT_FOUND;
		goto FINISH_OFF;
	}

	SNPRINTF_OFFSET(sql_query_string, cur_query, QUERY_SIZE,
					"SELECT %s FROM mail_task_tbl %s %s", field_list, input_conditional_clause, input_ordering_clause);
	EM_DEBUG_LOG_SEC("emstorage_query_mail_list : query[%s].", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG("After sqlite3_prepare_v2 hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {err = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {err = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (rc == SQLITE_DONE) {
		EM_DEBUG_EXCEPTION("no task found...");
		err = EMAIL_ERROR_TASK_NOT_FOUND;
		count = 0;
		goto FINISH_OFF;
	}

	if (!(task_item_from_tbl = (email_task_t*)em_malloc(sizeof(email_task_t) * count))) {
		EM_DEBUG_EXCEPTION("malloc for mail_list_item_from_tbl failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < count; i++) {
		/*  get recordset */
		field_index = 0;

		_get_stmt_field_data_int(hStmt, (int*)&(task_item_from_tbl[i].task_id), field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(task_item_from_tbl[i].task_type), field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(task_item_from_tbl[i].task_status), field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(task_item_from_tbl[i].task_priority), field_index++);
		_get_stmt_field_data_int(hStmt, (int*)&(task_item_from_tbl[i].task_parameter_length), field_index++);
		_get_stmt_field_data_blob(hStmt, (void**)&(task_item_from_tbl[i].task_parameter), field_index++);

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_LOG("after sqlite3_step(), i = %d, rc = %d.", i,  rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {err = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
	}

FINISH_OFF:

	if (err == EMAIL_ERROR_NONE) {
		if (output_task_list)
			*output_task_list = task_item_from_tbl;
		*output_task_count = count;
	} else {
		if (task_item_from_tbl) {
			for (i = 0; i < count; i++)
				EM_SAFE_FREE(task_item_from_tbl[i].task_parameter);

			free(task_item_from_tbl);
		}
	}

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			err = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emstorage_check_and_update_server_uid_by_message_id(char *multi_user_name, int account_id, email_mailbox_type_e input_mailbox_type, char *message_id, char *server_uid, int *mail_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id:[%d], mailbox_type:[%d], message_id:[%s], server_uid:[%s]", account_id, input_mailbox_type, message_id, server_uid);
	int err = EMAIL_ERROR_NONE;

	if (message_id == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	int rc = -1;
	int count = 0;
	int temp_mail_id = 0;
	int where_pararaph_length = 0;
	char *where_pararaph = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	char **result = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT mail_id from mail_tbl ");

	where_pararaph_length = EM_SAFE_STRLEN(message_id) + 100;
	where_pararaph = em_malloc(sizeof(char) * where_pararaph_length);
	if (where_pararaph == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (account_id != ALL_ACCOUNT)
		sqlite3_snprintf(where_pararaph_length, where_pararaph, "WHERE account_id = %d AND mailbox_type = %d AND message_id like '%q'", account_id, input_mailbox_type, message_id);
	else
		sqlite3_snprintf(where_pararaph_length, where_pararaph, "WHERE mailbox_type = %d AND message_id like '%q'", input_mailbox_type, message_id);

	if (strlen(sql_query_string) + strlen(where_pararaph) < QUERY_SIZE)
		strcat(sql_query_string, where_pararaph);

	EM_DEBUG_LOG_SEC("query[%s]", sql_query_string);

        EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, &count, 0, NULL), rc);
        EM_DEBUG_DB_EXEC((SQLITE_OK != rc && -1 != rc), {err = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
                ("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


        EM_DEBUG_LOG("Count of mails [%d]", count);

	if (count) {
		_get_table_field_data_int(result, &temp_mail_id, 1);
		EM_DEBUG_LOG("Searched mail_id [%d]", temp_mail_id);

		memset(sql_query_string, 0x00, QUERY_SIZE);
		sqlite3_snprintf(sizeof(sql_query_string), sql_query_string, "UPDATE mail_tbl set server_mail_id = '%q'", server_uid);

                if (strlen(sql_query_string) + strlen(where_pararaph) < QUERY_SIZE)
                        strcat(sql_query_string, where_pararaph);

		err = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", err);
			goto FINISH_OFF;
		}

	} else {
		err = EMAIL_ERROR_MAIL_NOT_FOUND;
	}

FINISH_OFF:

	sqlite3_free_table(result);
	result = NULL;


	EM_SAFE_FREE(where_pararaph);

	if (mail_id != NULL)
		*mail_id = temp_mail_id;

	EM_DEBUG_FUNC_END("err : [%d]", err);
	return err;
}
/* Tasks --------------------------------------------------------------------------*/

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
INTERNAL_FUNC int emstorage_add_auto_download_activity(char *multi_user_name, email_event_auto_download *local_activity, int *activity_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("local_activity[%p], activity_id[%p], transaction[%d], err_code[%p]", local_activity, activity_id, transaction, err_code);

	if (!local_activity || !activity_id) {
		EM_DEBUG_EXCEPTION("local_activity[%p], activity_id[%p]", local_activity, activity_id);
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);
	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string),
		"INSERT INTO mail_auto_download_activity_tbl VALUES "
		"("
		"? "  /* Activity ID */
		",?"  /* Status */
		",?"  /* Account ID */
		",?"  /* Local Mail ID */
		",?"  /* Server mail ID */
		",?"  /* Mailbox ID*/
		",?"  /* Multi USER NAME */
		") ");

	char *sql = "SELECT max(rowid) FROM mail_auto_download_activity_tbl;";
	char **result = NULL;


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);

	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql, rc, sqlite3_errmsg(local_db_handle)));

	if (NULL == result[1]) rc = 1;
	else rc = atoi(result[1])+1;
	sqlite3_free_table(result);
	result = NULL;

	*activity_id = local_activity->activity_id = rc;

	EM_DEBUG_LOG_SEC(">>>>> ACTIVITY ID [%d], MAIL ID [%d], SERVER MAIL ID [%lu]",
			local_activity->activity_id, local_activity->mail_id, local_activity->server_mail_id);

	if (local_activity->mailbox_id)
		EM_DEBUG_LOG(" MAILBOX ID [%d]", local_activity->mailbox_id);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	_bind_stmt_field_data_int(hStmt, i++, local_activity->activity_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->status);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->account_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->mail_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->server_mail_id);
	_bind_stmt_field_data_int(hStmt, i++, local_activity->mailbox_id);
	_bind_stmt_field_data_string(hStmt, i++, (char *)local_activity->multi_user_name, 0, MAX_USER_NAME_LENGTH);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);

	EM_DEBUG_DB_EXEC((rc == SQLITE_FULL), {error = EMAIL_ERROR_MAIL_MEMORY_FULL; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d, errmsg = %s.", rc, sqlite3_errmsg(local_db_handle)));

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);
	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_delete_auto_download_activity(char *multi_user_name, int account_id, int mail_id, int activity_id, int transaction, int *err_code)
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
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	if (activity_id == 0)
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_auto_download_activity_tbl WHERE account_id = %d AND mail_id = %d", account_id, mail_id);
	else
		SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_auto_download_activity_tbl WHERE account_id = %d AND activity_id = %d", account_id, activity_id);

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
		goto FINISH_OFF;
	}

	/*  validate activity existence */
	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION("No matching activity found");
		error = EMAIL_ERROR_DATA_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emstorage_delete_all_auto_download_activity(char *multi_user_name, int account_id, int transaction, int *err_code)
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_auto_download_activity_tbl WHERE account_id = %d", account_id);

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION("No matching activities found in mail_auto_download_activity_tbl");
		error = EMAIL_ERROR_DATA_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_delete_auto_download_activity_by_mailbox(char *multi_user_name, int account_id, int mailbox_id, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_id[%d], transaction[%d], err_code[%p]", account_id, mailbox_id, transaction, err_code);

	if (account_id < FIRST_ACCOUNT_ID || mailbox_id < 0) {
		EM_DEBUG_EXCEPTION("account_id[%d], mailbox_id[%d], transaction[%d], err_code[%p]", account_id, mailbox_id, transaction, err_code);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int rc = -1;
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "DELETE FROM mail_auto_download_activity_tbl WHERE account_id = %d AND mailbox_id = %d", account_id, mailbox_id);

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_EXCEPTION("No matching activities found in mail_auto_download_activity_tbl");
		error = EMAIL_ERROR_DATA_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_auto_download_activity(char *multi_user_name, int account_id, int input_mailbox_id, email_event_auto_download **event_start, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], event_start[%p], err_code[%p]", account_id, event_start, err_code);

	if (account_id < FIRST_ACCOUNT_ID || !event_start || input_mailbox_id <= 0 || !count) {
		EM_DEBUG_EXCEPTION("account_id[%d], event_start[%p], input_mailbox_id[%d], count[%p], err_code[%p]", account_id, event_start, input_mailbox_id, count, err_code);

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
	email_event_auto_download *event_list = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(*) FROM mail_auto_download_activity_tbl WHERE account_id = %d AND mailbox_id = '%d' order by activity_id", account_id, input_mailbox_id);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
		("SQL(%s) sqlite3_get_table fail:%d -%s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	*count = atoi(result[1]);
	sqlite3_free_table(result);

	EM_DEBUG_LOG_SEC("Query = [%s]", sql_query_string);

	if (!*count) {
		EM_DEBUG_LOG("No matched activity found in mail_auto_download_activity_tbl");
		error = EMAIL_ERROR_MAIL_NOT_FOUND;
		ret = true;
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("Activity Count = %d", *count);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT * FROM mail_auto_download_activity_tbl WHERE account_id = %d AND mailbox_id = '%d' order by activity_id", account_id, input_mailbox_id);

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG(" Bbefore sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (!(event_list = (email_event_auto_download *)em_malloc(sizeof(email_event_auto_download)*(*count)))) {
		EM_DEBUG_EXCEPTION("Malloc failed");

		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < (*count); i++) {
		_get_stmt_field_data_int(hStmt, &(event_list[i].activity_id), ACTIVITY_ID_IDX_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, &(event_list[i].status), STATUS_IDX_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, &(event_list[i].account_id), ACCOUNT_ID_IDX_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, &(event_list[i].mail_id), MAIL_ID_IDX_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, (int *)&(event_list[i].server_mail_id), SERVER_MAIL_ID_IDX_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL);
		_get_stmt_field_data_int(hStmt, &(event_list[i].mailbox_id), MAILBOX_ID_IDX_MAIL_AUTO_DOWNLOAD_ACTIVITY_TBL);

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
	}

	ret = true;

FINISH_OFF:

	if (true == ret)
	  *event_start = event_list;
	else {
		EM_SAFE_FREE(event_list);
		*event_start = NULL;
		*count = 0;
	}

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_auto_download_activity_count(char *multi_user_name, int *activity_count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("activity_count[%p], err_code[%p]", activity_count, err_code);

	if (!activity_count || !err_code) {
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(*) FROM mail_auto_download_activity_tbl;");

	EM_DEBUG_LOG_DEV(" Query [%s]", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG_DEV("before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	_get_stmt_field_data_int(hStmt, activity_count, 0);

	EM_DEBUG_LOG("counts of activities in activity table [%d]", *activity_count);

	ret = true;

FINISH_OFF:

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_auto_download_account_list(char *multi_user_name, int **account_list, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_list[%p], count[%p] err_code[%p]", account_list, count, err_code);

	if (!account_list || !count) {
		EM_DEBUG_EXCEPTION("account_list[%p], count[%p]", account_list, count);
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char *sql = "SELECT count(distinct account_id) FROM mail_auto_download_activity_tbl";
	char **result;
	int i = 0, rc = -1;
	int *result_account_list = NULL;
	DB_STMT hStmt = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
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

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT distinct account_id FROM mail_auto_download_activity_tbl");

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);

	EM_DEBUG_LOG("Before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	if (NULL == (result_account_list = (int *)em_malloc(sizeof(int)*(*count)))) {
		EM_DEBUG_EXCEPTION(" em_mallocfailed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < (*count); i++) {
		_get_stmt_field_data_int(hStmt, result_account_list + i, 0);

		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
		EM_DEBUG_LOG("account id -> %d", result_account_list[i]);
	}

	ret = true;

FINISH_OFF:

	if (ret == true)
		*account_list = result_account_list;
	else
		EM_SAFE_FREE(result_account_list);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_auto_download_mailbox_list(char *multi_user_name, int account_id, int **mailbox_list, int *count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_list[%p], count[%p] err_code[%p]", account_id, mailbox_list, count, err_code);

	if (account_id < FIRST_ACCOUNT_ID || !mailbox_list || !count) {
		EM_DEBUG_EXCEPTION("account_id[%d], mailbox_list[%p], count[%p]", account_id, mailbox_list, count);
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);

	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(distinct mailbox_id) FROM mail_auto_download_activity_tbl WHERE account_id = %d order by mailbox_id", account_id);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_get_table(local_db_handle, sql_query_string, &result, NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {error = EMAIL_ERROR_DB_FAILURE; sqlite3_free_table(result); goto FINISH_OFF; },
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

	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT distinct mailbox_id FROM mail_auto_download_activity_tbl WHERE account_id = %d order by mailbox_id", account_id);

	EM_DEBUG_LOG_SEC(" Query [%s]", sql_query_string);


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);


	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));


	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	mbox_list = (int *)em_malloc(sizeof(int)*(*count)); /* prevent */
	if (mbox_list == NULL) {
		EM_DEBUG_EXCEPTION(" em_mallocfailed...");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < (*count); i++) {
		_get_stmt_field_data_int(hStmt, mbox_list + i, 0);
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);

		EM_DEBUG_DB_EXEC((rc != SQLITE_ROW && rc != SQLITE_DONE), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
			("sqlite3_step fail:%d", rc));
		EM_DEBUG_LOG("mbox_list %d", mbox_list[i]);
	}

	ret = true;

FINISH_OFF:

	if (ret == true)
		*mailbox_list = mbox_list;
	else
		EM_SAFE_FREE(mbox_list);

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_get_auto_download_activity_count_by_mailbox(char *multi_user_name, int account_id, int input_mailbox_id, int *activity_count, int transaction, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], activity_count[%p], err_code[%p]", account_id, activity_count, err_code);

	if (account_id < FIRST_ACCOUNT_ID || !activity_count || !err_code) {
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

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_READ_TRANSACTION(transaction);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));
	SNPRINTF(sql_query_string, sizeof(sql_query_string), "SELECT count(*) FROM mail_auto_download_activity_tbl WHERE account_id = %d and mailbox_id = '%d'", account_id, input_mailbox_id);

	EM_DEBUG_LOG_SEC(" Query [%s]", sql_query_string);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_prepare_v2(local_db_handle, sql_query_string, EM_SAFE_STRLEN(sql_query_string), &hStmt, NULL), rc);
	EM_DEBUG_LOG("before sqlite3_prepare hStmt = %p", hStmt);
	EM_DEBUG_DB_EXEC((SQLITE_OK != rc), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("SQL(%s) sqlite3_prepare fail:(%d) %s", sql_query_string, rc, sqlite3_errmsg(local_db_handle)));

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_step(hStmt), rc);
	EM_DEBUG_DB_EXEC((rc != SQLITE_ROW), {error = EMAIL_ERROR_DB_FAILURE; goto FINISH_OFF; },
		("sqlite3_step fail:%d", rc));

	_get_stmt_field_data_int(hStmt, activity_count, 0);

	EM_DEBUG_LOG("count of activities in activity table [%d]", *activity_count);

	ret = true;

FINISH_OFF:

	if (hStmt != NULL) {
		rc = sqlite3_finalize(hStmt);
		hStmt = NULL;
		if (rc != SQLITE_OK) {
			EM_DEBUG_EXCEPTION("sqlite3_finalize error [%d]", rc);
			error = EMAIL_ERROR_DB_FAILURE;
		}
	}

	EMSTORAGE_FINISH_READ_TRANSACTION(transaction);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emstorage_update_auto_download_activity(char *multi_user_name, char *old_server_uid, char *new_server_uid, char *mailbox_name, int mailbox_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("old_server_uid[%s], new_server_uid[%s], mailbox_id[%d]", old_server_uid, new_server_uid, mailbox_id);

	int rc = -1, ret = false;
	int error = EMAIL_ERROR_NONE;
	char sql_query_string[QUERY_SIZE] = {0, };
	int transaction = true;

	if (!old_server_uid || !new_server_uid || (!mailbox_name && mailbox_id < 0)) {
		EM_DEBUG_EXCEPTION("Invalid parameters");
		error = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_START_WRITE_TRANSACTION(multi_user_name, transaction, error);
	memset(sql_query_string, 0x00, sizeof(sql_query_string));

	if (mailbox_id > 0)
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			 "UPDATE mail_auto_download_activity_tbl SET server_mail_id = %s , mailbox_id ='%d' WHERE server_mail_id = %s ", new_server_uid, mailbox_id, old_server_uid);
	else if (mailbox_name)
		SNPRINTF(sql_query_string, sizeof(sql_query_string),
			 "UPDATE mail_auto_download_activity_tbl SET server_mail_id = %s WHERE server_mail_id = %s ", new_server_uid, old_server_uid);

	EM_DEBUG_LOG_SEC("Query [%s]", sql_query_string);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
	if (error != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emstorage_exec_query_by_prepare_v2 failed:[%d]", error);
			goto FINISH_OFF;
	}

	rc = sqlite3_changes(local_db_handle);
	if (rc == 0) {
		EM_DEBUG_LOG("No matching found in mail_auto_download_activity_tbl");
	}

	ret = true;

FINISH_OFF:

	EMSTORAGE_FINISH_WRITE_TRANSACTION(multi_user_name, transaction, ret, error);

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

#endif

#ifdef __FEATURE_UPDATE_DB_TABLE_SCHEMA__

typedef struct {
	char *column_name;
	char *column_type;
} email_column_info_t;

static int get_column_information_from_table_callback(void *arg1, int argc, char **argv, char **input_column_name)
{
	EM_DEBUG_FUNC_BEGIN("arg1[%p] argc[%d] argv[%p] column_name[%p]", arg1, argc, argv, input_column_name);

	int i = 0;
	int validated = 0;
	char *column_name = NULL;
	char *column_type = NULL;
	GList *new_list = *((GList**)arg1);
	email_column_info_t *column_info_item = NULL;

	for (i = 0; i < argc; ++i) {
		/* EM_DEBUG_LOG("%s = %s", input_column_name[i], argv[i]); */
		if (EM_SAFE_STRCMP(input_column_name[i], "name") == 0) {
			if (column_name)
				EM_SAFE_FREE(column_name);

			column_name = EM_SAFE_STRDUP(argv[i]);
			validated = 1;
		} else if (EM_SAFE_STRCMP(input_column_name[i], "type") == 0) {
			if (column_type)
				EM_SAFE_FREE(column_type);

			column_type = EM_SAFE_STRDUP(argv[i]);
		}
	}

	if (validated) {
		EM_DEBUG_LOG("column_name[%s] column_type[%s]", column_name, column_type);
		column_info_item = em_malloc(sizeof(email_column_info_t));
        if (column_info_item == NULL) {
            EM_DEBUG_EXCEPTION("em_mallocfailed");
            goto FINISH_OFF;
        }

		column_info_item->column_name = EM_SAFE_STRDUP(column_name);
		column_info_item->column_type = EM_SAFE_STRDUP(column_type);
		new_list = g_list_append(new_list, (gpointer)column_info_item);
		*((GList**)arg1) = new_list;
	}

FINISH_OFF:

    EM_SAFE_FREE(column_name);
    EM_SAFE_FREE(column_type);

	EM_DEBUG_FUNC_END();
	return 0;
}

static int emstorage_get_column_information_from_table(char *multi_user_name, const char *input_table_name, GList **output_column_info)
{
	EM_DEBUG_FUNC_BEGIN("input_table_name[%p] output_column_info[%p]", input_table_name, output_column_info);
	int err = EMAIL_ERROR_NONE;
	int result_from_sqlite = 0;
	char *error_message_from_sqlite = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };
	GList *new_list = NULL;
	sqlite3 *local_db_handle = emstorage_get_db_connection(multi_user_name);

	SNPRINTF(sql_query_string, QUERY_SIZE, "pragma table_info(%s);", input_table_name);

	result_from_sqlite = sqlite3_exec(local_db_handle, sql_query_string, get_column_information_from_table_callback, &new_list, &error_message_from_sqlite);

	if (result_from_sqlite != SQLITE_OK)
		EM_DEBUG_EXCEPTION("sqlite3_exec returns [%d]", result_from_sqlite);

	EM_DEBUG_LOG("new_list[%p] output_column_info[%p]", new_list, output_column_info);

	if (new_list && output_column_info) {
		EM_DEBUG_LOG("g_list_length[%d]", g_list_length(new_list));
		*output_column_info = new_list;
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static  int emstorage_create_renamed_table(char *multi_user_name, char **input_full_query, int input_query_index, char *input_source_table_name, char *input_new_table_name)
{
	EM_DEBUG_FUNC_BEGIN("input_full_query [%p] input_query_index[%d] input_source_table_name[%p] input_new_table_name[%p]", input_full_query, input_query_index, input_source_table_name, input_new_table_name);
	int error = EMAIL_ERROR_NONE;
	int rc = -1;
	sqlite3 *local_db_handle = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	if (input_full_query == NULL || input_source_table_name == NULL || input_new_table_name == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; }, ("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));

	EM_DEBUG_LOG("[%s] will be replaced by [%s]", input_source_table_name, input_new_table_name);

	EM_SAFE_STRNCPY(sql_query_string, input_full_query[input_query_index], sizeof(sql_query_string)-1); /*prevent 21984*/
	reg_replace(sql_query_string, input_source_table_name, input_new_table_name);

	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
FINISH_OFF:

	if (error == EMAIL_ERROR_NONE) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} else {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "rollback", NULL, NULL, NULL), rc);
	}

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

static int emstorage_add_column(char *multi_user_name, char *input_table_name, email_column_info_t *input_new_column)
{
	EM_DEBUG_FUNC_BEGIN("input_table_name[%p] input_new_column[%p]", input_table_name, input_new_column);
	int error = EMAIL_ERROR_NONE;
	int rc = -1;
	sqlite3 *local_db_handle = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	if (input_table_name == NULL || input_new_column == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; }, ("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
	SNPRINTF(sql_query_string, QUERY_SIZE, "ALTER TABLE %s ADD COLUMN %s %s;", input_table_name, input_new_column->column_name, input_new_column->column_type);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
FINISH_OFF:

	if (error == EMAIL_ERROR_NONE) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} else {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "rollback", NULL, NULL, NULL), rc);
	}

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

static int emstorage_drop_table(char *multi_user_name, char *input_table_name)
{
	EM_DEBUG_FUNC_BEGIN("input_table_name[%p]", input_table_name);
	int error = EMAIL_ERROR_NONE;
	int rc = -1;
	sqlite3 *local_db_handle = NULL;
	char sql_query_string[QUERY_SIZE] = {0, };

	if (input_table_name == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	local_db_handle = emstorage_get_db_connection(multi_user_name);

	EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "BEGIN;", NULL, NULL, NULL), rc);
	EM_DEBUG_DB_EXEC(SQLITE_OK != rc, {goto FINISH_OFF; }, ("SQL(BEGIN EXCLUSIVE) exec fail:%d -%s", rc, sqlite3_errmsg(local_db_handle)));
	SNPRINTF(sql_query_string, QUERY_SIZE, "DROP TABLE %s;", input_table_name);
	error = emstorage_exec_query_by_prepare_v2(local_db_handle, sql_query_string);
FINISH_OFF:

	if (error == EMAIL_ERROR_NONE) {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "END;", NULL, NULL, NULL), rc);
	} else {
		EMSTORAGE_PROTECTED_FUNC_CALL(sqlite3_exec(local_db_handle, "rollback", NULL, NULL, NULL), rc);
	}

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

gint glist_compare_column_name(gconstpointer old_column_info, gconstpointer new_column_info)
{
	EM_DEBUG_FUNC_BEGIN("old_column_info[%p] new_column_info[%p]", old_column_info, new_column_info);
	email_column_info_t *left_one  = (email_column_info_t*)old_column_info;
	email_column_info_t *right_one = (email_column_info_t*)new_column_info;

	if (old_column_info == NULL || new_column_info == NULL)
		return -1;

	return EM_SAFE_STRCMP((char*)left_one->column_name, (char*)right_one->column_name);
}

INTERNAL_FUNC int emstorage_update_db_table_schema(char *multi_user_name)
{
	EM_DEBUG_FUNC_BEGIN();
	int i = 0;
	int j = 0;
	int error = EMAIL_ERROR_NONE;
	int query_len = 0;
	email_column_info_t *new_column_info = NULL;
	email_column_info_t *p_column_info = NULL;
	char **create_table_query = NULL;
	GList *found_data = NULL;
	GList *column_list_of_old_table = NULL;
	GList *column_list_of_new_table = NULL;
	char table_names[CREATE_TABLE_MAX][2][50] = { { "mail_account_tbl", "mail_account_tbl_new" },
		{ "mail_box_tbl", "mail_box_tbl_new" },
		{ "mail_read_mail_uid_tbl", "mail_read_mail_uid_tbl_new" },
		{ "mail_rule_tbl", "mail_rule_tbl_new" },
		{ "mail_tbl", "mail_tbl_new" },
		{ "mail_attachment_tbl", "mail_attachment_tbl_new" },
#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
		{ "mail_partial_body_activity_tbl", "mail_partial_body_activity_tbl_new" },
#else
		{ "", "" },
#endif
		{ "mail_meeting_tbl", "mail_meeting_tbl_new" },
#ifdef __FEATURE_LOCAL_ACTIVITY__
		{ "mail_local_activity_tbl", "mail_local_activity_tbl_new" },
#else
		{ "", "" },
#endif
		{ "mail_certificate_tbl", "mail_certificate_tbl_new" },
		{ "mail_task_tbl", "mail_task_tbl_new" },
#ifdef __FEATURE_BODY_SEARCH__
		{ "mail_text_tbl", "mail_text_tbl_new" },
#else
		{ "", "" },
#endif

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
		{ "mail_auto_download_activity_tbl", "mail_auto_download_activity_tbl_new" }
#else
		{ "", "" }
#endif

	};

	error = emcore_load_query_from_file((char *)EMAIL_SERVICE_CREATE_TABLE_QUERY_FILE_PATH, &create_table_query, &query_len);

	if (error != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_load_sql_from_file failed [%d]", error);
		goto FINISH_OFF;
	}

	if (query_len < CREATE_TABLE_MAX) {
		EM_DEBUG_EXCEPTION("SQL string array length is difference from CREATE_TABLE_MAX");
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	for (i = CREATE_TABLE_MAIL_ACCOUNT_TBL; i < CREATE_TABLE_MAX; i++) {
		EM_DEBUG_LOG("table [%s] new_table [%s]", table_names[i][0], table_names[i][1]);
		if (EM_SAFE_STRLEN(table_names[i][0]) && EM_SAFE_STRLEN(table_names[i][1])) {
			/* Check existing of _new table */
			emstorage_drop_table(multi_user_name, table_names[i][1]);
			error = emstorage_create_renamed_table(multi_user_name, create_table_query, i, table_names[i][0], table_names[i][1]);
			if (error != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emstorage_create_renamed_table failed [%d]", error);
				goto FINISH_OFF;
			}

			emstorage_get_column_information_from_table(multi_user_name, table_names[i][0], &column_list_of_old_table);
			emstorage_get_column_information_from_table(multi_user_name, table_names[i][1], &column_list_of_new_table);

			/* Compare fields and add new field */
			for (j = 0; j < g_list_length(column_list_of_new_table); j++) {
				new_column_info = (email_column_info_t*)g_list_nth_data(column_list_of_new_table, j);
				found_data = g_list_find_custom(column_list_of_old_table, (gconstpointer)new_column_info, glist_compare_column_name);
				if (found_data == NULL) {
					/* add new field*/
					emstorage_add_column(multi_user_name, table_names[i][0], new_column_info);
				}
			}

			emstorage_drop_table(multi_user_name, table_names[i][1]);
		} else
			EM_DEBUG_LOG("Skipped");
	}

FINISH_OFF:
	if (create_table_query) {
		int i = 0;
		for (i = 0; i < query_len; i++) {
			if (create_table_query[i]) {
				EM_SAFE_FREE(create_table_query[i]);
			}
		}
		EM_SAFE_FREE(create_table_query);
	}

	found_data = g_list_first(column_list_of_old_table);
	while (found_data != NULL) {
		p_column_info = (email_column_info_t *)found_data->data;
		EM_SAFE_FREE(p_column_info->column_name);
		EM_SAFE_FREE(p_column_info->column_type);
		EM_SAFE_FREE(p_column_info);

		found_data = g_list_next(found_data);
	}
	g_list_free(column_list_of_old_table);

	found_data = g_list_first(column_list_of_new_table);
	while (found_data != NULL) {
		p_column_info = (email_column_info_t *)found_data->data;
		EM_SAFE_FREE(p_column_info->column_name);
		EM_SAFE_FREE(p_column_info->column_type);
		EM_SAFE_FREE(p_column_info);

		found_data = g_list_next(found_data);
	}
	g_list_free(column_list_of_new_table);

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}
#endif /* __FEATURE_UPDATE_DB_TABLE_SCHEMA__ */

/*EOF*/
