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
import QtQuick.Effects
import QtQuick.Controls

import Nebula

Item {

    id: root
    required property GameDetails details
    
    property int selectedInstallation: installationPicker.selectedInstallationId
    property int logsInstallationId: 0
    property InstallationDetails installationDetails


    property int logoPosition: installationDetails == null ? details.logoPosition : installationDetails.resolvedLogoPosition
    property string logo: installationDetails == null ? details.logoUrl() : installationDetails.resolvedLogoUrl
    property string banner: installationDetails == null ? details.bannerUrl() : installationDetails.resolvedBannerUrl

    property bool isFavorite: false

    signal closeRequested
    signal tagClicked(int id, string name, color color)

    function refresh(gameClosed = false) {
        isFavorite = false
        details = ApplicationContext.getGameDetails(details.gameId)
        installationPicker.refresh()
        achievementsSection.refresh()
        achievementUpdateRunner.update(root.details.gameId, gameClosed ? 2 : 3)
    }

    onDetailsChanged: {
        if(installationDetails != null && installationDetails.colorOverride != null) {
            Theme.colors.accentOverride = installationDetails.colorOverride 
        } else if(details != null && details.colorOverride != null) {
            Theme.colors.accentOverride = details.colorOverride
        } else {
            Theme.colors.accentOverride = null
        }
    }

    onInstallationDetailsChanged: {
        installationPicker.selectedInstallation = installationDetails

        if(installationDetails != null && installationDetails.colorOverride != null) {
            Theme.colors.accentOverride = installationDetails.colorOverride 
        } else if(details != null && details.colorOverride != null) {
            Theme.colors.accentOverride = details.colorOverride
        } else {
            Theme.colors.accentOverride = null
        }
    }

    Component.onCompleted: {
        achievementUpdateRunner.update(root.details.gameId, 0)
    }

    onSelectedInstallationChanged: {
        if(selectedInstallation != 0) {
            logsInstallationId = selectedInstallation
        }
    }

    Component.onDestruction: {
        if(logsInstallationId != 0) {
            ApplicationContext.clearInstallationLogs(logsInstallationId)
        }
    }

    AchievementUpdateRunner { 
        id: achievementUpdateRunner
        onUpdateComplete: ret => {
            if(ret) {
                achievementsSection.refresh()
            }
        }
    }

    GridLayout {

        width: parent.width
        height: parent.height

        rows: 3
        columns: 2

        columnSpacing: 8
        rowSpacing: 8

        uniformCellWidths: false
        uniformCellHeights: false

        // Banner
        Item {

            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.preferredHeight: 240

            NblCrossfadeImage {
                id: banner
                source: root.banner
                fillMode: Image.PreserveAspectCrop
                width: parent.width
                height: parent.height
                visible: false
                layer.enabled: true
            }

            Item {
                id: maskShape
                width: banner.width
                height: banner.height
                visible: false
                layer.enabled: true
                Rectangle {
                    anchors.fill: maskShape
                    radius: 8
                }
            }

            MultiEffect {
                source: banner
                anchors.fill: banner
                maskEnabled: true
                maskSource: maskShape
                maskThresholdMin: 0.5
                maskThresholdMax: 1.0
            }

            // Logo image
            Item {

                height: parent.height
                width: 256
                visible: root.logoPosition < 3
                x: _calculateX()

                function _calculateX() {
                    if(root.logoPosition == 0) { // Left
                        return 0
                    }
                    if(root.logoPosition == 1) { // Center
                        return (parent.width / 2) - (width / 2)
                    }
                    if(root.logoPosition == 2) { // Right
                        return parent.width - width
                    }
                    return 0
                }

                Behavior on x {
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.InOutQuad
                    }
                }

                NblCrossfadeImage {
                    id: logo
                    source: root.logoPosition >= 3 ? "" : root.logo
                    anchors.centerIn: parent
                    width: parent.width - 16
                    height: parent.height - 16
                    fillMode: Image.PreserveAspectFit
                }
            }
        }

        RowLayout {

            id: launchBar

            Layout.fillWidth: true
            Layout.preferredHeight: 72
            Layout.topMargin: -24

            spacing: 8

            Rectangle {
                Layout.preferredWidth: 8
                Layout.preferredHeight: 48
                Layout.alignment: Qt.AlignBottom
                radius: 8
                color: Theme.colors.layer1
            }

            Item {
                Layout.maximumWidth: 250
                Layout.minimumWidth: 48
                Layout.fillWidth: true
                Layout.preferredHeight: 72

                Rectangle {
                    width: parent.width + 16
                    height: parent.height + 16
                    x: -8
                    y: -8
                    radius: 16
                    color: Theme.colors.layer0
                    enabled: false
                }

                LaunchButton {
                    id: launchBtn
                    anchors.fill: parent
                    installationId: root.selectedInstallation
                    gameId: root.details.gameId
                    onInstallationAdded: {
                        root.refresh()
                    }
                    onApplicationStopped: {
                        root.refresh()
                    }
                }                
            }

            InstallationPicker {
                id: installationPicker
                gameId: root.details.gameId
                Layout.maximumWidth: 250
                Layout.minimumWidth: 48
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                Layout.alignment: Qt.AlignBottom

                onSelectedInstallationChanged: {
                    root.installationDetails = selectedInstallation
                }
            }
            Rectangle {
                visible: root.width > 808
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                Layout.alignment: Qt.AlignBottom
                radius: 8
                color: Theme.colors.layer1
            }

        }

        // Actions
        Item {

            Layout.preferredWidth: 272
            Layout.preferredHeight: 48

            RowLayout {
                anchors.fill: parent

                spacing: 8

                NblIconButton {
                    Layout.fillWidth: true
                    icon: ""
                    colorUnhovered: root.isFavorite ? Theme.colors.accent : Theme.colors.textDim
                    onClicked: {


                        let favoriteTagId = ApplicationContext.findTagId("favorite")

                        let rc = 0
                        if(root.isFavorite) {
                            rc = ApplicationContext.untagGame(root.details.gameId, favoriteTagId)
                        } else {
                            rc = ApplicationContext.tagGame(root.details.gameId, favoriteTagId)
                        }
                        if(rc != 0) {
                            ApplicationContext.pushNotification({
                                type: "error",
                                title: "Unable to mark as favorite",
                                message: "Does the \"favorite\" tag exist?"
                            })
                        } else {
                            root.isFavorite = !root.isFavorite
                            tagModel.refresh()
                        }
                    }
                }

                NblIconButton {
                    Layout.fillWidth: true
                    icon: ""
                    onClicked: {
                        ApplicationContext.openInstallationFolder(root.selectedInstallation)
                    }
                }

                NblIconButton {
                    Layout.fillWidth: true
                    icon: "󰆍"
                    onClicked: {
                        let dialog = DialogManager.create("Launch/StdoutDialog.qml", { 
                            process: launchBtn.handle,
                            retainedLogs: ApplicationContext.getInstallationLogs(root.selectedInstallation)
                        }) as NblDialog
                        dialog.open()
                    }
                }

                NblIconButton {
                    Layout.fillWidth: true
                    icon: ""
                    onClicked: {
                        let dialog = DialogManager.create("Pages/EditInstallationDialog.qml", { 
                            gameId: root.details.gameId,
                            installationId: root.selectedInstallation
                        }) as NblDialog
                        dialog.apply.connect(function() {
                            root.refresh()
                        })
                        dialog.open()
                    }
                }

                NblIconButton {
                    Layout.fillWidth: true
                    icon: ""
                    onClicked: {
                        DialogManager.openDeleteConfirmation( "Are you sure you want to delete this game and all of its installations?", function() {
                            ApplicationContext.deleteGame(root.details.gameId)
                            root.closeRequested()
                        })
                    }
                }
            }
        }

        // Title / Description
        Item {

            Layout.fillHeight: true
            Layout.fillWidth: true

            Column {

                anchors.fill: parent
                anchors.margins: 16

                spacing: 16

                Label {
                    font.family: Theme.font
                    font.pixelSize: 24
                    font.weight: 500
                    color: Theme.colors.text
                    text: root.details.title
                }

                Label {
                    width: parent.width

                    font.family: Theme.font
                    font.pixelSize: 16
                    font.weight: 400
                    wrapMode: Text.WordWrap

                    color: Theme.colors.textDim
                    text: root.details.description
                }
            }
        }

        // Tags and other details
        Item { 
            Layout.fillHeight: true
            Layout.preferredWidth: 272

            Rectangle {
                anchors.fill: parent
                color: Theme.colors.layer1
                border.width: 2
                border.color: Theme.colors.border
                radius: 8
            }

            NblScrollView {

                anchors.fill: parent
                anchors.topMargin: 16
                anchors.bottomMargin: 16
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                contentHeight: righthandColumn.height
                contentWidth: width
                clip: true

                ColumnLayout {

                    id: righthandColumn
                    width: parent.width - 20
                    x: 12
                    spacing: 8

                    Column {

                        Layout.fillWidth: true
                        visible: achievementsSection.totalCount > 0
                        spacing: 8

                        Item {
                            width: parent.width
                            height: 16
                            RowLayout {
                                spacing: 4
                                Label {
                                    font.weight: 600
                                    color: Theme.colors.text
                                    text: "Achievements"
                                }
                                Label {
                                    font.weight: 600
                                    color: Theme.colors.textDim
                                    text: `(${achievementsSection.unlockedCount} / ${achievementsSection.totalCount}) (${(100.0 * (achievementsSection.unlockedCount / achievementsSection.totalCount)).toFixed(0)}%)`
                                }
                                Item {
                                    Layout.fillHeight: true
                                    implicitWidth: height
                                    visible: achievementUpdateRunner.isRunning

                                    NblBusyIndicator {
                                        color: Theme.colors.textDim
                                        icon: ""
                                        anchors.fill: parent
                                    }
                                }
                            }

                            NblContextMenu {
                                id: contextMenu
                                height: 48
                                y: 0

                                contentItem: ColumnLayout {
                                    spacing: 0
                                    NblButton {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true

                                        icon: ""
                                        text: "Refresh"

                                        onClicked: {
                                            achievementUpdateRunner.update(root.details.gameId, 4)
                                            contextMenu.close()
                                        }
                                        radius: 2
                                    }
                                }
                            }


                            HoverHandler {
                                cursorShape: Qt.PointingHandCursor
                            }
                            TapHandler {
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                onTapped: (point, btn) => {

                                    if(btn == Qt.RightButton) {
                                        contextMenu.open()
                                    } else {
                                        let dialog = DialogManager.create("Achievement/AchievementDialog.qml", { 
                                            gameId: root.details.gameId,
                                        }) as NblDialog
                                        dialog.closed.connect(() => {
                                            achievementsSection.refresh()
                                        })
                                        dialog.open()
                                    }
                                }
                            }
                        }

                        AchievementSection {
                            id: achievementsSection
                            width: parent.width
                            gameId: root.details.gameId
                        }
                    }

                    Label {
                        font.family: Theme.font
                        font.pixelSize: 16
                        font.weight: 600
                        color: Theme.colors.text
                        text: "Tags"
                    }

                    Flow {

                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: GameTagModel {
                                id: tagModel
                                gameId: root.details.gameId
                                pageSize: 8
                            }

                            Tag {
                                required property var model;
                                color: model.color
                                text: model.name
                                width: (parent.width / 2) - 4

                                Component.onCompleted: {
                                    if(model.name == "favorite") root.isFavorite = true
                                }

                                onClicked: {
                                    root.tagClicked(model.id, model.name, model.color)
                                }
                            }
                        }
                    }



                    Label {
                        font.family: Theme.font
                        font.pixelSize: 16
                        font.weight: 600
                        color: Theme.colors.text
                        text: "Metadata"
                    }

                    Label {
                        font.family: Theme.font
                        font.pixelSize: 14
                        font.weight: 600
                        color: Theme.colors.textDim
                        text: "Release Date: " + Theme.formatDate(root.details.releaseDate)
                    }

                    Label {
                        font.family: Theme.font
                        font.pixelSize: 14
                        font.weight: 600
                        color: Theme.colors.textDim
                        text: "Added: " + Theme.formatDate(root.details.added)
                    }

                    Label {
                        font.family: Theme.font
                        font.pixelSize: 14
                        font.weight: 600
                        color: Theme.colors.textDim
                        text: "Last played: " + (root.details.lastPlayed == 0 ? "Never" : Theme.formatDate(root.details.lastPlayed))
                    }

                    Label {
                        visible: root.details.rating > 0
                        font.family: Theme.font
                        font.pixelSize: 14
                        font.weight: 600
                        color: Theme.colors.textDim
                        text: "Rating: " + root.details.rating.toFixed(1) + "%"
                    }

                    Label {
                        font.family: Theme.font
                        font.pixelSize: 14
                        font.weight: 600
                        color: Theme.colors.textDim
                        text: "Play time: " + root._formatElapsedTime(root.details.playTime)
                    }

                    Label {
                        font.family: Theme.font
                        font.pixelSize: 14
                        font.weight: 600
                        color: Theme.colors.textDim
                        text: "Play count: " + root.details.playCount
                    }
                }
            }
        }
    }

    function _formatElapsedTime(totalMs) {
        let totalSeconds = Math.floor(totalMs / 1000);
        let seconds = totalSeconds % 60;
        let minutes = Math.floor(totalSeconds / 60) % 60;
        let hours = Math.floor(totalSeconds / 3600);

        let pad = (num) => String(num).padStart(2, '0');

        return `${pad(hours)}:${pad(minutes)}:${pad(seconds)}`;
    }

}
