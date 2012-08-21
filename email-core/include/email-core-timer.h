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

#include "email-internal-types.h"

typedef void (*EMAIL_TIMER_CALLBACK)(void *a_pData);

/**
 * Callback for timeout action
 *
 * @param[in] a_pData			Specifies the data passed to callback.
 * @return This function returns true for repeat alerts or false for mail resend.
 */
INTERNAL_FUNC int emcore_timer_ex_callback(void *a_pData);

/**
 * Set timer
 *
 * @param[in] a_nSetTimeValue	Specifies the timeout value.
 * @param[in] a_pCallBack		Specifies the Callback to be called on timeout
 * @param[in] a_pData		Specifies the pointer to user data to be passed to Callback.
 * @remarks N/A
 * @return This function returns the timer id.
 */
INTERNAL_FUNC int emcore_set_timer_ex(long a_nSetTimeValue, EMAIL_TIMER_CALLBACK a_pCallBack, void *a_pData);

/**
 * Kill timer.
 *
 * @param[in] a_nTimerID		Specifies the timer id.
 * @remarks N/A
 * @return This function returns void.
 */
INTERNAL_FUNC void emcore_kill_timer_ex(int a_nTimerID);
