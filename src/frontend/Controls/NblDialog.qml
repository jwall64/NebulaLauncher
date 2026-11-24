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
import QtQuick.Layouts

import Nebula

Item {

    id: root

    default property alias content: contentContainer.children

    property list<QtObject> above: []
    property list<QtObject> navigation: []

    property string applyText: "Apply"
    property string okText: "OK"
    property string cancelText: "Cancel"

    property bool showApplyButton: true
    property bool showOkButton: true
    property bool showCancelButton: true

    property color okButtonColor: Theme.colors.accent

    property int targetWidth: 800
    property int targetHeight: 600

    property int padding: 16
    property int contentPadding: 0
    property int navigationPadding: 0
    property int navigationGap: 8

    readonly property int navigationHeight: navigationContainer.height + navigationPadding * 2

    signal apply
    signal cancel
    signal closed

    width: Window.width
    height: Window.height

    function open() { 
        popup.open()
    }

    function close() {
        popup.close()
    }

    Popup {
        id: popup

        onClosed: {
            root.closed()
            root.parent = null
        }

        width: root.width
        height: root.height
        modal: true

        function snapToPixel(v) {
            const ratio = Screen.devicePixelRatio
            return Math.round(v * ratio) / ratio
        }

        topPadding: snapToPixel((height - Math.min(height - 16, root.targetHeight)) / 2)
        bottomPadding: topPadding

        leftPadding: snapToPixel((width - Math.min(width - 16, root.targetWidth)) / 2)
        rightPadding: leftPadding

        background: Rectangle {
            anchors.fill: parent
            opacity: 0.3
            color: "black"
        }

        contentItem: Item {

            Rectangle {
                anchors.fill: parent
                color: Theme.colors.layer0
                radius: 16
            }

            ColumnLayout {

                anchors.fill: parent
                anchors.margins: root.padding
                spacing: root.navigationGap

                Item {
                    id: contentContainer
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: root.contentPadding
                }

                Item {

                    id: navigationContainer

                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    Layout.margins: root.navigationPadding

                    RowLayout {

                        anchors.fill: parent

                        layoutDirection: Qt.RightToLeft
                        spacing: 8

                        Item {

                            id: okBtn
                            visible: root.showOkButton

                            Layout.preferredWidth: 128
                            Layout.fillHeight: true

                            Rectangle {
                                anchors.fill: parent
                                color: root.okButtonColor
                                radius: 8
                            }

                            Label {
                                anchors.centerIn: parent
                                font.pixelSize: 16
                                font.family: Theme.font
                                color: Theme.colors.blackOrWhite(root.okButtonColor)
                                text: root.okText
                            }

                            MouseArea {
                                id: addMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.apply()
                                    popup.close()
                                }
                            }
                        }

                        Item {

                            id: applyBtn
                            visible: root.showApplyButton

                            Layout.preferredWidth: 128
                            Layout.fillHeight: true

                            Rectangle {
                                anchors.fill: parent
                                color: okMouseArea.containsMouse ? Theme.colors.border : Theme.colors.layer1
                                radius: 8
                            }

                            Label {
                                anchors.centerIn: parent
                                font.pixelSize: 16
                                font.family: Theme.font
                                color: Theme.colors.text
                                text: root.applyText
                            }

                            MouseArea {
                                id: okMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.apply()
                                }
                            }
                        }

                        Item {

                            id: cancelBtn
                            visible: root.showCancelButton

                            Layout.preferredWidth: 128
                            Layout.fillHeight: true

                            Rectangle {
                                anchors.fill: parent
                                color: cancelMouseArea.containsMouse ? Theme.colors.border : Theme.colors.layer1
                                radius: 8
                            }

                            Label {
                                anchors.centerIn: parent
                                font.pixelSize: 16
                                font.family: Theme.font
                                color: Theme.colors.text
                                text: root.cancelText
                            }

                            MouseArea {
                                id: cancelMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.cancel()
                                    popup.close()
                                }
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            children: root.navigation
                        }
                    }
                }
            }
        }

        Item {
            anchors.fill: parent
            children: root.above
        }
    }
}
