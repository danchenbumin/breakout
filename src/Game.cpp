#include "Game.h"
#include <fstream>

/**
 * @brief 构造函数 — 使用成员初始化列表设置默认值
 * @details paddle 初始化为(0,0)位置，100×20大小；
 *          初始状态为 MENU，球粘在球拍上等待发射。
 */
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

/**
 * @brief 初始化游戏 — 加载配置文件并重置到初始状态
 * @details 先调用 LoadConfig() 加载关卡配置，再调用 ResetGame() 初始化所有对象
 */
void Game::Init() {
    LoadConfig("config.json");
    ResetGame();
}

/**
 * @brief 加载JSON配置文件
 * @param configPath JSON配置文件路径（相对于上一级目录）
 * @details 从项目根目录读取 config.json 并解析
 */
void Game::LoadConfig(const std::string& configPath) {
    std::ifstream f("../" + configPath);
    json config = json::parse(f);
}

/**
 * @brief 主游戏更新循环 — 每帧调用一次
 * @details 状态机驱动：MENU等待发射 → PLAYING处理输入/碰撞/道具 →
 *          PAUSED暂停 → GAMEOVER/VICTORY等待重开。
 *          全局快捷键：R键重置、P键暂停/继续。
 */
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

/**
 * @brief 游戏画面绘制 — 每帧调用一次
 * @details 绘制顺序：背景→边界墙→顶部UI栏→游戏对象→粒子→状态提示。
 *          所有UI文字使用英文，符合课程要求。
 */
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

/**
 * @brief 释放游戏资源 — 清空所有动态容器
 * @details 在程序退出前调用，清理砖块、球、道具、粒子列表
 */
void Game::Shutdown() {
    bricks.clear();
    balls.clear();
    powerUps.clear();
    particles.clear();
}

/**
 * @brief 重置游戏状态 — 清空所有对象并恢复初始配置
 * @details 生成5行8列彩色砖块（红→橙→黄→绿→蓝），
 *          球回到球拍上等待发射，分数/生命/道具/粒子全部重置。
 */
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

/**
 * @brief 碰撞检测 — 检测球与球拍、球与砖块的碰撞
 * @details 球拍碰撞：反转Y速度，球位置修正避免卡入球拍。
 *          砖块碰撞：使用圆与矩形碰撞算法(CheckCollisionCircleRec)，
 *          击中后标记砖块非活跃、加分10、生成粒子、30%概率掉落道具。
 *          每次只处理一个碰撞（break跳出内层循环）。
 */
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

/**
 * @brief 检查游戏胜负状态
 * @details 所有球掉出屏幕→扣一条命→球重置到球拍上。
 *          命归零→GAMEOVER。所有砖块消灭→VICTORY。
 */
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

/**
 * @brief 生成粒子 — 砖块破碎或道具拾取时的视觉反馈
 * @param x 生成位置X
 * @param y 生成位置Y
 * @param color 粒子颜色（通常取砖块颜色）
 * @param count 粒子数量
 * @details 每个粒子随机速度发射，生命0.5秒后自动淡出删除
 */
    for (int i = 0; i < count; i++) {
        Particle p;
        p.pos = {x, y};
        p.vel = {(float)(rand() % 100 - 50) / 10.0f, (float)(rand() % 100 - 50) / 10.0f};
        p.color = color;
        p.life = 0.5f;
        particles.push_back(p);
    }
}

/**
 * @brief 更新所有粒子状态
 * @details 逐帧更新粒子位置，生命归零的粒子从列表中移除。
 *          速度乘以deltaTime*60保持帧率无关的运动。
 */
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

/**
 * @brief 生成道具 — 随机选择道具类型
 * @param x 掉落起始X坐标
 * @param y 掉落起始Y坐标
 * @details 从3种道具类型中随机选择，加入powerUps列表
 */
    PowerUpType type = static_cast<PowerUpType>(rand() % 3);
    powerUps.emplace_back(x, y, type);
}

/**
 * @brief 更新所有道具位置
 * @details 道具向下掉落，移出屏幕底部（y>600）自动删除
 */
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

/**
 * @brief 检测道具与球拍碰撞 — 触发道具效果
 * @details 使用 CheckCollisionCircleRec 检测，
 *          碰撞后调用 effect->Apply() 触发效果并从列表中移除道具
 */
    for (auto it = powerUps.begin(); it != powerUps.end();) {
        if (CheckCollisionCircleRec(it->position, 15, paddle.rect)) {
            it->effect->Apply(*this);
            it = powerUps.erase(it);
        } else {
            ++it;
        }
    }
}

/**
 * @brief 生成额外球 — 多球道具效果实现
 * @param count 额外生成的球数量
 * @details 基于当前第一个球的位置，速度随机扰动后生成新球加入balls列表
 */
    if (balls.empty()) return;
    Ball baseBall = balls[0];
    for (int i = 0; i < count; i++) {
        Vector2 speed = {baseBall.speed.x + (rand() % 100 - 50)/50.0f, baseBall.speed.y};
        balls.emplace_back(baseBall.position, speed, baseBall.radius);
    }
}

/**
 * @brief 减速所有球 — 减速道具效果实现
 * @param factor 速度缩放因子（0.5=减半）
 * @param duration 效果持续时间（秒）
 * @details slowEffectRemainingTime 开始倒计时，
 *          到期后 Update() 中自动将球速度恢复为原速度÷factor
 */
    for (auto& ball : balls) {
        ball.speed.x *= factor;
        ball.speed.y *= factor;
    }
    slowEffectRemainingTime = duration;
}