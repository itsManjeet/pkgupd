add_executable(${PROJECT_NAME}_TEST
    pkgupd_test.cpp
        Executor.cpp)

target_link_libraries(${PROJECT_NAME}_TEST
    ${PROJECT_NAME}_LIB
    GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_TEST)