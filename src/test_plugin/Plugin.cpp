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
 * \file Plugin.cpp
 * \author jwall64
 * \brief Test plugin
 */

#include <Plugin/PluginInitializer.hpp>
#include <Plugin/InstallationType.hpp>
#include <Plugin/AccountProvider.hpp>
#include <Plugin/MetadataProvider.hpp>
#include <Plugin/GameProvider.hpp>
#include <Plugin/PluginContext.hpp>
#include <Platform/Logging.hpp>
#include <Core/Array.hpp>
#include <Core/String.hpp>
#include <Data/Model.hpp>
#include <Plugin/Field.hpp>
#include "Data/OnlineAccountModel.hpp"


namespace {

enum _TestEnum : u32 {
    ONE,
    TWO,
    THREE,
    _END
};

u32 _count(nbl::PluginContext* pluginContext, void* userData) { 
    return _END;
}

nbl::StringView _name(nbl::PluginContext* pluginContext, u32 index, void* userData) {
    switch(index) {
        case ONE: return "one";
        case TWO: return "two";
        case THREE: return "three";
    }
    return "";
}

u32 _index(nbl::PluginContext* pluginContext, nbl::StringView name, void* userData) {
    if(name == "one") return ONE;
    if(name == "two") return TWO;
    if(name == "three") return THREE;
    return 0;
}

void _suggest(nbl::PluginContext* pluginContext, nbl::StringView current, nbl::FunctionRef<void(nbl::StringView)> suggestionConsumer, void* userData) {

    suggestionConsumer("Hello");
    suggestionConsumer("World");

}

nbl::Field _listTemplate = {
    .type = nbl::FieldType::STRING,
    .key = "",
    .name = "",
    .defaultValue = { .stringValue = { .resolved = "Test", .raw = "Test" } }
};

nbl::Array _structFields = {
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "string",
        .name = "String"
    },
    nbl::Field {
        .type = nbl::FieldType::INTEGER,
        .key = "int",
        .name = "Int",
        .integerData = {
            .min = -10,
            .max = 10
        }
    }
};

nbl::Array _fields = {
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "string",
        .name = "String",
        .stringData = {
            .pfnSuggest = _suggest,
            .userData = nullptr
        }
    },
    nbl::Field {
        .type = nbl::FieldType::INTEGER,
        .key = "integer",
        .name = "Integer",
        .integerData = {
            .min = 0,
            .max = 10
        }
    },
    nbl::Field {
        .type = nbl::FieldType::FLOAT,
        .key = "float",
        .name = "Float",
        .floatData = {
            .min = -100.0f,
            .max = 100.0f
        }
    },
    nbl::Field {
        .type = nbl::FieldType::BOOLEAN,
        .key = "boolean",
        .name = "Boolean",
        .defaultValue = {
            .booleanValue = true
        }
    },
    nbl::Field {
        .type = nbl::FieldType::ENUM,
        .key = "enum",
        .name = "Enum",
        .enumData = {
            .pfnIndex = _index,
            .pfnName = _name,
            .pfnCount = _count,
            .userData = nullptr
        },
        .defaultValue = {
            .enumValue = ONE
        }
    },
    nbl::Field {
        .type = nbl::FieldType::DATE,
        .key = "date",
        .name = "Date",
        .defaultValue = {
            .uintegerValue = 0
        }
    },
    nbl::Field {
        .type = nbl::FieldType::ACCOUNT,
        .key = "account",
        .name = "Account",
        .accountData = {
            .accountProvider = "test"
        },
        .defaultValue = {
            .accountValue = {
                .id = 0,
                .isDefault = true
            }
        },
    },
    nbl::Field {
        .type = nbl::FieldType::ACCOUNT,
        .key = "account_default",
        .name = "Account (Default)",
        .accountData = {
            .accountProvider = "test"
        },
        .defaultValue = {
            .accountValue = {
                .id = 0,
                .isDefault = true
            }
        },
        .flags = static_cast<nbl::FieldFlags>(nbl::FieldFlag::SHOW_DEFAULT_OPTION)
    },
    nbl::Field {
        .type = nbl::FieldType::LIST,
        .key = "list",
        .name = "List",
        .listData = {
            .templateField = &_listTemplate,
            .min = 0,
            .max = 3
        },
        .defaultValue = {
            .compositeValue = {}
        }
    },
    nbl::Field {
        .type = nbl::FieldType::STRUCT,
        .key = "struct",
        .name = "Struct",
        .structData = {
            .fields = _structFields
        },
        .defaultValue = {
            .compositeValue = {}
        }
    }
};

