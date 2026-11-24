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

    property bool collapsed: true
    property real contentHeight: 48
    property real collapsedHeight: 48
    property string label: ""
    property string icon: ""
    property bool colorizeIcon: true
    property color iconColor: Theme.colors.text
    property int iconRadius: 0
    property int iconSize: collapsedHeight - 8
    property bool bold: false

    readonly property real contentWidth: width - collapsedHeight

    default property alias content: contentContainer.children
    readonly property bool effectivelyCollapsed: collapsed || !enabled

    implicitHeight: effectivelyCollapsed ? collapsedHeight : contentHeight

    onEnabledChanged: {
        if(!enabled) {
            collapsed = true
        }
    }


    Item {

        width: root.effectivelyCollapsed ? parent.width : root.collapsedHeight
        height: root.collapsedHeight

        Rectangle {
            anchors.fill: parent
            visible: root.effectivelyCollapsed
            color: root.enabled && collapseMouseArea.containsMouse ? Theme.colors.layer1 : "transparent"
            radius: 8
        }

        Label {

            width: root.collapsedHeight
            height: root.collapsedHeight

            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter

            text: root.effectivelyCollapsed ? "" : ""
            font.family: Theme.nerdFont
            font.pixelSize: 20
            color: Theme.colors.textDim

        }

        MouseArea {
            id: collapseMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                root.collapsed = !root.collapsed
            }
        }

        RowLayout {
            visible: root.effectivelyCollapsed

            spacing: 8

            height: root.collapsedHeight
            x: root.collapsedHeight

            Item {
                Layout.preferredWidth: Math.min(root.iconSize, root.collapsedHeight)
                Layout.fillHeight: true
                visible: root.icon.length > 0

                NblInlineIcon {
                    id: iconSource
                    width: parent.width
                    height: parent.width
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.iconColor
                    icon: root.icon
                    colorize: root.colorizeIcon
                    imageRadius: root.iconRadius
                }
            }

            Label {
                Layout.fillHeight: true
                Layout.fillWidth: true

                verticalAlignment: Qt.AlignVCenter

                text: root.label
                font.family: Theme.font
                font.pixelSize: 16
                font.bold: root.bold
                color: Theme.colors.text
            }
        }
    }

    Item {

        visible: !root.effectivelyCollapsed
        width: parent.width
        height: root.contentHeight

        Rectangle {
            width: 2
            height: parent.height - root.collapsedHeight
            y: root.collapsedHeight
            x: (root.collapsedHeight / 2) - 1
            color: Theme.colors.border
        }

        Item {
            id: contentContainer

            width: root.contentWidth
            height: root.contentHeight
            x: root.collapsedHeight
        }

    }

}
