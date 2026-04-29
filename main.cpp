#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>
#include <unordered_map>
#include <enet/enet.h>
#include "raylib.h"
using namespace std;

// ====================== 本周新增：多线程异步加载相关定义 ======================
// 加载状态枚举，完全匹配PPT定义
enum class LoadState { IDLE, LOADING, DONE };
// 全局加载状态与互斥锁（保护共享数据，避免数据竞争）
LoadState g_loadState = LoadState::IDLE;
mutex g_stateMutex;
// 加载完成后的砖块新颜色，共享数据，加锁保护
vector<Color> g_newBrickColors;
future<vector<Color>> g_loadFuture;

// 【加分项】PPT要求的线程安全纹理缓存单例
class TextureCache {
private:
    unordered_map<string, Texture2D> cache;
    mutable mutex mtx;
    TextureCache() = default; // 私有构造，单例模式
public:
    // 线程安全的单例获取（C++11后静态局部变量初始化天然线程安全）
    static TextureCache& GetInstance() {
        static TextureCache instance;
        return instance;
    }
    // 线程安全的纹理获取，重复路径只加载一次
    Texture2D GetTexture(const string& path) {
        lock_guard<mutex> lock(mtx);
        auto it = cache.find(path);
        if (it != cache.end()) return it->second;
        Texture2D tex = LoadTexture(path.c_str());
        cache[path] = tex;
        return tex;
    }
    // 线程安全的缓存清理
    void ClearCache() {
        lock_guard<mutex> lock(mtx);
        for (auto& pair : cache) UnloadTexture(pair.second);
        cache.clear();
    }
};

// 异步加载函数：工作线程执行，模拟大纹理加载耗时
vector<Color> AsyncLoadLargeResource() {
    // 模拟加载大型纹理/关卡资源的耗时操作（500ms，符合PPT示例）
    this_thread::sleep_for(chrono::milliseconds(500));
    
    // 加载完成后生成新的砖块配色，作为加载结果返回
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
// ================================================================================

// 强制1字节对齐，解决跨平台结构体数据错位问题
#pragma pack(1)
struct GameState {
    float ballX, ballY;
    float ballSpeedX, ballSpeedY;
    float paddle1X; // 主机的下板
    float paddle2X; // 客户端的上板
    int score;
    int lives;
    bool gameOver;
    bool victory;
    // 道具同步状态
    bool powerUpActive;
    float powerUpX, powerUpY;
    int powerUpType; // 1=加宽 2=加速 3=生命
};
#pragma pack()

struct Snapshot {
    GameState state;
    double timestamp;
};

// 粒子结构体
struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
    float maxLife;
};

// 基础游戏对象类
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
    Brick(Vector2 pos = {0, 0}, float w = 85, float h = 25, Color c = BLUE)
        : GameObject(pos), PhysicalObject(pos, {0, 0}, 0), VisualObject(pos, c, true),
          width(w), height(h), active(true), hits(1) {}

    float width;
    float height;
    bool active;
    int hits;

    void Draw() const override {
        if (visible && active) {
            DrawRectangle(position.x, position.y, width, height, color);
            DrawRectangleLines(position.x, position.y, width, height, WHITE);
        }
    }
};

