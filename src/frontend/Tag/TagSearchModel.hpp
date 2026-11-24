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

#include <Core/Types.hpp>
#include <Platform/Logging.hpp>

#include <QObject>
#include <QColor>
#include <QtQml>

#include "Tag/TagMeta.hpp"
#include "PagedListModel.hpp"

namespace nbl::frontend {

class TagSearchModel : public PagedListModel {

    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QByteArray searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(bool exactMatch READ exactMatch NOTIFY exactMatchChanged)

public:

    explicit TagSearchModel(QObject* parent = nullptr) :
        PagedListModel(parent),
        _bExactMatch(false),
        _mSearchQuery{},
        _mData{}
    { 
        setPaused(true);
    }

    enum Roles {
        IdRole,
        NameRole,
        ColorRole
    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        Q_UNUSED(parent)
        return _mData.size();
    }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool exactMatch() const { return _bExactMatch; }

    QByteArray searchQuery() {
        return _mSearchQuery;
    }

    void setSearchQuery(QByteArray searchQuery) {
        _mSearchQuery = searchQuery;
        searchQueryChanged();
        refresh();
    }

    Q_INVOKABLE inline qint64 id(qsizetype index) {
        return _mData[index].id;
    }
    Q_INVOKABLE inline QString name(qsizetype index) {
        return _mData[index].name;
    }
    Q_INVOKABLE inline QColor color(qsizetype index) {
        return _mData[index].color;
    }

signals:

    void searchQueryChanged();
    void exactMatchChanged();


protected:

    virtual void _clear() override { 
        _mData.clear(); 
        if(_bExactMatch) {
            _bExactMatch = false;
            emit exactMatchChanged();
        }
    }
    virtual nbl::Result _fetchPageData(qsizetype limit, qsizetype offset) override;

private:

    bool _bExactMatch;
    QByteArray _mSearchQuery;

    QList<TagMeta> _mData;

};

}
