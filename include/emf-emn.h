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
 * This file defines all APIs of EMN.
 * @file	emf-emn.h
 * @author	Kyuho Jo(kyuho.jo@samsung.com)
 * @version	0.1
 * @brief	This file is the header file of EMN library.
 */

#ifndef __EMF_EMN_H__
#define __EMF_EMN_H__

/**
* @ingroup EMAIL_FRAMEWORK
* @defgroup EMN EMN
* @{
*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#if !defined(EXPORT_API)
#define EXPORT_API __attribute__((visibility("default")))
#endif

/** 
 * This callback specifies the callback of retrieving the result that is downloaded new messages.
 *
 * @param[in] mail_count	Specifies count of new mail.
 * @param[in] user_data		Specifies the user data.
 * @param[in] err_code		Specifies the error code.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
typedef int (*emf_emn_noti_cb)(
	void*					user_data,
	int                     err_code
);

#ifdef USE_OMA_EMN
/**
 * Handle OMA EMN data
 *
 * @param[in] wbxml_b64		Specifies the encoded string
 * @param[in] callback		Specifies the callback function for retrieving the result that is downloaded new mail.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int emf_emn_handler(unsigned char* wbxml_b64, emf_emn_noti_cb callback, int* err_code);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
* @} @}
*/


#endif /* __EMF_EMN_H__ */
/* EOF */

