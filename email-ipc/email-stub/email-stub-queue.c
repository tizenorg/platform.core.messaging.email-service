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
#include <malloc.h>

#include "email-stub-queue.h"
#include "email-ipc-build.h"

#include "email-debug-log.h"

static emipc_email_queue_item *head = NULL;
static emipc_email_queue_item *tail = NULL;
int count = 0;

EXPORT_API void *emipc_pop_in_queue()
{
	void *data = NULL;

	if (head) {
		emipc_email_queue_item *popped = head;
		data = popped->data;
	
		if (popped->next) {
			head = popped->next;
		} else {
			head = tail = NULL;
		}
		
		EM_SAFE_FREE(popped);
		count = (count <= 0) ? 0 : count-1;
		
		return data;
	}
	return NULL;
}	

EXPORT_API bool emipc_push_in_queue(void *data)
{
	emipc_email_queue_item *item = NULL;
	if (!data) {
		EM_DEBUG_EXCEPTION("[IPCLib] emipc_push_in_queue - invalid input\n");
		return false;
	}

	item = (emipc_email_queue_item *)malloc(sizeof(emipc_email_queue_item));
	if (item == NULL) {
		EM_DEBUG_EXCEPTION("Malloc failed.");
		return false;
	}
	memset(item, 0x00, sizeof(emipc_email_queue_item));

	item->data = data;
	item->next = NULL;
	if (tail) {
		tail->next = item;
		tail = item;
	} else {
		head = tail = item;
	}

	count++;
	
	return true;
}

EXPORT_API int emipc_get_count_of_item()
{
	return count;
}
