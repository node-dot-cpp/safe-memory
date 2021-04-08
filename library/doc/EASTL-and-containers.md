
EASTL and containers
====================

This document is a short guide on how `EASTL` containers where _adapted_ into __safe__ containers.

Memory states glosary for this doc:
* Allocated: memory asigned by the allocator to a pariticular object, may be Valid, Uninitialized, Zeroed or Zombie.
	+ Valid: contains a valid contructed object.
	+ Zeroed: self explained.
	+ Uninitialized: may be zero or may have any kind of garbage.
	+ Zombie: contains an object that has already been destructed. May or may not already been returned to the allocator, as long as the allocator has not already reasigned it.

Safety on containers
--------------------
Safety on containers goes around validation of method arguments, and safety of iterators.
For safety of iterators, we define two kind of iterators for each container: _regular_ iterator and __safe__ iterator.

Under `safememory` context, a _regular_ iterator should be allowed to exist only on the stack and should be enforced lifetime checks, same as raw pointer. Dereference will always point to _allocated_ memory.

A __safe__ iterator on the other hand, has the same set of rules that a `soft_ptr`, can be stored on the heap, and has means to verify where the target memory is still _allocated_ or not.

Each container provides both iterators types, and two sets of methods to work with one or the other kind:

	iterator       		begin() noexcept;
	iterator_safe       begin_safe() noexcept;



Also each safe container has also an __\_safe__ sister (i.e. `vector_safe`, `string_safe`, etc) where all methods and iterators are safe. i.e. `vector_safe::iterator` => `vector::iterator_safe`, `vector_safe::begin()` => `vector::begin_safe()`, and so on.



Implementation of safe containers
---------------------------------
To get this working, we modified `eastl` containers as little as possible and implemented a safety layer over them, inheriting privately and passing a very custom allocator that is explained in detail below:

	namespace safememory
	{
		template <typename T, memory_safety Safety = safeness_declarator<T>::is_safe>
		class vector : protected eastl::vector<T, detail::allocator_to_eastl_vector<Safety>>


Some methods require validation of its arguments to be safe, we do that before calling the base container corresponding method:

	reference operator[](size_type n)
	{
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(n >= size()))
				ThrowRangeException("vector::operator[] -- out of range");
		}

		return base_type::operator[](n);
	}

For _regular_ iterators, since they don't allow to dereference any invalid memory, they can't be implemented with raw pointers. They are a full class that knows the valid iterable range and its current position, quite straight forward and nothing obscure here.
Only to understand that each time an iterator comes in as argument, it has to be converted to raw pointer, feed to the underlying `eastl` container, and then the returning raw pointer converted back to a _regular_ iterator:


	iterator insert(const_iterator_arg position, const value_type& value) {
		return makeIt(base_type::insert(toBase(position), value));
	}


For __safe__ iterators we must merge the properties of _regular_ iterators with the properties of a `soft_ptr`, so so we must first understand how `soft_ptr` works.

Under `safememory` when an object is allocated on the heap, an special allocation function (`make_owning`) will allocate some extra memory to place a `ControlBlock` before the object, and an `owning_ptr` to such object will be returned. Then the `owning_ptr` knows about that `ControlBlock` and allows to create `soft_ptr` that _hook_ on the `ControlBlock`. When the `owning_ptr` is called to destroy the object, it releases its memory and notifies all `soft_ptr` that are still hooked.

The tricky part is how to get that `soft_ptr`, and how to make it hook to the `ControlBlock` without breaking everything in the process.
First attempt was to put `owning_ptr` inside the underlying containers, but that prove to be too complex, as ownership semantics of `owning_ptr` forced to a complete rewrite of most methods, droping all the adventages of using an underlying container.

Tried again, this time using __custom__ allocation function that makes the same as `make_owning` does, allocating some extra room for `ControlBlock` and initializing it, but it doesn't construct the object, and instead of returning an `owning_ptr`, it returns a `soft_ptr_with_zero_offset`. Then `soft_ptr` can be constructed form `soft_ptr_with_zero_offset` as it knows if there is a `ControlBlock` to hook, but unlike `owning_ptr` it does behave more like a raw pointer to minimize changes needed.

