#pragma once

#include "IStage.hpp"

namespace devil
{
// 1-1 전용 배치 값입니다. 다른 스테이지는 자기 StageX 파일에 별도 배치를 둡니다.
struct Stage1Layout
{
    static constexpr float floor = 360.f;
    static constexpr float spawnX = 148.f;
    static constexpr float doorX = 716.f;
    static constexpr float doorWidth = 32.f;
    static constexpr float doorHeight = 36.f;
    static constexpr float holeX = 644.f;
    static constexpr float holeWidth = 36.f;
    static constexpr float holeOpenDuration = .27f;
    static constexpr float triggerX = holeX - 30.f;
};

class Stage1 final : public IStage
{
public:
    Stage1();

    std::string_view label() const override { return "1"; }
    Vec2 spawnPoint() const override;
    void reset() override;
    void update(PlayerState& player, float dt) override;
    const std::vector<RectF>& solids() const override { return solids_; }
    bool touchesHazard(const RectF& playerBounds) const override;
    bool isAtGoal(const PlayerState& player) const override;
    Vec2 goalPoint() const override;
    void draw(sf::RenderTarget& target) const override;

    bool holeOpen() const { return holeOpen_; }
    float holeTime() const { return holeTime_; }
    float holeDepth() const;

private:
    bool holeOpen_ = false;
    float holeTime_ = 0.f;
    std::vector<RectF> solids_;
    void rebuildSolids();
};
}
