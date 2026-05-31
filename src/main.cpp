#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <enet/enet.h>
#include "raylib.h"
#include "nlohmann/json.hpp"

using namespace std;
using json = nlohmann::json;
namespace fs = std::filesystem;

// ====================== 菱形继承类体系 ======================
/**
 * @brief 游戏对象基类 — 菱形继承体系的虚基类
 * @details 所有游戏对象（球、球拍、砖块、道具）均继承自此基类。
 *          PhysicalObject 和 VisualObject 通过 virtual 继承避免二义性。
 *          参见 PPT 第3页的菱形继承类图。
 */
class GameObject {
public:
    GameObject(Vector2 pos = {0, 0}) : position(pos) {}
    Vector2 position; ///< 对象在屏幕上的位置
};

/**
 * @brief 物理对象 — 拥有速度和碰撞能力
 * @details virtual 继承 GameObject，与 VisualObject 共同构成菱形继承的两条分支。
 *          提供 Move() 移动和 BounceEdge() 边界反弹的默认实现。
 */
class PhysicalObject : virtual public GameObject {
public:
    PhysicalObject(Vector2 pos = {0, 0}, Vector2 vel = {0, 0}, float r = 0)
        : GameObject(pos), velocity(vel), radius(r) {}
    Vector2 velocity; ///< 对象速度向量
    float radius;     ///< 碰撞半径

    /** @brief 移动对象：position += velocity */
    void Move() {
        position.x += velocity.x;
        position.y += velocity.y;
    }

    /**
     * @brief 边界反弹检测
     * @param screenWidth 屏幕宽度
     * @param screenHeight 屏幕高度
     * @details 碰左右边界或上边界时反转对应速度分量，底边界不反弹
     */
    void BounceEdge(int screenWidth, int screenHeight) {
        if (position.x - radius <= 5 || position.x + radius >= screenWidth - 5) {
            velocity.x *= -1;
        }
        if (position.y - radius <= 5) {
            velocity.y *= -1;
        }
    }
};

/**
 * @brief 可视对象 — 拥有颜色和绘制能力
 * @details virtual 继承 GameObject。定义纯虚函数 Draw()，
 *          子类必须实现自己的绘制逻辑（多态绘制）。
 */
class VisualObject : virtual public GameObject {
public:
    VisualObject(Vector2 pos = {0, 0}, Color c = WHITE, bool vis = true)
        : GameObject(pos), color(c), visible(vis) {}
    Color color;    ///< 对象颜色
    bool visible;   ///< 是否可见
    virtual void Draw() const = 0; ///< 纯虚函数：多态绘制接口
};

/**
 * @brief 球类 — 菱形继承自 PhysicalObject 和 VisualObject
 * @details 继承移动/碰撞能力和绘制能力。默认速度(3,-3)向右下方运动。
 */
class Ball : public PhysicalObject, public VisualObject {
public:
    Ball(Vector2 pos = {400, 300}, Vector2 vel = {3, -3}, float r = 10, Color c = RED)
        : GameObject(pos), PhysicalObject(pos, vel, r), VisualObject(pos, c, true) {}

    /** @brief 多态绘制：用 DrawCircleV 绘制圆形球 */
    void Draw() const override {
        if (visible) DrawCircleV(position, radius, color);
    }
};

/**
 * @brief 球拍类 — 菱形继承自 PhysicalObject 和 VisualObject
 * @details 玩家通过 A/D 键或方向键控制球拍左右移动。
 *          支持加长道具效果（Widen）和恢复（ResetWidth）。
 */
class Paddle : public PhysicalObject, public VisualObject {
public:
    Paddle(Vector2 pos = {0, 0}, float w = 100, float h = 20, Color c = BLUE)
        : GameObject(pos), PhysicalObject(pos, {0, 0}, 0), VisualObject(pos, c, true),
          width(w), height(h), originalWidth(w) {}

    float width;         ///< 当前宽度（道具效果可临时增加）
    float height;        ///< 球拍高度
    float originalWidth; ///< 原始宽度，用于道具效果结束后恢复

    /** @brief 向左移动，边界锁定≥5像素 */
    void MoveLeft(float speed) {
        position.x -= speed;
        if (position.x < 5) position.x = 5;
    }

    /** @brief 向右移动，边界锁定≤795-width像素 */
    void MoveRight(float speed) {
        position.x += speed;
        if (position.x + width > 795) position.x = 795 - width;
    }

    /** @brief 恢复原始宽度（道具效果结束后调用） */
    void ResetWidth() {
        width = originalWidth;
    }

    /** @brief 加长球拍：宽度×1.5（道具效果） */
    void Widen() {
        width = originalWidth * 1.5f;
    }

    /** @brief 多态绘制：用 DrawRectangle 绘制矩形球拍 */
    void Draw() const override {
        if (visible) DrawRectangle(position.x, position.y, width, height, color);
    }
};

/**
 * @brief 砖块类 — 菱形继承自 PhysicalObject 和 VisualObject
 * @details active 标志控制砖块是否还存在。被球击中后 active=false，绘制时跳过。
 *          type 字段用于关卡布局配置（0=空, 1=普通砖等）。
 */
class Brick : public PhysicalObject, public VisualObject {
public:
    Brick(Vector2 pos = {0, 0}, float w = 85, float h = 25, Color c = BLUE, int t = 1)
        : GameObject(pos), PhysicalObject(pos, {0, 0}, 0), VisualObject(pos, c, true),
          width(w), height(h), active(true), type(t) {}

    float width;  ///< 砖块宽度
    float height; ///< 砖块高度
    bool active;  ///< 砖块是否仍存在
    int type;     ///< 砖块类型（用于JSON关卡布局）

    /** @brief 多态绘制：活跃砖块绘制填充矩形+白色边框 */
    void Draw() const override {
        if (visible && active) {
            DrawRectangle(position.x, position.y, width, height, color);
            DrawRectangleLines(position.x, position.y, width, height, WHITE);
        }
    }
};

