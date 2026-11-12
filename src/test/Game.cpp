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


#include <gtest/gtest.h>

#include <Data/Model.hpp>
#include <Data/GameModel.hpp>
#include <Data/AchievementModel.hpp>
#include <Data/TagModel.hpp>
#include <Data/FilterModel.hpp>
#include <Core/Array.hpp>

namespace {

nbl::Game _gameMeta = {
    .id = 0,
    .title = "Test Game",
    .description = "A test game",
    .banner = 1,
    .poster = 2,
    .icon = 3,
    .logo = 4,
    .releaseDate = 1600000000,
    .rating = 4.5
};

}

TEST(Game, AddGame) {
 
    nbl::DatabaseConnection connection;
    ASSERT_TRUE(nbl::DatabaseConnection::openInMemory(&connection).success());
    ASSERT_TRUE(nbl::initDatabase(&connection).success());

    i64 gameId = 0;
    ASSERT_TRUE(nbl::Game::add(&connection, _gameMeta, &gameId).success());
    ASSERT_NE(gameId, 0);
}

TEST(Game, RemoveGame) {
 
    nbl::DatabaseConnection connection;
    ASSERT_TRUE(nbl::DatabaseConnection::openInMemory(&connection).success());
    ASSERT_TRUE(nbl::initDatabase(&connection).success());

    i64 gameId = 0;
    ASSERT_TRUE(nbl::Game::add(&connection, _gameMeta, &gameId).success());
    ASSERT_NE(gameId, 0);
    ASSERT_TRUE(nbl::Game::remove(&connection, gameId).success());
}

TEST(Game, AddGameTag) {
 
    nbl::DatabaseConnection connection;
    ASSERT_TRUE(nbl::DatabaseConnection::openInMemory(&connection).success());
    ASSERT_TRUE(nbl::initDatabase(&connection).success());

    i64 gameId = 0;
    ASSERT_TRUE(nbl::Game::add(&connection, _gameMeta, &gameId).success());
    ASSERT_NE(gameId, 0);

    i64 tId = 0;
    ASSERT_TRUE(nbl::Tag::add(&connection, "Action", 0xFF0000, &tId).success());
    ASSERT_TRUE(nbl::Tag::tagGame(&connection, gameId, tId).success());
}

TEST(Game, RemoveGameTag) {
 
    nbl::DatabaseConnection connection;
    ASSERT_TRUE(nbl::DatabaseConnection::openInMemory(&connection).success());
    ASSERT_TRUE(nbl::initDatabase(&connection).success());

    i64 gameId = 0;
    ASSERT_TRUE(nbl::Game::add(&connection, _gameMeta, &gameId).success());
    ASSERT_NE(gameId, 0);

    i64 tId;
    ASSERT_TRUE(nbl::Tag::add(&connection, "Action", 0xFF0000, &tId).success());

    ASSERT_TRUE(nbl::Tag::tagGame(&connection, gameId, tId).success());
    ASSERT_TRUE(nbl::Tag::untagGame(&connection, gameId, tId).success());
}

TEST(Game, GetGameTags) {

    nbl::DatabaseConnection connection;
    ASSERT_TRUE(nbl::DatabaseConnection::openInMemory(&connection).success());
    ASSERT_TRUE(nbl::initDatabase(&connection).success());

    i64 gameId = 0;
    ASSERT_TRUE(nbl::Game::add(&connection, _gameMeta, &gameId).success());
    ASSERT_NE(gameId, 0);

    i64 actId;
    ASSERT_TRUE(nbl::Tag::add(&connection, "Action", 0xFF0000, &actId).success());

    i64 advId;
    ASSERT_TRUE(nbl::Tag::add(&connection, "Adventure", 0x00FF00, &advId).success());

    ASSERT_TRUE(nbl::Tag::tagGame(&connection, gameId, actId).success());
    ASSERT_TRUE(nbl::Tag::tagGame(&connection, gameId, advId).success());

    struct _FoundData {
        bool action = false;
        bool adventure = false;
    } found;

    ASSERT_TRUE(nbl::Tag::enumerateGameTags(&connection, gameId, [&](const nbl::Tag& tag) -> nbl::Result {
        if(tag.name == "Action") {
            found.action = true;
        }
        if(tag.name == "Adventure") {
            found.adventure = true;
        }
        return nbl::Result::SUCCESS;
    }).success());

    ASSERT_TRUE(found.action);
    ASSERT_TRUE(found.adventure);
}

TEST(Game, GetGameMeta) {

    nbl::DatabaseConnection connection;
    ASSERT_TRUE(nbl::DatabaseConnection::openInMemory(&connection).success());
    ASSERT_TRUE(nbl::initDatabase(&connection).success());

    i64 gameId = 0;
    ASSERT_TRUE(nbl::Game::add(&connection, _gameMeta, &gameId).success());
    ASSERT_NE(gameId, 0);


    bool found = false;
    ASSERT_EQ(nbl::Game::get(&connection, gameId, [&](const nbl::Game& details) -> nbl::Result {
        if(details.title == "Test Game" 
        && details.description == "A test game" 
        && details.banner == 1
        && details.poster == 2 
        && details.icon == 3 
        && details.logo == 4 
        && details.releaseDate == 1600000000 
        && details.rating == 4.5) {

            found = true;
        }
        return nbl::Result::SUCCESS;
    }), nbl::Result::SUCCESS);

    ASSERT_TRUE(found);

}

