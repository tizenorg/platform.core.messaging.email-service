/*
*  email-service
*
* Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Sunghyun Kwon <sh0701.kwon@samsung.com>, Minsoo Kim <minnsoo.kim@samsung.com>
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

/**
 *
 * This file contains functionality related to cynara(privilege)
 * to interact with email-service.
 * @file	email-core-cynara.c
 * @author	sh0701.kwon@samsung.com
 * @version	0.1
 * @brief 	This file contains functionality to provide cynara support in email-service. 
 */

#include <pthread.h>
#include <cynara-error.h>
#include <cynara-client.h>
#include <cynara-session.h>
#include <cynara-creds-commons.h>

#include "email-debug-log.h"
#include "email-utilities.h"

typedef struct _cynara_info_t {
	cynara *email_cynara;
	enum cynara_client_creds client_method;
	enum cynara_user_creds user_method;
} cynara_info_t;

static cynara_info_t *cynara_info = NULL;
pthread_mutex_t cynara_mutex = PTHREAD_MUTEX_INITIALIZER;

INTERNAL_FUNC int emcore_init_cynara()
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = CYNARA_API_SUCCESS;
	int err = EMAIL_ERROR_NONE;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	ENTER_CRITICAL_SECTION(cynara_mutex);
	cynara_info = (cynara_info_t *)em_malloc(sizeof(cynara_info));
	if (cynara_info == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	ret = cynara_initialize(&(cynara_info->email_cynara), NULL);
	if (ret != CYNARA_API_SUCCESS) {
		cynara_strerror(ret, errno_buf, ERRNO_BUF_SIZE);
		EM_DEBUG_EXCEPTION("cynara_initialize failed : [%d], error : [%s]", 
							ret,
							errno_buf);
		err = EMAIL_ERROR_NOT_INITIALIZED;
		goto FINISH_OFF;
	}

	ret = cynara_creds_get_default_client_method(&(cynara_info->client_method));
	if (ret != CYNARA_API_SUCCESS) {
		cynara_strerror(ret, errno_buf, ERRNO_BUF_SIZE);
		EM_DEBUG_EXCEPTION("cynara_creds_get_default_client_method failed : [%d], error : [%s]", 
							ret,
							errno_buf);
		err = EMAIL_ERROR_NOT_INITIALIZED;
		goto FINISH_OFF;
	}

	ret = cynara_creds_get_default_user_method(&(cynara_info->user_method));
	if (ret != CYNARA_API_SUCCESS) {
		cynara_strerror(ret, errno_buf, ERRNO_BUF_SIZE);
		EM_DEBUG_EXCEPTION("cynara_creds_get_default_user_method failed : [%d], error : [%s]", 
							ret,
							errno_buf);
		err = EMAIL_ERROR_NOT_INITIALIZED;
		goto FINISH_OFF;
	}

FINISH_OFF:

	LEAVE_CRITICAL_SECTION(cynara_mutex);

	EM_DEBUG_FUNC_END();
	return err;
}

INTERNAL_FUNC void emcore_finish_cynara()
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = CYNARA_API_SUCCESS;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (cynara_info == NULL) {
		EM_DEBUG_EXCEPTION("cynara did not initialize");
		return;
	}

	ENTER_CRITICAL_SECTION(cynara_mutex);
	ret = cynara_finish(cynara_info->email_cynara);
	if (ret != CYNARA_API_SUCCESS) {
		cynara_strerror(ret, errno_buf, ERRNO_BUF_SIZE);
		EM_DEBUG_EXCEPTION("cynara_finish failed : [%d], error : [%s]", 
							ret, 
							errno_buf);
	}
	EM_SAFE_FREE(cynara_info);
	LEAVE_CRITICAL_SECTION(cynara_mutex);

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int emcore_check_privilege(int socket_fd)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = CYNARA_API_SUCCESS;
	int	err = EMAIL_ERROR_NONE;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (socket_fd < 0) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	if (cynara_info->email_cynara == NULL) {
		err = emcore_init_cynara();
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_init_cynara failed : [%d]", err);
			return err;
		}
	}
	
	err = EMAIL_ERROR_PERMISSION_DENIED;

	pid_t client_pid = 0;
	char *client_uid = NULL;
	char *client_smack = NULL;
	char *client_session = NULL;

	ENTER_CRITICAL_SECTION(cynara_mutex);

	ret = cynara_creds_socket_get_client(socket_fd, cynara_info->client_method, &client_smack);
	if (ret != CYNARA_API_SUCCESS) {
		cynara_strerror(ret, errno_buf, ERRNO_BUF_SIZE);
		EM_DEBUG_EXCEPTION("cynara_creds_socket_get_client failed : [%d], error : [%s]",
							ret,
							errno_buf);
		goto FINISH_OFF;
	}

	ret = cynara_creds_socket_get_user(socket_fd, cynara_info->user_method, &client_uid);
	if (ret != CYNARA_API_SUCCESS) {
		cynara_strerror(ret, errno_buf, ERRNO_BUF_SIZE);
		EM_DEBUG_EXCEPTION("cynara_creds_socket_get_user failed : [%d], error : [%s]", 
							ret, 
							errno_buf);
		goto FINISH_OFF;
	}

	ret = cynara_creds_socket_get_pid(socket_fd, &client_pid);
	if (ret != CYNARA_API_SUCCESS) {
		cynara_strerror(ret, errno_buf, ERRNO_BUF_SIZE);
		EM_DEBUG_EXCEPTION("cynara_creds_socket_get_pid failed : [%d], error : [%s]", 
							ret, 
							errno_buf);
		goto FINISH_OFF;
	}

	client_session = cynara_session_from_pid(client_pid);
	if (client_session == NULL) {
		cynara_strerror(ret, errno_buf, ERRNO_BUF_SIZE);
		EM_DEBUG_EXCEPTION("cynara_session_from_pid failed error : [%s]", 
							errno_buf);
		goto FINISH_OFF;
	}

	ret = cynara_check(cynara_info->email_cynara, client_smack, client_session, client_uid, 
					"http://tizen.org/privilege/email");
	if (ret != CYNARA_API_ACCESS_ALLOWED) {
		cynara_strerror(ret, errno_buf, ERRNO_BUF_SIZE);
		EM_DEBUG_EXCEPTION("cynara_check failed : [%d], error : [%s]", 
							ret,
							errno_buf);
		goto FINISH_OFF;
	}

	err = EMAIL_ERROR_NONE;

FINISH_OFF:

	LEAVE_CRITICAL_SECTION(cynara_mutex);

	EM_SAFE_FREE(client_uid);
	EM_SAFE_FREE(client_smack);
	EM_SAFE_FREE(client_session);

	EM_DEBUG_FUNC_END();
	return err;
}