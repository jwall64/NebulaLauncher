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

#include <Data/GameModel.hpp>

namespace nbl {



struct InstallationLookupResult {
    i64 installationId;
    i64 gameId;
    i64 deviceId;

    StringView title;
    StringView gameTitle;
    u64 playTime;
    u64 playCount;
    u64 lastPlayed;
    u64 added;

    i64 poster;
    i64 icon;

    u64 releaseDate;
    f64 rating;

    const Filter& filter;
};

struct Installation;

    typedef nbl::FunctionRef<Result(const InstallationLookupResult& result)> InstallationLookupCallback;
    typedef nbl::FunctionRef<Result(const Installation& installation, const Game& game)> InstallationExtendedCallback;
    typedef nbl::FunctionRef<Result(const Installation& meta)> InstallationMetaCallback;




struct Installation {
    i64 id;
    i64 gameId;
    i64 deviceId;
    StringView title;
    StringView description;
    i64 banner;
    i64 poster;
    i64 icon;
    i64 logo;
    LogoPosition logoPosition;
    StringView type;
    StringView command;
    u64 playTime;
    u64 playCount;
    u64 lastPlayed;
    u64 added;
    Optional<u32> colorOverride;


    static Result add(DatabaseConnection* connection, const Installation& meta, i64* outInstallationId);
    static Result remove(DatabaseConnection* connection, i64 installationId);

    static Result get(DatabaseConnection* connection, i64 installationId, InstallationMetaCallback callback);
    static Result getExtended(DatabaseConnection* connection, i64 installationId, InstallationExtendedCallback callback);

    static Result update(DatabaseConnection* connection, const Installation& meta);

    static Result lookup(
            DatabaseConnection* connection, 
            const Filter& criteria,
            InstallationLookupCallback callback); 

    static Result lookupForGame(
            DatabaseConnection* connection, 
            i64 gameId, 
            i64 deviceId,
            u64 limit, 
            u64 offset, 
            InstallationLookupCallback callback);

    static Result markJustPlayed(DatabaseConnection* connection, i64 installationId);
    static Result bumpPlayTime(DatabaseConnection* connection, i64 installationId, u64 playTimeToAdd);


    static Result lookupForFilter(DatabaseConnection* connection, i64 filterId, i64 deviceId, u64 offset, u64 limit, InstallationLookupCallback callback);
};

}


