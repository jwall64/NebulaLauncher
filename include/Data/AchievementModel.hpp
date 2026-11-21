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
 * \file Data/AchievementModel.hpp
 * \author jwall64
 * \brief Structs related to achievements in the database, and functions for performing CRUD operations on them
 */

#pragma once

#include <Core/Types.hpp>
#include <Core/String.hpp>
#include <Data/Database.hpp>
#include <Data/Common.hpp>

#include <Core/FunctionRef.hpp>

namespace nbl {

enum class AchievementSortBy : u32 {
    NUMBER,             // The achievement's provider-determined index
    NAME,               // The achievement's display name
    DATE_UNLOCKED,      // The date the achievement was unlocked
    PROGRESS,           // How close the user is to completing the achievement
    GLOBAL_UNLOCK_RATE, // The percentage of players worldwide who have unlocked the achievement
};

enum class AchievementProviderStatus : u32 {
    NOT_ADDED,           // The achievement provider has not considered this game yet.
    ADDED_BY_PROVIDER,   // The achievement provdier has automatically determined that it can provide achievements for this game.
    ADDED_BY_USER,       // The user manually added information for this achievement provider and game.
    REMOVED_BY_PROVIDER, // The achievement provider has automatically determined that it cannot provide achievements for this game.
    REMOVED_BY_USER      // The user manually removed information for this achievement provider and game. Providers should not attempt to re-add achievement sets for states with this status.
};


enum class AchievementFlag : u8 {
    PROGRESSION = 0b001,   // This achievement is required to beat the game
    WIN_CONDITION = 0b010, // Unlocking this achievement is the same as beating the game
    SPOILER = 0b100,       // This achievement's title and/or description would spoil the story for a new player
    MISSABLE = 0b1000,     // It is possible to put your save file in a state in which this achievement is no longer obtainable without restarting
    STALE = 0b10000000     // Used to easily identify achivements which have been removed from a set without iterating over all of them.
};
typedef u8 AchievementFlags;


inline AchievementFlags operator|(AchievementFlag flag, AchievementFlag other) { return static_cast<u8>(flag) | static_cast<u8>(other); }
inline AchievementFlags operator|(AchievementFlag flag, AchievementFlags other) { return static_cast<u8>(flag) | other; }
inline AchievementFlags operator|(AchievementFlags other, AchievementFlag flag) { return static_cast<u8>(flag) | other; }

inline AchievementFlags& operator|=(AchievementFlags& flags, AchievementFlag other) { flags = flags | other; return flags; }

inline AchievementFlags operator&(AchievementFlag flag, AchievementFlag other) { return static_cast<u8>(flag) & static_cast<u8>(other); }
inline AchievementFlags operator&(AchievementFlag flag, AchievementFlags other) { return static_cast<u8>(flag) & other; }
inline AchievementFlags operator&(AchievementFlags other, AchievementFlag flag) { return static_cast<u8>(flag) & other; }

inline AchievementFlags& operator&=(AchievementFlags& flags, AchievementFlag other) { flags = flags | other; return flags; }


struct Achievement;
typedef nbl::FunctionRef<Result(const Achievement&)> AchievementCallback;

struct Achievement {
    i64 id;
    i64 setId;
    u64 number;
    StringView providerId;
    StringView title;
    StringView description;
    i64 icon;
    i64 lockedIcon;
    f64 globalUnlockRate;
    struct {
        u32 numerator;
        u32 denominator;
    } progress;
    u64 dateUnlocked;
    AchievementFlags flags;
    u32 borderColor;
    ConstSpan<u8> providerData;

    template<typename T>
    void getProviderData(T* output) const {
        if(providerData.length() != sizeof(T)) return;
        memcpy(output, providerData.data(), sizeof(T));
    }

    static Result add(DatabaseConnection* connection, const Achievement& achievement, i64* outId);
    static Result upsert(DatabaseConnection* connection, const Achievement& achievement, i64* outId);
    static Result upsertMeta(DatabaseConnection* connection, const Achievement& achievement, i64* outId);

    static Result update(DatabaseConnection* connection, const Achievement& achievement);
    static Result updateProgress(DatabaseConnection* connection, const Achievement& achievement);

    static Result remove(DatabaseConnection* connection, i64 id);
    static Result get(DatabaseConnection* connection, i64 id, AchievementCallback callback);

    static Result enumerateForGame(DatabaseConnection* connection, i64 limit, i64 offset, AchievementSortBy sortBy, SortOrder sortOrder, i64 gameId, AchievementCallback callback);
    static Result enumerateInSet(DatabaseConnection* connection, i64 limit, i64 offset, AchievementSortBy sortBy, SortOrder sortOrder, i64 setId, AchievementCallback callback);

    static Result countInSet(DatabaseConnection* connection, i64 setId, u64* outNumAchievements);
    static Result countUnlockedInSet(DatabaseConnection* connection, i64 setId, u64* outNumAchievements);

    static Result countForGame(DatabaseConnection* connection, i64 gameId, u64* outNumAchievements, bool includeHidden = false);
    static Result countUnlockedForGame(DatabaseConnection* connection, i64 gameId, u64* outNumAchievements, bool includeHidden = false);

};


struct AchievementSet;
typedef nbl::FunctionRef<Result(const AchievementSet&)> AchievementSetCallback;

struct AchievementSet {
    i64 id;
    i64 gameId;
    u64 lastUpdated;
    bool hidden;
    u32 awardColor;
    StringView name;
    StringView provider;
    StringView providerId;
    ConstSpan<u8> providerData;

    template<typename T>
    void getProviderData(T* output) const {
        if(providerData.length() != sizeof(T)) return;
        memcpy(output, providerData.data(), sizeof(T));
    }

	static Result add(DatabaseConnection* connection, const AchievementSet& achievementSet, i64* outId);
	static Result upsert(DatabaseConnection* connection, const AchievementSet& achievement, i64* outId);

	static Result update(DatabaseConnection* connection, const AchievementSet& achievementSet);
	static Result setHidden(DatabaseConnection* connection, i64 id, bool hidden);

	static Result remove(DatabaseConnection* connection, i64 id);
	static Result removeAllForProvider(DatabaseConnection* connection, i64 gameId, StringView provider);

	static Result get(DatabaseConnection* connection, i64 id, AchievementSetCallback callback);
	static Result enumerateForGame(DatabaseConnection* connection, i64 limit, i64 offset, i64 gameId, AchievementSetCallback callback);
	static Result countForGameAndProvider(DatabaseConnection* connection, i64 gameId, StringView providerId, u64* outNumSets);

};


struct AchievementState;
typedef nbl::FunctionRef<Result(const AchievementState&)> AchievementStateCallback;

struct AchievementState {
    i64 gameId;
    StringView provider;
    StringView fields;
    AchievementProviderStatus status;

    static Result enumerate(DatabaseConnection* connection, i64 gameId, AchievementStateCallback callback);
    static Result upsert(DatabaseConnection* connection, i64 gameId, const AchievementState& state);

};





}
