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
 * \file Platform/Linux/DynamicLibrary.cpp
 * \author jwall64
 * \brief Implementation of Platform/DynamicLibrary.hpp for Linux
 */

#include <Platform/PlatformDefines.hpp>
#ifdef NBL_PLATFORM_LINUX

#include <Platform/DynamicLibrary.hpp>

#include <Core/Result.hpp>
#include <Platform/Logging.hpp>
#include <Platform/Arena.hpp>
#include <Utility/StringBuilder.hpp>

#include <dlfcn.h>


nbl::StringView nbl::dynamicLibraryFileExt() {
    static StringView _ext = ".so";
    return _ext;
}

nbl::StringView nbl::executableFileExt() {
    return {};
}


nbl::DynamicLibrary::~DynamicLibrary() {
    if(_pHandle) {
        ::dlclose(_pHandle);
        _pHandle = nullptr;
    }
}

nbl::Result nbl::DynamicLibrary::_findSymbol(StringView name, void** outSym) {

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    if(!name.nullTerminated()) {
        NBL_CHECK(nbl::StringBuilder::nullTerminate(name, &name, &frame));
    }

    dlerror(); // Clear dlerror
    void* sym = dlsym(_pHandle, name.data());

    const char* err = dlerror();
    if(err) {
        nbl::log(nbl::LogLevel::ERROR, "Failed to find symbol: ", name, " (", err, ")");
        return nbl::Result::IO_ERROR;
    }

    *outSym = sym;
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::DynamicLibrary::load(StringView path, DynamicLibrary* outLibrary) {

    if(path.length() == 0) {
        return nbl::Result::INVALID_ARGUMENT;
    }

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    if(!path.nullTerminated()) {
        NBL_CHECK(nbl::StringBuilder::nullTerminate(path, &path, &frame));
    }

    void* handle = ::dlopen(zPath.data(), RTLD_LAZY);
    if(handle == nullptr) {
        nbl::log(nbl::LogLevel::ERROR, "Failed to load library: ", path, " (", dlerror(), ")");
        return nbl::Result::IO_ERROR;
    }

    outLibrary->_pHandle = handle;
    return nbl::Result::SUCCESS;
}


#endif
