#include "Ball.h"

Ball::Ball(Vector2 pos, Vector2 sp, float r) {
    position = pos;
    speed = sp;
    radius = r;
}

void Ball::Move() {
    position.x += speed.x;
    position.y += speed.y;
}

void Ball::Draw() const { // 新增const
    DrawCircleV(position, radius, RED);
}

void Ball::BounceEdge(int screenWidth, int screenHeight) {
    // 左右边界：反弹
    if (position.x - radius <= 0 || position.x + radius >= screenWidth) {
        speed.x *= -1;
    }
    // 上边界：反弹
    if (position.y - radius <= 0) {
        speed.y *= -1;
    }
    // 底边界：移除反弹（交给主程序判断游戏失败）
    // if (position.y + radius >= screenHeight) speed.y *= -1;
}