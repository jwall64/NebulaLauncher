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
 * \file Data/GameModel.cpp
 * \author jwall64
 * \brief Implementation of Data/GameModel.hpp
 */



#include <Core/Array.hpp>
#include <Core/Math.hpp>
#include <Platform/Arena.hpp>
#include <Utility/StringBuilder.hpp>
#include <Data/Database.hpp>
#include <Data/Model.hpp>
#include <Data/GameModel.hpp>
#include <Data/FilterModel.hpp>
#include <Platform/Time.hpp>


nbl::Result nbl::Game::add(DatabaseConnection* connection, const nbl::Game& meta, i64* outGameId) {

    static nbl::PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
INSERT INTO game (title, description, banner, poster, icon, logo, logoPosition, releaseDate, rating, added, colorOverride) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11);
        )", &_stmt));
    }

    NBL_CHECK(_stmt.bindAll(1, meta.title, meta.description, meta.banner, meta.poster, meta.icon, meta.logo, static_cast<u32>(meta.logoPosition), meta.releaseDate, meta.rating, nbl::getCurrentTime(), meta.colorOverride));

    return connection->execute(&_stmt, outGameId); 
}

nbl::Result nbl::Game::remove(DatabaseConnection* connection, i64 gameId) {

    static nbl::PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
DELETE FROM game WHERE gId = ?1;
        )", &_stmt));
    }

    NBL_CHECK(_stmt.bindUint(1, gameId));
    return connection->execute(&_stmt);
}


nbl::Result nbl::Game::update(DatabaseConnection* connection, const Game& meta) {

    static nbl::PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
UPDATE game SET title=?1, description=?2, banner=?3, poster=?4, icon=?5, logo=?6, logoPosition=?7, releaseDate=?8, rating=?9, colorOverride = ?10 WHERE gId=?11;
        )", &_stmt));
    }

    NBL_CHECK(_stmt.bindAll(1, meta.title, meta.description, meta.banner, meta.poster, meta.icon, meta.logo, static_cast<u32>(meta.logoPosition), meta.releaseDate, meta.rating, meta.colorOverride, meta.id));

    return connection->execute(&_stmt); 

}


nbl::Result nbl::Game::get(DatabaseConnection* connection, i64 gameId, GameCallback callback) {

 static nbl::PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
SELECT 
game.gId, game.title, game.description, game.banner, game.poster, game.icon, game.logo, game.logoPosition, game.releaseDate, game.rating, game.previousPlayTime, game.previousPlayCount, game.lastPlayedDeleted, game.added, game.colorOverride 
FROM game
WHERE game.gId = ?1;
        )", &_stmt));
    }

    NBL_CHECK(_stmt.bindInt(1, gameId));

    return connection->execute(&_stmt, [&](i32 argc, nbl::DataValue* values, const char** columnNames, i64 lastInsertRowId) -> nbl::Result {

        if(argc != 15) return nbl::Result::IO_ERROR;

        Game meta = {
            .id = values[0].intValue,
            .title = values[1].textValue,
            .description = values[2].textValue,
            .banner = values[3].intValue,
            .poster = values[4].intValue,
            .icon = values[5].intValue,
            .logo = values[6].intValue,
            .logoPosition = static_cast<LogoPosition>(values[7].uintValue()),
            .releaseDate = values[8].uintValue(),
            .rating = values[9].floatValue,
            .previousPlayTime = values[10].uintValue(),
            .previousPlayCount = values[11].uintValue(),
            .lastPlayedDeleted = values[12].uintValue(),
            .added = values[13].uintValue(),
            .colorOverride = values[14].uintValueOrNull().cast<u32>()
        };
        return callback(meta);

    });
}

