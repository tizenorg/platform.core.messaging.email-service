/*
*  email-service
*
* Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Kyuho Jo <kyuho.jo@samsung.com>, Sunghyun Kwon <sh0701.kwon@samsung.com>
* 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

/* common header */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* open header */
#include <glib.h>

#include "email-api.h"
#include "email-api-account.h"
#include "email-api-network.h"

/* internal header */
#include "testapp-utility.h"
#include "testapp-account.h"
#include <sys/time.h>
#include <sys/times.h>

/* internal defines */

#define GWB_RECV_SERVER_ADDR  	    "pop.gawab.com"
#define GWB_SMTP_SERVER_ADDR  	    "smtp.gawab.com"

#define VDF_RECV_SERVER_ADDR  "imap.email.vodafone.de"
#define VDF_SMTP_SERVER_ADDR  "smtp.email.vodafone.de"

/*  SAMSUNG 3G TEST */
#define S3G_RECV_SERVER_ADDR		    "165.213.73.235"
#define S3G_RECV_SERVER_PORT		    EMAIL_POP3_PORT
#define S3G_RECV_USE_SECURITY		    0
#define S3G_RECV_IMAP_USE_SECURITY	1
#define S3G_SMTP_SERVER_ADDR		    "165.213.73.235"
#define S3G_SMTP_SERVER_PORT		    465
#define S3G_SMTP_AUTH				        1
#define S3G_SMTP_USE_SECURITY	      1
#define S3G_KEEP_ON_SERVER		      1

