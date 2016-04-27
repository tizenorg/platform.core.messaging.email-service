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


#ifndef __EMAIL_API_INTI_H__
#define __EMAIL_API_INTI_H__

#include "email-types.h"

/**
 * @file email-api-init.h
 * @brief This file contains the data structures and interfaces of Email FW Initialization provided by email-service.
 *
 * @{
 * @code
 *
 *  #include "email-api-init.h"
 *
 *  bool
 *  other_app_invoke_uniform_api_sample(int* error_code)
 *  {
 *       int err = EMAIL_ERROR_NONE;
 *
 *      // Opens connections to email-service and DB
 *      // The connections will be maintain throughout application's execution
 *      if(EMAIL_ERROR_NONE == email_service_begin())
 *      {
 *          If(EMAIL_ERROR_NONE != email_open_db())
 *          {
 *              return false;
 *          }
 *
 *          // Executes email_init_storage() if and only if there is no db file.
 *              // This function will create db file and tables for email service
 *          If(EMAIL_ERROR_NONE !=email_init_storage())
 *          {
 *              return false;
 *          }
 *      }
 *
 *      ......
 *
 *      // Work with calling MAPI functions
 *
 *      ......
 *
 *      // Closes the connections to email-service and DB after all email jobs are finished. (ex. close an email application)
 *      // DO NOT have to call these funtions until the connections are not needed any more.
 *      err =email_close_db();
 *      err =email_service_end();
 *   }
 *
 * @endcode
 * @}
 */

/**
 * @addtogroup EMAIL_SERVICE_ETC_MODULE
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Creates all tables for an email.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 3.0 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_init_storage(void);

/**
 * @brief Opens the email DB and registers a busy handler.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 3.0 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_open_db(void);


/**
 * @brief Closes the connection to the email DB.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 3.0 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_close_db(void);

/**
 * @brief Initializes IPC Proxy by an application which used the Email FW API's.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 3.0 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_service_begin(void);

/**
 * @brief Finalizes IPC Proxy by an application which used the Email FW API's.

 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 3.0 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_service_end(void);

/**
 * @brief Checks whether the email-service process is running.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 3.0 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_ping_service(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * @}
 */


#endif  /* __EMAIL_API_INTI_H__ */
