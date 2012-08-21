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


#ifndef __EMAIL_API_RULE_H__
#define __EMAIL_API_RULE_H__

#include "email-types.h"

/**
* @defgroup EMAIL_SERVICE Email Service
* @{
*/

/**
* @ingroup EMAIL_SERVICE
* @defgroup EMAIL_API_RULE Email Rule API
* @{
*/

 /**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with email-service.
 * @file		email-api-rule.h
 * @author	Kyuho Jo <kyuho.jo@samsung.com>
 * @author	Sunghyun Kwon <sh0701.kwon@samsung.com>
 * @version	0.1
 * @brief 		This file contains the data structures and interfaces of Rule related Functionality provided by
 *			email-service .
 * @{

 * @code
 * 	#include "email-api-rule.h"
 *	bool
 *	other_app_invoke_uniform_api_sample(int* error_code)
 *	{
 *		int err = EMAIL_ERROR_NONE;
 *		email_rule_t*  rule = NULL;
 *		int filter_id = 1;
 *		int count = 0;
 *
 *		// Get a information of filtering
 *		printf("Enter filter Id:\n");
 *		scanf("%d",&filter_id);
 *
 *		if(EMAIL_ERROR_NONE == email_get_rule (filter_id,&rule))
 *			//success
 *		else
 *			//failure
 *
 *		// Get all filterings
 *		if(EMAIL_ERROR_NONE == email_get_rule_list(&rule,&count))
 *			//success
 *		else
 *			//failure
 *
 *
 *		// Add a filter information
 *		if(EMAIL_ERROR_NONE == email_add_rule (rule))
 *			//success
 *		else
 *			//failure
 *		err = email_free_rule (&rule,1);
 *
 *		// Change a filter information
 *		if(EMAIL_ERROR_NONE == email_update_rule (filter_id,rule))
 *			//success
 *		else
 *			//failure
 *		err = email_free_rule (&rule,1);
 *
 *		// Delete a filter information
 *		printf("Enter filter Id:\n");
 *		scanf("%d",&filter_id);
 *
 *		if(EMAIL_ERROR_NONE == email_delete_rule (filter_id))
 *			//success
 *		else
 *			//failure
 *
 *		// Free allocated memory
 *		if(EMAIL_ERROR_NONE == email_free_rule (&rule,1))
 *			//success
 *		else
 *			//failure
 *
 *	}
 *
 * @endcode
 * @}
 */



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**

 * @open
 * @fn email_get_rule(int filter_id, email_rule_t** filtering_set)
 * @brief	Get a information of filtering.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure
 * @param[in] filter_id			Specifies the filter ID.
 * @param[out] filtering_set	The returned information of filter are saved here.
 * @exception 	#EMAIL_ERROR-INVALID_PARAM 	-Invalid argument
 * @see 	email_rule_t
 * @remarks N/A
 */
EXPORT_API int email_get_rule(int filter_id, email_rule_t** filtering_set);

/**

 * @open
 * @fn email_get_rule_list(email_rule_t** filtering_set, int* count)
 * @brief	Get all filterings.This function gives all the filter rules already set before by user.
 * 		This will provide total number of filter rules available and information of all rules.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure
 * @param[out] filtering_set		The returned filterings are saved here.(possibly NULL)
 * @param[out] count				The count of returned filters is saved here.(possibly 0)
 * @exception 	#EMAIL_ERROR-INVALID_PARAM 	-Invalid argument
 * @see 	email_rule_t
 * @remarks N/A
 */
EXPORT_API int email_get_rule_list(email_rule_t** filtering_set, int* count);

/**

 * @open
 * @fn email_add_rule(email_rule_t* filtering_set)
 * @brief	Add a filter information.This function is invoked if user wants to add a new filter rule.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] filtering_set		Specifies the pointer of adding filter structure.
 * @exception 	#EMAIL_ERROR-INVALID_PARAM 	-Invalid argument
 * @see 	email_rule_t
 * @remarks N/A
 */
EXPORT_API int email_add_rule(email_rule_t* filtering_set);

/**

 * @open
 * @fn email_update_rule(int filter_id, email_rule_t* new_set)
 * @brief	Change a filter information.This function will update the existing filter rule with new information.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] filter_id	Specifies the original filter ID.
 * @param[in] new_set	Specifies the information of new filter.
 * @exception 	#EMAIL_ERROR-INVALID_PARAM 	-Invalid argument
 * @see 	email_rule_t
 * @remarks N/A
 */
EXPORT_API int email_update_rule(int filter_id, email_rule_t* new_set);

/**

 * @open
 * @fn email_delete_rule(int filter_id)
 * @brief	Delete a filter information.This function will delete the exsting filter information by specified filter Id.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure
 * @param[in] filter_id	Specifies the filter ID.
 * @exception 	#EMAIL_ERROR-INVALID_PARAM 	-Invalid argument
 * @see 	none
 * @remarks N/A
 */
EXPORT_API int email_delete_rule(int filter_id);



/**

 * @open
 * @fn email_free_rule (email_rule_t** filtering_set, int count)
 * @brief	Free allocated memory.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure
 * @param[in] filtering_set	Specifies the pointer of pointer of filter structure for memory free.
 * @param[in] count			Specifies the count of filter.
 * @exception 	#EMAIL_ERROR-INVALID_PARAM 	-Invalid argument
 * @see 	email_rule_t
 * @remarks N/A
 */
EXPORT_API int email_free_rule (email_rule_t** filtering_set, int count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
* @} @}
*/


#endif /* __EMAIL_API_RULE_H__ */
