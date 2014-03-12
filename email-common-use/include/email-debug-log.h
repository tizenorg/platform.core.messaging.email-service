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



/******************************************************************************
 * File: email-debug-log.h
 * Desc: email-common-use debug header
 *
 * Auth:
 *
 * History:
 *    2006.08.01 : created
 *****************************************************************************/
#ifndef __EMAIL_DEBUG_LOG_H__
#define __EMAIL_DEBUG_LOG_H__

#ifdef  __cplusplus
extern "C"
{
#endif
#include <tzplatform_config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlog.h>
#include <errno.h>

#define __FEATURE_DEBUG_LOG__
#define __FEATURE_LOG_FOR_LIFE_CYCLE_OF_FUNCTION__
#define __FEATURE_LOG_FOR_DEBUG_LOG_DEV__

#ifdef  __FEATURE_DEBUG_LOG__

/* definition of LOG_TAG */
#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "EMAIL_SERVICE"

#define	EM_DEBUG_LOG(format, arg...)        SLOGD(format, ##arg)
#define	EM_DEBUG_EXCEPTION(format, arg...)  SLOGE("[EXCEPTION!] " format "\n", ##arg)

#ifdef _SECURE_LOG
#undef _SECURE_LOG
#endif

#ifdef _SECURE_LOG
#define	EM_DEBUG_LOG_SEC(format, arg...)        SECURE_SLOGD(format, ##arg)
#define	EM_DEBUG_EXCEPTION_SEC(format, arg...)  SECURE_SLOGE("[EXCEPTION!] " format "\n", ##arg)
#define	EM_DEBUG_FUNC_BEGIN_SEC(format, arg...) EM_DEBUG_LOG_SEC("BEGIN - "format, ##arg)
#else
#define	EM_DEBUG_LOG_SEC(format, arg...)        SLOGD(format, ##arg)
#define	EM_DEBUG_EXCEPTION_SEC(format, arg...)  SLOGE("[EXCEPTION!] " format "\n", ##arg)
#define	EM_DEBUG_FUNC_BEGIN_SEC(format, arg...) EM_DEBUG_LOG("BEGIN - "format, ##arg)
#endif

#ifdef  _DEBUG_MIME_PARSE_
#define EM_DEBUG_LOG_MIME(format, arg...)   EM_DEBUG_LOG_SEC(format, ##arg)
#else   /*  _DEBUG_MIME_PARSE */
#define EM_DEBUG_LOG_MIME(format, arg...)
#endif /*  _DEBUG_MIME_PARSE */

#ifdef __FEATURE_LOG_FOR_LIFE_CYCLE_OF_FUNCTION__
#define	EM_DEBUG_FUNC_BEGIN(format, arg...) EM_DEBUG_LOG("BEGIN - "format, ##arg)
#define	EM_DEBUG_FUNC_END(format, arg...)   EM_DEBUG_LOG("END - "format, ##arg)
#else /* __FEATURE_LOG_FOR_LIFE_CYCLE_OF_FUNCTION__ */
#define	EM_DEBUG_FUNC_BEGIN(format, arg...)
#define	EM_DEBUG_FUNC_END(format, arg...)
#undef	EM_DEBUG_FUNC_BEGIN_SEC
#define	EM_DEBUG_FUNC_BEGIN_SEC(format, arg...)
#endif

#ifdef __FEATURE_LOG_FOR_DEBUG_LOG_DEV__
#define	EM_DEBUG_LOG_DEV(format, arg...) EM_DEBUG_LOG("[DEV] " format, ##arg)
#else /* __FEATURE_LOG_FOR_DEBUG_LOG_DEV__ */
#define	EM_DEBUG_LOG_DEV(format, arg...)
#endif

#define	EM_DEBUG_LINE                       EM_DEBUG_LOG("FUNC[%s : %d]", __FUNCTION__, __LINE__)
#define EM_DEBUG_DB_EXEC(eval, expr, X)     if (eval) { EM_DEBUG_LOG X; expr;} else {;}

#define EM_DEBUG_ERROR_FILE_PATH            tzplatform_mkpath(TZ_USER_DATA, "email/.email_data/.critical_error.log")
#define EM_DEBUG_CRITICAL_EXCEPTION(format, arg...)   \
			{\
				FILE *fp_error = NULL;\
				fp_error = fopen(EM_DEBUG_ERROR_FILE_PATH, "a");\
				if(fp_error) {\
					char now_time_string[30] = {0,}; \
					time_t now = time(NULL); \
					strftime(now_time_string, 20, "%Y.%m.%d %H:%M:%S", localtime(&now)); \
					fprintf(fp_error, "[%s][%s() :%s:%d] " format "\n",\
						now_time_string, \
						__FUNCTION__, (rindex(__FILE__, '/')? rindex(__FILE__,'/')+1 : __FILE__ ),\
						__LINE__, ##arg); \
					fclose(fp_error);\
				}\
			}
#define EM_DEBUG_ALARM_LOG_FILE_PATH         tzplatform_mkpath(TZ_USER_DATA, "email/.email_data/.alarm.log")
#define EM_DEBUG_ALARM_LOG(format, arg...)   \
			{\
				FILE *fp_error = NULL;\
				fp_error = fopen(EM_DEBUG_ALARM_LOG_FILE_PATH, "a");\
				if(fp_error) {\
					char now_time_string[30] = {0,}; \
					time_t now = time(NULL); \
					strftime(now_time_string, 20, "%Y.%m.%d %H:%M:%S", localtime(&now)); \
					fprintf(fp_error, "[%s][%s() :%s:%d] " format "\n",\
						now_time_string, \
						__FUNCTION__, (rindex(__FILE__, '/')? rindex(__FILE__,'/')+1 : __FILE__ ),\
						__LINE__, ##arg); \
					fclose(fp_error);\
				}\
			}
#define EM_DEBUG_PERROR(str) 	({\
		char buf[128] = {0};\
		strerror_r(errno, buf, sizeof(buf));\
		EM_DEBUG_EXCEPTION("%s: %s(%d)", str, buf, errno);\
	})

#define EM_VALIDATION_SYSTEM_LOG(CLASSIFICATION, MAIL_ID, FORMAT, ARG...) {char now_time_string[30] = {0,}; \
		time_t now = time(NULL); \
		strftime(now_time_string, 20, "%Y.%m.%d %H:%M:%S", localtime(&now)); \
		LOG(LOG_VERBOSE, "VLD_EMAIL", "[Email " CLASSIFICATION "]%s, %d," FORMAT "\n", now_time_string, MAIL_ID, ##ARG);}

#ifdef _USE_PROFILE_DEBUG_
#define EM_PROFILE_BEGIN(pfid) \
	unsigned int __prf_l1_##pfid = __LINE__;\
	struct timeval __prf_1_##pfid;\
	struct timeval __prf_2_##pfid;\
	do {\
		gettimeofday(&__prf_1_##pfid, 0);\
		EM_DEBUG_LOG("**PROFILE BEGIN** [EMAILFW: %s() :%s %u ~ ] " #pfid \
		" ->  Start Time: %u.%06u seconds\n",\
			__FUNCTION__,\
		rindex(__FILE__,'/')+1, \
		__prf_l1_##pfid,\
		(unsigned int)__prf_1_##pfid.tv_sec,\
		(unsigned int)__prf_1_##pfid.tv_usec );\
	} while (0)

#define EM_PROFILE_END(pfid) \
	unsigned int __prf_l2_##pfid = __LINE__;\
	do { \
		gettimeofday(&__prf_2_##pfid, 0);\
		long __ds = __prf_2_##pfid.tv_sec - __prf_1_##pfid.tv_sec;\
		long __dm = __prf_2_##pfid.tv_usec - __prf_1_##pfid.tv_usec;\
		if ( __dm < 0 ) { __ds--; __dm = 1000000 + __dm; } \
		EM_DEBUG_LOG("**PROFILE END** [EMAILFW: %s() :%s %u ~ %u] " #pfid                            \
		" -> Elapsed Time: %u.%06u seconds\n",\
			__FUNCTION__,\
		rindex(__FILE__, '/')+1,\
		__prf_l1_##pfid,\
		__prf_l2_##pfid,\
		(unsigned int)(__ds),\
		(unsigned int)(__dm));\
	} while (0)
#else

#define EM_PROFILE_BEGIN(pfid)
#define EM_PROFILE_END(pfid)

#endif


#else /* __FEATURE_DEBUG_LOG__ */

	#define EM_DEBUG_LINE
	#define EM_DEBUG_LOG(format, arg...)
	#define EM_DEBUG_ASSERT(format, arg...)
	#define EM_DEBUG_EXCEPTION(format, arg...)


	#define EM_DEBUG_DB_EXEC(eval, expr, X)  if (eval) { EM_DEBUG_DB X; expr;} else {;}
	#define EM_PROFILE_BEGIN(pfid)
	#define EM_PROFILE_END(pfid)

#endif /* __FEATURE_DEBUG_LOG__ */

/* Use it in api function */
#define	EM_DEBUG_API_BEGIN(format, arg...) EM_DEBUG_LOG("BEGIN - "format, ##arg)
#define	EM_DEBUG_API_END(format, arg...)   EM_DEBUG_LOG("END - "format, ##arg)

#define EM_NULL_CHECK_FOR_VOID(expr)	\
	{\
		if (!expr) {\
			EM_DEBUG_EXCEPTION ("INVALID PARAM: "#expr" NULL ");\
			return;\
		}\
	}

#define EM_IF_NULL_RETURN_VALUE(expr, val) \
	{\
		if (!expr ) {\
			EM_DEBUG_EXCEPTION ("INVALID PARAM: "#expr" NULL ");\
			return val;	\
		}\
	}

#define EM_RETURN_ERR_CODE(err_ptr, err_code, ret) \
	{\
		if(err_ptr) *err_ptr = err_code;\
		return ret;\
	}

#define EM_EXIT_ERR_CODE(err_ptr, err_code) \
	{\
		if(err_ptr) *err_ptr = err_code;\
		return;\
	}


#define EM_SAFE_FREE(expr)	 \
	({\
		if (expr) {\
			free (expr);\
			expr = NULL;\
		}\
	})

#define EM_SAFE_STRDUP(s) \
	({\
		char* _s = (char*)s;\
		(_s)? strdup(_s) : NULL;\
	})

#define EM_SAFE_STRCMP(dest, src) \
	({\
		char* _dest = dest;\
		char* _src = src;\
		((_src) && (_dest))? strcmp(_dest, _src) : -1;\
	})

#define EM_SAFE_STRCPY(dest, src) \
	({\
		char* _dest = dest;\
		char* _src = src;\
		((_src) && (_dest))? strcpy(_dest, _src) : NULL;\
	})

#define EM_SAFE_STRNCPY(dest, src, size) \
	({\
		char* _dest = dest;\
		char* _src = src;\
		int _size = size;\
		((_src) && (_dest))? strncpy(_dest, _src, _size) : NULL;\
	})

#define EM_SAFE_STRCAT(dest, src) \
	({\
		char* _dest = dest;\
		char* _src = src;\
		((_src) && (_dest))? strcat(_dest, _src) : NULL;\
	})

#define EM_SAFE_STRLEN(s) \
	({\
		char* _s = (char*)s;\
		(_s)? strlen(_s) : 0;\
	})


#define EM_IF_ACCOUNT_ID_NULL(expr, ret) {\
		if (expr <= 0) {\
			EM_DEBUG_EXCEPTION_SEC ("EM_IF_ACCOUNT_ID_NULL: Account ID [ %d ]  ", expr);\
			return ret;\
		}\
	}


#define ERRNO_BUF_SIZE		64

#define EM_STRERROR(errno_buf) ({\
					strerror_r(errno, errno_buf, sizeof(errno_buf));\
					errno_buf;\
				})


#ifdef  __cplusplus
}
#endif	/* __cplusplu */

#endif	/* __EMAIL_DEBUG_LOG_H__ */
