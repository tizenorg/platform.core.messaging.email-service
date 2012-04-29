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
#define S3G_RECV_SERVER_PORT		    EMF_POP3_PORT
#define S3G_RECV_USE_SECURITY		    0
#define S3G_RECV_IMAP_USE_SECURITY	1
#define S3G_SMTP_SERVER_ADDR		    "165.213.73.235"
#define S3G_SMTP_SERVER_PORT		    465
#define S3G_SMTP_AUTH				        1
#define S3G_SMTP_USE_SECURITY	      1
#define S3G_KEEP_ON_SERVER		      1

gboolean testapp_test_create_account_by_account_type(int account_type,int *account_id) 
{
	emf_account_t *account = NULL;
	char id_string[100] = { 0, }, password_string[100] = { 0, }, address_string[100]  = { 0, };
	int err_code = EMF_ERROR_NONE, samsung3g_account_index;
	unsigned handle;

	switch(account_type) {
		case 4 : 
		case 5 :
			do {
				testapp_print("Enter your account index [1~10] : ");
				scanf("%d",&samsung3g_account_index);
			}while( samsung3g_account_index > 10 || samsung3g_account_index < 1);
			sprintf(id_string, "test%02d", samsung3g_account_index);
			sprintf(address_string, "<test%02d@streaming.s3glab.net>", samsung3g_account_index);
			strcpy(password_string, id_string);
			break;
		default:
			testapp_print("Enter email address : ");
			scanf("%s", address_string);

			testapp_print("Enter id : ");
			scanf("%s", id_string);

			testapp_print("Enter password_string : ");
			scanf("%s", password_string);
			break;
	}

	account = malloc(sizeof(emf_account_t));
	memset(account, 0x00, sizeof(emf_account_t));

	/* Common Options */
	account->account_bind_type             = EMF_BIND_TYPE_EM_CORE;                           
	account->retrieval_mode                = EMF_IMAP4_RETRIEVAL_MODE_ALL;                                                   
	account->use_security                  = 1;                                                   
	account->sending_server_type           = EMF_SERVER_TYPE_SMTP;                                                   
	account->flag1                         = 2;
	account->flag2                         = 1;
	account->pop_before_smtp               = 0;
	account->apop                          = 0;
	account->logo_icon_path                = NULL;   
	account->preset_account                = 0;   
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
	account->target_storage                = 0;   
	account->check_interval                = 0; 
	account->keep_on_server	               = 1;

	account->account_name                  = strdup(address_string); 
	account->display_name                  = strdup(id_string); 
	account->email_addr                    = strdup(address_string); 
	account->reply_to_addr                 = strdup(address_string); 
	account->return_addr                   = strdup(address_string); 
	
	account->user_name                     = strdup(id_string); 
	account->password                      = strdup(password_string); 
	account->sending_user                  = strdup(id_string); 
	account->sending_password              = strdup(password_string); 

	switch (account_type) {
		case 1:/*  gawab */
			account->receiving_server_type  = EMF_SERVER_TYPE_POP3 ; 
			account->receiving_server_addr  = strdup(GWB_RECV_SERVER_ADDR); 
			account->port_num               = EMF_POP3S_PORT;     			
			account->sending_server_addr    = strdup(GWB_SMTP_SERVER_ADDR); 
			account->use_security           = 1;  
			account->sending_auth           = 1; 
			account->sending_port_num       = EMF_SMTPS_PORT; 
			account->sending_security       = 1;

			break;

		case 2:/*  vadofone */
			account->receiving_server_type  = EMF_SERVER_TYPE_IMAP4; 
			account->receiving_server_addr  = strdup(VDF_RECV_SERVER_ADDR); 
			account->port_num               = EMF_IMAP_PORT;        
			account->sending_server_addr    = strdup(VDF_SMTP_SERVER_ADDR); 
			account->use_security           = 0;  
			account->sending_auth           = 0; 
			break;

		case 4:/*  SAMSUNG 3G TEST */
			account->receiving_server_type  = EMF_SERVER_TYPE_POP3;
			account->receiving_server_addr  = strdup(S3G_RECV_SERVER_ADDR); 
			account->port_num               = S3G_RECV_SERVER_PORT;        
			account->sending_server_addr    = strdup(S3G_SMTP_SERVER_ADDR); 
			account->sending_port_num       = S3G_SMTP_SERVER_PORT;
 			account->use_security           = S3G_RECV_USE_SECURITY;                                                   
			account->sending_security       = S3G_SMTP_USE_SECURITY;
			account->sending_auth		        = S3G_SMTP_AUTH;
			break;
	
		case 5:/*  SAMSUNG 3G TEST */
			account->receiving_server_type  = EMF_SERVER_TYPE_IMAP4;
			account->receiving_server_addr  = strdup(S3G_RECV_SERVER_ADDR); 
			account->port_num               = EMF_IMAPS_PORT;        
			account->sending_server_addr    = strdup(S3G_SMTP_SERVER_ADDR); 
			account->sending_port_num       = S3G_SMTP_SERVER_PORT;
 			account->use_security           = 1;                                                   
			account->sending_security 	    = S3G_SMTP_USE_SECURITY;
			account->sending_auth		        = S3G_SMTP_AUTH;
			break;

		case 6:/*  Gmail POP3 */
			account->receiving_server_type  = EMF_SERVER_TYPE_POP3; 
			account->receiving_server_addr  = strdup("pop.gmail.com"); 
			account->port_num               = 995;        
			account->use_security           = 1;                                                   
			account->sending_server_addr    = strdup("smtp.gmail.com"); 
			account->sending_port_num       = 465;                                                        
			account->sending_security       = 1;  
			account->sending_auth           = 1;   
			break;

		case 7 : /*  Gmail IMAP4 */
			account->receiving_server_type  = EMF_SERVER_TYPE_IMAP4; 
			account->receiving_server_addr  = strdup("imap.gmail.com"); 
			account->port_num               = 993;        
			account->use_security           = 1;                                                   
			account->sending_server_addr    = strdup("smtp.gmail.com"); 
			account->sending_port_num       = 465;                                                        
			account->sending_security       = 1;  
			account->sending_auth           = 1;   
			break;

		case 8: /*  Active Sync */
			account->receiving_server_type  = EMF_SERVER_TYPE_ACTIVE_SYNC; 
			account->receiving_server_addr  = strdup(""); 
			account->port_num               = 0;        
			account->use_security           = 1;                                                   
			account->sending_server_addr    = strdup(""); 
			account->sending_port_num       = 0;                                                        
			account->sending_security       = 1;  
			account->sending_auth           = 1;   
			break;

		case 9: /*  AOL */
			account->receiving_server_type  = EMF_SERVER_TYPE_IMAP4; 
			account->receiving_server_addr  = strdup("imap.aol.com"); 
			account->port_num               = 143;        
			account->use_security           = 0;                                                   
			account->sending_server_addr    = strdup("smtp.aol.com"); 
			account->sending_port_num       = 587;                                                        
			account->sending_security       = 0;  
			account->sending_auth           = 1;   
			break;

		case 10: /*  Hotmail */
			account->receiving_server_type  = EMF_SERVER_TYPE_POP3; 
			account->receiving_server_addr  = strdup("pop3.live.com"); 
			account->port_num               = 995;        
			account->use_security           = 1;                                                   
			account->sending_server_addr    = strdup("smtp.live.com"); 
			account->sending_port_num       = 587;                                                        
			account->sending_security       = 0x02;  
			account->sending_auth           = 1;   
			break;

		case 11:/*  Daum IMAP4*/
			account->receiving_server_type  = EMF_SERVER_TYPE_IMAP4;
			account->receiving_server_addr  = strdup("imap.daum.net");
			account->port_num               = 993;
			account->use_security           = 1;
			account->sending_server_addr    = strdup("smtp.daum.net");
			account->sending_port_num       = 465;
            account->sending_security 	    = 1;
			account->sending_auth		    = 1;
			break;

		case 12:/*  Daum POP3*/
			account->receiving_server_type  = EMF_SERVER_TYPE_POP3;
			account->receiving_server_addr  = strdup("pop.daum.net");
			account->port_num               = 995;
			account->use_security           = 1;
			account->sending_server_addr    = strdup("smtp.daum.net");
			account->sending_port_num       = 465;
            account->sending_security 	    = 1;
			account->sending_auth		    = 1;
			break;

		default:
			testapp_print("Invalid Account Number\n");
			return FALSE;
			break;
	}
	account->my_account_id = 77;   
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
	int err = EMF_ERROR_NONE;
	
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
	
	scanf("%d",&account_type);

	if(!testapp_test_create_account_by_account_type(account_type,&err)) {
		testapp_print ("   testapp_test_create_account_by_account_type error\n");
		return FALSE;
	}
	return FALSE;
}

