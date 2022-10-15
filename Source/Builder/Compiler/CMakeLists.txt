add_subdirectory(AutoConf)
add_subdirectory(Cargo)
add_subdirectory(CMake)
add_subdirectory(Go)
add_subdirectory(Makefile)
add_subdirectory(Meson)
add_subdirectory(Pysetup)
add_subdirectory(Script)
add_subdirectory(System)

add_library(Compiler   OBJECT
    "Compiler.cxx"
    "Compiler.hxx")
target_link_libraries(Compiler PUBLIC
    AutoConf
    Cargo
    CMake
    Go
    Makefile
    Meson
    Pysetup 
    Script
    System)