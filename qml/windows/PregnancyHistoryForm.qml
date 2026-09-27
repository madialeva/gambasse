pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Gambasse

// Pregnancy history form (Widgets PregnancyHistoryWindow): personal data,
// pregnancy and medical history, housing, the physical examination with its
// three inner groups and the two treatment tabs. Every control is transcribed
// from the .cpp layout (absolute coordinates, group-relative) and bound by
// name to PregnancyHistoryController; the three fixed VB.NET rows (iron, folic
// acid and deworming) are shown read-only because the form never persisted
// them.
Item {
    id: root

    property HistoryController controller: null

    // Previous treatment: five rows of date / medication / dose at the
    // coordinates of the .cpp arrays (prevY, prevH).
    readonly property var previousRows: [
        {
            "y": 51,
            "h": 38,
            "date": "supplementDate1",
            "medication": "supplementMedication1",
            "dose": "supplementDose1"
        },
        {
            "y": 94,
            "h": 22,
            "date": "supplementDate2",
            "medication": "supplementMedication2",
            "dose": "supplementDose2"
        },
        {
            "y": 122,
            "h": 22,
            "date": "supplementDate3",
            "medication": "supplementMedication3",
            "dose": "supplementDose3"
        },
        {
            "y": 150,
            "h": 22,
            "date": "supplementDate4",
            "medication": "supplementMedication4",
            "dose": "supplementDose4"
        },
        {
            "y": 178,
            "h": 22,
            "date": "supplementDate5",
            "medication": "supplementMedication5",
            "dose": "supplementDose5"
        }
    ]

    // Current treatments: the persisted rows plus the three fixed ones, at the
    // coordinates of the .cpp arrays (nowLabelY, nowY).
    readonly property var currentRows: [
        {
            "caption": qsTr("Calcium"),
            "labelX": 19,
            "labelY": 32,
            "labelW": 116,
            "fieldY": 13,
            "fieldH": 38,
            "fixed": false,
            "fields": ["supplementCalciumD", "supplementCalciumM", "supplementCalciumPd"]
        },
        {
            "caption": qsTr("Iron"),
            "labelX": 19,
            "labelY": 50,
            "labelW": 116,
            "fieldY": 50,
            "fieldH": 21,
            "fixed": true,
            "values": [qsTr("60 mg a day"), qsTr("Ferrous sulfate"), qsTr("Until birth")]
        },
        {
            "caption": qsTr("Folic acid"),
            "labelX": 19,
            "labelY": 70,
            "labelW": 116,
            "fieldY": 70,
            "fieldH": 21,
            "fixed": true,
            "values": [qsTr("4 mg a day"), qsTr("Vitamin B9"), qsTr("Until the fifth month of pregnancy")]
        },
        {
            "caption": qsTr("Deworming"),
            "labelX": 19,
            "labelY": 93,
            "labelW": 116,
            "fieldY": 90,
            "fieldH": 21,
            "fixed": true,
            "values": [qsTr("2nd or 3rd trimester"), qsTr("Mebendazole"), qsTr("Single dose")]
        },
        {
            "caption": qsTr("Vitamin C"),
            "labelX": 19,
            "labelY": 114,
            "labelW": 116,
            "fieldY": 110,
            "fieldH": 21,
            "fixed": false,
            "fields": ["supplementVitaminCD", "supplementVitaminCM", "supplementVitaminCPd"]
        },
        {
            "caption": qsTr("Other vitamins"),
            "labelX": 18,
            "labelY": 134,
            "labelW": 120,
            "fieldY": 130,
            "fieldH": 21,
            "fixed": false,
            "fields": ["supplementOtherVitaminsD", "supplementOtherVitaminsM", "supplementOtherVitaminsPd"]
        },
        {
            "caption": qsTr("Malaria prophylaxis"),
            "labelX": 18,
            "labelY": 153,
            "labelW": 120,
            "fieldY": 150,
            "fieldH": 21,
            "fixed": false,
            "fields": ["supplementMalariaProphylaxisD", "supplementMalariaProphylaxisM", "supplementMalariaProphylaxisPd"]
        }
    ]

    // --- Personal data (2, 2, 482, 109). ---
    GxGroup {
        objectName: "personalGroup"
        x: 2
        y: 2
        width: 482
        height: 109
        title: qsTr("Personal data")

        GxField {
            objectName: "nameField"
            x: 15
            y: 18
            width: 359
            height: 21
            controller: root.controller
            labelText: qsTr("Name:")
            staticText: root.controller ? root.controller.patientName : ""
            readOnly: true
            maxLength: 200
        }
        GxField {
            objectName: "openingDateField"
            x: 15
            y: 47
            width: 196
            height: 21
            controller: root.controller
            field: "openingDate"
            type: "date"
            labelText: qsTr("Opening date:")
        }
        GxField {
            x: 15
            y: 77
            width: 462
            height: 21
            controller: root.controller
            field: "allergies"
            labelText: qsTr("Allergies:")
            maxLength: 100
        }
    }

    // --- Pregnancy history (2, 113, 482, 181). ---
    GxGroup {
        objectName: "pregnancyHistoryGroup"
        x: 2
        y: 113
        width: 482
        height: 181
        title: qsTr("Pregnancy history")

        GxField {
            x: 15
            y: 21
            width: 261
            height: 21
            controller: root.controller
            field: "obstetricLastMenstruation"
            type: "date"
            labelText: qsTr("Last menstruation date:")
        }
        GxField {
            x: 298
            y: 21
            width: 178
            height: 21
            controller: root.controller
            field: "obstetricDeceasedSiblingCount"
            type: "number"
            labelText: qsTr("Deceased siblings at birth:")
            integerDigits: 2
        }
        GxField {
            x: 15
            y: 49
            width: 231
            height: 21
            controller: root.controller
            field: "obstetricExpectedDelivery"
            type: "date"
            labelText: qsTr("Expected delivery date:")
        }
        GxField {
            x: 252
            y: 47
            width: 103
            height: 21
            controller: root.controller
            field: "obstetricDeliveryCount"
            type: "number"
            labelText: qsTr("Delivery count:")
            integerDigits: 2
        }
        GxField {
            x: 361
            y: 47
            width: 116
            height: 21
            controller: root.controller
            field: "obstetricAbortionCount"
            type: "number"
            labelText: qsTr("Abortion count:")
            integerDigits: 3
        }
        GxField {
            x: 15
            y: 74
            width: 462
            height: 21
            controller: root.controller
            field: "obstetricDeliveryProblems"
            labelText: qsTr("Delivery problems:")
            maxLength: 200
        }
        GxField {
            x: 14
            y: 101
            width: 462
            height: 21
            controller: root.controller
            field: "obstetricGynecologicalDiseases"
            labelText: qsTr("Gynecological diseases:")
            maxLength: 200
        }
        GxField {
            x: 15
            y: 128
            width: 462
            height: 21
            controller: root.controller
            field: "obstetricBarrierMethods"
            labelText: qsTr("Barrier methods:")
            maxLength: 200
        }
        GxCheckField {
            x: 15
            y: 155
            controller: root.controller
            field: "obstetricHivWoman"
            text: qsTr("HIV (woman)")
        }
        GxCheckField {
            x: 125
            y: 155
            controller: root.controller
            field: "obstetricHivMan"
            text: qsTr("HIV (man)")
        }
        GxField {
            x: 252
            y: 154
            width: 224
            height: 21
            controller: root.controller
            field: "obstetricSerologies"
            labelText: qsTr("Serologies:")
            maxLength: 100
        }
    }

    // --- Medical history (2, 295, 482, 263). ---
    GxGroup {
        objectName: "medicalHistoryGroup"
        x: 2
        y: 295
        width: 482
        height: 263
        title: qsTr("Pregnant woman history")

        GxLabel {
            x: 15
            y: 17
            width: 138
            height: 17
            text: qsTr("Chronic diseases")
        }
        GxCheckField {
            x: 15
            y: 42
            controller: root.controller
            field: "medicalDiabetes"
            text: qsTr("Diabetes")
        }
        GxField {
            x: 99
            y: 39
            width: 378
            height: 21
            controller: root.controller
            field: "medicalDiabetesTreatment"
            labelText: qsTr("Treatment:")
            maxLength: 100
        }
        GxCheckField {
            x: 15
            y: 73
            controller: root.controller
            field: "medicalHepatitis"
            text: qsTr("Hepatitis")
        }
        GxField {
            x: 99
            y: 73
            width: 73
            height: 21
            controller: root.controller
            field: "medicalHepatitisType"
            labelText: qsTr("Type:")
            maxLength: 3
        }
        GxField {
            x: 190
            y: 73
            width: 286
            height: 21
            controller: root.controller
            field: "medicalHepatitisTreatment"
            labelText: qsTr("Treatment:")
            maxLength: 100
        }
        GxCheckField {
            x: 15
            y: 105
            controller: root.controller
            field: "medicalHypertension"
            text: "HTA"
        }
        GxField {
            x: 99
            y: 105
            width: 378
            height: 21
            controller: root.controller
            field: "medicalHypertensionTreatment"
            labelText: qsTr("Treatment:")
            maxLength: 100
        }
        GxField {
            x: 15
            y: 133
            width: 462
            height: 21
            controller: root.controller
            field: "medicalOtherDiseases"
            labelText: qsTr("Other diseases:")
            maxLength: 200
        }
        GxField {
            x: 15
            y: 175
            width: 462
            height: 21
            controller: root.controller
            field: "medicalDisabilities"
            labelText: qsTr("Disabilities:")
            maxLength: 200
        }
        GxField {
            x: 15
            y: 214
            width: 462
            height: 38
            controller: root.controller
            field: "medicalConflicts"
            labelText: qsTr("Conflicts, relationships, family problems:")
            maxLength: 200
        }
    }

    // --- Housing (492, 2, 514, 87). ---
    GxGroup {
        objectName: "housingGroup"
        x: 492
        y: 2
        width: 514
        height: 87
        title: qsTr("Housing")

        GxCheckField {
            x: 17
            y: 18
            controller: root.controller
            field: "housingPotableWater"
            text: qsTr("Potable water")
        }
        GxCheckField {
            x: 129
            y: 18
            controller: root.controller
            field: "housingLatrine"
            text: qsTr("Latrine")
        }
        GxCheckField {
            x: 316
            y: 18
            controller: root.controller
            field: "housingMosquitoNetParents"
            text: qsTr("Parents mosquito net")
        }
        GxCheckField {
            x: 449
            y: 18
            controller: root.controller
            field: "housingMosquitoNetUseParents"
            text: qsTr("Parents net use")
        }
        GxCheckField {
            x: 17
            y: 38
            controller: root.controller
            field: "housingTreatedWater"
            text: qsTr("Treated water")
        }
        GxCheckField {
            x: 129
            y: 38
            controller: root.controller
            field: "housingDomesticHygiene"
            text: qsTr("Domestic hygiene")
        }
        GxCheckField {
            x: 316
            y: 38
            controller: root.controller
            field: "housingMosquitoNetChildren"
            text: qsTr("Children mosquito net")
        }
        GxCheckField {
            x: 449
            y: 38
            controller: root.controller
            field: "housingMosquitoNetUseChildren"
            text: qsTr("Children net use")
        }
        GxField {
            x: 17
            y: 57
            width: 284
            height: 23
            controller: root.controller
            field: "housingDomesticAnimals"
            type: "combo"
            labelText: qsTr("Domestic animals:")
            options: root.controller ? root.controller.domesticAnimalsOptions() : []
        }
    }

    // --- Physical examination (492, 95, 514, 126). ---
    GxGroup {
        objectName: "examGroup"
        x: 492
        y: 95
        width: 514
        height: 126
        title: qsTr("Physical examination")

        GxField {
            x: 6
            y: 22
            width: 105
            height: 21
            controller: root.controller
            field: "physicalExamWeight"
            type: "number"
            labelText: qsTr("Weight (Kg):")
            integerDigits: 3
            decimalDigits: 1
        }
        GxField {
            x: 121
            y: 22
            width: 81
            height: 21
            controller: root.controller
            field: "physicalExamTemperature"
            type: "number"
            labelText: qsTr("Temperature (ºC):")
            integerDigits: 2
            decimalDigits: 1
        }
        GxField {
            x: 6
            y: 47
            width: 110
            height: 21
            controller: root.controller
            field: "physicalExamHeight"
            type: "number"
            labelText: qsTr("Height (cm):")
            integerDigits: 3
        }
        GxField {
            x: 6
            y: 74
            width: 109
            height: 21
            controller: root.controller
            field: "physicalExamAbdominalPerimeter"
            type: "number"
            labelText: qsTr("Abdominal P. (cm):")
            integerDigits: 3
        }
        GxField {
            x: 6
            y: 99
            width: 150
            height: 21
            controller: root.controller
            field: "physicalExamVomiting"
            labelText: qsTr("Vomiting:")
            maxLength: 100
        }
        GxField {
            x: 167
            y: 99
            width: 166
            height: 21
            controller: root.controller
            field: "physicalExamFetalAuscultation"
            labelText: qsTr("Fetal auscultation:")
            maxLength: 100
        }
        GxField {
            x: 342
            y: 99
            width: 166
            height: 21
            controller: root.controller
            field: "physicalExamFetalPosition"
            labelText: qsTr("Fetal position:")
            maxLength: 100
        }

        // Inner groups: their titles share the line of the outer one.
        GxGroup {
            objectName: "uterineGroup"
            x: 208
            y: 11
            width: 122
            height: 66
            title: qsTr("Uterine height")

            GxField {
                x: 6
                y: 17
                width: 110
                height: 21
                controller: root.controller
                field: "physicalExamUterineHeightWeeks"
                type: "number"
                labelText: qsTr("Week:")
                integerDigits: 2
            }
            GxField {
                x: 6
                y: 40
                width: 110
                height: 21
                controller: root.controller
                field: "physicalExamUterineHeightCm"
                type: "number"
                labelText: qsTr("Centimeters:")
                integerDigits: 2
            }
        }
        GxGroup {
            objectName: "urineGroup"
            x: 337
            y: 11
            width: 97
            height: 66
            title: qsTr("Urine")

            GxCheckField {
                x: 9
                y: 21
                controller: root.controller
                field: "physicalExamLeukocytosis"
                text: qsTr("Leukocytosis")
            }
            GxCheckField {
                x: 9
                y: 40
                controller: root.controller
                field: "physicalExamProteinuria"
                text: qsTr("Proteinuria")
            }
        }
        GxGroup {
            objectName: "edemasGroup"
            x: 440
            y: 11
            width: 68
            height: 83
            title: qsTr("Edemas")

            GxCheckField {
                x: 9
                y: 19
                controller: root.controller
                field: "physicalExamMmi"
                text: "MMI"
            }
            GxCheckField {
                x: 9
                y: 39
                controller: root.controller
                field: "physicalExamFace"
                text: qsTr("Face")
            }
            GxCheckField {
                x: 9
                y: 59
                controller: root.controller
                field: "physicalExamGeneral"
                text: qsTr("General")
            }
        }
    }

    // --- Treatments (492, 223, 514, 295): a titleless box holding the two
    // treatment tabs, like the QTabWidget of the Widgets window.
    GxGroup {
        objectName: "treatmentsGroup"
        x: 492
        y: 223
        width: 514
        height: 295
        title: ""

        Item {
            id: tabWidget
            x: 7
            y: 16
            width: 501
            height: 271

            // QTabBar look (Fusion): each tab as wide as its caption, the
            // selected one 2 px taller and open towards the page, the others
            // closed by the page frame line.
            // A plain row rather than a TabBar: the TabBar list view shifts
            // and stretches its buttons, and these tabs need exact geometry.
            Item {
                id: treatmentTabs
                objectName: "treatmentTabs"
                property int currentIndex: 0
                // Above the page frame: the selected tab hides its top line.
                z: 1
                width: parent.width
                height: 25
                Row {
                    // Neighbouring tabs share their border line.
                    spacing: -1
                    TreatmentTab {
                        index: 0
                        text: qsTr("Previous treatment")
                    }
                    TreatmentTab {
                        index: 1
                        text: qsTr("Current treatments")
                    }
                }
            }

            // Page frame of the tab widget; the page content sits 2 px inside
            // it, where the QTabWidget places its stacked page.
            Rectangle {
                objectName: "treatmentPageFrame"
                x: 0
                y: 24
                width: tabWidget.width
                height: tabWidget.height - 25
                color: Theme.tabPage
                border.color: Theme.inputBorder
                border.width: 1

                Item {
                    x: 2
                    y: 2
                    width: parent.width - 4
                    height: parent.height - 3

                    StackLayout {
                        anchors.fill: parent
                        currentIndex: treatmentTabs.currentIndex

                        // Previous treatment: five rows of date, medication, dose.
                        Item {
                            objectName: "previousTreatmentPage"

                            GxLabel {
                                x: 11
                                y: 3
                                width: 327
                                height: 52
                                text: qsTr("MALARIA PROPHYLAXIS, DEWORMING, VITAMIN A, FOLIC ACID, IRON")
                            }
                            Repeater {
                                model: root.previousRows
                                delegate: Item {
                                    id: previousRow
                                    required property var modelData
                                    required property int index
                                    x: 0
                                    y: previousRow.modelData.y
                                    width: parent ? parent.width : 0
                                    height: previousRow.modelData.h

                                    GxField {
                                        x: 5
                                        width: 100
                                        height: previousRow.modelData.h
                                        controller: root.controller
                                        field: previousRow.modelData.date
                                        type: "date"
                                        labelPosition: previousRow.index === 0 ? "above" : "left"
                                        labelText: previousRow.index === 0 ? qsTr("Date:") : ""
                                    }
                                    GxField {
                                        x: 111
                                        width: 179
                                        height: previousRow.modelData.h
                                        controller: root.controller
                                        field: previousRow.modelData.medication
                                        labelPosition: previousRow.index === 0 ? "above" : "left"
                                        labelText: previousRow.index === 0 ? qsTr("Medication:") : ""
                                        maxLength: 100
                                    }
                                    GxField {
                                        x: 296
                                        width: 183
                                        height: previousRow.modelData.h
                                        controller: root.controller
                                        field: previousRow.modelData.dose
                                        labelPosition: previousRow.index === 0 ? "above" : "left"
                                        labelText: previousRow.index === 0 ? qsTr("Dose:") : ""
                                        maxLength: 100
                                    }
                                }
                            }
                        }

                        // Current treatments: seven rows of dose, medication and
                        // next dose; the caption sits on the left of the row.
                        Item {
                            objectName: "currentTreatmentPage"

                            Repeater {
                                model: root.currentRows
                                delegate: Item {
                                    id: currentRow
                                    required property var modelData
                                    required property int index
                                    x: 0
                                    y: 0
                                    width: parent ? parent.width : 0
                                    height: parent ? parent.height : 0

                                    GxLabel {
                                        x: currentRow.modelData.labelX
                                        y: currentRow.modelData.labelY
                                        width: currentRow.modelData.labelW
                                        height: 17
                                        text: currentRow.modelData.caption
                                    }
                                    GxField {
                                        x: 141
                                        y: currentRow.modelData.fieldY
                                        width: 100
                                        height: currentRow.modelData.fieldH
                                        controller: root.controller
                                        field: currentRow.modelData.fixed ? "" : currentRow.modelData.fields[0]
                                        staticText: currentRow.modelData.fixed ? currentRow.modelData.values[0] : ""
                                        readOnly: currentRow.modelData.fixed
                                        labelPosition: currentRow.index === 0 ? "above" : "left"
                                        labelText: currentRow.index === 0 ? qsTr("Dose:") : ""
                                        maxLength: 50
                                    }
                                    GxField {
                                        x: 240
                                        y: currentRow.modelData.fieldY
                                        width: 109
                                        height: currentRow.modelData.fieldH
                                        controller: root.controller
                                        field: currentRow.modelData.fixed ? "" : currentRow.modelData.fields[1]
                                        staticText: currentRow.modelData.fixed ? currentRow.modelData.values[1] : ""
                                        readOnly: currentRow.modelData.fixed
                                        labelPosition: currentRow.index === 0 ? "above" : "left"
                                        labelText: currentRow.index === 0 ? qsTr("Medication:") : ""
                                        maxLength: 50
                                    }
                                    GxField {
                                        x: 348
                                        y: currentRow.modelData.fieldY
                                        width: 127
                                        height: currentRow.modelData.fieldH
                                        controller: root.controller
                                        field: currentRow.modelData.fixed ? "" : currentRow.modelData.fields[2]
                                        staticText: currentRow.modelData.fixed ? currentRow.modelData.values[2] : ""
                                        readOnly: currentRow.modelData.fixed
                                        labelPosition: currentRow.index === 0 ? "above" : "left"
                                        labelText: currentRow.index === 0 ? qsTr("Next dose:") : ""
                                        maxLength: 50
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    component TreatmentTab: TabButton {
        id: tabButton
        required property int index
        width: implicitWidth
        height: 25
        z: checked ? 1 : 0
        checked: treatmentTabs.currentIndex === index
        leftPadding: 13
        rightPadding: 13
        topPadding: 0
        bottomPadding: 1
        onClicked: treatmentTabs.currentIndex = index
        // Painted by a plain child rather than the background: the
        // control re-positions its background item on state changes.
        background: null
        Item {
            z: -1
            width: tabButton.width
            height: tabButton.height
            Rectangle {
                y: tabButton.checked ? 0 : 2
                width: parent.width
                height: tabButton.checked ? parent.height : parent.height - 3
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: tabButton.checked ? Theme.tabTop : Theme.tabInactiveTop
                    }
                    GradientStop {
                        position: 1.0
                        color: tabButton.checked ? Theme.tabPage : Theme.tabInactiveBottom
                    }
                }
            }
            Rectangle {
                y: tabButton.checked ? 0 : 2
                width: parent.width
                height: 1
                color: tabButton.checked ? Theme.inputBorder : Theme.checkBorder
            }
            Rectangle {
                x: 1
                y: tabButton.checked ? 1 : 3
                width: parent.width - 2
                height: 1
                color: tabButton.checked ? Theme.tabTopLine : Theme.tabInactiveTopLine
            }
            Rectangle {
                y: tabButton.checked ? 0 : 2
                width: 1
                height: tabButton.checked ? parent.height : parent.height - 3
                color: tabButton.checked ? Theme.inputBorder : Theme.checkBorder
            }
            Rectangle {
                anchors.right: parent.right
                y: tabButton.checked ? 0 : 2
                width: 1
                height: tabButton.checked ? parent.height : parent.height - 3
                color: tabButton.checked ? Theme.inputBorder : Theme.checkBorder
            }
        }
        contentItem: Text {
            // Unselected captions follow their tab 2 px down.
            topPadding: tabButton.checked ? 1 : 3
            text: tabButton.text
            color: Theme.windowText
            font: tabButton.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
