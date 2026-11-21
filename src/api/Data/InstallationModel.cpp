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
 * \file Data/InstallationModel.cpp
 * \author jwall64
 * \brief Implementation of Data/InstallationModel.hpp
 */


#include <Core/Result.hpp>
#include <Core/Array.hpp>
#include <Core/Math.hpp>
#include <Utility/StringBuilder.hpp>
#include <Platform/Time.hpp>
#include <Data/Database.hpp>
#include <Data/Model.hpp>
#include <Data/FilterModel.hpp>
#include <Data/InstallationModel.hpp>


nbl::Result nbl::Installation::add(DatabaseConnection* connection, const Installation& meta, i64* outInstallationId) {

    static nbl::PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
INSERT INTO installation 
(gId, deviceId, title, description, banner, poster, icon, logo, logoPosition, type, command, playTime, playCount, lastPlayed, added, colorOverride) 
VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, 0, 0, 0, ?12, ?13);
        )", &_stmt));
    }

    NBL_CHECK(_stmt.bindAll(1, meta.gameId, meta.deviceId, meta.title, meta.description, meta.banner, meta.poster, meta.icon, meta.logo, static_cast<u32>(meta.logoPosition), meta.type, meta.command, nbl::getCurrentTime(), meta.colorOverride));

    return connection->execute(&_stmt, outInstallationId);
}


nbl::Result nbl::Installation::remove(DatabaseConnection* connection, i64 installationId) {

    static nbl::PreparedStatement _updateGameStmt;
    if(!_updateGameStmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
UPDATE game SET 
previousPlayTime = previousPlayTime + installation.playTime,
previousPlayCount = previousPlayCount + installation.playCount,
lastPlayedDeleted = max(lastPlayedDeleted, installation.lastPlayed)   
FROM installation
WHERE installation.gId = game.gId
  AND installation.iId = ?1;
        )", &_updateGameStmt));
    }

    static nbl::PreparedStatement _removeInstallationStmt;
    if(!_removeInstallationStmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
DELETE FROM installation WHERE iId = ?1;
        )", &_removeInstallationStmt));
    }



    NBL_CHECK(_updateGameStmt.bindUint(1, installationId));
    NBL_CHECK(_removeInstallationStmt.bindUint(1, installationId));
    return connection->doTransaction(nbl::Array{&_updateGameStmt,&_removeInstallationStmt});
}

nbl::Result nbl::Installation::update(DatabaseConnection* connection, const Installation& meta) {

    static nbl::PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
UPDATE installation SET gId=?1, deviceId=?2, title=?3, description=?4, banner=?5, poster=?6, icon=?7, logo=?8, logoPosition=?9, type=?10, command=?11, colorOverride=?12 WHERE iId=?13;
        )", &_stmt));
    }


    NBL_CHECK(_stmt.bindAll(1, meta.gameId, meta.deviceId, meta.title, meta.description, meta.banner, meta.poster, meta.icon, meta.logo, static_cast<u32>(meta.logoPosition), meta.type, meta.command, meta.colorOverride, meta.id));

    return connection->execute(&_stmt);
}


nbl::Result nbl::Installation::get(DatabaseConnection* connection, i64 installationid, InstallationMetaCallback callback) {

    static PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
SELECT iId, gId, deviceId, title, description, banner, poster, icon, logo, logoPosition, type, command, playTime, playCount, lastPlayed, added, colorOverride
FROM installation
WHERE iId = ?1;
        )", &_stmt));
    }

    NBL_CHECK(_stmt.bindInt(1, installationid));


    return connection->execute(&_stmt, [&](i32 columnCount, nbl::DataValue* columnValues, const char** columnNames, i64 lastInsertRowId) -> nbl::Result {

        NBL_ASSERT(columnCount == 17);

        Installation meta = {
            .id = columnValues[0].intValue,
            .gameId = columnValues[1].intValue,
            .deviceId = columnValues[2].intValue,
            .title = columnValues[3].textValue,
            .description = columnValues[4].textValue,
            .banner = columnValues[5].intValue,
            .poster = columnValues[6].intValue,
            .icon = columnValues[7].intValue,
            .logo = columnValues[8].intValue,
            .logoPosition = static_cast<LogoPosition>(columnValues[9].intValue),
            .type = columnValues[10].textValue,
            .command = columnValues[11].textValue,
            .playTime = columnValues[12].uintValue(),
            .playCount = columnValues[13].uintValue(),
            .lastPlayed = columnValues[14].uintValue(),
            .added = columnValues[15].uintValue(),
            .colorOverride = columnValues[16].uintValueOrNull().cast<u32>()
        };

        return callback(meta);

    });

}


