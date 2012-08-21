/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/

/***************************************
* add your header file here
* e.g., 
* #include "uts_ApplicationLib_recurGetDayOfWeek_func.h"
***************************************/
#include "uts_email_get_imap_folder_list.h"
#include "../TC_Utility/uts-email-real-utility.c"


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
				g_accountId = uts_Email_Add_Real_Account_01();
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




/***************************************
* add your Test Cases
* e.g., 
* static void uts_ApplicationLib_recurGetDayOfWeek_01()
* {
* 	int ret;
*	
* 	ret = target_api();
* 	if (ret == 1)
* 		tet_result(TET_PASS);
* 	else
* 		tet_result(TET_FAIL);	
* }
*
* static void uts_ApplicationLib_recurGetDayOfWeek_02()
* {
* 		code..
*		condition1
* 			tet_result(TET_PASS);
*		condition2
*			tet_result(TET_FAIL);
* }
*
***************************************/


/*Testcase   :		uts_Email_Get_Imap_Folder_List_01
  TestObjective  :	To get imap mailbox list
  APIs Tested    :	int  email_get_imap_mailbox_list(int account_id, unsigned *handle)
 */

static void uts_Email_Get_Imap_Folder_List_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle;
	email_account_t *account = NULL ;
	int count = 0;

	tet_infoline("uts_email_get_imap_folder_list_01 Begin\n");
	err_code = email_get_account_list(&account, &count);
	if (EMAIL_ERROR_NONE != err_code || count <=0 ) {
		tet_printf("email_get_account_list failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}

	err_code = email_sync_imap_mailbox_list(account[0].account_id, &handle);
			if (EMAIL_ERROR_NONE == err_code) {
				tet_infoline("Email get Imap mailbox list  :  success\n ");
				tet_result(TET_PASS);	
			}
			else {
				tet_printf("Email get Imap mailbox list  :  failed err_code[%d]\n", err_code);
				tet_result(TET_FAIL);
			}
		}

/*Testcase   :		uts_email_get_imap_folder_list_02
  TestObjective  :	To validate parametr for account_id in get imap mailbox list
  APIs Tested    :	int  email_get_imap_mailbox_list(int account_id, unsigned *handle)
 */

static void uts_Email_Get_Imap_Folder_List_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle;
	int account_id = -1;


	tet_infoline("uts_email_get_imap_folder_list_02 Begin\n");


	err_code = email_sync_imap_mailbox_list(account_id, &handle);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email get Imap mailbox list  :  success\n ");
		tet_result(TET_PASS);	
	}
	else {
		tet_printf("Email get Imap mailbox list  :  failed \n");
		tet_result(TET_FAIL);
	}
}


