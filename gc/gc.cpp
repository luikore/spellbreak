#include "obj.hpp"

// stack

Val GC::Stack::pop_obj() {
	Val o = back();
	pop_back();
	return o;
}

void GC::Stack::push_gray_val(Val v) {
	if(IS_PTR(v) && ((Obj*)v)->color() == WHITE)
		push_back(v);
}

// public

GC::GC(size_t _mark_max, size_t _sweep_max) : mark_max(_mark_max), sweep_max(_sweep_max), phase(IDLE), pause(false),
	sweeping(NULL), swept(NULL), head(NULL), allocated(0), snapshot(NULL) {
	assert(_mark_max > 0);
	assert(_sweep_max > 0);
}

GC::~GC() {
	Obj* o = (Obj*)head;
	Obj* p;
	while(o) {
		p = o->next();
		free(o);
		o = p;
	}
}

Val GC::alloc_string(size_t size) {
	return alloc(sizeof(StringObj) + size * sizeof(char));
}

Val GC::alloc_tuple(size_t size) {
	assert(size > 0);
	Obj* o = (Obj*)alloc(sizeof(TupleObj) + size * sizeof(Val));
	o->storage(size);
	return o;
}

Val GC::alloc_array(size_t init_size) {
	ArrayObj* o = (ArrayObj*)alloc(sizeof(ArrayObj));
	new(&o->array) ValArray(init_size);
	o->storage(ARRAY_LIKE);
	return o;
}

Val GC::alloc_hash(size_t init_size) {
	HashObj* o = (HashObj*)alloc(sizeof(HashObj));
	new(&o->hash) ValHash(init_size);
	o->storage(HASH_LIKE);
	return o;
}

Val GC::alloc_tree_map() {
	TreeMapObj* o = (TreeMapObj*)alloc(sizeof(TreeMapObj));
	new(&o->tree_map) ValTreeMap();
	o->storage(TREE_MAP_LIKE);
	return o;
}

void GC::add_to_root(Val v) {
	if(IS_PTR(v)) {
		registered.insert(v);
		mark((Obj*)v);
	}
}

void GC::remove_from_root(Val v) {
	if(IS_PTR(v))
		registered.erase(v);
}

void GC::mark(Val v) {
	assert(IS_PTR(v));
	Obj* o = (Obj*)v;
	if(o->color() != WHITE) return;
	if(phase == MARK)
		gray.push_back(o);
	else if(phase == SWEEP) {
		gray.push_back(o);
		while(!gray.empty())
			mark_propagate();
	}
}

void GC::step() {
	if(pause)
		return;
	if(phase == MARK)
		mark_step();
	else if(phase == SWEEP)
		sweep_step();
	else if(phase == IDLE)
		init_step();
}

void GC::finish_gc() {
	while(phase == MARK)
		mark_step();
	while(phase == SWEEP)
		sweep_step();
}

void GC::full_gc() {
	if(phase == IDLE)
		init_step();
	finish_gc();
}

size_t GC::objects() {
	return allocated;
}

void GC::inspect() {
	const char* phase_s = (phase == IDLE ? "idle" : phase == MARK ? "mark" : phase == SWEEP ? "sweep" : "error");
	printf("-- state --\n");
	printf("mark_max: %lu\tsweep_max: %lu\tallocated: %lu\n", mark_max, sweep_max, allocated);
	printf("registered: %lu\tgray: %lu\t\tphase:%s\n", registered.size(), gray.size(), phase_s);
	printf("head: %p\n", head);
	printf("snapshot: %p\n", snapshot);
	printf("sweeping: %p\n", sweeping);
	printf("swept: %p\n", swept);
}

void GC::inspect_heap() {
	Obj* o = (Obj*)head;
	printf("-- heap --\n");
	while(o) {
		printf("%p:%d\n", o, o->color());
		o = o->next();
	}
}

void GC::inspect_registered() {
	printf("-- registerd --\n");
	for(Set::iterator i = registered.begin(); i != registered.end(); i++)
		printf("%p:%d\n", *i, ((Obj*)(*i))->color());
}

void GC::inspect_gray() {
	printf("-- gray --\n");
	for(Stack::iterator i = gray.begin(); i != gray.end(); i++)
		printf("%p:%d\n", *i, ((Obj*)(*i))->color());
}

void GC::inspect_all() {
	inspect();
	inspect_heap();
	inspect_registered();
	inspect_gray();
	printf("\n");
}

// protected

void GC::mark_propagate() {
	assert(!gray.empty());
	Obj* o = (Obj*)gray.pop_obj();
	assert(o->color() == BLACK || o->color() == WHITE);
	if(o->color() == BLACK)
		return;
	o->color(BLACK);
	int8_t st = o->storage();

	if(st == 0) {
		return;

	} else if(st > 0) {
		Val* p = ((TupleObj*)o)->ptr();
		for(size_t i = 0; i < st; i++)
			gray.push_gray_val(p[i]);

	} else if(st == ARRAY_LIKE) {
		ValArray* array = ((ArrayObj*)o)->ptr();
		for(ValArray::iterator i = array->begin(); i != array->end(); i++)
			gray.push_gray_val(*i);

	} else if(st == HASH_LIKE) {
		ValHash* hash = ((HashObj*)o)->ptr();
		for(ValHash::iterator i = hash->begin(); i != hash->end(); i++) {
			gray.push_gray_val(i->first);
			gray.push_gray_val(i->second);
		}

	} else if(st == TREE_MAP_LIKE) {
		ValTreeMap* tree_map = ((TreeMapObj*)o)->ptr();
		for(ValTreeMap::iterator i = tree_map->begin(); i != tree_map->end(); i++) {
			gray.push_gray_val(i->first);
			gray.push_gray_val(i->second);
		}
	} else {
		assert(false);
	}
}

void GC::init_step() {
	assert(gray.empty());
	if(allocated < 2)
		return;

	// snapshot
	swept = head;
	snapshot = swept;
	sweeping = ((Obj*)swept)->next();

	// init gray
	for(Set::iterator i = registered.begin(); i != registered.end(); i++)
		gray.push_back(*i);
	phase = MARK;
}

void GC::mark_step() {
	for(size_t i = 0; i < mark_max; i++) {
		if(gray.empty()) {
			phase = SWEEP;
			return;
		}
		mark_propagate();
	}
}

void GC::sweep_step() {
	for(size_t i = 0; i < sweep_max; i++) {
		if(!sweeping) {
			if(snapshot)
				((Obj*)snapshot)->color(WHITE);
			swept = NULL;
			snapshot = NULL;
			phase = IDLE;
			return;
		}
		assert(IS_PTR(sweeping));

		Obj* o = (Obj*)sweeping;
		if(o->color() == BLACK) {
			o->color(WHITE);
			swept = o;
			sweeping = o->next();
		} else {
			assert(o->color() == WHITE);
			((Obj*)swept)->next(o->next());
			sweeping = o->next();

			int8_t st = o->storage();
			if(st == ARRAY_LIKE) {
				((ArrayObj*)o)->ptr()->~ValArray();
			} else if(st == HASH_LIKE) {
				((HashObj*)o)->ptr()->~ValHash();
			} else if(st == TREE_MAP_LIKE) {
				((TreeMapObj*)o)->ptr()->~ValTreeMap();
			} else {
				assert(st >= 0);
			}

			allocated--;
			assert(allocated >= 0);
			free(o);
		}
	}
}

Val GC::alloc(size_t size) {
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
	return (Val)o;
}