nbl::Result nbl::Installation::getExtended(DatabaseConnection* connection, i64 installationId, InstallationExtendedCallback callback) {

    static nbl::PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
SELECT 
installation.iId, installation.gId, installation.deviceId, installation.title, installation.description, installation.banner, installation.poster, installation.icon, installation.logo, installation.logoPosition, installation.type, installation.command, installation.playTime, installation.playCount, installation.lastPlayed, installation.added, installation.colorOverride,
game.gId, game.title, game.description, game.banner, game.poster, game.icon, game.logo, game.logoPosition, game.releaseDate, game.rating, game.colorOverride
FROM installation
JOIN game ON installation.gId = game.gId
WHERE installation.iId = ?1;
        )", &_stmt));
    }

    NBL_CHECK(_stmt.bindUint(1, installationId));

    return connection->execute(&_stmt, [&](i32 columnCount, nbl::DataValue* columnValues, const char** columnNames, i64 lastInsertRowId) -> nbl::Result {

        NBL_ASSERT(columnCount == 28);

        return callback({
                .id = columnValues[0].intValue,
                .gameId = columnValues[1].intValue,
                .deviceId = columnValues[2].intValue,
                .title = columnValues[3].textValue,
                .description = columnValues[4].textValue,
                .banner = columnValues[5].intValue,
                .poster = columnValues[6].intValue,
                .icon = columnValues[7].intValue,
                .logo = columnValues[8].intValue,
                .logoPosition = static_cast<LogoPosition>(columnValues[9].intValue),
                .type = columnValues[10].textValue,
                .command = columnValues[11].textValue,
                .playTime = columnValues[12].uintValue(),
                .playCount = columnValues[13].uintValue(),
                .lastPlayed = columnValues[14].uintValue(),
                .added = columnValues[15].uintValue(),
                .colorOverride = columnValues[16].uintValueOrNull().cast<u32>()
            },
            {
                .id = columnValues[17].intValue,
                .title = columnValues[18].textValue,
                .description = columnValues[19].textValue,
                .banner = columnValues[20].intValue,
                .poster = columnValues[21].intValue,
                .icon = columnValues[22].intValue,
                .logo = columnValues[23].intValue,
                .logoPosition = static_cast<LogoPosition>(columnValues[24].intValue),
                .releaseDate = columnValues[25].uintValue(),
                .rating = columnValues[26].floatValue,
                .colorOverride = columnValues[27].uintValueOrNull().cast<u32>()
            });
    });
}

