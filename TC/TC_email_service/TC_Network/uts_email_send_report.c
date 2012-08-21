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
#include "uts_email_send_report.h"
#include "../TC_Utility/uts-email-real-utility.c"


sqlite3 *sqlite_emmb; 
static int g_accountId;


static void startup()
{
	int err_code = EMF_ERROR_NONE;
	int count = 0;
	int i = 0;
	emf_account_t *pAccount = NULL ;

	tet_printf("\n TC startup");

	if (EMF_ERROR_NONE == email_service_begin()) {
		tet_infoline("Email service Begin\n");
		if (EMF_ERROR_NONE == email_open_db()) {
			tet_infoline("Email open DB success\n");
			err_code = email_get_account_list(&pAccount, &count);
			if (!count) {   
				g_accountId = uts_Email_Add_Real_Account_01();
				uts_Email_Add_Real_Message_02();
			}   
			else {
				g_accountId = pAccount[i].account_id;
				uts_Email_Add_Real_Message_02();
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

	if (EMF_ERROR_NONE == email_close_db()) {
		tet_infoline("Email Close DB Success\n");
		if (EMF_ERROR_NONE == email_service_end())
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


/*Testcase   :		uts_Email_Send_Report_01
  TestObjective  :	To send a read receipt mail
  APIs Tested    :	int email_send_report(emf_mail_t *mail,  unsigned *handle)
  */

static void uts_Email_Send_Report_01()
{
	int err_code = EMF_ERROR_NONE;
	int handle;
	int count = 0;
	int i = 0;
	emf_mailbox_t mbox;
	emf_account_t *account = NULL ;
	emf_mail_list_item_t *mail_list = NULL;
	emf_mail_t *pMail;
	emf_mail_t *mail;

	tet_infoline("uts_Email_Send_Report_01 Begin\n");

	memset(&mbox, 0x00, sizeof(emf_mailbox_t));
	mbox.name = strdup("INBOX");

	mbox.account_id = g_accountId;
	count = 0;
	err_code = email_get_mail_list(mbox.account_id, mbox.name, &mail_list, &count, 0);
	if (EMF_ERROR_NONE == err_code) {
		tet_infoline("email_get_mail_list  :  success\n");
		err_code = email_get_mail(&mbox , mail_list[i].mail_id, &pMail);	
		if (EMF_ERROR_NONE == err_code) {
			tet_infoline("email_get_mail success\n");

			mail = (emf_mail_t  *)malloc(sizeof(emf_mail_t));	
			memset(mail , 0x00, sizeof(emf_mail_t));
			mail->info = (emf_mail_info_t  *)malloc(sizeof(emf_mail_info_t));	
			memset(mail->info , 0x00, sizeof(emf_mail_info_t));
			mail->head = (emf_mail_head_t  *)malloc(sizeof(emf_mail_head_t));	
			memset(mail->head , 0x00, sizeof(emf_mail_head_t));
			mail->body = (emf_mail_body_t  *)malloc(sizeof(emf_mail_body_t));	
			memset(mail->body , 0x00, sizeof(emf_mail_body_t));
			tet_printf("account_id[%d]", pMail->info->account_id);
			mail->info->account_id = pMail->info->account_id;
			tet_printf("from[%s]", pMail->head->from);
			mail->head->from = strdup(pMail->head->from);	
			mail->head->mid = strdup("Mid");

			tet_printf("mid[%d]", mail->head->mid);

			err_code = email_send_report(mail , &handle);
			if (EMF_ERROR_NONE == err_code) {
				tet_infoline("email_send_report success \n");					
				tet_result(TET_PASS);
			}
			else {
				tet_printf("email_send_report failed  :  error_code[%d]\n", err_code);
				tet_result(TET_FAIL);
			}
			email_free_mail(&mail, 1);
		}
		else {
			tet_printf("email_get_mail failed  :  error_code[%d]\n", err_code);
			tet_result(TET_UNRESOLVED);
		}

		email_free_mail(&pMail, 1);
	}
	else {
		tet_printf("email_get_mail failed  :  error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}

/* 	email_free_mail_list(&mail_list, count) */
	free(mail_list);
}


/*Testcase   :		uts_Email_Send_Report_02
  TestObjective  :	To validate send a read receipt mail
  APIs Tested    :	int email_send_report(emf_mail_t *mail,  unsigned *handle)
  */
static void uts_Email_Send_Report_02()
{
	int err_code = EMF_ERROR_NONE;
	int handle;
	emf_mail_t *mail = NULL;/*Validation Field*/

	tet_infoline("uts_Email_Send_Report_02 Begin\n");

	err_code = email_send_report(mail , &handle);
	if (EMF_ERROR_NONE != err_code) {
		tet_infoline("email_send_report  :  success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_infoline("email_send_report  :  failed\n");
		tet_result(TET_FAIL);	
	}

}

