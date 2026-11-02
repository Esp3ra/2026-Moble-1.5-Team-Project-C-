#include "GameModel.hpp"

#include "IStage.hpp"

#include <algorithm>
#include <limits>

namespace devil
{
namespace
{
bool overlapsVertically(const RectF& first, const RectF& second)
{
    return first.y < second.bottom() && first.bottom() > second.y;
}

bool overlapsHorizontally(const RectF& first, const RectF& second)
{
    return first.x < second.right() && first.right() > second.x;
}
}

Game::Game(IStage& stage) : stage_(&stage)
{
    resetPlayerAndStage();
}

IStage& Game::stage() { return *stage_; }
const IStage& Game::stage() const { return *stage_; }

void Game::resetPlayerAndStage()
{
    stage_->reset();
    player = {};
    const Vec2 spawn = stage_->spawnPoint();
    player.x = spawn.x;
    player.y = spawn.y;
    player.grounded = true;
    doorEntryStart_ = spawn;
    phase = Phase::Playing;
    phaseTime = 0.f;
}

void Game::startStage(IStage& nextStage, bool preserveRunStats)
{
    const int savedDeaths = preserveRunStats ? deaths : 0;
    const float savedElapsed = preserveRunStats ? elapsed : 0.f;
    stage_ = &nextStage;
    resetPlayerAndStage();
    deaths = savedDeaths;
    elapsed = savedElapsed;
}

void Game::retry()
{
    const int savedDeaths = deaths;
    const float savedElapsed = elapsed;
    resetPlayerAndStage();
    deaths = savedDeaths;
    elapsed = savedElapsed;
}

void Game::newRun()
{
    deaths = 0;
    elapsed = 0.f;
    resetPlayerAndStage();
}

void Game::die()
{
    if (phase != Phase::Playing) return;
    phase = Phase::Dying;
    phaseTime = 0.f;
    ++deaths;
}

void Game::update(Input input, float dt)
{
    if (phase == Phase::Cleared)
    {
        phaseTime += dt;
        return;
    }

    if (phase == Phase::EnteringDoor)
    {
        elapsed += dt;
        phaseTime += dt;
        const float amount = std::min(1.f, phaseTime / GameConfig::doorEnterDuration);
        const float smooth = amount * amount * (3.f - 2.f * amount);
        const Vec2 goal = stage_->goalPoint();
        player.x = doorEntryStart_.x + (goal.x - doorEntryStart_.x) * smooth;
        player.y = doorEntryStart_.y + (goal.y - doorEntryStart_.y) * smooth;
        player.walkTime += dt * 13.f;
        player.facing = goal.x >= doorEntryStart_.x ? 1 : -1;
        if (amount >= 1.f)
        {
            phase = Phase::Cleared;
            phaseTime = 0.f;
            player.velocityY = 0.f;
            player.grounded = true;
        }
        return;
    }

    elapsed += dt;
    if (phase == Phase::Dying)
    {
        phaseTime += dt;
        if (phaseTime >= GameConfig::deathDelay) retry();
        return;
    }

    const int direction = std::clamp(input.direction, -1, 1);
    if (direction != 0)
    {
        player.facing = direction;
        player.walkTime += dt * 13.f;
    }
    else
    {
        player.walkTime = 0.f;
    }

    player.jumpBuffer = input.jumpPressed ? .09f : std::max(0.f, player.jumpBuffer - dt);
    player.coyoteTime = player.grounded ? .055f : std::max(0.f, player.coyoteTime - dt);
    if (player.jumpBuffer > 0.f && player.coyoteTime > 0.f)
    {
        player.velocityY = -GameConfig::jumpSpeed;
        player.grounded = false;
        player.coyoteTime = 0.f;
        player.jumpBuffer = 0.f;
    }

    const RectF oldHorizontalBounds = player.bounds();
    player.x = std::clamp(player.x + static_cast<float>(direction) * GameConfig::speed * dt,
        GameConfig::left, GameConfig::right - GameConfig::playerWidth);

    if (direction != 0)
    {
        for (const RectF& solid : stage_->solids())
        {
            const RectF moved = player.bounds();
            if (!overlapsVertically(moved, solid)) continue;
            if (direction > 0 && oldHorizontalBounds.right() <= solid.x + .1f && moved.right() > solid.x)
                player.x = std::min(player.x, solid.x - GameConfig::playerWidth);
            else if (direction < 0 && oldHorizontalBounds.x >= solid.right() - .1f && moved.x < solid.right())
                player.x = std::max(player.x, solid.right());
        }
    }

    stage_->update(player, dt);

    const float oldY = player.y;
    const float oldFeet = player.feet();
    player.velocityY += GameConfig::gravity * dt;
    player.y += player.velocityY * dt;
    player.grounded = false;

    if (player.y < stage_->ceilingY())
    {
        player.y = stage_->ceilingY();
        player.velocityY = std::max(0.f, player.velocityY);
    }

    if (player.velocityY >= 0.f)
    {
        float landingY = std::numeric_limits<float>::max();
        const RectF moved = player.bounds();
        for (const RectF& solid : stage_->solids())
        {
            if (!overlapsHorizontally(moved, solid)) continue;
            if (oldFeet <= solid.y + .1f && moved.bottom() >= solid.y)
                landingY = std::min(landingY, solid.y - GameConfig::playerHeight);
        }
        if (landingY != std::numeric_limits<float>::max())
        {
            player.y = landingY;
            player.velocityY = 0.f;
            player.grounded = true;
        }
    }
    else
    {
        float ceilingY = -std::numeric_limits<float>::max();
        const RectF moved = player.bounds();
        for (const RectF& solid : stage_->solids())
        {
            if (!overlapsHorizontally(moved, solid)) continue;
            if (oldY >= solid.bottom() - .1f && moved.y <= solid.bottom())
                ceilingY = std::max(ceilingY, solid.bottom());
        }
        if (ceilingY != -std::numeric_limits<float>::max())
        {
            player.y = ceilingY;
            player.velocityY = 0.f;
        }
    }

    if (stage_->touchesHazard(player.bounds()) || player.y > GameConfig::height + 12.f)
    {
        die();
        return;
    }

    if (stage_->isAtGoal(player))
    {
        phase = Phase::EnteringDoor;
        phaseTime = 0.f;
        player.velocityY = 0.f;
        player.grounded = true;
        doorEntryStart_ = {player.x, player.y};
    }
}
}
