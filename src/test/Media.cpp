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

#include <Data/MediaManager.hpp>
#include <Platform/Arena.hpp>
#include <Platform/File.hpp>
#include <Platform/Time.hpp>
#include <Utility/StringBuilder.hpp>

namespace {

nbl::Result _ignoreImage(usize size, u8** out) {
    static u8 buffer[16];
    if(size > sizeof(buffer)) return nbl::Result::INVALID_ARGUMENT;
    *out = buffer;
    return nbl::Result::SUCCESS;
}

}

TEST(Media, PruneUnusedReferences) {

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    nbl::StringView dbPath{};
    ASSERT_TRUE(nbl::StringBuilder(&frame)
        .append(nbl::getTempDirectory())
        .append("/nebula_media_test_")
        .append(nbl::getCurrentTime())
        .append(".db")
        .buildNullTerminated(&dbPath)
        .success());

    nbl::deleteFile(dbPath);

    {
        nbl::MediaManager media;
        ASSERT_TRUE(nbl::MediaManager::create(dbPath, 1, &media).success());

        u8 red[4] = { 255, 0, 0, 255 };
        u8 green[4] = { 0, 255, 0, 255 };

        i64 liveId = 0;
        i64 staleId = 0;
        ASSERT_TRUE(media.reserveAndSave("https://example.com/live.png", nbl::ImageClass::ICON, 1, 1, nbl::ConstSpan<u8>(red, 4), &liveId).success());
        ASSERT_TRUE(media.reserveAndSave("https://example.com/stale.png", nbl::ImageClass::ICON, 1, 1, nbl::ConstSpan<u8>(green, 4), &staleId).success());

        // reserveAndSave stores the pixels immediately, without waiting for a
        // frontend request.
        bool livePresent = false;
        ASSERT_TRUE(media.loadImage(liveId, [&](const nbl::ImageLoadResult& result) -> nbl::Result {
            livePresent = result.isPresent;
            return nbl::Result::SUCCESS;
        }, _ignoreImage).success());
        ASSERT_TRUE(livePresent);

        i64 liveRefs[] = { liveId };
        ASSERT_TRUE(media.pruneUnused(nbl::ConstSpan<i64>(liveRefs, 1)).success());

        // The reference nothing points at is gone, and so is its image data.
        ASSERT_TRUE(media.loadImage(staleId, [](const nbl::ImageLoadResult&) -> nbl::Result {
            return nbl::Result::SUCCESS;
        }, _ignoreImage).error());

        // The live reference still loads.
        ASSERT_TRUE(media.loadImage(liveId, [&](const nbl::ImageLoadResult& result) -> nbl::Result {
            livePresent = result.isPresent;
            return nbl::Result::SUCCESS;
        }, _ignoreImage).success());
        ASSERT_TRUE(livePresent);
    }

    nbl::deleteFile(dbPath);
}
