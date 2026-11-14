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

NblDialog {
    
    id: root

    property int tabId: 0

    property list<int> removedFilters: []

    showApplyButton: tabId != 0

    onTabIdChanged: {
        if(tabId == 0) return
        let ref = ApplicationContext.getTabMeta(tabId)

        nameInput.text = ref.name
        iconPicker.icon = ref.icon
        iconPicker.isImage = ref.iconIsImage
        iconPicker.colorize = ref.colorizeIcon
        hiddenBox.checked = ref.hidden
        filterList.initialItems = ref.encodeFilters()
    }

    function encode() {

        let filters = []
        for(let i = 0 ; i < filterList.count; i++) {
            let filter = filterList.itemAt(i)
            filters.push(filter.encode(i, filterList.count))
        }

        return {
            id: tabId,
            name: nameInput.text,
            icon: iconPicker.icon,
            iconIsImage: iconPicker.isImage,
            colorizeIcon: iconPicker.colorize,
            hidden: hiddenBox.checked,
            filters: filters,
            removedFilters: removedFilters
        }
    }

    onApply: {
        let tabMeta = encode()
        if(tabId == 0) {
            ApplicationContext.addTab(tabMeta)
        } else {
            ApplicationContext.updateTab(tabId, tabMeta)
        }
    }

    NblScrollView {

        width: parent.width
        height: parent.height - 16
        y: 16

        contentWidth: width - 48
        contentHeight: controls.height
        clip: true

        Column {

            id: controls
            x: 24
            width: parent.width
            spacing: 16

            NblTextBox {
                id: nameInput
                width: controls.width
                label: "Name"
            }

            NblIconPicker {
                id: iconPicker
            }

            NblCheckBox {
                id: hiddenBox
                width: controls.width
                text: "Hidden"

                NblToolTip {
                    delay: 500
                    text: "Hidden tabs can be shown again with Ctrl+H or the Show Hidden Tabs menu item."
                    visible: hiddenBox.hovered
                }
            }

            
            NblListInput {

                id: filterList
                label: "Filters"
                width: controls.width

                allowReorder: true

                createNew: function(index) {
                    return {
                        id: 0,
                        name: "Filter",
                        icon: "󰊗",
                        colorizeIcon: true,
                        multiLine: true,
                        useIcons: false,
                        showTitles: false,
                        showInstallations: false,
                        onlyInstalled: false,
                        showFilterInfo: false,
                        iconIsImage: false,
                        sortBy: 0,
                        sortOrder: 0,
                        tags: [],
                        excludeTags: [],
                        minReleaseDate: 0,
                        maxReleaseDate: 0,
                        minRating: 0,
                        maxRating: 0,
                        minTime: 0,
                        maxTime: 0,
                        minAchievements: 0,
                        maxAchievements: 0,
                        minUnlockedAchievements: 0,
                        maxUnlockedAchievements: 0,
                    }
                }

                onItemPopped: index => {

                    let item = itemAt(index)
                    let id = item.modelData.id

                    if(id != 0 && !root.removedFilters.includes(id)) {
                        root.removedFilters.push(id)
                    }
                }

                delegate: NblDropdown {

                    id: dropdown
                    width: filterList.itemWidth
                    icon: filterIconPicker.icon
                    iconSize: 20

                    contentHeight: filterContent.height
                    label: filterNameBox.text

                    required property var modelData

                    function encode(index, count) {
                        return {
                            id: modelData.id,
                            priority: count - index - 1,
                            name: filterNameBox.text,
                            icon: filterIconPicker.icon,
                            colorizeIcon: filterIconPicker.colorize,
                            multiLine: multiLineBox.checked,
                            useIcons: useIconsBox.checked,
                            showTitles: showTitlesBox.checked,
                            onlyInstalled: onlyInstalledBox.checked,
                            showInstallations: searchBox.currentIndex == 1,
                            showFilterInfo: showFilterInfoBox.checked,
                            iconIsImage: filterIconPicker.isImage,
                            sortBy: sortByBox.currentIndex,
                            sortOrder: sortOrderBox.currentIndex,
                            tags: tagSetsList.encode(),
                            excludeTags: excludeTagsList.encode(),
                            minReleaseDate: minReleaseBox.getTime(),
                            maxReleaseDate: maxReleaseBox.getTime(),
                            minRating: minRatingBox.value,
                            maxRating: maxRatingBox.value,
                            minTime: minTimeBox.value,
                            maxTime: maxTimeBox.value,
                            minAchievements: minAchBox.value,
                            maxAchievements: maxAchBox.value,
                            minUnlockedAchievements: minUnlockedAchBox.value,
                            maxUnlockedAchievements: maxUnlockedAchBox.value,
                        }
                    }

                    Column {

                        width: dropdown.contentWidth

                        id: filterContent
                        spacing: 8


                        NblTextBox {
                            id: filterNameBox
                            width: parent.width
                            label: "Name"
                            text: dropdown.modelData.name
                        }

                        NblIconPicker {
                            id: filterIconPicker
                            //dbIcon: dropdown.modelData.icon
                            icon: dropdown.modelData.icon
                            isImage: dropdown.modelData.iconIsImage
                            colorize: dropdown.modelData.colorizeIcon
                        }

                        NblComboBox {
                            id: searchBox
                            width: parent.width
                            label: "Search for"
                            model: [ "Games", "Installations" ]
                            currentIndex: dropdown.modelData.showInstallations ? 1 : 0
                        }
                        NblCheckBox {
                            id: onlyInstalledBox
                            visible: searchBox.currentIndex == 0
                            Layout.fillWidth: true
                            text: "Only Installed"
                            checked: dropdown.modelData.onlyInstalled
                        }

                        RowLayout {
                            width: parent.width

                            NblCheckBox {
                                id: multiLineBox
                                Layout.fillWidth: true
                                text: "Multi-line"
                                checked: dropdown.modelData.multiLine
                            }
                            NblCheckBox {
                                id: useIconsBox
                                Layout.fillWidth: true
                                text: "Use Icons"
                                checked: dropdown.modelData.useIcons
                            }
                            NblCheckBox {
                                id: showTitlesBox
                                Layout.fillWidth: true
                                text: "Show Titles"
                                checked: dropdown.modelData.showTitles
                            }
                            NblCheckBox {
                                id: showFilterInfoBox
                                Layout.fillWidth: true
                                text: "Show Filter Info"
                                checked: dropdown.modelData.showFilterInfo
                            }
                        }

                        NblComboBox {
                            id: sortByBox
                            width: parent.width
                            label: "Sort By"
                            model: [ "Title", "Last Played", "Added", "Play Count", "Play Time", "Release Date", "Rating", "Achievement Progress" ]
                            currentIndex: dropdown.modelData.sortBy
                        }

                        NblComboBox {
                            id: sortOrderBox
                            width: parent.width
                            label: "Sort Order"
                            model: [ "Ascending", "Descending" ]
                            currentIndex: dropdown.modelData.sortOrder
                        }

                        NblListInput {
                            id: tagSetsList
                            label: "Tag Sets"
                            width: parent.width

                            initialItems: getInitialItems()

                            function getInitialItems() {
                                let tagLists = dropdown.modelData.tags
                                let out = []
                                for(let i = 0 ; i < tagLists.length ; i++) {
                                    out.push({
                                        index: i + 1,
                                        initialTagIds: tagLists[i]
                                    })
                                }
                                return out
                            }

                            createNew: function(i) {
                                return {
                                    index: i + 1,
                                    initialTagIds: []
                                }
                            }
                            delegate: TagList {
                                required property var modelData
                                width: parent.width
                                labelIndent: 24
                                label: "Tag Set " + modelData.index
                                initialTags: ApplicationContext.getTagsFromIds(modelData.initialTagIds)
                            }

                            function encode() {

                                let out = []

                                for(let i = 0 ; i < count ; i ++) {

                                    let set = []

                                    let tagList = itemAt(i) as TagList
                                    let tags = tagList.getListedTags()
                                    for(let t = 0 ; t < tags.length ; t++) {
                                        set.push(tags[t])
                                    }

                                    out.push(set)
                                }

                                return out
                            }
                        }

                        TagList {
                            id: excludeTagsList
                            width: parent.width
                            label: "Exclude Tags"
                            initialTags: ApplicationContext.getTagsFromIds(dropdown.modelData.excludeTags)

                            function encode() {

                                let set = []
                                let tags = getListedTags()
                                for(let t = 0 ; t < tags.length ; t++) {
                                    set.push(tags[t])
                                }

                                return set
                            }
                        }

                        function getDate(time) {
                            let out = new Date()
                            out.setTime(time)
                            return out
                        }

                        NblDateInput {
                            id: minReleaseBox
                            width: parent.width
                            label: "Minimum Release Date"
                            date: parent.getDate(dropdown.modelData.minReleaseDate)
                        }

                        NblDateInput {
                            id: maxReleaseBox
                            width: parent.width
                            label: "Maximum Release Date (exclusive)"
                            date: parent.getDate(dropdown.modelData.maxReleaseDate)
                        }

                        NblNumberInput {
                            id: minRatingBox
                            width: parent.width
                            label: "Minimum Rating"
                            min: 0
                            max: 100
                            allowDecimal: true
                            value: dropdown.modelData.minRating
                        }

                        NblNumberInput {
                            id: maxRatingBox
                            width: parent.width
                            label: "Maximum Rating (exclusive)"
                            min: 0
                            max: 100
                            allowDecimal: true
                            value: dropdown.modelData.maxRating
                        }

                        NblNumberInput {
                            id: minTimeBox
                            width: parent.width
                            label: "Minimum Play Time (ms)"
                            min: 0
                            value: dropdown.modelData.minTime
                        }

                        NblNumberInput {
                            id: maxTimeBox
                            width: parent.width
                            label: "Maximum Play Time (ms) (exclusive)"
                            min: 0
                            value: dropdown.modelData.maxTime
                        }

                        NblNumberInput {
                            id: minAchBox
                            width: parent.width
                            label: "Minimum Achievements"
                            min: 0
                            value: dropdown.modelData.minAchievements
                        }

                        NblNumberInput {
                            id: maxAchBox
                            width: parent.width
                            label: "Maximum Achievements"
                            min: 0
                            value: dropdown.modelData.maxAchievements
                        }

                        NblNumberInput {
                            id: minUnlockedAchBox
                            width: parent.width
                            label: "Minimum Unlocked Achievements"
                            min: 0
                            value: dropdown.modelData.minUnlockedAchievements
                        }

                        NblNumberInput {
                            id: maxUnlockedAchBox
                            width: parent.width
                            label: "Maximum Unlocked Achievements"
                            min: 0
                            value: dropdown.modelData.maxUnlockedAchievements
                        }
                    }
                }
            }
        }
    }
}
