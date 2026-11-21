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
#include <Data/Common.hpp>
#include <Data/FilterModel.hpp>

namespace nbl {

struct Game;
struct GameLookupResult;

typedef nbl::FunctionRef<Result(const Game&)> GameCallback;
typedef nbl::FunctionRef<Result(const GameLookupResult&)> GameLookupCallback;
typedef nbl::FunctionRef<Result(i64 gameId, StringView platformName, StringView platformId)> GameExternalIdCallback;


struct GameLookupResult {
    i64 id;
    StringView title;
    u64 releaseDate;
    f64 rating;
    i64 poster;
    i64 icon;

    union {
        struct {
            u64 numAchievements;
            u64 numUnlockedAchievements;
        };
        u64 firstAdded;
        u64 playTime;
        u64 playCount;
        u64 lastPlayed;
    };

    const Filter& filter;
};


struct Game {
    i64 id;
    StringView title;
    StringView description;
    i64 banner;
    i64 poster;
    i64 icon;
    i64 logo;
    LogoPosition logoPosition;
    u64 releaseDate;
    f64 rating;
    u64 previousPlayTime;
    u64 previousPlayCount;
    u64 lastPlayedDeleted;
    u64 added;
    Optional<u32> colorOverride;

    u64 totalPlayTime;
    u64 totalPlayCount;
    u64 lastPlayed;
    u64 installationCount;


    static Result add(DatabaseConnection* connection, const Game& meta, i64* outGameId);
    static Result remove(DatabaseConnection* connection, i64 gameId);
    static Result update(DatabaseConnection* connection, const Game& meta);

    /**
     * \brief Looks up a game in the database, without computing aggregates for it (totalPlayTime, etc.)
     */
    static Result get(DatabaseConnection* connection, i64 gameId, GameCallback callback);

    /**
     * \brief Looks up a game in the database, and also computes aggregates for it (totalPlayTime, etc.)
     */
    static Result getExtended(DatabaseConnection* connection, i64 gameId, GameCallback callback);


    static Result enumerateExternalIds(DatabaseConnection* connection, i64 gameId, GameExternalIdCallback callback);
    static Result getExternalId(DatabaseConnection* connection, i64 gameId, StringView platformName, GameExternalIdCallback callback);
    static Result setExternalId(DatabaseConnection* connection, i64 gameId, StringView platformName, StringView platformId);
    static Result removeExternalId(DatabaseConnection* connection, i64 gameId, StringView platformName);
    static Result clearExternalIds(DatabaseConnection* connection, i64 gameId);


    static Result getLastPlayed(DatabaseConnection* connection, i64 gameId, u64* outTime);
    static Result getFirstAdded(DatabaseConnection* connection, i64 gameId, u64* outTime);
    static Result getPlayCount(DatabaseConnection* connection, i64 gameId, u64* outCount);
    static Result getPlayTime(DatabaseConnection* connection, i64 gameId, u64* outTime);

    static Result lookupForFilter(
        DatabaseConnection* connection, 
        i64 filterId, 
        i64 deviceId, 
        u64 offset, 
        u64 limit, 
        GameLookupCallback callback);

    static Result lookup(
        DatabaseConnection* connection, 
        const Filter& criteria,
        GameLookupCallback callback);

};

}
