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
 * \file Plugin/PluginInitContext.cpp
 * \author jwall64
 * \brief Implementation of Plugin/PluginInitContext.hpp
 */

#include <Plugin/PluginInitContext.hpp>

#include <Plugin/InstallationType.hpp>
#include <Plugin/PluginContext.hpp>
#include <Plugin/AccountProvider.hpp>
#include <Plugin/MetadataProvider.hpp>
#include <Platform/Allocator.hpp>
#include <Data/Database.hpp>
#include <Data/Model.hpp>
#include <Codec/Json.hpp>
#include <Plugin/Field.hpp>

bool nbl::PluginInitContext::_verifyId(nbl::StringView id) {
    for(char c : id) {
        if((c < 'a' || c > 'z') && (c < '0' || c > '9') && c != '_') {
            return false;
        }
    }
    return true;
}

nbl::Result nbl::PluginInitContext::create(Registry::Builder* builder, LoadedPlugin* plugin, PluginContext* context, StringView installFolder, PluginInitContext* outCtx) {

    outCtx->_pBuilder = builder;
    outCtx->_pPlugin = plugin;
    outCtx->_pContext = context;
    outCtx->_mInstallFolder = installFolder;

    return nbl::Result::SUCCESS;
}

nbl::Result nbl::PluginInitContext::getStoredFieldsObject(JsonObject* outValues, StringView* outVersion, LinearAllocator* allocator) {
    *outValues = {};
    *outVersion = {};
    nbl::Result res = nbl::getPluginFields(_pContext->databaseConnection(), getMeta()->id, [&](const PluginFields& fields) -> nbl::Result {
        JsonElement e{};
        NBL_CHECK(nbl::parseJson(fields.fieldsJson, allocator, &e));
        if(e.type() != JsonType::OBJECT) return nbl::Result::FORMAT_ERROR;
        *outValues = e.asObject();
        NBL_CHECK(nbl::StringBuilder(allocator).append(fields.version).buildNullTerminated(outVersion));
        return nbl::Result::SUCCESS;
    });
    if(res == Result::NOT_FOUND) return nbl::Result::SUCCESS;
    return res;
}

nbl::Result nbl::PluginInitContext::getStoredFieldsVersion(StringView* outVersion, LinearAllocator* allocator) {
    *outVersion = {};
    nbl::Result res = nbl::getPluginFieldsVersion(_pContext->databaseConnection(), getMeta()->id, [&](const PluginFields& fields) -> nbl::Result {
        NBL_CHECK(nbl::StringBuilder(allocator).append(fields.version).buildNullTerminated(outVersion));
        return nbl::Result::SUCCESS;
    });
    if(res == Result::NOT_FOUND) return nbl::Result::SUCCESS;
    return res;
}

nbl::Result nbl::PluginInitContext::getStoredFieldValues(Span<Field> fields, Span<FieldValue>* outValues, LinearAllocator* allocator) {

    JsonObject obj{};
    StringView ver{};
    NBL_CHECK(getStoredFieldsObject(&obj, &ver, allocator));

    return decodeFieldValues(_pContext, fields, obj, outValues, allocator);
}

nbl::Result nbl::PluginInitContext::storeFieldValues(Span<Field> fields, Span<FieldValue> values) {

    AllocationFrame frame(Arena::threadLocal());
    NBL_CHECK(encodeFieldValues(_pContext, fields, values, frame.writer()));

    StringView fieldsJson = StringView(static_cast<char*>(frame.begin()), frame.index());
    return nbl::setPluginFields(_pContext->databaseConnection(), getMeta()->id, fieldsJson, getMeta()->version);
}

nbl::Result nbl::PluginInitContext::setFields(Span<Field> fields, void(*pfnUpdateCallback)(Span<Field> fields, Span<FieldValue> newValues, void* userData), void* userData) {

    _pPlugin->fields = fields;
    _pPlugin->pfnUpdateCallback = pfnUpdateCallback;
    _pPlugin->userData = userData;

    return nbl::Result::SUCCESS;
}
