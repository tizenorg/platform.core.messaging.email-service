/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-release-mail.h"
#include "../TC_Utility/uts-email-dummy-utility.c"


sqlite3 *sqlite_emmb;


static void startup()
{
	tet_printf("\n TC startup");
	
	if (EMF_ERROR_NONE == email_service_begin()) {
		tet_infoline("Email service Begin\n");
		if (EMF_ERROR_NONE == email_open_db())
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

	if (EMF_ERROR_NONE == email_close_db())	{
		tet_infoline("Email Close DB Success\n");
		if (EMF_ERROR_NONE == email_service_end())
			tet_infoline("Email service close Success\n");
		else
			tet_infoline("Email service end failed\n");
	}
	else
		tet_infoline("Email Close DB failed\n");
}






/*Testcase   :		uts_Email_Release_Mail_01
  TestObjective  :	To Free the MailList Information
  APIs Tested    :	int email_release_mail(emf_mail_list_item_t **mail_list, int count)
  */

static void uts_Email_Release_Mail_01()
{
	int err_code = EMF_ERROR_NONE;
	int count = 0;
	emf_mail_list_item_t *mail_list = NULL;
	
	tet_infoline("uts_Email_Release_Mail_01\n");

	err_code = email_get_mail_list(0, NULL, &mail_list, &count, 0);
	if (EMF_ERROR_NONE == err_code) 	{	
				free(mail_list); 
		if (EMF_ERROR_NONE == err_code) {
			tet_infoline("Email Release Mail Success\n");
				tet_result(TET_PASS);
			}
		else {
			tet_printf("Email Release mail failed : error_code[%d]\n", err_code);
			tet_result(TET_FAIL);
			}  
	}
	else {
		tet_printf("Email get mail list failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);	
	}	
}


/*Testcase   :		uts_Email_Release_Mail_02
  TestObjective  :	To validate parameter for mail_list in  Free the MailList Information
  APIs Tested    :	int email_release_mail(emf_mail_list_item_t **mail_list, int count)
  */

static void uts_Email_Release_Mail_02()
{
	int err_code = EMF_ERROR_NONE;
	int count = 1;
	
	tet_infoline("uts_Email_Release_Mail_02\n");

	err_code = email_release_mail(NULL, count);
	if (EMF_ERROR_NONE != err_code)	{
		tet_infoline("Email Release mail Success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email Release Mail  failed : \n");
		tet_result(TET_FAIL);
	}				
}