We modified `eastl` containers to use `soft_ptr_with_zero_offset` to hold the allocated pointer, and allocate them throught our __custom__ allocation functions. Then we can create `soft_ptr` from them and __safe__ iterators became a reality.


Container paricularities
------------------------

### safememory::vector
This is the most straight forward of all containers. Only particularity is that has __experimental__ support for `soft_this_ptr` on the elements it contains.
Vector allocates memory when the first element is pushed, so iterators to default constructed vector have `nullptr` inside.

### safememory::string
Underlying `eastl::basic_string` implements SSO (short string optimization), this means that when the string is short enought characters are stored inside the instance body and not on the heap. This is done using an `union` an putting there all data including the `soft_ptr_with_zero_offset`.
For _regular_ iterators this works the same, but when a __safe__ iterator is required, data has to be moved to the heap.
Also a particularity of `eastl::basic_string` is that even default constructed instances have one null `'\0'` character in the buffer, so iterators to default constructed string don't have `nullptr` inside.

### safememory::unordered_map
Here `eastl::hashtable` has a couple of tricks we must address.
First on the table itself, the last pointer is asigned with a non-null but non dereferenceable value:

		void increment_bucket()
		{
			++mpBucket;
			while(*mpBucket == NULL) // We store an extra bucket with some non-NULL value at the end 
				++mpBucket;          // of the bucket array so that finding the end of the bucket
			mpNode = *mpBucket;      // array is quick and simple.
		}

So `soft_ptr_with_zero_offset` has to live with that, but such value can't propagate into `soft_ptr`.

Second, to avoid allocate on default construct, all instances of `eastl::hashtable` share a common static constant representation of an empty hash table. Only when the first element is inserted, allocation takes place. This is another _special_ value that `soft_ptr_with_zero_offset` has to live with, but can't propagate into `soft_ptr`. And __safe__ iterators to empty hash tables are actually default constructed ones.


### safememory::array
Array does not use allocation, all elements are stored in the body of the array.
If array is created on the stack, all elements are on the stack. If we allocate an array on the heap, we are doing the allocation. Array internally never allocates, doesn't have an allocator, or does anything with memory. 

For _regular_ iterators this is not an issue, but for __safe__ iterators it is, as we can only create them when the array is on the heap. And to reach the `ControlBlock` we can only rely on constructors and some mechanims like `soft_this_ptr` does.

Now `easlt::array` uses aggreagate initialization:

		public:

		// Note that the member data is intentionally public.
		// This allows for aggregate initialization of the
		// object (e.g. array<int, 5> a = { 0, 3, 2, 4 }; )
		value_type mValue[N ? N : 1];


But such has very narrow C++ rules requiring no user constructor and no members with user constructors. So we must either drop `eastl::array` or drop __safe__ iterators.
The result is a fully custom implementation of `safememory::array` not depending on any underlying implementation, and using a constructor with `std::initializer_list<T>`.
This implementation can't be used in `constexpr` context. And may have other issues I can't foresee at this time.


### safememory::basic_string_literal
String literal class don't exist on `std` or `eastl` so is fully implemented on `safememory`.
The important part is that while we can't create `soft_ptr` because literal has no `ControlBlock`, a _regular_ iterator would be __safe__ because literal will live in memory forever. Only need to use some different iterator for _checker_ to understand this diference.



Dependency order
----------------
Since `safememory` library depends (or uses) `eastl` containers, that stablishes a dependency order.
If from `eastl` we try to `#include` something from `safememory` we would be inverting the dependecy order and that would cause `#include` loops (an is very bad practice).
Then all types and functions from `safememory` that `eastl` containers use must be _injected_, and since adding one more template parameter everywhere whould have required a lot of _intrussion_ at `eastl` we decided to overload the existing __Allocator__ template parameter, to fullfil all the required tasks.
We know that adding more than one responsability to a single template parameter is also bad practice, but in this particular case we felt it gives the best balance.


