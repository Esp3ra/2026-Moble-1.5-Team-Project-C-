#include "FinalStage.hpp"

#include "RenderCommon.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <cmath>

namespace devil
{
namespace
{
constexpr sf::Color finalEarth{91, 66, 48};
constexpr sf::Color finalAir{246, 177, 111};
// 모든 위험물도 최종 방의 갈색 계열에서 명도만 낮춰 통일합니다.
constexpr sf::Color trapColor{42, 29, 23};
constexpr sf::Color seamColor{123, 83, 58};
constexpr float pi = 3.14159265358979323846f;
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

void drawStage2SpikeBank(sf::RenderTarget& target, float x, float baseY,
    float width, float height, sf::Color color)
{
    if (height <= 0.f || width <= 0.f) return;

    const float baseHeight = std::min(spikeBaseHeight, height);
    render::box(target, x, baseY - baseHeight, width, baseHeight, color);

    const float shoulderHeight = std::min(spikeShoulderHeight,
        std::max(0.f, height - baseHeight));
    const float toothHeight = std::max(0.f,
        height - baseHeight - shoulderHeight);
    for (int index = 0; index < spikeCount; ++index)
    {
        const float toothX = spikeToothX(x, width, index);
        const float shoulderX = toothX
            - (spikeShoulderWidth - spikeToothWidth) * .5f;
        render::box(target, shoulderX, baseY - baseHeight - shoulderHeight,
            spikeShoulderWidth, shoulderHeight, color);
        render::box(target, toothX, baseY - height,
            spikeToothWidth, toothHeight, color);
    }
}

bool touchesStage2SpikeBank(const RectF& playerBounds, float x, float baseY,
    float width, float height)
{
    if (height <= 0.f || width <= 0.f) return false;

    const float baseHeight = std::min(spikeBaseHeight, height);
    if (playerBounds.intersects({x, baseY - baseHeight, width, baseHeight}))
        return true;

    const float shoulderHeight = std::min(spikeShoulderHeight,
        std::max(0.f, height - baseHeight));
    const float toothHeight = std::max(0.f,
        height - baseHeight - shoulderHeight);
    for (int index = 0; index < spikeCount; ++index)
    {
        const float toothX = spikeToothX(x, width, index);
        const float shoulderX = toothX
            - (spikeShoulderWidth - spikeToothWidth) * .5f;
        const RectF shoulder{shoulderX, baseY - baseHeight - shoulderHeight,
            spikeShoulderWidth, shoulderHeight};
        const RectF tooth{toothX, baseY - height,
            spikeToothWidth, toothHeight};
        if (playerBounds.intersects(shoulder) || playerBounds.intersects(tooth))
            return true;
    }
    return false;
}

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
    saw.setFillColor(trapColor);
    target.draw(saw);

    sf::CircleShape hub(radius * .22f, 16);
    hub.setOrigin({radius * .22f, radius * .22f});
    hub.setPosition({std::round(x), std::round(y)});
    hub.setFillColor(finalAir);
    hub.setOutlineThickness(3.f);
    hub.setOutlineColor(seamColor);
    target.draw(hub);
}

void drawChaser(sf::RenderTarget& target, float frontX)
{
    using namespace render;
    constexpr float toothDepth = 22.f;
    constexpr int toothCount = 4;
    const float top = FinalStageLayout::ceiling;
    const float height = FinalStageLayout::upperFloor - top;
    box(target, GameConfig::left - 36.f, top,
        std::max(0.f, frontX - GameConfig::left + 36.f), height, finalEarth);

    const float toothHeight = height / static_cast<float>(toothCount);
    for (int index = 0; index < toothCount; ++index)
    {
        sf::ConvexShape tooth(3);
        tooth.setPoint(0, {0.f, 0.f});
        tooth.setPoint(1, {toothDepth, toothHeight * .5f});
        tooth.setPoint(2, {0.f, toothHeight});
        tooth.setPosition({std::round(frontX),
            std::round(top + static_cast<float>(index) * toothHeight)});
        tooth.setFillColor(trapColor);
        target.draw(tooth);
    }
}
}

FinalStage::FinalStage()
{
    reset();
}

Vec2 FinalStage::spawnPoint() const
{
    return {FinalStageLayout::spawnX,
        FinalStageLayout::upperFloor - GameConfig::playerHeight};
}

