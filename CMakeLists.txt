cmake_minimum_required(VERSION 3.2)
set(PROJECT "PKGUPD")

project(${PROJECT})

set(CMAKE_CXX_STANDARD 17) # using std::filesystem
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(YAML-CPP REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_library(LIBCURL curl REQUIRED)

file(GLOB_RECURSE LIBPKGUPD_Src "libpkgupd/*.cc" "libpkgupd/*.hh")
add_library(libpkgupd ${LIBPKGUPD_Src})
target_link_libraries(libpkgupd ${LIBCURL} ${YAML_CPP_LIBRARIES})


file(GLOB PKGUPD_Src "pkgupd/*.cc" "pkgupd/*.hh")
add_executable(pkgupd ${PKGUPD_Src})
add_dependencies(pkgupd libpkgupd)
target_link_libraries(pkgupd libpkgupd)

install(TARGETS pkgupd
    DESTINATION bin)

set(BUILD_TEST "build test" OFF)
if (BUILD_TEST)
enable_testing()
add_subdirectory(tests)
endif()