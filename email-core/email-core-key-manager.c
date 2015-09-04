/*
*  email-service
*
* Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Sunghyun Kwon <sh0701.kwon@samsung.com>
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

/*
 * email-core-task-manager.c
 *
 *  Created on: 2015. 7. 24.
 *      Author: sh0701.kwon@samsung.com
 */

#include <ckmc/ckmc-manager.h>

#include "email-core-utils.h"
#include "email-debug-log.h"
#include "email-utilities.h"

/* Adding '/' method for system daemon */
static char *add_shared_owner_prefix(const char *name)
{
	EM_DEBUG_FUNC_BEGIN();
	char *ckm_alias = NULL;

	if (name == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return NULL;
	}

	ckm_alias = g_strconcat(ckmc_label_shared_owner, ckmc_label_name_separator, name, NULL);

	EM_DEBUG_FUNC_END();
	return ckm_alias;
}

/* Add new data */
INTERNAL_FUNC int emcore_add_password_in_key_manager(char *data_name, char *stored_data)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int ckmc_ret = CKMC_ERROR_NONE;
	char *alias = NULL;
	ckmc_policy_s email_policy;
	ckmc_raw_buffer_s email_data;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (data_name == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}
	
//	alias = add_shared_owner_prefix(data_name);
	alias = g_strdup(data_name);
	EM_DEBUG_LOG("alias : [%s]", alias);

	email_policy.password = NULL;
	email_policy.extractable = true;

//	EM_DEBUG_LOG("stored_data : [%s]", stored_data);

	email_data.data = (unsigned char *)stored_data;
	email_data.size = strlen(stored_data);

	ckmc_ret = ckmc_save_data(alias, email_data, email_policy);
	if (ckmc_ret != CKMC_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("ckmc_save_data failed [%d][%s]", errno, EM_STRERROR(errno_buf));
		err = EMAIL_ERROR_SECURED_STORAGE_FAILURE;
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_SAFE_FREE(alias);

	EM_DEBUG_FUNC_END();
	return err;
}

/* Get the stored data */
INTERNAL_FUNC int emcore_get_password_in_key_manager(char *data_name, char **stored_data)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int ckmc_ret = CKMC_ERROR_NONE;
	char *alias = NULL;
	ckmc_raw_buffer_s *email_data = NULL;

	if (data_name == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

//	alias = add_shared_owner_prefix(data_name); 
	alias = g_strdup(data_name);
	EM_DEBUG_LOG("alias : [%s]", alias);

	ckmc_ret = ckmc_get_data(alias, NULL, &email_data);
	if (ckmc_ret != CKMC_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("ckmc_get_data failed : [%d]", ckmc_ret);
		err = EMAIL_ERROR_SECURED_STORAGE_FAILURE;
		goto FINISH_OFF;
	}

//	EM_DEBUG_LOG("stored_data : [%s]", email_data->data);
//	EM_DEBUG_LOG("stored_data length : [%d]", email_data->size);

FINISH_OFF:

	if (stored_data) {
		if (email_data) *stored_data = g_strndup((const gchar *)email_data->data, email_data->size);
	}

	if (email_data)
		ckmc_buffer_free(email_data);

	EM_SAFE_FREE(alias);

	EM_DEBUG_FUNC_END();
	return err;
}

/* Remove the stored data */
INTERNAL_FUNC int emcore_remove_password_in_key_manager(char *data_name)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int ckmc_ret = CKMC_ERROR_NONE;
	char *alias = NULL;

	if (data_name == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

//	alias = add_shared_owner_prefix(data_name);  
	alias = g_strdup(data_name);
	EM_DEBUG_LOG("alias : [%s]", alias);

	ckmc_ret = ckmc_remove_alias(alias);
	if (ckmc_ret != CKMC_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("ckmc_remove_alias failed : [%d]", ckmc_ret);
		err = EMAIL_ERROR_SECURED_STORAGE_FAILURE;
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_SAFE_FREE(alias);
	EM_DEBUG_FUNC_END();
	return err;
}

INTERNAL_FUNC int emcore_get_certificate_in_key_manager(char *alias, char *password, 
														const unsigned char **cert_data, 
														int *cert_size)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

	if (alias == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	int ckmc_ret = CKMC_ERROR_NONE;
	unsigned char *p_cert_data = NULL;
	ckmc_cert_s *output_cert = NULL;

	ckmc_ret = ckmc_get_cert(alias, password, &output_cert);
	if (ckmc_ret != CKMC_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("ckmc_get_cert failed : [%d]", ckmc_ret);
		err = EMAIL_ERROR_SECURED_STORAGE_FAILURE;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("Cert size : [%d]", output_cert->cert_size);
	EM_DEBUG_LOG("Cert format : [%d]", output_cert->data_format);
	EM_DEBUG_LOG_DEV("Cert string : [%s]", output_cert->raw_cert);

	p_cert_data = em_malloc(output_cert->cert_size + 1);
	if (p_cert_data == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memcpy(p_cert_data, output_cert->raw_cert, output_cert->cert_size);

	*cert_data = p_cert_data;
	*cert_size = output_cert->cert_size;

FINISH_OFF:

	if (output_cert)
		ckmc_cert_free(output_cert);

	if (err != EMAIL_ERROR_NONE) {
		EM_SAFE_FREE(p_cert_data);
	}

	EM_DEBUG_FUNC_END();
	return err;
}
