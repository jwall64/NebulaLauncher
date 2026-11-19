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
 * \file Platform/Linux/LinuxUtil.cpp
 * \author jwall64
 * \brief Implementation of Linux-specific utility functions.
 */

#include <Platform/PlatformDefines.hpp>
#ifdef NBL_PLATFORM_LINUX

#include "LinuxUtil.hpp"
#include <Core/Result.hpp>

#include <errno.h>
#include <unistd.h>

nbl::Result nbl::linux::writeFd(void* impl, const void* data, usize length, usize* bytesWritten) {

    int fd = static_cast<int>(reinterpret_cast<intptr_t>(impl));

    ssize_t result = ::write(fd, data, length);
    if(result < 0) {
        return nbl::Result::IO_ERROR;
    }
    if(bytesWritten) {
        *bytesWritten = static_cast<usize>(result);
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::linux::readFd(void* impl, void* data, usize length, usize* bytesRead) {

    int fd = static_cast<int>(reinterpret_cast<intptr_t>(impl));

    ssize_t result = ::read(fd, data, length);
    if(result < 0) {
        if(errno == EINTR) {
            return nbl::Result::SUCCESS;
        }
        return nbl::Result::IO_ERROR;
    }
    if(result == 0) {
        return nbl::Result::END_OF_STREAM;
    }
    if(bytesRead) {
        *bytesRead = static_cast<usize>(result);
    }
    return nbl::Result::SUCCESS;
}

#endif
