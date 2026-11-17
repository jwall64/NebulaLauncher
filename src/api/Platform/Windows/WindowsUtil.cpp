#include "WindowsUtil.hpp"

#include <Core/Math.hpp>

nbl::Result nbl::win::writeHandle(void* impl, const void* data, usize length, usize* bytesWritten) {

    HANDLE h = impl;

    DWORD toWrite = static_cast<DWORD>(min(length, 0xFFFFFFFF));
    DWORD written;

    if(!WriteFile(h, data, toWrite, &written, nullptr)) {
        return Result::IO_ERROR;
    }
    if(bytesWritten) {
        *bytesWritten = static_cast<usize>(written);
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::win::readHandle(void* impl, void* data, usize length, usize* bytesRead) {

    HANDLE h = impl;

    DWORD toRead = static_cast<DWORD>(min(length, 0xFFFFFFFF));
    DWORD read;

    if(!ReadFile(h, data, toRead, &read, nullptr)) {
        return Result::IO_ERROR;
    }
    if(bytesRead) {
        *bytesRead = static_cast<usize>(read);
    }
    return nbl::Result::SUCCESS;
}
