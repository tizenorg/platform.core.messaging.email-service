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
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

/* open header */
#include <glib.h>

#include "email-api-init.h"

/* internal header */
#include "testapp-utility.h"
#include "testapp-account.h"
#include "testapp-mail.h"
#include "testapp-mailbox.h"
#include "testapp-rule.h"
#include "testapp-thread.h"
#include "testapp-others.h"
#include "db-util.h"

/* function prototype */
static void testapp_system_signal_handler (int signal_number);


/* implementation */
static gboolean testapp_initialize_testing ()
{
	struct timeval tv_1, tv_2;
	int interval;
	int error;
	
	/* register signal handler */
	if ( signal (SIGINT, testapp_system_signal_handler) == SIG_ERR ) {
		testapp_print ("register signal handler fail\n");
		return FALSE;
	}

	if ( signal (SIGQUIT, testapp_system_signal_handler) == SIG_ERR ) {
		testapp_print ("register signal handler fail\n");
		return FALSE;
	}

	if ( signal (SIGTSTP, testapp_system_signal_handler) == SIG_ERR ) {
		testapp_print ("register signal handler fail\n");
		return FALSE;
	}

	if ( signal (SIGTERM, testapp_system_signal_handler) == SIG_ERR ) {
		testapp_print ("register signal handler fail\n");
		return FALSE;
	}

	
	gettimeofday(&tv_1, NULL);
	
	if ( email_service_begin() != EMAIL_ERROR_NONE ) {
		testapp_print ("unexpected error: opening email service fail\n");
		return FALSE;
	}
	gettimeofday(&tv_2, NULL);
	interval = tv_2.tv_usec - tv_1.tv_usec;
	testapp_print("\t email_service_begin Proceed time %d us\n",interval);

	gettimeofday(&tv_1, NULL);
	if ( (error = email_open_db()) != EMAIL_ERROR_NONE) {
		testapp_print("email_open_db failed [%d]\n", error);
	}
	gettimeofday(&tv_2, NULL);
	interval = tv_2.tv_usec - tv_1.tv_usec;
	testapp_print("\t email_open_db Proceed time %d us\n",interval);

	return TRUE;
}

static gboolean testapp_finalize_testing ()
{
	int error;

	if ( (error = email_close_db()) != EMAIL_ERROR_NONE) {
		testapp_print("email_close_db failed [%d]\n", error);
	}	

	if ( email_service_end() != EMAIL_ERROR_NONE) {
		testapp_print ("unexpected error: closing email service fail \n");
	}

	return TRUE;
}

static void testapp_system_signal_handler (int signal_number)
{
	testapp_print ("signal:%d\n", signal_number);
	switch (signal_number) {
		case SIGQUIT:
		case SIGINT:
		case SIGTSTP:
		case SIGTERM:
			testapp_finalize_testing();
			break;

		default:
			testapp_print ("unhandled signal:%d\n", signal_number);
			break;
	}
	exit(0);
}


static gboolean testapp_interpret_command (int menu_number)
{
	gboolean go_to_loop = TRUE;

	switch (menu_number) {
		case 1:
			testapp_account_main();
			break;
			
		case 2:
			testapp_mail_main();
			break;
			
		case 3:
			email_test_mailbox_main();
			break;
			
		case 4:
			break;
			
		case 5:
			email_test_rule_main();
			break;
			
		case 6:
			testapp_thread_main();
			break;
			
		case 7:
			testapp_others_main();
			break;
		case 0:
			go_to_loop = FALSE;
			break;
		default:
			break;
	}

	return go_to_loop;
}

int main (int argc, char *argv[])
{
	gboolean go_to_loop = TRUE;
	int menu_number = 0;
	int result_from_scanf = 0;

	if ( testapp_initialize_testing() == FALSE ) {
		testapp_print ("email-serivce is not ready\n");
		exit(0);
	}

	while (go_to_loop) {
		testapp_show_menu (EMAIL_MAIN_MENU);
		testapp_show_prompt (EMAIL_MAIN_MENU);

		result_from_scanf = scanf ("%d", &menu_number);

		go_to_loop = testapp_interpret_command (menu_number);
	}

	testapp_finalize_testing();

	exit(0);
}


