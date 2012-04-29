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
 * File :  email-device.h
 * Desc :  email-core Device Library Header
 *
 * Auth : 
 *
 * History : 
 *    2012.02.20  :  created
 *****************************************************************************/
#ifndef __EMAIL_DEVICE_H__
#define __EMAIL_DEVICE_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "email-internal-types.h"

INTERNAL_FUNC int emdevice_set_sleep_on_off(int on, int *error_code);
INTERNAL_FUNC int emdevice_set_dimming_on_off(int on, int *error_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __EMAIL_DEVICE_H__ */
