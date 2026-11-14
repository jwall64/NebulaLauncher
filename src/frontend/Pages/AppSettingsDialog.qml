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

import Nebula.Controls

NblDialog {

    id: root

    padding: 0
    navigationPadding: 16

    targetWidth: 960

    showCancelButton: (tabLoader.item as SettingsTab).showCancelButton
    showApplyButton: (tabLoader.item as SettingsTab).showApplyButton

    Item {

        anchors.fill: parent

        property int sidebarWidth: 200

        Item {
            id: sidebar
            width: parent.sidebarWidth
            height: parent.height + root.navigationHeight + root.navigationGap

            property int currentTab: 0

            Rectangle { 
                width: 2
                height: parent.height - 32
                x: parent.width
                y: 16
                color: Theme.colors.border
            }

            Flickable {

                anchors.fill: parent
                anchors.topMargin: 16

                id: tabFlickable

                contentWidth: width
                contentHeight: tabRepeater.height
                flickableDirection: Flickable.VerticalFlick
                clip: true

                ScrollBar.horizontal: null
                ScrollBar.vertical: ScrollBar {
                    parent: tabFlickable
                    anchors.top: tabFlickable.top
                    anchors.bottom: tabFlickable.bottom
                    anchors.right: tabFlickable.right
                    active: tabFlickable.movingVertically
                }

                Column {

                    id: tabRepeater
                    width: parent.width

                    Repeater {
                        model: [
                            { icon: "", name: "Theme", tab: "ThemeSettingsTab.qml" },
                            { icon: "", name: "Tags", tab: "../Tag/TagSettingsTab.qml" },
                            { icon: "", name: "Online Accounts", tab: "../Account/AccountSettingsTab.qml" },
                            { icon: "", name: "Plugins", tab: "../Plugin/PluginSettingsTab.qml" },
                            { icon: "\uf0ad", name: "Maintenance", tab: "MaintenanceSettingsTab.qml" }
                        ]

                        delegate: Item {

                            required property var modelData
                            required property int index

                            readonly property bool isActive: sidebar.currentTab == index
                            readonly property bool isHighlighted: isActive || tabArea.containsMouse

                            width: tabRepeater.width
                            height: 48

                            Rectangle {
                                anchors.fill: parent
                                color: parent.isActive
                                        ? Theme.colors.border
                                        : parent.isHighlighted ? Theme.colors.layer1 : "transparent"
                            }

                            Label {
                                anchors.verticalCenter: parent.verticalCenter
                                x: 24
                                text: parent.modelData.icon
                                font.family: Theme.nerdFont
                                font.pixelSize: 24
                                color: parent.isHighlighted ? Theme.colors.text : Theme.colors.textDim
                            }

                            Label {
                                anchors.verticalCenter: parent.verticalCenter
                                x: 64
                                text: parent.modelData.name
                                font.family: Theme.font
                                font.pixelSize: 16
                                color: parent.isHighlighted ? Theme.colors.text : Theme.colors.textDim
                            }

                            MouseArea {
                                id: tabArea
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                anchors.fill: parent
                                onClicked: {
                                    if(parent.index == sidebar.currentTab) return

                                    let currentTab = tabLoader.item as SettingsTab
                                    if(currentTab.modified) {
                                        ApplicationContext.pushNotification({
                                            type: "error",
                                            title: "Cannot Change Tabs",
                                            message: "Please apply your changes first."
                                        })
                                        return
                                    }

                                    currentTab.modified = false
                                    currentTab.visible = false

                                    sidebar.currentTab = parent.index
                                    tabLoader.source = parent.modelData.tab
                                }
                            }
                        }
                    }
                }
            }
        }
        
        Item {

            width: parent.width - parent.sidebarWidth - 48
            height: parent.height - 48
            x: parent.sidebarWidth + 24
            y: 24

            Loader {
                id: tabLoader
                anchors.fill: parent
                source: "ThemeSettingsTab.qml"
                active: true
            }
        }

    }

    onApply: {
        if(tabLoader.item) {
            (tabLoader.item as SettingsTab).apply()
        }
    }

}
