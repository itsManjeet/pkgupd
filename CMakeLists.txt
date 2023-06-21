cmake_minimum_required(VERSION 3.2)
set(PROJECT "PKGUPD")

project(${PROJECT})
set(POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_STANDARD 17) # using std::filesystem
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(YAML-CPP REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_compile_options(-ggdb)
file(GLOB_RECURSE PKGUPD_Src "pkgupd/*.cc" "pkgupd/*.hh")
add_executable(pkgupd ${PKGUPD_Src})
target_link_libraries(pkgupd ${YAML_CPP_LIBRARIES})


# find_package(PkgConfig REQUIRED)
# pkg_check_modules(SQUASHFUSE squashfuse REQUIRED)
# include_directories(${SQUASHFUSE_INCLUDE_DIRS})
# file(GLOB RUNTIME_Src "runtime/*.cc" "runtime/*.hh")
# add_executable(runtime ${RUNTIME_Src})
# target_link_libraries(runtime ${SQUASHFUSE_LIBRARIES})

install(TARGETS pkgupd
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libpkgupd/
        DESTINATION "include/libpkgupd"
        FILES_MATCHING
            PATTERN "*.hh")

install(FILES services/pkgupd-watchdog.service
    DESTINATION "lib/systemd/user")

install(FILES scripts/pkgupd-app-remove
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                GROUP_EXECUTE GROUP_READ
                WORLD_EXECUTE WORLD_READ

    DESTINATION bin)

set(INSTALL_GTEST OFF)
if (BUILD_TEST)
enable_testing()
add_subdirectory(tests)
endif()
