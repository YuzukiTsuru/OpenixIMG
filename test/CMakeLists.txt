add_executable(OpenixCFGTest
        OpenixCFGTest.cpp
)

target_link_libraries(OpenixCFGTest
        openiximg
)
target_include_directories(OpenixCFGTest PUBLIC
        ${CMAKE_SOURCE_DIR}/includes
)

add_test(NAME OpenixCFGTest COMMAND OpenixCFGTest)

# OpenixPartition test
source_group("Test Sources" FILES OpenixPartitionTest.cpp)
add_executable(OpenixPartitionTest
        OpenixPartitionTest.cpp
)

target_link_libraries(OpenixPartitionTest
        openiximg
)
target_include_directories(OpenixPartitionTest PUBLIC
        ${CMAKE_SOURCE_DIR}/includes
)

add_test(NAME OpenixPartitionTest COMMAND OpenixPartitionTest)