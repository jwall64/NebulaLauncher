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
#include <QColorSpace>
#include <QHash>
#include <QImage>
#include <QImageReader>
#include <QJSEngine>
#include <QUrl>

#include <vector>


namespace {

// Decode a local image now so it survives without the original file. Images
// that are only referenced by URL are still fetched lazily.
nbl::Result _prepareLocalImage(const QString& url, nbl::ImageClass imageClass, QImage* outImage) {

    QString path = QUrl(url).toLocalFile();
    if(path.isEmpty()) return nbl::Result::INVALID_ARGUMENT;

    QImageReader reader(path);
    reader.setAutoTransform(true);
    if(!reader.canRead()) return nbl::Result::IO_ERROR;

    QImage image = reader.read();
    if(image.isNull()) return nbl::Result::IO_ERROR;

    nbl::ImageClass::Dimensions dim = imageClass.dimensions();
    if(dim.width > 0 && dim.height > 0 && (image.width() > dim.width || image.height() > dim.height)) {
        image = image.scaled(
            dim.width,
            dim.height,
            imageClass.shouldCrop() ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
    }

    QColorSpace colorSpace = image.colorSpace();
    if(colorSpace.isValid() && colorSpace != QColorSpace::SRgb) {
        image.convertToColorSpace(QColorSpace::SRgb);
    }

    if(image.format() != QImage::Format_RGBA8888) {
        image = image.convertToFormat(QImage::Format_RGBA8888);
    }

    *outImage = image;
    return nbl::Result::SUCCESS;
}

}


nbl::Result ApplicationContext::pushImage(const QString& url, nbl::ImageClass ic, i64* outId) {

    if(url.startsWith("image://nblmedia/")) {
        bool ok;
        qint64 existingId = url.sliced(17).toInt(&ok);
        if(!ok) {
            return nbl::Result::INVALID_ARGUMENT;
        }
        *outId = existingId;
        return nbl::Result::SUCCESS;
    }

    QByteArray utf8 = url.toUtf8();
    nbl::StringView urlView(utf8.constData(), static_cast<usize>(utf8.length()));

    if(!url.startsWith("http://") && !url.startsWith("https://")) {
        QImage image;
        if(_prepareLocalImage(url, ic, &image).success()) {
            return mediaManager()->reserveAndSave(
                urlView,
                ic,
                static_cast<u32>(image.width()),
                static_cast<u32>(image.height()),
                nbl::ConstSpan<u8>(image.constBits(), static_cast<usize>(image.sizeInBytes())),
                outId
            );
        }
        // If the file cannot be decoded, keep the reference and let the normal
        // lazy path deal with it (or fail there).
    }

    return mediaManager()->reserveReference(urlView, ic, outId);
}


bool ApplicationContext::cleanupUnusedImages() {

    nbl::DatabaseConnection* connection = databaseConnection();
    nbl::MediaManager* media = mediaManager();
    if(!connection || !media) return false;

    // The media database doesn't know which rows in the main database still
    // point at an image, so collect every live reference first.
    static nbl::StringView query = R"(
SELECT poster FROM game WHERE poster NOT NULL AND poster != 0
UNION ALL SELECT banner FROM game WHERE banner NOT NULL AND banner != 0
UNION ALL SELECT icon FROM game WHERE icon NOT NULL AND icon != 0
UNION ALL SELECT logo FROM game WHERE logo NOT NULL AND logo != 0
UNION ALL SELECT poster FROM installation WHERE poster NOT NULL AND poster != 0
UNION ALL SELECT banner FROM installation WHERE banner NOT NULL AND banner != 0
UNION ALL SELECT icon FROM installation WHERE icon NOT NULL AND icon != 0
UNION ALL SELECT logo FROM installation WHERE logo NOT NULL AND logo != 0
UNION ALL SELECT icon FROM achievement WHERE icon NOT NULL AND icon != 0
UNION ALL SELECT lockedIcon FROM achievement WHERE lockedIcon NOT NULL AND lockedIcon != 0
-- flags & 1 is OnlineAccountFlag::IMAGE_ICON
UNION ALL SELECT icon FROM online_account WHERE (flags & 1) != 0 AND icon NOT NULL AND icon != 0
-- flags & 4 is TabFlag::IMAGE_ICON
UNION ALL SELECT icon FROM tab WHERE (flags & 4) != 0 AND icon != 0
-- flags & 128 is FilterFlag::IMAGE_ICON
UNION ALL SELECT icon FROM filter WHERE (flags & 128) != 0 AND icon != 0
)";

