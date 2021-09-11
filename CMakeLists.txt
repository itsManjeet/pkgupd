cmake_minimum_required(VERSION 3.10)
set(PROJECT "PKGUPD")

project(${PROJECT})

set(CMAKE_CXX_STANDARD 17) # using std::filesystem
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

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

set_target_properties(pkgupd PROPERTIES SUFFIX "-new")


install(TARGETS pkgupd
    DESTINATION bin)