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
 * \brief SteamGridDB plugin
 */

#include <Plugin/PluginInitializer.hpp>
#include <Plugin/MetadataProvider.hpp>
#include <Plugin/AccountProvider.hpp>
#include <Platform/Arena.hpp>
#include <Platform/File.hpp>
#include <Platform/Logging.hpp>
#include <Utility/StringBuilder.hpp>
#include <Plugin/PluginContext.hpp>
#include <Network/HttpClient.hpp>
#include <Codec/Json.hpp>
#include <Core/String.hpp>
#include "Data/OnlineAccountModel.hpp"
#include "Plugin/Field.hpp"



namespace {

enum TriState : u32 {
    DENY = 0,
    ALLOW = 1,
    ALLOW_EXCLUSIVELY = 2
};

struct _Filters {
    u32 allowNsfw = DENY;
    u32 allowHumor = DENY;
    u32 allowFlash = DENY;
};

_Filters _filters;
char _tokenBuffer[64]{};
usize _tokenLength = 0;
nbl::HttpClient _httpClient{};

nbl::Array _accountFields = {
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "key",
        .name = "API Key"
    }
};

nbl::Result _login(nbl::PluginContext* context, nbl::Span<nbl::FieldValue> fields, nbl::LoginTracker* tracker, void* userData) {

    nbl::StringView token = fields[0].stringValue.raw;
    if(token.length() > 64) {
        tracker->fail("Token is too long");
        return nbl::Result::INVALID_ARGUMENT;
    }

    memcpy(_tokenBuffer, token.data(), token.length());
    _tokenLength = token.length();

    tracker->complete(context, "API Key", "", false, fields);
    return nbl::Result::SUCCESS;
}

nbl::Result _refresh(nbl::PluginContext* context, const nbl::OnlineAccount& account, nbl::LoginTracker* tracker, void* userData) {

    nbl::StringView fields = account.fields;

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::JsonElement element{};
    NBL_CHECK(nbl::parseJson(account.fields, &frame, &element));

    nbl::Span<nbl::FieldValue> values{};

    nbl::decodeFieldValues(context, _accountFields, element.asObject(), &values, &frame);

    nbl::StringView token = values[0].stringValue.raw;
    if(token.length() > 64) {
        tracker->fail("Token is too long");
        return nbl::Result::INVALID_ARGUMENT;
    }

    memcpy(_tokenBuffer, token.data(), token.length());
    _tokenLength = token.length();

    return nbl::Result::NO_CHANGE;
}


nbl::Result _logout(nbl::PluginContext* context, i64 account, void* userData) {

    _tokenLength = 0;
    memset(_tokenBuffer, 0, 64);

    return nbl::Result::SUCCESS;
}

bool _canProvide(nbl::PluginContext* context, const nbl::MetadataLookupInfo& info, void* userData) {
    return _tokenLength > 0 && (info.title.hasValue() || info.platformNames.contains("steam") || info.platformNames.contains("sgdb"));
}

