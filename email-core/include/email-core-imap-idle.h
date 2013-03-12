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
 *
 * This file contains functinality related to IMAP IDLE.
 * @file		email-core-imap-idle.h
 * @author	
 * @version	0.1
 * @brief 		This file contains functionality to provide IMAP IDLE support in email-service. 
 */

#include "email-internal-types.h"

/**


 * @fn emcore_create_imap_idle_thread(int *err_code)
 * @brief	Creates a thread that listens for IMAP IDLE Notifications.
 *
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */

INTERNAL_FUNC int emcore_create_imap_idle_thread(int accountID, int *err_code);

/**


 * @fn emcore_kill_imap_idle_thread(int *err_code)
 * @brief	Kills IMAP IDLE thread
 *
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emcore_kill_imap_idle_thread(int *err_code);