nbl::Result nbl::Game::getExtended(DatabaseConnection* connection, i64 gameId, GameCallback callback) {

    static nbl::PreparedStatement _getGameDetailsStmt;
    if(!_getGameDetailsStmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
SELECT
game.gId, game.title, game.description, game.banner, game.poster, game.icon, game.logo, game.logoPosition, game.releaseDate, game.rating, game.previousPlayTime, game.previousPlayCount, game.lastPlayedDeleted, game.added, game.colorOverride,
sum(installation.playTime) + game.previousPlayTime, sum(installation.playCount) + game.previousPlayCount, max(max(installation.lastPlayed), game.lastPlayedDeleted), count(installation.iId)
FROM game
FULL OUTER JOIN installation ON installation.gId = game.gId
WHERE game.gId = ?1;
        )", &_getGameDetailsStmt));
    }

    NBL_CHECK(_getGameDetailsStmt.bindInt(1, gameId));

    return connection->execute(&_getGameDetailsStmt, [&](i32 argc, nbl::DataValue* values, const char** columnNames, i64 lastInsertRowId) -> nbl::Result {

        Game details = {
            .id = values[0].intValue,
            .title = values[1].textValue,
            .description = values[2].textValue,
            .banner = values[3].intValue,
            .poster = values[4].intValue,
            .icon = values[5].intValue,
            .logo = values[6].intValue,
            .logoPosition = static_cast<LogoPosition>(values[7].intValue),
            .releaseDate = values[8].uintValue(),
            .rating = values[9].floatValue,
            .previousPlayTime = values[10].uintValue(),
            .previousPlayCount = values[11].uintValue(),
            .lastPlayedDeleted = values[12].uintValue(),
            .added = values[13].uintValue(),
            .colorOverride = values[14].uintValueOrNull().cast<u32>(),
            .totalPlayTime = values[15].uintValue(),
            .totalPlayCount = values[16].uintValue(),
            .lastPlayed = values[17].uintValue(),
            .installationCount = values[18].uintValue()
        };
        return callback(details);

    });
    
}



nbl::Result nbl::Game::lookup(DatabaseConnection *connection, const Filter& meta, GameLookupCallback callback) {

    const SearchCriteria& criteria = meta.criteria;

    usize sortByIndex = static_cast<usize>(criteria.sortBy);
    usize sortOrderIndex = static_cast<usize>(criteria.sortOrder);

    bool hasTags = criteria.tags.length() > 0 || criteria.excludedTags.length() > 0;

    usize statementIndex = (sortByIndex * 2 + sortOrderIndex) + (16 * hasTags);
    if(statementIndex >= 32) {
        return nbl::Result::INVALID_ARGUMENT;
    }

    static nbl::PreparedStatement _lookupGamesStmts[32];
    nbl::PreparedStatement& lookupGamesStmt = _lookupGamesStmts[statementIndex];

    if(!lookupGamesStmt.isValid(connection)) {

        static StringView _stmtWith = R"(
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
all_game_tags AS (
	SELECT game.gId, tag_closure.ancestorId as tId
	FROM game, tag_closure
	WHERE tag_closure.descendantId IN (SELECT game_tag.tId FROM game_tag WHERE game_tag.gId = game.gId)
),
matching_sets AS (
	SELECT 
		all_game_tags.gId,
		search_sets.setId,
		COUNT(DISTINCT search_sets.tId) AS matchedTags
	FROM all_game_tags
	JOIN search_sets 
	  ON search_sets.tId = all_game_tags.tId
	GROUP BY
		all_game_tags.gId,
		search_sets.setId
),
excluded_tags AS (
	SELECT descendantId as tId
	FROM tag_closure
	WHERE NOT ?2 IS NULL
	  AND tag_closure.ancestorId IN (SELECT tId FROM tag WHERE name IN (SELECT value FROM json_each(?2)))
))";    
        StringView _stmtSelect = R"(
SELECT DISTINCT
game.gId, game.title, game.releaseDate, game.rating, game.poster, game.icon)";

        StringView _stmtJoins = R"(
FROM game
FULL OUTER JOIN installation ON installation.gId = game.gId
)";
        StringView _stmtWhere = R"(
WHERE game.releaseDate >= ?3
  AND (?4 = 0 OR game.releaseDate <= ?4)
  AND game.rating >= ?5
  AND (?6 = 0 OR game.rating <= ?6)
  AND (?7 IS NULL OR game.title LIKE '%' || ?7 || '%')
  AND (?10 = 0 OR installation.deviceId = ?10)
)";

        static StringView _stmtEnd = R"(