void FinalStage::reset()
{
    chaserActive_ = false;
    chaserX_ = FinalStageLayout::chaserStartX;
    upperGapOpen_ = false;
    upperGapTime_ = 0.f;
    pushWallTriggered_ = false;
    pushWallTime_ = 0.f;
    upperSpikeTriggered_ = false;
    upperSpikeTime_ = 0.f;
    descentStarted_ = false;
    descentTime_ = 0.f;
    floorSawTriggered_ = false;
    floorSawTime_ = 0.f;
    lowerGapOpen_ = false;
    lowerGapTime_ = 0.f;
    dartMode_ = DartMode::Waiting;
    dartX_ = GameConfig::left - FinalStageLayout::dartWidth - 4.f;
    ceilingBlockTriggered_ = false;
    ceilingBlockTime_ = 0.f;
    sawRotation_ = 0.f;
    rebuildSolids();
}

float FinalStage::upperGapProgress() const
{
    return upperGapOpen_ ? smoothStep(upperGapTime_ / .26f) : 0.f;
}

float FinalStage::pushWallProgress() const
{
    if (!pushWallTriggered_ || pushWallTime_ < FinalStageLayout::pushWallDelay)
        return 0.f;

    const float riseEnd = FinalStageLayout::pushWallDelay +
        FinalStageLayout::pushWallRiseDuration;
    if (pushWallTime_ < riseEnd)
        return smoothStep((pushWallTime_ - FinalStageLayout::pushWallDelay) /
            FinalStageLayout::pushWallRiseDuration);
    if (pushWallTime_ < FinalStageLayout::pushWallHoldEnd) return 1.f;
    if (pushWallTime_ < FinalStageLayout::pushWallRetractEnd)
        return 1.f - smoothStep((pushWallTime_ - FinalStageLayout::pushWallHoldEnd) /
            (FinalStageLayout::pushWallRetractEnd - FinalStageLayout::pushWallHoldEnd));
    return 0.f;
}

RectF FinalStage::pushWallBounds() const
{
    const float height = FinalStageLayout::pushWallHeight * pushWallProgress();
    return {FinalStageLayout::pushWallX, FinalStageLayout::upperFloor - height,
        FinalStageLayout::pushWallWidth, height};
}

float FinalStage::upperSpikeProgress() const
{
    if (!upperSpikeTriggered_ ||
        upperSpikeTime_ <= FinalStageLayout::upperSpikeWarningDuration) return 0.f;
    return smoothStep((upperSpikeTime_ - FinalStageLayout::upperSpikeWarningDuration) /
        FinalStageLayout::upperSpikeRiseDuration);
}

float FinalStage::floorSawProgress() const
{
    if (!floorSawTriggered_ ||
        floorSawTime_ <= FinalStageLayout::floorSawWarningDuration) return 0.f;
    return smoothStep((floorSawTime_ - FinalStageLayout::floorSawWarningDuration) /
        FinalStageLayout::floorSawRiseDuration);
}

float FinalStage::floorSawY() const
{
    const float hiddenY = FinalStageLayout::lowerFloor +
        FinalStageLayout::floorSawRadius + 6.f;
    const float raisedY = FinalStageLayout::lowerFloor +
        FinalStageLayout::floorSawRadius - FinalStageLayout::floorSawExposedHeight;
    return hiddenY + (raisedY - hiddenY) * floorSawProgress();
}

float FinalStage::floorSawVisibleHeight() const
{
    return std::max(0.f, FinalStageLayout::lowerFloor -
        (floorSawY() - FinalStageLayout::floorSawRadius));
}

float FinalStage::lowerGapProgress() const
{
    return lowerGapOpen_ ? smoothStep(lowerGapTime_ / .26f) : 0.f;
}

bool FinalStage::dartActive() const
{
    return dartMode_ == DartMode::Flying;
}

bool FinalStage::ceilingBlockGone() const
{
    return ceilingBlockTriggered_ &&
        ceilingBlockTime_ >= FinalStageLayout::ceilingBlockRetractEnd;
}

