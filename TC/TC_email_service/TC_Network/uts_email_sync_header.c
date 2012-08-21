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
#include "uts_email_sync_header.h"
#include "../TC_Utility/uts-email-real-utility.c"


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


/*Testcase   :		uts_Email_Sync_Header_01
  TestObjective  :	To download email headers
  APIs Tested    :	int email_sync_header(int input_account_id, int input_maibox_id, unsigned* handle)
 */

static void uts_Email_Sync_Header_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle;
	email_mailbox_t *mailbox = NULL;
	email_account_t *account = NULL ;
	int count = 0;

	tet_infoline("uts_email_sync_header_01 Begin\n");
	err_code = email_get_account_list(&account, &count);
	if (EMAIL_ERROR_NONE != err_code || count <=0 ) {
		tet_printf("email_get_account_list failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}

	err_code = email_get_mailbox_by_mailbox_type(account[0].account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_printf("Email get mailbox by mailbox type failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}

	err_code = email_sync_header(g_accountId, mailbox->mailbox_id , &handle);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email sync mail header : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email sync mail header failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}
	if (mailbox) {
		email_free_mailbox(&mailbox, 1);
		mailbox = NULL;
	}
}


/*Testcase   :		uts_Email_Sync_Header_02
  TestObjective  :	To validate parameter for mailbox account_id  in download email headers
  APIs Tested    :	int email_sync_header(int input_account_id, int input_maibox_id, unsigned* handle)
 */

static void uts_Email_Sync_Header_02()
{
	int err_code = EMAIL_ERROR_NONE;
	email_mailbox_t mbox;
	int handle;

	tet_infoline("uts_Email_Sync_Header_02 Begin\n");

	err_code = email_sync_header(-1, 0, &handle);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email sync mail header : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email sync mail header failed : \n");
		tet_result(TET_FAIL);
	}
}

/*Testcase   :		uts_Email_Sync_Header_03
  TestObjective  :	To validate parameter for mailbox   in download email headers
  APIs Tested    :	int email_sync_header(int input_account_id, int input_maibox_id, unsigned* handle)
 */

static void uts_Email_Sync_Header_03()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle;

	tet_infoline("uts_Email_Sync_Header_03 Begin\n");


	err_code = email_sync_header(-1, -1 , &handle);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email sync mail header : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email sync mail header failed : \n");
		tet_result(TET_FAIL);
	}
}

