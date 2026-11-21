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

#include <Core/Types.hpp>
#include <Core/String.hpp>

#include <Platform/File.hpp>
#include <Network/HttpClient.hpp>
#include <Codec/Json.hpp>
#include <Platform/Allocator.hpp>

#include "DownloadManager.hpp"

namespace nbl::mc {


struct VersionRef {
public:

    StringView id() const;
    StringView assets() const;
    StringView mainClass() const;
    StringView type() const;

    JsonObject assetIndex() const;
    JsonObject downloadInfo(StringView key, StringView* id) const;
    JsonObject loggingInfo(StringView key) const;
    JsonObject javaVersion() const;

    Result enumerateArguments(StringView key, nbl::FunctionRef<Result(JsonElement)> argumentConsumer) const;
    Result enumerateLibraries(nbl::FunctionRef<Result(JsonElement)> libraryConsumer) const;

    VersionRef* parent;
    JsonObject root;

    friend class VersionManager;
};


class VersionManager {
public:

    Result open(JsonObject manifestJson, StringView version,  DownloadManager* client, LinearAllocator* allocator, VersionRef* outRef);

    static Result create(StringView folder, VersionManager* out);

private:

    Result _download(JsonObject manifestJson, StringView version, StringView suggestedPath, DownloadManager* client, FileChannel* outFile);

    nbl::StringView _mFolderPath;
};


inline Result VersionManager::create(StringView folder, VersionManager* out) {
    out->_mFolderPath = folder;
    NBL_CHECK(nbl::makeDirectories(folder));
    return nbl::Result::SUCCESS;
}


}
