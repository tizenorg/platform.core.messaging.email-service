#include <glib.h>
#include <gmime/gmime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "testapp-utility.h"
#include "testapp-gmime.h"


#define TEST_EML_PATH "/tmp/test.eml"

static void print_depth(int depth)
{
	int i;
	for (i = 0; i < depth; i++)
		testapp_print("    \n");
}

static void print_mime_struct(GMimeObject *part, int depth)
{
	const GMimeContentType *type;
	GMimeMultipart *multipart;
	GMimeObject *subpart;
	int i, n;

	print_depth(depth);
	type = g_mime_object_get_content_type(part);
	testapp_print("Content-Type: %s/%s\n", type->type, type->subtype);

	if (GMIME_IS_MULTIPART(part)) {
		multipart = (GMimeMultipart *)part;

		n = g_mime_multipart_get_count(multipart);
		for (i = 0; i < n; i++) {
			subpart = g_mime_multipart_get_part(multipart, i);
			print_mime_struct(subpart, depth + 1);
			g_object_unref(subpart);
		}
	}
}

static void
test_eml_parsing_foreach_callback(GMimeObject *parent, GMimeObject *part, gpointer user_data)
{
	int *cnt= user_data;

	(*cnt)++;

	if (GMIME_IS_PART(part)) {
		testapp_print ("Part\n");
		GMimePart *leaf_part = NULL;
		leaf_part = (GMimePart *)part;

		testapp_print("Content ID:%s\n", g_mime_part_get_content_id(leaf_part));
		testapp_print("Description:%s\n", g_mime_part_get_content_description(leaf_part));
		testapp_print("MD5:%s\n", g_mime_part_get_content_md5(leaf_part));
		testapp_print("Location:%s\n", g_mime_part_get_content_location(leaf_part));

		GMimeContentEncoding enc = g_mime_part_get_content_encoding(leaf_part);
		switch(enc) {
		case GMIME_CONTENT_ENCODING_DEFAULT:
			testapp_print("Encoding:ENCODING_DEFAULT\n");
			break;
		case GMIME_CONTENT_ENCODING_7BIT:
			testapp_print("Encoding:ENCODING_7BIT\n");
			break;
		case GMIME_CONTENT_ENCODING_8BIT:
			testapp_print("Encoding:ENCODING_8BIT\n");
			break;
		case GMIME_CONTENT_ENCODING_BINARY:
			testapp_print("Encoding:ENCODING_BINARY\n");
			break;
		case GMIME_CONTENT_ENCODING_BASE64:
			testapp_print("Encoding:ENCODING_BASE64\n");
			break;
		case GMIME_CONTENT_ENCODING_QUOTEDPRINTABLE:
			testapp_print("Encoding:ENCODING_QUOTEDPRINTABLE\n");
			break;
		case GMIME_CONTENT_ENCODING_UUENCODE:
			testapp_print("Encoding:ENCODING_UUENCODE\n");
			break;

		default:
			break;
		}

		testapp_print("File name:%s\n\n\n", g_mime_part_get_filename(leaf_part));
		GMimeDataWrapper *data = g_mime_part_get_content_object(leaf_part);

		if (data) {
			char tmp_path[50] = {0,};
			snprintf(tmp_path, sizeof(tmp_path), "/tmp/gmime_content%d", *cnt);

			int fd;
			if ((fd = open(tmp_path, O_RDWR|O_CREAT, 0)) == -1) {
				testapp_print ("open fail\n");
			}
			GMimeStream *out_stream;
			out_stream = g_mime_stream_fs_new(fd);
			g_mime_data_wrapper_write_to_stream(data, out_stream);
			g_object_unref(out_stream);
		}
	}
}

static gboolean testapp_test_gmime_eml_parsing(void)
{
	GMimeMessage *message;
	GMimeParser *parser;
	GMimeStream *stream;
	int fd;
	int count = 0;

	if ((fd = open(TEST_EML_PATH, O_RDONLY, 0)) == -1) {
		testapp_print ("open fail\n");
		return 0;
	}

	/* init the gmime library */
	g_mime_init(0);

	/* create a stream to read from the file descriptor */
	stream = g_mime_stream_fs_new(fd);

	/* create a new parser object to parse the stream */
	parser = g_mime_parser_new_with_stream(stream);

	/* unref the stream (parser owns a ref, so this object does not actually get free'd until we destroy the parser) */
	g_object_unref(stream);

	/* parse the message from the stream */
	message = g_mime_parser_construct_message(parser);

	/* free the parser (and the stream) */
	g_object_unref(parser);

	testapp_print("Sender:%s\n", g_mime_message_get_sender(message));
	testapp_print("Reply To:%s\n", g_mime_message_get_reply_to(message));
	testapp_print("Subject:%s\n", g_mime_message_get_subject(message));
	testapp_print("Date:%s\n", g_mime_message_get_date_as_string(message));
	testapp_print("Message ID:%s\n", g_mime_message_get_message_id(message));

	GMimeObject *po = (GMimeObject *)message;
	testapp_print("Header String:%s\n\n\n\n", g_mime_header_list_to_string(po->headers));

	g_mime_message_foreach(message, test_eml_parsing_foreach_callback, &count);
}

static gboolean testapp_test_interpret_command (int menu_number)
{
	gboolean go_to_loop = TRUE;

	switch (menu_number) {
		case 1:
			testapp_test_gmime_eml_parsing();
			break;

		case 0:
			go_to_loop = FALSE;
			break;
		default:
			break;
	}

	return go_to_loop;
}

void testapp_gmime_main()
{
	gboolean go_to_loop = TRUE;
	int menu_number = 0;

	while (go_to_loop) {
		testapp_show_menu (EMAIL_GMIME_MENU);
		testapp_show_prompt (EMAIL_GMIME_MENU);

		result_from_scanf = scanf ("%d", &menu_number);

		go_to_loop = testapp_test_interpret_command (menu_number);
	}
}
