# OpenixIMG Source CMakeLists.txt

add_library(openiximg
        OpenixIMGWTY.cpp
        OpenixPacker.cpp
        OpenixCFG.cpp
        OpenixPartition.cpp
        OpenixIMGFile.cpp
)

target_include_directories(openiximg
        PRIVATE
        ${CMAKE_SOURCE_DIR}/includes
        ${CMAKE_SOURCE_DIR}/lib/rc6/includes
        ${CMAKE_SOURCE_DIR}/lib/twofish/includes
)

target_link_libraries(openiximg
        PRIVATE
        rc6
        twofish
)

target_compile_features(openiximg
        PRIVATE
        cxx_std_17
)

target_compile_definitions(openiximg
        PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

install(
        TARGETS openiximg
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)
