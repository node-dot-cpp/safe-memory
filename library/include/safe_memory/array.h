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

/////////////////////////////////////////////////////////////////////////////
// First version inspired from EASTL-3.17.03
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef SAFE_MEMORY_ARRAY_H
#define SAFE_MEMORY_ARRAY_H

#include <EASTL/internal/config.h>
#include <EASTL/iterator.h>
#include <EASTL/algorithm.h>
#include <safe_memory/safe_ptr.h>
#include <safe_memory/detail/array_of.h>

namespace safe_memory
{
	template <typename T, eastl_size_t N = 1, memory_safety Safety = safeness_declarator<T>::is_safe>
	struct array
	{
	public:
		typedef array<T, N, Safety>                           this_type;
		typedef T                                             value_type;
		typedef value_type&                                   reference;
		typedef const value_type&                             const_reference;
		typedef value_type*                                   pointer;
		typedef const value_type*                             const_pointer;
		typedef eastl_size_t                                  size_type;
		typedef ptrdiff_t                                     difference_type;

		static constexpr memory_safety is_safe = Safety;
		typedef soft_ptr<this_type, is_safe>                                       soft_ptr_this_type;

		typedef typename detail::array_of_iterator_stack<T>                        stack_only_iterator;
		typedef typename detail::const_array_of_iterator_stack<T>                  const_stack_only_iterator;
		typedef typename detail::array_of_iterator<T, false, soft_ptr_this_type, true>  heap_safe_iterator;
		typedef typename detail::array_of_iterator<T, true, soft_ptr_this_type, true>   const_heap_safe_iterator;

		static constexpr bool use_base_iterator = Safety == memory_safety::none;
		
		typedef std::conditional_t<use_base_iterator, pointer, stack_only_iterator>               iterator;
		typedef std::conditional_t<use_base_iterator, const_pointer, const_stack_only_iterator>   const_iterator;
		typedef eastl::reverse_iterator<iterator>                                  reverse_iterator;
		typedef eastl::reverse_iterator<const_iterator>                            const_reverse_iterator;

		typedef heap_safe_iterator                                         iterator_safe;
		typedef const_heap_safe_iterator                                   const_iterator_safe;
		typedef eastl::reverse_iterator<iterator_safe>                     reverse_iterator_safe;
		typedef eastl::reverse_iterator<const_iterator_safe>               const_reverse_iterator_safe;


	public:
		enum
		{
			count = N
		};

		// Note that the member data is intentionally public.
		// This allows for aggregate initialization of the
		// object (e.g. array<int, 5> a = { 0, 3, 2, 4 }; )
		value_type mValue[N];

	public:
		// We intentionally provide no constructor, destructor, or assignment operator.

		void fill(const value_type& value) { 
			eastl::fill_n(&mValue[0], N, value);
		}

		// Unlike the swap function for other containers, array::swap takes linear time,
		// may exit via an exception, and does not cause iterators to become associated with the other container.
		void swap(this_type& x) noexcept(eastl::is_nothrow_swappable<value_type>::value) {
			eastl::swap_ranges(&mValue[0], &mValue[N], &x.mValue[0]);
		}

		constexpr pointer       begin_unsafe() noexcept { return &mValue[0]; }
		constexpr const_pointer begin_unsafe() const noexcept { return &mValue[0]; }
		constexpr const_pointer cbegin_unsafe() const noexcept { return &mValue[0]; }

		constexpr pointer       end_unsafe() noexcept { return &mValue[N]; }
		constexpr const_pointer end_unsafe() const noexcept { return &mValue[N]; }
		constexpr const_pointer cend_unsafe() const noexcept { return &mValue[N]; }

		constexpr iterator       begin() noexcept { return makeIt(begin_unsafe()); }
		constexpr const_iterator begin() const noexcept { return makeIt(begin_unsafe()); }
		constexpr const_iterator cbegin() const noexcept { return makeIt(begin_unsafe()); }

		constexpr iterator       end() noexcept { return makeIt(end_unsafe()); }
		constexpr const_iterator end() const noexcept { return makeIt(end_unsafe()); }
		constexpr const_iterator cend() const noexcept { return makeIt(end_unsafe()); }

