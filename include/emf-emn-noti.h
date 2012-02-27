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
 * File: emf_noti.h
 * Desc: Email notification
 *
 * Auth:
 *
 * History:
 *    2006.08.01 : created
 *****************************************************************************/
#ifndef __EMF_EMN_NOTI_H__
#define __EMF_EMN_NOTI_H__


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

#define EMAIL_GCONF_DIR  "/Apps/Email"


int emf_emn_noti_init(void);
int emf_emn_noti_quit(int bExtDest);

void 
emn_test_func(void);
#endif /*__EMF_EMN_NOTI_H__*/
