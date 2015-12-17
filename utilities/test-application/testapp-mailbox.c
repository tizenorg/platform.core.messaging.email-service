/*
*  email-service
*
* Copyright(c) 2012 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Kyuho Jo <kyuho.jo@samsung.com>, Sunghyun Kwon <sh0701.kwon@samsung.com>
*
* Licensed under the Apache License, Version 2.0(the "License");
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
#include <string.h>


/* open header */
#include <glib.h>

#include "email-api-account.h"
#include "email-api-network.h"
#include "email-api-mailbox.h"

/* internal header */
#include "testapp-utility.h"
#include "testapp-mailbox.h"

static gboolean testapp_print_mailbox_list(email_mailbox_t *input_mailbox_list, int input_count)
{
	int i;
	char time_string[40] = { 0, };

	testapp_print("There are %d mailboxes\n", input_count);
	testapp_print("============================================================================\n");
	testapp_print("id\ta_id\talias\tunread\t total\t total_on_ server\tmailbox_type\tlast_sync_time\n");
	testapp_print("============================================================================\n");
	if (input_count == 0) {
		testapp_print("No mailbox is matched\n");
	} else {
		for (i = 0; i < input_count; i++) {
			strftime(time_string, 40, "%Y-%m-%d %H:%M:%S", localtime(&(input_mailbox_list[i].last_sync_time)));
			testapp_print("%2d\t", input_mailbox_list[i].mailbox_id);
			testapp_print("%2d\t%-12s\t", input_mailbox_list[i].account_id, input_mailbox_list[i].alias);
			testapp_print("%3d\t%3d\t%3d\t%3d\t%s\n"
					, input_mailbox_list[i].unread_count
					, input_mailbox_list[i].total_mail_count_on_local
					, input_mailbox_list[i].total_mail_count_on_server
					, input_mailbox_list[i].mailbox_type
					, time_string);
		}
	}
	testapp_print("============================================================================\n");

	return FALSE;
}

static gboolean testapp_test_add_mailbox()
{
	email_mailbox_t  mailbox;
	int account_id, mailbox_type = 0;
	int local_yn = 0;
	char arg[500];
	int ret;
    int handle;

    memset(&mailbox, 0, sizeof(email_mailbox_t));

	memset(arg, 0x00, 500);
	testapp_print("\n> Enter mailbox name: ");
	if (0 >= scanf("%s", arg))
		testapp_print("Invalid input. ");
	mailbox.mailbox_name = strdup(arg);

	memset(arg, 0x00, 500);
	testapp_print("> Enter mailbox alias name: ");
	if (0 >= scanf("%s", arg))
		testapp_print("Invalid input. ");
	mailbox.alias = strdup(arg);

	testapp_print("> Enter account id: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");
	mailbox.account_id = account_id;

	testapp_print("> Enter local_yn(1/0): ");
	if (0 >= scanf("%d", &local_yn))
		testapp_print("Invalid input. ");
	mailbox.local = local_yn;

	testapp_print("> Enter mailbox type: ");
	if (0 >= scanf("%d", &mailbox_type))
		testapp_print("Invalid input. ");

	mailbox.mailbox_type = mailbox_type;
	mailbox.eas_data = strdup("EAS DATA TEST");
	mailbox.eas_data_length = strlen(mailbox.eas_data) + 1;


	ret = email_add_mailbox(&mailbox, local_yn ? 0 : 1, &handle);

	if (ret < 0) {
		testapp_print("\n email_add_mailbox failed");
	} else {
		testapp_print("\n email_add_mailbox succeed : handle[%d], mailbox_id [%d]\n", handle, mailbox.mailbox_id);
	}

	if (mailbox.eas_data)
		free(mailbox.eas_data);

	return FALSE;
}

