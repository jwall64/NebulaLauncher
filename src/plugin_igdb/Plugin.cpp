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
 * \file Plugin.cpp
 * \author jwall64
 * \brief IGDB plugin
 */

#include <Plugin/PluginInitializer.hpp>
#include <Plugin/GameProvider.hpp>
#include <Plugin/AccountProvider.hpp>
#include <Platform/Arena.hpp>
#include <Platform/File.hpp>
#include <Platform/Logging.hpp>
#include <Utility/StringBuilder.hpp>
#include <Plugin/PluginContext.hpp>
#include <Network/HttpClient.hpp>
#include <Codec/Json.hpp>
#include <Core/String.hpp>
#include <Core/Ring.hpp>
#include <Core/Stream.hpp>
#include <Platform/Time.hpp>
#include <Plugin/Field.hpp>
#include <Data/Model.hpp>

#include <utility>
#include "Data/OnlineAccountModel.hpp"


namespace {


enum _ExternalGameSource : u32 {
    EXT_STEAM = 1,
    EXT_GIANTBOMB = 3,
    EXT_GOG = 5,
    EXT_YOUTUBE = 10,
    EXT_MICROSOFT = 11,
    EXT_APPLE = 13,
    EXT_TWITCH = 14,
    EXT_ANDROID = 15,
    EXT_AMAZON = 20,
    EXT_AMAZON_LUNA = 22,
    EXT_AMAZON_ADG = 23,
    EXT_EGS = 26,
    EXT_OCULUS = 28,
    EXT_UTOMIK = 29,
    EXT_ITCHIO = 30,
    EXT_XBOX = 31,
    EXT_KARTRIDGE = 32,
    EXT_PS = 36,
    EXT_FOCUS = 37,
    EXT_XBOX_GPU_CLOUD = 54,
    EXT_GAMEJOLT = 55
};


nbl::StringView _externalSourceName(u32 index) {
    switch(index) {
        case EXT_STEAM: return "steam";
    	case EXT_GIANTBOMB: return "giantbomb";
    	case EXT_GOG: return "gog";
    	case EXT_YOUTUBE: return "youtube";
    	case EXT_MICROSOFT: return "microsoft_store";
    	case EXT_APPLE: return "app_store";
    	case EXT_TWITCH: return "twitch";
    	case EXT_ANDROID: return "play_store";
    	case EXT_AMAZON: return "amazon_appstore";
    	case EXT_AMAZON_LUNA: return "amazon_luna";
    	case EXT_AMAZON_ADG: return "amazon_digital_games";
    	case EXT_EGS: return "epic_games";
    	case EXT_OCULUS: return "oculus";
    	case EXT_UTOMIK: return "utomik";
    	case EXT_ITCHIO: return "itchio";
    	case EXT_XBOX: return "xbox";
    	case EXT_KARTRIDGE: return "kartridge";
    	case EXT_PS: return "playstation";
    	case EXT_FOCUS: return "focus_entertainment";
    	case EXT_XBOX_GPU_CLOUD: return "xbox_gamepass_ultimate_cloud";
    	case EXT_GAMEJOLT: return "gamejolt";
    }
    return "";
};


enum _TagMode : u32 {
    TAG_MODE_ALL = 0,
    TAG_MODE_NONE = 1,
    TAG_MODE_MAPPED = 2
};

u32 _tagModeIndex(nbl::PluginContext* context, nbl::StringView name, void* userData) {
    if(name == "none") return TAG_MODE_NONE;
    if(name == "mapped") return TAG_MODE_MAPPED;
    return TAG_MODE_ALL;
}

u32 _tagModeCount(nbl::PluginContext* context, void* userData) { return 3; }

nbl::StringView _tagModeName(nbl::PluginContext* context, u32 index, void* userData) {
    switch(index) {
        default:
        case TAG_MODE_ALL: return "all";
        case TAG_MODE_NONE: return "none";
        case TAG_MODE_MAPPED: return "mapped";
    }
}
nbl::StringView _tagModeDisplayName(nbl::PluginContext* context, u32 index, void* userData) {
    switch(index) {
        default:
        case TAG_MODE_ALL: return "All Tags";
        case TAG_MODE_NONE: return "No Tags";
        case TAG_MODE_MAPPED: return "Mapped Tags";
    }
}


struct _TagMapping {
    u64 tagHash;
    nbl::StringView igdbTagName;
    nbl::StringView mappedTagName;
};
nbl::Arena _configArena{};
nbl::Span<_TagMapping> _mappings{};
_TagMode _tagMode = TAG_MODE_ALL;


i32 _partitionMappings(_TagMapping* mappings, i32 low, i32 high) {

    u64 pivot = mappings[high].tagHash;
    i32 i = low - 1;

    for(i32 j = low; j <= high - 1; j++) {
        if(mappings[j].tagHash < pivot) {
            i++;
            std::swap(mappings[i], mappings[j]);
        }
    }

    std::swap(mappings[i + 1], mappings[high]);
    return i + 1;
}

void _quicksortMappings(_TagMapping* mappings, i32 low, i32 high) {

    if(low >= high) return;

    i32 pivotIndex = _partitionMappings(mappings, low, high);

    _quicksortMappings(mappings, low, pivotIndex - 1);
    _quicksortMappings(mappings, pivotIndex + 1, high);

}

nbl::Optional<nbl::StringView> _findMapping(nbl::StringView tag) {

    if(_mappings.length() == 0) return {};
    if(_mappings.length() == 1) {
        if(_mappings[0].igdbTagName == tag) {
            return _mappings[0].mappedTagName;
        }
        return {};
    }

    u64 hash = tag.hash();

    i32 low = 0;
    i32 high = _mappings.length() - 1;

    while(low <= high) {

        i32 index = low + (high - low) / 2;
        _TagMapping& midpointTag = _mappings[index];

        if(midpointTag.tagHash == hash) {
            if(midpointTag.igdbTagName == tag) {
                return midpointTag.mappedTagName;
            } else {
                // Hash collision. Extremely unlikely but we account for it.
                low = index;
                high = index + 1;
                while(low > 0) {
                    low--;
                    if(_mappings[low].tagHash != hash) break;
                    if(_mappings[low].igdbTagName == tag) return _mappings[low].mappedTagName;
                }
                while(high < _mappings.length() - 1) {
                    if(_mappings[high].tagHash != hash) break;
                    if(_mappings[high].igdbTagName == tag) return _mappings[high].mappedTagName;
                    high++;
                }
                return {};
            }
        }

        if(midpointTag.tagHash < hash) {
            low = index + 1;
        } else {
            high = index - 1;
        }
    }
    return {};
};


nbl::HttpClient _client{};

char _accessTokenBuffer[256];
char _clientIdBuffer[256];

nbl::StringView _accessToken;
nbl::StringView _clientId;
u64 _expiry;


nbl::Result _doLogin(nbl::StringView clientId, nbl::StringView clientSecret) {

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::StringView url;

    NBL_CHECK(nbl::StringBuilder(&frame)
            .append("https://id.twitch.tv/oauth2/token?client_id=")
            .append(nbl::UrlEncode(clientId))
            .append("&client_secret=")
            .append(nbl::UrlEncode(clientSecret))
            .append("&grant_type=client_credentials")
            .buildNullTerminated(&url));

    u64 preRequestTime = nbl::getCurrentTime();
    void* start = frame.end();
    NBL_CHECK(_client.makePostRequest(url, "", frame.writer()));

    u32 failures = 0;
    _client.awaitAll(&failures);
    if(failures > 0) {
        return nbl::Result::IO_ERROR;
    }

    nbl::StringView responseStr(static_cast<char*>(start), reinterpret_cast<usize>(frame.end()) - reinterpret_cast<usize>(start));
    nbl::JsonElement response;

    NBL_CHECK(nbl::parseJson(responseStr, &frame, &response));
    if(response.type() != nbl::JsonType::OBJECT) return nbl::Result::FORMAT_ERROR;

    nbl::StringView token = response.asObject().getString("access_token");
    if(token.length() > 256) return nbl::Result::FORMAT_ERROR;

    u64 expiresIn = response.asObject().getNumber("expires_in").asUint() * 1000;

    memcpy(_accessTokenBuffer, token.data(), token.length());
    memcpy(_clientIdBuffer, clientId.data(), clientId.length());

    _accessToken = nbl::StringView(_accessTokenBuffer, token.length());
    _clientId = nbl::StringView(_clientIdBuffer, clientId.length());
    _expiry = preRequestTime + expiresIn;

    return nbl::Result::SUCCESS;
}


nbl::Array _accountFields {
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "client_id",
        .name = "Client ID"
    },
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "client_secret",
        .name = "Client Secret",
        .flags = nbl::FieldFlag::PASSWORD | 0
    },
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "access",
        .name = "",
        .flags = nbl::FieldFlag::HIDDEN | 0
    },
    nbl::Field {
        .type = nbl::FieldType::DATE,
        .key = "expiry",
        .name = "",
        .flags = nbl::FieldFlag::HIDDEN | 0
    },
};


