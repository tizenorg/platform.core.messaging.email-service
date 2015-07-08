#!/bin/sh
source /etc/tizen-platform.conf

#################################################################
# Add preset account information
#################################################################
echo "[EMAIL-SERVICE] Start adding preset account information..."

################################################################################################
MP3_PATH=$TZ_USER_SHARE/settings/Alerts/Over the horizon.mp
# for active sync
vconftool set -t int    db/email_handle/active_sync_handle "0"          -g 6514 -s "email::vconf_active_sync_handle"

# for default mail slot szie
vconftool set -t int    db/private/email-service/slot_size "100"        -g 6514 -s "email::vconf_slot_size"

# for latest mail id
vconftool set -t int    db/private/email-service/latest_mail_id "0"     -g 6514 -s "email::vconf_latest_mail_id"

# for default account id
vconftool set -t int    db/private/email-service/default_account_id "0" -g 6514 -s "email::vconf_default_account_id"

# for default account id
vconftool set -t int    memory/sync/email "0" -i -g 6514                        -s "email::vconf_sync_status"

# for priority send
vconftool set -t string db/private/email-service/noti_ringtone_path "3" -g 6514 -s "email::vconf_ringtone_path"
vconftool set -t int    db/private/email-service/noti_rep_type "0" -g 6514                -s "email::vconf_rep_type"
vconftool set -t bool   db/private/email-service/noti_notification_ticker "0" -g 6514     -s "email::vconf_notification"
vconftool set -t bool   db/private/email-service/noti_display_content_ticker "0" -g 6514  -s "email::vconf_display_content"
vconftool set -t bool   db/private/email-service/noti_badge_ticker "0" -i -g 6514         -s "email::vconf_bagdge"
vconftool set -t int    db/private/email-service/noti_private_id/1 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/2 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/3 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/4 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/5 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/6 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/7 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/8 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/9 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/10 "0" -i -g 6514
vconftool set -t bool   db/private/email-service/noti_vibration_status "0" -g 6514        -s "email::vconf_vibration"
vconftool set -t bool   db/private/email-service/noti_vip_vibration_status "0" -g 6514    -s "email::vconf_vip_vibration"
vconftool set -t bool   db/private/email-service/noti_use_default_ringtone "1" -g 6514    -s "email::vconf_use_default_ringtone"
vconftool set -t bool   db/private/email-service/noti_vip_use_default_ringtone "1" -g 6514    -s "email::vconf_vip_use_default_ringtone"


#################################################################
# Create DB file and tables.
#################################################################
echo "[EMAIL-SERVICE] Creating Email Tables ..."
mkdir -p /opt/usr
mkdir -p ${TZ_USER_DB}

mv ${TZ_SYS_DATA}/email/res/email-service.sql ${TZ_USER_DATA}/email/res/
mv ${TZ_SYS_DATA}/email/res/image/Q02_Notification_email.png ${TZ_USER_DATA}/email/res/
sqlite3 ${TZ_USER_DB}/.email-service.db 'PRAGMA journal_mode = PERSIST;'
sqlite3 ${TZ_USER_DB}/.email-service.db < ${TZ_USER_DATA}/email/res/email-service.sql

echo "[EMAIL-SERVICE] Finish Creating Email Tables."

chgrp 6006 ${TZ_USER_DB}/.email-service.db*
chmod 664 ${TZ_USER_DB}/.email-service.db
chmod 664 ${TZ_USER_DB}/.email-service.db-journal

mkdir -m775 -p ${TZ_USER_DATA}/email/.email_data
chgrp 6006 ${TZ_USER_DATA}/email/.email_data

mkdir -m775 -p ${TZ_USER_DATA}/email/.email_data/tmp
chgrp 6006 ${TZ_USER_DATA}/email/.email_data/tmp

mkdir -p ${TZ_SYS_SHARE}/cert-svc/certs/trusteduser/email
chgrp 6006 ${TZ_SYS_SHARE}/cert-svc/certs/trusteduser/email

