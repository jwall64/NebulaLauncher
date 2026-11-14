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

#include "ModLoader.hpp"

#include <Utility/StringBuilder.hpp>
#include <Platform/Time.hpp>
#include <Codec/Json.hpp>
#include <Platform/File.hpp>

#include "VersionManager.hpp"

nbl::Result nbl::mc::generateFabricVersionMeta(VersionRef* outVersion, StringView folder, DownloadManager* manager, LinearAllocator* allocator) {

    StringView versionId = outVersion->parent->id();

    StringView loaderPath{};
    NBL_CHECK(StringBuilder(allocator).append(folder).append("/fabric/loader-").append(versionId).append(".json").buildNullTerminated(&loaderPath));

    if(!nbl::fileExists(loaderPath)) {
        StringView url{};
        NBL_CHECK(StringBuilder(allocator).append("https://meta.fabricmc.net/v2/versions/loader/").append(versionId).buildNullTerminated(&url));
        NBL_CHECK(manager->downloadNow(loaderPath, url));
    }

    nbl::FileChannel file{};
    NBL_CHECK(nbl::FileChannel::open(loaderPath, &file, FileOpenFlag::READ));

    nbl::JsonElement element{};
    NBL_CHECK(nbl::parseJson(file.reader(), allocator, &element));
    if(element.type() != JsonType::ARRAY) return nbl::Result::FORMAT_ERROR;

    nbl::JsonObject versionObj = element.asArray().getObject(0);
    nbl::JsonObject loaderObj = versionObj.getObject("loader");

    StringView fabricVersion = loaderObj.getString("version");

    StringView fabricVersionId;
    NBL_CHECK(StringBuilder(allocator).append("fabric-loader-").append(fabricVersion).append('-').append(versionId).buildNullTerminated(&fabricVersionId));


    nbl::JsonObject::Builder builder{};
    NBL_CHECK(nbl::JsonObject::Builder::create(allocator, &builder));



    JsonObject launcherMeta = versionObj.getObject("launcherMeta");

    NBL_CHECK(builder.add("id", fabricVersionId));
    NBL_CHECK(builder.add("inheritsFrom", versionId));
    NBL_CHECK(builder.add("mainClass", launcherMeta.getObject("mainClass").getString("client")));
    NBL_CHECK(builder.add("type", loaderObj.getBoolean("stable") ? "release" : "snapshot"));

    // TODO: These are not being added. Likely an issue with JsonObject::Builder
    NBL_CHECK(builder.addObject("arguments", [&](auto& argObj) {
        return argObj.addArray("jvm", [&](auto& argArr) {
            return argArr.add("-DFabricMcEmu=net.minecraft.client.main.Main");
        });
    }));

    NBL_CHECK(builder.addArray("libraries", [&](auto& libArr) -> nbl::Result {
        for(JsonElement e : launcherMeta.getObject("libraries").getArray("client")) {
            NBL_CHECK(libArr.add(e));
        }
        for(JsonElement e : launcherMeta.getObject("libraries").getArray("common")) {
            NBL_CHECK(libArr.add(e));
        }
        libArr.addObject([&](auto& fabObj) -> nbl::Result {
            NBL_CHECK(fabObj.add("name", loaderObj.getString("maven")));
            NBL_CHECK(fabObj.add("url", "https://maven.fabricmc.net/"));
            return nbl::Result::SUCCESS;
        });
        return nbl::Result::SUCCESS;
    }));

    NBL_CHECK(builder.build(&outVersion->root));

    return nbl::Result::SUCCESS;
}

nbl::Result nbl::mc::generateQuiltVersionMeta(VersionRef* outVersion, StringView folder, DownloadManager* manager, LinearAllocator* allocator) {

    StringView versionId = outVersion->parent->id();

    StringView loaderPath{};
    NBL_CHECK(StringBuilder(allocator).append(folder).append("/quilt/loader-").append(versionId).append(".json").buildNullTerminated(&loaderPath));

    if(!nbl::fileExists(loaderPath)) {
        StringView url{};
        NBL_CHECK(StringBuilder(allocator).append("https://meta.quiltmc.org/v3/versions/loader/").append(versionId).append("/profile/json").buildNullTerminated(&url));
        NBL_CHECK(manager->downloadNow(loaderPath, url));
    }

    nbl::FileChannel file{};
    NBL_CHECK(nbl::FileChannel::open(loaderPath, &file, FileOpenFlag::READ));

    nbl::JsonElement element{};
    NBL_CHECK(nbl::parseJson(file.reader(), allocator, &element));
    if(element.type() != JsonType::OBJECT) return nbl::Result::FORMAT_ERROR;

    outVersion->root = element.asObject();

    return nbl::Result::SUCCESS;
}
