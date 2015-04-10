/*
*  email-service
*
* Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
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
 * This file contains functionality related to KNOX
 * to interact with email-service.
 * @file	email-core-cynara.c
 * @author	
 * @version	0.1
 * @brief 	This file contains functionality to provide cynara support in email-service. 
 */

#include <email-utilities.h>
#include <cynara-error.h>
#include <cynara-client.h>
#include <cynara-session.h>
#include <cynara-creds-commons.h>

#include "email-debug-log.h"

cynara *email_cynara = NULL;

INTERNAL_FUNC void emcore_init_cynara()
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = CYNARA_API_SUCCESS;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	ret = cynara_initialize(&email_cynara, NULL);
	if (ret != CYNARA_API_SUCCESS) {
		EM_DEBUG_EXCEPTION("cynara_initialize failed : [%d], error : [%s][%d]", 
							ret, 
							EM_STRERROR(errno_buf), 
							errno);
	}
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void emcore_finish_cynara()
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = CYNARA_API_SUCCESS;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	ret = cynara_finish(email_cynara);
	if (ret != CYNARA_API_SUCCESS) {
		EM_DEBUG_EXCEPTION("cynara_finish failed : [%d], error : [%s][%d]", 
							ret, 
							EM_STRERROR(errno_buf), 
							errno);
	}
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int emcore_check_privilege(int socket_fd, int client_pid)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = CYNARA_API_SUCCESS;
	int err = EMAIL_ERROR_NONE;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (socket_fd < 0 || client_pid <= 0) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	if (email_cynara == NULL) {
		ret = cynara_initialize(&email_cynara, NULL);
		if (ret != CYNARA_API_SUCCESS) {
			EM_DEBUG_EXCEPTION("cynara_initialize failed : [%d], error : [%s][%d]", 
								ret, 
								EM_STRERROR(errno_buf), 
								errno);
			goto FINISH_OFF;
		}
	}

	char *client_uid = NULL;
	char *client_smack = NULL;
	char *client_session = NULL;

	ret = cynara_creds_socket_get_client(socket_fd, CLIENT_METHOD_SMACK, &client_smack);
	if (ret != CYNARA_API_SUCCESS) {
		EM_DEBUG_EXCEPTION("cynara_creds_socket_get_client failed : [%d], error : [%s][%d]",
							ret,
							EM_STRERROR(errno_buf),
							errno);
		goto FINISH_OFF;
	}

	ret = cynara_creds_socket_get_user(socket_fd, USER_METHOD_UID, &client_uid);
	if (ret != CYNARA_API_SUCCESS) {
		EM_DEBUG_EXCEPTION("cynara_creds_socket_get_user failed : [%d], error : [%s][%d]", 
							ret, 
							EM_STRERROR(errno_buf), 
							errno);
		goto FINISH_OFF;
	}

	client_session = cynara_session_from_pid(client_pid);
	if (client_session == NULL) {
		EM_DEBUG_EXCEPTION("cynara_session_from_pid failed error : [%s][%d]", 
							EM_STRERROR(errno_buf), 
							errno);
		goto FINISH_OFF;
	}

	ret = cynara_check(email_cynara, client_smack, client_session, client_uid, 
					"http://tizen.org/privilege/email");
	if (ret != CYNARA_API_ACCESS_ALLOWED) {
		EM_DEBUG_EXCEPTION("cynara_check failed : [%d]", ret);
		goto FINISH_OFF;
	}
/*
	ret = cynara_check(email_cynara, client_smack, client_session, client_uid, 
					"http://tizen.org/privilege/email.admin");
	if (ret != CYNARA_API_ACCESS_ALLOWED) {
		EM_DEBUG_EXCEPTION("cynara_check failed : [%d]", ret);
		goto FINISH_OFF;
	}
*/
FINISH_OFF:

	if (ret != CYNARA_API_ACCESS_ALLOWED) {
		err = EMAIL_ERROR_PERMISSION_DENIED;
	}

	EM_SAFE_FREE(client_uid);
	EM_SAFE_FREE(client_smack);
	EM_SAFE_FREE(client_session);

	EM_DEBUG_FUNC_END();
	return err;
}
