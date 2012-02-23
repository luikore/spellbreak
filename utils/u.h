#pragma once
#include <stdint.h>
#include <stdlib.h>

/* http://en.wikipedia.org/wiki/Utf-8#Description */

typedef struct {
  size_t size;
  uint32_t ch;
} UCodePoint;

typedef struct {
  uint8_t size;
  uint8_t chs[6];
} UByteSeq;

static UCodePoint u_code_point(uint8_t* p, size_t remaining) {
  size_t i;
  UCodePoint res = {0, 0};
  if (remaining == 0) {
    return res;
  }
  res.ch = p[0];

  if (res.ch < 128) {
    res.size = 1;
    return res;
  } else if (res.ch >> 5 == 6) {
    if (remaining >= 2) {
      res.ch &= 31;
      res.size = 2;
    } else {
      return res;
    }
  } else if (res.ch >> 4 == 14) {
    if (remaining >= 3) {
      res.ch &= 15;
      res.size = 3;
    } else {
      return res;
    }
  } else if (res.ch >> 3 == 30) {
    if (remaining >= 4) {
      res.ch &= 7;
      res.size = 4;
    } else {
      return res;
    }
  } else if (res.ch >> 2 == 62) {
    if (remaining >= 5) {
      res.ch &= 3;
      res.size = 5;
    } else {
      return res;
    }
  } else if (res.ch >> 1 == 126) {
    if (remaining >= 6) {
      res.ch &= 1;
      res.size = 6;
    } else {
      return res;
    }
  } else {
    return res;
  }

  // scan the char
  for (i=1; i<res.size; i++) {
    res.ch <<= 6;
    if ((p[i] >> 6) != 2) {
      res.size = 0;
      break;
    }
    res.ch |= (p[i] & 63);
  }
  return res;
}

static UByteSeq u_byte_seq(uint32_t cp) {
  UByteSeq bs;

  if (cp <= 0x7f) {
    bs.chs[0] = (uint8_t)cp;
    bs.size = 1;

  } else if (cp <= 0x7ffu) {
    bs.chs[0] = (uint8_t)(192 | (cp >> 6));
    bs.chs[1] = (uint8_t)(128 | (cp & 63));
    bs.size = 2;

  } else if (cp <= 0xffffu) {
    bs.chs[0] = (uint8_t)(224 | (cp >> 12));
    bs.chs[1] = (uint8_t)(128 | ((cp >> 6) & 63));
    bs.chs[2] = (uint8_t)(128 | (cp & 63));
    bs.size = 3;

  } else if (cp <= 0x1fffffu) {
    bs.chs[0] = (uint8_t)(240 | (cp >> 18));
    bs.chs[1] = (uint8_t)(128 | ((cp >> 12) & 63));
    bs.chs[2] = (uint8_t)(128 | ((cp >> 6) & 63));
    bs.chs[3] = (uint8_t)(128 | (cp & 63));
    bs.size = 4;

  } else if (cp <= 0x3ffffffu) {
    bs.chs[0] = (uint8_t)(248 | (cp >> 24));
    bs.chs[1] = (uint8_t)(128 | ((cp >> 18) & 63));
    bs.chs[2] = (uint8_t)(128 | ((cp >> 12) & 63));
    bs.chs[3] = (uint8_t)(128 | ((cp >> 6) & 63));
    bs.chs[4] = (uint8_t)(128 | (cp & 63));
    bs.size = 5;

  } else if (cp <= 0x7fffffffu) {
    bs.chs[0] = (uint8_t)(252 | (cp >> 30));
    bs.chs[1] = (uint8_t)(128 | ((cp >> 24) & 63));
    bs.chs[2] = (uint8_t)(128 | ((cp >> 18) & 63));
    bs.chs[3] = (uint8_t)(128 | ((cp >> 12) & 63));
    bs.chs[4] = (uint8_t)(128 | ((cp >> 6) & 63));
    bs.chs[5] = (uint8_t)(128 | (cp & 63));
    bs.size = 6;

  } else {
    bs.size = 0; // out of range
  }
  return bs;
}