static gboolean testapp_test_delete_mailbox()
{
	int mailbox_id = 0;
	int on_server = 0;
	int ret;
	int handle;

	testapp_print("\n> Enter mailbox id:");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input");

	testapp_print("> Enter on_server(1/0): ");
	if (0 >= scanf("%d", &on_server))
		testapp_print("Invalid input");

	ret = email_delete_mailbox(mailbox_id, on_server, &handle);

	if (ret < 0) {
		testapp_print("\n email_delete_mailbox failed");
	} else {
		testapp_print("\n email_delete_mailbox succeed : handle[%d]\n", handle);
	}

	return FALSE;

}

static gboolean testapp_test_rename_mailbox()
{
	testapp_print("testapp_test_rename_mailbox\n");
	int mailbox_id;
	char mailbox_name[500] = { 0, };
	char mailbox_alias[500] = { 0, };
	int err;
	int handle = 0;

	testapp_print("> Enter mailbox id: ");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input");

	testapp_print("> Enter new mailbox name: ");
	if (0 >= scanf("%s", mailbox_name))
		testapp_print("Invalid input");

	testapp_print("> Enter new mailbox name: ");
	if (0 >= scanf("%s", mailbox_alias))
		testapp_print("Invalid input");

	if ((err = email_rename_mailbox(mailbox_id, mailbox_name, mailbox_alias, true, &handle)) < 0) {
		testapp_print("\n email_rename_mailbox failed[%d]\n", err);
	} else {
		testapp_print("\n email_rename_mailbox succeed. handle [%d]\n", handle);
	}

	return FALSE;
}

static gboolean testapp_test_get_imap_mailbox_list()
{
	int account_id = 0;
	int handle = 0;

	testapp_print("> Enter account id: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input");

	if (email_sync_imap_mailbox_list(account_id, &handle) < 0)
		testapp_print("email_sync_imap_mailbox_list failed");

	return FALSE;

}

static gboolean testapp_test_set_local_mailbox()
{
	int mailbox_id = 0;
	int is_local_mailbox = 0;

	testapp_print("> Enter mailbox id: ");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input");

	testapp_print("> Enter local: ");
	if (0 >= scanf("%d", &is_local_mailbox))
		testapp_print("Invalid input");

	if (email_set_local_mailbox(mailbox_id, is_local_mailbox) < 0)
		testapp_print("email_set_local_mailbox failed");

	return FALSE;
}

static gboolean testapp_test_delete_mailbox_ex()
{
	int  err = EMAIL_ERROR_NONE;
	int  mailbox_id_count = 0 ;
	int *mailbox_id_array = NULL;
	int  account_id = 0;
	int  on_server = 0;
	int  handle = 0;
	int  i = 0;

	testapp_print("\n > Enter account_id: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input");

	testapp_print("\n > Enter mailbox_id_count: ");
	if (0 >= scanf("%d", &mailbox_id_count))
		testapp_print("Invalid input");

	testapp_print("\n > Enter on_server: ");
	if (0 >= scanf("%d", &on_server))
		testapp_print("Invalid input");

	mailbox_id_count = (mailbox_id_count < 5000) ? mailbox_id_count : 5000;

	if (mailbox_id_count > 0) {
		mailbox_id_array = malloc(sizeof(int) * mailbox_id_count);
	}

	for (i = 0; i < mailbox_id_count; i++) {
		testapp_print("\n > Enter mailbox id: ");
		if (0 >= scanf("%d", (mailbox_id_array + i)))
			testapp_print("Invalid input");
	}

	err = email_delete_mailbox_ex(account_id, mailbox_id_array, mailbox_id_count, on_server, &handle);

	testapp_print("\nemail_delete_mailbox_ex returns [%d], handle [%d] \n", err, handle);

	if (mailbox_id_array)
		free(mailbox_id_array);
	return 0;
}

