#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include <Core/Result.hpp>

namespace nbl::win {
	
Result writeHandle(void* impl, const void* data, usize length, usize* bytesWritten);
Result readHandle(void* impl, void* data, usize length, usize* bytesRead);

inline nbl::Writer handleWriter(HANDLE h) {
    return nbl::Writer{
        .impl = h,
        .pfnWrite = nbl::win::writeHandle
    };
}

}