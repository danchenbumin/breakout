# 双人联机打砖块游戏
四川大学2025级《面向对象程序设计（C++）》课程最终项目

## 项目简介
这是一个基于Raylib图形库和ENet网络库开发的双人联机打砖块游戏。从最基础的变量和循环开始，逐步实现了面向对象封装、继承多态、模板、文件操作、网络编程等完整的C++核心知识点。通过"知识点+游戏实践"的教学方式，完整体验了从一行代码到一个可交付作品的完整开发流程。

## 环境要求
- 操作系统：WSL Ubuntu 22.04（推荐）
- 编译器：支持C++17标准的GCC/Clang
- 构建工具：CMake >= 3.10
- 依赖库：Raylib >= 4.0、ENet >= 1.3.17

## 编译运行步骤
```bash
# 1. 克隆项目到本地
git clone https://github.com/你的GitHub用户名/breakout.git
cd breakout

# 2. 创建build目录并进入
mkdir -p build
cd build

# 3. 生成编译配置并编译
cmake ..
make -j4

# 4. 运行游戏
./breakout