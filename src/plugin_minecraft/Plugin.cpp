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

#include <Core/Array.hpp>
#include <Core/String.hpp>
#include <Utility/StringBuilder.hpp>
#include <Data/OnlineAccountModel.hpp>
#include <Plugin/PluginContext.hpp>
#include <Plugin/PluginInitializer.hpp>
#include <Plugin/InstallationType.hpp>
#include <Plugin/AccountProvider.hpp>
#include <Plugin/Field.hpp>

#include "MinecraftLauncher.hpp"
#include "AuthManager.hpp"
#include "LogTransformer.hpp"

namespace {


nbl::mc::MinecraftLauncher _launcher{};

nbl::LogTransformFunction _logTransformer = {
    .pfnCreate = nbl::mc::createMinecraftLogTransformer,
    .pfnTransform = nbl::mc::transformMinecraftLogLine,
    .pfnDestroy = nbl::mc::destroyMinecraftLogTransformer,
    .userData = nullptr
};


// TODO: Fields custom JVM args, wrapper program, etc.
nbl::Array _fields = {
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "version",
        .name = "Version",
        .stringData = {
            .pfnSuggest = [](
                    nbl::PluginContext* context, 
                    nbl::StringView current, 
                    std::function_ref<void(nbl::StringView)> suggestionConsumer, 
                    void* userData) {

                nbl::mc::MinecraftLauncher* mc = static_cast<nbl::mc::MinecraftLauncher*>(userData);
                mc->suggestVersions(context, current, suggestionConsumer);
            },
            .userData = &_launcher
        }
    },
    nbl::Field {
        .type = nbl::FieldType::ACCOUNT,
        .key = "account",
        .name = "Account",
        .accountData = {
            .accountProvider = "minecraft",
        },
        .defaultValue = {
            .accountValue = {
                .id = 0,
                .isDefault = true
            }
        },
        .flags = nbl::FieldFlag::SHOW_DEFAULT_OPTION | 0
    },
    nbl::Field {
        .type = nbl::FieldType::ENUM,
        .key = "mod_loader",
        .name = "Mod Loader",
        .enumData = {
            .pfnIndex = nbl::mc::ModLoader::index,
            .pfnName = nbl::mc::ModLoader::name,
            .pfnDisplayName = nbl::mc::ModLoader::displayName,
            .pfnCount = nbl::mc::ModLoader::count,
        }
    },
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "working_dir",
        .name = "Working Directory"
    }
};


nbl::AccountProvider _provider {
    .id = "minecraft",
    .name = "Minecraft",
    .loginText = "Login With Browser",
    .fields = nbl::mc::MC_AUTH_FIELDS,
    .maxAccounts = 16,
    .pfnLogin = nbl::mc::login,
    .pfnRefresh = nbl::mc::refresh,
    .pfnLogout = nbl::mc::logout,
    .userData = &_launcher
};

nbl::Result _launch(
        const nbl::InstallationType* type, 
        nbl::PluginContext* ctx, 
        nbl::Span<nbl::FieldValue> fields, 
        const nbl::Installation& meta,
        nbl::ProcessTracker* tracker, 
        void* userData) {

    nbl::OnlineAccount acc;
    if(fields[1].accountValue.isDefault) {

        return nbl::OnlineAccount::getFirst(ctx->databaseConnection(), _provider.id, [&](const nbl::OnlineAccount& account) {
            return _launcher.launch(ctx, fields[0].stringValue.resolved, account, fields[2].enumValue, fields[3].stringValue.resolved, meta.id, tracker);
        });

    } else {

        return nbl::OnlineAccount::get(ctx->databaseConnection(), fields[1].accountValue.id, [&](const nbl::OnlineAccount& account) -> nbl::Result {
            if(account.provider != _provider.id) return nbl::Result::INVALID_ARGUMENT;
            return _launcher.launch(ctx, fields[0].stringValue.resolved, account, fields[2].enumValue, fields[3].stringValue.resolved, meta.id, tracker);
        });
    }

}

nbl::Result _getFolder(
        const nbl::InstallationType* type, 
        nbl::PluginContext* ctx, 
        nbl::Span<nbl::FieldValue> fields, 
        const nbl::Installation& meta, 
        nbl::StringView* outFolder,
        void* userData) {

    *outFolder = fields[3].stringValue.resolved;
    return nbl::Result::SUCCESS;
}


nbl::InstallationType _type = {
    .id = "minecraft",
    .name = "Minecraft",
    .fields = _fields,
    .launchFunc = {
        .pfnLaunchInstallation = _launch,
        .userData = nullptr
    },
    .getFolderFunc = {
        .pfnGetFolder = _getFolder,
        .userData = nullptr
    },
    .logTransform = _logTransformer
};


}


extern "C" i32 initPlugin(nbl::PluginInitContext *context) {

    {
        nbl::StringView launcherRoot{};
        nbl::AllocationFrame frame(nbl::Arena::threadLocal());
        NBL_CHECK(nbl::StringBuilder(&frame).append(context->pluginDataPath()).append("/minecraft").buildNullTerminated(&launcherRoot));
        NBL_CHECK(nbl::mc::MinecraftLauncher::init(launcherRoot, &_launcher));
    }
    context->registerObject(&_type);
    context->registerObject(&_provider);

    return 0;
}
