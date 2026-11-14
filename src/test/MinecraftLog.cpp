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

#include "LogTransformer.hpp"

#include <Platform/Allocator.hpp>
#include <Utility/StringBuilder.hpp>

#include <string>

namespace {

struct LogParser {
    void* state = nullptr;

    LogParser() {
        EXPECT_EQ(nbl::mc::createMinecraftLogTransformer(nullptr, &state, nullptr), nbl::Result::SUCCESS);
    }

    ~LogParser() {
        nbl::mc::destroyMinecraftLogTransformer(nullptr, state, nullptr);
    }

    std::string feed(nbl::StringView line) {
        char buffer[4096];
        nbl::InPlaceAllocator allocator(buffer, sizeof(buffer));
        nbl::StringBuilder builder(&allocator);

        EXPECT_EQ(nbl::mc::transformMinecraftLogLine(nullptr, state, line, &builder, nullptr), nbl::Result::SUCCESS);

        nbl::StringView result{};
        EXPECT_EQ(builder.build(&result), nbl::Result::SUCCESS);

        return std::string(result.data(), result.length());
    }
};

}

TEST(MinecraftLog, FormatsSingleLineEvent) {
    LogParser parser;

    std::string output = parser.feed(
            "<log4j:event logger=\"net.minecraft.client.Minecraft\" timestamp=\"1700000000123\" level=\"INFO\" thread=\"Render thread\">"
            "<log4j:message><![CDATA[Hello world]]></log4j:message></log4j:event>");

    EXPECT_NE(output.find("[Render thread/INFO]: Hello world"), std::string::npos);
}

TEST(MinecraftLog, JoinsMultilineEvent) {
    LogParser parser;

    EXPECT_EQ(parser.feed("<log4j:event logger=\"minecraft\" timestamp=\"1700000000123\" level=\"WARN\" thread=\"Server thread\">"), "");
    EXPECT_EQ(parser.feed("<log4j:message><![CDATA[First line\nSecond line]]></log4j:message>"), "");
    std::string output = parser.feed("</log4j:event>");

    EXPECT_NE(output.find("[Server thread/WARN]: First line\nSecond line"), std::string::npos);
}

TEST(MinecraftLog, PassesThroughPlainText) {
    LogParser parser;

    EXPECT_EQ(parser.feed("this is not an xml log line"), "this is not an xml log line");
}

TEST(MinecraftLog, DropsBlankLines) {
    LogParser parser;

    EXPECT_EQ(parser.feed(""), "");
}

TEST(MinecraftLog, FallsBackToRawMalformedEvent) {
    LogParser parser;

    nbl::StringView line = "<log4j:event logger=\"minecraft\" level=\"INFO\" thread=\"main\"></log4j:event>";
    EXPECT_EQ(parser.feed(line), std::string(line.data(), line.length()));
}

TEST(MinecraftLog, RestoresEscapedCdataEnd) {
    LogParser parser;

    std::string output = parser.feed(
            "<log4j:event logger=\"minecraft\" timestamp=\"1700000000123\" level=\"INFO\" thread=\"main\">"
            "<log4j:message><![CDATA[before ]]]]><![CDATA[> after]]></log4j:message></log4j:event>");

    EXPECT_NE(output.find("[main/INFO]: before ]]> after"), std::string::npos);
}

TEST(MinecraftLog, FlushesUnclosedEventWhenAnotherStarts) {
    LogParser parser;

    nbl::StringView first = "<log4j:event logger=\"minecraft\" level=\"INFO\" thread=\"main\">";
    EXPECT_EQ(parser.feed(first), "");

    std::string output = parser.feed("<log4j:event logger=\"minecraft\" level=\"INFO\" thread=\"main\"></log4j:event>");

    EXPECT_NE(output.find(std::string(first.data(), first.length())), std::string::npos);
}

TEST(MinecraftLog, AppendsThrowableText) {
    LogParser parser;

    std::string output = parser.feed(
            "<log4j:event logger=\"minecraft\" timestamp=\"1700000000123\" level=\"ERROR\" thread=\"main\">"
            "<log4j:message><![CDATA[Something failed]]></log4j:message>"
            "<log4j:throwable><![CDATA[java.lang.Exception: nope\n\tat example.Broken.method(Broken.java:42)]]></log4j:throwable>"
            "</log4j:event>");

    EXPECT_NE(output.find("[main/ERROR]: Something failed\njava.lang.Exception: nope"), std::string::npos);
    EXPECT_NE(output.find("example.Broken.method"), std::string::npos);
}
