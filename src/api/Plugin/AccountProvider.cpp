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
 * \file Plugin/AccountProvider.cpp
 * \author jwall64
 * \brief Implementation of Plugin/AccountProvider.hpp
 */

#include <Plugin/AccountProvider.hpp>

#include <Core/Random.hpp>
#include <Core/Result.hpp>
#include <Codec/Base64.hpp>
#include <Platform/Time.hpp>
#include <Platform/Random.hpp>
#include <Network/HttpClient.hpp>
#include <Network/HttpServer.hpp>
#include <Plugin/Field.hpp>
#include <Data/Model.hpp>
#include <Data/MediaManager.hpp>
#include <Data/OnlineAccountModel.hpp>
#include <Utility/Hash.hpp>


void nbl::LoginTracker::complete(PluginContext* context, StringView username, StringView icon, OnlineAccountFlags flags, Span<FieldValue> values) {

    AllocationFrame frame(Arena::threadLocal());
    StringBuilder builder(&frame);
    nbl::Result res;
    if((res = nbl::encodeFieldValues(context, _pActive->fields, values, builder)).error()) {
        NBL_ERROR("Failed to encode field values for online account: ", res.value);
        _finishTracking(false, "Failed to encode field values.");
        return;
    }

    StringView fieldsStr;
    if((res = builder.buildNullTerminated(&fieldsStr)).error()) {
        NBL_ERROR("Failed to build fields string for online account: ", res.value);
        _finishTracking(false, "Failed to build fields string.");
        return;
    }

    InlineIcon iconId;
    if(flags & OnlineAccountFlag::IMAGE_ICON) {
        if((res = _pMediaManager->reserveReference(icon, ImageClass::INLINE_ICON, &iconId.imageId)).error()) {
            NBL_ERROR("Failed to save icon for online account: ", res.value);
            _finishTracking(false, "Failed to save account icon.");
            return;
        }
    } else {
        iconId.utf.length = min(static_cast<usize>(4), icon.length());
        memcpy(iconId.utf.data, icon.data(), iconId.utf.length);
    }

    nbl::OnlineAccount account={
        .id = _mAccount,
        .fields = fieldsStr,
        .provider = _pActive->id,
        .updatedAt = nbl::getCurrentTime(),
        .username = username,
        .icon = iconId.encoded,
        .flags = flags,
        .status = OnlineAccountStatus::LOGGED_IN
    };

    DatabaseConnection dc{};
    res = (DatabaseConnection::open(context->databasePath(), &dc));

    if(!res.success()) {
        _finishTracking(false, "Failed to open database.");
    }

    res = OnlineAccount::update(&dc, account);

    if(res.success()) {
        _finishTracking(true, {});
    } else {
        _finishTracking(false, "Failed to save account info to database.");
    }
}

nbl::Result nbl::LoginTracker::beginOAuthLogin(StringView url, StringView clientId, Span<StringView> scopes, u16 port, OAuthResponseType type, Span<char> challengeB64, StringBuilder* responseBuilder) {

    if(challengeB64.length() < 43 || challengeB64.length() > 128) {
        return nbl::Result::INVALID_ARGUMENT;
    }

    for(const StringView& scope : scopes) {
        for(char c : scope) {
            if(c == ' ' || c == '"' || c == '\\') return nbl::Result::INVALID_ARGUMENT;
        }
    }
    if(port == 0) {
        nbl::Random random{};
        random.seed(nbl::getCurrentTime());
        port = 40000 + (random.next() % 10000);
    } 


    // PKCE
    nbl::Array<u8, 96> challengeBytes;
    NBL_CHECK(nbl::secureRandom(challengeBytes));
    NBL_CHECK(nbl::encodeBase64(challengeBytes.span().slice(0, nbl::b64DecodedLength(challengeB64.length())), challengeB64, nbl::Base64CharacterSet::URL_SAFE));

    nbl::Array<u8, 32> hash{};
    NBL_CHECK(nbl::sha256(ConstSpan<u8>(reinterpret_cast<u8*>(challengeB64.data()), challengeB64.length()), hash));

    nbl::Array<char, 43> hashB64{};
    NBL_CHECK(nbl::encodeBase64(hash, hashB64, nbl::Base64CharacterSet::URL_SAFE));

    {
        StringView responseTypeStr{};
        switch(type) {
            default:
            case nbl::OAuthResponseType::NONE:
                responseTypeStr = "none";
                break;
            case nbl::OAuthResponseType::CODE:
                responseTypeStr = "code";
                break;
            case nbl::OAuthResponseType::TOKEN:
                responseTypeStr = "token";
                break;
        }

        nbl::AllocationFrame frame(nbl::Arena::threadLocal());
        StringView finalUrl;
        nbl::StringBuilder builder(&frame);
        builder
            .append(url)
            .append("?client_id=").append(UrlEncode(clientId))
            .append("&code_challenge=").append(StringView(hashB64.data, hashB64.size()))
            .append("&code_challenge_method=S256")
            .append("&response_type=").append(responseTypeStr)
            .append("&redirect_uri=http%3A%2F%2Flocalhost%3A").append(port);

        usize index = 0;
        if(scopes.length() > 0) {
            builder.append("&scope=");
            for(StringView& scope : scopes) {
                if(index++ > 0) {
                    builder.append("%20");
                }
                builder.append(UrlEncode(scope));
            }
        }
        NBL_CHECK(builder.build(&finalUrl));
        if(!_pfnOpenUrl(finalUrl, _pUserData)) {
            return nbl::Result::INVALID_STATE;
        }
    }

    NBL_CHECK(nbl::OAuthCallbackListener::listen(port, responseBuilder));

    if(type == OAuthResponseType::NONE) {
        return responseBuilder->retain(0, 0);
    }

    StringView path{};
    NBL_CHECK(responseBuilder->build(&path));

    usize index = 0;
    NBL_CHECK(path.findFirst('?', &index));

    StringView current = path.slice(index + 1);
    bool c = true;
    while(c) {
        Result findRes = current.findFirst('&', &index);
        if(findRes == Result::NOT_FOUND) {
            c = false;
            index = current.length();
        }
        StringView arg = current.slice(0, index);

        usize delim;
        NBL_CHECK(arg.findFirst('=', &delim));

        StringView field = arg.slice(0, delim);
        StringView value = arg.slice(delim + 1);

        NBL_DEBUG(field, '=', value);

        if((type == OAuthResponseType::CODE && field == "code")
        || (type == OAuthResponseType::TOKEN && field == "token")) {

            usize preLength = responseBuilder->length();
            NBL_CHECK(responseBuilder->appendChecked(UrlDecode(value)));
            usize responseLength = responseBuilder->length() - preLength;
            return responseBuilder->retainLast(responseLength);
        }

        if(c) {
            current = current.slice(index + 1);
        }
    }

    return nbl::Result::FORMAT_ERROR;
}


