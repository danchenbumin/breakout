#pragma once
#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include <vector>
#include <string>

// 游戏状态枚举
enum class GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAMEOVER,
    VICTORY
};

class Game {
private:
    Ball ball;
    Paddle paddle;
    std::vector<Brick> bricks;
    int score;
    int lives;
    GameState currentState;
    bool ballStuck; // 球是否粘在球拍上（空格发射机制）

    // 内部辅助函数
    void ResetGame();
    void CheckCollision();
    void CheckGameState();
public:
    Game();
    void Init();
    void Update();
    void Draw();
    void Shutdown();
};