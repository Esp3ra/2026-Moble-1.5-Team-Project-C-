#include "Stage3Objects.hpp"

#include "RenderCommon.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace devil
{
namespace
{
constexpr float coinCollectionDuration = .24f;
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

sf::Color withAlpha(sf::Color color, float opacity)
{
    color.a = static_cast<std::uint8_t>(std::round(
        std::clamp(opacity, 0.f, 1.f) * 255.f));
    return color;
}
}

Coin::Coin(int triggerId, RectF bounds) : triggerId_(triggerId), bounds_(bounds)
{
    listeners_.reserve(4);
}

void Coin::subscribe(ITriggerListener& listener)
{
    if (std::find(listeners_.begin(), listeners_.end(), &listener) == listeners_.end())
        listeners_.push_back(&listener);
}

void Coin::reset()
{
    collectionTime_ = 0.f;
    collected_ = false;
}

void Coin::update(const RectF& playerBounds, float dt)
{
    if (!collected_ && playerBounds.intersects(bounds_))
    {
        collected_ = true;
        collectionTime_ = 0.f;
        notifyListeners();
    }

    if (collected_)
        collectionTime_ += std::max(0.f, dt);
}

bool Coin::isVisible() const
{
    return !collected_ || collectionTime_ < coinCollectionDuration;
}

void Coin::notifyListeners()
{
    for (ITriggerListener* const listener : listeners_)
        if (listener != nullptr) listener->onTrigger(triggerId_);
}

void Coin::draw(sf::RenderTarget& target) const
{
    if (!isVisible()) return;

    const float progress = collected_
        ? std::clamp(collectionTime_ / coinCollectionDuration, 0.f, 1.f) : 0.f;
    const float size = bounds_.width * (1.f - progress * .45f);
    const float x = bounds_.x + (bounds_.width - size) * .5f;
    const float y = bounds_.y - progress * 13.f + (bounds_.height - size) * .5f;
    const sf::Color gold = withAlpha(sf::Color(255, 213, 64), 1.f - progress);
    const sf::Color shine = withAlpha(sf::Color(255, 243, 170), 1.f - progress);
    render::box(target, x, y, size, size, gold);
    render::box(target, x + size * .25f, y + size * .16f, size * .27f, size * .68f, shine);
}

DisappearingFloor::DisappearingFloor(int triggerId, RectF bounds)
    : triggerId_(triggerId), bounds_(bounds)
{
}

void DisappearingFloor::reset()
{
    active_ = true;
}

void DisappearingFloor::onTrigger(int triggerId)
{
    if (triggerId == triggerId_) active_ = false;
}

MovingWall::MovingWall(int triggerId, RectF initialBounds, float speed, float stopX)
    : triggerId_(triggerId), initialBounds_(initialBounds), bounds_(initialBounds),
      speed_(speed), stopX_(stopX)
{
}

void MovingWall::reset()
{
    bounds_ = initialBounds_;
    active_ = false;
}

void MovingWall::onTrigger(int triggerId)
{
    if (triggerId == triggerId_) active_ = true;
}

void MovingWall::update(const PlayerState&, float dt)
{
    if (!active_) return;
    bounds_.x = std::min(stopX_, bounds_.x + speed_ * std::max(0.f, dt));
}

void MovingWall::draw(sf::RenderTarget& target) const
{
    if (!active_) return;
    render::box(target, bounds_.x, bounds_.y, bounds_.width, bounds_.height, render::ink);
    render::box(target, bounds_.x + 3.f, bounds_.y, bounds_.width - 6.f,
        bounds_.height, sf::Color(180, 45, 38));

    constexpr float toothHeight = 12.f;
    for (float y = bounds_.y + 4.f; y < bounds_.bottom() - toothHeight; y += toothHeight)
        render::box(target, bounds_.right() - 3.f, y, 6.f, 6.f, render::light);
}

ProximityHazard::ProximityHazard(float x, float floorY, float width, float maximumHeight,
    float triggerX, float riseSpeed)
    : x_(x), floorY_(floorY), width_(width), maximumHeight_(maximumHeight),
      triggerX_(triggerX), riseSpeed_(riseSpeed)
{
}

void ProximityHazard::reset()
{
    currentHeight_ = 0.f;
    active_ = false;
}

void ProximityHazard::update(const PlayerState& player, float dt)
{
    if (!active_ && player.centerX() >= triggerX_) active_ = true;
    if (!active_) return;
    currentHeight_ = std::min(maximumHeight_, currentHeight_ + riseSpeed_ * std::max(0.f, dt));
}

bool ProximityHazard::needsUpdate(const PlayerState& player) const
{
    return active_ || player.centerX() >= triggerX_;
}

RectF ProximityHazard::bounds() const
{
    return {x_, floorY_ - currentHeight_, width_, currentHeight_};
}

bool ProximityHazard::touches(const RectF& playerBounds) const
{
    return active_ && touchesStage2SpikeBank(playerBounds,
        x_, floorY_, width_, currentHeight_);
}

void ProximityHazard::draw(sf::RenderTarget& target) const
{
    if (!active_ || currentHeight_ <= 0.f) return;
    const RectF spike = bounds();
    drawStage2SpikeBank(target, spike.x, spike.bottom(), spike.width,
        spike.height, sf::Color(193, 53, 44));
}

CrushingCeiling::CrushingCeiling(int triggerId, RectF initialBounds,
    float landingBottomY, float fallSpeed)
    : triggerId_(triggerId), initialBounds_(initialBounds), bounds_(initialBounds),
      landingTopY_(landingBottomY - initialBounds.height), fallSpeed_(fallSpeed)
{
}

void CrushingCeiling::reset()
{
    bounds_ = initialBounds_;
    state_ = State::Waiting;
    justLanded_ = false;
}

void CrushingCeiling::onTrigger(int triggerId)
{
    if (triggerId == triggerId_ && state_ == State::Waiting)
        state_ = State::Falling;
}

void CrushingCeiling::update(float dt)
{
    if (state_ == State::Landed)
    {
        justLanded_ = false;
        return;
    }
    if (state_ != State::Falling) return;

    const float nextY = bounds_.y + fallSpeed_ * std::max(0.f, dt);
    if (nextY >= landingTopY_)
    {
        // Do not leave a sub-pixel remainder at rest: GameModel compares this exact
        // top edge when resolving a landing on the new step.
        bounds_.y = landingTopY_;
        state_ = State::Landed;
        justLanded_ = true;
        return;
    }
    bounds_.y = nextY;
}

bool CrushingCeiling::needsUpdate() const
{
    return state_ == State::Falling || justLanded_;
}

bool CrushingCeiling::isDangerous() const
{
    // Falling is hazardous for the complete descent. justLanded_ preserves that
    // verdict in the snap frame before the stationary step becomes safe to stand on.
    return state_ == State::Falling || justLanded_;
}

bool CrushingCeiling::isFalling() const
{
    return state_ == State::Falling;
}

bool CrushingCeiling::isSolid() const
{
    return state_ == State::Landed;
}

void CrushingCeiling::draw(sf::RenderTarget& target) const
{
    render::box(target, bounds_.x, bounds_.y, bounds_.width, bounds_.height, render::earth);
    render::box(target, bounds_.x, bounds_.y + bounds_.height - 5.f,
        bounds_.width, 5.f, state_ == State::Falling ? sf::Color(193, 53, 44) : render::ink);
}
}
