cmake_minimum_required(VERSION 3.22.1)
project(pkgupd
        VERSION 2.0
        DESCRIPTION "Package Manager"
        LANGUAGES C CXX)

set(POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include_directories(src)

add_subdirectory(src)

add_subdirectory(bin)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/src
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")