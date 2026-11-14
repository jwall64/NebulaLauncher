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
 * \file Platform/Allocator.hpp
 * \author jwall64
 * \brief Linear allocator interface 
 */

#pragma once

#include <Core/Types.hpp>
#include <Core/Result.hpp>
#include <Core/Stream.hpp>

namespace nbl {

class LinearAllocator {
public:
    
    inline virtual ~LinearAllocator() {};

    virtual void* begin() const noexcept = 0;
    virtual void* end() const noexcept = 0;

    virtual Result push(usize amount, void** outPtr) noexcept = 0;

    Result write(const void* data, usize length) noexcept;
    
    template<typename T>
    inline Result allocate(T** location, usize amount = 1) noexcept {
        return push(amount * sizeof(T), reinterpret_cast<void**>(location));
    }
    
    template<typename T>
    inline Result allocate(Span<T>* span) noexcept {
        if(span == nullptr || span->length() == 0 || span->data() != nullptr) return Result::INVALID_ARGUMENT;

        T* data;
        NBL_CHECK(allocate(&data, span->length()));
        *span = Span<T>(data, span->length());
        return Result::SUCCESS;
    }

    virtual Result pop(usize amount) noexcept = 0;
    
    inline Result popTo(usize index) {
        return pop(this->index() - index);
    }

    inline Result popToPtr(void* ptr) {
        if(ptr < begin() || ptr >= end()) return nbl::Result::INVALID_ARGUMENT; 
        usize amount = static_cast<u8*>(end()) - static_cast<u8*>(ptr);
        return pop(amount);
    }

    virtual usize index() const noexcept = 0;
    
    inline Span<u8> span() noexcept {
        return Span<u8>(static_cast<u8*>(begin()), index());
    }
    inline ConstSpan<u8> constSpan() const noexcept {
        return ConstSpan<u8>(static_cast<const u8*>(begin()), index());
    }

    inline Writer writer() noexcept {
        return Writer {
            .impl = this,
            .pfnWrite = _write
        };
    }

    Result readAll(Reader reader) noexcept;


    inline bool is(Writer writer) {
        return writer.impl == this && writer.pfnWrite == _write;
    }
    static inline bool isLinearWriter(Writer writer) {
        return writer.pfnWrite == _write;
    }


private:

    static inline Result _write(void* impl, const void* data, usize dataLength, usize* written) noexcept {
        LinearAllocator* self = static_cast<LinearAllocator*>(impl);
        NBL_CHECK(self->write(data, dataLength));
        if(written) *written = dataLength;
        return nbl::Result::SUCCESS;
    }

};



class AllocationFrame : public LinearAllocator {
public:

    AllocationFrame(LinearAllocator* parent) :
        _pParent(parent),
        _mIndex(parent->index()),
        _mStart(_mIndex)
    { }

    ~AllocationFrame() {
        if(_mIndex > _mStart) {
            _pParent->popTo(_mStart);
        }
    }

    void* begin() const noexcept override {
        return static_cast<u8*>(_pParent->begin()) + _mStart;
    }

    void* end() const noexcept override {
        return static_cast<u8*>(_pParent->end());
    }

    Result push(usize amount, void** outPtr) noexcept override {
        NBL_ASSERT(_mIndex == _pParent->index());
        NBL_CHECK(_pParent->push(amount, outPtr));
        _mIndex += amount;
        return nbl::Result::SUCCESS;
    }

    Result pop(usize amount) noexcept override {
        NBL_ASSERT(_mIndex == _pParent->index());
        NBL_CHECK(_pParent->pop(amount));
        _mIndex -= amount;
        return nbl::Result::SUCCESS;
    }

    inline usize index() const noexcept override {
        return _mIndex - _mStart;
    }

    void release() noexcept {
        NBL_ASSERT(_mIndex == _pParent->index());
        _mStart = _mIndex;
    }

private:

    LinearAllocator* _pParent;
    usize _mIndex;
    usize _mStart;

    friend class Arena;

};



class InPlaceAllocator : public LinearAllocator {
public:

    InPlaceAllocator(void* buffer, usize capacity) : 
        _pData(buffer),
        _mCapacity(capacity),
        _mIndex(0)
    { }

    inline void* begin() const noexcept override { return _pData; }
    inline void* end() const noexcept override { return static_cast<u8*>(_pData) + _mIndex; }

    Result push(usize amount, void** outPtr) noexcept override;
    Result pop(usize amount) noexcept override;

    inline usize index() const noexcept override {
        return _mIndex;
    }

private:
    void* _pData;
    usize _mCapacity;
    usize _mIndex;
};

}
