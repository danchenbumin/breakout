# 双人联机打砖块游戏 (Breakout)

> 四川大学 2025级《面向对象程序设计（C++）》课程最终项目  
> 作者：danchenbumin | 2026年5月  
> GitHub：https://github.com/danchenbumin/breakout

---

## 项目简介

这是一个基于 **Raylib 图形库**和 **ENet 网络库**开发的双人联机打砖块游戏。从最基础的变量和循环开始，逐步实现了面向对象封装、继承多态、模板、文件操作、网络编程等完整的C++核心知识点。通过"知识点+游戏实践"的教学方式，完整体验了从一行代码到一个可交付作品的完整开发流程。

### 核心功能

| 功能 | 说明 |
|------|------|
| 🎮 **双人局域网联机** | Host/Client架构，ENet UDP可靠传输，30 tick/s状态同步 |
| 📝 **JSON驱动关卡** | 3个难度递增关卡，布局/颜色/球速均JSON配置 |
| 💾 **实时存档/读档** | 通关自动保存，退出自动保存，启动可选继续 |
| ✏️ **关卡编辑器** | 按E进入编辑模式，鼠标添加/删除砖块，S键保存 |
| 🎨 **粒子特效** | 对象池管理，砖块破碎/道具拾取粒子反馈 |
| 🎯 **道具系统** | 加长板(W)、减速球(S)、额外生命(+) 三种道具 |

### 操作说明

| 按键 | 功能 |
|------|------|
| A / D | 移动球拍（左/右） |
| R | 重置游戏 |
| L | 异步加载资源 |
| E | 进入/退出关卡编辑模式 |
| ESC | 退出游戏 |

---

## 技术架构

### 类继承体系（菱形继承）

```
                    GameObject (位置)
                   /          \
         PhysicalObject     VisualObject
         (速度/碰撞)        (颜色/绘制)
                   \          /
        Ball / Paddle / Brick / PowerUp
```

- 使用 `virtual` 继承解决菱形继承二义性
- 纯虚函数 `Draw()` 实现多态绘制
- 工厂模式创建道具效果，`unique_ptr` 管理生命周期
- 对象池模式管理粒子，单例模式管理纹理缓存

### 设计模式

| 模式 | 应用场景 | C++实现 |
|------|---------|---------|
| 工厂模式 | 道具效果创建 | `unique_ptr<PowerUpEffect>` + 抽象基类 |
| 单例模式 | 纹理缓存 | `static` 局部变量 + `mutex` 线程安全 |
| 对象池模式 | 粒子系统 | 预分配 `Particle[100]` 静态数组 |
| 状态模式 | 游戏流程 | `enum class GameState` |
| 策略模式 | 道具效果 | `PowerUpEffect` 抽象基类多态 |

### 性能优化

| 优化项 | 方案 | 效果 |
|--------|------|------|
| 碰撞检测 | 8×6 空间网格划分 | O(N×M) → 接近 O(1) |
| 粒子管理 | 预分配对象池复用 | 零动态分配，无内存碎片 |
| 资源加载 | `std::async` 异步加载 | 主线程无阻塞 |
| 网络同步 | 双快照线性插值 | 消除UDP抖动 |

---

## 项目结构

```
breakout/
├── src/                    # 源代码
│   ├── main.cpp            # 主程序（类定义 + 游戏循环）
│   ├── Ball.cpp / Ball.h   # 球类
│   ├── Paddle.cpp / Paddle.h  # 球拍类
│   ├── Brick.cpp / Brick.h # 砖块类
│   ├── PowerUp.cpp / PowerUp.h # 道具系统（工厂模式）
│   └── Game.cpp / Game.h   # 游戏管理类（状态机）
├── include/                # 第三方头文件
│   └── json.hpp            # nlohmann/json (header-only)
├── levels/                 # 关卡配置
│   ├── level1.json         # 第一关：入门（5×8 全砖）
│   ├── level2.json         # 第二关：空心（难度提升）
│   └── level3.json         # 第三关：十字（最高难度）
├── build/                  # 编译输出（Git忽略）
├── CMakeLists.txt          # CMake构建配置
├── .gitignore              # Git忽略规则
└── README.md               # 本文件
```

---

## 环境要求

- **操作系统**：WSL Ubuntu 22.04（推荐）或原生 Linux
- **编译器**：支持 C++17 标准的 GCC/Clang
- **构建工具**：CMake >= 3.10
- **依赖库**：Raylib >= 4.0、ENet >= 1.3.17

### 安装依赖（Ubuntu/Debian）

```bash
# Raylib
sudo apt install libraylib-dev

# ENet
sudo apt install libenet-dev

# nlohmann/json (已包含在项目的 include/ 目录中)
# 也可通过 apt 安装: sudo apt install nlohmann-json3-dev
```

---

## 编译运行

```bash
# 1. 克隆项目
git clone https://github.com/danchenbumin/breakout.git
cd breakout

# 2. 编译
mkdir -p build && cd build
cmake ..
make -j4

# 3. 运行（双人模式：先启动Host，再启动Client）
./breakout
# 选择模式：1 = Host（先运行），2 = Client（后运行）
```

---

## 开发历程

项目采用**渐进式迭代开发**，13周完成从基础框架到完整游戏：

| 周次 | 里程碑 | 提交 |
|------|--------|------|
| W1-3 | 基础框架搭建 | 首次提交 |
| W4-6 | 面向对象封装与类设计 | 类重构 |
| W7 | 游戏状态机与碰撞检测 | `92eb217` |
| W8 | 道具系统 + 粒子特效 + JSON配置 | `5474aa0` |
| W9 | ENet局域网双人联机 | `fd13840` |
| W10 | 多线程异步加载 | `cd9cfc6` |
| W11 | 性能优化（网格碰撞 + 对象池） | `2095eaf` |
| W12 | 数据持久化（存档 + 关卡编辑器） | `903ccb7` |
| W13 | 项目收尾与工程整理 | `ec441a2` |

Git提交遵循 [Conventional Commits](https://www.conventionalcommits.org/) 规范。

---

## AI 辅助开发说明

本项目在开发过程中使用了以下AI工具辅助开发（总占比约10%）：

- **GitHub Copilot**：代码补全、模板生成（~5%）
- **ChatGPT / Claude**：架构咨询、Bug分析（~5%）

所有AI生成的代码均经过人工审核和修改，关键AI辅助部分在代码注释中已标注。AI使用遵循"理解为主、AI为辅"的原则。

---

## 许可证

本项目为课程作业项目，仅供学习和参考。

---

## 相关文档

- [项目设计报告](项目设计报告.md) — 完整的技术文档与设计说明
- [汇报PPT内容](汇报PPT内容.md) — 最终路演PPT文稿
- [双人联机打砖块游戏_最终路演.pptx](双人联机打砖块游戏_最终路演.pptx) — 路演演示文稿