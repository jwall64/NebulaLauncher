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

    required property string icon
    required property string name
    property var alignment: Text.AlignLeft
    property bool colorize: true
    property bool iconIsImage: false

    signal clicked

    property bool collapsed: false

    height: 48

    Rectangle {
        id: background
        anchors.fill: parent
        color: mouseArea.containsMouse ? Theme.colors.border : Theme.colors.layer0
        radius: 8
    }

    MouseArea {
        id: mouseArea
        anchors.fill: background
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.clicked()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Rectangle {
            color: "transparent"
            implicitWidth: 32
            implicitHeight: 32

            NblInlineIcon {
                width: 24
                height: 24
                anchors.centerIn: parent

                icon: root.icon
                isImage: root.iconIsImage
                color: mouseArea.containsMouse ? Theme.colors.text : Theme.colors.textDim
                colorize: root.colorize
            }
        }

        Rectangle {
            color: "transparent"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter
            visible: !root.collapsed
            
            Label {
                width: parent.width - (root.alignment == Qt.AlignHCenter ? 32 : 0)
                anchors.verticalCenter: parent.verticalCenter
                text: root.name
                font.family: Theme.font
                font.pixelSize: 16
                elide: Qt.ElideRight
                horizontalAlignment: root.alignment
                color: mouseArea.containsMouse ? Theme.colors.text : Theme.colors.textDim
            }
        }

    }

}
