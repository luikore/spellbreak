#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "ccan/darray.h"
#include "u.h"

typedef struct {
  uint32_t from;
  uint32_t to;
} CharRange;

enum Indent {
  NOTDENT, INDENT, SAMEDENT, DEDENT
};

typedef struct {
  size_t pos;
  uint32_t ch;
  int last_indent;
} State;

typedef darray(State) StateStack;

typedef struct {
  uint8_t* str;
  size_t size;
  size_t pos;
  uint32_t ch;
  int last_indent;
  // backward tracing
  StateStack state_stack;
} Scanner;

static Scanner scanner_init(char* str, size_t size) {
  Scanner s;
  s.pos = 0;
  s.size = size;
  s.str = (uint8_t*)malloc(size + 1);
  s.ch = 0;
  s.last_indent = -1;
  strncpy(s.str, str, size);

  darray_init(s.state_stack);
  darray_realloc(s.state_stack, 10);

  return s;
}

static void scanner_free(Scanner* sc) {
  free(sc->str);
  darray_free(sc->state_stack);
}

static void scanner_push(Scanner* sc) {
  State s = {sc->pos, sc->ch, sc->last_indent};
  darray_push(sc->state_stack, s);
}

static void scanner_pop(Scanner* sc) {
  State s = darray_pop(sc->state_stack);
  sc->pos = s.pos;
  sc->ch = s.ch;
  sc->last_indent = s.last_indent;
}

static bool scanner_eos(Scanner* sc) {
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

static bool scanner_word_boundary(Scanner* sc, size_t pos) {
  char* p = (char*)(sc->str + pos);
  return (isalnum(p[0]) || p[0] == '_') ^ (isalnum(p[1]) || p[1] == '_');
}

static enum Indent scanner_scan_indent(Scanner* sc) {
  if (!sc->pos) {
    return SAMEDENT;
  }
  // backtrack ' ' until meet bol
  size_t pos = sc->pos - 1;
  char* p = (char*)(sc->str);
  while (pos && p[pos] == ' ') {
    pos--;
  }
  if (pos == 0 || p[pos] == '\n') {
    size_t indent = sc->pos - 1 - pos;
    if (sc->last_indent == -1) {
      sc->last_indent = indent;
      return indent ? INDENT : SAMEDENT;
    } else {
      int r = (sc->last_indent < indent ? INDENT : sc->last_indent > indent ? DEDENT : SAMEDENT);
      sc->last_indent = indent;
      return r;
    }
  } else {
    return NOTDENT;
  }
}

// prereq: ty is one of [ a A z Z b B ]
// returns TRUE (match) or FALSE (not match)
static bool scanner_scan_special(Scanner* sc, char ty) {
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
  return false;
}
