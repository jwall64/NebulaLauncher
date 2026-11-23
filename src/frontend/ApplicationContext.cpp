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


#include "ApplicationContext.hpp"

#include <Core/Result.hpp>
#include <Core/String.hpp>
#include <Core/Span.hpp>
#include <Core/Math.hpp>
#include <Platform/Logging.hpp>
#include <Platform/Time.hpp>
#include <Platform/Arena.hpp>
#include <Core/Assert.hpp>
#include <Platform/File.hpp>
#include <Platform/Process.hpp>
#include <Utility/StringBuilder.hpp>
#include <Plugin/InstallationType.hpp>
#include <Plugin/PluginContext.hpp>
#include <Plugin/AchievementProvider.hpp>
#include <Codec/Json.hpp>
#include <Data/Model.hpp>
#include <Data/Database.hpp>
#include <Data/MediaManager.hpp>
#include <Plugin/AccountProvider.hpp>
#include <Plugin/Field.hpp>
#include <Utility/Placeholder.hpp>

#include <QString>
#include <QDesktopServices>
#include <QColor>
#include <QHash>
#include <QJSEngine>


nbl::Result ApplicationContext::pushImage(const QString& url, nbl::ImageClass ic, i64* outId) {

    if(url.startsWith("image://nblmedia/")) {
        bool ok;
        qint64 existingId = url.sliced(17).toInt(&ok);
        if(!ok) {
            return nbl::Result::INVALID_ARGUMENT;
        }
        *outId = existingId;
        return nbl::Result::SUCCESS;

    } else {

        QByteArray utf8 = url.toUtf8();
        return mediaManager()->reserveReference(nbl::StringView(utf8.data(), utf8.length()), ic, outId);
    }
}




void ApplicationContext::_initialize() {

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    QByteArray qDataDir = dataFolder().toUtf8();
    nbl::StringView nblDataDir = nbl::StringView(qDataDir.data(), qDataDir.length());

    NBL_ASSERT(nbl::makeDirectories(nblDataDir).success());

    nbl::StringView path;
    NBL_ASSERT(nbl::StringBuilder(&frame).append(nblDataDir).append(NBL_PATH_SEPARATOR).append("nebula.db").buildNullTerminated(&path).success());


    nbl::StringView mediaPath;
    NBL_ASSERT(nbl::StringBuilder(&frame).append(nblDataDir).append(NBL_PATH_SEPARATOR).append("media.db").buildNullTerminated(&mediaPath).success());


    nbl::StringView applicationPath = nbl::getBinaryPath();
    usize lastSlash = 0;
    NBL_ASSERT(applicationPath.findLast(NBL_PATH_SEPARATOR, &lastSlash).success());

    nbl::StringView pluginFolder;
    NBL_ASSERT(nbl::StringBuilder(&frame).append(applicationPath.slice(0, lastSlash)).append(NBL_PATH_SEPARATOR).append("plugins").buildNullTerminated(&pluginFolder).success());
    NBL_ASSERT(nbl::makeDirectories(pluginFolder).success());

    nbl::StringView pluginDataPath;
    NBL_ASSERT(nbl::StringBuilder(&frame).append(nblDataDir).append(NBL_PATH_SEPARATOR).append("plugindata").buildNullTerminated(&pluginDataPath).success());


    nbl::StringView hostname = nbl::getHostName();

    NBL_ASSERT(nbl::PluginContext::create(hostname, path, mediaPath, pluginFolder, pluginDataPath, {
        .pfnNotify = [](void* userData, nbl::NotificationType type, nbl::StringView title, nbl::StringView message, nbl::Optional<nbl::StringView> icon, nbl::Optional<u32> duration) {
        
            ApplicationContext* ctx = static_cast<ApplicationContext*>(userData);

            QVariantMap map = {};
            switch(type) {
                case nbl::NotificationType::ERROR:
                    map["type"] = "error";    
                    break;
                case nbl::NotificationType::WARNING:
                    map["type"] = "warning";    
                    break;
                default:
                    map["type"] = "info";
            }

            map["title"] = QString::fromUtf8(title.data(), title.length());
            map["message"] = QString::fromUtf8(message.data(), message.length());
            if(icon.hasValue()) map["icon"] = nbl::frontend::fromNbl(*icon);
            if(duration.hasValue()) map["duration"] = *duration;
            
            ctx->pushNotification(map);

        },
        .userData = this
    }, &_mContext).success());

    NBL_ASSERT(_mContext.beginLoad().success());
}


QString ApplicationContext::dataFolder() const {
    static QString _dataFolder = {};
    if(_dataFolder.isEmpty()) {
        if(qEnvironmentVariableIsSet("NBL_DATA_FOLDER")) {
            _dataFolder = qEnvironmentVariable("NBL_DATA_FOLDER");
        } else {
            nbl::StringView dataDir = nbl::getDataDirectory();
            _dataFolder = QString::fromUtf8(dataDir.data(), dataDir.length()) + NBL_PATH_SEPARATOR + "NebulaLauncher";
        }
    }
    return _dataFolder;
}

QString ApplicationContext::pluginFolder() const {
    static QString _pluginFolder = {};
    if(_pluginFolder.isEmpty()) {

        nbl::StringView applicationPath = nbl::getBinaryPath();
        usize lastSlash = 0;
        NBL_ASSERT(applicationPath.findLast(NBL_PATH_SEPARATOR, &lastSlash).success());
        _pluginFolder = QString::fromUtf8(applicationPath.data(), lastSlash) + NBL_PATH_SEPARATOR + "plugins";
    }
    return _pluginFolder;
}


void ApplicationContext::saveSettings(const QVariantMap& settings) {

    QJsonObject obj = QJsonObject::fromVariantMap(settings);
    QJsonDocument doc(obj);

    QString filePath = dataFolder() + NBL_PATH_SEPARATOR + "settings.json";
    QFile file(filePath);

    if(file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    } else {
        QByteArray arr = filePath.toUtf8();
        nbl::log(nbl::LogLevel::ERROR, "Unable to save settings to ", nbl::StringView(arr.data()), arr.length());
    }
}

QVariantMap ApplicationContext::loadSettings() {

    QFile file(dataFolder() + NBL_PATH_SEPARATOR + "settings.json");
    if(!file.exists()) {
        return QVariantMap();
    }

    if(!file.open(QIODevice::ReadOnly)) {
        nbl::log(nbl::LogLevel::ERROR, "Unable to load settings file!");
        return QVariantMap();
    }
    
    QJsonParseError err = {};
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    
    if(err.error != QJsonParseError::NoError || !doc.isObject()) {
        nbl::log(nbl::LogLevel::ERROR, "An error occurred while loading the settings file!");
        return QVariantMap();
    }

    return doc.object().toVariantMap();
}
