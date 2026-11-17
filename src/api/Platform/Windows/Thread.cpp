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
  * \file Platform/Windows/Thread.cpp
  * \author jwall64
  * \brief Implementation of Platform/Process.hpp for Windows.
  */

#include <Platform/PlatformDefines.hpp>

#ifdef NBL_PLATFORM_WINDOWS

#include <Core/Result.hpp>
#include <Platform/Process.hpp>
#include <Core/Assert.hpp>
#include <Platform/Logging.hpp>
#include <Platform/Pool.hpp>

#include "WindowsUtil.hpp"

namespace {

struct _ThreadData {
    nbl::Thread::EntryPointFn function;
    void* userData;
    std::atomic_int refCount;
    HANDLE handle;
};

nbl::Pool<_ThreadData> _sThreadPool;

DWORD WINAPI _threadEntryPoint(void* data) {
    _ThreadData* threadData = static_cast<_ThreadData*>(data);

    i32 result = threadData->function(threadData->userData);

    int old = threadData->refCount.fetch_sub(1);
    if(old == 1) {
        _sThreadPool.pop(threadData);
    }

    return static_cast<DWORD>(result);
}

}

nbl::Thread::~Thread() {
    detach();
}


bool nbl::Thread::isRunning() const {
    if(_pHandle == nullptr) return false;

    _ThreadData* data = static_cast<_ThreadData*>(_pHandle);
    return data->refCount == 2;
}


nbl::Result nbl::Thread::join(i32* outReturnValue) {

    if(!_pHandle) return nbl::Result::SUCCESS;

    _ThreadData* data = static_cast<_ThreadData*>(_pHandle);
    _pHandle = nullptr;

    int old = data->refCount.fetch_sub(1);
    if(old == 1) {
        if(outReturnValue) *outReturnValue = 0;
        _sThreadPool.pop(data);
    }
    else {
        void* ret;
        HANDLE handle = data->handle;
        DWORD result = WaitForSingleObject(handle, INFINITE);
        if(result != WAIT_OBJECT_0) {
            CloseHandle(handle);
            return nbl::Result::THREAD_ERROR;
        }
        if(outReturnValue) {
            DWORD rc;
            GetExitCodeThread(handle, &rc);
            *outReturnValue = static_cast<i32>(rc);
        }

        CloseHandle(handle);
    }


    return nbl::Result::SUCCESS;
}

nbl::Result nbl::Thread::detach() {

    if(_pHandle) {

        _ThreadData* data = static_cast<_ThreadData*>(_pHandle);
        _pHandle = nullptr;

        int old = data->refCount.fetch_sub(1);
        if(old == 1) {
            _sThreadPool.pop(data);
        }
        else {
            if(!CloseHandle(data->handle)) {
                return nbl::Result::THREAD_ERROR;
            }
        }
    }

    return nbl::Result::SUCCESS;

}

nbl::Result nbl::Thread::create(EntryPointFn function, void* userData, Thread* outThread) {

    if(_sThreadPool.capacity() == 0) {
        NBL_CHECK(nbl::Pool<_ThreadData>::create(128, &_sThreadPool));
    }

    NBL_ASSERT(outThread != nullptr);

    _ThreadData* threadData;
    NBL_CHECK(_sThreadPool.push(&threadData));

    threadData->function = function;
    threadData->userData = userData;
    threadData->refCount = 2;

    int result = CreateThread(
        nullptr,
        0,
        _threadEntryPoint,
        threadData,
        0,
        nullptr);

    if(result != 0) {
        nbl::log(nbl::LogLevel::DEBUG, "Thread create failed: ", result);
        _sThreadPool.pop(threadData);
        return nbl::Result::THREAD_ERROR;
    }

    outThread->_pHandle = threadData;
    return nbl::Result::SUCCESS;

}


#endif