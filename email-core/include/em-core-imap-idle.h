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


/**
 *
 * This file contains functinality related to IMAP IDLE.
 * @file		em-core-imap-idle.h
 * @author	
 * @version	0.1
 * @brief 		This file contains functionality to provide IMAP IDLE support in email-service. 
 */

#include "emflib.h"


/**

 * @open
 * @fn em_core_imap_idle_thread_create(int *err_code)
 * @brief	Creates a thread that listens for IMAP IDLE Notifications.
 *
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */

EXPORT_API int em_core_imap_idle_thread_create(int accountID, int *err_code);

/**

 * @open
 * @fn em_core_imap_idle_thread_kill(int *err_code)
 * @brief	Kills IMAP IDLE thread
 *
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int em_core_imap_idle_thread_kill(int *err_code);



/**

 * @open
 * @fn em_core_imap_idle_run(int *err_code, int iAccountID)
 * @brief	Make a list of mailboxes to IDLE on, connect to all mailboxes in list and start idle loop
 *
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */

static int em_core_imap_idle_run(int iAccountID);

/**

 * @open
 * @fn em_core_imap_idle_loop_start(emf_mailbox_t *mailbox_list,  int num, int *err_code)
 * @brief	starts a loop which waits on select call. Select call monitors all the socket descriptors in the hold_connection fields of mailbox_list 
 *
 * @param[in] mailbox_list 	list of mailboxes
 * @param[in] num			Count of mailboxes in list
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
static int em_core_imap_idle_loop_start(emf_mailbox_t *mailbox_list,  int num, int *err_code);



/**

 * @open
 * @fn em_core_imap_idle_loop_start(emf_mailbox_t *mailbox_list,  int num, int *err_code)
 * @brief	Creates and inserts an event in event queue for syncing mailbox
 *
 * @param[in] mailbox	 	mailbox to be synced.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
static int em_core_imap_idle_insert_sync_event(emf_mailbox_t *mailbox, int *err_code);



/**

 * @open
 * @fn em_core_imap_idle_connect_and_idle_on_mailbox(emf_mailbox_t *mailbox, int *err_code)
 * @brief	Opens connection to mailbox(selects mailbox) and sends IDLE command
 *
 * @param[in] mailbox	 	mailbox to IDLE on.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
static int em_core_imap_idle_connect_and_idle_on_mailbox(emf_mailbox_t *mailbox, int *err_code);



/**

 * @open
 * @fn em_core_imap_idle_parse_response_stream(emf_mailbox_t *mailbox, int *err_code)
 * @brief	Gets and parsee the IDLE notification coming from network
 *
 * @param[in] mailbox	 	mailbox that got IDLE notification.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
static int em_core_imap_idle_parse_response_stream(emf_mailbox_t *mailbox, int *err_code);
