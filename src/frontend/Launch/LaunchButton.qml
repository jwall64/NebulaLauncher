// Copyright 2026 jwall64
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import QtQuick
import QtQuick.Controls
import QtQuick.Effects

import Nebula

Item {

    id: root

    required property int gameId
    required property int installationId
    property ProcessHandle handle

    property string message: "Starting..."
    property double progress: 0

    onInstallationIdChanged: {
        handle = ApplicationContext.getRunningInstallation(installationId)
        processState = checkState()
    }

    signal installationAdded
    signal applicationStopped

    implicitHeight: 72
    implicitWidth: 212

    enum State {
        NOT_INSTALLED,
        NOT_RUNNING,
        STARTING,
        RUNNING,
        STOPPING
    }

    property int processState: LaunchButton.State.NOT_INSTALLED

    onHandleChanged: {
        processState = checkState()
    }

    function checkState() {

        let wasRunning = processState === LaunchButton.State.RUNNING || state === LaunchButton.State.STOPPING

        if(installationId == 0) return LaunchButton.State.NOT_INSTALLED

        if(!handle) {
            if(wasRunning) applicationStopped()
            return LaunchButton.State.NOT_RUNNING
        }
        if(handle.isRunning) {
            if(handle.isStopping) {
                return LaunchButton.State.STOPPING 
            } else {
                return LaunchButton.State.RUNNING
            }
        } else {
            return LaunchButton.State.STARTING
        }
    }

    Rectangle {

        id: background

        height: parent.height
        width: parent.width

        property bool textVisible: width > 150

        color: (root.processState === LaunchButton.State.STOPPING || root.processState === LaunchButton.State.STARTING) 
            ? Theme.colors.layer1
            : Theme.colors.accent
        radius: 8

        Row {

            visible: root.processState === LaunchButton.State.NOT_INSTALLED

            spacing: 8
            anchors.centerIn: parent

            Label {
                text: ""
                font.family: Theme.nerdFont
                font.pixelSize: 28
                color: Theme.colors.textOnAccent
            }
            Label {
                visible: background.textVisible
                text: "INSTALL"
                font.family: Theme.font
                font.pixelSize: 28
                font.bold: true
                color: Theme.colors.textOnAccent
            }
        }

        Row {

            visible: root.processState === LaunchButton.State.NOT_RUNNING

            spacing: 8
            anchors.centerIn: parent

            Label {
                text: ""
                font.family: Theme.nerdFont
                font.pixelSize: 28
                color: Theme.colors.textOnAccent
            }
            Label {
                visible: background.textVisible
                text: "LAUNCH"
                font.family: Theme.font
                font.pixelSize: 28
                font.bold: true
                color: Theme.colors.textOnAccent
            }
        }

        Row {

            visible: root.processState === LaunchButton.State.RUNNING

            spacing: 8
            anchors.centerIn: parent

            Label {
                text: ""
                font.family: Theme.nerdFont
                font.pixelSize: 28
                color: Theme.colors.textOnAccent
            }
            Label {
                visible: background.textVisible
                text: "STOP"
                font.family: Theme.font
                font.pixelSize: 28
                font.bold: true
                color: Theme.colors.textOnAccent
            }
        }

        Row {

            visible: root.processState === LaunchButton.State.STARTING

            spacing: 8
            anchors.centerIn: parent

            Label {
                visible: !background.textVisible
                text: ""
                font.family: Theme.nerdFont
                font.pixelSize: 28
                color: Theme.colors.textDim
            }

            Label {
                visible: background.textVisible
                text: root.message
                font.family: Theme.font
                font.pixelSize: 16
                color: Theme.colors.textDim
            }
        }

        Item {
            visible: root.processState === LaunchButton.State.STARTING
            anchors.fill: background

            Item {
                id: maskShape
                anchors.fill: parent
                layer.enabled: true
                visible: false
                Rectangle {
                    anchors.fill: parent
                    radius: background.radius
                }
            }

            Rectangle {
                id: fillShape
                anchors.fill: parent

                color: "transparent"
                layer.enabled: true
                visible: false

                Rectangle {
                    width: parent.width * Math.min(1, Math.max(0, root.progress))
                    height: 8
                    anchors.bottom: parent.bottom
                    color: Theme.colors.textDim

                    Behavior on width {
                        NumberAnimation { duration: 250 }
                    }
                }
            }

            MultiEffect {
                source: fillShape
                anchors.fill: fillShape
                maskEnabled: true
                maskSource: maskShape
            }
        }

        Row {

            visible: root.processState === LaunchButton.State.STOPPING

            spacing: 8
            anchors.centerIn: parent

            Label {
                visible: !background.textVisible
                text: ""
                font.family: Theme.nerdFont
                font.pixelSize: 28
                color: Theme.colors.textDim
            }

            Label {
                visible: background.textVisible
                text: "Stopping..."
                font.family: Theme.font
                font.pixelSize: 16
                color: Theme.colors.textDim
            }
        }
    }

    Timer {
        interval: 1000
        repeat: true
        running: root.handle != null

        onTriggered: {
            root.handle.check()
            root.processState = root.checkState()
        }
    }

    Connections {
        target: root.handle
        function onStopped() {
            root.message = "Starting..."
            root.progress = 0
            root.handle = null
        }
        function onStatusUpdated(message) {
            root.message = message
            root.handle.check()
        }
        function onProgressUpdated(progress) {
            root.progress = progress
            root.handle.check()
        }
    }

    MouseArea {
        anchors.fill: background
        cursorShape: (root.processState === LaunchButton.State.STOPPING || root.processState === LaunchButton.State.STARTING) ? Qt.ArrowCursor : Qt.PointingHandCursor
        hoverEnabled: true
        onClicked: {
            if(root.processState === LaunchButton.State.NOT_INSTALLED) {
                let dialog = DialogManager.create("Installation/AddInstallationDialog.qml", { 
                    gameId: root.gameId
                }) as NblDialog
                dialog.apply.connect(function() {
                    root.installationAdded()
                })
                dialog.open()
            }
            else if(root.processState === LaunchButton.State.STOPPING || root.processState === LaunchButton.State.STARTING) return
            else if(root.processState === LaunchButton.State.RUNNING) {
                root.handle.stop()
            } else {
                root.handle = ApplicationContext.launchInstallation(root.installationId)
            }
        }
    }
}

