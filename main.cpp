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

// ====================== 所有类定义 ======================
class GameObject {
public:
    GameObject(Vector2 pos = {0, 0}) : position(pos) {}
    Vector2 position;
};

class PhysicalObject : virtual public GameObject {
public:
    PhysicalObject(Vector2 pos = {0, 0}, Vector2 vel = {0, 0}, float r = 0)
        : GameObject(pos), velocity(vel), radius(r) {}
    Vector2 velocity;
    float radius;

    void Move() {
        position.x += velocity.x;
        position.y += velocity.y;
    }

    void BounceEdge(int screenWidth, int screenHeight) {
        if (position.x - radius <= 5 || position.x + radius >= screenWidth - 5) {
            velocity.x *= -1;
        }
        if (position.y - radius <= 5) {
            velocity.y *= -1;
        }
    }
};

class VisualObject : virtual public GameObject {
public:
    VisualObject(Vector2 pos = {0, 0}, Color c = WHITE, bool vis = true)
        : GameObject(pos), color(c), visible(vis) {}
    Color color;
    bool visible;
    virtual void Draw() const = 0;
};

class Ball : public PhysicalObject, public VisualObject {
public:
    Ball(Vector2 pos = {400, 300}, Vector2 vel = {3, -3}, float r = 10, Color c = RED)
        : GameObject(pos), PhysicalObject(pos, vel, r), VisualObject(pos, c, true) {}

    void Draw() const override {
        if (visible) DrawCircleV(position, radius, color);
    }
};

class Paddle : public PhysicalObject, public VisualObject {
public:
    Paddle(Vector2 pos = {0, 0}, float w = 100, float h = 20, Color c = BLUE)
        : GameObject(pos), PhysicalObject(pos, {0, 0}, 0), VisualObject(pos, c, true),
          width(w), height(h), originalWidth(w) {}

    float width;
    float height;
    float originalWidth;

    void MoveLeft(float speed) {
        position.x -= speed;
        if (position.x < 5) position.x = 5;
    }

    void MoveRight(float speed) {
        position.x += speed;
        if (position.x + width > 795) position.x = 795 - width;
    }

    void ResetWidth() {
        width = originalWidth;
    }

    void Widen() {
        width = originalWidth * 1.5f;
    }

    void Draw() const override {
        if (visible) DrawRectangle(position.x, position.y, width, height, color);
    }
};

class Brick : public PhysicalObject, public VisualObject {
public:
    Brick(Vector2 pos = {0, 0}, float w = 85, float h = 25, Color c = BLUE, int t = 1)
        : GameObject(pos), PhysicalObject(pos, {0, 0}, 0), VisualObject(pos, c, true),
          width(w), height(h), active(true), type(t) {}

    float width;
    float height;
    bool active;
    int type;

    void Draw() const override {
        if (visible && active) {
            DrawRectangle(position.x, position.y, width, height, color);
            DrawRectangleLines(position.x, position.y, width, height, WHITE);
        }
    }
};

class PowerUp : public PhysicalObject, public VisualObject {
public:
    PowerUp(Vector2 pos, int type) : PhysicalObject(pos, {0, 2}, 15), 
        VisualObject(pos, WHITE, true), type(type), active(true) {
        if (type == 1) color = PURPLE;
        else if (type == 2) color = YELLOW;
        else if (type == 3) color = GREEN;
    }

    int type;
    bool active;

    void Update() {
        Move();
    }

    void Draw() const override {
        if (active && visible) {
            DrawCircleV(position, radius, color);
            DrawText(type == 1 ? "W" : type == 2 ? "S" : "+", position.x-5, position.y-5, 12, WHITE);
        }
    }
};
// ======================================================================

// ====================== 本周新增：JSON工具函数（已修复所有报错） ======================
Color GetColorFromName(const string& name) {
    static unordered_map<string, Color> colorMap = {
        {"red", RED}, {"orange", ORANGE}, {"yellow", YELLOW},
        {"green", GREEN}, {"blue", BLUE}, {"purple", PURPLE},
        {"cyan", Color{0, 255, 255, 255}}, {"white", WHITE}, {"gray", GRAY}
    };
    if (colorMap.count(name)) return colorMap[name];
    return BLUE;
}

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

void SaveJSON(const string& path, const json& data) {
    try {
        ofstream file(path);
        file << data.dump(4); // 格式化输出，缩进4空格
        TraceLog(LOG_INFO, "文件保存成功: %s", path.c_str());
    } catch (const exception& e) {
        TraceLog(LOG_ERROR, "文件保存失败: %s", e.what());
    }
}
// ======================================================================

// ====================== 本周新增：存档系统 ======================
struct SaveData {
    int version = 1;
    int currentLevel = 1;
    int score = 0;
    int lives = 3;
};

bool SaveExists() {
    return fs::exists("save.json");
}

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
const int MAX_PARTICLES = 100;

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
    float maxLife;
    bool active;
};

Particle g_particlePool[MAX_PARTICLES];

void InitParticlePool() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        g_particlePool[i].active = false;
    }
}

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

void CreateParticles(Vector2 pos, Color color, int count) {
    uniform_real_distribution<float> dist(-3, 3);
    random_device rd;
    mt19937 rng(rd());
    for (int i = 0; i < count; i++) {
        SpawnParticle(pos, {dist(rng), dist(rng)}, color, 1.0f);
    }
}
// ======================================================================

// ====================== 碰撞检测网格划分 ======================
const int GRID_WIDTH = 8;
const int GRID_HEIGHT = 6;
const int CELL_WIDTH = 100;
const int CELL_HEIGHT = 100;

vector<Brick*> g_grid[GRID_WIDTH][GRID_HEIGHT];
bool CheckBallBrickCollision(Ball& ball, vector<Brick>& bricks, int& score);

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

// ====================== 本周新增：编辑模式 ======================
bool g_editingMode = false;
int g_selectedBrickType = 1;

void ToggleEditingMode() {
    g_editingMode = !g_editingMode;
    if (g_editingMode) {
        TraceLog(LOG_INFO, "进入编辑模式：鼠标左键添加砖块，右键删除，按S保存");
    } else {
        TraceLog(LOG_INFO, "退出编辑模式");
    }
}

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

// 多线程异步加载
enum class LoadState { IDLE, LOADING, DONE };
LoadState g_loadState = LoadState::IDLE;
mutex g_stateMutex;
vector<Color> g_newBrickColors;
future<vector<Color>> g_loadFuture;

class TextureCache {
private:
    unordered_map<string, Texture2D> cache;
    mutable mutex mtx;
    TextureCache() = default;
public:
    static TextureCache& GetInstance() {
        static TextureCache instance;
        return instance;
    }
    Texture2D GetTexture(const string& path) {
        lock_guard<mutex> lock(mtx);
        auto it = cache.find(path);
        if (it != cache.end()) return it->second;
        Texture2D tex = LoadTexture(path.c_str());
        cache[path] = tex;
        return tex;
    }
    void ClearCache() {
        lock_guard<mutex> lock(mtx);
        for (auto& pair : cache) UnloadTexture(pair.second);
        cache.clear();
    }
};

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

// 网络同步结构体
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

struct Snapshot {
    GameState state;
    double timestamp;
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

// ====================== 本周新增：从JSON加载关卡 ======================
void LoadLevel(int level) {
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