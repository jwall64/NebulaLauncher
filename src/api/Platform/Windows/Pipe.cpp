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
  * \file Platform/Windows/Pipe.cpp
  * \author jwall64
  * \brief Implementation of Platform/Pipe.hpp for Windows.
  */

#include <Platform/PlatformDefines.hpp>
#ifdef NBL_PLATFORM_WINDOWS

#include "WindowsUtil.hpp"

#include <Core/Result.hpp>
#include <Platform/Pipe.hpp>


void nbl::Pipe::close() {
    if(_mReadEndHandle) {
        CloseHandle(_mReadEndHandle);
    }
    if(_mWriteEndHandle) {
        CloseHandle(_mWriteEndHandle);
    }
}

void nbl::Pipe::closeWriter() {
    if(_mWriteEndHandle) {
        CloseHandle(_mWriteEndHandle);
    }
}

nbl::Reader nbl::Pipe::reader() {
    return Reader{
        .impl = _mReadEndHandle,
        .pfnRead = win::readHandle
    };
}

nbl::Writer nbl::Pipe::writer() {
    return Writer{
        .impl = _mWriteEndHandle,
        .pfnWrite = win::writeHandle
    };
}

nbl::Result nbl::Pipe::create(nbl::Pipe* outPipe) {

    HANDLE inHandle;
    HANDLE outHandle;

    if(!CreatePipe(&outPipe->_mReadEndHandle, &outPipe->_mWriteEndHandle, nullptr, 0)) {
        return nbl::Result::IO_ERROR;
    }

    return nbl::Result::SUCCESS;
}


#endif
