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

static gboolean testapp_test_ping_service()
{
	if (email_ping_service() < 0)
		testapp_print("email_ping_service failed..!");

	return FALSE;
}

static gboolean testapp_test_cancel_job()
{
	int account_id = 0;
	int handle = 0;

	testapp_print("\n > Enter account_id(0: all account): ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter handle: ");
	if (0 >= scanf("%d", &handle))
		testapp_print("Invalid input. ");

	if (email_cancel_job(account_id, handle, EMAIL_CANCELED_BY_USER) < 0)
		testapp_print("email_cancel_job failed..!");
	return FALSE;
}

static gboolean testapp_test_init_storage()
{
	if (email_init_storage() < 0)
		testapp_print("email_init_storaege failed..!");

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
	char *preview_buffer = NULL;
	char  html_file_path[1024] = { 0, };

	testapp_print("\n > Enter file path : ");
	if (0 >= scanf("%s", html_file_path))
		testapp_print("Invalid input. ");

	emcore_get_preview_text_from_file(NULL, NULL, html_file_path, 1024, &preview_buffer);

	testapp_print("\n result :\n %s ", preview_buffer);

	return TRUE;
}

static gboolean testapp_test_get_task_information()
{
	int i = 0;
	int err = EMAIL_ERROR_NONE;
	int task_information_count = 0;
	email_task_information_t *task_information_array = NULL;


	err = email_get_task_information(&task_information_array, &task_information_count);

	testapp_print("\n======================================================================\n");
	for (i = 0; i < task_information_count; i++)
		testapp_print("type[%d], account_id[%d], handle[%d], status[%d] nth[%d] progress[%d] mail_id[%d]", task_information_array[i].type, task_information_array[i].account_id, task_information_array[i].handle, task_information_array[i].status,(int)task_information_array[i].user_data2, (int)task_information_array[i].user_data1, task_information_array[i].mail_id );
	testapp_print("\n======================================================================\n");

	testapp_print("testapp_test_get_task_information  ..........End\n");

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

	if (result_charset) {
		testapp_print("return_charset->name [%s]", result_charset->name);
		encoded_file_name = (char*)g_convert(filename, -1, "UTF-8", result_charset->name, &bytes_read, &bytes_written, &glib_error);
	} else {
		i = 0;
		while (filename[i++] & 0x80)
			has_special_character = 1;
		testapp_print("has_special_character [%d]", has_special_character);
		if (has_special_character)
			encoded_file_name = (char*)g_convert(filename, -1, "UTF-8", "EUC-KR", &bytes_read, &bytes_written, &glib_error);
	}

	if (encoded_file_name == NULL)
		encoded_file_name = strdup(filename);

	testapp_print("encoded_file_name [%s]", encoded_file_name);

	if (!encode_base64(encoded_file_name, strlen(encoded_file_name), &base64_file_name, (unsigned long*)&base64_file_name_length, &error)) {
		testapp_print("encode_base64 failed. error [%d]", error);
		goto FINISH_OFF;
	}

	testapp_print("base64_file_name [%s]", base64_file_name);

	if (base64_file_name) {
		free(encoded_file_name);
		encoded_file_name = malloc(strlen(base64_file_name) + 15);
		if (!encoded_file_name) {
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

int(*Datastore_FI_EMTB)(char **);
int(*Datastore_FI_EMSB)(char **);
int(*Datastore_FI_EMOB)(char **);
int(*Datastore_FI_EMDR)(char **);
int(*Datastore_FI_EMMF)(char **);
int(*Datastore_FI_EMTR)(char **);
int(*Datastore_FI_EMSP)(char **);

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

	if (output_str)
		free(output_str);

	if (handle) {
		dlclose(handle);
		dl_error = dlerror();
		if (dl_error)
			testapp_print("\t Close handle return  :  %s\n", dl_error);
	}

	return true;
}

int(*Datastore_R_EMTB)(char **);
int(*Datastore_R_EMSB)(char **);
int(*Datastore_R_EMOB)(char **);
int(*Datastore_R_EMDR)(char **);
int(*Datastore_R_EMMF)(char **);
int(*Datastore_R_EMTR)(char **);
int(*Datastore_R_EMSP)(char **);


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

	if (output_str)
		free(output_str);

	if (handle) {
		dlclose(handle);
		dl_error = dlerror();
		if (dl_error)
			testapp_print("\t Close handle return  :  %s\n", dl_error);
	}

	return true;
}

int(*Datastore_C_EMTB)(char **);
int(*Datastore_C_EMSB)(char **);
int(*Datastore_C_EMOB)(char **);
int(*Datastore_C_EMDR)(char **);
int(*Datastore_C_EMMF)(char **);
int(*Datastore_C_EMTR)(char **);
int(*Datastore_C_EMSP)(char **);

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

	if (output_str)
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

	testapp_print("\n > Enter mail id : ");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");

	email_show_user_message(mail_id, EMAIL_ACTION_SEND_MAIL, EMAIL_ERROR_NETWORK_NOT_AVAILABLE);
	return FALSE;
}

static gboolean testapp_test_get_mime_entity()
{
	char mime_path[512] = {0, };
	char *mime_entity = NULL;

	testapp_print("\n > Enter mime path for parsing : ");
	if (0 >= scanf("%s", mime_path))
		testapp_print("Invalid input. ");

	email_get_mime_entity(mime_path, &mime_entity);

	testapp_print("\nmime_entity = %s\n", mime_entity);
	return true;
}

static gboolean testapp_test_query_smtp_mail_size_limit()
{
	int account_id = 0;
	testapp_print("\n > Enter account id : ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	email_query_smtp_mail_size_limit(account_id, NULL);
	return true;
}

static gboolean testapp_test_verify_email_address()
{
	int err = EMAIL_ERROR_NONE;
	char email_address[512] = {0, };

	testapp_print("\n > Enter mime path for parsing : ");
	if (0 >= scanf("%s", email_address))
		testapp_print("Invalid input. ");

	err = email_verify_email_address(email_address);

	testapp_print("\nemail_verify_email_address returns [%d]\n", err);
	return true;
}



static gboolean testapp_test_interpret_command(int menu_number)
{
	gboolean go_to_loop = TRUE;

	switch (menu_number) {
		case 1:
			testapp_test_ping_service();
			break;
		case 2:
			testapp_test_init_storage();
			break;
		case 3:
			testapp_test_cancel_job();
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
			testapp_test_get_task_information();
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
		case 17:
			testapp_test_query_smtp_mail_size_limit();
			break;
		case 18:
			testapp_test_verify_email_address();
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
		testapp_show_menu(EMAIL_OTHERS_MENU);
		testapp_show_prompt(EMAIL_OTHERS_MENU);

		if (0 >= scanf("%d", &menu_number))
			testapp_print("Invalid input. ");

		go_to_loop = testapp_test_interpret_command(menu_number);
	}
}

