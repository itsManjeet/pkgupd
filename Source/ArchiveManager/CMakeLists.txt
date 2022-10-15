add_subdirectory(AppImage)
add_subdirectory(Squashfs)
add_subdirectory(Tarball)

add_library(ArchiveManager  OBJECT
    "ArchiveManager.cxx"
    "ArchiveManager.hxx")

target_link_libraries(ArchiveManager PUBLIC
    AppImage
    Squashfs
    Tarball)