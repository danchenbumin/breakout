#pragma once
#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "PowerUp.h"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

enum class GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAMEOVER,
    VICTORY
};

// 粒子结构体
struct Particle {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float life;
};

class Game {
private:
    std::vector<Ball> balls; // 改为多球
    Paddle paddle;
    std::vector<Brick> bricks;
    std::vector<PowerUp> powerUps; // 道具列表
    int score;
    int lives;
    GameState currentState;
    bool ballStuck;
    float deltaTime;
    float slowEffectRemainingTime; // 减速效果剩余时间

    void LoadConfig(const std::string& configPath);
    void ResetGame();
    void CheckCollision();
    void CheckGameState();
    void SpawnPowerUp(float x, float y); // 生成道具
    void UpdatePowerUps(); // 更新道具
    void CheckPowerUpCollision(); // 检测道具与球拍碰撞

     std::vector<Particle> particles; // 粒子列表

    void SpawnParticles(float x, float y, Color color, int count); // 生成粒子
    void UpdateParticles(); // 更新粒子

public:
    Game();
    void Init();
    void Update();
    void Draw();
    void Shutdown();

    // 供道具效果调用的接口
    Paddle& GetPaddle() { return paddle; }
    void SpawnExtraBalls(int count); // 生成额外的球
    void SlowAllBalls(float factor, float duration); // 减速所有球
};