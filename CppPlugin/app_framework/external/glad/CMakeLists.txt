cmake_minimum_required(VERSION 3.22.1)

project (glad)

add_library(glad STATIC src/glad.c)
target_include_directories(glad PUBLIC include)
set_property(TARGET glad PROPERTY CXX_STANDARD 11)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(glad PRIVATE -fPIC)
endif()

# not using the normal pattern since cmake flattens
# the includes into one directory, we have two
# subdirectories.
install( DIRECTORY "include" DESTINATION ".")
install( TARGETS glad ARCHIVE DESTINATION lib)
