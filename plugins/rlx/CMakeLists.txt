add_library(rlx SHARED rlx.cc)
target_link_libraries(rlx yaml-cpp)
install(TARGETS rlx LIBRARY DESTINATION lib/pkgupd/rlx)