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
// https://github.com/electronicarts/EASTL/blob/3.15.00/include/EASTL/hash_set.h

///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file is based on the TR1 (technical report 1) reference implementation
// of the unordered_set/unordered_map C++ classes as of about 4/2005. Most likely
// many or all C++ library vendors' implementations of this classes will be 
// based off of the reference version and so will look pretty similar to this
// file as well as other vendors' versions. 
///////////////////////////////////////////////////////////////////////////////


#ifndef SAFE_MEMORY_EASTL_HASH_SET_H
#define SAFE_MEMORY_EASTL_HASH_SET_H


#include <safe_memory/EASTL/internal/hashtable.h>
#include <functional>


namespace safe_memory
{
	/// hash_set
	///
	/// Implements a hash_set, which is a hashed unique-item container.
	/// Lookups are O(1) (that is, they are fast) but the container is 
	/// not sorted. Note that lookups are only O(1) if the hash table
	/// is well-distributed (non-colliding). The lookup approaches
	/// O(n) behavior as the table becomes increasingly poorly distributed.
	///
	/// set_max_load_factor
	/// If you want to make a hashtable never increase its bucket usage,
	/// call set_max_load_factor with a very high value such as 100000.f.
	///
	/// bCacheHashCode
	/// We provide the boolean bCacheHashCode template parameter in order 
	/// to allow the storing of the hash code of the key within the map. 
	/// When this option is disabled, the rehashing of the table will 
	/// call the hash function on the key. Setting bCacheHashCode to true 
	/// is useful for cases whereby the calculation of the hash value for
	/// a contained object is very expensive.
	///
	/// find_as
	/// In order to support the ability to have a hashtable of strings but
	/// be able to do efficiently lookups via char pointers (i.e. so they 
	/// aren't converted to string objects), we provide the find_as 
	/// function. This function allows you to do a find with a key of a
	/// type other than the hashtable key type.
	///
	/// Example find_as usage:
	///     hash_set<string> hashSet;
	///     i = hashSet.find_as("hello");    // Use default hash and compare.
	///
	/// Example find_as usage (namespaces omitted for brevity):
	///     hash_set<string> hashSet;
	///     i = hashSet.find_as("hello", hash<char*>(), equal_to_2<string, char*>());
	///
	template <typename Value, typename Hash = hash<Value>, typename Predicate = equal_to<Value>, 
			  memory_safety Safety = safeness_declarator<Value>::is_safe, bool bCacheHashCode = false>
	class SAFE_MEMORY_DEEP_CONST_WHEN_PARAMS hash_set
		: public detail::hashtable<Value, Value, Safety, detail::use_self<Value>, Predicate,
						   Hash, detail::mod_range_hashing, detail::default_ranged_hash, 
						   detail::prime_rehash_policy, bCacheHashCode, false, true>
	{
	public:
		typedef detail::hashtable<Value, Value, Safety, detail::use_self<Value>, Predicate, 
						  Hash, detail::mod_range_hashing, detail::default_ranged_hash,
						  detail::prime_rehash_policy, bCacheHashCode, false, true>       base_type;
		typedef hash_set<Value, Hash, Predicate, Safety, bCacheHashCode>       this_type;
		typedef typename base_type::size_type                                     size_type;
		typedef typename base_type::value_type                                    value_type;
		// typedef typename base_type::allocator_type                                allocator_type;
		typedef typename base_type::node_type                                     node_type;

	public:
		/// hash_set
		///
		/// Default constructor.
		/// 
		explicit hash_set(/*const allocator_type& allocator = EASTL_HASH_SET_DEFAULT_ALLOCATOR*/)
			: base_type(0, Hash(), detail::mod_range_hashing(), detail::default_ranged_hash(), Predicate(), detail::use_self<Value>()/*, allocator*/)
		{
			// Empty
		}


		/// hash_set
		///
		/// Constructor which creates an empty container, but start with nBucketCount buckets.
		/// We default to a small nBucketCount value, though the user really should manually 
		/// specify an appropriate value in order to prevent memory from being reallocated.
		///
		explicit hash_set(size_type nBucketCount, const Hash& hashFunction = Hash(), const Predicate& predicate = Predicate()/*, 
						  const allocator_type& allocator = EASTL_HASH_SET_DEFAULT_ALLOCATOR*/)
			: base_type(nBucketCount, hashFunction, detail::mod_range_hashing(), detail::default_ranged_hash(), predicate, detail::use_self<Value>()/*, allocator*/)
		{
			// Empty
		}


		hash_set(const this_type& x)
		  : base_type(x)
		{
		}


		hash_set(this_type&& x)
		  : base_type(std::move(x))
		{
		}


		// hash_set(this_type&& x, const allocator_type& allocator)
		//   : base_type(std::move(x), allocator)
		// {
		// }


		/// hash_set
		///
		/// initializer_list-based constructor. 
		/// Allows for initializing with brace values (e.g. hash_set<int> hs = { 3, 4, 5, }; )
		///     
		hash_set(std::initializer_list<value_type> ilist, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
				   const Predicate& predicate = Predicate()/*, const allocator_type& allocator = EASTL_HASH_SET_DEFAULT_ALLOCATOR*/)
			: base_type(nBucketCount, hashFunction, detail::mod_range_hashing(), detail::default_ranged_hash(), predicate, detail::use_self<Value>()/*, allocator*/)
		{
			//TODO: mb: improve, since we know the list size before construction
			base_type::insert_unsafe(ilist.begin(), ilist.end());
		}


		/// hash_set
		///
		/// An input bucket count of <= 1 causes the bucket count to be equal to the number of 
		/// elements in the input range.
		///
		// template <typename FowardIterator>
		// hash_set(FowardIterator first, FowardIterator last, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
		// 		 const Predicate& predicate = Predicate()/*, const allocator_type& allocator = EASTL_HASH_SET_DEFAULT_ALLOCATOR*/)
		// 	: base_type(first, last, nBucketCount, hashFunction, detail::mod_range_hashing(), detail::default_ranged_hash(), predicate, detail::use_self<Value>()/*, allocator*/)
		// {
		// 	// Empty
		// }


		this_type& operator=(const this_type& x)
		{
			return static_cast<this_type&>(base_type::operator=(x));
		}


		this_type& operator=(std::initializer_list<value_type> ilist)
		{
			return static_cast<this_type&>(base_type::operator=(ilist));
		}


		this_type& operator=(this_type&& x)
		{
			return static_cast<this_type&>(base_type::operator=(std::move(x)));
		}

	}; // hash_set






