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
 * \file Platform/File.hpp
 * \author jwall64
 * \brief Interface for reading, writing, and managing files and directories
 */

#pragma once

#include <Core/Types.hpp>
#include <Core/String.hpp>
#include <Core/Result.hpp>
#include <Core/Stream.hpp>


#if defined(NBL_PLATFORM_WINDOWS)
#include <minwinbase.h>
#define NBL_PATH_SEPARATOR '\\';
#else
#define NBL_PATH_SEPARATOR '/';
#endif


namespace nbl {

enum class FileOpenFlag : u8 {
    READ   = 0b0001,
    WRITE  = 0b0010,
    CREATE = 0b0100,
    APPEND = 0b1000,
    CREATE_PARENTS = 0b10000
};

typedef u8 FileOpenFlags;

inline FileOpenFlags operator&(FileOpenFlag flag, FileOpenFlag other) {
    return static_cast<u8>(flag) & static_cast<u8>(other);
}

inline FileOpenFlags operator&(FileOpenFlags existing, FileOpenFlag flag) {
    return static_cast<u8>(flag) & existing;
}

inline FileOpenFlags operator&(FileOpenFlag flag, FileOpenFlags existing) {
    return static_cast<u8>(flag) & existing;
}

inline FileOpenFlags operator&=(FileOpenFlag& flag, FileOpenFlag other) {
    return static_cast<u8>(flag) & static_cast<u8>(other);
}

inline FileOpenFlags operator&=(FileOpenFlags& existing, FileOpenFlag flag) {
    return static_cast<u8>(flag) & existing;
}

inline FileOpenFlags operator&=(FileOpenFlag& flag, FileOpenFlags existing) {
    return static_cast<u8>(flag) & existing;
}

inline FileOpenFlags operator|(FileOpenFlag flag, FileOpenFlag other) {
    return static_cast<u8>(flag) | static_cast<u8>(other);
}

inline FileOpenFlags operator|(FileOpenFlag flag, FileOpenFlags existing) {
    return static_cast<u8>(flag) | existing;
}

inline FileOpenFlags operator|(FileOpenFlags existing, FileOpenFlag flag) {
    return static_cast<u8>(flag) | existing;
}

inline FileOpenFlags operator|=(FileOpenFlag& flag, FileOpenFlag other) {
    return static_cast<u8>(flag) | static_cast<u8>(other);
}

inline FileOpenFlags operator|=(FileOpenFlag& flag, FileOpenFlags existing) {
    return static_cast<u8>(flag) | existing;
}

inline FileOpenFlags operator|=(FileOpenFlags& existing, FileOpenFlag flag) {
    return static_cast<u8>(flag) | existing;
}


enum class FileSeekOrigin : u8 {
    BEGIN,
    CURRENT,
    END
};


class FileChannel {
public:

    inline FileChannel() : _mHandle(nullptr) {}
    inline ~FileChannel() { _close(); }

    FileChannel(const FileChannel& other) = delete;
    FileChannel(FileChannel&& other);

    FileChannel& operator=(const FileChannel& other) = delete;
    FileChannel& operator=(FileChannel&& other);

    Writer writer();
    Reader reader();

    nbl::Result seek(isize offset, FileSeekOrigin origin, usize* outOffset);

    nbl::Result offset(usize* outOffset);

    static Result open(StringView path, FileChannel* outChannel, FileOpenFlag flag) {
        return open(path, outChannel, static_cast<FileOpenFlags>(flag));
    }

    static Result open(StringView path, FileChannel* outChannel, FileOpenFlags flags);

private:

    void _close();

    void* _mHandle;
};

struct FileInfo {
    StringView name;
    bool isDirectory;
};

class FolderIterator {
public:

    FolderIterator() = default;
    ~FolderIterator() { _close(); }

    bool next(StringView* name);
    void rewind();

    static Result create(StringView path, FolderIterator* outIterator);

private:

    void _close();

#if defined(NBL_PLATFORM_WINDOWS) 
    HANDLE _mHandle;
    Span<wchar_t> _mOriginalQuery;
    StringView _mLastName;
    bool _bPassedFirst;
    Arena _mAlloc;
#else
    void* _mHandle;
#endif

public:

    struct Iterator {
        StringView currentName;
        FolderIterator* it;

        StringView operator*() {
            return currentName;
        }

        Iterator& operator++() {
            if(!it->next(&currentName)) {
                currentName = {};
            }
            return *this;
        }

        Iterator operator++(int) {
            Iterator out = *this;
            operator++();
            return out;
        }

        bool operator!=(const Iterator& other) {
            return other.it == it && other.currentName == currentName;
        }

    };

    Iterator begin() {
        rewind();
        Iterator out = {
            .currentName{},
            .it = this
        };
        out++;
        return out;
    }

    Iterator end() {
        return {
            .currentName{},
            .it = this
        };
    }

};


bool fileExists(StringView path);
Result deleteFile(StringView path);

Result makeDirectories(StringView path);
Result createLink(StringView target, StringView path);

bool isExecutable(StringView path);
Result setExecutable(StringView path);

u64 modifyTime(StringView path);
Result setModifyTime(StringView path, u64 modifyTime);

Result getOpenFileLimit(u64* currentLimit, u64* maxLimit);
Result setOpenFileLimit(u64 limit);

StringView getWorkingDirectory();
StringView getDataDirectory();
StringView getTempDirectory();

}
