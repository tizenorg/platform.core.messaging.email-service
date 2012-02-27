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



/* common header */
#include <stdarg.h>
#include <string.h>

/* open header */
#include <glib.h>
#include <glib/gprintf.h>

/* internal header */
#include "emf-test-utility.h"

/* internal data struct */

void testapp_print (char *fmt, ...)
{
	va_list args = {0};
	va_start(args, fmt);
	vfprintf (stdout, fmt, args);
	va_end (args);
	fflush (stdout);
}

void testapp_show_menu (eEMF_MENU menu)
{
	switch (menu) {
		case EMF_MAIN_MENU:
			testapp_print ("==========================================\n");
			testapp_print ("    Email service test application \n");
			testapp_print ("==========================================\n");
			testapp_print ("1. Account Test\n");
			testapp_print ("2. Message Test\n");
			testapp_print ("3. Mailbox Test\n");
			testapp_print ("5. Rule Test\n");
			testapp_print ("6. Thread Test\n");
			testapp_print ("7. Others\n");
			testapp_print ("0. Exit \n");
			testapp_print ("------------------------------------------\n");
			break;
			
		case EMF_ACCOUNT_MENU:
			testapp_print ("==========================================\n");
			testapp_print ("    ACCOUNT MENU \n");
			testapp_print ("==========================================\n");
			testapp_print (" 1.  Create Account\n");
			testapp_print (" 2.  Update Account\n");
			testapp_print (" 3.  Delete Account\n");
			testapp_print (" 4.  Get Account\n");
			testapp_print (" 5.  Get Account List\n");
			testapp_print (" 7.  Validate Account\n");
			testapp_print (" 8.  Cancel validate Account\n");
			testapp_print (" 9.  Backup All Accounts\n");
			testapp_print (" 10. Restore accounts\n");
			testapp_print (" 11. Get password length of account\n");
			testapp_print (" 13. Clear all notifications\n");
			testapp_print (" 0.  Go to Main Menu\n");
			testapp_print ("------------------------------------------\n");
			break;
			
		case EMF_MESSAGE_MENU:
			testapp_print ("==========================================\n");
			testapp_print ("    MESSAGE MENU\n");
			testapp_print ("==========================================\n");
			testapp_print ("1.  Mail Save\n");
			testapp_print ("2.  Mail Send\n");
			testapp_print ("3.  Mail Update\n");
			testapp_print ("4.  Add Attachment\n");
			testapp_print ("5.  Get Mail \n");
			testapp_print ("6.  Get Mail Info\n");
			testapp_print ("7.  Get Mail Header\n");
			testapp_print ("8.  Get Mail Body\n");
			testapp_print ("9.  Mail Count \n");
			testapp_print ("10. Mail Flag Modify\n");
			testapp_print ("11. Mail Extra Flag Modify\n");
			testapp_print ("12. Download \n");
			testapp_print ("14. Mail Delete\n");
			testapp_print ("16. Download body\n");
			testapp_print ("17. Download attachment\n");
			testapp_print ("18. Load test - mail send\n");
			testapp_print ("19. Mailbox Get List\n");
			testapp_print ("20. Mail Delete All\n");
			testapp_print ("21. Mail move\n");		
			testapp_print ("23. Mail Resend\n");				
			testapp_print ("24. Find mails\n");				
			testapp_print ("26. Count Messages On Sending\n");
			testapp_print ("27. Move all mails to mailbox\n");
			testapp_print ("28. Count Messages with Draft Flag\n");
			testapp_print ("29. Get latest unread mail Id\n");
			testapp_print ("38. Get total email disk usage \n");	
			testapp_print ("39. Send Mail with Option \n");	
			testapp_print ("40. Email Address Format Check\n");
			testapp_print ("41. Get Max Mail Count\n");
			testapp_print ("42. Storage test : (Input : to fields)\n");
			testapp_print ("43. Send mail Cancel\n");
			testapp_print ("44. Cancel Download Body\n");
 			testapp_print ("48. Get thread information\n");
			testapp_print ("49. Get sender list\n");
			testapp_print ("51. Get mail list ex\n");
			testapp_print ("52. Get address info list\n");
			testapp_print ("53. Save HTML mail with inline content\n");
			testapp_print ("55. Set a field of flags\n");
			testapp_print ("56. Add mail\n");
			testapp_print ("57. Update mail\n");
			testapp_print ("0.  Go to Main Menu\n");
			testapp_print ("------------------------------------------\n");
			break;
			
		case EMF_MAILBOX_MENU:
			testapp_print ("==========================================\n");
			testapp_print ("   MAILBOX MENU\n");
			testapp_print ("==========================================\n");
			testapp_print ("1. Add mailbox\n");
			testapp_print ("2. Delete mailbox\n");
			testapp_print ("3. Update mailbox\n");
			testapp_print ("4. Get IMAP mailbox List\n");
			testapp_print ("6. Get Child List for given mailbox \n");
			testapp_print ("7. Get mailbox by mailbox type\n");
			testapp_print ("8. Set mail slot size\n");
			testapp_print ("0. Go to Main Menu\n");
			testapp_print ("------------------------------------------\n");
			break;
			
		case EMF_RULE_MENU:
			testapp_print ("==========================================\n");
			testapp_print ("   RULE MENU\n");
			testapp_print ("==========================================\n");
			testapp_print ("1. Add Rule\n");
			testapp_print ("2. Delete Rule\n");
			testapp_print ("3. Update Rule\n");
			testapp_print ("5. Get Rule\n");
			testapp_print ("6. Get Rule List\n");
			testapp_print ("0. Go to Main Menu\n");
			testapp_print ("------------------------------------------\n");
			break;
			
		case EMF_THREAD_MENU:
			testapp_print ("==========================================\n");
			testapp_print ("   TRHEAD MENU\n");
			testapp_print ("==========================================\n");
			testapp_print ("1. Move Thread\n");
			testapp_print ("2. Delete Thread\n");
			testapp_print ("3. Set Seen Flag of Thread\n");
			testapp_print ("0. Go to Main Menu\n");
			testapp_print ("------------------------------------------\n");
			break;
			
		case EMF_OTHERS_MENU:
			testapp_print ("==========================================\n");
			testapp_print ("   OTHERS\n");
			testapp_print ("==========================================\n");
			testapp_print ("1.  Get Network Status\n");
			testapp_print ("2.  Get Pending Job\n");
			testapp_print ("3.  Cancel Job\n");
			testapp_print ("5.  Set DNET Proper Profile Type\n");
			testapp_print ("6.  Get DNET Proper Profile Type\n");
			testapp_print ("7.  Send mail cancel job\n");
			testapp_print ("8.  Thread View Test\n");
			testapp_print ("11. Print receiving event queue via debug msg\n");
			testapp_print ("12. Create DB full\n");
			testapp_print ("13. Encoding Test\n");
			testapp_print ("14. DTT Test\n");
			testapp_print ("0.  Go to Main Menu\n");
			testapp_print ("------------------------------------------\n");
			break;
			
		default:
			break;
	}
}
void testapp_show_prompt (eEMF_MENU menu)
{
	switch (menu) {
		case EMF_MAIN_MENU:
			testapp_print ("[MAIN]# ");
			break;
			
		case EMF_ACCOUNT_MENU:
			testapp_print ("[ACCOUNT]# ");
			break;
			
		case EMF_MESSAGE_MENU:
			testapp_print ("[MESSAGE]# ");
			break;
			
		case EMF_MAILBOX_MENU:
			testapp_print ("[MAILBOX]# ");
			break;
			
		case EMF_RULE_MENU:
			testapp_print ("[RULE]# ");
			break;
			
		case EMF_THREAD_MENU:
			testapp_print ("[THREAD]# ");
			break;
			
		case EMF_OTHERS_MENU:
			testapp_print ("[OTHERS]# ");
			break;
			
		default:
			break;
	}
}


