/*
 * Copyright 2026 jwall64
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/**
 * \file Platform/Arena.cpp
 * \author jwall64
 * \brief Implementation of Platform/Arena.hpp
 */

#include <Core/Math.hpp>
#include <Platform/Arena.hpp>
#include <Platform/Allocator.hpp>
#include <Platform/Logging.hpp>
#include <Utility/StringBuilder.hpp>

#include "Memory.hpp"


namespace {

thread_local nbl::Arena _sThreadArena;

}

nbl::Result nbl::Arena::push(usize amount, void** outPtr) noexcept {
    
    if(amount == 0) return nbl::Result::INVALID_ARGUMENT;

    usize available = _mCapacity - _mIndex;
    if(amount > available) {
        NBL_ERROR("Out of memory! ", amount, " > ", available);
        return nbl::Result::OUT_OF_MEMORY;
    }

    void* out = end();
    usize remaining = _mCommitted - _mIndex;
    if(remaining < amount) {
        usize needed = amount - remaining;
        usize uncommitted = _mCapacity - _mCommitted;
        usize committed = nbl::min(uncommitted, nbl::alignUp(needed, nbl::pageSize()));
        if(committed < needed) {
            return nbl::Result::OUT_OF_MEMORY;
        }

        u8* allocatedEnd = static_cast<u8*>(_pHead) + _mCommitted;
        NBL_CHECK(nbl::commitMemory(allocatedEnd, &committed));
        _mCommitted += committed;
    }

    _mIndex += amount;
    if(outPtr) *outPtr = out;
    memset(out, 0, amount);

    return nbl::Result::SUCCESS;
}

nbl::Result nbl::Arena::pop(usize amount) noexcept {

    if(amount == 0) return nbl::Result::SUCCESS;
    if(amount > _mIndex) return nbl::Result::INVALID_ARGUMENT;

    _mIndex -= amount;
    usize remaining = _mCommitted - _mIndex;
    usize pageSize = nbl::pageSize();

    usize remainingPages = remaining / pageSize;
    if(remainingPages > 1) {
        usize uncommitted = (remainingPages - 1) * pageSize;
        u8* allocatedEnd = static_cast<u8*>(_pHead) + _mCommitted;
        NBL_CHECK(nbl::uncommitMemory(allocatedEnd - uncommitted, &uncommitted));

        _mCommitted -= uncommitted;
    }

    return nbl::Result::SUCCESS;
}

nbl::Result nbl::Arena::create(usize amount, Arena* outArena) noexcept {

    usize mapped = amount;
    void* data = nullptr;
    NBL_CHECK(nbl::mapMemory(&mapped, &data));

    outArena->_pHead = data;
    outArena->_mCapacity = mapped;
    outArena->_mCommitted = 0;
    outArena->_mIndex = 0;

    return nbl::Result::SUCCESS;
}

nbl::Arena* nbl::Arena::threadLocal() noexcept {

    if(_sThreadArena._mCapacity == 0) {
        NBL_ASSERT(nbl::Arena::create(1024 * 1024 * 32, &_sThreadArena) == nbl::Result::SUCCESS);
    }

    return &_sThreadArena;
}

bool nbl::Arena::isThreadLocalWriter(Writer writer) noexcept {

    const nbl::LinearAllocator* alloc = nullptr;
    if(nbl::StringBuilder::isStringBuilderWriter(writer)) {
        alloc = static_cast<nbl::StringBuilder*>(writer.impl)->allocator();
    }
    else if(nbl::LinearAllocator::isLinearWriter(writer)) {
        alloc = static_cast<nbl::LinearAllocator*>(writer.impl);
    }
    else {
        return false;
    }

    if(const Arena* arena = dynamic_cast<const nbl::Arena*>(alloc)) {
        return arena == threadLocal();
    }
    if(const AllocationFrame* frame = dynamic_cast<const nbl::AllocationFrame*>(alloc)) {
        return frame->_pParent == threadLocal();
    }

    return false;
}
