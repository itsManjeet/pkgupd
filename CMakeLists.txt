cmake_minimum_required(VERSION 3.22.1)
project(pkgupd
        VERSION 1.0
        DESCRIPTION "Package Manager"
        LANGUAGES C CXX)

set(POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include_directories(include)
add_subdirectory(src)
add_subdirectory(bin)

find_package(YAML-CPP REQUIRED)

install(TARGETS pkgupd
        RUNTIME DESTINATION bin)