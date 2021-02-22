/* -------------------------------------------------------------------------------
* Copyright (c) 2020, OLogN Technologies AG
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the OLogN Technologies AG nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------------------*/

#ifndef SAFE_MEMORY_UNORDERED_SET_H
#define SAFE_MEMORY_UNORDERED_SET_H

#include <utility>
#include <EASTL/unordered_set.h>
#include <safememory/functional.h>
#include <safememory/detail/allocator_to_eastl.h>
#include <safememory/detail/hashtable_iterator.h>


namespace safememory
{
	template <typename Key, typename Hash = hash<Key>, typename Predicate = equal_to<Key>, 
			  memory_safety Safety = safeness_declarator<Key>::is_safe>
	class SAFEMEMORY_DEEP_CONST_WHEN_PARAMS unordered_set
		: private eastl::unordered_set<Key, Hash, Predicate, detail::allocator_to_eastl_hashtable<Safety>>
	{
	public:
		typedef eastl::unordered_set<Key, Hash, Predicate, detail::allocator_to_eastl_hashtable<Safety>> base_type;
		typedef unordered_set<Key, Hash, Predicate, Safety>                    this_type;
		typedef typename base_type::size_type                                     size_type;
		typedef typename base_type::key_type                                      key_type;
		typedef typename base_type::mapped_type                                   mapped_type;
		typedef typename base_type::value_type                                    value_type;     // NOTE: 'value_type = pair<const key_type, mapped_type>'.
		typedef typename base_type::allocator_type                                allocator_type;
		typedef typename base_type::node_type                                     node_type;
		typedef typename base_type::iterator                                      base_iterator;
		typedef typename base_type::const_iterator                                const_base_iterator;
		typedef typename base_type::local_iterator                                local_iterator;
		typedef typename base_type::const_local_iterator                          const_local_iterator;
		typedef typename base_type::insert_return_type                            base_insert_return_type;

		typedef typename detail::hashtable_heap_safe_iterator<base_iterator, base_iterator, allocator_type>        heap_safe_iterator;
		typedef typename detail::hashtable_heap_safe_iterator<const_base_iterator, base_iterator, allocator_type>   const_heap_safe_iterator;
		typedef typename detail::hashtable_stack_only_iterator<base_iterator, base_iterator, allocator_type>       stack_only_iterator;
		typedef typename detail::hashtable_stack_only_iterator<const_base_iterator, base_iterator, allocator_type>  const_stack_only_iterator;

		// TODO: add 'use_base_iterator'

		typedef stack_only_iterator                                               iterator;
		typedef const_stack_only_iterator                                         const_iterator;
		typedef eastl::pair<iterator, bool>                                       insert_return_type;

		typedef heap_safe_iterator                                                iterator_safe;
		typedef const_heap_safe_iterator                                          const_iterator_safe;
		typedef eastl::pair<iterator_safe, bool>                                  insert_return_type_safe;

	public:
		explicit unordered_set(): base_type(allocator_type()) {}
	 	explicit unordered_set(size_type nBucketCount, const Hash& hashFunction = Hash(), 
						  const Predicate& predicate = Predicate())
			: base_type(nBucketCount, hashFunction, predicate, allocator_type())
		    {}
		unordered_set(const this_type& x) = default;
		unordered_set(this_type&& x) = default;
		unordered_set(std::initializer_list<value_type> ilist, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
				   const Predicate& predicate = Predicate())
			: base_type(ilist, nBucketCount, hashFunction, predicate, allocator_type())
            {}

		this_type& operator=(const this_type& x) = default;
		this_type& operator=(this_type&& x) = default;
		this_type& operator=(std::initializer_list<value_type> ilist)
            { return static_cast<this_type&>(base_type::operator=(ilist)); }

		void swap(this_type& x) { base_type::swap(x); }

		iterator       begin() noexcept { return makeIt(base_type::begin()); }
		const_iterator begin() const noexcept { return makeIt(base_type::begin()); }
		const_iterator cbegin() const noexcept { return makeIt(base_type::cbegin()); }

		iterator       end() noexcept { return makeIt(base_type::end()); }
		const_iterator end() const noexcept { return makeIt(base_type::end()); }
		const_iterator cend() const noexcept { return makeIt(base_type::cend()); }

		local_iterator begin(size_type n) noexcept { return base_type::begin(n); }
		const_local_iterator begin(size_type n) const noexcept { return base_type::begin(n); }
		const_local_iterator cbegin(size_type n) const noexcept { return base_type::cbegin(n); }

		local_iterator end(size_type n) noexcept { return base_type::end(n); }
		const_local_iterator end(size_type n) const noexcept { return base_type::end(n); }
		const_local_iterator cend(size_type n) const noexcept { return base_type::cend(n); }

        // using base_type::at;
        // using base_type::operator[];

        using base_type::empty;
        using base_type::size;
        using base_type::bucket_count;
        using base_type::bucket_size;

//        using base_type::bucket;
        using base_type::load_factor;
        using base_type::get_max_load_factor;
        using base_type::set_max_load_factor;
        using base_type::rehash_policy;

		template <class... Args>
		insert_return_type emplace(Args&&... args) {
            return makeIt(base_type::emplace(std::forward<Args>(args)...));
        }

		template <class... Args>
		iterator emplace_hint(const_iterator position, Args&&... args) {
            return makeIt(base_type::emplace_hint(position.toBase(), std::forward<Args>(args)...));
        }

		template <class... Args>
        insert_return_type try_emplace(const key_type& k, Args&&... args) {
            return makeIt(base_type::try_emplace(k, std::forward<Args>(args)...));
        }

		template <class... Args>
        insert_return_type try_emplace(key_type&& k, Args&&... args) {
            return makeIt(base_type::try_emplace(std::move(k), std::forward<Args>(args)...));
        }

		template <class... Args> 
        iterator try_emplace(const_iterator position, const key_type& k, Args&&... args) {
            return makeIt(base_type::try_emplace(position.toBase(), k, std::forward<Args>(args)...));
        }

		template <class... Args>
        iterator try_emplace(const_iterator position, key_type&& k, Args&&... args) {
            return makeIt(base_type::try_emplace(position.toBase(), std::move(k), std::forward<Args>(args)...));
        }

		insert_return_type insert(const value_type& value) {
             return makeIt(base_type::insert(value));
        }

		insert_return_type insert(value_type&& value) {
            return makeIt(base_type::insert(std::move(value)));
        }

		iterator insert(const_iterator hint, const value_type& value) {
            return makeIt(base_type::insert(hint.toBase(), value));
        }

		iterator insert(const_iterator hint, value_type&& value) {
            return makeIt(base_type::insert(hint.toBase(), std::move(value)));
        }

		void insert(std::initializer_list<value_type> ilist) {
            base_type::insert(ilist);
        }

		template <typename InputIterator>
        void insert_unsafe(InputIterator first, InputIterator last) {
            base_type::insert(first, last);
        }

		template <class M>
        insert_return_type insert_or_assign(const key_type& k, M&& obj) {
            return makeIt(base_type::insert_or_assign(k, std::forward<M>(obj)));
        }

		template <class M>
        insert_return_type insert_or_assign(key_type&& k, M&& obj) {
            return makeIt(base_type::insert_or_assign(std::move(k), std::forward<M>(obj)));
        }

		template <class M>
        iterator insert_or_assign(const_iterator hint, const key_type& k, M&& obj) {
            return makeIt(base_type::insert_or_assign(hint.toBase(), k, std::forward<M>(obj)));
        }

		template <class M>
        iterator insert_or_assign(const_iterator hint, key_type&& k, M&& obj) {
            return makeIt(base_type::insert_or_assign(hint.toBase(), std::move(k), std::forward<M>(obj)));
        }

		iterator erase(const_iterator position) {
            return makeIt(base_type::erase(position.toBase()));
        }

		iterator erase(const_iterator first, const_iterator last) {
            return makeIt(base_type::erase(first.toBase(), last.toBase()));
        }

		size_type erase(const key_type& k) { return base_type::erase(k); }

        using base_type::clear;
        using base_type::rehash;
        using base_type::reserve;

		iterator       find(const key_type& key) { return makeIt(base_type::find(key)); }
		const_iterator find(const key_type& key) const { return makeIt(base_type::find(key)); }

        using base_type::count;

		eastl::pair<iterator, iterator> equal_range(const key_type& k) {
            auto p = base_type::equal_range(k);
            return { makeIt(p.first), makeIt(p.second) };
        }

		eastl::pair<const_iterator, const_iterator> equal_range(const key_type& k) const {
            auto p = base_type::equal_range(k);
            return { makeIt(p.first), makeIt(p.second) };
        }

		using base_type::validate;
		int validate_iterator(const_base_iterator it) const noexcept { return base_type::validate_iterator(it); }
		//TODO: custom validation for safe iterators
		int validate_iterator(const const_stack_only_iterator& it) const noexcept { return base_type::validate_iterator(toBase(it)); }
		int validate_iterator(const const_heap_safe_iterator& it) const noexcept { return base_type::validate_iterator(toBase(it)); }

		bool operator==(const this_type& other) const { return eastl::operator==(this->toBase(), other.toBase()); }
		bool operator!=(const this_type& other) const {	return eastl::operator!=(this->toBase(), other.toBase()); }

		// iterator_safe make_safe(const iterator& it) {	return makeSafeIt(toBase(it)); }
		// const_iterator_safe make_safe(const const_iterator& it) const {	return makeSafeIt(toBase(it)); }

    protected:
		const base_type& toBase() const noexcept { return *this; }
		const_base_iterator toBase(const const_stack_only_iterator& it) const { return it.toBase(); }
		const_base_iterator toBase(const const_heap_safe_iterator& it) const { return it.toBase(); }

        iterator makeIt(base_iterator it) const {
			return iterator::fromBase(it);
        }

        eastl::pair<iterator, bool> makeIt(eastl::pair<base_iterator, bool> r) const {
            return eastl::pair<iterator, bool>(makeIt(r.first), r.second);
        }
	}; // unordered_set