LIMIT ?9
OFFSET ?8;
        )";

    
        AllocationFrame frame(Arena::threadLocal());

        StringBuilder stmtBuilder(&frame);

        if(hasTags) {
            stmtBuilder.append(_stmtWith);
        }
        if(criteria.sortBy == SortBy::ACHIEVEMENT_PROGRESS) {
            if(hasTags) {
                stmtBuilder.append(',');
            } else {
                stmtBuilder.append("WITH");
            }

            stmtBuilder.append(R"(
achievements AS (
	SELECT game.gId, aId, (dateUnlocked > 0) AS unlocked
	FROM achievement 
	JOIN achievement_set ON achievement.asId = achievement_set.asId 
	JOIN game ON achievement_set.gId = game.gId 
),
achievement_counts AS (
    SELECT gId, COUNT(aId) AS count
    FROM achievements
    GROUP BY gId
),
achievement_unlocked_counts AS (
    SELECT gId, COUNT(aId) AS count
    FROM achievements
    WHERE unlocked
    GROUP BY gId
))");
        }

        stmtBuilder.append(_stmtSelect);
        switch(criteria.sortBy) {
        default:
            break;
        case SortBy::LAST_PLAYED:
            stmtBuilder.append(R"(,
MAX(MAX(installation.lastPlayed), game.lastPlayedDeleted))");
            break;
        case SortBy::ADDED:
            stmtBuilder.append(R"(,
MIN(installation.added))");
            break;
        case SortBy::PLAY_COUNT:
            stmtBuilder.append(R"(,
SUM(installation.playCount) + game.previousPlayCount)");
            break;
        case SortBy::PLAY_TIME:
            stmtBuilder.append(R"(,
SUM(installation.playTime) + game.previousPlayTime)");
            break;
        case SortBy::ACHIEVEMENT_PROGRESS:
            stmtBuilder.append(R"(, 
COALESCE(achievement_counts.count, 0), 
COALESCE(achievement_unlocked_counts.count, 0) 
            )");
           break;
        }

        stmtBuilder.append(_stmtJoins);
        if(criteria.sortBy == SortBy::ACHIEVEMENT_PROGRESS) {
            stmtBuilder.append(R"(
LEFT JOIN achievement_counts ON achievement_counts.gId = game.gId
LEFT JOIN achievement_unlocked_counts ON achievement_unlocked_counts.gId = game.gId)");
        }
        if(hasTags) {
            stmtBuilder.append(R"(
LEFT JOIN matching_sets ON matching_sets.gId = game.gId
LEFT JOIN set_sizes ON set_sizes.setId = matching_sets.setId)");
        }

        stmtBuilder.append(_stmtWhere);
        if(hasTags) {
            stmtBuilder.append(R"(
  AND (?1 IS NULL OR matching_sets.matchedTags = set_sizes.requiredTags)
  AND (?2 IS NULL OR NOT EXISTS (SELECT tId FROM excluded_tags INTERSECT SELECT tId FROM all_game_tags WHERE all_game_tags.gId = game.gId))
            )");
        }
        if(criteria.sortBy == SortBy::ACHIEVEMENT_PROGRESS) {
            stmtBuilder.append(R"(
  AND (?11 = 0 OR ?11 <= COALESCE(achievement_counts.count, 0))
  AND (?12 = 0 OR ?12 > COALESCE(achievement_counts.count, 0))
  AND (?13 = 0 OR ?13 <= COALESCE(achievement_unlocked_counts.count, 0))
  AND (?14 = 0 OR ?14 > COALESCE(achievement_unlocked_counts.count, 0))
            )");
        }

        stmtBuilder.append("GROUP BY game.gId\n");

        switch(criteria.sortBy) {
            case SortBy::TITLE:
                if(criteria.titleFilter.empty()) {
                    stmtBuilder.append("ORDER BY game.title");
                } else {
                    stmtBuilder.append("ORDER BY nbl_str_distance(game.title, ?7)");
                }
                break;
            case SortBy::LAST_PLAYED:
                stmtBuilder.append("ORDER BY max(installation.lastPlayed)");
                break;
            case SortBy::RELEASE_DATE:
                stmtBuilder.append("ORDER BY game.releaseDate");
                break;
            case SortBy::ADDED:
                stmtBuilder.append("ORDER BY min(installation.added)");
                break;
            case SortBy::PLAY_COUNT:
                stmtBuilder.append("ORDER BY sum(installation.playCount)");
                break;
            case SortBy::PLAY_TIME:
                stmtBuilder.append("ORDER BY sum(installation.playTime)");
                break;
            case SortBy::RATING:
                stmtBuilder.append("ORDER BY game.rating");
                break;
            case SortBy::ACHIEVEMENT_PROGRESS:
                stmtBuilder.append("ORDER BY CAST(COALESCE(achievement_unlocked_counts.count, 0) AS REAL) / NULLIF(COALESCE(achievement_counts.count, 0), 0)");
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

        StringView finalStmt;
        NBL_CHECK(stmtBuilder.buildNullTerminated(&finalStmt));
        NBL_CHECK(connection->prepare(finalStmt, &lookupGamesStmt));
    }

    {
        
        AllocationFrame frame(Arena::threadLocal());
        if(criteria.tags.length() == 0) {
            NBL_CHECK(lookupGamesStmt.bindNull(1));
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

            NBL_CHECK(lookupGamesStmt.bindString(1, tagsJson));
        }

        if(criteria.excludedTags.length() == 0) {
            NBL_CHECK(lookupGamesStmt.bindNull(2));
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

            NBL_CHECK(lookupGamesStmt.bindString(2, tagsJson));
        }

        u64 maxReleaseDate = criteria.maxReleaseDate;
        if(maxReleaseDate > INT64_MAX) {
            maxReleaseDate = INT64_MAX;
        }

        
        NBL_CHECK(lookupGamesStmt.bindAll(3, 
            criteria.minReleaseDate, 
            criteria.maxReleaseDate, 
            criteria.minRating, 
            criteria.maxRating, 
            criteria.titleFilter, 
            criteria.offset, 
            criteria.limit == 0 ? 10 : criteria.limit, 
            criteria.deviceId));

        if(criteria.sortBy == SortBy::ACHIEVEMENT_PROGRESS) {
            NBL_CHECK(lookupGamesStmt.bindAll(11,
                criteria.minAchievements,
                criteria.maxAchievements,
                criteria.minUnlockedAchievements,
                criteria.maxUnlockedAchievements
            ));
        }
    }

    return connection->execute(&lookupGamesStmt, [&](i32 argc, nbl::DataValue* values, const char** columnNames, i64 lastInsertRowId) -> nbl::Result {

        GameLookupResult res = {
            .id = values[0].intValue,
            .title = values[1].textValue,
            .releaseDate = values[2].uintValue(),
            .rating = values[3].floatValue,
            .poster = values[4].intValue,
            .icon = values[5].intValue,
            .filter = meta
        };

        switch(criteria.sortBy) {
        case SortBy::LAST_PLAYED:
            res.lastPlayed = values[6].uintValue();
            break;
        case SortBy::ADDED:
            res.firstAdded = values[6].uintValue();
            break;
        case SortBy::PLAY_COUNT:
            res.playCount = values[6].uintValue();
            break;
        case SortBy::PLAY_TIME:
            res.playTime = values[6].uintValue();
            break;
        case SortBy::ACHIEVEMENT_PROGRESS:
            res.numAchievements = values[6].uintValue();
            res.numUnlockedAchievements = values[7].uintValue();
            break;
        default:
            break;
        }

        return callback(res);

    });
}


