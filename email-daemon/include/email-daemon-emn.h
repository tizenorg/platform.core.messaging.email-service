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
 * This file defines all APIs of EMN.
 * @file	email-daemon-emn.h
 * @author	Kyuho Jo(kyuho.jo@samsung.com)
 * @version	0.1
 * @brief	This file is the header file of EMN library.
 */

#ifndef __EMAIL_DAEMON_EMN_H__
#define __EMAIL_DAEMON_EMN_H__


#include "email-internal-types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

enum
{
	EMN_HEADER_DATA = 0,
	EMN_WBXML_DATA,
};

/* Error Values for the Email options */
enum
{
	EMAIL_OPTION_ERROR_NONE = 0,
	EMAIL_OPTION_ERROR_INVALID_PARAM,
	EMAIL_OPTION_ERROR_STORAGE,
};

/* Manual Network value */
#define   EMAIL_OPTION_NETWORK_MANUAL  1


/* Enums for the Gcong Option Key */
typedef enum
{
	EMAIL_OPTION_KEY_HOME_NETWORK = 0,
	EMAIL_OPTION_KEY_ROAMING_NETWORK,
}optionkey;


int emdaemon_initialize_emn(void);
int emdaemon_finalize_emn(int bExtDest);

/** 
 * This callback specifies the callback of retrieving the result that is downloaded new messages.
 *
 * @param[in] mail_count	Specifies count of new mail.
 * @param[in] user_data		Specifies the user data.
 * @param[in] err_code		Specifies the error code.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
typedef int (*email_emn_noti_cb)(
	void*					user_data,
	int                     err_code
);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __EMAIL_DAEMON_EMN_H__ */
/* EOF */

