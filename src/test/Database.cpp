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

#include <Data/Database.hpp>
#include <Data/Model.hpp>

TEST(Database, MigratesVersionOne) {

    nbl::DatabaseConnection connection;
    ASSERT_TRUE(nbl::DatabaseConnection::openInMemory(&connection).success());

    // Minimal version 1 schema. initDatabase only needs the tables that the
    // migration adds indexes to.
    ASSERT_TRUE(connection.execute(R"(
CREATE TABLE installation (iId INTEGER PRIMARY KEY, gId INTEGER);
CREATE TABLE achievement_set (asId INTEGER PRIMARY KEY, gId INTEGER);
CREATE TABLE achievement (aId INTEGER PRIMARY KEY, asId INTEGER);
PRAGMA user_version = 1;
)").success());

    ASSERT_TRUE(nbl::initDatabase(&connection).success());

    nbl::PreparedStatement versionStmt{};
    ASSERT_TRUE(connection.prepare("PRAGMA user_version;", &versionStmt).success());

    i32 version = 0;
    ASSERT_TRUE(connection.execute(&versionStmt, [&](i32 argc, nbl::DataValue* values, const char**, i64) -> nbl::Result {
        version = values[0].intValue;
        return nbl::Result::SUCCESS;
    }).success());

    ASSERT_EQ(version, 2);

    nbl::PreparedStatement indexStmt{};
    ASSERT_TRUE(connection.prepare("SELECT COUNT(*) FROM sqlite_master WHERE type='index' AND name IN ('idx_installation_gId', 'idx_achievement_set_gId', 'idx_achievement_asId');", &indexStmt).success());

    i64 indexCount = 0;
    ASSERT_TRUE(connection.execute(&indexStmt, [&](i32 argc, nbl::DataValue* values, const char**, i64) -> nbl::Result {
        indexCount = values[0].intValue;
        return nbl::Result::SUCCESS;
    }).success());

    ASSERT_EQ(indexCount, 3);
}
