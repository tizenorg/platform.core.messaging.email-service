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

/*
 * email-utilities.c
 *
 *  Created on: 2012. 3. 6.
 *      Author: kyuho.jo@samsung.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>
#include <regex.h>
#include <security-server.h>

#include "c-client.h"

#include "email-types.h"
#include "email-internal-types.h"
#include "email-utilities.h"

INTERNAL_FUNC void* em_malloc(int len)
{
	/* EM_DEBUG_LOG("Memory allocation size[%d] bytes", len); */
	if (len <= 0) {
		EM_DEBUG_LOG ("len should be positive.[%d]", len);
		return NULL;
	}

	void *p = calloc(1,len);
	if (!p)
		EM_DEBUG_PERROR("malloc failed");

	return p;
}


INTERNAL_FUNC void* em_memdup(void* src, int len)
{
	/* EM_DEBUG_LOG("Memory allocation size[%d] bytes", len); */
	if (len <= 0) {
		EM_DEBUG_LOG ("len should be positive.[%d]", len);
		return NULL;
	}

	void *p = calloc(1,len);
	if (!p) {
		EM_DEBUG_EXCEPTION("malloc failed");
		return NULL;
	}

	memcpy(p, src, len);

	return p;
}


/*  remove left space, tab, CR, L */
INTERNAL_FUNC char *em_trim_left(char *str)
{
	char *p, *temp_buffer = NULL;

	/* EM_DEBUG_FUNC_BEGIN() */
	if (!str) return NULL;

	p = str;
	while (*p && (*p == ' ' || *p == '\t' || *p == LF || *p == CR)) p++;

	if (!*p) return NULL;

	temp_buffer = EM_SAFE_STRDUP(p);

	strncpy(str, temp_buffer, EM_SAFE_STRLEN(str));
	str[EM_SAFE_STRLEN(temp_buffer)] = NULL_CHAR;

	EM_SAFE_FREE(temp_buffer);

	return str;
}

/*  remove right space, tab, CR, L */
INTERNAL_FUNC char *em_trim_right(char *str)
{
	char *p;

	/* EM_DEBUG_FUNC_BEGIN() */
	if (!str) return NULL;

	p = str+EM_SAFE_STRLEN(str)-1;
	while (((int)p >= (int)str) && (*p == ' ' || *p == '\t' || *p == LF || *p == CR))
		*p --= '\0';

	if ((int) p < (int)str)
		return NULL;

	return str;
}

INTERNAL_FUNC char* em_upper_string(char *str)
{
	char *p = str;
	while (*p)  {
		*p = toupper(*p);
		p++;
	}
	return str;
}

INTERNAL_FUNC char*  em_lower_string(char *str)
{
	char *p = str;
	while (*p)  {
		*p = tolower(*p);
		p++;
	}
	return str;
}

INTERNAL_FUNC int em_upper_path(char *path)
{
	int i = 0, is_utf7 = 0, len = path ? (int)EM_SAFE_STRLEN(path) : -1;
	for (; i < len; i++) {
		if (path[i] == '&' || path[i] == 5) {
			is_utf7 = 1;
		}
		else {
			if (is_utf7) {
				if (path[i] == '-') is_utf7 = 0;
			}
			else {
				path[i] = toupper(path[i]);
			}
		}
	}

	return 1;
}

INTERNAL_FUNC void em_skip_whitespace(char *addr_str, char **pAddr)
{
	EM_DEBUG_FUNC_BEGIN("addr_str[%p]", addr_str);

	if (!addr_str)
		return ;
	char *str = addr_str;
	char ptr[EM_SAFE_STRLEN(str)+1]  ;
	int i, j = 0;

	str = addr_str ;
	for (i = 0; str[i] != NULL_CHAR ; i++) {
		if (str[i] != SPACE && str[i] != TAB && str[i] != CR && str[i] != LF)
			ptr[j++] = str[i];
	}
	ptr[j] = NULL_CHAR;

	*pAddr = EM_SAFE_STRDUP(ptr);
	EM_DEBUG_FUNC_END("ptr[%s]", ptr);
}

INTERNAL_FUNC char* em_skip_whitespace_without_strdup(char *source_string)
{
	EM_DEBUG_FUNC_BEGIN("source_string[%p]", source_string);

	if (!source_string)
		return NULL;
	int i;

	for (i = 0; source_string[i] != NULL_CHAR ; i++) {
		if (source_string[i] != SPACE) /*  || source_string[i] != TAB || source_string[i] != CR || source_string[i] || LF) */
			break;
	}

	EM_DEBUG_FUNC_END("i[%d]", i);
	return source_string + i;
}

