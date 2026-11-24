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
import QtQuick.Effects
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Window

import Nebula

Item {

    id: root

    required property string imageSource
    required property real aspectRatio

    property string title: ""
    property string filterInfo: ""
    property string filterInfoIcon: ""

    readonly property bool showTitle: title.length > 0
    readonly property bool showFilterInfo: filterInfo.length > 0

    property string fallbackSource
    property bool doScaling: true
    property var titleAlignment: Qt.AlignLeft

    signal clicked

    readonly property real scaleFactor: width / aspectRatio
    property bool highlighted

    implicitHeight: (width / aspectRatio) + (showTitle ? 24 : 0) + (showFilterInfo ? 24 : 0)

    ColumnLayout {

        width: root.aspectRatio * root.scaleFactor

        Item {

            Layout.fillWidth: true
            Layout.preferredHeight: root.scaleFactor
            Layout.alignment: Qt.AlignHCenter

            Item {

                id: highlightShape
                visible: root.highlighted

                width: parent.width + 12
                height: parent.height + 12
                x: -6
                y: -6

                Shape {
                    anchors.fill: parent        
                    preferredRendererType: Shape.CurveRenderer

                    ShapePath {
                        PathRectangle {
                            width: highlightShape.width
                            height: highlightShape.height
                            radius: 14
                        }

                        strokeColor: "transparent"
                        strokeWidth: -1 

                        fillGradient: ConicalGradient {

                            centerX: root.width / 2
                            centerY: root.height / 2
                            angle: 0
                            NumberAnimation on angle {
                                from: 0
                                to: 360
                                duration: 2000
                                loops: Animation.Infinite
                                running: true
                            }

                            GradientStop { position: 0.0; color: Theme.colors.accent.darker(1.3) }
                            GradientStop { position: 0.5; color: Theme.colors.accent.lighter(1.3) }
                            GradientStop { position: 1.0; color: Theme.colors.accent.darker(1.3) }
                        }
                    }
                }
                Rectangle {

                    anchors.fill: parent
                    anchors.margins: 4
                    radius: 10
                    color: Theme.colors.layer0
                }
            }

            NblImage {
                id: image
                anchors.fill: parent
                source: root.imageSource.length == 0 ? root.fallbackSource : root.imageSource
                visible: false
                layer.enabled: true

                property bool loadFailed: false

                onStatusChanged: {
                    if (status === Image.Error) {
                        if (!loadFailed) {
                            loadFailed = true
                            source = ""
                            source = root.fallbackSource
                        }
                    } else if (status === Image.Ready) {
                        loadFailed = false
                    }
                }
            }

            Item {
                id: maskShape
                anchors.fill: image
                visible: false
                layer.enabled: true
                Rectangle {
                    anchors.fill: parent
                    radius: 8
                }
            }

            MultiEffect {
                source: image
                anchors.fill: image
                maskEnabled: true
                maskSource: maskShape
                maskThresholdMin: 0.5
                maskSpreadAtMin: 1.0

                scale: root.doScaling && mouseArea.containsMouse ? 1.05 : 1.0
                Behavior on scale {
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.InOutQuad
                    }
                }
            }
        }

        Label {

            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 8

            visible: root.showTitle
            text: root.title
            elide: Text.ElideRight
            horizontalAlignment: root.titleAlignment

            font.family: Theme.font
            font.pixelSize: 16
            color: Theme.colors.text
        }

        RowLayout {

            Layout.fillWidth: true
            Layout.preferredHeight: 16
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.alignment: root.titleAlignment

            visible: root.showFilterInfo

            Label {
                text: root.filterInfoIcon
                visible: root.filterInfoIcon.length > 0
                elide: Text.ElideRight

                font.family: Theme.nerdFont
                font.pixelSize: 16
                color: Theme.colors.textDim
            }

            Label {
                text: root.filterInfo
                elide: Text.ElideRight

                font.family: Theme.font
                font.pixelSize: 16
                color: Theme.colors.textDim
            }
        }
    }


    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.clicked()
        }
    }
}
