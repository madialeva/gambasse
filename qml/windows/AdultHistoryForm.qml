pragma ComponentBehavior: Bound
import QtQuick
import Gambasse

// Adult history form (Widgets AdultHistoryWindow): the four groups and every
// control transcribed from the .cpp layout, which places them with absolute
// coordinates inside the 1008x383 content area. The coordinates below are
// therefore group-relative, exactly like the setGeometry() calls they come
// from; every control is bound by name to AdultHistoryController.
Item {
    id: root

    property HistoryController controller: null

    // --- Personal data (2, 2, 482, 112). ---
    GxGroup {
        objectName: "personalGroup"
        x: 2
        y: 2
        width: 482
        height: 112
        title: qsTr("Personal data")

        GxField {
            objectName: "nameField"
            x: 15
            y: 25
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
            y: 54
            width: 222
            height: 21
            controller: root.controller
            field: "openingDate"
            type: "date"
            labelText: qsTr("Opening date:")
        }
        GxField {
            x: 15
            y: 84
            width: 462
            height: 21
            controller: root.controller
            field: "allergies"
            labelText: qsTr("Allergies:")
            maxLength: 100
        }
    }

    // --- Adult history (2, 120, 482, 260). ---
    GxGroup {
        objectName: "adultHistoryGroup"
        x: 2
        y: 120
        width: 482
        height: 260
        title: qsTr("Adult history")

        GxLabel {
            x: 15
            y: 25
            width: 138
            height: 17
            text: qsTr("Chronic diseases")
        }
        GxCheckField {
            x: 15
            y: 50
            controller: root.controller
            field: "historyDiabetes"
            text: qsTr("Diabetes")
        }
        GxField {
            x: 99
            y: 47
            width: 378
            height: 21
            controller: root.controller
            field: "historyDiabetesTreatment"
            labelText: qsTr("Treatment:")
            maxLength: 100
        }
        GxCheckField {
            x: 15
            y: 81
            controller: root.controller
            field: "historyHepatitis"
            text: qsTr("Hepatitis")
        }
        GxField {
            x: 94
            y: 81
            width: 73
            height: 21
            controller: root.controller
            field: "historyHepatitisType"
            labelText: qsTr("Type:")
            maxLength: 3
        }
        GxField {
            x: 175
            y: 81
            width: 302
            height: 21
            controller: root.controller
            field: "historyHepatitisTreatment"
            labelText: qsTr("Treatment:")
            maxLength: 100
        }
        GxCheckField {
            x: 15
            y: 113
            controller: root.controller
            field: "historyHiv"
            text: "HIV"
        }
        GxField {
            x: 99
            y: 113
            width: 378
            height: 21
            controller: root.controller
            field: "historyHivTreatment"
            labelText: qsTr("Treatment:")
            maxLength: 100
        }
        GxField {
            x: 15
            y: 141
            width: 462
            height: 21
            controller: root.controller
            field: "historyOtherDiseases"
            labelText: qsTr("Other diseases:")
            maxLength: 200
        }
        GxField {
            x: 15
            y: 165
            width: 269
            height: 21
            controller: root.controller
            field: "historyOccupation"
            labelText: qsTr("Occupation:")
            maxLength: 100
        }
        GxField {
            x: 339
            y: 165
            width: 138
            height: 21
            controller: root.controller
            field: "historyChildCount"
            type: "number"
            labelText: qsTr("Number of children:")
            integerDigits: 2
        }
        GxField {
            x: 15
            y: 191
            width: 462
            height: 21
            controller: root.controller
            field: "historyDisabilities"
            labelText: qsTr("Disabilities:")
            maxLength: 200
        }
        // The Widgets composite is a single-line input of 36 px: it only
        // gives those 3 px to the last row, but keeps the same box.
        GxField {
            x: 15
            y: 217
            width: 462
            height: 36
            controller: root.controller
            field: "historyConflicts"
            labelText: qsTr("Conflicts, relationships, family problems:")
            maxLength: 200
        }
    }

    // --- Housing (492, 2, 514, 109). ---
    GxGroup {
        objectName: "housingGroup"
        x: 492
        y: 2
        width: 514
        height: 109
        title: qsTr("Housing")

        GxCheckField {
            x: 17
            y: 31
            controller: root.controller
            field: "housingPotableWater"
            text: qsTr("Potable water")
        }
        GxCheckField {
            x: 175
            y: 31
            controller: root.controller
            field: "housingMosquitoNet"
            text: qsTr("Mosquito net")
        }
        GxCheckField {
            x: 293
            y: 31
            controller: root.controller
            field: "housingMosquitoNetUse"
            text: qsTr("Net use")
        }
        GxCheckField {
            x: 17
            y: 55
            controller: root.controller
            field: "housingLatrine"
            text: qsTr("Latrine")
        }
        GxField {
            x: 175
            y: 55
            width: 284
            height: 23
            controller: root.controller
            field: "housingDomesticAnimals"
            type: "combo"
            labelText: qsTr("Domestic animals:")
            options: root.controller ? root.controller.domesticAnimalsOptions() : []
        }
        GxCheckField {
            x: 17
            y: 79
            controller: root.controller
            field: "housingDomesticHygiene"
            text: qsTr("Domestic hygiene")
        }
        GxCheckField {
            x: 175
            y: 80
            controller: root.controller
            field: "housingSanitaryControl"
            text: qsTr("Sanitary control")
        }
    }

    // --- Vital signs (492, 117, 514, 134). ---
    GxGroup {
        objectName: "vitalsGroup"
        x: 492
        y: 117
        width: 514
        height: 134
        title: qsTr("Vital signs")

        GxField {
            x: 6
            y: 22
            width: 150
            height: 21
            controller: root.controller
            field: "physicalExamWeight"
            type: "number"
            labelText: qsTr("Weight (Kg):")
            integerDigits: 3
            decimalDigits: 1
        }
        GxField {
            x: 164
            y: 22
            width: 185
            height: 21
            controller: root.controller
            field: "physicalExamTemperature"
            type: "number"
            labelText: qsTr("Temperature (ºC):")
            integerDigits: 2
            decimalDigits: 1
        }
        GxField {
            x: 357
            y: 22
            width: 140
            height: 21
            controller: root.controller
            field: "physicalExamHeight"
            type: "number"
            labelText: qsTr("Height (cm):")
            integerDigits: 3
        }
        GxField {
            x: 6
            y: 49
            width: 85
            height: 21
            controller: root.controller
            field: "physicalExamBmi"
            type: "number"
            labelText: qsTr("BMI:")
            integerDigits: 2
            decimalDigits: 2
        }
        GxField {
            x: 97
            y: 49
            width: 165
            height: 21
            controller: root.controller
            field: "physicalExamAbdominalPerimeter"
            type: "number"
            labelText: qsTr("Abdominal P. (cm):")
            integerDigits: 3
        }
        GxField {
            x: 6
            y: 76
            width: 212
            height: 21
            controller: root.controller
            field: "physicalExamConsciousness"
            labelText: qsTr("Consciousness:")
            maxLength: 100
        }
        GxField {
            x: 281
            y: 76
            width: 70
            height: 21
            controller: root.controller
            field: "physicalExamHeartRate"
            labelText: qsTr("Heart rate:")
            maxLength: 100
        }
        GxField {
            x: 390
            y: 76
            width: 69
            height: 21
            controller: root.controller
            field: "physicalExamRespiratoryRate"
            labelText: qsTr("Respiratory rate:")
            maxLength: 100
        }
        GxField {
            x: 6
            y: 102
            width: 212
            height: 21
            controller: root.controller
            field: "physicalExamCapillaryGlucose"
            labelText: qsTr("Capillary glucose:")
            maxLength: 100
        }
    }
}