nbl::Result _login(nbl::PluginContext* context, nbl::Span<nbl::FieldValue> fields, nbl::LoginTracker* tracker, void* userData) {

    nbl::StringView clientId = fields[0].stringValue.raw;
    nbl::StringView clientSecret = fields[1].stringValue.raw;

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    NBL_CHECK(_doLogin(clientId, clientSecret));

    fields[2].stringValue.raw = _accessToken;
    fields[3].uintegerValue = _expiry;

    tracker->complete(context, "Twitch Developer Application", "", false, fields);

    return nbl::Result::SUCCESS;
}
nbl::Result _refresh(nbl::PluginContext* context, const nbl::OnlineAccount& account, nbl::LoginTracker* tracker, void* userData) {

    nbl::StringView fields = account.fields;

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::JsonElement element{};
    NBL_CHECK(nbl::parseJson(account.fields, &frame, &element));

    nbl::Span<nbl::FieldValue> values{};
    nbl::decodeFieldValues(context, _accountFields, element.asObject(), &values, &frame);

    nbl::StringView clientId = values[0].stringValue.raw;
    nbl::StringView clientSecret = values[1].stringValue.raw;
    nbl::StringView accessToken = values[2].stringValue.raw;

    if(clientSecret.length() > 256) {
        tracker->fail("Access token is too long");
        return nbl::Result::INVALID_ARGUMENT;
    }

    _expiry = values[3].uintegerValue;

    u64 now = nbl::getCurrentTime();
    if(now + (1000 * 60 * 60 * 24 * 3) < _expiry) {
        memcpy(_accessTokenBuffer, accessToken.data(), accessToken.length());
        _accessToken = nbl::StringView(_accessTokenBuffer, accessToken.length());

        memcpy(_clientIdBuffer, clientId.data(), clientId.length());
        _clientId = nbl::StringView(_clientIdBuffer, clientId.length());

        return nbl::Result::NO_CHANGE;
    }

    NBL_CHECK(_doLogin(clientId, clientSecret));

    values[2].stringValue.raw = _accessToken;
    values[3].uintegerValue = _expiry;

    tracker->complete(context, "Twitch Developer Application", "", false, values);

    return nbl::Result::SUCCESS;
}
nbl::Result _logout(nbl::PluginContext* context, i64 accountId, void* userData) {

    _accessToken = {};
    memset(_accessTokenBuffer, 0, 256);

    return nbl::Result::SUCCESS;
}

