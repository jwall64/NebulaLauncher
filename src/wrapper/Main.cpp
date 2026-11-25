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
 * \file Wrapper.cpp
 * \author jwall64
 * \brief Nebula Wrapper application. Ensures play time is still tracked even when the launcher is closed
 */

#include <Core/Result.hpp>
#include <Platform/Logging.hpp>
#include <Platform/Process.hpp>
#include <Platform/Time.hpp>
#include <Codec/Json.hpp>
#include <Data/Database.hpp>
#include <Data/Model.hpp>

int main(int argc, char** argv) {

    if(argc != 4) {
        nbl::log(nbl::LogLevel::ERROR, "Usage: ", nbl::StringView(argv[0]), " [iId] [/path/to/nebula.db] [command]");
        return 1;
    }

    i64 iId = 0;
    nbl::JsonNumber num = {};
    nbl::Result res = nbl::parseJsonNumber(argv[1], &num);

    if(res.error() || num.type == nbl::JsonNumberType::FLOAT) {
        nbl::log(nbl::LogLevel::ERROR, "Unable to parse installation ID!");
        return 1;
    }
    iId = num.asInt();

    {
        nbl::DatabaseConnection conn;
        res = nbl::DatabaseConnection::open(argv[2], &conn);
        if(res.error()) {
            nbl::log(nbl::LogLevel::WARN, "Unable to open the database!");
        } else {
            nbl::Installation::markJustPlayed(&conn, iId);
        }
    }

    u64 startTime = nbl::getCurrentTime();
    nbl::ProcessHandle proc;
    res = nbl::ProcessHandle::run(argv[3], &proc, {
        .outputMode = nbl::ProcessOutputMode::INHERIT,
        .outputRedirectFile = {},
        .killOnClose = true
    });
    if(res.error()) {
        nbl::log(nbl::LogLevel::ERROR, "Unable to start the program!");
        return 1;
    }

    i32 exitCode = 0;
    proc.wait(&exitCode);

    if(exitCode != 0) {
        nbl::log(nbl::LogLevel::WARN, "Process exited with code ", exitCode);
    }

    u64 endTime = nbl::getCurrentTime();

    {
        nbl::DatabaseConnection conn;
        res = nbl::DatabaseConnection::open(argv[2], &conn);
        if(res.error()) {
            nbl::log(nbl::LogLevel::WARN, "Unable to open the database!");
        } else {
            nbl::Installation::bumpPlayTime(&conn, iId, endTime - startTime);
        }
    }

    nbl::log(nbl::LogLevel::INFO, "Exiting wrapper...");
    return exitCode;
}
