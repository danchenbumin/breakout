/* #ifndef PADDLE_H
#define PADDLE_H

#include "raylib.h"

class Paddle {
private:
    Rectangle rect;
public:
    Paddle(float x, float y, float w, float h);
    void Draw();
    void MoveLeft(float speed);
    void MoveRight(float speed);
};

#endif */
#pragma once
#include "raylib.h"

class Paddle {
public:
    Rectangle rect;
    float originalWidth; // 原始宽度
    float effectRemainingTime; // 效果剩余时间

    Paddle(float x, float y, float width, float height);
    void Draw() const;
    void MoveLeft(float speed);
    void MoveRight(float speed);
    void Extend(float extraWidth, float duration);
    void Update(float dt);
};