gboolean testapp_test_create_account_by_account_type(int account_type,int *account_id) 
{
	email_account_t *account = NULL;
	char id_string[100] = { 0, }, password_string[100] = { 0, }, address_string[100]  = { 0, };
	int err_code = EMAIL_ERROR_NONE, samsung3g_account_index;
	int result_from_scanf = 0;
	unsigned handle;

	switch(account_type) {
		case 4 : 
		case 5 :
			do {
				testapp_print("Enter your account index [1~10] : ");
				result_from_scanf = scanf("%d",&samsung3g_account_index);
			}while( samsung3g_account_index > 10 || samsung3g_account_index < 1);
			sprintf(id_string, "test%02d", samsung3g_account_index);
			sprintf(address_string, "<test%02d@streaming.s3glab.net>", samsung3g_account_index);
			strcpy(password_string, id_string);
			break;
		default:
			testapp_print("Enter email address : ");
			result_from_scanf = scanf("%s", address_string);

			testapp_print("Enter id : ");
			result_from_scanf = scanf("%s", id_string);

			testapp_print("Enter password_string : ");
			result_from_scanf = scanf("%s", password_string);
			break;
	}

	account = malloc(sizeof(email_account_t));
	memset(account, 0x00, sizeof(email_account_t));

	typedef struct {
		int is_preset_account;
		int index_color;
	} user_data_t;
	user_data_t data = (user_data_t) {1, 0};
	/* if user_data_t has any pointer member, please don't use sizeof(). */
	/* You need to serialize user_data to buffer and then take its length */
	int data_length = sizeof(data);

	/* Common Options */
	account->retrieval_mode                = EMAIL_IMAP4_RETRIEVAL_MODE_ALL;
	account->incoming_server_secure_connection	= 1;
	account->outgoing_server_type          = EMAIL_SERVER_TYPE_SMTP;
	account->auto_download_size			   = 2;
	account->outgoing_server_use_same_authenticator = 1;
	account->pop_before_smtp               = 0;
	account->incoming_server_requires_apop = 0;
	account->logo_icon_path                = NULL;
	account->user_data                     = malloc (data_length);
	memcpy( account->user_data, (void*) &data, data_length );
	account->user_data_length              = data_length;
	account->options.priority              = 3;
	account->options.keep_local_copy       = 0;
	account->options.req_delivery_receipt  = 0;
	account->options.req_read_receipt      = 0;
	account->options.download_limit        = 0;
	account->options.block_address         = 0;
	account->options.block_subject         = 0;
	account->options.display_name_from     = NULL;
	account->options.reply_with_body       = 0;
	account->options.forward_with_files    = 0;
	account->options.add_myname_card       = 0;
	account->options.add_signature         = 0;
	account->options.signature             = NULL;
	account->options.add_my_address_to_bcc = 0;
	account->check_interval                = 0;
	account->keep_mails_on_pop_server_after_download	= 1;
	account->default_mail_slot_size        = 200;

	account->account_name                  = strdup(address_string);
	account->user_display_name             = strdup(id_string);
	account->user_email_address            = strdup(address_string);
	account->reply_to_address              = strdup(address_string);
	account->return_address                = strdup(address_string);

	account->incoming_server_user_name     = strdup(id_string);
	account->incoming_server_password      = strdup(password_string);
	account->outgoing_server_user_name     = strdup(id_string);
	account->outgoing_server_password	   = strdup(password_string);

	switch (account_type) {
		case 1:/*  gawab */
			account->incoming_server_type  		 = EMAIL_SERVER_TYPE_POP3 ;
			account->incoming_server_address	 = strdup(GWB_RECV_SERVER_ADDR);
			account->incoming_server_port_number = EMAIL_POP3S_PORT;
			account->outgoing_server_address     = strdup(GWB_SMTP_SERVER_ADDR);
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_need_authentication = 1;
			account->outgoing_server_port_number = EMAIL_SMTPS_PORT;
			account->outgoing_server_secure_connection       = 1;

			break;

		case 2:/*  vadofone */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_IMAP4;
			account->incoming_server_address= strdup(VDF_RECV_SERVER_ADDR);
			account->incoming_server_port_number = EMAIL_IMAP_PORT;
			account->outgoing_server_address     = strdup(VDF_SMTP_SERVER_ADDR);
			account->incoming_server_secure_connection	= 0;
			account->outgoing_server_need_authentication = 0;
			break;

		case 4:/*  SAMSUNG 3G TEST */
			account->incoming_server_type  		 = EMAIL_SERVER_TYPE_POP3;
			account->incoming_server_address	 = strdup(S3G_RECV_SERVER_ADDR);
			account->incoming_server_port_number = S3G_RECV_SERVER_PORT;
			account->outgoing_server_address     = strdup(S3G_SMTP_SERVER_ADDR);
			account->outgoing_server_port_number = S3G_SMTP_SERVER_PORT;
 			account->incoming_server_secure_connection	= S3G_RECV_USE_SECURITY;
			account->outgoing_server_secure_connection  = S3G_SMTP_USE_SECURITY;
			account->outgoing_server_need_authentication = S3G_SMTP_AUTH;
			break;

		case 5:/*  SAMSUNG 3G TEST */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_IMAP4;
			account->incoming_server_address= strdup(S3G_RECV_SERVER_ADDR);
			account->incoming_server_port_number = EMAIL_IMAPS_PORT;
			account->outgoing_server_address     = strdup(S3G_SMTP_SERVER_ADDR);
			account->outgoing_server_port_number = S3G_SMTP_SERVER_PORT;
 			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_secure_connection  = S3G_SMTP_USE_SECURITY;
			account->outgoing_server_need_authentication = S3G_SMTP_AUTH;
			break;

		case 6:/*  Gmail POP3 */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_POP3;
			account->incoming_server_address= strdup("pop.gmail.com");
			account->incoming_server_port_number = 995;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address    = strdup("smtp.gmail.com");
			account->outgoing_server_port_number = 465;
			account->outgoing_server_secure_connection = 1;
			account->outgoing_server_need_authentication = 1;
			break;

		case 7 : /*  Gmail IMAP4 */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_IMAP4;
			account->incoming_server_address= strdup("imap.gmail.com");
			account->incoming_server_port_number = 993;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address    = strdup("smtp.gmail.com");
			account->outgoing_server_port_number = 465;
			account->outgoing_server_secure_connection = 1;
			account->outgoing_server_need_authentication = 1;
			break;

		case 8: /*  Active Sync */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_ACTIVE_SYNC;
			account->incoming_server_address= strdup("");
			account->incoming_server_port_number = 0;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address    = strdup("");
			account->outgoing_server_port_number = 0;
			account->outgoing_server_secure_connection = 1;
			account->outgoing_server_need_authentication = 1;
			break;

		case 9: /*  AOL */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_IMAP4;
			account->incoming_server_address= strdup("imap.aol.com");
			account->incoming_server_port_number = 143;
			account->incoming_server_secure_connection	= 0;
			account->outgoing_server_address    = strdup("smtp.aol.com");
			account->outgoing_server_port_number = 587;
			account->outgoing_server_secure_connection = 0;
			account->outgoing_server_need_authentication = 1;
			break;

		case 10: /*  Hotmail */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_POP3;
			account->incoming_server_address= strdup("pop3.live.com");
			account->incoming_server_port_number = 995;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address    = strdup("smtp.live.com");
			account->outgoing_server_port_number = 587;
			account->outgoing_server_secure_connection  = 0x02;
			account->outgoing_server_need_authentication = 1;
			break;

		case 11:/*  Daum IMAP4*/
			account->incoming_server_type  = EMAIL_SERVER_TYPE_IMAP4;
			account->incoming_server_address= strdup("imap.daum.net");
			account->incoming_server_port_number = 993;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address    = strdup("smtp.daum.net");
			account->outgoing_server_port_number = 465;
            account->outgoing_server_secure_connection = 1;
			account->outgoing_server_need_authentication = 1;
			break;

		case 12:/*  Daum POP3*/
			account->incoming_server_type  = EMAIL_SERVER_TYPE_POP3;
			account->incoming_server_address= strdup("pop.daum.net");
			account->incoming_server_port_number = 995;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address    = strdup("smtp.daum.net");
			account->outgoing_server_port_number = 465;
            account->outgoing_server_secure_connection = 1;
			account->outgoing_server_need_authentication = 1;
			break;

		default:
			testapp_print("Invalid Account Number\n");
			return FALSE;
			break;
	}
	account->account_svc_id = 77;
	err_code = email_add_account_with_validation(account, &handle);
	if( err_code < 0) {
		testapp_print ("   email_add_account_with_validation error : %d\n",err_code);
		return FALSE;
	}

	testapp_print ("   email_add_account succeed\n");

	if(account_id)
		*account_id = account->account_id;

	err_code = email_free_account(&account, 1);
	return TRUE;

}

