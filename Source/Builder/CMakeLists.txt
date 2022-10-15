add_subdirectory(Compiler)

add_library(Builder OBJECT
    "Builder.cxx"
    "Builder.hxx")
target_link_libraries(Builder PUBLIC Compiler)