		constexpr reverse_iterator       rbegin() noexcept { return reverse_iterator(makeIt(end_unsafe())); }
		constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(makeIt(end_unsafe())); }
		constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(makeIt(end_unsafe())); }

		constexpr reverse_iterator       rend() noexcept { return reverse_iterator(makeIt(begin_unsafe())); }
		constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(makeIt(begin_unsafe())); }
		constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(makeIt(begin_unsafe())); }

		constexpr iterator_safe       begin_safe(const soft_ptr_this_type& ptr) noexcept { return makeSafeIt(ptr, begin_unsafe()); }
		constexpr const_iterator_safe begin_safe(const soft_ptr_this_type& ptr) const noexcept { return makeSafeIt(ptr, begin_unsafe()); }
		constexpr const_iterator_safe cbegin_safe(const soft_ptr_this_type& ptr) const noexcept { return makeSafeIt(ptr, begin_unsafe()); }

		constexpr iterator_safe       end_safe(const soft_ptr_this_type& ptr) noexcept { return makeSafeIt(ptr, end_unsafe()); }
		constexpr const_iterator_safe end_safe(const soft_ptr_this_type& ptr) const noexcept { return makeSafeIt(ptr, end_unsafe()); }
		constexpr const_iterator_safe cend_safe(const soft_ptr_this_type& ptr) const noexcept { return makeSafeIt(ptr, end_unsafe()); }

		constexpr reverse_iterator_safe       rbegin_safe(const soft_ptr_this_type& ptr) noexcept { return reverse_iterator_safe(makeSafeIt(ptr, end_unsafe())); }
		constexpr const_reverse_iterator_safe rbegin_safe(const soft_ptr_this_type& ptr) const noexcept { return const_reverse_iterator_safe(makeSafeIt(ptr, end_unsafe())); }
		constexpr const_reverse_iterator_safe crbegin_safe(const soft_ptr_this_type& ptr) const noexcept { return const_reverse_iterator_safe(makeSafeIt(ptr, end_unsafe())); }

		constexpr reverse_iterator_safe       rend_safe(const soft_ptr_this_type& ptr) noexcept { return reverse_iterator_safe(makeSafeIt(ptr, begin_unsafe())); }
		constexpr const_reverse_iterator_safe rend_safe(const soft_ptr_this_type& ptr) const noexcept { return const_reverse_iterator_safe(makeSafeIt(ptr, begin_unsafe())); }
		constexpr const_reverse_iterator_safe crend_safe(const soft_ptr_this_type& ptr) const noexcept { return const_reverse_iterator_safe(makeSafeIt(ptr, begin_unsafe())); }

		constexpr bool empty() const noexcept { return N == 0; }
		constexpr size_type size() const noexcept { return N; }
		constexpr size_type max_size() const noexcept { return N; }

		constexpr T*       data() noexcept { return mValue; }
		constexpr const T* data() const noexcept { return mValue; }

		constexpr reference       operator[](size_type i) { return at(i); }
		constexpr const_reference operator[](size_type i) const { return at(i); }
 
		constexpr const_reference at(size_type i) const {
			if constexpr(is_safe == memory_safety::safe) {
				if(NODECPP_UNLIKELY(i >= size()))
					ThrowRangeException("array::at -- out of range");
			}

			return mValue[i];
		}

		constexpr reference       at(size_type i) {
			if constexpr(is_safe == memory_safety::safe) {
				if(NODECPP_UNLIKELY(i >= size()))
					ThrowRangeException("array::at -- out of range");
			}

			return mValue[i];
		}

		constexpr reference       front() {
			if constexpr(is_safe == memory_safety::safe) {
				if(NODECPP_UNLIKELY(empty()))
					ThrowRangeException("array::front -- empty array");
			}

			return mValue[0];
		}

		constexpr const_reference front() const {
			if constexpr(is_safe == memory_safety::safe) {
				if(NODECPP_UNLIKELY(empty()))
					ThrowRangeException("array::front -- empty array");
			}

			return mValue[0];
		}

		constexpr reference       back() {
			if constexpr(is_safe == memory_safety::safe) {
				if(NODECPP_UNLIKELY(empty()))
					ThrowRangeException("array::front -- empty array");
			}

			return mValue[N -1];
		}
		constexpr const_reference back() const {
			if constexpr(is_safe == memory_safety::safe) {
				if(NODECPP_UNLIKELY(empty()))
					ThrowRangeException("array::front -- empty array");
			}

			return mValue[N - 1];
		}

		bool validate() const { return true; }
		int  validate_iterator(const_pointer i) const {
			if(i >= mValue)
			{
				if(i < (mValue + N))
					return (eastl::isf_valid | eastl::isf_current | eastl::isf_can_dereference);

				if(i <= (mValue + N))
					return (eastl::isf_valid | eastl::isf_current);
			}

			return eastl::isf_none;
		}

		int  validate_iterator(const const_stack_only_iterator& i) const { return validate_iterator(toBase(i)); }
		int  validate_iterator(const const_heap_safe_iterator& i) const { return validate_iterator(toBase(i)); }

		iterator_safe make_safe(const soft_ptr_this_type& ptr, const iterator& position) { return makeSafeIt(ptr, toBase(position)); }
		const_iterator_safe make_safe(const soft_ptr_this_type& ptr, const const_iterator& position) const { return makeSafeIt(ptr, toBase(position)); }

	protected:
		[[noreturn]] static void ThrowRangeException(const char* msg) { throw std::out_of_range(msg); }
		[[noreturn]] static void ThrowInvalidArgumentException(const char* msg) { throw std::invalid_argument(msg); }


		pointer toBase(pointer it) const { return it; }
		const_pointer toBase(const_pointer it) const { return it; }

		pointer toBase(const stack_only_iterator& it) const { return it.toRaw(begin_unsafe()); }
		const_pointer toBase(const const_stack_only_iterator& it) const { return it.toRaw(begin_unsafe()); }

		pointer toBase(const heap_safe_iterator& it) const { return it.toRaw(begin_unsafe()); }
		const_pointer toBase(const const_heap_safe_iterator& it) const { return it.toRaw(begin_unsafe()); }

		constexpr iterator makeIt(pointer it) {
			if constexpr (use_base_iterator)
				return it;
			else
				return iterator::makePtr(data(), it, N);
		}
		constexpr const_iterator makeIt(const_pointer it) const {
			if constexpr (use_base_iterator)
				return it;
			else
				return const_iterator::makePtr(const_cast<T*>(data()), it, N);
		}

		iterator_safe makeSafeIt(const soft_ptr_this_type& ptr, pointer it) {
			if constexpr(is_safe == memory_safety::safe) {
				if(NODECPP_UNLIKELY(ptr.operator->() != this))
					ThrowInvalidArgumentException("array::make_safe -- wrong soft_ptr");
			}

			return iterator_safe::makePtr(ptr, it, N);
		}
		
		const_iterator_safe makeSafeIt(const soft_ptr_this_type& ptr, const_pointer it) const {
			if constexpr(is_safe == memory_safety::safe) {
				if(NODECPP_UNLIKELY(ptr.operator->() != this))
					ThrowInvalidArgumentException("array::make_safe -- wrong soft_ptr");
			}

			return const_iterator_safe::makePtr(ptr, it, N);
		}

	}; // class array


	template <typename T, memory_safety Safety>
	struct array<T, 0, Safety>
	{
	public:
		typedef array<T, 0, Safety>                           this_type;
		typedef T                                             value_type;
		typedef value_type&                                   reference;
		typedef const value_type&                             const_reference;
		typedef value_type*                                   pointer;
		typedef const value_type*                             const_pointer;
		typedef eastl_size_t                                  size_type;
		typedef ptrdiff_t                                     difference_type;

		static constexpr memory_safety is_safe = Safety;
		typedef soft_ptr<this_type, is_safe>                                       soft_ptr_this_type;

		typedef typename detail::array_of_iterator_stack<T>                        stack_only_iterator;
		typedef typename detail::const_array_of_iterator_stack<T>                  const_stack_only_iterator;
		typedef typename detail::array_of_iterator<T, false, soft_ptr_this_type, true>  heap_safe_iterator;
		typedef typename detail::array_of_iterator<T, true, soft_ptr_this_type, true>   const_heap_safe_iterator;

		static constexpr bool use_base_iterator = Safety == memory_safety::none;
		
		typedef std::conditional_t<use_base_iterator, pointer, stack_only_iterator>               iterator;
		typedef std::conditional_t<use_base_iterator, const_pointer, const_stack_only_iterator>   const_iterator;
		typedef eastl::reverse_iterator<iterator>                                  reverse_iterator;
		typedef eastl::reverse_iterator<const_iterator>                            const_reverse_iterator;

		typedef heap_safe_iterator                                         iterator_safe;
		typedef const_heap_safe_iterator                                   const_iterator_safe;
		typedef eastl::reverse_iterator<iterator_safe>                     reverse_iterator_safe;
		typedef eastl::reverse_iterator<const_iterator_safe>               const_reverse_iterator_safe;


	public:
		enum
		{
			count = 0
		};

		// Note that the member data is intentionally public.
		// This allows for aggregate initialization of the
		// object (e.g. array<int, 5> a = { 0, 3, 2, 4 }; )
//		value_type mValue[N];

	public:
		// We intentionally provide no constructor, destructor, or assignment operator.

		void fill(const value_type& value) {
			ThrowRangeException("array::fill -- out of range");
		}

		// Unlike the swap function for other containers, array::swap takes linear time,
		// may exit via an exception, and does not cause iterators to become associated with the other container.
		void swap(this_type& x) noexcept(eastl::is_nothrow_swappable<value_type>::value) { /* do nothing */ }

		constexpr pointer       begin_unsafe() noexcept { nullptr; }
		constexpr const_pointer begin_unsafe() const noexcept { return nullptr; }
		constexpr const_pointer cbegin_unsafe() const noexcept { return nullptr; }

		constexpr pointer       end_unsafe() noexcept { return nullptr; }
		constexpr const_pointer end_unsafe() const noexcept { return nullptr; }
		constexpr const_pointer cend_unsafe() const noexcept { return nullptr; }

		constexpr iterator       begin() noexcept { return iterator(); }
		constexpr const_iterator begin() const noexcept { return const_iterator(); }
		constexpr const_iterator cbegin() const noexcept { return const_iterator(); }

		constexpr iterator       end() noexcept { return iterator(); }
		constexpr const_iterator end() const noexcept { return const_iterator(); }
		constexpr const_iterator cend() const noexcept { return const_iterator(); }

		constexpr reverse_iterator       rbegin() noexcept { return reverse_iterator(end()); }
		constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
		constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

		constexpr reverse_iterator       rend() noexcept { return reverse_iterator(begin()); }
		constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
		constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

		constexpr iterator_safe       begin_safe(const soft_ptr_this_type& ptr) noexcept { return iterator_safe(); }
		constexpr const_iterator_safe begin_safe(const soft_ptr_this_type& ptr) const noexcept { return const_iterator_safe(); }
		constexpr const_iterator_safe cbegin_safe(const soft_ptr_this_type& ptr) const noexcept { return const_iterator_safe(); }

		constexpr iterator_safe       end_safe(const soft_ptr_this_type& ptr) noexcept { return iterator_safe(); }
		constexpr const_iterator_safe end_safe(const soft_ptr_this_type& ptr) const noexcept { return const_iterator_safe(); }
		constexpr const_iterator_safe cend_safe(const soft_ptr_this_type& ptr) const noexcept { return const_iterator_safe(); }

		constexpr reverse_iterator_safe       rbegin_safe(const soft_ptr_this_type& ptr) noexcept { return reverse_iterator_safe(end_safe()); }
		constexpr const_reverse_iterator_safe rbegin_safe(const soft_ptr_this_type& ptr) const noexcept { return const_reverse_iterator_safe(end_safe()); }
		constexpr const_reverse_iterator_safe crbegin_safe(const soft_ptr_this_type& ptr) const noexcept { return const_reverse_iterator_safe(end_safe()); }

		constexpr reverse_iterator_safe       rend_safe(const soft_ptr_this_type& ptr) noexcept { return reverse_iterator_safe(begin_safe()); }
		constexpr const_reverse_iterator_safe rend_safe(const soft_ptr_this_type& ptr) const noexcept { return const_reverse_iterator_safe(begin_safe()); }
		constexpr const_reverse_iterator_safe crend_safe(const soft_ptr_this_type& ptr) const noexcept { return const_reverse_iterator_safe(begin_safe()); }

		constexpr bool empty() const noexcept { return true; }
		constexpr size_type size() const noexcept { return 0; }
		constexpr size_type max_size() const noexcept { return 0; }

		constexpr T*       data() noexcept { ThrowRangeException("array::data -- out of range"); }
		constexpr const T* data() const noexcept { ThrowRangeException("array::data -- out of range"); }

		constexpr reference       operator[](size_type i) { ThrowRangeException("array::operator[] -- out of range"); }
		constexpr const_reference operator[](size_type i) const { ThrowRangeException("array::operator[] -- out of range"); }

		constexpr const_reference at(size_type i) const { ThrowRangeException("array::at -- out of range"); }

		constexpr reference       at(size_type i) { ThrowRangeException("array::at -- out of range"); }

		constexpr reference       front() { ThrowRangeException("array::front -- out of range"); }

		constexpr const_reference front() const { ThrowRangeException("array::front -- out of range"); }

		constexpr reference       back() { ThrowRangeException("array::back -- out of range"); }
		constexpr const_reference back() const { ThrowRangeException("array::back -- out of range"); }

		bool validate() const { return true; }
		int  validate_iterator(const_pointer i) const { return isf_none; }
		int  validate_iterator(const const_stack_only_iterator& i) const { return isf_none; }
		int  validate_iterator(const const_heap_safe_iterator& i) const { return isf_none; }

		iterator_safe make_safe(const soft_ptr_this_type& ptr, const iterator& position) { return iterator_safe(); }
		const_iterator_safe make_safe(const soft_ptr_this_type& ptr, const const_iterator& position) const { return const_iterator_safe(); }

	protected:
		[[noreturn]] static void ThrowRangeException(const char* msg) { throw std::out_of_range(msg); }

	}; // class array

	///////////////////////////////////////////////////////////////////////////
	// template deduction guides
	///////////////////////////////////////////////////////////////////////////
	#ifdef __cpp_deduction_guides
		template <class T, class... U> array(T, U...) -> array<T, 1 + sizeof...(U)>;
	#endif


	///////////////////////////////////////////////////////////////////////
	// global operators
	///////////////////////////////////////////////////////////////////////

	template <typename T, size_t N, memory_safety S>
	constexpr inline bool operator==(const array<T, N, S>& a, const array<T, N, S>& b)
	{
		return eastl::equal(&a.mValue[0], &a.mValue[N], &b.mValue[0]);
	}


	template <typename T, size_t N, memory_safety S>
	constexpr inline bool operator<(const array<T, N, S>& a, const array<T, N, S>& b)
	{
		return eastl::lexicographical_compare(&a.mValue[0], &a.mValue[N], &b.mValue[0], &b.mValue[N]);
	}


	template <typename T, size_t N, memory_safety S>
	constexpr inline bool operator!=(const array<T, N, S>& a, const array<T, N, S>& b)
	{
		return !eastl::equal(&a.mValue[0], &a.mValue[N], &b.mValue[0]);
	}


	template <typename T, size_t N, memory_safety S>
	constexpr inline bool operator>(const array<T, N, S>& a, const array<T, N, S>& b)
	{
		return eastl::lexicographical_compare(&b.mValue[0], &b.mValue[N], &a.mValue[0], &a.mValue[N]);
	}


	template <typename T, size_t N, memory_safety S>
	constexpr inline bool operator<=(const array<T, N, S>& a, const array<T, N, S>& b)
	{
		return !eastl::lexicographical_compare(&b.mValue[0], &b.mValue[N], &a.mValue[0], &a.mValue[N]);
	}


	template <typename T, size_t N, memory_safety S>
	constexpr inline bool operator>=(const array<T, N, S>& a, const array<T, N, S>& b)
	{
		return !eastl::lexicographical_compare(&a.mValue[0], &a.mValue[N], &b.mValue[0], &b.mValue[N]);
	}


	///////////////////////////////////////////////////////////////////////
	// to_array
	///////////////////////////////////////////////////////////////////////
	namespace detail
	{
		template<memory_safety Safety, class T, size_t N, size_t... I>
		constexpr auto to_array(T (&a)[N], eastl::index_sequence<I...>)
		{
			return safe_memory::array<eastl::remove_cv_t<T>, N, Safety>{{a[I]...}};
		}

		template<memory_safety Safety, class T, size_t N, size_t... I>
		constexpr auto to_array(T (&&a)[N], eastl::index_sequence<I...>)
		{
			return safe_memory::array<eastl::remove_cv_t<T>, N, Safety>{{std::move(a[I])...}};
		}
	}

	template<class T, size_t N, memory_safety Safety>
	constexpr safe_memory::array<eastl::remove_cv_t<T>, N> to_array(T (&a)[N])
	{
		static_assert(eastl::is_constructible_v<T, T&>, "element type T must be copy-initializable");
		static_assert(!eastl::is_array_v<T>, "passing multidimensional arrays to to_array is ill-formed");
		return detail::to_array<Safety>(a, eastl::make_index_sequence<N>{});
	}

	template<class T, size_t N, memory_safety Safety>
	constexpr safe_memory::array<eastl::remove_cv_t<T>, N> to_array(T (&&a)[N])
	{
		static_assert(eastl::is_move_constructible_v<T>, "element type T must be move-constructible");
		static_assert(!eastl::is_array_v<T>, "passing multidimensional arrays to to_array is ill-formed");
		return detail::to_array<Safety>(std::move(a), eastl::make_index_sequence<N>{});
	}


} // namespace safe_memory


#endif // Header include guard

