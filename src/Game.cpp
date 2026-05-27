#include "Game.h"
#include <fstream>

// 构造函数：使用成员初始化列表
Game::Game() 
    : paddle(0, 0, 100, 20),
      score(0),
      lives(3),
      currentState(GameState::MENU),
      ballStuck(true),
      deltaTime(0.0f),
      slowEffectRemainingTime(0.0f)
{
}

// 游戏初始化
void Game::Init() {
    LoadConfig("config.json");
    ResetGame();
}

void Game::LoadConfig(const std::string& configPath) {
    // 改成从项目根目录加载config.json
    std::ifstream f("../" + configPath);
    json config = json::parse(f);
}

// 游戏状态更新
void Game::Update() {
    deltaTime = GetFrameTime(); // 获取每帧的时间差
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
                balls[0].speed = {2.0f, -2.0f}; // 发射第一个球
            }
            break;

        case GameState::PLAYING:
            // P键暂停
            if (IsKeyPressed(KEY_P)) {
                currentState = GameState::PAUSED;
                break;
            }

            // 球拍移动
            if (IsKeyDown(KEY_LEFT)) paddle.MoveLeft(5.0f);
            if (IsKeyDown(KEY_RIGHT)) paddle.MoveRight(5.0f);

            // 球拍效果更新（加长板倒计时）
            paddle.Update(deltaTime);

            // 球粘在球拍上的逻辑
            if (ballStuck) {
                balls[0].position.x = paddle.rect.x + paddle.rect.width / 2;
                balls[0].position.y = paddle.rect.y - balls[0].radius;
                if (IsKeyPressed(KEY_SPACE)) {
                    ballStuck = false;
                    balls[0].speed = {2.0f, -2.0f}; // 发射
                }
            } else {
                // 更新所有球
                for (auto& ball : balls) {
                    ball.Move();
                    ball.BounceEdge(800, 600);
                }
                CheckCollision(); // 碰撞检测
            }

            // 更新道具和粒子
            UpdatePowerUps();
            UpdateParticles();
            CheckPowerUpCollision();

            // 减速效果倒计时
            if (slowEffectRemainingTime > 0) {
                slowEffectRemainingTime -= deltaTime;
                if (slowEffectRemainingTime <= 0) {
                    // 恢复所有球的速度
                    for (auto& ball : balls) {
                        ball.speed.x *= 2.0f;
                        ball.speed.y *= 2.0f;
                    }
                }
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
    for (const auto& ball : balls) ball.Draw();
    paddle.Draw();
    for (auto& brick : bricks) brick.Draw();
    for (const auto& powerUp : powerUps) powerUp.Draw();

    // 绘制粒子（必须在BeginDrawing之后）
    for (const auto& p : particles) {
        DrawCircleV(p.pos, 3, p.color);
    }

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
    balls.clear();
    powerUps.clear();
    particles.clear();
}

// 重置游戏状态
void Game::ResetGame() {
    bricks.clear();
    float brickWidth = 85;
    float brickHeight = 25;
    // 从上到下的彩色分层：红→橙→黄→绿→蓝（经典打砖块配色）
    Color brickColors[5] = {RED, ORANGE, YELLOW, GREEN, BLUE};

    // 初始化5行8列彩色砖块
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 8; col++) {
            float x = 50 + col * 90;
            float y = 100 + row * 35;
            // 调用带颜色的Brick构造函数
            bricks.emplace_back(x, y, brickWidth, brickHeight, brickColors[row]);
        }
    }

    // 重置球和球拍
    paddle.rect = {350, 550, 100, 20};
    balls.clear();
    balls.emplace_back(Vector2{400, 540}, Vector2{0, 0}, 10);
    ballStuck = true;
    score = 0;
    lives = 3;
    currentState = GameState::MENU;

    // 清空道具和粒子
    powerUps.clear();
    particles.clear();
    slowEffectRemainingTime = 0.0f;
}

