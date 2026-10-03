pragma ComponentBehavior: Bound
import QtQuick
import Gambasse

// Pediatric consultation form (VB.NET FrmConsultaPediatrica): every group,
// tab and control transcribed from FrmConsultaPediatrica.Designer.vb, which
// places them with absolute coordinates inside the 1008x561 content area.
// Coordinates are relative to their group or tab page, like the designer's;
// every control is bound by name to PediatricConsultationController, and the
// combos show its translated domain lists. The four buttons belong to
// ConsultationWindow.
Item {
    id: root

    property PediatricConsultationController controller: null
    readonly property var options: root.controller ? root.controller.domainOptions : ({})

    // The domain labels follow the language: the codes stay selected.
    Connections {
        target: InterfaceSettings
        function onLanguageChanged() {
            if (root.controller)
                root.controller.refreshLanguage();
        }
    }

    // Indicated treatment rows: five alike, the combos 21 px apart and the
    // inputs 20 px apart, as in the designer; the column captions go above
    // the first row (see TreatmentPage).
    readonly property var treatmentRows: [
        {
            comboY: 28,
            inputY: 30
        },
        {
            comboY: 49,
            inputY: 50
        },
        {
            comboY: 70,
            inputY: 70
        },
        {
            comboY: 91,
            inputY: 90
        },
        {
            comboY: 112,
            inputY: 110
        }
    ]
    // Columns of the treatment rows: field, caption and geometry.
    readonly property var treatmentColumns: [
        {
            x: 13,
            w: 212,
            caption: qsTr("Active ingredient:")
        },
        {
            x: 231,
            w: 103,
            caption: qsTr("Description:")
        },
        {
            x: 340,
            w: 109,
            caption: qsTr("Route:")
        },
        {
            x: 455,
            w: 69,
            caption: qsTr("Days:")
        },
        {
            x: 530,
            w: 93,
            caption: qsTr("Dose:")
        },
        {
            x: 629,
            w: 80,
            caption: qsTr("Frequency:")
        },
        {
            x: 716,
            w: 86,
            caption: qsTr("Infusion ml/h:")
        }
    ]

    // --- Personal data (2, 2, 567, 147) with the consultation list. ---
    GxGroup {
        objectName: "personalGroup"
        x: 2
        y: 2
        width: 567
        height: 147
        title: qsTr("Personal data")

        GxField {
            objectName: "nameField"
            x: 15
            y: 20
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
            x: 7
            y: 43
            width: 554
            height: 100
            nested: true
            title: qsTr("Consultation list")

            ConsultationList {
                x: 6
                y: 21
                width: 543
                height: 73
                controller: root.controller
            }
        }
    }

    // --- Reason (575, 2, 430, 68): the text box takes the area below the
    // title badge, which already shows the caption. ---
    GxGroup {
        objectName: "reasonGroup"
        x: 575
        y: 2
        width: 430
        height: 68
        title: qsTr("Reason for consultation")

        Field {
            x: 6
            y: 18
            width: 420
            height: 44
            field: "reason"
            type: "multiline"
            maxLength: 500
        }
    }

    // --- Nursing diagnosis and other diagnosis, outside any group. ---
    Combo {
        x: 575
        y: 76
        width: 426
        height: 22
        field: "nursingDiagnosis"
        labelText: qsTr("Nursing diagnosis:")
        options: root.options.nursingDiagnosis
    }
    Field {
        x: 616
        y: 113
        width: 385
        height: 36
        field: "otherDiagnosis"
        type: "multiline"
        labelText: qsTr("Other diagnosis:")
        maxLength: 500
    }

    // --- Physical examination (2, 155, 999, 211): eleven tabs. ---
    GxGroup {
        objectName: "examGroup"
        x: 2
        y: 155
        width: 999
        height: 211
        title: qsTr("Physical examination")

        GxTabView {
            objectName: "examTabs"
            x: 6
            y: 19
            width: 988
            height: 182
            // Eleven captions in 988 px: with the default 13 px the uppercase
            // Portuguese ones overrun the zone.
            tabPadding: 8
            titles: [qsTr("Clinical signs"), qsTr("Respiratory"), qsTr("Elimination"), qsTr("Circulatory"), qsTr("Skin"), qsTr("Eye"), qsTr("Ear"), qsTr("Mouth"), qsTr("Abdomen/pelvis"), qsTr("Neurological"), qsTr("Endocrine")]

            // Clinical signs: redistributed over the page width (the
            // designer left its right third empty), so no caption squeezes
            // its field in any language.
            Item {
                objectName: "clinicalSignsPage"
                Field {
                    x: 18
                    y: 23
                    width: 135
                    height: 21
                    field: "weight"
                    type: "number"
                    labelText: qsTr("Weight (Kg):")
                    integerDigits: 2
                    decimalDigits: 2
                }
                Field {
                    x: 170
                    y: 23
                    width: 135
                    height: 21
                    field: "length"
                    type: "number"
                    labelText: qsTr("Length (cm):")
                    integerDigits: 3
                }
                Combo {
                    x: 322
                    y: 23
                    width: 175
                    height: 22
                    field: "weightRatio"
                    labelText: qsTr("Weight/Height:")
                    options: root.options.ratio
                }
                Combo {
                    x: 514
                    y: 23
                    width: 165
                    height: 22
                    field: "heightRatio"
                    labelText: qsTr("Height/Age:")
                    options: root.options.ratio
                }
                Field {
                    x: 18
                    y: 52
                    width: 135
                    height: 21
                    field: "temperature"
                    type: "number"
                    labelText: qsTr("Temp. (ºC):")
                    integerDigits: 3
                    decimalDigits: 1
                }
                Field {
                    x: 170
                    y: 52
                    width: 150
                    height: 21
                    field: "bloodGlucose"
                    type: "number"
                    labelText: qsTr("Glucose mg/dl:")
                    integerDigits: 3
                }
                Combo {
                    x: 322
                    y: 51
                    width: 175
                    height: 22
                    field: "consciousness"
                    labelText: qsTr("Consciousness:")
                    options: root.options.consciousness
                }
                Combo {
                    x: 514
                    y: 52
                    width: 260
                    height: 22
                    field: "mentalStatus"
                    labelText: qsTr("Mental status:")
                    options: root.options.mentalStatus
                }
                Combo {
                    x: 18
                    y: 83
                    width: 270
                    height: 22
                    field: "armColor"
                    labelText: qsTr("Arm circumference:")
                    options: root.options.armColor
                }
                Combo {
                    x: 308
                    y: 83
                    width: 190
                    height: 22
                    field: "appetiteTest"
                    labelText: qsTr("Appetite test:")
                    options: root.options.test
                }
                Combo {
                    x: 518
                    y: 83
                    width: 190
                    height: 22
                    field: "malariaTest"
                    labelText: qsTr("Malaria test:")
                    options: root.options.test
                }
                Check {
                    x: 20
                    y: 114
                    field: "bradypnea"
                    text: qsTr("Severe bradypnea/apnea")
                }
                Check {
                    x: 222
                    y: 114
                    field: "convulsion"
                    text: qsTr("Convulsion")
                }
                Check {
                    x: 340
                    y: 114
                    field: "drinkingEating"
                    text: qsTr("Cannot drink or eat")
                }
            }

            // Respiratory.
            Item {
                objectName: "respiratoryPage"
                Combo {
                    x: 7
                    y: 7
                    width: 173
                    height: 22
                    field: "retractionType"
                    labelText: qsTr("Retraction:")
                    options: root.options.retractionType
                }
                Combo {
                    x: 187
                    y: 7
                    width: 108
                    height: 22
                    field: "retractionSeverity"
                    options: root.options.severity
                }
                Combo {
                    x: 362
                    y: 7
                    width: 160
                    height: 22
                    field: "tachypnea"
                    labelText: qsTr("Tachypnea:")
                    options: root.options.severity
                }
                Check {
                    x: 542
                    y: 11
                    field: "burning"
                    text: qsTr("Nasal flaring")
                }
                Check {
                    x: 688
                    y: 11
                    field: "grunting"
                    text: qsTr("Expiratory grunting")
                }
                Check {
                    x: 855
                    y: 11
                    field: "runnyNose"
                    text: qsTr("Runny nose")
                }
                Combo {
                    x: 7
                    y: 44
                    width: 141
                    height: 22
                    field: "wheeze"
                    labelText: qsTr("Wheezing:")
                    options: root.options.severity
                }
                Combo {
                    x: 167
                    y: 44
                    width: 137
                    height: 22
                    field: "secretions"
                    labelText: qsTr("Secretions:")
                    options: root.options.secretions
                }
                Combo {
                    x: 323
                    y: 44
                    width: 199
                    height: 22
                    field: "stridor"
                    labelText: qsTr("Inspiratory stridor:")
                    options: root.options.severity
                }
                Check {
                    x: 542
                    y: 47
                    field: "congestion"
                    text: qsTr("Nasal congestion")
                }
                Check {
                    x: 738
                    y: 47
                    field: "tearing"
                    text: qsTr("Tearing")
                }
                Check {
                    x: 855
                    y: 47
                    field: "soreThroat"
                    text: qsTr("Sore throat")
                }
                Check {
                    x: 21
                    y: 82
                    field: "cough"
                    text: qsTr("Cough")
                }
                Field {
                    x: 85
                    y: 82
                    width: 111
                    height: 20
                    field: "coughDays"
                    type: "number"
                    labelText: qsTr("Days:")
                    integerDigits: 2
                }
                Field {
                    x: 210
                    y: 82
                    width: 230
                    height: 21
                    field: "oxygenSaturation"
                    type: "number"
                    labelText: qsTr("Oxygen saturation (%):")
                    integerDigits: 3
                }
            }

            // Elimination.
            Item {
                objectName: "eliminationPage"
                Combo {
                    x: 16
                    y: 21
                    width: 215
                    height: 22
                    field: "stool"
                    labelText: qsTr("Stool:")
                    options: root.options.stool
                }
                Field {
                    x: 251
                    y: 21
                    width: 159
                    height: 21
                    field: "bleeding"
                    labelText: qsTr("Bleeding:")
                    maxLength: 100
                }
                Combo {
                    x: 16
                    y: 68
                    width: 159
                    height: 22
                    field: "urine"
                    labelText: qsTr("Urine:")
                    options: root.options.urine
                }
                Combo {
                    x: 251
                    y: 68
                    width: 159
                    height: 22
                    field: "vomiting"
                    labelText: qsTr("Vomiting:")
                    options: root.options.vomiting
                }
            }

            // Circulatory.
            Item {
                objectName: "circulatoryPage"
                Combo {
                    x: 27
                    y: 16
                    width: 215
                    height: 22
                    field: "capillaryRefill"
                    labelText: qsTr("Capillary refill:")
                    options: root.options.capillaryRefill
                }
                Check {
                    x: 272
                    y: 20
                    field: "absentPulse"
                    text: qsTr("No radial pulse")
                }
                Combo {
                    x: 27
                    y: 54
                    width: 177
                    height: 22
                    field: "cyanosis"
                    labelText: qsTr("Cyanosis:")
                    options: root.options.cyanosis
                }
                Field {
                    x: 272
                    y: 55
                    width: 224
                    height: 21
                    field: "edema"
                    labelText: qsTr("Edemas:")
                    maxLength: 100
                }
                Field {
                    x: 27
                    y: 91
                    width: 103
                    height: 21
                    field: "heartRate"
                    type: "number"
                    labelText: qsTr("HR (bpm):")
                    integerDigits: 3
                }
                Field {
                    x: 272
                    y: 91
                    width: 224
                    height: 21
                    field: "arrhythmia"
                    labelText: qsTr("Arrhythmia:")
                    maxLength: 100
                }
            }

            // Skin.
            Item {
                objectName: "skinPage"
                Check {
                    x: 37
                    y: 17
                    field: "skinFolds"
                    text: qsTr("Skin fold > 2\"")
                }
                Check {
                    x: 210
                    y: 17
                    field: "palePalms"
                    text: qsTr("Pale palms")
                }
                Check {
                    x: 392
                    y: 17
                    field: "drySkin"
                    text: qsTr("Dry skin")
                }
                Field {
                    x: 525
                    y: 17
                    width: 428
                    height: 55
                    field: "bodyLesions"
                    type: "multiline"
                    labelPosition: "above"
                    labelText: qsTr("Body lesions:")
                    maxLength: 500
                }
                Check {
                    x: 37
                    y: 54
                    field: "petechiae"
                    text: qsTr("Petechiae/purpura")
                }
                Check {
                    x: 210
                    y: 54
                    field: "pustules"
                    text: qsTr("Widespread pustules")
                }
                Check {
                    x: 392
                    y: 54
                    field: "jaundice"
                    text: qsTr("Jaundice")
                }
                Combo {
                    x: 37
                    y: 87
                    width: 323
                    height: 22
                    field: "exanthema"
                    labelText: qsTr("Exanthema:")
                    options: root.options.exanthema
                }
                Combo {
                    x: 396
                    y: 87
                    width: 194
                    height: 22
                    field: "infection"
                    labelText: qsTr("Infection:")
                    options: root.options.infection
                }
                Combo {
                    x: 619
                    y: 87
                    width: 124
                    height: 22
                    field: "skinEdema"
                    labelText: qsTr("Edema:")
                    options: root.options.edema
                }
            }

            // Eye.
            Item {
                objectName: "eyePage"
                Combo {
                    x: 39
                    y: 25
                    width: 233
                    height: 22
                    field: "eyeDischarge"
                    labelText: qsTr("Discharge:")
                    options: root.options.eyeDischarge
                }
                Check {
                    x: 39
                    y: 66
                    field: "eyelidEdema"
                    text: qsTr("Eyelid edema")
                }
                Check {
                    x: 196
                    y: 66
                    field: "stuckEyelids"
                    text: qsTr("Stuck eyelids")
                }
                Check {
                    x: 39
                    y: 102
                    field: "redConjunctiva"
                    text: qsTr("Red conjunctiva")
                }
                Check {
                    x: 196
                    y: 102
                    field: "paleConjunctiva"
                    text: qsTr("Pale conjunctiva")
                }
                Check {
                    x: 341
                    y: 102
                    field: "sunkenEyes"
                    text: qsTr("Enophthalmos")
                }
            }

            // Ear.
            Item {
                objectName: "earPage"
                Check {
                    x: 30
                    y: 17
                    field: "swellingBehindEar"
                    text: qsTr("Swelling behind the ear")
                }
                Combo {
                    x: 269
                    y: 17
                    width: 153
                    height: 22
                    field: "earPain"
                    labelText: qsTr("Pain:")
                    options: root.options.earPain
                }
                Combo {
                    x: 30
                    y: 50
                    width: 222
                    height: 22
                    field: "earDischarge"
                    labelText: qsTr("Discharge:")
                    options: root.options.earDischarge
                }
                Check {
                    x: 269
                    y: 54
                    field: "itchyEarCanal"
                    text: qsTr("Itchy ear canal")
                }
                Combo {
                    x: 30
                    y: 88
                    width: 222
                    height: 22
                    field: "otoscopy"
                    labelText: qsTr("Tympanic otoscopy:")
                    options: root.options.otoscopy
                }
            }

            // Mouth.
            Item {
                objectName: "mouthPage"
                GxLabel {
                    x: 20
                    y: 18
                    width: 172
                    height: 17
                    text: qsTr("Erythematous tonsils with")
                }
                Combo {
                    x: 196
                    y: 18
                    width: 225
                    height: 22
                    field: "tonsils"
                    options: root.options.tonsils
                }
                Combo {
                    x: 438
                    y: 18
                    width: 209
                    height: 22
                    field: "mouthPain"
                    labelText: qsTr("Sore throat:")
                    options: root.options.mouthPain
                }
                Field {
                    x: 676
                    y: 18
                    width: 189
                    height: 21
                    field: "lesionType"
                    labelText: qsTr("Lesion type:")
                    maxLength: 100
                }
                Check {
                    x: 20
                    y: 54
                    field: "koplikSpots"
                    text: qsTr("Koplik spots")
                }
                Check {
                    x: 188
                    y: 54
                    field: "whitePatches"
                    text: qsTr("White patches")
                }
                Check {
                    x: 319
                    y: 54
                    field: "redTongue"
                    text: qsTr("Red, swollen tongue")
                }
                Check {
                    x: 20
                    y: 85
                    field: "palatalPetechiae"
                    text: qsTr("Palatal petechiae")
                }
                Check {
                    x: 188
                    y: 85
                    field: "cervicalNodes"
                    text: qsTr("Enlarged neck nodes")
                }
            }

            // Abdomen / pelvis.
            Item {
                objectName: "abdomenPage"
                Combo {
                    x: 21
                    y: 26
                    width: 307
                    height: 22
                    field: "abdominalPainSigns"
                    labelText: qsTr("Pathology signs:")
                    options: root.options.abdominalPainSigns
                }
            }

            // Neurological.
            Item {
                objectName: "neurologicalPage"
                Check {
                    x: 27
                    y: 21
                    field: "severeHeadache"
                    text: qsTr("Severe headache")
                }
                Check {
                    x: 174
                    y: 21
                    field: "photophobia"
                    text: qsTr("Photophobia")
                }
                Check {
                    x: 276
                    y: 21
                    field: "brudzinskiSign"
                    text: qsTr("Brudzinski sign")
                }
                Check {
                    x: 448
                    y: 21
                    field: "kernigSign"
                    text: qsTr("Kernig sign")
                }
                Check {
                    x: 27
                    y: 66
                    field: "neckStiffness"
                    text: qsTr("Neck stiffness")
                }
                Check {
                    x: 174
                    y: 66
                    field: "bulgingFontanelle"
                    text: qsTr("Bulging fontanelle")
                }
                Check {
                    x: 340
                    y: 66
                    field: "developmentalProblems"
                    text: qsTr("Developmental problems")
                }
                // 40 px right of the designer, clear of the caption of the
                // check before it; the right edge stays at 858.
                Field {
                    x: 540
                    y: 64
                    width: 318
                    height: 21
                    field: "developmentalProblemsDescription"
                    maxLength: 100
                }
            }

            // Endocrine.
            Item {
                objectName: "endocrinePage"
                Check {
                    x: 34
                    y: 23
                    field: "excessiveHunger"
                    text: qsTr("Excessive hunger")
                }
                Check {
                    x: 196
                    y: 23
                    field: "excessiveThirst"
                    text: qsTr("Excessive thirst")
                }
                Check {
                    x: 330
                    y: 23
                    field: "asthenia"
                    text: qsTr("Asthenia")
                }
                Field {
                    x: 445
                    y: 23
                    width: 509
                    height: 21
                    field: "lymphadenopathy"
                    labelText: qsTr("Lymphadenopathy:")
                    maxLength: 50
                }
                Field {
                    x: 34
                    y: 58
                    width: 287
                    height: 21
                    field: "rapidWeightLoss"
                    labelText: qsTr("Rapid weight loss (days/kg):")
                    maxLength: 100
                }
                Field {
                    x: 352
                    y: 58
                    width: 383
                    height: 21
                    field: "breathOdor"
                    labelText: qsTr("Breath odor:")
                    maxLength: 100
                }
            }
        }
    }

    // --- Treatments and recommendations (12, 380, 820, 173): three tabs. ---
    GxTabView {
        objectName: "treatmentTabs"
        x: 12
        y: 380
        width: 820
        height: 173
        titles: [qsTr("Indicated treatments (1-5)"), qsTr("Indicated treatments (6-10)"), qsTr("Child care recommendations")]

        TreatmentPage {
            objectName: "treatments1Page"
            first: 1
            dx: 0
        }
        TreatmentPage {
            objectName: "treatments6Page"
            first: 6
            dx: -1
        }
        Item {
            objectName: "recommendationsPage"
            Repeater {
                model: 5
                delegate: Combo {
                    required property int index
                    x: 16
                    y: 5 + 28 * index
                    width: 705
                    height: 23
                    field: "recommendation" + (index + 1)
                    options: root.options.careRecommendation
                }
            }
        }
    }

    // Five treatment rows from row `first`, under a row of column captions;
    // the second page sits 1 px left of the first in the designer. The
    // captions are labels of their own, so the first row keeps the 22 px of
    // the others instead of sharing its box with a caption.
    component TreatmentPage: Item {
        id: page
        property int first: 1
        property int dx: 0

        Repeater {
            model: root.treatmentColumns
            delegate: GxLabel {
                required property var modelData
                x: modelData.x + page.dx
                y: 8
                width: modelData.w
                height: 17
                text: modelData.caption
            }
        }

        Repeater {
            model: root.treatmentRows
            delegate: Item {
                id: row
                required property var modelData
                required property int index
                readonly property string n: String(page.first + row.index)
                // Only a coordinate origin for the row's controls.
                width: 0
                height: 0

                Combo {
                    x: 13 + page.dx
                    y: row.modelData.comboY
                    width: 212
                    height: 22
                    field: "treatmentMedication" + row.n
                    options: root.options.activeIngredient
                }
                Field {
                    x: 231 + page.dx
                    y: row.modelData.inputY
                    width: 103
                    height: 21
                    field: "treatmentDescription" + row.n
                    maxLength: 100
                }
                Combo {
                    x: 340 + page.dx
                    y: row.modelData.comboY
                    width: 109
                    height: 22
                    field: "treatmentRoute" + row.n
                    options: root.options.administrationRoute
                }
                Combo {
                    x: 455 + page.dx
                    y: row.modelData.comboY
                    width: 69
                    height: 22
                    field: "treatmentDays" + row.n
                    options: root.options.administrationDays
                }
                Field {
                    x: 530 + page.dx
                    y: row.modelData.inputY
                    width: 93
                    height: 21
                    field: "treatmentDose" + row.n
                    maxLength: 100
                }
                Field {
                    x: 629 + page.dx
                    y: row.modelData.inputY
                    width: 80
                    height: 21
                    field: "treatmentFrequency" + row.n
                    maxLength: 100
                }
                Field {
                    x: 716 + page.dx
                    y: row.modelData.inputY
                    width: 86
                    height: 21
                    field: "treatmentPerfusion" + row.n
                    type: "number"
                    integerDigits: 5
                }
            }
        }
    }

    // The form's controls, bound to the controller.
    component Field: GxField {
        controller: root.controller
    }
    component Combo: GxField {
        controller: root.controller
        type: "combo"
    }
    component Check: GxCheckField {
        controller: root.controller
    }
}
