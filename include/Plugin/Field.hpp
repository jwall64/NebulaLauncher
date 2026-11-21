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
 * \file Plugin/Field.hpp
 * \author jwall64
 * \brief Field interface used in several plugin objects
 */

#pragma once

#include <Core/Types.hpp>
#include <Core/String.hpp>
#include <Codec/Json.hpp>
#include <Platform/Allocator.hpp>
#include <Utility/Placeholder.hpp>

#include <Core/FunctionRef.hpp>

namespace nbl {


enum class FieldType : u8 {
    STRING        = 0,  // A text box. Stored as a string.
    INTEGER       = 1,  // A number input box that only allows integers. Stored as a number.
    FLOAT         = 2,  // A number input box. Stored as a number.
    BOOLEAN       = 3,  // A checkbox. Stored as a boolean
    ENUM          = 4,  // A combobox that displays certain values. Stored as a string key.
    DATE          = 5,  // A text box that only allows dates. Stored as a timestamp
    ACCOUNT       = 6,  // A combobox that displays accounts from a certain provider
    COLOR         = 7,  // A color picker. Stored as a u32 in RGBA format
    LIST          = 8,  // Zero or more fields of the same type. Stored as a list of values
    STRUCT        = 9,  // A group of other fields. Stored as a list of key-value pairs
    ACTION        = 10, // A button. Does not store any value, but triggers a callback when clicked
    FIELD_ACTION  = 11, // Another field. Does not store any value, but triggers a callback when edited
};

// Affects how fields are saved, loaded, and displayed
enum class FieldFlag : u64 {

    HIDDEN = 0x1,                   // This field will not be visible in the UI
    SECRET = 0x2,                   // TODO: This field's value will be encrypted. The key should be placed in secret storage. Unimplemented because it breaks portability, and I'd need a way to let users import keys. (Just make them input a password? PBES2?)
    EPHEMERAL = 0x4,                // Not stored
    
    STRICT_PLACEHOLDERS = 0x10,     // Unmatched placeholders will cause an error to be thrown during serialization
    APPLY_GAME_PLACEHOLDERS = 0x20, // If this field belongs to an installation type, game-specific placeholders will be applied
    PASSWORD = 0x40,                // The field should echo dots instead of characters

    SHOW_DEFAULT_OPTION = 0x80,     // The "default" account option will be selectable
    ALLOW_TRANSPARENCY = 0x100,     // The alpha slider will be available in the color picker

    HORIZONTAL = 0x200              // Struct field inputs will be placed left-to-right instead of top-to-bottom
};


typedef u64 FieldFlags;

inline FieldFlags operator|(FieldFlag flag, FieldFlag other) { return static_cast<u64>(flag) | static_cast<u8>(other); }
inline FieldFlags operator|(FieldFlag flag, FieldFlags other) { return static_cast<u64>(flag) | other; }
inline FieldFlags operator|(FieldFlags other, FieldFlag flag) { return static_cast<u64>(flag) | other; }

inline FieldFlags operator|=(FieldFlag& flag, FieldFlag other) { return static_cast<u64>(flag) | static_cast<u8>(other); }
inline FieldFlags operator|=(FieldFlag& flag, FieldFlags other) { return static_cast<u64>(flag) | other; }
inline FieldFlags operator|=(FieldFlags& other, FieldFlag flag) { return static_cast<u64>(flag) | other; }

inline FieldFlags operator&(FieldFlag flag, FieldFlag other) { return static_cast<u64>(flag) & static_cast<u8>(other); }
inline FieldFlags operator&(FieldFlag flag, FieldFlags other) { return static_cast<u64>(flag) & other; }
inline FieldFlags operator&(FieldFlags other, FieldFlag flag) { return static_cast<u64>(flag) & other; }

inline FieldFlags operator&=(FieldFlag& flag, FieldFlag other) { return static_cast<u64>(flag) | static_cast<u8>(other); }
inline FieldFlags operator&=(FieldFlag& flag, FieldFlags other) { return static_cast<u64>(flag) & other; }
inline FieldFlags operator&=(FieldFlags& other, FieldFlag flag) { return static_cast<u64>(flag) & other; }


struct FieldValue {
    union {
        struct {
            StringView resolved; 
            StringView raw; // No placeholders applied
            inline void set(StringView str) noexcept {
                resolved = str;
                raw = str;
            }
        } stringValue; // for STRING
        i64 integerValue; // for INTEGER
        u64 uintegerValue; // for DATE
        f64 floatValue; // for FLOAT
        bool booleanValue; // for BOOLEAN
        u32 enumValue; // for ENUM
        union {
            u32 rgba;
            u8 channels[4];
            struct {
                u8 r;
                u8 g;
                u8 b;
                u8 a;
            };
        } colorValue; // for COLOR
        struct {
            i64 id;
            bool isDefault;
        } accountValue; // for ACCOUNT
        Span<FieldValue> compositeValue; // for LIST and STRUCT
    };
};

class PluginContext;

struct Field {
    FieldType type;
    StringView key;
    StringView name;
    union {
        struct {
            i64 min;
            i64 max;
        } integerData;
        struct {
            f64 min;
            f64 max;
        } floatData;
        struct {
            void (*pfnSuggest)(PluginContext* context, StringView current, nbl::FunctionRef<void(StringView)> suggestionConsumer, void* userData);
            void* userData;
            nbl::Span<nbl::Placeholder> customPlaceholders;