float FinalStage::ceilingBlockProgress() const
{
    if (!ceilingBlockTriggered_ ||
        ceilingBlockTime_ < FinalStageLayout::ceilingBlockWarningDuration)
        return 0.f;

    const float fallEnd = FinalStageLayout::ceilingBlockWarningDuration
        + FinalStageLayout::ceilingBlockFallDuration;
    if (ceilingBlockTime_ < fallEnd)
        return smoothStep((ceilingBlockTime_ -
            FinalStageLayout::ceilingBlockWarningDuration) /
            FinalStageLayout::ceilingBlockFallDuration);
    if (ceilingBlockTime_ < FinalStageLayout::ceilingBlockHoldEnd) return 1.f;
    if (ceilingBlockTime_ < FinalStageLayout::ceilingBlockRetractEnd)
        return 1.f - smoothStep((ceilingBlockTime_ -
            FinalStageLayout::ceilingBlockHoldEnd) /
            (FinalStageLayout::ceilingBlockRetractEnd -
                FinalStageLayout::ceilingBlockHoldEnd));
    return 0.f;
}

RectF FinalStage::ceilingBlockBounds() const
{
    const float targetY = FinalStageLayout::lowerFloor -
        FinalStageLayout::ceilingBlockHeight;
    const float y = FinalStageLayout::lowerCeiling +
        (targetY - FinalStageLayout::lowerCeiling) * ceilingBlockProgress();
    return {FinalStageLayout::ceilingBlockX, y,
        FinalStageLayout::ceilingBlockWidth, FinalStageLayout::ceilingBlockHeight};
}

RectF FinalStage::chaserBounds() const
{
    return {GameConfig::left - 40.f, FinalStageLayout::ceiling,
        std::max(0.f, chaserX_ - GameConfig::left + 48.f),
        FinalStageLayout::upperFloor - FinalStageLayout::ceiling};
}

RectF FinalStage::upperSpikeBounds() const
{
    const float height = FinalStageLayout::upperSpikeHeight * upperSpikeProgress();
    return {FinalStageLayout::upperSpikeX, FinalStageLayout::upperFloor - height,
        FinalStageLayout::upperSpikeWidth, height};
}

float FinalStage::wallSawX(std::size_t index) const
{
    constexpr std::array<float, FinalStageLayout::wallSawCount> centersX{
        FinalStageLayout::shaftRight - 4.f,
        FinalStageLayout::shaftLeft + 4.f,
        FinalStageLayout::shaftRight - 4.f};
    if (index >= FinalStageLayout::wallSawCount) return 0.f;
    if (!descentStarted_) return centersX[index];

    const float progress = smoothStep(
        descentTime_ / FinalStageLayout::wallSawMoveDuration);
    const float inward = FinalStageLayout::wallSawTravel * progress;
    return centersX[index] + (index % 2 == 0 ? -inward : inward);
}

RectF FinalStage::wallSawBounds(std::size_t index) const
{
    constexpr std::array<float, FinalStageLayout::wallSawCount> centersY{250.f, 340.f, 430.f};
    if (index >= FinalStageLayout::wallSawCount) return {};
    const float radius = FinalStageLayout::wallSawRadius * .72f;
    return {wallSawX(index) - radius, centersY[index] - radius,
        radius * 2.f, radius * 2.f};
}

RectF FinalStage::floorSawBounds() const
{
    // 보이는 톱니 끝보다 판정을 작게 두어 낮은 원작형 점프로도 넘을 수 있게 합니다.
    const float radius = FinalStageLayout::floorSawRadius * .32f;
    return {FinalStageLayout::floorSawX - radius, floorSawY() - radius,
        radius * 2.f, radius * 2.f};
}

RectF FinalStage::dartBounds() const
{
    return {dartX_, FinalStageLayout::dartY,
        FinalStageLayout::dartWidth, FinalStageLayout::dartHeight};
}

