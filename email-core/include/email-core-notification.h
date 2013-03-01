#ifndef __EMAIL_CORE_NOTIFICATION_H__
#define __EMAIL_CORE_NOTIFICATION_H__

#include "email-internal-types.h"
#include "email-core-utils.h"
#include "email-debug-log.h"
#include "email-convert.h"
#include <notification.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

INTERNAL_FUNC int emcore_initialize_notification(void);
INTERNAL_FUNC int emcore_shutdown_notification(void);
INTERNAL_FUNC void emcore_notification_handle(email_noti_on_storage_event transaction_type, int data1, int data2 , char *data3, int data4);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
