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

#include "safe_ptr_common.h"
#include "safe_ptr_impl.h"


#ifdef NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
thread_local size_t safememory::detail::onStackSafePtrCreationCount = 0; 
thread_local size_t safememory::detail::onStackSafePtrDestructionCount = 0;
#endif // NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING

#ifdef NODECPP_DEBUG_COUNT_SOFT_PTR_ENABLED
thread_local std::size_t safememory::detail::CountSoftPtrZeroOffsetDtor = 0;
thread_local std::size_t safememory::detail::CountSoftPtrBaseDtor = 0;
#endif // NODECPP_DEBUG_COUNT_SOFT_PTR_ENABLED

thread_local void* safememory::detail::thg_stackPtrForMakeOwningCall = NODECPP_SECOND_NULLPTR;

namespace safememory::detail {
#if defined NODECPP_USE_NEW_DELETE_ALLOC
thread_local void** zombieList_ = nullptr;
#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
thread_local std::map<uint8_t*, size_t, std::greater<uint8_t*>> zombieMap;
thread_local bool doZombieEarlyDetection_ = true;
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
#endif // NODECPP_USE_NEW_DELETE_ALLOC
} // namespace safememory::detail

#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
namespace safememory::detail::impl {
	NODECPP_NOINLINE void dbgThrowNullPtrAccess( const DbgCreationAndDestructionInfo& info )
	{
		nodecpp::error::string_ref extra( info.toStr().c_str() );
		throw nodecpp::error::nodecpp_error(nodecpp::error::NODECPP_EXCEPTION::null_ptr_access, std::move( extra ) );
	}
} // namespace safememory::detail::impl
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
