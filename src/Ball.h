#pragma once
#include "raylib.h"

/**
 * @brief 球类 — 表示游戏中的球对象
 * @details 管理球的位置、速度和碰撞检测。球的速度方向决定其运动轨迹，
 *          边界反弹和砖块碰撞由外部调用 BounceEdge() 处理。
 */
class Ball {
public:
    Vector2 position;  ///< 球的当前位置
    Vector2 speed;     ///< 球的当前速度
    float radius;      ///< 球的半径

    /**
     * @brief 构造函数 — 初始化球的位置、速度和半径
     * @param pos 初始位置
     * @param sp 初始速度向量
     * @param r 球的半径
     */
    Ball(Vector2 pos, Vector2 sp, float r);

    /**
     * @brief 移动球 — 根据当前速度更新位置
     * @details 每帧调用一次，将 speed 加到 position 上。
     * @note 调用前确保 speed 已正确设置
     */
    void Move();

    /**
     * @brief 绘制球 — 在当前位置绘制圆形
     * @details 使用 Raylib 的 DrawCircleV 绘制，颜色固定为 RED。
     */
    void Draw() const;

    /**
     * @brief 边界反弹检测 — 检查球是否碰到屏幕边界并反转速度
     * @param screenWidth 屏幕宽度
     * @param screenHeight 屏幕高度
     * @details 碰到左右边界和上边界时反弹，底边界不反弹，
     *          由主程序检测球出界后判定游戏失败。
     * @note 本函数只修改速度方向，不修改位置
     */
    void BounceEdge(int screenWidth, int screenHeight);
};