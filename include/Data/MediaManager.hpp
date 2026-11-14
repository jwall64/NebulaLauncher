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
 * \file Data/MediaManager.hpp
 * \author jwall64
 * \brief Interface for managing media files in a SQLite database
 */



#pragma once

#include <Data/Database.hpp>
#include <Platform/Mutex.hpp>
#include <Core/Array.hpp>

namespace nbl {


struct ImageClass {
    u8 index;

    ImageClass(u8 value) : index(value) {}
    operator u8() { return index; }

    enum Value : u8 {
        UNKNOWN,
        BANNER,
        POSTER,
        ICON,
        LOGO,
        INLINE_ICON,
        ACHIEVEMENT_ICON
    };

    struct Dimensions {
        u32 width;
        u32 height;
    };

    Dimensions dimensions() {
        switch (index) {
            default:
            case UNKNOWN:
                return { 0, 0 };
            case BANNER:
                return { 600, 900 };
            case POSTER:
                return { 3840, 1240 };
            case ICON:
            case LOGO:
                return { 1024, 1024 };
            case INLINE_ICON:
            case ACHIEVEMENT_ICON:
                return { 256, 256 };
        }
    }

    bool shouldCrop() {
        return index < LOGO;
    }
};

struct ImageLoadResult {
    ImageClass imageClass;
    bool isPresent;
    union {
        struct {
            StringView imageUrl;
            i64 deviceId;
        } referenceData;
        struct {
            Span<u8> pixels;
            u32 width;
            u32 height;
        } imageData;
    };
};


typedef std::function_ref<Result(const ImageLoadResult&)> ImageLoadCallback;
typedef std::function_ref<Result(usize, u8**)> ImageAllocationCallback;


class MediaManager {
public:

    Result reserveReference(StringView url, ImageClass imageClass, i64* outRefId);
    Result reserveAndSave(StringView url, ImageClass imageClass, u32 width, u32 height, ConstSpan<u8> pixels, i64* outRefId);

    Result saveImage(i64 refId, u32 width, u32 height, ConstSpan<u8> pixels);
    Result loadImage(i64 imageId, ImageLoadCallback res, ImageAllocationCallback alloc);

    Result pruneUnused(ConstSpan<i64> liveRefs);

    i64 deviceId() const { return _mDeviceId; }

    static Result create(StringView dbPath, i64 deviceId, MediaManager* outManager);

private:

    static constexpr usize _LOAD_POOL_SIZE = 8;

    DatabaseConnection _mConnection;
    Mutex _mMutex;
    i64 _mDeviceId;
    std::atomic<u32> _mCurrentlySaving;

    Array<DatabaseConnection, _LOAD_POOL_SIZE> _mLoadConnections{};
    Mutex _mLoadMutex;
    Condition _mLoadCondition;
    Array<bool, _LOAD_POOL_SIZE> _mLoadInUse{};
    Array<PreparedStatement, _LOAD_POOL_SIZE> _mLoadStatements{};

    Result _acquireLoadConnection(usize* outIndex);
    void _releaseLoadConnection(usize index);
};


}

