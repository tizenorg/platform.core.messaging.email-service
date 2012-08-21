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
#include "uts_email_download_attachment.h"
#include "../TC_Utility/uts-email-real-utility.c"
#include "../TC_Utility/uts-email-dummy-utility.c"

sqlite3 *sqlite_emmb; 
int g_accountId;
int mail_id ;

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
			mail_id = uts_Email_Add_Real_Message_02();			
			}
			else {
				g_accountId = pAccount[i].account_id;
			mail_id = uts_Email_Add_Real_Message_02();			
			
			}
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


/*Testcase   :		uts_email_download_attachment_01
  TestObjective  :	To download attachment part of email
  APIs Tested    :	int email_download_attachment(int mail_id, int nth, unsigned* handle)
 */
static void uts_Email_Download_Attachment_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle;
	int i = 0;
	email_mail_list_item_t *mail_list = NULL;
	int count = 0;
	
	tet_infoline("uts_Email_Download_Attachment_01 Begin\n");

	err_code = uts_Email_Get_Mail_List_03(g_accountId, EMAIL_MAILBOX_TYPE_INBOX, &mail_list, &count, 0);
	if (EMAIL_ERROR_NONE != err_code || count < 0) {
		tet_printf("Email Get Account List Failed :  err_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
		return;
	}

	for (i ; i < count ; i++) {
		if(mail_list[i].has_attachment) {
			err_code = email_download_attachment(mail_list[i].mail_id, 1, &handle);
			if (EMAIL_ERROR_NONE == err_code) {
				tet_infoline("Email download mail attachment success\n");
				tet_result(TET_PASS);
				return ;
			}
			else
			{
				tet_printf("Email download mail attachment failed : error_code[%d]\n", err_code);
				tet_result(TET_FAIL);
				return;
			}
		}
	}
}



/*Testcase   :		uts_email_download_attachment_02
  TestObjective  :	To validate parameter account_id for email download attachment
  APIs Tested    :	int email_download_attachment(int mail_id, int nth, unsigned* handle)
 */
static void uts_Email_Download_Attachment_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle;
	int mail_id = 0;  /* validation fiel */
	
	tet_infoline("uts_Email_Download_Attachment_02 Begin\n");

	err_code = email_download_attachment(mail_id, 1, &handle);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email download mail attachment success\n");		
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email download attachment failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}
}




/*Testcase   :		uts_email_download_attachment_03
  TestObjective  :	To validate parameter mailbox for email download attachment
  APIs Tested    :	int email_download_attachment(int mail_id, int nth, unsigned* handle)
 */
static void uts_Email_Download_Attachment_03()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle;
	
	tet_infoline("uts_Email_Download_Attachment_03 Begin\n");


	err_code = email_download_attachment(1, 0, &handle);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email download mail attachment success\n");		
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email download attachment failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}
}
