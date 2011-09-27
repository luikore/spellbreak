#pragma once
#include <stdlib.h>
#include <stdint.h>
#include "obj.hpp"

// Incremental GC with 3-color marking
class GC {

	enum Phase { IDLE, MARK, SWEEP };

	struct Stack : public std::vector<Obj*> {
		Obj* pop_obj() {
			Obj* o = back();
			pop_back();
			return o;
		}

		void push_gray(Obj* o) {
			if(o->color() == Obj::WHITE)
				push_back(o);
		}

		void push_gray_val(Val v) {
			if(IS_PTR(v) && ((Obj*)v)->color() == Obj::WHITE)
				push_back((Obj*)v);
		}

		void remove(Obj* o) {
			for(iterator i = begin(); i != end(); i++)
				if(*i == o)
					erase(i);
		}
	};

	const size_t mark_max;	// max actions in a mark step
	const size_t sweep_max;	// max actions in a sweep step
	Obj* head;		// list of all objects
	Obj* sweeping;		// list of snapshot objects to sweep
	Obj* swept;		// prev of sweeping
	Obj* snapshot;		// the last one to mark as white
	Stack registered;	// root set
	Stack gray;		// gray set
	Phase phase;
	size_t allocated;	// object count

	void mark_propagate() {
		assert(!gray.empty());
		Obj* o = gray.pop_obj();
		if(o->color() == Obj::BLACK)
			return;
		o->color(Obj::BLACK);
		int8_t st = o->storage();

		if(st == 0) {
			return;

		} else if(st > 0) {
			Val* p = ((TupleObj*)o)->ptr();
			for(size_t i = 0; i < st; i++)
				gray.push_gray_val(p[i]);

		} else if(st == Obj::ARRAY_LIKE) {
			ValArray* array = ((ArrayObj*)o)->ptr();
			for(ValArray::iterator i = array->begin(); i != array->end(); i++)
				gray.push_gray_val(*i);

		} else if(st == Obj::HASH_LIKE) {
			ValHash* hash = ((HashObj*)o)->ptr();
			for(ValHash::iterator i = hash->begin(); i != hash->end(); i++) {
				gray.push_gray_val(i->first);
				gray.push_gray_val(i->second);
			}

		} else if(st == Obj::TREE_MAP_LIKE) {
			ValTreeMap* tree_map = ((TreeMapObj*)o)->ptr();
			for(ValTreeMap::iterator i = tree_map->begin(); i != tree_map->end(); i++) {
				gray.push_gray_val(i->first);
				gray.push_gray_val(i->second);
			}
		}
	}

	void init_step() {
		assert(gray.empty());
		if(allocated < 2)
			return;

		// snapshot
		swept = head;
		snapshot = swept;
		sweeping = swept->next();

		// init gray
		for(Stack::iterator i = registered.begin(); i != registered.end(); i++)
			gray.push_gray(*i);
		phase = MARK;
	}

	void mark_step() {
		for(size_t i = 0; i < mark_max; i++) {
			if(gray.empty()) {
				phase = SWEEP;
				return;
			}
			mark_propagate();
		}
	}

	void sweep_step() {
		for(size_t i = 0; i < sweep_max; i++) {
			if(!sweeping) {
				if(snapshot)
					snapshot->color(Obj::WHITE);
				swept = NULL;
				snapshot = NULL;
				phase = IDLE;
				return;
			}
			assert(IS_PTR(sweeping));

			if(sweeping->color() == Obj::BLACK) {
				sweeping->color(Obj::WHITE);
				swept = sweeping;
				sweeping = sweeping->next();
			} else {
				assert(sweeping->color() == Obj::WHITE);
				swept->next(sweeping->next());
				Obj* to_free = sweeping;
				sweeping = sweeping->next();

				int8_t st = to_free->storage();
				if(st == Obj::ARRAY_LIKE) {
					((ArrayObj*)to_free)->ptr()->~ValArray();
				} else if(st == Obj::HASH_LIKE) {
					((HashObj*)to_free)->ptr()->~ValHash();
				} else if(st == Obj::TREE_MAP_LIKE) {
					((TreeMapObj*)to_free)->ptr()->~ValTreeMap();
				}

				allocated--;
				free(to_free);
			}
		}
	}

public:

