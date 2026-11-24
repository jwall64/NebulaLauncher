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

import Nebula

Item {

    id: root

    property string label
    property bool allowReorder: false
    property int minItems: 0
    property int maxItems: 999

    property list<var> initialItems
    property ListModel items: ListModel {}

    property int controlsOffset: 0

    readonly property int count: items.count
    readonly property int controlsWidth: allowReorder ? 64 : 48

    readonly property int itemWidth: width - root.controlsWidth - 8

    required property var createNew
    required property Component delegate

    function itemAt(i) {
        return itemRepeater.itemAt(i)
    }

    function wasAdded(i) {
        return items[i].added
    }

    signal itemPopped(int index)
    signal itemMoved(int oldIndex, int currentIndex)
    signal itemAdded()


    implicitHeight: contentColumn.height

    onInitialItemsChanged: {

        items.clear()
        for(let i = 0 ; i < initialItems.length ; i++) {
            items.append({ modelData: initialItems[i], added: false })
        }
    }

    Column {
        id: contentColumn
        width: parent.width

        spacing: 8

        Item {
            width: parent.width
            height: 24

            Rectangle {
                width: parent.width
                height: 2
                y: (parent.height / 2) - 1
                color: Theme.colors.border
            }

            Rectangle {
                id: labelBg
                visible: root.label.length > 0
                width: labelLabel.width + 12
                height: parent.height
                x: 12
                color: Theme.colors.layer0
            }

            Label {
                id: labelLabel
                anchors.centerIn: labelBg
                font.family: Theme.font
                font.pixelSize: 16
                text: root.label
                color: Theme.colors.textDim
            }
        }

        Item {
            width: parent.width
            height: itemsColumn.height

            Column {
                id: itemsColumn
                width: root.itemWidth
                spacing: 8

                Repeater {
                    id: itemRepeater
                    model: root.items
                    delegate: root.delegate

                    function getItemHeight(i, def) {

                        if(i >= count) return def
                        return itemAt(i).height
                    }
                }
            }

            Item {
                width: root.controlsWidth
                x: parent.width - width
                y: root.controlsOffset
                Column {
                    width: parent.width
                    spacing: 8
                    Repeater {
                        model: root.count

                        Item {

                            id: itemControls
                            required property int modelData
                            readonly property int index: modelData

                            width: root.controlsWidth
                            height: itemRepeater.getItemHeight(index, 96)

                            RowLayout {

                                spacing: 0

                                width: parent.width
                                height: Math.min(48, parent.height)

                                Item {

                                    Layout.fillHeight: true
                                    Layout.fillWidth: true
                                    visible: root.allowReorder

                                    ColumnLayout {
                                        anchors.fill: parent
                                        spacing: 0

                                        Item {
                                            Layout.fillHeight: true
                                            Layout.fillWidth: true
                                            Rectangle { 
                                                id: upBg
                                                anchors.fill: parent;
                                                color: upArea.containsMouse ? Theme.colors.layer1 : Theme.colors.layer0
                                                radius: 8
                                            }
                                            Label {
                                                anchors.centerIn: upBg
                                                text: ""
                                                font.family: Theme.nerdFont
                                                font.pixelSize: 20
                                                color: itemControls.index == 0 ? Theme.colors.textDim : Theme.colors.text
                                            }

                                            MouseArea {
                                                id: upArea
                                                visible: itemControls.index > 0
                                                anchors.fill: upBg
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    let index = itemControls.index
                                                    if(index == 0) return

                                                    root.items.move(index, index - 1, 1)
                                                    root.itemMoved(index, index - 1)
                                                }
                                            }
                                        }
                                        Item {
                                            Layout.fillHeight: true
                                            Layout.fillWidth: true
                                            Rectangle {
                                                id: downBg
                                                anchors.fill: parent
                                                color: downArea.containsMouse ? Theme.colors.layer1 : Theme.colors.layer0
                                                radius: 8
                                            }

                                            Label {
                                                anchors.centerIn: downBg
                                                text: ""
                                                font.family: Theme.nerdFont
                                                font.pixelSize: 20
                                                color: (itemControls.index + 1) == root.count ? Theme.colors.textDim : Theme.colors.text
                                            }

                                            MouseArea {
                                                id: downArea
                                                visible: (itemControls.index + 1) < root.count
                                                anchors.fill: downBg
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    let index = itemControls.index
                                                    if((index + 1) == root.count) return

                                                    root.items.move(index, index + 1, 1)
                                                    root.itemMoved(index, index + 1)
                                                }
                                            }
                                        }
                                    }
                                }
                                Item {

                                    enabled: root.count > root.minItems

                                    Layout.fillHeight: true
                                    Layout.fillWidth: true
                                    Rectangle {
                                        id: removeBg
                                        anchors.fill: parent
                                        color: removeArea.containsMouse ? Theme.colors.layer1 : Theme.colors.layer0
                                        radius: 8
                                    }

                                    Label {
                                        anchors.centerIn: removeBg
                                        text: ""
                                        font.family: Theme.nerdFont
                                        font.pixelSize: 24
                                        color: parent.enabled ? Theme.colors.text : Theme.colors.textDim
                                    }

                                    MouseArea {
                                        id: removeArea
                                        anchors.fill: removeBg
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            root.itemPopped(itemControls.index)
                                            root.items.remove(itemControls.index, 1)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Item {

            visible: root.items.count < root.maxItems 

            width: parent.width
            height: 48

            Rectangle {
                anchors.fill: parent
                color: addArea.containsMouse ? Theme.colors.layer1 : Theme.colors.layer0
                radius: 8
            }

            Label {
                anchors.fill: parent
                anchors.leftMargin: 16
                verticalAlignment: Qt.AlignVCenter
                font.family: Theme.font
                font.pixelSize: 16
                font.variableAxes: { "wght": 600 }
                text: "+ New"
                color: Theme.colors.text
            }

            MouseArea {
                id: addArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.items.append({ modelData: root.createNew(root.items.count), added: true })
                    root.itemAdded()
                }
            }
        }
    }

}
