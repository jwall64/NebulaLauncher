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
#include "Launch/ProcessHandle.hpp"

#include <QMetaObject>
#include <QRegularExpression>
#include <QThread>

#include <Platform/Logging.hpp>
#include <Core/Result.hpp>

#define BACKLOG_SIZE 1024 * 1024 // 1 MiB

namespace {

i32 _threadProc(void* userData) {

    nbl::frontend::ProcessHandle* handle = static_cast<nbl::frontend::ProcessHandle*>(userData);
    QString currentLine = "";
    QStringList lines = {};

    static const QRegularExpression ansiRegex("\x1B\\[[0-9;]*[a-zA-Z]");

    nbl::Reader reader = handle->tracker()->handle()->stdoutReader();
    while(true) {

        char copyBuffer[4096];
        usize bytesRead = 0;
        nbl::Result rc = reader.read(copyBuffer, 4096, &bytesRead);
        if(rc.error()) {
            return 1;
        }

        for(usize i = 0 ; i < bytesRead ; i++) {
            char c = copyBuffer[i];
            if(c == '\n') {
                lines.append(currentLine.remove(ansiRegex));
                currentLine.clear();
            } else {
                currentLine += c;
            }
        }

        if(!lines.isEmpty()) {
            handle->appendLines(lines);
            lines.clear();
        }

        if(rc == nbl::Result::END_OF_STREAM) {
            break;
        }
    }

    // Flush a final line that did not end with a newline.
    if(!currentLine.isEmpty()) {
        handle->appendLine(currentLine.remove(ansiRegex));
    }

    return 0;
}

}


QString nbl::frontend::ProcessHandle::logs() {

    QMutexLocker locker(&_mLogMutex);

    if(_mLogsCacheValid) return _mLogsCache;

    _mLogsCache.clear();
    if(!_mLines.isEmpty()) {
        _mLogsCache = _mLines.join('\n');
        _mLogsCache += '\n';
    }
    _mLogsCacheValid = true;

    return _mLogsCache;
}


void nbl::frontend::ProcessHandle::appendLines(const QStringList& logLines) {

    if(logLines.isEmpty()) return;

    {
        QMutexLocker locker(&_mLogMutex);

        for(const QString& line : logLines) {
            _mLines.append(line);
            _mLogSize += line.size() + 1;
        }

        if(_mLogSize > _MAX_LOG_SIZE) {
            while(_mLogSize > _LOG_TRIM_TARGET && !_mLines.isEmpty()) {
                _mLogSize -= _mLines.takeFirst().size() + 1;
            }
        }

        _mLogsCacheValid = false;
    }

    if(QThread::currentThread() == thread()) {
        _scheduleLogUpdate();
    } else {
        QMetaObject::invokeMethod(this, [this]() {
            _scheduleLogUpdate();
        }, Qt::QueuedConnection);
    }
}


void nbl::frontend::ProcessHandle::waitForLogs() {
    if(_mWatcherThread.isRunning()) {
        _mWatcherThread.join(nullptr);
    }
}


void nbl::frontend::ProcessHandle::_scheduleLogUpdate() {
    _mLogsDirty = true;

    // Coalesce bursts of output into at most one QML update every 50 ms.
    if(!_mUpdateTimer.isActive()) {
        _mUpdateTimer.start();
    }
}


void nbl::frontend::ProcessHandle::_flushLogs() {
    if(_mLogsDirty) {
        _mLogsDirty = false;
        emit logsUpdated();
    }
}


nbl::Result nbl::frontend::ProcessHandle::initLogging() {

    NBL_CHECK(nbl::Thread::create(_threadProc, this, &_mWatcherThread));
    return nbl::Result::SUCCESS;

}

