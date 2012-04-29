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

#include "email-api-rule.h"

/* internal header */
#include "testapp-utility.h"
#include "testapp-mailbox.h"

static gboolean testapp_test_add_rule()
{
	emf_rule_t*  rule = NULL;
	int account_id = 0;
	int action = 0;
	int type = 0;
	int flag = 0;
	char arg[500];

	rule = malloc(sizeof(emf_rule_t));
	testapp_print("> Enter account id: ");
	scanf("%d", &account_id);
	rule->account_id = account_id;

	testapp_print("> Enter Type(FROM - 1 / SUBJECT - 2): ");
	scanf("%d", &type);
	rule->type= type;		

	memset(arg, 0x00, 500);
	testapp_print("\n> Enter Filtering Value:");
	scanf("%s",arg);
	rule->value= strdup(arg);	

	testapp_print("> Enter Action(MOVE - 1, BLOCK - 2, DELETE - 3): ");
	scanf("%d", &action);
	rule->faction= action;	

	memset(arg, 0x00, 500);
	testapp_print("\n> Enter mailbox name:");
	scanf("%s",arg);
	rule->mailbox= strdup(arg);

	testapp_print("> Enter Flag1 value: ");
	scanf("%d", &flag);
	rule->flag1= flag;

	testapp_print("> Enter Flag2 value: ");
	scanf("%d", &flag);
	rule->flag2= flag;

	if ( email_add_rule(rule) < 0)
		testapp_print("\n EmfRuleAdd failed");

	
	email_free_rule(&rule, 1);
	
	return FALSE;
}

static gboolean testapp_test_delete_rule()
{

	int filter_id = 0;

	testapp_print("> Enter filter id: ");
	scanf("%d", &filter_id);

	if(email_delete_rule(filter_id) < 0)
		testapp_print("email_delete_rule failed..! ");
		
	return FALSE;
}


static gboolean testapp_test_update_rule()
{

	emf_rule_t*  rule = NULL;
	int account_id = 0;
	int action = 0;
	int type = 0;
	int flag = 0;
	char arg[500];
	int filter_id = 0;

	rule = malloc(sizeof(emf_rule_t));
	memset(rule,0X00,sizeof(emf_rule_t));
	testapp_print("> Enter filter id: ");
	scanf("%d", &filter_id);
	
	testapp_print("> Enter account id: ");
	scanf("%d", &account_id);
	rule->account_id = account_id;

	testapp_print("> Enter Type(FROM - 1 / SUBJECT - 2): ");
	scanf("%d", &type);
	rule->type= type;		

	memset(arg, 0x00, 500);
	testapp_print("\n> Enter Filtering Value:");
	scanf("%s",arg);
	rule->value= strdup(arg);	

	testapp_print("> Enter Action(MOVE - 1, BLOCK - 2, DELETE - 3): ");
	scanf("%d", &action);
	rule->faction= action;	

	memset(arg, 0x00, 500);
	testapp_print("\n> Enter mailbox name:");
	scanf("%s",arg);
	rule->mailbox= strdup(arg);

	testapp_print("> Enter Flag1 value: ");
	scanf("%d", &flag);
	rule->flag1= flag;

	testapp_print("> Enter Flag2 value: ");
	scanf("%d", &flag);
	rule->flag2= flag;
	
	if( !email_update_rule(filter_id, rule) < 0)
		testapp_print("email_update_rule failed..! ");
		
	email_free_rule(&rule, 1);
		
	return FALSE;
}


static gboolean testapp_test_get_rule(void)
{
	emf_rule_t*  rule = NULL;
	int filter_id = 0;

	testapp_print("> Enter filter id: ");
	scanf("%d", &filter_id);

	if(email_get_rule(filter_id, &rule) >= 0)	
		testapp_print("\n Got rule of account_id = %d and type = %d\n", rule->account_id, rule->type);

	email_free_rule(&rule, 1);
	
	return FALSE;
	
}

static gboolean testapp_test_get_rule_list	(void)
{
	int count, i;
	emf_rule_t* rule_list=NULL;

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

void emf_test_rule_main()
{
	gboolean go_to_loop = TRUE;
	int menu_number = 0;
	
	while (go_to_loop) {
		testapp_show_menu (EMF_RULE_MENU);
		testapp_show_prompt (EMF_RULE_MENU);
			
		scanf ("%d", &menu_number);

		go_to_loop = testapp_test_interpret_command (menu_number);
	}
}

