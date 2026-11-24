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

import Nebula

Item {

    id: root

    property string label: ""
    property alias selectedColor: colorPicker.color

    signal accepted

    implicitWidth: Math.max(72, labelText.implicitWidth + 8)

    Rectangle {
        anchors.fill: parent
        anchors.margins: 4
        color: root.selectedColor
        radius: 4
    }

    Label {
        id: labelText
        text: root.label
        font.family: Theme.font
        font.pixelSize: 14
        font.variableAxes: { "wght": 600 }
        color: Theme.colors.readableOn(root.selectedColor)
        anchors.centerIn: parent
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            colorPickerPopup.open()
        }
    }

    Popup {
                            
        id: colorPickerPopup

        width: 250
        height: 250

        onOpened: {
            let realCoords = root.mapToItem(null, 0, 0)
            x = Math.min(0, root.Window.width - realCoords.x - 250)
            y = Math.min(root.height + 8, root.Window.height - realCoords.y - 250)
        }

        modal: false
        clip: true
        padding: 8

        background: Rectangle {
            id: background
            anchors.fill: parent
            color: Theme.colors.layer0
            border.width: 2
            border.color: Theme.colors.border
            radius: 8
        }

        contentItem: NblColorPicker {
            id: colorPicker
        }

        onClosed: {
            root.accepted()
        }
    } 
}

