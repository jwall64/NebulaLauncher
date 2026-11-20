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
 * \file Platform/Windows/DynamicLibrary.cpp
 * \author jwall64
 * \brief Implementation of Platform/DynamicLibrary.hpp for Windows
 */

#include <Platform/PlatformDefines.hpp>
#ifdef NBL_PLATFORM_WINDOWS

#include <Platform/DynamicLibrary.hpp>

#include <Core/Result.hpp>
#include <Platform/Logging.hpp>
#include <Platform/Arena.hpp>
#include <Utility/StringBuilder.hpp>

#include "WindowsUtil.hpp"


nbl::StringView nbl::dynamicLibraryFileExt() {
    static StringView _ext = ".dll";
    return _ext;
}

nbl::StringView nbl::executableFileExt() {
    static StringView _ext = ".exe";
    return _ext;
}


nbl::DynamicLibrary::~DynamicLibrary() {
    if(_pHandle) {
        FreeLibrary(*reinterpret_cast<HMODULE*>(&_pHandle));
        _pHandle = nullptr;
    }
}

nbl::Result nbl::DynamicLibrary::_findSymbol(StringView name, void** outSym) {

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::StringView zName;
    NBL_CHECK(nbl::StringBuilder::nullTerminate(name, &zName, &frame));

    FARPROC sym = GetProcAddress(*reinterpret_cast<HMODULE*>(&_pHandle), name.data());

    if(sym == nullptr) {
        nbl::log(nbl::LogLevel::ERROR, "Failed to find symbol: ", name);
        return nbl::Result::IO_ERROR;
    }

    *outSym = reinterpret_cast<void*>(sym);
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::DynamicLibrary::load(StringView path, DynamicLibrary* outLibrary) {

    if(path.length() == 0) {
        return nbl::Result::INVALID_ARGUMENT;
    }


    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), nullptr, 0);
    if(sz == 0) return Result::INVALID_ARGUMENT;

    wchar_t* fileName;

    frame.allocate(&fileName, sz + 1);
    if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), fileName, sz)) {
        return Result::INVALID_ARGUMENT;
    }

    HMODULE handle = LoadLibraryW(fileName);
    if(handle == nullptr) {
        nbl::log(nbl::LogLevel::ERROR, "Failed to load library: ", path);
        return nbl::Result::IO_ERROR;
    }

    outLibrary->_pHandle = handle;
    return nbl::Result::SUCCESS;
}


#endif
