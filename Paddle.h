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
    Rectangle rect;  // 球拍的矩形（位置+尺寸）

    // 构造函数
    Paddle(float x, float y, float w, float h);

    // 成员函数
    void Draw();               // 绘制球拍
    void MoveLeft(float speed); // 向左移动
    void MoveRight(float speed);// 向右移动
};