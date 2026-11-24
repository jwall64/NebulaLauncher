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
import QtQuick.Effects
import QtQuick.Shapes

import Nebula

Item {

    id: root

    property string title
    property string icon
    property color borderColor: "white"
    property bool unlocked: false
    property bool hidden: false
    property bool rare: false

    property int tooltipDelay: 1000
    property int borderThickness: 4

    readonly property color trueBorderColor: unlocked ? Theme.colors.mixSourceOver(borderColor, Theme.colors.accent) : Theme.colors.border

    signal clicked

    // Standard Border
    Rectangle {
        anchors.fill: parent
        visible: !rareBorder.visible
        radius: 8
        color: root.trueBorderColor
    }

    // Rare Border
    Item {
        anchors.fill: parent

        id: rareBorder
        visible: root.rare && !root.hidden && root.unlocked
        Shape {
            anchors.fill: parent        
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                PathRectangle {
                    width: rareBorder.width
                    height: rareBorder.height
                    radius: 8
                }

                strokeColor: "transparent"
                strokeWidth: -1 

                fillGradient: ConicalGradient {

                    centerX: rareBorder.width / 2
                    centerY: rareBorder.height / 2
                    angle: 0
                    NumberAnimation on angle {
                        from: 0
                        to: 360
                        duration: 4000
                        loops: Animation.Infinite
                        running: true
                    }

                    GradientStop { position: 0.0; color: root.trueBorderColor.darker(1.5) }
                    GradientStop { position: 0.5; color: root.trueBorderColor.lighter(1.5) }
                    GradientStop { position: 1.0; color: root.trueBorderColor.darker(1.5) }
                }
            }
        }
    }


    // Icon
    Item {
        anchors.fill: parent
        anchors.margins: root.borderThickness
        visible: !root.hidden
        Rectangle {
            id: imageRect
            anchors.fill: parent
            visible: false
            radius: 8 - root.borderThickness
            layer.enabled: true
            layer.smooth: true
        }
        NblImage {
            id: imageSource
            anchors.fill: parent
            source: root.icon 
            layer.enabled: true
            layer.smooth: true
            layer.effect: MultiEffect {
                maskEnabled: true
                maskSource: imageRect
                saturation: root.unlocked 
                    ? 0.0 
                    : Theme.colors.tintUi ? -0.5 : -1.0
                colorization: root.unlocked || !Theme.colors.tintUi ? 0.0 : 1.0
                colorizationColor: Theme.colors.accent
                maskThresholdMin: 0.5
                maskSpreadAtMin: 1.0
            }
        }
    }

    // Hidden Text
    Label {
        anchors.fill: parent
        visible: root.hidden
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        font.variableAxes: { "wght": 700 }
        font.pixelSize: 32
        text: "?"
        color: Theme.colors.textDim
    }

    // Sparkles for Rare Border
    Loader {
        active: rareBorder.visible
        visible: rareBorder.visible
        sourceComponent: Item {

            width: root.width - (root.borderThickness * 2)
            height: root.height - (root.borderThickness * 2)
            x: root.borderThickness
            y: root.borderThickness

            Repeater {
                model: 4
                delegate: AchievementSparkle {
                    required property int modelData
                    color: root.trueBorderColor.lighter(1.5)
                    particleSize: root.borderThickness * 8
                    x: (modelData % 2) * parent.width
                    y: Math.floor(modelData / 2) * parent.height
                }
            }
        }
    }

    NblToolTip {
        delay: root.tooltipDelay
        text: root.title
        visible: mouseArea.containsMouse
    }

    MouseArea {
        id: mouseArea
        anchors.fill: root
        onClicked: { root.clicked() }
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: root.enabled
    }
}

