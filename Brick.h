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
    Rectangle rect;
    bool active;
    Color color; // 新增：砖块颜色

    Brick(float x, float y, float width, float height);
    Brick(float x, float y, float width, float height, Color color); // 带颜色的构造函数
    void Draw();
};