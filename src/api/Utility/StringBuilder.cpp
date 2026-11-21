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
 * \file Utility/StringBuilder.cpp
 * \author jwall64
 * \brief Implementation of Utility/StringBuilder.hpp
 */

#include <Utility/StringBuilder.hpp>

nbl::Result nbl::StringBuilder::replaceChecked(char oldValue, StringView newValue, usize startIndex) noexcept {
        
    if(!_mLastResult.success()) {
        return _mLastResult;
    }
    if(_mLength == 0) {
        return nbl::Result::SUCCESS;
    }

    char* stringData = static_cast<char*>(_pAllocator->begin()) + _mBegin;
    usize stringLength = _mLength;

    usize occurrances = 0;
    for(usize i = startIndex ; i < stringLength; i++) {
        char c = stringData[i];
        if(c == oldValue) {
            occurrances++;
        }
    }

    usize outStringLength = stringLength + (occurrances * (newValue.length() - 1));


    if(outStringLength > stringLength) {
        usize diff = outStringLength - stringLength;
        NBL_CHECK(_pAllocator->push(diff, nullptr));

        for(usize i = startIndex; i < outStringLength; i++) {
            char c = stringData[i];
            if(c == oldValue) {
                memmove(stringData + i + newValue.length(), stringData + i + 1, stringLength - i - 1);
                memcpy(stringData + i, newValue.data(), newValue.length());
                i += newValue.length() - 1;
                stringLength += newValue.length() - 1;
            } else {
                stringData[i] = c;
            }
        }

    } else {
        for(usize i = startIndex; i < stringLength; i++) {
            char c = stringData[i];
            if(c == oldValue) {
                stringData[i] = newValue.data()[0];
            }
        }
    }

    _mLength = outStringLength;

    return Result::SUCCESS;


}
