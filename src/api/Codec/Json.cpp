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
 * \file Codec/Json.cpp
 * \author jwall64
 * \brief Partial implementation of Codec/Json.hpp
 */

#include <Codec/Json.hpp>
#include <Core/Result.hpp>
#include <Platform/Allocator.hpp>
#include <Utility/StringBuilder.hpp>


namespace {


nbl::Result _deepCopy(nbl::JsonElement element, nbl::JsonElement* other, nbl::LinearAllocator* allocator) {

    //other->type = element.type();
    if(element.type() == nbl::JsonType::STRING) {
        nbl::StringView sv;
        NBL_CHECK(nbl::StringBuilder(allocator).append(element.asString()).buildNullTerminated(&sv));
        *other = sv;

    } else if(element.type() == nbl::JsonType::ARRAY) {
        nbl::JsonArray arr(nullptr, element.asArray().count);
        NBL_CHECK(allocator->allocate(&arr.elements, element.asArray().count));
        for(usize i = 0 ; i < element.asArray().count ; i++) {
            NBL_CHECK(_deepCopy(element.asArray()[i], &arr.elements[i], allocator));
        }
        *other = arr;

    } else if(element.type() == nbl::JsonType::OBJECT) {

        nbl::JsonObject obj;

        NBL_CHECK(allocator->allocate(&obj.buckets, element.asObject().bucketCount));
        NBL_CHECK(allocator->allocate(&obj.entries, element.asObject().entryCount));

        memcpy(obj.buckets, element.asObject().buckets, element.asObject().bucketCount * sizeof(nbl::JsonObject::Bucket)); 

        obj.entryCount = element.asObject().entryCount;
        obj.bucketCount = element.asObject().bucketCount;

        for(usize i = 0 ; i < element.asObject().entryCount ; i++) {
            NBL_CHECK(nbl::StringBuilder(allocator).append(element.asObject().entries[i].key).buildNullTerminated(&obj.entries[i].key));
            NBL_CHECK(_deepCopy(element.asObject().entries[i].value, &obj.entries[i].value, allocator));
        }

        *other = obj;

    } else {
        *other = element;
    }
    return nbl::Result::SUCCESS;
}

}


const nbl::JsonElement* nbl::JsonObject::get(nbl::StringView key) const {

    if(bucketCount == 0) return nullptr;

    usize bucket = key.hash() % bucketCount;
    for(usize i = 0 ; i < buckets[bucket].count ; i++) {
        Entry& ent = entries[i + buckets[bucket].offset];
        if(ent.key == key) {
            return &ent.value;
        }
    }
    return nullptr;
}

nbl::JsonElement* nbl::JsonArray::get(usize index) {
    if(index >= count) return nullptr;
    return elements + index;
}

const nbl::JsonElement* nbl::JsonArray::get(usize index) const {
    if(index >= count) return nullptr;
    return elements + index;
}

nbl::JsonObject nbl::JsonArray::getObject(usize index) const {
    const JsonElement* element = get(index);
    if(element == nullptr || element->type() != JsonType::OBJECT) return {};
    return element->asObject();
}

nbl::JsonObject nbl::JsonArray::getObject(usize index, JsonObject defaultValue) const {
    const JsonElement* element = get(index);
    if(element == nullptr || element->type() != JsonType::OBJECT) return defaultValue;
    return element->asObject();
}

nbl::JsonArray nbl::JsonArray::getArray(usize index, JsonArray defaultValue) const {
    const JsonElement* element = get(index);
    if(element == nullptr || element->type() != JsonType::ARRAY) return defaultValue;
    return element->asArray();
}

nbl::StringView nbl::JsonArray::getString(usize index, StringView defaultValue) const {
    const JsonElement* element = get(index);
    if(element == nullptr || element->type() != JsonType::STRING) return defaultValue;
    return element->asString();
}

nbl::JsonNumber nbl::JsonArray::getNumber(usize index, JsonNumber defaultValue) const { 
    const JsonElement* element = get(index);
    if(element == nullptr || element->type() != JsonType::NUMBER) return defaultValue;
    return element->asNumber();
}

