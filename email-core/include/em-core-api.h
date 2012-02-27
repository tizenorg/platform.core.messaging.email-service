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


/******************************************************************************
 * File :  em-core-api.h
 * Desc :  Mail Engine API Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#ifndef _EM_CORE_API_H_
#define _EM_CORE_API_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


int em_core_encode_base64(char *src, unsigned long src_len, char **enc, unsigned long* enc_len, int *err_code);

int em_core_decode_quotedprintable(unsigned char *enc_text, unsigned long enc_len, char **dec_text, unsigned long*dec_len, int *err_code);

int em_core_decode_base64(unsigned char *enc_text, unsigned long enc_len, char **dec_text, unsigned long* dec_len, int *err_code);

int em_core_init(int *err_code);

/*  em_core_set_logout_status - Set the logout status */
EXPORT_API void em_core_set_logout_status(int status);

/*  em_core_get_logout_status - Get the logout status */
EXPORT_API void em_core_get_logout_status(int  *status);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*_EM_CORE_API_H_*/
