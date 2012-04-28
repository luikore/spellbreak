#include <string.h>
#include "scanner.hpp"
#include <assert.h>

static void assert_cyclic_trans(uint32_t cp) {
  Scanner sc = Scanner("", 0);
  Scanner::ByteSeq bs = sc.byte_seq(cp);
  assert(sc.code_point(bs.chs, bs.size).ch == cp);
}

static void test_code_point() {
  assert_cyclic_trans(0x0);
  assert_cyclic_trans(0x24);
  assert_cyclic_trans(0xa21);
  assert_cyclic_trans(0x20ac);
  assert_cyclic_trans(0x24b62);
  assert_cyclic_trans(0x24b621);
  assert_cyclic_trans(0x1024b62);
}

static void test_ascii_scan() {
  Scanner sc = Scanner("abcd", 4);
  assert(sc.scan_char('b') == 0);
  assert(sc.pos == 0);
  assert(sc.scan_char('a') == 1);
  assert(sc.pos == 1);
  assert(sc.scan_char('b') == 1);
  assert(sc.pos == 2);
  assert(sc.scan_char('c') == 1);
  assert(sc.pos == 3);
  assert(sc.scan_char('b') == 0);
  assert(sc.pos == 3);
  assert(sc.scan_char('d') == 1);
  assert(sc.pos == 4);
  assert(sc.scan_char('d') == -1);
  assert(sc.pos == 4);
}

static void test_utf_scan() {
  const char* str = "\xe6\x88\x91\xe4\xbb\xac\xe6\x98\xaf\xe5\x82\xbb\xe9\x80\xbc";
  Scanner sc = Scanner(str, strlen(str));
  assert(sc.scan_char(25105) == 3);
  assert(sc.scan_char(20204) == 3);
  assert(sc.scan_char(26159) == 3);
  assert(sc.scan_char(20667) == 3);
  assert(sc.scan_char(36924) == 3);
  assert(sc.eos());
}

void scanner_test() {
  test_code_point();
  test_ascii_scan();
  test_utf_scan();
}
