add_library(libzinc INTERFACE)
target_include_directories(libzinc INTERFACE ../include)

add_executable(zinc_main main.cc)
enable_cxx_properties_for_target(zinc_main)
target_link_libraries(zinc_main PRIVATE libzinc)