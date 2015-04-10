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


#ifndef __EMAIL_API_RULE_H__
#define __EMAIL_API_RULE_H__

#include "email-types.h"

/**
 * @file email-api-rule.h
 * @brief This file contains the data structures and interfaces of Rule related Functionality provided by email-service.
 *
 * @{
 * @code
 *  #include "email-api-rule.h"
 *  bool
 *  other_app_invoke_uniform_api_sample(int* error_code)
 *  {
 *      int err = EMAIL_ERROR_NONE;
 *      email_rule_t*  rule = NULL;
 *      int filter_id = 1;
 *      int count = 0;
 *
 *      // Gets information of filtering
 *      printf("Enter filter Id:\n");
 *      scanf("%d",&filter_id);
 *
 *      if(EMAIL_ERROR_NONE == email_get_rule (filter_id,&rule))
 *          //success
 *      else
 *          //failure
 *
 *      // Get all filterings
 *      if(EMAIL_ERROR_NONE == email_get_rule_list(&rule,&count))
 *          //success
 *      else
 *          //failure
 *
 *
 *      // Adds a filter information
 *      if(EMAIL_ERROR_NONE == email_add_rule (rule))
 *          //success
 *      else
 *          //failure
 *      err = email_free_rule (&rule,1);
 *
 *      // Changes a filter information
 *      if(EMAIL_ERROR_NONE == email_update_rule (filter_id,rule))
 *          //success
 *      else
 *          //failure
 *      err = email_free_rule (&rule,1);
 *
 *      // Deletes a filter information
 *      printf("Enter filter Id:\n");
 *      scanf("%d",&filter_id);
 *
 *      if(EMAIL_ERROR_NONE == email_delete_rule (filter_id))
 *          //success
 *      else
 *          //failure
 *
 *      // Free allocated memory
 *      if(EMAIL_ERROR_NONE == email_free_rule (&rule,1))
 *          //success
 *      else
 *          //failure
 *
 *  }
 *
 * @endcode
 * @}
 */

/**
 * @internal
 * @ingroup EMAIL_SERVICE_FRAMEWORK
 * @defgroup EMAIL_SERVICE_RULE_MOUDLE Rule API
 * @brief Rule API is a set of operations to manage email rules like add, get, delete or update rule related details.
 *
 * @section EMAIL_SERVICE_RULE_MOUDLE_HEADER Required Header
 *   \#include <email-api-rule.h>
 *
 * @section EMAIL_SERVICE_RULE_MOUDLE_OVERVIEW Overview
 * Rule API is a set of operations to manage email rules like add, get, delete or update rule related details.
 */

/**
 * @internal
 * @addtogroup EMAIL_SERVICE_RULE_MOUDLE
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Gets a filter rule.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  filter_id      The filter ID
 * @param[out] filtering_set  The filter rule
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @retval  #EMAIL_ERROR_INVALID_PARAM  Invalid argument
 *
 * @see #email_rule_t
 */
EXPORT_API int email_get_rule(int filter_id, email_rule_t** filtering_set);

/**
 * @brief Gets all filter rules.
 * @details This function gives all the filter rules already set before by user.
 *          This will provide total number of filter rules available and information of all rules.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[out] filtering_set  The filtering rules (possibly @c NULL)
 * @param[out] count          The count of returned filters (possibly @c 0)
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @retval  EMAIL_ERROR_INVALID_PARAM  Invalid argument
 *
 * @see #email_rule_t
 */
EXPORT_API int email_get_rule_list(email_rule_t** filtering_set, int* count);

/**
 * @brief Adds a filter rule.
 * @details This function is invoked if a user wants to add a new filter rule.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  filtering_set  The pointer of adding a filter structure
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @retval  EMAIL_ERROR_INVALID_PARAM  Invalid argument
 *
 * @see #email_rule_t
 */
EXPORT_API int email_add_rule(email_rule_t* filtering_set);

/**
 * @brief Changes a filter rule.
 * @details This function will update the existing filter rule with new information.
 *
 * @param[in]  filter_id  The original filter ID
 * @param[in]  new_set    The information of new filter
 
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @retval  EMAIL_ERROR_INVALID_PARAM  Invalid argument
 *
 * @see #email_rule_t
 */
EXPORT_API int email_update_rule(int filter_id, email_rule_t* new_set);

/**
 * @brief Deletes a filter rule.
 * @details This function will delete the existing filter information by the specified filter ID.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  filter_id  The filter ID
 *
 * @return #EMAIL_ERROR_NONE on success,
 *         otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @retval  EMAIL_ERROR_INVALID_PARAM  Invalid argument
 */
EXPORT_API int email_delete_rule(int filter_id);

/**
 * @brief Frees allocated memory.
 *
 * @since_tizen 2.3
 * @privlevel N/P
 *
 * @param[in] filtering_set  The pointer of pointer of filter structure for memory freeing
 * @param[in] count          The count of filter
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @retval  EMAIL_ERROR_INVALID_PARAM  Invalid argument
 *
 * @see #email_rule_t
 */
EXPORT_API int email_free_rule (email_rule_t** filtering_set, int count);

/**
 * @brief Deletes a filter rule.
 * @details This function will delete the existing filter information by the specified filter ID.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  filter_id  The filter ID
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @retval  EMAIL_ERROR_INVALID_PARAM  Invalid argument
 */
EXPORT_API int email_apply_rule(int filter_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * @}
 */


#endif /* __EMAIL_API_RULE_H__ */
