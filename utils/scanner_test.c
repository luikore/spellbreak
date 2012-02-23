#include <string.h>
#include "scanner.h"
#include <assert.h>

static void test_ascii_scan() {
  Scanner sc = scanner_init("abcd", 4);
  assert(scanner_scan_char(&sc, 'b') == 0);
  assert(sc.pos == 0);
  assert(scanner_scan_char(&sc, 'a') == 1);
  assert(sc.pos == 1);
  assert(scanner_scan_char(&sc, 'b') == 1);
  assert(sc.pos == 2);
  assert(scanner_scan_char(&sc, 'c') == 1);
  assert(sc.pos == 3);
  assert(scanner_scan_char(&sc, 'b') == 0);
  assert(sc.pos == 3);
  assert(scanner_scan_char(&sc, 'd') == 1);
  assert(sc.pos == 4);
  assert(scanner_scan_char(&sc, 'd') == -1);
  assert(sc.pos == 4);
  scanner_free(&sc);
}

static void test_utf_scan() {
  char* str = "\xe6\x88\x91\xe4\xbb\xac\xe6\x98\xaf\xe5\x82\xbb\xe9\x80\xbc";
  Scanner sc = scanner_init(str, strlen(str));
  assert(scanner_scan_char(&sc, 25105) == 3);
  assert(scanner_scan_char(&sc, 20204) == 3);
  assert(scanner_scan_char(&sc, 26159) == 3);
  assert(scanner_scan_char(&sc, 20667) == 3);
  assert(scanner_scan_char(&sc, 36924) == 3);
  assert(scanner_eos(&sc));
}

void scanner_test() {
  test_ascii_scan();
  test_utf_scan();
}
