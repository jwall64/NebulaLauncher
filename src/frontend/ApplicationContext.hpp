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

#include <QObject>
#include <QColor>
#include <QtQml>
#include <QProcess>

#include <Data/Database.hpp>
#include <Platform/File.hpp>
#include <Platform/Pool.hpp>
#include <Plugin/PluginContext.hpp>
#include <Plugin/PluginLoader.hpp>

#include "Data/MediaManager.hpp"
#include "Installation/InstallationDetails.hpp"
#include "Game/GameDetails.hpp"
#include "Tab/TabRef.hpp"
#include "Launch/ProcessHandle.hpp"

namespace nbl::frontend {


inline QByteArray fromNbl(nbl::StringView sv) {
    return QByteArray(sv.data(), sv.length());
}

inline QUrl mediaUrl(i64 refId) {
    if(refId == 0) return {};
    return QUrl::fromEncoded(QString("image://nblmedia/%1").arg(refId).toUtf8());
}

inline QByteArray inlineIcon(InlineIcon icon, bool isImage) {
    if(isImage) {
        return mediaUrl(icon.imageId).toEncoded();
    } else {
        return QByteArray(icon.utf.data, icon.utf.length);
    }
}


}

struct InstallationAddResult {
    Q_GADGET
    Q_PROPERTY(bool success MEMBER success)
    Q_PROPERTY(qint64 gameId MEMBER gameId)
    Q_PROPERTY(qint64 installationId MEMBER installationId)

public:

    InstallationAddResult() = default;

    InstallationAddResult(nbl::Result errorCode) :
        success(false),
        gameId(0),
        installationId(0)
    { }

    InstallationAddResult(bool success, qint64 gameId, qint64 installationId) :
        success(success),
        gameId(gameId),
        installationId(installationId)
    { }

    bool success;
    qint64 gameId;
    qint64 installationId;
};


class ApplicationContext : public QObject {

    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString dataFolder READ dataFolder CONSTANT)
    Q_PROPERTY(QString pluginFolder READ pluginFolder CONSTANT)
    Q_PROPERTY(qint64 deviceId READ deviceId CONSTANT)

public:

    static ApplicationContext* instance() {
        static ApplicationContext _instance {};
        return &_instance;
    }

    static ApplicationContext* create(QQmlEngine* engine, QJSEngine* scriptEngine) {
        Q_UNUSED(engine);
        Q_UNUSED(scriptEngine);

        return instance();
    }

    // Properties
    QString dataFolder() const;
    QString pluginFolder() const;

    qint64 deviceId() const { 
        return _mContext.mediaManager()->deviceId(); 
    }


    // General
    Q_INVOKABLE void saveSettings(const QVariantMap& settings);
    Q_INVOKABLE QVariantMap loadSettings();

    Q_INVOKABLE void pushNotification(QVariantMap details) {
        emit notification(details);
    }
    Q_INVOKABLE inline QUrl mediaUrl(qint64 refId) {
        return nbl::frontend::mediaUrl(refId);
    }


    // InstallationType
    Q_INVOKABLE nbl::frontend::ProcessHandle* launchInstallation(qint64 installationId);
    Q_INVOKABLE nbl::frontend::ProcessHandle* getRunningInstallation(qint64 installationId);
    Q_INVOKABLE QString getInstallationLogs(qint64 installationId);
    Q_INVOKABLE void clearInstallationLogs(qint64 installationId);
    Q_INVOKABLE int openInstallationFolder(qint64 installationId);

    // Game
    Q_INVOKABLE qint64 addGame(QVariantMap details);
    Q_INVOKABLE int updateGame(QVariantMap details);
    Q_INVOKABLE int deleteGame(qint64 gameId);
    Q_INVOKABLE qint64 getRandomGameId();
    Q_INVOKABLE nbl::frontend::GameDetails* getGameDetails(qint64 gameId);

    // Installation
    Q_INVOKABLE InstallationAddResult addInstallation(const QVariantMap& params);
    Q_INVOKABLE int updateInstallation(qint64 id, const QVariantMap& params);
    Q_INVOKABLE int deleteInstallation(qint64 id);
    Q_INVOKABLE nbl::frontend::InstallationDetails* getInstallationDetails(qint64 installationId);

    // Tab
    Q_INVOKABLE int addTab(const QVariantMap& params);
    Q_INVOKABLE int updateTab(qint64 id, const QVariantMap& params);
    Q_INVOKABLE int deleteTab(qint64 id);
    Q_INVOKABLE nbl::frontend::TabRef* getTabMeta(qint64 id);

    // Tag
    Q_INVOKABLE qint64 addTag(QString name, QColor color);
    Q_INVOKABLE qint64 findTagId(QString name);
    Q_INVOKABLE int removeTag(qint64 tag);
    Q_INVOKABLE int renameTag(qint64 tag, QStringView newName);
    Q_INVOKABLE int recolorTag(qint64 tag, QColor color);
    Q_INVOKABLE int addTagParent(qint64 tag, qint64 parent);
    Q_INVOKABLE int tagGame(qint64 gameId, qint64 tag);
    Q_INVOKABLE int untagGame(qint64 gameId, qint64 tag);
    Q_INVOKABLE int tagInstallation(qint64 installationId, qint64 tag);
    Q_INVOKABLE int untagInstallation(qint64 installationId, qint64 tag);
    Q_INVOKABLE int updateTags(QVariantList modifiedTags);
    Q_INVOKABLE QVariantList getTagsFromIds(QVariantList ids);
    Q_INVOKABLE QList<QVariant> getGameTags(qint64 gId);
    Q_INVOKABLE QList<QVariant> getInstallationTags(qint64 iId);
    Q_INVOKABLE QList<QVariant> getTagParents(qint64 tag);
    Q_INVOKABLE QVariantMap gameExternalIds(qint64 gameIds);


    nbl::DatabaseConnection* databaseConnection() {
        return _mContext.databaseConnection();
    }

    nbl::PluginContext* pluginContext() {
        return &_mContext;
    }

    nbl::MediaManager* mediaManager() {
        return _mContext.mediaManager();
    }

    nbl::Result pushImage(const QString& url, nbl::ImageClass ic, i64* outId);

    void init() {
        if(!_bInitialized) {
            _initialize();
            _bInitialized = true;
        }
    }

signals:

    void notification(QVariantMap details);

private:


    void _initialize();
    ApplicationContext() = default;

    bool _bInitialized = false;
    nbl::PluginContext _mContext;
    QHash<i64, nbl::frontend::ProcessHandle*> _mRunningProcesses;
    QHash<i64, QString> _mInstallationLogs;

};
