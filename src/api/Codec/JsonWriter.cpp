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
 * \file Codec/JsonWriter.cpp
 * \author jwall64
 * \brief Partial implementation of Codec/Json.hpp
 */

#include <Codec/Json.hpp>
#include <Platform/Logging.hpp>

#include <cmath>



nbl::Result nbl::writeJson(Writer writer, JsonElement element) {

    switch(element.type()) {
        case(nbl::JsonType::NONE): {
            return writer.writeString("null");
        }
        case(nbl::JsonType::NUMBER): {
            switch(element.asNumber().type) {
                case(nbl::JsonNumberType::SIGNED): {
                    return nbl::writeInt(element.asNumber().intValue, writer);
                }
                case(nbl::JsonNumberType::UNSIGNED): {
                    return nbl::writeUint(element.asNumber().uintValue, writer);
                }
                case(nbl::JsonNumberType::FLOAT): {
                    i32 fpInfo = std::fpclassify(element.asNumber().floatValue);
                    if(fpInfo == FP_INFINITE || fpInfo == FP_NAN) {
                        return writer.writeString("0");
                    }
                    return nbl::writeFloat(element.asNumber().floatValue, writer);
                }
            }
            break;
        }
        case(nbl::JsonType::BOOLEAN): {
            if(element.asBoolean()) {
                return writer.writeString("true");
            } else {
                return writer.writeString("false");
            }
        }
        case(nbl::JsonType::STRING): {
            NBL_CHECK(writer.writeChar('"'));
            for(char c : element.asString()) {
                switch(c) {
                    case '"':
                    case '\\':
                        writer.writeChar('\\');
                        writer.writeChar(c);
                        continue;
                    case '\b':
                        writer.writeString("\\b");
                        continue;
                    case '\f':
                        writer.writeString("\\f");
                        continue;
                    case '\n':
                        writer.writeString("\\n");
                        continue;
                    case '\r':
                        writer.writeString("\\r");
                        continue;
                    case '\t':
                        writer.writeString("\\t");
                        continue;
                }
                if(c >= 0 && c < 0x20) {
                    nbl::log(nbl::LogLevel::ERROR, "Cannot escape character ", static_cast<i32>(c), " to encode in JSON string");
                    return nbl::Result::FORMAT_ERROR;
                }
                writer.writeChar(c);
            }
            NBL_CHECK(writer.writeChar('"'));
            return nbl::Result::SUCCESS;
        }
        case(nbl::JsonType::ARRAY): {
            NBL_CHECK(writer.writeChar('['));
            usize index = 0;
            for(JsonElement& ele : element.asArray()) {
                if(index++ > 0) {
                    NBL_CHECK(writer.writeChar(','));
                }
                NBL_CHECK(writeJson(writer, ele));
            }
            NBL_CHECK(writer.writeChar(']'));
            return nbl::Result::SUCCESS;
        }
        case(nbl::JsonType::OBJECT): {
            NBL_CHECK(writer.writeChar('{'));
            usize index = 0;
            for(JsonObject::Entry& ent : element.asObject()) {
                if(index++ > 0) {
                    NBL_CHECK(writer.writeChar(','));
                }
                NBL_CHECK(writer.writeChar('"'));
                NBL_CHECK(writer.writeString(ent.key));
                NBL_CHECK(writer.writeString("\":"));
                NBL_CHECK(writeJson(writer, ent.value));
            }
            NBL_CHECK(writer.writeChar('}'));
            return nbl::Result::SUCCESS;
        }
    }

    return nbl::Result::INVALID_ARGUMENT;
}


