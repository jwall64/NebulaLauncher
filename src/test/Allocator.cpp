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

#include <Platform/Allocator.hpp>

TEST(Allocator, InPlaceAllocatorStartsEmpty) {

    char buffer[16];
    nbl::InPlaceAllocator allocator(buffer, sizeof(buffer));

    EXPECT_EQ(allocator.index(), 0);

    void* data = nullptr;
    EXPECT_EQ(allocator.push(4, &data), nbl::Result::SUCCESS);
    EXPECT_EQ(allocator.index(), 4);
    EXPECT_NE(data, nullptr);
}
