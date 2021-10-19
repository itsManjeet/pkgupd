cmake_minimum_required(VERSION 3.2)
set(PROJECT "PKGUPD")

project(${PROJECT})

set(CMAKE_CXX_STANDARD 17) # using std::filesystem
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

option(INSTALL_GTEST "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" OFF)


find_package(YAML-CPP REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIRS})


find_library(LIBCURL curl REQUIRED)

file(GLOB LIBPKGUPD_Src "libpkgupd/*.cc" "libpkgupd/*.hh")
add_library(libpkgupd ${LIBPKGUPD_Src})
target_link_libraries(libpkgupd ${LIBCURL} ${YAML_CPP_LIBRARIES})

file(GLOB PKGUPD_Src "pkgupd/*.cc" "pkgupd/*.hh")
add_executable(pkgupd ${PKGUPD_Src})
add_dependencies(pkgupd libpkgupd)
target_link_libraries(pkgupd libpkgupd)


option(BUILD_TESTS "Build Test programs" ON)

# if (BUILD_TESTS)
#   enable_testing()
  
#  include(FetchContent)
#   FetchContent_Declare(
#     googletest
#     URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
#   )
#   FetchContent_MakeAvailable(googletest)
#   add_compile_definitions(TEST_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")

  
#   file(GLOB UnitTest_Src "tests/*.cc")
#   add_executable(unittest ${UnitTest_Src})
#   add_dependencies(unittest libpkgupd)
#   target_link_libraries(unittest libpkgupd gtest_main)

#   include(GoogleTest)
#   gtest_discover_tests(unittest)
# endif (BUILD_TESTS)

install(TARGETS pkgupd
    DESTINATION bin)