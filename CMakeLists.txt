cmake_minimum_required(VERSION 3.22.1)
project(pkgupd
        VERSION 1.0
        DESCRIPTION "Package Manager"
        LANGUAGES C CXX)

set(POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(FetchContent)

FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG yaml-cpp-0.7.0
)
FetchContent_GetProperties(yaml-cpp)

if (NOT yaml-cpp_POPULATED)
    message(STATUS "Fetching yaml-cpp...")
    FetchContent_Populate(yaml-cpp)
    add_subdirectory(${yaml-cpp_SOURCE_DIR} ${yaml-cpp_BINARY_DIR})
endif ()

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