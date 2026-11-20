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
 * \file Plugin.cpp
 * \author jwall64
 * \brief RetroAchievements plugin entrypoint
 */

#include <Codec/Json.hpp>
#include <Core/String.hpp>
#include <Core/Result.hpp>
#include <Data/Model.hpp>
#include <Data/OnlineAccountModel.hpp>
#include <Data/AchievementModel.hpp>
#include <Data/Database.hpp>
#include <Network/HttpClient.hpp>
#include <Platform/Allocator.hpp>
#include <Platform/Time.hpp>
#include <Utility/StringBuilder.hpp>
#include <Plugin/Field.hpp>
#include <Plugin/PluginInitializer.hpp>
#include <Plugin/AchievementProvider.hpp>
#include <Plugin/AccountProvider.hpp>

#include "Constants.hpp"
#include "Utility.hpp"
#include "Profile.hpp"

namespace {


nbl::HttpClient _httpClient;
nbl::ra::UserProfile _profile;

u8 _localBuffer[4096];
nbl::InPlaceAllocator _localAlloc(_localBuffer, 4096);

nbl::StringView _db_path;


nbl::Array _accountFields {
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "username",
        .name = "Username"
    },
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "api_key",
        .name = "API Key",
        .flags = nbl::FieldFlag::PASSWORD | 0
    },
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "ulid",
        .name = "",
        .flags = nbl::FieldFlag::HIDDEN | 0
    },
    nbl::Field {
        .type = nbl::FieldType::INTEGER,
        .key = "points",
        .name = "",
        .flags = nbl::FieldFlag::HIDDEN | 0
    },
    nbl::Field {
        .type = nbl::FieldType::INTEGER,
        .key = "true_points",
        .name = "",
        .flags = nbl::FieldFlag::HIDDEN | 0
    }
};


nbl::Result _login(nbl::PluginContext* context, nbl::Span<nbl::FieldValue> fields, nbl::LoginTracker* tracker, void* userData) {
    tracker->postStatus("Downloading profile...");
    NBL_CHECK(nbl::ra::downloadProfile(&_httpClient, fields[1].stringValue.raw, fields[0].stringValue.raw, &_localAlloc, &_profile));

    fields[2].stringValue.raw = _profile.ulid;
    fields[3].uintegerValue = _profile.points;
    fields[4].uintegerValue = _profile.truePoints;

    tracker->complete(context, _profile.username, _profile.pfp, nbl::OnlineAccountFlag::IMAGE_ICON | 0, fields);
    return nbl::Result::SUCCESS;
}

nbl::Result _refresh(nbl::PluginContext* context, const nbl::OnlineAccount& account, nbl::LoginTracker* tracker, void* userData) {

    nbl::StringView fields = account.fields;

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::JsonElement element{};
    NBL_CHECK(nbl::parseJson(account.fields, &frame, &element));

    nbl::Span<nbl::FieldValue> values{};

    nbl::decodeFieldValues(context, _accountFields, element.asObject(), &values, &frame);

    _localAlloc.popTo(nbl::alignUp(_db_path.length() + 1, 8));
    NBL_CHECK(nbl::ra::downloadProfile(&_httpClient, values[1].stringValue.raw, values[0].stringValue.raw, &_localAlloc, &_profile));

    values[2].stringValue.raw = _profile.ulid;
    values[3].uintegerValue = _profile.points;
    values[4].uintegerValue = _profile.truePoints;

    tracker->complete(context, _profile.username, _profile.pfp, nbl::OnlineAccountFlag::IMAGE_ICON | 0, values);
    return nbl::Result::SUCCESS;

}

nbl::Result _logout(nbl::PluginContext* context, i64 accountId, void* userData) {

    _profile.ulid = {};
    _profile.apiKey = {};
    _profile.username = {};

    _localAlloc.popTo(nbl::alignUp(_db_path.length() + 1, 8));

    return nbl::Result::SUCCESS;
}

nbl::AccountProvider _provider {
    .id = "ra",
    .name = "RetroAchievements",
    .fields = _accountFields,
    .maxAccounts = 1,
    .pfnLogin = _login,
    .pfnRefresh = _refresh,
    .pfnLogout = _logout,
    .userData = nullptr,
};


