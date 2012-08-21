/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-get-mail-list-ex.h"
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






/*Testcase   :		uts_Email_Get_Mail_List_Ex_01
  TestObjective  :	Get the Mail List information from DB based on the mailbox mailbox_name
  APIs Tested    :	int email_get_mail_list_ex(email_list_filter_t *input_filter_list, int input_filter_count, email_list_sorting_rule_t *input_sorting_rule_list,
   int input_sorting_rule_count, int input_start_index, int input_limit_count, email_mail_list_item_t** output_mail_list, int *output_result_count)
 */

static void uts_Email_Get_Mail_List_Ex_01()
{
	int err_code = EMAIL_ERROR_NONE;
	email_mail_list_item_t *mail_list = NULL;
	int count = 0;
	int start_index = 0;
	int limit_count = 2;
	email_list_filter_t *filter_list = NULL;
		
	tet_infoline("uts_Email_Get_Mail_Ex_01\n");
	
	filter_list = malloc(sizeof(email_list_filter_t) * 1);
	memset(filter_list, 0 , sizeof(email_list_filter_t) * 1);

	filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[0].list_filter_item.rule.key_value.integer_type_value  = EMAIL_MAILBOX_TYPE_INBOX;
	filter_list[0].list_filter_item.rule.case_sensitivity              = false;

	err_code = email_get_mail_list_ex(filter_list, 1, NULL, 0, start_index, limit_count, &mail_list , &count);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email get  mail list ex Success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email get  mail list ex  failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}  
	email_free_list_filter(&filter_list, 1);
}


/*Testcase   :		uts_Email_Get_Mail_List_Ex_02
  TestObjective  :	Validate parameter for mail_list in Get the Mail List information from DB based on the mailbox mailbox_name
  APIs Tested    :	email_get_mail_list_ex(int account_id , const char *mailbox_mailbox_name, int thread_id, int start_index, int limit_count, email_sort_type_t sorting, 
  			email_mail_list_item_t **mail_list,  int *result_count)
 */
static void uts_Email_Get_Mail_List_Ex_02()
{
	int err_code = EMAIL_ERROR_NONE;
	email_mail_list_item_t *mail_list = NULL;
	int count = 0;
	int start_index = 0;
	int limit_count = 2;
		
	tet_infoline("uts_Email_Get_Mail_Ex_02\n");
	
	count = 0;
	err_code = email_get_mail_list_ex(NULL, 1, NULL, 0, start_index, limit_count, &mail_list , &count);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email get  mail list ex Success\n");
		tet_result(TET_PASS);
	}
	else {
			tet_printf("Email get  mail list ex  failed : error_code[%d]\n", err_code);
			tet_result(TET_FAIL);
	}  

}

/*Testcase   :		uts_Email_Get_Mail_List_Ex_03
  TestObjective  :	Validate parameter for result_count in Get the Mail List information from DB based on the mailbox mailbox_name
  APIs Tested    :	email_get_mail_list_ex(int account_id , const char *mailbox_mailbox_name, int thread_id, int start_index, int limit_count, email_sort_type_t sorting, 
  			email_mail_list_item_t **mail_list,  int *result_count)
 */

static void uts_Email_Get_Mail_List_Ex_03()
{
	int err_code = EMAIL_ERROR_NONE;
	email_mail_list_item_t *mail_list = NULL;
	int count = 0;
	int start_index = 0;
	int limit_count = 2 ;
	email_list_filter_t *filter_list = NULL;

	tet_infoline("uts_Email_Get_Mail_Ex_03\n");


	filter_list = malloc(sizeof(email_list_filter_t) * 1);
	memset(filter_list, 0 , sizeof(email_list_filter_t) * 1);

	filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[0].list_filter_item.rule.key_value.integer_type_value  = EMAIL_MAILBOX_TYPE_INBOX;
	filter_list[0].list_filter_item.rule.case_sensitivity              = false;
	count = 0;			
	err_code = email_get_mail_list_ex(filter_list, 1, NULL, 0, start_index, limit_count, &mail_list , 0);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email get  mail list ex Success\n");
		tet_result(TET_PASS);
	}
	else {
			tet_printf("Email get  mail list ex  failed : error_code[%d]\n", err_code);
			tet_result(TET_FAIL);
	}

	email_free_list_filter(&filter_list, 1);

}






