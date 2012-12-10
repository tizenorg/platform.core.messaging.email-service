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


 * @fn email_show_user_message(void)
 * @brief	This function show user message.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @exception 	none
 * @see 	email_action_t
 * @remarks N/A
 */
EXPORT_API int email_show_user_message(int id, email_action_t action, int error_code);

EXPORT_API int email_parse_mime_file(char *eml_file_path, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data, int *output_attachment_count);

EXPORT_API int email_write_mime_file(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data, int input_attachment_count, char **output_file_path);

EXPORT_API int email_delete_parsed_data(email_mail_data_t *input_mail_data);

EXPORT_API int email_get_mime_entity(char *mime_path, char **mime_entity);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __EMAIL_API_ETC_H__ */

/**
* @} @}
*/
