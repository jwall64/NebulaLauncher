#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif 

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef NOGDI
#define NOGDI
#endif

#include <winsock2.h>
#include <windows.h>

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