bool _canProvide(nbl::PluginContext* context, const nbl::OnlineGameLookupInfo& info, void* userData) {

    return !_accessToken.empty();
}

nbl::Result _provide(nbl::PluginContext* context, const nbl::OnlineGameLookupInfo& info, nbl::OnlineGameCallback callback, void* userData) {

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::StringView fields = "fields aggregated_rating,artworks.image_id,artworks.image_type,cover.image_id,cover.image_type,first_release_date,name,summary,external_games.uid,external_games.external_game_source";

    nbl::StringBuilder builder(&frame);
    builder.append(fields);
    if(_tagMode != TAG_MODE_NONE) {
        builder.append(",themes.slug,genres.slug,player_perspectives.slug");
    }
    builder.append("; limit 20;");

    if(info.meaning == nbl::OnlineGameLookupInfo::TITLE) {
        void* start = frame.end();
        usize startIndex = frame.index();
        NBL_CHECK(nbl::writeJson(frame.writer(), info.param)); // Write as a JSON string to add quotes escape control characters
        nbl::StringView escapedTitle(static_cast<char*>(start), frame.index() - startIndex);

        builder
            .append(" search ")
            .append(escapedTitle)
            .append(";");

    } else {

        nbl::JsonNumber id;
        NBL_CHECK(nbl::parseJsonNumber(info.param, &id));

        if(id.type != nbl::JsonNumberType::UNSIGNED) return nbl::Result::FORMAT_ERROR;

        builder
            .append(" where id = ")
            .append(id.asUint())
            .append(";");
    }

    nbl::StringView body;
    NBL_CHECK(builder.buildNullTerminated(&body));

    nbl::StringView auth;
    NBL_CHECK(nbl::StringBuilder(&frame).append("Bearer ").append(_accessToken).buildNullTerminated(&auth));

    nbl::Array<nbl::HttpHeader, 3> headers = {
        nbl::HttpHeader{ .name = "Client-ID", .value = _clientId },
        nbl::HttpHeader{ .name = "Authorization", .value = auth },
        nbl::HttpHeader{ .name = "Accept", .value = "application/json" }
    };

    char* start = static_cast<char*>(frame.end());
    NBL_CHECK(_client.makePostRequest("https://api.igdb.com/v4/games", body, frame.writer(), { .headers = headers } ));

    u32 failures = 0;
    _client.awaitAll(&failures);
    if(failures > 0) {
        return nbl::Result::IO_ERROR;
    }

    nbl::StringView responseStr(start, reinterpret_cast<usize>(frame.end()) - reinterpret_cast<usize>(start));
    nbl::JsonElement response;

    NBL_CHECK(nbl::parseJson(responseStr, &frame, &response));
    if(response.type() != nbl::JsonType::ARRAY) return nbl::Result::FORMAT_ERROR;

    for(nbl::JsonElement ele : response.asArray()) {

        nbl::AllocationFrame subFrame(nbl::Arena::threadLocal());

        if(ele.type() != nbl::JsonType::OBJECT) return nbl::Result::FORMAT_ERROR;
        nbl::JsonObject obj = ele.asObject();

        nbl::OnlineGameMeta meta{};

        u64 id = obj.getNumber("id").asUint();
        meta.rating = obj.getNumber("aggregated_rating").asFloat();
        meta.releaseDate = obj.getNumber("first_release_date").asUint() * 1000;

        meta.title = obj.getString("name");
        meta.description = obj.getString("summary");


        nbl::JsonObject cover = obj.getObject("cover");
        u64 coverImageType = cover.getNumber("image_type").asUint();
        if(coverImageType == 8) {
            NBL_CHECK(nbl::StringBuilder(&subFrame).append("https://images.igdb.com/igdb/image/upload/t_original/").append(cover.getString("image_id")).append(".png").build(&meta.posterUrl));
        }

        for(nbl::JsonElement element : obj.getArray("artworks")) {
            if(element.type() != nbl::JsonType::OBJECT) continue;

            u64 imageType = element.asObject().getNumber("image_type").asUint();
            if(meta.posterUrl.length() == 0 && (imageType == 8 || imageType == 10)) {
                NBL_CHECK(nbl::StringBuilder(&subFrame).append("https://images.igdb.com/igdb/image/upload/t_original/").append(element.asObject().getString("image_id")).append(".png").build(&meta.posterUrl));
            }
            if(meta.iconUrl.length() == 0 && (imageType == 11 || imageType == 13)) {
                NBL_CHECK(nbl::StringBuilder(&subFrame).append("https://images.igdb.com/igdb/image/upload/t_original/").append(element.asObject().getString("image_id")).append(".png").build(&meta.iconUrl));
            }
            if(meta.logoUrl.length() == 0 && (imageType == 5 || imageType == 6 || imageType == 7)) {
                NBL_CHECK(nbl::StringBuilder(&subFrame).append("https://images.igdb.com/igdb/image/upload/t_original/").append(element.asObject().getString("image_id")).append(".png").build(&meta.logoUrl));
            }
            if(meta.bannerUrl.length() == 0 && (imageType == 1 || imageType == 2 || imageType == 3)) {
                NBL_CHECK(nbl::StringBuilder(&subFrame).append("https://images.igdb.com/igdb/image/upload/t_original/").append(element.asObject().getString("image_id")).append(".png").build(&meta.bannerUrl));
                if(imageType == 3) { // Key art with logo
                    meta.logoPosition = nbl::LogoPosition::HIDDEN;
                }
            }
        }

        nbl::StringView idStr;
        NBL_CHECK(nbl::StringBuilder(&subFrame).append(id).build(&idStr));

        nbl::JsonArray arr = obj.getArray("external_games");

        meta.platformNames = nbl::Span<nbl::StringView>(nullptr, arr.count + 1);
        meta.platformIds = nbl::Span<nbl::StringView>(nullptr, arr.count + 1);
        usize platformCount = 0;

        subFrame.allocate(&meta.platformNames);
        subFrame.allocate(&meta.platformIds);

        meta.platformNames[platformCount] = "igdb";
        meta.platformIds[platformCount] = idStr;
        platformCount++;

        if(arr.count > 0) {

            for(nbl::JsonElement element : arr) {
                if(element.type() != nbl::JsonType::OBJECT) continue;
                nbl::JsonObject obj = element.asObject();

                nbl::StringView uid = obj.getString("uid");
                u64 sourceId = obj.getNumber("external_game_source").asUint();

                nbl::StringView platformName = _externalSourceName(sourceId);
                if(uid.empty() || platformName.empty()) continue;

                meta.platformNames[platformCount] = platformName;
                meta.platformIds[platformCount] = uid;
                platformCount++;
            }
        }

        if(_tagMode != TAG_MODE_NONE) {

            nbl::JsonArray themes = obj.getArray("themes");
            nbl::JsonArray genres = obj.getArray("genres");
            nbl::JsonArray perspectives = obj.getArray("player_perspectives");

            usize maxTagCount = themes.count + genres.count + perspectives.count;
            if(maxTagCount > 0) {

                meta.tags = nbl::Span<nbl::StringView>(nullptr, maxTagCount);
                usize tags = 0;

                nbl::Array allTags = { themes, genres, perspectives };
                NBL_CHECK(subFrame.allocate(&meta.tags));

                for(nbl::JsonArray a : allTags) {
                    for(nbl::JsonElement e : a) {
                        if(e.type() != nbl::JsonType::OBJECT) continue;
                        nbl::StringView slug = e.asObject().getString("slug");

                        nbl::StringView tag{};
                        NBL_CHECK(nbl::StringBuilder(&subFrame).append(slug)
                                .replace('-',"_")
                                .buildNullTerminated(&tag));

                        nbl::Optional<nbl::StringView> mapping = _findMapping(tag);
                        if(mapping.hasValue()) {
                            nbl::log(nbl::LogLevel::DEBUG, "Found mapping: ", tag, " -> ", mapping);
                            tag = *mapping;
                        } else if(_tagMode == TAG_MODE_MAPPED) continue;

                        if(tag.length() > 0) {
                            meta.tags[tags++] = tag;
                        }
                    }
                }

                meta.tags = meta.tags.head(tags);
            }
        }

        NBL_CHECK(callback(meta));
    }

    return nbl::Result::SUCCESS;
}