nbl::Result nbl::Game::enumerateExternalIds(DatabaseConnection* connection, i64 gameId, GameExternalIdCallback callback) {
 
    static nbl::PreparedStatement _enumerateExternalIdsStmt;
    if(!_enumerateExternalIdsStmt.isValid(connection)) {

        NBL_CHECK(connection->prepare(R"(
SELECT gId, platformName, platformId
FROM game_external_id
WHERE gId = ?1;
        )", &_enumerateExternalIdsStmt));
    }

    NBL_CHECK(_enumerateExternalIdsStmt.bindInt(1, gameId));

    return connection->execute(&_enumerateExternalIdsStmt, [&](i32 columnCount, DataValue* columnValues, const char** columnNames, i64 lastInsertRowId) -> nbl::Result {

        if(columnCount != 3) {
            return nbl::Result::IO_ERROR;
        }
        return callback(
            columnValues[0].intValue,
            columnValues[1].textValue, 
            columnValues[2].textValue
        );
    });
}

nbl::Result nbl::Game::getExternalId(DatabaseConnection* connection, i64 gameId, StringView platformName, GameExternalIdCallback callback) {

    static nbl::PreparedStatement _getExternalIdStmt;
if(!_getExternalIdStmt.isValid(connection)) {

    NBL_CHECK(connection->prepare(R"(
SELECT gId, platformName, platformId
FROM game_external_id
WHERE gId = ?1 AND platformName = ?2;
        )", &_getExternalIdStmt));
    }

    NBL_CHECK(_getExternalIdStmt.bindInt(1, gameId));
    NBL_CHECK(_getExternalIdStmt.bindString(2, platformName));

    return connection->execute(&_getExternalIdStmt, [&](i32 columnCount, DataValue* columnValues, const char** columnNames, i64 lastInsertRowId) -> nbl::Result {

        if(columnCount != 3) {
            return nbl::Result::IO_ERROR;
        }
        return callback(
            columnValues[0].intValue,
            columnValues[1].textValue, 
            columnValues[2].textValue
        );
    });
}

nbl::Result nbl::Game::setExternalId(DatabaseConnection* connection, i64 gameId, StringView platformName, StringView platformId) {

    static nbl::PreparedStatement _setExternalIdStmt;
    if(!_setExternalIdStmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
INSERT INTO game_external_id (gId, platformName, platformId)
VALUES (?1, ?2, ?3)
ON CONFLICT(gId, platformName) DO
UPDATE SET platformId = ?3;
        )", &_setExternalIdStmt));
    }
    
    NBL_CHECK(_setExternalIdStmt.bindInt(1, gameId));
    NBL_CHECK(_setExternalIdStmt.bindString(2, platformName));
    NBL_CHECK(_setExternalIdStmt.bindString(3, platformId));

    return connection->execute(&_setExternalIdStmt);

}

nbl::Result nbl::Game::removeExternalId(DatabaseConnection* connection, i64 gameId, StringView platformName) {
    
    static nbl::PreparedStatement _removeExternalIdStmt;
    if(!_removeExternalIdStmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
DELETE FROM game_external_id 
WHERE gId = ?1 AND platformName = ?2;
        )", &_removeExternalIdStmt));
    }
    
    NBL_CHECK(_removeExternalIdStmt.bindInt(1, gameId));
    NBL_CHECK(_removeExternalIdStmt.bindString(2, platformName));

    return connection->execute(&_removeExternalIdStmt);

}

