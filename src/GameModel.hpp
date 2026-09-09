#pragma once

#include "Common.hpp"

namespace devil
{
class IStage;

// 모든 스테이지에 동일하게 적용되는 플레이어 물리와 게임 상태입니다.
class Game
{
public:
    explicit Game(IStage& stage);

    PlayerState player;
    float elapsed = 0.f;
    float phaseTime = 0.f;
    int deaths = 0;
    Phase phase = Phase::Playing;

    float centerX() const { return player.centerX(); }
    float feet() const { return player.feet(); }

    IStage& stage();
    const IStage& stage() const;
    void startStage(IStage& nextStage, bool preserveRunStats = true);
    void retry();
    void newRun();
    void die();
    void update(Input input, float dt = GameConfig::step);

private:
    IStage* stage_ = nullptr;
    Vec2 doorEntryStart_{};
    void resetPlayerAndStage();
};
}
