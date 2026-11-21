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
#include "Game/GameSearchModel.hpp"
#include "ApplicationContext.hpp"

#include <Core/Result.hpp>
#include <Platform/Logging.hpp>
#include <Data/Database.hpp>
#include <Data/Model.hpp>
#include <Data/FilterModel.hpp>

QVariant nbl::frontend::GameSearchModel::data(const QModelIndex& index, int role) const {

    if(!index.isValid() || index.row() >= _mData.size()) return QVariant();

    _Row row = _mData.at(index.row());

    switch(role) {
        case IdRole:
            return row.gId;
        case TitleRole:
            return row.title;
        case PosterRole:
            return nbl::frontend::mediaUrl(row.poster);
        case IconRole:
            return nbl::frontend::mediaUrl(row.icon);
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> nbl::frontend::GameSearchModel::roleNames() const {
    QHash<int, QByteArray> roles = {};
    roles[IdRole] = "id";
    roles[TitleRole] = "title";
    roles[PosterRole] = "poster";
    roles[IconRole] = "icon";
    return roles;
}


nbl::Result nbl::frontend::GameSearchModel::_fetchPageData(qsizetype limit, qsizetype offset) {

    nbl::DatabaseConnection* db = ApplicationContext::instance()->databaseConnection();
    
    nbl::StringView query = nbl::StringView(_mSearchQuery.data(), _mSearchQuery.length());

    QList<_Row> found{};
    bool exactMatch = false;
    found.reserve(limit);

    nbl::SearchCriteria criteria = {
        .sortBy = nbl::SortBy::TITLE,
        .offset = static_cast<u64>(offset),
        .limit = static_cast<u64>(limit),
        .titleFilter = query
    };

    NBL_CHECK(nbl::Game::lookup(db, { .criteria = criteria }, [&](const nbl::GameLookupResult& meta) -> nbl::Result {

        _Row ref(
                meta.id,
                QByteArray(meta.title.data(), meta.title.length()), 
                meta.poster,
                meta.icon
        );

        found.append(ref);
        return nbl::Result::SUCCESS;
    }));


    if(found.size() > 0) {
        beginInsertRows(QModelIndex(), _mData.size(), _mData.size() + found.size() - 1);
        _mData.reserve(found.size());
        for(auto& ref : found) {
            _mData.append(ref);
        }
        endInsertRows();
    }

    if(exactMatch && !_bExactMatch) {
        _bExactMatch = true;
        exactMatchChanged();
    }

    return nbl::Result::SUCCESS;
}
