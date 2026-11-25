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

#include "AccountModel.hpp"
#include "ApplicationContext.hpp"
#include "Data/OnlineAccountModel.hpp"

#include <QDesktopServices>

#include <Core/Result.hpp>
#include <Core/String.hpp>
#include <Platform/Time.hpp>
#include <Platform/Mutex.hpp>
#include <Plugin/AccountProvider.hpp>
#include <Plugin/Field.hpp>
#include <Plugin/PluginContext.hpp>

namespace {

nbl::LoginTracker _tracker{};
nbl::Thread _loginThread{};
bool _setup = false;

nbl::Result _startTracking(void* userData, i64 accountId, const nbl::AccountProvider* provider) {

    if(!_setup) {
        NBL_CHECK(nbl::LoginTracker::create(
            [](bool success, nbl::StringView message, void* userData) {
                if(!success) {                  
                    nbl::PluginContext* ctx = ApplicationContext::instance()->pluginContext();
                    ctx->pushNotification(nbl::NotificationType::ERROR, "Failed to log in", message);
                }

                nbl::frontend::AccountModel* model = static_cast<nbl::frontend::AccountModel*>(userData);
                emit model->loginFinished(success, QString::fromUtf8(message.data(), message.length()));
            },
            [](nbl::StringView message, void* userData) {
                nbl::frontend::AccountModel* model = static_cast<nbl::frontend::AccountModel*>(userData);
                emit model->loginStatus(QString::fromUtf8(message.data(), message.length()));
            },
            [](nbl::StringView url, void* userData) {
                QDesktopServices::openUrl(QUrl(nbl::frontend::fromNbl(url)));
                return true;
            },
            ApplicationContext::instance()->mediaManager(),
            &_tracker
        ));
        _setup = true;
    }

    return _tracker.startTracking(userData, accountId, provider);
}

}

int nbl::frontend::AccountModel::rowCount(const QModelIndex& parent) const {
    return _mData.length();
}

QVariant nbl::frontend::AccountModel::data(const QModelIndex& index, int role) const {

    if(index.row() >= _mData.length()) return {};
    _Row r = _mData[index.row()];

    switch(role) {
        case IdRole:
            return r.id;
        case UsernameRole:
            return r.username;
        case IconRole:
            return nbl::frontend::inlineIcon(r.icon, r.flags & OnlineAccountFlag::IMAGE_ICON);
        case IconIsImageRole:
            return (r.flags & OnlineAccountFlag::IMAGE_ICON) != 0;
        case IconNearestScalingRole:
            return (r.flags & OnlineAccountFlag::ICON_NEAREST_SCALING) != 0;
        case StatusRole:
            return static_cast<quint32>(r.status);
        case FieldValuesRole: {
            if(r.status == OnlineAccountStatus::NOT_LOGGED_IN) return QVariant();
            return nbl::frontend::FieldRef::parse(r.encodedFields, _pProvider->fields);
        }
        default:
            return {};
    }
}

QHash<int, QByteArray> nbl::frontend::AccountModel::roleNames() const {
    QHash<int, QByteArray> out = {};
    out[IdRole] = "id";
    out[UsernameRole] = "username";
    out[IconRole] = "icon";
    out[IconIsImageRole] = "iconIsImage";
    out[IconNearestScalingRole] = "iconNearestScaling";
    out[StatusRole] = "status";
    out[FieldValuesRole] = "fieldValues";
    return out;
}

void nbl::frontend::AccountModel::refresh() {

    ApplicationContext* context = ApplicationContext::instance();

    beginResetModel();
    int preCount = count();
    _mData.clear();

    nbl::OnlineAccount::enumerateForProvider(context->databaseConnection(), _pProvider->id, [&](auto& account) {
        if(account.status == OnlineAccountStatus::NOT_LOGGED_IN) return nbl::Result::SUCCESS;
        _mData.append({
            .id = account.id,
            .encodedFields = fromNbl(account.fields),
            .username = fromNbl(account.username),
            .icon = { .encoded = account.icon },
            .flags = account.flags,
            .status = account.status
        });
        return nbl::Result::SUCCESS;
    });

    if(count() != preCount) {
        emit countChanged();
    }

    endResetModel();
}

nbl::frontend::FieldList* nbl::frontend::AccountModel::fields() const {
    return new nbl::frontend::FieldList(_pProvider->fields);
}

void nbl::frontend::AccountModel::setProviderId(QString providerId) {
    QByteArray utf = providerId.toUtf8();
    StringView id = StringView(utf.data(), utf.length());

    if(_pProvider && _pProvider->id == id) {
        return;
    }

    const AccountProvider* prov = ApplicationContext::instance()->pluginContext()->get<AccountProvider>(id);

    _pProvider = prov;
    emit providerIdChanged();

    if(prov) {
        refresh();
    } else {
        beginResetModel();
        int preCount = count();
        _mData.clear();

        if(count() != preCount) {
            emit countChanged();
        }
        endResetModel();
    }
}

qint32 nbl::frontend::AccountModel::login(QVariantMap fields) {
    
    if(_setup && _tracker.isTracking()) {
        ApplicationContext::instance()->pluginContext()->pushNotification(
            NotificationType::ERROR,
            "Error",
            "You are already signing into another account."
        );
        return nbl::Result::INVALID_STATE;
    }

    i64 newAccount;
    NBL_CHECK(nbl::OnlineAccount::create(
        ApplicationContext::instance()->databaseConnection(), 
        {
            .fields = {},
            .provider = _pProvider->id,
            .updatedAt = nbl::getCurrentTime(),
            .username = {},
            .icon = {},
            .flags = 0,
            .status = OnlineAccountStatus::NOT_LOGGED_IN
        }, 
        &newAccount
    ));

    NBL_CHECK(_startTracking(this, newAccount, _pProvider));

    nbl::Result res = _startLogin(newAccount, fields);
    if(res.error()) {
        _tracker.fail("Failed to start login!");
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::frontend::AccountModel::_startLogin(i64 accountId, QVariantMap fields) {
    Latch copyLatch{};
    NBL_CHECK(nbl::Latch::create(1, &copyLatch));

    struct _UserData {
        const AccountProvider* provider;
        QVariantMap qtFields;
        Latch* copyLatch;
    } _userData = { _pProvider, fields, &copyLatch };

    _loginThread.detach();
    NBL_CHECK(nbl::Thread::create([](void* userData) -> i32 {
    
        _UserData ud = *static_cast<_UserData*>(userData);
        ud.copyLatch->decrement();

        nbl::AllocationFrame frame(nbl::Arena::threadLocal());
        nbl::Field dummy = {
            .type = FieldType::STRUCT,
            .structData = {
                .fields = ud.provider->fields
            }
        };

        nbl::FieldValue value = nbl::frontend::FieldRef(&dummy).toNbl(ud.qtFields, &frame);
        if(ud.provider->login(ApplicationContext::instance()->pluginContext(), value.compositeValue, &_tracker).error()) {
            _tracker.fail("An error occurred while logging in");
        }
        if(_tracker.isTracking()) {
            _tracker.fail("The AccountProvider failed to call complete() or fail() before returning from login()");
        }
        return 0;

    }, &_userData, &_loginThread));

    copyLatch.wait();
    return nbl::Result::SUCCESS;
}

void nbl::frontend::AccountModel::logout(qint64 accountId) {
    _pProvider->logout(ApplicationContext::instance()->pluginContext(), accountId);
    OnlineAccount::remove(ApplicationContext::instance()->databaseConnection(), accountId);
    refresh();
}
