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
 * \file Plugin/InstallationType.hpp
 * \author jwall64
 * \brief Plugin-registerable object that is in charge of launching installations that use it
 */

#pragma once

#include <Core/String.hpp>
#include <Core/Span.hpp>
#include <Codec/Json.hpp>
#include <Data/Model.hpp>
#include <Data/InstallationModel.hpp>
#include <Plugin/PluginContext.hpp>
#include <Plugin/ProcessTracker.hpp>
#include <Plugin/Field.hpp>
#include <Plugin/LogTransformer.hpp>

namespace nbl {

struct InstallationType;
struct LaunchInstallationFunction {
    Result (*pfnLaunchInstallation)(
            const InstallationType* type,
            PluginContext* ctx, 
            nbl::Span<FieldValue> fields, 
            const nbl::Installation& meta,
            ProcessTracker* tracker, 
            void* userData);
    void* userData;
};
struct GetFolderFunction {
    Result (*pfnGetFolder)(
            const InstallationType* type,
            PluginContext* ctx, 
            nbl::Span<FieldValue> fields, 
            const nbl::Installation& meta,
            StringView* outFolder,
            void* userData);
    void* userData;
};

struct InstallationType {
    StringView id;
    StringView name;
    Span<Field> fields;
    LaunchInstallationFunction launchFunc;
    GetFolderFunction getFolderFunc;
    LogTransformFunction logTransform;

    inline Result launch(PluginContext* ctx, nbl::Span<FieldValue> fields, const nbl::Installation& meta, ProcessTracker* handle) const {
        if(launchFunc.pfnLaunchInstallation == nullptr) return nbl::Result::PLUGIN_ERROR;
        return launchFunc.pfnLaunchInstallation(this, ctx, fields, meta, handle, launchFunc.userData);
    }

    inline Result getFolder(PluginContext* ctx, nbl::Span<FieldValue> fields, const nbl::Installation& meta, StringView* outFolder) const {
        if(getFolderFunc.pfnGetFolder == nullptr) return nbl::Result::PLUGIN_ERROR;
        return getFolderFunc.pfnGetFolder(this, ctx, fields, meta, outFolder, getFolderFunc.userData);
    }

    static InstallationType COMMAND;
};

}