/**
 * @brief 道具类 — 菱形继承自 PhysicalObject 和 VisualObject
 * @details 道具从砖块位置掉落，碰到球拍触发效果。
 *          type=1: 加长板(W), type=2: 减速球(S), type=3: 额外生命(+)。
 */
class PowerUp : public PhysicalObject, public VisualObject {
public:
    PowerUp(Vector2 pos, int type) : PhysicalObject(pos, {0, 2}, 15),
        VisualObject(pos, WHITE, true), type(type), active(true) {
        if (type == 1) color = PURPLE;
        else if (type == 2) color = YELLOW;
        else if (type == 3) color = GREEN;
    }

    int type;   ///< 道具类型：1=加长板, 2=减速球, 3=额外生命
    bool active; ///< 道具是否仍在游戏中

    /** @brief 更新：向下移动（速度2像素/帧） */
    void Update() {
        Move();
    }

    /** @brief 多态绘制：绘制圆形+类型文字标识 */
    void Draw() const override {
        if (active && visible) {
            DrawCircleV(position, radius, color);
            DrawText(type == 1 ? "W" : type == 2 ? "S" : "+", position.x-5, position.y-5, 12, WHITE);
        }
    }
};
// ======================================================================

// ====================== JSON工具函数 ======================
/**
 * @brief 将颜色名称字符串转换为 Raylib Color
 * @param name 颜色名称（如 "red", "blue"）
 * @return 对应的 Color 结构体，未知名称返回 BLUE
 * @details 支持9种颜色映射，用于解析JSON关卡配置中的color_map
 */
Color GetColorFromName(const string& name) {
    static unordered_map<string, Color> colorMap = {
        {"red", RED}, {"orange", ORANGE}, {"yellow", YELLOW},
        {"green", GREEN}, {"blue", BLUE}, {"purple", PURPLE},
        {"cyan", Color{0, 255, 255, 255}}, {"white", WHITE}, {"gray", GRAY}
    };
    if (colorMap.count(name)) return colorMap[name];
    return BLUE;
}

/**
 * @brief 加载JSON文件（带错误降级处理）
 * @param path JSON文件路径
 * @param fallback 文件缺失或解析失败时使用的默认配置
 * @return 解析成功返回配置，失败返回fallback
 * @details 文件不存在时输出 LOG_WARNING，解析错误时输出 LOG_ERROR，
 *          均自动降级为fallback，确保游戏不会因配置文件问题而崩溃。
 * @note 这是错误降级机制的关键函数，体现了异常处理与RAII思想
 */
json LoadJSONWithFallback(const string& path, const json& fallback) {
    try {
        ifstream file(path);
        if (!file.is_open()) {
            TraceLog(LOG_WARNING, "文件不存在: %s, 使用默认配置", path.c_str());
            return fallback;
        }
        json config;
        file >> config;
        return config;
    } catch (const json::parse_error& e) {
        TraceLog(LOG_ERROR, "JSON解析失败: %s, 使用默认配置", e.what());
        return fallback;
    }
}

/**
 * @brief 保存JSON数据到文件
 * @param path 目标文件路径
 * @param data 要保存的JSON对象
 * @details 使用 dump(4) 格式化输出，缩进4空格，增强可读性。
 *          写入失败时通过 TraceLog 报告错误。
 */
void SaveJSON(const string& path, const json& data) {
    try {
        ofstream file(path);
        file << data.dump(4);
        TraceLog(LOG_INFO, "文件保存成功: %s", path.c_str());
    } catch (const exception& e) {
        TraceLog(LOG_ERROR, "文件保存失败: %s", e.what());
    }
}
// ======================================================================

// ====================== 存档系统 ======================
/**
 * @brief 存档数据结构
 * @details version 字段用于存档格式版本管理，升级游戏时可兼容旧版存档。
 */
struct SaveData {
    int version = 1;      ///< 存档格式版本号
    int currentLevel = 1; ///< 当前关卡编号
    int score = 0;        ///< 当前分数
    int lives = 3;        ///< 剩余生命
};

/**
 * @brief 检测存档文件是否存在
 * @return true 存档存在, false 不存在
 */
bool SaveExists() {
    return fs::exists("save.json");
}

/**
 * @brief 加载存档数据
 * @return SaveData 包含关卡、分数、生命信息
 * @details 使用 LoadJSONWithFallback 进行容错加载，
 *          存档缺失时返回默认值（第1关、0分、3条命）
 */
SaveData LoadSave() {
    SaveData data;
    json save = LoadJSONWithFallback("save.json", json::object());

    if (save.contains("version") && save["version"] == 1) {
        data.currentLevel = save.value("current_level", 1);
        data.score = save.value("score", 0);
        data.lives = save.value("lives", 3);
    }
    return data;
}

/**
 * @brief 保存游戏进度到 save.json
 * @param data 包含当前关卡、分数、生命的存档数据
 * @details 通关自动保存，退出自动保存。使用JSON格式便于调试和手动编辑。
 */
void SaveGame(const SaveData& data) {
    json save;
    save["version"] = data.version;
    save["current_level"] = data.currentLevel;
    save["score"] = data.score;
    save["lives"] = data.lives;
    SaveJSON("save.json", save);
}
// ======================================================================

// ====================== 性能测量工具 ======================
struct PerformanceStats {
    double updateTime;
    double drawTime;
    double collisionTime;
    double particleTime;
    double networkTime;
    int frameCount;
    double totalTime;
};

PerformanceStats g_stats = {0};
double g_lastStatsPrint = 0.0;

#define MEASURE_BLOCK_START() double __start = GetTime()
#define MEASURE_BLOCK_END(var) var += (GetTime() - __start) * 1000