nbl::Result nbl::Game::clearExternalIds(DatabaseConnection* connection, i64 gameId) {
    
    static nbl::PreparedStatement _clearExternalIdsStmt;
    if(!_clearExternalIdsStmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
DELETE FROM game_external_id 
WHERE gId = ?1;
        )", &_clearExternalIdsStmt));
    }
    
    NBL_CHECK(_clearExternalIdsStmt.bindInt(1, gameId));

    return connection->execute(&_clearExternalIdsStmt);
}


nbl::Result nbl::Game::getLastPlayed(DatabaseConnection* connection, i64 gameId, u64* outTime) {

    static nbl::PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
SELECT max(max(installation.lastPlayed), game.lastPlayedDeleted) FROM installation JOIN game ON game.gId=installation.gId WHERE installation.gId=?1;
        )", &_stmt));
    }
    
    NBL_CHECK(_stmt.bindInt(1, gameId));
    return connection->execute(&_stmt, [&](i32 count, DataValue* values, const char** names, i64 lastInsertRowId) -> nbl::Result {
        *outTime = values[0].uintValue();
        return nbl::Result::SUCCESS;
    });
}
nbl::Result nbl::Game::getFirstAdded(DatabaseConnection* connection, i64 gameId, u64* outTime) {

    static nbl::PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
SELECT min(added) FROM installation WHERE gId=?1;
        )", &_stmt));
    }
    
    NBL_CHECK(_stmt.bindInt(1, gameId));
    return connection->execute(&_stmt, [&](i32 count, DataValue* values, const char** names, i64 lastInsertRowId) -> nbl::Result {
        *outTime = values[0].uintValue();
        return nbl::Result::SUCCESS;
    });

}
nbl::Result nbl::Game::getPlayCount(DatabaseConnection* connection, i64 gameId, u64* outCount) {
    static nbl::PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
SELECT sum(installation.playCount) + game.previousPlayCount FROM installation JOIN game ON game.gId=installation.gId WHERE installation.gId=?1;
        )", &_stmt));
    }
    
    NBL_CHECK(_stmt.bindInt(1, gameId));
    return connection->execute(&_stmt, [&](i32 count, DataValue* values, const char** names, i64 lastInsertRowId) -> nbl::Result {
        *outCount = values[0].uintValue();
        return nbl::Result::SUCCESS;
    });
}
nbl::Result nbl::Game::getPlayTime(DatabaseConnection* connection, i64 gameId, u64* outTime) {
    static nbl::PreparedStatement _stmt;
    if(!_stmt.isValid(connection)) {
        NBL_CHECK(connection->prepare(R"(
SELECT sum(installation.playTime) + game.previousPlayTime FROM installation JOIN game ON game.gId=installation.gId WHERE installation.gId=?1;
        )", &_stmt));
    }
    
    NBL_CHECK(_stmt.bindInt(1, gameId));
    return connection->execute(&_stmt, [&](i32 count, DataValue* values, const char** names, i64 lastInsertRowId) -> nbl::Result {
        *outTime = values[0].uintValue();
        return nbl::Result::SUCCESS;
    });
}


nbl::Result nbl::Game::lookupForFilter(DatabaseConnection* connection, i64 filterId, i64 deviceId, u64 offset, u64 limit, GameLookupCallback callback) {

    NBL_CHECK(Filter::get(connection, filterId, [&](const nbl::Filter& meta) -> nbl::Result {

        Filter updated = meta;

        updated.criteria.offset = offset;
        updated.criteria.deviceId = (meta.flags & FilterFlag::ONLY_INSTALLED) ? deviceId : 0;

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
