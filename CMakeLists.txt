cmake_minimum_required(VERSION 3.25)

project(Gambasse VERSION 1.0.0 LANGUAGES CXX)

# The compiler of the project: GCC 16 or later (toolchain-cpp's gcc, and its
# gcc-mingw cross compiler for Windows, AGENTS.md), checked before anything is
# searched or built. The MinGW of the Qt kits is rejected too.
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
    if(WIN32)
        set(_gambasse_compiler_hint
            "Windows is built from Linux with toolchain-cpp's gcc-mingw cross compiler: configure with the windows preset, or run scripts/build-windows.sh")
    else()
        set(_gambasse_compiler_hint
            "Install it with ../toolchain-cpp/install.sh gcc and configure with the native preset, which uses it")
    endif()
    message(FATAL_ERROR
        "Found ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CXX_COMPILER}), "
        "but the project needs GCC 16 or later. ${_gambasse_compiler_hint}.")
endif()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

# The self-contained executables, Qt and the C++ runtime linked statically: for
# Linux (static preset, scripts/build-static.sh) with toolchain-cpp's qt-static
# kit and no run path, and for Windows (windows preset, scripts/build-windows.sh),
# cross-built with its gcc-mingw and qt-mingw. Off for every other build.
option(GAMBASSE_STATIC "Build a self-contained executable with the static Qt kit" OFF)
if(GAMBASSE_STATIC AND NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Windows)$")
    message(FATAL_ERROR "GAMBASSE_STATIC is only supported on Linux and Windows.")
endif()
# The Windows build is the self-contained executable only: there is no package
# with the Qt DLLs beside it.
if(WIN32 AND NOT GAMBASSE_STATIC)
    message(FATAL_ERROR
        "The Windows build is the self-contained executable, cross-built from Linux: "
        "configure with the windows preset (GAMBASSE_STATIC=ON), or run "
        "scripts/build-windows.sh.")
endif()

# GCC 16's libstdc++ is newer than the system's: the development executables and
# the tests find it through their run path, and the Qt libraries they load share
# it. The Linux package ships it instead (deploy-linux.sh), and the static
# executable links it.
if(NOT GAMBASSE_STATIC)
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libstdc++.so
        OUTPUT_VARIABLE _gambasse_libstdcxx OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    file(REAL_PATH "${_gambasse_libstdcxx}" _gambasse_libstdcxx)
    cmake_path(GET _gambasse_libstdcxx PARENT_PATH _gambasse_libstdcxx_dir)
    list(APPEND CMAKE_BUILD_RPATH ${_gambasse_libstdcxx_dir})
endif()

# Qt 6.12 everywhere (development machines, CI and the Linux package): no code
# paths for other versions. The setup also enables AUTOMOC and every Qt policy
# up to 6.12 (QTP0001: the QML module under the standard qrc:/qt/qml/ prefix).
find_package(Qt6 6.12 REQUIRED COMPONENTS Core Gui Sql Qml Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.12)

# The static build needs the static kit: with the shared one it would build an
# executable that still loads Qt from toolchain-cpp.
if(GAMBASSE_STATIC)
    get_target_property(_gambasse_qt_type Qt6::Core TYPE)
    if(NOT _gambasse_qt_type STREQUAL "STATIC_LIBRARY")
        cmake_path(GET CMAKE_SOURCE_DIR PARENT_PATH _gambasse_parent_dir)
        if(WIN32)
            message(FATAL_ERROR
                "GAMBASSE_STATIC for Windows needs the static Qt for Windows, but the Qt "
                "found (${Qt6_DIR}) is not static. It is expected in "
                "${_gambasse_parent_dir}/toolchain-cpp/software/Qt/6.12.0-mingw: install it "
                "with ../toolchain-cpp/install.sh qt-mingw and configure with the windows preset.")
        endif()
        message(FATAL_ERROR
            "GAMBASSE_STATIC needs a static Qt, but the Qt found (${Qt6_DIR}) is not "
            "static. The static Qt is expected in "
            "${_gambasse_parent_dir}/toolchain-cpp/software/Qt/6.12.0-static: install it "
            "with ../toolchain-cpp/install.sh qt-static and configure with the static preset.")
    endif()
endif()

include(CTest)

# --- Core: data access and business rules -----------------------------------
# QtCore/QtSql plus the QtGui image classes (QImage, QImageReader/Writer) for
# the patient photos; no windows or UI: shared by the QML module and the
# GUI-less tests.
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
target_link_libraries(GambasseCore PUBLIC Qt6::Core Qt6::Gui Qt6::Sql)

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
        src/ui/AppInfo.cpp
        src/ui/AppInfo.h
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
        qml/dialogs/AboutDialog.qml
        qml/dialogs/PhotoDialog.qml
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
# What the information dialog says about this build (AppInfo).
target_compile_definitions(GambasseQml PRIVATE
    GAMBASSE_CXX_STANDARD="${CMAKE_CXX_STANDARD}"
    GAMBASSE_COMPILER_VERSION="${CMAKE_CXX_COMPILER_VERSION}")
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

# Static executable: the C++ runtime inside, and the Qt plugins it links, fixed
# here. qmlimportscanner adds the QML modules' plugins.
if(GAMBASSE_STATIC)
    target_link_options(Gambasse PRIVATE -static-libstdc++ -static-libgcc)
endif()
if(GAMBASSE_STATIC AND WIN32)
    # Windows: its platform, which also brings the native file dialog of the
    # photo chooser (plus offscreen for the diagnostics), SQLite, JPEG and SVG
    # images and SVG icons. winpthreads is static only in gcc-mingw.
    qt_import_plugins(Gambasse
        INCLUDE_BY_TYPE platforms Qt6::QWindowsIntegrationPlugin Qt6::QOffscreenIntegrationPlugin
        INCLUDE_BY_TYPE imageformats Qt6::QJpegPlugin Qt6::QSvgPlugin
        INCLUDE_BY_TYPE iconengines Qt6::QSvgIconPlugin
        INCLUDE_BY_TYPE sqldrivers Qt6::QSQLiteDriverPlugin
        EXCLUDE_BY_TYPE tls networkinformation generic qmltooling)
elseif(GAMBASSE_STATIC)
    # Linux: the plugins the Linux package ships (deploy-linux.sh): xcb (plus
    # offscreen for the diagnostics), SQLite, JPEG and SVG images, SVG icons and
    # the input contexts. No platform theme, so the photo chooser stays the Qt
    # Quick one, in the application's language and theme.
    qt_import_plugins(Gambasse
        INCLUDE_BY_TYPE platforms Qt6::QXcbIntegrationPlugin Qt6::QOffscreenIntegrationPlugin
        INCLUDE_BY_TYPE xcbglintegrations Qt6::QXcbGlxIntegrationPlugin Qt6::QXcbEglIntegrationPlugin
        INCLUDE_BY_TYPE platforminputcontexts
            Qt6::QComposePlatformInputContextPlugin Qt6::QIbusPlatformInputContextPlugin
        INCLUDE_BY_TYPE imageformats Qt6::QJpegPlugin Qt6::QSvgPlugin
        INCLUDE_BY_TYPE iconengines Qt6::QSvgIconPlugin
        INCLUDE_BY_TYPE sqldrivers Qt6::QSQLiteDriverPlugin
        EXCLUDE_BY_TYPE platformthemes tls networkinformation generic qmltooling)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
