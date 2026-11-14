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
#include <QtQml>

#include <Platform/Process.hpp>
#include <Plugin/ProcessTracker.hpp>
#include <Plugin/LogTransformer.hpp>


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
        _mWatcherThread{},
        _mLogs(""),
        _mPluginContext(nullptr),
        _mLogTransformer{},
        _mLogState(nullptr)
    { }

    ~ProcessHandle() {
        if(_mWatcherThread.isRunning()) {
            _mWatcherThread.join(nullptr);
        }

        if(_mLogTransformer.pfnDestroy != nullptr && _mLogState != nullptr) {
            _mLogTransformer.pfnDestroy(_mPluginContext, _mLogState, _mLogTransformer.userData);
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

    inline void setLogTransformer(nbl::PluginContext* context, const nbl::LogTransformFunction& transformer) {
        _mPluginContext = context;
        _mLogTransformer = transformer;
    }

    inline bool hasLogTransformer() const {
        return _mLogTransformer.valid();
    }

    nbl::Result transformLine(nbl::StringView line, QString* outLine);

    inline QString logs() {
        return _mLogs;
    }

    inline void appendLine(QString logLine) {
        _mLogs += logLine + '\n';
        emit logsUpdated();
    }

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

    bool _bIsStopping;
    nbl::ProcessTracker _mTracker;
    nbl::ProcessTracker::State _mPrevState;
    nbl::Thread _mWatcherThread;
    QString _mLogs;

    nbl::PluginContext* _mPluginContext;
    nbl::LogTransformFunction _mLogTransformer;
    void* _mLogState;

};

}