struct _ProviderData {
    u64 latestUnlock;
    u64 numPlayers;
    char completionPercents[16];
    bool completed;
    bool hardcore;

    nbl::StringView userCompletion() {
        return nbl::StringView(completionPercents);
    }
    void setUserCompletion(nbl::StringView completion) {
        NBL_ASSERT(completion.length() < 8);
        memcpy(completionPercents, completion.data(), completion.length());
    }
    nbl::StringView hardcoreCompletion() {
        return nbl::StringView(completionPercents + 8);
    }
    void setHardcoreCompletion(nbl::StringView completion) {
        NBL_ASSERT(completion.length() < 8);
        memcpy(completionPercents + 8, completion.data(), completion.length());
    }
};

nbl::Result _updateAchievementState(nbl::PluginContext* context, const nbl::Game& game, nbl::AchievementProviderStatus status, nbl::Span<nbl::FieldValue> values, nbl::AchievementHelper& helper, nbl::StringView externalId, void* userData) {

    nbl::Array<nbl::FieldValue, 1> backupValues{};

    nbl::DatabaseConnection conn{};
    if(status == nbl::AchievementProviderStatus::NOT_ADDED) {

        if(externalId.empty()) {
            return helper.setProviderStatus(nbl::AchievementProviderStatus::REMOVED_BY_PROVIDER);
        }

        NBL_CHECK(nbl::DatabaseConnection::open(_db_path, &conn));

        nbl::PreparedStatement mapGameStmt;
        NBL_CHECK(conn.prepare("SELECT game.id FROM game JOIN game_mapping ON game_mapping.game_id = game.id WHERE game_mapping.igdb_id = ?1;", &mapGameStmt));

        mapGameStmt.bindStringOrNull(1, externalId);

        nbl::Optional<i64> raId{};
        NBL_CHECK(conn.execute(&mapGameStmt, [&](i32 columns, nbl::DataValue* values, const char** names, i64 lastInsertRowId) -> nbl::Result {
            if(columns != 1) return nbl::Result::UNEXPECTED_ERROR;
            raId = values[0].intValueOrNull();
            return nbl::Result::SUCCESS;
        }));

        if(!raId.hasValue()) {
            return helper.setProviderStatus(nbl::AchievementProviderStatus::REMOVED_BY_PROVIDER);
        }

        values = backupValues;
        backupValues[0].integerValue = *raId;
        NBL_CHECK(helper.upsertProviderState(values));

    } else if(status != nbl::AchievementProviderStatus::ADDED_BY_USER 
           && status != nbl::AchievementProviderStatus::ADDED_BY_PROVIDER) {

        return nbl::Result::SUCCESS;
    }


    nbl::Array<nbl::StringView, 16> setIds{};
    usize numSets = 0;

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    usize index = 0;


    i64 gameId = values[0].integerValue;
    if(gameId > 0) {

        nbl::StringView idStr;
        NBL_CHECK(nbl::StringBuilder(&frame).append(gameId).buildNullTerminated(&idStr));
        setIds[numSets++] = idStr;

        NBL_CHECK(helper.addOrUpdateAchievementSet(idStr, "Base Set", 0, false, _ProviderData{}));
        
        if(!conn.isValid()) {
            NBL_CHECK(nbl::DatabaseConnection::open(_db_path, &conn));
        }

        nbl::PreparedStatement getSubsetsStmt;
        NBL_CHECK(conn.prepare("SELECT subset.id, subset.name FROM subset_parent JOIN subset ON subset_parent.subset_id = subset.id WHERE subset_parent.game_id = ?1 LIMIT 15;", &getSubsetsStmt));
        NBL_CHECK(getSubsetsStmt.bindInt(1, gameId));

        NBL_CHECK(conn.execute(&getSubsetsStmt, [&](i32 columns, nbl::DataValue* values, const char**, i64) -> nbl::Result {

            if(columns != 2) return nbl::Result::UNEXPECTED_ERROR;

            nbl::StringView subsetIdStr;
            NBL_CHECK(nbl::StringBuilder(&frame).append(values[0].intValue).buildNullTerminated(&subsetIdStr));
            setIds[numSets++] = subsetIdStr;

            return helper.addOrUpdateAchievementSet(subsetIdStr, values[1].textValue, 0, true, _ProviderData{});
        }));
    }

    NBL_CHECK(helper.dropUnknownSets(setIds.span().slice(0, numSets)));
    return nbl::Result::SUCCESS;
}

