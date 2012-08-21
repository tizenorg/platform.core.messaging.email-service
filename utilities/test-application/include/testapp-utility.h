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



#ifndef EMAIL_TEST_UTILITY_H
#define EMAIL_TEST_UTILITY_H

#include <glib.h>

typedef enum
{
	EMAIL_MAIN_MENU = 0x0,
	EMAIL_ACCOUNT_MENU,
	EMAIL_MAIL_MENU,
	EMAIL_MAILBOX_MENU,
	EMAIL_RULE_MENU,
	EMAIL_THREAD_MENU,
	EMAIL_OTHERS_MENU,
} eEMAIL_MENU;


void testapp_print(char *fmt, ...);
void testapp_show_menu(eEMAIL_MENU menu);
void testapp_show_prompt(eEMAIL_MENU menu);

#endif