nbl::Result _provide(nbl::PluginContext* context, const nbl::MetadataLookupInfo& info, nbl::MetadataCallback callback, void* userData) {

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::StringView authHeader;
    nbl::StringBuilder(&frame).append("Bearer ").append(nbl::StringView(_tokenBuffer, _tokenLength)).buildNullTerminated(&authHeader);

    nbl::HttpHeader header = {
        .name = "Authorization",
        .value = authHeader
    };

    nbl::StringView sgdbId;
    nbl::StringView steamId;
    for(usize i = 0 ; i < info.platformIds.length() ; i++) {
        if(info.platformNames[i] == "sgdb") {
            sgdbId = info.platformIds[i];
        } else if(info.platformNames[i] == "steam") {
            steamId = info.platformIds[i];
        }
    }

    if(sgdbId.empty()) {

        u64 intId;

        if(steamId.empty()) {

            usize begin = frame.index();
            if(!info.title.hasValue()) {
                return nbl::Result::INVALID_STATE;
            }

            nbl::StringView url;
            NBL_CHECK(nbl::StringBuilder(&frame)
                    .append("https://www.steamgriddb.com/api/v2/search/autocomplete/")
                    .append(nbl::UrlEncode(*info.title))
                    .buildNullTerminated(&url));

            void* responseStart = frame.end();
            NBL_CHECK(_httpClient.makeRequest(url, frame.writer(), { .headers = { &header, 1 } }));

            u32 numFailures = 0;
            _httpClient.awaitAll(&numFailures);
            if(numFailures > 0) return nbl::Result::IO_ERROR;

            usize responseBytes = reinterpret_cast<usize>(frame.end()) - reinterpret_cast<usize>(responseStart);
            nbl::StringView responseStr(static_cast<char*>(responseStart), responseBytes);

            nbl::JsonElement response{};
            NBL_CHECK(nbl::parseJson(responseStr, &frame, &response));

            if(response.type() != nbl::JsonType::OBJECT) return nbl::Result::FORMAT_ERROR;
            if(!response.asObject().getBoolean("success")) return nbl::Result::SUCCESS;

            nbl::JsonArray data = response.asObject().getArray("data");
            if(data.count == 0) return nbl::Result::SUCCESS;

            intId = data.getObject(0).getNumber("id").asUint();
            frame.popTo(begin);

        } else {

            usize begin = frame.index();
            nbl::StringView url;
            NBL_CHECK(nbl::StringBuilder(&frame)
                    .append("https://www.steamgriddb.com/api/v2/games/steam/")
                    .append(nbl::UrlEncode(steamId))
                    .buildNullTerminated(&url));

            void* responseStart = frame.end();
            NBL_CHECK(_httpClient.makeRequest(url, frame.writer(), { .headers = { &header, 1 } }));

            u32 numFailures = 0;
            _httpClient.awaitAll(&numFailures);
            if(numFailures > 0) return nbl::Result::IO_ERROR;


            usize responseBytes = reinterpret_cast<usize>(frame.end()) - reinterpret_cast<usize>(responseStart);
            nbl::StringView responseStr(static_cast<char*>(responseStart), responseBytes);

            nbl::JsonElement response{};
            NBL_CHECK(nbl::parseJson(responseStr, &frame, &response));

            if(response.type() != nbl::JsonType::OBJECT) return nbl::Result::FORMAT_ERROR;
            if(!response.asObject().getBoolean("success")) return nbl::Result::SUCCESS;

            nbl::JsonObject data = response.asObject().getObject("data");
            if(data.entryCount == 0) return nbl::Result::SUCCESS;

            intId = data.getNumber("id").asUint();
            frame.popTo(begin);
        }

        NBL_CHECK(nbl::StringBuilder(&frame).append(intId).build(&sgdbId));
    }

    {
        usize begin = frame.index();

        nbl::StringView gridUrl;
        NBL_CHECK(nbl::StringBuilder(&frame)
                .append("https://www.steamgriddb.com/api/v2/grids/game/")
                .append(nbl::UrlEncode(sgdbId))
                .append("?dimensions=600x900,512x512,1024x1024&limit=20")
                .append(_filters)
                .buildNullTerminated(&gridUrl)
        );

        void* responseStart = frame.end();
        NBL_CHECK(_httpClient.makeRequest(gridUrl, frame.writer(), { .headers = { &header, 1 } }));

        u32 numFailures = 0;
        _httpClient.awaitAll(&numFailures);
        if(numFailures > 0) return nbl::Result::IO_ERROR;

        usize responseBytes = reinterpret_cast<usize>(frame.end()) - reinterpret_cast<usize>(responseStart);
        nbl::StringView responseStr(static_cast<char*>(responseStart), responseBytes);

        nbl::JsonElement response{};
        NBL_CHECK(nbl::parseJson(responseStr, &frame, &response));

        if(response.type() == nbl::JsonType::OBJECT && response.asObject().getBoolean("success")) {
            for(nbl::JsonElement e : response.asObject().getArray("data")) {
                if(e.type() != nbl::JsonType::OBJECT) continue;

                u32 width = e.asObject().getNumber("width").asUint();
                u32 height = e.asObject().getNumber("height").asUint();
                nbl::MetadataField field = (width == height) ? nbl::MetadataField::ICON : nbl::MetadataField::POSTER;
                nbl::Result res = callback(field, { e.asObject().getString("url") });

                if(res != nbl::Result::SUCCESS) return res;
            }
        }
        frame.popTo(begin);
    }

    {
        usize begin = frame.index();

        nbl::StringView gridUrl;
        NBL_CHECK(nbl::StringBuilder(&frame)
                .append("https://www.steamgriddb.com/api/v2/heroes/game/")
                .append(nbl::UrlEncode(sgdbId))
                .append("?dimensions=1920x620,3840x1240&limit=20")
                .append(_filters)
                .buildNullTerminated(&gridUrl)
        );

        void* responseStart = frame.end();
        NBL_CHECK(_httpClient.makeRequest(gridUrl, frame.writer(), { .headers = { &header, 1 } }));

        u32 numFailures = 0;
        _httpClient.awaitAll(&numFailures);
        if(numFailures > 0) return nbl::Result::IO_ERROR;

        usize responseBytes = reinterpret_cast<usize>(frame.end()) - reinterpret_cast<usize>(responseStart);
        nbl::StringView responseStr(static_cast<char*>(responseStart), responseBytes);

        nbl::JsonElement response{};
        NBL_CHECK(nbl::parseJson(responseStr, &frame, &response));

        if(response.type() == nbl::JsonType::OBJECT && response.asObject().getBoolean("success")) {
            for(nbl::JsonElement e : response.asObject().getArray("data")) {
                if(e.type() != nbl::JsonType::OBJECT) continue;

                nbl::Result res = callback(nbl::MetadataField::BANNER, { e.asObject().getString("url") });

                if(res != nbl::Result::SUCCESS) return res;
            }
        }
        frame.popTo(begin);
    }

    {
        usize begin = frame.index();

        nbl::StringView gridUrl;
        NBL_CHECK(nbl::StringBuilder(&frame)
                .append("https://www.steamgriddb.com/api/v2/logos/game/")
                .append(nbl::UrlEncode(sgdbId))
                .append("?limit=20")
                .append(_filters)
                .buildNullTerminated(&gridUrl)
        );

        void* responseStart = frame.end();
        NBL_CHECK(_httpClient.makeRequest(gridUrl, frame.writer(), { .headers = { &header, 1 } }));

        u32 numFailures = 0;
        _httpClient.awaitAll(&numFailures);
        if(numFailures > 0) return nbl::Result::IO_ERROR;

        usize responseBytes = reinterpret_cast<usize>(frame.end()) - reinterpret_cast<usize>(responseStart);
        nbl::StringView responseStr(static_cast<char*>(responseStart), responseBytes);

        nbl::JsonElement response{};
        NBL_CHECK(nbl::parseJson(responseStr, &frame, &response));

        if(response.type() == nbl::JsonType::OBJECT && response.asObject().getBoolean("success")) {
            for(nbl::JsonElement e : response.asObject().getArray("data")) {
                if(e.type() != nbl::JsonType::OBJECT) continue;
                u64 pixels = e.asObject().getNumber("width").asUint() * e.asObject().getNumber("height").asUint();
                if(pixels < (32*32) || pixels > (4096*4096)) continue;

                nbl::Result res = callback(nbl::MetadataField::LOGO, { e.asObject().getString("url") });

                if(res != nbl::Result::SUCCESS) return res;
            }
        }
        frame.popTo(begin);
    }

    {
        usize begin = frame.index();

        nbl::StringView gridUrl;
        NBL_CHECK(nbl::StringBuilder(&frame)
                .append("https://www.steamgriddb.com/api/v2/icons/game/")
                .append(nbl::UrlEncode(sgdbId))
                .append("?limit=20")
                .append(_filters)
                .buildNullTerminated(&gridUrl)
        );

        void* responseStart = frame.end();
        NBL_CHECK(_httpClient.makeRequest(gridUrl, frame.writer(), { .headers = { &header, 1 } }));

        u32 numFailures = 0;
        _httpClient.awaitAll(&numFailures);
        if(numFailures > 0) return nbl::Result::IO_ERROR;

        usize responseBytes = reinterpret_cast<usize>(frame.end()) - reinterpret_cast<usize>(responseStart);
        nbl::StringView responseStr(static_cast<char*>(responseStart), responseBytes);

        nbl::JsonElement response{};
        NBL_CHECK(nbl::parseJson(responseStr, &frame, &response));

        if(response.type() == nbl::JsonType::OBJECT && response.asObject().getBoolean("success")) {
            for(nbl::JsonElement e : response.asObject().getArray("data")) {
                if(e.type() != nbl::JsonType::OBJECT) continue;
                u64 pixels = e.asObject().getNumber("width").asUint() * e.asObject().getNumber("height").asUint();
                if(pixels < (32*32) || pixels > (4096*4096)) continue;

                nbl::Result res = callback(nbl::MetadataField::ICON, { e.asObject().getString("url") });

                if(res != nbl::Result::SUCCESS) return res;
            }
        }
        frame.popTo(begin);
    }


    return nbl::Result::SUCCESS;
}

