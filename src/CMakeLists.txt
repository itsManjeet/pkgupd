add_library(${PROJECT_NAME}_LIB STATIC
        ArchiveManager.cpp ArchiveManager.cpp
        Colors.h
        Configuration.cpp Configuration.h
        defines.hxx
        Downloader.cpp Downloader.h
        Engine.cpp Engine.h
        Execute.h
        MetaInfo.h MetaInfo.cpp
        Repository.cpp Repository.h
        Resolver.h
        SystemDatabase.cpp SystemDatabase.h
        Trigger.cpp Trigger.h
        json.h
        Builder.cpp
        Builder.h
        Executor.cpp
)
target_link_libraries(${PROJECT_NAME}_LIB yaml-cpp)