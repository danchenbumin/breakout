cmake_minimum_required(VERSION 3.10)
project(breakout_week2)

# 找到所有的cpp文件，自动添加到编译列表，不用手动一个个加
file(GLOB SOURCES *.cpp)

# 可执行文件
add_executable(breakout_week2 ${SOURCES})

# 链接raylib库
find_package(raylib REQUIRED)
target_link_libraries(breakout_week2 raylib)

# C++17标准，支持unique_ptr这些特性
target_compile_features(breakout_week2 PRIVATE cxx_std_17)