nbl::AccountProvider _provider {
    .id = "sgdb",
    .name = "SteamGridDB",
    .fields = _accountFields,
    .maxAccounts = 1,
    .pfnLogin = _login,
    .pfnRefresh = _refresh,
    .pfnLogout = _logout,
    .userData = nullptr
};

nbl::MetadataProvider _metaProvider {
    .id = "sgdb",
    .name = "SteamGridDB",
    .pfnCanProvide = _canProvide,
    .pfnProvide = _provide,
    .userData = nullptr
};

u32 _tristateIndex(nbl::PluginContext* context, nbl::StringView name, void* userData) {
    if(name == "true") return ALLOW_EXCLUSIVELY;
    if(name == "any") return ALLOW;
    return DENY;
}

u32 _tristateCount(nbl::PluginContext* context, void* userData) { return 3; }

nbl::StringView _tristateName(nbl::PluginContext* context, u32 index, void* userData) {
    switch(index) {
        default:
        case DENY: return "false";
        case ALLOW: return "any";
        case ALLOW_EXCLUSIVELY: return "true";
    }
}nbl::StringView _tristateDisplayName(nbl::PluginContext* context, u32 index, void* userData) {
    switch(index) {
        default:
        case DENY: return "Deny";
        case ALLOW: return "Allow";
        case ALLOW_EXCLUSIVELY: return "Allow Exclusively";
    }
}

