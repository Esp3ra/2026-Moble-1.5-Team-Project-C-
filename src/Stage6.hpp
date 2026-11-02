#pragma once

#include "IStage.hpp"

#include <cstddef>

namespace devil
{
// 톱날만으로 구성한 좌하단 -> 우상단 상승 코스입니다.
struct Stage6Layout
{
    static constexpr float ceiling = 64.f;
    static constexpr float spawnX = 148.f;
    static constexpr float startFloor = 456.f;
    static constexpr float lowerFloor = startFloor; // 기존 검증 코드 호환 이름
    static constexpr float startLedgeEnd = 260.f;

    static constexpr float bridgeX = 260.f;
    static constexpr float bridgeTileWidth = 36.f;
    static constexpr std::size_t bridgeTileCount = 3;

    static constexpr float chaseTriggerX = 166.f;
    static constexpr float chaseRadius = 34.f;
    static constexpr float chaseSpeed = 246.f;
    static constexpr float chaseReturnDuration = .68f;

    static constexpr std::size_t platformSawCount = 3;
    static constexpr float platformSawRadius = 32.f;
    static constexpr float firstSawX = 400.f;
    static constexpr float firstSawY = 446.f;
    static constexpr float secondSawX = 512.f;
    static constexpr float secondSawY = 398.f;
    static constexpr float thirdSawX = 584.f;
    static constexpr float thirdSawY = 350.f;
    static constexpr float sawTopWidth = 80.f;

    static constexpr float rightLedgeX = 640.f;
    static constexpr float rightLedgeWidth = 224.f;
    static constexpr float rightLedgeFloor = 268.f;
    static constexpr float doorGroundX = rightLedgeX;

    static constexpr float doorX = 812.f;
    static constexpr float doorWidth = 32.f;
    static constexpr float doorHeight = 36.f;

    static constexpr float doorSawTriggerX = 700.f;
    static constexpr float doorSawX = 758.f;
    static constexpr float doorSawRadius = 24.f;
    static constexpr float doorSawExposedHeight = 22.f;
    static constexpr float doorSawWarningDuration = .04f;
    static constexpr float doorSawRiseDuration = .12f;
    static constexpr float doorSawHoldEnd = .46f;
    static constexpr float doorSawRetractEnd = .68f;
};

class Stage6 final : public IStage
{
public:
    Stage6();

    std::string_view label() const override { return "6"; }
    Vec2 spawnPoint() const override;
    float ceilingY() const override { return Stage6Layout::ceiling; }
    void reset() override;
    void update(PlayerState& player, float dt) override;
    const std::vector<RectF>& solids() const override { return solids_; }
    bool touchesHazard(const RectF& playerBounds) const override;
    bool isAtGoal(const PlayerState& player) const override;
    Vec2 goalPoint() const override;
    void draw(sf::RenderTarget& target) const override;

    bool chaseStarted() const;
    bool chaseGone() const;
    float chaseX() const { return chaseX_; }
    float chaseY() const { return chaseY_; }
    bool bridgeTriggered() const { return escapeTriggered_; }
    float bridgeDropProgress(std::size_t index) const;
    float platformSawProgress(std::size_t index) const;
    bool firstSawReady() const { return platformSawProgress(0) >= 1.f; }
    bool secondSawReady() const { return platformSawProgress(1) >= 1.f; }
    bool thirdSawReady() const { return platformSawProgress(2) >= 1.f; }
    RectF sawTop(std::size_t index) const;
    bool doorSawTriggered() const { return doorSawTriggered_; }
    float doorSawProgress() const;
    float doorSawY() const;

private:
    enum class ChaseMode { Hidden, Chasing, Returning, Gone };

    ChaseMode chaseMode_ = ChaseMode::Hidden;
    float chaseX_ = 0.f;
    float chaseY_ = 0.f;
    float chaseReturnStartX_ = 0.f;
    float chaseReturnTime_ = 0.f;

    bool escapeTriggered_ = false;
    float escapeTime_ = 0.f;
    float platformRotation_ = 0.f;
    bool doorSawTriggered_ = false;
    float doorSawTime_ = 0.f;
    bool playerAscending_ = false;
    std::vector<RectF> solids_;

    float platformSawX(std::size_t index) const;
    float platformSawTargetY(std::size_t index) const;
    float platformSawY(std::size_t index) const;
    RectF chaseHitbox() const;
    RectF sawBody(std::size_t index) const;
    RectF doorSawBody() const;
    void rebuildSolids();
};
}
