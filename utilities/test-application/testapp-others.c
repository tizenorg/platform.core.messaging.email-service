/*
*  email-service
*
* Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
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

/* open header */
#include <glib.h>
#include <dlfcn.h>
#include <vconf.h>
#include "c-client.h" 


#include "email-api.h"
#include "email-api-account.h"
#include "email-api-network.h"


/* internal header */
#include "testapp-utility.h"
#include "testapp-others.h"
#include "email-ipc.h"
#include "email-core-utils.h"

static gboolean testapp_test_get_network_status()
{
	int on_sending = 0;
	int on_receiving = 0;
	email_get_network_status(&on_sending, &on_receiving);
	testapp_print("\tNetwork status : \n On sending - %d \n On Receiving - %d \n", on_sending, on_receiving);
	return FALSE;
}

static gboolean testapp_test_get_pending_job()
{
	int action = -1;
	int account_id = 0;
	int mail_id = 0;
	int result_from_scanf = 0;
	email_event_status_type_t status = -1;
	testapp_print( " Enter Action \n SEND_MAIL = 0 \n SYNC_HEADER = 1 \n" \
			    " DOWNLOAD_BODY,= 2 \n DOWNLOAD_ATTACHMENT = 3 \n" \
			    " DELETE_MAIL = 4 \n SEARCH_MAIL = 5 \n SAVE_MAIL = 6 \n" \
			    " NUM = 7 \n");
	result_from_scanf = scanf("%d",&action);

	testapp_print("\n > Enter account_id: ");
	result_from_scanf = scanf("%d", &account_id);

	testapp_print("\n > Enter Mail Id: ");
	result_from_scanf = scanf("%d", &mail_id);

	if( email_get_pending_job( action, account_id, mail_id, &status) >= 0)
		testapp_print("\t status - %d \n",status);

	return FALSE;
}

static gboolean testapp_test_cancel_job	()
{
	int account_id = 0;
	int handle = 0;
	int result_from_scanf = 0;

	testapp_print("\n > Enter account_id (0: all account): ");
	result_from_scanf = scanf("%d", &account_id);

	testapp_print("\n > Enter handle: ");
	result_from_scanf = scanf("%d", &handle);

	if(email_cancel_job(account_id, handle, EMAIL_CANCELED_BY_USER) < 0)
		testapp_print("email_cancel_job failed..!");
	return FALSE;
}

static gboolean testapp_test_set_dnet_proper_profile_type()
{
	testapp_print("NOT Support\n");
	
	return TRUE;
}

static gboolean testapp_test_get_dnet_proper_profile_type()
{
	testapp_print("NOT Support\n");

	return TRUE;
}

static gboolean testapp_test_get_preview_text_from_file()
{
	int   result_from_scanf;
	char *preview_buffer = NULL;
	char  html_file_path[1024] = { 0, };

	testapp_print("\n > Enter file path : ");
	result_from_scanf = scanf("%s", html_file_path);

	emcore_get_preview_text_from_file(NULL, html_file_path, 1024, &preview_buffer);

	testapp_print("\n result :\n %s ", preview_buffer);

	return TRUE;
}

static gboolean testapp_test_print_receving_queue_via_debug_msg()
{
	int err;
	void* hAPI = (void*)emipc_create_email_api(_EMAIL_API_PRINT_RECEIVING_EVENT_QUEUE);

	if(hAPI == NULL)
		return EMAIL_ERROR_NULL_VALUE;
		
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		testapp_print("testapp_test_print_receving_queue_via_debug_msg - emipc_execute_proxy_api failed \n ");
		if(hAPI == NULL)
			return EMAIL_ERROR_NULL_VALUE;
	}

	emipc_get_parameter(hAPI, 1, 0, sizeof(int), &err);
	
	testapp_print(" >>>> RETURN VALUE : %d \n", err);

	emipc_destroy_email_api(hAPI);

	hAPI = NULL;
	testapp_print("testapp_test_print_receving_queue_via_debug_msg  ..........End\n");
	return err;
}

static gboolean testapp_test_create_db_full()
{
	int err;

	err = email_create_db_full();
	
	testapp_print("testapp_test_create_db_full returns [%d]", err);

	return err;
}

static int encode_base64(char *src, unsigned long src_len, char **enc, unsigned long* enc_len, int *err_code)
{
	unsigned char *content = NULL;
	int ret = true; 
	int err = EMAIL_ERROR_NONE;

	if (err_code != NULL) {
		*err_code = EMAIL_ERROR_NONE;
	}

	content = rfc822_binary(src, src_len, enc_len);

	if (content)
		*enc = (char *)content;
	else {
		err = EMAIL_ERROR_UNKNOWN;
		ret = false;
	}

	if (err_code)
	    *err_code = err;

	return ret;
}


