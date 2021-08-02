add_library(app SHARED app.cc)
target_link_libraries(app yaml-cpp curlcpp)
install(TARGETS app LIBRARY DESTINATION lib/pkgupd/app)
