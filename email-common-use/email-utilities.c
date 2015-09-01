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
#include <locale.h>

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

INTERNAL_FUNC void em_skip_whitespace_without_alias(char *addr_str, char **pAddr)
{
	EM_DEBUG_FUNC_BEGIN("addr_str[%p]", addr_str);

	if (!addr_str)
		return ;
	char *str = addr_str;
	char ptr[EM_SAFE_STRLEN(addr_str) + 1];
	int i = 0, j = 0;
	char *first_qu = NULL;
	char *last_qu = NULL;
	char *first_c = NULL;
	char *last_c = NULL;

	// find first and last quatation
	for (i = 0; str[i] != '\0'; i++) {
		if (!first_qu && str[i] == '\"')
			first_qu = str + i;
		if (str[i] == '\"')
			last_qu = str + i;
	}

	if (!first_qu || !last_qu) {
		// if there is no qutation
		for (i = 0; str[i] != NULL_CHAR ; i++) {
			if (str[i] != SPACE && str[i] != TAB && str[i] != CR && str[i] != LF)
				ptr[j++] = str[i];
		}
	} else {
		// find first and last character except for space
		for (first_c = first_qu + 1; *first_c == ' '; first_c++);
		for (last_c = last_qu - 1; *last_c == ' '; last_c--);
		for (i = 0; str[i] != '\0'; i++) {
			if (str + i <= first_qu || str + i >= last_qu) {
				if (str[i] != SPACE && str[i] != TAB && str[i] != CR && str[i] != LF)
					ptr[j++] = str[i];
			} else if (first_qu < str + i && str + i < last_qu) {
				if (str + i < first_c || str + i > last_c)
					continue;
				else
					ptr[j++] = str[i];
			}
		}
	}
	ptr[j++] = NULL_CHAR;

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

INTERNAL_FUNC int em_replace_string_ex(char **input_source_string, char *input_old_string, char *input_new_string)
{
	EM_DEBUG_FUNC_BEGIN();
	int   err = 0;
	int   buffer_length = 0;
	int   old_string_length = 0;
	int   new_string_length = 0;
	int   match_count = 0;

	char *cursor_of_source_string = NULL;
	char *cursor_of_result_buffer = NULL;
	char *result_buffer = NULL;
	char *source_string = NULL;
	char *found_pos = NULL;

	EM_IF_NULL_RETURN_VALUE(input_source_string, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_old_string, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_new_string, EMAIL_ERROR_INVALID_PARAM);

	source_string = *input_source_string;

	found_pos = strstr(source_string, input_old_string);

	if (found_pos == NULL) {
		err = EMAIL_ERROR_DATA_NOT_FOUND;
		goto FINISH_OFF;
	}

	old_string_length = EM_SAFE_STRLEN(input_old_string);
	new_string_length = EM_SAFE_STRLEN(input_new_string);

	while (found_pos) {
		match_count++;
		found_pos++;
		found_pos = strstr(found_pos, input_old_string);
	}

	buffer_length  = EM_SAFE_STRLEN(source_string) + ((new_string_length - old_string_length) * match_count) + 50;

	result_buffer  = (char*)malloc(buffer_length);

	if (!result_buffer) {
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memset(result_buffer, 0 , buffer_length);

	cursor_of_source_string = source_string;
	cursor_of_result_buffer = result_buffer;
	found_pos = strstr(source_string, input_old_string);

	while (found_pos) {
		memcpy(cursor_of_result_buffer, cursor_of_source_string, found_pos - cursor_of_source_string);

		cursor_of_result_buffer = result_buffer + EM_SAFE_STRLEN(result_buffer);
		cursor_of_source_string = found_pos + old_string_length;

		memcpy(cursor_of_result_buffer, input_new_string, new_string_length);

		cursor_of_result_buffer = result_buffer + EM_SAFE_STRLEN(result_buffer);

		found_pos++;
		found_pos = strstr(found_pos, input_old_string);
	}

	EM_SAFE_STRCAT(result_buffer, cursor_of_source_string);

	EM_SAFE_FREE(*input_source_string);
	*input_source_string = result_buffer;

FINISH_OFF:

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
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

	if (!input_source_file_path || !output_file_name || !output_extension || EM_SAFE_STRLEN(input_source_file_path) <= 0) {
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

	while(pos_on_string >= 0 && source_file_path[pos_on_string] != '.') {
		pos_on_string--;
	}

	if (pos_on_string >= 0 && pos_on_string < EM_SAFE_STRLEN(source_file_path) - 1)
		extension = source_file_path + pos_on_string + 1;

	EM_DEBUG_LOG("*extension [%s] pos_on_string [%d]", extension, pos_on_string);

FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return extension;
}

INTERNAL_FUNC char *em_shrink_filename(char *fname, int size_limit)
{
	EM_DEBUG_FUNC_BEGIN("fname[%s], size_limit[%d]", fname, size_limit);

	char *modified_name = NULL;
	char *extension = NULL;

	modified_name = em_malloc(sizeof(char)*size_limit);
	if (!modified_name) {
		return NULL;
	}

	char *tmp_ext = NULL;
	char *tmp_name = NULL;
	char *tmp_name_strip = NULL;

	extension = em_get_extension_from_file_path(fname, NULL);

	if (extension && EM_SAFE_STRLEN(extension) > 0) {
		int ext_len = EM_SAFE_STRLEN(extension);
		int name_len = EM_SAFE_STRLEN(fname) - EM_SAFE_STRLEN(extension) - 1;
		int name_strip_len = size_limit - EM_SAFE_STRLEN(extension) - 2;

		tmp_ext = em_malloc(sizeof(char)*(ext_len+1));
		if (tmp_ext == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			goto FINISH_OFF;
		}

		tmp_name = em_malloc(sizeof(char)*(name_len+1));
		if (tmp_name == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			goto FINISH_OFF;
		}

		tmp_name_strip = em_malloc(sizeof(char)*name_strip_len);
		if (tmp_name_strip == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			goto FINISH_OFF;
		}

		snprintf(tmp_ext, sizeof(char)*(ext_len+1), "%s", extension);
		snprintf(tmp_name, sizeof(char)*(name_len+1), "%s", fname);

		EM_DEBUG_LOG(">>>>> extention [%s]", tmp_ext);
		EM_DEBUG_LOG(">>>>> name [%s]", tmp_name);

		if (EM_SAFE_STRLEN(extension) > EM_SAFE_STRLEN(fname) - EM_SAFE_STRLEN(extension)) {
			snprintf(modified_name, sizeof(char)*size_limit, "%s", fname);
		} else {
			if (tmp_name_strip && name_strip_len > 1) {
				snprintf(tmp_name_strip, sizeof(char)*name_strip_len, "%s", tmp_name);
				snprintf(modified_name, sizeof(char)*size_limit, "%s.%s", tmp_name_strip, tmp_ext);
			} else {
				snprintf(modified_name, sizeof(char)*size_limit, "%s", fname);
			}
		}
	} else {
		snprintf(modified_name, sizeof(char)*size_limit, "%s", fname);
	}

FINISH_OFF:

	EM_SAFE_FREE(tmp_ext);
	EM_SAFE_FREE(tmp_name);
	EM_SAFE_FREE(tmp_name_strip);

	EM_DEBUG_FUNC_END();

	return modified_name;
}

INTERNAL_FUNC int em_get_encoding_type_from_file_path(const char *input_file_path, char **output_encoding_type)
{
	EM_DEBUG_FUNC_BEGIN("input_file_path[%d], output_encoding_type[%p]", input_file_path, output_encoding_type);
	int   err = EMAIL_ERROR_NONE;
	int   pos_of_filename = 0;
	int   pos_of_dot = 0;
	int   enf_of_string = 0;
	int   result_string_length = 0;
	char *result_encoding_type = NULL;

	if (!input_file_path || !output_encoding_type) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err  = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	enf_of_string = pos_of_filename = EM_SAFE_STRLEN(input_file_path);

	while(pos_of_filename > 0 && input_file_path[pos_of_filename--] != '/') {
		if(input_file_path[pos_of_filename] == '.')
			pos_of_dot = pos_of_filename;
	}

	if(pos_of_filename != 0)
		pos_of_filename += 2;

	if(pos_of_dot != 0 && pos_of_dot > pos_of_filename)
		result_string_length = pos_of_dot - pos_of_filename;
	else
		result_string_length = enf_of_string - pos_of_filename;

	if( !(result_encoding_type = em_malloc(sizeof(char) * (result_string_length + 1))) ) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		err  = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memcpy(result_encoding_type, input_file_path + pos_of_filename, result_string_length);

	EM_DEBUG_LOG("result_encoding_type [%s]", result_encoding_type);

	*output_encoding_type = result_encoding_type;

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int em_get_content_type_from_extension_string(const char *extension_string, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("extension_string[%s]", extension_string);
	int i = 0, err = EMAIL_ERROR_NONE, result_content_type = TYPEAPPLICATION;
	char *image_extension[] = { "jpeg", "jpg", "png", "gif", "bmp", "pic", "agif", "tif", "wbmp" , "p7s", "p7m", "asc", NULL};

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
	case EXTENSION_ASC:
		result_content_type = TYPEPGP;
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

#define EMAIL_ATOM                             "([^]()<>@,;:\\\".[\x20\x01-\x1f\x7f])+"  // x20: space,
#define EMAIL_QTEXT                            "[^\"\\\x0d]" /* " \ CR */
#define EMAIL_DTEXT                            "[^][\\\x0d]" /* [ ] \ CR */
#define EMAIL_QUOTED_PAIR                      "([\\].)" // first char :\ second char : anything (.)
#define EMAIL_QUOTED_STRING                    "[\"](" EMAIL_QTEXT "|" EMAIL_QUOTED_PAIR ")*[\"]"
#define EMAIL_WORD                             "(" EMAIL_ATOM "|" EMAIL_QUOTED_STRING ")"
#define EMAIL_PHRASE                           "(" EMAIL_ATOM "|" EMAIL_QUOTED_STRING ")"

#define EMAIL_DOMAIN_LITERAL                   "\\[(" EMAIL_DTEXT "|" EMAIL_QUOTED_PAIR ")*\\]" /* literal match for "[" and "]"*/
#define EMAIL_SUB_DOMAIN                       "(" EMAIL_ATOM "|" EMAIL_DOMAIN_LITERAL ")"

#define EMAIL_LOCAL_PART                       "(" EMAIL_WORD "(\\." EMAIL_WORD ")*)"
#define EMAIL_DOMAIN                           "(" EMAIL_SUB_DOMAIN "(\\." EMAIL_SUB_DOMAIN ")*)"
#define EMAIL_ADDR_SPEC                        "(" EMAIL_LOCAL_PART "@" EMAIL_DOMAIN ")"

#define EMAIL_MAILBOX                          "("EMAIL_ADDR_SPEC "|" EMAIL_PHRASE "[[:space:]]*" "<" EMAIL_ADDR_SPEC ">|<" EMAIL_ADDR_SPEC ">)"
#define EMAIL_ADDRESS                          "^([:blank:]*" EMAIL_MAILBOX "[,;[:blank:]]*([,;][,;[:blank:]]*" EMAIL_MAILBOX "[,;[:blank:]]*)*)$"


static int em_verify_email_address_by_using_regex(char *address)
{
	EM_DEBUG_FUNC_BEGIN_SEC("address[%s]", address);

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

	reg_rule = EMAIL_ADDRESS;

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

static int em_verify_email_address_without_regex(char *address)
{
	EM_DEBUG_FUNC_BEGIN_SEC("address[%s]", address);
	char *local_address = NULL;
	char *address_start = NULL;
	char *cur = NULL;
	char *local_part = NULL;
	char *domain = NULL;
	char *saveptr = NULL;
	char currunt_char;
	int address_length = 0;
	int i = 0;
	int error = EMAIL_ERROR_NONE;
	int occur = 0;

	EM_DEBUG_LOG_SEC("address [%s]", address);

	if (address == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		error = EMAIL_ERROR_INVALID_PARAM;
		return error;
	}

	local_address = strdup(address);

	address_start = local_address;

	while ((cur = strchr(address_start, '\"'))) {
		address_start = cur + 1;
		if (local_address >= cur - 1 || *(cur - 1) != '\\')
			occur++;
	}

	if (occur % 2) {
		error = EMAIL_ERROR_INVALID_ADDRESS;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_ADDRESS");
		goto FINISH_OFF;
	}

	if ((cur = strchr(address_start, '<'))) {
		char *close_pos = NULL;

		address_start = cur + 1;
		close_pos = address_start;

		while ((cur = strchr(close_pos, '>'))) {
			close_pos = cur + 1;
		}

		if (address_start == close_pos) {
			error = EMAIL_ERROR_INVALID_ADDRESS;
			EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_ADDRESS");
			goto FINISH_OFF;
		}

		address_start[close_pos - 1 - address_start] = '\0';
	}

	EM_DEBUG_LOG_SEC("address_start [%s]", address_start);

	address_length = EM_SAFE_STRLEN(address_start);

	for (i = 0; i < address_length; i++) {
		currunt_char = address_start[i];
		if (!isalpha(currunt_char) && !isdigit(currunt_char) && currunt_char != '_' && currunt_char != '.' && currunt_char != '@') {
			error = EMAIL_ERROR_INVALID_ADDRESS;
			EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_ADDRESS");
			goto FINISH_OFF;
		}
	}

	if (strstr(address_start, "..") || strstr(address_start, ".@") || strstr(address_start, "@.") || strstr(address_start, "._.")) {
		error = EMAIL_ERROR_INVALID_ADDRESS;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_ADDRESS");
		goto FINISH_OFF;
	}

	local_part = strtok_r(address_start, "@", &saveptr);

	EM_DEBUG_LOG("local_part [%s]", local_part);

	if (local_part == NULL || EM_SAFE_STRLEN(local_part) == 0) {
		error = EMAIL_ERROR_INVALID_ADDRESS;
		goto FINISH_OFF;
	}

	domain = strtok_r(NULL, "@", &saveptr);

	EM_DEBUG_LOG("domain [%s]", domain);

	if (domain == NULL || EM_SAFE_STRLEN(domain) < 3) {
		error = EMAIL_ERROR_INVALID_ADDRESS;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_ADDRESS");
		goto FINISH_OFF;
	}

	if (strchr(domain, '.') == NULL) {
		error = EMAIL_ERROR_INVALID_ADDRESS;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_ADDRESS");
		goto FINISH_OFF;
	}

	if (!isalpha(local_part[0])) {
		error = EMAIL_ERROR_INVALID_ADDRESS;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_ADDRESS");
		goto FINISH_OFF;
	}

FINISH_OFF:
	EM_SAFE_FREE(local_address);

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int em_verify_email_address(char *address)
{
	EM_DEBUG_FUNC_BEGIN("address[%p]", address);
	int error = EMAIL_ERROR_NONE;
	char *result_locale = NULL;

	setlocale(LC_ALL, "");

	result_locale = setlocale(LC_ALL, NULL);

	EM_DEBUG_LOG("LC_ALL[%s]" , result_locale);

	if ( EM_SAFE_STRCMP(result_locale, "or_IN.UTF-8") == 0)
		error = em_verify_email_address_without_regex(address);
	else
		error = em_verify_email_address_by_using_regex(address);

	EM_DEBUG_FUNC_END("error [%d]", error);
	return error;
}

INTERNAL_FUNC int em_verify_email_address_of_mail_data (email_mail_data_t *mail_data)
{
	EM_DEBUG_FUNC_BEGIN("mail_data[%p]", mail_data);
	char *address_array[4] = { mail_data->full_address_from, mail_data->full_address_to, mail_data->full_address_cc, mail_data->full_address_bcc};
	int  err = EMAIL_ERROR_NONE, i;

	/* check for email_address validation */
	for (i = 0; i < 4; i++) {
		if (address_array[i] && address_array[i][0] != 0) {
			err = em_verify_email_address (address_array[i]);
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

INTERNAL_FUNC int em_verify_email_address_of_mail_tbl(emstorage_mail_tbl_t *input_mail_tbl)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_tbl[%p]", input_mail_tbl);
	char *address_array[4] = { input_mail_tbl->full_address_to, input_mail_tbl->full_address_cc, input_mail_tbl->full_address_bcc, input_mail_tbl->full_address_from};
	int  err = EMAIL_ERROR_NONE, i;

	/* check for email_address validation */
	for (i = 0; i < 4; i++) {
		if (address_array[i] && address_array[i][0] != 0) {
			if ((err = em_verify_email_address (address_array[i])) != EMAIL_ERROR_NONE) {
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
	EM_DEBUG_LOG_SEC("em_upper_string result : %s\n", copy_of_subject);

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



	while ((result = g_strrstr(curpos, "RE:")) != NULL) {
		curpos = result + 3;
		EM_DEBUG_LOG_SEC("RE result : %s", curpos);
	}

	while ((result = g_strrstr(curpos, "FWD:")) != NULL) {
		curpos = result + 4;
		EM_DEBUG_LOG_SEC("FWD result : %s", curpos);
	}

	while ((result = g_strrstr(curpos, "FW:")) != NULL) {
		curpos = result + 3;
		EM_DEBUG_LOG_SEC("FW result : %s", curpos);
	}

	while (curpos != NULL && *curpos == ' ') {
		curpos++;
	}

	gap = curpos - copy_of_subject;

	EM_SAFE_STRNCPY(stripped_subject, subject + gap, stripped_subject_buffer_size);

FINISH_OFF:
	EM_SAFE_FREE(copy_of_subject);

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

INTERNAL_FUNC int em_get_account_server_type_by_account_id(char *multi_user_name, int account_id, email_account_server_t* account_server_type, int flag, int *error)
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

	if( !emstorage_get_account_by_id(multi_user_name, account_id, WITHOUT_OPTION, &account_tbl_data, false, &err)) {
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

/* thread with task queue generic functions */

pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;

email_thread_handle_t* em_thread_create(void *(*thread_exit)(void*), void *arg)
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
	worker_handle->destroy = destroy;
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

INTERNAL_FUNC int em_fopen(const char *filename, const char *mode, FILE **fp)
{
	EM_DEBUG_FUNC_BEGIN("filename : [%s]", filename);

	int err = EMAIL_ERROR_NONE;

	if (!filename) {
		EM_DEBUG_EXCEPTION("Invalid param");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	FILE *temp_fp = NULL;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	temp_fp = fopen(filename, mode);
	if (temp_fp == NULL) {
		EM_DEBUG_EXCEPTION("fopen failed : [%s][%d]", EM_STRERROR(errno_buf), errno);
		if (errno == EACCES || errno == EPERM)
			err = EMAIL_ERROR_PERMISSION_DENIED;
		else if (errno == ENOSPC)
			err = EMAIL_ERROR_MAIL_MEMORY_FULL;
		else
			err = EMAIL_ERROR_SYSTEM_FAILURE;
	}

	if (fp)
		*fp = temp_fp;
	else
		if (temp_fp) fclose(temp_fp);

	EM_DEBUG_FUNC_END();
	return err;
}

INTERNAL_FUNC int em_open(const char *filename, int oflags, mode_t mode, int *handle)
{
	EM_DEBUG_FUNC_BEGIN("filename : [%s]", filename);
	int err = EMAIL_ERROR_NONE;

	if (!filename) {
		EM_DEBUG_EXCEPTION("Invalid param");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	int temp_handle = -1;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (mode)
		temp_handle = open(filename, oflags, mode);
	else
		temp_handle = open(filename, oflags);

	if (temp_handle < 0) {
		EM_DEBUG_EXCEPTION("open failed : [%s][%d]", EM_STRERROR(errno_buf), errno);
		if (errno == EACCES || errno == EPERM)
			err = EMAIL_ERROR_PERMISSION_DENIED;
		else if (errno == ENOSPC)
			err = EMAIL_ERROR_MAIL_MEMORY_FULL;
		else
			err = EMAIL_ERROR_SYSTEM_FAILURE;
	}

	if (handle)
		*handle = temp_handle;
	else
		if (temp_handle >= 0) close(temp_handle);

	EM_DEBUG_FUNC_END();
	return err;
}
