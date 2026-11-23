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
 * \file Plugin/PluginContext.cpp
 * \author jwall64
 * \brief Implementation of Plugin/PluginContext.hpp
 */

#include <Plugin/PluginContext.hpp>

#include <Plugin/InstallationType.hpp>
#include <Plugin/AccountProvider.hpp>
#include <Plugin/MetadataProvider.hpp>
#include <Plugin/GameProvider.hpp>
#include <Core/Result.hpp>
#include <Core/Array.hpp>
#include <Core/Math.hpp>
#include <Platform/Process.hpp>
#include <Platform/Arena.hpp>
#include <Platform/File.hpp>
#include <Utility/StringBuilder.hpp>
#include <Data/Database.hpp>
#include <Data/Model.hpp>
#include <Data/DeviceModel.hpp>
#include <Data/MediaManager.hpp>


nbl::Result nbl::PluginContext::create(StringView deviceName, StringView databasePath, StringView mediaDbPath, StringView pluginFolder, StringView pluginDataPath, NotificationConsumer consumer, PluginContext* ctx) {

    NBL_CHECK(nbl::Arena::create(1024 * 1024 * 16, &ctx->_mArena));

    {
        nbl::AllocationFrame frame(&ctx->_mArena);
        NBL_CHECK(nbl::StringBuilder(&frame)
                .append(databasePath)
                .buildNullTerminated(&ctx->_mDatabasePath));

        NBL_CHECK(nbl::StringBuilder(&frame)
                .append(pluginDataPath)
                .buildNullTerminated(&ctx->_mPluginDataPath));

        StringView binaryPath = getBinaryPath();
        usize index;
        NBL_CHECK(binaryPath.findLast(NBL_PATH_SEPARATOR, &index));
        StringView binaryFolder = binaryPath.slice(0, index + 1);
        NBL_CHECK(nbl::StringBuilder(&frame)
                .append(binaryFolder)
                .append("Nebula_Wrapper")
                .buildNullTerminated(&ctx->_mWrapperPath));

        frame.release();
    }

    usize offset = ctx->_mArena.index() + 1;
    usize padding = nbl::alignUp(offset, sizeof(void*)) - ctx->_mArena.index();

    void* alloc;
    NBL_CHECK(ctx->_mArena.push(padding, &alloc));
    memset(alloc, 0, padding);


    NBL_CHECK(nbl::DatabaseConnection::open(ctx->_mDatabasePath, &ctx->_mConnection));
    NBL_CHECK(nbl::initDatabase(&ctx->_mConnection));

    i64 deviceId;
    NBL_ASSERT(nbl::HostDevice::upsert(&ctx->_mConnection, deviceName, &deviceId).success()); 
    nbl::log(nbl::LogLevel::DEBUG, "Device ID: ", deviceId);

    NBL_CHECK(nbl::MediaManager::create(mediaDbPath, deviceId, &ctx->_mMediaManager));

    ctx->_mNotificationConsumer = consumer;

    NBL_CHECK(nbl::PluginLoader::create(ctx, pluginFolder, &ctx->_mLoader));

    return nbl::Result::SUCCESS;
}

nbl::Result nbl::PluginContext::runCommand(i64 installationId, StringView command, StringView workingDir, ProcessHandle* handle) {

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::StringView idString;
    NBL_CHECK(nbl::StringBuilder(&frame).append(installationId).buildNullTerminated(&idString));

    nbl::Array args = {
        idString,
        _mDatabasePath,
        command
    };

    nbl::ProcessHandle::spawn(_mWrapperPath, args, handle, {
        .outputMode = ProcessOutputMode::PIPE,
        .workingDir = workingDir
    });

    return nbl::Result::SUCCESS;
}
