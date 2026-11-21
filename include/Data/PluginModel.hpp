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

#pragma once

#include <Data/Common.hpp>

namespace nbl {

struct PluginFields {
    StringView pluginId;
    StringView fieldsJson;
    StringView version;
};

typedef nbl::FunctionRef<Result(const PluginFields&)> PluginFieldsCallback;

Result getPluginFields(DatabaseConnection* connection, StringView pluginId, PluginFieldsCallback callback);

Result getPluginFieldsVersion(DatabaseConnection* connection, StringView pluginId, PluginFieldsCallback callback);

Result setPluginFields(DatabaseConnection* connection, StringView pluginid, StringView fieldsJson, StringView version);

Result prunePluginFields(DatabaseConnection* connection, JsonArray keepIds);

}
