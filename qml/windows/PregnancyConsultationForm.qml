pragma ComponentBehavior: Bound
import QtQuick
import Gambasse

// Pregnancy consultation form (VB.NET FrmConsultaMullerGravida): every group
// and control transcribed from FrmConsultaMullerGravida.Designer.vb, which
// places them with absolute coordinates inside the 1008x341 content area.
// Coordinates are group-relative, like the designer's; every control is bound
// by name to PregnancyConsultationController, except the three fixed
// treatment rows (iron, folic acid, deworming), shown as read-only text and
// never saved. The four buttons belong to ConsultationWindow.
Item {
    id: root

    property ConsultationController controller: null

    // Treatments given now: the persisted rows plus the three fixed ones,
    // 6 px lower than the designer so the first row clears the title badge.
    readonly property var treatmentRows: [
        {
            "caption": qsTr("Calcium"),
            "labelX": 4,
            "labelY": 36,
            "labelW": 116,
            "fieldY": 34,
            "fieldH": 21,
            "fixed": false,
            "fields": ["supplementCalciumD", "supplementCalciumM", "supplementCalciumPd"]
        },
        {
            "caption": qsTr("Iron"),
            "labelX": 3,
            "labelY": 54,
            "labelW": 117,
            "fieldY": 54,
            "fieldH": 21,
            "fixed": true,
            "values": [qsTr("60 mg a day"), qsTr("Ferrous sulfate"), qsTr("Until birth")]
        },
        {
            "caption": qsTr("Folic acid"),
            "labelX": 4,
            "labelY": 74,
            "labelW": 116,
            "fieldY": 74,
            "fieldH": 21,
            "fixed": true,
            "values": [qsTr("4 mg a day"), qsTr("Vitamin B9"), qsTr("Until the fifth month of pregnancy")]
        },
        {
            "caption": qsTr("Deworming"),
            "labelX": 4,
            "labelY": 97,
            "labelW": 116,
            "fieldY": 94,
            "fieldH": 21,
            "fixed": true,
            "values": [qsTr("2nd or 3rd trimester"), qsTr("Mebendazole"), qsTr("Single dose")]
        },
        {
            "caption": qsTr("Vitamin C"),
            "labelX": 4,
            "labelY": 118,
            "labelW": 116,
            "fieldY": 114,
            "fieldH": 21,
            "fixed": false,
            "fields": ["supplementVitaminCD", "supplementVitaminCM", "supplementVitaminCPd"]
        },
        {
            "caption": qsTr("Other vitamins"),
            "labelX": 3,
            "labelY": 138,
            "labelW": 120,
            "fieldY": 134,
            "fieldH": 21,
            "fixed": false,
            "fields": ["supplementOtherVitaminsD", "supplementOtherVitaminsM", "supplementOtherVitaminsPd"]
        },
        {
            "caption": qsTr("Malaria prophylaxis"),
            "labelX": 4,
            "labelY": 157,
            "labelW": 120,
            "fieldY": 154,
            "fieldH": 21,
            "fixed": false,
            "fields": ["supplementMalariaProphylaxisD", "supplementMalariaProphylaxisM", "supplementMalariaProphylaxisPd"]
        }
    ]
    // Dose, medication and next-dose columns (x, width).
    readonly property var treatmentColumns: [
        {
            "x": 126,
            "w": 100,
            "caption": qsTr("Dose:")
        },
        {
            "x": 225,
            "w": 109,
            "caption": qsTr("Medication:")
        },
        {
            "x": 333,
            "w": 100,
            "caption": qsTr("Next dose:")
        }
    ]

    // --- Personal data (2, 2, 555, 201) with the consultation list. ---
    GxGroup {
        objectName: "personalGroup"
        x: 2
        y: 2
        width: 555
        height: 201
        title: qsTr("Personal data")

        GxField {
            objectName: "nameField"
            x: 15
            y: 19
            width: 323
            height: 21
            controller: root.controller
            labelText: qsTr("Name:")
            staticText: root.controller ? root.controller.patientName : ""
            readOnly: true
            maxLength: 200
        }
        GxField {
            objectName: "dateField"
            x: 359
            y: 19
            width: 188
            height: 21
            controller: root.controller
            labelText: qsTr("Date:")
            staticText: root.controller ? root.controller.dateText : ""
            readOnly: true
        }
        GxGroup {
            objectName: "consultationListGroup"
            x: 7
            y: 43
            width: 544
            height: 152
            nested: true
            title: qsTr("Consultation list")

            ConsultationList {
                x: 6
                y: 21
                width: 533
                height: 125
                controller: root.controller
            }
        }
    }

    // --- Reason (565, 2, 440, 68): the text box takes the area below the
    // title badge, which already shows the caption. ---
    GxGroup {
        objectName: "reasonGroup"
        x: 565
        y: 2
        width: 440
        height: 68
        title: qsTr("Reason for consultation")

        GxField {
            x: 6
            y: 18
            width: 428
            height: 44
            controller: root.controller
            field: "reason"
            type: "multiline"
            maxLength: 500
        }
    }

    // --- Treatments given now (565, 76, 441, 180): 6 px taller than the
    // designer, for the rows moved below the title badge. ---
    GxGroup {
        objectName: "treatmentsGroup"
        x: 565
        y: 76
        width: 441
        height: 180
        title: qsTr("Treatments given now")

        // Column captions above the first row, which keeps the 21 px of the
        // others.
        Repeater {
            model: root.treatmentColumns
            delegate: GxLabel {
                required property var modelData
                x: modelData.x
                y: 17
                width: modelData.w
                height: 17
                text: modelData.caption
            }
        }
        Repeater {
            model: root.treatmentRows
            delegate: Item {
                id: treatmentRow
                required property var modelData
                required property int index
                // Only a coordinate origin for the row's controls: sized zero
                // so it overlaps nothing (the title badge included).
                x: 0
                y: 0
                width: 0
                height: 0

                GxLabel {
                    x: treatmentRow.modelData.labelX
                    y: treatmentRow.modelData.labelY
                    width: treatmentRow.modelData.labelW
                    height: 17
                    text: treatmentRow.modelData.caption
                }
                Repeater {
                    model: root.treatmentColumns
                    delegate: GxField {
                        id: treatmentField
                        required property var modelData
                        required property int index
                        x: treatmentField.modelData.x
                        y: treatmentRow.modelData.fieldY
                        width: treatmentField.modelData.w
                        height: treatmentRow.modelData.fieldH
                        controller: root.controller
                        field: treatmentRow.modelData.fixed ? "" : treatmentRow.modelData.fields[treatmentField.index]
                        staticText: treatmentRow.modelData.fixed ? treatmentRow.modelData.values[treatmentField.index] : ""
                        readOnly: treatmentRow.modelData.fixed
                        maxLength: 50
                    }
                }
            }
        }
    }

    // --- Physical examination (2, 209, 555, 126) and its three nested
    // subgroups, moved left into the free space and the urine and edema ones
    // widened, so their checks are not cut (97 and 68 px in the designer). ---
    GxGroup {
        objectName: "examGroup"
        x: 2
        y: 209
        width: 555
        height: 126
        title: qsTr("Physical examination")

        GxField {
            x: 6
            y: 22
            width: 105
            height: 21
            controller: root.controller
            field: "weight"
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
            field: "temperature"
            type: "number"
            labelText: qsTr("T. (ºC):")
            integerDigits: 3
            decimalDigits: 1
        }
        GxField {
            x: 6
            y: 47
            width: 110
            height: 21
            controller: root.controller
            field: "height"
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
            field: "abdominalPerimeter"
            type: "number"
            labelText: qsTr("A. P. (cm):")
            integerDigits: 3
        }
        GxField {
            x: 6
            y: 99
            width: 150
            height: 21
            controller: root.controller
            field: "vomiting"
            labelText: qsTr("Vomiting:")
            maxLength: 100
        }
        GxField {
            x: 189
            y: 99
            width: 166
            height: 21
            controller: root.controller
            field: "fetalAuscultation"
            labelText: qsTr("Fetal auscultation:")
            maxLength: 100
        }
        GxField {
            x: 383
            y: 99
            width: 166
            height: 21
            controller: root.controller
            field: "fetalPosition"
            labelText: qsTr("Fetal position:")
            maxLength: 100
        }

        GxGroup {
            objectName: "uterineHeightGroup"
            x: 210
            y: 11
            width: 122
            height: 66
            nested: true
            title: qsTr("Uterine height")

            GxField {
                x: 6
                y: 17
                width: 110
                height: 21
                controller: root.controller
                field: "uterineHeightWeeks"
                type: "number"
                labelText: qsTr("Week:")
                integerDigits: 3
            }
            GxField {
                x: 6
                y: 40
                width: 110
                height: 21
                controller: root.controller
                field: "uterineHeightCm"
                type: "number"
                labelText: qsTr("Centimeters:")
                integerDigits: 3
            }
        }

        GxGroup {
            objectName: "urineGroup"
            x: 341
            y: 11
            width: 115
            height: 66
            nested: true
            title: qsTr("Urine")

            GxCheckField {
                x: 9
                y: 21
                controller: root.controller
                field: "leukocytosis"
                text: qsTr("Leukocytosis")
            }
            GxCheckField {
                x: 9
                y: 40
                controller: root.controller
                field: "proteinuria"
                text: qsTr("Proteinuria")
            }
        }

        GxGroup {
            objectName: "edemaGroup"
            x: 465
            y: 11
            width: 84
            height: 83
            nested: true
            title: qsTr("Edemas")

            GxCheckField {
                x: 9
                y: 19
                controller: root.controller
                field: "edemaLegs"
                text: qsTr("MMI")
            }
            GxCheckField {
                x: 9
                y: 39
                controller: root.controller
                field: "edemaFace"
                text: qsTr("Face")
            }
            GxCheckField {
                x: 9
                y: 59
                controller: root.controller
                field: "edemaGeneral"
                text: qsTr("General")
            }
        }
    }
}
