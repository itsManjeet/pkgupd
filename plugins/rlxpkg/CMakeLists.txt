add_library(rlxpkg SHARED rlxpkg.cc)
target_link_libraries(rlxpkg yaml-cpp)
install(TARGETS rlxpkg LIBRARY DESTINATION lib/pkgupd/rlxpkg)