void PrintPerformanceStats() {
    double now = GetTime();
    if (now - g_lastStatsPrint >= 10.0) {
        if (g_stats.frameCount > 0) {
            TraceLog(LOG_INFO, "=== 性能统计（平均每帧，单位ms） ===");
            TraceLog(LOG_INFO, "总更新耗时: %.2f", g_stats.updateTime / g_stats.frameCount);
            TraceLog(LOG_INFO, "  碰撞检测: %.2f", g_stats.collisionTime / g_stats.frameCount);
            TraceLog(LOG_INFO, "  粒子更新: %.2f", g_stats.particleTime / g_stats.frameCount);
            TraceLog(LOG_INFO, "  网络同步: %.2f", g_stats.networkTime / g_stats.frameCount);
            TraceLog(LOG_INFO, "绘制耗时: %.2f", g_stats.drawTime / g_stats.frameCount);
            TraceLog(LOG_INFO, "平均帧率: %.1f", g_stats.frameCount / (now - g_lastStatsPrint));
            TraceLog(LOG_INFO, "===================================");
        }
        g_stats = {0};
        g_lastStatsPrint = now;
    }
    g_stats.frameCount++;
}
// ======================================================================

// ====================== 粒子对象池 ======================
const int MAX_PARTICLES = 100; ///< 对象池最大粒子数

/**
 * @brief 粒子结构体（对象池元素）
 * @details 预分配100个粒子的静态数组，通过active标志管理复用。
 *          对象池模式避免动态分配造成的性能开销和内存碎片。
 */
struct Particle {
    Vector2 position; ///< 当前位置
    Vector2 velocity; ///< 当前速度
    Color color;      ///< 粒子颜色
    float life;       ///< 剩余生命（秒）
    float maxLife;    ///< 初始生命（用于计算alpha渐变）
    bool active;      ///< 粒子是否活跃（对象池标志位）
};

Particle g_particlePool[MAX_PARTICLES]; ///< 全局粒子对象池

/** @brief 初始化对象池：将所有粒子标记为非活跃 */
void InitParticlePool() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        g_particlePool[i].active = false;
    }
}

/**
 * @brief 从对象池分配一个粒子
 * @param pos 初始位置
 * @param vel 初始速度
 * @param color 粒子颜色
 * @param life 生命时长（秒）
 * @details 线性扫描空闲槽位，O(1)分配（100个固定扫描）。
 *          对象池满时静默丢弃（实际使用中不会超过100）。
 */
void SpawnParticle(Vector2 pos, Vector2 vel, Color color, float life) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!g_particlePool[i].active) {
            g_particlePool[i].position = pos;
            g_particlePool[i].velocity = vel;
            g_particlePool[i].color = color;
            g_particlePool[i].life = life;
            g_particlePool[i].maxLife = life;
            g_particlePool[i].active = true;
            return;
        }
    }
}

/**
 * @brief 更新所有活跃粒子
 * @param dt 帧时间差（秒）
 * @details 逐帧更新位置，生命归零时自动标记为非活跃（释放回池）。
 *          性能由 MEASURE_BLOCK 宏统计。
 */
void UpdateParticles(float dt) {
    MEASURE_BLOCK_START();
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (g_particlePool[i].active) {
            g_particlePool[i].position.x += g_particlePool[i].velocity.x;
            g_particlePool[i].position.y += g_particlePool[i].velocity.y;
            g_particlePool[i].life -= dt;
            if (g_particlePool[i].life <= 0) {
                g_particlePool[i].active = false;
            }
        }
    }
    MEASURE_BLOCK_END(g_stats.particleTime);
}

/**
 * @brief 绘制所有活跃粒子
 * @details 粒子透明度随剩余生命比例渐变（alpha = life/maxLife），实现淡出效果
 */
void DrawParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (g_particlePool[i].active) {
            float alpha = g_particlePool[i].life / g_particlePool[i].maxLife;
            DrawCircleV(g_particlePool[i].position, 3, Color{
                g_particlePool[i].color.r,
                g_particlePool[i].color.g,
                g_particlePool[i].color.b,
                (unsigned char)(alpha * 255)
            });
        }
    }
}

/**
 * @brief 批量生成粒子（砖块破碎特效）
 * @param pos 中心位置
 * @param color 粒子颜色（取砖块颜色）
 * @param count 生成数量
 * @details 每个粒子随机速度方向，生命1秒，用于砖块破碎/道具拾取的视觉反馈
 */
void CreateParticles(Vector2 pos, Color color, int count) {
    uniform_real_distribution<float> dist(-3, 3);
    random_device rd;
    mt19937 rng(rd());
    for (int i = 0; i < count; i++) {
        SpawnParticle(pos, {dist(rng), dist(rng)}, color, 1.0f);
    }
}
// ======================================================================

// ====================== 空间网格碰撞检测 ======================
/**
 * @details 将屏幕划分为8×6=48个100×100像素的网格单元。
 *          球只需检测所在格及相邻8格（共9格）内的砖块，
 *          将碰撞检测复杂度从 O(N×M) 降至接近 O(1)。
 *          这是课程要求的关键性能优化技术。
 */
const int GRID_WIDTH = 8;   ///< 网格列数（800÷100）
const int GRID_HEIGHT = 6;  ///< 网格行数（600÷100）
const int CELL_WIDTH = 100; ///< 每格宽度（像素）
const int CELL_HEIGHT = 100;///< 每格高度（像素）

vector<Brick*> g_grid[GRID_WIDTH][GRID_HEIGHT]; ///< 空间网格：每格存储砖块指针
bool CheckBallBrickCollision(Ball& ball, vector<Brick>& bricks, int& score);

/**
 * @brief 更新空间网格 — 每帧重新分配砖块到对应网格单元
 * @param bricks 当前活跃砖块列表
 * @details 每帧先清空网格，再将活跃砖块按位置分配。
 *          这是空间换时间的典型应用。
 */
