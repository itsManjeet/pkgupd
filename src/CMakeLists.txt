add_executable(${PROJECT_NAME}
        ArchiveManager.cpp ArchiveManager.cpp
        Colors.h
        Configuration.cpp Configuration.h
        defines.hxx
        Http.cpp Http.h
        Engine.cpp Engine.h
        Executor.h Executor.cpp
        MetaInfo.h MetaInfo.cpp
        Repository.cpp Repository.h
        Resolver.h
        SystemDatabase.cpp SystemDatabase.h
        Trigger.cpp Trigger.h
        json.h
        Builder.cpp Builder.h
        Ignite.cpp
        Ignite.h
        Container.cpp
        Container.h

        bin/pkgupd.cpp

        bin/pkgupd-autoremove.cpp
        bin/pkgupd-cleanup.cpp
        bin/pkgupd-depends.cpp
        bin/pkgupd-info.cpp
        bin/pkgupd-installer.cpp
        bin/pkgupd-owner.cpp
        bin/pkgupd-remove.cpp
        bin/pkgupd-search.cpp
        bin/pkgupd-sync.cpp
        bin/pkgupd-trigger.cpp
        bin/pkgupd-update.cpp
        bin/pkgupd-build.cpp
        bin/pkgupd-cachefile.cpp

        bin/common.h

        bin/pkgupd-ignite.cpp
        bin/ignite/ignite_common.h
        bin/ignite/ignite-status.cpp
        bin/ignite/ignite-build.cpp
        bin/ignite/ignite-checkout.cpp
        bin/ignite/ignite-meta.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE
        libyaml-cpp.a)