bool nbl::JsonArray::getBoolean(usize index, bool defaultValue) const {
    const JsonElement* element = get(index);
    if(element == nullptr || element->type() != JsonType::BOOLEAN) return defaultValue;
    return element->asBoolean();
}


nbl::JsonObject nbl::JsonObject::getObject(StringView key, JsonObject defaultValue) const {
    const JsonElement* element = get(key);
    if(element == nullptr || element->type() != JsonType::OBJECT) return defaultValue;
    return element->asObject();
}

nbl::JsonArray nbl::JsonObject::getArray(StringView key, JsonArray defaultValue) const {
    const JsonElement* element = get(key);
    if(element == nullptr || element->type() != JsonType::ARRAY) return defaultValue;
    return element->asArray();
}

nbl::StringView nbl::JsonObject::getString(StringView key, StringView defaultValue) const {
    const JsonElement* element = get(key);
    if(element == nullptr || element->type() != JsonType::STRING) return defaultValue;
    return element->asString();
}

nbl::JsonNumber nbl::JsonObject::getNumber(StringView key, JsonNumber defaultValue) const { 
    const JsonElement* element = get(key);
    if(element == nullptr || element->type() != JsonType::NUMBER) return defaultValue;
    return element->asNumber();
}

bool nbl::JsonObject::getBoolean(StringView key, bool defaultValue) const {
    const JsonElement* element = get(key);
    if(element == nullptr || element->type() != JsonType::BOOLEAN) return defaultValue;
    return element->asBoolean();
}



nbl::Result nbl::JsonObject::Builder::add(StringView key, JsonElement element) {

    JsonObject::Entry* entry;
    NBL_CHECK(_scratch()->allocate(&entry));
    
    NBL_CHECK(StringBuilder(_pAllocator).append(key).buildNullTerminated(&entry->key));
    NBL_CHECK(_deepCopy(element, &entry->value, _pAllocator));
    _mEntryCount++;

    return nbl::Result::SUCCESS;
}
nbl::Result nbl::JsonObject::Builder::addObject(StringView key, nbl::FunctionRef<Result(JsonObject::Builder&)> callback, usize buckets) {

    JsonObject::Entry* entry = nullptr;
    JsonObject obj{};
    NBL_CHECK(_scratch()->allocate(&entry));
    
    NBL_CHECK(StringBuilder(_pAllocator).append(key).buildNullTerminated(&entry->key));

    Builder child(_pAllocator, _scratch());
    NBL_CHECK(callback(child));
    NBL_CHECK(child.build(&obj, buckets));
    entry->value = obj;
    _mEntryCount++;

    return nbl::Result::SUCCESS;

}
nbl::Result nbl::JsonObject::Builder::addArray(StringView key, nbl::FunctionRef<Result(JsonArray::Builder&)> callback) {

    JsonObject::Entry* entry = nullptr;
    JsonArray arr{};
    NBL_CHECK(_scratch()->allocate(&entry));
    
    NBL_CHECK(StringBuilder(_pAllocator).append(key).buildNullTerminated(&entry->key));

    JsonArray::Builder child(_pAllocator, _scratch());
    NBL_CHECK(callback(child));
    NBL_CHECK(child.build(&arr));
    entry->value = arr;
    _mEntryCount++;

    return nbl::Result::SUCCESS;
}
nbl::Result nbl::JsonObject::Builder::build(nbl::JsonObject* out, usize bucketCount) {

    if(out == nullptr) return nbl::Result::INVALID_ARGUMENT;
    *out = JsonObject{};

    if(_mEntryCount == 0) return nbl::Result::SUCCESS;
    if(bucketCount == 0) {
        bucketCount = _mEntryCount * 1.5f;
    }

    NBL_CHECK(_pAllocator->allocate(&out->entries, _mEntryCount));

    out->entryCount = _mEntryCount;
    out->bucketCount = bucketCount;
    NBL_CHECK(_pAllocator->allocate(&out->buckets, out->bucketCount));

    if(bucketCount == 1) {
        out->buckets[0].count = _mEntryCount;
        out->buckets[0].offset = 0;
        memcpy(out->entries, _pBegin, _mEntryCount * sizeof(Entry));
    } else {

        for(usize i = 0 ; i < _mEntryCount ; i++) {
            usize bucketIndex = _pBegin[i].key.hash() % out->bucketCount;
            out->buckets[bucketIndex].count++;
        }

        usize runningOffset = 0;
        for(usize i = 0 ; i < out->bucketCount ; i++) {
            out->buckets[i].offset = runningOffset;
            runningOffset += out->buckets[i].count;
        }

        usize* bucketCounts;
        NBL_CHECK(_scratch()->allocate(&bucketCounts, out->bucketCount));

        for(usize i = 0 ; i < _mEntryCount ; i++) {
            usize bucketIndex = _pBegin[i].key.hash() % out->bucketCount;

            usize entryIndex = out->buckets[bucketIndex].offset + bucketCounts[bucketIndex];
            bucketCounts[bucketIndex]++;

            out->entries[entryIndex] = _pBegin[i];
        }
    }

    NBL_CHECK(_scratch()->popToPtr(_pBegin));
    
    return nbl::Result::SUCCESS;
}
nbl::Result nbl::JsonObject::Builder::create(nbl::LinearAllocator* allocator, nbl::JsonObject::Builder* out) {

    NBL_CHECK(nbl::Arena::create(1024 * 1024 * 32, &out->_mScratch));
    out->_pAllocator = allocator;
    out->_pParentScratch = nullptr;
    out->_pBegin = static_cast<JsonObject::Entry*>(out->_mScratch.begin());
    out->_mEntryCount = 0;
    return Result::SUCCESS;
}

