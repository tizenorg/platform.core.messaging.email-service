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

#include "email-api-rule.h"

/* internal header */
#include "testapp-utility.h"
#include "testapp-mailbox.h"

static gboolean testapp_test_add_rule()
{
	email_rule_t*  rule = NULL;
	int account_id = 0;
	int target_mailbox_id = 0;
	int action = 0;
	int type = 0;
	int flag = 0;
	char arg[500];

	rule = malloc(sizeof(email_rule_t));
	testapp_print("> Enter account id: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");
	rule->account_id = account_id;

	testapp_print("> Enter Type(FROM - 1 / SUBJECT - 2): ");
	if (0 >= scanf("%d", &type))
		testapp_print("Invalid input. ");
	rule->type= type;		

	memset(arg, 0x00, 500);
	testapp_print("\n> Enter Filtering Value:");
	if (0 >= scanf("%s",arg))
		testapp_print("Invalid input. ");
	rule->value= strdup(arg);	

	testapp_print("> Enter Action(MOVE - 1, BLOCK - 2, DELETE - 3): ");
	if (0 >= scanf("%d", &action))
		testapp_print("Invalid input. ");
	rule->faction= action;	

	if (action == 1) {
		testapp_print("\n> Enter target mailbox id:");
		if (0 >= scanf("%d", &target_mailbox_id))
			testapp_print("Invalid input. ");
		rule->target_mailbox_id = target_mailbox_id;
	}

	testapp_print("> Enter Flag1 value [On/Off]: ");
	if (0 >= scanf("%d", &flag))
		testapp_print("Invalid input. ");
	rule->flag1= flag;

	testapp_print("> Enter Flag2 value [0:Exactly same as, 1:Include, 2:Compare Domain] : ");
	if (0 >= scanf("%d", &flag))
		testapp_print("Invalid input. ");
	rule->flag2= flag;

	if (email_add_rule(rule) < 0)
		testapp_print("\n email_add_rule failed");

	testapp_print("ID of rule : [%d]\n", rule->filter_id);

	if (email_apply_rule(rule->filter_id) != EMAIL_ERROR_NONE)
		testapp_print("email_apply_rule failed\n");
	
	email_free_rule(&rule, 1);
	
	return FALSE;
}

static gboolean testapp_test_delete_rule()
{
	int filter_id = 0;

	testapp_print("> Enter filter id: ");
	if (0 >= scanf("%d", &filter_id))
		testapp_print("Invalid input. ");

	if(email_delete_rule(filter_id) < 0)
		testapp_print("email_delete_rule failed..! ");
		
	return FALSE;
}


static gboolean testapp_test_update_rule()
{
	email_rule_t*  rule = NULL;
	int account_id = 0;
	int target_mailbox_id = 0;
	int action = 0;
	int type = 0;
	int flag = 0;
	char arg[500];
	int filter_id = 0;

	rule = malloc(sizeof(email_rule_t));
	memset(rule,0X00,sizeof(email_rule_t));
	testapp_print("> Enter filter id: ");
	if (0 >= scanf("%d", &filter_id))
		testapp_print("Invalid input. ");
	
	testapp_print("> Enter account id: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");
	rule->account_id = account_id;

	testapp_print("> Enter Type(FROM - 1 / SUBJECT - 2): ");
	if (0 >= scanf("%d", &type))
		testapp_print("Invalid input. ");
	rule->type= type;		

	memset(arg, 0x00, 500);
	testapp_print("\n> Enter Filtering Value:");
	if (0 >= scanf("%s",arg))
		testapp_print("Invalid input. ");
	rule->value= strdup(arg);	

	testapp_print("> Enter Action(MOVE - 1, BLOCK - 2, DELETE - 3): ");
	if (0 >= scanf("%d", &action))
		testapp_print("Invalid input. ");
	rule->faction= action;	

	if (action == 1) {
		testapp_print("\n> Enter target mailbox id:");
		if (0 >= scanf("%d", &target_mailbox_id))
			testapp_print("Invalid input. ");
		rule->target_mailbox_id = target_mailbox_id;
	}

	testapp_print("> Enter Flag1 value: ");
	if (0 >= scanf("%d", &flag))
		testapp_print("Invalid input. ");
	rule->flag1= flag;

	testapp_print("> Enter Flag2 value: ");
	if (0 >= scanf("%d", &flag))
		testapp_print("Invalid input. ");
	rule->flag2= flag;
	
	if( !email_update_rule(filter_id, rule) < 0)
		testapp_print("email_update_rule failed..! ");
		
	email_free_rule(&rule, 1);
		
	return FALSE;
}


static gboolean testapp_test_get_rule(void)
{
	email_rule_t*  rule = NULL;
	int filter_id = 0;

	testapp_print("> Enter filter id: ");
	if (0 >= scanf("%d", &filter_id))
		testapp_print("Invalid input. ");

	if(email_get_rule(filter_id, &rule) >= 0)	
		testapp_print("\n Got rule of account_id = %d and type = %d\n", rule->account_id, rule->type);

	email_free_rule(&rule, 1);
	
	return FALSE;
	
}

static gboolean testapp_test_get_rule_list	(void)
{
	int count, i;
	email_rule_t* rule_list=NULL;

	if(email_get_rule_list(&rule_list, &count) < 0) {
		testapp_print("   email_get_rule_list error\n");
		return false ;
	}
	
	for(i=0;i<count;i++){
		testapp_print("   %2d) Fileter_Id: %d | Account_id: %d  | Type: %d | Value %s \n", i+1, 
			rule_list[i].filter_id,
			rule_list[i].account_id, 
			rule_list[i].type,
			rule_list[i].value);
	}

	email_free_rule(&rule_list, count);
	return FALSE;

}



static gboolean testapp_test_interpret_command (int menu_number)
{
	gboolean go_to_loop = TRUE;
	
	switch (menu_number) {
		case 1:
			testapp_test_add_rule();
			break;
		case 2:
			testapp_test_delete_rule ();
			break;
		case 3:
			testapp_test_update_rule();
			break;
		case 5:
			testapp_test_get_rule ();
			break;
		case 6:
			testapp_test_get_rule_list();
			break;

		case 0:
			go_to_loop = FALSE;
			break;
		default:
			break;
	}

	return go_to_loop;
}

void email_test_rule_main()
{
	gboolean go_to_loop = TRUE;
	int menu_number = 0;
	
	while (go_to_loop) {
		testapp_show_menu (EMAIL_RULE_MENU);
		testapp_show_prompt (EMAIL_RULE_MENU);
			
		if (0 >= scanf("%d", &menu_number))
			testapp_print("Invalid input. ");

		go_to_loop = testapp_test_interpret_command (menu_number);
	}
}

