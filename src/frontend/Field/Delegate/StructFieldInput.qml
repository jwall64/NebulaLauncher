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
import QtQuick.Layouts

import Nebula

FieldInputDelegate {

    id: root
    implicitHeight: contentColumn.height

    onValueChanged: {
        fieldInput.load(value)
    }

    Column {

        id: contentColumn
        spacing: 8
        width: parent.width

        Item {

            visible: root.field.name.length > 0

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
                x: 12
                color: Theme.colors.layer0
            }

            Label {
                id: labelLabel
                anchors.centerIn: labelBg
                font.family: Theme.font
                font.pixelSize: 16
                text: root.field.name
                color: Theme.colors.textDim
            }
        }
        GridLayout {

            id: layout
            width: parent.width
            columns: root.field.horizontal ? fieldInput.count : 1
            rowSpacing: 8
            columnSpacing: 8

            FieldInputRepeater {
                id: fieldInput
                fields: root.field.structFields
                itemWidth: root.field.horizontal ? ((layout.width + layout.columnSpacing) / count) - layout.columnSpacing : layout.width

                onModified: {
                    root.value = save()
                }
            }
        }
    }
}