nbl::AccountProvider _provider {
    .id = "igdb",
    .name = "IGDB",
    .fields = _accountFields,
    .maxAccounts = 1,
    .pfnLogin = _login,
    .pfnRefresh = _refresh,
    .pfnLogout = _logout,
    .userData = nullptr,
};

nbl::GameProvider _gameProvider {
    .id = "igdb",
    .name = "IGDB",
    .pfnCanProvide = _canProvide,
    .pfnProvide = _provide,
    .userData = nullptr
};


nbl::Array _mappingFields = {
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "id",
        .name = "IGDB Tag Name"
    },
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "value",
        .name = "Nebula Tag Name"
    }
};

nbl::Field _listField = {
    .type = nbl::FieldType::STRUCT,
    .key = "",
    .name = "",
    .structData = {
        .fields = _mappingFields.span()
    },
    .defaultValue = {},
    .flags = nbl::FieldFlag::HORIZONTAL | 0
};


nbl::Array _pluginFields = {
    nbl::Field { 
        .type = nbl::FieldType::ENUM,
        .key = "tag_mode",
        .name = "Apply Tags",
        .enumData = {
            .pfnIndex = _tagModeIndex,
            .pfnName = _tagModeName,
            .pfnDisplayName = _tagModeDisplayName,
            .pfnCount = _tagModeCount,
            .userData = nullptr
        },
        .defaultValue = {
            .enumValue = TAG_MODE_ALL
        }
    },
    nbl::Field {
        .type = nbl::FieldType::LIST,
        .key = "tag_mappings",
        .name = "Tag Mappings",
        .listData = {
            .templateField = &_listField,
            .min = 0,
            .max = 999,
        }
    }
};