void UpdateGrid(const vector<Brick>& bricks) {
    for (int i = 0; i < GRID_WIDTH; i++) {
        for (int j = 0; j < GRID_HEIGHT; j++) {
            g_grid[i][j].clear();
        }
    }
    for (const auto& brick : bricks) {
        if (brick.active) {
            int gx = brick.position.x / CELL_WIDTH;
            int gy = brick.position.y / CELL_HEIGHT;
            if (gx >= 0 && gx < GRID_WIDTH && gy >= 0 && gy < GRID_HEIGHT) {
                g_grid[gx][gy].push_back((Brick*)&brick);
            }
        }
    }
}

/**
 * @brief 球与砖块碰撞检测（空间网格优化版）
 * @param ball 球对象引用
 * @param bricks 砖块列表（未使用，碰撞检测通过网格完成）
 * @param score 分数引用，击中砖块+10
 * @return true 击中砖块, false 未击中
 * @details 使用圆与矩形碰撞算法（CheckCollisionCircleRec）。
 *          仅检测球所在网格单元及其周围8个相邻格（共最多9格）。
 *          击中后：标记砖块非活跃、加分、反转Y速度、生成粒子。
 *          CPU占用从原始遍历的~30%降至<5%。
 * @note 本函数不修改球的速度X分量，只反转Y方向
 */
bool CheckBallBrickCollision(Ball& ball, vector<Brick>& bricks, int& score) {
    MEASURE_BLOCK_START();
    int gx = ball.position.x / CELL_WIDTH;
    int gy = ball.position.y / CELL_HEIGHT;

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int checkX = gx + dx;
            int checkY = gy + dy;
            if (checkX >= 0 && checkX < GRID_WIDTH && checkY >= 0 && checkY < GRID_HEIGHT) {
                for (auto brick : g_grid[checkX][checkY]) {
                    if (brick->active && CheckCollisionCircleRec(ball.position, ball.radius,
                        {brick->position.x, brick->position.y, brick->width, brick->height})) {
                        brick->active = false;
                        score += 10;
                        ball.velocity.y *= -1;
                        CreateParticles(brick->position, brick->color, 10);
                        MEASURE_BLOCK_END(g_stats.collisionTime);
                        return true;
                    }
                }
            }
        }
    }
    MEASURE_BLOCK_END(g_stats.collisionTime);
    return false;
}
// ======================================================================

// ====================== 关卡编辑器 ======================
bool g_editingMode = false;    ///< 是否处于编辑模式
int g_selectedBrickType = 1;   ///< 当前选择的砖块类型

/** @brief 切换编辑模式：E键触发，模式切换时输出日志 */
void ToggleEditingMode() {
    g_editingMode = !g_editingMode;
    if (g_editingMode) {
        TraceLog(LOG_INFO, "进入编辑模式：鼠标左键添加砖块，右键删除，按S保存");
    } else {
        TraceLog(LOG_INFO, "退出编辑模式");
    }
}

/**
 * @brief 编辑模式更新 — 处理鼠标增删砖块操作
 * @param bricks 砖块列表引用
 * @param brickWidth 标准砖块宽度（用于网格对齐）
 * @param brickHeight 标准砖块高度（用于网格对齐）
 * @details 左键：在鼠标位置添加砖块（自动对齐网格+重叠检测+边界保护）。
 *          右键：删除鼠标悬停的砖块。
 *          S键：保存当前砖块布局到 levels/custom.json。
 */
void UpdateEditingMode(vector<Brick>& bricks, float brickWidth, float brickHeight) {
    if (!g_editingMode) return;

    Vector2 mouse = GetMousePosition();
    
    // 鼠标左键添加砖块
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // 对齐到网格
        float x = floor(mouse.x / brickWidth) * brickWidth;
        float y = floor(mouse.y / brickHeight) * brickHeight;
        
        // 检查是否已有砖块
        bool exists = false;
        for (auto& brick : bricks) {
            if (fabs(brick.position.x - x) < 1 && fabs(brick.position.y - y) < 1) {
                exists = true;
                break;
            }
        }
        
        if (!exists && x >= 5 && x + brickWidth <= 795 && y >= 100 && y + brickHeight <= 500) {
            bricks.emplace_back(Vector2{x, y}, brickWidth, brickHeight, PURPLE, g_selectedBrickType);
        }
    }
    
    // 鼠标右键删除砖块
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        for (auto it = bricks.begin(); it != bricks.end();) {
            if (CheckCollisionPointRec(mouse, {it->position.x, it->position.y, it->width, it->height})) {
                it = bricks.erase(it);
            } else {
                it++;
            }
        }
    }
    
    // 按S保存当前关卡
    if (IsKeyPressed(KEY_S)) {
        json level;
        level["version"] = 1;
        level["name"] = "自定义关卡";
        level["ball_speed"] = 3.0;
        level["bricks"]["rows"] = 5;
        level["bricks"]["cols"] = 8;
        level["bricks"]["width"] = 85;
        level["bricks"]["height"] = 25;
        level["bricks"]["offset_x"] = 50;
        level["bricks"]["offset_y"] = 100;
        
        // 初始化布局为全0
        vector<vector<int>> layout(5, vector<int>(8, 0));
        for (const auto& brick : bricks) {
            int col = (brick.position.x - 50) / 90;
            int row = (brick.position.y - 100) / 35;
            if (row >= 0 && row < 5 && col >= 0 && col < 8) {
                layout[row][col] = brick.type;
            }
        }
        level["bricks"]["layout"] = layout;
        level["bricks"]["color_map"]["1"] = "purple";
        
        SaveJSON("levels/custom.json", level);
    }
}
// ======================================================================

// ====================== 多线程异步加载 ======================
/**
 * @brief 异步加载状态枚举
 */
