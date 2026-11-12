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
 * \file Data/Model.cpp
 * \author jwall64
 * \brief Partial implementation of Data/Model.hpp
 */



#include <Core/Result.hpp>
#include <Data/Database.hpp>
#include <Data/Model.hpp>

#include <Platform/Arena.hpp>
#include <Utility/StringBuilder.hpp>
#include <Codec/Json.hpp>



nbl::StringView nbl::getName(SortBy sortBy) {
    switch(sortBy) {
        default:
        case SortBy::TITLE:
            return "title";
        case SortBy::LAST_PLAYED:
            return "last_played";
        case SortBy::ADDED:
            return "added";
        case SortBy::PLAY_COUNT:
            return "play_count";
        case SortBy::PLAY_TIME:
            return "play_time";
        case SortBy::RELEASE_DATE:
            return "release_date";
        case SortBy::RATING:
            return "rating";
        case SortBy::ACHIEVEMENT_PROGRESS:
            return "achievement_progress";
    }
}

nbl::Result nbl::byName(StringView name, SortBy* outSortBy) {
    for(auto i = 0 ; i < static_cast<i32>(SortBy::_END) ; i++) {
        if(name == getName(static_cast<SortBy>(i))) {
            *outSortBy = static_cast<SortBy>(i);
            return nbl::Result::SUCCESS;
        }
    }
    nbl::log(nbl::LogLevel::WARN, "Unknown installation sort by: ", name);
    return nbl::Result::INVALID_ARGUMENT;
}

nbl::StringView nbl::getName(SortOrder sortOrder) {
    switch(sortOrder) {
        default:
        case SortOrder::ASCENDING:
            return "ascending";
        case SortOrder::DESCENDING:
            return "descending";
    }
}

nbl::Result nbl::byName(StringView name, SortOrder* outSortOrder) {
    if(name == getName(SortOrder::ASCENDING)) {
        *outSortOrder = SortOrder::ASCENDING;
        return nbl::Result::SUCCESS;
    } else if(name == getName(SortOrder::DESCENDING)) {
        *outSortOrder = SortOrder::DESCENDING;
        return nbl::Result::SUCCESS;
    }
    nbl::log(nbl::LogLevel::WARN, "Unknown sort order: ", name);
    return nbl::Result::INVALID_ARGUMENT;
}


