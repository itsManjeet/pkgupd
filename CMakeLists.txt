cmake_minimum_required(VERSION 3.22.1)
project(pkgupd
        VERSION 1.0
        DESCRIPTION "Package Manager"
        LANGUAGES C CXX)

set(POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
find_package(OSTree)

find_library(CURL curl REQUIRED)
find_package(yaml-cpp)


include_directories(include)
add_subdirectory(src)
add_subdirectory(bin)

install(TARGETS ${PROJECT_NAME}_LIB ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/src
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")