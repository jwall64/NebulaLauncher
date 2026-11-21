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

#pragma once

#include <Core/Types.hpp>
#include <Core/String.hpp>
#include <Platform/Arena.hpp>
#include <Codec/Json.hpp>
#include <Network/HttpClient.hpp>
#include <Plugin/ProcessTracker.hpp>
#include <Plugin/PluginContext.hpp>
#include <Plugin/PluginInitContext.hpp>
#include <Data/OnlineAccountModel.hpp>

#include "ModLoader.hpp"

namespace nbl::mc {


class MinecraftLauncher {
public:


    Result launch(PluginContext* context, StringView version, const OnlineAccount& account, ModLoader modLoader, StringView workingDir, i64 installationId, ProcessTracker* tracker);

    inline StringView rootFolder() const {
        return _mRootFolder;
    }

    void suggestVersions(PluginContext* context, StringView current, nbl::FunctionRef<void(nbl::StringView)> suggestionConsumer);

    static Result init(StringView rootFolder, MinecraftLauncher* launcher);

private:

    static void _manifestDownloadComplete(void* userData, nbl::Result res, i32 responseCode);

    StringView _mRootFolder;
    HttpClient _mHttpClient;
    Arena _mDataArena;
    Optional<JsonObject> _mVersionManifest;


};



}