INTERNAL_FUNC char* em_replace_all_string(char *source_string, char *old_string, char *new_string)
{
	EM_DEBUG_FUNC_BEGIN();
	char *result_buffer = NULL;
	int i = 0, j = 0;
	int old_str_length = 0;
	int new_str_length = 0;
	int realloc_len = 0;

	EM_IF_NULL_RETURN_VALUE(source_string, NULL);
	EM_IF_NULL_RETURN_VALUE(old_string, NULL);
	EM_IF_NULL_RETURN_VALUE(new_string, NULL);

	int src_len = EM_SAFE_STRLEN(source_string);
	old_str_length = EM_SAFE_STRLEN(old_string);
	new_str_length = EM_SAFE_STRLEN(new_string);

	if (src_len <= 0) {
		EM_DEBUG_LOG("source_string is invalid");
		return NULL;
	}

	result_buffer = calloc(src_len+1, sizeof(char));
	if (!result_buffer) {
		EM_DEBUG_EXCEPTION("calloc failed");
		return NULL;
	}

	if (!strstr(source_string, old_string)) {
		memcpy(result_buffer, source_string, src_len);
		return result_buffer;
	}

	realloc_len = src_len + 1;

	for (i = 0; i < src_len && source_string[i] != '\0';) {
		if (old_str_length <= src_len - i &&
				memcmp(&source_string[i], old_string, old_str_length) == 0) {

			if (old_str_length != new_str_length) {
				realloc_len = realloc_len - old_str_length + new_str_length;
				result_buffer = realloc(result_buffer, realloc_len);
				if (!result_buffer) {
					EM_DEBUG_EXCEPTION("realloc failed");
					return NULL;
				}
			}
			memcpy(&result_buffer[j], new_string, new_str_length);
			i += old_str_length;
			j += new_str_length;
		} else {
			result_buffer[j] = source_string[i];
			i++;
			j++;
		}
	}

	if (j < realloc_len)
		result_buffer[j] = '\0';
	else
		result_buffer[realloc_len-1] = '\0';

	EM_DEBUG_FUNC_END();
	return result_buffer;
}

INTERNAL_FUNC char* em_replace_string(char *source_string, char *old_string, char *new_string)
{
	EM_DEBUG_FUNC_BEGIN();
	char *result_buffer = NULL;
	char *p = NULL;
	int   buffer_length = 0;

	EM_IF_NULL_RETURN_VALUE(source_string, NULL);
	EM_IF_NULL_RETURN_VALUE(old_string, NULL);
	EM_IF_NULL_RETURN_VALUE(new_string, NULL);

	p = strstr(source_string, old_string);

	if (p == NULL) {
		EM_DEBUG_EXCEPTION("old_string not found in source_string");
		EM_DEBUG_FUNC_END("return NULL");
		return NULL;
	}

	buffer_length	= EM_SAFE_STRLEN(source_string) + 1024;
	result_buffer  = (char *)em_malloc(buffer_length);

	if (!result_buffer) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		return NULL;
	}

	strncpy(result_buffer, source_string, p - source_string);
	snprintf(result_buffer + strlen(result_buffer), buffer_length - strlen(result_buffer), "%s%s", new_string, p + strlen(old_string)); /*prevent 34351*/

	EM_DEBUG_FUNC_END("result_buffer[%p]", result_buffer);
	return result_buffer;
}

/* Memory clean up */
#include <sys/mman.h>

/* #define GETSP() 				({ unsigned int sp; asm volatile ("mov %0, sp " : "=r"(sp)); sp;}) */
#define BUF_SIZE				256
#define PAGE_SIZE			    (1 << 12)
#define _ALIGN_UP(addr, size)   (((addr)+((size)-1))&(~((size)-1)))
#define _ALIGN_DOWN(addr, size) ((addr)&(~((size)-1)))
#define PAGE_ALIGN(addr)        _ALIGN_DOWN(addr, PAGE_SIZE)

int stack_trim(void)
{
	/*
	char buf[BUF_SIZE];
	FILE *file;
	unsigned int stacktop;
	int found = 0;
	unsigned int sp;

	asm volatile ("mov %0, sp " : "=r"(sp));

	sprintf(buf, "/proc/%d/maps", getpid());
	file = fopen(buf, "r");
	while (fgets(buf, BUF_SIZE, file) != NULL) {
		if (strstr(buf, "[stack]")) {
			found = 1;
			break;
		}
	}

	fclose(file);

	if (found) {
		sscanf(buf, "%x-", &stacktop);
		if (madvise((void *)PAGE_ALIGN(stacktop), PAGE_ALIGN(sp)-stacktop, MADV_DONTNEED) < 0)
			perror("stack madvise fail");
	}
	*/
	return 1;
}

