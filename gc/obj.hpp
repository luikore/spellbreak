#pragma once

#include <vector>
#include <map>
#include <tr1/unordered_map>
#include <assert.h>

typedef void* Val;
typedef std::vector<Val> ValArray;
typedef std::tr1::unordered_map<Val, Val> ValHash;
typedef std::map<Val, Val> ValTreeMap;
#define IS_PTR(v) (v && ((((uint64_t)(v)) >> 48) == 0))

struct Obj {

	static const uint8_t WHITE = 0;
	static const uint8_t GRAY = 1;
	static const uint8_t BLACK = 2;
	static const int8_t ARRAY_LIKE = -3;
	static const int8_t HASH_LIKE = -2;
	static const int8_t TREE_MAP_LIKE = -1;

	struct Tag {
		uint8_t pad[6];	// pointer to next
		uint8_t color;	// 0:white, 2:black
		int8_t storage;	// -3:array_like, -2:hash_like, -1:tree_map_like, 0:no_type, 1..127:tuple_like
	};

	union {
		uint64_t p;
		Tag tag;
	} meta;

	uint8_t color() {
		return meta.tag.color;
	}

	void color(uint8_t c) {
		meta.tag.color = c;
	}

	int8_t storage() {
		return meta.tag.storage;
	}
	
	void storage(int8_t ty) {
		assert(ty >= 0 || ty == HASH_LIKE || ty == ARRAY_LIKE || ty == TREE_MAP_LIKE);
		meta.tag.storage = ty;
	}

	Obj* next() {
		return (Obj*)(meta.p & ((1ull << 48) - 1));
	}

	void next(Obj* o) {
		assert((uint64_t)o < (1ull << 48));
		meta.p &= ~((1ull << 48) - 1);
		meta.p |= (uint64_t)o;
	}
};

struct TupleObj : public Obj {
	size_t size() {
		assert(storage() > 0);
		return storage();
	}
	Val* ptr() {
		assert(storage() > 0);
		return (Val*)(void*)(&meta.p + 1);
	}
	// TODO call mark after linking elems
};

struct ArrayObj : public Obj {
	ValArray array;
	ValArray* ptr() {
		assert(storage() == ARRAY_LIKE);
		return &array;
	}
	// TODO call mark after adding elems
};

struct HashObj : public Obj {
	ValHash hash;
	ValHash* ptr() {
		assert(storage() == HASH_LIKE);
		return &hash;
	}
	// TODO call mark after adding elems
};

struct TreeMapObj : public Obj {
	ValTreeMap tree_map;
	ValTreeMap* ptr() {
		assert(storage() == TREE_MAP_LIKE);
		return &tree_map;
	}
	// TODO call mark after adding elems
};