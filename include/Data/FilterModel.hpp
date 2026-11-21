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

#include <Data/Common.hpp>

namespace nbl {

enum class FilterFlag : u64 {
    MULTI_LINE = 0x1,
    SHOW_TITLES = 0x2,
    USE_ICONS = 0x4,
    ONLY_INSTALLED = 0x8,
    SHOW_INSTALLATIONS = 0x10,
    COLORIZE_ICON = 0x20,
    SHOW_FILTER_INFO = 0x40,
    IMAGE_ICON = 0x80
};
typedef u64 FilterFlags;


inline FilterFlags operator&(FilterFlag flag, FilterFlag other) { return static_cast<FilterFlags>(flag) & static_cast<FilterFlags>(other); }
inline FilterFlags operator&(FilterFlag flag, FilterFlags other) { return static_cast<FilterFlags>(flag) & other; }
inline FilterFlags operator&(FilterFlags other, FilterFlag flag) { return static_cast<FilterFlags>(flag) & other; }

inline FilterFlags& operator&=(FilterFlags& flags, FilterFlag other) { flags = flags & other; return flags; }

inline FilterFlags operator|(FilterFlag flag, FilterFlag other) { return static_cast<FilterFlags>(flag) | static_cast<FilterFlags>(other); }
inline FilterFlags operator|(FilterFlag flag, FilterFlags other) { return static_cast<FilterFlags>(flag) | other; }
inline FilterFlags operator|(FilterFlags other, FilterFlag flag) { return static_cast<FilterFlags>(flag) | other; }

inline FilterFlags& operator|=(FilterFlags& flags, FilterFlag other) { flags = flags | other; return flags; }


struct Filter;
typedef nbl::FunctionRef<Result(const Filter&)> FilterCallback;
typedef nbl::FunctionRef<Result(const SearchCriteria&)> FilterCriteriaCallback;

struct Filter {
    i64 filterId;
    i64 tabId;
    i64 priority;
    StringView name;
    InlineIcon icon;
    FilterFlags flags;
    SearchCriteria criteria;

    static Result add(DatabaseConnection* connection, const Filter& meta, i64* outFilterId);
    static Result get(DatabaseConnection* connection, i64 filterId, FilterCallback callback);
    static Result update(DatabaseConnection* connection, const Filter& meta);
    static Result remove(DatabaseConnection* connection, i64 filterId);

    static Result count(DatabaseConnection* connection, u64* numFilters);

    static Result setCriteria(DatabaseConnection* connection, i64 filterId, const SearchCriteria& criteria);
    static Result getCriteria(DatabaseConnection* connection, i64 filterId, FilterCriteriaCallback callback);

    static Result enumerateForTab(DatabaseConnection* connection, i64 tabId, u64 offset, u64 limit, FilterCallback callback);

};




}
