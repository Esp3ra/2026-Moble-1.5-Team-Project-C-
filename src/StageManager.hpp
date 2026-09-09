#pragma once

#include "GameModel.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace devil
{
class IStage;

// 스테이지 등록, 순차 전환, 전체 재시작을 한 곳에서 처리합니다.
// Stage2 이후를 합칠 때는 StageManager.cpp의 등록 목록만 공통 담당자가 수정합니다.
class StageManager
{
public:
    StageManager();
    explicit StageManager(std::vector<std::unique_ptr<IStage>> stages);
    ~StageManager();

    StageManager(const StageManager&) = delete;
    StageManager& operator=(const StageManager&) = delete;

    Game& game() { return *game_; }
    const Game& game() const { return *game_; }
    IStage& currentStage();
    const IStage& currentStage() const;
    std::size_t currentIndex() const { return currentIndex_; }
    std::size_t stageCount() const { return stages_.size(); }
    bool isLastStage() const { return currentIndex_ + 1 >= stages_.size(); }

    void update(Input input, float dt = GameConfig::step);
    void retry();
    void newRun();

private:
    std::vector<std::unique_ptr<IStage>> stages_;
    std::unique_ptr<Game> game_;
    std::size_t currentIndex_ = 0;
    void loadStage(std::size_t index, bool preserveRunStats);
};
}
