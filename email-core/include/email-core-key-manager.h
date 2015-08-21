/*
*  email-service
*
* Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Sunghyun Kwon <sh0701.kwon@samsung.com>
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

#ifndef __EMAIL_CORE_KEY_MANAGER_H__
#define __EMAIL_CORE_KEY_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

INTERNAL_FUNC int emcore_add_password_in_key_manager(char *data_name, char *stored_data);
INTERNAL_FUNC int emcore_get_password_in_key_manager(char *data_name, char **stored_data);
INTERNAL_FUNC int emcore_remove_password_in_key_manager(char *data_name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

