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

#include "email-device.h"
#include "email-debug-log.h"



INTERNAL_FUNC int emdevice_set_sleep_on_off(int on, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN("on[%d], err_code[%p]", on, error_code);
	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emdevice_set_dimming_on_off(int on, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN("on[%d], err_code[%p]", on, error_code);
	EM_DEBUG_FUNC_END();
	return true;
}