enum class LoadState { IDLE, LOADING, DONE };
LoadState g_loadState = LoadState::IDLE;    ///< 当前加载状态
mutex g_stateMutex;                          ///< 保护加载状态的互斥锁
vector<Color> g_newBrickColors;              ///< 异步加载结果
future<vector<Color>> g_loadFuture;          ///< 异步任务future

/**
 * @brief 纹理缓存 — 单例模式 + 线程安全
 * @details 使用 static 局部变量的 Meyers Singleton 实现。
 *          内部使用 mutex 保护缓存读写，保证多线程环境下的安全性。
 *          GetInstance() 返回单例引用，GetTexture() 惰性加载+缓存复用。
 */
class TextureCache {
private:
    unordered_map<string, Texture2D> cache;
    mutable mutex mtx;
    TextureCache() = default;
public:
    /** @brief 获取单例实例（线程安全的 Meyers Singleton） */
    static TextureCache& GetInstance() {
        static TextureCache instance;
        return instance;
    }

    /**
     * @brief 获取纹理（惰性加载+缓存）
     * @param path 纹理文件路径
     * @return 纹理对象，已缓存则直接返回
     */
    Texture2D GetTexture(const string& path) {
        lock_guard<mutex> lock(mtx);
        auto it = cache.find(path);
        if (it != cache.end()) return it->second;
        Texture2D tex = LoadTexture(path.c_str());
        cache[path] = tex;
        return tex;
    }

    /** @brief 清空所有已缓存纹理并释放GPU资源 */
    void ClearCache() {
        lock_guard<mutex> lock(mtx);
        for (auto& pair : cache) UnloadTexture(pair.second);
        cache.clear();
    }
};

/**
 * @brief 异步加载大量资源 — 模拟耗时资源加载
 * @return 生成的5个随机颜色
 * @details 通过 std::async 在后台线程执行，sleep(500ms)模拟IO延迟。
 *          L键触发，加载完成后自动应用新颜色到当前关卡。
 */
vector<Color> AsyncLoadLargeResource() {
    this_thread::sleep_for(chrono::milliseconds(500));
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> colorDist(0, 255);
    vector<Color> newColors;
    for (int i = 0; i < 5; i++) {
        newColors.emplace_back(Color{
            (unsigned char)colorDist(rng),
            (unsigned char)colorDist(rng),
            (unsigned char)colorDist(rng),
            255
        });
    }
    return newColors;
}

// ====================== 网络同步 ======================
/**
 * @brief 游戏状态结构体 — 用于网络同步的数据包
 * @details 包含所有需要在Host和Client之间同步的游戏状态。
 *          #pragma pack(1) 确保结构体紧凑无填充，减少网络传输量。
 */
#pragma pack(1)
struct GameState {
    float ballX, ballY;
    float ballSpeedX, ballSpeedY;
    float paddle1X;
    float paddle2X;
    int score;
    int lives;
    bool gameOver;
    bool victory;
    bool powerUpActive;
    float powerUpX, powerUpY;
    int powerUpType;
    int currentLevel;
    bool editingMode;
};
#pragma pack()

/**
 * @brief 快照结构体 — 用于网络状态插值
 * @details 存储游戏状态和时间戳，双快照插值算法消除网络抖动。
 *          lastSnapshot + nextSnapshot + 当前时间 → 线性插值 → 平滑画面。
 */
struct Snapshot {
    GameState state;  ///< 游戏状态数据
    double timestamp; ///< 快照时间戳
};

// 全局变量
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PORT = 12345;
const float NETWORK_TICK_RATE = 1.0f / 30.0f;
const int TOTAL_LEVELS = 3;

bool isHost = false;
ENetHost* host = nullptr;
ENetPeer* peer = nullptr;

GameState currentState;
Snapshot lastSnapshot, nextSnapshot;
double lastNetworkTime = 0.0f;

vector<Brick> bricks;
Ball ball({400, 300}, {3, -3}, 10, RED);
Paddle paddle1({350, 550}, 100, 20, BLUE);
Paddle paddle2({350, 30}, 100, 20, GREEN);
PowerUp* powerUp = nullptr;

mt19937 rng(random_device{}());
float g_brickWidth = 85;
float g_brickHeight = 25;

/**
 * @brief 从JSON加载关卡 — 数据驱动关卡系统的核心函数
 * @param level 关卡编号（1-3）
 * @details 读取 levels/level{level}.json 配置文件，
 *          解析砖块布局、颜色映射、球速等参数并生成砖块阵列。
 *          文件缺失或格式错误时自动降级为默认5×8全砖布局。
 *          加载后自动重置球、球拍位置和粒子池。
 * @note 课程PDF点名要求本函数添加Doxygen注释（示例函数之一）
 */
    bricks.clear();
    string filename = "levels/level" + to_string(level) + ".json";
    
    // 默认关卡配置
    json defaultConfig = R"({
        "version": 1,
        "name": "默认关卡",
        "ball_speed": 3.0,
        "bricks": {
            "rows": 5,
            "cols": 8,
            "width": 85,
            "height": 25,
            "offset_x": 50,
            "offset_y": 100,
            "layout": [
                [1,1,1,1,1,1,1,1],
                [1,1,1,1,1,1,1,1],
                [1,1,1,1,1,1,1,1],
                [1,1,1,1,1,1,1,1],
                [1,1,1,1,1,1,1,1]
            ],
            "color_map": {"1": "red"}
        }
    })"_json;
    
    json config = LoadJSONWithFallback(filename, defaultConfig);
    
    float ballSpeed = config.value("ball_speed", 3.0f);
    int rows = config["bricks"].value("rows", 5);
    int cols = config["bricks"].value("cols", 8);
    g_brickWidth = config["bricks"].value("width", 85.0f);
    g_brickHeight = config["bricks"].value("height", 25.0f);
    float offsetX = config["bricks"].value("offset_x", 50.0f);
    float offsetY = config["bricks"].value("offset_y", 100.0f);
    
    auto layout = config["bricks"]["layout"];
    auto colorMap = config["bricks"]["color_map"];
    
    unordered_map<int, Color> brickColors;
    for (auto& [key, value] : colorMap.items()) {
        int type = stoi(key);
        brickColors[type] = GetColorFromName(value.get<string>());
    }
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int type = layout[i][j];
            if (type != 0) {
                Color color = brickColors.count(type) ? brickColors[type] : BLUE;
                float x = offsetX + j * (g_brickWidth + 5);
                float y = offsetY + i * (g_brickHeight + 10);
                bricks.emplace_back(Vector2{x, y}, g_brickWidth, g_brickHeight, color, type);
            }
        }
    }
    
    // 重置球和板子
    ball.position = {400, 300};
    ball.velocity = {ballSpeed, -ballSpeed};
    paddle1.position = {350, 550};
    paddle1.ResetWidth();
    paddle2.position = {350, 30};
    paddle2.ResetWidth();
    if (powerUp) { delete powerUp; powerUp = nullptr; }
    InitParticlePool();
    
    currentState.currentLevel = level;
    UpdateGrid(bricks);
    
    TraceLog(LOG_INFO, "加载关卡 %d: %s", level, config["name"].get<string>().c_str());
}
// ======================================================================