static gboolean testapp_test_update_account()
{
	int account_id;
	emf_account_t *account = NULL;
	char account_name[20];	
	int err = EMF_ERROR_NONE;
	char signature[100] = {0};
	char email_addr[50] = {0,};
	int add_my_address_to_bcc = 0;
	int my_account_id = 0, with_validation = 0;

	
	testapp_print("\n>> Enter Account No: ");
	scanf("%d",&account_id);

/* sowmya.kr, 281209 Adding signature to options in emf_account_t changes */
	if( (err = email_get_account(account_id, GET_FULL_DATA,&account)) < 0) {
		testapp_print ("email_get_account failed \n");
	}
	else
		testapp_print ("email_get_account result account_name - %s \n", account->account_name);

	testapp_print ("email_get_account result signature - %s \n", account->options.signature);

#ifdef __FEATURE_AUTO_POLLING__
	testapp_print ("email_get_account result check_interval - %d \n", account->check_interval);
#endif

	testapp_print("\n Enter new Account name:");
	scanf("%s",account_name);

	
	testapp_print("\n Enter new email addr:");
	scanf("%s",email_addr);
#ifdef __FEATURE_AUTO_POLLING__
	testapp_print("\n Enter new check interval (in mins):");
	scanf("%d",&(account->check_interval));
#endif
	testapp_print("\n Enter new signature:");
	scanf("%s",signature);

	testapp_print("\n>> Enter add_my_address_to_bcc:(0:off, 1:on) ");
	scanf("%d",&add_my_address_to_bcc);

	testapp_print("\n>> Enter my_account_id: ");
	scanf("%d",&my_account_id);

	testapp_print("\n>> With validation ? (0: No, 1:Yes) ");
	scanf("%d",&with_validation);

    if( account )  {
		account->account_name = strdup(account_name);
		testapp_print("\n Assigning New Account name: %p", account->account_name);
		account->email_addr = strdup(email_addr);
		account->options.signature = strdup(signature);
		testapp_print("\n Assigning New Account name: %p", account->options.signature);
		account->options.add_my_address_to_bcc = add_my_address_to_bcc;
		account->my_account_id= my_account_id;

		if(with_validation) {
			if((err = email_update_account_with_validation(account_id, account)) < 0) 
				testapp_print ("email_update_account_with_validation successful \n");
		}
		else {
			if((err = email_update_account(account_id, account)) < 0) 
				testapp_print ("email_update_account_with_validation successful \n");
		}
    }
	return FALSE;
}