	// set to true to pause gc
	bool pause;

	GC(size_t _mark_max, size_t _sweep_max)
	: mark_max(_mark_max), sweep_max(_sweep_max), phase(IDLE), pause(false), sweeping(NULL), swept(NULL), head(NULL), allocated(0), snapshot(NULL) {
		assert(_mark_max > 0);
		assert(_sweep_max > 0);
	}

	~GC() {
		Obj* o = head;
		Obj* p;
		while(o) {
			p = o->next();
			free(o);
			o = p;
		}
	}

	Obj* alloc(size_t size) {
		Obj* o = (Obj*)malloc(size);
		if(!o && phase == SWEEP) {
			sweep_step();
			o = (Obj*)malloc(size);
		}
		if(!o)
			throw "OOM";
		memset(o, 0, size);
		o->next(head);
		head = o;
		allocated++;
		return o;
	}

	TupleObj* alloc_tuple(size_t size) {
		assert(size > 0);
		TupleObj* o = (TupleObj*)alloc(sizeof(TupleObj) + size * sizeof(Val));
		o->storage(size);
		return o;
	}

	ArrayObj* alloc_array(size_t init_size) {
		ArrayObj* o = (ArrayObj*)alloc(sizeof(ArrayObj));
		new(&o->array) ValArray(init_size);
		o->storage(Obj::ARRAY_LIKE);
		return o;
	}

	HashObj* alloc_hash(size_t init_size) {
		HashObj* o = (HashObj*)alloc(sizeof(HashObj));
		new(&o->hash) ValHash(init_size);
		o->storage(Obj::HASH_LIKE);
		return o;
	}

	TreeMapObj* alloc_tree_map() {
		TreeMapObj* o = (TreeMapObj*)alloc(sizeof(TreeMapObj));
		new(&o->tree_map) ValTreeMap();
		o->storage(Obj::TREE_MAP_LIKE);
		return o;
	}

	// for adding globals, and stack objs. (o can be not on heap)
	void add_to_root(Obj* o) {
		// todo check dup?
		registered.push_back(o);
		mark(o);
	}

	void remove_from_root(Obj* o) {
		registered.remove(o);
	}

	// after adding a member, (always) mark the member (also known as write_barrier)
	void mark(Obj* o) {
		if(o->color() != Obj::WHITE) return;
		if(phase == MARK)
			gray.push_back(o);
		else if(phase == SWEEP) {
			gray.push_back(o);
			while(!gray.empty())
				mark_propagate();
		}
	}

	// incremental gc step, do nothing if paused
	void step() {
		if(pause)
			return;
		// inspect_heap();
		if(phase == MARK)
			mark_step();
		else if(phase == SWEEP)
			sweep_step();
		else if(phase == IDLE)
			init_step();
	}

	// complete the gc cycle, no matter whether paused
	void full_gc() {
		if(phase == IDLE)
			init_step();
		while(phase == MARK)
			mark_step();
		while(phase == SWEEP)
			sweep_step();
	}
	
	size_t objects() {
		return allocated;
	}

	void inspect() {
		const char* phase_s = (phase == IDLE ? "idle" : phase == MARK ? "mark" : phase == SWEEP ? "sweep" : "error");
		printf("\nmark_max: %lu\tsweep_max: %lu\tallocated: %lu\n", mark_max, sweep_max, allocated);
		printf("registered: %lu\tgray: %lu\t\tphase:%s\n", registered.size(), gray.size(), phase_s);
		printf("head: %p\n", head);
		printf("snapshot: %p\n", snapshot);
		printf("sweeping: %p\n", sweeping);
		printf("swept: %p\n\n", swept);
	}

	void inspect_heap() {
		Obj* o = head;
		while(o) {
			printf("%p:%d\n", o, o->color());
			o = o->next();
		}
		printf("\n");
	}
};
