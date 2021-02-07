find_package(Catch2 REQUIRED)

add_executable(zinc_test main.cc tests/sanity.cc)
enable_cxx_properties_for_target(zinc_test)
target_include_directories(zinc_test PRIVATE ./)
target_link_libraries(zinc_test PRIVATE libzinc Catch2::Catch2)