INTERNAL_FUNC void em_flush_memory()
{
	EM_DEBUG_FUNC_BEGIN();
	/*  flush memory in heap */
	malloc_trim(0);

	/*  flush memory in stack */
	stack_trim();

	/*  flush memory for sqlite */
	emstorage_flush_db_cache();
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int em_get_file_name_from_file_path(char *input_source_file_path, char **output_file_name)
{
	EM_DEBUG_FUNC_BEGIN_SEC("input_source_file_path[%s], output_file_name [%p]", input_source_file_path, output_file_name);
	int   err = EMAIL_ERROR_NONE;
	int   pos_on_string = 0;
	int   file_name_length = 0;
	char *start_pos_of_file_name = NULL;
	char *end_pos_of_file_name = NULL;
	char *end_pos_of_file_path = NULL;
	char  file_name_string[MAX_PATH] = { 0, };

	if (!input_source_file_path || !output_file_name) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err  = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	pos_on_string        = EM_SAFE_STRLEN(input_source_file_path) - 1;
	end_pos_of_file_path = input_source_file_path + pos_on_string;
	end_pos_of_file_name = end_pos_of_file_path;

	while(pos_on_string >= 0 && input_source_file_path[pos_on_string] != '/') {
		pos_on_string--;
	}

	pos_on_string++;

	if(pos_on_string >= 0) {
		start_pos_of_file_name = input_source_file_path + pos_on_string;
		file_name_length       = end_pos_of_file_name - start_pos_of_file_name + 1;
		memcpy(file_name_string, start_pos_of_file_name, file_name_length);
	}

	EM_DEBUG_LOG_SEC("file_name_string [%s] pos_on_string [%d] file_name_length [%d]", file_name_string, pos_on_string, file_name_length);

	*output_file_name = EM_SAFE_STRDUP(file_name_string);

FINISH_OFF:
	EM_DEBUG_FUNC_END("err = [%d]", err);
	return err;
}

INTERNAL_FUNC int em_get_file_name_and_extension_from_file_path(char *input_source_file_path, char **output_file_name, char **output_extension)
{
	EM_DEBUG_FUNC_BEGIN_SEC("input_source_file_path[%s], output_file_name [%p], output_extension [%p]", input_source_file_path, output_file_name, output_extension);
	int   err = EMAIL_ERROR_NONE;
	int   pos_on_string = 0;
	int   file_name_length = 0;
	int   extention_length = 0;
	char *start_pos_of_file_name = NULL;
	char *end_pos_of_file_name = NULL;
	char *dot_pos_of_file_path = NULL;
	char *end_pos_of_file_path = NULL;
	char  file_name_string[MAX_PATH] = { 0, };
	char  extension_string[MAX_PATH] = { 0, };

	if (!input_source_file_path || !output_file_name || !output_extension) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err  = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	pos_on_string        = EM_SAFE_STRLEN(input_source_file_path) - 1;
	end_pos_of_file_path = input_source_file_path + pos_on_string;
	end_pos_of_file_name = end_pos_of_file_path;

	while(pos_on_string >= 0 && input_source_file_path[pos_on_string] != '/') {
		if(input_source_file_path[pos_on_string] == '.') {
			if(dot_pos_of_file_path == NULL) {
				end_pos_of_file_name = input_source_file_path + pos_on_string;
				dot_pos_of_file_path = end_pos_of_file_name;
			}
		}
		pos_on_string--;
	}

	pos_on_string++;

	if(pos_on_string >= 0) {
		start_pos_of_file_name = input_source_file_path + pos_on_string;
		file_name_length       = end_pos_of_file_name - start_pos_of_file_name;
		memcpy(file_name_string, start_pos_of_file_name, file_name_length);
	}

	if(dot_pos_of_file_path != NULL) {
		extention_length       = (end_pos_of_file_path + 1) - (dot_pos_of_file_path + 1);
		memcpy(extension_string, dot_pos_of_file_path + 1, extention_length);
	}

	EM_DEBUG_LOG_SEC("*file_name_string [%s] pos_on_string [%d]", file_name_string, pos_on_string);

	*output_file_name = EM_SAFE_STRDUP(file_name_string);
	*output_extension = EM_SAFE_STRDUP(extension_string);

FINISH_OFF:
	EM_DEBUG_FUNC_END("err = [%d]", err);
	return err;
}

INTERNAL_FUNC char *em_get_extension_from_file_path(char *source_file_path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("source_file_path[%s]", source_file_path);
	int err = EMAIL_ERROR_NONE, pos_on_string = 0;
	char *extension = NULL;

	if (!source_file_path) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err  = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	pos_on_string = EM_SAFE_STRLEN(source_file_path) - 1;

	while(pos_on_string > 0 && source_file_path[pos_on_string--] != '.') ;

	if(pos_on_string > 0)
		extension = source_file_path + pos_on_string + 2;

	EM_DEBUG_LOG("*extension [%s] pos_on_string [%d]", extension, pos_on_string);

FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return extension;
}

INTERNAL_FUNC int em_get_encoding_type_from_file_path(const char *input_file_path, char **output_encoding_type)
{
	EM_DEBUG_FUNC_BEGIN("input_file_path[%d], output_encoding_type[%p]", input_file_path, output_encoding_type);
	int   err = EMAIL_ERROR_NONE;
	int   pos_of_filename = 0;
	int   pos_of_dot = 0;
	int   enf_of_string = 0;
	int   result_string_length = 0;
	char *filename = NULL;
	char *result_encoding_type = NULL;

	if (!input_file_path || !output_encoding_type) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err  = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	enf_of_string = pos_of_filename = EM_SAFE_STRLEN(input_file_path);

	while(pos_of_filename >= 0 && input_file_path[pos_of_filename--] != '/') {
		if(input_file_path[pos_of_filename] == '.')
			pos_of_dot = pos_of_filename;
	}

	if(pos_of_filename != 0)
		pos_of_filename += 2;

	filename = (char*)input_file_path + pos_of_filename;

	if(pos_of_dot != 0 && pos_of_dot > pos_of_filename)
		result_string_length = pos_of_dot - pos_of_filename;
	else
		result_string_length = enf_of_string - pos_of_filename;

	EM_DEBUG_LOG("pos_of_dot [%d], pos_of_filename [%d], enf_of_string[%d],result_string_length [%d]", pos_of_dot, pos_of_filename, enf_of_string, result_string_length);

	if( !(result_encoding_type = 	em_malloc(sizeof(char) * (result_string_length + 1))) ) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		err  = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memcpy(result_encoding_type, input_file_path + pos_of_filename, result_string_length);

	EM_DEBUG_LOG("*result_encoding_type [%s]", result_encoding_type);

	*output_encoding_type = result_encoding_type;

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int em_get_content_type_from_extension_string(const char *extension_string, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("extension_string[%s]", extension_string);
	int i = 0, err = EMAIL_ERROR_NONE, result_content_type = TYPEAPPLICATION;
	char *image_extension[] = { "jpeg", "jpg", "png", "gif", "bmp", "pic", "agif", "tif", "wbmp" , "p7s", "p7m", NULL};

	if (!extension_string) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err  = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	while(image_extension[i]) {
		EM_DEBUG_LOG("image_extension[%d] [%s]", i, image_extension[i]);
		if(strcasecmp(image_extension[i], extension_string) == 0) {
			break;
		}
		i++;
	}

	switch (i) {
	case EXTENSION_JPEG:
	case EXTENSION_JPG:
	case EXTENSION_PNG:
	case EXTENSION_GIF:
	case EXTENSION_BMP:
	case EXTENSION_PIC:
	case EXTENSION_AGIF:
	case EXTENSION_TIF:
	case EXTENSION_WBMP:
		result_content_type = TYPEIMAGE;
		break;
	case EXTENSION_P7S:
		result_content_type = TYPEPKCS7_SIGN;
		break;
	case EXTENSION_P7M:
		result_content_type = TYPEPKCS7_MIME;
		break;
	default:
		break;
	}

FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return result_content_type;
}

#define EMAIL_ACCOUNT_RGEX                     "([a-z0-9!#$%&'*+/=?^_`{|}~-]+.)*[a-z0-9!#$%&'*+/=?^_`{|}~-]+"
#define EMAIL_DOMAIN_RGEX                      "([a-z0-9!#$%&'*+/=?^_`{|}~-]+.)+[a-z0-9!#$%&'*+/=?^_`{|}~-]+"

#define EMAIL_ADDR_RGEX                        "[[:space:]]*<"EMAIL_ACCOUNT_RGEX"@"EMAIL_DOMAIN_RGEX">[[:space:]]*"
#define EMAIL_ALIAS_RGEX                       "([[:space:]]*\"[^\"]*\")?"EMAIL_ADDR_RGEX
#define EMAIL_ALIAS_LIST_RGEX                  "^("EMAIL_ALIAS_RGEX"[;,])*"EMAIL_ALIAS_RGEX"[;,]?[[:space:]]*$"

#define EMAIL_ADDR_WITHOUT_BRACKET_RGEX        "[[:space:]]*"EMAIL_ACCOUNT_RGEX"@"EMAIL_DOMAIN_RGEX"[[:space:]]*"
#define EMAIL_ALIAS_WITHOUT_BRACKET_RGEX       "([[:space:]]*\"[^\"]*\")?"EMAIL_ADDR_WITHOUT_BRACKET_RGEX
#define EMAIL_ALIAS_LIST_WITHOUT_BRACKET_RGEX  "("EMAIL_ALIAS_WITHOUT_BRACKET_RGEX"[;,])*"EMAIL_ADDR_WITHOUT_BRACKET_RGEX"[;,]?[[:space:]]*$"

INTERNAL_FUNC int em_verify_email_address(char *address, int without_bracket)
{
	EM_DEBUG_FUNC_BEGIN_SEC("address[%s] without_bracket[%d]", address, without_bracket);

	/*  this following code verfies the email alias string using reg. exp. */
	regex_t alias_list_regex = {0};
	int error = EMAIL_ERROR_NONE;
	char *reg_rule = NULL;
	int alias_len = 0;
	regmatch_t *pmatch = NULL;

	if(!address || EM_SAFE_STRLEN(address) == 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if(without_bracket)
		reg_rule = EMAIL_ALIAS_LIST_WITHOUT_BRACKET_RGEX;
	else
		reg_rule = EMAIL_ALIAS_LIST_RGEX;

	if (regcomp (&alias_list_regex, reg_rule, REG_ICASE | REG_EXTENDED) != 0) {
		EM_DEBUG_EXCEPTION("email alias regex unrecognized");
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	alias_len = EM_SAFE_STRLEN(address) + 1;
	pmatch = (regmatch_t *) em_malloc (alias_len * sizeof (regmatch_t));
	if (!pmatch) {
		EM_DEBUG_EXCEPTION("em_malloc error");
		goto FINISH_OFF;
	}

	if (regexec (&alias_list_regex, address, alias_len, pmatch, 0) == REG_NOMATCH) {
		EM_DEBUG_LOG_SEC("failed :[%s]", address);
		error = EMAIL_ERROR_INVALID_ADDRESS;
		goto FINISH_OFF;
	}

FINISH_OFF:
	regfree(&alias_list_regex);
	EM_SAFE_FREE (pmatch);

	EM_DEBUG_FUNC_END("err [%d]", error);
	return error;
}

INTERNAL_FUNC int em_verify_email_address_of_mail_data (email_mail_data_t *mail_data, int without_bracket)
{
	EM_DEBUG_FUNC_BEGIN("mail_data[%p] without_bracket[%d]", mail_data, without_bracket);
	char *address_array[4] = { mail_data->full_address_from, mail_data->full_address_to, mail_data->full_address_cc, mail_data->full_address_bcc};
	int  err = EMAIL_ERROR_NONE, i;

	/* check for email_address validation */
	for (i = 0; i < 4; i++) {
		if (address_array[i] && address_array[i][0] != 0) {
			err = em_verify_email_address (address_array[i] , without_bracket);
			if (err != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION_SEC("em_verify_email_address error[%d] idx[%d] addr[%s]", err, i, address_array[i]);
				goto FINISH_OFF;
			}
		}
	}
FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int em_verify_email_address_of_mail_tbl(emstorage_mail_tbl_t *input_mail_tbl, int input_without_bracket)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_tbl[%p] input_without_bracket[%d]", input_mail_tbl, input_without_bracket);
	char *address_array[4] = { input_mail_tbl->full_address_to, input_mail_tbl->full_address_cc, input_mail_tbl->full_address_bcc, input_mail_tbl->full_address_from};
	int  err = EMAIL_ERROR_NONE, i;

	/* check for email_address validation */
	for (i = 0; i < 4; i++) {
		if (address_array[i] && address_array[i][0] != 0) {
			if ((err = em_verify_email_address (address_array[i] , input_without_bracket)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION_SEC("em_verify_email_address error[%d] idx[%d] addr[%s]", err, i, address_array[i]);
				goto FINISH_OFF;
			}
		}
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int em_find_tag_for_thread_view(char *subject, int *result)
{
	EM_DEBUG_FUNC_BEGIN();
	int error_code = EMAIL_ERROR_NONE;
	char *copy_of_subject = NULL;

	EM_IF_NULL_RETURN_VALUE(subject, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(result, EMAIL_ERROR_INVALID_PARAM);

	*result = FALSE;

	copy_of_subject = EM_SAFE_STRDUP(subject);

	if (copy_of_subject == NULL) {
		EM_DEBUG_EXCEPTION("strdup is failed.");
		goto FINISH_OFF;
	}

	em_upper_string(copy_of_subject);
	EM_DEBUG_LOG("em_upper_string result : %s\n", copy_of_subject);

	if (strstr(copy_of_subject, "RE:") == NULL) {
		if (strstr(copy_of_subject, "FWD:") == NULL) {
			if (strstr(copy_of_subject, "FW:") != NULL)
				*result = TRUE;
		}
		else
			*result = TRUE;
	}
	else
		*result = TRUE;

FINISH_OFF:
	EM_SAFE_FREE(copy_of_subject);

	EM_DEBUG_FUNC_END("result : %d", *result);

	return error_code;
}

INTERNAL_FUNC int em_find_pos_stripped_subject_for_thread_view(char *subject, char *stripped_subject, int stripped_subject_buffer_size)
{
	EM_DEBUG_FUNC_BEGIN("subject [%p] stripped_subject [%p] stripped_subject_buffer_size[%d]", subject, stripped_subject, stripped_subject_buffer_size);
	int error_code = EMAIL_ERROR_NONE;
	int gap;
	char *copy_of_subject = NULL, *curpos = NULL, *result;

	EM_IF_NULL_RETURN_VALUE(subject, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(stripped_subject, EMAIL_ERROR_INVALID_PARAM);

	copy_of_subject = EM_SAFE_STRDUP(subject);

	if (copy_of_subject == NULL) {
		EM_DEBUG_EXCEPTION("strdup is failed");
		goto FINISH_OFF;
	}

	em_upper_string(copy_of_subject);
	curpos = copy_of_subject;

	EM_DEBUG_LOG("em_upper_string result : %s", copy_of_subject);

	while ((result = strstr(curpos, "RE:")) != NULL) {
		curpos = result + 3;
		EM_DEBUG_LOG("RE result : %s", curpos);
	}

	while ((result = strstr(curpos, "FWD:")) != NULL) {
		curpos = result + 4;
		EM_DEBUG_LOG("FWD result : %s", curpos);
	}

	while ((result = strstr(curpos, "FW:")) != NULL) {
		curpos = result + 3;
		EM_DEBUG_LOG("FW result : %s", curpos);
	}

	while (curpos != NULL && *curpos == ' ') {
		curpos++;
	}

	gap = curpos - copy_of_subject;

	EM_SAFE_STRNCPY(stripped_subject, subject + gap, stripped_subject_buffer_size);

FINISH_OFF:
	EM_SAFE_FREE(copy_of_subject);

	if (error_code == EMAIL_ERROR_NONE && stripped_subject)
		EM_DEBUG_LOG("result[%s]", stripped_subject);

	EM_DEBUG_FUNC_END("error_code[%d]", error_code);
	return error_code;
}


/*
 * encoding base64
 */
INTERNAL_FUNC int em_encode_base64(char *src, unsigned long src_len, char **enc, unsigned long* enc_len, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

    unsigned char *content;
    int ret = true, err = EMAIL_ERROR_NONE;

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

	EM_DEBUG_FUNC_END();
    return ret;
}

/*
 * decoding base64
 */
INTERNAL_FUNC int em_decode_base64(unsigned char *enc_text, unsigned long enc_len, char **dec_text, unsigned long* dec_len, int *err_code)
{
    unsigned char *text = enc_text;
    unsigned long size = enc_len;
    unsigned char *content;
    int ret = true, err = EMAIL_ERROR_NONE;

	if (err_code != NULL) {
		*err_code = EMAIL_ERROR_NONE;
	}

    EM_DEBUG_FUNC_BEGIN();

    content = rfc822_base64(text, size, dec_len);
    if (content)
        *dec_text = (char *)content;
    else
    {
        err = EMAIL_ERROR_UNKNOWN;
        ret = false;
    }

    if (err_code)
        *err_code = err;

    return ret;
}

INTERNAL_FUNC int em_get_account_server_type_by_account_id(int account_id, email_account_server_t* account_server_type, int flag, int *error)
{
	EM_DEBUG_FUNC_BEGIN();
	emstorage_account_tbl_t *account_tbl_data = NULL;
	int ret = false;
	int err= EMAIL_ERROR_NONE;

	if (account_server_type == NULL ) {
		EM_DEBUG_EXCEPTION("account_server_type is NULL");
		err = EMAIL_ERROR_INVALID_PARAM;
		ret = false;
		goto FINISH_OFF;
	}

	if( !emstorage_get_account_by_id(account_id, WITHOUT_OPTION, &account_tbl_data, false, &err)) {
		EM_DEBUG_EXCEPTION ("emstorage_get_account_by_id failed [%d] ", err);
		ret = false;
		goto FINISH_OFF;
	}

	if ( flag == false )  {	/*  sending server */
		*account_server_type = account_tbl_data->outgoing_server_type;
	} else if ( flag == true ) {	/*  receiving server */
		*account_server_type = account_tbl_data->incoming_server_type;
	}

	ret = true;

FINISH_OFF:
	if ( account_tbl_data != NULL ) {
		emstorage_free_account(&account_tbl_data, 1, NULL);
	}
	if ( error != NULL ) {
		*error = err;
	}

	return ret;
}

#include <vconf.h>
#include <dbus/dbus.h>

#define ACTIVE_SYNC_HANDLE_INIT_VALUE		(-1)
#define ACTIVE_SYNC_HANDLE_BOUNDARY			(-100000000)


INTERNAL_FUNC int em_get_handle_for_activesync(int *handle, int *error)
{
	EM_DEBUG_FUNC_BEGIN();

	static int next_handle = 0;
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if ( handle == NULL ) {
		EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed : handle is NULL");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ( vconf_get_int(VCONFKEY_EMAIL_SERVICE_ACTIVE_SYNC_HANDLE, &next_handle)  != 0 ) {
		EM_DEBUG_EXCEPTION("vconf_get_int failed");
		if ( next_handle != 0 ) {
			err = EMAIL_ERROR_GCONF_FAILURE;
			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG(">>>>>> VCONFKEY_EMAIL_SERVICE_ACTIVE_SYNC_HANDLE : get lastest handle[%d]", next_handle);

	/*  set the value of the handle for active sync */
	next_handle--;
	if ( next_handle < ACTIVE_SYNC_HANDLE_BOUNDARY ) {
		next_handle = ACTIVE_SYNC_HANDLE_INIT_VALUE;
	}
	if ( vconf_set_int(VCONFKEY_EMAIL_SERVICE_ACTIVE_SYNC_HANDLE, next_handle) != 0) {
		EM_DEBUG_EXCEPTION("vconf_set_int failed");
		err = EMAIL_ERROR_GCONF_FAILURE;
		goto FINISH_OFF;
	}
	ret = true;
	*handle = next_handle;
	EM_DEBUG_LOG(">>>>>> return next handle[%d]", *handle);

FINISH_OFF:
	if ( error != NULL ) {
		*error = err;
	}

	return ret;
}

INTERNAL_FUNC int em_check_socket_privilege_by_pid(int pid)
{
	EM_DEBUG_FUNC_BEGIN("pid [%d]", pid);
	int smack_ret = 0;

	// smack_ret = security_server_check_privilege_by_pid(pid, "email-service::write", "rw");
	if (smack_ret == SECURITY_SERVER_API_ERROR_ACCESS_DENIED) {
		EM_DEBUG_EXCEPTION("SECURITY_SERVER_API_ERROR_ACCESS_DENIED");
		return EMAIL_ERROR_PERMISSION_DENIED;
	}

	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

INTERNAL_FUNC int em_check_db_privilege_by_pid(int pid)
{
	EM_DEBUG_FUNC_BEGIN("pid [%d]", pid);
	int smack_ret = 0;

	// smack_ret = security_server_check_privilege_by_pid(pid, "email-service::db", "rw");
	if (smack_ret == SECURITY_SERVER_API_ERROR_ACCESS_DENIED) {
		EM_DEBUG_EXCEPTION("SECURITY_SERVER_API_ERROR_ACCESS_DENIED");
		return EMAIL_ERROR_PERMISSION_DENIED;
	}

	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}


/* thread with task queue generic functions */

pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;

email_thread_handle_t* em_thread_create (void *(*thread_exit)(void*), void *arg)
{
	pthread_mutex_lock(&g_mu);

	email_thread_handle_t* thd_handle = (email_thread_handle_t*) calloc (1,sizeof (email_thread_handle_t));
	if (!thd_handle) {
		EM_DEBUG_EXCEPTION ("out of memory");
		goto FINISH_OFF;
	}

	thd_handle->q = g_queue_new ();
	if (!thd_handle->q) {
		EM_DEBUG_EXCEPTION ("g_queue_new failed");
		goto FINISH_OFF;
	}
	g_queue_init (thd_handle->q);

	int ret = pthread_mutex_init (&thd_handle->mu, NULL);
	if (ret == -1) {
		EM_DEBUG_EXCEPTION ("pthread_mutex_init failed [%d]", errno);
		goto FINISH_OFF;
	}

	ret = pthread_cond_init (&thd_handle->cond, NULL);
	if (ret == -1) {
		EM_DEBUG_EXCEPTION ("pthread_cond_init failed [%d]", errno);
		goto FINISH_OFF;
	}

	thd_handle->thread_exit     = thread_exit;
	thd_handle->thread_exit_arg = arg;

	pthread_mutex_unlock (&g_mu);
	return thd_handle;

FINISH_OFF:
	pthread_mutex_unlock (&g_mu);
	em_thread_destroy (thd_handle);
	return NULL;
}

void em_thread_destroy (email_thread_handle_t* thd_handle)
{
	if (!thd_handle) return;

	if (!g_queue_is_empty (thd_handle->q)) {
		EM_DEBUG_EXCEPTION ("delete q item routine here");
	}

	pthread_mutex_destroy (&thd_handle->mu);
	pthread_cond_destroy (&thd_handle->cond);

	EM_SAFE_FREE (thd_handle);
}

typedef struct {
	void *(*thread_func)(void*);  	/* thread main function */
	void *(*destroy)(void*);		/* destroyer function */
	void *arg;
	email_thread_handle_t *thd_handle;
} worker_handle_t;

static void* worker_func (void* arg)
{
	if (!arg) {
		EM_DEBUG_EXCEPTION ("PARAMETER NULL");
		return NULL;
	}
	/* first task is passed by arg */
	worker_handle_t* warg = (worker_handle_t*) arg;
	email_thread_handle_t *thd_handle = warg->thd_handle;

	if (!thd_handle) {
		EM_DEBUG_EXCEPTION ("PARAMETER NULL");
		return NULL;
	}

	/* consume task in queue and exit */
	do {
		/* running thread main function */
		if (warg->thread_func)
			(warg->thread_func) (warg->arg); 
		if (warg->destroy)
			(warg->destroy) (warg->arg);
		EM_SAFE_FREE (warg);

		/* if there is a pending job */
		pthread_mutex_lock (&thd_handle->mu);
		warg = g_queue_pop_head (thd_handle->q);
		if (!warg) {
			thd_handle->running = 0;
			pthread_mutex_unlock (&thd_handle->mu);
			(thd_handle->thread_exit) (thd_handle->thread_exit_arg);
			return NULL;
		}
		pthread_mutex_unlock (&thd_handle->mu);
	} while (1);

	return NULL;
}

void em_thread_run (email_thread_handle_t *thd_handle, void *(*thread_func)(void*), void *(*destroy)(void*), void* arg)
{
	if(!thd_handle || !thread_func) {
		EM_DEBUG_EXCEPTION ("invalid param");
		return;
	}

	worker_handle_t *worker_handle = (worker_handle_t*) calloc (1, sizeof (worker_handle_t));
	if (!worker_handle) {
		EM_DEBUG_EXCEPTION ("out of memory");
		return;
	}
	worker_handle->thread_func = thread_func;
	worker_handle->arg  = arg;
	worker_handle->thd_handle = thd_handle;

	pthread_mutex_lock (&thd_handle->mu);

	/* adding task to queue */
	if (thd_handle->running) {
		g_queue_push_tail (thd_handle->q, worker_handle);		
	}
	else {
		thd_handle->running = 1;
		pthread_t tid = 0;
		int err = pthread_create (&tid, NULL, worker_func, worker_handle);
		if (err < 0 )
			EM_DEBUG_EXCEPTION ("pthread_create failed [%d]", errno);
		thd_handle->tid = tid;
	}
	pthread_mutex_unlock (&thd_handle->mu);
}

void em_thread_join (email_thread_handle_t *thd_handle)
{
	int err = pthread_join (thd_handle->tid, NULL);
	if (err < 0) {
		EM_DEBUG_EXCEPTION ("pthread_join failed [%d]", errno);
	}
}


