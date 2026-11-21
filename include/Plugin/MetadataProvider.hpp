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
 * \file Plugin/MetadataProvider.hpp
 * \author jwall64
 * \brief Plugin-registerable object which gets metadata for games, grouped by kind
 */


#pragma once

#include <Core/String.hpp>
#include <Core/Optional.hpp>

#include <Core/FunctionRef.hpp>
#include "Plugin/PluginContext.hpp"

namespace nbl {

struct MetadataLookupInfo {
    Optional<StringView> title;
    Optional<u64> releaseDate;
    Span<StringView> platformNames;
    Span<StringView> platformIds;
};

enum MetadataField {
    TITLE,
    DESCRIPTION,
    RELEASE_DATE,
    RATING,
    TAG,
    POSTER,
    BANNER,
    ICON,
    LOGO
};

union MetadataValue {
    StringView string;
    StringView title;
    StringView description;
    u64 releaseDate;
    f64 rating;
    StringView tag;
    StringView poster;
    StringView banner;
    StringView icon;
    StringView logo;
};

typedef nbl::FunctionRef<Result(MetadataField, MetadataValue)> MetadataCallback;

struct MetadataProvider {

    StringView id;
    StringView name;
    bool(*pfnCanProvide)(PluginContext* context, const MetadataLookupInfo& info, void* userData);
    Result(*pfnProvide)(PluginContext* context, const MetadataLookupInfo& info, MetadataCallback callback, void* userData);
    void* userData;

    bool canProvide(PluginContext* context, const MetadataLookupInfo& info) const {
        if(pfnCanProvide == nullptr) return false;
        return pfnCanProvide(context, info, userData);
    }

    Result provide(PluginContext* context, const MetadataLookupInfo& info, MetadataCallback callback) const {
        if(pfnProvide == nullptr) return nbl::Result::PLUGIN_ERROR;
        return pfnProvide(context, info, callback, userData);
    }

};


}