// 碰撞检测
void Game::CheckCollision() {
    // 所有球与球拍碰撞
    for (auto& ball : balls) {
        if (CheckCollisionCircleRec(ball.position, ball.radius, paddle.rect)) {
            ball.speed.y *= -1;
            ball.position.y = paddle.rect.y - ball.radius - 1; // 避免卡进球拍
        }
    }

    // 所有球与砖块碰撞
    for (auto& ball : balls) {
        for (auto& brick : bricks) {
            if (brick.active && CheckCollisionCircleRec(ball.position, ball.radius, brick.rect)) {
                brick.active = false;
                score += 10;
                ball.speed.y *= -1;
                
                // 生成砖块破碎粒子
                SpawnParticles(brick.rect.x + brick.rect.width/2, brick.rect.y + brick.rect.height/2, brick.color, 10);
                
                // 30%概率生成道具
                if (rand() % 100 < 30) {
                    SpawnPowerUp(brick.rect.x + brick.rect.width/2, brick.rect.y + brick.rect.height/2);
                }
                break; // 一次只处理一个碰撞
            }
        }
    }
}

// 检查游戏状态（胜利/失败）
void Game::CheckGameState() {
    // 检查所有球是否都掉出屏幕
    bool allBallsLost = true;
    for (auto& ball : balls) {
        if (ball.position.y + ball.radius < 600) {
            allBallsLost = false;
            break;
        }
    }

    if (allBallsLost) {
        lives--;
        if (lives <= 0) {
            currentState = GameState::GAMEOVER;
        } else {
            // 重置到只有一个球在球拍上
            balls.clear();
            balls.emplace_back(Vector2{400, 540}, Vector2{0, 0}, 10);
            ballStuck = true;
            powerUps.clear(); // 清空所有道具
            slowEffectRemainingTime = 0.0f;
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

// 粒子相关函数
void Game::SpawnParticles(float x, float y, Color color, int count) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.pos = {x, y};
        p.vel = {(float)(rand() % 100 - 50) / 10.0f, (float)(rand() % 100 - 50) / 10.0f};
        p.color = color;
        p.life = 0.5f;
        particles.push_back(p);
    }
}

void Game::UpdateParticles() {
    for (auto it = particles.begin(); it != particles.end();) {
        it->pos.x += it->vel.x * deltaTime * 60;
        it->pos.y += it->vel.y * deltaTime * 60;
        it->life -= deltaTime;
        if (it->life <= 0) {
            it = particles.erase(it);
        } else {
            ++it;
        }
    }
}

// 道具相关函数
void Game::SpawnPowerUp(float x, float y) {
    PowerUpType type = static_cast<PowerUpType>(rand() % 3);
    powerUps.emplace_back(x, y, type);
}

void Game::UpdatePowerUps() {
    for (auto it = powerUps.begin(); it != powerUps.end();) {
        it->Update(deltaTime);
        // 道具掉出屏幕则移除
        if (it->position.y > 600) {
            it = powerUps.erase(it);
        } else {
            ++it;
        }
    }
}

void Game::CheckPowerUpCollision() {
    for (auto it = powerUps.begin(); it != powerUps.end();) {
        if (CheckCollisionCircleRec(it->position, 15, paddle.rect)) {
            it->effect->Apply(*this);
            it = powerUps.erase(it);
        } else {
            ++it;
        }
    }
}

// 道具效果接口
void Game::SpawnExtraBalls(int count) {
    if (balls.empty()) return;
    Ball baseBall = balls[0];
    for (int i = 0; i < count; i++) {
        Vector2 speed = {baseBall.speed.x + (rand() % 100 - 50)/50.0f, baseBall.speed.y};
        balls.emplace_back(baseBall.position, speed, baseBall.radius);
    }
}

void Game::SlowAllBalls(float factor, float duration) {
    for (auto& ball : balls) {
        ball.speed.x *= factor;
        ball.speed.y *= factor;
    }
    slowEffectRemainingTime = duration;
}