void FinalStage::rebuildSolids()
{
    solids_.clear();

    // 위층 바닥은 아래층 천장까지 이어지는 두꺼운 분리 벽입니다.
    if (upperGapOpen_)
    {
        solids_.push_back({GameConfig::left, FinalStageLayout::upperFloor,
            FinalStageLayout::upperGapX - GameConfig::left,
            FinalStageLayout::lowerCeiling - FinalStageLayout::upperFloor});
        solids_.push_back({FinalStageLayout::upperGapX + FinalStageLayout::upperGapWidth,
            FinalStageLayout::upperFloor,
            FinalStageLayout::shaftLeft - FinalStageLayout::upperGapX -
                FinalStageLayout::upperGapWidth,
            FinalStageLayout::lowerCeiling - FinalStageLayout::upperFloor});
        // 아래층까지 이어져 보이지 않도록 구덩이 바닥 아래를 다시 막습니다.
        solids_.push_back({FinalStageLayout::upperGapX,
            FinalStageLayout::upperFloor + FinalStageLayout::upperPitDepth,
            FinalStageLayout::upperGapWidth,
            FinalStageLayout::lowerCeiling - FinalStageLayout::upperFloor -
                FinalStageLayout::upperPitDepth});
    }
    else
    {
        solids_.push_back({GameConfig::left, FinalStageLayout::upperFloor,
            FinalStageLayout::shaftLeft - GameConfig::left,
            FinalStageLayout::lowerCeiling - FinalStageLayout::upperFloor});
    }

    const RectF pushWall = pushWallBounds();
    if (pushWall.height >= 1.f) solids_.push_back(pushWall);

    solids_.push_back({FinalStageLayout::shaftRight, FinalStageLayout::ceiling,
        GameConfig::right - FinalStageLayout::shaftRight,
        FinalStageLayout::lowerFloor - FinalStageLayout::ceiling});

    const float lowerDepth = GameConfig::height - FinalStageLayout::lowerFloor + 40.f;
    if (lowerGapOpen_)
    {
        solids_.push_back({GameConfig::left, FinalStageLayout::lowerFloor,
            FinalStageLayout::lowerGapX - GameConfig::left, lowerDepth});
        solids_.push_back({FinalStageLayout::lowerGapX + FinalStageLayout::lowerGapWidth,
            FinalStageLayout::lowerFloor,
            FinalStageLayout::shaftRight - FinalStageLayout::lowerGapX -
                FinalStageLayout::lowerGapWidth,
            lowerDepth});
    }
    else
    {
        solids_.push_back({GameConfig::left, FinalStageLayout::lowerFloor,
            FinalStageLayout::shaftRight - GameConfig::left, lowerDepth});
    }
}

void FinalStage::update(PlayerState& player, float dt)
{
    if (!chaserActive_ && player.centerX() >= FinalStageLayout::chaserTriggerX &&
        player.feet() <= FinalStageLayout::upperFloor + 2.f)
        chaserActive_ = true;

    if (chaserActive_ && !descentStarted_)
        chaserX_ = std::min(FinalStageLayout::chaserStopX,
            chaserX_ + FinalStageLayout::chaserSpeed * dt);

    if (!upperGapOpen_ && player.centerX() >= FinalStageLayout::upperGapTriggerX &&
        player.feet() <= FinalStageLayout::upperFloor + 2.f)
    {
        upperGapOpen_ = true;
        upperGapTime_ = 0.f;
        pushWallTriggered_ = true;
        pushWallTime_ = 0.f;
    }
    if (upperGapOpen_) upperGapTime_ += dt;
    if (pushWallTriggered_) pushWallTime_ += dt;

    if (!upperSpikeTriggered_ &&
        player.centerX() >= FinalStageLayout::upperSpikeTriggerX &&
        player.feet() <= FinalStageLayout::upperFloor + 2.f)
    {
        upperSpikeTriggered_ = true;
        upperSpikeTime_ = 0.f;
    }
    if (upperSpikeTriggered_) upperSpikeTime_ += dt;

    if (!descentStarted_ && player.centerX() >= FinalStageLayout::shaftLeft &&
        player.feet() > FinalStageLayout::upperFloor + 4.f)
        descentStarted_ = true;
    if (descentStarted_) descentTime_ += dt;

    // 낙하 통로 안에서는 아래층 함정을 미리 깨우지 않습니다.
    const bool onLowerLevel = player.grounded &&
        std::abs(player.feet() - FinalStageLayout::lowerFloor) <= 2.f;
    if (descentStarted_ && onLowerLevel)
    {
        if (!floorSawTriggered_ && player.centerX() <= FinalStageLayout::floorSawTriggerX)
        {
            floorSawTriggered_ = true;
            floorSawTime_ = 0.f;
        }
        if (!lowerGapOpen_ && player.centerX() <= FinalStageLayout::lowerGapTriggerX)
        {
            lowerGapOpen_ = true;
            lowerGapTime_ = 0.f;
        }
        if (dartMode_ == DartMode::Waiting && player.centerX() <= FinalStageLayout::dartTriggerX)
            dartMode_ = DartMode::Flying;
    }

    if (descentStarted_ && !ceilingBlockTriggered_ &&
        player.centerX() <= FinalStageLayout::ceilingBlockTriggerX &&
        player.feet() > FinalStageLayout::lowerCeiling + 40.f)
    {
        ceilingBlockTriggered_ = true;
        ceilingBlockTime_ = 0.f;
    }
    if (ceilingBlockTriggered_ &&
        ceilingBlockTime_ < FinalStageLayout::ceilingBlockRetractEnd)
        ceilingBlockTime_ += dt;

    if (floorSawTriggered_) floorSawTime_ += dt;
    if (lowerGapOpen_) lowerGapTime_ += dt;

    if (dartMode_ == DartMode::Flying)
    {
        dartX_ += FinalStageLayout::dartSpeed * dt;
        if (dartX_ > GameConfig::right + 12.f) dartMode_ = DartMode::Gone;
    }

    sawRotation_ += 470.f * dt;
    rebuildSolids();
}

