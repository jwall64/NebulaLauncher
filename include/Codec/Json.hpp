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
 * \file Codec/Json.hpp
 * \author jwall64
 * \brief Interface for reading and writing JSON data
 */

#pragma once

#include <Core/Types.hpp>
#include <Core/String.hpp>
#include <Platform/Arena.hpp>
#include <Core/Assert.hpp>
#include <Platform/Allocator.hpp>

#include <Core/FunctionRef.hpp>

namespace nbl {

enum class JsonType {
    NONE,
    NUMBER,
    BOOLEAN,
    STRING,
    ARRAY,
    OBJECT
};

enum class JsonNumberType {
    SIGNED,
    UNSIGNED,
    FLOAT
};

struct JsonNumber {
    JsonNumberType type;
    union {
        u64 uintValue;
        i64 intValue;
        f64 floatValue;
    };

    u64 asUint() const noexcept {
        switch(type) {
            case JsonNumberType::SIGNED: return static_cast<u64>(intValue);
            case JsonNumberType::UNSIGNED: return uintValue;
            case JsonNumberType::FLOAT: return static_cast<u64>(floatValue);
        }
        return 0;
    }

    i64 asInt() const noexcept {
        switch(type) {
            case JsonNumberType::SIGNED: return intValue;
            case JsonNumberType::UNSIGNED: return static_cast<i64>(intValue);
            case JsonNumberType::FLOAT: return static_cast<i64>(floatValue);
        }
        return 0;
    }

    f64 asFloat() const noexcept {
        switch(type) {
            case JsonNumberType::SIGNED: return static_cast<f64>(intValue);
            case JsonNumberType::UNSIGNED: return static_cast<f64>(uintValue);
            case JsonNumberType::FLOAT: return floatValue;
        }
        return 0.0;
    }
};

struct JsonObject;
struct JsonElement;
struct JsonArray {
    JsonElement* elements;
    usize count;

    JsonElement* get(usize index);
    const JsonElement* get(usize index) const;

    JsonElement& operator[](usize index);
    const JsonElement& operator[](usize index) const;

    inline JsonElement* begin() { return elements; }
    JsonElement* end();

    inline const JsonElement* begin() const { return elements; }
    const JsonElement* end() const;

    JsonObject getObject(usize index) const;
    JsonObject getObject(usize index, JsonObject defaultValue) const;
    JsonArray getArray(usize index, JsonArray defaultValue={}) const;
    StringView getString(usize index, StringView defaultValue={}) const;
    JsonNumber getNumber(usize index, JsonNumber defaultValue={}) const;
    bool getBoolean(usize index, bool defaultValue=false) const;

    class Builder;
};

struct JsonObject {
    struct Entry;
    struct Bucket {
        usize offset;
        usize count;
    };

    Entry* entries;
    usize entryCount;

    Bucket* buckets;
    usize bucketCount;

    const JsonElement* get(StringView key) const;
    bool has(StringView key) const { return get(key) != nullptr; }

    inline Entry* begin() { return entries; }
    Entry* end();

    inline const Entry* begin() const { return entries; }
    const Entry* end() const;

    JsonObject getObject(StringView key, JsonObject defaultValue={}) const;
    JsonArray getArray(StringView key, JsonArray defaultValue={}) const;
    StringView getString(StringView key, StringView defaultValue={}) const;
    JsonNumber getNumber(StringView key, JsonNumber defaultValue={}) const;
    bool getBoolean(StringView key, bool defaultValue=false) const;

    class Builder;
};

class JsonElement {
public:

    JsonElement() :
        _mType(JsonType::NONE),
        object{}
    {} 

    JsonElement(const JsonElement& other) noexcept = default;
    JsonElement& operator=(const JsonElement& other) noexcept = default;

    JsonElement(JsonNumber number) :
        _mType(JsonType::NUMBER),
        number(number)
    {}

    JsonElement(u64 uint) :
        _mType(JsonType::NUMBER),
        number{ .type = JsonNumberType::UNSIGNED, .uintValue = uint }
    {}
    
    JsonElement(u32 uint) :
        _mType(JsonType::NUMBER),
        number{ .type = JsonNumberType::UNSIGNED, .uintValue = uint }
    {}

