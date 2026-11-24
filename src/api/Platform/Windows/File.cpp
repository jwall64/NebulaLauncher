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
 * \file Platform/Windows/File.cpp
 * \author jwall64
 * \brief Implementation of Platform/File.hpp for Windows.
 */


#include <Platform/PlatformDefines.hpp>
#ifdef NBL_PLATFORM_WINDOWS

#include <Core/Result.hpp>
#include <Core/Math.hpp>
#include <Core/Assert.hpp>
#include <Platform/File.hpp>
#include <Platform/Socket.hpp>
#include <Platform/Arena.hpp>
#include <Platform/Logging.hpp>
#include <Utility/StringBuilder.hpp>

#include "WindowsUtil.hpp"

#include <shlobj.h>
#include <aclapi.h>


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
        .pfnWrite = nbl::win::writeHandle
    };
}

nbl::Reader nbl::FileChannel::reader() {
    return {
        .impl = _mHandle,
        .pfnRead = nbl::win::readHandle
    };
}

nbl::Result nbl::FileChannel::seek(isize offset, FileSeekOrigin origin, usize* outOffset) {
    DWORD method = FILE_BEGIN;
    switch(origin) {
        case FileSeekOrigin::BEGIN:
            method = FILE_BEGIN;
            break;
        case FileSeekOrigin::CURRENT:
            method = FILE_CURRENT;
            break;
        case FileSeekOrigin::END:
            method = FILE_END;
            break;
    }

    LARGE_INTEGER distance;
    distance.QuadPart = offset;

    LARGE_INTEGER newPtr;

    if(!SetFilePointerEx(_mHandle, distance, &newPtr, method)) {
        return nbl::Result::IO_ERROR;
    }
    if(outOffset) {
        *outOffset = static_cast<usize>(newPtr.QuadPart);
    }

    return nbl::Result::SUCCESS;
}

nbl::Result nbl::FileChannel::offset(usize* outOffset) {

    LARGE_INTEGER dist{};
    dist.QuadPart = 0;

    LARGE_INTEGER newPtr{};
    if(!SetFilePointerEx(_mHandle, dist, &newPtr, FILE_CURRENT)) {
        return nbl::Result::IO_ERROR;
    }
    if(outOffset) {
        *outOffset = static_cast<usize>(newPtr.QuadPart);
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::FileChannel::open(StringView path, FileChannel* outChannel, FileOpenFlags flags) {
    DWORD desiredAccess = 0;
    DWORD creationDisposition = 0;

    if(flags & FileOpenFlag::READ) {
        desiredAccess |= GENERIC_READ;
    } 
    if(flags & FileOpenFlag::WRITE) {
        desiredAccess |= GENERIC_WRITE;
    }
    
    if(desiredAccess == 0) {
        return nbl::Result::INVALID_ARGUMENT;
    }

    if(flags & FileOpenFlag::CREATE) {
        desiredAccess |= GENERIC_WRITE;
        if(flags & FileOpenFlag::APPEND) {
            creationDisposition = OPEN_ALWAYS;
        }
        else {
            creationDisposition = CREATE_ALWAYS;
        }

        if(flags & FileOpenFlag::CREATE_PARENTS) {
            usize last = 0;
            NBL_CHECK(path.findLast('\\', &last));
            NBL_CHECK(makeDirectories(path.slice(0, last)));
        }
    }
    else {
        if(flags & FileOpenFlag::APPEND || !(flags & FileOpenFlag::WRITE)) {
            creationDisposition = OPEN_EXISTING;
        }
        else {
            creationDisposition = TRUNCATE_EXISTING;
        }
    }

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), nullptr, 0);
    if(sz == 0) return Result::INVALID_ARGUMENT;

    wchar_t* fileName;

    frame.allocate(&fileName, sz + 1);
    if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), fileName, sz)) {
        return Result::INVALID_ARGUMENT;
    }

    outChannel->_mHandle = CreateFileW(
        fileName,
        desiredAccess,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        creationDisposition,
        0,
        nullptr
    );

    if(outChannel->_mHandle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        NBL_DEBUG("Failed to open file: ", path, " (", err, ")");

        outChannel->_mHandle = nullptr;
        switch(err) {
            case ERROR_FILE_NOT_FOUND: return nbl::Result::NOT_FOUND;
            case ERROR_TOO_MANY_OPEN_FILES: return nbl::Result::TOO_MANY_OPEN_FILES;
            default: nbl::Result::IO_ERROR;
        }
    }

    return nbl::Result::SUCCESS;
}

