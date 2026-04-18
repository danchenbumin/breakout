/* #ifndef BALL_H
#define BALL_H

#include "raylib.h"

class Ball {
private:
    Vector2 position;
    Vector2 speed;
    float radius;
public:
    Ball(Vector2 pos, Vector2 sp, float r);
    void Move();
    void Draw();
    void BounceEdge(int screenWidth, int screenHeight);
};

#endif */
#pragma once
#include "raylib.h"

class Ball {
public:
    Vector2 position;  // 球的位置
    Vector2 speed;     // 球的速度
    float radius;      // 球的半径

    // 构造函数
    Ball(Vector2 pos, Vector2 sp, float r);

    // 成员函数
    void Move();               // 移动球
    void Draw() const;         // 绘制球（新增const）
    void BounceEdge(int screenWidth, int screenHeight); // 边界反弹（调整：底边界不反弹，用于触发失败）
};