	/// hash_multiset
	///
	/// Implements a hash_multiset, which is the same thing as a hash_set 
	/// except that contained elements need not be unique. See the documentation 
	/// for hash_set for details.
	///
	template <typename Value, typename Hash = hash<Value>, typename Predicate = equal_to<Value>, 
			  memory_safety Safety = safeness_declarator<Value>::is_safe, bool bCacheHashCode = false>
	class SAFE_MEMORY_DEEP_CONST_WHEN_PARAMS hash_multiset
		: public detail::hashtable<Value, Value, Safety, detail::use_self<Value>, Predicate,
						   Hash, detail::mod_range_hashing, detail::default_ranged_hash,
						   detail::prime_rehash_policy, bCacheHashCode, false, false>
	{
	public:
		typedef detail::hashtable<Value, Value, Safety, detail::use_self<Value>, Predicate,
						  Hash, detail::mod_range_hashing, detail::default_ranged_hash,
						  detail::prime_rehash_policy, bCacheHashCode, false, false>          base_type;
		typedef hash_multiset<Value, Hash, Predicate, Safety, bCacheHashCode>      this_type;
		typedef typename base_type::size_type                                         size_type;
		typedef typename base_type::value_type                                        value_type;
		// typedef typename base_type::allocator_type                                    allocator_type;
		typedef typename base_type::node_type                                         node_type;

		using base_type::insert_unsafe;

	public:
		/// hash_multiset
		///
		/// Default constructor.
		/// 
		explicit hash_multiset(/*const allocator_type& allocator = EASTL_HASH_MULTISET_DEFAULT_ALLOCATOR*/)
			: base_type(0, Hash(), detail::mod_range_hashing(), detail::default_ranged_hash(), Predicate(), detail::use_self<Value>()/*, allocator*/)
		{
			// Empty
		}


		/// hash_multiset
		///
		/// Constructor which creates an empty container, but start with nBucketCount buckets.
		/// We default to a small nBucketCount value, though the user really should manually 
		/// specify an appropriate value in order to prevent memory from being reallocated.
		///
		explicit hash_multiset(size_type nBucketCount, const Hash& hashFunction = Hash(), 
							   const Predicate& predicate = Predicate()/*, const allocator_type& allocator = EASTL_HASH_MULTISET_DEFAULT_ALLOCATOR*/)
			: base_type(nBucketCount, hashFunction, detail::mod_range_hashing(), detail::default_ranged_hash(), predicate, detail::use_self<Value>()/*, allocator*/)
		{
			// Empty
		}


		hash_multiset(const this_type& x)
		  : base_type(x)
		{
		}


		hash_multiset(this_type&& x)
		  : base_type(std::move(x))
		{
		}


		// hash_multiset(this_type&& x, const allocator_type& allocator)
		//   : base_type(std::move(x), allocator)
		// {
		// }


		/// hash_multiset
		///
		/// initializer_list-based constructor. 
		/// Allows for initializing with brace values (e.g. hash_set<int> hs = { 3, 3, 4, }; )
		///     
		hash_multiset(std::initializer_list<value_type> ilist, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
				   const Predicate& predicate = Predicate()/*, const allocator_type& allocator = EASTL_HASH_MULTISET_DEFAULT_ALLOCATOR*/)
			: base_type(nBucketCount, hashFunction, detail::mod_range_hashing(), detail::default_ranged_hash(), predicate, detail::use_self<Value>()/*, allocator*/)
		{
			//TODO: mb: improve, since we know the list size before construction
			base_type::insert_unsafe(ilist.begin(), ilist.end());
		}


		/// hash_multiset
		///
		/// An input bucket count of <= 1 causes the bucket count to be equal to the number of 
		/// elements in the input range.
		///
		// template <typename FowardIterator>
		// hash_multiset(FowardIterator first, FowardIterator last, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
		// 			  const Predicate& predicate = Predicate()/*, const allocator_type& allocator = EASTL_HASH_MULTISET_DEFAULT_ALLOCATOR*/)
		// 	: base_type(first, last, nBucketCount, hashFunction, detail::mod_range_hashing(), detail::default_ranged_hash(), predicate, detail::use_self<Value>()/*, allocator*/)
		// {
		// 	// Empty
		// }


		this_type& operator=(const this_type& x)
		{
			return static_cast<this_type&>(base_type::operator=(x));
		}


		this_type& operator=(std::initializer_list<value_type> ilist)
		{
			return static_cast<this_type&>(base_type::operator=(ilist));
		}


		this_type& operator=(this_type&& x)
		{
			return static_cast<this_type&>(base_type::operator=(std::move(x)));
		}

	}; // hash_multiset