nbl::Array _accountFields {
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "username",
        .name = "Username",
        .flags = nbl::FieldFlag::EPHEMERAL | 0
    },
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "password",
        .name = "Password",
        .flags = nbl::FieldFlag::PASSWORD | nbl::FieldFlag::EPHEMERAL
    }
};

nbl::AccountProvider _provider {
    .id = "test",
    .name = "Test",
    .fields = _accountFields,
    .maxAccounts = 3,
    .pfnLogin = [](nbl::PluginContext* context, nbl::Span<nbl::FieldValue> values, nbl::LoginTracker* tracker, void* userData) -> nbl::Result {
        tracker->postStatus("3...");
        nbl::sleep(1000);
        tracker->postStatus("2...");
        nbl::sleep(1000);
        tracker->postStatus("1...");
        nbl::sleep(1000);

        tracker->complete(context, values[0].stringValue.raw, "", false, values);
        return nbl::Result::SUCCESS;
    },
    .pfnRefresh = [](nbl::PluginContext* context, const nbl::OnlineAccount& account, nbl::LoginTracker* tracker, void* userData) -> nbl::Result {
        return nbl::Result::NO_CHANGE;
    },
    .pfnLogout = [](nbl::PluginContext* context, i64 accountId, void* userData) -> nbl::Result {
        return nbl::Result::SUCCESS;
    }
};

nbl::Result _lookupMetadata(nbl::PluginContext* context, const nbl::MetadataLookupInfo& info, nbl::MetadataCallback callback, void* userData) { 
    callback(nbl::MetadataField::TITLE, { "Hello" });
    nbl::sleep(1000);
    callback(nbl::MetadataField::TITLE, { "Hello2" });
    callback(nbl::MetadataField::POSTER, { "qrc:/qt/qml/Nebula/images/UnknownPoster.png" });
    callback(nbl::MetadataField::POSTER, { "qrc:/qt/qml/Nebula/images/UnknownPoster.png" });
    callback(nbl::MetadataField::POSTER, { "qrc:/qt/qml/Nebula/images/UnknownPoster.png" });
    nbl::sleep(1000);
    callback(nbl::MetadataField::POSTER, { "qrc:/qt/qml/Nebula/images/UnknownPoster.png" });
    callback(nbl::MetadataField::POSTER, { "qrc:/qt/qml/Nebula/images/UnknownPoster.png" });
    callback(nbl::MetadataField::POSTER, { "qrc:/qt/qml/Nebula/images/UnknownPoster.png" });
    callback(nbl::MetadataField::POSTER, { "qrc:/qt/qml/Nebula/images/UnknownPoster.png" });
    callback(nbl::MetadataField::BANNER, { "qrc:/qt/qml/Nebula/images/UnknownIcon.png" });
    callback(nbl::MetadataField::BANNER, { "qrc:/qt/qml/Nebula/images/UnknownIcon.png" });
    callback(nbl::MetadataField::ICON, { "qrc:/qt/qml/Nebula/images/UnknownIcon.png" });
    callback(nbl::MetadataField::ICON, { "qrc:/qt/qml/Nebula/images/UnknownIcon.png" });
    callback(nbl::MetadataField::LOGO, { "qrc:/qt/qml/Nebula/images/UnknownIcon.png" });
    callback(nbl::MetadataField::LOGO, { "qrc:/qt/qml/Nebula/images/UnknownIcon.png" });
    callback(nbl::MetadataField::DESCRIPTION, { "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum." });
    callback(nbl::MetadataField::RELEASE_DATE, { .releaseDate = 1788405195318U });
    callback(nbl::MetadataField::RATING, { .rating = 99.0 });
    callback(nbl::MetadataField::TAG, { "hello" });
    callback(nbl::MetadataField::TAG, { "world" });
    callback(nbl::MetadataField::TAG, { "platformer" });
    nbl::sleep(1000);
    return nbl::Result::SUCCESS;
}

nbl::MetadataProvider _metaProvider = {
    .id = "test",
    .name = "Test",
    .pfnCanProvide = [](auto, auto, void*) { return true; },
    .pfnProvide = _lookupMetadata,
    .userData = nullptr
};

