/* #ifndef BRICK_H
#define BRICK_H

#include "raylib.h"

class Brick {
private:
    Rectangle rect;
    bool active;
public:
    Brick(float x, float y, float w, float h);
    void Draw();
    bool IsActive() { return active; }
    void SetActive(bool a) { active = a; }
};

#endif */
#pragma once
#include "raylib.h"

class Brick {
public:
    Rectangle rect;  // 砖块的矩形（位置+尺寸）
    bool active;     // 砖块是否激活（显示/可碰撞）

    // 构造函数
    Brick(float x, float y, float w, float h);

    // 成员函数
    void Draw(); // 绘制砖块
};