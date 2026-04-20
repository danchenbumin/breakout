cmake_minimum_required(VERSION 3.10)
project(breakout)

# 只编译main.cpp，杜绝旧文件重复定义内存崩溃
add_executable(breakout main.cpp)

# 链接本地安装好的raylib
find_package(raylib REQUIRED)
target_link_libraries(breakout raylib)

# pkg-config链接ENet网络库
find_package(PkgConfig REQUIRED)
pkg_check_modules(ENET REQUIRED libenet)
target_include_directories(breakout PRIVATE ${ENET_INCLUDE_DIRS})
target_link_libraries(breakout ${ENET_LIBRARIES})

target_compile_features(breakout PRIVATE cxx_std_17)
