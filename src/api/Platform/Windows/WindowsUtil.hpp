#pragma once

#include <Platform/WindowsApi.hpp>

#include <Core/Result.hpp>
#include <Core/Stream.hpp>

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
