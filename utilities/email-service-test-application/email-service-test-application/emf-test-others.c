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

/* open header */
#include <glib.h>
#include <dlfcn.h>
#include <vconf.h>
#include "c-client.h" 


#include "Emf_Mapi.h"
#include "Emf_Mapi_Message.h"
#include "Emf_Mapi_Network.h"


/* internal header */
#include "emf-test-utility.h"
#include "emf-test-others.h"
#include "ipc-library.h"

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
	emf_event_status_type_t status = -1;
	testapp_print( " Enter Action \n SEND_MAIL = 0 \n SYNC_HEADER = 1 \n" \
			    " DOWNLOAD_BODY,= 2 \n DOWNLOAD_ATTACHMENT = 3 \n" \
			    " DELETE_MAIL = 4 \n SEARCH_MAIL = 5 \n SAVE_MAIL = 6 \n" \
			    " NUM = 7 \n");
	scanf("%d",&action);

	testapp_print("\n > Enter account_id: ");
	scanf("%d", &account_id);

	testapp_print("\n > Enter Mail Id: ");
	scanf("%d", &mail_id);

	if( email_get_pending_job( action, account_id, mail_id, &status) >= 0)
		testapp_print("\t status - %d \n",status);

	return FALSE;
}

static gboolean testapp_test_cancel_job	()
{
	int account_id = 0;
	int handle = 0;

	testapp_print("\n > Enter account_id (0: all account): ");
	scanf("%d", &account_id);

	testapp_print("\n > Enter handle: ");
	scanf("%d", &handle);

	if(email_cancel_job(account_id, handle) < 0)
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

static gboolean testapp_test_test_thread_functions()
{
	/*
	int result, thread_id, thread_item_count = 0;
	char sampletext[200] = " RE: re: FW: Re: RE: FWD: Hello re fw:qwe", *latest_thread_datetime = NULL;

	
	em_core_find_tag_for_thread_view(sampletext, &result);

	testapp_print("em_core_find_tag_for_thread_view returns = %d\n", result);

	if(result) {
		em_storage_get_thread_id_of_thread_mails(1, "Inbox", sampletext, &thread_id, &latest_thread_datetime, &thread_item_count);
		testapp_print("em_storage_get_thread_id_of_thread_mails returns = %d\n", thread_id);
	}
	*/
	
	return TRUE;
}

static gboolean testapp_test_print_receving_queue_via_debug_msg()
{
	int err;
	void* hAPI = (void*)ipcEmailAPI_Create(_EMAIL_API_PRINT_RECEIVING_EVENT_QUEUE);

	if(hAPI == NULL)
		return EMF_ERROR_NULL_VALUE;
		
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		testapp_print("testapp_test_print_receving_queue_via_debug_msg - ipcEmailProxy_ExecuteAPI failed \n ");
		if(hAPI == NULL)
			return EMF_ERROR_NULL_VALUE;
	}

	err = *(int*)ipcEmailAPI_GetParameter(hAPI, 1, 0);
	
	testapp_print(" >>>> RETURN VALUE : %d \n", err);

	ipcEmailAPI_Destroy(hAPI);

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
	int err = EMF_ERROR_NONE;

	if (err_code != NULL) {
		*err_code = EMF_ERROR_NONE;
	}

	content = rfc822_binary(src, src_len, enc_len);

	if (content)
		*enc = (char *)content;
	else {
		err = EMF_ERROR_UNKNOWN;
		ret = false;
	}

	if (err_code)
	    *err_code = err;

	return ret;
}


static gboolean testapp_test_encoding_test()
{
	int error = EMF_ERROR_NONE;
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
			testapp_print("em_core_malloc failed.");
			goto FINISH_OFF;
		}
		snprintf(encoded_file_name, strlen(base64_file_name) + 15, "=?UTF-8?B?%s?=", base64_file_name);
		testapp_print("encoded_file_name [%s]", encoded_file_name);
	}
FINISH_OFF:

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
		case 8:
			testapp_test_test_thread_functions();
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

	while (go_to_loop) {
		testapp_show_menu (EMF_OTHERS_MENU);
		testapp_show_prompt (EMF_OTHERS_MENU);

		scanf ("%d", &menu_number);

		go_to_loop = testapp_test_interpret_command (menu_number);
	}
}

