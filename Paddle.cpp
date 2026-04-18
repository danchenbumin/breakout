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

void Paddle::Extend(float extraWidth, float duration) {
    rect.width = originalWidth + extraWidth;
    effectRemainingTime = duration;
}

void Paddle::Update(float dt) {
    if (effectRemainingTime > 0) {
        effectRemainingTime -= dt;
        if (effectRemainingTime <= 0) {
            rect.width = originalWidth; // 效果到期，自动恢复原宽度
        }
    }
}