nbl::Result _lookupGames(nbl::PluginContext* context, const nbl::OnlineGameLookupInfo& info, nbl::OnlineGameCallback callback, void* userData) {
    static nbl::Array<nbl::StringView, 3> _tags = {
        "hello",
        "world",
        "platformer"
    };

    callback({
        .title = "Test",
        .description = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.",
        .posterUrl = "qrc:/qt/qml/Nebula/images/UnknownPoster.png",
        .iconUrl = "qrc:/qt/qml/Nebula/images/UnknownIcon.png",
        .bannerUrl = "qrc:/qt/qml/Nebula/images/UnknownIcon.png",
        .logoUrl = "qrc:/qt/qml/Nebula/images/UnknownIcon.png",
        .logoPosition = nbl::LogoPosition::CENTER,
        .releaseDate = 1788405195318U,
        .rating = 99.0,
        .tags = _tags
    });

    return nbl::Result::SUCCESS;
}

nbl::GameProvider _gameProvider = {
    .id = "test",
    .name = "Test",
    .pfnCanProvide = [](auto, auto, void*) { return true; },
    .pfnProvide = _lookupGames,
    .userData = nullptr
};

nbl::Result _launch(
        const nbl::InstallationType* type, 
        nbl::PluginContext* ctx, 
        nbl::Span<nbl::FieldValue> fields, 
        const nbl::Installation& meta, 
        nbl::ProcessTracker* handle, 
        void* userData) {

    nbl::log(nbl::LogLevel::INFO, "String: ", fields[0].stringValue.resolved);
    nbl::log(nbl::LogLevel::INFO, "Integer: ", fields[1].integerValue);
    nbl::log(nbl::LogLevel::INFO, "Float: ", fields[2].floatValue);
    nbl::log(nbl::LogLevel::INFO, "Boolean: ", fields[3].booleanValue);
    nbl::log(nbl::LogLevel::INFO, "Enum: ", _name(ctx, fields[4].enumValue, nullptr));
    nbl::log(nbl::LogLevel::INFO, "Date: ", fields[5].uintegerValue);

    nbl::log(nbl::LogLevel::INFO, "Account: ", fields[6].accountValue.id);
    nbl::log(nbl::LogLevel::INFO, "Account (Default): ", fields[7].accountValue.id);

    nbl::log(nbl::LogLevel::INFO, "List.length: ", fields[8].compositeValue.length());
    for(usize i = 0 ; i < fields[8].compositeValue.length() ; i++) {
        nbl::log(nbl::LogLevel::INFO, "List[", i, "]: ", fields[8].compositeValue[i].stringValue.resolved);
    }
    nbl::log(nbl::LogLevel::INFO, "Struct.String: ", fields[9].compositeValue[0].stringValue.resolved);
    nbl::log(nbl::LogLevel::INFO, "Struct.Int: ", fields[9].compositeValue[1].integerValue);

    ctx->pushNotification(nbl::NotificationType::INFO, "Hello", "Hello from test plugin");

    nbl::sleep(1000);
    handle->pushStatus("Status", 0.33);

    nbl::sleep(1000);
    handle->pushStatus("Status 2", 0.66);

    nbl::sleep(1000);
    handle->pushStatus("Status 3", 1.0);

    nbl::sleep(1000);
    handle->markComplete();

    return nbl::Result::SUCCESS;
}


nbl::InstallationType _type = {
    .id = "test",
    .name = "Test",
    .fields = _fields,
    .launchFunc = {
        .pfnLaunchInstallation = _launch,
        .userData = nullptr
    }
};


nbl::Array _pluginFields {
    nbl::Field {
        .type = nbl::FieldType::STRING,
        .key = "string",
        .name = "String",
        .stringData = {
            .pfnSuggest = _suggest,
            .userData = nullptr
        }
    }
};


}


extern "C" i32 initPlugin(nbl::PluginInitContext* context) {

    nbl::log(nbl::LogLevel::INFO, "Hello from plugin");

    context->setFields(_pluginFields, [](nbl::Span<nbl::Field> fields, nbl::Span<nbl::FieldValue> fieldValues, void* userData) {
        nbl::log(nbl::LogLevel::INFO, "Fields updated: string=", fieldValues[0].stringValue.resolved);
    }, nullptr);

    {
        nbl::AllocationFrame frame(nbl::Arena::threadLocal());
        nbl::Span<nbl::FieldValue> values{};
        NBL_CHECK(context->getStoredFieldValues(_pluginFields, &values, &frame));

        nbl::log(nbl::LogLevel::INFO, "string=", values[0].stringValue.resolved);
    }

    NBL_CHECK(context->registerObject(&_type));
    NBL_CHECK(context->registerObject(&_provider));
    NBL_CHECK(context->registerObject(&_metaProvider));
    NBL_CHECK(context->registerObject(&_gameProvider));

    return 0;
}
