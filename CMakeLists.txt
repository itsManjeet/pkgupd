cmake_minimum_required(VERSION 3.22.1)
project(pkgupd
    VERSION 1.0
    DESCRIPTION "Package Manager"
    LANGUAGES C CXX)

set(POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(FetchContent)
FetchContent_Declare(yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG yaml-cpp-0.7.0
)
FetchContent_MakeAvailable(yaml-cpp)

include_directories(include)
add_subdirectory(src)
add_subdirectory(bin)


install(TARGETS pkgupd
    RUNTIME DESTINATION bin)