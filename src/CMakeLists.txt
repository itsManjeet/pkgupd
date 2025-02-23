add_library(${PROJECT_NAME}_Shared
        ArchiveManager.cpp ArchiveManager.cpp
        Colors.h
        Configuration.cpp Configuration.h
        defines.h
        Engine.cpp Engine.h
        Executor.h Executor.cpp
        MetaInfo.h MetaInfo.cpp
        Repository.cpp Repository.h
        Resolver.h
        SystemDatabase.cpp SystemDatabase.h
        Trigger.cpp Trigger.h
        json.h
        Builder.cpp Builder.h
        Container.cpp
        Container.h
)

target_link_libraries(${PROJECT_NAME}_Shared PUBLIC
        yaml-cpp)
