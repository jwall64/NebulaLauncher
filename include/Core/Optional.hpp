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
 * \file Core/Optional.hpp
 * \author jwall64
 * \brief Contains an optional/nullable type
 */

#pragma once

#include <Core/Types.hpp>
#include <Core/Assert.hpp>

#include <utility>
#include <new>

namespace nbl {


template<typename T>
class Optional {
public:


    Optional() :
        _mValue{},
        _mHasValue(false)
    { }


    Optional(const T& value) :
        _mValue{},
        _mHasValue(true)
    {
        new (_mValue) T(value);
    }
    

    Optional(T&& value) :
        _mValue{},
        _mHasValue(true)
    {
        new (_mValue) T(std::move(value));
    }


    Optional(const Optional& other) :
        _mValue{},
        _mHasValue(other._mHasValue)
    {
        if(_mHasValue) {
            new (_mValue) T(*reinterpret_cast<const T*>(other._mValue));
        }
    }

    Optional(Optional&& other) :
        _mValue{},
        _mHasValue(other._mHasValue)
    {
        if(_mHasValue) {
            new (_mValue) T(std::move(*reinterpret_cast<T*>(other._mValue)));
            other._mHasValue = false;
        }
    }


    ~Optional() {
        if(_mHasValue) {
            reinterpret_cast<T*>(_mValue)->~T();
        }
    }


    Optional& operator=(const Optional& other) {
        if(this == &other) return *this;
        this->~Optional();

        _mHasValue = other._mHasValue;
        if(_mHasValue) {
            new (_mValue) T(*reinterpret_cast<const T*>(other._mValue));
        }

        return *this;
    }


    Optional& operator=(Optional&& other) {
        if(this == &other) return *this;
        this->~Optional();

        _mHasValue = other._mHasValue;
        if(_mHasValue) {
            new (_mValue) T(std::move(*reinterpret_cast<T*>(other._mValue)));
            other._mHasValue = false;
        }

        return *this;
    }


    Optional& operator=(const T& value) {
        this->~Optional();
        _mHasValue = true;
        new (_mValue) T(value);
        return *this;
    }
    

    Optional& operator=(T&& value) {
        this->~Optional();
        _mHasValue = true;
        new (_mValue) T(std::move(value));
        return *this;
    }


    bool hasValue() const {
        return _mHasValue;
    }


    const T& operator*() const {
        NBL_ASSERT(_mHasValue);
        return *reinterpret_cast<const T*>(_mValue);
    }


    const T* operator->() const {
        NBL_ASSERT(_mHasValue);
        return reinterpret_cast<const T*>(_mValue);
    }


    template<typename O>
    inline Optional<O> map(O(*pfnMapper)(T)) const {
        if(!_mHasValue) return Optional<O>();
        return Optional<O>(pfnMapper(**this));
    }


    template<typename O>
    inline Optional<O> cast() const {
        if(!_mHasValue) return Optional<O>();
        return Optional<O>(static_cast<O>(**this));
    }

private:
    alignas(T) u8 _mValue[sizeof(T)];
    bool _mHasValue;
};

}
