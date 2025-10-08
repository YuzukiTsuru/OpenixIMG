cmake_minimum_required(VERSION 3.11)
project(OpenixIMG C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
        lib/rc6/includes
        lib/twofish/includes
        includes
)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(app)
add_subdirectory(test)