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
 * email-core-task-manager.c
 *
 *  Created on: 2012. 11. 1.
 *      Author: kyuho.jo@samsung.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dbus/dbus.h>

#include "email-internal-types.h"
#include "email-utilities.h"
#include "email-core-tasks.h"
#include "email-core-task-manager.h"
#include "email-core-signal.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-debug-log.h"

/* TODO : implement a function for removing a task from task pool */
/* TODO : after fetching a task from DB, update status of the task. */


#define REGISTER_TASK_BINDER(TASK_NAME) emcore_register_task_handler(TASK_NAME, task_handler_##TASK_NAME, email_encode_task_parameter_##TASK_NAME, email_decode_task_parameter_##TASK_NAME)

/*- variables - begin --------------------------------------------------------*/
static pthread_cond_t  _task_available_signal   = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t _task_available_lock     = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _task_manager_loop_lock  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _active_task_pool_lock   = PTHREAD_MUTEX_INITIALIZER;

static email_active_task_t _active_task_pool[MAX_ACTIVE_TASK];
static thread_t            _thread_task_manager_loop;
static int                 _task_manager_loop_availability = 1;
/*- variables - end ----------------------------------------------------------*/
static int emcore_insert_task_to_active_task_pool(int input_task_slot_index, int input_task_id, email_task_type_t input_task_type, thread_t input_thread_id);
static int emcore_remove_task_from_active_task_pool(int input_task_id);
static int emcore_find_available_slot_in_active_task_pool(int *result_index);
static int emcore_update_task_status_on_task_table(int input_task_id, email_task_status_type_t task_status);
static int emcore_get_task_handler_reference(email_task_type_t input_task_type, email_task_handler_t **output_task_handler);