/**
 * @brief 重置游戏 — 重新加载当前关卡并清零分数/生命
 * @details 保留当前关卡编号，重新生成砖块，清零分数和生命，
 *          重置所有道具、粒子、异步加载状态。
 */
void ResetGame() {
    LoadLevel(currentState.currentLevel);
    currentState.score = 0;
    currentState.lives = 3;
    currentState.gameOver = false;
    currentState.victory = false;
    currentState.powerUpActive = false;
    lock_guard<mutex> lock(g_stateMutex);
    g_loadState = LoadState::IDLE;
    g_newBrickColors.clear();
}

/**
 * @brief 主机逻辑更新 — Host端的每帧更新
 * @details 处理：编辑模式切换、玩家输入(A/D移动)、异步加载触发、
 *          球移动/碰撞/道具/粒子、通关检测与自动存档。
 *          更新后将状态同步到 currentState 供网络广播。
 */
void UpdateHostLogic() {
    MEASURE_BLOCK_START();
    float dt = GetFrameTime();
    
    // 编辑模式切换
    if (IsKeyPressed(KEY_E)) {
        ToggleEditingMode();
        currentState.editingMode = g_editingMode;
    }
    
    // 编辑模式更新
    UpdateEditingMode(bricks, g_brickWidth, g_brickHeight);
    
    if (!g_editingMode) {
        if (IsKeyDown(KEY_A)) paddle1.MoveLeft(5.0f);
        if (IsKeyDown(KEY_D)) paddle1.MoveRight(5.0f);

        // 异步加载触发
        if (IsKeyPressed(KEY_L)) {
            lock_guard<mutex> lock(g_stateMutex);
            if (g_loadState == LoadState::IDLE) {
                g_loadState = LoadState::LOADING;
                g_loadFuture = async(launch::async, AsyncLoadLargeResource);
            }
        }

        // 检查异步加载完成
        {
            lock_guard<mutex> lock(g_stateMutex);
            if (g_loadState == LoadState::LOADING) {
                if (g_loadFuture.wait_for(chrono::seconds(0)) == future_status::ready) {
                    g_newBrickColors = g_loadFuture.get();
                    g_loadState = LoadState::DONE;
                    // 重新加载当前关卡，使用新颜色
                    LoadLevel(currentState.currentLevel);
                }
            }
        }

        ball.Move();
        ball.BounceEdge(SCREEN_WIDTH, SCREEN_HEIGHT);

        // 球与板子碰撞
        if (CheckCollisionCircleRec(ball.position, ball.radius,
            {paddle1.position.x, paddle1.position.y, paddle1.width, paddle1.height})) {
            ball.velocity.y = -fabs(ball.velocity.y);
            ball.position.y = paddle1.position.y - ball.radius - 1;
            CreateParticles(ball.position, BLUE, 5);
        }

        if (CheckCollisionCircleRec(ball.position, ball.radius,
            {paddle2.position.x, paddle2.position.y, paddle2.width, paddle2.height})) {
            ball.velocity.y = fabs(ball.velocity.y);
            ball.position.y = paddle2.position.y + paddle2.height + ball.radius + 1;
            CreateParticles(ball.position, GREEN, 5);
        }

        // 球与砖块碰撞
        CheckBallBrickCollision(ball, bricks, currentState.score);
        UpdateGrid(bricks);

        // 道具更新
        if (powerUp && powerUp->active) {
            powerUp->Update();
            if (CheckCollisionCircleRec(powerUp->position, powerUp->radius,
                {paddle1.position.x, paddle1.position.y, paddle1.width, paddle1.height})) {
                if (powerUp->type == 1) paddle1.Widen();
                else if (powerUp->type == 2) ball.velocity.x *= 1.2f, ball.velocity.y *= 1.2f;
                else if (powerUp->type == 3) currentState.lives++;
                powerUp->active = false;
                CreateParticles(powerUp->position, powerUp->color, 15);
                powerUp = nullptr;
            } else if (CheckCollisionCircleRec(powerUp->position, powerUp->radius,
                {paddle2.position.x, paddle2.position.y, paddle2.width, paddle2.height})) {
                if (powerUp->type == 1) paddle2.Widen();
                else if (powerUp->type == 2) ball.velocity.x *= 1.2f, ball.velocity.y *= 1.2f;
                else if (powerUp->type == 3) currentState.lives++;
                powerUp->active = false;
                CreateParticles(powerUp->position, powerUp->color, 15);
                powerUp = nullptr;
            } else if (powerUp->position.y > SCREEN_HEIGHT) {
                powerUp->active = false;
                powerUp = nullptr;
            }
        }

        // 随机掉落道具
        static uniform_int_distribution<int> dist(1, 5);
        static uniform_int_distribution<int> typeDist(1, 3);
        if (dist(rng) == 1 && !powerUp && !bricks.empty()) {
            for (const auto& brick : bricks) {
                if (!brick.active) {
                    powerUp = new PowerUp(brick.position, typeDist(rng));
                    break;
                }
            }
        }

        UpdateParticles(dt);

        // 球出界
        if (ball.position.y > SCREEN_HEIGHT || ball.position.y < 0) {
            currentState.lives--;
            if (currentState.lives <= 0) {
                currentState.gameOver = true;
            } else {
                ball.position = {400, 300};
                ball.velocity = {3, -3};
                paddle1.ResetWidth();
                paddle2.ResetWidth();
            }
        }

        // 检查通关
        bool allBricksBroken = true;
        for (const auto& brick : bricks) {
            if (brick.active) {
                allBricksBroken = false;
                break;
            }
        }
        
        if (allBricksBroken) {
            if (currentState.currentLevel < TOTAL_LEVELS) {
                // 自动加载下一关
                currentState.currentLevel++;
                LoadLevel(currentState.currentLevel);
                // 自动保存进度
                SaveData save;
                save.currentLevel = currentState.currentLevel;
                save.score = currentState.score;
                save.lives = currentState.lives;
                SaveGame(save);
            } else {
                currentState.victory = true;
                // 通关后删除存档
                if (fs::exists("save.json")) {
                    fs::remove("save.json");
                }
            }
        }
    }

    // 更新同步状态
    currentState.ballX = ball.position.x;
    currentState.ballY = ball.position.y;
    currentState.ballSpeedX = ball.velocity.x;
    currentState.ballSpeedY = ball.velocity.y;
    currentState.paddle1X = paddle1.position.x;
    currentState.paddle2X = paddle2.position.x;
    if (powerUp) {
        currentState.powerUpActive = true;
        currentState.powerUpX = powerUp->position.x;
        currentState.powerUpY = powerUp->position.y;
        currentState.powerUpType = powerUp->type;
    } else {
        currentState.powerUpActive = false;
    }

    MEASURE_BLOCK_END(g_stats.updateTime);
}

