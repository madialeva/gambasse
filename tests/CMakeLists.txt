# Unit tests (Qt Test). Built with BUILD_TESTING=ON (default).
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

add_executable(test_columnfilterproxy test_columnfilterproxy.cpp ../src/filter/ColumnFilterProxy.cpp)

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
