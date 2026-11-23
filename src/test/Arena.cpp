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


#include <Platform/Arena.hpp>

TEST(Arena, Create)
{
    nbl::Arena arena;
    EXPECT_TRUE(nbl::Arena::create(1024, &arena).success());
    EXPECT_GE(arena.capacity(), 1024);
}

TEST(Arena, PushAndGet)
{
    nbl::Arena arena;
    EXPECT_TRUE(nbl::Arena::create(1024, &arena).success());

    int* value;
    EXPECT_TRUE(arena.allocate(&value).success());
    *value = 42;

    void* ptr = arena.begin();
    EXPECT_EQ(*static_cast<int*>(ptr), 42);
}

TEST(Arena, PushBeyondCapacityFails)
{
    nbl::Arena arena;
    ASSERT_TRUE(nbl::Arena::create(1024, &arena).success());

    void* ptr = nullptr;
    EXPECT_EQ(arena.push(arena.capacity() + 1, &ptr), nbl::Result::OUT_OF_MEMORY);
    EXPECT_EQ(ptr, nullptr);
    EXPECT_EQ(arena.index(), static_cast<usize>(0));
}
