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
#include "Tag/TagSearchModel.hpp"
#include "ApplicationContext.hpp"

#include <Core/Result.hpp>
#include <Platform/Logging.hpp>
#include <Data/Database.hpp>
#include <Data/Model.hpp>
#include <Data/TagModel.hpp>

QVariant nbl::frontend::TagSearchModel::data(const QModelIndex& index, int role) const {

    if(!index.isValid() || index.row() >= _mData.size()) return QVariant();

    TagMeta ref = _mData.at(index.row());

    switch(role) {
        case IdRole:
            return ref.id;
        case NameRole:
            return ref.name;
        case ColorRole:
            return ref.color;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> nbl::frontend::TagSearchModel::roleNames() const {
    QHash<int, QByteArray> roles = {};
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[ColorRole] = "color";
    return roles;
}


nbl::Result nbl::frontend::TagSearchModel::_fetchPageData(qsizetype limit, qsizetype offset) {

    nbl::DatabaseConnection* db = ApplicationContext::instance()->databaseConnection();
    
    nbl::StringView query = nbl::StringView(_mSearchQuery.data(), _mSearchQuery.length());

    QList<TagMeta> found{};
    bool exactMatch = false;
    found.reserve(limit);

    if(query.empty()) {
        NBL_CHECK(nbl::Tag::enumerate(db, limit, offset, [&](const nbl::Tag& tag) -> nbl::Result {

            TagMeta ref(tag.id, QString::fromUtf8(tag.name.data(), tag.name.length()), QColor::fromRgb(tag.color)); 
            found.append(ref);
            return nbl::Result::SUCCESS;

        }));
    } else {
        NBL_CHECK(nbl::Tag::search(db, query, limit, offset, [&](const nbl::Tag& tag) -> nbl::Result {

            if(query == tag.name) {
                exactMatch = true;
            }

            TagMeta ref(tag.id, QString::fromUtf8(tag.name.data(), tag.name.length()), QColor::fromRgb(tag.color)); 
            found.append(ref);
            return nbl::Result::SUCCESS;

        }));
    }

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
