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
 * \file Plugin/GameProvider.hpp
 * \author jwall64
 * \brief Plugin-registerable object which gets metadata for games, grouped by title
 */

#pragma once

#include <Core/String.hpp>
#include <Plugin/PluginContext.hpp>
#include <Data/Model.hpp>
#include <Data/GameModel.hpp>

#include <Core/FunctionRef.hpp>

namespace nbl {

struct OnlineGameLookupInfo {
    enum ParamMeaning {
        TITLE,
        ID
    };
    StringView param;
    ParamMeaning meaning;
};

struct OnlineGameMeta {
    StringView title;
    StringView description;
    StringView posterUrl;
    StringView iconUrl;
    StringView bannerUrl;
    StringView logoUrl;
    LogoPosition logoPosition;
    u64 releaseDate;
    f64 rating;
    Span<StringView> tags;
    Span<StringView> platformNames;
    Span<StringView> platformIds;
};


typedef nbl::FunctionRef<Result(const OnlineGameMeta&)> OnlineGameCallback;

struct GameProvider {

    StringView id;
    StringView name;
    bool (*pfnCanProvide)(PluginContext* context, const OnlineGameLookupInfo& info, void* userData);
    Result (*pfnProvide)(PluginContext* context, const OnlineGameLookupInfo& info, OnlineGameCallback callback, void* userData);
    void* userData;

    bool canProvide(PluginContext* context, OnlineGameLookupInfo info) const {
        if(!pfnCanProvide) return false;
        return pfnCanProvide(context, info, userData);
    }

    Result provide(PluginContext* context, OnlineGameLookupInfo info, OnlineGameCallback callback) const {
        if(!pfnProvide) return nbl::Result::PLUGIN_ERROR;
        return pfnProvide(context, info, callback, userData);
    }
};

}
