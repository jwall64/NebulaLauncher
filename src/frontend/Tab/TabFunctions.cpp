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
 * \file TabFunctions.cpp
 * \author jwall64
 * \brief Partial implementation of ApplicationContext.hpp, containing functions related to managing tabs
 */

#include <QObject>

#include "ApplicationContext.hpp"

#include <Data/MediaManager.hpp>
#include <Data/Model.hpp>
#include <Data/FilterModel.hpp>

namespace {

nbl::Result _pushFilters(ApplicationContext* context, nbl::DatabaseConnection* connection, i64 tabId, const QVariantList& filters) {

    for(const QVariant& filterVar : filters) {

        nbl::AllocationFrame frame(nbl::Arena::threadLocal());

        QVariantMap filter = filterVar.toMap();
        i64 filterId = filter["id"].toLongLong();
        
        QByteArray name = filter["name"].toByteArray();
        QString icon = filter["icon"].toString();
        bool colorizeIcon = filter["colorizeIcon"].toBool();
        bool multiLine = filter["multiLine"].toBool();
        bool useIcons = filter["useIcons"].toBool();
        bool showTitles = filter["showTitles"].toBool();
        bool onlyInstalled = filter["onlyInstalled"].toBool();
        bool showInstallations = filter["showInstallations"].toBool();
        bool showFilterInfo = filter["showFilterInfo"].toBool();
        bool iconIsImage = filter["iconIsImage"].toBool();
        i64 priority = filter["priority"].toLongLong();


        nbl::InlineIcon iconId{};
        if(iconIsImage) {
            NBL_CHECK(context->pushImage(icon, nbl::ImageClass::INLINE_ICON, &iconId.imageId));
        } else {
            if(icon.length() > 0) {
                QByteArray utf = icon.toUtf8();
                qsizetype length = nbl::min(static_cast<qsizetype>(4), utf.length());
                memcpy(iconId.utf.data, utf.data(), length);
                iconId.utf.length = length;
            }
        }


        nbl::SortBy sortBy = static_cast<nbl::SortBy>(filter["sortBy"].toInt());
        nbl::SortOrder sortOrder = static_cast<nbl::SortOrder>(filter["sortOrder"].toInt());
        
        u64 minReleaseDate = filter["minReleaseDate"].toULongLong();
        u64 maxReleaseDate = filter["maxReleaseDate"].toULongLong();
        f64 minRating = filter["minRating"].toDouble();
        f64 maxRating = filter["maxRating"].toDouble();
        u64 minPlayTime = filter["minTime"].toULongLong();
        u64 maxPlayTime = filter["maxTime"].toULongLong();
        u64 minAchievements = filter["minAchievements"].toULongLong();
        u64 maxAchievements = filter["maxAchievements"].toULongLong();
        u64 minUnlockedAchievements = filter["minUnlockedAchievements"].toULongLong();
        u64 maxUnlockedAchievements = filter["maxUnlockedAchievements"].toULongLong();


        QVariantList setsList = filter["tags"].toList();
        nbl::Span<nbl::ConstSpan<i64>> tagSets(nullptr, setsList.length());
        if(setsList.length() > 0) {
            NBL_CHECK(frame.allocate(&tagSets));
            usize setCount = 0;
            for(QVariant var : setsList) {

                QVariantList list = var.toList();

                nbl::Span<i64> tags(nullptr, list.length());
                usize tagCount = 0;
                NBL_CHECK(frame.allocate(&tags));
                for(QVariant tId : list) {
                    tags[tagCount++] = tId.toLongLong();
                }
                tagSets[setCount++] = tags;
            }
        }


        QVariantList excludeList = filter["excludeTags"].toList();
        nbl::Span<i64> exclude(nullptr, excludeList.length());
        if(excludeList.length() > 0) {
            NBL_CHECK(frame.allocate(&exclude));
            usize i = 0;
            for(QVariant var : excludeList) {
                exclude[i++] = var.toLongLong();
            }
        }


        nbl::FilterFlags flags;
        if(colorizeIcon) flags |= nbl::FilterFlag::COLORIZE_ICON;
        if(multiLine) flags |= nbl::FilterFlag::MULTI_LINE;
        if(showTitles) flags |= nbl::FilterFlag::SHOW_TITLES;
        if(useIcons) flags |= nbl::FilterFlag::USE_ICONS;
        if(onlyInstalled) flags |= nbl::FilterFlag::ONLY_INSTALLED;
        if(showInstallations) flags |= nbl::FilterFlag::SHOW_INSTALLATIONS;
        if(showFilterInfo) flags |= nbl::FilterFlag::SHOW_FILTER_INFO;
        if(iconIsImage) flags |= nbl::FilterFlag::IMAGE_ICON;


        nbl::Filter meta = {
            .filterId = filterId,
            .tabId = tabId,
            .priority = priority,
            .name = nbl::StringView(name.data(), name.length()),
            .icon = iconId,
            .flags = flags,
            .criteria = {
                .tags = tagSets,
                .excludedTags = exclude,
                .minReleaseDate = minReleaseDate,
                .maxReleaseDate = maxReleaseDate,
                .minRating = minRating,
                .maxRating = maxRating,
                .minPlayTime = minPlayTime,
                .maxPlayTime = maxPlayTime,
                .sortBy = sortBy,
                .sortOrder = sortOrder,
                .minAchievements = minAchievements,
                .maxAchievements = maxAchievements,
                .minUnlockedAchievements = minUnlockedAchievements,
                .maxUnlockedAchievements = maxUnlockedAchievements
            }
        };

        if(meta.filterId == 0) {
            NBL_CHECK(nbl::Filter::add(connection, meta, &meta.filterId));
        } else {
            NBL_CHECK(nbl::Filter::update(connection, meta));
        }
    }

    return nbl::Result::SUCCESS;
}

}