	///////////////////////////////////////////////////////////////////////
	// global operators
	///////////////////////////////////////////////////////////////////////

	template <typename Value, typename Hash, typename Predicate, memory_safety Safety, bool bCacheHashCode>
	inline bool operator==(const hash_set<Value, Hash, Predicate, Safety, bCacheHashCode>& a, 
						   const hash_set<Value, Hash, Predicate, Safety, bCacheHashCode>& b)
	{
		typedef typename hash_set<Value, Hash, Predicate, Safety, bCacheHashCode>::const_iterator const_iterator;

		// We implement branching with the assumption that the return value is usually false.
		if(a.size() != b.size())
			return false;

		// For set (with its unique keys), we need only test that each element in a can be found in b,
		// as there can be only one such pairing per element. multiset needs to do a something more elaborate.
		for(const_iterator ai = a.begin(), aiEnd = a.end(), biEnd = b.end(); ai != aiEnd; ++ai)
		{
			const_iterator bi = b.find(*ai);

			if((bi == biEnd) || !(*ai == *bi)) // We have to compare values in addition to making sure the lookups succeeded. This is because the lookup is done via the user-supplised Predicate
				return false;                  // which isn't strictly required to be identical to the Value operator==, though 99% of the time it will be so.  
		}

		return true;
	}

