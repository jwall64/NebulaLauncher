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
 * \file Platform/Linux/File.cpp
 * \author jwall64
 * \brief Implementation of Platform/File.hpp for Linux.
 */


#include <Platform/PlatformDefines.hpp>
#ifdef NBL_PLATFORM_LINUX

#include <Core/Result.hpp>
#include <Core/Math.hpp>
#include <Core/Assert.hpp>
#include <Platform/File.hpp>
#include <Platform/Socket.hpp>
#include <Platform/Arena.hpp>
#include <Platform/Logging.hpp>
#include <Utility/StringBuilder.hpp>

#include "LinuxUtil.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/resource.h>

namespace {

char gCwd[PATH_MAX];
char gDataDir[PATH_MAX];
char gTempDir[PATH_MAX];


int gGetFd(void* handle) {
    return static_cast<int>(reinterpret_cast<intptr_t>(handle));
}

}


nbl::FileChannel::FileChannel(FileChannel&& other) :
    _mHandle(other._mHandle) {
    other._mHandle = nullptr;
}

nbl::FileChannel& nbl::FileChannel::operator=(FileChannel&& other) {
    if(&other == this) return *this;
    _mHandle = other._mHandle;
    other._mHandle = nullptr;
    return *this;
}


nbl::Writer nbl::FileChannel::writer() {
    return {
        .impl = _mHandle,
        .pfnWrite = nbl::linux::writeFd
    };
}

nbl::Reader nbl::FileChannel::reader() {
    return {
        .impl = _mHandle,
        .pfnRead = nbl::linux::readFd
    };
}

nbl::Result nbl::FileChannel::seek(isize offset, FileSeekOrigin origin, usize* outOffset) {
    int whence = SEEK_SET;
    switch(origin) {
        case FileSeekOrigin::BEGIN:
            whence = SEEK_SET;
            break;
        case FileSeekOrigin::CURRENT:
            whence = SEEK_CUR;
            break;
        case FileSeekOrigin::END:
            whence = SEEK_END;
            break;
    }

    off_t result = ::lseek(gGetFd(_mHandle), static_cast<off_t>(offset), whence);
    if(result < 0) {
        return nbl::Result::IO_ERROR;
    }
    if(outOffset) {
        *outOffset = static_cast<usize>(result);
    }

    return nbl::Result::SUCCESS;
}

