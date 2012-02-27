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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include "db-util.h"
#include "em-storage.h"
#include "emf-emn-storage.h"
#include "emf-dbglog.h"
#include "emflib.h"

#define EMF_DB_PARS_ERR(code__, msg__)			memset(msg__, 0x00, sizeof(msg__)); EDBGetErrorInfo(&code__, msg__)

#ifdef UNUSED
sqlite3* sqlite_emmb;

static int _string_to_tm(struct tm* time, const char* string);
static void _tm_to_string(const struct tm* time, char* string, size_t string_size);


static void __emn_storage_log_func(const char* format, ...)
{
	FILE* fp = NULL;
	va_list va_args;
	char msg_buffer[256] = {0, };
	
	va_start(va_args, format);
	vsnprintf(msg_buffer, sizeof(msg_buffer), format, va_args);
	va_end(va_args);
	
	if (strlen(LOG_FILE_PATH) > 0)
		fp = fopen(LOG_FILE_PATH, "a");
	else 
		fp = fopen("/var/log/email_ui.log", "a");
	
	if (fp != NULL) {
/* 		tzset(); */
		time_t ct = time(&ct);
		struct tm* lt = localtime(&ct);
		
		if (lt) {
			fprintf(fp, "[%d-%d-%d %d:%d:%d] %s: %s\n",
				lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
				lt->tm_hour, lt->tm_min, lt->tm_sec,
				"EMN", msg_buffer);
		}
		
		fclose(fp);
	}
}


enum {
	ACCOUNT_ID___MAIL_NEW_TBL = 0,
	ACCOUNT_NAME___MAIL_NEW_TBL,
	ARRIVALTIME___MAIL_NEW_TBL,
	NEW_MAIL_COUNT___MAIL_NEW_TBL,
	SVC_TYPE___MAIL_NEW_TBL,		
};

/*  static char _g_sql_query[1024] = {0x00, }; */
/*  char _g_err_msg[1024] = {0x00, }; */
/*  int _g_err = 0; */


static char* _cpy_str(char* src)
{
	char* p = NULL;
	if (src) {
		if (!(p = malloc((int)strlen(src) + 1)))
			return NULL;
		
		strcpy(p, src);
	}
	return p;
}

static int _emf_storage_close(int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = true;
	int rc = -1;
	rc = db_util_close( sqlite_emmb);
	sqlite_emmb = NULL;
	if(SQLITE_OK != rc) {
		EM_DEBUG_LOG("\t db_util_close failed - %d\n", rc);
		ret = false;	
	}
	
	if (err_code != NULL)
		*err_code = rc;
	
    return ret;
}

static int _emf_storage_open(int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = true;
	
	if ( !em_db_open(&sqlite_emmb, NULL) ) {
		EM_DEBUG_EXCEPTION("em_db_open failed\n");
		ret = false;
	}					
	
	return ret;
}



static int _string_to_tm(struct tm* time, const char* string)
{
	EM_DEBUG_FUNC_BEGIN();
	
	char year[5] = {0x00, };
	char month[3] = {0x00, };
	char hour[3] = {0x00, };
	char day[3] = {0x00, };
	char min[3] = {0x00, };
	char sec[3] = {0x00, };
	
	if (!time)
		return false;
	
	if ((!string) || (strlen(string) != 14)) 
		return false;
	
	strncpy(year, string, 4); time->tm_year = atoi(year); string += 4;
	strncpy(month, string, 2); time->tm_mon = atoi(month); string += 2;
	strncpy(day, string, 2); time->tm_mday = atoi(day); string += 2;
	strncpy(hour, string, 2); time->tm_hour = atoi(hour); string += 2;
	strncpy(min, string, 2);  time->tm_min = atoi(min); string += 2;
	strncpy(sec, string, 2); time->tm_sec = atoi(sec);
	
	return true;
}

static void _tm_to_string(const struct tm* time, char* string, size_t string_size)
{
	EM_DEBUG_FUNC_BEGIN();
	
	memset(string, 0x00, string_size);
	
	snprintf(string, string_size, "%04d%02d%02d%02d%02d%02d", 
						   time->tm_year, time->tm_mon,
						   time->tm_mday, time->tm_hour,
						   time->tm_min,  time->tm_sec);
	
	return;
}

#endif