static gboolean testapp_test_get_mailbox_by_type()
{
	int account_id = 0;
	int err_code = EMAIL_ERROR_NONE;
	email_mailbox_t *mailbox = NULL;
	email_mailbox_type_e mailbox_type = 0;

	testapp_print("\n > Enter account id: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input");

	testapp_print("\n > Enter mailbox_type: ");
	if (0 >= scanf("%d", (int*)&mailbox_type))
		testapp_print("Invalid input");

	if ((err_code = email_get_mailbox_by_mailbox_type(account_id, mailbox_type, &mailbox)) < 0) {
		testapp_print("email_get_mailbox_by_mailbox_type error : %d\n", err_code);
		return false ;
	}

	testapp_print_mailbox_list(mailbox, 1);

	email_free_mailbox(&mailbox, 1);
	return FALSE;
}

static gboolean testapp_test_set_mailbox_type()
{
	int  mailbox_id = 0;
	int  mailbox_type = 0;
	int  err_code = EMAIL_ERROR_NONE;

	testapp_print("\n > Enter mailbox id : ");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input");

	testapp_print("\n > Enter mailbox type : ");
	if (0 >= scanf("%d", &mailbox_type))
		testapp_print("Invalid input");

	if ((err_code = email_set_mailbox_type(mailbox_id, mailbox_type)) != EMAIL_ERROR_NONE) {
		testapp_print("\nemail_set_mailbox_type error : %d\n", err_code);
	}

	return FALSE;
}

static gboolean testapp_test_set_mail_slot_size()
{
	int account_id = 0;
	int mailbox_id = 0;
	int mail_slot_size = 0;
	int err_code = EMAIL_ERROR_NONE;

	testapp_print("\n > Enter account id(0: All account): ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input");

	testapp_print("\n> Enter mailbox id(0 : All mailboxes):");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input");

	testapp_print("\n > Enter mailbox slot size: ");
	if (0 >= scanf("%d", &mail_slot_size))
		testapp_print("Invalid input");

	if ((err_code = email_set_mail_slot_size(account_id, mailbox_id, mail_slot_size)) < 0) {
		testapp_print("   testapp_test_set_mail_slot_size error : %d\n", err_code);
		return false ;
	}

	return FALSE;
}

static gboolean testapp_test_get_mailbox_list()
{
	int account_id = 0;
	int mailbox_sync_type;
	int count = 0;
	int error_code = EMAIL_ERROR_NONE;
	email_mailbox_t *mailbox_list = NULL;
	testapp_print("\n > Enter account id: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input");
	testapp_print("\n > Enter mailbox_sync_type\n[-1 :for all mailboxes, 0 : for mailboxes from server, 1 : local mailboxes\n : ");
	if (0 >= scanf("%d", &mailbox_sync_type))
		testapp_print("Invalid input");

	if ((error_code = email_get_mailbox_list(account_id, mailbox_sync_type, &mailbox_list, &count)) < 0) {
		testapp_print("   email_get_mailbox_list error %d\n", error_code);
		return false ;
	}

	testapp_print_mailbox_list(mailbox_list, count);

	email_free_mailbox(&mailbox_list, count);

	if ((error_code = email_get_mailbox_list_ex(account_id, mailbox_sync_type, 1, &mailbox_list, &count)) < 0) {
		testapp_print("   email_get_mailbox_list_ex error %d\n", error_code);
		return false ;
	}

	testapp_print_mailbox_list(mailbox_list, count);

	email_free_mailbox(&mailbox_list, count);
	return FALSE;
}

static gboolean testapp_test_get_mailbox_list_by_keyword()
{
	int account_id = 0;
	int count = 0;
	char keyword[500] = { 0, };
	int error_code = EMAIL_ERROR_NONE;
	email_mailbox_t *mailbox_list = NULL;
	testapp_print("\n > Enter account id: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input");

	testapp_print("> Enter keyword: ");
	if (0 >= scanf("%s", keyword))
		testapp_print("Invalid input");

	if ((error_code = email_get_mailbox_list_by_keyword(account_id, keyword, &mailbox_list, &count)) < 0) {
		testapp_print("   email_get_mailbox_list_by_keyword error %d\n", error_code);
		return false ;
	}

	testapp_print_mailbox_list(mailbox_list, count);

	email_free_mailbox(&mailbox_list, count);

	return FALSE;
}