nbl::Array _pluginFields {
    nbl::Field {
        .type = nbl::FieldType::ENUM,
        .key = "nsfw",
        .name = "NSFW Filter",
        .enumData = {
            .pfnIndex = _tristateIndex,
            .pfnName = _tristateName,
            .pfnDisplayName = _tristateDisplayName,
            .pfnCount = _tristateCount
        },
        .defaultValue = { .enumValue = DENY }
    },
        nbl::Field {
        .type = nbl::FieldType::ENUM,
        .key = "humor",
        .name = "Humor Filter",
        .enumData = {
            .pfnIndex = _tristateIndex,
            .pfnName = _tristateName,
            .pfnDisplayName = _tristateDisplayName,
            .pfnCount = _tristateCount
        },
        .defaultValue = { .enumValue = DENY }
    },
        nbl::Field {
        .type = nbl::FieldType::ENUM,
        .key = "epilepsy",
        .name = "Epilepsy Filter",
        .enumData = {
            .pfnIndex = _tristateIndex,
            .pfnName = _tristateName,
            .pfnDisplayName = _tristateDisplayName,
            .pfnCount = _tristateCount,
        },
        .defaultValue = { .enumValue = DENY }
    }
};

}

extern "C" i32 NBL_EXPORT initPlugin(nbl::PluginInitContext* context) {

    nbl::log(nbl::LogLevel::INFO, "Initializing SteamGridDB plugin...");
    NBL_CHECK(context->registerObject(&_provider));
    NBL_CHECK(context->registerObject(&_metaProvider));
    
    context->setFields(_pluginFields, [](nbl::Span<nbl::Field> fields, nbl::Span<nbl::FieldValue> fieldValues, void* userData) {
        _filters.allowNsfw = fieldValues[0].enumValue;
        _filters.allowHumor = fieldValues[1].enumValue;
        _filters.allowFlash = fieldValues[2].enumValue;
    }, nullptr);

    {
        nbl::AllocationFrame frame(nbl::Arena::threadLocal());
        nbl::Span<nbl::FieldValue> values{};
        NBL_CHECK(context->getStoredFieldValues(_pluginFields, &values, &frame));
        _filters.allowNsfw = values[0].enumValue;
        _filters.allowHumor = values[1].enumValue;
        _filters.allowFlash = values[2].enumValue;
    }

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::StringView pluginFolder;
    nbl::StringView apiKeyPath;

    NBL_CHECK(nbl::HttpClient::create(1, 1, 0, 0, &_httpClient));

    NBL_CHECK(nbl::StringBuilder(&frame).append(context->pluginDataPath()).append(NBL_PATH_SEPARATOR).append("sgdb").buildNullTerminated(&pluginFolder));
    NBL_CHECK(nbl::StringBuilder(&frame).append(pluginFolder).append(NBL_PATH_SEPARATOR).append("apikey.txt").buildNullTerminated(&apiKeyPath));
    NBL_CHECK(nbl::makeDirectories(pluginFolder));

    nbl::FileChannel channel;
    nbl::Result res = nbl::FileChannel::open(apiKeyPath, &channel, nbl::FileOpenFlag::READ);

    if(res.error() && res != nbl::Result::NOT_FOUND) {
        return res;
    }

    if(res == nbl::Result::SUCCESS) {
        usize justRead = 0;
        while((res = channel.reader().read(_tokenBuffer + _tokenLength, 64 - _tokenLength, &justRead)).success()) {
            _tokenLength += justRead;
        }
        if(res.error() && res != nbl::Result::END_OF_STREAM) {
            return res;
        }
    }


    return 0;
}

namespace nbl {
Result write(const _Filters& filters, Writer writer);
}

nbl::Result nbl::write(const _Filters& filters, Writer writer) {
    if(filters.allowNsfw != DENY) {
        NBL_CHECK(writer.writeString("&nsfw="));
        NBL_CHECK(writer.writeString(_tristateName(nullptr, filters.allowNsfw, nullptr)));
    }
    if(filters.allowHumor != DENY) {
        NBL_CHECK(writer.writeString("&humor="));
        NBL_CHECK(writer.writeString(_tristateName(nullptr, filters.allowHumor, nullptr)));
    }
    if(filters.allowFlash != DENY) {
        NBL_CHECK(writer.writeString("&epilepsy="));
        NBL_CHECK(writer.writeString(_tristateName(nullptr, filters.allowFlash, nullptr)));
    }
    return nbl::Result::SUCCESS;
}