static gboolean testapp_test_encoding_test()
{
	int error = EMAIL_ERROR_NONE;
	int has_special_character = 0, base64_file_name_length = 0, i;
	gsize bytes_read, bytes_written;
	char *encoded_file_name = NULL, *base64_file_name = NULL;
	SIZEDTEXT source_text;
	GError *glib_error = NULL;
	CHARSET *result_charset = NULL;
	char filename[] = {0xEB, 0xB0, 0x94, 0xED, 0x83, 0x95, 0x32, 0x2E, 0x70, 0x6E, 0x67, 0x00}; /* UTF-8 */

	source_text.data = (unsigned char*)filename; 
	source_text.size = strlen(filename);

	result_charset = (CHARSET*)utf8_infercharset(&source_text);

	if(result_charset) {
		testapp_print("return_charset->name [%s]", result_charset->name);
		encoded_file_name = (char*)g_convert (filename, -1, "UTF-8", result_charset->name, &bytes_read, &bytes_written, &glib_error);
	}
	else {
		i = 0;
		while(filename[i++] & 0x80)
			has_special_character = 1;
		testapp_print("has_special_character [%d]", has_special_character);
		if(has_special_character)
			encoded_file_name = (char*)g_convert (filename, -1, "UTF-8", "EUC-KR", &bytes_read, &bytes_written, &glib_error);
	}
	
	if(encoded_file_name == NULL)
		encoded_file_name = strdup(filename);

	testapp_print("encoded_file_name [%s]", encoded_file_name);

	if(!encode_base64(encoded_file_name, strlen(encoded_file_name), &base64_file_name, (unsigned long*)&base64_file_name_length, &error)) {
		testapp_print("encode_base64 failed. error [%d]", error);
		goto FINISH_OFF;
	}

	testapp_print("base64_file_name [%s]", base64_file_name);

	if(base64_file_name) {
		free(encoded_file_name);
		encoded_file_name = malloc(strlen(base64_file_name) + 15);
		if(!encoded_file_name) {
			testapp_print("em_malloc failed.");
			goto FINISH_OFF;
		}
		snprintf(encoded_file_name, strlen(base64_file_name) + 15, "=?UTF-8?B?%s?=", base64_file_name);
		testapp_print("encoded_file_name [%s]", encoded_file_name);
	}
FINISH_OFF:

	if (encoded_file_name)
		free(encoded_file_name);

	if (base64_file_name)
		free(base64_file_name);

	return error;
}

#define LIB_EMAIL_SERVICE_PATH	"/usr/lib/libemail-api.so"

int (*Datastore_FI_EMTB)(char **);
int (*Datastore_FI_EMSB)(char **);
int (*Datastore_FI_EMOB)(char **);
int (*Datastore_FI_EMDR)(char **);
int (*Datastore_FI_EMMF)(char **);
int (*Datastore_FI_EMTR)(char **);
int (*Datastore_FI_EMSP)(char **);

static gboolean email_test_dtt_Datastore_FI()
{
	void *handle = NULL;
	char *dl_error = NULL, *output_str = NULL;

	handle = dlopen(LIB_EMAIL_SERVICE_PATH, RTLD_LAZY | RTLD_GLOBAL);
	if (!handle) {
		dl_error = dlerror();
		if (dl_error)
			testapp_print("\t dlopen error : Open Library with absolute path return  :  %s\n", dl_error);
		return false;
	}	

	Datastore_FI_EMTB = dlsym(handle, "Datastore_FI_EMTB");
	Datastore_FI_EMSB = dlsym(handle, "Datastore_FI_EMSB");
	Datastore_FI_EMOB = dlsym(handle, "Datastore_FI_EMOB");
	Datastore_FI_EMDR = dlsym(handle, "Datastore_FI_EMDR");
	Datastore_FI_EMMF = dlsym(handle, "Datastore_FI_EMMF");
	Datastore_FI_EMTR = dlsym(handle, "Datastore_FI_EMTR");
	Datastore_FI_EMSP = dlsym(handle, "Datastore_FI_EMSP");

	Datastore_FI_EMTB(&output_str);

	testapp_print("\nemail_test_dtt_Datastore_FI\n%s\n", output_str);
	
	if(output_str)
		free(output_str);

	if (handle) {
		dlclose(handle);
		dl_error = dlerror();
		if (dl_error)
			testapp_print("\t Close handle return  :  %s\n", dl_error);
	}

	return true;
}

int (*Datastore_R_EMTB)(char **);
int (*Datastore_R_EMSB)(char **);
int (*Datastore_R_EMOB)(char **);
int (*Datastore_R_EMDR)(char **);
int (*Datastore_R_EMMF)(char **);
int (*Datastore_R_EMTR)(char **);
int (*Datastore_R_EMSP)(char **);


