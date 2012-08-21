/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-get-mailbox-list.h"
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
			tet_infoline("Email Close DB failed\n");		
}








/*Testcase   :		uts_Email_Get_Mailbox_List_01
  TestObjective  :	To get  all local folderlist
  APIs Tested    :	int email_get_mailbox_list(int account_id, int local_yn, email_mailbox_t **mailbox_list, int *count) 
 */

static void uts_Email_Get_Mailbox_List_01()
{
	int err_code = EMAIL_ERROR_NONE ;
	email_mailbox_t *pMailbox_list = NULL;
	int count = 0;
	int local_yn = 0;
		
	tet_infoline("uts_Email_Get_Mailbox_List_01 Begin\n");
	
			err_code = email_get_mailbox_list(g_accountId, local_yn, &pMailbox_list, &count);
			if (EMAIL_ERROR_NONE == err_code) {	
				tet_infoline("Email get all local mailboxes  :  Success\n");	
				tet_result(TET_PASS);
			}
			else {
				tet_printf("Email get all local mailboxes failed : error_code[%d]\n", err_code);
					tet_result(TET_FAIL);
			}
		
}


/*Testcase   :		uts_Email_Get_Mailbox_List_02
  TestObjective  :	To get  all server mailbox list as well as local folderlist
  APIs Tested    :	int email_get_mailbox_list(int account_id, int local_yn, email_mailbox_t **mailbox_list, int *count) 
 */

static void uts_Email_Get_Mailbox_List_02()
{
	int err_code = EMAIL_ERROR_NONE ;
	email_mailbox_t *pMailbox_list = NULL;
	int local_yn = 1;
	int count = 0;

	tet_infoline("uts_Email_Get_Mailbox_List_02 Begin\n");

	err_code = email_get_mailbox_list(g_accountId , local_yn, &pMailbox_list, &count);
	if (EMAIL_ERROR_NONE == err_code) {	
		tet_infoline("Email get all server & local mailboxes  :  Success\n");	
		tet_result(TET_PASS);
	}
	else {
			tet_printf("Email get all server & local mailboxes failed : error_code[%d]\n", err_code);
				tet_result(TET_FAIL);
   	}
}


/*Testcase   :		uts_Email_Get_Mailbox_List_03
  TestObjective  :	To validate parameter for account_id in  get all mailbox list
  APIs Tested    :	int email_get_mailbox_list(int account_id, int local_yn, email_mailbox_t **mailbox_list, int *count) 
 */

static void uts_Email_Get_Mailbox_List_03()
{
	int err_code = EMAIL_ERROR_NONE ;
	email_mailbox_t *pMailbox_list = NULL;
	int local_yn = 1;
	int count = 0;
	int account_id = -1;

	tet_infoline("uts_Email_Get_Mailbox_List_03 Begin\n");

	err_code = email_get_mailbox_list(account_id , local_yn, &pMailbox_list, &count);
	if (EMAIL_ERROR_NONE != err_code) {	
		tet_infoline("Email get all mailboxes validate parameter  :  Success\n");	
			tet_result(TET_PASS);
	}
	else {
			tet_printf("Email get all mailboxes validate parameter failed : \n");
			tet_result(TET_FAIL);
	}	
}


/*Testcase   :		uts_Email_Get_Mailbox_List_04
  TestObjective  :	To validate parameter for mailbox_list in  get all mailbox list
  APIs Tested    :	int email_get_mailbox_list(int account_id, int local_yn, email_mailbox_t **mailbox_list, int *count) 
 */

static void uts_Email_Get_Mailbox_List_04()
{
	int err_code = EMAIL_ERROR_NONE ;
	int local_yn = 1;
	int count = 0;
	int account_id = 1;

	tet_infoline("uts_Email_Get_Mailbox_List_04 Begin\n");

	err_code = email_get_mailbox_list(account_id , local_yn, NULL, &count);
	if (EMAIL_ERROR_NONE != err_code) {	
		tet_infoline("Email get all mailboxes validate parameter  :  Success\n");	
			tet_result(TET_PASS);
	}
	else {
			tet_printf("Email get all mailboxes validate parameter failed : \n");
			tet_result(TET_FAIL);
	}	
}


/*Testcase   :		uts_Email_Get_Mailbox_List_05
  TestObjective  :	To validate parameter for count in  get all mailbox list
  APIs Tested    :	int email_get_mailbox_list(int account_id, int local_yn, email_mailbox_t **mailbox_list, int *count) 
 */

static void uts_Email_Get_Mailbox_List_05()
{
	int err_code = EMAIL_ERROR_NONE ;
	email_mailbox_t *pMailbox_list = NULL;
	int local_yn = 1;
	int account_id = 1;

	tet_infoline("uts_Email_Get_Mailbox_List_05 Begin\n");

	err_code = email_get_mailbox_list(account_id , local_yn, &pMailbox_list, 0);
	if (EMAIL_ERROR_NONE != err_code) {	
		tet_infoline("Email get all mailboxes validate parameter  :  Success\n");	
			tet_result(TET_PASS);
	}
	else {
			tet_printf("Email get all mailboxes validate parameter failed : \n");
			tet_result(TET_FAIL);
	}	
}
