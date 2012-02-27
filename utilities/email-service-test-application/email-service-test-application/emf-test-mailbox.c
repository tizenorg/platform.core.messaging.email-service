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


/* open header */
#include <glib.h>

#include "Emf_Mapi_Message.h"
#include "Emf_Mapi_Network.h"
#include "Emf_Mapi_Mailbox.h"

/* internal header */
#include "emf-test-utility.h"
#include "emf-test-mailbox.h"


static gboolean testapp_test_add_mailbox()
{
	emf_mailbox_t  mbox;
	int account_id,mailbox_type = 0;
	int local_yn = 0;
	char arg[500];
	int ret;
    unsigned handle;

	memset(arg, 0x00, 500);
	testapp_print("\n> Enter mailbox name: ");
	scanf("%s",arg);
	mbox.name = strdup(arg);
	
	memset(arg, 0x00, 500);
	testapp_print("> Enter mailbox alias name: ");
	scanf("%s",arg);
	mbox.alias = strdup(arg);

	testapp_print("> Enter account id: ");
	scanf("%d", &account_id);
	mbox.account_id = account_id;

	testapp_print("> Enter local_yn (1/0): ");
	scanf("%d", &local_yn);
	mbox.local= local_yn;	


	testapp_print("> Enter mailbox type: ");
	scanf("%d", &mailbox_type);
	mbox.mailbox_type= mailbox_type;

	ret = email_add_mailbox(&mbox, local_yn?0:1, &handle);

	if (ret  < 0) {
		testapp_print("\n email_add_mailbox failed");
	}
	else {
		testapp_print("\n email_add_mailbox succeed : handle[%d]\n", handle);
	}
	
	return FALSE;
}

static gboolean testapp_test_delete_mailbox()
{

	emf_mailbox_t  mbox;
	int account_id;
	int local_yn = 0;
	char arg[500];
	int ret;
	unsigned handle;

	memset(arg, 0x00, 500);
	testapp_print("\n> Enter mailbox name:");
	scanf("%s",arg);
	mbox.name = strdup(arg);
	
	testapp_print("> Enter account id: ");
	scanf("%d", &account_id);
	mbox.account_id = account_id;

	testapp_print("> Enter local_yn (1/0): ");
	scanf("%d", &local_yn);

	ret = email_delete_mailbox(&mbox, local_yn?0:1, &handle);

	if ( ret < 0) {
		testapp_print("\n email_delete_mailbox failed");
	}
	else {
		testapp_print("\n email_delete_mailbox succeed : handle[%d]\n", handle);
	}
	
	return FALSE;

}

static gboolean testapp_test_update_mailbox()
{
	testapp_print ("testapp_test_update_mailbox - support ONLY updating mailbox type\n");
	emf_mailbox_t  *old_mailbox_name = NULL;
	emf_mailbox_t  *new_mbox = NULL;
	int account_id,mailbox_type = 0;
	char arg[500];
	int err;

	memset(arg, 0x00, 500);
	
	testapp_print("> Enter account id: ");
	scanf("%d", &account_id);

	testapp_print("\n> Enter mailbox name: ");
	scanf("%s", arg);
	
	testapp_print("> Enter mailbox type: ");
	scanf("%d", &mailbox_type);

	/*  Get old mailbox information from db */
	if ( (err = email_get_mailbox_by_name(account_id, arg, &old_mailbox_name)) < 0 ) {
		testapp_print("\n email_get_mailbox_by_name failed[%d]\n", err);
	}
	else {
		testapp_print("\n email_get_mailbox_by_name succeed\n");
	}
	
	/*  copy old maibox to new mailbox */
	if ( (err = email_get_mailbox_by_name(account_id, arg, &new_mbox)) < 0 ) {
		testapp_print("\n email_get_mailbox_by_name failed[%d]\n", err);
	}
	else {
		testapp_print("\n email_get_mailbox_by_name succeed\n");
	}
	
	/*  set new value of new mailbox */
	new_mbox->mailbox_type= mailbox_type;
	
	if ( (err = email_update_mailbox(old_mailbox_name, new_mbox)) < 0) {
		testapp_print("\n email_update_mailbox failed[%d]\n", err);
	}
	else {
		testapp_print("\n email_update_mailbox succeed\n");
	}

	email_free_mailbox(&old_mailbox_name, 1);
	email_free_mailbox(&new_mbox, 1);

	return FALSE;
}

static gboolean testapp_test_get_imap_mailbox_list()
{
	int account_id = 0;
	unsigned handle = 0;
	
	testapp_print("> Enter account id: ");
	scanf("%d", &account_id);	
	
	if(  email_get_imap_mailbox_list(account_id, "", &handle) < 0)
		testapp_print("email_get_imap_mailbox_list failed");

	return FALSE;

}