static gboolean email_test_dtt_Datastore_R()
{
	void *handle = NULL;
	char *dl_error = NULL, *output_str = NULL;

	handle = dlopen(LIB_EMAIL_SERVICE_PATH, RTLD_LAZY | RTLD_GLOBAL);
	if (!handle) {
		dl_error = dlerror();
		if (dl_error)
			testapp_print("\t dlopen error : Open Library with absolute path return  :  %s\n", dl_error);
		return false;
	}	

	Datastore_R_EMTB = dlsym(handle, "Datastore_R_EMTB");
	Datastore_R_EMSB = dlsym(handle, "Datastore_R_EMSB");
	Datastore_R_EMOB = dlsym(handle, "Datastore_R_EMOB");
	Datastore_R_EMDR = dlsym(handle, "Datastore_R_EMDR");
	Datastore_R_EMMF = dlsym(handle, "Datastore_R_EMMF");
	Datastore_R_EMTR = dlsym(handle, "Datastore_R_EMTR");
	Datastore_R_EMSP = dlsym(handle, "Datastore_R_EMSP");

	Datastore_R_EMTB(&output_str);

	testapp_print("\nemail_test_dtt_Datastore_R\n%s\n", output_str);

	if(output_str)
		free(output_str);

	if (handle) {
		dlclose(handle);
		dl_error = dlerror();
		if (dl_error)
			testapp_print("\t Close handle return  :  %s\n", dl_error);
	}

	return true;
}

int (*Datastore_C_EMTB)(char **);
int (*Datastore_C_EMSB)(char **);
int (*Datastore_C_EMOB)(char **);
int (*Datastore_C_EMDR)(char **);
int (*Datastore_C_EMMF)(char **);
int (*Datastore_C_EMTR)(char **);
int (*Datastore_C_EMSP)(char **);

static gboolean email_test_dtt_Datastore_C()
{
	void *handle = NULL;
	char *dl_error = NULL, *output_str = NULL;

	handle = dlopen(LIB_EMAIL_SERVICE_PATH, RTLD_LAZY | RTLD_GLOBAL);
	if (!handle) {
		dl_error = dlerror();
		if (dl_error)
			testapp_print("\t dlopen error : Open Library with absolute path return  :  %s\n", dl_error);
		return false;
	}	

	Datastore_C_EMTB = dlsym(handle, "Datastore_C_EMTB");
	Datastore_C_EMSB = dlsym(handle, "Datastore_C_EMSB");
	Datastore_C_EMOB = dlsym(handle, "Datastore_C_EMOB");
	Datastore_C_EMDR = dlsym(handle, "Datastore_C_EMDR");
	Datastore_C_EMMF = dlsym(handle, "Datastore_C_EMMF");
	Datastore_C_EMTR = dlsym(handle, "Datastore_C_EMTR");
	Datastore_C_EMSP = dlsym(handle, "Datastore_C_EMSP");

	Datastore_C_EMTB(&output_str);


	testapp_print("\nemail_test_dtt_Datastore_C\n%s\n", output_str);
	
	if(output_str)
		free(output_str);

	if (handle) {
		dlclose(handle);
		dl_error = dlerror();
		if (dl_error)
			testapp_print("\t Close handle return  :  %s\n", dl_error);
	}

	return true;
}

static gboolean testapp_test_show_user_message()
{
	int mail_id;
	int result_from_scanf = 0;

	testapp_print("\n > Enter mail id : ");
	result_from_scanf = scanf("%d", &mail_id);

	email_show_user_message(mail_id, EMAIL_ACTION_SEND_MAIL, EMAIL_ERROR_NETWORK_NOT_AVAILABLE);
	return FALSE;
}

static gboolean testapp_test_get_mime_entity()
{
	char mime_path[512] = {0, };
	int result_from_scanf = 0;
	char *mime_entity = NULL;

	testapp_print("\n > Enter mime path for parsing : ");
	result_from_scanf = scanf("%s", mime_path);
	
	email_get_mime_entity(mime_path, &mime_entity);

	testapp_print("\nmime_entity = %s\n", mime_entity);
	return true;
}

static gboolean testapp_test_interpret_command (int menu_number)
{
	gboolean go_to_loop = TRUE;

	switch (menu_number) {
		case 1:
			testapp_test_get_network_status();
			break;
		case 2:
			testapp_test_get_pending_job ();
			break;
		case 3:
			testapp_test_cancel_job ();
			break;
 		case 5:	
			testapp_test_set_dnet_proper_profile_type();
			break;
		case 6:
			testapp_test_get_dnet_proper_profile_type();
			break;
		case 7:
			testapp_test_get_preview_text_from_file();
			break;
		case 11:
			testapp_test_print_receving_queue_via_debug_msg();
			break;
		case 12:
			testapp_test_create_db_full();
			break;
		case 13:
			testapp_test_encoding_test();
			break;
		case 14:
			email_test_dtt_Datastore_FI();
			email_test_dtt_Datastore_C();
			email_test_dtt_Datastore_R();
			break;
		case 15:
			testapp_test_show_user_message();
			break;
		case 16:
			testapp_test_get_mime_entity();
			break;
		case 0:
			go_to_loop = FALSE;
			break;
		default:
			break;
	}

	return go_to_loop;
}

void testapp_others_main()
{
	gboolean go_to_loop = TRUE;
	int menu_number = 0;
	int result_from_scanf = 0;

	while (go_to_loop) {
		testapp_show_menu (EMAIL_OTHERS_MENU);
		testapp_show_prompt (EMAIL_OTHERS_MENU);

		result_from_scanf = scanf ("%d", &menu_number);

		go_to_loop = testapp_test_interpret_command (menu_number);
	}
}

