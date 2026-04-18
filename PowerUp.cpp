#include "PowerUp.h"
#include "Game.h" // 新增：引入Game的完整定义

// 加长板效果
class ExtendPaddleEffect : public PowerUpEffect {
private:
    float extraWidth;
    float duration;
public:
    ExtendPaddleEffect(float extraWidth, float duration) 
        : extraWidth(extraWidth), duration(duration) {}

    void Apply(Game& game) override {
        game.GetPaddle().Extend(extraWidth, duration);
    }
};

// 多球效果
class MultiBallEffect : public PowerUpEffect {
private:
    int extraBalls;
public:
    MultiBallEffect(int extraBalls) : extraBalls(extraBalls) {}

    void Apply(Game& game) override {
        game.SpawnExtraBalls(extraBalls);
    }
};

// 减速球效果
class SlowBallEffect : public PowerUpEffect {
private:
    float speedFactor;
    float duration;
public:
    SlowBallEffect(float speedFactor, float duration) 
        : speedFactor(speedFactor), duration(duration) {}

    void Apply(Game& game) override {
        game.SlowAllBalls(speedFactor, duration);
    }
};

// 道具工厂函数
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

void PowerUp::Update(float dt) {
    position.y += 100 * dt; // 道具下落速度
}

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