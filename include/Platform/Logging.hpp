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
 * \file Platform/Logging.hpp
 * \author jwall64
 * \brief Functions for writing to standard output and standard error
 */

#pragma once

#include <Core/Types.hpp>
#include <Core/String.hpp>
#include <Core/Stream.hpp>

#include <utility>

/**
 * \brief The default log level for the logging system. If NBL_LOG_LEVEL is not defined before including this header, it will be set to 2 (INFO) in release builds and 1 (DEBUG) in debug builds. This allows for filtering of log messages based on their severity, with higher log levels including messages from lower levels.
 */
#ifndef NBL_LOG_LEVEL
#ifdef NDEBUG
#define NBL_LOG_LEVEL 2
#else
#define NBL_LOG_LEVEL 1
#endif
#endif



#pragma push_macro("stdout")
#pragma push_macro("stderr")

#undef stdout
#undef stderr


namespace nbl {

/**
 * \brief An enumeration representing different log levels for categorizing log messages based on their severity. The log levels are ordered from least severe (TRACE) to most severe (FATAL), allowing for filtering of log messages based on the configured log level.
 */
enum class LogLevel : usize {
    TRACE = 0,
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5
};

/**
 * \brief Comparison operators for LogLevel that allow for comparing log levels with usize values. These operators enable filtering of log messages based on their severity by comparing the log level of a message with the configured log level threshold.
 * \param l1 The LogLevel to compare.
 * \param val The usize value representing the log level threshold to compare against.
 * \return true if the LogLevel is greater than (more severe than) the threshold, false otherwise.
 */
inline bool operator>(LogLevel l1, usize val) {
    return static_cast<usize>(l1) > val;
}

/**
 * \brief Comparison operators for LogLevel that allow for comparing log levels with usize values. These operators enable filtering of log messages based on their severity by comparing the log level of a message with the configured log level threshold.
 * \param l1 The LogLevel to compare.
 * \param val The usize value representing the log level threshold to compare against.
 * \return true if the LogLevel is less than (less severe than) the threshold, false otherwise.
 */
inline bool operator<(LogLevel l1, usize val) {
    return static_cast<usize>(l1) < val;
}

/**
 * \brief Logs a message to the standard output stream.
 * \param data The string view containing the message to be logged.
 */
void stdout(StringView data);


/**
 * \brief Logs a message to the standard error stream.
 * \param data The string view containing the message to be logged.
 */
void stderr(StringView data);



/**
 * \brief Returns a Writer object that can be used to write log messages to the standard output stream. This function provides a convenient way to write log messages using the Writer interface, allowing for more flexible and efficient logging.
 * \return A Writer object for writing log messages to the standard output stream.
 */
Writer stdoutWriter();

/**
 * \brief Returns a Writer object that can be used to write log messages to the standard error stream. This function provides a convenient way to write log messages using the Writer interface, allowing for more flexible and efficient logging.
 * \return A Writer object for writing log messages to the standard error stream.
 */
Writer stderrWriter();


/**
 * \brief Logs a message at a specified log level. This function takes a log level and a variadic list of arguments, converts the arguments to their string representations, and writes the log message to the standard output stream with a colored prefix indicating the log level. The log message is only logged if the specified log level is greater than or equal to the configured log level threshold (NBL_LOG_LEVEL).
 * \tparam Args The types of the arguments to be logged.
 * \param level The log level at which to log the message.
 * \param args The variadic list of arguments to be logged, which will be converted to their string representations and included in the log message.
 */
template<typename... Args>
inline void log(nbl::LogLevel level, Args&&... args) {

    thread_local static u8 _logBuffer[4096];

    constexpr static const char* _prefixes[6] = {
        "\x1B[2m[TRACE] ",
        "\x1B[34m[DEBUG] ",
        "[INFO] ",
        "\x1B[33m[WARN] ",
        "\x1B[31m[ERROR] ",
        "\x1B[38;5;160m[FATAL] "
    };

    if(level < NBL_LOG_LEVEL || level > LogLevel::FATAL) {
        return;
    }

    BufferedWriter bufferedWriter(stdoutWriter(), _logBuffer, 4096);
    Writer out = bufferedWriter;

    out.writeString(_prefixes[static_cast<usize>(level)]);
    (void(write(std::forward<Args>(args), out)), ...);
    out.writeString("\n\x1B[0m");

    bufferedWriter.flush();
}

#define NBL_TRACE(...) nbl::log(nbl::LogLevel::TRACE, __VA_ARGS__);
#define NBL_DEBUG(...) nbl::log(nbl::LogLevel::DEBUG, __VA_ARGS__);
#define NBL_INFO(...) nbl::log(nbl::LogLevel::INFO, __VA_ARGS__);
#define NBL_WARN(...) nbl::log(nbl::LogLevel::WARN, __VA_ARGS__);
#define NBL_ERROR(...) nbl::log(nbl::LogLevel::ERROR, __VA_ARGS__);
#define NBL_FATAL(...) nbl::log(nbl::LogLevel::FATAL, __VA_ARGS__);


}


#pragma pop_macro("stderr")
#pragma pop_macro("stdout")
