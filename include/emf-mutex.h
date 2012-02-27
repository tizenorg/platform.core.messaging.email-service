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


#ifndef _EMF_MUTEX_H_
#define _EMF_MUTEX_H_

#include <sys/time.h>

class Mutex
{
public:
	Mutex(){ pthread_mutex_init(&m, NULL); }
	void lock(){ pthread_mutex_lock(&m); }
	void unlock(){ pthread_mutex_unlock(&m); }
	pthread_mutex_t* pMutex() { return &m; }	

private:
	pthread_mutex_t m;
};

class MutexLocker
{
public:
	MutexLocker(Mutex& mx) {
		pm = &mx;
		pm->lock();
	}

	~MutexLocker() {
		pm->unlock();
	}

private:
	Mutex *pm;
};

class CndVar
{
public:
	CndVar(){ pthread_cond_init(&c, NULL); }
	void wait(pthread_mutex_t* m) { pthread_cond_wait(&c, m); }
	int timedwait(pthread_mutex_t* m, int sec)  { 
		struct timeval now = {0};
		struct timespec timeout = {0};
		gettimeofday(&now, NULL);
		timeout.tv_sec = now.tv_sec+sec;
		timeout.tv_nsec = now.tv_usec;
		int retcode = pthread_cond_timedwait(&c, m, &timeout);
		return retcode;
	}
	int signal(){ return pthread_cond_signal(&c); }
	int broadcast(){ return pthread_cond_broadcast(&c); }

private:
	pthread_cond_t c;
};


#endif

