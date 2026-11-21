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

#include <Data/Database.hpp>

namespace nbl {


struct HostDevice;
typedef nbl::FunctionRef<Result(const HostDevice&)> DeviceCallback;


struct HostDevice {
    i64 id; 
    StringView name;

    static Result add(DatabaseConnection* connection, HostDevice meta, i64* outId);
    static Result get(DatabaseConnection* connection, i64 id, DeviceCallback callback);
    static Result update(DatabaseConnection* connection, HostDevice meta); 
    static Result upsert(DatabaseConnection* connection, StringView hostname, i64* outId);
    static Result remove(DatabaseConnection* connection, i64 id);
};



}