nbl::Result _updateAchievements(nbl::PluginContext* context, const nbl::AchievementSet& set, nbl::AchievementHelper& helper, nbl::AchievementUpdateReason reason, void* userData) {

    if(!_profile.isValid()) return nbl::Result::NO_CHANGE;

    if(reason == nbl::AchievementUpdateReason::GAME_PAGE_OPENED) {
        u64 now = nbl::getCurrentTime();
        if(now - set.lastUpdated < (1000 * 60 * 60)) {
            return nbl::Result::NO_CHANGE;
        }
    }

    _ProviderData ud{};
    set.getProviderData(&ud);

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    nbl::StringView url;
    NBL_CHECK(nbl::StringBuilder(&frame)
        .append(RA_API_URL)
        .append("/API_GetGameInfoAndUserProgress.php?g=")
        .append(set.providerId)
        .append("&u=")
        .append(_profile.ulid)
        .append("&y=")
        .append(_profile.apiKey)
        .buildNullTerminated(&url));

    char* end = static_cast<char*>(frame.end());
    NBL_CHECK(_httpClient.awaitRequest(url, frame.writer()));

    nbl::StringView response(end, static_cast<char*>(frame.end()) - end);
    nbl::JsonElement responseJson{};

    NBL_CHECK(nbl::parseJson(response, &frame, &responseJson));
    if(responseJson.type() != nbl::JsonType::OBJECT) return nbl::Result::FORMAT_ERROR;

    u64 numPlayers = responseJson.asObject().getNumber("NumDistinctPlayers").asUint();

    nbl::JsonObject obj = responseJson.asObject().getObject("Achievements");
    if(obj.entryCount == 0) {
        nbl::log(nbl::LogLevel::DEBUG, "Achievement set: ", set.providerId, " has no achievements");
        return nbl::Result::NO_CHANGE;
    }

    bool updated = false;

    u64 totalPlayers = responseJson.asObject().getNumber("NumDistinctPlayers").asUint();
    if(totalPlayers != ud.numPlayers) {

        updated = true;
        ud.numPlayers = totalPlayers;

    } else {

        nbl::StringView userCompletion = responseJson.asObject().getString("UserCompletion");
        nbl::StringView hardcoreCompletion = responseJson.asObject().getString("UserCompletionHardcore");

        if(userCompletion != ud.userCompletion() || hardcoreCompletion != ud.hardcoreCompletion()) {
        
            updated = true;
            ud.setUserCompletion(userCompletion);
            ud.setHardcoreCompletion(hardcoreCompletion);
        }
    }

    u64 numComplete = 0;
    u64 numHardcore = 0;

    u64 latestUpdate = 0;
    for(nbl::JsonObject::Entry& ent : obj) {

        nbl::StringView id = ent.key;
        if(ent.value.type() != nbl::JsonType::OBJECT) return nbl::Result::FORMAT_ERROR;

        u64 time;
        NBL_CHECK(nbl::ra::parseDate(ent.value.asObject().getString("DateModified"), &time));

        if(latestUpdate < time) latestUpdate = time;

        nbl::StringView unlockDate = ent.value.asObject().getString("DateEarnedHardcore");
        bool hardcore = unlockDate.length() > 0;
        bool unlocked = hardcore || (unlockDate = ent.value.asObject().getString("DateEarned")).length() > 0;

        if(unlocked) numComplete++;
        if(hardcore) numHardcore++;

        u64 unlockTime = 0;
        if(unlockDate.length() > 0) {
            NBL_CHECK(nbl::ra::parseDate(unlockDate, &unlockTime));
        }


        if(set.lastUpdated < time) {

            nbl::AchievementFlags flags = 0;
            nbl::StringView type = ent.value.asObject().getString("Type");
            if(type == "progression") {
                flags |= nbl::AchievementFlag::PROGRESSION;
            } else if(type == "win_condition") {
                flags |= nbl::AchievementFlag::WIN_CONDITION;
            } else if(type == "missable") {
                flags |= nbl::AchievementFlag::MISSABLE;
            }
            nbl::AllocationFrame frame(nbl::Arena::threadLocal());

            nbl::StringView badgeUrl{};
            nbl::StringBuilder(&frame)
                .append(RA_MEDIA_URL)
                .append("/Badge/")
                .append(ent.value.asObject().getString("BadgeName"))
                .append(".png")
                .buildNullTerminated(&badgeUrl);

            nbl::StringView lockedBadgeUrl{};
            nbl::StringBuilder(&frame)
                .append(RA_MEDIA_URL)
                .append("/Badge/")
                .append(ent.value.asObject().getString("BadgeName"))
                .append("_lock.png")
                .buildNullTerminated(&lockedBadgeUrl);

            nbl::StringView descriptionWithPoints{};
            nbl::StringBuilder(&frame)
                .append(ent.value.asObject().getString("Description"))
                .append(" (").append(ent.value.asObject().getNumber("Points").asInt()).append(')')
                .buildNullTerminated(&descriptionWithPoints);

            helper.addOrUpdateAchievementWithProgress(
                set.id,
                ent.value.asObject().getNumber("DisplayOrder").asUint(),
                id,
                ent.value.asObject().getString("Title"),
                descriptionWithPoints,
                badgeUrl,
                lockedBadgeUrl,
                100.0 * ent.value.asObject().getNumber(hardcore ? "NumAwardedHardcore" : "NumAwarded").asFloat() / numPlayers,
                0, 0,
                unlockDate.length() == 0 ? 0 : unlockTime, 
                flags,
                hardcore ? RA_BORDER_COLOR_HARDCORE : RA_BORDER_COLOR_CASUAL,
                {}
            );
            updated = true;

        } else if(updated || ud.latestUnlock < unlockTime) { 

            ud.latestUnlock = unlockTime;

            helper.setAchievementProgress(
                    set.id, 
                    id,
                    100.0 * ent.value.asObject().getNumber(hardcore ? "NumAwardedHardcore" : "NumAwarded").asFloat() / numPlayers,
                    0, 0,
                    unlockDate.length() == 0 ? 0 : unlockTime, 
                    hardcore ? RA_BORDER_COLOR_HARDCORE : RA_BORDER_COLOR_CASUAL);

            updated = true;
        }
    }

    if(numHardcore == obj.entryCount && !ud.hardcore) {
        ud.hardcore = true;
        ud.completed = true;
        updated = true;
        helper.setAwardColor(set.id, RA_BORDER_COLOR_HARDCORE);
    } else if(numComplete == obj.entryCount && !ud.completed) {
        ud.completed = true;
        updated = true;
        helper.setAwardColor(set.id, RA_BORDER_COLOR_CASUAL);
    }

    if(updated) {
        helper.updateSetProviderData(set.id, ud);
        helper.bumpSetUpdateTime(set.id, latestUpdate);
        return nbl::Result::SUCCESS;
    }

    return nbl::Result::NO_CHANGE;
}


