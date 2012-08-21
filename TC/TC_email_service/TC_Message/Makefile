###################################################
# add your TestCase List Here
#
# e.g.,
# TC1 = utc_frameworkName_apiName_func
# TC2 = utc_ApplicationLib_recurGetDayOfWeek_func
TARGETS = uts-email-add-message \
	uts-email-add-attachment \
	uts-email-update-message \
	uts-email-count-message \
	uts-email-delete-message \
	uts-email-delete-attachment \
	uts-email-get-info \
	uts-email-free-mail-info \
	uts-email-get-disk-space-usage \
	uts-email-get-max-mail-count \
	uts-email-cancel-send-mail \
	uts-email-send-retry \
	uts-email-get-mail-list-ex \
	uts-email-get-attachment-info \
	uts-email-free-attachment-info \
	uts-email-move-mail-to-folder \
	uts-email-move-all-mails-to-folder \
	uts-email-get-mailbox-list \
	uts-email-delete-all-message-in-folder \
	uts-email-free-mailbox \
	uts-email-create-db-full \
	uts-email-clear-mail-data \

#	uts-email-query-message \
	uts-email-add-query-message \
	uts-email-release-query-message \
	uts-email-get-list-by-emails \
	uts-email-get-mail-list-for-thread-view \
	uts-email-get-mail-list-of-thread \
	uts-email-get-mail-list-of-sender \
	uts-email-get-thread-information \
	uts-email-searched-mail-list \
	uts-email-get-searched-mail-list_by-count \
	uts-email-get-mail-list-by-count \
	uts-email-convert-mailbox-name-to-utf8 \
	uts-email-convert-text-utf8 \
	uts-email-convert-utf8-text \
	uts-email-get-mail-during-sync \
	uts-email-count-message-with-searching \
	uts-email-get-rfc-822 \
	uts-email-parse-rfc-822 \
	uts-email-free-envelope \
	uts-email-get-temp-file-size \
	uts-email-get-mail-list (deprecated function) 
###################################################
# add your Package Config Info Here
#
# e.g.,
# PKGS=calendar
PKGS= email-service

LIBS = `pkg-config --libs $(PKGS)`
LIBS += `pkg-config --libs glib-2.0`
LIBS +=$(TET_ROOT)/lib/tet3/tcm_s.o
LIBS +=-L$(TET_ROOT)/lib/tet3 -ltcm_s
LIBS +=-L$(TET_ROOT)/lib/tet3 -lapi_s

INCS  =  `pkg-config --cflags $(PKGS)`
INCS  += `pkg-config --cflags glib-2.0`
INCS  += -I$(TET_ROOT)/inc/tet3 
INCS  += -I$(TET_SUITE_ROOT)/TC_Utility/include

UTIL_SRC = $(TET_SUITE_ROOT)/TC_Utility/uts-email-dummy-utility.c
UTIL_SRC += $(TET_SUITE_ROOT)/TC_Utility/uts-email-real-utility.c

CFLAGS = $(INCS)
CFLAGS += -D_TETWARE_MODE
LDFLAGS = $(LIBS)

###################################################
# Modify here 
# depending on the Test Case you want to build
# 
# e.g.,
# TCLIST = $(TC1) $(TC2)
all : $(TARGETS)

$(TARGETS) : %: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean :
	rm -rf $(TARGETS)

