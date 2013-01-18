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
 * File: email-network.c
 * Desc: Data network interface
 *
 * Auth:
 *
 * History:
 *	2006.11.17 : created
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <email-storage.h> 
#include <vconf.h>
#include "glib.h"

#include "lnx_inc.h"
#include "utf8aux.h"
#include "c-client.h"
#include "email-debug-log.h"
#include "email-types.h"
#include "email-core-utils.h" 
#include "email-core-mailbox.h"

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
#include "email-core-event.h"
#endif

/* _get_network_status - Get the data network status from vconf */
static int _get_network_status(int *network_status)
{
	EM_DEBUG_FUNC_BEGIN("network_status [%p]", network_status);

	int value = 0;

	if(!network_status) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (vconf_get_int(VCONFKEY_NETWORK_STATUS, &value)) {
		EM_DEBUG_EXCEPTION("Failed vconf_get_int [VCONFKEY_NETWORK_STATUS]");
		return EMAIL_ERROR_SYSTEM_FAILURE;
	}

	*network_status = value;

	EM_DEBUG_FUNC_END("network_status [%d]", value);
	return EMAIL_ERROR_NONE;
}

/* Check code for SIM status */
static int  _get_sim_status(int *sim_status)
{
	EM_DEBUG_FUNC_BEGIN();
	int value;

	if(!sim_status) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (vconf_get_int(VCONFKEY_TELEPHONY_SIM_SLOT, &value)  != 0) {
		EM_DEBUG_EXCEPTION("Failed vconf_get_int [VCONFKEY_TELEPHONY_SIM_SLOT]");
		return EMAIL_ERROR_SYSTEM_FAILURE;
	}

	*sim_status = value;

	EM_DEBUG_FUNC_END("status[%d]", value);
	return EMAIL_ERROR_NONE;
}

static int _get_wifi_status(int *wifi_status)
{
 	EM_DEBUG_FUNC_BEGIN();

	int value;

	if(!wifi_status) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (vconf_get_int(VCONFKEY_WIFI_STATE, &value)  != 0) {
		EM_DEBUG_EXCEPTION("vconf_get_int failed");
		return EMAIL_ERROR_SYSTEM_FAILURE;
	}

	*wifi_status = value;

	EM_DEBUG_FUNC_END("status[%d]", *wifi_status);
	return EMAIL_ERROR_NONE;
}


