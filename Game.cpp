#include "Game.h"

// 构造函数：使用成员初始化列表，调用Ball和Paddle的带参构造
Game::Game() 
    : ball({0, 0}, {0, 0}, 10),
      paddle(0, 0, 100, 20),
      score(0),
      lives(3),
      currentState(GameState::MENU),
      ballStuck(true)
{
}

// 游戏初始化
void Game::Init() {
    ResetGame();
}

// 游戏状态更新
void Game::Update() {
    // 全局按键：R键重开
    if (IsKeyPressed(KEY_R)) {
        ResetGame();
    }

    switch (currentState) {
        case GameState::MENU:
            // 按空格开始游戏
            if (IsKeyPressed(KEY_SPACE)) {
                currentState = GameState::PLAYING;
                ballStuck = false;
                ball.speed = {2, -2}; // 发射球
            }
            break;

        case GameState::PLAYING:
            // P键暂停
            if (IsKeyPressed(KEY_P)) {
                currentState = GameState::PAUSED;
                break;
            }

            // 球拍移动
            if (IsKeyDown(KEY_LEFT)) paddle.MoveLeft(5);
            if (IsKeyDown(KEY_RIGHT)) paddle.MoveRight(5);

            // 球粘在球拍上的逻辑
            if (ballStuck) {
                ball.position.x = paddle.rect.x + paddle.rect.width / 2;
                ball.position.y = paddle.rect.y - ball.radius;
                if (IsKeyPressed(KEY_SPACE)) {
                    ballStuck = false;
                    ball.speed = {2, -2}; // 发射
                }
            } else {
                ball.Move();
                ball.BounceEdge(800, 600);
                CheckCollision(); // 碰撞检测
            }

            CheckGameState(); // 检查胜利/失败
            break;

        case GameState::PAUSED:
            // 按P继续
            if (IsKeyPressed(KEY_P)) {
                currentState = GameState::PLAYING;
            }
            break;

        case GameState::GAMEOVER:
        case GameState::VICTORY:
            // 按R重开，已在全局处理
            break;
    }
}

// 游戏画面绘制（全英文UI）
void Game::Draw() {
    BeginDrawing();
    ClearBackground({20, 20, 30, 255}); // 深灰黑背景

    // 绘制边界墙
    DrawRectangle(0, 0, 5, 600, GRAY);
    DrawRectangle(795, 0, 5, 600, GRAY);
    DrawRectangle(0, 0, 800, 5, GRAY);
    DrawRectangle(0, 595, 800, 5, GRAY);

    // 顶部UI栏（全英文）
    DrawText(TextFormat("Score: %d", score), 20, 15, 20, WHITE);
    DrawText("P: Pause  R: Restart", 300, 15, 20, LIGHTGRAY);
    DrawText(TextFormat("Lives: %d", lives), 680, 15, 20, GREEN);

    // 绘制游戏对象
    ball.Draw();
    paddle.Draw();
    for (auto& brick : bricks) brick.Draw();

    // 状态提示（全英文）
    if (currentState == GameState::MENU || ballStuck) {
        DrawText("Press SPACE to launch", 300, 100, 20, YELLOW);
        DrawText("PRESS SPACE", 350, 500, 20, YELLOW);
    } else if (currentState == GameState::PAUSED) {
        DrawText("PAUSED | Press P to continue", 280, 300, 30, WHITE);
    } else if (currentState == GameState::GAMEOVER) {
        DrawText("GAME OVER | Press R to restart", 250, 300, 30, RED);
    } else if (currentState == GameState::VICTORY) {
        DrawText("YOU WIN! | Press R to restart", 250, 300, 30, GREEN);
    }

    EndDrawing();
}

// 游戏资源释放
void Game::Shutdown() {
    bricks.clear();
}

// 重置游戏状态
void Game::ResetGame() {
    bricks.clear();
    float brickWidth = 85;
    float brickHeight = 25;

    // 初始化5行8列砖块
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 8; col++) {
            float x = 50 + col * 90;
            float y = 100 + row * 35;
            Brick brick(x, y, brickWidth, brickHeight);
            bricks.push_back(brick);
        }
    }

    // 重置球和球拍
    paddle.rect = {350, 550, 100, 20};
    ball = Ball({400, 540}, {0, 0}, 10);
    ballStuck = true;
    score = 0;
    lives = 3;
    currentState = GameState::MENU;
}

// 碰撞检测
void Game::CheckCollision() {
    // 球与球拍碰撞
    if (CheckCollisionCircleRec(ball.position, ball.radius, paddle.rect)) {
        ball.speed.y *= -1;
        ball.position.y = paddle.rect.y - ball.radius - 1; // 避免卡进球拍
    }

    // 球与砖块碰撞
    for (auto& brick : bricks) {
        if (brick.active && CheckCollisionCircleRec(ball.position, ball.radius, brick.rect)) {
            brick.active = false;
            score += 10; // 击碎砖块加10分
            ball.speed.y *= -1;
            break; // 一次只处理一个碰撞
        }
    }
}

// 检查游戏状态（胜利/失败）
void Game::CheckGameState() {
    // 球掉到底部，扣生命
    if (ball.position.y + ball.radius >= 600) {
        lives--;
        if (lives <= 0) {
            currentState = GameState::GAMEOVER;
        } else {
            // 重置球到球拍上
            ballStuck = true;
            ball.position = {paddle.rect.x + paddle.rect.width / 2, paddle.rect.y - ball.radius};
            ball.speed = {0, 0};
        }
    }

    // 检查所有砖块是否被击碎（胜利）
    bool allBricksBroken = true;
    for (const auto& brick : bricks) {
        if (brick.active) {
            allBricksBroken = false;
            break;
        }
    }
    if (allBricksBroken) {
        currentState = GameState::VICTORY;
    }
}