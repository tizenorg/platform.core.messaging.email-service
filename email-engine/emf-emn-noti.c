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



#include <vconf.h>
#include <string.h>
#include "emflib.h"
#include "emf-emn-noti.h"
#include "emf-emn.h"
#include "emf-dbglog.h"
#include <glib.h>

int emf_emn_noti_init(void)
{
	EM_DEBUG_FUNC_BEGIN();

	EM_DEBUG_LOG("SUBSCRIE for User.Push.Email SUCCESSS >>> \n");

	return 0;
}

int emf_emn_noti_quit(gboolean bExtDest)
{
	EM_DEBUG_FUNC_BEGIN();
	return 0;
}

/* EOF */
