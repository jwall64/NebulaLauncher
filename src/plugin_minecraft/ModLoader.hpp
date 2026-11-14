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

#include <Core/String.hpp>
#include "DownloadManager.hpp"
#include "VersionManager.hpp"

namespace nbl::mc {


struct ModLoader {
    enum Value : u32 {
        VANILLA,
        FABRIC,
        QUILT,
        _END
    };

    ModLoader(Value v) : value(v) {}
    ModLoader(u32 v) : value(v) {}

    operator u32() { return value; }

    u32 value;

    static inline u32 index(nbl::PluginContext* ctx,StringView name, void* userData) {
        if(name == "fabric") return ModLoader::FABRIC;
        if(name == "quilt") return ModLoader::QUILT;
        return VANILLA;
    }
    static inline StringView name(nbl::PluginContext* ctx,u32 index, void* userData) {
        switch(index) {
            default:
            case VANILLA: return "vanilla";
            case FABRIC: return "fabric";
            case QUILT: return "quilt";
        }
    }
    static inline StringView displayName(nbl::PluginContext* ctx,u32 index, void* userData) {
        switch(index) {
            default:
            case VANILLA: return "None";
            case FABRIC: return "Fabric";
            case QUILT: return "Quilt";
        }
    }
    static inline u32 count(nbl::PluginContext* ctx,void* userData) {
        return _END;
    }
};


Result generateFabricVersionMeta(VersionRef* outVersion, StringView folder, DownloadManager* manager, LinearAllocator* allocator);
Result generateQuiltVersionMeta(VersionRef* outVersion, StringView folder, DownloadManager* manager, LinearAllocator* allocator);


}
