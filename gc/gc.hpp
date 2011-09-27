#pragma once

#include <stdlib.h>
#include <vector>
#include <set>

typedef void* Val;

// Incremental GC with 3-color marking
class GC {

	enum Phase { IDLE, MARK, SWEEP };
	struct Stack : public std::vector<Val> {
		Val pop_obj();
		void push_gray(Val o);
		void push_gray_val(Val v);
		void remove(Val o);
	};
	typedef std::set<Val> Set;

	const size_t mark_max;	// max actions in a mark step
	const size_t sweep_max;	// max actions in a sweep step
	Val head;		// list of all objects
	Val sweeping;		// list of snapshot objects to sweep
	Val swept;		// prev of sweeping
	Val snapshot;		// the last one to mark as white
	Set registered;	// root set
	Stack gray;		// gray set
	Phase phase;
	size_t allocated;	// object count

	void mark_propagate();
	void init_step();
	void mark_step();
	void sweep_step();
	Val alloc(size_t size);

public:

	// set to true to pause gc
	bool pause;

	GC(size_t _mark_max, size_t _sweep_max);
	~GC();
	Val alloc_string(size_t size);
	Val alloc_tuple(size_t size);
	Val alloc_array(size_t init_size);
	Val alloc_hash(size_t init_size);
	Val alloc_tree_map();
	// for adding globals and stack objs. (if v is pointer, should always be on heap)
	void add_to_root(Val v);
	void remove_from_root(Val v);
	// after adding a member, (always) mark the member (also known as write_barrier)
	void mark(Val v);
	// incremental gc step, do nothing if paused
	void step();
	// complete the gc cycle, no matter whether paused, if idle, do nothing
	void finish_gc();
	// complete the gc cycle, no matter whether paused, if idle, gc once and again
	void full_gc();
	size_t objects();
	void inspect();
	void inspect_heap();
	void inspect_registered();
	void inspect_gray();
	void inspect_all();
};