nbl::Result _loadConfig(nbl::Span<nbl::FieldValue> values) {

    _configArena.popTo(0);

    _TagMode tagMode = static_cast<_TagMode>(values[0].enumValue);
    if(tagMode != TAG_MODE_NONE && values[1].compositeValue.length() > 0) {

        _TagMapping* mappings;
        NBL_CHECK(_configArena.allocate(&mappings, values[1].compositeValue.length()));

        usize index = 0;
        for(nbl::FieldValue mapping : values[1].compositeValue) {

            void* igdb = _configArena.end();
            usize igdbIndex = _configArena.index();
            NBL_CHECK(_configArena.writer().writeString(mapping.compositeValue[0].stringValue.resolved));
            usize length = _configArena.index() - igdbIndex;

            void* mapped = _configArena.end();
            usize mappedIndex = _configArena.index();
            NBL_CHECK(_configArena.writer().writeString(mapping.compositeValue[1].stringValue.resolved));
            usize mappedLength = _configArena.index() - mappedIndex;

            mappings[index++] = _TagMapping {
                .tagHash = nbl::StringView(static_cast<char*>(igdb), length).hash(),
                .igdbTagName = nbl::StringView(static_cast<char*>(igdb), length),
                .mappedTagName = nbl::StringView(static_cast<char*>(mapped), mappedLength)
            };
        }

        _mappings = nbl::Span(mappings, values[1].compositeValue.length());
        _quicksortMappings(_mappings.data(), 0, _mappings.length() - 1);
    }

    return nbl::Result::SUCCESS;
}

}

extern "C" i32 NBL_EXPORT initPlugin(nbl::PluginInitContext* context) {

    nbl::log(nbl::LogLevel::INFO, "Initializing IGDB plugin...");
    NBL_CHECK(context->registerObject(&_provider));
    NBL_CHECK(context->registerObject(&_gameProvider));

    NBL_CHECK(nbl::Arena::create(1024 * 1024 * 4, &_configArena));

    context->setFields(_pluginFields, [](nbl::Span<nbl::Field> fields, nbl::Span<nbl::FieldValue> values, void* userData) {
        _loadConfig(values);
    }, nullptr);


    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    NBL_CHECK(nbl::HttpClient::create(4, 4, 0, 0, &_client));


    nbl::Span<nbl::FieldValue> values{};
    NBL_CHECK(context->getStoredFieldValues(_pluginFields, &values, &frame));

    NBL_CHECK(_loadConfig(values));

    return 0;
}
