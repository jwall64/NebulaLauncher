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
 * \file Plugin/LogTransformer.hpp
 * \author jwall64
 * \brief Optional hook for turning a process's raw output into displayable log lines
 */

#pragma once

#include <Core/Result.hpp>
#include <Core/String.hpp>

namespace nbl {

class PluginContext;
class StringBuilder;

/**
 * \brief A plugin-provided log transformer.
 *
 * A transformer is created once per launched installation, so it can keep any parser state
 * it needs without sharing it between concurrent installations. It receives complete lines
 * (without the trailing newline) and appends the text that should be shown to the user.
 *
 * Returning success with no output drops the line. Return an error to let the frontend fall
 * back to the raw line.
 */
struct LogTransformFunction {
    Result (*pfnCreate)(PluginContext* context, void** outState, void* userData) = nullptr;
    Result (*pfnTransform)(PluginContext* context, void* state, StringView line, StringBuilder* out, void* userData) = nullptr;
    void (*pfnDestroy)(PluginContext* context, void* state, void* userData) = nullptr;
    void* userData = nullptr;

    inline bool valid() const { return pfnTransform != nullptr; }
};

}
