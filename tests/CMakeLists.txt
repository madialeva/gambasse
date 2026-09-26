# Unit tests (Qt Test). Built with BUILD_TESTING=ON (default).
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Test)

add_executable(test_columnfilterproxy test_columnfilterproxy.cpp ../src/ui/filter/ColumnFilterProxy.cpp)

set_target_properties(test_columnfilterproxy PROPERTIES AUTOMOC ON)

target_include_directories(test_columnfilterproxy PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_columnfilterproxy PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
)

add_test(NAME columnfilterproxy COMMAND test_columnfilterproxy)

# Run GUI-less tests headless on CI (no X server needed).
set_tests_properties(columnfilterproxy PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_schemamigrator
    test_schemamigrator.cpp
    ../src/data/common/SchemaMigrator.cpp
    ${CMAKE_SOURCE_DIR}/resources/bd/bd.qrc
)

set_target_properties(test_schemamigrator PROPERTIES AUTOMOC ON AUTORCC ON)

target_include_directories(test_schemamigrator PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_schemamigrator PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Test
)

add_test(NAME schemamigrator COMMAND test_schemamigrator)

set_tests_properties(schemamigrator PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_appsettings test_appsettings.cpp ../src/logic/AppSettings.cpp)

set_target_properties(test_appsettings PROPERTIES AUTOMOC ON)

target_include_directories(test_appsettings PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_appsettings PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME appsettings COMMAND test_appsettings)

set_tests_properties(appsettings PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_photomanager test_photomanager.cpp ../src/logic/PhotoManager.cpp)

set_target_properties(test_photomanager PROPERTIES AUTOMOC ON)

target_include_directories(test_photomanager PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_photomanager PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME photomanager COMMAND test_photomanager)

set_tests_properties(photomanager PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_patientservice
    test_patientservice.cpp
    ../src/logic/PatientService.cpp
    ../src/logic/PhotoManager.cpp
    ../src/data/common/Database.cpp
    ../src/data/common/SchemaMigrator.cpp
    ${CMAKE_SOURCE_DIR}/resources/bd/bd.qrc
)

set_target_properties(test_patientservice PROPERTIES AUTOMOC ON AUTORCC ON)

target_include_directories(test_patientservice PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_patientservice PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Test
)

add_test(NAME patientservice COMMAND test_patientservice)

set_tests_properties(patientservice PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_clinicalcontext
    test_clinicalcontext.cpp
    ../src/logic/ClinicalContext.cpp
    ../src/data/common/Database.cpp
    ../src/data/common/SchemaMigrator.cpp
    ${CMAKE_SOURCE_DIR}/resources/bd/bd.qrc
)

set_target_properties(test_clinicalcontext PROPERTIES AUTOMOC ON AUTORCC ON)

target_include_directories(test_clinicalcontext PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_clinicalcontext PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Test
)

add_test(NAME clinicalcontext COMMAND test_clinicalcontext)

set_tests_properties(clinicalcontext PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_sexlabel
    test_sexlabel.cpp
    ../src/logic/SexLabel.cpp
    ${CMAKE_SOURCE_DIR}/translations/i18n.qrc
)

set_target_properties(test_sexlabel PROPERTIES AUTOMOC ON AUTORCC ON)

target_include_directories(test_sexlabel PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_sexlabel PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME sexlabel COMMAND test_sexlabel)

set_tests_properties(sexlabel PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_pediatricehistoryservice
    test_pediatricehistoryservice.cpp
    ../src/logic/PediatricHistoryService.cpp
    ../src/data/common/Database.cpp
    ../src/data/common/SchemaMigrator.cpp
    ${CMAKE_SOURCE_DIR}/resources/bd/bd.qrc
)

set_target_properties(test_pediatricehistoryservice PROPERTIES AUTOMOC ON AUTORCC ON)

target_include_directories(test_pediatricehistoryservice PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_pediatricehistoryservice PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Test
)

add_test(NAME pediatricehistoryservice COMMAND test_pediatricehistoryservice)

set_tests_properties(pediatricehistoryservice PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_uxcheck test_uxcheck.cpp)

set_target_properties(test_uxcheck PROPERTIES AUTOMOC ON)

