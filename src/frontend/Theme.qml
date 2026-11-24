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


pragma Singleton

import QtQuick

QtObject {

    property string font: _private.mainFont.name
    property string italicFont: _private.italicFont.name
    property string nerdFont: _private.nerdFont.name
    property string monospaceFont: _private.monoFont.name

    property string dateInputFormat: "yyyy/MM/dd"
    function formatDateInput(epochMilli) {
        return Qt.formatDate(new Date(epochMilli), dateInputFormat)
    }

    property string dateFormat: "MMM d, yyyy"
    function formatDate(epochMilli) {
        return Qt.formatDate(new Date(epochMilli), dateFormat)
    }

    property var colors: QtObject {

        function calculateBrightness(color) {
            var r = color.r * 255
            var g = color.g * 255
            var b = color.b * 255
            var brightness = (r * 299 + g * 587 + b * 114) / 1000
            return brightness
        }

        function readableOn(color) {

            let adjustedSaturation = 0.0
            let adjustedLightness = 0.0

            if(color.hslLightness > 0.5) {
                adjustedSaturation = color.hslSaturation * 0.5
                adjustedLightness = 0.25
            } else {
                adjustedSaturation = color.hslSaturation * 1.5
                adjustedLightness = 0.85
            }

            return Qt.hsla(color.hslHue, color.hslSaturation, adjustedLightness)
        }

        function blackOrWhite(color) {
            return calculateBrightness(color) > 200 ? "#000000" : "#ffffff"
        }

        function tagColor(tagName) {

            let hash = 2166136261;

            for (let i = 0; i < tagName.length; ++i) {
                hash ^= tagName.charCodeAt(i);
                hash = Math.imul(hash, 16777619);
            }

            // end of MurmurHash3
            function finalize(x) {
                x = Math.imul(x ^ (x >>> 16), 0x85ebca6b);
                x = Math.imul(x ^ (x >>> 13), 0xc2b2ae35);
                x = (x ^ (x >>> 16)) >>> 0;

                return x / 4294967296.0;
            }

            var h = finalize(hash);
            var s = 0.6 + finalize(hash ^ 0x9e3779b9) * 0.3;
            var l = 0.4 + finalize(hash ^ 0x7f4a7c15) * 0.5;

            return Qt.hsla(h, s, l, 1.0);
        }

        function mixSourceOver(c1, c2) {
            return Qt.rgba(
                (c1.r * c1.a) + (c2.r * (1 - c1.a)),
                (c1.g * c1.a) + (c2.g * (1 - c1.a)),
                (c1.b * c1.a) + (c2.b * (1 - c1.a)),
                1.0
            )
        }

        property bool lightMode: false
        property bool tintUi: true
        property color activeColor: "#d658fc"
        property var accentOverride: null
        property color accent: accentOverride ?? activeColor

        property color layer0: Qt.hsla(accent.hslHue, tintUi ? 0.3 : 0, lightMode ? 0.95 : 0.125, 1.0)
        property color layer1: Qt.hsla(accent.hslHue, tintUi ? 0.3 : 0, lightMode ? 0.9 : 0.169, 1.0)
        property color border: Qt.hsla(accent.hslHue, tintUi ? 0.3 : 0, lightMode ? 0.8 : 0.228, 1.0)
        property color text: lightMode ? "#000000" : "#ffffff"
        property color textDim: Qt.hsla(accent.hslHue, tintUi ? 0.3 : 0, lightMode ? 0.2 : 0.75, 1.0)
        property color textOnAccent: blackOrWhite(accent)

        property color shadow: lightMode ? "#99000000" : "#FF000000"
    }

    property var _private: QtObject {

        property FontLoader mainFont: FontLoader {
            source: "qrc:/qt/qml/Nebula/fonts/NotoSansDisplay-VariableFont_wdth,wght.ttf"
        }
        property FontLoader italicFont: FontLoader {
            source: "qrc:/qt/qml/Nebula/fonts/NotoSansDisplay-Italic-VariableFont_wdth,wght.ttf"
        }
        property FontLoader nerdFont: FontLoader {
            source: "qrc:/qt/qml/Nebula/fonts/NotoSansMNerdFontPropo-Regular.ttf"
        }
        property FontLoader monoFont: FontLoader {
            source: "qrc:/qt/qml/Nebula/fonts/NotoSansMono-VariableFont_wdth,wght.ttf"
        }
    }

}
