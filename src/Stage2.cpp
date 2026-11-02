#include "Stage2.hpp"

#include "RenderCommon.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>

namespace devil
{
namespace
{
float animatedHeight(bool triggered, float elapsed, float duration, float maximum)
{
    if (!triggered) return 0.f;
    const float progress = std::clamp(elapsed / duration, 0.f, 1.f);
    // 처음에는 빠르게 튀어나오고 끝부분에서 부드럽게 멈춥니다.
    const float eased = 1.f - (1.f - progress) * (1.f - progress);
    return maximum * eased;
}

constexpr int spikeCount = 3;
constexpr float spikeToothWidth = 4.f;
constexpr float spikeSideMargin = 4.f;
constexpr float spikeBaseHeight = 2.f;
constexpr float spikeShoulderWidth = 8.f;
constexpr float spikeShoulderHeight = 4.f;

float spikeToothX(float bankX, float bankWidth, int index)
{
    const float usable = bankWidth - spikeSideMargin * 2.f - spikeToothWidth;
    return bankX + spikeSideMargin
        + usable * static_cast<float>(index) / static_cast<float>(spikeCount - 1);
}

void drawSpikeBank(sf::RenderTarget& target, float x, float baseY,
    float width, float height, bool pointsDown)
{
    if (height <= 0.f || width <= 0.f) return;

    const float baseHeight = std::min(spikeBaseHeight, height);
    if (pointsDown)
        render::box(target, x, baseY, width, baseHeight, render::earth);
    else
        render::box(target, x, baseY - baseHeight, width, baseHeight, render::earth);

    const float shoulderHeight = std::min(spikeShoulderHeight,
        std::max(0.f, height - baseHeight));
    const float toothHeight = std::max(0.f,
        height - baseHeight - shoulderHeight);
    for (int i = 0; i < spikeCount; ++i)
    {
        const float toothX = spikeToothX(x, width, i);
        const float shoulderX = toothX
            - (spikeShoulderWidth - spikeToothWidth) * .5f;
        if (pointsDown)
        {
            render::box(target, shoulderX, baseY + baseHeight,
                spikeShoulderWidth, shoulderHeight, render::earth);
            render::box(target, toothX, baseY + baseHeight + shoulderHeight,
                spikeToothWidth, toothHeight, render::earth);
        }
        else
        {
            render::box(target, shoulderX,
                baseY - baseHeight - shoulderHeight,
                spikeShoulderWidth, shoulderHeight, render::earth);
            render::box(target, toothX, baseY - height,
                spikeToothWidth, toothHeight, render::earth);
        }
    }
}

bool touchesSpikeBank(const RectF& playerBounds, float x, float baseY,
    float width, float height, bool pointsDown)
{
    if (height <= 0.f) return false;

    const float baseHeight = std::min(spikeBaseHeight, height);
    const RectF base{x, pointsDown ? baseY : baseY - baseHeight,
        width, baseHeight};
    if (playerBounds.intersects(base)) return true;

    const float shoulderHeight = std::min(spikeShoulderHeight,
        std::max(0.f, height - baseHeight));
    const float toothHeight = std::max(0.f,
        height - baseHeight - shoulderHeight);
    for (int i = 0; i < spikeCount; ++i)
    {
        const float toothX = spikeToothX(x, width, i);
        const float shoulderX = toothX
            - (spikeShoulderWidth - spikeToothWidth) * .5f;
        const RectF shoulder{shoulderX,
            pointsDown ? baseY + baseHeight : baseY - baseHeight - shoulderHeight,
            spikeShoulderWidth, shoulderHeight};
        const RectF tooth{toothX,
            pointsDown ? baseY + baseHeight + shoulderHeight : baseY - height,
            spikeToothWidth, toothHeight};
        if (playerBounds.intersects(shoulder) || playerBounds.intersects(tooth))
            return true;
    }
    return false;
}
}

Stage2::Stage2()
{
    reset();
}

Vec2 Stage2::spawnPoint() const
{
    return {Stage2Layout::spawnX, Stage2Layout::floor - GameConfig::playerHeight};
}

void Stage2::reset()
{
    floorSpikesTriggered_ = true; // 첫 가시는 미리 보여 주어 점프를 유도합니다.
    firstMoveTriggered_ = false;
    finalSpikesTriggered_ = false;
    holeTriggered_ = false;
    edgeTriggered_ = false;
    exitHoleTriggered_ = false;
    edgeTime_ = 0.f;
    exitHoleTime_ = 0.f;
    floorSpikeTime_ = Stage2Layout::floorRiseDuration;
    firstMoveTime_ = 0.f;
    finalSpikeTime_ = 0.f;
    holeTime_ = 0.f;
    rebuildSolids();
}

void Stage2::rebuildSolids()
{
    const float depth = GameConfig::height - Stage2Layout::floor + 40.f;
    solids_.clear();
    // 왼쪽부터 열린 구멍을 제외한 바닥만 등록합니다(Stage1과 같은 방식).
    float cursor = GameConfig::left;
    const auto cutHole = [&](float x, float width)
    {
        if (x > cursor)
            solids_.push_back({cursor, Stage2Layout::floor, x - cursor, depth});
        cursor = x + width;
    };
    if (exitHoleTriggered_)
        cutHole(Stage2Layout::exitHoleX, Stage2Layout::exitHoleWidth);
    if (holeOpen())
    {
        const float extra = edgeTriggered_ ? Stage2Layout::edgeWidth : 0.f;
        cutHole(Stage2Layout::holeX - extra, Stage2Layout::holeWidth + extra);
    }
    if (cursor < GameConfig::right)
        solids_.push_back({cursor, Stage2Layout::floor, GameConfig::right - cursor, depth});
}

void Stage2::update(PlayerState& player, float dt)
{
    // 이 방은 오른쪽에서 왼쪽으로 진행하므로 X가 작아질 때 함정이 작동합니다.
    // 공중에서도 접근을 감지해, 일찍 점프하면 착지 예상 지점으로 이동합니다.
    if (!firstMoveTriggered_ && player.x <= Stage2Layout::firstMoveTriggerX)
        firstMoveTriggered_ = true;

    if (!finalSpikesTriggered_ && player.x <= Stage2Layout::finalTriggerX)
        finalSpikesTriggered_ = true;

    // 구멍 직전에 접근하면 바닥을 제거해 짧은 점프 여유를 줍니다.
    const bool wasOpen = holeOpen();
    const bool wasEdgeOpen = edgeTriggered_;
    const bool wasExitOpen = exitHoleTriggered_;
    if (player.x <= Stage2Layout::holeTriggerX) holeTriggered_ = true;
    if (holeTriggered_ && player.x <= Stage2Layout::edgeTriggerX) edgeTriggered_ = true;
    if (player.x <= Stage2Layout::exitHoleTriggerX) exitHoleTriggered_ = true;
    dt = std::max(0.f, dt);
    if (holeTriggered_) holeTime_ += dt;
    if (edgeTriggered_) edgeTime_ += dt;
    if (exitHoleTriggered_) exitHoleTime_ += dt;
    if (wasOpen != holeOpen() || wasEdgeOpen != edgeTriggered_
        || wasExitOpen != exitHoleTriggered_) rebuildSolids();

    if (floorSpikesTriggered_)
        floorSpikeTime_ += dt;
    if (firstMoveTriggered_)
        firstMoveTime_ = std::min(firstMoveTime_ + dt, Stage2Layout::spikeMoveDuration);
    if (finalSpikesTriggered_)
        finalSpikeTime_ += dt;
}

float Stage2::floorSpikeHeight() const
{
    return animatedHeight(floorSpikesTriggered_, floorSpikeTime_,
        Stage2Layout::floorRiseDuration, Stage2Layout::floorSpikeHeight);
}

float Stage2::finalSpikeHeight() const
{
    return animatedHeight(finalSpikesTriggered_, finalSpikeTime_,
        Stage2Layout::floorRiseDuration, Stage2Layout::floorSpikeHeight);
}

float Stage2::firstSpikeX() const
{
    // 문이 있는 왼쪽으로 한 번 이동합니다. 가까이서 점프하면 넘어갈 수 있습니다.
    return Stage2Layout::firstSpikeX - Stage2Layout::firstMoveDistance * std::clamp(
        firstMoveTime_ / Stage2Layout::spikeMoveDuration, 0.f, 1.f);
}

float Stage2::finalSpikeX() const
{
    return Stage2Layout::floorSpikeX;
}

float Stage2::holeDepth() const
{
    if (!holeOpen()) return 0.f;
    const float progress = std::clamp(holeTime_
        / Stage2Layout::holeOpenDuration, 0.f, 1.f);
    return (GameConfig::height - Stage2Layout::floor) * progress * progress;
}

bool Stage2::touchesHazard(const RectF& playerBounds) const
{
    const float floorHeight = floorSpikeHeight();
    if (floorHeight > 0.f)
    {
        if (touchesSpikeBank(playerBounds, firstSpikeX(),
                Stage2Layout::floor, Stage2Layout::floorSpikeBankWidth,
                floorHeight, false))
            return true;
    }

    if (touchesSpikeBank(playerBounds, finalSpikeX(),
            Stage2Layout::floor, Stage2Layout::floorSpikeBankWidth,
            finalSpikeHeight(), false))
        return true;
    return false;
}

bool Stage2::isAtGoal(const PlayerState& player) const
{
    const RectF doorBounds{Stage2Layout::doorX,
        Stage2Layout::floor - Stage2Layout::doorHeight,
        Stage2Layout::doorWidth, Stage2Layout::doorHeight};
    return player.bounds().intersects(doorBounds);
}

Vec2 Stage2::goalPoint() const
{
    return {Stage2Layout::doorX
            + (Stage2Layout::doorWidth - GameConfig::playerWidth) * .5f,
        Stage2Layout::floor - GameConfig::playerHeight};
}

void Stage2::draw(sf::RenderTarget& target) const
{
    using namespace render;
    box(target, GameConfig::left, GameConfig::ceiling,
        GameConfig::right - GameConfig::left,
        Stage2Layout::floor - GameConfig::ceiling, air);
    drawDoor(target, Stage2Layout::doorX, Stage2Layout::floor,
        Stage2Layout::doorWidth, Stage2Layout::doorHeight);

    const float floorHeight = floorSpikeHeight();
    drawSpikeBank(target, firstSpikeX(), Stage2Layout::floor,
        Stage2Layout::floorSpikeBankWidth, floorHeight, false);
    drawSpikeBank(target,
        finalSpikeX(),
        Stage2Layout::floor, Stage2Layout::floorSpikeBankWidth,
        finalSpikeHeight(), false);

    if (holeOpen())
        box(target, Stage2Layout::holeX, Stage2Layout::floor,
            Stage2Layout::holeWidth, holeDepth(), air);
    const auto drawCollapse = [&](float x, float width, float time)
    {
        const float progress = std::clamp(time / Stage2Layout::holeOpenDuration, 0.f, 1.f);
        box(target, x, Stage2Layout::floor, width,
            (GameConfig::height - Stage2Layout::floor) * progress * progress, air);
    };
    if (edgeTriggered_)
        drawCollapse(Stage2Layout::holeX - Stage2Layout::edgeWidth,
            Stage2Layout::edgeWidth, edgeTime_);
    if (exitHoleTriggered_)
        drawCollapse(Stage2Layout::exitHoleX, Stage2Layout::exitHoleWidth, exitHoleTime_);
}
}
