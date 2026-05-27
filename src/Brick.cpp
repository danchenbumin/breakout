#include "Brick.h"

Brick::Brick(float x, float y, float width, float height) {
    rect = {x, y, width, height};
    active = true;
    color = GREEN; // 默认颜色
}

Brick::Brick(float x, float y, float width, float height, Color color) {
    rect = {x, y, width, height};
    active = true;
    this->color = color;
}

void Brick::Draw() {
    if (active) {
        DrawRectangleRec(rect, color);
        DrawRectangleLinesEx(rect, 2, WHITE); // 白色描边，和同学效果一致
    }
}