static gboolean testapp_test_sync_mailbox()
{
	int account_id = 0;
	int handle = 0;
	int mailbox_id = 0;

	testapp_print("\n > Enter Account id(0: for all account) : ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input");

	testapp_print("\n > Enter Mailbox id(0: for all mailboxes) : ");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input");

	if (account_id == ALL_ACCOUNT) {
		if (email_sync_header_for_all_account(&handle) < 0)
			testapp_print("\n email_sync_header_for_all_account failed\n");
		else
			testapp_print("\n email_sync_header_for_all_account success. Handle[%d]\n", handle);
	} else {
		if (email_sync_header(account_id, mailbox_id, &handle) < 0)
			testapp_print("\n email_sync_header failed\n");
		else
			testapp_print("\n email_sync_header success. Handle[%d]\n", handle);
	}

	return FALSE;
}

static gboolean testapp_test_stamp_sync_time()
{
	int input_mailbox_id = 0;

	testapp_print("\n > Enter Mailbox id : ");
	if (0 >= scanf("%d", &input_mailbox_id))
		testapp_print("Invalid input");

	email_stamp_sync_time_of_mailbox(input_mailbox_id);

	return FALSE;
}

static gboolean testapp_test_rename_mailbox_ex()
{
	testapp_print("testapp_test_rename_mailbox_ex\n");
	int mailbox_id;
	char mailbox_name[500] = { 0, };
	char mailbox_alias[500] = { 0, };
	char eas_data[500] = "OK. Done";
	int err;
	int handle = 0;

	testapp_print("> Enter mailbox id: ");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input. ");

	testapp_print("> Enter new mailbox name: ");
	if (0 >= scanf("%s", mailbox_name))
		testapp_print("Invalid input. ");

	testapp_print("> Enter new mailbox alias: ");
	if (0 >= scanf("%s", mailbox_alias))
		testapp_print("Invalid input. ");

	if ((err = email_rename_mailbox_ex(mailbox_id, mailbox_name, mailbox_alias, eas_data, strlen(eas_data), false, &handle)) < 0) {
		testapp_print("\n email_rename_mailbox failed[%d]\n", err);
	} else {
		testapp_print("\n email_rename_mailbox succeed\n");
	}
	return FALSE;
}

static gboolean testapp_test_interpret_command(int menu_number)
{
	gboolean go_to_loop = TRUE;

	switch (menu_number) {
		case 1:
			testapp_test_add_mailbox();
			break;

		case 2:
			testapp_test_delete_mailbox();
			break;

		case 3:
			testapp_test_rename_mailbox();
			break;

		case 4:
			testapp_test_get_imap_mailbox_list();
			break;

		case 5:
			testapp_test_set_local_mailbox();
			break;

		case 6:
			testapp_test_delete_mailbox_ex();
			break;

		case 7:
			testapp_test_get_mailbox_by_type();
			break;

		case 8:
			testapp_test_set_mailbox_type();
			break;

		case 9:
			testapp_test_set_mail_slot_size();
			break;

		case 10:
			testapp_test_get_mailbox_list();
			break;

		case 11:
			testapp_test_sync_mailbox();
			break;

		case 12:
			testapp_test_stamp_sync_time();
			break;

		case 13:
			testapp_test_rename_mailbox_ex();
			break;

		case 14:
			testapp_test_get_mailbox_list_by_keyword();
			break;

		case 0:
			go_to_loop = FALSE;
			break;
		default:
			break;
	}

	return go_to_loop;
}

void email_test_mailbox_main()
{
	gboolean go_to_loop = TRUE;
	int menu_number = 0;

	while (go_to_loop) {
		testapp_show_menu(EMAIL_MAILBOX_MENU);
		testapp_show_prompt(EMAIL_MAILBOX_MENU);

		if (0 >= scanf("%d", &menu_number))
			testapp_print("Invalid input. ");

		go_to_loop = testapp_test_interpret_command(menu_number);
	}
}

