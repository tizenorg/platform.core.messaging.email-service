/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-get-info.h"
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






/*Testcase   :		uts_Email_Get_Info_01
  TestObjective  :	To get  mail info 
  APIs Tested    :	int email_get_mail_data(int input_mail_id, email_mail_data_t **output_mail_data)
 */

static void uts_Email_Get_Info_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int count = 0;
	email_mail_data_t *mail_data = NULL;
	email_mail_list_item_t *mail_list = NULL;
	email_account_t *account = NULL ;

	tet_infoline("utc_Email_Get_Info_01 Begin\n");

	err_code = email_get_account_list(&account, &count);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email get account list success\n");
		if (count > 0) {
			g_accountId = account[0].account_id;
			count = 0;
			err_code = uts_Email_Get_Mail_List_03(g_accountId, EMAIL_MAILBOX_TYPE_INBOX, &mail_list, &count, 0);
			if (EMAIL_ERROR_NONE == err_code) {
				tet_infoline("Email get mail list  :  success\n");
				if (count  > 0) {
					err_code = email_get_mail_data(mail_list[0].mail_id , &mail_data);
					if (EMAIL_ERROR_NONE == err_code) {
						tet_infoline("Email get mail data  :  success\n");
						tet_result(TET_PASS);
					}
					else {
						tet_printf("Email get mail data   :  Failed err_code[%d] \n", err_code);
						tet_result(TET_FAIL);
					}

				}
				else {
					tet_infoline("No Mails in mailbox\n");
					tet_result(TET_UNRESOLVED);
				}
			}
			else {
				tet_printf("Email get mail list  :  Failed err_code[%d]", err_code);
				tet_result(TET_UNRESOLVED);
			}
		}
		else {
			tet_infoline("NO email account found : success\n");
			tet_result(TET_UNRESOLVED);
		}
	}
	else {
		tet_printf("Email get account list failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}
}


/*Testcase   :		uts_Email_Get_Info_02
  TestObjective  :	To validate parameter for mailbox in  get  mail info
  APIs Tested    :	int email_get_info(email_mailbox_t *mailbox, int mail_id, email_mail_info_t **info)  
 */

static void uts_Email_Get_Info_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int mail_id = -1;
	email_mail_data_t *mail_data = NULL;

	tet_infoline("utc_Email_Get_Info_02 Begin\n");

	err_code = email_get_mail_data(mail_id , &mail_data);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email get mail data  :  success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email get mail data   :  Failed \n");
		tet_result(TET_FAIL);
	}

}

				
/*Testcase   :		uts_Email_Get_Info_03
  TestObjective  :	To validate parameter for mail_id in  get  mail info
  APIs Tested    :	int email_get_info(email_mailbox_t *mailbox, int mail_id, email_mail_info_t **info)  
 */

static void uts_Email_Get_Info_03()
{
	int err_code = EMAIL_ERROR_NONE;

	tet_infoline("utc_Email_Get_Info_03 Begin\n");

	err_code = email_get_mail_data(1 , NULL);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email get mail info  :  success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email get mail info   :  Failed \n");
		tet_result(TET_FAIL);
	}
}

				
