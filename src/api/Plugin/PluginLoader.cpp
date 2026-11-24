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
 * \file Plugin/PluginLoader.cpp
 * \author jwall64
 * \brief Implementation of Plugin/PluginLoader.hpp
 */

#include <Plugin/PluginLoader.hpp>

#include <Core/Result.hpp>
#include <Core/Stream.hpp>
#include <Utility/StringBuilder.hpp>
#include <Platform/File.hpp>
#include <Platform/DynamicLibrary.hpp>
#include <Platform/Allocator.hpp>
#include <Codec/Json.hpp>
#include <Plugin/PluginInitContext.hpp>
#include <Plugin/PluginContext.hpp>
#include <Plugin/InstallationType.hpp>
#include <Plugin/AccountProvider.hpp>
#include <Data/OnlineAccountModel.hpp>


nbl::Result nbl::PluginLoader::beginLoadAsync() {

    return nbl::Thread::create([](void* userData) -> i32 {
        return static_cast<PluginLoader*>(userData)->_loadAll();
    }, this, &_mLoadThread);
}

nbl::Result nbl::PluginLoader::awaitLoaded() {
    i32 result = 0;
    _mLoadThread.join(&result);
    return result;
}

nbl::Result nbl::PluginLoader::_loadAll() {

    nbl::FolderIterator it{};
    NBL_CHECK(nbl::FolderIterator::create(_mPluginFolder, &it));

    nbl::Arena registrationsTempArena{};
    NBL_CHECK(nbl::Arena::create(1024 * 1024 * 64, &registrationsTempArena));

    nbl::Registry::Builder builder(&registrationsTempArena);

    {
        PluginInitContext builtin{};
        LoadedPlugin builtinPlugin{};
        NBL_CHECK(PluginInitContext::create(&builder, &builtinPlugin, _pContext, {}, &builtin));

        builtin.registerObject(&InstallationType::COMMAND);
    }

    StringView pluginFolder;
    while(it.next(&pluginFolder)) {

        if(pluginFolder == "." || pluginFolder == "..") continue;

        nbl::log(nbl::LogLevel::DEBUG, "Attempting to load plugin in folder ", pluginFolder);

        nbl::AllocationFrame frame(nbl::Arena::threadLocal());
        nbl::StringView fullPath = {};
        NBL_CHECK(nbl::StringBuilder(&frame).append(_mPluginFolder).append(NBL_PATH_SEPARATOR).append(pluginFolder).build(&fullPath));

        nbl::Result res = _load(fullPath, &builder);
        if(res.error()) {
            nbl::log(nbl::LogLevel::WARN, "Unable to load plugin at ", pluginFolder, " (error ", res, ")");
        }
    }

    NBL_CHECK(builder.build(&_pContext->_mArena, &_pContext->_mRegistry));

    LoginTracker tracker{};
    NBL_CHECK(LoginTracker::create(
        [](bool success, StringView error, void* userData) {
            if(!success) {
                const AccountProvider* prov = static_cast<const AccountProvider*>(userData);
                NBL_ASSERT(prov != nullptr);
                NBL_ERROR("Failed to refresh account for ", prov->id, ": ", error);
            }
        },
        [](StringView message, void* userData) {
            NBL_DEBUG("Refresh tracker: ", message)
        },
        [](StringView url, void* userData) {
            NBL_WARN("Attempt to open browser during refresh!");
            return false;
        },
        _pContext->mediaManager(),
        &tracker
    ));

    NBL_CHECK(nbl::OnlineAccount::enumerate(_pContext->databaseConnection(), [&](const OnlineAccount& account) -> nbl::Result {

        if(account.status == OnlineAccountStatus::NOT_LOGGED_IN) {
            return OnlineAccount::remove(_pContext->databaseConnection(), account.id);
        }

        const nbl::AccountProvider* provider = _pContext->get<AccountProvider>(account.provider);
        if(provider == nullptr) return nbl::Result::SUCCESS;

        NBL_CHECK(tracker.startTracking(const_cast<AccountProvider*>(provider), account.id, provider));

        nbl::Result res = provider->refresh(_pContext, account, &tracker);
        if(res == Result::NO_CHANGE) {
            NBL_DEBUG("Plugin requested no change to account in DB");
            tracker._finishTracking(true, {});
        } else if(res.error()) {
            tracker.fail("An error occurred");
            NBL_CHECK(OnlineAccount::updateStatus(_pContext->databaseConnection(), account.id, OnlineAccountStatus::EXPIRED))
        } else if(tracker.isTracking()) {
            tracker.fail("The AccountProvider failed to call complete() or fail() before returning from refresh()");
            NBL_CHECK(OnlineAccount::updateStatus(_pContext->databaseConnection(), account.id, OnlineAccountStatus::EXPIRED))
        }
        return res;
    }));

    return nbl::Result::SUCCESS;
}


