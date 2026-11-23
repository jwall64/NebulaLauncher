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

set(QT_QML_GENERATE_QMLLS_INI ON)

project(Nebula_App LANGUAGES CXX)

find_package(Qt6 6.11.1 COMPONENTS Quick ShaderTools Qml REQUIRED)

qt_standard_project_setup(REQUIRES 6.11.1)

file(GLOB_RECURSE NBL_APP_FONTS RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/fonts/*.ttf")
file(GLOB_RECURSE NBL_APP_IMAGES RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/images/*.png")

file(GLOB_RECURSE NBL_APP_QML_SCRIPTS RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/*.qml")
file(GLOB_RECURSE NBL_APP_SOURCES RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
list(REMOVE_ITEM NBL_APP_SOURCES "Main.cpp")

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(DialogManager.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(Nebula_App Main.cpp)
qt_add_qml_module(Nebula_App
    URI Nebula
    VERSION 1.0
    QML_FILES ${NBL_APP_QML_SCRIPTS}
    RESOURCES ${NBL_APP_FONTS} ${NBL_APP_IMAGES}
    SOURCES ${NBL_APP_SOURCES}
)

file(GLOB NBL_APP_SHADERS RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/shaders/*")

message(${NBL_APP_SHADERS})

qt_add_shaders(Nebula_App "/"
    FILES ${NBL_APP_SHADERS}
)

target_include_directories(Nebula_App PRIVATE 
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/Account" 
    "${CMAKE_CURRENT_LIST_DIR}/Achievement" 
    "${CMAKE_CURRENT_LIST_DIR}/Field" 
    "${CMAKE_CURRENT_LIST_DIR}/Filter" 
    "${CMAKE_CURRENT_LIST_DIR}/Game" 
    "${CMAKE_CURRENT_LIST_DIR}/Installation" 
    "${CMAKE_CURRENT_LIST_DIR}/Launch" 
    "${CMAKE_CURRENT_LIST_DIR}/Plugin"
    "${CMAKE_CURRENT_LIST_DIR}/Tab"
    "${CMAKE_CURRENT_LIST_DIR}/Tag"
)

target_link_libraries(Nebula_App PRIVATE Nebula_Api)
target_link_libraries(Nebula_App PRIVATE Qt6::Quick Qt6::ShaderTools)
target_link_libraries(Nebula_App PRIVATE CURL::libcurl)


install(TARGETS Nebula_App
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET Nebula_App
    OUTPUT_SCRIPT NBL_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${NBL_DEPLOY_SCRIPT})