nbl::Result nbl::Installation::lookup(
        DatabaseConnection* connection, 
        const Filter& meta,
        InstallationLookupCallback callback) {


    const SearchCriteria& criteria = meta.criteria;
    

    static nbl::PreparedStatement _stmts[16];

    usize index = static_cast<usize>(criteria.sortBy) * 2 + static_cast<usize>(criteria.sortOrder);
    if(index >= 16) {
        return nbl::Result::INVALID_ARGUMENT;
    }

    nbl::PreparedStatement& lookupInstallationsStmt = _stmts[index];

    if(!lookupInstallationsStmt.isValid(connection)) {

        static StringView _stmtBegin = R"(
WITH 
search_sets AS (
	SELECT
		outer.key AS setId,
		inner.value AS tId
	FROM json_each(?1) AS outer
	JOIN json_each(outer.value) AS inner
),
set_sizes AS (
	SELECT
		setId,
		COUNT(*) AS requiredTags
	FROM search_sets
	GROUP BY setId
),
all_installation_tags AS (
	SELECT installation.iId, tag_closure.ancestorId as tId
	FROM installation, tag_closure
	WHERE tag_closure.descendantId IN (SELECT installation_tag.tId FROM installation_tag WHERE installation_tag.iId = installation.iId)
	   OR tag_closure.descendantId IN (SELECT game_tag.tId FROM game_tag WHERE game_tag.gId = installation.gId)
),
matching_sets AS (
	SELECT 
		all_installation_tags.iId,
		search_sets.setId,
		COUNT(DISTINCT search_sets.tId) AS matchedTags
	FROM all_installation_tags
	JOIN search_sets 
	  ON search_sets.tId = all_installation_tags.tId
	GROUP BY
		all_installation_tags.iId,
		search_sets.setId
),
excluded_tags AS (
	SELECT descendantId AS tId
	FROM tag_closure
	WHERE NOT ?2 IS NULL
	  AND tag_closure.ancestorId IN (SELECT tId FROM tag WHERE name IN (SELECT value FROM json_each(?2)))
),
achievements AS (
	SELECT game.gId, aId, (dateUnlocked > 0) AS unlocked
	FROM achievement 
	JOIN achievement_set ON achievement.asId = achievement_set.asId 
	JOIN game ON achievement_set.gId = game.gId
)
SELECT DISTINCT 
installation.iId, installation.gId,
installation.title, game.title, installation.playTime, installation.playCount, installation.lastPlayed, installation.added, 
IFNULL(installation.poster, game.poster), IFNULL(installation.icon, game.icon),
game.releaseDate, game.rating,
IFNULL(installation.title, game.title) AS resolved_title
FROM installation
LEFT JOIN matching_sets ON matching_sets.iId = installation.iId
LEFT JOIN set_sizes ON set_sizes.setId = matching_sets.setId
JOIN game ON installation.gId = game.gId
WHERE (?12 = 0 OR installation.deviceId = ?12)
  AND game.releaseDate >= ?3
  AND (?4 = 0 OR game.releaseDate < ?4)
  AND game.rating >= ?5
  AND (?6 = 0 OR game.rating < ?6)
  AND installation.playTime >= ?7
  AND (?8 = 0 OR installation.playTime < ?8)
  AND (?9 IS NULL OR resolved_title LIKE '%' || ?9 || '%')
  AND (?1 IS NULL OR matching_sets.matchedTags = set_sizes.requiredTags)
  AND (?2 IS NULL OR NOT EXISTS (SELECT tId FROM excluded_tags INTERSECT SELECT tId FROM all_installation_tags WHERE all_installation_tags.iId = installation.iId))
  AND (?13 = 0 OR ?13 <= (SELECT COUNT(aId) FROM achievements WHERE gId = game.gId))
  AND (?14 = 0 OR ?14 > (SELECT COUNT(aId) FROM achievements WHERE gId = game.gId))
  AND (?15 = 0 OR ?15 <= (SELECT COUNT(aId) FROM achievements WHERE gId = game.gId AND achievements.unlocked = 1))
  AND (?16 = 0 OR ?16 > (SELECT COUNT(aId) FROM achievements WHERE gId = game.gId AND achievements.unlocked = 1))
GROUP BY installation.iId
)";

        static StringView _stmtEnd = R"(
LIMIT ?11
OFFSET ?10;
        )";

        AllocationFrame frame(Arena::threadLocal());

        StringBuilder stmtBuilder(&frame);
        stmtBuilder.append(_stmtBegin);

        switch(criteria.sortBy) {
            case SortBy::TITLE:
                if(criteria.titleFilter.empty()) {
                    stmtBuilder.append("ORDER BY resolved_title");
                } else {
                    stmtBuilder.append("ORDER BY nbl_str_distance(resolved_title, ?9)");
                }
                break;
            case SortBy::LAST_PLAYED:
                stmtBuilder.append("ORDER BY installation.lastPlayed");
                break;
            case SortBy::ADDED:
                stmtBuilder.append("ORDER BY installation.added");
                break;
            case SortBy::PLAY_COUNT:
                stmtBuilder.append("ORDER BY installation.playCount");
                break;
            case SortBy::PLAY_TIME:
                stmtBuilder.append("ORDER BY installation.playTime");
                break;
            case SortBy::RELEASE_DATE:
                stmtBuilder.append("ORDER BY game.releaseDate");
                break;
            case SortBy::RATING:
                stmtBuilder.append("ORDER BY game.rating");
                break;
            case SortBy::ACHIEVEMENT_PROGRESS:
                stmtBuilder.append("ORDER BY CAST((SELECT COUNT(aId) FROM achievements WHERE achievements.gId=game.gId AND unlocked = 1) AS REAL) / (SELECT COUNT(aId) FROM achievements WHERE achievements.gId=game.gId)");
                break;
            default:
                return nbl::Result::INVALID_ARGUMENT;
        }

        if(criteria.sortOrder == SortOrder::ASCENDING) {
            stmtBuilder.append(" ASC");
        } else {
            stmtBuilder.append(" DESC");
        }
        stmtBuilder.append(_stmtEnd);

        StringView stmt;
        NBL_CHECK(stmtBuilder.buildNullTerminated(&stmt));

        NBL_CHECK(connection->prepare(stmt, &lookupInstallationsStmt));
    }



    AllocationFrame frame(Arena::threadLocal());

    if(criteria.tags.length() == 0) {
        NBL_CHECK(lookupInstallationsStmt.bindNull(1));
    } else {

        StringBuilder tagsBuilder(&frame);
        NBL_CHECK(nbl::JsonWriter(tagsBuilder.writer())
            .writeArray([&](auto arr) {
                for(ConstSpan<i64> tagSet : criteria.tags) {
                    arr.writeArray([&](auto inner) {
                        for(i64 tag : tagSet) {
                            inner.add(tag);
                        }
                        return Result::SUCCESS;
                    });
                }
                return Result::SUCCESS;
            }));

        StringView tagsJson;
        tagsBuilder.build(&tagsJson);

        NBL_CHECK(lookupInstallationsStmt.bindString(1, tagsJson));
    }

    if(criteria.excludedTags.length() == 0) {
        NBL_CHECK(lookupInstallationsStmt.bindNull(2));
    } else {
        StringBuilder tagsBuilder(&frame);
        NBL_CHECK(nbl::JsonWriter(tagsBuilder.writer())
            .writeArray([&](auto arr) {
                for(i64 tag : criteria.excludedTags) {
                    arr.add(tag);
                }
                return Result::SUCCESS;
            }));

        StringView tagsJson;
        tagsBuilder.build(&tagsJson);

        NBL_CHECK(lookupInstallationsStmt.bindString(2, tagsJson));
    }

    NBL_CHECK(lookupInstallationsStmt.bindAll(3, 
        criteria.minReleaseDate, 
        criteria.maxReleaseDate, 
        criteria.minRating, 
        criteria.maxRating, 
        criteria.minPlayTime, 
        criteria.maxPlayTime, 
        criteria.titleFilter, 
        criteria.offset, 
        criteria.limit == 0 ? 10 : criteria.limit, 
        criteria.deviceId,
        criteria.minAchievements,
        criteria.maxAchievements,
        criteria.minUnlockedAchievements,
        criteria.maxUnlockedAchievements
    ));

    NBL_CHECK(connection->execute(&lookupInstallationsStmt, [&](
        i32 columnCount, 
        DataValue* columnValues, 
        const char** columnNames, 
        i64 lastInsertRowId) -> nbl::Result {

        NBL_ASSERT(columnCount == 12);
        InstallationLookupResult res = {
            .installationId = columnValues[0].intValue,
            .gameId = columnValues[1].intValue,
            .title = columnValues[2].textValue,
            .gameTitle = columnValues[3].textValue,
            .playTime = columnValues[4].uintValue(),
            .playCount = columnValues[5].uintValue(),
            .lastPlayed = columnValues[6].uintValue(),
            .added = columnValues[7].uintValue(),
            .poster = columnValues[8].intValue,
            .icon = columnValues[9].intValue,
            .releaseDate = columnValues[10].uintValue(),
            .rating = columnValues[11].floatValue,
            .filter = meta
        };

        return callback(
            res
        );
    }));

    return nbl::Result::SUCCESS;
}

