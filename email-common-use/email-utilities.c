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

#include "c-client.h"

#include "email-types.h"
#include "email-internal-types.h"
#include "email-utilities.h"

INTERNAL_FUNC void* em_malloc(unsigned len)
{
	/* EM_DEBUG_LOG("Memory allocation size[%d] bytes", len); */
	void *p = NULL;

	if (len <= 0) {
		EM_DEBUG_EXCEPTION("len should be positive.[%d]", len);
		return NULL;
	}

	p = malloc(len);

	if (p)
		memset(p, 0x00, len);
	else
		EM_DEBUG_EXCEPTION("malloc failed");
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

	strncpy(str, temp_buffer, strlen(str));
	str[strlen(temp_buffer)] = NULL_CHAR;

	EM_SAFE_FREE(temp_buffer);

	return str;
}

/*  remove right space, tab, CR, L */
INTERNAL_FUNC char *em_trim_right(char *str)
{
	char *p;

	/* EM_DEBUG_FUNC_BEGIN() */
	if (!str) return NULL;

	p = str+strlen(str)-1;
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
	int i = 0, is_utf7 = 0, len = path ? (int)strlen(path) : -1;
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
	char ptr[strlen(str)+1]  ;
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

	buffer_length	= strlen(source_string) + 1024;
	result_buffer  = (char *)em_malloc(buffer_length);

	if (!result_buffer) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		return NULL;
	}

	strncpy(result_buffer, source_string, p - source_string);
	snprintf(result_buffer + strlen(result_buffer), buffer_length - strlen(result_buffer), "%s%s", new_string, p + strlen(old_string));

	EM_DEBUG_FUNC_END("result_buffer[%s]", result_buffer);
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

#define DATE_TIME_STRING_LEGNTH 14