    JsonElement(i64 sint) :
        _mType(JsonType::NUMBER),
        number{ .type = JsonNumberType::SIGNED, .intValue = sint }
    {}

    JsonElement(i32 sint) :
        _mType(JsonType::NUMBER),
        number{ .type = JsonNumberType::SIGNED, .intValue = sint }
    {}

    JsonElement(f64 dbl) :
        _mType(JsonType::NUMBER),
        number{ .type = JsonNumberType::FLOAT, .floatValue = dbl }
    {}

    JsonElement(bool boolean) :
        _mType(JsonType::BOOLEAN),
        boolean(boolean)
    {}
    
    JsonElement(StringView string) :
        _mType(JsonType::STRING),
        string(string)
    {}
 
    JsonElement(const char* string) :
        _mType(JsonType::STRING),
        string(string)
    {}

    JsonElement(const JsonArray& array) :
        _mType(JsonType::ARRAY),
        array(array)
    {}

    JsonElement(const JsonObject& object) :
        _mType(JsonType::OBJECT),
        object(object)
    {}

    inline JsonType type() const noexcept {
        return _mType;
    }

    inline JsonObject asObject(JsonObject defaultValue = {}) const noexcept {
        if(_mType == JsonType::OBJECT) return object;
        return defaultValue;
    }

    inline JsonArray asArray(JsonArray defaultValue = {}) const noexcept {
        if(_mType == JsonType::ARRAY) return array;
        return defaultValue;
    }

    inline JsonNumber asNumber(JsonNumber defaultValue = {}) const noexcept {
        if(_mType == JsonType::NUMBER) return number;
        return defaultValue;
    }

    inline i64 asInt(i64 defaultValue = {}) const noexcept {
        if(_mType == JsonType::NUMBER) return number.asInt();
        return defaultValue;
    }

    inline u64 asUint(u64 defaultValue = {}) const noexcept {
        if(_mType == JsonType::NUMBER) return number.asUint();
        return defaultValue;
    }

    inline f64 asFloat(f64 defaultValue = {}) const noexcept {
        if(_mType == JsonType::NUMBER) return number.asFloat();
        return defaultValue;
    }

    inline StringView asString(StringView defaultValue = {}) const noexcept {
        if(_mType == JsonType::STRING) return string;
        return defaultValue;
    }

    inline bool asBoolean(bool defaultValue = false) const noexcept {
        if(_mType == JsonType::BOOLEAN) return boolean;
        return defaultValue;
    }

private:
    JsonType _mType;
    union {
        JsonNumber number;
        bool boolean;
        StringView string;
        JsonArray array;
        JsonObject object;
    };
};

struct JsonObject::Entry {
    StringView key;
    JsonElement value;
};


inline JsonElement& JsonArray::operator[](usize index) {
    NBL_ASSERT(index < count);
    return elements[index];
}

inline const JsonElement& JsonArray::operator[](usize index) const {
    NBL_ASSERT(index < count);
    return elements[index];
}

inline JsonElement* JsonArray::end() {
    return elements + count;
}
inline const JsonElement* JsonArray::end() const {
    return elements + count;
}

inline JsonObject::Entry* JsonObject::end() {
    return entries + entryCount;
}
inline const JsonObject::Entry* JsonObject::end() const {
    return entries + entryCount;
}

Result parseJson(Reader reader, LinearAllocator* storage, JsonElement* outElement, LinearAllocator* tempStorage = nullptr, bool retainOrder = false);
Result parseJson(StringView string, LinearAllocator* storage, JsonElement* outElement, LinearAllocator* tempStorage = nullptr, bool retainOrder = false);
Result parseJsonNumber(Reader reader, JsonNumber* outNumber);
Result parseJsonNumber(StringView string, JsonNumber* outNumber);

Result writeJson(Writer writer, JsonElement element);

inline Result write(JsonElement element, Writer writer) { return writeJson(writer, element); }
inline Result write(JsonArray element, Writer writer) { return writeJson(writer, element); }
inline Result write(JsonObject element, Writer writer) { return writeJson(writer, element); }

Writer jsonStringWriter(Writer* writer);

class JsonWriter {
public:

