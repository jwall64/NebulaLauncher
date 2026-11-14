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
 * \file LogTransformer.hpp
 * \author jwall64
 * \brief Converts Minecraft's LegacyXMLLayout console output into readable log lines
 */

#pragma once

#include <Plugin/LogTransformer.hpp>

namespace nbl::mc {

Result createMinecraftLogTransformer(PluginContext* context, void** outState, void* userData);
Result transformMinecraftLogLine(PluginContext* context, void* state, StringView line, StringBuilder* out, void* userData);
void destroyMinecraftLogTransformer(PluginContext* context, void* state, void* userData);

}