static gboolean testapp_test_get_child_mailbox_list ()
{

	int account_id =0;
	int count = 0;
	int i = 0, err_code = EMF_ERROR_NONE;
	emf_mailbox_t *mailbox_list=NULL;
	char parent_mailbox[100], *parent_mailbox_pointer = NULL;

	
	memset(parent_mailbox,0x00,sizeof(parent_mailbox));
	
	testapp_print("\n > Enter account id: ");
	scanf("%d", &account_id);

	testapp_print("\n > Enter parent mailbox name to fetch child list: ");
	scanf("%s", parent_mailbox);

	

	if(strcmp(parent_mailbox, "0") != 0) {
		testapp_print("\ninput : %s\n", parent_mailbox);
		parent_mailbox_pointer = parent_mailbox;
	}

	if( (err_code = email_get_child_mailbox_list(account_id,parent_mailbox_pointer, &mailbox_list, &count)) < 0) {
		testapp_print("   email_get_child_mailbox_list error : %d\n",err_code);
		return false ;
	}

	testapp_print("There are %d mailboxes\n", count);

	testapp_print("============================================================================\n");
	testapp_print("number\taccount_id\t name\t alias\t local_yn\t unread\t mailbox_type\thas_archived_mails\n");
	testapp_print("============================================================================\n");
	if(count == 0) {
		testapp_print("No mailbox is matched\n");
	}
	else {
		for(i=0;i<count;i++)
		{
			testapp_print("[%d] - ", i);
			testapp_print(" %2d\t [%-15s]\t[%-15s]\t", mailbox_list[i].account_id, mailbox_list[i].name, mailbox_list[i].alias);
			testapp_print(" %d\t %d\t %d\n", mailbox_list[i].local, mailbox_list[i].unread_count,mailbox_list[i].mailbox_type, mailbox_list[i].has_archived_mails);
		}
	}
	testapp_print("============================================================================\n");
	
	email_free_mailbox(&mailbox_list, count);
	return FALSE;
}

static gboolean testapp_test_get_mailbox_by_type ()
{

	int account_id =0;	
	int i = 0, err_code = EMF_ERROR_NONE;
	emf_mailbox_t *mailbox =NULL;
	emf_mailbox_type_e mailbox_type =0;
	
	testapp_print("\n > Enter account id: ");
	scanf("%d", &account_id);

	testapp_print("\n > Enter mailbox_type: ");
	scanf("%d", (int*)&mailbox_type);

	if( (err_code = email_get_mailbox_by_mailbox_type(account_id,mailbox_type,&mailbox)) < 0) {
		testapp_print("   email_get_mailbox_by_mailbox_type error : %d\n",err_code);
		return false ;
	}

	testapp_print("============================================================================\n");
	testapp_print("number\taccount_id\t name\t alias\t local_yn\t unread\t mailbox_type\thas_archived_mails\n");
	testapp_print("============================================================================\n");
	
	testapp_print("[%d] - ", i);
	testapp_print(" %2d\t [%-15s]\t[%-15s]\t", mailbox->account_id, mailbox->name, mailbox->alias);
	testapp_print(" %d\t %d\t %d\n", mailbox->local, mailbox->unread_count,mailbox->mailbox_type, mailbox->has_archived_mails);
		
	testapp_print("============================================================================\n");
	
	email_free_mailbox(&mailbox, 1);
	return FALSE;
}

static gboolean testapp_test_set_mail_slot_size ()
{

	int account_id = 0, mail_slot_size = 0;	
	int err_code = EMF_ERROR_NONE;
	char arg[500];
	char *mailbox_name = NULL;
	
	testapp_print("\n > Enter account id (0: All account): ");
	scanf("%d", &account_id);

	memset(arg, 0x00, 500);
	testapp_print("\n> Enter mailbox name (0 : All mailboxes):");
	scanf("%s",arg);
	if (strcmp(arg, "0") != 0 ) {
		mailbox_name = arg;
	}

	testapp_print("\n > Enter mailbox slot size: ");
	scanf("%d", &mail_slot_size);

	if( (err_code = email_set_mail_slot_size(account_id, mailbox_name, mail_slot_size) ) < 0) {
		testapp_print("   testapp_test_set_mail_slot_size error : %d\n", err_code);
		return false ;
	}

	return FALSE;
}


static gboolean testapp_test_interpret_command (int menu_number)
{
	gboolean go_to_loop = TRUE;
	
	switch (menu_number) {
		case 1:
			testapp_test_add_mailbox();
			break;

		case 2:
			testapp_test_delete_mailbox ();
			break;

		case 3:
			testapp_test_update_mailbox ();
			break;

		case 4:
			testapp_test_get_imap_mailbox_list();
			break;			

		case 6:
			testapp_test_get_child_mailbox_list();
			break;	

		case 7:
			testapp_test_get_mailbox_by_type();
			break;	

		case 8:
			testapp_test_set_mail_slot_size();
			break;	

		case 0:
			go_to_loop = FALSE;
			break;
		default:
			break;
	}

	return go_to_loop;
}

void emf_test_mailbox_main()
{
	gboolean go_to_loop = TRUE;
	int menu_number = 0;
	
	while (go_to_loop) {
		testapp_show_menu (EMF_MAILBOX_MENU);
		testapp_show_prompt (EMF_MAILBOX_MENU);
			
		scanf ("%d", &menu_number);

		go_to_loop = testapp_test_interpret_command (menu_number);
	}
}

