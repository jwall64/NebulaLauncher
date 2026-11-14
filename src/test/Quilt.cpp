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

#include <gtest/gtest.h>

#include "ModLoader.hpp"

#include <Codec/Json.hpp>
#include <Platform/Arena.hpp>
#include <Platform/File.hpp>
#include <Platform/Time.hpp>
#include <Utility/StringBuilder.hpp>

namespace {

const char* BASE_VERSION = R"({
    "id": "1.20.1",
    "mainClass": "net.minecraft.client.main.Main",
    "assets": "5",
    "assetIndex": { "id": "5" },
    "javaVersion": { "component": "java-runtime-gamma" },
    "arguments": { "jvm": [], "game": [] },
    "libraries": [],
    "downloads": { "client": { "sha1": "x", "url": "x" } },
    "logging": { "client": { "argument": "-Dlog4j.configurationFile=${path}", "file": { "id": "client-1.12.xml", "sha1": "x", "size": 1, "url": "x" } } }
})";

const char* QUILT_PROFILE = R"({
    "id": "quilt-loader-0.20.0-beta.9-1.20.1",
    "inheritsFrom": "1.20.1",
    "type": "release",
    "mainClass": "org.quiltmc.loader.impl.launch.knot.KnotClient",
    "arguments": { "game": [] },
    "libraries": [
        { "name": "org.quiltmc:quilt-loader:0.20.0-beta.9", "url": "https://maven.quiltmc.org/repository/release/" }
    ]
})";

struct TempFolder {
    nbl::StringView path;

    explicit TempFolder(nbl::LinearAllocator* allocator) {
        EXPECT_TRUE(nbl::StringBuilder(allocator)
            .append(nbl::getTempDirectory())
            .append("/nebula_quilt_test_")
            .append(nbl::getCurrentTime())
            .buildNullTerminated(&path)
            .success());

        EXPECT_TRUE(nbl::makeDirectories(path).success());
    }
};

nbl::StringView makePath(nbl::LinearAllocator* allocator, nbl::StringView folder, nbl::StringView file) {
    nbl::StringView path{};
    EXPECT_TRUE(nbl::StringBuilder(allocator)
        .append(folder)
        .append('/')
        .append(file)
        .buildNullTerminated(&path)
        .success());
    return path;
}

void writeFile(nbl::StringView path, nbl::StringView contents) {
    nbl::FileChannel file{};
    ASSERT_TRUE(nbl::FileChannel::open(path, &file, nbl::FileOpenFlag::WRITE | nbl::FileOpenFlag::CREATE).success());
    ASSERT_TRUE(file.writer().writeAll(contents.data(), contents.length()).success());
}

}

TEST(Quilt, ReadsCachedProfile) {

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    TempFolder folder(&frame);
    nbl::StringView quiltFolder = makePath(&frame, folder.path, "quilt");
    ASSERT_TRUE(nbl::makeDirectories(quiltFolder).success());

    nbl::StringView profilePath = makePath(&frame, quiltFolder, "loader-1.20.1.json");
    writeFile(profilePath, nbl::StringView(QUILT_PROFILE));

    nbl::JsonElement baseElement{};
    ASSERT_TRUE(nbl::parseJson(nbl::StringView(BASE_VERSION), &frame, &baseElement).success());

    nbl::mc::VersionRef base{};
    base.parent = nullptr;
    base.root = baseElement.asObject();

    nbl::mc::VersionRef quilt{};
    quilt.parent = &base;
    quilt.root = {};

    ASSERT_TRUE(nbl::mc::generateQuiltVersionMeta(&quilt, folder.path, nullptr, &frame).success());

    EXPECT_EQ(quilt.root.getString("id"), "quilt-loader-0.20.0-beta.9-1.20.1");
    EXPECT_EQ(quilt.root.getString("inheritsFrom"), "1.20.1");
    EXPECT_EQ(quilt.id(), "quilt-loader-0.20.0-beta.9-1.20.1");
    EXPECT_EQ(quilt.mainClass(), "org.quiltmc.loader.impl.launch.knot.KnotClient");
    EXPECT_EQ(quilt.assets(), "5");
    EXPECT_EQ(quilt.root.getArray("libraries").count, 1);
}
