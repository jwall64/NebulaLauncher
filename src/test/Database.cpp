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


#include <Data/Database.hpp>
#include <Data/Model.hpp>
#include <Data/TagModel.hpp>

#include <gtest/gtest.h>


TEST(Database, SingleTransaction)
{
    nbl::DatabaseConnection connection;
    ASSERT_TRUE(nbl::DatabaseConnection::openInMemory(&connection).success());
    ASSERT_TRUE(nbl::initDatabase(&connection).success());

    i64 tagId = 0;
    ASSERT_TRUE(nbl::Tag::add(&connection, "Action", 0xFF0000, &tagId).success());

    bool found = false;
    ASSERT_TRUE(nbl::Tag::descendants(&connection, tagId, [&](nbl::Tag meta) -> nbl::Result {
        if(meta.name == "Action") found = true;
        return nbl::Result::SUCCESS;
    }).success());
    ASSERT_TRUE(found);
}

TEST(Database, NestedTransaction)
{
    nbl::DatabaseConnection connection;
    ASSERT_TRUE(nbl::DatabaseConnection::openInMemory(&connection).success());
    ASSERT_TRUE(nbl::initDatabase(&connection).success());

    ASSERT_TRUE(connection.beginTransaction().success());

    i64 outerTag = 0;
    ASSERT_TRUE(nbl::Tag::add(&connection, "Action", 0xFF0000, &outerTag).success());

    ASSERT_TRUE(connection.beginTransaction().success());

    i64 innerTag = 0;
    ASSERT_TRUE(nbl::Tag::add(&connection, "Adventure", 0x00FF00, &innerTag).success());

    ASSERT_TRUE(connection.commitTransaction().success());
    ASSERT_TRUE(connection.commitTransaction().success());

    bool foundOuter = false;
    ASSERT_TRUE(nbl::Tag::descendants(&connection, outerTag, [&](nbl::Tag meta) -> nbl::Result {
        if(meta.name == "Action") foundOuter = true;
        return nbl::Result::SUCCESS;
    }).success());
    ASSERT_TRUE(foundOuter);

    bool foundInner = false;
    ASSERT_TRUE(nbl::Tag::descendants(&connection, innerTag, [&](nbl::Tag meta) -> nbl::Result {
        if(meta.name == "Adventure") foundInner = true;
        return nbl::Result::SUCCESS;
    }).success());
    ASSERT_TRUE(foundInner);
}
