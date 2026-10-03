cmake_minimum_required(VERSION 3.21)

project(Gambasse VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

# Qt 6.6 everywhere (development machines, Windows kit and CI): no code paths
# for other versions. The setup also enables AUTOMOC and every Qt policy up to
# 6.6 (QTP0001: the QML module under the standard qrc:/qt/qml/ prefix).
find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Sql Qml Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.6)

include(CTest)

# --- Core: data access and business rules -----------------------------------
# QtCore/QtSql only, no UI: shared by the QML module and the GUI-less tests.
qt_add_library(GambasseCore STATIC
    src/Paths.h
    src/data/common/Database.cpp
    src/data/common/Database.h
    src/data/common/ConsultationTable.h
    src/data/common/DatabaseConsultations.cpp
    src/data/common/SchemaMigrator.cpp
    src/data/common/SchemaMigrator.h
    src/data/model/AdultConsultation.h
    src/data/model/AdultHistory.h
    src/data/model/Consultation.h
    src/data/model/Patient.h
    src/data/model/PatientsModel.cpp
    src/data/model/PatientsModel.h
    src/data/model/PediatricConsultation.h
    src/data/model/PediatricHistory.h
    src/data/model/PregnancyConsultation.h
    src/data/model/PregnancyHistory.h
    src/logic/AdultHistoryService.cpp
    src/logic/AdultHistoryService.h
    src/logic/AppSettings.cpp
    src/logic/AppSettings.h
    src/logic/ClinicalContext.cpp
    src/logic/ClinicalContext.h
    src/logic/ConsultationService.h
    src/logic/PatientService.cpp
    src/logic/PatientService.h
    src/logic/PediatricConsultationLabels.cpp
    src/logic/PediatricConsultationLabels.h
    src/logic/PediatricHistoryLabels.cpp
    src/logic/PediatricHistoryLabels.h
    src/logic/PediatricHistoryService.cpp
    src/logic/PediatricHistoryService.h
    src/logic/PregnancyHistoryService.cpp
    src/logic/PregnancyHistoryService.h
    src/logic/PhotoManager.cpp
    src/logic/PhotoManager.h
    src/logic/SexLabel.cpp
    src/logic/SexLabel.h
)
target_include_directories(GambasseCore PUBLIC src)
target_link_libraries(GambasseCore PUBLIC Qt6::Core Qt6::Sql)

# --- QML module `Gambasse` ----------------------------------------------------
# Views (QML_FILES) plus the C++ types they use (SOURCES): the controllers and
# settings are registered declaratively (QML_ELEMENT / QML_SINGLETON), so the
# views import them as typed QML types instead of receiving context objects.
# A static library shared by the application and the tests, so both resolve
# `import Gambasse` identically.
qt_add_library(GambasseQml STATIC)
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(GambasseQml
    URI Gambasse
    VERSION 1.0
    # Under an import path (build/qml) like an installed module, so qmllint and
    # qmlls find it; build/Gambasse is taken by the Linux executable.
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/Gambasse
    # QAbstractItemModel (PatientController.gridModel) is declared there.
    DEPENDENCIES QtQml.Models
    SOURCES
        src/ui/AppIcon.cpp
        src/ui/AppIcon.h
        src/ui/DateFormat.h
        src/ui/InterfaceSettings.cpp
        src/ui/InterfaceSettings.h
        src/ui/controller/AdultConsultationController.cpp
        src/ui/controller/AdultConsultationController.h
        src/ui/controller/AdultHistoryController.cpp
        src/ui/controller/AdultHistoryController.h
        src/ui/controller/ConsultationController.cpp
        src/ui/controller/ConsultationController.h
        src/ui/controller/FormController.cpp
        src/ui/controller/FormController.h
        src/ui/controller/HistoryController.cpp
        src/ui/controller/HistoryController.h
        src/ui/controller/PatientController.cpp
        src/ui/controller/PatientController.h
        src/ui/controller/PediatricConsultationController.cpp
        src/ui/controller/PediatricConsultationController.h
        src/ui/controller/PediatricHistoryController.cpp
        src/ui/controller/PediatricHistoryController.h
        src/ui/controller/PregnancyConsultationController.cpp
        src/ui/controller/PregnancyConsultationController.h
        src/ui/controller/PregnancyHistoryController.cpp
        src/ui/controller/PregnancyHistoryController.h
        src/ui/filter/ColumnFilterProxy.cpp
        src/ui/filter/ColumnFilterProxy.h
    QML_FILES
        qml/App.qml
        qml/Root.qml
        qml/Theme.qml
        qml/TitleBar.qml
        qml/ResizeHandles.qml
        qml/windows/SplashWindow.qml
        qml/windows/MainWindow.qml
        qml/components/GxTextInput.qml
        qml/components/GxNumberInput.qml
        qml/components/GxDateInput.qml
        qml/components/GxComboInput.qml
        qml/components/GxLabel.qml
        qml/components/GxCheck.qml
        qml/components/GxDialog.qml
        qml/components/GxField.qml
        qml/components/GxGroup.qml
        qml/components/GxTextArea.qml
        qml/components/GxCheckField.qml
        qml/components/GxTabView.qml
        qml/components/GxActionButton.qml
        qml/windows/HistoryWindow.qml
        qml/windows/ConsultationWindow.qml
        qml/windows/ConsultationList.qml
        qml/windows/AdultConsultationForm.qml
        qml/windows/PregnancyConsultationForm.qml
        qml/windows/PediatricConsultationForm.qml
        qml/windows/AdultHistoryForm.qml
        qml/windows/PediatricHistoryForm.qml
        qml/windows/PregnancyHistoryForm.qml
        qml/windows/StartupError.qml
)
# The generated type registration includes each registered header by its bare
# file name; project code keeps including them from the `src` root.
target_include_directories(GambasseQml PRIVATE src/ui src/ui/controller)
target_link_libraries(GambasseQml PUBLIC
    GambasseCore
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

# --- Application ---------------------------------------------------------------
qt_add_executable(Gambasse WIN32
    src/main.cpp
    resources/resources.qrc
    resources/bd/bd.qrc
    translations/i18n.qrc
)

# Single source of truth for the program version, published at runtime with
# QCoreApplication::setApplicationVersion() and checked against the branch name
# in CI.
target_compile_definitions(Gambasse PRIVATE GAMBASSE_VERSION="${PROJECT_VERSION}")

target_link_libraries(Gambasse PRIVATE
    GambasseQml
    GambasseQmlplugin
)

# After building, copy Qt DLLs and plugins next to the executable on Windows so
# it can run from build/ without modifying the system PATH.
if(WIN32)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET Gambasse POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations
                    --qmldir "${CMAKE_SOURCE_DIR}/qml"
                    $<TARGET_FILE:Gambasse>
            COMMENT "Running windeployqt for Gambasse.exe"
        )
    else()
        message(WARNING "windeployqt was not found next to Qt6::qmake; build/Gambasse.exe will not include Qt DLLs.")
    endif()
endif()

# The standalone launcher is deployed only on Windows. It starts the Qt
# application from lib/ without depending on the Qt runtime itself.
if(WIN32)
    add_executable(Lanzador WIN32 src/launcher/launcher.cpp)
    set_target_properties(Lanzador PROPERTIES OUTPUT_NAME "gambasse")
    target_link_libraries(Lanzador PRIVATE shell32)
    target_link_options(Lanzador PRIVATE -static -static-libgcc -static-libstdc++)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
