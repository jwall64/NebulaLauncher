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

enum class TabFlag : u64 {
    COLORIZE_ICON = 0x1,
    HIDDEN = 0x2,
    IMAGE_ICON = 0x4,
};

typedef u64 TabFlags;

inline TabFlags operator|(TabFlag flag, TabFlag other) { return static_cast<u8>(flag) | static_cast<u8>(other); }
inline TabFlags operator|(TabFlag flag, TabFlags other) { return static_cast<u8>(flag) | other; }
inline TabFlags operator|(TabFlags other, TabFlag flag) { return static_cast<u8>(flag) | other; }

inline TabFlags& operator|=(TabFlags& flags, TabFlag other) { flags = flags | other; return flags; }

inline TabFlags operator&(TabFlag flag, TabFlag other) { return static_cast<u8>(flag) & static_cast<u8>(other); }
inline TabFlags operator&(TabFlag flag, TabFlags other) { return static_cast<u8>(flag) & other; }
inline TabFlags operator&(TabFlags other, TabFlag flag) { return static_cast<u8>(flag) & other; }

inline TabFlags& operator&=(TabFlags& flags, TabFlag other) { flags = flags | other; return flags; }


struct Tab;
typedef nbl::FunctionRef<Result(const Tab&)> TabCallback;


struct Tab {
    i64 tabId;
    StringView name;
    InlineIcon icon;
    TabFlags flags;

    static Result add(DatabaseConnection* connection, const Tab& meta, i64* outTabId);
    static Result get(DatabaseConnection* connection, i64 tabId, TabCallback callback);
    static Result update(DatabaseConnection* connection, const Tab& meta);
    static Result remove(DatabaseConnection* connection, i64 tabId);

    static Result count(DatabaseConnection* connection, u64* numTabs);
    static Result enumerate(DatabaseConnection* connection, u64 offset, u64 limit, TabCallback callback);
};




}

