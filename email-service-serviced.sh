#!/bin/sh

source /etc/tizen-platform.conf

account_count=$(sqlite3 ${TZ_USER_DB}/.email-service.db "select COUNT(*) from mail_account_tbl")
if [ "$(echo "$account_count" | cut -c0-1)" == "0" ]
then
	echo 'There is no account'
elif [ "$(echo "$account_count" | cut -c0-1)" == "" ]
then
	echo 'DB failure'
else
	/usr/bin/email-service-serviced &
fi
