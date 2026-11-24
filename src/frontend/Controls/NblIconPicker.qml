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

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects

import Nebula

Item {

    id: root

    implicitWidth: 64
    implicitHeight: 70

    property string icon: "󰊗"
    property bool isImage: false
    property bool colorize: true


    property list<string> presetIcons: [
        "", "", "", "", "", "", "", "", "", "", "", "󰊗", "", "", "", "", "", "󰀄",
        "", "", "", "", "", "", "", "", "", "", "", "", "󰇺", "", "󰓥", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
        "", "", "", "", "", "", "", "", 
        "", "", "", "", "", "", "", 
        "", "", "", "", "", "", "",
        "", "", "󰟡", "󱎓", "󰖫", "󰜭", "", "󰮡", "", "󰯚", "", "", "", "", "", "", "", "", "", "", ""
    ]

    Item {
        width: parent.width
        height: parent.width
        y: parent.height - parent.width

        Rectangle {
            anchors.fill: parent
            radius: 8
            border.width: 2
            border.color: mouseArea.containsMouse ? Theme.colors.text : Theme.colors.border
            color: "transparent"
        }

        Item {

            width: 0
            height: 0
            x: 16
            y: -12

            Rectangle {
                id: textBg
                width: iconText.width + 12
                height: iconText.height - 8
                anchors.centerIn: iconText
                color: Theme.colors.layer0
            }

            Label {
                id: iconText
                text: "Icon"
                font.family: Theme.font
                font.pixelSize: 16
                color: mouseArea.containsMouse ? Theme.colors.text : Theme.colors.textDim
            }
        }

        NblInlineIcon {
            id: iconDisplay
            anchors.fill: parent
            anchors.margins: 12
            icon: root.icon
            isImage: root.isImage
            colorize: root.colorize
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                popup.open()
            }
        }

        Popup {
            id: popup
            visible: false

            width: 300
            height: 300

            x: parent.height + 8

            onOpened: {
                let realCoords = root.mapToItem(null, 0, 0)
                x = Math.min(0, root.Window.width - realCoords.x - 250)
                y = Math.min(parent.height + 8, root.Window.height - realCoords.y - 250)
            }

            background: Rectangle {
                radius: 8
                border.width: 2
                border.color: Theme.colors.border
                color: Theme.colors.layer1
            }

            contentItem: ColumnLayout {

                spacing: 8

                Item {

                    Layout.fillWidth: true
                    Layout.preferredHeight: 64

                    Rectangle {
                        width: parent.height
                        height: parent.height
                        anchors.centerIn: parent
                        color: Theme.colors.layer0
                        border.width: 2
                        border.color: Theme.colors.border
                        radius: 8
                    }

                    TextInput {

                        id: iconInput
                        visible: !root.isImage

                        anchors.centerIn: parent
                        text: root.icon
                        font.pixelSize: 48
                        font.family: Theme.nerdFont

                        color: Theme.colors.text

                        onTextEdited: {
                            if([...text].length > 1) {
                                text = [...text][0]
                            }
                        }

                        onTextChanged: {
                            root.icon = text
                        }

                        onVisibleChanged: {
                            if(!visible) {
                                deselect()
                            }
                        }
                    }

                    Item {
                        visible: root.isImage
                        width: 48
                        height: 48
                        anchors.centerIn: parent
                        NblImage {
                            id: iconImage
                            anchors.fill: parent
                            source: root.isImage ? root.icon : ""
                            fillMode: Image.PreserveAspectFit
                            layer.enabled: true
                            visible: !root.colorize
                        }
                        MultiEffect {
                            visible: root.colorize
                            anchors.fill: iconImage
                            source: iconImage
                            colorization: 1.0
                            colorizationColor: Theme.colors.text
                        }
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: {
                            fileDialog.open()
                        }
                    }
                }

                NblCheckBox {
                    visible: root.isImage
                    text: "Colorize Image"
                    checked: root.colorize
                    onToggled: {
                        root.colorize = checked
                    }
                }

                ScrollView {

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    contentHeight: pickerItems.height

                    Flickable {

                        anchors.fill: parent
                        clip: true
                        
                        Flow {
                            id: pickerItems
                            width: parent.width - 8
                            x: 4
                            spacing: 8
                            Repeater {
                                model: root.presetIcons
                                delegate: NblInlineIcon {
                                    width: 32
                                    height: 32
                                    required property var modelData
                                    icon: modelData

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            root.isImage = false
                                            root.icon = parent.modelData
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Icon Image"
        fileMode: FileDialog.OpenFile
        nameFilters: [ "Image files (*.png *.svg)", "All files (*)" ]

        onAccepted: {
            let icon = fileDialog.selectedFile.toString()
            root.icon = icon
            root.isImage = true
        }

        onRejected: {

        }
    }
}

