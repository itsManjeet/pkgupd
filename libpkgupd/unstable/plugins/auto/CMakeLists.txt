add_library(auto SHARED auto.cc)
target_link_libraries(auto yaml-cpp)
install(TARGETS auto LIBRARY DESTINATION lib/pkgupd/auto)