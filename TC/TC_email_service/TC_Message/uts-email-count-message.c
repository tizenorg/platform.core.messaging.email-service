/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-count-message.h"
#include "../TC_Utility/uts-email-dummy-utility.c"


sqlite3 *sqlite_emmb;
static int g_accountId; 



static void startup()
{
	int err_code = EMAIL_ERROR_NONE;
	int count = 0;
	int i = 0;
	email_account_t *pAccount = NULL ;
	email_mail_list_item_t *mail_list = NULL;

	tet_printf("\n TC startup");
	if (EMAIL_ERROR_NONE == email_service_begin()) {
		tet_infoline("Email service Begin\n");
		if (EMAIL_ERROR_NONE == email_open_db()) {
			tet_infoline("Email open DB success\n");

			g_accountId = 0;
			/*  Check if there are any accounts and Get account id. If there is no account in the db, add new dummy account data to the d */
			err_code = email_get_account_list(&pAccount, &count);
			if (EMAIL_ERROR_NONE == err_code) {
				/* get the account id from the d */
				g_accountId = pAccount[i].account_id;
				email_free_account(&pAccount, count);	
				tet_printf("g_accountId[%d]\n", g_accountId);
			}
			else if (EMAIL_ERROR_ACCOUNT_NOT_FOUND == err_code) {
				/*  Add new dummy account to the db if there is no account in the d */
				tet_printf("Add new account\n");
				g_accountId = uts_Email_Add_Dummy_Account_01();
				tet_printf("g_accountId[%d]\n", g_accountId);
			}
			else {
				tet_printf("email_get_account_list() failed :  err_code[%d]\n", err_code);
			}

			/*  Check if there are any mails and Get mail id of one of them. If there is no mail in the db, add new dummy mail data to the d */
			count = 0;
			err_code = uts_Email_Get_Mail_List_03(ALL_ACCOUNT, 0, &mail_list, &count, 0);
			if (EMAIL_ERROR_NONE == err_code) {
				tet_printf("email_get_mail_list() success\n");
			}
			else if (EMAIL_ERROR_MAIL_NOT_FOUND == err_code) {
				tet_printf("Add new email\n");
				err_code = uts_Email_Add_Dummy_Message_02();
				if (EMAIL_ERROR_NONE == err_code) {	/*  Make db contain at least one mai */
						tet_printf("uts_Email_Add_Dummy_Message_02() success.\n");
					}
				else {
						tet_printf("uts_Email_Add_Dummy_Message_02() failed  : err_code[%d]\n", err_code);
				}
			}
			else {
				tet_printf("Email Get Mail List Failed  : err_code[%d]", err_code);
			}
		}
		else
			tet_infoline("Email open DB failed\n");
	}
	else {	
		tet_infoline("Email service not started\n");	
	}
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
		tet_infoline("Email Close DB failed\n");
}






/*Testcase   :		uts_Email_Count_Message_01
  TestObjective  :	To get mail count for mailbox
  APIs Tested    :	int email_count_mail(email_list_filter_t *input_filter_list, int input_filter_count, int *output_total_mail_count, int *output_unseen_mail_count)
 */

static void uts_Email_Count_Message_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int total = 0;
	int unseen = 0;
	email_list_filter_t *filter_list = NULL;

	tet_infoline("utc_Email_Count_Message_01 Begin\n");

	filter_list = malloc(sizeof(email_list_filter_t) * 3);
	memset(filter_list, 0 , sizeof(email_list_filter_t) * 3);

	filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[0].list_filter_item.rule.key_value.integer_type_value  = EMAIL_MAILBOX_TYPE_INBOX;
	filter_list[0].list_filter_item.rule.case_sensitivity              = false;

	err_code = email_count_mail(filter_list, 1, &total, &unseen);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email count message  : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email count message  :  failed error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}
}

/*Testcase   :		uts_Email_Count_Message_02
  TestObjective  :	To validate parameter for mailbox in get mail count for mailbox
  APIs Tested    :	int email_count_mail(email_list_filter_t *input_filter_list, int input_filter_count, int *output_total_mail_count, int *output_unseen_mail_count)
 */

static void uts_Email_Count_Message_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int total = 0;
	int unseen = 0;

	tet_infoline("utc_Email_Count_Message_02 Begin\n");

	err_code = email_count_mail(NULL, 1, &total, &unseen);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email count message  : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_infoline("Email count message  :  failed \n");
		tet_result(TET_FAIL);
	}
}

/*Testcase   :		uts_Email_Count_Message_03
  TestObjective  :	To validate parameter for total in  get mail count for mailbox
  APIs Tested    :	int email_count_mail(email_list_filter_t *input_filter_list, int input_filter_count, int *output_total_mail_count, int *output_unseen_mail_count)
 */

static void uts_Email_Count_Message_03()
{
	int err_code = EMAIL_ERROR_NONE;
	int unseen = 0;
	email_list_filter_t *filter_list = NULL;

	tet_infoline("utc_Email_Count_Message_03 Begin\n");

	filter_list = malloc(sizeof(email_list_filter_t) * 3);
	memset(filter_list, 0 , sizeof(email_list_filter_t) * 3);

	filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[0].list_filter_item.rule.key_value.integer_type_value  = EMAIL_MAILBOX_TYPE_SENTBOX;
	filter_list[0].list_filter_item.rule.case_sensitivity              = false;

	err_code = email_count_mail(filter_list, 1, 0, &unseen);

	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email count message  : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_infoline("Email count message  :  failed \n");
		tet_result(TET_FAIL);
	}
}

/*Testcase   :		uts_Email_Count_Message_04
  TestObjective  :	To validate parameter for seen in get mail count for mailbox
  APIs Tested    :	int email_count_mail(email_list_filter_t *input_filter_list, int input_filter_count, int *output_total_mail_count, int *output_unseen_mail_count)
 */

static void uts_Email_Count_Message_04()
{
	int err_code = EMAIL_ERROR_NONE;
	int total = 0;
	email_list_filter_t *filter_list = NULL;

	tet_infoline("utc_Email_Count_Message_04 Begin\n");

	filter_list = malloc(sizeof(email_list_filter_t) * 3);
	memset(filter_list, 0 , sizeof(email_list_filter_t) * 3);

	filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[0].list_filter_item.rule.key_value.integer_type_value  = EMAIL_MAILBOX_TYPE_SENTBOX;
	filter_list[0].list_filter_item.rule.case_sensitivity              = false;

	err_code = email_count_mail(filter_list, 1, &total, 0);

	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email count message  : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_infoline("Email count message  :  failed \n");
		tet_result(TET_FAIL);
	}
}

