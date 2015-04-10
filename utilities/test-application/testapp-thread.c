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



/* common header */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* open header */
#include <glib.h>

#include "email-api-account.h"
#include "email-api-mail.h"

/* internal header */
#include "testapp-utility.h"
#include "testapp-thread.h"

static gboolean testapp_test_move_thread()
{
	int thread_id, move_always_flag;
	int target_mailbox_id;

	testapp_print("\n > Enter thread_id: ");
	if (0 >= scanf("%d", &thread_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter target_mailbox_id: ");
	if (0 >= scanf("%d", &target_mailbox_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter move_always_flag: ");
	if (0 >= scanf("%d", &move_always_flag))
		testapp_print("Invalid input. ");
	
	email_move_thread_to_mailbox(thread_id, target_mailbox_id, move_always_flag);

	return FALSE;
}

static gboolean testapp_test_delete_thread()
{
	int thread_id, delete_always_flag;

	testapp_print("\n > Enter thread_id: ");
	if (0 >= scanf("%d", &thread_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter delete_always_flag: ");
	if (0 >= scanf("%d", &delete_always_flag))
		testapp_print("Invalid input. ");

	email_delete_thread(thread_id, delete_always_flag);

	return FALSE;
}

static gboolean testapp_test_set_seen_flag_of_thread()
{
	int thread_id, seen_flag, on_server;

	testapp_print("\n > Enter thread_id: ");
	if (0 >= scanf("%d", &thread_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter seen_flag: ");
	if (0 >= scanf("%d", &seen_flag))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter on_server: ");
	if (0 >= scanf("%d", &on_server))
		testapp_print("Invalid input. ");

	email_modify_seen_flag_of_thread(thread_id, seen_flag, on_server);

	return FALSE;
}

static gboolean testapp_test_interpret_command (int menu_number)
{
	gboolean go_to_loop = TRUE;

	switch (menu_number) {
		case 1:
			testapp_test_move_thread();
			break;

		case 2:
			testapp_test_delete_thread();
			break;

		case 3:
			testapp_test_set_seen_flag_of_thread();
			break;

		case 0:
			go_to_loop = FALSE;
			break;
		default:
			break;
	}

	return go_to_loop;
}

void testapp_thread_main()
{
	gboolean go_to_loop = TRUE;
	int menu_number = 0;

	while (go_to_loop) {
		testapp_show_menu (EMAIL_THREAD_MENU);
		testapp_show_prompt (EMAIL_THREAD_MENU);

		if (0 >= scanf("%d", &menu_number))
			testapp_print("Invalid input. ");

		go_to_loop = testapp_test_interpret_command (menu_number);
	}
}