static gboolean testapp_test_create_account() 
{
	int account_type = 0 ;
	int err = EMAIL_ERROR_NONE;
	int result_from_scanf = 0;
	
	testapp_print("1. Gawab\n");
	testapp_print("2. Vodafone\n");
	testapp_print("4. SAMSUNG 3G TEST (POP)\n");
	testapp_print("5. SAMSUNG 3G TEST (IMAP)\n");
	testapp_print("6. Gmail (POP3)\n");
	testapp_print("7. Gmail (IMAP4)\n");
	testapp_print("8. Active Sync (dummy)\n");
	testapp_print("9. AOL\n");
	testapp_print("10. Hotmail\n");
	testapp_print("11. Daum (IMAP4)\n");
	testapp_print("12. Daum (POP3)\n");
	testapp_print("Choose server type: ");
	
	result_from_scanf = scanf("%d",&account_type);

	if(!testapp_test_create_account_by_account_type(account_type,&err)) {
		testapp_print ("   testapp_test_create_account_by_account_type error\n");
		return FALSE;
	}
	return FALSE;
}

static gboolean testapp_test_update_account()
{
	int result_from_scanf = 0;
	int account_id;
	email_account_t *account = NULL;
	char account_name[256];
	int err = EMAIL_ERROR_NONE;
	char signature[100] = {0};
	char user_email_address[256] = {0,};
	int add_my_address_to_bcc = 0;
	int account_svc_id = 0, with_validation = 0;

	
	testapp_print("\n>> Enter Account No: ");
	result_from_scanf = scanf("%d",&account_id);

/* sowmya.kr, 281209 Adding signature to options in email_account_t changes */
	if( (err = email_get_account(account_id, GET_FULL_DATA,&account)) != EMAIL_ERROR_NONE) {
		testapp_print ("email_get_account failed - %d\n", err);
		return false;
	}

	testapp_print ("email_get_account result account_name - %s \n", account->account_name);

	testapp_print ("email_get_account result signature - %s \n", account->options.signature);

#ifdef __FEATURE_AUTO_POLLING__
	testapp_print ("email_get_account result check_interval - %d \n", account->check_interval);
#endif

	testapp_print("\n Enter new Account name:");
	result_from_scanf = scanf("%s",account_name);

	
	testapp_print("\n Enter new email addr:");
	result_from_scanf = scanf("%s",user_email_address);
#ifdef __FEATURE_AUTO_POLLING__
	testapp_print("\n Enter new check interval (in mins):");
	result_from_scanf = scanf("%d",&(account->check_interval));
#endif
	testapp_print("\n Enter new signature:");
	result_from_scanf = scanf("%s",signature);

	testapp_print("\n>> Enter add_my_address_to_bcc:(0:off, 1:on) ");
	result_from_scanf = scanf("%d",&add_my_address_to_bcc);

	testapp_print("\n>> Enter account_svc_id: ");
	result_from_scanf = scanf("%d",&account_svc_id);

	testapp_print("\n>> With validation ? (0: No, 1:Yes) ");
	result_from_scanf = scanf("%d",&with_validation);

    if( account )  {
		account->account_name = strdup(account_name);
		testapp_print("\n Assigning New Account name: (%s)", account->account_name);
		account->user_email_address = strdup(user_email_address);
		account->options.signature = strdup(signature);
		testapp_print("\n Assigning New Signature: (%s)\n", account->options.signature);
		account->options.add_my_address_to_bcc = add_my_address_to_bcc;
		account->account_svc_id = account_svc_id;

		if(with_validation) {
			if((err = email_update_account_with_validation(account_id, account)) != EMAIL_ERROR_NONE){
				testapp_print ("email_update_account_with_validation failed - %d\n", err);
				return false;
			}
				testapp_print ("email_update_account_with_validation successful \n");
		}
		else {
			if((err = email_update_account(account_id, account)) != EMAIL_ERROR_NONE) {
				testapp_print ("email_update_account failed - %d\n", err);
				return false;
			}
			testapp_print ("email_update_account successful \n");
		}
    }
	return true;
}