            void suggest(PluginContext* context, StringView current, nbl::FunctionRef<void(StringView)> suggestionConsumer) const {
                if(!pfnSuggest) return;
                pfnSuggest(context, current, suggestionConsumer, userData);
            }
        } stringData;
        struct {
            u32 (*pfnIndex)(PluginContext* context, StringView name, void* userData);
            StringView (*pfnName)(PluginContext* context, u32 index, void* userData);
            StringView (*pfnDisplayName)(PluginContext* context, u32 index, void* userData);
            u32 (*pfnCount)(PluginContext* context, void* userData);
            void* userData;

            u32 count(PluginContext* context) const { 
                if(!pfnCount) return 0;
                return pfnCount(context, userData); 
            }
            u32 index(PluginContext* context, StringView name) const { 
                if(!pfnIndex) return 0;
                return pfnIndex(context, name, userData); 
            }
            StringView name(PluginContext* context, u32 index) const { 
                if(!pfnName) return "";
                return pfnName(context, index, userData);
            }
            StringView displayName(PluginContext* context, u32 index) const { 
                if(!pfnDisplayName) return name(context, index);
                return pfnDisplayName(context, index, userData);
            }
        } enumData;
        struct {
            StringView accountProvider;
        } accountData;
        struct {
            Field* templateField;
            u32 min;
            u32 max;
        } listData;
        struct {
            Span<Field> fields;
        } structData;
        struct {
            void (*pfnActivate)(PluginContext* pluginContext, void* userData); // TODO: Add a handle back into the UI which allows the plugin to get/set other field values
            void* userData;

            void activate(PluginContext* pluginContext) const {
                if(!pfnActivate) return;
                return pfnActivate(pluginContext, userData);
            }
        } actionData;
        struct {
            Field* templateField;
            void (*pfnActivate)(PluginContext* pluginContext, FieldValue value, void* userData); // TODO: Add a handle back into the UI which allows the plugin to get/set other field values
            void* userData;

            void activate(PluginContext* pluginContext, FieldValue value) const {
                if(!pfnActivate) return;
                return pfnActivate(pluginContext, value, userData);
            }
        } fieldActionData;
    };
    FieldValue defaultValue;
    FieldFlags flags;
};

constexpr inline bool shouldEncodeField(Field field) {
    if(field.type == FieldType::ACTION || field.type == FieldType::FIELD_ACTION) return false;
    if(field.flags & FieldFlag::EPHEMERAL) return false;
    return true;
}

Result encodeFieldValues(PluginContext* context, Span<Field> fields, Span<FieldValue> values, Writer writer, Span<Placeholder> extraPlaceholders={});
Result decodeFieldValues(PluginContext* context, Span<Field> fields, JsonObject obj, Span<FieldValue>* outValues, LinearAllocator* allocator);

}
