add_subdirectory(ArchiveManager)
add_subdirectory(Builder)
add_subdirectory(Bundler)
add_subdirectory(Configuration)
add_subdirectory(Container)
add_subdirectory(Downloader)
add_subdirectory(Executor)
add_subdirectory(Installer)
add_subdirectory(Package)
add_subdirectory(Recipe)
add_subdirectory(Resolver)
add_subdirectory(Stripper)
add_subdirectory(Triggerer)
add_subdirectory(Uninstaller)
add_subdirectory(Utilities)

file(GLOB Main_Src "main/*.cxx")

add_executable(pkgupd ${Main_Src})
target_link_libraries(pkgupd 
    ArchiveManager
    Builder
    Bundler
    Configuration
    Downloader
    Executor
    Installer
    Package
    Recipe
    Resolver
    Triggerer
    Uninstaller
    Utilities)