/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-free-rule.h"
#include "../TC_Utility/uts-email-dummy-utility.c"


sqlite3 *sqlite_emmb;
int g_accountId;

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






/*Testcase   :		uts_Email_Free_Rule_01
  TestObjective  :	To free allcated memory for a filter rule 
  APIs Tested    :	int email_free_rule(email_rule_t *filtering_set, int count) 
 */

static void uts_Email_Free_Rule_01()
{
	int err_code = EMAIL_ERROR_NONE ;
	email_rule_t *pRule;
	email_mailbox_t *mailbox = NULL;

	tet_infoline("uts_Email_Free_Rule_01 Begin\n");

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
	
	err_code = email_free_rule(&pRule, 1);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email free rule success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_infoline("Email free rule failed\n");
		tet_result(TET_FAIL);
	}
	if (mailbox) {
		email_free_mailbox(&mailbox, 1);
		mailbox = NULL;
	}
}