/**
 * @brief 客户端逻辑更新 — Client端的每帧更新
 * @details 处理玩家输入（控制paddle2顶部绿色球拍）并发送球拍位置到Host。
 *          使用 UNSEQUENCED 标志避免旧包阻塞新包。
 */
void UpdateClientLogic() {
    MEASURE_BLOCK_START();
    float dt = GetFrameTime();
    
    if (!currentState.editingMode) {
        if (IsKeyDown(KEY_A)) paddle2.MoveLeft(5.0f);
        if (IsKeyDown(KEY_D)) paddle2.MoveRight(5.0f);
    }

    UpdateParticles(dt);

    float paddleX = paddle2.position.x;
    ENetPacket* packet = enet_packet_create(&paddleX, sizeof(float), ENET_PACKET_FLAG_UNSEQUENCED);
    enet_peer_send(peer, 1, packet);

    MEASURE_BLOCK_END(g_stats.updateTime);
}

/**
 * @brief 网络状态插值 — 消除UDP抖动
 * @param now 当前时间戳
 * @details 在 lastSnapshot 和 nextSnapshot 之间做线性插值。
 *          插值系数 t 钳制在 [0,1] 范围确保画面不跳跃。
 *          同时处理关卡切换检测和编辑模式状态同步。
 * @note 这是课程要求的关键技术：双快照线性插值算法，
 *       网络延迟 < 30ms 时画面流畅无感知
 */
void InterpolateState(double now) {
    if (nextSnapshot.timestamp <= lastSnapshot.timestamp) return;

    float t = (now - lastSnapshot.timestamp) / (nextSnapshot.timestamp - lastSnapshot.timestamp);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    ball.position.x = lastSnapshot.state.ballX * (1-t) + nextSnapshot.state.ballX * t;
    ball.position.y = lastSnapshot.state.ballY * (1-t) + nextSnapshot.state.ballY * t;
    paddle1.position.x = lastSnapshot.state.paddle1X * (1-t) + nextSnapshot.state.paddle1X * t;
    
    // 关卡切换时重新加载
    if (nextSnapshot.state.currentLevel != currentState.currentLevel && isHost == false) {
        LoadLevel(nextSnapshot.state.currentLevel);
    }
    
    // 同步编辑模式状态
    g_editingMode = nextSnapshot.state.editingMode;

    if (nextSnapshot.state.powerUpActive) {
        if (!powerUp) {
            powerUp = new PowerUp({nextSnapshot.state.powerUpX, nextSnapshot.state.powerUpY}, nextSnapshot.state.powerUpType);
        } else {
            powerUp->position.x = lastSnapshot.state.powerUpX * (1-t) + nextSnapshot.state.powerUpX * t;
            powerUp->position.y = lastSnapshot.state.powerUpY * (1-t) + nextSnapshot.state.powerUpY * t;
        }
    } else {
        if (powerUp) { delete powerUp; powerUp = nullptr; }
    }

    currentState.score = nextSnapshot.state.score;
    currentState.lives = nextSnapshot.state.lives;
    currentState.gameOver = nextSnapshot.state.gameOver;
    currentState.victory = nextSnapshot.state.victory;
    currentState.currentLevel = nextSnapshot.state.currentLevel;
}

/**
 * @brief 处理ENet网络事件 — 非阻塞轮询
 * @details Host端：接收客户端球拍位置。
 *          Client端：接收Host广播的完整GameState作为nextSnapshot。
 *          使用 enet_host_service(host, &event, 0) 非阻塞轮询。
 */
