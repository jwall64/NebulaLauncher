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

Control {
    id: root
    property string label
    property string tooltip
    property int tooltipDelay

    readonly property int backgroundOffset: root.font.pixelSize * 0.9

    property color labelColor: enabled ? Theme.colors.textDim : Theme.colors.border
    property color borderColor: Theme.colors.border

    default property alias content: contentItem.children

    font.family: Theme.font
    padding: 4

    Rectangle {
        id: background
        color: Theme.colors.layer0
        radius: 8
        width: parent.width
        height: parent.height - root.backgroundOffset
        y: root.backgroundOffset
        border.width: 2
        border.color: root.borderColor
    }

    Item {
        id: contentItem
        width: background.width - root.leftPadding - root.rightPadding
        height: background.height - root.topPadding - root.bottomPadding
        x: root.leftPadding
        y: root.topPadding + background.y
    }

    Item {

        visible: root.label && root.label.length > 0

        width: 0
        height: 0
        x: 16

        Rectangle { 
            width: labelText.width + 12
            height: labelText.height - 8
            anchors.centerIn: labelText
            color: Theme.colors.layer0 

            NblToolTip {
                delay: root.tooltipDelay
                text: root.tooltip
                visible: hover.containsMouse
            }

            MouseArea {
                id: hover
                anchors.fill: parent
                hoverEnabled: root.tooltip.length > 0
            }
        }
        Label { 
            id: labelText
            text: root.label
            color: root.labelColor
        }
    }
}