	template <typename Value, typename Hash, typename Predicate, memory_safety Safety, bool bCacheHashCode>
	inline bool operator!=(const hash_set<Value, Hash, Predicate, Safety, bCacheHashCode>& a, 
						   const hash_set<Value, Hash, Predicate, Safety, bCacheHashCode>& b)
	{
		return !(a == b);
	}


	template <typename Value, typename Hash, typename Predicate, memory_safety Safety, bool bCacheHashCode>
	inline bool operator==(const hash_multiset<Value, Hash, Predicate, Safety, bCacheHashCode>& a, 
						   const hash_multiset<Value, Hash, Predicate, Safety, bCacheHashCode>& b)
	{
		typedef typename hash_multiset<Value, Hash, Predicate, Safety, bCacheHashCode>::const_iterator const_iterator;
		typedef typename std::iterator_traits<const_iterator>::difference_type difference_type;

		// We implement branching with the assumption that the return value is usually false.
		if(a.size() != b.size())
			return false;

		// We can't simply search for each element of a in b, as it may be that the bucket for 
		// two elements in a has those same two elements in b but in different order (which should 
		// still result in equality). Also it's possible that one bucket in a has two elements which 
		// both match a solitary element in the equivalent bucket in b (which shouldn't result in equality).
		// std::pair<const_iterator, const_iterator> aRange;
		// std::pair<const_iterator, const_iterator> bRange;

		const_iterator ai = a.begin();
		const_iterator aiEnd = a.end();
		while(ai != aiEnd) // For each element in a...
		{
			std::pair<const_iterator, const_iterator> aRange = a.equal_range(*ai); // Get the range of elements in a that are equal to ai.
			std::pair<const_iterator, const_iterator> bRange = b.equal_range(*ai); // Get the range of elements in b that are equal to ai.

			// We need to verify that aRange == bRange. First make sure the range sizes are equivalent...
			const difference_type aDistance = std::distance(aRange.first, aRange.second);
			const difference_type bDistance = std::distance(bRange.first, bRange.second);

			if(aDistance != bDistance)
				return false;

			// At this point, aDistance > 0 and aDistance == bDistance.
			// Implement a fast pathway for the case that there's just a single element.
			if(aDistance == 1)
			{
				if(!(*aRange.first == *bRange.first))   // We have to compare values in addition to making sure the distance (element count) was equal. This is because the lookup is done via the user-supplised Predicate
					return false;                       // which isn't strictly required to be identical to the Value operator==, though 99% of the time it will be so. Ditto for the is_permutation usage below.
			}
			else
			{
				// Check to see if these aRange and bRange are any permutation of each other. 
				// This check gets slower as there are more elements in the range.
				if(!std::is_permutation(aRange.first, aRange.second, bRange.first))
					return false;
			}
			
			ai = aRange.second;
		}

		return true;
	}

	template <typename Value, typename Hash, typename Predicate, memory_safety Safety, bool bCacheHashCode>
	inline bool operator!=(const hash_multiset<Value, Hash, Predicate, Safety, bCacheHashCode>& a, 
						   const hash_multiset<Value, Hash, Predicate, Safety, bCacheHashCode>& b)
	{
		return !(a == b);
	}

} // namespace eastl


#endif // Header include guard











