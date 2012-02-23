#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "darray.h"
#include "u.h"

typedef struct {
  uint32_t from;
  uint32_t to;
} CharRange;

typedef darray(size_t) PosStack;
typedef darray(uint32_t) ChStack;

typedef struct {
  uint8_t* str;
  size_t size;
  size_t pos;
  uint32_t ch;
  // backward tracing
  PosStack pos_stack;
  ChStack ch_stack;
} Scanner;

static Scanner scanner_init(char* str, size_t size) {
  Scanner s;
  s.pos = 0;
  s.size = size;
  s.str = (char*)malloc(size + 1);
  s.ch = 0;
  strncpy(s.str, str, size);

  darray_init(s.pos_stack);
  darray_realloc(s.pos_stack, 10);
  darray_init(s.ch_stack);
  darray_realloc(s.ch_stack, 10);

  return s;
}

static void scanner_free(Scanner* sc) {
  free(sc->str);
  darray_free(sc->pos_stack);
  darray_free(sc->ch_stack);
}

static void scanner_push(Scanner* sc) {
  darray_push(sc->pos_stack, sc->pos);
  darray_push(sc->ch_stack, sc->ch);
}

static void scanner_pop(Scanner* sc) {
  sc->pos = darray_pop(sc->pos_stack);
  sc->ch = darray_pop(sc->ch_stack);
}

static int scanner_eos(Scanner* sc) {
  return sc->pos == sc->size;
}

// -1: eof
// -2: invalid byte sequence
// else byte size scanned (maybe 0)
static int scanner_scan_char(Scanner* sc, uint32_t expected) {
  size_t remaining = sc->size - sc->pos;
  if (!remaining) return -1;
  UCodePoint cp = u_code_point(sc->str + sc->pos, remaining);
  if (!cp.size) return -2;

  if (cp.ch == expected) {
    sc->ch = cp.ch;
    sc->pos += cp.size;
    return cp.size;
  } else {
    return 0;
  }
}

// -1: eof
// -2: invalid byte sequence
// else byte size scanned (maybe 0)
static int scanner_scan_range(Scanner* sc, CharRange* r, size_t size, int negative) {
  size_t remaining = sc->size - sc->pos;
  if (!remaining) return -1;
  UCodePoint cp = u_code_point(sc->str + sc->pos, remaining);
  if (!cp.size) return -2;

  size_t i;
  if (negative) {
    for (i = 0; i < size; i++) {
      if (cp.ch >= r[i].from && cp.ch <= r[i].to) {
        return 0;
      }
    }
    sc->ch = cp.ch;
    sc->pos += cp.size;
    return cp.size;
  } else {
    for (i = 0; i < size; i++) {
      if (cp.ch >= r[i].from && cp.ch <= r[i].to) {
        sc->ch = cp.ch;
        sc->pos += cp.size;
        return cp.size;
      }
    }
    return 0;
  }
}

static int scanner_word_boundary(Scanner* sc, size_t pos) {
  char* p = (char*)(sc->str + pos);
  return (isalnum(p[0]) || p[0] == '_') ^ (isalnum(p[1]) || p[1] == '_');
}

// prereq: ty is one of [ a A z Z b B ]
// returns TRUE (match) or FALSE (not match)
static int scanner_scan_special(Scanner* sc, char ty) {
  switch (ty) {
    case 'a': // line start
      return !sc->pos || sc->str[sc->pos - 1] == '\n';
    case 'A': // whole start
      return !sc->pos;
    case 'z': // line end
      return scanner_eos(sc) || sc->str[sc->pos] == '\n';
    case 'Z': // whole end
      return scanner_eos(sc);
    case 'b': // word boundary
      return   (sc->pos && scanner_word_boundary(sc, sc->pos - 1)) ||
        (!scanner_eos(sc) && scanner_word_boundary(sc, sc->pos));
    case 'B': // not word boundary
      return !((sc->pos && scanner_word_boundary(sc, sc->pos - 1)) ||
        (!scanner_eos(sc) && scanner_word_boundary(sc, sc->pos)));
  }
  return 0;
}