static gboolean testapp_test_delete_account ()
{
	int account_id;
	email_account_t *account=NULL;
	int err = EMAIL_ERROR_NONE;
	int result_from_scanf = 0;

	testapp_print("\n>> Enter Account No: ");
	result_from_scanf = scanf("%d",&account_id);

/* sowmya.kr, 281209 Adding signature to options in email_account_t changes */
	if( (err = email_get_account(account_id, WITHOUT_OPTION,&account)) < 0) {
		testapp_print ("email_get_account failed \n");
		testapp_print("testapp_test_delete_account failed\n");
	}
	else {
		testapp_print ("email_get_account result account_name - %s \n", account->account_name);

		if((err = email_delete_account(account_id)) < 0) 
			testapp_print ("email_delete_account failed[%d]\n", err);
		else
			testapp_print ("email_delete_account successful \n");
	}
	return FALSE;

}


static gboolean testapp_test_validate_account ()
{
	int result_from_scanf = 0;
	int account_id;
	email_account_t *account=NULL;
	int err_code = EMAIL_ERROR_NONE;
	unsigned account_handle = 0;
	
	testapp_print("\n>> Enter Account No: ");
	result_from_scanf = scanf("%d",&account_id);

/* sowmya.kr, 281209 Adding signature to options in email_account_t changes */
	if( (err_code = email_get_account(account_id, WITHOUT_OPTION,&account)) < 0 ) {
		testapp_print ("email_get_account failed \n");
		return FALSE;
	}
	else
		testapp_print ("email_get_account result account_name - %s \n", account->account_name);

	if((err_code = email_validate_account(account_id, &account_handle)) == EMAIL_ERROR_NONE ) 
		testapp_print ("email_validate_account successful  account_handle : %u\n",account_handle);
	else
		testapp_print ("email_validate_account failed err_code: %d \n",err_code);
		
	return FALSE;

}