    JsonWriter(Writer writer) :
        _mLastResult(nbl::Result::SUCCESS),
        _mSink(writer)
    { }

    class ObjectWriter;

    class ArrayWriter {
    public:
        ArrayWriter& add(JsonElement element);
        ArrayWriter& write(nbl::FunctionRef<Result(Writer writer)> writer);
        ArrayWriter& writeObject(nbl::FunctionRef<Result(ObjectWriter& writer)> writer);
        ArrayWriter& writeArray(nbl::FunctionRef<Result(ArrayWriter& writer)> writer);
    private:

        ArrayWriter(JsonWriter& parent) :
            _rParent(parent)
        { }

        usize _mNumElements = 0;
        JsonWriter& _rParent;
        friend class JsonWriter;
    };

    class ObjectWriter {
    public:
        ObjectWriter& add(StringView key, JsonElement element);
        ObjectWriter& write(StringView key, nbl::FunctionRef<Result(Writer writer)> writer);
        ObjectWriter& writeString(StringView key, nbl::FunctionRef<Result(Writer writer)> writer);
        ObjectWriter& writeObject(StringView key, nbl::FunctionRef<Result(ObjectWriter& writer)> writer);
        ObjectWriter& writeArray(StringView key, nbl::FunctionRef<Result(ArrayWriter& writer)> writer);

    private:

        ObjectWriter(JsonWriter& parent) :
            _rParent(parent)
        { }

        usize _mNumElements = 0;
        JsonWriter& _rParent;
        friend class JsonWriter;
    };


    Result writeElement(JsonElement element);
    Result writeObject(nbl::FunctionRef<Result(ObjectWriter& writer)> writer);
    Result writeArray(nbl::FunctionRef<Result(ArrayWriter& writer)> writer);

private:

    Result _mLastResult;
    Writer _mSink;

};


class JsonObject::Builder {
public:

    Builder() = default;

    Builder(const Builder& other) = delete;

    Result add(StringView key, JsonElement element);
    Result addObject(StringView key, nbl::FunctionRef<Result(JsonObject::Builder&)> callback, usize buckets = 0);
    Result addArray(StringView key, nbl::FunctionRef<Result(JsonArray::Builder&)> callback);

    Result build(JsonObject* out, usize bucketCount = 0);

    static Result create(nbl::LinearAllocator* allocator, nbl::JsonObject::Builder* outBuilder);

private:

    Builder(LinearAllocator* allocator, Arena* scratch) :
        _pAllocator(allocator),
        _mScratch{},
        _pParentScratch(scratch),
        _pBegin(static_cast<JsonObject::Entry*>(scratch->end())),
        _mEntryCount(0)
    { }

    inline Arena* _scratch() { 
        if(_pParentScratch) {
            return _pParentScratch;
        }
        return &_mScratch;
    }

    nbl::LinearAllocator* _pAllocator;
    Arena _mScratch;
    Arena* _pParentScratch;
    
    JsonObject::Entry* _pBegin;
    usize _mEntryCount;
    

    friend class JsonArray::Builder;
};

class JsonArray::Builder {
public:

    Builder() = default;

    Builder(const Builder& other) = delete;

    Result add(JsonElement element);
    Result addObject(nbl::FunctionRef<Result(JsonObject::Builder&)> callback);
    Result addArray(nbl::FunctionRef<Result(JsonArray::Builder&)> callback);

    Result build(JsonArray* out);

    static Result create(nbl::LinearAllocator* allocator, nbl::JsonArray::Builder* outBuilder);

private:

    Builder(LinearAllocator* allocator, Arena* scratch) :
        _pAllocator(allocator),
        _mScratch{},
        _pParentScratch(scratch),
        _mElementCount(0),
        _pBegin(static_cast<JsonElement*>(scratch->end()))
    { }

    inline Arena* _scratch() { 
        if(_pParentScratch) {
            return _pParentScratch;
        }
        return &_mScratch;
    }

    nbl::LinearAllocator* _pAllocator;
    Arena _mScratch;
    Arena* _pParentScratch;

    JsonElement* _pBegin;
    usize _mElementCount;

    friend class JsonObject::Builder;
};


}
