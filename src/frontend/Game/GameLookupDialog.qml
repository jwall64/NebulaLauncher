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
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Effects

import Nebula

NblDialog {

    id: root

    showApplyButton: false
    showOkButton: false

    signal gameAdded(var id)

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Item {

            id: searchRow
            property bool modified: false

            Layout.fillWidth: true
            Layout.preferredHeight: 64

            RowLayout {
                anchors.fill: parent
                NblTextBox {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    id: queryBox
                    label: "Query"
                    placeholderText: "Search for a game by title"
                    onTextEdited: {
                        searchRow.modified = true
                    }
                    Component.onCompleted: {
                        forceActiveFocus()
                    }
                    Keys.onReturnPressed: {
                        if(!searchRow.modified) {
                            return
                        }
                        searchRow.modified = false
                        searchHelper.lookup(queryBox.text)
                        resultsModel.clear()
                    }
                }
                NblButton {
                    Layout.preferredWidth: 128
                    Layout.fillHeight: true
                    Layout.topMargin: queryBox.backgroundOffset
                    enabled: searchRow.modified && queryBox.text.length > 0
                    icon: ""
                    text: "Search"
                    activeColor: Theme.colors.accent
                    inactiveColor: enabled ? Theme.colors.accent : Theme.colors.layer1
                    textColor: enabled ? Theme.colors.textOnAccent : Theme.colors.textDim
                    centered: true
                    onClicked: {
                        if(!searchRow.modified) {
                            return
                        }
                        searchRow.modified = false
                        searchHelper.lookup(queryBox.text)
                        resultsModel.clear()
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            NblScrollView {

                width: parent.width + 12
                height: parent.height

                contentWidth: width - 12
                contentHeight: resultsColumn.height
                clip: true

                Column {

                    id: resultsColumn
                    width: parent.width
                    spacing: 8

                    OnlineGameLookupHelper {
                        id: searchHelper
                        onReceivedResult: (data) => {
                            resultsModel.append({meta: data})
                        }
                    }

                    Repeater {
                        model: ListModel {
                            id: resultsModel
                        }
                        delegate: Item {

                            id: resultItem

                            required property int index
                            required property var meta

                            width: resultsColumn.width
                            height: 128

                            Rectangle {
                                anchors.fill: parent
                                color: resultArea.containsMouse ? Theme.colors.layer1 : "transparent"
                                radius: 8
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 16
                                Item {
                                    Layout.fillHeight: true
                                    Layout.preferredWidth: height * 2/3

                                    NblImage {
                                        id: posterImage
                                        anchors.fill: parent
                                        source: resultItem.meta.posterUrl
                                        layer.enabled: true
                                        visible: false
                                    }
                                    Rectangle {
                                        id: maskShape
                                        radius: 8
                                        anchors.fill: parent
                                        layer.enabled: true
                                        visible: false
                                    }
                                    MultiEffect {
                                        anchors.fill: parent
                                        source: posterImage
                                        maskEnabled: true
                                        maskSource: maskShape
                                    }

                                }
                                Item {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true

                                    ColumnLayout {
                                        anchors.fill: parent
                                        spacing: 4
                                        Item {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 24
                                            RowLayout {
                                                anchors.fill: parent
                                                spacing: 4
                                                Label {
                                                    Layout.preferredWidth: implicitWidth
                                                    Layout.maximumWidth: parent.width - dateLabel.implicitWidth - parent.spacing
                                                    text: resultItem.meta.title
                                                    color: Theme.colors.text
                                                    font.family: Theme.font
                                                    elide: Text.ElideRight
                                                }
                                                Label {
                                                    id: dateLabel
                                                    text: "(" + new Date(resultItem.meta.releaseDate).getFullYear() + ")"
                                                    color: Theme.colors.textDim
                                                    font.family: Theme.font
                                                }
                                                Item {
                                                    Layout.fillWidth: true
                                                }
                                            }
                                        }
                                        Item {
                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                            Label { 
                                                anchors.fill: parent
                                                text: resultItem.meta.description
                                                elide: Text.ElideRight
                                                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                                                font.family: Theme.font
                                                color: Theme.colors.textDim
                                            }
                                        }
                                        Item {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 24
                                            Row {
                                                id: tagRow

                                                anchors.fill: parent
                                                spacing: 4
                                                Repeater {

                                                    id: tagRepeater

                                                    model: resultItem.meta.tags
                                                    delegate: Tag {
                                                        required property string modelData
                                                        required property int index

                                                        property int tagId: 0

                                                        height: tagRow.height

                                                        text: modelData
                                                        color: "white"
                                                        enabled: false

                                                        Component.onCompleted: {
                                                            tagId = ApplicationContext.findTagId(text)
                                                            if(tagId == 0) {
                                                                color = Theme.colors.tagColor(text)
                                                            } else {
                                                                let tagMeta = ApplicationContext.getTagsFromIds([tagId])
                                                                color = tagMeta[0].color
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            MouseArea {
                                id: resultArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor

                                onClicked: {
                                    let params = {}
                                    params["banner"] = resultItem.meta["bannerUrl"]
                                    params["poster"] = resultItem.meta["posterUrl"]
                                    params["icon"] = resultItem.meta["iconUrl"]
                                    params["logo"] = resultItem.meta["logoUrl"]
                                    params["logoPosition"] = resultItem.meta["logoPosition"]
                                    params["title"] = resultItem.meta["title"]
                                    params["description"] = resultItem.meta["description"]
                                    params["releaseDate"] = resultItem.meta["releaseDate"]
                                    params["rating"] = resultItem.meta["rating"]
                                    params["externalIds"] = resultItem.meta["externalIds"]

                                    let tags = []

                                    for(let i = 0 ; i < tagRepeater.count ; i++) {
                                        let tagItem = tagRepeater.itemAt(i) as Tag

                                        let tagId = tagItem.tagId
                                        if(tagId == 0) {
                                            tagId = ApplicationContext.addTag(tagItem.text, tagItem.color)
                                            if(tagId == 0) {
                                                ApplicationContext.pushNotification({
                                                    type: "error",
                                                    title: "Failed to add tag",
                                                    message: "An error occurred while adding a tag!"
                                                })
                                                continue
                                            }
                                        }

                                        tags.push(tagId)
                                    }
                                    params["tags"] = tags

                                    let id = ApplicationContext.addGame(params)
                                    if(id < 0) {
                                        ApplicationContext.pushNotification({
                                            type: "error",
                                            title: "Failed to add a Game",
                                            message: "An error occurred while updating a game"
                                        })
                                    } else {
                                        root.gameAdded(id)
                                    }
                                    root.close()
                                }
                            }
                        }
                    }
                }
            }
        }
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 64

            NblButton {

                anchors.centerIn: parent

                text: "Add Game Manually"
                inactiveColor: Theme.colors.layer1
                centered: true
                leftPadding: 48
                rightPadding: 48

                onClicked: {
                    let dialog = DialogManager.create("Game/AddGameDialog.qml", {})

                    dialog.closed.connect(root.close)
                    dialog.gameAdded.connect(root.gameAdded)

                    root.visible = false
                    dialog.open()
                }
            }
        }
    }
}
