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
 * \file Data/MediaManager.cpp
 * \author jwall64
 * \brief Implementation of Data/MediaManager.hpp
 */

#include <Data/MediaManager.hpp>

#include <Core/Span.hpp>
#include <Core/Array.hpp>
#include <Core/Result.hpp>
#include <Core/Stream.hpp>
#include <Platform/Allocator.hpp>
#include <Platform/Arena.hpp>
#include <Utility/Hash.hpp>
#include <Utility/StringBuilder.hpp>
#include <Codec/Qoi.hpp>
#include <Data/Database.hpp>


nbl::Result nbl::MediaManager::reserveReference(StringView url, ImageClass ic, i64* outRefId) {

    auto lock = _mMutex.frameLock();

    static PreparedStatement _stmt{};
    if(!_stmt.isValid(&_mConnection)) {
        NBL_CHECK(_mConnection.prepare("INSERT INTO image_ref (url, urlDevice, imageClass) VALUES (?1, ?2, ?3) ON CONFLICT (url, urlDevice) DO UPDATE SET urlDevice=urlDevice RETURNING id;", &_stmt));
    }

    i64 devId = _mDeviceId;
    if(url.startsWith("https://") || url.startsWith("http://")) {
        devId = 0;
    }

    _stmt.bindAll(1, url, devId, ic.index);
    return _mConnection.execute(&_stmt, [&](i32 columns, nbl::DataValue* values, const char** names, i64 lastInsertRowId) -> nbl::Result {
        if(columns == 0) return nbl::Result::UNEXPECTED_ERROR;
        *outRefId = values[0].intValue;
        return nbl::Result::SUCCESS;
    });
}


nbl::Result nbl::MediaManager::reserveAndSave(StringView url, ImageClass imageClass, u32 width, u32 height, ConstSpan<u8> pixels, i64* outRefId) {

    DatabaseConnection* conn = &_mConnection;

    NBL_CHECK(conn->beginTransaction());

    NBL_CHECK_OR_ROLLBACK(conn, reserveReference(url, imageClass, outRefId));
    NBL_CHECK_OR_ROLLBACK(conn, saveImage(*outRefId, width, height, pixels));

    NBL_CHECK_OR_ROLLBACK(conn, conn->commitTransaction());

    return nbl::Result::SUCCESS;
}

nbl::Result nbl::MediaManager::saveImage(i64 refId, u32 width, u32 height, ConstSpan<u8> pixels) {

    if(width == 0 || height == 0) return nbl::Result::INVALID_ARGUMENT;
    if(pixels.length() < width * height * 4) return nbl::Result::INVALID_ARGUMENT;


    Arena arena{};
    NBL_CHECK(Arena::create((pixels.length() / 4) * 5 + 16, &arena));

    nbl::QoiImageMeta meta {
        .width = width,
        .height = height,
        .channels = 4,
        .colorSpace = ColorSpace::SRGB
    };

    u8* blobStart = static_cast<u8*>(arena.end());
    NBL_CHECK(QoiImageMeta::encodeImageData(&arena, meta, pixels));

    Span<u8> data(blobStart, static_cast<u8*>(arena.end()) - blobStart);

    Array<u8, 16> hash{};
    NBL_CHECK(md5(data, hash));
    
    if(_mCurrentlySaving++ == 0) {
        NBL_CHECK(_mConnection.beginTransaction());
    }

    auto lock = _mMutex.frameLock();

    static PreparedStatement _stmt{};
    if(!_stmt.isValid(&_mConnection)) {
        NBL_CHECK(_mConnection.prepare("INSERT INTO image (width, height, hash, data) VALUES (?1, ?2, ?3, ?4) ON CONFLICT (hash) DO UPDATE set width=width RETURNING id;", &_stmt));
    }
    static PreparedStatement _refStmt{};
    if(!_refStmt.isValid(&_mConnection)) {
        NBL_CHECK(_mConnection.prepare("UPDATE image_ref SET imageId=?1 WHERE id=?2;", &_refStmt));
    }


    Optional<i64> imageId{};

    DatabaseConnection* conn = &_mConnection;
    
    _stmt.bindAll(1, width, height, hash.span(), data);
    NBL_CHECK_OR_ROLLBACK(conn, conn->execute(&_stmt, [&](i32 columns, nbl::DataValue* values, const char** names, i64 lastInsertRowId) -> nbl::Result {
        if(columns == 0) return nbl::Result::UNEXPECTED_ERROR;
        imageId = values[0].intValue;
        return nbl::Result::SUCCESS;
    }));

    _refStmt.bindAll(1, *imageId, refId);
    NBL_CHECK_OR_ROLLBACK(conn, conn->execute(&_refStmt));

    if(--_mCurrentlySaving == 0) {
        NBL_CHECK(_mConnection.commitTransaction());
    }

    return nbl::Result::SUCCESS;
}