nbl::Field _listTemplate = {
    .type = nbl::FieldType::INTEGER,
    .integerData = {
        .min = 0,
        .max = INT64_MAX
    }
};

nbl::Array _fields = {
    nbl::Field {
        .type = nbl::FieldType::INTEGER,
        .key = "id",
        .name = "Game ID",
        .integerData = {
            .min = 0,
            .max = INT32_MAX
        }
    }
};


nbl::AchievementProvider _achProvider {
    .id = "ra",
    .name = "RetroAchievements",
    .requestedExternalId = "igdb",
    .fields = _fields,
    .pfnUpdateAchievementState = _updateAchievementState,
    .pfnUpdateAchievements = _updateAchievements,
    .userData = nullptr
};

}


extern "C" i32 initPlugin(nbl::PluginInitContext* context) {  


    nbl::log(nbl::LogLevel::INFO, "Initializing RetroAchievements plugin...");
    NBL_CHECK(context->registerObject(&_provider));
    NBL_CHECK(context->registerObject(&_achProvider));

    NBL_CHECK(nbl::HttpClient::create(1, 1, 0, 0, &_httpClient));

    nbl::StringBuilder(&_localAlloc).append(context->installFolder()).append("/ra.db").buildNullTerminated(&_db_path);
    usize padding = nbl::alignUp(_localAlloc.index(), 8);
    if(padding > 0) {
        _localAlloc.push(padding, nullptr);
    }

    return 0; 
}
