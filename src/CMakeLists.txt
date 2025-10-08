# OpenixIMG Source CMakeLists.txt

# 添加可执行文件
add_library(openiximg
        OpenixIMGWTY.cpp
        OpenixPacker.cpp
        OpenixCFG.cpp
        OpenixPartition.cpp
)

# 添加包含目录
target_include_directories(openiximg
        PRIVATE
        ${CMAKE_SOURCE_DIR}/includes
        ${CMAKE_SOURCE_DIR}/lib/rc6/includes
        ${CMAKE_SOURCE_DIR}/lib/twofish/includes
)

# 链接RC6和Twofish库
target_link_libraries(openiximg
        PRIVATE
        rc6
        twofish
)

# 设置C++标准
target_compile_features(openiximg
        PRIVATE
        cxx_std_17
)

# 如果在Windows上，添加必要的定义和链接
target_compile_definitions(openiximg
        PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

# 安装规则
install(
        TARGETS openiximg
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)