nbl::Result nbl::FileChannel::offset(usize* outOffset) {
    off_t current = ::lseek(gGetFd(_mHandle), 0, SEEK_CUR);
    if(current < 0) {
        return nbl::Result::IO_ERROR;
    }
    if(outOffset) {
        *outOffset = static_cast<usize>(current);
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::FileChannel::open(StringView path, FileChannel* outChannel, FileOpenFlags flags) {
    int openFlags = 0;

    if(flags & FileOpenFlag::READ && flags & FileOpenFlag::WRITE) {
        openFlags |= O_RDWR;
} else if(flags & FileOpenFlag::READ) {
        openFlags |= O_RDONLY;
    } else if(flags & FileOpenFlag::WRITE) {
        openFlags |= O_WRONLY;    
    } else {
        return nbl::Result::INVALID_ARGUMENT;
    }

    if(flags & FileOpenFlag::CREATE) {
        openFlags |= O_CREAT;

        if(flags & FileOpenFlag::CREATE_PARENTS) {
            usize last = 0;
            NBL_CHECK(path.findLast('/', &last));
            NBL_CHECK(makeDirectories(path.slice(0, last)));
        }
    }
    if(flags & FileOpenFlag::APPEND) {
        openFlags |= O_APPEND;
    }

    int fd;
    if(path.nullTerminated()) {
        fd = ::open(path.data(), openFlags, 0644);
    } else {

        nbl::AllocationFrame frame(nbl::Arena::threadLocal());
        StringView nullTerminatedPath{};
        NBL_CHECK(nbl::StringBuilder::nullTerminate(path, &nullTerminatedPath, &frame));
        fd = ::open(nullTerminatedPath.data(), openFlags, 0644);
    }   

    if(fd < 0) {
        NBL_DEBUG("Failed to open file: ", path, " (", errno, ")");

        switch(errno) {
            case ENOENT: return nbl::Result::NOT_FOUND;
            case EMFILE: return nbl::Result::TOO_MANY_OPEN_FILES;
            default: return nbl::Result::IO_ERROR;
        }
    }

    outChannel->_mHandle = reinterpret_cast<void*>(static_cast<intptr_t>(fd));
    return nbl::Result::SUCCESS;
}

void nbl::FileChannel::_close() {
    if(_mHandle) {
        ::close(gGetFd(_mHandle));
        _mHandle = nullptr;
    }
}

nbl::StringView nbl::getWorkingDirectory() {
    if(gCwd[0] == '\0') {
        NBL_ASSERT(getcwd(gCwd, PATH_MAX) != nullptr);
    }
    return StringView(gCwd);
}

nbl::StringView nbl::getDataDirectory() {
    if(gDataDir[0] == '\0') {

        const char* xdgDataHome = getenv("XDG_DATA_HOME");
        if(xdgDataHome && xdgDataHome[0] != '\0') {
            strlcpy(gDataDir, xdgDataHome, PATH_MAX);
        } else {
            const char* home = getenv("HOME");
            if(home && home[0] != '\0') {
                strlcpy(gDataDir, home, PATH_MAX);
                strlcat(gDataDir, "/.local/share", PATH_MAX);
            } else {
                // Fallback to current working directory if HOME is not set
                NBL_ASSERT(getcwd(gDataDir, PATH_MAX) != nullptr);
            }
        }

    }
    return StringView(gDataDir);
}


nbl::StringView nbl::getTempDirectory() { 
    if(gTempDir[0] == '\0') {
        const char* tempEnv = getenv("TMPDIR");
        if(tempEnv && tempEnv[0] != '\0') {
            strlcpy(gTempDir, tempEnv, PATH_MAX);
        } else {
            strlcpy(gTempDir, "/tmp", PATH_MAX);
        }
    }
    return StringView(gTempDir);
}


bool nbl::FolderIterator::next(StringView* name) {

    struct dirent* entry = ::readdir(static_cast<DIR*>(_mHandle));
    if(entry == nullptr) return false;

    *name = entry->d_name;
    return true;
}

nbl::Result nbl::FolderIterator::create(StringView path, FolderIterator* outIterator) {

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::StringView nullPath;
    NBL_CHECK(nbl::StringBuilder(&frame).append(path).buildNullTerminated(&nullPath));

    DIR* handle = ::opendir(nullPath.data());
    if(handle == nullptr) {
        return nbl::Result::IO_ERROR;
    }

    outIterator->_mHandle = handle;
    return nbl::Result::SUCCESS;
}


void nbl::FolderIterator::_close() {
    ::closedir(static_cast<DIR*>(_mHandle));
}

void nbl::FolderIterator::rewind() {
    ::rewinddir(static_cast<DIR*>(_mHandle));
}


nbl::Result nbl::makeDirectories(StringView path) {

    if(path.length() == 0 || path.length() >= PATH_MAX) return nbl::Result::INVALID_ARGUMENT;

    char temp[PATH_MAX] = {};
    memcpy(temp, path.data(), path.length());

    StringView stackPath = StringView(temp, path.length());

    usize index = 0;
    Result rc = nbl::Result::SUCCESS;
    while(true) {

        rc = stackPath.findFirst('/', &index, index);
        if(rc.error() && rc != nbl::Result::NOT_FOUND) {
            return rc;
        }
        if(rc == nbl::Result::NOT_FOUND) {
            index = path.length();
        }

        if(index == 0) {
            index += 1;
            continue;
        }

        temp[index] = 0;

        if(::mkdir(temp, 0775) == -1) {
            if(errno != EEXIST) {
                nbl::log(nbl::LogLevel::ERROR, "Could not mkdir! ", errno);
                return nbl::Result::IO_ERROR;
            }
        }
        temp[index] = '/';
        index += 1;

        if(index >= path.length()) break;
    }

    return nbl::Result::SUCCESS;
}


bool nbl::fileExists(StringView path) {

    if(path.length() == 0 || path.length() >= PATH_MAX) return false;

    if(path.nullTerminated()) {
        return access(path.data(), F_OK) == 0;
    } else {
        char temp[PATH_MAX] = {};
        memcpy(temp, path.data(), path.length());
        return access(temp, F_OK) == 0;
    }
}


nbl::Result nbl::deleteFile(StringView path) {

    if(path.length() == 0 || path.length() >= PATH_MAX) return nbl::Result::INVALID_ARGUMENT;

    int rc;
    if(path.nullTerminated()) {
        rc = unlink(path.data());
    } else {
        char temp[PATH_MAX] = {};
        memcpy(temp, path.data(), path.length());
        rc = unlink(temp);
    }

    if(rc != 0) return nbl::Result::IO_ERROR;
    return nbl::Result::SUCCESS;
}

bool nbl::isExecutable(StringView path) {
    if(path.length() == 0 || path.length() >= PATH_MAX) return false;

    if(path.nullTerminated()) {
        return access(path.data(), X_OK) == 0;
    } else {
        char temp[PATH_MAX] = {};
        memcpy(temp, path.data(), path.length());
        return access(temp, X_OK) == 0;
    }
}

nbl::Result nbl::setExecutable(StringView path) {

    if(path.length() == 0 || path.length() >= PATH_MAX) return nbl::Result::INVALID_ARGUMENT;

    struct stat fileStat;

    if(path.nullTerminated()) {

        if(access(path.data(), X_OK) == 0) return nbl::Result::SUCCESS;

        if(stat(path.data(), &fileStat) != 0) return nbl::Result::OS_ERROR;
        int mode = fileStat.st_mode & 0777;

        if(chmod(path.data(), mode | 0100) != 0) return nbl::Result::OS_ERROR;
        return nbl::Result::SUCCESS;

    } else {
        char temp[PATH_MAX] = {};
        memcpy(temp, path.data(), path.length());

        if(access(temp, X_OK) == 0) return nbl::Result::SUCCESS;

        if(stat(temp, &fileStat) != 0) return nbl::Result::OS_ERROR;
        int mode = fileStat.st_mode & 0777;

        if(chmod(temp, mode | 0100) != 0) return nbl::Result::OS_ERROR;
        return nbl::Result::SUCCESS;
    }

}


nbl::Result nbl::createLink(StringView target, StringView path) {

    AllocationFrame frame(nbl::Arena::threadLocal());

    if(!target.nullTerminated()) {
        NBL_CHECK(StringBuilder::nullTerminate(target, &target, &frame));
    }
    if(!path.nullTerminated()) {
        NBL_CHECK(StringBuilder::nullTerminate(path, &path, &frame));
    }

    int rc = symlink(target.data(), path.data());
    if(rc != 0) return nbl::Result::OS_ERROR;

    return nbl::Result::SUCCESS;

}

u64 nbl::modifyTime(StringView path) {

    if(path.length() == 0 || path.length() >= PATH_MAX) return nbl::Result::INVALID_ARGUMENT;

    struct stat fileStat;

    if(path.nullTerminated()) {

        if(stat(path.data(), &fileStat) != 0) return 0;
        return fileStat.st_mtime;

    } else {
        char temp[PATH_MAX] = {};
        memcpy(temp, path.data(), path.length());

        if(stat(temp, &fileStat) != 0) return 0;
        return fileStat.st_mtime;
    }
}

nbl::Result nbl::setModifyTime(StringView path, u64 modifyTime) {

    if(path.length() == 0 || path.length() >= PATH_MAX) return nbl::Result::INVALID_ARGUMENT;

    struct timespec times[2];
    times[0].tv_nsec = UTIME_OMIT;

    times[1].tv_sec = modifyTime / 1000;
    times[1].tv_nsec = (modifyTime * 1000000) % 1000000;

    if(path.nullTerminated()) {

        if(utimensat(AT_FDCWD, path.data(), times, 0) == -1) return nbl::Result::OS_ERROR;

    } else {
        char temp[PATH_MAX] = {};
        memcpy(temp, path.data(), path.length());

        if(utimensat(AT_FDCWD, temp, times, 0) == -1) return nbl::Result::OS_ERROR;
    }

    return nbl::Result::SUCCESS;
}

nbl::Result nbl::getOpenFileLimit(u64* currentLimit, u64* maxLimit) {
    struct rlimit limit;
    if(getrlimit(RLIMIT_NOFILE, &limit) == 0) {
        if(currentLimit) *currentLimit = limit.rlim_cur;
        if(maxLimit) *maxLimit = limit.rlim_max;
        return nbl::Result::SUCCESS;
    } else {
        return nbl::Result::OS_ERROR;
    }
}

nbl::Result nbl::setOpenFileLimit(u64 limit) {

    struct rlimit rlimit;
    if(getrlimit(RLIMIT_NOFILE, &rlimit) == -1) {
        return nbl::Result::OS_ERROR;
    }

    rlimit.rlim_cur = nbl::min(limit, rlimit.rlim_max);

    if(setrlimit(RLIMIT_NOFILE, &rlimit) == -1) {
        nbl::log(nbl::LogLevel::ERROR, "Could not setrlimit! ", errno);
        return nbl::Result::OS_ERROR;
    }
    return nbl::Result::SUCCESS;
}


#endif
