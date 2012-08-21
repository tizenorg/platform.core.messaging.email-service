/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-update-rule.h"
#include "../TC_Utility/uts-email-dummy-utility.c"


sqlite3 *sqlite_emmb;
static int g_accountId; 

static void startup()
{
	int err_code = EMAIL_ERROR_NONE;
	int count = 0;
	int i = 0;
	email_account_t *pAccount = NULL ;
				
	tet_printf("\n TC startup");
	
	if (EMAIL_ERROR_NONE == email_service_begin()) {
		tet_infoline("Email service Begin\n");
		if (EMAIL_ERROR_NONE == email_open_db()) {   
			tet_infoline("Email open DB success\n");
			err_code = email_get_account_list(&pAccount, &count);
			if (!count) {   
				g_accountId = uts_Email_Add_Dummy_Account_01();
			}   
			else
				g_accountId = pAccount[i].account_id;
			email_free_account(&pAccount, count);

		}
		else
			tet_infoline("Email open DB failed\n");
	}
	else
		tet_infoline("Email service not started\n");    	
}

static void cleanup()
{		
	tet_printf("\n TC End");
	if (EMAIL_ERROR_NONE == email_close_db()) {
		tet_infoline("Email Close DB Success\n");
		if (EMAIL_ERROR_NONE == email_service_end())
			tet_infoline("Email service close Success\n");
		else
			tet_infoline("Email service end failed\n");
	}
	else
		tet_infoline("Email Cloase DB failed\n");		
}






/*Testcase   :		uts_Email_Update_Rule_01
  TestObjective  :	To update filter  rule information
  APIs Tested    :	int email_update_rule(int filter_id, email_rule_t *new_set) 
 */

static void uts_Email_Update_Rule_01()
{
	int err_code = EMAIL_ERROR_NONE ;
	int count = 0;
	int i = 0;
	email_rule_t *pRule_list = NULL;
	email_rule_t *pNew_rule;
	email_rule_t *pRule = NULL;
	email_mailbox_t *mailbox = NULL;

	tet_infoline("uts_Email_Update_Rule_01 Begin\n");

	tet_printf("Email Add rule\n");
	pRule = (email_rule_t  *)malloc(sizeof(email_rule_t));
	memset(pRule, 0x00, sizeof(email_rule_t));

	err_code = email_get_mailbox_by_mailbox_type(g_accountId, EMAIL_MAILBOX_TYPE_INBOX, &mailbox);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_printf("Email get mailbox by mailbox type failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}

	pRule->account_id = g_accountId;
	pRule->type = RULE_TYPE_INCLUDES;
	pRule->value = strdup("a");
	pRule->faction = EMAIL_FILTER_BLOCK;
	pRule->target_mailbox_id = mailbox->mailbox_id;
	pRule->flag1 = 1;
	pRule->flag2 = 0;
	err_code = email_add_rule(pRule);
	if (EMAIL_ERROR_NONE == err_code || EMAIL_ERROR_ALREADY_EXISTS == err_code) {
		tet_infoline("Email add rule Success\n");
	}
	else {
		tet_printf("Email add rule failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}
	email_free_rule(&pRule, 1);
	
	tet_printf("Email Get Rule List\n");
	err_code = email_get_rule_list(&pRule_list, &count);
	if (EMAIL_ERROR_NONE == err_code && count > 0) {
			tet_infoline("Email get all filter rule Success\n");
			pNew_rule = (email_rule_t *)malloc(sizeof(email_rule_t));
			memset(pNew_rule , 0x00, sizeof(email_rule_t));
			
			pNew_rule->account_id = pRule_list[i].account_id;
			pNew_rule->type = RULE_TYPE_EXACTLY;
			pNew_rule->value = strdup("ab");
			pNew_rule->target_mailbox_id =  mailbox->mailbox_id;
			pNew_rule->faction = pRule_list[i].faction;
			pNew_rule->flag1 = pRule_list[i].flag1;
			pNew_rule->flag2 = pRule_list[i].flag2;

			tet_printf("Email Update Rule\n");
			err_code = email_update_rule(pRule_list[i].filter_id, pNew_rule);
			if (EMAIL_ERROR_NONE == err_code) {
					tet_infoline("Email update rule :  Success\n");
					tet_result(TET_PASS);			
			}
			else {
				tet_printf("Email update filter rule failed : error_code[%d]\n", err_code);
				tet_result(TET_FAIL);
			}
			email_free_rule(&pNew_rule, 1);
	}
	else {
		tet_printf("Email get all filter rule failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}
	if (mailbox) {
		email_free_mailbox(&mailbox, 1);
		mailbox = NULL;
	}
}


/*Testcase   :		uts_Email_Update_Rule_02
  TestObjective  :	To validate parameter for new_set in  update filter rule information
  APIs Tested    :	int email_update_rule(int filter_id , email_rule_t *new_set) 
 */

static void uts_Email_Update_Rule_02()
{
	int err_code = EMAIL_ERROR_NONE ;
	email_rule_t *pRule = NULL;
	
	tet_infoline("uts_Email_Update_Rule_02 Begin\n");

	err_code = email_update_rule(1, NULL);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email update filter rule Success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email update filter rule failed\n");
		tet_result(TET_FAIL);
		}
}


/*Testcase   :		uts_Email_Update_Rule_03
  TestObjective  :	To validate parameter for  filter_id in update filter rule information
  APIs Tested    :	int email_update_rule(int filter_id , email_rule_t *new_set) 
 */

static void uts_Email_Update_Rule_03()
{
	int err_code = EMAIL_ERROR_NONE ;
	email_rule_t *pRule = NULL;
	
	tet_infoline("uts_Email_Update_Rule_03 Begin\n");

	err_code = email_update_rule(0, pRule);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email update filter rule Success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email update filter rule failed\n");
		tet_result(TET_FAIL);
		}
}

