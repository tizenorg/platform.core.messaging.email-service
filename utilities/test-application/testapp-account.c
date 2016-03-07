/*
*  email-service
*
* Copyright(c) 2012 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Kyuho Jo <kyuho.jo@samsung.com>, Sunghyun Kwon <sh0701.kwon@samsung.com>
*
* Licensed under the Apache License, Version 2.0(the "License");
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
#include "email-api-smime.h"
#include "email-api-account.h"
#include "email-api-network.h"
#include "email-types.h"

/* internal header */
#include "testapp-utility.h"
#include "testapp-account.h"
#include <sys/time.h>
#include <sys/times.h>
#include <tzplatform_config.h>

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

gboolean  testapp_create_account_object(email_account_t **result_account)
{
	email_account_t *account = NULL;
	char id_string[100] = { 0, }, password_string[1000] = { 0, }, address_string[100]  = { 0, };
	char accesss_token[1000] = { 0, }, refresh_token[1000] = { 0, };
	int samsung3g_account_index;
	int account_type;

	testapp_print("1. Gawab\n");
	testapp_print("2. Vodafone\n");
	testapp_print("4. SAMSUNG 3G TEST(POP)\n");
	testapp_print("5. SAMSUNG 3G TEST(IMAP)\n");
	testapp_print("6. Gmail(POP3)\n");
	testapp_print("7. Gmail(IMAP4)\n");
	testapp_print("8. Active Sync(dummy)\n");
	testapp_print("9. AOL\n");
	testapp_print("10. Hotmail\n");
	testapp_print("11. Daum(IMAP4)\n");
	testapp_print("12. Daum(POP3)\n");
	testapp_print("13. Yahoo(IMAP ID)\n");
	testapp_print("14. Gmail IMAP with XOAUTH\n");
	testapp_print("15. Yandex\n");
	testapp_print("16. mopera\n");
	testapp_print("Choose server type: ");


	if (0 >= scanf("%d", &account_type))
		testapp_print("Invalid input. ");

	switch (account_type) {
		case 4:
		case 5:
			do {
				testapp_print("Enter your account index [1~10] : ");
				if (0 >= scanf("%d", &samsung3g_account_index))
					testapp_print("Invalid input. ");
			} while (samsung3g_account_index > 10 || samsung3g_account_index < 1);
			sprintf(id_string, "test%02d", samsung3g_account_index);
			sprintf(address_string, "test%02d@streaming.s3glab.net", samsung3g_account_index);
			sprintf(password_string, "test%02d", samsung3g_account_index);
			break;
		case 14:
			testapp_print("Enter email address : ");
			if (0 >= scanf("%s", address_string))
				testapp_print("Invalid input. ");
			strcpy(id_string, address_string);

			testapp_print("Enter access token : ");
			if (0 >= scanf("%s", accesss_token))
				testapp_print("Invalid input. ");

			testapp_print("Enter refresh token : ");
			if (0 >= scanf("%s", refresh_token))
				testapp_print("Invalid input. ");

			snprintf(password_string, 100, "%s\001%s\001", accesss_token, refresh_token);
			break;
		default:
			testapp_print("Enter email address : ");
			if (0 >= scanf("%s", address_string))
				testapp_print("Invalid input. ");

			testapp_print("Enter id : ");
			if (0 >= scanf("%s", id_string))
				testapp_print("Invalid input. ");

			testapp_print("Enter password_string : ");
			if (0 >= scanf("%s", password_string))
				testapp_print("Invalid input. ");
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
	account->retrieval_mode                          = EMAIL_IMAP4_RETRIEVAL_MODE_ALL;
	account->incoming_server_secure_connection	     = 1;
	account->outgoing_server_type                    = EMAIL_SERVER_TYPE_SMTP;
	account->auto_download_size			             = 2;
	account->outgoing_server_use_same_authenticator  = 1;
	account->auto_resend_times                       = 3;
	account->pop_before_smtp                         = 0;
	account->incoming_server_requires_apop           = 0;
	account->incoming_server_authentication_method   = 0;
	account->logo_icon_path                          = NULL;
	account->color_label                             = (128 << 16) | (128 << 8) | (128);
	account->user_data                               = malloc(data_length);
	memcpy(account->user_data, (void *)&data, data_length);
	account->user_data_length                        = data_length;
	account->options.priority                        = EMAIL_MAIL_PRIORITY_NORMAL;
	account->options.keep_local_copy                 = 1;
	account->options.req_delivery_receipt            = 0;
	account->options.req_read_receipt                = 0;
	account->options.download_limit                  = 0;
	account->options.block_address                   = 0;
	account->options.block_subject                   = 0;
	account->options.display_name_from               = strdup("test");
	account->options.reply_with_body                 = 0;
	account->options.forward_with_files              = 0;
	account->options.add_myname_card                 = 0;
	account->options.add_signature                   = 1;
	account->options.signature                       = strdup("abcdef");
	account->options.add_my_address_to_bcc           = 0;
	account->check_interval                          = 0;
	account->keep_mails_on_pop_server_after_download = 1;
	account->default_mail_slot_size                  = 20;
	account->roaming_option                          = EMAIL_ROAMING_OPTION_RESTRICTED_BACKGROUND_TASK;
	account->peak_interval                           = 30;
	account->peak_days                               = EMAIL_PEAK_DAYS_MONDAY | EMAIL_PEAK_DAYS_TUEDAY | EMAIL_PEAK_DAYS_THUDAY | EMAIL_PEAK_DAYS_FRIDAY;
	account->peak_start_time                         = 830;
	account->peak_end_time                           = 1920;

	account->account_name                            = strdup(address_string);
	account->user_display_name                       = strdup(id_string);
	account->user_email_address                      = strdup(address_string);
	account->reply_to_address                        = strdup(address_string);
	account->return_address                          = strdup(address_string);

	account->incoming_server_user_name               = strdup(id_string);
	account->incoming_server_password                = strdup(password_string);
	account->outgoing_server_user_name               = strdup(id_string);
	account->outgoing_server_password	             = strdup(password_string);

	switch (account_type) {
		case 1:/*  gawab */
			account->incoming_server_type  		         = EMAIL_SERVER_TYPE_POP3 ;
			account->incoming_server_address	         = strdup(GWB_RECV_SERVER_ADDR);
			account->incoming_server_port_number         = EMAIL_POP3S_PORT;
			account->outgoing_server_address             = strdup(GWB_SMTP_SERVER_ADDR);
			account->incoming_server_secure_connection	 = 1;
			account->outgoing_server_need_authentication = 1;
			account->outgoing_server_port_number         = EMAIL_SMTPS_PORT;
			account->outgoing_server_secure_connection   = 1;

			break;

		case 2:/*  vadofone */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_IMAP4;
			account->incoming_server_address = strdup(VDF_RECV_SERVER_ADDR);
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
			account->incoming_server_address = strdup(S3G_RECV_SERVER_ADDR);
			account->incoming_server_port_number = EMAIL_IMAPS_PORT;
			account->outgoing_server_address     = strdup(S3G_SMTP_SERVER_ADDR);
			account->outgoing_server_port_number = S3G_SMTP_SERVER_PORT;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_secure_connection  = S3G_SMTP_USE_SECURITY;
			account->outgoing_server_need_authentication = S3G_SMTP_AUTH;
			break;

		case 6:/*  Gmail POP3 */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_POP3;
			account->incoming_server_address = strdup("pop.gmail.com");
			account->incoming_server_port_number = 995;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address    = strdup("smtp.gmail.com");
			account->outgoing_server_port_number = 465;
			account->outgoing_server_secure_connection = 1;
			account->outgoing_server_need_authentication = 1;
			break;

		case 7: /*  Gmail IMAP4 */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_IMAP4;
			account->incoming_server_address = strdup("imap.gmail.com");
			account->incoming_server_port_number = 993;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address = strdup("smtp.gmail.com");
			account->outgoing_server_port_number = 465;
			account->outgoing_server_secure_connection = 1;
			account->outgoing_server_need_authentication   = EMAIL_AUTHENTICATION_METHOD_DEFAULT;
			account->incoming_server_authentication_method = EMAIL_AUTHENTICATION_METHOD_NO_AUTH;
			break;

		case 8: /*  Active Sync */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_ACTIVE_SYNC;
			account->incoming_server_address = strdup("");
			account->incoming_server_port_number = 0;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address    = strdup("");
			account->outgoing_server_port_number = 0;
			account->outgoing_server_secure_connection = 1;
			account->outgoing_server_need_authentication = 1;
			break;

		case 9: /*  AOL */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_IMAP4;
			account->incoming_server_address = strdup("imap.aol.com");
			account->incoming_server_port_number = 143;
			account->incoming_server_secure_connection	= 0;
			account->outgoing_server_address    = strdup("smtp.aol.com");
			account->outgoing_server_port_number = 587;
			account->outgoing_server_secure_connection = 0;
			account->outgoing_server_need_authentication = 1;
			break;

		case 10: /*  Hotmail */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_POP3;
			account->incoming_server_address = strdup("pop3.live.com");
			account->incoming_server_port_number = 995;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address    = strdup("smtp.live.com");
			account->outgoing_server_port_number = 587;
			account->outgoing_server_secure_connection  = 0x02;
			account->outgoing_server_need_authentication = 1;
			break;

		case 11:/*  Daum IMAP4*/
			account->incoming_server_type  = EMAIL_SERVER_TYPE_IMAP4;
			account->incoming_server_address = strdup("imap.daum.net");
			account->incoming_server_port_number = 993;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address    = strdup("smtp.daum.net");
			account->outgoing_server_port_number = 465;
			account->outgoing_server_secure_connection = 1;
			account->outgoing_server_need_authentication = 1;
			break;

		case 12:/*  Daum POP3*/
			account->incoming_server_type  = EMAIL_SERVER_TYPE_POP3;
			account->incoming_server_address = strdup("pop.daum.net");
			account->incoming_server_port_number = 995;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address    = strdup("smtp.daum.net");
			account->outgoing_server_port_number = 465;
			account->outgoing_server_secure_connection = 1;
			account->outgoing_server_need_authentication = 1;
			break;

		case 13: /* Yahoo IMAP ID */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_IMAP4;
			account->incoming_server_address = strdup("samsung.imap.mail.yahoo.com");
			account->incoming_server_port_number = 993;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address    = strdup("samsung.smtp.mail.yahoo.com");
			account->outgoing_server_port_number = 465;
			account->outgoing_server_secure_connection = 1;
			account->outgoing_server_need_authentication = 1;
			break;

		case 14: /*  XOAUTH */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_IMAP4;
			account->incoming_server_address = strdup("imap.gmail.com");
			account->incoming_server_port_number = 993;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address = strdup("smtp.gmail.com");
			account->outgoing_server_port_number = 465;
			account->outgoing_server_secure_connection = 1;
			account->outgoing_server_need_authentication   = EMAIL_AUTHENTICATION_METHOD_XOAUTH2;
			account->incoming_server_authentication_method = EMAIL_AUTHENTICATION_METHOD_XOAUTH2;
			break;

		case 15: /*  yandex */
			account->incoming_server_type  = EMAIL_SERVER_TYPE_IMAP4;
			account->incoming_server_address = strdup("imap.yandex.ru");
			account->incoming_server_port_number = 993;
			account->incoming_server_secure_connection	= 1;
			account->outgoing_server_address = strdup("smtp.yandex.ru");
			account->outgoing_server_port_number = 465;
			account->outgoing_server_secure_connection = 1;
			account->outgoing_server_need_authentication   = EMAIL_AUTHENTICATION_METHOD_DEFAULT;
			account->incoming_server_authentication_method = EMAIL_AUTHENTICATION_METHOD_DEFAULT;
			break;

		case 16: /*  mopera */
			account->incoming_server_type                  = EMAIL_SERVER_TYPE_POP3;
			account->incoming_server_address               = strdup("mail.mopera.net");
			account->incoming_server_port_number           = 110;
			account->incoming_server_secure_connection	   = 0;
			account->incoming_server_authentication_method = EMAIL_AUTHENTICATION_METHOD_NO_AUTH;
			account->outgoing_server_address               = strdup("mail.mopera.net");
			account->outgoing_server_port_number           = 465;
			account->outgoing_server_secure_connection     = 0;
			account->outgoing_server_need_authentication   = EMAIL_AUTHENTICATION_METHOD_DEFAULT;
			break;

		default:
			testapp_print("Invalid Account Number\n");
			return FALSE;
			break;
	}
	account->account_svc_id = 77;

	if (result_account)
		*result_account = account;

	return TRUE;
}

static gboolean testapp_test_add_account_with_validation()
{
	int err = EMAIL_ERROR_NONE;
	email_account_t *account = NULL;
	int handle;

	if (!testapp_create_account_object(&account)) {
		testapp_print("testapp_test_create_account_by_account_type error\n");
		return FALSE;
	}

	err = email_add_account_with_validation(account, &handle);
	if (err < 0) {
		testapp_print("email_add_account_with_validation error : %d\n", err);
		err = email_free_account(&account, 1);
		return FALSE;
	}

	testapp_print("email_add_account succeed. account_id\n", account->account_id);

	err = email_free_account(&account, 1);

	return TRUE;
}

static gboolean testapp_test_update_account()
{
	int account_id;
	email_account_t *account = NULL;
	//char account_name[256];
	int err = EMAIL_ERROR_NONE;
	//char signature[100] = {0};
	//char user_email_address[256] = {0,};
	//int add_my_address_to_bcc = 0;
	int with_validation = 0; //account_svc_id = 0,

	testapp_print("\n>> Enter Account ID: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	if ((err = email_get_account(account_id, GET_FULL_DATA, &account)) != EMAIL_ERROR_NONE) {
		testapp_print("email_get_account failed - %d\n", err);
		return false;
	}

	testapp_print("email_get_account result account_name - %s \n", account->account_name);
	testapp_print("email_get_account result address - %s \n", account->user_email_address);
	testapp_print("email_get_account result signature - %s \n", account->options.signature);
	testapp_print("email_get_account result check_interval - %d \n", account->check_interval);

	testapp_print("\n Enter new check interval(in mins):");
	if (0 >= scanf("%d", &(account->check_interval)))
		testapp_print("Invalid input. ");
/*
	testapp_print("\n Enter new peak interval(in mins):");
	if (0 >= scanf("%d", &(account->peak_interval));

	testapp_print("\n Enter new peak days:");
	if (0 >= scanf("%d", &(account->peak_days));

	testapp_print("\n Enter new peak start time:");
	if (0 >= scanf("%d", &(account->peak_start_time));

	testapp_print("\n Enter new peak end time:");
	if (0 >= scanf("%d", &(account->peak_end_time));
*/

/*
	testapp_print("\n Enter new Account name:");
	if (0 >= scanf("%s",account_name);

	testapp_print("\n Enter new email addr:");
	if (0 >= scanf("%s",user_email_address);

	testapp_print("\n Enter new signature:");
	if (0 >= scanf("%s",signature);

	testapp_print("\n>> Enter add_my_address_to_bcc:(0:off, 1:on) ");
	if (0 >= scanf("%d", &add_my_address_to_bcc);

	testapp_print("\n>> Enter account_svc_id: ");
	if (0 >= scanf("%d", &account_svc_id);

	testapp_print("\n>> With validation ?(0: No, 1:Yes) ");
	if (0 >= scanf("%d", &with_validation);
*/
    if (account)  {
		testapp_print("\n Assigning New Account name:(%s)", account->account_name);
		testapp_print("\n Assigning New Signature:(%s)\n", account->options.signature);
		/*
		account->account_name = strdup(account_name);
		account->user_email_address = strdup(user_email_address);
		account->options.signature = strdup(signature);
		account->options.add_my_address_to_bcc = add_my_address_to_bcc;
		account->account_svc_id = account_svc_id;
		*/

		if (with_validation) {
			if ((err = email_update_account_with_validation(account_id, account)) != EMAIL_ERROR_NONE) {
				testapp_print("email_update_account_with_validation failed - %d\n", err);
				return false;
			}
				testapp_print("email_update_account_with_validation successful \n");
		} else {
			if ((err = email_update_account(account_id, account)) != EMAIL_ERROR_NONE) {
				testapp_print("email_update_account failed - %d\n", err);
				return false;
			}
			testapp_print("email_update_account successful \n");
		}
    }
	return true;
}

static gboolean testapp_test_delete_account()
{
	int account_id;
	email_account_t *account = NULL;
	int err = EMAIL_ERROR_NONE;

	testapp_print("\n>> Enter Account No: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

/* sowmya.kr, 281209 Adding signature to options in email_account_t changes */
	if ((err = email_get_account(account_id, WITHOUT_OPTION, &account)) < 0) {
		testapp_print("email_get_account failed \n");
		testapp_print("testapp_test_delete_account failed\n");
	} else {
		testapp_print("email_get_account result account_name - %s \n", account->account_name);
		if ((err = email_delete_account(account_id)) < 0)
			testapp_print("email_delete_account failed[%d]\n", err);
		else
			testapp_print("email_delete_account successful \n");
	}
	return FALSE;

}


static gboolean testapp_test_validate_account()
{
	email_account_t *account = NULL;
	int err_code = EMAIL_ERROR_NONE;
	int handle = 0;

	if (!testapp_create_account_object(&account)) {
		testapp_print("testapp_create_account_object error\n");
		return FALSE;
	}

	if ((err_code = email_validate_account_ex(account, &handle)) == EMAIL_ERROR_NONE)
		testapp_print("email_validate_account_ex successful handle : %u\n", handle);
	else
		testapp_print("email_validate_account_ex failed err_code : %d \n", err_code);

	if (account)
		email_free_account(&account, 1);

	return FALSE;

}


static gboolean testapp_test_cancel_validate_account()
{
	int account_id = 0;
	int err_code = EMAIL_ERROR_NONE;
	unsigned account_handle = 0;

	testapp_print("\n > Enter account_id: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter handle: ");
	if (0 >= scanf("%d", &account_handle))
		testapp_print("Invalid input. ");

	err_code = email_cancel_job(account_id, account_handle, EMAIL_CANCELED_BY_USER);
	if (err_code == 0)
		testapp_print("email_cancel_job Success..!handle:[%d]", account_handle);
	else
		testapp_print("email_cancel_job failed err_code: %d \n", err_code);

	return FALSE;
}

static gboolean testapp_test_get_account()
{
	int account_id;
	email_account_t *account = NULL;
	int err_code = EMAIL_ERROR_NONE;
	testapp_print("\n>> Enter Account No: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	typedef struct {
		int is_preset_account;
		int index_color;
	} user_data_t;

	testapp_print("\n----------------------------------------------------------\n");
	testapp_print("email_get_account GET_FULL_DATA \n");
	if ((err_code = email_get_account(account_id, GET_FULL_DATA, &account)) < 0) {
		testapp_print("email_get_account failed - %d\n", err_code);
		return FALSE;
	}

	user_data_t *val = (user_data_t *)account->user_data;
	int is_preset_account =  val ? val->is_preset_account : 0;
	int index_color = val ? val->index_color : 0;

	testapp_print("email_get_account result\n"
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
			"auto_resend_times %d\n"
			"roaming_option %d\n"
			"peak_interval %d\n"
			"peak_days %d\n"
			"peak_start_time %d\n"
			"peak_end_time %d\n"
			"color_label %d\n"
			"notification_status %d\n"
			"vibrate_status %d\n"
			"display_content_status %d\n"
			"default_ringtone_status %d\n"
			"alert_ringtone_path %s\n"
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
		account->digest_type,
		account->auto_resend_times,
		account->roaming_option,
		account->peak_interval,
		account->peak_days,
		account->peak_start_time,
		account->peak_end_time,
		account->color_label,
		account->options.notification_status,
		account->options.vibrate_status,
		account->options.display_content_status,
		account->options.default_ringtone_status,
		account->options.alert_ringtone_path
		);

	err_code = email_free_account(&account, 1);

	testapp_print("\n----------------------------------------------------------\n");
	testapp_print("email_get_account WITHOUT_OPTION \n");

	if ((err_code = email_get_account(account_id, WITHOUT_OPTION, &account)) < 0) {
		testapp_print("email_get_account failed \n");
		return FALSE;
	}

	testapp_print("email_get_account result\n"
			"account_name - %s \n"
			"user_email_address - %s \n"
			"incoming_server_secure_connection %d \n"
			"add_signature : %d \n",
		account->account_name,
		account->user_email_address,
		account->incoming_server_secure_connection,
		account->options.add_signature
	);

	if (account->options.signature)
		testapp_print("signature : %s\n", account->options.signature);
	else
		testapp_print("signature not retrieved \n");

	err_code = email_free_account(&account, 1);

	testapp_print("\n----------------------------------------------------------\n");
	testapp_print("email_get_account ONLY_OPTION \n");

	if ((err_code = email_get_account(account_id, ONLY_OPTION, &account)) < 0) {
		testapp_print("email_get_account failed \n");
		return FALSE;
	}

	testapp_print("email_get_account result\n"
			"add_sig : %d \n"
			"signature %s \n"
			"add_my_address_to_bcc %d\n"
			"account_svc_id %d\n",
		account->options.add_signature,
		account->options.signature,
		account->options.add_my_address_to_bcc,
		account->account_svc_id
		);

	if (account->account_name)
		testapp_print("account_name : %s \n", account->account_name);
	else
		testapp_print("account_name not retrieved \n");

	if (account->user_email_address)
		testapp_print("user_email_address : %s \n", account->user_email_address);
	else
		testapp_print("user_email_address not retrieved \n");
	err_code = email_free_account(&account, 1);

	return FALSE;
}

static gboolean testapp_test_get_account_list()
{

	int count, i;
	email_account_t *account_list = NULL;
	struct timeval tv_1, tv_2;
	int interval;
	int err_code = EMAIL_ERROR_NONE;

	gettimeofday(&tv_1, NULL);

	if ((err_code = email_get_account_list(&account_list, &count)) < 0) {
		testapp_print("   email_get_account_list error\n");
		return false ;
	}

	gettimeofday(&tv_2, NULL);
	interval = tv_2.tv_usec - tv_1.tv_usec;
	testapp_print("\t testapp_test_get_account_list Proceed time %d us\n", interval);

	for (i = 0; i < count; i++) {
		testapp_print("   %2d) %-15s %-30s\n", account_list[i].account_id,
			account_list[i].account_name,
			account_list[i].user_email_address);
	}

	err_code = email_free_account(&account_list, count);
	return FALSE;
}

static gboolean testapp_test_update_check_interval()
{
	int account_id = 0;
	int err_code = EMAIL_ERROR_NONE;
	email_account_t *account = NULL;

	testapp_print("\n Enter account id :");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	if ((err_code = email_get_account(account_id, GET_FULL_DATA_WITHOUT_PASSWORD, &account)) != EMAIL_ERROR_NONE) {
		testapp_print("email_get_account failed [%d]\n", err_code);
		goto FINISH_OFF;
	}

	testapp_print("\n Enter new check interval(in mins):");
	if (0 >= scanf("%d", &(account->check_interval)))
		testapp_print("Invalid input. ");

	if ((err_code = email_update_account(account_id, account)) != EMAIL_ERROR_NONE) {
		testapp_print("email_update_account failed [%d]\n", err_code);
		goto FINISH_OFF;
	}

	testapp_print("email_update_account successful \n");

FINISH_OFF:
	if (account)
		email_free_account(&account, 1);

	return err_code;
}

static gboolean testapp_test_backup_account()
{
	char *file_name = tzplatform_mkpath(TZ_SYS_DATA, "email/accounts_file");
	int error_code;
	error_code = email_backup_accounts_into_secure_storage(file_name);
	testapp_print("\n email_backup_accounts_into_secure_storage returned [%d]\n", error_code);
	return FALSE;
}
static gboolean testapp_test_restore_account()
{
	char *file_name = tzplatform_mkpath(TZ_SYS_DATA,"email/accounts_file");
	int error_code;
	error_code = email_restore_accounts_from_secure_storage(file_name);
	testapp_print("\n email_restore_accounts_from_secure_storage returned [%d]\n", error_code);
	return FALSE;
}

static gboolean testapp_test_get_password_length_of_account()
{
	int password_length, account_id;

	testapp_print("\n input account id\n");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");
	email_get_password_length_of_account(account_id, EMAIL_GET_INCOMING_PASSWORD_LENGTH, &password_length);
	testapp_print("testapp_test_get_password_length_of_account returned [%d]\n", password_length);
	return FALSE;
}

static gboolean testapp_test_update_notification()
{
	int error_code;
	int account_id = 0;
	int t_mail_count = 0;
	int input_from_eas = 0;
	int unread_mail_count = 0;

	testapp_print("\n Input account ID:\n");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	testapp_print("\n Input mail count:\n");
	if (0 >= scanf("%d", &t_mail_count))
		testapp_print("Invalid input. ");

	testapp_print("\n Input unread mail count:\n");
	if (0 >= scanf("%d", &unread_mail_count))
		testapp_print("Invalid input. ");

	testapp_print("\n Input From eas:\n");
	if (0 >= scanf("%d", &input_from_eas))
		testapp_print("Invalid input. ");

	error_code = email_update_notification_bar(account_id, t_mail_count, unread_mail_count, input_from_eas);
	testapp_print("email_update_notification_bar returned [%d]\n", error_code);
	return FALSE;
}

static gboolean testapp_test_clear_notification()
{
	int account_id = 0;
	int error_code;

	testapp_print("\n Input account ID:\n");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	error_code = email_clear_notification_bar(account_id);
	testapp_print("email_clear_notification_bar returned [%d]\n", error_code);
	return FALSE;
}

static gboolean testapp_test_clear_all_notification()
{
	int error_code;

	error_code = email_clear_notification_bar(ALL_ACCOUNT);
	testapp_print("email_clear_notification_bar returned [%d]\n", error_code);
	return FALSE;
}

static gboolean testapp_test_save_default_account_id()
{
	int error_code;
	int account_id = 0;

	testapp_print("\nInput default account id : ");

	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	error_code = email_save_default_account_id(account_id);

	testapp_print("email_save_default_account_id returned [%d]\n", error_code);
	return FALSE;
}

static gboolean testapp_test_load_default_account_id()
{
	int error_code;
	int account_id = 0;

	error_code = email_load_default_account_id(&account_id);

	testapp_print("\ndefault account id : %d\n", account_id);
	testapp_print("email_load_default_account_id returned [%d]\n", error_code);
	return FALSE;
}

static gboolean testapp_test_add_account()
{
	int err = EMAIL_ERROR_NONE;
	email_account_t *account = NULL;

	if (!testapp_create_account_object(&account)) {
		testapp_print("testapp_test_create_account_by_account_type error\n");
		return FALSE;
	}

	err = email_add_account(account);
	if (err < 0) {
		testapp_print("email_add_account error : %d\n", err);
		err = email_free_account(&account, 1);
		return FALSE;
	}

	testapp_print("email_add_account succeed. account_id\n", account->account_id);

	err = email_free_account(&account, 1);

	return true;
}

static gboolean testapp_test_update_peak_schedule()
{
	int account_id;
	email_account_t *account = NULL;
	int err = EMAIL_ERROR_NONE;

	testapp_print("\n>> Enter Account No: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	if ((err = email_get_account(account_id, GET_FULL_DATA, &account)) != EMAIL_ERROR_NONE) {
		testapp_print("email_get_account failed [%d]\n", err);
		return false;
	}

	testapp_print("old check_interval - %d \n", account->check_interval);
	testapp_print("\n Enter new check interval(in mins):");
	if (0 >= scanf("%d", &(account->check_interval)))
		testapp_print("Invalid input. ");

	testapp_print("old peak_interval - %d \n", account->peak_interval);

	testapp_print("\n Enter new peak interval(in mins):");
	if (0 >= scanf("%d", &(account->peak_interval)))
		testapp_print("Invalid input. ");

	testapp_print("old peak_days - %d \n", account->peak_days);

	testapp_print("\n Enter new peak days:");
	if (0 >= scanf("%d", &(account->peak_days)))
		testapp_print("Invalid input. ");

	testapp_print("old peak_start_time - %d \n", account->peak_start_time);

	testapp_print("\n Enter new peak start time:");
	if (0 >= scanf("%d", &(account->peak_start_time)))
		testapp_print("Invalid input. ");

	testapp_print("old peak_end_time - %d \n", account->peak_start_time);

	testapp_print("\n Enter new peak end time:");
	if (0 >= scanf("%d", &(account->peak_end_time)))
		testapp_print("Invalid input. ");

	if (account)  {
		if ((err = email_update_account(account_id, account)) != EMAIL_ERROR_NONE) {
			testapp_print("email_update_account failed [%d]\n", err);
			return false;
		}
		testapp_print("email_update_account successful \n");
	}
	return true;
}

static gboolean testapp_test_interpret_command(int selected_number)
{
	gboolean go_to_loop = TRUE;

	switch (selected_number) {
		case 1:
			testapp_test_add_account_with_validation();
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

		case 6:
			testapp_test_update_check_interval();
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

		case 13:
			testapp_test_update_notification();
			break;

		case 14:
			testapp_test_clear_notification();
			break;

		case 15:
			testapp_test_clear_all_notification();
			break;

		case 16:
			testapp_test_save_default_account_id();
			break;

		case 17:
			testapp_test_load_default_account_id();
			break;

		case 18:
			testapp_test_add_account();
			break;

		case 19:
			testapp_test_update_peak_schedule();
			break;

		case 0:
			go_to_loop = FALSE;
			break;

		default:
			break;
	}

	return go_to_loop;
}

void testapp_account_main()
{
	gboolean go_to_loop = TRUE;
	int menu_number = 0;

	while (go_to_loop) {
		testapp_show_menu(EMAIL_ACCOUNT_MENU);
		testapp_show_prompt(EMAIL_ACCOUNT_MENU);

		if (0 >= scanf("%d", &menu_number))
			testapp_print("Invalid input");

		go_to_loop = testapp_test_interpret_command(menu_number);
	}
}

