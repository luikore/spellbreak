#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stack>

struct Scanner {

  /* http://en.wikipedia.org/wiki/Utf-8#Description */

  struct CodePoint {
    size_t size;
    uint32_t ch;
  };

  struct ByteSeq {
    uint8_t size;
    uint8_t chs[6];
  };

  struct CharRange {
    uint32_t from;
    uint32_t to;
  };

  enum Indent {
    NOTDENT, INDENT, SAMEDENT, DEDENT
  };

  struct State {
    size_t pos;
    uint32_t ch;
    int last_indent;
  };

  uint8_t* str;
  size_t size;
  // states
  size_t pos;
  uint32_t ch;
  int last_indent;
  // backward tracing
  std::stack<State> state_stack;

  Scanner(const char* _str, size_t _size) {
    str = (uint8_t*)malloc(_size + 1);
    size = _size;
    strncpy((char*)str, _str, size);

    pos = 0;
    ch = 0;
    last_indent = -1;
  }

  void push() {
    State s = {pos, ch, last_indent};
    state_stack.push(s);
  }

  void pop() {
    State s = state_stack.top();
    state_stack.pop();
    pos = s.pos;
    ch = s.ch;
    last_indent = s.last_indent;
  }

  bool eos() {
    return pos == size;
  }

  // -1: eof
  // -2: invalid byte sequence
  // else byte size scanned (maybe 0)
  int scan_char(uint32_t expected) {
    size_t remaining = size - pos;
    if (!remaining) return -1;
    CodePoint cp = code_point(str + pos, remaining);
    if (!cp.size) return -2;

    if (cp.ch == expected) {
      ch = cp.ch;
      pos += cp.size;
      return cp.size;
    } else {
      return 0;
    }
  }

  // -1: eof
  // -2: invalid byte sequence
  // else byte size scanned (maybe 0)
  int scan_range(CharRange* r, size_t size, int negative) {
    size_t remaining = size - pos;
    if (!remaining) return -1;
    CodePoint cp = code_point(str + pos, remaining);
    if (!cp.size) return -2;

    size_t i;
    if (negative) {
      for (i = 0; i < size; i++) {
        if (cp.ch >= r[i].from && cp.ch <= r[i].to) {
          return 0;
        }
      }
      ch = cp.ch;
      pos += cp.size;
      return cp.size;
    } else {
      for (i = 0; i < size; i++) {
        if (cp.ch >= r[i].from && cp.ch <= r[i].to) {
          ch = cp.ch;
          pos += cp.size;
          return cp.size;
        }
      }
      return 0;
    }
  }

  bool word_boundary(size_t on_pos) {
    char* p = (char*)(str + on_pos);
    return (isalnum(p[0]) || p[0] == '_') ^ (isalnum(p[1]) || p[1] == '_');
  }

  Indent scan_indent() {
    if (!pos) {
      return SAMEDENT;
    }
    // backtrack ' ' until meet bol
    size_t pos = this->pos - 1;
    char* p = (char*)str;
    while (pos && p[pos] == ' ') {
      pos--;
    }
    if (pos == 0 || p[pos] == '\n') {
      size_t indent = this->pos - 1 - pos;
      if (last_indent == -1) {
        last_indent = indent;
        return indent ? INDENT : SAMEDENT;
      } else {
        Indent r = (last_indent < indent ? INDENT : last_indent > indent ? DEDENT : SAMEDENT);
        last_indent = indent;
        return r;
      }
    } else {
      return NOTDENT;
    }
  }

  // prereq: ty is one of [ a A z Z b B ]
  // returns TRUE (match) or FALSE (not match)
  bool scan_special(char ty) {
    switch (ty) {
      case 'a': // line start
        return !pos || str[pos - 1] == '\n';
      case 'A': // whole start
        return !pos;
      case 'z': // line end
        return eos() || str[pos] == '\n';
      case 'Z': // whole end
        return eos();
      case 'b': // word boundary
        return   (pos && word_boundary(pos - 1)) ||
          (!eos() && word_boundary(pos));
      case 'B': // not word boundary
        return !((pos && word_boundary(pos - 1)) ||
          (!eos() && word_boundary(pos)));
    }
    return false;
  }

  CodePoint code_point(uint8_t* p, size_t remaining) {
    size_t i;
    CodePoint res = {0, 0};
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

  ByteSeq byte_seq(uint32_t cp) {
    ByteSeq bs;

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
};