void nbl::FileChannel::_close() {
    if(_mHandle) {
        CloseHandle(_mHandle);
        _mHandle = nullptr;
    }
}

nbl::StringView nbl::getWorkingDirectory() {

    static char* _data;
    static usize _length;

    if(_length == 0) {

        DWORD wsize = GetCurrentDirectory(0, nullptr);
        if(wsize == 0) return {};

        wchar_t* temp;
        AllocationFrame frame = Arena::threadLocal();
        frame.allocate(&temp, wsize);

        wsize = GetCurrentDirectoryW(wsize, temp);

        int utfSize = WideCharToMultiByte(CP_UTF8, 0, temp, wsize, nullptr, 0, nullptr, nullptr);
        if(utfSize <= 0) return {};

        _data = new char[utfSize];
        if(_data == nullptr) return {};

        _length = utfSize;

        WideCharToMultiByte(CP_UTF8, 0, temp, wsize, _data, utfSize, nullptr, nullptr);
    }
    return StringView(_data, _length);
}

nbl::StringView nbl::getDataDirectory() {

    static char* _data;
    static usize _length;

    if(_length == 0) {

        PWSTR path = nullptr;
        HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path);

        NBL_ASSERT(SUCCEEDED(hr));
        DWORD len = wcslen(path);
   
        int utfSize = WideCharToMultiByte(CP_UTF8, 0, path, len, nullptr, 0, nullptr, nullptr);
        if(utfSize <= 0) return {};

        _data = new char[utfSize];
        if(_data == nullptr) return {};

        _length = utfSize;

        WideCharToMultiByte(CP_UTF8, 0, path, len, _data, utfSize, nullptr, nullptr);
    }
    return StringView(_data, _length);
}


nbl::StringView nbl::getTempDirectory() { 

    static char* _data;
    static usize _length;

    if(_length == 0) {

        DWORD wsize = GetTempPathW(0, nullptr);
        if(wsize == 0) return {};

        wchar_t* temp;
        AllocationFrame frame = Arena::threadLocal();
        frame.allocate(&temp, wsize);

        wsize = GetTempPathW(wsize, temp);

        int utfSize = WideCharToMultiByte(CP_UTF8, 0, temp, wsize, nullptr, 0, nullptr, nullptr);
        if(utfSize <= 0) return {};

        _data = new char[utfSize];
        if(_data == nullptr) return {};

        _length = utfSize;

        WideCharToMultiByte(CP_UTF8, 0, temp, wsize, _data, utfSize, nullptr, nullptr);
    }
    return StringView(_data, _length);
}


bool nbl::FolderIterator::next(StringView* name) {

    if(_bPassedFirst) {
        WIN32_FIND_DATAW data{};
        if(!FindNextFileW(_mHandle, &data)) {
            return false;
        }

        _mAlloc.popToPtr(_mLastName.begin());

        DWORD fileNameLen = wcslen(data.cFileName);

        int utfSize = WideCharToMultiByte(CP_UTF8, 0, data.cFileName, fileNameLen, nullptr, 0, nullptr, nullptr);
        if(utfSize <= 0) return false;

        char* fileName;
        NBL_CHECK(_mAlloc.allocate(&fileName, utfSize));

        WideCharToMultiByte(CP_UTF8, 0, data.cFileName, fileNameLen, fileName, utfSize, nullptr, nullptr);
        if(utfSize <= 0) return false;

        _mLastName = StringView(fileName, utfSize);
    }
    _bPassedFirst = true;

    *name = _mLastName;
    return true;
}

nbl::Result nbl::FolderIterator::create(StringView path, FolderIterator* outIterator) {

    NBL_CHECK(Arena::create(MAXSHORT * 2, &outIterator->_mAlloc));

    usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), nullptr, 0);
    if(sz == 0) return Result::INVALID_ARGUMENT;

    wchar_t* folderPath;

    outIterator->_mAlloc.allocate(&folderPath, sz + 3);
    if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), folderPath, sz)) {
        return Result::INVALID_ARGUMENT;
    }
    folderPath[sz] = L'\\';
    folderPath[sz + 1] = L'*';
    folderPath[sz + 2] = L'\0';

    outIterator->_mOriginalQuery = Span<wchar_t>(folderPath, sz + 2);
    outIterator->rewind();

    return nbl::Result::SUCCESS;
}


void nbl::FolderIterator::_close() {
    FindClose(_mHandle);
    _mAlloc.popTo(0);
}