	template <typename Key, typename Hash = hash<Key>, typename Predicate = equal_to<Key>, 
			  memory_safety Safety = safeness_declarator<Key>::is_safe>
	class SAFEMEMORY_DEEP_CONST_WHEN_PARAMS unordered_multiset
		: private eastl::unordered_multiset<Key, Hash, Predicate, detail::allocator_to_eastl_hashtable<Safety>>
	{
	public:
		typedef eastl::unordered_multiset<Key, Hash, Predicate, detail::allocator_to_eastl_hashtable<Safety>> base_type;
		typedef unordered_multiset<Key, Hash, Predicate, Safety>                    this_type;
		typedef typename base_type::size_type                                     size_type;
		typedef typename base_type::key_type                                      key_type;
		typedef typename base_type::mapped_type                                   mapped_type;
		typedef typename base_type::value_type                                    value_type;     // NOTE: 'value_type = pair<const key_type, mapped_type>'.
		typedef typename base_type::allocator_type                                allocator_type;
		typedef typename base_type::node_type                                     node_type;
		typedef typename base_type::iterator                                      base_iterator;
		typedef typename base_type::const_iterator                                const_base_iterator;
		typedef typename base_type::local_iterator                                local_iterator;
		typedef typename base_type::const_local_iterator                          const_local_iterator;
		typedef typename base_type::insert_return_type                            base_insert_return_type;

		typedef typename detail::hashtable_heap_safe_iterator<base_iterator, base_iterator, allocator_type>        heap_safe_iterator;
		typedef typename detail::hashtable_heap_safe_iterator<const_base_iterator, base_iterator, allocator_type>   const_heap_safe_iterator;
		typedef typename detail::hashtable_stack_only_iterator<base_iterator, base_iterator, allocator_type>       stack_only_iterator;
		typedef typename detail::hashtable_stack_only_iterator<const_base_iterator, base_iterator, allocator_type>  const_stack_only_iterator;

		// TODO: add 'use_base_iterator'

		typedef stack_only_iterator                                               iterator;
		typedef const_stack_only_iterator                                         const_iterator;
		typedef iterator                                                          insert_return_type;

		typedef heap_safe_iterator                                                iterator_safe;
		typedef const_heap_safe_iterator                                          const_iterator_safe;
		typedef iterator_safe                                                     insert_return_type_safe;

	public:
		explicit unordered_multiset(): base_type(allocator_type()) {}
	 	explicit unordered_multiset(size_type nBucketCount, const Hash& hashFunction = Hash(), 
						  const Predicate& predicate = Predicate())
			: base_type(nBucketCount, hashFunction, predicate, allocator_type())
		    {}
		unordered_multiset(const this_type& x) = default;
		unordered_multiset(this_type&& x) = default;
		unordered_multiset(std::initializer_list<value_type> ilist, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
				   const Predicate& predicate = Predicate())
			: base_type(ilist, nBucketCount, hashFunction, predicate, allocator_type())
            {}

		this_type& operator=(const this_type& x) = default;
		this_type& operator=(this_type&& x) = default;
		this_type& operator=(std::initializer_list<value_type> ilist)
            { return static_cast<this_type&>(base_type::operator=(ilist)); }

		void swap(this_type& x) { base_type::swap(x); }

		iterator       begin() noexcept { return makeIt(base_type::begin()); }
		const_iterator begin() const noexcept { return makeIt(base_type::begin()); }
		const_iterator cbegin() const noexcept { return makeIt(base_type::cbegin()); }

		iterator       end() noexcept { return makeIt(base_type::end()); }
		const_iterator end() const noexcept { return makeIt(base_type::end()); }
		const_iterator cend() const noexcept { return makeIt(base_type::cend()); }

		local_iterator begin(size_type n) noexcept { return base_type::begin(n); }
		const_local_iterator begin(size_type n) const noexcept { return base_type::begin(n); }
		const_local_iterator cbegin(size_type n) const noexcept { return base_type::cbegin(n); }

		local_iterator end(size_type n) noexcept { return base_type::end(n); }
		const_local_iterator end(size_type n) const noexcept { return base_type::end(n); }
		const_local_iterator cend(size_type n) const noexcept { return base_type::cend(n); }

        // using base_type::at;
        // using base_type::operator[];

        using base_type::empty;
        using base_type::size;
        using base_type::bucket_count;
        using base_type::bucket_size;

//        using base_type::bucket;
        using base_type::load_factor;
        using base_type::get_max_load_factor;
        using base_type::set_max_load_factor;
        using base_type::rehash_policy;

		template <class... Args>
		insert_return_type emplace(Args&&... args) {
            return makeIt(base_type::emplace(std::forward<Args>(args)...));
        }

		template <class... Args>
		iterator emplace_hint(const_iterator position, Args&&... args) {
            return makeIt(base_type::emplace_hint(position.toBase(), std::forward<Args>(args)...));
        }

		template <class... Args>
        insert_return_type try_emplace(const key_type& k, Args&&... args) {
            return makeIt(base_type::try_emplace(k, std::forward<Args>(args)...));
        }

		template <class... Args>
        insert_return_type try_emplace(key_type&& k, Args&&... args) {
            return makeIt(base_type::try_emplace(std::move(k), std::forward<Args>(args)...));
        }

		template <class... Args> 
        iterator try_emplace(const_iterator position, const key_type& k, Args&&... args) {
            return makeIt(base_type::try_emplace(position.toBase(), k, std::forward<Args>(args)...));
        }

		template <class... Args>
        iterator try_emplace(const_iterator position, key_type&& k, Args&&... args) {
            return makeIt(base_type::try_emplace(position.toBase(), std::move(k), std::forward<Args>(args)...));
        }

		insert_return_type insert(const value_type& value) {
            return makeIt(base_type::insert(value));
        }

		insert_return_type insert(value_type&& value) {
            return makeIt(base_type::insert(std::move(value)));
        }

		iterator insert(const_iterator hint, const value_type& value) {
            return makeIt(base_type::insert(hint.toBase(), value));
        }

		iterator insert(const_iterator hint, value_type&& value) {
            return makeIt(base_type::insert(hint.toBase(), std::move(value)));
        }

		void insert(std::initializer_list<value_type> ilist) {
            base_type::insert(ilist);
        }

		template <typename InputIterator>
        void insert_unsafe(InputIterator first, InputIterator last) {
            base_type::insert(first, last);
        }

		template <class M>
        insert_return_type insert_or_assign(const key_type& k, M&& obj) {
            return makeIt(base_type::insert_or_assign(k, std::forward<M>(obj)));
        }

		template <class M>
        insert_return_type insert_or_assign(key_type&& k, M&& obj) {
            return makeIt(base_type::insert_or_assign(std::move(k), std::forward<M>(obj)));
        }

		template <class M>
        iterator insert_or_assign(const_iterator hint, const key_type& k, M&& obj) {
            return makeIt(base_type::insert_or_assign(hint.toBase(), k, std::forward<M>(obj)));
        }

		template <class M>
        iterator insert_or_assign(const_iterator hint, key_type&& k, M&& obj) {
            return makeIt(base_type::insert_or_assign(hint.toBase(), std::move(k), std::forward<M>(obj)));
        }

		iterator erase(const_iterator position) {
            return makeIt(base_type::erase(position.toBase()));
        }

		iterator erase(const_iterator first, const_iterator last) {
            return makeIt(base_type::erase(first.toBase(), last.toBase()));
        }

		size_type erase(const key_type& k) { return base_type::erase(k); }

        using base_type::clear;
        using base_type::rehash;
        using base_type::reserve;

		iterator       find(const key_type& key) { return makeIt(base_type::find(key)); }
		const_iterator find(const key_type& key) const { return makeIt(base_type::find(key)); }

        using base_type::count;

		eastl::pair<iterator, iterator> equal_range(const key_type& k) {
            auto p = base_type::equal_range(k);
            return { makeIt(p.first), makeIt(p.second) };
        }

		eastl::pair<const_iterator, const_iterator> equal_range(const key_type& k) const {
            auto p = base_type::equal_range(k);
            return { makeIt(p.first), makeIt(p.second) };
        }

		using base_type::validate;
		int validate_iterator(const_base_iterator it) const noexcept { return base_type::validate_iterator(it); }
		//TODO: custom validation for safe iterators
		int validate_iterator(const const_stack_only_iterator& it) const noexcept { return base_type::validate_iterator(toBase(it)); }
		int validate_iterator(const const_heap_safe_iterator& it) const noexcept { return base_type::validate_iterator(toBase(it)); }

		bool operator==(const this_type& other) const { return eastl::operator==(this->toBase(), other.toBase()); }
		bool operator!=(const this_type& other) const {	return eastl::operator!=(this->toBase(), other.toBase()); }

		// iterator_safe make_safe(const iterator& it) const {	return makeSafeIt(toBase(it)); }
		// const_iterator_safe make_safe(const const_iterator& it) const {	return makeSafeIt(toBase(it)); }

    private:
		const base_type& toBase() const noexcept { return *this; }
		const_base_iterator toBase(const const_stack_only_iterator& it) const { return it.toBase(); }
		const_base_iterator toBase(const const_heap_safe_iterator& it) const { return it.toBase(); }

        iterator makeIt(base_iterator it) const {
			return iterator::fromBase(it);
        }

        eastl::pair<iterator, bool> makeIt(eastl::pair<base_iterator, bool> r) const {
            return eastl::pair<iterator, bool>(makeIt(r.first), r.second);
        }
	}; // unordered_multiset

	///////////////////////////////////////////////////////////////////////
	// global operators
	///////////////////////////////////////////////////////////////////////

	template <typename K, typename H, typename P, memory_safety S>
	inline void swap(const unordered_set<K, H, P, S>& a, 
					 const unordered_set<K, H, P, S>& b)
	{
		a.swap(b);
	}

	template <typename K, typename H, typename P, memory_safety S>
	inline void swap(const unordered_multiset<K, H, P, S>& a, 
					 const unordered_multiset<K, H, P, S>& b)
	{
		a.swap(b);
	}

} // namespace safememory


#endif //SAFE_MEMORY_UNORDERED_SET_H