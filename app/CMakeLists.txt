add_executable(openiximgcli
        OpenixIMG.cpp
)

# 添加包含目录
target_include_directories(openiximgcli
        PRIVATE
        ${CMAKE_SOURCE_DIR}/includes
        ${CMAKE_SOURCE_DIR}/lib/rc6/includes
        ${CMAKE_SOURCE_DIR}/lib/twofish/includes
)

# 链接RC6和Twofish库
target_link_libraries(openiximgcli
        PRIVATE
        rc6
        twofish
        openiximg
)

# 设置C++标准
target_compile_features(openiximgcli
        PRIVATE
        cxx_std_17
)

# 如果在Windows上，添加必要的定义和链接
target_compile_definitions(openiximgcli
        PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

# 安装规则
install(
        TARGETS openiximgcli
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)