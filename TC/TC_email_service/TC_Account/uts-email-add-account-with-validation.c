/*
* Copyright (c) 2010  Samsung Electronics, Inc.
* All rights reserved.
*
* This software is a confidential and proprietary information of Samsung
* Electronics, Inc. ("Confidential Information").  You shall not disclose such
* Confidential Information and shall use it only in accordance with the terms
* of the license agreement you entered into with Samsung Electronics.
*/

#include "uts-email-add-account-with-validation.h"

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

/*Testcase   : 		uts_Email_Add_Account_With_Validation_01
  TestObjective  : 	To  add and validate an email account
  APIs Tested    : 	int email_add_account_with_validation(email_account_t *account, unsigned *handle)
 */

static void uts_Email_Add_Account_With_Validation_01()
{
	email_account_t *account = NULL;
	int handle;
	int err_code = EMAIL_ERROR_NONE;

	tet_infoline("uts_Email_Add_Account_With_Validation_01 Begin\n");
	account = (email_account_t  *)malloc(sizeof(email_account_t));
	if (account) {
		memset(account, 0x00, sizeof(email_account_t));

		account->account_name		= strdup("GmailImap");
		account->user_display_name	= strdup("samsungtest");
		account->user_email_address	= strdup("samsungtest09@gmail.com");
		account->reply_to_address	= strdup("samsungtest09@gmail.com");
		account->return_address		= strdup("samsungtest09@gmail.com");
		account->incoming_server_type   = EMAIL_SERVER_TYPE_IMAP4;
		account->incoming_server_address= strdup("imap.gmail.com");
		account->incoming_server_port_number = 993;
		account->incoming_server_secure_connection	= 1;
		account->retrieval_mode	= EMAIL_IMAP4_RETRIEVAL_MODE_NEW;
		account->incoming_server_user_name = strdup("Samsung Test 09");
		account->incoming_server_password = strdup("samsung09");
		account->outgoing_server_type = EMAIL_SERVER_TYPE_SMTP;
		account->outgoing_server_address    = strdup("smtp.gmail.com");
		account->outgoing_server_port_number = 465;
		account->outgoing_server_secure_connection	= 1;
		account->outgoing_server_need_authentication = 1;
		account->outgoing_server_user_name	= strdup("samsungtest09");
		account->outgoing_server_password	= strdup("samsung09");
		account->pop_before_smtp	= 0;
		account->incoming_server_requires_apop = 0;
		account->auto_download_size	= 0;			/*  downloading option, 0 is subject only, 1 is text body, 2 is normal */
		account->outgoing_server_use_same_authenticator = 1;			/*  Specifies the 'Same as POP3' option, 0 is none, 1 is 'Same as POP3 */
		account->logo_icon_path	= NULL;
		account->options.priority = 3;
		account->options.keep_local_copy = 0;
		account->options.req_delivery_receipt = 0;
		account->options.req_read_receipt = 0;
		account->options.download_limit = 0;
		account->options.block_address = 0;
		account->options.block_subject = 0;
		account->options.display_name_from = NULL;
		account->options.reply_with_body = 0;
		account->options.forward_with_files = 0;
		account->options.add_myname_card = 0;
		account->options.add_signature = 0;
		account->options.signature = strdup("Gmail POP3 Signature");
		account->check_interval = 0;

		err_code = email_add_account_with_validation(account, &handle);
		if (EMAIL_ERROR_NONE == err_code) {
			tet_printf("Email add account_with_validation success : accountID[%d]\n", account->account_id);
			tet_result(TET_PASS);
		}
		else {
			tet_printf("Email add account_with_validation failed : error_code[%d]\n", err_code);
			tet_result(TET_FAIL);
		}
		email_free_account(&account, 1);
	}
}

/*Testcase   : 		uts_Email_Add_Account_With_Validation_02
  TestObjective  : 	To validate parameter for account in add an email account with validation
  APIs Tested    : 	int email_add_account_with_validation(email_account_t *account, unsigned *handle)
 */

static void uts_Email_Add_Account_With_Validation_02()
{
	int err_code = EMAIL_ERROR_NONE;
	email_account_t *pAccount = NULL ;
	int handle;

	tet_infoline("uts_Email_Add_Account_With_Validation_02 : Begin\n");

	err_code = email_add_account_with_validation(pAccount, &handle);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_printf("Email add account with validation success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email add account with validation failed\n");
		tet_result(TET_FAIL);
	}
}



