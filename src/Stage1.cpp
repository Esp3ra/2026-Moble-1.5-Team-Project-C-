#include "Stage1.hpp"

#include "RenderCommon.hpp"

#include <algorithm>

namespace devil
{
Stage1::Stage1()
{
    reset();
}

Vec2 Stage1::spawnPoint() const
{
    return {Stage1Layout::spawnX, Stage1Layout::floor - GameConfig::playerHeight};
}
void Stage1::rebuildSolids()
{
    const float depth = GameConfig::height - Stage1Layout::floor + 40.f;
    solids_.clear();
    if (!holeOpen_)
    {
        solids_.push_back({GameConfig::left, Stage1Layout::floor,
            GameConfig::right - GameConfig::left, depth});
        return;
    }

    solids_.push_back({GameConfig::left, Stage1Layout::floor,
        Stage1Layout::holeX - GameConfig::left, depth});
    solids_.push_back({Stage1Layout::holeX + Stage1Layout::holeWidth, Stage1Layout::floor,
        GameConfig::right - Stage1Layout::holeX - Stage1Layout::holeWidth, depth});
}

void Stage1::reset()
{
    holeOpen_ = false;
    holeTime_ = 0.f;
    rebuildSolids();
}

void Stage1::update(PlayerState& player, float dt)
{
    if (!holeOpen_ && player.x + GameConfig::playerWidth >= Stage1Layout::triggerX)
    {
        holeOpen_ = true;
        holeTime_ = 0.f;
        rebuildSolids();
    }
    if (holeOpen_)
    {
        holeTime_ += dt;
    }
}

float Stage1::holeDepth() const
{
    if (!holeOpen_) return 0.f;
    const float progress = std::clamp(
        holeTime_ / Stage1Layout::holeOpenDuration, 0.f, 1.f);
    // 영상처럼 처음에는 작게 벌어지고 아래쪽으로 갈수록 빠르게 열립니다.
    const float eased = progress * progress;
    return (GameConfig::height - Stage1Layout::floor) * eased;
}

bool Stage1::touchesHazard(const RectF&) const
{
    return false;
}

bool Stage1::isAtGoal(const PlayerState& player) const
{
    const RectF doorBounds{Stage1Layout::doorX,
        Stage1Layout::floor - Stage1Layout::doorHeight,
        Stage1Layout::doorWidth, Stage1Layout::doorHeight};
    return player.bounds().intersects(doorBounds);
}

Vec2 Stage1::goalPoint() const
{
    return {Stage1Layout::doorX +
        (Stage1Layout::doorWidth - GameConfig::playerWidth) * .5f,
        Stage1Layout::floor - GameConfig::playerHeight};
}

void Stage1::draw(sf::RenderTarget& target) const
{
    using namespace render;
    box(target, GameConfig::left, GameConfig::ceiling,
        GameConfig::right - GameConfig::left, Stage1Layout::floor - GameConfig::ceiling, air);
    if (holeOpen_)
    {
        box(target, Stage1Layout::holeX, Stage1Layout::floor,
            Stage1Layout::holeWidth, holeDepth(), air);
    }
    drawDoor(target, Stage1Layout::doorX, Stage1Layout::floor,
        Stage1Layout::doorWidth, Stage1Layout::doorHeight);
}
}
