#!/bin/sh
source /etc/tizen-platform.conf

#################################################################
# Create DB file and tables.
#################################################################
echo "[EMAIL-SERVICE] Creating Email Tables ..."
mkdir -p /opt/usr
mkdir -p ${TZ_USER_DB}
mkdir -p ${TZ_USER_DATA}/email/res/

cp ${TZ_SYS_DATA}/email/res/email-service.sql ${TZ_USER_DATA}/email/res/
cp ${TZ_SYS_DATA}/email/res/image/Q02_Notification_email.png ${TZ_USER_DATA}/email/res/
sqlite3 ${TZ_USER_DB}/.email-service.db 'PRAGMA journal_mode = PERSIST;'
sqlite3 ${TZ_USER_DB}/.email-service.db < ${TZ_USER_DATA}/email/res/email-service.sql

echo "[EMAIL-SERVICE] Finish Creating Email Tables."

chmod 664 ${TZ_USER_DB}/.email-service.db
chmod 664 ${TZ_USER_DB}/.email-service.db-journal

#################################################################
# Create data path
#################################################################

mkdir -m775 -p ${TZ_USER_DATA}/email/.email_data

mkdir -m775 -p ${TZ_USER_DATA}/email/.email_data/tmp
mkdir -m775 -p ${TZ_USER_SHARE}/email/.email_data/tmp

