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
#include "uts_email_send_saved.h"
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


/*Testcase   :		uts_Email_Send_Saved_01
  TestObjective  :	To send all saved mails
  APIs Tested    :	int email_send_saved(int account_id, email_option_t *sending_option, unsigned *handle)
  */

static void uts_Email_Send_Saved_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle;
	email_option_t option;
	email_account_t *account = NULL ;
	int count = 0;

	tet_infoline("uts_email_send_saved_01 Begin\n");
	
	err_code = email_get_account_list(&account, &count);
	if (EMAIL_ERROR_NONE != err_code || count <=0 ) {
		tet_printf("email_get_account_list failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}

	memset(&option , 0x00, sizeof(email_option_t));
	option.keep_local_copy = 1;
	
	err_code = email_send_saved(account[0].account_id, &option, &handle);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email send and saved :  success\n");					
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email send and saved failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}

}

/*Testcase   :		uts_Email_Send_Saved_02
  TestObjective  :	To validate parameter for account_id in send all saved mails
  APIs Tested    :	int email_send_saved(int account_id, email_option_t *sending_option, unsigned *handle)
  */

static void uts_Email_Send_Saved_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle;
	int account_id = -1;
	email_option_t option;
	
	tet_infoline("uts_email_send_saved_02 Begin\n");

	memset(&option , 0x00, sizeof(email_option_t));
	option.keep_local_copy = 1;
	
	err_code = email_send_saved(account_id, &option, &handle);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email send and saved :  success\n");					
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email send and saved failed : \n");
		tet_result(TET_FAIL);
	}
}


/*Testcase   :		uts_email_send_saved_03
  TestObjective  :	To validate parameter for sending_option in send all saved mails
  APIs Tested    :	int email_send_saved(int account_id, email_option_t *sending_option, unsigned *handle)
  */

static void uts_Email_Send_Saved_03()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle;
	
	tet_infoline("uts_email_send_saved_03 Begin\n");

	
	err_code = email_send_saved(g_accountId, NULL, &handle);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email send and saved :  success\n");					
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email send and saved failed : \n");
		tet_result(TET_FAIL);
	}
}