// 道具类
class PowerUp : public PhysicalObject, public VisualObject {
public:
    PowerUp(Vector2 pos, int type) : PhysicalObject(pos, {0, 2}, 15), 
        VisualObject(pos, WHITE, true), type(type), active(true) {
        if (type == 1) color = PURPLE; // 加宽
        else if (type == 2) color = YELLOW; // 加速
        else if (type == 3) color = GREEN; // 生命
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

// 全局变量
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PORT = 12345;
const float NETWORK_TICK_RATE = 1.0f / 30.0f;

bool isHost = false;
ENetHost* host = nullptr;
ENetPeer* peer = nullptr;

GameState currentState;
Snapshot lastSnapshot, nextSnapshot;
double lastNetworkTime = 0.0f;

vector<Brick> bricks;
vector<Particle> particles;
Ball ball({400, 300}, {3, -3}, 10, RED);
Paddle paddle1({350, 550}, 100, 20, BLUE);
Paddle paddle2({350, 30}, 100, 20, GREEN);
PowerUp* powerUp = nullptr;

mt19937 rng(random_device{}());

// 创建粒子特效
void CreateParticles(Vector2 pos, Color color, int count) {
    uniform_real_distribution<float> dist(-3, 3);
    for (int i = 0; i < count; i++) {
        Particle p;
        p.position = pos;
        p.velocity = {dist(rng), dist(rng)};
        p.color = color;
        p.life = 1.0f;
        p.maxLife = 1.0f;
        particles.push_back(p);
    }
}

// 更新粒子
void UpdateParticles(float dt) {
    for (auto it = particles.begin(); it != particles.end();) {
        it->position.x += it->velocity.x;
        it->position.y += it->velocity.y;
        it->life -= dt;
        if (it->life <= 0) {
            it = particles.erase(it);
        } else {
            it++;
        }
    }
}

// 绘制粒子
void DrawParticles() {
    for (const auto& p : particles) {
        float alpha = p.life / p.maxLife;
        DrawCircleV(p.position, 3, Color{p.color.r, p.color.g, p.color.b, (unsigned char)(alpha * 255)});
    }
}

// 初始化砖块，支持自定义配色
void InitBricks(const vector<Color>& colors = {RED, ORANGE, YELLOW, GREEN, BLUE}) {
    bricks.clear();
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 8; col++) {
            bricks.emplace_back(
                Vector2{50.0f + col * 90.0f, 100.0f + row * 35.0f},
                85, 25,
                row < colors.size() ? colors[row] : BLUE
            );
        }
    }
}

void ResetGame() {
    InitBricks();
    ball.position = {400, 300};
    ball.velocity = {3, -3};
    paddle1.position = {350, 550};
    paddle1.ResetWidth();
    paddle2.position = {350, 30};
    paddle2.ResetWidth();
    if (powerUp) { delete powerUp; powerUp = nullptr; }
    particles.clear();
    currentState.score = 0;
    currentState.lives = 3;
    currentState.gameOver = false;
    currentState.victory = false;
    currentState.powerUpActive = false;
    // 重置加载状态
    lock_guard<mutex> lock(g_stateMutex);
    g_loadState = LoadState::IDLE;
    g_newBrickColors.clear();
}