nbl::Result nbl::initDatabase(DatabaseConnection* connection) {

    i32 userVersion = 0;

    nbl::PreparedStatement stmt = {};
    NBL_CHECK(connection->prepare("PRAGMA user_version;", &stmt));

    NBL_CHECK(connection->execute(&stmt, [&](i32 argc, DataValue* argv, const char** columnNames, i64 lastInsertRowId) -> nbl::Result {
        userVersion = argv[0].intValue;
        return nbl::Result::SUCCESS;
    }));

    nbl::log(nbl::LogLevel::DEBUG, "Database user_version: ", userVersion);

    if(userVersion == 0) {
        // Database is uninitialized

        nbl::log(nbl::LogLevel::DEBUG, "Initializing database...");
        nbl::StringView initQuery = R"(
PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;

BEGIN EXCLUSIVE TRANSACTION;
CREATE TABLE IF NOT EXISTS game (
    gId INTEGER PRIMARY KEY,
    title TEXT NOT NULL,
    description TEXT,
    poster INTEGER,
    banner INTEGER,
    icon INTEGER,
    logo INTEGER,
    logoPosition INTEGER NOT NULL DEFAULT 0,
    releaseDate INTEGER,
    rating REAL,
    previousPlayTime INTEGER NOT NULL DEFAULT 0,
    previousPlayCount INTEGER NOT NULL DEFAULT 0,
    lastPlayedDeleted INTEGER NOT NULL DEFAULT 0,
    added INTEGER NOT NULL DEFAULT 0,
    colorOverride INTEGER
);
CREATE TABLE IF NOT EXISTS game_external_id (
    gId INTEGER,
    platformName TEXT,
    platformId TEXT,
    UNIQUE (gId, platformName),
    FOREIGN KEY(gId) REFERENCES game(gId) ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS device (
    deviceId INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE
);
CREATE TABLE IF NOT EXISTS installation (
    iId INTEGER PRIMARY KEY,
    gId INTEGER NOT NULL,
    deviceId INTEGER,
    title TEXT,
    description TEXT,
    poster INTEGER,
    banner INTEGER,
    icon INTEGER,
    logo INTEGER,
    logoPosition INTEGER NOT NULL DEFAULT 4,
    type TEXT NOT NULL,
    command TEXT NOT NULL,
    playTime INTEGER NOT NULL DEFAULT 0,
    playCount INTEGER NOT NULL DEFAULT 0,
    added INTEGER NOT NULL,
    lastPlayed INTEGER,
    colorOverride INTEGER,
    FOREIGN KEY(gId) REFERENCES game(gId) ON DELETE CASCADE,
    FOREIGN KEY(deviceId) REFERENCES device(deviceId) ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS tag (
    tId INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    color INTEGER NOT NULL DEFAULT 0xFFFFFFFF
);
CREATE UNIQUE INDEX idx_tag_name ON tag(name);

CREATE TABLE IF NOT EXISTS tag_closure (
    ancestorId INTEGER NOT NULL,
    descendantId INTEGER NOT NULL,
    depth INTEGER NOT NULL,
    PRIMARY KEY(ancestorId, descendantId),
    FOREIGN KEY(ancestorId) REFERENCES tag(tId),
    FOREIGN KEY(descendantId) REFERENCES tag(tId)
);
CREATE TABLE IF NOT EXISTS game_tag (
    gId INTEGER NOT NULL,
    tId INTEGER NOT NULL,
    PRIMARY KEY(gId, tId),
    FOREIGN KEY(gId) REFERENCES game(gId) ON DELETE CASCADE,
    FOREIGN KEY(tId) REFERENCES tag(tId) ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS installation_tag (
    iId INTEGER NOT NULL,
    tId INTEGER NOT NULL,
    PRIMARY KEY(iId, tId),
    FOREIGN KEY(iId) REFERENCES installation(iId) ON DELETE CASCADE,
    FOREIGN KEY(tId) REFERENCES tag(tId) ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS tab (
    tabId INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    icon INTEGER NOT NULL,
    flags INTEGER NOT NULL DEFAULT 0
);
CREATE TABLE IF NOT EXISTS filter (
    fId INTEGER PRIMARY KEY,
    tabId INTEGER NOT NULL,
    name TEXT NOT NULL,
    icon INTEGER NOT NULL,
    criteria TEXT NOT NULL,
    flags INTEGER NOT NULL DEFAULT 0,
    priority INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY(tabId) REFERENCES tab(tabId) ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS plugin_fields (
    pluginId TEXT UNIQUE PRIMARY KEY,
    fields TEXT NOT NULL,
    version TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS achievement_set (
    asId INTEGER PRIMARY KEY,
    gId INTEGER NOT NULL,
    lastUpdated INTEGER DEFAULT 0,
    hidden BOOLEAN NOT NULL DEFAULT 0,
    awardColor INTEGER NOT NULL DEFAULT 0,
    name TEXT NOT NULL,
    providerId TEXT NOT NULL,
    provider TEXT NOT NULL,
    providerData BLOB,
    FOREIGN KEY(gId) REFERENCES game(gId) ON DELETE CASCADE,
    UNIQUE (gId, providerId, provider)
);

CREATE TABLE IF NOT EXISTS achievement (
    aId INTEGER PRIMARY KEY,
    asId INTEGER NOT NULL,
    number INTEGER NOT NULL DEFAULT 0,
    providerId TEXT,
    title TEXT,
    description TEXT,
    icon INTEGER,
    lockedIcon INTEGER,
    globalUnlockRate REAL,
    progressNumerator INTEGER DEFAULT 0,
    progressDenominator INTEGER DEFAULT 0,
    dateUnlocked INTEGER,
    flags INTEGER DEFAULT 0,
    borderColor INTEGER DEFAULT 0,
    providerData BLOB,
    FOREIGN KEY(asId) REFERENCES achievement_set(asId) ON DELETE CASCADE,
    UNIQUE (providerId, asId)
);

CREATE TABLE IF NOT EXISTS achievement_state (
    gId INTEGER NOT NULL,
    provider TEXT NOT NULL,
    fields TEXT,
    status INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY(gId) REFERENCES game(gId) ON DELETE CASCADE,
    UNIQUE (gId, provider)
);

CREATE TABLE IF NOT EXISTS online_account (
    oaId INTEGER PRIMARY KEY,
    fields TEXT,
    provider TEXT NOT NULL,
    updatedAt INTEGER DEFAULT 0,
    username TEXT,
    icon INTEGER,
    flags INTEGER DEFAULT 0,
    status INTEGER DEFAULT 0
);

-- Foreign keys are not indexed automatically by SQLite. These are the joins
-- and lookups used by the game/installation list queries.
CREATE INDEX IF NOT EXISTS idx_installation_gId ON installation(gId);
CREATE INDEX IF NOT EXISTS idx_achievement_set_gId ON achievement_set(gId);
CREATE INDEX IF NOT EXISTS idx_achievement_asId ON achievement(asId);

-- Default Rows
INSERT INTO tag
(tId, name, color)
VALUES
(1, 'favorite', 16018372);

INSERT INTO tag_closure
(ancestorId, descendantId, depth)
VALUES
(1, 1, 0);

INSERT INTO tab 
(tabId,name,icon,flags) 
VALUES 
(1,'Home','',0);

INSERT INTO filter 
(fId,tabId,name,icon,criteria,flags,priority) 
VALUES
(1,1,'All Games','󰊗','{"sortBy":"title","sortOrder":"ascending"}',1,0),
(2,1,'Recently Played','','{"sortBy":"last_played","sortOrder":"descending"}',0,1);

INSERT INTO tab 
(tabId,name,icon,flags) 
VALUES 
(2,'Favorites','',0);

INSERT INTO filter 
(fId,tabId,name,icon,criteria,flags,priority) 
VALUES
(3,2,'Favorites','','{"sortBy":"title","sortOrder":"ascending","tags":[[1]]}',1,0);

PRAGMA user_version = 2;
COMMIT;
)";

        NBL_CHECK(connection->execute(initQuery));

        return nbl::Result::SUCCESS;

    } else if(userVersion == 1) {

        nbl::StringView initQuery = R"(
PRAGMA foreign_keys = ON;
BEGIN TRANSACTION;
CREATE INDEX IF NOT EXISTS idx_installation_gId ON installation(gId);
CREATE INDEX IF NOT EXISTS idx_achievement_set_gId ON achievement_set(gId);
CREATE INDEX IF NOT EXISTS idx_achievement_asId ON achievement(asId);
PRAGMA user_version = 2;
COMMIT;
)";
        NBL_CHECK(connection->execute(initQuery));

        return nbl::Result::SUCCESS;

    } else if(userVersion == 2) {

        nbl::StringView initQuery = "PRAGMA foreign_keys = ON;";
        NBL_CHECK(connection->execute(initQuery));

        return nbl::Result::SUCCESS;

    } else {
        nbl::log(nbl::LogLevel::ERROR, "Unsupported database version: ", userVersion);
        return nbl::Result::INVALID_STATE;
    }
}
