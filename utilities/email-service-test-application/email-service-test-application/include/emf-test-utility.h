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



#ifndef EMF_TEST_UTILITY_H
#define EMF_TEST_UTILITY_H

#include <glib.h>

typedef enum
{
	EMF_MAIN_MENU = 0x0,
	EMF_ACCOUNT_MENU,
	EMF_MESSAGE_MENU,
	EMF_MAILBOX_MENU,
	EMF_RULE_MENU,
	EMF_THREAD_MENU,
	EMF_OTHERS_MENU,
} eEMF_MENU;


/* export API */
void testapp_print(char *fmt, ...);
void testapp_show_menu(eEMF_MENU menu);
void testapp_show_prompt(eEMF_MENU menu);

#endif

