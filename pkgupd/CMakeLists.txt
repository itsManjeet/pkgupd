file(GLOB PKGUPD_MODULES "*.cc")

foreach(module_file ${PKGUPD_MODULES})
    get_filename_component(module ${module_file} NAME_WE)
    add_library(${module} ${module_file})
    target_link_libraries(${module} libpkgupd)
endforeach()
