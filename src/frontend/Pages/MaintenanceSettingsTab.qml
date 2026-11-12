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

SettingsTab {

    id: root

    showApplyButton: false

    Column {

        anchors.fill: parent
        spacing: 16

        Label {
            text: "Image Cache"
            font.family: Theme.font
            font.pixelSize: 18
            font.weight: 500
            color: Theme.colors.text
        }

        Label {
            width: parent.width
            text: "Image references left behind by deleted games, installations, tabs, achievements, and accounts are cleaned up when the launcher starts. You can also run the cleanup now."
            wrapMode: Text.WordWrap
            font.family: Theme.font
            font.pixelSize: 16
            color: Theme.colors.textDim
        }

        NblButton {
            text: "Clean Up Unused Images"
            width: 280
            height: 48
            inactiveColor: Theme.colors.layer1
            centered: true

            onClicked: {
                if(ApplicationContext.cleanupUnusedImages()) {
                    statusLabel.text = "Unused image data removed."
                } else {
                    statusLabel.text = "The image cleanup failed. Check the log for details."
                }
            }
        }

        Label {
            id: statusLabel
            font.family: Theme.font
            font.pixelSize: 16
            color: Theme.colors.textDim
        }
    }
}