    std::vector<i64> liveRefs;
    nbl::PreparedStatement stmt{};
    if(connection->prepare(query, &stmt).error()) return false;

    if(connection->execute(&stmt, [&](i32 columns, nbl::DataValue* values, const char**, i64) -> nbl::Result {
        if(columns > 0 && values[0].type == nbl::DataType::INTEGER) {
            liveRefs.push_back(values[0].intValue);
        }
        return nbl::Result::SUCCESS;
    }).error()) {
        return false;
    }

    return media->pruneUnused(nbl::ConstSpan<i64>(liveRefs.data(), liveRefs.size())).success();
}




void ApplicationContext::_initialize() {

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    QByteArray qDataDir = dataFolder().toUtf8();
    nbl::StringView nblDataDir = nbl::StringView(qDataDir.data(), qDataDir.length());

    NBL_ASSERT(nbl::makeDirectories(nblDataDir).success());

    nbl::StringView path;
    NBL_ASSERT(nbl::StringBuilder(&frame).append(nblDataDir).append("/nebula.db").build(&path).success());


    nbl::StringView mediaPath;
    NBL_ASSERT(nbl::StringBuilder(&frame).append(nblDataDir).append("/media.db").build(&mediaPath).success());


    nbl::StringView applicationPath = nbl::getBinaryPath();
    usize lastSlash = 0;
    NBL_ASSERT(applicationPath.findLast('/', &lastSlash).success());

    nbl::StringView pluginFolder;
    NBL_ASSERT(nbl::StringBuilder(&frame).append(applicationPath.slice(0, lastSlash)).append("/plugins").buildNullTerminated(&pluginFolder).success());
    NBL_ASSERT(nbl::makeDirectories(pluginFolder).success());

    nbl::StringView pluginDataPath;
    NBL_ASSERT(nbl::StringBuilder(&frame).append(nblDataDir).append("/plugindata").build(&pluginDataPath).success());


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

    // Drop image references that no game, installation, tab, achievement, or
    // account points at anymore, then reclaim the image data they owned.
    if(!cleanupUnusedImages()) {
        NBL_WARN("Failed to clean up unused images.");
    }
}


QString ApplicationContext::dataFolder() const {
    static QString _dataFolder = {};
    if(_dataFolder.isEmpty()) {
        if(qEnvironmentVariableIsSet("NBL_DATA_FOLDER")) {
            _dataFolder = qEnvironmentVariable("NBL_DATA_FOLDER");
        } else {
            nbl::StringView dataDir = nbl::getDataDirectory();
            _dataFolder = QString::fromUtf8(dataDir.data(), dataDir.length()).append("/NebulaLauncher");
        }
    }
    return _dataFolder;
}

QString ApplicationContext::pluginFolder() const {
    static QString _pluginFolder = {};
    if(_pluginFolder.isEmpty()) {

        nbl::StringView applicationPath = nbl::getBinaryPath();
        usize lastSlash = 0;
        NBL_ASSERT(applicationPath.findLast('/', &lastSlash).success());
        _pluginFolder = QString::fromUtf8(applicationPath.data(), lastSlash) + "/plugins";
    }
    return _pluginFolder;
}


void ApplicationContext::saveSettings(const QVariantMap& settings) {

    QJsonObject obj = QJsonObject::fromVariantMap(settings);
    QJsonDocument doc(obj);

    QString filePath = dataFolder() + "/settings.json";
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

    QFile file(dataFolder() + "/settings.json");
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
