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
import QtQuick.Effects

import Nebula

ApplicationWindow {
    visible: true
    title: "Nebula"
    color: "black"

    id: root

    font.family: Theme.font
    font.pixelSize: 16
    font.weight: 500

    property alias currentPage: pageLoader.currentItem

    Item {

        id: content
        anchors.fill: parent

        Rectangle {
            anchors.fill: parent
            color: Theme.colors.layer0
        }

        ColumnLayout {
            id: columnLayout
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

            Header {
                id: header
                sidebar: sidebarItem
                Layout.fillWidth: true
                Layout.preferredHeight: 64

                onSearchRequested: (query) => {
                    root.openPage("Game/GameSearchPage.qml", {
                        query: query,
                    })
                }
                onRandomRequested: {
                    let gameId = ApplicationContext.getRandomGameId()
                    if(gameId == 0) {
                        ApplicationContext.pushNotification({
                            type: "error",
                            title: "Shuffle Failed",
                            message: "Do you have any installed games?"
                        })
                    }
                    root.openGameDetails(gameId)
                }
            }

            RowLayout {
                id: rowLayout
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                layoutDirection: Qt.RightToLeft

                StackView {
                    id: pageLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    popEnter: null
                    popExit: null
                    pushEnter: null
                    pushExit: null
                    replaceEnter: null
                    replaceExit: null
                }
                Sidebar {
                    id: sidebarItem
                    Layout.fillHeight: true

                    onTabClicked: (tabId) => {
                        root.openTab(tabId)
                    }
                    onTabEdited: (tabId) => {
                        let tabPage = pageLoader.currentItem as TabPage
                        if(tabPage && tabPage.tabId == tabId) {
                            tabPage.refresh()
                        } 
                    }
                    onTabDeleted: (tabId) => {
                        let tabPage = pageLoader.currentItem as TabPage
                        if(!tabPage || tabPage.tabId == tabId) {
                            let first = getFirstTab()
                            if(first != -1) root.openTab(first)
                        } 
                    }
                    onGameAdded: function(id) {
                        root.openGameDetails(id)
                    }
                }
            }
        }

        layer.enabled: true
        layer.effect: MultiEffect {
            blurEnabled: dialogParent.children.length > 0
            blur: 1.0
        }
    }

    Shortcut {
        sequence: "Ctrl+H"
        context: Qt.ApplicationShortcut
        onActivated: {
            sidebarItem.showHidden = !sidebarItem.showHidden
        }
    }
    Shortcut {
        sequence: "/"
        context: Qt.ApplicationShortcut
        onActivated: {
            header.focusSearchBar()
        }
    }

    Item {
        id: dialogParent
    }

    NblNotificationColumn {

        parent: Overlay.overlay
        z: 100

        id: notificationColumn
        width: 316
        x: root.width - width - 8
        y: root.height - height - 8
    }
    
    Connections {
        target: ApplicationContext
        function onNotification(details) {
            notificationColumn.push(details)
        }
    }

    Connections {
        target: pageLoader.currentItem
        ignoreUnknownSignals: true
        function onCloseRequested() {
            while (pageLoader.depth > 1) {
                pageLoader.pop()
            }

            let tabPage = pageLoader.currentItem as TabPage
            if(tabPage) {
                tabPage.refresh()
            }
        }
        function onInstallationClicked(installationId) {
            root.openInstallationDetails(installationId)
        }
        function onGameClicked(gameId) {
            root.openGameDetails(gameId)
        }
        function onTagClicked(tagId, tagName, tagColor) {
            root.openPage("Tag/GameWithTagPage.qml", {
                tag: tagId,
                name: tagName,
                color: tagColor,
            })
        }
    }

    Component.onCompleted: {

        DialogManager.dialogParent = dialogParent

        var settings = ApplicationContext.loadSettings()
        if(settings.theme) {
            if(settings.theme.active_color != null) {
                Theme.colors.activeColor = settings.theme.active_color
            }
            if(settings.theme.light_theme != null) {
                Theme.colors.lightMode = settings.theme.light_theme
            }
            if(settings.theme.tint_ui != null) {
                Theme.colors.tintUi = settings.theme.tint_ui
            }
        }
    }

    function openTab(id) {

        Theme.colors.accentOverride = null
        let initDepth = pageLoader.depth
        while(pageLoader.depth > 1) {
            pageLoader.pop()
        }

        if(initDepth > 1) {
            let page = pageLoader.currentItem as TabPage
            if(page && page.tabId == id) {
                page.refresh()
                return
            }
        }

        pageLoader.replace("Tab/TabPage.qml", {
            tabId: id
        })
    }

    function openPage(name, opts) {
        Theme.colors.accentOverride = null
        while(pageLoader.depth > 1) {
            pageLoader.pop()
        }
        pageLoader.replace(name, opts)
    }

    function openInstallationDetails(installationId) {

        Theme.colors.accentOverride = null
        while (pageLoader.depth > 1) {
            pageLoader.pop()
        }
        let iDetails = ApplicationContext.getInstallationDetails(installationId)
        let gDetails = ApplicationContext.getGameDetails(iDetails.gameId)

        pageLoader.push("Game/GamePage.qml", {
            details: gDetails,
            installationDetails: iDetails,
        })
    }
 
    function openGameDetails(gameId) {

        Theme.colors.accentOverride = null
        while (pageLoader.depth > 1) {
            pageLoader.pop()
        }
        let details = ApplicationContext.getGameDetails(gameId)

        pageLoader.push("Game/GamePage.qml", {
            details: details,
        })
    }

    function closeInstallationDetails() {
        Theme.colors.accentOverride = null
        while (pageLoader.depth > 1) {
            pageLoader.pop()
        }

        let tabPage = pageLoader.currentItem as TabPage
        if(tabPage) {
            tabPage.refresh()
        }
    }
}