void UpdateHostLogic() {
    float dt = GetFrameTime();
    if (IsKeyDown(KEY_A)) paddle1.MoveLeft(5.0f);
    if (IsKeyDown(KEY_D)) paddle1.MoveRight(5.0f);

    // ====================== 本周新增：异步加载触发逻辑 ======================
    // 按下L键，且当前无加载任务时，启动异步加载
    if (IsKeyPressed(KEY_L)) {
        lock_guard<mutex> lock(g_stateMutex);
        if (g_loadState == LoadState::IDLE) {
            g_loadState = LoadState::LOADING;
            // 启动异步任务，强制在新线程执行，符合PPT要求
            g_loadFuture = async(launch::async, AsyncLoadLargeResource);
        }
    }

    // 检查异步加载是否完成
    {
        lock_guard<mutex> lock(g_stateMutex);
        if (g_loadState == LoadState::LOADING) {
            // 非阻塞检查future状态，主线程不卡顿
            if (g_loadFuture.wait_for(chrono::seconds(0)) == future_status::ready) {
                g_newBrickColors = g_loadFuture.get();
                g_loadState = LoadState::DONE;
                // 加载完成，更换砖块颜色
                InitBricks(g_newBrickColors);
            }
        }
    }
    // ========================================================================

    ball.Move();
    ball.BounceEdge(SCREEN_WIDTH, SCREEN_HEIGHT);

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

    for (auto& brick : bricks) {
        if (brick.active && CheckCollisionCircleRec(ball.position, ball.radius,
            {brick.position.x, brick.position.y, brick.width, brick.height})) {
            brick.active = false;
            currentState.score += 10;
            ball.velocity.y *= -1;
            CreateParticles(brick.position, brick.color, 10);
            
            // 随机掉落道具
            uniform_int_distribution<int> dist(1, 5);
            if (dist(rng) == 1 && !powerUp) {
                uniform_int_distribution<int> typeDist(1, 3);
                powerUp = new PowerUp(brick.position, typeDist(rng));
            }
            break;
        }
    }

    // 更新道具
    if (powerUp && powerUp->active) {
        powerUp->Update();
        // 检测道具碰撞
        if (CheckCollisionCircleRec(powerUp->position, powerUp->radius,
            {paddle1.position.x, paddle1.position.y, paddle1.width, paddle1.height})) {
            // 主机吃到道具
            if (powerUp->type == 1) paddle1.Widen();
            else if (powerUp->type == 2) ball.velocity.x *= 1.2f, ball.velocity.y *= 1.2f;
            else if (powerUp->type == 3) currentState.lives++;
            powerUp->active = false;
            CreateParticles(powerUp->position, powerUp->color, 15);
            powerUp = nullptr;
        } else if (CheckCollisionCircleRec(powerUp->position, powerUp->radius,
            {paddle2.position.x, paddle2.position.y, paddle2.width, paddle2.height})) {
            // 客户端吃到道具
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

    // 更新粒子
    UpdateParticles(dt);

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

    bool allBricksBroken = true;
    for (const auto& brick : bricks) {
        if (brick.active) {
            allBricksBroken = false;
            break;
        }
    }
    if (allBricksBroken) currentState.victory = true;

    // 更新同步状态，把道具的状态也同步
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
}

void UpdateClientLogic() {
    float dt = GetFrameTime();
    if (IsKeyDown(KEY_A)) paddle2.MoveLeft(5.0f);
    if (IsKeyDown(KEY_D)) paddle2.MoveRight(5.0f);

    UpdateParticles(dt);

    float paddleX = paddle2.position.x;
    ENetPacket* packet = enet_packet_create(&paddleX, sizeof(float), ENET_PACKET_FLAG_UNSEQUENCED);
    enet_peer_send(peer, 1, packet);
}

void InterpolateState(double now) {
    if (nextSnapshot.timestamp <= lastSnapshot.timestamp) return;

    float t = (now - lastSnapshot.timestamp) / (nextSnapshot.timestamp - lastSnapshot.timestamp);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    ball.position.x = lastSnapshot.state.ballX * (1-t) + nextSnapshot.state.ballX * t;
    ball.position.y = lastSnapshot.state.ballY * (1-t) + nextSnapshot.state.ballY * t;
    paddle1.position.x = lastSnapshot.state.paddle1X * (1-t) + nextSnapshot.state.paddle1X * t;
    
    // 客户端自己的板子，跳过插值，用本地实时的，避免抖动
    // paddle2.position.x = lastSnapshot.state.paddle2X * (1-t) + nextSnapshot.state.paddle2X * t;

    // 同步道具状态
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
}

void ProcessNetworkEvents() {
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
}

int main() {
    if (enet_initialize() != 0) {
        cerr << "ENet init failed!" << endl;
        return 1;
    }
    atexit(enet_deinitialize);

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
        ResetGame();
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

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Breakout - Co-op + MultiThread");
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
        BeginDrawing();
        ClearBackground({20, 20, 30, 255});

        DrawRectangle(0, 0, 5, SCREEN_HEIGHT, GRAY);
        DrawRectangle(SCREEN_WIDTH-5, 0, 5, SCREEN_HEIGHT, GRAY);
        DrawRectangle(0, 0, SCREEN_WIDTH, 5, GRAY);
        DrawRectangle(0, SCREEN_HEIGHT-5, SCREEN_WIDTH, 5, GRAY);

        DrawText(TextFormat("Score: %d", currentState.score), 20, 15, 20, WHITE);
        DrawText(TextFormat("Lives: %d", currentState.lives), 680, 15, 20, GREEN);
        DrawText("A/D Move | R Reset | L Load Resource", 250, 15, 20, LIGHTGRAY);

        ball.Draw();
        paddle1.Draw();
        paddle2.Draw();
        for (const auto& brick : bricks) brick.Draw();
        if (powerUp) powerUp->Draw();
        DrawParticles();

        // ====================== 本周新增：加载中提示 ======================
        {
            lock_guard<mutex> lock(g_stateMutex);
            if (g_loadState == LoadState::LOADING) {
                // 加载期间屏幕中央显示Loading...，游戏完全不卡顿
                DrawText("Loading...", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2, 40, YELLOW);
            }
        }
        // ==================================================================

        if (!peer) {
            DrawText("Waiting for connection...", 300, 300, 30, YELLOW);
        } else if (currentState.gameOver) {
            DrawText("GAME OVER | Press R to restart", 220, 300, 30, RED);
        } else if (currentState.victory) {
            DrawText("YOU WIN! | Press R to restart", 240, 300, 30, GREEN);
        }

        EndDrawing();
    }

    // 资源清理
    if (powerUp) delete powerUp;
    TextureCache::GetInstance().ClearCache(); // 清理纹理缓存
    if (peer) enet_peer_disconnect(peer, 0);
    enet_host_destroy(host);
    CloseWindow();

    return 0;
}