void ProcessNetworkEvents() {
    MEASURE_BLOCK_START();
    ENetEvent event;
    while (enet_host_service(host, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                cout << "Client connected!" << endl;
                peer = event.peer;
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                if (isHost) {
                    float paddleX = *(float*)event.packet->data;
                    paddle2.position.x = paddleX;
                } else {
                    lastSnapshot = nextSnapshot;
                    nextSnapshot.state = *(GameState*)event.packet->data;
                    nextSnapshot.timestamp = GetTime();
                }
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                cout << "Disconnected!" << endl;
                peer = nullptr;
                break;

            default: break;
        }
    }
    MEASURE_BLOCK_END(g_stats.networkTime);
}

int main() {
    if (enet_initialize() != 0) {
        cerr << "ENet init failed!" << endl;
        return 1;
    }
    atexit(enet_deinitialize);

    InitParticlePool();
    
    // 本周新增：启动时检测存档并询问是否继续
    SaveData saveData;
    if (SaveExists()) {
        cout << "检测到存档，是否继续上次的进度？(y/n): ";
        char choice;
        cin >> choice;
        if (choice == 'y' || choice == 'Y') {
            saveData = LoadSave();
            cout << "已加载存档：第" << saveData.currentLevel << "关，分数：" << saveData.score << endl;
        } else {
            // 删除旧存档
            if (fs::exists("save.json")) {
                fs::remove("save.json");
            }
        }
    }

    cout << "Select mode:" << endl;
    cout << "1. Host (run first)" << endl;
    cout << "2. Client (run second)" << endl;
    int choice;
    cin >> choice;
    isHost = (choice == 1);

    if (isHost) {
        ENetAddress address;
        enet_address_set_host(&address, "0.0.0.0");
        address.port = PORT;
        host = enet_host_create(&address, 1, 2, 0, 0);
        cout << "Host started, waiting for client..." << endl;
        
        // 加载存档中的关卡
        currentState.currentLevel = saveData.currentLevel;
        currentState.score = saveData.score;
        currentState.lives = saveData.lives;
        LoadLevel(currentState.currentLevel);
    } else {
        host = enet_host_create(nullptr, 1, 2, 0, 0);
        ENetAddress address;
        enet_address_set_host(&address, "127.0.0.1");
        address.port = PORT;
        peer = enet_host_connect(host, &address, 2, 0);
        cout << "Connecting to host..." << endl;
    }

    if (!host) {
        cerr << "ENet host create failed!" << endl;
        return 1;
    }

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Breakout - Co-op + Data Persistence");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        double now = GetTime();
        ProcessNetworkEvents();

        if (isHost && peer) {
            UpdateHostLogic();

            if (now - lastNetworkTime >= NETWORK_TICK_RATE) {
                ENetPacket* packet = enet_packet_create(&currentState, sizeof(GameState),
                    ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(peer, 0, packet);
                lastNetworkTime = now;
            }
        } else if (!isHost && peer) {
            UpdateClientLogic();
            InterpolateState(now);
        }

        if (IsKeyPressed(KEY_R) && isHost) {
            ResetGame();
        }

        // 绘制
        MEASURE_BLOCK_START();
        BeginDrawing();
        ClearBackground({20, 20, 30, 255});

        DrawRectangle(0, 0, 5, SCREEN_HEIGHT, GRAY);
        DrawRectangle(SCREEN_WIDTH-5, 0, 5, SCREEN_HEIGHT, GRAY);
        DrawRectangle(0, 0, SCREEN_WIDTH, 5, GRAY);
        DrawRectangle(0, SCREEN_HEIGHT-5, SCREEN_WIDTH, 5, GRAY);

        DrawText(TextFormat("Score: %d", currentState.score), 20, 15, 20, WHITE);
        DrawText(TextFormat("Lives: %d", currentState.lives), 680, 15, 20, GREEN);
        DrawText(TextFormat("Level: %d", currentState.currentLevel), 350, 15, 20, YELLOW);
        DrawText("A/D Move | R Reset | L Load | E Edit", 200, 40, 16, LIGHTGRAY);

        ball.Draw();
        paddle1.Draw();
        paddle2.Draw();
        for (const auto& brick : bricks) brick.Draw();
        if (powerUp) powerUp->Draw();
        DrawParticles();

        // 编辑模式提示
        if (g_editingMode) {
            DrawText("编辑模式：左键添加 | 右键删除 | S保存", 250, 570, 20, YELLOW);
            // 绘制鼠标位置的砖块预览
            Vector2 mouse = GetMousePosition();
            float x = floor(mouse.x / g_brickWidth) * g_brickWidth;
            float y = floor(mouse.y / g_brickHeight) * g_brickHeight;
            DrawRectangle(x, y, g_brickWidth, g_brickHeight, Color{255, 255, 255, 100});
        }

        // 加载中提示
        {
            lock_guard<mutex> lock(g_stateMutex);
            if (g_loadState == LoadState::LOADING) {
                DrawText("Loading...", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2, 40, YELLOW);
            }
        }

        if (!peer) {
            DrawText("Waiting for connection...", 300, 300, 30, YELLOW);
        } else if (currentState.gameOver) {
            DrawText("GAME OVER | Press R to restart", 220, 300, 30, RED);
        } else if (currentState.victory) {
            DrawText("YOU WIN ALL LEVELS!", 240, 300, 30, GREEN);
        }

        EndDrawing();
        MEASURE_BLOCK_END(g_stats.drawTime);

        PrintPerformanceStats();
    }

    // 退出时自动保存进度
    if (isHost && !currentState.gameOver && !currentState.victory) {
        SaveData save;
        save.currentLevel = currentState.currentLevel;
        save.score = currentState.score;
        save.lives = currentState.lives;
        SaveGame(save);
        cout << "已自动保存进度" << endl;
    }

    // 资源清理
    if (powerUp) delete powerUp;
    TextureCache::GetInstance().ClearCache();
    if (peer) enet_peer_disconnect(peer, 0);
    enet_host_destroy(host);
    CloseWindow();

    return 0;
}