INTERNAL_FUNC char *em_get_extension_from_file_path(char *source_file_path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("source_file_path[%s]", source_file_path);
	int err = EMF_ERROR_NONE, pos_on_string = 0;
	char *extension = NULL;

	if (!source_file_path) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err  = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	pos_on_string = strlen(source_file_path) - 1;

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
	int   err = EMF_ERROR_NONE;
	int   pos_of_filename = 0;
	int   pos_of_dot = 0;
	int   enf_of_string = 0;
	int   result_string_length = 0;
	char *filename = NULL;
	char *result_encoding_type = NULL;

	if (!input_file_path || !output_encoding_type) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err  = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	enf_of_string = pos_of_filename = strlen(input_file_path) - 1;

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
		EM_DEBUG_EXCEPTION("EMF_ERROR_OUT_OF_MEMORY");
		err  = EMF_ERROR_OUT_OF_MEMORY;
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
	int i = 0, err = EMF_ERROR_NONE, result_content_type = TYPEAPPLICATION;
	char *image_extension[] = { "jpeg", "jpg", "png", "gif", "bmp", "pic", "agif", "tif", "wbmp" , NULL};

	if (!extension_string) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err  = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	while(image_extension[i]) {
		EM_DEBUG_LOG("image_extension[%d] [%s]", i, image_extension[i]);
		if(strcasecmp(image_extension[i], extension_string) == 0) {
			result_content_type = TYPEIMAGE;
			break;
		}
		i++;
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

INTERNAL_FUNC int em_verify_email_address(char *address, int without_bracket, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("address[%s] without_bracket[%d]", address, without_bracket);

	/*  this following code verfies the email alias string using reg. exp. */
	regex_t alias_list_regex = {0};
	int ret = false, error = EMF_ERROR_NONE;
	char *reg_rule = NULL;

	if(!address || strlen(address) == 0) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		if (err_code)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}

	if(without_bracket)
		reg_rule = EMAIL_ALIAS_LIST_WITHOUT_BRACKET_RGEX;
	else
		reg_rule = EMAIL_ALIAS_LIST_RGEX;

	if (regcomp(&alias_list_regex, reg_rule, REG_ICASE | REG_EXTENDED)) {
		EM_DEBUG_EXCEPTION("email alias regex unrecognized");
		if (err_code)
			*err_code = EMF_ERROR_UNKNOWN;
		return false;
	}

	int alias_len = strlen(address) + 1;
	regmatch_t pmatch[alias_len];

	bzero(pmatch, alias_len);

	if (regexec(&alias_list_regex, address, alias_len, pmatch, 0) == REG_NOMATCH)
		EM_DEBUG_LOG("failed :[%s]", address);
	else {
		EM_DEBUG_LOG("success :[%s]", address);
		ret = true;
	}

	regfree(&alias_list_regex);

	if (err_code)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int em_verify_email_address_of_mail_data(emf_mail_data_t *mail_data, int without_bracket, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_data[%p] without_bracket[%d]", mail_data, without_bracket);
	char *address_array[4] = { mail_data->full_address_from, mail_data->full_address_to, mail_data->full_address_cc, mail_data->full_address_bcc};
	int  ret = false, err = EMF_ERROR_NONE, i;

	/* check for email_address validation */
	for (i = 0; i < 4; i++) {
		if (address_array[i] && address_array[i][0] != 0) {
			if (!em_verify_email_address(address_array[i] , without_bracket, &err)) {
				err = EMF_ERROR_INVALID_ADDRESS;
				EM_DEBUG_EXCEPTION("Invalid Email Address [%d][%s]", i, address_array[i]);
				goto FINISH_OFF;
			}
		}
	}
	ret = true;
FINISH_OFF:
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int em_verify_email_address_of_mail_tbl(emstorage_mail_tbl_t *input_mail_tbl, int input_without_bracket)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_tbl[%p] input_without_bracket[%d]", input_mail_tbl, input_without_bracket);
	char *address_array[4] = { input_mail_tbl->full_address_to, input_mail_tbl->full_address_cc, input_mail_tbl->full_address_bcc, input_mail_tbl->full_address_from};
	int  err = EMF_ERROR_NONE, i;

	/* check for email_address validation */
	for (i = 0; i < 4; i++) {
		if (address_array[i] && address_array[i][0] != 0) {
			if (!em_verify_email_address(address_array[i] , input_without_bracket, &err)) {
				err = EMF_ERROR_INVALID_ADDRESS;
				EM_DEBUG_EXCEPTION("Invalid Email Address [%d][%s]", i, address_array[i]);
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
	int error_code = EMF_ERROR_NONE;
	char *copy_of_subject = NULL;

	EM_IF_NULL_RETURN_VALUE(subject, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(result, EMF_ERROR_INVALID_PARAM);

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

INTERNAL_FUNC int em_find_pos_stripped_subject_for_thread_view(char *subject, char *stripped_subject)
{
	EM_DEBUG_FUNC_BEGIN();
	int error_code = EMF_ERROR_NONE;
	int gap;
	char *copy_of_subject = NULL, *curpos = NULL, *result;

	EM_IF_NULL_RETURN_VALUE(subject, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(stripped_subject, EMF_ERROR_INVALID_PARAM);

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

	strcpy(stripped_subject, subject + gap);

FINISH_OFF:
	EM_SAFE_FREE(copy_of_subject);

	if (error_code == EMF_ERROR_NONE && stripped_subject)
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
    int ret = true, err = EMF_ERROR_NONE;

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
    int ret = true, err = EMF_ERROR_NONE;

	if (err_code != NULL) {
		*err_code = EMF_ERROR_NONE;
	}

    EM_DEBUG_FUNC_BEGIN();

    content = rfc822_base64(text, size, dec_len);
    if (content)
        *dec_text = (char *)content;
    else
    {
        err = EMF_ERROR_UNKNOWN;
        ret = false;
    }

    if (err_code)
        *err_code = err;

    return ret;
}
