/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-cancel-send-mail.h"
#include "../TC_Utility/uts-email-dummy-utility.c"


sqlite3 *sqlite_emmb;


static void startup()
{
	tet_printf("\n TC startup");
	
	if (EMAIL_ERROR_NONE == email_service_begin()) {
		tet_infoline("Email service Begin\n");
		if (EMAIL_ERROR_NONE == email_open_db())
			tet_infoline("Email open DB success\n");
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






/*Testcase   :		uts_Email_Cancel_Send_Mail_01
  TestObjective  :	To get the total count and unread count of all mailboxes
  APIs Tested    :	int email_cancel_sending_mail(int mail_id)
 */

static void uts_Email_Cancel_Send_Mail_01()
{
	int err_code = EMAIL_ERROR_NONE;
	email_mail_list_item_t *mail_list = NULL;
	int account_id = 0;
	int count = 0;
	int i = 0;
	email_account_t *pAccount = NULL ;
	
	tet_infoline("uts_Email_Cancel_Send_Mail_01 Begin\n");

	err_code = email_get_account_list(&pAccount, &count);
	if (EMAIL_ERROR_NONE != err_code || count <= 0) {
		tet_printf("Email Get Account List Failed  : err_code[%d]", err_code);
		tet_result(TET_UNRESOLVED);
	}

	count = 0;
	err_code = uts_Email_Get_Mail_List_03(pAccount[0].account_id, EMAIL_MAILBOX_TYPE_INBOX, &mail_list, &count, 0);
	if (EMAIL_ERROR_NONE == err_code) {
		err_code = email_cancel_sending_mail(mail_list[i].mail_id);
		if (EMAIL_ERROR_NONE == err_code) {
			tet_infoline("Email Cancel Send Mail Success\n");
			tet_result(TET_PASS);
		}
		else {
			tet_printf("Email Cancel Send Mail Failed\n");
			tet_result(TET_FAIL);
		}     
	}	
	else {
		tet_printf("email_get_mail_list() Failed : failed error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}
}


/*Testcase   :		uts_Email_Cancel_Send_Mail_02
  TestObjective  :	To validate parameter for mail_id in cancel send mail
  APIs Tested    :	int email_cancel_sending_mail(int mail_id)
 */

static void uts_Email_Cancel_Send_Mail_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int mail_id = -1;
	
	tet_infoline("uts_Email_Cancel_Send_Mail_02 Begin\n");

	err_code = email_cancel_sending_mail(mail_id);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email Cancel Send Mail Success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email Cancel Send Mail  failed : \n");
		tet_result(TET_FAIL);
	}				
}


