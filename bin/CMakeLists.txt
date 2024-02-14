add_executable(${PROJECT_NAME}
        pkgupd.cpp

        pkgupd-unlocked.cpp
        unlocked/unlocked-autoremove.cpp
        unlocked/unlocked-cleanup.cpp
        unlocked/unlocked-depends.cpp
        unlocked/unlocked-info.cpp
        unlocked/unlocked-installer.cpp
        unlocked/unlocked-owner.cpp
        unlocked/unlocked-remove.cpp
        unlocked/unlocked-search.cpp
        unlocked/unlocked-sync.cpp
        unlocked/unlocked-trigger.cpp
        unlocked/unlocked-update.cpp
        unlocked/unlocked-build.cpp
        unlocked/unlocked-cachefile.cpp
        unlocked/unlocked_common.h

        pkgupd-ignite.cpp
        ignite/ignite_common.h
        ignite/ignite-status.cpp
        ignite/ignite-build.cpp
        ignite/ignite-checkout.cpp
        ignite/ignite-meta.cpp

        pkgupd-sysroot.cpp
        sysroot/sysroot_common.h
        sysroot/sysroot-list.cpp
        sysroot/sysroot-upgrade.cpp

)

target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_LIB)