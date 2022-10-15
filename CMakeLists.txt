cmake_minimum_required(VERSION 3.2)
set(PROJECT "PKGUPD")

project(${PROJECT})
set(POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_STANDARD 17) # using std::filesystem
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

configure_file(define.hxx.in ${CMAKE_BINARY_DIR}/define/define.hxx)
include_directories(${CMAKE_BINARY_DIR}/define)
add_subdirectory(Source)