TEST(Game, LookupSingleGame) {

    nbl::DatabaseConnection connection;
    ASSERT_TRUE(nbl::DatabaseConnection::openInMemory(&connection).success());
    ASSERT_TRUE(nbl::initDatabase(&connection).success());

    i64 gameId = 0;
    ASSERT_TRUE(nbl::Game::add(&connection, _gameMeta, &gameId).success());
    ASSERT_NE(gameId, 0);

    i64 foundId = 0;
    ASSERT_TRUE(nbl::Game::lookup(
        &connection, {}, 
        [&](const nbl::GameLookupResult& res) -> nbl::Result {

        if(res.title == "Test Game" 
        && res.poster == 2
        && res.icon == 3 
        && res.releaseDate == 1600000000 
        && res.rating == 4.5) {

            foundId = res.id;
        }
        return nbl::Result::SUCCESS;
    }).success());

    ASSERT_EQ(foundId, gameId);

}


TEST(Game, LookupSingleGameByName) {

    nbl::DatabaseConnection connection;
    ASSERT_TRUE(nbl::DatabaseConnection::openInMemory(&connection).success());
    ASSERT_TRUE(nbl::initDatabase(&connection).success());

    i64 gameId = 0;
    ASSERT_TRUE(nbl::Game::add(&connection, _gameMeta, &gameId).success());
    ASSERT_NE(gameId, 0);

    i64 foundId = 0;

    nbl::SearchCriteria criteria = {
        .titleFilter = "Test Game"
    };


    ASSERT_TRUE(nbl::Game::lookup(
        &connection, { .criteria = criteria }, 
        [&](const nbl::GameLookupResult& details) -> nbl::Result {

        if(details.title == "Test Game" 
        && details.poster == 2
        && details.icon == 3 
        && details.releaseDate == 1600000000 
        && details.rating == 4.5) {

            foundId = details.id;
        }
        return nbl::Result::SUCCESS;
    }).success());

    ASSERT_EQ(foundId, gameId);
}

TEST(Game, LookupSingleGameByTag) {

    nbl::DatabaseConnection connection;
    ASSERT_TRUE(nbl::DatabaseConnection::openInMemory(&connection).success());
    ASSERT_TRUE(nbl::initDatabase(&connection).success());

    i64 gameId = 0;
    ASSERT_TRUE(nbl::Game::add(&connection, _gameMeta, &gameId).success());
    ASSERT_NE(gameId, 0);

    i64 tId;
    ASSERT_TRUE(nbl::Tag::add(&connection, "Action", 0xFF0000, &tId).success());
    ASSERT_TRUE(nbl::Tag::tagGame(&connection, gameId, tId).success());

    i64 foundId = 0;
    nbl::SearchCriteria criteria = {};

    nbl::Array tags = { tId };
    nbl::Array tagSets = { tags.constSpan() };

    criteria.tags = tagSets.span();

    ASSERT_TRUE(nbl::Game::lookup(
        &connection, { .criteria = criteria }, 
        [&](const nbl::GameLookupResult& res) -> nbl::Result {

        if(res.title == "Test Game" 
        && res.poster == 2 
        && res.icon == 3 
        && res.releaseDate == 1600000000 
        && res.rating == 4.5) {

            foundId = res.id;
        }
        return nbl::Result::SUCCESS;
    }).success());

    ASSERT_EQ(foundId, gameId);

}

TEST(Game, LookupByAchievementProgress) {

    nbl::DatabaseConnection connection;
    ASSERT_TRUE(nbl::DatabaseConnection::openInMemory(&connection).success());
    ASSERT_TRUE(nbl::initDatabase(&connection).success());

    i64 gameId = 0;
    ASSERT_TRUE(nbl::Game::add(&connection, _gameMeta, &gameId).success());
    ASSERT_NE(gameId, 0);

    nbl::AchievementSet set = {
        .id = 0,
        .gameId = gameId,
        .lastUpdated = 0,
        .hidden = false,
        .awardColor = 0,
        .name = "Test Set",
        .provider = "test",
        .providerId = "1",
        .providerData = {}
    };

    i64 setId = 0;
    ASSERT_TRUE(nbl::AchievementSet::add(&connection, set, &setId).success());

    nbl::Achievement unlocked = {
        .id = 0,
        .setId = setId,
        .number = 1,
        .providerId = "unlocked",
        .title = "Unlocked",
        .description = "",
        .icon = 0,
        .lockedIcon = 0,
        .globalUnlockRate = 0.0,
        .progress = { 0, 0 },
        .dateUnlocked = 1,
        .flags = 0,
        .borderColor = 0,
        .providerData = {}
    };

    nbl::Achievement locked = unlocked;
    locked.number = 2;
    locked.providerId = "locked";
    locked.title = "Locked";
    locked.dateUnlocked = 0;

    i64 achievementId = 0;
    ASSERT_TRUE(nbl::Achievement::add(&connection, unlocked, &achievementId).success());
    ASSERT_TRUE(nbl::Achievement::add(&connection, locked, &achievementId).success());

    nbl::SearchCriteria criteria = {
        .sortBy = nbl::SortBy::ACHIEVEMENT_PROGRESS,
        .sortOrder = nbl::SortOrder::DESCENDING,
        .limit = 10
    };

    bool found = false;
    ASSERT_TRUE(nbl::Game::lookup(
        &connection, { .criteria = criteria },
        [&](const nbl::GameLookupResult& res) -> nbl::Result {

        found = res.id == gameId
            && res.numAchievements == 2
            && res.numUnlockedAchievements == 1;

        return nbl::Result::SUCCESS;
    }).success());

    ASSERT_TRUE(found);
}
