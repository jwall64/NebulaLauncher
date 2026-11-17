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
 * \file Platform/Windows/Time.cpp
 * \author jwall64
 * \brief Implementation of Platform/Time.hpp for Windows.
 */

#include <Platform/PlatformDefines.hpp>

#ifdef NBL_PLATFORM_WINDOWS

#include <Windows.h>

#include <Platform/Time.hpp>

u64 nbl::getCurrentTime() {

    FILETIME ft{};
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER uli{};
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    constexpr static u64 _diff = 116444736000000000ULL;
    return (uli.QuadPart - _diff) / 10000ULL;
}

nbl::Result nbl::utcFromComponents(u32 year, u8 month, u8 day, u8 hour,
                                   u8 minute, u8 second, u64* out) {
    if (out == nullptr || year > 9999 || year < 1970 || month > 12 ||
        month < 1 || day > 31 || hour > 23 || minute > 59 || second > 59)
        return nbl::Result::INVALID_ARGUMENT;


    SYSTEMTIME st = {};
    st.wYear = static_cast<WORD>(year);
    st.wMonth = static_cast<WORD>(month);
    st.wDay = static_cast<WORD>(day);
    st.wHour = static_cast<WORD>(hour);
    st.wMinute = static_cast<WORD>(minute);
    st.wSecond = static_cast<WORD>(second);
}

#endif
