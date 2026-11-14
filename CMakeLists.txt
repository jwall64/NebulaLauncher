# Copyright 2026 jwall64
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 4.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_EXTENSIONS OFF)

project(Nebula LANGUAGES C CXX)

set(NBL_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

option(NBL_ENABLE_ASAN "Enable AddressSanitizer" OFF)

option(NBL_BUILD_FRONTEND "Build Frontend" ON)
option(NBL_BUILD_TESTS "Build Tests" ON)
option(NBL_BUILD_TEST_PLUGIN "Build Test Plugin" ON)
option(NBL_BUILD_SGDB_PLUGIN "Build SteamGridDB Plugin" ON)
option(NBL_BUILD_IGDB_PLUGIN "Build IGDB.com Plugin" ON)
option(NBL_BUILD_STEAM_PLUGIN "Build Steam Plugin" ON)
option(NBL_BUILD_RA_PLUGIN "Build RetroAchivements Plugin" ON)
option(NBL_BUILD_MINECRAFT_PLUGIN "Build Minecraft Plugin" ON)

#
# Dependencies
#
include(cmake/CPM.cmake)
CPMAddPackage(
    NAME googletest
    GIT_REPOSITORY "https://github.com/google/googletest"
    GIT_TAG "v1.17.0"
    OPTIONS
        "INSTALL_GTEST OFF"
        "BUILD_GMOCK OFF"
)

CPMAddPackage(
    NAME curl
    GIT_REPOSITORY "https://github.com/curl/curl"
    GIT_TAG "curl-8_20_0"
    OPTIONS
        "BUILD_CURL_EXE OFF"
        "BUILD_SHARED_LIBS OFF"
        "CMAKE_USE_OPENSSL ON"
        "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

find_package(OpenSSL REQUIRED)

include(cmake/SQLite3.cmake)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#
# Targets
#

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /Entry:WinMainCRTStartup")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /Subsystem:Windows")
endif()

set(NBL_SRC_DIR ${NBL_ROOT_DIR}/src)
set(NBL_INCLUDE_DIR ${NBL_ROOT_DIR}/include)

file(GLOB_RECURSE NBL_API_SOURCES ${NBL_SRC_DIR}/api/**.cpp)
file(GLOB_RECURSE NBL_API_HEADERS ${NBL_INCLUDE_DIR}/api/**.hpp)

add_library(Nebula_Api SHARED ${NBL_API_SOURCES})
target_include_directories(Nebula_Api PUBLIC ${NBL_INCLUDE_DIR})
target_include_directories(Nebula_Api PRIVATE ${NBL_SRC_DIR}/api)
target_precompile_headers(Nebula_Api PUBLIC ${NBL_API_HEADERS})
target_link_libraries(Nebula_Api PRIVATE CURL::libcurl)
target_link_libraries(Nebula_Api PRIVATE OpenSSL::Crypto)
target_link_libraries(Nebula_Api PRIVATE SQLite3::SQLite3)

if(NBL_ENABLE_ASAN)
    if(MSVC)
        target_compile_options(Nebula_Api PUBLIC /fsanitize=address)
    else()
        target_compile_options(Nebula_Api PUBLIC -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(Nebula_Api PUBLIC -fsanitize=address)
    endif()
endif()


file(GLOB_RECURSE NBL_WRAPPER_SOURCES ${NBL_SRC_DIR}/wrapper/**.cpp)

add_executable(Nebula_Wrapper ${NBL_WRAPPER_SOURCES})
target_include_directories(Nebula_Wrapper PRIVATE ${NBL_SRC_DIR}/wrapper)
target_link_libraries(Nebula_Wrapper PRIVATE Nebula_Api)

if(${NBL_BUILD_TEST_PLUGIN}) 

    file(GLOB_RECURSE NBL_TEST_PLUGIN_SOURCES ${NBL_SRC_DIR}/test_plugin/**.cpp)
    add_library(Nebula_TestPlugin SHARED ${NBL_TEST_PLUGIN_SOURCES})
    target_link_libraries(Nebula_TestPlugin PRIVATE Nebula_Api)
    set_target_properties(Nebula_TestPlugin PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/test_plugin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/test_plugin"
        PREFIX ""
        OUTPUT_NAME "plugin"
    )

    add_custom_command(
        TARGET Nebula_TestPlugin
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${NBL_SRC_DIR}/test_plugin/plugin.json" "${CMAKE_BINARY_DIR}/bin/plugins/test_plugin"
        VERBATIM
    )

endif()

if(${NBL_BUILD_SGDB_PLUGIN}) 

    file(GLOB_RECURSE NBL_SGDB_PLUGIN_SOURCES ${NBL_SRC_DIR}/plugin_sgdb/**.cpp)
    add_library(Nebula_SgdbPlugin SHARED ${NBL_SGDB_PLUGIN_SOURCES})
    target_link_libraries(Nebula_SgdbPlugin PRIVATE Nebula_Api)
    set_target_properties(Nebula_SgdbPlugin PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/sgdb"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/sgdb"
        PREFIX ""
        OUTPUT_NAME "plugin"
    )

    add_custom_command(
        TARGET Nebula_SgdbPlugin
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${NBL_SRC_DIR}/plugin_sgdb/plugin.json" "${CMAKE_BINARY_DIR}/bin/plugins/sgdb"
        VERBATIM
    )
endif()

if(${NBL_BUILD_IGDB_PLUGIN}) 

    file(GLOB_RECURSE NBL_IGDB_PLUGIN_SOURCES ${NBL_SRC_DIR}/plugin_igdb/**.cpp)
    add_library(Nebula_IgdbPlugin SHARED ${NBL_IGDB_PLUGIN_SOURCES})
    target_link_libraries(Nebula_IgdbPlugin PRIVATE Nebula_Api)
    set_target_properties(Nebula_IgdbPlugin PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/igdb"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/igdb"
        PREFIX ""
        OUTPUT_NAME "plugin"
    )

    add_custom_command(
        TARGET Nebula_IgdbPlugin
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${NBL_SRC_DIR}/plugin_igdb/plugin.json" "${CMAKE_BINARY_DIR}/bin/plugins/igdb"
        VERBATIM
    )

endif()


if(${NBL_BUILD_RA_PLUGIN}) 

    file(GLOB NBL_RA_PLUGIN_SOURCES ${NBL_SRC_DIR}/plugin_retroachievements/*.cpp)
    add_library(Nebula_RaPlugin SHARED ${NBL_RA_PLUGIN_SOURCES})
    target_link_libraries(Nebula_RaPlugin PRIVATE Nebula_Api)
    set_target_properties(Nebula_RaPlugin PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/retroachievements"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/retroachievements"
        PREFIX ""
        OUTPUT_NAME "plugin"
    )

    add_custom_command(
        TARGET Nebula_RaPlugin
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy 
            "${NBL_SRC_DIR}/plugin_retroachievements/plugin.json" 
            "${NBL_SRC_DIR}/plugin_retroachievements/ra.db" 
            "${CMAKE_BINARY_DIR}/bin/plugins/retroachievements"
        VERBATIM
    )
endif()


if(${NBL_BUILD_MINECRAFT_PLUGIN}) 

    file(GLOB_RECURSE NBL_TEST_PLUGIN_SOURCES ${NBL_SRC_DIR}/plugin_minecraft/**.cpp)
    add_library(Nebula_MinecraftPlugin SHARED ${NBL_TEST_PLUGIN_SOURCES})
    target_link_libraries(Nebula_MinecraftPlugin PRIVATE Nebula_Api)
    set_target_properties(Nebula_MinecraftPlugin PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/minecraft"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/minecraft"
        PREFIX ""
        OUTPUT_NAME "plugin"
    )

    add_custom_command(
        TARGET Nebula_MinecraftPlugin
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${NBL_SRC_DIR}/plugin_minecraft/plugin.json" "${CMAKE_BINARY_DIR}/bin/plugins/minecraft"
        VERBATIM
    )

endif()

if(${NBL_BUILD_STEAM_PLUGIN}) 

    CPMAddPackage(
        NAME Steamworks
        GIT_REPOSITORY "https://github.com/rlabrecque/SteamworksSDK"
        GIT_TAG df2baabf574a738ef1ea90a7e89339107fc0a279
        #URL "https://partner.steamgames.com/downloads/steamworks_sdk_165.zip"
        DOWNLOAD_ONLY TRUE
    )
    add_library(Steamworks INTERFACE)
    target_include_directories(Steamworks INTERFACE "${Steamworks_SOURCE_DIR}/public")

    file(GLOB NBL_STEAM_CLIENT_SOURCES ${NBL_SRC_DIR}/plugin_steam/client/*.cpp)
    add_executable(Nebula_SteamAchievementClient ${NBL_STEAM_CLIENT_SOURCES})
    target_link_libraries(Nebula_SteamAchievementClient PRIVATE Nebula_Api)
    target_link_libraries(Nebula_SteamAchievementClient PRIVATE Steamworks)
    target_link_libraries(Nebula_SteamAchievementClient PRIVATE OpenSSL::Crypto)
    set_target_properties(Nebula_SteamAchievementClient PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/steam"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/steam"
        PREFIX ""
        OUTPUT_NAME "AchievementClient"
    )


    file(GLOB NBL_STEAM_PLUGIN_SOURCES ${NBL_SRC_DIR}/plugin_steam/*.cpp)
    add_library(Nebula_SteamPlugin SHARED ${NBL_STEAM_PLUGIN_SOURCES})
    target_link_libraries(Nebula_SteamPlugin PRIVATE Nebula_Api)
    set_target_properties(Nebula_SteamPlugin PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/steam"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/steam"
        PREFIX ""
        OUTPUT_NAME "plugin"
    )

    add_custom_command(
        TARGET Nebula_SteamPlugin
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${NBL_SRC_DIR}/plugin_steam/plugin.json" "${CMAKE_BINARY_DIR}/bin/plugins/steam"
        VERBATIM
    )

endif()


if(${NBL_BUILD_FRONTEND}) 
    add_subdirectory(src/frontend)
endif()

if(${NBL_BUILD_TESTS}) 
    file(GLOB NBL_TEST_SOURCES ${NBL_SRC_DIR}/test/*.cpp)
    if(${NBL_BUILD_RA_PLUGIN}) 
        file(GLOB NBL_RA_TEST_SOURCES ${NBL_SRC_DIR}/test/retroachievements/*.cpp)
        list(APPEND NBL_TEST_SOURCES ${NBL_RA_TEST_SOURCES} ${NBL_RA_PLUGIN_SOURCES})
    endif()

    list(FILTER NBL_TEST_SOURCES EXCLUDE REGEX ".*/Plugin.cpp$")

    if(NOT ${NBL_BUILD_MINECRAFT_PLUGIN})
        list(FILTER NBL_TEST_SOURCES EXCLUDE REGEX ".*/MinecraftLog.cpp$")
        list(FILTER NBL_TEST_SOURCES EXCLUDE REGEX ".*/Quilt.cpp$")
    endif()

    add_executable(Nebula_Test ${NBL_TEST_SOURCES})
    target_link_libraries(Nebula_Test PRIVATE Nebula_Api)
    target_link_libraries(Nebula_Test PRIVATE GTest::gtest_main)

    if(${NBL_BUILD_MINECRAFT_PLUGIN})
        target_link_libraries(Nebula_Test PRIVATE Nebula_MinecraftPlugin)
        target_include_directories(Nebula_Test PRIVATE ${NBL_SRC_DIR}/plugin_minecraft)
    endif()
endif()