bool FinalStage::touchesHazard(const RectF& playerBounds) const
{
    if (chaserActive_ && !descentStarted_ && playerBounds.intersects(chaserBounds()))
        return true;
    if (upperSpikeTriggered_)
    {
        const RectF spikes = upperSpikeBounds();
        if (touchesStage2SpikeBank(playerBounds, spikes.x, spikes.bottom(),
                spikes.width, spikes.height)) return true;
    }

    if (upperGapOpen_)
    {
        const RectF upperPit{FinalStageLayout::upperGapX,
            FinalStageLayout::upperFloor + 12.f,
            FinalStageLayout::upperGapWidth,
            FinalStageLayout::upperPitDepth - 12.f};
        if (playerBounds.intersects(upperPit)) return true;
    }

    for (std::size_t index = 0; index < FinalStageLayout::wallSawCount; ++index)
        if (playerBounds.intersects(wallSawBounds(index))) return true;

    if (floorSawTriggered_ && floorSawVisibleHeight() >= 6.f &&
        playerBounds.intersects(floorSawBounds())) return true;
    if (ceilingBlockTriggered_ && !ceilingBlockGone() &&
        playerBounds.intersects(ceilingBlockBounds())) return true;
    return dartMode_ == DartMode::Flying && playerBounds.intersects(dartBounds());
}

bool FinalStage::isAtGoal(const PlayerState& player) const
{
    if (!descentStarted_ || !floorSawTriggered_ ||
        !lowerGapOpen_ || dartMode_ == DartMode::Waiting || !ceilingBlockGone())
        return false;
    const RectF doorBounds{FinalStageLayout::doorX,
        FinalStageLayout::lowerFloor - FinalStageLayout::doorHeight,
        FinalStageLayout::doorWidth, FinalStageLayout::doorHeight};
    return player.bounds().intersects(doorBounds);
}

Vec2 FinalStage::goalPoint() const
{
    return {FinalStageLayout::doorX +
        (FinalStageLayout::doorWidth - GameConfig::playerWidth) * .5f,
        FinalStageLayout::lowerFloor - GameConfig::playerHeight};
}

