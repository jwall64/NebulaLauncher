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

import Nebula

Item {

    id: root

    property bool collapsed: true
    property int selectedTab: 0
    property bool showHidden: false

    signal tabClicked(int tabId)
    signal tabEdited(int tabId)
    signal gameAdded(int gameId)

    implicitWidth: collapsed ? 64 : 240

    Component.onCompleted: {
        if(tabRepeater.count > 0) {
            root.tabClicked((tabRepeater.itemAt(0) as TabButton).tabId)
        }
    }

    function reloadTabs() {
        tabsModel.refresh()
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: Theme.colors.layer1
        border.width: 2
        border.color: Theme.colors.border
        radius: 8
    }

    ColumnLayout {
        width: parent.width - 16
        height: parent.height - 16
        anchors.centerIn: parent

        spacing: 8

        Item {

            Layout.fillWidth: true
            Layout.fillHeight: true

            NblScrollView {

                id: flickable
                width: parent.width + 8
                height: parent.height
                clip: true

                contentWidth: width - 8
                contentHeight: tabList.height

                onAtYEndChanged: {
                    if(atYEnd) {
                        tabsModel.loadNextPage()
                    }
                }

                Column {
                    id: tabList
                    width: flickable.contentWidth
                    spacing: 8

                    Repeater {

                        id: tabRepeater

                        model: TabModel { 
                            pageSize: 20
                            id: tabsModel
                        }

                        delegate: TabButton {
                            required property var modelData

                            tabId: modelData.id
                            width: tabList.width
                            visible: !modelData.hidden || root.showHidden

                            collapsed: root.collapsed
                            icon: modelData.icon
                            name: modelData.name
                            colorize: modelData.colorizeIcon
                            iconIsImage: modelData.iconIsImage

                            onClicked: {
                                root.selectedTab = tabId
                                root.tabClicked(tabId)
                            }
                            onEditRequested: {
                                let dialog = DialogManager.create("Tab/AddTabDialog.qml", { tabId: tabId }) as AddTabDialog
                                dialog.apply.connect(function() {
                                    root.reloadTabs()
                                    root.tabEdited(tabId)
                                })
                                dialog.open()
                            }
                            onDeleteRequested: {
                                DialogManager.openDeleteConfirmation("Are you sure you want to delete this tab?", function() {
                                    ApplicationContext.deleteTab(tabId)
                                    root.reloadTabs()
                                    root.tabEdited(tabId)
                                })
                            }
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            implicitHeight: root.collapsed ? 48 + 48 + 8 : 48

            FlexboxLayout {
                gap: 8
                width: parent.width
                height: parent.height
                direction: root.collapsed ? FlexboxLayout.Column : FlexboxLayout.Row

                NblSidebarButton {

                    Layout.preferredWidth: 48
                    collapsed: true
                    icon: ""
                    name: ""

                    onClicked: { 
                        let dialog = DialogManager.create("Pages/AppSettingsDialog.qml", { }) as NblDialog
                        dialog.open()
                    }
                }

                NblSidebarButton {

                    Layout.fillWidth: true

                    collapsed: root.collapsed
                    icon: ""
                    name: "Add"
                    alignment: Text.AlignHCenter

                    onClicked: { 
                        addMenu.open()
                    }

                    NblContextMenu {
                        id: addMenu
                        height: 144
                        contentItem: ColumnLayout {
                            spacing: 0
                            NblButton {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                icon: "󰊗"
                                text: "Add Game"
                                onClicked: {
                                    addMenu.close()
                                    let dialog = DialogManager.create("Game/GameLookupDialog.qml", {}) as GameLookupDialog
                                    dialog.gameAdded.connect(function(id) {
                                        root.gameAdded(id)
                                    })
                                    dialog.open()
                                }
                                radius: 2
                            }
                            NblButton {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                icon: "󰓩"
                                text: "Add Tab"
                                onClicked: {
                                    addMenu.close()
                                    let dialog = DialogManager.create("Tab/AddTabDialog.qml", {}) as NblDialog
                                    dialog.apply.connect(function() {
                                        root.reloadTabs()
                                    })
                                    dialog.open()
                                }
                                radius: 2
                            }
                            NblButton {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                icon: root.showHidden ? "" : ""
                                text: root.showHidden ? "Hide Hidden Tabs" : "Show Hidden Tabs"
                                onClicked: {
                                    addMenu.close()
                                    root.showHidden = !root.showHidden
                                }
                                radius: 2
                            }
                        }
                    }
                }
            }
        }
    }
}
