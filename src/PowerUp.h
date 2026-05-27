#pragma once
#include "raylib.h"
#include <memory>

// 前向声明：告诉编译器Game是一个类，不用完整定义
class Game;

// 道具类型枚举
enum class PowerUpType {
    PADDLE_EXTEND, // 加长板
    MULTI_BALL,    // 多球
    SLOW_BALL      // 减速球
};

// 道具效果抽象基类（工厂模式基础）
class PowerUpEffect {
public:
    virtual ~PowerUpEffect() = default;
    virtual void Apply(Game& game) = 0;
};

// 道具类
class PowerUp {
public:
    Vector2 position;
    PowerUpType type;
    bool active;
    float duration;
    std::unique_ptr<PowerUpEffect> effect;

    PowerUp(float x, float y, PowerUpType type);
    void Update(float dt);
    void Draw() const;
};