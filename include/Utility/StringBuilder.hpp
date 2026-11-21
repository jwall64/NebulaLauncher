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
 * \file Utility/StringBuilder.hpp
 * \author jwall64
 * \brief Utility for dynamically creating strings using a linear allocator
 */


#pragma once

#include <Core/Result.hpp>
#include <Core/String.hpp>
#include <Core/Stream.hpp>
#include <Platform/Allocator.hpp>

#include <utility>

namespace nbl {

/**
 * \brief An implementation of a string builder that allows for efficient construction of strings using a memory arena for temporary storage.  
 */
class StringBuilder {
public:


    StringBuilder(LinearAllocator* allocator) noexcept :
        _mLastResult(Result::SUCCESS),
        _mLength(0),
        _mBegin(0),
        _pAllocator(allocator)
    { }


    template<typename T>
    StringBuilder& append(T&& value) noexcept {
        appendChecked(std::forward<T>(value));
        return *this;
    }

    StringBuilder& replace(char oldValue, StringView newValue, usize startIndex = 0) noexcept {
        replaceChecked(oldValue, newValue);
        return *this;
    }

    Result replaceChecked(char oldValue, StringView newValue, usize startIndex = 0) noexcept;


    template<typename T>
    Result appendChecked(T&& value) noexcept {
        if(!_mLastResult.success()) {
            return _mLastResult;
        }
        if(_mLength == 0) {
            _mBegin = _pAllocator->index();
        }
        _mLastResult = write(std::forward<T>(value), _pAllocator->writer());
        _mLength = _pAllocator->index() - _mBegin;
        return _mLastResult;
    }

    Result build(StringView* outString) noexcept {
        if(!_mLastResult.success()) {
            return _mLastResult;
        }

        char* ptr = static_cast<char*>(_pAllocator->begin()) + _mBegin;
        *outString = StringView(ptr, _mLength);

        return Result::SUCCESS;
    }


    Result buildNullTerminated(StringView* outString) noexcept {

        if(!_mLastResult.success()) {
            return _mLastResult;
        }
        NBL_CHECK(_pAllocator->write("\0", 1));

        char* ptr = static_cast<char*>(_pAllocator->begin()) + _mBegin;
        *outString = StringView(ptr, _mLength);

        return Result::SUCCESS;
    }

    Result retain(usize index, usize length) noexcept {

        if(index + length > _mLength) return nbl::Result::INVALID_ARGUMENT;

        char* ptr = static_cast<char*>(_pAllocator->begin()) + _mBegin;
        if(length > 0) {
            memmove(ptr, ptr + index, length);
        }

        NBL_CHECK(_pAllocator->popToPtr(ptr + length));
        _mLength = length;

        return nbl::Result::SUCCESS;
    }


    Result retainLast(usize length) noexcept {
        if(length > _mLength) return nbl::Result::INVALID_ARGUMENT;
        return retain(_mLength - length, length);
    }


    operator Writer() noexcept {
        return writer();
    }

    Writer writer() noexcept {
        return Writer {
            .impl = this,
            .pfnWrite = _write
        };       
    }

    inline const LinearAllocator* allocator() const noexcept {
        return _pAllocator;
    }

    inline usize length() const noexcept { return _mLength; }

    static inline bool isStringBuilderWriter(Writer writer) noexcept {
        return writer.pfnWrite == _write;
    }

    static inline Result nullTerminate(StringView string, StringView* outString, AllocationFrame* arena) noexcept {
        return StringBuilder(arena).append(string).append('\0').build(outString);
    }

private:
    Result _mLastResult;
    usize _mLength;
    usize _mBegin;
    LinearAllocator* _pAllocator;

    inline static Result _write(void* impl, const void* data, usize length, usize* bytesWritten) noexcept {
        StringBuilder* builder = static_cast<StringBuilder*>(impl);
        NBL_CHECK(builder->appendChecked(StringView(static_cast<const char*>(data), length)));
        if(bytesWritten) *bytesWritten = length;
        return Result::SUCCESS;
    }

};

}
