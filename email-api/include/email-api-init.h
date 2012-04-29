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


#ifndef __EMAIL_API_INTI_H__
#define __EMAIL_API_INTI_H__

#include "email-types.h"

/**
* @defgroup EMAIL_SERVICE Email Service
* @{
*/


/**
* @ingroup EMAIL_SERVICE
* @defgroup EMAIL_API_INIT Email Initialization API
* @{
*/

/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with email-service.
 * @file		email-api-init.h
 * @author	Kyuho Jo <kyuho.jo@samsung.com>
 * @author	Sunghyun Kwon <sh0701.kwon@samsung.com>
 * @version	0.1
 * @brief 		This file contains the data structures and interfaces of Email FW Initialization provided by
 *			email-service .
 *@{
 *@code
 *
 *	#include "emf_mapi_init.h"
 *
 *	bool
 *	other_app_invoke_uniform_api_sample(int* error_code)
 *	{
 *		 int err = EMF_ERROR_NONE;
 *
 *		// Open connections to email-service and DB
 *		// The connections will be maintain throughout application's execution
 *		if(EMF_ERROR_NONE == email_service_begin())
 *		{
 *			If(EMF_ERROR_NONE != email_open_db())
 *			{
 *				return false;
 *			}
 *
 *			// Execute email_init_storage() if and only if there is no db file.
 *  			// This fuction will create db file and tables for email service
 *			If(EMF_ERROR_NONE !=email_init_storage())
 *			{
 *				return false;
 *			}
 *		}
 *
 *		......
 *
 *		// Work with calling MAPI functions
 *
 *		......
 *
 *		// Close the connections to email-service and DB after all email job is finished. (ex. close an email application)
 *		// DO NOT have to call these funtions until the connections is not needed any more.
 *		err =email_close_db();
 *		err =email_service_end();
 * 	 }
 *
 * @endcode
 * @}
 */



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**

 * @open
 * @fn email_init_storage(void)
 * @brief	Create all table for email. Exposed to External Application- core team.Creates all Email DB tables [ EXTERNAL]
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure
 * @exception none
 * @see none
 * @remarks N/A
 */
EXPORT_API int email_init_storage(void);

/**

 * @open
 * @fn email_open_db(void)
 * @brief This function Open the email DB and register busy handler
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @exception none
 * @see 	none
 * @remarks N/A
 */
EXPORT_API int email_open_db(void);


/**

 * @open
 * @fn email_close_db(void)
 * @brief	This function closes the connection of  the email DB
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @exception none
 * @see none
 * @remarks N/A
 */
EXPORT_API int email_close_db(void);

/**

 * @open
 * @fn email_service_begin(void)
 * @brief	Does the IPC Proxy Initialization by the Application which used the Email FW API's
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @exception none
 * @see 	none
 * @remarks N/A
 */
EXPORT_API int email_service_begin(void);

/**

 * @open
 * @fn email_service_end(void)
 * @brief	This function does the IPC Proxy Finaization by the Application which used the Email FW API's
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @exception 	none
 * @see 	none
 * @remarks N/A
 */
EXPORT_API int email_service_end(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
* @} @}
*/


#endif  /* __EMAIL_API_INTI_H__ */
