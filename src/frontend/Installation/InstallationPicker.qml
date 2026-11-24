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
import QtQuick.Layouts

import Nebula

Item { 

    id: root

    property int gameId
    property InstallationDetails selectedInstallation: null
    readonly property int selectedInstallationId: selectedInstallation == null ? 0 : selectedInstallation.installationId

    Rectangle {
        anchors.fill: parent
        color: Theme.colors.layer1
        radius: 8
    }

    function refresh() {
        let id = selectedInstallationId
        installationsModel.refresh()
        if(id != 0) {
            selectedInstallation = ApplicationContext.getInstallationDetails(id)
        }
    }

    RowLayout {

        visible: root.selectedInstallation != null

        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Item {

            Layout.fillHeight: true
            Layout.preferredWidth: parent.height

            NblImage {
                id: imageSource
                anchors.fill: parent
                anchors.verticalCenter: parent.verticalCenter
                fillMode: Image.PreserveAspectFit
                layer.enabled: true
                visible: false
                source: root.selectedInstallation == null ? "" : root.selectedInstallation.resolvedIconUrl
            }

            Item {
                id: maskShape
                anchors.fill: imageSource
                visible: false
                layer.enabled: true
                Rectangle {
                    anchors.fill: parent
                    radius: 8
                }
            }

            MultiEffect {
                anchors.fill: imageSource
                source: imageSource
                maskEnabled: true
                maskSource: maskShape
                maskThresholdMin: 0.5
                maskSpreadAtMin: 1.0
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true

            visible: parent.width > parent.height * 2

            Label {
                text: root.selectedInstallation == null ? "" : root.selectedInstallation.resolvedTitle
                y: -(parent.height / 7)
                width: parent.width
                color: Theme.colors.text
                font.family: Theme.font
                font.pixelSize: parent.height / 2
                elide: Text.ElideRight
            }

            Label {
                text: "Last played: " + (root.selectedInstallation == null || root.selectedInstallation.lastPlayed == 0 ? "Never" : Theme.formatDate(root.selectedInstallation.lastPlayed))
                width: parent.width
                y: parent.height / 2
                color: Theme.colors.textDim
                font.family: Theme.font
                font.pixelSize: parent.height / 2.5
                elide: Text.ElideRight
            }
        }

        Item {

            visible: parent.width >= parent.height * 2

            Layout.fillHeight: true
            Layout.preferredWidth: parent.height

            Label {
                anchors.fill: parent
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignHCenter
                font.family: Theme.font
                font.pixelSize: 20
                text: ""
                color: Theme.colors.textDim
            }
        }
    }

    Popup {

        property GameInstallationsModel model: GameInstallationsModel {
            id: installationsModel
            gameId: root.gameId
            pageSize: 10
            onIsLoadingChanged: {
                if(!isLoading && root.selectedInstallation == null && count > 0) {
                    root.selectedInstallation = ApplicationContext.getInstallationDetails(getId(0))
                }
            }
        }

        id: dropdown

        width: parent.width
        height: Math.min(160, ((installationsModel.count + 1) * 48) + 16)

        y: parent.height + 8

        padding: 8
        background: Rectangle {
            color: Theme.colors.layer0
            border.width: 2
            border.color: Theme.colors.border
            radius: 8
        }

        contentItem: NblScrollView {

            id: scrollView

            clip: true

            contentWidth: width
            contentHeight: installationsColumn.height

            onAtYEndChanged: {
                if(atYEnd && !installationsModel.allLoaded && !installationsModel.isLoading) {
                    installationsModel.loadNextPage()
                }
            }

            Column {

                id: installationsColumn
                width: scrollView.contentWidth

                Repeater {
                    model: installationsModel
                    delegate: Item {

                        id: installationItem
                        width: installationsColumn.width
                        height: 48

                        required property var installationId
                        required property string icon
                        required property string resolvedTitle
                        required property var lastPlayed

                        Rectangle {
                            anchors.fill: parent
                            visible: mouseArea.containsMouse
                            color: Theme.colors.layer1
                            radius: 8
                        }

                        RowLayout {

                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8

                            Item {

                                Layout.fillHeight: true
                                Layout.preferredWidth: parent.height

                                NblImage {
                                    id: dImageSource
                                    anchors.fill: parent
                                    anchors.verticalCenter: parent.verticalCenter
                                    fillMode: Image.PreserveAspectFit
                                    layer.enabled: true
                                    visible: false
                                    source: installationItem.icon
                                }

                                Item {
                                    id: dMaskShape
                                    anchors.fill: dImageSource
                                    visible: false
                                    layer.enabled: true
                                    Rectangle {
                                        anchors.fill: parent
                                        radius: 8
                                    }
                                }

                                MultiEffect {
                                    anchors.fill: dImageSource
                                    source: dImageSource
                                    maskEnabled: true
                                    maskSource: dMaskShape
                                    maskThresholdMin: 0.5
                                    maskSpreadAtMin: 1.0
                                }
                            }

                            Item {

                                id: labels
                                Layout.fillHeight: true
                                Layout.fillWidth: true

                                visible: parent.width > parent.height * 2

                                Label {
                                    text: installationItem.resolvedTitle
                                    y: -(parent.height / 7)
                                    width: parent.width
                                    color: Theme.colors.text
                                    font.family: Theme.font
                                    font.pixelSize: parent.height / 2
                                    elide: Text.ElideRight
                                }

                                Label {
                                    text: "Last played: " + (installationItem.lastPlayed == 0 ? "Never" : Theme.formatDate(installationItem.lastPlayed))
                                    width: parent.width
                                    y: parent.height / 2
                                    color: Theme.colors.textDim
                                    font.family: Theme.font
                                    font.pixelSize: parent.height / 2.5
                                    elide: Text.ElideRight
                                }
                            }
                        }
                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.selectedInstallation = ApplicationContext.getInstallationDetails(installationItem.installationId)
                                dropdown.close()
                            }
                        }
                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: {
                                ctxMenu.open()
                            }
                        }

                        NblContextMenu {
                            id: ctxMenu
                            height: 96
                            y: 0
                            contentItem: ColumnLayout {
                                spacing: 0
                                NblButton {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    icon: ""
                                    text: "Edit"

                                    onClicked: {
                                        ctxMenu.close()
                                        dropdown.close()
                                        let dialog = DialogManager.create("Installation/EditInstallationDialog.qml", { 
                                            installationId: installationItem.installationId
                                        }) as NblDialog
                                        dialog.open()
                                    }
                                    radius: 2
                                }
                                NblButton {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    icon: ""
                                    text: "Delete"
                                    onClicked: {
                                        ctxMenu.close()
                                        dropdown.close()
                                        DialogManager.openDeleteConfirmation("Are you sure you want to delete this installation?", function() {
                                            ApplicationContext.deleteInstallation(installationItem.installationId)
                                            root.selectedInstallation = null
                                            root.refresh()
                                        })
                                    }
                                    radius: 2
                                }
                            }
                        }
                    }
                }

                Item {
                    width: installationsColumn.width
                    height: 48

                    Rectangle {
                        anchors.fill: parent
                        radius: 8
                        color: addMouseArea.containsMouse ? Theme.colors.layer1 : Theme.colors.layer0
                    }

                    Label {
                        anchors.fill: parent
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                        font.family: Theme.font
                        font.pixelSize: 16
                        text: "+ Add Installation"
                        color: Theme.colors.text
                    }

                    MouseArea {
                        id: addMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            dropdown.close()
                            let dialog = DialogManager.create("Installation/AddInstallationDialog.qml", { gameId: root.gameId }) as NblDialog
                            dialog.apply.connect(function() {
                                root.refresh()
                            })
                            dialog.open()
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        enabled: root.selectedInstallation != null
        anchors.fill: parent
        hoverEnabled: enabled
        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: {
            dropdown.open()
        }
    }
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: {
            selectedCtxMenu.open()
        }
    }

    NblContextMenu {
        id: selectedCtxMenu
        height: 96
        x: root.width + 8
        y: 0
        contentItem: ColumnLayout {
            spacing: 0
            NblButton {
                Layout.fillWidth: true
                Layout.fillHeight: true
                icon: ""
                text: "Edit"

                onClicked: {
                    selectedCtxMenu.close()
                    dropdown.close()
                    let dialog = DialogManager.create("Installation/EditInstallationDialog.qml", { 
                        installationId: root.selectedInstallationId
                    }) as NblDialog
                    dialog.open()
                }
                radius: 2
            }
            NblButton {
                Layout.fillWidth: true
                Layout.fillHeight: true
                icon: ""
                text: "Delete"
                onClicked: {
                    selectedCtxMenu.close()
                    dropdown.close()
                    DialogManager.openDeleteConfirmation("Are you sure you want to delete this installation?", function() {
                        ApplicationContext.deleteInstallation(root.selectedInstallationId)
                        root.selectedInstallation = null
                        root.refresh()
                    })
                }
                radius: 2
            }
        }
    }

}
