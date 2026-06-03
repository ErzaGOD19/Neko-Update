/*
 * Neko Void Updater - QML Interface
 * Copyright (C) 2024 Alexander
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: window
    visible: false
    width: 900
    height: 600
    maximumWidth: width
    maximumHeight: height
    title: "Neko Void Updater"
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint

    // --- Palette Properties ---
    readonly property color colorBg: "#1e1e2e"
    readonly property color colorSurface: "#25263a"
    readonly property color colorCurrentLine: "#313244"
    readonly property color colorSelection: "#45475a"
    readonly property color colorFg: "#cdd6f4"
    readonly property color colorComment: "#7f849c"
    readonly property color colorCyan: "#89dceb"
    readonly property color colorGreen: "#a6e3a1"
    readonly property color colorOrange: "#fab387"
    readonly property color colorPink: "#f5c2e7"
    readonly property color colorPurple: "#cba6f7"
    readonly property color colorRed: "#f38ba8"

    readonly property color dracBg: "#282a36"
    readonly property color dracPurple: "#bd93f9"
    readonly property color dracRed: "#ff5555"
    readonly property color dracYellow: "#f1fa8c"
    readonly property color dracGreen: "#50fa7b"

    // Entrance Animation
    opacity: 0
    Component.onCompleted: entranceAnim.start()
    NumberAnimation { id: entranceAnim; target: window; property: "opacity"; from: 0; to: 1; duration: 400; easing.type: Easing.OutQuad }

    // --- Fonts ---
    FontLoader { id: protoFont; source: "qrc:/0xProtoNerdFont-Regular.ttf" }
    FontLoader { id: protoFontBold; source: "qrc:/0xProtoNerdFont-Bold.ttf" }
    FontLoader { id: protoFontMono; source: "qrc:/0xProtoNerdFontMono-Regular.ttf" }

    // --- Main Layout ---
    Rectangle {
        id: mainContainer
        anchors.fill: parent
        color: colorBg
        border.color: dracPurple
        border.width: 2
        radius: 12
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // --- Custom Title Bar ---
            Rectangle {
                id: titleBar
                Layout.fillWidth: true
                height: 40
                color: dracBg
                
                MouseArea {
                    anchors.fill: parent
                    onPressed: (mouse) => { if (mouse.button === Qt.LeftButton) window.startSystemMove() }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    spacing: 10

                    Text { text: "󰄛"; color: dracPurple; font.pixelSize: 18; font.family: protoFont.name }
                    Text { text: "Neko Void Updater"; color: dracPurple; font.pixelSize: 13; font.bold: true; font.family: protoFont.name }
                    Item { Layout.fillWidth: true }
                    Row {
                        spacing: 12
                        Layout.alignment: Qt.AlignVCenter
                        Rectangle { width: 14; height: 14; radius: 7; color: dracGreen; MouseArea { anchors.fill: parent; onClicked: window.showMinimized() } }
                        Rectangle { width: 14; height: 14; radius: 7; color: dracRed; MouseArea { anchors.fill: parent; onClicked: window.close() } }
                    }
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: colorSelection; opacity: 0.2 }
            }

            // --- Body Content ---
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                gradient: Gradient {
                    GradientStop { position: 0; color: colorSurface }
                    GradientStop { position: 1; color: colorBg }
                }

                // Service Warning Banner
                Rectangle {
                    visible: !backend.serviceActive && backend.frequency !== "manual"
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 45
                    color: dracRed
                    opacity: 0.9
                    z: 10

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 15
                        anchors.rightMargin: 15
                        spacing: 10
                        Text { text: ""; font.family: protoFont.name; color: "white"; font.pixelSize: 16 }
                        Text { 
                            text: qsTr("Servicio de fondo inactivo. Ejecuta: 'sudo ln -s /etc/sv/neko-void-sync /var/service/'")
                            color: "white"
                            font.pixelSize: 11
                            font.family: protoFont.name
                            Layout.fillWidth: true
                        }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 25
                    spacing: 30

                    // Left Column: Status & Dual Actions
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 25

                        // Status Card
                        Rectangle {
                            id: statusCard
                            Layout.fillWidth: true
                            height: 240
                            radius: 18
                            color: colorCurrentLine
                            border.color: backend.busy ? colorSelection : (backend.hasUpdates ? colorOrange : colorGreen)
                            border.width: 1
                            Behavior on border.color { ColorAnimation { duration: 300 } }

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 18
                                
                                Item {
                                    width: 100; height: 100
                                    Layout.alignment: Qt.AlignHCenter
                                    Image {
                                        id: statusImg
                                        source: backend.busy ? "qrc:/icon-working.png" : (backend.hasUpdates ? "qrc:/icon-warning.png" : "qrc:/icon-normal.png")
                                        anchors.fill: parent
                                        smooth: true
                                        fillMode: Image.PreserveAspectFit
                                        NumberAnimation on y { from: 0; to: -4; duration: 2000; loops: Animation.Infinite; easing.type: Easing.InOutQuad }
                                    }
                                }

                                Text {
                                    text: backend.busy ? qsTr("Processing...") : (backend.hasUpdates ? qsTr("Updates available") : qsTr("System up to date"))
                                    color: backend.busy ? colorComment : (backend.hasUpdates ? colorOrange : colorGreen)
                                    font.pixelSize: 26
                                    font.bold: true
                                    font.family: protoFont.name
                                    Layout.alignment: Qt.AlignHCenter
                                }

                                Text {
                                    text: backend.statusText
                                    color: colorComment
                                    font.pixelSize: 14
                                    font.family: protoFont.name
                                    Layout.alignment: Qt.AlignHCenter
                                    horizontalAlignment: Text.AlignHCenter
                                    wrapMode: Text.WordWrap
                                    Layout.preferredWidth: parent.width - 40
                                }
                            }
                        }

                        // Action Buttons Row
                        RowLayout {
                            spacing: 20
                            Layout.fillWidth: true

                            // Check Button
                            Button {
                                id: checkBtn
                                Layout.fillWidth: true
                                Layout.preferredHeight: 60
                                text: qsTr("Check for updates")
                                enabled: !backend.busy
                                
                                contentItem: RowLayout {
                                    spacing: 12
                                    Item { Layout.fillWidth: true }
                                    Text { text: "󰇚"; font.family: protoFont.name; font.pixelSize: 20; color: checkBtn.enabled ? colorBg : colorComment; Layout.alignment: Qt.AlignVCenter }
                                    Text { text: checkBtn.text; font.family: protoFont.name; font.bold: true; font.pixelSize: 14; color: checkBtn.enabled ? colorBg : colorComment; Layout.alignment: Qt.AlignVCenter }
                                    Item { Layout.fillWidth: true }
                                }

                                background: Rectangle {
                                    radius: 14
                                    color: checkBtn.enabled ? (checkBtn.pressed ? Qt.darker(colorPurple, 1.1) : (checkBtn.hovered ? Qt.lighter(colorPurple, 1.05) : colorPurple)) : colorSelection
                                    Behavior on color { ColorAnimation { duration: 200 } }
                                }
                                onClicked: backend.checkNow()
                            }

                            // Update Button
                            Button {
                                id: applyBtn
                                Layout.fillWidth: true
                                Layout.preferredHeight: 60
                                text: qsTr("Update now")
                                enabled: backend.hasUpdates && !backend.busy
                                
                                contentItem: RowLayout {
                                    spacing: 12
                                    Item { Layout.fillWidth: true }
                                    Text { text: "󰚰"; font.family: protoFont.name; font.pixelSize: 20; color: applyBtn.enabled ? colorBg : colorComment; Layout.alignment: Qt.AlignVCenter }
                                    Text { text: applyBtn.text; font.family: protoFont.name; font.bold: true; font.pixelSize: 14; color: applyBtn.enabled ? colorBg : colorComment; Layout.alignment: Qt.AlignVCenter }
                                    Item { Layout.fillWidth: true }
                                }

                                background: Rectangle {
                                    radius: 14
                                    color: applyBtn.enabled ? (applyBtn.pressed ? Qt.darker(colorGreen, 1.1) : (applyBtn.hovered ? Qt.lighter(colorGreen, 1.05) : colorGreen)) : colorSelection
                                    Behavior on color { ColorAnimation { duration: 200 } }
                                    
                                    SequentialAnimation on opacity {
                                        running: applyBtn.enabled
                                        loops: Animation.Infinite
                                        NumberAnimation { from: 1.0; to: 0.85; duration: 1200; easing.type: Easing.InOutQuad }
                                        NumberAnimation { from: 0.85; to: 1.0; duration: 1200; easing.type: Easing.InOutQuad }
                                    }
                                }
                                onClicked: backend.applyUpdates()
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }

                    // Right Column: Preferences
                    ColumnLayout {
                        Layout.preferredWidth: 300
                        Layout.fillHeight: true
                        spacing: 20

                        Text { text: "󰒓 Preferences"; color: colorPurple; font.pixelSize: 20; font.bold: true; font.family: protoFont.name }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 18
                            color: colorSurface
                            border.color: colorCurrentLine
                            border.width: 1

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 15

                                Text { text: qsTr("Check frequency"); color: colorComment; font.pixelSize: 13; font.family: protoFont.name }

                                Repeater {
                                    model: [
                                        { id: "manual", label: qsTr("Manual"), icon: "󰙨" },
                                        { id: "1d", label: qsTr("Daily"), icon: "󰃭" },
                                        { id: "3d", label: qsTr("Every 3 days"), icon: "󰃮" },
                                        { id: "1w", label: qsTr("Weekly"), icon: "󰃯" }
                                    ]
                                    Rectangle {
                                        Layout.fillWidth: true; height: 42; radius: 10
                                        color: backend.frequency === modelData.id ? colorSelection : "transparent"
                                        border.color: backend.frequency === modelData.id ? colorGreen : "transparent"
                                        border.width: 1
                                        RowLayout {
                                            anchors.fill: parent; anchors.margins: 8; spacing: 10
                                            Text { text: modelData.icon; font.family: protoFont.name; font.pixelSize: 15; color: backend.frequency === modelData.id ? colorGreen : colorComment }
                                            Text { text: modelData.label; color: backend.frequency === modelData.id ? colorFg : colorComment; font.pixelSize: 12; font.family: protoFont.name; Layout.fillWidth: true }
                                            Rectangle { width: 10; height: 10; radius: 5; color: backend.frequency === modelData.id ? colorGreen : "transparent"; border.color: colorComment; border.width: backend.frequency === modelData.id ? 0 : 1 }
                                        }
                                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: backend.setFrequency(modelData.id) }
                                    }
                                }

                                Item { Layout.fillHeight: true }

                                Rectangle {
                                    Layout.fillWidth: true; height: 65; radius: 10; color: colorBg; border.color: colorCurrentLine
                                    ColumnLayout {
                                        anchors.fill: parent; anchors.margins: 10; spacing: 2
                                        Text { text: "Neko Void Updater"; color: colorPurple; font.pixelSize: 11; font.bold: true; font.family: protoFont.name }
                                        Text { text: "Void Linux System Updater"; color: colorComment; font.pixelSize: 9; font.family: protoFont.name }
                                    }
                                }

                                // Maintenance Button
                                Button {
                                    id: cleanBtn
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 45
                                    enabled: !backend.busy
                                    
                                    contentItem: RowLayout {
                                        spacing: 10
                                        Item { Layout.fillWidth: true }
                                        Text { text: "󰃢"; font.family: protoFont.name; font.pixelSize: 16; color: cleanBtn.enabled ? colorBg : colorComment }
                                        Text { text: qsTr("Clean system"); font.family: protoFont.name; font.bold: true; font.pixelSize: 12; color: cleanBtn.enabled ? colorBg : colorComment }
                                        Item { Layout.fillWidth: true }
                                    }

                                    background: Rectangle {
                                        radius: 10
                                        color: cleanBtn.enabled ? (cleanBtn.pressed ? Qt.darker(colorPink, 1.1) : (cleanBtn.hovered ? Qt.lighter(colorPink, 1.05) : colorPink)) : colorSelection
                                        Behavior on color { ColorAnimation { duration: 200 } }
                                    }
                                    onClicked: backend.cleanSystem()
                                }
                            }
                        }
                    }
                }
            }

            // --- Footer ---
            Rectangle {
                id: footer
                Layout.fillWidth: true; height: 35; color: colorBg
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 20; anchors.rightMargin: 20
                    Text {
                        text: "󱐋 " + (backend.updateCount > 0 ? qsTr("%1 updates ready").arg(backend.updateCount) : qsTr("System synchronized"))
                        color: backend.updateCount > 0 ? colorOrange : colorComment
                        font.pixelSize: 11; font.family: protoFontMono.name
                    }
                    Item { Layout.fillWidth: true }
                    Text { text: "Void Linux | XBPS | Rust Core"; color: colorComment; font.pixelSize: 11; font.family: protoFontMono.name; opacity: 0.6 }
                }
            }
        }
    }
}