static gboolean testapp_test_cancel_validate_account ()
{
	int result_from_scanf = 0;
	int account_id = 0;
	int err_code = EMAIL_ERROR_NONE;
	unsigned account_handle = 0;

	testapp_print("\n > Enter account_id: ");
	result_from_scanf = scanf("%d", &account_id);

	testapp_print("\n > Enter handle: ");
	result_from_scanf = scanf("%d", &account_handle);

	err_code = email_cancel_job(account_id, account_handle, EMAIL_CANCELED_BY_USER);
	if(err_code == 0)
		testapp_print("email_cancel_job Success..!handle:[%d]", account_handle);
	else
		testapp_print ("email_cancel_job failed err_code: %d \n",err_code);
	
	return FALSE;
}

static gboolean testapp_test_get_account()
{
	int result_from_scanf = 0;
	int account_id;
	email_account_t *account=NULL;
	int err_code = EMAIL_ERROR_NONE;
	testapp_print("\n>> Enter Account No: ");
	result_from_scanf = scanf("%d",&account_id);

	typedef struct {
		int is_preset_account;
		int index_color;
	} user_data_t;

	testapp_print ("\n----------------------------------------------------------\n");
	testapp_print ("email_get_account GET_FULL_DATA \n");
	if( (err_code = email_get_account(account_id,GET_FULL_DATA,&account)) < 0) {
		testapp_print ("email_get_account failed - %d\n", err_code);
		return FALSE;
	}

	user_data_t* val = (user_data_t*) account->user_data;
	int is_preset_account =  val? val->is_preset_account : 0;
	int index_color = val? val->index_color : 0;

	testapp_print ("email_get_account result\n"
			"account_name - %s \n"
			"user_email_address - %s \n"
			"incoming_server_secure_connection %d \n"
			"add_sig : %d \n"
			"signature %s \n"
			"add_my_address_to_bcc %d \n"
			"account_svc_id %d\n"
			"incoming_server_address %s\n"
			"outgoing_server_address %s\n"
			"default_mail_slot_size %d\n"
			"sync_status %d\n"
			"sync_disabled %d\n"
			"is_preset %d\n"
			"index_color %d\n"
			"certificate_path %s\n"
			"digest_type %d\n"
		,
		account->account_name,
		account->user_email_address,
		account->incoming_server_secure_connection,
		account->options.add_signature,
		account->options.signature,
		account->options.add_my_address_to_bcc,
		account->account_svc_id,
		account->incoming_server_address,
		account->outgoing_server_address,
		account->default_mail_slot_size,
		account->sync_status,
		account->sync_disabled,
		is_preset_account,
		index_color,
		account->certificate_path,
		account->digest_type
		);

	err_code = email_free_account(&account, 1);

	testapp_print ("\n----------------------------------------------------------\n");
	testapp_print ("email_get_account WITHOUT_OPTION \n");

	if( (err_code = email_get_account(account_id, WITHOUT_OPTION, &account)) < 0) {
		testapp_print ("email_get_account failed \n");
		return FALSE;
	}

	testapp_print ("email_get_account result\n"
			"account_name - %s \n"
			"user_email_address - %s \n"
			"incoming_server_secure_connection %d \n"
			"add_signature : %d \n",
		account->account_name,
		account->user_email_address,
		account->incoming_server_secure_connection,
		account->options.add_signature
	);

	if(account->options.signature)
		testapp_print ("signature : %s\n", account->options.signature);
	else
		testapp_print ("signature not retrieved \n");

	err_code = email_free_account(&account, 1);

	testapp_print ("\n----------------------------------------------------------\n");
	testapp_print ("email_get_account ONLY_OPTION \n");

	if( (err_code = email_get_account(account_id, ONLY_OPTION, &account)) < 0) {
		testapp_print ("email_get_account failed \n");
		return FALSE;
	}

	testapp_print ("email_get_account result\n"
			"add_sig : %d \n"
			"signature %s \n"
			"add_my_address_to_bcc %d\n"
			"account_svc_id %d\n",
		account->options.add_signature,
		account->options.signature,
		account->options.add_my_address_to_bcc,
		account->account_svc_id
		);

	if(account->account_name)
		testapp_print ("account_name : %s \n", account->account_name);
	else
		testapp_print ("account_name not retrieved \n");	

	if(account->user_email_address)
		testapp_print ("user_email_address : %s \n", account->user_email_address);
	else
		testapp_print ("user_email_address not retrieved \n");	
	err_code = email_free_account(&account, 1);
		
	return FALSE;
}

