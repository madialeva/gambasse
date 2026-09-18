# Vendored UxWidgets controls library (typed field + composite label/field
# control). Self-contained copy so Gambasse does not depend on the external
# Qt.UxWidgets repository; see README.md for the origin snapshot.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(UXWIDGETS_HEADERS
    include/UxWidgets/UxField.h
    include/UxWidgets/UxTextField.h
    include/UxWidgets/UxNumberField.h
    include/UxWidgets/UxDateField.h
    include/UxWidgets/UxInput.h
    include/UxWidgets/UxTextInput.h
    include/UxWidgets/UxNumberInput.h
    include/UxWidgets/UxDateInput.h
    include/UxWidgets/UxLabel.h
    include/UxWidgets/UxComboInput.h
)

set(UXWIDGETS_SOURCES
    src/UxField.cpp
    src/UxTextField.cpp
    src/UxNumberField.cpp
    src/UxDateField.cpp
    src/UxInput.cpp
    src/UxTextInput.cpp
    src/UxNumberInput.cpp
    src/UxDateInput.cpp
    src/UxLabel.cpp
    src/UxComboInput.cpp
)

add_library(UxWidgets STATIC ${UXWIDGETS_SOURCES} ${UXWIDGETS_HEADERS})
add_library(UxWidgets::UxWidgets ALIAS UxWidgets)

target_include_directories(UxWidgets PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(UxWidgets PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
