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
 * \file Plugin/PluginInitializer.hpp
 * \author jwall64
 * \brief Contains a prototype for a function with C linkage for initializing plugins. This file should only be included by plugins.
 */

#pragma once

#include <Plugin/PluginInitContext.hpp>
#include <Platform/PlatformDefines.hpp>

#ifdef NBL_PLATFORM_WINDOWS
#define NBL_EXPORT __declspec(dllexport)
#else
#define NBL_EXPORT __attribute__((visibility("default")))
#endif

extern "C" i32 NBL_EXPORT initPlugin(nbl::PluginInitContext* context);