static gboolean testapp_test_get_account_list ()
{

	int count, i;
	email_account_t *account_list=NULL;
	struct timeval tv_1, tv_2;
	int interval;
	int err_code = EMAIL_ERROR_NONE;

	gettimeofday(&tv_1, NULL);

	if((err_code = email_get_account_list(&account_list, &count)) < 0 ) {
		testapp_print("   email_get_account_list error\n");
		return false ;
	}

	gettimeofday(&tv_2, NULL);
	interval = tv_2.tv_usec - tv_1.tv_usec;
	testapp_print("\t testapp_test_get_account_list Proceed time %d us\n",interval);
	
	for(i=0;i<count;i++){
		testapp_print("   %2d) %-15s %-30s\n",account_list[i].account_id, 
			account_list[i].account_name, 
			account_list[i].user_email_address);
	}

	err_code = email_free_account(&account_list, count);
	return FALSE;
}

static gboolean testapp_test_backup_account()
{
	char *file_name = "accounts_file";
	int error_code;
	error_code = email_backup_accounts_into_secure_storage(file_name);
	testapp_print("\n email_backup_accounts_into_secure_storage returned [%d]\n",error_code);
	return FALSE;
}
static gboolean testapp_test_restore_account()
{
	char *file_name = "accounts_file";
	int error_code;
	error_code = email_restore_accounts_from_secure_storage(file_name);
	testapp_print("\n email_restore_accounts_from_secure_storage returned [%d]\n",error_code);
	return FALSE;
}

static gboolean testapp_test_get_password_length_of_account()
{
	int result_from_scanf = 0;
	int error_code, password_length, account_id;

	testapp_print("\n input account id\n");
	result_from_scanf = scanf("%d", &account_id);
	error_code = email_get_password_length_of_account(account_id, &password_length);
	testapp_print("testapp_test_get_password_length_of_account returned [%d]\n",password_length);
	return FALSE;
}

static gboolean testapp_test_query_server_info()
{
	int result_from_scanf = 0;
	int error_code;
	char domain_name[255];
	email_server_info_t *result_server_info;

	testapp_print("\n input domain name\n");
	result_from_scanf = scanf("%s", domain_name);

	error_code = email_query_server_info(domain_name, &result_server_info);
	testapp_print("email_query_server_info returned [%d]\n",error_code);
	if(error_code == EMAIL_ERROR_NONE)
		testapp_print("service_name [%s]\n", result_server_info->service_name);
	return FALSE;
}

static gboolean testapp_test_clear_all_notification()
{
	int error_code;

	error_code = email_clear_all_notification_bar();
	testapp_print("email_clear_all_notification_bar returned [%d]\n",error_code);
	return FALSE;
}

static gboolean testapp_test_save_default_account_id()
{
	int result_from_scanf = 0;
	int error_code;
	int account_id = 0;

	testapp_print ("\nInput default account id : ");

	result_from_scanf = scanf("%d", &account_id);

	error_code = email_save_default_account_id(account_id);

	testapp_print("email_save_default_account_id returned [%d]\n",error_code);
	return FALSE;
}

static gboolean testapp_test_load_default_account_id()
{
	int error_code;
	int account_id = 0;

	error_code = email_load_default_account_id(&account_id);

	testapp_print ("\ndefault account id : %d\n", account_id);
	testapp_print("email_load_default_account_id returned [%d]\n",error_code);
	return FALSE;
}

