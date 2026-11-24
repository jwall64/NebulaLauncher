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
import QtQuick.Effects
import QtQuick.Dialogs

import Nebula

Item {

    id: root
    required property string label
    property real aspectRatio: 1.0
    property real scaleFactor: Math.min(width / aspectRatio, height - 6)
    property var fillMode: Image.PreserveAspectCrop

    property string tooltip
    property int tooltipDelay: 1000

    property string imageSource: ""

    Rectangle {

        id: background
        y: 6

        width: root.aspectRatio * root.scaleFactor
        height: root.scaleFactor 

        anchors.centerIn: parent

        border.width: 2
        border.color: Theme.colors.border
        color: "transparent"

        radius: 8

        NblToolTip {
            delay: root.tooltipDelay
            text: root.tooltip
            visible: mouseArea.containsMouse
        }

    }

    Label {
        text: ""
        font.pixelSize: 48
        font.family: Theme.nerdFont
        color: mouseArea.containsMouse ? Theme.colors.text : Theme.colors.textDim
        anchors.centerIn: parent

        visible: root.imageSource.length == 0
    }

    NblImage {
        id: loadedImage
        anchors.fill: background
        anchors.margins: 2
        source: root.imageSource
        fillMode: root.fillMode
        visible: false
        layer.enabled: true
    }

    Item {
        id: imageMask
        anchors.fill: background
        anchors.margins: 2
        visible: false
        layer.enabled: true
        Rectangle {
            anchors.fill: parent
            radius: 6
        }
    }

    MultiEffect {
        source: loadedImage
        anchors.fill: loadedImage
        maskSource: imageMask
        maskEnabled: true
        visible: root.imageSource.length != 0
    }

    MouseArea {
        id: mouseArea
        anchors.fill: background
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onClicked: {
            fileDialog.open()
        }
    }

    Item {

        visible: root.imageSource.length == 0

        width: 0
        height: 0
        x: background.x + 16
        y: -6

        Rectangle { 
            width: labelText.width + 12
            height: labelText.height - 8
            anchors.centerIn: labelText
            color: Theme.colors.layer0 
        }
        Label { 
            id: labelText
            text: root.label
            font.pixelSize: 16
            font.family: Theme.font
            color: mouseArea.containsMouse ? Theme.colors.text : Theme.colors.textDim
        }
    }

    FileDialog {
        id: fileDialog
        title: root.label + " Image"
        fileMode: FileDialog.OpenFile
        nameFilters: [ "Image files (*.png *.jpg *.jpeg)", "All files (*)" ]

        onAccepted: {
            root.imageSource = fileDialog.selectedFile
        }

        onRejected: {

        }

    }


}
