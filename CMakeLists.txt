cmake_minimum_required(VERSION 3.11)
project(OpenixIMG C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add MSVC specific macros
if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/rc6/includes
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/twofish/includes
        ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(app)
add_subdirectory(test)