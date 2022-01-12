include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
add_definitions("-DDATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/data\"")

file(GLOB_RECURSE PkgupdTest_Src "*.cc" "*.hh")
add_executable(pkgupd-test ${PkgupdTest_Src})
target_link_libraries(pkgupd-test gtest_main libpkgupd)
include(GoogleTest)
gtest_discover_tests(pkgupd-test)
