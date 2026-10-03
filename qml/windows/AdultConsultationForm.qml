pragma ComponentBehavior: Bound
import QtQuick
import Gambasse

// Adult consultation form (VB.NET FrmConsultaAdulto): every group and control
// transcribed from FrmConsultaAdulto.Designer.vb, which places them with
// absolute coordinates inside the 1008x561 content area. The coordinates below
// are therefore group-relative, like the designer's; every control is bound by
// name to AdultConsultationController. The four buttons belong to
// ConsultationWindow.
Item {
    id: root

    property ConsultationController controller: null

    // Lesion rows: region and impairment, the first one with its captions
    // above (38 px), the others 20 px apart, like the designer.
    readonly property var lesionRows: [
        {
            y: 18,
            h: 38
        },
        {
            y: 55,
            h: 21
        },
        {
            y: 75,
            h: 21
        },
        {
            y: 95,
            h: 21
        },
        {
            y: 115,
            h: 21
        },
        {
            y: 135,
            h: 21
        },
        {
            y: 155,
            h: 21
        },
        {
            y: 175,
            h: 21
        }
    ]

    // --- Personal data (2, 2, 565, 201) with the consultation list. ---
    GxGroup {
        objectName: "personalGroup"
        x: 2
        y: 2
        width: 565
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
            x: 366
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
            x: 5
            y: 43
            width: 554
            height: 152
            nested: true
            title: qsTr("Consultation list")

            ConsultationList {
                x: 6
                y: 21
                width: 543
                height: 125
                controller: root.controller
            }
        }
    }

    // --- Reason (575, 2, 430, 68). The designer's field repeated the group
    // title as its own caption; the badge already shows it, so the text box
    // takes the whole area below the badge. ---
    GxGroup {
        objectName: "reasonGroup"
        x: 575
        y: 2
        width: 430
        height: 68
        title: qsTr("Reason for consultation")

        GxField {
            x: 6
            y: 18
            width: 420
            height: 44
            controller: root.controller
            field: "reason"
            type: "multiline"
            maxLength: 500
        }
    }

    // --- Lesion (575, 76, 430, 318): body diagram and eight rows of region
    // number and impairment. ---
    GxGroup {
        objectName: "lesionGroup"
        x: 575
        y: 76
        width: 430
        height: 318
        title: qsTr("Lesion")

        // Reference picture with the numbered body regions, zoomed to fit
        // like the PictureBox; 3 px lower than the designer to clear the
        // title badge.
        Item {
            objectName: "bodyDiagram"
            x: 6
            y: 18
            width: 240
            height: 295
            Image {
                anchors.fill: parent
                source: "qrc:/img/body-regions.jpg"
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
            Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.color: Theme.inputBorder
                border.width: 1
            }
        }

        Repeater {
            model: root.lesionRows
            delegate: Item {
                id: lesionRow
                required property var modelData
                required property int index
                x: 0
                y: lesionRow.modelData.y
                width: parent ? parent.width : 0
                height: lesionRow.modelData.h

                GxField {
                    x: 252
                    width: 56
                    height: lesionRow.modelData.h
                    controller: root.controller
                    field: "lesionRegion" + (lesionRow.index + 1)
                    type: "number"
                    integerDigits: 2
                    labelPosition: lesionRow.index === 0 ? "above" : "left"
                    labelText: lesionRow.index === 0 ? qsTr("Region") : ""
                }
                GxField {
                    x: 307
                    width: 120
                    height: lesionRow.modelData.h
                    controller: root.controller
                    field: "lesionImpairment" + (lesionRow.index + 1)
                    labelPosition: lesionRow.index === 0 ? "above" : "left"
                    labelText: lesionRow.index === 0 ? qsTr("Impairment") : ""
                    maxLength: 100
                }
            }
        }
    }

    // --- Physical examination (2, 205, 565, 352) and its seven subgroups,
    // reorganized: the designer's widths (105-166 px, for an 8 pt font) cut
    // the Spanish and Portuguese captions. The top row keeps respiratory,
    // circulatory and skin, wider; the bottom row keeps elimination and
    // health disorders, and a right column stacks endocrine-metabolic over
    // conduct (a check and a field). ---
    GxGroup {
        objectName: "examGroup"
        x: 2
        y: 205
        width: 565
        height: 352
        title: qsTr("Physical examination")

        GxGroup {
            objectName: "respiratoryGroup"
            x: 6
            y: 23
            width: 140
            height: 123
            nested: true
            title: qsTr("Respiratory")

            GxCheckField {
                x: 6
                y: 21
                controller: root.controller
                field: "nasalFlaring"
                text: qsTr("Nasal flaring")
            }
            GxCheckField {
                x: 6
                y: 45
                controller: root.controller
                field: "stridor"
                text: qsTr("Stridor")
            }
            GxCheckField {
                x: 6
                y: 69
                controller: root.controller
                field: "wheezing"
                text: qsTr("Wheezing")
            }
            GxCheckField {
                x: 6
                y: 93
                controller: root.controller
                field: "retraction"
                text: qsTr("Retraction")
            }
        }
        GxGroup {
            objectName: "circulatoryGroup"
            x: 151
            y: 23
            width: 195
            height: 123
            nested: true
            title: qsTr("Circulatory")

            GxCheckField {
                x: 5
                y: 21
                controller: root.controller
                field: "arrhythmia"
                text: qsTr("Arrhythmia")
            }
            GxCheckField {
                x: 5
                y: 45
                controller: root.controller
                field: "capillaryRefill"
                text: qsTr("Capillary refill")
            }
            GxCheckField {
                x: 5
                y: 69
                controller: root.controller
                field: "absentPulse"
                text: qsTr("No radial pulse")
            }
            GxCheckField {
                x: 5
                y: 93
                controller: root.controller
                field: "edema"
                text: qsTr("Edema")
            }
        }
        GxGroup {
            objectName: "skinGroup"
            x: 351
            y: 23
            width: 208
            height: 123
            nested: true
            title: qsTr("Skin and mucosa")

            GxCheckField {
                x: 5
                y: 21
                controller: root.controller
                field: "skinFolds"
                text: qsTr("Skin fold > 2\"")
            }
            GxCheckField {
                x: 5
                y: 45
                controller: root.controller
                field: "paleMucosa"
                text: qsTr("Pale mucosa")
            }
            GxCheckField {
                x: 5
                y: 69
                controller: root.controller
                field: "sunkenEyes"
                text: qsTr("Enophthalmos")
            }
            GxCheckField {
                x: 5
                y: 93
                controller: root.controller
                field: "palePalms"
                text: qsTr("Pale palms")
            }
        }
        GxGroup {
            objectName: "endocrineGroup"
            x: 326
            y: 152
            width: 233
            height: 115
            nested: true
            title: qsTr("Endocrine-metabolic")

            GxCheckField {
                x: 11
                y: 21
                controller: root.controller
                field: "organomegaly"
                text: qsTr("Organomegaly")
            }
            GxField {
                x: 11
                y: 45
                width: 211
                height: 21
                controller: root.controller
                field: "organomegalyDescription"
                maxLength: 50
            }
            GxCheckField {
                x: 11
                y: 69
                controller: root.controller
                field: "lymphadenopathy"
                text: qsTr("Lymphadenopathy")
            }
            GxField {
                x: 11
                y: 90
                width: 211
                height: 21
                controller: root.controller
                field: "lymphadenopathyDescription"
                maxLength: 50
            }
        }
        GxGroup {
            objectName: "eliminationGroup"
            x: 6
            y: 152
            width: 150
            height: 196
            nested: true
            title: qsTr("Elimination")

            GxCheckField {
                x: 6
                y: 21
                controller: root.controller
                field: "secretion"
                text: qsTr("Secretion")
            }
            GxCheckField {
                x: 6
                y: 45
                controller: root.controller
                field: "stoolChanges"
                text: qsTr("Stool changes")
            }
            GxCheckField {
                x: 6
                y: 69
                controller: root.controller
                field: "cough"
                text: qsTr("Cough")
            }
            GxField {
                x: 6
                y: 93
                width: 138
                height: 21
                controller: root.controller
                field: "coughDescription"
                maxLength: 50
            }
            GxCheckField {
                x: 6
                y: 119
                controller: root.controller
                field: "urineChanges"
                text: qsTr("Urine changes")
            }
            GxField {
                x: 6
                y: 144
                width: 138
                height: 21
                controller: root.controller
                field: "urineChangesDescription"
                maxLength: 50
            }
            GxCheckField {
                x: 6
                y: 171
                controller: root.controller
                field: "vomiting"
                text: qsTr("Vomiting")
            }
        }
        GxGroup {
            objectName: "disordersGroup"
            x: 161
            y: 152
            width: 160
            height: 196
            nested: true
            title: qsTr("Health disorders")

            GxField {
                x: 6
                y: 25
                width: 148
                height: 38
                controller: root.controller
                field: "dehydrationDegree"
                labelPosition: "above"
                labelText: qsTr("Dehydration degree:")
                maxLength: 50
            }
            GxField {
                x: 6
                y: 76
                width: 148
                height: 38
                controller: root.controller
                field: "malnutritionDegree"
                labelPosition: "above"
                labelText: qsTr("Malnutrition degree:")
                maxLength: 50
            }
            GxField {
                x: 6
                y: 127
                width: 148
                height: 38
                controller: root.controller
                field: "other"
                labelPosition: "above"
                labelText: qsTr("Others:")
                maxLength: 50
            }
        }
        GxGroup {
            objectName: "conductGroup"
            x: 326
            y: 271
            width: 233
            height: 77
            nested: true
            title: qsTr("Conduct")

            GxCheckField {
                x: 11
                y: 21
                controller: root.controller
                field: "irritable"
                text: qsTr("Irritable")
            }
            GxField {
                x: 8
                y: 45
                width: 217
                height: 21
                controller: root.controller
                field: "otherCode"
                labelText: qsTr("Other:")
                maxLength: 100
            }
        }
    }
}
