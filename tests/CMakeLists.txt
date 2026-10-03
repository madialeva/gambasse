# Unit tests (Qt Test). Built with BUILD_TESTING=ON (default).
find_package(Qt6 REQUIRED COMPONENTS Core Gui Sql Test)

# gambasse_add_test(<name> [LIBS ...] [SOURCES ...])
# Builds test_<name>.cpp against the layer under test (GambasseCore for the
# GUI-less suites, GambasseQml for the controllers and the views) plus any extra
# sources (resources), and runs it headless (no X server needed on CI).
function(gambasse_add_test name)
    cmake_parse_arguments(TEST "" "" "LIBS;SOURCES" ${ARGN})
    add_executable(test_${name} test_${name}.cpp ${TEST_SOURCES})
    set_target_properties(test_${name} PROPERTIES AUTOMOC ON AUTORCC ON)
    target_link_libraries(test_${name} PRIVATE Qt6::Test ${TEST_LIBS})
    # Shared test helpers (ConsultationTestSupport.h) by angle include.
    target_include_directories(test_${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME ${name} COMMAND test_${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

set(SCHEMA_QRC ${CMAKE_SOURCE_DIR}/resources/bd/bd.qrc)
set(IMAGES_QRC ${CMAKE_SOURCE_DIR}/resources/resources.qrc)
set(I18N_QRC ${CMAKE_SOURCE_DIR}/translations/i18n.qrc)
set(QML_LIBS GambasseQml GambasseQmlplugin)

# Data and business rules.
gambasse_add_test(schemamigrator LIBS GambasseCore SOURCES ${SCHEMA_QRC})
gambasse_add_test(appsettings LIBS GambasseCore)
gambasse_add_test(photomanager LIBS GambasseCore)
gambasse_add_test(patientservice LIBS GambasseCore SOURCES ${SCHEMA_QRC})
gambasse_add_test(clinicalcontext LIBS GambasseCore SOURCES ${SCHEMA_QRC})
gambasse_add_test(sexlabel LIBS GambasseCore SOURCES ${I18N_QRC})
gambasse_add_test(pediatricconsultationlabels LIBS GambasseCore SOURCES ${I18N_QRC})
gambasse_add_test(pediatricehistoryservice LIBS GambasseCore SOURCES ${SCHEMA_QRC})
gambasse_add_test(adulthistoryservice LIBS GambasseCore SOURCES ${SCHEMA_QRC})
gambasse_add_test(pregnancyhistoryservice LIBS GambasseCore SOURCES ${SCHEMA_QRC})
gambasse_add_test(adultconsultationservice LIBS GambasseCore SOURCES ${SCHEMA_QRC})
gambasse_add_test(pregnancyconsultationservice LIBS GambasseCore SOURCES ${SCHEMA_QRC})
gambasse_add_test(pediatricconsultationservice LIBS GambasseCore SOURCES ${SCHEMA_QRC})

# Controllers and view helpers.
gambasse_add_test(columnfilterproxy LIBS ${QML_LIBS})
gambasse_add_test(patientcontroller LIBS ${QML_LIBS} SOURCES ${SCHEMA_QRC} ${IMAGES_QRC} ${I18N_QRC})

# QML foundation, control library and screens.
gambasse_add_test(qmlfoundation LIBS ${QML_LIBS} SOURCES ${SCHEMA_QRC} ${I18N_QRC} ${IMAGES_QRC})
target_compile_definitions(test_qmlfoundation PRIVATE
    QML_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures")

gambasse_add_test(qmlcomponents LIBS ${QML_LIBS} SOURCES ${IMAGES_QRC})
target_compile_definitions(test_qmlcomponents PRIVATE
    QML_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
    QML_COMPONENTS_DIR="${CMAKE_SOURCE_DIR}/qml/components")

set(SCREEN_QRC ${SCHEMA_QRC} ${IMAGES_QRC} ${I18N_QRC})
gambasse_add_test(qmlmainwindow LIBS ${QML_LIBS} SOURCES ${SCREEN_QRC})
gambasse_add_test(qmladulthistory LIBS ${QML_LIBS} SOURCES ${SCREEN_QRC})
# The pediatric and pregnancy history screens share one data-driven suite.
gambasse_add_test(qmlhistories LIBS ${QML_LIBS} SOURCES ${SCREEN_QRC})
# The three consultation screens share one data-driven suite.
gambasse_add_test(qmlconsultations LIBS ${QML_LIBS} SOURCES ${SCREEN_QRC})