nbl::Result nbl::FolderIterator::rewind() {

    if(_mHandle) {
        FindClose(_mHandle);
    }
    if(!_mLastName.empty()) {
        _mAlloc.popToPtr(_mLastName.begin());
    }

    WIN32_FIND_DATAW data;
    HANDLE handle = FindFirstFileW(_mOriginalQuery.data(), &data);

    if(handle == INVALID_HANDLE_VALUE) {
        return nbl::Result::IO_ERROR;
    }

    DWORD fileNameLen = wcslen(data.cFileName);

    int utfSize = WideCharToMultiByte(CP_UTF8, 0, data.cFileName, fileNameLen, nullptr, 0, nullptr, nullptr);
    if(utfSize <= 0) return nbl::Result::IO_ERROR;

    char* fileName;
    NBL_CHECK(_mAlloc.allocate(&fileName, utfSize));

    WideCharToMultiByte(CP_UTF8, 0, data.cFileName, fileNameLen, fileName, utfSize, nullptr, nullptr);
    if(utfSize <= 0) return nbl::Result::IO_ERROR;

    _mHandle = handle;
    _bPassedFirst = false;
    _mLastName = StringView(fileName, utfSize);

    return nbl::Result::SUCCESS;
}



nbl::Result nbl::makeDirectories(StringView path) {

    if(path.length() == 0) return nbl::Result::INVALID_ARGUMENT;

    char temp[MAX_PATH] = {};
    memcpy(temp, path.data(), path.length());

    StringView stackPath = StringView(temp, path.length());

    usize index = 0;
    Result rc = nbl::Result::SUCCESS;
    while(true) {

        rc = stackPath.findFirst(NBL_PATH_SEPARATOR, &index, index);
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

        AllocationFrame frame(Arena::threadLocal());

        usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), index, nullptr, 0);
        if(sz == 0) return Result::INVALID_ARGUMENT;

        wchar_t* folderPath;

        frame.allocate(&folderPath, sz + 1);
        if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), index, folderPath, sz)) {
            return Result::INVALID_ARGUMENT;
        }

        if(!CreateDirectoryW(folderPath, nullptr)) {
            if(GetLastError() != ERROR_ALREADY_EXISTS) {
                return nbl::Result::IO_ERROR;
            }
        }
        temp[index] = NBL_PATH_SEPARATOR;
        index += 1;

        if(index >= path.length()) break;
    }

    return nbl::Result::SUCCESS;
}


bool nbl::fileExists(StringView path) {

    if(path.length() == 0) return false;

    AllocationFrame frame(Arena::threadLocal());

    usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), nullptr, 0);
    if(sz == 0) return false;

    wchar_t* wpath;

    frame.allocate(&wpath, sz + 1);
    if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), wpath, sz)) {
        return false;
    }

    DWORD attributes = GetFileAttributesW(wpath);
    return attributes != INVALID_FILE_ATTRIBUTES;
}


nbl::Result nbl::deleteFile(StringView path) {

    if(path.length() == 0) return nbl::Result::INVALID_ARGUMENT;

    AllocationFrame frame(Arena::threadLocal());

    usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), nullptr, 0);
    if(sz == 0) return Result::INVALID_ARGUMENT;

    wchar_t* wpath;

    frame.allocate(&wpath, sz + 1);
    if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), wpath, sz)) {
        return Result::INVALID_ARGUMENT;
    }

    if(!DeleteFileW(wpath)) {
        return Result::IO_ERROR;
    }

    return nbl::Result::SUCCESS;
}

bool nbl::isExecutable(StringView path) {

    if(path.length() == 0) return false;

    AllocationFrame frame(Arena::threadLocal());

    usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), nullptr, 0);
    if(sz == 0) return false;

    wchar_t* wpath;

    frame.allocate(&wpath, sz + 1);
    if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), wpath, sz)) {
        return false;
    }

    HANDLE h = CreateFileW(
        wpath,
        FILE_GENERIC_EXECUTE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if(h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        return true;
    }

    return false;
}