nbl::Result nbl::Installation::lookupForGame(DatabaseConnection* connection, i64 gameId, i64 deviceId, u64 limit, u64 offset, InstallationLookupCallback callback) {

    static nbl::PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
SELECT 
installation.iId, game.gId, installation.title, game.title, 
installation.playTime, installation.playCount, installation.lastPlayed, installation.added, 
IIF(installation.poster == 0, game.poster, installation.poster), IIF(installation.icon == 0, game.icon, installation.icon), game.releaseDate, game.rating
FROM installation
JOIN game ON installation.gId = game.gId
WHERE installation.gId = ?1
  AND (?2 IS NULL OR installation.deviceId = ?2)
ORDER BY installation.lastPlayed DESC
LIMIT ?3
OFFSET ?4;
        )", &_stmt));
    }

    NBL_CHECK(_stmt.bindInt(1, gameId));
    NBL_CHECK(_stmt.bindInt(2, deviceId));
    NBL_CHECK(_stmt.bindUint(3, limit));
    NBL_CHECK(_stmt.bindUint(4, offset));

    NBL_CHECK(connection->execute(&_stmt, [&](
        i32 columnCount, 
        DataValue* columnValues, 
        const char** columnNames, 
        i64 lastInsertRowId) -> nbl::Result {

        if(columnCount < 12) {
            return nbl::Result::IO_ERROR;
        }
        InstallationLookupResult res = {
            .installationId = columnValues[0].intValue,
            .gameId = columnValues[1].intValue,
            .title = columnValues[2].textValue,
            .gameTitle = columnValues[3].textValue,
            .playTime = columnValues[4].uintValue(),
            .playCount = columnValues[5].uintValue(),
            .lastPlayed = columnValues[6].uintValue(),
            .added = columnValues[7].uintValue(),
            .poster = columnValues[8].intValue,
            .icon = columnValues[9].intValue,
            .releaseDate = columnValues[10].uintValue(),
            .rating = columnValues[11].floatValue,
            .filter = nbl::Filter{
                .criteria = {
                    .gameId = gameId
                }
            }
        };

        return callback(res);
    }));

    return nbl::Result::SUCCESS;

}


