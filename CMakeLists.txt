cmake_minimum_required(VERSION 3.10)
project(pkgupd)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(yaml-cpp REQUIRED)
include_directories(lib ${YAML_CPP_INCLUDE_DIRS})

add_subdirectory(plugins)

add_executable(pkgupd main.cc)
target_link_libraries(pkgupd yaml-cpp curlcpp dl)
install(TARGETS pkgupd
    RUNTIME DESTINATION bin)