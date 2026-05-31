#include "Paddle.h"

Paddle::Paddle(float x, float y, float width, float height) {
    rect = {x, y, width, height};
    originalWidth = width;
    effectRemainingTime = 0;
}

void Paddle::Draw() const {
    DrawRectangleRec(rect, BLUE);
}

void Paddle::MoveLeft(float speed) {
    rect.x -= speed;
    if (rect.x < 5) rect.x = 5;
}

void Paddle::MoveRight(float speed) {
    rect.x += speed;
    if (rect.x + rect.width > 795) rect.x = 795 - rect.width;
}

/**
 * @brief 加长球拍
 * @param extraWidth 额外宽度（像素）
 * @param duration 持续时间（秒）
 * @details 调用后 effectRemainingTime 开始倒计时，Update() 到期自动恢复。
 */
void Paddle::Extend(float extraWidth, float duration) {
    rect.width = originalWidth + extraWidth;
    effectRemainingTime = duration;
}

/**
 * @brief 更新球拍状态
 * @param dt 帧时间差（秒）
 * @details 处理道具效果倒计时：effectRemainingTime > 0 时逐帧递减，
 *          归零时自动恢复原始宽度。
 */
void Paddle::Update(float dt) {
    if (effectRemainingTime > 0) {
        effectRemainingTime -= dt;
        if (effectRemainingTime <= 0) {
            rect.width = originalWidth;
        }
    }
}