nbl::Result nbl::Installation::markJustPlayed(DatabaseConnection* connection, i64 installationId) {

    static nbl::PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
UPDATE installation
SET playCount = playCount + 1,
    lastPlayed = ?1
WHERE iId = ?2;
        )", &_stmt));
    }

    NBL_CHECK(_stmt.bindUint(1, nbl::getCurrentTime()));
    NBL_CHECK(_stmt.bindUint(2, installationId));

    return connection->execute(&_stmt);
}

nbl::Result nbl::Installation::bumpPlayTime(DatabaseConnection* connection, i64 installationId, u64 playTime) {

    static nbl::PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
UPDATE installation
SET playTime = playTime + ?1
WHERE iId = ?2;
        )", &_stmt));
    }
    
    NBL_CHECK(_stmt.bindUint(1, playTime));
    NBL_CHECK(_stmt.bindUint(2, installationId));

    return connection->execute(&_stmt);

}


nbl::Result nbl::Installation::lookupForFilter(DatabaseConnection* connection, i64 filterId, i64 deviceId, u64 offset, u64 limit, InstallationLookupCallback callback) {

    NBL_CHECK(Filter::get(connection, filterId, [&](const nbl::Filter& meta) -> nbl::Result {

        Filter updated = meta;

        updated.criteria.offset = offset;
        updated.criteria.deviceId = deviceId;

        if(limit > 0) {
            updated.criteria.limit = updated.criteria.limit > 0 ? nbl::min(limit, meta.criteria.limit) : limit;
        }

        return lookup(
            connection,
            updated,
            callback
        );

     }));

     return nbl::Result::SUCCESS;
}

