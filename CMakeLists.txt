cmake_minimum_required(VERSION 3.21)

project(Gambasse VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql)

include(CTest)

# --- UxWidgets control library (vendored) ------------------------------------
# Self-contained copy under src/uxwidgets, so the project does not depend on
# the external Qt.UxWidgets repository. Adoption in the screens belongs to
# later issues; here the library only needs to compile and link.
add_subdirectory(src/uxwidgets)

set(SOURCES
    src/main.cpp
    src/ui/SplashWindow.cpp
    src/ui/MainWindow.cpp
    src/ui/TitleBar.cpp
    src/data/Database.cpp
    src/data/PatientsModel.cpp
    src/filter/ColumnFilterProxy.cpp
)

set(HEADERS
    src/Paths.h
    src/ui/SplashWindow.h
    src/ui/MainWindow.h
    src/ui/TitleBar.h
    src/data/Database.h
    src/data/Patient.h
    src/data/PatientsModel.h
    src/filter/ColumnFilterProxy.h
)

set(RESOURCES
    resources/resources.qrc
    translations/i18n.qrc
)

qt_add_executable(Gambasse WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(Gambasse PRIVATE src)

# Single source of truth for the program version, published at runtime with
# QCoreApplication::setApplicationVersion() and checked against the branch name
# in CI.
target_compile_definitions(Gambasse PRIVATE GAMBASSE_VERSION="${PROJECT_VERSION}")

target_link_libraries(Gambasse PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    UxWidgets::UxWidgets
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
