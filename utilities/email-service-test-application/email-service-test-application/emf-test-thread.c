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



/* common header */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* open header */
#include <glib.h>

#include "Emf_Mapi_Message.h"

/* internal header */
#include "emf-test-utility.h"
#include "emf-test-thread.h"

static gboolean testapp_test_move_thread()
{
	int thread_id, move_always_flag;
	char target_mailbox_name[512];
	int result;

	testapp_print("\n > Enter thread_id: ");
	scanf("%d", &thread_id);

	testapp_print("\n > Enter target_mailbox_name: ");
	scanf("%s", target_mailbox_name);

	testapp_print("\n > Enter move_always_flag: ");
	scanf("%d", &move_always_flag);
	
	result = email_move_thread_to_mailbox(thread_id, target_mailbox_name, move_always_flag);

	return FALSE;
}

static gboolean testapp_test_delete_thread()
{
	int thread_id, delete_always_flag;
	int result;

	testapp_print("\n > Enter thread_id: ");
	scanf("%d", &thread_id);

	testapp_print("\n > Enter delete_always_flag: ");
	scanf("%d", &delete_always_flag);

	result = email_delete_thread(thread_id, delete_always_flag);

	return FALSE;
}

static gboolean testapp_test_set_seen_flag_of_thread()
{
	int thread_id, seen_flag, on_server;
	int result;

	testapp_print("\n > Enter thread_id: ");
	scanf("%d", &thread_id);

	testapp_print("\n > Enter seen_flag: ");
	scanf("%d", &seen_flag);

	testapp_print("\n > Enter on_server: ");
	scanf("%d", &on_server);

	result = email_modify_seen_flag_of_thread(thread_id, seen_flag, on_server);

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
		testapp_show_menu (EMF_THREAD_MENU);
		testapp_show_prompt (EMF_THREAD_MENU);

		scanf ("%d", &menu_number);

		go_to_loop = testapp_test_interpret_command (menu_number);
	}
}