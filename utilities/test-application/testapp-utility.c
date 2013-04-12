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



/* common header */
#include <stdarg.h>
#include <string.h>

/* open header */
#include <glib.h>
#include <glib/gprintf.h>

/* internal header */
#include "testapp-utility.h"

/* internal data struct */

void testapp_print (char *fmt, ...)
{
	va_list args = {0};
	va_start(args, fmt);
	vfprintf (stdout, fmt, args);
	va_end (args);
	fflush (stdout);
}

void testapp_show_menu (eEMAIL_MENU menu)
{
	switch (menu) {
		case EMAIL_MAIN_MENU:
			testapp_print ("==========================================\n");
			testapp_print ("    Email test application \n");
			testapp_print ("==========================================\n");
			testapp_print ("1. Account Test\n");
			testapp_print ("2. Mail Test\n");
			testapp_print ("3. Mailbox Test\n");
			testapp_print ("5. Rule Test\n");
			testapp_print ("6. Thread Test\n");
			testapp_print ("7. Others\n");
			testapp_print ("0. Exit \n");
			testapp_print ("------------------------------------------\n");
			break;
			
		case EMAIL_ACCOUNT_MENU:
			testapp_print ("==========================================\n");
			testapp_print ("    ACCOUNT MENU \n");
			testapp_print ("==========================================\n");
			testapp_print (" 1.  Add account with validation\n");
			testapp_print (" 2.  Update account\n");
			testapp_print (" 3.  Delete account\n");
			testapp_print (" 4.  Get account\n");
			testapp_print (" 5.  Get account list\n");
			testapp_print (" 6.  Update check interval\n");
			testapp_print (" 7.  Validate account\n");
			testapp_print (" 8.  Cancel validate Account\n");
			testapp_print (" 9.  Backup All accounts\n");
			testapp_print (" 10. Restore accounts\n");
			testapp_print (" 11. Get password length of account\n");
			testapp_print (" 12. Query server info\n");
			testapp_print (" 13. Clear all notifications\n");
			testapp_print (" 14. Save default account ID\n");
			testapp_print (" 15. Load default account ID\n");
			testapp_print (" 16. Add certificate\n");
			testapp_print (" 17. Get certificate\n");
			testapp_print (" 18. Delete certificate\n");
			testapp_print (" 19. Add Account\n");
			testapp_print (" 0.  Go to Main Menu\n");
			testapp_print ("------------------------------------------\n");
			break;
			
		case EMAIL_MAIL_MENU:
			testapp_print ("==========================================\n");
			testapp_print ("    MAIL MENU\n");
			testapp_print ("==========================================\n");
			testapp_print ("1.  Get mails\n");
			testapp_print ("2.  Send a mail\n");
			testapp_print ("3.  Get mail list ex\n");
			testapp_print ("4.  Add Attachment\n");
			testapp_print ("5.  Set deleted flag\n");
			testapp_print ("6.  Expunge deleted flagged mails\n");
			testapp_print ("7.  Send read receipt\n");
			testapp_print ("8.  Delete attachment\n");
			testapp_print ("9.  Mail Count \n");
			testapp_print ("10. Move mails to another account\n");
			testapp_print ("11. Send mail with downloading attachment of original mail\n");
			testapp_print ("12. Get mail data\n");
			testapp_print ("13. Schedule sending mail\n");
			testapp_print ("14. Delete a mail \n");
			testapp_print ("15. Update mail attribute \n");
			testapp_print ("16. Download mail body\n");
			testapp_print ("17. Download an attachment\n");
			testapp_print ("20. Delete all mail\n");
			testapp_print ("21. Move Mail \n");
			testapp_print ("23. Resend Mail \n");
			testapp_print ("27. Move all mails to mailbox\n");
			testapp_print ("38. Get total email disk usage \n");	
			testapp_print ("40. Verify Email Address Format\n");
			testapp_print ("41. Get Max Mail Count\n");
			testapp_print ("42. Storage test : (Input : to fields)\n");
			testapp_print ("43. Send mail Cancel\n");
			testapp_print ("44. Cancel Download Body\n");
			testapp_print ("46. Get thread list\n");
 			testapp_print ("48. Get thread information\n");
			testapp_print ("51. Get mail list\n");
			testapp_print ("52. Get address info list\n");
			testapp_print ("55. Set a field of flags\n");
			testapp_print ("56. Add mail\n");
			testapp_print ("57. Update mail\n");
			testapp_print ("58. Search on server\n");
			testapp_print ("59. Add mail to search result table\n");
			testapp_print ("60. Parse mime file\n");
			testapp_print ("61. Write mime file\n");
			testapp_print ("0.  Go to Main Menu\n");
			testapp_print ("------------------------------------------\n");
			break;
			
		case EMAIL_MAILBOX_MENU:
			testapp_print ("==========================================\n");
			testapp_print ("   MAILBOX MENU\n");
			testapp_print ("==========================================\n");
			testapp_print (" 1. Add mailbox\n");
			testapp_print (" 2. Delete mailbox\n");
			testapp_print (" 3. Raname mailbox\n");
			testapp_print (" 4. Get IMAP mailbox List\n");
			testapp_print (" 5. Set local mailbox\n");
			testapp_print (" 6. Delete mailbox ex\n");
			testapp_print (" 7. Get mailbox by mailbox type\n");
			testapp_print (" 8. Set mailbox type\n");
			testapp_print (" 9. Set mail slot size\n");
			testapp_print ("10. Get mailbox list\n");
			testapp_print ("11. Sync mailbox\n");
			testapp_print ("12. Stamp sync time\n");
			testapp_print ("0. Go to Main Menu\n");
			testapp_print ("------------------------------------------\n");
			break;
			
		case EMAIL_RULE_MENU:
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
			
		case EMAIL_THREAD_MENU:
			testapp_print ("==========================================\n");
			testapp_print ("   TRHEAD MENU\n");
			testapp_print ("==========================================\n");
			testapp_print ("1. Move Thread\n");
			testapp_print ("2. Delete Thread\n");
			testapp_print ("3. Set Seen Flag of Thread\n");
			testapp_print ("0. Go to Main Menu\n");
			testapp_print ("------------------------------------------\n");
			break;
			
		case EMAIL_OTHERS_MENU:
			testapp_print ("==========================================\n");
			testapp_print ("   OTHERS\n");
			testapp_print ("==========================================\n");
			testapp_print ("3.  Cancel Job\n");
			testapp_print ("5.  Set DNET Proper Profile Type\n");
			testapp_print ("6.  Get DNET Proper Profile Type\n");
			testapp_print ("7.  Get preview text\n");
			testapp_print ("11. Get task information\n");
			testapp_print ("12. Create DB full\n");
			testapp_print ("13. Encoding Test\n");
			testapp_print ("14. DTT Test\n");
			testapp_print ("15. Show User Message\n");
			testapp_print ("16. Get mime entity in signed file\n");
			testapp_print ("0.  Go to Main Menu\n");
			testapp_print ("------------------------------------------\n");
			break;
			
		default:
			break;
	}
}
void testapp_show_prompt (eEMAIL_MENU menu)
{
	switch (menu) {
		case EMAIL_MAIN_MENU:
			testapp_print ("[MAIN]# ");
			break;
			
		case EMAIL_ACCOUNT_MENU:
			testapp_print ("[ACCOUNT]# ");
			break;
			
		case EMAIL_MAIL_MENU:
			testapp_print ("[MAIL]# ");
			break;
			
		case EMAIL_MAILBOX_MENU:
			testapp_print ("[MAILBOX]# ");
			break;
			
		case EMAIL_RULE_MENU:
			testapp_print ("[RULE]# ");
			break;
			
		case EMAIL_THREAD_MENU:
			testapp_print ("[THREAD]# ");
			break;
			
		case EMAIL_OTHERS_MENU:
			testapp_print ("[OTHERS]# ");
			break;
			
		default:
			break;
	}
}


