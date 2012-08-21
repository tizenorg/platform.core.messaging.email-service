/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-add-mailbox.h"
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






/*Testcase   :		uts_Email_Add_Mailbox_01
  TestObjective  :	To add  a new mailbox to an account only localy
  APIs Tested    :	int email_add_mailbox(email_mailbox_t *new_mailbox, int on_server, unsigned *handle) 
 */

static void uts_Email_Add_Mailbox_01()
{
	int err_code = EMAIL_ERROR_NONE ;
	email_mailbox_t mbox;
	int local_yn = 1;
	int handle;
	
	tet_infoline("uts_Email_Add_Mailbox_01 Begin\n");

	memset(&mbox , 0x00, sizeof(email_mailbox_t));
	mbox.mailbox_name = strdup("Test");
	mbox.alias = strdup("Test");
	mbox.local = local_yn;
	mbox.mailbox_type = 7;
	mbox.account_id = g_accountId;
	err_code = email_add_mailbox(&mbox , 0, &handle);
	if (EMAIL_ERROR_NONE == err_code) {	
		tet_infoline("Email add mailbox Success\n");	
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email add mailbox failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}
}


/*Testcase   :		uts_Email_Add_Mailbox_02
  TestObjective  :	To add  a new mailbox to an account on server as well as in local also
  APIs Tested    :	int email_add_mailbox(email_mailbox_t *new_mailbox, int on_server, unsigned *handle) 
 */

static void uts_Email_Add_Mailbox_02()
{
	int err_code = EMAIL_ERROR_NONE ;
	email_mailbox_t mbox;
	int local_yn = 0;
	int handle;
	
	tet_infoline("uts_Email_Add_Mailbox_02 Begin\n");

	memset(&mbox , 0x00, sizeof(email_mailbox_t));
	mbox.mailbox_name = strdup("Test");
	mbox.alias = strdup("Test");
	mbox.local = local_yn;
	mbox.mailbox_type = 7;
			
	mbox.account_id = g_accountId;
	err_code = email_add_mailbox(&mbox , 1, &handle);
	if (EMAIL_ERROR_NONE == err_code) {	
		tet_infoline("Email add mailbox on server and in local  :  Success\n");	
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email add mailbox failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}
}


/*Testcase   :		uts_Email_Add_Mailbox_03
  TestObjective  :	To validate parameter for add  a new mailbox 
  APIs Tested    :	int email_add_mailbox(email_mailbox_t *new_mailbox, int on_server, unsigned *handle) 
 */

static void uts_Email_Add_Mailbox_03()
{
	int err_code = EMAIL_ERROR_NONE ;
	email_mailbox_t *mbox = NULL;
	int local_yn = 0;
	int handle;
	int count = 0;

	tet_infoline("uts_Email_Add_Mailbox_03 Begin\n");

	err_code = email_add_mailbox(mbox , 1, &handle);
	if (EMAIL_ERROR_NONE != err_code) {	
		tet_infoline("Email add mailbox validate parameter :  Success\n");	
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email add mailbox failed validate parameter\n");
		tet_result(TET_FAIL);
	}
	
}


