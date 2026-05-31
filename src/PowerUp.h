#pragma once
#include "raylib.h"
#include <memory>

// 前向声明：告诉编译器Game是一个类，不用完整定义
class Game;

/**
 * @brief 道具类型枚举
 * @details PADDLE_EXTEND=加长板,  MULTI_BALL=多球,  SLOW_BALL=减速球
 */
enum class PowerUpType {
    PADDLE_EXTEND, ///< 加长板：球拍宽度+50%
    MULTI_BALL,    ///< 多球：额外生成2个球
    SLOW_BALL      ///< 减速球：所有球速度降至50%
};

/**
 * @brief 道具效果抽象基类 — 工厂模式的产品接口
 * @details 每种道具效果实现各自的 Apply() 方法，
 *          通过多态统一调用。使用 unique_ptr 管理生命周期。
 */
class PowerUpEffect {
public:
    virtual ~PowerUpEffect() = default;

    /**
     * @brief 应用道具效果
     * @param game 游戏实例引用，供效果函数修改游戏状态
     * @details 纯虚函数，子类必须实现具体效果逻辑
     */
    virtual void Apply(Game& game) = 0;
};

/**
 * @brief 道具类 — 表示游戏中掉落/可拾取的道具
 * @details 道具从砖块位置掉落，碰到球拍后触发效果。
 *          采用策略模式，effect 成员决定具体行为。
 */
class PowerUp {
public:
    Vector2 position;                     ///< 道具当前位置
    PowerUpType type;                     ///< 道具类型
    bool active;                          ///< 道具是否仍在游戏中
    float duration;                       ///< 效果持续时间（秒）
    std::unique_ptr<PowerUpEffect> effect; ///< 道具效果（策略模式）

    /**
     * @brief 构造函数
     * @param x 道具初始X坐标
     * @param y 道具初始Y坐标
     * @param type 道具类型
     * @details 自动通过工厂函数 CreatePowerUpEffect 创建对应的效果对象
     */
    PowerUp(float x, float y, PowerUpType type);

    /**
     * @brief 更新道具位置
     * @param dt 帧时间差（秒）
     * @details 道具以100像素/秒的速度向下掉落
     */
    void Update(float dt);

    /**
     * @brief 绘制道具
     * @details 根据类型绘制不同颜色方块+白色边框：
     *          PADDLE_EXTEND=蓝色, MULTI_BALL=黄色, SLOW_BALL=紫色
     */
    void Draw() const;
};