nbl::Result nbl::PluginLoader::_load(StringView folder, Registry::Builder* registrations) {

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    StringView manifestPath{};
    NBL_CHECK(StringBuilder(&frame).append(folder).append(NBL_PATH_SEPARATOR).append("plugin.json").buildNullTerminated(&manifestPath));

    FileChannel manifestStream{};
    NBL_CHECK(FileChannel::open(manifestPath, &manifestStream, FileOpenFlag::READ));

    JsonElement value{};
    NBL_CHECK(parseJson(manifestStream.reader(), &frame, &value));
    if(value.type() != JsonType::OBJECT) return nbl::Result::INVALID_ARGUMENT;

    StringView id = value.asObject().getString("id");
    StringView name = value.asObject().getString("name");
    StringView description = value.asObject().getString("description");
    StringView version = value.asObject().getString("version");

    if(id.empty() || name.empty() || version.empty()) return nbl::Result::INVALID_ARGUMENT;

    auto _init = [&](LoadedPlugin* lp) -> Result {
        NBL_CHECK(StringBuilder(&_mDataArena).append(id).buildNullTerminated(&lp->meta.id));
        NBL_CHECK(StringBuilder(&_mDataArena).append(name).buildNullTerminated(&lp->meta.name));
        NBL_CHECK(StringBuilder(&_mDataArena).append(description).buildNullTerminated(&lp->meta.description));
        NBL_CHECK(StringBuilder(&_mDataArena).append(version).buildNullTerminated(&lp->meta.version));

        NBL_DEBUG("Initializing plugin: ", id);

        StringView libraryPath = {};
        NBL_CHECK(StringBuilder(&frame).append(folder).append(NBL_PATH_SEPARATOR).append("plugin").append(dynamicLibraryFileExt()).buildNullTerminated(&libraryPath));

        NBL_CHECK(nbl::DynamicLibrary::load(libraryPath, &lp->lib));

        i32(*initFunc)(nbl::PluginInitContext*);
        NBL_CHECK(lp->lib.findSymbol("initPlugin", &initFunc));

        if(initFunc == nullptr) {
            NBL_ERROR("An error occurred while initializing a plugin! Could not find initPlugin()");
            return nbl::Result::PLUGIN_ERROR;
        }

        PluginInitContext ctx;
        NBL_CHECK(PluginInitContext::create(registrations, lp, _pContext, folder, &ctx));

        i32 rc = initFunc(&ctx);
        if(rc != 0) {
            NBL_ERROR("An error occurred while initializing a plugin! ", rc);
            return nbl::Result::PLUGIN_ERROR;
        }
        NBL_INFO("Initialized plugin: ", name, " version ", version);
        return nbl::Result::SUCCESS;
    };
    
    LoadedPlugin* ref = nullptr;
    NBL_CHECK(_mMetaArena.allocate(&ref));

    Result res = _init(ref);
    if(res.error()) {
        NBL_ERROR("Failed to initialize plugin ", id);
        _mMetaArena.popToPtr(ref);
    }

    return nbl::Result::SUCCESS;
}

void nbl::PluginLoader::_unloadAll() {
    for(LoadedPlugin& p : _mMetaArena.span().reinterpret<LoadedPlugin>()) {
        (&p)->~LoadedPlugin();
    }
    _mMetaArena.popTo(0);
    _mDataArena.popTo(0);
}

nbl::Result nbl::PluginLoader::create(PluginContext* context, StringView pluginFolder, PluginLoader* out) {

    NBL_CHECK(nbl::makeDirectories(pluginFolder));
    NBL_CHECK(nbl::Arena::create(1024 * 1024 * 32, &out->_mDataArena));
    NBL_CHECK(nbl::Arena::create(1024 * sizeof(LoadedPlugin), &out->_mMetaArena));

    NBL_CHECK(nbl::StringBuilder(&out->_mDataArena).append(pluginFolder).build(&out->_mPluginFolder));
    out->_pContext = context;

    return nbl::Result::SUCCESS;
}
