cmake_minimum_required(VERSION 3.22.1)
set(PROJECT "PKGUPD")

project(${PROJECT})
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
file(GLOB_RECURSE PKGUPD_Src "src/*.cxx" "src/*.hxx")
add_executable(pkgupd ${PKGUPD_Src})
target_link_libraries(pkgupd yaml-cpp -static)

install(TARGETS pkgupd
    RUNTIME DESTINATION bin)