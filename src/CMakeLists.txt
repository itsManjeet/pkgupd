add_library(${PROJECT_NAME}_LIB SHARED
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
        Sysroot.cpp
        Sysroot.h
        SysUpgrade.cpp
        SysUpgrade.h
        Deployment.cpp Deployment.h
        Error.h
)
target_link_libraries(${PROJECT_NAME}_LIB
        ${YAML_CPP_LIBRARIES}
        ${CURL}
        OSTree)
