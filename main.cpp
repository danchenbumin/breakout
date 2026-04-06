/* #include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include <vector>

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "打砖块2D - 第二周");

    // 创建游戏对象
    Ball ball({400, 300}, {2, 2}, 10);
    Paddle paddle(350, 550, 100, 20);

    // 创建砖块（示例：一排5个）
    std::vector<Brick> bricks;
    float brickWidth = 100;
    float brickHeight = 30;
    for (int i = 0; i < 8; i++) {
        bricks.emplace_back(50 + i * 120, 100, brickWidth, brickHeight);
    }

    // 绘制左右墙为灰色矩形
    DrawRectangle(0, 0, 5, screenHeight, GRAY);   // 左墙宽5像素
    DrawRectangle(screenWidth-5, 0, 5, screenHeight, GRAY); // 右墙
    // 绘制天花板和地板
    DrawRectangle(0, 0, screenWidth, 5, GRAY);    // 顶墙高5像素
    DrawRectangle(0, screenHeight-5, screenWidth, 5, GRAY); // 底墙

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // 更新
        ball.Move();
        ball.BounceEdge(screenWidth, screenHeight);

        // 板移动
        if (IsKeyDown(KEY_LEFT)) paddle.MoveLeft(5);
        if (IsKeyDown(KEY_RIGHT)) paddle.MoveRight(5);

        // 绘制
        BeginDrawing();
        ClearBackground(RAYWHITE);

        ball.Draw();
        paddle.Draw();
        for (auto& brick : bricks) brick.Draw();

        EndDrawing();
    }

    CloseWindow();
    return 0;
} */
<<<<<<< HEAD
/* #include "raylib.h"
=======
#include "raylib.h"
>>>>>>> b944724345d279076ab6b6a71e2e5e0df864e362
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include <vector>

// 重置游戏状态（重新开始时调用）
void ResetGame(Ball& ball, Paddle& paddle, std::vector<Brick>& bricks) {
    // 重置球的位置和速度
    ball.position = {400, 300};
    ball.speed = {2, 2};
    // 重置球拍位置
    paddle.rect = {350, 550, 100, 20};
    // 重置砖块（全部激活）
    float brickWidth = 100;
    float brickHeight = 30;
    bricks.clear();
    for (int i = 0; i < 8; i++) {
        bricks.emplace_back(50 + i * 120, 100, brickWidth, brickHeight);
    }
}

int main() {
    // 1. 窗口初始化
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "打砖块2D - 完整版");

    // 2. 游戏对象初始化
    Ball ball({400, 300}, {2, 2}, 10);
    Paddle paddle(350, 550, 100, 20);
    std::vector<Brick> bricks;
    ResetGame(ball, paddle, bricks); // 初始化砖块

    // 游戏状态枚举
    enum GameState {
        PLAYING,   // 游戏中
        WIN,       // 胜利
        GAME_OVER  // 失败
    };
    GameState gameState = PLAYING;

    SetTargetFPS(60); // 设置帧率60

    // 3. 游戏主循环
    while (!WindowShouldClose()) {
        // ========== 第一部分：输入处理 + 状态重置 ==========
        if (IsKeyPressed(KEY_R)) { // 按R重新开始
            ResetGame(ball, paddle, bricks);
            gameState = PLAYING;
        }

        if (gameState == PLAYING) { // 只有游戏中才更新逻辑
            // ========== 第二部分：更新游戏状态 ==========
            // 球移动
            ball.Move();
            // 边界反弹（左右+上）
            ball.BounceEdge(screenWidth, screenHeight);

            // 球拍移动
            if (IsKeyDown(KEY_LEFT)) paddle.MoveLeft(5);
            if (IsKeyDown(KEY_RIGHT)) paddle.MoveRight(5);

            // ========== 碰撞检测：球 + 球拍 ==========
            if (CheckCollisionCircleRec(ball.position, ball.radius, paddle.rect)) {
                // 反转垂直速度（反弹）
                ball.speed.y *= -1;
                // 微调球的位置，避免卡进球拍
                ball.position.y = paddle.rect.y - ball.radius - 1;
            }

            // ========== 碰撞检测：球 + 砖块 ==========
            for (auto& brick : bricks) {
                if (brick.active && CheckCollisionCircleRec(ball.position, ball.radius, brick.rect)) {
                    // 标记砖块为非激活（击碎）
                    brick.active = false;
                    
                    // 检测碰撞方向，优化反弹逻辑（避免球卡进砖块）
                    // 球的中心坐标
                    float ballX = ball.position.x;
                    float ballY = ball.position.y;
                    // 砖块的边界
                    float brickLeft = brick.rect.x;
                    float brickRight = brick.rect.x + brick.rect.width;
                    float brickTop = brick.rect.y;
                    float brickBottom = brick.rect.y + brick.rect.height;

                    // 水平方向碰撞（左右）
                    if (ballX < brickLeft + 5 || ballX > brickRight - 5) {
                        ball.speed.x *= -1;
                    }
                    // 垂直方向碰撞（上下）
                    if (ballY < brickTop + 5 || ballY > brickBottom - 5) {
                        ball.speed.y *= -1;
                    }

                    break; // 一次只碰撞一个砖块
                }
            }

            // ========== 游戏结束判断 ==========
            // 1. 球掉到底部 → 失败
            if (ball.position.y + ball.radius >= screenHeight) {
                gameState = GAME_OVER;
            }
            // 2. 所有砖块被击碎 → 胜利
            bool allBricksBroken = true;
            for (const auto& brick : bricks) {
                if (brick.active) {
                    allBricksBroken = false;
                    break;
                }
            }
            if (allBricksBroken) {
                gameState = WIN;
            }
        }

        // ========== 第三部分：绘制游戏画面 ==========
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // 1. 绘制边界墙（移到循环内，每帧绘制）
        DrawRectangle(0, 0, 5, screenHeight, GRAY);        // 左墙
        DrawRectangle(screenWidth-5, 0, 5, screenHeight, GRAY); // 右墙
        DrawRectangle(0, 0, screenWidth, 5, GRAY);        // 顶墙
        DrawRectangle(0, screenHeight-5, screenWidth, 5, GRAY); // 底墙

        // 2. 绘制游戏对象
        ball.Draw();
        paddle.Draw();
        for (auto& brick : bricks) brick.Draw();

        // 3. 绘制游戏状态提示
        if (gameState == GAME_OVER) {
            DrawText("游戏失败！按R重新开始", screenWidth/2 - 150, screenHeight/2, 30, RED);
        } else if (gameState == WIN) {
            DrawText("游戏胜利！按R重新开始", screenWidth/2 - 150, screenHeight/2, 30, GREEN);
        }

        EndDrawing();
    }

    // 4. 游戏收尾
    CloseWindow();
    return 0;
<<<<<<< HEAD
} */
// main.cpp 精简后
#include "raylib.h"
#include "Game.h"

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    // 初始化窗口
    InitWindow(screenWidth, screenHeight, "Breakout - Enhanced Edition");
    SetTargetFPS(60);

    // 创建游戏对象，全程只需要这一个对象
    Game game;
    game.Init();

    // 游戏主循环，极简
    while (!WindowShouldClose()) {
        game.Update();
        game.Draw();
    }

    // 收尾释放
    game.Shutdown();
    CloseWindow();
    return 0;
=======
>>>>>>> b944724345d279076ab6b6a71e2e5e0df864e362
}