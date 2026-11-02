#pragma once

#include "Common.hpp"

#include <vector>

namespace sf { class RenderTarget; }

namespace devil
{
// Non-owning observer contract.  A Coin owns neither its listeners nor their lifetime.
class ITriggerListener
{
public:
    virtual ~ITriggerListener() = default;
    virtual void onTrigger(int triggerId) = 0;
};

class Coin final
{
public:
    Coin(int triggerId, RectF bounds);

    void subscribe(ITriggerListener& listener);
    void reset();
    void update(const RectF& playerBounds, float dt);
    bool isVisible() const;
    bool isCollected() const { return collected_; }
    void draw(sf::RenderTarget& target) const;

private:
    int triggerId_ = 0;
    RectF bounds_{};
    std::vector<ITriggerListener*> listeners_;
    float collectionTime_ = 0.f;
    bool collected_ = false;

    void notifyListeners();
};

class DisappearingFloor final : public ITriggerListener
{
public:
    DisappearingFloor(int triggerId, RectF bounds);

    void reset();
    void onTrigger(int triggerId) override;
    bool isActive() const { return active_; }
    const RectF& bounds() const { return bounds_; }

private:
    int triggerId_ = 0;
    RectF bounds_{};
    bool active_ = true;
};

// Common hazard seam: Stage3 can update, cull, collide with, and render each hazard
// through one collection while each concrete trap keeps its own behavior.
class StageHazard
{
public:
    virtual ~StageHazard() = default;

    virtual void reset() = 0;
    virtual void update(const PlayerState& player, float dt) = 0;
    virtual bool needsUpdate(const PlayerState& player) const = 0;
    virtual bool isActive() const = 0;
    virtual RectF bounds() const = 0;
    virtual bool touches(const RectF& playerBounds) const
    {
        return playerBounds.intersects(bounds());
    }
    virtual void draw(sf::RenderTarget& target) const = 0;
};

class MovingWall final : public StageHazard, public ITriggerListener
{
public:
    MovingWall(int triggerId, RectF initialBounds, float speed, float stopX);

    void reset() override;
    void onTrigger(int triggerId) override;
    void update(const PlayerState& player, float dt) override;
    bool needsUpdate(const PlayerState&) const override { return active_; }
    bool isActive() const override { return active_; }
    RectF bounds() const override { return bounds_; }
    void draw(sf::RenderTarget& target) const override;

private:
    int triggerId_ = 0;
    RectF initialBounds_{};
    RectF bounds_{};
    float speed_ = 0.f;
    float stopX_ = 0.f;
    bool active_ = false;
};

class ProximityHazard final : public StageHazard
{
public:
    ProximityHazard(float x, float floorY, float width, float maximumHeight,
        float triggerX, float riseSpeed);

    void reset() override;
    void update(const PlayerState& player, float dt) override;
    bool needsUpdate(const PlayerState& player) const override;
    bool isActive() const override { return active_; }
    RectF bounds() const override;
    bool touches(const RectF& playerBounds) const override;
    void draw(sf::RenderTarget& target) const override;

private:
    float x_ = 0.f;
    float floorY_ = 0.f;
    float width_ = 0.f;
    float maximumHeight_ = 0.f;
    float triggerX_ = 0.f;
    float riseSpeed_ = 0.f;
    float currentHeight_ = 0.f;
    bool active_ = false;
};

class CrushingCeiling final : public ITriggerListener
{
public:
    CrushingCeiling(int triggerId, RectF initialBounds, float landingBottomY, float fallSpeed);

    void reset();
    void onTrigger(int triggerId) override;
    void update(float dt);
    bool needsUpdate() const;
    bool isDangerous() const;
    bool isFalling() const;
    bool isSolid() const;
    RectF bounds() const { return bounds_; }
    void draw(sf::RenderTarget& target) const;

private:
    enum class State { Waiting, Falling, Landed };

    int triggerId_ = 0;
    RectF initialBounds_{};
    RectF bounds_{};
    float landingTopY_ = 0.f;
    float fallSpeed_ = 0.f;
    State state_ = State::Waiting;
    bool justLanded_ = false;
};
}
