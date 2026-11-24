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
#include <QtQml>

#include "PagedListModel.hpp"

namespace nbl::frontend {

class GameSearchModel : public PagedListModel {

    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QByteArray searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(bool exactMatch READ exactMatch NOTIFY exactMatchChanged)
    Q_PROPERTY(bool includeNewGame READ includeNewGame WRITE setIncludeNewGame NOTIFY includeNewGameChanged)

public:

    explicit GameSearchModel(QObject* parent = nullptr) :
        PagedListModel(parent),
        _bExactMatch(false),
        _bIncludeNewGame(false),
        _mSearchQuery{},
        _mData{}
    { }

    enum Roles {
        IdRole,
        TitleRole,
        PosterRole,
        IconRole
    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        Q_UNUSED(parent)
        return _mData.size();
    }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QByteArray searchQuery() { return _mSearchQuery; }
    bool exactMatch() const { return _bExactMatch; }
    bool includeNewGame() const { return _bIncludeNewGame; }

    void setIncludeNewGame(bool includeNewGame) { 
        _bIncludeNewGame = includeNewGame; 
        includeNewGameChanged();
        refresh(); 
    }
    void setSearchQuery(QByteArray searchQuery) {
        _mSearchQuery = searchQuery;
        searchQueryChanged();
        refresh();
    }

signals:

    void searchQueryChanged();
    void exactMatchChanged();
    void includeNewGameChanged();

protected:

    virtual void _clear() override { _mData.clear(); }
    virtual nbl::Result _fetchPageData(qsizetype limit, qsizetype offset) override;

private:

    struct _Row {
        qint64 gId;
        QByteArray title;
        qint64 poster;
        qint64 icon;
    };

    void _fetchPageData();

    bool _bExactMatch;
    bool _bIncludeNewGame;
    QByteArray _mSearchQuery;

    QList<_Row> _mData;

};

}
