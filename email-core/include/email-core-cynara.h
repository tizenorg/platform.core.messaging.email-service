/*
*  email-service
*
* Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Sunghyun Kwon <sh0701.kwon@samsung.com>, Minsoo Kim <minnsoo.kim@samsung.com>
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
 * This file contains functionality related to cynara(privilege)
 * to interact with email-service.
 * @file	email-core-cynara.h
 * @author	sh0701.kwon@samsung.com
 * @version	0.1
 * @brief	This file contains functionality to provide cynara support in email-service. 
 */

INTERNAL_FUNC void emcore_init_cynara();

INTERNAL_FUNC void emcore_finish_cynara();

INTERNAL_FUNC int emcore_check_privilege(int socket_fd, int client_pid);