static gboolean testapp_test_delete_account ()
{
	int account_id;
	emf_account_t *account=NULL;
	int err = EMF_ERROR_NONE;
	testapp_print("\n>> Enter Account No: ");
	scanf("%d",&account_id);

/* sowmya.kr, 281209 Adding signature to options in emf_account_t changes */
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
	int account_id;
	emf_account_t *account=NULL;
	int err_code = EMF_ERROR_NONE;
	unsigned account_handle = 0;
	
	testapp_print("\n>> Enter Account No: ");
	scanf("%d",&account_id);

/* sowmya.kr, 281209 Adding signature to options in emf_account_t changes */
	if( (err_code = email_get_account(account_id, WITHOUT_OPTION,&account)) < 0 ) {
		testapp_print ("email_get_account failed \n");
		return FALSE;
	}
	else
		testapp_print ("email_get_account result account_name - %s \n", account->account_name);

	if((err_code = email_validate_account(account_id, &account_handle)) == EMF_ERROR_NONE ) 
		testapp_print ("email_validate_account successful  account_handle : %u\n",account_handle);
	else
		testapp_print ("email_validate_account failed err_code: %d \n",err_code);
		
	return FALSE;

}


static gboolean testapp_test_cancel_validate_account ()
{
	int account_id = 0;
	int err_code = EMF_ERROR_NONE;
	unsigned account_handle = 0;

	testapp_print("\n > Enter account_id: ");
	scanf("%d", &account_id);

	testapp_print("\n > Enter handle: ");
	scanf("%d", &account_handle);

	err_code = email_cancel_job(account_id, account_handle);
	if(err_code == 0)
		testapp_print("email_cancel_job Success..!handle:[%d]", account_handle);
	else
		testapp_print ("email_cancel_job failed err_code: %d \n",err_code);
	
	return FALSE;
}

