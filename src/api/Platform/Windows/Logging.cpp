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
 * \file Platform/Windows/Logging.cpp
 * \author jwall64
 * \brief Implementation of Platform/Logging.hpp for Windows.
 */

#include <Platform/PlatformDefines.hpp>
#ifdef NBL_PLATFORM_WINDOWS

#include "WindowsUtil.hpp"

#include <Platform/Logging.hpp>


void nbl::stdout(StringView data) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if(h == nullptr || h == INVALID_HANDLE_VALUE) {
        return;
    }

    NBL_ASSERT(data.length() <= 0xFFFFFFFF);

    DWORD remaining = static_cast<DWORD>(data.length());
    DWORD written = 0;
    while(remaining > 0) {
        DWORD justWritten = 0;
        if(!WriteFile(h, data.data() + written, remaining, &justWritten, nullptr)) {
            return;
        }
        if(justWritten == 0) {
            return;
        }
        written += justWritten;
        remaining -= justWritten;
    }
}

void nbl::stderr(StringView data) {

    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    if(h == nullptr || h == INVALID_HANDLE_VALUE) {
        return;
    }

    NBL_ASSERT(data.length() <= 0xFFFFFFFF);

    DWORD remaining = static_cast<DWORD>(data.length());
    DWORD written = 0;
    while(remaining > 0) {
        DWORD justWritten = 0;
        if(!WriteFile(h, data.data() + written, remaining, &justWritten, nullptr)) {
            return;
        }
        if(justWritten == 0) {
            return;
        }
        written += justWritten;
        remaining -= justWritten;
    }
}

nbl::Writer nbl::stdoutWriter() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if(h == nullptr || h == INVALID_HANDLE_VALUE) {
        return NullStream();
    }
    return nbl::win::handleWriter(h); 
}

nbl::Writer nbl::stderrWriter() {
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    if(h == nullptr || h == INVALID_HANDLE_VALUE) {
        return NullStream();
    }
    return nbl::win::handleWriter(h); 
}

#endif  // NBL_PLATFORM_LINUX
