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

import Nebula

Column {

    id: root
    spacing: 8

    required property int gameId

    property int unlockedCount: setsModel.getTotalUnlockedAchievementCount()
    property int totalCount: setsModel.getTotalAchievementCount()
    property bool hasAnyWinCondition: setsModel.hasAnyWinCondition()
    property bool isComplete: unlockedCount == totalCount

    readonly property int setCount: setsModel.count

    function refresh() {
        setsModel.refresh()
        totalCount = setsModel.getTotalAchievementCount()
        unlockedCount = setsModel.getTotalUnlockedAchievementCount()
        hasAnyWinCondition = setsModel.hasAnyWinCondition()
    }

    Repeater {
        model: AchievementSetModel {
            id: setsModel
            gameId: root.gameId
            includeHidden: false
        }
        delegate: Column {

            id: setColumn
            width: root.width
            spacing: 8

            required property var modelData

            readonly property int unlockedCount: setsModel.getSetUnlockedAchievementCount(modelData.id)
            readonly property int totalCount: setsModel.getSetAchievementCount(modelData.id)
            readonly property bool unlockedAll: totalCount == unlockedCount && totalCount > 0

            readonly property bool unlockedOne: achievementRepeater.count > 0 && achievementRepeater.itemAt(0).fullWidth
            readonly property int maxVisibleAchievements: unlockedOne ? 6 : 5
            readonly property bool shouldOverflow: achievementsModel.count > maxVisibleAchievements
            readonly property int visibleAchievements: maxVisibleAchievements - (shouldOverflow ? 1 : 0)

            visible: totalCount > 0

            Loader {
                active: setsModel.count > 1
                height: active ? 16 : 0
                sourceComponent: Label {
                    width: setColumn.width
                    text: `${setColumn.modelData.name} (${setColumn.unlockedCount} / ${setColumn.totalCount}) (${(100.0 * (setColumn.unlockedCount / setColumn.totalCount)).toFixed(0)}%)`
                    color: Theme.colors.textDim
                }
            }

            Flow {
                id: achievementRow
                readonly property real achievementSize: (achievementRow.width + 4) / 5 - 4


                property AchievementModel achievementsModel: AchievementModel {
                    id: achievementsModel
                    setId: setColumn.modelData.id
                    sortBy: 2
                    sortOrder: 1
                    pageSize: 7
                }

                width: parent.width
                spacing: 4


                Loader {
                    active: setColumn.unlockedAll
                    visible: active
                    height: active ? achievementRow.achievementSize : 0
                    width: active ? achievementRow.width : 0
                    sourceComponent: Item {

                        width: achievementRow.width
                        height: achievementRow.achievementSize
                        z: 10

                        Label {
                            width: parent.height
                            height: width
                            font.pixelSize: 48
                            horizontalAlignment: Qt.AlignHCenter
                            verticalAlignment: Qt.AlignVCenter
                            text: "" //       
                            color: Theme.colors.mixSourceOver(setColumn.modelData.awardColor, Theme.colors.accent)
                        }

                        Item {
                            width: parent.width - achievementRow.achievementSize - 8
                            height: parent.height - 4
                            x: achievementRow.achievementSize + 8
                            y: 2
                            Label {
                                width: parent.width
                                anchors.top: parent.top
                                text: "Complete"
                                color: Theme.colors.mixSourceOver(setColumn.modelData.awardColor, Theme.colors.accent)
                                font.pixelSize: 16
                                font.variableAxes: { "wght": 600 }
                                elide: Text.ElideRight
                            }
                            Label {
                                width: parent.width
                                anchors.bottom: parent.bottom
                                text: "All achievements unlocked!"
                                color: Theme.colors.text
                                font.pixelSize: 14
                                elide: Text.ElideRight
                            }
                        }
                    }
                }

                Repeater {
                    id: achievementRepeater
                    model: achievementRow.achievementsModel
                    delegate: Item {
                        id: achievementItem
                        required property var modelData
                        required property int index

                        readonly property bool fullWidth: index == 0 && modelData.dateUnlocked > 0 && !setColumn.unlockedAll
                        readonly property bool isProgression: modelData.flags & 1
                        readonly property bool isWinCondition: modelData.flags & 2

                        width: fullWidth ? achievementRow.width : achievementRow.achievementSize
                        height: achievementRow.achievementSize

                        visible: index < setColumn.visibleAchievements

                        AchievementIcon {
                            title: achievementItem.modelData.title
                            icon: achievementItem.modelData.dateUnlocked > 0 ? achievementItem.modelData.icon : achievementItem.modelData.lockedIcon
                            borderColor: achievementItem.modelData.borderColor
                            unlocked: achievementItem.modelData.dateUnlocked > 0
                            hidden: !unlocked && (achievementItem.modelData.flags & 4)
                            rare: achievementItem.modelData.globalUnlockRate < 10.0
                            width: achievementRow.achievementSize
                            height: width
                        }

                        Loader {
                            active: achievementItem.fullWidth
                            sourceComponent: Item {
                                width: achievementItem.width - achievementRow.achievementSize - 8
                                height: achievementItem.height - 4
                                x: achievementRow.achievementSize + 8
                                y: 2
                                AchievementTitle {
                                    width: parent.width
                                    anchors.top: parent.top
                                    title: achievementItem.modelData.title
                                    isProgression: achievementItem.isProgression
                                    isWinCondition: achievementItem.isWinCondition
                                }
                                Label {
                                    width: parent.width
                                    anchors.bottom: parent.bottom
                                    text: achievementItem.modelData.description
                                    color: Theme.colors.textDim
                                    font.pixelSize: 14
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                let dialog = DialogManager.create("Achievement/AchievementDialog.qml", { 
                                    gameId: root.gameId,
                                    setId: setColumn.modelData.id
                                }) as NblDialog
                                dialog.open()
                            }
                        }
                    }
                }

                Item {
                    id: overflowButton
                    width: achievementRow.achievementSize
                    height: width

                    visible: setColumn.shouldOverflow

                    Rectangle {
                        anchors.fill: parent
                        color: Theme.colors.border
                        radius: 8
                    }

                    Label {
                        anchors.fill: parent
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                        text: ""
                        font.family: Theme.nerdFont
                        font.pixelSize: 32
                        color: Theme.colors.textDim
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            let dialog = DialogManager.create("Achievement/AchievementDialog.qml", { 
                                gameId: root.gameId
                            }) as NblDialog
                            dialog.open()
                        }
                    }
                }
            }
        }
    }
}