INTERNAL_FUNC int emnetwork_check_network_status(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int network_status = 0;
	int sim_status     = VCONFKEY_TELEPHONY_SIM_UNKNOWN;
	int wifi_status    = 0;
	int err            = EMAIL_ERROR_NONE;
	int ret            = false;

	if ( (err = _get_network_status(&network_status)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_get_network_status failed [%d]", err);
		goto FINISH_OFF;
	}

	if(network_status == 0) {
		EM_DEBUG_LOG("VCONFKEY_NETWORK_STATUS is 0");

		if ( (err = _get_wifi_status(&wifi_status)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("_get_wifi_status failed [%d]", err);
			goto FINISH_OFF;
		}

		if (wifi_status == 0) {
			EM_DEBUG_LOG("Furthermore, WIFI is off");

			if ( (err = _get_sim_status(&sim_status)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("_get_sim_status failed [%d]", err);
				goto FINISH_OFF;
			}

			if (sim_status != VCONFKEY_TELEPHONY_SIM_INSERTED) {
				EM_DEBUG_EXCEPTION("EMAIL_ERROR_NO_SIM_INSERTED");
				err = EMAIL_ERROR_NO_SIM_INSERTED;
				goto FINISH_OFF;
			}

			EM_DEBUG_EXCEPTION("EMAIL_ERROR_NETWORK_NOT_AVAILABLE");
			err = EMAIL_ERROR_NETWORK_NOT_AVAILABLE;
			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG("Data Network Mode is ON");
	ret = true;

FINISH_OFF: 

	emcore_set_network_error(err);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

INTERNAL_FUNC long emnetwork_callback_ssl_cert_query(char *reason, char *host, char *cert)
{
	EM_DEBUG_FUNC_BEGIN("reason[%s], host[%s], cert[%s]", reason, host, cert);
	int ret = 1;

	/*  FIXME : add user interface for Accept or Reject */
	/*  1:Accept - ignore this certificate error */
	/*  0:Reject - stop ssl connection */
	ret = 1;

	EM_DEBUG_FUNC_END("ret[%s]", ret ? "ignore error" : "stop connection");
	return ret;
}

/* ------ socket read/write handle ---------------------------------------- */
int _g_canceled = 0;
/*  TCPSTREAM = SENDSTREAM->netstream->stream; socket-id = TCPSTREAM->tcpsi, tcpso; */
/*  sockid_in  = ((TCPSTREAM*)send_stream->netstream->stream)->tcpsi; */
/*  sockid_out = ((TCPSTREAM*)send_stream->netstream->stream)->tcpso; */

INTERNAL_FUNC long tcp_getbuffer_lnx(TCPSTREAM *stream, unsigned long size, char *s)
{
	struct timeval tmout;
	fd_set readfds;
	int nleave, nread, sret, sockid, maxfd;
	char *p = s;
	int max_timeout = 0;
	sockid = stream->tcpsi;
	maxfd = sockid + 1;
/* EM_DEBUG_LOG("start sockid %d", sockid);	*/
	if (sockid < 0) return 0;

	if (stream->ictr > 0) {
		int copy_sz;
		if (stream->ictr >= size) {
			memcpy(p, stream->iptr, size);
			copy_sz = size;
		}
		else {
			memcpy(p, stream->iptr, stream->ictr);
			copy_sz = stream->ictr;
		}
		p += copy_sz;
		nleave = size - copy_sz;
		stream->iptr += copy_sz;
		stream->ictr -= copy_sz;

		if (nleave <= 0) {
			*p = '\0';
			/* EM_DEBUG_LOG("end");	*/
			return 1;
		}
	}
	else {
		nleave = size;
	}

	while (nleave > 0) {
#ifdef TEST_CANCEL_JOB
		if (!emcore_check_thread_status()) {
			EM_DEBUG_EXCEPTION("thread canceled");
			tcp_abort(stream);
			goto JOB_CANCEL;
		}
#endif
		/* if (_g_canceled){ */
		/* 	EM_DEBUG_LOG1("thread canceled\n"); */
		/* 	tcp_abort(stream); */
		/* 	return 0; */
		/* } */

		tmout.tv_usec = 0;
		tmout.tv_sec = 1;
		FD_ZERO(&readfds);
		FD_SET(sockid, &readfds);

		sret = select(maxfd, &readfds, NULL, NULL, &tmout);
		if (sret < 0) {
			EM_DEBUG_EXCEPTION("select error[%d]\n", errno);
			tcp_abort(stream);
			return 0;
		}
		else if (!sret) {
			if (max_timeout >= 5) {
				EM_DEBUG_EXCEPTION("max select timeout %d", max_timeout);
				emcore_set_network_error(EMAIL_ERROR_NO_RESPONSE);
				return 0;
			}
			EM_DEBUG_EXCEPTION("%d select timeout", max_timeout);
			++max_timeout;
			continue;
		}

		nread = read(sockid, p, nleave);
		if (nread < 0) {
			EM_DEBUG_EXCEPTION("socket read error");
/* 			if (errno == EINTR) continue; */
			tcp_abort(stream);
			return 0;
		}

		if (!nread) {
			EM_DEBUG_EXCEPTION("socket read no data");
			tcp_abort(stream);
			return 0;
		}

		p += nread;
		nleave -= nread;
	}

	*p = '\0';

	if (_g_canceled) {
		EM_DEBUG_EXCEPTION("thread canceled");
		tcp_abort(stream);
		return 0;
	}

	return 1;
/* 	EM_DEBUG_LOG("end"); */
#ifdef TEST_CANCEL_JOB
JOB_CANCEL:
	return 0;
#endif
}

long tcp_getdata_lnx(TCPSTREAM *stream)
{
	struct timeval tmout;
	fd_set readfds;
	int nread, sret, sockid, maxfd;
	int max_timeout = 0;
	
	sockid = stream->tcpsi;
	maxfd = sockid + 1;
	
	/* EM_DEBUG_LOG("start sockid %d", sockid);	*/
	if (sockid < 0) return false;
	
	while (stream->ictr < 1)  {
#ifdef TEST_CANCEL_JOB
		if (!emcore_check_thread_status())  {
			EM_DEBUG_EXCEPTION("thread canceled...");
			tcp_abort(stream);
			goto JOB_CANCEL;
		}
#endif
		
		/* if (_g_canceled){ */
		/* 	EM_DEBUG_LOG1("thread canceled\n"); */
		/* 	tcp_abort(stream); */
		/* 	return 0; */
		/* } */
		
		tmout.tv_usec = 0;/* 1000*10; */
		tmout.tv_sec = 1;
		
		FD_ZERO(&readfds);
		FD_SET(sockid, &readfds);
		
		sret = select(maxfd, &readfds, NULL, NULL, &tmout);
		
		if (sret < 0) {
			EM_DEBUG_LOG("select error[%d]", errno);
			
			tcp_abort(stream);
			return false;
		}
		else if (!sret) {
			if (max_timeout >= 50) {
				EM_DEBUG_EXCEPTION("max select timeout %d", max_timeout);
				
				emcore_set_network_error(EMAIL_ERROR_NO_RESPONSE);
				return false;
			}
			
			EM_DEBUG_EXCEPTION("%d select timeout", max_timeout);
			
			++max_timeout;
			continue;
		}
		
		if ((nread = read(sockid, stream->ibuf, BUFLEN)) < 0) {
			EM_DEBUG_EXCEPTION("socket read failed...");
			
			emcore_set_network_error(EMAIL_ERROR_SOCKET_FAILURE);
			
			/* if (errno == EINTR) contine; */
			tcp_abort(stream);
			return false;
		}
		
		if (!nread) {
			EM_DEBUG_EXCEPTION("socket read no data...");
			
			emcore_set_network_error(EMAIL_ERROR_INVALID_RESPONSE);
			
			tcp_abort(stream);
			return false;
		}
		
		stream->ictr = nread;
		stream->iptr = stream->ibuf;
	}
	
	if (_g_canceled) {
		EM_DEBUG_EXCEPTION("\t thread canceled...\n");
		
		tcp_abort(stream);
		return false;
	}
	
	/* EM_DEBUG_LOG("end");	*/
	return true;
	
#ifdef TEST_CANCEL_JOB
JOB_CANCEL:
	return false;
#endif
}

INTERNAL_FUNC long tcp_sout_lnx(TCPSTREAM *stream, char *string, unsigned long size)
{
	struct timeval tmout;
	fd_set writefds;
	int sret, nwrite, maxfd, sockid;
	int max_timeout = 0;
	
	sockid = stream->tcpso;
	maxfd = sockid + 1;
/* EM_DEBUG_LOG("start sockid %d", sockid); */
	if (sockid < 0) return 0;

	while (size > 0) {
#ifdef TEST_CANCEL_JOB
		if (!emcore_check_thread_status()) {
			EM_DEBUG_EXCEPTION("thread canceled");
			tcp_abort(stream);
			goto JOB_CANCEL;
		}
#endif
		/* if (_g_canceled){ */
		/* 	EM_DEBUG_LOG1("thread canceled"); */
		/* 	tcp_abort(stream); */
		/* 	return 0; */
		/* } */

		tmout.tv_usec = 0;
		tmout.tv_sec = 1;
		FD_ZERO(&writefds);
		FD_SET(sockid, &writefds);

		sret = select(maxfd, NULL, &writefds, NULL, &tmout);
		if (sret < 0) {
			EM_DEBUG_LOG("select error[%d]", errno);
			tcp_abort(stream);
			return 0;
		}
		else if (!sret) {
			if (max_timeout >= 50) {
				EM_DEBUG_EXCEPTION("max select timeout %d", max_timeout);
				emcore_set_network_error(EMAIL_ERROR_NO_RESPONSE);
				return 0;
			}
			EM_DEBUG_EXCEPTION("%d select timeout", max_timeout);
			++max_timeout;
			continue;
		}

		if ((nwrite = write(sockid, string, size)) < 0) { /*prevent 22857*/
			EM_DEBUG_EXCEPTION("socket write error");
/* 			if (errno == EINTR) continue; */
			tcp_abort(stream);
			return 0;
		}

		if (!nwrite) {
			EM_DEBUG_EXCEPTION("socket write no data");
			tcp_abort(stream);
			return 0;
		}

		string += nwrite;
		size -= nwrite;
	}

	if (_g_canceled) {
		EM_DEBUG_EXCEPTION("thread canceled");
		tcp_abort(stream);
		return 0;
	}
/* EM_DEBUG_LOG("end"); */
	return 1;

#ifdef TEST_CANCEL_JOB
JOB_CANCEL:
	return 0;
#endif
}

INTERNAL_FUNC long tcp_soutr_lnx(TCPSTREAM *stream, char *string)
{
	return tcp_sout_lnx(stream, string, EM_SAFE_STRLEN(string));
}

