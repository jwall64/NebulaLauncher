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
 * \file Main.cpp
 * \author jwall64
 * \brief Main application entry point.
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>

#include <Core/String.hpp>
#include <Platform/Logging.hpp>
#include <Media/MediaImageProvider.hpp>

#include "ApplicationContext.hpp"

int run(int argc, char** argv) {

    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        qputenv("QT_QUICK_CONTROLS_STYLE", "Fusion");
    }

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/qt/qml/Nebula/images/Nebula.png"));
    
    QQmlApplicationEngine engine{};

    engine.addImageProvider(QLatin1String("nblmedia"), new nbl::frontend::MediaImageProvider());

    ApplicationContext* inst = ApplicationContext::instance();
    QQmlEngine::setObjectOwnership(inst, QQmlEngine::CppOwnership);
    inst->init();

    engine.loadFromModule("Nebula", "MainWindow");

    return app.exec();
}

int main(int argc, char** argv) {

    int rc = run(argc, argv);
    if(rc != 0) {
        nbl::log(nbl::LogLevel::ERROR, "Application error: ", rc);
        return rc;
    }

    return 0;
}
