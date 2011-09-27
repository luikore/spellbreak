#include "gc.cpp"

void test_gc() {
	GC gc = GC(3, 3);

	TupleObj* o0 = (TupleObj*)gc.alloc_tuple(1);
	ArrayObj* o1 = (ArrayObj*)gc.alloc_array(0);
	o1->array.push_back((Val)o0);

	gc.add_to_root(o1);
	gc.mark(o0);
	gc.full_gc();

	HashObj* o2 = (HashObj*)gc.alloc_hash(0);
	TreeMapObj* o3 = (TreeMapObj*)gc.alloc_tree_map();
	o0->ptr()[0] = (Val)o2;

	gc.mark(o2);
	assert(gc.objects() == 4);
	gc.full_gc();
	gc.full_gc();
	
	gc.alloc_tuple(3);
	assert(gc.objects() == 5);
	gc.full_gc();
	assert(gc.objects() == 4);
	
	gc.remove_from_root(o1);
	gc.full_gc();
	assert(gc.objects() == 1);
}

void test_obj() {
	GC gc = GC(3, 3);
	
	TupleObj* t = (TupleObj*)gc.alloc_tuple(1);
	assert(t->storage() == 1);
	
	t->ptr()[0] = gc.alloc_hash(0);
	HashObj* h = (HashObj*)(t->ptr()[0]);
	assert(h->storage() == HASH_LIKE);
	
	(*(h->ptr()))[0] = (Val)(gc.alloc_tree_map());
	TreeMapObj* tm = (TreeMapObj*)((*(h->ptr()))[0]);
	assert(tm->storage() == TREE_MAP_LIKE);
	
	ArrayObj* a = (ArrayObj*)gc.alloc_array(0);
	a->ptr()->push_back(t);
	assert(a->storage() == ARRAY_LIKE);
}

int main (int argc, char const *argv[]) {
	test_obj();
	test_gc();
	return 0;
}