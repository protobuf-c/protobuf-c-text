#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <protobuf-c/protobuf-c.h>
#include "protobuf-c-text/protobuf-c-text.h"
#include "addressbook.pb-c.h"
#include "broken-alloc.h"

#include <check.h>

START_TEST(test_deep_nesting)
{
  ProtobufCTextError tf_res;
  Tutorial__Recurse *msg;
  
  msg = (Tutorial__Recurse *)protobuf_c_text_from_string(
      &tutorial__recurse__descriptor,
      "id: 1 m {\n"
      " id: 2 m {\n"
      "  id: 3 m {\n"
      "   id: 4 m {\n"
      "    id: 5 m {\n"
      "     id: 6 m {\n"
      "      id: 7 m {\n"
      "       id: 8 m {\n"
      "        id: 9 m {\n"
      "         id: 10 m {\n"
      "          id: 11 m {\n"
      "           id: 12 m {\n"
      "            id: 13 m {\n"
      "             id: 14 m {\n"
      "              id: 15 m {\n"
      "               id: 16\n"
      "              }\n"
      "             }\n"
      "            }\n"
      "           }\n"
      "          }\n"
      "         }\n"
      "        }\n"
      "       }\n"
      "      }\n"
      "     }\n"
      "    }\n"
      "   }\n"
      "  }\n"
      " }\n"
      "}\n",
      &tf_res, NULL);

  if (tf_res.error_txt != NULL) {
    ck_assert_msg(strlen(tf_res.error_txt) == 0,
        "There was an unexpected error: [%d]\"%s\"",
        strlen(tf_res.error_txt), tf_res.error_txt);
  }
  ck_assert_int_eq(tf_res.complete, 1);
  ck_assert_msg(msg != NULL, "Unexpected malloc failure.");
  tutorial__recurse__free_unpacked(msg, NULL);
}
END_TEST

Suite *
suite_odd_messages(void)
{
  Suite *s = suite_create("Protobuf C Text Format - Parsing");
  TCase *tc = tcase_create("Odd messages");

  /* Tests for odd messages. */
  tcase_add_test(tc, test_deep_nesting);
  suite_add_tcase(s, tc);

  return s;
}

int
main(int argc, char *argv[])
{
  int number_failed, exit_code = EXIT_SUCCESS;
  Suite *s_odd = suite_odd_messages();
  SRunner *sr_odd = srunner_create(s_odd);

  /* Run odd message tests. */
  srunner_run_all(sr_odd, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr_odd);
  srunner_free(sr_odd);

  if (number_failed > 0)
    exit_code = EXIT_FAILURE;
  return exit_code;
}
