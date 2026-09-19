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

add_executable(test_schemamigrator test_schemamigrator.cpp ../src/data/common/SchemaMigrator.cpp)

set_target_properties(test_schemamigrator PROPERTIES AUTOMOC ON)

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
