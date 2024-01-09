add_executable(${PROJECT_NAME}
        pkgupd.cpp
        pkgupd-autoremove.cpp
        pkgupd-cleanup.cpp
        pkgupd-depends.cpp
        pkgupd-info.cpp
        pkgupd-installer.cpp
        pkgupd-owner.cpp
        pkgupd-remove.cpp
        pkgupd-search.cpp
        pkgupd-sync.cpp
        pkgupd-trigger.cpp
        pkgupd-update.cpp
        pkgupd-build.cpp
        pkgupd-cachefile.cpp
        pkgupd-ignite.cpp
        ignite/ignite_common.h
        ignite/ignite-status.cpp
        ignite/ignite-build.cpp
        ignite/ignite-checkout.cpp
        ignite/ignite-meta.cpp
        
        pkgupd-sysroot.cpp
        sysroot/sysroot_common.h
        sysroot/sysroot-list.cpp

)

target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_LIB -static)