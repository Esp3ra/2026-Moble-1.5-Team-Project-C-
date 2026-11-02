#pragma once

#include "IStage.hpp"

namespace devil
{
// 원작 2-5의 가시 방을 위한 배치 값입니다.
// 플레이어는 오른쪽에서 시작해 왼쪽 문으로 이동합니다.
struct Stage2Layout
{
    static constexpr float floor = 360.f;
    static constexpr float spawnX = 790.f;
    static constexpr float doorX = 142.f;
    static constexpr float doorWidth = 32.f;
    static constexpr float doorHeight = 36.f;

    static constexpr float floorSpikeX = 260.f;
    // 참고 화면의 캐릭터 대비 가시 비율을 공통 플레이어 크기에 맞춥니다.
    static constexpr float floorSpikeBankWidth = 34.f;
    static constexpr float floorSpikeHeight = 11.f;
    static constexpr float floorRiseDuration = .05f;

    static constexpr float firstSpikeX = 631.f;
    // 오른쪽에서 가시 끝까지 18px 이내로 접근했을 때 이동합니다.
    static constexpr float firstMoveTriggerX = firstSpikeX + floorSpikeBankWidth + 18.f;
    static constexpr float spikeMoveDuration = .20f;
    static constexpr float firstMoveDistance = 58.f;
    // 가시에 닿기 20px 전에 드러납니다.
    static constexpr float finalTriggerX = floorSpikeX + floorSpikeBankWidth + 20.f;
    static constexpr float holeX = 454.f;
    static constexpr float holeWidth = 52.f;
    // 오른쪽 끝에 닿기 16px 전에 구멍을 엽니다.
    static constexpr float holeTriggerX = holeX + holeWidth + 16.f;
    static constexpr float holeOpenDuration = .06f;
    // 첫 구멍의 왼쪽 가장자리도 접근하면 추가로 무너집니다.
    static constexpr float edgeWidth = 24.f;
    static constexpr float edgeTriggerX = holeX + 12.f;
    // 첫 구멍이 확장된 뒤에도 캐릭터 하나가 설 수 있는 발판을 남깁니다.
    static constexpr float landingWidth = GameConfig::playerWidth + 4.f;
    static constexpr float exitHoleWidth = 40.f;
    static constexpr float exitHoleX = holeX - edgeWidth - landingWidth - exitHoleWidth;
    static constexpr float exitHoleTriggerX = exitHoleX + exitHoleWidth + 16.f;
};

class Stage2 final : public IStage
{
public:
    Stage2();

    std::string_view label() const override { return "2"; }
    Vec2 spawnPoint() const override;
    void reset() override;
    void update(PlayerState& player, float dt) override;
    const std::vector<RectF>& solids() const override { return solids_; }
    bool touchesHazard(const RectF& playerBounds) const override;
    bool isAtGoal(const PlayerState& player) const override;
    Vec2 goalPoint() const override;
    void draw(sf::RenderTarget& target) const override;

    bool floorSpikesTriggered() const { return floorSpikesTriggered_; }
    float floorSpikeHeight() const;
    float finalSpikeHeight() const;
    float firstSpikeX() const;
    float finalSpikeX() const;
    bool holeOpen() const { return holeTriggered_; }
    float holeDepth() const;

private:
    bool floorSpikesTriggered_ = false;
    bool firstMoveTriggered_ = false;
    bool finalSpikesTriggered_ = false;
    bool holeTriggered_ = false;
    bool edgeTriggered_ = false;
    bool exitHoleTriggered_ = false;
    float floorSpikeTime_ = 0.f;
    float firstMoveTime_ = 0.f;
    float finalSpikeTime_ = 0.f;
    float holeTime_ = 0.f;
    float edgeTime_ = 0.f;
    float exitHoleTime_ = 0.f;
    std::vector<RectF> solids_;
    void rebuildSolids();
};
}
