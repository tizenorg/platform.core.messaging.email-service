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
 * This file defines all APIs of EMN's storage.
 * @file	emf-emn-storage.h
 * @version	0.1
 * @brief	This file is the header file of EMN's storage library.
 */

#ifndef __EMF_EMN_STORAGE_H__
#define __EMF_EMN_STORAGE_H__

/**
* @ingroup EMAIL_FRAMEWORK
* @defgroup EMN_STORAGE EMN Storage
* @{
*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#if !defined(EXPORT_API)
#define EXPORT_API __attribute__((visibility("default")))
#endif

#ifdef MAILHOME
#undef MAILHOME
#endif

#define MAILHOME DATA_PATH"/.email-service/.emfdata"	/**< This defines the storage mailbox. */ 
#define MAILTEMP "tmp"	/**< This defines the temporary mailbox. */
#ifndef true
#define true 1		/**< This defines true. */
#define false 0		/**< This defines false. */
#endif
	
/** 
 * This enumeration specifies the mail service type.
 */
typedef enum
{
    EMF_SVC_EMN = 1,	/**< Specifies the EMN service.*/
    EMF_SVC_VISTO,		/**< Specifies the Pushmail service(VISTO).*/
    EMF_SVC_IMAP_IDLE	/**< Specifies the normal email service.*/
} emf_svc_type_t;



#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
* @} @}
*/



#endif /* __EMF_EMN_STORAGE_H__ */
/* EOF */

