#pragma once
#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "PowerUp.h"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * @brief 游戏状态枚举 — 状态模式管理游戏流程
 */
enum class GameState {
    MENU,     ///< 主菜单，等待按空格开始
    PLAYING,  ///< 游戏中
    PAUSED,   ///< 暂停中
    GAMEOVER, ///< 游戏结束
    VICTORY   ///< 通关胜利
};

/**
 * @brief 粒子结构体 — 用于砖块破碎/道具拾取的视觉反馈
 */
struct Particle {
    Vector2 pos;   ///< 当前位置
    Vector2 vel;   ///< 当前速度
    Color color;   ///< 粒子颜色
    float life;    ///< 剩余生命（秒）
};

/**
 * @brief 游戏管理类 — 游戏核心控制器（状态模式 + 工厂模式 + 策略模式）
 * @details 管理游戏的所有状态和数据：球、球拍、砖块、道具、粒子。
 *          通过状态机切换 MENU→PLAYING→PAUSED→GAMEOVER/VICTORY 流程。
 *          提供 GetPaddle()/SpawnExtraBalls()/SlowAllBalls() 接口供道具效果调用。
 */
class Game {
private:
    std::vector<Ball> balls;                 ///< 所有球（支持多球道具）
    Paddle paddle;                           ///< 玩家球拍
    std::vector<Brick> bricks;               ///< 所有砖块
    std::vector<PowerUp> powerUps;           ///< 活跃道具列表
    int score;                               ///< 当前分数
    int lives;                               ///< 剩余生命
    GameState currentState;                  ///< 当前游戏状态
    bool ballStuck;                          ///< 球是否粘在球拍上（发射前）
    float deltaTime;                         ///< 帧时间差
    float slowEffectRemainingTime;           ///< 减速效果剩余时间
    std::vector<Particle> particles;         ///< 粒子列表

    // ===== 私有辅助函数 =====

    /**
     * @brief 加载JSON配置文件
     * @param configPath JSON文件路径
     */
    void LoadConfig(const std::string& configPath);

    /**
     * @brief 重置游戏状态
     * @details 清空砖块/道具/粒子，重置分数、生命、球拍位置
     */
    void ResetGame();

    /**
     * @brief 碰撞检测 — 检测所有球与球拍、砖块的碰撞
     * @details 球拍碰撞：反转Y速度，球位置修正避免卡入。
     *          砖块碰撞：标记砖块非活跃、加分、生成粒子和道具。
     */
    void CheckCollision();

    /**
     * @brief 检查游戏胜负状态
     * @details 所有球掉出屏幕→扣命→GAMEOVER；所有砖块消灭→VICTORY
     */
    void CheckGameState();

    /**
     * @brief 生成道具
     * @param x 掉落起始X坐标
     * @param y 掉落起始Y坐标
     * @details 随机选择道具类型，30%概率在砖块破碎时触发
     */
    void SpawnPowerUp(float x, float y);

    /**
     * @brief 更新所有道具位置
     * @details 移出屏幕底部的道具自动删除
     */
    void UpdatePowerUps();

    /**
     * @brief 检测道具与球拍碰撞
     * @details 碰撞后调用 effect->Apply() 触发具体效果
     */
    void CheckPowerUpCollision();

    /**
     * @brief 生成粒子
     * @param x 生成位置X
     * @param y 生成位置Y
     * @param color 粒子颜色
     * @param count 粒子数量
     */
    void SpawnParticles(float x, float y, Color color, int count);

    /**
     * @brief 更新所有粒子状态
     * @details 更新粒子位置，生命归零的粒子自动移除
     */
    void UpdateParticles();

public:
    Game();

    /**
     * @brief 初始化游戏 — 加载配置并重置
     */
    void Init();

    /**
     * @brief 主更新循环 — 每帧调用一次
     * @details 处理输入、更新对象、碰撞检测、道具/粒子更新
     */
    void Update();

    /**
     * @brief 主绘制循环
     * @details 绘制边界、UI、所有游戏对象、粒子、状态提示
     */
    void Draw();

    /**
     * @brief 释放游戏资源
     * @details 清空所有容器，在主循环结束后调用
     */
    void Shutdown();

    // ===== 道具效果接口 =====

    /**
     * @brief 获取球拍引用 — 供道具效果修改球拍状态
     * @return Paddle& 球拍的可修改引用
     */
    Paddle& GetPaddle() { return paddle; }

    /**
     * @brief 生成额外球 — 多球道具效果
     * @param count 额外生成的球数量
     * @details 基于当前第一个球的位置和速度，随机扰动后生成新球
     */
    void SpawnExtraBalls(int count);

    /**
     * @brief 减速所有球 — 减速道具效果
     * @param factor 速度缩放因子（0.5=减半）
     * @param duration 效果持续时间（秒）
     * @details slowEffectRemainingTime 倒计时，到期自动恢复原速度
     */
    void SlowAllBalls(float factor, float duration);
};