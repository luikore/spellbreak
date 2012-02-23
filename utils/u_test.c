#include <stdio.h>
#include <stdarg.h>
#include "u.h"
#include <assert.h>

void assert_cyclic_trans(uint32_t cp) {
  size_t i;
  UByteSeq bs = u_byte_seq(cp);
  assert(u_code_point(bs.chs, bs.size).ch == cp);
}

void u_test() {
  assert_cyclic_trans(0x0);
  assert_cyclic_trans(0x24);
  assert_cyclic_trans(0xa21);
  assert_cyclic_trans(0x20ac);
  assert_cyclic_trans(0x24b62);
  assert_cyclic_trans(0x24b621);
  assert_cyclic_trans(0x1024b62);
}