Allocator responsabilities
--------------------------
On `safememory` all pointer wrappers support the idea of compile time safety on/off, this is achived throught template parameter enum `memory_safety::none` and `memory_safety::safe`. This idea is forwarded into `eastl` containers throught the allocator type. We use one allocator type when `memory_safety::none` and a different allocator type when `memory_safety::safe`:


    template<memory_safety Safety>
    using allocator_to_eastl_vector = std::conditional_t<Safety == memory_safety::safe,
			allocator_to_eastl_vector_impl, allocator_to_eastl_vector_no_checks>;



On `safememory` we use wrappers for allocated heap memory pointers. More, we make a distiction between a pointer to an object and a pointer to an array of objects (i.e. the later allows `operator[]` while the first not). Allocator provides both type aliases for containers to know about:

    template<class T>
	using pointer = soft_ptr_with_zero_offset_impl<T>;

	template<class T>
	using array_pointer = soft_ptr_with_zero_offset_impl<flexible_array<T>>;


Allocator of course provides means to allocate / deallocate one object, and also an array of objects:

	template<class T>
	pointer<T> allocate_node();

	template<class T>
	array_pointer<T> allocate_array(std::size_t count, int flags = 0);

	template<class T>
	void deallocate_node(const pointer<T>& p);

	template<class T>
	void deallocate_array(const array_pointer<T>& p, std::size_t count);


Also provides means to convert allocated wrappers to __raw__ pointers and to `soft_ptr`:

	template<class T>
	static soft_ptr_impl<T> to_soft(const pointer<T>& p);

	template<class T>
	static T* to_raw(const pointer<T>& p);


Has knowledge of two _special_ pointer values used by `eastl::hashtable`, that should be supported inside `soft_ptr_with_zero_offset` but can't be mapped to `soft_ptr` because there is no `ControlBlock` to hook.


	template<class T>
	static pointer<T> get_hashtable_sentinel();

	template<class T>
	static array_pointer<T> get_empty_hashtable();


And last, provides a __RAII__ helper to allow `soft_this_ptr` to work when an element is pushed by-value inside an `EASTL::vector`:

	template<class T>
	static soft_this_ptr_raii<T> make_raii(const pointer<T>& p);


Important files
---------------

### `safememory/detail/allocator_to_eastl.h`

Here is where all the magic is hidden (and most likely all the bugs).
The most critical part are allocation functions at the beginning of the file, the bottom 3 are the entry points, and the top 2 are called internally from the others:

	template<std::size_t alignment>
	void* zombie_allocate_helper(std::size_t sz);

	template<memory_safety is_safe, std::size_t alignment>
	std::pair<make_zero_offset_t, void*> allocate_helper(std::size_t sz);

	template<memory_safety is_safe, class T>
	void deallocate_helper(const soft_ptr_with_zero_offset<T, is_safe>& p);

	template<memory_safety is_safe, class T>
	soft_ptr_with_zero_offset<T, is_safe> allocate_node_helper();

	template<memory_safety is_safe, typename T, bool zeroed>
	soft_ptr_with_zero_offset<flexible_array<T>, is_safe> allocate_flexible_array_helper(std::size_t count);


### `safememory/detail/soft_ptr_with_zero_offset.h`
All container alocations return an instance of `soft_ptr_with_zero_offset`, and the main function of this class is to be a _marker_ of such thing. There are 4 specializations of this class to match each of the 4 allocators functions described above.

Some importan points:
* Only the allocator creates them.
* They won't construct or destruct the instance it points.
* They don't own the memory, can be copied.
* They can be used inside `union` (`eastl::string` needs that).
* They can have _special_ values that point to static or invalid memory (`eastl::hashtable` needs that).
* Allocator can create a `soft_ptr` from them, but first _special_ values are checked.


### `safememory/detail/flexible_array.h`
All array allocations return an instance of `soft_ptr_with_zero_offset<flexible_array<T>>`, and the main function is to be a marker of the existance of an array in the memory layout.

Some importan points:
* Only the allocator creates them.
* They won't construct or destruct any instance inside such memory array.
* Specialized `soft_ptr_with_zero_offset<flexible_array<T>>` has array operators overloaded and pointer arithmetics, so simplify changes.