static gboolean testapp_test_get_account()
{
	int account_id;
	emf_account_t *account=NULL;
	int err_code = EMF_ERROR_NONE;
	testapp_print("\n>> Enter Account No: ");
	scanf("%d",&account_id);

/* sowmya.kr, 281209 Adding signature to options in emf_account_t changes */
	testapp_print ("email_get_account GET_FULL_DATA \n");
	if( (err_code = email_get_account(account_id,GET_FULL_DATA,&account)) < 0) {
		testapp_print ("email_get_account failed \n");
		return FALSE;
	}
	else
		testapp_print ("email_get_account result account_name - %s \n email_addr - %s \n use_security %d \n add_sig : %d \n  signature %s \n add_my_address_to_bcc %d \nmy_account_id %d\n", 
		account->account_name,
		account->email_addr,
		account->use_security,
		account->options.add_signature,
		account->options.signature,
		account->options.add_my_address_to_bcc,
		account->my_account_id
		);

	err_code = email_free_account(&account, 1);

	testapp_print ("email_get_account WITHOUT_OPTION \n");

	if( (err_code = email_get_account(account_id,WITHOUT_OPTION,&account)) < 0) {
		testapp_print ("email_get_account failed \n");
		return FALSE;
	}
	else
		testapp_print ("email_get_account result account_name - %s \n email_addr - %s \n use_security %d \n add_sig : %d \n ", 
		account->account_name,
		account->email_addr,
		account->use_security,
		account->options.add_signature);

	if(account->options.signature)
		testapp_print ("signature : %s \n", account->options.signature);
	else
		testapp_print ("signature not retrieved \n");

	err_code = email_free_account(&account, 1);

	testapp_print ("email_get_account ONLY_OPTION \n");

	if( (err_code = email_get_account(account_id,ONLY_OPTION,&account)) < 0) {
		testapp_print ("email_get_account failed \n");
		return FALSE;
	}
	else
		testapp_print ("email_get_account result add_sig : %d \n signature %s \n add_my_address_to_bcc %d\n my_account_id %d\n", 
		account->options.add_signature,
		account->options.signature,
		account->options.add_my_address_to_bcc,
		account->my_account_id
		);

	if(account->account_name)
		testapp_print ("account_name : %s \n", account->account_name);
	else
		testapp_print ("account_name not retrieved \n");	

	if(account->email_addr)
		testapp_print ("email_addr : %s \n", account->email_addr);
	else
		testapp_print ("email_addr not retrieved \n");	
	err_code = email_free_account(&account, 1);
		
	return FALSE;
}

static gboolean testapp_test_get_account_list ()
{

	int count, i;
	emf_account_t *account_list=NULL;
	struct timeval tv_1, tv_2;
	int interval;
	int err_code = EMF_ERROR_NONE;

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
			account_list[i].email_addr);
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
	int error_code, password_length, account_id;

	testapp_print("\n input account id\n");
	scanf("%d", &account_id);
	error_code = email_get_password_length_of_account(account_id, &password_length);
	testapp_print("testapp_test_get_password_length_of_account returned [%d]\n",password_length);
	return FALSE;
}

static gboolean testapp_test_query_server_info()
{
	int error_code;
	char domain_name[255];
	emf_server_info_t *result_server_info;

	testapp_print("\n input domain name\n");
	scanf("%s", domain_name);

	error_code = email_query_server_info(domain_name, &result_server_info);
	testapp_print("email_query_server_info returned [%d]\n",error_code);
	if(error_code == EMF_ERROR_NONE)
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
	
	while (go_to_loop) {
		testapp_show_menu (EMF_ACCOUNT_MENU);
		testapp_show_prompt (EMF_ACCOUNT_MENU);
			
		scanf ("%d", &menu_number);

		go_to_loop = testapp_test_interpret_command (menu_number);
	}	
}