nbl::Result nbl::MediaManager::loadImage(i64 refId, ImageLoadCallback callback, ImageAllocationCallback alloc) {

    usize connIndex = 0;
    NBL_CHECK(_acquireLoadConnection(&connIndex));

    DatabaseConnection* conn = &_mLoadConnections[connIndex];
    PreparedStatement* stmt = &_mLoadStatements[connIndex];

    nbl::Result res = nbl::Result::SUCCESS;

    if(!stmt->isValid(conn)) {
        NBL_CHECK(conn->prepare("SELECT image.width, image.height, image.data, image_ref.url, image_ref.urlDevice, image_ref.imageClass FROM image_ref LEFT JOIN image ON image_ref.imageId = image.id WHERE image_ref.id = ?1;", stmt));
    }
    if(res.success()) {
        res = stmt->bind(1, refId);
    }

    bool found = false;
    if(res.success()) {
        res = conn->execute(stmt, [&](i32 columns, nbl::DataValue* values, const char** names, i64 lastInsertRowId) -> nbl::Result {
        if(found) {
            NBL_WARN("Two rows were returned for a single image ref!");
            return nbl::Result::SUCCESS;
        }
        found = true;
 

        if(values[2].type == DataType::NULL_TYPE) {
            StringView url = values[3].textValue;
            i64 device = values[4].intValue;

            return callback({
                .imageClass = values[5].intValue,
                .isPresent = false,
                .referenceData = {
                    .imageUrl = url,
                    .deviceId = device
                }
            });
        
        } else {
            u32 width = values[0].uintValue();
            u32 height = values[1].uintValue();
            ConstSpan<u8> qoiData = values[2].blobValue;

            QoiImageMeta meta = {
                .width = width,
                .height = height,
                .channels = 4,
                .colorSpace = ColorSpace::SRGB
            };

            u8* imageBuf;
            NBL_CHECK(alloc(width * height * 4, &imageBuf));

            Span<u8> imageSpan { imageBuf, width * height * 4 }; 

            NBL_CHECK(nbl::QoiImageMeta::decodeImageData(qoiData, meta, imageSpan));

            return callback({
                .imageClass = values[5].intValue,
                .isPresent = true,
                .imageData = {
                    .pixels = imageSpan,
                    .width = width,
                    .height = height
                }
            });
        }
        });
    }

    _releaseLoadConnection(connIndex);

    if(res.error()) return res;
    if(!found) return nbl::Result::NOT_FOUND;
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::MediaManager::_acquireLoadConnection(usize* outIndex) {
    _mLoadMutex.lock();
    while(true) {
        usize freeIndex = _LOAD_POOL_SIZE;
        for(usize i = 0; i < _LOAD_POOL_SIZE; i++) {
            if(!_mLoadInUse[i]) {
                freeIndex = i;
                break;
            }
        }
        if(freeIndex != _LOAD_POOL_SIZE) {
            _mLoadInUse[freeIndex] = true;
            *outIndex = freeIndex;
            _mLoadMutex.unlock();
            return nbl::Result::SUCCESS;
        }
        _mLoadCondition.wait(_mLoadMutex);
    }
}

void nbl::MediaManager::_releaseLoadConnection(usize index) {
    _mLoadMutex.lock();
    _mLoadInUse[index] = false;
    _mLoadCondition.signal();
    _mLoadMutex.unlock();
}


nbl::Result nbl::MediaManager::pruneUnused(ConstSpan<i64> liveRefs) {

    auto lock = _mMutex.frameLock();

    // Build a JSON array of the references that are still in use and let
    // SQLite turn it into a table. This keeps the statement within SQLite's
    // parameter limit and avoids a transaction, which could otherwise race
    // with image data being saved on a loader thread.
    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::StringView liveRefsJson{};
    nbl::StringBuilder builder(&frame);

    builder.append('[');
    for(usize i = 0; i < liveRefs.length(); i++) {
        if(i > 0) builder.append(',');
        builder.append(liveRefs[i]);
    }
    builder.append(']');
    NBL_CHECK(builder.build(&liveRefsJson));

    static PreparedStatement _refStmt{};
    if(!_refStmt.isValid(&_mConnection)) {
        NBL_CHECK(_mConnection.prepare("DELETE FROM image_ref WHERE id NOT IN (SELECT value FROM json_each(?1));", &_refStmt));
    }

    NBL_CHECK(_refStmt.bind(1, liveRefsJson));
    NBL_CHECK(_mConnection.execute(&_refStmt));

    static PreparedStatement _imageStmt{};
    if(!_imageStmt.isValid(&_mConnection)) {
        NBL_CHECK(_mConnection.prepare("DELETE FROM image WHERE NOT EXISTS (SELECT 1 FROM image_ref WHERE image_ref.imageId = image.id);", &_imageStmt));
    }

    return _mConnection.execute(&_imageStmt);
}


nbl::Result nbl::MediaManager::create(StringView dbPath, i64 deviceId, MediaManager* outManager) {

    DatabaseConnection* connection = &outManager->_mConnection;
    NBL_CHECK(DatabaseConnection::open(dbPath, connection));

    outManager->_mDeviceId = deviceId;

    for(usize i = 0; i < MediaManager::_LOAD_POOL_SIZE; i++) {
        NBL_CHECK(DatabaseConnection::open(dbPath, &outManager->_mLoadConnections[i]));
    }

    NBL_CHECK(nbl::Mutex::create(&outManager->_mLoadMutex));
    NBL_CHECK(nbl::Condition::create(&outManager->_mLoadCondition));

    i32 userVersion = 0;

    nbl::PreparedStatement stmt = {};
    NBL_CHECK(connection->prepare("PRAGMA user_version;", &stmt));

    NBL_CHECK(connection->execute(&stmt, [&](i32 argc, DataValue* argv, const char** columnNames, i64 lastInsertRowId) -> nbl::Result {
        userVersion = argv[0].intValue;
        return nbl::Result::SUCCESS;
    }));


    if(userVersion == 0) {

        nbl::log(nbl::LogLevel::DEBUG, "Initializing database...");
        nbl::StringView initQuery = R"(
PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;

BEGIN EXCLUSIVE TRANSACTION;

CREATE TABLE image (
    id INTEGER PRIMARY KEY,
    width INTEGER,
    height INTEGER,
    hash BLOB UNIQUE,
    data BLOB
);

CREATE TABLE image_ref (
    id INTEGER PRIMARY KEY,
    imageId INTEGER,
    url TEXT,
    urlDevice INTEGER NOT NULL DEFAULT 0,
    imageClass INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY (imageId) REFERENCES image(id) ON DELETE SET NULL,
    UNIQUE (url, urlDevice)
);

CREATE INDEX IF NOT EXISTS idx_image_ref_imageId ON image_ref(imageId);

PRAGMA user_version = 2;
COMMIT;
        )";

        NBL_CHECK(connection->execute(initQuery));
        return nbl::Result::SUCCESS;

    } else if(userVersion == 1) {

        nbl::StringView initQuery = R"(
PRAGMA foreign_keys = ON;
BEGIN TRANSACTION;
CREATE INDEX IF NOT EXISTS idx_image_ref_imageId ON image_ref(imageId);
PRAGMA user_version = 2;
COMMIT;
)";
        NBL_CHECK(connection->execute(initQuery));

        return nbl::Result::SUCCESS;

    } else if(userVersion == 2) {

        nbl::StringView initQuery = "PRAGMA foreign_keys = ON;";
        NBL_CHECK(connection->execute(initQuery));

        return nbl::Result::SUCCESS;
    } else {
        nbl::log(nbl::LogLevel::ERROR, "Unsupported media database version: ", userVersion);
        return nbl::Result::INVALID_STATE;
    }
}

