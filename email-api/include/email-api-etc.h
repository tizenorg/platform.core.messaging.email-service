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

/**
* @defgroup EMAIL_SERVICE Email Service
* @{
*/

/**
* @ingroup EMAIL_SERVICE
* @defgroup EMAIL_API_ETC Email API
* @{
*/

/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with email-service.
 * @file	email-api-etc.h
 * @author	Kyuho Jo <kyuho.jo@samsung.com>
 * @author	Sunghyun Kwon <sh0701.kwon@samsung.com>
 * @version	0.1
 * @brief 	This file contains the data structures and interfaces of Accounts provided by
 *			email-service .
 */

#ifndef __EMAIL_API_ETC_H__
#define __EMAIL_API_ETC_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "email-types.h"

/**
 * @fn email_show_user_message
 * @brief	This function show user message.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @exception 	none
 * @see 	email_action_t
 * @remarks N/A
 */
EXPORT_API int email_show_user_message(int id, email_action_t action, int error_code);

/**
 * @fn email_parse_mime_file
 * @brief	This function parse mime file
 * @param[in]	eml_file_path
 * @param[out]	output_mail_data
 * @param[out]	output_attachment_data
 * @param[out]	output_attachment_count
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @exception 	none
 * @see
 * @remarks N/A
 */
EXPORT_API int email_parse_mime_file(char *eml_file_path, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data, int *output_attachment_count);

/**
 * @fn email_write_mime_file
 * @brief	This function create mime file from input data
 * @param[in]	input_mail_data
 * @param[in]	input_attachment_data
 * @param[in]	input_attachment_count
 * @param[out]	output_file_path
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @exception 	none
 * @see
 * @remarks N/A
 */
EXPORT_API int email_write_mime_file(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data, int input_attachment_count, char **output_file_path);

/**
 * @fn email_delete_parsed_data
 * @brief	This function delete parsed files of mime
 * @param[in]	input_mail_data
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @exception 	none
 * @see
 * @remarks N/A
 */
EXPORT_API int email_delete_parsed_data(email_mail_data_t *input_mail_data);


/**
 * @fn email_get_mime_entity
 * @brief	This function get mime entity
 * @param[in]	mime_path
 * @param[out]	mime_entity
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @exception 	none
 * @see
 * @remarks N/A
 */
EXPORT_API int email_get_mime_entity(char *mime_path, char **mime_entity);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __EMAIL_API_ETC_H__ */

/**
* @} @}
*/
