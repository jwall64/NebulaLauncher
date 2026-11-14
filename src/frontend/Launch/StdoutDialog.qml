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

NblDialog {

    id: root

    property ProcessHandle process: null
    property string retainedLogs: ""

    targetWidth: 1200
    targetHeight: 800

    showApplyButton: false
    showOkButton: false

    cancelText: "Back"

    navigation: [
        NblCheckBox {
            id: autoScrollBox
            anchors.verticalCenter: parent.verticalCenter
            text: "Auto Scroll"
            checked: true
            onToggled: {
                flickable.scrollToBottom()
            }
        }
    ]

    function refreshLogs() {
        outputContent.text = process != null ? process.logs : retainedLogs
    }

    function appendLogs() {
        if(process == null) return

        let logs = process.logs
        let oldLength = outputContent.text.length

        // Avoid re-rendering the whole log on every update. The ring buffer only
        // trims occasionally, so the text is usually an extension of what QML
        // already has.
        if(logs.startsWith(outputContent.text)) {
            outputContent.insert(oldLength, logs.substring(oldLength))
        } else {
            outputContent.text = logs
        }
    }

    onProcessChanged: refreshLogs()
    onRetainedLogsChanged: {
        if(process == null) refreshLogs()
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: "black"
    }

    Label {
        visible: !scrollView.visible
        anchors.centerIn: parent

        text: "The installation is not running."
        color: "white"

        font.pixelSize: 20
        font.family: Theme.font
    }

    ScrollView {

        visible: root.process != null || outputContent.text.length > 0

        id: scrollView

        anchors.fill: parent
        width: parent.width
        height: parent.height - 16
        y: 8

        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        contentHeight: outputContent.height

        property bool isScrolling: ScrollBar.vertical.active

        Flickable {

            id: flickable

            anchors.fill: parent
            clip: true

            property bool autoScroll: autoScrollBox.checked
            property real lastY: contentY

            Connections {
                target: root.process
                function onLogsUpdated() {
                    if(root.process == null) return
                    root.appendLogs()
                    flickable.scrollToBottom()
                }
                function onStopped() {
                    // Keep the final output around after the process handle goes away.
                    if(root.process != null) {
                        root.retainedLogs = root.process.logs
                    }
                    root.process = null
                }
            }

            onHeightChanged: {
                scrollToBottom()
            }

            onContentYChanged: {
                if(scrollView.isScrolling && flickable.contentY < lastY) {
                    autoScrollBox.checked = false
                }
                lastY = contentY
            }

            function scrollToBottom() {
                if(!autoScroll) return
                if(flickable.contentHeight > flickable.height) {
                    flickable.contentY = flickable.contentHeight - flickable.height
                }
            }

            TextEdit {

                id: outputContent

                width: parent.width - 16
                x: 8

                color: "white"
                font.pixelSize: 16
                font.family: Theme.monospaceFont

                wrapMode: TextEdit.WordWrap
                readOnly: true
                selectByMouse: true
                selectionColor: Theme.colors.accent
                selectedTextColor: Theme.colors.textOnAccent
            }
        }
    }
}