int ApplicationContext::addTab(const QVariantMap& params) {

    QByteArray name = params["name"].toByteArray();
    QString icon = params["icon"].toString();
    bool colorizeIcon = params["colorizeIcon"].toBool();
    bool hidden = params["hidden"].toBool();
    bool iconIsImage = params["iconIsImage"].toBool();

    nbl::InlineIcon iconId{};
    if(iconIsImage) {
        NBL_CHECK(pushImage(icon, nbl::ImageClass::INLINE_ICON, &iconId.imageId));
    } else {
        if(icon.length() > 0) {
            QByteArray utf = icon.toUtf8();
            qsizetype length = nbl::min(static_cast<qsizetype>(4), utf.length());
            memcpy(iconId.utf.data, utf.data(), length);
            iconId.utf.length = length;
        }
    }

    QByteArray realIcon = icon.toUtf8();

    nbl::TabFlags flags;
    if(colorizeIcon) flags |= nbl::TabFlag::COLORIZE_ICON;
    if(hidden) flags |= nbl::TabFlag::HIDDEN;
    if(iconIsImage) flags |= nbl::TabFlag::IMAGE_ICON;

    nbl::Tab meta = {
        .name = nbl::StringView(name.data(), name.length()),
        .icon = iconId,
        .flags = flags
    };

    nbl::DatabaseConnection* c = databaseConnection();
    NBL_CHECK(c->beginTransaction());

    NBL_CHECK_OR_ROLLBACK(c, nbl::Tab::add(c, meta, &meta.tabId));
    NBL_CHECK_OR_ROLLBACK(c, _pushFilters(this, c, meta.tabId, params["filters"].toList()));

    NBL_CHECK(c->commitTransaction());

    return nbl::Result::SUCCESS;
}
int ApplicationContext::updateTab(qint64 id, const QVariantMap& params) {

    QByteArray name = params["name"].toByteArray();
    QString icon = params["icon"].toString();
    bool colorizeIcon = params["colorizeIcon"].toBool();
    bool hidden = params["hidden"].toBool();
    bool iconIsImage = params["iconIsImage"].toBool();

    nbl::InlineIcon iconId{};
    if(iconIsImage) {
        NBL_CHECK(pushImage(icon, nbl::ImageClass::INLINE_ICON, &iconId.imageId));
    } else {
        if(icon.length() > 0) {
            QByteArray utf = icon.toUtf8();
            qsizetype length = nbl::min(static_cast<qsizetype>(4), utf.length());
            memcpy(iconId.utf.data, utf.data(), length);
            iconId.utf.length = length;
        }
    }

    nbl::TabFlags flags;
    if(colorizeIcon) flags |= nbl::TabFlag::COLORIZE_ICON;
    if(hidden) flags |= nbl::TabFlag::HIDDEN;
    if(iconIsImage) flags |= nbl::TabFlag::IMAGE_ICON;

    QByteArray realIcon = icon.toUtf8();

    nbl::Tab meta = {
        .tabId = static_cast<i64>(id),
        .name = nbl::StringView(name.data(), name.length()),
        .icon = iconId,
        .flags = flags
    };

    nbl::DatabaseConnection* c = databaseConnection();
    NBL_CHECK(c->beginTransaction());
    NBL_CHECK_OR_ROLLBACK(c, nbl::Tab::update(c, meta));

    NBL_CHECK_OR_ROLLBACK(c, _pushFilters(this, c, meta.tabId, params["filters"].toList()));

    for(const QVariant& removed : params["removedFilters"].toList()) {
        NBL_CHECK_OR_ROLLBACK(c, nbl::Filter::remove(c, removed.toLongLong()));
    }

    NBL_CHECK(c->commitTransaction());
    return nbl::Result::SUCCESS;
}

int ApplicationContext::deleteTab(qint64 id) { 
    return nbl::Tab::remove(databaseConnection(), id);
}

nbl::frontend::TabRef* ApplicationContext::getTabMeta(qint64 id) {
    return nbl::frontend::TabRef::lookup(id);
}
