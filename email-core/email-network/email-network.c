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

int network_status = 0;

/* _get_network_status - Get the data network status from vconf */

INTERNAL_FUNC void emnetwork_set_network_status(int input_network_status)
{
	EM_DEBUG_FUNC_BEGIN();
	network_status = input_network_status;
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int emnetwork_get_network_status()
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_FUNC_END();
	return network_status;
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

/* Check code for flight mode */
static int _get_flight_mode(int *flight_mode)
{
	EM_DEBUG_FUNC_BEGIN();
	int value;

	if(!flight_mode) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (vconf_get_bool(VCONFKEY_TELEPHONY_FLIGHT_MODE, &value) != 0) {
		EM_DEBUG_EXCEPTION("Failed vconf_get_bool [VCONFKEY_TELEPHONY_FLIGHT_MODE]");
		return EMAIL_ERROR_SYSTEM_FAILURE;
	}

	EM_DEBUG_LOG("flight_mode : [%d]", value);

	*flight_mode = value;

	EM_DEBUG_FUNC_END("status[%d]", value);
	return EMAIL_ERROR_NONE;

}

INTERNAL_FUNC int emnetwork_get_wifi_status(int *wifi_status)
{
 	EM_DEBUG_FUNC_BEGIN();

	int value;

	if(!wifi_status) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (vconf_get_int(VCONFKEY_WIFI_STATE, &value) != 0) {
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
	int sim_status     = VCONFKEY_TELEPHONY_SIM_UNKNOWN;
	int wifi_status    = 0;
	int flight_mode    = 0;
	int err            = EMAIL_ERROR_NONE;
	int ret            = false;

	if (network_status == 0) {
		EM_DEBUG_LOG("VCONFKEY_NETWORK_STATUS is 0");

		if ((err = _get_flight_mode(&flight_mode)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("_get_flight_mode failed : [%d]", err);
			goto FINISH_OFF;
		}

		if (flight_mode) {
			EM_DEBUG_LOG("Flight mode enable");
			err = EMAIL_ERROR_FLIGHT_MODE_ENABLE;
			goto FINISH_OFF;			
		}

		if ((err = emnetwork_get_wifi_status(&wifi_status)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emnetwork_get_wifi_status failed [%d]", err);
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

INTERNAL_FUNC int emnetwork_get_roaming_status(int *output_roaming_status)
{
	EM_DEBUG_FUNC_BEGIN("output_roaming_status [%p]", output_roaming_status);

	int value;

	if(!output_roaming_status) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if (vconf_get_int(VCONFKEY_TELEPHONY_SVC_ROAM, &value)  != 0) {
		EM_DEBUG_EXCEPTION("vconf_get_int failed");
		return EMAIL_ERROR_SYSTEM_FAILURE;
	}

	/* VCONFKEY_TELEPHONY_SVC_ROAM or VCONFKEY_TELEPHONY_SVC_ROAM_OFF */
	if(value == VCONFKEY_TELEPHONY_SVC_ROAM_ON)
		*output_roaming_status = 1;
	else
		*output_roaming_status = 0;
	*output_roaming_status = value;

	EM_DEBUG_FUNC_END("output_roaming_status[%d]", *output_roaming_status);
	return EMAIL_ERROR_NONE;
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
INTERNAL_FUNC long tcp_getbuffer_lnx(TCPSTREAM *stream, unsigned long size, char *s)
{
	struct timeval tmout;
	fd_set readfds;
	int nleave, nread, sret, sockid, maxfd;
	char *p = s;
	int max_timeout = 0;
	sockid = stream->tcpsi;
	maxfd = sockid + 1;
	/* EM_DEBUG_LOG("start sockid %d", sockid); */
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
			return 1;
		}
	}
	else {
		nleave = size;
	}

	while (nleave > 0) {
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
	return 1;
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

	return 1;
}

INTERNAL_FUNC long tcp_soutr_lnx(TCPSTREAM *stream, char *string)
{
	return tcp_sout_lnx(stream, string, EM_SAFE_STRLEN(string));
}

