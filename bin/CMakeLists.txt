add_executable(${PROJECT_NAME}
        pkgupd.cpp

        autoremove.cpp
        cleanup.cpp
        depends.cpp
        info.cpp
        installer.cpp
        owner.cpp
        remove.cpp
        search.cpp
        sync.cpp
        trigger.cpp
        update.cpp
        build.cpp
        cachefile.cpp

        common.h

        ignite.cpp
        ignite/common.h
        ignite/status.cpp
        ignite/build.cpp
        ignite/checkout.cpp
        ignite/meta.cpp
)

target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_Shared)