#include "PowerUp.h"
#include "Game.h" // 引入Game的完整定义以调用Paddle/Ball接口

// ====================== 道具效果实现类（工厂模式） ======================

/**
 * @brief 加长板效果 — 球拍宽度临时增加
 */
class ExtendPaddleEffect : public PowerUpEffect {
private:
    float extraWidth; ///< 额外增加的宽度
    float duration;   ///< 效果持续时间（秒）
public:
    ExtendPaddleEffect(float extraWidth, float duration)
        : extraWidth(extraWidth), duration(duration) {}

    /**
     * @brief 应用效果：调用 Game::GetPaddle().Extend()
     */
    void Apply(Game& game) override {
        game.GetPaddle().Extend(extraWidth, duration);
    }
};

/**
 * @brief 多球效果 — 额外生成2个球
 */
class MultiBallEffect : public PowerUpEffect {
private:
    int extraBalls; ///< 额外生成的球数量
public:
    MultiBallEffect(int extraBalls) : extraBalls(extraBalls) {}

    /**
     * @brief 应用效果：调用 Game::SpawnExtraBalls()
     */
    void Apply(Game& game) override {
        game.SpawnExtraBalls(extraBalls);
    }
};

/**
 * @brief 减速球效果 — 所有球速度临时减半
 */
class SlowBallEffect : public PowerUpEffect {
private:
    float speedFactor; ///< 速度缩放因子
    float duration;    ///< 效果持续时间（秒）
public:
    SlowBallEffect(float speedFactor, float duration)
        : speedFactor(speedFactor), duration(duration) {}

    /**
     * @brief 应用效果：调用 Game::SlowAllBalls()
     */
    void Apply(Game& game) override {
        game.SlowAllBalls(speedFactor, duration);
    }
};

/**
 * @brief 道具效果工厂函数 — 根据类型创建对应效果对象
 * @param type 道具类型枚举
 * @return unique_ptr 管理的效果对象
 * @details PADDLE_EXTEND→加长40像素/5秒，
 *          MULTI_BALL→额外2个球，
 *          SLOW_BALL→速度×0.5/持续5秒
 */
std::unique_ptr<PowerUpEffect> CreatePowerUpEffect(PowerUpType type) {
    switch (type) {
        case PowerUpType::PADDLE_EXTEND:
            return std::make_unique<ExtendPaddleEffect>(40.0f, 5.0f);
        case PowerUpType::MULTI_BALL:
            return std::make_unique<MultiBallEffect>(2);
        case PowerUpType::SLOW_BALL:
            return std::make_unique<SlowBallEffect>(0.5f, 5.0f);
        default:
            return nullptr;
    }
}

PowerUp::PowerUp(float x, float y, PowerUpType type) {
    position = {x, y};
    this->type = type;
    active = true;
    effect = CreatePowerUpEffect(type);
}

/**
 * @brief 更新道具位置
 * @param dt 帧时间差（秒）
 * @details 道具以100像素/秒匀速向下掉落
 */
void PowerUp::Update(float dt) {
    position.y += 100 * dt;
}

/**
 * @brief 绘制道具
 * @details 30×30方块+白色边框，颜色按类型：加长板=蓝色，多球=黄色，减速球=紫色
 */
void PowerUp::Draw() const {
    Color color;
    switch (type) {
        case PowerUpType::PADDLE_EXTEND: color = BLUE; break;
        case PowerUpType::MULTI_BALL: color = YELLOW; break;
        case PowerUpType::SLOW_BALL: color = PURPLE; break;
    }
    DrawRectangle(position.x - 15, position.y - 15, 30, 30, color);
    DrawRectangleLines(position.x - 15, position.y - 15, 30, 30, WHITE);
}