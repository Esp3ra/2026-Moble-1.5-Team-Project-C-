#pragma once

#include "IStage.hpp"
#include "Stage3Objects.hpp"

#include <memory>
#include <vector>

namespace devil
{
// Every Stage 3 world-space placement lives here; object classes only receive values.
struct Stage3Layout
{
    static constexpr float floorY = 360.f;
    static constexpr float terrainOverscan = 40.f;
    static constexpr float worldBottomY = GameConfig::height + terrainOverscan;
    static constexpr float floorDepth = worldBottomY - floorY;
    static constexpr float spawnX = 130.f;

    static constexpr float coinSize = 15.f;
    static constexpr float coin1X = 300.f;
    static constexpr float coin1Y = 325.f;
    static constexpr float coin2X = 520.f;
    static constexpr float coin2Y = 275.f;
    static constexpr float coin3X = 690.f;
    static constexpr float coin3Y = 325.f;

    static constexpr float holeX = 340.f;
    static constexpr float holeWidth = 70.f;

    static constexpr float platform1X = 480.f;
    static constexpr float platform1Y = 310.f;
    static constexpr float platform1Width = 80.f;
    static constexpr float platform1Height = 20.f;

    static constexpr float proximityX = 600.f;
    static constexpr float proximityWidth = 34.f;
    static constexpr float proximityHeight = 11.f;
    static constexpr float proximityTriggerX = 570.f;
    static constexpr float proximityRiseSpeed = 250.f;

    static constexpr float wallX = GameConfig::left;
    static constexpr float wallHeight = 150.f;
    static constexpr float wallY = floorY - wallHeight;
    static constexpr float wallWidth = 26.f;
    static constexpr float wallSpeed = 75.f;
    static constexpr float wallStopX = 694.f;

    static constexpr float originalDoorX = 740.f;
    static constexpr float originalDoorFloorY = floorY;
    static constexpr float relocatedDoorX = 490.f;
    static constexpr float relocatedDoorFloorY = platform1Y;
    static constexpr float doorWidth = 32.f;
    static constexpr float doorHeight = 36.f;

    static constexpr float ceilingX = 720.f;
    static constexpr float ceilingStartY = GameConfig::ceiling;
    static constexpr float ceilingWidth = 120.f;
    static constexpr float ceilingHeight = 20.f;
    // This is the specified Y=360 bottom anchor; the top becomes a 20px step.
    static constexpr float ceilingLandingY = floorY;
    static constexpr float ceilingFallSpeed = 210.f;
};

class Stage3 final : public IStage, private ITriggerListener
{
public:
    Stage3();

    std::string_view label() const override { return "3"; }
    Vec2 spawnPoint() const override;
    void reset() override;
    void update(PlayerState& player, float dt) override;
    const std::vector<RectF>& solids() const override { return solids_; }
    bool touchesHazard(const RectF& playerBounds) const override;
    bool isAtGoal(const PlayerState& player) const override;
    Vec2 goalPoint() const override;
    void draw(sf::RenderTarget& target) const override;

private:
    std::vector<RectF> staticSolids_;
    std::vector<RectF> solids_;
    std::vector<std::unique_ptr<Coin>> coins_;
    std::vector<std::unique_ptr<DisappearingFloor>> dynamicFloors_;
    std::vector<std::unique_ptr<StageHazard>> hazards_;
    std::vector<std::unique_ptr<CrushingCeiling>> ceilings_;
    Vec2 m_currentDoorPos_{};
    bool doorRelocated_ = false;
    bool solidsDirty_ = true;

    void onTrigger(int triggerId) override;
    void rebuildSolids();
};

// StageManager.cpp에서 Stage3로 등록합니다.
}
