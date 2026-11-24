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

SettingsTab {

    id: root

    Component.onCompleted: {
        searchModel.paused = false
    }

    onRefreshed: {
        searchModel.refresh()
        flickable.contentY = 0
    }

    onApplied: {
    
        let res = ApplicationContext.updateTags(encodeModifiedTags())
        if(res != 0) {
            ApplicationContext.pushNotification({
                type: "error",
                title: "Failed to Update Tags",
                message: "An error occurred. This is usually due to cycles in a tag's ancestry."
            })
            console.log("An error occurred while updating tags!", res)
        }

        let removedTags = getRemovedTags()
        for(let i = 0 ; i < removedTags.length ; i++) {
            let res = ApplicationContext.removeTag(removedTags[i])
            if(res != 0) {
                ApplicationContext.pushNotification({
                    type: "error",
                    title: "Failed to Remove a Tag",
                    message: "An error occurred while removing the tag: \"" + removedTags[i] + "\""
                })
                console.log("An error occurred while removing a tag!", res) 
            }
        }

        refresh()
    }

    RowLayout {

        width: parent.width
        height: 54

        NblTextBox {
            id: filterBox
            Layout.fillWidth: true
            label: "Filter"
        }

        NblButton {

            id: newBtn

            icon: ""
            text: "New"

            Layout.preferredWidth: 128
            Layout.preferredHeight: 48
            Layout.topMargin: 6

            inactiveColor: Theme.colors.layer1
            centered: true

            onClicked: {
                if(root.modified) {
                    ApplicationContext.pushNotification({
                        type: "error",
                        title: "Cannot Add Tag",
                        message: "Please apply your changes first."
                    })
                    return
                }

                newPopup.open()
            }
        }
    }

    Popup {
        id: newPopup

        width: 350
        height: 316

        onOpened: {
            let realCoords = root.mapToItem(null, 0, 0)
            x = Math.min(newBtn.x, root.Window.width - realCoords.x - 350)
            y = Math.min(newBtn.y + newBtn.height + 8, root.Window.height - realCoords.y - 316)
        }

        padding: 8

        background: Rectangle {
            color: Theme.colors.layer0
            radius: 8
            border.width: 2
            border.color: Theme.colors.border
        }

        contentItem: Column {
            id: newTagInfo
            width: parent.width
            spacing: 8
            NblTextBox {
                id: newTagNameBox
                label: "Name"
                width: parent.width

                onTextEdited: {
                    text = text.toLowerCase().replace(' ', '_')
                }

            }

            NblColorSwatch {
                id: newTagColorBox
                label: "Color"
                width: 96
                height: 48
                selectedColor: Qt.hsva(Math.random(), 0.6 + Math.random() * 0.3, 0.4 + Math.random() * 0.5)
            }

            TagList {
                id: newParentsBox
                label: "Parents"
                width: parent.width
                allowNewTags: false
            }

            Item {

                width: 128
                height: 48

                Rectangle {
                    anchors.fill: parent
                    color: Theme.colors.accent
                    radius: 8
                }

                Label {
                    anchors.centerIn: parent
                    font.pixelSize: 16
                    color: Theme.colors.textOnAccent
                    text: "Add"
                }

                MouseArea {
                    id: addMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if(newTagNameBox.text.length == 0) {
                            ApplicationContext.pushNotification({
                                type: "error",
                                title: "Cannot Add Tag",
                                message: "Name field must not be empty!"
                            })
                            return
                        }
                        let tagId = ApplicationContext.addTag(newTagNameBox.text, newTagColorBox.selectedColor)

                        let parents = newParentsBox.getAddedTags()
                        for(let i = 0 ; i < parents.length ; i++) {
                            ApplicationContext.addTagParent(tagId, parents[i])
                        }

                        root.refresh()
                        newPopup.close()
                    }
                }
            }
        }
    }


    function encodeModifiedTags() {

        let output = []

        for(let i = 0 ; i < tagRepeater.count ; i++) {
            let tagElement = tagRepeater.itemAt(i)
            if(tagElement.modified && !tagElement.removed) {
                output.push(tagElement.encodeTagInfo())
            }
        }

        return output
    }

    function getRemovedTags() {
        let output = []
        for(let i = 0 ; i < tagRepeater.count ; i++) {
            let tagElement = tagRepeater.itemAt(i)
            if(tagElement.removed) {
                output.push(tagElement.getId())
            }
        }
        return output
    }

    TagSearchModel {
        id: searchModel
        searchQuery: filterBox.text
        pageSize: 30
    }

    NblScrollView {

        width: root.width + 8
        height: parent.height - 64
        y: 64

        id: flickable

        contentWidth: root.width
        contentHeight: tagColumn.height
        clip: true

        onAtYEndChanged: {
            if(atYEnd) {
                searchModel.loadNextPage()
            }
        }

        Column {

            id: tagColumn

            property NblDropdown currentlyOpenDropdown: null

            Repeater {

                id: tagRepeater
                model: searchModel

                delegate: Item { 

                    width: root.width
                    height: dropdown.height

                    id: tagElement
                    
                    required property var modelData
                    property bool modified: false
                    property bool removed: false

                    onModifiedChanged: {
                        if(modified) root.modified = true
                    }

                    function getId() {
                        return modelData.id
                    }

                    function encodeTagInfo() {
                        return {
                            id: tagElement.modelData.id,
                            originalName: tagElement.modelData.name,
                            name: tagNameBox.text,
                            color: tagColorBox.selectedColor,
                            addedParents: parentsBox.getAddedTags(),
                            removedParents: parentsBox.getRemovedTags()
                        }
                    }

                    NblDropdown {

                        id: dropdown

                        width: parent.width - collapsedHeight
                        label: tagElement.modelData.name
                        icon: ""
                        iconColor: tagColorBox.selectedColor
                        iconSize: 20

                        contentHeight: tagInfo.height
                        enabled: !tagElement.removed
                        bold: tagElement.modified

                        property bool loadedTags: false
                        onCollapsedChanged: {
                            if(!collapsed) {

                                if(tagColumn.currentlyOpenDropdown && tagColumn.currentlyOpenDropdown != this) {
                                    tagColumn.currentlyOpenDropdown.collapsed = true
                                }
                                tagColumn.currentlyOpenDropdown = this

                                if(!loadedTags) {

                                    loadedTags = true
                                    parentsBox.initialTags = ApplicationContext.getTagParents(tagElement.modelData.id)
                                }
                            }
                        }

                        Column {
                            id: tagInfo
                            width: dropdown.contentWidth
                            NblTextBox {
                                id: tagNameBox
                                label: "Name"
                                width: parent.width
                                text: tagElement.modelData.name
                                onTextEdited: {
                                    text = text.toLowerCase().replace(' ', '_')
                                    tagElement.modified = true
                                }
                            }

                            NblColorSwatch {
                                id: tagColorBox
                                label: "Color"
                                width: 96
                                height: 48
                                selectedColor: tagElement.modelData.color
                                onSelectedColorChanged: {
                                    if(selectedColor != tagElement.modelData.color) {
                                        tagElement.modified = true
                                    }
                                }
                            }

                            TagList {
                                id: parentsBox
                                label: "Parents"
                                width: parent.width
                                allowNewTags: false
                                onModified: {
                                    tagElement.modified = true
                                }
                            }
                        }
                    }

                    Item {
                        width: dropdown.collapsedHeight
                        height: width

                        x: tagElement.width - width

                        Rectangle {
                            anchors.fill: parent
                            color: removeArea.containsMouse ? Theme.colors.layer1 : Theme.colors.layer0
                            radius: 8
                        }

                        Label {
                            text: tagElement.removed ? "" : ""
                            font.family: Theme.nerdFont
                            font.pixelSize: 20
                            anchors.centerIn: parent
                            color: Theme.colors.text
                        }

                        MouseArea {
                            id: removeArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                tagElement.removed = !tagElement.removed
                            }
                        }
                    }

                    Rectangle {
                        width: dropdown.width
                        height: 8
                        anchors.verticalCenter: parent.verticalCenter
                        color: Theme.colors.layer0
                        visible: tagElement.removed
                    }

                    Rectangle {
                        width: dropdown.width
                        height: 2
                        anchors.verticalCenter: parent.verticalCenter
                        color: Theme.colors.textDim
                        visible: tagElement.removed
                    }

                }
            }
        }
    }
}

