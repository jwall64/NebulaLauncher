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
 * \file Plugin/AccountProvider.hpp
 * \author jwall64
 * \brief Plugin-registerable object for plugins which rely on online accounts
 */

#pragma once

#include <Core/String.hpp>
#include <Platform/Mutex.hpp>
#include <Plugin/PluginContext.hpp>
#include <Data/OnlineAccountModel.hpp>
#include "Data/MediaManager.hpp"

namespace nbl {

enum class OAuthResponseType {
    NONE = 0,
    CODE = 1,
    TOKEN = 2
};

class LoginTracker {
public:

    inline Result startTracking(void* userData, i64 account, const AccountProvider* provider) {
        //auto lock = _mMutex.frameLock();

        if(!_bIdle) return nbl::Result::INVALID_STATE;
        _bIdle = false;
        _pUserData = userData;
        _mAccount = account;
        _pActive = provider;
        return nbl::Result::SUCCESS;
    }

    inline void postStatus(StringView message) {
        //auto lock = _mMutex.frameLock();
        if(_bIdle) return;
        _pfnStatus(message, _pUserData);
    }

    inline bool isTracking() { 
        //auto lock = _mMutex.frameLock();
        return !_bIdle; 
    }

    void fail(StringView message) {
        _finishTracking(false, message);
    }
    void complete(PluginContext* context, StringView username, StringView icon, OnlineAccountFlags flags, Span<FieldValue> values);
        
    Result beginOAuthLogin(StringView url, StringView clientId, Span<StringView> claims, u16 port, OAuthResponseType type, Span<char> challengeB64, StringBuilder* responseBuilder);

    static Result create(
            void (*pfnFinished)(bool success, StringView message, void* userData), 
            void (*pfnStatus)(StringView message, void* userData), 
            bool (*pfnOpenUrl)(StringView url, void* userData), 
            MediaManager* mediaManager,
            LoginTracker* outTracker) {

        if(pfnFinished == nullptr || pfnStatus == nullptr || pfnOpenUrl == nullptr || outTracker == nullptr) {
            return nbl::Result::INVALID_ARGUMENT;
        }

        //NBL_CHECK(nbl::Mutex::create(&outTracker->_mMutex));
        outTracker->_pfnFinished = pfnFinished;
        outTracker->_pfnStatus = pfnStatus;
        outTracker->_pfnOpenUrl = pfnOpenUrl;
        outTracker->_pMediaManager = mediaManager;
        outTracker->_bIdle = true;
        outTracker->_pUserData = nullptr;
        return nbl::Result::SUCCESS;
    }

private:

    inline void _finishTracking(bool success, StringView message) {
        //auto lock = _mMutex.frameLock();

        if(_bIdle) return;
        _pfnFinished(success, message, _pUserData);
        _bIdle = true;
    }

    bool _bIdle;
    void* _pUserData;
    i64 _mAccount;
    const AccountProvider* _pActive;
    void (*_pfnFinished)(bool, StringView, void*);
    void (*_pfnStatus)(StringView, void*);
    bool (*_pfnOpenUrl)(StringView, void*);
    MediaManager* _pMediaManager;
    //Mutex _mMutex;

    friend class PluginLoader;
};


struct AccountProvider {

    StringView id;
    StringView name;
    StringView loginText = "Login";
    Span<Field> fields;
    u32 maxAccounts;

    // Called when the user logs in through the UI
    Result(*pfnLogin)(PluginContext* context, Span<FieldValue> values, LoginTracker* tracker, void* userData);

    // Called when the application is loaded
    Result(*pfnRefresh)(PluginContext* context, const OnlineAccount& account, LoginTracker* tracker, void* userData);

    // Called when the user logs out through the UI
    Result(*pfnLogout)(PluginContext* context, i64 accountId, void* userData);

    // Passed to the above functions
    void* userData;


    Result login(PluginContext* context, Span<FieldValue> values, LoginTracker* tracker) const {
        if(!pfnLogin) return nbl::Result::PLUGIN_ERROR;
        return pfnLogin(context, values, tracker, userData);
    }

    Result refresh(PluginContext* context, const OnlineAccount& account, LoginTracker* tracker) const {
        if(!pfnRefresh) return nbl::Result::PLUGIN_ERROR;
        return pfnRefresh(context, account, tracker, userData);
    }

    Result logout(PluginContext* context, i64 accountId) const {
        if(!pfnLogout) return nbl::Result::PLUGIN_ERROR;
        return pfnLogout(context, accountId, userData);
    }

};


}
