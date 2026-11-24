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

#include <Core/Types.hpp>
#include <Core/Result.hpp>
#include <Platform/Logging.hpp>
#include <Plugin/AchievementProvider.hpp>
#include <Plugin/PluginInitializer.hpp>
#include <Codec/Json.hpp>
#include <Platform/Allocator.hpp>
#include <Platform/DynamicLibrary.hpp>
#include <Platform/Process.hpp>
#include <Platform/Arena.hpp>
#include <Platform/File.hpp>
#include <Utility/StringBuilder.hpp>
#include <Plugin/PluginContext.hpp>
#include <Core/Span.hpp>
#include <Core/String.hpp>
#include <Data/Model.hpp>


#include "Constants.hpp"


namespace {


char _wrapperPathBuf[4096]{};
nbl::StringView _clientPath{};


nbl::Result _getAchievements(nbl::PluginContext* context, i32 appId, nbl::JsonObject* out, nbl::LinearAllocator* allocator) {

    char appIdBuf[16];
    nbl::InPlaceAllocator alloc(appIdBuf, 16);
    nbl::StringView appIdStr;
    NBL_CHECK(nbl::StringBuilder(&alloc).append(appId).build(&appIdStr));

    nbl::ProcessHandle handle{};
    NBL_CHECK(nbl::ProcessHandle::spawn(_clientPath, {}, &handle, {
        .outputMode = nbl::ProcessOutputMode::PIPE,
        .environment = nbl::Array {
            nbl::EnvVar {
                .variable = "SteamAppId",
                .value = appIdStr
            }
        }
    }));

    nbl::JsonElement element{};
    nbl::Result parseResult = nbl::parseJson(handle.stdoutReader(), allocator, &element);

    i32 exitCode = 0;
    NBL_CHECK(handle.wait(&exitCode));


    if(exitCode != 0) {
        return exitCode;
    }
    NBL_CHECK(parseResult);

    if(element.asObject().getBoolean("success")) {
        *out = element.asObject();
    } else {
        i32 code = element.asObject().getNumber("code").asInt();
        if(code == -3) { // -3 means Steam is not running.
            return nbl::Result::NO_CHANGE;
        } else {
            context->pushNotification(nbl::NotificationType::ERROR, "Steam Client Error", element.asObject().getString("error"));
            return nbl::Result::IO_ERROR;
        }
    }

    return nbl::Result::SUCCESS;
}


nbl::Result _updateAchievementState(nbl::PluginContext* context, const nbl::Game& game, nbl::AchievementProviderStatus status, nbl::Span<nbl::FieldValue> values, nbl::AchievementHelper& helper, nbl::StringView externalId, void* userData) {

    if(externalId.empty()) return nbl::Result::NO_CHANGE;

    switch(status) {
        default:
        case nbl::AchievementProviderStatus::REMOVED_BY_PROVIDER:
        case nbl::AchievementProviderStatus::REMOVED_BY_USER:
            return nbl::Result::NO_CHANGE;
        case nbl::AchievementProviderStatus::ADDED_BY_PROVIDER:
        case nbl::AchievementProviderStatus::ADDED_BY_USER:
            NBL_CHECK(helper.dropUnknownSets(nbl::Span<nbl::StringView>(&externalId, 1)));

            // Intentional fall-through
        case nbl::AchievementProviderStatus::NOT_ADDED: {
            if(!externalId.empty()) {
                NBL_CHECK(helper.addAchievementSet(externalId, "Steam", 0));
                NBL_CHECK(helper.setProviderStatus(nbl::AchievementProviderStatus::ADDED_BY_PROVIDER));
            }
            return nbl::Result::SUCCESS;
        }
    }
}


nbl::Result _updateAchievements(nbl::PluginContext* context, const nbl::AchievementSet& set, nbl::AchievementHelper& helper, nbl::AchievementUpdateReason reason, void* userData) {

    nbl::JsonNumber appId{};
    NBL_CHECK(nbl::parseJsonNumber(nbl::StringReader(set.providerId), &appId));

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::JsonObject obj{};
    nbl::Result res = _getAchievements(context, appId.asInt(), &obj, &frame);
    if(res == nbl::Result::NO_CHANGE) return nbl::Result::NO_CHANGE;
    if(res.error()) return res;

    u64 latestUnlock = obj.getNumber("latest_unlock").asUint();

    if(latestUnlock <= set.lastUpdated) {
        return nbl::Result::NO_CHANGE;
    }

    nbl::StringView storedHash(reinterpret_cast<const char*>(set.providerData.data()), set.providerData.length());
    nbl::StringView generatedHash = obj.getString("revision_hash");

    bool schemaUpdated = set.lastUpdated == 0;
    if(generatedHash != storedHash) {
        schemaUpdated = true;
        helper.markAchievementsStale(set.id);
    }

    bool updated = schemaUpdated;


    for(nbl::JsonElement& e : obj.getArray("achievements")) {

        u64 unlockTime = e.asObject().getNumber("unlock_time").asUint();

        if(unlockTime == 0 && e.asObject().getBoolean("unlocked")) {
            unlockTime = 1259625600000; 
        }

        if(schemaUpdated) {

            nbl::AchievementFlags flags = 0;
            if(e.asObject().getBoolean("hidden")) {
                flags |= nbl::AchievementFlag::SPOILER;
            }

            helper.addOrUpdateAchievementWithProgress(
                set.id,
                e.asObject().getNumber("number").asUint(),
                e.asObject().getString("name"),
                e.asObject().getString("display_name"),
                e.asObject().getString("description"),
                e.asObject().getString("icon"),
                e.asObject().getString("icon_gray"),
                e.asObject().getNumber("global_unlock_rate").asFloat(),
                0, 0,
                unlockTime,
                flags,
                nbl::steam::BORDER_COLOR);

        }
        else if(unlockTime > set.lastUpdated) {
            
            helper.setAchievementProgress(
                    set.id, 
                    e.asObject().getString("name"),
                    e.asObject().getNumber("global_unlock_rate").asFloat(), 
                    0, 0, // Doesn't seem to be obtainable without knowing which stats correspond to which achievements :/
                    unlockTime, 
                    nbl::steam::BORDER_COLOR);

            updated = true;
        }
    }

    if(updated) {
        if(schemaUpdated) {
            helper.dropStaleAchievements(set.id);
            helper.updateSetProviderData(set.id, nbl::ConstSpan<u8>(
                reinterpret_cast<const u8*>(generatedHash.data()),
                generatedHash.length()
            ));
        }
        helper.bumpSetUpdateTime(set.id, latestUnlock);
        return nbl::Result::SUCCESS;
    }

    return nbl::Result::NO_CHANGE;
}

nbl::AchievementProvider _achProvider {
    .id = "steam",
    .name = "Steam",
    .requestedExternalId = "steam",
    .fields = {},
    .pfnUpdateAchievementState = _updateAchievementState,
    .pfnUpdateAchievements = _updateAchievements,
    .userData = nullptr
};


}


extern "C" i32 NBL_EXPORT initPlugin(nbl::PluginInitContext* context) {
    NBL_INFO("Initializing Steam plugin...");

    {
        nbl::InPlaceAllocator alloc(_wrapperPathBuf, 4096);
        NBL_CHECK(nbl::StringBuilder(&alloc).append(context->installFolder()).append(NBL_PATH_SEPARATOR).append("AchievementClient").append(nbl::executableFileExt()).buildNullTerminated(&_clientPath));
    }

    context->registerObject(&_achProvider);

    return 0; 
}
