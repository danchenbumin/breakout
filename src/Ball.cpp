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

void Ball::Draw() const {
    DrawCircleV(position, radius, RED);
}

/**
 * @brief 边界反弹检测
 * @param screenWidth 屏幕宽度
 * @param screenHeight 屏幕高度
 * @details 碰左右边界或上边界时反转对应速度分量。
 *          底边界故意不反弹，交由主程序判断球出界。
 * @note 左右边界各留5像素边距用于绘制墙壁
 */
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