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

import Nebula

Item {
    id: root

    property string text: ""
    property list<var> badges: []
    property color color: Theme.colors.text

    property alias font: titleText.font

    implicitHeight: titleText.height

    Row {
        id: row
        width: parent.width
        Text {
            id: titleText
            width: Math.max(
                0,
                Math.min(
                    implicitWidth,
                    root.width - (16 + row.spacing) * root.badges.length
                )
            )

            maximumLineCount: 1
            text: root.text

            verticalAlignment: Qt.AlignVCenter

            font.family: Theme.font
            font.pixelSize: 16
            font.variableAxes: { "wght": 600 }

            color: root.color
            elide: Text.ElideRight
        }
        Repeater {
            model: root.badges
            delegate: Item {
                id: badgeItem
                required property var modelData
                height: root.height
                width: height
                Label {
                    anchors.fill: parent
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                    text: badgeItem.modelData.icon
                    color: root.color
                }
                NblToolTip {
                    text: badgeItem.modelData.text
                    delay: 500
                    visible: progressionHover.containsMouse
                }
                MouseArea {
                    id: progressionHover
                    anchors.fill: parent
                    hoverEnabled: true
                }
            }
        }
    }
}

