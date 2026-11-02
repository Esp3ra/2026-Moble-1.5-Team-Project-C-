#include "Stage6.hpp"

#include "RenderCommon.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <cmath>

namespace devil
{
namespace
{
constexpr sf::Color stageEarth{112, 77, 54};
constexpr sf::Color stageAir{249, 184, 117};
// 톱날도 이 방의 갈색 팔레트 안에서 명암만 낮춰 표현합니다.
constexpr sf::Color sawColor{48, 32, 25};
constexpr sf::Color sawHighlight{126, 84, 57};
constexpr float pi = 3.14159265358979323846f;

float smoothStep(float value)
{
    value = std::clamp(value, 0.f, 1.f);
    return value * value * (3.f - 2.f * value);
}

void drawSaw(sf::RenderTarget& target, float x, float y, float radius, float rotation)
{
    constexpr std::size_t pointCount = 40;
    sf::ConvexShape saw(pointCount);
    for (std::size_t index = 0; index < pointCount; ++index)
    {
        const float angle = static_cast<float>(index) * 2.f * pi /
            static_cast<float>(pointCount);
        const float pointRadius = index % 2 == 0 ? radius : radius * .73f;
        saw.setPoint(index, {std::cos(angle) * pointRadius + radius,
            std::sin(angle) * pointRadius + radius});
    }
    saw.setOrigin({radius, radius});
    saw.setPosition({std::round(x), std::round(y)});
    saw.setRotation(sf::degrees(rotation));
    saw.setFillColor(sawColor);
    target.draw(saw);

    sf::CircleShape hub(radius * .22f, 16);
    hub.setOrigin({radius * .22f, radius * .22f});
    hub.setPosition({std::round(x), std::round(y)});
    hub.setFillColor(stageAir);
    hub.setOutlineThickness(3.f);
    hub.setOutlineColor(sawHighlight);
    target.draw(hub);
}

void drawSpikeWall(sf::RenderTarget& target, float frontCenterX, float centerY)
{
    constexpr int toothCount = 4;
    constexpr float spikeDepth = 17.f;
    const float halfHeight = Stage6Layout::chaseRadius;
    const float top = centerY - halfHeight;
    const float height = halfHeight * 2.f;
    const float spikeBaseX = frontCenterX + 7.f;
    const float wallLeft = GameConfig::left - 48.f;

    render::box(target, wallLeft, top,
        std::max(0.f, spikeBaseX - wallLeft), height, stageEarth);
    render::box(target, wallLeft, top + 5.f,
        std::max(0.f, spikeBaseX - wallLeft - 4.f), 4.f, sawHighlight);
    render::box(target, spikeBaseX - 5.f, top, 5.f, height, sawColor);

    const float toothHeight = height / static_cast<float>(toothCount);
    for (int index = 0; index < toothCount; ++index)
    {
        sf::ConvexShape tooth(3);
        tooth.setPoint(0, {0.f, 1.f});
        tooth.setPoint(1, {spikeDepth, toothHeight * .5f});
        tooth.setPoint(2, {0.f, toothHeight - 1.f});
        tooth.setPosition({std::round(spikeBaseX),
            std::round(top + static_cast<float>(index) * toothHeight)});
        tooth.setFillColor(sawColor);
        target.draw(tooth);
    }
}
}

Stage6::Stage6()
{
    reset();
}

Vec2 Stage6::spawnPoint() const
{
    return {Stage6Layout::spawnX,
        Stage6Layout::startFloor - GameConfig::playerHeight};
}

void Stage6::reset()
{
    chaseMode_ = ChaseMode::Hidden;
    chaseX_ = GameConfig::left - Stage6Layout::chaseRadius - 2.f;
    chaseY_ = Stage6Layout::startFloor - Stage6Layout::chaseRadius;
    chaseReturnStartX_ = chaseX_;
    chaseReturnTime_ = 0.f;
    escapeTriggered_ = false;
    escapeTime_ = 0.f;
    platformRotation_ = 0.f;
    doorSawTriggered_ = false;
    doorSawTime_ = 0.f;
    playerAscending_ = false;
    rebuildSolids();
}

bool Stage6::chaseStarted() const
{
    return chaseMode_ != ChaseMode::Hidden;
}

bool Stage6::chaseGone() const
{
    return chaseMode_ == ChaseMode::Gone;
}

float Stage6::bridgeDropProgress(std::size_t index) const
{
    if (!escapeTriggered_ || index >= Stage6Layout::bridgeTileCount) return 0.f;
    constexpr float firstDelay = .58f;
    constexpr float stagger = .11f;
    constexpr float duration = .34f;
    const float localTime = escapeTime_ - firstDelay - static_cast<float>(index) * stagger;
    const float progress = std::clamp(localTime / duration, 0.f, 1.f);
    return progress * progress;
}

float Stage6::platformSawProgress(std::size_t index) const
{
    if (!escapeTriggered_ || index >= Stage6Layout::platformSawCount) return 0.f;
    constexpr std::array<float, Stage6Layout::platformSawCount> delays{.06f, .35f, .66f};
    constexpr std::array<float, Stage6Layout::platformSawCount> durations{.42f, .46f, .50f};
    return smoothStep((escapeTime_ - delays[index]) / durations[index]);
}

float Stage6::platformSawX(std::size_t index) const
{
    constexpr std::array<float, Stage6Layout::platformSawCount> values{
        Stage6Layout::firstSawX, Stage6Layout::secondSawX, Stage6Layout::thirdSawX};
    return values[std::min(index, values.size() - 1)];
}

float Stage6::platformSawTargetY(std::size_t index) const
{
    constexpr std::array<float, Stage6Layout::platformSawCount> values{
        Stage6Layout::firstSawY, Stage6Layout::secondSawY, Stage6Layout::thirdSawY};
    return values[std::min(index, values.size() - 1)];
}

float Stage6::platformSawY(std::size_t index) const
{
    const float hiddenY = GameConfig::height + Stage6Layout::platformSawRadius + 8.f;
    return hiddenY + (platformSawTargetY(index) - hiddenY) * platformSawProgress(index);
}

RectF Stage6::sawTop(std::size_t index) const
{
    return {platformSawX(index) - Stage6Layout::sawTopWidth * .5f,
        platformSawY(index) - Stage6Layout::platformSawRadius - 2.f,
        Stage6Layout::sawTopWidth, 4.f};
}

float Stage6::doorSawProgress() const
{
    if (!doorSawTriggered_ ||
        doorSawTime_ <= Stage6Layout::doorSawWarningDuration) return 0.f;

    const float riseEnd = Stage6Layout::doorSawWarningDuration +
        Stage6Layout::doorSawRiseDuration;
    if (doorSawTime_ < riseEnd)
        return smoothStep((doorSawTime_ - Stage6Layout::doorSawWarningDuration) /
            Stage6Layout::doorSawRiseDuration);
    if (doorSawTime_ <= Stage6Layout::doorSawHoldEnd) return 1.f;

    return 1.f - smoothStep((doorSawTime_ - Stage6Layout::doorSawHoldEnd) /
        (Stage6Layout::doorSawRetractEnd - Stage6Layout::doorSawHoldEnd));
}

float Stage6::doorSawY() const
{
    const float hiddenY = Stage6Layout::rightLedgeFloor +
        Stage6Layout::doorSawRadius + 6.f;
    const float raisedY = Stage6Layout::rightLedgeFloor +
        Stage6Layout::doorSawRadius - Stage6Layout::doorSawExposedHeight;
    return hiddenY + (raisedY - hiddenY) * doorSawProgress();
}

RectF Stage6::chaseHitbox() const
{
    const float radius = Stage6Layout::chaseRadius * .72f;
    return {chaseX_ - radius, chaseY_ - radius, radius * 2.f, radius * 2.f};
}

RectF Stage6::sawBody(std::size_t index) const
{
    // 보이는 톱니 끝보다 판정은 작게 잡아 위의 좁은 발판으로 착지할 여지를 둡니다.
    const float radius = Stage6Layout::platformSawRadius * .42f;
    return {platformSawX(index) - radius, platformSawY(index) - radius,
        radius * 2.f, radius * 2.f};
}

RectF Stage6::doorSawBody() const
{
    const float radius = Stage6Layout::doorSawRadius * .44f;
    return {Stage6Layout::doorSawX - radius, doorSawY() - radius,
        radius * 2.f, radius * 2.f};
}

void Stage6::rebuildSolids()
{
    solids_.clear();
    const float startDepth = GameConfig::height - Stage6Layout::startFloor + 40.f;

    solids_.push_back({GameConfig::left, Stage6Layout::startFloor,
        Stage6Layout::startLedgeEnd - GameConfig::left, startDepth});

    for (std::size_t index = 0; index < Stage6Layout::bridgeTileCount; ++index)
    {
        if (bridgeDropProgress(index) <= 0.f)
        {
            solids_.push_back({Stage6Layout::bridgeX +
                static_cast<float>(index) * Stage6Layout::bridgeTileWidth,
                Stage6Layout::startFloor, Stage6Layout::bridgeTileWidth, 16.f});
        }
    }

    solids_.push_back({Stage6Layout::rightLedgeX, Stage6Layout::rightLedgeFloor,
        Stage6Layout::rightLedgeWidth, GameConfig::height - Stage6Layout::rightLedgeFloor});

    // 톱날의 평평한 윗면은 아래에서 머리를 막지 않는 단방향 발판으로 사용합니다.
    if (!playerAscending_)
        for (std::size_t index = 0; index < Stage6Layout::platformSawCount; ++index)
            if (platformSawProgress(index) >= 1.f) solids_.push_back(sawTop(index));
}

void Stage6::update(PlayerState& player, float dt)
{
    playerAscending_ = player.velocityY < 0.f;
    if (chaseMode_ == ChaseMode::Hidden &&
        player.x + GameConfig::playerWidth >= Stage6Layout::chaseTriggerX)
        chaseMode_ = ChaseMode::Chasing;

    if (!escapeTriggered_ &&
        (player.x + GameConfig::playerWidth >= Stage6Layout::startLedgeEnd - 2.f ||
            player.feet() > Stage6Layout::startFloor + 2.f))
    {
        escapeTriggered_ = true;
        escapeTime_ = 0.f;
    }

    if (escapeTriggered_)
    {
        escapeTime_ += dt;
        platformRotation_ += 430.f * dt;
    }

    if (!doorSawTriggered_ &&
        player.x + GameConfig::playerWidth >= Stage6Layout::doorSawTriggerX &&
        player.feet() <= Stage6Layout::rightLedgeFloor + 2.f)
    {
        doorSawTriggered_ = true;
        doorSawTime_ = 0.f;
    }
    if (doorSawTriggered_ && doorSawTime_ < Stage6Layout::doorSawRetractEnd)
        doorSawTime_ += dt;

    if (chaseMode_ == ChaseMode::Chasing)
    {
        chaseX_ += Stage6Layout::chaseSpeed * dt;
        if (chaseX_ - Stage6Layout::chaseRadius >= Stage6Layout::startLedgeEnd)
        {
            chaseMode_ = ChaseMode::Returning;
            chaseReturnStartX_ = chaseX_;
            chaseReturnTime_ = 0.f;
        }
    }
    else if (chaseMode_ == ChaseMode::Returning)
    {
        chaseReturnTime_ += dt;
        const float progress = smoothStep(chaseReturnTime_ /
            Stage6Layout::chaseReturnDuration);
        const float hiddenX = GameConfig::left - Stage6Layout::chaseRadius - 2.f;
        chaseX_ = chaseReturnStartX_ + (hiddenX - chaseReturnStartX_) * progress;
        if (chaseReturnTime_ >= Stage6Layout::chaseReturnDuration)
        {
            chaseX_ = hiddenX;
            chaseMode_ = ChaseMode::Gone;
        }
    }

    rebuildSolids();
}

bool Stage6::touchesHazard(const RectF& playerBounds) const
{
    if ((chaseMode_ == ChaseMode::Chasing || chaseMode_ == ChaseMode::Returning) &&
        playerBounds.intersects(chaseHitbox())) return true;

    for (std::size_t index = 0; index < Stage6Layout::platformSawCount; ++index)
        if (platformSawProgress(index) > .08f && playerBounds.intersects(sawBody(index)))
            return true;
    if (doorSawProgress() > .08f && playerBounds.intersects(doorSawBody()))
        return true;
    return false;
}

bool Stage6::isAtGoal(const PlayerState& player) const
{
    if (!thirdSawReady()) return false;
    const RectF doorBounds{Stage6Layout::doorX,
        Stage6Layout::rightLedgeFloor - Stage6Layout::doorHeight,
        Stage6Layout::doorWidth, Stage6Layout::doorHeight};
    return player.bounds().intersects(doorBounds);
}

Vec2 Stage6::goalPoint() const
{
    return {Stage6Layout::doorX +
        (Stage6Layout::doorWidth - GameConfig::playerWidth) * .5f,
        Stage6Layout::rightLedgeFloor - GameConfig::playerHeight};
}

void Stage6::draw(sf::RenderTarget& target) const
{
    using namespace render;
    box(target, 0.f, 0.f, GameConfig::width, GameConfig::height, stageEarth);
    box(target, GameConfig::left, Stage6Layout::ceiling,
        GameConfig::right - GameConfig::left,
        GameConfig::height - Stage6Layout::ceiling, stageAir);

    box(target, GameConfig::left, Stage6Layout::startFloor,
        Stage6Layout::startLedgeEnd - GameConfig::left,
        GameConfig::height - Stage6Layout::startFloor, stageEarth);
    box(target, Stage6Layout::rightLedgeX, Stage6Layout::rightLedgeFloor,
        Stage6Layout::rightLedgeWidth,
        GameConfig::height - Stage6Layout::rightLedgeFloor, stageEarth);

    for (std::size_t index = 0; index < Stage6Layout::bridgeTileCount; ++index)
    {
        const float progress = bridgeDropProgress(index);
        const float y = Stage6Layout::startFloor + progress * 150.f;
        if (y < GameConfig::height + 18.f)
        {
            const float x = Stage6Layout::bridgeX +
                static_cast<float>(index) * Stage6Layout::bridgeTileWidth;
            box(target, x, y, Stage6Layout::bridgeTileWidth, 16.f, stageEarth);
            box(target, x + 3.f, y + 3.f,
                Stage6Layout::bridgeTileWidth - 6.f, 3.f, sawHighlight);
        }
    }

    for (std::size_t index = 0; index < Stage6Layout::platformSawCount; ++index)
    {
        if (platformSawProgress(index) <= 0.f) continue;
        drawSaw(target, platformSawX(index), platformSawY(index),
            Stage6Layout::platformSawRadius,
            index % 2 == 0 ? platformRotation_ : -platformRotation_ * 1.08f);
        if (platformSawProgress(index) >= 1.f)
        {
            const RectF top = sawTop(index);
            box(target, top.x, top.y, top.width, top.height, stageEarth);
        }
    }

    if (chaseMode_ == ChaseMode::Chasing || chaseMode_ == ChaseMode::Returning)
        drawSpikeWall(target, chaseX_, chaseY_);

    if (doorSawProgress() > 0.f)
    {
        drawSaw(target, Stage6Layout::doorSawX, doorSawY(),
            Stage6Layout::doorSawRadius, -platformRotation_ * 1.18f);
        box(target, Stage6Layout::doorSawX - Stage6Layout::doorSawRadius - 2.f,
            Stage6Layout::rightLedgeFloor,
            Stage6Layout::doorSawRadius * 2.f + 4.f,
            GameConfig::height - Stage6Layout::rightLedgeFloor, stageEarth);
        box(target, Stage6Layout::doorSawX - Stage6Layout::doorSawRadius,
            Stage6Layout::rightLedgeFloor - 2.f,
            Stage6Layout::doorSawRadius * 2.f, 2.f, sawHighlight);
    }

    drawDoor(target, Stage6Layout::doorX, Stage6Layout::rightLedgeFloor,
        Stage6Layout::doorWidth, Stage6Layout::doorHeight);
}
}
