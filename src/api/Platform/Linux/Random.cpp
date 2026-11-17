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
 * \file Platform/Linux/Random.cpp
 * \author jwall64
 * \brief Implementation of Platform/Random.hpp for Linux.
 */

#include <Platform/PlatformDefines.hpp>

#ifdef NBL_PLATFORM_LINUX

#include <Platform/Random.hpp>

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

nbl::Result nbl::secureRandom(Span<u8> buffer) {

    if(buffer.length() == 0) return nbl::Result::INVALID_ARGUMENT;

    int fd = ::open("/dev/urandom", O_RDONLY);
    if(fd == -1) return nbl::Result::IO_ERROR;

    size_t totalRead = 0;
    ssize_t justRead = 0;

    while((justRead = ::read(fd, buffer.data() + totalRead, buffer.length() - totalRead)) > 0) {
        totalRead += justRead;
    }
    if(justRead == -1) return nbl::Result::IO_ERROR;

    return nbl::Result::SUCCESS;
}

#endif
