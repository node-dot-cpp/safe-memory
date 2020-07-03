/* -------------------------------------------------------------------------------
* Copyright (c) 2019, OLogN Technologies AG
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

// Initial vesion from:
// https://github.com/electronicarts/EASTL/blob/3.15.00/include/EASTL/internal/hashtable.h

/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a hashtable, much like the C++11 unordered_set/unordered_map.
// proposed classes.
///////////////////////////////////////////////////////////////////////////////


#ifndef SAFE_MEMORY_EASTL_INTERNAL_HASHTABLE_H
#define SAFE_MEMORY_EASTL_INTERNAL_HASHTABLE_H

#include <safe_memory/safe_ptr.h>
#include <safe_memory/detail/safe_alloc.h>
#include <safe_memory/functional.h>

#include <type_traits>
#include <iterator>
#include <functional>
#include <utility>
#include <algorithm>
#include <initializer_list>
#include <tuple>
#include <string>

// EA_DISABLE_ALL_VC_WARNINGS()
// 	#include <new>
// 	#include <stddef.h>
// EA_RESTORE_ALL_VC_WARNINGS()

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4512)  // 'class' : assignment operator could not be generated.
	#pragma warning(disable: 4530)  // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
	#pragma warning(disable: 4571)  // catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught.
#endif


namespace safe_memory::detail
{
#if defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 303) && !defined(EA_COMPILER_RVCT)
	#define EASTL_MAY_ALIAS __attribute__((__may_alias__))
#else
	#define EASTL_MAY_ALIAS
#endif

	/// use_self
	///
	/// operator()(x) simply returns x. Used in sets, as opposed to maps.
	/// This is a template policy implementation; it is an alternative to 
	/// the use_first template implementation.
	///
	/// The existance of use_self may seem odd, given that it does nothing,
	/// but these kinds of things are useful, virtually required, for optimal 
	/// generic programming.
	///
	template <typename T>
	struct use_self             // : public unary_function<T, T> // Perhaps we want to make it a subclass of unary_function.
	{
		typedef T result_type;

		const T& operator()(const T& x) const
			{ return x; }
	};

	/// use_first
	///
	/// operator()(x) simply returns x.first. Used in maps, as opposed to sets.
	/// This is a template policy implementation; it is an alternative to 
	/// the use_self template implementation. This is the same thing as the
	/// SGI SGL select1st utility.
	///
	template <typename Pair>
	struct use_first
	{
		typedef Pair argument_type;
		typedef typename Pair::first_type result_type;

		const result_type& operator()(const Pair& x) const
			{ return x.first; }
	};

	/// hash_node
	///
	/// A hash_node stores an element in a hash table, much like a 
	/// linked list node stores an element in a linked list. 
	/// A hash_node additionally can, via template parameter,
	/// store a hash code in the node to speed up hash calculations 
	/// and comparisons in some cases.
	/// 
	template <typename Value, memory_safety Safety, bool bCacheHashCode>
	struct hash_node;

		template <typename Value, memory_safety Safety>
		struct hash_node<Value, Safety, true>
		{
			hash_node() = default;

			template<class... Args>
			hash_node(Args&&... args)
				:mValue(std::forward<Args>(args)...) 
				{}

			Value        mValue;
			owning_ptr<hash_node, Safety>   mpNext;
			std::size_t mnHashCode;

		} EASTL_MAY_ALIAS;

		template <typename Value, memory_safety Safety>
		struct hash_node<Value, Safety, false>
		{
			hash_node() = default;

			template<class... Args>
			hash_node(Args&&... args)
				:mValue(std::forward<Args>(args)...) 
				{}

		    Value      mValue;
			owning_ptr<hash_node, Safety> mpNext;

		} EASTL_MAY_ALIAS;

	// has_hashcode_member
	//
	// Custom type-trait that checks for the existence of a class data member 'mnHashCode'.  
	//
	// In order to explicitly instantiate the hashtable without error we need to SFINAE away the functions that will
	// fail to compile based on if the 'hash_node' contains a 'mnHashCode' member dictated by the hashtable template
	// parameters. The hashtable support this level of configuration to allow users to choose which between the space vs.
	// time optimization.
	//
	// namespace Internal
	// {
	// 	template <class T>
	// 	struct has_hashcode_member 
	// 	{
	// 	private:
	// 		template <class U> static safememory::no_type test(...);
	// 		template <class U> static safememory::yes_type test(decltype(U::mnHashCode)* = 0);
	// 	public:
	// 		static const bool value = sizeof(test<T>(0)) == sizeof(safememory::yes_type);
	// 	};
	// }
	
	// static_assert(Internal::has_hashcode_member<hash_node<int, true>>::value, "contains a mnHashCode member");
	// static_assert(!Internal::has_hashcode_member<hash_node<int, false>>::value, "doesn't contain a mnHashCode member");

	// convenience macros to increase the readability of the code paths that must SFINAE on if the 'hash_node'
	// contains the cached hashed value or not. 
	// #define ENABLE_IF_HAS_HASHCODE(T, RT) typename std::enable_if<Internal::has_hashcode_member<T>::value, RT>::type*
	// #define ENABLE_IF_HASHCODE_SIZET(T, RT) typename std::enable_if<std::is_convertible<T, size_t>::value, RT>::type
	#define SM_ENABLE_IF_TRUETYPE(T) typename std::enable_if<T::value>::type*
	#define SM_DISABLE_IF_TRUETYPE(T) typename std::enable_if<!T::value>::type*


	/// node_const_iterator
	///
	/// Node iterators iterate nodes within a given bucket.
	///
	/// We define the const iterator as a base class of the non-const.
	///
	template <typename Value, bool bCacheHashCode, memory_safety Safety>
	struct node_const_iterator
	{
		typedef node_const_iterator<Value, bCacheHashCode, Safety>    this_type;
		typedef hash_node<Value, Safety, bCacheHashCode> 			node_type;
		typedef Value                                                    value_type;
		typedef const Value*										 pointer;
		typedef const Value&	 									 reference;
		typedef std::ptrdiff_t                                           difference_type;
		typedef std::forward_iterator_tag          			             iterator_category;

		static constexpr memory_safety is_safe = Safety;

	private:
		template <typename, typename, memory_safety, typename, typename, typename, typename, typename, typename, bool, bool, bool>
		friend class hashtable;

	protected:
		soft_ptr_with_zero_offset<node_type, Safety> mpNode;

		explicit node_const_iterator(soft_ptr_with_zero_offset<node_type, Safety> pNode)
			: mpNode(pNode) { }

		void increment()
			{ mpNode = mpNode->mpNext; }

	public:
		node_const_iterator() { }

		node_const_iterator(const node_const_iterator&) = default;
		node_const_iterator& operator=(const node_const_iterator&) = default;
		node_const_iterator(node_const_iterator&&) = default;
		node_const_iterator& operator=(node_const_iterator&&) = default;

		reference operator*() const
			{ return mpNode->mValue; }

		pointer operator->() const
			{ return &(mpNode->mValue); }

		node_const_iterator& operator++()
			{ increment(); return *this; }

		node_const_iterator operator++(int)
			{ node_const_iterator temp(*this); increment(); return temp; }

		bool operator==(const node_const_iterator& other) const
			{ return mpNode == other.mpNode; }

		bool operator!=(const node_const_iterator& other) const
			{ return !operator==(other); }

	};



	/// node_iterator
	///
	/// Node iterators iterate nodes within a given bucket.
	///
	/// The the const_iterator is a base class.
	///
	template <typename Value, bool bCacheHashCode, memory_safety Safety>
	struct node_iterator : public node_const_iterator<Value, bCacheHashCode, Safety>
	{
	public:
		typedef node_const_iterator<Value, bCacheHashCode, Safety>       base_type;
		typedef node_iterator<Value, bCacheHashCode, Safety>    this_type;
		typedef typename base_type::node_type                            node_type;
		typedef typename base_type::value_type                            value_type;
		typedef Value* pointer;
		typedef Value& reference;
		typedef typename base_type::difference_type                         difference_type;
		typedef typename base_type::iterator_category    			             iterator_category;

		// static constexpr memory_safety is_safe = Safety;

	private:
		template <typename, typename, memory_safety, typename, typename, typename, typename, typename, typename, bool, bool, bool>
		friend class hashtable;

	public:
		node_iterator() { }

	private:
		explicit node_iterator(soft_ptr_with_zero_offset<node_type, Safety> pNode)
			: base_type(pNode) { }
	public:

		node_iterator(const node_iterator&) = default;
		node_iterator& operator=(const node_iterator&) = default;
		node_iterator(node_iterator&&) = default;
		node_iterator& operator=(node_iterator&&) = default;

		reference operator*() const
			{ return base_type::mpNode->mValue; }

		pointer operator->() const
			{ return &(base_type::mpNode->mValue); }

		node_iterator& operator++()
			{ base_type::increment(); return *this; }

		node_iterator operator++(int)
			{ node_iterator temp(*this); base_type::increment(); return temp; }

	}; // node_iterator



	/// hashtable_base_iterator
	///
	/// A hashtable_iterator iterates the entire hash table and not just
	/// nodes within a single bucket. Users in general will use a hash
	/// table iterator much more often, as it is much like other container
	/// iterators (e.g. vector::iterator).
	///
	/// We define the const_iterator as a base class.
	///
	template <typename Value, bool bConst, bool bCacheHashCode, memory_safety Safety>
	struct hashtable_base_iterator
	{
	public:
		typedef hashtable_base_iterator<Value, bConst, bCacheHashCode, Safety> this_type;
		typedef hash_node<Value, Safety, bCacheHashCode>              		node_type;

		typedef Value                                                    value_type;
		// typedef const Value*									 		 pointer;
		// typedef const Value&									 		 reference;
		typedef std::conditional_t<bConst, const Value*, Value*> 		pointer;
		typedef std::conditional_t<bConst, const Value&, Value&> 		reference;

		typedef std::ptrdiff_t                                           difference_type;
		typedef std::forward_iterator_tag                                iterator_category;


		typedef soft_ptr_with_zero_offset<node_type, Safety>	node_ptr;
		typedef safe_array_iterator2<owning_ptr<node_type, Safety>, Safety>	bucket_it;

		static constexpr memory_safety is_safe = Safety;


	protected:
	public:
		template <typename, typename, memory_safety, typename, typename, typename, typename, typename, typename, bool, bool, bool>
		friend class hashtable;

		node_ptr  mpNode;      // Current node within current bucket.
		bucket_it mpBucket;    // Current bucket.

		hashtable_base_iterator(node_ptr pNode, bucket_it pBucket)
			: mpNode(pNode), mpBucket(pBucket) { }
		hashtable_base_iterator(std::nullptr_t nulp, bucket_it pBucket)
			: mpNode(nulp), mpBucket(pBucket) { }

	public:
		hashtable_base_iterator() { }

		hashtable_base_iterator(const hashtable_base_iterator& x) = default;
		hashtable_base_iterator& operator=(const hashtable_base_iterator& x) = default;

		hashtable_base_iterator(hashtable_base_iterator&&) = default;
		hashtable_base_iterator& operator=(hashtable_base_iterator&&) = default;

	// allow non-const to const convertion
	template<typename V, typename X = std::enable_if_t<bConst>>
	hashtable_base_iterator(const hashtable_base_iterator<V, false, bCacheHashCode, Safety>& ri)
		: mpNode(ri.mpNode), mpBucket(ri.mpBucket) {}

	// allow non-const to const convertion
	template<typename V, typename X = std::enable_if_t<bConst>>
	hashtable_base_iterator& operator=(const hashtable_base_iterator<V, false, bCacheHashCode, Safety>& ri) {
		this->mpNode = ri.mpNode;
		this->mpBucket = ri.mpBucket;
		return *this;
	}



	protected:
		void increment_bucket_if_null()
		{
			while(mpNode == nullptr) { // We store an extra bucket at the end 
				++mpBucket;
				if(mpBucket.is_end())
					return;          // of the bucket array so that finding the end of the bucket
				mpNode = *mpBucket;      // array is quick and simple.
			}
		}

		void increment()
		{
			mpNode = mpNode->mpNext;
			increment_bucket_if_null();
		}

	public:
		reference operator*() const
			{ return mpNode->mValue; }

		pointer operator->() const
			{ return &(mpNode->mValue); }

		this_type& operator++()
			{ increment(); return *this; }

		this_type operator++(int)
			{ this_type temp(*this); increment(); return temp; }

		bool operator==(const this_type& other) const
			{ return mpNode == other.mpNode && mpBucket == other.mpBucket; }

		bool operator!=(const this_type& other) const
			{ return !operator==(other); }
		
	}; // hashtable_base_iterator

	template <typename Value, bool bCacheHashCode, memory_safety Safety>
	using hashtable_iterator = hashtable_base_iterator<Value, false, bCacheHashCode, Safety>;

	template <typename Value, bool bCacheHashCode, memory_safety Safety>
	using hashtable_const_iterator = hashtable_base_iterator<Value, true, bCacheHashCode, Safety>;



	/// ht_distance
	///
	/// This function returns the same thing as distance() for 
	/// forward iterators but returns zero for input iterators.
	/// The reason why is that input iterators can only be read
	/// once, and calling distance() on an input iterator destroys
	/// the ability to read it. This ht_distance is used only for
	/// optimization and so the code will merely work better with
	/// forward iterators that input iterators.
	///
	template <typename Iterator>
	inline typename std::iterator_traits<Iterator>::difference_type
	distance_fw_impl(Iterator /*first*/, Iterator /*last*/, std::input_iterator_tag)
	{
		return 0;
	}

	template <typename Iterator>
	inline typename std::iterator_traits<Iterator>::difference_type
	distance_fw_impl(Iterator first, Iterator last, std::forward_iterator_tag)
		{ return std::distance(first, last); }

	template <typename Iterator>
	inline typename std::iterator_traits<Iterator>::difference_type
	ht_distance(Iterator first, Iterator last)
	{
		typedef typename std::iterator_traits<Iterator>::iterator_category IC;
		return distance_fw_impl(first, last, IC());
	}




	/// mod_range_hashing
	///
	/// Implements the algorithm for conversion of a number in the range of
	/// [0, SIZE_T_MAX] to the range of [0, BucketCount).
	///
	struct mod_range_hashing
	{
		uint32_t operator()(std::size_t r, uint32_t n) const
			{ return r % n; }
	};


	/// default_ranged_hash
	///
	/// Default ranged hash function H. In principle it should be a
	/// function object composed from objects of type H1 and H2 such that
	/// h(k, n) = h2(h1(k), n), but that would mean making extra copies of
	/// h1 and h2. So instead we'll just use a tag to tell class template
	/// hashtable to do that composition.
	///
	struct default_ranged_hash{ };


	/// prime_rehash_policy
	///
	/// Default value for rehash policy. Bucket size is (usually) the
	/// smallest prime that keeps the load factor small enough.
	///
	struct prime_rehash_policy
	{
	public:
		float            mfMaxLoadFactor;
		float            mfGrowthFactor;
		mutable uint32_t mnNextResize;

	public:
		prime_rehash_policy(float fMaxLoadFactor = 1.f)
			: mfMaxLoadFactor(fMaxLoadFactor), mfGrowthFactor(2.f), mnNextResize(0) { }

		float GetMaxLoadFactor() const
			{ return mfMaxLoadFactor; }

		/// Return a bucket count no greater than nBucketCountHint, 
		/// Don't update member variables while at it.
		static uint32_t GetPrevBucketCountOnly(uint32_t nBucketCountHint);

		/// Return a bucket count no greater than nBucketCountHint.
		/// This function has a side effect of updating mnNextResize.
		uint32_t GetPrevBucketCount(uint32_t nBucketCountHint) const;

		/// Return a bucket count no smaller than nBucketCountHint.
		/// This function has a side effect of updating mnNextResize.
		uint32_t GetNextBucketCount(uint32_t nBucketCountHint) const;

		/// Return a bucket count appropriate for nElementCount elements.
		/// This function has a side effect of updating mnNextResize.
		uint32_t GetBucketCount(uint32_t nElementCount) const;

		/// nBucketCount is current bucket count, nElementCount is current element count,
		/// and nElementAdd is number of elements to be inserted. Do we need 
		/// to increase bucket count? If so, return pair(true, n), where 
		/// n is the new bucket count. If not, return pair(false, 0).
		std::pair<bool, uint32_t>
		GetRehashRequired(uint32_t nBucketCount, uint32_t nElementCount, uint32_t nElementAdd) const;
	};





	///////////////////////////////////////////////////////////////////////
	// Base classes for hashtable. We define these base classes because 
	// in some cases we want to do different things depending on the 
	// value of a policy class. In some cases the policy class affects
	// which member functions and nested typedefs are defined; we handle that
	// by specializing base class templates. Several of the base class templates
	// need to access other members of class template hashtable, so we use
	// the "curiously recurring template pattern" (parent class is templated 
	// on type of child class) for them.
	///////////////////////////////////////////////////////////////////////


	/// rehash_base
	///
	/// Give hashtable the get_max_load_factor functions if the rehash 
	/// policy is prime_rehash_policy.
	///
	template <typename RehashPolicy, typename Hashtable>
	struct rehash_base { };

	template <typename Hashtable>
	struct rehash_base<prime_rehash_policy, Hashtable>
	{
		// Returns the max load factor, which is the load factor beyond
		// which we rebuild the container with a new bucket count.
		float get_max_load_factor() const
		{
			const Hashtable* const pThis = static_cast<const Hashtable*>(this);
			return pThis->rehash_policy().GetMaxLoadFactor();
		}

		// If you want to make the hashtable never rehash (resize), 
		// set the max load factor to be a very high number (e.g. 100000.f).
		void set_max_load_factor(float fMaxLoadFactor)
		{
			Hashtable* const pThis = static_cast<Hashtable*>(this);
			pThis->rehash_policy(prime_rehash_policy(fMaxLoadFactor));    
		}
	};




	/// hash_code_base
	///
	/// Encapsulates two policy issues that aren't quite orthogonal.
	///   (1) The difference between using a ranged hash function and using
	///       the combination of a hash function and a range-hashing function.
	///       In the former case we don't have such things as hash codes, so
	///       we have a dummy type as placeholder.
	///   (2) Whether or not we cache hash codes. Caching hash codes is
	///       meaningless if we have a ranged hash function. This is because
	///       a ranged hash function converts an object directly to its
	///       bucket index without ostensibly using a hash code.
	/// We also put the key extraction and equality comparison function 
	/// objects here, for convenience.
	///
	template <typename Key, typename Value, typename ExtractKey, typename Equal, 
			  typename H1, typename H2, typename H, memory_safety S, bool bCacheHashCode>
	struct hash_code_base;


	/// hash_code_base
	///
	/// Specialization: ranged hash function, no caching hash codes. 
	/// H1 and H2 are provided but ignored. We define a dummy hash code type.
	///
	template <typename Key, typename Value, typename ExtractKey, typename Equal, typename H1, typename H2, typename H, memory_safety Safety>
	struct hash_code_base<Key, Value, ExtractKey, Equal, H1, H2, H, Safety, false>
	{
	protected:
		ExtractKey  mExtractKey;    // To do: Make this member go away entirely, as it never has any data.
		Equal       mEqual;         // To do: Make this instance use zero space when it is zero size.
		H           mRangedHash;    // To do: Make this instance use zero space when it is zero size

	public:
		H1 hash_function() const
			{ return H1(); }

		Equal equal_function() const // Deprecated. Use key_eq() instead, as key_eq is what the new C++ standard 
			{ return mEqual; }       // has specified in its hashtable (unordered_*) proposal.

		const Equal& key_eq() const
			{ return mEqual; }

		Equal& key_eq()
			{ return mEqual; }

	protected:
		typedef void*    hash_code_t;
		typedef uint32_t bucket_index_t;

		hash_code_base(const ExtractKey& extractKey, const Equal& eq, const H1&, const H2&, const H& h)
			: mExtractKey(extractKey), mEqual(eq), mRangedHash(h) { }

		hash_code_t get_hash_code(const Key&) const
		{
			// EA_UNUSED(key);
			return 0;
		}

		bucket_index_t bucket_index(hash_code_t, uint32_t) const
			{ return (bucket_index_t)0; }

		bucket_index_t bucket_index(const Key& key, hash_code_t, uint32_t nBucketCount) const
			{ return (bucket_index_t)mRangedHash(key, nBucketCount); }

		bucket_index_t bucket_index(const hash_node<Value, Safety, false>& pNode, uint32_t nBucketCount) const
			{ return (bucket_index_t)mRangedHash(mExtractKey(pNode.mValue), nBucketCount); }

		bool compare(const Key& key, hash_code_t, hash_node<Value, Safety, false>& pNode) const
			{ return mEqual(key, mExtractKey(pNode.mValue)); }

		void copy_code(hash_node<Value, Safety, false>&, const hash_node<Value, Safety, false>&) const
			{ } // Nothing to do.

		void set_code(hash_node<Value, Safety, false>&, hash_code_t) const
		{
			// EA_UNUSED(pDest);
			// EA_UNUSED(c);
		}

		void base_swap(hash_code_base& x)
		{
			std::swap(mExtractKey, x.mExtractKey);
			std::swap(mEqual,      x.mEqual);
			std::swap(mRangedHash, x.mRangedHash);
		}

	}; // hash_code_base



	// No specialization for ranged hash function while caching hash codes.
	// That combination is meaningless, and trying to do it is an error.


	/// hash_code_base
	///
	/// Specialization: ranged hash function, cache hash codes. 
	/// This combination is meaningless, so we provide only a declaration
	/// and no definition.
	///
	template <typename Key, typename Value, typename ExtractKey, typename Equal, typename H1, typename H2, typename H, memory_safety Safety>
	struct hash_code_base<Key, Value, ExtractKey, Equal, H1, H2, H, Safety, true>;



	/// hash_code_base
	///
	/// Specialization: hash function and range-hashing function, 
	/// no caching of hash codes. H is provided but ignored. 
	/// Provides typedef and accessor required by TR1.
	///
	template <typename Key, typename Value, typename ExtractKey, typename Equal, typename H1, typename H2, memory_safety Safety>
	struct hash_code_base<Key, Value, ExtractKey, Equal, H1, H2, default_ranged_hash, Safety, false>
	{
	protected:
		ExtractKey  mExtractKey;
		Equal       mEqual;
		H1          m_h1;
		H2          m_h2;

	public:
		typedef H1 hasher;

		H1 hash_function() const
			{ return m_h1; }

		Equal equal_function() const // Deprecated. Use key_eq() instead, as key_eq is what the new C++ standard 
			{ return mEqual; }       // has specified in its hashtable (unordered_*) proposal.

		const Equal& key_eq() const
			{ return mEqual; }

		Equal& key_eq()
			{ return mEqual; }

	protected:
		typedef std::size_t hash_code_t;
		typedef uint32_t bucket_index_t;
		typedef hash_node<Value, Safety, false> node_type;

		hash_code_base(const ExtractKey& ex, const Equal& eq, const H1& h1, const H2& h2, const default_ranged_hash&)
			: mExtractKey(ex), mEqual(eq), m_h1(h1), m_h2(h2) { }

		hash_code_t get_hash_code(const Key& key) const
			{ return (hash_code_t)m_h1(key); }

		bucket_index_t bucket_index(hash_code_t c, uint32_t nBucketCount) const
			{ return (bucket_index_t)m_h2(c, nBucketCount); }

		bucket_index_t bucket_index(const Key&, hash_code_t c, uint32_t nBucketCount) const
			{ return (bucket_index_t)m_h2(c, nBucketCount); }

		bucket_index_t bucket_index(const node_type& pNode, uint32_t nBucketCount) const
			{ return (bucket_index_t)m_h2((hash_code_t)m_h1(mExtractKey(pNode.mValue)), nBucketCount); }

		bool compare(const Key& key, hash_code_t, const node_type& pNode) const
			{ return mEqual(key, mExtractKey(pNode.mValue)); }

		void copy_code(node_type&, const node_type&) const
			{ } // Nothing to do.

		void set_code(node_type&, hash_code_t) const
			{ } // Nothing to do.

		void base_swap(hash_code_base& x)
		{
			std::swap(mExtractKey, x.mExtractKey);
			std::swap(mEqual,      x.mEqual);
			std::swap(m_h1,        x.m_h1);
			std::swap(m_h2,        x.m_h2);
		}

	}; // hash_code_base



	/// hash_code_base
	///
	/// Specialization: hash function and range-hashing function, 
	/// caching hash codes. H is provided but ignored. 
	/// Provides typedef and accessor required by TR1.
	///
	template <typename Key, typename Value, typename ExtractKey, typename Equal, typename H1, typename H2, memory_safety Safety>
	struct hash_code_base<Key, Value, ExtractKey, Equal, H1, H2, default_ranged_hash, Safety, true>
	{
	protected:
		ExtractKey  mExtractKey;
		Equal       mEqual;
		H1          m_h1;
		H2          m_h2;

	public:
		typedef H1 hasher;

		H1 hash_function() const
			{ return m_h1; }

		Equal equal_function() const // Deprecated. Use key_eq() instead, as key_eq is what the new C++ standard 
			{ return mEqual; }       // has specified in its hashtable (unordered_*) proposal.

		const Equal& key_eq() const
			{ return mEqual; }

		Equal& key_eq()
			{ return mEqual; }

	protected:
		typedef uint32_t hash_code_t;
		typedef uint32_t bucket_index_t;
		typedef hash_node<Value, Safety, true> node_type;

		hash_code_base(const ExtractKey& ex, const Equal& eq, const H1& h1, const H2& h2, const default_ranged_hash&)
			: mExtractKey(ex), mEqual(eq), m_h1(h1), m_h2(h2) { }

		hash_code_t get_hash_code(const Key& key) const
			{ return (hash_code_t)m_h1(key); }

		bucket_index_t bucket_index(hash_code_t c, uint32_t nBucketCount) const
			{ return (bucket_index_t)m_h2(c, nBucketCount); }

		bucket_index_t bucket_index(const Key&, hash_code_t c, uint32_t nBucketCount) const
			{ return (bucket_index_t)m_h2(c, nBucketCount); }

		bucket_index_t bucket_index(const node_type& pNode, uint32_t nBucketCount) const
			{ return (bucket_index_t)m_h2((uint32_t)pNode.mnHashCode, nBucketCount); }

		bool compare(const Key& key, hash_code_t c, node_type& pNode) const
			{ return (pNode.mnHashCode == c) && mEqual(key, mExtractKey(pNode.mValue)); }

		void copy_code(node_type& pDest, const node_type& pSource) const
			{ pDest.mnHashCode = pSource.mnHashCode; }

		void set_code(node_type& pDest, hash_code_t c) const
			{ pDest.mnHashCode = c; }

		void base_swap(hash_code_base& x)
		{
			std::swap(mExtractKey, x.mExtractKey);
			std::swap(mEqual,      x.mEqual);
			std::swap(m_h1,        x.m_h1);
			std::swap(m_h2,        x.m_h2);
		}

	}; // hash_code_base





	///////////////////////////////////////////////////////////////////////////
	/// hashtable
	///
	/// Key and Value: arbitrary CopyConstructible types.
	///
	/// Safety: enum to enable or disable safe iterators
	///
	/// ExtractKey: function object that takes a object of type Value
	/// and returns a value of type Key.
	///
	/// Equal: function object that takes two objects of type k and returns
	/// a bool-like value that is true if the two objects are considered equal.
	///
	/// H1: a hash function. A unary function object with argument type
	/// Key and result type size_t. Return values should be distributed
	/// over the entire range [0, numeric_limits<uint32_t>::max()].
	///
	/// H2: a range-hashing function (in the terminology of Tavori and
	/// Dreizin). This is a function which takes the output of H1 and 
	/// converts it to the range of [0, n]. Usually it merely takes the
	/// output of H1 and mods it to n.
	///
	/// H: a ranged hash function (Tavori and Dreizin). This is merely
	/// a class that combines the functionality of H1 and H2 together, 
	/// possibly in some way that is somehow improved over H1 and H2
	/// It is a binary function whose argument types are Key and size_t 
	/// and whose result type is uint32_t. Given arguments k and n, the 
	/// return value is in the range [0, n). Default: h(k, n) = h2(h1(k), n). 
	/// If H is anything other than the default, H1 and H2 are ignored, 
	/// as H is thus overriding H1 and H2.
	///
	/// RehashPolicy: Policy class with three members, all of which govern
	/// the bucket count. nBucket(n) returns a bucket count no smaller
	/// than n. GetBucketCount(n) returns a bucket count appropriate
	/// for an element count of n. GetRehashRequired(nBucketCount, nElementCount, nElementAdd)
	/// determines whether, if the current bucket count is nBucket and the
	/// current element count is nElementCount, we need to increase the bucket
	/// count. If so, returns pair(true, n), where n is the new
	/// bucket count. If not, returns pair(false, <anything>).
	///
	/// Currently it is hard-wired that the number of buckets never
	/// shrinks. Should we allow RehashPolicy to change that?
	///
	/// bCacheHashCode: true if we store the value of the hash
	/// function along with the value. This is a time-space tradeoff.
	/// Storing it may improve lookup speed by reducing the number of 
	/// times we need to call the Equal function.
	///
	/// bMutableIterators: true if hashtable::iterator is a mutable
	/// iterator, false if iterator and const_iterator are both const 
	/// iterators. This is true for hash_map and hash_multimap,
	/// false for hash_set and hash_multiset.
	///
	/// bUniqueKeys: true if the return value of hashtable::count(k)
	/// is always at most one, false if it may be an arbitrary number. 
	/// This is true for hash_set and hash_map and is false for 
	/// hash_multiset and hash_multimap.
	///
	///////////////////////////////////////////////////////////////////////
	/// Note:
	/// If you want to make a hashtable never increase its bucket usage,
	/// call set_max_load_factor with a very high value such as 100000.f.
	///
	/// find_as
	/// In order to support the ability to have a hashtable of strings but
	/// be able to do efficiently lookups via char pointers (i.e. so they 
	/// aren't converted to string objects), we provide the find_as 
	/// function. This function allows you to do a find with a key of a
	/// type other than the hashtable key type. See the find_as function
	/// for more documentation on this.
	///
	/// find_by_hash
	/// In the interest of supporting fast operations wherever possible,
	/// we provide a find_by_hash function which finds a node using its
	/// hash code.  This is useful for cases where the node's hash is
	/// already known, allowing us to avoid a redundant hash operation
	/// in the normal find path.
	/// 
	template <typename Key, typename Value, memory_safety Safety, typename ExtractKey, 
			  typename Equal, typename H1, typename H2, typename H, 
			  typename RehashPolicy, bool bCacheHashCode, bool bMutableIterators, bool bUniqueKeys>
	class hashtable
		:   public rehash_base<RehashPolicy, hashtable<Key, Value, Safety, ExtractKey, Equal, H1, H2, H, RehashPolicy, bCacheHashCode, bMutableIterators, bUniqueKeys> >,
			public hash_code_base<Key, Value, ExtractKey, Equal, H1, H2, H, Safety, bCacheHashCode>
	{
	public:
		typedef Key                                                                                 key_type;
		typedef Value                                                                               value_type;
		typedef typename ExtractKey::result_type                                                    mapped_type;
		typedef hash_code_base<Key, Value, ExtractKey, Equal, H1, H2, H, Safety, bCacheHashCode>            hash_code_base_type;
		typedef typename hash_code_base_type::hash_code_t                                           hash_code_t;

		typedef Equal                                                                               key_equal;
		typedef std::ptrdiff_t                                                                           difference_type;
		typedef std::size_t                                                                              size_type;
		typedef value_type&                                                                         reference;
		typedef const value_type&                                                                   const_reference;

		typedef node_const_iterator<value_type, bCacheHashCode, Safety>              const_local_iterator;
		typedef std::conditional_t<bMutableIterators, node_iterator<value_type, bCacheHashCode, Safety>, const_local_iterator> local_iterator;

		typedef hashtable_const_iterator<value_type, bCacheHashCode, Safety>         const_iterator;
		typedef std::conditional_t<bMutableIterators, hashtable_iterator<value_type, bCacheHashCode, Safety>, const_iterator> iterator;

		typedef hash_node<value_type, Safety, bCacheHashCode>                                               node_type;
		typedef typename std::conditional_t<bUniqueKeys, std::pair<iterator, bool>, iterator>       insert_return_type;
		typedef hashtable<Key, Value, Safety, ExtractKey, Equal, H1, H2, H, 
							RehashPolicy, bCacheHashCode, bMutableIterators, bUniqueKeys>           this_type;
		typedef RehashPolicy                                                                        rehash_policy_type;
		typedef ExtractKey                                                                          extract_key_type;
		typedef H1                                                                                  h1_type;
		typedef H2                                                                                  h2_type;
		typedef H                                                                                   h_type;
		typedef std::integral_constant<bool, bUniqueKeys>                                           has_unique_keys_type;

		typedef owning_ptr<node_type, Safety>                                                      owning_node_type;
		typedef soft_ptr_with_zero_offset<node_type, Safety>                               soft_node_type;
		typedef owning_ptr<array_of2<owning_node_type, Safety>, Safety>                                    owning_bucket_type;
		typedef soft_ptr_with_zero_offset<array_of2<owning_node_type, Safety>, Safety>             soft_bucket_type;



		using hash_code_base_type::key_eq;
		using hash_code_base_type::hash_function;
		using hash_code_base_type::mExtractKey;
		using hash_code_base_type::get_hash_code;
		using hash_code_base_type::bucket_index;
		using hash_code_base_type::compare;
		using hash_code_base_type::set_code;
		using hash_code_base_type::copy_code;
		typedef typename hash_code_base_type::bucket_index_t 	bucket_index_t;

		static const bool kCacheHashCode = bCacheHashCode;
		static constexpr memory_safety is_safe = Safety;

	protected:
		owning_bucket_type     mpBucketArray;
		size_type       mnBucketCount;
		size_type       mnElementCount;
		RehashPolicy    mRehashPolicy;  // To do: Use base class optimization to make this go away.


	public:
		hashtable(size_type nBucketCount, const H1&, const H2&, const H&, const Equal&, const ExtractKey&/*, 
				  const allocator_type& allocator = EASTL_HASHTABLE_DEFAULT_ALLOCATOR*/);
		
		// template <typename FowardIterator>
		// hashtable(FowardIterator first, FowardIterator last, size_type nBucketCount, 
		// 		  const H1&, const H2&, const H&, const Equal&, const ExtractKey&/*, 
		// 		  const allocator_type& allocator = EASTL_HASHTABLE_DEFAULT_ALLOCATOR*/); 
		
		hashtable(const hashtable& x);

		// initializer_list ctor support is implemented in subclasses (e.g. hash_set).
		// hashtable(initializer_list<value_type>, size_type nBucketCount, const H1&, const H2&, const H&, 
		//           const Equal&, const ExtractKey&, const allocator_type& allocator = EASTL_HASHTABLE_DEFAULT_ALLOCATOR);

		hashtable(this_type&& x);
		// hashtable(this_type&& x, const allocator_type& allocator);
	   ~hashtable();

		// const allocator_type& get_allocator() const noexcept;
		// allocator_type&       get_allocator() noexcept;
		// void                  set_allocator(const allocator_type& allocator);

		this_type& operator=(const this_type& x);
		this_type& operator=(std::initializer_list<value_type> ilist);
		this_type& operator=(this_type&& x);

		void swap(this_type& x);

		iterator begin() noexcept
		{
			auto it = GetBucketArrayIt(0);
			iterator i(*it, it);
			i.increment_bucket_if_null();
			return i;
		}

		const_iterator begin() const noexcept
			{ return cbegin(); }

		const_iterator cbegin() const noexcept
		{
			auto it = GetBucketArrayIt(0);
			const_iterator i(*it, it);
			i.increment_bucket_if_null();
			return i;
		}

		iterator end() noexcept
			{ return iterator(nullptr, GetBucketArrayIt(mnBucketCount)); }

		const_iterator end() const noexcept
			{ return cend(); }

		const_iterator cend() const noexcept
			{ return const_iterator(nullptr, GetBucketArrayIt(mnBucketCount)); }

		// Returns an iterator to the first item in bucket n.
		local_iterator begin(size_type n) noexcept
			{ return local_iterator(mpBucketArray->at(n)); }

		const_local_iterator begin(size_type n) const noexcept
			{ return const_local_iterator(mpBucketArray->at(n)); }

		const_local_iterator cbegin(size_type n) const noexcept
			{ return const_local_iterator(mpBucketArray->at(n)); }

		// Returns an iterator to the last item in a bucket returned by begin(n).
		local_iterator end(size_type) noexcept
			{ return local_iterator(); }

		const_local_iterator end(size_type) const noexcept
			{ return const_local_iterator(); }

		const_local_iterator cend(size_type) const noexcept
			{ return const_local_iterator(); }

		bool empty() const noexcept
			{ return mnElementCount == 0; }

		size_type size() const noexcept
			{ return mnElementCount; }

		size_type bucket_count() const noexcept
			{ return mnBucketCount; }

		size_type bucket_size(size_type n) const noexcept
			{ return (size_type)std::distance(begin(n), end(n)); }

		//size_type bucket(const key_type& k) const noexcept
		//    { return bucket_index(k, (hash code here), (uint32_t)mnBucketCount); }

		// Returns the ratio of element count to bucket count. A return value of 1 means 
		// there's an optimal 1 bucket for each element.
		float load_factor() const noexcept
			{ return (float)mnElementCount / (float)mnBucketCount; }

		// Inherited from the base class.
		// Returns the max load factor, which is the load factor beyond
		// which we rebuild the container with a new bucket count.
		// get_max_load_factor comes from rehash_base.
		//    float get_max_load_factor() const;

		// Inherited from the base class.
		// If you want to make the hashtable never rehash (resize), 
		// set the max load factor to be a very high number (e.g. 100000.f).
		// set_max_load_factor comes from rehash_base.
		//    void set_max_load_factor(float fMaxLoadFactor);

		/// Generalization of get_max_load_factor. This is an extension that's
		/// not present in C++ hash tables (unordered containers).
		const rehash_policy_type& rehash_policy() const noexcept
			{ return mRehashPolicy; }

		/// Generalization of set_max_load_factor. This is an extension that's
		/// not present in C++ hash tables (unordered containers).
		void rehash_policy(const rehash_policy_type& rehashPolicy);

		template <class... Args>
		insert_return_type emplace(Args&&... args);

		template <class... Args>
		iterator emplace_hint(const_iterator position, Args&&... args);

		template <class... Args> insert_return_type try_emplace(const key_type& k, Args&&... args);
		template <class... Args> insert_return_type try_emplace(key_type&& k, Args&&... args);
		template <class... Args> iterator           try_emplace(const_iterator position, const key_type& k, Args&&... args);
		template <class... Args> iterator           try_emplace(const_iterator position, key_type&& k, Args&&... args);

		insert_return_type                     insert(const value_type& value);
		insert_return_type                     insert(value_type&& otherValue);
		iterator                               insert(const_iterator hint, const value_type& value);
		iterator                               insert(const_iterator hint, value_type&& value);
		void                                   insert(std::initializer_list<value_type> ilist);
		template <typename InputIterator> void insert_unsafe(InputIterator first, InputIterator last);
	  //insert_return_type                     insert(node_type&& nh);
	  //iterator                               insert(const_iterator hint, node_type&& nh);

		// This overload attempts to mitigate the overhead associated with mismatched cv-quality elements of
		// the hashtable pair. It can avoid copy overhead because it will perfect forward the user provided pair types
		// until it can constructed in-place in the allocated hashtable node.  
		//
		// Ideally we would remove this overload as it deprecated and removed in C++17 but it currently causes
		// performance regressions for hashtables with complex keys (keys that allocate resources).
		// template <class P,
		//           class = typename std::enable_if_t<
		// 			#if EASTL_ENABLE_PAIR_FIRST_ELEMENT_CONSTRUCTOR
		//               !std::is_same_v<std::decay_t<P>, key_type> &&
		// 			#endif
		//               !std::is_literal_type_v<P> &&
		//               std::is_constructible_v<value_type, P&&>>>
		// insert_return_type insert(P&& otherValue);

		// Non-standard extension
		// template <class P> // See comments below for the const value_type& equivalent to this function.
		// insert_return_type insert(hash_code_t c, owning_node_type pNodeNew, P&& otherValue);

		// We provide a version of insert which lets the caller directly specify the hash value and 
		// a potential node to insert if needed. This allows for less thread contention in the case
		// of a thread-shared hash table that's accessed during a mutex lock, because the hash calculation
		// and node creation is done outside of the lock. If pNodeNew is supplied by the user (i.e. non-NULL) 
		// then it must be freeable via the hash table's allocator. If the return value is true then this function 
		// took over ownership of pNodeNew, else pNodeNew is still owned by the caller to free or to pass 
		// to another call to insert. pNodeNew need not be assigned the value by the caller, as the insert
		// function will assign value to pNodeNew upon insertion into the hash table. pNodeNew may be 
		// created by the user with the allocate_uninitialized_node function, and freed by the free_uninitialized_node function.
		// insert_return_type insert(hash_code_t c, owning_node_type pNodeNew, const value_type& value);

		template <class M> std::pair<iterator, bool> insert_or_assign(const key_type& k, M&& obj);
		template <class M> std::pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj);
		template <class M> iterator                    insert_or_assign(const_iterator hint, const key_type& k, M&& obj);
		template <class M> iterator                    insert_or_assign(const_iterator hint, key_type&& k, M&& obj);

		// Used to allocate and free memory used by insert(const value_type& value, hash_code_t c, node_type* pNodeNew).
		// owning_node_type allocate_uninitialized_node();
		// void       free_uninitialized_node(owning_node_type pNode);

		iterator         erase(const_iterator position);
		iterator         erase(const_iterator first, const_iterator last);
		size_type        erase(const key_type& k);

		void clear();
		void clear(bool clearBuckets);                  // If clearBuckets is true, we free the bucket memory and set the bucket count back to the newly constructed count.
		// void reset_lose_memory() noexcept;           // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.
		void rehash(size_type nBucketCount);
		void reserve(size_type nElementCount);

		iterator       find(const key_type& key);
		const_iterator find(const key_type& key) const;

		/// Implements a find whereby the user supplies a comparison of a different type
		/// than the hashtable value_type. A useful case of this is one whereby you have
		/// a container of string objects but want to do searches via passing in char pointers.
		/// The problem is that without this kind of find, you need to do the expensive operation
		/// of converting the char pointer to a string so it can be used as the argument to the 
		/// find function.
		///
		/// Example usage (namespaces omitted for brevity):
		///     hash_set<string> hashSet;
		///     hashSet.find_as("hello");    // Use default hash and compare.
		///
		/// Example usage (note that the predicate uses string as first type and char* as second):
		///     hash_set<string> hashSet;
		///     hashSet.find_as("hello", hash<char*>(), equal_to_2<string, char*>());
		///
		// template <typename U, typename UHash, typename BinaryPredicate>
		// iterator       find_as(const U& u, UHash uhash, BinaryPredicate predicate);

		// template <typename U, typename UHash, typename BinaryPredicate>
		// const_iterator find_as(const U& u, UHash uhash, BinaryPredicate predicate) const;

		// template <typename U>
		// iterator       find_as(const U& u);

		// template <typename U>
		// const_iterator find_as(const U& u) const;

		// Note: find_by_hash and find_range_by_hash both perform a search based on a hash value.
		// It is important to note that multiple hash values may map to the same hash bucket, so
		// it would be incorrect to assume all items returned match the hash value that
		// was searched for.

		/// Implements a find whereby the user supplies the node's hash code.
		/// It returns an iterator to the first element that matches the given hash. However, there may be multiple elements that match the given hash.

		// template<typename HashCodeT>
		// ENABLE_IF_HASHCODE_SIZET(HashCodeT, iterator) find_by_hash(HashCodeT c)
		// {
		// 	static_assert(bCacheHashCode,
		// 		"find_by_hash(hash_code_t c) is designed to avoid recomputing hashes, "
		// 		"so it requires cached hash codes.  Consider setting template parameter "
		// 		"bCacheHashCode to true or using find_by_hash(const key_type& k, hash_code_t c) instead.");

		// 	const size_type n = (size_type)bucket_index(c, (uint32_t)mnBucketCount);

		// 	soft_ptr<node_type> pNode = DoFindNode(mpBucketArray->at_unsafe(n), c);

		// 	return pNode ? iterator(pNode, GetBucketArrayIt() + n) : end();
		// }

		// template<typename HashCodeT>
		// ENABLE_IF_HASHCODE_SIZET(HashCodeT, const_iterator) find_by_hash(HashCodeT c) const
		// {
		// 	static_assert(bCacheHashCode,
		// 						"find_by_hash(hash_code_t c) is designed to avoid recomputing hashes, "
		// 						"so it requires cached hash codes.  Consider setting template parameter "
		// 						"bCacheHashCode to true or using find_by_hash(const key_type& k, hash_code_t c) instead.");

		// 	const size_type n = (size_type)bucket_index(c, (uint32_t)mnBucketCount);

		// 	soft_ptr<node_type> pNode = DoFindNode(mpBucketArray->at_unsafe(n), c);

		// 	return pNode ?
		// 			   const_iterator(pNode, GetBucketArrayIt() + n) :
		// 			   cend();
		// }

		// iterator find_by_hash(const key_type& k, hash_code_t c)
		// {
		// 	const size_type n = (size_type)bucket_index(c, (uint32_t)mnBucketCount);

		// 	soft_ptr<node_type> pNode = DoFindNode(mpBucketArray->at_unsafe(n), k, c);
		// 	return pNode ? iterator(pNode, GetBucketArrayIt() + n) : end();
		// }

		// const_iterator find_by_hash(const key_type& k, hash_code_t c) const
		// {
		// 	const size_type n = (size_type)bucket_index(c, (uint32_t)mnBucketCount);

		// 	soft_ptr<node_type> pNode = DoFindNode(mpBucketArray->at_unsafe(n), k, c);
		// 	return pNode ? const_iterator(pNode, GetBucketArrayIt() + n) : cend();
		// }

		// Returns a pair that allows iterating over all nodes in a hash bucket
		//   first in the pair returned holds the iterator for the beginning of the bucket,
		//   second in the pair returned holds the iterator for the end of the bucket,
		// If no bucket is found, both values in the pair are set to end().
		//
		// See also the note above.
		// std::pair<iterator, iterator> find_range_by_hash(hash_code_t c);
		// std::pair<const_iterator, const_iterator> find_range_by_hash(hash_code_t c) const;

		size_type count(const key_type& k) const noexcept;

		std::pair<iterator, iterator>             equal_range(const key_type& k);
		std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const;

		bool validate() const;
		iterator_validity  validate_iterator(const_iterator i) const;

	protected:
		safe_array_iterator2<owning_node_type, Safety> GetBucketArrayIt(size_t n) const { 
			return safe_array_iterator2<owning_node_type, Safety>::make(mpBucketArray, n);
		}

		// We must remove one of the 'DoGetResultIterator' overloads from the overload-set (via SFINAE) because both can
		// not compile successfully at the same time. The 'bUniqueKeys' template parameter chooses at compile-time the
		// type of 'insert_return_type' between a pair<iterator,bool> and a raw iterator. We must pick between the two
		// overloads that unpacks the iterator from the pair or simply passes the provided iterator to the caller based
		// on the class template parameter.
		template <typename BoolConstantT>
		iterator DoGetResultIterator(BoolConstantT,
		                             const insert_return_type& irt,
		                             SM_ENABLE_IF_TRUETYPE(BoolConstantT) = nullptr) const noexcept
		{
			return irt.first;
		}

		template <typename BoolConstantT>
		iterator DoGetResultIterator(BoolConstantT,
		                             const insert_return_type& irt,
		                             SM_DISABLE_IF_TRUETYPE(BoolConstantT) = nullptr) const noexcept
		{
			return irt;
		}

		owning_node_type  DoAllocateNodeFromKey(const key_type& key);
		owning_node_type  DoAllocateNodeFromKey(key_type&& key);
		void        DoFreeNode(owning_node_type pNode);
		void        DoFreeNodes(soft_bucket_type pBucketArray, size_type);

		owning_bucket_type DoAllocateBuckets(size_type n);
		void        DoFreeBuckets(owning_bucket_type pBucketArray, size_type n);

		template <typename BoolConstantT, class... Args, SM_ENABLE_IF_TRUETYPE(BoolConstantT) = nullptr>
		std::pair<iterator, bool> DoInsertValue(BoolConstantT, Args&&... args);

		template <typename BoolConstantT, class... Args, SM_DISABLE_IF_TRUETYPE(BoolConstantT) = nullptr>
		iterator DoInsertValue(BoolConstantT, Args&&... args);


		template <typename BoolConstantT>
		std::pair<iterator, bool> DoInsertValueExtra(BoolConstantT,
													   const key_type& k,
													   hash_code_t c,
													   owning_node_type pNodeNew,
													   value_type&& value,
													   SM_ENABLE_IF_TRUETYPE(BoolConstantT) = nullptr);

		template <typename BoolConstantT>
		std::pair<iterator, bool> DoInsertValue(BoolConstantT,
												  value_type&& value,
												  SM_ENABLE_IF_TRUETYPE(BoolConstantT) = nullptr);

		template <typename BoolConstantT>
		iterator DoInsertValueExtra(BoolConstantT,
									const key_type& k,
									hash_code_t c,
									owning_node_type pNodeNew,
									value_type&& value,
									SM_DISABLE_IF_TRUETYPE(BoolConstantT) = nullptr);

		template <typename BoolConstantT>
		iterator DoInsertValue(BoolConstantT, value_type&& value, SM_DISABLE_IF_TRUETYPE(BoolConstantT) = nullptr);


		template <typename BoolConstantT>
		std::pair<iterator, bool> DoInsertValueExtra(BoolConstantT,
													   const key_type& k,
													   hash_code_t c,
													   owning_node_type pNodeNew,
													   const value_type& value,
													   SM_ENABLE_IF_TRUETYPE(BoolConstantT) = nullptr);

		template <typename BoolConstantT>
		std::pair<iterator, bool> DoInsertValue(BoolConstantT,
		                                          const value_type& value,
		                                          SM_ENABLE_IF_TRUETYPE(BoolConstantT) = nullptr);

		template <typename BoolConstantT>
		iterator DoInsertValueExtra(BoolConstantT,
		                            const key_type& k,
		                            hash_code_t c,
		                            owning_node_type pNodeNew,
		                            const value_type& value,
		                            SM_DISABLE_IF_TRUETYPE(BoolConstantT) = nullptr);

		template <typename BoolConstantT>
		iterator DoInsertValue(BoolConstantT, const value_type& value, SM_DISABLE_IF_TRUETYPE(BoolConstantT) = nullptr);

		template <class... Args>
		owning_node_type DoAllocateNode(Args&&... args);
		owning_node_type DoAllocateNode(value_type&& value);
		owning_node_type DoAllocateNode(const value_type& value);

		// DoInsertKey is supposed to get hash_code_t c  = get_hash_code(key).
		// it is done in case application has it's own hashset/hashmap-like containter, where hash code is for some reason known prior the insert
		// this allows to save some performance, especially with heavy hash functions
		std::pair<iterator, bool> DoInsertKey(std::true_type, const key_type& key, hash_code_t c);
		iterator                    DoInsertKey(std::false_type, const key_type& key, hash_code_t c);
		std::pair<iterator, bool> DoInsertKey(std::true_type, key_type&& key, hash_code_t c);
		iterator                    DoInsertKey(std::false_type, key_type&& key, hash_code_t c);

		// We keep DoInsertKey overload without third parameter, for compatibility with older revisions of EASTL (3.12.07 and earlier)
		// It used to call get_hash_code as a first call inside the DoInsertKey.
		std::pair<iterator, bool> DoInsertKey(std::true_type, const key_type& key)  { return DoInsertKey(std::true_type(),  key, get_hash_code(key)); }
		iterator                    DoInsertKey(std::false_type, const key_type& key) { return DoInsertKey(std::false_type(), key, get_hash_code(key)); }
		std::pair<iterator, bool> DoInsertKey(std::true_type, key_type&& key)       { return DoInsertKey(std::true_type(),  std::move(key), get_hash_code(key)); }
		iterator                    DoInsertKey(std::false_type, key_type&& key)      { return DoInsertKey(std::false_type(), std::move(key), get_hash_code(key)); }

		void DoInit();
		void       DoRehash(size_type nBucketCount);
		// soft_node_type DoFindNode(soft_node_type pNode, const key_type& k, hash_code_t c) const;
		soft_node_type DoFindNode(bucket_index_t bucket, const key_type& k, hash_code_t c) const;


		[[noreturn]] static
		void ThrowRangeException(const char* msg) { throw std::out_of_range(msg); }

		// template <typename T>
		// ENABLE_IF_HAS_HASHCODE(T, node_type) DoFindNode(T* pNode, hash_code_t c) const
		// {
		// 	for (; pNode; pNode = pNode->mpNext)
		// 	{
		// 		if (pNode->mnHashCode == c)
		// 			return pNode;
		// 	}
		// 	return NULL;
		// }

		// template <typename U, typename BinaryPredicate>
		// node_type* DoFindNodeT(node_type* pNode, const U& u, BinaryPredicate predicate) const;

	}; // class hashtable


	///////////////////////////////////////////////////////////////////////
	// hashtable
	///////////////////////////////////////////////////////////////////////

	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>
	::hashtable(size_type nBucketCount, const H1& h1, const H2& h2, const H& h,
				const Eq& eq, const EK& ek/*, const allocator_type& allocator*/)
		:   rehash_base<RP, hashtable>(),
			hash_code_base<K, V, EK, Eq, H1, H2, H, S, bC>(ek, eq, h1, h2, h),
			mnBucketCount(),
			mnElementCount(0),
			mRehashPolicy()/*,
			mAllocator(allocator)*/
	{
#ifdef SAFE_MEMORY_CHECKER_EXTENSIONS
// checker needs this to actually instantiate the operator method
		auto P1 = &Eq::operator();
		auto P2 = &H1::operator();
#endif
		// if(nBucketCount < 2)  // If we are starting in an initially empty state, with no memory allocation done.
		// 	reset_lose_memory();
		// else // Else we are creating a potentially non-empty hashtable...
		// {
			// EASTL_ASSERT(nBucketCount < 10000000);
			mnBucketCount = (size_type)mRehashPolicy.GetNextBucketCount((uint32_t)nBucketCount);
			mpBucketArray = DoAllocateBuckets(mnBucketCount); // mnBucketCount will always be at least 2.
		// }
	}



	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// template <typename FowardIterator>
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::hashtable(FowardIterator first, FowardIterator last, size_type nBucketCount, 
	// 																 const H1& h1, const H2& h2, const H& h, 
	// 																 const Eq& eq, const EK& ek/*, const allocator_type& allocator*/)
	// 	:   rehash_base<rehash_policy_type, hashtable>(),
	// 		hash_code_base<key_type, value_type, extract_key_type, key_equal, h1_type, h2_type, h_type, kCacheHashCode>(ek, eq, h1, h2, h),
	// 	  //mnBucketCount(0), // This gets re-assigned below.
	// 		mnElementCount(0),
	// 		mRehashPolicy()/*,
	// 		mAllocator(allocator)*/
	// {
	// 	if(nBucketCount < 2)
	// 	{
	// 		const size_type nElementCount = (size_type)ht_distance(first, last);
	// 		mnBucketCount = (size_type)mRehashPolicy.GetBucketCount((uint32_t)nElementCount);
	// 	}
	// 	else
	// 	{
	// 		EASTL_ASSERT(nBucketCount < 10000000);
	// 		mnBucketCount = nBucketCount;
	// 	}

	// 	mpBucketArray = DoAllocateBuckets(mnBucketCount); // mnBucketCount will always be at least 2.

	// 	#if EASTL_EXCEPTIONS_ENABLED
	// 		try
	// 		{
	// 	#endif
	// 			for(; first != last; ++first)
	// 				insert(*first);
	// 	#if EASTL_EXCEPTIONS_ENABLED
	// 		}
	// 		catch(...)
	// 		{
	// 			clear();
	// 			DoFreeBuckets(std::move(mpBucketArray), mnBucketCount);
	// 			throw;
	// 		}
	// 	#endif
	// }



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::hashtable(const this_type& x)
		:   rehash_base<RP, hashtable>(x),
			hash_code_base<K, V, EK, Eq, H1, H2, H, S, bC>(x),
			mnBucketCount(x.mnBucketCount),
			mnElementCount(x.mnElementCount),
			mRehashPolicy(x.mRehashPolicy)/*,
			mAllocator(x.mAllocator)*/
	{
		if(mnElementCount) // If there is anything to copy...
		{
			mpBucketArray = DoAllocateBuckets(mnBucketCount); // mnBucketCount will be at least 2.

			// #if EASTL_EXCEPTIONS_ENABLED
				try
				{
			// #endif
					for(size_type i = 0; i < x.mnBucketCount; ++i)
					{
						soft_node_type  pNodeSource = x.mpBucketArray->at_unsafe(i);
						
						if(pNodeSource) {
							mpBucketArray->at_unsafe(i) = DoAllocateNode(pNodeSource->mValue);
							
							copy_code(*mpBucketArray->at_unsafe(i), *pNodeSource);
							pNodeSource = pNodeSource->mpNext;
						}
						soft_node_type ppNodeDest  = mpBucketArray->at_unsafe(i);

						while(pNodeSource)
						{
							ppNodeDest->mpNext = DoAllocateNode(pNodeSource->mValue);
							copy_code(*ppNodeDest->mpNext, *pNodeSource);
							ppNodeDest = ppNodeDest->mpNext;
							pNodeSource = pNodeSource->mpNext;
						}
					}
			// #if EASTL_EXCEPTIONS_ENABLED
				}
				catch(...)
				{
					clear();
					DoFreeBuckets(std::move(mpBucketArray), mnBucketCount);
					throw;
				}
			// #endif
		}
		else
		{
			// In this case, instead of allocate memory and copy nothing from x, 
			// we reset ourselves to a zero allocation state.
			DoInit();
		}
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::hashtable(this_type&& x)
		:   rehash_base<RP, hashtable>(x),
			hash_code_base<K, V, EK, Eq, H1, H2, H, S, bC>(x),
			mnBucketCount(0),
			mnElementCount(0),
			mRehashPolicy(x.mRehashPolicy)/*,
			mAllocator(x.mAllocator)*/
	{
//		reset_lose_memory(); // We do this here the same as we do it in the default ctor because it puts the container in a proper initial empty state. This code would be cleaner if we could rely on being able to use C++11 delegating constructors and just call the default ctor here.
		swap(x);
	}


	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::hashtable(this_type&& x, const allocator_type& allocator)
	// 	:   rehash_base<RP, hashtable>(x),
	// 		hash_code_base<K, V, EK, Eq, H1, H2, H, bC>(x),
	// 		mnBucketCount(0),
	// 		mnElementCount(0),
	// 		mRehashPolicy(x.mRehashPolicy),
	// 		mAllocator(allocator)
	// {
	// 	reset_lose_memory(); // We do this here the same as we do it in the default ctor because it puts the container in a proper initial empty state. This code would be cleaner if we could rely on being able to use C++11 delegating constructors and just call the default ctor here.
	// 	swap(x); // swap will directly or indirectly handle the possibility that mAllocator != x.mAllocator.
	// }


	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// inline const typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::allocator_type&
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::get_allocator() const noexcept
	// {
	// 	return mAllocator;
	// }



	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::allocator_type&
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::get_allocator() noexcept
	// {
	// 	return mAllocator;
	// }



	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// inline void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::set_allocator(const allocator_type& allocator)
	// {
	// 	mAllocator = allocator;
	// }



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::this_type&
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::operator=(const this_type& x)
	{
		if(this != &x)
		{
			clear();

			// #if EASTL_ALLOCATOR_COPY_ENABLED
			// 	mAllocator = x.mAllocator;
			// #endif

			insert_unsafe(x.begin(), x.end());
		}
		return *this;
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::this_type&
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::operator=(this_type&& x)
	{
		if(this != &x)
		{
//			clear();        // To consider: Are we really required to clear here? x is going away soon and will clear itself in its dtor.
			swap(x);        // member swap handles the case that x has a different allocator than our allocator by doing a copy.
		}
		return *this;
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::this_type&
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::operator=(std::initializer_list<value_type> ilist)
	{
		// The simplest means of doing this is to clear and insert. There probably isn't a generic
		// solution that's any more efficient without having prior knowledge of the ilist contents.
		clear();
		insert_unsafe(ilist.begin(), ilist.end());
		return *this;
	}



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::~hashtable()
	{
		clear();
		DoFreeBuckets(std::move(mpBucketArray), mnBucketCount);
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::owning_node_type
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoAllocateNodeFromKey(const key_type& key)
	{
// //		node_type* const pNode = (node_type*)allocate_memory(mAllocator, sizeof(node_type), EASTL_ALIGN_OF(value_type), 0);
// 		node_type* const pNode = (node_type*)safememory::lib_helpers::allocate_memory(sizeof(node_type), alignof(value_type), 0);
// 		EASTL_ASSERT_MSG(pNode != nullptr, "the behaviour of eastl::allocators that return nullptr is not defined.");

// 		#if EASTL_EXCEPTIONS_ENABLED
// 			try
// 			{
// 		#endif
// 				::new(std::addressof(pNode->mValue)) value_type(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
// 				pNode->mpNext = NULL;
// 				return pNode;
// 		#if EASTL_EXCEPTIONS_ENABLED
// 			}
// 			catch(...)
// 			{
// //				EASTLFree(mAllocator, pNode, sizeof(node_type));
// 				safememory::lib_helpers::EASTLFree(pNode, sizeof(node_type));
// 				throw;
// 			}
// 		#endif

		return make_owning_2<node_type, S>(std::piecewise_construct, std::forward_as_tuple(key), std::tuple<>());
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
				typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::owning_node_type
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoAllocateNodeFromKey(key_type&& key)
	{
// 		// node_type* const pNode = (node_type*)allocate_memory(mAllocator, sizeof(node_type), EASTL_ALIGN_OF(value_type), 0);
// 		node_type* const pNode = (node_type*)safememory::lib_helpers::allocate_memory(sizeof(node_type), alignof(value_type), 0);
// 		EASTL_ASSERT_MSG(pNode != nullptr, "the behaviour of std::allocators that return nullptr is not defined.");

// 		#if EASTL_EXCEPTIONS_ENABLED
// 			try
// 			{
// 		#endif
// 				::new(std::addressof(pNode->mValue)) value_type(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
// 				pNode->mpNext = NULL;
// 				return pNode;
// 		#if EASTL_EXCEPTIONS_ENABLED
// 			}
// 			catch(...)
// 			{
// //				EASTLFree(mAllocator, pNode, sizeof(node_type));
// 				safememory::lib_helpers::EASTLFree(pNode, sizeof(node_type));
// 				throw;
// 			}
// 		#endif

		return make_owning_2<node_type, S>(std::piecewise_construct, std::forward_as_tuple(std::move(key)), std::tuple<>());
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoFreeNode(owning_node_type pNode)
	{
		// pNode->~node_type();
		// // EASTLFree(mAllocator, pNode, sizeof(node_type));
		// safememory::lib_helpers::EASTLFree(pNode, sizeof(node_type));
	}



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoFreeNodes(soft_bucket_type pNodeArray, size_type n)
	{
		if(pNodeArray) {
			for(size_type i = 0; i < n; ++i)
			{
				// owning_node_type pNode = std::move(pNodeArray->at_unsafe(i));
				// while(pNode)
				// {
				// 	owning_node_type pTempNode = std::move(pNode->mpNext);
				// 	DoFreeNode(std::move(pNode));
				// 	pNode = std::move(pTempNode);
				// }
				pNodeArray->at_unsafe(i) = nullptr;
			}
		}
	}



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::owning_bucket_type
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoAllocateBuckets(size_type n)
	{
		owning_bucket_type pBucketArray = make_owning_array_of<owning_node_type, S>(n);
		std::uninitialized_value_construct(pBucketArray->begin(), pBucketArray->begin() + n);

		return pBucketArray;
	}



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoFreeBuckets(owning_bucket_type pBucketArray, size_type n)
	{
		// If n <= 1, then pBucketArray is from the shared gpEmptyBucketArray. We don't test 
		// for pBucketArray == &gpEmptyBucketArray because one library have a different gpEmptyBucketArray
		// than another but pass a hashtable to another. So we go by the size.
		// if(n > 1)
		// 	// EASTLFree(mAllocator, pBucketArray, (n + 1) * sizeof(node_type*)); // '+1' because DoAllocateBuckets allocates nBucketCount + 1 buckets in order to have a NULL sentinel at the end.
	
		// pBucketArray will self destroy here
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::swap(this_type& x)
	{
		hash_code_base<K, V, EK, Eq, H1, H2, H, S, bC>::base_swap(x); // hash_code_base has multiple implementations, so we let them handle the swap.
		std::swap(mRehashPolicy, x.mRehashPolicy);
		std::swap(mpBucketArray, x.mpBucketArray);
		std::swap(mnBucketCount, x.mnBucketCount);
		std::swap(mnElementCount, x.mnElementCount);

		// if (mAllocator != x.mAllocator) // If allocators are not equivalent...
		// {
		// 	std::swap(mAllocator, x.mAllocator);
		// }
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::rehash_policy(const rehash_policy_type& rehashPolicy)
	{
		mRehashPolicy = rehashPolicy;

		const size_type nBuckets = rehashPolicy.GetBucketCount((uint32_t)mnElementCount);

		if(nBuckets > mnBucketCount)
			DoRehash(nBuckets);
	}



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find(const key_type& k)
	{
		const hash_code_t c = get_hash_code(k);
		const bucket_index_t n = bucket_index(k, c, (uint32_t)mnBucketCount);

		soft_node_type pNode = DoFindNode(n, k, c);
		return pNode ? iterator(pNode, GetBucketArrayIt(n)) : end();
	}



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::const_iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find(const key_type& k) const
	{
		const hash_code_t c = get_hash_code(k);
		const bucket_index_t n = bucket_index(k, c, (uint32_t)mnBucketCount);

		soft_node_type pNode = DoFindNode(n, k, c);
		return pNode ? const_iterator(pNode, GetBucketArrayIt(n)) : cend();
	}



	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// template <typename U, typename UHash, typename BinaryPredicate>
	// inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find_as(const U& other, UHash uhash, BinaryPredicate predicate)
	// {
	// 	const hash_code_t c = (hash_code_t)uhash(other);
	// 	const size_type   n = (size_type)(c % mnBucketCount); // This assumes we are using the mod range policy.

	// 	node_type* const pNode = DoFindNodeT(mpBucketArray->at_unsafe(n), other, predicate);
	// 	return pNode ? iterator(pNode, mpBucketArray + n) : iterator(mpBucketArray + mnBucketCount); // iterator(mpBucketArray + mnBucketCount) == end()
	// }



	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// template <typename U, typename UHash, typename BinaryPredicate>
	// inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::const_iterator
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find_as(const U& other, UHash uhash, BinaryPredicate predicate) const
	// {
	// 	const hash_code_t c = (hash_code_t)uhash(other);
	// 	const size_type   n = (size_type)(c % mnBucketCount); // This assumes we are using the mod range policy.

	// 	node_type* const pNode = DoFindNodeT(mpBucketArray->at_unsafe(n), other, predicate);
	// 	return pNode ? const_iterator(pNode, mpBucketArray + n) : const_iterator(mpBucketArray + mnBucketCount); // iterator(mpBucketArray + mnBucketCount) == end()
	// }


	/// hashtable_find
	///
	/// Helper function that defaults to using hash<U> and equal_to_2<T, U>.
	/// This makes it so that by default you don't need to provide these.
	/// Note that the default hash functions may not be what you want, though.
	///
	/// Example usage. Instead of this:
	///     hash_set<string> hashSet;
	///     hashSet.find("hello", hash<char*>(), equal_to_2<string, char*>());
	///
	/// You can use this:
	///     hash_set<string> hashSet;
	///     hashtable_find(hashSet, "hello");
	///
	// template <typename H, typename U>
	// inline typename H::iterator hashtable_find(H& hashTable, U u)
	// 	{ return hashTable.find_as(u, hash<U>(), eastl::equal_to_2<const typename H::key_type, U>()); }

	// template <typename H, typename U>
	// inline typename H::const_iterator hashtable_find(const H& hashTable, U u)
	// 	{ return hashTable.find_as(u, hash<U>(), eastl::equal_to_2<const typename H::key_type, U>()); }



	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// template <typename U>
	// inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find_as(const U& other)
	// 	{ return safememory::hashtable_find(*this, other); }
	// 	// VC++ doesn't appear to like the following, though it seems correct to me.
	// 	// So we implement the workaround above until we can straighten this out.
	// 	//{ return find_as(other, eastl::hash<U>(), eastl::equal_to_2<const key_type, U>()); }


	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// template <typename U>
	// inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::const_iterator
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find_as(const U& other) const
	// 	{ return hashtable_find(*this, other); }
		// VC++ doesn't appear to like the following, though it seems correct to me.
		// So we implement the workaround above until we can straighten this out.
		//{ return find_as(other, eastl::hash<U>(), eastl::equal_to_2<const key_type, U>()); }



	// template <typename K, typename V, memory_safety S, typename EK, typename Eq, 
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// std::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::const_iterator,
	// 			typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::const_iterator>
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find_range_by_hash(hash_code_t c) const
	// {
	// 	const size_type start = (size_type)bucket_index(c, (uint32_t)mnBucketCount);
	// 	soft_ptr<node_type> pNodeStart = mpBucketArray->at_unsafe(start);

	// 	if (pNodeStart)
	// 	{
	// 		std::pair<const_iterator, const_iterator> pair(const_iterator(pNodeStart, GetBucketArrayIt() + start), 
	// 														 const_iterator(pNodeStart, GetBucketArrayIt() + start));
	// 		pair.second.increment_bucket();
	// 		return pair;
	// 	}

	// 	return std::pair<const_iterator, const_iterator>(cend(), cend());
	// }



	// template <typename K, typename V, memory_safety S, typename EK, typename Eq, 
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// std::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator,
	// 			typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator>
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find_range_by_hash(hash_code_t c)
	// {
	// 	const size_type start = (size_type)bucket_index(c, (uint32_t)mnBucketCount);
	// 	soft_ptr<node_type> pNodeStart = mpBucketArray->at_unsafe(start);

	// 	if (pNodeStart)
	// 	{
	// 		std::pair<iterator, iterator> pair(iterator(pNodeStart, GetBucketArrayIt() + start), 
	// 											 iterator(pNodeStart, GetBucketArrayIt() + start));
	// 		pair.second.increment_bucket();
	// 		return pair;

	// 	}

	// 	return std::pair<iterator, iterator>(end(), end());
	// }



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::size_type
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::count(const key_type& k) const noexcept
	{
		const hash_code_t c      = get_hash_code(k);
		const size_type   n      = (size_type)bucket_index(k, c, (uint32_t)mnBucketCount);
		size_type         result = 0;

		// To do: Make a specialization for bU (unique keys) == true and take 
		// advantage of the fact that the count will always be zero or one in that case. 
		for(soft_node_type pNode = mpBucketArray->at_unsafe(n); pNode; pNode = pNode->mpNext)
		{
			if(compare(k, c, *pNode))
				++result;
		}
		return result;
	}



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	std::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator,
				typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator>
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::equal_range(const key_type& k)
	{
		const hash_code_t c     = get_hash_code(k);
		const bucket_index_t n  = bucket_index(k, c, (uint32_t)mnBucketCount);
		soft_node_type        pNode = DoFindNode(n, k, c);
		auto       head  = GetBucketArrayIt(n);

		if(pNode)
		{
			soft_node_type p1 = pNode->mpNext;

			for(; p1; p1 = p1->mpNext)
			{
				if(!compare(k, c, *p1))
					break;
			}

			iterator first(pNode, head);
			iterator last(p1, head);

			last.increment_bucket_if_null();

			return std::pair<iterator, iterator>(first, last);
		}

		return std::pair<iterator, iterator>(end(), end());
	}




	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	std::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::const_iterator,
				typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::const_iterator>
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::equal_range(const key_type& k) const
	{
		const hash_code_t c     = get_hash_code(k);
		const bucket_index_t n  = bucket_index(k, c, (uint32_t)mnBucketCount);
		soft_node_type        pNode = DoFindNode(n, k, c);
		auto       head  = GetBucketArrayIt(n);

		if(pNode)
		{
			soft_node_type p1 = pNode->mpNext;

			for(; p1; p1 = p1->mpNext)
			{
				if(!compare(k, c, *p1))
					break;
			}

			const_iterator first(pNode, head);
			const_iterator last(p1, head);

			last.increment_bucket_if_null();

			return std::pair<const_iterator, const_iterator>(first, last);
		}

		return std::pair<const_iterator, const_iterator>(cend(), cend());
	}



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::soft_node_type 
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoFindNode(typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::bucket_index_t n, const key_type& k, hash_code_t c) const
	{
		soft_node_type pNode = mpBucketArray->at_unsafe(n);
		for(; pNode; pNode = pNode->mpNext)
		{
			if(compare(k, c, *pNode))
				return pNode;
		}
		return soft_node_type();
	}



	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// template <typename U, typename BinaryPredicate>
	// inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::soft_node_type 
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoFindNodeT(soft_ptr<node_type> pNode, const U& other, BinaryPredicate predicate) const
	// {
	// 	for(; pNode; pNode = pNode->mpNext)
	// 	{
	// 		if(predicate(mExtractKey(pNode->mValue), other)) // Intentionally compare with key as first arg and other as second arg.
	// 			return pNode;
	// 	}
	// 	return soft_ptr<node_type>();
	// }



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <typename BoolConstantT, class... Args, SM_ENABLE_IF_TRUETYPE(BoolConstantT)>
	std::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator, bool>
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertValue(BoolConstantT, Args&&... args) // true_type means bUniqueKeys is true.
	{
		// Adds the value to the hash table if not already present. 
		// If already present then the existing value is returned via an iterator/bool pair.

		// We have a chicken-and-egg problem here. In order to know if and where to insert the value, we need to get the 
		// hashtable key for the value. But we don't explicitly have a value argument, we have a templated Args&&... argument.
		// We need the value_type in order to proceed, but that entails getting an instance of a value_type from the args.
		// And it may turn out that the value is already present in the hashtable and we need to cancel the insertion, 
		// despite having obtained a value_type to put into the hashtable. We have mitigated this problem somewhat by providing
		// specializations of the insert function for const value_type& and value_type&&, and so the only time this function
		// should get called is when args refers to arguments to construct a value_type.

		auto  pNodeNew = DoAllocateNode(std::forward<Args>(args)...);
		const key_type&   k        = mExtractKey(pNodeNew->mValue);
		const hash_code_t c        = get_hash_code(k);
		bucket_index_t    n        = bucket_index(k, c, (uint32_t)mnBucketCount);
		soft_node_type pNode    = DoFindNode(n, k, c);

		if(pNode == nullptr) // If value is not present... add it.
		{
			const std::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, (uint32_t)1);

			set_code(*pNodeNew, c); // This is a no-op for most hashtables.

			// #if EASTL_EXCEPTIONS_ENABLED
			// 	try
			// 	{
			// #endif
					if(bRehash.first)
					{
						n = (size_type)bucket_index(k, c, (uint32_t)bRehash.second);
						DoRehash(bRehash.second);
					}

					// EASTL_ASSERT((uintptr_t)mpBucketArray != (uintptr_t)&gpEmptyBucketArray[0]);
					soft_node_type pNodeIt = pNodeNew;
					pNodeNew->mpNext = std::move(mpBucketArray->at_unsafe(n));
					mpBucketArray->at_unsafe(n) = std::move(pNodeNew);
					++mnElementCount;

					return std::pair<iterator, bool>(iterator(pNodeIt, GetBucketArrayIt(n)), true);
			// #if EASTL_EXCEPTIONS_ENABLED
			// 	}
			// 	catch(...)
			// 	{
			// 		// DoFreeNode(pNodeNew);
			// 		throw;
			// 	}
			// #endif
		}
		// else
		// {
		// 	// To do: We have an inefficiency to deal with here. We allocated a node above but we are freeing it here because
		// 	// it turned out it wasn't needed. But we needed to create the node in order to get the hashtable key for
		// 	// the node. One possible resolution is to create specializations: DoInsertValue(true_type, value_type&&) and 
		// 	// DoInsertValue(true_type, const value_type&) which don't need to create a node up front in order to get the 
		// 	// hashtable key. Probably most users would end up using these pathways instead of this Args... pathway.
		// 	// While we should considering handling this to-do item, a lot of the performance limitations of maps and sets 
		// 	// in practice is with finding elements rather than adding (potentially redundant) new elements.
		// 	// DoFreeNode(pNodeNew);
		// }

		return std::pair<iterator, bool>(iterator(pNode, GetBucketArrayIt(n)), false);
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <typename BoolConstantT, class... Args, SM_DISABLE_IF_TRUETYPE(BoolConstantT)>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertValue(BoolConstantT, Args&&... args) // false_type means bUniqueKeys is false.
	{
		const std::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, (uint32_t)1);

		if(bRehash.first)
			DoRehash(bRehash.second);

		auto        pNodeNew = DoAllocateNode(std::forward<Args>(args)...);
		const key_type&   k        = mExtractKey(pNodeNew->mValue);
		const hash_code_t c        = get_hash_code(k);
		const bucket_index_t n     = bucket_index(k, c, (uint32_t)mnBucketCount);

		set_code(*pNodeNew, c); // This is a no-op for most hashtables.

		// To consider: Possibly make this insertion not make equal elements contiguous.
		// As it stands now, we insert equal values contiguously in the hashtable.
		// The benefit is that equal_range can work in a sensible manner and that
		// erase(value) can more quickly find equal values. The downside is that
		// this insertion operation taking some extra time. How important is it to
		// us that equal_range span all equal items? 
		soft_node_type pNodePrev = DoFindNode(n, k, c);
		soft_node_type pNodeIt = pNodeNew;

		if(pNodePrev == nullptr)
		{
			// EASTL_ASSERT((void**)mpBucketArray != &gpEmptyBucketArray[0]);
			pNodeNew->mpNext = std::move(mpBucketArray->at_unsafe(n));
			mpBucketArray->at_unsafe(n) = std::move(pNodeNew);
		}
		else
		{
			pNodeNew->mpNext  = std::move(pNodePrev->mpNext);
			pNodePrev->mpNext = std::move(pNodeNew);
		}

		++mnElementCount;

		return iterator(pNodeIt, GetBucketArrayIt(n));
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <class... Args>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::owning_node_type
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoAllocateNode(Args&&... args)
	{
		// node_type* const pNode = (node_type*)allocate_memory(mAllocator, sizeof(node_type), EASTL_ALIGN_OF(value_type), 0);
		// node_type* const pNode = (node_type*)safememory::lib_helpers::allocate_memory(sizeof(node_type), alignof(value_type), 0);
		// EASTL_ASSERT_MSG(pNode != nullptr, "the behaviour of eastl::allocators that return nullptr is not defined.");

		// #if EASTL_EXCEPTIONS_ENABLED
		// 	try
		// 	{
		// #endif
		// 		::new(std::addressof(pNode->mValue)) value_type(std::forward<Args>(args)...);
		// 		pNode->mpNext = NULL;
		// 		return pNode;
		// #if EASTL_EXCEPTIONS_ENABLED
		// 	}
		// 	catch(...)
		// 	{
		// 		// EASTLFree(mAllocator, pNode, sizeof(node_type));
		// 		safememory::lib_helpers::EASTLFree(pNode, sizeof(node_type));
		// 		throw;
		// 	}
		// #endif
		return make_owning_2<node_type, S>(std::forward<Args>(args)...);
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Note: The following insertion-related functions are nearly copies of the above three functions,
	// but are for value_type&& and const value_type& arguments. It's useful for us to have the functions
	// below, even when using a fully compliant C++11 compiler that supports the above functions. 
	// The reason is because the specializations below are slightly more efficient because they can delay
	// the creation of a node until it's known that it will be needed.
	////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <typename BoolConstantT>
	std::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator, bool>
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertValueExtra(BoolConstantT, const key_type& k,
		hash_code_t c, owning_node_type pNodeNew, value_type&& value, SM_ENABLE_IF_TRUETYPE(BoolConstantT)) // true_type means bUniqueKeys is true.
	{
		// Adds the value to the hash table if not already present. 
		// If already present then the existing value is returned via an iterator/bool pair.
		bucket_index_t  n     = bucket_index(k, c, (uint32_t)mnBucketCount);
		soft_node_type  pNode = DoFindNode(n, k, c);

		if(pNode == nullptr) // If value is not present... add it.
		{
			const std::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, (uint32_t)1);

			// Allocate the new node before doing the rehash so that we don't 
			// do a rehash if the allocation throws.
			// #if EASTL_EXCEPTIONS_ENABLED
			// 	bool nodeAllocated;  // If exceptions are enabled then we we need to track if we allocated the node so we can free it in the catch block.
			// #endif

			// if(pNodeNew)
			// {
			// 	::new(std::addressof(pNodeNew->mValue)) value_type(std::move(value)); // It's expected that pNodeNew was allocated with allocate_uninitialized_node.
			// 	#if EASTL_EXCEPTIONS_ENABLED
			// 		nodeAllocated = false;
			// 	#endif
			// }
			// else
			// {
				pNodeNew = DoAllocateNode(std::move(value));
				// #if EASTL_EXCEPTIONS_ENABLED
				// 	nodeAllocated = true;
				// #endif
			// }

			set_code(*pNodeNew, c); // This is a no-op for most hashtables.

			// #if EASTL_EXCEPTIONS_ENABLED
				// try
				// {
			// #endif
					if(bRehash.first)
					{
						n = (size_type)bucket_index(k, c, (uint32_t)bRehash.second);
						DoRehash(bRehash.second);
					}

					// EASTL_ASSERT((uintptr_t)mpBucketArray != (uintptr_t)&gpEmptyBucketArray[0]);
					soft_node_type pNodeIt = pNodeNew;
					pNodeNew->mpNext = std::move(mpBucketArray->at_unsafe(n));
					mpBucketArray->at_unsafe(n) = std::move(pNodeNew);
					++mnElementCount;

					return std::pair<iterator, bool>(iterator(pNodeIt, GetBucketArrayIt(n)), true);
			// #if EASTL_EXCEPTIONS_ENABLED
				// }
				// catch(...)
				// {
				// 	// if(nodeAllocated) // If we allocated the node within this function, free it. Else let the caller retain ownership of it.
				// 	// 	DoFreeNode(pNodeNew);
				// 	throw;
				// }
			// #endif
		}
		// Else the value is already present, so don't add a new node. And don't free pNodeNew.

		return std::pair<iterator, bool>(iterator(pNode, GetBucketArrayIt(n)), false);
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <typename BoolConstantT>
	std::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator, bool>
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertValue(BoolConstantT, value_type&& value, SM_ENABLE_IF_TRUETYPE(BoolConstantT)) // true_type means bUniqueKeys is true.
	{
		const key_type&   k = mExtractKey(value);
		const hash_code_t c = get_hash_code(k);

		return DoInsertValueExtra(std::true_type(), k, c, {nullptr}, std::move(value));
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <typename BoolConstantT>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertValueExtra(BoolConstantT, const key_type& k, hash_code_t c, owning_node_type pNodeNew, value_type&& value, 
			SM_DISABLE_IF_TRUETYPE(BoolConstantT)) // false_type means bUniqueKeys is false.
	{
		const std::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, (uint32_t)1);

		if(bRehash.first)
			DoRehash(bRehash.second); // Note: We don't need to wrap this call with try/catch because there's nothing we would need to do in the catch.

		const bucket_index_t n = bucket_index(k, c, (uint32_t)mnBucketCount);

		// if(pNodeNew)
		// 	::new(std::addressof(pNodeNew->mValue)) value_type(std::move(value)); // It's expected that pNodeNew was allocated with allocate_uninitialized_node.
		// else
			pNodeNew = DoAllocateNode(std::move(value));

		set_code(*pNodeNew, c); // This is a no-op for most hashtables.

		// To consider: Possibly make this insertion not make equal elements contiguous.
		// As it stands now, we insert equal values contiguously in the hashtable.
		// The benefit is that equal_range can work in a sensible manner and that
		// erase(value) can more quickly find equal values. The downside is that
		// this insertion operation taking some extra time. How important is it to
		// us that equal_range span all equal items? 
		soft_node_type pNodePrev = DoFindNode(n, k, c);
		soft_node_type pNodeIt = pNodeNew;

		if(pNodePrev == nullptr)
		{
			// EASTL_ASSERT((void**)mpBucketArray != &gpEmptyBucketArray[0]);
			pNodeNew->mpNext = std::move(mpBucketArray->at_unsafe(n));
			mpBucketArray->at_unsafe(n) = std::move(pNodeNew);
		}
		else
		{
			pNodeNew->mpNext  = std::move(pNodePrev->mpNext);
			pNodePrev->mpNext = std::move(pNodeNew);
		}

		++mnElementCount;

		return iterator(pNodeIt, GetBucketArrayIt(n));
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template<typename BoolConstantT>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertValue(BoolConstantT, value_type&& value, SM_DISABLE_IF_TRUETYPE(BoolConstantT)) // false_type means bUniqueKeys is false.
	{
		const key_type&   k = mExtractKey(value);
		const hash_code_t c = get_hash_code(k);

		return DoInsertValueExtra(std::false_type(), k, c, {nullptr}, std::move(value));
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::owning_node_type
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoAllocateNode(value_type&& value)
	{
//		node_type* const pNode = (node_type*)allocate_memory(mAllocator, sizeof(node_type), EASTL_ALIGN_OF(value_type), 0);
		// node_type* const pNode = (node_type*)safememory::lib_helpers::allocate_memory(sizeof(node_type), alignof(value_type), 0);
		// EASTL_ASSERT_MSG(pNode != nullptr, "the behaviour of eastl::allocators that return nullptr is not defined.");

		// #if EASTL_EXCEPTIONS_ENABLED
		// 	try
		// 	{
		// #endif
		// 		::new(std::addressof(pNode->mValue)) value_type(std::move(value));
		// 		pNode->mpNext = NULL;
		// 		return pNode;
		// #if EASTL_EXCEPTIONS_ENABLED
		// 	}
		// 	catch(...)
		// 	{
		// 		// EASTLFree(mAllocator, pNode, sizeof(node_type));
		// 		safememory::lib_helpers::EASTLFree(pNode, sizeof(node_type));
		// 		throw;
		// 	}
		// #endif
		return make_owning_2<node_type, S>(std::move(value));
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template<typename BoolConstantT>
	std::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator, bool>
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertValueExtra(BoolConstantT, const key_type& k, hash_code_t c, owning_node_type pNodeNew, const value_type& value, 
			SM_ENABLE_IF_TRUETYPE(BoolConstantT)) // true_type means bUniqueKeys is true.
	{
		// Adds the value to the hash table if not already present. 
		// If already present then the existing value is returned via an iterator/bool pair.
		bucket_index_t         n     = bucket_index(k, c, (uint32_t)mnBucketCount);
		soft_node_type  pNode = DoFindNode(n, k, c);

		if(pNode == nullptr) // If value is not present... add it.
		{
			const std::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, (uint32_t)1);

			// Allocate the new node before doing the rehash so that we don't 
			// do a rehash if the allocation throws.
			// #if EASTL_EXCEPTIONS_ENABLED
			// 	bool nodeAllocated;  // If exceptions are enabled then we we need to track if we allocated the node so we can free it in the catch block.
			// #endif

			// if(pNodeNew)
			// {
			// 	::new(std::addressof(pNodeNew->mValue)) value_type(value); // It's expected that pNodeNew was allocated with allocate_uninitialized_node.
			// 	#if EASTL_EXCEPTIONS_ENABLED
			// 		nodeAllocated = false;
			// 	#endif
			// }
			// else
			// {
				pNodeNew = DoAllocateNode(value);
			// 	#if EASTL_EXCEPTIONS_ENABLED
			// 		nodeAllocated = true;
			// 	#endif
			// }

			set_code(*pNodeNew, c); // This is a no-op for most hashtables.

			// #if EASTL_EXCEPTIONS_ENABLED
				// try
				// {
			// #endif
					if(bRehash.first)
					{
						n = (size_type)bucket_index(k, c, (uint32_t)bRehash.second);
						DoRehash(bRehash.second);
					}

					// EASTL_ASSERT((uintptr_t)mpBucketArray != (uintptr_t)&gpEmptyBucketArray[0]);
					soft_node_type pNodeIt = pNodeNew;
					pNodeNew->mpNext = std::move(mpBucketArray->at_unsafe(n));
					mpBucketArray->at_unsafe(n) = std::move(pNodeNew);
					++mnElementCount;

					return std::pair<iterator, bool>(iterator(pNodeIt, GetBucketArrayIt(n)), true);
			// #if EASTL_EXCEPTIONS_ENABLED
				// }
				// catch(...)
				// {
				// 	// if(nodeAllocated) // If we allocated the node within this function, free it. Else let the caller retain ownership of it.
				// 	// 	DoFreeNode(pNodeNew);
				// 	throw;
				// }
			// #endif
		}
		// Else the value is already present, so don't add a new node. And don't free pNodeNew.

		return std::pair<iterator, bool>(iterator(pNode, GetBucketArrayIt(n)), false);
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
				typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template<typename BoolConstantT>
	std::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator, bool>
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertValue(BoolConstantT, const value_type& value, SM_ENABLE_IF_TRUETYPE(BoolConstantT)) // true_type means bUniqueKeys is true.
	{
		const key_type&   k = mExtractKey(value);
		const hash_code_t c = get_hash_code(k);

		return DoInsertValueExtra(std::true_type(), k, c, {nullptr}, value);
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
				typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <typename BoolConstantT>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertValueExtra(BoolConstantT, const key_type& k, hash_code_t c, owning_node_type pNodeNew, const value_type& value,
			SM_DISABLE_IF_TRUETYPE(BoolConstantT)) // false_type means bUniqueKeys is false.
	{
		const std::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, (uint32_t)1);

		if(bRehash.first)
			DoRehash(bRehash.second); // Note: We don't need to wrap this call with try/catch because there's nothing we would need to do in the catch.

		const bucket_index_t n = bucket_index(k, c, (uint32_t)mnBucketCount);

		// if(pNodeNew)
		// 	::new(std::addressof(pNodeNew->mValue)) value_type(value); // It's expected that pNodeNew was allocated with allocate_uninitialized_node.
		// else
			pNodeNew = DoAllocateNode(value);

		set_code(*pNodeNew, c); // This is a no-op for most hashtables.

		// To consider: Possibly make this insertion not make equal elements contiguous.
		// As it stands now, we insert equal values contiguously in the hashtable.
		// The benefit is that equal_range can work in a sensible manner and that
		// erase(value) can more quickly find equal values. The downside is that
		// this insertion operation taking some extra time. How important is it to
		// us that equal_range span all equal items? 
		soft_node_type pNodePrev = DoFindNode(n, k, c);
		soft_node_type pNodeIt = pNodeNew;

		if(pNodePrev == nullptr)
		{
			// EASTL_ASSERT((void**)mpBucketArray != &gpEmptyBucketArray[0]);
			pNodeNew->mpNext = std::move(mpBucketArray->at_unsafe(n));
			mpBucketArray->at_unsafe(n) = std::move(pNodeNew);
		}
		else
		{
			pNodeNew->mpNext  = std::move(pNodePrev->mpNext);
			pNodePrev->mpNext = std::move(pNodeNew);
		}

		++mnElementCount;

		return iterator(pNodeIt, GetBucketArrayIt(n));
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
				typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template<typename BoolConstantT>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertValue(BoolConstantT, const value_type& value, SM_DISABLE_IF_TRUETYPE(BoolConstantT)) // false_type means bUniqueKeys is false.
	{
		const key_type&   k = mExtractKey(value);
		const hash_code_t c = get_hash_code(k);

		return DoInsertValueExtra(std::false_type(), k, c, {nullptr}, value);
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::owning_node_type
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoAllocateNode(const value_type& value)
	{
		// node_type* const pNode = (node_type*)allocate_memory(mAllocator, sizeof(node_type), EASTL_ALIGN_OF(value_type), 0);
		// node_type* const pNode = (node_type*)safememory::lib_helpers::allocate_memory(sizeof(node_type), alignof(value_type), 0);
		// EASTL_ASSERT_MSG(pNode != nullptr, "the behaviour of eastl::allocators that return nullptr is not defined.");

		// #if EASTL_EXCEPTIONS_ENABLED
		// 	try
		// 	{
		// #endif
		// 		::new(std::addressof(pNode->mValue)) value_type(value);
		// 		pNode->mpNext = NULL;
		// 		return pNode;
		// #if EASTL_EXCEPTIONS_ENABLED
		// 	}
		// 	catch(...)
		// 	{
		// 		// EASTLFree(mAllocator, pNode, sizeof(node_type));
		// 		safememory::lib_helpers::EASTLFree(pNode, sizeof(node_type));
		// 		throw;
		// 	}
		// #endif
		return make_owning_2<node_type, S>(value);
	}


	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::owning_node_type
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::allocate_uninitialized_node()
	// {
	// 	// We don't wrap this in try/catch because users of this function are expected to do that themselves as needed.
	// 	// node_type* const pNode = (node_type*)allocate_memory(mAllocator, sizeof(node_type), EASTL_ALIGN_OF(value_type), 0);
	// 	node_type* const pNode = (node_type*)safememory::lib_helpers::allocate_memory(sizeof(node_type), alignof(value_type), 0);
	// 	EASTL_ASSERT_MSG(pNode != nullptr, "the behaviour of eastl::allocators that return nullptr is not defined.");
	// 	// Leave pNode->mValue uninitialized.
	// 	pNode->mpNext = NULL;
	// 	return pNode;
	// }


	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::free_uninitialized_node(owning_node_type pNode)
	// {
	// 	// pNode->mValue is expected to be uninitialized.
	// 	// EASTLFree(mAllocator, pNode, sizeof(node_type));
	// 	safememory::lib_helpers::EASTLFree(pNode, sizeof(node_type));
	// }


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	std::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator, bool>
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertKey(std::true_type, const key_type& key, const hash_code_t c) // true_type means bUniqueKeys is true.
	{
		bucket_index_t         n     = bucket_index(key, c, (uint32_t)mnBucketCount);
		soft_node_type  pNode = DoFindNode(n, key, c);

		if(pNode == nullptr)
		{
			const std::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, (uint32_t)1);

			// Allocate the new node before doing the rehash so that we don't
			// do a rehash if the allocation throws.
			owning_node_type pNodeNew = DoAllocateNodeFromKey(key);
			set_code(*pNodeNew, c); // This is a no-op for most hashtables.

			// #if EASTL_EXCEPTIONS_ENABLED
			// 	try
			// 	{
			// #endif
					if(bRehash.first)
					{
						n = (size_type)bucket_index(key, c, (uint32_t)bRehash.second);
						DoRehash(bRehash.second);
					}

					// EASTL_ASSERT((void**)mpBucketArray != &gpEmptyBucketArray[0]);
					soft_node_type pNodeIt = pNodeNew;
					pNodeNew->mpNext = std::move(mpBucketArray->at_unsafe(n));
					mpBucketArray->at_unsafe(n) = std::move(pNodeNew);
					++mnElementCount;

					return std::pair<iterator, bool>(iterator(pNodeIt, GetBucketArrayIt(n)), true);
			// #if EASTL_EXCEPTIONS_ENABLED
			// 	}
			// 	catch(...)
			// 	{
			// 		DoFreeNode(pNodeNew);
			// 		throw;
			// 	}
			// #endif
		}

		return std::pair<iterator, bool>(iterator(pNode, GetBucketArrayIt(n)), false);
	}



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertKey(std::false_type, const key_type& key, const hash_code_t c) // false_type means bUniqueKeys is false.
	{
		const std::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, (uint32_t)1);

		if(bRehash.first)
			DoRehash(bRehash.second);

		const bucket_index_t   n = bucket_index(key, c, (uint32_t)mnBucketCount);

		auto pNodeNew = DoAllocateNodeFromKey(key);
		set_code(*pNodeNew, c); // This is a no-op for most hashtables.

		// To consider: Possibly make this insertion not make equal elements contiguous.
		// As it stands now, we insert equal values contiguously in the hashtable.
		// The benefit is that equal_range can work in a sensible manner and that
		// erase(value) can more quickly find equal values. The downside is that
		// this insertion operation taking some extra time. How important is it to
		// us that equal_range span all equal items? 
		soft_node_type pNodePrev = DoFindNode(n, key, c);
		soft_node_type pNodeIt = pNodeNew;

		if(pNodePrev == nullptr)
		{
			// EASTL_ASSERT((void**)mpBucketArray != &gpEmptyBucketArray[0]);
			pNodeNew->mpNext = std::move(mpBucketArray->at_unsafe(n));
			mpBucketArray->at_unsafe(n) = std::move(pNodeNew);
		}
		else
		{
			pNodeNew->mpNext  = std::move(pNodePrev->mpNext);
			pNodePrev->mpNext = std::move(pNodeNew);
		}

		++mnElementCount;

		return iterator(pNodeIt, GetBucketArrayIt(n));
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
				typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	std::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator, bool>
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertKey(std::true_type, key_type&& key, const hash_code_t c) // true_type means bUniqueKeys is true.
	{
		bucket_index_t         n     = bucket_index(key, c, (uint32_t)mnBucketCount);
		soft_node_type  pNode = DoFindNode(n, key, c);

		if(pNode == nullptr)
		{
			const std::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, (uint32_t)1);

			// Allocate the new node before doing the rehash so that we don't
			// do a rehash if the allocation throws.
			auto pNodeNew = DoAllocateNodeFromKey(std::move(key));
			set_code(*pNodeNew, c); // This is a no-op for most hashtables.

			// #if EASTL_EXCEPTIONS_ENABLED
				// try
				// {
			// #endif
					if(bRehash.first)
					{
						n = (size_type)bucket_index(key, c, (uint32_t)bRehash.second);
						DoRehash(bRehash.second);
					}

					// EASTL_ASSERT((void**)mpBucketArray != &gpEmptyBucketArray[0]);
					soft_node_type pNodeIt = pNodeNew;
					pNodeNew->mpNext = std::move(mpBucketArray->at_unsafe(n));
					mpBucketArray->at_unsafe(n) = std::move(pNodeNew);
					++mnElementCount;

					return std::pair<iterator, bool>(iterator(pNodeIt, GetBucketArrayIt(n)), true);
			// #if EASTL_EXCEPTIONS_ENABLED
				// }
				// catch(...)
				// {
				// 	// DoFreeNode(pNodeNew);
				// 	throw;
				// }
			// #endif
		}

		return std::pair<iterator, bool>(iterator(pNode, GetBucketArrayIt(n)), false);
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
				typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertKey(std::false_type, key_type&& key, const hash_code_t c) // false_type means bUniqueKeys is false.
	{
		const std::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, (uint32_t)1);

		if(bRehash.first)
			DoRehash(bRehash.second);

		const bucket_index_t   n = bucket_index(key, c, (uint32_t)mnBucketCount);

		auto pNodeNew = DoAllocateNodeFromKey(std::move(key));
		set_code(*pNodeNew, c); // This is a no-op for most hashtables.

		// To consider: Possibly make this insertion not make equal elements contiguous.
		// As it stands now, we insert equal values contiguously in the hashtable.
		// The benefit is that equal_range can work in a sensible manner and that
		// erase(value) can more quickly find equal values. The downside is that
		// this insertion operation taking some extra time. How important is it to
		// us that equal_range span all equal items? 
		soft_node_type pNodePrev = DoFindNode(n, key, c);
		soft_node_type pNodeIt = pNodeNew;

		if(pNodePrev == nullptr)
		{
			// EASTL_ASSERT((void**)mpBucketArray != &gpEmptyBucketArray[0]);
			pNodeNew->mpNext = std::move(mpBucketArray->at_unsafe(n));
			mpBucketArray->at_unsafe(n) = std::move(pNodeNew);
		}
		else
		{
			pNodeNew->mpNext  = std::move(pNodePrev->mpNext);
			pNodePrev->mpNext = std::move(pNodeNew);
		}

		++mnElementCount;

		return iterator(pNodeIt, GetBucketArrayIt(n));
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
				typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <class... Args>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert_return_type
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::emplace(Args&&... args)
	{
		return DoInsertValue(has_unique_keys_type(), std::forward<Args>(args)...); // Need to use forward instead of move because Args&& is a "universal reference" instead of an rvalue reference.
	}

	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
				typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <class... Args>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::emplace_hint(const_iterator, Args&&... args)
	{
		// We currently ignore the iterator argument as a hint.
		insert_return_type result = DoInsertValue(has_unique_keys_type(), std::forward<Args>(args)...);
		return DoGetResultIterator(has_unique_keys_type(), result);
	}

	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	          typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <class... Args>
	// inline eastl::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator, bool>
	inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert_return_type
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::try_emplace(const key_type& key, Args&&... args)
	{
		return DoInsertValue(has_unique_keys_type(), std::piecewise_construct, std::forward_as_tuple(key),
		                     std::forward_as_tuple(std::forward<Args>(args)...));
	}

	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	          typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <class... Args>
	// inline eastl::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator, bool>
	inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert_return_type
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::try_emplace(key_type&& key, Args&&... args)
	{
		return DoInsertValue(has_unique_keys_type(), std::piecewise_construct, std::forward_as_tuple(std::move(key)),
		                     std::forward_as_tuple(std::forward<Args>(args)...));
	}

	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
				typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <class... Args>
	inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::try_emplace(const_iterator, const key_type& key, Args&&... args)
	{
		insert_return_type result = DoInsertValue(
		    has_unique_keys_type(),
		    value_type(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(std::forward<Args>(args)...)));

		return DoGetResultIterator(has_unique_keys_type(), result);
	}

	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
				typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <class... Args>
	inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::try_emplace(const_iterator, key_type&& key, Args&&... args)
	{
		insert_return_type result =
		    DoInsertValue(has_unique_keys_type(), value_type(std::piecewise_construct, std::forward_as_tuple(std::move(key)),
		                                                     std::forward_as_tuple(std::forward<Args>(args)...)));

		return DoGetResultIterator(has_unique_keys_type(), result);
	}

	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert_return_type
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert(value_type&& otherValue)
	{
		return DoInsertValue(has_unique_keys_type(), std::move(otherValue));
	}


	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// template <class P>
	// typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert_return_type
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert(hash_code_t c, owning_node_type pNodeNew, P&& otherValue)
	// {
	// 	// pNodeNew->mValue is expected to be uninitialized.
	// 	value_type value(std::forward<P>(otherValue)); // Need to use forward instead of move because P&& is a "universal reference" instead of an rvalue reference.
	// 	const key_type& k = mExtractKey(value);
	// 	return DoInsertValueExtra(has_unique_keys_type(), k, c, std::move(pNodeNew), std::move(value));
	// }


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert(const_iterator, value_type&& value)
	{
		// We currently ignore the iterator argument as a hint.
		insert_return_type result = DoInsertValue(has_unique_keys_type(), value_type(std::move(value)));
		return DoGetResultIterator(has_unique_keys_type(), result);
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert_return_type
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert(const value_type& value) 
	{
		return DoInsertValue(has_unique_keys_type(), value);
	}


	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert_return_type
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert(hash_code_t c, owninig_node_type pNodeNew, const value_type& value) 
	// {
	// 	// pNodeNew->mValue is expected to be uninitialized.
	// 	const key_type& k = mExtractKey(value);
	// 	return DoInsertValueExtra(has_unique_keys_type(), k, c, std::move(pNodeNew), value);
	// }


	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	//           typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// template <typename P, class>
	// typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert_return_type
	// hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert(P&& otherValue)
	// {
	// 	return emplace(std::forward<P>(otherValue));
	// }


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert(const_iterator, const value_type& value)
	{
		// We ignore the first argument (hint iterator). It's not likely to be useful for hashtable containers.
		insert_return_type result = DoInsertValue(has_unique_keys_type(), value);
		return result.first; // Note by Paul Pedriana while perusing this code: This code will fail to compile when bU is false (i.e. for multiset, multimap).
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert(std::initializer_list<value_type> ilist)
	{
		insert_unsafe(ilist.begin(), ilist.end());
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <typename InputIterator>
	void
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert_unsafe(InputIterator first, InputIterator last)
	{
		const uint32_t nElementAdd = (uint32_t)ht_distance(first, last);
		const std::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, nElementAdd);

		if(bRehash.first)
			DoRehash(bRehash.second);

		for(; first != last; ++first)
			DoInsertValue(has_unique_keys_type(), *first);
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	          typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <class M>
	std::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator, bool>
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert_or_assign(const key_type& k, M&& obj)
	{
		auto iter = find(k);
		if(iter == end())
		{
			return insert(value_type(std::piecewise_construct, std::forward_as_tuple(k), std::forward_as_tuple(std::forward<M>(obj))));
		}
		else
		{
			iter->second = std::forward<M>(obj);
			return {iter, false};
		}
	}

	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <class M>
	std::pair<typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator, bool>
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert_or_assign(key_type&& k, M&& obj)
	{
		auto iter = find(k);
		if(iter == end())
		{
			return insert(value_type(std::piecewise_construct, std::forward_as_tuple(std::move(k)), std::forward_as_tuple(std::forward<M>(obj))));
		}
		else
		{
			iter->second = std::forward<M>(obj);
			return {iter, false};
		}
	}

	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <class M>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator 
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert_or_assign(const_iterator, const key_type& k, M&& obj)
	{
		return insert_or_assign(k, std::forward<M>(obj)).first; // we ignore the iterator hint
	}

	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	template <class M>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator 
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert_or_assign(const_iterator, key_type&& k, M&& obj)
	{
		return insert_or_assign(std::move(k), std::forward<M>(obj)).first; // we ignore the iterator hint
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::erase(const_iterator i)
	{
		iterator iNext(i.mpNode, i.mpBucket); // Convert from const_iterator to iterator while constructing.
		++iNext;

		soft_node_type pNode        =  i.mpNode;
		soft_node_type pNodeCurrent = *i.mpBucket;

		if(*i.mpBucket == pNode) {
			owning_node_type tmp = std::move(*i.mpBucket);
			*i.mpBucket = std::move(tmp->mpNext);
			DoFreeNode(std::move(tmp));
			--mnElementCount;
		}
		else
		{
			// We have a singly-linked list, so we have no choice but to
			// walk down it till we find the node before the node at 'i'.
			soft_node_type pNodeCurrent = *i.mpBucket;
			soft_node_type pNodeNext = pNodeCurrent->mpNext;

			while(pNodeNext != pNode)
			{
				pNodeCurrent = pNodeNext;
				pNodeNext    = pNodeCurrent->mpNext;
			}

			owning_node_type tmp = std::move(pNodeCurrent->mpNext);
			pNodeCurrent->mpNext = std::move(tmp->mpNext);
			DoFreeNode(std::move(tmp));
			--mnElementCount;
		}


		return iNext;
	}



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::erase(const_iterator first, const_iterator last)
	{
		while(first != last)
			first = erase(first);
		return iterator(first.mpNode, first.mpBucket);
	}



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	typename hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::size_type 
	hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::erase(const key_type& k)
	{
		// To do: Reimplement this function to do a single loop and not try to be 
		// smart about element contiguity. The mechanism here is only a benefit if the 
		// buckets are heavily overloaded; otherwise this mechanism may be slightly slower.

		// const hash_code_t c = get_hash_code(k);
		// const size_type   n = (size_type)bucket_index(k, c, (uint32_t)mnBucketCount);
		const size_type   nElementCountSaved = mnElementCount;

		// auto pBucketArray = mpBucketArray + n;

		// while(*pBucketArray && !compare(k, c, *pBucketArray))
		// 	pBucketArray = &(*pBucketArray)->mpNext;

		// while(*pBucketArray && compare(k, c, *pBucketArray))
		// {
		// 	node_type* const pNode = *pBucketArray;
		// 	*pBucketArray = pNode->mpNext;
		// 	DoFreeNode(pNode);
		// 	--mnElementCount;
		// }

		std::pair<const_iterator, const_iterator> p = equal_range(k);
		erase(p.first, p.second);

		return nElementCountSaved - mnElementCount;
	}



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::clear()
	{
		DoFreeNodes(mpBucketArray, mnBucketCount);
		mnElementCount = 0;
	}



	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::clear(bool clearBuckets)
	{
		DoFreeNodes(mpBucketArray, mnBucketCount);
		if(clearBuckets)
		{
			DoFreeBuckets(std::move(mpBucketArray), mnBucketCount);
			DoInit();
		}
		mnElementCount = 0;
	}



	// template <typename K, typename V, memory_safety S, typename EK, typename Eq,
	// 		  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	// inline void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::reset_lose_memory() noexcept
	// {
	// 	// The reset function is a special extension function which unilaterally 
	// 	// resets the container to an empty state without freeing the memory of 
	// 	// the contained objects. This is useful for very quickly tearing down a 
	// 	// container built into scratch memory.
	// 	mnBucketCount  = 1;

	// 	#ifdef _MSC_VER
	// 		mpBucketArray = (node_type**)&gpEmptyBucketArray[0];
	// 	#else
	// 		void* p = &gpEmptyBucketArray[0];
	// 		memcpy(&mpBucketArray, &p, sizeof(mpBucketArray)); // Other compilers implement strict aliasing and casting is thus unsafe.
	// 	#endif

	// 	mnElementCount = 0;
	// 	mRehashPolicy.mnNextResize = 0;
	// }

	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::reserve(size_type nElementCount)
	{
		rehash(mRehashPolicy.GetBucketCount(uint32_t(nElementCount)));
	}

	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::rehash(size_type nBucketCount)
	{
		// Note that we unilaterally use the passed in bucket count; we do not attempt migrate it
		// up to the next prime number. We leave it at the user's discretion to do such a thing.
		DoRehash(nBucketCount);
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInit()
	{
		mnBucketCount = (size_type)mRehashPolicy.GetNextBucketCount(1);
		mpBucketArray = DoAllocateBuckets(mnBucketCount); // mnBucketCount will always be at least 2.
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	void hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoRehash(size_type nNewBucketCount)
	{
		auto pNewBucketArray = DoAllocateBuckets(nNewBucketCount); // nNewBucketCount should always be >= 2.

		// #if EASTL_EXCEPTIONS_ENABLED
			try
			{
		// #endif
				owning_node_type pNode;

				for(size_type i = 0; i < mnBucketCount; ++i)
				{
					while((pNode = std::move(mpBucketArray->at_unsafe(i))) != nullptr) // Using '!=' disables compiler warnings.
					{
						const size_type nNewBucketIndex = (size_type)bucket_index(*pNode, (uint32_t)nNewBucketCount);

						mpBucketArray->at_unsafe(i) = std::move(pNode->mpNext);
						pNode->mpNext    = std::move(pNewBucketArray->at_unsafe(nNewBucketIndex));
						pNewBucketArray->at_unsafe(nNewBucketIndex) = std::move(pNode);
					}
				}

				DoFreeBuckets(std::move(mpBucketArray), mnBucketCount);
				mnBucketCount = nNewBucketCount;
				mpBucketArray = std::move(pNewBucketArray);
		// #if EASTL_EXCEPTIONS_ENABLED
			}
			catch(...)
			{
				// A failure here means that a hash function threw an exception.
				// We can't restore the previous state without calling the hash
				// function again, so the only sensible recovery is to delete everything.
				DoFreeNodes(pNewBucketArray, nNewBucketCount);
				DoFreeBuckets(std::move(pNewBucketArray), nNewBucketCount);
				DoFreeNodes(mpBucketArray, mnBucketCount);
				mnElementCount = 0;
				throw;
			}
		// #endif
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline bool hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::validate() const
	{
		// Verify our empty bucket array is unmodified.
		// if(gpEmptyBucketArray[0] != NULL)
		// 	return false;

		// if(gpEmptyBucketArray[1] != (void*)uintptr_t(~0))
		// 	return false;

		// Verify that we have at least one bucket. Calculations can  
		// trigger division by zero exceptions otherwise.
		if(mnBucketCount == 0)
			return false;

		// Verify that gpEmptyBucketArray is used correctly.
		// gpEmptyBucketArray is only used when initially empty.
		// if((void**)mpBucketArray == &gpEmptyBucketArray[0])
		// {
		// 	if(mnElementCount) // gpEmptyBucketArray is used only for empty hash tables.
		// 		return false;

		// 	if(mnBucketCount != 1) // gpEmptyBucketArray is used exactly an only for mnBucketCount == 1.
		// 		return false;
		// }
		// else
		// {
		// 	if(mnBucketCount < 2) // Small bucket counts *must* use gpEmptyBucketArray.
		// 		return false;
		// }

		// Verify that the element count matches mnElementCount. 
		size_type nElementCount = 0;

		for(const_iterator temp = begin(), tempEnd = end(); temp != tempEnd; ++temp)
			++nElementCount;

		if(nElementCount != mnElementCount)
			return false;

		// To do: Verify that individual elements are in the expected buckets.

		return true;
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	iterator_validity hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>::validate_iterator(const_iterator i) const
	{
		if(i == const_iterator())
			return iterator_validity::Null;
		// else if(i.mpBucket.arr == mpBucketArray ) {
			
			//is mine and current
			if(i == end())
				return iterator_validity::ValidEnd; 
			// To do: Come up with a more efficient mechanism of doing this.
			for(const_iterator temp = begin(), tempEnd = end(); temp != tempEnd; ++temp)
			{
				if(temp == i)
					return iterator_validity::ValidCanDeref;
			}
		// }
		//TODO 
		return iterator_validity::InvalidZoombie;
	}



	///////////////////////////////////////////////////////////////////////
	// global operators
	///////////////////////////////////////////////////////////////////////

	// operator==, != have been moved to the specific container subclasses (e.g. hash_map).

	// The following comparison operators are deprecated and will likely be removed in a  
	// future version of this package.
	//
	// Comparing hash tables for less-ness is an odd thing to do. We provide it for 
	// completeness, though the user is advised to be wary of how they use this.
	//
	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline bool operator<(const hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>& a, 
						  const hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>& b)
	{
		// This requires hash table elements to support operator<. Since the hash table
		// doesn't compare elements via less (it does so via equals), we must use the 
		// globally defined operator less for the elements.
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline bool operator>(const hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>& a, 
						  const hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>& b)
	{
		return b < a;
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline bool operator<=(const hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>& a, 
						   const hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>& b)
	{
		return !(b < a);
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline bool operator>=(const hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>& a, 
						   const hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>& b)
	{
		return !(a < b);
	}


	template <typename K, typename V, memory_safety S, typename EK, typename Eq,
			  typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
	inline void swap(const hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>& a, 
					 const hashtable<K, V, S, EK, Eq, H1, H2, H, RP, bC, bM, bU>& b)
	{
		a.swap(b);
	}


} // namespace safe_memory


#ifdef _MSC_VER
	#pragma warning(pop)
#endif


#endif // Header include guard








