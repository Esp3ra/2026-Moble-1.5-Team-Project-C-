#pragma once

#include "IStage.hpp"

#include <cstddef>

namespace devil
{
// 위층 횡단, 벽 톱날 낙하, 아래층 역주행으로 이어지는 최종 혼합 방입니다.
struct FinalStageLayout
{
    static constexpr float ceiling = 64.f;
    static constexpr float upperFloor = 190.f;
    static constexpr float lowerCeiling = 326.f;
    static constexpr float lowerFloor = 476.f;
    static constexpr float spawnX = 132.f;

    static constexpr float shaftLeft = 704.f;
    static constexpr float shaftRight = 840.f;

    static constexpr float doorX = 116.f;
    static constexpr float doorWidth = 32.f;
    static constexpr float doorHeight = 36.f;

    static constexpr float chaserTriggerX = 244.f;
    static constexpr float chaserStartX = 72.f;
    static constexpr float chaserStopX = 638.f;
    static constexpr float chaserSpeed = 150.f;

    static constexpr float upperGapX = 350.f;
    static constexpr float upperGapWidth = 52.f;
    static constexpr float upperGapTriggerX = 302.f;
    static constexpr float upperPitDepth = 58.f;

    // Push 방처럼 구덩이의 반대편에서 잠깐 솟아, 성급한 점프를 막는 벽입니다.
    static constexpr float pushWallX = upperGapX + upperGapWidth;
    static constexpr float pushWallWidth = 18.f;
    static constexpr float pushWallHeight = 74.f;
    static constexpr float pushWallDelay = .035f;
    static constexpr float pushWallRiseDuration = .115f;
    static constexpr float pushWallHoldEnd = .50f;
    static constexpr float pushWallRetractEnd = .68f;

    static constexpr float upperSpikeX = 518.f;
    static constexpr float upperSpikeWidth = 34.f;
    static constexpr float upperSpikeHeight = 11.f;
    static constexpr float upperSpikeTriggerX = 468.f;
    static constexpr float upperSpikeWarningDuration = .045f;
    static constexpr float upperSpikeRiseDuration = .135f;

    static constexpr std::size_t wallSawCount = 3;
    static constexpr float wallSawRadius = 22.f;
    static constexpr float wallSawTravel = 34.f;
    static constexpr float wallSawMoveDuration = .16f;

    static constexpr float floorSawX = 610.f;
    static constexpr float floorSawRadius = 26.f;
    static constexpr float floorSawExposedHeight = 22.f;
    static constexpr float floorSawTriggerX = 690.f;
    static constexpr float floorSawWarningDuration = .06f;
    static constexpr float floorSawRiseDuration = .20f;

    static constexpr float lowerGapX = 420.f;
    static constexpr float lowerGapWidth = 54.f;
    static constexpr float lowerGapTriggerX = 520.f;

    static constexpr float dartWidth = 24.f;
    static constexpr float dartHeight = 8.f;
    static constexpr float dartY = lowerFloor - 18.f;
    static constexpr float dartSpeed = 620.f;
    static constexpr float dartTriggerX = 360.f;

    static constexpr float ceilingBlockTriggerX = 292.f;
    static constexpr float ceilingBlockX = 238.f;
    static constexpr float ceilingBlockWidth = 32.f;
    static constexpr float ceilingBlockHeight = 28.f;
    static constexpr float ceilingBlockWarningDuration = .06f;
    static constexpr float ceilingBlockFallDuration = .18f;
    static constexpr float ceilingBlockHoldEnd = .46f;
    static constexpr float ceilingBlockRetractEnd = .68f;
};

class FinalStage final : public IStage
{
public:
    FinalStage();

    std::string_view label() const override { return "FINAL"; }
    Vec2 spawnPoint() const override;
    float ceilingY() const override { return FinalStageLayout::ceiling; }
    void reset() override;
    void update(PlayerState& player, float dt) override;
    const std::vector<RectF>& solids() const override { return solids_; }
    bool touchesHazard(const RectF& playerBounds) const override;
    bool isAtGoal(const PlayerState& player) const override;
    Vec2 goalPoint() const override;
    void draw(sf::RenderTarget& target) const override;

    bool chaserActive() const { return chaserActive_; }
    float chaserX() const { return chaserX_; }
    bool upperGapOpen() const { return upperGapOpen_; }
    float upperGapProgress() const;
    bool pushWallTriggered() const { return pushWallTriggered_; }
    bool pushWallGone() const
    {
        return pushWallTriggered_ &&
            pushWallTime_ >= FinalStageLayout::pushWallRetractEnd;
    }
    float pushWallProgress() const;
    RectF pushWallBounds() const;
    bool upperSpikeTriggered() const { return upperSpikeTriggered_; }
    float upperSpikeProgress() const;
    bool descentStarted() const { return descentStarted_; }
    RectF wallSawBounds(std::size_t index) const;
    bool floorSawTriggered() const { return floorSawTriggered_; }
    float floorSawProgress() const;
    float floorSawY() const;
    float floorSawVisibleHeight() const;
    bool lowerGapOpen() const { return lowerGapOpen_; }
    float lowerGapProgress() const;
    bool dartActive() const;
    float dartX() const { return dartX_; }
    bool ceilingBlockTriggered() const { return ceilingBlockTriggered_; }
    bool ceilingBlockGone() const;
    float ceilingBlockProgress() const;
    RectF ceilingBlockBounds() const;

private:
    enum class DartMode { Waiting, Flying, Gone };

    bool chaserActive_ = false;
    float chaserX_ = FinalStageLayout::chaserStartX;

    bool upperGapOpen_ = false;
    float upperGapTime_ = 0.f;
    bool pushWallTriggered_ = false;
    float pushWallTime_ = 0.f;
    bool upperSpikeTriggered_ = false;
    float upperSpikeTime_ = 0.f;
    bool descentStarted_ = false;
    float descentTime_ = 0.f;

    bool floorSawTriggered_ = false;
    float floorSawTime_ = 0.f;
    bool lowerGapOpen_ = false;
    float lowerGapTime_ = 0.f;
    DartMode dartMode_ = DartMode::Waiting;
    float dartX_ = 0.f;
    bool ceilingBlockTriggered_ = false;
    float ceilingBlockTime_ = 0.f;
    float sawRotation_ = 0.f;

    std::vector<RectF> solids_;

    RectF chaserBounds() const;
    RectF upperSpikeBounds() const;
    float wallSawX(std::size_t index) const;
    RectF floorSawBounds() const;
    RectF dartBounds() const;
    void rebuildSolids();
};
}
