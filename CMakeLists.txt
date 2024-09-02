cmake_minimum_required(VERSION 3.10)

# 工程名称
project(RecordPCM)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 查找 ALSA 库
find_package(ALSA REQUIRED)

file(GLOB SOURCE_FILES "src/main.cpp" "src/audio_driver.cpp")

# 添加可执行文件
add_executable(RecordPCM ${SOURCE_FILES})

include_directories(${CMAKE_SOURCE_DIR}/include)

# 链接 ALSA 库
target_link_libraries(RecordPCM ${ALSA_LIBRARIES})

# 设置可执行文件的输出路径
set_target_properties(RecordPCM PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)