nbl::Writer nbl::jsonStringWriter(Writer* writer) {

     return {
        .impl = writer,
        .pfnWrite = [](void* impl, const void* data, usize length, usize* bytesWritten) -> nbl::Result {

            Writer writer = *static_cast<Writer*>(impl);
            for(usize i = 0 ; i < length ; i++) {
                char c = static_cast<const char*>(data)[i];
                switch(c) {
                    case '"':
                    case '\\':
                        NBL_CHECK(writer.writeChar('\\'));
                        NBL_CHECK(writer.writeChar(c));
                        continue;
                    case '\b':
                        NBL_CHECK(writer.writeString("\\b"));
                        continue;
                    case '\f':
                        NBL_CHECK(writer.writeString("\\f"));
                        continue;
                    case '\n':
                        NBL_CHECK(writer.writeString("\\n"));
                        continue;
                    case '\r':
                        NBL_CHECK(writer.writeString("\\r"));
                        continue;
                    case '\t':
                        NBL_CHECK(writer.writeString("\\t"));
                        continue;
                }
                if(c < 0x20) {
                    nbl::log(nbl::LogLevel::ERROR, "Cannot escape character ", static_cast<i32>(c), " to encode in JSON string");
                    return nbl::Result::FORMAT_ERROR;
                }
                NBL_CHECK(writer.writeChar(c));
            }

            if(bytesWritten) *bytesWritten = length;
            return nbl::Result::SUCCESS;
        }
    };
}


nbl::JsonWriter::ArrayWriter& nbl::JsonWriter::ArrayWriter::add(JsonElement element) {
    if(_rParent._mLastResult.error()) return *this;

    if(_mNumElements++ > 0) {
        _rParent._mLastResult = _rParent._mSink.writeChar(',');
        if(_rParent._mLastResult.error()) return *this;
    }

    _rParent._mLastResult = writeJson(_rParent._mSink, element);
    return *this;
}

nbl::JsonWriter::ArrayWriter& nbl::JsonWriter::ArrayWriter::write(nbl::FunctionRef<Result(Writer writer)> writer) {

    if(_rParent._mLastResult.error()) return *this;
    
    if(_mNumElements++ > 0) {
        _rParent._mLastResult = _rParent._mSink.writeChar(',');
        if(_rParent._mLastResult.error()) return *this;
    }

    _rParent._mLastResult = writer(_rParent._mSink);
    return *this;
}

nbl::JsonWriter::ArrayWriter& nbl::JsonWriter::ArrayWriter::writeObject(nbl::FunctionRef<Result(ObjectWriter& writer)> writer) {

    if(_rParent._mLastResult.error()) return *this;
    
    if(_mNumElements++ > 0) {
        _rParent._mLastResult = _rParent._mSink.writeChar(',');
        if(_rParent._mLastResult.error()) return *this;
    }
    
    _rParent._mLastResult = _rParent._mSink.writeChar('{');
    if(_rParent._mLastResult.error()) return *this;

    auto obj = ObjectWriter(_rParent);
    Result res = writer(obj);
    
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = res;
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = _rParent._mSink.writeChar('}');
    return *this;

}

nbl::JsonWriter::ArrayWriter& nbl::JsonWriter::ArrayWriter::writeArray(nbl::FunctionRef<Result(ArrayWriter& writer)> writer) {
    if(_rParent._mLastResult.error()) return *this;
    
    if(_mNumElements++ > 0) {
        _rParent._mLastResult = _rParent._mSink.writeChar(',');
        if(_rParent._mLastResult.error()) return *this;
    }
    
    _rParent._mLastResult = _rParent._mSink.writeChar('[');
    if(_rParent._mLastResult.error()) return *this;

    auto obj = ArrayWriter(_rParent);
    nbl::Result res = writer(obj);

    if(_rParent._mLastResult.error()) return *this;
    
    _rParent._mLastResult = res;
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = _rParent._mSink.writeChar(']');
    return *this;
}

nbl::JsonWriter::ObjectWriter& nbl::JsonWriter::ObjectWriter::add(StringView key, JsonElement element) {
    
    if(_rParent._mLastResult.error()) return *this;
    
    if(_mNumElements++ > 0) {
        _rParent._mLastResult = _rParent._mSink.writeChar(',');
        if(_rParent._mLastResult.error()) return *this;
    }

    _rParent._mLastResult = _rParent._mSink.writeChar('"');
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = _rParent._mSink.writeString(key);
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = _rParent._mSink.writeString("\":");
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = writeJson(_rParent._mSink, element);
    return *this;
}

nbl::JsonWriter::ObjectWriter& nbl::JsonWriter::ObjectWriter::write(StringView key, nbl::FunctionRef<Result(Writer writer)> writer) {

    if(_rParent._mLastResult.error()) return *this;
    
    if(_mNumElements++ > 0) {
        _rParent._mLastResult = _rParent._mSink.writeChar(',');
        if(_rParent._mLastResult.error()) return *this;
    }

    _rParent._mLastResult = _rParent._mSink.writeChar('"');
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = _rParent._mSink.writeString(key);
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = _rParent._mSink.writeString("\":");
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = writer(_rParent._mSink);
    return *this;
}


