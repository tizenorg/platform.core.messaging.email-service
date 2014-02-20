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

#ifndef __EMAIL_DBUS_ACTIVATION_H__
#define __EMAIL_DBUS_ACTIVATION_H__

/* standard library header */
#include <glib-object.h>

typedef struct _email_service_t EmailService;
typedef struct _email_service_class_t EmailServiceClass;

#define EMAIL_SERVICE_NAME "org.tizen.email_service"
#define EMAIL_SERVICE_PATH "/org/tizen/email_service"

GType email_service_get_type(void);

struct _email_service_t {
	GObject parent;
	int status;
};

struct _email_service_class_t {
	GObjectClass parent;
};

#define EMAIL_SERVICE_TYPE              (email_service_get_type())
#define EMAIL_SERVICE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), EMAIL_SERVICE_TYPE, EmailService))
#define EMAIL_SERVICE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EMAIL_SERVICE_TYPE, EmailServiceClass))
#define IS_EMAIL_SERVICE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), EMAIL_SERVICE_TYPE))
#define IS_EMAIL_SERVICE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EMAIL_SERVICE_TYPE))
#define EMAIL_SERVICE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EMAIL_SERVICE_TYPE, EmailServiceClass))

typedef enum {
	NFC_SERVICE_ERROR_INVALID_PRAM
} email_servcie_error;

/**
 *     launch the email-service
 */

gboolean email_service_launch(EmailService *email_service, guint *result_val, GError **error);
EXPORT_API int emipc_init_dbus_connection();
EXPORT_API int emipc_launch_email_service();

EXPORT_API void emipc_set_launch_method(int input_launch_method);
EXPORT_API int  emipc_get_launch_method();

#endif /* __EMAIL_DBUS_ACTIVATION_H__ */
