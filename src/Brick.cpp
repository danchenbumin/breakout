#include "Brick.h"

Brick::Brick(float x, float y, float width, float height) {
    rect = {x, y, width, height};
    active = true;
    color = GREEN;
}

Brick::Brick(float x, float y, float width, float height, Color color) {
    rect = {x, y, width, height};
    active = true;
    this->color = color;
}

/**
 * @brief 绘制砖块
 * @details 活跃砖块绘制填充矩形+白色边框；非活跃砖块跳过不绘制。
 *          白色描边使砖块之间视觉分隔清晰，和经典打砖块效果一致。
 */
void Brick::Draw() {
    if (active) {
        DrawRectangleRec(rect, color);
        DrawRectangleLinesEx(rect, 2, WHITE);
    }
}