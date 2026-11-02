#include "Stage3.hpp"

#include "RenderCommon.hpp"

#include <algorithm>
#include <utility>

namespace devil
{
namespace
{
RectF groundSegment(float x, float width)
{
    return {x, Stage3Layout::floorY, width, Stage3Layout::floorDepth};
}
}

Stage3::Stage3()
{
    staticSolids_.reserve(3);
    staticSolids_.push_back(groundSegment(GameConfig::left,
        Stage3Layout::holeX - GameConfig::left));
    staticSolids_.push_back(groundSegment(Stage3Layout::holeX + Stage3Layout::holeWidth,
        GameConfig::right - Stage3Layout::holeX - Stage3Layout::holeWidth));
    staticSolids_.push_back({Stage3Layout::platform1X, Stage3Layout::platform1Y,
        Stage3Layout::platform1Width, Stage3Layout::platform1Height});

    coins_.reserve(3);
    dynamicFloors_.reserve(1);
    hazards_.reserve(2);
    ceilings_.reserve(1);
    solids_.reserve(staticSolids_.size() + dynamicFloors_.capacity() + ceilings_.capacity());

    dynamicFloors_.push_back(std::make_unique<DisappearingFloor>(1,
        RectF{Stage3Layout::holeX, Stage3Layout::floorY,
            Stage3Layout::holeWidth, Stage3Layout::floorDepth}));

    auto movingWall = std::make_unique<MovingWall>(2,
        RectF{Stage3Layout::wallX, Stage3Layout::wallY, Stage3Layout::wallWidth,
            Stage3Layout::wallHeight},
        Stage3Layout::wallSpeed, Stage3Layout::wallStopX);
    MovingWall& movingWallListener = *movingWall;
    hazards_.push_back(std::move(movingWall));
    hazards_.push_back(std::make_unique<ProximityHazard>(Stage3Layout::proximityX,
        Stage3Layout::floorY, Stage3Layout::proximityWidth, Stage3Layout::proximityHeight,
        Stage3Layout::proximityTriggerX, Stage3Layout::proximityRiseSpeed));

    ceilings_.push_back(std::make_unique<CrushingCeiling>(3,
        RectF{Stage3Layout::ceilingX, Stage3Layout::ceilingStartY,
            Stage3Layout::ceilingWidth, Stage3Layout::ceilingHeight},
        Stage3Layout::ceilingLandingY, Stage3Layout::ceilingFallSpeed));

    coins_.push_back(std::make_unique<Coin>(1,
        RectF{Stage3Layout::coin1X, Stage3Layout::coin1Y,
            Stage3Layout::coinSize, Stage3Layout::coinSize}));
    coins_.push_back(std::make_unique<Coin>(2,
        RectF{Stage3Layout::coin2X, Stage3Layout::coin2Y,
            Stage3Layout::coinSize, Stage3Layout::coinSize}));
    coins_.push_back(std::make_unique<Coin>(3,
        RectF{Stage3Layout::coin3X, Stage3Layout::coin3Y,
            Stage3Layout::coinSize, Stage3Layout::coinSize}));

    for (const auto& floor : dynamicFloors_)
        coins_[0]->subscribe(*floor);
    coins_[1]->subscribe(movingWallListener);
    coins_[2]->subscribe(*ceilings_[0]);
    // The controller observes every trigger so collision snapshots are marked dirty
    // even when the concrete listener itself is the only object whose state changes.
    for (const auto& coin : coins_)
        coin->subscribe(*this);

    reset();
}

Vec2 Stage3::spawnPoint() const
{
    return {Stage3Layout::spawnX, Stage3Layout::floorY - GameConfig::playerHeight};
}

void Stage3::reset()
{
    for (const auto& coin : coins_) coin->reset();
    for (const auto& floor : dynamicFloors_) floor->reset();
    for (const auto& hazard : hazards_) hazard->reset();
    for (const auto& ceiling : ceilings_) ceiling->reset();

    m_currentDoorPos_ = {Stage3Layout::originalDoorX, Stage3Layout::originalDoorFloorY};
    doorRelocated_ = false;

    // Game queries solids before its first update after a retry, so reset commits
    // the initial snapshot immediately. Later frames use the dirty flag below.
    solidsDirty_ = true;
    rebuildSolids();
    solidsDirty_ = false;
}

void Stage3::onTrigger(int triggerId)
{
    solidsDirty_ = true;
    if (triggerId != 3) return;
    doorRelocated_ = true;
    m_currentDoorPos_ = {Stage3Layout::relocatedDoorX, Stage3Layout::relocatedDoorFloorY};
}

void Stage3::rebuildSolids()
{
    solids_.clear();
    solids_.insert(solids_.end(), staticSolids_.begin(), staticSolids_.end());

    for (const auto& floor : dynamicFloors_)
    {
        if (!floor->isActive()) continue;
        solids_.push_back(floor->bounds());
    }
    for (const auto& ceiling : ceilings_)
    {
        if (!ceiling->isSolid()) continue;
        solids_.push_back(ceiling->bounds());
    }
}

void Stage3::update(PlayerState& player, float dt)
{
    const RectF playerBounds = player.bounds();
    for (const auto& coin : coins_)
    {
        if (!coin->isVisible()) continue;
        coin->update(playerBounds, dt);
    }
    for (const auto& hazard : hazards_)
    {
        if (!hazard->needsUpdate(player)) continue;
        hazard->update(player, dt);
    }
    bool ceilingStillChanging = false;
    for (const auto& ceiling : ceilings_)
    {
        if (!ceiling->needsUpdate()) continue;
        ceiling->update(dt);
        if (ceiling->isFalling() || ceiling->needsUpdate())
        {
            // Even before it becomes solid, retain the refresh request while the
            // ceiling is moving. The landing frame inserts its final solid exactly once.
            solidsDirty_ = true;
            ceilingStillChanging = true;
        }
    }

    if (solidsDirty_)
        rebuildSolids();

    // A just-landed ceiling remains dirty for its hazard frame. Once that frame has
    // been consumed, its solid bounds are immutable and later ticks skip rebuilding.
    solidsDirty_ = ceilingStillChanging;
}

bool Stage3::touchesHazard(const RectF& playerBounds) const
{
    for (const auto& hazard : hazards_)
    {
        if (!hazard->isActive()) continue;
        if (hazard->touches(playerBounds)) return true;
    }
    for (const auto& ceiling : ceilings_)
    {
        if (!ceiling->isDangerous()) continue;
        if (playerBounds.intersects(ceiling->bounds())) return true;
    }
    return false;
}

bool Stage3::isAtGoal(const PlayerState& player) const
{
    if (!doorRelocated_) return false;
    const RectF doorBounds{m_currentDoorPos_.x,
        m_currentDoorPos_.y - Stage3Layout::doorHeight,
        Stage3Layout::doorWidth, Stage3Layout::doorHeight};
    return player.bounds().intersects(doorBounds);
}

Vec2 Stage3::goalPoint() const
{
    return {m_currentDoorPos_.x + (Stage3Layout::doorWidth - GameConfig::playerWidth) * .5f,
        m_currentDoorPos_.y - GameConfig::playerHeight};
}

void Stage3::draw(sf::RenderTarget& target) const
{
    using namespace render;
    box(target, GameConfig::left, GameConfig::ceiling,
        GameConfig::right - GameConfig::left, Stage3Layout::floorY - GameConfig::ceiling, air);

    for (const RectF& solid : staticSolids_)
    {
        if (solid.y >= Stage3Layout::floorY) continue;
        box(target, solid.x, solid.y, solid.width, solid.height, earth);
    }
    for (const auto& floor : dynamicFloors_)
    {
        if (floor->isActive()) continue;
        const RectF& hole = floor->bounds();
        box(target, hole.x, hole.y, hole.width, hole.height, air);
    }
    for (const auto& ceiling : ceilings_)
        ceiling->draw(target);
    for (const auto& hazard : hazards_)
    {
        if (!hazard->isActive()) continue;
        hazard->draw(target);
    }
    for (const auto& coin : coins_)
    {
        if (!coin->isVisible()) continue;
        coin->draw(target);
    }

    drawDoor(target, m_currentDoorPos_.x, m_currentDoorPos_.y,
        Stage3Layout::doorWidth, Stage3Layout::doorHeight);
}
}
