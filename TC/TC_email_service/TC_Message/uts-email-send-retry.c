/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-send-retry.h"
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






/*Testcase   :		uts_Email_Send_Retry_01
  TestObjective  :	To Retry mail send
  APIs Tested    :	int email_retry_sending_mail(int mail_id, int timeout_in_sec)
 */

static void uts_Email_Send_Retry_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int count = 0;
	int i = 0;
	email_mail_list_item_t *mail_list = NULL;
	
	tet_infoline("uts_Email_Send_Retry_01\n");

	err_code = uts_Email_Get_Mail_List_03(ALL_ACCOUNT, 0, &mail_list, &count, 0);
	if (EMAIL_ERROR_NONE == err_code) {	
		if (count > 0) {

						err_code = email_retry_sending_mail(mail_list[i].mail_id , 3);
						if (EMAIL_ERROR_NONE == err_code) {
				tet_infoline("Email Send Retry Success\n");
					tet_result(TET_PASS);
				}
					else {
				tet_printf("Email Send Retry failed : error_code[%d]\n", err_code);
				tet_result(TET_FAIL);
				}  
		}
		else {
			tet_infoline("No mail found\n");
			tet_result(TET_UNRESOLVED);
		}
	}
	else {
		tet_printf("Email get mail list failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);	
	}	
}


/*Testcase   :		uts_Email_Send_Retry_02
  TestObjective  :	To validate parameter for mail_id in Retry mail send
  APIs Tested    :	int email_retry_sending_mail(int mail_id, int timeout_in_sec)
 */

static void uts_Email_Send_Retry_02()
{
	int err_code = EMAIL_ERROR_NONE;
	
	tet_infoline("uts_Email_Send_Retry_02\n");

	err_code = email_retry_sending_mail(-1, 2);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email Send Retry Success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email Send Retry  failed : \n");
		tet_result(TET_FAIL);
	}				
}



