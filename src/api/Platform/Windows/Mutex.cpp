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
  * \file Platform/Windows/Mutex.cpp
  * \author jwall64
  * \brief Implementation of Platform/Mutex.cpp for Windows
  */

#include <Platform/PlatformDefines.hpp>

#ifdef NBL_PLATFORM_WINDOWS

#include <Core/Result.hpp>
#include <Platform/Mutex.hpp>

#include "WindowsUtil.hpp"

nbl::Mutex::~Mutex() {
    DeleteCriticalSection(_mHandle);
}

void nbl::Mutex::lock() {
    EnterCriticalSection(_mHandle, INFINITE);
}

void nbl::Mutex::unlock() {
    LeaveCriticalSection(_mHandle);
}

nbl::Result nbl::Mutex::create(Mutex* out) {
    if(!InitializeCriticalSectionAndSpinCount(&out->_mHandle, 0x00000400)) {
        return nbl::Result::THREAD_ERROR;
    }
    return nbl::Result::SUCCESS;
}



nbl::Condition::~Condition() {
    broadcast();
}

void nbl::Condition::signal() {
    WakeConditionVariable(_mHandle);
}
void nbl::Condition::broadcast() {
    WakeAllConditionVariable(_mHandle);
}
void nbl::Condition::wait(Mutex& mutex) {
    SleepConditionVariableCS(_mHandle, &mutex._mHandle);
}

nbl::Result nbl::Condition::create(Condition* out) {
    if(!InitializeConditionVariable(&out->_mHandle)) {
        return nbl::Result::THREAD_ERROR;
    }
    return nbl::Result::SUCCESS;
}



nbl::Latch::~Latch() {
    InterlockedExchange(&_mCount, 0);
}

void nbl::Latch::increment() {
    InterlockedIncrement(_mCount);
}

void nbl::Latch::decrement() {
    InterlockedDecrement(_mCount);
}

void nbl::Latch::wait() {
    LONG current = _mCount;
    while(current > 0) {
        WaitOnAddress(&_mCount, &current, sizeof(LONG), INFINITE);
        current = _mCount;
    }
}

nbl::Result nbl::Latch::create(usize count, Latch* out) {
    out->_mCount = count;
}


#endif 
