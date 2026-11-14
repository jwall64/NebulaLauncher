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
#include <QMutex>
#include <QStringList>
#include <QTimer>
#include <QtQml>

#include <Platform/Process.hpp>
#include <Plugin/ProcessTracker.hpp>


namespace nbl::frontend {

class ProcessHandle : public QObject {

    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningUpdated)
    Q_PROPERTY(bool isStopping READ isStopping NOTIFY isStoppingUpdated)
    Q_PROPERTY(QString logs READ logs NOTIFY logsUpdated)

public:

    ProcessHandle(QObject* parent = nullptr) :
        QObject(parent),
        _bIsStopping(false),
        _mTracker{},
        _mPrevState(nbl::ProcessTracker::State::NOT_STARTED),
        _mWatcherThread{},
        _mLines{},
        _mLogSize(0),
        _mLogsCache{},
        _mLogsCacheValid(false),
        _mLogsDirty(false),
        _mUpdateTimer{}
    {
        _mUpdateTimer.setSingleShot(true);
        _mUpdateTimer.setInterval(50);
        connect(&_mUpdateTimer, &QTimer::timeout, this, &ProcessHandle::_flushLogs);
    }

    ~ProcessHandle() {
        if(_mWatcherThread.isRunning()) {
            _mWatcherThread.join(nullptr);
        }
    }

    inline bool isRunning() {
        return _mPrevState == nbl::ProcessTracker::State::RUNNING;
    }

    inline bool isStopping() {
        return _bIsStopping;
    }

    Q_INVOKABLE void stop() {

        if(_bIsStopping) return;

        _bIsStopping = true;
        emit isStoppingUpdated();

        _mTracker.terminate();
    }

    Q_INVOKABLE void check() {

        _mTracker.check();
        nbl::ProcessTracker::State state = _mTracker.state();

        if(state != _mPrevState) {

            if(_mPrevState == nbl::ProcessTracker::State::RUNNING) {
                _mPrevState = state;
                emit isRunningUpdated();
            } else {
                _mPrevState = state;
                if(state == nbl::ProcessTracker::State::RUNNING) {
                    emit isRunningUpdated();
                }
            }
            if(state == nbl::ProcessTracker::State::COMPLETE) {
                emit complete();
                emit stopped();
            }
            else if(state == nbl::ProcessTracker::State::FAILED) {
                emit failed();
                emit stopped();
            }
        }
    }

    inline nbl::ProcessTracker* tracker() { return &_mTracker; }

    QString logs();

    void appendLines(const QStringList& logLines);
    inline void appendLine(const QString& logLine) {
        appendLines(QStringList { logLine });
    }

    // Joins the output watcher. Call after the process has exited to make sure
    // every buffered line has reached the log store.
    void waitForLogs();

    nbl::Result initLogging();

    inline void pushStatus(QString message) {
        emit statusUpdated(message);
    }
    inline void pushProgress(f64 progress) {
        emit progressUpdated(progress);
    }

signals:

    void isRunningUpdated();
    void isStoppingUpdated();
    void logsUpdated();
    void stopped();
    void complete();
    void failed();
    void statusUpdated(QString message);
    void progressUpdated(double progress);

private:

    // Keep no more than 1 MiB of log text. When the limit is exceeded, drop
    // the oldest quarter so trimming does not happen on every new line.
    static constexpr qsizetype _MAX_LOG_SIZE = 1024 * 1024;
    static constexpr qsizetype _LOG_TRIM_TARGET = _MAX_LOG_SIZE * 3 / 4;

    void _flushLogs();
    void _scheduleLogUpdate();

    bool _bIsStopping;
    nbl::ProcessTracker _mTracker;
    nbl::ProcessTracker::State _mPrevState;
    nbl::Thread _mWatcherThread;

    mutable QMutex _mLogMutex;
    QStringList _mLines;
    qsizetype _mLogSize;
    QString _mLogsCache;
    bool _mLogsCacheValid;
    bool _mLogsDirty;
    QTimer _mUpdateTimer;

};

}
