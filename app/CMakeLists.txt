add_executable(openiximgcli
        OpenixIMG.cpp
)

target_include_directories(openiximgcli
        PRIVATE
        ${CMAKE_SOURCE_DIR}/includes
        ${CMAKE_SOURCE_DIR}/lib/rc6/includes
        ${CMAKE_SOURCE_DIR}/lib/twofish/includes
)

target_link_libraries(openiximgcli
        PRIVATE
        rc6
        twofish
        openiximg
)

target_compile_features(openiximgcli
        PRIVATE
        cxx_std_17
)

target_compile_definitions(openiximgcli
        PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

install(
        TARGETS openiximgcli
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)