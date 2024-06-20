add_library(${PROJECT_NAME}_LIB STATIC
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
)
target_link_libraries(${PROJECT_NAME}_LIB PRIVATE
        yaml-cpp::yaml-cpp)
