/* -------------------------------------------------------------------------------
* Copyright (c) 2018, OLogN Technologies AG
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

#ifndef SAFE_PTR_COMMON_H
#define SAFE_PTR_COMMON_H

#include <foundation.h>
#include <nodecpp_assert.h>
#include <log.h>
#include <memory>
#include <stdint.h>
#include <safe_memory/checker_attributes.h>
#include <allocator_template.h>

namespace nodecpp::safememory
{
	constexpr uint64_t module_id = 2;
} // namespace nodecpp::safememory

namespace nodecpp
{
	constexpr const char* safememory_module_id = "safememory";
}

#if defined NODECPP_MSVC
#define NODISCARD _NODISCARD
#elif (defined NODECPP_GCC) || (defined NODECPP_CLANG)
#define NODISCARD [[nodiscard]]
#else
#define NODISCARD
#endif


#if defined NODECPP_X64 && !defined NODECPP_NOT_USING_IIBMALLOC
#define NODECPP_USE_IIBMALLOC
#else
#define NODECPP_USE_NEW_DELETE_ALLOC
#endif


namespace nodecpp::safememory {
enum class StdAllocEnforcer { enforce };
} // namespace nodecpp::safememory


#ifdef NODECPP_USE_IIBMALLOC

#include <iibmalloc.h>
using namespace nodecpp::iibmalloc;
namespace nodecpp::safememory
{
NODECPP_FORCEINLINE void* allocate( size_t sz, size_t alignment ) { return g_AllocManager.allocate( sz ); } // TODO: proper implementation for alignment
NODECPP_FORCEINLINE void* allocate( size_t sz ) { return g_AllocManager.allocate( sz ); }
template<size_t alignment>
NODECPP_FORCEINLINE void* allocateAligned( size_t sz ) { return g_AllocManager.allocateAligned<alignment>( sz ); }
NODECPP_FORCEINLINE void deallocate( void* ptr ) { g_AllocManager.deallocate( ptr ); }
NODECPP_FORCEINLINE void* zombieAllocate( size_t sz ) { return g_AllocManager.zombieableAllocate( sz ); }
template<size_t sz, size_t alignment>
NODECPP_FORCEINLINE void* zombieAllocateAligned() { return g_AllocManager.zombieableAllocateAligned<sz, alignment>(); }
NODECPP_FORCEINLINE void zombieDeallocate( void* ptr ) { g_AllocManager.zombieableDeallocate( ptr ); }
NODECPP_FORCEINLINE bool isZombieablePointerInBlock(void* allocatedPtr, void* ptr ) { return g_AllocManager.isZombieablePointerInBlock( allocatedPtr, ptr ); }
#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
NODECPP_FORCEINLINE bool isPointerNotZombie(void* ptr ) { return g_AllocManager.isPointerNotZombie( ptr ); }
inline bool doZombieEarlyDetection( bool doIt = true ) { return g_AllocManager.doZombieEarlyDetection( doIt ); }
#else
constexpr bool isPointerNotZombie(void* ptr ) { return true; }
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
NODECPP_FORCEINLINE constexpr size_t getPrefixByteCount() { static_assert(guaranteed_prefix_size <= 3*sizeof(void*)); return guaranteed_prefix_size; }
inline void killAllZombies() { g_AllocManager.killAllZombies(); }
NODECPP_FORCEINLINE size_t allocatorAlignmentSize() { return ALIGNMENT; }
inline bool interceptNewDeleteOperators( bool doIntercept ) { return interceptNewDeleteOperators( doIntercept ? &g_AllocManager : nullptr ) != nullptr;}

struct IIBRawAllocator
{
	static constexpr size_t guaranteed_alignment = NODECPP_GUARANTEED_IIBMALLOC_ALIGNMENT;
	template<size_t alignment = 0> 
	static NODECPP_FORCEINLINE void* allocate( size_t allocSize ) { return ::nodecpp::safememory::allocateAligned<alignment>( allocSize ); }
//	static NODECPP_FORCEINLINE void* allocate( size_t allocSize, size_t allignment ) { return ::nodecpp::safememory::allocate( allocSize ); }
	template<size_t alignment = 0> 
	static NODECPP_FORCEINLINE void deallocate( void* ptr ) { return ::nodecpp::safememory::deallocate( ptr ); }
};

template<class _Ty>
using iiballocator = selective_allocator<IIBRawAllocator, _Ty>;
template< class T1, class T2 >
bool operator==( const iiballocator<T1>& lhs, const iiballocator<T2>& rhs ) noexcept { return true; }
template< class T1, class T2 >
bool operator!=( const iiballocator<T1>& lhs, const iiballocator<T2>& rhs ) noexcept { return false; }

} // namespace nodecpp::safememory

#elif defined NODECPP_USE_NEW_DELETE_ALLOC

#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
#include <map>
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
namespace nodecpp::safememory
{
template<class T>
using iiballocator =  std::allocator<T>;
template<class T>
using stdallocator =  std::allocator<T>;

// NOTE: while being non-optimal, following calls provide safety guarantees and can be used at least for debug purposes
extern thread_local void** zombieList_; // must be set to zero at the beginning of a thread function
#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
extern thread_local std::map<uint8_t*, size_t, std::greater<uint8_t*>> zombieMap;
extern thread_local bool doZombieEarlyDetection_;
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION

inline void killAllZombies()
{
	while ( zombieList_ != nullptr )
	{
		void** next = reinterpret_cast<void**>( *zombieList_ );
		delete [] zombieList_;
		zombieList_ = next;
	}
#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, doZombieEarlyDetection_ || ( !doZombieEarlyDetection_ && zombieMap.empty() ) );
	zombieMap.clear();
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
}
NODECPP_FORCEINLINE void* allocate( size_t sz, size_t alignment ) { void* ret = ::operator new [] (sz, std::align_val_t(alignment)); return ret; } // TODO: proper implementation for alignment
NODECPP_FORCEINLINE void* allocate( size_t sz ) { void* ret = ::operator new [] (sz); return ret; }
template<size_t alignment>
NODECPP_FORCEINLINE void* allocateAligned( size_t sz ) { return allocate( sz, alignment ); }
NODECPP_FORCEINLINE void deallocate( void* ptr, size_t alignment ) { ::operator delete [] (ptr, std::align_val_t(alignment)); }
NODECPP_FORCEINLINE void deallocate( void* ptr ) { ::operator delete [] (ptr); }
NODECPP_FORCEINLINE void* zombieAllocate( size_t sz ) { 
	uint8_t* ret = new uint8_t[ 4 * sizeof(uint64_t) + sz ]; 
	*reinterpret_cast<uint64_t*>(ret) = sz; 
	return ret + 4 * sizeof(uint64_t);
}
NODECPP_FORCEINLINE void* zombieAllocate( size_t sz, size_t alignment ) { 
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, alignment <= 4 * sizeof(uint64_t), "alignment = {}", alignment );
	return zombieAllocate( sz );
}
template<size_t sz, size_t alignment>
NODECPP_FORCEINLINE void* zombieAllocateAligned() {
	return zombieAllocate(sz, alignment);
}
NODECPP_FORCEINLINE void zombieDeallocate( void* ptr ) { 
	void** blockStart = reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(ptr) - 4 * sizeof(uint64_t)); 
#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
	size_t allocSize = *reinterpret_cast<uint64_t*>(blockStart);
	zombieMap.insert( std::make_pair( reinterpret_cast<uint8_t*>(blockStart), 4 * sizeof(uint64_t) + allocSize ) );
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
	*blockStart = zombieList_; 
	zombieList_ = blockStart;
}
NODECPP_FORCEINLINE bool isZombieablePointerInBlock(void* allocatedPtr, void* ptr ) { return ptr >= allocatedPtr && reinterpret_cast<uint8_t*>(allocatedPtr) + *(reinterpret_cast<uint64_t*>(allocatedPtr) - 2) > reinterpret_cast<uint8_t*>(ptr); }
#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
NODECPP_FORCEINLINE bool isPointerNotZombie(void* ptr ) { 
	auto iter = zombieMap.lower_bound( reinterpret_cast<uint8_t*>( ptr ) );
	if ( iter != zombieMap.end() )
		return reinterpret_cast<uint8_t*>( ptr ) >= iter->first + iter->second;
	else
		return true;
}
inline bool doZombieEarlyDetection( bool doIt = true )
{
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, zombieMap.empty(), "to (re)set doZombieEarlyDetection() zombieMap must be empty" );
	bool ret = doZombieEarlyDetection_;
	doZombieEarlyDetection_ = doIt;
	return ret;
}
#else
constexpr bool isPointerNotZombie(void* ptr ) { return true; }
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
NODECPP_FORCEINLINE constexpr size_t getPrefixByteCount() { return sizeof(uint64_t); }
NODECPP_FORCEINLINE size_t allocatorAlignmentSize() { return sizeof(void*); }
} //namespace nodecpp::safememory
inline bool interceptNewDeleteOperators( bool doIntercept ) { return true;}

#else
#error at least some specific allocation functionality must be selected
#endif


namespace nodecpp::safememory
{
#ifdef NODECPP_GCC
extern void forcePreviousChangesToThisInDtor( void* p );
#else
#define forcePreviousChangesToThisInDtor(x)
#endif

template<class T>
void destruct( T* t )
{
	if constexpr ( std::is_polymorphic<T>::value )
	{
		auto vpt = nodecpp::platform::backup_vmt_pointer(t);
		t->~T();
		nodecpp::platform::restore_vmt_pointer( t, vpt);
	}
	else
		t->~T();
}

struct make_owning_t {};
template<class T> class owning_ptr_impl; // forward declaration
template<class T> class soft_ptr_base_impl; // forward declaration
template<class T> class soft_ptr_impl; // forward declaration
template<class T> class soft_ptr_base_no_checks; // forward declaration
template<class T> class soft_ptr_no_checks; // forward declaration


enum class memory_safety { none, safe };

template<class T>
struct safeness_declarator {
#ifdef NODECPP_MEMORY_SAFETY
#if NODECPP_MEMORY_SAFETY == 1
	static constexpr memory_safety is_safe = memory_safety::safe;
#elif NODECPP_MEMORY_SAFETY == 0
	static constexpr memory_safety is_safe = memory_safety::none;
#else
#error Unexpected value of NODECPP_MEMORY_SAFETY (expected values are 1 or 0)
#endif // NODECPP_MEMORY_SAFETY defined
#else
	static constexpr memory_safety is_safe = memory_safety::safe; // by default
#endif
};

#ifdef NODECPP_MEMORY_SAFETY_EXCLUSIONS
#include NODECPP_MEMORY_SAFETY_EXCLUSIONS
#endif

/* Sample of user-defined exclusion:
template<> struct nodecpp::safememory::safeness_declarator<double> { static constexpr memory_safety is_safe = memory_safety::none; };
*/

//#define NODECPP_DEBUG_COUNT_SOFT_PTR_ENABLED
#ifdef NODECPP_DEBUG_COUNT_SOFT_PTR_ENABLED
extern thread_local std::size_t CountSoftPtrZeroOffsetDtor;
extern thread_local std::size_t CountSoftPtrBaseDtor;
#define NODECPP_DEBUG_COUNT_SOFT_PTR_BASE_DTOR() { ::nodecpp::safememory::CountSoftPtrBaseDtor++; }
#define NODECPP_DEBUG_COUNT_SOFT_PTR_ZERO_OFFSET_DTOR() { ::nodecpp::safememory::CountSoftPtrZeroOffsetDtor++; }
#else
#define NODECPP_DEBUG_COUNT_SOFT_PTR_BASE_DTOR() { }
#define NODECPP_DEBUG_COUNT_SOFT_PTR_ZERO_OFFSET_DTOR() { }
#endif


} // namespace nodecpp::safememory

#endif // SAFE_PTR_COMMON_H