/*- task handlers helpers - begin --------------------------------------------*/
static int emcore_initialize_task_handler(email_task_t *input_task)
{
	EM_DEBUG_FUNC_BEGIN("input_task [%p]", input_task);
	int err = EMAIL_ERROR_NONE;

	if(input_task == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* insert task to _active_task_pool */
	emcore_insert_task_to_active_task_pool(input_task->active_task_id
			, input_task->task_id
			, input_task->task_type
			, THREAD_SELF());

	/* send notification for 'task start */
	if( (err = emcore_send_task_status_signal(input_task->task_type, input_task->task_id, EMAIL_TASK_STATUS_STARTED, EMAIL_ERROR_NONE, 0)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_send_task_status_signal failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static int emcore_finalize_task_handler(email_task_t *input_task, int input_error_code)
{
	EM_DEBUG_FUNC_BEGIN("input_task [%p] input_error_code [%d]",input_task ,input_error_code);
	int err = EMAIL_ERROR_NONE;
	email_task_status_type_t task_status = EMAIL_TASK_STATUS_FINISHED;

	if(input_task == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if(input_error_code != EMAIL_ERROR_NONE) {
		task_status = EMAIL_TASK_STATUS_FAILED;
	}

	/* remove task from task table */
	if( (err = emcore_remove_task_from_task_table(input_task->task_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_remove_task_from_active_task_pool failed [%d]", err);
		goto FINISH_OFF;
	}

	/* remove task id from active task id array */
	if( (err = emcore_remove_task_from_active_task_pool(input_task->task_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_remove_task_from_active_task_pool failed [%d]", err);
		goto FINISH_OFF;
	}

	/* send signal for 'task finish or failure */
	if( (err = emcore_send_task_status_signal(input_task->task_type, input_task->task_id, task_status, input_error_code, 0)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_send_task_status_signal failed [%d]", err);
		goto FINISH_OFF;
	}

	ENTER_CRITICAL_SECTION(_task_available_lock);
	WAKE_CONDITION_VARIABLE(_task_available_signal);
	LEAVE_CRITICAL_SECTION(_task_available_lock);

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

void* emcore_default_task_handler(void *intput_param)
{
	EM_DEBUG_FUNC_BEGIN("intput_param [%p]", intput_param);
	int err = EMAIL_ERROR_NONE;
	email_task_t *task = intput_param;
	email_task_handler_t *task_handler = NULL;
	void *decoded_task_parameter = NULL;

	if((err = emcore_initialize_task_handler(task)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_initialize_task_handler failed. [%d]", err);
		goto FINISH_OFF;
	}

	/* create a thread to do this task */
	if((err = emcore_get_task_handler_reference(task->task_type, &task_handler)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_LOG("emcore_get_task_handler_reference returns [%d]", err);
	}
	else {
		/* Decode parameter */
		emcore_decode_task_parameter(task->task_type, task->task_parameter, task->task_parameter_length, &decoded_task_parameter);
		task_handler->task_handler_function(decoded_task_parameter);
	}

FINISH_OFF:
	emcore_finalize_task_handler(task, err);

	if(task) {
		EM_SAFE_FREE(task->task_parameter);
		EM_SAFE_FREE(task);
	}

	EM_SAFE_FREE(decoded_task_parameter);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return NULL;
}
/*- task handlers helpers - end   --------------------------------------------*/

int                   _task_handler_array_size;
email_task_handler_t **_task_handler_array;

static int emcore_register_task_handler(email_task_type_t input_task_type, void* (*input_task_handler)(void *), int (*input_task_parameter_encoder)(void*,char**,int*), int (*input_task_parameter_decoder)(char*,int,void**))
{
	EM_DEBUG_FUNC_BEGIN("input_task_type [%d] input_task_handler [%p] input_task_parameter_encoder [%p] input_task_parameter_decoder [%p]", input_task_type, input_task_handler, input_task_parameter_encoder, input_task_parameter_decoder);
	int err = EMAIL_ERROR_NONE;
	email_task_handler_t *new_task_handler = NULL;

	new_task_handler = malloc(sizeof(email_task_handler_t));

	if (new_task_handler == NULL) {
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		goto FINISH_OFF;
	}

	new_task_handler->task_type              = input_task_type;
	new_task_handler->task_handler_function  = input_task_handler;
	new_task_handler->task_parameter_encoder = input_task_parameter_encoder;
	new_task_handler->task_parameter_decoder = input_task_parameter_decoder;

	_task_handler_array_size++;

	if (_task_handler_array) {
		_task_handler_array = realloc(_task_handler_array, sizeof(email_task_handler_t*) * _task_handler_array_size);
	}
	else {
		_task_handler_array = malloc(sizeof(email_task_handler_t*) * _task_handler_array_size);
	}

	if (_task_handler_array == NULL) {
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	_task_handler_array[_task_handler_array_size - 1] = new_task_handler;

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


INTERNAL_FUNC int emcore_init_task_handler_array()
{
	EM_DEBUG_FUNC_BEGIN();

	if (_task_handler_array == NULL) {
		_task_handler_array      = NULL;
		_task_handler_array_size = 0;

		REGISTER_TASK_BINDER(EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT);
		REGISTER_TASK_BINDER(EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX);
		REGISTER_TASK_BINDER(EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL);
	}

	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

INTERNAL_FUNC int emcore_free_task_handler_array()
{
	EM_DEBUG_FUNC_BEGIN();

	int i = 0;

	for(i = 0; i < _task_handler_array_size; i++) {
		free(_task_handler_array[i]);
	}

	free(_task_handler_array);
	_task_handler_array      = NULL;
	_task_handler_array_size = 0;

	EM_DEBUG_FUNC_END();
	return EMAIL_ERROR_NONE;
}

static int emcore_get_task_handler_reference(email_task_type_t input_task_type, email_task_handler_t **output_task_handler)
{
	EM_DEBUG_FUNC_BEGIN("input_task_type [%d] output_task_handler [%p]", input_task_type, output_task_handler);
	int i = 0;
	int err = EMAIL_ERROR_NONE;

	if (output_task_handler == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	*output_task_handler = NULL;
	for (i = 0; i < _task_handler_array_size; i++) {
		if (_task_handler_array[i]->task_type == input_task_type) {
			*output_task_handler = _task_handler_array[i];
			break;
		}
	}

	if (*output_task_handler == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_TASK_BINDER_NOT_FOUND");
		err = EMAIL_ERROR_TASK_BINDER_NOT_FOUND;
		goto FINISH_OFF;
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_encode_task_parameter(email_task_type_t input_task_type, void *input_task_parameter_struct, char **output_byte_stream, int *output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN("input_task_type [%d] input_task_parameter_struct [%p] output_byte_stream [%p] output_stream_size [%p]", input_task_type, input_task_parameter_struct, output_byte_stream, output_stream_size);
	int err = EMAIL_ERROR_NONE;
	email_task_handler_t *task_handler = NULL;
	int (*task_parameter_encoder)(void*, char**, int*);

	if (input_task_parameter_struct == NULL || output_byte_stream == NULL || output_stream_size == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emcore_get_task_handler_reference(input_task_type, &task_handler)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_task_handler_reference failed [%d]", err);
		goto FINISH_OFF;
	}

	task_parameter_encoder = task_handler->task_parameter_encoder;

	if ((err = task_parameter_encoder(input_task_parameter_struct, output_byte_stream, output_stream_size)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("task_parameter_encoder failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_decode_task_parameter(email_task_type_t input_task_type, char *input_byte_stream, int input_stream_size, void **output_task_parameter_struct)
{
	EM_DEBUG_FUNC_BEGIN("input_task_type [%d] input_byte_stream [%p] input_stream_size [%d] output_task_parameter_struct [%p]", input_task_type, input_byte_stream, input_stream_size, output_task_parameter_struct);
	int err = EMAIL_ERROR_NONE;
	email_task_handler_t *task_handler = NULL;
	int (*task_parameter_decoder)(char*, int, void**);

	if (input_byte_stream == NULL || output_task_parameter_struct == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = emcore_get_task_handler_reference(input_task_type, &task_handler)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_task_handler_reference failed [%d]", err);
		goto FINISH_OFF;
	}

	task_parameter_decoder = task_handler->task_parameter_decoder;

	if ((err = task_parameter_decoder(input_byte_stream, input_stream_size, output_task_parameter_struct)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("task_parameter_decoder failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

int emcore_fetch_task_from_task_pool(email_task_t **output_task)
{
	EM_DEBUG_FUNC_BEGIN("output_task [%p]", output_task);
	int err = EMAIL_ERROR_NONE;
	int output_task_count;

	if((err = emstorage_query_task("WHERE task_status == 1", " ORDER BY date_time ASC, task_priority ASC LIMIT 0, 1", output_task, &output_task_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_task failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


/*-Task manager loop - begin -------------------------------------------------------------*/


static int emcore_insert_task_to_active_task_pool(int input_task_slot_index, int input_task_id, email_task_type_t input_task_type, thread_t input_thread_id)
{
	EM_DEBUG_FUNC_BEGIN("input_task_slot_index [%d] input_task_id [%d] input_task_type [%d] input_thread_id [%d]", input_task_slot_index, input_task_id, input_task_type, input_thread_id);
	int err = EMAIL_ERROR_NONE;

	ENTER_CRITICAL_SECTION(_active_task_pool_lock);
	_active_task_pool[input_task_slot_index].task_id    = input_task_id;
	_active_task_pool[input_task_slot_index].task_type  = input_task_type;
	_active_task_pool[input_task_slot_index].thread_id  = input_thread_id;
	LEAVE_CRITICAL_SECTION(_active_task_pool_lock);

	EM_DEBUG_LOG("_active_task_pool[%d].task_id [%d]", input_task_slot_index, _active_task_pool[input_task_slot_index].task_id);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static int emcore_remove_task_from_active_task_pool(int input_task_id)
{
	EM_DEBUG_FUNC_BEGIN(" input_task_id [%d]", input_task_id);
	int err = EMAIL_ERROR_NONE;
	int i = 0;

	ENTER_CRITICAL_SECTION(_active_task_pool_lock);
	for(i = 0; i < MAX_ACTIVE_TASK; i++) {
		if(_active_task_pool[i].task_id == input_task_id) {
			_active_task_pool[i].task_id    = 0;
			_active_task_pool[i].task_type  = 0;
			_active_task_pool[i].thread_id  = 0;
			break;
		}
	}
	LEAVE_CRITICAL_SECTION(_active_task_pool_lock);

	if(i >= MAX_ACTIVE_TASK) {
		EM_DEBUG_LOG("couldn't find proper task in active task pool [%d]", input_task_id);
		err = EMAIL_ERROR_TASK_NOT_FOUND;
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_get_active_task_by_thread_id(thread_t input_thread_id, email_active_task_t **output_active_task)
{
	EM_DEBUG_FUNC_BEGIN(" input_thread_id [%d] output_active_task [%p]", input_thread_id, output_active_task);
	int err = EMAIL_ERROR_NONE;
	int i = 0;

	if (output_active_task == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	for(i = 0; i < MAX_ACTIVE_TASK; i++) {
		if(_active_task_pool[i].thread_id == input_thread_id) {
			*output_active_task = _active_task_pool + i;
			break;
		}
	}

	if(i >= MAX_ACTIVE_TASK) {
		EM_DEBUG_LOG("couldn't find proper task in active task pool [%d]", input_thread_id);
		err = EMAIL_ERROR_TASK_NOT_FOUND;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


static int emcore_find_available_slot_in_active_task_pool(int *result_index)
{
	EM_DEBUG_FUNC_BEGIN("result_index [%p]", result_index);
	int i = 0;
	int err = EMAIL_ERROR_NONE;

	if (result_index == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	ENTER_CRITICAL_SECTION(_active_task_pool_lock);
	for(i = 0; i < MAX_ACTIVE_TASK; i++) {
		if(_active_task_pool[i].task_id == 0) {
			break;
		}
	}

	if(i >= MAX_ACTIVE_TASK) {
		EM_DEBUG_EXCEPTION("There is no available task slot");
		err = EMAIL_NO_AVAILABLE_TASK_SLOT;
	}
	else {
		_active_task_pool[i].task_id = -1;
		*result_index = i;
	}
	LEAVE_CRITICAL_SECTION(_active_task_pool_lock);

FINISH_OFF :
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

void* thread_func_task_manager_loop(void *arg)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int thread_error;
	int available_slot_index = 0;
	email_task_t *new_task = NULL;

	/* while loop */
	while (_task_manager_loop_availability) {
		/* fetch task from DB */
		if( ((err = emcore_fetch_task_from_task_pool(&new_task)) == EMAIL_ERROR_NONE) &&
			((err = emcore_find_available_slot_in_active_task_pool(&available_slot_index)) == EMAIL_ERROR_NONE)	) {

			/* update task status as STARTED */
			if((err = emcore_update_task_status_on_task_table(new_task->task_id, EMAIL_TASK_STATUS_STARTED)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_update_task_status_on_task_table failed [%d]", err);
			}
			new_task->active_task_id = available_slot_index;

			/* create a thread to do this task */
			THREAD_CREATE(new_task->thread_id, emcore_default_task_handler, new_task, thread_error);

			/* new_task and task_parameter will be free in task handler. */
			new_task     = NULL;

		}
		else {
			/* If there is no task or no available slot, sleep until someone wake you up. */
			/* Wake up case 1 : by emcore_add_task_to_task_table */
			/* Wake up case 2 : when some task terminated */
			ENTER_CRITICAL_SECTION(_task_available_lock);
			SLEEP_CONDITION_VARIABLE(_task_available_signal, _task_available_lock);
			LEAVE_CRITICAL_SECTION(_task_available_lock);
			EM_DEBUG_LOG("thread_func_task_manager_loop wake up!");
		}
	}

	EM_DEBUG_FUNC_END();
	return NULL;
}

INTERNAL_FUNC int emcore_start_task_manager_loop()
{
    EM_DEBUG_FUNC_BEGIN();
	int thread_error;
	int err = EMAIL_ERROR_NONE;

	memset(&_active_task_pool, 0, sizeof(email_active_task_t) * MAX_ACTIVE_TASK);

	if (_thread_task_manager_loop) {
		EM_DEBUG_EXCEPTION("service thread is already running...");
		err = EMAIL_ERROR_ALREADY_INITIALIZED;
		goto FINISH_OFF;
	}
	emcore_init_task_handler_array();

	_task_manager_loop_availability = 10;

    /* create thread */
	THREAD_CREATE(_thread_task_manager_loop, thread_func_task_manager_loop, NULL, thread_error);

	if (thread_error != 0) {
        EM_DEBUG_EXCEPTION("cannot create thread [%d]", thread_error);
        err = EMAIL_ERROR_SYSTEM_FAILURE;
        goto FINISH_OFF;
    }

FINISH_OFF :
	EM_DEBUG_FUNC_END("err [%d]", err);
    return err;
}

INTERNAL_FUNC int emcore_stop_task_manager_loop()
{
    EM_DEBUG_FUNC_BEGIN();

    int err = EMAIL_ERROR_NONE;

    /* TODO : cancel tasks */

    /* stop event_data loop */
    _task_manager_loop_availability = 0;

	ENTER_CRITICAL_SECTION(_task_manager_loop_lock);
	WAKE_CONDITION_VARIABLE(_task_available_signal);
	LEAVE_CRITICAL_SECTION(_task_manager_loop_lock);

	/* wait for thread finished */
	THREAD_JOIN(_thread_task_manager_loop);

	_thread_task_manager_loop = 0;

	DELETE_CRITICAL_SECTION(_task_manager_loop_lock);
	DELETE_CONDITION_VARIABLE(_task_available_signal);

	/* Free _active_task_pool */

//FINISH_OFF :

	EM_DEBUG_FUNC_END("err [%d]", err);
    return err;
}

INTERNAL_FUNC int emcore_add_task_to_task_table(email_task_type_t input_task_type, email_task_priority_t input_task_priority, char *input_task_parameter, int input_task_parameter_length, int *output_task_id)
{
	EM_DEBUG_FUNC_BEGIN("input_task_type [%d] input_task_priority [%d] input_task_parameter [%p] input_task_parameter_length [%d] output_task_id [%p]", input_task_type, input_task_priority, input_task_parameter, input_task_parameter_length, output_task_id);
	int err = EMAIL_ERROR_NONE;

	if((err = emstorage_add_task(input_task_type, input_task_priority, input_task_parameter, input_task_parameter_length, false, output_task_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_add_task failed [%d]", err);
		goto FINISH_OFF;
	}

	ENTER_CRITICAL_SECTION(_task_available_lock);
	WAKE_CONDITION_VARIABLE(_task_available_signal);
	LEAVE_CRITICAL_SECTION(_task_available_lock);

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_remove_task_from_task_table(int input_task_id)
{
	EM_DEBUG_FUNC_BEGIN("input_task_id [%d]", input_task_id);
	int err = EMAIL_ERROR_NONE;

	if((err = emstorage_delete_task(input_task_id, true)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_delete_task failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static int emcore_update_task_status_on_task_table(int input_task_id, email_task_status_type_t task_status)
{
	EM_DEBUG_FUNC_BEGIN("input_task_id [%d] task_status [%d]", input_task_id, task_status);
	int err = EMAIL_ERROR_NONE;

	if((err = emstorage_update_task_status(input_task_id, task_status, true)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_update_task_status failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

/*-Task manager loop - end-------------------------------------------------------------*/
