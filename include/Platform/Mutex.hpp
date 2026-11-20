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
 * \file Platform/Mutex.hpp
 * \author jwall64
 * \brief Interfaces for thread synchronization objects
 */


#pragma once

#include <Platform/PlatformDefines.hpp>
#include <Platform/Process.hpp>
#include <Core/Result.hpp>

#include <utility>

#if defined(NBL_PLATFORM_LINUX)

#include <pthread.h>
#include <atomic>

#elif defined(NBL_PLATFORM_WINDOWS)

#include <synchapi.h>
#undef ERROR

#endif


namespace nbl {


class Mutex {
public:

    Mutex() = default;
    ~Mutex();

    void lock();
    void unlock();

    static Result create(Mutex* out);


    struct FrameLock {
        ~FrameLock() {
            mutex->unlock();
        }
        Mutex* mutex;
    };

    [[nodiscard]] FrameLock frameLock() {
        lock();
        return { .mutex = this };
    }

private:

#if defined(NBL_PLATFORM_LINUX)
    pthread_mutex_t _mHandle;
#elif defined(NBL_PLATFORM_WINDOWS)
    CRITICAL_SECTION _mHandle;
#else
    void* _mHandle;
#endif
    friend class Condition;
};


class Condition {
public:

    Condition() = default;
    ~Condition();

    void signal();
    void broadcast();
    void wait(Mutex& mutex);

    static Result create(Condition* out);

private:

#if defined(NBL_PLATFORM_LINUX)
    pthread_cond_t _mHandle;
#elif defined(NBL_PLATFORM_WINDOWS)
    CONDITION_VARIABLE _mHandle;
#else
    void* _mHandle;
#endif

};


class Latch {
public:

    ~Latch();

    void increment();
    void decrement();
    void wait();
    static Result create(usize count, Latch* out);

private:

#if defined(NBL_PLATFORM_LINUX)
    pthread_mutex_t _mMutex;
    pthread_cond_t _mCondition;
    std::atomic_int _mCount;
    bool _bInitialized;
#elif defined(NBL_PLATFORM_WINDOWS)
    volatile LONG _mCount;
#else
    void* _mHandle;
#endif

};


template<typename T>
class Future {
public:

    void complete(const T& value) {
        new (_mValue) T(value);
        _mCondition.broadcast();
    }

    void complete(T&& value) {
        new (_mValue) T(std::move(value));
        _mCondition.broadcast();
    }

    template<typename... Args>
    void emplaceComplete(Args&&... args) {
        new (_mValue) T(std::forward<Args...>(args...));
        _mCondition.broadcast();
    }

    T& wait() {
        _mMutex.lock();
        _mCondition.wait(_mMutex);
        _mMutex.unlock();
        return *reinterpret_cast<T*>(_mValue);
    }

    static Result create(Future* out) {
        NBL_CHECK(Condition::create(&out->_mCondition));
        NBL_CHECK(Mutex::create(&out->_mMutex));
    }

private:
    Condition _mCondition;
    Mutex _mMutex;
    u8 _mValue[sizeof(T)];
};

}

