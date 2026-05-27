cmake_minimum_required(VERSION 3.10)
project(breakout)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找raylib
find_package(raylib REQUIRED)

# 查找ENet
find_library(ENET_LIBRARY enet REQUIRED)
find_path(ENET_INCLUDE_DIR enet/enet.h REQUIRED)

# 添加nlohmann/json头文件目录（把json.hpp放在项目根目录的include文件夹里）
include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(breakout
    src/main.cpp
    src/Ball.cpp
    src/Brick.cpp
    src/Game.cpp
    src/Paddle.cpp
    src/PowerUp.cpp
)

target_link_libraries(breakout raylib ${ENET_LIBRARY})
file(COPY ${PROJECT_SOURCE_DIR}/levels DESTINATION ${PROJECT_BINARY_DIR})