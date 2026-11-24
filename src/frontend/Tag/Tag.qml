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

Control {

    id: root

    required property string text
    required property color color

    property int tooltipDelay: 1000
    
    topPadding: 4
    bottomPadding: 4
    leftPadding: 8
    rightPadding: 8

    implicitWidth: textDisplay.implicitWidth + leftPadding + rightPadding
    implicitHeight: textDisplay.implicitHeight + topPadding + bottomPadding

    property int textSize: 14

    signal clicked

    background: Rectangle {
        anchors.fill: parent
        color: root.color
        radius: root.height / 2
    }

    contentItem: Item {

        Text {

            id: textDisplay
            text: root.text

            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            elide: Qt.ElideRight

            width: root.availableWidth
            height: root.availableHeight

            font.pixelSize: root.textSize
            font.family: Theme.font
            font.variableAxes: { "wght": 600 }
            color: Theme.colors.readableOn(root.color)
        }

        NblToolTip {
            text: root.text
            delay: root.tooltipDelay
            visible: hover.containsMouse
        }

        MouseArea {
            id: hover
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                root.clicked()
            }
        }
    }
}
