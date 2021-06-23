add_library(port SHARED port.cc)
target_link_libraries(port yaml-cpp)
install(TARGETS port LIBRARY DESTINATION lib/pkgupd/port)