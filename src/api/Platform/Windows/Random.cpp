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
  * \file Platform/Windows/Random.cpp
  * \author jwall64
  * \brief Implementation of Platform/Random.hpp for Windows.
  */

#include <Platform/PlatformDefines.hpp>

#ifdef NBL_PLATFORM_WINDOWS

#include <Platform/Random.hpp>

#include "WindowsUtil.hpp"
#include <bcrypt.h>

nbl::Result nbl::secureRandom(Span<u8> buffer) {

    if(buffer.length() == 0 || buffer.length() > 0xFFFFFFFF) return nbl::Result::INVALID_ARGUMENT;

    NTSTATUS status = BCryptGenRandom(
        NULL,
        buffer.data(),
        static_cast<ULONG>(buffer.length()),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );

    return nbl::Result::SUCCESS;
}

#endif
