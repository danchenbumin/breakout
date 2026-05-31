#pragma once
#include "raylib.h"

/**
 * @brief 球拍类 — 表示玩家的挡板
 * @details 玩家通过左右移动球拍来反弹球。支持道具加长效果和自动恢复。
 *          球拍边界锁定在屏幕内（左右各留5像素边距）。
 */
class Paddle {
public:
    Rectangle rect;                ///< 球拍矩形区域
    float originalWidth;           ///< 原始宽度，用于道具效果结束后恢复
    float effectRemainingTime;     ///< 加长效果剩余时间（秒），0表示无效果

    /**
     * @brief 构造函数 — 初始化球拍位置和大小
     * @param x 球拍左上角X坐标
     * @param y 球拍左上角Y坐标
     * @param width 球拍宽度
     * @param height 球拍高度
     */
    Paddle(float x, float y, float width, float height);

    /**
     * @brief 绘制球拍
     * @details 使用 Raylib 绘制蓝色矩形
     */
    void Draw() const;

    /**
     * @brief 向左移动球拍
     * @param speed 移动速度（像素/帧）
     * @details 移动后自动钳制到左边界（≥5像素）
     */
    void MoveLeft(float speed);

    /**
     * @brief 向右移动球拍
     * @param speed 移动速度（像素/帧）
     * @details 移动后自动钳制到右边界（≤795 - 宽度）
     */
    void MoveRight(float speed);

    /**
     * @brief 加长球拍（道具效果）
     * @param extraWidth 额外增加的宽度（像素）
     * @param duration 效果持续时间（秒）
     * @details 宽度变为 originalWidth + extraWidth，duration 秒后自动恢复
     */
    void Extend(float extraWidth, float duration);

    /**
     * @brief 更新球拍状态 — 每帧调用
     * @param dt 帧时间差（秒）
     * @details 更新道具效果倒计时，到期自动恢复原始宽度
     */
    void Update(float dt);
};