nbl::JsonWriter::ObjectWriter& nbl::JsonWriter::ObjectWriter::writeString(StringView key, nbl::FunctionRef<Result(Writer writer)> writer) {

        if(_rParent._mLastResult.error()) return *this;
    
    if(_mNumElements++ > 0) {
        _rParent._mLastResult = _rParent._mSink.writeChar(',');
        if(_rParent._mLastResult.error()) return *this;
    }

    _rParent._mLastResult = _rParent._mSink.writeChar('"');
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = _rParent._mSink.writeString(key);
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = _rParent._mSink.writeString("\":");
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = _rParent._mSink.writeChar('"');
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = writer(jsonStringWriter(&_rParent._mSink));

    _rParent._mLastResult = _rParent._mSink.writeChar('"');
    if(_rParent._mLastResult.error()) return *this;
    return *this;

}


nbl::JsonWriter::ObjectWriter& nbl::JsonWriter::ObjectWriter::writeObject(StringView key, nbl::FunctionRef<Result(ObjectWriter& writer)> writer) {

    if(_rParent._mLastResult.error()) return *this;
    
    if(_mNumElements++ > 0) {
        _rParent._mLastResult = _rParent._mSink.writeChar(',');
        if(_rParent._mLastResult.error()) return *this;
    }

    _rParent._mLastResult = _rParent._mSink.writeChar('"');
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = _rParent._mSink.writeString(key);
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = _rParent._mSink.writeString("\":{");
    if(_rParent._mLastResult.error()) return *this;

    auto obj = ObjectWriter(_rParent);
    Result res = writer(obj);

    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = res;
    if(_rParent._mLastResult.error()) return *this;
    
    _rParent._mLastResult = _rParent._mSink.writeChar('}');
    return *this;
}
nbl::JsonWriter::ObjectWriter& nbl::JsonWriter::ObjectWriter::writeArray(StringView key, nbl::FunctionRef<Result(ArrayWriter& writer)> writer) {
    if(_rParent._mLastResult.error()) return *this;
    
    if(_mNumElements++ > 0) {
        _rParent._mLastResult = _rParent._mSink.writeChar(',');
        if(_rParent._mLastResult.error()) return *this;
    }

    _rParent._mLastResult = _rParent._mSink.writeChar('"');
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = _rParent._mSink.writeString(key);
    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = _rParent._mSink.writeString("\":[");
    if(_rParent._mLastResult.error()) return *this;

    auto obj = ArrayWriter(_rParent);
    Result res = writer(obj);

    if(_rParent._mLastResult.error()) return *this;

    _rParent._mLastResult = res;
    if(_rParent._mLastResult.error()) return *this;
    
    _rParent._mLastResult = _rParent._mSink.writeChar(']');
    return *this;
}


nbl::Result nbl::JsonWriter::writeElement(nbl::JsonElement element) {
    return nbl::writeJson(_mSink, element);
}

nbl::Result nbl::JsonWriter::writeObject(nbl::FunctionRef<Result(ObjectWriter& writer)> writer) {

    NBL_CHECK(_mLastResult = _mSink.writeChar('{'));

    auto obj = ObjectWriter(*this);
    Result res = writer(obj);

    if(_mLastResult.error()) return _mLastResult;

    _mLastResult = res;
    if(_mLastResult.error()) return _mLastResult;

    NBL_CHECK(_mLastResult = _mSink.writeChar('}'));

    return nbl::Result::SUCCESS;
}
nbl::Result nbl::JsonWriter::writeArray(nbl::FunctionRef<Result(ArrayWriter& writer)> writer) {
    NBL_CHECK(_mLastResult = _mSink.writeChar('['));

    auto obj = ArrayWriter(*this);
    Result res = writer(obj);

    if(_mLastResult.error()) return _mLastResult;

    _mLastResult = res;
    if(_mLastResult.error()) return _mLastResult;

    NBL_CHECK(_mLastResult = _mSink.writeChar(']'));

    return nbl::Result::SUCCESS;
}
