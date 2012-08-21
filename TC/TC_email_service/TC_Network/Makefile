###################################################
# add your TestCase List Here
#
# e.g.,
# TC1 = utc_frameworkName_apiName_func
# TC2 = utc_ApplicationLib_recurGetDayOfWeek_func
TARGETS = uts_email_send_mail \
	uts_email_send_saved \
	uts_email_sync_header \
	uts_email_sync_header_for_all_account \
	uts_email_download_body \
	uts_email_download_attachment \
	uts_email_get_imap_folder_list \
	uts_email_cancel_job \
	uts_email_get_network_status \
	uts_email_get_pending_job \
	uts_email_sync_local_activity \

###################################################
# add your Package Config Info Here
#
# e.g.,
# PKGS=calendar
PKGS= email-service

LIBS  = `pkg-config --libs $(PKGS)` 
LIBS += `pkg-config --libs glib-2.0` 
LIBS +=$(TET_ROOT)/lib/tet3/tcm_s.o
LIBS +=-L$(TET_ROOT)/lib/tet3 -ltcm_s
LIBS +=-L$(TET_ROOT)/lib/tet3 -lapi_s

INCS  =  -I. `pkg-config --cflags $(PKGS)`
INCS  +=  -I. `pkg-config --cflags glib-2.0`
INCS  += -I$(TET_ROOT)/inc/tet3 
INCS  += -I$(TET_SUITE_ROOT)/TC_email_service/TC_Utility

UTIL_SRC = $(TET_SUITE_ROOT)/TC_email_service/TC_Utility/uts-email-dummy-utility.c
UTIL_SRC += $(TET_SUITE_ROOT)/TC_email_service/TC_Utility/uts-email-real-utility.c

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

