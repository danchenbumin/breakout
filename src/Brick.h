#pragma once
#include "raylib.h"

/**
 * @brief 砖块类 — 表示游戏中的砖块对象
 * @details 砖块有位置、大小、颜色和激活状态。被球击中后 active 变为 false，
 *          绘制时跳过非活跃砖块。支持带颜色的构造函数用于关卡配置。
 */
class Brick {
public:
    Rectangle rect;   ///< 砖块的矩形区域（位置+大小）
    bool active;      ///< 砖块是否仍存在（未被击碎）
    Color color;      ///< 砖块颜色，由关卡JSON或构造函数指定

    /**
     * @brief 构造函数（默认颜色）
     * @param x 砖块左上角X坐标
     * @param y 砖块左上角Y坐标
     * @param width 砖块宽度
     * @param height 砖块高度
     * @note 默认颜色为 GREEN
     */
    Brick(float x, float y, float width, float height);

    /**
     * @brief 构造函数（指定颜色）
     * @param x 砖块左上角X坐标
     * @param y 砖块左上角Y坐标
     * @param width 砖块宽度
     * @param height 砖块高度
     * @param color 砖块颜色
     * @details 用于从JSON关卡配置中加载不同颜色的砖块
     */
    Brick(float x, float y, float width, float height, Color color);

    /**
     * @brief 绘制砖块
     * @details 仅绘制活跃砖块，包含填充色和白色边框。
     *          非活跃砖块不绘制，实现"被击碎消失"效果。
     */
    void Draw();
};