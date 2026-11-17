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
  * \file Platform/Windows/Memory.cpp
  * \author jwall64
  * \brief Implementation of Platform/Memory.hpp for Windows.
  */

#include <Platform/PlatformDefines.hpp>

#ifdef NBL_PLATFORM_WINDOWS

#include <Core/Math.hpp>
#include <Core/Result.hpp>
#include <Platform/Logging.hpp>

#include "../Memory.hpp"

#include "WindowsUtil.hpp"


usize nbl::pageSize() noexcept {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwPageSize;
}

nbl::Result nbl::mapMemory(usize* size, void** outPtr) noexcept {
    if(outPtr == nullptr || size == nullptr || *size == 0) return nbl::Result::INVALID_ARGUMENT;

    *size = nbl::alignUp(*size, getpagesize());
    void* out = VirtualAlloc(nullptr, *size, MEM_RESERVE, PAGE_NOACCESS);
    if(out == nullptr) return nbl::Result::OUT_OF_MEMORY;

    *outPtr = out;
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::unmapMemory(void* ptr, usize* size) noexcept {
    if(ptr == nullptr || size == nullptr || *size == 0) return nbl::Result::INVALID_ARGUMENT;

    *size = nbl::alignUp(*size, getpagesize());
    if(!VirtualFree(ptr, *size, MEM_RELEASE)) {
        return nbl::Result::INVALID_ARGUMENT;
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::commitMemory(void* ptr, usize* size) noexcept {
    NBL_ASSERT(ptr != nullptr);
    NBL_ASSERT(size != nullptr);
    NBL_ASSERT(*size != 0);

    *size = nbl::alignUp(*size, getpagesize());
    void* newPtr = VirtualAlloc(ptr, *size, MEM_COMMIT, PAGE_READWRITE);

    if(newPtr == nullptr) {
        nbl::log(nbl::LogLevel::DEBUG, "Failed to commit memory! ", errno);
        return nbl::Result::INVALID_ARGUMENT;
    }
    NBL_ASSERT(newPtr == ptr);

    return nbl::Result::SUCCESS;
}

nbl::Result nbl::uncommitMemory(void* ptr, usize* size) noexcept {
    if(ptr == nullptr || size == nullptr || *size == 0) return nbl::Result::INVALID_ARGUMENT;

    *size = nbl::alignUp(*size, getpagesize());
    if(!VirtualFree(ptr, *size, MEM_DECOMMIT)) {
        return nbl::Result::INVALID_ARGUMENT;
    }
    return nbl::Result::SUCCESS;
}


#endif // NBL_PLATFORM_LINUX
