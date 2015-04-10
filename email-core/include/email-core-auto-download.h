/*
*  email-service
*
* Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Minsoo Kim <minnsoo.kim@samsung.com>, Kyuho Jo <kyuho.jo@samsung.com>,
* Sunghyun Kwon <sh0701.kwon@samsung.com>
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

#ifndef __EMAIL_CORE_AUTO_DOWNLOAD_H__
#define __EMAIL_CORE_AUTO_DOWNLOAD_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct auto_download_t
{
	int activity_id;
	int status;
	int account_id;
	int mail_id;
	unsigned long server_mail_id;
	int mailbox_id;
	char *multi_user_name;
} email_event_auto_download;

INTERNAL_FUNC int emcore_start_auto_download_loop(int *err_code);
INTERNAL_FUNC int emcore_auto_download_loop_continue(void);
INTERNAL_FUNC int emcore_stop_auto_download_loop(int *err_code);
INTERNAL_FUNC int emcore_insert_auto_download_event(email_event_auto_download *event_data, int *err_code);
INTERNAL_FUNC int emcore_retrieve_auto_download_event(email_event_auto_download **event_data, int *err_code);
INTERNAL_FUNC int emcore_is_auto_download_queue_empty(void);
INTERNAL_FUNC int emcore_is_auto_download_queue_full(void);
INTERNAL_FUNC int emcore_clear_auto_download_queue(void);

INTERNAL_FUNC int emcore_insert_auto_download_job(char *multi_user_name, int account_id, int mailbox_id, int mail_id, int auto_download_on, char *uid, int *err_code);
INTERNAL_FUNC int emcore_insert_auto_download_activity(email_event_auto_download *local_activity, int *activity_id, int *err_code);
INTERNAL_FUNC int emcore_delete_auto_download_activity(char *multi_user_name, int account_id, int mail_id, int activity_id, int *err_code);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* EOF */
