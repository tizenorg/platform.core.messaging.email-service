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
 * File :  em-core-account.h
 * Desc :  Account Management
 * * Auth : 
 * * History : 
 *    2010.08.25  :  created
 *****************************************************************************/
#ifndef _EM_CORE_ACCOUNT_H_
#define _EM_CORE_ACCOUNT_H_

#include "emf-types.h"
#include "em-core-types.h"
#include "em-storage.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

EXPORT_API emf_account_t *em_core_get_account_reference(int account_id);

EXPORT_API int em_core_account_validate(int account_id, int *err_code);

EXPORT_API int em_core_account_delete(int account_id, int *err_code);

EXPORT_API int em_core_account_create(emf_account_t *account, int *err_code);

EXPORT_API int em_core_init_account_reference();

EXPORT_API int em_core_refresh_account_reference();

EXPORT_API int em_core_free_account_reference();

EXPORT_API int em_core_account_free(emf_account_t **account_list, int count, int *err_code);

EXPORT_API int em_core_account_get_list_refer(emf_account_t **account_list, int *count, int *err_code);

EXPORT_API int em_core_account_validate_with_account_info(emf_account_t *account, int *err_code);

EXPORT_API int em_core_query_server_info(const char* domain_name, emf_server_info_t **result_server_info);

EXPORT_API int em_core_free_server_info(emf_server_info_t **target_server_info);

EXPORT_API int em_core_save_default_account_id(int input_account_id);

EXPORT_API int em_core_load_default_account_id(int *output_account_id);


#ifdef __FEATURE_BACKUP_ACCOUNT__
EXPORT_API int em_core_backup_accounts(const char *file_path, int *error_code);

EXPORT_API int em_core_restore_accounts(const char *file_path, int *error_code);
#endif /*   __FEATURE_BACKUP_ACCOUNT_ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*_EM_CORE_ACCOUNT_H_*/

