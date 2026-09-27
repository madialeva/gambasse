pragma ComponentBehavior: Bound
import QtQuick
import Gambasse

// Pediatric history form (Widgets PediatricHistoryWindow): the seven groups,
// the four previous-treatment rows and the vaccine grid, transcribed from the
// .cpp layout (absolute coordinates inside the 1008x561 content area, so the
// coordinates below are group-relative). Every control is bound by name to
// PediatricHistoryController.
Item {
    id: root

    property HistoryController controller: null

    // --- Personal data (2, 2, 482, 116). ---
    GxGroup {
        objectName: "personalGroup"
        x: 2
        y: 2
        width: 482
        height: 116
        title: qsTr("Personal data")

        GxField {
            objectName: "nameField"
            x: 15
            y: 29
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
            y: 58
            width: 222
            height: 21
            controller: root.controller
            field: "openingDate"
            type: "date"
            labelText: qsTr("Opening date:")
        }
        GxField {
            x: 15
            y: 88
            width: 462
            height: 21
            controller: root.controller
            field: "allergies"
            labelText: qsTr("Allergies:")
            maxLength: 100
        }
    }

    // --- Birth history (2, 120, 482, 108). ---
    GxGroup {
        objectName: "birthHistoryGroup"
        x: 2
        y: 120
        width: 482
        height: 108
        title: qsTr("Birth history")

        GxField {
            x: 13
            y: 26
            width: 148
            height: 21
            controller: root.controller
            field: "neonatalWeight"
            type: "number"
            labelText: qsTr("Weight (grams):")
            integerDigits: 5
        }
        GxField {
            x: 181
            y: 26
            width: 123
            height: 21
            controller: root.controller
            field: "neonatalHeight"
            type: "number"
            labelText: qsTr("Height (cm):")
            integerDigits: 3
        }
        GxField {
            x: 15
            y: 53
            width: 462
            height: 21
            controller: root.controller
            field: "neonatalDeliveryIncidents"
            labelText: qsTr("Delivery incidents:")
            maxLength: 100
        }
        GxField {
            x: 15
            y: 80
            width: 462
            height: 21
            controller: root.controller
            field: "neonatalMalformations"
            labelText: qsTr("Malformations or disabilities:")
            maxLength: 100
        }
    }

    // --- Child history (2, 233, 482, 129). ---
    GxGroup {
        objectName: "childHistoryGroup"
        x: 2
        y: 233
        width: 482
        height: 129
        title: qsTr("Child history")

        GxLabel {
            x: 15
            y: 24
            width: 138
            height: 17
            text: qsTr("Chronic diseases")
        }
        GxField {
            x: 15
            y: 47
            width: 461
            height: 21
            controller: root.controller
            field: "clinicalDiseaseType"
            labelText: qsTr("Disease type:")
            maxLength: 3
        }
        GxField {
            x: 15
            y: 74
            width: 461
            height: 21
            controller: root.controller
            field: "clinicalDiseaseTreatment"
            labelText: qsTr("Treatment:")
            maxLength: 100
        }
        GxField {
            x: 15
            y: 101
            width: 462
            height: 21
            controller: root.controller
            field: "otherDiseases"
            labelText: qsTr("Other diseases:")
            maxLength: 200
        }
    }

    // --- Housing (2, 368, 482, 174). ---
    GxGroup {
        objectName: "housingGroup"
        x: 2
        y: 368
        width: 482
        height: 174
        title: qsTr("Housing")

        GxCheckField {
            x: 15
            y: 31
            controller: root.controller
            field: "latrine"
            text: qsTr("Latrine")
        }
        GxCheckField {
            x: 175
            y: 31
            controller: root.controller
            field: "mosquitoNet"
            text: qsTr("Mosquito net")
        }
        GxCheckField {
            x: 293
            y: 31
            controller: root.controller
            field: "netUse"
            text: qsTr("Net use")
        }
        GxCheckField {
            x: 15
            y: 55
            controller: root.controller
            field: "domesticHygiene"
            text: qsTr("Domestic hygiene")
        }
        GxField {
            x: 175
            y: 55
            width: 284
            height: 23
            controller: root.controller
            field: "domesticAnimals"
            type: "combo"
            labelText: qsTr("Domestic animals:")
            options: root.controller ? root.controller.domesticAnimalsOptions() : []
        }
        // Single-line composite of 38 px in Widgets: it only gives the row
        // extra height, it is not a text area.
        GxField {
            x: 14
            y: 91
            width: 462
            height: 38
            controller: root.controller
            field: "conflicts"
            labelText: qsTr("Conflicts, relationships, family problems:")
            maxLength: 200
        }
        GxField {
            x: 15
            y: 143
            width: 461
            height: 21
            controller: root.controller
            field: "economicSituation"
            labelText: qsTr("Economic situation:")
        }
    }

    // --- Feeding (492, 2, 514, 117). ---
    GxGroup {
        objectName: "feedingGroup"
        x: 492
        y: 2
        width: 514
        height: 117
        title: qsTr("Feeding")

        GxCheckField {
            x: 17
            y: 30
            controller: root.controller
            field: "feedingBreastfeeding"
            text: qsTr("Breastfeeding")
        }
        GxCheckField {
            x: 193
            y: 30
            controller: root.controller
            field: "feedingTreatedWater"
            text: qsTr("Treated water WITH:")
        }
        // Widgets input at (328, 31, 103, 21): its empty caption still takes
        // 6 px plus the 6 px spacing, so the field itself starts 12 px in.
        GxField {
            x: 340
            y: 31
            width: 91
            height: 21
            controller: root.controller
            field: "feedingTreatedWaterDetails"
        }
        GxField {
            x: 17
            y: 62
            width: 491
            height: 21
            controller: root.controller
            field: "feedingFeedingProblems"
            labelText: qsTr("Feeding problems:")
            maxLength: 100
        }
        GxField {
            x: 17
            y: 89
            width: 491
            height: 21
            controller: root.controller
            field: "feedingAdditionalFeeding"
            labelText: qsTr("Additional feeding:")
            maxLength: 100
        }
    }

    // --- Previous treatments (492, 121, 514, 173). ---
    // The first row carries its captions above the fields; the other three
    // rows have none (the VB.NET zero-width captions), so their composites
    // span the full width like the Widgets ones.
    GxGroup {
        objectName: "treatmentsGroup"
        x: 492
        y: 121
        width: 514
        height: 173
        title: qsTr("Previous treatments")

        GxField {
            x: 17
            y: 29
            width: 100
            height: 44
            controller: root.controller
            field: "supplementDate1"
            type: "date"
            labelPosition: "above"
            labelText: qsTr("Date:")
        }
        GxField {
            x: 123
            y: 30
            width: 221
            height: 44
            controller: root.controller
            field: "supplementTreatment1"
            type: "combo"
            labelPosition: "above"
            labelText: qsTr("Medication:")
            editable: true
            options: root.controller ? root.controller.treatmentTypeOptions() : []
        }
        GxField {
            x: 17
            y: 78
            width: 100
            height: 22
            controller: root.controller
            field: "supplementDate2"
            type: "date"
        }
        GxField {
            x: 123
            y: 78
            width: 221
            height: 22
            controller: root.controller
            field: "supplementTreatment2"
            type: "combo"
            editable: true
            options: root.controller ? root.controller.treatmentTypeOptions() : []
        }
        GxField {
            x: 17
            y: 106
            width: 100
            height: 22
            controller: root.controller
            field: "supplementDate3"
            type: "date"
        }
        GxField {
            x: 123
            y: 106
            width: 221
            height: 22
            controller: root.controller
            field: "supplementTreatment3"
            type: "combo"
            editable: true
            options: root.controller ? root.controller.treatmentTypeOptions() : []
        }
        GxField {
            x: 17
            y: 134
            width: 100
            height: 22
            controller: root.controller
            field: "supplementDate4"
            type: "date"
        }
        GxField {
            x: 123
            y: 134
            width: 221
            height: 22
            controller: root.controller
            field: "supplementTreatment4"
            type: "combo"
            editable: true
            options: root.controller ? root.controller.treatmentTypeOptions() : []
        }
    }

    // --- Vaccines (492, 297, 371, 169). ---
    // Row captions on the left, checks in three columns, in the same cells as
    // the Widgets grid (the schedule of the VB.NET form).
    GxGroup {
        objectName: "vaccinesGroup"
        x: 492
        y: 297
        width: 371
        height: 169
        title: qsTr("Vaccines")

        GxLabel {
            x: 17
            y: 27
            width: 92
            height: 17
            text: qsTr("Birth")
        }
        GxLabel {
            x: 17
            y: 50
            width: 92
            height: 17
            text: qsTr("6 weeks")
        }
        GxLabel {
            x: 17
            y: 73
            width: 92
            height: 17
            text: qsTr("10 weeks")
        }
        GxLabel {
            x: 17
            y: 96
            width: 92
            height: 17
            text: qsTr("14 weeks")
        }
        GxLabel {
            x: 17
            y: 119
            width: 92
            height: 17
            text: qsTr("9 months")
        }
        GxLabel {
            x: 17
            y: 142
            width: 92
            height: 17
            text: qsTr("10 months")
        }

        GxCheckField {
            x: 115
            y: 26
            controller: root.controller
            field: "vaccineBcg"
            text: "BCG"
        }
        GxCheckField {
            x: 225
            y: 26
            controller: root.controller
            field: "vaccineOpv0"
            text: "OPV-0"
        }
        GxCheckField {
            x: 115
            y: 49
            controller: root.controller
            field: "vaccineDptHib1"
            text: "DPT+HIB-1"
        }
        GxCheckField {
            x: 225
            y: 49
            controller: root.controller
            field: "vaccineOpv1"
            text: "OPV-1"
        }
        GxCheckField {
            x: 302
            y: 49
            controller: root.controller
            field: "vaccineHepB1"
            text: "Hep B1"
        }
        GxCheckField {
            x: 115
            y: 72
            controller: root.controller
            field: "vaccineDptHib2"
            text: "DPT+HIB-2"
        }
        GxCheckField {
            x: 225
            y: 72
            controller: root.controller
            field: "vaccineOpv2"
            text: "OPV-2"
        }
        GxCheckField {
            x: 302
            y: 72
            controller: root.controller
            field: "vaccineHepB2"
            text: "Hep B2"
        }
        GxCheckField {
            x: 115
            y: 95
            controller: root.controller
            field: "vaccineDptHib3"
            text: "DPT+HIB-3"
        }
        GxCheckField {
            x: 225
            y: 95
            controller: root.controller
            field: "vaccineOpv3"
            text: "OPV-3"
        }
        GxCheckField {
            x: 302
            y: 96
            controller: root.controller
            field: "vaccineHepB3"
            text: "Hep B3"
        }
        GxCheckField {
            x: 115
            y: 118
            controller: root.controller
            field: "vaccineMeasles9"
            text: qsTr("Measles")
        }
        GxCheckField {
            x: 115
            y: 141
            controller: root.controller
            field: "vaccineMeasles10"
            text: qsTr("Measles")
        }
    }
}