nbl::Result nbl::setExecutable(StringView path) {

    if(path.length() == 0) return nbl::Result::INVALID_ARGUMENT;

    AllocationFrame frame(Arena::threadLocal());

    usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), nullptr, 0);
    if(sz == 0) return Result::INVALID_ARGUMENT;

    wchar_t* wpath;

    frame.allocate(&wpath, sz + 1);
    if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), wpath, sz)) {
        return Result::INVALID_ARGUMENT;
    }

    PACL oldDacl = nullptr;
    PACL newDacl = nullptr;
    PSECURITY_DESCRIPTOR sd = nullptr;

    DWORD rc = GetNamedSecurityInfoW(
        wpath,
        SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION,
        nullptr, nullptr, &oldDacl, nullptr,
        &sd
    );
    if(rc != ERROR_SUCCESS) {
        return Result::SUCCESS;
    }

    wchar_t users[6]{
        L'U', L's', L'e', L'r', L's', L'\0'
    };

    EXPLICIT_ACCESS_W ea = {
        .grfAccessPermissions = FILE_GENERIC_EXECUTE,
        .grfAccessMode = GRANT_ACCESS,
        .grfInheritance = NO_INHERITANCE,
        .Trustee = {
            .TrusteeForm = TRUSTEE_IS_NAME,
            .TrusteeType = TRUSTEE_IS_UNKNOWN,
            .ptstrName = users
        }
    };

    rc = SetEntriesInAclW(1, &ea, oldDacl, &newDacl);
    if(rc != ERROR_SUCCESS) {
        if(sd) LocalFree(sd);
        return Result::IO_ERROR;
    }

    rc = SetNamedSecurityInfoW(
        wpath,
        SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION,
        nullptr, nullptr, newDacl, nullptr
    );
    LocalFree(newDacl);

    if(sd) LocalFree(sd);
    return Result::SUCCESS;
}


nbl::Result nbl::createLink(StringView target, StringView path) {

    AllocationFrame frame(nbl::Arena::threadLocal());

    wchar_t* wtarget;
    wchar_t* wpath;

    {
        usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, target.data(), target.length(), nullptr, 0);
        if(sz == 0) return Result::INVALID_ARGUMENT;

        frame.allocate(&wtarget, sz + 1);
        if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, target.data(), target.length(), wtarget, sz)) {
            return Result::INVALID_ARGUMENT;
        }
    }

    {
        usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), nullptr, 0);
        if(sz == 0) return Result::INVALID_ARGUMENT;

        frame.allocate(&wpath, sz + 1);
        if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), wpath, sz)) {
            return Result::INVALID_ARGUMENT;
        }
    }

    if(!CreateSymbolicLinkW(wtarget, wpath, 0)) {
        return nbl::Result::OS_ERROR;
    }

    return nbl::Result::SUCCESS;

}

u64 nbl::modifyTime(StringView path) {

    if(path.length() == 0) return nbl::Result::INVALID_ARGUMENT;

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), nullptr, 0);
    if(sz == 0) return Result::INVALID_ARGUMENT;

    wchar_t* fileName;

    frame.allocate(&fileName, sz + 1);
    if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), fileName, sz)) {
        return Result::INVALID_ARGUMENT;
    }

    WIN32_FILE_ATTRIBUTE_DATA attr;
    if(!GetFileAttributesExW(fileName, GetFileExInfoStandard, &attr)) {
        return Result::IO_ERROR;
    }

    ULARGE_INTEGER li;
    li.HighPart = attr.ftLastWriteTime.dwHighDateTime;
    li.LowPart = attr.ftLastWriteTime.dwLowDateTime;

    return li.QuadPart;
}

nbl::Result nbl::setModifyTime(StringView path, u64 modifyTime) {

    if(path.length() == 0) return nbl::Result::INVALID_ARGUMENT;

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), nullptr, 0);
    if(sz == 0) return Result::INVALID_ARGUMENT;

    wchar_t* fileName;

    frame.allocate(&fileName, sz + 1);
    if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.length(), fileName, sz)) {
        return Result::INVALID_ARGUMENT;
    }

    HANDLE h = CreateFileW(
        fileName,
        FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if(h == INVALID_HANDLE_VALUE) {
        return Result::INVALID_ARGUMENT;
    }


    constexpr static u64 _diff = 116444736000000000ULL;

    ULARGE_INTEGER li{};
    li.QuadPart = (modifyTime * 10000ULL) + _diff;

    FILETIME ft{};
    ft.dwHighDateTime = li.HighPart;
    ft.dwLowDateTime = li.LowPart;

    if(!SetFileTime(h, nullptr, nullptr, &ft)) {
        return Result::IO_ERROR;
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::getOpenFileLimit(u64* currentLimit, u64* maxLimit) {
    *currentLimit = 16711680;
    *maxLimit = 16711680;
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::setOpenFileLimit(u64 limit) {
    return Result::NO_CHANGE;
}


#endif
