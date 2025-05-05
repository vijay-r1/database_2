import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import QtQuick.Window 2.15
import com.thermo.fetcher 1.0

ApplicationWindow {
visible: true
width: 800
height: 500
title: "Export App"
Material.theme: Material.Light


Rectangle {
    anchors.fill: parent
    color: "#f0f0f0"

    // Sidebar with Export Icon
    Rectangle {
        width: 60
        height: parent.height
        color: "#2196F3"

        Image {
            id: exportIcon
            source: "qrc:/qml/export_icon.png"
            anchors.centerIn: parent
            width: 32
            height: 32

            MouseArea {
                anchors.fill: parent
                onClicked: exportPopup.visible = true
            }
        }
    }

    // Main Popup Window
    Rectangle {
        id: exportPopup
        visible: false
        width: 500
        height: 450
        radius: 10
        anchors.centerIn: parent
        color: "white"
        border.color: "#aaa"
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15

            RowLayout {
                spacing: 10
                Label { text: "Type:"; font.bold: true }
                ComboBox {
                    id: typeCombo
                    model: ["CSV", "PDF", "PUC"]
                    onCurrentTextChanged: {
                        if (currentText === "PDF") {
                            lengthCombo.model = ["1 Day", "1 Week"]
                        } else {
                            lengthCombo.model = ["1 Week", "1 Month", "6 Months", "Custom"]
                        }
                    }
                }
            }

            RowLayout {
                spacing: 10
                Label { text: "Length:"; font.bold: true }
                ComboBox { id: lengthCombo }
            }

            RowLayout {
                spacing: 10
                Label { text: "Date Range:"; font.bold: true }
            }

            RowLayout {
                spacing: 10

                ComboBox { id: fromYear; model: getYears(); currentIndex: 23 } // default 2023
                ComboBox { id: fromMonth; model: getMonths(); currentIndex: 0 }
                ComboBox { id: fromDay; model: getDays(); currentIndex: 0 }
            }

                // "to" Label
             Label {
                text: "to"
                horizontalAlignment: Text.AlignHCenter
                width: 200
            }
            RowLayout {
               spacing: 10
                ComboBox { id: toYear; model: getYears(); currentIndex: 23 }
                ComboBox { id: toMonth; model: getMonths(); currentIndex: 0 }
                ComboBox { id: toDay; model: getDays(); currentIndex: 0 }
            }

            Rectangle {
                width: parent.width
                height: 40
                color: "#87CEEB"
                radius: 6

                Text {
                    anchors.centerIn: parent
                    text: "Export Log"
                    font.bold: true
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        var fromDate = fromYear.currentText + "/" + pad(fromMonth.currentText) + "/" + pad(fromDay.currentText) + " 00:00:00"
                        var toDate = toYear.currentText + "/" + pad(toMonth.currentText) + "/" + pad(toDay.currentText) + " 23:59:59"

                        console.log("Export:", typeCombo.currentText, lengthCombo.currentText, "From:", fromDate, "To:", toDate)
                        dataFetcher.startExport(
                            typeCombo.currentText,
                            lengthCombo.currentText,
                            fromDate,
                            toDate
                        )
                    }
                }
            }
        }
    }
}

// Helper functions
function getYears() {
    var list = []
    for (var y = 2000; y <= 2050; y++) {
        list.push(y.toString())
    }
    return list
}

function getMonths() {
    var list = []
    for (var m = 1; m <= 12; m++) {
        list.push(m.toString())
    }
    return list
}

function getDays() {
    var list = []
    for (var d = 1; d <= 31; d++) {
        list.push(d.toString())
    }
    return list
}

function pad(n) {
    return n.length === 1 ? "0" + n : n
}

// Backend Connection
Connections {
    target: dataFetcher
    function onLogMessage(msg) {
        console.log("Log:", msg)
    }

    function onExportComplete() {
        console.log("Export Finished")
    }
}

DataFetcher { id: dataFetcher }


}

