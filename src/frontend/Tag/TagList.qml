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

Item {

    id: root
    implicitHeight: column.height

    enum TagState {
        INITIAL,
        ADDED,
        REMOVED,
        IGNORED
    }

    property string label
    property int labelIndent: 12
    property list<var> tags: []
    property list<var> initialTags: []

    property string tooltip
    property int tooltipDelay: 1000

    property bool allowNewTags: true

    onInitialTagsChanged: {
        tags = []
        for(let i = 0 ; i < initialTags.length ; i++) {
            tags[i] = {
                "index": i,
                "id": initialTags[i].id,
                "name": initialTags[i].name,
                "color": initialTags[i].color,
                "state": TagList.TagState.INITIAL
            }
        }
    }

    function getTags(state) {
        let out = []
        for(let i = 0 ; i < tags.length ; i++) {
            if(tags[i].state == state) {
                out.push(tags[i].id)
            }
        }
        return out
    }

    function getListedTags() {
        return [...getInitialTags(), ...getAddedTags()]
    }

    function getInitialTags() {
        return getTags(TagList.TagState.INITIAL)
    }
    
    function getAddedTags() {
        return getTags(TagList.TagState.ADDED)
    }

    function getRemovedTags() {
        return getTags(TagList.TagState.REMOVED)
    }

    function resetTagStates() {
        for(let i = 0 ; i < tags.length ; i++) {
            tags[i].state = TagList.TagState.INITIAL
        }
    }

    function addTag(id, name, color) {

        root.modified()

        tagInput.text = ""

        for(let i = 0 ; i < root.tags.length ; i++) {
            if(root.tags[i].name == name) {
                if(root.tags[i].state != TagList.TagState.INITIAL) {
                    root.tags[i].state = TagList.TagState.ADDED
                }
                return
            }
        }
        
        let index = root.tags.length

        root.tags.push({
            id: id,
            index: index,
            name: name,
            color: color,
            state: TagList.TagState.ADDED
        })
    }


    signal modified

    Column {

        Item {
            width: parent.width
            height: 30

            Rectangle {
                width: parent.width
                height: 2
                y: (parent.height / 2) - 1
                color: Theme.colors.border
            }

            Rectangle {
                id: labelBg
                width: labelLabel.width + 12
                height: parent.height
                x: root.labelIndent
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
                id: labelLabel
                anchors.centerIn: labelBg
                font.family: Theme.font
                font.pixelSize: 16
                text: root.label
                color: Theme.colors.textDim
            }
        }


        id: column

        width: parent.width
        spacing: 8

        Flow {

            id: tagList
            width: parent.width
            spacing: 4

            Label {
                height: 24
                visible: root.tags.length == (root.getRemovedTags().length + root.getTags(TagList.TagState.IGNORED).length)

                color: Theme.colors.text
                font.family: Theme.font
                font.pixelSize: 16
                text: "No tags"
            }

            Repeater {
                model: root.tags
                delegate: Item {

                    id: existingTag

                    width: tag.width
                    height: tag.height

                    required property var modelData
                    visible: modelData.state != TagList.TagState.REMOVED && modelData.state != TagList.TagState.IGNORED

                    Tag {
                        id: tag
                        enabled: false
                        text: existingTag.modelData.name
                        color: existingTag.modelData.color
                    }
                    
                    MouseArea {
                        id: tagListArea
                        anchors.fill: parent
                        hoverEnabled: true
                    }

                    Rectangle {

                        visible: tagListArea.containsMouse

                        height: parent.height - 4
                        width: height
                        radius: height / 2

                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 2

                        color: Theme.colors.layer0

                        Text {
                            anchors.fill: parent
                            verticalAlignment: Qt.AlignVCenter
                            horizontalAlignment: Qt.AlignHCenter
                            color: Theme.colors.text
                            text: ""
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.modified()
                                root.tags[existingTag.modelData.index].state = 
                                    existingTag.modelData.state == TagList.ADDED ?
                                        TagList.TagState.IGNORED :
                                        TagList.TagState.REMOVED
                            }
                        }
                    }
                }
            }
        }

        NblTextBox {
            id: tagInput
            width: parent.width

            Keys.onReturnPressed: event => {
                // Add tag
                if(tagSearchModel.exactMatch) {
                    popup.addTop()
                } else if(root.allowNewTags) {
                    popup.addNew()
                }
                event.accepted = true
            }

            onTextEdited: {
                text = text.toLowerCase().replace(' ', '_')
                if(text.length > 0) popup.open()
            }

            onPressed: {
                popup.open()
            }
        }
    }

    Popup {

        id: popup

        visible: false
        width: parent.width
        height: 128

        y: tagInput.y + tagInput.height
        z: 1000
        modal: false
        clip: true

        contentHeight: tagResults.height

        background: null
        margins: 0
        padding: 0

        onOpened: {
            tagSearchModel.paused = false
        }

        function addTop() {
            let id = tagSearchModel.id(0);
            let name = tagSearchModel.name(0);
            let color = tagSearchModel.color(0);

            root.addTag(id, name, color)
        }

        function addNew() {

            root.modified()

            let name = tagInput.text
            tagInput.text = ""

            for(let i = 0 ; i < root.tags.length ; i++) {
                if(root.tags[i].name == name) {
                    if(root.tags[i].state != TagList.TagState.INITIAL) {
                        root.tags[i].state = TagList.TagState.ADDED
                    }
                    return
                }
            }

            let color = Theme.colors.tagColor(name)
            let index = root.tags.length
            let id = ApplicationContext.addTag(name, color)
            if(id == 0) {
                ApplicationContext.pushNotification({
                    type: "error",
                    title: "Failed to add tag",
                    message: "An error occurred while adding a tag!"
                })
            }

            root.tags.push({
                index: index,
                name: name,
                color: color,
                state: TagList.TagState.ADDED
            })
        }

        contentItem: Item {
            id: tagResults

            Rectangle {
                width: parent.width
                height: Math.min(128, resultsColumn.height + 8)
                color: Theme.colors.layer1
                border.width: 2
                border.color: Theme.colors.border
                radius: 8
            }

            ScrollView {

                width: parent.width
                height: parent.height - 4
                y: 2

                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                contentHeight: resultsColumn.height

                Flickable {

                    anchors.fill: parent
                    interactive: true
                    clip: true

                    onAtYEndChanged: {
                        if(atYEnd) {
                            tagSearchModel.loadNextPage()
                        }
                    }

                    Column {

                        id: resultsColumn
                        width: parent.width - 4
                        x: 2

                        Item {

                            visible: root.allowNewTags && tagInput.text.length > 0 && !tagSearchModel.exactMatch

                            width: parent.width
                            height: 32

                            Rectangle {
                                anchors.fill: parent
                                color: newTagMouseArea.containsMouse ? Theme.colors.border : "transparent"
                                radius: 2
                            }

                            Label {
                                width: parent.width - 32
                                x: 16
                                anchors.verticalCenter: parent.verticalCenter
                                text: "+ New Tag"
                                font.pixelSize: 16
                                font.variableAxes: { "wght": 600 }
                                font.family: Theme.font
                                color: Theme.colors.text
                            }

                            MouseArea {
                                id: newTagMouseArea
                                anchors.fill: parent
                                propagateComposedEvents: false
                                cursorShape: Qt.PointingHandCursor
                                hoverEnabled: true
                                onClicked: {
                                    popup.addNew()
                                }
                            }
                        }

                        Repeater {

                            model: TagSearchModel {
                                id: tagSearchModel
                                searchQuery: tagInput.text
                                pageSize: 5
                            }

                            Item {

                                required property var modelData

                                property int tagId: modelData.id
                                property string tagName: modelData.name
                                property string tagColor: modelData.color

                                width: resultsColumn.width
                                height: 32

                                Rectangle {
                                    anchors.fill: parent
                                    color: tagArea.containsMouse ? Theme.colors.border : "transparent"
                                    radius: 4
                                }

                                Label {
                                    width: parent.width - 32
                                    x: 16
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: parent.modelData.name
                                    font.pixelSize: 16
                                    font.family: Theme.font
                                    color: Theme.colors.text
                                }

                                MouseArea {
                                    id: tagArea
                                    anchors.fill: parent
                                    propagateComposedEvents: false
                                    cursorShape: Qt.PointingHandCursor
                                    hoverEnabled: true
                                    onClicked: {
                                        root.addTag(parent.tagId, parent.tagName, parent.tagColor)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
