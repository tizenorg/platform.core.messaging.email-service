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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include "device/power.h"
#include "email-device.h"
#include "email-debug-log.h"

int stay_awake_flag[STAY_AWAKE_FLAG_MAX] = { 0, };
static pthread_mutex_t stay_awake_flag_lock = PTHREAD_MUTEX_INITIALIZER;

INTERNAL_FUNC int emdevice_set_sleep_on_off(email_stay_awake_flag_owner_t input_flag_owner, int input_allow_to_sleep, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN("input_flag_owner[%d] input_allow_to_sleep[%d] err_code[%p]", input_flag_owner, input_allow_to_sleep, error_code);
	int result_from_pm_api = 0;

	ENTER_CRITICAL_SECTION(stay_awake_flag_lock);

	stay_awake_flag[input_flag_owner] = !input_allow_to_sleep;

	if (input_allow_to_sleep == 1) {
		int i = 0;
		int allowed_to_sleep = 1;

		for (i = 0; i < STAY_AWAKE_FLAG_MAX; i++) {
			if (stay_awake_flag[i] == 1) {
				allowed_to_sleep = 0;
				break;
			}
		}

		if (allowed_to_sleep) {
			/* allowed to sleep */
			result_from_pm_api = device_power_release_lock(POWER_LOCK_CPU);
			EM_DEBUG_LOG("display_unlock_state() returns [%d]", result_from_pm_api);
		} else {
			EM_DEBUG_LOG("other worker[%d] is working on", i);
		}
	} else {
		/* Stay awake */
		result_from_pm_api = device_power_request_lock(POWER_LOCK_CPU, 0);
		EM_DEBUG_LOG("display_lock_state() returns [%d]", result_from_pm_api);
	}

	LEAVE_CRITICAL_SECTION(stay_awake_flag_lock);

	EM_DEBUG_FUNC_END();
	return true;
}


