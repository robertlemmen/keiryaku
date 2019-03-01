#include <check.h>
#include <stdlib.h>

#include "check_heap.h"

int main(int argc, char **argv) {
    Suite *s = suite_create("keiryaku");

    suite_add_tcase(s, make_heap_checks());

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