nbl::Result nbl::JsonArray::Builder::add(JsonElement element) {

    JsonElement* entry;
    NBL_CHECK(_scratch()->allocate(&entry));
    NBL_CHECK(_deepCopy(element, entry, _pAllocator));
    _mElementCount++;

    return nbl::Result::SUCCESS;

}
nbl::Result nbl::JsonArray::Builder::addObject(nbl::FunctionRef<Result(JsonObject::Builder&)> callback) {

    JsonElement* entry = nullptr;
    JsonObject obj{};
    NBL_CHECK(_scratch()->allocate(&entry));
    
    JsonObject::Builder child(_pAllocator, _scratch());
    NBL_CHECK(callback(child));
    NBL_CHECK(child.build(&obj));
    *entry = obj;
    _mElementCount++;

    return nbl::Result::SUCCESS;

}
nbl::Result nbl::JsonArray::Builder::addArray(nbl::FunctionRef<Result(JsonArray::Builder&)> callback) {

    JsonElement* entry = nullptr;
    JsonArray arr{};
    NBL_CHECK(_scratch()->allocate(&entry));
    
    Builder child(_pAllocator, _scratch());
    NBL_CHECK(callback(child));
    NBL_CHECK(child.build(&arr));
    *entry = arr;
    _mElementCount++;

    return nbl::Result::SUCCESS;
}
nbl::Result nbl::JsonArray::Builder::build(nbl::JsonArray* out) {

    if(out == nullptr) return nbl::Result::INVALID_ARGUMENT;
    *out = JsonArray{};

    if(_mElementCount == 0) return nbl::Result::SUCCESS;

    NBL_CHECK(_pAllocator->allocate(&out->elements, _mElementCount));
    memcpy(out->elements, _pBegin, _mElementCount * sizeof(JsonElement));
    out->count = _mElementCount;

    NBL_CHECK(_scratch()->popToPtr(_pBegin));

    return nbl::Result::SUCCESS;
}
nbl::Result nbl::JsonArray::Builder::create(nbl::LinearAllocator* allocator, nbl::JsonArray::Builder* out) {

    NBL_CHECK(nbl::Arena::create(1024 * 1024 * 32, &out->_mScratch));
    out->_pAllocator = allocator;
    out->_pParentScratch = nullptr;
    out->_mElementCount = 0;
    out->_pBegin = static_cast<JsonElement*>(out->_mScratch.end());
    return Result::SUCCESS;
}