target_include_directories(test_uxcheck PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_uxcheck PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    UxWidgets::UxWidgets
)

add_test(NAME uxcheck COMMAND test_uxcheck)

set_tests_properties(uxcheck PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_pediatricehistorywindow
    test_pediatricehistorywindow.cpp
    ../src/ui/window/PediatricHistoryWindow.cpp
    ../src/ui/window/TitleBar.cpp
    ../src/logic/PediatricHistoryService.cpp
    ../src/logic/PediatricHistoryLabels.cpp
    ../src/data/common/Database.cpp
    ../src/data/common/SchemaMigrator.cpp
    ${CMAKE_SOURCE_DIR}/resources/bd/bd.qrc
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)

set_target_properties(test_pediatricehistorywindow PROPERTIES AUTOMOC ON AUTORCC ON)

target_include_directories(test_pediatricehistorywindow PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_pediatricehistorywindow PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Test
    UxWidgets::UxWidgets
)

add_test(NAME pediatricehistorywindow COMMAND test_pediatricehistorywindow)

set_tests_properties(pediatricehistorywindow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_adulthistoryservice
    test_adulthistoryservice.cpp
    ../src/logic/AdultHistoryService.cpp
    ../src/data/common/Database.cpp
    ../src/data/common/SchemaMigrator.cpp
    ${CMAKE_SOURCE_DIR}/resources/bd/bd.qrc
)

set_target_properties(test_adulthistoryservice PROPERTIES AUTOMOC ON AUTORCC ON)

target_include_directories(test_adulthistoryservice PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_adulthistoryservice PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Test
)

add_test(NAME adulthistoryservice COMMAND test_adulthistoryservice)

set_tests_properties(adulthistoryservice PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_pregnancyhistoryservice
    test_pregnancyhistoryservice.cpp
    ../src/logic/PregnancyHistoryService.cpp
    ../src/data/common/Database.cpp
    ../src/data/common/SchemaMigrator.cpp
    ${CMAKE_SOURCE_DIR}/resources/bd/bd.qrc
)

set_target_properties(test_pregnancyhistoryservice PROPERTIES AUTOMOC ON AUTORCC ON)

target_include_directories(test_pregnancyhistoryservice PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_pregnancyhistoryservice PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Test
)

add_test(NAME pregnancyhistoryservice COMMAND test_pregnancyhistoryservice)

set_tests_properties(pregnancyhistoryservice PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_adulthistorywindow
    test_adulthistorywindow.cpp
    ../src/ui/window/AdultHistoryWindow.cpp
    ../src/ui/window/TitleBar.cpp
    ../src/logic/AdultHistoryService.cpp
    ../src/logic/PediatricHistoryLabels.cpp
    ../src/data/common/Database.cpp
    ../src/data/common/SchemaMigrator.cpp
    ${CMAKE_SOURCE_DIR}/resources/bd/bd.qrc
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)

set_target_properties(test_adulthistorywindow PROPERTIES AUTOMOC ON AUTORCC ON)

target_include_directories(test_adulthistorywindow PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_adulthistorywindow PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Test
    UxWidgets::UxWidgets
)

add_test(NAME adulthistorywindow COMMAND test_adulthistorywindow)

set_tests_properties(adulthistorywindow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_pregnancyhistorywindow
    test_pregnancyhistorywindow.cpp
    ../src/ui/window/PregnancyHistoryWindow.cpp
    ../src/ui/window/TitleBar.cpp
    ../src/logic/PregnancyHistoryService.cpp
    ../src/logic/PediatricHistoryLabels.cpp
    ../src/data/common/Database.cpp
    ../src/data/common/SchemaMigrator.cpp
    ${CMAKE_SOURCE_DIR}/resources/bd/bd.qrc
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)

set_target_properties(test_pregnancyhistorywindow PROPERTIES AUTOMOC ON AUTORCC ON)

target_include_directories(test_pregnancyhistorywindow PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_pregnancyhistorywindow PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Test
    UxWidgets::UxWidgets
)

add_test(NAME pregnancyhistorywindow COMMAND test_pregnancyhistorywindow)

set_tests_properties(pregnancyhistorywindow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
