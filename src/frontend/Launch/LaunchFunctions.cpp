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
 * \file LaunchFunctions.cpp
 * \author jwall64
 * \brief Partial implementation of ApplicationContext.hpp, containing functions related to launching games
 */

#include "ApplicationContext.hpp"
#include "Launch/ProcessHandle.hpp"

#include <Plugin/InstallationType.hpp>

#include <QDesktopServices>

namespace {


void _pushStatus(i64 id, nbl::Optional<nbl::StringView> status, nbl::Optional<f64> progress) {
    ApplicationContext* ctx = ApplicationContext::instance();
    nbl::frontend::ProcessHandle* hnd = ctx->getRunningInstallation(id);
    if(hnd == nullptr) return;

    if(status.hasValue()) {
        QString statusStr = QString::fromUtf8(status->data(), status->length());
        hnd->pushStatus(statusStr);
    }
    if(progress.hasValue()) {
        hnd->pushProgress(*progress);
    }
}


}


nbl::frontend::ProcessHandle* ApplicationContext::launchInstallation(qint64 installationId) {

    if(!databaseConnection()->isValid()) {
        nbl::log(nbl::LogLevel::ERROR, "Cannot launch installation, database connection is not valid");
        return nullptr;
    }

    if(_mRunningProcesses.contains(installationId)) {
        nbl::frontend::ProcessHandle* handle = _mRunningProcesses[installationId];
        if(handle->isRunning()) {
            _mContext.pushNotification(nbl::NotificationType::ERROR, "Cannot start installation", "A process for this installation is already running!");
            return nullptr;
        }
    }

    nbl::frontend::ProcessHandle* handle = new nbl::frontend::ProcessHandle();
    QQmlEngine::setObjectOwnership(handle, QQmlEngine::CppOwnership);

    connect(handle, &nbl::frontend::ProcessHandle::stopped, this, [this, installationId, handle]() {
        _mRunningProcesses.remove(installationId);
        handle->deleteLater();
    });

    if(nbl::ProcessTracker::create(installationId, _pushStatus, handle->tracker()).error()) {
        _mContext.pushNotification(nbl::NotificationType::ERROR, "Failed to launch", "Failed to create process tracker");
        handle->deleteLater();
        return nullptr;
    }
    _mRunningProcesses[installationId] = handle;


    nbl::Thread launchThread{};
    nbl::Result res = nbl::Thread::create([](void* userData) -> i32 {

        i64 installationId = reinterpret_cast<i64>(userData);
        ApplicationContext* ctx = ApplicationContext::instance();

        nbl::frontend::ProcessHandle* handle = ctx->_mRunningProcesses[installationId];
        if(handle == nullptr) return -1;

        nbl::Result res = nbl::Installation::get(ctx->databaseConnection(), installationId, [&](const nbl::Installation& meta) -> nbl::Result {

            if(meta.deviceId != ctx->deviceId()) {
                nbl::log(nbl::LogLevel::ERROR, "Attempt to launch an installation from another device! (", meta.deviceId, " != ", ctx->deviceId(), ")");
                return nbl::Result::INVALID_STATE;
            }

            const nbl::InstallationType* installType = ctx->_mContext.get<nbl::InstallationType>(meta.type);
            if(installType == nullptr) {
                nbl::log(nbl::LogLevel::ERROR, "Unknown installation type: ", meta.type);
                return nbl::Result::INVALID_STATE;
            }

            nbl::AllocationFrame frame(nbl::Arena::threadLocal());
            nbl::Span<nbl::FieldValue> values = {};

            if(installType->fields.length() > 0) {
                nbl::JsonElement value = {};
                NBL_CHECK(nbl::parseJson(meta.command, &frame, &value));
                NBL_CHECK(nbl::decodeFieldValues(&ctx->_mContext, installType->fields, value.asObject(), &values, &frame));
            }

            handle->setLogTransformer(ctx->pluginContext(), installType->logTransform);
            NBL_CHECK(installType->launch(&ctx->_mContext, values, meta, handle->tracker()));
            NBL_CHECK(handle->initLogging());
            handle->check();

            return nbl::Result::SUCCESS;
        });

        if(res.error()) {
            handle->deleteLater();
            ctx->_mContext.pushNotification(nbl::NotificationType::ERROR, "Failed to launch", "An error occurred while launching an installation");
        }

        return res.value;

    }, reinterpret_cast<void*>(installationId), &launchThread);


    if(res.error()) {
        handle->deleteLater();
        _mContext.pushNotification(nbl::NotificationType::ERROR, "Failed to launch", "Failed to spawn a new thread");
        return nullptr;
    }

    return handle;
}

nbl::frontend::ProcessHandle* ApplicationContext::getRunningInstallation(qint64 installationId) {
    if(!_mRunningProcesses.contains(installationId)) return nullptr;
    return _mRunningProcesses[installationId];
}

int ApplicationContext::openInstallationFolder(qint64 installationId) {
 
    if(!databaseConnection()->isValid()) {
        nbl::log(nbl::LogLevel::ERROR, "Cannot open installation folder, database connection is not valid");
        return -1;
    }

    nbl::frontend::ProcessHandle* handle = new nbl::frontend::ProcessHandle();

    nbl::Result res = nbl::Installation::get(databaseConnection(), installationId, [&](const nbl::Installation& meta) -> nbl::Result {

        if(meta.deviceId != deviceId()) {
            nbl::log(nbl::LogLevel::ERROR, "Attempt to open an installation folder for another device!");
            return nbl::Result::INVALID_STATE;
        }

        const nbl::InstallationType* installType = _mContext.get<nbl::InstallationType>(meta.type);
        if(installType == nullptr) {
            nbl::log(nbl::LogLevel::ERROR, "Unknown installation type: ", meta.type);
            return nbl::Result::INVALID_STATE;
        }

        nbl::AllocationFrame frame(nbl::Arena::threadLocal());
        nbl::FieldValue* values;
        NBL_CHECK(frame.allocate(&values, installType->fields.length()));
        nbl::Span<nbl::FieldValue> valueSpan(values, installType->fields.length());

        nbl::JsonElement value = {};
        NBL_CHECK(nbl::parseJson(meta.command, &frame, &value));
        NBL_CHECK(nbl::decodeFieldValues(&_mContext, installType->fields, value.asObject(), &valueSpan, &frame));

        nbl::StringView folder = {};
        NBL_CHECK(installType->getFolder(&_mContext, valueSpan, meta, &folder));

        if(folder.empty()) {
            _mContext.pushNotification(nbl::NotificationType::ERROR, "Could not open folder", "The installation did not specify a valid installation path");
            return nbl::Result::SUCCESS;
        }

        QDesktopServices::openUrl(QUrl::fromLocalFile(nbl::frontend::fromNbl(folder)));
        return nbl::Result::SUCCESS;
    });

    if(res.error()) {
        _mContext.pushNotification(nbl::NotificationType::ERROR, "Could not open folder", "An error occurred while getting the installation folder.");
    }

    return res;
}