static gboolean testapp_test_add_certificate()
{
	int result_from_scanf = 0;
	int ret = 0;
	char save_name[50] = {0, };
	char certificate_path[255] = {0, };

	testapp_print("Input cert path : ");
	result_from_scanf = scanf("%s", certificate_path);	

	testapp_print("Input cert email-address : ");
	result_from_scanf = scanf("%s", save_name);

	testapp_print("cert path : [%s]", certificate_path);
	testapp_print("email-address : [%s]", save_name);

	ret = email_add_certificate(certificate_path, save_name);
	if (ret != EMAIL_ERROR_NONE) {
		testapp_print("Add certificate failed\n");
		return false;
	}

	testapp_print("Add certificate success\n");
	return true;
}

static gboolean testapp_test_get_certificate()
{
	int result_from_scanf = 0;
	int ret = 0;
	char save_name[20] = {0, };
	email_certificate_t *certificate = NULL;

	testapp_print("Input cert email-address : ");
	result_from_scanf = scanf("%s", save_name);

	ret = email_get_certificate(save_name, &certificate);
	if (ret != EMAIL_ERROR_NONE) {
		testapp_print("Get certificate failed\n");
		return false;
	}

	testapp_print("certificate_id : %d\n", certificate->certificate_id);
	testapp_print("issue_year : %d\n", certificate->issue_year);
	testapp_print("issue_month : %d\n", certificate->issue_month);
	testapp_print("issue_day : %d\n", certificate->issue_day);
	testapp_print("expiration_year : %d\n", certificate->expiration_year);
	testapp_print("expiration_month : %d\n", certificate->expiration_month);
	testapp_print("expiration_day : %d\n", certificate->expiration_day);
	testapp_print("issue_organization_name : %s\n", certificate->issue_organization_name);
	testapp_print("subject_string : %s\n", certificate->subject_str);
	testapp_print("file path : %s\n", certificate->filepath);

	if (certificate)
		email_free_certificate(&certificate, 1);

	testapp_print("Get certificate success\n");
	return true;
}

static gboolean testapp_test_delete_certificate()
{
	int result_from_scanf = 0;
	int ret = 0;
	char save_name[20] = {0, };

	testapp_print("Input cert email-address : ");
	result_from_scanf = scanf("%s", save_name);

	ret = email_delete_certificate(save_name);
	if (ret != EMAIL_ERROR_NONE) {
		testapp_print("Delete certificate failed\n");
		return false;
	}

	testapp_print("Delete certificate success\n");
	return true;
}
static gboolean testapp_test_interpret_command (int selected_number)
{
	gboolean go_to_loop = TRUE;
	
	switch (selected_number) {
		case 1:
			testapp_test_create_account();
			break;

		case 2:
			testapp_test_update_account();
			break;

		case 3:
			testapp_test_delete_account();
			break;

		case 4:
			testapp_test_get_account();
			break;
		
		case 5:
			testapp_test_get_account_list();
			break;

		case 7:
			testapp_test_validate_account();
			break;		

		case 8:
			testapp_test_cancel_validate_account();
			break;	

		case 9:
			testapp_test_backup_account();
			break;	

		case 10:
			testapp_test_restore_account();
			break;	

		case 11:
			testapp_test_get_password_length_of_account();
			break;

		case 12:
			testapp_test_query_server_info();
			break;

		case 13:
			testapp_test_clear_all_notification();
			break;

		case 14:
			testapp_test_save_default_account_id();
			break;

		case 15:
			testapp_test_load_default_account_id();
			break;

		case 16:
			testapp_test_add_certificate();
			break;

		case 17:
			testapp_test_get_certificate();
			break;

		case 18:
			testapp_test_delete_certificate();
			break;

		case 0:
			go_to_loop = FALSE;
			break;

		default:
			break;
	}

	return go_to_loop;
}

void testapp_account_main ()
{
	gboolean go_to_loop = TRUE;
	int menu_number = 0;
	int result_from_scanf = 0;
	
	while (go_to_loop) {
		testapp_show_menu (EMAIL_ACCOUNT_MENU);
		testapp_show_prompt (EMAIL_ACCOUNT_MENU);
			
		result_from_scanf = scanf ("%d", &menu_number);

		go_to_loop = testapp_test_interpret_command (menu_number);
	}	
}

