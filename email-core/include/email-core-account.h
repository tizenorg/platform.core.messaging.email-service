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
 * File :  email-core-account.h
 * Desc :  Account Management
 * * Auth : 
 * * History : 
 *    2010.08.25  :  created
 *****************************************************************************/
#ifndef _EMAIL_CORE_ACCOUNT_H_
#define _EMAIL_CORE_ACCOUNT_H_

#include "email-types.h"
#include "email-internal-types.h"
#include "email-storage.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

INTERNAL_FUNC int emcore_validate_account(int account_id, int *err_code);

INTERNAL_FUNC int emcore_validate_account_with_account_info(emf_account_t *account, int *err_code);

INTERNAL_FUNC int emcore_create_account(emf_account_t *account, int *err_code);

INTERNAL_FUNC int emcore_delete_account_(int account_id, int *err_code);

INTERNAL_FUNC int emcore_free_account(emf_account_t **account_list, int count, int *err_code);

INTERNAL_FUNC int emcore_init_account_reference();

INTERNAL_FUNC int emcore_refresh_account_reference();

INTERNAL_FUNC int emcore_free_account_reference();

INTERNAL_FUNC emf_account_t *emcore_get_account_reference(int account_id);

INTERNAL_FUNC int emcore_get_account_reference_list(emf_account_t **account_list, int *count, int *err_code);

INTERNAL_FUNC int emcore_query_server_info(const char* domain_name, emf_server_info_t **result_server_info);

INTERNAL_FUNC int emcore_free_server_info(emf_server_info_t **target_server_info);

INTERNAL_FUNC int emcore_save_default_account_id(int input_account_id);

INTERNAL_FUNC int emcore_load_default_account_id(int *output_account_id);


#ifdef __FEATURE_BACKUP_ACCOUNT__
INTERNAL_FUNC int emcore_backup_accounts(const char *file_path, int *error_code);

INTERNAL_FUNC int emcore_restore_accounts(const char *file_path, int *error_code);
#endif /*   __FEATURE_BACKUP_ACCOUNT_ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*_EMAIL_CORE_ACCOUNT_H_*/