void FinalStage::draw(sf::RenderTarget& target) const
{
    using namespace render;
    box(target, 0.f, 0.f, GameConfig::width, GameConfig::height, finalEarth);

    // ㄹ자 동선: 위층 -> 오른쪽 낙하 통로 -> 아래층.
    box(target, GameConfig::left, FinalStageLayout::ceiling,
        FinalStageLayout::shaftRight - GameConfig::left,
        FinalStageLayout::upperFloor - FinalStageLayout::ceiling, finalAir);
    box(target, FinalStageLayout::shaftLeft, FinalStageLayout::upperFloor,
        FinalStageLayout::shaftRight - FinalStageLayout::shaftLeft,
        FinalStageLayout::lowerFloor - FinalStageLayout::upperFloor, finalAir);
    box(target, GameConfig::left, FinalStageLayout::lowerCeiling,
        FinalStageLayout::shaftRight - GameConfig::left,
        FinalStageLayout::lowerFloor - FinalStageLayout::lowerCeiling, finalAir);

    if (upperGapOpen_)
    {
        box(target, FinalStageLayout::upperGapX, FinalStageLayout::upperFloor,
            FinalStageLayout::upperGapWidth,
            FinalStageLayout::upperPitDepth, finalAir);
        box(target, FinalStageLayout::upperGapX,
            FinalStageLayout::upperFloor + FinalStageLayout::upperPitDepth - 6.f,
            FinalStageLayout::upperGapWidth, 6.f, trapColor);
        const float tileY = FinalStageLayout::upperFloor +
            upperGapProgress() * (FinalStageLayout::upperPitDepth + 18.f);
        if (tileY < FinalStageLayout::upperFloor + FinalStageLayout::upperPitDepth - 2.f)
        {
            box(target, FinalStageLayout::upperGapX, tileY,
                FinalStageLayout::upperGapWidth, 14.f, finalEarth);
            box(target, FinalStageLayout::upperGapX + 4.f, tileY + 3.f,
                FinalStageLayout::upperGapWidth - 8.f, 3.f, seamColor);
        }
    }


    const RectF pushWall = pushWallBounds();
    if (pushWall.height > 0.f)
    {
        box(target, pushWall.x, pushWall.y, pushWall.width, pushWall.height, finalEarth);
        box(target, pushWall.x + 3.f, pushWall.y,
            pushWall.width - 6.f, 3.f, seamColor);
    }

    if (lowerGapOpen_)
    {
        box(target, FinalStageLayout::lowerGapX, FinalStageLayout::lowerFloor,
            FinalStageLayout::lowerGapWidth,
            GameConfig::height - FinalStageLayout::lowerFloor, finalAir);
        const float tileY = FinalStageLayout::lowerFloor + lowerGapProgress() * 90.f;
        if (tileY < GameConfig::height + 18.f)
            box(target, FinalStageLayout::lowerGapX, tileY,
                FinalStageLayout::lowerGapWidth, 14.f, finalEarth);
    }

    if (upperSpikeTriggered_)
    {
        // 먼저 바닥 이음새가 보이고, 바로 뒤에 가시가 부드럽게 솟습니다.
        box(target, FinalStageLayout::upperSpikeX, FinalStageLayout::upperFloor - 2.f,
            FinalStageLayout::upperSpikeWidth, 2.f, seamColor);
        const RectF spikes = upperSpikeBounds();
        drawStage2SpikeBank(target, spikes.x, spikes.bottom(), spikes.width,
            spikes.height, trapColor);
    }
    if (chaserActive_ && !descentStarted_) drawChaser(target, chaserX_);

    constexpr std::array<float, FinalStageLayout::wallSawCount> centersY{250.f, 340.f, 430.f};
    for (std::size_t index = 0; index < FinalStageLayout::wallSawCount; ++index)
        drawSaw(target, wallSawX(index), centersY[index],
            FinalStageLayout::wallSawRadius,
            index % 2 == 0 ? sawRotation_ : -sawRotation_);

    if (floorSawTriggered_)
    {
        drawSaw(target, FinalStageLayout::floorSawX, floorSawY(),
            FinalStageLayout::floorSawRadius, -sawRotation_ * 1.12f);
        // 바닥 아래의 톱날은 지면으로 다시 덮어 윗부분만 노출합니다.
        box(target, FinalStageLayout::floorSawX - FinalStageLayout::floorSawRadius - 2.f,
            FinalStageLayout::lowerFloor,
            FinalStageLayout::floorSawRadius * 2.f + 4.f,
            GameConfig::height - FinalStageLayout::lowerFloor, finalEarth);
        box(target, FinalStageLayout::floorSawX - FinalStageLayout::floorSawRadius,
            FinalStageLayout::lowerFloor - 2.f,
            FinalStageLayout::floorSawRadius * 2.f, 2.f, seamColor);
    }

    if (dartMode_ == DartMode::Flying)
    {
        const RectF dart = dartBounds();
        box(target, dart.x, dart.y, dart.width - 7.f, dart.height, trapColor);
        sf::ConvexShape tip(3);
        tip.setPoint(0, {0.f, 0.f});
        tip.setPoint(1, {7.f, dart.height * .5f});
        tip.setPoint(2, {0.f, dart.height});
        tip.setPosition({std::round(dart.right() - 7.f), std::round(dart.y)});
        tip.setFillColor(trapColor);
        target.draw(tip);
    }

    if (ceilingBlockTriggered_ && !ceilingBlockGone())
    {
        const RectF block = ceilingBlockBounds();
        box(target, block.x, block.y, block.width, block.height, finalEarth);
        box(target, block.x + 4.f, block.y + block.height - 5.f,
            block.width - 8.f, 5.f, trapColor);
        box(target, block.x + 3.f, block.y + 3.f,
            block.width - 6.f, 3.f, seamColor);
    }

    drawDoor(target, FinalStageLayout::doorX, FinalStageLayout::lowerFloor,
        FinalStageLayout::doorWidth, FinalStageLayout::doorHeight);
}
}
