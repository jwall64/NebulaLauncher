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

import QtQuick
import QtQuick.Controls
import Nebula

NblDialog {

    id: root

    property string pluginName
    required property FieldList fields
    required property var values

    Component.onCompleted: {
        fields.load(values)
    }

    onValuesChanged: {
        fields.load(values)
    }

    function save() {
        return fields.save()
    }

    NblScrollView {
        width: parent.width + 16
        height: parent.height

        contentWidth: parent.width
        contentHeight: contentColumn.height

        Column {
            width: parent.width
            id: contentColumn 

            spacing: 16

            Label {
                text: root.pluginName + " Settings"
                font.family: Theme.font
                font.pixelSize: 16
            }

            FieldInputRepeater {
                width: parent.width
                id: fields
                fields: root.fields
            }
        }
    } 
}
